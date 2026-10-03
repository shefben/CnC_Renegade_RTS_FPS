/*
**	OpenW3D
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/***********************************************************************************************
 ***                            Confidential - Westwood Studios                              ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : OpenW3D                                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwphys/hybridrealization.cpp                 $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * See hybridrealization.h.  Section 12.1's solver is written exactly as the plan states it, step   *
 * by step, because every step is a place two implementations could disagree and the checksum       *
 * would say so: forward propagation, backward propagation, the clamp walk, quantization, and the    *
 * revalidation that quantization can fail.                                                          *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#include "hybridrealization.h"

#include <math.h>
#include <string.h>


static inline float Max2(float a,float b)	{ return (a > b) ? a : b; }
static inline float Min2(float a,float b)	{ return (a < b) ? a : b; }
static inline float Clamp3(float v,float lo,float hi)	{ return (v < lo) ? lo : ((v > hi) ? hi : v); }
static const float	PI_OVER_180	= 3.14159265358979323846f / 180.0f;

//	The clamp walk's rounding allowance, relative to the height: a few float units, far below the position quantum.
static const float	CLAMP_ROUNDING	= 1.0e-5f;

//	The interval a structure sample has: no ground limit at all, only the grade propagation.
static const float	UNBOUNDED_Z	= 1.0e30f;

//	Section 12.1's sample spacing: `min(W/2, A/3)`.
static const float	SPACING_W	= 2.0f;
static const float	SPACING_A	= 3.0f;


const char * Hybrid_Profile_Failure_Name(HybridProfileFailure failure)
{
	switch (failure) {
		case HYBRID_PROFILE_OK:							return "ok";
		case HYBRID_PROFILE_INVALID_INPUT:			return "invalid_input";
		case HYBRID_PROFILE_FORWARD_EMPTY:			return "forward_propagation_empty";
		case HYBRID_PROFILE_BACKWARD_EMPTY:			return "backward_propagation_empty";
		case HYBRID_PROFILE_CLAMP_EMPTY:				return "clamp_empty";
		case HYBRID_PROFILE_GRADE_AFTER_QUANTIZE:	return "grade_after_quantization";
		default:												break;
	}
	return "unknown";
}


float Hybrid_Profile_Sample_Spacing(const HybridConstants & constants)
{
	return Max2(Min2(constants.W / SPACING_W,constants.A / SPACING_A),Max2(constants.Q,1.0e-3f));
}


/***********************************************************************************************
**	Section 12.1 -- the deterministic vertical-profile solver.
***********************************************************************************************/
bool Hybrid_Solve_Vertical_Profile(const DynamicVectorClass<HybridProfileSample> & samples,
											  float max_cut_depth,float max_fill_height,float max_grade_radians,
											  float quantum,HybridSolvedProfile * out,HybridProfileFailure * failure_out)
{
	if (failure_out != nullptr) *failure_out = HYBRID_PROFILE_OK;
	if (out == nullptr) return false;
	out->Reset();

	const int count = samples.Count();
	if ((count < 2) || !(quantum > 0.0f) || !(max_grade_radians > 0.0f)) {
		if (failure_out != nullptr) *failure_out = HYBRID_PROFILE_INVALID_INPUT;
		return false;
	}
	for (int i = 1; i < count; ++i) {
		if (samples[i].Station <= samples[i - 1].Station) {
			if (failure_out != nullptr) *failure_out = HYBRID_PROFILE_INVALID_INPUT;
			return false;
		}
	}

	const float g = (float)::tan((double)max_grade_radians);

	/*
	**	The feasibility intervals.  A sample a compiled crossing or junction pins holds a quantized height --
	**	this solver's own output, quantized in step 4 -- so it stands for the cell of real heights that quantize
	**	to it, and propagation works on that cell: a deck end this solver graded at the class maximum is then
	**	re-solvable from its own pinned heights, where an exact real pin would sit up to half a quantum below
	**	the line it was cut from.  Step 4 still returns the pin itself and revalidates every grade.  Everything
	**	else may sit anywhere in its `RoadVerticalInterval`.
	*/
	const float pin_cell = 0.5f * HYBRID_QUANTUM_POSITION;
	DynamicVectorClass<float> lo;
	DynamicVectorClass<float> hi;
	for (int i = 0; i < count; ++i) {
		const HybridProfileSample & s = samples[i];
		if (s.Fixed) {
			lo.Add(s.FixedZ - pin_cell);
			hi.Add(s.FixedZ + pin_cell);
		} else if (s.Unbounded) {
			lo.Add(-UNBOUNDED_Z);
			hi.Add(UNBOUNDED_Z);
		} else {
			const HybridVerticalInterval interval = Hybrid_Road_Vertical_Interval(s.TerrainZ,max_cut_depth,
																								  max_fill_height);
			lo.Add(interval.Low);
			hi.Add(interval.High);
		}
	}

	//	1. Forward propagation.
	for (int i = 1; i < count; ++i) {
		const float ds = samples[i].Station - samples[i - 1].Station;
		lo[i] = Max2(lo[i],lo[i - 1] - (g * ds));
		hi[i] = Min2(hi[i],hi[i - 1] + (g * ds));
		if (lo[i] > hi[i]) {
			if (failure_out != nullptr) *failure_out = HYBRID_PROFILE_FORWARD_EMPTY;
			return false;
		}
	}

	//	2. Backward propagation.
	for (int i = count - 2; i >= 0; --i) {
		const float ds = samples[i + 1].Station - samples[i].Station;
		lo[i] = Max2(lo[i],lo[i + 1] - (g * ds));
		hi[i] = Min2(hi[i],hi[i + 1] + (g * ds));
		if (lo[i] > hi[i]) {
			if (failure_out != nullptr) *failure_out = HYBRID_PROFILE_BACKWARD_EMPTY;
			return false;
		}
	}

	//	3. The clamp walk: stay as close to the ground as the intervals and the grade allow.
	//	A pinned sample aims at its pin, everything else at the ground.
	DynamicVectorClass<float> z;
	z.Add(Clamp3(samples[0].Fixed ? samples[0].FixedZ : samples[0].TerrainZ,lo[0],hi[0]));
	for (int i = 1; i < count; ++i) {
		const float ds = samples[i].Station - samples[i - 1].Station;
		float floor = Max2(lo[i],z[i - 1] - (g * ds));
		const float ceiling = Min2(hi[i],z[i - 1] + (g * ds));
		/*
		**	After both propagations this interval is never empty in exact arithmetic: `z[i-1]` lies inside its own
		**	propagated interval, which is within `g * ds` of this one.  A walk standing exactly on a propagated bound
		**	can still overshoot it by a rounding unit -- `(hi + g*ds) - g*ds > hi` -- and that is not a refusal: it
		**	takes the bound.  Step 4 quantizes and revalidates every grade regardless.
		*/
		if (floor > ceiling) {
			if ((floor - ceiling) > (CLAMP_ROUNDING * Max2(1.0f,(float)fabs((double)ceiling)))) {
				if (failure_out != nullptr) *failure_out = HYBRID_PROFILE_CLAMP_EMPTY;
				return false;
			}
			floor = ceiling;
		}
		z.Add(Clamp3(samples[i].Fixed ? samples[i].FixedZ : samples[i].TerrainZ,floor,ceiling));
	}

	/*
	**	4. Quantize to `Q`, then revalidate every adjacent grade against the class maximum plus the
	**	angular quantum.  A pinned sample returns its pin exactly: the crossing that pinned it compiled a
	**	quantized height already, and the walk kept it inside that height's cell.
	*/
	for (int i = 0; i < count; ++i) z[i] = samples[i].Fixed ? samples[i].FixedZ : Hybrid_Quantize_Position(z[i]);

	float steepest = 0.0f;
	for (int i = 1; i < count; ++i) {
		const float ds = Max2(samples[i].Station - samples[i - 1].Station,quantum);
		const float grade = (float)::atan((double)((float)fabs((double)(z[i] - z[i - 1])) / ds));
		if (grade > steepest) steepest = grade;
		if (grade > (max_grade_radians + HYBRID_QUANTUM_ANGLE)) {
			if (failure_out != nullptr) *failure_out = HYBRID_PROFILE_GRADE_AFTER_QUANTIZE;
			return false;
		}
	}

	out->Z = z;
	out->SteepestGrade = steepest;
	for (int i = 0; i < count; ++i) {
		if (samples[i].Unbounded) continue;			// a structure's own surface asks nothing of the ground
		const float correction = z[i] - samples[i].TerrainZ;
		if (correction < 0.0f)	out->DeepestCut = Max2(out->DeepestCut,-correction);
		else							out->HighestFill = Max2(out->HighestFill,correction);
	}
	return true;
}


