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
 *                     $Archive:: /Commando/Code/wwphys/hybridanchors.cpp                     $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Runtime Phase 8.  Every check below is a *validation* of something an earlier phase already   *
 * fixed: no centre moves, no value is drawn, and a hard requirement the realized world does not  *
 * meet fails the attempt.  See hybridanchors.h for why.                                        *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#include "hybridanchors.h"

#include "wwdebug.h"
#include "wwmath.h"

#include <math.h>
#include <string.h>


static inline float Max2(float a,float b)	{ return (a > b) ? a : b; }
static inline float Min2(float a,float b)	{ return (a < b) ? a : b; }


/*
**	Section 8 Phase 8's own constants, each used exactly once below.
*/
static const float	EXIT_SWEEP_W				= 6.0f;		// radially from R_BASE_PROTECTED to +6W
static const int		EXIT_LATERAL_SAMPLES		= 5;			// across the swept rectangle, ends included
static const float	SECTOR_DEGREES				= 360.0f / (float)HYBRID_EXIT_SECTOR_COUNT;
static const float	PI_OVER_180					= 3.14159265358979323846f / 180.0f;


const char * Hybrid_Anchor_Failure_Name(HybridAnchorFailure failure)
{
	switch (failure) {
		case HYBRID_ANCHOR_OK:							return "ok";
		case HYBRID_ANCHOR_INVALID_INPUT:			return "invalid_input";
		case HYBRID_ANCHOR_MISSING_ROLE:				return "missing_role";
		case HYBRID_ANCHOR_BASE_NOT_PLACEABLE:		return "base_not_placeable";
		case HYBRID_ANCHOR_BASE_HARD_OVERLAP:		return "base_hard_overlap";
		case HYBRID_ANCHOR_BASE_TOO_STEEP:			return "base_too_steep";
		case HYBRID_ANCHOR_BASE_NO_EXPANSION:		return "base_no_expansion";
		case HYBRID_ANCHOR_BASE_NO_EXITS:			return "base_no_exits";
		case HYBRID_ANCHOR_RESOURCE_ILLEGAL:		return "resource_illegal";
		case HYBRID_ANCHOR_RESOURCE_NO_APPROACH:	return "resource_no_approach";
		case HYBRID_ANCHOR_FAIRNESS_HOME:			return "fairness_home";
		case HYBRID_ANCHOR_FAIRNESS_FRONT:			return "fairness_front";
		case HYBRID_ANCHOR_BASE_TO_BASE_ZERO:		return "base_to_base_zero";
		default:												break;
	}
	return "unknown";
}


HybridAnchorParams::HybridAnchorParams(void) :
	SeaLevel(0.0f),MaxBuildableSlopeRadians(0.4886f),FieldRadiusMax(60.0f),
	ReferenceVehicleSpeed(10.0f),
	MaxInitialResourceTravelTimeDifferenceSeconds(15.0f),
	MaxBaseToFrontTravelTimeDifferenceSeconds(20.0f),
	MinBaseExpansionArea(4096.0f),MinVehicleApproachCount(2),MinVehicleLaneClearance(1.0f),
	HeavyVehicleWidth(6.8f)
{
	memset(&Constants,0,sizeof(Constants));
}


void HybridAnchorSet::Reset(void)
{
	Bases.Delete_All();
	Resources.Delete_All();
	for (int team = 0; team < 2; ++team) {
		HomeFieldDistance[team]	= -1.0f;
		HomeFieldTime[team]		= 0.0f;
		FrontDistance[team]		= -1.0f;
		FrontTime[team]			= 0.0f;
	}
	BaseToBaseDistance	= 0.0f;
	HomeTimeDifference	= 0.0f;
	FrontTimeDifference	= 0.0f;
	Frozen					= false;
	Checksum					= 0;
}


const HybridBaseAnchor * HybridAnchorSet::Find_Base(unsigned int site_id) const
{
	for (int i = 0; i < Bases.Count(); ++i) {
		if (Bases[i].SiteId == site_id) return &Bases[i];
	}
	return nullptr;
}


