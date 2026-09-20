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
 *                     $Archive:: /Commando/Code/Combat/worldtraversability.h                 $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	Whether one place can be reached from another, by what, and through where.                  *
 *                                                                                             *
 *	Roadmap Section 52B, Phase 65.  Query_Placement_Surface is a disc test; nothing in the      *
 *	tree could answer "is B reachable from A on foot but not by tank".  That is the question    *
 *	that decides whether a world satisfies Section 3A.5's alternate routes, flanking routes     *
 *	and chokepoints, and it is the question a generator has to be able to ask about its own     *
 *	output before that output is allowed to ship.                                               *
 *                                                                                             *
 *	**This is an analysis structure, not a pathfinder.**  PathfindClass stays the authority     *
 *	for how a unit actually moves, and nothing here is ever consulted to move one.  The graph   *
 *	exists to answer questions about the *shape* of the world, which a pathfinder is the        *
 *	wrong tool for: a pathfinder tells you one route and says nothing about whether it was      *
 *	the only one.                                                                               *
 *                                                                                             *
 *	Coarse by construction.  One node per grid cell of a size the caller picks, defaulting to   *
 *	the locomotion class's own corridor width, so the same world yields a different graph for   *
 *	infantry and for tracked vehicles.  Every dimension it reasons with -- what it can climb,   *
 *	how wide it is, how much headroom it needs -- comes from WorldMetricsClass (Phase 63);      *
 *	nothing here carries its own copy of a step height or a slope limit.                        *
 *                                                                                             *
 *	What contributes edges: the heightfield, the water areas, the escarpments of Phase 64, and  *
 *	the bridges.  A crossing carries the clearance and width it states, so a bridge a tank      *
 *	cannot fit under or across is simply not an edge in the tank's graph while remaining one    *
 *	in the infantry graph -- which is Phase 65's acceptance line, kept structurally rather      *
 *	than by a special case.                                                                     *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef WORLDTRAVERSABILITY_H
#define WORLDTRAVERSABILITY_H

#ifndef ALWAYS_H
#include "always.h"
#endif

#ifndef VECTOR3_H
#include "vector3.h"
#endif

#ifndef VECTOR_H
#include "vector.h"
#endif

#ifndef WORLDMETRICS_H
#include "worldmetrics.h"
#endif


/*
**	Why an edge exists.  Worth keeping, because "these two places are connected only by a
**	bridge" is a different fact about a world from "these two places are connected by open
**	ground", and a chokepoint report that cannot tell them apart is much less useful.
*/
enum TraversabilityEdgeKind
{
	TRAVERSE_EDGE_GROUND	= 0,	// adjacent cells of walkable terrain
	TRAVERSE_EDGE_BRIDGE,			// a crossing that states it carries this class
	TRAVERSE_EDGE_ROAD,				// a road surface joining two cells the raw ground would not

	TRAVERSE_EDGE_KIND_COUNT,
};


struct TraversabilityEdgeStruct
{
	TraversabilityEdgeStruct(void) : NodeA(-1), NodeB(-1), Kind(TRAVERSE_EDGE_GROUND), Width(0.0f), SourceID(-1) { }

	int		NodeA;
	int		NodeB;
	int		Kind;
	float		Width;		// narrowest point of the edge, metres; 0 means "the cell size"
	int		SourceID;	// bridge or road id for the crossing kinds, -1 otherwise

	bool	operator == (const TraversabilityEdgeStruct & that) const
	{
		return (NodeA == that.NodeA) && (NodeB == that.NodeB) && (Kind == that.Kind) && (SourceID == that.SourceID);
	}
	bool	operator != (const TraversabilityEdgeStruct & that) const	{ return !(*this == that); }
};


/*
**	One node: a cell of the coarse grid that this locomotion class can stand in.
*/
struct TraversabilityNodeStruct
{
	TraversabilityNodeStruct(void) : GridX(0), GridY(0), Position(0.0f,0.0f,0.0f), Component(-1), Passable(false) { }

	int		GridX;
	int		GridY;
	Vector3	Position;		// cell centre, at ground height
	int		Component;		// connected-component id, -1 until the graph is built
	bool		Passable;

	bool	operator == (const TraversabilityNodeStruct & that) const
	{
		return (GridX == that.GridX) && (GridY == that.GridY);
	}
	bool	operator != (const TraversabilityNodeStruct & that) const	{ return !(*this == that); }
};


typedef DynamicVectorClass<int>	TraversabilityPathClass;


class WorldTraversabilityGraph
{
public:

