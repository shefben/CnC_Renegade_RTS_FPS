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
 *                     $Archive:: /Commando/Code/wwphys/hybridinteractions.h                  $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Hybrid world generator rewrite, item I12 -- runtime Phase 11, the deterministic interaction   *
 * solver (Procedural_World_Generator_Rewrite_Plan_FINAL_AUDITED.md Section 8 Phase 11 and        *
 * Sections 12.3 to 12.5).                                                                       *
 *                                                                                             *
 * Phase 10 decided where every road runs and left each crossing as an unresolved structural      *
 * interval.  This phase decides *what each one is*: a bridge, a ford, a surface pass, a road cut, *
 * a ramp pass, a tunnel, an at-grade junction or an overpass.  It instantiates nothing -- no      *
 * RoadSystem, BridgeSystem or TunnelSystem object is created here, and no terrain is touched.     *
 * The output is one compiled solution record per conflict for Phase 12 to realize.                *
 *                                                                                             *
 * Every method order in this file is the plan's, and it is fixed: the solver never reorders a     *
 * sequence to find a cheaper answer, and it never invents a method a Phase-6 window does not      *
 * already permit.  A mandatory Phase-9 obligation keeps its required method or the attempt fails; *
 * the solver never moves an obligation to a different obstruction.                                *
 *                                                                                             *
 * This is the last phase allowed to change a crossing's method or location, or to spend a         *
 * remaining Phase-10 corridor envelope.                                                          *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef HYBRIDINTERACTIONS_H
#define HYBRIDINTERACTIONS_H

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
#include "hybridroutes.h"
#include "hybridlocalpass.h"


/*
**	The methods Phase 11 may compile, in the plan's own vocabulary.  Nothing outside this list is a
**	solution, and a window's `HybridWindowMethodFlag` bits decide which of them are even available at a
**	given obstruction -- Phase 6 computed those from the frozen preflight metrics and Phase 11 may not
**	re-derive them.
*/
enum HybridInteractionMethod
{
	HYBRID_METHOD_BRIDGE = 0,			// river, canyon or ravine; the only method a quota counts
	HYBRID_METHOD_FORD,					// river only, light classes only, never a quota
	HYBRID_METHOD_SURFACE_PASS,		// canyon/ravine descent across an accepted pass-capable window
	HYBRID_METHOD_ROAD_CUT,				// cliff/ridge, within max_cut_depth and the road deformation width
	HYBRID_METHOD_RAMP_PASS,			// cliff/ridge, across an accepted RAMP_PASS_WINDOW
	HYBRID_METHOD_TUNNEL,				// fallback only, and never mandatory in this generator version
	HYBRID_METHOD_AT_GRADE_JUNCTION,	// road x road
	HYBRID_METHOD_OVERPASS,				// road x road
	HYBRID_METHOD_COUNT
};

const char *	Hybrid_Interaction_Method_Name(HybridInteractionMethod method);

//	Does this method satisfy a Section-5 bridge quota?  Only a bridge does; a ford never does.
bool	Hybrid_Interaction_Counts_As_Bridge(HybridInteractionMethod method);


/*
**	Section 8 Phase 11's metric resolution order for a bridge kind's maximum supported span: "resolve
**	metrics from the existing bridge definition first and `BridgePlannerParams` second".  The definition's
**	own value wins when it supplies one; otherwise `BridgePlannerParams::MaxSpanLength` is the answer, and
**	it is an answer rather than an absence.  There is no third source and no constant of this phase's own:
**	when neither supplies a span, preflight has already refused the request.
**
**	Several kinds inheriting the same planner-wide span therefore have genuinely equal span capability, and
**	selection among them falls to the remaining suitability tests and then the lower `StableId` -- which is
**	the plan's own tie break, not a shortcut around a missing metric.
*/
float	Hybrid_Resolve_Bridge_Max_Span(float definition_max_span,float planner_max_span_length);


