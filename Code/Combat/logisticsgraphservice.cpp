/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/logisticsgraphservice.cpp             $*
 *                                                                                             *
 *	V2 roadmap Section 34 Phase 21 -- see the comment in logisticsgraphservice.h.               *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "logisticsgraphservice.h"

#include "actionparams.h"
#include "baseselectionplanner.h"
#include "bridgesystem.h"
#include "buildingdescriptor.h"
#include "buildingdescriptormanager.h"
#include "building.h"
#include "crc.h"
#include "expansionregionregistry.h"
#include "gameobjmanager.h"
#include "physicalgameobj.h"
#include "playertype.h"
#include "repairbaygameobj.h"
#include "roadgraphgenerator.h"
#include "roadsystem.h"
#include "scriptablegameobj.h"
#include "scriptcommands.h"
#include "settlementgenerator.h"
#include "strategicplanner.h"
#include "teampowergridservice.h"
#include "tiberiumfieldgenerator.h"

static const float	CHECKSUM_QUANTUM		= 1000.0f;
static const float	CONVOY_ARRIVAL_RADIUS	= 5.0f;
static const float	BRIDGE_SAMPLE_HEIGHT_GAP	= 6.0f;
static const float	ORDER_PRIORITY			= 100.0f;		// matches Dispatch_Order/CommanderGuardAI's own


bool				LogisticsGraphService::Built						= false;
bool				LogisticsGraphService::ConvoyEnabled			= false;
float				LogisticsGraphService::ConstructionSupportRadius	= 0.0f;
unsigned int	LogisticsGraphService::Checksum					= 0;

DynamicVectorClass<LogisticsNode>								LogisticsGraphService::Nodes;
DynamicVectorClass<LogisticsRoute>								LogisticsGraphService::Routes;
DynamicVectorClass<LogisticsGraphService::ConvoyRecordStruct *>	LogisticsGraphService::Convoys;


const char * Logistics_Node_Kind_Name(LogisticsNodeKindType kind)
{
	switch (kind) {
		case LOGISTICS_NODE_BASE:				return "base";
		case LOGISTICS_NODE_REFINERY:			return "refinery";
		case LOGISTICS_NODE_FACTORY:			return "factory";
		case LOGISTICS_NODE_REPAIR:				return "repair";
		case LOGISTICS_NODE_FORWARD_BASE:		return "forward_base";
		case LOGISTICS_NODE_RESOURCE_FIELD:	return "resource_field";
		case LOGISTICS_NODE_LOGISTICS_POI:	return "logistics_poi";
		default:										return "unknown";
	}
}


const char * Logistics_Route_Status_Name(LogisticsRouteStatusType status)
{
	switch (status) {
		case LOGISTICS_ROUTE_OPEN:					return "open";
		case LOGISTICS_ROUTE_BLOCKED_BRIDGE:	return "blocked_bridge";
		case LOGISTICS_ROUTE_UNREACHABLE:		return "unreachable";
		default:											return "unknown";
	}
}


void LogisticsGraphService::Init(void)		{ Clear(); }
void LogisticsGraphService::Shutdown(void)	{ Clear(); }


void LogisticsGraphService::Clear(void)
{
	Nodes.Delete_All();
	Routes.Delete_All();
	for (int i = 0; i < Convoys.Count(); i++) { delete Convoys[i]; }
	Convoys.Delete_All();
	Built						= false;
	ConvoyEnabled			= false;
	ConstructionSupportRadius	= 0.0f;
	Checksum					= 0;
}


