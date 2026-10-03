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
 *                     $Archive:: /Commando/Code/wwphys/hybridtopology.h                      $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Hybrid world generator rewrite, item I10 -- runtime Phase 9, the transportation topology      *
 * (Procedural_World_Generator_Rewrite_Plan_FINAL_AUDITED.md Section 8 Phase 9).                 *
 *                                                                                             *
 * This phase decides *which* roads exist, what class each one is, and which frozen points every  *
 * one of them must pass through.  It decides no terrain path at all: Phase 10 routes each edge   *
 * on the analysis grid, and it may not rediscover a crossing this phase did not record.          *
 *                                                                                             *
 * The plan's fourteen construction steps run in its exact order -- base entrances, the family    *
 * chains and their crossing obligations, the two bridge quotas, the highway assignment, the      *
 * field access points, the home roads, the field service branches, the service and trail quota   *
 * fills, the optional cross-links and the two overpass passes -- because several of them consume *
 * what an earlier one produced: a physical crossing window satisfies exactly one obligation, and  *
 * an overpass obligation is one waypoint *shared* by two edges.                                 *
 *                                                                                             *
 * It replaces the native MST road topology: `RoadGraphGenerator`'s minimum spanning tree is not   *
 * consulted on the hybrid path, and the authored/legacy path is untouched.                       *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef HYBRIDTOPOLOGY_H
#define HYBRIDTOPOLOGY_H

#include "always.h"
#include "vector.h"
#include "vector2.h"
#include "wwstring.h"
#include "hybridplanning.h"
#include "hybridmapidentity.h"
#include "hybridstrategicgraph.h"
#include "hybridcoarsematrix.h"
#include "hybridlandforms.h"
#include "hybridhydrology.h"
#include "hybridanchors.h"


/*
**	The node kinds the plan names.  The ordinal is part of every deterministic sort key, so never
**	reorder.
*/
enum HybridTopologyNodeKind
{
	HYBRID_NODE_BASE = 0,					// the logical base node, which owns its entrance points
	HYBRID_NODE_BASE_ENTRANCE,				// BASE_ENTRANCE_POINT: one per heavy family per base
	HYBRID_NODE_FAMILY_WAYPOINT,			// a Phase-3 route-family waypoint
	HYBRID_NODE_CROSSING_OBLIGATION,		// a mandatory crossing, bound to one Phase-6 window
	HYBRID_NODE_OVERPASS_OBLIGATION,		// a grade-separated road-over-road crossing, shared by two edges
	HYBRID_NODE_FIELD_ACCESS,				// FIELD_ACCESS_POINT: where a field's road actually ends
	HYBRID_NODE_INTERCHANGE_LOCAL_JUNCTION,	// Section 12.6: L_s, where one side's ramps meet the lower roads
	HYBRID_NODE_KIND_COUNT
};

const char *	Hybrid_Topology_Node_Kind_Name(HybridTopologyNodeKind kind);


/*
**	The edge kinds, which are what Section-5's road quotas are counted over.
*/
enum HybridTopologyEdgeKind
{
	HYBRID_EDGE_BASE_INTERNAL = 0,		// zero cost, never realized, contracted in the Section-12.2 max-flow
	HYBRID_EDGE_FAMILY_TRUNK,				// a heavy route family's own chain
	HYBRID_EDGE_HOME_ROAD,					// a base entrance to its own home field's access point
	HYBRID_EDGE_FIELD_BRANCH,				// a contested/remote field's service branch
	HYBRID_EDGE_SERVICE_FILL,				// quota-filling SERVICE_DIRT_2 between family waypoints
	HYBRID_EDGE_TRAIL,						// TRAIL_1 infantry shortcut
	HYBRID_EDGE_CROSS_LINK,					// optional heavy cross-link, or a synthesized bridge/overpass link
	HYBRID_EDGE_HIGHWAY_RAMP,				// Section 12.6: one ON_RAMP or OFF_RAMP, constructed, never searched
	HYBRID_EDGE_INTERCHANGE_CROSSING,	// Section 12.6: a DIAMOND_ACCESS crossing joining its two local junctions
	HYBRID_TOPOLOGY_EDGE_KIND_COUNT
};

const char *	Hybrid_Topology_Edge_Kind_Name(HybridTopologyEdgeKind kind);


/*
**	The navigation capabilities one road edge is required to carry -- Section 12.1's "required navigation
**	capabilities", which every road record stores.
**
**	They are decided here, where the edge is created and its *role* is known, and they are copied unchanged
**	into every later record.  They are deliberately not derived from the road class: class decides the
**	cross section, grade, cross slope, turn radius and render tier, while which vehicles an edge must carry
**	is decided by what the edge is for, and two edges of one class can legitimately differ.  Section 8
**	Phase 12's swept-envelope checks name the three the plan uses:
**
**		Humvee/light reference -> every road
**		heavy reference         -> every heavy-capable strategic edge
**		Harvester envelope      -> every home/resource economy edge
*/
enum HybridNavCapability
{
	HYBRID_NAV_LIGHT			= 1 << 0,
	HYBRID_NAV_HEAVY			= 1 << 1,
	HYBRID_NAV_HARVESTER		= 1 << 2,
};