/*
**	One bridge kind as the registry describes it, flattened into plain numbers so this phase does not
**	depend on the Combat layer.  `StableId` is the registry index, and it is the deterministic tie break
**	the plan asks for after "smallest sufficient maximum span".
**
**	Every metric here is resolved by the caller from the existing bridge definition first and
**	`BridgePlannerParams` second.  Where a required metric exists in neither, preflight has already
**	failed -- this phase never substitutes a constant of its own.
*/
struct HybridBridgeKind
{
	HybridBridgeKind(void) :
		StableId(0),ClearWidth(0.0f),StructuralDepth(0.0f),RequiredClearance(0.0f),MaxSpan(0.0f),
		RequiredApproachLength(0.0f),StartSeat(0.0f),EndSeat(0.0f) { }

	//	DynamicVectorClass<T> (wwlib/vector.h) requires T to support == and !=.
	bool	operator == (const HybridBridgeKind & that) const	{ return StableId == that.StableId; }
	bool	operator != (const HybridBridgeKind & that) const	{ return !(*this == that); }

	int		StableId;
	float		ClearWidth;					// clear deck width
	float		StructuralDepth;			// deck thickness below the running surface
	float		RequiredClearance;		// the kind's own clearance requirement under the deck
	float		MaxSpan;						// maximum supported span, through Hybrid_Resolve_Bridge_Max_Span
	float		RequiredApproachLength;	// 0 = none registered; the solver then uses 8W

	/*
	**	The canonical abutment seats: the definition's start and end cap section lengths.  The cap is the
	**	abutment BridgeSystem lays inward from each endpoint, so a deck reaches this far past the obstruction
	**	at each end for its abutment to stand on the bank, and the span it is selected against is the measured
	**	obstruction plus both of them.
	*/
	float		StartSeat;
	float		EndSeat;
};


/*
**	One tunnel module as the registry describes it, for Section 12.5's "shortest sufficient registered
**	definition/module sequence, tie break by StableId".
*/
struct HybridTunnelModule
{
	HybridTunnelModule(void) :
		StableId(0),Length(0.0f),ClearWidth(0.0f),ClearHeight(0.0f),MaxGradeRadians(0.0f),
		PortalCompatible(false) { }

	bool	operator == (const HybridTunnelModule & that) const	{ return StableId == that.StableId; }
	bool	operator != (const HybridTunnelModule & that) const	{ return !(*this == that); }

	int		StableId;
	float		Length;
	float		ClearWidth;					// 0 = a procedural tube, sized per tunnel
	float		ClearHeight;
	float		MaxGradeRadians;
	bool		PortalCompatible;
};


/*
**	Why a conflict could not be solved, in the categories the plan's own failure vocabulary uses.  The
**	solver records one of these per unsolved conflict so a refused attempt reads as a story rather than
**	as a bare refusal.
*/
enum HybridInteractionProblem
{
	HYBRID_PROBLEM_NONE = 0,
	HYBRID_PROBLEM_NO_HARD_VALID_STATION,	// every station in the search window failed a common test
	HYBRID_PROBLEM_SPAN_TOO_LONG,				// no registered bridge kind spans the obstruction
	HYBRID_PROBLEM_WIDTH_TOO_NARROW,			// no kind meets the Section-12.4 clear width
	HYBRID_PROBLEM_APPROACH_DOES_NOT_FIT,	// the straight approach reserve has no room
	HYBRID_PROBLEM_GRADE_TOO_STEEP,
	HYBRID_PROBLEM_TURN_TOO_TIGHT,
	HYBRID_PROBLEM_CLEARANCE,					// swept vehicle width/height, or overpass vertical gap
	HYBRID_PROBLEM_CROSS_SLOPE,				// the surface tilts more across the roadbed than the class allows
	HYBRID_PROBLEM_RESERVE_CONFLICT,			// an approach reserve hits an unrelated base or Tiberium core
	HYBRID_PROBLEM_METHOD_UNAVAILABLE,		// the window permits no method this conflict may use
	HYBRID_PROBLEM_CUT_OR_FILL,			// the profile cannot be realized inside its RoadVerticalInterval
	HYBRID_PROBLEM_TOO_CLOSE_TO_CROSSING,	// inside 4W of an unrelated compiled crossing
	HYBRID_PROBLEM_GORE_RESERVE,				// an approach, portal or overpass ramp reserve meets a gore reserve (12.6.10)
	HYBRID_PROBLEM_INTERCHANGE_FOOTPRINT,	// an incidental junction inside an interchange footprint + RoadOuterBlendWidth
	HYBRID_PROBLEM_COUNT
};

