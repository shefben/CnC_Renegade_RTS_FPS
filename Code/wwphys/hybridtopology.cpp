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
 *                     $Archive:: /Commando/Code/wwphys/hybridtopology.cpp                    $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Runtime Phase 9.  The plan's fourteen construction steps, in its order, with each step named   *
 * where it starts.  Nothing here touches terrain: every decision is about which edges exist,     *
 * what class they are and which frozen points they must pass through.                           *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#include "hybridtopology.h"
#include "hybridroutes.h"

#include "wwdebug.h"
#include "wwmath.h"

#include <math.h>
#include <string.h>


static inline float Max2(float a,float b)	{ return (a > b) ? a : b; }
static inline float Min2(float a,float b)	{ return (a < b) ? a : b; }
static inline float Clamp2(float v,float lo,float hi)	{ return (v < lo) ? lo : ((v > hi) ? hi : v); }


/*
**	Section 8 Phase 9's own constants, each used exactly once below.
*/
static const float	ENTRANCE_POINT_W				= 1.0f;		// entrance point at R_BASE_PROTECTED + W
static const float	ENTRANCE_CORRIDOR_INNER_W	= 1.0f;		// corridor from R_BASE_PROTECTED - W ...
static const float	ENTRANCE_CORRIDOR_OUTER_W	= 6.0f;		// ... to R_BASE_PROTECTED + 6W
static const float	WINDOW_SPACING_W				= 12.0f;		// selected windows are >= 12W apart
static const float	HOME_ROAD_SECONDARY_W		= 12.0f;		// a home road under 12W is SECONDARY_2
static const float	TRAIL_MIN_W						= 8.0f;		// TRAIL_1 candidates are >= 8W apart
static const float	CROSS_LINK_MIN_W				= 8.0f;		// optional cross-link candidates too
static const float	OVERPASS_ARM_W					= 10.0f;		// 10W of segment on each side of a crossing
static const float	OVERPASS_SPACING_W			= 12.0f;		// 12W between selected overpasses
static const float	OVERPASS_SLOPE_DISC_W		= 2.0f;		// the 2W terrain disc the slope is sampled over
static const float	OVERPASS_ANGLE_MIN			= 60.0f;		// the legal crossing angle band, in degrees
static const float	OVERPASS_ANGLE_MAX			= 120.0f;
static const float	FIELD_ACCESS_TURN_FACTOR	= 1.5f;		// 1.5 * Harvester length turning disc
static const float	CROSS_LINK_PATH_WEIGHT		= 0.50f;		// score = 0.50*path + 0.30*flow + 0.20*frontier
static const float	CROSS_LINK_FLOW_WEIGHT		= 0.30f;
static const float	CROSS_LINK_FRONTIER_WEIGHT	= 0.20f;
static const float	PI_OVER_180						= 3.14159265358979323846f / 180.0f;
static const int		FIELD_ACCESS_ROTATIONS		= 16;			// the 22.5-degree rotation search


const char * Hybrid_Topology_Node_Kind_Name(HybridTopologyNodeKind kind)
{
	switch (kind) {
		case HYBRID_NODE_BASE:						return "base";
		case HYBRID_NODE_BASE_ENTRANCE:			return "base_entrance";
		case HYBRID_NODE_FAMILY_WAYPOINT:		return "family_waypoint";
		case HYBRID_NODE_CROSSING_OBLIGATION:	return "crossing_obligation";
		case HYBRID_NODE_OVERPASS_OBLIGATION:	return "overpass_obligation";
		case HYBRID_NODE_FIELD_ACCESS:			return "field_access";
		case HYBRID_NODE_INTERCHANGE_LOCAL_JUNCTION:	return "interchange_local_junction";
		default:											break;
	}
	return "unknown";
}


const char * Hybrid_Topology_Edge_Kind_Name(HybridTopologyEdgeKind kind)
{
	switch (kind) {
		case HYBRID_EDGE_BASE_INTERNAL:	return "base_internal";
		case HYBRID_EDGE_FAMILY_TRUNK:	return "family_trunk";
		case HYBRID_EDGE_HOME_ROAD:		return "home_road";
		case HYBRID_EDGE_FIELD_BRANCH:	return "field_branch";
		case HYBRID_EDGE_SERVICE_FILL:	return "service_fill";
		case HYBRID_EDGE_TRAIL:				return "trail";
		case HYBRID_EDGE_CROSS_LINK:		return "cross_link";
		case HYBRID_EDGE_HIGHWAY_RAMP:	return "highway_ramp";
		case HYBRID_EDGE_INTERCHANGE_CROSSING:	return "interchange_crossing";
		default:									break;
	}
	return "unknown";
}


const char * Hybrid_Ramp_Role_Name(HybridRampRole role)
{
	switch (role) {
		case HYBRID_RAMP_ROLE_NONE:	return "none";
		case HYBRID_RAMP_ROLE_ON:		return "on_ramp";
		case HYBRID_RAMP_ROLE_OFF:		return "off_ramp";
		default:								break;
	}
	return "unknown";
}


const char * Hybrid_Interchange_Side_Name(HybridInterchangeSide side)
{
	switch (side) {
		case HYBRID_INTERCHANGE_SIDE_RIGHT:	return "right";
		case HYBRID_INTERCHANGE_SIDE_LEFT:	return "left";
		case HYBRID_INTERCHANGE_SIDE_NONE:	return "none";
		default:										break;
	}
	return "unknown";
}


const char * Hybrid_Interchange_Form_Name(HybridInterchangeForm form)
{
	switch (form) {
		case HYBRID_INTERCHANGE_DIAMOND_ACCESS:	return "diamond_access";
		case HYBRID_INTERCHANGE_SIDE_ACCESS:		return "side_access";
		default:												break;
	}
	return "unknown";
}


const char * Hybrid_Interchange_Status_Name(HybridInterchangeStatus status)
{
	switch (status) {
		case HYBRID_INTERCHANGE_ACTIVE:		return "active";
		case HYBRID_INTERCHANGE_WITHDRAWN:	return "withdrawn";
		default:										break;
	}
	return "unknown";
}


const char * Hybrid_Interchange_Crossing_Mode_Name(HybridInterchangeCrossingMode mode)
{
	switch (mode) {
		case HYBRID_INTERCHANGE_CROSSING_NONE:					return "none";
		case HYBRID_INTERCHANGE_CROSSING_GRADE_SEPARATED:	return "grade_separated";
		case HYBRID_INTERCHANGE_CROSSING_AT_GRADE:			return "at_grade";
		default:															break;
	}
	return "unknown";
}


const char * Hybrid_Interchange_Rejection_Name(HybridInterchangeRejection rejection)
{
	switch (rejection) {
		case HYBRID_INTERCHANGE_REJECT_AXIS:				return "axis";
		case HYBRID_INTERCHANGE_REJECT_ARRIVAL_ANGLE:	return "arrival_angle";
		case HYBRID_INTERCHANGE_REJECT_ARRIVAL_SERVED:	return "arrival_served";
		case HYBRID_INTERCHANGE_REJECT_CROSSING:			return "crossing";
		case HYBRID_INTERCHANGE_REJECT_NO_CANDIDATE:		return "no_candidate";
		case HYBRID_INTERCHANGE_REJECT_QUALIFICATION:	return "qualification";
		case HYBRID_INTERCHANGE_REJECT_EDGE_CAP:			return "edge_cap";
		default:														break;
	}
	return "unknown";
}


const char * Hybrid_Topology_Failure_Name(HybridTopologyFailure failure)
{
	switch (failure) {
		case HYBRID_TOPOLOGY_OK:						return "ok";
		case HYBRID_TOPOLOGY_INVALID_INPUT:			return "invalid_input";
		case HYBRID_TOPOLOGY_NO_ENTRANCE_SECTOR:	return "no_entrance_sector";
		case HYBRID_TOPOLOGY_OBLIGATION_UNBOUND:	return "obligation_unbound";
		case HYBRID_TOPOLOGY_BRIDGE_QUOTA:			return "bridge_quota";
		case HYBRID_TOPOLOGY_NO_FIELD_ACCESS:		return "no_field_access";
		case HYBRID_TOPOLOGY_SERVICE_QUOTA:			return "service_quota";
		case HYBRID_TOPOLOGY_TRAIL_QUOTA:			return "trail_quota";
		case HYBRID_TOPOLOGY_EDGE_BUDGET:			return "edge_budget";
		case HYBRID_TOPOLOGY_NO_CYCLE:				return "no_cycle";
		case HYBRID_TOPOLOGY_UNREALIZABLE:			return "unrealizable";
		default:												break;
	}
	return "unknown";
}


static int Size_Pick(HybridSizeClass size_class,int small,int medium,int large)
{
	switch (size_class) {
		case HYBRID_SIZE_SMALL:		return small;
		case HYBRID_SIZE_MEDIUM:	return medium;
		case HYBRID_SIZE_LARGE:		return large;
		default:							break;
	}
	return medium;
}


const char * Hybrid_Quota_Refusal_Name(HybridQuotaRefusal refusal)
{
	switch (refusal) {
		case HYBRID_QUOTA_REFUSE_METHOD:				return "method";
		case HYBRID_QUOTA_REFUSE_APPROACH_CLASS:	return "approach_class";
		case HYBRID_QUOTA_REFUSE_CORRIDOR:			return "corridor";
		case HYBRID_QUOTA_REFUSE_SPACING:			return "spacing";
		case HYBRID_QUOTA_REFUSE_WRONG_SIDE:		return "wrong_side";
		case HYBRID_QUOTA_REFUSE_POSE:				return "pose";
		case HYBRID_QUOTA_REFUSE_CONSUMED:			return "consumed";
		case HYBRID_QUOTA_REFUSE_NO_BANK_PAIR:		return "no_bank_pair";
		case HYBRID_QUOTA_REFUSE_OTHER:				return "other";
		default:												break;
	}
	return "unknown";
}


const char * Hybrid_Quota_Path_Name(HybridQuotaPath path)
{
	switch (path) {
		case HYBRID_QUOTA_PATH_FILTER:		return "filter";
		case HYBRID_QUOTA_PATH_FAMILY:		return "family";
		case HYBRID_QUOTA_PATH_CROSS_LINK:	return "cross_link";
		case HYBRID_QUOTA_PATH_FALLBACK:		return "fallback";
		default:										break;
	}
	return "unknown";
}


const char * Hybrid_Overpass_Refusal_Name(HybridOverpassRefusal refusal)
{
	switch (refusal) {
		case HYBRID_OVERPASS_REFUSE_ANGLE:		return "angle";
		case HYBRID_OVERPASS_REFUSE_ARM:			return "arm_highway";
		case HYBRID_OVERPASS_REFUSE_ARM_OTHER:	return "arm_other";
		case HYBRID_OVERPASS_REFUSE_PROTECTED:	return "protected";
		case HYBRID_OVERPASS_REFUSE_WATER_CLEARANCE:	return "water_clearance";
		case HYBRID_OVERPASS_REFUSE_TIBERIUM:	return "tiberium";
		case HYBRID_OVERPASS_REFUSE_WINDOW:		return "window";
		case HYBRID_OVERPASS_REFUSE_SPACING:	return "spacing";
		case HYBRID_OVERPASS_REFUSE_SLOPE:		return "slope";
		case HYBRID_OVERPASS_REFUSE_POSE:		return "pose";
		default:											break;
	}
	return "unknown";
}


void HybridQuotaCensus::Reset(void)
{
	Required		= 0;
	Have			= 0;
	Candidates	= 0;
	Bindable		= 0;
	for (int p = 0; p < HYBRID_QUOTA_PATH_COUNT; ++p) {
		for (int r = 0; r < HYBRID_QUOTA_REFUSAL_COUNT; ++r) Refused[p][r] = 0;
	}
}


void HybridOverpassCensus::Reset(void)
{
	Required		= 0;
	Have			= 0;
	Natural		= 0;
	Pairs			= 0;
	Synthesized	= 0;
	Families		= 0;
	Highways		= 0;
	HeavyPairs	= 0;
	for (int i = 0; i < 4; ++i) AngleBucket[i] = 0;
	for (int r = 0; r < HYBRID_OVERPASS_REFUSAL_COUNT; ++r) {
		NaturalRefused[r]		= 0;
		SynthesizedRefused[r]= 0;
	}
}


int Hybrid_River_Bridge_Quota(HybridSizeClass size_class)		{ return Size_Pick(size_class,1,2,3); }
int Hybrid_Canyon_Bridge_Quota(HybridSizeClass size_class)		{ return Size_Pick(size_class,1,1,2); }
int Hybrid_Highway_Quota(HybridSizeClass size_class)				{ return Size_Pick(size_class,0,1,2); }
int Hybrid_Overpass_Target(HybridSizeClass size_class)			{ return Size_Pick(size_class,0,1,2); }
int Hybrid_Service_Branch_Quota(HybridSizeClass size_class)	{ return Size_Pick(size_class,2,4,8); }
int Hybrid_Trail_Quota(HybridSizeClass size_class)				{ return Size_Pick(size_class,1,2,4); }
int Hybrid_Cross_Link_Cap(HybridSizeClass size_class)			{ return Size_Pick(size_class,2,5,10); }


int Hybrid_Extra_Cross_Link_Count(HybridSizeClass size_class,float road_density)
{
	const float cap = (float)Hybrid_Cross_Link_Cap(size_class);
	return (int)Hybrid_Round_Half_Up(Max2(0.0f,road_density) * cap);
}


/***********************************************************************************************
**	HybridTopology and HybridTopologyParams.
***********************************************************************************************/

void HybridTopology::Reset(void)
{
	Nodes.Delete_All();
	Edges.Delete_All();
	Entrances.Delete_All();
	Interchanges.Delete_All();

	RequiredRiverBridges		= 0;
	RequiredCanyonBridges	= 0;
	RequiredHighways			= 0;
	RequestedOverpassTarget	= 0;
	RequiredServiceBranches	= 0;
	RequiredTrails				= 0;
	OptionalCrossLinkQuota	= 0;

	RiverBridges	= 0;
	CanyonBridges	= 0;
	Highways			= 0;
	ReservedOverpassCount	= 0;
	ServiceBranches= 0;
	Trails			= 0;
	CrossLinks		= 0;
	RejectedApproachClass	= 0;
	RejectedWrongSide			= 0;
	RejectedPose				= 0;
	RecoveredByNextRank		= 0;
	RampCandidatesEvaluated	= 0;
	for (int i = 0; i < HYBRID_INTERCHANGE_FORM_COUNT; ++i) InterchangesByForm[i] = 0;
	for (int i = 0; i < HYBRID_RAMP_ROLE_COUNT; ++i) RampsByRole[i] = 0;
	InterchangeWithdrawals	= 0;
	for (int i = 0; i < HYBRID_INTERCHANGE_REJECTION_COUNT; ++i) AnchorRejections[i] = 0;
	for (int i = 0; i < 8; ++i) CandidateRefusals[i] = 0;
	for (int i = 0; i < HYBRID_INTERCHANGE_FORM_COUNT; ++i) AnchorsByForm[i] = 0;
	for (int i = 0; i < 3; ++i) StraightExtentRefusals[i] = 0;
	AnchorTurnDegrees.Delete_All();
	AnchorNeighbourBefore.Delete_All();
	AnchorNeighbourAfter.Delete_All();
	Checksum			= 0;
}


const HybridInterchange * HybridTopology::Find_Interchange(unsigned int stable_id) const
{
	for (int i = 0; i < Interchanges.Count(); ++i) {
		if (Interchanges[i].StableId == stable_id) return &Interchanges[i];
	}
	return nullptr;
}


const HybridTopologyNode * HybridTopology::Find_Node(unsigned int stable_id) const
{
	const int index = (int)stable_id - 1;
	if ((index >= 0) && (index < Nodes.Count()) && (Nodes[index].StableId == stable_id)) {
		return &Nodes[index];
	}
	for (int i = 0; i < Nodes.Count(); ++i) {
		if (Nodes[i].StableId == stable_id) return &Nodes[i];
	}
	return nullptr;
}


const HybridTopologyEdge * HybridTopology::Find_Edge(unsigned int stable_id) const
{
	const int index = (int)stable_id - 1;
	if ((index >= 0) && (index < Edges.Count()) && (Edges[index].StableId == stable_id)) {
		return &Edges[index];
	}
	for (int i = 0; i < Edges.Count(); ++i) {
		if (Edges[i].StableId == stable_id) return &Edges[i];
	}
	return nullptr;
}


const HybridBaseEntrance * HybridTopology::Find_Entrance(int team,int family) const
{
	for (int i = 0; i < Entrances.Count(); ++i) {
		if ((Entrances[i].Team == team) && (Entrances[i].RouteFamily == family)) return &Entrances[i];
	}
	return nullptr;
}


int HybridTopology::Count_Of_Edge_Kind(HybridTopologyEdgeKind kind) const
{
	int count = 0;
	for (int i = 0; i < Edges.Count(); ++i) {
		if (Edges[i].Kind == kind) count++;
	}
	return count;
}


int HybridTopology::Count_Of_Road_Class(HybridRoadClass road_class) const
{
	int count = 0;
	for (int i = 0; i < Edges.Count(); ++i) {
		if (Edges[i].RoadClass == road_class) count++;
	}
	return count;
}


int HybridTopology::Count_Of_Node_Kind(HybridTopologyNodeKind kind) const
{
	int count = 0;
	for (int i = 0; i < Nodes.Count(); ++i) {
		if (Nodes[i].Kind == kind) count++;
	}
	return count;
}


int HybridTopology::Realized_Edge_Count(void) const
{
	int count = 0;
	for (int i = 0; i < Edges.Count(); ++i) {
		if (Edges[i].Is_Realized()) count++;
	}
	return count;
}


bool HybridTopology::Has_Cycle(void) const
{
	/*
	**	A connected component with at least as many realized edges as nodes has a cycle.  Counting is
	**	enough here and needs no traversal order: union-find over the realized edges, then compare each
	**	component's edge count against its node count.
	*/
	const int count = Nodes.Count();
	if (count <= 0) return false;

	DynamicVectorClass<int> parent;
	for (int i = 0; i < count; ++i) parent.Add(i);

	for (int e = 0; e < Edges.Count(); ++e) {

		if (!Edges[e].Is_Realized()) continue;

		int a = -1;
		int b = -1;
		for (int i = 0; i < count; ++i) {
			if (Nodes[i].StableId == Edges[e].NodeA) a = i;
			if (Nodes[i].StableId == Edges[e].NodeB) b = i;
		}
		if ((a < 0) || (b < 0)) continue;

		while (parent[a] != a) a = parent[a];
		while (parent[b] != b) b = parent[b];
		if (a == b) return true;					// the edge closes a loop
		parent[(a < b) ? b : a] = (a < b) ? a : b;
	}
	return false;
}


HybridTopologyParams::HybridTopologyParams(void) :
	RoadDensity(0.65f),FieldRadiusMax(60.0f),MinVehicleLaneClearance(1.0f),HarvesterLength(7.4f),
	MaxRoutedEdges(32),MaxCutDepth(0.0f),MaxFillHeight(0.0f),OverpassClearGap(0.0f),TerrainCellSize(0.0f),
	LightVehicleWidth(0.0f),HeavyVehicleWidth(0.0f),HarvesterWidth(0.0f)
{
	memset(&Constants,0,sizeof(Constants));
	for (int c = 0; c < HYBRID_ROAD_CLASS_COUNT; ++c) {
		BridgeApproachMax[c]	= 0.0f;
		OverpassDepthMax[c]	= 0.0f;
	}
}


/***********************************************************************************************
**	Section 12.2's unit-capacity edge-disjoint max-flow, with BASE_INTERNAL contracted.
***********************************************************************************************/

struct FlowGraph
{
	DynamicVectorClass<int>	Group;		// node index -> contracted super-node index
	DynamicVectorClass<int>	EdgeA;		// super-node endpoints of every realized edge
	DynamicVectorClass<int>	EdgeB;
	DynamicVectorClass<unsigned char>	Used;
	int							NodeCount;
};


static int Flow_Find(DynamicVectorClass<int> & parent,int index)
{
	while (parent[index] != index) index = parent[index];
	return index;
}


static void Flow_Build(const HybridTopology & topology,FlowGraph * out)
{
	const int count = topology.Nodes.Count();
	out->Group.Delete_All();
	out->EdgeA.Delete_All();
	out->EdgeB.Delete_All();
	out->Used.Delete_All();
	out->NodeCount = count;

	for (int i = 0; i < count; ++i) out->Group.Add(i);

	//	BASE_INTERNAL is free connectivity inside one base, so its endpoints contract to one node.
	for (int e = 0; e < topology.Edges.Count(); ++e) {

		if (topology.Edges[e].Kind != HYBRID_EDGE_BASE_INTERNAL) continue;
		int a = -1, b = -1;
		for (int i = 0; i < count; ++i) {
			if (topology.Nodes[i].StableId == topology.Edges[e].NodeA) a = i;
			if (topology.Nodes[i].StableId == topology.Edges[e].NodeB) b = i;
		}
		if ((a < 0) || (b < 0)) continue;
		const int root_a = Flow_Find(out->Group,a);
		const int root_b = Flow_Find(out->Group,b);
		if (root_a == root_b) continue;
		out->Group[(root_a < root_b) ? root_b : root_a] = (root_a < root_b) ? root_a : root_b;
	}

	for (int e = 0; e < topology.Edges.Count(); ++e) {

		if (!topology.Edges[e].Is_Realized()) continue;
		int a = -1, b = -1;
		for (int i = 0; i < count; ++i) {
			if (topology.Nodes[i].StableId == topology.Edges[e].NodeA) a = i;
			if (topology.Nodes[i].StableId == topology.Edges[e].NodeB) b = i;
		}
		if ((a < 0) || (b < 0)) continue;
		const int super_a = Flow_Find(out->Group,a);
		const int super_b = Flow_Find(out->Group,b);
		if (super_a == super_b) continue;
		out->EdgeA.Add(super_a);
		out->EdgeB.Add(super_b);
		out->Used.Add(0);
	}
}


int Hybrid_Topology_Max_Flow(const HybridTopology & topology,unsigned int gdi_base_node,
										unsigned int nod_base_node)
{
	const int count = topology.Nodes.Count();
	if (count <= 0) return 0;

	FlowGraph flow;
	Flow_Build(topology,&flow);

	int source = -1;
	int sink = -1;
	for (int i = 0; i < count; ++i) {
		if (topology.Nodes[i].StableId == gdi_base_node) source = Flow_Find(flow.Group,i);
		if (topology.Nodes[i].StableId == nod_base_node) sink = Flow_Find(flow.Group,i);
	}
	if ((source < 0) || (sink < 0) || (source == sink)) return 0;

	int value = 0;

	for (;;) {

		//	Edmonds-Karp: one BFS per augmentation, edges visited in StableId order, unit capacities.
		DynamicVectorClass<int> from_edge;
		DynamicVectorClass<int> from_node;
		DynamicVectorClass<int> queue;
		for (int i = 0; i < count; ++i) {
			from_edge.Add(-1);
			from_node.Add(-1);
		}

		queue.Add(source);
		from_node[source] = source;
		int head = 0;
		bool reached = false;

		while ((head < queue.Count()) && !reached) {

			const int node = queue[head++];
			for (int e = 0; (e < flow.EdgeA.Count()) && !reached; ++e) {

				if (flow.Used[e] != 0) continue;
				int other = -1;
				if (flow.EdgeA[e] == node)			other = flow.EdgeB[e];
				else if (flow.EdgeB[e] == node)	other = flow.EdgeA[e];
				else										continue;
				if (from_node[other] >= 0) continue;

				from_node[other] = node;
				from_edge[other] = e;
				if (other == sink) {
					reached = true;
					break;
				}
				queue.Add(other);
			}
		}

		if (!reached) break;

		int node = sink;
		while (node != source) {
			const int edge = from_edge[node];
			flow.Used[edge] = 1;
			node = from_node[node];
		}
		value++;
	}
	return value;
}


/***********************************************************************************************
**	The builder.
***********************************************************************************************/

struct TopologyBuilder
{
	const HybridTopologyParams *		P;
	const HybridMapIdentity *			Identity;
	const HybridStrategicGraph *		Graph;
	const HybridLandformSet *			Landforms;
	const HybridHydrologySet *			Hydrology;
	const HybridAnchorSet *				Anchors;
	HybridCoarseMatrices *				Matrices;
	HybridTopologyHeightSampleFunc	Sample;
	void *									Context;
	HybridTopology *						Out;

	int										FamilyCount;
	unsigned int							BaseNode[2];			// the logical base node per team
	unsigned int							NextNodeId;
	unsigned int							NextEdgeId;

	//	Which Phase-6 windows are already consumed: one physical window satisfies one category only.
	DynamicVectorClass<unsigned char>	WindowUsed;

	//	Where each selected window sits along its own obstruction, for the >= 12W spacing rule.
	DynamicVectorClass<unsigned int>		SelectedWindow;
};


static bool Fail(HybridTopologyResult * result,HybridTopologyFailure failure,const char * detail)
{
	if (result != nullptr) {
		result->Failure	= failure;
		result->Detail		= (detail != nullptr) ? detail : "";
	}
	return false;
}


static unsigned int Add_Node(TopologyBuilder & b,HybridTopologyNodeKind kind,const Vector2 & position,
										int family,int team,unsigned int site_id,unsigned int window_id,int sector)
{
	HybridTopologyNode node;
	node.StableId		= b.NextNodeId++;
	node.Kind			= kind;
	node.Position.Set(Hybrid_Quantize_Position(position.X),Hybrid_Quantize_Position(position.Y));
	node.RouteFamily	= family;
	node.Team			= team;
	node.SiteId			= site_id;
	node.WindowId		= window_id;
	node.SectorOrdinal= sector;
	b.Out->Nodes.Add(node);
	return node.StableId;
}


static float Node_Distance(const TopologyBuilder & b,unsigned int node_a,unsigned int node_b)
{
	const HybridTopologyNode * a = b.Out->Find_Node(node_a);
	const HybridTopologyNode * c = b.Out->Find_Node(node_b);
	if ((a == nullptr) || (c == nullptr)) return 0.0f;
	return Hybrid_Quantize_Position((c->Position - a->Position).Length());
}


