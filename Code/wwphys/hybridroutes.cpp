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
 ***                            Confidential - Westwood Studios                              ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : OpenW3D                                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwphys/hybridroutes.cpp                      $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Runtime Phase 10.  See hybridroutes.h for what this phase owns; the plan's own order is kept  *
 * below: the priority groups, the sub-routing through frozen waypoints, the heading-aware A*     *
 * with its exact cost terms, and the seven-step path-to-spline conversion.                      *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#include "hybridroutes.h"

#include "wwdebug.h"
#include "wwmath.h"

#include <math.h>
#include <string.h>


static inline float Max2(float a,float b)	{ return (a > b) ? a : b; }
static inline float Min2(float a,float b)	{ return (a < b) ? a : b; }
static inline float Clamp2(float v,float lo,float hi)	{ return (v < lo) ? lo : ((v > hi) ? hi : v); }


/*
**	Section 8 Phase 10's own constants.
*/
static const float	WINDOW_PENALTY_A			= 20.0f;		// 20A for entering a non-mandatory window
static const float	REUSE_MODIFIER				= -0.25f;	// -0.25*d for a dirt/trail move that reuses a road
static const float	HEURISTIC_WEIGHT			= 0.75f;		// 0.75 * Euclidean
static const float	COLLINEAR_RUN_W			= 6.0f;		// the >6W collinear rule

/*
**	How much straight road a turn needs on each side, as a multiple of the class minimum turn radius.
**	A centripetal Catmull-Rom through controls one radius apart bends tighter than that radius -- measured
**	on the failing seeds it came out at about 0.97 of the control spacing -- so the corner needs room for
**	its own arc, which is the chord of a quarter turn: twice the radius.
*/
/*
**	A centripetal Catmull-Rom through a 45-degree vertex bends to roughly a quarter of its control spacing
**	-- measured on this grid: controls five cells apart produced a 23-unit corner against the 26.6 a
**	`HIGHWAY_4` needs.  So a turn needs about four radii of straight around it, which is what this factor
**	is; the exact class limit is still enforced on the spline afterwards.
*/
/*
**	How far a turn has to stand from the one before it, as a multiple of the class minimum turn radius.
**
**	An 8-way chain has only two ways to hold a wide radius, and they pull in opposite directions.
**
**	A run of turns the *same* way is how a grid draws an arc: 45-degree steps `d` apart have their vertices
**	on a circle of radius `d / 2 sin(22.5deg)` = `1.3 d`, and a centripetal Catmull-Rom interpolates those
**	vertices, so it follows that circle.  Such a turn is legal as soon as it is about `0.8 R` from the last
**	one -- crowding them is what holds the radius up.
**
**	A turn standing on its own is the opposite case: the curve through it is a single rounded corner, and
**	the measured relation is that the corner comes out at about a quarter of the control spacing.  So an
**	isolated turn needs `4 R` of run behind it, the same separation the control thinning uses.
**
**	Getting this wrong in either direction is measurable.  Demanding twice the room for an arc and half of
**	it for a hairpin left an 8.3 radius where 16.6 was needed.  Allowing an isolated turn after one class
**	radius of straight left 13.6 where 26.6 was needed -- a lone 45-degree vertex with 60 units of straight
**	either side, which is a 15-unit corner whatever the chain's own cell-centre triples measure.
*/
static const float	TURN_SPACING_ARC			= 0.8f;		// an arc of same-way 45-degree steps
static const float	TURN_SPACING_ISOLATED	= 4.0f;		// a corner standing on its own

/*
**	The baseline the chain's own curvature is measured over, as a multiple of the class radius.  One cell
**	only sees a single vertex, and a zig-zag of two opposite vertices one cell apart measures as two legal
**	corners while the spline through them bends to a fraction of either.
*/
static const float	CURVATURE_BASELINE_FACTOR	= 1.0f;
static const float	CURVATURE_SEARCH_MARGIN	= 1.20f;		// the search holds the class radius itself

/*
**	The road centerline is built from arcs of a little more than the class minimum turn radius, so that the
**	exact Section-7 measurement taken afterwards on the sampled polyline is not decided by float noise or by
**	the 0.001 position quantum.  It is a construction margin only: the accepted radius is still measured
**	against the class minimum itself, and a curve under it is still refused.
*/
static const float	FILLET_RADIUS_MARGIN		= 1.05f;
static const float	FILLET_RADIUS_STEP		= 0.10f;		// one escalation step, on a curvature refusal

static const int		DIRECTION_COUNT			= 8;
static const int		STATE_COUNT					= DIRECTION_COUNT + 1;	// the eight directions plus START
static const int		HEAP_ENTRIES_PER_STATE	= 4;			// lazily-deleted duplicates per state
static const int		START_DIRECTION			= DIRECTION_COUNT;
static const float	PI_OVER_180					= 3.14159265358979323846f / 180.0f;
static const float	TWO_PI							= 6.28318530717958647692f;

//	The plan's neighbor order, `E,NE,N,NW,W,SW,S,SE`, which is also the expansion order.
static const int	NEIGHBOR_DX[DIRECTION_COUNT] = {  1, 1, 0,-1,-1,-1, 0, 1 };
static const int	NEIGHBOR_DY[DIRECTION_COUNT] = {  0, 1, 1, 1, 0,-1,-1,-1 };


const char * Hybrid_Route_Priority_Name(HybridRoutePriority priority)
{
	switch (priority) {
		case HYBRID_ROUTE_PRIORITY_FAMILY_TRUNK:	return "family_trunk";
		case HYBRID_ROUTE_PRIORITY_BRIDGE_LINK:	return "bridge_link";
		case HYBRID_ROUTE_PRIORITY_HOME_ROAD:		return "home_road";
		case HYBRID_ROUTE_PRIORITY_FIELD_BRANCH:	return "field_branch";
		case HYBRID_ROUTE_PRIORITY_SERVICE_FILL:	return "service_fill";
		case HYBRID_ROUTE_PRIORITY_OVERPASS_LINK:	return "overpass_link";
		case HYBRID_ROUTE_PRIORITY_OPTIONAL_LINK:	return "optional_link";
		case HYBRID_ROUTE_PRIORITY_TRAIL:			return "trail";
		default:												break;
	}
	return "unknown";
}


const char * Hybrid_Route_Failure_Name(HybridRouteFailure failure)
{
	switch (failure) {
		case HYBRID_ROUTE_OK:						return "ok";
		case HYBRID_ROUTE_INVALID_INPUT:			return "invalid_input";
		case HYBRID_ROUTE_SCRATCH_EXHAUSTED:	return "scratch_exhausted";
		case HYBRID_ROUTE_NO_PATH:				return "no_path";
		case HYBRID_ROUTE_NO_SPLINE:				return "no_spline";
		case HYBRID_ROUTE_EXPANSION_BUDGET:	return "expansion_budget";
		case HYBRID_ROUTE_WAYPOINT_MISSED:		return "waypoint_missed";
		default:											break;
	}
	return "unknown";
}


HybridRouteCostWeights Hybrid_Route_Cost_Weights(HybridRoadClass road_class)
{
	HybridRouteCostWeights weights;
	switch (road_class) {
		case HYBRID_ROAD_HIGHWAY_4:			weights.Ks = 10.0f;	weights.Kt = 8.0f;	weights.Kc = 3.0f;	break;
		case HYBRID_ROAD_PRIMARY_2:			weights.Ks = 8.0f;	weights.Kt = 6.0f;	weights.Kc = 2.5f;	break;
		case HYBRID_ROAD_SECONDARY_2:			weights.Ks = 6.0f;	weights.Kt = 4.0f;	weights.Kc = 2.0f;	break;
		case HYBRID_ROAD_SERVICE_DIRT_2:		weights.Ks = 4.0f;	weights.Kt = 3.0f;	weights.Kc = 1.5f;	break;
		default:										weights.Ks = 3.0f;	weights.Kt = 2.0f;	weights.Kc = 1.0f;	break;
	}
	return weights;
}


HybridRoutePriority Hybrid_Route_Priority_Of(const HybridTopology & topology,const HybridTopologyEdge & edge)
{
	switch (edge.Kind) {

		case HYBRID_EDGE_FAMILY_TRUNK:	return HYBRID_ROUTE_PRIORITY_FAMILY_TRUNK;
		case HYBRID_EDGE_HOME_ROAD:		return HYBRID_ROUTE_PRIORITY_HOME_ROAD;
		case HYBRID_EDGE_FIELD_BRANCH:	return HYBRID_ROUTE_PRIORITY_FIELD_BRANCH;
		case HYBRID_EDGE_SERVICE_FILL:	return HYBRID_ROUTE_PRIORITY_SERVICE_FILL;
		case HYBRID_EDGE_TRAIL:				return HYBRID_ROUTE_PRIORITY_TRAIL;

		case HYBRID_EDGE_CROSS_LINK:
		{
			/*
			**	A cross-link is one of three things, and its own waypoints say which: a mandatory
			**	synthesized bridge link carries a crossing obligation, an overpass link carries a shared
			**	overpass obligation, and anything else is an optional heavy cross-link.
			*/
			bool crossing = false;
			bool overpass = false;
			for (int w = 0; w < edge.Waypoints.Count(); ++w) {
				const HybridTopologyNode * node = topology.Find_Node(edge.Waypoints[w]);
				if (node == nullptr) continue;
				if (node->Kind == HYBRID_NODE_CROSSING_OBLIGATION)	crossing = true;
				if (node->Kind == HYBRID_NODE_OVERPASS_OBLIGATION)	overpass = true;
			}
			if (crossing)	return HYBRID_ROUTE_PRIORITY_BRIDGE_LINK;
			if (overpass)	return HYBRID_ROUTE_PRIORITY_OVERPASS_LINK;
			return HYBRID_ROUTE_PRIORITY_OPTIONAL_LINK;
		}

		default:									break;
	}
	return HYBRID_ROUTE_PRIORITY_OPTIONAL_LINK;
}


void HybridRouteSet::Reset(void)
{
	Routes.Delete_All();
	AnalysisCountX		= 0;
	AnalysisCountY		= 0;
	TotalExpansions	= 0;
	TotalInsertions	= 0;
	WidenedRoutes		= 0;
	Checksum				= 0;
}


const HybridRoute * HybridRouteSet::Find(unsigned int edge_id) const
{
	for (int i = 0; i < Routes.Count(); ++i) {
		if (Routes[i].EdgeId == edge_id) return &Routes[i];
	}
	return nullptr;
}


int HybridRouteSet::Count_Of_Class(HybridRoadClass road_class) const
{
	int count = 0;
	for (int i = 0; i < Routes.Count(); ++i) {
		if (Routes[i].RoadClass == road_class) count++;
	}
	return count;
}


float HybridRouteSet::Total_Length_Of_Class(HybridRoadClass road_class) const
{
	float total = 0.0f;
	for (int i = 0; i < Routes.Count(); ++i) {
		if (Routes[i].RoadClass != road_class) continue;
		total += Routes[i].Length;
	}
	return Hybrid_Quantize_Position(total);
}


HybridRouteParams::HybridRouteParams(void) :
	WorldMinX(0.0f),WorldMinY(0.0f),WorldWidth(0.0f),WorldHeight(0.0f),
	MaxCutDepth(6.0f),MaxFillHeight(6.0f),RiverWidth(6.0f),FieldRadiusMax(60.0f),
	MaxLocalRetriesPerStage(8),AStarExpansionsPerRoute(0),AStarExpansionsPerAttempt(0),
	Arena(nullptr),Counters(nullptr)
{
	memset(&Constants,0,sizeof(Constants));
}


unsigned int Hybrid_Route_Checksum(const HybridRouteSet & set,unsigned int seed)
{
	HybridHashStream stream(seed);

	stream.Add_Int(set.Routes.Count());
	stream.Add_Int(set.AnalysisCountX);
	stream.Add_Int(set.AnalysisCountY);

	for (int i = 0; i < set.Routes.Count(); ++i) {

		const HybridRoute & route = set.Routes[i];
		stream.Add_Uint(route.EdgeId);
		stream.Add_Uint((unsigned int)route.RoadClass);
		stream.Add_Uint((unsigned int)route.Priority);
		stream.Add_Int(route.RouteFamily);
		stream.Add_Int(route.Envelope);
		stream.Add_Bool(route.Required);
		stream.Add_Length(route.Length);
		stream.Add_Int(route.ControlInsertions);
		stream.Add_Int(route.Points.Count());
		for (int p = 0; p < route.Points.Count(); ++p) {
			stream.Add_Length(route.Points[p].X);
			stream.Add_Length(route.Points[p].Y);
		}
		stream.Add_Int(route.Cells.Count());
		for (int c = 0; c < route.Cells.Count(); ++c) stream.Add_Int(route.Cells[c]);
		stream.Add_Int(route.Waypoints.Count());
		for (int w = 0; w < route.Waypoints.Count(); ++w) stream.Add_Uint(route.Waypoints[w]);
		stream.Add_Int(route.Structural.Count());
		for (int s = 0; s < route.Structural.Count(); ++s) {
			stream.Add_Uint(route.Structural[s].WindowId);
			stream.Add_Int(route.Structural[s].FirstPoint);
			stream.Add_Int(route.Structural[s].LastPoint);
			stream.Add_Length(route.Structural[s].Ingress.X);
			stream.Add_Length(route.Structural[s].Ingress.Y);
			stream.Add_Length(route.Structural[s].Egress.X);
			stream.Add_Length(route.Structural[s].Egress.Y);
		}
	}
	return stream.Get();
}


/***********************************************************************************************
**	The router.
***********************************************************************************************/

//	One A* search state: a cell entered from one of the eight directions, or the START state.
struct RouteSearchState
{
	float				G;
	float				F;
	int				FromCell;
	int				FromDirection;
	unsigned char	Open;
	unsigned char	Closed;
};


struct RouteBuilder
{
	const HybridRouteParams *		P;
	const HybridMapIdentity *		Identity;
	const HybridCoarseMatrices *	Matrices;
	const HybridLandformSet *		Landforms;
	const HybridHydrologySet *		Hydrology;
	const HybridAnchorSet *			Anchors;
	const HybridTopology *			Topology;
	HybridRouteHeightSampleFunc	Sample;
	void *								Context;
	HybridRouteSet *					Out;

	//	The analysis grid, identical to the one Phase 6 drained over.
	int									CountX;
	int									CountY;
	int									CellCount;
	float									A;
	float									OriginX;
	float									OriginY;

	//	Per-search state, one entry per (cell,direction) pair, plus the open heap.
	RouteSearchState *				States;

	/*
	**	Per-leg caches, keyed by cell and stamped with a generation, so a new leg costs nothing to clear.
	**	Every one of these was a scan before: the refusal rules walk the bases, the field reserves, the
	**	water bodies and the rivers; the window lookup walks every accepted window; the collinear and reuse
	**	rules walk every already-routed spline's samples.  Doing that per candidate *move* is what made a
	**	single MEDIUM world take minutes instead of seconds.
	*/
	int *									CacheStamp;
	unsigned int *						CacheRefusal;
	unsigned int *						CacheWindow;
	float *								CacheHeight;
	unsigned int *						CacheRouteMask;	// bit per already-routed route, by route index
	unsigned char *					CacheHasHeight;
	int									Generation;

	int *									Heap;
	int									HeapCount;
	int									HeapCapacity;
	bool									HeapOverflowed;
	DynamicVectorClass<int>			Block;			// owner when there is no attempt arena

	//	The current edge's routing context.
	HybridRoadClass					CurrentClass;
	int									CurrentFamily;
	float									CurrentHalfWidth;
	Vector2								CorridorA;			// the intent corridor this leg is measured against
	Vector2								CorridorB;

	/*
	**	The whole edge's intent corridor: its frozen node chain, endpoints and waypoints in order.  A
	**	route through a crossing window legitimately leaves the straight line between its two endpoints,
	**	so the envelope is measured against the chain the plan routed it through instead.
	*/
	DynamicVectorClass<Vector2>		CorridorPath;
	unsigned int						CurrentEdgeId;
	bool									CurrentIsOptional;
	unsigned int						OwnWindow;			// the window this leg is allowed to cross, 0 for none
	unsigned int						OwnFieldSite;		// the Tiberium reserve this leg may enter, 0 for none

	/*
	**	This leg's own endpoints.  They are frozen: Phase 3 placed a route waypoint under the archetype
	**	guide exemption that lets a mandatory crossing guide sit on the obstruction it must cross, and
	**	Phase 9 froze it, so the cell it stands in is a legal endpoint here whatever else is in it.  A
	**	router that refused its own frozen endpoint could never route that family at all.
	*/
	int									StartCell;
	int									GoalCell;
	int									Expansions;

	/*
	**	What the previous leg left at this leg's start: the heading it arrived on and how much straight road
	**	it arrived with.  A leg's start is a frozen waypoint, not a fresh beginning, and a turn crowded
	**	against it is exactly as tight as a turn crowded against another turn.  Without this the road left
	**	every crossing waypoint on a heading of its own choosing -- measured on the failing seed, an 8.9
	**	radius two cells past a window where 16.6 was needed.
	*/
	int									EntryDirection;
	float									EntryRun;

	/*
	**	True while the leg being routed *is* a crossing: both its endpoints are one window's frozen approach
	**	points.  That interval is structural, not open road -- Phase 10 only decides it is straight and where
	**	it starts and ends, and the interaction phase compiles the deck or bore and the ramps onto it -- so
	**	the class's own turn geometry is not asked of it, exactly as the grade and cross-slope tests are not.
	*/
	bool									StructuralLeg;

	//	The last refusal's own numbers, for the failure detail.
	const int *							LastChain;
	int									LastChainCount;
	int									LastExpansions;
	int									LastGoalTouched;
	int									LastClosed;
	float									LastRadius;
	int									LastRadiusAt;
	unsigned int						LastRadiusWindow;


