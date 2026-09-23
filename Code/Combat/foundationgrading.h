/*
**	Command & Conquer Renegade(tm)
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
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/foundationgrading.h                   $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	V2 roadmap Section 34 Phase 17 steps 3-9 -- bounded foundation grading.                      *
 *                                                                                             *
 *	The Commander may flatten the ground a building stands on.  That is the whole feature, and     *
 *	every interesting thing about it is a limit: how deep, how high, how much slope change, how     *
 *	much area, and what it is never allowed to touch.  Without those, "grade the terrain under a     *
 *	building" is a tool for deleting a river, a cliff or a road, and Section 1.6's own rule --         *
 *	runtime mutation happens inside the fixed perimeter, not to it -- would be a suggestion.           *
 *                                                                                             *
 *	So this file is shaped as a two-phase operation rather than a function that changes the           *
 *	ground.  `Evaluate` measures a request against the real heightfield and the real protected        *
 *	features and answers with a full report -- cut volume, fill volume, depth, rise, slope           *
 *	change, area, and the first limit it breaks.  `Execute` takes a request that already             *
 *	evaluated clean, re-evaluates it (because the world may have moved between the two, and           *
 *	because a server must never trust a report a client computed), and only then touches the        *
 *	terrain.  A caller cannot reach the second without the first having said yes.                     *
 *                                                                                             *
 *	Canonical ownership.  The ground has one owner -- `WorldTerrainSystem`, whose own header        *
 *	says so -- and this file does not become a second one.  It computes what to change and then     *
 *	asks that owner to change it through `Modify_Height_Region`, the same primitive                 *
 *	`Apply_Road_Grade` and `Apply_River_Cut` are already built from, and then asks it to rebuild    *
 *	collision and invalidate the far layer exactly as those two do.  Nothing here writes a          *
 *	height sample directly.                                                                          *
 *                                                                                             *
 *	The apron.  Step 4 says "cut/fill volume inside footprint plus transition apron", and the        *
 *	apron is not decoration: a footprint flattened with a vertical wall around it is a               *
 *	discontinuity in the collision surface that units catch on.  The apron is the ring between       *
 *	the footprint radius and the radius at which the graded height has blended back to the           *
 *	original ground, and its width is derived -- the height difference divided by the profile's      *
 *	own maximum slope change -- rather than named, so a tall cut gets a long ramp and a shallow       *
 *	one does not.                                                                                     *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef FOUNDATIONGRADING_H
#define FOUNDATIONGRADING_H

#ifndef ALWAYS_H
#include "always.h"
#endif

#include "vector.h"
#include "vector3.h"
#include "wwstring.h"

class AABoxClass;


/*
**	Why a grading request was refused.  Step 5's four profile limits and step 6's protected
**	features, one value each, plus the two structural refusals that come before either.
*/
enum FoundationGradingResultType
{
	FOUNDATION_GRADING_VALID = 0,
	FOUNDATION_GRADING_NO_TERRAIN,			// no generated heightfield to grade
	FOUNDATION_GRADING_NO_DESCRIPTOR,		// the request names no loaded descriptor
	FOUNDATION_GRADING_PLACEMENT_INVALID,	// the ghost this grading was requested from is not valid
	FOUNDATION_GRADING_DEPTH_EXCEEDED,
	FOUNDATION_GRADING_HEIGHT_EXCEEDED,
	FOUNDATION_GRADING_SLOPE_CHANGE_EXCEEDED,
	FOUNDATION_GRADING_AREA_EXCEEDED,
	FOUNDATION_GRADING_PROTECTED_FEATURE,

	FOUNDATION_GRADING_RESULT_COUNT
};

const char *	Foundation_Grading_Result_Name(FoundationGradingResultType result);


/*
**	Step 5's profile limits.  Defaults are the conservative end of what the existing generated
**	world already tolerates -- MaxSlopeChangeRadians is the buildable-slope cap every
**	terrain-reading generator in this chain shares, so a grading operation can never leave ground
**	steeper than the ground a generator would have been willing to make.
*/
struct FoundationGradingLimits
{
	FoundationGradingLimits(void);

	float	MaxCutDepth;				// world units, > 0 -- how far below original ground a cut may go
	float	MaxFillHeight;				// world units, > 0 -- how far above it a fill may go
	float	MaxSlopeChangeRadians;	// > 0 -- the steepest the apron may be left
	float	MaxAffectedArea;			// world units^2, > 0 -- footprint plus apron, in plan
};


struct FoundationGradingRequest
{
	FoundationGradingRequest(void) : Position(0.0f,0.0f,0.0f),RotationDegrees(0.0f),
												Team(-1),SequenceId(0),DescriptorName("") { }

