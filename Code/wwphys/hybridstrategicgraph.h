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
 *                     $Archive:: /Commando/Code/wwphys/hybridstrategicgraph.h                $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Hybrid world generator rewrite, item I4 -- runtime Phase 3, the strategic gameplay graph      *
 * (Procedural_World_Generator_Rewrite_Plan_FINAL_AUDITED.md Section 8 Phase 3, Sections 6.1     *
 * and 6.10).  Gameplay roles exist before geography: this phase places both bases, all four     *
 * Section-5 Tiberium role classes, the front-center analysis node and every heavy route-family  *
 * waypoint, then records the route and resource intents that Phases 4, 8 and 9 consume.         *
 *                                                                                             *
 * Everything it reads is already frozen -- the Phase-2 `HybridMapIdentity`, the Section-6.10    *
 * `HybridArchetypeIntentSet` and the preflight metric cache -- so no archetype name is          *
 * reinterpreted here and no guide is re-derived.  The one live query is the canonical placeable *
 * boundary (`WorldBoundsService`), which is already configured for the attempt; asking it       *
 * rather than re-deriving a boundary keeps one owner for "can this disc exist here".            *
 *                                                                                             *
 * No terrain, water or road geometry is created or moved.  A role that cannot be placed         *
 * legally fails the attempt (Section 6.1); Phase 8 later revalidates these centers against      *
 * realized terrain and is forbidden to move them.                                              *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef HYBRIDSTRATEGICGRAPH_H
#define HYBRIDSTRATEGICGRAPH_H

#include "always.h"
#include "vector.h"
#include "vector2.h"
#include "wwstring.h"
#include "hybridplanrecords.h"
#include "hybridplanning.h"
#include "hybridmapidentity.h"


/*
**	The obstruction a crossing obligation is about.  Phase 3 reads it from the frozen intent set,
**	never from the archetype name, and Phase 9 matches its own obstruction windows against it.
*/
enum HybridObstructionKind
{
	HYBRID_OBSTRUCTION_NONE = 0,
	HYBRID_OBSTRUCTION_RIVER,
	HYBRID_OBSTRUCTION_LAKE,
	HYBRID_OBSTRUCTION_COAST,
	HYBRID_OBSTRUCTION_CANYON,
	HYBRID_OBSTRUCTION_RAVINE,
	HYBRID_OBSTRUCTION_RIDGE,
	HYBRID_OBSTRUCTION_CLIFF,
	HYBRID_OBSTRUCTION_KIND_COUNT
};

const char *	Hybrid_Obstruction_Kind_Name(HybridObstructionKind kind);


/*
**	The method an obligation requires.  `ANY` is the plan's own "method_or_ANY": the obligation
**	still has to be satisfied by some legal physical crossing, but Phase 9's method order chooses
**	which one.
*/
enum HybridCrossingMethod
{
	HYBRID_CROSSING_ANY = 0,
	HYBRID_CROSSING_BRIDGE,
	HYBRID_CROSSING_PASS,
	HYBRID_CROSSING_FORD_LIGHT,
	HYBRID_CROSSING_FORD_HARVESTER,
	HYBRID_CROSSING_METHOD_COUNT
};

const char *	Hybrid_Crossing_Method_Name(HybridCrossingMethod method);


/*
**	One `CROSSING_OBLIGATION_INTENT`: the plan's exact tuple (family id, obstruction kind,
**	strategic guide position, required method or ANY), plus the intent it came from so Phase 9 can
**	bind the physical window back to the archetype record.  The guide is a strategic waypoint, not
**	a bridge location; its purpose is to make the later window selector prefer the right part of
**	the obstruction.
*/
struct HybridCrossingObligation
{
	HybridCrossingObligation(void) :
		StableId(0),RouteFamily(-1),Obstruction(HYBRID_OBSTRUCTION_NONE),
		RequiredMethod(HYBRID_CROSSING_ANY),DistinctCrossingRequired(false),
		Guide(0.0f,0.0f),IntentStableId(0),WaypointStableId(0) { }