bool LogisticsGraphService::Rebuild_Nodes(bool convoy_enabled,float construction_support_radius,
														 StringClass * error_out)
{
	Nodes.Delete_All();
	Routes.Delete_All();
	for (int i = 0; i < Convoys.Count(); i++) { delete Convoys[i]; }
	Convoys.Delete_All();
	Built = false;

	ConvoyEnabled				= convoy_enabled;
	ConstructionSupportRadius	= (construction_support_radius > 0.0f) ? construction_support_radius : 0.0f;

	//	BASE.  One per faction that actually has a selected site -- a plan too small to have
	//	chosen one is a correct, unremarkable outcome, not an error (the same "no plan, no
	//	nodes" tolerance every consumer of StrategicPlanner/BaseSelectionPlanner already grants).
	{
		const int factions[2] = { PLAYERTYPE_GDI,PLAYERTYPE_NOD };
		for (int f = 0; f < 2; f++) {
			SelectedBaseSite site;
			if (!BaseSelectionPlanner::Has_Selection() || !BaseSelectionPlanner::Get_Selected_Site(factions[f],&site)) {
				continue;
			}
			LogisticsNode node;
			node.Kind			= LOGISTICS_NODE_BASE;
			node.Id.Format("logistics_base_%s",(factions[f] == PLAYERTYPE_GDI) ? "gdi" : "nod");
			node.Team			= factions[f];
			node.Position		= site.Position;
			node.BuildingId	= -1;
			node.Operational	= true;
			int strategic_index = -1;
			StrategicPlanner::Find_Node_By_Id(site.Id,&strategic_index);
			node.StrategicNodeIndex = strategic_index;
			Nodes.Add(node);
		}
	}

	//	FORWARD_BASE.  Every ExpansionRegion the Commander has actually built that is not the
	//	starting base -- real Phase 17 data, not re-derived here.  No StrategicPlanner origin of
	//	its own (a forward base is a runtime cluster, not a plan-time candidate), so its route
	//	search falls back to the nearest real strategic node to its own position.
	{
		for (int i = 0; i < ExpansionRegionRegistry::Get_Region_Count(); i++) {
			ExpansionRegion region;
			if (!ExpansionRegionRegistry::Get_Region(i,&region) || !region.IsForward) { continue; }

			LogisticsNode node;
			node.Kind				= LOGISTICS_NODE_FORWARD_BASE;
			node.Id					= region.Id;
			node.Team				= region.Team;
			node.Position			= region.Center;
			node.BuildingId		= -1;
			node.Operational		= (region.StructureCount > 0);
			node.StrategicNodeIndex	= Find_Nearest_Strategic_Node(region.Center);
			Nodes.Add(node);
		}
	}

	//	RESOURCE_FIELD.  Every resolved Tiberium field -- real footprint, real route data of its
	//	own (HasHarvesterRoute) already computed by TiberiumFieldGenerator, not re-derived here.
	{
		for (int i = 0; i < TiberiumFieldGenerator::Get_Field_Count(); i++) {
			ResolvedTiberiumField field;
			if (!TiberiumFieldGenerator::Get_Field(i,&field)) { continue; }

			LogisticsNode node;
			node.Kind			= LOGISTICS_NODE_RESOURCE_FIELD;
			node.Id				= field.Id;
			node.Team			= field.OwningFaction;
			node.Position		= field.Position;
			node.BuildingId	= -1;
			node.Operational	= field.HasHarvesterRoute;
			int strategic_index = -1;
			StrategicPlanner::Find_Node_By_Id(field.ResourceNodeId,&strategic_index);
			node.StrategicNodeIndex = strategic_index;
			Nodes.Add(node);
		}
	}

	//	LOGISTICS_POI.  Every resolved POI -- neutral, nobody's team, per settlementgenerator.h's
	//	own comment on ResolvedPoi.
	{
		for (int i = 0; i < SettlementGenerator::Get_Poi_Count(); i++) {
			ResolvedPoi poi;
			if (!SettlementGenerator::Get_Poi(i,&poi)) { continue; }

			LogisticsNode node;
			node.Kind			= LOGISTICS_NODE_LOGISTICS_POI;
			node.Id				= poi.Id;
			node.Team			= PLAYERTYPE_NEUTRAL;
			node.Position		= poi.Position;
			node.BuildingId	= -1;
			node.Operational	= poi.HasRealAsset;
			int strategic_index = -1;
			StrategicPlanner::Find_Node_By_Id(poi.Id,&strategic_index);
			node.StrategicNodeIndex = strategic_index;
			Nodes.Add(node);
		}
	}

	Built = true;
	Recompute_Checksum();

	if (error_out != nullptr) { *error_out = ""; }
	return true;
}


