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
 *                     $Archive:: /Commando/Code/wwphys/hybridroutes.h                        $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Hybrid world generator rewrite, item I11 -- runtime Phase 10, the bounded heading-aware A*    *
 * and its splines (Procedural_World_Generator_Rewrite_Plan_FINAL_AUDITED.md Section 8 Phase 10, *
 * Section 16.2).                                                                               *
 *                                                                                             *
 * Phase 9 decided which roads exist and which frozen points each one must pass through; this    *
 * phase decides where each one actually runs.  Nothing is realized on the ground: the output is *
 * one accepted spline per topology edge, plus the cell chain it came from, and the crossing      *
 * intervals stay unresolved records for Phase 11 to compile.                                    *
 *                                                                                             *
 * Everything bounded is bounded here: three search envelopes per edge (preferred, `+S`, `+2S`), *
 * `8N` A* expansions per route and `R*8N` per attempt, and at most                              *
 * `retry.max_local_retries_per_stage` control-point insertions per envelope.  A route that      *
 * cannot be found inside those bounds fails the attempt; nothing is widened to rescue it.       *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef HYBRIDROUTES_H
#define HYBRIDROUTES_H

#include "always.h"
#include <string.h>
#include "vector.h"
#include "vector2.h"
#include "wwstring.h"
#include "hybridplanning.h"
#include "hybridmapidentity.h"
#include "hybridcoarsematrix.h"
#include "hybridlandforms.h"
#include "hybridhydrology.h"
#include "hybridanchors.h"
#include "hybridtopology.h"


//	Section 8 Phase 10's routing priority groups, in the plan's own order.  The ordinal is the key.
enum HybridRoutePriority
{
	HYBRID_ROUTE_PRIORITY_FAMILY_TRUNK = 0,	// 1 heavy route-family trunks
	HYBRID_ROUTE_PRIORITY_BRIDGE_LINK,			// 2 mandatory synthesized river/canyon bridge cross-links
	HYBRID_ROUTE_PRIORITY_HOME_ROAD,				// 3 home-field roads
	HYBRID_ROUTE_PRIORITY_FIELD_BRANCH,			// 4 contested/remote service branches
	HYBRID_ROUTE_PRIORITY_SERVICE_FILL,			// 5 quota-filling SERVICE_DIRT_2 cross-links
	HYBRID_ROUTE_PRIORITY_OVERPASS_LINK,		// 6 mandatory/synthesized overpass SECONDARY_2 cross-links
	HYBRID_ROUTE_PRIORITY_OPTIONAL_LINK,		// 7 optional heavy cross-links
	HYBRID_ROUTE_PRIORITY_TRAIL,					// 8 TRAIL_1 shortcuts
	HYBRID_ROUTE_PRIORITY_COUNT
};

const char *	Hybrid_Route_Priority_Name(HybridRoutePriority priority);

//	Which group a topology edge belongs to, from its kind, class and waypoints.
HybridRoutePriority	Hybrid_Route_Priority_Of(const HybridTopology & topology,const HybridTopologyEdge & edge);


/*
**	One structural interval inside a route: the frozen straight centerline that carries the road across an
**	accepted crossing window.  Phase 10 decides only that the interval is straight, where it starts and
**	where it ends; whether it becomes a bridge, an overpass, a bore or a cut is the interaction phase's
**	choice, so nothing here names a method.
*/
struct HybridRouteStructuralSpan
{
	HybridRouteStructuralSpan(void) :
		WindowId(0),FirstPoint(0),LastPoint(0),Ingress(0.0f,0.0f),Egress(0.0f,0.0f) { }

	//	DynamicVectorClass<T> requires == and !=.  The window is this record's identity within a route.
	bool	operator == (const HybridRouteStructuralSpan & that) const	{ return WindowId == that.WindowId; }
	bool	operator != (const HybridRouteStructuralSpan & that) const	{ return !(*this == that); }

	unsigned int	WindowId;			// the HybridCrossingWindow this interval crosses
	int				FirstPoint;			// index into HybridRoute::Points of the ingress sample
	int				LastPoint;			// index into HybridRoute::Points of the egress sample
	Vector2			Ingress;				// the window's frozen approach point on the near bank
	Vector2			Egress;				// the window's frozen approach point on the far bank
};


/*
**	One routed edge: the accepted spline, the A* cell chain behind it, and what the search cost.
*/
struct HybridRoute
{
	HybridRoute(void) :
		EdgeId(0),RoadClass(HYBRID_ROAD_CLASS_COUNT),Priority(HYBRID_ROUTE_PRIORITY_FAMILY_TRUNK),
		RouteFamily(-1),Envelope(0),Required(false),Capabilities(0),Length(0.0f),ControlInsertions(0),
		Expansions(0),
		BoundsMin(0.0f,0.0f),BoundsMax(0.0f,0.0f) { }

