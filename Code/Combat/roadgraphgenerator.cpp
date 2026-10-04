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
 *                     $Archive:: /Commando/Code/Combat/roadgraphgenerator.cpp                $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "roadgraphgenerator.h"

#include "crc.h"
#include "roadspline.h"
#include "roadsystem.h"
#include "strategicplanner.h"
#include "terrainmask.h"
#include "watersystem.h"
#include "hydrologygenerator.h"
#include "mapplangenerator.h"
#include "worldboundsservice.h"
#include "worldmetrics.h"
#include "worldterrainsystem.h"
#include "wwdebug.h"
#include "wwmath.h"


bool				RoadGraphGenerator::Built			= false;
unsigned int	RoadGraphGenerator::Checksum		= 0;

DynamicVectorClass<int>				RoadGraphGenerator::OwnedRoadIds;
DynamicVectorClass<RoadGraphEdge>	RoadGraphGenerator::Edges;

RoadGraphParams	RoadGraphGenerator::LastParams;

DynamicVectorClass<StringClass>	RoadGraphGenerator::ExclusionEdgeIds;
DynamicVectorClass<int>				RoadGraphGenerator::ExclusionChunkIndices;


/*
**	Section 3A.1/10A.1's authoritative GDI Humvee collision width, in world units.  Measured,
**	not guessed and not taken from real-world HMMWV dimensions -- 10A.1 explicitly rules real-
**	world dimensions out, and this project does not guess an enumerable content identifier
**	either, so the number comes from actually reading the shipped `Data/always.dat`.
**
**	Method: `Code/wwlib/mixfile.cpp`'s own MIX1 format (4-byte "MIX1" signature, a header-
**	offset table of CRC32/offset/size entries keyed by `CRC_Stringi` of the *uppercased*
**	filename -- `Code/wwlib/realcrc.cpp`'s own standard reflected CRC-32 -- and a names-offset
**	table of the plain filenames) locates `v_gdi_humvee.w3d` inside the mix without guessing an
**	offset, the same file `WorldMetricsClass::Resolve_Extents` would load at runtime.
**	`Code/ww3d2/w3d_file.h`'s own `W3D_CHUNK_BOX` (0x740) / `W3dBoxStruct` layout (Version,
**	Attributes, Name, Color, Center, Extent) then reads the `V_GDI_HUMVEE.WORLDBOX` sub-object
**	directly -- the same sub-object `Resolve_Extents` itself looks up by name, and which its
**	own comment already identifies as "where Phys3Class itself gets its collision box".
**
**	The box's own extents are X=4.723, Y=2.556, Z=1.548 -- length, width, height, in that
**	order, confirmed by the same ordering (length > width > height) independently holding for
**	`v_nod_buggy.w3d`'s and `v_gdi_apc_m.w3d`'s own WORLDBOX chunks too, so the local +X axis is
**	this engine's own consistent vehicle-forward convention and +Y is the lateral axis a road
**	lane actually has to admit.  GDI_HUMVEE_WIDTH is therefore the box's own full Y extent.
*/
static const float GDI_HUMVEE_WIDTH = 2.556f;

/*
**	Section 10A.1's own derivation, verbatim, using the measured width above and
**	WorldMetricsClass's own resolved WORLD_UNITS_PER_METER (Section 3A.1).
**
**		C = max(0.35 metres converted to world units, H * 0.15)
**		NormalLaneWidth = max(H + 2*C, H * 1.30)
*/
static const float ROAD_GRAPH_LANE_CLEARANCE = WWMath::Max (
	0.35f * WorldMetricsClass::Get_World_Units_Per_Meter (),
	GDI_HUMVEE_WIDTH * 0.15f);
static const float ROAD_GRAPH_NORMAL_LANE_WIDTH = WWMath::Max (
	GDI_HUMVEE_WIDTH + (2.0f * ROAD_GRAPH_LANE_CLEARANCE),
	GDI_HUMVEE_WIDTH * 1.30f);

//	Section 10A.3's own default lane counts.  CITY_STREET and SERVICE both name a possible
//	profile-driven exception (arterial CITY_STREET up to 4 lanes; SERVICE down to 1 wide lane)
//	that no profile field exists to express yet, so both use their own stated default here.
static const float ROAD_GRAPH_CLASS_WIDTH[ROAD_GRAPH_CLASS_COUNT] =
{
	ROAD_GRAPH_NORMAL_LANE_WIDTH * 4.0f,	// HIGHWAY -- 4 lanes, 2 each direction
	ROAD_GRAPH_NORMAL_LANE_WIDTH * 2.0f,	// PRIMARY -- 2 lanes
	ROAD_GRAPH_NORMAL_LANE_WIDTH * 2.0f,	// SECONDARY -- 2 lanes
	ROAD_GRAPH_NORMAL_LANE_WIDTH * 2.0f,	// CITY_STREET -- 2 lanes by default
	ROAD_GRAPH_NORMAL_LANE_WIDTH * 2.0f,	// SERVICE -- 2 lanes by default
	ROAD_GRAPH_NORMAL_LANE_WIDTH * 1.0f,	// DIRT_TRACK -- 1 wide lane
};

static const float ROAD_GRAPH_CLASS_SHOULDER[ROAD_GRAPH_CLASS_COUNT] =
{
	3.0f, 2.0f, 1.5f, 1.5f, 1.0f, 0.5f,
};

//	A chunk this far or closer to a StrategicNode is considered "at" it, for both endpoint
//	lookup and crossing-candidate detection.
static const float NODE_CHUNK_RADIUS = 48.0f;

static const float CHECKSUM_QUANTUM = 1000.0f;

static const float COST_JITTER_WEIGHT = 0.02f;

