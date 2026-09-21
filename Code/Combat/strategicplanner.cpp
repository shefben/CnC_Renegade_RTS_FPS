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
 *                     $Archive:: /Commando/Code/Combat/strategicplanner.cpp                  $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "strategicplanner.h"

#include "crc.h"
#include "hydrologygenerator.h"
#include "interiorcliffgenerator.h"
#include "playertype.h"
#include "terrainmask.h"
#include "watersystem.h"
#include "worldboundsservice.h"
#include "worldterrainsystem.h"
#include "wwdebug.h"
#include "wwmath.h"


bool				StrategicPlanner::Built			= false;
unsigned int	StrategicPlanner::Checksum		= 0;

DynamicVectorClass<StrategicNode>	StrategicPlanner::Nodes;
DynamicVectorClass<float>				StrategicPlanner::TravelCost;


//	How many base candidates a faction is offered when the world can spare them -- more than
//	the targeted validation's own floor of 2, so "multiple" (step 2/3's own word) means something.
static const int TARGET_BASE_CANDIDATES_PER_FACTION = 4;

//	The reference distance Get_Inward_Distance is normalized against for the suitability score's
//	boundary-distance term.  Not a hard limit -- a point further than this still scores the
//	term's full weight, it simply cannot score higher for being further still.
static const float BOUNDARY_DISTANCE_REFERENCE = 200.0f;

//	How far a resource candidate's own suitability bonus reaches before it stops mattering to a
//	base score.  Independent of MaxEconomicRange (the step-11 survival gate): this shapes which
//	otherwise-similar chunk a base prefers, that decides whether the pairing survives at all.
static const float RESOURCE_PROXIMITY_RANGE = 400.0f;

//	Section 3B.4: never hash a raw float.
static const float CHECKSUM_QUANTUM = 1000.0f;

//	How much a chunk's own deterministic seeded hash can move its terrain score, either way.
//	Small enough that a genuinely better chunk always outranks a genuinely worse one; large
//	enough that two equally-scored chunks (a common case on regular terrain) do not always
//	resolve their tie the same way regardless of seed, which is this generator's only use of
//	its own seed -- see this file's header comment.
static const float TERRAIN_SCORE_JITTER_WEIGHT = 0.05f;

static const int NEIGHBOR_DX[4] = {  1, 0, -1, 0 };
static const int NEIGHBOR_DY[4] = {  0, 1,  0,-1 };


StrategicPlannerParams::StrategicPlannerParams(void) :
	Seed (1),
	MinApproachCount (2),
	MinBaseCandidatesPerFaction (2),
	MinBaseSeparation (150.0f),
	MaxTiberiumCandidates (4),
	MaxSettlementCandidates (3),
	MaxPoiCandidates (3),
	MaxExpansionCandidates (2),
	MaxEconomicRange (500.0f)
{
}


StrategicNode::StrategicNode(void) :
	Type (STRATEGIC_NODE_BASE_CANDIDATE),
	Faction (PLAYERTYPE_NEUTRAL),
	Position (0.0f,0.0f,0.0f),
	Suitability (0.0f)
{
}


bool StrategicNode::operator == (const StrategicNode & that) const
{
	return (Id == that.Id) && (Type == that.Type) && (Faction == that.Faction) &&
			 (Position == that.Position) && (Suitability == that.Suitability);
}


void StrategicPlanner::Init(void)
{
	Clear();
}


void StrategicPlanner::Shutdown(void)
{
	Clear();
}


void StrategicPlanner::Clear(void)
{
	Nodes.Delete_All();
	TravelCost.Delete_All();
	Built		= false;
	Checksum	= 0;
}


bool StrategicPlanner::Get_Node(int index,StrategicNode * out)
{
	if ((index < 0) || (index >= Nodes.Count()) || (out == nullptr)) {
		return false;
	}
	*out = Nodes[index];
	return true;
}


int StrategicPlanner::Get_Node_Count_By_Type(StrategicNodeType type)
{
	int count = 0;
	for (int i = 0; i < Nodes.Count(); i++) {
		if (Nodes[i].Type == type) count++;
	}
	return count;
}


