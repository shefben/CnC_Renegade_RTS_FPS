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
 *                     $Archive:: /Commando/Code/wwphys/hybridstrategicgraph.cpp              $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Hybrid world generator rewrite, item I4 -- runtime Phase 3.  See hybridstrategicgraph.h.      *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#include "hybridstrategicgraph.h"
#include "worldboundsservice.h"
#include "wwmath.h"
#include "wwdebug.h"

#include <math.h>


static inline float Max2(float a,float b)	{ return (a > b) ? a : b; }
static inline float Min2(float a,float b)	{ return (a < b) ? a : b; }


/*
**	Section 6.1 / Phase 3 guide constants, named once.
*/
static const float	ROLE_REPAIR_LIMIT_D				= 0.10f;		// Section 6.1: a guide moves at most 0.10D
static const float	CONTESTED_MEDIUM_V				= 0.32f;
static const float	CONTESTED_LARGE_V					= 0.56f;
static const float	REMOTE_FIELD_V						= 0.70f;
static const float	REMOTE_FIELD_U_LIMIT				= 0.20f;

/*
**	Section 6.10's river influence band: the plan's own `centerline + 0.5*river_width + 2W`.
**
**	Which intents are hard here is deliberate, and it is the frozen ones.  Phase 2 freezes real
**	geometry for exactly two kinds -- the guided-river centerline spline, and the coast's sea
**	boundary `v` on a drawn side -- and Section 6.10 says so for the river in as many words: the
**	guided river is never moved around bases later, so Phase 3 treats its influence band as hard and
**	moves the role instead.  Every other entry in the table (lake, basin, canyon, ridge, ravine,
**	neck, open-maneuver disc) is a guide with a *target* size whose polygon does not exist yet, and
**	the phases that later realize it are required to keep clear of the reserves frozen here: Phase 5
**	rejects a landform candidate that intersects `BASE_PROTECTED` or a Tiberium hard reserve, Phase 6
**	does the same for water, and Phase 8 fails the attempt if realized terrain violates a role.
**	Treating those target radii as hard now would also contradict the archetypes themselves -- a
**	Section-6.3 lake radius of `0.18D..0.25D` centered at `v=+-0.38` always covers the strategic
**	centre, while Section 6.3 simultaneously requires a dry central corridor there, which only the
**	shaped Phase-6 polygon can provide.
*/
static const float	RIVER_BAND_W						= 2.0f;

/*
**	Section 6.10 river-adjacent contested field: the candidate offset puts the reserve edge exactly
**	7W outside the provisional bank at the station being tested.  Section 6.2 states the requirement
**	globally -- the field's nearest edge lies 4W..10W from the principal river bank -- so a candidate
**	whose reserve comes closer than 4W to any *other* part of the meandering centerline is not hard
**	valid and the station walk continues.  Without that test a station on the inside of a tight
**	meander would put the field in the water it is supposed to sit beside.
*/
static const float	RIVER_ADJACENT_CLEARANCE_W		= 7.0f;
static const float	RIVER_ADJACENT_MIN_CLEARANCE_W	= 4.0f;

enum { MAX_ROUTE_FAMILIES = 3 };
enum { MAX_FAMILY_GUIDES = 4 };


const char * Hybrid_Obstruction_Kind_Name(HybridObstructionKind kind)
{
	switch (kind) {
		case HYBRID_OBSTRUCTION_NONE:		return "NONE";
		case HYBRID_OBSTRUCTION_RIVER:	return "RIVER";
		case HYBRID_OBSTRUCTION_LAKE:		return "LAKE";
		case HYBRID_OBSTRUCTION_COAST:	return "COAST";
		case HYBRID_OBSTRUCTION_CANYON:	return "CANYON";
		case HYBRID_OBSTRUCTION_RAVINE:	return "RAVINE";
		case HYBRID_OBSTRUCTION_RIDGE:	return "RIDGE";
		case HYBRID_OBSTRUCTION_CLIFF:	return "CLIFF";
		default:									return "unknown";
	}
}


const char * Hybrid_Crossing_Method_Name(HybridCrossingMethod method)
{
	switch (method) {
		case HYBRID_CROSSING_ANY:					return "ANY";
		case HYBRID_CROSSING_BRIDGE:				return "BRIDGE";
		case HYBRID_CROSSING_PASS:					return "PASS";
		case HYBRID_CROSSING_FORD_LIGHT:			return "FORD_LIGHT";
		case HYBRID_CROSSING_FORD_HARVESTER:	return "FORD_HARVESTER";
		default:											return "unknown";
	}
}


const char * Hybrid_Strategic_Graph_Failure_Name(HybridStrategicGraphFailure failure)
{
	switch (failure) {
		case HYBRID_GRAPH_OK:									return "OK";
		case HYBRID_GRAPH_NO_LEGAL_BASE_POSITION:			return "NO_LEGAL_BASE_POSITION";
		case HYBRID_GRAPH_NO_LEGAL_RESOURCE_POSITION:	return "NO_LEGAL_RESOURCE_POSITION";
		case HYBRID_GRAPH_NO_LEGAL_WAYPOINT_POSITION:	return "NO_LEGAL_WAYPOINT_POSITION";
		case HYBRID_GRAPH_MISSING_GUIDED_RIVER_INTENT:	return "MISSING_GUIDED_RIVER_INTENT";
		case HYBRID_GRAPH_NO_RIVER_ADJACENT_FIELD:		return "NO_RIVER_ADJACENT_FIELD";
		case HYBRID_GRAPH_ROLE_COUNT_MISMATCH:				return "ROLE_COUNT_MISMATCH";
		case HYBRID_GRAPH_INCOMPLETE_ROUTE_FAMILY:		return "INCOMPLETE_ROUTE_FAMILY";
		case HYBRID_GRAPH_MISSING_RESOURCE_INTENT:		return "MISSING_RESOURCE_INTENT";
		case HYBRID_GRAPH_NO_GRAPH_CYCLE:					return "NO_GRAPH_CYCLE";
		default:														return "unknown";
	}
}


int Hybrid_Contested_Field_Count(HybridSizeClass size_class)
{
	switch (size_class) {
		case HYBRID_SIZE_MEDIUM:	return 3;
		case HYBRID_SIZE_LARGE:		return 5;
		default:							return 1;		// SMALL
	}
}


int Hybrid_Remote_Field_Count(HybridSizeClass size_class)
{
	return (size_class == HYBRID_SIZE_LARGE) ? 2 : 1;
}