	//	DynamicVectorClass<T> (wwlib/vector.h) requires T to support == and !=.  EdgeId is the key.
	bool	operator == (const HybridRoute & that) const	{ return EdgeId == that.EdgeId; }
	bool	operator != (const HybridRoute & that) const	{ return !(*this == that); }

	unsigned int			EdgeId;				// HybridTopologyEdge::StableId; this record's own identity
	HybridRoadClass		RoadClass;
	HybridRoutePriority	Priority;
	int						RouteFamily;
	int						Envelope;			// 0 preferred, 1 preferred+S, 2 preferred+2S
	bool						Required;
	unsigned int			Capabilities;		// the edge's HybridNavCapability mask, copied unchanged
	float						Length;				// spline arc length, quantized
	int						ControlInsertions;
	int						Expansions;

	Vector2					BoundsMin;
	Vector2					BoundsMax;

	DynamicVectorClass<Vector2>		Points;		// spline samples at min(W,A/2), quantized
	DynamicVectorClass<float>			Stations;	// along-route arc length of each sample
	DynamicVectorClass<int>				Cells;		// the accepted A* chain, row-major analysis cell ids
	DynamicVectorClass<unsigned int>	Waypoints;	// the frozen waypoints it was routed through, in order

	//	The straight structural intervals inside this route, in travel order; empty for an open road.
	DynamicVectorClass<HybridRouteStructuralSpan>	Structural;
};


struct HybridRouteSet
{
	HybridRouteSet(void)	{ Reset (); }

	void	Reset(void);

	const HybridRoute *	Find(unsigned int edge_id) const;
	int						Count_Of_Class(HybridRoadClass road_class) const;
	float						Total_Length_Of_Class(HybridRoadClass road_class) const;

	DynamicVectorClass<HybridRoute>	Routes;

	int			AnalysisCountX;				// the grid every chain's cell ids index into
	int			AnalysisCountY;
	int			TotalExpansions;
	int			TotalInsertions;
	int			WidenedRoutes;					// routes that needed an envelope past the preferred one
	unsigned int	Checksum;					// Phase 10 gate value
};


/*
**	One edge that may not use its narrower corridor envelopes again.
**
**	Phase 11 is allowed to answer "this crossing cannot be compiled -- give the road one more envelope", and
**	this is how the answer comes back: the edge, and the narrowest envelope it may now start from.  Nothing
**	else about the search changes, and no envelope is ever reconsidered once it has been ruled out, so a
**	reroute always moves outward and the same route state cannot come round again.
*/
struct HybridRouteEnvelopeFloor
{
	HybridRouteEnvelopeFloor(void) : EdgeId(0),MinEnvelope(0) { }

	//	DynamicVectorClass<T> requires == and !=.
	bool	operator == (const HybridRouteEnvelopeFloor & that) const	{ return EdgeId == that.EdgeId; }
	bool	operator != (const HybridRouteEnvelopeFloor & that) const	{ return !(*this == that); }

	unsigned int	EdgeId;
	int				MinEnvelope;		// 1 or 2; 0 is the ordinary start and needs no record
};


struct HybridRouteParams
{
	HybridRouteParams(void);

	HybridConstants	Constants;

	float					WorldMinX;
	float					WorldMinY;
	float					WorldWidth;
	float					WorldHeight;

	float					MaxCutDepth;					// Section 7.1's vertical-interval inputs
	float					MaxFillHeight;
	float					RiverWidth;						// the river bank term, as Phase 6/7 use it
	float					FieldRadiusMax;
	int					MaxLocalRetriesPerStage;
	int					AStarExpansionsPerRoute;	// 8N from the Section-16.1 budgets
	int					AStarExpansionsPerAttempt;	// R * 8N

	//	Empty for an ordinary run: every edge starts at envelope 0.
	DynamicVectorClass<HybridRouteEnvelopeFloor>	EnvelopeFloors;

	HybridScratchArena *		Arena;
	HybridWorkCounters *		Counters;
};


enum HybridRouteFailure
{
	HYBRID_ROUTE_OK = 0,
	HYBRID_ROUTE_INVALID_INPUT,				// no topology, no frozen anchors, or degenerate constants
	HYBRID_ROUTE_SCRATCH_EXHAUSTED,			// the arena cannot hold the search state
	HYBRID_ROUTE_NO_PATH,						// no envelope contains a legal path for a mandatory edge
	HYBRID_ROUTE_NO_SPLINE,						// a path exists but no legal spline survives the insertions
	HYBRID_ROUTE_EXPANSION_BUDGET,			// the per-route or per-attempt A* ceiling was reached
	HYBRID_ROUTE_WAYPOINT_MISSED,				// gate: a frozen waypoint cell is not on the accepted chain
	HYBRID_ROUTE_CROSSING_MISALIGNED,		// gate: a crossing is not entered or left along its own axis
	HYBRID_ROUTE_FAILURE_COUNT
};

