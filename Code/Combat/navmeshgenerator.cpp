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
 *                     $Archive:: /Commando/Code/Combat/navmeshgenerator.cpp                  $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "navmeshgenerator.h"

#include "aabox.h"
#include "bridgeplanner.h"
#include "crc.h"
#include "Pathfind.h"
#include "PathfindPortal.h"
#include "PathfindSector.h"
#include "proceduralassetregistry.h"
#include "roadgraphgenerator.h"
#include "settlementgenerator.h"
#include "strategicplanner.h"
#include "watersystem.h"
#include "worldboundsservice.h"
#include "worldmetrics.h"
#include "worldterrainsystem.h"
#include "wwdebug.h"
#include "wwmath.h"


NavMeshParams::NavMeshParams(void) :
	StructureClearanceMargin(2.0f),
	BridgeLinkSearchRadius(80.0f),
	FlightFloorClearanceMargin(15.0f)
{
}


bool				NavMeshGenerator::Built						= false;
int				NavMeshGenerator::SectorCount				= 0;
int				NavMeshGenerator::PortalCount				= 0;
int				NavMeshGenerator::BridgePortalCount		= 0;
bool				NavMeshGenerator::GroundVehicleIncluded	= false;
float				NavMeshGenerator::MaxClimbableSlopeRadians	= 0.0f;
bool				NavMeshGenerator::HasFlightVolume		= false;
float				NavMeshGenerator::FlightMinAltitude		= 0.0f;
float				NavMeshGenerator::FlightMaxAltitude		= 0.0f;
unsigned int	NavMeshGenerator::Checksum					= 0;
unsigned int	NavMeshGenerator::Revision					= 0;

int				NavMeshGenerator::ChunkCountX				= 0;
int				NavMeshGenerator::ChunkCountY				= 0;
DynamicVectorClass<int>	NavMeshGenerator::ChunkNavigable;


void NavMeshGenerator::Init(void)
{
	Clear();
}


void NavMeshGenerator::Shutdown(void)
{
	Clear();
}


void NavMeshGenerator::Clear(void)
{
	if (PathfindClass::Get_Instance() != nullptr) {
		PathfindClass::Get_Instance()->Reset_Portals();
		PathfindClass::Get_Instance()->Reset_Sectors();
	}

	Built						= false;
	SectorCount				= 0;
	PortalCount				= 0;
	BridgePortalCount		= 0;
	GroundVehicleIncluded	= false;
	MaxClimbableSlopeRadians	= 0.0f;
	HasFlightVolume			= false;
	FlightMinAltitude		= 0.0f;
	FlightMaxAltitude		= 0.0f;
	Checksum					= 0;
	Revision					= 0;

	ChunkCountX				= 0;
	ChunkCountY				= 0;
	ChunkNavigable.Delete_All();
}


bool NavMeshGenerator::Is_Chunk_Navigable(int px,int py)
{
	if ((px < 0) || (py < 0) || (px >= ChunkCountX) || (py >= ChunkCountY)) return false;
	return ChunkNavigable[px + (py * ChunkCountX)] != 0;
}


//	One resolved structure exclusion -- a real footprint radius, resolved the same way
//	GeneratedWorldIntegrator resolves collision, not a guessed box.
struct StructureExclusion
{
	Vector3	Position;
	float		Radius;

	bool	operator == (const StructureExclusion &) const	{ return false; }
	bool	operator != (const StructureExclusion &) const	{ return true; }
};


static bool Overlaps_Any_Exclusion(const DynamicVectorClass<StructureExclusion> & exclusions,
												const Vector3 & point,float clearance_margin)
{
	for (int i = 0; i < exclusions.Count(); i++) {
		const StructureExclusion & e = exclusions[i];
		float dx = point.X - e.Position.X;
		float dy = point.Y - e.Position.Y;
		float limit = e.Radius + clearance_margin;
		if ((dx * dx + dy * dy) <= (limit * limit)) {
			return true;
		}
	}
	return false;
}


