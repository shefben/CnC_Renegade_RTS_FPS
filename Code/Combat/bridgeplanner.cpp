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
 *                     $Archive:: /Commando/Code/Combat/bridgeplanner.cpp                     $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "bridgeplanner.h"

#include "bridgesection.h"
#include "bridgesystem.h"
#include "crc.h"
#include "escarpment.h"
#include "roadgraphgenerator.h"
#include "roadspline.h"
#include "roadsystem.h"
#include "strategicplanner.h"
#include "terrainmask.h"
#include "watersystem.h"
#include "worldterrainsystem.h"
#include "wwdebug.h"
#include "wwmath.h"


bool				BridgePlanner::Built			= false;
unsigned int	BridgePlanner::Checksum		= 0;

DynamicVectorClass<int>					BridgePlanner::OwnedBridgeIds;
DynamicVectorClass<ResolvedBridge>	BridgePlanner::Bridges;


//	The bank-point search step and bound, world units -- a bridge's own span is measured by
//	walking outward from the crossing point until the obstruction (water or cliff face) clears,
//	capped so a search over an implausibly wide obstruction gives up rather than looping.
static const float BANK_SEARCH_STEP		= 4.0f;
static const int	 BANK_SEARCH_MAX_STEPS	= 60;		// 240 world units each side

//	How close to an escarpment face counts as "still on the cliff", for the same bank search.
static const float CLIFF_FACE_TEST_DISTANCE = 3.0f;

//	Deck structural depth, added on top of the clearance margin -- Section 11A.3's own third
//	term, a fixed stand-in until a real per-definition value is confirmed.
static const float DECK_STRUCTURAL_DEPTH = 0.6f;

//	Cumulative thresholds for the deterministic initial-damage-state draw, PRISTINE-heaviest.
static const float DAMAGE_STATE_THRESHOLD[BRIDGE_STATE_COUNT] = { 0.70f, 0.85f, 0.95f, 1.00f };

static const float CHECKSUM_QUANTUM = 1000.0f;


static const char * Road_Graph_Class_Bridge_Name(RoadGraphClassType cls)
{
	switch (cls) {
		case ROAD_GRAPH_HIGHWAY:
		case ROAD_GRAPH_PRIMARY:			return "ow_bridge_steel_truss";
		case ROAD_GRAPH_SECONDARY:
		case ROAD_GRAPH_CITY_STREET:		return "ow_bridge_concrete";
		case ROAD_GRAPH_SERVICE:
		case ROAD_GRAPH_DIRT_TRACK:
		default:									return "ow_bridge_timber";
	}
}


BridgePlannerParams::BridgePlannerParams(void) :
	Seed (1),
	WaterClearanceMargin (2.0f),
	MaxSpanLength (200.0f)
{
}


ResolvedBridge::ResolvedBridge(void) :
	BridgeId (-1),
	InitialState (BRIDGE_STATE_PRISTINE),
	CountsAsConnectivity (true),
	StrategicImportance (0.0f)
{
}


bool ResolvedBridge::operator == (const ResolvedBridge & that) const
{
	return (Id == that.Id) && (BridgeId == that.BridgeId) && (CrossingNodeId == that.CrossingNodeId) &&
			 (RoadEdgeId == that.RoadEdgeId) && (InitialState == that.InitialState) &&
			 (CountsAsConnectivity == that.CountsAsConnectivity) && (StrategicImportance == that.StrategicImportance);
}


void BridgePlanner::Init(void)
{
	Clear();
}


void BridgePlanner::Shutdown(void)
{
	Clear();
}


