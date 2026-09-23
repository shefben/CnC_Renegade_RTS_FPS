/*
**	OpenW3D
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
 *                 Project Name : OpenW3D                                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/worldtraversability.cpp               $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   WorldTraversabilityGraph::Build_Nodes -- one cell at a time, can this class stand here     *
 *   WorldTraversabilityGraph::Build_Ground_Edges -- can it get to the next cell                *
 *   WorldTraversabilityGraph::Build_Crossing_Edges -- bridges, by what they say they carry     *
 *   WorldTraversabilityGraph::Find_Chokepoints -- the edges no alternate route covers          *
 *   WorldTraversabilityGraph::Count_Distinct_Routes -- edge-disjoint, so detours do not count  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "worldtraversability.h"

#include "worldmetrics.h"

#include "bridgesystem.h"
#include "bridgesection.h"
#include "escarpment.h"
#include "tunnelsystem.h"
#include "heightfield.h"
#include "watersystem.h"
#include "worldterrainsystem.h"

#include "aabox.h"
#include "wwdebug.h"
#include "wwmath.h"


/*
**	An upper bound on how many cells a graph may hold.  A coarse graph over a large map is a
**	few tens of thousands of cells; a hundred thousand means the caller asked for a cell size
**	that makes this a second pathfinding grid, which is exactly what Phase 65 says it is not.
**	Refusing loudly is better than quietly building something nobody intended.
*/
static const int MAX_GRAPH_NODES = 262144;

/*
**	Floor on cell size, for the same reason.
*/
static const float MIN_CELL_SIZE = 1.0f;

/*
**	How much of a cell's span a bridge abutment has to be within to count as arriving in it.
*/
static const float BRIDGE_SNAP_CELLS = 1.5f;


static DynamicVectorClass<TraversabilityNodeStruct>	_Nodes;
static DynamicVectorClass<TraversabilityEdgeStruct>	_Edges;

//	Per-node edge lists, flattened: _Adjacency holds edge indices, _AdjacencyStart[i] and
//	_AdjacencyCount[i] the range belonging to node i.  Built once after the edges are, because
//	every query walks neighbours and none of them should be walking the whole edge list.
static DynamicVectorClass<int>	_Adjacency;
static DynamicVectorClass<int>	_AdjacencyStart;
static DynamicVectorClass<int>	_AdjacencyCount;

//	Grid index: cell (gx,gy) to node index, or -1.  A dense map, because the grid is dense and
//	a hash of it would be slower and larger.
static DynamicVectorClass<int>	_GridToNode;

static int		_GridX			= 0;
static int		_GridY			= 0;
static float	_CellSize		= 0.0f;
static Vector3	_Origin(0.0f,0.0f,0.0f);
static int		_ComponentCount	= 0;
static bool		_Built			= false;
static WorldLocomotionClassType	_Loco = LOCOMOTION_INVALID;


/***********************************************************************************************
 * WorldTraversabilityGraph::Can_Step --                                                       *
 *                                                                                             *
 * Either the rise between two cells is small enough to be stepped over, or the slope it makes *
 * across the gap is shallow enough to walk up.  Both, because they are different facts: a     *
 * kerb is a small rise at an infinite slope, and a hillside is a large rise at a gentle one.  *
 *=============================================================================================*/
bool WorldTraversabilityGraph::Can_Step(float height_difference,float horizontal_distance,
													 float step_height,float max_climbable_slope)
{
	float rise = WWMath::Fabs(height_difference);

	if (rise <= step_height) {
		return true;
	}
	if (horizontal_distance <= WWMATH_EPSILON) {
		return false;
	}
	if (max_climbable_slope <= 0.0f) {
		return false;
	}

	float slope = atanf(rise / horizontal_distance);
	return (slope <= max_climbable_slope);
}


/***********************************************************************************************
 * WorldTraversabilityGraph::Crossing_Carries --                                               *
 *                                                                                             *
 * Phase 65's acceptance line lives here.  A crossing states a width and a headroom; a body    *
 * has a width and a height.  A bridge that states nothing carries everything, because every   *
 * bridge definition that predates Phase 64 states nothing and none of them should start       *
 * refusing traffic they have always carried.                                                  *
 *=============================================================================================*/