/*
**	The capability mask each edge role carries, straight from the roles Section 8 Phase 9 creates:
**
**	  - a family trunk is a heavy route family's own chain, and step 11's optional heavy cross-link is named
**	    a heavy link by the plan -- both carry the heavy reference;
**	  - a home road and a contested or remote field branch are the resource economy edges, so they carry the
**	    Harvester;
**	  - a quota-filling service link, a trail, and a cross-link synthesized to fill the river or canyon bridge
**	    quota or the overpass quota exist to meet a count, and the plan gives none of them a heavier role, so
**	    they carry the light reference only.
**
**	Every realized road carries the light reference.  Nothing here reads the road class.
*/
unsigned int Hybrid_Topology_Edge_Capabilities(HybridTopologyEdgeKind kind,bool heavy_link)
{
	switch (kind) {
		case HYBRID_EDGE_BASE_INTERNAL:	return 0;
		case HYBRID_EDGE_FAMILY_TRUNK:	return HYBRID_NAV_LIGHT | HYBRID_NAV_HEAVY;
		case HYBRID_EDGE_HOME_ROAD:
		case HYBRID_EDGE_FIELD_BRANCH:	return HYBRID_NAV_LIGHT | HYBRID_NAV_HARVESTER;
		case HYBRID_EDGE_CROSS_LINK:		return heavy_link ? (HYBRID_NAV_LIGHT | HYBRID_NAV_HEAVY) : HYBRID_NAV_LIGHT;
		default:									break;
	}
	return HYBRID_NAV_LIGHT;
}


float Hybrid_Required_Swept_Width(unsigned int capabilities,float light_width,float heavy_width,float harvester_width)
{
	float swept = 0.0f;
	if (((capabilities & HYBRID_NAV_LIGHT) != 0) && (light_width > swept))			swept = light_width;
	if (((capabilities & HYBRID_NAV_HEAVY) != 0) && (heavy_width > swept))			swept = heavy_width;
	if (((capabilities & HYBRID_NAV_HARVESTER) != 0) && (harvester_width > swept))	swept = harvester_width;
	return swept;
}


float Hybrid_Edge_Roadbed_Width(const HybridTopologyEdge & edge,float w)
{
	const float width = Hybrid_Roadbed_Width(edge.RoadClass,w);
	return (edge.Kind == HYBRID_EDGE_HIGHWAY_RAMP) ? (width + (2.0f * edge.RampRoadbedExtra)) : width;
}


static unsigned int Add_Edge(TopologyBuilder & b,HybridTopologyEdgeKind kind,HybridRoadClass road_class,
										unsigned int node_a,unsigned int node_b,int family,bool required,
										unsigned int obligation,unsigned int source_edge,bool heavy_link = false)
{
	HybridTopologyEdge edge;
	edge.StableId		= b.NextEdgeId++;
	edge.Kind			= kind;
	edge.RoadClass		= road_class;
	edge.Capabilities	= Hybrid_Topology_Edge_Capabilities(kind,heavy_link);
	edge.NodeA			= node_a;
	edge.NodeB			= node_b;
	edge.RouteFamily	= family;
	edge.Required		= required;
	edge.ObligationId	= obligation;
	edge.SourceEdgeId	= source_edge;
	edge.Length			= Node_Distance(b,node_a,node_b);
	b.Out->Edges.Add(edge);
	return edge.StableId;
}


static bool Edges_Adjacent(const TopologyBuilder & b,unsigned int node_a,unsigned int node_b)
{
	for (int e = 0; e < b.Out->Edges.Count(); ++e) {
		const HybridTopologyEdge & edge = b.Out->Edges[e];
		if ((edge.NodeA == node_a) && (edge.NodeB == node_b)) return true;
		if ((edge.NodeA == node_b) && (edge.NodeB == node_a)) return true;
	}
	return false;
}


//	The shared predicates, plus the semantic tests Phase 4's layers answer.
static bool Point_Is_Clear(const TopologyBuilder & b,const Vector2 & world,bool allow_base)
{
	const HybridSemanticMatrix & matrix = b.Matrices->Matrix;
	int cx = 0, cy = 0;
	if (!matrix.World_To_Cell(world,&cx,&cy))												return false;
	if (!matrix.Has_Flag(cx,cy,HYBRID_FLAG_PLACEABLE))									return false;
	if (Hybrid_Point_In_Water(*b.Hydrology,world))										return false;
	if (Hybrid_Point_In_Immutable_Landform(*b.Landforms,world))						return false;
	if (!allow_base) {
		for (int i = 0; i < b.Anchors->Bases.Count(); ++i) {
			const HybridBaseAnchor & base = b.Anchors->Bases[i];
			if ((world - base.Center).Length() <= base.ProtectedRadius) return false;
		}
	}
	return true;
}


static bool Height_At(const TopologyBuilder & b,const Vector2 & world,float * height_out)
{
	if (b.Sample == nullptr) return false;
	return b.Sample(world,height_out,b.Context);
}


//	The steepest absolute slope sampled over a disc, in radians, from the realized heightfield.
static bool Disc_Max_Slope(const TopologyBuilder & b,const Vector2 & centre,float radius,float * radians_out)
{
	const float step = Max2(0.5f * b.P->Constants.A,b.P->Constants.Q);
	float worst = 0.0f;
	bool sampled = false;

	for (float y = centre.Y - radius; y <= (centre.Y + radius); y += step) {
		for (float x = centre.X - radius; x <= (centre.X + radius); x += step) {

			const Vector2 point(x,y);
			if ((point - centre).Length() > radius) continue;

			float here = 0.0f, east = 0.0f, north = 0.0f;
			if (!Height_At(b,point,&here)) continue;
			if (!Height_At(b,Vector2(point.X + step,point.Y),&east)) continue;
			if (!Height_At(b,Vector2(point.X,point.Y + step),&north)) continue;

			const float dx = (east - here) / step;
			const float dy = (north - here) / step;
			worst = Max2(worst,(float)atan((double)sqrt((double)((dx * dx) + (dy * dy)))));
			sampled = true;
		}
	}
	if (radians_out != nullptr) *radians_out = worst;
	return sampled;
}


//	Is the straight run from `from` to `to` within a road class's maximum grade on realized ground?
static bool Run_Within_Grade(const TopologyBuilder & b,const Vector2 & from,const Vector2 & to,
										HybridRoadClass road_class)
{
	const float step = Max2(0.5f * b.P->Constants.A,b.P->Constants.Q);
	const float grade = (float)tan((double)(Hybrid_Road_Class_Spec(road_class).MaxGradeDegrees * PI_OVER_180));

	Vector2 axis = to - from;
	const float length = axis.Length();
	if (length <= 0.0f) return true;
	axis *= (1.0f / length);

	float previous = 0.0f;
	bool have_previous = false;

	for (float along = 0.0f; along <= length; along += step) {

		float height = 0.0f;
		if (!Height_At(b,from + (axis * along),&height)) return false;
		if (have_previous && ((float)fabs((double)(height - previous)) > (grade * step))) return false;
		previous			= height;
		have_previous	= true;
	}
	return true;
}


/***********************************************************************************************
**	Step 1: base entrance assignment.
***********************************************************************************************/

static float Wrapped_Angle_Difference(const Vector2 & a,const Vector2 & b)
{
	const float dot = Clamp2((a.X * b.X) + (a.Y * b.Y),-1.0f,1.0f);
	return (float)acos((double)dot);
}


/*
**	Step 5 of the entrance rules: the corridor capsule must stay inside the Phase-8 viable swept
**	sector.  That is the same ground Phase 8 already swept -- dry, placeable and clear of immutable
**	walls -- measured over the capsule this phase is about to reserve, plus the sector's own angular
**	span so a corridor cannot lean into the neighbouring sector.
*/
static bool Corridor_Fits_Sector(const TopologyBuilder & b,const Vector2 & centre,float protected_radius,
											int ordinal,const HybridBaseEntrance & entrance)
{
	const HybridConstants & k = b.P->Constants;
	const Vector2 direction = Hybrid_Exit_Sector_Direction(b.Identity->Frame,ordinal);
	const Vector2 lateral(-direction.Y,direction.X);
	const float half_span = 0.5f * (360.0f / (float)HYBRID_EXIT_SECTOR_COUNT) * PI_OVER_180;
	const float step = Max2(0.5f * k.A,k.Q);

	Vector2 axis = entrance.CorridorB - entrance.CorridorA;
	const float length = axis.Length();
	if (length <= 0.0f) return false;
	axis *= (1.0f / length);

	for (float along = 0.0f; along <= length; along += step) {
		for (int side = -1; side <= 1; ++side) {

			const Vector2 point = entrance.CorridorA + (axis * along) +
										 (lateral * ((float)side * entrance.CorridorHalfWidth));

			//	Inside the base's own protected disc the ground is the base's, not the sector's.
			const float radius = (point - centre).Length();
			if (radius <= protected_radius) continue;

			if (!Point_Is_Clear(b,point,true)) return false;

			/*
			**	Step 6 reserves the corridor against unrelated Tiberium, so a corridor that already runs into a
			**	frozen Tiberium hard reserve cannot be reserved at all: the family trunk that leaves through it owns
			**	no field, and Phase 10 refuses that ground to it.  Measured, 148 of 512 frozen corridors over 420
			**	worlds overlapped a reserve, and their trunks then failed to leave their own base.
			*/
			for (int r = 0; r < b.Anchors->Resources.Count(); ++r) {
				if ((point - b.Anchors->Resources[r].Center).Length() <= b.Anchors->Resources[r].ReserveRadius) return false;
			}

			Vector2 outward = point - centre;
			if (outward.Length() <= 0.0f) continue;
			outward *= (1.0f / outward.Length());
			if (Wrapped_Angle_Difference(outward,direction) > half_span) return false;
		}
	}
	return true;
}


static void Reserve_Entrance_Corridor(TopologyBuilder & b,const HybridBaseEntrance & entrance)
{
	HybridSemanticMatrix & matrix = b.Matrices->Matrix;

	HybridReservation reservation;
	reservation.StableId	= (unsigned int)(b.Matrices->Reservations.Count() + 1);
	reservation.Priority	= HYBRID_RESERVE_HEAVY_CORRIDOR;
	reservation.A			= entrance.CorridorA;
	reservation.B			= entrance.CorridorB;
	reservation.Radius	= entrance.CorridorHalfWidth;
	reservation.SiteId	= entrance.BaseSiteId;
	reservation.RouteFamily = entrance.RouteFamily;
	reservation.Exclusive= false;
	b.Matrices->Reservations.Add(reservation);

	//	Reserved against unrelated Tiberium and decoration: the decoration stamp is the semantic half.
	for (int cy = 0; cy < matrix.Get_Count_Y(); ++cy) {
		for (int cx = 0; cx < matrix.Get_Count_X(); ++cx) {

			const Vector2 centre = matrix.Cell_Center(cx,cy);
			Vector2 axis = entrance.CorridorB - entrance.CorridorA;
			const float length = axis.Length();
			float t = 0.0f;
			if (length > 0.0f) {
				t = Clamp2((((centre.X - entrance.CorridorA.X) * axis.X) +
								((centre.Y - entrance.CorridorA.Y) * axis.Y)) / (length * length),0.0f,1.0f);
			}
			const Vector2 nearest(entrance.CorridorA.X + (axis.X * t),entrance.CorridorA.Y + (axis.Y * t));
			if ((centre - nearest).Length() > (entrance.CorridorHalfWidth + (0.5f * matrix.Get_Cell_Size()))) continue;

			const int index = matrix.Index(cx,cy);
			matrix.NoDecorationMask[index]	= 1;
			matrix.Flags[index]					|= HYBRID_FLAG_NO_DECORATION;
		}
	}
}


/*
**	Step 1.  One `BASE_ENTRANCE_POINT` per heavy family per base, from the frozen Phase-8 sectors, in
**	ascending family id, with the desired direction taken from that family's own guides.
*/
static bool Assign_Base_Entrances(TopologyBuilder & b,HybridTopologyResult * result)
{
	const HybridConstants & k = b.P->Constants;
	StringClass detail;

	for (int team = 0; team < 2; ++team) {

		const HybridBaseAnchor * base = b.Anchors->Base_Of_Team(team);
		if (base == nullptr) {
			return Fail(result,HYBRID_TOPOLOGY_INVALID_INPUT,"a team has no frozen base anchor");
		}

		b.BaseNode[team] = Add_Node(b,HYBRID_NODE_BASE,base->Center,-1,team,base->SiteId,0,-1);

		for (int family = 0; family < b.FamilyCount; ++family) {

			//	The family's own guides: its Phase-3 waypoints, in StableId order.  GDI leaves toward the
			//	first one, Nod toward the last, which is the plan's own asymmetry.
			const HybridStrategicSite * guide = nullptr;
			for (int s = 0; s < b.Graph->Sites.Count(); ++s) {
				const HybridStrategicSite & site = b.Graph->Sites[s];
				if (site.Role != HYBRID_SITE_ROUTE_WAYPOINT) continue;
				if (site.RouteFamily != family) continue;
				if ((team == 0) && (guide != nullptr)) continue;		// keep the first
				guide = &site;												// ...or fall through to the last
			}

			Vector2 desired = (guide != nullptr) ? (guide->Position - base->Center)
														  : (b.Identity->Frame.AxisU * ((team == 0) ? 1.0f : -1.0f));
			if (desired.Length() <= 0.0f) desired = b.Identity->Frame.AxisU;
			desired *= (1.0f / desired.Length());

			//	Rank the frozen viable sectors by (angular difference, ordinal).
			DynamicVectorClass<int> ranked;
			for (int i = 0; i < base->ViableSectors.Count(); ++i) ranked.Add(base->ViableSectors[i]);
			for (int i = 1; i < ranked.Count(); ++i) {
				for (int j = i; j > 0; --j) {
					const float left = Wrapped_Angle_Difference(Hybrid_Exit_Sector_Direction(b.Identity->Frame,
																												ranked[j - 1]),desired);
					const float right = Wrapped_Angle_Difference(Hybrid_Exit_Sector_Direction(b.Identity->Frame,
																												 ranked[j]),desired);
					bool swap = false;
					if (right < left)											swap = true;
					else if ((right == left) && (ranked[j] < ranked[j - 1]))	swap = true;
					if (!swap) break;
					const int hold = ranked[j - 1];
					ranked[j - 1]	= ranked[j];
					ranked[j]		= hold;
				}
			}

			bool placed = false;

			for (int pass = 0; (pass < 2) && !placed; ++pass) {
				for (int i = 0; (i < ranked.Count()) && !placed; ++i) {

					const int ordinal = ranked[i];

					//	Pass 0 only takes sectors no other family at this base has taken yet.
					bool taken = false;
					for (int e = 0; e < b.Out->Entrances.Count(); ++e) {
						if (b.Out->Entrances[e].BaseSiteId != base->SiteId) continue;
						if (b.Out->Entrances[e].SectorOrdinal == ordinal) taken = true;
					}
					if ((pass == 0) && taken) continue;

					const Vector2 direction = Hybrid_Exit_Sector_Direction(b.Identity->Frame,ordinal);

					HybridBaseEntrance entrance;
					entrance.BaseSiteId			= base->SiteId;
					entrance.Team					= team;
					entrance.RouteFamily			= family;
					entrance.SectorOrdinal		= ordinal;
					entrance.Reused				= taken;
					entrance.Point					= base->Center + (direction * (base->ProtectedRadius +
																							  (ENTRANCE_POINT_W * k.W)));
					entrance.CorridorA			= base->Center + (direction * Max2(0.0f,base->ProtectedRadius -
																							  (ENTRANCE_CORRIDOR_INNER_W * k.W)));
					entrance.CorridorB			= base->Center + (direction * (base->ProtectedRadius +
																							  (ENTRANCE_CORRIDOR_OUTER_W * k.W)));
					entrance.CorridorHalfWidth	= (0.5f * Hybrid_Roadbed_Width(HYBRID_ROAD_PRIMARY_2,k.W)) +
															b.P->MinVehicleLaneClearance;

					if (!Corridor_Fits_Sector(b,base->Center,base->ProtectedRadius,ordinal,entrance)) continue;

					entrance.NodeId = Add_Node(b,HYBRID_NODE_BASE_ENTRANCE,entrance.Point,family,team,
														base->SiteId,0,ordinal);
					b.Out->Entrances.Add(entrance);
					Reserve_Entrance_Corridor(b,entrance);

					//	The logical base node reaches every entrance by a zero-cost BASE_INTERNAL edge.
					Add_Edge(b,HYBRID_EDGE_BASE_INTERNAL,HYBRID_ROAD_CLASS_COUNT,b.BaseNode[team],
								entrance.NodeId,family,false,0,0);
					placed = true;
				}
			}

			if (!placed) {
				detail.Format("base %u family %d has no viable sector whose entrance corridor fits",
									base->SiteId,family);
				return Fail(result,HYBRID_TOPOLOGY_NO_ENTRANCE_SECTOR,detail.Peek_Buffer());
			}
		}
	}
	return true;
}


/***********************************************************************************************
**	Step 2: the family chains, and the crossing obligations bound to Phase-6 windows.
***********************************************************************************************/

//	Every window already selected must be >= 12W away along its own obstruction.
static bool Window_Spacing_Is_Legal(const TopologyBuilder & b,const HybridCrossingWindow & candidate)
{
	const float spacing = WINDOW_SPACING_W * b.P->Constants.W;

	for (int i = 0; i < b.SelectedWindow.Count(); ++i) {

		const HybridCrossingWindow * selected = b.Hydrology->Find_Window(b.SelectedWindow[i]);
		if (selected == nullptr) continue;
		if (selected->ObstructionRecordId != candidate.ObstructionRecordId) continue;
		if (selected->Obstruction != candidate.Obstruction) continue;
		if ((float)fabs((double)(selected->Station - candidate.Station)) < spacing) return false;
	}
	return true;
}


static int Window_Index(const TopologyBuilder & b,unsigned int window_id)
{
	for (int i = 0; i < b.Hydrology->Windows.Count(); ++i) {
		if (b.Hydrology->Windows[i].StableId == window_id) return i;
	}
	return -1;
}


static void Consume_Window(TopologyBuilder & b,const HybridCrossingWindow & window)
{
	const int index = Window_Index(b,window.StableId);
	if ((index >= 0) && (index < b.WindowUsed.Count())) b.WindowUsed[index] = 1;
	b.SelectedWindow.Add(window.StableId);

	if (window.Obstruction == HYBRID_OBSTRUCTION_RIVER)						b.Out->RiverBridges++;
	else if ((window.Obstruction == HYBRID_OBSTRUCTION_CANYON) ||
				(window.Obstruction == HYBRID_OBSTRUCTION_RAVINE))				b.Out->CanyonBridges++;
}


//	Where a waypoint at `position` belongs: the family's trunk edge whose own segment passes nearest it, and the
//	slot in that edge's waypoint list that keeps the list in along-edge order.
static bool Find_Insertion(const TopologyBuilder & b,int family,const Vector2 & position,int * edge_out,int * slot_out)
{
	int best = -1;
	float best_distance = 3.4e38f;

	for (int e = 0; e < b.Out->Edges.Count(); ++e) {

		HybridTopologyEdge & edge = b.Out->Edges[e];
		if (edge.Kind != HYBRID_EDGE_FAMILY_TRUNK) continue;
		if (edge.RouteFamily != family) continue;

		const HybridTopologyNode * a = b.Out->Find_Node(edge.NodeA);
		const HybridTopologyNode * c = b.Out->Find_Node(edge.NodeB);
		if ((a == nullptr) || (c == nullptr)) continue;

		Vector2 axis = c->Position - a->Position;
		const float length = axis.Length();
		float t = 0.0f;
		if (length > 0.0f) {
			t = Clamp2((((position.X - a->Position.X) * axis.X) +
							((position.Y - a->Position.Y) * axis.Y)) / (length * length),0.0f,1.0f);
		}
		const Vector2 nearest(a->Position.X + (axis.X * t),a->Position.Y + (axis.Y * t));
		const float distance = (position - nearest).Length();
		if (distance >= best_distance) continue;
		best_distance	= distance;
		best				= e;
	}
	if (best < 0) return false;

	const HybridTopologyEdge & edge = b.Out->Edges[best];
	const HybridTopologyNode * a = b.Out->Find_Node(edge.NodeA);

	//	Waypoints stay ordered from NodeA to NodeB, which is the order Phase 10 routes the sub-legs in.
	int insert_at = edge.Waypoints.Count();
	if (a != nullptr) {
		const float own = (position - a->Position).Length();
		for (int i = 0; i < edge.Waypoints.Count(); ++i) {
			const HybridTopologyNode * existing = b.Out->Find_Node(edge.Waypoints[i]);
			if (existing == nullptr) continue;
			if ((existing->Position - a->Position).Length() > own) {
				insert_at = i;
				break;
			}
		}
	}
	*edge_out = best;
	*slot_out = insert_at;
	return true;
}


//	Insert a waypoint into the chain edge whose own segment passes nearest it, in along-edge order.
static void Insert_Waypoint(TopologyBuilder & b,int family,unsigned int node_id,const Vector2 & position)
{
	int edge = -1;
	int slot = 0;
	if (!Find_Insertion(b,family,position,&edge,&slot)) return;
	b.Out->Edges[edge].Waypoints.Insert(slot,node_id);
}


/***********************************************************************************************
**	Q-023: structural-pose realizability.
**
**	Phase 9 chooses which physical windows become structural waypoints on which edges.  A structural waypoint
**	is a pose -- a point, an axis the road must run along through it, and the straight it needs either side --
**	and a road of the edge's class has to be able to line up with each one and get from one to the next.
**	Phase 9 does not search terrain; it refuses only what is impossible even on empty ground, with the route
**	owner's own corner construction (Hybrid_Road_Poses_Connect), so that Phase 10 is never handed a sequence
**	whose answer is already known to be no.
***********************************************************************************************/

static const float	FACING_TOLERANCE	= 1.0e-3f;		// the normalized quantum, on a dot product


HybridRoadPose Hybrid_Window_Pose(const HybridCrossingWindow & window,const Vector2 & previous,const Vector2 & next)
{
	HybridRoadPose pose;
	pose.Point = window.Center;

	Vector2 axis = window.ApproachB - window.ApproachA;
	const float length = axis.Length();
	if (length <= 0.0f) return pose;
	axis *= (1.0f / length);

	//	The bank the road arrives on is the one the whole journey reaches first -- Phase 10's own rule.
	const float a_then_b = (window.ApproachA - previous).Length() + (next - window.ApproachB).Length();
	const float b_then_a = (window.ApproachB - previous).Length() + (next - window.ApproachA).Length();
	pose.Axis	= (a_then_b <= b_then_a) ? axis : Vector2(-axis.X,-axis.Y);
	pose.Before	= 0.5f * length;
	pose.After	= 0.5f * length;
	return pose;
}


float Hybrid_Overpass_Half_Deck(const Vector2 & axis,const Vector2 & other_axis,HybridRoadClass road_class,
										 HybridRoadClass other_class,float lane_width)
{
	const float a = axis.Length();
	const float c = other_axis.Length();
	if ((a <= 0.0f) || (c <= 0.0f)) return 0.0f;
	const float sine = (float)fabs((double)(((axis.X * other_axis.Y) - (axis.Y * other_axis.X)) / (a * c)));
	const float cosine = (float)fabs((double)(((axis.X * other_axis.X) + (axis.Y * other_axis.Y)) / (a * c)));
	if (sine <= 1.0e-3f) return 0.0f;
	const float width = Max2(Hybrid_Roadbed_Width(road_class,lane_width),Hybrid_Roadbed_Width(other_class,lane_width));
	return 0.5f * ((width / sine) + ((width * cosine) / sine));
}


HybridRoadPose Hybrid_Overpass_Pose(const Vector2 & point,const Vector2 & previous,const Vector2 & next,
												const HybridConstants & constants,float half_deck)
{
	HybridRoadPose pose;
	pose.Point = point;
	Vector2 axis = next - previous;
	const float length = axis.Length();
	if (length <= 0.0f) return pose;
	pose.Axis	= axis * (1.0f / length);
	pose.Before	= Max2(OVERPASS_ARM_W * constants.W,half_deck);
	pose.After	= pose.Before;
	return pose;
}


bool Hybrid_Pose_Faces_Intent(const Vector2 & previous,const HybridRoadPose & pose,const Vector2 & next)
{
	if (!pose.Is_Structural()) return true;

	Vector2 axis = pose.Axis;
	axis *= (1.0f / axis.Length());

	Vector2 incoming = pose.Point - previous;
	if (incoming.Length() > 0.0f) {
		incoming *= (1.0f / incoming.Length());
		if (((incoming.X * axis.X) + (incoming.Y * axis.Y)) < -FACING_TOLERANCE) return false;
	}
	Vector2 outgoing = next - pose.Point;
	if (outgoing.Length() > 0.0f) {
		outgoing *= (1.0f / outgoing.Length());
		if (((outgoing.X * axis.X) + (outgoing.Y * axis.Y)) < -FACING_TOLERANCE) return false;
	}
	return true;
}


bool Hybrid_Window_Bindable(const HybridCrossingWindow & window,const HybridRoadPose & previous,
									 const HybridRoadPose & next,HybridRoadClass road_class,const HybridConstants & constants,
									 int * refusal_out)
{
	if (refusal_out != nullptr) *refusal_out = HYBRID_BINDING_OK;

	if (!window.Approach_Planned_For(road_class)) {
		if (refusal_out != nullptr) *refusal_out = HYBRID_BINDING_APPROACH_CLASS;
		return false;
	}

	const HybridRoadPose pose = Hybrid_Window_Pose(window,previous.Point,next.Point);
	if (!Hybrid_Pose_Faces_Intent(previous.Point,pose,next.Point)) {
		if (refusal_out != nullptr) *refusal_out = HYBRID_BINDING_WRONG_SIDE;
		return false;
	}

	if (!Hybrid_Road_Poses_Connect(previous,pose,road_class,constants) ||
		 !Hybrid_Road_Poses_Connect(pose,next,road_class,constants)) {
		if (refusal_out != nullptr) *refusal_out = HYBRID_BINDING_POSE;
		return false;
	}
	return true;
}


int Hybrid_First_Bindable_Window(const HybridCrossingWindow * const * ranked,int count,
											const HybridRoadPose & previous,const HybridRoadPose & next,
											HybridRoadClass road_class,const HybridConstants & constants)
{
	for (int i = 0; i < count; ++i) {
		if (ranked[i] == nullptr) continue;
		if (Hybrid_Window_Bindable(*ranked[i],previous,next,road_class,constants,nullptr)) return i;
	}
	return -1;
}


/*
**	One node of an edge's chain as the realizability test sees it -- a real node, or a candidate that does not
**	exist yet (`NodeId` 0).
*/
struct ChainEntry
{
	ChainEntry(void) : NodeId(0),Kind(HYBRID_NODE_FAMILY_WAYPOINT),WindowId(0),Position(0.0f,0.0f),HalfDeck(0.0f) { }

	//	DynamicVectorClass<T> requires == and !=.
	bool	operator == (const ChainEntry & that) const
	{
		return (NodeId == that.NodeId) && (Kind == that.Kind) && (WindowId == that.WindowId) &&
				 (Position == that.Position);
	}
	bool	operator != (const ChainEntry & that) const	{ return !(*this == that); }

	unsigned int				NodeId;
	HybridTopologyNodeKind	Kind;
	unsigned int				WindowId;
	Vector2						Position;
	float							HalfDeck;		// overpass only: the deck straight on this road, 0 = not yet known
};