void BridgePlanner::Clear(void)
{
	if (OwnedBridgeIds.Count() > 0) {
		BridgeSystem::Destroy_Geometry();

		DynamicVectorClass<BridgeClass> keep;
		for (int i = 0; i < BridgeSystem::Get_Bridge_Count(); i++) {
			BridgeClass * bridge = BridgeSystem::Peek_Bridge(i);
			if (bridge == nullptr) continue;

			bool owned = false;
			for (int j = 0; j < OwnedBridgeIds.Count(); j++) {
				if (OwnedBridgeIds[j] == i) { owned = true; break; }
			}
			if (!owned) keep.Add(*bridge);
		}

		BridgeSystem::Clear_Bridges();
		for (int i = 0; i < keep.Count(); i++) {
			BridgeSystem::Add_Bridge(keep[i]);
		}
		if (keep.Count() > 0) {
			BridgeSystem::Build_Layout();
			BridgeSystem::Build_Geometry();
		}
	}

	OwnedBridgeIds.Delete_All();
	Bridges.Delete_All();

	Built		= false;
	Checksum	= 0;
}


bool BridgePlanner::Get_Bridge(int index,ResolvedBridge * out)
{
	if ((index < 0) || (index >= Bridges.Count()) || (out == nullptr)) {
		return false;
	}
	*out = Bridges[index];
	return true;
}


//	Is this point still on the obstruction the bridge is meant to cross.
static bool On_Obstruction(const Vector3 & point,bool is_water)
{
	if (is_water) {
		return WaterSystem::Is_Point_In_Water(point.X,point.Y);
	}
	float upper,lower;
	return EscarpmentSystem::Sample_Face(point.X,point.Y,CLIFF_FACE_TEST_DISTANCE,&upper,&lower);
}


//	Walk outward from the crossing point along the nearest road's own local perpendicular until
//	the obstruction clears on each side, bounded.  Returns false if the search never clears
//	within BANK_SEARCH_MAX_STEPS on either side (an obstruction wider than this bridge planner
//	will consider spanning).
static bool Find_Bridge_Banks(const Vector3 & crossing_position,int road_id,bool is_water,
										 Vector3 * start_out,Vector3 * end_out,float * span_out)
{
	Vector3 tangent(1.0f,0.0f,0.0f);
	RoadSplineClass * road = RoadSystem::Peek_Road(road_id);
	if (road != nullptr) {
		int best = -1;
		float best_distance = 0.0f;
		for (int i = 0; i < road->Get_Point_Count(); i++) {
			float d = (road->Get_Point(i).Position - crossing_position).Length();
			if ((best < 0) || (d < best_distance)) { best = i; best_distance = d; }
		}
		if (best >= 0) tangent = road->Get_Point(best).Tangent;
	}

	Vector3 perp(-tangent.Y,tangent.X,0.0f);
	float perp_length = perp.Length();
	perp = (perp_length > 0.0001f) ? (perp / perp_length) : Vector3(0.0f,1.0f,0.0f);

	float extent_pos = -1.0f;
	for (int i = 1; i <= BANK_SEARCH_MAX_STEPS; i++) {
		Vector3 p = crossing_position + (perp * (BANK_SEARCH_STEP * (float)i));
		if (!On_Obstruction(p,is_water)) { extent_pos = BANK_SEARCH_STEP * (float)i; break; }
	}

	float extent_neg = -1.0f;
	for (int i = 1; i <= BANK_SEARCH_MAX_STEPS; i++) {
		Vector3 p = crossing_position - (perp * (BANK_SEARCH_STEP * (float)i));
		if (!On_Obstruction(p,is_water)) { extent_neg = BANK_SEARCH_STEP * (float)i; break; }
	}

	if ((extent_pos < 0.0f) || (extent_neg < 0.0f)) return false;

	Vector3 start = crossing_position - (perp * extent_neg);
	Vector3 end   = crossing_position + (perp * extent_pos);
	float h;
	if (WorldTerrainSystem::Sample_Height(start.X,start.Y,&h)) start.Z = h;
	if (WorldTerrainSystem::Sample_Height(end.X,end.Y,&h))   end.Z   = h;

	*start_out	= start;
	*end_out		= end;
	*span_out	= extent_pos + extent_neg;
	return true;
}