const char *	Hybrid_Interaction_Problem_Name(HybridInteractionProblem problem);


/*
**	Section 12.1's compiled junction roles.  An at-grade junction or an overpass is exactly one of them, and the
**	three Section-12.6 highway access roles are intended junctions, never rediscovered as incidental crossings.
**	Ordinals are part of the checksum: never reorder.
*/
enum HybridJunctionRole
{
	HYBRID_JUNCTION_ROLE_NONE = 0,						// not a road x road solution
	HYBRID_JUNCTION_ROLE_INCIDENTAL,					// an incidental ROAD_X_ROAD crossing
	HYBRID_JUNCTION_ROLE_SHARED_NODE,					// the topology's intended junction at a shared node (Q-025)
	HYBRID_JUNCTION_ROLE_OVERPASS_OBLIGATION,		// a reserved Phase-9 overpass
	HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_MERGE,		// an ON_RAMP over its MERGE_SECTION and gore reserve
	HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_DIVERGE,		// an OFF_RAMP over its DIVERGE_SECTION and gore reserve
	HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_LOCAL_JUNCTION,	// all arms at one interchange local junction L_s
	HYBRID_JUNCTION_ROLE_COUNT
};

const char *	Hybrid_Junction_Role_Name(HybridJunctionRole role);

/*
**	One compiled solution.  It says what the crossing *is* and where, and nothing about how it is built:
**	Phase 12 owns realization, and this record is the whole of what it is allowed to consume about a
**	crossing.
*/
struct HybridInteraction
{
	HybridInteraction(void) :
		StableId(0),EdgeId(0),OtherEdgeId(0),NodeId(0),Obstruction(HYBRID_OBSTRUCTION_NONE),ObstructionRecordId(0),
		WindowId(0),Mandatory(false),Method(HYBRID_METHOD_BRIDGE),RoadClass(HYBRID_ROAD_CLASS_COUNT),
		FallbackFrom(HYBRID_PROBLEM_NONE),
		Station(0.0f,0.0f),Tangent(1.0f,0.0f),ApproachA(0.0f,0.0f),ApproachB(0.0f,0.0f),Span(0.0f),
		ApproachReserve(0.0f),BridgeKind(-1),TunnelModule(-1),TunnelLength(0.0f),DeckBottomZ(0.0f),
		DeckTopZ(0.0f),LowerSurfaceZ(0.0f),RampLength(0.0f),UpperIsThisEdge(true),CutDepth(0.0f),
		DeckStartZ(0.0f),DeckEndZ(0.0f),DeckReachA(0.0f),DeckReachB(0.0f),
		FirstPoint(0),LastPoint(0),Envelope(0),Score(0.0f),Role(HYBRID_JUNCTION_ROLE_NONE),JunctionZ(0.0f) { }

	//	DynamicVectorClass<T> requires == and !=.  The record's own id is its identity.
	bool	operator == (const HybridInteraction & that) const	{ return StableId == that.StableId; }
	bool	operator != (const HybridInteraction & that) const	{ return !(*this == that); }

