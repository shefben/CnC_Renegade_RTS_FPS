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
 *                     $Archive:: /Commando/Code/wwphys/hybridlocalpass.h                     $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * The one local-pass builder: can a road get across this obstruction *on the ground*, and if so  *
 * along exactly what centerline?                                                                *
 *                                                                                             *
 * Section 8 Phase 6 step 18 asks for a "sampled `PRIMARY_2` swept path across the local           *
 * floor/ramp", and Section 8 Phase 11 offers a surface pass and a `RAMP_PASS_WINDOW` as methods.  *
 * Both need the same answer about the same geometry, so both ask it here.  Phase 6 proves a       *
 * candidate exists and freezes it; Phase 11 revalidates the frozen candidate and chooses the      *
 * method; Phase 12 realizes the road and moves the terrain.  Nothing in this file deforms         *
 * anything.                                                                                      *
 *                                                                                               *
 * The geometry that matters is *diagonal*.  A descent exists precisely because it may spend run   *
 * along the obstruction to buy grade: a wall too steep to climb straight down is often easy to    *
 * climb at an angle, and testing the straight normal line instead is what made every pass         *
 * unreachable on every seed.  So each ramp is built with the along-obstruction run the class      *
 * grade actually requires,                                                                       *
 *                                                                                               *
 *     minimum_horizontal_run = vertical_run / tan(PRIMARY_2.max_grade)                            *
 *     required_tangent_run   = sqrt(max(0, minimum_horizontal_run^2 - normal_run^2))              *
 *                                                                                               *
 * which is geometry rather than a tuning value.                                                   *
 *                                                                                               *
 * What the builder produces is *planned road geometry*, so it is judged as road: the profile has   *
 * to hold the class grade, turn radius and cross slope, and every sample of it has to be           *
 * realizable inside its own `RoadVerticalInterval`.  The untouched wall underneath being steeper   *
 * than a road may climb is exactly the point and is not a refusal.                                *
 *                                                                                               *
 *---------------------------------------------------------------------------------------------*/

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef HYBRIDLOCALPASS_H
#define HYBRIDLOCALPASS_H

#include "always.h"
#include <string.h>
#include "vector.h"
#include "vector2.h"
#include "hybridplanning.h"


/*
**	What shape of pass this is.  The two differ in construction, not in how they are judged: a canyon or
**	ravine is descended, crossed along its floor and climbed again, while a cliff or ridge band is climbed
**	once on the diagonal.
*/
enum HybridLocalPassKind
{
	HYBRID_LOCAL_PASS_FLOOR = 0,		// canyon/ravine: descend, cross the floor, ascend
	HYBRID_LOCAL_PASS_RAMP,				// cliff: one diagonal ramp across the face/talus band
	HYBRID_LOCAL_PASS_KIND_COUNT
};

const char *	Hybrid_Local_Pass_Kind_Name(HybridLocalPassKind kind);


//	Why a candidate was refused.  Reported per orientation, so a census can say which way was blocked.
enum HybridLocalPassFailure
{
	HYBRID_LOCAL_PASS_OK = 0,
	HYBRID_LOCAL_PASS_NO_GEOMETRY,			// degenerate frame, or no obstruction width to cross
	HYBRID_LOCAL_PASS_NO_HEIGHT,				// the height field has no answer somewhere on the candidate
	HYBRID_LOCAL_PASS_RUN,						// the obstruction has no room for the run the grade needs
	HYBRID_LOCAL_PASS_GRADE,					// the profile itself exceeds the class maximum grade
	HYBRID_LOCAL_PASS_TURN_RADIUS,			// a corner of the candidate is tighter than the class allows
	HYBRID_LOCAL_PASS_CROSS_SLOPE,
	HYBRID_LOCAL_PASS_CUT_FILL,				// a sample cannot be realized inside its RoadVerticalInterval
	HYBRID_LOCAL_PASS_FLOOR_WIDTH,			// the floor cannot carry the swept vehicle envelope
	HYBRID_LOCAL_PASS_FLOOR_GRADE,			// the floor runs steeper along itself than the class allows
	HYBRID_LOCAL_PASS_FAILURE_COUNT
};

const char *	Hybrid_Local_Pass_Failure_Name(HybridLocalPassFailure failure);


/*
**	The height field the candidate is measured against.  Phase 6 passes its provisional height, Phase 11
**	the realized macro terrain: the builder does not care which, only that one answer exists per point.
*/
typedef bool (*HybridLocalPassHeightFunc)(const Vector2 & world,float * height_out,void * context);


/*
**	Everything the builder needs that is not local to the station.
*/
struct HybridLocalPassParams
{
	HybridLocalPassParams(void);

	HybridConstants	Constants;

	float					MaxCutDepth;					// Section 7.1's vertical-interval inputs
	float					MaxFillHeight;

	float					SweptVehicleWidth;			// the widest envelope the pass must carry
	float					MinVehicleLaneClearance;	// fairness.min_vehicle_lane_clearance

	HybridRoadClass	PassClass;						// the class a pass is proved for: PRIMARY_2, per step 18
};


//	The most control points a pass centerline can need: approach, ramp feet, floor ends, approach.
#define HYBRID_LOCAL_PASS_MAX_CONTROLS 6


/*
**	One accepted local pass.  This is the record Phase 6 freezes and Phase 11 consumes, so it carries the
**	whole geometry rather than a verdict: the same pass has to be evaluated twice, not invented twice.
*/
struct HybridLocalPassGeometry
{
	HybridLocalPassGeometry(void)	{ ::memset (this,0,sizeof (*this)); }