//	The mask Phase 9 assigns to an edge of this kind.  `BASE_INTERNAL` is never realized and carries none.
unsigned int	Hybrid_Topology_Edge_Capabilities(HybridTopologyEdgeKind kind,bool heavy_link);

//	The swept width a capability mask requires: the widest of the reference vehicles it names (Section 12.4).
float	Hybrid_Required_Swept_Width(unsigned int capabilities,float light_width,float heavy_width,float harvester_width);


struct HybridTopologyNode
{
	HybridTopologyNode(void) :
		StableId(0),Kind(HYBRID_NODE_FAMILY_WAYPOINT),Position(0.0f,0.0f),RouteFamily(-1),Team(-1),
		SiteId(0),WindowId(0),SectorOrdinal(-1) { }

	//	DynamicVectorClass<T> (wwlib/vector.h) requires T to support == and !=.  StableId is the key.
	bool	operator == (const HybridTopologyNode & that) const	{ return StableId == that.StableId; }
	bool	operator != (const HybridTopologyNode & that) const	{ return !(*this == that); }

	unsigned int				StableId;			// creation order, 1-based
	HybridTopologyNodeKind	Kind;
	Vector2						Position;
	int							RouteFamily;		// -1 when the node belongs to no single family
	int							Team;					// 0 GDI, 1 Nod, -1 neutral
	unsigned int				SiteId;				// the strategic site this came from, 0 when synthesized
	unsigned int				WindowId;			// the Phase-6 crossing window this is bound to, 0 when none
	int							SectorOrdinal;		// base entrance only: the frozen Phase-8 exit sector
};


/*
**	Section 12.6's highway access vocabulary.  Ordinals are part of the checksum: never reorder.
*/
//	HybridRampRole is in hybridplanrecords.h: the shared planning helpers construct ramps by role.

enum HybridInterchangeSide
{
	HYBRID_INTERCHANGE_SIDE_RIGHT = 0,	// n_RIGHT = -n_LEFT; carriageway travel tau_RIGHT = a
	HYBRID_INTERCHANGE_SIDE_LEFT,			// n_LEFT = rotate(a,+90); tau_LEFT = -a
	HYBRID_INTERCHANGE_SIDE_COUNT,
	HYBRID_INTERCHANGE_SIDE_NONE = HYBRID_INTERCHANGE_SIDE_COUNT
};

enum HybridInterchangeForm
{
	HYBRID_INTERCHANGE_DIAMOND_ACCESS = 0,
	HYBRID_INTERCHANGE_SIDE_ACCESS,
	HYBRID_INTERCHANGE_FORM_COUNT
};

enum HybridInterchangeStatus
{
	HYBRID_INTERCHANGE_ACTIVE = 0,
	HYBRID_INTERCHANGE_WITHDRAWN,			// Phase 10's only fallback (12.6.9): the frozen Phase-9 variant restored
	HYBRID_INTERCHANGE_STATUS_COUNT
};

enum HybridInterchangeCrossingMode
{
	HYBRID_INTERCHANGE_CROSSING_NONE = 0,			// SIDE_ACCESS
	HYBRID_INTERCHANGE_CROSSING_GRADE_SEPARATED,	// one reserved OVERPASS_OBLIGATION at the anchor
	HYBRID_INTERCHANGE_CROSSING_AT_GRADE,			// two edges through the anchor, an intended at-grade junction
	HYBRID_INTERCHANGE_CROSSING_MODE_COUNT
};

//	Why Phase 9 step 15 rejected an anchor (a diagnostic; rejection is never a phase failure).
enum HybridInterchangeRejection
{
	HYBRID_INTERCHANGE_REJECT_AXIS = 0,				// the two highway directions cancel (|sum| < Q)
	HYBRID_INTERCHANGE_REJECT_ARRIVAL_ANGLE,		// unused since D-023 (no arrival-angle prefilter); kept for its ordinal
	HYBRID_INTERCHANGE_REJECT_ARRIVAL_SERVED,		// an access arrival already belongs to another interchange
	HYBRID_INTERCHANGE_REJECT_CROSSING,				// DIAMOND: neither overpass nor at-grade crossing is feasible
	HYBRID_INTERCHANGE_REJECT_NO_CANDIDATE,			// no k in 0..K_GORE-1 passed 12.6.8
	HYBRID_INTERCHANGE_REJECT_QUALIFICATION,		// a retargeted quota edge fell below its Section-5 length
	HYBRID_INTERCHANGE_REJECT_EDGE_CAP,				// the realized-edge count would exceed R
	HYBRID_INTERCHANGE_REJECTION_COUNT
};