bool WorldTraversabilityGraph::Crossing_Carries(float crossing_width,float crossing_clearance,
																const WorldMetricsStruct & metrics)
{
	if (crossing_width > 0.0f) {
		if (crossing_width < metrics.CorridorWidth) {
			return false;
		}
	}

	if (crossing_clearance > 0.0f) {
		if (crossing_clearance < metrics.OverheadClearance) {
			return false;
		}
	}

	return true;
}


/***********************************************************************************************
 * WorldTraversabilityGraph::Compute_Wade_Depth --                                             *
 *=============================================================================================*/
float WorldTraversabilityGraph::Compute_Wade_Depth(WorldLocomotionClassType loco,
																	const WorldMetricsStruct & metrics)
{
	if (loco == LOCOMOTION_FLYING) {
		//	It is not in the water at all.
		return 1000000.0f;
	}
	if (loco != LOCOMOTION_INFANTRY) {
		//	A ground vehicle in this engine does not ford anything.  A river is a wall to it,
		//	and the bridge over it is the only reason the far bank is in its graph.
		return 0.0f;
	}
	//	Waist-deep.  Past that a soldier is swimming, which is a different locomotor.
	return metrics.StandHeight * 0.5f;
}


/***********************************************************************************************
 * WorldTraversabilityGraph::Node_At --                                                        *
 *=============================================================================================*/
int WorldTraversabilityGraph::Node_At(int gx,int gy)
{
	if ((gx < 0) || (gy < 0) || (gx >= _GridX) || (gy >= _GridY)) {
		return -1;
	}
	return _GridToNode[(gy * _GridX) + gx];
}


/***********************************************************************************************
 * WorldTraversabilityGraph::Build_Nodes -- one cell at a time, can this class stand here       *
 *                                                                                             *
 * Four tests, in the order that rejects most cheaply: is the cell on the field at all, is the  *
 * ground under it flat enough for a body of this radius, is it under more water than this      *
 * class can wade, and is it standing on a cliff face.                                          *
 *                                                                                             *
 * Query_Placement_Surface is the corridor-width test: it samples a disc of the body's own      *
 * radius rather than a point, so a cell whose centre is flat but whose surroundings are not    *
 * is correctly refused.  The radius comes from WorldMetricsClass, which is the whole reason    *
 * Phase 63 gates this one.                                                                     *
 *=============================================================================================*/
bool WorldTraversabilityGraph::Build_Nodes(const WorldMetricsStruct & metrics)
{
	AABoxClass bounds;
	if (!WorldTerrainSystem::Get_Bounds(&bounds)) {
		return false;
	}

	_Origin.Set(bounds.Center.X - bounds.Extent.X,bounds.Center.Y - bounds.Extent.Y,0.0f);

	float span_x = bounds.Extent.X * 2.0f;
	float span_y = bounds.Extent.Y * 2.0f;

	_GridX = (int)(span_x / _CellSize);
	_GridY = (int)(span_y / _CellSize);
	if (_GridX < 1) { _GridX = 1; }
	if (_GridY < 1) { _GridY = 1; }

	if ((_GridX * _GridY) > MAX_GRAPH_NODES) {
		WWDEBUG_SAY(("WorldTraversabilityGraph: %d x %d cells at %.2fm exceeds the %d cell cap; "
						 "ask for a larger cell size.\r\n",_GridX,_GridY,_CellSize,MAX_GRAPH_NODES));
		return false;
	}

	_GridToNode.Resize(_GridX * _GridY);
	for (int i = 0; i < (_GridX * _GridY); i++) {
		_GridToNode.Add(-1);
	}

	float radius = metrics.CollisionRadius;
	if (radius < WWMATH_EPSILON) {
		radius = _CellSize * 0.5f;
	}
	float wade_depth = Compute_Wade_Depth(_Loco,metrics);

	for (int gy = 0; gy < _GridY; gy++) {
		for (int gx = 0; gx < _GridX; gx++) {

			float x = _Origin.X + ((gx + 0.5f) * _CellSize);
			float y = _Origin.Y + ((gy + 0.5f) * _CellSize);

			TerrainPlacementInfoClass info;
			if (!WorldTerrainSystem::Query_Placement_Surface(Vector3(x,y,0.0f),radius,&info)) {
				continue;
			}
			if (!info.IsOnTerrain) {
				continue;
			}

			//	A cell this class cannot stand on because the ground under it is too steep is
			//	not a node, which is what makes a cliff top and a cliff bottom two places
			//	rather than one.  A flyer skips the test: a slope is not an obstacle to it.
			if (_Loco != LOCOMOTION_FLYING) {
				if (info.MaxSlope > metrics.MaxClimbableSlope) {
					continue;
				}

				float depth = 0.0f;
				if (WaterSystem::Get_Water_Depth(x,y,info.AverageHeight,&depth)) {
					if (depth > wade_depth) {
						continue;
					}
				}
			}

			TraversabilityNodeStruct node;
			node.GridX = gx;
			node.GridY = gy;
			node.Position.Set(x,y,info.AverageHeight);
			node.Passable = true;
			node.Component = -1;

			_GridToNode[(gy * _GridX) + gx] = _Nodes.Count();
			_Nodes.Add(node);
		}
	}

	return (_Nodes.Count() > 0);
}