float StrategicPlanner::Get_Travel_Cost(int index_a,int index_b)
{
	int n = Nodes.Count();
	if ((index_a < 0) || (index_a >= n) || (index_b < 0) || (index_b >= n)) {
		return -1.0f;
	}
	return TravelCost[(index_a * n) + index_b];
}


/*
**	Per-chunk terrain suitability.  Eligible gates out cliff band, interior cliffs (both already
**	folded into Buildable, read once rather than re-derived) and water (Section 8.1's water/cliff
**	exclusion, answered by WaterSystem's own headless query rather than a second test of the
**	same fact). ApproachCount is Phase 7 step 1's "reachable area estimate/expansion room/
**	roadability" collapsed into one cheap proxy: how many of a chunk's own four neighbours are
**	themselves buildable, which is also exactly Phase 7 step 4's own approach-count rejection
**	test, so eligibility and approach counting are one walk, not two.
*/
struct ChunkScore
{
	bool		Eligible;
	int		ApproachCount;
	float		Terrain;			// 0..1
	Vector3	Position;

	//	DynamicVectorClass<T> (wwlib/vector.h) requires T to support == and !=.
	bool	operator == (const ChunkScore & that) const
	{
		return (Eligible == that.Eligible) && (ApproachCount == that.ApproachCount) &&
				 (Terrain == that.Terrain) && (Position == that.Position);
	}
	bool	operator != (const ChunkScore & that) const	{ return !(*this == that); }
};


static void Compute_Chunk_Scores(int count_x,int count_y,unsigned int seed,DynamicVectorClass<ChunkScore> * out)
{
	out->Delete_All();
	for (int i = 0; i < (count_x * count_y); i++) {
		ChunkScore score;
		score.Eligible			= false;
		score.ApproachCount	= 0;
		score.Terrain			= 0.0f;
		out->Add(score);
	}

	for (int py = 0; py < count_y; py++) {
		for (int px = 0; px < count_x; px++) {

			int index = (py * count_x) + px;
			ChunkScore & score = (*out)[index];

			TerrainChunkInfoClass info;
			if (!WorldTerrainSystem::Get_Chunk_Info(px,py,&info)) continue;

			AABoxClass bounds;
			if (!WorldTerrainSystem::Get_Patch_Bounds(px,py,&bounds)) continue;
			score.Position = bounds.Center;

			bool in_water = WaterSystem::Is_Point_In_Water(bounds.Center.X,bounds.Center.Y);
			score.Eligible = info.Buildable && !in_water;
		}
	}

	for (int py = 0; py < count_y; py++) {
		for (int px = 0; px < count_x; px++) {

			int index = (py * count_x) + px;
			ChunkScore & score = (*out)[index];
			if (!score.Eligible) continue;

			int approaches = 0;
			for (int d = 0; d < 4; d++) {
				int nx = px + NEIGHBOR_DX[d];
				int ny = py + NEIGHBOR_DY[d];
				if ((nx < 0) || (nx >= count_x) || (ny < 0) || (ny >= count_y)) continue;
				if ((*out)[(ny * count_x) + nx].Eligible) approaches++;
			}
			score.ApproachCount = approaches;

			float boundary_distance = WorldBoundsService::Get_Inward_Distance(
													Vector2(score.Position.X,score.Position.Y));
			float boundary_term = boundary_distance / BOUNDARY_DISTANCE_REFERENCE;
			if (boundary_term < 0.0f) boundary_term = 0.0f;
			if (boundary_term > 1.0f) boundary_term = 1.0f;

			float approach_term = (float)approaches / 4.0f;

			float jitter = (TerrainMaskClass::Hash_Value(px,py,(int)seed) - 0.5f) * TERRAIN_SCORE_JITTER_WEIGHT;

			score.Terrain = (boundary_term * 0.5f) + (approach_term * 0.5f) + jitter;
		}
	}
}