const char *	Hybrid_Ramp_Role_Name(HybridRampRole role);
const char *	Hybrid_Interchange_Side_Name(HybridInterchangeSide side);
const char *	Hybrid_Interchange_Form_Name(HybridInterchangeForm form);
const char *	Hybrid_Interchange_Status_Name(HybridInterchangeStatus status);
const char *	Hybrid_Interchange_Crossing_Mode_Name(HybridInterchangeCrossingMode mode);
const char *	Hybrid_Interchange_Rejection_Name(HybridInterchangeRejection rejection);


/*
**	One ramp's highway-parallel interval, in chain station relative to the anchor `X` along the highway axis
**	`a` (12.6.7: for RIGHT the station is the side-frame `x`, for LEFT it is `-x`), with the Q-026 subdivisions:
**	an ON_RAMP's full auxiliary lane follows its gore and the merge taper ends the interval; an OFF_RAMP's
**	diverge taper begins it and the full auxiliary lane runs to its gore.  `From < To` in chain order.
*/
struct HybridRampInterval
{
	HybridRampInterval(void) : GoreStation(0.0f),From(0.0f),To(0.0f),FullFrom(0.0f),FullTo(0.0f),TaperFrom(0.0f),
		TaperTo(0.0f) { }

	float	GoreStation;
	float	From;
	float	To;
	float	FullFrom;					// the full-width auxiliary lane
	float	FullTo;
	float	TaperFrom;					// the merge or diverge taper
	float	TaperTo;
};


struct HybridTopologyEdge
{
	HybridTopologyEdge(void) :
		StableId(0),Kind(HYBRID_EDGE_FAMILY_TRUNK),RoadClass(HYBRID_ROAD_CLASS_COUNT),NodeA(0),NodeB(0),
		RouteFamily(-1),Required(false),ObligationId(0),SourceEdgeId(0),Capabilities(0),Length(0.0f),
		InterchangeId(0),RampRole(HYBRID_RAMP_ROLE_NONE),RampSide(HYBRID_INTERCHANGE_SIDE_NONE),RampRoadbedExtra(0.0f) { }

	bool	operator == (const HybridTopologyEdge & that) const	{ return StableId == that.StableId; }
	bool	operator != (const HybridTopologyEdge & that) const	{ return !(*this == that); }

	bool	Is_Realized(void) const	{ return Kind != HYBRID_EDGE_BASE_INTERNAL; }

	unsigned int				StableId;			// creation order, 1-based
	HybridTopologyEdgeKind	Kind;
	HybridRoadClass			RoadClass;			// HYBRID_ROAD_CLASS_COUNT for BASE_INTERNAL only
	unsigned int				NodeA;				// HybridTopologyNode::StableId
	unsigned int				NodeB;
	int							RouteFamily;
	bool							Required;			// a mandatory edge: Phase 10 may not drop it
	unsigned int				ObligationId;		// the Phase-3 crossing obligation it satisfies, 0 when none
	unsigned int				SourceEdgeId;		// the Phase-3 intent edge it came from, 0 when synthesized
	unsigned int				Capabilities;		// HybridNavCapability bits: what this edge must carry
	float							Length;				// quantized straight graph length through its waypoints

	/*
	**	The frozen intermediate waypoints, in order from NodeA to NodeB: crossing obligations and
	**	shared overpass obligations.  Phase 10 routes sub-legs through exactly these.
	*/
	DynamicVectorClass<unsigned int>	Waypoints;

	//	HIGHWAY_RAMP and INTERCHANGE_CROSSING edges only (Section 12.6): the owning interchange; ramps also
	//	carry their role, side and highway-parallel interval.  NodeA is the local junction, NodeB the anchor.
	unsigned int				InterchangeId;
	HybridRampRole				RampRole;
	HybridInterchangeSide	RampSide;
	HybridRampInterval		RampInterval;

	//	D-024: HIGHWAY_RAMP only -- `E`, the widening either side that carries its capabilities' widest vehicle.
	float							RampRoadbedExtra;
};

//	An edge's paved roadbed width: its class's, and for a ramp D-024's effective `max(1.50W, swept)`.
float	Hybrid_Edge_Roadbed_Width(const HybridTopologyEdge & edge,float w);


/*
**	One assigned base entrance, the plan's `(base,family_id,sector_ordinal,entrance_point,
**	entrance_corridor)` tuple.
*/
struct HybridBaseEntrance
{
	HybridBaseEntrance(void) :
		BaseSiteId(0),NodeId(0),Team(-1),RouteFamily(-1),SectorOrdinal(-1),Reused(false),
		Point(0.0f,0.0f),CorridorA(0.0f,0.0f),CorridorB(0.0f,0.0f),CorridorHalfWidth(0.0f) { }

	bool	operator == (const HybridBaseEntrance & that) const	{ return NodeId == that.NodeId; }
	bool	operator != (const HybridBaseEntrance & that) const	{ return !(*this == that); }

