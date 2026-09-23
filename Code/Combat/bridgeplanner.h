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
 *                     $Archive:: /Commando/Code/Combat/bridgeplanner.h                       $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	V2 roadmap Section 34 Phase 9 -- sectional bridge planning, generation, damage integration,  *
 *	and rerouting hooks.  Section 11.                                                           *
 *                                                                                             *
 *	Consume, resolve, realize through the one bridge system.  Every candidate this file reads     *
 *	comes from `RoadGraphGenerator::RoadGraphEdge::UnresolvedCrossingIds` (Phase 8 step 15) --      *
 *	this file never re-derives which roads need a crossing, only what to build there.  `BridgeSystem` *
 *	(`Code/wwphys/bridgesystem.h`, complete before this roadmap started) already owns bridge          *
 *	layout, collision, damage state and multiplayer state-block serialization; this generator          *
 *	calls `Add_Bridge`/`Build_Layout`/`Build_Geometry`/`Set_Damage_State`/`Connect_Roads`, the same     *
 *	entry points the console command and self check already use.                                        *
 *                                                                                             *
 *	Nav/damage hooks are free.  Step 11 asks to "link runtime bridge damage state to nav                 *
 *	invalidation" -- `WorldTraversabilityGraph` (Section 52B Phase 65, already complete) already            *
 *	reads live `BridgeSystem` state (`Is_Traversable`/`Get_Traversable_Extent`/`Crossing_Carries`)          *
 *	every time it is built, so a bridge this generator places is respected by it with no new code            *
 *	in either file.  Nothing here duplicates that.                                                            *
 *                                                                                             *
 *	What is NOT resolved this phase.  Section 8 step 6, "adjust road approaches to exact bridge               *
 *	endpoints", is not implemented: the road this bridge crosses under still runs through as one                *
 *	continuous centre line rather than being split and re-terminated at the bridge's own two                     *
 *	abutments.  `RoadSystem` has no way to remove or split an already-added road (the same "add-or-               *
 *	clear" limitation every shared collection in this tree has), and rebuilding Phase 8's roads to                 *
 *	pre-split at every crossing is real scope this phase does not take on.  The bridge itself is a                  *
 *	real, correctly-placed, correctly-conformed object either way -- `BridgeClass::Conform_Point`                    *
 *	answers for its own deck -- what is missing is a caller at that exact point knowing to prefer it                  *
 *	over the ground.  See docs/procedural_world/BridgePlanning.md.                                                     *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef BRIDGEPLANNER_H
#define BRIDGEPLANNER_H

#ifndef ALWAYS_H
#include "always.h"
#endif

#include "vector.h"
#include "wwstring.h"


class ResolvedBridge
{
public:
	ResolvedBridge(void);

	bool	operator == (const ResolvedBridge & that) const;
	bool	operator != (const ResolvedBridge & that) const	{ return !(*this == that); }

	StringClass		Id;						// stable id, Section 3B.5
	int				BridgeId;				// the BridgeSystem::Add_Bridge id this resolution owns
	StringClass		CrossingNodeId;		// the StrategicNode (CROSSING_CANDIDATE/TUNNEL_CANDIDATE) this resolves
	StringClass		RoadEdgeId;				// the RoadGraphEdge this bridge belongs to
	int				InitialState;			// a BridgeDamageStateType
	bool				CountsAsConnectivity;	// step 10 -- false for an initially-BROKEN bridge

	//	Step 12.  A coarse, storage-only heuristic (by the road class it carries) -- no AI
	//	Commander consumer exists yet, the same "reserved, unconsumed" state several earlier
	//	fields in this generation chain were in before their own consumer arrived.
	float				StrategicImportance;
};


/*
**	Gap report item 5.  Why a crossing candidate could not be resolved at all, once every
**	permitted fallback has been tried -- named exactly the categories the gap report itself
**	lists, so a failed seed's own diagnostics read the same vocabulary the report does.
*/
enum CrossingFailureReasonType
{
	CROSSING_FAILURE_NONE = 0,
	CROSSING_FAILURE_SPAN_TOO_LONG,
	CROSSING_FAILURE_APPROACH_GRADE_TOO_HIGH,
	CROSSING_FAILURE_NO_VALID_ABUTMENT,
	CROSSING_FAILURE_INSUFFICIENT_CLEARANCE,
	CROSSING_FAILURE_ROAD_WIDTH_MISMATCH,
	CROSSING_FAILURE_COLLISION,
	CROSSING_FAILURE_OUT_OF_BOUNDS,
	CROSSING_FAILURE_NO_ALTERNATE_ROUTE,
	CROSSING_FAILURE_TYPE_COUNT
};

const char *	Crossing_Failure_Reason_Name(CrossingFailureReasonType reason);


/*
**	Which fallback stage actually resolved a crossing -- gap report item 5's own numbered list.
**	Stage 3 (alternate crossing type) is never reported here: an unresolved crossing becomes a
**	TunnelPlanner candidate after this plan completes (V2 Phase 10), with its own diagnostics.
*/
enum CrossingFallbackStageType
{
	CROSSING_STAGE_ORIGINAL_POSITION = 0,
	CROSSING_STAGE_LOCAL_REPOSITION,
	CROSSING_STAGE_EXTENDED_APPROACH,
	CROSSING_STAGE_ALTERNATE_CROSSING_TYPE,
	CROSSING_STAGE_REROUTE,
	CROSSING_STAGE_COUNT
};


/*
**	One crossing's own resolution record -- gap report item 5's own "expose these in the
**	procedural-generation diagnostics so a failed seed can be reproduced and understood" ask.
**	Kept for every crossing this generator was ever asked to resolve, on the edge it started on
**	or its replacement after a reroute, whether it finally succeeded or not, so a failed seed's
**	own story is readable from this list alone rather than a bare "bridge failed".
*/
class CrossingResolutionRecord
{
public:
	CrossingResolutionRecord(void);

	bool	operator == (const CrossingResolutionRecord & that) const;
	bool	operator != (const CrossingResolutionRecord & that) const	{ return !(*this == that); }

	StringClass						CrossingNodeId;
	StringClass						RoadEdgeId;
	bool								Resolved;
	int								AttemptsMade;
	CrossingFallbackStageType	StageResolvedAt;		// meaningful only when Resolved
	float								Displacement;				// world units off the original point, stage 1 only
	CrossingFailureReasonType	FinalFailureReason;	// meaningful only when !Resolved
};


struct BridgePlannerParams
{
	BridgePlannerParams(void);

	//	Already derived for the bridges domain by the caller.  Consumed for the initial-damage-
	//	state draw per bridge -- the one place this generator's output is not a pure function of
	//	the terrain and the road graph.
	unsigned int	Seed;

	float	WaterClearanceMargin;	// world units, > 0 -- Section 11A.3's flood/visual/gameplay margin
	float	MaxSpanLength;				// world units, > 0 -- step 3's own "reject impossible bridges" bound

	//	Gap report item 5.  The same shared slope cap every terrain-reading generator in this
	//	chain already uses, applied here to the approach either abutment makes.
	float	MaxApproachGradeRadians;

	//	Gap report item 5's own "bounded attempts... must not be capable of looping indefinitely",
	//	made a real number rather than an assumption: how many times fallback stage 4/5 (reroute)
	//	may retry one edge before that edge's own remaining crossings are declared unresolved.
	int	MaxRerouteAttemptsPerEdge;
};


class BridgePlanner
{
public:

	static void			Init(void);
	static void			Shutdown(void);

	/*
	**	Requires RoadGraphGenerator::Has_Graph() -- every candidate comes from an edge's own
	**	unresolved-crossing list.  Returns true with zero bridges when no edge carries one, which
	**	is correct for a world with no water/interior cliffs to cross.
	*/
	static bool			Generate(const BridgePlannerParams & params,StringClass * error_out);

	//	Drops only this generator's own bridges, the same owned-id idiom every other procedural-
	//	world generator in this tree uses for BridgeSystem's shared, add-or-clear collection.
	static void			Clear(void);

	static bool			Has_Plan(void)				{ return Built; }
	static int			Get_Bridge_Count(void)	{ return Bridges.Count(); }
	static bool			Get_Bridge(int index,ResolvedBridge * out);

	static unsigned int	Get_Checksum(void)		{ return Checksum; }

	/*
	**	Gap report item 5's own diagnostics.  One record per crossing this generator was asked
	**	to resolve, in the order it attempted them; a crossing that moved to a different edge
	**	after a reroute is still one record, naming whichever edge it finally settled on.
	*/
	static int			Get_Diagnostic_Count(void)	{ return Diagnostics.Count(); }
	static bool			Get_Diagnostic(int index,CrossingResolutionRecord * out);
	static int			Get_Unresolved_Count(void);

	static int			Check_Traversable_States_Pass(StringClass * report);
	static int			Check_Broken_States_Block(StringClass * report);
	static int			Check_No_Required_Connectivity_Lost(StringClass * report);

private:

	static bool			Built;
	static unsigned int	Checksum;

	static DynamicVectorClass<int>								OwnedBridgeIds;
	static DynamicVectorClass<ResolvedBridge>				Bridges;
	static DynamicVectorClass<CrossingResolutionRecord>	Diagnostics;
};

#endif	// BRIDGEPLANNER_H