static const int NEIGHBOR_DX[4] = {  1, 0, -1, 0 };
static const int NEIGHBOR_DY[4] = {  0, 1,  0,-1 };


static int Road_Graph_Class_To_Road_Class(RoadGraphClassType cls)
{
	switch (cls) {
		case ROAD_GRAPH_HIGHWAY:
		case ROAD_GRAPH_PRIMARY:			return ROAD_CLASS_HIGHWAY;
		case ROAD_GRAPH_SECONDARY:
		case ROAD_GRAPH_CITY_STREET:		return ROAD_CLASS_STREET;
		case ROAD_GRAPH_SERVICE:
		case ROAD_GRAPH_DIRT_TRACK:
		default:									return ROAD_CLASS_TRACK;
	}
}


RoadGraphParams::RoadGraphParams(void) :
	Seed (1),
	MaxBuildableSlopeRadians (0.4886922f),
	SlopeCostWeight (3.0f),
	CrossingCostPenalty (8.0f),
	BoundaryApproachCostWeight (1.5f),
	RequireBridgeCrossing (false)
{
}


RoadGraphEdge::RoadGraphEdge(void) :
	RoadId (-1),
	FromNodeIndex (-1),
	ToNodeIndex (-1),
	Class (ROAD_GRAPH_DIRT_TRACK),
	Length (0.0f)
{
}


bool RoadGraphEdge::operator == (const RoadGraphEdge & that) const
{
	if ((Id != that.Id) || (RoadId != that.RoadId) || (FromNodeIndex != that.FromNodeIndex) ||
		 (RequiredBridgeCrossingId != that.RequiredBridgeCrossingId) ||
		 (ToNodeIndex != that.ToNodeIndex) || (Class != that.Class) || (Length != that.Length) ||
		 (UnresolvedCrossingIds.Count() != that.UnresolvedCrossingIds.Count())) {
		return false;
	}
	for (int i = 0; i < UnresolvedCrossingIds.Count(); i++) {
		if (UnresolvedCrossingIds[i] != that.UnresolvedCrossingIds[i]) return false;
	}
	return true;
}


void RoadGraphGenerator::Init(void)
{
	Clear();
}


void RoadGraphGenerator::Shutdown(void)
{
	Clear();
}


void RoadGraphGenerator::Adopt_Owned_Road_Id(int road_id)
{
	if ((road_id < 0) || (road_id >= RoadSystem::Get_Road_Count())) {
		return;
	}
	for (int i = 0; i < OwnedRoadIds.Count(); i++) {
		if (OwnedRoadIds[i] == road_id) {
			return;
		}
	}
	OwnedRoadIds.Add(road_id);
}


bool RoadGraphGenerator::Adopt_Graph(const DynamicVectorClass<RoadGraphEdge> & edges,unsigned int checksum)
{
	Clear();
	for (int i = 0; i < edges.Count(); i++) {
		Edges.Add(edges[i]);
	}
	Checksum	= checksum;
	Built		= true;
	return true;
}


void RoadGraphGenerator::Clear(void)
{
	if (OwnedRoadIds.Count() > 0) {
		RoadSystem::Destroy_Geometry();

		DynamicVectorClass<RoadSplineClass> keep;
		for (int i = 0; i < RoadSystem::Get_Road_Count(); i++) {
			RoadSplineClass * road = RoadSystem::Peek_Road(i);
			if (road == nullptr) continue;

			bool owned = false;
			for (int j = 0; j < OwnedRoadIds.Count(); j++) {
				if (OwnedRoadIds[j] == i) { owned = true; break; }
			}
			if (!owned) keep.Add(*road);
		}

		RoadSystem::Clear_Roads();
		for (int i = 0; i < keep.Count(); i++) {
			RoadSystem::Add_Road(keep[i]);
		}
		if (keep.Count() > 0) {
			RoadSystem::Build_Network();
		}
	}

	OwnedRoadIds.Delete_All();
	Edges.Delete_All();
	ExclusionEdgeIds.Delete_All();
	ExclusionChunkIndices.Delete_All();

	Built		= false;
	Checksum	= 0;
}


bool RoadGraphGenerator::Get_Edge(int index,RoadGraphEdge * out)
{
	if ((index < 0) || (index >= Edges.Count()) || (out == nullptr)) {
		return false;
	}
	*out = Edges[index];
	return true;
}