	unsigned int	BaseSiteId;
	unsigned int	NodeId;					// the HYBRID_NODE_BASE_ENTRANCE node
	int				Team;
	int				RouteFamily;
	int				SectorOrdinal;			// a frozen Phase-8 viable sector
	bool				Reused;					// every viable sector was already taken, so one is shared

	Vector2			Point;					// R_BASE_PROTECTED + W along the sector centerline
	Vector2			CorridorA;				// the capsule: R_BASE_PROTECTED - W ...
	Vector2			CorridorB;				// ... to R_BASE_PROTECTED + 6W
	float				CorridorHalfWidth;	// 0.5*RoadbedWidth(PRIMARY_2) + min_vehicle_lane_clearance
};


/*
**	One retargeted arrival of an interchange: the edge, the anchor it ended at (the frozen fallback variant
**	Phase 10's withdrawal restores) and the local junction it ends at while the interchange is active.
*/
struct HybridInterchangeRetarget
{
	HybridInterchangeRetarget(void) : EdgeId(0),FallbackNode(0),LocalJunction(0),Side(HYBRID_INTERCHANGE_SIDE_NONE) { }

	bool	operator == (const HybridInterchangeRetarget & that) const	{ return EdgeId == that.EdgeId; }
	bool	operator != (const HybridInterchangeRetarget & that) const	{ return !(*this == that); }

	unsigned int				EdgeId;
	unsigned int				FallbackNode;		// X
	unsigned int				LocalJunction;		// L_s
	HybridInterchangeSide	Side;
};


/*
**	Section 12.6's HIGHWAY_INTERCHANGE record (12.6.4 step 8, 12.6.15): one anchor `X` on a HIGHWAY_4 family,
**	its frame, the selected gore candidate and every record it created, by StableId.
*/
struct HybridInterchange
{
	HybridInterchange(void) :
		StableId(0),Form(HYBRID_INTERCHANGE_SIDE_ACCESS),Status(HYBRID_INTERCHANGE_ACTIVE),AnchorNode(0),
		HighwayEdgeIn(0),HighwayEdgeOut(0),Axis(1.0f,0.0f),AccessSides(0),CrossingMode(HYBRID_INTERCHANGE_CROSSING_NONE),
		CrossingClass(HYBRID_ROAD_CLASS_COUNT),SelectedK(-1),LocalOffset(0.0f),GoreOffset(0.0f),StraightHalfExtent(0.0f),
		CrossingObligation(0),Capabilities(0)
	{
		LocalJunction[0] = LocalJunction[1] = 0;
	}

	bool	operator == (const HybridInterchange & that) const	{ return StableId == that.StableId; }
	bool	operator != (const HybridInterchange & that) const	{ return !(*this == that); }

	unsigned int						StableId;
	HybridInterchangeForm			Form;
	HybridInterchangeStatus			Status;
	unsigned int						AnchorNode;			// X
	unsigned int						HighwayEdgeIn;		// the HIGHWAY_4 trunk edge arriving at X
	unsigned int						HighwayEdgeOut;	// ...and the one leaving it
	Vector2								Axis;					// a, unit
	unsigned int						AccessSides;		// bit HYBRID_INTERCHANGE_SIDE_*: sides with an access arrival
	HybridInterchangeCrossingMode	CrossingMode;
	HybridRoadClass					CrossingClass;		// PRIMARY_2 or SECONDARY_2 for DIAMOND_ACCESS
	int									SelectedK;			// the gore candidate, 0..K_GORE-1
	float									LocalOffset;		// d_L(k)
	float									GoreOffset;			// G(k)
	float									StraightHalfExtent;// S_I(k)
	unsigned int						LocalJunction[HYBRID_INTERCHANGE_SIDE_COUNT];	// L_s node, 0 when none
	unsigned int						CrossingObligation;	// the OVERPASS_OBLIGATION node, grade-separated only
	unsigned int						Capabilities;		// LIGHT | every retargeted arrival's capabilities
	DynamicVectorClass<unsigned int>				RampEdges;			// RIGHT then LEFT, OFF then ON
	DynamicVectorClass<unsigned int>				CrossingEdges;
	DynamicVectorClass<HybridInterchangeRetarget>	Retargets;			// the frozen fallback variant
};


/*
**	Runtime Phase 9's output.
*/
struct HybridTopology
{
	HybridTopology(void)	{ Reset (); }

	void	Reset(void);

	const HybridTopologyNode *	Find_Node(unsigned int stable_id) const;
	const HybridTopologyEdge *	Find_Edge(unsigned int stable_id) const;
	const HybridBaseEntrance *	Find_Entrance(int team,int family) const;
	const HybridInterchange *	Find_Interchange(unsigned int stable_id) const;

