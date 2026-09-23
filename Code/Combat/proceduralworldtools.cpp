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
 *                     $Archive:: /Commando/Code/Combat/proceduralworldtools.cpp              $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "proceduralworldtools.h"

#include "aabox.h"
#include "combatspacevalidator.h"
#include "transportationnetworkvalidator.h"
#include "proceduralworldmanager.h"
#include "worldterrainsystem.h"
#include "watersystem.h"
#include "strategicplanner.h"
#include "roadgraphgenerator.h"
#include "bridgeplanner.h"
#include "baseselectionplanner.h"
#include "tiberiumfieldgenerator.h"
#include "settlementgenerator.h"
#include "playertype.h"
#include "rawfile.h"

#include <cstdio>

#include "nlohmann/json.hpp"

using nlohmann::json;


ProceduralWorldValidationReport::ProceduralWorldValidationReport(void) :
	GeneratorVersion (0),
	Seed (0),
	ProfileHash (0),
	ContentManifestHash (0),
	CombatSpacePassed (false),
	CombatSpaceFailureCount (0),
	SightlineSampleCount (0),
	SightlineP50 (0.0f),
	SightlineP90 (0.0f),
	SightlineMax (0.0f),
	BaseToBaseTravelTimeSeconds (-1.0f),
	SingleBridgeDependency (false),
	OwnedFieldsWithoutRoute (0),
	SettlementsWithoutRoadConnection (0),
	TransportPassed (false),
	UnboundAbutmentCount (0),
	BridgeNoRoadCount (0),
	BridgeNarrowerCount (0),
	DuplicateRoadCount (0),
	ExcessiveGradeCount (0),
	InsufficientClearanceCount (0),
	RequiredBaseRoutesChecked (false),
	RequiredBaseRoutesConnected (false),
	UnreachableCellCount (0)
{
}


ProceduralWorldSeedBatchResult::ProceduralWorldSeedBatchResult(void) :
	Seed (0),
	GenerateSucceeded (false),
	CombatSpacePassed (false),
	TransportPassed (false)
{
}