const HybridBaseAnchor * HybridAnchorSet::Base_Of_Team(int team) const
{
	for (int i = 0; i < Bases.Count(); ++i) {
		if (Bases[i].Team == team) return &Bases[i];
	}
	return nullptr;
}


const HybridResourceAnchor * HybridAnchorSet::Find_Resource(unsigned int site_id) const
{
	for (int i = 0; i < Resources.Count(); ++i) {
		if (Resources[i].SiteId == site_id) return &Resources[i];
	}
	return nullptr;
}


int HybridAnchorSet::Count_Of_Role(HybridSiteRole role) const
{
	int count = 0;
	for (int i = 0; i < Resources.Count(); ++i) {
		if (Resources[i].Role == role) count++;
	}
	return count;
}


Vector2 Hybrid_Exit_Sector_Direction(const HybridStrategicFrame & frame,int ordinal)
{
	//	Ordinal 0 is strategic +u and ordinals increase counter-clockwise by 22.5 degrees.
	const float angle = (float)ordinal * SECTOR_DEGREES * PI_OVER_180;
	const float c = (float)cos((double)angle);
	const float s = (float)sin((double)angle);
	return Vector2((frame.AxisU.X * c) + (frame.AxisV.X * s),(frame.AxisU.Y * c) + (frame.AxisV.Y * s));
}


/***********************************************************************************************
**	The validator.
***********************************************************************************************/

struct AnchorValidator
{
	const HybridAnchorParams *			P;
	const HybridMapIdentity *			Identity;
	const HybridStrategicGraph *		Graph;
	const HybridCoarseMatrices *		Matrices;
	const HybridLandformSet *			Landforms;
	const HybridHydrologySet *			Hydrology;
	HybridAnchorHeightSampleFunc		Sample;
	void *									Context;
	HybridAnchorSet *						Out;
};


static bool Fail(HybridAnchorResult * result,HybridAnchorFailure failure,const char * detail)
{
	if (result != nullptr) {
		result->Failure	= failure;
		result->Detail		= (detail != nullptr) ? detail : "";
	}
	return false;
}


static bool Height_At(const AnchorValidator & v,const Vector2 & world,float * height_out)
{
	return v.Sample(world,height_out,v.Context);
}


//	The two shared predicates, which live with the records they read rather than once per phase.
static bool Inside_Immutable_Wall(const AnchorValidator & v,const Vector2 & world)
{
	return Hybrid_Point_In_Immutable_Landform(*v.Landforms,world);
}


static bool Inside_Water(const AnchorValidator & v,const Vector2 & world)
{
	return Hybrid_Point_In_Water(*v.Hydrology,world);
}


//	Is the coarse cell under this point placeable land?
static bool Is_Placeable(const AnchorValidator & v,const Vector2 & world)
{
	const HybridSemanticMatrix & matrix = v.Matrices->Matrix;
	int cx = 0, cy = 0;
	if (!matrix.World_To_Cell(world,&cx,&cy)) return false;
	return matrix.Has_Flag(cx,cy,HYBRID_FLAG_PLACEABLE) && matrix.Has_Flag(cx,cy,HYBRID_FLAG_LAND);
}


/*
**	The realized ground slope at a point, from the written heightfield: the larger of the two axis
**	gradients over one analysis spacing, as an angle.  Sampling the heightfield rather than the
**	formula is deliberate -- Phase 8 validates the ground that exists, including whatever the
**	ordinary-slope relaxation pass left behind.
*/
static bool Slope_At(const AnchorValidator & v,const Vector2 & world,float * radians_out)
{
	const float step = Max2(0.5f * v.P->Constants.A,v.P->Constants.Q);
	float west = 0.0f, east = 0.0f, south = 0.0f, north = 0.0f;

	if (!Height_At(v,Vector2(world.X - step,world.Y),&west)) return false;
	if (!Height_At(v,Vector2(world.X + step,world.Y),&east)) return false;
	if (!Height_At(v,Vector2(world.X,world.Y - step),&south)) return false;
	if (!Height_At(v,Vector2(world.X,world.Y + step),&north)) return false;

	const float dx = (east - west) / (2.0f * step);
	const float dy = (north - south) / (2.0f * step);
	const float gradient = (float)sqrt((double)((dx * dx) + (dy * dy)));
	*radians_out = (float)atan((double)gradient);
	return true;
}


