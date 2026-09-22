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
 *                     $Archive:: /Commando/Code/Combat/combatspacevalidator.h                $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	V2 roadmap Section 34 Phase 15 -- FPS/RTS combat-space analysis and deterministic local        *
 *	repair.  Section 17, Section 1.2.                                                             *
 *                                                                                             *
 *	A validator, the same shape item 6's `TransportationNetworkValidator` already is -- reads       *
 *	state every earlier phase already produced (`BaseSelectionPlanner`, `TiberiumFieldGenerator`,    *
 *	`SettlementGenerator`, `RoadGraphGenerator`), never re-derives it, and reports rather than          *
 *	mutates.  Section 17A.1's own thresholds (`MaxMandatoryExposedInfantryRunDistance/                  *
 *	TimeSeconds`, `PreferredCoverInterval`, `MinVehicleLaneClearance`, `MinVehicleApproachCount`,          *
 *	`MinInfantryApproachCount`) were already parsed onto `ProceduralWorldProfile::Fairness` in           *
 *	Phase 1 for exactly this phase; no new schema field was needed.                                        *
 *                                                                                             *
 *	Repair, honestly not attempted.  Step 7 names eight repair operations; every one of them             *
 *	either mutates state a phase 4-13 generator already committed (a road, a bridge, a placed              *
 *	structure) with no cheap undo, or needs a full regeneration of a downstream generator this               *
 *	validator does not own.  `BridgePlanner`, `BaseSelectionPlanner` and `TiberiumFieldGenerator`             *
 *	each already do real, bounded, in-place repair *during their own generation*, where undo is                *
 *	still cheap; retrofitting a second repair pass after every generator has already committed is               *
 *	the unsafe case those files' own refused repairs already document.  So this phase's own step 12               *
 *	("reject... rather than silently accepting") folds into `ProceduralWorldManager::Generate_World`'s              *
 *	own existing bounded seed-retry loop (gap report item 5) as one more input, the same way                        *
 *	`TransportationNetworkValidator`'s required-base-routes flag already does -- a hard failure asks                 *
 *	for a fresh seed, not a silent pass.                                                                              *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef COMBATSPACEVALIDATOR_H
#define COMBATSPACEVALIDATOR_H

#ifndef ALWAYS_H
#include "always.h"
#endif

#include "vector.h"
#include "vector3.h"
#include "wwstring.h"


//	Step 6, named exactly as that step lists it.
enum CombatSpaceFailureType
{
	COMBAT_FAILURE_DISCONNECTED_BASE = 0,
	COMBAT_FAILURE_NO_RESOURCE_PATH,
	COMBAT_FAILURE_IMPOSSIBLE_CHOKE,
	COMBAT_FAILURE_EXPOSED_INFANTRY_MARCH,
	COMBAT_FAILURE_UNUSABLE_VEHICLE_LANE,
	COMBAT_FAILURE_TYPE_COUNT
};

const char *	Combat_Space_Failure_Name(CombatSpaceFailureType type);


//	Step 7, named exactly as that step lists it.  See the class comment for why none of these is
//	ever attempted -- named for diagnostic vocabulary completeness, the same pattern every other
//	generator's own unreachable enum values in this chain already use.
enum CombatSpaceRepairOperationType
{
	COMBAT_REPAIR_TERRAIN_FOLD_BERM = 0,
	COMBAT_REPAIR_COVER_PLACEMENT,
	COMBAT_REPAIR_INFANTRY_SIDE_PATH,
	COMBAT_REPAIR_ROAD_SPUR,
	COMBAT_REPAIR_RAMP,
	COMBAT_REPAIR_TUNNEL_CANDIDATE,
	COMBAT_REPAIR_SECOND_CROSSING,
	COMBAT_REPAIR_MOVE_POI_RESOURCE,
	COMBAT_REPAIR_OPERATION_TYPE_COUNT
};

const char *	Combat_Space_Repair_Operation_Name(CombatSpaceRepairOperationType op);


class CombatSpaceFailureRecord
{
public:
	CombatSpaceFailureRecord(void);

	bool	operator == (const CombatSpaceFailureRecord & that) const;
	bool	operator != (const CombatSpaceFailureRecord & that) const	{ return !(*this == that); }

	CombatSpaceFailureType	Type;
	StringClass					Description;
};


//	Step 13's own "human-readable validation report", plus the real numbers step 1 asks a
//	profile to threshold against.  Section 17A.4's own P50/P90/max, from real terrain-occlusion
//	sightline sampling -- see combatspacevalidator.cpp's own comment for exactly what counts as
//	an occluder and what does not.
struct CombatSpaceMetrics
{
	CombatSpaceMetrics(void);

	int		SightlineSampleCount;
	float		SightlineP50;
	float		SightlineP90;
	float		SightlineMax;

	float		BaseToBaseTravelTimeSeconds;		// -1 when no selection exists to measure
	bool			SingleBridgeDependency;
	int		OwnedFieldsWithoutRoute;				// TiberiumFieldGenerator fields with an owner but no route
	int		SettlementsWithoutRoadConnection;
};


struct CombatSpaceValidationParams
{
	CombatSpaceValidationParams(void);

	unsigned int	Seed;

	//	Copied from profile.Fairness by the caller, Section 17A.1's own fields, named identically
	//	-- already parsed since Phase 1 for exactly this phase, no new schema field needed.
	float	MaxMandatoryExposedInfantryRunTimeSeconds;
	float	MinVehicleLaneClearance;

	//	The same fairness-estimate reference infantry speed BaseSelectionPlanner already uses
	//	and documents -- an infantry-specific route time needs the same undefined-physical-
	//	quantity treatment a vehicle one does, not a second invented number.
	float	ReferenceInfantrySpeed;
};


class CombatSpaceValidator
{
public:

	/*
	**	Runs every check below in order, logs one WWDEBUG_SAY line per finding, and returns true
	**	only when no hard failure (Section 6's own list) was found.  Soft findings (a sightline
	**	distribution, a travel time) never fail this on their own -- the same "structural finding,
	**	not automatically world-breaking" distinction TransportationNetworkValidator's own header
	**	already draws.  `report_out`, when given, collects the same lines the log gets.
	*/
	static bool			Validate(const CombatSpaceValidationParams & params,StringClass * report_out);

	static void			Clear(void);

	static const CombatSpaceMetrics &	Get_Metrics(void)	{ return Metrics; }

	static int			Get_Failure_Count(void)	{ return Failures.Count(); }
	static bool			Get_Failure(int index,CombatSpaceFailureRecord * out);

	static bool			Passed(void)	{ return LastResult; }

private:

	static CombatSpaceMetrics	Metrics;
	static DynamicVectorClass<CombatSpaceFailureRecord>	Failures;
	static bool						LastResult;
};

#endif	// COMBATSPACEVALIDATOR_H