static ChainEntry Chain_Entry_Of(const TopologyBuilder & b,unsigned int node_id)
{
	ChainEntry entry;
	const HybridTopologyNode * node = b.Out->Find_Node(node_id);
	if (node == nullptr) return entry;
	entry.NodeId	= node->StableId;
	entry.Kind		= node->Kind;
	entry.WindowId	= node->WindowId;
	entry.Position	= node->Position;
	return entry;
}


//	The intent direction of `edge` through its waypoint `node_id`: from the node before it to the node after it.
static Vector2 Edge_Axis_Through(const TopologyBuilder & b,const HybridTopologyEdge & edge,unsigned int node_id)
{
	for (int w = 0; w < edge.Waypoints.Count(); ++w) {
		if (edge.Waypoints[w] != node_id) continue;
		const unsigned int before = (w > 0) ? edge.Waypoints[w - 1] : edge.NodeA;
		const unsigned int after = ((w + 1) < edge.Waypoints.Count()) ? edge.Waypoints[w + 1] : edge.NodeB;
		const HybridTopologyNode * a = b.Out->Find_Node(before);
		const HybridTopologyNode * c = b.Out->Find_Node(after);
		if ((a == nullptr) || (c == nullptr)) break;
		return c->Position - a->Position;
	}
	return Vector2(0.0f,0.0f);
}


//	NodeA, its waypoints in order, NodeB -- the node chain Phase 10 routes the edge through.
static void Edge_Chain(const TopologyBuilder & b,const HybridTopologyEdge & edge,DynamicVectorClass<ChainEntry> * out)
{
	out->Delete_All();
	out->Add(Chain_Entry_Of(b,edge.NodeA));
	for (int w = 0; w < edge.Waypoints.Count(); ++w) {
		ChainEntry entry = Chain_Entry_Of(b,edge.Waypoints[w]);
		if (entry.Kind == HYBRID_NODE_OVERPASS_OBLIGATION) {
			//	The deck this road keeps straight: measured against the other road through the same point.
			const Vector2 own = Edge_Axis_Through(b,edge,entry.NodeId);
			for (int e = 0; e < b.Out->Edges.Count(); ++e) {
				const HybridTopologyEdge & other = b.Out->Edges[e];
				if (other.StableId == edge.StableId) continue;
				const Vector2 other_axis = Edge_Axis_Through(b,other,entry.NodeId);
				if (other_axis.Length() <= 0.0f) continue;
				entry.HalfDeck = Hybrid_Overpass_Half_Deck(own,other_axis,edge.RoadClass,other.RoadClass,
																		 b.P->Constants.W);
				break;
			}
		}
		out->Add(entry);
	}
	out->Add(Chain_Entry_Of(b,edge.NodeB));
}


//	The pose chain[i] presents to a road along this chain.  The two ends are free.
static HybridRoadPose Chain_Pose(const TopologyBuilder & b,const DynamicVectorClass<ChainEntry> & chain,int i)
{
	HybridRoadPose pose;
	pose.Point = chain[i].Position;
	if ((i <= 0) || (i >= (chain.Count() - 1))) return pose;

	const Vector2 & previous = chain[i - 1].Position;
	const Vector2 & next = chain[i + 1].Position;
	if ((chain[i].Kind == HYBRID_NODE_CROSSING_OBLIGATION) && (chain[i].WindowId != 0)) {
		const HybridCrossingWindow * window = b.Hydrology->Find_Window(chain[i].WindowId);
		if (window != nullptr) return Hybrid_Window_Pose(*window,previous,next);
	}
	if (chain[i].Kind == HYBRID_NODE_OVERPASS_OBLIGATION) {
		return Hybrid_Overpass_Pose(chain[i].Position,previous,next,b.P->Constants,chain[i].HalfDeck);
	}
	return pose;
}


//	Where the road meets a pose from one side: a structure's near end (`entry`) or far end, a free pose's point.
static Vector2 Pose_Far_End(const HybridRoadPose & pose,bool entry)
{
	if (!pose.Is_Structural()) return pose.Point;
	Vector2 axis = pose.Axis;
	axis *= (1.0f / axis.Length());
	return entry ? (pose.Point - (axis * pose.Before)) : (pose.Point + (axis * pose.After));
}


/*
**	The realizability checks around chain[lo..hi] for `road_class`: the class approach capability and the
**	wrong-side test of every structural entry in it, and the connector of every consecutive pair that touches
**	it -- one further out on each side, because a neighbour's own pose turns with its neighbours.
*/
static int Chain_Refusal(const TopologyBuilder & b,const DynamicVectorClass<ChainEntry> & chain,int lo,int hi,
								 HybridRoadClass road_class)
{
	const int last = chain.Count() - 1;
	if (last < 1) return HYBRID_BINDING_OK;

	for (int i = ((lo > 1) ? lo : 1); i <= ((hi < (last - 1)) ? hi : (last - 1)); ++i) {

		const HybridRoadPose pose = Chain_Pose(b,chain,i);
		if (!pose.Is_Structural()) continue;
		if ((chain[i].Kind == HYBRID_NODE_CROSSING_OBLIGATION) && (chain[i].WindowId != 0)) {
			const HybridCrossingWindow * window = b.Hydrology->Find_Window(chain[i].WindowId);
			if ((window != nullptr) && !window->Approach_Planned_For(road_class)) return HYBRID_BINDING_APPROACH_CLASS;
		}
		if (!Hybrid_Pose_Faces_Intent(chain[i - 1].Position,pose,chain[i + 1].Position)) return HYBRID_BINDING_WRONG_SIDE;
	}

	for (int i = (((lo - 1) > 0) ? (lo - 1) : 0); i <= ((hi < (last - 1)) ? hi : (last - 1)); ++i) {
		const HybridRoadPose from = Chain_Pose(b,chain,i);
		const HybridRoadPose to = Chain_Pose(b,chain,i + 1);
		if (!from.Is_Structural() && !to.Is_Structural()) continue;

		//	An ordinary waypoint in the middle of the edge is a guide corner, turned toward its other neighbour.
		Vector2 behind(0.0f,0.0f);
		Vector2 ahead(0.0f,0.0f);
		const bool from_interior = !from.Is_Structural() && (i > 0);
		const bool to_interior = !to.Is_Structural() && ((i + 1) < last);
		if (from_interior) behind = Pose_Far_End(Chain_Pose(b,chain,i - 1),false);
		if (to_interior) ahead = Pose_Far_End(Chain_Pose(b,chain,i + 2),true);
		if (!Hybrid_Road_Poses_Connect(from,to,road_class,b.P->Constants,from_interior ? &behind : nullptr,
												 to_interior ? &ahead : nullptr)) return HYBRID_BINDING_POSE;
	}
	return HYBRID_BINDING_OK;
}


//	Counts one refusal in the Q-023 census.
static void Note_Binding_Refusal(TopologyBuilder & b,int refusal)
{
	if (refusal == HYBRID_BINDING_APPROACH_CLASS)	b.Out->RejectedApproachClass++;
	else if (refusal == HYBRID_BINDING_WRONG_SIDE)	b.Out->RejectedWrongSide++;
	else if (refusal == HYBRID_BINDING_POSE)			b.Out->RejectedPose++;
}


/*
**	The class a structural binding on a heavy family is judged for.  The bindings are made before the highway
**	assignment, so a family that may still become `HIGHWAY_4` is held to it: a window only `PRIMARY_2` can line
**	up with would otherwise be bound now and found impossible once the family is upgraded.  With no highway in
**	the size's quota no family can become one.
*/
static HybridRoadClass Heavy_Binding_Class(const TopologyBuilder & b)
{
	return (b.Out->RequiredHighways > 0) ? HYBRID_ROAD_HIGHWAY_4 : HYBRID_ROAD_PRIMARY_2;
}


/*
**	Would binding `window` into `family`'s chain, where Insert_Waypoint would put it, leave a sequence a road of
**	`road_class` can drive?  The local checks around the new waypoint only.
*/
static int Family_Binding_Refusal(const TopologyBuilder & b,int family,const HybridCrossingWindow & window,
											 HybridRoadClass road_class)
{
	int edge = -1;
	int slot = 0;
	if (!Find_Insertion(b,family,window.Center,&edge,&slot)) return HYBRID_BINDING_POSE;

	DynamicVectorClass<ChainEntry> chain;
	Edge_Chain(b,b.Out->Edges[edge],&chain);
	ChainEntry candidate;
	candidate.Kind		= HYBRID_NODE_CROSSING_OBLIGATION;
	candidate.WindowId	= window.StableId;
	candidate.Position	= window.Center;
	const int at = slot + 1;
	chain.Insert(at,candidate);
	return Chain_Refusal(b,chain,at - 1,at + 1,road_class);
}


//	Does one window outrank another for an obligation?  Preferred beats plain, then nearer the guide.
static bool Outranks(bool preferred,float distance,bool held_preferred,float held_distance)
{
	if (held_preferred && !preferred) return false;
	if ((held_preferred == preferred) && (distance >= held_distance)) return false;
	return true;
}


/*
**	Step 2.  The chain per family -- GDI entrance, the family's Phase-3 waypoints in order, the Nod
**	entrance -- and then each Phase-3 crossing obligation bound to one qualifying Phase-6 window.
*/
static bool Build_Family_Chains(TopologyBuilder & b,HybridTopologyResult * result)
{
	const HybridConstants & k = b.P->Constants;
	const float corridor_half = Hybrid_Max_Corridor_Half_Width(HYBRID_ROAD_PRIMARY_2,k.S,k.D);
	StringClass detail;

	for (int family = 0; family < b.FamilyCount; ++family) {

		const HybridBaseEntrance * gdi = b.Out->Find_Entrance(0,family);
		const HybridBaseEntrance * nod = b.Out->Find_Entrance(1,family);
		if ((gdi == nullptr) || (nod == nullptr)) {
			return Fail(result,HYBRID_TOPOLOGY_NO_ENTRANCE_SECTOR,"a family has no pair of entrances");
		}

		unsigned int previous = gdi->NodeId;

		for (int s = 0; s < b.Graph->Sites.Count(); ++s) {

			const HybridStrategicSite & site = b.Graph->Sites[s];
			if (site.Role != HYBRID_SITE_ROUTE_WAYPOINT) continue;
			if (site.RouteFamily != family) continue;

			const unsigned int node = Add_Node(b,HYBRID_NODE_FAMILY_WAYPOINT,site.Position,family,-1,
														 site.StableId,0,-1);
			Add_Edge(b,HYBRID_EDGE_FAMILY_TRUNK,HYBRID_ROAD_PRIMARY_2,previous,node,family,true,0,0);
			previous = node;
		}

		Add_Edge(b,HYBRID_EDGE_FAMILY_TRUNK,HYBRID_ROAD_PRIMARY_2,previous,nod->NodeId,family,true,0,0);
	}

	//	The obligations, in (family, StableId) order, each bound to one window it may keep.
	for (int o = 0; o < b.Graph->Obligations.Count(); ++o) {

		const HybridCrossingObligation & obligation = b.Graph->Obligations[o];
		const Vector2 guide = b.Identity->Frame.To_World(obligation.Guide);

		int chosen = -1;
		float chosen_distance = 0.0f;
		int ranked_first = -1;					//	the best window before Q-023's realizability is asked
		float ranked_distance = 0.0f;

		//	Why each window was passed over, so an unbound obligation says what stood in the way.
		int refused_used = 0;
		int refused_obstruction = 0;
		int refused_method = 0;
		int refused_distance = 0;
		int refused_spacing = 0;
		int refused_pose = 0;
		float nearest_refused = 3.4e38f;
		const HybridRoadClass binding_class = Heavy_Binding_Class(b);

		for (int w = 0; w < b.Hydrology->Windows.Count(); ++w) {

			if (b.WindowUsed[w] != 0) { refused_used++; continue; }
			const HybridCrossingWindow & window = b.Hydrology->Windows[w];

			//	The obstruction the obligation is about, and the method it requires.
			if ((obligation.Obstruction != HYBRID_OBSTRUCTION_NONE) &&
				 (obligation.Obstruction != window.Obstruction)) { refused_obstruction++; continue; }

			if (obligation.RequiredMethod == HYBRID_CROSSING_BRIDGE) {
				if (!window.Has_Method(HYBRID_WINDOW_METHOD_BRIDGE)) { refused_method++; continue; }
			} else if (obligation.RequiredMethod == HYBRID_CROSSING_PASS) {
				if (!window.Has_Method(HYBRID_WINDOW_METHOD_PASS)) { refused_method++; continue; }
			}

			const float distance = (window.Center - guide).Length();
			if (distance > corridor_half) {
				refused_distance++;
				if (distance < nearest_refused) nearest_refused = distance;
				continue;
			}
			if (!Window_Spacing_Is_Legal(b,window)) { refused_spacing++; continue; }

			//	Preferred beats plain, then nearer the guide, then the lower StableId.
			if ((ranked_first < 0) ||
				 Outranks(window.Preferred,distance,b.Hydrology->Windows[ranked_first].Preferred,ranked_distance)) {
				ranked_first		= w;
				ranked_distance	= distance;
			}

			//	Q-023: a window no road of this family's class can drive through, here, is never ranked at all.
			const int refusal = Family_Binding_Refusal(b,obligation.RouteFamily,window,binding_class);
			if (refusal != HYBRID_BINDING_OK) {
				Note_Binding_Refusal(b,refusal);
				refused_pose++;
				continue;
			}

			if ((chosen >= 0) &&
				 !Outranks(window.Preferred,distance,b.Hydrology->Windows[chosen].Preferred,chosen_distance)) continue;
			chosen				= w;
			chosen_distance	= distance;
		}
		if ((chosen >= 0) && (ranked_first >= 0) && (chosen != ranked_first)) b.Out->RecoveredByNextRank++;

		if (chosen < 0) {
			detail.Format("obligation %u (family %d, obstruction %s, method %d) has no qualifying window "
								"among %d: %d already used, %d wrong obstruction, %d without the method, %d past the "
								"%.0f corridor half-width (nearest %.0f), %d too close to another, %d no %s road can drive",
								obligation.StableId,obligation.RouteFamily,
								Hybrid_Obstruction_Kind_Name(obligation.Obstruction),
								(int)obligation.RequiredMethod,b.Hydrology->Windows.Count(),refused_used,
								refused_obstruction,refused_method,refused_distance,corridor_half,
								(nearest_refused < 3.4e38f) ? nearest_refused : -1.0f,refused_spacing,refused_pose,
								Hybrid_Road_Class_Name(binding_class));
			return Fail(result,HYBRID_TOPOLOGY_OBLIGATION_UNBOUND,detail.Peek_Buffer());
		}

		const HybridCrossingWindow & window = b.Hydrology->Windows[chosen];
		const unsigned int node = Add_Node(b,HYBRID_NODE_CROSSING_OBLIGATION,window.Center,
													  obligation.RouteFamily,-1,obligation.WaypointStableId,
													  window.StableId,-1);
		Insert_Waypoint(b,obligation.RouteFamily,node,window.Center);
		Consume_Window(b,window);
	}
	return true;
}


/***********************************************************************************************
**	Steps 3 and 4: the two Section-5 bridge quotas, filled from unused Phase-6 windows.
***********************************************************************************************/

/*
**	How far a point is from one family's own chain, measured on its trunk segments rather than on its
**	waypoints.  A chain segment is what the road will actually follow, so this is the distance that
**	decides whether binding a crossing to this family would drag its route off its corridor.
*/
static float Distance_To_Family_Chain(const TopologyBuilder & b,int family,const Vector2 & point)
{
	float best = 3.4e38f;

	for (int e = 0; e < b.Out->Edges.Count(); ++e) {

		const HybridTopologyEdge & edge = b.Out->Edges[e];
		if (edge.Kind != HYBRID_EDGE_FAMILY_TRUNK) continue;
		if (edge.RouteFamily != family) continue;

		const HybridTopologyNode * a = b.Out->Find_Node(edge.NodeA);
		const HybridTopologyNode * c = b.Out->Find_Node(edge.NodeB);
		if ((a == nullptr) || (c == nullptr)) continue;

		Vector2 axis = c->Position - a->Position;
		const float length = axis.Length();
		float t = 0.0f;
		if (length > 0.0f) {
			t = Clamp2((((point.X - a->Position.X) * axis.X) + ((point.Y - a->Position.Y) * axis.Y)) /
							(length * length),0.0f,1.0f);
		}
		const Vector2 nearest(a->Position.X + (axis.X * t),a->Position.Y + (axis.Y * t));
		best = Min2(best,(point - nearest).Length());
	}
	return best;
}


static bool Window_Is_Bridge_Category(const HybridCrossingWindow & window,bool river)
{
	if (river) return window.Obstruction == HYBRID_OBSTRUCTION_RIVER;
	return (window.Obstruction == HYBRID_OBSTRUCTION_CANYON) || (window.Obstruction == HYBRID_OBSTRUCTION_RAVINE);
}


//	The family whose chain passes nearest a point, and how far its nearest waypoint is.
static int Nearest_Family(const TopologyBuilder & b,const Vector2 & position,float * distance_out)
{
	int family = -1;
	float best = 3.4e38f;

	for (int n = 0; n < b.Out->Nodes.Count(); ++n) {

		const HybridTopologyNode & node = b.Out->Nodes[n];
		if ((node.Kind != HYBRID_NODE_FAMILY_WAYPOINT) && (node.Kind != HYBRID_NODE_BASE_ENTRANCE)) continue;
		if (node.RouteFamily < 0) continue;

		const float distance = (node.Position - position).Length();
		if (distance >= best) continue;
		best		= distance;
		family	= node.RouteFamily;
	}
	if (distance_out != nullptr) *distance_out = best;
	return family;
}


/*
**	The synthesized fallback of steps 3 and 4: one `SECONDARY_2` cross-link between the nearest heavy
**	family waypoints on opposite banks, through the window.  "Opposite banks" is the sign of each
**	candidate's offset across the window's own tangent, which is the only bank definition that does not
**	need the obstruction's polygon.
*/
static bool Bank_Nodes(const TopologyBuilder & b,const HybridCrossingWindow & window,int * positive_out,
							  int * negative_out)
{
	/*
	**	The two banks are the window's own approach sides: `ApproachA` and `ApproachB` are by construction
	**	the two ends of the reserved straight crossing, so the sign of a node's offset along that axis is
	**	which bank it stands on.  Measuring the banks from the crossing the plan already froze is steadier
	**	than measuring them from the obstruction's local tangent, which says nothing at a distance.
	*/
	Vector2 normal = window.ApproachA - window.ApproachB;
	if (normal.Length() <= 0.0f) normal.Set(-window.Tangent.Y,window.Tangent.X);
	normal *= (1.0f / normal.Length());

	int near_positive = -1;
	int near_negative = -1;
	float best_positive = 3.4e38f;
	float best_negative = 3.4e38f;

	for (int n = 0; n < b.Out->Nodes.Count(); ++n) {

		/*
		**	A heavy family's own nodes are its waypoints *and* its two base entrances: an entrance is the
		**	family's endpoint on that side of the map, and on a two-waypoint family it is often the only
		**	node on one bank at all.
		*/
		const HybridTopologyNode & node = b.Out->Nodes[n];
		if ((node.Kind != HYBRID_NODE_FAMILY_WAYPOINT) && (node.Kind != HYBRID_NODE_BASE_ENTRANCE)) continue;
		if (node.RouteFamily < 0) continue;

		const Vector2 offset = node.Position - window.Center;
		const float side = (offset.X * normal.X) + (offset.Y * normal.Y);
		const float distance = offset.Length();

		if ((side > 0.0f) && (distance < best_positive)) {
			best_positive	= distance;
			near_positive	= n;
		}
		if ((side < 0.0f) && (distance < best_negative)) {
			best_negative	= distance;
			near_negative	= n;
		}
	}

	if ((near_positive < 0) || (near_negative < 0)) return false;
	*positive_out = near_positive;
	*negative_out = near_negative;
	return true;
}


//	Could the bank cross-link through `window` be driven by its fixed `SECONDARY_2` class?
static int Bank_Cross_Link_Refusal(const TopologyBuilder & b,const HybridCrossingWindow & window,int positive,
											  int negative)
{
	DynamicVectorClass<ChainEntry> chain;
	chain.Add(Chain_Entry_Of(b,b.Out->Nodes[positive].StableId));
	ChainEntry candidate;
	candidate.Kind		= HYBRID_NODE_CROSSING_OBLIGATION;
	candidate.WindowId	= window.StableId;
	candidate.Position	= window.Center;
	chain.Add(candidate);
	chain.Add(Chain_Entry_Of(b,b.Out->Nodes[negative].StableId));
	return Chain_Refusal(b,chain,1,1,HYBRID_ROAD_SECONDARY_2);
}


static void Add_Bank_Cross_Link(TopologyBuilder & b,const HybridCrossingWindow & window,int near_positive,
										  int near_negative)
{
	const unsigned int node = Add_Node(b,HYBRID_NODE_CROSSING_OBLIGATION,window.Center,-1,-1,0,
													window.StableId,-1);
	const unsigned int edge = Add_Edge(b,HYBRID_EDGE_CROSS_LINK,HYBRID_ROAD_SECONDARY_2,
													b.Out->Nodes[near_positive].StableId,
													b.Out->Nodes[near_negative].StableId,-1,true,0,0);

	for (int e = 0; e < b.Out->Edges.Count(); ++e) {
		if (b.Out->Edges[e].StableId != edge) continue;
		b.Out->Edges[e].Waypoints.Add(node);
		break;
	}
}


/*
**	How a quota window would be bound, decided before it is ranked (Q-023): to its nearest family when it is a
**	preferred window there, otherwise by the plan's bank cross-link, otherwise to the nearest family when the
**	window is inside that family's corridor -- each only when the resulting structural sequence can be driven.
*/
enum BridgeBindingKind
{
	BRIDGE_BINDING_NONE = 0,
	BRIDGE_BINDING_FAMILY,
	BRIDGE_BINDING_CROSS_LINK,
};

//	One binding refusal in a quota census, on the path that refused it.
static void Note_Quota_Refusal(HybridQuotaCensus * census,HybridQuotaPath path,int binding_refusal)
{
	HybridQuotaRefusal refusal = HYBRID_QUOTA_REFUSE_OTHER;
	if (binding_refusal == HYBRID_BINDING_APPROACH_CLASS)	refusal = HYBRID_QUOTA_REFUSE_APPROACH_CLASS;
	if (binding_refusal == HYBRID_BINDING_WRONG_SIDE)		refusal = HYBRID_QUOTA_REFUSE_WRONG_SIDE;
	if (binding_refusal == HYBRID_BINDING_POSE)				refusal = HYBRID_QUOTA_REFUSE_POSE;
	census->Refused[path][refusal]++;
}


static float Distance_To_Segment_Point(const Vector2 & point,const Vector2 & a,const Vector2 & c)
{
	const Vector2 axis = c - a;
	const float length2 = (axis.X * axis.X) + (axis.Y * axis.Y);
	float t = 0.0f;
	if (length2 > 0.0f) t = Clamp2((((point.X - a.X) * axis.X) + ((point.Y - a.Y) * axis.Y)) / length2,0.0f,1.0f);
	return (point - Vector2(a.X + (axis.X * t),a.Y + (axis.Y * t))).Length();
}


//	The distance from a point to a family's nearest own node: its waypoints and its two base entrances.
static float Family_Node_Distance(const TopologyBuilder & b,int family,const Vector2 & position)
{
	float best = 3.4e38f;
	for (int n = 0; n < b.Out->Nodes.Count(); ++n) {
		const HybridTopologyNode & node = b.Out->Nodes[n];
		if ((node.Kind != HYBRID_NODE_FAMILY_WAYPOINT) && (node.Kind != HYBRID_NODE_BASE_ENTRANCE)) continue;
		if (node.RouteFamily != family) continue;
		best = Min2(best,(node.Position - position).Length());
	}
	return best;
}


/*
**	The families a preferred window belongs to: every family whose required heavy corridor (Phase 4's
**	`HEAVY_CORRIDOR` reservations, which is how Phase 6 marked the window preferred) contains it.  A window
**	preferred only for its nearness to a crossing guide belongs to the family whose node is nearest.
*/
static int Preferred_Families(const TopologyBuilder & b,const HybridCrossingWindow & window,int * families,int capacity)
{
	int count = 0;
	for (int r = 0; r < b.Matrices->Reservations.Count(); ++r) {
		const HybridReservation & reservation = b.Matrices->Reservations[r];
		if (reservation.Priority != HYBRID_RESERVE_HEAVY_CORRIDOR) continue;
		if ((reservation.RouteFamily < 0) || (reservation.RouteFamily >= b.FamilyCount)) continue;
		if (Distance_To_Segment_Point(window.Center,reservation.A,reservation.B) > reservation.Radius) continue;
		bool seen = false;
		for (int i = 0; (i < count) && !seen; ++i) seen = (families[i] == reservation.RouteFamily);
		if (seen || (count >= capacity)) continue;
		//	Ascending family id: the plan's ranking key begins with it.
		int slot = count++;
		while ((slot > 0) && (families[slot - 1] > reservation.RouteFamily)) {
			families[slot] = families[slot - 1];
			slot--;
		}
		families[slot] = reservation.RouteFamily;
	}
	if (count == 0) {
		float distance = 0.0f;
		const int nearest = Nearest_Family(b,window.Center,&distance);
		if (nearest >= 0) families[count++] = nearest;
	}
	return count;
}


//	One family binding of a quota window, with the census of why it was refused.
static bool Quota_Family_Binding(TopologyBuilder & b,const HybridCrossingWindow & window,int family,
											HybridQuotaPath path,HybridQuotaCensus * census)
{
	int edge = -1;
	int slot = 0;
	if (!Find_Insertion(b,family,window.Center,&edge,&slot)) {
		census->Refused[path][HYBRID_QUOTA_REFUSE_OTHER]++;
		return false;
	}
	const int refusal = Family_Binding_Refusal(b,family,window,Heavy_Binding_Class(b));
	if (refusal == HYBRID_BINDING_OK) return true;
	Note_Binding_Refusal(b,refusal);
	Note_Quota_Refusal(census,path,refusal);
	return false;
}


/*
**	One candidate binding for a bridge quota: a window, and the family it goes into or the bank cross-link it
**	gets.  Steps 3 and 4 rank preferred windows first by `(family_id, distance from that family's nearest
**	guide, StableId)` and insert each into *that* family -- the family whose corridor made it preferred, not
**	whichever family happens to own the nearest node -- and then non-preferred windows by lowest `StableId`
**	with the plan's bank cross-link.
*/
struct QuotaCandidate
{
	QuotaCandidate(void) : Window(-1),Family(-1),Preferred(false),Distance(0.0f),StableId(0),
		Binding(BRIDGE_BINDING_NONE),Positive(-1),Negative(-1) { }