//	Sorts chunk indices by descending Terrain, ascending index as the deterministic tie-break.
//	Insertion sort -- fine at chunk-grid scale, the same tolerance every other bounded pass in
//	this codebase already accepts.
static void Sort_By_Terrain_Descending(DynamicVectorClass<int> & indices,const DynamicVectorClass<ChunkScore> & scores)
{
	for (int a = 1; a < indices.Count(); a++) {
		int key = indices[a];
		int b = a - 1;
		while ((b >= 0) &&
				 ((scores[indices[b]].Terrain < scores[key].Terrain) ||
				  ((scores[indices[b]].Terrain == scores[key].Terrain) && (indices[b] > key)))) {
			indices[b + 1] = indices[b];
			b--;
		}
		indices[b + 1] = key;
	}
}


static bool Far_Enough(const Vector3 & position,const DynamicVectorClass<StrategicNode> & existing,float min_separation)
{
	for (int i = 0; i < existing.Count(); i++) {
		if ((position - existing[i].Position).Length() < min_separation) {
			return false;
		}
	}
	return true;
}


/***********************************************************************************************
 * Generate -- Phase 7 steps 1-11                                                                *
 *=============================================================================================*/
bool StrategicPlanner::Generate(const StrategicPlannerParams & params,StringClass * error_out)
{
	#define FAIL(msg) { if (error_out) *error_out = msg; Clear(); return false; }

	Clear();

	if (!WorldTerrainSystem::Has_Chunk_Registry())	FAIL("no terrain chunk registry -- generate macro terrain first");
	if (params.MinApproachCount < 1)						FAIL("min approach count must be at least 1");
	if (params.MinBaseCandidatesPerFaction < 1)			FAIL("min base candidates per faction must be at least 1");
	if (params.MinBaseSeparation <= 0.0f)					FAIL("min base separation must be positive");
	if (params.MaxEconomicRange <= 0.0f)					FAIL("max economic range must be positive");

	int count_x = WorldTerrainSystem::Get_Chunk_Count_X();
	int count_y = WorldTerrainSystem::Get_Chunk_Count_Y();
	if ((count_x <= 0) || (count_y <= 0))					FAIL("chunk registry is empty");

	DynamicVectorClass<ChunkScore> scores;
	Compute_Chunk_Scores(count_x,count_y,params.Seed,&scores);

	DynamicVectorClass<int> ranked;
	for (int i = 0; i < scores.Count(); i++) {
		if (scores[i].Eligible) ranked.Add(i);
	}
	Sort_By_Terrain_Descending(ranked,scores);

	DynamicVectorClass<bool> claimed;
	for (int i = 0; i < scores.Count(); i++) claimed.Add(false);

	//
	//	Step 5 (candidate positions only -- survival against step 11's economic-range gate is
	//	decided after bases exist below).
	//
	DynamicVectorClass<int> tiberium_candidate_chunks;
	{
		DynamicVectorClass<StrategicNode> placed;		// separation bookkeeping only, not final nodes
		for (int r = 0; (r < ranked.Count()) && (placed.Count() < params.MaxTiberiumCandidates); r++) {
			int idx = ranked[r];
			if (claimed[idx]) continue;
			if (!Far_Enough(scores[idx].Position,placed,params.MinBaseSeparation)) continue;

			StrategicNode marker;
			marker.Position = scores[idx].Position;
			placed.Add(marker);
			tiberium_candidate_chunks.Add(idx);
			claimed[idx] = true;
		}
	}

	//
	//	Steps 2-4: base candidates, alternating GDI/Nod, rejecting anything below the approach-
	//	count floor, scored by terrain suitability plus a bonus for standing near an as-yet-
	//	unconfirmed Tiberium candidate.
	//
	int gdi_count = 0;
	int nod_count = 0;
	{
		DynamicVectorClass<int> base_ranked;
		for (int i = 0; i < scores.Count(); i++) {
			if (!scores[i].Eligible || claimed[i]) continue;
			if (scores[i].ApproachCount < params.MinApproachCount) continue;		// step 4
			base_ranked.Add(i);
		}

		DynamicVectorClass<float> base_score;
		for (int i = 0; i < base_ranked.Count(); i++) {
			int idx = base_ranked[i];
			float nearest = -1.0f;
			for (int t = 0; t < tiberium_candidate_chunks.Count(); t++) {
				float d = (scores[idx].Position - scores[tiberium_candidate_chunks[t]].Position).Length();
				if ((nearest < 0.0f) || (d < nearest)) nearest = d;
			}
			float bonus = 0.0f;
			if (nearest >= 0.0f) {
				bonus = 1.0f - (nearest / RESOURCE_PROXIMITY_RANGE);
				if (bonus < 0.0f) bonus = 0.0f;
			}
			base_score.Add(scores[idx].Terrain + (bonus * 0.5f));
		}

		//	Sort base_ranked by base_score, same insertion-sort idiom as the terrain-only rank.
		for (int a = 1; a < base_ranked.Count(); a++) {
			int key_idx = base_ranked[a];
			float key_score = base_score[a];
			int b = a - 1;
			while ((b >= 0) &&
					 ((base_score[b] < key_score) || ((base_score[b] == key_score) && (base_ranked[b] > key_idx)))) {
				base_ranked[b + 1] = base_ranked[b];
				base_score[b + 1]	= base_score[b];
				b--;
			}
			base_ranked[b + 1] = key_idx;
			base_score[b + 1]	= key_score;
		}

		bool next_is_gdi = true;
		for (int r = 0; r < base_ranked.Count(); r++) {
			if ((gdi_count >= TARGET_BASE_CANDIDATES_PER_FACTION) && (nod_count >= TARGET_BASE_CANDIDATES_PER_FACTION)) break;

			int idx = base_ranked[r];
			if (claimed[idx]) continue;
			if (!Far_Enough(scores[idx].Position,Nodes,params.MinBaseSeparation)) continue;

			//	Alternate normally; a faction that has already reached its own target yields the
			//	slot to the other one rather than wasting a candidate neither faction needed.
			bool want_gdi;
			if (gdi_count >= TARGET_BASE_CANDIDATES_PER_FACTION)		want_gdi = false;
			else if (nod_count >= TARGET_BASE_CANDIDATES_PER_FACTION)	want_gdi = true;
			else																			want_gdi = next_is_gdi;

			StrategicNode node;
			StringClass id;
			if (want_gdi) {
				id.Format("base_gdi_%d",gdi_count);
				node.Faction = PLAYERTYPE_GDI;
				gdi_count++;
			} else {
				id.Format("base_nod_%d",nod_count);
				node.Faction = PLAYERTYPE_NOD;
				nod_count++;
			}
			node.Id				= id;
			node.Type			= STRATEGIC_NODE_BASE_CANDIDATE;
			node.Position		= scores[idx].Position;
			node.Suitability	= scores[idx].Terrain;
			Nodes.Add(node);

			claimed[idx] = true;
			next_is_gdi = !next_is_gdi;
		}
	}

	//
	//	Step 5, finished: a Tiberium candidate survives only if a base of either faction landed
	//	within economic range of it -- step 11, made concrete.
	//
	int resource_index = 0;
	for (int t = 0; t < tiberium_candidate_chunks.Count(); t++) {
		int idx = tiberium_candidate_chunks[t];

		float nearest_base = -1.0f;
		for (int n = 0; n < Nodes.Count(); n++) {
			if (Nodes[n].Type != STRATEGIC_NODE_BASE_CANDIDATE) continue;
			float d = (scores[idx].Position - Nodes[n].Position).Length();
			if ((nearest_base < 0.0f) || (d < nearest_base)) nearest_base = d;
		}
		if ((nearest_base < 0.0f) || (nearest_base > params.MaxEconomicRange)) continue;

		StrategicNode node;
		StringClass id; id.Format("resource_%d",resource_index);
		node.Id				= id;
		node.Type			= STRATEGIC_NODE_RESOURCE_CANDIDATE;
		node.Faction		= PLAYERTYPE_NEUTRAL;
		node.Position		= scores[idx].Position;
		node.Suitability	= scores[idx].Terrain;
		Nodes.Add(node);
		resource_index++;
	}

	//
	//	Step 6: settlements -- ordinary terrain suitability, away from what is already claimed.
	//
	{
		int settlement_index = 0;
		for (int r = 0; (r < ranked.Count()) && (settlement_index < params.MaxSettlementCandidates); r++) {
			int idx = ranked[r];
			if (claimed[idx]) continue;
			if (!Far_Enough(scores[idx].Position,Nodes,params.MinBaseSeparation)) continue;

			StrategicNode node;
			StringClass id; id.Format("settlement_%d",settlement_index);
			node.Id				= id;
			node.Type			= STRATEGIC_NODE_SETTLEMENT_CANDIDATE;
			node.Faction		= PLAYERTYPE_NEUTRAL;
			node.Position		= scores[idx].Position;
			node.Suitability	= scores[idx].Terrain;
			Nodes.Add(node);

			claimed[idx] = true;
			settlement_index++;
		}
	}

	//
	//	Step 7 and step 8, both filed as POI_CANDIDATE -- step 9's own closed node vocabulary has
	//	no separate expansion-area type, so a forward outpost is a POI whose own selection favours
	//	distance from every base rather than raw terrain score.  Ordinary POIs first, then
	//	expansion outposts, so a small world spends its limited candidates on the ordinary ones.
	//
	{
		int poi_index = 0;
		for (int r = 0; (r < ranked.Count()) && (poi_index < params.MaxPoiCandidates); r++) {
			int idx = ranked[r];
			if (claimed[idx]) continue;
			if (!Far_Enough(scores[idx].Position,Nodes,params.MinBaseSeparation)) continue;

			StrategicNode node;
			StringClass id; id.Format("poi_%d",poi_index);
			node.Id				= id;
			node.Type			= STRATEGIC_NODE_POI_CANDIDATE;
			node.Faction		= PLAYERTYPE_NEUTRAL;
			node.Position		= scores[idx].Position;
			node.Suitability	= scores[idx].Terrain;
			Nodes.Add(node);

			claimed[idx] = true;
			poi_index++;
		}

		DynamicVectorClass<int> frontier_ranked;
		for (int i = 0; i < scores.Count(); i++) {
			if (!scores[i].Eligible || claimed[i]) continue;
			frontier_ranked.Add(i);
		}
		DynamicVectorClass<float> frontier_score;
		for (int i = 0; i < frontier_ranked.Count(); i++) {
			int idx = frontier_ranked[i];
			float nearest_base = -1.0f;
			for (int n = 0; n < Nodes.Count(); n++) {
				if (Nodes[n].Type != STRATEGIC_NODE_BASE_CANDIDATE) continue;
				float d = (scores[idx].Position - Nodes[n].Position).Length();
				if ((nearest_base < 0.0f) || (d < nearest_base)) nearest_base = d;
			}
			frontier_score.Add((nearest_base < 0.0f) ? 0.0f : nearest_base);
		}
		for (int a = 1; a < frontier_ranked.Count(); a++) {
			int key_idx = frontier_ranked[a];
			float key_score = frontier_score[a];
			int b = a - 1;
			while ((b >= 0) &&
					 ((frontier_score[b] < key_score) || ((frontier_score[b] == key_score) && (frontier_ranked[b] > key_idx)))) {
				frontier_ranked[b + 1] = frontier_ranked[b];
				frontier_score[b + 1]	= frontier_score[b];
				b--;
			}
			frontier_ranked[b + 1] = key_idx;
			frontier_score[b + 1]	= key_score;
		}

		int expansion_index = 0;
		for (int r = 0; (r < frontier_ranked.Count()) && (expansion_index < params.MaxExpansionCandidates); r++) {
			int idx = frontier_ranked[r];
			if (claimed[idx]) continue;
			if (!Far_Enough(scores[idx].Position,Nodes,params.MinBaseSeparation)) continue;

			StrategicNode node;
			StringClass id; id.Format("expansion_%d",expansion_index);
			node.Id				= id;
			node.Type			= STRATEGIC_NODE_POI_CANDIDATE;
			node.Faction		= PLAYERTYPE_NEUTRAL;
			node.Position		= scores[idx].Position;
			node.Suitability	= scores[idx].Terrain;
			Nodes.Add(node);

			claimed[idx] = true;
			expansion_index++;
		}
	}

	//
	//	Crossing/tunnel candidates: reused from Phase 5/6's own already-computed bypass and
	//	crossing points, never re-derived from the terrain a second time.  Optional inputs --
	//	either generator may simply not have run.
	//
	if (InteriorCliffGenerator::Has_Cliffs()) {
		int crossing_index = 0;
		for (int i = 0; i < InteriorCliffGenerator::Get_Feature_Count(); i++) {
			InteriorCliffFeature f;
			if (!InteriorCliffGenerator::Get_Feature(i,&f) || !f.HasBypassPoint) continue;

			StrategicNode node;
			StringClass id;
			if (f.Traversability == INTERIOR_CLIFF_TUNNEL_BYPASS) {
				id.Format("tunnel_%d",crossing_index);
				node.Type = STRATEGIC_NODE_TUNNEL_CANDIDATE;
			} else if (f.Traversability == INTERIOR_CLIFF_BRIDGE_CROSSING) {
				id.Format("crossing_cliff_%d",crossing_index);
				node.Type = STRATEGIC_NODE_CROSSING_CANDIDATE;
			} else {
				continue;		// a ramp candidate is not infrastructure to plan; the cliff is just not fully blocking there
			}
			node.Id			= id;
			node.Faction	= PLAYERTYPE_NEUTRAL;
			node.Position	= f.BypassPoint;
			node.Suitability = 0.0f;
			Nodes.Add(node);
			crossing_index++;
		}
	}

	if (HydrologyGenerator::Has_Hydrology()) {
		int crossing_index = 0;
		for (int i = 0; i < HydrologyGenerator::Get_Feature_Count(); i++) {
			HydrologyFeature f;
			if (!HydrologyGenerator::Get_Feature(i,&f)) continue;

			for (int c = 0; c < f.Crossings.Count(); c++) {
				if (f.Crossings[c].Kind == HYDRO_CROSSING_ROUTE_AROUND) continue;		// a routing hint, not infrastructure

				StrategicNode node;
				StringClass id; id.Format("crossing_water_%d",crossing_index);
				node.Id				= id;
				node.Type			= STRATEGIC_NODE_CROSSING_CANDIDATE;
				node.Faction		= PLAYERTYPE_NEUTRAL;
				node.Position		= f.Crossings[c].Position;
				node.Suitability	= 0.0f;
				Nodes.Add(node);
				crossing_index++;
			}
		}
	}

	//
	//	Step 10: approximate straight-line travel cost between every pair of nodes.
	//
	{
		int n = Nodes.Count();
		for (int i = 0; i < (n * n); i++) TravelCost.Add(0.0f);
		for (int a = 0; a < n; a++) {
			for (int b = 0; b < n; b++) {
				TravelCost[(a * n) + b] = (Nodes[a].Position - Nodes[b].Position).Length();
			}
		}
	}

	{
		CRCEngine crc;
		for (int i = 0; i < Nodes.Count(); i++) {
			const StrategicNode & node = Nodes[i];
			int q_type			= (int)node.Type;
			int q_faction		= node.Faction;
			int q_x				= (int)(node.Position.X * CHECKSUM_QUANTUM);
			int q_y				= (int)(node.Position.Y * CHECKSUM_QUANTUM);
			int q_suitability	= (int)(node.Suitability * CHECKSUM_QUANTUM);
			crc(&q_type,sizeof(q_type));
			crc(&q_faction,sizeof(q_faction));
			crc(&q_x,sizeof(q_x));
			crc(&q_y,sizeof(q_y));
			crc(&q_suitability,sizeof(q_suitability));
		}
		Checksum = (unsigned int)crc;
	}

	Built = true;

	WWDEBUG_SAY(("StrategicPlanner: %d node(s) (%d GDI base, %d Nod base), checksum 0x%08X\r\n",
					 Nodes.Count(),gdi_count,nod_count,Checksum));

	#undef FAIL
	return true;
}