	unsigned int				StableId;
	unsigned int				EdgeId;					// the road this solution belongs to
	unsigned int				OtherEdgeId;			// road x road only; 0 otherwise
	unsigned int				NodeId;					// a topology junction: the node its roads share; 0 otherwise
	HybridObstructionKind	Obstruction;
	unsigned int				ObstructionRecordId;	// river or landform StableId; 0 for road x road
	unsigned int				WindowId;				// the Phase-6 window used; 0 when none was needed
	bool							Mandatory;				// a frozen Phase-9 crossing or overpass obligation
	HybridInteractionMethod	Method;
	HybridRoadClass			RoadClass;				// the class that drives across it

	/*
	**	Why the first method in this obstruction's order did not win, when a later one did: the plan's orders
	**	are fixed, so a fallback is only legitimate when the method ahead of it refused for a named reason,
	**	and the record says which.  `HYBRID_PROBLEM_NONE` when the first method is the one compiled.
	*/
	HybridInteractionProblem	FallbackFrom;

	Vector2						Station;					// the compiled crossing point, quantized
	Vector2						Tangent;					// the road's direction through the station
	Vector2						ApproachA;				// the two straight approach-reserve endpoints
	Vector2						ApproachB;
	float							Span;
	float							ApproachReserve;		// max(8W, selected_bridge.required_approach_length)

	int							BridgeKind;				// index into the params' bridge kinds, -1 = none
	int							TunnelModule;			// index into the params' tunnel modules, -1 = none
	float							TunnelLength;

	/*
	**	Section 12.3's vertical geometry.  `DeckBottomZ`/`DeckTopZ` describe a bridge or an overpass deck;
	**	`LowerSurfaceZ` is the road passing underneath an overpass, and `RampLength` is its `L_ramp`.
	**	`CutDepth` is the centerline correction a road cut needs.  Everything unused for a method stays 0.
	*/
	float							DeckBottomZ;
	float							DeckTopZ;
	float							LowerSurfaceZ;
	float							RampLength;
	bool							UpperIsThisEdge;		// overpass only: which of the two roads rides over
	float							CutDepth;

	/*
	**	The road surface where it meets the structure at each end, in travel order: a bridge's two abutments, a
	**	tunnel's two portals, an overpass deck's two ends.  A river bridge and an overpass hold a level deck, so
	**	both equal `DeckTopZ`; a canyon deck runs between the two road profiles (Section 12.3) and a bore between
	**	its portals, so theirs are the road's own solved elevations there, frozen here for Phase 12 to pin.
	*/
	float							DeckStartZ;
	float							DeckEndZ;

	/*
	**	How far the deck or bore reaches from `Station` along `Tangent`, back towards `ApproachA` and on towards
	**	`ApproachB`: a bridge's two abutment ends, a tunnel's two portals, an overpass deck's two ends.  These are
	**	the structure's own endpoints, which Phase 12 hands to BridgeSystem / TunnelSystem.
	*/
	float							DeckReachA;
	float							DeckReachB;

	int							FirstPoint;				// the owning route's sample range this solution covers
	int							LastPoint;

	/*
	**	The horizontal centerline the road is realized on through this solution, from where it leaves the
	**	Phase-10 centerline to where it rejoins it (Hybrid_Build_Crossing_Path).  An at-grade junction has
	**	none -- each road keeps its own centerline through it.  An overpass's is the upper road's own
	**	centerline over its two ramps and the deck, indexed into that road's realized centerline.
	*/
	HybridCrossingPath		Path;
	int							Envelope;				// the Phase-10 envelope the owning route was accepted in
	float							Score;					// the bridge candidate score, 0 for other methods

	/*
	**	The compiled junction role (Section 12.1).  The three highway access roles are set where they are
	**	compiled; for every other road x road solution it is left NONE and Hybrid_Interaction_Junction_Role
	**	reads it from what the record already says -- a reserved overpass, a shared node, or incidental.
	*/
	HybridJunctionRole		Role;

	/*
	**	Section 12.6.10's compiled elevation of a highway access junction: z_L for a HIGHWAY_RAMP_LOCAL_JUNCTION,
	**	the highway's surface at the gore for a HIGHWAY_RAMP_MERGE / DIVERGE.  0 for every other solution.
	*/
	float							JunctionZ;
};

