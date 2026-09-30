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
	**	The feasibility intervals.  A sample a compiled crossing or junction pins has `lo = hi = fixed`;
	**	everything else may sit anywhere in its `RoadVerticalInterval`.
	*/
	DynamicVectorClass<float> lo;
	DynamicVectorClass<float> hi;
	for (int i = 0; i < count; ++i) {
		const HybridProfileSample & s = samples[i];
		if (s.Fixed) {
			lo.Add(s.FixedZ);
			hi.Add(s.FixedZ);
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
	DynamicVectorClass<float> z;
	z.Add(Clamp3(samples[0].TerrainZ,lo[0],hi[0]));
	for (int i = 1; i < count; ++i) {
		const float ds = samples[i].Station - samples[i - 1].Station;
		const float floor = Max2(lo[i],z[i - 1] - (g * ds));
		const float ceiling = Min2(hi[i],z[i - 1] + (g * ds));
		if (floor > ceiling) {
			if (failure_out != nullptr) *failure_out = HYBRID_PROFILE_CLAMP_EMPTY;
			return false;
		}
		z.Add(Clamp3(samples[i].TerrainZ,floor,ceiling));
	}

	/*
	**	4. Quantize to `Q`, then revalidate every adjacent grade against the class maximum plus the
	**	angular quantum.  A pinned sample is quantized like any other: the crossing that pinned it
	**	compiled a quantized height already, so this is the identity there.
	*/
	for (int i = 0; i < count; ++i) z[i] = Hybrid_Quantize_Position(z[i]);

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

	const float start_station = route.Stations[first_point];

	/*
	**	The interval's start, then every whole multiple of `spacing` along the polyline in station order,
	**	then the exact end.  A multiple that lands on or past a route point is placed on the segment that
	**	actually contains that station, so no gap ever exceeds `spacing` and the two endpoints are exact.
	*/
	const float end_station = route.Stations[last_point] - start_station;
	if (end_station <= 0.0f) return false;

	HybridProfileSample first;
	first.Position = route.Points[first_point];
	first.Station = 0.0f;
	if (!sample(first.Position,&first.TerrainZ,context)) return false;
	out->Add(first);

	int segment = first_point;
	//	Every multiple strictly inside the interval, so the final gap to the exact end is never over `spacing`;
	//	a multiple within a quantum of the end is the end.
	for (float next = spacing; next < (end_station - HYBRID_QUANTUM_POSITION); next += spacing) {

		//	Advance to the segment that contains `next`.
		while ((segment < (last_point - 1)) && ((route.Stations[segment + 1] - start_station) <= next)) segment++;

		const Vector2 & a = route.Points[segment];
		const Vector2 & b = route.Points[segment + 1];
		const float sa = route.Stations[segment] - start_station;
		const float sb = route.Stations[segment + 1] - start_station;
		const float length = sb - sa;
		const float t = (length > 0.0f) ? Clamp3((next - sa) / length,0.0f,1.0f) : 0.0f;

		HybridProfileSample s;
		s.Position.Set(a.X + ((b.X - a.X) * t),a.Y + ((b.Y - a.Y) * t));
		s.Station = next;
		if (!sample(s.Position,&s.TerrainZ,context)) return false;
		out->Add(s);
	}

	HybridProfileSample end;
	end.Position = route.Points[last_point];
	end.Station = end_station;
	if (!sample(end.Position,&end.TerrainZ,context)) return false;
	out->Add(end);
	return out->Count() >= 2;
}
