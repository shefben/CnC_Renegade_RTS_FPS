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
static inline int Max_Int(int a,int b)	{ return (a > b) ? a : b; }
static inline int Min_Int(int a,int b)	{ return (a < b) ? a : b; }
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
	**	The range's start, the exact end, and the fewest equal steps between them that keep every gap within
	**	`spacing`.  Each sample is placed on the segment that actually contains its station.  Equal steps, not
	**	whole multiples of `spacing` and then whatever is left: a structure end landing just past a multiple left
	**	a gap of a hundredth of a unit there, and quantizing the two heights of so short a step to Q reads as a
	**	grade no road has -- the revalidation then refused profiles that were feasible.
	*/
	const float end_station = to_station - from_station;
	if (end_station <= 0.0f) return false;

	HybridProfileSample first;
	first.Position = Hybrid_Polyline_Point_At(points,stations,from_station);
	first.Station = 0.0f;
	if (!sample(first.Position,&first.TerrainZ,context)) return false;
	out->Add(first);

	int segment = 0;
	const int steps = Max_Int((int)::ceil((double)((end_station - HYBRID_QUANTUM_POSITION) / spacing)),1);
	const float step = end_station / (float)steps;
	for (int n = 1; n < steps; ++n) {
		const float next = step * (float)n;

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


const char * Hybrid_Realized_Pin_Kind_Name(HybridRealizedPinKind kind)
{
	switch (kind) {
		case HYBRID_PIN_LOCAL_JUNCTION:		return "local_junction";
		case HYBRID_PIN_RAMP_GORE:				return "ramp_gore";
		case HYBRID_PIN_JUNCTION:				return "junction";
		case HYBRID_PIN_OVERPASS_CROSSING:	return "overpass_crossing";
		case HYBRID_PIN_DECK_END:				return "deck_end";
		default:										break;
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
	Pins.Delete_All();
	Constraints.Delete_All();
	Checksum = 0;
}


const HybridBridgeAssembly * HybridRealizationPlan::Find_Bridge(unsigned int stable_id) const
{
	for (int b = 0; b < Bridges.Count(); ++b) {
		if (Bridges[b].StableId == stable_id) return &Bridges[b];
	}
	return nullptr;
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
	MinVehicleLaneClearance(0.0f),TerrainCellSize(0.0f)
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
**	An interchange local junction's compiled elevation z_L (12.6.10 item 3): every road through L_s is held there.
*/
struct JunctionSurfaceView
{
	Vector2								Point;
	float									Z;
	unsigned int						InteractionId;
	DynamicVectorClass<int>			Roads;			// indices into the physical-road list
	bool	operator == (const JunctionSurfaceView & that) const	{ return Point == that.Point; }
	bool	operator != (const JunctionSurfaceView & that) const	{ return !(*this == that); }
};


//	Is a physical road carrying these logical edges a participant in this compiled junction record?
static bool Junction_Carries(const HybridTopology & topology,const HybridRouteSet & routes,const HybridInteraction & one,
									  const DynamicVectorClass<unsigned int> & edges)
{
	return Carries_Edge(edges,one.EdgeId) || ((one.OtherEdgeId != 0) && Carries_Edge(edges,one.OtherEdgeId)) ||
			 Road_Carries_Node(topology,routes,edges,one.NodeId);
}


//	Is `point` on a realized road's roadbed -- inside its actual paved edge on the side it lies, plus `margin`?
static bool On_Roadbed(const HybridRealizedRoad & road,const Vector2 & point,float margin)
{
	float best = 3.4e38f;
	float edge = 0.0f;
	for (int i = 1; i < road.Samples.Count(); ++i) {
		const HybridRealizedSample & a = road.Samples[i - 1];
		const HybridRealizedSample & b = road.Samples[i];
		const Vector2 d = b.Position - a.Position;
		const float length2 = (d.X * d.X) + (d.Y * d.Y);
		float t = (length2 > 0.0f) ? ((((point.X - a.Position.X) * d.X) + ((point.Y - a.Position.Y) * d.Y)) / length2) : 0.0f;
		t = Clamp3(t,0.0f,1.0f);
		const float distance = (point - (a.Position + (d * t))).Length();
		if (distance >= best) continue;
		best = distance;
		const bool left = (((d.X * (point.Y - a.Position.Y)) - (d.Y * (point.X - a.Position.X))) >= 0.0f);
		edge = left ? (a.Left + ((b.Left - a.Left) * t)) : (a.Right + ((b.Right - a.Right) * t));
	}
	return best <= (edge + margin);
}


/*
**	D-029: one road's fixed heights as they are compiled -- each a solver structure and its canonical record, in the
**	same order -- and the conflict that stopped them, if any.  The STRUCTURE records come first.
*/
struct ConstraintSet
{
	ConstraintSet(void) : RoadEdgeId(0),Q(0.0f),Real(0),Base(0),Conflict(false) { }

	DynamicVectorClass<HybridProfileStructure>		Structures;
	DynamicVectorClass<HybridVerticalConstraint>	Records;
	unsigned int												RoadEdgeId;
	float															Q;
	int															Real;
	int															Base;					// records compiled before this road's
	bool															Conflict;
	HybridConstraintConflict								Clash;
};


static HybridProfileStructure Structure_Of(const HybridVerticalConstraint & record)
{
	HybridProfileStructure structure;
	structure.From			= record.From;
	structure.To			= record.To;
	structure.Mode			= record.Level ? HYBRID_PROFILE_STRUCTURE_LEVEL : HYBRID_PROFILE_STRUCTURE_CARRIED;
	structure.EndsFixed	= record.EndsFixed;
	structure.StartZ		= record.StartZ;
	structure.EndZ			= record.EndZ;
	return structure;
}


static HybridVerticalConstraint Record_Of(const HybridProfileStructure & structure,unsigned int road,HybridVerticalConstraintKind kind,
														unsigned int interaction,unsigned int owner,float reach)
{
	HybridVerticalConstraint record;
	record.RoadEdgeId		= road;
	record.InteractionId	= interaction;
	record.OwnerEdgeId	= owner;
	record.Kind				= kind;
	record.From				= structure.From;
	record.To				= structure.To;
	record.Level			= (structure.Mode == HYBRID_PROFILE_STRUCTURE_LEVEL);
	record.EndsFixed		= structure.EndsFixed;
	record.StartZ			= structure.StartZ;
	record.EndZ				= structure.EndZ;
	record.Reach			= reach;
	return record;
}


//	A constraint's height at `station`, when it states one: a level hold, or a carried line between pinned ends.
static bool Constraint_Z_At(const HybridProfileStructure & s,float station,float * z_out)
{
	if (s.Mode == HYBRID_PROFILE_STRUCTURE_LEVEL) {
		*z_out = s.StartZ;
		return true;
	}
	if (!s.EndsFixed) return false;
	const float span = s.To - s.From;
	const float t = (span > 0.0f) ? Clamp3((station - s.From) / span,0.0f,1.0f) : 0.0f;
	*z_out = s.StartZ + ((s.EndZ - s.StartZ) * t);
	return true;
}


/*
**	Add one constraint -- its pieces together -- to a road.  Where a piece overlaps a constraint the road already holds,
**	the two are compared over the overlap: the same height within 2Q is the same surface held once already, and only
**	the rest of the piece is added; any other height cannot hold with it, and is a conflict naming both.  Nothing is
**	ever dropped silently.
*/
static bool Constraint_Add(ConstraintSet & set,const DynamicVectorClass<HybridProfileStructure> & pieces,HybridVerticalConstraintKind kind,
									unsigned int interaction,unsigned int owner,float reach)
{
	if (set.Conflict) return false;
	const float q = set.Q;
	for (int p = 0; p < pieces.Count(); ++p) {
		const HybridProfileStructure & piece = pieces[p];

		//	Every overlap with what is held, checked at both of its ends.
		DynamicVectorClass<float> cut_from;
		DynamicVectorClass<float> cut_to;
		const int held_count = set.Structures.Count();
		for (int e = 0; e < held_count; ++e) {
			const HybridProfileStructure & held = set.Structures[e];
			const float lo = Max2(piece.From,held.From);
			const float hi = Min2(piece.To,held.To);
			if (hi <= (lo + q)) continue;
			const float at[2] = { lo,hi };
			for (int k = 0; k < 2; ++k) {
				float z_new = 0.0f;
				float z_held = 0.0f;
				if (!Constraint_Z_At(piece,at[k],&z_new) || !Constraint_Z_At(held,at[k],&z_held)) continue;
				if ((float)fabs((double)(z_new - z_held)) <= (2.0f * q)) continue;
				set.Conflict				= true;
				set.Clash.Kind				= HYBRID_CONFLICT_OVERLAP;
				set.Clash.RoadEdgeId		= set.RoadEdgeId;
				set.Clash.HasA				= true;
				set.Clash.HasB				= true;
				set.Clash.A					= set.Records[e];
				set.Clash.B					= Record_Of(piece,set.RoadEdgeId,kind,interaction,owner,reach);
				set.Clash.B.StableId		= set.Base + set.Records.Count() + 1;
				set.Clash.StationA		= at[k];
				set.Clash.StationB		= at[k];
				set.Clash.ZA				= z_held;
				set.Clash.ZB				= z_new;
				set.Clash.Dz				= (float)fabs((double)(z_new - z_held));
				return false;
			}
			cut_from.Add(lo);
			cut_to.Add(hi);
		}

		//	The rest of the piece, outside every overlap: a sweep over the overlaps in station order.
		for (int i = 1; i < cut_from.Count(); ++i) {
			for (int j = i; (j > 0) && (cut_from[j] < cut_from[j - 1]); --j) {
				const float f = cut_from[j];
				const float g = cut_to[j];
				cut_from[j]		= cut_from[j - 1];
				cut_to[j]		= cut_to[j - 1];
				cut_from[j - 1]= f;
				cut_to[j - 1]	= g;
			}
		}
		float cursor = piece.From;
		for (int c = 0; c <= cut_from.Count(); ++c) {
			const float until = (c < cut_from.Count()) ? cut_from[c] : piece.To;
			if (until > (cursor + q)) {
				HybridProfileStructure rest = piece;
				rest.From	= cursor;
				rest.To		= until;
				if ((rest.Mode != HYBRID_PROFILE_STRUCTURE_LEVEL) && rest.EndsFixed) {
					Constraint_Z_At(piece,cursor,&rest.StartZ);
					Constraint_Z_At(piece,until,&rest.EndZ);
					rest.StartZ	= Hybrid_Quantize_Position(rest.StartZ);
					rest.EndZ	= Hybrid_Quantize_Position(rest.EndZ);
				}
				HybridVerticalConstraint record = Record_Of(rest,set.RoadEdgeId,kind,interaction,owner,reach);
				record.StableId = set.Base + set.Records.Count() + 1;
				set.Structures.Add(rest);
				set.Records.Add(record);
			}
			if (c < cut_from.Count()) cursor = Max2(cursor,cut_to[c]);
		}
	}
	return true;
}


/*
**	12.2 step 5 (D-028/D-029): a compiled road-road junction's shared patch, processed once under the earlier road.
**	Inside the junction's footprint on this road -- `[window_from, window_to]`, its arm reach either side of the junction
**	point `centre` -- the one stretch of this road's centerline over the earlier road's roadbed that passes through the
**	junction point is held at that road's own solved surface, sample by sample, the junction point itself among the
**	samples.  Other overlaps inside the window are incidental.  `margin` (one and a half terrain cells) carries the
**	stretch past the earlier roadbed's edge, so every grid vertex the centerline is interpolated from holds one surface.
*/
static void Insert_Cut(DynamicVectorClass<float> & cuts,float at,float q)
{
	for (int c = 0; c < cuts.Count(); ++c) {
		if ((float)fabs((double)(cuts[c] - at)) <= q) return;
	}
	int slot = cuts.Count();
	cuts.Add(at);
	while ((slot > 0) && (cuts[slot - 1] > at)) {
		cuts[slot] = cuts[slot - 1];
		slot--;
	}
	cuts[slot] = at;
}


static void Junction_Pieces(const HybridRealizedRoad & other,const HybridRoute & road,float window_from,float window_to,
									 float centre,float spacing,float q,float margin,const DynamicVectorClass<HybridProfileStructure> & held,
									 DynamicVectorClass<HybridProfileStructure> * pieces)
{
	pieces->Delete_All();
	const float total = road.Stations[road.Stations.Count() - 1];
	window_from	= Max2(window_from,0.0f);
	window_to	= Min2(window_to,total);
	if (((window_to - window_from) <= q) || (other.Samples.Count() < 2)) return;
	const int steps = Max_Int((int)::ceil((double)((window_to - window_from) / spacing)),1);

	//	The stretch over the earlier roadbed through the junction point.
	float from = -1.0f;
	float to = -1.0f;
	bool inside = false;
	float run_from = 0.0f;
	for (int i = 0; i <= steps; ++i) {
		const float at = window_from + ((window_to - window_from) * ((float)i / (float)steps));
		const bool here = On_Roadbed(other,Hybrid_Polyline_Point_At(road.Points,road.Stations,at),margin);
		if (here && !inside) run_from = at;
		if (here) {
			inside = true;
			if ((centre >= (run_from - spacing)) && (centre <= (at + spacing))) {
				from	= run_from;
				to		= at;
			}
		} else {
			inside = false;
		}
	}
	if (from < 0.0f) return;
	if ((to - from) <= (2.0f * q)) {
		const float z = Realized_Z_At(other,Hybrid_Polyline_Point_At(road.Points,road.Stations,0.5f * (from + to)));
		HybridProfileStructure level;
		level.From			= Max2((0.5f * (from + to)) - q,0.0f);
		level.To				= Min2((0.5f * (from + to)) + q,total);
		level.Mode			= HYBRID_PROFILE_STRUCTURE_LEVEL;
		level.EndsFixed	= true;
		level.StartZ		= Hybrid_Quantize_Position(z);
		level.EndZ			= level.StartZ;
		pieces->Add(level);
		return;
	}

	/*
	**	Sample by sample, the junction point among the cuts, so the line follows the earlier surface rather than a chord.
	**	D-029: every end of a height this road already holds inside the stretch is a cut too, so where the two meet they are
	**	compared at the earlier surface itself, never at a chord spanning the other's end.
	*/
	DynamicVectorClass<float> cuts;
	const int count = Max_Int((int)::ceil((double)((to - from) / spacing)),1);
	for (int s = 0; s <= count; ++s) cuts.Add((s == count) ? to : (from + ((to - from) * ((float)s / (float)count))));
	if ((centre > (from + q)) && (centre < (to - q))) Insert_Cut(cuts,centre,q);
	for (int h = 0; h < held.Count(); ++h) {
		const float ends[2] = { held[h].From,held[h].To };
		for (int e = 0; e < 2; ++e) {
			if ((ends[e] > (from + q)) && (ends[e] < (to - q))) Insert_Cut(cuts,ends[e],q);
		}
	}
	for (int s = 1; s < cuts.Count(); ++s) {
		if ((cuts[s] - cuts[s - 1]) <= q) continue;
		HybridProfileStructure shared;
		shared.From			= cuts[s - 1];
		shared.To			= cuts[s];
		shared.Mode			= HYBRID_PROFILE_STRUCTURE_CARRIED;
		shared.EndsFixed	= true;
		shared.StartZ		= Hybrid_Quantize_Position(Realized_Z_At(other,Hybrid_Polyline_Point_At(road.Points,road.Stations,cuts[s - 1])));
		shared.EndZ			= Hybrid_Quantize_Position(Realized_Z_At(other,Hybrid_Polyline_Point_At(road.Points,road.Stations,cuts[s])));
		pieces->Add(shared);
	}
}


/*
**	One junction surface held on a road: a level patch `2Q` long at `station`, the elevation the junction has there.
**	Section 12.1 holds every sample on it at `z` and propagates the class grade out of it.
*/
static HybridProfileStructure Level_Patch(float total,float station,float z,float q)
{
	float from = station - q;
	float to = station + q;
	if (from < 0.0f) {
		from	= 0.0f;
		to		= Min2(2.0f * q,total);
	}
	if (to > total) {
		to		= total;
		from	= Max2(total - (2.0f * q),0.0f);
	}
	HybridProfileStructure surface;
	surface.From		= from;
	surface.To			= to;
	surface.Mode		= HYBRID_PROFILE_STRUCTURE_LEVEL;
	surface.EndsFixed	= true;
	surface.StartZ		= Hybrid_Quantize_Position(z);
	surface.EndZ		= surface.StartZ;
	return surface;
}


/*
**	D-027: each compiled STRUCTURAL_PIN_LANDING on this road -- one whose pin its realized centerline passes through, or
**	whose junction it is attached to -- held level at the pin's elevation for exactly the landing's length away from the
**	pin on each ground side.  A deck or bore beside the pin keeps its own surface: a landing is never laid over a
**	structure, so at a deck end it is the ground road outside the deck.
**
**	D-029: a road attached to a compiled junction carries its record, and the junction's landing, wherever its centerline
**	runs: a trunk through a topology node need not pass through the node point itself, and the junction pin -- the earliest
**	road's own surface at that point -- stands on its roadbed beside it.  That road is held level about its station nearest
**	the pin, as every other road through the junction is.
*/
static void Add_Landings(const HybridTopology & topology,const HybridRouteSet & routes,const HybridInteractionSet & interactions,
								 const HybridRoute & road,const DynamicVectorClass<unsigned int> & edges,ConstraintSet & set)
{
	const float q = set.Q;
	const float total = road.Stations[road.Stations.Count() - 1];
	for (int l = 0; l < interactions.Landings.Count(); ++l) {
		const HybridPinLanding & landing = interactions.Landings[l];
		float distance = 0.0f;
		const float station = Nearest_Station(road.Points,road.Stations,landing.Position,&distance);
		const HybridInteraction * owner = interactions.Find(landing.InteractionId);
		const bool attached = (owner != nullptr) && (owner->Method == HYBRID_METHOD_AT_GRADE_JUNCTION) &&
									 Junction_Carries(topology,routes,*owner,edges);
		if (!attached && (distance > (2.0f * q))) continue;
		float from = Max2(station - landing.Length,0.0f);
		float to = Min2(station + landing.Length,total);
		for (int s = 0; s < set.Real; ++s) {
			const HybridProfileStructure & deck = set.Structures[s];
			if ((deck.To <= (from + q)) || (deck.From >= (to - q))) continue;
			if (deck.From >= (station - q)) to = Min2(to,deck.From);
			if (deck.To <= (station + q)) from = Max2(from,deck.To);
		}
		if ((to - from) <= q) continue;
		HybridProfileStructure level;
		level.From			= from;
		level.To				= to;
		level.Mode			= HYBRID_PROFILE_STRUCTURE_LEVEL;
		level.EndsFixed	= true;
		level.StartZ		= Hybrid_Quantize_Position(landing.Z);
		level.EndZ			= level.StartZ;
		DynamicVectorClass<HybridProfileStructure> one;
		one.Add(level);
		if (!Constraint_Add(set,one,HYBRID_VCON_LANDING,landing.InteractionId,0,0.0f)) return;
	}
}


//	The record whose height pins a sample at `station`, or -1.
static int Pinning_Record(const ConstraintSet & set,float station,float z)
{
	int found = -1;
	for (int r = 0; r < set.Structures.Count(); ++r) {
		const HybridProfileStructure & s = set.Structures[r];
		if ((station < (s.From - set.Q)) || (station > (s.To + set.Q))) continue;
		float at = 0.0f;
		if (!Constraint_Z_At(s,station,&at)) continue;
		if ((float)fabs((double)(at - z)) <= (2.0f * set.Q)) return r;
		if (found < 0) found = r;
	}
	return found;
}


/*
**	D-029: which fixed heights, or which ground, emptied the profile.  Section 12.1's forward propagation run again,
**	each bound remembering the sample it came from: the first sample left with no legal height has one bound from
**	itself and the other from a sample behind it, and those two samples are the conflict.
*/
static void Diagnose_Profile(const DynamicVectorClass<HybridProfileSample> & samples,const ConstraintSet & set,float max_cut,
									  float max_fill,float max_grade_radians,HybridProfileFailure failure,HybridConstraintConflict * out)
{
	*out = HybridConstraintConflict();
	out->RoadEdgeId		= set.RoadEdgeId;
	out->AllowedGrade		= (float)::tan((double)max_grade_radians);
	out->Kind				= HYBRID_CONFLICT_OTHER;
	const int count = samples.Count();
	if ((failure != HYBRID_PROFILE_FORWARD_EMPTY) || (count < 2)) return;
	const float g = out->AllowedGrade;
	const float pin_cell = 0.5f * HYBRID_QUANTUM_POSITION;
	DynamicVectorClass<float> own_lo;
	DynamicVectorClass<float> own_hi;
	for (int i = 0; i < count; ++i) {
		const HybridProfileSample & s = samples[i];
		if (s.Fixed) {
			own_lo.Add(s.FixedZ - pin_cell);
			own_hi.Add(s.FixedZ + pin_cell);
		} else if (s.Unbounded) {
			own_lo.Add(-1.0e9f);
			own_hi.Add(1.0e9f);
		} else {
			const HybridVerticalInterval interval = Hybrid_Road_Vertical_Interval(s.TerrainZ,max_cut,max_fill);
			own_lo.Add(interval.Low);
			own_hi.Add(interval.High);
		}
	}
	float lo = own_lo[0];
	float hi = own_hi[0];
	int lo_from = 0;
	int hi_from = 0;
	for (int i = 1; i < count; ++i) {
		const float ds = samples[i].Station - samples[i - 1].Station;
		if ((lo - (g * ds)) > own_lo[i]) {
			lo -= g * ds;
		} else {
			lo			= own_lo[i];
			lo_from	= i;
		}
		if ((hi + (g * ds)) < own_hi[i]) {
			hi += g * ds;
		} else {
			hi			= own_hi[i];
			hi_from	= i;
		}
		if (lo <= hi) continue;

		//	The earlier sample is side A, this one side B.  Side A's bound is what reaches here.
		const int a = (lo_from == i) ? hi_from : lo_from;
		const int b = i;
		const bool a_low = (lo_from != i);			// A sets the floor (a height the road cannot sink below), B the ceiling
		const HybridProfileSample & sa = samples[a];
		const HybridProfileSample & sb = samples[b];
		out->StationA		= sa.Station;
		out->StationB		= sb.Station;
		out->Separation	= sb.Station - sa.Station;
		out->ZA				= sa.Fixed ? sa.FixedZ : (a_low ? own_lo[a] : own_hi[a]);
		out->ZB				= sb.Fixed ? sb.FixedZ : (a_low ? own_hi[b] : own_lo[b]);
		out->Dz				= (float)fabs((double)(out->ZA - out->ZB));
		out->RequiredGrade= out->Dz / Max2(out->Separation,set.Q);
		const int ra = sa.Fixed ? Pinning_Record(set,sa.Station,sa.FixedZ) : -1;
		const int rb = sb.Fixed ? Pinning_Record(set,sb.Station,sb.FixedZ) : -1;
		out->HasA = (ra >= 0);
		out->HasB = (rb >= 0);
		if (out->HasA) out->A = set.Records[ra];
		if (out->HasB) out->B = set.Records[rb];
		if (out->HasA && out->HasB) {
			out->Kind = HYBRID_CONFLICT_GRADE;
		} else if (out->HasA || out->HasB) {
			out->Kind = HYBRID_CONFLICT_CUT_FILL;
			//	The ground side: where the fixed height's reach and the cut/fill interval part.
			const HybridProfileSample & ground = out->HasA ? sb : sa;
			const float pinned = out->HasA ? out->ZA : out->ZB;
			out->TerrainZ = ground.TerrainZ;
			if (pinned > ground.TerrainZ) {
				out->RequiredCutFill	= (pinned - (g * out->Separation)) - ground.TerrainZ;
				out->AllowedCutFill	= max_fill;
			} else {
				out->RequiredCutFill	= (pinned + (g * out->Separation)) - ground.TerrainZ;
				out->AllowedCutFill	= -max_cut;
			}
		} else {
			out->Kind		= HYBRID_CONFLICT_GROUND;
			out->TerrainZ	= sb.TerrainZ;
		}
		return;
	}
}


//	A conflict, read aloud: the road, both sides with their provenance, and the blocker.
static void Describe_Side(const HybridConstraintConflict & c,bool side_a,StringClass * out)
{
	const bool has = side_a ? c.HasA : c.HasB;
	const HybridVerticalConstraint & record = side_a ? c.A : c.B;
	const float station = side_a ? c.StationA : c.StationB;
	const float z = side_a ? c.ZA : c.ZB;
	if (!has) {
		out->Format("the road's own cut/fill bound %.3f at station %.2f",z,station);
		return;
	}
	if (record.OwnerEdgeId != 0) {
		out->Format("%s constraint %u (interaction %u, earlier road %u, stations %.2f..%.2f) at station %.2f z %.3f",
						Hybrid_Vertical_Constraint_Kind_Name(record.Kind),record.StableId,record.InteractionId,record.OwnerEdgeId,
						record.From,record.To,station,z);
	} else {
		out->Format("%s constraint %u (interaction %u, stations %.2f..%.2f) at station %.2f z %.3f",
						Hybrid_Vertical_Constraint_Kind_Name(record.Kind),record.StableId,record.InteractionId,record.From,record.To,
						station,z);
	}
}


static void Describe_Conflict(const HybridConstraintConflict & c,HybridRoadClass road_class,HybridProfileFailure failure,StringClass * out)
{
	StringClass a;
	StringClass b;
	Describe_Side(c,true,&a);
	Describe_Side(c,false,&b);
	switch (c.Kind) {
		case HYBRID_CONFLICT_OVERLAP:
			out->Format("road %u (%s): %s and %s hold one stretch %.3f apart",c.RoadEdgeId,Hybrid_Road_Class_Name(road_class),
							a.Peek_Buffer(),b.Peek_Buffer(),c.Dz);
			break;
		case HYBRID_CONFLICT_GRADE:
			out->Format("road %u (%s): %s and %s are %.2f apart and %.3f apart in height: grade %.4f needed, %.4f allowed",
							c.RoadEdgeId,Hybrid_Road_Class_Name(road_class),a.Peek_Buffer(),b.Peek_Buffer(),c.Separation,c.Dz,
							c.RequiredGrade,c.AllowedGrade);
			break;
		case HYBRID_CONFLICT_CUT_FILL:
			out->Format("road %u (%s): %s and %s are %.2f apart: over ground %.3f the road would need %s %.3f, %.3f allowed "
							"(grade %.4f needed, %.4f allowed)",c.RoadEdgeId,Hybrid_Road_Class_Name(road_class),a.Peek_Buffer(),
							b.Peek_Buffer(),c.Separation,c.TerrainZ,(c.RequiredCutFill >= 0.0f) ? "fill" : "cut",
							(float)fabs((double)c.RequiredCutFill),(float)fabs((double)c.AllowedCutFill),c.RequiredGrade,c.AllowedGrade);
			break;
		case HYBRID_CONFLICT_GROUND:
			out->Format("road %u (%s): no fixed height takes part: %s and %s are %.2f apart and %.3f apart in height (grade %.4f "
							"needed, %.4f allowed)",c.RoadEdgeId,Hybrid_Road_Class_Name(road_class),a.Peek_Buffer(),b.Peek_Buffer(),
							c.Separation,c.Dz,c.RequiredGrade,c.AllowedGrade);
			break;
		default:
			out->Format("road %u (%s): Section 12.1 refuses it: %s",c.RoadEdgeId,Hybrid_Road_Class_Name(road_class),
							Hybrid_Profile_Failure_Name(failure));
			break;
	}
}


//	The actual paved edges either side of one sample of a physical road (12.6.11, D-022, D-024).
static void Sample_Edges(const HybridRealizationParams & params,const HybridTopology & topology,
								 const HybridInteractionSet & interactions,const HybridRealizedRoad & road,
								 const HybridTopologyEdge & first_edge,float total,const Vector2 & travel,
								 HybridRealizedSample * sample)
{
	const HybridConstants & k = params.Constants;
	const HybridRoadClassSpec & spec = Hybrid_Road_Class_Spec(road.RoadClass);
	sample->Left				= -Hybrid_Road_Inner_Edge_Offset(road.RoadClass,k.W);
	sample->Right				= Hybrid_Road_Outer_Edge_Offset(road.RoadClass,k.W);
	sample->CarriageLeft		= 0.5f * ((float)spec.Lanes + spec.MedianW) * k.W;
	sample->CarriageRight	= sample->CarriageLeft;

	HybridInterchangeConstants c;
	Hybrid_Interchange_Constants(k.W,&c);

	//	A ramp, stored L_s -> gore: inner (highway side) is left of its traffic, which an OFF_RAMP runs against.
	if (first_edge.Kind == HYBRID_EDGE_HIGHWAY_RAMP) {
		float inner = 0.0f;
		float outer = 0.0f;
		Hybrid_Ramp_Effective_Edges(c,first_edge.RampRoadbedExtra,total - sample->Station,&inner,&outer);
		float inner_shoulder = 0.0f;
		float outer_shoulder = 0.0f;
		Hybrid_Ramp_Gore_Shoulders(c,total - sample->Station,&inner_shoulder,&outer_shoulder);
		const bool against = (first_edge.RampRole == HYBRID_RAMP_ROLE_OFF);
		sample->Left				= against ? outer : inner;
		sample->Right				= against ? inner : outer;
		//	The lane and D-024's widening are paved carriageway; the rest is shoulder.
		sample->CarriageLeft		= sample->Left - (against ? outer_shoulder : inner_shoulder);
		sample->CarriageRight	= sample->Right - (against ? inner_shoulder : outer_shoulder);
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
		float carriage = md.Samples[0].H;
		for (int s = 1; s < md.Samples.Count(); ++s) {
			const HybridMergeDivergeSample & a = md.Samples[s - 1];
			const HybridMergeDivergeSample & b = md.Samples[s];
			const float s_lo = Min2(a.Station,b.Station);
			const float s_hi = Max2(a.Station,b.Station);
			if ((chain < s_lo) || (chain > s_hi)) continue;
			const float span = b.Station - a.Station;
			const float t = (span != 0.0f) ? Clamp3((chain - a.Station) / span,0.0f,1.0f) : 0.0f;
			widened = a.E + ((b.E - a.E) * t);
			carriage = a.H + ((b.H - a.H) * t);
			break;
		}
		const Vector2 normal = (md.Side == HYBRID_INTERCHANGE_SIDE_RIGHT) ? Vector2(ic->Axis.Y,-ic->Axis.X)
																							 : Vector2(-ic->Axis.Y,ic->Axis.X);
		if (((left_of_travel.X * normal.X) + (left_of_travel.Y * normal.Y)) > 0.0f) {
			sample->Left			= Max2(sample->Left,widened);
			sample->CarriageLeft	= Max2(sample->CarriageLeft,carriage);
		} else {
			sample->Right			= Max2(sample->Right,widened);
			sample->CarriageRight= Max2(sample->CarriageRight,carriage);
		}
	}
}


/*
**	The plan, both ways (D-029).  `compile` true: Phase 11 -- every road's constraints derived from the compiled records
**	and the roads solved before it, conflicts diagnosed.  False: Phase 12 -- every road's constraints taken unchanged
**	from `interactions.Constraints`, nothing derived.  Each road is solved once with exactly its constraints.
*/
static bool Build_Plan(const HybridRealizationParams & params,const HybridTopology & topology,const HybridRouteSet & routes,
							  const HybridInteractionSet & interactions,HybridRouteHeightSampleFunc sample,void * context,bool compile,
							  DynamicVectorClass<HybridVerticalConstraint> * constraints_out,HybridRealizationPlan * out,
							  HybridRealizationResult * result)
{
	if (result != nullptr) {
		result->Failure	= HYBRID_REALIZATION_OK;
		result->Detail		= "";
		result->EdgeId		= 0;
		result->Profile	= HYBRID_PROFILE_OK;
		result->Conflict	= HybridConstraintConflict();
	}
	if (out == nullptr) return false;
	out->Reset();
	if (constraints_out != nullptr) constraints_out->Delete_All();
	const HybridConstants & k = params.Constants;
	if ((sample == nullptr) || !(k.W > 0.0f) || !(k.A > 0.0f)) {
		return Realization_Fail(result,HYBRID_REALIZATION_INVALID_INPUT,0,"Phase 12 needs the world constants and the terrain");
	}
	if (!compile && !interactions.ConstraintsCompiled) {
		return Realization_Fail(result,HYBRID_REALIZATION_INVALID_INPUT,0,
										"Phase 11 compiled no vertical constraints: Phase 12 derives none of its own (D-029)");
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

	//	Every physical road, realized horizontally, before any is graded.
	DynamicVectorClass<HybridRoute> physical;
	DynamicVectorClass< DynamicVectorClass<unsigned int> > carried;
	for (int o = 0; o < names.Count(); ++o) {
		HybridRoute road;
		DynamicVectorClass<unsigned int> edges;
		if ((topology.Find_Edge(names[o]) == nullptr) || !Hybrid_Realized_Physical_Road(routes,interactions,names[o],&road,&edges)) {
			detail.Format("edge %u cannot be realized from its route and compiled crossings",names[o]);
			return Realization_Fail(result,HYBRID_REALIZATION_ROAD,names[o],detail.Peek_Buffer());
		}
		physical.Add(road);
		carried.Add(edges);
	}

	//	Every interchange local junction's compiled z_L, and the roads through it.
	DynamicVectorClass<JunctionSurfaceView> surfaces;
	for (int i = 0; compile && (i < interactions.Interactions.Count()); ++i) {
		const HybridInteraction & one = interactions.Interactions[i];
		if ((one.Method != HYBRID_METHOD_AT_GRADE_JUNCTION) ||
			 (Hybrid_Interaction_Junction_Role(one) != HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_LOCAL_JUNCTION)) continue;
		JunctionSurfaceView surface;
		surface.Point				= one.Station;
		surface.Z					= one.JunctionZ;
		surface.InteractionId	= one.StableId;
		for (int r = 0; r < physical.Count(); ++r) {
			if (Road_Carries_Node(topology,routes,carried[r],one.NodeId)) surface.Roads.Add(r);
		}
		surfaces.Add(surface);
	}

	for (int o = 0; o < names.Count(); ++o) {

		const HybridTopologyEdge * first_edge = topology.Find_Edge(names[o]);
		const HybridRoute & road = physical[o];
		const DynamicVectorClass<unsigned int> & edges = carried[o];
		const float total = road.Stations[road.Stations.Count() - 1];
		ConstraintSet set;
		set.RoadEdgeId	= names[o];
		set.Q				= k.Q;
		set.Base			= out->Constraints.Count();

		if (compile) {
			//	Its own compiled decks and bores.
			DynamicVectorClass<HybridProfileStructure> structures;
			DynamicVectorClass<unsigned int> owners;
			if (!Hybrid_Road_Profile_Structures(interactions,road,&structures,&edges,&owners)) {
				detail.Format("edge %u's compiled structures cannot be placed on it",names[o]);
				return Realization_Fail(result,HYBRID_REALIZATION_ROAD,names[o],detail.Peek_Buffer());
			}
			for (int s = 0; s < structures.Count(); ++s) {
				HybridVerticalConstraint record = Record_Of(structures[s],names[o],HYBRID_VCON_STRUCTURE,owners[s],0,0.0f);
				record.StableId = set.Base + set.Records.Count() + 1;
				set.Structures.Add(structures[s]);
				set.Records.Add(record);
			}
			set.Real = set.Structures.Count();

			//	D-027's compiled structural landings.
			Add_Landings(topology,routes,interactions,road,edges,set);

			/*
			**	Every compiled at-grade or topology junction it shares with a road solved before it: that road's own solved
			**	surface over the junction's footprint on this road (D-028).  Only roads both in one compiled junction record
			**	share a surface; the earliest road through it owns it.
			*/
			const float arm = Hybrid_Junction_Arm_Reach(first_edge->RoadClass,k.W);
			for (int r = 0; (r < out->Roads.Count()) && !set.Conflict; ++r) {
				for (int i = 0; (i < interactions.Interactions.Count()) && !set.Conflict; ++i) {
					const HybridInteraction & one = interactions.Interactions[i];
					if (one.Method != HYBRID_METHOD_AT_GRADE_JUNCTION) continue;
					if (!Junction_Carries(topology,routes,one,edges) || !Junction_Carries(topology,routes,one,out->Roads[r].Edges)) continue;
					bool owned_earlier = false;
					for (int e = 0; (e < r) && !owned_earlier; ++e) owned_earlier = Junction_Carries(topology,routes,one,out->Roads[e].Edges);
					if (owned_earlier) continue;			// the earliest road through it owns its surface
					const float at = Nearest_Station(road.Points,road.Stations,one.Station,nullptr);
					DynamicVectorClass<HybridProfileStructure> pieces;
					Junction_Pieces(out->Roads[r],road,at - arm,at + arm,at,Hybrid_Profile_Sample_Spacing(k),k.Q,
										 1.5f * params.TerrainCellSize,set.Structures,&pieces);
					Constraint_Add(set,pieces,HYBRID_VCON_JUNCTION,one.StableId,out->Roads[r].EdgeId,arm);
				}
			}

			//	Each interchange local junction it passes through, at its compiled z_L.
			for (int s = 0; (s < surfaces.Count()) && !set.Conflict; ++s) {
				bool member = false;
				for (int m = 0; (m < surfaces[s].Roads.Count()) && !member; ++m) member = (surfaces[s].Roads[m] == o);
				if (!member) continue;
				DynamicVectorClass<HybridProfileStructure> patch;
				patch.Add(Level_Patch(total,Nearest_Station(road.Points,road.Stations,surfaces[s].Point,nullptr),surfaces[s].Z,k.Q));
				Constraint_Add(set,patch,HYBRID_VCON_LOCAL_JUNCTION,surfaces[s].InteractionId,0,0.0f);
			}

			//	An overpass's lower road at its compiled surface, and a ramp's gore at its highway's solved surface (12.6.10).
			for (int i = 0; (i < interactions.Interactions.Count()) && !set.Conflict; ++i) {
				const HybridInteraction & one = interactions.Interactions[i];
				const bool mine = Carries_Edge(edges,one.EdgeId);
				const bool theirs = Carries_Edge(edges,one.OtherEdgeId);
				if (!mine && !theirs) continue;
				const float station = Nearest_Station(road.Points,road.Stations,one.Station,nullptr);
				const HybridJunctionRole role = Hybrid_Interaction_Junction_Role(one);
				DynamicVectorClass<HybridProfileStructure> patch;
				if (one.Method == HYBRID_METHOD_OVERPASS) {
					if (!theirs) continue;
					patch.Add(Level_Patch(total,station,one.LowerSurfaceZ,k.Q));
					Constraint_Add(set,patch,HYBRID_VCON_OVERPASS_LOWER,one.StableId,0,0.0f);
					continue;
				}
				if ((one.Method != HYBRID_METHOD_AT_GRADE_JUNCTION) || !mine) continue;
				if ((role != HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_MERGE) && (role != HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_DIVERGE)) continue;
				const HybridRealizedRoad * highway = out->Find_Road(one.OtherEdgeId);
				const float gore_z = (highway != nullptr) ? Realized_Z_At(*highway,one.Station) : one.JunctionZ;
				patch.Add(Level_Patch(total,station,gore_z,k.Q));
				Constraint_Add(set,patch,HYBRID_VCON_RAMP_GORE,one.StableId,(highway != nullptr) ? highway->EdgeId : 0,0.0f);
			}
		} else {
			//	Phase 12: exactly the constraints Phase 11 compiled for this road, in their order.
			for (int c = 0; c < interactions.Constraints.Count(); ++c) {
				const HybridVerticalConstraint & record = interactions.Constraints[c];
				if (record.RoadEdgeId != names[o]) continue;
				set.Structures.Add(Structure_Of(record));
				set.Records.Add(record);
				if (record.Kind == HYBRID_VCON_STRUCTURE) set.Real = set.Structures.Count();
			}
		}

		const float max_grade = Hybrid_Road_Class_Spec(first_edge->RoadClass).MaxGradeDegrees * PI_OVER_180;
		if (set.Conflict) {
			if (result != nullptr) result->Conflict = set.Clash;
			Describe_Conflict(set.Clash,first_edge->RoadClass,HYBRID_PROFILE_OK,&detail);
			return Realization_Fail(result,HYBRID_REALIZATION_PROFILE,names[o],detail.Peek_Buffer());
		}

		DynamicVectorClass<HybridProfileSample> samples;
		HybridSolvedProfile solved;
		HybridProfileFailure failure = HYBRID_PROFILE_OK;
		if (!Hybrid_Solve_Road_Stretch(road.Points,road.Stations,0.0f,total,set.Structures,Hybrid_Profile_Sample_Spacing(k),
												 sample,context,params.MaxCutDepth,params.MaxFillHeight,max_grade,k.Q,&samples,&solved,
												 nullptr,nullptr,&failure)) {
			if (result != nullptr) result->Profile = failure;
			HybridConstraintConflict conflict;
			Diagnose_Profile(samples,set,params.MaxCutDepth,params.MaxFillHeight,max_grade,failure,&conflict);
			if (result != nullptr) result->Conflict = conflict;
			Describe_Conflict(conflict,first_edge->RoadClass,failure,&detail);
			return Realization_Fail(result,HYBRID_REALIZATION_PROFILE,names[o],detail.Peek_Buffer());
		}
		for (int r = 0; r < set.Records.Count(); ++r) {
			out->Constraints.Add(set.Records[r]);
			if (constraints_out != nullptr) constraints_out->Add(set.Records[r]);
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
			//	Strictly inside a deck or bore: its two ends are the abutments and portals, which stand on the road's own ground.
			for (int s = 0; (s < set.Structures.Count()) && !one.Structure; ++s) {
				if (set.Records[s].Kind != HYBRID_VCON_STRUCTURE) continue;
				one.Structure = (one.Station > (set.Structures[s].From + k.Q)) && (one.Station < (set.Structures[s].To - k.Q));
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

	/*
	**	D-026: every frozen structural point, in record order.  Each at-grade junction: an interchange local junction at
	**	its compiled z_L, a ramp's gore at the realized highway's surface, any other at the surface of the earliest road
	**	through it (12.2 step 5).  Each overpass crossing at its lower road's compiled surface.  Then each deck end.  A
	**	point already pinned keeps its first pin.
	*/
	for (int i = 0; i < interactions.Interactions.Count(); ++i) {
		const HybridInteraction & one = interactions.Interactions[i];
		HybridRealizedPin pin;
		pin.Position		= one.Station;
		pin.InteractionId	= one.StableId;
		if (one.Method == HYBRID_METHOD_OVERPASS) {
			const HybridRealizedRoad * lower = out->Find_Road(one.OtherEdgeId);
			if (lower == nullptr) continue;
			pin.Kind			= HYBRID_PIN_OVERPASS_CROSSING;
			pin.Z				= one.LowerSurfaceZ;
			pin.RoadIndex	= (int)(lower - &out->Roads[0]);
		} else if (one.Method == HYBRID_METHOD_AT_GRADE_JUNCTION) {
			int earliest = -1;
			for (int r = 0; (r < out->Roads.Count()) && (earliest < 0); ++r) {
				const HybridRealizedRoad & road = out->Roads[r];
				if (Carries_Edge(road.Edges,one.EdgeId) || ((one.OtherEdgeId != 0) && Carries_Edge(road.Edges,one.OtherEdgeId)) ||
					 Road_Carries_Node(topology,routes,road.Edges,one.NodeId)) {
					earliest = r;
				}
			}
			if (earliest < 0) continue;
			const HybridJunctionRole role = Hybrid_Interaction_Junction_Role(one);
			pin.RoadIndex = earliest;
			if (role == HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_LOCAL_JUNCTION) {
				pin.Kind	= HYBRID_PIN_LOCAL_JUNCTION;
				pin.Z		= one.JunctionZ;
			} else if ((role == HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_MERGE) || (role == HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_DIVERGE)) {
				const HybridRealizedRoad * highway = out->Find_Road(one.OtherEdgeId);
				if (highway == nullptr) continue;
				pin.Kind			= HYBRID_PIN_RAMP_GORE;
				pin.RoadIndex	= (int)(highway - &out->Roads[0]);
				pin.Z				= Realized_Z_At(*highway,one.Station);
			} else {
				pin.Kind	= HYBRID_PIN_JUNCTION;
				pin.Z		= Realized_Z_At(out->Roads[earliest],one.Station);
			}
		} else {
			continue;
		}
		pin.Z = Hybrid_Quantize_Position(pin.Z);
		bool known = false;
		for (int p = 0; (p < out->Pins.Count()) && !known; ++p) known = ((out->Pins[p].Position - pin.Position).Length() <= k.Q);
		if (!known) out->Pins.Add(pin);
	}
	for (int b = 0; b < out->Bridges.Count(); ++b) {
		const HybridBridgeAssembly & assembly = out->Bridges[b];
		const HybridRealizedRoad * carried = out->Find_Road(assembly.EdgeId);
		for (int end = 0; end < 2; ++end) {
			HybridRealizedPin pin;
			pin.Kind			= HYBRID_PIN_DECK_END;
			pin.Position	= (end == 0) ? assembly.Start : assembly.End;
			pin.Z				= (end == 0) ? assembly.StartZ : assembly.EndZ;
			pin.RoadIndex	= (carried != nullptr) ? (int)(carried - &out->Roads[0]) : -1;
			pin.AssemblyId	= assembly.StableId;
			out->Pins.Add(pin);
		}
	}

	out->Checksum = Hybrid_Realization_Checksum(*out);
	return true;
}


bool Hybrid_Compile_Vertical_Constraints(const HybridRealizationParams & params,const HybridTopology & topology,
													  const HybridRouteSet & routes,const HybridInteractionSet & interactions,
													  HybridRouteHeightSampleFunc sample,void * context,
													  DynamicVectorClass<HybridVerticalConstraint> * constraints_out,
													  HybridRealizationPlan * plan_out,HybridRealizationResult * result)
{
	HybridRealizationPlan local;
	return Build_Plan(params,topology,routes,interactions,sample,context,true,constraints_out,
							(plan_out != nullptr) ? plan_out : &local,result);
}


bool Hybrid_Plan_Realization(const HybridRealizationParams & params,const HybridTopology & topology,
									  const HybridRouteSet & routes,const HybridInteractionSet & interactions,
									  HybridRouteHeightSampleFunc sample,void * context,HybridRealizationPlan * out,
									  HybridRealizationResult * result)
{
	return Build_Plan(params,topology,routes,interactions,sample,context,false,nullptr,out,result);
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
			stream.Add_Length(one.CarriageLeft);
			stream.Add_Length(one.CarriageRight);
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
	stream.Add_Int(plan.Pins.Count());
	for (int p = 0; p < plan.Pins.Count(); ++p) {
		const HybridRealizedPin & pin = plan.Pins[p];
		stream.Add_Int((int)pin.Kind);
		stream.Add_Length(pin.Position.X);
		stream.Add_Length(pin.Position.Y);
		stream.Add_Length(pin.Z);
		stream.Add_Int(pin.RoadIndex);
		stream.Add_Uint(pin.AssemblyId);
		stream.Add_Uint(pin.InteractionId);
	}
	return stream.Get();
}


/***********************************************************************************************
**	Section 12.2 -- the terrain deformation.
***********************************************************************************************/


//	HeightfieldClass's own far-edge rule (WWMATH_EPSILON): a point on the field's last vertex line belongs to the last cell.
static const float GRID_EDGE_EPSILON = 0.0001f;


void Hybrid_Bridge_End_Support_Extents(const HybridRealizationPlan & plan,const HybridBridgeAssembly & assembly,int end,
												  float * left_out,float * right_out)
{
	const float half = 0.5f * assembly.Width;
	*left_out	= half;
	*right_out	= half;
	const HybridRealizedRoad * road = plan.Find_Road(assembly.EdgeId);
	if ((road == nullptr) || (road->Samples.Count() < 2)) return;
	const Vector2 from = (end == 0) ? assembly.Start : assembly.End;
	const Vector2 to = (end == 0) ? assembly.End : assembly.Start;
	const Vector2 axis = Unit_Vector(to - from);
	int nearest = 0;
	float best = 3.4e38f;
	for (int s = 0; s < road->Samples.Count(); ++s) {
		const float distance = (road->Samples[s].Position - from).Length();
		if (distance >= best) continue;
		best		= distance;
		nearest	= s;
	}
	//	The road's paved edges there, left and right of its travel, turned into the frame looking into the deck.
	const int ahead = ((nearest + 1) < road->Samples.Count()) ? (nearest + 1) : nearest;
	const int behind = ((nearest + 1) < road->Samples.Count()) ? nearest : (nearest - 1);
	const Vector2 travel = road->Samples[ahead].Position - road->Samples[behind].Position;
	const bool same = (((travel.X * axis.X) + (travel.Y * axis.Y)) >= 0.0f);
	const HybridRealizedSample & at = road->Samples[nearest];
	*left_out	= Max2(half,same ? at.Left : at.Right);
	*right_out	= Max2(half,same ? at.Right : at.Left);
}


bool Hybrid_Bridge_End_Support_Contains(const HybridBridgeAssembly & assembly,int end,float left,float right,float cell,
													 const Vector2 & vertex)
{
	const Vector2 from = (end == 0) ? assembly.Start : assembly.End;
	const Vector2 to = (end == 0) ? assembly.End : assembly.Start;
	const Vector2 axis = Unit_Vector(to - from);
	const Vector2 across(-axis.Y,axis.X);
	const float reach = Min2(1.5f * cell,(to - from).Length());
	const Vector2 offset = vertex - from;
	const float along = (offset.X * axis.X) + (offset.Y * axis.Y);
	if ((along < -GRID_EDGE_EPSILON) || (along > (reach + GRID_EDGE_EPSILON))) return false;

	//	One of the vertex's four cells -- together the axis-aligned square of half side `cell` round it -- overlaps the
	//	footprint rectangle: separating axes x, y, along and across.
	const float half_along = 0.5f * reach;
	const float half_across = 0.5f * (left + right);
	const Vector2 d = vertex - (from + (axis * half_along) + (across * (0.5f * (left - right))));
	const float ax = (float)fabs((double)axis.X);
	const float ay = (float)fabs((double)axis.Y);
	if ((float)fabs((double)d.X) > (cell + (half_along * ax) + (half_across * ay) + GRID_EDGE_EPSILON)) return false;
	if ((float)fabs((double)d.Y) > (cell + (half_along * ay) + (half_across * ax) + GRID_EDGE_EPSILON)) return false;
	if ((float)fabs((double)((d.X * axis.X) + (d.Y * axis.Y))) > (half_along + (cell * (ax + ay)) + GRID_EDGE_EPSILON)) return false;
	if ((float)fabs((double)((d.X * across.X) + (d.Y * across.Y))) > (half_across + (cell * (ax + ay)) + GRID_EDGE_EPSILON)) return false;
	return true;
}


static bool In_Support(const HybridRealizationPlan & plan,const HybridBridgeAssembly & assembly,int end,float cell,const Vector2 & p)
{
	float left = 0.0f;
	float right = 0.0f;
	Hybrid_Bridge_End_Support_Extents(plan,assembly,end,&left,&right);
	return Hybrid_Bridge_End_Support_Contains(assembly,end,left,right,cell,p);
}


bool Hybrid_Grid_Height_At(const HybridHeightGrid & grid,const float * heights,const Vector2 & point,float * height_out,
									int * vertices_out,float * weights_out)
{
	if ((heights == nullptr) || (grid.CountX < 2) || (grid.CountY < 2) || !(grid.Spacing > 0.0f)) return false;
	const float gx = (point.X - grid.OriginX) / grid.Spacing;
	const float gy = (point.Y - grid.OriginY) / grid.Spacing;
	int cx = (int)::floor((double)gx);
	int cy = (int)::floor((double)gy);
	if ((cx == (grid.CountX - 1)) && ((gx - (float)cx) < GRID_EDGE_EPSILON)) cx--;
	if ((cy == (grid.CountY - 1)) && ((gy - (float)cy) < GRID_EDGE_EPSILON)) cy--;
	if ((cx < 0) || (cy < 0) || (cx >= (grid.CountX - 1)) || (cy >= (grid.CountY - 1))) return false;

	const float u = Clamp3((point.X - (grid.OriginX + ((float)cx * grid.Spacing))) / grid.Spacing,0.0f,1.0f);
	const float v = Clamp3((point.Y - (grid.OriginY + ((float)cy * grid.Spacing))) / grid.Spacing,0.0f,1.0f);
	const int low = (cy * grid.CountX) + cx;
	int index[3];
	float weight[3];
	if (u >= v) {
		//	Below the diagonal: the low corner, the next along x, the high corner.
		index[0] = low;		weight[0] = 1.0f - u;
		index[1] = low + 1;	weight[1] = u - v;
		index[2] = low + grid.CountX + 1;	weight[2] = v;
	} else {
		//	Above it: the low corner, the high corner, the next along y.
		index[0] = low;		weight[0] = 1.0f - v;
		index[1] = low + grid.CountX + 1;	weight[1] = u;
		index[2] = low + grid.CountX;	weight[2] = v - u;
	}
	if (height_out != nullptr) {
		*height_out = (heights[index[0]] * weight[0]) + (heights[index[1]] * weight[1]) + (heights[index[2]] * weight[2]);
	}
	for (int i = 0; i < 3; ++i) {
		if (vertices_out != nullptr) vertices_out[i] = index[i];
		if (weights_out != nullptr) weights_out[i] = weight[i];
	}
	return true;
}


//	The plan assembly whose deck runs between these two road samples, and which of its ends is `this_end`; -1 when none.
static int Deck_Assembly_Of(const HybridRealizationPlan & plan,const Vector2 & this_end,const Vector2 & other_end,float tolerance,
									 int * end_out)
{
	for (int b = 0; b < plan.Bridges.Count(); ++b) {
		const HybridBridgeAssembly & assembly = plan.Bridges[b];
		if (((assembly.Start - this_end).Length() <= tolerance) && ((assembly.End - other_end).Length() <= tolerance)) {
			*end_out = 0;
			return b;
		}
		if (((assembly.End - this_end).Length() <= tolerance) && ((assembly.Start - other_end).Length() <= tolerance)) {
			*end_out = 1;
			return b;
		}
	}
	return -1;
}


//	Is this vertex under a bridge's unsupported span -- past both end support footprints, inside its deck width?
static bool Under_Bridge_Span(const HybridRealizationPlan & plan,float cell,const Vector2 & vertex)
{
	for (int b = 0; b < plan.Bridges.Count(); ++b) {
		const HybridBridgeAssembly & assembly = plan.Bridges[b];
		if (assembly.Method != HYBRID_METHOD_BRIDGE) continue;
		const Vector2 axis = Unit_Vector(assembly.End - assembly.Start);
		const Vector2 offset = vertex - assembly.Start;
		const float along = (offset.X * axis.X) + (offset.Y * axis.Y);
		const float across = (float)fabs((double)((offset.X * -axis.Y) + (offset.Y * axis.X)));
		const float span = (assembly.End - assembly.Start).Length();
		if ((along > (1.5f * cell)) && (along < (span - (1.5f * cell))) && (across <= (0.5f * assembly.Width))) return true;
	}
	return false;
}


static void Claim_Support(DynamicVectorClass<HybridTerrainClaim> * claims,int vertex,unsigned int assembly_id)
{
	if (claims == nullptr) return;
	for (int c = 0; c < claims->Count(); ++c) {
		if ((*claims)[c].Vertex == vertex) return;
	}
	HybridTerrainClaim claim;
	claim.Kind			= HYBRID_TERRAIN_BRIDGE_END_SUPPORT;
	claim.Vertex		= vertex;
	claim.AssemblyId	= assembly_id;
	claims->Add(claim);
}


//	Steps 1-5 over the grid, road by road in plan order.
static void Deform_Roads(const HybridRealizationPlan & plan,const HybridConstants & k,const HybridHydrologySet * hydrology,
								 const HybridHeightGrid & grid,float * heights,HybridDeformationStats & counted,
								 DynamicVectorClass<HybridTerrainClaim> * claims)
{
	const float deck_tolerance = 0.25f * k.W;

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
					//	The true distance to the nearest point of the road, signed + left of travel: beyond a road's end the
					//	roadbed is a round cap, never a band along the end segment's line.
					lateral[v]	= (((d.X * (p.Y - a.Y)) - (d.Y * (p.X - a.X))) >= 0.0f) ? distance : -distance;
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

				/*
				**	1-2 on the grid: the ground is bilinear between vertices, so the surface carries the roadbed out to its
				**	paved edges only when every vertex of every cell the paved roadbed reaches holds the road.  The four
				**	cells round a vertex reach the roadbed when the vertex is within the paved edge plus that square's
				**	extent towards the road, `cell * (|n.x| + |n.y|)` along the unit `n` from the road to the vertex.
				*/
				const Vector2 p(grid.OriginX + (grid.Spacing * (float)ix),grid.OriginY + (grid.Spacing * (float)iy));
				const Vector2 foot_point = a.Position + ((b.Position - a.Position) * t);
				const Vector2 outward = (distance > 0.0f) ? ((p - foot_point) * (1.0f / distance)) : Vector2(0.0f,0.0f);
				const float cell_reach = grid.Spacing * ((float)fabs((double)outward.X) + (float)fabs((double)outward.Y));
				const bool roadbed = (distance <= (edge + cell_reach));

				/*
				**	4: under a deck or in a bore, nothing -- but its two abutment seats, one and a half grid cells inside each
				**	end, are graded with the road, so the ground the road's last point stands on is the deck's own end and
				**	not a sample of the bank under it.  A segment touching a structure sample lies wholly inside the
				**	structure: its samples are strictly interior and its ends are the road's own.
				*/
				const int index = (iy * grid.CountX) + ix;
				int seat_assembly = -1;
				int seat_end = 0;
				if (a.Structure || b.Structure) {
					const float foot = a.Station + ((b.Station - a.Station) * t);
					int lo = segment[v] - 1;
					int hi = segment[v];
					while ((lo > 0) && road.Samples[lo].Structure) lo--;
					while ((hi < (count - 1)) && road.Samples[hi].Structure) hi++;
					const float seat = 1.5f * grid.Spacing;
					if ((foot > (road.Samples[lo].Station + seat)) && (foot < (road.Samples[hi].Station - seat))) {
						counted.Structure++;
						continue;
					}
					const bool near_lo = ((foot - road.Samples[lo].Station) <= (road.Samples[hi].Station - foot));
					seat_assembly = Deck_Assembly_Of(plan,road.Samples[near_lo ? lo : hi].Position,road.Samples[near_lo ? hi : lo].Position,
																deck_tolerance,&seat_end);
				}
				if (owned[index] != 0) {
					counted.Protected++;
					continue;
				}
				/*
				**	3: no water -- except a deck end's own support footprint (D-026).  A bank that meets the deck end inside one
				**	grid cell would otherwise leave the road's last stretch interpolated down to the river bed.  The terrain cells
				**	under the end support, 1.5 cells along the deck and across its width, are structural support graded with the
				**	road and recorded as the assembly's; the span, the channel beside the deck and the approach are untouched.
				*/
				if ((hydrology != nullptr) && Hybrid_Point_In_Water(*hydrology,p)) {
					if (!roadbed || (seat_assembly < 0) ||
						 !In_Support(plan,plan.Bridges[seat_assembly],seat_end,grid.Spacing,p)) {
						counted.Water++;
						continue;
					}
					Claim_Support(claims,index,plan.Bridges[seat_assembly].StableId);
				}
				const float z = a.Z + ((b.Z - a.Z) * t);
				heights[index] = roadbed ? z : Hybrid_Road_Cross_Section_Height(distance,z,heights[index],edge,blend);
				counted.Changed++;
				if (roadbed) mine.Add(index);
			}
		}
		for (int m = 0; m < mine.Count(); ++m) owned[mine[m]] = 1;
	}
}


/*
**	The structural pins on the grid.  `local` (a patch round one pin, D-027's Phase-11 judgement) skips the pins whose
**	triangle is off the patch; on the whole field a pin off it is a failure.
*/
static void Realize_Pins(const HybridRealizationPlan & plan,const HybridConstants & k,const HybridHydrologySet * hydrology,
								 const HybridHeightGrid & grid,float * heights,HybridDeformationStats & counted,
								 DynamicVectorClass<HybridTerrainClaim> * claims,bool local)
{
	/*
	**	D-026: every structural pin realized exactly.  The surface under a pin is one triangle of the grid; the least change
	**	to its vertices that puts the surface at the pin's height is a step along their weights.  Pins sharing vertices are
	**	met together by repeating those steps (Kaczmarz), which converges to the least total change.  Water is never moved
	**	but a deck end's own support footprint, and nothing under a bridge's unsupported span.
	*/
	const int pin_count = plan.Pins.Count();
	DynamicVectorClass<int> pin_vertex;
	DynamicVectorClass<float> pin_weight;
	DynamicVectorClass<unsigned char> pin_moves;
	pin_vertex.Resize(3 * pin_count);
	pin_weight.Resize(3 * pin_count);
	pin_moves.Resize(3 * pin_count);
	for (int n = 0; n < pin_count; ++n) {
		const HybridRealizedPin & pin = plan.Pins[n];
		int vertex[3] = { -1,-1,-1 };
		float weight[3] = { 0.0f,0.0f,0.0f };
		unsigned char moves[3] = { 0,0,0 };
		float ignored = 0.0f;
		if (Hybrid_Grid_Height_At(grid,heights,pin.Position,&ignored,vertex,weight)) {
			const HybridBridgeAssembly * assembly = (pin.Kind == HYBRID_PIN_DECK_END) ? plan.Find_Bridge(pin.AssemblyId) : nullptr;
			const int end = ((assembly != nullptr) && ((assembly->End - pin.Position).Length() < (assembly->Start - pin.Position).Length())) ? 1 : 0;
			for (int i = 0; i < 3; ++i) {
				const Vector2 p(grid.OriginX + (grid.Spacing * (float)(vertex[i] % grid.CountX)),
									 grid.OriginY + (grid.Spacing * (float)(vertex[i] / grid.CountX)));
				bool movable = !Under_Bridge_Span(plan,grid.Spacing,p);
				if (movable && (hydrology != nullptr) && Hybrid_Point_In_Water(*hydrology,p)) {
					movable = (assembly != nullptr) && In_Support(plan,*assembly,end,grid.Spacing,p);
					if (movable && (weight[i] > 0.0f)) Claim_Support(claims,vertex[i],assembly->StableId);
				}
				moves[i] = movable ? 1 : 0;
			}
		}
		for (int i = 0; i < 3; ++i) {
			pin_vertex.Add(vertex[i]);
			pin_weight.Add(weight[i]);
			pin_moves.Add(moves[i]);
		}
	}
	const float target = 0.25f * k.Q;
	float worst = 0.0f;
	int pass = 0;
	for (; pass < 64; ++pass) {
		worst = 0.0f;
		for (int n = 0; n < pin_count; ++n) {
			if (pin_vertex[3 * n] < 0) {
				if (!local) worst = 3.4e38f;
				continue;
			}
			float surface = 0.0f;
			float moving = 0.0f;
			for (int i = 0; i < 3; ++i) {
				const float w = pin_weight[(3 * n) + i];
				surface += w * heights[pin_vertex[(3 * n) + i]];
				if (pin_moves[(3 * n) + i] != 0) moving += w * w;
			}
			const float residual = plan.Pins[n].Z - surface;
			worst = Max2(worst,(float)fabs((double)residual));
			if (!(moving > 0.0f)) continue;
			for (int i = 0; i < 3; ++i) {
				if (pin_moves[(3 * n) + i] != 0) heights[pin_vertex[(3 * n) + i]] += pin_weight[(3 * n) + i] * (residual / moving);
			}
		}
		if (worst <= target) break;
	}
	counted.Pins			= pin_count;
	counted.PinPasses		= pass + 1;
	counted.PinResidual	= worst;
}


bool Hybrid_Deform_Terrain(const HybridRealizationPlan & plan,const HybridConstants & k,const HybridHydrologySet * hydrology,
									const HybridHeightGrid & grid,float * heights,HybridDeformationStats * stats,
									DynamicVectorClass<HybridTerrainClaim> * claims)
{
	if ((heights == nullptr) || (grid.CountX <= 0) || (grid.CountY <= 0) || !(grid.Spacing > 0.0f)) return false;
	HybridDeformationStats local;
	HybridDeformationStats & counted = (stats != nullptr) ? *stats : local;
	counted = HybridDeformationStats();
	if (claims != nullptr) claims->Delete_All();
	Deform_Roads(plan,k,hydrology,grid,heights,counted,claims);
	Realize_Pins(plan,k,hydrology,grid,heights,counted,claims,false);
	return true;
}


/***********************************************************************************************
**	D-027 -- structural pins against the terrain grid, judged before Phase 12.
***********************************************************************************************/

float Hybrid_Realized_Surface_Tolerance(const HybridConstants & k,float cell,HybridRoadClass road_class)
{
	const float grade = (float)::tan((double)(Hybrid_Road_Class_Spec(road_class).MaxGradeDegrees * PI_OVER_180));
	return (2.0f * k.Q) + (grade * cell * 1.41421356f);
}


float Hybrid_Realized_Grade_Limit(HybridRoadClass road_class,float base,float q)
{
	const float angle = (Hybrid_Road_Class_Spec(road_class).MaxGradeDegrees * PI_OVER_180) + HYBRID_QUANTUM_ANGLE;
	return (float)::tan((double)angle) + ((2.0f * q) / Max2(base,q));
}


float Hybrid_Realized_Cross_Limit(HybridRoadClass road_class,float base,float q)
{
	return (float)::tan((double)(Hybrid_Road_Class_Spec(road_class).MaxCrossSlopeDegrees * PI_OVER_180)) +
			 ((2.0f * q) / Max2(base,q));
}


/*
**	The judgement's view of the realized world, built from the plan the way Phase 12's runtime objects are: every road's
**	ground stretches (segments between two non-structure samples), bucketed, every road's centerline, and every deck.
*/
struct PinSegment
{
	PinSegment(void) : Road(-1),Sample(0) { }
	bool	operator == (const PinSegment & that) const	{ return (Road == that.Road) && (Sample == that.Sample); }
	bool	operator != (const PinSegment & that) const	{ return !(*this == that); }

	int	Road;
	int	Sample;			// the segment Samples[Sample - 1] .. Samples[Sample]
};

struct PinLine
{
	bool	operator == (const PinLine & that) const	{ return &Points == &that.Points; }
	bool	operator != (const PinLine & that) const	{ return !(*this == that); }

	DynamicVectorClass<Vector2>	Points;
	DynamicVectorClass<float>		Stations;
};

struct PinWorld
{
	const HybridRealizationPlan *		Plan;
	const HybridHeightGrid *			Grid;
	const float *							Heights;
	DynamicVectorClass<PinSegment>	Segments;
	DynamicVectorClass<PinLine>		Lines;			// per road
	float										BucketOriginX;
	float										BucketOriginY;
	float										BucketSize;
	int										BucketsX;
	int										BucketsY;
	DynamicVectorClass<int>				Head;				// per bucket, into Next / Entry
	DynamicVectorClass<int>				Next;
	DynamicVectorClass<int>				Entry;			// into Segments
};


static void Pin_World_Build(PinWorld * w,float bucket)
{
	const HybridRealizationPlan & plan = *w->Plan;
	Vector2 lo(3.4e38f,3.4e38f);
	Vector2 hi(-3.4e38f,-3.4e38f);
	float reach = 0.0f;
	for (int r = 0; r < plan.Roads.Count(); ++r) {
		const HybridRealizedRoad & road = plan.Roads[r];
		PinLine line;
		w->Lines.Add(line);
		PinLine & added = w->Lines[w->Lines.Count() - 1];
		for (int i = 0; i < road.Samples.Count(); ++i) {
			const HybridRealizedSample & s = road.Samples[i];
			added.Points.Add(s.Position);
			added.Stations.Add(s.Station);
			lo.Set(Min2(lo.X,s.Position.X),Min2(lo.Y,s.Position.Y));
			hi.Set(Max2(hi.X,s.Position.X),Max2(hi.Y,s.Position.Y));
			reach = Max2(reach,Max2(s.Left,s.Right));
			if ((i > 0) && !road.Samples[i - 1].Structure && !s.Structure) {
				PinSegment seg;
				seg.Road		= r;
				seg.Sample	= i;
				w->Segments.Add(seg);
			}
		}
	}
	w->BucketSize		= bucket;
	w->BucketOriginX	= lo.X - reach - bucket;
	w->BucketOriginY	= lo.Y - reach - bucket;
	w->BucketsX			= Max_Int((int)::ceil((double)((hi.X + reach + bucket - w->BucketOriginX) / bucket)) + 1,1);
	w->BucketsY			= Max_Int((int)::ceil((double)((hi.Y + reach + bucket - w->BucketOriginY) / bucket)) + 1,1);
	w->Head.Resize(w->BucketsX * w->BucketsY);
	for (int b = 0; b < (w->BucketsX * w->BucketsY); ++b) w->Head.Add(-1);
	for (int s = 0; s < w->Segments.Count(); ++s) {
		const HybridRealizedRoad & road = plan.Roads[w->Segments[s].Road];
		const HybridRealizedSample & a = road.Samples[w->Segments[s].Sample - 1];
		const HybridRealizedSample & b = road.Samples[w->Segments[s].Sample];
		const float extent = Max2(Max2(a.Left,a.Right),Max2(b.Left,b.Right));
		const int x0 = Max_Int((int)::floor((double)((Min2(a.Position.X,b.Position.X) - extent - w->BucketOriginX) / bucket)),0);
		const int y0 = Max_Int((int)::floor((double)((Min2(a.Position.Y,b.Position.Y) - extent - w->BucketOriginY) / bucket)),0);
		const int x1 = Min_Int((int)::floor((double)((Max2(a.Position.X,b.Position.X) + extent - w->BucketOriginX) / bucket)),w->BucketsX - 1);
		const int y1 = Min_Int((int)::floor((double)((Max2(a.Position.Y,b.Position.Y) + extent - w->BucketOriginY) / bucket)),w->BucketsY - 1);
		for (int by = y0; by <= y1; ++by) {
			for (int bx = x0; bx <= x1; ++bx) {
				const int index = (by * w->BucketsX) + bx;
				w->Next.Add(w->Head[index]);
				w->Head[index] = w->Entry.Count();
				w->Entry.Add(s);
			}
		}
	}
}


//	The station and paved edges of a plan road at `station`, and its solved surface there.
static void Pin_Plan_At(const HybridRealizedRoad & road,float station,float * z_out,float * left_out,float * right_out)
{
	const int count = road.Samples.Count();
	int lo = 0;
	int hi = count - 1;
	while ((hi - lo) > 1) {
		const int mid = (lo + hi) / 2;
		if (road.Samples[mid].Station <= station) {
			lo = mid;
		} else {
			hi = mid;
		}
	}
	const HybridRealizedSample & a = road.Samples[lo];
	const HybridRealizedSample & b = road.Samples[hi];
	const float span = b.Station - a.Station;
	const float t = (span > 0.0f) ? Clamp3((station - a.Station) / span,0.0f,1.0f) : 0.0f;
	*z_out		= a.Z + ((b.Z - a.Z) * t);
	*left_out	= a.Left + ((b.Left - a.Left) * t);
	*right_out	= a.Right + ((b.Right - a.Right) * t);
}


/*
**	Is `q` on some road's paved roadbed (Section 12.3's On_Paved_Road over the plan): the earliest such road and the
**	station on it.  A segment's band between its samples, and the joint at an interior sample of a ground stretch.
*/
static bool Pin_Paved_Owner(const PinWorld & w,const Vector2 & q,float slack,int * owner_out,float * station_out)
{
	*owner_out = -1;
	const int bx = (int)::floor((double)((q.X - w.BucketOriginX) / w.BucketSize));
	const int by = (int)::floor((double)((q.Y - w.BucketOriginY) / w.BucketSize));
	if ((bx < 0) || (by < 0) || (bx >= w.BucketsX) || (by >= w.BucketsY)) return false;
	for (int e = w.Head[(by * w.BucketsX) + bx]; e >= 0; e = w.Next[e]) {
		const PinSegment & seg = w.Segments[w.Entry[e]];
		if ((*owner_out >= 0) && (seg.Road >= *owner_out)) continue;
		const HybridRealizedRoad & road = w.Plan->Roads[seg.Road];
		const HybridRealizedSample & a = road.Samples[seg.Sample - 1];
		const HybridRealizedSample & b = road.Samples[seg.Sample];
		const Vector2 d = b.Position - a.Position;
		const Vector2 off = q - a.Position;
		const float length2 = (d.X * d.X) + (d.Y * d.Y);
		bool inside = false;
		float station = a.Station;
		if (length2 > 0.0f) {
			const float t = ((off.X * d.X) + (off.Y * d.Y)) / length2;
			if ((t >= 0.0f) && (t <= 1.0f)) {
				const float lateral = ((d.X * off.Y) - (d.Y * off.X)) / (float)sqrt((double)length2);
				const float left = a.Left + ((b.Left - a.Left) * t);
				const float right = a.Right + ((b.Right - a.Right) * t);
				inside = (lateral <= (left + slack)) && (-lateral <= (right + slack));
				station = a.Station + ((b.Station - a.Station) * t);
			}
		}
		if (!inside && (seg.Sample > 1) && !road.Samples[seg.Sample - 2].Structure) {
			const Vector2 side(-d.Y,d.X);
			const bool left_side = ((off.X * side.X) + (off.Y * side.Y)) >= 0.0f;
			inside = (off.Length() <= ((left_side ? a.Left : a.Right) + slack));
			station = a.Station;
		}
		if (!inside) continue;
		*owner_out		= seg.Road;
		*station_out	= station;
	}
	return (*owner_out >= 0);
}


//	The deck over `q`, if any (BridgeSystem::Conform_Point over the plan's assemblies): the higher of two.
static bool Pin_Deck_At(const HybridRealizationPlan & plan,const Vector2 & q,float * z_out)
{
	bool found = false;
	for (int b = 0; b < plan.Bridges.Count(); ++b) {
		const HybridBridgeAssembly & assembly = plan.Bridges[b];
		const Vector2 d = assembly.End - assembly.Start;
		const float length2 = (d.X * d.X) + (d.Y * d.Y);
		if (!(length2 > 0.0f)) continue;
		const Vector2 off = q - assembly.Start;
		const float t = ((off.X * d.X) + (off.Y * d.Y)) / length2;
		if ((t < 0.0f) || (t > 1.0f)) continue;
		const float lateral = (float)fabs((double)(((d.X * off.Y) - (d.Y * off.X)) / (float)sqrt((double)length2)));
		if (lateral > (0.5f * assembly.Width)) continue;
		const float z = assembly.StartZ + ((assembly.EndZ - assembly.StartZ) * t);
		if (!found || (z > *z_out)) *z_out = z;
		found = true;
	}
	return found;
}


//	What a vehicle expecting `expected` stands on at `q` (Section 12.3's Surface_At): a deck when nearer, the ground else.
static bool Pin_Support(const PinWorld & w,const Vector2 & q,float expected,float * support_out)
{
	float ground = 0.0f;
	if (!Hybrid_Grid_Height_At(*w.Grid,w.Heights,q,&ground)) return false;
	*support_out = ground;
	float deck = 0.0f;
	if (Pin_Deck_At(*w.Plan,q,&deck) && ((float)fabs((double)(deck - expected)) <= (float)fabs((double)(ground - expected)))) {
		*support_out = deck;
	}
	return true;
}


/*
**	One envelope at one plan sample, exactly as Section 12.3's swept query places and judges it: placed on the roadbed
**	midpoint, else flush with either paved edge, wholly on paved surface or a deck at the road's height; then standing
**	on the surface it should within the ordinary tolerance, at the class grade and cross slope.
*/
static bool Pin_Sweep(const PinWorld & w,const HybridConstants & k,int road_index,int sample,float length,float width,
							 StringClass * detail_out)
{
	const HybridRealizedRoad & road = w.Plan->Roads[road_index];
	const PinLine & line = w.Lines[road_index];
	const int count = road.Samples.Count();
	const float total = road.Samples[count - 1].Station;
	const float half_length = 0.5f * length;
	const float half_width = 0.5f * width;
	const float spacing = Hybrid_Profile_Sample_Spacing(k);
	const float centre_station = Clamp3(road.Samples[sample].Station,Min2(half_length,0.5f * total),Max2(total - half_length,0.5f * total));
	const float tolerance = Hybrid_Realized_Surface_Tolerance(k,w.Grid->Spacing,road.RoadClass);

	Vector2 point[3];
	Vector2 left[3];
	float expected[3];
	float left_edge[3];
	float right_edge[3];
	for (int a = 0; a < 3; ++a) {
		const float at = Clamp3(centre_station + (half_length * (float)(a - 1)),0.0f,total);
		point[a] = Hybrid_Polyline_Point_At(line.Points,line.Stations,at);
		const Vector2 direction = Unit_Vector(Hybrid_Polyline_Point_At(line.Points,line.Stations,Min2(at + spacing,total)) -
														  Hybrid_Polyline_Point_At(line.Points,line.Stations,Max2(at - spacing,0.0f)));
		left[a].Set(-direction.Y,direction.X);
		Pin_Plan_At(road,at,&expected[a],&left_edge[a],&right_edge[a]);
	}

	Vector2 q[3][3];
	float surface[3][3];
	float limit[3][3];
	bool found = false;
	for (int placement = 0; (placement < 3) && !found; ++placement) {
		bool all = true;
		for (int a = 0; (a < 3) && all; ++a) {
			const float centre = (placement == 0) ? (0.5f * (left_edge[a] - right_edge[a]))
									 : ((placement == 1) ? (left_edge[a] - half_width) : (half_width - right_edge[a]));
			for (int b = 0; (b < 3) && all; ++b) {
				const float offset = centre + (half_width * (float)(b - 1));
				q[a][b] = point[a] + (left[a] * offset);
				surface[a][b] = expected[a];
				limit[a][b] = tolerance;
				int owner = -1;
				float owner_station = 0.0f;
				const bool own = (offset <= (left_edge[a] + k.Q)) && (-offset <= (right_edge[a] + k.Q));
				if (Pin_Paved_Owner(w,q[a][b],k.Q,&owner,&owner_station) || own) {
					if ((owner >= 0) && (owner < road_index)) {
						float ignored_left = 0.0f;
						float ignored_right = 0.0f;
						Pin_Plan_At(w.Plan->Roads[owner],owner_station,&surface[a][b],&ignored_left,&ignored_right);
						limit[a][b] = Max2(tolerance,Hybrid_Realized_Surface_Tolerance(k,w.Grid->Spacing,w.Plan->Roads[owner].RoadClass));
					}
					continue;
				}
				float deck = 0.0f;
				all = Pin_Deck_At(*w.Plan,q[a][b],&deck) && ((float)fabs((double)(deck - expected[a])) <= tolerance);
			}
		}
		found = all;
	}
	if (!found) {
		if (detail_out != nullptr) {
			detail_out->Format("road %u: the %.2f-wide envelope leaves the paved surface at station %.2f",road.EdgeId,width,
									 centre_station);
		}
		return false;
	}

	float support[3][3];
	for (int a = 0; a < 3; ++a) {
		for (int b = 0; b < 3; ++b) {
			float stand = 0.0f;
			if (!Pin_Support(w,q[a][b],surface[a][b],&stand)) {
				if (detail_out != nullptr) detail_out->Format("road %u: the envelope leaves the terrain at station %.2f",road.EdgeId,centre_station);
				return false;
			}
			const float off = stand - surface[a][b];
			if ((float)fabs((double)off) > limit[a][b]) {
				if (detail_out != nullptr) {
					detail_out->Format("road %u: the envelope stands %.4f off the solved surface at (%.2f,%.2f)",road.EdgeId,off,
											 q[a][b].X,q[a][b].Y);
				}
				return false;
			}
			support[a][b] = expected[a] + off;
		}
	}
	const float grade = (float)fabs((double)(support[2][1] - support[0][1])) / Max2(length,k.Q);
	const float cross = (float)fabs((double)(support[1][2] - support[1][0])) / Max2(width,k.Q);
	if ((grade > Hybrid_Realized_Grade_Limit(road.RoadClass,length,k.Q)) ||
		 (cross > Hybrid_Realized_Cross_Limit(road.RoadClass,width,k.Q))) {
		if (detail_out != nullptr) {
			detail_out->Format("road %u: the %.2f x %.2f envelope at station %.2f stands on a %.4f grade and a %.4f cross slope",
									 road.EdgeId,length,width,centre_station,grade,cross);
		}
		return false;
	}
	return true;
}


//	One ground sample's realized surface (Section 12.3's road-surface check): centre and paved edges, and cross slope.
static bool Pin_Road_Surface(const PinWorld & w,const HybridConstants & k,int road_index,int sample,StringClass * detail_out)
{
	const HybridRealizedRoad & road = w.Plan->Roads[road_index];
	const HybridRealizedSample & s = road.Samples[sample];
	const int ahead = ((sample + 1) < road.Samples.Count()) ? (sample + 1) : sample;
	const int behind = (sample > 0) ? (sample - 1) : sample;
	const Vector2 direction = Unit_Vector(road.Samples[ahead].Position - road.Samples[behind].Position);
	const Vector2 side(-direction.Y,direction.X);
	float departure[3];
	for (int b = 0; b < 3; ++b) {
		const Vector2 q = (b == 0) ? (s.Position - (side * s.Right)) : ((b == 1) ? s.Position : (s.Position + (side * s.Left)));
		float surface = s.Z;
		float tolerance = Hybrid_Realized_Surface_Tolerance(k,w.Grid->Spacing,road.RoadClass);
		int owner = -1;
		float owner_station = 0.0f;
		if (Pin_Paved_Owner(w,q,k.Q,&owner,&owner_station) && (owner < road_index)) {
			float ignored_left = 0.0f;
			float ignored_right = 0.0f;
			Pin_Plan_At(w.Plan->Roads[owner],owner_station,&surface,&ignored_left,&ignored_right);
			tolerance = Max2(tolerance,Hybrid_Realized_Surface_Tolerance(k,w.Grid->Spacing,w.Plan->Roads[owner].RoadClass));
		}
		float stand = 0.0f;
		if (!Pin_Support(w,q,surface,&stand) || ((float)fabs((double)(stand - surface)) > tolerance)) {
			if (detail_out != nullptr) {
				detail_out->Format("road %u's realized surface at (%.2f,%.2f) stands %.4f off its solved %.3f",road.EdgeId,q.X,q.Y,
										 stand - surface,surface);
			}
			return false;
		}
		departure[b] = stand - surface;
	}
	const float width = Max2(s.Left + s.Right,k.Q);
	const float cross = (float)fabs((double)(departure[2] - departure[0])) / width;
	if (cross > Hybrid_Realized_Cross_Limit(road.RoadClass,width,k.Q)) {
		if (detail_out != nullptr) {
			detail_out->Format("road %u's roadbed at (%.2f,%.2f) is realized at a %.4f cross slope",road.EdgeId,s.Position.X,
									 s.Position.Y,cross);
		}
		return false;
	}
	return true;
}


bool Hybrid_Plan_Rasterization(const HybridRealizationPlan & plan,const HybridConstants & k,const HybridHydrologySet * hydrology,
										 const HybridHeightGrid & grid,HybridRouteHeightSampleFunc sample,void * context,float max_cut,
										 float max_fill,const HybridPinEnvelopes & envelopes,DynamicVectorClass<HybridRasterIssue> * issues)
{
	if (issues != nullptr) issues->Delete_All();
	const float cell = grid.Spacing;
	const float near_reach = Hybrid_Pin_Landing_Length(cell) + (2.0f * cell);
	float widest = 0.0f;
	float longest = 0.0f;
	for (int e = 0; e < HYBRID_ENVELOPE_COUNT; ++e) {
		widest	= Max2(widest,envelopes.Width[e]);
		longest	= Max2(longest,envelopes.Length[e]);
	}

	//	The patch: the field's own vertices under every road and its reach, at the heights the field holds there now.
	Vector2 lo(3.4e38f,3.4e38f);
	Vector2 hi(-3.4e38f,-3.4e38f);
	for (int r = 0; r < plan.Roads.Count(); ++r) {
		for (int i = 0; i < plan.Roads[r].Samples.Count(); ++i) {
			const Vector2 & p = plan.Roads[r].Samples[i].Position;
			lo.Set(Min2(lo.X,p.X),Min2(lo.Y,p.Y));
			hi.Set(Max2(hi.X,p.X),Max2(hi.Y,p.Y));
		}
	}
	for (int n = 0; n < plan.Pins.Count(); ++n) {
		lo.Set(Min2(lo.X,plan.Pins[n].Position.X),Min2(lo.Y,plan.Pins[n].Position.Y));
		hi.Set(Max2(hi.X,plan.Pins[n].Position.X),Max2(hi.Y,plan.Pins[n].Position.Y));
	}
	if (!(hi.X >= lo.X)) return true;
	const float reach = longest + widest + Hybrid_Road_Outer_Blend_Width(k.W,k.A) + (8.0f * k.W) + (2.0f * cell);
	HybridHeightGrid patch;
	patch.Spacing = cell;
	const int ix0 = (int)::floor((double)((lo.X - reach - grid.OriginX) / cell));
	const int iy0 = (int)::floor((double)((lo.Y - reach - grid.OriginY) / cell));
	const int ix1 = (int)::ceil((double)((hi.X + reach - grid.OriginX) / cell));
	const int iy1 = (int)::ceil((double)((hi.Y + reach - grid.OriginY) / cell));
	patch.OriginX	= grid.OriginX + ((float)ix0 * cell);
	patch.OriginY	= grid.OriginY + ((float)iy0 * cell);
	patch.CountX	= (ix1 - ix0) + 1;
	patch.CountY	= (iy1 - iy0) + 1;
	DynamicVectorClass<float> original;
	original.Resize(patch.CountX * patch.CountY);
	for (int iy = 0; iy < patch.CountY; ++iy) {
		for (int ix = 0; ix < patch.CountX; ++ix) {
			float z = 0.0f;
			sample(Vector2(patch.OriginX + (cell * (float)ix),patch.OriginY + (cell * (float)iy)),&z,context);
			original.Add(z);
		}
	}

	//	Phase 12 exactly: the roads, then the pins.
	DynamicVectorClass<float> heights;
	heights.Resize(original.Count());
	for (int v = 0; v < original.Count(); ++v) heights.Add(original[v]);
	HybridDeformationStats stats;
	Deform_Roads(plan,k,hydrology,patch,&heights[0],stats,nullptr);
	DynamicVectorClass<float> raw;
	raw.Resize(heights.Count());
	for (int v = 0; v < heights.Count(); ++v) raw.Add(heights[v]);
	Realize_Pins(plan,k,hydrology,patch,&heights[0],stats,nullptr,true);

	bool clean = true;
	StringClass detail;

	//	Each pin: its own surface at 2Q, and the ground it moved inside cut/fill.
	for (int n = 0; n < plan.Pins.Count(); ++n) {
		const HybridRealizedPin & pin = plan.Pins[n];
		const char * kind = Hybrid_Realized_Pin_Kind_Name(pin.Kind);
		float surface = 0.0f;
		int vertex[3] = { -1,-1,-1 };
		float weight[3];
		bool failed = !Hybrid_Grid_Height_At(patch,&heights[0],pin.Position,&surface,vertex,weight) ||
						  ((float)fabs((double)(surface - pin.Z)) > (2.0f * k.Q));
		if (failed) {
			detail.Format("the %s pin at (%.2f,%.2f) stands %.4f off its frozen %.3f on the grid",kind,pin.Position.X,pin.Position.Y,
							  surface - pin.Z,pin.Z);
		}
		for (int i = 0; (i < 3) && !failed && (vertex[i] >= 0); ++i) {
			const int v = vertex[i];
			if (heights[v] == raw[v]) continue;
			if (((original[v] - heights[v]) > (max_cut + k.Q)) || ((heights[v] - original[v]) > (max_fill + k.Q))) {
				detail.Format("the %s pin at (%.2f,%.2f) moves the ground %.3f, past its cut/fill",kind,pin.Position.X,pin.Position.Y,
								  heights[v] - original[v]);
				failed = true;
			}
		}
		if (!failed) continue;
		clean = false;
		if (issues != nullptr) {
			HybridRasterIssue issue;
			issue.Pin		= n;
			issue.Road		= pin.RoadIndex;
			issue.Detail	= detail;
			issues->Add(issue);
		}
	}

	//	Section 12.3 over every ground sample of every road: its surface, and every envelope it must carry.
	PinWorld w;
	w.Plan		= &plan;
	w.Grid		= &patch;
	w.Heights	= &heights[0];
	Pin_World_Build(&w,4.0f * k.W);
	for (int r = 0; r < plan.Roads.Count(); ++r) {
		const HybridRealizedRoad & road = plan.Roads[r];
		bool road_reported = false;
		for (int i = 0; i < road.Samples.Count(); ++i) {
			if (road.Samples[i].Structure) continue;
			StringClass why;
			bool ok = Pin_Road_Surface(w,k,r,i,&why);
			for (int e = 0; (e < HYBRID_ENVELOPE_COUNT) && ok; ++e) {
				const unsigned int needs = (e == HYBRID_ENVELOPE_LIGHT) ? ~0u
												 : ((e == HYBRID_ENVELOPE_HEAVY) ? (unsigned int)HYBRID_NAV_HEAVY : (unsigned int)HYBRID_NAV_HARVESTER);
				if ((road.Capabilities & needs) == 0) continue;
				if (!(envelopes.Length[e] > 0.0f) || !(envelopes.Width[e] > 0.0f)) continue;
				ok = Pin_Sweep(w,k,r,i,envelopes.Length[e],envelopes.Width[e],&why);
			}
			if (ok) continue;
			clean = false;
			//	A failure within L_pin + 2 cells of a pin is that pin's; anywhere else it is the road's, once per road.
			int nearest = -1;
			float best = near_reach;
			for (int n = 0; n < plan.Pins.Count(); ++n) {
				const float distance = (plan.Pins[n].Position - road.Samples[i].Position).Length();
				if (distance > best) continue;
				best		= distance;
				nearest	= n;
			}
			if ((nearest < 0) && road_reported) continue;
			if (nearest < 0) road_reported = true;
			if (issues != nullptr) {
				bool known = false;
				for (int s = 0; (s < issues->Count()) && !known; ++s) known = (nearest >= 0) && ((*issues)[s].Pin == nearest);
				if (known) continue;
				HybridRasterIssue issue;
				issue.Pin		= nearest;
				issue.Road		= r;
				issue.Detail	= why;
				issues->Add(issue);
			}
		}
	}
	return clean;
}