	int	Cell_X(int cell) const					{ return cell % CountX; }
	int	Cell_Y(int cell) const					{ return cell / CountX; }
	int	Cell_Index(int x,int y) const			{ return (y * CountX) + x; }
	bool	Valid_Cell(int x,int y) const			{ return (x >= 0) && (y >= 0) && (x < CountX) && (y < CountY); }

	Vector2	Cell_Center(int cell) const
	{
		return Vector2(OriginX + (((float)Cell_X(cell) + 0.5f) * A),
							OriginY + (((float)Cell_Y(cell) + 0.5f) * A));
	}

	/*
	**	The cell containing a quantized world position; a position exactly on a boundary takes the lower
	**	row-major cell id, which is the plan's own tie rule.
	*/
	int	Cell_Of(const Vector2 & world) const
	{
		float fx = (world.X - OriginX) / A;
		float fy = (world.Y - OriginY) / A;
		int x = (int)::floor((double)fx);
		int y = (int)::floor((double)fy);
		if ((float)x == fx) x = (x > 0) ? (x - 1) : 0;			// exactly on the boundary: the lower cell
		if ((float)y == fy) y = (y > 0) ? (y - 1) : 0;
		if (x < 0) x = 0;
		if (y < 0) y = 0;
		if (x > (CountX - 1)) x = CountX - 1;
		if (y > (CountY - 1)) y = CountY - 1;
		return Cell_Index(x,y);
	}
};


static bool Fail(HybridRouteResult * result,HybridRouteFailure failure,const char * detail)
{
	if (result != nullptr) {
		result->Failure	= failure;
		result->Detail		= (detail != nullptr) ? detail : "";
	}
	return false;
}


static bool Height_At(const RouteBuilder & b,const Vector2 & world,float * height_out)
{
	if (b.Sample == nullptr) return false;
	return b.Sample(world,height_out,b.Context);
}


static bool Allocate_Search(RouteBuilder & b,HybridRouteResult * result)
{
	const int states = b.CellCount * STATE_COUNT;
	const int state_words = (int)((states * sizeof(RouteSearchState)) / sizeof(int));

	/*
	**	The open set uses lazy deletion: a state whose `g` improves is pushed again and the stale entry is
	**	skipped when it pops.  So the heap holds more entries than there are states -- one per improvement
	**	-- and sizing it at one entry per state let it run off the end of its own block and into the state
	**	array behind it, which is what made a search whose goal was provably reachable report no path.
	*/
	const int heap_capacity = states * HEAP_ENTRIES_PER_STATE;
	const int cache_words = (b.CellCount * 5) + (((b.CellCount) + 3) / 4);
	const int words = state_words + heap_capacity + cache_words;

	int * base = nullptr;
	if (b.P->Arena != nullptr) {
		base = (int *)b.P->Arena->Allocate(words * (int)sizeof(int),16);
		if (base == nullptr) {
			return Fail(result,HYBRID_ROUTE_SCRATCH_EXHAUSTED,"A* state array does not fit the arena");
		}
	} else {
		b.Block.Resize(words);
		while (b.Block.Count() < words) b.Block.Add(0);
		if (b.Block.Count() < words) {
			return Fail(result,HYBRID_ROUTE_SCRATCH_EXHAUSTED,"A* state array could not be allocated");
		}
		base = &b.Block[0];
	}

	b.States			= (RouteSearchState *)base;
	b.Heap			= base + state_words;
	b.HeapCapacity	= heap_capacity;

	int * cursor = base + state_words + heap_capacity;
	b.CacheStamp		= cursor;								cursor += b.CellCount;
	b.CacheRefusal		= (unsigned int *)cursor;			cursor += b.CellCount;
	b.CacheWindow		= (unsigned int *)cursor;			cursor += b.CellCount;
	b.CacheHeight		= (float *)cursor;					cursor += b.CellCount;
	b.CacheRouteMask	= (unsigned int *)cursor;			cursor += b.CellCount;
	b.CacheHasHeight	= (unsigned char *)cursor;
	memset(b.CacheStamp,0,(size_t)(b.CellCount * (int)sizeof(int)));
	b.Generation = 1;
	return true;
}


/***********************************************************************************************
**	Hard-invalid cells, and the per-leg exemptions the plan lists.
***********************************************************************************************/

//	Does an accepted window cover this point -- the only place a road may cross water or a wall?
static unsigned int Window_At(const RouteBuilder & b,const Vector2 & world)
{
	for (int i = 0; i < b.Hydrology->Windows.Count(); ++i) {

		const HybridCrossingWindow & window = b.Hydrology->Windows[i];
		Vector2 axis = window.ApproachB - window.ApproachA;
		const float length = axis.Length();
		if (length <= 0.0f) continue;

		const float t = Clamp2((((world.X - window.ApproachA.X) * axis.X) +
										((world.Y - window.ApproachA.Y) * axis.Y)) / (length * length),0.0f,1.0f);
		const Vector2 nearest(window.ApproachA.X + (axis.X * t),window.ApproachA.Y + (axis.Y * t));

		/*
		**	The window's crossable width: the roadbed of the class actually crossing it plus the lane
		**	clearance Phase 9 reserved.  Phase 9 sized the *reservation* for a PRIMARY_2 crossing, but a
		**	HIGHWAY_4 trunk is wider than that, and a window it cannot fit through is a window it cannot
		**	use at all -- which is how a family trunk ends up with no route through its own obligation.
		*/
		const float half = (0.5f * Hybrid_Roadbed_Width(b.CurrentClass,b.P->Constants.W)) + b.P->Constants.W;
		if ((world - nearest).Length() <= half) return window.StableId;
	}
	return 0;
}


/*
**	Section 8 Phase 10's hard-infinite cell list, with the three per-leg exemptions it names: the
**	edge's own frozen entrance corridor, the edge's own resource endpoint, and an accepted crossing
**	window.
*/
/*
**	Section 8 Phase 10's hard-infinite cell list, as one classifier: the bits say *which* rules refuse a
**	cell, so the router's own question ("is this cell usable?") and the diagnostic question ("what is
**	refusing this region?") are answered by one implementation instead of two that can disagree.
**
**	The three per-leg exemptions the plan names are honoured here: the edge's own frozen entrance
**	corridor, the edge's own resource endpoint, and an accepted crossing window.
*/
enum HybridRouteRefusal
{
	HYBRID_REFUSE_UNPLACEABLE	= 1 << 0,
	HYBRID_REFUSE_BASE_DISC		= 1 << 1,
	HYBRID_REFUSE_TIBERIUM		= 1 << 2,
	HYBRID_REFUSE_WATER			= 1 << 3,		// standing water: never crossable in this generator version
	HYBRID_REFUSE_WALL			= 1 << 4,		// a wall or river channel outside an accepted window
};


static unsigned int Classify_Point(const RouteBuilder & b,const Vector2 & centre)
{
	const HybridSemanticMatrix & matrix = b.Matrices->Matrix;
	unsigned int refusals = 0;

	int cx = 0, cy = 0;
	if (!matrix.World_To_Cell(centre,&cx,&cy))						return HYBRID_REFUSE_UNPLACEABLE;
	if (!matrix.Has_Flag(cx,cy,HYBRID_FLAG_PLACEABLE))			refusals |= HYBRID_REFUSE_UNPLACEABLE;

	//	BASE_PROTECTED, except through this edge's own entrance corridor.
	for (int i = 0; i < b.Anchors->Bases.Count(); ++i) {

		const HybridBaseAnchor & base = b.Anchors->Bases[i];
		if ((centre - base.Center).Length() > base.ProtectedRadius) continue;

		bool allowed = false;
		for (int e = 0; e < b.Topology->Entrances.Count(); ++e) {

			const HybridBaseEntrance & entrance = b.Topology->Entrances[e];
			if (entrance.BaseSiteId != base.SiteId) continue;

			//	Only the corridor this leg actually uses: its own endpoint node's entrance.
			bool own = false;
			const HybridTopologyEdge * edge = b.Topology->Find_Edge(b.CurrentEdgeId);
			if (edge != nullptr) {
				if ((edge->NodeA == entrance.NodeId) || (edge->NodeB == entrance.NodeId)) own = true;
			}
			if (!own) continue;

			Vector2 axis = entrance.CorridorB - entrance.CorridorA;
			const float length = axis.Length();
			if (length <= 0.0f) continue;
			const float t = Clamp2((((centre.X - entrance.CorridorA.X) * axis.X) +
											((centre.Y - entrance.CorridorA.Y) * axis.Y)) / (length * length),0.0f,1.0f);
			const Vector2 nearest(entrance.CorridorA.X + (axis.X * t),entrance.CorridorA.Y + (axis.Y * t));
			if ((centre - nearest).Length() <= entrance.CorridorHalfWidth) allowed = true;
		}
		if (!allowed) refusals |= HYBRID_REFUSE_BASE_DISC;
	}

	//	A Tiberium hard reserve, except this leg's own field endpoint.
	for (int i = 0; i < b.Anchors->Resources.Count(); ++i) {
		const HybridResourceAnchor & anchor = b.Anchors->Resources[i];
		if ((centre - anchor.Center).Length() > anchor.ReserveRadius) continue;
		if (anchor.SiteId == b.OwnFieldSite) continue;
		refusals |= HYBRID_REFUSE_TIBERIUM;
	}

	/*
	**	Standing water is not crossable open road (Phase 6 step 20) -- but an accepted crossing window is not
	**	open road.  A window is a frozen interval across an obstruction, and the obstruction under one is
	**	whatever Phase 6 put there: a river channel, a canyon, or the standing water a channel widens into.
	**	Refusing it even inside the window left a family trunk with no route through its own obligation,
	**	because the two frozen approach points stand on the two banks and every cell between them is wet.
	*/
	if (Hybrid_Point_In_Standing_Water(*b.Hydrology,centre,nullptr,nullptr) && (Window_At(b,centre) == 0)) {
		refusals |= HYBRID_REFUSE_WATER;
	}

	/*
	**	A river channel, and an immutable *wall*, only inside an accepted window.  The wall is the wall,
	**	not the whole feature: Phase 5 distinguishes a wall from a floor in its own `LandformMask`, and a
	**	canyon floor is drivable ground -- it is what a pass window crosses *to*.
	*/
	const bool wet = Hybrid_Point_In_Water(*b.Hydrology,centre);
	const unsigned char mask = matrix.LandformMask[matrix.Index(cx,cy)];
	const unsigned char wall_bits = (unsigned char)(HYBRID_LANDFORM_MASK_CLIFF_FACE |
																	HYBRID_LANDFORM_MASK_CANYON_WALL |
																	HYBRID_LANDFORM_MASK_RAVINE_WALL);
	const bool wall = (mask & wall_bits) != 0;
	if ((wet || wall) && (Window_At(b,centre) == 0)) refusals |= HYBRID_REFUSE_WALL;

	return refusals;
}


/*
**	The same classifier over a cell, which is the router's own resolution, and over a world point, which
**	is where a spline sample actually lies.  A sample and its cell centre can disagree by half a cell, and
**	judging a sample by its cell centre refuses road that is in fact inside a crossing window's corridor.
*/
static unsigned int Classify_Cell(const RouteBuilder & b,int cell)
{
	return Classify_Point(b,b.Cell_Center(cell));
}


static bool Cell_Is_Hard_Invalid(const RouteBuilder & b,int cell)
{
	//	A leg's own frozen endpoints are legal whatever stands in them: Phase 3 placed a route waypoint
	//	under the archetype guide exemption, and Phase 9 froze it.
	if ((cell == b.StartCell) || (cell == b.GoalCell)) return false;
	return Classify_Cell(b,cell) != 0;
}


/*
**	Fill one cell's cache entry: its refusal mask, the accepted window over it, its realized height and
**	which already-routed splines pass within `W` of it.
*/
static void Fill_Cell_Cache(RouteBuilder & b,int cell)
{
	const Vector2 centre = b.Cell_Center(cell);

	b.CacheRefusal[cell]	= Classify_Point(b,centre);
	b.CacheWindow[cell]	= Window_At(b,centre);

	float height = 0.0f;
	b.CacheHasHeight[cell]	= Height_At(b,centre,&height) ? (unsigned char)1 : (unsigned char)0;
	b.CacheHeight[cell]		= height;

	unsigned int mask = 0;
	const float w = b.P->Constants.W;
	const int limit = (b.Out->Routes.Count() < 32) ? b.Out->Routes.Count() : 32;
	for (int r = 0; r < limit; ++r) {
		const HybridRoute & route = b.Out->Routes[r];
		for (int p = 0; p < route.Points.Count(); ++p) {
			if ((route.Points[p] - centre).Length() > w) continue;
			mask |= (1u << r);
			break;
		}
	}
	b.CacheRouteMask[cell]	= mask;
	b.CacheStamp[cell]		= b.Generation;
}


static void Touch_Cell(RouteBuilder & b,int cell)
{
	if (b.CacheStamp[cell] != b.Generation) Fill_Cell_Cache(b,cell);
}


static unsigned int Cached_Refusal(RouteBuilder & b,int cell)
{
	if ((cell == b.StartCell) || (cell == b.GoalCell)) return 0;
	Touch_Cell(b,cell);
	return b.CacheRefusal[cell];
}


static unsigned int Cached_Window(RouteBuilder & b,int cell)
{
	Touch_Cell(b,cell);
	return b.CacheWindow[cell];
}


static bool Cached_Height(RouteBuilder & b,int cell,float * height_out)
{
	Touch_Cell(b,cell);
	if (b.CacheHasHeight[cell] == 0) return false;
	*height_out = b.CacheHeight[cell];
	return true;
}


static unsigned int Cached_Route_Mask(RouteBuilder & b,int cell)
{
	Touch_Cell(b,cell);
	return b.CacheRouteMask[cell];
}


//	Section 7.1's vertical interval, and the grade a move between two cells would climb.
static bool Move_Grade(RouteBuilder & b,int from,int to,float * grade_out,float * distance_out)
{
	const int dx = b.Cell_X(to) - b.Cell_X(from);
	const int dy = b.Cell_Y(to) - b.Cell_Y(from);
	const float d = ((dx != 0) && (dy != 0)) ? (b.A * 1.41421356f) : b.A;

	float z_from = 0.0f;
	float z_to = 0.0f;
	if (!Cached_Height(b,from,&z_from)) return false;
	if (!Cached_Height(b,to,&z_to)) return false;

	const HybridVerticalInterval interval_from = Hybrid_Road_Vertical_Interval(z_from,b.P->MaxCutDepth,
																								  b.P->MaxFillHeight);
	const HybridVerticalInterval interval_to = Hybrid_Road_Vertical_Interval(z_to,b.P->MaxCutDepth,
																								 b.P->MaxFillHeight);
	const float gap = Hybrid_Interval_Gap(interval_from,interval_to);

	if (grade_out != nullptr)		*grade_out = (float)atan((double)(gap / d));
	if (distance_out != nullptr)	*distance_out = d;
	return true;
}


//	The distance from a point to one segment.
static float Distance_To_Segment(const Vector2 & point,const Vector2 & a,const Vector2 & b)
{
	const Vector2 axis = b - a;
	const float length_sq = (axis.X * axis.X) + (axis.Y * axis.Y);
	float t = 0.0f;
	if (length_sq > 0.0f) {
		t = Clamp2((((point.X - a.X) * axis.X) + ((point.Y - a.Y) * axis.Y)) / length_sq,0.0f,1.0f);
	}
	const float dx = point.X - (a.X + (axis.X * t));
	const float dy = point.Y - (a.Y + (axis.Y * t));
	return (float)sqrt((double)((dx * dx) + (dy * dy)));
}


//	How far a point sits outside the edge's own intent corridor, in units of `S`.
static float Corridor_Excess(const RouteBuilder & b,const Vector2 & world)
{
	float distance = 3.4e38f;

	if (b.CorridorPath.Count() >= 2) {
		for (int i = 0; i < (b.CorridorPath.Count() - 1); ++i) {
			distance = Min2(distance,Distance_To_Segment(world,b.CorridorPath[i],b.CorridorPath[i + 1]));
		}
	} else {
		distance = Distance_To_Segment(world,b.CorridorA,b.CorridorB);
	}
	if (distance >= 3.4e38f) return 0.0f;
	return Max2(0.0f,distance - b.CurrentHalfWidth) / Max2(b.P->Constants.S,b.P->Constants.Q);
}


/*
**	The two collinear rules.  Both are about running *along* another road rather than crossing it, so
**	both are measured on the move itself: the move's own cells lie within `W` of the other centerline
**	and the move continues in the direction it arrived from, which at analysis spacing `A` already
**	covers more than the `6W` the plan names (`2A` is 30 world units against `6W` of about 20).
*/
//	Declared with the A* below, which owns the state encoding this walk follows.
static int State_Index(const RouteBuilder & b,int cell,int direction);


/*
**	The two collinear rules, measured as the plan states them: the *run* along another road, not the
**	single move.  "Collinear within `W` of a different heavy-family centerline for `>6W`" is a length, so
**	the move's own cells are tested against that road and then the parent chain is walked back while it
**	keeps hugging the same road, accumulating the run.  Only a run longer than `6W` is hard-invalid.
**
**	`route_out` reports which road was being followed, so the walk measures one road rather than hopping
**	between two that happen to pass nearby.
*/
static int Route_Followed_By_Move(RouteBuilder & b,int from,int to,bool different_family_only)
{
	const unsigned int shared = Cached_Route_Mask(b,from) & Cached_Route_Mask(b,to);
	if (shared == 0) return -1;

	const int limit = (b.Out->Routes.Count() < 32) ? b.Out->Routes.Count() : 32;
	for (int r = 0; r < limit; ++r) {

		if ((shared & (1u << r)) == 0) continue;
		const HybridRoute & route = b.Out->Routes[r];
		if (route.EdgeId == b.CurrentEdgeId) continue;
		if (different_family_only) {
			if ((route.RouteFamily < 0) || (b.CurrentFamily < 0)) continue;
			if (route.RouteFamily == b.CurrentFamily) continue;
		}
		return r;
	}
	return -1;
}