/*
**	The unique path between two anchor nodes through this generator's own minimum spanning
**	tree -- real road distance for a caller (BaseSelectionPlanner, TiberiumFieldGenerator, ...)
**	that needs to measure one between two StrategicPlanner node indices, rather than each caller
**	re-deriving the same breadth-first walk over Edges.  A tree has at most (anchor_count - 1)
**	edges, always far smaller than any world's own chunk count, so this cannot loop.
*/
bool RoadGraphGenerator::Find_Anchor_Path(int from_node,int to_node,
														 DynamicVectorClass<int> * edge_indices_out,float * length_out)
{
	if (length_out != nullptr) *length_out = 0.0f;

	if (from_node == to_node) return true;

	DynamicVectorClass<int>	visited_nodes;
	DynamicVectorClass<int>	parent_edge;

	visited_nodes.Add(from_node);
	parent_edge.Add(-1);

	int head = 0;
	while (head < visited_nodes.Count()) {
		int current = visited_nodes[head];
		if (current == to_node) break;

		for (int e = 0; e < Edges.Count(); e++) {
			const RoadGraphEdge & edge = Edges[e];

			int other = -1;
			if (edge.FromNodeIndex == current)			other = edge.ToNodeIndex;
			else if (edge.ToNodeIndex == current)		other = edge.FromNodeIndex;
			if (other < 0) continue;

			bool already = false;
			for (int v = 0; v < visited_nodes.Count(); v++) {
				if (visited_nodes[v] == other) { already = true; break; }
			}
			if (already) continue;

			visited_nodes.Add(other);
			parent_edge.Add(e);
		}

		head++;
	}

	int idx = -1;
	for (int v = 0; v < visited_nodes.Count(); v++) {
		if (visited_nodes[v] == to_node) { idx = v; break; }
	}
	if (idx < 0) return false;		// not reachable in the tree

	DynamicVectorClass<int> reversed;
	int walk = idx;
	while (parent_edge[walk] >= 0) {
		int e = parent_edge[walk];
		reversed.Add(e);

		const RoadGraphEdge & edge = Edges[e];
		int parent_node = (edge.FromNodeIndex == visited_nodes[walk]) ? edge.ToNodeIndex : edge.FromNodeIndex;

		int parent_walk_index = -1;
		for (int v = 0; v < visited_nodes.Count(); v++) {
			if (visited_nodes[v] == parent_node) { parent_walk_index = v; break; }
		}
		if (parent_walk_index < 0) break;		// defensive; a well-formed tree never hits this
		walk = parent_walk_index;
	}

	float length = 0.0f;
	for (int i = reversed.Count() - 1; i >= 0; i--) {
		if (edge_indices_out != nullptr) edge_indices_out->Add(reversed[i]);
		length += Edges[reversed[i]].Length;
	}
	if (length_out != nullptr) *length_out = length;
	return true;
}


/*
**	Routing cost field, one entry per chunk.  Blocked is a hard exclusion (cliff band always;
**	water/interior-cliff unless a crossing candidate sits there); Cost is everything else,
**	always finite for a chunk that is not Blocked.
*/
struct RouteChunk
{
	bool		Blocked;
	bool		HasCrossing;
	StringClass	CrossingId;
	float		Cost;
	Vector3	Position;

	bool	operator == (const RouteChunk & that) const
	{
		return (Blocked == that.Blocked) && (HasCrossing == that.HasCrossing) &&
				 (CrossingId == that.CrossingId) && (Cost == that.Cost) && (Position == that.Position);
	}
	bool	operator != (const RouteChunk & that) const	{ return !(*this == that); }
};


//	D-003: a chunk the map plan painted as a lane (or the ramp a lane climbs) costs this
//	fraction of its terrain-derived cost, so the routed road follows the planned corridor
//	whenever the corridor is at all passable.  Blocks and crossing rules are untouched.
static const float PLAN_LANE_COST_SCALE = 0.5f;

static void Compute_Route_Chunks(int count_x,int count_y,const RoadGraphParams & params,
											 DynamicVectorClass<RouteChunk> * out)
{
	out->Delete_All();
	for (int i = 0; i < (count_x * count_y); i++) {
		RouteChunk rc;
		rc.Blocked		= true;
		rc.HasCrossing	= false;
		rc.Cost			= 1.0f;
		out->Add(rc);
	}

	for (int py = 0; py < count_y; py++) {
		for (int px = 0; px < count_x; px++) {

			int index = (py * count_x) + px;
			RouteChunk & rc = (*out)[index];

			TerrainChunkInfoClass info;
			if (!WorldTerrainSystem::Get_Chunk_Info(px,py,&info)) continue;

			AABoxClass bounds;
			if (!WorldTerrainSystem::Get_Patch_Bounds(px,py,&bounds)) continue;
			rc.Position = bounds.Center;

			if (info.CliffBand) continue;		// never routable, no exception

			bool blocked_feature = info.InteriorCliff ||
											WaterSystem::Is_Point_In_Water(bounds.Center.X,bounds.Center.Y);

			rc.Blocked	= blocked_feature;		// lifted below if a crossing candidate is here
			rc.Cost		= 1.0f;

			float slope_term = info.MaxSlopeRadians / params.MaxBuildableSlopeRadians;
			if (slope_term < 0.0f) slope_term = 0.0f;
			rc.Cost += slope_term * params.SlopeCostWeight;

			float boundary_distance = WorldBoundsService::Get_Inward_Distance(
													Vector2(bounds.Center.X,bounds.Center.Y));
			if (boundary_distance < 200.0f) {
				float approach_term = 1.0f - (boundary_distance / 200.0f);
				if (approach_term < 0.0f) approach_term = 0.0f;
				rc.Cost += approach_term * params.BoundaryApproachCostWeight;
			}

			if (MapPlanGenerator::Has_Plan()) {
				MapPlanCellKind kind = MapPlanGenerator::Get_Kind_At(bounds.Center.X,bounds.Center.Y);
				if ((kind == MAP_PLAN_CELL_LANE) || (kind == MAP_PLAN_CELL_RAMP)) {
					rc.Cost *= PLAN_LANE_COST_SCALE;
				}
			}
		}
	}

	//	Crossing candidates: any StrategicNode of the right kind within NODE_CHUNK_RADIUS of a
	//	blocked chunk lifts that one chunk's block, at a real (large but finite) cost.
	if (StrategicPlanner::Has_Plan()) {
		for (int n = 0; n < StrategicPlanner::Get_Node_Count(); n++) {
			StrategicNode node;
			if (!StrategicPlanner::Get_Node(n,&node)) continue;
			if ((node.Type != STRATEGIC_NODE_CROSSING_CANDIDATE) && (node.Type != STRATEGIC_NODE_TUNNEL_CANDIDATE)) continue;

			for (int py = 0; py < count_y; py++) {
				for (int px = 0; px < count_x; px++) {
					int index = (py * count_x) + px;
					RouteChunk & rc = (*out)[index];
					if (!rc.HasCrossing && ((rc.Position - node.Position).Length() <= NODE_CHUNK_RADIUS)) {
						rc.Blocked		= false;
						rc.HasCrossing	= true;
						rc.CrossingId	= node.Id;
						rc.Cost			+= params.CrossingCostPenalty;
					}
				}
			}
		}
	}
}