	HybridLocalPassKind	Kind;
	int						Orientation;			// 0 = the first ramp leans along +tangent, 1 = along -tangent
	float						TangentRun;				// the along-obstruction run each ramp spends, quantized

	Vector2					Controls[HYBRID_LOCAL_PASS_MAX_CONTROLS];
	int						ControlCount;			// the ordered pass centerline, approach to approach

	bool						HasFloor;
	Vector2					FloorEntry;				// where the descent meets the floor
	Vector2					FloorExit;				// where the ascent leaves it

	float						Length;					// the candidate centerline's own length, quantized
	float						SteepestGrade;			// the worst grade on the accepted profile, radians
	float						TightestRadius;		// the tightest corner on it
};


/*
**	What the builder measured, whether or not it accepted anything.
**
**	A refusal is only useful if it says by how much: a station that needs 212 units of along-obstruction run
**	and has 210 is a different fact about the generator than one that needs 212 and has 40.  Every field here
**	is geometry the builder computed anyway, so nothing is recomputed to report it, and nothing here is part
**	of the world -- it is diagnosis, not state.
*/
struct HybridLocalPassMeasurement
{
	HybridLocalPassMeasurement(void)	{ ::memset (this,0,sizeof (*this)); }

	bool		Valid;						// the frame and the height field answered, so the numbers mean something

	float		NormalRun;					// the normal span one ramp has: rim to floor edge, or the whole band
	float		VerticalA;					// approach A down to the floor (or across the band)
	float		VerticalB;

	float		RequiredHorizontalA;		// vertical / tan(max_grade)
	float		RequiredHorizontalB;
	float		RequiredTangentA;			// sqrt(max(0, required_horizontal^2 - normal_run^2))
	float		RequiredTangentB;

	//	Directional, and orientation-independent: how much run the obstruction offers each way from here.
	float		AvailablePlus;
	float		AvailableMinus;

	/*
	**	The best the two deterministic orientations could do: the smaller of the two orientations' worst
	**	ramp ratio, `required_tangent_run / available_run`.  `<= 1` means the run fits and any refusal came
	**	from later validation; `> 1` means no orientation has the room.
	*/
	float		RatioBest;

	float		MaxGradeRadians;
	float		FloorHalfWidth;
	float		RimHalfWidth;
	float		Depth;						// the vertical the pass has to make
	float		FloorGrade;					// the floor segment's own longitudinal grade, when one was built

	/*
	**	What the compiled profile actually did, which is a different fact from the run: a station whose run
	**	fits and whose profile is still too steep is not short of obstruction, it is being judged by geometry
	**	that does not match what was built.  `ProfileLeg` is the control index the refusing leg starts at.
	*/
	float		ProfileGrade;
	int		ProfileLeg;

	HybridLocalPassFailure	Failure[2];	// per orientation, in the order they were tried
};


/*
**	Build and validate the local pass at one station.
**
**	`centre`, `tangent` and `normal` are the obstruction's local frame; `normal` points from approach A
**	toward approach B.  `obstruction_half` is the half span of the obstruction itself, `floor_half` and
**	`rim_half` come from the owning landform (a cliff passes `0` for the floor).  `approach_length` is how
**	far outside the obstruction each approach point stands.
**
**	`available_run_plus` and `available_run_minus` are how much along-obstruction run this station may spend
**	in each direction -- the caller knows them, because they are properties of the obstruction, not of the
**	road.  They are *directional* and rarely equal: a station is somewhere along the feature, so the run
**	toward one end is the arc distance to that end and the run toward the other is what is left.  A ramp may
**	not buy extra run by leaving the feature it is crossing, because past the end it is no longer this
**	window's local pass but ordinary routing around the obstacle, so a run the geometry needs and the
**	obstruction cannot offer is a `GRADE` refusal: there is no legal profile here.
**
**	A floor pass spends run *both* ways -- the descent leans one way and the ascent the other -- so it needs
**	each ramp's own required run to fit that ramp's own direction.  A cliff ramp spends run one way only.
**
**	Both tangent orientations are tried in a fixed order -- `+tangent` first, then `-tangent` -- and the
**	first fully legal candidate wins.  This is one descent, one floor where there is one, and one ascent:
**	there are no switchbacks, so a station that cannot fit that shape in either orientation is not
**	pass-capable.  Nothing here is drawn or randomized; the same inputs always produce the same pass.
**
**	`measure_out` is optional and is filled whether or not a candidate was accepted, so a census can say how
**	far short a refusal fell rather than only that it happened.
*/
bool	Hybrid_Build_Local_Pass(const HybridLocalPassParams & params,HybridLocalPassKind kind,
										 const Vector2 & centre,const Vector2 & tangent,const Vector2 & normal,
										 float obstruction_half,float floor_half,float rim_half,
										 float approach_length,float available_run_plus,float available_run_minus,
										 HybridLocalPassHeightFunc sample,void * context,
										 HybridLocalPassGeometry * out,HybridLocalPassFailure * failure_out,
										 HybridLocalPassMeasurement * measure_out = nullptr);

/*
**	Validate an already-frozen pass.  Phase 11 calls this on the geometry Phase 6 stored, at its own
**	sample spacing, so a `pass_capable` flag is never permission to skip validation.
*/
bool	Hybrid_Validate_Local_Pass(const HybridLocalPassParams & params,
											  const HybridLocalPassGeometry & geometry,float sample_spacing,
											  HybridLocalPassHeightFunc sample,void * context,
											  HybridLocalPassFailure * failure_out);

//	The sample spacing each phase uses: `min(W/2, A/3)`, the plan's own swept-route spacing.
float	Hybrid_Local_Pass_Sample_Spacing(const HybridConstants & constants);


#endif	// HYBRIDLOCALPASS_H