/***********************************************************************************************
 * WorldTraversabilityGraph::Add_Edge --                                                       *
 *=============================================================================================*/
void WorldTraversabilityGraph::Add_Edge(int a,int b,int kind,float width,int source_id)
{
	if ((a < 0) || (b < 0) || (a == b)) {
		return;
	}

	TraversabilityEdgeStruct edge;
	edge.NodeA = (a < b) ? a : b;
	edge.NodeB = (a < b) ? b : a;
	edge.Kind = kind;
	edge.Width = width;
	edge.SourceID = source_id;
	_Edges.Add(edge);
}


/***********************************************************************************************
 * WorldTraversabilityGraph::Build_Ground_Edges -- can it get to the next cell                  *
 *                                                                                             *
 * Only east and north, so each pair is considered once and the graph comes out undirected      *
 * without duplicates.  Diagonals are deliberately not edges: a coarse graph that lets a body   *
 * slip diagonally between two blocked cells reports routes through walls.                      *
 *=============================================================================================*/
void WorldTraversabilityGraph::Build_Ground_Edges(const WorldMetricsStruct & metrics)
{
	for (int i = 0; i < _Nodes.Count(); i++) {

		const TraversabilityNodeStruct & node = _Nodes[i];

		for (int dir = 0; dir < 2; dir++) {

			int nx = node.GridX + ((dir == 0) ? 1 : 0);
			int ny = node.GridY + ((dir == 1) ? 1 : 0);

			int other = Node_At(nx,ny);
			if (other < 0) {
				continue;
			}

			const TraversabilityNodeStruct & neighbour = _Nodes[other];

			if (_Loco != LOCOMOTION_FLYING) {

				float rise = neighbour.Position.Z - node.Position.Z;
				if (!Can_Step(rise,_CellSize,metrics.StepHeight,metrics.MaxClimbableSlope)) {
					continue;
				}

				//	Phase 64's faces.  A cliff between two cells this class cannot scale is a
				//	wall, and a wall is the absence of an edge -- not an expensive edge.
				if (EscarpmentSystem::Is_Segment_Blocked(node.Position,neighbour.Position,
																	  metrics.StepHeight)) {
					continue;
				}
			}

			Add_Edge(i,other,TRAVERSE_EDGE_GROUND,_CellSize,-1);
		}
	}
}


/***********************************************************************************************
 * WorldTraversabilityGraph::Build_Crossing_Edges -- bridges, by what they say they carry       *
 *                                                                                             *
 * A bridge joins the cell at one abutment to the cell at the other, once, regardless of how    *
 * long it is -- the graph is about connectivity, and everything in between is deck.  Whether   *
 * the edge exists at all is decided by what the crossing states about itself, which is why     *
 * Phase 64 had to give a bridge a clearance to state before this phase could ask.              *
 *=============================================================================================*/