/***********************************************************************************************
**	Section 12.2 -- the cross-section.
***********************************************************************************************/
float Hybrid_Road_Cross_Section_Half_Width(HybridRoadClass road_class,float w,float a)
{
	return (0.5f * Hybrid_Roadbed_Width(road_class,w)) + Hybrid_Road_Outer_Blend_Width(w,a);
}


float Hybrid_Road_Cross_Section_Height(float lateral,float solved_z,float terrain_z_at_offset,
												  float half_roadbed,float outer_blend)
{
	const float distance = (float)fabs((double)lateral);

	//	1 and 2: the roadbed is level, and the shoulder edge is at roadbed height.
	if (distance <= half_roadbed) return solved_z;

	//	3: smoothstep from the shoulder edge to untouched terrain across exactly `outer_blend`.
	if (!(outer_blend > 0.0f) || (distance >= (half_roadbed + outer_blend))) return terrain_z_at_offset;

	const float s = (distance - half_roadbed) / outer_blend;
	const float weight = s * s * (3.0f - (2.0f * s));
	return solved_z + ((terrain_z_at_offset - solved_z) * weight);
}


/***********************************************************************************************
**	The interval sampler.
***********************************************************************************************/
bool Hybrid_Sample_Route_Interval(const HybridRoute & route,int first_point,int last_point,float spacing,
											 HybridRouteHeightSampleFunc sample,void * context,
											 DynamicVectorClass<HybridProfileSample> * out)
{
	if ((out == nullptr) || (sample == nullptr) || !(spacing > 0.0f)) return false;
	out->Delete_All();

	const int points = route.Points.Count();
	if ((points < 2) || (first_point < 0) || (last_point >= points) || (last_point <= first_point)) return false;
	if (route.Stations.Count() != points) return false;

	return Hybrid_Sample_Polyline_Range(route.Points,route.Stations,route.Stations[first_point],
													route.Stations[last_point],spacing,sample,context,out);
}


//	The segment of a station-ordered polyline holding `station`, and the parameter on it.
static int Polyline_Segment_At(const DynamicVectorClass<float> & stations,float station,int from_segment,float * t_out)
{
	const int count = stations.Count();
	int segment = (from_segment > 0) ? from_segment : 0;
	while ((segment < (count - 2)) && (stations[segment + 1] <= station)) segment++;
	const float sa = stations[segment];
	const float sb = stations[segment + 1];
	*t_out = (sb > sa) ? Clamp3((station - sa) / (sb - sa),0.0f,1.0f) : 0.0f;
	return segment;
}


Vector2 Hybrid_Polyline_Point_At(const DynamicVectorClass<Vector2> & points,
											const DynamicVectorClass<float> & stations,float station)
{
	const int count = points.Count();
	if ((count == 0) || (stations.Count() != count)) return Vector2(0.0f,0.0f);
	if ((count == 1) || (station <= stations[0])) return points[0];
	if (station >= stations[count - 1]) return points[count - 1];

	float t = 0.0f;
	const int segment = Polyline_Segment_At(stations,station,0,&t);
	const Vector2 & a = points[segment];
	const Vector2 & b = points[segment + 1];
	//	A station that is a polyline point gives that point exactly, not an interpolation that rounds near it.
	if (t <= 0.0f) return a;
	if (t >= 1.0f) return b;
	return Vector2(a.X + ((b.X - a.X) * t),a.Y + ((b.Y - a.Y) * t));
}