/***********************************************************************************************
 * Build_Validation_Report -- Section 27.3.  Read-only: both validators' own cached static state *
 *	                          from their last Validate() call, plus the active manifest.  Does    *
 *	                          not call Validate() itself -- whoever generated/validated the        *
 *	                          world already did, and re-running it here would silently ask a        *
 *	                          different question than "what did the last real pass find".           *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
ProceduralWorldValidationReport ProceduralWorldToolsService::Build_Validation_Report(void)
{
	ProceduralWorldValidationReport report;

	const WorldGenerationManifest & manifest = ProceduralWorldManager::Get_Manifest ();
	report.GeneratorVersion		= manifest.GeneratorVersion;
	report.Seed							= manifest.Seed;
	report.ProfileHash				= manifest.ProfileHash;
	report.ContentManifestHash	= manifest.ContentManifestHash;

	report.CombatSpacePassed			= CombatSpaceValidator::Passed ();
	report.CombatSpaceFailureCount	= CombatSpaceValidator::Get_Failure_Count ();
	const CombatSpaceMetrics & metrics = CombatSpaceValidator::Get_Metrics ();
	report.SightlineSampleCount					= metrics.SightlineSampleCount;
	report.SightlineP50								= metrics.SightlineP50;
	report.SightlineP90								= metrics.SightlineP90;
	report.SightlineMax								= metrics.SightlineMax;
	report.BaseToBaseTravelTimeSeconds			= metrics.BaseToBaseTravelTimeSeconds;
	report.SingleBridgeDependency				= metrics.SingleBridgeDependency;
	report.OwnedFieldsWithoutRoute				= metrics.OwnedFieldsWithoutRoute;
	report.SettlementsWithoutRoadConnection	= metrics.SettlementsWithoutRoadConnection;

	report.TransportPassed						= TransportationNetworkValidator::Passed ();
	report.UnboundAbutmentCount				= TransportationNetworkValidator::Get_Unbound_Abutment_Count ();
	report.BridgeNoRoadCount					= TransportationNetworkValidator::Get_Bridge_With_No_Road_Count ();
	report.BridgeNarrowerCount				= TransportationNetworkValidator::Get_Bridge_Narrower_Than_Road_Count ();
	report.DuplicateRoadCount					= TransportationNetworkValidator::Get_Duplicate_Road_Through_Bridge_Count ();
	report.ExcessiveGradeCount				= TransportationNetworkValidator::Get_Excessive_Approach_Grade_Count ();
	report.InsufficientClearanceCount		= TransportationNetworkValidator::Get_Insufficient_Clearance_Count ();
	report.RequiredBaseRoutesChecked		= TransportationNetworkValidator::Did_Check_Required_Base_Routes ();
	report.RequiredBaseRoutesConnected	= TransportationNetworkValidator::Has_Required_Base_Routes ();
	report.UnreachableCellCount				= TransportationNetworkValidator::Get_Unreachable_Passable_Cell_Count ();

	return report;
}


bool ProceduralWorldToolsService::Serialize_Validation_Report(const ProceduralWorldValidationReport & r,StringClass * out_json)
{
	if (out_json == nullptr) return false;

	json root;
	root ["generator_version"]			= r.GeneratorVersion;
	root ["seed"]								= r.Seed;
	root ["profile_hash"]					= r.ProfileHash;
	root ["content_manifest_hash"]		= r.ContentManifestHash;

	json combat_space;
	combat_space ["passed"]									= r.CombatSpacePassed;
	combat_space ["failure_count"]							= r.CombatSpaceFailureCount;
	combat_space ["sightline_sample_count"]				= r.SightlineSampleCount;
	combat_space ["sightline_p50"]							= r.SightlineP50;
	combat_space ["sightline_p90"]							= r.SightlineP90;
	combat_space ["sightline_max"]							= r.SightlineMax;
	combat_space ["base_to_base_travel_time_seconds"]	= r.BaseToBaseTravelTimeSeconds;
	combat_space ["single_bridge_dependency"]			= r.SingleBridgeDependency;
	combat_space ["owned_fields_without_route"]			= r.OwnedFieldsWithoutRoute;
	combat_space ["settlements_without_road_connection"]	= r.SettlementsWithoutRoadConnection;
	root ["combat_space"] = combat_space;

	json transport;
	transport ["passed"]									= r.TransportPassed;
	transport ["unbound_abutment_count"]				= r.UnboundAbutmentCount;
	transport ["bridge_no_road_count"]					= r.BridgeNoRoadCount;
	transport ["bridge_narrower_count"]					= r.BridgeNarrowerCount;
	transport ["duplicate_road_count"]					= r.DuplicateRoadCount;
	transport ["excessive_grade_count"]					= r.ExcessiveGradeCount;
	transport ["insufficient_clearance_count"]			= r.InsufficientClearanceCount;
	transport ["required_base_routes_checked"]			= r.RequiredBaseRoutesChecked;
	transport ["required_base_routes_connected"]		= r.RequiredBaseRoutesConnected;
	transport ["unreachable_cell_count"]					= r.UnreachableCellCount;
	root ["transportation_network"] = transport;

	*out_json = root.dump ().c_str ();
	return true;
}


StringClass ProceduralWorldToolsService::Format_Validation_Report_Text(const ProceduralWorldValidationReport & r)
{
	StringClass s (512u,true);
	s.Format (
		"generator_version=%u seed=%u profile=%u content_manifest=%u\n"
		"combat space: %s -- %d hard failure(s), sightline P50=%.1f P90=%.1f max=%.1f, "
		"base-to-base travel %.1fs, single-bridge-dependency=%s, %d field(s) without a route, "
		"%d settlement(s) without a road\n"
		"transportation network: %s -- %d unbound abutment(s), %d bridge(s) with no road, "
		"%d bridge(s) narrower than their road, %d duplicate road(s), %d excessive approach grade(s), "
		"%d insufficient clearance(s), required base routes %s (%s), %d unreachable passable cell(s)\n",
		r.GeneratorVersion,r.Seed,r.ProfileHash,r.ContentManifestHash,
		r.CombatSpacePassed ? "PASS" : "FAIL",r.CombatSpaceFailureCount,
		r.SightlineP50,r.SightlineP90,r.SightlineMax,
		r.BaseToBaseTravelTimeSeconds,r.SingleBridgeDependency ? "yes" : "no",
		r.OwnedFieldsWithoutRoute,r.SettlementsWithoutRoadConnection,
		r.TransportPassed ? "PASS" : "FAIL",
		r.UnboundAbutmentCount,r.BridgeNoRoadCount,r.BridgeNarrowerCount,r.DuplicateRoadCount,
		r.ExcessiveGradeCount,r.InsufficientClearanceCount,
		r.RequiredBaseRoutesChecked ? (r.RequiredBaseRoutesConnected ? "connected" : "DISCONNECTED") : "not checked",
		r.RequiredBaseRoutesChecked ? "checked" : "no traversable graph/base pair",
		r.UnreachableCellCount);
	return s;
}


bool ProceduralWorldToolsService::Write_Validation_Report_To_File(const char * file_path,StringClass * error_out)
{
	if ((file_path == nullptr) || (file_path [0] == '\0')) {
		if (error_out != nullptr) *error_out = "null file path";
		return false;
	}

	ProceduralWorldValidationReport report = Build_Validation_Report ();
	StringClass json_text;
	if (!Serialize_Validation_Report (report,&json_text)) {
		if (error_out != nullptr) *error_out = "serialize failed";
		return false;
	}

	RawFileClass file (file_path);
	if (!file.Open (FileClass::WRITE)) {
		if (error_out != nullptr) error_out->Format ("could not open '%s' for write",file_path);
		return false;
	}
	file.Write ((const char *)json_text,(int)json_text.Get_Length ());
	file.Close ();
	return true;
}


StringClass ProceduralWorldToolsService::Get_Reproduction_String(void)
{
	return WorldGenerationManifestService::Format_Reproduction_String (ProceduralWorldManager::Get_Manifest ());
}


/***********************************************************************************************
 * World_To_Pixel -- pure ground-plane (X,Y) to raster (px,py) mapping.                          *
 *                                                                                             *
 *	Y is flipped (world +Y is "up the map", raster +py is "down the image") so the written PPM    *
 *	reads the right way up in an ordinary image viewer, the same convention every top-down          *
 *	minimap tool uses.  Returns false, writing nothing, for a degenerate (zero-extent) bounds or     *
 *	a point that falls outside it -- callers skip the plot rather than clamp a marker onto an          *
 *	edge it was never actually near.                                                                    *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool ProceduralWorldToolsService::World_To_Pixel(const AABoxClass & bounds,float world_x,float world_y,
																  int width,int height,int * px_out,int * py_out)
{
	if ((px_out == nullptr) || (py_out == nullptr) || (width <= 0) || (height <= 0)) return false;

	float min_x = bounds.Center.X - bounds.Extent.X;
	float max_x = bounds.Center.X + bounds.Extent.X;
	float min_y = bounds.Center.Y - bounds.Extent.Y;
	float max_y = bounds.Center.Y + bounds.Extent.Y;

	float span_x = max_x - min_x;
	float span_y = max_y - min_y;
	if ((span_x <= 0.0f) || (span_y <= 0.0f)) return false;

	if ((world_x < min_x) || (world_x > max_x) || (world_y < min_y) || (world_y > max_y)) return false;

	float u = (world_x - min_x) / span_x;
	float v = (world_y - min_y) / span_y;

	int px = (int)(u * (float)(width - 1));
	int py = (int)((1.0f - v) * (float)(height - 1));

	if ((px < 0) || (px >= width) || (py < 0) || (py >= height)) return false;

	*px_out = px;
	*py_out = py;
	return true;
}


void ProceduralWorldToolsService::Fill_Buffer(unsigned char * rgb,int width,int height,
															  unsigned char r,unsigned char g,unsigned char b)
{
	if ((rgb == nullptr) || (width <= 0) || (height <= 0)) return;
	for (int i = 0; i < (width * height); i++) {
		rgb [(i * 3) + 0] = r;
		rgb [(i * 3) + 1] = g;
		rgb [(i * 3) + 2] = b;
	}
}


void ProceduralWorldToolsService::Plot_Point(unsigned char * rgb,int width,int height,int px,int py,
															 unsigned char r,unsigned char g,unsigned char b)
{
	if ((rgb == nullptr) || (px < 0) || (px >= width) || (py < 0) || (py >= height)) return;
	int i = (py * width) + px;
	rgb [(i * 3) + 0] = r;
	rgb [(i * 3) + 1] = g;
	rgb [(i * 3) + 2] = b;
}


void ProceduralWorldToolsService::Plot_Filled_Square(unsigned char * rgb,int width,int height,int px,int py,int half_size,
																		unsigned char r,unsigned char g,unsigned char b)
{
	for (int dy = -half_size; dy <= half_size; dy++) {
		for (int dx = -half_size; dx <= half_size; dx++) {
			Plot_Point (rgb,width,height,px + dx,py + dy,r,g,b);
		}
	}
}


//	Integer Bresenham, the standard form -- no floating accumulation, so a line between two
//	in-bounds points never drifts off either endpoint regardless of length.
void ProceduralWorldToolsService::Plot_Line(unsigned char * rgb,int width,int height,int px0,int py0,int px1,int py1,
															unsigned char r,unsigned char g,unsigned char b)
{
	int dx = (px1 > px0) ? (px1 - px0) : (px0 - px1);
	int dy = (py1 > py0) ? -(py1 - py0) : -(py0 - py1);
	int sx = (px0 < px1) ? 1 : -1;
	int sy = (py0 < py1) ? 1 : -1;
	int err = dx + dy;

	int x = px0;
	int y = py0;

	for (;;) {
		Plot_Point (rgb,width,height,x,y,r,g,b);
		if ((x == px1) && (y == py1)) break;
		int e2 = 2 * err;
		if (e2 >= dy) { err += dy; x += sx; }
		if (e2 <= dx) { err += dx; y += sy; }
	}
}


bool ProceduralWorldToolsService::Write_PPM_Buffer_To_File(const char * file_path,const unsigned char * rgb,
																				 int width,int height,StringClass * error_out)
{
	if ((file_path == nullptr) || (rgb == nullptr) || (width <= 0) || (height <= 0)) {
		if (error_out != nullptr) *error_out = "invalid argument";
		return false;
	}

	RawFileClass file (file_path);
	if (!file.Open (FileClass::WRITE)) {
		if (error_out != nullptr) error_out->Format ("could not open '%s' for write",file_path);
		return false;
	}

	char header [64];
	int header_len = ::sprintf (header,"P6\n%d %d\n255\n",width,height);
	file.Write (header,header_len);
	file.Write ((const void *)rgb,width * height * 3);
	file.Close ();
	return true;
}


/***********************************************************************************************
 * Write_Top_Down_Preview -- Section 27.1/27.2/27.4's real walk against the live generated world. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool ProceduralWorldToolsService::Write_Top_Down_Preview(const char * file_path,unsigned int layer_mask,int resolution,
																			  StringClass * error_out)
{
	if ((file_path == nullptr) || (file_path [0] == '\0')) {
		if (error_out != nullptr) *error_out = "null file path";
		return false;
	}
	if (!WorldTerrainSystem::Has_Chunk_Registry ()) {
		if (error_out != nullptr) *error_out = "no generated terrain to preview";
		return false;
	}

	int resolution_clamped = resolution;
	if (resolution_clamped < 64) resolution_clamped = 64;
	if (resolution_clamped > 1024) resolution_clamped = 1024;

	AABoxClass bounds;
	if (!WorldTerrainSystem::Get_Bounds (&bounds)) {
		if (error_out != nullptr) *error_out = "terrain has no bounds";
		return false;
	}

	int width = resolution_clamped;
	int height = resolution_clamped;
	unsigned char * rgb = new unsigned char [width * height * 3];
	Fill_Buffer (rgb,width,height,16,16,24);		// dark background: unsampled/out-of-bounds space

	//	Height/cliffs, sampled per pixel -- the two layers WorldTerrainSystem alone can answer.
	if ((layer_mask & (PREVIEW_LAYER_HEIGHT | PREVIEW_LAYER_CLIFFS)) != 0) {
		float min_z = 0.0f, max_z = 0.0f;
		bool have_range = false;

		int chunk_count_x = WorldTerrainSystem::Get_Chunk_Count_X ();
		int chunk_count_y = WorldTerrainSystem::Get_Chunk_Count_Y ();
		float chunk_width  = (chunk_count_x > 0) ? ((bounds.Extent.X * 2.0f) / (float)chunk_count_x) : 0.0f;
		float chunk_height = (chunk_count_y > 0) ? ((bounds.Extent.Y * 2.0f) / (float)chunk_count_y) : 0.0f;

		for (int py = 0; py < height; py++) {
			for (int px = 0; px < width; px++) {

				float u = (float)px / (float)(width - 1);
				float v = 1.0f - ((float)py / (float)(height - 1));
				float world_x = (bounds.Center.X - bounds.Extent.X) + (u * bounds.Extent.X * 2.0f);
				float world_y = (bounds.Center.Y - bounds.Extent.Y) + (v * bounds.Extent.Y * 2.0f);

				float sample_height = 0.0f;
				if (!WorldTerrainSystem::Sample_Height (world_x,world_y,&sample_height)) continue;

				if (!have_range) { min_z = sample_height; max_z = sample_height; have_range = true; }
				if (sample_height < min_z) min_z = sample_height;
				if (sample_height > max_z) max_z = sample_height;

				bool is_cliff = false;
				if (((layer_mask & PREVIEW_LAYER_CLIFFS) != 0) && (chunk_width > 0.0f) && (chunk_height > 0.0f)) {
					int cx = (int)((world_x - (bounds.Center.X - bounds.Extent.X)) / chunk_width);
					int cy = (int)((world_y - (bounds.Center.Y - bounds.Extent.Y)) / chunk_height);
					TerrainChunkInfoClass info;
					if (WorldTerrainSystem::Get_Chunk_Info (cx,cy,&info)) {
						is_cliff = info.CliffBand || info.InteriorCliff;
					}
				}

				if (is_cliff) {
					Plot_Point (rgb,width,height,px,py,120,90,60);
				} else if ((layer_mask & PREVIEW_LAYER_HEIGHT) != 0) {
					float range = (max_z - min_z);
					float t = (range > 0.001f) ? ((sample_height - min_z) / range) : 0.5f;
					unsigned char shade = (unsigned char)(40.0f + (t * 180.0f));
					Plot_Point (rgb,width,height,px,py,shade / 2,shade,shade / 3);
				}

				if (((layer_mask & PREVIEW_LAYER_WATER) != 0) && WaterSystem::Is_Point_In_Water (world_x,world_y)) {
					Plot_Point (rgb,width,height,px,py,40,90,180);
				}
			}
		}
	}

	if ((layer_mask & PREVIEW_LAYER_ROADS) != 0) {
		for (int i = 0; i < RoadGraphGenerator::Get_Edge_Count (); i++) {
			RoadGraphEdge edge;
			if (!RoadGraphGenerator::Get_Edge (i,&edge)) continue;
			StrategicNode from_node, to_node;
			if (!StrategicPlanner::Get_Node (edge.FromNodeIndex,&from_node)) continue;
			if (!StrategicPlanner::Get_Node (edge.ToNodeIndex,&to_node)) continue;
			int px0,py0,px1,py1;
			if (!World_To_Pixel (bounds,from_node.Position.X,from_node.Position.Y,width,height,&px0,&py0)) continue;
			if (!World_To_Pixel (bounds,to_node.Position.X,to_node.Position.Y,width,height,&px1,&py1)) continue;
			Plot_Line (rgb,width,height,px0,py0,px1,py1,200,200,60);
		}
	}

	if ((layer_mask & PREVIEW_LAYER_BRIDGES) != 0) {
		for (int i = 0; i < BridgePlanner::Get_Bridge_Count (); i++) {
			ResolvedBridge bridge;
			if (!BridgePlanner::Get_Bridge (i,&bridge)) continue;
			int node_index = -1;
			if (!StrategicPlanner::Find_Node_By_Id (bridge.CrossingNodeId,&node_index)) continue;
			StrategicNode node;
			if (!StrategicPlanner::Get_Node (node_index,&node)) continue;
			int px,py;
			if (!World_To_Pixel (bounds,node.Position.X,node.Position.Y,width,height,&px,&py)) continue;
			Plot_Filled_Square (rgb,width,height,px,py,2,230,140,40);
		}
	}

	if ((layer_mask & PREVIEW_LAYER_BASES) != 0) {
		static const int FACTIONS [2] = { PLAYERTYPE_NOD,PLAYERTYPE_GDI };
		for (int i = 0; i < 2; i++) {
			SelectedBaseSite site;
			if (!BaseSelectionPlanner::Get_Selected_Site (FACTIONS [i],&site)) continue;
			int px,py;
			if (!World_To_Pixel (bounds,site.Position.X,site.Position.Y,width,height,&px,&py)) continue;
			unsigned char red   = (FACTIONS [i] == PLAYERTYPE_NOD) ? 230 : 40;
			unsigned char green = (FACTIONS [i] == PLAYERTYPE_NOD) ? 40 : 230;
			Plot_Filled_Square (rgb,width,height,px,py,4,red,green,40);
		}
	}

	if ((layer_mask & PREVIEW_LAYER_TIBERIUM) != 0) {
		for (int i = 0; i < TiberiumFieldGenerator::Get_Field_Count (); i++) {
			ResolvedTiberiumField field;
			if (!TiberiumFieldGenerator::Get_Field (i,&field)) continue;
			int px,py;
			if (!World_To_Pixel (bounds,field.Position.X,field.Position.Y,width,height,&px,&py)) continue;
			Plot_Filled_Square (rgb,width,height,px,py,2,40,220,90);
		}
	}

	if ((layer_mask & PREVIEW_LAYER_SETTLEMENTS) != 0) {
		for (int i = 0; i < SettlementGenerator::Get_Settlement_Count (); i++) {
			ResolvedSettlement settlement;
			if (!SettlementGenerator::Get_Settlement (i,&settlement)) continue;
			int px,py;
			if (!World_To_Pixel (bounds,settlement.Position.X,settlement.Position.Y,width,height,&px,&py)) continue;
			Plot_Filled_Square (rgb,width,height,px,py,3,180,180,180);
		}
	}

	if ((layer_mask & PREVIEW_LAYER_BOUNDARY) != 0) {
		int corners_x [4], corners_y [4];
		float min_x = bounds.Center.X - bounds.Extent.X, max_x = bounds.Center.X + bounds.Extent.X;
		float min_y = bounds.Center.Y - bounds.Extent.Y, max_y = bounds.Center.Y + bounds.Extent.Y;
		bool ok = true;
		ok = ok && World_To_Pixel (bounds,min_x,min_y,width,height,&corners_x [0],&corners_y [0]);
		ok = ok && World_To_Pixel (bounds,max_x,min_y,width,height,&corners_x [1],&corners_y [1]);
		ok = ok && World_To_Pixel (bounds,max_x,max_y,width,height,&corners_x [2],&corners_y [2]);
		ok = ok && World_To_Pixel (bounds,min_x,max_y,width,height,&corners_x [3],&corners_y [3]);
		if (ok) {
			for (int i = 0; i < 4; i++) {
				int j = (i + 1) % 4;
				Plot_Line (rgb,width,height,corners_x [i],corners_y [i],corners_x [j],corners_y [j],255,60,60);
			}
		}
	}

	bool wrote = Write_PPM_Buffer_To_File (file_path,rgb,width,height,error_out);
	delete [] rgb;
	return wrote;
}


/***********************************************************************************************
 * Run_Seed_Batch -- Phase 27's own deterministic seed batch runner.                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
int ProceduralWorldToolsService::Run_Seed_Batch(unsigned int start_seed,int count,const char * profile_path,
																  ProceduralWorldSeedBatchResult * results_out,StringClass * error_out)
{
	if ((results_out == nullptr) || (count <= 0)) {
		if (error_out != nullptr) *error_out = "invalid argument";
		return 0;
	}

	int written = 0;
	int bounded_count = (count > MAX_SEED_BATCH_RESULTS) ? MAX_SEED_BATCH_RESULTS : count;

	for (int i = 0; i < bounded_count; i++) {

		unsigned int seed = start_seed + (unsigned int)i;

		ProceduralWorldManager::Destroy_World ();

		StringClass request_error;
		if (!ProceduralWorldManager::Request_World (seed,profile_path,&request_error)) {
			if (error_out != nullptr) *error_out = request_error;
			break;		//	step 9: stop outright, do not keep grinding through the rest
		}

		StringClass generate_error;
		bool generated = ProceduralWorldManager::Generate_World (&generate_error);

		ProceduralWorldSeedBatchResult & result = results_out [written];
		result.Seed = seed;
		result.GenerateSucceeded = generated;
		result.CombatSpacePassed = CombatSpaceValidator::Passed ();
		result.TransportPassed   = TransportationNetworkValidator::Passed ();
		written++;

		if (!generated) {
			if (error_out != nullptr) *error_out = generate_error;
			break;		//	the one hard failure Generate_World itself ever reports
		}
	}

	return written;
}