	//	DynamicVectorClass<T> (wwlib/vector.h) requires T to support == and !=.  StableId is the key.
	bool	operator == (const HybridCrossingObligation & that) const	{ return StableId == that.StableId; }
	bool	operator != (const HybridCrossingObligation & that) const	{ return !(*this == that); }

	unsigned int			StableId;						// creation order, 1-based
	int						RouteFamily;
	HybridObstructionKind	Obstruction;
	HybridCrossingMethod	RequiredMethod;
	bool						DistinctCrossingRequired;	// DISTINCT_CROSSING_REQUIRED: not the same physical object
	Vector2					Guide;							// strategic (u,v), as the Section-6.10 table gives it
	unsigned int			IntentStableId;				// HybridArchetypeIntent::StableId it came from
	unsigned int			WaypointStableId;				// the family waypoint placed for that guide
};


/*
**	Runtime Phase 3's output.  Sites and edges use the Section-2.1 records from item I1; StableIds
**	are creation order, 1-based, within their own record kind.
*/
struct HybridStrategicGraph
{
	HybridStrategicGraph(void) : GdiBaseId(0),NodBaseId(0),FrontCenterId(0),Checksum(0) { }

	void	Reset(void)
	{
		Sites.Delete_All ();
		Edges.Delete_All ();
		Obligations.Delete_All ();
		GdiBaseId		= 0;
		NodBaseId		= 0;
		FrontCenterId	= 0;
		Checksum			= 0;
	}

	const HybridStrategicSite *	Find_Site(unsigned int stable_id) const;
	int									Count_Of_Role(HybridSiteRole role) const;
	int									Count_Of_Intent(HybridEdgeIntent intent) const;

	//	Section 8 Phase 3 gate: the distinct heavy families must leave a cycle, never a tree.
	bool									Has_Cycle(void) const;

	DynamicVectorClass<HybridStrategicSite>			Sites;
	DynamicVectorClass<HybridStrategicEdge>			Edges;
	DynamicVectorClass<HybridCrossingObligation>	Obligations;

	unsigned int	GdiBaseId;
	unsigned int	NodBaseId;
	unsigned int	FrontCenterId;
	unsigned int	Checksum;			// Phase 3 gate value
};


/*
**	Phase 3's inputs beyond the frozen identity/intent records.  Plain values only: the profile
**	fields the plan names and the constants preflight already computed, so the builder is
**	reproducible from (params, identity, intents) plus the configured world boundary.
*/
struct HybridStrategicGraphParams
{
	HybridStrategicGraphParams(void);

	unsigned int		MapPlanSeed;			// SEED_DOMAIN_MAP_PLAN, already derived
	HybridConstants	Constants;

	float					BoundaryMargin;		// plan.boundary_margin_cells * world.chunk_size
	float					FieldRadiusMax;		// resource.field_radius_max -- every Tiberium role's reserve
	float					RiverWidth;				// water.river_width -- the guided river's influence band
};


/*
**	Phase 3 failures.  Every one is a named attempt failure: Section 6.1's bounded repair is
**	exhausted, or the phase gate itself does not hold.  The manager retries with a new seed.
*/
enum HybridStrategicGraphFailure
{
	HYBRID_GRAPH_OK = 0,
	HYBRID_GRAPH_NO_LEGAL_BASE_POSITION,			// base guide plus 0.10D repair has no hard-valid candidate
	HYBRID_GRAPH_NO_LEGAL_RESOURCE_POSITION,
	HYBRID_GRAPH_NO_LEGAL_WAYPOINT_POSITION,
	HYBRID_GRAPH_MISSING_GUIDED_RIVER_INTENT,		// RIVER_ADJACENT_CONTESTED without a frozen spline
	HYBRID_GRAPH_NO_RIVER_ADJACENT_FIELD,			// Section 6.10 station walk found no legal bank candidate
	HYBRID_GRAPH_ROLE_COUNT_MISMATCH,				// gate: Section 5 role counts
	HYBRID_GRAPH_INCOMPLETE_ROUTE_FAMILY,			// gate: a family is not one complete GDI-to-Nod intent
	HYBRID_GRAPH_MISSING_RESOURCE_INTENT,			// gate: a field has no attachment / no service branch
	HYBRID_GRAPH_NO_GRAPH_CYCLE,						// gate: a tree fails this phase
	HYBRID_GRAPH_FAILURE_COUNT
};