/*
**	Step "possible heavy exit sectors": a heavy swept rectangle of width
**	`heavy_vehicle_width + 2*min_vehicle_lane_clearance` travelling radially from `R_BASE_PROTECTED`
**	out to `R_BASE_PROTECTED + 6W`, with no water or immutable wall collision and slope within the
**	`PRIMARY_2` maximum grade.
*/
static bool Sector_Is_Viable(const AnchorValidator & v,const Vector2 & center,float protected_radius,int ordinal)
{
	const HybridConstants & k = v.P->Constants;
	const Vector2 direction = Hybrid_Exit_Sector_Direction(v.Identity->Frame,ordinal);
	const Vector2 lateral(-direction.Y,direction.X);

	const float width = v.P->HeavyVehicleWidth + (2.0f * v.P->MinVehicleLaneClearance);
	const float half_width = 0.5f * width;
	const float reach = protected_radius + (EXIT_SWEEP_W * k.W);
	const float step = Max2(0.5f * k.A,k.Q);
	const float max_grade = (float)tan((double)(Hybrid_Road_Class_Spec(HYBRID_ROAD_PRIMARY_2).MaxGradeDegrees *
															 PI_OVER_180));

	float previous_center_height = 0.0f;
	bool have_previous = false;

	for (float along = protected_radius; along <= reach; along += step) {

		const Vector2 spine = center + (direction * along);

		for (int lateral_index = 0; lateral_index < EXIT_LATERAL_SAMPLES; ++lateral_index) {

			const float offset = -half_width + ((width * (float)lateral_index) /
															(float)(EXIT_LATERAL_SAMPLES - 1));
			const Vector2 point = spine + (lateral * offset);

			if (!Is_Placeable(v,point))				return false;
			if (Inside_Water(v,point))					return false;
			if (Inside_Immutable_Wall(v,point))		return false;

			float height = 0.0f;
			if (!Height_At(v,point,&height))			return false;
		}

		//	The grade a heavy vehicle would actually climb along the sector's spine.
		float height = 0.0f;
		if (!Height_At(v,spine,&height)) return false;
		if (have_previous && ((float)fabs((double)(height - previous_center_height)) > (max_grade * step))) {
			return false;
		}
		previous_center_height = height;
		have_previous = true;
	}
	return true;
}


/*
**	The clear expansion area inside the protected disc, measured the way the existing owner sizes it:
**	`BaseSelectionPlanner` turns `fairness.min_base_expansion_area` into a radius with
**	`sqrt(area/pi)` -- Section 0.5's `R_EXPAND`, which is already part of `R_BASE_PROTECTED` -- so the
**	question Phase 8 asks is how much of that same disc is actually clear buildable land.
*/
static float Measure_Expansion_Area(const AnchorValidator & v,const HybridBaseAnchor & base,
												float * max_core_slope_out,bool * core_sampled_out)
{
	const float step = Max2(0.5f * v.P->Constants.A,v.P->Constants.Q);
	const float cell = step * step;
	float area = 0.0f;
	float worst_core = 0.0f;
	bool core_sampled = false;

	for (float y = base.Center.Y - base.ProtectedRadius; y <= (base.Center.Y + base.ProtectedRadius); y += step) {
		for (float x = base.Center.X - base.ProtectedRadius; x <= (base.Center.X + base.ProtectedRadius); x += step) {

			const Vector2 point(x,y);
			const float distance = (point - base.Center).Length();
			if (distance > base.ProtectedRadius) continue;

			float slope = 0.0f;
			const bool sampled = Slope_At(v,point,&slope);

			if (sampled && (distance <= base.CoreRadius)) {
				worst_core = Max2(worst_core,slope);
				core_sampled = true;
			}

			if (!sampled)								continue;
			if (!Is_Placeable(v,point))			continue;
			if (Inside_Water(v,point))				continue;
			if (Inside_Immutable_Wall(v,point))	continue;
			if (slope > v.P->MaxBuildableSlopeRadians) continue;
			area += cell;
		}
	}

	if (max_core_slope_out != nullptr)	*max_core_slope_out = worst_core;
	if (core_sampled_out != nullptr)		*core_sampled_out = core_sampled;
	return area;
}