static bool Cell_Hugs_Route(RouteBuilder & b,int cell,int route_index)
{
	if ((route_index < 0) || (route_index >= 32)) return false;
	return (Cached_Route_Mask(b,cell) & (1u << route_index)) != 0;
}


//	Is a base's protected disc over this cell?  The collinear rules exclude the contraction zones.
static bool Cell_In_Base_Contraction(RouteBuilder & b,int cell)
{
	const Vector2 point = b.Cell_Center(cell);
	for (int i = 0; i < b.Anchors->Bases.Count(); ++i) {
		const HybridBaseAnchor & base = b.Anchors->Bases[i];
		if ((point - base.Center).Length() <= base.ProtectedRadius) return true;
	}
	return false;
}


static bool Move_Runs_Along_Existing_Road(RouteBuilder & b,int state,int from,int to,
														  bool different_family_only)
{
	const int route_index = Route_Followed_By_Move(b,from,to,different_family_only);
	if (route_index < 0) return false;

	//	A run that starts or ends inside a base contraction zone is exempt (the plan says so explicitly).
	if (Cell_In_Base_Contraction(b,from) || Cell_In_Base_Contraction(b,to)) return false;

	const float limit = COLLINEAR_RUN_W * b.P->Constants.W;
	float run = (b.Cell_Center(to) - b.Cell_Center(from)).Length();
	if (run > limit) return true;

	//	Walk the parent chain back while it keeps hugging the same road.
	int walk = state;
	int guard = b.CellCount;
	int current = from;

	while ((walk >= 0) && (guard-- > 0)) {

		const int parent_cell = b.States[walk].FromCell;
		const int parent_direction = b.States[walk].FromDirection;
		if (parent_cell < 0) break;
		if (Cell_In_Base_Contraction(b,parent_cell)) break;
		if (!Cell_Hugs_Route(b,parent_cell,route_index)) break;

		run += (b.Cell_Center(current) - b.Cell_Center(parent_cell)).Length();
		if (run > limit) return true;

		current	= parent_cell;
		walk		= State_Index(b,parent_cell,parent_direction);
	}
	return false;
}


//	A dirt or trail move that follows an already-routed compatible road earns the reuse modifier.
static bool Move_Reuses_Road(RouteBuilder & b,int to)
{
	if ((b.CurrentClass != HYBRID_ROAD_SERVICE_DIRT_2) && (b.CurrentClass != HYBRID_ROAD_TRAIL_1)) return false;
	return Cached_Route_Mask(b,to) != 0;
}


/***********************************************************************************************
**	The heading-aware A*.
***********************************************************************************************/

static int State_Index(const RouteBuilder & b,int cell,int direction)
{
	return (cell * STATE_COUNT) + direction;
}


static bool Heap_Less(const RouteBuilder & b,int left,int right)
{
	const RouteSearchState & a = b.States[left];
	const RouteSearchState & c = b.States[right];

	//	The plan's open-set key: (f, g, row-major cell id, incoming direction ordinal).
	if (a.F != c.F) return a.F < c.F;
	if (a.G != c.G) return a.G < c.G;
	const int cell_a = left / STATE_COUNT;
	const int cell_c = right / STATE_COUNT;
	if (cell_a != cell_c) return cell_a < cell_c;
	return (left % STATE_COUNT) < (right % STATE_COUNT);
}


static void Heap_Push(RouteBuilder & b,int state)
{
	//	A full heap is a bounded, named failure, never a write past the block.
	if (b.HeapCount >= b.HeapCapacity) {
		b.HeapOverflowed = true;
		return;
	}

	int index = b.HeapCount++;
	b.Heap[index] = state;
	while (index > 0) {
		const int parent = (index - 1) / 2;
		if (!Heap_Less(b,b.Heap[index],b.Heap[parent])) break;
		const int swap = b.Heap[parent];
		b.Heap[parent]	= b.Heap[index];
		b.Heap[index]	= swap;
		index = parent;
	}
}


static int Heap_Pop(RouteBuilder & b)
{
	if (b.HeapCount <= 0) return -1;

	const int top = b.Heap[0];
	b.HeapCount--;
	if (b.HeapCount > 0) {
		b.Heap[0] = b.Heap[b.HeapCount];
		int index = 0;
		for (;;) {
			const int left = (2 * index) + 1;
			const int right = left + 1;
			int best = index;
			if ((left < b.HeapCount) && Heap_Less(b,b.Heap[left],b.Heap[best]))		best = left;
			if ((right < b.HeapCount) && Heap_Less(b,b.Heap[right],b.Heap[best]))	best = right;
			if (best == index) break;
			const int swap = b.Heap[best];
			b.Heap[best]	= b.Heap[index];
			b.Heap[index]	= swap;
			index = best;
		}
	}
	return top;
}


/*
**	How far the road has run straight since its last heading change, measured back along the parent chain.
**	A turn is legal only when that run is at least the class minimum turn radius.
*/
/*
**	Which way a turn goes, as a signed step between two 8-way directions: +1 counter-clockwise, -1
**	clockwise, 0 for no turn.  Two turns the *same* way compound into one broader bend, and that bend is
**	what a centripetal Catmull-Rom rounds into a radius well under either turn's own -- so same-way turns
**	need twice the straight between them that an S-curve does.
*/
static int Turn_Sign(int from_direction,int to_direction)
{
	if ((from_direction < 0) || (from_direction >= DIRECTION_COUNT)) return 0;
	if (from_direction == to_direction) return 0;

	int difference = to_direction - from_direction;
	while (difference < 0) difference += DIRECTION_COUNT;
	if (difference == 0) return 0;
	return (difference <= (DIRECTION_COUNT / 2)) ? 1 : -1;
}


static bool Turn_Spacing_Is_Legal(RouteBuilder & b,int state,int incoming,float min_turn_radius,int turn_sign)
{
	if (min_turn_radius <= 0.0f) return true;

	const float isolated = TURN_SPACING_ISOLATED * min_turn_radius;
	const float arc = TURN_SPACING_ARC * min_turn_radius;

	float run = 0.0f;
	int walk = state;
	int guard = b.CellCount;

	while ((walk >= 0) && (guard-- > 0)) {

		const int cell = walk / STATE_COUNT;
		const int parent_cell = b.States[walk].FromCell;
		const int parent_direction = b.States[walk].FromDirection;
		if (parent_cell < 0) {

			//	The route's own start: there is nothing behind it, frozen or otherwise.
			if (b.EntryDirection == START_DIRECTION) return true;

			//	A leg boundary: carry the previous leg's heading and its trailing straight across it.
			run += b.EntryRun;
			if (run >= isolated) return true;
			if (incoming != b.EntryDirection) {
				const int boundary_sign = Turn_Sign(b.EntryDirection,incoming);
				if (boundary_sign != turn_sign) return false;
				return run >= arc;
			}
			return true;											// straight through it: nothing to crowd
		}

		run += (b.Cell_Center(cell) - b.Cell_Center(parent_cell)).Length();
		if (run >= isolated) return true;						// room to round this corner on its own

		//	The move into `cell` was itself a turn: this one continues it only if it goes the same way.
		if ((parent_direction != START_DIRECTION) && (parent_direction != incoming)) {
			const int previous_sign = Turn_Sign(parent_direction,incoming);
			if (previous_sign != turn_sign) return false;		// an S inside one corner's room
			return run >= arc;										// an arc of same-way steps
		}

		incoming	= parent_direction;
		walk		= State_Index(b,parent_cell,parent_direction);
	}
	return true;
}


//	Declared with the spline conversion below, which is the other place this measure is used.
static float Curvature_Radius(const Vector2 & a,const Vector2 & b,const Vector2 & c);


/*
**	The same curvature measure the spline check uses, applied to the chain the search is building: the
**	circumradius over a baseline of the class minimum turn radius.  Search and check then agree, so a bend
**	the check would reject is never built in the first place -- which is what the three-envelope escalation
**	is for, rather than for discovering a geometry the search could have avoided.
*/
static bool Chain_Curvature_Is_Legal(RouteBuilder & b,int state,int neighbor,float min_radius)
{
	if (min_radius <= 0.0f) return true;

	/*
	**	The baseline is the straight a corner needs, not the radius itself: the chain is measured over the
	**	same window the turn-spacing rule reserves, so a zig-zag inside that window is rejected rather than
	**	averaged away by a baseline shorter than the bend.
	*/
	const int span = (int)Hybrid_Round_Half_Up((CURVATURE_BASELINE_FACTOR * min_radius) /
															  Max2(b.A,b.P->Constants.Q));
	const int baseline = (span > 1) ? span : 1;

	//	Walk back `2 * baseline` cells; the midpoint of that walk is the corner under test.
	int walk = state;
	int steps = 0;
	int mid_cell = -1;
	int far_cell = -1;

	while ((walk >= 0) && (steps < (2 * baseline))) {

		const int cell = walk / STATE_COUNT;
		const int parent_cell = b.States[walk].FromCell;
		const int parent_direction = b.States[walk].FromDirection;
		if (parent_cell < 0) return true;					// not enough road yet to describe a bend

		steps++;
		if (steps == baseline)		mid_cell = cell;
		if (steps == (2 * baseline))	far_cell = parent_cell;

		walk = State_Index(b,parent_cell,parent_direction);
	}
	if ((mid_cell < 0) || (far_cell < 0)) return true;

	return Curvature_Radius(b.Cell_Center(far_cell),b.Cell_Center(mid_cell),
									b.Cell_Center(neighbor)) >= min_radius;
}


/*
**	The spline check's cross-slope test, applied to the move that would put road there.  Same reason as the
**	curvature rule: a search that ignores a constraint the check enforces spends three envelopes finding
**	out, and on a hillside steeper than the class can be graded into there is nothing to find.
*/
static bool Cross_Slope_Is_Legal(RouteBuilder & b,int from,int to)
{
	const HybridConstants & k = b.P->Constants;
	const float half_roadbed = 0.5f * Hybrid_Roadbed_Width(b.CurrentClass,k.W);
	const float max_cross = Hybrid_Road_Class_Spec(b.CurrentClass).MaxCrossSlopeDegrees * PI_OVER_180;
	if (half_roadbed <= 0.0f) return true;

	Vector2 axis = b.Cell_Center(to) - b.Cell_Center(from);
	if (axis.Length() <= 0.0f) return true;
	axis *= (1.0f / axis.Length());
	const Vector2 normal(-axis.Y,axis.X);
	const Vector2 centre = b.Cell_Center(to);

	/*
	**	The spline does not run through cell centres -- it can sit half a cell off them -- so the move is
	**	tested where the road may actually land: at the cell centre and half a cell either side along the
	**	move.  Otherwise the search approves ground the spline check then refuses, which is what spent all
	**	three envelopes on one hillside.
	*/
	for (int station = -1; station <= 1; ++station) {

		const Vector2 point = centre + (axis * ((float)station * 0.5f * b.A));

		float left = 0.0f;
		float right = 0.0f;
		if (!Height_At(b,point + (normal * half_roadbed),&left)) continue;
		if (!Height_At(b,point - (normal * half_roadbed),&right)) continue;

		const HybridVerticalInterval interval_left = Hybrid_Road_Vertical_Interval(left,b.P->MaxCutDepth,
																									  b.P->MaxFillHeight);
		const HybridVerticalInterval interval_right = Hybrid_Road_Vertical_Interval(right,b.P->MaxCutDepth,
																										b.P->MaxFillHeight);
		if (!Hybrid_Grade_Compatible(interval_left,interval_right,2.0f * half_roadbed,max_cross)) return false;
	}
	return true;
}


static float Heading_Change(int incoming,int direction)
{
	if (incoming == START_DIRECTION) return 0.0f;

	int difference = incoming - direction;
	while (difference < 0) difference += DIRECTION_COUNT;
	if (difference > (DIRECTION_COUNT / 2)) difference = DIRECTION_COUNT - difference;
	return ((float)difference / (float)DIRECTION_COUNT) * TWO_PI;
}


/*
**	One A* leg: from `start` to `goal`, both analysis cells, inside the current envelope.  The chain is
**	appended to `chain` without repeating the start when it is already the last element.
*/
//	Declared with the spline conversion below, which owns the chain's direction encoding.
static int Chain_Direction(const RouteBuilder & b,int from,int to);


/*
**	One A* leg.  `entry_direction` is the heading the road arrives on from the previous leg, so the turn
**	rules apply across a leg boundary: a leg that started with no heading could turn as sharply as it liked
**	immediately after a frozen waypoint, and a waypoint is exactly where the corners are.
*/
static bool Search_Leg(RouteBuilder & b,int start,int goal,int entry_direction,
								DynamicVectorClass<int> * chain,HybridRouteResult * result)
{
	b.StartCell	= start;
	b.GoalCell	= goal;
	b.Generation++;					// a new leg: every cached answer belongs to the previous one

	const HybridConstants & k = b.P->Constants;
	const HybridRouteCostWeights weights = Hybrid_Route_Cost_Weights(b.CurrentClass);
	const float max_grade = Max2(Hybrid_Road_Class_Spec(b.CurrentClass).MaxGradeDegrees * PI_OVER_180,k.Q);
	const float min_turn_radius = b.StructuralLeg ? 0.0f
																: (Hybrid_Road_Class_Spec(b.CurrentClass).MinTurnRadiusW *
																	k.W);
	const float window_penalty = WINDOW_PENALTY_A * b.A;

	for (int i = 0; i < (b.CellCount * STATE_COUNT); ++i) {
		b.States[i].G				= -1.0f;
		b.States[i].F				= 0.0f;
		b.States[i].FromCell		= -1;
		b.States[i].FromDirection= -1;
		b.States[i].Open			= 0;
		b.States[i].Closed		= 0;
	}
	b.HeapCount			= 0;
	b.HeapOverflowed	= false;


	const Vector2 goal_point = b.Cell_Center(goal);
	const int start_state = State_Index(b,start,entry_direction);
	b.States[start_state].G = 0.0f;
	b.States[start_state].F = HEURISTIC_WEIGHT * (goal_point - b.Cell_Center(start)).Length();
	b.States[start_state].Open = 1;
	Heap_Push(b,start_state);

	int found = -1;

	while ((b.HeapCount > 0) && (found < 0)) {

		const int state = Heap_Pop(b);
		if (state < 0) break;
		if (b.States[state].Closed != 0) continue;
		b.States[state].Closed	= 1;
		b.States[state].Open		= 0;

		const int cell = state / STATE_COUNT;
		const int incoming = state % STATE_COUNT;

		if (cell == goal) {
			found = state;
			break;
		}

		b.Expansions++;
		b.Out->TotalExpansions++;
		if (b.P->Counters != nullptr) b.P->Counters->Add(HYBRID_WORK_ASTAR_EXPANSIONS,1);
		if ((b.P->AStarExpansionsPerRoute > 0) && (b.Expansions > b.P->AStarExpansionsPerRoute)) {
			Fail(result,HYBRID_ROUTE_EXPANSION_BUDGET,"the per-route A* expansion ceiling was reached");
			return false;
		}
		if ((b.P->AStarExpansionsPerAttempt > 0) && (b.Out->TotalExpansions > b.P->AStarExpansionsPerAttempt)) {
			Fail(result,HYBRID_ROUTE_EXPANSION_BUDGET,"the per-attempt A* expansion ceiling was reached");
			return false;
		}

		const int x = b.Cell_X(cell);
		const int y = b.Cell_Y(cell);

		for (int direction = 0; direction < DIRECTION_COUNT; ++direction) {

			const int nx = x + NEIGHBOR_DX[direction];
			const int ny = y + NEIGHBOR_DY[direction];
			if (!b.Valid_Cell(nx,ny)) continue;

			const int neighbor = b.Cell_Index(nx,ny);
			if (Cached_Refusal(b,neighbor) != 0) continue;
			const Vector2 neighbor_centre = b.Cell_Center(neighbor);

			float grade = 0.0f;
			float d = 0.0f;
			if (!Move_Grade(b,cell,neighbor,&grade,&d)) continue;

			/*
			**	The ordinary grade test does not apply to a move into or out of a crossing structure's own
			**	cells.  Section 8 Phase 10 says so in as many words: "For a bridge/tunnel/overpass obligation
			**	cell, this phase only routes through the frozen XY cell using the ordinary feasibility
			**	interval.  Exact structural endpoint elevations are not needed to choose the horizontal path
			**	and are compiled in the immediately following interaction phase."  A bridge deck spans the
			**	drop; judging that move by the terrain under it is what made every route across a canyon or
			**	river fail with no path at all.
			*/
			const bool structural = (Cached_Window(b,cell) != 0) || (Cached_Window(b,neighbor) != 0);
			if (!structural && (grade > max_grade)) continue;
			if (!structural && !Cross_Slope_Is_Legal(b,cell,neighbor)) continue;

			/*
			**	A heading-aware A* has to honour the class's own turn radius in the search, not only in the
			**	spline check afterwards.  A corner needs straight road around it: two turns closer together
			**	than the class minimum turn radius describe a bend no spline through those cells can round, and
			**	discovering that in the spline check costs the whole edge three envelopes and then the attempt.
			**	Measured on the failing seeds the splines came out at 26.5 against the 26.6 a `HIGHWAY_4`
			**	needs and 16.5 against the 16.6 a `PRIMARY_2` needs -- both of them this rule, one phase late.
			*/
			if ((incoming != START_DIRECTION) && (direction != incoming)) {

				/*
				**	How sharp one cell's corner may be.  An 8-way step is 45 degrees, and a class whose minimum
				**	turn radius is wider than an analysis cell cannot turn twice inside one: a 90-degree corner
				**	at a single cell needs about five times the control spacing a 45-degree one does, and the
				**	curve through it comes out at a fifth of that spacing whatever the runs either side measure
				**	-- on the failing seed, 21.9 where 26.6 was needed, with 138 units of straight in front of
				**	it.  A class that turns inside a cell keeps the full eight-way freedom.
				*/
				int steps = direction - incoming;
				while (steps > (DIRECTION_COUNT / 2)) steps -= DIRECTION_COUNT;
				while (steps < -(DIRECTION_COUNT / 2)) steps += DIRECTION_COUNT;
				if ((min_turn_radius > b.A) && ((steps > 1) || (steps < -1))) continue;

				if (!Turn_Spacing_Is_Legal(b,state,incoming,min_turn_radius,
													Turn_Sign(incoming,direction))) continue;
			}
			/*
			**	The search holds itself to a margin above the class minimum, because it measures the *chain*
			**	while the check measures the *spline* through it: a centripetal Catmull-Rom sits inside its
			**	control polygon and comes out a few percent tighter than the cell centres suggest.  The check
			**	stays exact; only the search is conservative, so it never builds a bend the check will reject.
			*/
			if (!Chain_Curvature_Is_Legal(b,state,neighbor,CURVATURE_SEARCH_MARGIN * min_turn_radius)) continue;

			//	The two collinear rules: running along another heavy family, or along any road.
			if (b.CurrentFamily >= 0) {
				if (Move_Runs_Along_Existing_Road(b,state,cell,neighbor,true)) continue;
			}
			if (b.CurrentIsOptional) {
				if (Move_Runs_Along_Existing_Road(b,state,cell,neighbor,false)) continue;
			}

			const Vector2 & neighbor_point = neighbor_centre;
			const float heading = Heading_Change(incoming,direction);
			const float excess = Corridor_Excess(b,neighbor_point);

			/*
			**	The envelope bounds the *search*, not only its cost: Section 8 Phase 10 says an edge gets at
			**	most three of them and that "no search may leave the third envelope".  Keeping it as a cost
			**	term let the search wander outside and hand the spline check a path it then had to refuse,
			**	which spent all three envelopes discovering what the first one already knew.
			*/
			if (excess > 0.0f) continue;

			float cost = d;
			cost += d * weights.Ks * (float)pow((double)(grade / max_grade),4.0);
			cost += b.A * weights.Kt * (float)pow((double)(heading / 3.14159265358979323846f),2.0);
			cost += b.A * weights.Kc * (excess * excess);

			//	Entering a window this leg is not obliged to use is penalized, never forbidden.
			const unsigned int window = Cached_Window(b,neighbor);
			if ((window != 0) && (window != b.OwnWindow)) cost += window_penalty;

			if (Move_Reuses_Road(b,neighbor)) cost += REUSE_MODIFIER * d;

			const int neighbor_state = State_Index(b,neighbor,direction);
			const float g = b.States[state].G + cost;
			const float quantized = Hybrid_Quantize_Position(g);

			if (b.States[neighbor_state].G >= 0.0f) {
				//	Reopening is allowed only on a strictly lower quantized g.
				if (quantized >= Hybrid_Quantize_Position(b.States[neighbor_state].G)) continue;
			}

			b.States[neighbor_state].G					= g;
			b.States[neighbor_state].F					= g + (HEURISTIC_WEIGHT *
																			(goal_point - neighbor_point).Length());
			b.States[neighbor_state].FromCell			= cell;
			b.States[neighbor_state].FromDirection	= incoming;
			b.States[neighbor_state].Closed			= 0;
			b.States[neighbor_state].Open				= 1;
			Heap_Push(b,neighbor_state);
		}
	}

	if (found < 0) {
		//	How far the search actually got, for the failure detail.
		b.LastExpansions	= b.Expansions;
		b.LastGoalTouched	= 0;
		b.LastClosed		= 0;
		for (int direction = 0; direction < STATE_COUNT; ++direction) {
			if (b.States[State_Index(b,goal,direction)].G >= 0.0f) b.LastGoalTouched++;
		}
		for (int i = 0; i < (b.CellCount * STATE_COUNT); ++i) {
			if (b.States[i].Closed != 0) b.LastClosed++;
		}

		if (b.HeapOverflowed) {
			Fail(result,HYBRID_ROUTE_EXPANSION_BUDGET,"the A* open set exceeded its own capacity");
		}
		return false;
	}

	//	Walk the parents back, then append in forward order.
	DynamicVectorClass<int> reverse;
	int state = found;
	int guard = b.CellCount * STATE_COUNT;
	while ((state >= 0) && (guard-- > 0)) {
		const int cell = state / STATE_COUNT;
		reverse.Add(cell);
		const int from_cell = b.States[state].FromCell;
		const int from_direction = b.States[state].FromDirection;
		if (from_cell < 0) break;
		state = State_Index(b,from_cell,from_direction);
	}

	for (int i = reverse.Count() - 1; i >= 0; --i) {
		if ((chain->Count() > 0) && ((*chain)[chain->Count() - 1] == reverse[i])) continue;
		chain->Add(reverse[i]);
	}
	return true;
}


