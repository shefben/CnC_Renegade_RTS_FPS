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
 *                     $Archive:: /Commando/Code/Combat/generatedworldplacementrules.cpp      $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "generatedworldplacementrules.h"

#include "aabox.h"
#include "bridgeplanner.h"
#include "bridgesystem.h"
#include "crc.h"
#include "escarpment.h"
#include "navmeshgenerator.h"
#include "roadgraphgenerator.h"
#include "roadspline.h"
#include "roadsystem.h"
#include "strategicplanner.h"
#include "watersystem.h"
#include "worldboundsservice.h"
#include "worldterrainsystem.h"
#include "wwmath.h"


//	WWMath::Min/Max are float-only, and every index in this file is an int.  Rounding a chunk
//	index through a float to borrow them would be the wrong kind of reuse.
static inline int Int_Min(int a,int b)	{ return (a < b) ? a : b; }


bool				GeneratedWorldPlacementRules::ChokepointsBuilt		= false;
int				GeneratedWorldPlacementRules::ChokepointCount		= 0;
int				GeneratedWorldPlacementRules::ChokepointCountX	= 0;
int				GeneratedWorldPlacementRules::ChokepointCountY	= 0;
unsigned int	GeneratedWorldPlacementRules::ChokepointChecksum	= 0;

DynamicVectorClass<int>	GeneratedWorldPlacementRules::Chokepoint;


const char *	Generated_World_Exclusion_Name(GeneratedWorldExclusionType exclusion)
{
	switch (exclusion) {
		case GENERATED_WORLD_EXCLUSION_NONE:					return "none";
		case GENERATED_WORLD_EXCLUSION_BOUNDARY_CLIFF_BAND:	return "boundary cliff band";
		case GENERATED_WORLD_EXCLUSION_WATER:					return "water";
		case GENERATED_WORLD_EXCLUSION_IMMUTABLE_CLIFF:		return "immutable major cliff";
		case GENERATED_WORLD_EXCLUSION_TUNNEL:					return "tunnel portal";
		case GENERATED_WORLD_EXCLUSION_BRIDGE:					return "bridge structure or approach";
		case GENERATED_WORLD_EXCLUSION_RESOURCE_ROUTE:		return "protected resource route";
		case GENERATED_WORLD_EXCLUSION_NAV_CHOKEPOINT:		return "invalid nav chokepoint";
		default:														return "unknown";
	}
}


void GeneratedWorldPlacementRules::Init(void)
{
	Clear_Chokepoints();
}


void GeneratedWorldPlacementRules::Shutdown(void)
{
	Clear_Chokepoints();
}


void GeneratedWorldPlacementRules::Clear_Chokepoints(void)
{
	ChokepointsBuilt		= false;
	ChokepointCount		= 0;
	ChokepointCountX		= 0;
	ChokepointCountY		= 0;
	ChokepointChecksum	= 0;
	Chokepoint.Delete_All();
}


bool GeneratedWorldPlacementRules::Has_Generated_World(void)
{
	//	Any one of these carrying data means something below can exclude something.  The chunk
	//	registry is listed first because every other generated-world owner in this chain is
	//	built from it, so it is the usual answer.
	if (WorldTerrainSystem::Has_Chunk_Registry())	return true;
	if (WaterSystem::Get_Area_Count() > 0)				return true;
	if (EscarpmentSystem::Get_Escarpment_Count() > 0)	return true;
	if (BridgePlanner::Has_Plan())						return true;
	if (StrategicPlanner::Has_Plan())					return true;
	if (NavMeshGenerator::Has_Navigation())			return true;
	return false;
}


