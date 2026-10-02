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
 *                     $Archive:: /Commando/Code/wwphys/hybridrealization.h                   $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Hybrid world generator rewrite, item I13 -- runtime Phase 12, the part that is arithmetic.       *
 *                                                                                               *
 * Phase 12 consumes the frozen Phase-10 splines and Phase-11 compiled interactions and turns them  *
 * into roads on the ground.  It does not reroute, move a crossing, change a method or reopen       *
 * Phases 9 to 11; a failure here fails the attempt.  Two things about it are pure computation over   *
 * frozen records and live here, where a self-check can prove them exactly:                        *
 *                                                                                               *
 *   - Section 12.1's deterministic vertical-profile solver: sample a road interval at              *
 *     `min(W/2, A/3)`, give every sample its `RoadVerticalInterval` (or the fixed elevation a       *
 *     compiled crossing or junction endpoint requires), propagate the class grade forward and       *
 *     backward, walk the clamp, quantize, revalidate.  The output is the centerline the road will   *
 *     be built to, or a named refusal;                                                             *
 *   - Section 12.2's cross-section: at each solved sample, the roadbed is flat at the solved         *
 *     height, the shoulder is at the roadbed edge, and the ground blends from the shoulder edge to    *
 *     untouched terrain with a smoothstep over exactly `outer_blend = max(W, A/2)`.                *
 *                                                                                               *
 * The terrain edit itself, and the RoadSystem / BridgeSystem / TunnelSystem objects, belong to     *
 * their owners and are driven from ProceduralWorldManager::Realize_Hybrid_Roads.                   *
 *                                                                                               *
 *---------------------------------------------------------------------------------------------*/

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef HYBRIDREALIZATION_H
#define HYBRIDREALIZATION_H

#include "always.h"
#include "vector.h"
#include "vector2.h"
#include "wwstring.h"
#include "hybridplanning.h"
#include "hybridroutes.h"
#include "hybridinteractions.h"


/*
**	Why a profile could not be solved.  Every one is a hard attempt failure: the road was accepted by
**	Phase 10 and its crossings compiled by Phase 11, so a profile that cannot exist means an earlier
**	phase promised something this one cannot build, and that is reported rather than patched.
*/
enum HybridProfileFailure
{
	HYBRID_PROFILE_OK = 0,
	HYBRID_PROFILE_INVALID_INPUT,				// too few samples, no height, or a degenerate spacing
	HYBRID_PROFILE_FORWARD_EMPTY,				// forward propagation emptied an interval
	HYBRID_PROFILE_BACKWARD_EMPTY,			// backward propagation emptied an interval
	HYBRID_PROFILE_CLAMP_EMPTY,				// the clamp walk found no legal height at a sample
	HYBRID_PROFILE_GRADE_AFTER_QUANTIZE,	// quantization pushed an adjacent grade past the class maximum
	HYBRID_PROFILE_FAILURE_COUNT
};

const char *	Hybrid_Profile_Failure_Name(HybridProfileFailure failure);


/*
**	One sample of a road interval the solver reads: where it is, the terrain there before this road
**	touches it, and -- when a compiled crossing or junction fixes the height here -- that height.
*/
struct HybridProfileSample
{
	HybridProfileSample(void) :
		Position(0.0f,0.0f),Station(0.0f),TerrainZ(0.0f),Fixed(false),FixedZ(0.0f),Unbounded(false) { }

	//	DynamicVectorClass<T> (wwlib/vector.h) requires T to support == and !=.
	bool	operator == (const HybridProfileSample & that) const
	{
		return (Station == that.Station) && (Position == that.Position) && (TerrainZ == that.TerrainZ) &&
				 (Fixed == that.Fixed) && (FixedZ == that.FixedZ) && (Unbounded == that.Unbounded);
	}
	bool	operator != (const HybridProfileSample & that) const	{ return !(*this == that); }

	Vector2	Position;
	float		Station;			// along-road arc length from the interval start
	float		TerrainZ;		// Phase-7 / current terrain height before deformation
	bool		Fixed;			// a compiled deck, portal or junction elevation pins this sample
	float		FixedZ;

	/*
	**	A sample on a structure that carries its own surface -- a canyon deck, a tunnel bore -- is not on the
	**	ground, so no cut or fill interval applies there; only the grade does.  Its `TerrainZ` is then the line
	**	the structure keeps to rather than the ground under it, so the clamp walk holds the deck on the line
	**	between its two ends instead of pulling it down toward a canyon floor.
	*/
	bool		Unbounded;
};


/*
**	The solved profile: one height per sample, quantized to `Q`, every adjacent grade inside the class
**	maximum, and the cut and fill each sample asks of the ground for the record.
*/
struct HybridSolvedProfile
{
	HybridSolvedProfile(void) : SteepestGrade(0.0f),DeepestCut(0.0f),HighestFill(0.0f) { }

	void	Reset(void)	{ Z.Delete_All(); SteepestGrade = 0.0f; DeepestCut = 0.0f; HighestFill = 0.0f; }

	DynamicVectorClass<float>	Z;					// solved height per sample
	float								SteepestGrade;	// radians, on the quantized profile
	float								DeepestCut;		// max(terrain - z)
	float								HighestFill;	// max(z - terrain)
};