static bool Validate_Base(AnchorValidator & v,const HybridStrategicSite & site,int team,
									HybridAnchorResult * result)
{
	const HybridConstants & k = v.P->Constants;
	StringClass detail;

	HybridBaseAnchor base;
	base.SiteId				= site.StableId;
	base.Team				= team;
	base.Center				= site.Position;
	base.ProtectedRadius	= k.RBaseProtected;
	base.CoreRadius		= Min2(k.RBase,k.RBaseProtected);

	/*
	**	The Phase-4 reservation is the authority for this centre and radius, so the frozen record is
	**	taken from it rather than recomputed: a reservation that no longer matches the site would mean
	**	an earlier phase moved something, which is exactly what this phase exists to refuse.
	*/
	for (int r = 0; r < v.Matrices->Reservations.Count(); ++r) {
		const HybridReservation & reservation = v.Matrices->Reservations[r];
		if (reservation.Priority != HYBRID_RESERVE_BASE_PROTECTED) continue;
		if (reservation.SiteId != site.StableId) continue;
		base.Center				= reservation.A;
		base.ProtectedRadius	= reservation.Radius;
		break;
	}

	if ((base.Center - site.Position).Length() > k.Q) {
		detail.Format("base %u sits %.3f from its own reservation centre",site.StableId,
						  (base.Center - site.Position).Length());
		return Fail(result,HYBRID_ANCHOR_BASE_NOT_PLACEABLE,detail.Peek_Buffer());
	}

	//	The whole protected disc must be placeable land, with no water or immutable wall in it.
	const float step = Max2(0.5f * k.A,k.Q);
	for (float y = base.Center.Y - base.ProtectedRadius; y <= (base.Center.Y + base.ProtectedRadius); y += step) {
		for (float x = base.Center.X - base.ProtectedRadius; x <= (base.Center.X + base.ProtectedRadius); x += step) {

			const Vector2 point(x,y);
			if ((point - base.Center).Length() > base.ProtectedRadius) continue;

			if (!Is_Placeable(v,point)) {
				detail.Format("base %u protected disc reaches unplaceable ground at %.1f,%.1f",site.StableId,
								  point.X,point.Y);
				return Fail(result,HYBRID_ANCHOR_BASE_NOT_PLACEABLE,detail.Peek_Buffer());
			}
			if (Inside_Water(v,point)) {
				detail.Format("base %u protected disc reaches water at %.1f,%.1f",site.StableId,point.X,point.Y);
				return Fail(result,HYBRID_ANCHOR_BASE_HARD_OVERLAP,detail.Peek_Buffer());
			}
			if (Inside_Immutable_Wall(v,point)) {
				detail.Format("base %u protected disc reaches an immutable wall at %.1f,%.1f",site.StableId,
								  point.X,point.Y);
				return Fail(result,HYBRID_ANCHOR_BASE_HARD_OVERLAP,detail.Peek_Buffer());
			}
		}
	}

	//	Slope over the starting-building proxy area, and the clear expansion area in the same disc.
	bool core_sampled = false;
	base.ExpansionArea = Measure_Expansion_Area(v,base,&base.MaxCoreSlopeRadians,&core_sampled);

	if (!core_sampled) {
		detail.Format("base %u proxy area is outside the realized heightfield",site.StableId);
		return Fail(result,HYBRID_ANCHOR_BASE_NOT_PLACEABLE,detail.Peek_Buffer());
	}
	if (base.MaxCoreSlopeRadians > v.P->MaxBuildableSlopeRadians) {
		detail.Format("base %u proxy area reaches %.2f degrees, past the %.2f buildable limit",site.StableId,
						  base.MaxCoreSlopeRadians / PI_OVER_180,v.P->MaxBuildableSlopeRadians / PI_OVER_180);
		return Fail(result,HYBRID_ANCHOR_BASE_TOO_STEEP,detail.Peek_Buffer());
	}
	if (base.ExpansionArea < v.P->MinBaseExpansionArea) {
		detail.Format("base %u has %.0f of clear expansion area, fairness needs %.0f",site.StableId,
						  base.ExpansionArea,v.P->MinBaseExpansionArea);
		return Fail(result,HYBRID_ANCHOR_BASE_NO_EXPANSION,detail.Peek_Buffer());
	}

	//	The sixteen exit sectors, frozen as a sorted ordinal list.
	for (int ordinal = 0; ordinal < HYBRID_EXIT_SECTOR_COUNT; ++ordinal) {
		if (!Sector_Is_Viable(v,base.Center,base.ProtectedRadius,ordinal)) continue;
		base.ViableSectorMask |= (1u << ordinal);
		base.ViableSectors.Add(ordinal);
	}

	const int required = (v.P->MinVehicleApproachCount > 2) ? v.P->MinVehicleApproachCount : 2;
	if (base.ViableSectors.Count() < required) {
		detail.Format("base %u has %d viable heavy exit sectors, fairness needs %d",site.StableId,
						  base.ViableSectors.Count(),required);
		return Fail(result,HYBRID_ANCHOR_BASE_NO_EXITS,detail.Peek_Buffer());
	}

	v.Out->Bases.Add(base);
	return true;
}