void LogisticsGraphService::Rebuild_Building_Nodes(void)
{
	//	Real REFINERY/FACTORY/REPAIR nodes, from live buildings.  Uses the same two-step
	//	BaseGameObj -> ScriptableGameObj -> BuildingGameObj cast Phase 20's own
	//	building_subcomponents console command already established for walking every building in
	//	the world.
	SLNode<BaseGameObj> * objnode;
	for (objnode = GameObjManager::Get_Game_Obj_List()->Head(); objnode; objnode = objnode->Next()) {

		BaseGameObj * base = objnode->Data();
		if (base == nullptr) { continue; }

		ScriptableGameObj * scriptable = base->As_ScriptableGameObj();
		BuildingGameObj * building = (scriptable != nullptr) ? scriptable->As_BuildingGameObj() : nullptr;
		if (building == nullptr) { continue; }

		const BuildingModelDescriptor * descriptor = nullptr;
		const StringClass & descriptor_name = building->Get_Definition().Get_Descriptor_Name();
		if (descriptor_name.Get_Length() > 0) {
			descriptor = BuildingDescriptorManager::Get_Descriptor(descriptor_name.Peek_Buffer());
		}

		LogisticsNodeKindType kind;
		if (building->As_RepairBayGameObj() != nullptr) {
			kind = LOGISTICS_NODE_REPAIR;
		} else if ((descriptor != nullptr) && descriptor->Has_Subcomponent(SUBCOMPONENT_REFINERY_PROCESSOR)) {
			kind = LOGISTICS_NODE_REFINERY;
		} else if ((descriptor != nullptr) && (descriptor->Production.Kind != BUILDING_PRODUCTION_NONE)) {
			kind = LOGISTICS_NODE_FACTORY;
		} else {
			continue;			// not a logistics-relevant building
		}

		int building_id = building->Get_ID();

		LogisticsNode node;
		node.Kind			= kind;
		node.Id.Format("logistics_building_%d",building_id);
		node.Team			= building->Get_Player_Type();
		building->Get_Position(&node.Position);
		node.BuildingId	= building_id;
		node.Operational	= building->Are_Capabilities_Enabled() &&
									 TeamPowerGridService::Is_Member_Powered((PowerGridMemberIdType)building_id);
		node.StrategicNodeIndex = Find_Nearest_Strategic_Node(node.Position);
		Nodes.Add(node);
	}

	Recompute_Checksum();
}


int LogisticsGraphService::Find_Nearest_Strategic_Node(const Vector3 & position)
{
	int best_index = -1;
	float best_distance2 = 0.0f;

	for (int i = 0; i < StrategicPlanner::Get_Node_Count(); i++) {
		StrategicNode candidate;
		if (!StrategicPlanner::Get_Node(i,&candidate)) { continue; }

		float distance2 = (candidate.Position - position).Length2();
		if ((best_index < 0) || (distance2 < best_distance2)) {
			best_index		= i;
			best_distance2	= distance2;
		}
	}

	return best_index;
}