const char *	Hybrid_Route_Failure_Name(HybridRouteFailure failure);

struct HybridRouteResult
{
	HybridRouteResult(void) : Failure(HYBRID_ROUTE_OK) { }

	HybridRouteFailure	Failure;
	StringClass				Detail;
};


/*
**	Section 8 Phase 10's cost constants per road class: the grade, turn and corridor-excess weights.
*/
struct HybridRouteCostWeights
{
	float	Ks;			// grade
	float	Kt;			// heading change
	float	Kc;			// corridor excess
};

HybridRouteCostWeights	Hybrid_Route_Cost_Weights(HybridRoadClass road_class);


/*
**	Runtime Phase 10.  Routes every topology edge in the plan's priority order, through its frozen
**	waypoints, inside at most three envelopes, and converts each accepted cell chain into a spline by
**	the plan's deterministic procedure.  Returns false with result->Failure set on the first failure,
**	leaving `out` empty.  `sample` reads the realized Phase-7 height, as in Phases 8 and 9.
*/
typedef bool (*HybridRouteHeightSampleFunc)(const Vector2 & world,float * height_out,void * context);

bool	Hybrid_Build_Routes(const HybridRouteParams & params,const HybridMapIdentity & identity,
										const HybridCoarseMatrices & matrices,const HybridLandformSet & landforms,
										const HybridHydrologySet & hydrology,const HybridAnchorSet & anchors,
										const HybridTopology & topology,HybridRouteHeightSampleFunc sample,
										void * context,HybridRouteSet * out,HybridRouteResult * result);

/*
**	Routing diagnostics.  `Hybrid_Route_Census` answers "what refuses this region?" with the same
**	classifier the router itself uses, for one topology edge, one leg at a time: how many analysis cells
**	each rule refuses, whether the leg's goal is reachable from its start at all, what refuses the
**	frontier the flood stops at, and what refuses each of the goal's eight neighbours.
**
**	It is the measurement a `no_path` failure is diagnosed from, and it is called only from a self-check
**	or a developer tool -- generation itself never calls it, so a normal build is not noisy.
*/
enum HybridRouteRefusalRule
{
	HYBRID_ROUTE_RULE_UNPLACEABLE = 0,
	HYBRID_ROUTE_RULE_BASE_DISC,
	HYBRID_ROUTE_RULE_TIBERIUM,
	HYBRID_ROUTE_RULE_WATER,
	HYBRID_ROUTE_RULE_WALL,
	HYBRID_ROUTE_RULE_COUNT
};

const char *	Hybrid_Route_Refusal_Rule_Name(HybridRouteRefusalRule rule);

struct HybridRouteCensus
{
	HybridRouteCensus(void)	{ memset (this,0,sizeof (*this)); }

	unsigned int	EdgeId;
	int				LegCount;
	int				Leg;							// the leg this census describes
	bool				GoalReachable;

	int				CellCount;
	int				Accepted;
	int				Refused[HYBRID_ROUTE_RULE_COUNT];
	int				RefusedByMoreThanOne;
	int				Reachable;					// cells the flood from `start` can actually enter
	int				MoveReachable;				// ...and with the move-grade rule applied as well
	bool				GoalMoveReachable;
	int				GradeBlockedMoves;
	float				NearestReachableDistance;	// how close the flood got to the goal, world units

	//	What refuses the frontier the flood stopped at: the rule counts that actually block this leg.
	int				Frontier[HYBRID_ROUTE_RULE_COUNT];
	int				FrontierCells;

	int				StartCell;
	int				GoalCell;
	unsigned int	FromNode;					// the leg's own endpoints, as topology nodes
	unsigned int	GoalNode;
	int				GoalNodeKind;				// HybridTopologyNodeKind
	unsigned int	GoalWindowId;
	int				GoalX;
	int				GoalY;
	int				NeighborCell[8];
	int				NeighborX[8];
	int				NeighborY[8];
	unsigned int	NeighborRefusal[8];		// bit i set when rule i refuses that neighbour; 0 = accepted
};

bool	Hybrid_Route_Census(const HybridRouteParams & params,const HybridMapIdentity & identity,
									 const HybridCoarseMatrices & matrices,const HybridLandformSet & landforms,
									 const HybridHydrologySet & hydrology,const HybridAnchorSet & anchors,
									 const HybridTopology & topology,HybridRouteHeightSampleFunc sample,
									 void * context,unsigned int edge_id,HybridRouteCensus * out);