/***********************************************************************************************
**	Section 12.1 over a stretch of road with its structures.
***********************************************************************************************/
bool Hybrid_Solve_Road_Stretch(const DynamicVectorClass<Vector2> & points,const DynamicVectorClass<float> & stations,
										 float from_station,float to_station,
										 const DynamicVectorClass<HybridProfileStructure> & structures,float spacing,
										 HybridRouteHeightSampleFunc sample,void * context,float max_cut_depth,
										 float max_fill_height,float max_grade_radians,float quantum,
										 DynamicVectorClass<HybridProfileSample> * samples_out,
										 HybridSolvedProfile * solved_out,DynamicVectorClass<float> * start_z_out,
										 DynamicVectorClass<float> * end_z_out,HybridProfileFailure * failure_out)
{
	if (failure_out != nullptr) *failure_out = HYBRID_PROFILE_INVALID_INPUT;
	if ((samples_out == nullptr) || (solved_out == nullptr) || (to_station <= from_station)) return false;
	samples_out->Delete_All();
	solved_out->Reset();
	if (start_z_out != nullptr) start_z_out->Delete_All();
	if (end_z_out != nullptr) end_z_out->Delete_All();

	const float q = Max2(quantum,HYBRID_QUANTUM_POSITION);

	//	The cut stations: the stretch's own ends and every structure end strictly inside it, in order.
	DynamicVectorClass<float> cuts;
	cuts.Add(from_station);
	for (int s = 0; s < structures.Count(); ++s) {
		const float ends[2] = { structures[s].From,structures[s].To };
		for (int e = 0; e < 2; ++e) {
			if ((ends[e] <= (from_station + q)) || (ends[e] >= (to_station - q))) continue;
			int slot = cuts.Count();
			cuts.Add(ends[e]);
			while ((slot > 0) && (cuts[slot - 1] > ends[e])) {
				cuts[slot] = cuts[slot - 1];
				slot--;
			}
			cuts[slot] = ends[e];
		}
	}
	cuts.Add(to_station);

	for (int c = 0; (c + 1) < cuts.Count(); ++c) {
		const float from = cuts[c];
		const float to = cuts[c + 1];
		if ((to - from) <= q) continue;
		DynamicVectorClass<HybridProfileSample> piece;
		if (!Hybrid_Sample_Polyline_Range(points,stations,from,to,spacing,sample,context,&piece)) return false;
		for (int i = 0; i < piece.Count(); ++i) {
			if ((samples_out->Count() > 0) && (i == 0)) continue;			// the join is the previous piece's end
			piece[i].Station += (from - from_station);
			samples_out->Add(piece[i]);
		}
	}
	if (samples_out->Count() < 2) return false;

	//	Each structure's two ends as sample indices, and the surface it holds between them.
	DynamicVectorClass<int> first_index;
	DynamicVectorClass<int> last_index;
	for (int s = 0; s < structures.Count(); ++s) {

		const HybridProfileStructure & structure = structures[s];
		const float from = structure.From - from_station;
		const float to = structure.To - from_station;
		int i_from = -1;
		int i_to = -1;
		for (int i = 0; i < samples_out->Count(); ++i) {
			const float at = (*samples_out)[i].Station;
			if ((at >= (from - q)) && (at <= (to + q))) {
				if (i_from < 0) i_from = i;
				i_to = i;
			}
		}
		first_index.Add(i_from);
		last_index.Add(i_to);
		if ((i_from < 0) || (i_to < i_from)) continue;

		HybridProfileSample & head = (*samples_out)[i_from];
		HybridProfileSample & tail = (*samples_out)[i_to];
		const bool whole = ((from + q) >= 0.0f) && ((to - q) <= (to_station - from_station)) &&
								 (head.Station <= (from + q)) && (tail.Station >= (to - q));

		if (structure.Mode == HYBRID_PROFILE_STRUCTURE_LEVEL) {
			for (int i = i_from; i <= i_to; ++i) {
				(*samples_out)[i].Fixed		= true;
				(*samples_out)[i].FixedZ	= structure.StartZ;
			}
			continue;
		}

		//	CARRIED: the line between its two ends, pinned or still to be solved.
		const float z_from = structure.EndsFixed ? structure.StartZ : head.TerrainZ;
		const float z_to = structure.EndsFixed ? structure.EndZ : tail.TerrainZ;
		const float run = Max2(tail.Station - head.Station,q);
		for (int i = i_from; i <= i_to; ++i) {
			HybridProfileSample & one = (*samples_out)[i];
			const bool end = (i == i_from) || (i == i_to);
			if (end && structure.EndsFixed && whole) {
				one.Fixed	= true;
				one.FixedZ	= (i == i_from) ? structure.StartZ : structure.EndZ;
				continue;
			}
			if (end) continue;
			const float ratio = (one.Station - head.Station) / run;
			one.Unbounded	= true;
			one.TerrainZ	= z_from + ((z_to - z_from) * ratio);
		}
	}

	if (!Hybrid_Solve_Vertical_Profile(*samples_out,max_cut_depth,max_fill_height,max_grade_radians,quantum,
												  solved_out,failure_out)) return false;

	for (int s = 0; s < structures.Count(); ++s) {
		const bool inside = (first_index[s] >= 0) && (last_index[s] >= first_index[s]);
		if (start_z_out != nullptr) start_z_out->Add(inside ? solved_out->Z[first_index[s]] : structures[s].StartZ);
		if (end_z_out != nullptr) end_z_out->Add(inside ? solved_out->Z[last_index[s]] : structures[s].EndZ);
	}
	if (failure_out != nullptr) *failure_out = HYBRID_PROFILE_OK;
	return true;
}


bool Hybrid_Sample_Polyline_Range(const DynamicVectorClass<Vector2> & points,
											 const DynamicVectorClass<float> & stations,float from_station,
											 float to_station,float spacing,HybridRouteHeightSampleFunc sample,
											 void * context,DynamicVectorClass<HybridProfileSample> * out)
{
	if ((out == nullptr) || (sample == nullptr) || !(spacing > 0.0f)) return false;
	out->Delete_All();

	const int count = points.Count();
	if ((count < 2) || (stations.Count() != count)) return false;
	if ((from_station < stations[0]) || (to_station > stations[count - 1])) return false;

	/*
	**	The range's start, then every whole multiple of `spacing` along the polyline in station order, then
	**	the exact end.  A multiple is placed on the segment that actually contains its station, so no gap ever
	**	exceeds `spacing` and the two endpoints are exact.
	*/
	const float end_station = to_station - from_station;
	if (end_station <= 0.0f) return false;

	HybridProfileSample first;
	first.Position = Hybrid_Polyline_Point_At(points,stations,from_station);
	first.Station = 0.0f;
	if (!sample(first.Position,&first.TerrainZ,context)) return false;
	out->Add(first);

	int segment = 0;
	//	Every multiple strictly inside the range, so the final gap to the exact end is never over `spacing`;
	//	a multiple within a quantum of the end is the end.
	for (float next = spacing; next < (end_station - HYBRID_QUANTUM_POSITION); next += spacing) {

		float t = 0.0f;
		segment = Polyline_Segment_At(stations,from_station + next,segment,&t);
		const Vector2 & a = points[segment];
		const Vector2 & b = points[segment + 1];

		HybridProfileSample s;
		s.Position.Set(a.X + ((b.X - a.X) * t),a.Y + ((b.Y - a.Y) * t));
		s.Station = next;
		if (!sample(s.Position,&s.TerrainZ,context)) return false;
		out->Add(s);
	}

	HybridProfileSample end;
	end.Position = Hybrid_Polyline_Point_At(points,stations,to_station);
	end.Station = end_station;
	if (!sample(end.Position,&end.TerrainZ,context)) return false;
	out->Add(end);
	return out->Count() >= 2;
}