/*
**	A field anchor is legal when its full `resource.field_radius_max` reserve is dry and placeable,
**	outside every immutable wall and outside `BASE_PROTECTED`, and at least one of its Phase-3 route
**	corridors can reach the reserve edge without an immutable obstruction in the way.
*/
static bool Validate_Resource(AnchorValidator & v,const HybridStrategicSite & site,HybridAnchorResult * result)
{
	const HybridConstants & k = v.P->Constants;
	StringClass detail;

	HybridResourceAnchor anchor;
	anchor.SiteId			= site.StableId;
	anchor.Role				= site.Role;
	anchor.Team				= site.Team;
	anchor.Center			= site.Position;
	anchor.ReserveRadius	= Max2(v.P->FieldRadiusMax,site.Radius);

	const float step = Max2(0.5f * k.A,k.Q);

	for (float y = anchor.Center.Y - anchor.ReserveRadius; y <= (anchor.Center.Y + anchor.ReserveRadius); y += step) {
		for (float x = anchor.Center.X - anchor.ReserveRadius; x <= (anchor.Center.X + anchor.ReserveRadius); x += step) {

			const Vector2 point(x,y);
			if ((point - anchor.Center).Length() > anchor.ReserveRadius) continue;

			if (!Is_Placeable(v,point) || Inside_Water(v,point) || Inside_Immutable_Wall(v,point)) {
				detail.Format("field %u reserve is illegal at %.1f,%.1f",site.StableId,point.X,point.Y);
				return Fail(result,HYBRID_ANCHOR_RESOURCE_ILLEGAL,detail.Peek_Buffer());
			}

			/*
			**	"Outside BASE_PROTECTED" is measured against the frozen protected *disc*, not against the
			**	rasterized coarse flag: a cell is `S` wide, so a cell whose centre is inside the disc is
			**	flagged for its whole area and a reserve edge that legally clears the disc by less than
			**	half a cell would be refused for a rounding artifact.  Phase 3 already separates a field
			**	centre from a base centre by `R_BASE_PROTECTED + field_radius_max`, which is the same
			**	geometry this measures.
			*/
			for (int b = 0; b < v.Out->Bases.Count(); ++b) {
				const HybridBaseAnchor & base = v.Out->Bases[b];
				if ((point - base.Center).Length() > base.ProtectedRadius) continue;
				detail.Format("field %u reserve reaches base %u's protected disc at %.1f,%.1f",site.StableId,
									base.SiteId,point.X,point.Y);
				return Fail(result,HYBRID_ANCHOR_RESOURCE_ILLEGAL,detail.Peek_Buffer());
			}
		}
	}

	//	The approaches: a Phase-3 edge attached to this field whose straight run to the reserve edge is clear.
	for (int e = 0; e < v.Graph->Edges.Count(); ++e) {

		const HybridStrategicEdge & edge = v.Graph->Edges[e];
		unsigned int other_id = 0;
		if (edge.SiteA == site.StableId)			other_id = edge.SiteB;
		else if (edge.SiteB == site.StableId)	other_id = edge.SiteA;
		else												continue;

		const HybridStrategicSite * other = v.Graph->Find_Site(other_id);
		if (other == nullptr) continue;

		Vector2 direction = other->Position - anchor.Center;
		const float length = direction.Length();
		if (length <= 0.0f) continue;
		direction *= (1.0f / length);

		bool clear = true;
		for (float along = 0.0f; (along <= anchor.ReserveRadius) && clear; along += step) {
			const Vector2 point = anchor.Center + (direction * along);
			if (Inside_Water(v,point))					clear = false;
			if (Inside_Immutable_Wall(v,point))		clear = false;
		}
		if (!clear) continue;

		anchor.ApproachCount++;
		if (anchor.ApproachEdgeId == 0) anchor.ApproachEdgeId = edge.StableId;
	}

	if (anchor.ApproachCount <= 0) {
		detail.Format("field %u has no clear route-corridor approach to its reserve edge",site.StableId);
		return Fail(result,HYBRID_ANCHOR_RESOURCE_NO_APPROACH,detail.Peek_Buffer());
	}

	v.Out->Resources.Add(anchor);
	return true;
}