/***********************************************************************************************
**	World point -> chunk index, from the chunk registry's own patch bounds.
**
**	Derived from patch (0,0) rather than from a stored cell size, because the heightfield owns
**	that number and a copy of it here is one more thing that can fall out of step with the
**	terrain.  A patch is axis-aligned and uniform, which is what makes one patch enough.
*/
bool GeneratedWorldPlacementRules::World_To_Chunk(float x,float y,int * px_out,int * py_out)
{
	if (!WorldTerrainSystem::Has_Chunk_Registry()) return false;

	int count_x = WorldTerrainSystem::Get_Chunk_Count_X();
	int count_y = WorldTerrainSystem::Get_Chunk_Count_Y();
	if ((count_x <= 0) || (count_y <= 0)) return false;

	AABoxClass first;
	if (!WorldTerrainSystem::Get_Patch_Bounds(0,0,&first)) return false;

	float size_x = first.Extent.X * 2.0f;
	float size_y = first.Extent.Y * 2.0f;
	if ((size_x <= 0.0f) || (size_y <= 0.0f)) return false;

	float origin_x = first.Center.X - first.Extent.X;
	float origin_y = first.Center.Y - first.Extent.Y;

	int px = (int)WWMath::Floor((x - origin_x) / size_x);
	int py = (int)WWMath::Floor((y - origin_y) / size_y);

	if ((px < 0) || (py < 0) || (px >= count_x) || (py >= count_y)) return false;

	if (px_out != nullptr) *px_out = px;
	if (py_out != nullptr) *py_out = py;
	return true;
}


bool GeneratedWorldPlacementRules::Is_Chunk_Chokepoint(int px,int py)
{
	if (!ChokepointsBuilt) return false;
	if ((px < 0) || (py < 0) || (px >= ChokepointCountX) || (py >= ChokepointCountY)) return false;
	return Chokepoint[px + (py * ChokepointCountX)] != 0;
}


/***********************************************************************************************
**	Articulation points of the 4-connected navigable grid -- Hopcroft/Tarjan, iterative.
**
**	Iterative rather than recursive on purpose: the recursion depth of this search is the length
**	of the longest simple path through the walkable world, which on a large open map is every
**	chunk in it.  A generated world is allowed to be large, so the search carries its own stack.
*/
bool GeneratedWorldPlacementRules::Find_Articulation_Points(const DynamicVectorClass<int> & navigable,
																				int count_x,int count_y,
																				DynamicVectorClass<int> * out_articulation)
{
	if (out_articulation == nullptr) return false;
	if ((count_x <= 0) || (count_y <= 0)) return false;

	int total = count_x * count_y;
	if (navigable.Count() != total) return false;

	out_articulation->Delete_All();
	out_articulation->Resize(total);
	for (int i = 0; i < total; i++) {
		out_articulation->Add(0);
	}

	DynamicVectorClass<int> disc;		// 0 = unvisited, otherwise the discovery time
	DynamicVectorClass<int> low;
	DynamicVectorClass<int> parent;
	DynamicVectorClass<int> next_edge;	// which of the four neighbours this node will try next

	disc.Resize(total);
	low.Resize(total);
	parent.Resize(total);
	next_edge.Resize(total);
	for (int i = 0; i < total; i++) {
		disc.Add(0);
		low.Add(0);
		parent.Add(-1);
		next_edge.Add(0);
	}

	//	The search's own stack, as storage plus a top index.  DynamicVectorClass<int>::Delete is
	//	ambiguous for T == int (Delete(int index) and Delete(T const &) are the same call), so a
	//	pop is an index decrement here rather than a Delete.
	DynamicVectorClass<int> stack;
	stack.Resize(total);
	for (int i = 0; i < total; i++) {
		stack.Add(0);
	}
	int stack_top = 0;

	int timer = 0;

	for (int root = 0; root < total; root++) {

		if (navigable[root] == 0) continue;
		if (disc[root] != 0) continue;

		timer++;
		disc[root]	= timer;
		low[root]	= timer;
		parent[root]	= -1;
		next_edge[root]	= 0;

		int root_children = 0;

		stack_top			= 0;
		stack[stack_top]	= root;
		stack_top++;

		while (stack_top > 0) {

			int u = stack[stack_top - 1];

			if (next_edge[u] < 4) {

				int edge = next_edge[u];
				next_edge[u] = edge + 1;

				int ux = u % count_x;
				int uy = u / count_x;

				int vx = ux, vy = uy;
				switch (edge) {
					case 0:	vx = ux + 1;	break;
					case 1:	vx = ux - 1;	break;
					case 2:	vy = uy + 1;	break;
					default:	vy = uy - 1;	break;
				}

				if ((vx < 0) || (vy < 0) || (vx >= count_x) || (vy >= count_y)) continue;

				int v = vx + (vy * count_x);
				if (navigable[v] == 0) continue;
				if (v == parent[u]) continue;

				if (disc[v] == 0) {
					timer++;
					disc[v]			= timer;
					low[v]			= timer;
					parent[v]		= u;
					next_edge[v]	= 0;
					stack[stack_top]	= v;
					stack_top++;
				} else {
					//	Back edge.
					low[u] = Int_Min(low[u],disc[v]);
				}

			} else {

				stack_top--;

				int p = parent[u];
				if (p >= 0) {
					low[p] = Int_Min(low[p],low[u]);

					if (p == root) {
						root_children++;
					} else if (low[u] >= disc[p]) {
						(*out_articulation)[p] = 1;
					}
				}
			}
		}

		//	A root is an articulation point exactly when it has more than one DFS child: its
		//	subtrees are otherwise unreachable from each other.
		if (root_children > 1) {
			(*out_articulation)[root] = 1;
		}
	}

	return true;
}