HybridStrategicGraphParams::HybridStrategicGraphParams(void) :
	MapPlanSeed(0),BoundaryMargin(0.0f),FieldRadiusMax(60.0f),RiverWidth(6.0f)
{
	Hybrid_Compute_Constants (2.556f,1.0f,60.0f,1200.0f,1200.0f,3.0f,10000.0f,&Constants);
}


/***********************************************************************************************
 * HybridStrategicGraph accessors                                                              *
 *---------------------------------------------------------------------------------------------*/
const HybridStrategicSite * HybridStrategicGraph::Find_Site(unsigned int stable_id) const
{
	for (int i = 0; i < Sites.Count (); i++) {
		if (Sites[i].StableId == stable_id) return &Sites[i];
	}
	return nullptr;
}


int HybridStrategicGraph::Count_Of_Role(HybridSiteRole role) const
{
	int count = 0;
	for (int i = 0; i < Sites.Count (); i++) {
		if (Sites[i].Role == role) count++;
	}
	return count;
}


int HybridStrategicGraph::Count_Of_Intent(HybridEdgeIntent intent) const
{
	int count = 0;
	for (int i = 0; i < Edges.Count (); i++) {
		if (Edges[i].Intent == intent) count++;
	}
	return count;
}


/*
**	Union-find over the sites in creation order: the first edge whose endpoints already share a set
**	closes a cycle.  Edges are visited in StableId order, so the answer never depends on container
**	layout.
*/
bool HybridStrategicGraph::Has_Cycle(void) const
{
	const int count = Sites.Count ();
	if (count <= 0) {
		return false;
	}

	DynamicVectorClass<int> parent;
	parent.Resize (count);
	for (int i = 0; i < count; i++) {
		parent.Add (i);
	}

	bool cycle = false;
	for (int e = 0; e < Edges.Count (); e++) {

		int a = -1;
		int b = -1;
		for (int i = 0; i < count; i++) {
			if (Sites[i].StableId == Edges[e].SiteA) a = i;
			if (Sites[i].StableId == Edges[e].SiteB) b = i;
		}
		if ((a < 0) || (b < 0) || (a == b)) {
			continue;
		}

		while (parent[a] != a) a = parent[a];
		while (parent[b] != b) b = parent[b];
		if (a == b) {
			cycle = true;
			break;
		}
		parent[b] = a;
	}

	return cycle;
}


/***********************************************************************************************
 * Phase 3 geometry helpers                                                                    *
 *---------------------------------------------------------------------------------------------*/
static bool
Fail(HybridStrategicGraphResult * result,HybridStrategicGraphFailure failure,const char * detail)
{
	if (result != nullptr) {
		result->Failure	= failure;
		result->Detail		= (detail != nullptr) ? detail : "";
	}
	return false;
}


static float
Distance_Between(const Vector2 & a,const Vector2 & b)
{
	const float dx = a.X - b.X;
	const float dy = a.Y - b.Y;
	return WWMath::Sqrt ((dx * dx) + (dy * dy));
}


static float
Distance_To_Segment(const Vector2 & point,const Vector2 & a,const Vector2 & b)
{
	const float dx = b.X - a.X;
	const float dy = b.Y - a.Y;
	const float length_squared = (dx * dx) + (dy * dy);
	if (length_squared <= 0.0f) {
		return Distance_Between (point,a);
	}

	float t = (((point.X - a.X) * dx) + ((point.Y - a.Y) * dy)) / length_squared;
	t = Min2 (1.0f,Max2 (0.0f,t));
	const Vector2 closest (a.X + (dx * t),a.Y + (dy * t));
	return Distance_Between (point,closest);
}


static float
Distance_To_Polyline(const Vector2 & point,const DynamicVectorClass<Vector2> & points)
{
	if (points.Count () <= 0) {
		return 1.0e30f;
	}
	if (points.Count () == 1) {
		return Distance_Between (point,points[0]);
	}

	float nearest = 1.0e30f;
	for (int i = 1; i < points.Count (); i++) {
		const float distance = Distance_To_Segment (point,points[i - 1],points[i]);
		if (distance < nearest) nearest = distance;
	}
	return nearest;
}


/*
**	Is the whole reserve disc inside the placeable boundary, with the profile's boundary margin kept
**	clear as well?  `WorldBoundsService` is the canonical owner of that question, and its own rule
**	already excludes the cliff band, so Phase 3 adds only the Section-6.1 margin term.  On a world
**	with no configured boundary -- a legacy map, or a self-check running without one -- the service
**	answers "legal", which is the same degradation every other placement consumer gets.
*/
static bool
Reserve_Is_Placeable(const HybridStrategicGraphParams & params,const Vector2 & center,float reserve_radius)
{
	return WorldBoundsService::Can_Place_In_Region (center,reserve_radius + params.BoundaryMargin);
}


/*
**	The frozen Section-6.10 hard intents -- the guided river's influence band and the exterior sea --
**	tested as the semantic types the plan names.  Both are unavailable to base and resource roles; a
**	route waypoint may sit on its own crossing intent, which is why this test is not applied to it.
*/
static bool
Intersects_Hard_Intent(const HybridStrategicGraphParams & params,const HybridMapIdentity & identity,
							  const HybridArchetypeIntentSet & intents,const Vector2 & center,float reserve_radius,
							  bool include_guided_river)
{
	const HybridConstants & k = params.Constants;
	const float scale = identity.Frame.Scale;
	const Vector2 uv = identity.Frame.To_Strategic (center);

	for (int i = 0; i < intents.Intents.Count (); i++) {
		const HybridArchetypeIntent & intent = intents.Intents[i];

		switch (intent.Kind) {

			case HYBRID_INTENT_GUIDED_RIVER:
			case HYBRID_INTENT_COASTAL_GUIDED_RIVER:
			{
				if (!include_guided_river) break;
				const float band = (0.5f * params.RiverWidth) + (RIVER_BAND_W * k.W);
				if (Distance_To_Polyline (center,intent.Points) < (band + reserve_radius)) return true;
				break;
			}

			case HYBRID_INTENT_COAST_GUIDE:
			{
				//	Exterior sea: everything at or beyond the sea boundary `v` on the chosen side.
				const float boundary_v = intent.Guide.Y;
				const float inward = (intent.VSign >= 0) ? ((boundary_v - uv.Y) * scale) : ((uv.Y - boundary_v) * scale);
				if (inward <= reserve_radius) return true;
				break;
			}

			default:
				break;
		}
	}

	return false;
}