void WorldTraversabilityGraph::Build_Crossing_Edges(const WorldMetricsStruct & metrics)
{
	float snap = _CellSize * BRIDGE_SNAP_CELLS;

	for (int b = 0; b < BridgeSystem::Get_Bridge_Count(); b++) {

		BridgeClass * bridge = BridgeSystem::Peek_Bridge(b);
		if (bridge == nullptr) {
			continue;
		}

		//	A broken span carries nothing.  This is the one piece of live state the graph
		//	reads, and it is read at build time: a bridge that breaks mid-match invalidates
		//	the graph, which the caller rebuilds if it cares.
		if (!BridgeSystem::Is_Traversable(bridge->Get_ID())) {
			continue;
		}

		BridgeDefinitionClass * definition = BridgeSystem::Find_Definition(bridge->Get_Definition());

		float width = bridge->Get_Width();
		float clearance = 0.0f;
		if (definition != nullptr) {
			if (width <= 0.0f) {
				width = definition->Get_Width();
			}
			clearance = definition->Get_Required_Clearance();
		}

		if (!Crossing_Carries(width,clearance,metrics)) {
			continue;
		}

		int start_node = Find_Node(bridge->Get_Start());
		int end_node = Find_Node(bridge->Get_End());

		//	An abutment landing on a cell this class cannot stand in -- the bank is too steep
		//	for it -- is searched outward a little, because a bridge end is a point and a cell
		//	is a square.
		if ((start_node < 0) || (end_node < 0)) {

			int reach = (int)(snap / _CellSize);
			if (reach < 1) { reach = 1; }

			if (start_node < 0) {
				start_node = -1;
				int gx = (int)((bridge->Get_Start().X - _Origin.X) / _CellSize);
				int gy = (int)((bridge->Get_Start().Y - _Origin.Y) / _CellSize);
				for (int dy = -reach; (dy <= reach) && (start_node < 0); dy++) {
					for (int dx = -reach; (dx <= reach) && (start_node < 0); dx++) {
						start_node = Node_At(gx + dx,gy + dy);
					}
				}
			}
			if (end_node < 0) {
				end_node = -1;
				int gx = (int)((bridge->Get_End().X - _Origin.X) / _CellSize);
				int gy = (int)((bridge->Get_End().Y - _Origin.Y) / _CellSize);
				for (int dy = -reach; (dy <= reach) && (end_node < 0); dy++) {
					for (int dx = -reach; (dx <= reach) && (end_node < 0); dx++) {
						end_node = Node_At(gx + dx,gy + dy);
					}
				}
			}
		}

		if ((start_node < 0) || (end_node < 0)) {
			continue;
		}

		Add_Edge(start_node,end_node,TRAVERSE_EDGE_BRIDGE,width,bridge->Get_ID());
	}

	/*
	**	V2 Phase 10.  A tunnel joins the open ground at its two mouths the way a bridge joins its
	**	two banks: one edge, if the body is a class the bore admits and fits its clearance.  The
	**	mouth cells are searched outward a little for the same reason a bridge end is.
	*/
	for (int t = 0; t < TunnelSystem::Get_Tunnel_Count(); t++) {

		const TunnelClass * tunnel = TunnelSystem::Peek_Tunnel(t);
		if (tunnel == nullptr) {
			continue;
		}
		if (_Loco == LOCOMOTION_FLYING) {
			continue;
		}
		if ((_Loco != LOCOMOTION_INFANTRY) && !Tunnel_Class_Carries_Vehicles(tunnel->Class)) {
			continue;
		}
		if (!Crossing_Carries(tunnel->Clearance.Get_Bore_Width(),tunnel->Clearance.Get_Bore_Height(),metrics)) {
			continue;
		}

		int mouth_node[2] = { -1, -1 };
		const Vector3 * mouth[2] = { &tunnel->Start, &tunnel->End };
		int reach = (int)(snap / _CellSize);
		if (reach < 1) { reach = 1; }
		for (int m = 0; m < 2; m++) {
			mouth_node[m] = Find_Node(*mouth[m]);
			if (mouth_node[m] >= 0) continue;
			int gx = (int)((mouth[m]->X - _Origin.X) / _CellSize);
			int gy = (int)((mouth[m]->Y - _Origin.Y) / _CellSize);
			for (int dy = -reach; (dy <= reach) && (mouth_node[m] < 0); dy++) {
				for (int dx = -reach; (dx <= reach) && (mouth_node[m] < 0); dx++) {
					mouth_node[m] = Node_At(gx + dx,gy + dy);
				}
			}
		}

		if ((mouth_node[0] < 0) || (mouth_node[1] < 0) || (mouth_node[0] == mouth_node[1])) {
			continue;
		}

		Add_Edge(mouth_node[0],mouth_node[1],TRAVERSE_EDGE_TUNNEL,tunnel->Clearance.Get_Bore_Width(),tunnel->Get_ID());
	}
}