bool GeneratedWorldPlacementRules::Build_Chokepoints(StringClass * error_out)
{
	#define FAIL(msg) { if (error_out) *error_out = msg; Clear_Chokepoints(); return false; }

	Clear_Chokepoints();

	if (!NavMeshGenerator::Has_Navigation()) FAIL("no navigation built -- generate the nav mesh first");

	int count_x = NavMeshGenerator::Get_Chunk_Count_X();
	int count_y = NavMeshGenerator::Get_Chunk_Count_Y();
	if ((count_x <= 0) || (count_y <= 0)) FAIL("navigation reports an empty chunk grid");

	DynamicVectorClass<int> navigable;
	navigable.Resize(count_x * count_y);
	for (int py = 0; py < count_y; py++) {
		for (int px = 0; px < count_x; px++) {
			navigable.Add(NavMeshGenerator::Is_Chunk_Navigable(px,py) ? 1 : 0);
		}
	}

	DynamicVectorClass<int> articulation;
	if (!Find_Articulation_Points(navigable,count_x,count_y,&articulation)) {
		FAIL("the chokepoint analysis refused the navigable grid it was given");
	}

	int chokepoint_count = 0;
	for (int i = 0; i < articulation.Count(); i++) {
		if (articulation[i] != 0) chokepoint_count++;
	}

	{
		CRCEngine crc;
		crc(&count_x,sizeof(count_x));
		crc(&count_y,sizeof(count_y));
		crc(&chokepoint_count,sizeof(chokepoint_count));
		for (int i = 0; i < articulation.Count(); i++) {
			int flag = articulation[i];
			crc(&flag,sizeof(flag));
		}
		ChokepointChecksum = (unsigned int)crc;
	}

	Chokepoint.Delete_All();
	Chokepoint.Resize(articulation.Count());
	for (int i = 0; i < articulation.Count(); i++) {
		Chokepoint.Add(articulation[i]);
	}

	ChokepointCountX	= count_x;
	ChokepointCountY	= count_y;
	ChokepointCount	= chokepoint_count;
	ChokepointsBuilt	= true;

	#undef FAIL
	return true;
}