	int	Count_Of_Edge_Kind(HybridTopologyEdgeKind kind) const;
	int	Count_Of_Road_Class(HybridRoadClass road_class) const;
	int	Count_Of_Node_Kind(HybridTopologyNodeKind kind) const;
	int	Realized_Edge_Count(void) const;

	//	Section 8 Phase 9's gate: the realized graph must leave a cycle, never a tree.
	bool	Has_Cycle(void) const;

	DynamicVectorClass<HybridTopologyNode>		Nodes;
	DynamicVectorClass<HybridTopologyEdge>		Edges;
	DynamicVectorClass<HybridBaseEntrance>		Entrances;
	DynamicVectorClass<HybridInterchange>		Interchanges;		// Section 12.6, StableId order

	//	Section 5 quota bookkeeping, kept for the gate and the log.
	int			RequiredRiverBridges;
	int			RequiredCanyonBridges;
	int			RequiredHighways;
	int			RequestedOverpassTarget;	// Section 5's overpass number, a target since Q-024 (0 / 1 / 2)
	int			RequiredServiceBranches;
	int			RequiredTrails;
	int			OptionalCrossLinkQuota;

	int			RiverBridges;			// obligations/quota fills bound to river windows
	int			CanyonBridges;			// ...and to canyon/ravine windows
	int			Highways;
	int			ReservedOverpassCount;	// the hard-valid OVERPASS_OBLIGATIONs actually reserved
	int			ServiceBranches;
	int			Trails;
	int			CrossLinks;

	/*
	**	Q-023's census, not part of the frozen record: candidate structural bindings refused because no road of
	**	the edge's class could line up with the window on both banks, because the window faces away from the
	**	road's intent on either side, or because no class-radius road joins the resulting structural poses even
	**	on empty ground -- and bindings that took a lower-ranked window after a higher one was refused.
	*/
	int			RejectedApproachClass;
	int			RejectedWrongSide;
	int			RejectedPose;
	int			RecoveredByNextRank;

	//	Section 12.6.15's diagnostics: not part of the frozen record.
	int			RampCandidatesEvaluated;				// "highway ramp candidates evaluated"
	int			InterchangesByForm[HYBRID_INTERCHANGE_FORM_COUNT];
	int			RampsByRole[HYBRID_RAMP_ROLE_COUNT];
	int			InterchangeWithdrawals;
	int			AnchorRejections[HYBRID_INTERCHANGE_REJECTION_COUNT];
	int			CandidateRefusals[8];						// gore candidates refused, by the 12.6.8 test (1..7)
	int			AnchorsByForm[HYBRID_INTERCHANGE_FORM_COUNT];	// anchors framed and given candidates, by form
	int			StraightExtentRefusals[3];				// test 1 split: chain reach, pose before X, pose after X
	DynamicVectorClass<float>	AnchorTurnDegrees;			// each framed anchor: the chain's turn at X
	DynamicVectorClass<float>	AnchorNeighbourBefore;		// ...the distance to the stop before X along a
	DynamicVectorClass<float>	AnchorNeighbourAfter;		// ...and to the stop after it

	unsigned int	Checksum;			// Phase 9 gate value

	//	Q-024's diagnostic: how far the reserved overpasses fell short of the target.  Not an attempt failure.
	int	Overpass_Target_Shortage(void) const
	{
		return (RequestedOverpassTarget > ReservedOverpassCount) ? (RequestedOverpassTarget - ReservedOverpassCount) : 0;
	}
};


/*
**	Phase 9's inputs beyond the frozen Phase 2-8 records.  Plain values only.
*/
struct HybridTopologyParams
{
	HybridTopologyParams(void);

	HybridConstants	Constants;

	float					RoadDensity;					// plan.road_density -- the optional cross-link count
	float					FieldRadiusMax;				// resource.field_radius_max
	float					MinVehicleLaneClearance;
	float					HarvesterLength;				// the turning disc a field access point must clear
	int					MaxRoutedEdges;				// R: 18 / 32 / 52 by size class, from the work budgets
	bool					TunnelsAvailable;				// Section 5's tunnel toggle, for window method choice

	//	Section 12.6's planning inputs for step 15 (highway access interchanges).
	float					MaxCutDepth;					// grading.max_cut_depth
	float					MaxFillHeight;					// grading.max_fill_height
	float					OverpassClearGap;				// Section 12.3's minimum_clear_vertical_gap
	float					BridgeApproachMax[HYBRID_ROAD_CLASS_COUNT];	// preflight metric cache
	float					OverpassDepthMax[HYBRID_ROAD_CLASS_COUNT];

	//	The reference vehicles' swept widths (preflight metric cache): a ramp's roadbed carries its widest (D-024).
	float					LightVehicleWidth;
	float					HeavyVehicleWidth;
	float					HarvesterWidth;
};