	int					Window;
	int					Family;
	bool					Preferred;
	float					Distance;
	unsigned int		StableId;
	BridgeBindingKind	Binding;
	int					Positive;
	int					Negative;

	//	DynamicVectorClass<T> requires == and !=.
	bool	operator == (const QuotaCandidate & that) const
	{
		return (Window == that.Window) && (Family == that.Family) && (Binding == that.Binding);
	}
	bool	operator != (const QuotaCandidate & that) const	{ return !(*this == that); }
};


//	Does `a` outrank `b` in the plan's quota order?
static bool Quota_Outranks(const QuotaCandidate & a,const QuotaCandidate & c)
{
	if (a.Preferred != c.Preferred) return a.Preferred;
	if (a.Preferred) {
		if (a.Family != c.Family) return a.Family < c.Family;
		if (a.Distance != c.Distance) return a.Distance < c.Distance;
		return a.StableId < c.StableId;
	}
	//	Non-preferred: the plan's lowest StableId; the bank cross-link the plan names before the family fallback.
	if (a.StableId != c.StableId) return a.StableId < c.StableId;
	if (a.Binding != c.Binding) return a.Binding == BRIDGE_BINDING_CROSS_LINK;
	return a.Family < c.Family;
}


/*
**	Every realizable binding of one quota window, offered to `consider`.  A preferred window is tried in each
**	family whose corridor contains it.  A non-preferred window gets the plan's bank cross-link; when that cannot
**	be built -- no heavy node on the far bank, or a sequence no `SECONDARY_2` can drive -- it is bound into any
**	family whose own chain passes within that family's `MaxCorridorHalfWidth(PRIMARY_2)`, which the plan's
**	preferred procedure already does for windows on a corridor (D-011).  Returns the number offered.
*/
static int Quota_Bindings(TopologyBuilder & b,int w,HybridQuotaCensus * census,DynamicVectorClass<QuotaCandidate> * out)
{
	const HybridCrossingWindow & window = b.Hydrology->Windows[w];
	int offered = 0;

	QuotaCandidate base;
	base.Window		= w;
	base.Preferred	= window.Preferred;
	base.StableId	= window.StableId;

	if (window.Preferred) {
		int families[8];
		const int count = Preferred_Families(b,window,families,8);
		for (int i = 0; i < count; ++i) {
			if (!Quota_Family_Binding(b,window,families[i],HYBRID_QUOTA_PATH_FAMILY,census)) continue;
			QuotaCandidate candidate = base;
			candidate.Family		= families[i];
			candidate.Distance	= Family_Node_Distance(b,families[i],window.Center);
			candidate.Binding		= BRIDGE_BINDING_FAMILY;
			out->Add(candidate);
			offered++;
		}
		return offered;
	}

	int positive = -1;
	int negative = -1;
	if (Bank_Nodes(b,window,&positive,&negative)) {
		const int refusal = Bank_Cross_Link_Refusal(b,window,positive,negative);
		if (refusal == HYBRID_BINDING_OK) {
			QuotaCandidate candidate = base;
			candidate.Binding		= BRIDGE_BINDING_CROSS_LINK;
			candidate.Positive	= positive;
			candidate.Negative	= negative;
			out->Add(candidate);
			return 1;
		}
		Note_Binding_Refusal(b,refusal);
		Note_Quota_Refusal(census,HYBRID_QUOTA_PATH_CROSS_LINK,refusal);
	} else {
		census->Refused[HYBRID_QUOTA_PATH_CROSS_LINK][HYBRID_QUOTA_REFUSE_NO_BANK_PAIR]++;
	}

	const float corridor_half = Hybrid_Max_Corridor_Half_Width(HYBRID_ROAD_PRIMARY_2,b.P->Constants.S,b.P->Constants.D);
	bool any_corridor = false;
	for (int family = 0; family < b.FamilyCount; ++family) {
		if (Distance_To_Family_Chain(b,family,window.Center) > corridor_half) continue;
		any_corridor = true;
		if (!Quota_Family_Binding(b,window,family,HYBRID_QUOTA_PATH_FALLBACK,census)) continue;
		QuotaCandidate candidate = base;
		candidate.Family		= family;
		candidate.Distance	= Family_Node_Distance(b,family,window.Center);
		candidate.Binding		= BRIDGE_BINDING_FAMILY;
		out->Add(candidate);
		offered++;
	}
	if (!any_corridor) census->Refused[HYBRID_QUOTA_PATH_FALLBACK][HYBRID_QUOTA_REFUSE_CORRIDOR]++;
	return offered;
}


static bool Fill_Bridge_Quota(TopologyBuilder & b,bool river,HybridTopologyResult * result)
{
	StringClass detail;
	HybridQuotaCensus local;
	HybridQuotaCensus * census = (result != nullptr) ? (river ? &result->RiverQuota : &result->CanyonQuota) : &local;

	for (;;) {

		const int have = river ? b.Out->RiverBridges : b.Out->CanyonBridges;
		const int required = river ? b.Out->RequiredRiverBridges : b.Out->RequiredCanyonBridges;
		if (have >= required) {
			census->Have = have;
			return true;
		}
		census->Reset();
		census->Required	= required;
		census->Have		= have;

		bool have_choice = false;
		QuotaCandidate chosen;
		bool have_ranked = false;
		QuotaCandidate ranked_first;			//	the best window before Q-023's realizability is asked
		int refused = 0;

		for (int w = 0; w < b.Hydrology->Windows.Count(); ++w) {

			const HybridCrossingWindow & window = b.Hydrology->Windows[w];
			if (!Window_Is_Bridge_Category(window,river)) continue;
			census->Candidates++;
			if (b.WindowUsed[w] != 0) {
				census->Refused[HYBRID_QUOTA_PATH_FILTER][HYBRID_QUOTA_REFUSE_CONSUMED]++;
				continue;
			}
			if (!window.Has_Method(HYBRID_WINDOW_METHOD_BRIDGE)) {
				census->Refused[HYBRID_QUOTA_PATH_FILTER][HYBRID_QUOTA_REFUSE_METHOD]++;
				continue;
			}
			if (!Window_Spacing_Is_Legal(b,window)) {
				census->Refused[HYBRID_QUOTA_PATH_FILTER][HYBRID_QUOTA_REFUSE_SPACING]++;
				continue;
			}

			QuotaCandidate plain;
			plain.Window		= w;
			plain.Preferred	= window.Preferred;
			plain.StableId		= window.StableId;
			plain.Family		= Nearest_Family(b,window.Center,&plain.Distance);
			if (!have_ranked || Quota_Outranks(plain,ranked_first)) {
				ranked_first	= plain;
				have_ranked		= true;
			}

			//	Q-023: a binding no road could drive is not ranked at all.
			DynamicVectorClass<QuotaCandidate> bindings;
			if (Quota_Bindings(b,w,census,&bindings) == 0) {
				refused++;
				continue;
			}
			census->Bindable++;
			for (int i = 0; i < bindings.Count(); ++i) {
				if (have_choice && !Quota_Outranks(bindings[i],chosen)) continue;
				chosen		= bindings[i];
				have_choice	= true;
			}
		}

		if (!have_choice) {
			detail.Format("%s bridge quota short: %d of %d, no bridge-capable window left (%d refused as undrivable)",
								river ? "river" : "canyon/ravine",have,required,refused);
			return Fail(result,HYBRID_TOPOLOGY_BRIDGE_QUOTA,detail.Peek_Buffer());
		}
		if (have_ranked && (chosen.Window != ranked_first.Window)) b.Out->RecoveredByNextRank++;

		const HybridCrossingWindow & window = b.Hydrology->Windows[chosen.Window];
		if (chosen.Binding == BRIDGE_BINDING_FAMILY) {
			const unsigned int node = Add_Node(b,HYBRID_NODE_CROSSING_OBLIGATION,window.Center,chosen.Family,
														  -1,0,window.StableId,-1);
			Insert_Waypoint(b,chosen.Family,node,window.Center);
		} else {
			Add_Bank_Cross_Link(b,window,chosen.Positive,chosen.Negative);
		}
		Consume_Window(b,window);
	}
}

/***********************************************************************************************
**	Step 5: the highway assignment, by graph-level trunk length.
***********************************************************************************************/

static void Assign_Highways(TopologyBuilder & b)
{
	const int quota = b.Out->RequiredHighways;

	//	Trunk length per family, as the plan measures it: the sum of its graph-level edge lengths.
	DynamicVectorClass<float> length;
	DynamicVectorClass<int> order;
	for (int family = 0; family < b.FamilyCount; ++family) {
		float total = 0.0f;
		for (int e = 0; e < b.Out->Edges.Count(); ++e) {
			if (b.Out->Edges[e].Kind != HYBRID_EDGE_FAMILY_TRUNK) continue;
			if (b.Out->Edges[e].RouteFamily != family) continue;
			total += b.Out->Edges[e].Length;
		}
		length.Add(Hybrid_Quantize_Position(total));
		order.Add(family);
	}

	//	Descending trunk length, then ascending family id, with no other eligibility judgment.
	for (int i = 1; i < order.Count(); ++i) {
		for (int j = i; j > 0; --j) {
			bool swap = false;
			if (length[order[j]] > length[order[j - 1]])												swap = true;
			else if ((length[order[j]] == length[order[j - 1]]) && (order[j] < order[j - 1]))	swap = true;
			if (!swap) break;
			const int hold = order[j - 1];
			order[j - 1]	= order[j];
			order[j]			= hold;
		}
	}

	for (int i = 0; i < order.Count(); ++i) {

		const HybridRoadClass road_class = (i < quota) ? HYBRID_ROAD_HIGHWAY_4 : HYBRID_ROAD_PRIMARY_2;
		if (i < quota) b.Out->Highways++;

		for (int e = 0; e < b.Out->Edges.Count(); ++e) {
			if (b.Out->Edges[e].Kind != HYBRID_EDGE_FAMILY_TRUNK) continue;
			if (b.Out->Edges[e].RouteFamily != order[i]) continue;
			b.Out->Edges[e].RoadClass = road_class;
		}
	}
}


/***********************************************************************************************
**	Steps 6 to 8: the field access points, the home roads and the field service branches.
***********************************************************************************************/

//	The Phase-3 attachment point of a field: the other endpoint of one of its own intent edges.
static bool Field_Attachment(const TopologyBuilder & b,unsigned int field_site,Vector2 * out,
										unsigned int * node_out)
{
	float best = 3.4e38f;
	bool found = false;

	for (int e = 0; e < b.Graph->Edges.Count(); ++e) {

		const HybridStrategicEdge & edge = b.Graph->Edges[e];
		unsigned int other_id = 0;
		if (edge.SiteA == field_site)			other_id = edge.SiteB;
		else if (edge.SiteB == field_site)	other_id = edge.SiteA;
		else											continue;

		const HybridStrategicSite * other = b.Graph->Find_Site(other_id);
		if (other == nullptr) continue;

		//	The topology node that stands for that site: a waypoint, or a base's own entrance.
		for (int n = 0; n < b.Out->Nodes.Count(); ++n) {

			const HybridTopologyNode & node = b.Out->Nodes[n];
			const bool matches = (node.SiteId == other_id) &&
										((node.Kind == HYBRID_NODE_FAMILY_WAYPOINT) ||
										 (node.Kind == HYBRID_NODE_BASE_ENTRANCE));
			if (!matches) continue;

			const float distance = (node.Position - other->Position).Length();
			if (distance >= best) continue;
			best			= distance;
			*out			= node.Position;
			*node_out	= node.StableId;
			found			= true;
		}
	}
	return found;
}


/*
**	Step 6.  The frozen `FIELD_ACCESS_POINT`: `p = c + dir*(field_radius_max + min_vehicle_lane_clearance)`
**	with `dir = normalize(a - c)`, rotated in 22.5-degree steps (`+22.5,-22.5,+45,-45,...`) until the
**	first hard-valid point.  The road always terminates here, never at the field centre.
*/
static bool Create_Field_Access(TopologyBuilder & b,const HybridResourceAnchor & anchor,const Vector2 & attachment,
											unsigned int * node_out,HybridTopologyResult * result)
{
	const HybridConstants & k = b.P->Constants;
	const float reach = b.P->FieldRadiusMax + b.P->MinVehicleLaneClearance;
	const float turning = FIELD_ACCESS_TURN_FACTOR * b.P->HarvesterLength;
	StringClass detail;

	Vector2 base_dir = attachment - anchor.Center;
	if (base_dir.Length() <= 0.0f) {
		//	`a == c`: the plan falls back to the strategic vector from the map centre outward.
		base_dir = anchor.Center - b.Identity->Frame.Center;
		if (base_dir.Length() <= 0.0f) base_dir = b.Identity->Frame.AxisU;
	}
	base_dir *= (1.0f / base_dir.Length());

	for (int rotation = 0; rotation <= FIELD_ACCESS_ROTATIONS; ++rotation) {

		//	0, +22.5, -22.5, +45, -45, ... +180: the plan's own search order.
		const int step = (rotation + 1) / 2;
		const float sign = ((rotation % 2) == 1) ? 1.0f : -1.0f;
		const float angle = sign * (float)step * (360.0f / (float)FIELD_ACCESS_ROTATIONS) * PI_OVER_180;
		const float c = (float)cos((double)angle);
		const float s = (float)sin((double)angle);
		const Vector2 direction((base_dir.X * c) - (base_dir.Y * s),(base_dir.X * s) + (base_dir.Y * c));
		const Vector2 point = anchor.Center + (direction * reach);

		if (!Point_Is_Clear(b,point,false)) continue;

		//	The straight run from the reserve edge outward, and the Harvester's turning disc.
		const Vector2 edge_point = anchor.Center + (direction * b.P->FieldRadiusMax);
		if (!Run_Within_Grade(b,edge_point,point,HYBRID_ROAD_SERVICE_DIRT_2)) continue;

		bool clear = true;
		const float sample = Max2(0.5f * k.A,k.Q);
		for (float y = point.Y - turning; (y <= (point.Y + turning)) && clear; y += sample) {
			for (float x = point.X - turning; (x <= (point.X + turning)) && clear; x += sample) {
				const Vector2 disc(x,y);
				if ((disc - point).Length() > turning) continue;
				if (!Point_Is_Clear(b,disc,false)) clear = false;
			}
		}
		if (!clear) continue;

		*node_out = Add_Node(b,HYBRID_NODE_FIELD_ACCESS,point,-1,anchor.Team,anchor.SiteId,0,-1);
		return true;
	}

	detail.Format("field %u has no hard-valid access point in any of the 16 rotations",anchor.SiteId);
	return Fail(result,HYBRID_TOPOLOGY_NO_FIELD_ACCESS,detail.Peek_Buffer());
}


/*
**	Steps 6, 7 and 8 together, because step 6's attachment point is what steps 7 and 8 then connect: a
**	home field attaches to its own base's nearest entrance, every other field to its Phase-3 route
**	attachment by `SERVICE_DIRT_2`.
*/
static bool Build_Field_Roads(TopologyBuilder & b,HybridTopologyResult * result)
{
	const HybridConstants & k = b.P->Constants;

	for (int i = 0; i < b.Anchors->Resources.Count(); ++i) {

		const HybridResourceAnchor & anchor = b.Anchors->Resources[i];
		const bool home = (anchor.Role == HYBRID_SITE_HOME_TIBERIUM_GDI) ||
								(anchor.Role == HYBRID_SITE_HOME_TIBERIUM_NOD);

		Vector2 attachment(0.0f,0.0f);
		unsigned int attachment_node = 0;

		if (home) {

			//	The owning base's entrance at minimum quantized Euclidean distance.
			float best = 3.4e38f;
			for (int e = 0; e < b.Out->Entrances.Count(); ++e) {
				const HybridBaseEntrance & entrance = b.Out->Entrances[e];
				if (entrance.Team != anchor.Team) continue;
				const float distance = Hybrid_Quantize_Position((entrance.Point - anchor.Center).Length());
				if (distance >= best) continue;
				best					= distance;
				attachment			= entrance.Point;
				attachment_node	= entrance.NodeId;
			}
			if (attachment_node == 0) {
				return Fail(result,HYBRID_TOPOLOGY_NO_ENTRANCE_SECTOR,"a home field has no base entrance to attach to");
			}

		} else if (!Field_Attachment(b,anchor.SiteId,&attachment,&attachment_node)) {

			//	No Phase-3 attachment node: fall back to the nearest family waypoint, which is what the
			//	attachment intent would have named anyway.
			float best = 3.4e38f;
			for (int n = 0; n < b.Out->Nodes.Count(); ++n) {
				const HybridTopologyNode & node = b.Out->Nodes[n];
				if (node.Kind != HYBRID_NODE_FAMILY_WAYPOINT) continue;
				const float distance = (node.Position - anchor.Center).Length();
				if (distance >= best) continue;
				best					= distance;
				attachment			= node.Position;
				attachment_node	= node.StableId;
			}
			if (attachment_node == 0) {
				return Fail(result,HYBRID_TOPOLOGY_INVALID_INPUT,"no family waypoint exists to attach a field to");
			}
		}

		unsigned int access = 0;
		if (!Create_Field_Access(b,anchor,attachment,&access,result)) return false;

		if (home) {
			//	Step 7: PRIMARY_2, unless the entrance and the access point are closer than 12W.
			const float distance = Hybrid_Quantize_Position((attachment - b.Out->Find_Node(access)->Position).Length());
			const HybridRoadClass road_class = (distance < (HOME_ROAD_SECONDARY_W * k.W))
														 ? HYBRID_ROAD_SECONDARY_2 : HYBRID_ROAD_PRIMARY_2;
			Add_Edge(b,HYBRID_EDGE_HOME_ROAD,road_class,attachment_node,access,-1,true,0,0);
			continue;
		}

		//	Step 8: every other field branch is SERVICE_DIRT_2 and counts toward the Section-5 quota.
		Add_Edge(b,HYBRID_EDGE_FIELD_BRANCH,HYBRID_ROAD_SERVICE_DIRT_2,attachment_node,access,-1,true,0,0);
		b.Out->ServiceBranches++;
	}
	return true;
}


/***********************************************************************************************
**	Steps 9 and 10: the service and trail quota fills.
***********************************************************************************************/

struct PairCandidate
{
	unsigned int	NodeA;
	unsigned int	NodeB;
	float				Distance;
	float				Score;

	bool	operator == (const PairCandidate & that) const
	{
		return (NodeA == that.NodeA) && (NodeB == that.NodeB);
	}
	bool	operator != (const PairCandidate & that) const	{ return !(*this == that); }
};


//	Ascending (quantized distance, lower endpoint StableId, higher endpoint StableId).
static void Sort_By_Distance(DynamicVectorClass<PairCandidate> * pairs)
{
	for (int i = 1; i < pairs->Count(); ++i) {
		for (int j = i; j > 0; --j) {
			const PairCandidate & left = (*pairs)[j - 1];
			const PairCandidate & right = (*pairs)[j];
			bool swap = false;
			if (right.Distance < left.Distance)											swap = true;
			else if (right.Distance == left.Distance) {
				if (right.NodeA < left.NodeA)												swap = true;
				else if ((right.NodeA == left.NodeA) && (right.NodeB < left.NodeB))	swap = true;
			}
			if (!swap) break;
			const PairCandidate hold = (*pairs)[j - 1];
			(*pairs)[j - 1]	= (*pairs)[j];
			(*pairs)[j]			= hold;
		}
	}
}


/*
**	A "non-base waypoint": every topology node a road may start or end at, which is everything except the
**	logical base node, its entrances (reached only through BASE_INTERNAL) and the two structural waypoints.  A
**	mid-edge overpass is a deck; a crossing obligation is a bridge or pass the road through it must run
**	straight across (Q-022), so a road ending there would end in the middle of a structure -- measured, four of
**	seventeen Phase-11 road x road refusals in a 1400-world census were cross-links ending at one.
*/
static bool Is_Pairable_Waypoint(const HybridTopologyNode & node)
{
	return (node.Kind == HYBRID_NODE_FAMILY_WAYPOINT) || (node.Kind == HYBRID_NODE_FIELD_ACCESS);
}


/*
**	Candidate pairs of non-base waypoints.  `different_families` is step 10's and step 11's extra rule;
**	`minimum` is their `>=8W` floor, which step 9 does not have.
**
**	"Non-base waypoint" is read as every node a road may end at, not only a Phase-3 family waypoint: a
**	two-family map has exactly two of those and exactly one pair between them, which cannot fill a
**	Section-5 trail quota of two by any ordering.  A field access point is a waypoint in the same sense, and
**	pairing it keeps the intent of the rule -- a shortcut between *different* routes -- because the "different
**	families" test still refuses two nodes of one family.  A crossing waypoint is not: see
**	Is_Pairable_Waypoint.
*/
static void Collect_Waypoint_Pairs(const TopologyBuilder & b,bool different_families,float minimum,
											  DynamicVectorClass<PairCandidate> * out)
{
	out->Delete_All();

	for (int i = 0; i < b.Out->Nodes.Count(); ++i) {

		const HybridTopologyNode & first = b.Out->Nodes[i];
		if (!Is_Pairable_Waypoint(first)) continue;

		for (int j = i + 1; j < b.Out->Nodes.Count(); ++j) {

			const HybridTopologyNode & second = b.Out->Nodes[j];
			if (!Is_Pairable_Waypoint(second)) continue;
			if (different_families && (first.RouteFamily >= 0) && (second.RouteFamily >= 0) &&
				 (first.RouteFamily == second.RouteFamily)) continue;
			if (Edges_Adjacent(b,first.StableId,second.StableId)) continue;

			const float distance = Hybrid_Quantize_Position((second.Position - first.Position).Length());
			if (distance < minimum) continue;

			PairCandidate candidate;
			candidate.NodeA		= (first.StableId < second.StableId) ? first.StableId : second.StableId;
			candidate.NodeB		= (first.StableId < second.StableId) ? second.StableId : first.StableId;
			candidate.Distance	= distance;
			candidate.Score		= 0.0f;
			out->Add(candidate);
		}
	}
	Sort_By_Distance(out);
}


static bool Fill_Service_Quota(TopologyBuilder & b,HybridTopologyResult * result)
{
	if (b.Out->ServiceBranches >= b.Out->RequiredServiceBranches) return true;

	DynamicVectorClass<PairCandidate> pairs;
	Collect_Waypoint_Pairs(b,false,0.0f,&pairs);

	DynamicVectorClass<unsigned int> consumed;

	for (int i = 0; (i < pairs.Count()) && (b.Out->ServiceBranches < b.Out->RequiredServiceBranches); ++i) {

		//	A pair already consumed by another newly added service edge is skipped, not reused.
		bool used = false;
		for (int c = 0; c < consumed.Count(); ++c) {
			if ((consumed[c] == pairs[i].NodeA) || (consumed[c] == pairs[i].NodeB)) used = true;
		}
		if (used) continue;

		Add_Edge(b,HYBRID_EDGE_SERVICE_FILL,HYBRID_ROAD_SERVICE_DIRT_2,pairs[i].NodeA,pairs[i].NodeB,-1,
					false,0,0);
		consumed.Add(pairs[i].NodeA);
		consumed.Add(pairs[i].NodeB);
		b.Out->ServiceBranches++;
	}

	if (b.Out->ServiceBranches < b.Out->RequiredServiceBranches) {
		StringClass detail;
		detail.Format("service quota short: %d of %d, %d candidate pairs",b.Out->ServiceBranches,
						  b.Out->RequiredServiceBranches,pairs.Count());
		return Fail(result,HYBRID_TOPOLOGY_SERVICE_QUOTA,detail.Peek_Buffer());
	}
	return true;
}


static bool Fill_Trail_Quota(TopologyBuilder & b,HybridTopologyResult * result)
{
	const float minimum = TRAIL_MIN_W * b.P->Constants.W;

	DynamicVectorClass<PairCandidate> pairs;
	Collect_Waypoint_Pairs(b,true,minimum,&pairs);

	for (int i = 0; (i < pairs.Count()) && (b.Out->Trails < b.Out->RequiredTrails); ++i) {
		if (Edges_Adjacent(b,pairs[i].NodeA,pairs[i].NodeB)) continue;
		Add_Edge(b,HYBRID_EDGE_TRAIL,HYBRID_ROAD_TRAIL_1,pairs[i].NodeA,pairs[i].NodeB,-1,false,0,0);
		b.Out->Trails++;
	}

	if (b.Out->Trails < b.Out->RequiredTrails) {
		StringClass detail;
		detail.Format("trail quota short: %d of %d, %d candidate pairs",b.Out->Trails,b.Out->RequiredTrails,
						  pairs.Count());
		return Fail(result,HYBRID_TOPOLOGY_TRAIL_QUOTA,detail.Peek_Buffer());
	}
	return true;
}


/***********************************************************************************************
**	Step 11: the optional heavy cross-links and their exact benefit score.
***********************************************************************************************/

//	The mean finite shortest-path distance over all unordered node pairs, by Floyd-Warshall.
static float Mean_Finite_Distance(const HybridTopology & topology)
{
	const int count = topology.Nodes.Count();
	if (count <= 1) return 0.0f;

	DynamicVectorClass<float> distance;
	for (int i = 0; i < (count * count); ++i) distance.Add(-1.0f);
	for (int i = 0; i < count; ++i) distance[(i * count) + i] = 0.0f;

	for (int e = 0; e < topology.Edges.Count(); ++e) {

		const HybridTopologyEdge & edge = topology.Edges[e];
		int a = -1, b = -1;
		for (int i = 0; i < count; ++i) {
			if (topology.Nodes[i].StableId == edge.NodeA) a = i;
			if (topology.Nodes[i].StableId == edge.NodeB) b = i;
		}
		if ((a < 0) || (b < 0)) continue;

		const float length = (edge.Kind == HYBRID_EDGE_BASE_INTERNAL) ? 0.0f : edge.Length;
		const int forward = (a * count) + b;
		const int back = (b * count) + a;
		if ((distance[forward] < 0.0f) || (length < distance[forward])) {
			distance[forward]	= length;
			distance[back]		= length;
		}
	}

	for (int via = 0; via < count; ++via) {
		for (int from = 0; from < count; ++from) {
			const float first = distance[(from * count) + via];
			if (first < 0.0f) continue;
			for (int to = 0; to < count; ++to) {
				const float second = distance[(via * count) + to];
				if (second < 0.0f) continue;
				const int index = (from * count) + to;
				if ((distance[index] < 0.0f) || ((first + second) < distance[index])) {
					distance[index] = first + second;
				}
			}
		}
	}

	float total = 0.0f;
	int pairs = 0;
	for (int from = 0; from < count; ++from) {
		for (int to = from + 1; to < count; ++to) {
			const float value = distance[(from * count) + to];
			if (value < 0.0f) continue;				// disconnected pairs are excluded from the mean
			total += value;
			pairs++;
		}
	}
	return (pairs > 0) ? (total / (float)pairs) : 0.0f;
}


