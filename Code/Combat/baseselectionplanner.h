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
 *                     $Archive:: /Commando/Code/Combat/baseselectionplanner.h                $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	V2 roadmap Section 34 Phase 11 -- final base selection and deterministic fairness repair.    *
 *	Sections 16, 17.4, 17.5; the Section 17A.1 thresholds this generator gates against were       *
 *	already parsed onto ProceduralWorldProfile::Fairness in Phase 1, waiting for a consumer.       *
 *                                                                                             *
 *	Choose, not discover.  StrategicPlanner (Phase 7) already produced every BASE_CANDIDATE        *
 *	this generator will ever see, each one a real, terrain-scored possibility; RoadGraphGenerator   *
 *	(Phase 8) already connected every one of them into a single minimum-spanning tree, so a real     *
 *	road distance between any two candidates already exists rather than the straight-line             *
 *	estimate Phase 7's own Get_Travel_Cost documents itself as a stand-in for; BridgePlanner           *
 *	(Phase 9/10) already resolved or rejected every crossing that tree needed.  This file's own         *
 *	job is exactly Phase 11's purpose line: pick the one GDI/Nod pair that is actually fair,             *
 *	given everything already built, never re-deriving terrain, roads or bridges itself.                  *
 *                                                                                             *
 *	Site selection, not base layout.  Section 16A's own contract -- initial building transforms,        *
 *	factory-exit corridors, spawn sets, match bootstrap -- is a distinct, larger piece of work            *
 *	this generator does not attempt; Phase 11's own "Implement in order" list never asks for a             *
 *	building transform, only a site, a protected region and a repair log.  What this generator            *
 *	does honour from the roadmap's own Phase 11 completion note is reserving that region now, so           *
 *	Phase 12 (Tiberium) and Phase 13 (settlements) have real ground to exclude rather than a bare           *
 *	point -- see ProtectedRadius below and docs/procedural_world/BaseSelection.md for what is and          *
 *	is not resolved.                                                                                       *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef BASESELECTIONPLANNER_H
#define BASESELECTIONPLANNER_H

#ifndef ALWAYS_H
#include "always.h"
#endif

#include "vector.h"
#include "vector3.h"
#include "wwstring.h"


//	Phase 11 step 4, named exactly as that step lists it.
enum BaseRepairOperationType
{
	BASE_REPAIR_NONE = 0,
	BASE_REPAIR_ADJUST_ROAD_EDGE,
	BASE_REPAIR_ADD_RAMP,
	BASE_REPAIR_ADD_TUNNEL_CANDIDATE,
	BASE_REPAIR_MOVE_RESOURCE_CANDIDATE,
	BASE_REPAIR_MOVE_BASE,
	BASE_REPAIR_ADJUST_LOCAL_TERRAIN,
	BASE_REPAIR_OPERATION_TYPE_COUNT
};

const char *	Base_Repair_Operation_Name(BaseRepairOperationType op);


/*
**	One faction's chosen site -- Phase 11 step 9's manifest anchor, and step 8's own reserved
**	region.  `ProtectedRadius` is a conservative circle, not Section 16A's real footprint/
**	clearance/approach-zone shape (that needs canonical building descriptors this generator
**	never touches); it exists so a later phase has real ground to exclude rather than a bare
**	point, sized from the same `Fairness.MinBaseExpansionArea` step 1's own "expansion room"
**	criterion already scores candidates against.
*/
class SelectedBaseSite
{
public:
	SelectedBaseSite(void);

	bool	operator == (const SelectedBaseSite & that) const;
	bool	operator != (const SelectedBaseSite & that) const	{ return !(*this == that); }

	StringClass	Id;					// the StrategicNode BASE_CANDIDATE id this selection names
	int			Faction;				// a PLAYERTYPE value
	Vector3		Position;
	float			ProtectedRadius;	// world units

	//	Step 3's own per-faction metrics, computed for the pair actually chosen.
	float			InitialResourceTravelTimeSeconds;	// -1 when no reachable resource candidate exists
	int			VehicleRouteCount;							// -1 when no wheeled-vehicle graph could be built
	int			InfantryRouteCount;							// -1 when no infantry graph could be built
};


struct BaseFairnessMetrics
{
	BaseFairnessMetrics(void);

	float	ResourceTravelTimeDifferenceSeconds;
	float	BaseToBaseTravelTimeSeconds;
	bool		SingleBridgeDependency;	// true when the only road route between the pair crosses exactly one bridge
	bool		PassedThresholds;
};