/***********************************************************************************************
 * WorldTraversabilityGraph::Build_Components -- flood fill, so reachability is a comparison    *
 *=============================================================================================*/
void WorldTraversabilityGraph::Build_Components(void)
{
	//	Flatten the edge lists first.  Count, then place.
	_AdjacencyStart.Delete_All();
	_AdjacencyCount.Delete_All();
	_Adjacency.Delete_All();

	int node_count = _Nodes.Count();
	for (int i = 0; i < node_count; i++) {
		_AdjacencyStart.Add(0);
		_AdjacencyCount.Add(0);
	}

	for (int e = 0; e < _Edges.Count(); e++) {
		_AdjacencyCount[_Edges[e].NodeA]++;
		_AdjacencyCount[_Edges[e].NodeB]++;
	}

	int running = 0;
	for (int i = 0; i < node_count; i++) {
		_AdjacencyStart[i] = running;
		running += _AdjacencyCount[i];
	}

	for (int i = 0; i < running; i++) {
		_Adjacency.Add(-1);
	}

	DynamicVectorClass<int> fill;
	for (int i = 0; i < node_count; i++) {
		fill.Add(0);
	}

	for (int e = 0; e < _Edges.Count(); e++) {
		int a = _Edges[e].NodeA;
		int b = _Edges[e].NodeB;
		_Adjacency[_AdjacencyStart[a] + fill[a]] = e;
		fill[a]++;
		_Adjacency[_AdjacencyStart[b] + fill[b]] = e;
		fill[b]++;
	}

	//	Components, breadth first from every unvisited node.  Iterative: a recursive flood
	//	over a hundred thousand cells is a stack overflow waiting for a large map.
	_ComponentCount = 0;
	DynamicVectorClass<int> queue;

	for (int start = 0; start < node_count; start++) {

		if (_Nodes[start].Component >= 0) {
			continue;
		}

		int component = _ComponentCount++;
		queue.Delete_All();
		queue.Add(start);
		_Nodes[start].Component = component;

		int head = 0;
		while (head < queue.Count()) {

			int current = queue[head++];
			int first = _AdjacencyStart[current];
			int count = _AdjacencyCount[current];

			for (int k = 0; k < count; k++) {
				const TraversabilityEdgeStruct & edge = _Edges[_Adjacency[first + k]];
				int other = (edge.NodeA == current) ? edge.NodeB : edge.NodeA;
				if (_Nodes[other].Component < 0) {
					_Nodes[other].Component = component;
					queue.Add(other);
				}
			}
		}
	}
}


/***********************************************************************************************
 * WorldTraversabilityGraph::Build --                                                          *
 *=============================================================================================*/