/*
**	Phase 9 failures.  Each is a named attempt failure: the manager retries with a new seed.
*/
/*
**	A point a road has to pass, as Phase 9 freezes it (Q-023).  A structural waypoint -- a crossing window or an
**	overpass obligation -- has a traversal `Axis`, signed for travel along its edge, and straight reserves
**	`Before` and `After` it along that axis; an ordinary waypoint, a base entrance or a field access point has
**	no axis (zero) and may be passed on any heading.
*/
struct HybridRoadPose
{
	HybridRoadPose(void) : Point(0.0f,0.0f),Axis(0.0f,0.0f),Before(0.0f),After(0.0f) { }

	Vector2	Point;
	Vector2	Axis;
	float		Before;
	float		After;

	bool	Is_Structural(void) const	{ return Axis.Length() > 0.0f; }
};

enum HybridTopologyFailure
{
	HYBRID_TOPOLOGY_OK = 0,
	HYBRID_TOPOLOGY_INVALID_INPUT,				// no graph, no frozen anchors, or degenerate constants
	HYBRID_TOPOLOGY_NO_ENTRANCE_SECTOR,			// no viable sector fits a family's entrance corridor
	HYBRID_TOPOLOGY_OBLIGATION_UNBOUND,			// a mandatory crossing has no qualifying Phase-6 window
	HYBRID_TOPOLOGY_BRIDGE_QUOTA,					// a Section-5 bridge quota cannot be filled
	HYBRID_TOPOLOGY_NO_FIELD_ACCESS,				// no hard-valid FIELD_ACCESS_POINT exists for a field
	HYBRID_TOPOLOGY_SERVICE_QUOTA,				// too few candidate pairs for the dirt/service quota
	HYBRID_TOPOLOGY_TRAIL_QUOTA,					// too few candidate pairs for the trail quota
	HYBRID_TOPOLOGY_EDGE_BUDGET,					// more routed edges than R allows
	HYBRID_TOPOLOGY_NO_CYCLE,
	HYBRID_TOPOLOGY_UNREALIZABLE,			// gate: a structural waypoint sequence no road of its class can drive						// gate: a tree fails this phase
	HYBRID_TOPOLOGY_FAILURE_COUNT
};

const char *	Hybrid_Topology_Failure_Name(HybridTopologyFailure failure);

/*
**	Why a Section-5 bridge-quota candidate window was passed over, and by which binding path.  `FILTER` is the
**	window-level test every candidate takes first; `FAMILY` is the preferred window's insertion into its
**	nearest family, `CROSS_LINK` the plan's synthesized `SECONDARY_2` bank cross-link, and `FALLBACK` the
**	nearest-family binding a non-preferred window gets when the cross-link cannot be built.
*/
enum HybridQuotaRefusal
{
	HYBRID_QUOTA_REFUSE_METHOD = 0,				// the window has no bridge method
	HYBRID_QUOTA_REFUSE_APPROACH_CLASS,			// Phase 6 found no approach for the binding class
	HYBRID_QUOTA_REFUSE_CORRIDOR,					// outside the owning family's maximum corridor
	HYBRID_QUOTA_REFUSE_SPACING,					// within 12W of a selected window on its obstruction
	HYBRID_QUOTA_REFUSE_WRONG_SIDE,				// Q-023's wrong-side test
	HYBRID_QUOTA_REFUSE_POSE,						// Q-023's pose-connector test
	HYBRID_QUOTA_REFUSE_CONSUMED,					// already bound by an obligation or an earlier quota pass
	HYBRID_QUOTA_REFUSE_NO_BANK_PAIR,			// no heavy node on one of the two banks
	HYBRID_QUOTA_REFUSE_OTHER,						// no family, or no insertion slot on the family chain
	HYBRID_QUOTA_REFUSAL_COUNT
};

enum HybridQuotaPath
{
	HYBRID_QUOTA_PATH_FILTER = 0,
	HYBRID_QUOTA_PATH_FAMILY,
	HYBRID_QUOTA_PATH_CROSS_LINK,
	HYBRID_QUOTA_PATH_FALLBACK,
	HYBRID_QUOTA_PATH_COUNT
};

const char *	Hybrid_Quota_Refusal_Name(HybridQuotaRefusal refusal);
const char *	Hybrid_Quota_Path_Name(HybridQuotaPath path);

//	One bridge quota's last selection pass: the pass that failed, or the last one that succeeded.
struct HybridQuotaCensus
{
	HybridQuotaCensus(void)	{ Reset (); }
	void	Reset(void);

	int	Required;
	int	Have;
	int	Candidates;			// windows of the quota's own obstruction category considered in the pass
	int	Bindable;			// of those, the ones that could have been bound
	int	Refused[HYBRID_QUOTA_PATH_COUNT][HYBRID_QUOTA_REFUSAL_COUNT];
};

