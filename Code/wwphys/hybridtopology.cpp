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
		default:									break;
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
		case HYBRID_TOPOLOGY_OVERPASS_QUOTA:		return "overpass_quota";
		case HYBRID_TOPOLOGY_EDGE_BUDGET:			return "edge_budget";
		case HYBRID_TOPOLOGY_NO_CYCLE:				return "no_cycle";
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


int Hybrid_River_Bridge_Quota(HybridSizeClass size_class)		{ return Size_Pick(size_class,1,2,3); }
int Hybrid_Canyon_Bridge_Quota(HybridSizeClass size_class)		{ return Size_Pick(size_class,1,1,2); }
int Hybrid_Highway_Quota(HybridSizeClass size_class)				{ return Size_Pick(size_class,0,1,2); }
int Hybrid_Overpass_Quota(HybridSizeClass size_class)			{ return Size_Pick(size_class,0,1,2); }
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

	RequiredRiverBridges		= 0;
	RequiredCanyonBridges	= 0;
	RequiredHighways			= 0;
	RequiredOverpasses		= 0;
	RequiredServiceBranches	= 0;
	RequiredTrails				= 0;
	OptionalCrossLinkQuota	= 0;

	RiverBridges	= 0;
	CanyonBridges	= 0;
	Highways			= 0;
	Overpasses		= 0;
	ServiceBranches= 0;
	Trails			= 0;
	CrossLinks		= 0;
	Checksum			= 0;
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
	MaxRoutedEdges(32),TunnelsAvailable(false)
{
	memset(&Constants,0,sizeof(Constants));
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


//	Insert a waypoint into the chain edge whose own segment passes nearest it, in along-edge order.
static void Insert_Waypoint(TopologyBuilder & b,int family,unsigned int node_id,const Vector2 & position)
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
	if (best < 0) return;

	HybridTopologyEdge & edge = b.Out->Edges[best];
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
	edge.Waypoints.Insert(insert_at,node_id);
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

		//	Why each window was passed over, so an unbound obligation says what stood in the way.
		int refused_used = 0;
		int refused_obstruction = 0;
		int refused_method = 0;
		int refused_distance = 0;
		int refused_spacing = 0;
		float nearest_refused = 3.4e38f;

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
			} else if (obligation.RequiredMethod == HYBRID_CROSSING_TUNNEL) {
				if (!window.Has_Method(HYBRID_WINDOW_METHOD_TUNNEL)) { refused_method++; continue; }
			}

			const float distance = (window.Center - guide).Length();
			if (distance > corridor_half) {
				refused_distance++;
				if (distance < nearest_refused) nearest_refused = distance;
				continue;
			}
			if (!Window_Spacing_Is_Legal(b,window)) { refused_spacing++; continue; }

			//	Preferred beats plain, then nearer the guide, then the lower StableId.
			if (chosen >= 0) {
				const HybridCrossingWindow & held = b.Hydrology->Windows[chosen];
				if (held.Preferred && !window.Preferred) continue;
				if ((held.Preferred == window.Preferred) && (distance >= chosen_distance)) continue;
			}
			chosen				= w;
			chosen_distance	= distance;
		}

		if (chosen < 0) {
			detail.Format("obligation %u (family %d, obstruction %s, method %d) has no qualifying window "
								"among %d: %d already used, %d wrong obstruction, %d without the method, %d past the "
								"%.0f corridor half-width (nearest %.0f), %d too close to another",
								obligation.StableId,obligation.RouteFamily,
								Hybrid_Obstruction_Kind_Name(obligation.Obstruction),
								(int)obligation.RequiredMethod,b.Hydrology->Windows.Count(),refused_used,
								refused_obstruction,refused_method,refused_distance,corridor_half,
								(nearest_refused < 3.4e38f) ? nearest_refused : -1.0f,refused_spacing);
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
static bool Add_Bank_Cross_Link(TopologyBuilder & b,const HybridCrossingWindow & window)
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
	return true;
}