/***********************************************************************************************
**	Section 12.2 / 12.2a -- the realization plan.
***********************************************************************************************/

const char * Hybrid_Bridge_Piece_Kind_Name(HybridBridgePieceKind kind)
{
	switch (kind) {
		case HYBRID_BRIDGE_PIECE_WHOLE:						return "whole";
		case HYBRID_BRIDGE_PIECE_START_ABUTMENT:			return "start_abutment";
		case HYBRID_BRIDGE_PIECE_END_ABUTMENT:				return "end_abutment";
		case HYBRID_BRIDGE_PIECE_CENTER:						return "center";
		case HYBRID_BRIDGE_PIECE_SHORT_CENTER:				return "short_center";
		case HYBRID_BRIDGE_PIECE_SUPPORT:					return "support";
		case HYBRID_BRIDGE_PIECE_DAMAGED_TRAVERSABLE:	return "damaged_traversable";
		case HYBRID_BRIDGE_PIECE_BROKEN:						return "broken";
		default:														break;
	}
	return "unknown";
}


const char * Hybrid_Realization_Failure_Name(HybridRealizationFailure failure)
{
	switch (failure) {
		case HYBRID_REALIZATION_OK:				return "ok";
		case HYBRID_REALIZATION_INVALID_INPUT:	return "invalid_input";
		case HYBRID_REALIZATION_ROAD:				return "road";
		case HYBRID_REALIZATION_PROFILE:			return "profile";
		default:											break;
	}
	return "unknown";
}


void HybridRealizationPlan::Reset(void)
{
	Roads.Delete_All();
	Bridges.Delete_All();
	Checksum = 0;
}


const HybridRealizedRoad * HybridRealizationPlan::Find_Road(unsigned int edge_id) const
{
	for (int r = 0; r < Roads.Count(); ++r) {
		for (int e = 0; e < Roads[r].Edges.Count(); ++e) {
			if (Roads[r].Edges[e] == edge_id) return &Roads[r];
		}
	}
	return nullptr;
}


HybridRealizationParams::HybridRealizationParams(void) :
	MaxCutDepth(0.0f),MaxFillHeight(0.0f),LightVehicleWidth(0.0f),HeavyVehicleWidth(0.0f),HarvesterWidth(0.0f),
	MinVehicleLaneClearance(0.0f)
{
	memset(&Constants,0,sizeof(Constants));
}


static bool Realization_Fail(HybridRealizationResult * result,HybridRealizationFailure failure,unsigned int edge_id,
									  const char * detail)
{
	if (result != nullptr) {
		result->Failure	= failure;
		result->EdgeId		= edge_id;
		result->Detail		= detail;
	}
	return false;
}


static Vector2 Unit_Vector(const Vector2 & v)
{
	const float length = v.Length();
	return (length > 0.0f) ? Vector2(v.X / length,v.Y / length) : Vector2(0.0f,0.0f);
}


//	The station of the point on a station-ordered polyline nearest `point`, and how far off it lies.
static float Nearest_Station(const DynamicVectorClass<Vector2> & points,const DynamicVectorClass<float> & stations,
									  const Vector2 & point,float * distance_out)
{
	float best = 3.4e38f;
	float station = (stations.Count() > 0) ? stations[0] : 0.0f;
	for (int i = 1; i < points.Count(); ++i) {
		const Vector2 d = points[i] - points[i - 1];
		const float length2 = (d.X * d.X) + (d.Y * d.Y);
		float t = (length2 > 0.0f) ? ((((point.X - points[i - 1].X) * d.X) + ((point.Y - points[i - 1].Y) * d.Y)) / length2) : 0.0f;
		t = Clamp3(t,0.0f,1.0f);
		const float distance = (point - (points[i - 1] + (d * t))).Length();
		if (distance >= best) continue;
		best		= distance;
		station	= stations[i - 1] + ((stations[i] - stations[i - 1]) * t);
	}
	if (distance_out != nullptr) *distance_out = best;
	return station;
}


//	A realized road's solved surface at `point`: the height at the nearest station of its samples.
static float Realized_Z_At(const HybridRealizedRoad & road,const Vector2 & point)
{
	const int count = road.Samples.Count();
	if (count == 0) return 0.0f;
	float best = 3.4e38f;
	float z = road.Samples[0].Z;
	for (int i = 1; i < count; ++i) {
		const Vector2 a = road.Samples[i - 1].Position;
		const Vector2 d = road.Samples[i].Position - a;
		const float length2 = (d.X * d.X) + (d.Y * d.Y);
		float t = (length2 > 0.0f) ? ((((point.X - a.X) * d.X) + ((point.Y - a.Y) * d.Y)) / length2) : 0.0f;
		t = Clamp3(t,0.0f,1.0f);
		const float distance = (point - (a + (d * t))).Length();
		if (distance >= best) continue;
		best	= distance;
		z		= road.Samples[i - 1].Z + ((road.Samples[i].Z - road.Samples[i - 1].Z) * t);
	}
	return z;
}


static bool Carries_Edge(const DynamicVectorClass<unsigned int> & edges,unsigned int edge_id)
{
	for (int e = 0; e < edges.Count(); ++e) {
		if (edges[e] == edge_id) return true;
	}
	return false;
}


//	Does one of these logical edges carry `node` -- at an end as Phase 10 realized it, or as a waypoint?
static bool Road_Carries_Node(const HybridTopology & topology,const HybridRouteSet & routes,
										const DynamicVectorClass<unsigned int> & edges,unsigned int node)
{
	if (node == 0) return false;
	for (int e = 0; e < edges.Count(); ++e) {
		const HybridTopologyEdge * edge = topology.Find_Edge(edges[e]);
		if (edge == nullptr) continue;
		if ((Hybrid_Route_Edge_End(topology,routes,*edge,false) == node) || (Hybrid_Route_Edge_End(topology,routes,*edge,true) == node)) {
			return true;
		}
		for (int w = 0; w < edge->Waypoints.Count(); ++w) {
			if (edge->Waypoints[w] == node) return true;
		}
	}
	return false;
}