class BaseRepairLogEntry
{
public:
	BaseRepairLogEntry(void);

	bool	operator == (const BaseRepairLogEntry & that) const;
	bool	operator != (const BaseRepairLogEntry & that) const	{ return !(*this == that); }

	int							PairAttemptIndex;	// which ranked candidate pair this repair happened under
	BaseRepairOperationType	Operation;
	StringClass					TargetId;				// the StrategicNode/RoadGraphEdge id the repair touched
	bool							Applied;					// false when the operation had nothing left to try
};


struct BaseSelectionParams
{
	BaseSelectionParams(void);

	unsigned int	Seed;

	//	Copied from the profile by the caller, Section 17A.1's own fields, named identically.
	float	MaxInitialResourceTravelTimeDifferenceSeconds;
	float	MaxBaseToBaseTravelTimeDifferenceSeconds;
	float	MinBaseExpansionArea;
	bool		AllowSingleBridgeDependency;

	//	Step 5's own bound, sourced by the caller from the same
	//	ProceduralWorldRetryLimits::MaxLocalRetriesPerStage every other bounded local-repair loop
	//	in this chain already answers to, rather than a second ceiling invented for this file.
	int	MaxRepairAttemptsPerPair;

	//	Section 3A.1-flavoured reference speeds, world units per second.  No profile or content
	//	field supplies a canonical vehicle/infantry travel speed -- this is a fairness *estimate*,
	//	not a physics quantity RoadGraphGenerator or MoveablePhysClass are asked to honour -- so
	//	these are fixed, documented constants (baseselectionplanner.cpp's own comment gives the
	//	real-world reference each is drawn from), the same reasoning Get_World_Units_Per_Meter
	//	already used for the one other undefined physical constant this chain needed.
	float	ReferenceVehicleSpeed;
	float	ReferenceInfantrySpeed;
};


class BaseSelectionPlanner
{
public:

	static void			Init(void);
	static void			Shutdown(void);

	/*
	**	Requires StrategicPlanner::Has_Plan(), RoadGraphGenerator::Has_Graph() and
	**	BridgePlanner::Has_Plan() -- Phase 11's own purpose line: choose bases only once roads,
	**	bridges and terrain constraints are known.  Returns true with no selection when fewer
	**	than one GDI and one Nod BASE_CANDIDATE exist to choose between, which is a correct,
	**	unremarkable outcome for a plan too small to have produced one; `Has_Selection` is how a
	**	caller learns whether that happened.  Fails (false) only when every candidate pair was
	**	tried and none passed even after repair -- step 7's own "generation failure with exact
	**	diagnostic".
	*/
	static bool			Generate(const BaseSelectionParams & params,StringClass * error_out);

	static void			Clear(void);
	static bool			Has_Selection(void)		{ return Built; }

	static bool			Get_Selected_Site(int faction,SelectedBaseSite * out);
	static const BaseFairnessMetrics &	Get_Fairness_Metrics(void)	{ return LastMetrics; }

	static int			Get_Repair_Log_Count(void)	{ return RepairLog.Count(); }
	static bool			Get_Repair_Log_Entry(int index,BaseRepairLogEntry * out);

	//	Step 6: how many ranked candidate pairs were tried and rejected before the one finally
	//	chosen (0 when the first-ranked pair passed outright, with or without repair).
	static int			Get_Rejected_Pair_Count(void)	{ return RejectedPairCount; }

	static unsigned int	Get_Checksum(void)		{ return Checksum; }

	static int			Check_Both_Bases_Have_Build_Area(StringClass * report);
	static int			Check_Both_Bases_Have_Resource_Access(StringClass * report);
	static int			Check_Connectivity_Requirements(bool allow_single_bridge_dependency,StringClass * report);
	static int			Check_Travel_Time_Within_Threshold(float max_resource_time_difference,
																			float max_base_to_base_time,StringClass * report);

private:

	static bool			Built;
	static unsigned int	Checksum;

	static SelectedBaseSite		GdiSite;
	static SelectedBaseSite		NodSite;
	static bool						HasGdiSite;
	static bool						HasNodSite;

	static BaseFairnessMetrics	LastMetrics;
	static DynamicVectorClass<BaseRepairLogEntry>	RepairLog;
	static int						RejectedPairCount;
};

#endif	// BASESELECTIONPLANNER_H
