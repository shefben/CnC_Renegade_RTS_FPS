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
static const float	HEURISTIC_WEIGHT			= 0.75f;		// 0.75 * Euclidean
static const float	COLLINEAR_RUN_W			= 6.0f;		// the >6W collinear rule
static const float	JUNCTION_REACH_W			= 4.0f;		// a junction owns max(4W, R) of each road through it
static const float	PARALLEL_DEGREES			= 30.0f;	// Q-025: roads within this of each other run together
static const float	OVERPASS_ARM_ROUTES_W	= 10.0f;	// an overpass keeps 10W of straight either side (step 12)

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
static const unsigned int	ALIGNED_ID_BASE		= 0x40000000u;	// an interchange's aligned straight, never a window id
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
		case HYBRID_ROUTE_PRIORITY_TRAIL:				return "trail";
		case HYBRID_ROUTE_PRIORITY_INTERCHANGE:		return "interchange";
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
		case HYBRID_ROUTE_INTERCHANGE_INVALID:	return "interchange_invalid";
		case HYBRID_ROUTE_CROSSING_MISALIGNED:	return "crossing_misaligned";
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


int Hybrid_Route_Priority_Rank(HybridRoutePriority priority)
{
	if (priority == HYBRID_ROUTE_PRIORITY_FAMILY_TRUNK)	return 0;
	if (priority == HYBRID_ROUTE_PRIORITY_INTERCHANGE)		return 1;
	return (int)priority + 1;
}