/*
**	One junction surface held on a road: a level patch `2Q` long at `station`, the shared surface the junction
**	has there.  Section 12.1 holds every sample on it at `z` and propagates the class grade out of it.
*/
static void Add_Junction_Surface(DynamicVectorClass<HybridProfileStructure> * structures,int real_structures,float total,
											float station,float z,float q)
{
	float from = station - q;
	float to = station + q;
	if (from < 0.0f) {
		from	= 0.0f;
		to		= 2.0f * q;
	}
	if (to > total) {
		to		= total;
		from	= total - (2.0f * q);
	}
	//	A junction never stands on a deck or in a bore: those hold their own surface.
	for (int s = 0; s < real_structures; ++s) {
		if ((to >= ((*structures)[s].From - q)) && (from <= ((*structures)[s].To + q))) return;
	}
	//	One surface per junction point: a second record at the same place holds the same surface.
	for (int s = real_structures; s < structures->Count(); ++s) {
		if ((float)fabs((double)((*structures)[s].From - from)) <= q) return;
	}
	HybridProfileStructure surface;
	surface.From		= from;
	surface.To			= to;
	surface.Mode		= HYBRID_PROFILE_STRUCTURE_LEVEL;
	surface.EndsFixed	= true;
	surface.StartZ		= Hybrid_Quantize_Position(z);
	surface.EndZ		= surface.StartZ;
	structures->Add(surface);
}


//	The actual paved edges either side of one sample of a physical road (12.6.11, D-022, D-024).
static void Sample_Edges(const HybridRealizationParams & params,const HybridTopology & topology,
								 const HybridInteractionSet & interactions,const HybridRealizedRoad & road,
								 const HybridTopologyEdge & first_edge,float total,const Vector2 & travel,
								 HybridRealizedSample * sample)
{
	const HybridConstants & k = params.Constants;
	sample->Left	= -Hybrid_Road_Inner_Edge_Offset(road.RoadClass,k.W);
	sample->Right	= Hybrid_Road_Outer_Edge_Offset(road.RoadClass,k.W);

	HybridInterchangeConstants c;
	Hybrid_Interchange_Constants(k.W,&c);

	//	A ramp, stored L_s -> gore: inner (highway side) is left of its traffic, which an OFF_RAMP runs against.
	if (first_edge.Kind == HYBRID_EDGE_HIGHWAY_RAMP) {
		float inner = 0.0f;
		float outer = 0.0f;
		Hybrid_Ramp_Effective_Edges(c,first_edge.RampRoadbedExtra,total - sample->Station,&inner,&outer);
		const bool against = (first_edge.RampRole == HYBRID_RAMP_ROLE_OFF);
		sample->Left	= against ? outer : inner;
		sample->Right	= against ? inner : outer;
		return;
	}

	//	A highway: its ramp side widened to the compiled E(x) over each parallel interval it carries.
	if (road.RoadClass != HYBRID_ROAD_HIGHWAY_4) return;
	const Vector2 left_of_travel(-travel.Y,travel.X);
	for (int m = 0; m < interactions.MergeDiverge.Count(); ++m) {
		const HybridMergeDiverge & md = interactions.MergeDiverge[m];
		if (!Carries_Edge(road.Edges,md.HighwayEdgeId) || (md.Samples.Count() < 2)) continue;
		const HybridInterchange * ic = topology.Find_Interchange(md.InterchangeId);
		const HybridTopologyNode * anchor = (ic != nullptr) ? topology.Find_Node(ic->AnchorNode) : nullptr;
		if (anchor == nullptr) continue;
		const Vector2 offset = sample->Position - anchor->Position;
		const float chain = (offset.X * ic->Axis.X) + (offset.Y * ic->Axis.Y);
		const float lo = Min2(md.Samples[0].Station,md.Samples[md.Samples.Count() - 1].Station);
		const float hi = Max2(md.Samples[0].Station,md.Samples[md.Samples.Count() - 1].Station);
		if ((chain < (lo - k.Q)) || (chain > (hi + k.Q))) continue;

		float widened = md.Samples[0].E;
		for (int s = 1; s < md.Samples.Count(); ++s) {
			const HybridMergeDivergeSample & a = md.Samples[s - 1];
			const HybridMergeDivergeSample & b = md.Samples[s];
			const float s_lo = Min2(a.Station,b.Station);
			const float s_hi = Max2(a.Station,b.Station);
			if ((chain < s_lo) || (chain > s_hi)) continue;
			const float span = b.Station - a.Station;
			const float t = (span != 0.0f) ? Clamp3((chain - a.Station) / span,0.0f,1.0f) : 0.0f;
			widened = a.E + ((b.E - a.E) * t);
			break;
		}
		const Vector2 normal = (md.Side == HYBRID_INTERCHANGE_SIDE_RIGHT) ? Vector2(ic->Axis.Y,-ic->Axis.X)
																							 : Vector2(-ic->Axis.Y,ic->Axis.X);
		if (((left_of_travel.X * normal.X) + (left_of_travel.Y * normal.Y)) > 0.0f) {
			sample->Left = Max2(sample->Left,widened);
		} else {
			sample->Right = Max2(sample->Right,widened);
		}
	}
}