/*
**	Section 12.1.  `samples` are in station order with `Station` strictly increasing; the first and last
**	are the interval's exact endpoints.  `max_cut_depth` / `max_fill_height` are the profile's grading
**	limits and `max_grade_radians` the class maximum.  Deterministic: the same samples always give the
**	same profile.
*/
bool	Hybrid_Solve_Vertical_Profile(const DynamicVectorClass<HybridProfileSample> & samples,
												  float max_cut_depth,float max_fill_height,float max_grade_radians,
												  float quantum,HybridSolvedProfile * out,HybridProfileFailure * failure_out);


/*
**	Section 12.2's cross-section at one sample, as a function of signed lateral offset from the
**	centerline: the roadbed is level at `solved_z` out to `half_roadbed`, and from there the ground blends
**	to `terrain_z_at_offset` with a smoothstep over exactly `outer_blend`.  Beyond `half_roadbed +
**	outer_blend` the terrain is untouched.  `Hybrid_Road_Cross_Section_Half_Width` is that outer reach.
*/
float	Hybrid_Road_Cross_Section_Height(float lateral,float solved_z,float terrain_z_at_offset,
													 float half_roadbed,float outer_blend);
float	Hybrid_Road_Cross_Section_Half_Width(HybridRoadClass road_class,float w,float a);


/*
**	The sample spacing Section 12.1 prescribes, `min(W/2, A/3)`, and the interval sampler that builds a
**	station-ordered sample list from a route's point range with exact endpoints.  `first_point` and
**	`last_point` are indices into `route.Points`; the height callback is the current terrain.
*/
float	Hybrid_Profile_Sample_Spacing(const HybridConstants & constants);

bool	Hybrid_Sample_Route_Interval(const HybridRoute & route,int first_point,int last_point,float spacing,
												 HybridRouteHeightSampleFunc sample,void * context,
												 DynamicVectorClass<HybridProfileSample> * out);

/*
**	The same sampler over any station-ordered polyline and any station range on it: the exact position at
**	`from_station`, every whole multiple of `spacing` after it, and the exact position at `to_station`.  A
**	compiled crossing's structure ends -- a bridge abutment, a tunnel portal -- fall between polyline points,
**	and the road has to be sampled up to exactly there.  Sample stations are measured from `from_station`.
*/
bool	Hybrid_Sample_Polyline_Range(const DynamicVectorClass<Vector2> & points,
												 const DynamicVectorClass<float> & stations,float from_station,
												 float to_station,float spacing,HybridRouteHeightSampleFunc sample,
												 void * context,DynamicVectorClass<HybridProfileSample> * out);

//	The point at `station` on a station-ordered polyline, clamped to its two ends.
Vector2	Hybrid_Polyline_Point_At(const DynamicVectorClass<Vector2> & points,
											 const DynamicVectorClass<float> & stations,float station);


/*
**	A structure on a road, as Section 12.1 sees it: where it starts and stops along the road, and how it holds
**	the profile there.  A LEVEL structure -- a river deck, an overpass deck -- holds every sample on it at one
**	elevation.  A CARRIED one -- a canyon or ravine deck, a tunnel bore -- spans between the road's own
**	elevations at its two ends and holds its interior to the grade alone on the line between them; its ends are
**	pinned once a compiled record has fixed them, and solved with the road while they have not.
*/
enum HybridProfileStructureMode
{
	HYBRID_PROFILE_STRUCTURE_LEVEL = 0,
	HYBRID_PROFILE_STRUCTURE_CARRIED,
};

struct HybridProfileStructure
{
	HybridProfileStructure(void) :
		From(0.0f),To(0.0f),Mode(HYBRID_PROFILE_STRUCTURE_LEVEL),EndsFixed(false),StartZ(0.0f),EndZ(0.0f) { }

	//	DynamicVectorClass<T> requires == and !=.
	bool	operator == (const HybridProfileStructure & that) const
	{
		return (From == that.From) && (To == that.To) && (Mode == that.Mode) && (EndsFixed == that.EndsFixed) &&
				 (StartZ == that.StartZ) && (EndZ == that.EndZ);
	}
	bool	operator != (const HybridProfileStructure & that) const	{ return !(*this == that); }

	float								From;				// stations along the road, From < To
	float								To;
	HybridProfileStructureMode	Mode;
	bool								EndsFixed;		// CARRIED only: StartZ / EndZ are compiled pins
	float								StartZ;			// LEVEL: the deck elevation; CARRIED: the pinned ends
	float								EndZ;
};

/*
**	Section 12.1 over one stretch `[from_station, to_station]` of a station-ordered road centerline, with every
**	structure on it holding its own surface.  The stretch is sampled at `spacing` with its two ends and every
**	structure end inside it as exact samples of their own, then solved once.  This is the one vertical-profile
**	path on the hybrid startup path: Phase 11 asks it whether a crossing can be graded, and Phase 12 realizes
**	the road it returns.  `start_z_out` / `end_z_out` receive each structure's solved end elevations, or its
**	pins, in the order of `structures`; a structure outside the stretch reports its pins unchanged.
*/
bool	Hybrid_Solve_Road_Stretch(const DynamicVectorClass<Vector2> & points,const DynamicVectorClass<float> & stations,
										  float from_station,float to_station,
										  const DynamicVectorClass<HybridProfileStructure> & structures,float spacing,
										  HybridRouteHeightSampleFunc sample,void * context,float max_cut_depth,
										  float max_fill_height,float max_grade_radians,float quantum,
										  DynamicVectorClass<HybridProfileSample> * samples_out,
										  HybridSolvedProfile * solved_out,DynamicVectorClass<float> * start_z_out,
										  DynamicVectorClass<float> * end_z_out,HybridProfileFailure * failure_out);


#endif	// HYBRIDREALIZATION_H