static bool Fill_Bridge_Quota(TopologyBuilder & b,bool river,HybridTopologyResult * result)
{
	StringClass detail;

	for (;;) {

		const int have = river ? b.Out->RiverBridges : b.Out->CanyonBridges;
		const int required = river ? b.Out->RequiredRiverBridges : b.Out->RequiredCanyonBridges;
		if (have >= required) return true;

		int chosen = -1;
		bool chosen_preferred = false;
		int chosen_family = -1;
		float chosen_distance = 0.0f;

		for (int w = 0; w < b.Hydrology->Windows.Count(); ++w) {

			if (b.WindowUsed[w] != 0) continue;
			const HybridCrossingWindow & window = b.Hydrology->Windows[w];
			if (!Window_Is_Bridge_Category(window,river)) continue;
			if (!window.Has_Method(HYBRID_WINDOW_METHOD_BRIDGE)) continue;
			if (!Window_Spacing_Is_Legal(b,window)) continue;

			float distance = 0.0f;
			const int family = Nearest_Family(b,window.Center,&distance);

			//	Preferred windows first, ranked by (family id, distance to that family's nearest guide,
			//	StableId); a plain window is only considered when no preferred one is left.
			if (chosen >= 0) {
				if (chosen_preferred && !window.Preferred) continue;
				if (chosen_preferred == window.Preferred) {
					if (family > chosen_family) continue;
					if ((family == chosen_family) && (distance >= chosen_distance)) continue;
				}
			}
			chosen				= w;
			chosen_preferred	= window.Preferred;
			chosen_family		= family;
			chosen_distance	= distance;
		}

		if (chosen < 0) {
			detail.Format("%s bridge quota short: %d of %d, no bridge-capable window left",
								river ? "river" : "canyon/ravine",have,required);
			return Fail(result,HYBRID_TOPOLOGY_BRIDGE_QUOTA,detail.Peek_Buffer());
		}

		const HybridCrossingWindow & window = b.Hydrology->Windows[chosen];

		if (chosen_preferred && (chosen_family >= 0)) {

			const unsigned int node = Add_Node(b,HYBRID_NODE_CROSSING_OBLIGATION,window.Center,chosen_family,
														  -1,0,window.StableId,-1);
			Insert_Waypoint(b,chosen_family,node,window.Center);
			Consume_Window(b,window);
			continue;
		}

		/*
		**	The plan's fallback -- one `SECONDARY_2` cross-link between the nearest heavy waypoints on
		**	opposite banks -- assumes a heavy node exists on the far bank.  Often none does: a river or
		**	ravine can run close enough to the map edge that every heavy node stands on one side of it.
		**
		**	When that happens the window is bound to the nearest family chain instead, which is the very
		**	procedure a *preferred* window gets, and only when the window is inside that family's own
		**	`MaxCorridorHalfWidth(PRIMARY_2)` so the binding cannot drag a route across the map.  A bridge
		**	on a family route satisfies the same Section-5 quota with strictly more use than a cross-link
		**	to nowhere, and the alternative -- failing the attempt over ground that has a perfectly good
		**	crossing on it -- would reduce the quota to a coin flip on where Phase 5 put its ravines.
		*/
		if (!Add_Bank_Cross_Link(b,window)) {

			const float corridor_half = Hybrid_Max_Corridor_Half_Width(HYBRID_ROAD_PRIMARY_2,b.P->Constants.S,
																						  b.P->Constants.D);
			const float chain_distance = (chosen_family >= 0)
												 ? Distance_To_Family_Chain(b,chosen_family,window.Center) : 3.4e38f;
			if ((chosen_family >= 0) && (chain_distance <= corridor_half)) {
				const unsigned int node = Add_Node(b,HYBRID_NODE_CROSSING_OBLIGATION,window.Center,chosen_family,
															  -1,0,window.StableId,-1);
				Insert_Waypoint(b,chosen_family,node,window.Center);
				Consume_Window(b,window);
				continue;
			}

			//	Not usable at all: set the window aside so it is offered once, and try the next candidate.
			const int index = Window_Index(b,window.StableId);
			if ((index >= 0) && (index < b.WindowUsed.Count())) b.WindowUsed[index] = 1;
			continue;
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


//	A "non-base waypoint": every topology node a road may start or end at, which is everything except
//	the logical base node, its entrances (reached only through BASE_INTERNAL) and a mid-edge overpass.
static bool Is_Pairable_Waypoint(const HybridTopologyNode & node)
{
	return (node.Kind == HYBRID_NODE_FAMILY_WAYPOINT) || (node.Kind == HYBRID_NODE_CROSSING_OBLIGATION) ||
			 (node.Kind == HYBRID_NODE_FIELD_ACCESS);
}


/*
**	Candidate pairs of non-base waypoints.  `different_families` is step 10's and step 11's extra rule;
**	`minimum` is their `>=8W` floor, which step 9 does not have.
**
**	"Non-base waypoint" is read as every node a road may end at, not only a Phase-3 family waypoint: a
**	two-family map has exactly two of those and exactly one pair between them, which cannot fill a
**	Section-5 trail quota of two by any ordering.  A crossing waypoint and a field access point are
**	waypoints in the same sense, and pairing them keeps the intent of the rule -- a shortcut between
**	*different* routes -- because the "different families" test still refuses two nodes of one family.
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
static bool Overpass_Point_Is_Legal(const TopologyBuilder & b,const Vector2 & point)
{
	const HybridConstants & k = b.P->Constants;

	if (!Point_Is_Clear(b,point,false)) return false;

	const float clearance = k.A + (0.5f * Hybrid_Roadbed_Width(HYBRID_ROAD_SECONDARY_2,k.W));
	for (int i = 0; i < b.Hydrology->Bodies.Count(); ++i) {

		const HybridWaterBody & body = b.Hydrology->Bodies[i];
		if (body.Kind == HYBRID_WATER_BODY_ISLAND) continue;
		if ((point.X < (body.BoundsMin.X - clearance)) || (point.X > (body.BoundsMax.X + clearance)) ||
			 (point.Y < (body.BoundsMin.Y - clearance)) || (point.Y > (body.BoundsMax.Y + clearance))) continue;

		//	Water that merely surrounds the point still encloses it on the analysis grid.
		for (int p = 0; p < body.Points.Count(); ++p) {
			if ((body.Points[p] - point).Length() <= clearance) return false;
		}
	}

	for (int i = 0; i < b.Anchors->Bases.Count(); ++i) {
		if ((point - b.Anchors->Bases[i].Center).Length() <= b.Anchors->Bases[i].ProtectedRadius) return false;
	}
	for (int i = 0; i < b.Anchors->Resources.Count(); ++i) {
		if ((point - b.Anchors->Resources[i].Center).Length() <= b.Anchors->Resources[i].ReserveRadius) return false;
	}
	for (int i = 0; i < b.SelectedWindow.Count(); ++i) {
		const HybridCrossingWindow * window = b.Hydrology->Find_Window(b.SelectedWindow[i]);
		if (window == nullptr) continue;
		if ((point - window->Center).Length() <= (window->ApproachLength + (0.5f * window->Span))) return false;
	}
	for (int n = 0; n < b.Out->Nodes.Count(); ++n) {
		if (b.Out->Nodes[n].Kind != HYBRID_NODE_OVERPASS_OBLIGATION) continue;
		if ((point - b.Out->Nodes[n].Position).Length() < (OVERPASS_SPACING_W * k.W)) return false;
	}
	return true;
}


struct OverpassCandidate
{
	int				HighwayEdge;		// index into Out->Edges
	int				OtherEdge;			// index, or -1 for a synthesized cross-link candidate
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
**	plan names.  False means the candidate is not hard-valid at all.
*/
static bool Overpass_Cost(const TopologyBuilder & b,const Vector2 & point,const Vector2 & a0,const Vector2 & a1,
								  const Vector2 & b0,const Vector2 & b1,float * cost_out)
{
	const HybridConstants & k = b.P->Constants;
	const float arm = OVERPASS_ARM_W * k.W;
	const float max_grade = Hybrid_Road_Class_Spec(HYBRID_ROAD_SECONDARY_2).MaxGradeDegrees * PI_OVER_180;

	//	At least 10W of each segment on each side of the crossing.
	if ((point - a0).Length() < arm)	return false;
	if ((point - a1).Length() < arm)	return false;
	if ((point - b0).Length() < arm)	return false;
	if ((point - b1).Length() < arm)	return false;

	const float angle = Crossing_Angle_Degrees(a0,a1,b0,b1);
	if ((angle < (OVERPASS_ANGLE_MIN - 0.001f)) && ((180.0f - angle) < (OVERPASS_ANGLE_MIN - 0.001f))) return false;
	if (angle > (OVERPASS_ANGLE_MAX + 0.001f)) return false;

	if (!Overpass_Point_Is_Legal(b,point)) return false;

	float slope = 0.0f;
	if (!Disc_Max_Slope(b,point,OVERPASS_SLOPE_DISC_W * k.W,&slope)) return false;
	if (slope > max_grade) return false;

	float nearest = (point - a0).Length();
	nearest = Min2(nearest,(point - a1).Length());
	nearest = Min2(nearest,(point - b0).Length());
	nearest = Min2(nearest,(point - b1).Length());

	*cost_out = Clamp2(slope / Max2(max_grade,k.Q),0.0f,1.0f) + Clamp2(nearest / (0.5f * k.D),0.0f,1.0f);
	return true;
}


//	Step 14: one shared waypoint, inserted into both participating edges at the same quantized point.
static void Insert_Shared_Overpass(TopologyBuilder & b,int highway_edge,int other_edge,const Vector2 & point)
{
	const unsigned int node = Add_Node(b,HYBRID_NODE_OVERPASS_OBLIGATION,point,-1,-1,0,0,-1);

	const int edges[2] = { highway_edge, other_edge };
	for (int i = 0; i < 2; ++i) {

		if ((edges[i] < 0) || (edges[i] >= b.Out->Edges.Count())) continue;
		HybridTopologyEdge & edge = b.Out->Edges[edges[i]];
		const HybridTopologyNode * a = b.Out->Find_Node(edge.NodeA);

		int insert_at = edge.Waypoints.Count();
		if (a != nullptr) {
			const float own = (point - a->Position).Length();
			for (int w = 0; w < edge.Waypoints.Count(); ++w) {
				const HybridTopologyNode * existing = b.Out->Find_Node(edge.Waypoints[w]);
				if (existing == nullptr) continue;
				if ((existing->Position - a->Position).Length() > own) {
					insert_at = w;
					break;
				}
			}
		}
		edge.Waypoints.Insert(insert_at,node);
	}
	b.Out->Overpasses++;
}


static bool Reserve_Overpasses(TopologyBuilder & b,HybridTopologyResult * result)
{
	const HybridConstants & k = b.P->Constants;
	const float minimum = CROSS_LINK_MIN_W * k.W;
	StringClass detail;

	//	Step 12: the natural intersections between a HIGHWAY_4 segment and a PRIMARY_2/SECONDARY_2 one.
	while (b.Out->Overpasses < b.Out->RequiredOverpasses) {

		DynamicVectorClass<OverpassCandidate> candidates;

		for (int h = 0; h < b.Out->Edges.Count(); ++h) {

			const HybridTopologyEdge & highway = b.Out->Edges[h];
			if (highway.RoadClass != HYBRID_ROAD_HIGHWAY_4) continue;
			const HybridTopologyNode * h0 = b.Out->Find_Node(highway.NodeA);
			const HybridTopologyNode * h1 = b.Out->Find_Node(highway.NodeB);
			if ((h0 == nullptr) || (h1 == nullptr)) continue;

			for (int o = 0; o < b.Out->Edges.Count(); ++o) {

				if (o == h) continue;
				const HybridTopologyEdge & other = b.Out->Edges[o];
				if ((other.RoadClass != HYBRID_ROAD_PRIMARY_2) && (other.RoadClass != HYBRID_ROAD_SECONDARY_2)) continue;
				const HybridTopologyNode * o0 = b.Out->Find_Node(other.NodeA);
				const HybridTopologyNode * o1 = b.Out->Find_Node(other.NodeB);
				if ((o0 == nullptr) || (o1 == nullptr)) continue;

				Vector2 point(0.0f,0.0f);
				if (!Segments_Cross(h0->Position,h1->Position,o0->Position,o1->Position,&point,nullptr,nullptr)) continue;

				float cost = 0.0f;
				if (!Overpass_Cost(b,point,h0->Position,h1->Position,o0->Position,o1->Position,&cost)) continue;

				OverpassCandidate candidate;
				candidate.HighwayEdge	= h;
				candidate.OtherEdge		= o;
				candidate.NodeA			= 0;
				candidate.NodeB			= 0;
				candidate.Point			= point;
				candidate.Cost				= cost;
				candidate.Length			= 0.0f;
				candidates.Add(candidate);
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
		Insert_Shared_Overpass(b,candidates[best].HighwayEdge,candidates[best].OtherEdge,candidates[best].Point);
	}

	//	Step 13: synthesized SECONDARY_2 cross-links that cross a highway, ranked with their own length.
	while (b.Out->Overpasses < b.Out->RequiredOverpasses) {

		DynamicVectorClass<PairCandidate> pairs;
		Collect_Waypoint_Pairs(b,true,minimum,&pairs);

		DynamicVectorClass<OverpassCandidate> candidates;

		for (int p = 0; p < pairs.Count(); ++p) {

			const HybridTopologyNode * a = b.Out->Find_Node(pairs[p].NodeA);
			const HybridTopologyNode * c = b.Out->Find_Node(pairs[p].NodeB);
			if ((a == nullptr) || (c == nullptr)) continue;

			for (int h = 0; h < b.Out->Edges.Count(); ++h) {

				const HybridTopologyEdge & highway = b.Out->Edges[h];
				if (highway.RoadClass != HYBRID_ROAD_HIGHWAY_4) continue;
				const HybridTopologyNode * h0 = b.Out->Find_Node(highway.NodeA);
				const HybridTopologyNode * h1 = b.Out->Find_Node(highway.NodeB);
				if ((h0 == nullptr) || (h1 == nullptr)) continue;

				Vector2 point(0.0f,0.0f);
				if (!Segments_Cross(h0->Position,h1->Position,a->Position,c->Position,&point,nullptr,nullptr)) continue;

				float cost = 0.0f;
				if (!Overpass_Cost(b,point,h0->Position,h1->Position,a->Position,c->Position,&cost)) continue;

				OverpassCandidate candidate;
				candidate.HighwayEdge	= h;
				candidate.OtherEdge		= -1;
				candidate.NodeA			= pairs[p].NodeA;
				candidate.NodeB			= pairs[p].NodeB;
				candidate.Point			= point;
				candidate.Cost				= cost + (pairs[p].Distance / Max2(b.P->Constants.D,b.P->Constants.Q));
				candidate.Length			= pairs[p].Distance;
				candidates.Add(candidate);
			}
		}

		if (candidates.Count() == 0) {
			detail.Format("overpass quota short: %d of %d, no natural or synthesized candidate remains",
								b.Out->Overpasses,b.Out->RequiredOverpasses);
			return Fail(result,HYBRID_TOPOLOGY_OVERPASS_QUOTA,detail.Peek_Buffer());
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
		Insert_Shared_Overpass(b,chosen.HighwayEdge,b.Out->Edges.Count() - 1,chosen.Point);
	}
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

	//	Every overpass obligation is one waypoint shared by exactly two edges.
	for (int n = 0; n < out.Nodes.Count(); ++n) {

		if (out.Nodes[n].Kind != HYBRID_NODE_OVERPASS_OBLIGATION) continue;
		int uses = 0;
		for (int e = 0; e < out.Edges.Count(); ++e) {
			for (int w = 0; w < out.Edges[e].Waypoints.Count(); ++w) {
				if (out.Edges[e].Waypoints[w] == out.Nodes[n].StableId) uses++;
			}
		}
		if (uses == 2) continue;
		detail.Format("overpass obligation %u is used by %d edges, not two",out.Nodes[n].StableId,uses);
		return Fail(result,HYBRID_TOPOLOGY_OVERPASS_QUOTA,detail.Peek_Buffer());
	}

	if ((out.RiverBridges < out.RequiredRiverBridges) || (out.CanyonBridges < out.RequiredCanyonBridges)) {
		detail.Format("bridges %d/%d river, %d/%d canyon",out.RiverBridges,out.RequiredRiverBridges,
						  out.CanyonBridges,out.RequiredCanyonBridges);
		return Fail(result,HYBRID_TOPOLOGY_BRIDGE_QUOTA,detail.Peek_Buffer());
	}
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
	if (out.Overpasses < out.RequiredOverpasses) {
		detail.Format("overpasses %d/%d",out.Overpasses,out.RequiredOverpasses);
		return Fail(result,HYBRID_TOPOLOGY_OVERPASS_QUOTA,detail.Peek_Buffer());
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
	stream.Add_Int(topology.Overpasses);
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
	out->RequiredOverpasses			= Hybrid_Overpass_Quota(identity.SizeClass);
	out->RequiredServiceBranches	= Hybrid_Service_Branch_Quota(identity.SizeClass);
	out->RequiredTrails				= Hybrid_Trail_Quota(identity.SizeClass);
	out->OptionalCrossLinkQuota	= Hybrid_Extra_Cross_Link_Count(identity.SizeClass,params.RoadDensity);

	//	1 -- base entrances, and the zero-cost BASE_INTERNAL edges that join each base's own entrances.
	if (!Assign_Base_Entrances(b,result))						{ out->Reset(); return false; }

	//	2 -- the family chains, and every Phase-3 crossing obligation bound to one Phase-6 window.
	if (!Build_Family_Chains(b,result))							{ out->Reset(); return false; }

	//	3, 4 -- the two Section-5 bridge quotas.
	if (!Fill_Bridge_Quota(b,true,result))						{ out->Reset(); return false; }
	if (!Fill_Bridge_Quota(b,false,result))					{ out->Reset(); return false; }

	//	5 -- the highway assignment by trunk length.
	Assign_Highways(b);

	//	6, 7, 8 -- the field access points, the home roads and the field service branches.
	if (!Build_Field_Roads(b,result))							{ out->Reset(); return false; }

	//	9, 10 -- the service and trail quota fills.
	if (!Fill_Service_Quota(b,result))							{ out->Reset(); return false; }
	if (!Fill_Trail_Quota(b,result))								{ out->Reset(); return false; }

	//	11 -- the optional heavy cross-links; a shortage here is not a failure.
	Add_Optional_Cross_Links(b);

	//	12, 13, 14 -- the overpass obligations, each a waypoint shared by two edges.
	if (!Reserve_Overpasses(b,result))							{ out->Reset(); return false; }

	if (!Check_Gate(b,result))										{ out->Reset(); return false; }

	out->Checksum = Hybrid_Topology_Checksum(*out,identity.Checksum);

	WWDEBUG_SAY(("Hybrid Phase 9: %d nodes, %d edges (%d routed of R=%d), %d entrances, bridges %d river "
					 "%d canyon, %d highways, %d overpasses, %d service, %d trails, %d cross-links, "
					 "checksum 0x%08X\r\n",
					 out->Nodes.Count(),out->Edges.Count(),out->Realized_Edge_Count(),params.MaxRoutedEdges,
					 out->Entrances.Count(),out->RiverBridges,out->CanyonBridges,out->Highways,out->Overpasses,
					 out->ServiceBranches,out->Trails,out->CrossLinks,out->Checksum));

	return true;
}