bool LogisticsGraphService::Add_Route_If_New(int node_a,int node_b)
{
	if (node_a == node_b) { return false; }

	//	Undirected: always store the lower index first, so A->B and B->A collapse into one
	//	route -- RoadGraphGenerator's own path is symmetric, and Update_Route_Status only has
	//	one entry to revise either direction from.
	int from = (node_a < node_b) ? node_a : node_b;
	int to	= (node_a < node_b) ? node_b : node_a;

	int existing = -1;
	if (Find_Route_Between(from,to,&existing)) { return false; }

	const LogisticsNode & node_from = Nodes[from];
	const LogisticsNode & node_to	 = Nodes[to];

	LogisticsRoute route;
	route.Id.Format("logistics_route_%s_%s",node_from.Id.Peek_Buffer(),node_to.Id.Peek_Buffer());
	route.FromNodeIndex	= from;
	route.ToNodeIndex		= to;

	if ((node_from.StrategicNodeIndex < 0) || (node_to.StrategicNodeIndex < 0)) {
		route.Status	= LOGISTICS_ROUTE_UNREACHABLE;
		route.Length	= -1.0f;
		Routes.Add(route);
		return true;
	}

	DynamicVectorClass<int> edge_indices;
	float length = -1.0f;
	if (!RoadGraphGenerator::Find_Anchor_Path(node_from.StrategicNodeIndex,node_to.StrategicNodeIndex,
															 &edge_indices,&length)) {
		route.Status	= LOGISTICS_ROUTE_UNREACHABLE;
		route.Length	= -1.0f;
		Routes.Add(route);
		return true;
	}

	route.Status	= LOGISTICS_ROUTE_OPEN;
	route.Length	= length;

	for (int i = 0; i < edge_indices.Count(); i++) {
		RoadGraphEdge edge;
		if (!RoadGraphGenerator::Get_Edge(edge_indices[i],&edge)) { continue; }

		RoadSplineClass * road = RoadSystem::Peek_Road(edge.RoadId);
		if (road == nullptr) { continue; }

		for (int p = 0; p < road->Get_Point_Count(); p++) {
			route.Waypoints.Add(road->Peek_Point(p).Position);
		}
	}

	Routes.Add(route);
	return true;
}


bool LogisticsGraphService::Rebuild_Routes(StringClass * error_out)
{
	if (!Built) {
		if (error_out != nullptr) { *error_out = "Rebuild_Routes called before Rebuild_Nodes"; }
		return false;
	}

	Routes.Delete_All();

	for (int a = 0; a < Nodes.Count(); a++) {
		const LogisticsNode & node_a = Nodes[a];

		for (int b = a + 1; b < Nodes.Count(); b++) {
			const LogisticsNode & node_b = Nodes[b];

			bool relevant = false;

			//	Harvester routes: a base/forward base to a resource field of the same team.
			if ((node_a.Kind == LOGISTICS_NODE_RESOURCE_FIELD) || (node_b.Kind == LOGISTICS_NODE_RESOURCE_FIELD)) {
				const LogisticsNode & field	= (node_a.Kind == LOGISTICS_NODE_RESOURCE_FIELD) ? node_a : node_b;
				const LogisticsNode & other	= (node_a.Kind == LOGISTICS_NODE_RESOURCE_FIELD) ? node_b : node_a;
				if (((other.Kind == LOGISTICS_NODE_BASE) || (other.Kind == LOGISTICS_NODE_FORWARD_BASE)) &&
					 (other.Team == field.Team)) {
					relevant = true;
				}
			}

			//	Forward-position routes: a base to its own team's forward base.
			if (((node_a.Kind == LOGISTICS_NODE_BASE) && (node_b.Kind == LOGISTICS_NODE_FORWARD_BASE) && (node_a.Team == node_b.Team)) ||
				 ((node_b.Kind == LOGISTICS_NODE_BASE) && (node_a.Kind == LOGISTICS_NODE_FORWARD_BASE) && (node_a.Team == node_b.Team))) {
				relevant = true;
			}

			//	Staging/repair routes: a base/forward base to its own team's factory/refinery/repair.
			if ((node_a.Kind == LOGISTICS_NODE_FACTORY) || (node_a.Kind == LOGISTICS_NODE_REFINERY) || (node_a.Kind == LOGISTICS_NODE_REPAIR) ||
				 (node_b.Kind == LOGISTICS_NODE_FACTORY) || (node_b.Kind == LOGISTICS_NODE_REFINERY) || (node_b.Kind == LOGISTICS_NODE_REPAIR)) {
				const LogisticsNode & facility = ((node_a.Kind == LOGISTICS_NODE_FACTORY) || (node_a.Kind == LOGISTICS_NODE_REFINERY) ||
																 (node_a.Kind == LOGISTICS_NODE_REPAIR)) ? node_a : node_b;
				const LogisticsNode & other		= ((node_a.Kind == LOGISTICS_NODE_FACTORY) || (node_a.Kind == LOGISTICS_NODE_REFINERY) ||
																 (node_a.Kind == LOGISTICS_NODE_REPAIR)) ? node_b : node_a;
				if (((other.Kind == LOGISTICS_NODE_BASE) || (other.Kind == LOGISTICS_NODE_FORWARD_BASE)) &&
					 (other.Team == facility.Team)) {
					relevant = true;
				}
			}

			//	POI routes: any base/forward base to any POI -- nobody owns a POI, so team does
			//	not gate this one.
			if ((node_a.Kind == LOGISTICS_NODE_LOGISTICS_POI) || (node_b.Kind == LOGISTICS_NODE_LOGISTICS_POI)) {
				const LogisticsNode & other = (node_a.Kind == LOGISTICS_NODE_LOGISTICS_POI) ? node_b : node_a;
				if ((other.Kind == LOGISTICS_NODE_BASE) || (other.Kind == LOGISTICS_NODE_FORWARD_BASE)) {
					relevant = true;
				}
			}

			if (relevant) { Add_Route_If_New(a,b); }
		}
	}

	Recompute_Checksum();

	if (error_out != nullptr) { *error_out = ""; }
	return true;
}