static void Add_Optional_Cross_Links(TopologyBuilder & b)
{
	const int quota = b.Out->OptionalCrossLinkQuota;
	if (quota <= 0) return;

	const float minimum = CROSS_LINK_MIN_W * b.P->Constants.W;
	const float frontier = (b.Identity->Archetype == HYBRID_ARCHETYPE_OPEN_FRONTIER)
								  ? CROSS_LINK_FRONTIER_WEIGHT : 0.0f;

	for (int accepted = 0; accepted < quota; ++accepted) {

		DynamicVectorClass<PairCandidate> pairs;
		Collect_Waypoint_Pairs(b,true,minimum,&pairs);
		if (pairs.Count() == 0) return;								// a shortage is not a phase failure

		/*
		**	The score is measured on the graph *before* the candidate: mean shortest-path reduction,
		**	max-flow gain and the fixed open-frontier term.
		*/
		const float mean_before = Mean_Finite_Distance(*b.Out);
		const int flow_before = Hybrid_Topology_Max_Flow(*b.Out,b.BaseNode[0],b.BaseNode[1]);
		const float family_scale = (float)((b.FamilyCount > 0) ? b.FamilyCount : 1);

		int best = -1;
		float best_score = -1.0f;

		for (int i = 0; i < pairs.Count(); ++i) {

			//	Temporarily add the candidate, measure, then take it back out.
			const unsigned int edge = Add_Edge(b,HYBRID_EDGE_CROSS_LINK,HYBRID_ROAD_SECONDARY_2,pairs[i].NodeA,
														  pairs[i].NodeB,-1,false,0,0,true);
			const float mean_after = Mean_Finite_Distance(*b.Out);
			const int flow_after = Hybrid_Topology_Max_Flow(*b.Out,b.BaseNode[0],b.BaseNode[1]);
			b.Out->Edges.Delete(b.Out->Edges.Count() - 1);
			b.NextEdgeId = edge;											// the id is returned with the candidate

			const float reduction = Clamp2((mean_before - mean_after) / Max2(mean_before,b.P->Constants.Q),
													 0.0f,1.0f);
			const float gain = Clamp2((float)(flow_after - flow_before) / family_scale,0.0f,1.0f);
			const float score = (CROSS_LINK_PATH_WEIGHT * reduction) + (CROSS_LINK_FLOW_WEIGHT * gain) + frontier;

			pairs[i].Score = score;
			if (best >= 0) {
				if (score < best_score) continue;
				if ((score == best_score) && (pairs[i].Distance >= pairs[best].Distance)) continue;
			}
			best			= i;
			best_score	= score;
		}

		if (best < 0) return;

		//	Step 11: an optional *heavy* cross-link, so it carries the heavy reference.
		Add_Edge(b,HYBRID_EDGE_CROSS_LINK,HYBRID_ROAD_SECONDARY_2,pairs[best].NodeA,pairs[best].NodeB,-1,
					false,0,0,true);
		b.Out->CrossLinks++;
	}
}


/***********************************************************************************************
**	Steps 12 to 14: the overpass obligations, natural first and synthesized second.
***********************************************************************************************/

//	The proper crossing point of two segments, or false when they do not cross at an interior point.
static bool Segments_Cross(const Vector2 & a0,const Vector2 & a1,const Vector2 & b0,const Vector2 & b1,
									Vector2 * point_out,float * t_out,float * u_out)
{
	const Vector2 first = a1 - a0;
	const Vector2 second = b1 - b0;
	const float denominator = (first.X * second.Y) - (first.Y * second.X);
	if ((float)fabs((double)denominator) <= 1.0e-6f) return false;

	const Vector2 delta = b0 - a0;
	const float t = ((delta.X * second.Y) - (delta.Y * second.X)) / denominator;
	const float u = ((delta.X * first.Y) - (delta.Y * first.X)) / denominator;
	if ((t <= 0.0f) || (t >= 1.0f) || (u <= 0.0f) || (u >= 1.0f)) return false;

	if (point_out != nullptr) point_out->Set(Hybrid_Quantize_Position(a0.X + (first.X * t)),
															Hybrid_Quantize_Position(a0.Y + (first.Y * t)));
	if (t_out != nullptr) *t_out = t;
	if (u_out != nullptr) *u_out = u;
	return true;
}


static float Crossing_Angle_Degrees(const Vector2 & a0,const Vector2 & a1,const Vector2 & b0,const Vector2 & b1)
{
	Vector2 first = a1 - a0;
	Vector2 second = b1 - b0;
	if ((first.Length() <= 0.0f) || (second.Length() <= 0.0f)) return 0.0f;
	first *= (1.0f / first.Length());
	second *= (1.0f / second.Length());
	const float dot = Clamp2((float)fabs((double)((first.X * second.X) + (first.Y * second.Y))),0.0f,1.0f);
	return (float)acos((double)dot) / PI_OVER_180 + 0.0f;		// 0..90, measured between the lines
}


/*
**	Is an overpass legal at this point?  The exclusions and the spacing rule of step 12 -- plus the one
**	step 12 leaves implicit: the point has to be ground a road can stand on.
**
**	Step 12 lists base, Tiberium and crossing-window exclusions and a slope test, and says nothing about
**	water or walls, because an overpass is a road-over-road crossing between two road intent segments and
**	those segments are already meant to be routable.  They are only *intent* segments, though: a straight
**	line between two frozen waypoints crosses whatever lies between them.  Measured on the failing seed,
**	an overpass obligation landed in a lake, and Phase 10 then had a frozen waypoint no route could reach
**	-- standing water is never crossable (step 20) -- so the whole family trunk failed.  The clearance is
**	one analysis spacing plus the roadbed half width, which is what it takes for the router to arrive at
**	the waypoint and leave it again on the analysis grid.
*/
static int Overpass_Point_Refusal(const TopologyBuilder & b,const Vector2 & point)
{
	const HybridConstants & k = b.P->Constants;

	if (!Point_Is_Clear(b,point,false)) return HYBRID_OVERPASS_REFUSE_PROTECTED;

	const float clearance = k.A + (0.5f * Hybrid_Roadbed_Width(HYBRID_ROAD_SECONDARY_2,k.W));
	for (int i = 0; i < b.Hydrology->Bodies.Count(); ++i) {

		const HybridWaterBody & body = b.Hydrology->Bodies[i];
		if (body.Kind == HYBRID_WATER_BODY_ISLAND) continue;
		if ((point.X < (body.BoundsMin.X - clearance)) || (point.X > (body.BoundsMax.X + clearance)) ||
			 (point.Y < (body.BoundsMin.Y - clearance)) || (point.Y > (body.BoundsMax.Y + clearance))) continue;

		//	Water that merely surrounds the point still encloses it on the analysis grid.
		for (int p = 0; p < body.Points.Count(); ++p) {
			if ((body.Points[p] - point).Length() <= clearance) return HYBRID_OVERPASS_REFUSE_WATER_CLEARANCE;
		}
	}

	for (int i = 0; i < b.Anchors->Bases.Count(); ++i) {
		if ((point - b.Anchors->Bases[i].Center).Length() <= b.Anchors->Bases[i].ProtectedRadius) return HYBRID_OVERPASS_REFUSE_PROTECTED;
	}
	for (int i = 0; i < b.Anchors->Resources.Count(); ++i) {
		if ((point - b.Anchors->Resources[i].Center).Length() <= b.Anchors->Resources[i].ReserveRadius) return HYBRID_OVERPASS_REFUSE_TIBERIUM;
	}
	/*
	**	A selected crossing window's protected bounds: the obstruction and its straight approach reserve.  Whether
	**	an overpass leaves a road on the same edge room to turn onto that crossing, and its own ramps their `10W`,
	**	is the Q-023 pose rule's question (Overpass_Refusal), answered with the class's own radius construction; a
	**	fixed distance from every selected window refused overpasses on roads that never use that window.
	*/
	for (int i = 0; i < b.SelectedWindow.Count(); ++i) {
		const HybridCrossingWindow * window = b.Hydrology->Find_Window(b.SelectedWindow[i]);
		if (window == nullptr) continue;
		const float own = window->ApproachLength + (0.5f * window->Span);
		if ((point - window->Center).Length() <= own) return HYBRID_OVERPASS_REFUSE_WINDOW;
	}
	for (int n = 0; n < b.Out->Nodes.Count(); ++n) {
		if (b.Out->Nodes[n].Kind != HYBRID_NODE_OVERPASS_OBLIGATION) continue;
		if ((point - b.Out->Nodes[n].Position).Length() < (OVERPASS_SPACING_W * k.W)) return HYBRID_OVERPASS_REFUSE_SPACING;
	}
	return -1;
}


struct OverpassCandidate
{
	int				HighwayEdge;		// index into Out->Edges
	int				HighwaySlot;		// the highway's intent sub-segment the crossing lies on
	int				OtherEdge;			// index, or -1 for a synthesized cross-link candidate
	int				OtherSlot;
	unsigned int	NodeA;				// the synthesized candidate's own endpoints
	unsigned int	NodeB;
	Vector2			Point;
	float				Cost;
	float				Length;

	bool	operator == (const OverpassCandidate & that) const
	{
		return (HighwayEdge == that.HighwayEdge) && (OtherEdge == that.OtherEdge) &&
				 (NodeA == that.NodeA) && (NodeB == that.NodeB);
	}
	bool	operator != (const OverpassCandidate & that) const	{ return !(*this == that); }
};


/*
**	Step 12's exact cost: `clamp(local_max_slope/SECONDARY_2.max_grade,0,1) +
**	clamp(distance_to_nearest_edge_endpoint/(0.5D),0,1)`, with the slope sampled over the `2W` disc the
**	plan names.  -1 when the candidate is hard-valid, otherwise the first step-12 rule it fails.
*/
static int Overpass_Cost(const TopologyBuilder & b,const Vector2 & point,const Vector2 & a0,const Vector2 & a1,
								 const Vector2 & b0,const Vector2 & b1,float * cost_out)
{
	const HybridConstants & k = b.P->Constants;
	const float arm = OVERPASS_ARM_W * k.W;
	const float max_grade = Hybrid_Road_Class_Spec(HYBRID_ROAD_SECONDARY_2).MaxGradeDegrees * PI_OVER_180;

	/*
	**	`60..120` degrees between the two directed segments is `>= 60` between the two lines, which is what
	**	Crossing_Angle_Degrees measures (0..90).  The test this replaces compared that 0..90 value against both
	**	ends of the directed band and so could never refuse anything.
	*/
	const float angle = Crossing_Angle_Degrees(a0,a1,b0,b1);
	if (angle < (OVERPASS_ANGLE_MIN - 0.001f)) return HYBRID_OVERPASS_REFUSE_ANGLE;
	if ((180.0f - angle) > (OVERPASS_ANGLE_MAX + 90.001f)) return HYBRID_OVERPASS_REFUSE_ANGLE;

	//	At least 10W of each segment on each side of the crossing.
	if (((point - a0).Length() < arm) || ((point - a1).Length() < arm)) return HYBRID_OVERPASS_REFUSE_ARM;
	if (((point - b0).Length() < arm) || ((point - b1).Length() < arm)) return HYBRID_OVERPASS_REFUSE_ARM_OTHER;

	const int refusal = Overpass_Point_Refusal(b,point);
	if (refusal >= 0) return refusal;

	float slope = 0.0f;
	if (!Disc_Max_Slope(b,point,OVERPASS_SLOPE_DISC_W * k.W,&slope)) return HYBRID_OVERPASS_REFUSE_PROTECTED;
	if (slope > max_grade) return HYBRID_OVERPASS_REFUSE_SLOPE;

	float nearest = (point - a0).Length();
	nearest = Min2(nearest,(point - a1).Length());
	nearest = Min2(nearest,(point - b0).Length());
	nearest = Min2(nearest,(point - b1).Length());

	*cost_out = Clamp2(slope / Max2(max_grade,k.Q),0.0f,1.0f) + Clamp2(nearest / (0.5f * k.D),0.0f,1.0f);
	return -1;
}


/*
**	An edge as Phase 10 drives it: NodeA, its waypoints in order, NodeB, joined by straight intent segments.
**	Sub-segment `slot` runs from chain node `slot` to chain node `slot + 1`, and a waypoint inserted on it goes in
**	at `Waypoints[slot]`.  Steps 12 and 13 treat "every current topology edge as a straight intent segment between
**	its frozen endpoints"; once an edge carries waypoints, the straight segments it is driven along are these, and
**	the angle, the `10W` either side and the deck axis that Phase 10 realizes are all measured on the one the
**	crossing lies on.  Measuring them on the endpoint-to-endpoint line accepted overpasses the two roads then met
**	at 17 to 27 degrees.
*/
static int Edge_Segment_Count(const HybridTopologyEdge & edge)
{
	return edge.Waypoints.Count() + 1;
}


static bool Edge_Segment(const TopologyBuilder & b,const HybridTopologyEdge & edge,int slot,Vector2 * from,Vector2 * to)
{
	const unsigned int before = (slot > 0) ? edge.Waypoints[slot - 1] : edge.NodeA;
	const unsigned int after = (slot < edge.Waypoints.Count()) ? edge.Waypoints[slot] : edge.NodeB;
	const HybridTopologyNode * a = b.Out->Find_Node(before);
	const HybridTopologyNode * c = b.Out->Find_Node(after);
	if ((a == nullptr) || (c == nullptr)) return false;
	*from	= a->Position;
	*to	= c->Position;
	return true;
}


/*
**	Q-023 for an overpass: on this participating edge, the road reaches the shared point along its own planned
**	direction, keeps the `10W` either side its ramps need, and can get there from the waypoint behind and on to
**	the one ahead with its own class radius.
*/
static int Overpass_Refusal(const TopologyBuilder & b,const HybridTopologyEdge & edge,int slot,const Vector2 & point,
									 const Vector2 & own_axis,const Vector2 & other_axis,HybridRoadClass other_class)
{
	DynamicVectorClass<ChainEntry> chain;
	Edge_Chain(b,edge,&chain);
	ChainEntry candidate;
	candidate.Kind		= HYBRID_NODE_OVERPASS_OBLIGATION;
	candidate.Position	= point;
	candidate.HalfDeck	= Hybrid_Overpass_Half_Deck(own_axis,other_axis,edge.RoadClass,other_class,b.P->Constants.W);
	const int at = slot + 1;
	chain.Insert(at,candidate);
	return Chain_Refusal(b,chain,at - 1,at + 1,edge.RoadClass);
}


//	The same for a synthesized `SECONDARY_2` cross-link that does not exist yet: `a` to `c` through the point.
static int Synthesized_Overpass_Refusal(const TopologyBuilder & b,unsigned int a,unsigned int c,const Vector2 & point,
													 const Vector2 & other_axis,HybridRoadClass other_class)
{
	DynamicVectorClass<ChainEntry> chain;
	chain.Add(Chain_Entry_Of(b,a));
	ChainEntry candidate;
	candidate.Kind		= HYBRID_NODE_OVERPASS_OBLIGATION;
	candidate.Position	= point;
	const HybridTopologyNode * na = b.Out->Find_Node(a);
	const HybridTopologyNode * nc = b.Out->Find_Node(c);
	if ((na != nullptr) && (nc != nullptr)) {
		candidate.HalfDeck = Hybrid_Overpass_Half_Deck(nc->Position - na->Position,other_axis,HYBRID_ROAD_SECONDARY_2,
																	  other_class,b.P->Constants.W);
	}
	chain.Add(candidate);
	chain.Add(Chain_Entry_Of(b,c));
	return Chain_Refusal(b,chain,1,1,HYBRID_ROAD_SECONDARY_2);
}


//	Step 14: one shared waypoint, inserted into both participating edges at the same quantized point.
static void Insert_Shared_Overpass(TopologyBuilder & b,int highway_edge,int highway_slot,int other_edge,int other_slot,
											  const Vector2 & point)
{
	const unsigned int node = Add_Node(b,HYBRID_NODE_OVERPASS_OBLIGATION,point,-1,-1,0,0,-1);

	const int edges[2] = { highway_edge, other_edge };
	const int slots[2] = { highway_slot, other_slot };
	for (int i = 0; i < 2; ++i) {
		if ((edges[i] < 0) || (edges[i] >= b.Out->Edges.Count())) continue;
		HybridTopologyEdge & edge = b.Out->Edges[edges[i]];
		edge.Waypoints.Insert(slots[i],node);
	}
	b.Out->ReservedOverpassCount++;
}


static bool Reserve_Overpasses(TopologyBuilder & b,HybridTopologyResult * result)
{
	const HybridConstants & k = b.P->Constants;
	const float minimum = CROSS_LINK_MIN_W * k.W;
	StringClass detail;
	HybridOverpassCensus local;
	HybridOverpassCensus * census = (result != nullptr) ? &result->OverpassQuota : &local;
	census->Reset();
	census->Required	= b.Out->RequestedOverpassTarget;
	census->Have		= b.Out->ReservedOverpassCount;
	census->Families	= b.FamilyCount;
	census->Highways	= b.Out->Highways;

	//	Step 12: the natural intersections between a HIGHWAY_4 segment and a PRIMARY_2/SECONDARY_2 one.
	while (b.Out->ReservedOverpassCount < b.Out->RequestedOverpassTarget) {

		DynamicVectorClass<OverpassCandidate> candidates;
		census->Natural = 0;
		for (int r = 0; r < HYBRID_OVERPASS_REFUSAL_COUNT; ++r) census->NaturalRefused[r] = 0;

		for (int h = 0; h < b.Out->Edges.Count(); ++h) {

			const HybridTopologyEdge & highway = b.Out->Edges[h];
			if (highway.RoadClass != HYBRID_ROAD_HIGHWAY_4) continue;

			for (int o = 0; o < b.Out->Edges.Count(); ++o) {

				if (o == h) continue;
				const HybridTopologyEdge & other = b.Out->Edges[o];
				if ((other.RoadClass != HYBRID_ROAD_PRIMARY_2) && (other.RoadClass != HYBRID_ROAD_SECONDARY_2)) continue;

				for (int hs = 0; hs < Edge_Segment_Count(highway); ++hs) {
					Vector2 h0(0.0f,0.0f);
					Vector2 h1(0.0f,0.0f);
					if (!Edge_Segment(b,highway,hs,&h0,&h1)) continue;

					for (int os = 0; os < Edge_Segment_Count(other); ++os) {
						Vector2 o0(0.0f,0.0f);
						Vector2 o1(0.0f,0.0f);
						if (!Edge_Segment(b,other,os,&o0,&o1)) continue;

						Vector2 point(0.0f,0.0f);
						if (!Segments_Cross(h0,h1,o0,o1,&point,nullptr,nullptr)) continue;
						census->Natural++;

						float cost = 0.0f;
						const int rule = Overpass_Cost(b,point,h0,h1,o0,o1,&cost);
						if (rule >= 0) {
							census->NaturalRefused[rule]++;
							if (rule == HYBRID_OVERPASS_REFUSE_ANGLE) {
								const int bucket = (int)(Crossing_Angle_Degrees(h0,h1,o0,o1) / 15.0f);
								census->AngleBucket[(bucket < 0) ? 0 : ((bucket > 3) ? 3 : bucket)]++;
							}
							continue;
						}

						//	Q-023: both roads have to be able to drive it.
						int refusal = Overpass_Refusal(b,highway,hs,point,h1 - h0,o1 - o0,other.RoadClass);
						if (refusal == HYBRID_BINDING_OK) {
							refusal = Overpass_Refusal(b,other,os,point,o1 - o0,h1 - h0,highway.RoadClass);
						}
						if (refusal != HYBRID_BINDING_OK) {
							Note_Binding_Refusal(b,refusal);
							census->NaturalRefused[HYBRID_OVERPASS_REFUSE_POSE]++;
							continue;
						}

						OverpassCandidate candidate;
						candidate.HighwayEdge	= h;
						candidate.HighwaySlot	= hs;
						candidate.OtherEdge		= o;
						candidate.OtherSlot		= os;
						candidate.NodeA			= 0;
						candidate.NodeB			= 0;
						candidate.Point			= point;
						candidate.Cost				= cost;
						candidate.Length			= 0.0f;
						candidates.Add(candidate);
					}
				}
			}
		}

		if (candidates.Count() == 0) break;

		//	Rank by (overpass_cost, highway edge StableId, other edge StableId).
		int best = 0;
		for (int i = 1; i < candidates.Count(); ++i) {
			const OverpassCandidate & left = candidates[best];
			const OverpassCandidate & right = candidates[i];
			if (right.Cost > left.Cost) continue;
			if (right.Cost == left.Cost) {
				const unsigned int left_highway = b.Out->Edges[left.HighwayEdge].StableId;
				const unsigned int right_highway = b.Out->Edges[right.HighwayEdge].StableId;
				if (right_highway > left_highway) continue;
				if (right_highway == left_highway) {
					if (b.Out->Edges[right.OtherEdge].StableId > b.Out->Edges[left.OtherEdge].StableId) continue;
				}
			}
			best = i;
		}
		Insert_Shared_Overpass(b,candidates[best].HighwayEdge,candidates[best].HighwaySlot,candidates[best].OtherEdge,
									  candidates[best].OtherSlot,candidates[best].Point);
	}

	//	Step 13: synthesized SECONDARY_2 cross-links that cross a highway, ranked with their own length.
	while (b.Out->ReservedOverpassCount < b.Out->RequestedOverpassTarget) {

		DynamicVectorClass<PairCandidate> pairs;
		Collect_Waypoint_Pairs(b,true,minimum,&pairs);

		DynamicVectorClass<OverpassCandidate> candidates;
		census->Pairs			= pairs.Count();
		census->HeavyPairs	= 0;
		for (int p = 0; p < pairs.Count(); ++p) {
			const HybridTopologyNode * na = b.Out->Find_Node(pairs[p].NodeA);
			const HybridTopologyNode * nc = b.Out->Find_Node(pairs[p].NodeB);
			if ((na != nullptr) && (nc != nullptr) && (na->RouteFamily >= 0) && (nc->RouteFamily >= 0)) census->HeavyPairs++;
		}
		census->Synthesized	= 0;
		for (int r = 0; r < HYBRID_OVERPASS_REFUSAL_COUNT; ++r) census->SynthesizedRefused[r] = 0;

		for (int p = 0; p < pairs.Count(); ++p) {

			const HybridTopologyNode * a = b.Out->Find_Node(pairs[p].NodeA);
			const HybridTopologyNode * c = b.Out->Find_Node(pairs[p].NodeB);
			if ((a == nullptr) || (c == nullptr)) continue;

			for (int h = 0; h < b.Out->Edges.Count(); ++h) {

				const HybridTopologyEdge & highway = b.Out->Edges[h];
				if (highway.RoadClass != HYBRID_ROAD_HIGHWAY_4) continue;

				for (int hs = 0; hs < Edge_Segment_Count(highway); ++hs) {
					Vector2 h0(0.0f,0.0f);
					Vector2 h1(0.0f,0.0f);
					if (!Edge_Segment(b,highway,hs,&h0,&h1)) continue;

					Vector2 point(0.0f,0.0f);
					if (!Segments_Cross(h0,h1,a->Position,c->Position,&point,nullptr,nullptr)) continue;
					census->Synthesized++;

					float cost = 0.0f;
					const int rule = Overpass_Cost(b,point,h0,h1,a->Position,c->Position,&cost);
					if (rule >= 0) {
						census->SynthesizedRefused[rule]++;
						if (rule == HYBRID_OVERPASS_REFUSE_ANGLE) {
							const int bucket = (int)(Crossing_Angle_Degrees(h0,h1,a->Position,c->Position) / 15.0f);
							census->AngleBucket[(bucket < 0) ? 0 : ((bucket > 3) ? 3 : bucket)]++;
						}
						continue;
					}

					int refusal = Overpass_Refusal(b,highway,hs,point,h1 - h0,c->Position - a->Position,
															 HYBRID_ROAD_SECONDARY_2);
					if (refusal == HYBRID_BINDING_OK) {
						refusal = Synthesized_Overpass_Refusal(b,pairs[p].NodeA,pairs[p].NodeB,point,h1 - h0,
																			highway.RoadClass);
					}
					if (refusal != HYBRID_BINDING_OK) {
						Note_Binding_Refusal(b,refusal);
						census->SynthesizedRefused[HYBRID_OVERPASS_REFUSE_POSE]++;
						continue;
					}

					OverpassCandidate candidate;
					candidate.HighwayEdge	= h;
					candidate.HighwaySlot	= hs;
					candidate.OtherEdge		= -1;
					candidate.OtherSlot		= 0;
					candidate.NodeA			= pairs[p].NodeA;
					candidate.NodeB			= pairs[p].NodeB;
					candidate.Point			= point;
					candidate.Cost				= cost + (pairs[p].Distance / Max2(b.P->Constants.D,b.P->Constants.Q));
					candidate.Length			= pairs[p].Distance;
					candidates.Add(candidate);
				}
			}
		}

		/*
		**	Q-024: the Section-5 number is a target.  Every hard-valid candidate has been enumerated, ranked and
		**	taken in order; when none remains the shortfall is recorded, and Phase 9 does not invent a crossing
		**	to make up the count.
		*/
		if (candidates.Count() == 0) {
			census->Have = b.Out->ReservedOverpassCount;
			WWDEBUG_SAY(("Hybrid Phase 9: OVERPASS_TARGET_SHORTAGE %d (%d reserved of a target of %d)\r\n",
							 b.Out->Overpass_Target_Shortage(),b.Out->ReservedOverpassCount,b.Out->RequestedOverpassTarget));
			break;
		}

		int best = 0;
		for (int i = 1; i < candidates.Count(); ++i) {
			const OverpassCandidate & left = candidates[best];
			const OverpassCandidate & right = candidates[i];
			if (right.Cost > left.Cost) continue;
			if (right.Cost == left.Cost) {
				if (right.NodeA > left.NodeA) continue;
				if ((right.NodeA == left.NodeA) && (right.NodeB > left.NodeB)) continue;
			}
			best = i;
		}

		const OverpassCandidate & chosen = candidates[best];
		Add_Edge(b,HYBRID_EDGE_CROSS_LINK,HYBRID_ROAD_SECONDARY_2,chosen.NodeA,chosen.NodeB,-1,true,0,0);
		b.Out->CrossLinks++;
		Insert_Shared_Overpass(b,chosen.HighwayEdge,chosen.HighwaySlot,b.Out->Edges.Count() - 1,0,chosen.Point);
	}
	census->Have = b.Out->ReservedOverpassCount;
	return true;
}


/***********************************************************************************************
**	The Phase 9 gate.
***********************************************************************************************/