bool WorldTraversabilityGraph::Build(WorldLocomotionClassType loco,float cell_size)
{
	Destroy();

	if ((loco < 0) || (loco >= LOCOMOTION_COUNT)) {
		return false;
	}
	if (!WorldTerrainSystem::Has_Terrain()) {
		return false;
	}

	WorldMetricsStruct metrics;
	if (!WorldMetricsClass::Get_Class_Metrics(loco,&metrics)) {
		//	No definition of this class is loaded.  An empty graph would say every place is
		//	unreachable, which is a different and wrong claim.
		return false;
	}

	_Loco = loco;

	//	The cell has to be at least as wide as the body, or the graph reports routes through
	//	gaps the body does not fit in.
	_CellSize = (cell_size > 0.0f) ? cell_size : metrics.CorridorWidth;
	if (_CellSize < MIN_CELL_SIZE) {
		_CellSize = MIN_CELL_SIZE;
	}

	if (!Build_Nodes(metrics)) {
		Destroy();
		return false;
	}

	Build_Ground_Edges(metrics);
	Build_Crossing_Edges(metrics);
	Build_Components();

	_Built = true;
	return true;
}


void WorldTraversabilityGraph::Destroy(void)
{
	_Nodes.Delete_All();
	_Edges.Delete_All();
	_Adjacency.Delete_All();
	_AdjacencyStart.Delete_All();
	_AdjacencyCount.Delete_All();
	_GridToNode.Delete_All();

	_GridX = 0;
	_GridY = 0;
	_CellSize = 0.0f;
	_Origin.Set(0.0f,0.0f,0.0f);
	_ComponentCount = 0;
	_Built = false;
	_Loco = LOCOMOTION_INVALID;
}


bool WorldTraversabilityGraph::Is_Built(void)					{ return _Built; }
WorldLocomotionClassType WorldTraversabilityGraph::Get_Locomotion_Class(void)	{ return _Loco; }
float WorldTraversabilityGraph::Get_Cell_Size(void)			{ return _CellSize; }
int WorldTraversabilityGraph::Get_Node_Count(void)			{ return _Nodes.Count(); }
int WorldTraversabilityGraph::Get_Passable_Node_Count(void)	{ return _Nodes.Count(); }
int WorldTraversabilityGraph::Get_Edge_Count(void)			{ return _Edges.Count(); }
int WorldTraversabilityGraph::Get_Component_Count(void)		{ return _ComponentCount; }


const TraversabilityNodeStruct * WorldTraversabilityGraph::Peek_Node(int index)
{
	if ((index < 0) || (index >= _Nodes.Count())) {
		return nullptr;
	}
	return &_Nodes[index];
}


const TraversabilityEdgeStruct * WorldTraversabilityGraph::Peek_Edge(int index)
{
	if ((index < 0) || (index >= _Edges.Count())) {
		return nullptr;
	}
	return &_Edges[index];
}


int WorldTraversabilityGraph::Find_Node(const Vector3 & position)
{
	if (!_Built && (_Nodes.Count() == 0)) {
		return -1;
	}
	if (_CellSize <= 0.0f) {
		return -1;
	}

	int gx = (int)((position.X - _Origin.X) / _CellSize);
	int gy = (int)((position.Y - _Origin.Y) / _CellSize);
	return Node_At(gx,gy);
}


bool WorldTraversabilityGraph::Is_Reachable_Nodes(int from_node,int to_node)
{
	if ((from_node < 0) || (to_node < 0) ||
		 (from_node >= _Nodes.Count()) || (to_node >= _Nodes.Count())) {
		return false;
	}
	return (_Nodes[from_node].Component == _Nodes[to_node].Component);
}


bool WorldTraversabilityGraph::Is_Reachable(const Vector3 & from,const Vector3 & to)
{
	return Is_Reachable_Nodes(Find_Node(from),Find_Node(to));
}


/***********************************************************************************************
 * WorldTraversabilityGraph::Find_Route -- one route, in cells                                 *
 *=============================================================================================*/