void LogisticsGraphService::Update_Route_Status(void)
{
	for (int i = 0; i < Routes.Count(); i++) {
		LogisticsRoute & route = Routes[i];
		if (route.Status == LOGISTICS_ROUTE_UNREACHABLE) { continue; }

		int blocking_bridge = -1;
		for (int p = 0; p < route.Waypoints.Count(); p++) {
			int bridge_id = BridgeSystem::Find_Bridge_At(route.Waypoints[p],BRIDGE_SAMPLE_HEIGHT_GAP);
			if ((bridge_id >= 0) && !BridgeSystem::Is_Traversable(bridge_id)) {
				blocking_bridge = bridge_id;
				break;
			}
		}

		route.Status				= (blocking_bridge >= 0) ? LOGISTICS_ROUTE_BLOCKED_BRIDGE : LOGISTICS_ROUTE_OPEN;
		route.BlockingBridgeId	= blocking_bridge;
	}
}


bool LogisticsGraphService::Get_Node(int index,LogisticsNode * out)
{
	if ((index < 0) || (index >= Nodes.Count()) || (out == nullptr)) { return false; }
	*out = Nodes[index];
	return true;
}


bool LogisticsGraphService::Find_Node_By_Id(const StringClass & id,int * index_out)
{
	for (int i = 0; i < Nodes.Count(); i++) {
		if (Nodes[i].Id == id) {
			if (index_out != nullptr) { *index_out = i; }
			return true;
		}
	}
	return false;
}


bool LogisticsGraphService::Get_Route(int index,LogisticsRoute * out)
{
	if ((index < 0) || (index >= Routes.Count()) || (out == nullptr)) { return false; }
	*out = Routes[index];
	return true;
}


bool LogisticsGraphService::Find_Route_Between(int node_a,int node_b,int * index_out)
{
	int from = (node_a < node_b) ? node_a : node_b;
	int to	= (node_a < node_b) ? node_b : node_a;

	for (int i = 0; i < Routes.Count(); i++) {
		if ((Routes[i].FromNodeIndex == from) && (Routes[i].ToNodeIndex == to)) {
			if (index_out != nullptr) { *index_out = i; }
			return true;
		}
	}
	return false;
}


bool LogisticsGraphService::Is_Under_Construction_Support(const Vector3 & position,int team)
{
	if (ConstructionSupportRadius <= 0.0f) { return false; }

	for (int i = 0; i < Nodes.Count(); i++) {
		const LogisticsNode & node = Nodes[i];
		if ((node.Kind != LOGISTICS_NODE_FORWARD_BASE) || (node.Team != team) || !node.Operational) { continue; }

		if ((node.Position - position).Length() <= ConstructionSupportRadius) { return true; }
	}

	return false;
}


