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


static inline float Max2(float a,float b)	{ return (a > b) ? a : b; }
static inline float Min2(float a,float b)	{ return (a < b) ? a : b; }
static inline float Clamp3(float v,float lo,float hi)	{ return (v < lo) ? lo : ((v > hi) ? hi : v); }

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
		const float floor = Max2(lo[i],z[i - 1] - (g * ds));
		const float ceiling = Min2(hi[i],z[i - 1] + (g * ds));
		if (floor > ceiling) {
			if (failure_out != nullptr) *failure_out = HYBRID_PROFILE_CLAMP_EMPTY;
			return false;
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