static bool Family_Is_Connected(const TopologyBuilder & b,int family)
{
	const HybridBaseEntrance * gdi = b.Out->Find_Entrance(0,family);
	const HybridBaseEntrance * nod = b.Out->Find_Entrance(1,family);
	if ((gdi == nullptr) || (nod == nullptr)) return false;

	//	Walk this family's own trunk edges only: a family is a route, not a detour through another one.
	DynamicVectorClass<unsigned int> reached;
	reached.Add(gdi->NodeId);
	bool grew = true;

	while (grew) {

		grew = false;
		for (int e = 0; e < b.Out->Edges.Count(); ++e) {

			const HybridTopologyEdge & edge = b.Out->Edges[e];
			if (edge.Kind != HYBRID_EDGE_FAMILY_TRUNK) continue;
			if (edge.RouteFamily != family) continue;

			bool has_a = false;
			bool has_b = false;
			for (int i = 0; i < reached.Count(); ++i) {
				if (reached[i] == edge.NodeA) has_a = true;
				if (reached[i] == edge.NodeB) has_b = true;
			}
			if (has_a == has_b) continue;
			reached.Add(has_a ? edge.NodeB : edge.NodeA);
			grew = true;
		}
	}

	for (int i = 0; i < reached.Count(); ++i) {
		if (reached[i] == nod->NodeId) return true;
	}
	return false;
}


//	A unit vector, or zero.
static Vector2 Unit_Of(const Vector2 & v)
{
	const float length = v.Length();
	return (length > 0.0f) ? (v * (1.0f / length)) : Vector2(0.0f,0.0f);
}


/***********************************************************************************************
**	Step 15 (Section 12.6.4 / 12.6.8): highway access interchanges.
**
**	After step 14, every HIGHWAY_4 pass-through family waypoint with PRIMARY_2/SECONDARY_2 arrivals is an anchor
**	candidate.  Each is framed, its arrivals sided, its form and crossing mode decided, and its gore candidates
**	k = 0..K_GORE-1 tested in order against 12.6.8 on intent geometry, Phase-6 vectors, Phase-7 terrain and the
**	frozen Phase-9 records; the first hard-valid one is frozen as a HIGHWAY_INTERCHANGE with its local junctions,
**	ramps and crossing.  A ramp is never given its own bridge, cut or pass: a candidate whose ramp, gore
**	reserve, local junction or crossing would need one is refused here, never left for Phase 11.  Phase 9 proves
**	the frozen pose realizable with the shared 5W ramp constructor; Phase 10 builds the routed ramp.
***********************************************************************************************/

//	One heavy family's highway chain: its stops in travel order from the GDI entrance, with chain stations.
struct HighwayChain
{
	int									Family;
	DynamicVectorClass<ChainEntry>	Entries;
	DynamicVectorClass<float>			Station;
	DynamicVectorClass<unsigned int>	TrunkEdges;			// the family's trunk edge StableIds, in travel order
};


static void Build_Highway_Chain(const TopologyBuilder & b,int family,HighwayChain * out)
{
	out->Family = family;
	out->Entries.Delete_All();
	out->Station.Delete_All();
	out->TrunkEdges.Delete_All();
	for (int e = 0; e < b.Out->Edges.Count(); ++e) {
		const HybridTopologyEdge & edge = b.Out->Edges[e];
		if ((edge.Kind != HYBRID_EDGE_FAMILY_TRUNK) || (edge.RouteFamily != family)) continue;
		DynamicVectorClass<ChainEntry> chain;
		Edge_Chain(b,edge,&chain);
		for (int c = (out->Entries.Count() > 0) ? 1 : 0; c < chain.Count(); ++c) out->Entries.Add(chain[c]);
		out->TrunkEdges.Add(edge.StableId);
	}
	float station = 0.0f;
	for (int i = 0; i < out->Entries.Count(); ++i) {
		if (i > 0) station += (out->Entries[i].Position - out->Entries[i - 1].Position).Length();
		out->Station.Add(Hybrid_Quantize_Position(station));
	}
}


//	One arrival at an anchor: the edge, its side and the stop it departs X toward.
struct InterchangeArrival
{
	bool	operator == (const InterchangeArrival & that) const	{ return EdgeIndex == that.EdgeIndex; }
	bool	operator != (const InterchangeArrival & that) const	{ return !(*this == that); }

	int							EdgeIndex;
	HybridInterchangeSide	Side;
	bool							Access;				// PRIMARY_2 or SECONDARY_2
	Vector2						Beyond;				// the first stop beyond X along the edge
};


//	Everything one anchor's candidate tests need.
struct InterchangeAnchor
{
	const HighwayChain *								Chain;
	int													Index;				// X's index in Chain->Entries
	unsigned int										Node;
	Vector2												X;
	Vector2												Axis;					// a
	DynamicVectorClass<InterchangeArrival>		Arrivals;
	unsigned int										AccessSides;
	HybridInterchangeForm							Form;
	HybridInterchangeCrossingMode					Mode;
	HybridRoadClass									CrossingClass;
	float													LocalOverpass;		// d_Lov, grade-separated only
	bool													KeepsJunction;		// X keeps a conventional junction
	float													RampExtra;			// D-024's E for this anchor's ramps
};


/*
**	D-024: the `E` of every ramp an anchor would create.  Its ramps carry LIGHT and every access-side arrival's
**	capabilities (12.6.4 step 9), so their roadbed carries the widest of those vehicles.
*/
static float Anchor_Ramp_Extra(const TopologyBuilder & b,const InterchangeAnchor & a)
{
	unsigned int capabilities = HYBRID_NAV_LIGHT;
	for (int r = 0; r < a.Arrivals.Count(); ++r) {
		if ((a.AccessSides & (1u << a.Arrivals[r].Side)) == 0) continue;
		capabilities |= b.Out->Edges[a.Arrivals[r].EdgeIndex].Capabilities;
	}
	const HybridTopologyParams & p = *b.P;
	return Hybrid_Quantize_Position(Hybrid_Ramp_Roadbed_Extra(p.Constants.W,
																			  Hybrid_Required_Swept_Width(capabilities,p.LightVehicleWidth,
																														p.HeavyVehicleWidth,p.HarvesterWidth)));
}


static Vector2 Side_Tau(const InterchangeAnchor & a,int side)
{
	return (side == HYBRID_INTERCHANGE_SIDE_RIGHT) ? a.Axis : Vector2(-a.Axis.X,-a.Axis.Y);
}


static Vector2 Side_Normal(const InterchangeAnchor & a,int side)
{
	//	n_s is to the right of tau_s on both sides: n_RIGHT = rotate(a,-90), n_LEFT = rotate(a,+90).
	return (side == HYBRID_INTERCHANGE_SIDE_RIGHT) ? Vector2(a.Axis.Y,-a.Axis.X) : Vector2(-a.Axis.Y,a.Axis.X);
}


static Vector2 Side_World(const InterchangeAnchor & a,int side,const Vector2 & local)
{
	return a.X + (Side_Tau(a,side) * local.X) + (Side_Normal(a,side) * local.Y);
}


//	Chain station of side-frame `x`: for RIGHT the station is `x`, for LEFT it is `-x` (12.6.7), about X.
static float Side_Station(const InterchangeAnchor & a,int side,float x)
{
	return a.Chain->Station[a.Index] + ((side == HYBRID_INTERCHANGE_SIDE_RIGHT) ? x : -x);
}


//	L_ramp_plan(c) with Dz_plan(c, center, foot1, foot2) (12.6.7).
static float Ramp_Length_Plan(const TopologyBuilder & b,HybridRoadClass road_class,const Vector2 & center,
										const Vector2 & foot1,const Vector2 & foot2)
{
	const HybridConstants & k = b.P->Constants;
	float zc = 0.0f, z1 = 0.0f, z2 = 0.0f;
	Height_At(b,center,&zc);
	Height_At(b,foot1,&z1);
	Height_At(b,foot2,&z2);
	const float delta_z = b.P->OverpassClearGap + b.P->OverpassDepthMax[road_class] + Max2(0.0f,zc - Min2(z1,z2));
	const float grade = (float)tan((double)(Hybrid_Road_Class_Spec(road_class).MaxGradeDegrees * PI_OVER_180));
	return Max2(10.0f * k.W,(float)fabs((double)delta_z) / Max2(grade,k.Q));
}


//	The ramp exclusions of 12.6.8 test 5 at one world point.
static bool Ramp_Point_Is_Clear(const TopologyBuilder & b,const Vector2 & world)
{
	if (!Point_Is_Clear(b,world,false)) return false;
	for (int i = 0; i < b.Out->Entrances.Count(); ++i) {
		const HybridBaseEntrance & entrance = b.Out->Entrances[i];
		if (Distance_To_Segment_Point(world,entrance.CorridorA,entrance.CorridorB) <= entrance.CorridorHalfWidth) return false;
	}
	for (int i = 0; i < b.Anchors->Resources.Count(); ++i) {
		if ((world - b.Anchors->Resources[i].Center).Length() <= b.Anchors->Resources[i].ReserveRadius) return false;
	}
	return true;
}


//	Are two closed station intervals disjoint?
static bool Disjoint(float a0,float a1,float b0,float b1)
{
	return (a1 < b0) || (b1 < a0);
}


//	One ramp's gore reserve on the chain, and its gore station.
static void Gore_Reserve(const HybridInterchangeConstants & c,const InterchangeAnchor & a,int side,int role,
								 const HybridRampCandidate & cand,float * from,float * to,float * gore)
{
	float x0 = 0.0f, x1 = 0.0f;
	Hybrid_Gore_Reserve_Extent(c,cand.GoreOffset,role,&x0,&x1);
	const float xg = (role == HYBRID_RAMP_ROLE_ON) ? cand.GoreOffset : -cand.GoreOffset;
	const float s0 = Side_Station(a,side,x0);
	const float s1 = Side_Station(a,side,x1);
	*from	= Min2(s0,s1);
	*to	= Max2(s0,s1);
	*gore	= Side_Station(a,side,xg);
}


//	The ramp's highway-parallel interval, chain stations relative to X, with the Q-026 subdivisions.
static HybridRampInterval Ramp_Interval(const HybridInterchangeConstants & c,int side,int role,const HybridRampCandidate & cand)
{
	const float sign = (side == HYBRID_INTERCHANGE_SIDE_RIGHT) ? 1.0f : -1.0f;
	float g = 0.0f, f0 = 0.0f, f1 = 0.0f, t0 = 0.0f, t1 = 0.0f;
	if (role == HYBRID_RAMP_ROLE_ON) {
		g = cand.GoreOffset;		f0 = g;	f1 = g + c.MergeFullLength;	t0 = f1;	t1 = g + c.MergeLength;
	} else {
		g = -cand.GoreOffset;	t0 = g - c.DivergeLength;	t1 = g - c.DivergeFullLength;	f0 = t1;	f1 = g;
	}
	HybridRampInterval interval;
	interval.GoreStation	= Hybrid_Quantize_Position(sign * g);
	interval.FullFrom		= Hybrid_Quantize_Position(Min2(sign * f0,sign * f1));
	interval.FullTo		= Hybrid_Quantize_Position(Max2(sign * f0,sign * f1));
	interval.TaperFrom	= Hybrid_Quantize_Position(Min2(sign * t0,sign * t1));
	interval.TaperTo		= Hybrid_Quantize_Position(Max2(sign * t0,sign * t1));
	interval.From			= Min2(interval.FullFrom,interval.TaperFrom);
	interval.To				= Max2(interval.FullTo,interval.TaperTo);
	return interval;
}


//	Does the chain node at index `j` keep a conventional junction (an incident edge that is not the chain's own)?
static bool Chain_Node_Has_Junction(const TopologyBuilder & b,const HighwayChain & chain,int j)
{
	const unsigned int node = chain.Entries[j].NodeId;
	for (int e = 0; e < b.Out->Edges.Count(); ++e) {
		const HybridTopologyEdge & edge = b.Out->Edges[e];
		if (!edge.Is_Realized() || (edge.Kind == HYBRID_EDGE_HIGHWAY_RAMP) || (edge.Kind == HYBRID_EDGE_INTERCHANGE_CROSSING)) continue;
		if ((edge.NodeA != node) && (edge.NodeB != node)) continue;
		bool own = false;
		for (int t = 0; (t < chain.TrunkEdges.Count()) && !own; ++t) own = (chain.TrunkEdges[t] == edge.StableId);
		if (!own) return true;
	}
	return false;
}


/*
**	12.6.8 for candidate `k` of one anchor.  Returns -1 when hard-valid, else the failing test number (1..7) for
**	the diagnostics.  `skip` is an interchange StableId to leave out of the comparisons (the Phase-9 gate
**	re-checks a frozen interchange against every other one).  On success the ramp lengths (L_s to gore) are
**	returned in `ramp_length[side][role-1]`.
*/
static int Interchange_Candidate_Refusal(const TopologyBuilder & b,const HybridInterchangeConstants & c,
													  const InterchangeAnchor & a,const HybridRampCandidate & cand,
													  unsigned int skip,float ramp_length[2][2],int * diag = nullptr)
{
	const HybridConstants & k = b.P->Constants;
	const HighwayChain & chain = *a.Chain;
	const int i = a.Index;
	const int last = chain.Entries.Count() - 1;
	const float sx = chain.Station[i];
	const float spacing = Min2(k.W / 3.0f,0.25f * k.A);

	//	1. Straight extent and pose (Q-023's rule) and the chain's reach before each base entrance.
	HybridRoadPose pose;
	pose.Point	= a.X;
	pose.Axis	= a.Axis;
	pose.Before	= cand.StraightHalfExtent;
	pose.After	= cand.StraightHalfExtent;
	if (((sx - chain.Station[0]) < (cand.StraightHalfExtent + c.HighwayJunctionReserve)) ||
		 ((chain.Station[last] - sx) < (cand.StraightHalfExtent + c.HighwayJunctionReserve))) {
		if (diag != nullptr) diag[0]++;
		return 1;
	}
	{
		const HybridRoadPose from = Chain_Pose(b,chain.Entries,i - 1);
		Vector2 behind(0.0f,0.0f);
		const bool from_interior = !from.Is_Structural() && ((i - 1) > 0);
		if (from_interior) behind = Pose_Far_End(Chain_Pose(b,chain.Entries,i - 2),false);
		if (!Hybrid_Road_Poses_Connect(from,pose,HYBRID_ROAD_HIGHWAY_4,k,from_interior ? &behind : nullptr,nullptr)) {
			if (diag != nullptr) diag[1]++;
			return 1;
		}
		const HybridRoadPose to = Chain_Pose(b,chain.Entries,i + 1);
		Vector2 ahead(0.0f,0.0f);
		const bool to_interior = !to.Is_Structural() && ((i + 1) < last);
		if (to_interior) ahead = Pose_Far_End(Chain_Pose(b,chain.Entries,i + 2),true);
		if (!Hybrid_Road_Poses_Connect(pose,to,HYBRID_ROAD_HIGHWAY_4,k,nullptr,to_interior ? &ahead : nullptr)) {
			if (diag != nullptr) diag[2]++;
			return 1;
		}
	}

	//	2. Structure reserves along the chain.
	const float lo = sx - cand.StraightHalfExtent;
	const float hi = sx + cand.StraightHalfExtent;
	for (int j = 0; j <= last; ++j) {
		if (j == i) continue;
		const ChainEntry & entry = chain.Entries[j];
		const float sj = chain.Station[j];
		if ((entry.Kind == HYBRID_NODE_CROSSING_OBLIGATION) && (entry.WindowId != 0)) {
			const HybridCrossingWindow * window = b.Hydrology->Find_Window(entry.WindowId);
			const float reserve = ((window != nullptr) ? (0.5f * window->Span) : 0.0f) +
										 Max2(8.0f * k.W,b.P->BridgeApproachMax[HYBRID_ROAD_HIGHWAY_4]);
			if (!Disjoint(lo,hi,sj - reserve,sj + reserve)) return 2;
		} else if (entry.Kind == HYBRID_NODE_OVERPASS_OBLIGATION) {
			HybridRoadClass other_class = HYBRID_ROAD_SECONDARY_2;
			for (int e = 0; e < b.Out->Edges.Count(); ++e) {
				const HybridTopologyEdge & other = b.Out->Edges[e];
				if (other.RouteFamily == chain.Family) continue;
				for (int w = 0; w < other.Waypoints.Count(); ++w) {
					if (other.Waypoints[w] == entry.NodeId) other_class = other.RoadClass;
				}
			}
			const Vector2 along = Unit_Of(chain.Entries[(j < last) ? (j + 1) : j].Position - chain.Entries[(j > 0) ? (j - 1) : j].Position);
			const float reserve = (0.5f * Hybrid_Roadbed_Width(other_class,k.W)) +
										 Ramp_Length_Plan(b,HYBRID_ROAD_HIGHWAY_4,entry.Position,entry.Position - (along * (10.0f * k.W)),
															  entry.Position + (along * (10.0f * k.W)));
			if (!Disjoint(lo,hi,sj - reserve,sj + reserve)) return 2;
		} else if ((j > 0) && (j < last) && Chain_Node_Has_Junction(b,chain,j)) {
			if (!Disjoint(lo,hi,sj - c.HighwayJunctionReserve,sj + c.HighwayJunctionReserve)) return 2;
		}
	}
	for (int n = 0; n < b.Out->Interchanges.Count(); ++n) {
		const HybridInterchange & other = b.Out->Interchanges[n];
		if ((other.StableId == skip) || (other.Status != HYBRID_INTERCHANGE_ACTIVE)) continue;
		for (int j = 0; j <= last; ++j) {
			if (chain.Entries[j].NodeId != other.AnchorNode) continue;
			if (!Disjoint(lo,hi,chain.Station[j] - other.StraightHalfExtent,chain.Station[j] + other.StraightHalfExtent)) return 2;
		}
	}
	const float own_overpass = (a.Mode == HYBRID_INTERCHANGE_CROSSING_GRADE_SEPARATED)
										? ((0.5f * Hybrid_Roadbed_Width(a.CrossingClass,k.W)) +
											Ramp_Length_Plan(b,HYBRID_ROAD_HIGHWAY_4,a.X,a.X - (a.Axis * (10.0f * k.W)),a.X + (a.Axis * (10.0f * k.W))))
										: 0.0f;
	for (int side = 0; side < HYBRID_INTERCHANGE_SIDE_COUNT; ++side) {
		if ((a.AccessSides & (1u << side)) == 0) continue;
		for (int role = HYBRID_RAMP_ROLE_ON; role <= HYBRID_RAMP_ROLE_OFF; ++role) {
			float g0 = 0.0f, g1 = 0.0f, gore = 0.0f;
			Gore_Reserve(c,a,side,role,cand,&g0,&g1,&gore);
			if ((own_overpass > 0.0f) && !Disjoint(g0,g1,sx - own_overpass,sx + own_overpass)) return 2;
			if (a.KeepsJunction && !Disjoint(g0,g1,sx - c.HighwayJunctionReserve,sx + c.HighwayJunctionReserve)) return 2;

			//	3. Spacing against every gore of every accepted interchange on this chain.
			for (int n = 0; n < b.Out->Interchanges.Count(); ++n) {
				const HybridInterchange & other = b.Out->Interchanges[n];
				if ((other.StableId == skip) || (other.Status != HYBRID_INTERCHANGE_ACTIVE)) continue;
				for (int e = 0; e < other.RampEdges.Count(); ++e) {
					const HybridTopologyEdge * ramp = b.Out->Find_Edge(other.RampEdges[e]);
					if (ramp == nullptr) continue;
					for (int j = 0; j <= last; ++j) {
						if (chain.Entries[j].NodeId != other.AnchorNode) continue;
						const float other_gore = chain.Station[j] + ramp->RampInterval.GoreStation;
						const float other_from = chain.Station[j] + ramp->RampInterval.From - c.GoreReserveMargin;
						const float other_to = chain.Station[j] + ramp->RampInterval.To + c.GoreReserveMargin;
						if ((float)fabs((double)(gore - other_gore)) < c.InterchangeGoreSpacing) return 3;
						if (!Disjoint(g0,g1,other_from,other_to)) return 3;
					}
				}
			}
		}
	}

	//	5, 6. Every ramp: built by the shared 5W constructor, clear of every exclusion, within 12 degrees.
	const float max_grade = Hybrid_Road_Class_Spec(HYBRID_ROAD_HIGHWAY_RAMP).MaxGradeDegrees * PI_OVER_180;
	for (int side = 0; side < HYBRID_INTERCHANGE_SIDE_COUNT; ++side) {
		if ((a.AccessSides & (1u << side)) == 0) continue;
		for (int role = HYBRID_RAMP_ROLE_ON; role <= HYBRID_RAMP_ROLE_OFF; ++role) {
			Vector2 polygon[4];
			Hybrid_Ramp_Control_Polygon(c,cand,role,polygon);
			DynamicVectorClass<Vector2> points;
			DynamicVectorClass<float> stations;
			if (!Hybrid_Ramp_Centerline(polygon,c.RampRadius,k.Q,spacing,&points,&stations)) return 5;
			ramp_length[side][role - 1] = stations[stations.Count() - 1];
			const int gore_end = (role == HYBRID_RAMP_ROLE_ON) ? (points.Count() - 1) : 0;

			HybridVerticalInterval previous_interval;
			for (int p = 0; p < points.Count(); ++p) {
				const Vector2 world = Side_World(a,side,points[p]);
				//	Travel direction in the world, and the roadbed edges about the fixed RampLaneCenterline (D-022).
				const int q = ((p + 1) < points.Count()) ? (p + 1) : p;
				const int r = ((p + 1) < points.Count()) ? p : (p - 1);
				const Vector2 travel = Unit_Of(Side_World(a,side,points[q]) - Side_World(a,side,points[r]));
				if (travel.Length() <= 0.0f) continue;
				const Vector2 left(-travel.Y,travel.X);
				const float from_gore = (float)fabs((double)(stations[p] - stations[gore_end]));
				float inner = 0.0f, outer = 0.0f;
				Hybrid_Ramp_Effective_Edges(c,a.RampExtra,from_gore,&inner,&outer);
				const Vector2 inner_edge = world + (left * inner);
				const Vector2 outer_edge = world - (left * outer);
				if (!Ramp_Point_Is_Clear(b,world) || !Ramp_Point_Is_Clear(b,inner_edge) || !Ramp_Point_Is_Clear(b,outer_edge)) return 5;

				float z = 0.0f;
				if (!Height_At(b,world,&z)) return 5;
				const HybridVerticalInterval interval = Hybrid_Road_Vertical_Interval(z,b.P->MaxCutDepth,b.P->MaxFillHeight);
				if ((p > 0) && !Hybrid_Grade_Compatible(previous_interval,interval,stations[p] - stations[p - 1],max_grade)) return 6;
				previous_interval = interval;
			}

			//	The gore reserve's widened roadbed on the highway (the band the auxiliary lane adds).
			float g0 = 0.0f, g1 = 0.0f, gore = 0.0f;
			Gore_Reserve(c,a,side,role,cand,&g0,&g1,&gore);
			for (float s = g0; s <= g1; s += spacing) {
				const float x = (side == HYBRID_INTERCHANGE_SIDE_RIGHT) ? (s - sx) : (sx - s);
				const float along = (role == HYBRID_RAMP_ROLE_ON) ? (x - cand.GoreOffset) : (x + cand.GoreOffset + c.DivergeLength);
				const float span = (role == HYBRID_RAMP_ROLE_ON) ? c.MergeLength : c.DivergeLength;
				HybridAuxiliaryLaneSection section;
				Hybrid_Auxiliary_Lane_Section(c,role,Clamp2(along,0.0f,span),&section);
				for (int lateral = 0; lateral < 3; ++lateral) {
					const float y = c.EHwy + ((section.E - c.EHwy) * (0.5f * (float)lateral));
					if (!Ramp_Point_Is_Clear(b,Side_World(a,side,Vector2(x,y)))) return 5;
				}
			}
		}
	}

	/*
	**	5, 7. Local junctions.  The disc is clear (5).  Turn room (7): every pair of arms is at least 30 degrees apart.
	**	Reachability (7, D-023, replacing the 60-degree stem cone): each arrival must be able to leave L_s and reach
	**	its next stop without driving through the interchange -- its stem may not start inside the footprint wedge
	**	between the OFF and ON ramp arms (Phase 10 makes the footprint hard-infinite), and its intent leg from L_s
	**	may not cross either ramp (away from the shared arm at L_s), the highway's straight extent or the crossing.
	**	A nearly parallel feeder is therefore legal whenever that geometry works; no arrival angle is imposed.
	*/
	for (int side = 0; side < HYBRID_INTERCHANGE_SIDE_COUNT; ++side) {
		if ((a.AccessSides & (1u << side)) == 0) continue;
		const Vector2 local = Side_World(a,side,Vector2(0.0f,cand.LocalOffset));
		const Vector2 normal = Side_Normal(a,side);
		float widest = Hybrid_Roadbed_Width(HYBRID_ROAD_HIGHWAY_RAMP,k.W) + (2.0f * a.RampExtra);
		DynamicVectorClass<Vector2> arms;
		Vector2 polygon[4];
		Hybrid_Ramp_Control_Polygon(c,cand,HYBRID_RAMP_ROLE_ON,polygon);
		arms.Add(Unit_Of(Side_World(a,side,polygon[1]) - local));
		Hybrid_Ramp_Control_Polygon(c,cand,HYBRID_RAMP_ROLE_OFF,polygon);
		arms.Add(Unit_Of(Side_World(a,side,polygon[2]) - local));
		if (a.Form == HYBRID_INTERCHANGE_DIAMOND_ACCESS) {
			arms.Add(Vector2(-normal.X,-normal.Y));
			widest = Max2(widest,Hybrid_Roadbed_Width(a.CrossingClass,k.W));
		}
		//	The ramps' own polylines, for the reachability test.
		DynamicVectorClass<Vector2> ramp_lines[2];
		for (int role = HYBRID_RAMP_ROLE_ON; role <= HYBRID_RAMP_ROLE_OFF; ++role) {
			Vector2 ramp_polygon[4];
			Hybrid_Ramp_Control_Polygon(c,cand,role,ramp_polygon);
			DynamicVectorClass<float> ramp_stations;
			DynamicVectorClass<Vector2> ramp_points;
			if (Hybrid_Ramp_Centerline(ramp_polygon,c.RampRadius,k.Q,Min2(k.W / 3.0f,0.25f * k.A),&ramp_points,&ramp_stations)) {
				for (int q = 0; q < ramp_points.Count(); ++q) ramp_lines[role - 1].Add(Side_World(a,side,ramp_points[q]));
			}
		}
		const Vector2 highway_from = a.X - (a.Axis * cand.StraightHalfExtent);
		const Vector2 highway_to = a.X + (a.Axis * cand.StraightHalfExtent);
		const Vector2 far_local = Side_World(a,(side == HYBRID_INTERCHANGE_SIDE_RIGHT) ? HYBRID_INTERCHANGE_SIDE_LEFT
																														: HYBRID_INTERCHANGE_SIDE_RIGHT,
														 Vector2(0.0f,cand.LocalOffset));
		for (int r = 0; r < a.Arrivals.Count(); ++r) {
			const InterchangeArrival & arrival = a.Arrivals[r];
			if (arrival.Side != side) continue;
			const Vector2 stem = Unit_Of(arrival.Beyond - local);
			if (Wrapped_Angle_Difference(stem,Vector2(-normal.X,-normal.Y)) < ((45.0f * PI_OVER_180) + HYBRID_QUANTUM_ANGLE)) return 7;
			if (Segments_Cross(local,arrival.Beyond,highway_from,highway_to,nullptr,nullptr,nullptr)) return 7;
			if ((a.Form == HYBRID_INTERCHANGE_DIAMOND_ACCESS) &&
				 Segments_Cross(local,arrival.Beyond,local,far_local,nullptr,nullptr,nullptr)) return 7;
			for (int line = 0; line < 2; ++line) {
				for (int q = 1; q < ramp_lines[line].Count(); ++q) {
					if (((ramp_lines[line][q - 1] - local).Length() <= c.ArmLength) &&
						 ((ramp_lines[line][q] - local).Length() <= c.ArmLength)) continue;
					if (Segments_Cross(local,arrival.Beyond,ramp_lines[line][q - 1],ramp_lines[line][q],nullptr,nullptr,nullptr)) return 7;
				}
			}
			arms.Add(stem);
			widest = Max2(widest,Hybrid_Roadbed_Width(b.Out->Edges[arrival.EdgeIndex].RoadClass,k.W));
		}
		for (int p = 0; p < arms.Count(); ++p) {
			for (int q = p + 1; q < arms.Count(); ++q) {
				if (Wrapped_Angle_Difference(arms[p],arms[q]) < ((30.0f * PI_OVER_180) - HYBRID_QUANTUM_ANGLE)) return 7;
			}
		}
		const float disc = (0.5f * widest) + c.ArmLength;
		if (!Ramp_Point_Is_Clear(b,local)) return 5;
		for (int ring = 1; ring <= 2; ++ring) {
			for (int d = 0; d < 16; ++d) {
				const float angle = (float)d * ((360.0f / 16.0f) * PI_OVER_180);
				const Vector2 point = local + (Vector2((float)cos((double)angle),(float)sin((double)angle)) * (disc * 0.5f * (float)ring));
				if (!Ramp_Point_Is_Clear(b,point)) return 5;
			}
		}
	}

	//	5, 6. The DIAMOND crossing segment: clear, and at grade within its class grade.
	if (a.Form == HYBRID_INTERCHANGE_DIAMOND_ACCESS) {
		const Vector2 right = Side_World(a,HYBRID_INTERCHANGE_SIDE_RIGHT,Vector2(0.0f,cand.LocalOffset));
		const Vector2 left = Side_World(a,HYBRID_INTERCHANGE_SIDE_LEFT,Vector2(0.0f,cand.LocalOffset));
		const float length = (left - right).Length();
		const Vector2 along = Unit_Of(left - right);
		for (float s = 0.0f; s <= length; s += spacing) {
			if (!Ramp_Point_Is_Clear(b,right + (along * s))) return 5;
		}
		if ((a.Mode == HYBRID_INTERCHANGE_CROSSING_AT_GRADE) && !Run_Within_Grade(b,right,left,a.CrossingClass)) return 6;
	}
	return -1;
}