bool WorldTraversabilityGraph::Find_Route(int from_node,int to_node,TraversabilityPathClass * path_out)
{
	if (path_out == nullptr) {
		return false;
	}
	path_out->Delete_All();

	if (!Is_Reachable_Nodes(from_node,to_node)) {
		return false;
	}
	if (from_node == to_node) {
		path_out->Add(from_node);
		return true;
	}

	DynamicVectorClass<int> came_from;
	for (int i = 0; i < _Nodes.Count(); i++) {
		came_from.Add(-1);
	}

	DynamicVectorClass<int> queue;
	queue.Add(from_node);
	came_from[from_node] = from_node;

	int head = 0;
	bool found = false;

	while ((head < queue.Count()) && !found) {

		int current = queue[head++];
		int first = _AdjacencyStart[current];
		int count = _AdjacencyCount[current];

		for (int k = 0; k < count; k++) {
			const TraversabilityEdgeStruct & edge = _Edges[_Adjacency[first + k]];
			int other = (edge.NodeA == current) ? edge.NodeB : edge.NodeA;
			if (came_from[other] >= 0) {
				continue;
			}
			came_from[other] = current;
			if (other == to_node) {
				found = true;
				break;
			}
			queue.Add(other);
		}
	}

	if (!found) {
		return false;
	}

	//	Walk back, then reverse into the caller's list.
	DynamicVectorClass<int> reversed;
	int walk = to_node;
	while (walk != from_node) {
		reversed.Add(walk);
		walk = came_from[walk];
	}
	reversed.Add(from_node);

	for (int i = reversed.Count() - 1; i >= 0; i--) {
		path_out->Add(reversed[i]);
	}
	return true;
}


/***********************************************************************************************
 * WorldTraversabilityGraph::Count_Distinct_Routes -- edge-disjoint, so detours do not count    *
 *                                                                                             *
 * Find a route, strike its edges out, find another.  What survives that is a set of routes no  *
 * two of which share a single step, which is what "alternate route" has to mean for the        *
 * answer to be worth anything -- otherwise a corridor with a pillar in it reports two routes.  *
 *=============================================================================================*/
int WorldTraversabilityGraph::Count_Distinct_Routes(int from_node,int to_node,int max_routes)
{
	if (!Is_Reachable_Nodes(from_node,to_node)) {
		return 0;
	}
	if (from_node == to_node) {
		return 1;
	}
	if (max_routes < 1) {
		max_routes = 1;
	}

	DynamicVectorClass<bool> blocked;
	for (int i = 0; i < _Edges.Count(); i++) {
		blocked.Add(false);
	}

	DynamicVectorClass<int> came_from;
	DynamicVectorClass<int> came_edge;
	DynamicVectorClass<int> queue;

	int routes = 0;

	while (routes < max_routes) {

		came_from.Delete_All();
		came_edge.Delete_All();
		for (int i = 0; i < _Nodes.Count(); i++) {
			came_from.Add(-1);
			came_edge.Add(-1);
		}

		queue.Delete_All();
		queue.Add(from_node);
		came_from[from_node] = from_node;

		int head = 0;
		bool found = false;

		while ((head < queue.Count()) && !found) {

			int current = queue[head++];
			int first = _AdjacencyStart[current];
			int count = _AdjacencyCount[current];

			for (int k = 0; k < count; k++) {

				int edge_index = _Adjacency[first + k];
				if (blocked[edge_index]) {
					continue;
				}

				const TraversabilityEdgeStruct & edge = _Edges[edge_index];
				int other = (edge.NodeA == current) ? edge.NodeB : edge.NodeA;
				if (came_from[other] >= 0) {
					continue;
				}

				came_from[other] = current;
				came_edge[other] = edge_index;

				if (other == to_node) {
					found = true;
					break;
				}
				queue.Add(other);
			}
		}

		if (!found) {
			break;
		}

		//	Strike this route's edges out and look for the next one.
		int walk = to_node;
		while (walk != from_node) {
			blocked[came_edge[walk]] = true;
			walk = came_from[walk];
		}
		routes++;
	}

	return routes;
}


/***********************************************************************************************
 * WorldTraversabilityGraph::Find_Chokepoints -- the edges no alternate route covers            *
 *                                                                                             *
 * Standard bridge finding: an edge is a chokepoint when the subtree below it in a depth-first  *
 * traversal can reach nothing at or above its own discovery time by any other route.  Done     *
 * iteratively, because a depth-first walk of a large map's grid is far deeper than the stack.  *
 *                                                                                             *
 * Note the term collision: a graph "bridge" and a river bridge are different things, and this  *
 * function finds the first kind.  A river bridge is very often also one, which is the point.   *
 *=============================================================================================*/
