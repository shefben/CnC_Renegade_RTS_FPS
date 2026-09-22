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
 *                     $Archive:: /Commando/Code/Combat/settlementgenerator.h                 $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	V2 roadmap Section 34 Phase 13 -- settlements, abandoned buildings, industrial areas, and     *
 *	strategic POIs.  Section 14, Section 28.                                                     *
 *                                                                                             *
 *	Resolve, not discover.  `StrategicPlanner` (Phase 7) already placed every                      *
 *	`SETTLEMENT_CANDIDATE`/`POI_CANDIDATE`; `BaseSelectionPlanner` (Phase 11) and                     *
 *	`TiberiumFieldGenerator` (Phase 12) already reserved the ground this generator must not          *
 *	conflict with; `ProceduralAssetRegistry` (Section 28A) already names which real, offline-           *
 *	verified assets exist to place.  This file's own job is step 1 onward: reject conflicts,             *
 *	classify a layout, connect to the road network, and place real instances from the registry.           *
 *                                                                                             *
 *	What this generator does NOT do.  Instantiate a live `StaticPhysClass`/collision object, or         *
 *	wire nav -- the same "data, not a spawned object" split `TiberiumFieldGenerator`'s own class            *
 *	comment already draws for a Harvester.  It also does not lay a new physical connector road            *
 *	through `RoadSystem::Add_Road` -- `RoadConnected` is a real distance classification against               *
 *	the already-built network, not a new one; see docs/procedural_world/Settlements.md for why.             *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef SETTLEMENTGENERATOR_H
#define SETTLEMENTGENERATOR_H

#ifndef ALWAYS_H
#include "always.h"
#endif

#include "vector.h"
#include "vector3.h"
#include "wwstring.h"


//	Step 2, named exactly as that step lists it.
enum SettlementLayoutType
{
	SETTLEMENT_LAYOUT_LINEAR_ROADSIDE = 0,
	SETTLEMENT_LAYOUT_SMALL_GRID,
	SETTLEMENT_LAYOUT_INDUSTRIAL_LOOP,
	SETTLEMENT_LAYOUT_IRREGULAR_TOWN,
	SETTLEMENT_LAYOUT_RUINS,
	SETTLEMENT_LAYOUT_OUTPOST,
	SETTLEMENT_LAYOUT_TYPE_COUNT
};

const char *	Settlement_Layout_Type_Name(SettlementLayoutType type);


//	Step 9, named exactly as that step lists it.
enum PoiKindType
{
	POI_KIND_BUNKER = 0,
	POI_KIND_CONVOY_WRECK,
	POI_KIND_RESEARCH_SITE,
	POI_KIND_RELAY,
	POI_KIND_REPAIR_DEPOT,
	POI_KIND_RUINS,
	POI_KIND_TYPE_COUNT
};

const char *	Poi_Kind_Name(PoiKindType kind);


//	One placed instance -- data only, the same "generator produces the record, a later phase
//	instantiates it" split every other resolver in this chain already uses.
class PlacedStructureInstance
{
public:
	PlacedStructureInstance(void);

	bool	operator == (const PlacedStructureInstance & that) const;
	bool	operator != (const PlacedStructureInstance & that) const	{ return !(*this == that); }

	int			SettlementIndex;	// into SettlementGenerator's own settlement list
	StringClass	AssetId;				// a ProceduralAssetRegistry id
	Vector3		Position;
	float			RotationRadians;
};


class ResolvedSettlement
{
public:
	ResolvedSettlement(void);

	bool	operator == (const ResolvedSettlement & that) const;
	bool	operator != (const ResolvedSettlement & that) const	{ return !(*this == that); }

	StringClass					Id;					// the StrategicNode SETTLEMENT_CANDIDATE id this resolves
	SettlementLayoutType	Layout;
	Vector3					Position;
	float							Radius;				// world units -- the reserved region, same pattern as Phase 11/12

	bool							RoadConnected;		// step 3 -- within RoadConnectorMaxDistance of a real, already-built road
	float							RoadDistance;		// world units to the nearest real road point; -1 when none exists at all

	int							StructureCount;	// how many PlacedStructureInstance entries belong to this settlement
};


class ResolvedPoi
{
public:
	ResolvedPoi(void);

	bool	operator == (const ResolvedPoi & that) const;
	bool	operator != (const ResolvedPoi & that) const	{ return !(*this == that); }

	StringClass		Id;			// the StrategicNode POI_CANDIDATE id this resolves
	PoiKindType		Kind;
	Vector3			Position;

	//	Always false today -- ProceduralAssetRegistry carries no ASSET_CATEGORY_POI entry yet
	//	(no real asset exists for any of the six kinds above).  Recognised, never fabricated.
	bool				HasRealAsset;
};


struct SettlementParams
{
	SettlementParams(void);

	unsigned int	Seed;

	//	How close a settlement must land to an already-built road to count as connected (step
	//	3).  Beyond this, RoadConnected is honestly false rather than a connector this generator
	//	does not build (see the header's own comment on why).
	float	RoadConnectorMaxDistance;

	int	MaxStructuresPerSettlement;	// >= 0 -- step 4's own bound, this generator's ceiling

	//	The same shared cap every terrain-reading generator in this chain already uses.
	float	MaxBuildableSlopeRadians;
};


class SettlementGenerator
{
public:

	static void			Init(void);
	static void			Shutdown(void);

	/*
	**	Requires StrategicPlanner::Has_Plan(), RoadGraphGenerator::Has_Graph() and
	**	ProceduralAssetRegistry::Has_Registry().  Returns true with zero settlements when the
	**	plan has no SETTLEMENT_CANDIDATE/POI_CANDIDATE nodes, a correct outcome for a small world.
	*/
	static bool			Generate(const SettlementParams & params,StringClass * error_out);

	static void			Clear(void);
	static bool			Has_Settlements(void)	{ return Built; }

	static int			Get_Settlement_Count(void)	{ return Settlements.Count(); }
	static bool			Get_Settlement(int index,ResolvedSettlement * out);

	static int			Get_Structure_Count(void)	{ return Structures.Count(); }
	static bool			Get_Structure(int index,PlacedStructureInstance * out);

	static int			Get_Poi_Count(void)	{ return Pois.Count(); }
	static bool			Get_Poi(int index,ResolvedPoi * out);

	static unsigned int	Get_Checksum(void)		{ return Checksum; }

	static int			Check_No_Illegal_Placement(StringClass * report);
	static int			Check_No_Structure_Overlap(StringClass * report);

private:

	static bool			Built;
	static unsigned int	Checksum;

	static DynamicVectorClass<ResolvedSettlement>			Settlements;
	static DynamicVectorClass<PlacedStructureInstance>	Structures;
	static DynamicVectorClass<ResolvedPoi>					Pois;
};

#endif	// SETTLEMENTGENERATOR_H