/*
**	Section 0.10's provisional fairness, and nothing else: the home-field and base-to-front travel-time
**	differences over `PlanningDistance`, plus the rule that the strategic base-to-base planning distance
**	may not be zero.  The final base-to-base minimum/maximum gate belongs to Phase 19, where routed road
**	distance exists.
*/
static bool Check_Provisional_Fairness(AnchorValidator & v,HybridAnchorResult * result)
{
	const HybridStrategicGraph & graph = *v.Graph;
	HybridAnchorSet & out = *v.Out;
	const float speed = Max2(v.P->ReferenceVehicleSpeed,v.P->Constants.Q);
	StringClass detail;

	for (int team = 0; team < 2; ++team) {

		const HybridBaseAnchor * base = out.Base_Of_Team(team);
		if (base == nullptr) return Fail(result,HYBRID_ANCHOR_MISSING_ROLE,"a team has no validated base");

		const HybridSiteRole home_role = (team == 0) ? HYBRID_SITE_HOME_TIBERIUM_GDI : HYBRID_SITE_HOME_TIBERIUM_NOD;

		for (int s = 0; s < graph.Sites.Count(); ++s) {

			const HybridStrategicSite & site = graph.Sites[s];

			if (site.Role == home_role) {
				const float distance = Hybrid_Planning_Distance(graph,base->SiteId,site.StableId);
				if (distance >= 0.0f) {
					out.HomeFieldDistance[team]	= distance;
					out.HomeFieldTime[team]		= distance / speed;
				}
				continue;
			}
			if (site.Role != HYBRID_SITE_CONTESTED_TIBERIUM) continue;

			const float distance = Hybrid_Planning_Distance(graph,base->SiteId,site.StableId);
			if (distance < 0.0f) continue;
			if ((out.FrontDistance[team] < 0.0f) || (distance < out.FrontDistance[team])) {
				out.FrontDistance[team]	= distance;
				out.FrontTime[team]		= distance / speed;
			}
		}

		if (out.HomeFieldDistance[team] < 0.0f) {
			detail.Format("team %d base cannot reach its own HOME field on the strategic graph",team);
			return Fail(result,HYBRID_ANCHOR_MISSING_ROLE,detail.Peek_Buffer());
		}
		if (out.FrontDistance[team] < 0.0f) {
			detail.Format("team %d base cannot reach any CONTESTED field on the strategic graph",team);
			return Fail(result,HYBRID_ANCHOR_MISSING_ROLE,detail.Peek_Buffer());
		}
	}

	const HybridBaseAnchor * gdi = out.Base_Of_Team(0);
	const HybridBaseAnchor * nod = out.Base_Of_Team(1);
	if ((gdi == nullptr) || (nod == nullptr)) {
		return Fail(result,HYBRID_ANCHOR_MISSING_ROLE,"both bases are required");
	}

	out.BaseToBaseDistance = Hybrid_Planning_Distance(graph,gdi->SiteId,nod->SiteId);
	if (out.BaseToBaseDistance <= 0.0f) {
		detail.Format("the strategic base-to-base planning distance is %.3f",out.BaseToBaseDistance);
		return Fail(result,HYBRID_ANCHOR_BASE_TO_BASE_ZERO,detail.Peek_Buffer());
	}

	out.HomeTimeDifference	= (float)fabs((double)(out.HomeFieldTime[0] - out.HomeFieldTime[1]));
	out.FrontTimeDifference	= (float)fabs((double)(out.FrontTime[0] - out.FrontTime[1]));

	if (out.HomeTimeDifference > v.P->MaxInitialResourceTravelTimeDifferenceSeconds) {
		detail.Format("home-field times differ by %.2f s, fairness allows %.2f",out.HomeTimeDifference,
						  v.P->MaxInitialResourceTravelTimeDifferenceSeconds);
		return Fail(result,HYBRID_ANCHOR_FAIRNESS_HOME,detail.Peek_Buffer());
	}
	if (out.FrontTimeDifference > v.P->MaxBaseToFrontTravelTimeDifferenceSeconds) {
		detail.Format("base-to-front times differ by %.2f s, fairness allows %.2f",out.FrontTimeDifference,
						  v.P->MaxBaseToFrontTravelTimeDifferenceSeconds);
		return Fail(result,HYBRID_ANCHOR_FAIRNESS_FRONT,detail.Peek_Buffer());
	}
	return true;
}