	/*
	**	Build the graph for one locomotion class over the terrain currently loaded.
	**
	**	cell_size of zero takes the class's own corridor width from WorldMetricsClass, which
	**	is the right default: a graph whose cells are narrower than the body they describe
	**	reports routes that body cannot take.
	**
	**	Returns false when there is no terrain, or when WorldMetricsClass cannot answer for
	**	the class -- a level with no vehicles has no vehicle graph, and saying so is a better
	**	answer than an empty graph that reports everything unreachable.
	*/
	static bool		Build(WorldLocomotionClassType loco,float cell_size = 0.0f);
	static void		Destroy(void);
	static bool		Is_Built(void);

	static WorldLocomotionClassType	Get_Locomotion_Class(void);
	static float	Get_Cell_Size(void);

	/*
	**	Shape of what was built.
	*/
	static int		Get_Node_Count(void);
	static int		Get_Passable_Node_Count(void);
	static int		Get_Edge_Count(void);
	static int		Get_Component_Count(void);
	static const TraversabilityNodeStruct *	Peek_Node(int index);
	static const TraversabilityEdgeStruct *	Peek_Edge(int index);

	/*
	**	Which node a place is in.  -1 when the place is off the grid or in a cell this class
	**	cannot stand in, which is itself an answer: "you cannot start there".
	*/
	static int		Find_Node(const Vector3 & position);

	/*
	**	Is B reachable from A for this class?  Connected components, so it is a comparison,
	**	not a search.
	*/
	static bool		Is_Reachable(const Vector3 & from,const Vector3 & to);
	static bool		Is_Reachable_Nodes(int from_node,int to_node);

	/*
	**	One route, as a node index list.  Breadth-first, so it is the fewest cells rather
	**	than the shortest metres -- this is a question about shape, and a pathfinder is what
	**	you use when you care about the metres.
	*/
	static bool		Find_Route(int from_node,int to_node,TraversabilityPathClass * path_out);

	/*
	**	How many genuinely distinct routes there are, up to a limit: edge-disjoint, so two
	**	routes that differ by one cell of detour around the same chokepoint count once.  This
	**	is Section 3A.5's "alternate routes and flanking routes" made countable.
	*/
	static int		Count_Distinct_Routes(int from_node,int to_node,int max_routes = 4);

	/*
	**	The edges whose removal disconnects something: the chokepoints.  Standard bridge
	**	finding over the graph, which finds exactly the edges no alternate route covers.
	**	Appends edge indices; the list is not cleared first.
	*/
	static int		Find_Chokepoints(DynamicVectorClass<int> & edge_indices_out);

	/*
	**	How many passable cells cannot be reached from this one.  A generator's own sanity
	**	check on its output: a world with a large marooned region has a hole in it.
	*/
	static int		Count_Unreachable(int from_node);

	/*
	**	Routes that exist for one class and not another, answered as a pair of reachability
	**	questions over two builds.  Because only one graph is held at a time, the caller
	**	hands in the answer it already got from the other class rather than the graph itself.
	**
	**	This is the shape Phase 65's acceptance asks about: a crossing absent from the
	**	vehicle graph while present in the infantry graph.
	*/
	static bool		Is_Infantry_Only_Route(const Vector3 & from,const Vector3 & to,
													bool reachable_by_vehicle);

	/*
	**	The pure passability rules, exposed because they are the part worth testing without a
	**	world and because a caller sizing something it has not built yet needs them first.
	*/

	//	Can this class get from one cell to the next, given the height difference between the
	//	cells and how far apart they are?  Either the step is small enough to walk up, or the
	//	slope between them is shallow enough to climb.
	static bool		Can_Step(float height_difference,float horizontal_distance,
									float step_height,float max_climbable_slope);

	//	Does a crossing of this width and headroom carry this class?
	static bool		Crossing_Carries(float crossing_width,float crossing_clearance,
											  const WorldMetricsStruct & metrics);

	//	How deep this class can wade.  Infantry wade; nothing else does, which is why a
	//	river is a wall to a tank and a nuisance to a soldier.
	static float	Compute_Wade_Depth(WorldLocomotionClassType loco,const WorldMetricsStruct & metrics);

private:

	static bool		Build_Nodes(const WorldMetricsStruct & metrics);
	static void		Build_Ground_Edges(const WorldMetricsStruct & metrics);
	static void		Build_Crossing_Edges(const WorldMetricsStruct & metrics);
	static void		Build_Components(void);
	static void		Add_Edge(int a,int b,int kind,float width,int source_id);
	static int		Node_At(int gx,int gy);
};

#endif	// WORLDTRAVERSABILITY_H