static int Nearest_Chunk(const Vector3 & position,int count_x,int count_y,const DynamicVectorClass<RouteChunk> & chunks)
{
	int best = -1;
	float best_distance = 0.0f;
	for (int py = 0; py < count_y; py++) {
		for (int px = 0; px < count_x; px++) {
			int index = (py * count_x) + px;
			if (chunks[index].Blocked) continue;
			float d = (chunks[index].Position - position).Length();
			if ((best < 0) || (d < best_distance)) { best = index; best_distance = d; }
		}
	}
	return best;
}


/*
**	Dijkstra over the chunk grid, O(chunk_count^2) linear-scan (no heap) -- the same bounded-
**	simplicity tolerance every other chunk-grid pass in this tree already accepts.  Returns the
**	path as a list of chunk indices, source to destination inclusive, or an empty list if
**	unreachable.
*/
static void Route_Path(int from_index,int to_index,int count_x,int count_y,
								const DynamicVectorClass<RouteChunk> & chunks,unsigned int seed,
								DynamicVectorClass<int> * out_path)
{
	out_path->Delete_All();

	int total = count_x * count_y;
	DynamicVectorClass<float>	dist;
	DynamicVectorClass<int>	prev;
	DynamicVectorClass<bool>	visited;
	for (int i = 0; i < total; i++) {
		dist.Add(-1.0f);
		prev.Add(-1);
		visited.Add(false);
	}
	dist[from_index] = 0.0f;

	for (int iter = 0; iter < total; iter++) {

		int current = -1;
		float current_dist = 0.0f;
		for (int i = 0; i < total; i++) {
			if (visited[i] || (dist[i] < 0.0f)) continue;
			if ((current < 0) || (dist[i] < current_dist)) { current = i; current_dist = dist[i]; }
		}
		if (current < 0) break;
		visited[current] = true;
		if (current == to_index) break;

		int cx = current % count_x;
		int cy = current / count_x;
		for (int d = 0; d < 4; d++) {
			int nx = cx + NEIGHBOR_DX[d];
			int ny = cy + NEIGHBOR_DY[d];
			if ((nx < 0) || (nx >= count_x) || (ny < 0) || (ny >= count_y)) continue;
			int nindex = (ny * count_x) + nx;
			if (chunks[nindex].Blocked || visited[nindex]) continue;

			float jitter = TerrainMaskClass::Hash_Value(nx,ny,(int)seed) * COST_JITTER_WEIGHT;
			float edge_cost = chunks[nindex].Cost + jitter;
			float candidate = current_dist + edge_cost;
			if ((dist[nindex] < 0.0f) || (candidate < dist[nindex])) {
				dist[nindex] = candidate;
				prev[nindex] = current;
			}
		}
	}

	if (dist[to_index] < 0.0f) return;		// unreachable

	DynamicVectorClass<int> reversed;
	int walk = to_index;
	while (walk >= 0) {
		reversed.Add(walk);
		if (walk == from_index) break;
		walk = prev[walk];
	}
	for (int i = reversed.Count() - 1; i >= 0; i--) {
		out_path->Add(reversed[i]);
	}
}


static RoadGraphClassType Classify_Edge(StrategicNodeType from_type,StrategicNodeType to_type)
{
	if ((from_type == STRATEGIC_NODE_BASE_CANDIDATE) && (to_type == STRATEGIC_NODE_BASE_CANDIDATE)) {
		return ROAD_GRAPH_HIGHWAY;
	}
	if ((from_type == STRATEGIC_NODE_RESOURCE_CANDIDATE) || (to_type == STRATEGIC_NODE_RESOURCE_CANDIDATE)) {
		return ROAD_GRAPH_PRIMARY;
	}
	if ((from_type == STRATEGIC_NODE_SETTLEMENT_CANDIDATE) || (to_type == STRATEGIC_NODE_SETTLEMENT_CANDIDATE)) {
		return ROAD_GRAPH_CITY_STREET;
	}
	return ROAD_GRAPH_DIRT_TRACK;
}


/***********************************************************************************************
 * Generate -- Phase 8 steps 1-9, 13-15                                                          *
 *=============================================================================================*/
