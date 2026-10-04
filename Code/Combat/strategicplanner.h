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
 *                     $Archive:: /Commando/Code/Combat/strategicplanner.h                    $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	V2 roadmap Section 34 Phase 7 -- candidate bases, strategic anchors, POIs, and resource-site *
 *	planning.  Sections 15.1, 16.1, 16.2, 17.                                                   *
 *                                                                                             *
 *	Candidates, not a choice.  Phase 7 step 12 is explicit: "do not choose final bases yet."     *
 *	Every node this file produces is a scored possibility for a later phase to accept, reject or  *
 *	combine -- `WorldGenerationManifest::SelectedBaseCandidateIds` and its four siblings stay      *
 *	empty from here; they are named "Selected", and nothing here has selected anything.  What      *
 *	Phase 7 owes the phases after it is a graph worth choosing from, not a decision.                *
 *                                                                                             *
 *	Reuse over rediscovery.  Phase 5's interior cliff bypass candidates and Phase 6's hydrology    *
 *	crossing candidates already identified exactly the barrier points Section 9.5/17's road          *
 *	planner will need to resolve -- this file folds both into its own node graph as                  *
 *	CROSSING_CANDIDATE/TUNNEL_CANDIDATE nodes rather than re-deriving them from the terrain a           *
 *	second time.  `WaterSystem::Is_Point_In_Water` and the chunk registry's own `CliffBand`/              *
 *	`InteriorCliff`/`Buildable` flags are the exclusion tests Section 8.1's water/cliff exclusion         *
 *	step asks for, already computed by their own owners.                                                 *
 *                                                                                             *
 *	Faction reuses `PLAYERTYPE_GDI`/`PLAYERTYPE_NOD` (playertype.h) -- the canonical team              *
 *	identity every other Combat system already answers to -- rather than a parallel enum invented       *
 *	for this file alone.  A non-base node's `Faction` is `PLAYERTYPE_NEUTRAL`: nobody owns a               *
 *	Tiberium field, a settlement, a POI or a crossing candidate.                                          *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef STRATEGICPLANNER_H
#define STRATEGICPLANNER_H

#ifndef ALWAYS_H
#include "always.h"
#endif

#include "vector.h"
#include "vector3.h"
#include "wwstring.h"


//	Phase 7 step 9's high-level abstract node set, named exactly as that step lists it.
enum StrategicNodeType
{
	STRATEGIC_NODE_BASE_CANDIDATE = 0,
	STRATEGIC_NODE_RESOURCE_CANDIDATE,
	STRATEGIC_NODE_SETTLEMENT_CANDIDATE,
	STRATEGIC_NODE_POI_CANDIDATE,
	STRATEGIC_NODE_CROSSING_CANDIDATE,
	STRATEGIC_NODE_TUNNEL_CANDIDATE,
	STRATEGIC_NODE_ROUTE_POINT,		// migration adapter only: a hybrid road node (entrance, waypoint, junction), no role
	STRATEGIC_NODE_TYPE_COUNT
};


/*
**	One candidate, placed.  Id is Section 3B.5's stable identity -- "<type>_<ordinal>", assigned
**	in the same deterministic build order every time the same seed and terrain are planned, so a
**	later phase can name a candidate and have that name mean the same node again.
*/
class StrategicNode
{
public:
	StrategicNode(void);

	bool	operator == (const StrategicNode & that) const;
	bool	operator != (const StrategicNode & that) const	{ return !(*this == that); }

	StringClass				Id;
	StrategicNodeType		Type;
	int						Faction;			// a PLAYERTYPE value; PLAYERTYPE_NEUTRAL for every non-base node
	Vector3					Position;
	float						Suitability;	// 0..1, the score this node was accepted at
};


struct StrategicPlannerParams
{
	StrategicPlannerParams(void);

	//	Already derived for the bases domain by the caller.  Consumed narrowly: every ranking in
	//	this generator is otherwise a pure function of the terrain, so the seed's only effect is
	//	a small per-chunk score jitter that resolves ties between otherwise-equal candidates
	//	differently per seed, rather than the same regular terrain always ranking identically.
	unsigned int	Seed;