HybridRoutePriority Hybrid_Route_Priority_Of(const HybridTopology & topology,const HybridTopologyEdge & edge)
{
	switch (edge.Kind) {

		case HYBRID_EDGE_FAMILY_TRUNK:	return HYBRID_ROUTE_PRIORITY_FAMILY_TRUNK;
		case HYBRID_EDGE_HOME_ROAD:		return HYBRID_ROUTE_PRIORITY_HOME_ROAD;
		case HYBRID_EDGE_FIELD_BRANCH:	return HYBRID_ROUTE_PRIORITY_FIELD_BRANCH;
		case HYBRID_EDGE_SERVICE_FILL:	return HYBRID_ROUTE_PRIORITY_SERVICE_FILL;
		case HYBRID_EDGE_TRAIL:				return HYBRID_ROUTE_PRIORITY_TRAIL;
		case HYBRID_EDGE_HIGHWAY_RAMP:
		case HYBRID_EDGE_INTERCHANGE_CROSSING:	return HYBRID_ROUTE_PRIORITY_INTERCHANGE;

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


float Hybrid_Junction_Reach(HybridRoadClass road_class,float lane_width)
{
	return Max2(JUNCTION_REACH_W * lane_width,Hybrid_Road_Class_Spec(road_class).MinTurnRadiusW * lane_width);
}


float Hybrid_Parallel_Clearance(HybridRoadClass a,HybridRoadClass c,float lane_width,float lane_clearance)
{
	return (0.5f * Hybrid_Roadbed_Width(a,lane_width)) + (0.5f * Hybrid_Roadbed_Width(c,lane_width)) + lane_clearance;
}


const HybridTrunkRoad * HybridRouteSet::Find_Trunk(unsigned int edge_id) const
{
	for (int i = 0; i < Trunks.Count(); ++i) {
		for (int e = 0; e < Trunks[i].EdgeIds.Count(); ++e) {
			if (Trunks[i].EdgeIds[e] == edge_id) return &Trunks[i];
		}
	}
	return nullptr;
}


void HybridRouteSet::Reset(void)
{
	Routes.Delete_All();
	Trunks.Delete_All();
	WithdrawnInterchanges.Delete_All();
	AnalysisCountX		= 0;
	AnalysisCountY		= 0;
	TotalExpansions	= 0;
	TotalInsertions	= 0;
	WidenedRoutes		= 0;
	Checksum				= 0;
}


const char * Hybrid_Interchange_Outcome_Name(HybridInterchangeOutcome outcome)
{
	switch (outcome) {
		case HYBRID_INTERCHANGE_REALIZED:								return "INTERCHANGE_REALIZED";
		case HYBRID_INTERCHANGE_OUTCOME_WITHDRAWN:					return "INTERCHANGE_WITHDRAWN";
		case HYBRID_INTERCHANGE_WORLD_FAILED_BEFORE_REALIZATION:	return "WORLD_FAILED_BEFORE_INTERCHANGE_REALIZATION";
		default:																	break;
	}
	return "unknown";
}


HybridInterchangeOutcome Hybrid_Interchange_Outcome(const HybridRouteResult & result,unsigned int interchange_id)
{
	for (int i = 0; i < result.WithdrawnInterchanges.Count(); ++i) {
		if (result.WithdrawnInterchanges[i] == interchange_id) return HYBRID_INTERCHANGE_OUTCOME_WITHDRAWN;
	}
	for (int i = 0; i < result.RealizedInterchanges.Count(); ++i) {
		if (result.RealizedInterchanges[i] == interchange_id) return HYBRID_INTERCHANGE_REALIZED;
	}
	return HYBRID_INTERCHANGE_WORLD_FAILED_BEFORE_REALIZATION;
}


const char * Hybrid_Interchange_World_Outcome_Name(HybridInterchangeWorldOutcome outcome)
{
	switch (outcome) {
		case HYBRID_WORLD_ROUTED:											return "WORLD_ROUTED";
		case HYBRID_WORLD_FAILED_BEFORE_INTERCHANGE_REALIZATION:	return "WORLD_FAILED_BEFORE_INTERCHANGE_REALIZATION";
		case HYBRID_WORLD_FAILED_AFTER_INTERCHANGE_WITHDRAWAL:	return "WORLD_FAILED_AFTER_INTERCHANGE_WITHDRAWAL";
		case HYBRID_WORLD_FAILED_AFTER_INTERCHANGE_REALIZATION:	return "WORLD_FAILED_AFTER_INTERCHANGE_REALIZATION";
		default:																	break;
	}
	return "unknown";
}


HybridInterchangeWorldOutcome Hybrid_Interchange_World_Outcome(const HybridRouteResult & result,unsigned int interchange_id)
{
	if (result.Failure == HYBRID_ROUTE_OK) return HYBRID_WORLD_ROUTED;
	switch (Hybrid_Interchange_Outcome(result,interchange_id)) {
		case HYBRID_INTERCHANGE_OUTCOME_WITHDRAWN:	return HYBRID_WORLD_FAILED_AFTER_INTERCHANGE_WITHDRAWAL;
		case HYBRID_INTERCHANGE_REALIZED:				return HYBRID_WORLD_FAILED_AFTER_INTERCHANGE_REALIZATION;
		default:													break;
	}
	return HYBRID_WORLD_FAILED_BEFORE_INTERCHANGE_REALIZATION;
}


bool HybridRouteSet::Interchange_Withdrawn(unsigned int interchange_id) const
{
	for (int i = 0; i < WithdrawnInterchanges.Count(); ++i) {
		if (WithdrawnInterchanges[i] == interchange_id) return true;
	}
	return false;
}


unsigned int Hybrid_Route_Edge_End(const HybridTopology & topology,const HybridRouteSet & routes,
											 const HybridTopologyEdge & edge,bool node_b)
{
	const unsigned int node = node_b ? edge.NodeB : edge.NodeA;
	for (int i = 0; i < topology.Interchanges.Count(); ++i) {
		const HybridInterchange & interchange = topology.Interchanges[i];
		if (!routes.Interchange_Withdrawn(interchange.StableId)) continue;
		for (int r = 0; r < interchange.Retargets.Count(); ++r) {
			const HybridInterchangeRetarget & retarget = interchange.Retargets[r];
			if ((retarget.EdgeId == edge.StableId) && (retarget.LocalJunction == node)) return retarget.FallbackNode;
		}
	}
	return node;
}


bool Hybrid_Route_Edge_Withdrawn(const HybridTopology & topology,const HybridRouteSet & routes,
											const HybridTopologyEdge & edge)
{
	(void)topology;
	if ((edge.Kind != HYBRID_EDGE_HIGHWAY_RAMP) && (edge.Kind != HYBRID_EDGE_INTERCHANGE_CROSSING)) return false;
	return routes.Interchange_Withdrawn(edge.InterchangeId);
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
	Arena(nullptr),Counters(nullptr),MinVehicleLaneClearance(1.0f),CollectCensus(false)
{
	memset(&Constants,0,sizeof(Constants));
}


const char * Hybrid_Route_Block_Name(HybridRouteBlock block)
{
	switch (block) {
		case HYBRID_BLOCK_NONE:				return "none";
		case HYBRID_BLOCK_ENVELOPE:		return "envelope";
		case HYBRID_BLOCK_WORLD_BOUNDS:	return "world_bounds";
		case HYBRID_BLOCK_WATER:			return "standing_water";
		case HYBRID_BLOCK_RIVER:			return "river_outside_window";
		case HYBRID_BLOCK_WALL:				return "wall_outside_window";
		case HYBRID_BLOCK_BASE:				return "base_protected";
		case HYBRID_BLOCK_TIBERIUM:		return "tiberium_reserve";
		case HYBRID_BLOCK_GRADE:			return "grade";
		case HYBRID_BLOCK_CROSS_SLOPE:	return "cross_slope";
		case HYBRID_BLOCK_TURN:				return "turn_or_approach_pose";
		case HYBRID_BLOCK_EXISTING_ROAD:	return "existing_road";
		case HYBRID_BLOCK_BUDGET:			return "expansion_budget";
		case HYBRID_BLOCK_SPLINE:			return "centerline";
		default:									break;
	}
	return "unknown";
}


const char * Hybrid_Spline_Failure_Class_Name(HybridSplineFailureClass failure)
{
	switch (failure) {
		case HYBRID_SPLINE_FAILURE_NONE:					return "none";
		case HYBRID_SPLINE_FAILURE_CONTROL_FILLET:	return "control_fillet";
		case HYBRID_SPLINE_FAILURE_STRUCTURAL:			return "structural_transition";
		case HYBRID_SPLINE_FAILURE_QUANTIZATION:		return "quantization";
		case HYBRID_SPLINE_FAILURE_MIN_RADIUS:			return "min_radius";
		case HYBRID_SPLINE_FAILURE_GROUND:				return "ground";
		default:													break;
	}
	return "unknown";
}


const char * Hybrid_Structural_Cause_Name(HybridStructuralCause cause)
{
	switch (cause) {
		case HYBRID_STRUCTURAL_NONE:				return "none";
		case HYBRID_STRUCTURAL_LEAD_BEFORE:		return "lead_before_approach";
		case HYBRID_STRUCTURAL_DEPARTURE_AFTER:return "departure_after_structure";
		case HYBRID_STRUCTURAL_WRONG_SIDE:		return "wrong_side";
		case HYBRID_STRUCTURAL_OBSTACLE:			return "obstacle";
		case HYBRID_STRUCTURAL_ENVELOPE:			return "envelope";
		case HYBRID_STRUCTURAL_GRADE:				return "grade";
		case HYBRID_STRUCTURAL_CROSS_SLOPE:		return "cross_slope";
		case HYBRID_STRUCTURAL_SEQUENCE:			return "sequence_too_close";
		case HYBRID_STRUCTURAL_Q023_MISMATCH:	return "q023_phase10_mismatch";
		case HYBRID_STRUCTURAL_UNPLANNED_CROSSING:return "unplanned_crossing";
		case HYBRID_STRUCTURAL_FAR_BANK:			return "far_bank_approach";
		default:											break;
	}
	return "unknown";
}


void HybridRouteFailureCensus::Reset(void)
{
	EdgeId					= 0;
	RoadClass				= -1;
	EdgeKind					= -1;
	Envelope					= -1;
	StructuralWaypoints	= 0;
	Leg						= -1;
	Legs						= 0;
	LegFrom					= 0;
	LegTo						= 0;
	LegFromStructural		= false;
	LegToStructural		= false;
	Reason					= HYBRID_BLOCK_NONE;
	SplineBlock				= HYBRID_BLOCK_NONE;
	SplineClass				= HYBRID_SPLINE_FAILURE_NONE;
	StructuralCause		= HYBRID_STRUCTURAL_NONE;
	SplineJointDistance	= -1.0f;
	SplineReason			= "";
	for (int i = 0; i < HYBRID_BLOCK_COUNT; ++i) Frontier[i] = 0;
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
		stream.Add_Uint(route.Capabilities);
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

		//	Section 12.6.15: ramp metadata and a highway's widened intervals.
		if (route.RoadClass == HYBRID_ROAD_HIGHWAY_RAMP) {
			stream.Add_Uint(route.InterchangeId);
			stream.Add_Uint((unsigned int)route.RampRole);
			stream.Add_Uint((unsigned int)route.RampSide);
			stream.Add_Length(route.RampInterval.GoreStation);
			stream.Add_Length(route.RampInterval.From);
			stream.Add_Length(route.RampInterval.To);
			stream.Add_Length(route.RampInterval.FullFrom);
			stream.Add_Length(route.RampInterval.FullTo);
			stream.Add_Length(route.RampInterval.TaperFrom);
			stream.Add_Length(route.RampInterval.TaperTo);
			stream.Add_Length(route.RampRoadbedExtra);
		}
		stream.Add_Int(route.Widened.Count());
		for (int w = 0; w < route.Widened.Count(); ++w) {
			const HybridWidenedInterval & widened = route.Widened[w];
			stream.Add_Uint(widened.InterchangeId);
			stream.Add_Uint(widened.RampEdgeId);
			stream.Add_Uint((unsigned int)widened.Role);
			stream.Add_Uint((unsigned int)widened.Side);
			stream.Add_Length(widened.Interval.GoreStation);
			stream.Add_Length(widened.Interval.From);
			stream.Add_Length(widened.Interval.To);
			stream.Add_Length(widened.Interval.FullFrom);
			stream.Add_Length(widened.Interval.FullTo);
			stream.Add_Length(widened.Interval.TaperFrom);
			stream.Add_Length(widened.Interval.TaperTo);
		}
	}

	stream.Add_Int(set.WithdrawnInterchanges.Count());
	for (int i = 0; i < set.WithdrawnInterchanges.Count(); ++i) stream.Add_Uint(set.WithdrawnInterchanges[i]);

	stream.Add_Int(set.Trunks.Count());
	for (int i = 0; i < set.Trunks.Count(); ++i) {
		const HybridTrunkRoad & trunk = set.Trunks[i];
		stream.Add_Uint(trunk.StableId);
		stream.Add_Int(trunk.RouteFamily);
		stream.Add_Uint((unsigned int)trunk.RoadClass);
		stream.Add_Uint(trunk.Capabilities);
		stream.Add_Length(trunk.Length);
		stream.Add_Int(trunk.EdgeIds.Count());
		for (int e = 0; e < trunk.EdgeIds.Count(); ++e) {
			stream.Add_Uint(trunk.EdgeIds[e]);
			stream.Add_Length(trunk.EdgeFrom[e]);
			stream.Add_Length(trunk.EdgeTo[e]);
		}
		stream.Add_Int(trunk.Nodes.Count());
		for (int n = 0; n < trunk.Nodes.Count(); ++n) stream.Add_Uint(trunk.Nodes[n]);
		stream.Add_Int(trunk.JunctionNodes.Count());
		for (int n = 0; n < trunk.JunctionNodes.Count(); ++n) stream.Add_Uint(trunk.JunctionNodes[n]);
		stream.Add_Int(trunk.Points.Count());
		for (int q = 0; q < trunk.Points.Count(); ++q) {
			stream.Add_Length(trunk.Points[q].X);
			stream.Add_Length(trunk.Points[q].Y);
		}
		stream.Add_Int(trunk.Structural.Count());
		for (int s = 0; s < trunk.Structural.Count(); ++s) {
			stream.Add_Uint(trunk.Structural[s].WindowId);
			stream.Add_Int(trunk.Structural[s].FirstPoint);
			stream.Add_Int(trunk.Structural[s].LastPoint);
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
	unsigned int *						CacheEdge;			// per cell, its four forward moves: bit d measured, bit 4+d crosses a river
	unsigned int *						CacheRefusal;
	unsigned int *						CacheWindow;
	float *								CacheHeight;
	unsigned int *						CacheRouteMask;	// bit per already-routed route, by route index
	unsigned int *						CacheParallelMask;// ...within the Q-025 clearance of this road's class
	unsigned char *					CacheHasHeight;
	int									Generation;

	int *									Heap;
	int									HeapCount;
	int									HeapCapacity;
	bool									HeapOverflowed;
	DynamicVectorClass<int>			Block;			// owner when there is no attempt arena

	//	The census (HybridRouteParams::CollectCensus): every rule that refused a move into a cell, this leg.
	DynamicVectorClass<unsigned int>	BlockMask;
	int									LastFrontier[HYBRID_BLOCK_COUNT];

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
	DynamicVectorClass<float>			CorridorHalfWidths;	// per CorridorPath segment; empty = CurrentHalfWidth
	unsigned int						CurrentEdgeId;

	/*
	**	The physical road being routed (Q-025): every topology node it passes, the entrance corridors it may use
	**	(the base-entrance nodes among its own ends), and, for each road already on the ground, the nodes the
	**	two share -- the junctions that alone may bring two independent roads together.
	*/
	DynamicVectorClass<unsigned int>	RoadNodes;
	DynamicVectorClass<unsigned int>	OwnEntranceNodes;

	//	A trunk's joints: the frozen positions of the ordinary family waypoints between its logical edges.
	DynamicVectorClass<Vector2>		JointPoints;
	//	Every ordinary guide on the road -- joints and interior family waypoints -- by cell and frozen position.
	DynamicVectorClass<int>				GuideCells;
	DynamicVectorClass<Vector2>		GuidePoints;
	DynamicVectorClass<int>				SharedRoute;			// index into Out->Routes
	DynamicVectorClass<Vector2>		SharedPoint;			// the shared node's position
	bool									CurrentIsOptional;
	unsigned int						OwnWindow;			// the window this leg is allowed to cross, 0 for none
	unsigned int						ClosedWindow;		// a planned window this leg only approaches or leaves
	DynamicVectorClass<unsigned int>	ForbiddenWindows;	// windows this road may not cross (Route_Road)

	/*
	**	Section 12.6.9: the active interchanges' footprint polygons (world points; polygon i is FootprintCount[i]
	**	points from FootprintStart[i]), hard-infinite for every road routed after group 1b except in its own local
	**	junction's cell, which is that road's own endpoint and so already exempt as its start or goal cell.
	*/
	DynamicVectorClass<Vector2>		FootprintPoints;
	DynamicVectorClass<int>				FootprintStart;
	DynamicVectorClass<int>				FootprintCount;
	unsigned int							LastFailedEdge;		// the logical edge Route_Road last failed on
	HybridRouteResult *					Result;				// the caller's result, for the interchange outcomes
	unsigned int						OwnFieldSite;		// the Tiberium reserve this leg may enter, 0 for none
	unsigned int						OwnEndFieldSite;	// the centerline pass: the field at the road's other end

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
	**	it starts and ends, and the interaction phase compiles the deck and the ramps onto it -- so
	**	the class's own turn geometry is not asked of it, exactly as the grade and cross-slope tests are not.
	*/
	bool									StructuralLeg;
	bool									AlignedLeg;			// a leg of an interchange's aligned highway straight

	/*
	**	A leg that ends on a crossing's first frozen approach point has to *arrive* along the crossing's axis,
	**	and a leg that starts on its last one has to *leave* along it (revised D-009, Q-022): the crossing is
	**	entered and left straight, and the turn onto it is made on open road before the approach.  Zero when the
	**	leg's end is not a crossing.
	*/
	Vector2								ArrivalAxis;
	Vector2								DepartureAxis;

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
	const int cache_words = (b.CellCount * 7) + (((b.CellCount) + 3) / 4);
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
	b.CacheParallelMask= (unsigned int *)cursor;			cursor += b.CellCount;
	b.CacheEdge			= (unsigned int *)cursor;			cursor += b.CellCount;
	b.CacheHasHeight	= (unsigned char *)cursor;
	memset(b.CacheStamp,0,(size_t)(b.CellCount * (int)sizeof(int)));
	memset(b.CacheEdge,0,(size_t)(b.CellCount * (int)sizeof(unsigned int)));
	b.Generation = 1;
	return true;
}


/***********************************************************************************************
**	Hard-invalid cells, and the per-leg exemptions the plan lists.
***********************************************************************************************/

//	Does this accepted window's crossable footprint cover the point?
static bool Window_Covers(const RouteBuilder & b,const HybridCrossingWindow & window,const Vector2 & world)
{
	Vector2 axis = window.ApproachB - window.ApproachA;
	const float length = axis.Length();
	if (length <= 0.0f) return false;

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
	return ((world - nearest).Length() <= half);
}


//	Does an accepted window cover this point -- the only place a road may cross water or a wall?
static unsigned int Window_At(const RouteBuilder & b,const Vector2 & world)
{
	for (int i = 0; i < b.Hydrology->Windows.Count(); ++i) {
		if (Window_Covers(b,b.Hydrology->Windows[i],world)) return b.Hydrology->Windows[i].StableId;
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
	HYBRID_REFUSE_WALL			= 1 << 4,		// a cliff, canyon or ravine wall outside an accepted window
	HYBRID_REFUSE_CHANNEL		= 1 << 5,
	HYBRID_REFUSE_FOOTPRINT		= 1 << 6,		// an active interchange's footprint (12.6.9 item 4)		// a river channel outside an accepted window
};


//	DynamicVectorClass<int> cannot tell Delete(index) from Delete(value), so an int entry is removed by copying.
static void Erase_Int(DynamicVectorClass<int> * values,int index)
{
	DynamicVectorClass<int> kept;
	for (int i = 0; i < values->Count(); ++i) {
		if (i != index) kept.Add((*values)[i]);
	}
	values->Delete_All();
	for (int i = 0; i < kept.Count(); ++i) values->Add(kept[i]);
}


//	Is `window` one this road may not cross at all?
static bool Window_Forbidden(const RouteBuilder & b,unsigned int window)
{
	for (int i = 0; i < b.ForbiddenWindows.Count(); ++i) {
		if (b.ForbiddenWindows[i] == window) return true;
	}
	return false;
}


//	Is `point` inside footprint polygon `f` (even-odd rule)?
static bool Point_In_Footprint(const RouteBuilder & b,int f,const Vector2 & point)
{
	const int first = b.FootprintStart[f];
	const int count = b.FootprintCount[f];
	bool inside = false;
	for (int i = 0, j = count - 1; i < count; j = i++) {
		const Vector2 & pi = b.FootprintPoints[first + i];
		const Vector2 & pj = b.FootprintPoints[first + j];
		if (((pi.Y > point.Y) != (pj.Y > point.Y)) &&
			 (point.X < (((pj.X - pi.X) * (point.Y - pi.Y)) / (pj.Y - pi.Y)) + pi.X)) inside = !inside;
	}
	return inside;
}


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

			//	Only the corridor this road actually uses: its own end node's entrance.
			bool own = false;
			for (int n = 0; (n < b.OwnEntranceNodes.Count()) && !own; ++n) {
				if (b.OwnEntranceNodes[n] == entrance.NodeId) own = true;
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
		if ((anchor.SiteId == b.OwnFieldSite) || ((b.OwnEndFieldSite != 0) && (anchor.SiteId == b.OwnEndFieldSite))) continue;
		refusals |= HYBRID_REFUSE_TIBERIUM;
	}

	/*
	**	Standing water is not crossable open road (Phase 6 step 20) -- but an accepted crossing window is not
	**	open road.  A window is a frozen interval across an obstruction, and the obstruction under one is
	**	whatever Phase 6 put there: a river channel, a canyon, or the standing water a channel widens into.
	**	Refusing it even inside the window left a family trunk with no route through its own obligation,
	**	because the two frozen approach points stand on the two banks and every cell between them is wet.
	**
	**	A road crosses its planned window once, on the leg that is that crossing.  The legs that approach or
	**	leave it find its obstruction closed: open, it let a leg reach the near bank from the far one by crossing
	**	the same window outside its frozen interval, a crossing nothing recorded and nothing would build.
	*/
	const unsigned int window = Window_At(b,centre);
	const bool open_window = (window != 0) && (window != b.ClosedWindow) && !Window_Forbidden(b,window);
	if (Hybrid_Point_In_Standing_Water(*b.Hydrology,centre,nullptr,nullptr) && !open_window) {
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
	if ((wet || wall) && !open_window) refusals |= (wall ? HYBRID_REFUSE_WALL : HYBRID_REFUSE_CHANNEL);

	//	Section 12.6.9 item 4: an active interchange's footprint, for every road routed after group 1b.
	for (int f = 0; f < b.FootprintStart.Count(); ++f) {
		if (Point_In_Footprint(b,f,centre)) refusals |= HYBRID_REFUSE_FOOTPRINT;
	}

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
	unsigned int parallel = 0;
	const float w = b.P->Constants.W;
	const int limit = (b.Out->Routes.Count() < 32) ? b.Out->Routes.Count() : 32;
	for (int r = 0; r < limit; ++r) {
		const HybridRoute & route = b.Out->Routes[r];
		/*
		**	The search reasons on cell centres while the centerline it hands over is smoothed through them and can
		**	sit up to half a cell off, so the search holds itself half a cell further from another road than the
		**	rule requires -- as it holds itself to a margin on curvature.  The centerline check stays exact.
		*/
		const float clearance = Hybrid_Parallel_Clearance(b.CurrentClass,route.RoadClass,w,b.P->MinVehicleLaneClearance) +
										route.RampRoadbedExtra + (0.5f * b.A);
		for (int p = 0; p < route.Points.Count(); ++p) {
			const float distance = (route.Points[p] - centre).Length();
			if (distance <= w) mask |= (1u << r);
			if (distance < clearance) parallel |= (1u << r);
			if (((mask & (1u << r)) != 0) && ((parallel & (1u << r)) != 0)) break;
		}
	}
	b.CacheRouteMask[cell]	= mask;
	b.CacheParallelMask[cell]= parallel;
	b.CacheStamp[cell]		= b.Generation;
}


static void Touch_Cell(RouteBuilder & b,int cell)
{
	if (b.CacheStamp[cell] != b.Generation) Fill_Cell_Cache(b,cell);
}


/*
**	Where the straight segment `a`-`c` comes within a river's half width of its centerline, if it does: the river
**	geometry is fixed, so this is the same answer for every leg.
*/
static bool Segment_Meets_River(const RouteBuilder & b,const Vector2 & a,const Vector2 & c,Vector2 * at_out)
{
	const Vector2 lo(Min2(a.X,c.X),Min2(a.Y,c.Y));
	const Vector2 hi(Max2(a.X,c.X),Max2(a.Y,c.Y));
	for (int r = 0; r < b.Hydrology->Rivers.Count(); ++r) {

		const HybridRiverSystem & river = b.Hydrology->Rivers[r];
		if ((hi.X < river.BoundsMin.X) || (lo.X > river.BoundsMax.X) ||
			 (hi.Y < river.BoundsMin.Y) || (lo.Y > river.BoundsMax.Y)) continue;

		for (int i = 1; i < river.Points.Count(); ++i) {

			const Vector2 & p = river.Points[i - 1];
			const Vector2 & q = river.Points[i];
			const float half = 0.5f * Max2(river.Width_At_Index(i - 1),river.Width_At_Index(i));
			if ((Max2(p.X,q.X) < (lo.X - half)) || (Min2(p.X,q.X) > (hi.X + half)) ||
				 (Max2(p.Y,q.Y) < (lo.Y - half)) || (Min2(p.Y,q.Y) > (hi.Y + half))) continue;

			//	The two segments cross: the move meets the centerline itself.
			const Vector2 r_dir = c - a;
			const Vector2 s_dir = q - p;
			const float denominator = (r_dir.X * s_dir.Y) - (r_dir.Y * s_dir.X);
			if ((float)fabs((double)denominator) > 1.0e-6f) {
				const Vector2 d = p - a;
				const float u = ((d.X * s_dir.Y) - (d.Y * s_dir.X)) / denominator;
				const float v = ((d.X * r_dir.Y) - (d.Y * r_dir.X)) / denominator;
				if ((u >= 0.0f) && (u <= 1.0f) && (v >= 0.0f) && (v <= 1.0f)) {
					if (at_out != nullptr) *at_out = a + (r_dir * u);
					return true;
				}
			}

			//	Otherwise the nearest approach, which is at an end of one of the two.
			const Vector2 ends[4] = { a,c,p,q };
			for (int e = 0; e < 4; ++e) {
				const Vector2 & from = (e < 2) ? p : a;
				const Vector2 & to = (e < 2) ? q : c;
				const Vector2 seg = to - from;
				const float length2 = (seg.X * seg.X) + (seg.Y * seg.Y);
				float t_on = (length2 > 0.0f) ? ((((ends[e].X - from.X) * seg.X) + ((ends[e].Y - from.Y) * seg.Y)) / length2) : 0.0f;
				t_on = Clamp2(t_on,0.0f,1.0f);
				const Vector2 nearest = from + (seg * t_on);
				if ((ends[e] - nearest).Length() > half) continue;
				if (at_out != nullptr) *at_out = (e < 2) ? ends[e] : nearest;
				return true;
			}
		}
	}
	return false;
}


/*
**	Does the move from `cell` to its neighbour in `direction` cross a river channel outside a window this leg may
**	cross?  The hard-infinite cell list judges cells by their centres, and a river 2.5 to 6 units wide runs
**	between the centres of 8-to-16-unit analysis cells almost everywhere, so the search stepped across rivers --
**	its own planned window's included, on the legs that only approach or leave it -- wherever no centre happened
**	to land in the water.  The move itself is judged: where it meets a river, that point must lie in a window the
**	leg may use.
*/
static bool Move_Crosses_Channel(RouteBuilder & b,int cell,int direction,int neighbor)
{
	const int from = (direction < 4) ? cell : neighbor;
	const int to = (direction < 4) ? neighbor : cell;
	const int d = (direction < 4) ? direction : (direction - 4);
	const unsigned int measured = 1u << (unsigned int)d;
	const unsigned int crosses = 1u << (unsigned int)(4 + d);
	if ((b.CacheEdge[from] & measured) == 0) {
		if (Segment_Meets_River(b,b.Cell_Center(from),b.Cell_Center(to),nullptr)) b.CacheEdge[from] |= crosses;
		b.CacheEdge[from] |= measured;
	}
	if ((b.CacheEdge[from] & crosses) == 0) return false;

	Vector2 at(0.0f,0.0f);
	if (!Segment_Meets_River(b,b.Cell_Center(from),b.Cell_Center(to),&at)) return false;
	const unsigned int window = Window_At(b,at);
	return (window == 0) || (window == b.ClosedWindow) || Window_Forbidden(b,window);
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


static unsigned int Cached_Parallel_Mask(RouteBuilder & b,int cell)
{
	Touch_Cell(b,cell);
	return b.CacheParallelMask[cell];
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

	//	A physical road of several logical edges (Q-025) measures each piece of its corridor with that edge's own
	//	envelope width.
	float excess = 3.4e38f;
	if (b.CorridorPath.Count() >= 2) {
		for (int i = 0; i < (b.CorridorPath.Count() - 1); ++i) {
			const float half = (i < b.CorridorHalfWidths.Count()) ? b.CorridorHalfWidths[i] : b.CurrentHalfWidth;
			distance = Distance_To_Segment(world,b.CorridorPath[i],b.CorridorPath[i + 1]);
			excess = Min2(excess,Max2(0.0f,distance - half));
		}
	} else {
		distance = Distance_To_Segment(world,b.CorridorA,b.CorridorB);
		excess = Max2(0.0f,distance - b.CurrentHalfWidth);
	}
	if (excess >= 3.4e38f) return 0.0f;
	return excess / Max2(b.P->Constants.S,b.P->Constants.Q);
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


/*
**	Is `point` inside the road a real junction owns, between this road and Out->Routes[route]?  The nodes the
**	two share are the only places two independent roads may come together (Q-025); within the longer of the
**	two roads' junction reaches of one, they converge legitimately.
*/
static bool Near_Shared_Node(const RouteBuilder & b,int route,const Vector2 & point)
{
	/*
	**	The search judges a whole cell by its centre, so it exempts half a cell less than the junction reach:
	**	the first cell it counts then starts where the centerline check starts counting, not a cell later.
	*/
	const float w = b.P->Constants.W;
	const float reach = Max2(Hybrid_Junction_Reach(b.CurrentClass,w),Hybrid_Junction_Reach(b.Out->Routes[route].RoadClass,w)) -
							  (0.5f * b.A);
	for (int i = 0; i < b.SharedRoute.Count(); ++i) {
		if (b.SharedRoute[i] != route) continue;
		if ((b.SharedPoint[i] - point).Length() <= reach) return true;
	}
	return false;
}


static Vector2 Unit(const Vector2 & v);


//	The direction of a route's centerline nearest a point.
static Vector2 Route_Direction_Near(const HybridRoute & route,const Vector2 & point)
{
	int at = 1;
	float nearest = 3.4e38f;
	for (int i = 1; i < route.Points.Count(); ++i) {
		const float distance = Distance_To_Segment(point,route.Points[i - 1],route.Points[i]);
		if (distance >= nearest) continue;
		nearest	= distance;
		at			= i;
	}
	if (route.Points.Count() < 2) return Vector2(0.0f,0.0f);
	return Unit(route.Points[at] - route.Points[at - 1]);
}


static unsigned int Cached_Parallel_Mask(RouteBuilder & b,int cell);


/*
**	Q-025's common rule for independently realized roads, as the search applies it: a move that runs within 30
**	degrees of an already-routed road, closer than the two classes' ParallelCenterlineClearance, and has done
**	so for more than `6W` outside the junction the two roads share, would lay this road's roadbed over the
**	other's.  The search refuses it so the centerline check (First_Failing_Sample) is not handed one; the
**	check on the centerline stays the authority.
*/
static bool Move_Runs_Parallel(RouteBuilder & b,int state,int from,int to)
{
	const unsigned int shared = Cached_Parallel_Mask(b,from) & Cached_Parallel_Mask(b,to);
	if (shared == 0) return false;

	//	One cell inside the `6W`, for the same reason as the half-cell clearance margin.
	const float limit = Max2((COLLINEAR_RUN_W * b.P->Constants.W) - b.A,b.A);
	const float parallel_cosine = (float)::cos((double)(PARALLEL_DEGREES * PI_OVER_180));
	const int count = (b.Out->Routes.Count() < 32) ? b.Out->Routes.Count() : 32;

	for (int r = 0; r < count; ++r) {

		if ((shared & (1u << r)) == 0) continue;
		if (Near_Shared_Node(b,r,b.Cell_Center(to)) || Near_Shared_Node(b,r,b.Cell_Center(from))) continue;

		//	The run beside this road, back along the parent chain while it stays inside the clearance.
		float run = (b.Cell_Center(to) - b.Cell_Center(from)).Length();
		int first = from;
		int walk = state;
		int guard = b.CellCount;
		int current = from;
		while ((walk >= 0) && (guard-- > 0) && (run <= limit)) {
			const int parent_cell = b.States[walk].FromCell;
			const int parent_direction = b.States[walk].FromDirection;
			if (parent_cell < 0) break;
			if ((Cached_Parallel_Mask(b,parent_cell) & (1u << r)) == 0) break;
			if (Near_Shared_Node(b,r,b.Cell_Center(parent_cell))) break;
			run += (b.Cell_Center(current) - b.Cell_Center(parent_cell)).Length();
			first		= parent_cell;
			current	= parent_cell;
			walk		= State_Index(b,parent_cell,parent_direction);
		}
		if (run <= limit) continue;

		/*
		**	Running *along* the road, judged over the run rather than the last 8-way step: a staircase of diagonal
		**	and straight moves reads 45 degrees a step at a time, while the curve smoothed through it runs within
		**	30 degrees of the other road all the way.
		*/
		const Vector2 heading = Unit(b.Cell_Center(to) - b.Cell_Center(first));
		const Vector2 along = Route_Direction_Near(b.Out->Routes[r],b.Cell_Center(to));
		if ((float)fabs((double)((heading.X * along.X) + (heading.Y * along.Y))) >= parallel_cosine) return true;
	}
	return false;
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


//	Defined with the spline conversion below.
static Vector2 Unit(const Vector2 & v);


//	The unit vector of one 8-way direction.
static Vector2 Direction_Unit(int direction)
{
	if ((direction < 0) || (direction >= DIRECTION_COUNT)) return Vector2(0.0f,0.0f);
	return Unit(Vector2((float)NEIGHBOR_DX[direction],(float)NEIGHBOR_DY[direction]));
}


//	The 8-way direction nearest an arbitrary heading; the lower ordinal on a tie.
static int Nearest_Direction(const Vector2 & heading)
{
	int best = START_DIRECTION;
	float best_dot = -2.0f;
	for (int d = 0; d < DIRECTION_COUNT; ++d) {
		const Vector2 unit = Direction_Unit(d);
		const float dot = (unit.X * heading.X) + (unit.Y * heading.Y);
		if (dot > best_dot) {
			best_dot = dot;
			best = d;
		}
	}
	return best;
}


/*
**	May a move along `direction` leave or enter a crossing whose axis is `axis`?  The grid has eight headings
**	and a crossing axis any, so the road lines up with the crossing in its own turn before the approach; what
**	the search guarantees is that the turn it leaves for that is no more than one grid step, 45 degrees.
*/
static bool Direction_Fits_Axis(int direction,const Vector2 & axis)
{
	const Vector2 unit = Direction_Unit(direction);
	if (unit.Length() <= 0.0f) return false;
	const float dot = Clamp2((unit.X * axis.X) + (unit.Y * axis.Y),-1.0f,1.0f);
	return ((float)::acos((double)dot) <= ((45.0f * PI_OVER_180) + HYBRID_QUANTUM_ANGLE));
}


/*
**	The straight a structural transition needs on the chain beside a crossing, either side: the turn off (or
**	onto) the crossing's axis from the chain's grid heading, `phi`, and the chain's own next turn beyond it,
**	`turn`, each consume `R tan(angle/2)` of it, and the straight kept between the frozen approach point and
**	the arc comes on top.  It is the corner construction's own measure (Lead_Before), so arrival and departure
**	are judged by one rule; the corner construction remains the authority.
*/
static float Transition_Room(float radius,const Vector2 & axis,int heading,float turn,float straight)
{
	const Vector2 unit = Direction_Unit(heading);
	const float dot = Clamp2((unit.X * axis.X) + (unit.Y * axis.Y),-1.0f,1.0f);
	const float phi = (float)::acos((double)dot);
	return (radius * ((float)::tan((double)(0.5f * phi)) + (float)::tan((double)(0.5f * turn)))) + straight;
}


/*
**	Does a search that has reached its goal arrive in a way the crossing ahead can be turned onto?
**
**	The heading has to fit the axis, and the turn still left -- `phi`, at most one grid step -- has to fit on
**	the straight the chain arrives along: its own arc `R tan(phi/2)`, the straight kept before the frozen
**	approach point, and the arc of the grid step the chain turned onto that straight with, `R tan(22.5deg)`.
**	That is the corner construction's own measure of the room the lead needs, so a goal accepted here is one
**	the lead can be laid at; the corner construction remains the authority and refuses what does not fit.
*/
static bool Arrival_Fits_Crossing(RouteBuilder & b,int state,int cell,int goal,int incoming,float min_turn_radius)
{
	if (b.ArrivalAxis.Length() <= 0.0f) return true;
	if (!Direction_Fits_Axis(incoming,b.ArrivalAxis)) return false;
	if (min_turn_radius <= 0.0f) return true;

	//	The turn the chain made onto this straight is at most one grid step, 45 degrees.
	const float straight = Min2(b.P->Constants.W,0.5f * b.A);
	const float needed = Transition_Room(min_turn_radius,b.ArrivalAxis,incoming,45.0f * PI_OVER_180,straight);

	/*
	**	The straight run the chain arrives along: the move into the goal from `cell` (whose state is `state`),
	**	and the run behind it while the chain kept that heading, carried across the leg boundary when the leg
	**	started on it.  Judged when the move is generated, not when the goal is popped: a goal state is closed
	**	at its first arrival, and its cheapest path is not necessarily one with room for the lead.
	*/
	float run = (b.Cell_Center(goal) - b.Cell_Center(cell)).Length();
	if ((state % STATE_COUNT) != incoming) return (run >= needed);
	int walk = state;
	int guard = b.CellCount;
	while ((walk >= 0) && (guard-- > 0) && (run < needed)) {
		const int here = walk / STATE_COUNT;
		const int parent_cell = b.States[walk].FromCell;
		const int parent_direction = b.States[walk].FromDirection;
		if (parent_cell < 0) {
			if (b.EntryDirection == incoming) run += b.EntryRun;
			break;
		}
		run += (b.Cell_Center(here) - b.Cell_Center(parent_cell)).Length();
		if ((parent_direction != START_DIRECTION) && (parent_direction != incoming)) break;
		walk = State_Index(b,parent_cell,parent_direction);
	}
	return (run >= needed);
}


/*
**	Does the first turn after a crossing leave the departure transition room?  The same rule as the arrival
**	(Transition_Room), on the other side of the structure.
**
**	The leg leaving a crossing's far approach point is handed the crossing's own heading and the whole frozen
**	straight as run behind it, so the turn-spacing rule let it turn as soon as it left the egress cell.  But the
**	frozen straight is not room for an arc: the turn off the axis onto the grid heading the chain left on and
**	the chain's own first turn both have to fit on the straight the chain runs from the egress.  The corner
**	construction remains the authority; this keeps the search from building a departure it is certain to refuse.
*/
static bool Departure_Fits_Crossing(RouteBuilder & b,int state,int incoming,int direction,float min_turn_radius)
{
	if ((b.DepartureAxis.Length() <= 0.0f) || (min_turn_radius <= 0.0f)) return true;
	if ((incoming == START_DIRECTION) || (direction == incoming)) return true;
	if (b.States[state].FromCell < 0) return true;			// the first move itself: Direction_Fits_Axis's question

	//	The straight run from the leg's start to here, provided the chain has not turned on it already.
	float run = 0.0f;
	int walk = state;
	int guard = b.CellCount;
	while ((walk >= 0) && (guard-- > 0)) {
		const int here = walk / STATE_COUNT;
		const int parent_cell = b.States[walk].FromCell;
		const int parent_direction = b.States[walk].FromDirection;
		if (parent_cell < 0) break;
		run += (b.Cell_Center(here) - b.Cell_Center(parent_cell)).Length();
		const int parent_state = State_Index(b,parent_cell,parent_direction);
		const bool parent_is_start = (b.States[parent_state].FromCell < 0);
		if (!parent_is_start && (parent_direction != incoming)) return true;		// not the first turn: turn spacing's
		walk = parent_state;
	}

	const float straight = Min2(b.P->Constants.W,0.5f * b.A);
	return (run >= Transition_Room(min_turn_radius,b.DepartureAxis,incoming,Heading_Change(incoming,direction),straight));
}


/*
**	One A* leg.  `entry_direction` is the heading the road arrives on from the previous leg, so the turn
**	rules apply across a leg boundary: a leg that started with no heading could turn as sharply as it liked
**	immediately after a frozen waypoint, and a waypoint is exactly where the corners are.
*/
//	The census: one rule refused a move into `cell`.
static inline void Note_Block(RouteBuilder & b,int cell,HybridRouteBlock block)
{
	if (b.P->CollectCensus && (cell >= 0) && (cell < b.BlockMask.Count())) b.BlockMask[cell] |= (1u << (unsigned int)block);
}


//	The blocks a cell's own refusal bits stand for.
static unsigned int Refusal_Blocks(unsigned int refusal)
{
	unsigned int blocks = 0;
	if (refusal & HYBRID_REFUSE_UNPLACEABLE)	blocks |= (1u << HYBRID_BLOCK_WORLD_BOUNDS);
	if (refusal & HYBRID_REFUSE_BASE_DISC)		blocks |= (1u << HYBRID_BLOCK_BASE);
	if (refusal & HYBRID_REFUSE_TIBERIUM)		blocks |= (1u << HYBRID_BLOCK_TIBERIUM);
	if (refusal & HYBRID_REFUSE_WATER)			blocks |= (1u << HYBRID_BLOCK_WATER);
	if (refusal & HYBRID_REFUSE_WALL)			blocks |= (1u << HYBRID_BLOCK_WALL);
	if (refusal & HYBRID_REFUSE_CHANNEL)		blocks |= (1u << HYBRID_BLOCK_RIVER);
	if (refusal & HYBRID_REFUSE_FOOTPRINT)		blocks |= (1u << HYBRID_BLOCK_EXISTING_ROAD);
	return blocks;
}


static bool Search_Leg(RouteBuilder & b,int start,int goal,int entry_direction,
								DynamicVectorClass<int> * chain,HybridRouteResult * result)
{
	b.StartCell	= start;
	b.GoalCell	= goal;
	b.Generation++;					// a new leg: every cached answer belongs to the previous one

	const HybridConstants & k = b.P->Constants;
	const HybridRouteCostWeights weights = Hybrid_Route_Cost_Weights(b.CurrentClass);
	const float max_grade = Max2(Hybrid_Road_Class_Spec(b.CurrentClass).MaxGradeDegrees * PI_OVER_180,k.Q);
	const float min_turn_radius = (b.StructuralLeg || b.AlignedLeg) ? 0.0f
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
	if (b.P->CollectCensus) {
		if (b.BlockMask.Count() != b.CellCount) {
			b.BlockMask.Resize(b.CellCount);
			while (b.BlockMask.Count() < b.CellCount) b.BlockMask.Add(0);
		}
		for (int i = 0; i < b.CellCount; ++i) b.BlockMask[i] = 0;
		for (int i = 0; i < HYBRID_BLOCK_COUNT; ++i) b.LastFrontier[i] = 0;
	}


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
			if (b.P->CollectCensus) b.LastFrontier[HYBRID_BLOCK_BUDGET]++;
			return false;
		}
		if ((b.P->AStarExpansionsPerAttempt > 0) && (b.Out->TotalExpansions > b.P->AStarExpansionsPerAttempt)) {
			Fail(result,HYBRID_ROUTE_EXPANSION_BUDGET,"the per-attempt A* expansion ceiling was reached");
			if (b.P->CollectCensus) b.LastFrontier[HYBRID_BLOCK_BUDGET]++;
			return false;
		}

		const int x = b.Cell_X(cell);
		const int y = b.Cell_Y(cell);

		for (int direction = 0; direction < DIRECTION_COUNT; ++direction) {

			const int nx = x + NEIGHBOR_DX[direction];
			const int ny = y + NEIGHBOR_DY[direction];
			if (!b.Valid_Cell(nx,ny)) continue;

			const int neighbor = b.Cell_Index(nx,ny);

			//	The first move off a crossing's last approach point leaves along the crossing's own axis.
			if ((state == start_state) && (b.DepartureAxis.Length() > 0.0f) &&
				 !Direction_Fits_Axis(direction,b.DepartureAxis)) {
				Note_Block(b,neighbor,HYBRID_BLOCK_TURN);
				continue;
			}

			const unsigned int refusal = Cached_Refusal(b,neighbor);
			if (refusal != 0) {
				if (b.P->CollectCensus) b.BlockMask[neighbor] |= Refusal_Blocks(refusal);
				continue;
			}
			if (Move_Crosses_Channel(b,cell,direction,neighbor)) {
				Note_Block(b,neighbor,HYBRID_BLOCK_RIVER);
				continue;
			}

			const Vector2 neighbor_centre = b.Cell_Center(neighbor);

			//	A move onto the goal on a heading, or a straight, the crossing ahead cannot be turned onto is not an arrival.
			if ((neighbor == goal) && !Arrival_Fits_Crossing(b,state,cell,goal,direction,min_turn_radius)) {
				Note_Block(b,neighbor,HYBRID_BLOCK_TURN);
				continue;
			}

			float grade = 0.0f;
			float d = 0.0f;
			if (!Move_Grade(b,cell,neighbor,&grade,&d)) {
				Note_Block(b,neighbor,HYBRID_BLOCK_GRADE);
				continue;
			}

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
			if (!structural && (grade > max_grade)) {
				Note_Block(b,neighbor,HYBRID_BLOCK_GRADE);
				continue;
			}
			if (!structural && !Cross_Slope_Is_Legal(b,cell,neighbor)) {
				Note_Block(b,neighbor,HYBRID_BLOCK_CROSS_SLOPE);
				continue;
			}

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
				if ((min_turn_radius > b.A) && ((steps > 1) || (steps < -1))) {
					Note_Block(b,neighbor,HYBRID_BLOCK_TURN);
					continue;
				}

				if (!Turn_Spacing_Is_Legal(b,state,incoming,min_turn_radius,
													Turn_Sign(incoming,direction))) {
					Note_Block(b,neighbor,HYBRID_BLOCK_TURN);
					continue;
				}
				if (!Departure_Fits_Crossing(b,state,incoming,direction,min_turn_radius)) {
					Note_Block(b,neighbor,HYBRID_BLOCK_TURN);
					continue;
				}
			}
			/*
			**	The search holds itself to a margin above the class minimum, because it measures the *chain*
			**	while the check measures the *spline* through it: a centripetal Catmull-Rom sits inside its
			**	control polygon and comes out a few percent tighter than the cell centres suggest.  The check
			**	stays exact; only the search is conservative, so it never builds a bend the check will reject.
			*/
			if (!Chain_Curvature_Is_Legal(b,state,neighbor,CURVATURE_SEARCH_MARGIN * min_turn_radius)) {
				Note_Block(b,neighbor,HYBRID_BLOCK_TURN);
				continue;
			}

			//	The two collinear rules: running along another heavy family, or along any road.
			if (b.CurrentFamily >= 0) {
				if (Move_Runs_Along_Existing_Road(b,state,cell,neighbor,true)) {
					Note_Block(b,neighbor,HYBRID_BLOCK_EXISTING_ROAD);
					continue;
				}
			}
			if (b.CurrentIsOptional) {
				if (Move_Runs_Along_Existing_Road(b,state,cell,neighbor,false)) {
					Note_Block(b,neighbor,HYBRID_BLOCK_EXISTING_ROAD);
					continue;
				}
			}
			if (Move_Runs_Parallel(b,state,cell,neighbor)) {
				Note_Block(b,neighbor,HYBRID_BLOCK_EXISTING_ROAD);
				continue;
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
			if (excess > 0.0f) {
				Note_Block(b,neighbor,HYBRID_BLOCK_ENVELOPE);
				continue;
			}

			float cost = d;
			cost += d * weights.Ks * (float)pow((double)(grade / max_grade),4.0);
			cost += b.A * weights.Kt * (float)pow((double)(heading / 3.14159265358979323846f),2.0);
			cost += b.A * weights.Kc * (excess * excess);

			//	Entering a window this leg is not obliged to use is penalized, never forbidden.
			const unsigned int window = Cached_Window(b,neighbor);
			if ((window != 0) && (window != b.OwnWindow)) cost += window_penalty;

			//	No reuse discount (Q-025): an independent road may not take another's corridor for a lower cost.

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
			if (b.P->CollectCensus) b.LastFrontier[HYBRID_BLOCK_BUDGET]++;
		}

		//	The census: the frontier is every refused cell no move ever reached, counted by each rule that refused it.
		if (b.P->CollectCensus) {
			for (int c = 0; c < b.CellCount; ++c) {
				if (b.BlockMask[c] == 0) continue;
				bool reached = false;
				for (int direction = 0; (direction < STATE_COUNT) && !reached; ++direction) {
					if (b.States[State_Index(b,c,direction)].G >= 0.0f) reached = true;
				}
				if (reached) continue;
				for (int block = 0; block < HYBRID_BLOCK_COUNT; ++block) {
					if (b.BlockMask[c] & (1u << (unsigned int)block)) b.LastFrontier[block]++;
				}
			}
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
//	What the corner construction refused, for the census: the vertices on either side of the failure.
struct CornerFailure
{
	CornerFailure(void) : Kind(0),Where(0.0f,0.0f),LowFrozen(false),HighFrozen(false),LowLead(false),HighLead(false),
								 LowLeadSide(0),HighLeadSide(0),LowWindow(0),HighWindow(0),LowEnd(false),HighEnd(false),
								 FrozenPoint(0.0f,0.0f),FrozenAxis(0.0f,0.0f),Before(0.0f,0.0f),After(0.0f,0.0f),BeforeWindow(0),AfterWindow(0) { }

	int				Kind;					// 1 a frozen point left at a corner, 2 two fixed vertices crowding a leg
	Vector2			Where;
	bool				LowFrozen;
	bool				HighFrozen;
	bool				LowLead;
	bool				HighLead;
	int				LowLeadSide;
	int				HighLeadSide;
	unsigned int	LowWindow;
	unsigned int	HighWindow;
	bool				LowEnd;				// the road's own first vertex
	bool				HighEnd;				// ...or its last
	Vector2			FrozenPoint;		// Kind 1: the frozen vertex and its axis, and its two neighbours
	Vector2			FrozenAxis;
	Vector2			Before;
	Vector2			After;
	unsigned int	BeforeWindow;		// Kind 1: the crossings the two neighbours belong to, 0 for none
	unsigned int	AfterWindow;
};

static CornerFailure * s_corner_failure = nullptr;		// set only while the census asks


struct RouteVertex
{
	RouteVertex(void) : P(0.0f,0.0f),Axis(0.0f,0.0f),Frozen(false),Pinned(false),Guide(false),Lead(false),Keep(0.0f),
								Anchor(0.0f,0.0f),LeadSide(0),WindowId(0),ChainIndex(-1),StructuralAhead(false) { }

	//	DynamicVectorClass<T> requires == and !=.  The position is this vertex's identity.
	bool	operator == (const RouteVertex & that) const	{ return P == that.P; }
	bool	operator != (const RouteVertex & that) const	{ return !(*this == that); }

	Vector2			P;
	Vector2			Axis;					// for a frozen vertex, the direction the road must run through it
	bool				Frozen;				// an earlier phase fixed this position: never dropped, never a corner
	bool				Pinned;				// a corner whose position is fixed: never dropped, may carry an arc
	bool				Guide;				// pinned at an ordinary guide: yields to a structure's lead, to nothing else
	bool				Lead;					// a pinned corner laid to keep a frozen point straight (Insert_Frozen_Leads)
	float				Keep;					// frozen only: straight to keep either side when more than the default
	Vector2			Anchor;				// lead only: the frozen point it keeps straight, along `Axis`
	int				LeadSide;			// lead only: +1 laid before its frozen point, -1 after it
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
	RouteCrossing(void) : WindowId(0),FirstIndex(0),LastIndex(0),Ingress(0.0f,0.0f),Egress(0.0f,0.0f),Aligned(false) { }

	//	DynamicVectorClass<T> requires == and !=.  The window is this record's identity within a route.
	bool	operator == (const RouteCrossing & that) const	{ return WindowId == that.WindowId; }
	bool	operator != (const RouteCrossing & that) const	{ return !(*this == that); }

	unsigned int	WindowId;
	int				FirstIndex;			// the chain cell the near bank's approach point stands in
	int				LastIndex;			// the chain cell the far bank's approach point stands in
	Vector2			Ingress;
	Vector2			Egress;

	/*
	**	An interchange's aligned highway straight (12.6.9 item 1), not a crossing: the chain cells between are
	**	replaced by the straight a_I -> X -> b_I exactly as a crossing's are, but it is open road -- no structural
	**	span, every sample validated.  `WindowId` then holds ALIGNED_ID_BASE | interchange StableId, which no
	**	Phase-6 window has, so the two frozen ends form one straight run in the corner construction.
	*/
	bool				Aligned;
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
	const float widest = 150.0f * (3.14159265358979323846f / 180.0f);		// the sharpest turn a lead can take

	bool again = true;
	while (again) {
		again = false;
		for (int v = 1; v < (vertices->Count() - 1); ++v) {

			//	At a frozen point the limit is the sharpest turn its lead can be laid for, not a full reversal.
			const float limit = ((*vertices)[v].Frozen && !(*vertices)[v].Pinned) ? widest : reversal;
			if (Vertex_Turn((*vertices)[v - 1].P,(*vertices)[v].P,(*vertices)[v + 1].P) < limit) continue;

			/*
			**	A reversal at a corridor vertex drops that vertex.  A reversal *at a frozen point* drops the corridor
			**	vertex beside it that makes the road double back: a chain cell standing inside a crossing's approach
			**	corridor, nearer the water than the frozen approach point, would otherwise have the road arrive at
			**	the point heading away from the crossing and turn straight round onto it -- a U-turn no lead can
			**	straighten.  Two frozen points reversing on each other are left as the conflict they are.
			*/
			int drop = v;
			if ((*vertices)[v].Frozen || (*vertices)[v].Pinned) {
				const bool behind_free = ((v - 1) > 0) && !(*vertices)[v - 1].Frozen && !(*vertices)[v - 1].Pinned;
				const bool ahead_free = ((v + 1) < (vertices->Count() - 1)) && !(*vertices)[v + 1].Frozen &&
												!(*vertices)[v + 1].Pinned;
				if (behind_free)		drop = v - 1;
				else if (ahead_free)	drop = v + 1;
				else continue;
			}

			DynamicVectorClass<RouteVertex> kept;
			for (int k = 0; k < vertices->Count(); ++k) {
				if (k == drop) continue;
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

			/*
			**	Did it cross the window's obstruction?  The moves themselves are asked, as the search asked them:
			**	a move of this run that meets a river inside this window's footprint is a crossing of it.
			**	Comparing the run's two end cells about the window's centre missed a crossing the chain made near
			**	one end of the footprint -- the search had allowed it, nothing recorded it, and the road crossed
			**	water with no structure.  A road along the rim, meeting no river, crosses nothing.
			*/
			bool crossed = false;
			for (int c = first_vertex; (c < last_vertex) && !crossed; ++c) {
				Vector2 wet(0.0f,0.0f);
				crossed = Segment_Meets_River(b,b.Cell_Center(chain[c]),b.Cell_Center(chain[c + 1]),&wet) &&
							 Window_Covers(b,*window,wet);
			}
			if (!crossed) {
				const Vector2 normal(-window->Tangent.Y,window->Tangent.X);
				const Vector2 entered = b.Cell_Center(chain[first_vertex]) - window->Center;
				const Vector2 left = b.Cell_Center(chain[last_vertex]) - window->Center;
				const float across_in = (entered.X * normal.X) + (entered.Y * normal.Y);
				const float across_out = (left.X * normal.X) + (left.Y * normal.Y);
				crossed = ((across_in < 0.0f) && (across_out > 0.0f)) || ((across_in > 0.0f) && (across_out < 0.0f));
			}

			if (crossed) {

				RouteCrossing record;
				record.WindowId	= id;
				record.FirstIndex	= first_vertex;
				record.LastIndex	= last_vertex;

				//	The ingress is the approach point the run travels away from, along the window's own axis.
				const Vector2 along = window->ApproachB - window->ApproachA;
				const Vector2 travel = b.Cell_Center(chain[last_vertex]) - b.Cell_Center(chain[first_vertex]);
				const bool from_a = (((travel.X * along.X) + (travel.Y * along.Y)) >= 0.0f);
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
											const DynamicVectorClass<RouteVertex> & frozen_points,
											const DynamicVectorClass<int> & guide_cells,
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
			in_vertex.StructuralAhead= !record.Aligned;
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
		bool guide = false;
		for (int f = 0; (f < guide_cells.Count()) && !guide && !frozen; ++f) {
			if (guide_cells[f] == cell) guide = true;
		}
		for (int e = 0; (e < inserted.Count()) && !keep; ++e) {
			if (inserted[e] == cell) keep = true;
		}

		//	A structural point on open ground -- an overpass obligation -- stands at its own position on its axis.
		int structural_point = -1;
		for (int f = 0; (f < frozen_points.Count()) && (structural_point < 0); ++f) {
			if (b.Cell_Of(frozen_points[f].P) == cell) structural_point = f;
		}

		if (structural_point >= 0) {
			RouteVertex vertex = frozen_points[structural_point];
			vertex.ChainIndex = i;
			out->Add(vertex);
		} else if (keep || frozen || guide) {

			/*
			**	An ordinary guide's corner stands on the guide's own frozen position, which Phases 3 and 9 made legal
			**	ground, rather than on its cell centre, which can be up to half a cell diagonal away and inside the
			**	field reserve beside it.  It is held like a pinned corner -- an ordinary corridor corner crowding it
			**	gives way, it does not -- except to a structure's lead: the guide is the search's anchor, and the
			**	centerline is not bound to it, so where a crossing's straight approach needs the room, the guide's
			**	corner is dropped and the road takes the straight line.
			*/
			RouteVertex vertex;
			vertex.P				= b.Cell_Center(cell);
			for (int j = 0; (j < b.GuideCells.Count()) && guide; ++j) {
				if (b.GuideCells[j] == cell) vertex.P = b.GuidePoints[j];
			}
			vertex.Frozen		= frozen;
			vertex.Pinned		= guide;
			vertex.Guide		= guide;
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
**	The lead before frozen point `point`, which the road has to pass heading along `axis`, coming from `prior`.
**
**	A lead is the corner that turns the road onto a frozen point's axis *before* it reaches the point, so the
**	point itself is straight-through.  It stands `lead = R tan(theta/2) + straight` back along the axis, where
**	`theta` is the turn the road actually makes at the lead -- which moves as the lead does, so it is settled
**	by a few fixed-point steps -- and `straight` is the straight the road keeps between the end of that arc
**	and the frozen point.  The lead is refused when it would stand behind `prior`, when the turn at it is past
**	150 degrees, or when its own arc does not fit the lead's length: then there is no room on this side.
**
**	Returns true with `needed_out` false when the road already arrives along the axis.
*/
static bool Lead_Before(const Vector2 & prior,const Vector2 & point,const Vector2 & axis,float radius,
								float straight,bool * needed_out,Vector2 * gate_out)
{
	const float widest = 150.0f * PI_OVER_180;
	*needed_out = false;

	const Vector2 incoming = Unit(point - prior);
	if (incoming.Length() <= 0.0f) return false;

	float dot = Clamp2((incoming.X * axis.X) + (incoming.Y * axis.Y),-1.0f,1.0f);
	const float turn = (float)::acos((double)dot);
	if (turn <= HYBRID_QUANTUM_ANGLE) return true;
	if (turn >= widest) return false;

	float lead = (radius * (float)::tan((double)(0.5f * turn))) + straight;
	Vector2 gate(point.X - (axis.X * lead),point.Y - (axis.Y * lead));
	for (int step = 0; step < 8; ++step) {
		const float at_gate = Vertex_Turn(prior,gate,point);
		if (at_gate >= widest) return false;
		const float needed = (radius * (float)::tan((double)(0.5f * at_gate))) + straight;
		if (needed <= lead) break;
		lead = needed;
		gate.Set(point.X - (axis.X * lead),point.Y - (axis.Y * lead));
	}

	const Vector2 reach = gate - prior;
	const Vector2 straight_on = point - prior;
	if (((reach.X * straight_on.X) + (reach.Y * straight_on.Y)) <= 0.0f) return false;
	if (reach.Length() <= HYBRID_QUANTUM_POSITION) return false;

	const float at_gate = Vertex_Turn(prior,gate,point);
	if (at_gate >= widest) return false;
	if (((radius * (float)::tan((double)(0.5f * at_gate))) + straight) > (lead + HYBRID_QUANTUM_POSITION)) return false;

	*needed_out = true;
	*gate_out = gate;
	return true;
}


/*
**	Give every frozen interior point a straight run through it, so the arcs sit outside it.
**
**	An arc that rounds a corner does not pass through the corner, and a frozen point has to be interpolated
**	exactly -- so a frozen point may never *be* a corner.  A lead-in and a lead-out along the point's own
**	axis turn the corner at the frozen point into corners on open road, with the frozen point straight
**	between them.  For a crossing that axis is the line between its two frozen approach points, which is also
**	what makes the crossing itself straight; for any other frozen point it is the direction the corridor
**	already passes through it.
**
**	Revised D-009 (Q-022): Phase 10 owns this geometry completely.  A lead is placed wherever the road can
**	make the turn: a corridor vertex standing so close that the lead's arc would not fit is dropped, because
**	it is corridor and the road may simply run straighter there.  Only a route endpoint, another frozen point
**	or another pinned vertex can stop it, and then no lead is laid -- the feasibility pass refuses the frozen
**	corner that remains rather than leaving it for a later phase to cut into a crossing's straight approach.
**	Every lead is pinned: it may carry an arc, but it is never dropped.
**
**	`straight` is the straight kept between each lead's arc and its frozen point.
*/
static void Insert_Frozen_Leads(float radius,float straight,DynamicVectorClass<RouteVertex> * vertices)
{
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
			//	Only one crossing's own pair of approach points is one straight run; any other frozen point is its own.
			if (((*vertices)[v].WindowId == 0) || ((*vertices)[run_end + 1].WindowId != (*vertices)[v].WindowId)) break;
			run_end++;
		}

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
		if (axis.Length() <= 0.0f) axis = Unit(run_start - built[built.Count() - 1].P);
		if (axis.Length() <= 0.0f) {
			for (int k = v; k <= run_end; ++k) built.Add((*vertices)[k]);
			v = run_end + 1;
			continue;
		}

		//	The lead-in, dropping any corridor vertex behind it that leaves its arc no room.
		for (;;) {
			const Vector2 prior = built[built.Count() - 1].P;
			bool needed = false;
			Vector2 gate(0.0f,0.0f);
			if (Lead_Before(prior,run_start,axis,radius,Max2(straight,(*vertices)[v].Keep),&needed,&gate)) {
				if (needed) {
					RouteVertex lead;
					lead.P				= gate;
					lead.Pinned			= true;
					lead.Lead			= true;
					lead.Anchor			= run_start;
					lead.Axis			= axis;
					lead.LeadSide		= 1;
					lead.Keep			= Max2(straight,(*vertices)[v].Keep);
					lead.ChainIndex	= (*vertices)[v].ChainIndex;
					built.Add(lead);
				}
				break;
			}
			const RouteVertex & crowding = built[built.Count() - 1];
			if ((built.Count() <= 1) || crowding.Frozen || (crowding.Pinned && !crowding.Guide)) break;
			built.Delete(built.Count() - 1);
		}

		for (int k = v; k <= run_end; ++k) built.Add((*vertices)[k]);

		/*
		**	The lead-out, the same construction on the road travelled backwards: corridor vertices ahead that
		**	leave its arc no room are skipped, never the frozen point's own straight.
		*/
		int next = run_end + 1;
		const Vector2 back(-axis.X,-axis.Y);
		for (;;) {
			const Vector2 ahead = (*vertices)[next].P;
			bool needed = false;
			Vector2 gate(0.0f,0.0f);
			if (Lead_Before(ahead,run_finish,back,radius,Max2(straight,(*vertices)[run_end].Keep),&needed,&gate)) {
				if (needed) {
					RouteVertex lead;
					lead.P				= gate;
					lead.Pinned			= true;
					lead.Lead			= true;
					lead.Anchor			= run_finish;
					lead.Axis			= axis;
					lead.LeadSide		= -1;
					lead.Keep			= Max2(straight,(*vertices)[run_end].Keep);
					lead.ChainIndex	= (*vertices)[run_end].ChainIndex;
					built.Add(lead);
				}
				break;
			}
			const RouteVertex & crowding = (*vertices)[next];
			if ((next >= (vertices->Count() - 1)) || crowding.Frozen || (crowding.Pinned && !crowding.Guide)) break;
			next++;
		}

		v = next;
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
**	order.  The only tolerance is the position quantum `Q`, which is the rounding a vertex picks up when it
**	is written down.
**
**	A reversal cannot be filleted at any radius, and frozen or pinned corners crowding each other cannot be
**	resolved at all: both are reported as what they are, which points at the phase that froze them.  So is a
**	frozen point still standing at a corner once everything fits (revised D-009): its leads could not be laid,
**	and the road would drive the frozen point as a kink.
**
**	A dropped vertex beside a lead changes the turn the lead was sized for, so its length no longer matches
**	the arc it carries; a dropped vertex beside a frozen point changes whether that point needs a lead at all.
**	`leads_stale_out` then reports it and the caller lays the leads again.
*/
static bool Fillet_Is_Feasible(float radius,float q,float straight,DynamicVectorClass<RouteVertex> * vertices,
										 const char ** reason_out,bool * leads_stale_out,Vector2 * where_out = nullptr)
{
	const float quantum = Max2(q,1.0e-3f);
	const float reversal = 170.0f * PI_OVER_180;
	if (leads_stale_out != nullptr) *leads_stale_out = false;

	for (int pass = 0; pass <= vertices->Count(); ++pass) {

		if (vertices->Count() < 2) {
			if (reason_out != nullptr) *reason_out = "corridor collapsed to a point";
			if ((where_out != nullptr) && (vertices->Count() > 0)) *where_out = (*vertices)[0].P;
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
		int reversed_at = 0;
		for (int v = 0; v < vertices->Count(); ++v) {

			//	An end, or a point that must be interpolated exactly: no arc is placed there at all.
			if ((v == 0) || (v == (vertices->Count() - 1)) || (*vertices)[v].Frozen) {
				tangent.Add(0.0f);
				continue;
			}
			const float turn = Vertex_Turn((*vertices)[v - 1].P,(*vertices)[v].P,(*vertices)[v + 1].P);
			if ((turn >= reversal) && !reversed) {
				reversed = true;
				reversed_at = v;
			}
			if (turn <= quantum) {
				tangent.Add(0.0f);
				continue;
			}
			tangent.Add(radius * (float)::tan((double)(0.5f * turn)));
		}

		if (reversed) {
			if (reason_out != nullptr) *reason_out = "the corridor reverses on itself";
			if (where_out != nullptr) *where_out = (*vertices)[reversed_at].P;
			return false;
		}

		/*
		**	The first leg that cannot host the two arcs sharing it -- and, beside an interior frozen point, the
		**	straight kept between that point and the nearest arc, so the road is measurably on the frozen axis
		**	before and after the point rather than leaving an arc exactly on it.
		*/
		const int last_vertex = vertices->Count() - 1;
		int crowded = -1;
		for (int leg = 0; leg < last_vertex; ++leg) {
			const float length = ((*vertices)[leg + 1].P - (*vertices)[leg].P).Length();
			const RouteVertex & low = (*vertices)[leg];
			const RouteVertex & high = (*vertices)[leg + 1];
			const bool low_frozen = (leg > 0) && low.Frozen;
			const bool high_frozen = ((leg + 1) < last_vertex) && high.Frozen;
			const bool same_run = low_frozen && high_frozen && (low.WindowId != 0) && (low.WindowId == high.WindowId);
			float keep = 0.0f;
			if (!same_run) {
				if (low_frozen)	keep += Max2(straight,low.Keep);
				if (high_frozen)	keep += Max2(straight,high.Keep);
			}
			if ((tangent[leg] + tangent[leg + 1] + keep) <= (length + q)) continue;
			crowded = leg;
			break;
		}

		if (crowded < 0) {
			//	Every arc fits.  A frozen point left at a corner is a turn no arc rounds.
			for (int v = 1; v < (vertices->Count() - 1); ++v) {
				if (!(*vertices)[v].Frozen) continue;
				const float shorter = Min2(((*vertices)[v].P - (*vertices)[v - 1].P).Length(),
													((*vertices)[v + 1].P - (*vertices)[v].P).Length());
				const float rounding = (2.0f * q) / Max2(shorter,q);
				if (Vertex_Turn((*vertices)[v - 1].P,(*vertices)[v].P,(*vertices)[v + 1].P) <=
					 (HYBRID_QUANTUM_ANGLE + rounding)) continue;
				if (reason_out != nullptr) *reason_out = "a frozen point is left at a corner its leads could not straighten";
				if (where_out != nullptr) *where_out = (*vertices)[v].P;
				if (s_corner_failure != nullptr) {
					s_corner_failure->Kind			= 1;
					s_corner_failure->Where			= (*vertices)[v].P;
					s_corner_failure->FrozenPoint	= (*vertices)[v].P;
					s_corner_failure->FrozenAxis	= (*vertices)[v].Axis;
					s_corner_failure->Before		= (*vertices)[v - 1].P;
					s_corner_failure->After			= (*vertices)[v + 1].P;
					s_corner_failure->LowWindow	= (*vertices)[v].WindowId;
					s_corner_failure->BeforeWindow= (*vertices)[v - 1].Frozen ? (*vertices)[v - 1].WindowId : 0;
					s_corner_failure->AfterWindow	= (*vertices)[v + 1].Frozen ? (*vertices)[v + 1].WindowId : 0;
				}
				return false;
			}
			return true;
		}

		/*
		**	Drop the shallower of the two corners crowding this leg.  A frozen or pinned one never moves, and a
		**	leg between two of them has no give at all -- except a guide's corner beside a structure's lead, which
		**	gives way to it.
		*/
		const RouteVertex & low_vertex = (*vertices)[crowded];
		const RouteVertex & high_vertex = (*vertices)[crowded + 1];
		const bool low_movable = !low_vertex.Frozen && (!low_vertex.Pinned || (low_vertex.Guide && high_vertex.Lead)) &&
										 (crowded > 0);
		const bool high_movable = !high_vertex.Frozen && (!high_vertex.Pinned || (high_vertex.Guide && low_vertex.Lead)) &&
										  ((crowded + 1) < (vertices->Count() - 1));

		int drop = -1;
		if (low_movable && high_movable) {
			drop = (tangent[crowded] <= tangent[crowded + 1]) ? crowded : (crowded + 1);
		} else if (low_movable) {
			drop = crowded;
		} else if (high_movable) {
			drop = crowded + 1;
		}

		if (where_out != nullptr) *where_out = ((*vertices)[crowded].P + (*vertices)[crowded + 1].P) * 0.5f;
		if ((drop < 0) && (s_corner_failure != nullptr)) {
			const RouteVertex & low = (*vertices)[crowded];
			const RouteVertex & high = (*vertices)[crowded + 1];
			s_corner_failure->Kind			= 2;
			s_corner_failure->Where			= (low.P + high.P) * 0.5f;
			s_corner_failure->LowFrozen	= low.Frozen && !low.Lead;
			s_corner_failure->HighFrozen	= high.Frozen && !high.Lead;
			s_corner_failure->LowLead		= low.Lead;
			s_corner_failure->HighLead		= high.Lead;
			s_corner_failure->LowLeadSide	= low.LeadSide;
			s_corner_failure->HighLeadSide= high.LeadSide;
			s_corner_failure->LowWindow	= low.WindowId;
			s_corner_failure->HighWindow	= high.WindowId;
			s_corner_failure->LowEnd		= (crowded == 0);
			s_corner_failure->HighEnd		= ((crowded + 1) == (vertices->Count() - 1));
		}
		if (drop < 0) {
			if (reason_out != nullptr) {
				*reason_out = "frozen points closer than the class turn radius allows";
			}
			return false;
		}

		/*
		**	A frozen point's lead was decided against the vertex beside it -- and where that vertex already lay on
		**	the frozen axis, no lead was laid at all.  Dropping that vertex leaves the frozen point facing a corner
		**	further on that nothing straightens, so it re-lays the leads exactly as dropping a vertex beside a lead
		**	does.  Without it a crossing whose chain ran a cell straight on before turning was refused as "a frozen
		**	point left at a corner" when the turn was dropped, with room for the lead in plain sight.
		*/
		const bool beside_lead = ((drop > 0) && (*vertices)[drop - 1].Lead) ||
										 (((drop + 1) < vertices->Count()) && (*vertices)[drop + 1].Lead) ||
										 ((drop > 1) && (*vertices)[drop - 1].Frozen) ||
										 (((drop + 2) < vertices->Count()) && (*vertices)[drop + 1].Frozen);

		DynamicVectorClass<RouteVertex> kept;
		for (int v = 0; v < vertices->Count(); ++v) {
			if (v == drop) continue;
			kept.Add((*vertices)[v]);
		}
		vertices->Delete_All();
		for (int v = 0; v < kept.Count(); ++v) vertices->Add(kept[v]);

		if (beside_lead && (leads_stale_out != nullptr)) {
			*leads_stale_out = true;
			if (reason_out != nullptr) *reason_out = "a lead was sized for a corner that has been dropped";
			return false;
		}
	}

	if (reason_out != nullptr) *reason_out = "the corridor could not be filleted";
	return false;
}


/*
**	Leads and corner drops, settled together: dropping a corridor vertex beside a lead changes the turn the
**	lead was sized for, so every lead is laid again on what is left.  Each round removes a vertex, so the
**	rounds are bounded by the polyline itself.
*/
/*
**	Each lead is laid against the vertex beside it as it stood when the lead was laid.  Between two frozen runs
**	that neighbour is then replaced by the *other* run's lead, so each is sized for a turn the road no longer
**	makes.  Every lead is laid again against its actual neighbour until none moves by more than `q` -- two facing
**	leads settle together -- in a bounded number of rounds.
*/
static void Relax_Leads(float radius,float q,DynamicVectorClass<RouteVertex> * vertices)
{
	for (int round = 0; round < 16; ++round) {

		bool moved = false;
		for (int i = 1; i < (vertices->Count() - 1); ++i) {

			RouteVertex & lead = (*vertices)[i];
			if (!lead.Lead || (lead.LeadSide == 0)) continue;

			const bool before = (lead.LeadSide > 0);
			const Vector2 neighbour = before ? (*vertices)[i - 1].P : (*vertices)[i + 1].P;
			const Vector2 axis = before ? lead.Axis : Vector2(-lead.Axis.X,-lead.Axis.Y);
			bool needed = false;
			Vector2 gate(0.0f,0.0f);
			if (!Lead_Before(neighbour,lead.Anchor,axis,radius,lead.Keep,&needed,&gate) || !needed) continue;
			if ((gate - lead.P).Length() <= q) continue;
			lead.P = gate;
			moved = true;
		}
		if (!moved) break;
	}
}


static bool Settle_Corners(float radius,float q,float straight,DynamicVectorClass<RouteVertex> * vertices,
									const char ** reason_out,Vector2 * where_out = nullptr)
{
	Insert_Frozen_Leads(radius,straight,vertices);
	Relax_Leads(radius,q,vertices);
	const int rounds = vertices->Count() + 1;
	for (int round = 0; round <= rounds; ++round) {
		bool stale = false;
		if (Fillet_Is_Feasible(radius,q,straight,vertices,reason_out,&stale,where_out)) return true;
		if (!stale) return false;

		DynamicVectorClass<RouteVertex> stripped;
		for (int v = 0; v < vertices->Count(); ++v) {
			if (!(*vertices)[v].Lead) stripped.Add((*vertices)[v]);
		}
		vertices->Delete_All();
		for (int v = 0; v < stripped.Count(); ++v) vertices->Add(stripped[v]);
		Insert_Frozen_Leads(radius,straight,vertices);
		Relax_Leads(radius,q,vertices);
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
/*
**	The first sample where this road has run alongside an independent road too closely for more than `6W`
**	(Q-025), or -1.  The junction exemption is measured along each road from the node the two share:
**	`JunctionReach` of this road's class on this road, and of the other's on the other.
*/
static int First_Parallel_Overlap(RouteBuilder & b,const DynamicVectorClass<Vector2> & samples)
{
	const HybridConstants & k = b.P->Constants;
	const float limit = COLLINEAR_RUN_W * k.W;
	const float parallel_cosine = (float)::cos((double)(PARALLEL_DEGREES * PI_OVER_180));
	const float own_reach = Hybrid_Junction_Reach(b.CurrentClass,k.W);
	if (samples.Count() < 2) return -1;

	//	This road's own stations, and its bounds.
	DynamicVectorClass<float> along;
	Vector2 low = samples[0];
	Vector2 high = samples[0];
	along.Add(0.0f);
	for (int i = 1; i < samples.Count(); ++i) {
		along.Add(along[i - 1] + (samples[i] - samples[i - 1]).Length());
		low.Set(Min2(low.X,samples[i].X),Min2(low.Y,samples[i].Y));
		high.Set(Max2(high.X,samples[i].X),Max2(high.Y,samples[i].Y));
	}

	for (int r = 0; r < b.Out->Routes.Count(); ++r) {

		const HybridRoute & route = b.Out->Routes[r];
		if (route.Points.Count() < 2) continue;
		const float clearance = Hybrid_Parallel_Clearance(b.CurrentClass,route.RoadClass,k.W,b.P->MinVehicleLaneClearance) +
										route.RampRoadbedExtra;
		if ((route.BoundsMin.X > (high.X + clearance)) || (route.BoundsMax.X < (low.X - clearance)) ||
			 (route.BoundsMin.Y > (high.Y + clearance)) || (route.BoundsMax.Y < (low.Y - clearance))) continue;
		const float other_reach = Hybrid_Junction_Reach(route.RoadClass,k.W);

		//	The shared nodes, as a station on each road.
		DynamicVectorClass<float> own_station;
		DynamicVectorClass<float> other_station;
		for (int s = 0; s < b.SharedRoute.Count(); ++s) {
			if (b.SharedRoute[s] != r) continue;
			int nearest = 0;
			float best = 3.4e38f;
			for (int i = 0; i < samples.Count(); ++i) {
				const float distance = (samples[i] - b.SharedPoint[s]).Length();
				if (distance >= best) continue;
				best = distance;
				nearest = i;
			}
			own_station.Add(along[nearest]);
			int other = 0;
			best = 3.4e38f;
			for (int i = 0; i < route.Points.Count(); ++i) {
				const float distance = (route.Points[i] - b.SharedPoint[s]).Length();
				if (distance >= best) continue;
				best = distance;
				other = i;
			}
			other_station.Add(route.Stations[other]);
		}

		float run = 0.0f;
		for (int i = 0; i < samples.Count(); ++i) {

			bool overlap = false;
			const Vector2 & point = samples[i];
			if ((point.X >= (route.BoundsMin.X - clearance)) && (point.X <= (route.BoundsMax.X + clearance)) &&
				 (point.Y >= (route.BoundsMin.Y - clearance)) && (point.Y <= (route.BoundsMax.Y + clearance))) {

				int at = -1;
				float t_at = 0.0f;
				float nearest = 3.4e38f;
				for (int q = 1; q < route.Points.Count(); ++q) {
					const Vector2 a = route.Points[q - 1];
					const Vector2 d = route.Points[q] - a;
					const float length2 = (d.X * d.X) + (d.Y * d.Y);
					float t_on = (length2 > 0.0f) ? ((((point.X - a.X) * d.X) + ((point.Y - a.Y) * d.Y)) / length2) : 0.0f;
					t_on = Clamp2(t_on,0.0f,1.0f);
					const float distance = (point - (a + (d * t_on))).Length();
					if (distance >= nearest) continue;
					nearest	= distance;
					at			= q;
					t_at		= t_on;
				}
				if ((at > 0) && (nearest < clearance)) {
					const Vector2 other = Unit(route.Points[at] - route.Points[at - 1]);
					const Vector2 own = Unit(samples[(i + 1) < samples.Count() ? (i + 1) : i] - samples[(i > 0) ? (i - 1) : i]);
					if ((float)fabs((double)((other.X * own.X) + (other.Y * own.Y))) >= parallel_cosine) {
						const float other_along = route.Stations[at - 1] + ((route.Stations[at] - route.Stations[at - 1]) * t_at);
						bool junction = false;
						for (int s = 0; (s < own_station.Count()) && !junction; ++s) {
							if ((float)fabs((double)(along[i] - own_station[s])) <= own_reach) junction = true;
							if ((float)fabs((double)(other_along - other_station[s])) <= other_reach) junction = true;
						}
						overlap = !junction;
					}
				}
			}

			if (!overlap) {
				run = 0.0f;
				continue;
			}
			if ((i > 0) && (run >= 0.0f)) run += (samples[i] - samples[i - 1]).Length();
			if (run > limit) {
				return i;
			}
		}
	}
	return -1;
}


static int First_Failing_Sample(RouteBuilder & b,const DynamicVectorClass<Vector2> & samples,
										  const DynamicVectorClass<HybridRouteStructuralSpan> & spans,
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
		**	Structural ground is *this road's own* crossing interval, by sample index: the straight between the
		**	two frozen approach points, which is the obstruction and the alignment reserve either side of it.
		**	The interaction phase compiles the deck and grades that interval; everything outside it,
		**	including the turn that lines the road up with the crossing, is open road and Phase 10's own
		**	(revised D-009, Q-022), so it is validated in full like any other.
		**
		**	It is never any ground inside some window's footprint.  Exempting the footprint let a road run along
		**	the window's own axis through the water, outside its structural interval, reach the far approach
		**	point and turn back across on the interval itself: two crossings of one stream, one of them with no
		**	structure, which Phase 11 could only compile by looping the road round on itself.
		*/
		bool structural_sample = false;
		for (int s = 0; (s < spans.Count()) && !structural_sample; ++s) {
			if ((i >= spans[s].FirstPoint) && (i <= spans[s].LastPoint)) structural_sample = true;
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
		**	A river is narrower than the sample spacing and than half a cell, so neither the point test nor the
		**	cell test above sees a centerline step across one: the step itself is judged, as the search's moves
		**	are.  A step may meet a river only inside this road's own accepted crossing stretch: within one of its
		**	structural spans, or stepping into or out of one, at a point that span's own window covers.  Anywhere
		**	else -- including another window's footprint -- the road crosses water with no structure.
		*/
		bool step_inside_span = false;
		const HybridCrossingWindow * step_window = nullptr;
		for (int s = 0; (s < spans.Count()) && (i > 0); ++s) {
			const bool here = (i >= spans[s].FirstPoint) && (i <= spans[s].LastPoint);
			const bool before = ((i - 1) >= spans[s].FirstPoint) && ((i - 1) <= spans[s].LastPoint);
			if (here && before) step_inside_span = true;
			if ((here || before) && (step_window == nullptr)) step_window = b.Hydrology->Find_Window(spans[s].WindowId);
		}
		Vector2 wet(0.0f,0.0f);
		if ((i > 0) && !step_inside_span && Segment_Meets_River(b,samples[i - 1],samples[i],&wet) &&
			 ((step_window == nullptr) || !Window_Covers(b,*step_window,wet))) {
			if (reason_out != nullptr) *reason_out = "hard-invalid ground";
			b.LastRadiusAt		= i;
			b.LastRadiusWindow= HYBRID_REFUSE_CHANNEL;
			b.LastClosed		= samples.Count();
			b.LastRadius		= (float)b.Cell_Of(samples[i]);
			b.LastExpansions	= 0;
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
		**	accepted window the road stands on a deck whose geometry the interaction phase
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

	/*
	**	Q-025's common rule for independently realized roads, on the centerline itself: for more than `6W` of
	**	continuous road, outside the junction-owned interval of a node the two roads share, this road may not run
	**	within 30 degrees of an already-routed road and closer than their ParallelCenterlineClearance -- their
	**	roadbeds and the lane clearance between them would overlap.  Parallel roads further apart are legal.
	*/
	{
		const int parallel_at = First_Parallel_Overlap(b,samples);
		if (parallel_at >= 0) {
			if (reason_out != nullptr) *reason_out = "an independent road's roadbed overlaps this one";
			b.LastRadiusAt		= parallel_at;
			b.LastRadiusWindow= 0;
			return parallel_at;
		}
	}

	/*
	**	"The local vertical feasibility intervals admit the class max grade", read as what it says: there is a
	**	profile through every sample's `RoadVerticalInterval` that keeps the class grade -- Section 12.1's own
	**	forward and backward interval propagation, over each stretch of open road between this road's
	**	structural intervals (whose deck elevations are the interaction phase's to compile).  Testing
	**	only neighbouring pairs let a long climb steeper than the class through, each step absorbed by the
	**	next sample's cut or fill until the allowance ran out, and the road then could not be graded at all.
	*/
	{
		const float g = (float)::tan((double)max_grade_radians);
		DynamicVectorClass<float> lo;
		DynamicVectorClass<float> hi;
		DynamicVectorClass<float> along;
		int first = 0;
		while (first < samples.Count()) {

			//	The next stretch of open road: from `first` up to the next structural sample, exclusive.
			bool in_span = false;
			for (int s = 0; (s < spans.Count()) && !in_span; ++s) {
				if ((first > spans[s].FirstPoint) && (first < spans[s].LastPoint)) in_span = true;
			}
			if (in_span) {
				first++;
				continue;
			}
			int last = first;
			while ((last + 1) < samples.Count()) {
				bool stop = false;
				for (int s = 0; (s < spans.Count()) && !stop; ++s) {
					if (((last + 1) > spans[s].FirstPoint) && ((last + 1) <= spans[s].LastPoint)) stop = true;
				}
				if (stop) break;
				last++;
			}

			lo.Delete_All();
			hi.Delete_All();
			along.Delete_All();
			float travelled = 0.0f;
			for (int i = first; i <= last; ++i) {
				float ground = 0.0f;
				if (!Height_At(b,samples[i],&ground)) return i;
				const HybridVerticalInterval interval = Hybrid_Road_Vertical_Interval(ground,b.P->MaxCutDepth,
																										  b.P->MaxFillHeight);
				if (i > first) travelled += (samples[i] - samples[i - 1]).Length();
				lo.Add(interval.Low);
				hi.Add(interval.High);
				along.Add(travelled);
			}
			for (int i = 1; i < lo.Count(); ++i) {
				const float ds = along[i] - along[i - 1];
				lo[i] = Max2(lo[i],lo[i - 1] - (g * ds));
				hi[i] = Min2(hi[i],hi[i - 1] + (g * ds));
				if (lo[i] > hi[i]) {
					if (reason_out != nullptr) *reason_out = "no profile through the vertical intervals keeps the class grade";
					b.LastRadiusAt		= first + i;
					b.LastRadiusWindow= 0;
					return first + i;
				}
			}
			first = last + 1;
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
static bool Emit_Centerline(float radius,float q,const DynamicVectorClass<RouteVertex> & vertices,
									 float spacing,DynamicVectorClass<Vector2> * points,
									 DynamicVectorClass<float> * stations,
									 DynamicVectorClass<HybridRouteStructuralSpan> * spans)
{
	points->Delete_All();
	stations->Delete_All();
	spans->Delete_All();
	if (vertices.Count() < 2) return false;

	const float quantum = Max2(q,1.0e-3f);
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
**	itself never moves.  Each answer is bounded by `retry.max_local_retries_per_stage`; the insertions share the
**	envelope's local-retry budget with Route_Road's natural-crossing alignment repair, so the caller passes what
**	is left of it as `insertion_limit` and `insertions_out` reports what was spent, success or failure.
*/
static bool Chain_To_Spline(RouteBuilder & b,const DynamicVectorClass<int> & chain,
									 const DynamicVectorClass<int> & mandatory,
									 const DynamicVectorClass<RouteVertex> & structural_points,
									 const DynamicVectorClass<int> & guide_cells,
									 const DynamicVectorClass<RouteCrossing> & crossings,const Vector2 & first_point,
									 const Vector2 & last_point,DynamicVectorClass<Vector2> * points,
									 DynamicVectorClass<float> * stations,
									 DynamicVectorClass<HybridRouteStructuralSpan> * structural,
									 int insertion_limit,int * insertions_out,const char ** reason_out,
									 Vector2 * failure_out = nullptr)
{
	const HybridConstants & k = b.P->Constants;
	const float spacing = Min2(k.W,0.5f * k.A);
	const int limit = (b.P->MaxLocalRetriesPerStage > 0) ? b.P->MaxLocalRetriesPerStage : 0;
	if (insertions_out != nullptr) *insertions_out = 0;
	const float min_radius = Hybrid_Road_Class_Spec(b.CurrentClass).MinTurnRadiusW * k.W;

	DynamicVectorClass<int> extra;							// step 5's inserted vertices, in path order
	int insertions		= 0;
	int radius_steps	= 0;

	for (;;) {

		const float build_radius = min_radius * (FILLET_RADIUS_MARGIN +
															  ((float)radius_steps * FILLET_RADIUS_STEP));

		DynamicVectorClass<RouteVertex> vertices;
		Build_Route_Vertices(b,chain,mandatory,structural_points,guide_cells,extra,crossings,first_point,last_point,
									&vertices);
		const char * geometry = "none";
		Vector2 where = (vertices.Count() > 0) ? vertices[0].P : first_point;
		if (!Settle_Corners(build_radius,k.Q,spacing,&vertices,&geometry,&where)) {
			if (reason_out != nullptr) *reason_out = geometry;
			if (failure_out != nullptr) *failure_out = where;
			b.LastRadius	= min_radius;
			b.LastRadiusAt	= vertices.Count();
			return false;
		}

		DynamicVectorClass<Vector2> samples;
		DynamicVectorClass<float> sample_stations;
		DynamicVectorClass<HybridRouteStructuralSpan> spans;
		if (!Emit_Centerline(build_radius,k.Q,vertices,spacing,&samples,&sample_stations,&spans)) {
			if (reason_out != nullptr) *reason_out = "degenerate centerline";
			if (failure_out != nullptr) *failure_out = first_point;
			b.LastRadius	= min_radius;
			b.LastRadiusAt	= vertices.Count();
			return false;
		}

		int failing = First_Failing_Sample(b,samples,spans,reason_out);

		/*
		**	An ordinary route-family waypoint is a strategic anchor, not a physical constraint: the search's chain
		**	passes through its cell (the plan's "every frozen waypoint's own cell is on the accepted chain", which
		**	the Phase-10 gate checks on the chain), and that is where the waypoint's job ends.  The class-radius
		**	centerline built over the chain may round it outside the cell; requiring the final curve back inside
		**	refused legal roads wherever the family turned sharply at a guide.  Structural points -- crossings,
		**	overpasses, the road's frozen ends -- stay exact: they are frozen vertices, never rounded.
		*/
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

		if (failure_out != nullptr) *failure_out = samples[failing];
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
		if (insertions >= insertion_limit) return false;

		extra.Add(best);
		insertions++;
		if (insertions_out != nullptr) *insertions_out = insertions;
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
	out->Add(Hybrid_Route_Edge_End(*b.Topology,*b.Out,edge,false));
	for (int w = 0; w < edge.Waypoints.Count(); ++w) out->Add(edge.Waypoints[w]);
	out->Add(Hybrid_Route_Edge_End(*b.Topology,*b.Out,edge,true));
}


/*
**	The straight a road keeps either side of an overpass obligation: the deck an overpass there spans along it,
**	whichever road rides over (Hybrid_Overpass_Half_Deck, the topology owner's measure, so Phase 9 reserved
**	at least this).  The deck is a straight bridge, so the road may not start turning under it.
*/
static float Overpass_Straight(const RouteBuilder & b,const HybridTopologyEdge & edge,const HybridTopologyNode & node,
										 const Vector2 & axis)
{
	for (int e = 0; e < b.Topology->Edges.Count(); ++e) {

		const HybridTopologyEdge & other = b.Topology->Edges[e];
		if (other.StableId == edge.StableId) continue;
		int at = -1;
		for (int w = 0; (w < other.Waypoints.Count()) && (at < 0); ++w) {
			if (other.Waypoints[w] == node.StableId) at = w;
		}
		if (at < 0) continue;

		const unsigned int before_id = (at > 0) ? other.Waypoints[at - 1] : other.NodeA;
		const unsigned int after_id = ((at + 1) < other.Waypoints.Count()) ? other.Waypoints[at + 1] : other.NodeB;
		const HybridTopologyNode * before = b.Topology->Find_Node(before_id);
		const HybridTopologyNode * after = b.Topology->Find_Node(after_id);
		if ((before == nullptr) || (after == nullptr)) return 0.0f;
		return Hybrid_Overpass_Half_Deck(axis,after->Position - before->Position,edge.RoadClass,other.RoadClass,
													b.P->Constants.W);
	}
	return 0.0f;
}


//	The census owner of a centerline refusal: the rule the reason names, or the refusal bits of the cell it hit.
static int Spline_Reason_Block(const char * reason,unsigned int refusal_bits)
{
	if (reason == nullptr) return HYBRID_BLOCK_NONE;
	if (::strcmp(reason,"hard-invalid ground") == 0) {
		const unsigned int blocks = Refusal_Blocks(refusal_bits);
		for (int i = 0; i < HYBRID_BLOCK_COUNT; ++i) {
			if (blocks & (1u << (unsigned int)i)) return i;
		}
		return HYBRID_BLOCK_NONE;
	}
	if (::strcmp(reason,"outside the envelope") == 0) return HYBRID_BLOCK_ENVELOPE;
	if (::strcmp(reason,"cross slope past the class maximum") == 0) return HYBRID_BLOCK_CROSS_SLOPE;
	if ((::strcmp(reason,"grade past the class maximum") == 0) ||
		 (::strcmp(reason,"no profile through the vertical intervals keeps the class grade") == 0)) return HYBRID_BLOCK_GRADE;
	return HYBRID_BLOCK_TURN;
}


//	The topology node's kind, or the node count when the node does not exist.
static bool Node_Is_Structural(const RouteBuilder & b,unsigned int node_id)
{
	const HybridTopologyNode * node = b.Topology->Find_Node(node_id);
	return (node != nullptr) && ((node->Kind == HYBRID_NODE_CROSSING_OBLIGATION) ||
										  (node->Kind == HYBRID_NODE_OVERPASS_OBLIGATION));
}


//	Does a route's topology edge carry `node` -- as an end or as a waypoint?
static bool Route_Carries_Node(const RouteBuilder & b,const HybridRoute & route,unsigned int node)
{
	const HybridTopologyEdge * edge = b.Topology->Find_Edge(route.EdgeId);
	if (edge == nullptr) return false;
	if ((edge->NodeA == node) || (edge->NodeB == node)) return true;
	for (int w = 0; w < edge->Waypoints.Count(); ++w) {
		if (edge->Waypoints[w] == node) return true;
	}
	return false;
}


/*
**	One physical road (Q-025): a single topology edge, or a heavy family's whole trunk -- its logical edges in
**	travel order, each ending where the next begins.
**
**	Each logical edge keeps its own Phase-10 ownership: its own envelope, starting at its own Phase-11 floor,
**	its own corridor and its own expansion budget.  What changes for a trunk is that the legs of all its
**	edges are one sequence -- the heading and straight a leg ends on are what the next leg starts with, across
**	an ordinary family waypoint as anywhere else -- and the centerline is built once, over the whole
**	concatenated chain, so a family waypoint is a cell the search passes through rather than the end of one road
**	and the start of another.  The class-radius centerline is not bound to that cell.
**
**	A failure is charged to the logical edge it happened on: a leg with no path to the edge that leg belongs
**	to, a centerline failure to the edge whose chain lies nearest the failing point.  That edge alone moves to
**	its next envelope and the road is laid again from it; the edges before it keep the chains they had, and
**	after its third envelope the edge -- and so the attempt -- fails.  Each logical edge's route record is then
**	the exact slice of the one centerline its interval covers.
*/
/*
**	The crossing the search made without being sent to it that a corner refusal belongs to, -1 for none: one of the
**	two fixed vertices the refusal is between, a neighbour of the frozen point left at a corner, or -- failing
**	both -- the structure nearest the refusal, when that is an unplanned crossing rather than a planned one.
**	Only a corner refusal (`corner.Kind` 1 or 2) can belong to a crossing at all.  A window Phase 10 has already
**	promoted (its stops stand for no topology node) is the search's own choice too when `promoted_too` is set.
**	The repair asks only for exact involvement; the census also accepts the nearest-structure fallback
**	(`nearest_too`), which is a diagnostic reading, never a reason to change a road.
*/
static int Unplanned_Crossing_At_Fault(const CornerFailure & corner,const DynamicVectorClass<RouteCrossing> & crossings,
													const DynamicVectorClass<Vector2> & stops,
													const DynamicVectorClass<unsigned int> & stop_window,
													const DynamicVectorClass<Vector2> & stop_axis,
													const DynamicVectorClass<unsigned int> & stop_node,bool promoted_too,
													bool nearest_too)
{
	if ((corner.Kind != 1) && (corner.Kind != 2)) return -1;

	int nearest = -1;
	float nearest_distance = 3.4e38f;
	for (int c = 0; c < crossings.Count(); ++c) {
		if (crossings[c].Aligned) continue;
		const unsigned int id = crossings[c].WindowId;
		bool planned = false;
		for (int n = 0; (n < stop_window.Count()) && !planned; ++n) {
			planned = (stop_window[n] == id) && (!promoted_too || (stop_node[n] != 0));
		}
		if (planned) continue;
		if ((corner.Kind == 2) && ((corner.LowWindow == id) || (corner.HighWindow == id))) return c;
		if ((corner.Kind == 1) && ((corner.LowWindow == id) || (corner.BeforeWindow == id) || (corner.AfterWindow == id))) {
			return c;
		}
		const float distance = Min2((crossings[c].Ingress - corner.Where).Length(),(crossings[c].Egress - corner.Where).Length());
		if (distance < nearest_distance) {
			nearest_distance	= distance;
			nearest				= c;
		}
	}
	if ((nearest < 0) || !nearest_too) return -1;

	for (int n = 0; n < stops.Count(); ++n) {
		const bool structure = ((stop_window[n] != 0) || (stop_axis[n].Length() > 0.0f));
		const bool promoted = promoted_too && (stop_window[n] != 0) && (stop_node[n] == 0);
		if (structure && !promoted && ((stops[n] - corner.Where).Length() <= nearest_distance)) return -1;
	}
	return nearest;
}


//	Does the chain cross window `id`'s river from one bank to the other outside [first, last]?
static bool Chain_Recrosses_Window(RouteBuilder & b,const DynamicVectorClass<int> & chain,unsigned int id,int first,int last)
{
	const HybridCrossingWindow * window = b.Hydrology->Find_Window(id);
	if (window == nullptr) return false;
	const Vector2 normal(-window->Tangent.Y,window->Tangent.X);
	int index = 0;
	while (index < chain.Count()) {
		if ((index >= first) && (index <= last)) {
			index = last + 1;
			continue;
		}
		if (Cached_Window(b,chain[index]) != id) {
			index++;
			continue;
		}
		int run_end = index;
		while (((run_end + 1) < chain.Count()) && (((run_end + 1) < first) || ((run_end + 1) > last)) &&
				 (Cached_Window(b,chain[run_end + 1]) == id)) run_end++;
		const int before = (index > 0) ? (index - 1) : index;
		const int after = ((run_end + 1) < chain.Count()) ? (run_end + 1) : run_end;
		const Vector2 entered = b.Cell_Center(chain[before]) - window->Center;
		const Vector2 left = b.Cell_Center(chain[after]) - window->Center;
		const float across_in = (entered.X * normal.X) + (entered.Y * normal.Y);
		const float across_out = (left.X * normal.X) + (left.Y * normal.Y);
		bool wet = false;
		for (int c = before; (c < after) && !wet; ++c) {
			wet = Segment_Meets_River(b,b.Cell_Center(chain[c]),b.Cell_Center(chain[c + 1]),nullptr);
		}
		if (wet && (((across_in < 0.0f) && (across_out > 0.0f)) || ((across_in > 0.0f) && (across_out < 0.0f)))) return true;
		index = run_end + 1;
	}
	return false;
}


/*
**	The exact cause of a structural-transition refusal (census only).
**
**	Two fixed vertices crowding one leg: two structures -- or a structure and the road's own end -- are closer
**	than their turn allows (`sequence_too_close`), unless one side is a lead, in which case the lead before or
**	after its structure has no room.  A frozen point left at a corner: its neighbour on the far side of the axis
**	means the road meets it from behind (`wrong_side`); otherwise the lead on that side has no room.
**
**	A lead with no room is then judged on the transition the corner construction wanted: the straight `2R`
**	beyond the structure's end on that side, at the class roadbed width.  Hard ground, the envelope, the grade
**	or the cross slope there is the terrain's cause.  If none refuses it, the structure and its two stop
**	neighbours are asked of the Q-023 empty-plane connector: refused there means Phase 9's test and Phase 10's
**	disagree (`q023_phase10_mismatch`); accepted means the ideal transition is legal and the searched chain
**	left the lead before the approach (`lead_before_approach`) or the departure after the structure
**	(`departure_after_structure`) no room.
**
**	Two causes come first, because they are not about one transition's room at all.  The structure nearest
**	the failure may be a window the search crossed without being sent to it (`unplanned_crossing`): nothing
**	aligned the chain with its axis.  Or the chain may cross a planned window's own obstruction outside that
**	window's crossing (`far_bank_approach`): the road reaches the approach from the far bank, which Q-023's
**	empty-plane facing test cannot see.
*/
static int Structural_Cause(RouteBuilder & b,const CornerFailure & corner,const DynamicVectorClass<Vector2> & stops,
									 const DynamicVectorClass<unsigned int> & stop_window,
									 const DynamicVectorClass<Vector2> & stop_axis,const DynamicVectorClass<unsigned int> & stop_node,
									 const DynamicVectorClass<int> & chain,
									 const DynamicVectorClass<RouteCrossing> & crossings,HybridRoadClass road_class)
{
	const HybridConstants & k = b.P->Constants;
	const float radius = Hybrid_Road_Class_Spec(road_class).MinTurnRadiusW * k.W;

	if (Unplanned_Crossing_At_Fault(corner,crossings,stops,stop_window,stop_axis,stop_node,true,true) >= 0) {
		return HYBRID_STRUCTURAL_UNPLANNED_CROSSING;
	}

	//	The structure the failure belongs to: its two stops (a window) or its one (an overpass), nearest the failure.
	int lo = -1;
	int hi = -1;
	float best = 3.4e38f;
	for (int n = 0; n < stops.Count(); ++n) {
		const bool window = (stop_window[n] != 0) && ((n + 1) < stops.Count()) && (stop_window[n + 1] == stop_window[n]);
		const bool overpass = (stop_axis[n].Length() > 0.0f);
		if (!window && !overpass) continue;
		const int last = window ? (n + 1) : n;
		const float distance = Min2((stops[n] - corner.Where).Length(),(stops[last] - corner.Where).Length());
		if (distance < best) {
			best	= distance;
			lo		= n;
			hi		= last;
		}
		if (window) ++n;
	}
	if (lo < 0) return HYBRID_STRUCTURAL_SEQUENCE;

	//	The planned window crossed again outside its own crossing: the road came at it from the far bank.
	if (hi > lo) {
		for (int c = 0; c < crossings.Count(); ++c) {
			if (crossings[c].WindowId != stop_window[lo]) continue;
			if (Chain_Recrosses_Window(b,chain,crossings[c].WindowId,crossings[c].FirstIndex,crossings[c].LastIndex)) {
				return HYBRID_STRUCTURAL_FAR_BANK;
			}
		}
	}

	const Vector2 axis = (hi > lo) ? Unit(stops[hi] - stops[lo]) : stop_axis[lo];
	bool before = (((corner.Where - stops[lo]).X * axis.X) + ((corner.Where - stops[lo]).Y * axis.Y)) < 0.0f;

	if (corner.Kind == 2) {
		const bool low_fixed = corner.LowFrozen || corner.LowEnd;
		const bool high_fixed = corner.HighFrozen || corner.HighEnd;
		if (low_fixed && high_fixed && !corner.LowLead && !corner.HighLead) return HYBRID_STRUCTURAL_SEQUENCE;
		if (corner.LowLead && corner.HighLead) return HYBRID_STRUCTURAL_SEQUENCE;
		if (corner.LowLead)	before = (corner.LowLeadSide > 0);
		if (corner.HighLead)	before = (corner.HighLeadSide > 0);
	} else if (corner.Kind == 1) {
		const Vector2 neighbour = before ? corner.Before : corner.After;
		const float along = ((neighbour - corner.FrozenPoint).X * corner.FrozenAxis.X) +
								  ((neighbour - corner.FrozenPoint).Y * corner.FrozenAxis.Y);
		if ((before && (along > 0.0f)) || (!before && (along < 0.0f))) return HYBRID_STRUCTURAL_WRONG_SIDE;
	}

	//	The ideal transition on that side: straight on from the structure's end for 2R.
	const Vector2 end = before ? stops[lo] : stops[hi];
	const Vector2 out = before ? Vector2(-axis.X,-axis.Y) : axis;
	const float half = 0.5f * Hybrid_Roadbed_Width(road_class,k.W);
	const float max_grade = Hybrid_Road_Class_Spec(road_class).MaxGradeDegrees * PI_OVER_180;
	const float max_cross = Hybrid_Road_Class_Spec(road_class).MaxCrossSlopeDegrees * PI_OVER_180;
	const float step = Min2(k.W,0.5f * k.A);
	float previous_z = 0.0f;
	for (float s = step; s <= (2.0f * radius); s += step) {
		const Vector2 point = end + (out * s);
		if (Corridor_Excess(b,point) > 0.0f) return HYBRID_STRUCTURAL_ENVELOPE;
		if ((Classify_Point(b,point) != 0) && (Cached_Refusal(b,b.Cell_Of(point)) != 0)) return HYBRID_STRUCTURAL_OBSTACLE;
		float z = 0.0f;
		if (!Height_At(b,point,&z)) return HYBRID_STRUCTURAL_OBSTACLE;
		if (s > step) {
			const HybridVerticalInterval here = Hybrid_Road_Vertical_Interval(z,b.P->MaxCutDepth,b.P->MaxFillHeight);
			const HybridVerticalInterval there = Hybrid_Road_Vertical_Interval(previous_z,b.P->MaxCutDepth,b.P->MaxFillHeight);
			if (!Hybrid_Grade_Compatible(there,here,step,max_grade)) return HYBRID_STRUCTURAL_GRADE;
		}
		previous_z = z;
		const Vector2 normal(-out.Y,out.X);
		float left = 0.0f;
		float right = 0.0f;
		if (Height_At(b,point + (normal * half),&left) && Height_At(b,point - (normal * half),&right)) {
			const HybridVerticalInterval l = Hybrid_Road_Vertical_Interval(left,b.P->MaxCutDepth,b.P->MaxFillHeight);
			const HybridVerticalInterval r = Hybrid_Road_Vertical_Interval(right,b.P->MaxCutDepth,b.P->MaxFillHeight);
			if (!Hybrid_Grade_Compatible(l,r,2.0f * half,max_cross)) return HYBRID_STRUCTURAL_CROSS_SLOPE;
		}
	}

	//	The Q-023 empty-plane connector between the structure and its stop neighbours.
	HybridRoadPose structure;
	structure.Point	= (stops[lo] + stops[hi]) * 0.5f;
	structure.Axis	= axis;
	structure.Before	= (hi > lo) ? (0.5f * (stops[hi] - stops[lo]).Length()) : (OVERPASS_ARM_ROUTES_W * k.W);
	structure.After	= structure.Before;
	HybridRoadPose neighbour;
	neighbour.Point = before ? ((lo > 0) ? stops[lo - 1] : stops[lo]) : (((hi + 1) < stops.Count()) ? stops[hi + 1] : stops[hi]);
	if ((neighbour.Point - structure.Point).Length() > k.Q) {
		const bool joins = before ? Hybrid_Road_Poses_Connect(neighbour,structure,road_class,k)
										  : Hybrid_Road_Poses_Connect(structure,neighbour,road_class,k);
		if (!joins) return HYBRID_STRUCTURAL_Q023_MISMATCH;
	}
	return before ? HYBRID_STRUCTURAL_LEAD_BEFORE : HYBRID_STRUCTURAL_DEPARTURE_AFTER;
}


//	Removes stop `n` from Route_Road's parallel stop arrays and shifts the logical edges' stop ranges.
static void Remove_Stop(int n,int segment_count,DynamicVectorClass<Vector2> * stops,
								DynamicVectorClass<unsigned int> * stop_window,DynamicVectorClass<unsigned int> * stop_site,
								DynamicVectorClass<Vector2> * stop_axis,DynamicVectorClass<unsigned int> * stop_node,
								DynamicVectorClass<int> * stop_segment,DynamicVectorClass<int> * leg_start,
								DynamicVectorClass<int> * first_stop,DynamicVectorClass<int> * last_stop,
								DynamicVectorClass<unsigned int> * stop_align)
{
	const int s = (*stop_segment)[n];
	stop_align->Delete(n);
	stops->Delete(n);
	stop_window->Delete(n);
	stop_site->Delete(n);
	stop_axis->Delete(n);
	stop_node->Delete(n);
	Erase_Int(stop_segment,n);
	Erase_Int(leg_start,n);
	for (int q = 0; q < segment_count; ++q) {
		if (q > s) (*first_stop)[q]--;
		if (q >= s) (*last_stop)[q]--;
	}
}


/*
**	An edge advancing to its next envelope starts it as the plan's envelope sequence does: no natural crossing
**	promoted, no window unavailable, the full local-retry budget.
*/
static void Next_Envelope_Reset(int s,int segment_count,DynamicVectorClass<Vector2> * stops,
										  DynamicVectorClass<unsigned int> * stop_window,DynamicVectorClass<unsigned int> * stop_site,
										  DynamicVectorClass<Vector2> * stop_axis,DynamicVectorClass<unsigned int> * stop_node,
										  DynamicVectorClass<int> * stop_segment,DynamicVectorClass<int> * leg_start,
										  DynamicVectorClass<int> * first_stop,DynamicVectorClass<int> * last_stop,
										  DynamicVectorClass<int> * unavailable_segment,
										  DynamicVectorClass<unsigned int> * unavailable_window,DynamicVectorClass<int> * local_used,
										  DynamicVectorClass<unsigned int> * stop_align)
{
	for (int n = stops->Count() - 1; n >= 0; --n) {
		if (((*stop_segment)[n] != s) || ((*stop_node)[n] != 0) || ((*stop_window)[n] == 0)) continue;
		Remove_Stop(n,segment_count,stops,stop_window,stop_site,stop_axis,stop_node,stop_segment,leg_start,first_stop,
						last_stop,stop_align);
	}
	DynamicVectorClass<int> kept_segment;
	DynamicVectorClass<unsigned int> kept_window;
	for (int u = 0; u < unavailable_segment->Count(); ++u) {
		if ((*unavailable_segment)[u] == s) continue;
		kept_segment.Add((*unavailable_segment)[u]);
		kept_window.Add((*unavailable_window)[u]);
	}
	unavailable_segment->Delete_All();
	unavailable_window->Delete_All();
	for (int u = 0; u < kept_segment.Count(); ++u) {
		unavailable_segment->Add(kept_segment[u]);
		unavailable_window->Add(kept_window[u]);
	}
	(*local_used)[s] = 0;
}


//	The interchange anchored at `node`, when Phase 10 has not withdrawn it.
static const HybridInterchange * Active_Interchange_At(const RouteBuilder & b,unsigned int node)
{
	for (int i = 0; i < b.Topology->Interchanges.Count(); ++i) {
		const HybridInterchange & interchange = b.Topology->Interchanges[i];
		if ((interchange.AnchorNode != node) || (interchange.Status != HYBRID_INTERCHANGE_ACTIVE)) continue;
		if (b.Out->Interchange_Withdrawn(interchange.StableId)) continue;
		return &interchange;
	}
	return nullptr;
}


static bool Route_Road(RouteBuilder & b,const DynamicVectorClass<const HybridTopologyEdge *> & segments,
							  HybridRouteResult * result)
{
	const HybridConstants & k = b.P->Constants;
	const int segment_count = segments.Count();
	if (segment_count <= 0) return true;
	const HybridTopologyEdge & lead = *segments[0];
	const HybridRoadClass road_class = (lead.RoadClass < HYBRID_ROAD_CLASS_COUNT) ? lead.RoadClass
																								  : HYBRID_ROAD_SECONDARY_2;
	const float preferred = Hybrid_Preferred_Corridor_Half_Width(road_class,k.S,k.D);
	const float maximum = Hybrid_Max_Corridor_Half_Width(road_class,k.S,k.D);
	const bool trunk = (lead.Kind == HYBRID_EDGE_FAMILY_TRUNK) && (lead.RouteFamily >= 0);
	StringClass detail;

	//	The failure detail describes this road's own searches, never an earlier road's.
	b.LastExpansions		= 0;
	b.LastGoalTouched		= 0;
	b.LastClosed			= 0;
	b.LastRadius			= 0.0f;
	b.LastRadiusAt			= -1;
	b.LastRadiusWindow	= 0;

	/*
	**	The stops this road routes between, over all its logical edges.  A crossing waypoint is not one stop but
	**	two: the window's own frozen approach points, one on each bank.  Routing to the window's centre cell and
	**	out again left the search free to enter and leave the corridor at whatever angle suited it, and the
	**	crossing then was neither straight nor through the frozen approach points.  Stopping on each approach
	**	point in turn makes the crossing a leg of its own: straight, through both frozen points, and the only leg
	**	allowed inside the obstruction.  Consecutive logical edges share the stop at the node between them.
	*/
	DynamicVectorClass<Vector2> stops;
	DynamicVectorClass<unsigned int> stop_window;		// the window a stop belongs to, 0 for open ground
	DynamicVectorClass<unsigned int> stop_site;			// the Tiberium reserve a stop may enter, 0 for none

	/*
	**	An overpass obligation is a structural crossing on open ground: its deck is straight, and Phase 9 chose
	**	it where two intent segments cross at 60 to 120 degrees.  So each road passes it at its own position and
	**	along its own intent segment -- the line from the node before it to the node after it on this edge --
	**	exactly as a crossing window is passed along its axis (revised D-009, Q-022).
	*/
	DynamicVectorClass<Vector2> stop_axis;				// zero except at an overpass obligation
	DynamicVectorClass<unsigned int> stop_node;		// the topology node each stop stands for
	DynamicVectorClass<int> stop_segment;				// the logical edge a stop belongs to

	/*
	**	Section 12.6.9 item 1: a HIGHWAY_4 trunk through an active interchange anchor X runs straight along the
	**	frozen axis `a` over [X - S_I, X + S_I].  X becomes three stops, a_I, X and b_I, all marked with the
	**	interchange; the legs between them are its aligned straight, and the two legs outside them arrive on and
	**	leave along `a`.
	*/
	DynamicVectorClass<unsigned int> stop_align;		// the interchange a stop is aligned through, 0 for none
	DynamicVectorClass<int> first_stop;					// each logical edge's first stop
	DynamicVectorClass<RouteVertex> structural_points;
	b.RoadNodes.Delete_All();

	for (int s = 0; s < segment_count; ++s) {

		const HybridTopologyEdge & edge = *segments[s];
		DynamicVectorClass<unsigned int> nodes;
		Edge_Node_Chain(b,edge,&nodes);
		first_stop.Add((s == 0) ? 0 : (stops.Count() - 1));

		for (int n = 0; n < nodes.Count(); ++n) {

			bool known = false;
			for (int r = 0; (r < b.RoadNodes.Count()) && !known; ++r) known = (b.RoadNodes[r] == nodes[n]);
			if (!known) b.RoadNodes.Add(nodes[n]);
			if ((s > 0) && (n == 0)) {							//	the stop the previous edge ended on
				const HybridInterchange * aligned = (trunk && (road_class == HYBRID_ROAD_HIGHWAY_4))
																? Active_Interchange_At(b,nodes[n]) : nullptr;
				if ((aligned != nullptr) && (stops.Count() >= 2)) {
					//	b_I: the far end of the straight, past X along the road's own travel direction.
					const Vector2 x_point = stops[stops.Count() - 1];
					const Vector2 a_point = stops[stops.Count() - 2];
					const Vector2 axis = (((x_point - a_point).X * aligned->Axis.X) + ((x_point - a_point).Y * aligned->Axis.Y) >= 0.0f)
												? aligned->Axis : Vector2(-aligned->Axis.X,-aligned->Axis.Y);
					stops.Add(x_point + (axis * aligned->StraightHalfExtent));
					stop_window.Add(0);
					stop_site.Add(0);
					stop_axis.Add(axis);
					stop_node.Add(0);
					stop_segment.Add(s);
					stop_align.Add(aligned->StableId);
				}
				continue;
			}

			const HybridTopologyNode * node = b.Topology->Find_Node(nodes[n]);
			if (node == nullptr) continue;

			const HybridCrossingWindow * window = (node->WindowId != 0)
															  ? b.Hydrology->Find_Window(node->WindowId) : nullptr;
			if (window != nullptr) {

				/*
				**	Which bank the road arrives on is decided by the whole journey, not by the stop behind it: a
				**	window whose axis lies across the route is entered on the near bank and left on the far one.
				*/
				Vector2 before = (stops.Count() > 0) ? stops[stops.Count() - 1] : node->Position;
				if (stops.Count() == 0) {
					for (int q = n - 1; q >= 0; --q) {
						const HybridTopologyNode * previous = b.Topology->Find_Node(nodes[q]);
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

				for (int side = 0; side < 2; ++side) {
					const bool first_bank = (side == 0);
					stops.Add((a_first == first_bank) ? window->ApproachA : window->ApproachB);
					stop_window.Add(node->WindowId);
					stop_site.Add(0);
					stop_axis.Add(Vector2(0.0f,0.0f));
					stop_node.Add(node->StableId);
					stop_segment.Add(s);
					stop_align.Add(0);
				}
				continue;
			}

			Vector2 axis(0.0f,0.0f);
			if ((node->Kind == HYBRID_NODE_OVERPASS_OBLIGATION) && (n > 0) && ((n + 1) < nodes.Count())) {
				const HybridTopologyNode * before = b.Topology->Find_Node(nodes[n - 1]);
				const HybridTopologyNode * after = b.Topology->Find_Node(nodes[n + 1]);
				if ((before != nullptr) && (after != nullptr)) axis = Unit(after->Position - before->Position);
				if (axis.Length() > 0.0f) {
					RouteVertex point;
					point.P		= Vector2(Hybrid_Quantize_Position(node->Position.X),Hybrid_Quantize_Position(node->Position.Y));
					point.Axis	= axis;
					point.Frozen= true;
					point.Keep	= Overpass_Straight(b,edge,*node,axis);
					structural_points.Add(point);
				}
			}

			//	a_I before an active interchange anchor X that joins this logical edge to the next.
			const HybridInterchange * aligned = (trunk && (road_class == HYBRID_ROAD_HIGHWAY_4) && (n == (nodes.Count() - 1)) &&
															 ((s + 1) < segment_count) && (stops.Count() > 0))
															? Active_Interchange_At(b,nodes[n]) : nullptr;
			if (aligned != nullptr) {
				const Vector2 before = stops[stops.Count() - 1];
				const Vector2 axis = ((((node->Position - before).X * aligned->Axis.X) + ((node->Position - before).Y * aligned->Axis.Y)) >= 0.0f)
											? aligned->Axis : Vector2(-aligned->Axis.X,-aligned->Axis.Y);
				stops.Add(node->Position - (axis * aligned->StraightHalfExtent));
				stop_window.Add(0);
				stop_site.Add(0);
				stop_axis.Add(axis);
				stop_node.Add(0);
				stop_segment.Add(s);
				stop_align.Add(aligned->StableId);
			}

			stops.Add(node->Position);
			stop_window.Add(node->WindowId);
			stop_site.Add((node->Kind == HYBRID_NODE_FIELD_ACCESS) ? node->SiteId : 0);
			stop_axis.Add(axis);
			stop_node.Add(node->StableId);
			stop_segment.Add(s);
			stop_align.Add((aligned != nullptr) ? aligned->StableId : 0);
		}
	}
	if (stops.Count() < 2) return true;

	//	The last stop of each logical edge is the first of the next; the last edge ends on the road's last stop.
	DynamicVectorClass<int> last_stop;
	for (int s = 0; s < segment_count; ++s) last_stop.Add(((s + 1) < segment_count) ? first_stop[s + 1] : (stops.Count() - 1));

	b.CurrentClass			= road_class;
	b.CurrentFamily		= lead.RouteFamily;
	b.CurrentIsOptional	= !trunk && (Hybrid_Route_Priority_Of(*b.Topology,lead) == HYBRID_ROUTE_PRIORITY_OPTIONAL_LINK);
	b.EntryDirection		= START_DIRECTION;
	b.EntryRun				= 0.0f;
	b.StructuralLeg		= false;

	//	The junctions this road shares with every road already on the ground.
	b.SharedRoute.Delete_All();
	b.SharedPoint.Delete_All();
	for (int r = 0; r < b.Out->Routes.Count(); ++r) {
		for (int n = 0; n < b.RoadNodes.Count(); ++n) {
			if (!Route_Carries_Node(b,b.Out->Routes[r],b.RoadNodes[n])) continue;
			const HybridTopologyNode * node = b.Topology->Find_Node(b.RoadNodes[n]);
			if (node == nullptr) continue;
			b.SharedRoute.Add(r);
			b.SharedPoint.Add(node->Position);
		}
	}

	/*
	**	Where each logical edge's envelope search starts.  Phase 11 may have ruled out the envelopes it already
	**	tried, in which case the search begins strictly after the one that produced the route it refused.
	*/
	DynamicVectorClass<int> envelope;
	for (int s = 0; s < segment_count; ++s) {
		int floor = 0;
		for (int f = 0; f < b.P->EnvelopeFloors.Count(); ++f) {
			if (b.P->EnvelopeFloors[f].EdgeId != segments[s]->StableId) continue;
			floor = b.P->EnvelopeFloors[f].MinEnvelope;
			break;
		}
		envelope.Add((floor < 0) ? 0 : ((floor > 2) ? 2 : floor));
	}

	//	Each logical edge's starting state, so a later edge's retry keeps the chains of the edges before it.
	DynamicVectorClass<int> segment_chain;
	DynamicVectorClass<int> segment_crossings;
	DynamicVectorClass<int> segment_direction;
	DynamicVectorClass<float> segment_run;
	DynamicVectorClass<int> segment_expansions;
	DynamicVectorClass<int> local_used;						// local retries spent in each edge's current envelope
	for (int s = 0; s < segment_count; ++s) {
		segment_chain.Add(0);
		segment_crossings.Add(0);
		segment_direction.Add(START_DIRECTION);
		segment_run.Add(-1.0f);
		segment_expansions.Add(0);
		local_used.Add(0);
	}

	DynamicVectorClass<int> chain;
	DynamicVectorClass<RouteCrossing> crossings;
	int entry_direction = START_DIRECTION;
	float entry_run_override = -1.0f;
	int restart = 0;
	const char * spline_reason = "no path";

	//	The census of this road's last refusal.
	int failed_segment = 0;
	int failed_leg = -1;
	Vector2 last_failure_point(0.0f,0.0f);
	CornerFailure last_corner;
	DynamicVectorClass<int> last_chain;
	DynamicVectorClass<RouteCrossing> last_crossings;
	int failed_frontier[HYBRID_BLOCK_COUNT];
	for (int i = 0; i < HYBRID_BLOCK_COUNT; ++i) failed_frontier[i] = 0;

	//	The chain index each leg began at, so a crossing in the chain can be charged to the leg that made it.
	DynamicVectorClass<int> leg_start;
	for (int n = 0; n < stops.Count(); ++n) leg_start.Add(0);

	/*
	**	Three envelopes per logical edge, and in each one budget of `retry.max_local_retries_per_stage` local
	**	retries shared by the centerline's step-5 insertions and the natural-crossing alignment repair below
	**	(Section 17's Phase-10 operation).  A window made unavailable to an edge stays so for the rest of that
	**	edge's envelope; a promotion stays constrained for the rest of it.  Both are undone when the edge
	**	advances to its next envelope, which starts with a fresh budget, as any envelope does.
	*/
	const int local_limit = (b.P->MaxLocalRetriesPerStage > 0) ? b.P->MaxLocalRetriesPerStage : 0;
	DynamicVectorClass<int> unavailable_segment;			// (edge, window) pairs made unavailable in its envelope
	DynamicVectorClass<unsigned int> unavailable_window;
	const int attempts = 3 * segment_count * (local_limit + 2);
	for (int attempt = 0; attempt <= attempts; ++attempt) {

		//	Back to the start of the logical edge being retried.
		if (restart <= 0) {
			chain.Delete_All();
			crossings.Delete_All();
			entry_direction		= START_DIRECTION;
			entry_run_override	= -1.0f;
		} else {
			//	DynamicVectorClass<int> cannot tell Delete(index) from Delete(value), so the prefix is copied.
			DynamicVectorClass<int> kept_chain;
			for (int c = 0; c < segment_chain[restart]; ++c) kept_chain.Add(chain[c]);
			chain.Delete_All();
			for (int c = 0; c < kept_chain.Count(); ++c) chain.Add(kept_chain[c]);
			DynamicVectorClass<RouteCrossing> kept_crossings;
			for (int c = 0; c < segment_crossings[restart]; ++c) kept_crossings.Add(crossings[c]);
			crossings.Delete_All();
			for (int c = 0; c < kept_crossings.Count(); ++c) crossings.Add(kept_crossings[c]);
			entry_direction		= segment_direction[restart];
			entry_run_override	= segment_run[restart];
		}

		bool routed = true;
		for (int s = restart; (s < segment_count) && routed; ++s) {

			segment_chain[s]		= chain.Count();
			segment_crossings[s]	= crossings.Count();
			segment_direction[s]	= entry_direction;
			segment_run[s]			= entry_run_override;

			//	This logical edge's own corridor, envelope, endpoint exemptions and expansion budget.
			b.CurrentEdgeId		= segments[s]->StableId;
			b.CurrentHalfWidth	= Min2(preferred + ((float)envelope[s] * k.S),maximum);
			b.CorridorPath.Delete_All();
			b.CorridorHalfWidths.Delete_All();
			for (int n = first_stop[s]; n <= last_stop[s]; ++n) b.CorridorPath.Add(stops[n]);
			b.OwnEntranceNodes.Delete_All();
			b.OwnEntranceNodes.Add(segments[s]->NodeA);
			b.OwnEntranceNodes.Add(segments[s]->NodeB);
			b.Expansions = 0;
			b.ForbiddenWindows.Delete_All();
			for (int u = 0; u < unavailable_segment.Count(); ++u) {
				if (unavailable_segment[u] == s) b.ForbiddenWindows.Add(unavailable_window[u]);
			}

			for (int leg = first_stop[s]; (leg < last_stop[s]) && routed; ++leg) {

				//	This leg's own corridor, and the two per-leg exemptions its endpoints carry.
				b.CorridorA		= stops[leg];
				b.CorridorB		= stops[leg + 1];
				b.OwnWindow		= (stop_window[leg] != 0) ? stop_window[leg] : stop_window[leg + 1];
				b.StructuralLeg= (stop_window[leg] != 0) && (stop_window[leg] == stop_window[leg + 1]);
				b.AlignedLeg	= (stop_align[leg] != 0) && (stop_align[leg] == stop_align[leg + 1]);
				b.ClosedWindow	= b.StructuralLeg ? 0 : b.OwnWindow;
				b.OwnFieldSite	= (stop_site[leg] != 0) ? stop_site[leg] : stop_site[leg + 1];
				b.OwnEndFieldSite = 0;

				/*
				**	A crossing is entered and left along its own axis: a leg arriving at a crossing's first approach
				**	point must arrive on it, and a leg leaving its last one must leave on it.
				*/
				b.ArrivalAxis.Set(0.0f,0.0f);
				b.DepartureAxis.Set(0.0f,0.0f);
				if (!b.StructuralLeg && !b.AlignedLeg) {
					if (((leg + 2) < stops.Count()) && (stop_window[leg + 1] != 0) &&
						 (stop_window[leg + 1] == stop_window[leg + 2])) {
						b.ArrivalAxis = Unit(stops[leg + 2] - stops[leg + 1]);
					}
					if ((leg >= 1) && (stop_window[leg] != 0) && (stop_window[leg] == stop_window[leg - 1])) {
						b.DepartureAxis = Unit(stops[leg] - stops[leg - 1]);
					}
					if (stop_axis[leg + 1].Length() > 0.0f)	b.ArrivalAxis = stop_axis[leg + 1];
					if (stop_axis[leg].Length() > 0.0f)		b.DepartureAxis = stop_axis[leg];
				}

				const int start = b.Cell_Of(stops[leg]);
				const int goal = b.Cell_Of(stops[leg + 1]);
				leg_start[leg] = chain.Count();
				if (start == goal) {
					if ((chain.Count() == 0) || (chain[chain.Count() - 1] != start)) chain.Add(start);
					continue;
				}

				const int chain_before = chain.Count();
				const int entry = entry_direction;

				//	The straight this leg is handed by the one before it, so a turn at the boundary is measured.
				b.EntryDirection	= entry;
				b.EntryRun			= 0.0f;
				if (entry_run_override >= 0.0f) {
					b.EntryRun = entry_run_override;
				} else if ((entry != START_DIRECTION) && (chain.Count() >= 2)) {
					const int arrival = Chain_Direction(b,chain[chain.Count() - 2],chain[chain.Count() - 1]);
					for (int c = chain.Count() - 1; c >= 1; --c) {
						if (Chain_Direction(b,chain[c - 1],chain[c]) != arrival) break;
						b.EntryRun += (b.Cell_Center(chain[c]) - b.Cell_Center(chain[c - 1])).Length();
					}
				}

				/*
				**	A logical edge after the first starts at an ordinary family waypoint, which is position-only: the
				**	road is offered the heading it arrived on, so the waypoint costs no turn, and when that heading
				**	leads nowhere the edge searches from the waypoint as its own A* always has -- the centerline then
				**	rounds the corner there at the class radius, or refuses it.
				*/
				/*
				**	A leg arriving at a trunk joint -- the ordinary family waypoint where this logical edge hands over
				**	to the next -- is first asked to arrive heading toward the next edge's first stop, so the road
				**	can carry straight on through the waypoint; only when it cannot does it arrive as it likes and
				**	leave the corner to the centerline.  The waypoint stays position-only either way.
				*/
				const bool to_joint = ((s + 1) < segment_count) && ((leg + 1) == last_stop[s]) && !b.StructuralLeg && !b.AlignedLeg &&
											 (stop_window[leg + 1] == 0) && (stop_axis[leg + 1].Length() <= 0.0f) &&
											 (b.ArrivalAxis.Length() <= 0.0f) && ((leg + 2) < stops.Count());
				if (to_joint) b.ArrivalAxis = Unit(stops[leg + 2] - stops[leg + 1]);
				bool found = Search_Leg(b,start,goal,entry,&chain,result);
				if (!found && to_joint && ((result == nullptr) || (result->Failure != HYBRID_ROUTE_EXPANSION_BUDGET))) {
					b.ArrivalAxis.Set(0.0f,0.0f);
					found = Search_Leg(b,start,goal,entry,&chain,result);
				}
				if (!found && (s > 0) && (leg == first_stop[s]) && (entry != START_DIRECTION) &&
					 ((result == nullptr) || (result->Failure != HYBRID_ROUTE_EXPANSION_BUDGET))) {
					b.EntryDirection	= START_DIRECTION;
					b.EntryRun			= 0.0f;
					found = Search_Leg(b,start,goal,START_DIRECTION,&chain,result);
				}
				if (!found) {
					routed			= false;
					failed_segment	= s;
					failed_leg		= leg;
					for (int i = 0; i < HYBRID_BLOCK_COUNT; ++i) failed_frontier[i] = b.P->CollectCensus ? b.LastFrontier[i] : 0;
					break;
				}

				/*
				**	A structural leg *is* a crossing: record which window, which frozen approach point it entered
				**	by and left by, and the stretch of chain it laid down.
				*/
				if (b.StructuralLeg) {
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

				//	An aligned leg extends its interchange's straight record (a_I -> X -> b_I): open road, never a span.
				if (b.AlignedLeg) {
					int first_index = (chain_before > 0) ? (chain_before - 1) : 0;
					for (int c = chain.Count() - 1; c >= 0; --c) {
						if (chain[c] != start) continue;
						first_index = c;
						break;
					}
					const unsigned int id = ALIGNED_ID_BASE | stop_align[leg];
					const int last_record = crossings.Count() - 1;
					if ((last_record >= 0) && crossings[last_record].Aligned && (crossings[last_record].WindowId == id)) {
						crossings[last_record].LastIndex	= chain.Count() - 1;
						crossings[last_record].Egress		= stops[leg + 1];
					} else {
						RouteCrossing record;
						record.WindowId	= id;
						record.Aligned		= true;
						record.FirstIndex	= first_index;
						record.LastIndex	= chain.Count() - 1;
						record.Ingress		= stops[leg];
						record.Egress		= stops[leg + 1];
						if (record.LastIndex > record.FirstIndex) crossings.Add(record);
					}
				}

				//	The heading this leg ends on is the heading the next one starts with.
				if (chain.Count() >= 2) {
					const int direction = Chain_Direction(b,chain[chain.Count() - 2],chain[chain.Count() - 1]);
					entry_direction = (direction >= 0) ? direction : START_DIRECTION;
				}

				//	A crossing hands the next leg the crossing's own heading and the whole straight it ran along.
				entry_run_override = -1.0f;
				if (b.StructuralLeg) {
					entry_direction = Nearest_Direction(Unit(stops[leg + 1] - stops[leg]));
					entry_run_override = (stops[leg + 1] - stops[leg]).Length();
				}
				if (b.AlignedLeg) {
					int from_stop = leg;
					while ((from_stop > 0) && (stop_align[from_stop - 1] == stop_align[leg])) from_stop--;
					entry_direction = Nearest_Direction(Unit(stops[leg + 1] - stops[leg]));
					entry_run_override = (stops[leg + 1] - stops[from_stop]).Length();
				}
			}
			segment_expansions[s] = b.Expansions;
		}

		if (!routed) {
			if ((result != nullptr) && (result->Failure == HYBRID_ROUTE_EXPANSION_BUDGET)) {
				b.LastFailedEdge = segments[failed_segment]->StableId;
				return false;
			}
			if (envelope[failed_segment] >= 2) break;
			envelope[failed_segment]++;				// no path in this envelope: widen this logical edge
			Next_Envelope_Reset(failed_segment,segment_count,&stops,&stop_window,&stop_site,&stop_axis,&stop_node,
									  &stop_segment,&leg_start,&first_stop,&last_stop,&unavailable_segment,
									  &unavailable_window,&local_used,&stop_align);
			restart = failed_segment;
			continue;
		}

		/*
		**	The centerline is built over the whole concatenated chain, so neither a leg boundary nor a logical
		**	edge boundary is a corner.  Every piece of the corridor is measured against its own edge's envelope.
		*/
		b.CorridorPath.Delete_All();
		b.CorridorHalfWidths.Delete_All();
		for (int n = 0; n < stops.Count(); ++n) b.CorridorPath.Add(stops[n]);
		for (int n = 0; (n + 1) < stops.Count(); ++n) {
			b.CorridorHalfWidths.Add(Min2(preferred + ((float)envelope[stop_segment[n + 1]] * k.S),maximum));
		}
		b.CorridorA		= stops[0];
		b.CorridorB		= stops[stops.Count() - 1];
		b.CurrentEdgeId= lead.StableId;
		b.OwnEntranceNodes.Delete_All();
		b.OwnEntranceNodes.Add(segments[0]->NodeA);
		b.OwnEntranceNodes.Add(segments[segment_count - 1]->NodeB);

		/*
		**	The spline pass covers the whole road, so it gets its own cache generation and its own endpoint
		**	exemptions: reusing the last leg's would answer with that leg's window and field exemptions.
		*/
		b.StartCell		= (chain.Count() > 0) ? chain[0] : -1;
		b.GoalCell		= (chain.Count() > 0) ? chain[chain.Count() - 1] : -1;
		b.OwnWindow		= 0;
		b.ClosedWindow	= 0;
		/*
		**	The centerline keeps the exemption its legs had: a road that ends at a field access point may stand in
		**	that field's own reserve, which the access point sits just outside.  Dropping it for the centerline
		**	refused the arc a road ends on, a unit or two inside its own field's reserve, after the search had
		**	accepted the leg.
		*/
		b.OwnFieldSite		= stop_site[0];
		b.OwnEndFieldSite	= stop_site[stop_site.Count() - 1];
		b.StructuralLeg= false;
		b.ForbiddenWindows.Delete_All();
		b.Generation++;
		b.LastChain			= (chain.Count() > 0) ? &chain[0] : nullptr;
		b.LastChainCount	= chain.Count();

		DynamicVectorClass<Vector2> points;
		DynamicVectorClass<float> stations;
		int insertions = 0;
		const Vector2 first_point = stops[0];
		const Vector2 last_point = stops[stops.Count() - 1];

		/*
		**	The cells whose positions earlier phases froze.  A structural point -- a crossing or an overpass
		**	obligation -- is frozen: the road runs straight through it, because a deck is straight.  A
		**	route-family waypoint, including the one between two logical edges of a trunk, is a strategic anchor:
		**	the search's chain passes its cell (the plan's "every frozen waypoint's own cell is on the accepted
		**	chain"), and the centerline is not bound to it -- its corner yields to a structure's lead.
		*/
		DynamicVectorClass<int> mandatory_cells;
		DynamicVectorClass<int> guide_cells;
		b.JointPoints.Delete_All();
		b.GuideCells.Delete_All();
		b.GuidePoints.Delete_All();
		for (int s = 0; s < segment_count; ++s) {
			for (int w = 0; w < segments[s]->Waypoints.Count(); ++w) {
				const HybridTopologyNode * node = b.Topology->Find_Node(segments[s]->Waypoints[w]);
				if (node == nullptr) continue;
				if (node->Kind == HYBRID_NODE_FAMILY_WAYPOINT)	guide_cells.Add(b.Cell_Of(node->Position));
				else															mandatory_cells.Add(b.Cell_Of(node->Position));
			}
			if (s > 0) {
				const HybridTopologyNode * joint = b.Topology->Find_Node(segments[s]->NodeA);
				if (joint != nullptr) {
					const Vector2 point(Hybrid_Quantize_Position(joint->Position.X),Hybrid_Quantize_Position(joint->Position.Y));
					guide_cells.Add(b.Cell_Of(joint->Position));
					b.JointPoints.Add(point);
					b.GuideCells.Add(b.Cell_Of(joint->Position));
					b.GuidePoints.Add(point);
				}
			}
		}

		//	A window the search drove through without being sent to is still a crossing, recorded the same way.
		DynamicVectorClass<RouteCrossing> all_crossings;
		for (int c = 0; c < crossings.Count(); ++c) all_crossings.Add(crossings[c]);
		Note_Unplanned_Crossings(b,chain,&all_crossings);


		DynamicVectorClass<HybridRouteStructuralSpan> spans;
		Vector2 failure_point = first_point;
		CornerFailure corner;
		s_corner_failure = &corner;
		/*
		**	An ordinary guide's corner is held against ordinary corners but yields to a structure's lead: a guide
		**	held absolutely could neither be dropped nor moved, so one standing near a crossing left the crossing's
		**	lead-in no room and refused a road the terrain and the Q-023 empty-plane connector both allow.  Released
		**	altogether, its corner fell back to the chain's cell centres -- inside the field reserves guides stand
		**	beside -- and ordinary fillet crowding dropped it, cutting the road across the reserve.
		*/
		int spent = 0;
		for (int s = 0; s < segment_count; ++s) {
			if (local_used[s] > spent) spent = local_used[s];
		}
		const bool laid = Chain_To_Spline(b,chain,mandatory_cells,structural_points,guide_cells,all_crossings,
													 first_point,last_point,&points,&stations,&spans,
													 (spent < local_limit) ? (local_limit - spent) : 0,&insertions,&spline_reason,
													 &failure_point);
		s_corner_failure = nullptr;
		if (!laid) {
			last_corner = corner;
			if (b.P->CollectCensus) {
				last_chain.Delete_All();
				last_crossings.Delete_All();
				for (int c = 0; c < chain.Count(); ++c) last_chain.Add(chain[c]);
				for (int c = 0; c < all_crossings.Count(); ++c) last_crossings.Add(all_crossings[c]);
			}

			//	Charged to the logical edge whose chain lies nearest the failure.
			failed_leg = -1;
			last_failure_point = failure_point;
			int nearest = 0;
			float best = 3.4e38f;
			for (int c = 0; c < chain.Count(); ++c) {
				const float distance = (b.Cell_Center(chain[c]) - failure_point).Length();
				if (distance >= best) continue;
				best		= distance;
				nearest	= c;
			}
			failed_segment = 0;
			for (int s = 1; s < segment_count; ++s) {
				if (nearest >= segment_chain[s]) failed_segment = s;
			}

			local_used[failed_segment] += insertions;

			/*
			**	Phase10NaturalCrossingAlignmentRepair (Plan Section 8 Phase 10, Section 17).  A crossing the search
			**	made through an accepted window it was not sent to is aligned like a planned one (Q-022), and where
			**	the centerline refuses a corner that belongs to it -- the road not aligned with the window's frozen
			**	axis -- the window's own frozen approach pair becomes two constrained stops of the leg it happened on,
			**	in travel order, and the logical edge is searched again from its start in the same envelope.  The
			**	failing corner is the earliest along the road, so crossings are repaired one at a time in route
			**	order.  If a promoted crossing is the one still refused, its stops are withdrawn and the window is
			**	unavailable to that edge for the rest of the envelope.  Each promotion and each withdrawal consumes
			**	one local retry of that edge's envelope; with none left the envelope advances as before.  The window,
			**	its station, its obstruction and its method are never changed.
			*/
			const int withdrawn_fault = Unplanned_Crossing_At_Fault(corner,all_crossings,stops,stop_window,stop_axis,
																					  stop_node,true,false);
			int promoted_stop = -1;
			for (int n = 0; (n < stops.Count()) && (withdrawn_fault >= 0) && (promoted_stop < 0); ++n) {
				if ((stop_window[n] == all_crossings[withdrawn_fault].WindowId) && (stop_node[n] == 0)) promoted_stop = n;
			}
			if ((promoted_stop >= 0) && ((promoted_stop + 1) < stops.Count()) &&
				 (local_used[stop_segment[promoted_stop]] < local_limit)) {
				const int s = stop_segment[promoted_stop];
				for (int side = 0; side < 2; ++side) {
				Remove_Stop(promoted_stop,segment_count,&stops,&stop_window,&stop_site,&stop_axis,&stop_node,
								&stop_segment,&leg_start,&first_stop,&last_stop,&stop_align);
				}
				unavailable_segment.Add(s);
				unavailable_window.Add(all_crossings[withdrawn_fault].WindowId);
				local_used[s]++;
				restart = s;
				continue;
			}

			const int fault = (promoted_stop < 0)
									? Unplanned_Crossing_At_Fault(corner,all_crossings,stops,stop_window,stop_axis,stop_node,false,false)
									: -1;
			int fault_leg = 0;
			int fault_segment = -1;
			if (fault >= 0) {
				//	The leg whose chain holds the first cell inside the window, and the edge that leg belongs to.
				for (int n = 0; (n + 1) < stops.Count(); ++n) {
					if (leg_start[n] <= (all_crossings[fault].FirstIndex + 1)) fault_leg = n;
				}
				fault_segment = 0;
				for (int q = 0; q < segment_count; ++q) {
					if ((fault_leg >= first_stop[q]) && (fault_leg < last_stop[q])) fault_segment = q;
				}
			}
			if ((fault >= 0) && (local_used[fault_segment] < local_limit)) {
				const RouteCrossing crossing = all_crossings[fault];
				const int s = fault_segment;
				const Vector2 pair[2] = { crossing.Ingress,crossing.Egress };
				for (int side = 1; side >= 0; --side) {
					stops.Insert(fault_leg + 1,pair[side]);
					stop_window.Insert(fault_leg + 1,crossing.WindowId);
					stop_site.Insert(fault_leg + 1,0);
					stop_axis.Insert(fault_leg + 1,Vector2(0.0f,0.0f));
					stop_node.Insert(fault_leg + 1,0);
					stop_align.Insert(fault_leg + 1,0);
					stop_segment.Insert(fault_leg + 1,s);
					leg_start.Insert(fault_leg + 1,0);
				}
				for (int q = 0; q < segment_count; ++q) {
					if (q > s) first_stop[q] += 2;
					if (q >= s) last_stop[q] += 2;
				}
				local_used[s]++;
				restart = s;
				continue;
			}

			if (envelope[failed_segment] >= 2) break;
			envelope[failed_segment]++;
			Next_Envelope_Reset(failed_segment,segment_count,&stops,&stop_window,&stop_site,&stop_axis,&stop_node,
									  &stop_segment,&leg_start,&first_stop,&last_stop,&unavailable_segment,
									  &unavailable_window,&local_used,&stop_align);
			restart = failed_segment;
			continue;
		}

		/*
		**	One centerline, sliced back into the logical edges -- bookkeeping, never another constraint on the road.
		**	A logical boundary at an ordinary guide is the point of the centerline nearest the centre `G` of the
		**	guide's analysis cell: the sample nearest `G` between the previous boundary and the end (ties to the
		**	earlier), then the exact closest point to `G` on the segment before or after that sample, clamped and
		**	quantized.  A boundary inside a structural interval moves to that interval's nearer end, and every edge
		**	keeps a positive length.  The boundary point is inserted into both slices when it is not a sample
		**	already, which lies on the polyline and so changes nothing about the road.
		*/
		DynamicVectorClass<int> split;					// the sample at or before each boundary
		DynamicVectorClass<Vector2> split_point;		// the boundary point itself
		DynamicVectorClass<float> split_station;
		split.Add(0);
		split_point.Add(points[0]);
		split_station.Add(stations[0]);
		for (int s = 1; s < segment_count; ++s) {

			const HybridTopologyNode * joint = b.Topology->Find_Node(segments[s]->NodeA);
			const int from = split[split.Count() - 1];
			const float from_station = split_station[split_station.Count() - 1];
			const Vector2 g = (joint != nullptr) ? b.Cell_Center(b.Cell_Of(joint->Position)) : points[from];

			int at = from;
			float best = 3.4e38f;
			for (int i = from; i < points.Count(); ++i) {
				const float distance = Hybrid_Quantize_Position((points[i] - g).Length());
				if (distance >= best) continue;
				best	= distance;
				at		= i;
			}

			//	The exact closest point on the segment before or after that sample.
			int segment = -1;
			float segment_t = 0.0f;
			float segment_best = 3.4e38f;
			for (int side = 0; side < 2; ++side) {
				const int lo = (side == 0) ? (at - 1) : at;
				if ((lo < from) || ((lo + 1) >= points.Count())) continue;
				const Vector2 d = points[lo + 1] - points[lo];
				const float length2 = (d.X * d.X) + (d.Y * d.Y);
				float t_on = (length2 > 0.0f) ? ((((g.X - points[lo].X) * d.X) + ((g.Y - points[lo].Y) * d.Y)) / length2) : 0.0f;
				t_on = Clamp2(t_on,0.0f,1.0f);
				const float distance = Hybrid_Quantize_Position((g - (points[lo] + (d * t_on))).Length());
				if (distance >= segment_best) continue;
				segment_best	= distance;
				segment			= lo;
				segment_t		= t_on;
			}
			Vector2 point = points[at];
			int index = at;
			float station = stations[at];
			if (segment >= 0) {
				const Vector2 d = points[segment + 1] - points[segment];
				point		= Vector2(Hybrid_Quantize_Position(points[segment].X + (d.X * segment_t)),
									 Hybrid_Quantize_Position(points[segment].Y + (d.Y * segment_t)));
				index		= segment;
				station	= Hybrid_Quantize_Position(stations[segment] + ((stations[segment + 1] - stations[segment]) * segment_t));
				if (point == points[segment + 1]) {
					index		= segment + 1;
					station	= stations[segment + 1];
				}
				if (point == points[segment]) station = stations[segment];
			}

			//	Never inside a structural interval: its nearer end.
			for (int c = 0; c < spans.Count(); ++c) {
				if ((station <= stations[spans[c].FirstPoint]) || (station >= stations[spans[c].LastPoint])) continue;
				index		= ((station - stations[spans[c].FirstPoint]) <= (stations[spans[c].LastPoint] - station))
							  ? spans[c].FirstPoint : spans[c].LastPoint;
				point		= points[index];
				station	= stations[index];
			}

			//	Strictly ordered, and no edge of zero length.
			if (station <= from_station) {
				index		= ((from + 1) < (points.Count() - 1)) ? (from + 1) : (points.Count() - 2);
				point		= points[index];
				station	= stations[index];
			}
			split.Add(index);
			split_point.Add(point);
			split_station.Add(station);
		}
		split.Add(points.Count() - 1);
		split_point.Add(points[points.Count() - 1]);
		split_station.Add(stations[stations.Count() - 1]);

		for (int s = 0; s < segment_count; ++s) {

			const HybridTopologyEdge & edge = *segments[s];
			const int from = split[s];
			const int to = split[s + 1];
			const float from_station = split_station[s];

			HybridRoute route;
			route.EdgeId				= edge.StableId;
			route.RoadClass			= road_class;
			route.Priority				= Hybrid_Route_Priority_Of(*b.Topology,edge);
			route.RouteFamily			= edge.RouteFamily;
			route.Capabilities		= edge.Capabilities;		// Section 12.1: decided by Phase 9, never re-derived
			route.Envelope				= envelope[s];
			route.Required				= edge.Required;
			route.ControlInsertions	= insertions;
			route.Expansions			= segment_expansions[s];

			/*
			**	The boundary point, the samples strictly after it, and the next boundary point.  Sample `i` is slice
			**	point `i - from` either way: when the boundary is inside the segment after sample `from`, the boundary
			**	point takes that sample's place at the head of the slice.
			*/
			route.Points.Add(split_point[s]);
			route.Stations.Add(0.0f);
			for (int i = from + 1; i <= to; ++i) {
				if ((stations[i] >= split_station[s + 1]) && (i == to)) break;
				route.Points.Add(points[i]);
				route.Stations.Add(Hybrid_Quantize_Position(stations[i] - from_station));
			}
			if (!(route.Points[route.Points.Count() - 1] == split_point[s + 1])) {
				route.Points.Add(split_point[s + 1]);
				route.Stations.Add(Hybrid_Quantize_Position(split_station[s + 1] - from_station));
			}
			route.Length = (route.Stations.Count() > 0) ? route.Stations[route.Stations.Count() - 1] : 0.0f;

			const int cell_from = (s == 0) ? 0 : (segment_chain[s] - 1);
			const int cell_to = ((s + 1) < segment_count) ? (segment_chain[s + 1] - 1) : (chain.Count() - 1);
			for (int c = ((cell_from > 0) ? cell_from : 0); c <= cell_to; ++c) route.Cells.Add(chain[c]);
			for (int w = 0; w < edge.Waypoints.Count(); ++w) route.Waypoints.Add(edge.Waypoints[w]);
			for (int c = 0; c < spans.Count(); ++c) {
				if ((stations[spans[c].FirstPoint] < from_station) || (stations[spans[c].LastPoint] > split_station[s + 1])) continue;
				HybridRouteStructuralSpan span = spans[c];
				span.FirstPoint	= span.FirstPoint - from;
				span.LastPoint		= span.LastPoint - from;
				route.Structural.Add(span);
			}

			Vector2 minimum_point = route.Points[0];
			Vector2 maximum_point = route.Points[0];
			for (int i = 1; i < route.Points.Count(); ++i) {
				minimum_point.Set(Min2(minimum_point.X,route.Points[i].X),Min2(minimum_point.Y,route.Points[i].Y));
				maximum_point.Set(Max2(maximum_point.X,route.Points[i].X),Max2(maximum_point.Y,route.Points[i].Y));
			}
			route.BoundsMin = minimum_point;
			route.BoundsMax = maximum_point;

			if (envelope[s] > 0) b.Out->WidenedRoutes++;
			b.Out->Routes.Add(route);
		}

		//	The physical trunk itself (Q-025), which the logical slices are cut from.
		if (trunk) {
			HybridTrunkRoad record;
			record.StableId		= (unsigned int)(b.Out->Trunks.Count() + 1);
			record.RouteFamily	= lead.RouteFamily;
			record.RoadClass		= road_class;
			record.Capabilities	= lead.Capabilities;
			record.Length			= (stations.Count() > 0) ? stations[stations.Count() - 1] : 0.0f;
			for (int s = 0; s < segment_count; ++s) {
				record.EdgeIds.Add(segments[s]->StableId);
				record.EdgeFrom.Add(split_station[s]);
				record.EdgeTo.Add(split_station[s + 1]);
			}
			for (int n = 0; n < stop_node.Count(); ++n) {
				if (stop_node[n] == 0) continue;					// a promoted crossing's approach point
				if ((record.Nodes.Count() > 0) && (record.Nodes[record.Nodes.Count() - 1] == stop_node[n])) continue;
				record.Nodes.Add(stop_node[n]);
			}
			for (int n = 0; n < record.Nodes.Count(); ++n) {
				if (Node_Is_Structural(b,record.Nodes[n])) continue;
				bool joined = false;
				for (int e = 0; (e < b.Topology->Edges.Count()) && !joined; ++e) {
					const HybridTopologyEdge & other = b.Topology->Edges[e];
					if (!other.Is_Realized()) continue;
					bool own = false;
					for (int s = 0; (s < segment_count) && !own; ++s) own = (segments[s]->StableId == other.StableId);
					if (own) continue;
					if ((other.NodeA == record.Nodes[n]) || (other.NodeB == record.Nodes[n])) joined = true;
					for (int w = 0; (w < other.Waypoints.Count()) && !joined; ++w) joined = (other.Waypoints[w] == record.Nodes[n]);
				}
				if (joined) record.JunctionNodes.Add(record.Nodes[n]);
			}
			for (int i = 0; i < points.Count(); ++i) {
				record.Points.Add(points[i]);
				record.Stations.Add(stations[i]);
			}
			for (int c = 0; c < spans.Count(); ++c) record.Structural.Add(spans[c]);
			b.Out->Trunks.Add(record);
		}
		return true;
	}

	//	No legal road: the logical edge it failed on fails the attempt, or is dropped when it is optional.
	const HybridTopologyEdge & edge = *segments[failed_segment];
	b.LastFailedEdge = edge.StableId;
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
	if (b.P->CollectCensus && (result != nullptr)) {
		HybridRouteFailureCensus & census = result->Census;
		census.Reset();
		census.EdgeId		= edge.StableId;
		census.RoadClass	= (int)road_class;
		census.EdgeKind	= (int)edge.Kind;
		census.Envelope	= envelope[failed_segment];
		census.Legs			= last_stop[failed_segment] - first_stop[failed_segment];
		for (int w = 0; w < edge.Waypoints.Count(); ++w) {
			if (Node_Is_Structural(b,edge.Waypoints[w])) census.StructuralWaypoints++;
		}
		census.Leg = (failed_leg >= 0) ? (failed_leg - first_stop[failed_segment]) : -1;
		if ((failed_leg >= 0) && ((failed_leg + 1) < stop_node.Count())) {
			census.LegFrom				= stop_node[failed_leg];
			census.LegTo				= stop_node[failed_leg + 1];
			census.LegFromStructural= Node_Is_Structural(b,census.LegFrom);
			census.LegToStructural	= Node_Is_Structural(b,census.LegTo);
			int best = HYBRID_BLOCK_NONE;
			for (int i = 0; i < HYBRID_BLOCK_COUNT; ++i) {
				census.Frontier[i] = failed_frontier[i];
				if ((i != HYBRID_BLOCK_NONE) && (failed_frontier[i] > ((best == HYBRID_BLOCK_NONE) ? 0 : failed_frontier[best]))) best = i;
			}
			census.Reason = (failed_frontier[HYBRID_BLOCK_BUDGET] > 0) ? HYBRID_BLOCK_BUDGET : best;
		} else {
			census.Reason			= HYBRID_BLOCK_SPLINE;
			census.SplineReason	= spline_reason;
			census.SplineBlock	= Spline_Reason_Block(spline_reason,b.LastRadiusWindow);

			//	The exact geometric condition (Q-025's classification of centerline refusals).
			for (int j = 0; j < b.JointPoints.Count(); ++j) {
				const float distance = (b.JointPoints[j] - last_failure_point).Length();
				if ((census.SplineJointDistance < 0.0f) || (distance < census.SplineJointDistance)) census.SplineJointDistance = distance;
			}
			const char * why = (spline_reason != nullptr) ? spline_reason : "";
			const float class_radius = Hybrid_Road_Class_Spec(road_class).MinTurnRadiusW * k.W;
			if ((::strcmp(why,"a frozen point is left at a corner its leads could not straighten") == 0) ||
						  (::strcmp(why,"frozen points closer than the class turn radius allows") == 0) ||
						  (::strcmp(why,"a lead was sized for a corner that has been dropped") == 0)) {
				census.SplineClass = HYBRID_SPLINE_FAILURE_STRUCTURAL;
				census.StructuralCause = Structural_Cause(b,last_corner,stops,stop_window,stop_axis,stop_node,last_chain,
																		last_crossings,road_class);
			} else if (::strcmp(why,"curvature under the class minimum") == 0) {
				census.SplineClass = (b.LastRadius >= (0.99f * class_radius)) ? HYBRID_SPLINE_FAILURE_QUANTIZATION
																							 : HYBRID_SPLINE_FAILURE_MIN_RADIUS;
			} else if ((::strcmp(why,"the corridor reverses on itself") == 0) ||
						  (::strcmp(why,"the corridor could not be filleted") == 0) ||
						  (::strcmp(why,"corridor collapsed to a point") == 0) ||
						  (::strcmp(why,"degenerate centerline") == 0)) {
				census.SplineClass = HYBRID_SPLINE_FAILURE_CONTROL_FILLET;
			} else {
				census.SplineClass = HYBRID_SPLINE_FAILURE_GROUND;
			}
		}
	}
	return Fail(result,HYBRID_ROUTE_NO_SPLINE,detail.Peek_Buffer());
}


//	A heavy family's trunk edges in travel order: from the one nothing in the family leads into, end to end.
static void Family_Trunk_Edges(const HybridTopology & topology,int family,
										 DynamicVectorClass<const HybridTopologyEdge *> * out)
{
	out->Delete_All();
	DynamicVectorClass<const HybridTopologyEdge *> pool;
	for (int e = 0; e < topology.Edges.Count(); ++e) {
		const HybridTopologyEdge & edge = topology.Edges[e];
		if ((edge.Kind == HYBRID_EDGE_FAMILY_TRUNK) && (edge.RouteFamily == family)) pool.Add(&edge);
	}
	const HybridTopologyEdge * current = nullptr;
	for (int i = 0; (i < pool.Count()) && (current == nullptr); ++i) {
		bool led_into = false;
		for (int j = 0; (j < pool.Count()) && !led_into; ++j) led_into = (pool[j]->NodeB == pool[i]->NodeA);
		if (!led_into) current = pool[i];
	}
	while ((current != nullptr) && (out->Count() < pool.Count())) {
		out->Add(current);
		const HybridTopologyEdge * next = nullptr;
		for (int i = 0; (i < pool.Count()) && (next == nullptr); ++i) {
			if (pool[i]->NodeA == current->NodeB) next = pool[i];
		}
		current = next;
	}
}


/***********************************************************************************************
**	The Phase 10 gate.
***********************************************************************************************/

/***********************************************************************************************
**	Section 12.6.9: routing group 1b -- realizing Phase 9's frozen interchanges on the routed highway.
**
**	Phase 10 never moves a gore, swaps a side or picks another candidate: it realizes the frozen interchange or
**	withdraws it to its frozen Phase-9 fallback variant (item 3), the only fallback the plan allows.
***********************************************************************************************/

//	The side frame of 12.6.7: tau_RIGHT = a, tau_LEFT = -a; n_s is to the right of tau_s.
static Vector2 Interchange_World(const Vector2 & x_point,const Vector2 & axis,int side,const Vector2 & local)
{
	const Vector2 tau = (side == HYBRID_INTERCHANGE_SIDE_RIGHT) ? axis : Vector2(-axis.X,-axis.Y);
	const Vector2 normal = (side == HYBRID_INTERCHANGE_SIDE_RIGHT) ? Vector2(axis.Y,-axis.X) : Vector2(-axis.Y,axis.X);
	return x_point + (tau * local.X) + (normal * local.Y);
}


void Hybrid_Interchange_Footprint(const HybridConstants & k,const HybridInterchange & ic,const Vector2 & x_point,int side,
											 DynamicVectorClass<Vector2> * polygon)
{
	polygon->Delete_All();
	const float spacing = Min2(k.W / 3.0f,0.25f * k.A);
	HybridInterchangeConstants c;
	Hybrid_Interchange_Constants(k.W,&c);
	HybridRampCandidate cand;
	Hybrid_Ramp_Candidate(c,ic.SelectedK,0.0f,&cand);
	cand.LocalOffset				= ic.LocalOffset;
	cand.GoreOffset				= ic.GoreOffset;
	cand.StraightHalfExtent		= ic.StraightHalfExtent;

	DynamicVectorClass<Vector2> local;
	local.Add(Vector2(-ic.GoreOffset - c.DivergeLength,0.0f));
	for (float along = 0.0f; along <= c.DivergeLength; along += spacing) {
		HybridAuxiliaryLaneSection section;
		Hybrid_Auxiliary_Lane_Section(c,HYBRID_RAMP_ROLE_OFF,along,&section);
		local.Add(Vector2(-ic.GoreOffset - c.DivergeLength + along,section.D));
	}
	Vector2 control[4];
	DynamicVectorClass<Vector2> ramp_points;
	DynamicVectorClass<float> ramp_stations;
	Hybrid_Ramp_Control_Polygon(c,cand,HYBRID_RAMP_ROLE_OFF,control);
	if (Hybrid_Ramp_Centerline(control,c.RampRadius,k.Q,spacing,&ramp_points,&ramp_stations)) {
		for (int i = 0; i < ramp_points.Count(); ++i) local.Add(ramp_points[i]);
	}
	ramp_points.Delete_All();
	ramp_stations.Delete_All();
	Hybrid_Ramp_Control_Polygon(c,cand,HYBRID_RAMP_ROLE_ON,control);
	if (Hybrid_Ramp_Centerline(control,c.RampRadius,k.Q,spacing,&ramp_points,&ramp_stations)) {
		for (int i = 0; i < ramp_points.Count(); ++i) local.Add(ramp_points[i]);
	}
	for (float along = 0.0f; along <= c.MergeLength; along += spacing) {
		HybridAuxiliaryLaneSection section;
		Hybrid_Auxiliary_Lane_Section(c,HYBRID_RAMP_ROLE_ON,along,&section);
		local.Add(Vector2(ic.GoreOffset + along,section.D));
	}
	local.Add(Vector2(ic.GoreOffset + c.MergeLength,0.0f));
	for (int i = 0; i < local.Count(); ++i) polygon->Add(Interchange_World(x_point,ic.Axis,side,local[i]));
}


//	Is the routed highway straight along `axis` over [X - extent, X + extent], through X?
static bool Highway_Is_Aligned(const DynamicVectorClass<Vector2> & points,const Vector2 & x_point,const Vector2 & axis,
										 float extent,float q,float spacing,StringClass * why)
{
	int at = -1;
	float best = 3.4e38f;
	for (int i = 0; i < points.Count(); ++i) {
		const float distance = (points[i] - x_point).Length();
		if (distance >= best) continue;
		best	= distance;
		at		= i;
	}
	if ((at < 0) || (best > (spacing + (2.0f * q)))) {
		if (why != nullptr) why->Format("the routed highway misses X by %.3f",best);
		return false;
	}
	const Vector2 travel = Unit(points[((at + 1) < points.Count()) ? (at + 1) : at] - points[(at > 0) ? (at - 1) : at]);
	const Vector2 along = (((travel.X * axis.X) + (travel.Y * axis.Y)) >= 0.0f) ? axis : Vector2(-axis.X,-axis.Y);
	const Vector2 offset = points[at] - x_point;
	const float across = (float)fabs((double)((offset.X * along.Y) - (offset.Y * along.X)));
	if (across > (2.0f * q)) {
		if (why != nullptr) why->Format("the routed highway passes %.3f off X",across);
		return false;
	}
	float behind = 0.0f;
	float ahead = 0.0f;
	Hybrid_Polyline_Straight_Run(points,at,along,q,&behind,&ahead);
	const float shift = (offset.X * along.X) + (offset.Y * along.Y);
	if (((behind + shift) < (extent - spacing)) || ((ahead - shift) < (extent - spacing))) {
		if (why != nullptr) why->Format("the routed highway is straight only %.3f behind and %.3f ahead of X, %.3f needed",
												  behind + shift,ahead - shift,extent);
		return false;
	}
	return true;
}


//	The shortest distance from `point` to a polyline.
static float Polyline_Distance(const DynamicVectorClass<Vector2> & points,const Vector2 & point)
{
	float best = 3.4e38f;
	for (int i = 1; i < points.Count(); ++i) {
		const Vector2 d = points[i] - points[i - 1];
		const float length2 = (d.X * d.X) + (d.Y * d.Y);
		float t_on = (length2 > 0.0f) ? ((((point.X - points[i - 1].X) * d.X) + ((point.Y - points[i - 1].Y) * d.Y)) / length2) : 0.0f;
		t_on = Clamp2(t_on,0.0f,1.0f);
		best = Min2(best,(point - (points[i - 1] + (d * t_on))).Length());
	}
	return best;
}


/*
**	One constructed road's samples against 12.6.8's exclusions on the realized world, its class grade, and every
**	already routed group-1 roadbed.  `own_highway` (a HIGHWAY_4 logical edge pair) may be touched only where
**	`allow_own` says so -- a ramp inside its converged gore wedge, a crossing at X.  `inner_edge`/`outer_edge` are
**	the roadbed edges about the reference spline (left of travel negative).
*/
struct ConstructedCheck
{
	unsigned int	OwnIn;
	unsigned int	OwnOut;
	Vector2			X;
	Vector2			Axis;
	int				Side;
	float				OwnAlong;				// a crossing may touch its own highway within this of X along it
	bool				IsRamp;
	bool				Reversed;				// ramp: the samples run against its traffic (an OFF_RAMP stored L_s -> gore)
	float				Extra;					// ramp: D-024's E
	float				GoreStation;			// ramp: the station of its gore along the samples
	HybridInterchangeConstants	C;
};


static bool Constructed_Road_Is_Legal(RouteBuilder & b,const DynamicVectorClass<Vector2> & points,
												  const DynamicVectorClass<float> & stations,HybridRoadClass road_class,
												  const ConstructedCheck & check,StringClass * why)
{
	const HybridConstants & k = b.P->Constants;
	const float max_grade = Hybrid_Road_Class_Spec(road_class).MaxGradeDegrees * PI_OVER_180;
	const float inner_offset = -Hybrid_Road_Inner_Edge_Offset(road_class,k.W);
	const float outer_offset = Hybrid_Road_Outer_Edge_Offset(road_class,k.W);
	HybridVerticalInterval previous;

	/*
	**	A ramp's roadbed is joined to its own highway's from the gore (its last sample) until its inner edge clears the
	**	highway roadbed -- the converged gore wedge, which is one paved surface.  From the first clear sample on, the two
	**	roadbeds may not meet again.  The inner edge is the effective one (D-024), so a wider ramp stays joined longer.
	*/
	int joined_from = points.Count();
	if (check.IsRamp) {
		const float half_highway = 0.5f * Hybrid_Roadbed_Width(HYBRID_ROAD_HIGHWAY_4,k.W);
		for (int i = points.Count() - 1; i >= 0; --i) {
			float inner = 0.0f;
			float outer = 0.0f;
			Hybrid_Ramp_Effective_Edges(check.C,check.Extra,(float)fabs((double)(stations[i] - check.GoreStation)),&inner,&outer);
			bool joined = false;
			for (int o = 0; (o < b.Out->Routes.Count()) && !joined; ++o) {
				const HybridRoute & own = b.Out->Routes[o];
				if ((own.EdgeId != check.OwnIn) && (own.EdgeId != check.OwnOut)) continue;
				joined = (Polyline_Distance(own.Points,points[i]) < (half_highway + inner));
			}
			if (!joined) break;
			joined_from = i;
		}
	}

	for (int i = 0; i < points.Count(); ++i) {

		const int q = ((i + 1) < points.Count()) ? (i + 1) : i;
		const int r = ((i + 1) < points.Count()) ? i : (i - 1);
		const Vector2 travel = Unit(points[q] - points[r]);
		Vector2 left(-travel.Y,travel.X);
		float inner = inner_offset;
		float outer = outer_offset;
		if (check.IsRamp) {
			//	The inner edge is left of the ramp's *traffic* (D-022), which an OFF_RAMP's stored order runs against.
			if (check.Reversed) left = -left;
			Hybrid_Ramp_Effective_Edges(check.C,check.Extra,(float)fabs((double)(stations[i] - check.GoreStation)),&inner,&outer);
		}
		const Vector2 probes[3] = { points[i],points[i] + (left * inner),points[i] - (left * outer) };
		for (int p = 0; p < 3; ++p) {
			if ((Classify_Point(b,probes[p]) != 0) || Hybrid_Point_In_Water(*b.Hydrology,probes[p]) ||
				 Hybrid_Point_In_Immutable_Landform(*b.Landforms,probes[p])) {
				if (why != nullptr) why->Format("sample %d stands on refused ground (%.1f,%.1f)",i,probes[p].X,probes[p].Y);
				return false;
			}
		}
		if ((i > 0) && Segment_Meets_River(b,points[i - 1],points[i],nullptr)) {
			if (why != nullptr) why->Format("step %d crosses a river; a constructed road never has its own bridge",i);
			return false;
		}

		float z = 0.0f;
		if (!Height_At(b,points[i],&z)) {
			if (why != nullptr) why->Format("sample %d has no ground",i);
			return false;
		}
		const HybridVerticalInterval interval = Hybrid_Road_Vertical_Interval(z,b.P->MaxCutDepth,b.P->MaxFillHeight);
		if ((i > 0) && !Hybrid_Grade_Compatible(previous,interval,stations[i] - stations[i - 1],max_grade)) {
			if (why != nullptr) why->Format("step %d breaks the class grade",i);
			return false;
		}
		previous = interval;

		//	Every routed group-1 roadbed, its own highway only where the construction meets it.
		for (int o = 0; o < b.Out->Routes.Count(); ++o) {
			const HybridRoute & other = b.Out->Routes[o];
			if (other.Priority != HYBRID_ROUTE_PRIORITY_FAMILY_TRUNK) continue;
			const bool own = (other.EdgeId == check.OwnIn) || (other.EdgeId == check.OwnOut);
			float facing = Max2(inner,outer);
			if (own && check.IsRamp) {
				if (i >= joined_from) continue;
				facing = inner;														// the inner edge faces its own highway
			}
			if (own && !check.IsRamp) {
				const Vector2 offset = points[i] - check.X;
				const Vector2 normal = (check.Side == HYBRID_INTERCHANGE_SIDE_RIGHT) ? Vector2(check.Axis.Y,-check.Axis.X)
																										  : Vector2(-check.Axis.Y,check.Axis.X);
				const float lateral = (offset.X * normal.X) + (offset.Y * normal.Y);
				if ((float)fabs((double)lateral) <= check.OwnAlong) continue;
			}
			const float clearance = (0.5f * Hybrid_Roadbed_Width(other.RoadClass,k.W)) + facing;
			if (Polyline_Distance(other.Points,points[i]) < clearance) {
				if (why != nullptr) why->Format("sample %d meets the roadbed of group-1 road %u",i,other.EdgeId);
				return false;
			}
		}
	}
	return true;
}


//	The route record of a constructed road: samples from its NodeA to its NodeB, and the cells they stand in.
static void Constructed_Route(RouteBuilder & b,const HybridTopologyEdge & edge,const DynamicVectorClass<Vector2> & points,
										HybridRoute * out)
{
	out->EdgeId			= edge.StableId;
	out->RoadClass		= edge.RoadClass;
	out->Priority		= HYBRID_ROUTE_PRIORITY_INTERCHANGE;
	out->RouteFamily	= -1;
	out->Envelope		= 0;
	out->Required		= edge.Required;
	out->Capabilities	= edge.Capabilities;
	out->InterchangeId= edge.InterchangeId;
	out->RampRole		= edge.RampRole;
	out->RampSide		= edge.RampSide;
	out->RampInterval	= edge.RampInterval;
	out->RampRoadbedExtra= edge.RampRoadbedExtra;
	float station = 0.0f;
	for (int i = 0; i < points.Count(); ++i) {
		if (i > 0) station += (points[i] - points[i - 1]).Length();
		out->Points.Add(Vector2(Hybrid_Quantize_Position(points[i].X),Hybrid_Quantize_Position(points[i].Y)));
		out->Stations.Add(Hybrid_Quantize_Position(station));
		const int cell = b.Cell_Of(points[i]);
		if ((out->Cells.Count() == 0) || (out->Cells[out->Cells.Count() - 1] != cell)) out->Cells.Add(cell);
	}
	out->Length = Hybrid_Quantize_Position(station);
	for (int w = 0; w < edge.Waypoints.Count(); ++w) out->Waypoints.Add(edge.Waypoints[w]);
	Vector2 lo = (points.Count() > 0) ? points[0] : Vector2(0.0f,0.0f);
	Vector2 hi = lo;
	for (int i = 1; i < points.Count(); ++i) {
		lo.Set(Min2(lo.X,points[i].X),Min2(lo.Y,points[i].Y));
		hi.Set(Max2(hi.X,points[i].X),Max2(hi.Y,points[i].Y));
	}
	out->BoundsMin = lo;
	out->BoundsMax = hi;
}


//	A straight from `from` to `to`, sampled at `spacing`, both ends included.
static void Straight_Points(const Vector2 & from,const Vector2 & to,float spacing,DynamicVectorClass<Vector2> * points,
									 DynamicVectorClass<float> * stations)
{
	const float length = (to - from).Length();
	int steps = (int)::ceil((double)(length / Max2(spacing,1.0e-3f)));
	if (steps < 1) steps = 1;
	for (int i = 0; i <= steps; ++i) {
		const float t_on = (float)i / (float)steps;
		points->Add(from + ((to - from) * t_on));
		stations->Add(length * t_on);
	}
}


//	Withdraws an interchange (12.6.9 item 3): its constructed routes and widened intervals go, its arrivals end at X.
static void Withdraw_Interchange(RouteBuilder & b,unsigned int interchange_id)
{
	if (b.Out->Interchange_Withdrawn(interchange_id)) return;
	if (b.Result != nullptr) b.Result->WithdrawnInterchanges.Add(interchange_id);
	int slot = b.Out->WithdrawnInterchanges.Count();
	b.Out->WithdrawnInterchanges.Add(interchange_id);
	while ((slot > 0) && (b.Out->WithdrawnInterchanges[slot - 1] > interchange_id)) {
		b.Out->WithdrawnInterchanges[slot] = b.Out->WithdrawnInterchanges[slot - 1];
		slot--;
	}
	b.Out->WithdrawnInterchanges[slot] = interchange_id;

	DynamicVectorClass<HybridRoute> kept;
	for (int r = 0; r < b.Out->Routes.Count(); ++r) {
		HybridRoute route = b.Out->Routes[r];
		if (route.InterchangeId == interchange_id) continue;
		DynamicVectorClass<HybridWidenedInterval> widened;
		for (int w = 0; w < route.Widened.Count(); ++w) {
			if (route.Widened[w].InterchangeId != interchange_id) widened.Add(route.Widened[w]);
		}
		route.Widened.Delete_All();
		for (int w = 0; w < widened.Count(); ++w) route.Widened.Add(widened[w]);
		kept.Add(route);
	}
	b.Out->Routes.Delete_All();
	for (int r = 0; r < kept.Count(); ++r) b.Out->Routes.Add(kept[r]);
}


/*
**	Group 1b for one active interchange: the highway's alignment re-validated, then the crossing edge(s), then the
**	ramps RIGHT then LEFT, OFF then ON, each exactly by 12.6.7 on the routed highway and re-validated against the
**	realized world and every group-1 roadbed.  True with the routes added; false (nothing added) to withdraw.
*/
static bool Construct_Interchange(RouteBuilder & b,const HybridInterchange & ic,StringClass * why)
{
	const HybridConstants & k = b.P->Constants;
	const float spacing = Min2(k.W / 3.0f,0.25f * k.A);
	HybridInterchangeConstants c;
	Hybrid_Interchange_Constants(k.W,&c);
	const HybridTopologyNode * anchor = b.Topology->Find_Node(ic.AnchorNode);
	const HybridTrunkRoad * trunk = b.Out->Find_Trunk(ic.HighwayEdgeIn);
	if ((anchor == nullptr) || (trunk == nullptr)) {
		why->Format("interchange %u has no routed highway",ic.StableId);
		return false;
	}
	const Vector2 x_point = anchor->Position;
	if (!Highway_Is_Aligned(trunk->Points,x_point,ic.Axis,ic.StraightHalfExtent,k.Q,Min2(k.W,0.5f * k.A),why)) return false;

	HybridRampCandidate cand;
	Hybrid_Ramp_Candidate(c,ic.SelectedK,0.0f,&cand);
	cand.LocalOffset				= ic.LocalOffset;
	cand.GoreOffset				= ic.GoreOffset;
	cand.StraightHalfExtent		= ic.StraightHalfExtent;

	ConstructedCheck check;
	check.OwnIn		= ic.HighwayEdgeIn;
	check.OwnOut	= ic.HighwayEdgeOut;
	check.X			= x_point;
	check.Axis		= ic.Axis;
	check.C			= c;

	DynamicVectorClass<HybridRoute> built;

	//	The DIAMOND crossing: the straight L_RIGHT -> L_LEFT through X, one edge or two.
	if (ic.Form == HYBRID_INTERCHANGE_DIAMOND_ACCESS) {
		const Vector2 right = Interchange_World(x_point,ic.Axis,HYBRID_INTERCHANGE_SIDE_RIGHT,Vector2(0.0f,ic.LocalOffset));
		const Vector2 left = Interchange_World(x_point,ic.Axis,HYBRID_INTERCHANGE_SIDE_LEFT,Vector2(0.0f,ic.LocalOffset));
		for (int e = 0; e < ic.CrossingEdges.Count(); ++e) {
			const HybridTopologyEdge * edge = b.Topology->Find_Edge(ic.CrossingEdges[e]);
			if (edge == nullptr) continue;
			Vector2 from = right;
			Vector2 to = left;
			if (ic.CrossingEdges.Count() == 2) {
				from	= (e == 0) ? right : x_point;
				to		= (e == 0) ? x_point : left;
			}
			DynamicVectorClass<Vector2> points;
			DynamicVectorClass<float> stations;
			Straight_Points(from,to,spacing,&points,&stations);
			check.Side		= HYBRID_INTERCHANGE_SIDE_RIGHT;
			check.IsRamp	= false;
			check.Reversed	= false;
			check.Extra		= 0.0f;
			check.OwnAlong	= c.EHwy + (0.5f * Hybrid_Roadbed_Width(edge->RoadClass,k.W)) +
								  ((ic.CrossingMode == HYBRID_INTERCHANGE_CROSSING_GRADE_SEPARATED) ? (ic.LocalOffset - c.EHwy) : 0.0f);
			check.GoreStation= 0.0f;
			if (!Constructed_Road_Is_Legal(b,points,stations,edge->RoadClass,check,why)) {
				StringClass inner = *why;
				why->Format("interchange %u crossing edge %u: %s",ic.StableId,edge->StableId,inner.Peek_Buffer());
				return false;
			}
			HybridRoute route;
			Constructed_Route(b,*edge,points,&route);
			built.Add(route);
		}
	}

	//	The ramps, RIGHT then LEFT, OFF then ON -- the order Phase 9 created them in.
	for (int e = 0; e < ic.RampEdges.Count(); ++e) {
		const HybridTopologyEdge * edge = b.Topology->Find_Edge(ic.RampEdges[e]);
		if (edge == nullptr) continue;
		Vector2 polygon[4];
		Hybrid_Ramp_Control_Polygon(c,cand,edge->RampRole,polygon);
		DynamicVectorClass<Vector2> local_points;
		DynamicVectorClass<float> local_stations;
		if (!Hybrid_Ramp_Centerline(polygon,c.RampRadius,k.Q,spacing,&local_points,&local_stations)) {
			why->Format("interchange %u ramp %u cannot be built at R_r",ic.StableId,edge->StableId);
			return false;
		}

		//	World samples from L_s (NodeA) to the gore: an OFF_RAMP is built gore -> L_s and is reversed.
		DynamicVectorClass<Vector2> points;
		DynamicVectorClass<float> stations;
		const int count = local_points.Count();
		const float total = local_stations[count - 1];
		for (int i = 0; i < count; ++i) {
			const int source = (edge->RampRole == HYBRID_RAMP_ROLE_OFF) ? (count - 1 - i) : i;
			points.Add(Interchange_World(x_point,ic.Axis,edge->RampSide,local_points[source]));
			stations.Add((edge->RampRole == HYBRID_RAMP_ROLE_OFF) ? (total - local_stations[source]) : local_stations[source]);
		}

		//	The gore, exactly where Phase 9 froze it.
		const Vector2 gore = Interchange_World(x_point,ic.Axis,edge->RampSide,
															Vector2((edge->RampRole == HYBRID_RAMP_ROLE_OFF) ? -ic.GoreOffset : ic.GoreOffset,c.DAux));
		if ((points[count - 1] - gore).Length() > (2.0f * k.Q)) {
			why->Format("interchange %u ramp %u ends %.3f from its frozen gore",ic.StableId,edge->StableId,
							(points[count - 1] - gore).Length());
			return false;
		}

		check.Side			= edge->RampSide;
		check.IsRamp		= true;
		check.Reversed		= (edge->RampRole == HYBRID_RAMP_ROLE_OFF);
		check.Extra			= edge->RampRoadbedExtra;
		check.OwnAlong		= 0.0f;
		check.GoreStation	= stations[count - 1];
		if (!Constructed_Road_Is_Legal(b,points,stations,HYBRID_ROAD_HIGHWAY_RAMP,check,why)) {
			StringClass inner = *why;
			why->Format("interchange %u ramp %u: %s",ic.StableId,edge->StableId,inner.Peek_Buffer());
			return false;
		}
		HybridRoute route;
		Constructed_Route(b,*edge,points,&route);
		built.Add(route);
	}

	//	Realized: the routes, the highway's widened intervals, and the footprint every later road keeps out of.
	for (int r = 0; r < built.Count(); ++r) b.Out->Routes.Add(built[r]);
	for (int e = 0; e < ic.RampEdges.Count(); ++e) {
		const HybridTopologyEdge * ramp = b.Topology->Find_Edge(ic.RampEdges[e]);
		if (ramp == nullptr) continue;
		const unsigned int highway = (ramp->RampInterval.From >= 0.0f) ? ic.HighwayEdgeOut : ic.HighwayEdgeIn;
		for (int r = 0; r < b.Out->Routes.Count(); ++r) {
			if (b.Out->Routes[r].EdgeId != highway) continue;
			HybridWidenedInterval widened;
			widened.InterchangeId	= ic.StableId;
			widened.RampEdgeId		= ramp->StableId;
			widened.Role				= ramp->RampRole;
			widened.Side				= ramp->RampSide;
			widened.Interval			= ramp->RampInterval;
			b.Out->Routes[r].Widened.Add(widened);
		}
	}
	for (int side = 0; side < HYBRID_INTERCHANGE_SIDE_COUNT; ++side) {
		if ((ic.AccessSides & (1u << side)) == 0) continue;
		DynamicVectorClass<Vector2> footprint;
		Hybrid_Interchange_Footprint(k,ic,x_point,side,&footprint);
		b.FootprintStart.Add(b.FootprintPoints.Count());
		for (int i = 0; i < footprint.Count(); ++i) b.FootprintPoints.Add(footprint[i]);
		b.FootprintCount.Add(footprint.Count());
	}
	return true;
}


static void Construct_Interchanges(RouteBuilder & b)
{
	for (int i = 0; i < b.Topology->Interchanges.Count(); ++i) {
		const HybridInterchange & ic = b.Topology->Interchanges[i];
		if ((ic.Status != HYBRID_INTERCHANGE_ACTIVE) || b.Out->Interchange_Withdrawn(ic.StableId)) continue;
		StringClass why;
		if (Construct_Interchange(b,ic,&why)) {
			if (b.Result != nullptr) b.Result->RealizedInterchanges.Add(ic.StableId);
			continue;
		}
		WWDEBUG_SAY(("Hybrid Phase 10: interchange %u withdrawn in group 1b: %s\r\n",ic.StableId,why.Peek_Buffer()));
		Withdraw_Interchange(b,ic.StableId);
	}
}


/*
**	The tightest turn of a sampled road, measured as the route validator measures curvature: the circle through
**	three samples `baseline` apart, so the position quantum the samples were written to is not read as a turn.
*/
static float Sampled_Tightest_Radius(const DynamicVectorClass<Vector2> & points,int baseline)
{
	float tightest = 3.4e38f;
	const int step = (baseline > 1) ? baseline : 1;
	for (int i = step; (i + step) < points.Count(); ++i) {
		const Vector2 p = points[i - step];
		const Vector2 q = points[i];
		const Vector2 r = points[i + step];
		const float cross = ((q.X - p.X) * (r.Y - p.Y)) - ((q.Y - p.Y) * (r.X - p.X));
		const float a = (q - p).Length();
		const float bb = (r - q).Length();
		const float cc = (r - p).Length();
		if ((float)fabs((double)cross) <= (1.0e-4f * a * cc)) continue;
		tightest = Min2(tightest,(a * bb * cc) / (2.0f * (float)fabs((double)cross)));
	}
	return tightest;
}


/*
**	The Phase-10 gate's interchange clause (12.6.9): every active interchange is realized exactly as frozen -- the
**	highway straight through X over its extent, every ramp from its local junction to its frozen gore at d_aux with
**	the highway tangent and no turn under R_r, every crossing edge routed -- and no road routed after group 1b
**	enters an active footprint outside its own end cells.
*/
static bool Check_Interchanges_Gate(RouteBuilder & b,StringClass * detail)
{
	const HybridConstants & k = b.P->Constants;
	HybridInterchangeConstants c;
	Hybrid_Interchange_Constants(k.W,&c);

	for (int i = 0; i < b.Topology->Interchanges.Count(); ++i) {

		const HybridInterchange & ic = b.Topology->Interchanges[i];
		if ((ic.Status != HYBRID_INTERCHANGE_ACTIVE) || b.Out->Interchange_Withdrawn(ic.StableId)) continue;
		const HybridTopologyNode * anchor = b.Topology->Find_Node(ic.AnchorNode);
		const HybridTrunkRoad * trunk = b.Out->Find_Trunk(ic.HighwayEdgeIn);
		if ((anchor == nullptr) || (trunk == nullptr) ||
			 !Highway_Is_Aligned(trunk->Points,anchor->Position,ic.Axis,ic.StraightHalfExtent,k.Q,Min2(k.W,0.5f * k.A),detail)) {
			StringClass inner = *detail;
			detail->Format("interchange %u: %s",ic.StableId,inner.Peek_Buffer());
			return false;
		}
		for (int e = 0; e < ic.CrossingEdges.Count(); ++e) {
			if (b.Out->Find(ic.CrossingEdges[e]) != nullptr) continue;
			detail->Format("interchange %u: crossing edge %u has no route",ic.StableId,ic.CrossingEdges[e]);
			return false;
		}
		for (int e = 0; e < ic.RampEdges.Count(); ++e) {
			const HybridTopologyEdge * edge = b.Topology->Find_Edge(ic.RampEdges[e]);
			const HybridRoute * route = b.Out->Find(ic.RampEdges[e]);
			const HybridTopologyNode * local = (edge != nullptr) ? b.Topology->Find_Node(edge->NodeA) : nullptr;
			if ((edge == nullptr) || (route == nullptr) || (local == nullptr) || (route->Points.Count() < 3)) {
				detail->Format("interchange %u: ramp %u has no route",ic.StableId,ic.RampEdges[e]);
				return false;
			}
			const int last = route->Points.Count() - 1;
			const Vector2 gore = Interchange_World(anchor->Position,ic.Axis,edge->RampSide,
																Vector2((edge->RampRole == HYBRID_RAMP_ROLE_OFF) ? -ic.GoreOffset : ic.GoreOffset,c.DAux));
			if (((route->Points[0] - local->Position).Length() > (2.0f * k.Q)) || ((route->Points[last] - gore).Length() > (2.0f * k.Q))) {
				detail->Format("interchange %u: ramp %u does not run from its local junction to its frozen gore",ic.StableId,edge->StableId);
				return false;
			}
			//	The arc's tangent at the gore (the circle through the last three samples), against the highway axis.
			const Vector2 p0 = route->Points[last - 2];
			const Vector2 p1 = route->Points[last - 1];
			const float d = 2.0f * ((p0.X * (p1.Y - gore.Y)) + (p1.X * (gore.Y - p0.Y)) + (gore.X * (p0.Y - p1.Y)));
			float heading_error = 0.0f;
			if ((float)fabs((double)d) > 1.0e-6f) {
				const float a2 = (p0.X * p0.X) + (p0.Y * p0.Y);
				const float b2 = (p1.X * p1.X) + (p1.Y * p1.Y);
				const float g2 = (gore.X * gore.X) + (gore.Y * gore.Y);
				const Vector2 centre(((a2 * (p1.Y - gore.Y)) + (b2 * (gore.Y - p0.Y)) + (g2 * (p0.Y - p1.Y))) / d,
											((a2 * (gore.X - p1.X)) + (b2 * (p0.X - gore.X)) + (g2 * (p1.X - p0.X))) / d);
				const Vector2 radial = Unit(gore - centre);
				heading_error = (float)fabs((double)((radial.X * ic.Axis.X) + (radial.Y * ic.Axis.Y)));
				heading_error = (float)asin((double)Min2(heading_error,1.0f));
			} else {
				const Vector2 chord = Unit(gore - p1);
				const float cross = (float)fabs((double)((chord.X * ic.Axis.Y) - (chord.Y * ic.Axis.X)));
				heading_error = (float)asin((double)Min2(cross,1.0f));
			}
			if (heading_error > 0.001f) {
				detail->Format("interchange %u: ramp %u meets its gore %.5f rad off the highway tangent",ic.StableId,edge->StableId,
									heading_error);
				return false;
			}
			const float spacing = Min2(k.W / 3.0f,0.25f * k.A);
			const int baseline = (int)Hybrid_Round_Half_Up(c.RampRadius / Max2(spacing,k.Q));
			if (Sampled_Tightest_Radius(route->Points,baseline) < (0.99f * c.RampRadius)) {
				detail->Format("interchange %u: ramp %u turns under R_r",ic.StableId,edge->StableId);
				return false;
			}
		}
	}

	//	The footprints, against every road routed after group 1b.
	for (int r = 0; r < b.Out->Routes.Count(); ++r) {
		const HybridRoute & route = b.Out->Routes[r];
		if ((route.Priority == HYBRID_ROUTE_PRIORITY_FAMILY_TRUNK) || (route.Priority == HYBRID_ROUTE_PRIORITY_INTERCHANGE)) continue;
		const int first_cell = (route.Cells.Count() > 0) ? route.Cells[0] : -1;
		const int last_cell = (route.Cells.Count() > 0) ? route.Cells[route.Cells.Count() - 1] : -1;
		for (int p = 0; p < route.Points.Count(); ++p) {
			const int cell = b.Cell_Of(route.Points[p]);
			if ((cell == first_cell) || (cell == last_cell)) continue;
			for (int f = 0; f < b.FootprintStart.Count(); ++f) {
				if (!Point_In_Footprint(b,f,route.Points[p])) continue;
				detail->Format("route %u enters an interchange footprint at (%.1f,%.1f)",route.EdgeId,route.Points[p].X,
									route.Points[p].Y);
				return false;
			}
		}
	}
	return true;
}


static bool Check_Gate(RouteBuilder & b,HybridRouteResult * result)
{
	StringClass detail;

	for (int e = 0; e < b.Topology->Edges.Count(); ++e) {

		const HybridTopologyEdge & edge = b.Topology->Edges[e];
		if (!edge.Is_Realized()) continue;
		if (Hybrid_Route_Edge_Withdrawn(*b.Topology,*b.Out,edge)) continue;		// 12.6.9: inactive after withdrawal

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

		/*
		**	Revised D-009 (Q-022): every structural interval is entered and left along its own axis, with the
		**	turn onto it finished on open road before its frozen approach point.  The interval itself is one
		**	straight, and the road keeps the lead straight beside each end unless the route begins or ends there.
		*/
		const HybridConstants & k = b.P->Constants;
		const float straight = Min2(k.W,0.5f * k.A);
		for (int s = 0; s < route->Structural.Count(); ++s) {

			const HybridRouteStructuralSpan & span = route->Structural[s];
			const Vector2 axis = Unit(span.Egress - span.Ingress);
			const int first = span.FirstPoint;
			const int last = span.LastPoint;
			if ((first < 0) || (last >= route->Points.Count()) || (last <= first)) {
				detail.Format("route %u's crossing of window %u has no interval",route->EdgeId,span.WindowId);
				return Fail(result,HYBRID_ROUTE_CROSSING_MISALIGNED,detail.Peek_Buffer());
			}

			float behind = 0.0f;
			float through = 0.0f;
			float ahead = 0.0f;
			Hybrid_Polyline_Straight_Run(route->Points,first,axis,k.Q,&behind,&through);
			Hybrid_Polyline_Straight_Run(route->Points,last,axis,k.Q,nullptr,&ahead);

			const float interval = route->Stations[last] - route->Stations[first];
			const float before_room = Min2(straight,route->Stations[first]);
			const float after_room = Min2(straight,route->Stations[route->Points.Count() - 1] - route->Stations[last]);
			const float slack = 2.0f * k.Q;
			if ((through < (interval - slack)) || (behind < (before_room - slack)) || (ahead < (after_room - slack))) {
				detail.Format("route %u is not straight through window %u along its axis: %.3f of %.3f straight "
								  "through the interval, %.3f of %.3f before it, %.3f of %.3f after it",route->EdgeId,
								  span.WindowId,through,interval,behind,before_room,ahead,after_room);
				return Fail(result,HYBRID_ROUTE_CROSSING_MISALIGNED,detail.Peek_Buffer());
			}
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
		result->RealizedInterchanges.Delete_All();
		result->WithdrawnInterchanges.Delete_All();
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
	b.CacheEdge		= nullptr;
	b.Generation	= 0;
	b.AlignedLeg	= false;
	b.LastFailedEdge= 0;
	b.Result			= result;
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
	b.ClosedWindow	= 0;
	b.OwnFieldSite	= 0;
	b.OwnEndFieldSite = 0;
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
			if ((edge.Kind == HYBRID_EDGE_HIGHWAY_RAMP) || (edge.Kind == HYBRID_EDGE_INTERCHANGE_CROSSING)) continue;
			if ((int)Hybrid_Route_Priority_Of(topology,edge) != priority) continue;

			/*
			**	A heavy family's trunk is one physical road (Q-025): routed once, when its first logical edge
			**	comes up, as all of the family's trunk edges in travel order -- each starts where the one before
			**	it ends.  Every other edge is a road of its own.
			*/
			DynamicVectorClass<const HybridTopologyEdge *> segments;
			if ((edge.Kind == HYBRID_EDGE_FAMILY_TRUNK) && (edge.RouteFamily >= 0)) {
				if (out->Find(edge.StableId) != nullptr) continue;
				bool seen = false;
				for (int r = 0; (r < out->Routes.Count()) && !seen; ++r) {
					seen = (out->Routes[r].Priority == HYBRID_ROUTE_PRIORITY_FAMILY_TRUNK) &&
							 (out->Routes[r].RouteFamily == edge.RouteFamily);
				}
				if (seen) continue;
				Family_Trunk_Edges(topology,edge.RouteFamily,&segments);
			} else {
				segments.Add(&edge);
			}

			/*
			**	12.6.9 item 3(a): a highway edge carrying interchange alignment that fails its third envelope withdraws
			**	the interchange at its ends that is later in processing order, and is routed again from its first
			**	envelope; when none remains it fails by the ordinary rule.  Only before group 1b starts.
			*/
			for (;;) {
				if (Route_Road(b,segments,result)) break;
				unsigned int withdraw = 0;
				const HybridTopologyEdge * failed = topology.Find_Edge(b.LastFailedEdge);
				const bool envelope_failure = (result == nullptr) || (result->Failure != HYBRID_ROUTE_EXPANSION_BUDGET);
				if ((priority == HYBRID_ROUTE_PRIORITY_FAMILY_TRUNK) && envelope_failure && (failed != nullptr)) {
					for (int i = 0; i < topology.Interchanges.Count(); ++i) {
						const HybridInterchange & ic = topology.Interchanges[i];
						if ((ic.Status != HYBRID_INTERCHANGE_ACTIVE) || out->Interchange_Withdrawn(ic.StableId)) continue;
						if ((ic.AnchorNode != failed->NodeA) && (ic.AnchorNode != failed->NodeB)) continue;
						if (ic.StableId > withdraw) withdraw = ic.StableId;
					}
				}
				/*
				**	No interchange left to withdraw: the road fails by the ordinary rule.  A withdrawal already made stands
				**	(INTERCHANGE_WITHDRAWN); the world's failure after it is a separate fact (Hybrid_Interchange_World_Outcome).
				*/
				if (withdraw == 0) { out->Reset(); return false; }
				WWDEBUG_SAY(("Hybrid Phase 10: interchange %u withdrawn, highway edge %u has no aligned route\r\n",withdraw,
								 b.LastFailedEdge));
				Withdraw_Interchange(b,withdraw);
				if (result != nullptr) {
					result->Failure	= HYBRID_ROUTE_OK;
					result->Detail		= "";
				}
			}
			if (params.Counters != nullptr) params.Counters->Add(HYBRID_WORK_ROUTED_EDGES,segments.Count());
		}

		//	Group 1b: every group-1 road is on the ground, so the interchanges are constructed on it.
		if (priority == HYBRID_ROUTE_PRIORITY_FAMILY_TRUNK) Construct_Interchanges(b);
	}

	if (!Check_Gate(b,result)) { out->Reset(); return false; }
	{
		StringClass detail;
		if (!Check_Interchanges_Gate(b,&detail)) {
			Fail(result,HYBRID_ROUTE_INTERCHANGE_INVALID,detail.Peek_Buffer());
			out->Reset();
			return false;
		}
	}

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
	b.OwnEntranceNodes.Add(edge->NodeA);
	b.OwnEntranceNodes.Add(edge->NodeB);
	b.OwnWindow		= 0;
	b.ClosedWindow	= 0;
	b.OwnFieldSite	= 0;
	b.OwnEndFieldSite = 0;
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
	b.OwnEndFieldSite = 0;
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
		**	within the class maximum, unless either end is structural (a deck carries it).  Cells
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


/***********************************************************************************************
**	The straight a sampled road keeps through one of its samples.
***********************************************************************************************/
void Hybrid_Polyline_Straight_Run(const DynamicVectorClass<Vector2> & points,int index,const Vector2 & axis,
											 float q,float * behind_out,float * ahead_out)
{
	if (behind_out != nullptr) *behind_out = 0.0f;
	if (ahead_out != nullptr) *ahead_out = 0.0f;
	const int count = points.Count();
	const Vector2 unit = Unit(axis);
	if ((index < 0) || (index >= count) || (unit.Length() <= 0.0f)) return;

	const Vector2 & origin = points[index];
	for (int side = 0; side < 2; ++side) {

		const int step = (side == 0) ? -1 : 1;
		const float sense = (side == 0) ? -1.0f : 1.0f;
		float reached = 0.0f;
		for (int i = index + step; (i >= 0) && (i < count); i += step) {
			const Vector2 offset = points[i] - origin;
			const float along = ((offset.X * unit.X) + (offset.Y * unit.Y)) * sense;
			const float across = (float)fabs((double)((offset.X * unit.Y) - (offset.Y * unit.X)));
			//	On the line: within the quantum each position was written to, and the angular quantum over the run.
			if (along <= reached) break;
			if (across > ((2.0f * q) + (along * HYBRID_QUANTUM_ANGLE))) break;
			reached = along;
		}
		if ((side == 0) && (behind_out != nullptr)) *behind_out = reached;
		if ((side == 1) && (ahead_out != nullptr)) *ahead_out = reached;
	}
}


/***********************************************************************************************
**	The realized crossing path -- see Hybrid_Build_Crossing_Path in the header.
***********************************************************************************************/

/*
**	The corner polyline of one side of a crossing path, before the crossing's own controls: where the road
**	leaves the preliminary centerline.  `index` is the route sample the road turns at.  The road has to be
**	running straight into that sample, because the arc that turns it off starts on the route's own straight.
**	Returns the vertices to put in front of the controls, or false when no departure can start there.
*/
static bool Departure_Vertices(const HybridRoute & route,int index,const Vector2 & control,const Vector2 & axis,
										 float straight,float q,DynamicVectorClass<RouteVertex> * out,int * start_out)
{
	out->Delete_All();
	*start_out = index;

	//	The route already stands on the control, heading along the axis: it simply continues onto it.
	if ((route.Points[index] - control).Length() <= q) {
		float behind = 0.0f;
		Hybrid_Polyline_Straight_Run(route.Points,index,axis,q,&behind,nullptr);
		if ((index > 0) && (behind < Min2(straight,route.Stations[index]) - (2.0f * q))) return false;
		RouteVertex start;
		start.P = route.Points[index];
		out->Add(start);
		return true;
	}

	//	The route's own start is a node, not a corner: the road may leave it on any heading.
	if (index == 0) {
		RouteVertex start;
		start.P = route.Points[0];
		out->Add(start);
		return true;
	}

	const Vector2 heading = Unit(route.Points[index] - route.Points[index - 1]);
	float behind = 0.0f;
	Hybrid_Polyline_Straight_Run(route.Points,index,heading,q,&behind,nullptr);
	if (behind <= q) return false;

	//	The earliest sample on that straight is where the path begins; it coincides with the route up to its arc.
	int first = index;
	while ((first > 0) && ((route.Stations[index] - route.Stations[first - 1]) <= (behind + q))) first--;
	if (first == index) return false;

	RouteVertex start;
	start.P = route.Points[first];
	out->Add(start);

	RouteVertex corner;
	corner.P			= route.Points[index];
	corner.Pinned	= true;
	out->Add(corner);
	*start_out = first;
	return true;
}


//	The same on the far side, in travel order after the controls.
static bool Rejoin_Vertices(const HybridRoute & route,int index,const Vector2 & control,const Vector2 & axis,
									 float straight,float q,DynamicVectorClass<RouteVertex> * out,int * end_out)
{
	out->Delete_All();
	*end_out = index;
	const int last = route.Points.Count() - 1;

	if ((route.Points[index] - control).Length() <= q) {
		float ahead = 0.0f;
		Hybrid_Polyline_Straight_Run(route.Points,index,axis,q,nullptr,&ahead);
		const float remaining = route.Stations[last] - route.Stations[index];
		if ((index < last) && (ahead < Min2(straight,remaining) - (2.0f * q))) return false;
		RouteVertex end;
		end.P = route.Points[index];
		out->Add(end);
		return true;
	}

	if (index == last) {
		RouteVertex end;
		end.P = route.Points[last];
		out->Add(end);
		return true;
	}

	const Vector2 heading = Unit(route.Points[index + 1] - route.Points[index]);
	float ahead = 0.0f;
	Hybrid_Polyline_Straight_Run(route.Points,index,heading,q,nullptr,&ahead);
	if (ahead <= q) return false;

	int final_index = index;
	while ((final_index < last) && ((route.Stations[final_index + 1] - route.Stations[index]) <= (ahead + q))) final_index++;
	if (final_index == index) return false;

	RouteVertex corner;
	corner.P			= route.Points[index];
	corner.Pinned	= true;
	out->Add(corner);

	RouteVertex end;
	end.P = route.Points[final_index];
	out->Add(end);
	*end_out = final_index;
	return true;
}


//	Appends a vertex, merging it into the last one when they coincide; the crossing's own vertex wins.
static void Append_Vertex(DynamicVectorClass<RouteVertex> * list,const RouteVertex & vertex,float q)
{
	if ((list->Count() > 0) && (((*list)[list->Count() - 1].P - vertex.P).Length() <= q)) {
		RouteVertex & last = (*list)[list->Count() - 1];
		if (vertex.Frozen || vertex.Pinned) {
			const Vector2 keep = last.P;
			last = vertex;
			last.P = keep;
		}
		return;
	}
	list->Add(vertex);
}


//	The crossing's own controls as corner-polyline vertices: both ends frozen on their axes, interior ones pinned.
static void Control_Vertices(const Vector2 * controls,int control_count,DynamicVectorClass<RouteVertex> * out)
{
	out->Delete_All();
	const Vector2 entry_axis = Unit(controls[1] - controls[0]);
	const Vector2 exit_axis = Unit(controls[control_count - 1] - controls[control_count - 2]);
	for (int c = 0; c < control_count; ++c) {
		RouteVertex vertex;
		vertex.P = controls[c];
		if (c == 0) {
			vertex.Frozen	= true;
			vertex.Axis		= entry_axis;
			vertex.WindowId= 1;
		} else if (c == (control_count - 1)) {
			vertex.Frozen	= true;
			vertex.Axis		= exit_axis;
			vertex.WindowId= (control_count == 2) ? 1 : 2;		// a two-point crossing is one straight frozen run
		} else {
			vertex.Pinned	= true;
		}
		out->Add(vertex);
	}
}


bool Hybrid_Build_Crossing_Path(const HybridRoute & route,int first_point,int last_point,
										 const Vector2 * controls,int control_count,float radius,float spacing,
										 float quantum,float max_reach,HybridCrossingPath * out,
										 const char ** reason_out)
{
	if (reason_out != nullptr) *reason_out = "";
	if (out == nullptr) return false;
	out->Reset();

	const int count = route.Points.Count();
	if ((controls == nullptr) || (control_count < 2) || (count < 2) || (route.Stations.Count() != count) ||
		 (first_point < 0) || (last_point >= count) || (last_point <= first_point) || !(radius > 0.0f) ||
		 !(spacing > 0.0f)) {
		if (reason_out != nullptr) *reason_out = "invalid crossing path input";
		return false;
	}

	const Vector2 entry_axis = Unit(controls[1] - controls[0]);
	const Vector2 exit_axis = Unit(controls[control_count - 1] - controls[control_count - 2]);
	if ((entry_axis.Length() <= 0.0f) || (exit_axis.Length() <= 0.0f)) {
		if (reason_out != nullptr) *reason_out = "the crossing's end controls coincide";
		return false;
	}

	DynamicVectorClass<RouteVertex> crossing;
	Control_Vertices(controls,control_count,&crossing);

	/*
	**	Each side is found on its own, with the other side standing in as a straight run on along the crossing's
	**	axis, because the two are separated by the crossing's frozen ends and cannot crowd each other.  The road
	**	leaves the preliminary centerline at the latest sample at or before the interval's ingress, and rejoins
	**	at the earliest at or after its egress, from which the shared corner construction -- Phase 10's own
	**	leads and fillets at the class radius -- can reach the crossing; the closest is the smallest change to the
	**	road, and the search order makes the choice deterministic.
	*/
	const float extension = 2.0f * spacing;
	DynamicVectorClass<RouteVertex> near_side;
	int depart = -1;
	for (int d = first_point; d >= 0; --d) {

		if ((route.Stations[first_point] - route.Stations[d]) > max_reach) break;

		DynamicVectorClass<RouteVertex> candidate;
		int start = d;
		if (!Departure_Vertices(route,d,controls[0],entry_axis,spacing,quantum,&candidate,&start)) continue;

		DynamicVectorClass<RouteVertex> trial;
		for (int i = 0; i < candidate.Count(); ++i) Append_Vertex(&trial,candidate[i],quantum);
		for (int i = 0; i < crossing.Count(); ++i) Append_Vertex(&trial,crossing[i],quantum);
		RouteVertex beyond;
		beyond.P = controls[control_count - 1] + (exit_axis * extension);
		trial.Add(beyond);
		if (!Settle_Corners(radius,quantum,spacing,&trial,nullptr)) continue;

		depart = start;
		for (int i = 0; i < candidate.Count(); ++i) near_side.Add(candidate[i]);
		break;
	}
	if (depart < 0) {
		if (reason_out != nullptr) *reason_out = "no departure from the preliminary road meets the crossing at the class radius";
		return false;
	}

	DynamicVectorClass<RouteVertex> far_side;
	int rejoin = -1;
	for (int r = last_point; r < count; ++r) {

		if ((route.Stations[r] - route.Stations[last_point]) > max_reach) break;

		DynamicVectorClass<RouteVertex> candidate;
		int finish = r;
		if (!Rejoin_Vertices(route,r,controls[control_count - 1],exit_axis,spacing,quantum,&candidate,&finish)) continue;

		DynamicVectorClass<RouteVertex> trial;
		RouteVertex before;
		before.P = controls[0] - (entry_axis * extension);
		trial.Add(before);
		for (int i = 0; i < crossing.Count(); ++i) Append_Vertex(&trial,crossing[i],quantum);
		for (int i = 0; i < candidate.Count(); ++i) Append_Vertex(&trial,candidate[i],quantum);
		if (!Settle_Corners(radius,quantum,spacing,&trial,nullptr)) continue;

		rejoin = finish;
		for (int i = 0; i < candidate.Count(); ++i) far_side.Add(candidate[i]);
		break;
	}
	if (rejoin < 0) {
		if (reason_out != nullptr) *reason_out = "no rejoin onto the preliminary road leaves the crossing at the class radius";
		return false;
	}
	if (rejoin <= depart) {
		if (reason_out != nullptr) *reason_out = "the crossing path departs after it rejoins";
		return false;
	}

	//	The whole path, settled once more as one polyline and laid down exactly as Phase 10 lays a centerline.
	DynamicVectorClass<RouteVertex> vertices;
	for (int i = 0; i < near_side.Count(); ++i) Append_Vertex(&vertices,near_side[i],quantum);
	for (int i = 0; i < crossing.Count(); ++i) Append_Vertex(&vertices,crossing[i],quantum);
	for (int i = 0; i < far_side.Count(); ++i) Append_Vertex(&vertices,far_side[i],quantum);
	if (vertices.Count() < 2) {
		if (reason_out != nullptr) *reason_out = "the crossing path collapsed to a point";
		return false;
	}

	const char * why = nullptr;
	if ((vertices.Count() > 2) && !Settle_Corners(radius,quantum,spacing,&vertices,&why)) {
		if (reason_out != nullptr) *reason_out = (why != nullptr) ? why : "the crossing path could not be filleted";
		return false;
	}

	DynamicVectorClass<Vector2> samples;
	DynamicVectorClass<float> stations;
	DynamicVectorClass<HybridRouteStructuralSpan> spans;
	if (!Emit_Centerline(radius,quantum,vertices,spacing,&samples,&stations,&spans)) {
		if (reason_out != nullptr) *reason_out = "the crossing path is degenerate";
		return false;
	}

	//	Quantized and de-duplicated, with both ends exactly the preliminary route's own samples.
	DynamicVectorClass<Vector2> kept;
	for (int i = 0; i < samples.Count(); ++i) {
		const Vector2 q = (i == 0) ? route.Points[depart]
										   : Vector2(Hybrid_Quantize_Position(samples[i].X),Hybrid_Quantize_Position(samples[i].Y));
		if ((kept.Count() > 0) && ((q - kept[kept.Count() - 1]).Length() <= quantum)) continue;
		kept.Add(q);
	}
	if ((kept.Count() > 1) && ((kept[kept.Count() - 1] - route.Points[rejoin]).Length() <= quantum)) {
		kept[kept.Count() - 1] = route.Points[rejoin];
	} else {
		kept.Add(route.Points[rejoin]);
	}
	if (kept.Count() < 2) {
		if (reason_out != nullptr) *reason_out = "the crossing path collapsed to a point";
		return false;
	}

	float length = 0.0f;
	for (int i = 0; i < kept.Count(); ++i) {
		if (i > 0) length += (kept[i] - kept[i - 1]).Length();
		out->Points.Add(kept[i]);
		out->Stations.Add(length);
	}

	out->DepartPoint	= depart;
	out->RejoinPoint	= rejoin;
	out->Length			= Hybrid_Quantize_Position(length);
	out->AddedLength	= Hybrid_Quantize_Position(Max2(0.0f,length -
																		(route.Points[rejoin] - route.Points[depart]).Length()));
	return true;
}


/***********************************************************************************************
**	A HIGHWAY_RAMP's constructed centerline -- see Hybrid_Ramp_Centerline in the header.
***********************************************************************************************/
bool Hybrid_Ramp_Centerline(const Vector2 polygon[4],float ramp_radius,float quantum,float spacing,
									 DynamicVectorClass<Vector2> * points,DynamicVectorClass<float> * stations)
{
	if ((points == nullptr) || (stations == nullptr) || !(ramp_radius > 0.0f)) return false;

	DynamicVectorClass<RouteVertex> vertices;
	for (int i = 0; i < 4; ++i) {
		RouteVertex vertex;
		vertex.P			= polygon[i];
		vertex.Frozen	= (i == 0) || (i == 3);
		vertex.Pinned	= (i == 1) || (i == 2);
		vertices.Add(vertex);
	}
	const char * reason = nullptr;
	if (!Settle_Corners(ramp_radius,quantum,0.0f,&vertices,&reason)) return false;
	if (vertices.Count() != 4) return false;				// a constructed corner was dropped: not the plan's ramp

	DynamicVectorClass<HybridRouteStructuralSpan> spans;
	return Emit_Centerline(ramp_radius,quantum,vertices,spacing,points,stations,&spans);
}


/***********************************************************************************************
**	Two road poses on empty ground -- see Hybrid_Road_Poses_Connect in the header.
***********************************************************************************************/
bool Hybrid_Road_Poses_Connect(const HybridRoadPose & from,const HybridRoadPose & to,HybridRoadClass road_class,
										 const HybridConstants & constants,const Vector2 * before_from,const Vector2 * after_to)
{
	const HybridConstants & k = constants;
	const float radius = Hybrid_Road_Class_Spec(road_class).MinTurnRadiusW * k.W * FILLET_RADIUS_MARGIN;
	const float straight = Min2(k.W,0.5f * k.A);
	if (!(radius > 0.0f) || ((from.Point - to.Point).Length() <= k.Q)) return false;

	//	The structure's own straight, frozen at its far end (`from`) and its near end (`to`).
	RouteVertex start;
	RouteVertex leave;
	bool from_structural = from.Is_Structural();
	Vector2 from_axis(0.0f,0.0f);
	if (from_structural) {
		from_axis = Unit(from.Axis);
		start.P			= from.Point - (from_axis * from.Before);
		leave.P			= from.Point + (from_axis * from.After);
		leave.Axis		= from_axis;
		leave.Frozen	= true;
		leave.WindowId	= 1;
	} else {
		start.P = from.Point;
	}

	/*
	**	A free pose in the middle of its edge -- an ordinary guide -- is Phase 10's guide corner, with the road
	**	behind it as the start: held like a pinned corner, except that a structure's lead may drop it, because the
	**	centerline is not bound to the guide.
	*/
	RouteVertex from_corner;
	const bool from_guide = !from_structural && (before_from != nullptr) &&
									((*before_from - from.Point).Length() > k.Q);
	if (from_guide) {
		from_corner.P		= from.Point;
		from_corner.Pinned= true;
		from_corner.Guide	= true;
		start.P				= *before_from;
	}

	RouteVertex arrive;
	RouteVertex finish;
	const bool to_structural = to.Is_Structural();
	Vector2 to_axis(0.0f,0.0f);
	if (to_structural) {
		to_axis = Unit(to.Axis);
		arrive.P			= to.Point - (to_axis * to.Before);
		arrive.Axis		= to_axis;
		arrive.Frozen	= true;
		arrive.WindowId= 2;
		finish.P			= to.Point + (to_axis * to.After);
	} else {
		finish.P = to.Point;
	}

	RouteVertex to_corner;
	const bool to_guide = !to_structural && (after_to != nullptr) && ((*after_to - to.Point).Length() > k.Q);
	if (to_guide) {
		to_corner.P			= to.Point;
		to_corner.Pinned	= true;
		to_corner.Guide	= true;
		finish.P				= *after_to;
	}

	/*
	**	Where the road would turn on a direct route: the meeting point of the line leaving `from` along its
	**	axis and the line arriving at `to` along its axis, when both reach it going forward.  Without one of
	**	the two axes the leads alone are the turns.
	*/
	bool has_corner = false;
	Vector2 corner(0.0f,0.0f);
	if (from_structural && to_structural) {
		const Vector2 p = leave.P;
		const Vector2 r = from_axis;
		const Vector2 q = arrive.P;
		const Vector2 s = to_axis;
		const float denominator = (r.X * s.Y) - (r.Y * s.X);
		if ((float)fabs((double)denominator) > 1.0e-4f) {
			const Vector2 d = q - p;
			const float t = ((d.X * s.Y) - (d.Y * s.X)) / denominator;
			const float u = ((d.X * r.Y) - (d.Y * r.X)) / denominator;
			if ((t > k.Q) && (u > k.Q)) {
				has_corner = true;
				corner = p + (r * t);
			}
		}
	}

	for (int attempt = 0; attempt < 2; ++attempt) {

		if ((attempt == 0) && !has_corner) continue;

		DynamicVectorClass<RouteVertex> vertices;
		vertices.Add(start);
		if (from_guide) vertices.Add(from_corner);
		if (from_structural) vertices.Add(leave);
		if (attempt == 0) {
			RouteVertex middle;
			middle.P = corner;
			vertices.Add(middle);
		}
		if (to_structural) vertices.Add(arrive);
		if (to_guide) vertices.Add(to_corner);
		vertices.Add(finish);

		if (Settle_Corners(radius,k.Q,straight,&vertices,nullptr)) return true;
	}
	return false;
}