int WorldTraversabilityGraph::Find_Chokepoints(DynamicVectorClass<int> & edge_indices_out)
{
	int node_count = _Nodes.Count();
	if (node_count == 0) {
		return 0;
	}

	DynamicVectorClass<int> discovery;
	DynamicVectorClass<int> low;
	DynamicVectorClass<int> parent_edge;
	DynamicVectorClass<int> next_child;

	for (int i = 0; i < node_count; i++) {
		discovery.Add(-1);
		low.Add(0);
		parent_edge.Add(-1);
		next_child.Add(0);
	}

	int timer = 0;
	int found = 0;

	//	An explicit depth counter rather than growing and shrinking the vector: DynamicVectorClass
	//	of int cannot tell Delete(index) from Delete(value), and a stack that pops by value is a
	//	bug waiting to happen.  The storage is reused across roots and only ever grows.
	DynamicVectorClass<int> stack;
	int depth = 0;

	for (int root = 0; root < node_count; root++) {

		if (discovery[root] >= 0) {
			continue;
		}

		discovery[root] = low[root] = timer++;
		parent_edge[root] = -1;
		next_child[root] = 0;

		depth = 0;
		if (stack.Count() < 1) { stack.Add(root); } else { stack[0] = root; }
		depth = 1;

		while (depth > 0) {

			int current = stack[depth - 1];
			int first = _AdjacencyStart[current];
			int count = _AdjacencyCount[current];

			if (next_child[current] < count) {

				int edge_index = _Adjacency[first + next_child[current]];
				next_child[current]++;

				//	Never walk back up the edge we arrived on.  Walking back up a *different*
				//	edge to the same parent is allowed, and must be: two cells joined twice
				//	are not a chokepoint.
				if (edge_index == parent_edge[current]) {
					continue;
				}

				const TraversabilityEdgeStruct & edge = _Edges[edge_index];
				int other = (edge.NodeA == current) ? edge.NodeB : edge.NodeA;

				if (discovery[other] < 0) {
					discovery[other] = low[other] = timer++;
					parent_edge[other] = edge_index;
					next_child[other] = 0;
					if (depth < stack.Count()) { stack[depth] = other; } else { stack.Add(other); }
					depth++;
				} else {
					if (discovery[other] < low[current]) {
						low[current] = discovery[other];
					}
				}

			} else {

				depth--;

				if (depth > 0) {

					int parent = stack[depth - 1];
					if (low[current] < low[parent]) {
						low[parent] = low[current];
					}

					//	Nothing under this node can see past it, so the edge that got here is
					//	the only way in.
					if (low[current] > discovery[parent]) {
						edge_indices_out.Add(parent_edge[current]);
						found++;
					}
				}
			}
		}
	}

	return found;
}


/***********************************************************************************************
 * WorldTraversabilityGraph::Count_Unreachable --                                               *
 *=============================================================================================*/
int WorldTraversabilityGraph::Count_Unreachable(int from_node)
{
	if ((from_node < 0) || (from_node >= _Nodes.Count())) {
		return _Nodes.Count();
	}

	int component = _Nodes[from_node].Component;
	int unreachable = 0;

	for (int i = 0; i < _Nodes.Count(); i++) {
		if (_Nodes[i].Component != component) {
			unreachable++;
		}
	}
	return unreachable;
}


/***********************************************************************************************
 * WorldTraversabilityGraph::Is_Infantry_Only_Route --                                          *
 *=============================================================================================*/
bool WorldTraversabilityGraph::Is_Infantry_Only_Route(const Vector3 & from,const Vector3 & to,
																		bool reachable_by_vehicle)
{
	if (_Loco != LOCOMOTION_INFANTRY) {
		return false;
	}
	return Is_Reachable(from,to) && !reachable_by_vehicle;
}