//	Builds (or finds) the portal between two already-registered sectors and wires both sides --
//	the exact real sequence Pathfind.cpp's own waypath-portal code already uses: construct, set
//	bounds, Add_Dest_Sector on each end, PathfindClass::Add_Portal assigns the real id, then each
//	PathfindSectorClass::Add_Portal(id) records it, then this file releases its own construction
//	reference (Add_Portal already Add_Ref'd it).
static void Link_Sectors(PathfindSectorClass * sector_a,int index_a,
									PathfindSectorClass * sector_b,int index_b,
									const AABoxClass & portal_box)
{
	PathfindPortalClass * portal = new PathfindPortalClass;
	portal->Set_Bounding_Box(portal_box);
	portal->Add_Dest_Sector(index_a);
	portal->Add_Dest_Sector(index_b);

	int portal_id = PathfindClass::Get_Instance()->Add_Portal(portal);
	sector_a->Add_Portal((uint32)portal_id);
	sector_b->Add_Portal((uint32)portal_id);

	portal->Release_Ref();
}


static AABoxClass Union_Box(const AABoxClass & a,const AABoxClass & b)
{
	Vector3 min_a = a.Center - a.Extent;
	Vector3 max_a = a.Center + a.Extent;
	Vector3 min_b = b.Center - b.Extent;
	Vector3 max_b = b.Center + b.Extent;

	Vector3 lo(WWMath::Min(min_a.X,min_b.X),WWMath::Min(min_a.Y,min_b.Y),WWMath::Min(min_a.Z,min_b.Z));
	Vector3 hi(WWMath::Max(max_a.X,max_b.X),WWMath::Max(max_a.Y,max_b.Y),WWMath::Max(max_a.Z,max_b.Z));

	AABoxClass box;
	box.Center = (lo + hi) * 0.5f;
	box.Extent = (hi - lo) * 0.5f;
	return box;
}