//	Why a step-12 or step-13 overpass candidate (two intent segments that cross) was passed over.
enum HybridOverpassRefusal
{
	HYBRID_OVERPASS_REFUSE_ANGLE = 0,			// outside 60..120 degrees
	HYBRID_OVERPASS_REFUSE_ARM,					// less than 10W of the highway segment on one side
	HYBRID_OVERPASS_REFUSE_ARM_OTHER,			// less than 10W of the other segment on one side
	HYBRID_OVERPASS_REFUSE_PROTECTED,			// unplaceable, water, an immutable wall or BASE_PROTECTED
	HYBRID_OVERPASS_REFUSE_WATER_CLEARANCE,	// closer to standing water than the router can step around
	HYBRID_OVERPASS_REFUSE_TIBERIUM,				// inside a Tiberium hard reserve
	HYBRID_OVERPASS_REFUSE_WINDOW,				// inside a selected crossing window's own bounds
	HYBRID_OVERPASS_REFUSE_SPACING,				// within 12W of a selected overpass
	HYBRID_OVERPASS_REFUSE_SLOPE,					// the 2W disc is steeper than SECONDARY_2's grade
	HYBRID_OVERPASS_REFUSE_POSE,					// Q-023: one of the two roads cannot drive it
	HYBRID_OVERPASS_REFUSAL_COUNT
};

const char *	Hybrid_Overpass_Refusal_Name(HybridOverpassRefusal refusal);

//	The overpass quota's last pass of each step.
struct HybridOverpassCensus
{
	HybridOverpassCensus(void)	{ Reset (); }
	void	Reset(void);

	int	Required;
	int	Have;
	int	Natural;				// step 12: highway x PRIMARY_2/SECONDARY_2 intent segments that cross
	int	NaturalRefused[HYBRID_OVERPASS_REFUSAL_COUNT];
	int	Pairs;				// step 13: heavy waypoint pairs eligible for a synthesized cross-link
	int	Synthesized;		// of those pairs, (pair, highway) segments that cross
	int	SynthesizedRefused[HYBRID_OVERPASS_REFUSAL_COUNT];
	int	Families;			// heavy route families, and how many of them are highways
	int	Highways;
	int	HeavyPairs;			// eligible pairs whose two nodes are both on heavy families
	int	AngleBucket[4];	// refused-for-angle crossings by angle between the lines: <15, <30, <45, <60
};


struct HybridTopologyResult
{
	HybridTopologyResult(void) :
		Failure(HYBRID_TOPOLOGY_OK),RejectedApproachClass(0),RejectedWrongSide(0),RejectedPose(0),
		RecoveredByNextRank(0) { }

	HybridTopologyFailure	Failure;
	StringClass					Detail;

	//	The Q-023 census of the attempt, kept whether or not it succeeded (HybridTopology's own copy is reset).
	int							RejectedApproachClass;
	int							RejectedWrongSide;
	int							RejectedPose;
	int							RecoveredByNextRank;

	//	The quota passes' own accounting, so a short quota says what stood in the way.
	HybridQuotaCensus			RiverQuota;
	HybridQuotaCensus			CanyonQuota;
	HybridOverpassCensus		OverpassQuota;
};


/*
**	Section 5's per-size-class road quotas, so the builder, the gate and Phase 19 share one owner.
*/
int	Hybrid_River_Bridge_Quota(HybridSizeClass size_class);			// 1 / 2 / 3
int	Hybrid_Canyon_Bridge_Quota(HybridSizeClass size_class);		// 1 / 1 / 2
int	Hybrid_Highway_Quota(HybridSizeClass size_class);				// 0 / 1 / 2
int	Hybrid_Overpass_Target(HybridSizeClass size_class);			// 0 / 1 / 2, a target (Q-024)
int	Hybrid_Service_Branch_Quota(HybridSizeClass size_class);		// 2 / 4 / 8
int	Hybrid_Trail_Quota(HybridSizeClass size_class);					// 1 / 2 / 4
int	Hybrid_Cross_Link_Cap(HybridSizeClass size_class);				// 2 / 5 / 10

//	Section 5: `extra_links = RoundHalfUp(road_density * cap)`.
int	Hybrid_Extra_Cross_Link_Count(HybridSizeClass size_class,float road_density);


/*
**	Section 12.2's deterministic unit-capacity edge-disjoint GDI-to-Nod max-flow over a topology, with
**	`BASE_INTERNAL` edges contracted (they are free connectivity inside one base, never a route).
**	Exposed because Phase 9's cross-link benefit score and Phase 19's connectivity check are the same
**	measurement and must not be two implementations of it.
*/
int	Hybrid_Topology_Max_Flow(const HybridTopology & topology,unsigned int gdi_base_node,
										unsigned int nod_base_node);

/*
**	Runtime Phase 9.  Runs the plan's fourteen construction steps in order over the frozen Phase 2-8
**	records and checks the phase gate.  Returns false with result->Failure set on the first failure,
**	leaving `out` empty.  `matrices` is taken mutable only to reserve each entrance corridor against
**	unrelated Tiberium and decoration, which is step 6's own instruction.
*/
typedef bool (*HybridTopologyHeightSampleFunc)(const Vector2 & world,float * height_out,void * context);