HybridJunctionRole	Hybrid_Interaction_Junction_Role(const HybridInteraction & one);


/*
**	Section 12.6.10 item 5: one HIGHWAY_RAMP_MERGE / DIVERGE solution's samples at `min(W/3,A/4)` along the
**	highway, in chain order.  `Full` is true over the full-width auxiliary lane, false over the taper (Q-026);
**	`U` is the taper parameter (0 in the full lane), `D` the auxiliary-lane centre offset, `H` and `E` the
**	widened ramp-side carriageway and roadbed half widths, `Z` the surface elevation.
*/
struct HybridMergeDivergeSample
{
	HybridMergeDivergeSample(void) : Station(0.0f),Full(false),U(0.0f),D(0.0f),H(0.0f),E(0.0f),Z(0.0f) { }

	bool	operator == (const HybridMergeDivergeSample & that) const	{ return Station == that.Station; }
	bool	operator != (const HybridMergeDivergeSample & that) const	{ return !(*this == that); }

	float	Station;
	bool	Full;
	float	U;
	float	D;
	float	H;
	float	E;
	float	Z;
};


struct HybridMergeDiverge
{
	HybridMergeDiverge(void) : StableId(0),InteractionId(0),RampEdgeId(0),HighwayEdgeId(0),InterchangeId(0),
		Role(HYBRID_JUNCTION_ROLE_NONE),Side(HYBRID_INTERCHANGE_SIDE_NONE) { }

	bool	operator == (const HybridMergeDiverge & that) const	{ return StableId == that.StableId; }
	bool	operator != (const HybridMergeDiverge & that) const	{ return !(*this == that); }

	unsigned int				StableId;
	unsigned int				InteractionId;		// the HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_MERGE/DIVERGE record
	unsigned int				RampEdgeId;
	unsigned int				HighwayEdgeId;
	unsigned int				InterchangeId;
	HybridJunctionRole		Role;					// MERGE or DIVERGE
	HybridInterchangeSide	Side;
	HybridRampInterval		Interval;			// gore station, interval, full lane and taper
	DynamicVectorClass<HybridMergeDivergeSample>	Samples;
};



//	One unsolved conflict, kept whether or not the attempt survived it.  At most 12 per edge.
struct HybridInteractionIssue
{
	HybridInteractionIssue(void) :
		EdgeId(0),OtherEdgeId(0),Obstruction(HYBRID_OBSTRUCTION_NONE),WindowId(0),Mandatory(false),
		Problem(HYBRID_PROBLEM_NONE),StationsTried(0),Station(0.0f,0.0f) { }

	bool	operator == (const HybridInteractionIssue & that) const
	{
		return (EdgeId == that.EdgeId) && (WindowId == that.WindowId) && (Problem == that.Problem);
	}
	bool	operator != (const HybridInteractionIssue & that) const	{ return !(*this == that); }

	unsigned int				EdgeId;
	unsigned int				OtherEdgeId;
	HybridObstructionKind	Obstruction;
	unsigned int				WindowId;
	bool							Mandatory;
	HybridInteractionProblem	Problem;
	int							StationsTried;
	Vector2						Station;
};


struct HybridInteractionSet;

/*
**	One road's realized horizontal centerline: its Phase-10 centerline with each of its own compiled
**	crossing paths put in place of the stretch that path departs and rejoins, stations measured again, and
**	each crossing's structural interval re-indexed onto the new points with the crossing's own compiled ends
**	as its ingress and egress.  Junctions and overpasses leave a road's centerline where it is.  Phase 11
**	finds road x road crossings on these centerlines, and Phase 12 realizes them; it is the one function
**	both use, so the two can never disagree about where a road runs.
*/
bool	Hybrid_Realized_Route(const HybridRoute & route,const HybridInteractionSet & set,HybridRoute * out);