/*
**	Each rule list below is exactly the plan's list for that role class.  Phase 4 repairs a moved
**	reservation through this same entry point, which is why it is not static.
*/
bool Hybrid_Role_Is_Hard_Valid(const HybridStrategicGraphParams & params,const HybridMapIdentity & identity,
										 const HybridArchetypeIntentSet & intents,const HybridStrategicGraph & graph,
										 HybridRoleClass role_class,const Vector2 & center,bool exempt_placeable,
										 unsigned int ignore_site_id)
{
	const HybridConstants & k = params.Constants;

	switch (role_class) {

		case HYBRID_ROLE_BASE:
		{
			if (!Reserve_Is_Placeable (params,center,k.RBaseProtected)) return false;
			if (Intersects_Hard_Intent (params,identity,intents,center,k.RBaseProtected,true)) return false;

			//	Neither base's protected disc may touch the other's.
			for (int i = 0; i < graph.Sites.Count (); i++) {
				const HybridStrategicSite & site = graph.Sites[i];
				if (site.StableId == ignore_site_id) continue;
				if ((site.Role != HYBRID_SITE_GDI_BASE) && (site.Role != HYBRID_SITE_NOD_BASE)) continue;
				if (Distance_Between (center,site.Position) < (k.RBaseProtected + site.Radius)) return false;
			}
			return true;
		}

		case HYBRID_ROLE_RESOURCE:
		{
			if (!Reserve_Is_Placeable (params,center,params.FieldRadiusMax)) return false;
			if (Intersects_Hard_Intent (params,identity,intents,center,params.FieldRadiusMax,true)) return false;

			for (int i = 0; i < graph.Sites.Count (); i++) {
				const HybridStrategicSite & site = graph.Sites[i];
				if (site.StableId == ignore_site_id) continue;
				const float distance = Distance_Between (center,site.Position);

				switch (site.Role) {
					case HYBRID_SITE_GDI_BASE:
					case HYBRID_SITE_NOD_BASE:
						if (distance < (site.Radius + params.FieldRadiusMax)) return false;
						break;

					case HYBRID_SITE_HOME_TIBERIUM_GDI:
					case HYBRID_SITE_HOME_TIBERIUM_NOD:
					case HYBRID_SITE_CONTESTED_TIBERIUM:
					case HYBRID_SITE_REMOTE_TIBERIUM:
						//	Minimum center separation between two resource reserves.
						if (distance < (2.0f * params.FieldRadiusMax)) return false;
						break;

					default:
						break;
				}
			}
			return true;
		}

		case HYBRID_ROLE_WAYPOINT:
		default:
		{
			/*
			**	A waypoint's center must be placeable, or lie on its own explicit archetype
			**	crossing/pass intent -- that is what lets a mandatory bridge guide sit on the water it
			**	is required to cross.  Either way it stays out of every Tiberium reserve.
			*/
			if (!exempt_placeable && !Reserve_Is_Placeable (params,center,0.0f)) return false;

			for (int i = 0; i < graph.Sites.Count (); i++) {
				const HybridStrategicSite & site = graph.Sites[i];
				if (site.StableId == ignore_site_id) continue;
				switch (site.Role) {
					case HYBRID_SITE_HOME_TIBERIUM_GDI:
					case HYBRID_SITE_HOME_TIBERIUM_NOD:
					case HYBRID_SITE_CONTESTED_TIBERIUM:
					case HYBRID_SITE_REMOTE_TIBERIUM:
						if (Distance_Between (center,site.Position) < site.Radius) return false;
						break;

					default:
						break;
				}
			}
			return true;
		}
	}
}


/*
**	Section 6.1 bounded legality repair.  The guide itself is tested first -- with its placeability
**	exemption, which is what lets a mandatory crossing guide sit on the obstruction -- and then the
**	shared ring search takes over.  Once repair has moved the role off that guide it is an ordinary
**	point and must be placeable, so the ring test never carries the exemption.  An already accepted
**	role is never moved; this only ever moves the role being placed.
*/
struct HybridPlaceRoleContext
{
	const HybridStrategicGraphParams *	Params;
	const HybridMapIdentity *				Identity;
	const HybridArchetypeIntentSet *		Intents;
	const HybridStrategicGraph *			Graph;
	HybridRoleClass							RoleClass;
};


static bool
Place_Role_Test(const Vector2 & candidate,void * context)
{
	const HybridPlaceRoleContext * c = (const HybridPlaceRoleContext *)context;
	return Hybrid_Role_Is_Hard_Valid (*c->Params,*c->Identity,*c->Intents,*c->Graph,c->RoleClass,candidate,false,0);
}


static bool
Place_Role(const HybridStrategicGraphParams & params,const HybridMapIdentity & identity,
			  const HybridArchetypeIntentSet & intents,const HybridStrategicGraph & graph,
			  HybridRoleClass role_class,const Vector2 & guide_uv,bool exempt_placeable,Vector2 * out_world)
{
	const HybridConstants & k = params.Constants;
	const Vector2 origin = identity.Frame.To_World (guide_uv);

	const Vector2 candidate (Hybrid_Quantize_Position (origin.X),Hybrid_Quantize_Position (origin.Y));
	if (Hybrid_Role_Is_Hard_Valid (params,identity,intents,graph,role_class,candidate,exempt_placeable,0)) {
		*out_world = candidate;
		return true;
	}

	HybridPlaceRoleContext context;
	context.Params		= &params;
	context.Identity	= &identity;
	context.Intents	= &intents;
	context.Graph		= &graph;
	context.RoleClass	= role_class;

	return Hybrid_Repair_Ring_Search (identity.Frame,k.A,ROLE_REPAIR_LIMIT_D * k.D,origin,
												 Place_Role_Test,&context,out_world);
}


static unsigned int
Add_Site(HybridStrategicGraph * graph,HybridSiteRole role,int team,const Vector2 & world,float radius,int route_family)
{
	HybridStrategicSite site;
	site.StableId		= (unsigned int)graph->Sites.Count () + 1;
	site.Role			= role;
	site.Team			= team;
	site.Position.Set (Hybrid_Quantize_Position (world.X),Hybrid_Quantize_Position (world.Y));
	site.Radius			= Hybrid_Quantize_Position (radius);
	site.RouteFamily	= route_family;
	graph->Sites.Add (site);
	return site.StableId;
}