/***********************************************************************************************
**	The deterministic path-to-spline conversion.
***********************************************************************************************/

//	The 8-way direction from one cell to the next, or -1 when they are not neighbours.
static int Chain_Direction(const RouteBuilder & b,int from,int to)
{
	const int dx = b.Cell_X(to) - b.Cell_X(from);
	const int dy = b.Cell_Y(to) - b.Cell_Y(from);
	for (int direction = 0; direction < DIRECTION_COUNT; ++direction) {
		if ((NEIGHBOR_DX[direction] == dx) && (NEIGHBOR_DY[direction] == dy)) return direction;
	}
	return -1;
}


/***********************************************************************************************
**	The road centerline: straights and circular arcs.
**
**	Phase 10 has to produce a centerline that honours the class minimum turn radius -- `8W` for a
**	`HIGHWAY_4`, which is 26.6 on a world whose analysis cell is 15.  A curve drawn *through the cell
**	centres of an 8-way A\* chain* cannot do that except by accident: the chain's only turn granularity is
**	45 degrees at a cell vertex, so the curvature of any spline through it is decided by which cells the
**	search happened to pick.  Measured on the fixture seeds, that accident came out between 8.3 and 26.4
**	where 16.6 and 26.6 were needed, and every way of steering it -- thinning the controls, spacing the
**	chain's turns, raising the search's own margin -- moved the failure somewhere else instead of ending it.
**
**	So the chain is read as what it is: a *corridor*, a proof that a road can get from one frozen point to
**	the next through legal ground.  The centerline is then built as geometry that satisfies the radius by
**	construction -- straight runs joined by circular arcs of exactly the class radius -- and the Section-7
**	limit is a property of the curve rather than a test it has to pass by luck.  It is still measured
**	exactly, on the sampled polyline, with no tolerance beyond the project quantization scale.
**
**	Two things constrain where the arcs may go.  A frozen point -- a base entrance, a field access point, a
**	crossing's approach point -- must be interpolated exactly, and an arc that rounds a corner does not pass
**	through the corner; so the polyline is made to run *straight* through every frozen interior point, by
**	giving it a lead-in and a lead-out along its own axis, and the arcs then sit on the open road outside
**	them.  And an arc needs room: a turn of `theta` needs `R tan(theta/2)` of straight on each side of it.
**	Where two corners are too close to share the straight between them, the shallower of the two is dropped
**	-- it is a corridor vertex, not a frozen position -- until every arc fits.  If the two corners crowding
**	each other are both frozen, the route genuinely cannot be built at this radius, and that is reported as
**	such rather than papered over.
***********************************************************************************************/

//	One vertex of the corridor polyline the centerline is fitted to.
struct RouteVertex
{
	RouteVertex(void) : P(0.0f,0.0f),Axis(0.0f,0.0f),Frozen(false),WindowId(0),ChainIndex(-1),
								StructuralAhead(false) { }

	//	DynamicVectorClass<T> requires == and !=.  The position is this vertex's identity.
	bool	operator == (const RouteVertex & that) const	{ return P == that.P; }
	bool	operator != (const RouteVertex & that) const	{ return !(*this == that); }

	Vector2			P;
	Vector2			Axis;					// for a frozen vertex, the direction the road must run through it
	bool				Frozen;				// an earlier phase fixed this position: never dropped, never cut
	unsigned int	WindowId;			// non-zero: a crossing approach point
	int				ChainIndex;			// the chain cell this came from, -1 when it is not one
	bool				StructuralAhead;	// the leg leaving this vertex is inside a crossing
};


static Vector2 Unit(const Vector2 & v)
{
	const float length = v.Length();
	if (length <= 1.0e-6f) return Vector2(0.0f,0.0f);
	return Vector2(v.X / length,v.Y / length);
}


//	The turn at a polyline vertex, in radians from 0 (straight on) to pi (a reversal).
static float Vertex_Turn(const Vector2 & previous,const Vector2 & here,const Vector2 & next)
{
	const Vector2 incoming = Unit(here - previous);
	const Vector2 outgoing = Unit(next - here);
	if ((incoming.Length() <= 0.0f) || (outgoing.Length() <= 0.0f)) return 0.0f;

	float dot = (incoming.X * outgoing.X) + (incoming.Y * outgoing.Y);
	dot = Clamp2(dot,-1.0f,1.0f);
	return (float)::acos((double)dot);
}


/*
**	One crossing this route makes: which window, the frozen approach point it enters by, the one it leaves
**	by, and the stretch of chain that *is* the crossing.
**
**	It is not deduced from the chain afterwards.  Phase 10 routes a crossing as a leg of its own, from one
**	frozen approach point to the other, so the leg that built it says what it is -- which window, which bank
**	first, and which chain cells.  Deducing it instead, by looking for the approach cells in the finished
**	chain, gets it wrong in both directions: a route that only clips a window's corridor looks like a
**	crossing, and a genuine crossing whose bank cell centre falls outside the corridor does not.
*/
struct RouteCrossing
{
	RouteCrossing(void) : WindowId(0),FirstIndex(0),LastIndex(0),Ingress(0.0f,0.0f),Egress(0.0f,0.0f) { }

	//	DynamicVectorClass<T> requires == and !=.  The window is this record's identity within a route.
	bool	operator == (const RouteCrossing & that) const	{ return WindowId == that.WindowId; }
	bool	operator != (const RouteCrossing & that) const	{ return !(*this == that); }

	unsigned int	WindowId;
	int				FirstIndex;			// the chain cell the near bank's approach point stands in
	int				LastIndex;			// the chain cell the far bank's approach point stands in
	Vector2			Ingress;
	Vector2			Egress;
};


/*
**	Drop any vertex the road would have to double back to reach.
**
**	An 8-way chain turning through a switchback leaves one, and so does a frozen point that sits ahead of
**	its own cell centre, or a lead point placed between a corridor vertex and the frozen run behind it.  The
**	road simply does not go there, so the vertex goes.  A frozen position is never dropped: a reversal
**	between two of those is a real conflict, and the feasibility pass reports it as one.
**
**	The pass runs on whatever polyline it is given, and it is given each of them -- the corridor as first
**	built, and again once the frozen runs have their lead-ins and lead-outs -- because inserting a vertex
**	changes the angles at its neighbours and can make a reversal that was not there before.
*/
static void Drop_Reversals(DynamicVectorClass<RouteVertex> * vertices)
{
	const float reversal = 170.0f * (3.14159265358979323846f / 180.0f);

	bool again = true;
	while (again) {

		again = false;
		for (int v = 1; v < (vertices->Count() - 1); ++v) {

			if ((*vertices)[v].Frozen) continue;
			if (Vertex_Turn((*vertices)[v - 1].P,(*vertices)[v].P,(*vertices)[v + 1].P) < reversal) continue;

			DynamicVectorClass<RouteVertex> kept;
			for (int k = 0; k < vertices->Count(); ++k) {
				if (k == v) continue;
				kept.Add((*vertices)[k]);
			}
			vertices->Delete_All();
			for (int k = 0; k < kept.Count(); ++k) vertices->Add(kept[k]);
			again = true;
			break;
		}
	}
}


/*
**	Step 1 and step 2, as a corridor polyline: the frozen start, every cell where the chain changes
**	direction, every cell an earlier phase froze, every cell step 5 inserted, and the frozen end.
**
**	A crossing is not a run of cells here but its own two frozen approach points, in travel order, with the
**	interval between them marked structural.  The chain cells between the two banks are the crossing, and
**	the crossing's shape is the frozen interval, not whatever cells the search stepped through to get there.
*/
/*
**	Crossings the search made without being sent to make them.
**
**	A cell whose `LandformMask` carries a cliff face or a canyon or ravine wall is hard-invalid *unless it
**	lies inside an accepted window*, so a window is the only hole in an immutable barrier -- and a route is
**	free to use one it was never given as a stop.  When it does, the crossing is just as real as an obliged
**	one: the road has to get across that wall, and Phase 11 has to choose how.  Recording only the legs that
**	were planned as crossings left those traversals invisible, so Phase 11 compiled nothing for them and
**	Phase 12 would have laid road across an unmodified cliff face.  Measured over 200 seeds of every
**	archetype, 40 solved worlds crossed a cliff obstruction this way and not one road cut was compiled.
**
**	What makes it a crossing is what makes any crossing: the chain enters a window's footprint on one side
**	of its axis and leaves on the other.  The record is the window's own frozen approach pair, oriented by
**	the side the route arrived from, so the road crosses on the axis Phase 6 reserved -- exactly as a planned
**	crossing does, because it is the same crossing.
*/
static void Note_Unplanned_Crossings(RouteBuilder & b,const DynamicVectorClass<int> & chain,
												 DynamicVectorClass<RouteCrossing> * crossings)
{
	if (crossings == nullptr) return;

	int index = 0;
	while (index < chain.Count()) {

		const unsigned int id = Cached_Window(b,chain[index]);
		if (id == 0) {
			index++;
			continue;
		}

		//	The whole run of chain cells inside this one window.
		int last = index;
		while (((last + 1) < chain.Count()) && (Cached_Window(b,chain[last + 1]) == id)) last++;

		const int first_vertex = (index > 0) ? (index - 1) : index;
		const int last_vertex = ((last + 1) < chain.Count()) ? (last + 1) : last;

		bool covered = false;
		for (int c = 0; (c < crossings->Count()) && !covered; ++c) {
			const RouteCrossing & already = (*crossings)[c];
			if (already.WindowId == id) covered = true;
			if ((first_vertex <= already.LastIndex) && (last_vertex >= already.FirstIndex)) covered = true;
		}

		const HybridCrossingWindow * window = b.Hydrology->Find_Window(id);
		if (!covered && (window != nullptr) && (last_vertex > first_vertex)) {

			//	Did it come out the other side?  A road along the rim enters and leaves on one side.
			const Vector2 normal(-window->Tangent.Y,window->Tangent.X);
			const Vector2 entered = b.Cell_Center(chain[first_vertex]) - window->Center;
			const Vector2 left = b.Cell_Center(chain[last_vertex]) - window->Center;
			const float across_in = (entered.X * normal.X) + (entered.Y * normal.Y);
			const float across_out = (left.X * normal.X) + (left.Y * normal.Y);

			if (((across_in < 0.0f) && (across_out > 0.0f)) ||
				 ((across_in > 0.0f) && (across_out < 0.0f))) {

				RouteCrossing record;
				record.WindowId	= id;
				record.FirstIndex	= first_vertex;
				record.LastIndex	= last_vertex;

				//	`ApproachA` stands on the `+normal` side, so the side it arrived from names the ingress.
				const bool from_a = (across_in > 0.0f);
				record.Ingress		= from_a ? window->ApproachA : window->ApproachB;
				record.Egress		= from_a ? window->ApproachB : window->ApproachA;

				//	Kept in chain order, because the vertex builder matches on `FirstIndex`.
				int slot = crossings->Count();
				crossings->Add(record);
				while ((slot > 0) && ((*crossings)[slot - 1].FirstIndex > record.FirstIndex)) {
					(*crossings)[slot] = (*crossings)[slot - 1];
					slot--;
				}
				(*crossings)[slot] = record;
			}
		}

		index = last + 1;
	}
}


