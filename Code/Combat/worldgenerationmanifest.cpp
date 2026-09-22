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
 *                     $Archive:: /Commando/Code/Combat/worldgenerationmanifest.cpp           $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "worldgenerationmanifest.h"

#include "crc.h"
#include "worldseedservice.h"

#include "nlohmann/json.hpp"

using nlohmann::json;


WorldGenerationManifest::WorldGenerationManifest(void) :
	GeneratorVersion (0),
	Seed (0),
	ProfileHash (0),
	ContentManifestHash (0),
	CliffRingChecksum (0),
	TerrainChecksum (0),
	InteriorCliffChecksum (0),
	HydrologyChecksum (0),
	StrategicPlanChecksum (0),
	RoadGraphChecksum (0),
	BridgePlanChecksum (0),
	NavigationRevision (0)
{
}


/***********************************************************************************************
 * Format_Canonical_Profile_String -- Section 3B.4 quantization, one fixed layout                *
 *                                                                                             *
 *	Every field in declaration order, floats at a fixed 4-decimal precision, booleans as 0/1 --   *
 *	deterministic on every platform (no locale-dependent formatting, `StringClass::Format` is      *
 *	the same `sprintf`-family call this codebase already uses for player-facing text elsewhere,    *
 *	e.g. `commandersidebarrenderer.cpp`'s own `LOW POWER` line).  Never shown to a player; this     *
 *	is a hash input only.                                                                          *
 *=============================================================================================*/
static StringClass
Format_Canonical_Profile_String(const ProceduralWorldProfile & p)
{
	StringClass s (256u,true);
	s.Format (
		"schema=%u gen=%u seed=%u "
		"w.width=%.4f w.height=%.4f w.chunk=%.4f w.sea=%.4f w.fit=%d w.cx=%.4f w.cy=%.4f "
		"b.shape=%s b.band=%.4f b.cmin=%.4f b.cmax=%.4f b.hard=%.4f b.ceil=%.4f "
		"b.rimrough=%.4f b.facerough=%.4f b.toe=%.4f b.colsimp=%d "
		"t.relief=%.4f t.hill=%.4f t.mtn=%.4f t.icliff=%.4f t.valley=%.4f "
		"t.minelev=%.4f t.maxelev=%.4f t.slope=%.4f t.scale=%.4f t.ridge=%.4f t.plateau=%.4f t.approach=%.4f "
		"wa.riv=%d wa.lake=%d wa.pond=%d wa.coast=%d wa.rwidth=%.4f wa.swidth=%.4f wa.raccum=%.4f wa.rcount=%d "
		"rs.rmin=%.4f rs.rmax=%.4f rs.ymin=%.4f rs.ymax=%.4f rs.density=%.4f rs.anchors=%d rs.load=%.4f "
			"rs.grow=%d rs.growrate=%.4f "
		"f.forest=%d f.settle=%d f.tunnel=%d f.hwy=%d f.tib=%d "
		"r.local=%d r.alt=%d "
		"fa.resdiff=%.4f fa.incdiff=%.4f fa.frontdiff=%.4f fa.b2bdiff=%.4f "
		"fa.minveh=%d fa.mininf=%d fa.minexp=%.4f fa.bridgeok=%d "
		"fa.rundist=%.4f fa.runtime=%.4f fa.cover=%.4f fa.laneclear=%.4f",
		p.SchemaVersion,p.GeneratorVersion,p.Seed,
		p.World.Width,p.World.Height,p.World.ChunkSize,p.World.SeaLevel,p.World.FitToLevel ? 1 : 0,p.World.CenterX,p.World.CenterY,
		p.Boundary.Shape.Peek_Buffer (),p.Boundary.CliffBandWidth,p.Boundary.CliffMinHeight,
			p.Boundary.CliffMaxHeight,p.Boundary.HardBarrierOffset,p.Boundary.AirCeiling,
		p.Boundary.RimRoughness,p.Boundary.FaceRoughness,p.Boundary.ToeWidth,
			p.Boundary.CollisionSimplification,
		p.Terrain.MacroRelief,p.Terrain.HillDensity,p.Terrain.MountainDensity,
			p.Terrain.InteriorCliffDensity,p.Terrain.ValleyDensity,
		p.Terrain.MinElevation,p.Terrain.MaxElevation,p.Terrain.MaxBuildableSlopeDegrees,p.Terrain.FeatureScale,
			p.Terrain.RidgeSharpness,p.Terrain.PlateauChance,p.Terrain.BoundaryApproachFlattening,
		p.Water.Rivers ? 1 : 0,p.Water.Lakes ? 1 : 0,p.Water.Ponds ? 1 : 0,p.Water.Coastline ? 1 : 0,
			p.Water.RiverWidth,p.Water.StreamWidth,p.Water.RiverAccumulationChunks,p.Water.MaxRiverCount,
		p.Resource.FieldRadiusMin,p.Resource.FieldRadiusMax,p.Resource.InitialYieldMin,p.Resource.InitialYieldMax,
			p.Resource.CrystalDensity,p.Resource.HarvestAnchorCount,p.Resource.HarvesterLoadAmount,
			p.Resource.GrowthEnabled ? 1 : 0,p.Resource.GrowthRatePerSecond,
		p.Features.Forests ? 1 : 0,p.Features.Settlements ? 1 : 0,p.Features.Tunnels ? 1 : 0,
			p.Features.Highways ? 1 : 0,p.Features.TiberiumFields ? 1 : 0,
		p.Retry.MaxLocalRetriesPerStage,p.Retry.MaxAlternateCandidatesPerFeature,
		p.Fairness.MaxInitialResourceTravelTimeDifferenceSeconds,p.Fairness.MaxProjectedInitialIncomeDifferencePercent,
			p.Fairness.MaxBaseToFrontTravelTimeDifferenceSeconds,p.Fairness.MaxBaseToBaseTravelTimeDifferenceSeconds,
		p.Fairness.MinVehicleApproachCount,p.Fairness.MinInfantryApproachCount,
			p.Fairness.MinBaseExpansionArea,p.Fairness.AllowSingleBridgeDependency ? 1 : 0,
		p.Fairness.MaxMandatoryExposedInfantryRunDistance,p.Fairness.MaxMandatoryExposedInfantryRunTimeSeconds,
			p.Fairness.PreferredCoverInterval,p.Fairness.MinVehicleLaneClearance
	);
	return s;
}