	//	Section 10A.7's own normal target, reused here as Phase 7 step 4's "only one unavoidable
	//	approach" rejection threshold rather than a second number that could silently disagree
	//	with it -- ProceduralWorldManager sources this from profile.Fairness.MinVehicleApproachCount.
	int	MinApproachCount;

	int	MinBaseCandidatesPerFaction;	// >= 1 -- the targeted validation's own floor
	float	MinBaseSeparation;				// world units -- keeps candidates from clustering on one hill
	int	MaxTiberiumCandidates;
	int	MaxSettlementCandidates;
	int	MaxPoiCandidates;
	int	MaxExpansionCandidates;

	//	Step 11's "obviously economically disconnected" made concrete: a resource candidate with
	//	no base candidate of either faction within this straight-line range is dropped.
	float	MaxEconomicRange;
};


class StrategicPlanner
{
public:

	static void			Init(void);
	static void			Shutdown(void);

	/*
	**	Requires WorldTerrainSystem::Has_Chunk_Registry() -- every candidate is scored off the
	**	chunk registry's own data.  Returns true with a short node list for a world too small or
	**	too flat to place much on, which is a correct, unremarkable outcome; the targeted-
	**	validation checks below are how a caller learns whether that outcome is still acceptable.
	*/
	static bool			Generate(const StrategicPlannerParams & params,StringClass * error_out);

	/*
	**	Migration adapter (hybrid rewrite I13, Phase 12): the native hybrid generator publishes its frozen, realized
	**	records here read-only, so the consumers that still read this planner keep working.  It creates, moves and
	**	owns no runtime object -- Phase 12 owns those -- and `checksum` is the hybrid record's own.  Removed once every
	**	consumer is migrated by its own item (checklist item 22).
	*/
	static bool			Adopt_Plan(const DynamicVectorClass<StrategicNode> & nodes,unsigned int checksum);

	static void			Clear(void);
	static bool			Has_Plan(void)				{ return Built; }

	static int			Get_Node_Count(void)		{ return Nodes.Count(); }
	static bool			Get_Node(int index,StrategicNode * out);
	static int			Get_Node_Count_By_Type(StrategicNodeType type);

	//	Linear id lookup -- the canonical way a caller holding a stable id (Section 3B.5) rather
	//	than an index gets back to a node.  Small and shared rather than reinvented per caller;
	//	BaseSelectionPlanner and TiberiumFieldGenerator both need exactly this.
	static bool			Find_Node_By_Id(const StringClass & id,int * index_out);

	//	Step 10: approximate straight-line travel cost between two nodes, world units, -1 for an
	//	out-of-range index.  "Approximate" and "straight" per the step's own wording -- a real
	//	topographic/routed cost is Phase 8's job, once a road graph exists to measure one on.
	static float			Get_Travel_Cost(int index_a,int index_b);

	//	A CRC over every node's own id, type, faction, position and suitability, quantized
	//	(Section 3B.4).  Zero with no plan built.
	static unsigned int	Get_Checksum(void)		{ return Checksum; }

	/*
	**	The targeted validation Phase 7's own text names, plus the two structural exclusions its
	**	do-not-proceed list implies.
	*/
	static int			Check_Faction_Base_Minimum(int min_per_faction,StringClass * report);
	static int			Check_Tiberium_Minimum(int min_count,StringClass * report);
	static int			Check_No_Cliff_Or_Water_Overlap(StringClass * report);
	static int			Check_No_Economically_Disconnected_Resource(float max_range,StringClass * report);

private:

	static bool			Built;
	static unsigned int	Checksum;

	static DynamicVectorClass<StrategicNode>	Nodes;
	static DynamicVectorClass<float>				TravelCost;		// Nodes.Count() x Nodes.Count(), row-major
};

#endif	// STRATEGICPLANNER_H