unsigned int Hybrid_Anchor_Checksum(const HybridAnchorSet & set,unsigned int seed)
{
	HybridHashStream stream(seed);

	stream.Add_Int(set.Bases.Count());
	stream.Add_Int(set.Resources.Count());
	stream.Add_Bool(set.Frozen);

	for (int i = 0; i < set.Bases.Count(); ++i) {

		const HybridBaseAnchor & base = set.Bases[i];
		stream.Add_Uint(base.SiteId);
		stream.Add_Int(base.Team);
		stream.Add_Length(base.Center.X);
		stream.Add_Length(base.Center.Y);
		stream.Add_Length(base.ProtectedRadius);
		stream.Add_Length(base.CoreRadius);
		stream.Add_Length(base.ExpansionArea);
		stream.Add_Angle(base.MaxCoreSlopeRadians);
		stream.Add_Uint(base.ViableSectorMask);
		stream.Add_Int(base.ViableSectors.Count());
		for (int sector = 0; sector < base.ViableSectors.Count(); ++sector) {
			stream.Add_Int(base.ViableSectors[sector]);
		}
	}

	for (int i = 0; i < set.Resources.Count(); ++i) {

		const HybridResourceAnchor & anchor = set.Resources[i];
		stream.Add_Uint(anchor.SiteId);
		stream.Add_Uint((unsigned int)anchor.Role);
		stream.Add_Int(anchor.Team);
		stream.Add_Length(anchor.Center.X);
		stream.Add_Length(anchor.Center.Y);
		stream.Add_Length(anchor.ReserveRadius);
		stream.Add_Uint(anchor.ApproachEdgeId);
		stream.Add_Int(anchor.ApproachCount);
	}

	for (int team = 0; team < 2; ++team) {
		stream.Add_Length(set.HomeFieldDistance[team]);
		stream.Add_Length(set.FrontDistance[team]);
	}
	stream.Add_Length(set.BaseToBaseDistance);
	return stream.Get();
}


/***********************************************************************************************
 * Hybrid_Validate_Anchors -- runtime Phase 8                                                   *
 *=============================================================================================*/