unsigned int WorldGenerationManifestService::Compute_Profile_Hash(const ProceduralWorldProfile & profile)
{
	StringClass canonical = Format_Canonical_Profile_String (profile);
	return CRC::String (canonical.Peek_Buffer ());
}


/***********************************************************************************************
 * Sort_Stable -- Section 3B.2: never hash caller-supplied order                                 *
 *=============================================================================================*/
static void
Sort_Stable(DynamicVectorClass<StringClass> & ids)
{
	//	Insertion sort: content-manifest identifier lists are short in every phase this
	//	contract exists for, same tradeoff buildingdescriptormanager.cpp's own damage-state
	//	sort already makes for the same reason.
	for (int i = 1; i < ids.Count (); i++) {
		StringClass key = ids [i];
		int j = i - 1;
		while ((j >= 0) && (::strcmp (ids [j].Peek_Buffer (),key.Peek_Buffer ()) > 0)) {
			ids [j + 1] = ids [j];
			j--;
		}
		ids [j + 1] = key;
	}
}


unsigned int WorldGenerationManifestService::Compute_Content_Manifest_Hash(const DynamicVectorClass<StringClass> & content_identifiers)
{
	DynamicVectorClass<StringClass> sorted = content_identifiers;
	Sort_Stable (sorted);

	CRCEngine crc;
	for (int i = 0; i < sorted.Count (); i++) {
		crc (sorted [i].Peek_Buffer (),sorted [i].Get_Length ());
		crc ('\n');		// separator, so {"ab","c"} and {"a","bc"} never collide
	}
	return (unsigned int)crc;
}


WorldGenerationManifest WorldGenerationManifestService::Create_Empty_World_Manifest(
	const ProceduralWorldProfile & profile,const DynamicVectorClass<StringClass> & content_identifiers)
{
	WorldGenerationManifest m;
	m.GeneratorVersion		= profile.GeneratorVersion;
	m.Seed						= WorldSeedService::Normalize_Seed (profile.Seed);
	m.ProfileHash				= Compute_Profile_Hash (profile);
	m.ContentManifestHash	= Compute_Content_Manifest_Hash (content_identifiers);
	//	Every consequential-decision list, RoadGraphChecksum and NavigationRevision stay at
	//	their zero/empty default -- an "empty world" has made no generation decisions yet.
	return m;
}


static void
Write_String_Array(json & out,const char * key,const DynamicVectorClass<StringClass> & ids)
{
	json array = json::array ();
	for (int i = 0; i < ids.Count (); i++) {
		array.push_back (std::string (ids [i].Peek_Buffer ()));
	}
	out [key] = array;
}


static void
Read_String_Array(const json & root,const char * key,DynamicVectorClass<StringClass> * out)
{
	out->Delete_All ();
	if (!root.contains (key) || !root [key].is_array ()) return;
	for (auto & element : root [key]) {
		if (!element.is_string ()) continue;
		out->Add (StringClass (element.get<std::string> ().c_str ()));
	}
}