bool Hybrid_Plan_Realization(const HybridRealizationParams & params,const HybridTopology & topology,
									  const HybridRouteSet & routes,const HybridInteractionSet & interactions,
									  HybridRouteHeightSampleFunc sample,void * context,HybridRealizationPlan * out,
									  HybridRealizationResult * result)
{
	if (result != nullptr) {
		result->Failure	= HYBRID_REALIZATION_OK;
		result->Detail		= "";
		result->EdgeId		= 0;
		result->Profile	= HYBRID_PROFILE_OK;
	}
	if (out == nullptr) return false;
	out->Reset();
	const HybridConstants & k = params.Constants;
	if ((sample == nullptr) || !(k.W > 0.0f) || !(k.A > 0.0f)) {
		return Realization_Fail(result,HYBRID_REALIZATION_INVALID_INPUT,0,"Phase 12 needs the world constants and the terrain");
	}

	//	The physical roads, in routing order: a trunk once, by its first logical edge.
	DynamicVectorClass<unsigned int> names;
	DynamicVectorClass<int> ranks;
	for (int r = 0; r < routes.Routes.Count(); ++r) {
		const HybridRoute & route = routes.Routes[r];
		const HybridTopologyEdge * edge = topology.Find_Edge(route.EdgeId);
		if (edge == nullptr) continue;
		const HybridTrunkRoad * trunk = routes.Find_Trunk(route.EdgeId);
		const unsigned int name = ((trunk != nullptr) && (trunk->EdgeIds.Count() > 0)) ? trunk->EdgeIds[0] : route.EdgeId;
		if (Carries_Edge(names,name)) continue;
		const int rank = Hybrid_Route_Priority_Rank(Hybrid_Route_Priority_Of(topology,*edge));
		int slot = names.Count();
		names.Add(name);
		ranks.Add(rank);
		while ((slot > 0) && ((ranks[slot - 1] > rank) || ((ranks[slot - 1] == rank) && (names[slot - 1] > name)))) {
			names[slot]	= names[slot - 1];
			ranks[slot]	= ranks[slot - 1];
			slot--;
		}
		names[slot]	= name;
		ranks[slot]	= rank;
	}

	StringClass detail;
	for (int o = 0; o < names.Count(); ++o) {

		const HybridTopologyEdge * first_edge = topology.Find_Edge(names[o]);
		HybridRoute road;
		DynamicVectorClass<unsigned int> edges;
		DynamicVectorClass<HybridProfileStructure> structures;
		if ((first_edge == nullptr) || !Hybrid_Realized_Physical_Road(routes,interactions,names[o],&road,&edges) ||
			 !Hybrid_Road_Profile_Structures(interactions,road,&structures,&edges)) {
			detail.Format("edge %u cannot be realized from its route and compiled crossings",names[o]);
			return Realization_Fail(result,HYBRID_REALIZATION_ROAD,names[o],detail.Peek_Buffer());
		}
		const int real_structures = structures.Count();
		const float total = road.Stations[road.Stations.Count() - 1];

		//	Every junction surface this road shares (12.2 step 5, 12.6.10).
		for (int i = 0; i < interactions.Interactions.Count(); ++i) {
			const HybridInteraction & one = interactions.Interactions[i];
			const bool mine = Carries_Edge(edges,one.EdgeId);
			const bool theirs = Carries_Edge(edges,one.OtherEdgeId);
			//	A junction at a topology node holds every road through that node, not only the two its record names.
			const bool at_node = (one.Method == HYBRID_METHOD_AT_GRADE_JUNCTION) && (one.NodeId != 0) &&
										Road_Carries_Node(topology,routes,edges,one.NodeId);
			if (!mine && !theirs && !at_node) continue;
			const float station = Nearest_Station(road.Points,road.Stations,one.Station,nullptr);
			const HybridJunctionRole role = Hybrid_Interaction_Junction_Role(one);

			if (one.Method == HYBRID_METHOD_OVERPASS) {
				if (theirs) Add_Junction_Surface(&structures,real_structures,total,station,one.LowerSurfaceZ,k.Q);
				continue;
			}
			if (one.Method != HYBRID_METHOD_AT_GRADE_JUNCTION) continue;
			if (role == HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_LOCAL_JUNCTION) {
				Add_Junction_Surface(&structures,real_structures,total,station,one.JunctionZ,k.Q);
				continue;
			}
			if ((role == HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_MERGE) || (role == HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_DIVERGE)) {
				if (mine) Add_Junction_Surface(&structures,real_structures,total,station,one.JunctionZ,k.Q);
				continue;
			}
			//	Any other junction: the surface the road realized first through it already has there.
			const HybridRealizedRoad * earlier = nullptr;
			if (one.NodeId != 0) {
				for (int r = 0; (r < out->Roads.Count()) && (earlier == nullptr); ++r) {
					if (Road_Carries_Node(topology,routes,out->Roads[r].Edges,one.NodeId)) earlier = &out->Roads[r];
				}
			} else {
				earlier = out->Find_Road(mine ? one.OtherEdgeId : one.EdgeId);
			}
			if (earlier == nullptr) continue;
			Add_Junction_Surface(&structures,real_structures,total,station,Realized_Z_At(*earlier,one.Station),k.Q);
		}

		DynamicVectorClass<HybridProfileSample> samples;
		HybridSolvedProfile solved;
		HybridProfileFailure failure = HYBRID_PROFILE_OK;
		const float max_grade = Hybrid_Road_Class_Spec(first_edge->RoadClass).MaxGradeDegrees * PI_OVER_180;
		if (!Hybrid_Solve_Road_Stretch(road.Points,road.Stations,0.0f,total,structures,Hybrid_Profile_Sample_Spacing(k),
												 sample,context,params.MaxCutDepth,params.MaxFillHeight,max_grade,k.Q,&samples,&solved,
												 nullptr,nullptr,&failure)) {
			if (result != nullptr) result->Profile = failure;
			detail.Format("Section 12.1 refuses edge %u (%s) with its %d structures and %d junction surfaces: %s",names[o],
							  Hybrid_Road_Class_Name(first_edge->RoadClass),real_structures,structures.Count() - real_structures,
							  Hybrid_Profile_Failure_Name(failure));
			return Realization_Fail(result,HYBRID_REALIZATION_PROFILE,names[o],detail.Peek_Buffer());
		}

		HybridRealizedRoad realized;
		realized.StableId				= out->Roads.Count() + 1;
		realized.EdgeId				= names[o];
		realized.Edges					= edges;
		realized.RoadClass			= first_edge->RoadClass;
		realized.Tier					= Hybrid_Road_Class_Spec(first_edge->RoadClass).PhysicalTier;
		realized.Capabilities		= first_edge->Capabilities;
		realized.Group					= ranks[o];
		const HybridRoadClassSpec & spec = Hybrid_Road_Class_Spec(first_edge->RoadClass);
		realized.CarriagewayWidth	= ((float)spec.Lanes + spec.MedianW) * k.W;
		realized.ShoulderWidth		= (first_edge->Kind == HYBRID_EDGE_HIGHWAY_RAMP)
												? (0.5f * (Hybrid_Roadbed_Width(HYBRID_ROAD_HIGHWAY_RAMP,k.W) - k.W)) : (spec.ShoulderW * k.W);
		for (int e = 1; e < edges.Count(); ++e) {
			const HybridTopologyEdge * edge = topology.Find_Edge(edges[e]);
			if (edge != nullptr) realized.Capabilities |= edge->Capabilities;
		}

		for (int i = 0; i < samples.Count(); ++i) {
			HybridRealizedSample one;
			one.Position	= samples[i].Position;
			one.Station		= samples[i].Station;
			one.Z				= solved.Z[i];
			for (int s = 0; (s < real_structures) && !one.Structure; ++s) {
				one.Structure = (one.Station >= (structures[s].From - k.Q)) && (one.Station <= (structures[s].To + k.Q));
			}
			const int ahead = ((i + 1) < samples.Count()) ? (i + 1) : i;
			const int behind = ((i + 1) < samples.Count()) ? i : (i - 1);
			const Vector2 travel = Unit_Vector(samples[ahead].Position - samples[behind].Position);
			Sample_Edges(params,topology,interactions,realized,*first_edge,total,travel,&one);
			realized.Samples.Add(one);
		}
		out->Roads.Add(realized);
	}

	//	Every compiled bridge and overpass deck as its assembly: one WHOLE piece in this version.
	for (int i = 0; i < interactions.Interactions.Count(); ++i) {
		const HybridInteraction & one = interactions.Interactions[i];
		if ((one.Method != HYBRID_METHOD_BRIDGE) && (one.Method != HYBRID_METHOD_OVERPASS)) continue;
		const HybridTopologyEdge * edge = topology.Find_Edge(one.EdgeId);
		if (edge == nullptr) {
			detail.Format("compiled crossing %u carries edge %u, which is not in the topology",one.StableId,one.EdgeId);
			return Realization_Fail(result,HYBRID_REALIZATION_ROAD,one.EdgeId,detail.Peek_Buffer());
		}
		const Vector2 axis = Unit_Vector(one.Tangent);
		const float swept = Hybrid_Required_Swept_Width(edge->Capabilities,params.LightVehicleWidth,params.HeavyVehicleWidth,
																		params.HarvesterWidth);
		const float clear = Hybrid_Bridge_Clear_Width(edge->RoadClass,k.W,swept,params.MinVehicleLaneClearance);

		HybridBridgeAssembly assembly;
		assembly.StableId			= out->Bridges.Count() + 1;
		assembly.InteractionId	= one.StableId;
		assembly.Method			= one.Method;
		assembly.BridgeKind		= one.BridgeKind;
		assembly.EdgeId			= one.EdgeId;
		assembly.Start				= one.Station - (axis * one.DeckReachA);
		assembly.End				= one.Station + (axis * one.DeckReachB);
		assembly.StartZ			= one.DeckStartZ;
		assembly.EndZ				= one.DeckEndZ;
		assembly.Width				= (one.Method == HYBRID_METHOD_OVERPASS) ? Max2(Hybrid_Edge_Roadbed_Width(*edge,k.W),clear) : clear;

		HybridBridgePiece whole;
		whole.PieceId	= 1;
		whole.Kind		= HYBRID_BRIDGE_PIECE_WHOLE;
		whole.Start		= assembly.Start;
		whole.End		= assembly.End;
		whole.StartZ	= assembly.StartZ;
		whole.EndZ		= assembly.EndZ;
		whole.Width		= assembly.Width;
		assembly.Pieces.Add(whole);
		out->Bridges.Add(assembly);
	}

	out->Checksum = Hybrid_Realization_Checksum(*out);
	return true;
}