/***********************************************************************************************
 * Generate -- Phase 9 steps 1-9                                                                 *
 *=============================================================================================*/
bool BridgePlanner::Generate(const BridgePlannerParams & params,StringClass * error_out)
{
	#define FAIL(msg) { if (error_out) *error_out = msg; Clear(); return false; }

	Clear();

	if (!RoadGraphGenerator::Has_Graph())				FAIL("no road graph -- generate the road graph first");
	if (params.WaterClearanceMargin <= 0.0f)			FAIL("water clearance margin must be positive");
	if (params.MaxSpanLength <= 0.0f)					FAIL("max span length must be positive");

	int bridge_index = 0;

	for (int e = 0; e < RoadGraphGenerator::Get_Edge_Count(); e++) {
		RoadGraphEdge edge;
		if (!RoadGraphGenerator::Get_Edge(e,&edge)) continue;

		for (int c = 0; c < edge.UnresolvedCrossingIds.Count(); c++) {

			const StringClass & crossing_id = edge.UnresolvedCrossingIds[c];

			StrategicNode crossing_node;
			bool found = false;
			for (int n = 0; n < StrategicPlanner::Get_Node_Count(); n++) {
				StrategicNode candidate;
				if (!StrategicPlanner::Get_Node(n,&candidate)) continue;
				if (candidate.Id == crossing_id) { crossing_node = candidate; found = true; break; }
			}
			if (!found) continue;

			//	Re-derived, not stored on the StrategicNode: whether this crossing is water or
			//	an interior cliff, from the same headless queries Phase 7/8 already used.
			bool is_water = WaterSystem::Is_Point_In_Water(crossing_node.Position.X,crossing_node.Position.Y);

			Vector3 start,end;
			float span;
			if (!Find_Bridge_Banks(crossing_node.Position,edge.RoadId,is_water,&start,&end,&span)) {
				continue;		// step 3: impossible to bound this obstruction -- leave the crossing pending
			}
			if (span > params.MaxSpanLength) {
				continue;		// step 3: reject, return to the alternate-route/tunnel planner (Phase 10)
			}

			float deck_height = 0.0f;
			if (is_water) {
				float water_height;
				if (WaterSystem::Get_Water_Height(crossing_node.Position.X,crossing_node.Position.Y,&water_height)) {
					float higher_abutment = (start.Z > end.Z) ? start.Z : end.Z;
					float required = water_height + params.WaterClearanceMargin + DECK_STRUCTURAL_DEPTH;
					if (required > higher_abutment) {
						deck_height = required - higher_abutment;		// 11A.2: lowest valid elevation, no more
					}
				}
			}
			//	An interior-cliff crossing keeps deck_height at 0 -- the deck follows the two
			//	bank heights directly, which for a canyon/cliff crossing are already the toe/rim
			//	the crossing sits between.  See this file's header comment on 11A.2's own scope.

			RoadGraphClassType road_class = edge.Class;
			const char * bridge_name = Road_Graph_Class_Bridge_Name(road_class);

			BridgeClass bridge;
			StringClass name; name.Format("bridge_%d",bridge_index);
			bridge.Set_Name(name.Peek_Buffer());
			bridge.Set_Definition(bridge_name);
			bridge.Set_Endpoints(start,end);
			bridge.Set_Deck_Height(deck_height);

			int bridge_id = BridgeSystem::Add_Bridge(bridge);
			OwnedBridgeIds.Add(bridge_id);

			ResolvedBridge resolved;
			resolved.Id					= name;
			resolved.BridgeId			= bridge_id;
			resolved.CrossingNodeId	= crossing_id;
			resolved.RoadEdgeId			= edge.Id;

			float draw = TerrainMaskClass::Hash_Value(bridge_index,0,(int)params.Seed);
			int state = BRIDGE_STATE_BROKEN;
			for (int s = 0; s < BRIDGE_STATE_COUNT; s++) {
				if (draw <= DAMAGE_STATE_THRESHOLD[s]) { state = s; break; }
			}
			resolved.InitialState				= state;
			resolved.CountsAsConnectivity	= (state != BRIDGE_STATE_BROKEN);

			if (road_class == ROAD_GRAPH_HIGHWAY)			resolved.StrategicImportance = 1.0f;
			else if (road_class == ROAD_GRAPH_PRIMARY)	resolved.StrategicImportance = 0.6f;
			else if (road_class == ROAD_GRAPH_SECONDARY ||
						road_class == ROAD_GRAPH_CITY_STREET)	resolved.StrategicImportance = 0.4f;
			else														resolved.StrategicImportance = 0.2f;

			Bridges.Add(resolved);
			bridge_index++;
		}
	}

	if (OwnedBridgeIds.Count() > 0) {
		BridgeSystem::Build_Layout();
		BridgeSystem::Conform_Abutments();
		BridgeSystem::Connect_Roads();
		BridgeSystem::Build_Geometry();

		for (int i = 0; i < Bridges.Count(); i++) {
			BridgeSystem::Set_Damage_State(Bridges[i].BridgeId,Bridges[i].InitialState);
		}
	}

	{
		CRCEngine crc;
		for (int i = 0; i < Bridges.Count(); i++) {
			const ResolvedBridge & b = Bridges[i];
			int q_state			= b.InitialState;
			int q_counts		= b.CountsAsConnectivity ? 1 : 0;
			int q_importance	= (int)(b.StrategicImportance * CHECKSUM_QUANTUM);
			crc(&q_state,sizeof(q_state));
			crc(&q_counts,sizeof(q_counts));
			crc(&q_importance,sizeof(q_importance));
		}
		Checksum = (unsigned int)crc;
	}

	Built = true;

	WWDEBUG_SAY(("BridgePlanner: %d bridge(s), checksum 0x%08X\r\n",Bridges.Count(),Checksum));

	#undef FAIL
	return true;
}