/***********************************************************************************************
**	The exact connectivity test.  Remove every navigable chunk the footprint covers, flood fill
**	from one surviving navigable chunk, and see whether every other survivor was reached.
*/
bool GeneratedWorldPlacementRules::Would_Disconnect_Navigation(const Vector3 & position,float radius)
{
	if (!NavMeshGenerator::Has_Navigation()) return false;

	int count_x = NavMeshGenerator::Get_Chunk_Count_X();
	int count_y = NavMeshGenerator::Get_Chunk_Count_Y();
	if ((count_x <= 0) || (count_y <= 0)) return false;

	int total = count_x * count_y;

	DynamicVectorClass<int> remaining;
	remaining.Resize(total);
	for (int i = 0; i < total; i++) {
		remaining.Add(0);
	}

	int covered = 0;
	int survivors = 0;
	int seed = -1;

	float footprint_radius = (radius > 0.0f) ? radius : 0.0f;

	for (int py = 0; py < count_y; py++) {
		for (int px = 0; px < count_x; px++) {

			int slot = px + (py * count_x);
			if (!NavMeshGenerator::Is_Chunk_Navigable(px,py)) continue;

			AABoxClass bounds;
			if (!WorldTerrainSystem::Get_Patch_Bounds(px,py,&bounds)) continue;

			//	Footprint circle against the chunk's own axis-aligned plan rectangle -- the
			//	nearest point on the rectangle, compared against the radius.
			float nearest_x = WWMath::Clamp(position.X,bounds.Center.X - bounds.Extent.X,
																 bounds.Center.X + bounds.Extent.X);
			float nearest_y = WWMath::Clamp(position.Y,bounds.Center.Y - bounds.Extent.Y,
																 bounds.Center.Y + bounds.Extent.Y);
			float dx = position.X - nearest_x;
			float dy = position.Y - nearest_y;

			if ((dx * dx + dy * dy) <= (footprint_radius * footprint_radius)) {
				covered++;
				continue;								// removed by the placement
			}

			remaining[slot] = 1;
			survivors++;
			if (seed < 0) seed = slot;
		}
	}

	//	Nothing was taken away, or nothing (or one thing) is left: neither can split a world.
	if (covered == 0)		return false;
	if (survivors <= 1)	return false;

	DynamicVectorClass<int> queue;
	queue.Add(seed);
	remaining[seed] = 2;					// 2 == reached
	int reached = 1;

	int head = 0;
	while (head < queue.Count()) {

		int u = queue[head];
		head++;

		int ux = u % count_x;
		int uy = u / count_x;

		for (int edge = 0; edge < 4; edge++) {

			int vx = ux, vy = uy;
			switch (edge) {
				case 0:	vx = ux + 1;	break;
				case 1:	vx = ux - 1;	break;
				case 2:	vy = uy + 1;	break;
				default:	vy = uy - 1;	break;
			}

			if ((vx < 0) || (vy < 0) || (vx >= count_x) || (vy >= count_y)) continue;

			int v = vx + (vy * count_x);
			if (remaining[v] != 1) continue;

			remaining[v] = 2;
			reached++;
			queue.Add(v);
		}
	}

	return reached < survivors;
}


/***********************************************************************************************
**	Bridge structure and approach.
**
**	Two questions, one answer.  The deck is BridgeSystem's own Find_Bridge_At.  The approach is
**	the ground within one bridge-width of either abutment, which is the one distance the bridge
**	itself states -- a wider crossing needs a longer clear run onto it, and nothing else in this
**	tree names an approach length.
*/
static bool Is_On_Bridge_Or_Approach(const Vector3 & position,float radius)
{
	if (BridgeSystem::Find_Bridge_At(position) >= 0) return true;

	if (!BridgePlanner::Has_Plan()) return false;

	for (int b = 0; b < BridgePlanner::Get_Bridge_Count(); b++) {

		ResolvedBridge resolved;
		if (!BridgePlanner::Get_Bridge(b,&resolved)) continue;

		BridgeClass * bridge = BridgeSystem::Peek_Bridge(resolved.BridgeId);
		if (bridge == nullptr) continue;

		float width = bridge->Get_Width();
		if (width <= 0.0f) continue;

		float reach = width + radius;

		const Vector3 & start = bridge->Get_Start();
		const Vector3 & end	= bridge->Get_End();

		float dx1 = position.X - start.X;
		float dy1 = position.Y - start.Y;
		if ((dx1 * dx1 + dy1 * dy1) <= (reach * reach)) return true;

		float dx2 = position.X - end.X;
		float dy2 = position.Y - end.Y;
		if ((dx2 * dx2 + dy2 * dy2) <= (reach * reach)) return true;
	}

	return false;
}