/*
**	Every structure one road's compiled crossings put on it, on that road's realized centerline
**	(Hybrid_Realized_Route): each deck or bore's two ends along the road and the surface it holds -- a river or
**	overpass deck level at its top, a canyon or ravine deck and a tunnel bore carried between the end elevations
**	the record froze.  Section 12.1 over the realized road with these is the road's final profile; Phase 11
**	judges candidates with it and Phase 12 realizes it.  False when a structure cannot be placed on the road.
*/
struct HybridProfileStructure;
bool	Hybrid_Road_Profile_Structures(const HybridInteractionSet & set,const HybridRoute & realized,
												 DynamicVectorClass<HybridProfileStructure> * out,
												 const DynamicVectorClass<unsigned int> * edges = nullptr);

/*
**	The physical road a logical edge is part of, realized (Q-025): a heavy family's whole trunk -- each of its
**	logical edges realized with its own crossings and the slices joined end to end, one continuous road -- or
**	the edge's own realized road when it is a road of its own.  `edges_out` receives the logical edges it
**	carries, in travel order, for Hybrid_Road_Profile_Structures.  Section 12.1 grades a road over this.
*/
bool	Hybrid_Realized_Physical_Road(const HybridRouteSet & routes,const HybridInteractionSet & set,
												unsigned int edge_id,HybridRoute * out,
												DynamicVectorClass<unsigned int> * edges_out);


/*
**	Runtime Phase 11's output.
*/
struct HybridInteractionSet
{
	HybridInteractionSet(void)	{ Reset (); }

	void	Reset(void);

	const HybridInteraction *	Find(unsigned int stable_id) const;

	//	The solution compiled for one road's crossing of one window, or null when there is none.
	const HybridInteraction *	Find_For_Window(unsigned int edge_id,unsigned int window_id) const;

	int	Count_Of_Method(HybridInteractionMethod method) const;
	int	Count_Of_Method_On_Obstruction(HybridInteractionMethod method,
													HybridObstructionKind obstruction) const;

	DynamicVectorClass<HybridInteraction>			Interactions;
	DynamicVectorClass<HybridInteractionIssue>	Issues;
	DynamicVectorClass<HybridMergeDiverge>			MergeDiverge;		// Section 12.6.10 item 5, StableId order

	int				StationsEvaluated;		// work actually done, against the Section-16.1 budgets
	int				SweepSamples;
	int				RerouteRequests;			// edges Phase 11 asked to spend another Phase-10 envelope
	unsigned int	Checksum;
};


/*
**	Everything Phase 11 needs that is not already in a frozen record.  The bridge and tunnel metrics are
**	resolved by the caller from the registry and `BridgePlannerParams`; the vehicle and fairness numbers
**	come from the frozen preflight metric cache.
*/
struct HybridInteractionParams
{
	HybridInteractionParams(void);

	HybridConstants	Constants;

	float					WorldMinX;
	float					WorldMinY;
	float					WorldWidth;
	float					WorldHeight;

	float					MaxCutDepth;						// Section 7.1's vertical-interval inputs
	float					MaxFillHeight;
	float					RiverWidth;

	bool					AllowFords;							// plan.allow_fords
	bool					FordCapabilityAvailable;		// preflight: a canonical fordability query exists
	bool					TunnelsEnabled;					// world_features.tunnels
	bool					TunnelAvailable;					// preflight: tunnel_available

	float					WorldUnitsPerMeter;
	float					HeavyVehicleWidth;				// the swept clearance reference
	float					HeavyVehicleHeight;
	float					HarvesterWidth;
	float					LightVehicleWidth;				// the Humvee reference: what a trail or service road carries
	float					MinVehicleLaneClearance;		// fairness.min_vehicle_lane_clearance
	float					TierStep;							// Section 12.3's elevation term denominator

	float					BridgeWaterClearanceMargin;
	int					BridgeForClass[HYBRID_ROAD_CLASS_COUNT];		// registry index, -1 = none carries it
	float					BridgeRequiredWidth[HYBRID_ROAD_CLASS_COUNT];	// Section 12.4's clear-width requirement