static unsigned int
Add_Edge(HybridStrategicGraph * graph,HybridEdgeIntent intent,unsigned int site_a,unsigned int site_b,
			int route_family,HybridRoadClass road_class)
{
	HybridStrategicEdge edge;
	edge.StableId		= (unsigned int)graph->Edges.Count () + 1;
	edge.Intent			= intent;
	edge.SiteA			= site_a;
	edge.SiteB			= site_b;
	edge.RouteFamily	= route_family;
	edge.RoadClass		= road_class;
	graph->Edges.Add (edge);
	return edge.StableId;
}


/*
**	Section 6.10's river-adjacent contested field.  Walk the frozen guided-river stations from the
**	midpoint outward, negative along-river offset before positive; at each station test the left bank
**	then the right.  The candidate center sits `0.5*river_width + FieldRadiusMax + 7W` off the
**	centerline, so the reserve edge is exactly 7W outside the provisional bank -- inside Section
**	6.2's 4W..10W requirement -- and clear of the river influence band by construction.
*/
static bool
Place_River_Adjacent_Field(const HybridStrategicGraphParams & params,const HybridMapIdentity & identity,
									const HybridArchetypeIntentSet & intents,const HybridStrategicGraph & graph,
									const HybridArchetypeIntent & river,Vector2 * out_world)
{
	const HybridConstants & k = params.Constants;
	const int count = river.Points.Count ();
	if (count <= 0) {
		return false;
	}

	const float offset = (0.5f * params.RiverWidth) + params.FieldRadiusMax + (RIVER_ADJACENT_CLEARANCE_W * k.W);
	const int midpoint = count / 2;

	for (int step = 0; step <= count; step++) {
		for (int side = 0; side < 2; side++) {

			//	Negative along-river offset before positive, so a tie prefers the -v end of the river.
			const int index = (side == 0) ? (midpoint - step) : (midpoint + step);
			if ((index < 0) || (index >= count)) continue;
			if ((step == 0) && (side == 1)) continue;		// the midpoint itself is tested once

			//	Local tangent: central difference where both neighbours exist, one-sided at an end.
			const int before	= (index > 0) ? (index - 1) : index;
			const int after	= (index + 1 < count) ? (index + 1) : index;
			Vector2 tangent (river.Points[after].X - river.Points[before].X,
								  river.Points[after].Y - river.Points[before].Y);
			const float length = tangent.Length ();
			if (length <= 0.0f) continue;
			tangent.X /= length;
			tangent.Y /= length;

			//	Left normal first, then right.
			for (int normal_side = 0; normal_side < 2; normal_side++) {
				const Vector2 normal = (normal_side == 0) ? Vector2 (-tangent.Y,tangent.X)
																		 : Vector2 (tangent.Y,-tangent.X);

				Vector2 candidate (Hybrid_Quantize_Position (river.Points[index].X + (normal.X * offset)),
										 Hybrid_Quantize_Position (river.Points[index].Y + (normal.Y * offset)));

				if (!Reserve_Is_Placeable (params,candidate,params.FieldRadiusMax)) continue;

				//	Section 6.2's own requirement, measured against the whole frozen centerline.
				const float minimum = (0.5f * params.RiverWidth) + params.FieldRadiusMax
											 + (RIVER_ADJACENT_MIN_CLEARANCE_W * k.W);
				if (Distance_To_Polyline (candidate,river.Points) < minimum) continue;

				//	Every non-river archetype hard intent still applies; the river band cannot be hit.
				if (Intersects_Hard_Intent (params,identity,intents,candidate,params.FieldRadiusMax,false)) continue;

				bool clear = true;
				for (int i = 0; (i < graph.Sites.Count ()) && clear; i++) {
					const HybridStrategicSite & site = graph.Sites[i];
					const float distance = Distance_Between (candidate,site.Position);
					switch (site.Role) {
						case HYBRID_SITE_GDI_BASE:
						case HYBRID_SITE_NOD_BASE:
							if (distance < (site.Radius + params.FieldRadiusMax)) clear = false;
							break;
						case HYBRID_SITE_HOME_TIBERIUM_GDI:
						case HYBRID_SITE_HOME_TIBERIUM_NOD:
						case HYBRID_SITE_CONTESTED_TIBERIUM:
						case HYBRID_SITE_REMOTE_TIBERIUM:
							if (distance < (2.0f * params.FieldRadiusMax)) clear = false;
							break;
						default:
							break;
					}
				}
				if (!clear) continue;

				*out_world = candidate;
				return true;
			}
		}
	}

	return false;
}


/*
**	Which obstruction an archetype's crossing guides are about.  Read from the resolved archetype of
**	the frozen identity, so later phases match on the obstruction kind rather than the name.
*/
static HybridObstructionKind
Archetype_Obstruction(HybridArchetype archetype)
{
	switch (archetype) {
		case HYBRID_ARCHETYPE_RIVER_VALLEY:			return HYBRID_OBSTRUCTION_RIVER;
		case HYBRID_ARCHETYPE_TWIN_LAKES:			return HYBRID_OBSTRUCTION_LAKE;
		case HYBRID_ARCHETYPE_CANYON_CROSSROADS:	return HYBRID_OBSTRUCTION_CANYON;
		case HYBRID_ARCHETYPE_HIGHLAND_PASS:		return HYBRID_OBSTRUCTION_RIDGE;
		case HYBRID_ARCHETYPE_COASTAL_PENINSULA:	return HYBRID_OBSTRUCTION_COAST;
		case HYBRID_ARCHETYPE_PLATEAU_BASIN:		return HYBRID_OBSTRUCTION_CLIFF;		// the basin rim
		case HYBRID_ARCHETYPE_RAVINE_NETWORK:		return HYBRID_OBSTRUCTION_RAVINE;
		default:												return HYBRID_OBSTRUCTION_NONE;
	}
}


/***********************************************************************************************
 * Hybrid_Build_Strategic_Graph -- runtime Phase 3                                             *
 *---------------------------------------------------------------------------------------------*/