unsigned int Hybrid_Realization_Checksum(const HybridRealizationPlan & plan,unsigned int seed)
{
	HybridHashStream stream(seed);
	stream.Add_Int(plan.Roads.Count());
	for (int r = 0; r < plan.Roads.Count(); ++r) {
		const HybridRealizedRoad & road = plan.Roads[r];
		stream.Add_Uint(road.StableId);
		stream.Add_Uint(road.EdgeId);
		stream.Add_Int(road.Edges.Count());
		for (int e = 0; e < road.Edges.Count(); ++e) stream.Add_Uint(road.Edges[e]);
		stream.Add_Int((int)road.RoadClass);
		stream.Add_Int((int)road.Tier);
		stream.Add_Uint(road.Capabilities);
		stream.Add_Int(road.Group);
		stream.Add_Length(road.CarriagewayWidth);
		stream.Add_Length(road.ShoulderWidth);
		stream.Add_Int(road.Samples.Count());
		for (int s = 0; s < road.Samples.Count(); ++s) {
			const HybridRealizedSample & one = road.Samples[s];
			stream.Add_Length(one.Position.X);
			stream.Add_Length(one.Position.Y);
			stream.Add_Length(one.Station);
			stream.Add_Length(one.Z);
			stream.Add_Length(one.Left);
			stream.Add_Length(one.Right);
			stream.Add_Bool(one.Structure);
		}
	}
	stream.Add_Int(plan.Bridges.Count());
	for (int b = 0; b < plan.Bridges.Count(); ++b) {
		const HybridBridgeAssembly & assembly = plan.Bridges[b];
		stream.Add_Uint(assembly.StableId);
		stream.Add_Uint(assembly.InteractionId);
		stream.Add_Int((int)assembly.Method);
		stream.Add_Int(assembly.BridgeKind);
		stream.Add_Uint(assembly.EdgeId);
		stream.Add_Length(assembly.Width);
		stream.Add_Int(assembly.Pieces.Count());
		for (int p = 0; p < assembly.Pieces.Count(); ++p) {
			const HybridBridgePiece & piece = assembly.Pieces[p];
			stream.Add_Uint(piece.PieceId);
			stream.Add_Int((int)piece.Kind);
			stream.Add_Length(piece.Start.X);
			stream.Add_Length(piece.Start.Y);
			stream.Add_Length(piece.End.X);
			stream.Add_Length(piece.End.Y);
			stream.Add_Length(piece.StartZ);
			stream.Add_Length(piece.EndZ);
			stream.Add_Length(piece.Width);
		}
	}
	return stream.Get();
}


/***********************************************************************************************
**	Section 12.2 -- the terrain deformation.
***********************************************************************************************/

static inline int Max_Int(int a,int b)	{ return (a > b) ? a : b; }
static inline int Min_Int(int a,int b)	{ return (a < b) ? a : b; }