const char *	Hybrid_Strategic_Graph_Failure_Name(HybridStrategicGraphFailure failure);


/*
**	The Phase-3 hard-validity rule classes.  Exported because Phase 4 repairs a lower-priority
**	reservation with the same rules Phase 3 placed it under -- one owner for "may this role stand
**	here", never a second copy of the rule list.
*/
enum HybridRoleClass
{
	HYBRID_ROLE_BASE = 0,
	HYBRID_ROLE_RESOURCE,
	HYBRID_ROLE_WAYPOINT
};


/*
**	Is `center` a hard-valid position for a role of this class?  `exempt_placeable` is the archetype
**	guide exemption that lets a mandatory crossing guide sit on the obstruction it must cross;
**	`ignore_site_id` is the site being moved, which must not be tested against its own old position
**	(0 ignores nothing).
*/
bool	Hybrid_Role_Is_Hard_Valid(const HybridStrategicGraphParams & params,const HybridMapIdentity & identity,
											const HybridArchetypeIntentSet & intents,const HybridStrategicGraph & graph,
											HybridRoleClass role_class,const Vector2 & center,bool exempt_placeable,
											unsigned int ignore_site_id);

struct HybridStrategicGraphResult
{
	HybridStrategicGraphResult(void) : Failure(HYBRID_GRAPH_OK) { }

	HybridStrategicGraphFailure	Failure;
	StringClass							Detail;
};


/*
**	Section 5's per-size-class role counts Phase 3 owns.
*/
int	Hybrid_Contested_Field_Count(HybridSizeClass size_class);		// 1 / 3 / 5
int	Hybrid_Remote_Field_Count(HybridSizeClass size_class);			// 1 / 1 / 2


/*
**	Runtime Phase 3.  Creates every required role in the plan's exact order, applies the
**	Section-6.1 bounded legality repair where a guide is invalid, builds `route_family_count`
**	complete GDI-to-Nod families with their crossing obligations, attaches the resource intents and
**	checks the phase gate.  Returns false with result->Failure set on the first failure.
*/
bool	Hybrid_Build_Strategic_Graph(const HybridStrategicGraphParams & params,const HybridMapIdentity & identity,
												const HybridArchetypeIntentSet & intents,HybridStrategicGraph * graph,
												HybridStrategicGraphResult * result);

/*
**	Section 0.10's `PlanningDistance(a,b)`: the shortest path between two sites on the currently frozen
**	strategic graph, using quantized straight-line edge lengths, which is the only distance that
**	exists before Phase 10's A* routing.  Negative means the two sites are not connected on the graph.
**
**	It lives with the graph because the graph owns distances over itself: Phase 8's provisional
**	fairness and Phase 19's own re-check both call this rather than each walking the edge list.
*/
float	Hybrid_Planning_Distance(const HybridStrategicGraph & graph,unsigned int site_a,unsigned int site_b);


/*
**	The Phase 3 gate value: a quantized digest of every site, edge and obligation in stable record
**	order.  Hybrid_Build_Strategic_Graph stores it in graph->Checksum.
*/
unsigned int	Hybrid_Strategic_Graph_Checksum(const HybridStrategicGraph & graph,unsigned int seed = 0);


#endif // HYBRIDSTRATEGICGRAPH_H
