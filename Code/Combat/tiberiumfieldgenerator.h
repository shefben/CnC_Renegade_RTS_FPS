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
 *                     $Archive:: /Commando/Code/Combat/tiberiumfieldgenerator.h              $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	V2 roadmap Section 34 Phase 12 -- Tiberium fields, crystals, harvest areas, and Harvester    *
 *	routes.  Section 15, Section 17A.6.                                                          *
 *                                                                                             *
 *	Resolve, not discover.  Every candidate this generator turns into a real field already        *
 *	exists as a `STRATEGIC_NODE_RESOURCE_CANDIDATE` StrategicPlanner (Phase 7) placed and            *
 *	survived Phase 7's own economic-range gate against; every route this generator measures            *
 *	already exists as a path through RoadGraphGenerator's (Phase 8) own minimum spanning tree,           *
 *	found the same way BaseSelectionPlanner (Phase 11) measures a base-to-base route --                  *
 *	`RoadGraphGenerator::Find_Anchor_Path`, one owner for that walk, not a third copy of it.               *
 *                                                                                             *
 *	Data, not a spawned Harvester.  `HarvesterClass` (harvester.h) already has the whole contract         *
 *	a real Harvester needs -- `Set_Tiberium_Region`, `Set_Dock_Location`, `Set_Dock_Entrance` -- and         *
 *	already drives credits through the canonical economy once a level script gives it those three.           *
 *	No caller in this codebase spawns a Harvester for a generated world yet (the same "starting                *
 *	Harvesters" gap Section 16A.8's own match-bootstrap contract names and BaseSelectionPlanner's                *
 *	own header already declined to close), so this generator's job stops at producing the real,                  *
 *	authoritative field data -- position, footprint, owner, route -- that call is missing, not at                 *
 *	writing the call itself.                                                                                       *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef TIBERIUMFIELDGENERATOR_H
#define TIBERIUMFIELDGENERATOR_H

#ifndef ALWAYS_H
#include "always.h"
#endif

#include "vector.h"
#include "vector3.h"
#include "wwstring.h"


/*
**	Step 10, named exactly as that step lists it -- the same "named for completeness, not every
**	one attempted" pattern `BaseRepairOperationType` and `CrossingFallbackStageType` already use.
*/
enum ResourceRepairOperationType
{
	RESOURCE_REPAIR_NONE = 0,
	RESOURCE_REPAIR_ADJUST_FIELD_EDGE,
	RESOURCE_REPAIR_REPAIR_ROAD_LOCALLY,
	RESOURCE_REPAIR_ALTERNATE_FIELD_CANDIDATE,
	RESOURCE_REPAIR_OPERATION_TYPE_COUNT
};

const char *	Resource_Repair_Operation_Name(ResourceRepairOperationType op);


/*
**	One resolved field.  Id is Section 3B.5's stable identity, assigned in the same deterministic
**	build order every time the same seed/plan/road-graph are resolved.
*/
class ResolvedTiberiumField
{
public:
	ResolvedTiberiumField(void);

	bool	operator == (const ResolvedTiberiumField & that) const;
	bool	operator != (const ResolvedTiberiumField & that) const	{ return !(*this == that); }

	StringClass	Id;
	StringClass	ResourceNodeId;		// the StrategicNode RESOURCE_CANDIDATE this field resolves
	int			OwningFaction;			// PLAYERTYPE_GDI/NOD for an initial field, PLAYERTYPE_NEUTRAL for contested
	Vector3		Position;
	float			Radius;					// world units -- the field's own real footprint, <= the profile's own max
	float			InitialYield;			// Section 17A.6's own "initial yield"
	int			CrystalCount;

	bool			HasHarvesterRoute;
	float			HarvesterRouteLength;			// world units, real road distance, meaningful only when HasHarvesterRoute
	float			ExpectedRouteTimeSeconds;		// one-way, meaningful only when HasHarvesterRoute
	float			ExpectedIncomePerMinute;		// Section 17A.6's own "expected early income per minute"

	//	Step "both teams have viable early field" made concrete: true only for a non-contested
	//	field (a real OwningFaction) with a resolved route.
	bool			CountsAsEarlyField;
};


class TiberiumCrystalPlacement
{
public:
	TiberiumCrystalPlacement(void);

	bool	operator == (const TiberiumCrystalPlacement & that) const;
	bool	operator != (const TiberiumCrystalPlacement & that) const	{ return !(*this == that); }