	int					MaxLocalRetriesPerStage;
	int					MaxStationsPerCrossing;			// 64
	int					MaxProblemsPerEdge;				// 12

	DynamicVectorClass<HybridBridgeKind>		BridgeKinds;
	DynamicVectorClass<HybridTunnelModule>	TunnelModules;

	HybridScratchArena *		Arena;
	HybridWorkCounters *		Counters;
};


enum HybridInteractionFailure
{
	HYBRID_INTERACTION_OK = 0,
	HYBRID_INTERACTION_INVALID_INPUT,		// no routes, no topology, or degenerate constants
	HYBRID_INTERACTION_NO_SOLUTION,			// a conflict exhausted its fixed method order
	HYBRID_INTERACTION_OBLIGATION_LOST,		// a mandatory obligation could not keep its required method
	HYBRID_INTERACTION_QUOTA,					// a Section-5 bridge or overpass quota is unmet
	HYBRID_INTERACTION_SWEEP,					// a compiled interval failed its swept proxy validation
	HYBRID_INTERACTION_LIMIT,					// more than 12 problems on one edge, or a work budget
	HYBRID_INTERACTION_DETACHED,				// a compiled crossing is not attached to the road that drives it
	HYBRID_INTERACTION_INTERCHANGE,			// an active interchange's intended junctions or elevations do not compile (12.6.10)
	HYBRID_INTERACTION_FAILURE_COUNT
};

const char *	Hybrid_Interaction_Failure_Name(HybridInteractionFailure failure);

/*
**	What a road x road meeting is.  Two edges that end at one topology node meet there by design (`SHARED`);
**	a Phase-9 `OVERPASS_OBLIGATION` is a reserved crossing (`OBLIGATION`); anything else the two realized
**	centerlines do is `INCIDENTAL` and gets the junction / overpass / reroute order.
*/
enum HybridRoadMeeting
{
	HYBRID_ROAD_MEETING_SHARED = 0,
	HYBRID_ROAD_MEETING_OBLIGATION,
	HYBRID_ROAD_MEETING_INCIDENTAL,
	HYBRID_ROAD_MEETING_COUNT
};

//	The road x road accounting of one Phase-11 attempt, and the crossing that failed it, if one did.
struct HybridRoadCrossingCensus
{
	HybridRoadCrossingCensus(void)	{ Reset(); }
	void	Reset(void);

	int		Meetings[HYBRID_ROAD_MEETING_COUNT];	// every meeting found, by class
	int		IncidentalNearShared;	// crossings of two edges that share a node, away from that node
	int		IncidentalBeyondJunction;	// ...and beyond the junction's own arm reach there
	int		WaypointShared;			// edge pairs where one ends at a waypoint of the other
	int		FailedMeeting;				// HybridRoadMeeting of the failure, -1 when none failed
	int		FailedKindA;				// HybridTopologyEdgeKind of each edge
	int		FailedKindB;
	float		FailedAngle;				// degrees between the two roads at the crossing, 0..90
	float		FailedEndA;					// distance from the crossing to each edge's nearer endpoint node
	float		FailedEndB;
	int		FailedEndKindA;			// HybridTopologyNodeKind of that nearer endpoint node
	int		FailedEndKindB;
	bool		FailedWaypointShared;	// one edge ends at a node the other carries as a waypoint
	int		JunctionProblem;			// HybridInteractionProblem
	int		OverpassProblem[2];		// the first assignment, then the swap
};


struct HybridInteractionResult
{
	HybridInteractionResult(void) : Failure(HYBRID_INTERACTION_OK),RerouteEdgeId(0) { }

	HybridInteractionFailure	Failure;
	StringClass						Detail;

	//	Set when the solver wants one edge rerouted through its next Phase-10 envelope before failing.
	unsigned int					RerouteEdgeId;