//	The quantized graph length of an edge through its waypoints.
static float Edge_Graph_Length(const TopologyBuilder & b,const HybridTopologyEdge & edge)
{
	float length = 0.0f;
	for (int s = 0; s < Edge_Segment_Count(edge); ++s) {
		Vector2 from(0.0f,0.0f), to(0.0f,0.0f);
		if (Edge_Segment(b,edge,s,&from,&to)) length += (to - from).Length();
	}
	return Hybrid_Quantize_Position(length);
}


//	The first stop beyond `node` along `edge`, which ends at `node`.
static Vector2 Stop_Beyond(const TopologyBuilder & b,const HybridTopologyEdge & edge,unsigned int node)
{
	unsigned int next = 0;
	if (edge.NodeA == node)	next = (edge.Waypoints.Count() > 0) ? edge.Waypoints[0] : edge.NodeB;
	else							next = (edge.Waypoints.Count() > 0) ? edge.Waypoints[edge.Waypoints.Count() - 1] : edge.NodeA;
	const HybridTopologyNode * n = b.Out->Find_Node(next);
	return (n != nullptr) ? n->Position : Vector2(0.0f,0.0f);
}


/*
**	Phase-11's at-grade junction conditions on intent geometry and Phase-7 terrain (12.6.4 step 4): the crossing
**	meets the straight highway at a right angle by construction; outside every crossing-window approach reserve;
**	arms of max(4W, class radius) on both roads; the vertical correction from X to every arm end inside the
**	grading limits at the arm's class grade; and the cross slope at X within both classes.
*/
static bool At_Grade_Crossing_Is_Feasible(const TopologyBuilder & b,const InterchangeAnchor & a,float local_offset)
{
	const HybridConstants & k = b.P->Constants;
	const HybridRoadClass classes[2] = { HYBRID_ROAD_HIGHWAY_4,a.CrossingClass };
	const Vector2 axes[2] = { a.Axis,Side_Normal(a,HYBRID_INTERCHANGE_SIDE_LEFT) };

	//	D-023: the at-grade angle (30..150 degrees) is the crossing edge's own, L_RIGHT -> L_LEFT, against the highway.
	const float angle = Wrapped_Angle_Difference(axes[0],axes[1]) * (180.0f / 3.14159265358979323846f);
	if ((angle < (30.0f - 0.001f)) || (angle > (150.0f + 0.001f))) return false;

	for (int i = 0; i < b.SelectedWindow.Count(); ++i) {
		const HybridCrossingWindow * window = b.Hydrology->Find_Window(b.SelectedWindow[i]);
		if (window == nullptr) continue;
		if (Distance_To_Segment_Point(a.X,window->ApproachA,window->ApproachB) <=
			 (0.5f * Hybrid_Roadbed_Width(HYBRID_ROAD_HIGHWAY_4,k.W))) return false;
	}
	float z = 0.0f;
	if (!Height_At(b,a.X,&z)) return false;
	const HybridVerticalInterval centre = Hybrid_Road_Vertical_Interval(z,b.P->MaxCutDepth,b.P->MaxFillHeight);
	for (int r = 0; r < 2; ++r) {
		const HybridRoadClassSpec & spec = Hybrid_Road_Class_Spec(classes[r]);
		const float arm = Max2(4.0f * k.W,spec.MinTurnRadiusW * k.W);
		if ((r == 1) && (local_offset < arm)) return false;
		for (int sign = -1; sign <= 1; sign += 2) {
			float end = 0.0f;
			if (!Height_At(b,a.X + (axes[r] * (arm * (float)sign)),&end)) return false;
			const HybridVerticalInterval there = Hybrid_Road_Vertical_Interval(end,b.P->MaxCutDepth,b.P->MaxFillHeight);
			if (!Hybrid_Grade_Compatible(centre,there,arm,spec.MaxGradeDegrees * PI_OVER_180)) return false;
		}
		const Vector2 across(-axes[r].Y,axes[r].X);
		const float half = 0.5f * Hybrid_Roadbed_Width(classes[r],k.W);
		float left = 0.0f, right = 0.0f;
		if (!Height_At(b,a.X + (across * half),&left) || !Height_At(b,a.X - (across * half),&right)) return false;
		if (!Hybrid_Grade_Compatible(Hybrid_Road_Vertical_Interval(left,b.P->MaxCutDepth,b.P->MaxFillHeight),
											  Hybrid_Road_Vertical_Interval(right,b.P->MaxCutDepth,b.P->MaxFillHeight),2.0f * half,
											  spec.MaxCrossSlopeDegrees * PI_OVER_180)) return false;
	}
	return true;
}


/*
**	Frames anchor `index` of `chain` (12.6.4 steps 1-3) and decides its crossing (step 4).  Returns -1 when the
**	anchor goes on to its candidates, a HybridInterchangeRejection otherwise, or -2 when `index` is no anchor.
*/
static int Frame_Anchor(const TopologyBuilder & b,const HybridInterchangeConstants & c,const HighwayChain & chain,int index,
								InterchangeAnchor * out)
{
	const HybridConstants & k = b.P->Constants;
	const ChainEntry & entry = chain.Entries[index];
	if ((index <= 0) || (index >= (chain.Entries.Count() - 1)) || (entry.Kind != HYBRID_NODE_FAMILY_WAYPOINT)) return -2;

	out->Chain				= &chain;
	out->Index				= index;
	out->Node				= entry.NodeId;
	out->X					= entry.Position;
	out->Arrivals.Delete_All();
	out->AccessSides		= 0;
	out->Mode				= HYBRID_INTERCHANGE_CROSSING_NONE;
	out->CrossingClass	= HYBRID_ROAD_CLASS_COUNT;
	out->LocalOverpass	= 0.0f;
	out->KeepsJunction	= false;

	int highway_edges = 0;
	bool any_access = false;
	for (int e = 0; e < b.Out->Edges.Count(); ++e) {
		const HybridTopologyEdge & edge = b.Out->Edges[e];
		if ((edge.NodeA != entry.NodeId) && (edge.NodeB != entry.NodeId)) continue;
		if ((edge.Kind == HYBRID_EDGE_FAMILY_TRUNK) && (edge.RouteFamily == chain.Family)) {
			if (edge.RoadClass == HYBRID_ROAD_HIGHWAY_4) highway_edges++;
			continue;
		}
		if (!edge.Is_Realized() || (edge.Kind == HYBRID_EDGE_HIGHWAY_RAMP) || (edge.Kind == HYBRID_EDGE_INTERCHANGE_CROSSING)) continue;
		if ((edge.RoadClass == HYBRID_ROAD_PRIMARY_2) || (edge.RoadClass == HYBRID_ROAD_SECONDARY_2)) any_access = true;
	}
	if ((highway_edges != 2) || !any_access) return -2;

	//	2. Frame.
	const Vector2 p = chain.Entries[index - 1].Position;
	const Vector2 n = chain.Entries[index + 1].Position;
	const Vector2 sum = Unit_Of(out->X - p) + Unit_Of(n - out->X);
	if (sum.Length() < k.Q) return HYBRID_INTERCHANGE_REJECT_AXIS;
	out->Axis = Unit_Of(sum);

	//	3. Arrivals and sides.
	for (int e = 0; e < b.Out->Edges.Count(); ++e) {
		const HybridTopologyEdge & edge = b.Out->Edges[e];
		if ((edge.NodeA != entry.NodeId) && (edge.NodeB != entry.NodeId)) continue;
		if ((edge.Kind == HYBRID_EDGE_FAMILY_TRUNK) && (edge.RouteFamily == chain.Family)) continue;
		if (!edge.Is_Realized() || (edge.Kind == HYBRID_EDGE_HIGHWAY_RAMP) || (edge.Kind == HYBRID_EDGE_INTERCHANGE_CROSSING)) continue;

		InterchangeArrival arrival;
		arrival.EdgeIndex	= e;
		arrival.Beyond		= Stop_Beyond(b,edge,entry.NodeId);
		arrival.Access		= (edge.RoadClass == HYBRID_ROAD_PRIMARY_2) || (edge.RoadClass == HYBRID_ROAD_SECONDARY_2);
		/*
		**	The side is the sign of cross(a, delta), ties to RIGHT.  The arrival's direction decides nothing else
		**	(D-023): there is no minimum arrival-vs-highway angle; whether the arrival can reach its local junction
		**	is test 7's geometry, judged per candidate.
		*/
		const Vector2 delta = Unit_Of(arrival.Beyond - out->X);
		const float cross = (out->Axis.X * delta.Y) - (out->Axis.Y * delta.X);
		arrival.Side = (cross > 0.0f) ? HYBRID_INTERCHANGE_SIDE_LEFT : HYBRID_INTERCHANGE_SIDE_RIGHT;
		if (arrival.Access) {
			for (int i = 0; i < b.Out->Interchanges.Count(); ++i) {
				for (int r = 0; r < b.Out->Interchanges[i].Retargets.Count(); ++r) {
					if (b.Out->Interchanges[i].Retargets[r].EdgeId == edge.StableId) return HYBRID_INTERCHANGE_REJECT_ARRIVAL_SERVED;
				}
			}
			out->AccessSides |= (1u << arrival.Side);
		}
		out->Arrivals.Add(arrival);
	}
	out->Form = (out->AccessSides == 3u) ? HYBRID_INTERCHANGE_DIAMOND_ACCESS : HYBRID_INTERCHANGE_SIDE_ACCESS;
	out->RampExtra = Anchor_Ramp_Extra(b,*out);

	//	A non-access side keeps its arrivals at X, and with them X's conventional junction.
	for (int r = 0; r < out->Arrivals.Count(); ++r) {
		if ((out->AccessSides & (1u << out->Arrivals[r].Side)) == 0) out->KeepsJunction = true;
	}

	//	4. The DIAMOND crossing: grade-separated when the step-12 overpass tests pass, else at grade, else none.
	if (out->Form == HYBRID_INTERCHANGE_DIAMOND_ACCESS) {
		out->CrossingClass = HYBRID_ROAD_SECONDARY_2;
		for (int r = 0; r < out->Arrivals.Count(); ++r) {
			if (out->Arrivals[r].Access && (b.Out->Edges[out->Arrivals[r].EdgeIndex].RoadClass == HYBRID_ROAD_PRIMARY_2)) {
				out->CrossingClass = HYBRID_ROAD_PRIMARY_2;
			}
		}
		HybridRampCandidate k0;
		Hybrid_Ramp_Candidate(c,0,0.0f,&k0);
		const Vector2 foot_right = Side_World(*out,HYBRID_INTERCHANGE_SIDE_RIGHT,Vector2(0.0f,k0.LocalOffsetMin));
		const Vector2 foot_left = Side_World(*out,HYBRID_INTERCHANGE_SIDE_LEFT,Vector2(0.0f,k0.LocalOffsetMin));
		/*
		**	d_Lov: the deck reaches past the highway's paved roadbed by the crossing road's outer blend and L_pin (D-027),
		**	exactly as Phase 11 builds it, then L_ramp and the arm.
		*/
		const float overpass_offset = c.EHwy + Hybrid_Overpass_Deck_Allowance(k.W,k.A,b.P->TerrainCellSize) +
												Ramp_Length_Plan(b,out->CrossingClass,out->X,foot_right,foot_left) + c.ArmLength;
		HybridRampCandidate grade_separated;
		Hybrid_Ramp_Candidate(c,0,overpass_offset,&grade_separated);
		const Vector2 right = Side_World(*out,HYBRID_INTERCHANGE_SIDE_RIGHT,Vector2(0.0f,grade_separated.LocalOffset));
		const Vector2 left = Side_World(*out,HYBRID_INTERCHANGE_SIDE_LEFT,Vector2(0.0f,grade_separated.LocalOffset));
		float cost = 0.0f;
		if (Overpass_Cost(b,out->X,out->X - (out->Axis * grade_separated.StraightHalfExtent),
								out->X + (out->Axis * grade_separated.StraightHalfExtent),right,left,&cost) < 0) {
			out->Mode				= HYBRID_INTERCHANGE_CROSSING_GRADE_SEPARATED;
			out->LocalOverpass	= overpass_offset;
		} else if (At_Grade_Crossing_Is_Feasible(b,*out,k0.LocalOffset)) {
			out->Mode				= HYBRID_INTERCHANGE_CROSSING_AT_GRADE;
			out->KeepsJunction	= true;
		} else {
			return HYBRID_INTERCHANGE_REJECT_CROSSING;
		}
	}
	return -1;
}


static bool Build_Interchanges(TopologyBuilder & b)
{
	const HybridConstants & k = b.P->Constants;
	HybridInterchangeConstants c;
	Hybrid_Interchange_Constants(k.W,&c);

	//	Interchange processing order: highway family, then X's index along its chain from the GDI entrance.
	for (int family = 0; family < b.FamilyCount; ++family) {

		HighwayChain chain;
		Build_Highway_Chain(b,family,&chain);
		if ((chain.Entries.Count() < 3) || (chain.TrunkEdges.Count() == 0)) continue;
		const HybridTopologyEdge * first = b.Out->Find_Edge(chain.TrunkEdges[0]);
		if ((first == nullptr) || (first->RoadClass != HYBRID_ROAD_HIGHWAY_4)) continue;

		for (int index = 1; index < (chain.Entries.Count() - 1); ++index) {

			InterchangeAnchor anchor;
			const int framed = Frame_Anchor(b,c,chain,index,&anchor);
			if (framed == -2) continue;
			if (framed >= 0) {
				b.Out->AnchorRejections[framed]++;
				continue;
			}

			b.Out->AnchorsByForm[anchor.Form]++;
			{
				const Vector2 before = chain.Entries[index - 1].Position;
				const Vector2 after = chain.Entries[index + 1].Position;
				b.Out->AnchorTurnDegrees.Add(Wrapped_Angle_Difference(Unit_Of(anchor.X - before),Unit_Of(after - anchor.X)) *
													  (180.0f / 3.14159265358979323846f));
				b.Out->AnchorNeighbourBefore.Add(((anchor.X - before).X * anchor.Axis.X) + ((anchor.X - before).Y * anchor.Axis.Y));
				b.Out->AnchorNeighbourAfter.Add(((after - anchor.X).X * anchor.Axis.X) + ((after - anchor.X).Y * anchor.Axis.Y));
			}

			//	5. Candidates in ascending k; the first hard-valid one is selected.
			int selected = -1;
			HybridRampCandidate cand;
			float ramp_length[2][2] = { { 0.0f,0.0f },{ 0.0f,0.0f } };
			for (int kk = 0; (kk < c.GoreCandidates) && (selected < 0); ++kk) {
				b.Out->RampCandidatesEvaluated++;
				Hybrid_Ramp_Candidate(c,kk,anchor.LocalOverpass,&cand);
				const int refusal = Interchange_Candidate_Refusal(b,c,anchor,cand,0,ramp_length,b.Out->StraightExtentRefusals);
				if (refusal < 0)						selected = kk;
				else if (refusal < 8)				b.Out->CandidateRefusals[refusal]++;
			}
			if (selected < 0) {
				b.Out->AnchorRejections[HYBRID_INTERCHANGE_REJECT_NO_CANDIDATE]++;
				continue;
			}

			//	6. The Section-5 qualification recheck of every retargeted arrival, before anything changes.
			bool qualifies = true;
			for (int r = 0; r < anchor.Arrivals.Count(); ++r) {
				const InterchangeArrival & arrival = anchor.Arrivals[r];
				if ((anchor.AccessSides & (1u << arrival.Side)) == 0) continue;
				const HybridTopologyEdge & moved = b.Out->Edges[arrival.EdgeIndex];
				if (moved.Kind != HYBRID_EDGE_TRAIL) continue;
				const Vector2 local = Side_World(anchor,arrival.Side,Vector2(0.0f,cand.LocalOffset));
				float length = 0.0f;
				for (int s = 0; s < Edge_Segment_Count(moved); ++s) {
					Vector2 from(0.0f,0.0f), to(0.0f,0.0f);
					if (!Edge_Segment(b,moved,s,&from,&to)) continue;
					if ((s == 0) && (moved.NodeA == anchor.Node)) from = local;
					if ((s == (Edge_Segment_Count(moved) - 1)) && (moved.NodeB == anchor.Node)) to = local;
					length += (to - from).Length();
				}
				if (Hybrid_Quantize_Position(length) < (TRAIL_MIN_W * k.W)) qualifies = false;
			}
			if (!qualifies) {
				b.Out->AnchorRejections[HYBRID_INTERCHANGE_REJECT_QUALIFICATION]++;
				continue;
			}

			//	7. The routed-edge cap.
			const int sides = (anchor.Form == HYBRID_INTERCHANGE_DIAMOND_ACCESS) ? 2 : 1;
			const int crossing_edges = (anchor.Mode == HYBRID_INTERCHANGE_CROSSING_GRADE_SEPARATED) ? 1 :
												((anchor.Mode == HYBRID_INTERCHANGE_CROSSING_AT_GRADE) ? 2 : 0);
			if ((b.Out->Realized_Edge_Count() + (2 * sides) + crossing_edges) > b.P->MaxRoutedEdges) {
				b.Out->AnchorRejections[HYBRID_INTERCHANGE_REJECT_EDGE_CAP]++;
				continue;
			}

			//	8. Records, with fresh StableIds in the plan's order.
			HybridInterchange interchange;
			interchange.StableId				= (unsigned int)b.Out->Interchanges.Count() + 1;
			interchange.Form					= anchor.Form;
			interchange.Status				= HYBRID_INTERCHANGE_ACTIVE;
			interchange.AnchorNode			= anchor.Node;
			for (int t = 0; t < chain.TrunkEdges.Count(); ++t) {
				const HybridTopologyEdge * trunk = b.Out->Find_Edge(chain.TrunkEdges[t]);
				if (trunk == nullptr) continue;
				if (trunk->NodeB == anchor.Node) interchange.HighwayEdgeIn = trunk->StableId;
				if (trunk->NodeA == anchor.Node) interchange.HighwayEdgeOut = trunk->StableId;
			}
			interchange.Axis					= anchor.Axis;
			interchange.AccessSides			= anchor.AccessSides;
			interchange.CrossingMode		= anchor.Mode;
			interchange.CrossingClass		= anchor.CrossingClass;
			interchange.SelectedK			= selected;
			interchange.LocalOffset			= cand.LocalOffset;
			interchange.GoreOffset			= cand.GoreOffset;
			interchange.StraightHalfExtent= cand.StraightHalfExtent;

			for (int side = 0; side < HYBRID_INTERCHANGE_SIDE_COUNT; ++side) {
				if ((anchor.AccessSides & (1u << side)) == 0) continue;
				interchange.LocalJunction[side] = Add_Node(b,HYBRID_NODE_INTERCHANGE_LOCAL_JUNCTION,
																		 Side_World(anchor,side,Vector2(0.0f,cand.LocalOffset)),-1,-1,0,0,-1);
			}

			//	9. Capabilities: LIGHT and every retargeted arrival's own.
			unsigned int capabilities = HYBRID_NAV_LIGHT;
			for (int r = 0; r < anchor.Arrivals.Count(); ++r) {
				const InterchangeArrival & arrival = anchor.Arrivals[r];
				if ((anchor.AccessSides & (1u << arrival.Side)) == 0) continue;
				capabilities |= b.Out->Edges[arrival.EdgeIndex].Capabilities;
			}
			interchange.Capabilities = capabilities;

			for (int side = 0; side < HYBRID_INTERCHANGE_SIDE_COUNT; ++side) {
				if ((anchor.AccessSides & (1u << side)) == 0) continue;
				const int roles[2] = { HYBRID_RAMP_ROLE_OFF,HYBRID_RAMP_ROLE_ON };
				for (int ri = 0; ri < 2; ++ri) {
					const unsigned int id = Add_Edge(b,HYBRID_EDGE_HIGHWAY_RAMP,HYBRID_ROAD_HIGHWAY_RAMP,
																interchange.LocalJunction[side],anchor.Node,-1,true,0,0);
					HybridTopologyEdge & ramp = b.Out->Edges[b.Out->Edges.Count() - 1];
					ramp.Capabilities	= capabilities;
					ramp.InterchangeId= interchange.StableId;
					ramp.RampRole		= (HybridRampRole)roles[ri];
					ramp.RampSide		= (HybridInterchangeSide)side;
					ramp.RampInterval	= Ramp_Interval(c,side,roles[ri],cand);
					ramp.RampRoadbedExtra= anchor.RampExtra;
					ramp.Length			= Hybrid_Quantize_Position(ramp_length[side][roles[ri] - 1] + cand.GoreOffset);
					interchange.RampEdges.Add(id);
					b.Out->RampsByRole[roles[ri]]++;
				}
			}

			if (anchor.Mode == HYBRID_INTERCHANGE_CROSSING_GRADE_SEPARATED) {
				interchange.CrossingEdges.Add(Add_Edge(b,HYBRID_EDGE_INTERCHANGE_CROSSING,anchor.CrossingClass,
																	interchange.LocalJunction[HYBRID_INTERCHANGE_SIDE_RIGHT],
																	interchange.LocalJunction[HYBRID_INTERCHANGE_SIDE_LEFT],-1,true,0,0));
				interchange.CrossingObligation = Add_Node(b,HYBRID_NODE_OVERPASS_OBLIGATION,anchor.X,-1,-1,0,0,-1);
				b.Out->Edges[b.Out->Edges.Count() - 1].Waypoints.Add(interchange.CrossingObligation);
			} else if (anchor.Mode == HYBRID_INTERCHANGE_CROSSING_AT_GRADE) {
				interchange.CrossingEdges.Add(Add_Edge(b,HYBRID_EDGE_INTERCHANGE_CROSSING,anchor.CrossingClass,
																	interchange.LocalJunction[HYBRID_INTERCHANGE_SIDE_RIGHT],anchor.Node,-1,true,0,0));
				interchange.CrossingEdges.Add(Add_Edge(b,HYBRID_EDGE_INTERCHANGE_CROSSING,anchor.CrossingClass,
																	anchor.Node,interchange.LocalJunction[HYBRID_INTERCHANGE_SIDE_LEFT],-1,true,0,0));
			}
			for (int e = 0; e < b.Out->Edges.Count(); ++e) {
				HybridTopologyEdge & edge = b.Out->Edges[e];
				for (int x = 0; x < interchange.CrossingEdges.Count(); ++x) {
					if (edge.StableId != interchange.CrossingEdges[x]) continue;
					edge.Capabilities	= capabilities;
					edge.InterchangeId= interchange.StableId;
					edge.Length			= Edge_Graph_Length(b,edge);
				}
			}

			//	6. Retarget every access-side arrival to its side's local junction; the fallback variant is X.
			for (int r = 0; r < anchor.Arrivals.Count(); ++r) {
				const InterchangeArrival & arrival = anchor.Arrivals[r];
				if ((anchor.AccessSides & (1u << arrival.Side)) == 0) continue;
				HybridTopologyEdge & edge = b.Out->Edges[arrival.EdgeIndex];
				HybridInterchangeRetarget retarget;
				retarget.EdgeId			= edge.StableId;
				retarget.FallbackNode	= anchor.Node;
				retarget.LocalJunction	= interchange.LocalJunction[arrival.Side];
				retarget.Side				= arrival.Side;
				interchange.Retargets.Add(retarget);
				if (edge.NodeA == anchor.Node) edge.NodeA = retarget.LocalJunction;
				if (edge.NodeB == anchor.Node) edge.NodeB = retarget.LocalJunction;
				edge.Length = Edge_Graph_Length(b,edge);
			}

			b.Out->InterchangesByForm[anchor.Form]++;
			b.Out->Interchanges.Add(interchange);
		}
	}
	return true;
}