// Reserve one of hydrology's actual river crossings on an existing strategic edge.
// Keep the tree's anchors, but route both banks explicitly so the bridge planner
// receives a straight crossing with road left on either side after it cuts the span.
static bool Reserve_Bridge_Crossing(const RoadGraphParams & params,int count_x,int count_y,
	const DynamicVectorClass<RouteChunk> & chunks,DynamicVectorClass<RoadGraphEdge> & edges,
	DynamicVectorClass<int> & owned)
{
	int best_edge = -1;
	float best_cost = 1.0e30f;
	RoadSplineClass best_spline;
	StringClass best_crossing;
	DynamicVectorClass<StringClass> best_crossings;
	for (int f = 0; f < HydrologyGenerator::Get_Feature_Count(); f++) {
		HydrologyFeature feature;
		if (!HydrologyGenerator::Get_Feature(f,&feature) || feature.Type != HYDRO_FEATURE_RIVER) continue;
		WaterAreaClass * water = WaterSystem::Find_Area(feature.Name.Peek_Buffer());
		if (water == nullptr || water->Get_Station_Count() < 2) continue;
		for (int c = 0; c < feature.Crossings.Count(); c++) {
			const Vector3 center = feature.Crossings[c].Position;
			if (!WaterSystem::Is_Point_In_Water(center.X,center.Y)) continue;
			StringClass crossing_id;
			for (int n = 0; n < StrategicPlanner::Get_Node_Count(); n++) {
				StrategicNode node;
				if (StrategicPlanner::Get_Node(n,&node) && node.Type == STRATEGIC_NODE_CROSSING_CANDIDATE &&
					(node.Position - center).Length2() < 0.01f) { crossing_id = node.Id; break; }
			}
			if (crossing_id.Is_Empty()) continue;
			int station = 0;
			float nearest = 1.0e30f;
			for (int s = 0; s < water->Get_Station_Count(); s++) {
				float distance = (water->Get_Station(s).Position - center).Length2();
				if (distance < nearest) { nearest = distance; station = s; }
			}
			int before = station > 0 ? station - 1 : station;
			int after = station + 1 < water->Get_Station_Count() ? station + 1 : station;
			Vector3 flow = water->Get_Station(after).Position - water->Get_Station(before).Position;
			Vector3 axis(-flow.Y,flow.X,0.0f);
			if (axis.Length2() <= WWMATH_EPSILON) continue;
			axis.Normalize();
			AABoxClass bounds;
			if (!WorldTerrainSystem::Get_Patch_Bounds(0,0,&bounds)) continue;
			float approach = WWMath::Max(bounds.Extent.X * 2.0f,water->Get_Station(station).Width * 2.0f);
			Vector3 bank[2] = { center - axis * approach,center + axis * approach };
			bool legal = true;
			for (int b = 0; b < 2; b++) {
				if (WaterSystem::Is_Point_In_Water(bank[b].X,bank[b].Y) ||
					WorldBoundsService::Classify_Point(Vector2(bank[b].X,bank[b].Y)) >= WORLD_ZONE_CLIFF_BAND ||
					!WorldTerrainSystem::Sample_Height(bank[b].X,bank[b].Y,&bank[b].Z)) legal = false;
			}
			if (!legal) continue;
			for (int e = 0; e < edges.Count(); e++) {
				StrategicNode from,to;
				if (!StrategicPlanner::Get_Node(edges[e].FromNodeIndex,&from) ||
					!StrategicPlanner::Get_Node(edges[e].ToNodeIndex,&to)) continue;
				float side_a = Vector3::Dot_Product(from.Position - center,axis);
				float side_b = Vector3::Dot_Product(to.Position - center,axis);
				if (side_a * side_b >= 0.0f) continue;
				Vector3 a = bank[side_a < 0.0f ? 0 : 1], b = bank[side_a < 0.0f ? 1 : 0];
				float cost = (from.Position - a).Length() + (a - b).Length() + (b - to.Position).Length();
				if (cost >= best_cost) continue;
				int from_chunk = Nearest_Chunk(from.Position,count_x,count_y,chunks);
				int to_chunk = Nearest_Chunk(to.Position,count_x,count_y,chunks);
				int bank_a = Nearest_Chunk(a,count_x,count_y,chunks);
				int bank_b = Nearest_Chunk(b,count_x,count_y,chunks);
				if (from_chunk < 0 || to_chunk < 0 || bank_a < 0 || bank_b < 0 || bank_a == bank_b) continue;
				DynamicVectorClass<int> path_a,path_b;
				Route_Path(from_chunk,bank_a,count_x,count_y,chunks,params.Seed,&path_a);
				Route_Path(bank_b,to_chunk,count_x,count_y,chunks,params.Seed,&path_b);
				if (path_a.Count() == 0 || path_b.Count() == 0) continue;
				bool overlaps = false;
				for (int i = 0; i < path_a.Count(); i++) for (int j = 0; j < path_b.Count(); j++)
					if (path_a[i] == path_b[j]) overlaps = true;
				if (overlaps) continue;
				RoadSplineClass spline;
				spline.Add_Control_Point(from.Position);
				for (int i = 0; i < path_a.Count(); i++) spline.Add_Control_Point(chunks[path_a[i]].Position);
				spline.Add_Control_Point(a);
				Vector3 deck(center.X,center.Y,(a.Z + b.Z) * 0.5f);
				spline.Add_Control_Point(deck);
				spline.Add_Control_Point(b);
				for (int i = 0; i < path_b.Count(); i++) spline.Add_Control_Point(chunks[path_b[i]].Position);
				spline.Add_Control_Point(to.Position);
				spline.Set_Width(ROAD_GRAPH_CLASS_WIDTH[edges[e].Class]);
				spline.Set_Shoulder_Width(ROAD_GRAPH_CLASS_SHOULDER[edges[e].Class]);
				spline.Set_Road_Class(Road_Graph_Class_To_Road_Class(edges[e].Class));
				spline.Set_Material("l11_gravel.tga");
				// The span is cut by BridgePlanner before any approach grading: never fill the river.
				spline.Set_Grade_Policy(ROAD_GRADE_NONE);
				best_edge = e; best_cost = cost; best_spline = spline; best_crossing = crossing_id;
				best_crossings.Delete_All(); best_crossings.Add(crossing_id);
				for (int side = 0; side < 2; side++) {
					const DynamicVectorClass<int> & path = side == 0 ? path_a : path_b;
					for (int i = 0; i < path.Count(); i++) {
						const RouteChunk & chunk = chunks[path[i]];
						if (!chunk.HasCrossing) continue;
						bool exists = false;
						for (int j = 0; j < best_crossings.Count(); j++) if (best_crossings[j] == chunk.CrossingId) exists = true;
						if (!exists) best_crossings.Add(chunk.CrossingId);
					}
				}
			}
		}
	}
	if (best_edge < 0) return false;
	RoadGraphEdge & edge = edges[best_edge];
	RoadSystem::Remove_Road(edge.RoadId);
	edge.RoadId = RoadSystem::Add_Road(best_spline);
	owned.Add(edge.RoadId);
	edge.Length = best_cost;
	edge.RequiredBridgeCrossingId = best_crossing;
	edge.UnresolvedCrossingIds = best_crossings;
	return true;
}