bool	Hybrid_Build_Topology(const HybridTopologyParams & params,const HybridMapIdentity & identity,
										const HybridStrategicGraph & graph,const HybridLandformSet & landforms,
										const HybridHydrologySet & hydrology,const HybridAnchorSet & anchors,
										HybridCoarseMatrices * matrices,HybridTopologyHeightSampleFunc sample,
										void * context,HybridTopology * out,HybridTopologyResult * result);

/*
**	The Phase 9 gate value: a quantized digest of every node, edge and entrance in StableId order.
**	Hybrid_Build_Topology stores it in out->Checksum.
*/
unsigned int	Hybrid_Topology_Checksum(const HybridTopology & topology,unsigned int seed = 0);

/*
**	Phase 9 step 15 alone (Section 12.6.4 / 12.6.8), on a topology whose steps 1-14 are already frozen -- what
**	Hybrid_Build_Topology runs after step 14, exposed for controlled worlds -- followed by the Phase-9 gate's
**	interchange clause.  `family_count` is the identity's route family count.  False, with `detail_out`, when the
**	gate clause refuses the result; a rejected anchor is never a failure.
*/
bool	Hybrid_Build_Interchanges(const HybridTopologyParams & params,const HybridLandformSet & landforms,
										const HybridHydrologySet & hydrology,const HybridAnchorSet & anchors,
										HybridCoarseMatrices * matrices,HybridTopologyHeightSampleFunc sample,void * context,
										int family_count,HybridTopology * topology,StringClass * detail_out);


/*
**	Q-023, structural-pose realizability.  Phase 9 chooses which physical windows become structural waypoints on
**	which edges, and it may not freeze a sequence no road of the edge's class could drive even on empty ground.
**
**	Hybrid_Window_Pose: the pose a crossing window presents to a road travelling from `previous` to `next`: its
**	centre, its own axis signed for that travel, and its whole frozen straight -- the obstruction and the `8W`
**	approach reserve either side -- as the reserves.
**
**	Hybrid_Overpass_Pose: an overpass obligation's pose on one participating road: the shared point, the road's
**	own intent from `previous` to `next`, and the `10W` Phase 9 reserves either side for its ramps.
**
**	Hybrid_Pose_Faces_Intent: neither side has the road arriving at, or leaving, the structure from behind --
**	the axis is within 90 degrees of the planned direction on both sides.
**
**	Hybrid_Window_Bindable: everything Phase 9 asks before a window may be ranked for one edge -- the class
**	approach capability Phase 6 recorded, the wrong-side test, and the route owner's connector from the
**	neighbour behind and to the neighbour ahead (Hybrid_Road_Poses_Connect).
**
**	Hybrid_First_Bindable_Window: the first of `count` windows, in the caller's own rank order, that is bindable;
**	-1 when none is.  An impossible candidate is passed over, never chosen and then failed.
*/
HybridRoadPose	Hybrid_Window_Pose(const HybridCrossingWindow & window,const Vector2 & previous,const Vector2 & next);
HybridRoadPose	Hybrid_Overpass_Pose(const Vector2 & point,const Vector2 & previous,const Vector2 & next,
											const HybridConstants & constants,float half_deck = 0.0f);

/*
**	Half the deck an overpass spans along one road, `axis`, where it crosses `other_axis`: the deck covers the
**	other roadbed at the angle the two meet, `w / sin(theta) + w |cot(theta)|`, with `w` the wider of the two
**	roadbeds.  The deck is a straight bridge, so the road keeps this straight either side of the shared point;
**	Phase 9 reserves at least it (and `10W`), and Phase 10 keeps it.  Zero for parallel roads.
*/
float	Hybrid_Overpass_Half_Deck(const Vector2 & axis,const Vector2 & other_axis,HybridRoadClass road_class,
										HybridRoadClass other_class,float lane_width);
bool	Hybrid_Pose_Faces_Intent(const Vector2 & previous,const HybridRoadPose & pose,const Vector2 & next);
bool	Hybrid_Window_Bindable(const HybridCrossingWindow & window,const HybridRoadPose & previous,
										const HybridRoadPose & next,HybridRoadClass road_class,const HybridConstants & constants,
										int * refusal_out = nullptr);
int	Hybrid_First_Bindable_Window(const HybridCrossingWindow * const * ranked,int count,
											const HybridRoadPose & previous,const HybridRoadPose & next,
											HybridRoadClass road_class,const HybridConstants & constants);

//	Hybrid_Window_Bindable's refusals, in the order it asks.
enum HybridBindingRefusal
{
	HYBRID_BINDING_OK = 0,
	HYBRID_BINDING_APPROACH_CLASS,
	HYBRID_BINDING_WRONG_SIDE,
	HYBRID_BINDING_POSE,
};


#endif // HYBRIDTOPOLOGY_H