static void Build_Route_Vertices(RouteBuilder & b,const DynamicVectorClass<int> & chain,
											const DynamicVectorClass<int> & frozen_cells,
											const DynamicVectorClass<int> & inserted,
											const DynamicVectorClass<RouteCrossing> & crossings,
											const Vector2 & first_point,const Vector2 & last_point,
											DynamicVectorClass<RouteVertex> * out)
{
	out->Delete_All();
	if (chain.Count() == 0) return;

	const float quantum = Max2(b.P->Constants.Q,1.0e-3f);

	RouteVertex start;
	start.P			= Vector2(Hybrid_Quantize_Position(first_point.X),Hybrid_Quantize_Position(first_point.Y));
	start.Frozen	= true;
	out->Add(start);

	/*
	**	The cells the frozen endpoints stand in are not corridor vertices.  A frozen point sits up to half a
	**	cell from its own cell centre, and where it sits *ahead* of that centre along the route, taking both
	**	makes the polyline step backwards before it starts -- measured on the fixture seed, a three-unit step
	**	back out of a base entrance, which reads as a 174-degree corner.
	*/
	const int first_cell = b.Cell_Of(first_point);
	const int last_cell = b.Cell_Of(last_point);

	int i = 0;
	while (i < chain.Count()) {

		const int cell = chain[i];

		/*
		**	A genuinely crossed window: emit its two frozen approach points and skip the cells between the
		**	banks.  Both points carry the crossing axis, so the polyline runs straight through both of them.
		*/
		/*
		**	A crossing starts here: its two frozen approach points, in travel order, with the interval between
		**	them marked structural and the chain cells between the banks skipped.  The road's shape across an
		**	obstruction is the frozen interval, not the cells the search stepped through to get over it.
		*/
		int crossing = -1;
		for (int c = 0; (c < crossings.Count()) && (crossing < 0); ++c) {
			if (crossings[c].FirstIndex == i) crossing = c;
		}

		if (crossing >= 0) {

			const RouteCrossing & record = crossings[crossing];
			const Vector2 axis = Unit(record.Egress - record.Ingress);

			RouteVertex in_vertex;
			in_vertex.P					= Vector2(Hybrid_Quantize_Position(record.Ingress.X),
													  Hybrid_Quantize_Position(record.Ingress.Y));
			in_vertex.Axis				= axis;
			in_vertex.Frozen			= true;
			in_vertex.WindowId		= record.WindowId;
			in_vertex.ChainIndex		= record.FirstIndex;
			in_vertex.StructuralAhead= true;
			out->Add(in_vertex);

			RouteVertex out_vertex;
			out_vertex.P			= Vector2(Hybrid_Quantize_Position(record.Egress.X),
												  Hybrid_Quantize_Position(record.Egress.Y));
			out_vertex.Axis		= axis;
			out_vertex.Frozen		= true;
			out_vertex.WindowId	= record.WindowId;
			out_vertex.ChainIndex= record.LastIndex;
			out->Add(out_vertex);

			i = record.LastIndex + 1;
			continue;
		}

		/*
		**	The cells the frozen endpoints stand in are not corridor vertices of their own: the endpoint itself
		**	already represents that cell, and where it sits ahead of the cell centre along the route, taking
		**	both makes the polyline step backwards before it starts.  This comes *after* the crossing test, so
		**	an edge that begins or ends on a crossing's own bank still gets its structural interval.
		*/
		if ((cell == first_cell) || (cell == last_cell)) {
			i++;
			continue;
		}

		//	An ordinary corridor vertex: a direction change, a frozen cell, or a step-5 insertion.
		bool keep = (i == 0) || (i == (chain.Count() - 1));
		if (!keep) {
			const int incoming = Chain_Direction(b,chain[i - 1],chain[i]);
			const int outgoing = Chain_Direction(b,chain[i],chain[i + 1]);
			keep = (incoming != outgoing);
		}

		bool frozen = false;
		for (int f = 0; (f < frozen_cells.Count()) && !frozen; ++f) {
			if (frozen_cells[f] == cell) frozen = true;
		}
		for (int e = 0; (e < inserted.Count()) && !keep; ++e) {
			if (inserted[e] == cell) keep = true;
		}

		if (keep || frozen) {

			RouteVertex vertex;
			vertex.P				= b.Cell_Center(cell);
			vertex.Frozen		= frozen;
			vertex.ChainIndex	= i;
			out->Add(vertex);
		}
		i++;
	}

	RouteVertex end;
	end.P			= Vector2(Hybrid_Quantize_Position(last_point.X),Hybrid_Quantize_Position(last_point.Y));
	end.Frozen	= true;
	out->Add(end);

	/*
	**	Drop anything that coincides with its neighbour, keeping the frozen one of the pair: two controls a
	**	quantum apart describe no direction and would make every angle at them meaningless.
	*/
	DynamicVectorClass<RouteVertex> cleaned;
	for (int v = 0; v < out->Count(); ++v) {

		const RouteVertex & vertex = (*out)[v];
		if (cleaned.Count() > 0) {

			/*
			**	Two vertices at the same place are one vertex, and it keeps everything either of them knew.
			**	Dropping the later one outright loses whatever it carried -- and where a route *starts* on a
			**	crossing's own bank, what it carried was the crossing: the structural interval went unrecorded
			**	and the road was left standing in the water it was supposed to be spanning.
			*/
			RouteVertex & previous = cleaned[cleaned.Count() - 1];
			if ((vertex.P - previous.P).Length() <= quantum) {

				if (vertex.Frozen)					previous.Frozen = true;
				if (vertex.WindowId != 0)			previous.WindowId = vertex.WindowId;
				if (vertex.StructuralAhead)		previous.StructuralAhead = true;
				if (vertex.Axis.Length() > 0.0f)	previous.Axis = vertex.Axis;
				if (previous.ChainIndex < 0)		previous.ChainIndex = vertex.ChainIndex;
				continue;
			}
		}
		cleaned.Add(vertex);
	}

	out->Delete_All();
	for (int v = 0; v < cleaned.Count(); ++v) out->Add(cleaned[v]);
	Drop_Reversals(out);
}


/*
**	Give every frozen interior point a straight run through it, so the arcs can sit outside it.
**
**	An arc that rounds a corner does not pass through the corner, and a frozen point has to be interpolated
**	exactly -- so a frozen point may not *be* a corner.  A lead-in and a lead-out along the point's own axis
**	turn the corner at the frozen point into two corners on open road, with the frozen point straight
**	between them.  For a crossing that axis is the line between its two approach points, which is also what
**	makes the crossing itself straight; for any other frozen point it is the direction the corridor already
**	passes through it.
**
**	The lead is `R tan(theta/2)` long, which is exactly the straight the arc at it will need, plus a
**	quantum.  It is clamped so it cannot reach back past the vertex behind it; if that leaves the arc no
**	room, the feasibility pass drops that vertex, which it is free to do because it is corridor, not frozen.
*/
static void Insert_Frozen_Leads(RouteBuilder & b,float radius,DynamicVectorClass<RouteVertex> * vertices)
{
	const float quantum = Max2(b.P->Constants.Q,1.0e-3f);
	const float widest = 150.0f * (3.14159265358979323846f / 180.0f);		// a 150-degree turn, at most

	DynamicVectorClass<RouteVertex> built;

	int v = 0;
	while (v < vertices->Count()) {

		//	An interior run of frozen vertices: one ordinary waypoint, or a crossing's two approach points.
		if ((v == 0) || (v == (vertices->Count() - 1)) || !(*vertices)[v].Frozen) {
			built.Add((*vertices)[v]);
			v++;
			continue;
		}

		int run_end = v;
		while ((run_end + 1) < (vertices->Count() - 1)) {
			if (!(*vertices)[run_end + 1].Frozen) break;
			if ((*vertices)[run_end + 1].WindowId != (*vertices)[v].WindowId) break;
			run_end++;
		}

		const Vector2 before = built[built.Count() - 1].P;
		const Vector2 after = (*vertices)[run_end + 1].P;
		const Vector2 run_start = (*vertices)[v].P;
		const Vector2 run_finish = (*vertices)[run_end].P;

		/*
		**	The axis the road must run along through this frozen run.
		**
		**	A crossing's axis is the line between its own two approach points: that is what makes the crossing
		**	straight, and it is not ours to choose.  Any other frozen point takes the direction the road
		**	already arrives on, so the straight through it continues the road in and the corner moves to the
		**	lead-out beyond it.  Taking the chord from the vertex behind to the vertex ahead instead looks
		**	more symmetric and is wrong: at a 90-degree corner it puts the lead-in behind the vertex the road
		**	came from, which is a reversal no radius can fillet.
		*/
		Vector2 axis = (*vertices)[v].Axis;
		if (axis.Length() <= 0.0f) axis = Unit(run_finish - run_start);
		if (axis.Length() <= 0.0f) axis = Unit(run_start - before);
		if (axis.Length() <= 0.0f) {
			for (int k = v; k <= run_end; ++k) built.Add((*vertices)[k]);
			v = run_end + 1;
			continue;
		}

		/*
		**	The lead-in, on the approach side of the run.  It is `R tan(theta/2)` back along the axis, which is
		**	exactly the straight the arc at it will need, and it is only placed where it genuinely lies ahead
		**	of the vertex the road arrives from -- otherwise the road would have to double back to reach it.
		**	Where it cannot be placed, the frozen point keeps its corner: inside a crossing window that corner
		**	is the interaction phase's to compile, and anywhere else the curvature test will say so plainly.
		*/
		{
			const Vector2 incoming = Unit(run_start - before);
			float turn = 0.0f;
			if (incoming.Length() > 0.0f) {
				float dot = (incoming.X * axis.X) + (incoming.Y * axis.Y);
				dot = Clamp2(dot,-1.0f,1.0f);
				turn = (float)::acos((double)dot);
			}

			if (turn > quantum) {

				const float half = 0.5f * Min2(turn,widest);
				float lead = (radius * (float)::tan((double)half)) + b.P->Constants.Q;
				const float room = (run_start - before).Length();
				if (lead > (0.5f * room)) lead = 0.5f * room;

				const Vector2 gate_point(run_start.X - (axis.X * lead),run_start.Y - (axis.Y * lead));
				const Vector2 reach = gate_point - before;
				const Vector2 straight_on = run_start - before;
				const float forward = (reach.X * straight_on.X) + (reach.Y * straight_on.Y);

				if ((lead > quantum) && (forward > 0.0f) && (reach.Length() > quantum)) {
					RouteVertex gate;
					gate.P = Vector2(Hybrid_Quantize_Position(gate_point.X),
											Hybrid_Quantize_Position(gate_point.Y));
					gate.ChainIndex = (*vertices)[v].ChainIndex;
					built.Add(gate);
				}
			}
		}

		for (int k = v; k <= run_end; ++k) built.Add((*vertices)[k]);

		//	The lead-out, the same way, on the far side of the run from the road that leaves.
		{
			const Vector2 outgoing = Unit(after - run_finish);
			float turn = 0.0f;
			if (outgoing.Length() > 0.0f) {
				float dot = (outgoing.X * axis.X) + (outgoing.Y * axis.Y);
				dot = Clamp2(dot,-1.0f,1.0f);
				turn = (float)::acos((double)dot);
			}

			if (turn > quantum) {

				const float half = 0.5f * Min2(turn,widest);
				float lead = (radius * (float)::tan((double)half)) + b.P->Constants.Q;
				const float room = (after - run_finish).Length();
				if (lead > (0.5f * room)) lead = 0.5f * room;

				const Vector2 gate_point(run_finish.X + (axis.X * lead),run_finish.Y + (axis.Y * lead));
				const Vector2 reach = after - gate_point;
				const Vector2 straight_on = after - run_finish;
				const float forward = (reach.X * straight_on.X) + (reach.Y * straight_on.Y);

				if ((lead > quantum) && (forward > 0.0f) && (reach.Length() > quantum)) {
					RouteVertex gate;
					gate.P = Vector2(Hybrid_Quantize_Position(gate_point.X),
											Hybrid_Quantize_Position(gate_point.Y));
					gate.ChainIndex = (*vertices)[run_end].ChainIndex;
					built.Add(gate);
				}
			}
		}

		v = run_end + 1;
	}

	vertices->Delete_All();
	for (int k = 0; k < built.Count(); ++k) vertices->Add(built[k]);
	Drop_Reversals(vertices);
}


/*
**	Make every arc fit.  A turn of `theta` at radius `R` consumes `R tan(theta/2)` of the straight on each
**	side of it, so two corners sharing a leg need `T_i + T_(i+1)` of it and the leg is only `L_i` long.
**	Where that fails, the shallower of the two corners is dropped -- it is a corridor vertex, so the road
**	simply takes the straighter line there -- and the pass restarts, because dropping one vertex changes the
**	angles at its neighbours.  Ties go to the earlier vertex, so the result does not depend on iteration
**	order.
**
**	A reversal cannot be filleted at any radius, and two *frozen* corners crowding each other cannot be
**	resolved at all: both are reported as what they are, which points at the phase that froze them rather
**	than hiding a corner the class cannot drive.
*/
static bool Fillet_Is_Feasible(RouteBuilder & b,float radius,DynamicVectorClass<RouteVertex> * vertices,
										 const char ** reason_out)
{
	const float quantum = Max2(b.P->Constants.Q,1.0e-3f);
	const float reversal = 170.0f * (3.14159265358979323846f / 180.0f);

	for (int pass = 0; pass <= vertices->Count(); ++pass) {

		if (vertices->Count() < 2) {
			if (reason_out != nullptr) *reason_out = "corridor collapsed to a point";
			return false;
		}

		/*
		**	Dropping a crowded corner changes the angles at its neighbours, and can leave one of *them* a
		**	vertex the road would double back to reach.  So the reversal pass runs again here, on every
		**	iteration, rather than once before the loop.
		*/
		Drop_Reversals(vertices);

		//	The tangent length every corner needs.
		DynamicVectorClass<float> tangent;
		bool reversed = false;
		for (int v = 0; v < vertices->Count(); ++v) {

			//	An end, or a point that must be interpolated exactly: no arc is placed there at all.
			if ((v == 0) || (v == (vertices->Count() - 1)) || (*vertices)[v].Frozen) {
				tangent.Add(0.0f);
				continue;
			}
			const float turn = Vertex_Turn((*vertices)[v - 1].P,(*vertices)[v].P,(*vertices)[v + 1].P);
			if (turn >= reversal) reversed = true;
			if (turn <= quantum) {
				tangent.Add(0.0f);
				continue;
			}
			tangent.Add(radius * (float)::tan((double)(0.5f * turn)));
		}

		if (reversed) {
			if (reason_out != nullptr) *reason_out = "the corridor reverses on itself";
			return false;
		}

		//	The first leg that cannot host the two arcs sharing it.
		int crowded = -1;
		for (int leg = 0; leg < (vertices->Count() - 1); ++leg) {
			const float length = ((*vertices)[leg + 1].P - (*vertices)[leg].P).Length();
			if ((tangent[leg] + tangent[leg + 1]) <= length) continue;
			crowded = leg;
			break;
		}
		if (crowded < 0) return true;									// every arc fits

		/*
		**	Drop the shallower of the two corners crowding this leg.  A frozen one never moves, and a leg
		**	between two frozen ones has no give at all.
		*/
		const bool low_movable = !(*vertices)[crowded].Frozen && (crowded > 0);
		const bool high_movable = !(*vertices)[crowded + 1].Frozen &&
										  ((crowded + 1) < (vertices->Count() - 1));

		int drop = -1;
		if (low_movable && high_movable) {
			drop = (tangent[crowded] <= tangent[crowded + 1]) ? crowded : (crowded + 1);
		} else if (low_movable) {
			drop = crowded;
		} else if (high_movable) {
			drop = crowded + 1;
		}

		if (drop < 0) {
			if (reason_out != nullptr) {
				*reason_out = "frozen points closer than the class turn radius allows";
			}
			return false;
		}

		DynamicVectorClass<RouteVertex> kept;
		for (int v = 0; v < vertices->Count(); ++v) {
			if (v == drop) continue;
			kept.Add((*vertices)[v]);
		}
		vertices->Delete_All();
		for (int v = 0; v < kept.Count(); ++v) vertices->Add(kept[v]);
	}

	if (reason_out != nullptr) *reason_out = "the corridor could not be filleted";
	return false;
}


//	Samples a circular arc of `radius` about `centre`, from angle `from` through `sweep`, excluding the first
//	point, at about `spacing` of arc apart.
static void Arc_Samples(const Vector2 & centre,float radius,float from,float sweep,float spacing,
								DynamicVectorClass<Vector2> * out)
{
	out->Delete_All();
	//	Rounded *up*, and on the chord rather than the arc, so neither measure of the step exceeds the spacing.
	const float length = radius * (float)fabs((double)sweep);
	const int steps = (int)::ceil((double)(length / Max2(spacing,1.0e-3f)));
	const int count = (steps > 1) ? steps : 1;

	for (int i = 0; i <= count; ++i) {
		const float angle = from + (sweep * ((float)i / (float)count));
		out->Add(Vector2(centre.X + (radius * (float)::cos((double)angle)),
							  centre.Y + (radius * (float)::sin((double)angle))));
	}
}


//	Samples a straight structural centerline from `from` to `to`, inclusive, at about `spacing` apart.
static void Straight_Samples(const Vector2 & from,const Vector2 & to,float spacing,
									  DynamicVectorClass<Vector2> * out)
{
	out->Delete_All();
	//	Rounded *up*: an even division of the run has to stay at or under the spacing, never over it.
	const float length = (to - from).Length();
	const int steps = (int)::ceil((double)(length / Max2(spacing,1.0e-3f)));
	const int count = (steps > 1) ? steps : 1;
	for (int i = 0; i <= count; ++i) {
		const float ratio = (float)i / (float)count;
		out->Add(from + ((to - from) * ratio));
	}
}


/*
**	Appends one span's samples to the route's polyline, dropping the duplicate point at a span join and
**	carrying the arc length across.  Stations are measured on the polyline that is actually stored, so a
**	route made of several spans still reports one monotone station series.
*/
static void Append_Samples(DynamicVectorClass<Vector2> * points,DynamicVectorClass<float> * stations,
									const DynamicVectorClass<Vector2> & samples,float * length)
{
	for (int i = 0; i < samples.Count(); ++i) {
		if (points->Count() > 0) {
			const float step = (samples[i] - (*points)[points->Count() - 1]).Length();
			if (step <= 1.0e-4f) continue;
			*length += step;
		}
		points->Add(samples[i]);
		stations->Add(*length);
	}
}


//	The horizontal curvature radius at a sample triple; a straight run reports a huge radius.
static float Curvature_Radius(const Vector2 & a,const Vector2 & b,const Vector2 & c)
{
	const float ab = (b - a).Length();
	const float bc = (c - b).Length();
	const float ca = (a - c).Length();
	const float cross = ((b.X - a.X) * (c.Y - a.Y)) - ((b.Y - a.Y) * (c.X - a.X));
	const float area = 0.5f * (float)fabs((double)cross);
	if (area <= 1.0e-4f) return 3.4e38f;
	return (ab * bc * ca) / (4.0f * area);
}