	int		FieldIndex;		// into TiberiumFieldGenerator's own field list
	Vector3	Position;

	//	No live W3D crystal model is resolved here -- the same "missing art never blocks engine
	//	implementation" rule this project's own instructions state; placement is real, rendering
	//	is a request for the intended `ow_tiberium_crystal` family of assets, not a guess at a
	//	preset identifier this project does not enumerate.
};


class ResourceRepairLogEntry
{
public:
	ResourceRepairLogEntry(void);

	bool	operator == (const ResourceRepairLogEntry & that) const;
	bool	operator != (const ResourceRepairLogEntry & that) const	{ return !(*this == that); }

	StringClass						CandidateResourceNodeId;
	ResourceRepairOperationType	Operation;
	bool									Applied;
};


struct TiberiumFieldParams
{
	TiberiumFieldParams(void);

	unsigned int	Seed;

	//	Copied from the profile's own Resource block by the caller, named identically.
	float	FieldRadiusMin;
	float	FieldRadiusMax;
	float	InitialYieldMin;
	float	InitialYieldMax;
	float	CrystalDensity;
	int	HarvestAnchorCount;
	float	HarvesterLoadAmount;
	bool		GrowthEnabled;
	float	GrowthRatePerSecond;

	//	Section 3C.1's shared bounded-retry ceiling, the same one every other bounded local-
	//	repair loop in this chain already answers to.
	int	MaxRepairAttemptsPerField;

	//	The same fairness-estimate reference speed BaseSelectionPlanner already uses and
	//	documents -- one Harvester route time needs the same undefined-physical-quantity
	//	treatment a base-to-base one does, not a second invented number.
	float	ReferenceVehicleSpeed;

	//	The same shared cap every terrain-reading generator in this chain already uses
	//	(profile.Terrain.MaxBuildableSlopeDegrees, in radians).
	float	MaxBuildableSlopeRadians;
};


class TiberiumFieldGenerator
{
public:

	static void			Init(void);
	static void			Shutdown(void);

	/*
	**	Requires StrategicPlanner::Has_Plan(), RoadGraphGenerator::Has_Graph() and
	**	BaseSelectionPlanner::Has_Selection() -- step 1's own "initial per-team field" needs a
	**	selected base to be initial *for*.  Returns true with zero fields when the plan has no
	**	RESOURCE_CANDIDATE nodes at all, a correct outcome for a small or dry world.
	*/
	static bool			Generate(const TiberiumFieldParams & params,StringClass * error_out);

	static void			Clear(void);
	static bool			Has_Fields(void)			{ return Built; }

	static int			Get_Field_Count(void)	{ return Fields.Count(); }
	static bool			Get_Field(int index,ResolvedTiberiumField * out);

	static int			Get_Crystal_Count(void)	{ return Crystals.Count(); }
	static bool			Get_Crystal(int index,TiberiumCrystalPlacement * out);

	static int			Get_Repair_Log_Count(void)	{ return RepairLog.Count(); }
	static bool			Get_Repair_Log_Entry(int index,ResourceRepairLogEntry * out);

	static unsigned int	Get_Checksum(void)		{ return Checksum; }

	static int			Check_No_Illegal_Footprint(StringClass * report);
	static int			Check_Both_Factions_Have_Early_Field(StringClass * report);
	static int			Check_Harvestable_Fields_Have_Routes(StringClass * report);

private:

	//	Shared body for both initial-field resolution and every contested candidate: legalizes
	//	the footprint (shrinking it and logging the repair when needed), draws the yield, scatters
	//	crystals, resolves the Harvester route from `owning_base_node` when one is given, and
	//	stamps the Tiberium mask -- a no-op, not a failure, when the footprint is illegal at every
	//	radius this profile permits (the candidate is simply dropped, not fabricated).
	static void			Resolve_One_Field(int node_index,int owning_faction,int owning_base_node,
													  const TiberiumFieldParams & params,
													  bool has_gdi_site,const Vector3 & gdi_pos,float gdi_protect_radius,
													  bool has_nod_site,const Vector3 & nod_pos,float nod_protect_radius,
													  int * crystal_index_io);

	static bool			Built;
	static unsigned int	Checksum;

	static DynamicVectorClass<ResolvedTiberiumField>		Fields;
	static DynamicVectorClass<TiberiumCrystalPlacement>	Crystals;
	static DynamicVectorClass<ResourceRepairLogEntry>	RepairLog;
};

#endif	// TIBERIUMFIELDGENERATOR_H