bool NavMeshGenerator::Build(const NavMeshParams & params,StringClass * error_out)
{
	#define FAIL(msg) { if (error_out) *error_out = msg; Clear(); return false; }

	Clear();

	if (!WorldTerrainSystem::Has_Chunk_Registry()) FAIL("no terrain chunk registry -- generate macro terrain first");

	WorldMetricsStruct infantry;
	if (!WorldMetricsClass::Get_Class_Metrics(LOCOMOTION_INFANTRY,&infantry)) {
		FAIL("no infantry definition loaded -- cannot size a real ground sector");
	}

	float max_climbable_slope = infantry.MaxClimbableSlope;
	bool ground_vehicle_included = false;

	WorldMetricsStruct wheeled;
	if (WorldMetricsClass::Get_Class_Metrics(LOCOMOTION_WHEELED,&wheeled)) {
		max_climbable_slope = WWMath::Min(max_climbable_slope,wheeled.MaxClimbableSlope);
		ground_vehicle_included = true;
	}

	WorldMetricsStruct tracked;
	if (WorldMetricsClass::Get_Class_Metrics(LOCOMOTION_TRACKED,&tracked)) {
		max_climbable_slope = WWMath::Min(max_climbable_slope,tracked.MaxClimbableSlope);
		ground_vehicle_included = true;
	}

	int count_x = WorldTerrainSystem::Get_Chunk_Count_X();
	int count_y = WorldTerrainSystem::Get_Chunk_Count_Y();
	if ((count_x <= 0) || (count_y <= 0)) FAIL("chunk registry is empty");

	//	Real structure exclusions, resolved the same way GeneratedWorldIntegrator resolves
	//	collision -- reused, not re-derived.
	DynamicVectorClass<StructureExclusion> exclusions;
	if (SettlementGenerator::Has_Settlements()) {
		int structure_count = SettlementGenerator::Get_Structure_Count();
		for (int i = 0; i < structure_count; i++) {
			PlacedStructureInstance instance;
			if (!SettlementGenerator::Get_Structure(i,&instance)) continue;

			ProceduralAssetDescriptor asset;
			if (!ProceduralAssetRegistry::Get_Asset_By_Id(instance.AssetId,&asset)) continue;

			float radius = 0.0f, height = 0.0f;
			if (!asset.Resolve_Placement_Radius(&radius,&height)) continue;

			StructureExclusion exclusion;
			exclusion.Position	= instance.Position;
			exclusion.Radius		= radius;
			exclusions.Add(exclusion);
		}
	}

	//	No live PhysicsSceneClass (and so no PathfindClass -- Pathfinder is allocated inside
	//	PhysicsSceneClass's own constructor, pscene.cpp) is a real, legitimate state this
	//	generator can run in, the same "graceful no real objects, not a crash" answer
	//	FoliageSystem's own Add_Instance/Add_Proxy already give for the identical situation.
	bool has_pathfinder = (PathfindClass::Get_Instance() != nullptr);

	DynamicVectorClass<int>				sector_index;		// px + py*count_x -> PathfindClass sector index, or -1
	DynamicVectorClass<PathfindSectorClass *>	sector_ptr;	// same indexing, the sector object itself
	DynamicVectorClass<AABoxClass>		sector_bounds;
	sector_index.Resize(count_x * count_y);
	sector_ptr.Resize(count_x * count_y);
	sector_bounds.Resize(count_x * count_y);
	for (int i = 0; i < (count_x * count_y); i++) {
		sector_index.Add(-1);
		sector_ptr.Add(nullptr);
		sector_bounds.Add(AABoxClass());
	}

	/*
	**	Which chunks this generator judged walkable, recorded separately from which ones became
	**	registered PathfindClass sectors.  The two differ for one real reason: with no live
	**	PhysicsSceneClass there is no PathfindClass to register a sector into (see the comment
	**	above `has_pathfinder`), and a chunk's walkability is a fact about the terrain either way.
	**
	**	Phase 17's chokepoint analysis is the consumer, and it must read this one judgement
	**	rather than re-deriving a second walkability predicate that could quietly disagree with
	**	the sectors the AI actually paths over.
	*/
	ChunkCountX = count_x;
	ChunkCountY = count_y;
	ChunkNavigable.Resize(count_x * count_y);
	for (int i = 0; i < (count_x * count_y); i++) {
		ChunkNavigable.Add(0);
	}

	float highest_chunk_z = -100000.0f;
	int sector_count = 0;

	for (int py = 0; py < count_y; py++) {
		for (int px = 0; px < count_x; px++) {

			TerrainChunkInfoClass info;
			if (!WorldTerrainSystem::Get_Chunk_Info(px,py,&info)) continue;
			highest_chunk_z = WWMath::Max(highest_chunk_z,info.MaxHeight);

			if (info.CliffBand || info.InteriorCliff) continue;
			if (info.MaxSlopeRadians > max_climbable_slope) continue;

			AABoxClass bounds;
			if (!WorldTerrainSystem::Get_Patch_Bounds(px,py,&bounds)) continue;

			if (WaterSystem::Is_Point_In_Water(bounds.Center.X,bounds.Center.Y)) continue;
			if (Overlaps_Any_Exclusion(exclusions,bounds.Center,params.StructureClearanceMargin)) continue;

			//	Walkable.  Recorded before the pathfinder question, because that is what it is.
			ChunkNavigable[px + (py * count_x)] = 1;

			if (!has_pathfinder) continue;

			PathfindSectorClass * sector = new PathfindSectorClass;
			sector->Set_Bounding_Box(bounds);
			PathfindClass::Get_Instance()->Add_Sector(sector);

			int slot = px + (py * count_x);
			sector_index[slot]	= PathfindClass::Get_Instance()->Get_Sector_Index(sector);
			sector_ptr[slot]		= sector;
			sector_bounds[slot]	= bounds;
			sector_count++;

			//	Add_Sector already Add_Ref'd; this file's own construction reference is done.
			sector->Release_Ref();
		}
	}

	int portal_count = 0;
	for (int py = 0; py < count_y; py++) {
		for (int px = 0; px < count_x; px++) {

			int slot = px + (py * count_x);
			int idx_here = sector_index[slot];
			if (idx_here < 0) continue;

			if ((px + 1) < count_x) {
				int east_slot = (px + 1) + (py * count_x);
				int idx_east = sector_index[east_slot];
				if (idx_east >= 0) {
					AABoxClass portal_box = Union_Box(sector_bounds[slot],sector_bounds[east_slot]);
					float mid_x = (sector_bounds[slot].Center.X + sector_bounds[east_slot].Center.X) * 0.5f;
					portal_box.Extent.X = 0.1f;
					portal_box.Center.X = mid_x;
					Link_Sectors(sector_ptr[slot],idx_here,sector_ptr[east_slot],idx_east,portal_box);
					portal_count++;
				}
			}

			if ((py + 1) < count_y) {
				int north_slot = px + ((py + 1) * count_x);
				int idx_north = sector_index[north_slot];
				if (idx_north >= 0) {
					AABoxClass portal_box = Union_Box(sector_bounds[slot],sector_bounds[north_slot]);
					float mid_y = (sector_bounds[slot].Center.Y + sector_bounds[north_slot].Center.Y) * 0.5f;
					portal_box.Extent.Y = 0.1f;
					portal_box.Center.Y = mid_y;
					Link_Sectors(sector_ptr[slot],idx_here,sector_ptr[north_slot],idx_north,portal_box);
					portal_count++;
				}
			}
		}
	}

	//	Step 7 -- bridge off-mesh links.  Real endpoints: a resolved bridge's own road edge
	//	carries the two StrategicNode indices its route actually connects; those nodes' own real
	//	positions are the two sides of the water gap this file's terrain-only floodfill cannot
	//	cross on its own.  Nearest walkable sector to each -- not the bridge deck itself, which
	//	this file does not model as its own sector (see the header comment).
	int bridge_portal_count = 0;
	for (int b = 0; b < BridgePlanner::Get_Bridge_Count(); b++) {
		ResolvedBridge bridge;
		if (!BridgePlanner::Get_Bridge(b,&bridge)) continue;
		if (!bridge.CountsAsConnectivity) continue;

		RoadGraphEdge edge;
		bool found_edge = false;
		for (int e = 0; e < RoadGraphGenerator::Get_Edge_Count(); e++) {
			RoadGraphEdge candidate;
			if (!RoadGraphGenerator::Get_Edge(e,&candidate)) continue;
			if (candidate.Id == bridge.RoadEdgeId) { edge = candidate; found_edge = true; break; }
		}
		if (!found_edge) continue;

		StrategicNode from_node, to_node;
		if (!StrategicPlanner::Get_Node(edge.FromNodeIndex,&from_node)) continue;
		if (!StrategicPlanner::Get_Node(edge.ToNodeIndex,&to_node)) continue;

		int best_from = -1, best_to = -1;
		float best_from_dist = params.BridgeLinkSearchRadius;
		float best_to_dist = params.BridgeLinkSearchRadius;

		for (int slot = 0; slot < (count_x * count_y); slot++) {
			if (sector_index[slot] < 0) continue;
			const Vector3 & center = sector_bounds[slot].Center;

			float dx1 = center.X - from_node.Position.X;
			float dy1 = center.Y - from_node.Position.Y;
			float d1 = WWMath::Sqrt((dx1 * dx1) + (dy1 * dy1));
			if (d1 < best_from_dist) { best_from_dist = d1; best_from = slot; }

			float dx2 = center.X - to_node.Position.X;
			float dy2 = center.Y - to_node.Position.Y;
			float d2 = WWMath::Sqrt((dx2 * dx2) + (dy2 * dy2));
			if (d2 < best_to_dist) { best_to_dist = d2; best_to = slot; }
		}

		if ((best_from >= 0) && (best_to >= 0) && (best_from != best_to)) {
			AABoxClass portal_box = Union_Box(sector_bounds[best_from],sector_bounds[best_to]);
			Link_Sectors(sector_ptr[best_from],sector_index[best_from],
							 sector_ptr[best_to],sector_index[best_to],portal_box);
			bridge_portal_count++;
			portal_count++;
		}
	}

	//	Step 5 -- flight volume.  A real bound, not a sector graph; see the header comment.
	bool has_flight_volume = false;
	float flight_min = 0.0f, flight_max = 0.0f;
	if (highest_chunk_z > -100000.0f) {
		flight_min = highest_chunk_z + params.FlightFloorClearanceMargin;
		flight_max = WorldBoundsService::Get_Air_Ceiling();
		has_flight_volume = (flight_max > flight_min);
	}

	{
		CRCEngine crc;
		crc(&sector_count,sizeof(sector_count));
		crc(&portal_count,sizeof(portal_count));
		crc(&bridge_portal_count,sizeof(bridge_portal_count));
		int q_slope = (int)(max_climbable_slope * 100000.0f);
		crc(&q_slope,sizeof(q_slope));
		int q_ground = ground_vehicle_included ? 1 : 0;
		crc(&q_ground,sizeof(q_ground));
		Checksum = (unsigned int)crc;
	}

	SectorCount					= sector_count;
	PortalCount					= portal_count;
	BridgePortalCount			= bridge_portal_count;
	GroundVehicleIncluded	= ground_vehicle_included;
	MaxClimbableSlopeRadians	= max_climbable_slope;
	HasFlightVolume			= has_flight_volume;
	FlightMinAltitude		= flight_min;
	FlightMaxAltitude		= flight_max;
	Revision						= 1;
	Built							= true;

	#undef FAIL
	return true;
}


bool NavMeshGenerator::Get_Flight_Volume(float * min_altitude_out,float * max_altitude_out)
{
	if (!HasFlightVolume) return false;
	if (min_altitude_out != nullptr) *min_altitude_out = FlightMinAltitude;
	if (max_altitude_out != nullptr) *max_altitude_out = FlightMaxAltitude;
	return true;
}
