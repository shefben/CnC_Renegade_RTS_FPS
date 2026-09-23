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
 *                     $Archive:: /Commando/Code/Combat/logisticsgraphservice.h               $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	V2 roadmap Section 34 Phase 21 -- physical logistics layer.  Section 22.                    *
 *                                                                                             *
 *	Section 22.1's own philosophy decides this file's shape: "keep core player purchases        *
 *	simple... logistics primarily affects Commander/AI strategic throughput."  Nothing here      *
 *	ever touches a human player's purchase path -- it is a read-only graph over state other      *
 *	real owners already produced (StrategicPlanner's candidates, BaseSelectionPlanner's chosen    *
 *	sites, TiberiumFieldGenerator's resolved fields, SettlementGenerator's resolved POIs,          *
 *	ExpansionRegionRegistry's forward positions, and live production/refinery/repair              *
 *	buildings), plus one real distance/reachability primitive borrowed rather than reinvented:     *
 *	RoadGraphGenerator's own Dijkstra path search over the roads RoadSystem actually built.        *
 *	Section 22.3's "avoid abstract global modifiers with no physical counterplay" is why a         *
 *	route's length and blockage are asked of real road/bridge data instead of a straight-line       *
 *	guess -- a broken bridge (BridgeSystem::Is_Traversable) is what makes a route BLOCKED, not       *
 *	an invented "supply" number nothing in the world represents.                                     *
 *                                                                                             *
 *	Seven canonical node kinds, four of them real today.  BASE, FORWARD_BASE, RESOURCE_FIELD      *
 *	and LOGISTICS_POI each have a real generator-time or Commander-time source in this tree.        *
 *	REFINERY, FACTORY and REPAIR are equally real mechanisms -- REFINERY asks a building's own       *
 *	descriptor for Phase 20's own SUBCOMPONENT_REFINERY_PROCESSOR role, FACTORY asks for a real       *
 *	production block, REPAIR asks BuildingGameObj::As_RepairBayGameObj() directly -- but every        *
 *	one of them is empty today for the same reason Phase 20's own four unwired effect groups           *
 *	were: no buildable Refinery/Factory/Repair Bay descriptor exists anywhere in                        *
 *	data/building_descriptors/*.json yet (WIP_features.md's own P33 entry).  Disclosed, not             *
 *	fabricated; see docs/procedural_world/LogisticsLayer.md.                                            *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef LOGISTICSGRAPHSERVICE_H
#define LOGISTICSGRAPHSERVICE_H

#ifndef ALWAYS_H
#include "always.h"
#endif

#include "playertype.h"
#include "vector.h"
#include "vector3.h"
#include "wwstring.h"


//	Section 22's own canonical node list, named exactly as that section lists it.
enum LogisticsNodeKindType
{
	LOGISTICS_NODE_BASE = 0,
	LOGISTICS_NODE_REFINERY,
	LOGISTICS_NODE_FACTORY,
	LOGISTICS_NODE_REPAIR,
	LOGISTICS_NODE_FORWARD_BASE,
	LOGISTICS_NODE_RESOURCE_FIELD,
	LOGISTICS_NODE_LOGISTICS_POI,
	LOGISTICS_NODE_KIND_COUNT
};

const char *	Logistics_Node_Kind_Name(LogisticsNodeKindType kind);


class LogisticsNode
{
public:
	LogisticsNode(void) : Kind(LOGISTICS_NODE_BASE),Team(PLAYERTYPE_NEUTRAL),
		Position(0.0f,0.0f,0.0f),BuildingId(-1),StrategicNodeIndex(-1),Operational(false) { }

	bool	operator == (const LogisticsNode &) const	{ return false; }
	bool	operator != (const LogisticsNode &) const	{ return true; }

	StringClass					Id;
	LogisticsNodeKindType	Kind;
	int							Team;					// a PLAYERTYPE value; PLAYERTYPE_NEUTRAL for a field/POI nobody owns
	Vector3						Position;
	int							BuildingId;				// -1 when this node names no live GameObj
	int							StrategicNodeIndex;	// into StrategicPlanner's own node list; -1 when none resolves
	bool							Operational;			// alive, constructed and (where power applies) powered
};


enum LogisticsRouteStatusType
{
	LOGISTICS_ROUTE_OPEN = 0,
	LOGISTICS_ROUTE_BLOCKED_BRIDGE,
	LOGISTICS_ROUTE_UNREACHABLE,		// no real road path connects the two nodes (or neither resolves a position on the plan at all)
	LOGISTICS_ROUTE_STATUS_COUNT
};

const char *	Logistics_Route_Status_Name(LogisticsRouteStatusType status);


class LogisticsRoute
{
public:
	LogisticsRoute(void) : FromNodeIndex(-1),ToNodeIndex(-1),Length(-1.0f),
		Status(LOGISTICS_ROUTE_UNREACHABLE),BlockingBridgeId(-1) { }

	bool	operator == (const LogisticsRoute &) const	{ return false; }
	bool	operator != (const LogisticsRoute &) const	{ return true; }

	StringClass						Id;
	int								FromNodeIndex;
	int								ToNodeIndex;
	float								Length;				// real road length from RoadGraphGenerator; -1 when unreachable
	LogisticsRouteStatusType	Status;
	int								BlockingBridgeId;	// -1 unless Status == LOGISTICS_ROUTE_BLOCKED_BRIDGE

	//	The route's own real polyline, sampled once at Rebuild_Routes time from the RoadGraphEdge
	//	chain RoadGraphGenerator::Find_Anchor_Path returned -- what a convoy order (step 6) walks
	//	and what Update_Route_Status (step 9) tests against BridgeSystem.  Empty for an
	//	UNREACHABLE route.
	DynamicVectorClass<Vector3>	Waypoints;
};


class LogisticsGraphService
{
public:

	static void			Init(void);
	static void			Shutdown(void);
	static void			Clear(void);

	/*
	**	Plan/Commander-time node kinds -- BASE, FORWARD_BASE, RESOURCE_FIELD, LOGISTICS_POI.
	**	Headless-safe: reads only BaseSelectionPlanner, ExpansionRegionRegistry,
	**	TiberiumFieldGenerator and SettlementGenerator, none of which need a GameObjManager
	**	scene, which is what makes this half of the graph self-check testable end to end.
	**	`convoy_enabled`/`construction_support_radius` are the profile's own Section 22.1
	**	toggles (ProceduralWorldProfile::Logistics), stored for Start_Convoy and
	**	Is_Under_Construction_Support to read back.
	*/
	static bool			Rebuild_Nodes(bool convoy_enabled,float construction_support_radius,
												 StringClass * error_out);

	/*
	**	REFINERY/FACTORY/REPAIR -- live BuildingGameObj instances, found by walking
	**	GameObjManager's own object list.  Appended to whatever Rebuild_Nodes already built;
	**	call Rebuild_Nodes first.  Never called by this file's own self-check, for the same
	**	standing reason every other runtime-building construction path in this tree is not:
	**	`-terrainselfcheck` never brings up a GameObjManager scene either.  Real at the console/
	**	live-match call site (combat.cpp), unexercised device-less -- see
	**	docs/procedural_world/LogisticsLayer.md.
	*/
	static void			Rebuild_Building_Nodes(void);

	/*
	**	Real road-network routes between every relevant pair of nodes this graph now holds --
	**	Tiberium harvester routes (BASE/FORWARD_BASE <-> RESOURCE_FIELD of the same team),
	**	forward-position routes (BASE <-> FORWARD_BASE of the same team), staging/repair routes
	**	(BASE/FORWARD_BASE <-> that team's own FACTORY/REFINERY/REPAIR) and POI routes (any
	**	BASE/FORWARD_BASE <-> every LOGISTICS_POI, which nobody owns).  Length and reachability
	**	come from RoadGraphGenerator::Find_Anchor_Path over the real road graph, never a
	**	straight-line guess.  A node with no StrategicPlanner origin of its own (a live building,
	**	an expansion region) is routed from the nearest StrategicPlanner node to its own
	**	position -- a documented proxy, not the node's own exact position.
	*/
	static bool			Rebuild_Routes(StringClass * error_out);

	/*
	**	Step 9.  Re-tests every existing route's own sampled waypoints against
	**	BridgeSystem::Is_Traversable's current, live state and updates Status/BlockingBridgeId in
	**	place.  Cheap and safe to call every server tick -- it changes no topology, only reads a
	**	bridge's own live damage state -- because bridge health changes far more often than a
	**	route's own road path does.  "enemy obstruction" (Section 22's third named blockage
	**	cause) is not attempted here: no logistics-facing threat model exists yet, that is
	**	Phase 25's own recon/defense planner territory.
	*/
	static void			Update_Route_Status(void);

	static bool			Has_Graph(void)			{ return Built; }
	static int			Get_Node_Count(void)	{ return Nodes.Count(); }
	static bool			Get_Node(int index,LogisticsNode * out);
	static bool			Find_Node_By_Id(const StringClass & id,int * index_out);

	static int			Get_Route_Count(void)	{ return Routes.Count(); }
	static bool			Get_Route(int index,LogisticsRoute * out);
	static bool			Find_Route_Between(int node_a,int node_b,int * index_out);

	//	Section 22.2 step 5.  True when position lies within the profile's own
	//	ConstructionSupportRadius of a live, operational FORWARD_BASE node belonging to team.
	//	Real and tested; no caller applies a bonus from it yet -- see the header comment above
	//	and docs/procedural_world/LogisticsLayer.md for why that is a disclosed gap, not a stub.
	static bool			Is_Under_Construction_Support(const Vector3 & position,int team);

	/*
	**	Step 6-8.  Server-side.  Drives an existing, already-produced AI vehicle along a real
	**	route's own waypoints, one at a time, via the same ScriptEngine::Action_Goto call every
	**	other Commander-driven order in this tree already uses -- CommanderGuardAI's own "a
	**	controller ticked once a server frame, translating intent into the same AI calls
	**	Dispatch_Order uses" shape, applied to a route instead of a guard point.  No cargo/
	**	inventory abstraction is invented (Section 22.4/step 7's own instruction): what the
	**	vehicle already is -- a Harvester, any AI-driven vehicle -- is what it carries.  Refused
	**	when the profile's own ConvoyEnabled is false, the route does not exist, is not OPEN, or
	**	has no waypoints.
	*/
	static bool			Start_Convoy(int unit_id,int route_index);
	static void			Stop_Convoy(int unit_id);
	static bool			Is_In_Convoy(int unit_id);
	static int			Get_Active_Convoy_Count(void);
	static void			Update(float seconds);

	static unsigned int	Get_Checksum(void)	{ return Checksum; }

private:

	class ConvoyRecordStruct
	{
	public:
		ConvoyRecordStruct(void) : UnitId(0),RouteIndex(-1),WaypointIndex(0) { }
		int	UnitId;
		int	RouteIndex;
		int	WaypointIndex;
	};

	static int			Find_Nearest_Strategic_Node(const Vector3 & position);
	static bool			Add_Route_If_New(int node_a,int node_b);
	static void			Recompute_Checksum(void);

	static bool			Built;
	static bool			ConvoyEnabled;
	static float		ConstructionSupportRadius;
	static unsigned int	Checksum;

	static DynamicVectorClass<LogisticsNode>			Nodes;
	static DynamicVectorClass<LogisticsRoute>			Routes;
	static DynamicVectorClass<ConvoyRecordStruct *>	Convoys;
};

#endif	// LOGISTICSGRAPHSERVICE_H