bool Hybrid_Build_Strategic_Graph(const HybridStrategicGraphParams & params,const HybridMapIdentity & identity,
											 const HybridArchetypeIntentSet & intents,HybridStrategicGraph * graph,
											 HybridStrategicGraphResult * result)
{
	if (graph == nullptr) {
		return Fail (result,HYBRID_GRAPH_ROLE_COUNT_MISMATCH,"null output record");
	}

	graph->Reset ();
	if (result != nullptr) {
		result->Failure	= HYBRID_GRAPH_OK;
		result->Detail		= "";
	}

	const HybridConstants & k = params.Constants;
	const float s = (float)identity.FlankSign;
	StringClass detail (64u,true);
	Vector2 position (0.0f,0.0f);

	/*
	**	1, 2 -- GDI_BASE then NOD_BASE.  Creation order is the plan's priority order: the second base
	**	is tested against the first and moves if it has to, never the other way round.
	*/
	if (!Place_Role (params,identity,intents,*graph,HYBRID_ROLE_BASE,identity.GdiBaseGuide,false,&position)) {
		return Fail (result,HYBRID_GRAPH_NO_LEGAL_BASE_POSITION,"GDI_BASE");
	}
	graph->GdiBaseId = Add_Site (graph,HYBRID_SITE_GDI_BASE,0,position,k.RBaseProtected,-1);

	if (!Place_Role (params,identity,intents,*graph,HYBRID_ROLE_BASE,identity.NodBaseGuide,false,&position)) {
		return Fail (result,HYBRID_GRAPH_NO_LEGAL_BASE_POSITION,"NOD_BASE");
	}
	graph->NodBaseId = Add_Site (graph,HYBRID_SITE_NOD_BASE,1,position,k.RBaseProtected,-1);

	//	3, 4 -- the two home fields.
	if (!Place_Role (params,identity,intents,*graph,HYBRID_ROLE_RESOURCE,identity.GdiHomeFieldGuide,false,&position)) {
		return Fail (result,HYBRID_GRAPH_NO_LEGAL_RESOURCE_POSITION,"HOME_TIBERIUM_GDI");
	}
	const unsigned int gdi_home = Add_Site (graph,HYBRID_SITE_HOME_TIBERIUM_GDI,0,position,params.FieldRadiusMax,-1);

	if (!Place_Role (params,identity,intents,*graph,HYBRID_ROLE_RESOURCE,identity.NodHomeFieldGuide,false,&position)) {
		return Fail (result,HYBRID_GRAPH_NO_LEGAL_RESOURCE_POSITION,"HOME_TIBERIUM_NOD");
	}
	const unsigned int nod_home = Add_Site (graph,HYBRID_SITE_HOME_TIBERIUM_NOD,1,position,params.FieldRadiusMax,-1);

	//	5 -- the FRONT_CENTER analysis node.  A point site: no reserve, no radius.
	if (!Place_Role (params,identity,intents,*graph,HYBRID_ROLE_WAYPOINT,identity.FrontCenterGuide,false,&position)) {
		return Fail (result,HYBRID_GRAPH_NO_LEGAL_WAYPOINT_POSITION,"FRONT_CENTER");
	}
	graph->FrontCenterId = Add_Site (graph,HYBRID_SITE_FRONT_CENTER,-1,position,0.0f,-1);

	/*
	**	6 -- contested fields.  The Phase-3 non-home guide table is fixed; only `river_valley`
	**	replaces the *first* contested guide, and it does so from the already frozen guided river so
	**	no resource center moves once semantic reservations exist.
	*/
	const int contested_count	= Hybrid_Contested_Field_Count (identity.SizeClass);
	const int remote_count		= Hybrid_Remote_Field_Count (identity.SizeClass);

	const float contested_v[5] = {
		0.0f,
		+CONTESTED_MEDIUM_V, -CONTESTED_MEDIUM_V,
		+CONTESTED_LARGE_V, -CONTESTED_LARGE_V
	};

	for (int f = 0; f < contested_count; f++) {

		if ((f == 0) && intents.RiverAdjacentContested) {
			const HybridArchetypeIntent * river = intents.Find_First (HYBRID_INTENT_GUIDED_RIVER);
			if (river == nullptr) {
				return Fail (result,HYBRID_GRAPH_MISSING_GUIDED_RIVER_INTENT,"RIVER_ADJACENT_CONTESTED");
			}
			if (!Place_River_Adjacent_Field (params,identity,intents,*graph,*river,&position)) {
				return Fail (result,HYBRID_GRAPH_NO_RIVER_ADJACENT_FIELD,"first CONTESTED field");
			}
		} else {
			const Vector2 guide (0.0f,Hybrid_Quantize_Normalized (contested_v[f] * s));
			if (!Place_Role (params,identity,intents,*graph,HYBRID_ROLE_RESOURCE,guide,false,&position)) {
				detail.Format ("CONTESTED_TIBERIUM %d",f);
				return Fail (result,HYBRID_GRAPH_NO_LEGAL_RESOURCE_POSITION,detail.Peek_Buffer ());
			}
		}
		Add_Site (graph,HYBRID_SITE_CONTESTED_TIBERIUM,-1,position,params.FieldRadiusMax,-1);
	}

	//	7 -- remote-rich fields.  Their `u` is the plan's one keyed draw in this phase.
	for (int r = 0; r < remote_count; r++) {
		const float h = Hybrid_Hash01 (params.MapPlanSeed,Hybrid_Key ("remote_rich_u"),(unsigned int)r);
		const float u = Hybrid_Range (-REMOTE_FIELD_U_LIMIT,+REMOTE_FIELD_U_LIMIT,h,HYBRID_QUANTUM_NORMALIZED);
		const float v = Hybrid_Quantize_Normalized (((r == 0) ? +REMOTE_FIELD_V : -REMOTE_FIELD_V) * s);

		if (!Place_Role (params,identity,intents,*graph,HYBRID_ROLE_RESOURCE,Vector2 (u,v),false,&position)) {
			detail.Format ("REMOTE_TIBERIUM %d",r);
			return Fail (result,HYBRID_GRAPH_NO_LEGAL_RESOURCE_POSITION,detail.Peek_Buffer ());
		}
		Add_Site (graph,HYBRID_SITE_REMOTE_TIBERIUM,-1,position,params.FieldRadiusMax,-1);
	}

	/*
	**	8 -- route-family waypoints.  Default guide bands first, then the Section-6.10 archetype
	**	replacement: an archetype guide for family 0 or 1 replaces that family's default, two guides
	**	for one family are inserted in increasing source-to-destination `u` order, and family 2 keeps
	**	its default unless the table names it.
	*/
	int family_count = identity.RouteFamilyCount;
	if (family_count < 2) family_count = 2;
	if (family_count > MAX_ROUTE_FAMILIES) family_count = MAX_ROUTE_FAMILIES;

	Vector2 guide_uv[MAX_ROUTE_FAMILIES][MAX_FAMILY_GUIDES];
	int guide_intent[MAX_ROUTE_FAMILIES][MAX_FAMILY_GUIDES];
	int guide_count[MAX_ROUTE_FAMILIES];
	bool guide_from_archetype[MAX_ROUTE_FAMILIES];

	for (int family = 0; family < MAX_ROUTE_FAMILIES; family++) {
		guide_count[family]				= (family < family_count) ? 1 : 0;
		guide_from_archetype[family]	= false;
		for (int g = 0; g < MAX_FAMILY_GUIDES; g++) {
			guide_intent[family][g] = -1;
		}
		//	Default bands: family 0 at (0,0), family 1 at (0,+0.45*s), family 2 at (0,-0.45*s).
		switch (family) {
			case 0:	guide_uv[family][0] = identity.FrontCenterGuide;	break;
			case 1:	guide_uv[family][0] = identity.FlankGuide[0];			break;
			default:	guide_uv[family][0] = identity.FlankGuide[1];			break;
		}
	}

	//	Archetype guides, in intent StableId order, then sorted per family by increasing `u`.
	for (int i = 0; i < intents.Intents.Count (); i++) {
		const HybridArchetypeIntent & intent = intents.Intents[i];
		if ((intent.Kind != HYBRID_INTENT_ROUTE_FAMILY_CROSSING) && (intent.Kind != HYBRID_INTENT_BASIN_ENTRY)) continue;
		if ((intent.RouteFamily < 0) || (intent.RouteFamily >= family_count)) continue;

		const int family = intent.RouteFamily;

		//	The first archetype guide for a family replaces that family's default guide entirely.
		if (!guide_from_archetype[family]) {
			guide_from_archetype[family]	= true;
			guide_count[family]				= 0;
		}
		if (guide_count[family] >= MAX_FAMILY_GUIDES) continue;

		int slot = guide_count[family];
		while ((slot > 0) && (guide_uv[family][slot - 1].X > intent.Guide.X)) {
			guide_uv[family][slot]		= guide_uv[family][slot - 1];
			guide_intent[family][slot]	= guide_intent[family][slot - 1];
			slot--;
		}
		guide_uv[family][slot]		= intent.Guide;
		guide_intent[family][slot]	= i;
		guide_count[family]++;
	}

	unsigned int family_waypoint[MAX_ROUTE_FAMILIES][MAX_FAMILY_GUIDES];
	for (int family = 0; family < family_count; family++) {
		for (int g = 0; g < guide_count[family]; g++) {

			const bool exempt = (guide_intent[family][g] >= 0);
			if (!Place_Role (params,identity,intents,*graph,HYBRID_ROLE_WAYPOINT,guide_uv[family][g],exempt,&position)) {
				detail.Format ("ROUTE_WAYPOINT family %d guide %d",family,g);
				return Fail (result,HYBRID_GRAPH_NO_LEGAL_WAYPOINT_POSITION,detail.Peek_Buffer ());
			}
			family_waypoint[family][g] = Add_Site (graph,HYBRID_SITE_ROUTE_WAYPOINT,-1,position,0.0f,family);
		}
	}

	/*
	**	`CROSSING_OBLIGATION_INTENT` records: one per archetype guide tagged bridge or
	**	distinct-crossing, plus the one mandatory `SURFACE_SADDLE` pass.  The tuple is exactly
	**	(family id, obstruction kind, strategic guide, required method or ANY).
	*/
	const HybridObstructionKind obstruction = Archetype_Obstruction (identity.Archetype);

	for (int family = 0; family < family_count; family++) {
		for (int g = 0; g < guide_count[family]; g++) {

			if (guide_intent[family][g] < 0) continue;
			const HybridArchetypeIntent & intent = intents.Intents[guide_intent[family][g]];
			const bool bridge		= (intent.Flags & HYBRID_INTENT_FLAG_BRIDGE_REQUIRED) != 0;
			const bool distinct	= (intent.Flags & HYBRID_INTENT_FLAG_DISTINCT_CROSSING_REQUIRED) != 0;
			if (!bridge && !distinct) continue;

			HybridCrossingObligation obligation;
			obligation.StableId						= (unsigned int)graph->Obligations.Count () + 1;
			obligation.RouteFamily					= family;
			obligation.Obstruction					= obstruction;
			obligation.RequiredMethod				= bridge ? HYBRID_CROSSING_BRIDGE : HYBRID_CROSSING_ANY;
			obligation.DistinctCrossingRequired	= distinct;
			obligation.Guide							= intent.Guide;
			obligation.IntentStableId				= intent.StableId;
			obligation.WaypointStableId			= family_waypoint[family][g];
			graph->Obligations.Add (obligation);
		}
	}

	/*
	**	`highland_pass`'s surface saddle is deliberately *not* a crossing obligation.
	**
	**	Section 6.10 realizes the saddle by multiplying the principal ridge's amplitude down around the
	**	saddle point, and Phase 5 widens that flat until it holds a `PRIMARY_2` roadbed plus
	**	`2 * fairness.min_vehicle_lane_clearance`.  The road therefore crosses the ridge on ordinary drivable
	**	ground, which needs no structure and no Phase-6 window -- and step 18 creates windows for rivers,
	**	cliffs, canyons and ravines only, never ridges.  Binding a `RIDGE` obligation here asked Phase 9 for
	**	a window that by specification cannot exist, and every `highland_pass` seed failed
	**	`obligation_unbound` with the census reporting all 59 to 150 windows as "wrong obstruction".
	*/

	/*
	**	Each family is one complete GDI-to-Nod intent through its ordered guides.  Two families share
	**	only the base nodes -- every intermediate waypoint object is the family's own -- so
	**	`route_family_count >= 2` leaves a cycle by construction.
	*/
	for (int family = 0; family < family_count; family++) {
		const HybridEdgeIntent intent = (family == 0) ? HYBRID_EDGE_MAIN_ROUTE : HYBRID_EDGE_FLANK_ROUTE;
		unsigned int previous = graph->GdiBaseId;
		for (int g = 0; g < guide_count[family]; g++) {
			Add_Edge (graph,intent,previous,family_waypoint[family][g],family,HYBRID_ROAD_CLASS_COUNT);
			previous = family_waypoint[family][g];
		}
		Add_Edge (graph,intent,previous,graph->NodBaseId,family,HYBRID_ROAD_CLASS_COUNT);
	}

	//	Resource intents: each base to its home field.  Phase 9 picks that road's class.
	Add_Edge (graph,HYBRID_EDGE_RESOURCE_ROUTE,graph->GdiBaseId,gdi_home,-1,HYBRID_ROAD_CLASS_COUNT);
	Add_Edge (graph,HYBRID_EDGE_RESOURCE_ROUTE,graph->NodBaseId,nod_home,-1,HYBRID_ROAD_CLASS_COUNT);

	/*
	**	Every non-home field attaches to one heavy route-family waypoint and receives one
	**	`SERVICE_DIRT_2` branch intent from that attachment point.  Contested fields take the nearest
	**	waypoint outright; a remote-rich field takes the nearest on its own signed-`v` half and falls
	**	back to the globally nearest only when that half has none.  Ties resolve by
	**	(quantized distance, family id, waypoint StableId).
	*/
	for (int i = 0; i < graph->Sites.Count (); i++) {

		const HybridStrategicSite & field = graph->Sites[i];
		if ((field.Role != HYBRID_SITE_CONTESTED_TIBERIUM) && (field.Role != HYBRID_SITE_REMOTE_TIBERIUM)) continue;

		const bool remote = (field.Role == HYBRID_SITE_REMOTE_TIBERIUM);
		const float field_v = identity.Frame.To_Strategic (field.Position).Y;

		unsigned int best_id			= 0;
		int best_family				= 0;
		float best_distance			= 0.0f;
		unsigned int any_id			= 0;
		int any_family					= 0;
		float any_distance			= 0.0f;

		for (int w = 0; w < graph->Sites.Count (); w++) {

			const HybridStrategicSite & waypoint = graph->Sites[w];
			if (waypoint.Role != HYBRID_SITE_ROUTE_WAYPOINT) continue;

			const float distance = Hybrid_Quantize_Position (Distance_Between (field.Position,waypoint.Position));
			const bool better_any = (any_id == 0) || (distance < any_distance) ||
											((distance == any_distance) &&
											 ((waypoint.RouteFamily < any_family) ||
											  ((waypoint.RouteFamily == any_family) && (waypoint.StableId < any_id))));
			if (better_any) {
				any_id			= waypoint.StableId;
				any_family		= waypoint.RouteFamily;
				any_distance	= distance;
			}

			if (remote) {
				const float waypoint_v = identity.Frame.To_Strategic (waypoint.Position).Y;
				const bool same_half = ((field_v >= 0.0f) == (waypoint_v >= 0.0f));
				if (!same_half) continue;
			}

			const bool better = (best_id == 0) || (distance < best_distance) ||
									  ((distance == best_distance) &&
										((waypoint.RouteFamily < best_family) ||
										 ((waypoint.RouteFamily == best_family) && (waypoint.StableId < best_id))));
			if (better) {
				best_id			= waypoint.StableId;
				best_family		= waypoint.RouteFamily;
				best_distance	= distance;
			}
		}

		if (best_id == 0) {
			best_id = any_id;			// remote field with no waypoint on its own half
		}
		if (best_id == 0) {
			detail.Format ("field %u has no route waypoint to attach to",field.StableId);
			return Fail (result,HYBRID_GRAPH_MISSING_RESOURCE_INTENT,detail.Peek_Buffer ());
		}

		Add_Edge (graph,HYBRID_EDGE_RESOURCE_ROUTE,best_id,field.StableId,-1,HYBRID_ROAD_SERVICE_DIRT_2);
	}

	/*
	**	Phase 3 gate.  Exact role counts, `route_family_count` complete GDI-to-Nod intents, resource
	**	intents attached, and at least one cycle -- a tree fails.
	*/
	if ((graph->Count_Of_Role (HYBRID_SITE_GDI_BASE) != 1) ||
		 (graph->Count_Of_Role (HYBRID_SITE_NOD_BASE) != 1) ||
		 (graph->Count_Of_Role (HYBRID_SITE_HOME_TIBERIUM_GDI) != 1) ||
		 (graph->Count_Of_Role (HYBRID_SITE_HOME_TIBERIUM_NOD) != 1) ||
		 (graph->Count_Of_Role (HYBRID_SITE_FRONT_CENTER) != 1) ||
		 (graph->Count_Of_Role (HYBRID_SITE_CONTESTED_TIBERIUM) != contested_count) ||
		 (graph->Count_Of_Role (HYBRID_SITE_REMOTE_TIBERIUM) != remote_count)) {

		detail.Format ("roles %d/%d/%d/%d/%d/%d/%d",
							graph->Count_Of_Role (HYBRID_SITE_GDI_BASE),graph->Count_Of_Role (HYBRID_SITE_NOD_BASE),
							graph->Count_Of_Role (HYBRID_SITE_HOME_TIBERIUM_GDI),graph->Count_Of_Role (HYBRID_SITE_HOME_TIBERIUM_NOD),
							graph->Count_Of_Role (HYBRID_SITE_FRONT_CENTER),graph->Count_Of_Role (HYBRID_SITE_CONTESTED_TIBERIUM),
							graph->Count_Of_Role (HYBRID_SITE_REMOTE_TIBERIUM));
		return Fail (result,HYBRID_GRAPH_ROLE_COUNT_MISMATCH,detail.Peek_Buffer ());
	}

	for (int family = 0; family < family_count; family++) {

		unsigned int expected = graph->GdiBaseId;
		int links = 0;
		for (int e = 0; e < graph->Edges.Count (); e++) {
			const HybridStrategicEdge & edge = graph->Edges[e];
			if (edge.RouteFamily != family) continue;
			if (edge.SiteA != expected) {
				detail.Format ("family %d intent chain broken at edge %u",family,edge.StableId);
				return Fail (result,HYBRID_GRAPH_INCOMPLETE_ROUTE_FAMILY,detail.Peek_Buffer ());
			}
			expected = edge.SiteB;
			links++;
		}
		if ((links != (guide_count[family] + 1)) || (expected != graph->NodBaseId)) {
			detail.Format ("family %d has %d link(s) and ends at site %u",family,links,expected);
			return Fail (result,HYBRID_GRAPH_INCOMPLETE_ROUTE_FAMILY,detail.Peek_Buffer ());
		}
	}

	for (int i = 0; i < graph->Sites.Count (); i++) {

		const HybridStrategicSite & site = graph->Sites[i];
		const bool home = (site.Role == HYBRID_SITE_HOME_TIBERIUM_GDI) || (site.Role == HYBRID_SITE_HOME_TIBERIUM_NOD);
		const bool field = home || (site.Role == HYBRID_SITE_CONTESTED_TIBERIUM) || (site.Role == HYBRID_SITE_REMOTE_TIBERIUM);
		if (!field) continue;

		bool attached = false;
		bool branch = false;
		for (int e = 0; e < graph->Edges.Count (); e++) {
			const HybridStrategicEdge & edge = graph->Edges[e];
			if (edge.Intent != HYBRID_EDGE_RESOURCE_ROUTE) continue;
			if ((edge.SiteA != site.StableId) && (edge.SiteB != site.StableId)) continue;
			attached = true;
			if (edge.RoadClass == HYBRID_ROAD_SERVICE_DIRT_2) branch = true;
		}
		if (!attached || (!home && !branch)) {
			detail.Format ("field %u attached=%d service_branch=%d",site.StableId,(int)attached,(int)branch);
			return Fail (result,HYBRID_GRAPH_MISSING_RESOURCE_INTENT,detail.Peek_Buffer ());
		}
	}

	if (!graph->Has_Cycle ()) {
		return Fail (result,HYBRID_GRAPH_NO_GRAPH_CYCLE,"heavy families left a tree");
	}

	graph->Checksum = Hybrid_Strategic_Graph_Checksum (*graph,0);

	WWDEBUG_SAY (("HybridStrategicGraph: %d site(s) %d edge(s) %d obligation(s) families=%d contested=%d remote=%d "
					  "checksum=%08X\n",graph->Sites.Count (),graph->Edges.Count (),graph->Obligations.Count (),
					  family_count,contested_count,remote_count,graph->Checksum));
	return true;
}