	HybridRoadCrossingCensus	RoadCrossings;
};


/*
**	Runtime Phase 11.  Reads the frozen Phase 5, 6, 8, 9 and 10 records and the realized macro terrain
**	through `sample`; writes one solution per conflict.  Deterministic: no draw, no clock, no float
**	comparison the Section 0.3 quantization does not bound.
*/
bool	Hybrid_Build_Interactions(const HybridInteractionParams & params,const HybridMapIdentity & identity,
											const HybridCoarseMatrices & matrices,const HybridLandformSet & landforms,
											const HybridHydrologySet & hydrology,const HybridAnchorSet & anchors,
											const HybridTopology & topology,const HybridRouteSet & routes,
											HybridRouteHeightSampleFunc sample,void * context,
											HybridInteractionSet * out,HybridInteractionResult * result);

unsigned int	Hybrid_Interaction_Checksum(const HybridInteractionSet & set,unsigned int seed = 0);


/*
**	The most reroutes one *edge* may spend: the plan gives a road three corridor envelopes, so two of them
**	can ever be reached by a reroute.  There is no per-world cap -- each edge's envelope floor only rises,
**	which is what bounds the loop.
*/
#define HYBRID_MAX_REROUTES_PER_EDGE 2

/*
**	Phase 11 over an already-frozen Phase-10 route set, with the plan's next-envelope reroute behind it.
**
**	`routes` comes in built: the normal path runs Phase 10 exactly once, and this consumes what it froze.
**	Phase 10 runs again only when Phase 11 actually asks for another envelope, so a world that needs no
**	reroute pays for one routing pass and no more.  `route_params` is taken mutable because a reroute records
**	which edge may no longer use its narrower envelopes; `reroutes_out` counts the reroutes spent, and
**	`phase10_runs_out` the total Phase-10 executions including the frozen one the caller already made -- 1
**	with no reroute, 2 with one, 3 with two.
*/
bool	Hybrid_Solve_Interactions_With_Reroute(HybridRouteParams & route_params,
															const HybridInteractionParams & interaction_params,
															const HybridMapIdentity & identity,
															const HybridCoarseMatrices & matrices,
															const HybridLandformSet & landforms,
															const HybridHydrologySet & hydrology,
															const HybridAnchorSet & anchors,
															const HybridTopology & topology,
															HybridRouteHeightSampleFunc sample,void * context,
															HybridRouteSet * routes,HybridRouteResult * route_result,
															HybridInteractionSet * interactions,
															HybridInteractionResult * interaction_result,
															int * reroutes_out = nullptr,
															int * phase10_runs_out = nullptr);


/*
**	Section 12.3's road-over-road clearance, exported because Phase 12 verifies the same number against
**	the realized geometry and neither phase may hold its own copy of it.
*/
float	Hybrid_Overpass_Clear_Gap(float heavy_vehicle_height,float world_units_per_meter);

//	Section 12.4's minimum clear bridge width for one road class.
float	Hybrid_Bridge_Clear_Width(HybridRoadClass road_class,float lane_width,float swept_vehicle_width,
											float min_lane_clearance);



/*
**	Section 8 Phase 11's station list for one crossing problem (Q-022): candidate 0 is the problem's own anchor
**	-- `centre` projected onto the window's obstruction, which for a routed crossing is the window's own
**	station -- then the rest of `-max(8W,4S) .. +max(8W,4S)` along the obstruction at `min(W,A/2)`, both ends
**	included where they are on the obstruction, never more than `MaxStationsPerCrossing` in all.  Exported so
**	the gate can hold the list to that rule; the solver builds its own with this same function.
*/
void	Hybrid_Crossing_Station_List(const HybridInteractionParams & params,const HybridHydrologySet & hydrology,
											const HybridLandformSet & landforms,const HybridCrossingWindow & window,
											const Vector2 & centre,DynamicVectorClass<HybridObstructionStation> * out);


#endif	// HYBRIDINTERACTIONS_H