bool Hybrid_Validate_Anchors(const HybridAnchorParams & params,const HybridMapIdentity & identity,
											const HybridStrategicGraph & graph,const HybridCoarseMatrices & matrices,
											const HybridLandformSet & landforms,const HybridHydrologySet & hydrology,
											HybridAnchorHeightSampleFunc sample,void * context,
											HybridAnchorSet * out,HybridAnchorResult * result)
{
	if (result != nullptr) {
		result->Failure	= HYBRID_ANCHOR_OK;
		result->Detail		= "";
	}
	if ((out == nullptr) || (sample == nullptr)) {
		return Fail(result,HYBRID_ANCHOR_INVALID_INPUT,"no output record or no height sampler");
	}

	out->Reset();

	if ((graph.Sites.Count() <= 0) || (matrices.Matrix.Get_Cell_Count() <= 0) ||
		 (params.Constants.A <= 0.0f) || (params.Constants.W <= 0.0f)) {
		return Fail(result,HYBRID_ANCHOR_INVALID_INPUT,"no graph, no matrix, or degenerate constants");
	}

	AnchorValidator v;
	v.P				= &params;
	v.Identity		= &identity;
	v.Graph			= &graph;
	v.Matrices		= &matrices;
	v.Landforms		= &landforms;
	v.Hydrology		= &hydrology;
	v.Sample			= sample;
	v.Context		= context;
	v.Out				= out;

	//	The two bases, in team order, so the frozen record's order never depends on the graph's.
	for (int team = 0; team < 2; ++team) {

		const unsigned int expected = (team == 0) ? graph.GdiBaseId : graph.NodBaseId;
		const HybridStrategicSite * site = graph.Find_Site(expected);
		if (site == nullptr) {
			out->Reset();
			return Fail(result,HYBRID_ANCHOR_MISSING_ROLE,(team == 0) ? "no GDI base site" : "no Nod base site");
		}
		if (!Validate_Base(v,*site,team,result)) {
			out->Reset();
			return false;
		}
	}

	//	Every mandatory resource role, in StableId order, which is the graph's own creation order.
	for (int s = 0; s < graph.Sites.Count(); ++s) {

		const HybridStrategicSite & site = graph.Sites[s];
		if ((site.Role != HYBRID_SITE_HOME_TIBERIUM_GDI) && (site.Role != HYBRID_SITE_HOME_TIBERIUM_NOD) &&
			 (site.Role != HYBRID_SITE_CONTESTED_TIBERIUM) && (site.Role != HYBRID_SITE_REMOTE_TIBERIUM)) continue;

		if (!Validate_Resource(v,site,result)) {
			out->Reset();
			return false;
		}
	}

	if ((out->Count_Of_Role(HYBRID_SITE_HOME_TIBERIUM_GDI) != 1) ||
		 (out->Count_Of_Role(HYBRID_SITE_HOME_TIBERIUM_NOD) != 1)) {
		out->Reset();
		return Fail(result,HYBRID_ANCHOR_MISSING_ROLE,"each team needs exactly one HOME field");
	}
	if (out->Count_Of_Role(HYBRID_SITE_CONTESTED_TIBERIUM) < 1) {
		out->Reset();
		return Fail(result,HYBRID_ANCHOR_MISSING_ROLE,"a map needs at least one CONTESTED field");
	}

	if (!Check_Provisional_Fairness(v,result)) {
		out->Reset();
		return false;
	}

	out->Frozen		= true;
	out->Checksum	= Hybrid_Anchor_Checksum(*out,identity.Checksum);

	WWDEBUG_SAY(("Hybrid Phase 8: bases frozen at %d/%d viable exit sectors, expansion %.0f/%.0f, "
					 "home times %.2f/%.2f s (diff %.2f), front times %.2f/%.2f s (diff %.2f), "
					 "base-to-base %.1f, %d resource anchors, checksum 0x%08X\r\n",
					 (out->Bases.Count() > 0) ? out->Bases[0].ViableSectors.Count() : 0,
					 (out->Bases.Count() > 1) ? out->Bases[1].ViableSectors.Count() : 0,
					 (out->Bases.Count() > 0) ? out->Bases[0].ExpansionArea : 0.0f,
					 (out->Bases.Count() > 1) ? out->Bases[1].ExpansionArea : 0.0f,
					 out->HomeFieldTime[0],out->HomeFieldTime[1],out->HomeTimeDifference,
					 out->FrontTime[0],out->FrontTime[1],out->FrontTimeDifference,
					 out->BaseToBaseDistance,out->Resources.Count(),out->Checksum));

	return true;
}