/*
**	Step 4's sample test: inside the current envelope, clear of every hard obstacle, curvature at or
**	above the Section-7 class minimum, and vertical feasibility within the class grade and cross slope.
**	Returns the index of the earliest failing sample, or -1 when every sample passes.
*/
static int First_Failing_Sample(RouteBuilder & b,const DynamicVectorClass<Vector2> & samples,
										  const char ** reason_out)
{
	if (reason_out != nullptr) *reason_out = "none";
	b.LastRadius		= 0.0f;
	b.LastRadiusAt		= -1;
	b.LastRadiusWindow= 0;
	const HybridConstants & k = b.P->Constants;
	const HybridRoadClassSpec & spec = Hybrid_Road_Class_Spec(b.CurrentClass);
	const float min_radius = spec.MinTurnRadiusW * k.W;
	const float max_grade_radians = spec.MaxGradeDegrees * PI_OVER_180;
	const float max_cross_radians = spec.MaxCrossSlopeDegrees * PI_OVER_180;
	const float half_roadbed = 0.5f * Hybrid_Roadbed_Width(b.CurrentClass,k.W);
	const float spacing = Min2(k.W,0.5f * k.A);

	for (int i = 0; i < samples.Count(); ++i) {

		/*
		**	Structural ground is the crossing *and its approach transition*: the road is on the ramp onto the
		**	deck for an analysis cell either side of the corridor Phase 6 reserved, and that ramp's geometry
		**	is compiled by the interaction phase along with the deck itself.  Measured on the failing seed the
		**	last sub-radius corner left was exactly there -- the turn that lines a trunk up with its bridge.
		*/
		bool structural_sample = Window_At(b,samples[i]) != 0;
		if (!structural_sample) {
			const int reach = (int)Hybrid_Round_Half_Up(b.A / Max2(spacing,k.Q));
			for (int step = 1; (step <= reach) && !structural_sample; ++step) {
				if ((i - step) >= 0) {
					if (Window_At(b,samples[i - step]) != 0) structural_sample = true;
				}
				if ((i + step) < samples.Count()) {
					if (Window_At(b,samples[i + step]) != 0) structural_sample = true;
				}
			}
		}

		if (Corridor_Excess(b,samples[i]) > 0.0f) {
			if (reason_out != nullptr) *reason_out = "outside the envelope";
			b.LastRadiusAt		= i;
			b.LastRadiusWindow= Window_At(b,samples[i]);
			return i;
		}
		/*
		**	A sample is on hard ground only when the point *and* its own analysis cell both say so.  Phase 10
		**	reasons at analysis resolution -- that is the grid the search moves on and the resolution this
		**	spline is preliminary at -- so a curve that wanders half a cell inside cells the search already
		**	accepted is on legal ground; the exact footprint is Phase 11's and Phase 12's to compile.  The
		**	point test still stands on its own for the cases the cell centre cannot see, which is why both
		**	are asked.
		*/
		if (!structural_sample && (Classify_Point(b,samples[i]) != 0) &&
			 (Cached_Refusal(b,b.Cell_Of(samples[i])) != 0)) {
			if (reason_out != nullptr) *reason_out = "hard-invalid ground";
			b.LastRadiusAt		= i;
			b.LastRadiusWindow= Cached_Refusal(b,b.Cell_Of(samples[i]));
			b.LastClosed		= samples.Count();
			b.LastRadius		= (float)b.Cell_Of(samples[i]);
			b.LastExpansions	= 0;
			for (int c = 0; c < b.LastChainCount; ++c) {
				if (b.LastChain[c] == b.Cell_Of(samples[i])) b.LastExpansions = 1;		// refused cell is on the chain
			}
			return i;
		}

		/*
		**	Curvature is a property of the curve at the class's own scale.  Samples are `min(W,A/2)` apart --
		**	about three world units here -- so a circle through three *adjacent* samples measures the
		**	sampler's own wiggle, not the corner a vehicle drives: a one-unit lateral deviation over a
		**	six-unit baseline reports a radius of five, and no legal highway spline could ever pass.  The
		**	baseline is therefore the class minimum turn radius itself, which is the length over which the
		**	Section-7 limit is stated.
		*/
		const int span = (int)Hybrid_Round_Half_Up(min_radius / Max2(spacing,k.Q));
		const int baseline = (span > 1) ? span : 1;

		/*
		**	A sample inside an accepted crossing window is structural geometry, and Section 8 Phase 10 hands
		**	that to the next phase in as many words: the horizontal path only has to pass through the frozen
		**	cell, while the deck, its approach reserve and its vertical profile are compiled by the
		**	interaction phase.  Judging the preliminary spline's curvature there measures a corner Phase 11
		**	replaces with a straight span, so the class radius is enforced on open road only.
		*/
		if (!structural_sample && (i >= baseline) && (i < (samples.Count() - baseline))) {
			const float radius = Curvature_Radius(samples[i - baseline],samples[i],samples[i + baseline]);
			if (radius < min_radius) {
				if (reason_out != nullptr) *reason_out = "curvature under the class minimum";
				b.LastRadius		= radius;
				b.LastRadiusAt		= i;
				b.LastRadiusWindow= Window_At(b,samples[i]);
				return i;
			}
		}

		/*
		**	Grade and cross slope are terrain questions, so they are asked about open road only: inside an
		**	accepted window the road stands on a deck or in a bore whose geometry the interaction phase
		**	compiles, and the ground under it is not what a vehicle drives on.  That is the same exemption
		**	the move-grade and curvature tests already make, for the reason Phase 10 states itself.
		**
		**	Where they are asked, they are read as the plan words them: "local vertical feasibility intervals
		**	admit class max grade/cross-slope".  The interval is Section 7.1's `RoadVerticalInterval`, so a
		**	difference the available cut and fill can absorb is feasible even where the raw ground is
		**	steeper than the class allows -- which is what Phase 12's cut/fill will actually do.
		*/
		if (structural_sample) continue;

		float here = 0.0f;
		if (!Height_At(b,samples[i],&here)) {
			if (reason_out != nullptr) *reason_out = "no realized ground";
				b.LastRadiusAt		= i;
				b.LastRadiusWindow= Window_At(b,samples[i]);
			return i;
		}
		const HybridVerticalInterval interval_here = Hybrid_Road_Vertical_Interval(here,b.P->MaxCutDepth,
																									 b.P->MaxFillHeight);

		if (i > 0) {
			float previous = 0.0f;
			if (!Height_At(b,samples[i - 1],&previous)) return i;
			const HybridVerticalInterval interval_previous = Hybrid_Road_Vertical_Interval(previous,
																												b.P->MaxCutDepth,
																												b.P->MaxFillHeight);
			const float run = (samples[i] - samples[i - 1]).Length();
			if (!Hybrid_Grade_Compatible(interval_previous,interval_here,run,max_grade_radians)) {
				if (reason_out != nullptr) *reason_out = "grade past the class maximum";
				b.LastRadiusAt		= i;
				b.LastRadiusWindow= Window_At(b,samples[i]);
				return i;
			}
		}

		//	The cross slope across the roadbed, measured perpendicular to the local direction.
		if ((i > 0) && (half_roadbed > 0.0f)) {
			Vector2 axis = samples[i] - samples[i - 1];
			if (axis.Length() > 0.0f) {
				axis *= (1.0f / axis.Length());
				const Vector2 normal(-axis.Y,axis.X);
				float left = 0.0f;
				float right = 0.0f;
				if (!Height_At(b,samples[i] + (normal * half_roadbed),&left)) return i;
				if (!Height_At(b,samples[i] - (normal * half_roadbed),&right)) return i;
				const HybridVerticalInterval interval_left = Hybrid_Road_Vertical_Interval(left,b.P->MaxCutDepth,
																												b.P->MaxFillHeight);
				const HybridVerticalInterval interval_right = Hybrid_Road_Vertical_Interval(right,b.P->MaxCutDepth,
																												 b.P->MaxFillHeight);
				if (!Hybrid_Grade_Compatible(interval_left,interval_right,2.0f * half_roadbed,
													  max_cross_radians)) {
					if (reason_out != nullptr) *reason_out = "cross slope past the class maximum";
				b.LastRadiusAt		= i;
				b.LastRadiusWindow= Window_At(b,samples[i]);
					return i;
				}
			}
		}
	}
	return -1;
}


/*
**	Steps 1 to 6 for one accepted chain inside one envelope.  Returns false when no legal spline exists
**	within `retry.max_local_retries_per_stage` control insertions, which is the caller's cue to widen.
*/
/*
**	Steps 1 to 6 for one accepted chain inside one envelope.
**
**	The curve is anchored on the *frozen endpoint positions*, not on the first and last cell centres: Phase
**	9 froze a base entrance point and a field access point to the quantized position a road has to actually
**	meet, and a cell centre is up to half a cell away from it.  It also makes a degenerate chain work -- an
**	edge whose two endpoints fall in one analysis cell has a one-cell chain, which yields a single control
**	and no spline at all, and that is a legal short road, not a routing failure.
*/
/*
**	Steps 1 to 6 for one accepted chain inside one envelope, with the wide-radius thinning of the I11
**	decision in front of them.
**
**	The curve is anchored on the *frozen endpoint positions*, not on the first and last cell centres:
**	Phase 9 froze a base entrance point and a field access point to the position a road has to meet, and a
**	cell centre is up to half a cell away from it.  It also makes a degenerate chain work -- an edge whose
**	endpoints fall in one analysis cell has a one-cell chain, which is a legal short road.
**
**	A curvature failure is answered by thinning (a denser control set can only bend tighter); every other
**	failure is answered by the plan's own control insertion.  Each control set is evaluated once: the
**	attempted sets are recorded by digest, so the two repairs cannot oscillate against each other.
*/
/*
**	Lay the centerline down: a straight along each leg between the two arcs that share it, and a circular arc
**	of the build radius at each corner.  Stations are measured on the polyline that is actually stored, so
**	the route reports one monotone station series whatever it is made of.
**
**	A frozen point is never cut: its tangent length is zero, so the polyline passes exactly through it.  The
**	lead-in and lead-out put it on a straight run, so that costs no smoothness; where a crossing's own
**	approach point still carries a turn, the corner is inside the window and is the interaction phase's to
**	compile, which is the same exemption the grade and cross-slope tests already make there.
*/
static bool Emit_Centerline(RouteBuilder & b,float radius,const DynamicVectorClass<RouteVertex> & vertices,
									 float spacing,DynamicVectorClass<Vector2> * points,
									 DynamicVectorClass<float> * stations,
									 DynamicVectorClass<HybridRouteStructuralSpan> * spans)
{
	points->Delete_All();
	stations->Delete_All();
	spans->Delete_All();
	if (vertices.Count() < 2) return false;

	const float quantum = Max2(b.P->Constants.Q,1.0e-3f);
	float length = 0.0f;

	//	What each corner's arc consumes of the straights either side of it.
	DynamicVectorClass<float> tangent;
	for (int v = 0; v < vertices.Count(); ++v) {

		if ((v == 0) || (v == (vertices.Count() - 1)) || vertices[v].Frozen) {
			tangent.Add(0.0f);										// an end, or a point that must be interpolated
			continue;
		}
		const float turn = Vertex_Turn(vertices[v - 1].P,vertices[v].P,vertices[v + 1].P);
		tangent.Add((turn <= quantum) ? 0.0f : (radius * (float)::tan((double)(0.5f * turn))));
	}

	{
		DynamicVectorClass<Vector2> one;
		one.Add(vertices[0].P);
		Append_Samples(points,stations,one,&length);
	}

	for (int leg = 0; leg < (vertices.Count() - 1); ++leg) {

		const Vector2 direction = Unit(vertices[leg + 1].P - vertices[leg].P);
		if (direction.Length() <= 0.0f) return false;

		const Vector2 from = vertices[leg].P + (direction * tangent[leg]);
		const Vector2 to = vertices[leg + 1].P - (direction * tangent[leg + 1]);

		//	A crossing's own frozen interval: record where it begins before its straight is laid down.
		int structural = -1;
		if (vertices[leg].StructuralAhead) {
			HybridRouteStructuralSpan span;
			span.WindowId	= vertices[leg].WindowId;
			span.Ingress	= vertices[leg].P;
			span.Egress		= vertices[leg + 1].P;
			span.FirstPoint= (points->Count() > 0) ? (points->Count() - 1) : 0;
			spans->Add(span);
			structural = spans->Count() - 1;
		}

		DynamicVectorClass<Vector2> run;
		Straight_Samples(from,to,spacing,&run);
		Append_Samples(points,stations,run,&length);

		if (structural >= 0) (*spans)[structural].LastPoint = points->Count() - 1;

		//	The arc at the far end of this leg, unless the route ends there or the road runs straight on.
		const int corner = leg + 1;
		if (corner >= (vertices.Count() - 1)) continue;
		if (tangent[corner] <= 0.0f) continue;

		const Vector2 outgoing = Unit(vertices[corner + 1].P - vertices[corner].P);
		const float cross = (direction.X * outgoing.Y) - (direction.Y * outgoing.X);
		if ((float)fabs((double)cross) <= 1.0e-6f) continue;

		//	The centre stands one radius off this leg, on the side the road turns towards.
		const float sign = (cross > 0.0f) ? 1.0f : -1.0f;
		const Vector2 normal(-direction.Y * sign,direction.X * sign);
		const Vector2 centre = to + (normal * radius);
		const float start_angle = (float)::atan2((double)(to.Y - centre.Y),(double)(to.X - centre.X));
		const float turn = Vertex_Turn(vertices[corner - 1].P,vertices[corner].P,vertices[corner + 1].P);

		DynamicVectorClass<Vector2> arc;
		Arc_Samples(centre,radius,start_angle,sign * turn,spacing,&arc);
		Append_Samples(points,stations,arc,&length);
	}

	{
		DynamicVectorClass<Vector2> one;
		one.Add(vertices[vertices.Count() - 1].P);
		Append_Samples(points,stations,one,&length);
	}
	return (points->Count() >= 2);
}


/*
**	Steps 1 to 6 for one accepted chain inside one envelope.
**
**	The chain is the corridor; the centerline is geometry fitted to it.  Build the corridor polyline, give
**	every frozen point a straight run through itself, make every arc fit, lay the straights and arcs down,
**	and put the result through the whole validator -- envelope, obstacles, the exact Section-7 turn radius,
**	grade and cross slope.  Nothing about the radius is left to chance: the arcs *are* the radius.
**
**	Two failures have answers.  An obstacle, envelope, grade or cross-slope refusal means the geometry cut a
**	corner the corridor went around, so the plan's own step 5 inserts the nearest path cell as a vertex and
**	pulls it back.  A curvature refusal can only come from a junction the exact measure reads tighter than
**	the arc it is made of, so the build radius rises by one step, which can only flatten the curve; the gate
**	itself never moves.  Each answer is bounded by `retry.max_local_retries_per_stage`.
*/
static bool Chain_To_Spline(RouteBuilder & b,const DynamicVectorClass<int> & chain,
									 const DynamicVectorClass<int> & mandatory,
									 const DynamicVectorClass<RouteCrossing> & crossings,const Vector2 & first_point,
									 const Vector2 & last_point,DynamicVectorClass<Vector2> * points,
									 DynamicVectorClass<float> * stations,
									 DynamicVectorClass<HybridRouteStructuralSpan> * structural,
									 int * insertions_out,const char ** reason_out)
{
	const HybridConstants & k = b.P->Constants;
	const float spacing = Min2(k.W,0.5f * k.A);
	const int limit = (b.P->MaxLocalRetriesPerStage > 0) ? b.P->MaxLocalRetriesPerStage : 0;
	const float min_radius = Hybrid_Road_Class_Spec(b.CurrentClass).MinTurnRadiusW * k.W;

	DynamicVectorClass<int> extra;							// step 5's inserted vertices, in path order
	int insertions		= 0;
	int radius_steps	= 0;

	for (;;) {

		const float build_radius = min_radius * (FILLET_RADIUS_MARGIN +
															  ((float)radius_steps * FILLET_RADIUS_STEP));

		DynamicVectorClass<RouteVertex> vertices;
		Build_Route_Vertices(b,chain,mandatory,extra,crossings,first_point,last_point,&vertices);
		Insert_Frozen_Leads(b,build_radius,&vertices);

		const char * geometry = "none";
		if (!Fillet_Is_Feasible(b,build_radius,&vertices,&geometry)) {
			if (reason_out != nullptr) *reason_out = geometry;
			b.LastRadius	= min_radius;
			b.LastRadiusAt	= vertices.Count();
			return false;
		}

		DynamicVectorClass<Vector2> samples;
		DynamicVectorClass<float> sample_stations;
		DynamicVectorClass<HybridRouteStructuralSpan> spans;
		if (!Emit_Centerline(b,build_radius,vertices,spacing,&samples,&sample_stations,&spans)) {
			if (reason_out != nullptr) *reason_out = "degenerate centerline";
			b.LastRadius	= min_radius;
			b.LastRadiusAt	= vertices.Count();
			return false;
		}

		const int failing = First_Failing_Sample(b,samples,reason_out);
		if (failing < 0) {

			points->Delete_All();
			stations->Delete_All();
			for (int i = 0; i < samples.Count(); ++i) {
				points->Add(Vector2(Hybrid_Quantize_Position(samples[i].X),
										  Hybrid_Quantize_Position(samples[i].Y)));
				stations->Add(Hybrid_Quantize_Position(sample_stations[i]));
			}
			if (structural != nullptr) {
				structural->Delete_All();
				for (int i = 0; i < spans.Count(); ++i) structural->Add(spans[i]);
			}
			if (insertions_out != nullptr) *insertions_out = insertions;
			return true;
		}

		const bool curvature = (reason_out != nullptr) && (*reason_out != nullptr) &&
										(::strcmp(*reason_out,"curvature under the class minimum") == 0);

		if (curvature) {
			//	Build flatter.  The measured limit is untouched; only the geometry offered to it changes.
			if (radius_steps >= limit) return false;
			radius_steps++;
			continue;
		}

		/*
		**	Step 5: the nearest original path cell that is not already a vertex, ties broken by the earliest
		**	path index, inserted in path order.
		*/
		int best = -1;
		float best_distance = 3.4e38f;
		for (int i = 0; i < chain.Count(); ++i) {

			bool already = false;
			for (int v = 0; (v < vertices.Count()) && !already; ++v) {
				if (vertices[v].ChainIndex == i) already = true;
			}
			for (int e = 0; (e < extra.Count()) && !already; ++e) {
				if (extra[e] == chain[i]) already = true;
			}
			if (already) continue;

			const float distance = (b.Cell_Center(chain[i]) - samples[failing]).Length();
			if (distance >= best_distance) continue;
			best_distance	= distance;
			best				= chain[i];
		}
		if (best < 0) return false;
		if (insertions >= limit) return false;

		extra.Add(best);
		insertions++;
		b.Out->TotalInsertions++;
	}
}