/*
**	The Phase-9 gate's interchange clause: every active interchange references real connected topology, has its
**	side frozen, its two ramps per access side on the frozen local junctions, its retargeted arrivals ending there,
**	and its selected candidate still hard-valid against every other interchange (12.6.8, the shared 5W ramp
**	constructor included).
*/
static bool Check_Interchanges(const TopologyBuilder & b,StringClass * detail)
{
	HybridInterchangeConstants c;
	Hybrid_Interchange_Constants(b.P->Constants.W,&c);

	for (int i = 0; i < b.Out->Interchanges.Count(); ++i) {

		const HybridInterchange & interchange = b.Out->Interchanges[i];
		if (interchange.Status != HYBRID_INTERCHANGE_ACTIVE) continue;
		const HybridTopologyNode * anchor_node = b.Out->Find_Node(interchange.AnchorNode);
		const HybridTopologyEdge * in = b.Out->Find_Edge(interchange.HighwayEdgeIn);
		const HybridTopologyEdge * out = b.Out->Find_Edge(interchange.HighwayEdgeOut);
		if ((anchor_node == nullptr) || (in == nullptr) || (out == nullptr) || (in->NodeB != interchange.AnchorNode) ||
			 (out->NodeA != interchange.AnchorNode) || (in->RoadClass != HYBRID_ROAD_HIGHWAY_4) ||
			 (out->RoadClass != HYBRID_ROAD_HIGHWAY_4)) {
			detail->Format("interchange %u does not stand on a HIGHWAY_4 pass-through node",interchange.StableId);
			return false;
		}
		if ((interchange.AccessSides == 0) || (interchange.AccessSides > 3u) ||
			 ((interchange.Form == HYBRID_INTERCHANGE_DIAMOND_ACCESS) != (interchange.AccessSides == 3u))) {
			detail->Format("interchange %u has no frozen access side",interchange.StableId);
			return false;
		}
		int ramps = 0;
		for (int e = 0; e < interchange.RampEdges.Count(); ++e) {
			const HybridTopologyEdge * ramp = b.Out->Find_Edge(interchange.RampEdges[e]);
			if ((ramp == nullptr) || (ramp->Kind != HYBRID_EDGE_HIGHWAY_RAMP) || (ramp->NodeB != interchange.AnchorNode) ||
				 (ramp->RampSide >= HYBRID_INTERCHANGE_SIDE_COUNT) || (ramp->NodeA != interchange.LocalJunction[ramp->RampSide])) {
				detail->Format("interchange %u has a ramp off its local junction",interchange.StableId);
				return false;
			}
			ramps++;
		}
		const int sides = (interchange.AccessSides == 3u) ? 2 : 1;
		if (ramps != (2 * sides)) {
			detail->Format("interchange %u has %d ramps for %d access sides",interchange.StableId,ramps,sides);
			return false;
		}
		for (int r = 0; r < interchange.Retargets.Count(); ++r) {
			const HybridTopologyEdge * edge = b.Out->Find_Edge(interchange.Retargets[r].EdgeId);
			if ((edge == nullptr) || ((edge->NodeA != interchange.Retargets[r].LocalJunction) &&
											  (edge->NodeB != interchange.Retargets[r].LocalJunction))) {
				detail->Format("interchange %u: retargeted edge %u does not end at its local junction",interchange.StableId,
									interchange.Retargets[r].EdgeId);
				return false;
			}
		}

		//	The selected candidate, re-tested as frozen.
		HighwayChain chain;
		Build_Highway_Chain(b,in->RouteFamily,&chain);
		int index = -1;
		for (int j = 0; j < chain.Entries.Count(); ++j) {
			if (chain.Entries[j].NodeId == interchange.AnchorNode) index = j;
		}
		if (index < 0) {
			detail->Format("interchange %u: anchor not on its highway chain",interchange.StableId);
			return false;
		}
		InterchangeAnchor anchor;
		anchor.Chain			= &chain;
		anchor.Index			= index;
		anchor.Node				= interchange.AnchorNode;
		anchor.X					= anchor_node->Position;
		anchor.Axis				= interchange.Axis;
		anchor.AccessSides	= interchange.AccessSides;
		anchor.Form				= interchange.Form;
		anchor.Mode				= interchange.CrossingMode;
		anchor.CrossingClass	= interchange.CrossingClass;
		anchor.LocalOverpass	= 0.0f;
		anchor.KeepsJunction	= (interchange.CrossingMode == HYBRID_INTERCHANGE_CROSSING_AT_GRADE) ||
										  Chain_Node_Has_Junction(b,chain,index);
		anchor.RampExtra		= 0.0f;
		for (int r = 0; r < interchange.Retargets.Count(); ++r) {
			for (int e = 0; e < b.Out->Edges.Count(); ++e) {
				if (b.Out->Edges[e].StableId != interchange.Retargets[r].EdgeId) continue;
				InterchangeArrival arrival;
				arrival.EdgeIndex	= e;
				arrival.Side		= interchange.Retargets[r].Side;
				arrival.Access		= true;
				arrival.Beyond		= Stop_Beyond(b,b.Out->Edges[e],interchange.Retargets[r].LocalJunction);
				anchor.Arrivals.Add(arrival);
			}
		}
		anchor.RampExtra = Anchor_Ramp_Extra(b,anchor);
		for (int e = 0; e < interchange.RampEdges.Count(); ++e) {
			const HybridTopologyEdge * ramp = b.Out->Find_Edge(interchange.RampEdges[e]);
			if ((ramp == nullptr) || (ramp->RampRoadbedExtra != anchor.RampExtra)) {
				detail->Format("interchange %u: ramp %u does not carry its capabilities' roadbed (D-024)",interchange.StableId,
									interchange.RampEdges[e]);
				return false;
			}
		}
		HybridRampCandidate cand;
		Hybrid_Ramp_Candidate(c,interchange.SelectedK,0.0f,&cand);
		cand.LocalOffset			= interchange.LocalOffset;
		cand.GoreOffset			= interchange.GoreOffset;
		cand.StraightHalfExtent	= interchange.StraightHalfExtent;
		float lengths[2][2];
		const int refusal = Interchange_Candidate_Refusal(b,c,anchor,cand,interchange.StableId,lengths);
		if (refusal >= 0) {
			detail->Format("interchange %u: its frozen candidate k=%d fails 12.6.8 test %d",interchange.StableId,
								interchange.SelectedK,refusal);
			return false;
		}
	}
	return true;
}


bool Hybrid_Build_Interchanges(const HybridTopologyParams & params,const HybridLandformSet & landforms,
										const HybridHydrologySet & hydrology,const HybridAnchorSet & anchors,
										HybridCoarseMatrices * matrices,HybridTopologyHeightSampleFunc sample,void * context,
										int family_count,HybridTopology * topology,StringClass * detail_out)
{
	if ((matrices == nullptr) || (topology == nullptr) || (sample == nullptr)) return false;

	TopologyBuilder b;
	b.P				= &params;
	b.Identity		= nullptr;
	b.Graph			= nullptr;
	b.Landforms		= &landforms;
	b.Hydrology		= &hydrology;
	b.Anchors		= &anchors;
	b.Matrices		= matrices;
	b.Sample			= sample;
	b.Context		= context;
	b.Out				= topology;
	b.FamilyCount	= family_count;
	b.BaseNode[0]	= 0;
	b.BaseNode[1]	= 0;
	b.NextNodeId	= 1;
	b.NextEdgeId	= 1;
	for (int n = 0; n < topology->Nodes.Count(); ++n) {
		if (topology->Nodes[n].StableId >= b.NextNodeId) b.NextNodeId = topology->Nodes[n].StableId + 1;
		if ((topology->Nodes[n].Kind == HYBRID_NODE_CROSSING_OBLIGATION) && (topology->Nodes[n].WindowId != 0)) {
			b.SelectedWindow.Add(topology->Nodes[n].WindowId);
		}
	}
	for (int e = 0; e < topology->Edges.Count(); ++e) {
		if (topology->Edges[e].StableId >= b.NextEdgeId) b.NextEdgeId = topology->Edges[e].StableId + 1;
	}
	for (int w = 0; w < hydrology.Windows.Count(); ++w) b.WindowUsed.Add(0);

	Build_Interchanges(b);
	StringClass detail;
	if (!Check_Interchanges(b,&detail)) {
		if (detail_out != nullptr) *detail_out = detail;
		return false;
	}
	return true;
}


static bool Check_Gate(TopologyBuilder & b,HybridTopologyResult * result)
{
	HybridTopology & out = *b.Out;
	StringClass detail;

	for (int family = 0; family < b.FamilyCount; ++family) {
		if (Family_Is_Connected(b,family)) continue;
		detail.Format("family %d is not one connected GDI-to-Nod chain",family);
		return Fail(result,HYBRID_TOPOLOGY_NO_CYCLE,detail.Peek_Buffer());
	}

	//	Every selected window is used once: one physical crossing satisfies one obligation only.
	for (int i = 0; i < b.SelectedWindow.Count(); ++i) {
		for (int j = i + 1; j < b.SelectedWindow.Count(); ++j) {
			if (b.SelectedWindow[i] != b.SelectedWindow[j]) continue;
			detail.Format("window %u is bound to two obligations",b.SelectedWindow[i]);
			return Fail(result,HYBRID_TOPOLOGY_OBLIGATION_UNBOUND,detail.Peek_Buffer());
		}
	}

	//	No two families share a required bottleneck: their bound windows are disjoint sets.
	for (int i = 0; i < out.Nodes.Count(); ++i) {

		const HybridTopologyNode & first = out.Nodes[i];
		if (first.Kind != HYBRID_NODE_CROSSING_OBLIGATION) continue;
		if (first.RouteFamily < 0) continue;

		for (int j = i + 1; j < out.Nodes.Count(); ++j) {
			const HybridTopologyNode & second = out.Nodes[j];
			if (second.Kind != HYBRID_NODE_CROSSING_OBLIGATION) continue;
			if (second.RouteFamily < 0) continue;
			if (second.RouteFamily == first.RouteFamily) continue;
			if (second.WindowId != first.WindowId) continue;
			detail.Format("families %d and %d share required window %u",first.RouteFamily,second.RouteFamily,
							  first.WindowId);
			return Fail(result,HYBRID_TOPOLOGY_OBLIGATION_UNBOUND,detail.Peek_Buffer());
		}
	}

	/*
	**	Every overpass obligation is one waypoint shared by exactly two edges -- except a DIAMOND_ACCESS crossing's
	**	own, which its crossing edge carries over the highway's anchor node (12.6.4 step 4) and which is not a
	**	Section-5 overpass (12.6.2); the interchange clause below checks it.
	*/
	int interchange_obligations = 0;
	for (int n = 0; n < out.Nodes.Count(); ++n) {

		if (out.Nodes[n].Kind != HYBRID_NODE_OVERPASS_OBLIGATION) continue;
		bool interchange_owned = false;
		for (int i = 0; i < out.Interchanges.Count(); ++i) {
			if (out.Interchanges[i].CrossingObligation == out.Nodes[n].StableId) interchange_owned = true;
		}
		if (interchange_owned) {
			interchange_obligations++;
			continue;
		}
		int uses = 0;
		for (int e = 0; e < out.Edges.Count(); ++e) {
			for (int w = 0; w < out.Edges[e].Waypoints.Count(); ++w) {
				if (out.Edges[e].Waypoints[w] == out.Nodes[n].StableId) uses++;
			}
		}
		if (uses == 2) continue;
		detail.Format("overpass obligation %u is used by %d edges, not two",out.Nodes[n].StableId,uses);
		return Fail(result,HYBRID_TOPOLOGY_INVALID_INPUT,detail.Peek_Buffer());
	}

	if ((out.RiverBridges < out.RequiredRiverBridges) || (out.CanyonBridges < out.RequiredCanyonBridges)) {
		detail.Format("bridges %d/%d river, %d/%d canyon",out.RiverBridges,out.RequiredRiverBridges,
						  out.CanyonBridges,out.RequiredCanyonBridges);
		return Fail(result,HYBRID_TOPOLOGY_BRIDGE_QUOTA,detail.Peek_Buffer());
	}
	if (!Check_Interchanges(b,&detail)) return Fail(result,HYBRID_TOPOLOGY_INVALID_INPUT,detail.Peek_Buffer());

	if (out.Highways != out.RequiredHighways) {
		detail.Format("highways %d/%d",out.Highways,out.RequiredHighways);
		return Fail(result,HYBRID_TOPOLOGY_BRIDGE_QUOTA,detail.Peek_Buffer());
	}
	if (out.ServiceBranches < out.RequiredServiceBranches) {
		detail.Format("service branches %d/%d",out.ServiceBranches,out.RequiredServiceBranches);
		return Fail(result,HYBRID_TOPOLOGY_SERVICE_QUOTA,detail.Peek_Buffer());
	}
	if (out.Trails < out.RequiredTrails) {
		detail.Format("trails %d/%d",out.Trails,out.RequiredTrails);
		return Fail(result,HYBRID_TOPOLOGY_TRAIL_QUOTA,detail.Peek_Buffer());
	}
	//	Q-024: the reserved count is what Phase 9 found, never more than the target and never padded to it.
	if ((out.ReservedOverpassCount > out.RequestedOverpassTarget) ||
		 (out.ReservedOverpassCount != (out.Count_Of_Node_Kind(HYBRID_NODE_OVERPASS_OBLIGATION) - interchange_obligations))) {
		detail.Format("%d overpasses reserved against a target of %d, %d obligation nodes",out.ReservedOverpassCount,
						  out.RequestedOverpassTarget,out.Count_Of_Node_Kind(HYBRID_NODE_OVERPASS_OBLIGATION) - interchange_obligations);
		return Fail(result,HYBRID_TOPOLOGY_INVALID_INPUT,detail.Peek_Buffer());
	}

	/*
	**	Q-023: every edge's structural sequence is drivable by its own final class -- each window's approach
	**	capability, no structure faced from behind, and every consecutive pair joinable on empty ground.
	*/
	for (int e = 0; e < out.Edges.Count(); ++e) {
		const HybridTopologyEdge & edge = out.Edges[e];
		if (!edge.Is_Realized() || (edge.Waypoints.Count() == 0)) continue;
		DynamicVectorClass<ChainEntry> chain;
		Edge_Chain(b,edge,&chain);
		const int refusal = Chain_Refusal(b,chain,1,chain.Count() - 2,edge.RoadClass);
		if (refusal == HYBRID_BINDING_OK) continue;
		detail.Format("edge %u (%s) has a structural sequence no road of its class can drive (refusal %d)",
						  edge.StableId,Hybrid_Road_Class_Name(edge.RoadClass),refusal);
		return Fail(result,HYBRID_TOPOLOGY_UNREALIZABLE,detail.Peek_Buffer());
	}

	if (out.Realized_Edge_Count() > b.P->MaxRoutedEdges) {
		detail.Format("%d routed edges, R allows %d",out.Realized_Edge_Count(),b.P->MaxRoutedEdges);
		return Fail(result,HYBRID_TOPOLOGY_EDGE_BUDGET,detail.Peek_Buffer());
	}
	if (!out.Has_Cycle()) {
		return Fail(result,HYBRID_TOPOLOGY_NO_CYCLE,"the realized topology is a tree");
	}
	return true;
}


unsigned int Hybrid_Topology_Checksum(const HybridTopology & topology,unsigned int seed)
{
	HybridHashStream stream(seed);

	stream.Add_Int(topology.Nodes.Count());
	stream.Add_Int(topology.Edges.Count());
	stream.Add_Int(topology.Entrances.Count());
	stream.Add_Int(topology.RiverBridges);
	stream.Add_Int(topology.CanyonBridges);
	stream.Add_Int(topology.Highways);
	stream.Add_Int(topology.RequestedOverpassTarget);
	stream.Add_Int(topology.ReservedOverpassCount);
	stream.Add_Int(topology.ServiceBranches);
	stream.Add_Int(topology.Trails);
	stream.Add_Int(topology.CrossLinks);

	for (int i = 0; i < topology.Nodes.Count(); ++i) {
		const HybridTopologyNode & node = topology.Nodes[i];
		stream.Add_Uint(node.StableId);
		stream.Add_Uint((unsigned int)node.Kind);
		stream.Add_Length(node.Position.X);
		stream.Add_Length(node.Position.Y);
		stream.Add_Int(node.RouteFamily);
		stream.Add_Int(node.Team);
		stream.Add_Uint(node.SiteId);
		stream.Add_Uint(node.WindowId);
		stream.Add_Int(node.SectorOrdinal);
	}

	for (int i = 0; i < topology.Edges.Count(); ++i) {
		const HybridTopologyEdge & edge = topology.Edges[i];
		stream.Add_Uint(edge.StableId);
		stream.Add_Uint((unsigned int)edge.Kind);
		stream.Add_Uint((unsigned int)edge.RoadClass);
		stream.Add_Uint(edge.Capabilities);
		stream.Add_Uint(edge.NodeA);
		stream.Add_Uint(edge.NodeB);
		stream.Add_Int(edge.RouteFamily);
		stream.Add_Bool(edge.Required);
		stream.Add_Uint(edge.ObligationId);
		stream.Add_Length(edge.Length);
		stream.Add_Int(edge.Waypoints.Count());
		for (int w = 0; w < edge.Waypoints.Count(); ++w) stream.Add_Uint(edge.Waypoints[w]);

		//	Section 12.6.15: a ramp's role, side, gore station and interval with its Q-026 subintervals.
		if ((edge.Kind == HYBRID_EDGE_HIGHWAY_RAMP) || (edge.Kind == HYBRID_EDGE_INTERCHANGE_CROSSING)) {
			stream.Add_Uint(edge.InterchangeId);
			stream.Add_Uint((unsigned int)edge.RampRole);
			stream.Add_Uint((unsigned int)edge.RampSide);
			stream.Add_Length(edge.RampInterval.GoreStation);
			stream.Add_Length(edge.RampInterval.From);
			stream.Add_Length(edge.RampInterval.To);
			stream.Add_Length(edge.RampInterval.FullFrom);
			stream.Add_Length(edge.RampInterval.FullTo);
			stream.Add_Length(edge.RampInterval.TaperFrom);
			stream.Add_Length(edge.RampInterval.TaperTo);
			stream.Add_Length(edge.RampRoadbedExtra);
		}
	}

	//	Section 12.6.15: every interchange record, StableId order, active and withdrawn alike.
	stream.Add_Int(topology.Interchanges.Count());
	for (int i = 0; i < topology.Interchanges.Count(); ++i) {
		const HybridInterchange & interchange = topology.Interchanges[i];
		stream.Add_Uint(interchange.StableId);
		stream.Add_Uint((unsigned int)interchange.Form);
		stream.Add_Uint((unsigned int)interchange.Status);
		stream.Add_Uint(interchange.AnchorNode);
		stream.Add_Uint(interchange.HighwayEdgeIn);
		stream.Add_Uint(interchange.HighwayEdgeOut);
		stream.Add_Length(interchange.Axis.X);
		stream.Add_Length(interchange.Axis.Y);
		stream.Add_Uint(interchange.AccessSides);
		stream.Add_Uint((unsigned int)interchange.CrossingMode);
		stream.Add_Uint((unsigned int)interchange.CrossingClass);
		stream.Add_Int(interchange.SelectedK);
		stream.Add_Length(interchange.LocalOffset);
		stream.Add_Length(interchange.GoreOffset);
		stream.Add_Length(interchange.StraightHalfExtent);
		for (int s = 0; s < HYBRID_INTERCHANGE_SIDE_COUNT; ++s) stream.Add_Uint(interchange.LocalJunction[s]);
		stream.Add_Uint(interchange.CrossingObligation);
		stream.Add_Uint(interchange.Capabilities);
		stream.Add_Int(interchange.RampEdges.Count());
		for (int e = 0; e < interchange.RampEdges.Count(); ++e) stream.Add_Uint(interchange.RampEdges[e]);
		stream.Add_Int(interchange.CrossingEdges.Count());
		for (int e = 0; e < interchange.CrossingEdges.Count(); ++e) stream.Add_Uint(interchange.CrossingEdges[e]);
		stream.Add_Int(interchange.Retargets.Count());
		for (int r = 0; r < interchange.Retargets.Count(); ++r) {
			stream.Add_Uint(interchange.Retargets[r].EdgeId);
			stream.Add_Uint(interchange.Retargets[r].FallbackNode);
			stream.Add_Uint(interchange.Retargets[r].LocalJunction);
			stream.Add_Uint((unsigned int)interchange.Retargets[r].Side);
		}
	}

	for (int i = 0; i < topology.Entrances.Count(); ++i) {
		const HybridBaseEntrance & entrance = topology.Entrances[i];
		stream.Add_Uint(entrance.BaseSiteId);
		stream.Add_Uint(entrance.NodeId);
		stream.Add_Int(entrance.Team);
		stream.Add_Int(entrance.RouteFamily);
		stream.Add_Int(entrance.SectorOrdinal);
		stream.Add_Bool(entrance.Reused);
		stream.Add_Length(entrance.Point.X);
		stream.Add_Length(entrance.Point.Y);
		stream.Add_Length(entrance.CorridorHalfWidth);
	}

	return stream.Get();
}


//	A refused attempt keeps its Q-023 census in the result; the record itself is reset.
static void Copy_Census(const HybridTopology & topology,HybridTopologyResult * result)
{
	if (result == nullptr) return;
	result->RejectedApproachClass	= topology.RejectedApproachClass;
	result->RejectedWrongSide		= topology.RejectedWrongSide;
	result->RejectedPose				= topology.RejectedPose;
	result->RecoveredByNextRank	= topology.RecoveredByNextRank;
}


static bool Topology_Failed(HybridTopology * out,HybridTopologyResult * result)
{
	Copy_Census(*out,result);
	out->Reset();
	return false;
}


/***********************************************************************************************
 * Hybrid_Build_Topology -- runtime Phase 9                                                     *
 *=============================================================================================*/
bool Hybrid_Build_Topology(const HybridTopologyParams & params,const HybridMapIdentity & identity,
										const HybridStrategicGraph & graph,const HybridLandformSet & landforms,
										const HybridHydrologySet & hydrology,const HybridAnchorSet & anchors,
										HybridCoarseMatrices * matrices,HybridTopologyHeightSampleFunc sample,
										void * context,HybridTopology * out,HybridTopologyResult * result)
{
	if (result != nullptr) {
		result->Failure	= HYBRID_TOPOLOGY_OK;
		result->Detail		= "";
	}
	if ((matrices == nullptr) || (out == nullptr) || (sample == nullptr)) {
		return Fail(result,HYBRID_TOPOLOGY_INVALID_INPUT,"no matrices, no output record or no height sampler");
	}

	out->Reset();

	if (!anchors.Frozen || (anchors.Bases.Count() != 2) || (graph.Sites.Count() <= 0) ||
		 (params.Constants.W <= 0.0f) || (params.Constants.D <= 0.0f)) {
		return Fail(result,HYBRID_TOPOLOGY_INVALID_INPUT,"no frozen anchors, no graph, or degenerate constants");
	}

	TopologyBuilder b;
	b.P				= &params;
	b.Identity		= &identity;
	b.Graph			= &graph;
	b.Landforms		= &landforms;
	b.Hydrology		= &hydrology;
	b.Anchors		= &anchors;
	b.Matrices		= matrices;
	b.Sample			= sample;
	b.Context		= context;
	b.Out				= out;
	b.FamilyCount	= (identity.RouteFamilyCount > 0) ? identity.RouteFamilyCount : 2;
	b.BaseNode[0]	= 0;
	b.BaseNode[1]	= 0;
	b.NextNodeId	= 1;
	b.NextEdgeId	= 1;

	for (int w = 0; w < hydrology.Windows.Count(); ++w) b.WindowUsed.Add(0);

	out->RequiredRiverBridges		= Hybrid_River_Bridge_Quota(identity.SizeClass);
	out->RequiredCanyonBridges		= Hybrid_Canyon_Bridge_Quota(identity.SizeClass);
	out->RequiredHighways			= Hybrid_Highway_Quota(identity.SizeClass);
	out->RequestedOverpassTarget	= Hybrid_Overpass_Target(identity.SizeClass);
	out->RequiredServiceBranches	= Hybrid_Service_Branch_Quota(identity.SizeClass);
	out->RequiredTrails				= Hybrid_Trail_Quota(identity.SizeClass);
	out->OptionalCrossLinkQuota	= Hybrid_Extra_Cross_Link_Count(identity.SizeClass,params.RoadDensity);

	//	1 -- base entrances, and the zero-cost BASE_INTERNAL edges that join each base's own entrances.
	if (!Assign_Base_Entrances(b,result))						return Topology_Failed(out,result);

	//	2 -- the family chains, and every Phase-3 crossing obligation bound to one Phase-6 window.
	if (!Build_Family_Chains(b,result))							return Topology_Failed(out,result);

	//	3, 4 -- the two Section-5 bridge quotas.
	if (!Fill_Bridge_Quota(b,true,result))						return Topology_Failed(out,result);
	if (!Fill_Bridge_Quota(b,false,result))					return Topology_Failed(out,result);

	//	5 -- the highway assignment by trunk length.
	Assign_Highways(b);

	//	6, 7, 8 -- the field access points, the home roads and the field service branches.
	if (!Build_Field_Roads(b,result))							return Topology_Failed(out,result);

	//	9, 10 -- the service and trail quota fills.
	if (!Fill_Service_Quota(b,result))							return Topology_Failed(out,result);
	if (!Fill_Trail_Quota(b,result))								return Topology_Failed(out,result);

	//	11 -- the optional heavy cross-links; a shortage here is not a failure.
	Add_Optional_Cross_Links(b);

	//	12, 13, 14 -- the overpass obligations, each a waypoint shared by two edges.
	if (!Reserve_Overpasses(b,result))							return Topology_Failed(out,result);

	//	15 -- highway access interchanges (Section 12.6); a rejected anchor keeps its conventional junction.
	Build_Interchanges(b);

	if (!Check_Gate(b,result))										return Topology_Failed(out,result);

	out->Checksum = Hybrid_Topology_Checksum(*out,identity.Checksum);
	Copy_Census(*out,result);

	WWDEBUG_SAY(("Hybrid Phase 9: %d nodes, %d edges (%d routed of R=%d), %d entrances, bridges %d river "
					 "%d canyon, %d highways, %d overpasses, %d service, %d trails, %d cross-links, "
					 "checksum 0x%08X\r\n",
					 out->Nodes.Count(),out->Edges.Count(),out->Realized_Edge_Count(),params.MaxRoutedEdges,
					 out->Entrances.Count(),out->RiverBridges,out->CanyonBridges,out->Highways,out->ReservedOverpassCount,
					 out->ServiceBranches,out->Trails,out->CrossLinks,out->Checksum));

	return true;
}