bool Hybrid_Deform_Terrain(const HybridRealizationPlan & plan,const HybridConstants & k,const HybridHydrologySet * hydrology,
									const HybridHeightGrid & grid,float * heights,HybridDeformationStats * stats)
{
	if ((heights == nullptr) || (grid.CountX <= 0) || (grid.CountY <= 0) || !(grid.Spacing > 0.0f)) return false;
	HybridDeformationStats local;
	HybridDeformationStats & counted = (stats != nullptr) ? *stats : local;
	counted = HybridDeformationStats();

	const float blend = Hybrid_Road_Outer_Blend_Width(k.W,k.A);
	const int total = grid.CountX * grid.CountY;

	//	The roadbed vertices of every road already deformed: theirs, never a later road's to change.
	DynamicVectorClass<unsigned char> owned;
	owned.Resize(total);
	for (int v = 0; v < total; ++v) owned.Add(0);

	//	The nearest-centerline record of one vertex, for the road being deformed.
	DynamicVectorClass<float> best;
	DynamicVectorClass<int> segment;
	DynamicVectorClass<float> along;
	DynamicVectorClass<float> lateral;
	DynamicVectorClass<int> mine;

	for (int r = 0; r < plan.Roads.Count(); ++r) {

		const HybridRealizedRoad & road = plan.Roads[r];
		const int count = road.Samples.Count();
		if (count < 2) continue;

		float reach = 0.0f;
		Vector2 lo = road.Samples[0].Position;
		Vector2 hi = lo;
		for (int i = 0; i < count; ++i) {
			const HybridRealizedSample & one = road.Samples[i];
			reach = Max2(reach,Max2(one.Left,one.Right) + blend);
			lo.Set(Min2(lo.X,one.Position.X),Min2(lo.Y,one.Position.Y));
			hi.Set(Max2(hi.X,one.Position.X),Max2(hi.Y,one.Position.Y));
		}
		const int x0 = Max_Int((int)::floor((double)((lo.X - reach - grid.OriginX) / grid.Spacing)),0);
		const int y0 = Max_Int((int)::floor((double)((lo.Y - reach - grid.OriginY) / grid.Spacing)),0);
		const int x1 = Min_Int((int)::ceil((double)((hi.X + reach - grid.OriginX) / grid.Spacing)),grid.CountX - 1);
		const int y1 = Min_Int((int)::ceil((double)((hi.Y + reach - grid.OriginY) / grid.Spacing)),grid.CountY - 1);
		if ((x1 < x0) || (y1 < y0)) continue;
		const int width = (x1 - x0) + 1;
		const int box = width * ((y1 - y0) + 1);
		best.Delete_All();
		segment.Delete_All();
		along.Delete_All();
		lateral.Delete_All();
		best.Resize(box);
		segment.Resize(box);
		along.Resize(box);
		lateral.Resize(box);
		for (int v = 0; v < box; ++v) {
			best.Add(3.4e38f);
			segment.Add(-1);
			along.Add(0.0f);
			lateral.Add(0.0f);
		}

		//	Every vertex near each centerline segment, keeping the nearest segment.
		for (int i = 1; i < count; ++i) {
			const Vector2 a = road.Samples[i - 1].Position;
			const Vector2 b = road.Samples[i].Position;
			const float near_reach = Max2(Max2(road.Samples[i - 1].Left,road.Samples[i - 1].Right),
													Max2(road.Samples[i].Left,road.Samples[i].Right)) + blend;
			const int sx0 = Max_Int((int)::floor((double)((Min2(a.X,b.X) - near_reach - grid.OriginX) / grid.Spacing)),x0);
			const int sy0 = Max_Int((int)::floor((double)((Min2(a.Y,b.Y) - near_reach - grid.OriginY) / grid.Spacing)),y0);
			const int sx1 = Min_Int((int)::ceil((double)((Max2(a.X,b.X) + near_reach - grid.OriginX) / grid.Spacing)),x1);
			const int sy1 = Min_Int((int)::ceil((double)((Max2(a.Y,b.Y) + near_reach - grid.OriginY) / grid.Spacing)),y1);
			const Vector2 d = b - a;
			const float length2 = (d.X * d.X) + (d.Y * d.Y);
			const float length = (float)sqrt((double)length2);
			if (!(length > 0.0f)) continue;
			for (int iy = sy0; iy <= sy1; ++iy) {
				for (int ix = sx0; ix <= sx1; ++ix) {
					const Vector2 p(grid.OriginX + (grid.Spacing * (float)ix),grid.OriginY + (grid.Spacing * (float)iy));
					const float t = Clamp3((((p.X - a.X) * d.X) + ((p.Y - a.Y) * d.Y)) / length2,0.0f,1.0f);
					const Vector2 foot = a + (d * t);
					const float distance = (p - foot).Length();
					const int v = ((iy - y0) * width) + (ix - x0);
					if (distance >= best[v]) continue;
					best[v]		= distance;
					segment[v]	= i;
					along[v]		= t;
					lateral[v]	= ((d.X * (p.Y - a.Y)) - (d.Y * (p.X - a.X))) / length;		// + left of travel
				}
			}
		}

		mine.Delete_All();
		for (int iy = y0; iy <= y1; ++iy) {
			for (int ix = x0; ix <= x1; ++ix) {
				const int v = ((iy - y0) * width) + (ix - x0);
				if (segment[v] < 0) continue;
				const HybridRealizedSample & a = road.Samples[segment[v] - 1];
				const HybridRealizedSample & b = road.Samples[segment[v]];
				const float t = along[v];
				const float side = lateral[v];
				const float distance = (float)fabs((double)side);
				const float edge = (side >= 0.0f) ? (a.Left + ((b.Left - a.Left) * t)) : (a.Right + ((b.Right - a.Right) * t));
				if (distance > (edge + blend)) continue;

				const int index = (iy * grid.CountX) + ix;
				if (((t <= 0.5f) ? a.Structure : b.Structure)) {
					counted.Structure++;
					continue;
				}
				if (owned[index] != 0) {
					counted.Protected++;
					continue;
				}
				const Vector2 p(grid.OriginX + (grid.Spacing * (float)ix),grid.OriginY + (grid.Spacing * (float)iy));
				if ((hydrology != nullptr) && Hybrid_Point_In_Water(*hydrology,p)) {
					counted.Water++;
					continue;
				}
				const float z = a.Z + ((b.Z - a.Z) * t);
				heights[index] = Hybrid_Road_Cross_Section_Height(distance,z,heights[index],edge,blend);
				counted.Changed++;
				if (distance <= edge) mine.Add(index);
			}
		}
		for (int m = 0; m < mine.Count(); ++m) owned[mine[m]] = 1;
	}
	return true;
}