bool LogisticsGraphService::Start_Convoy(int unit_id,int route_index)
{
	if (!ConvoyEnabled) { return false; }
	if ((route_index < 0) || (route_index >= Routes.Count())) { return false; }

	const LogisticsRoute & route = Routes[route_index];
	if ((route.Status != LOGISTICS_ROUTE_OPEN) || (route.Waypoints.Count() == 0)) { return false; }

	Stop_Convoy(unit_id);

	ConvoyRecordStruct * record = new ConvoyRecordStruct;
	record->UnitId			= unit_id;
	record->RouteIndex		= route_index;
	record->WaypointIndex	= 0;
	Convoys.Add(record);

	return true;
}


void LogisticsGraphService::Stop_Convoy(int unit_id)
{
	for (int i = 0; i < Convoys.Count(); i++) {
		if (Convoys[i]->UnitId == unit_id) {
			delete Convoys[i];
			Convoys.Delete(i);
			return;
		}
	}
}


bool LogisticsGraphService::Is_In_Convoy(int unit_id)
{
	for (int i = 0; i < Convoys.Count(); i++) {
		if (Convoys[i]->UnitId == unit_id) { return true; }
	}
	return false;
}


int LogisticsGraphService::Get_Active_Convoy_Count(void)
{
	return Convoys.Count();
}


void LogisticsGraphService::Update(float seconds)
{
	(void)seconds;
	if (Convoys.Count() == 0) { return; }

	for (int i = Convoys.Count() - 1; i >= 0; i--) {
		ConvoyRecordStruct * record = Convoys[i];

		PhysicalGameObj * unit = GameObjManager::Find_PhysicalGameObj(record->UnitId);
		if ((unit == nullptr) || (record->RouteIndex < 0) || (record->RouteIndex >= Routes.Count())) {
			delete record;
			Convoys.Delete(i);
			continue;
		}

		const LogisticsRoute & route = Routes[record->RouteIndex];
		if ((route.Status != LOGISTICS_ROUTE_OPEN) || (record->WaypointIndex >= route.Waypoints.Count())) {
			//	Blocked mid-transit, or already arrived -- either way, nothing further to drive.
			delete record;
			Convoys.Delete(i);
			continue;
		}

		Vector3 position;
		unit->Get_Position(&position);

		const Vector3 & target = route.Waypoints[record->WaypointIndex];
		if ((target - position).Length() <= CONVOY_ARRIVAL_RADIUS) {
			record->WaypointIndex++;
			if (record->WaypointIndex >= route.Waypoints.Count()) {
				delete record;
				Convoys.Delete(i);
				continue;
			}
		}

		ActionParamsStruct params;
		params.Set_Basic(0,ORDER_PRIORITY,0);
		params.Set_Movement(route.Waypoints[record->WaypointIndex],1.0f,1.5f,false);
		ScriptEngine::Action_Goto(unit,params);
	}
}


void LogisticsGraphService::Recompute_Checksum(void)
{
	CRCEngine crc;
	for (int i = 0; i < Nodes.Count(); i++) {
		const LogisticsNode & node = Nodes[i];
		int q_kind	= (int)node.Kind;
		int q_team	= node.Team;
		int q_x		= (int)(node.Position.X * CHECKSUM_QUANTUM);
		int q_y		= (int)(node.Position.Y * CHECKSUM_QUANTUM);
		crc(&q_kind,sizeof(q_kind));
		crc(&q_team,sizeof(q_team));
		crc(&q_x,sizeof(q_x));
		crc(&q_y,sizeof(q_y));
	}
	for (int i = 0; i < Routes.Count(); i++) {
		const LogisticsRoute & route = Routes[i];
		int q_status	= (int)route.Status;
		int q_length	= (int)(route.Length * CHECKSUM_QUANTUM);
		crc(&route.FromNodeIndex,sizeof(route.FromNodeIndex));
		crc(&route.ToNodeIndex,sizeof(route.ToNodeIndex));
		crc(&q_status,sizeof(q_status));
		crc(&q_length,sizeof(q_length));
	}
	Checksum = (unsigned int)crc;
}