int StrategicPlanner::Check_Faction_Base_Minimum(int min_per_faction,StringClass * report)
{
	int gdi = 0, nod = 0;
	for (int i = 0; i < Nodes.Count(); i++) {
		if (Nodes[i].Type != STRATEGIC_NODE_BASE_CANDIDATE) continue;
		if (Nodes[i].Faction == PLAYERTYPE_GDI) gdi++;
		else if (Nodes[i].Faction == PLAYERTYPE_NOD) nod++;
	}

	int faults = 0;
	if (gdi < min_per_faction) {
		faults++;
		if (report) { StringClass l; l.Format("GDI has %d base candidate(s), fewer than %d\r\n",gdi,min_per_faction); *report += l; }
	}
	if (nod < min_per_faction) {
		faults++;
		if (report) { StringClass l; l.Format("Nod has %d base candidate(s), fewer than %d\r\n",nod,min_per_faction); *report += l; }
	}
	return faults;
}


int StrategicPlanner::Check_Tiberium_Minimum(int min_count,StringClass * report)
{
	int count = Get_Node_Count_By_Type(STRATEGIC_NODE_RESOURCE_CANDIDATE);
	if (count < min_count) {
		if (report) { StringClass l; l.Format("%d resource candidate(s), fewer than %d\r\n",count,min_count); *report += l; }
		return 1;
	}
	return 0;
}


