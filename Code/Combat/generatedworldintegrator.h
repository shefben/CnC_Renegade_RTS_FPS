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
 *                     $Archive:: /Commando/Code/Combat/generatedworldintegrator.h            $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	V2 roadmap Section 34 Phase 16 step 1-2 -- final generated-object registration lifecycle.    *
 *                                                                                             *
 *	`SettlementGenerator`'s own header (settlementgenerator.h) says outright what it does not     *
 *	do: "Instantiate a live StaticPhysClass/collision object, or wire nav."  Every other          *
 *	generated-geometry owner in this chain already closes that loop itself -- RoadSystem          *
 *	(roadsystem.cpp), EscarpmentSystem (escarpment.cpp), BridgeSystem (bridgesystem.cpp) and       *
 *	FoliageSystem (foliagesystem.cpp) each already call PhysicsSceneClass::Add_Static_Object      *
 *	the moment they place real geometry.  Settlement structures are the one generated-object       *
 *	type in the whole Phase 1-15 chain that reached this point still data-only, so this file's      *
 *	one job is closing exactly that gap, the same way `BuildingGameObj::Build_Runtime_Components`    *
 *	(building.cpp) already wraps a resolved model in a StaticPhysClass for a Commander-built          *
 *	building -- same pattern, reused rather than reinvented.                                          *
 *                                                                                             *
 *	Persistence identity.  `PlacedStructureInstance` carries no stable id of its own (only an          *
 *	array index into `SettlementGenerator`'s internal list, which is not stable across a re-run          *
 *	with different settlement counts).  This file assigns one real one per structure --                  *
 *	`<settlement StrategicNode id>:structure:<global structure index>` -- built from the same               *
 *	StrategicPlanner-owned id string every other Phase 11-15 generator already keys its own                 *
 *	records by.  Nothing consumes it yet; Phase 18/19's own journal and snapshot format are the                *
 *	first real consumers, the same "real output, no consumer yet" honesty Phase 15's                            *
 *	CombatSpaceValidator repair-operation names already carry.                                                   *
 *                                                                                             *
 *	What this file does NOT do.  Give Tiberium crystals or POIs real geometry -- neither has a         *
 *	real, offline-verified asset in `ProceduralAssetRegistry` today (see that file's own header),      *
 *	so there is nothing to load without guessing a model name, and this file refuses to guess           *
 *	one.  Nor does it retrofit a persistence id onto roads, bridges, bases or resource fields --          *
 *	every one of those already carries a real, stable StringClass id from the StrategicPlanner/            *
 *	RoadGraphGenerator/BridgePlanner that produced it.                                                       *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef GENERATEDWORLDINTEGRATOR_H
#define GENERATEDWORLDINTEGRATOR_H

#ifndef ALWAYS_H
#include "always.h"
#endif

#include "vector.h"
#include "wwstring.h"

class StaticPhysClass;


//	One structure's own integration record.  HasCollision is false, honestly, whenever the
//	structure's asset resolves to a descriptor with no real model to load -- the same
//	missing-model discipline BuildingGameObj::Build_Runtime_Components already uses; it is
//	never a fabricated collision object.
class IntegratedStructureRecord
{
public:
	IntegratedStructureRecord(void);

	bool	operator == (const IntegratedStructureRecord & that) const;
	bool	operator != (const IntegratedStructureRecord & that) const	{ return !(*this == that); }

	StringClass		PersistenceId;
	int				StructureIndex;	// SettlementGenerator::Get_Structure's own global index
	bool				HasCollision;
};


class GeneratedWorldIntegrator
{
public:

	static void		Init(void);
	static void		Shutdown(void);

	/*
	**	Requires SettlementGenerator::Has_Settlements() and ProceduralAssetRegistry::Has_Registry().
	**	Safe (returns true, zero records) when SettlementGenerator has no structures. Registers
	**	real collision for every structure whose ProceduralAssetDescriptor::DescriptorName
	**	resolves through BuildingDescriptorManager to a loadable model.
	*/
	static bool		Build(StringClass * error_out);

	/*
	**	Step 13 -- "integrate v1 shroud/radar over generated world coordinates."
	**
	**	WorldShroudManager is already entirely world-coordinate based (Get_Shroud_State,
	**	Is_Radar_Detected and every sensor source all take a Vector3), so no coordinate
	**	translation is needed or wanted.  What it does need is to be told where the world is:
	**	its per-team grid only exists once Re_Partition has been called, and the one existing
	**	caller of that -- PhysicsSceneClass::Re_Partition_Dynamic_Culling_System -- is documented
	**	editor-only and additionally resets vis, so a generated world cannot use it.  This sizes
	**	the shroud grid from the chunk registry's own real patch bounds instead, the same bounds
	**	NavMeshGenerator and MinimapBaseLayer already read, so a shroud cell and a world position
	**	line up for a generated world exactly as they do for a hand-authored one.
	**
	**	Requires WorldTerrainSystem::Has_Chunk_Registry().
	*/
	static bool		Partition_Shroud_Over_World(StringClass * error_out);

	static void		Clear(void);
	static bool		Has_Integrated(void)	{ return Built; }

	static int		Get_Record_Count(void)	{ return Records.Count(); }
	static bool		Get_Record(int index,IntegratedStructureRecord * out);

	static int		Get_Collision_Count(void);			// Records with HasCollision == true
	static int		Get_Missing_Model_Count(void);		// Records with HasCollision == false

	static unsigned int	Get_Checksum(void)	{ return Checksum; }

private:

	static bool			Built;
	static unsigned int	Checksum;

	static DynamicVectorClass<IntegratedStructureRecord>	Records;
	static DynamicVectorClass<StaticPhysClass *>				PhysObjects;
};

#endif	// GENERATEDWORLDINTEGRATOR_H