	Vector3		Position;
	float			RotationDegrees;
	int			Team;
	int			SequenceId;
	StringClass	DescriptorName;
};


/*
**	What the measurement found.  Returned whether the request passed or failed, because "this
**	cut is 0.4 metres too deep" is actionable where "rejected" is not -- the same reason
**	`BridgeClearanceReportStruct` reports a number rather than a bool.
*/
struct FoundationGradingReport
{
	FoundationGradingReport(void) : Result(FOUNDATION_GRADING_VALID),
											  TargetHeight(0.0f),CutVolume(0.0f),FillVolume(0.0f),
											  MaxCutDepth(0.0f),MaxFillHeight(0.0f),
											  MaxSlopeChangeRadians(0.0f),AffectedArea(0.0f),
											  ApronWidth(0.0f),PlateauRadius(0.0f),SampleCount(0),
											  ProtectedFeature(""),SequenceId(0) { }

	bool	Is_Valid(void) const	{ return Result == FOUNDATION_GRADING_VALID; }

	FoundationGradingResultType	Result;

	float		TargetHeight;				// the height the footprint would be flattened to
	float		CutVolume;					// world units^3 removed
	float		FillVolume;					// world units^3 added
	float		MaxCutDepth;				// deepest single cut found
	float		MaxFillHeight;				// tallest single fill found
	float		MaxSlopeChangeRadians;	// steepest slope the graded surface would leave
	float		AffectedArea;				// world units^2, footprint plus apron
	float		ApronWidth;					// world units, the transition ring's own width

	/*
	**	The radius actually levelled, which is the footprint disc plus one heightfield cell
	**	diagonal.  Wider than the footprint on purpose -- see foundationgrading.cpp on why a pad
	**	that stops at the footprint leaves its own rim on a slope.  Reported rather than
	**	recomputed by Execute so that one measurement decides the shape exactly once.
	*/
	float		PlateauRadius;

	int		SampleCount;				// how many heightfield samples were measured

	StringClass	ProtectedFeature;		// which one, when Result is PROTECTED_FEATURE
	int			SequenceId;
};


class FoundationGradingService
{
public:

	static void		Init(void);
	static void		Shutdown(void);

	static void		Set_Limits(const FoundationGradingLimits & limits);
	static const FoundationGradingLimits &	Get_Limits(void);

	/*
	**	Steps 3-6.  Measure a grading request against the real heightfield and the real protected
	**	features, and report.  Changes nothing, so a ghost may call it every time the cursor
	**	moves and the server may call it again on arrival and get the same answer.
	**
	**	Requires WorldTerrainSystem::Has_Terrain(); answers FOUNDATION_GRADING_NO_TERRAIN
	**	otherwise, which is every hand-authored map (directive 1.10 -- a legacy level's Commander
	**	simply cannot grade, it is not broken).
	*/
	static FoundationGradingReport	Evaluate(const FoundationGradingRequest & request);

	/*
	**	Steps 7-9.  Re-evaluate, and if it still passes, execute through the canonical terrain
	**	owner, rebuild the affected collision and far-terrain region, and revalidate the placement
	**	server-side on the ground as it now is.
	**
	**	The re-evaluation is not redundant.  Between a client's preview and the server's
	**	execution another building may have been graded next door, and the second grading's own
	**	limits must be measured against the ground that grading left, not the ground the client
	**	saw.  The placement revalidation after the fact is step 9 exactly: grading moves the
	**	surface, and a footprint that was valid on a slope must still be valid on the flat.
	*/
	static bool		Execute(const FoundationGradingRequest & request,FoundationGradingReport * report_out);

	//	How many gradings this service has executed, and the sequence id of the last one.  What
	//	makes "the server really did grade, once" observable rather than merely true -- the same
	//	bookkeeping CommanderPlacementValidator::Validate_Server_Request already keeps.
	static int		Get_Executed_Count(void);
	static int		Get_Rejected_Count(void);
	static int		Get_Last_Executed_Sequence(void);

	//	The region the last executed grading changed, for a caller that wants to rebuild
	//	something of its own over it.  False before any grading has executed.
	static bool		Get_Last_Executed_Region(AABoxClass * region_out);

	static void		Reset_Counters(void);

private:

	static FoundationGradingLimits	Limits;

	static int		ExecutedCount;
	static int		RejectedCount;
	static int		LastExecutedSequence;
	static bool		HasLastRegion;
	static Vector3	LastRegionCenter;
	static Vector3	LastRegionExtent;
};

#endif	// FOUNDATIONGRADING_H
