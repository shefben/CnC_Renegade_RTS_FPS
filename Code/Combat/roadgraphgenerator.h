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
 *                     $Archive:: /Commando/Code/Combat/roadgraphgenerator.h                  $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	V2 roadmap Section 34 Phase 8 -- road and highway graph plus runtime road geometry.           *
 *	Section 10.                                                                                  *
 *                                                                                             *
 *	Connect, then realize through the one road system.  This file decides WHICH strategic          *
 *	anchors need a road between them and finds a real, terrain-costed path for each; the road         *
 *	itself -- centre line, ribbon, junctions, grading, mask -- is `RoadSystem`'s own, unmodified        *
 *	and unduplicated (`Code/wwphys/roadsystem.h`).  `RoadGraphGenerator` never builds geometry;          *
 *	it calls `RoadSystem::Add_Road`/`Build_Network` the same way the console command and self          *
 *	check already do.                                                                                    *
 *                                                                                             *
 *	Six conceptual classes, three physical ones.  Section 10.1 names six road classes; v1's           *
 *	`RoadClassType` (roadspline.h) has three, already complete and already tested, and extending           *
 *	it purely to grow a name list would touch `BridgeSystem`'s own default bridge-to-class               *
 *	mapping for no reason this phase needs. `RoadGraphClassType` below is Section 10.1's own six           *
 *	names, used for THIS file's own cost-field weighting and default width -- a question of how             *
 *	important a route is -- mapped down to `RoadClassType`'s three for the question RoadSystem                *
 *	actually asks, which physical rendering/grading tier a road belongs to.  Two different                     *
 *	questions, one real owner for the second one.                                                              *
 *                                                                                             *
 *	Real widths, not Humvee-derived ones.  Section 10A.1's lane-width formula needs the                          *
 *	authoritative GDI Humvee collision envelope, which means naming a specific vehicle definition                *
 *	-- exactly the kind of enumerable content identifier this project does not guess.  Default                    *
 *	widths below are fixed, documented constants standing in for that derivation until a real                      *
 *	definition name is confirmed; see docs/procedural_world/RoadGraph.md.                                          *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef ROADGRAPHGENERATOR_H
#define ROADGRAPHGENERATOR_H

#ifndef ALWAYS_H
#include "always.h"
#endif

#include "vector.h"
#include "wwstring.h"


//	Section 10.1, named exactly as that section lists them.
enum RoadGraphClassType
{
	ROAD_GRAPH_HIGHWAY = 0,
	ROAD_GRAPH_PRIMARY,
	ROAD_GRAPH_SECONDARY,
	ROAD_GRAPH_CITY_STREET,
	ROAD_GRAPH_SERVICE,
	ROAD_GRAPH_DIRT_TRACK,
	ROAD_GRAPH_CLASS_COUNT
};


class RoadGraphEdge
{
public:
	RoadGraphEdge(void);

	bool	operator == (const RoadGraphEdge & that) const;
	bool	operator != (const RoadGraphEdge & that) const	{ return !(*this == that); }

	StringClass				Id;			// stable id, Section 3B.5
	int						RoadId;		// the RoadSystem::Add_Road id this edge owns
	int						FromNodeIndex;	// into StrategicPlanner's own node list
	int						ToNodeIndex;
	RoadGraphClassType	Class;
	float						Length;

	//	Section 3D.2-flavoured: every StrategicNode id (a CROSSING_CANDIDATE or TUNNEL_CANDIDATE)
	//	this edge's route passed through, unresolved -- Phase 8 step 15's own list, per edge
	//	rather than one global bag, so a later phase knows which road needs which crossing.
	DynamicVectorClass<StringClass>	UnresolvedCrossingIds;
};


struct RoadGraphParams
{
	RoadGraphParams(void);

	//	Already derived for the roads domain by the caller.  Consumed the same narrow way
	//	StrategicPlanner's own seed is: a small routing-cost jitter that resolves ties between
	//	otherwise-equal paths differently per seed.
	unsigned int	Seed;

	float	MaxBuildableSlopeRadians;		// the same shared cap every terrain-reading generator uses
	float	SlopeCostWeight;					// >= 0 -- how strongly steep chunks are avoided
	float	CrossingCostPenalty;				// >= 0 -- extra cost to route through a water/cliff crossing chunk
	float	BoundaryApproachCostWeight;	// >= 0 -- how strongly the boundary-approach band is avoided
};


class RoadGraphGenerator
{
public:

	static void			Init(void);
	static void			Shutdown(void);

	/*
	**	Requires WorldTerrainSystem::Has_Chunk_Registry() and StrategicPlanner::Has_Plan() --
	**	edges connect strategic nodes, so there is nothing to route without both.  Returns true
	**	with zero edges when the plan has fewer than two BASE_CANDIDATE/RESOURCE_CANDIDATE nodes
	**	to connect, which is a correct, unremarkable outcome for a very small world.
	*/
	static bool			Generate(const RoadGraphParams & params,StringClass * error_out);

	//	Drops only this generator's own roads from RoadSystem, the same owned-id idiom every
	//	other procedural-world generator in this tree uses for a collection it does not own
	//	outright.
	static void			Clear(void);

	static bool			Has_Graph(void)			{ return Built; }
	static int			Get_Edge_Count(void)	{ return Edges.Count(); }
	static bool			Get_Edge(int index,RoadGraphEdge * out);

	static unsigned int	Get_Checksum(void)		{ return Checksum; }

	static int			Check_No_Cliff_Band_Crossing(StringClass * report);
	static int			Check_No_Unresolved_Water_Or_Cliff_Passage(StringClass * report);
	static int			Check_Required_Nodes_Connected(StringClass * report);

private:

	static bool			Built;
	static unsigned int	Checksum;

	static DynamicVectorClass<int>				OwnedRoadIds;
	static DynamicVectorClass<RoadGraphEdge>	Edges;
};

#endif	// ROADGRAPHGENERATOR_H