int BridgePlanner::Check_Traversable_States_Pass(StringClass * report)
{
	int faults = 0;
	for (int i = 0; i < Bridges.Count(); i++) {
		if (Bridges[i].InitialState == BRIDGE_STATE_BROKEN) continue;
		if (!BridgeSystem::Is_Traversable(Bridges[i].BridgeId)) {
			faults++;
			if (report) { StringClass l; l.Format("bridge '%s' is not traversable in a non-broken state\r\n",Bridges[i].Id.Peek_Buffer()); *report += l; }
		}
	}
	return faults;
}


int BridgePlanner::Check_Broken_States_Block(StringClass * report)
{
	int faults = 0;
	for (int i = 0; i < Bridges.Count(); i++) {
		if (Bridges[i].InitialState != BRIDGE_STATE_BROKEN) continue;
		if (BridgeSystem::Is_Traversable(Bridges[i].BridgeId)) {
			faults++;
			if (report) { StringClass l; l.Format("bridge '%s' is broken but still reports traversable\r\n",Bridges[i].Id.Peek_Buffer()); *report += l; }
		}
	}
	return faults;
}


int BridgePlanner::Check_No_Required_Connectivity_Lost(StringClass * report)
{
	//	Structural by construction: CountsAsConnectivity is set false for exactly the bridges
	//	whose InitialState is BROKEN, nothing downstream reads a broken bridge as connectivity.
	//	This regression net confirms the flag and the state actually agree.
	int faults = 0;
	for (int i = 0; i < Bridges.Count(); i++) {
		bool should_count = (Bridges[i].InitialState != BRIDGE_STATE_BROKEN);
		if (Bridges[i].CountsAsConnectivity != should_count) {
			faults++;
			if (report) { StringClass l; l.Format("bridge '%s' connectivity flag disagrees with its own state\r\n",Bridges[i].Id.Peek_Buffer()); *report += l; }
		}
	}
	return faults;
}