unsigned int Hybrid_Strategic_Graph_Checksum(const HybridStrategicGraph & graph,unsigned int seed)
{
	HybridHashStream h (seed);

	h.Add_Int (graph.Sites.Count ());
	for (int i = 0; i < graph.Sites.Count (); i++) {
		const HybridStrategicSite & site = graph.Sites[i];
		h.Add_Uint (site.StableId);
		h.Add_Int ((int)site.Role);
		h.Add_Int (site.Team);
		h.Add_Int (site.RouteFamily);
		h.Add_Length (site.Position.X);
		h.Add_Length (site.Position.Y);
		h.Add_Length (site.Radius);
	}

	h.Add_Int (graph.Edges.Count ());
	for (int i = 0; i < graph.Edges.Count (); i++) {
		const HybridStrategicEdge & edge = graph.Edges[i];
		h.Add_Uint (edge.StableId);
		h.Add_Int ((int)edge.Intent);
		h.Add_Uint (edge.SiteA);
		h.Add_Uint (edge.SiteB);
		h.Add_Int (edge.RouteFamily);
		h.Add_Int ((int)edge.RoadClass);
	}

	h.Add_Int (graph.Obligations.Count ());
	for (int i = 0; i < graph.Obligations.Count (); i++) {
		const HybridCrossingObligation & obligation = graph.Obligations[i];
		h.Add_Uint (obligation.StableId);
		h.Add_Int (obligation.RouteFamily);
		h.Add_Int ((int)obligation.Obstruction);
		h.Add_Int ((int)obligation.RequiredMethod);
		h.Add_Bool (obligation.DistinctCrossingRequired);
		h.Add_Scalar (obligation.Guide.X);
		h.Add_Scalar (obligation.Guide.Y);
		h.Add_Uint (obligation.IntentStableId);
		h.Add_Uint (obligation.WaypointStableId);
	}

	return h.Get ();
}


