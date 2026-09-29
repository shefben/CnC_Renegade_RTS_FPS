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
	HYBRID_TOPOLOGY_EDGE_KIND_COUNT
};

const char *	Hybrid_Topology_Edge_Kind_Name(HybridTopologyEdgeKind kind);


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


struct HybridTopologyEdge
{
	HybridTopologyEdge(void) :
		StableId(0),Kind(HYBRID_EDGE_FAMILY_TRUNK),RoadClass(HYBRID_ROAD_CLASS_COUNT),NodeA(0),NodeB(0),
		RouteFamily(-1),Required(false),ObligationId(0),SourceEdgeId(0),Length(0.0f) { }

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
	float							Length;				// quantized straight graph length through its waypoints

	/*
	**	The frozen intermediate waypoints, in order from NodeA to NodeB: crossing obligations and
	**	shared overpass obligations.  Phase 10 routes sub-legs through exactly these.
	*/
	DynamicVectorClass<unsigned int>	Waypoints;
};


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
**	Runtime Phase 9's output.
*/
struct HybridTopology
{
	HybridTopology(void)	{ Reset (); }

	void	Reset(void);

	const HybridTopologyNode *	Find_Node(unsigned int stable_id) const;
	const HybridTopologyEdge *	Find_Edge(unsigned int stable_id) const;
	const HybridBaseEntrance *	Find_Entrance(int team,int family) const;

	int	Count_Of_Edge_Kind(HybridTopologyEdgeKind kind) const;
	int	Count_Of_Road_Class(HybridRoadClass road_class) const;
	int	Count_Of_Node_Kind(HybridTopologyNodeKind kind) const;
	int	Realized_Edge_Count(void) const;

	//	Section 8 Phase 9's gate: the realized graph must leave a cycle, never a tree.
	bool	Has_Cycle(void) const;

	DynamicVectorClass<HybridTopologyNode>		Nodes;
	DynamicVectorClass<HybridTopologyEdge>		Edges;
	DynamicVectorClass<HybridBaseEntrance>		Entrances;

	//	Section 5 quota bookkeeping, kept for the gate and the log.
	int			RequiredRiverBridges;
	int			RequiredCanyonBridges;
	int			RequiredHighways;
	int			RequiredOverpasses;
	int			RequiredServiceBranches;
	int			RequiredTrails;
	int			OptionalCrossLinkQuota;

	int			RiverBridges;			// obligations/quota fills bound to river windows
	int			CanyonBridges;			// ...and to canyon/ravine windows
	int			Highways;
	int			Overpasses;
	int			ServiceBranches;
	int			Trails;
	int			CrossLinks;

	unsigned int	Checksum;			// Phase 9 gate value
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
};


/*
**	Phase 9 failures.  Each is a named attempt failure: the manager retries with a new seed.
*/
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
	HYBRID_TOPOLOGY_OVERPASS_QUOTA,				// the overpass quota cannot be met, naturally or synthesized
	HYBRID_TOPOLOGY_EDGE_BUDGET,					// more routed edges than R allows
	HYBRID_TOPOLOGY_NO_CYCLE,						// gate: a tree fails this phase
	HYBRID_TOPOLOGY_FAILURE_COUNT
};

const char *	Hybrid_Topology_Failure_Name(HybridTopologyFailure failure);

struct HybridTopologyResult
{
	HybridTopologyResult(void) : Failure(HYBRID_TOPOLOGY_OK) { }

	HybridTopologyFailure	Failure;
	StringClass					Detail;
};


/*
**	Section 5's per-size-class road quotas, so the builder, the gate and Phase 19 share one owner.
*/
int	Hybrid_River_Bridge_Quota(HybridSizeClass size_class);			// 1 / 2 / 3
int	Hybrid_Canyon_Bridge_Quota(HybridSizeClass size_class);		// 1 / 1 / 2
int	Hybrid_Highway_Quota(HybridSizeClass size_class);				// 0 / 1 / 2
int	Hybrid_Overpass_Quota(HybridSizeClass size_class);				// 0 / 1 / 2
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


#endif // HYBRIDTOPOLOGY_H