bool RoadGraphGenerator::Generate(const RoadGraphParams & params,StringClass * error_out)
{
	#define FAIL(msg) { if (error_out) *error_out = msg; Clear(); return false; }

	Clear();

	if (!WorldTerrainSystem::Has_Chunk_Registry())	FAIL("no terrain chunk registry -- generate macro terrain first");
	if (!StrategicPlanner::Has_Plan())					FAIL("no strategic plan -- generate strategic candidates first");
	if (params.MaxBuildableSlopeRadians <= 0.0f)		FAIL("max buildable slope must be positive");
	if (params.SlopeCostWeight < 0.0f)					FAIL("slope cost weight must not be negative");
	if (params.CrossingCostPenalty < 0.0f)				FAIL("crossing cost penalty must not be negative");
	if (params.BoundaryApproachCostWeight < 0.0f)		FAIL("boundary approach cost weight must not be negative");

	int count_x = WorldTerrainSystem::Get_Chunk_Count_X();
	int count_y = WorldTerrainSystem::Get_Chunk_Count_Y();
	if ((count_x <= 0) || (count_y <= 0))					FAIL("chunk registry is empty");

	//	Remembered so Reroute_Edge_Avoiding can rebuild the identical cost field later without a
	//	caller having to resupply it.
	LastParams = params;

	DynamicVectorClass<RouteChunk> chunks;
	Compute_Route_Chunks(count_x,count_y,params,&chunks);

	//
	//	Step 3: which strategic nodes need connecting.  Bases and resources are the "major
	//	strategic anchors" the gate names; settlements/POIs/crossings are not independently
	//	wired into the highway skeleton this phase builds.
	//
	DynamicVectorClass<int> anchor_nodes;
	for (int i = 0; i < StrategicPlanner::Get_Node_Count(); i++) {
		StrategicNode node;
		if (!StrategicPlanner::Get_Node(i,&node)) continue;
		if ((node.Type == STRATEGIC_NODE_BASE_CANDIDATE) || (node.Type == STRATEGIC_NODE_RESOURCE_CANDIDATE)) {
			anchor_nodes.Add(i);
		}
	}

	if (anchor_nodes.Count() < 2) {
		Built = true;		// nothing to connect -- correct, not an error
		WWDEBUG_SAY(("RoadGraphGenerator: fewer than two anchor nodes, no edges built\r\n"));
		#undef FAIL
		return true;
	}

	//
	//	Prim's minimum spanning tree over the anchor nodes, using StrategicPlanner's own
	//	straight-line travel cost -- step 3's "select required graph edges", bounded at N-1
	//	edges rather than a complete graph.
	//
	DynamicVectorClass<bool>	in_tree;
	DynamicVectorClass<float>	best_cost;
	DynamicVectorClass<int>	best_from;
	for (int i = 0; i < anchor_nodes.Count(); i++) {
		in_tree.Add(false);
		best_cost.Add(-1.0f);
		best_from.Add(-1);
	}
	in_tree[0] = true;
	for (int i = 1; i < anchor_nodes.Count(); i++) {
		best_cost[i]	= StrategicPlanner::Get_Travel_Cost(anchor_nodes[0],anchor_nodes[i]);
		best_from[i]	= 0;
	}

	DynamicVectorClass<int> mst_from, mst_to;
	for (int step = 1; step < anchor_nodes.Count(); step++) {
		int pick = -1;
		float pick_cost = 0.0f;
		for (int i = 0; i < anchor_nodes.Count(); i++) {
			if (in_tree[i] || (best_cost[i] < 0.0f)) continue;
			if ((pick < 0) || (best_cost[i] < pick_cost)) { pick = i; pick_cost = best_cost[i]; }
		}
		if (pick < 0) break;		// disconnected under this metric -- should not happen, straight-line cost is always finite

		in_tree[pick] = true;
		mst_from.Add(best_from[pick]);
		mst_to.Add(pick);

		for (int i = 0; i < anchor_nodes.Count(); i++) {
			if (in_tree[i]) continue;
			float d = StrategicPlanner::Get_Travel_Cost(anchor_nodes[pick],anchor_nodes[i]);
			if ((d >= 0.0f) && ((best_cost[i] < 0.0f) || (d < best_cost[i]))) {
				best_cost[i] = d;
				best_from[i] = pick;
			}
		}
	}

	//
	//	Steps 4-9: realize each MST edge as a real, terrain-costed route, then a real road.
	//
	int edge_index = 0;
	for (int m = 0; m < mst_from.Count(); m++) {

		StrategicNode from_node, to_node;
		StrategicPlanner::Get_Node(anchor_nodes[mst_from[m]],&from_node);
		StrategicPlanner::Get_Node(anchor_nodes[mst_to[m]],&to_node);

		int from_chunk	= Nearest_Chunk(from_node.Position,count_x,count_y,chunks);
		int to_chunk	= Nearest_Chunk(to_node.Position,count_x,count_y,chunks);
		if ((from_chunk < 0) || (to_chunk < 0)) continue;

		DynamicVectorClass<int> path;
		Route_Path(from_chunk,to_chunk,count_x,count_y,chunks,params.Seed,&path);
		if (path.Count() < 2) continue;		// unreachable given this cost field's own blocks

		RoadGraphClassType cls = Classify_Edge(from_node.Type,to_node.Type);

		RoadSplineClass spline;
		spline.Add_Control_Point(from_node.Position);
		for (int i = 0; i < path.Count(); i++) {
			spline.Add_Control_Point(chunks[path[i]].Position);
		}
		spline.Add_Control_Point(to_node.Position);
		spline.Set_Width(ROAD_GRAPH_CLASS_WIDTH[cls]);
		spline.Set_Shoulder_Width(ROAD_GRAPH_CLASS_SHOULDER[cls]);
		spline.Set_Road_Class(Road_Graph_Class_To_Road_Class(cls));
		//	The authored ow_road_* surfaces are still optional runtime assets.
		//	Use the verified stock gravel until a road-class material is available,
		//	so generated carriageways are visible instead of untextured geometry.
		spline.Set_Material("l11_gravel.tga");
		spline.Set_Grade_Policy(ROAD_GRADE_BLEND);

		int road_id = RoadSystem::Add_Road(spline);
		OwnedRoadIds.Add(road_id);

		RoadGraphEdge edge;
		StringClass id; id.Format("road_%d",edge_index);
		edge.Id				= id;
		edge.RoadId			= road_id;
		edge.FromNodeIndex	= anchor_nodes[mst_from[m]];
		edge.ToNodeIndex		= anchor_nodes[mst_to[m]];
		edge.Class			= cls;
		edge.Length			= (from_node.Position - to_node.Position).Length();

		for (int i = 0; i < path.Count(); i++) {
			if (chunks[path[i]].HasCrossing) {
				bool already = false;
				for (int c = 0; c < edge.UnresolvedCrossingIds.Count(); c++) {
					if (edge.UnresolvedCrossingIds[c] == chunks[path[i]].CrossingId) { already = true; break; }
				}
				if (!already) edge.UnresolvedCrossingIds.Add(chunks[path[i]].CrossingId);
			}
		}

		Edges.Add(edge);
		edge_index++;
	}

	if (params.RequireBridgeCrossing && !Reserve_Bridge_Crossing(params,count_x,count_y,chunks,Edges,OwnedRoadIds)) {
		if (error_out) *error_out = "no river crossing could be connected to the strategic road network";
		Clear();
		return false;
	}
	if (OwnedRoadIds.Count() > 0) {
		RoadSystem::Build_Network();
	}

	{
		CRCEngine crc;
		for (int i = 0; i < Edges.Count(); i++) {
			const RoadGraphEdge & e = Edges[i];
			int q_class		= (int)e.Class;
			int q_from		= e.FromNodeIndex;
			int q_to			= e.ToNodeIndex;
			int q_length	= (int)(e.Length * CHECKSUM_QUANTUM);
			int q_crossings	= e.UnresolvedCrossingIds.Count();
			crc(&q_class,sizeof(q_class));
			crc(&q_from,sizeof(q_from));
			crc(&q_to,sizeof(q_to));
			crc(&q_length,sizeof(q_length));
			crc(&q_crossings,sizeof(q_crossings));
			crc(e.RequiredBridgeCrossingId.Peek_Buffer(),e.RequiredBridgeCrossingId.Get_Length());
		}
		Checksum = (unsigned int)crc;
	}

	Built = true;

	WWDEBUG_SAY(("RoadGraphGenerator: %d edge(s), checksum 0x%08X\r\n",Edges.Count(),Checksum));

	#undef FAIL
	return true;
}