/***********************************************************************************************
 * Hybrid_Planning_Distance -- Section 0.10's PlanningDistance over the frozen graph             *
 *                                                                                             *
 * Dijkstra with quantized straight-line edge lengths.  The relaxation order is by (distance,    *
 * StableId) so two equal-length paths always resolve the same way, and the answer is quantized   *
 * to `Q` because every fairness comparison downstream is made on quantized values.              *
 *=============================================================================================*/
float Hybrid_Planning_Distance(const HybridStrategicGraph & graph,unsigned int site_a,unsigned int site_b)
{
	if (site_a == site_b) return 0.0f;

	const int count = graph.Sites.Count ();
	if (count <= 0) return -1.0f;

	DynamicVectorClass<float> distance;
	DynamicVectorClass<unsigned char> settled;
	for (int i = 0; i < count; i++) {
		distance.Add (-1.0f);
		settled.Add (0);
	}

	int start = -1;
	int target = -1;
	for (int i = 0; i < count; i++) {
		if (graph.Sites[i].StableId == site_a) start = i;
		if (graph.Sites[i].StableId == site_b) target = i;
	}
	if ((start < 0) || (target < 0)) return -1.0f;

	distance[start] = 0.0f;

	for (;;) {

		int next = -1;
		for (int i = 0; i < count; i++) {
			if (settled[i] != 0) continue;
			if (distance[i] < 0.0f) continue;
			if (next < 0) { next = i; continue; }
			if (distance[i] < distance[next]) { next = i; continue; }
			//	Equal distances resolve by StableId, so the walk is total and reproducible.
			if ((distance[i] == distance[next]) && (graph.Sites[i].StableId < graph.Sites[next].StableId)) next = i;
		}
		if (next < 0) break;
		if (next == target) break;
		settled[next] = 1;

		for (int e = 0; e < graph.Edges.Count (); e++) {

			const HybridStrategicEdge & edge = graph.Edges[e];
			unsigned int other_id = 0;
			if (edge.SiteA == graph.Sites[next].StableId)			other_id = edge.SiteB;
			else if (edge.SiteB == graph.Sites[next].StableId)	other_id = edge.SiteA;
			else																continue;

			int other = -1;
			for (int i = 0; i < count; i++) {
				if (graph.Sites[i].StableId == other_id) { other = i; break; }
			}
			if (other < 0) continue;

			const float length = Hybrid_Quantize_Position ((graph.Sites[other].Position -
																			 graph.Sites[next].Position).Length ());
			const float through = distance[next] + length;
			if ((distance[other] < 0.0f) || (through < distance[other])) distance[other] = through;
		}
	}

	if (distance[target] < 0.0f) return -1.0f;
	return Hybrid_Quantize_Position (distance[target]);
}