int StrategicPlanner::Check_No_Cliff_Or_Water_Overlap(StringClass * report)
{
	int faults = 0;
	for (int i = 0; i < Nodes.Count(); i++) {
		const StrategicNode & node = Nodes[i];
		if ((node.Type != STRATEGIC_NODE_BASE_CANDIDATE) && (node.Type != STRATEGIC_NODE_RESOURCE_CANDIDATE) &&
			 (node.Type != STRATEGIC_NODE_SETTLEMENT_CANDIDATE) && (node.Type != STRATEGIC_NODE_POI_CANDIDATE)) {
			continue;		// crossing/tunnel candidates are meant to sit on a barrier
		}

		if (WaterSystem::Is_Point_In_Water(node.Position.X,node.Position.Y)) {
			faults++;
			if (report) { StringClass l; l.Format("node '%s' sits in water\r\n",node.Id.Peek_Buffer()); *report += l; }
		}
		if (WorldBoundsService::Is_Configured() &&
			 (WorldBoundsService::Classify_Point(Vector2(node.Position.X,node.Position.Y)) >= WORLD_ZONE_CLIFF_BAND)) {
			faults++;
			if (report) { StringClass l; l.Format("node '%s' lies in the cliff band\r\n",node.Id.Peek_Buffer()); *report += l; }
		}
	}
	return faults;
}


int StrategicPlanner::Check_No_Economically_Disconnected_Resource(float max_range,StringClass * report)
{
	int faults = 0;
	for (int i = 0; i < Nodes.Count(); i++) {
		if (Nodes[i].Type != STRATEGIC_NODE_RESOURCE_CANDIDATE) continue;

		float nearest = -1.0f;
		for (int n = 0; n < Nodes.Count(); n++) {
			if (Nodes[n].Type != STRATEGIC_NODE_BASE_CANDIDATE) continue;
			float d = (Nodes[i].Position - Nodes[n].Position).Length();
			if ((nearest < 0.0f) || (d < nearest)) nearest = d;
		}
		if ((nearest < 0.0f) || (nearest > max_range)) {
			faults++;
			if (report) { StringClass l; l.Format("resource '%s' has no base within range\r\n",Nodes[i].Id.Peek_Buffer()); *report += l; }
		}
	}
	return faults;
}