/***********************************************************************************************
**	Protected resource route.
**
**	Not every road: the roads that carry an economy.  A road-graph edge with a
**	STRATEGIC_NODE_RESOURCE_CANDIDATE at either end is the route a Harvester will run, and
**	Section 17A's own fairness thresholds are stated in terms of resource travel time -- a
**	building dropped across one of those roads would move that number after the world was
**	already accepted as fair.  Every other road may be built over.
**
**	The corridor is the road spline's own carriageway half-width plus the footprint radius.
*/
static bool Is_On_Protected_Resource_Route(const Vector3 & position,float radius)
{
	if (!RoadGraphGenerator::Has_Graph()) return false;
	if (RoadSystem::Get_Road_Count() <= 0) return false;

	/*
	**	One query, widened by the widest carriageway any road in this world actually has, so that
	**	no road is missed because the search stopped short of its own half-width.  Read off the
	**	splines rather than named as a constant here: the road generator owns that number.
	*/
	float widest_half_width = 0.0f;
	for (int r = 0; r < RoadSystem::Get_Road_Count(); r++) {
		RoadSplineClass * spline = RoadSystem::Peek_Road(r);
		if (spline == nullptr) continue;
		widest_half_width = WWMath::Max(widest_half_width,spline->Get_Width() * 0.5f);
	}

	RoadNavInfoClass info;
	if (!RoadSystem::Find_Nearest_Road(position,radius + widest_half_width,&info)) return false;
	if (info.RoadID < 0) return false;

	RoadSplineClass * road = RoadSystem::Peek_Road(info.RoadID);
	if (road == nullptr) return false;

	float half_width = road->Get_Width() * 0.5f;
	if (info.LateralDistance > (half_width + radius)) return false;

	for (int e = 0; e < RoadGraphGenerator::Get_Edge_Count(); e++) {

		RoadGraphEdge edge;
		if (!RoadGraphGenerator::Get_Edge(e,&edge)) continue;
		if (edge.RoadId != info.RoadID) continue;

		StrategicNode from_node, to_node;
		if (StrategicPlanner::Get_Node(edge.FromNodeIndex,&from_node) &&
			 (from_node.Type == STRATEGIC_NODE_RESOURCE_CANDIDATE)) {
			return true;
		}
		if (StrategicPlanner::Get_Node(edge.ToNodeIndex,&to_node) &&
			 (to_node.Type == STRATEGIC_NODE_RESOURCE_CANDIDATE)) {
			return true;
		}
	}

	return false;
}


GeneratedWorldExclusionType	GeneratedWorldPlacementRules::Classify(const Vector3 & position,float radius)
{
	GeneratedWorldExclusionType feature = Classify_Static_Features(position,radius);
	if (feature != GENERATED_WORLD_EXCLUSION_NONE) return feature;

	if (Would_Disconnect_Navigation(position,(radius > 0.0f) ? radius : 0.0f)) {
		return GENERATED_WORLD_EXCLUSION_NAV_CHOKEPOINT;
	}

	return GENERATED_WORLD_EXCLUSION_NONE;
}


GeneratedWorldExclusionType	GeneratedWorldPlacementRules::Classify_Static_Features(const Vector3 & position,
																												float radius)
{
	//	The most structural true thing about this position: the lowest set bit, in the order the
	//	enum declares, which is the order Phase 17 step 2 names the exclusions in.
	unsigned int mask = Get_Static_Feature_Mask(position,radius);
	if (mask == 0) return GENERATED_WORLD_EXCLUSION_NONE;

	for (int i = 1; i < GENERATED_WORLD_EXCLUSION_COUNT; i++) {
		if ((mask & (1u << i)) != 0) return (GeneratedWorldExclusionType)i;
	}
	return GENERATED_WORLD_EXCLUSION_NONE;
}