/***********************************************************************************************
**	One topology edge: its legs, its envelopes and its accepted spline.
***********************************************************************************************/

//	The ordered node chain of one edge: NodeA, its frozen waypoints in order, then NodeB.
static void Edge_Node_Chain(const RouteBuilder & b,const HybridTopologyEdge & edge,
									 DynamicVectorClass<unsigned int> * out)
{
	out->Delete_All();
	out->Add(edge.NodeA);
	for (int w = 0; w < edge.Waypoints.Count(); ++w) out->Add(edge.Waypoints[w]);
	out->Add(edge.NodeB);
}


static bool Route_Edge(RouteBuilder & b,const HybridTopologyEdge & edge,HybridRouteResult * result)
{
	const HybridConstants & k = b.P->Constants;
	const HybridRoadClass road_class = (edge.RoadClass < HYBRID_ROAD_CLASS_COUNT) ? edge.RoadClass
																								  : HYBRID_ROAD_SECONDARY_2;
	const float preferred = Hybrid_Preferred_Corridor_Half_Width(road_class,k.S,k.D);
	const float maximum = Hybrid_Max_Corridor_Half_Width(road_class,k.S,k.D);
	StringClass detail;

	DynamicVectorClass<unsigned int> nodes;
	Edge_Node_Chain(b,edge,&nodes);
	if (nodes.Count() < 2) return true;

	/*
	**	The stops this edge routes between.  A crossing waypoint is not one stop but two: the window's own
	**	frozen approach points, one on each bank.  Routing to the window's centre cell and out again left the
	**	search free to enter and leave the corridor at whatever angle suited it, and the crossing then was
	**	neither straight nor through the frozen approach points -- measured on the failing seed, an 8.9 radius
	**	two cells past a window where 16.6 was needed, and a structural interval that could not be recognized
	**	at all because the chain never touched both banks.  Stopping on each approach point in turn makes the
	**	crossing a leg of its own: straight, through both frozen points, and the only leg allowed inside the
	**	obstruction.
	*/
	DynamicVectorClass<Vector2> stops;
	DynamicVectorClass<unsigned int> stop_window;		// the window a stop belongs to, 0 for open ground
	DynamicVectorClass<unsigned int> stop_site;			// the Tiberium reserve a stop may enter, 0 for none

	for (int n = 0; n < nodes.Count(); ++n) {

		const HybridTopologyNode * node = b.Topology->Find_Node(nodes[n]);
		if (node == nullptr) continue;

		const HybridCrossingWindow * window = (node->WindowId != 0)
														  ? b.Hydrology->Find_Window(node->WindowId) : nullptr;
		if (window != nullptr) {

			/*
			**	Which bank the road arrives on is decided by the whole journey, not by the stop behind it.  A
			**	window whose axis lies across the route is entered on the near bank and left on the far one;
			**	taking only the nearer approach point first sent a trunk over a crossing and straight back
			**	again -- on the failing seed, a leg that arrived heading west and had to leave heading
			**	east-north-east, which is a reversal no road of any class can make.
			*/
			Vector2 before = (stops.Count() > 0) ? stops[stops.Count() - 1] : node->Position;
			if (stops.Count() == 0) {
				for (int p = n - 1; p >= 0; --p) {
					const HybridTopologyNode * previous = b.Topology->Find_Node(nodes[p]);
					if (previous == nullptr) continue;
					before = previous->Position;
					break;
				}
			}

			Vector2 after = node->Position;
			for (int q = n + 1; q < nodes.Count(); ++q) {
				const HybridTopologyNode * following = b.Topology->Find_Node(nodes[q]);
				if (following == nullptr) continue;
				after = following->Position;
				break;
			}

			const float a_then_b = (window->ApproachA - before).Length() + (after - window->ApproachB).Length();
			const float b_then_a = (window->ApproachB - before).Length() + (after - window->ApproachA).Length();
			const bool a_first = (a_then_b <= b_then_a);

			stops.Add(a_first ? window->ApproachA : window->ApproachB);
			stop_window.Add(node->WindowId);
			stop_site.Add(0);
			stops.Add(a_first ? window->ApproachB : window->ApproachA);
			stop_window.Add(node->WindowId);
			stop_site.Add(0);
			continue;
		}

		stops.Add(node->Position);
		stop_window.Add(node->WindowId);
		stop_site.Add((node->Kind == HYBRID_NODE_FIELD_ACCESS) ? node->SiteId : 0);
	}
	if (stops.Count() < 2) return true;

	b.CurrentClass			= road_class;
	b.CurrentFamily		= edge.RouteFamily;
	b.CurrentEdgeId		= edge.StableId;
	b.EntryDirection		= START_DIRECTION;
	b.EntryRun				= 0.0f;
	b.StructuralLeg		= false;

	b.CorridorPath.Delete_All();
	for (int n = 0; n < stops.Count(); ++n) b.CorridorPath.Add(stops[n]);
	b.CurrentIsOptional	= (Hybrid_Route_Priority_Of(*b.Topology,edge) == HYBRID_ROUTE_PRIORITY_OPTIONAL_LINK);
	const char * spline_reason = "no path";

	/*
	**	Where this edge's envelope search starts.  Phase 11 may have ruled out the envelopes it already tried,
	**	in which case the search begins strictly after the one that produced the route it refused.
	*/
	int first_envelope = 0;
	for (int f = 0; f < b.P->EnvelopeFloors.Count(); ++f) {
		if (b.P->EnvelopeFloors[f].EdgeId != edge.StableId) continue;
		first_envelope = b.P->EnvelopeFloors[f].MinEnvelope;
		break;
	}
	if (first_envelope < 0) first_envelope = 0;
	if (first_envelope > 2) first_envelope = 2;

	for (int envelope = first_envelope; envelope < 3; ++envelope) {

		b.CurrentHalfWidth = Min2(preferred + ((float)envelope * k.S),maximum);
		b.Expansions = 0;

		DynamicVectorClass<int> chain;
		DynamicVectorClass<RouteCrossing> crossings;
		bool routed = true;
		int entry_direction = START_DIRECTION;

		for (int leg = 0; (leg < (stops.Count() - 1)) && routed; ++leg) {

			//	This leg's own corridor, and the two per-leg exemptions its endpoints carry.
			b.CorridorA		= stops[leg];
			b.CorridorB		= stops[leg + 1];
			b.OwnWindow		= (stop_window[leg] != 0) ? stop_window[leg] : stop_window[leg + 1];
			b.StructuralLeg= (stop_window[leg] != 0) && (stop_window[leg] == stop_window[leg + 1]);
			b.OwnFieldSite	= (stop_site[leg] != 0) ? stop_site[leg] : stop_site[leg + 1];

			const int start = b.Cell_Of(stops[leg]);
			const int goal = b.Cell_Of(stops[leg + 1]);

			if (start == goal) {
				if ((chain.Count() == 0) || (chain[chain.Count() - 1] != start)) chain.Add(start);
				continue;
			}

			const int chain_before = chain.Count();

			const int entry = entry_direction;
			//	The straight this leg is handed by the one before it, so a turn at the boundary is measured.
			b.EntryDirection	= entry;
			b.EntryRun			= 0.0f;
			if ((entry != START_DIRECTION) && (chain.Count() >= 2)) {
				const int arrival = Chain_Direction(b,chain[chain.Count() - 2],chain[chain.Count() - 1]);
				for (int c = chain.Count() - 1; c >= 1; --c) {
					if (Chain_Direction(b,chain[c - 1],chain[c]) != arrival) break;
					b.EntryRun += (b.Cell_Center(chain[c]) - b.Cell_Center(chain[c - 1])).Length();
				}
			}

			if (!Search_Leg(b,start,goal,entry,&chain,result)) routed = false;

			/*
			**	A structural leg *is* a crossing: record which window, which frozen approach point it entered
			**	by and left by, and the stretch of chain it laid down.  That record is what the centerline is
			**	built from, so nothing has to guess afterwards whether a route crossed a window or merely
			**	passed through its corridor.
			*/
			if (routed && b.StructuralLeg) {

				int first_index = (chain_before > 0) ? (chain_before - 1) : 0;
				for (int c = chain.Count() - 1; c >= 0; --c) {
					if (chain[c] != start) continue;
					first_index = c;
					break;
				}

				RouteCrossing record;
				record.WindowId	= b.OwnWindow;
				record.FirstIndex	= first_index;
				record.LastIndex	= chain.Count() - 1;
				record.Ingress		= stops[leg];
				record.Egress		= stops[leg + 1];
				if (record.LastIndex > record.FirstIndex) crossings.Add(record);
			}

			//	The heading this leg ends on is the heading the next one starts with.
			if (routed && (chain.Count() >= 2)) {
				const int last = chain[chain.Count() - 1];
				const int previous = chain[chain.Count() - 2];
				const int direction = Chain_Direction(b,previous,last);
				entry_direction = (direction >= 0) ? direction : START_DIRECTION;
			}
		}

		if (!routed) {
			if ((result != nullptr) && (result->Failure == HYBRID_ROUTE_EXPANSION_BUDGET)) return false;
			continue;											// no path in this envelope: widen
		}

		//	The spline is built over the whole concatenated chain, so a leg boundary is not a corner.
		b.CorridorA = stops[0];
		b.CorridorB = stops[stops.Count() - 1];
		b.OwnWindow = 0;
		for (int w = 0; w < edge.Waypoints.Count(); ++w) {
			const HybridTopologyNode * node = b.Topology->Find_Node(edge.Waypoints[w]);
			if ((node != nullptr) && (node->WindowId != 0) && (b.OwnWindow == 0)) b.OwnWindow = node->WindowId;
		}

		/*
		**	The spline pass covers the whole edge, so it gets its own cache generation and its own
		**	endpoint exemptions: reusing the last leg's would answer with that leg's window and field
		**	exemptions, which are not this pass's.
		*/
		b.StartCell	= (chain.Count() > 0) ? chain[0] : -1;
		b.GoalCell	= (chain.Count() > 0) ? chain[chain.Count() - 1] : -1;
		b.OwnWindow	= 0;
		b.OwnFieldSite = 0;
		b.StructuralLeg = false;
		b.Generation++;

		b.LastChain			= (chain.Count() > 0) ? &chain[0] : nullptr;
		b.LastChainCount	= chain.Count();

		DynamicVectorClass<Vector2> points;
		DynamicVectorClass<float> stations;
		int insertions = 0;
		const Vector2 first_point = stops[0];
		const Vector2 last_point = stops[stops.Count() - 1];

		/*
		**	The cells whose positions earlier phases froze: every waypoint this edge must pass through, and
		**	every chain cell inside an accepted crossing window.  A window's corridor is about one roadbed
		**	wide, so a control set that thins or smooths across it lets the curve leave the corridor and
		**	land on the wall or channel the window exists to cross -- which is a `wall_outside_window`
		**	refusal on ground the search had every right to use.
		*/
		DynamicVectorClass<int> mandatory_cells;
		for (int w = 0; w < edge.Waypoints.Count(); ++w) {
			const HybridTopologyNode * node = b.Topology->Find_Node(edge.Waypoints[w]);
			if (node != nullptr) mandatory_cells.Add(b.Cell_Of(node->Position));
		}

		/*
		**	A window the search drove through without being sent to is still a crossing, and is
		**	recorded here rather than left for a later phase to deduce -- the same rule as a planned
		**	one, because it is the same crossing.
		*/
		Note_Unplanned_Crossings(b,chain,&crossings);

		DynamicVectorClass<HybridRouteStructuralSpan> spans;
		if (!Chain_To_Spline(b,chain,mandatory_cells,crossings,first_point,last_point,&points,&stations,
									&spans,&insertions,&spline_reason)) continue;

		HybridRoute route;
		route.EdgeId				= edge.StableId;
		route.RoadClass			= road_class;
		route.Priority				= Hybrid_Route_Priority_Of(*b.Topology,edge);
		route.RouteFamily			= edge.RouteFamily;
		route.Envelope				= envelope;
		route.Required				= edge.Required;
		route.ControlInsertions	= insertions;
		route.Expansions			= b.Expansions;
		route.Length				= (stations.Count() > 0) ? stations[stations.Count() - 1] : 0.0f;

		for (int i = 0; i < points.Count(); ++i) route.Points.Add(points[i]);
		for (int i = 0; i < stations.Count(); ++i) route.Stations.Add(stations[i]);
		for (int i = 0; i < chain.Count(); ++i) route.Cells.Add(chain[i]);
		for (int w = 0; w < edge.Waypoints.Count(); ++w) route.Waypoints.Add(edge.Waypoints[w]);
		for (int s = 0; s < spans.Count(); ++s) route.Structural.Add(spans[s]);

		Vector2 minimum_point = points[0];
		Vector2 maximum_point = points[0];
		for (int i = 1; i < points.Count(); ++i) {
			minimum_point.X = Min2(minimum_point.X,points[i].X);
			minimum_point.Y = Min2(minimum_point.Y,points[i].Y);
			maximum_point.X = Max2(maximum_point.X,points[i].X);
			maximum_point.Y = Max2(maximum_point.Y,points[i].Y);
		}
		route.BoundsMin = minimum_point;
		route.BoundsMax = maximum_point;

		if (envelope > 0) b.Out->WidenedRoutes++;
		b.Out->Routes.Add(route);
		return true;
	}

	//	Three envelopes and no legal spline: a mandatory edge fails the attempt, an optional one is dropped.
	if (!edge.Required) {
		WWDEBUG_SAY(("Hybrid Phase 10: optional edge %u (%s) has no legal route and is dropped\r\n",
						 edge.StableId,Hybrid_Topology_Edge_Kind_Name(edge.Kind)));
		return true;
	}

	detail.Format("edge %u (%s, %s) has no legal route in three envelopes: %s (sample %d, radius %.1f of "
					  "%.1f needed, window %u; last search: %d expansions, %d closed states, %d goal states "
					  "touched)",edge.StableId,Hybrid_Topology_Edge_Kind_Name(edge.Kind),
					  Hybrid_Road_Class_Name(road_class),spline_reason,b.LastRadiusAt,b.LastRadius,
					  Hybrid_Road_Class_Spec(road_class).MinTurnRadiusW * b.P->Constants.W,b.LastRadiusWindow,
					  b.LastExpansions,b.LastClosed,b.LastGoalTouched);
	return Fail(result,HYBRID_ROUTE_NO_SPLINE,detail.Peek_Buffer());
}


/***********************************************************************************************
**	The Phase 10 gate.
***********************************************************************************************/

static bool Check_Gate(RouteBuilder & b,HybridRouteResult * result)
{
	StringClass detail;

	for (int e = 0; e < b.Topology->Edges.Count(); ++e) {

		const HybridTopologyEdge & edge = b.Topology->Edges[e];
		if (!edge.Is_Realized()) continue;

		const HybridRoute * route = b.Out->Find(edge.StableId);
		if (route == nullptr) {
			if (!edge.Required) continue;					// an optional edge may legitimately be missing
			detail.Format("mandatory edge %u has no route",edge.StableId);
			return Fail(result,HYBRID_ROUTE_NO_PATH,detail.Peek_Buffer());
		}

		if (route->Points.Count() < 2) {
			detail.Format("route %u has %d samples",route->EdgeId,route->Points.Count());
			return Fail(result,HYBRID_ROUTE_NO_SPLINE,detail.Peek_Buffer());
		}

		//	Every frozen waypoint's own cell is on the accepted chain, exactly as the plan requires.
		for (int w = 0; w < edge.Waypoints.Count(); ++w) {

			const HybridTopologyNode * node = b.Topology->Find_Node(edge.Waypoints[w]);
			if (node == nullptr) continue;
			const int cell = b.Cell_Of(node->Position);

			bool on_chain = false;
			for (int c = 0; (c < route->Cells.Count()) && !on_chain; ++c) {
				if (route->Cells[c] == cell) on_chain = true;
			}

			/*
			**	A crossing waypoint is discharged by crossing its own window, which is a stronger statement
			**	than touching the cell the node's position happens to fall in: the route runs through both of
			**	the window's frozen approach points and the interval between them is its structural span.  The
			**	node's cell is the middle of the obstruction, which is exactly the stretch the route spans
			**	rather than drives through.
			*/
			for (int s = 0; (s < route->Structural.Count()) && !on_chain; ++s) {
				if (route->Structural[s].WindowId == node->WindowId) on_chain = true;
			}
			if (on_chain) continue;

			StringClass banks = "none";
			const HybridCrossingWindow * window = (node->WindowId != 0)
															  ? b.Hydrology->Find_Window(node->WindowId) : nullptr;
			if (window != nullptr) {
				const int cell_a = b.Cell_Of(window->ApproachA);
				const int cell_b = b.Cell_Of(window->ApproachB);
				int first_a = -1;
				int first_b = -1;
				int count_a = 0;
				int count_b = 0;
				for (int c = 0; c < route->Cells.Count(); ++c) {
					if (route->Cells[c] == cell_a) { if (first_a < 0) first_a = c; count_a++; }
					if (route->Cells[c] == cell_b) { if (first_b < 0) first_b = c; count_b++; }
				}
				banks.Format("A cell %d at %d x%d, B cell %d at %d x%d, span %.0f, span1 %u",cell_a,first_a,
								 count_a,cell_b,first_b,count_b,
								 (window->ApproachB - window->ApproachA).Length(),
								 (route->Structural.Count() > 0) ? route->Structural[0].WindowId : 0);
			}
			detail.Format("route %u misses frozen waypoint %u's cell (kind %d, window %u, cell %d, chain %d "
							  "cells, spans %d, banks %s)",route->EdgeId,node->StableId,(int)node->Kind,
							  node->WindowId,cell,route->Cells.Count(),route->Structural.Count(),
							  banks.Peek_Buffer());
			return Fail(result,HYBRID_ROUTE_WAYPOINT_MISSED,detail.Peek_Buffer());
		}
	}
	return true;
}