bool WorldGenerationManifestService::Serialize(const WorldGenerationManifest & m,StringClass * out_json)
{
	if (out_json == nullptr) return false;

	json root;
	root ["generator_version"]			= m.GeneratorVersion;
	root ["seed"]								= m.Seed;
	root ["profile_hash"]					= m.ProfileHash;
	root ["content_manifest_hash"]		= m.ContentManifestHash;
	root ["cliff_ring_checksum"]			= m.CliffRingChecksum;
	root ["terrain_checksum"]				= m.TerrainChecksum;
	root ["interior_cliff_checksum"]		= m.InteriorCliffChecksum;
	root ["hydrology_checksum"]			= m.HydrologyChecksum;
	root ["strategic_plan_checksum"]	= m.StrategicPlanChecksum;
	root ["road_graph_checksum"]			= m.RoadGraphChecksum;
	root ["bridge_plan_checksum"]		= m.BridgePlanChecksum;
	root ["navigation_revision"]			= m.NavigationRevision;

	Write_String_Array (root,"selected_base_candidate_ids",m.SelectedBaseCandidateIds);
	Write_String_Array (root,"selected_resource_field_ids",m.SelectedResourceFieldIds);
	Write_String_Array (root,"selected_road_graph_edge_ids",m.SelectedRoadGraphEdgeIds);
	Write_String_Array (root,"selected_bridge_candidate_ids",m.SelectedBridgeCandidateIds);
	Write_String_Array (root,"selected_tunnel_candidate_ids",m.SelectedTunnelCandidateIds);
	Write_String_Array (root,"settlement_layout_ids",m.SettlementLayoutIds);
	Write_String_Array (root,"important_poi_ids",m.ImportantPoiIds);
	Write_String_Array (root,"local_repair_operation_ids",m.LocalRepairOperationIds);
	Write_String_Array (root,"initial_base_layout_ids",m.InitialBaseLayoutIds);

	*out_json = root.dump ().c_str ();
	return true;
}


bool WorldGenerationManifestService::Deserialize(const char * json_text,WorldGenerationManifest * out_manifest,
																	StringClass * error_out)
{
	if ((json_text == nullptr) || (out_manifest == nullptr)) {
		if (error_out != nullptr) *error_out = "null argument";
		return false;
	}

	try {
		json root = json::parse (json_text);

		WorldGenerationManifest m;
		m.GeneratorVersion		= root.value ("generator_version",0u);
		m.Seed						= root.value ("seed",0u);
		m.ProfileHash				= root.value ("profile_hash",0u);
		m.ContentManifestHash	= root.value ("content_manifest_hash",0u);
		m.CliffRingChecksum		= root.value ("cliff_ring_checksum",0u);
		m.TerrainChecksum			= root.value ("terrain_checksum",0u);
		m.InteriorCliffChecksum	= root.value ("interior_cliff_checksum",0u);
		m.HydrologyChecksum		= root.value ("hydrology_checksum",0u);
		m.StrategicPlanChecksum	= root.value ("strategic_plan_checksum",0u);
		m.RoadGraphChecksum		= root.value ("road_graph_checksum",0u);
		m.BridgePlanChecksum		= root.value ("bridge_plan_checksum",0u);
		m.NavigationRevision		= root.value ("navigation_revision",0u);

		Read_String_Array (root,"selected_base_candidate_ids",&m.SelectedBaseCandidateIds);
		Read_String_Array (root,"selected_resource_field_ids",&m.SelectedResourceFieldIds);
		Read_String_Array (root,"selected_road_graph_edge_ids",&m.SelectedRoadGraphEdgeIds);
		Read_String_Array (root,"selected_bridge_candidate_ids",&m.SelectedBridgeCandidateIds);
		Read_String_Array (root,"selected_tunnel_candidate_ids",&m.SelectedTunnelCandidateIds);
		Read_String_Array (root,"settlement_layout_ids",&m.SettlementLayoutIds);
		Read_String_Array (root,"important_poi_ids",&m.ImportantPoiIds);
		Read_String_Array (root,"local_repair_operation_ids",&m.LocalRepairOperationIds);
		Read_String_Array (root,"initial_base_layout_ids",&m.InitialBaseLayoutIds);

		*out_manifest = m;

	} catch (json::exception & e) {
		if (error_out != nullptr) *error_out = e.what ();
		return false;
	}

	return true;
}


StringClass WorldGenerationManifestService::Format_Reproduction_String(const WorldGenerationManifest & m)
{
	StringClass s (128u,true);
	s.Format ("generator_version=%u seed=%u profile=%u content_manifest=%u",
				 m.GeneratorVersion,m.Seed,m.ProfileHash,m.ContentManifestHash);
	return s;
}
