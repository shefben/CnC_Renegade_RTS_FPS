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
 *                     $Archive:: /Commando/Code/Combat/worldgenerationmanifest.h             $*
 *                                                                                             *
 *	V2 roadmap Section 34 Phase 1 -- `WorldGenerationManifest` (Section 4's "Generation           *
 *	manifest", Section 29A.3's "authoritative generation decision manifest").                     *
 *                                                                                             *
 *	A persisted world's identity is generator version + seed + profile hash + content-manifest    *
 *	hash (Section 3B.6); this struct carries exactly that plus the consequential-decision          *
 *	fields Section 29A.3 lists, so a client reconstructing a world never has to independently       *
 *	reinterpret an ambiguous generator branch.  Every list field is empty in this phase's own       *
 *	`Create_Empty_World_Manifest` output -- Phase 2 onward populates the boundary, Phase 7 the       *
 *	base/resource candidates, and so on.  The fields exist now so later phases extend this          *
 *	manifest rather than each inventing a parallel one.                                             *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef WORLDGENERATIONMANIFEST_H
#define WORLDGENERATIONMANIFEST_H

#include "always.h"
#include "vector.h"
#include "wwstring.h"

#include "proceduralworldprofile.h"


struct WorldGenerationManifest
{
	WorldGenerationManifest(void);

	unsigned int	GeneratorVersion;
	unsigned int	Seed;						// normalized (WorldSeedService::Normalize_Seed), never the raw authored value
	unsigned int	ProfileHash;
	unsigned int	ContentManifestHash;

	//	Section 29A.3's consequential-decision lists.  Stable IDs only (Section 3B.5), never
	//	pointers/indices -- every list here is safe to serialize, diff, and network-replicate
	//	as-is.
	DynamicVectorClass<StringClass>	SelectedBaseCandidateIds;
	DynamicVectorClass<StringClass>	SelectedResourceFieldIds;
	DynamicVectorClass<StringClass>	SelectedRoadGraphEdgeIds;
	DynamicVectorClass<StringClass>	SelectedBridgeCandidateIds;
	DynamicVectorClass<StringClass>	SelectedTunnelCandidateIds;
	DynamicVectorClass<StringClass>	SettlementLayoutIds;
	DynamicVectorClass<StringClass>	ImportantPoiIds;
	DynamicVectorClass<StringClass>	LocalRepairOperationIds;
	DynamicVectorClass<StringClass>	InitialBaseLayoutIds;

	//	Phase 3 step 17.  A checksum over the generated boundary cliff ring's own canonical
	//	geometry, so two runs that claim the same seed/profile can be shown to have produced
	//	the same ring rather than merely asserted to have.  0 until a cliff ring is built.
	unsigned int	CliffRingChecksum;

	//	Phase 4 step 17.  WorldTerrainSystem::Get_World_Chunk_Checksum -- a CRC over every
	//	generated chunk's own checksum, in row-major order.  0 either until macro terrain is
	//	built, or permanently for a `fit_to_level` world, where the host level's own ground
	//	stands and Phase 4's generator never runs (see ProceduralWorldManager::Build_Macro_Terrain).
	unsigned int	TerrainChecksum;

	//	Phase 5.  InteriorCliffGenerator::Get_Checksum -- a CRC over every generated interior
	//	cliff feature's own type, traversability, length, tallest station and bypass point.  0
	//	until interior cliffs are built, same three reasons TerrainChecksum can be 0: no macro
	//	terrain to read candidates from (`fit_to_level`), macro terrain failed, or interior cliff
	//	generation itself failed.
	unsigned int	InteriorCliffChecksum;

	//	Phase 6.  HydrologyGenerator::Get_Checksum -- a CRC over every generated river/stream/
	//	lake/pond's own type, shore classification, enclosed flag, length and crossing
	//	candidates.  0 under the same conditions InteriorCliffChecksum can be 0 for: no chunk
	//	registry to read (`fit_to_level`), macro terrain failed, or hydrology generation itself
	//	failed.
	unsigned int	HydrologyChecksum;

	//	Phase 7.  StrategicPlanner::Get_Checksum -- a CRC over every candidate node's own id,
	//	type, faction, position and suitability.  0 under the same conditions HydrologyChecksum
	//	can be 0 for.  Deliberately not what SelectedBaseCandidateIds and its four siblings mean
	//	below -- those stay empty until a later phase actually selects from this plan; nothing
	//	here has selected anything (Phase 7 step 12).
	unsigned int	StrategicPlanChecksum;

	unsigned int	RoadGraphChecksum;		// 0 until Phase 8 exists

	//	Phase 9.  BridgePlanner::Get_Checksum -- a CRC over every resolved bridge's own initial
	//	state, connectivity flag and strategic importance.  0 under the same conditions
	//	RoadGraphChecksum can be 0 for.
	unsigned int	BridgePlanChecksum;
	unsigned int	NavigationRevision;		// 0 until Phase 16 exists
};


class WorldGenerationManifestService
{
	public:

		/*
		**	Section 3B.4 quantization: every float field is formatted to a fixed 4-decimal
		**	representation before hashing (Format_Canonical_Profile_String, worldgenerationmanifest.cpp),
		**	so a floating-point value that differs only in noise below 1e-4 does not change the
		**	hash.  Two profiles equal in every field, byte-for-byte after that quantization,
		**	always hash equal; any documented field difference always hashes different.
		*/
		static unsigned int	Compute_Profile_Hash(const ProceduralWorldProfile & profile);

		/*
		**	Section 29A.1's content-manifest hash.  `content_identifiers` names every asset/data
		**	source capable of changing authoritative generation, geometry, collision, or
		**	navigation -- W3D/collision content, procedural asset descriptors, bridge/tunnel
		**	module definitions, road cross-section definitions, traversal profiles, and so on, as
		**	those registries come to exist in later phases.  Empty today: no generator/asset
		**	registry exists yet (Phase 0's own baseline search confirmed this), so an empty list
		**	is the honest current content manifest, not a placeholder pretending one exists.
		**	Order-independent -- the identifiers are sorted before hashing (Section 3B.2: never
		**	depend on caller-supplied order for an authoritative result).
		*/
		static unsigned int	Compute_Content_Manifest_Hash(const DynamicVectorClass<StringClass> & content_identifiers);

		static WorldGenerationManifest	Create_Empty_World_Manifest(const ProceduralWorldProfile & profile,
																						const DynamicVectorClass<StringClass> & content_identifiers);

		static bool		Serialize(const WorldGenerationManifest & manifest,StringClass * out_json);
		static bool		Deserialize(const char * json_text,WorldGenerationManifest * out_manifest,
											 StringClass * error_out);

		//	Phase 1 step 12's exact format: "generator_version=<n> seed=<n> profile=<hash>
		//	content_manifest=<hash>".
		static StringClass	Format_Reproduction_String(const WorldGenerationManifest & manifest);
};

#endif // WORLDGENERATIONMANIFEST_H