/***********************************************************************************************
 * Hybrid_Build_Routes -- runtime Phase 10                                                      *
 *=============================================================================================*/
bool Hybrid_Build_Routes(const HybridRouteParams & params,const HybridMapIdentity & identity,
										const HybridCoarseMatrices & matrices,const HybridLandformSet & landforms,
										const HybridHydrologySet & hydrology,const HybridAnchorSet & anchors,
										const HybridTopology & topology,HybridRouteHeightSampleFunc sample,
										void * context,HybridRouteSet * out,HybridRouteResult * result)
{
	if (result != nullptr) {
		result->Failure	= HYBRID_ROUTE_OK;
		result->Detail		= "";
	}
	if ((out == nullptr) || (sample == nullptr)) {
		return Fail(result,HYBRID_ROUTE_INVALID_INPUT,"no output record or no height sampler");
	}

	out->Reset();

	if ((topology.Edges.Count() <= 0) || !anchors.Frozen || (params.Constants.A <= 0.0f) ||
		 (params.Constants.W <= 0.0f) || (params.WorldWidth <= 0.0f) || (params.WorldHeight <= 0.0f)) {
		return Fail(result,HYBRID_ROUTE_INVALID_INPUT,"no topology, no frozen anchors, or degenerate constants");
	}

	RouteBuilder b;
	b.P				= &params;
	b.Identity		= &identity;
	b.Matrices		= &matrices;
	b.Landforms		= &landforms;
	b.Hydrology		= &hydrology;
	b.Anchors		= &anchors;
	b.Topology		= &topology;
	b.Sample			= sample;
	b.Context		= context;
	b.Out				= out;
	b.A				= params.Constants.A;
	b.OriginX		= params.WorldMinX;
	b.OriginY		= params.WorldMinY;
	b.States			= nullptr;
	b.Heap			= nullptr;
	b.HeapCount		= 0;
	b.HeapCapacity	= 0;
	b.HeapOverflowed = false;
	b.CacheStamp	= nullptr;
	b.Generation	= 0;
	b.LastChain		= nullptr;
	b.LastChainCount = 0;
	b.CurrentClass	= HYBRID_ROAD_PRIMARY_2;
	b.CurrentFamily= -1;
	b.CurrentHalfWidth = 0.0f;
	b.CorridorA.Set(0.0f,0.0f);
	b.CorridorB.Set(0.0f,0.0f);
	b.CurrentEdgeId	= 0;
	b.CurrentIsOptional = false;
	b.OwnWindow		= 0;
	b.OwnFieldSite	= 0;
	b.StartCell		= -1;
	b.GoalCell		= -1;
	b.Expansions	= 0;

	b.CountX = (int)(params.WorldWidth / b.A);
	b.CountY = (int)(params.WorldHeight / b.A);
	if (((float)b.CountX * b.A) < params.WorldWidth) b.CountX++;
	if (((float)b.CountY * b.A) < params.WorldHeight) b.CountY++;
	if ((b.CountX < 3) || (b.CountY < 3)) {
		return Fail(result,HYBRID_ROUTE_INVALID_INPUT,"analysis grid is smaller than three cells");
	}
	b.CellCount = b.CountX * b.CountY;

	out->AnalysisCountX = b.CountX;
	out->AnalysisCountY = b.CountY;

	if (!Allocate_Search(b,result)) { out->Reset(); return false; }

	/*
	**	The plan's routing order: the eight priority groups, and inside a group the edge `StableId`.
	**	Routing in this order is what makes the reuse modifier and the collinear rules meaningful --
	**	each one reads the routes that are already on the ground.
	*/
	for (int priority = 0; priority < HYBRID_ROUTE_PRIORITY_COUNT; ++priority) {
		for (int e = 0; e < topology.Edges.Count(); ++e) {

			const HybridTopologyEdge & edge = topology.Edges[e];
			if (!edge.Is_Realized()) continue;
			if ((int)Hybrid_Route_Priority_Of(topology,edge) != priority) continue;

			if (!Route_Edge(b,edge,result)) { out->Reset(); return false; }
			if (params.Counters != nullptr) params.Counters->Add(HYBRID_WORK_ROUTED_EDGES,1);
		}
	}

	if (!Check_Gate(b,result)) { out->Reset(); return false; }

	out->Checksum = Hybrid_Route_Checksum(*out,identity.Checksum);

	WWDEBUG_SAY(("Hybrid Phase 10: %d routes (%d widened), %d expansions, %d control insertions, "
					 "highway %.0f primary %.0f secondary %.0f dirt %.0f trail %.0f, checksum 0x%08X\r\n",
					 out->Routes.Count(),out->WidenedRoutes,out->TotalExpansions,out->TotalInsertions,
					 out->Total_Length_Of_Class(HYBRID_ROAD_HIGHWAY_4),
					 out->Total_Length_Of_Class(HYBRID_ROAD_PRIMARY_2),
					 out->Total_Length_Of_Class(HYBRID_ROAD_SECONDARY_2),
					 out->Total_Length_Of_Class(HYBRID_ROAD_SERVICE_DIRT_2),
					 out->Total_Length_Of_Class(HYBRID_ROAD_TRAIL_1),out->Checksum));

	return true;
}


/***********************************************************************************************
 * Hybrid_Route_Census -- what refuses one leg's search region                                   *
 *                                                                                             *
 * Uses the router's own classifier, so what this reports is exactly what the router obeys.  For  *
 * each leg of `edge_id` it floods the accepted cells from the leg's start; the first leg whose    *
 * goal the flood cannot reach is the one described, with the rule counts over the whole grid, the *
 * rule counts on the frontier the flood stopped at, and the verdict on each of the goal's eight   *
 * neighbours.                                                                                    *
 *=============================================================================================*/
const char * Hybrid_Route_Refusal_Rule_Name(HybridRouteRefusalRule rule)
{
	switch (rule) {
		case HYBRID_ROUTE_RULE_UNPLACEABLE:	return "unplaceable";
		case HYBRID_ROUTE_RULE_BASE_DISC:	return "base_disc";
		case HYBRID_ROUTE_RULE_TIBERIUM:		return "tiberium_reserve";
		case HYBRID_ROUTE_RULE_WATER:			return "standing_water";
		case HYBRID_ROUTE_RULE_WALL:			return "wall_outside_window";
		default:										break;
	}
	return "unknown";
}


bool Hybrid_Route_Census(const HybridRouteParams & params,const HybridMapIdentity & identity,
									 const HybridCoarseMatrices & matrices,const HybridLandformSet & landforms,
									 const HybridHydrologySet & hydrology,const HybridAnchorSet & anchors,
									 const HybridTopology & topology,HybridRouteHeightSampleFunc sample,
									 void * context,unsigned int edge_id,HybridRouteCensus * out)
{
	if ((out == nullptr) || (sample == nullptr)) return false;
	*out = HybridRouteCensus();

	const HybridTopologyEdge * edge = topology.Find_Edge(edge_id);
	if (edge == nullptr) return false;

	HybridRouteSet scratch;

	RouteBuilder b;
	b.P				= &params;
	b.Identity		= &identity;
	b.Matrices		= &matrices;
	b.Landforms		= &landforms;
	b.Hydrology		= &hydrology;
	b.Anchors		= &anchors;
	b.Topology		= &topology;
	b.Sample			= sample;
	b.Context		= context;
	b.Out				= &scratch;
	b.A				= params.Constants.A;
	b.OriginX		= params.WorldMinX;
	b.OriginY		= params.WorldMinY;
	b.States			= nullptr;
	b.Heap			= nullptr;
	b.HeapCount		= 0;
	b.HeapCapacity	= 0;
	b.HeapOverflowed = false;
	b.CurrentClass	= (edge->RoadClass < HYBRID_ROAD_CLASS_COUNT) ? edge->RoadClass : HYBRID_ROAD_SECONDARY_2;
	b.CurrentFamily= edge->RouteFamily;
	b.CurrentEdgeId= edge->StableId;
	b.CurrentIsOptional = false;
	b.OwnWindow		= 0;
	b.OwnFieldSite	= 0;
	b.StartCell		= -1;
	b.GoalCell		= -1;
	b.Expansions	= 0;
	b.LastExpansions = 0;
	b.LastGoalTouched= 0;
	b.LastClosed	= 0;
	b.LastRadius	= 0.0f;
	b.LastRadiusAt	= -1;
	b.LastRadiusWindow = 0;
	b.EntryDirection	= START_DIRECTION;
	b.EntryRun			= 0.0f;
	b.StructuralLeg	= false;
	b.CorridorA.Set(0.0f,0.0f);
	b.CorridorB.Set(0.0f,0.0f);

	b.CountX = (int)(params.WorldWidth / b.A);
	b.CountY = (int)(params.WorldHeight / b.A);
	if (((float)b.CountX * b.A) < params.WorldWidth) b.CountX++;
	if (((float)b.CountY * b.A) < params.WorldHeight) b.CountY++;
	if ((b.CountX < 3) || (b.CountY < 3)) return false;
	b.CellCount = b.CountX * b.CountY;

	//	The census reads the same cached rules the router does, so it needs the same scratch.
	if (!Allocate_Search(b,nullptr)) return false;

	const float preferred = Hybrid_Preferred_Corridor_Half_Width(b.CurrentClass,params.Constants.S,
																					 params.Constants.D);
	const float maximum = Hybrid_Max_Corridor_Half_Width(b.CurrentClass,params.Constants.S,
																		  params.Constants.D);
	b.CurrentHalfWidth = maximum;							// the census is about the cells, not the envelope

	DynamicVectorClass<unsigned int> nodes;
	Edge_Node_Chain(b,*edge,&nodes);
	out->EdgeId		= edge_id;
	out->LegCount	= nodes.Count() - 1;
	out->CellCount	= b.CellCount;

	b.CorridorPath.Delete_All();
	for (int n = 0; n < nodes.Count(); ++n) {
		const HybridTopologyNode * node = topology.Find_Node(nodes[n]);
		if (node != nullptr) b.CorridorPath.Add(node->Position);
	}
	(void)preferred;

	for (int leg = 0; leg < (nodes.Count() - 1); ++leg) {

		const HybridTopologyNode * from = topology.Find_Node(nodes[leg]);
		const HybridTopologyNode * to = topology.Find_Node(nodes[leg + 1]);
		if ((from == nullptr) || (to == nullptr)) continue;

		b.CorridorA		= from->Position;
		b.CorridorB		= to->Position;
		b.OwnWindow		= (from->WindowId != 0) ? from->WindowId : to->WindowId;
		b.OwnFieldSite	= 0;
		if (from->Kind == HYBRID_NODE_FIELD_ACCESS)	b.OwnFieldSite = from->SiteId;
		if (to->Kind == HYBRID_NODE_FIELD_ACCESS)		b.OwnFieldSite = to->SiteId;

		const int start = b.Cell_Of(from->Position);
		const int goal = b.Cell_Of(to->Position);
		b.StartCell	= start;
		b.GoalCell	= goal;
		if (start == goal) continue;

		//	Classify every cell, then flood the accepted ones from the start.
		DynamicVectorClass<unsigned int> refusal;
		DynamicVectorClass<unsigned char> reached;
		for (int cell = 0; cell < b.CellCount; ++cell) {
			refusal.Add(Classify_Cell(b,cell));
			reached.Add(0);
		}
		refusal[start]	= 0;								// the frozen endpoints are legal by exemption
		refusal[goal]	= 0;

		DynamicVectorClass<int> queue;
		queue.Add(start);
		reached[start] = 1;
		int head = 0;
		while (head < queue.Count()) {
			const int cell = queue[head++];
			const int x = b.Cell_X(cell);
			const int y = b.Cell_Y(cell);
			for (int direction = 0; direction < DIRECTION_COUNT; ++direction) {
				const int nx = x + NEIGHBOR_DX[direction];
				const int ny = y + NEIGHBOR_DY[direction];
				if (!b.Valid_Cell(nx,ny)) continue;
				const int neighbor = b.Cell_Index(nx,ny);
				if (reached[neighbor] != 0) continue;
				if (refusal[neighbor] != 0) continue;
				reached[neighbor] = 1;
				queue.Add(neighbor);
			}
		}

		/*
		**	A second flood with the move rule the search also obeys: a move is only usable when its grade is
		**	within the class maximum, unless either end is structural (a deck or a bore carries it).  Cells
		**	being reachable while moves are not is the difference between "the ground is walled off" and "the
		**	ground is too steep to drive", and the two have different fixes.
		*/
		{
			const float max_grade = Max2(Hybrid_Road_Class_Spec(b.CurrentClass).MaxGradeDegrees * PI_OVER_180,
												  params.Constants.Q);
			DynamicVectorClass<unsigned char> move_reached;
			for (int cell = 0; cell < b.CellCount; ++cell) move_reached.Add(0);

			DynamicVectorClass<int> move_queue;
			move_queue.Add(start);
			move_reached[start] = 1;
			int move_head = 0;
			int blocked = 0;

			while (move_head < move_queue.Count()) {
				const int cell = move_queue[move_head++];
				const int x = b.Cell_X(cell);
				const int y = b.Cell_Y(cell);
				for (int direction = 0; direction < DIRECTION_COUNT; ++direction) {
					const int nx = x + NEIGHBOR_DX[direction];
					const int ny = y + NEIGHBOR_DY[direction];
					if (!b.Valid_Cell(nx,ny)) continue;
					const int neighbor = b.Cell_Index(nx,ny);
					if (move_reached[neighbor] != 0) continue;
					if (refusal[neighbor] != 0) continue;

					float grade = 0.0f;
					float d = 0.0f;
					if (!Move_Grade(b,cell,neighbor,&grade,&d)) continue;
					const bool structural = (Window_At(b,b.Cell_Center(cell)) != 0) ||
													(Window_At(b,b.Cell_Center(neighbor)) != 0);
					if (!structural && (grade > max_grade)) {
						blocked++;
						continue;
					}
					move_reached[neighbor] = 1;
					move_queue.Add(neighbor);
				}
			}
			out->MoveReachable		= move_queue.Count();
			out->GoalMoveReachable	= (move_reached[goal] != 0);
			out->GradeBlockedMoves	= blocked;

			if ((reached[goal] != 0) && out->GoalMoveReachable) continue;	// this leg is fine
		}

		if (reached[goal] != 0) {
			//	Cells are fine; the grade rule is what disconnects this leg.
			out->Leg					= leg;
			out->GoalReachable	= false;
			out->FromNode			= from->StableId;
			out->GoalNode			= to->StableId;
			out->GoalNodeKind		= (int)to->Kind;
			out->GoalWindowId		= to->WindowId;
			out->StartCell			= start;
			out->GoalCell			= goal;
			out->GoalX				= b.Cell_X(goal);
			out->GoalY				= b.Cell_Y(goal);
			out->Reachable			= queue.Count();
			return true;
		}

		//	This is the leg that is disconnected: describe it.
		out->Leg					= leg;
		out->GoalReachable	= false;
		out->FromNode			= from->StableId;
		out->GoalNode			= to->StableId;
		out->GoalNodeKind		= (int)to->Kind;
		out->GoalWindowId		= to->WindowId;
		out->StartCell			= start;
		out->GoalCell			= goal;
		out->GoalX				= b.Cell_X(goal);
		out->GoalY				= b.Cell_Y(goal);
		out->Reachable			= queue.Count();
		out->NearestReachableDistance = 3.4e38f;

		const Vector2 goal_point = b.Cell_Center(goal);

		for (int cell = 0; cell < b.CellCount; ++cell) {

			const unsigned int mask = refusal[cell];
			if (mask == 0) {
				out->Accepted++;
				if (reached[cell] != 0) {
					const float distance = (b.Cell_Center(cell) - goal_point).Length();
					if (distance < out->NearestReachableDistance) out->NearestReachableDistance = distance;
				}
				continue;
			}

			int rules = 0;
			for (int rule = 0; rule < HYBRID_ROUTE_RULE_COUNT; ++rule) {
				if ((mask & (1u << rule)) == 0) continue;
				out->Refused[rule]++;
				rules++;
			}
			if (rules > 1) out->RefusedByMoreThanOne++;

			//	Is this refused cell on the frontier -- next to something the flood did reach?
			bool frontier = false;
			const int x = b.Cell_X(cell);
			const int y = b.Cell_Y(cell);
			for (int direction = 0; (direction < DIRECTION_COUNT) && !frontier; ++direction) {
				const int nx = x + NEIGHBOR_DX[direction];
				const int ny = y + NEIGHBOR_DY[direction];
				if (!b.Valid_Cell(nx,ny)) continue;
				if (reached[b.Cell_Index(nx,ny)] != 0) frontier = true;
			}
			if (!frontier) continue;

			out->FrontierCells++;
			for (int rule = 0; rule < HYBRID_ROUTE_RULE_COUNT; ++rule) {
				if ((mask & (1u << rule)) != 0) out->Frontier[rule]++;
			}
		}

		for (int direction = 0; direction < DIRECTION_COUNT; ++direction) {

			const int nx = out->GoalX + NEIGHBOR_DX[direction];
			const int ny = out->GoalY + NEIGHBOR_DY[direction];
			out->NeighborCell[direction]		= -1;
			out->NeighborX[direction]			= nx;
			out->NeighborY[direction]			= ny;
			out->NeighborRefusal[direction]	= 0;
			if (!b.Valid_Cell(nx,ny)) continue;

			const int neighbor = b.Cell_Index(nx,ny);
			out->NeighborCell[direction]		= neighbor;
			out->NeighborRefusal[direction]	= refusal[neighbor];
		}
		return true;
	}

	//	Every leg's goal was reachable: the disconnection is not in the cell rules.
	out->GoalReachable = true;
	return true;
}