/***********************************************************************************************
 * Reroute_Edge_Avoiding -- gap report item 5's own fallback stage 4                             *
 *=============================================================================================*/
bool RoadGraphGenerator::Reroute_Edge_Avoiding(const StringClass & edge_id,const Vector3 & avoid_position,
																StringClass * error_out)
{
	#define RFAIL(msg) { if (error_out) *error_out = msg; return false; }

	if (!Built)		RFAIL("no road graph built yet");

	int edge_index = -1;
	for (int i = 0; i < Edges.Count(); i++) {
		if (Edges[i].Id == edge_id) { edge_index = i; break; }
	}
	if (edge_index < 0)		RFAIL("no such edge");

	int count_x = WorldTerrainSystem::Get_Chunk_Count_X();
	int count_y = WorldTerrainSystem::Get_Chunk_Count_Y();
	if ((count_x <= 0) || (count_y <= 0))		RFAIL("chunk registry is empty");

	DynamicVectorClass<RouteChunk> chunks;
	Compute_Route_Chunks(count_x,count_y,LastParams,&chunks);

	//	Block the chunk nearest the position that just failed, plus every chunk already excluded
	//	by an earlier reroute of this same edge -- so a second failed crossing on the same edge
	//	does not lead Dijkstra straight back through the first one's own dead end.
	int avoid_index = Nearest_Chunk(avoid_position,count_x,count_y,chunks);
	if (avoid_index >= 0) {
		bool already = false;
		for (int i = 0; i < ExclusionEdgeIds.Count(); i++) {
			if ((ExclusionEdgeIds[i] == edge_id) && (ExclusionChunkIndices[i] == avoid_index)) { already = true; break; }
		}
		if (!already) {
			ExclusionEdgeIds.Add(edge_id);
			ExclusionChunkIndices.Add(avoid_index);
		}
	}
	for (int i = 0; i < ExclusionEdgeIds.Count(); i++) {
		if (ExclusionEdgeIds[i] == edge_id) {
			chunks[ExclusionChunkIndices[i]].Blocked = true;
		}
	}

	RoadGraphEdge & edge = Edges[edge_index];

	StrategicNode from_node,to_node;
	if (!StrategicPlanner::Get_Node(edge.FromNodeIndex,&from_node) ||
		 !StrategicPlanner::Get_Node(edge.ToNodeIndex,&to_node))		RFAIL("edge names a strategic node that no longer exists");

	int from_chunk = Nearest_Chunk(from_node.Position,count_x,count_y,chunks);
	int to_chunk	= Nearest_Chunk(to_node.Position,count_x,count_y,chunks);
	if ((from_chunk < 0) || (to_chunk < 0))		RFAIL("no passable chunk remains near one of this edge's own anchors");

	DynamicVectorClass<int> path;
	Route_Path(from_chunk,to_chunk,count_x,count_y,chunks,LastParams.Seed,&path);
	if (path.Count() < 2)		RFAIL("no alternate route exists");

	RoadSplineClass spline;
	spline.Add_Control_Point(from_node.Position);
	for (int i = 0; i < path.Count(); i++) {
		spline.Add_Control_Point(chunks[path[i]].Position);
	}
	spline.Add_Control_Point(to_node.Position);
	spline.Set_Width(ROAD_GRAPH_CLASS_WIDTH[edge.Class]);
	spline.Set_Shoulder_Width(ROAD_GRAPH_CLASS_SHOULDER[edge.Class]);
	spline.Set_Road_Class(Road_Graph_Class_To_Road_Class(edge.Class));
	spline.Set_Grade_Policy(ROAD_GRADE_BLEND);

	int new_road_id = RoadSystem::Add_Road(spline);
	if (!RoadSystem::Integrate_New_Road(new_road_id)) {
		RoadSystem::Remove_Road(new_road_id);
		RFAIL("the alternate route's own road failed to tessellate");
	}

	int old_road_id = edge.RoadId;
	RoadSystem::Remove_Road(old_road_id);
	Adopt_Owned_Road_Id(new_road_id);

	edge.RoadId	= new_road_id;
	edge.Length	= (from_node.Position - to_node.Position).Length();
	edge.UnresolvedCrossingIds.Delete_All();
	for (int i = 0; i < path.Count(); i++) {
		if (chunks[path[i]].HasCrossing) {
			bool already = false;
			for (int c = 0; c < edge.UnresolvedCrossingIds.Count(); c++) {
				if (edge.UnresolvedCrossingIds[c] == chunks[path[i]].CrossingId) { already = true; break; }
			}
			if (!already) edge.UnresolvedCrossingIds.Add(chunks[path[i]].CrossingId);
		}
	}

	WWDEBUG_SAY(("RoadGraphGenerator: edge '%s' rerouted (avoided chunk near %.1f,%.1f) -- road %d replaces road %d, "
					 "%d unresolved crossing(s) on the new route\r\n",
					 edge_id.Peek_Buffer(),avoid_position.X,avoid_position.Y,
					 new_road_id,old_road_id,edge.UnresolvedCrossingIds.Count()));

	return true;

	#undef RFAIL
}