/*
**	The Phase 10 gate value: a quantized digest of every accepted route in EdgeId order.
*/
unsigned int	Hybrid_Route_Checksum(const HybridRouteSet & set,unsigned int seed = 0);


/*
**	The horizontal centerline a compiled crossing is driven on.
**
**	Phase 10 routes every crossing straight through its window's two frozen approach points.  Phase 11 may
**	then place the crossing somewhere else along the obstruction -- the plan's station search -- or compile a
**	pass whose far end leaves the cliff some way along it, and in both cases "the connected road can meet the
**	station within its max grade and minimum turn radius" is a statement about a road that has to exist.
**	This is that road: the preliminary centerline up to where it departs, Section-7 fillets at the class
**	turn radius into the crossing's first control point, the crossing's own controls, fillets back out of the
**	last one, and the preliminary centerline again from where it rejoins.
**
**	It is built with Phase 10's own corner construction, not a second turn solver: the two end controls are
**	*frozen* exactly as Phase 10 freezes a window's approach points, so the road runs straight through each
**	along the crossing's own axis and every turn onto it is finished on a lead before the control -- never
**	inside the straight approach reserve a bridge needs.  Interior controls -- a pass's own corners -- are
**	pinned corners carrying the same D-009 fillets the pass builder judged them by.  When the preliminary
**	centerline already runs through a control along its axis (the crossing at its own window station), the
**	path simply continues on it and no connector is built on that side.
**
**	It is built once, in Phase 11, and frozen in the compiled record; Phase 12 realizes it and nothing else.
*/
struct HybridCrossingPath
{
	HybridCrossingPath(void) : DepartPoint(0),RejoinPoint(0),Length(0.0f),AddedLength(0.0f) { }

	void	Reset(void)
	{
		DepartPoint	= 0;
		RejoinPoint	= 0;
		Length		= 0.0f;
		AddedLength	= 0.0f;
		Points.Delete_All();
		Stations.Delete_All();
	}

	int								DepartPoint;	// the preliminary route sample the realized road leaves at
	int								RejoinPoint;	// and the one it rejoins at
	float								Length;			// arc length of `Points`
	float								AddedLength;	// `Length` less the straight distance between its two ends, >= 0
	DynamicVectorClass<Vector2>	Points;			// route.Points[DepartPoint] .. route.Points[RejoinPoint]
	DynamicVectorClass<float>		Stations;		// arc length from Points[0]
};

/*
**	Builds the path above for one structural interval of `route` and the crossing's ordered `controls`
**	(at least two, the first on the interval's ingress side).  `max_reach` bounds how far back along the
**	route the road may depart, and how far forward it may rejoin, measured from the interval's own ends.
**	Fails, naming why in `reason_out`, when no departure or rejoin on the preliminary centerline can meet the
**	frozen controls at `radius` -- that is the plan's turn-radius test failing, not a geometry to bend.
*/
/*
**	Could a road of `road_class` get from `from` to `to` on empty ground?  Phase 10's own corner construction
**	answers it -- the frozen straight through each structural pose, the leads that finish every turn before a
**	structure, and the class-radius fillets -- applied to the two poses with nothing between them but, where
**	the two axes meet ahead, the one corridor corner a direct route would turn at.  Terrain is not consulted:
**	this is the plan's "provably impossible even on an empty plane", which is Phase 9's to refuse, and the
**	exact terrain path stays Phase 10's.
**
**	A free pose with a neighbour of its own beyond it -- an ordinary route waypoint in the middle of an edge --
**	is a pinned corner in Phase 10, whose own arc needs room on the leg it shares with the structure's lead;
**	`before_from` / `after_to` name that neighbour so the corner is part of the construction.
*/
bool	Hybrid_Road_Poses_Connect(const HybridRoadPose & from,const HybridRoadPose & to,HybridRoadClass road_class,
										const HybridConstants & constants,const Vector2 * before_from = nullptr,
										const Vector2 * after_to = nullptr);

/*
**	How far a sampled road runs straight along `axis` through its sample `index`, behind it and ahead of it:
**	the arc length over which every sample stands on that line, within the position quantum each sample was
**	written to and the angular quantum over the run.  The revised D-009 invariant is stated in it: a crossing
**	is entered and left along its own axis, with the turn onto it finished before its frozen approach point.
*/
void	Hybrid_Polyline_Straight_Run(const DynamicVectorClass<Vector2> & points,int index,const Vector2 & axis,
											 float q,float * behind_out,float * ahead_out);

bool	Hybrid_Build_Crossing_Path(const HybridRoute & route,int first_point,int last_point,
										 const Vector2 * controls,int control_count,float radius,float spacing,
										 float quantum,float max_reach,HybridCrossingPath * out,
										 const char ** reason_out = nullptr);


#endif // HYBRIDROUTES_H