unsigned int	GeneratedWorldPlacementRules::Get_Static_Feature_Mask(const Vector3 & position,float radius)
{
	unsigned int mask = 0;
	float footprint_radius = (radius > 0.0f) ? radius : 0.0f;

	/*
	**	Boundary cliff band.  Asked of the bounds service where one is configured -- the same
	**	call CommanderPlacementValidator already makes, kept here as well so that this file's own
	**	answer is complete rather than depending on its caller having asked first -- and of the
	**	chunk registry's own CliffBand flag, which is what a generated world actually marks.
	*/
	if (WorldBoundsService::Is_Configured() &&
		 !WorldBoundsService::Can_Place_In_Region(Vector2(position.X,position.Y),footprint_radius)) {
		mask |= (1u << GENERATED_WORLD_EXCLUSION_BOUNDARY_CLIFF_BAND);
	}

	int px = 0, py = 0;
	bool on_chunk = World_To_Chunk(position.X,position.Y,&px,&py);

	if (on_chunk) {
		TerrainChunkInfoClass info;
		if (WorldTerrainSystem::Get_Chunk_Info(px,py,&info) && info.CliffBand) {
			mask |= (1u << GENERATED_WORLD_EXCLUSION_BOUNDARY_CLIFF_BAND);
		}
	}

	//	Water.  The authored shapes, not the physics scene -- WaterSystem answers headless, which
	//	is what a placement validator running on a dedicated server needs.
	if (WaterSystem::Get_Area_Count() > 0) {
		if (WaterSystem::Is_Point_In_Water(position.X,position.Y)) {
			mask |= (1u << GENERATED_WORLD_EXCLUSION_WATER);
		}
		//	The footprint's own extent, four cardinal points out.  A disc test against every
		//	water area's polygon is what Find_Area_At already does per point; sampling the rim is
		//	how a corner that overhangs a bank is caught without a second polygon routine here.
		if (footprint_radius > 0.0f) {
			if (WaterSystem::Is_Point_In_Water(position.X + footprint_radius,position.Y) ||
				 WaterSystem::Is_Point_In_Water(position.X - footprint_radius,position.Y) ||
				 WaterSystem::Is_Point_In_Water(position.X,position.Y + footprint_radius) ||
				 WaterSystem::Is_Point_In_Water(position.X,position.Y - footprint_radius)) {
				mask |= (1u << GENERATED_WORLD_EXCLUSION_WATER);
			}
		}
	}

	/*
	**	Immutable major cliff.  The chunk registry's InteriorCliff flag is the canonical marker
	**	-- worldterrainsystem.h says outright that such a chunk "is never buildable, same rule as
	**	CliffBand" -- and EscarpmentSystem::Sample_Face is the sub-chunk answer for a footprint
	**	whose centre sits on a clear chunk but whose edge reaches a face.
	*/
	if (on_chunk) {
		TerrainChunkInfoClass info;
		if (WorldTerrainSystem::Get_Chunk_Info(px,py,&info) && info.InteriorCliff) {
			mask |= (1u << GENERATED_WORLD_EXCLUSION_IMMUTABLE_CLIFF);
		}
	}

	if (EscarpmentSystem::Get_Escarpment_Count() > 0) {
		float upper = 0.0f, lower = 0.0f;
		if (EscarpmentSystem::Sample_Face(position.X,position.Y,footprint_radius,&upper,&lower)) {
			mask |= (1u << GENERATED_WORLD_EXCLUSION_IMMUTABLE_CLIFF);
		}
	}

	/*
	**	Tunnel portal.  See this file's header: no tunnel system exists in this tree, so the only
	**	real tunnel data anywhere is the strategic plan's own candidate positions.  The ground a
	**	tunnel mouth was planned on is excluded; there is no interior to exclude.
	*/
	if (StrategicPlanner::Has_Plan()) {
		for (int n = 0; n < StrategicPlanner::Get_Node_Count(); n++) {
			StrategicNode node;
			if (!StrategicPlanner::Get_Node(n,&node)) continue;
			if (node.Type != STRATEGIC_NODE_TUNNEL_CANDIDATE) continue;

			float dx = position.X - node.Position.X;
			float dy = position.Y - node.Position.Y;
			if ((dx * dx + dy * dy) <= (footprint_radius * footprint_radius)) {
				mask |= (1u << GENERATED_WORLD_EXCLUSION_TUNNEL);
				break;
			}
		}
	}

	if (Is_On_Bridge_Or_Approach(position,footprint_radius)) {
		mask |= (1u << GENERATED_WORLD_EXCLUSION_BRIDGE);
	}

	if (Is_On_Protected_Resource_Route(position,footprint_radius)) {
		mask |= (1u << GENERATED_WORLD_EXCLUSION_RESOURCE_ROUTE);
	}

	return mask;
}