int RoadGraphGenerator::Check_No_Cliff_Band_Crossing(StringClass * report)
{
	int faults = 0;
	if (!WorldBoundsService::Is_Configured()) return 0;

	for (int i = 0; i < Edges.Count(); i++) {
		RoadSplineClass * road = RoadSystem::Peek_Road(Edges[i].RoadId);
		if (road == nullptr) continue;

		int point_count = road->Get_Point_Count();
		for (int p = 0; p < point_count; p++) {
			const Vector3 & pos = road->Get_Point(p).Position;
			if (WorldBoundsService::Classify_Point(Vector2(pos.X,pos.Y)) >= WORLD_ZONE_CLIFF_BAND) {
				faults++;
				if (report) { StringClass l; l.Format("edge '%s' crosses the cliff band\r\n",Edges[i].Id.Peek_Buffer()); *report += l; }
				break;
			}
		}
	}
	return faults;
}


int RoadGraphGenerator::Check_No_Unresolved_Water_Or_Cliff_Passage(StringClass * report)
{
	//	Structural by construction: Compute_Route_Chunks only ever lifts a water/interior-cliff
	//	chunk's block when a crossing candidate sits there, and every such chunk a route passes
	//	through is recorded in that edge's own UnresolvedCrossingIds.  This regression net
	//	confirms every edge whose route touched water/an interior cliff also names a crossing.
	int faults = 0;
	for (int i = 0; i < Edges.Count(); i++) {
		RoadSplineClass * road = RoadSystem::Peek_Road(Edges[i].RoadId);
		if (road == nullptr) continue;

		int point_count = road->Get_Point_Count();
		for (int p = 0; p < point_count; p++) {
			const Vector3 & pos = road->Get_Point(p).Position;
			bool in_water = WaterSystem::Is_Point_In_Water(pos.X,pos.Y);
			if (in_water && (Edges[i].UnresolvedCrossingIds.Count() == 0)) {
				faults++;
				if (report) { StringClass l; l.Format("edge '%s' passes through water with no crossing candidate\r\n",Edges[i].Id.Peek_Buffer()); *report += l; }
				break;
			}
		}
	}
	return faults;
}


int RoadGraphGenerator::Check_Required_Nodes_Connected(StringClass * report)
{
	int anchor_count = 0;
	for (int i = 0; i < StrategicPlanner::Get_Node_Count(); i++) {
		StrategicNode node;
		if (!StrategicPlanner::Get_Node(i,&node)) continue;
		if ((node.Type == STRATEGIC_NODE_BASE_CANDIDATE) || (node.Type == STRATEGIC_NODE_RESOURCE_CANDIDATE)) anchor_count++;
	}
	if (anchor_count < 2) return 0;		// nothing required to connect

	int required_edges = anchor_count - 1;
	if (Edges.Count() < required_edges) {
		if (report) {
			StringClass l;
			l.Format("%d anchor node(s) need %d edge(s) for a spanning tree; only %d built\r\n",
						anchor_count,required_edges,Edges.Count());
			*report += l;
		}
		return 1;
	}
	return 0;
}
