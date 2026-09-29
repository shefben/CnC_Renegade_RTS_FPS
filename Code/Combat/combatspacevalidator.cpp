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
 *                     $Archive:: /Commando/Code/Combat/combatspacevalidator.cpp              $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "combatspacevalidator.h"
#include "worldspatialindex.h"

#include "baseselectionplanner.h"
#include "lineseg.h"
#include "playertype.h"
#include "roadgraphgenerator.h"
#include "roadspline.h"
#include "roadsystem.h"
#include "settlementgenerator.h"
#include "strategicplanner.h"
#include "tiberiumfieldgenerator.h"
#include "worldmetrics.h"
#include "worldterrainsystem.h"
#include "wwdebug.h"
#include "wwmath.h"


CombatSpaceMetrics	CombatSpaceValidator::Metrics;
DynamicVectorClass<CombatSpaceFailureRecord>	CombatSpaceValidator::Failures;
bool						CombatSpaceValidator::LastResult = true;


//	Section 17A.4's own required documentation, made concrete here rather than left implicit:
static const float SIGHTLINE_EYE_HEIGHT			= 1.62f;	// WorldMetricsClass::SOLDIER_EYE_HEIGHT's own real value
static const int	SIGHTLINE_DIRECTION_COUNT		= 8;			// compass directions, evenly spaced
static const float SIGHTLINE_MAX_DISTANCE			= 250.0f;	// world units

//	Section 17A.3's own "which terrain/objects count as occluders": terrain, every collidable
//	object in the physics scene (placed structures included) and solid foliage, through
//	WorldSpatialIndex::Query_Sightline (D-001).  Each is asked through its own spatial structure;
//	nothing scans every object.


CombatSpaceFailureRecord::CombatSpaceFailureRecord(void) : Type(COMBAT_FAILURE_DISCONNECTED_BASE) { }

bool CombatSpaceFailureRecord::operator == (const CombatSpaceFailureRecord & that) const
{
	return (Type == that.Type) && (Description == that.Description);
}


CombatSpaceMetrics::CombatSpaceMetrics(void) :
	SightlineSampleCount(0), SightlineP50(0.0f), SightlineP90(0.0f), SightlineMax(0.0f),
	BaseToBaseTravelTimeSeconds(-1.0f), SingleBridgeDependency(false),
	OwnedFieldsWithoutRoute(0), SettlementsWithoutRoadConnection(0)
{ }


CombatSpaceValidationParams::CombatSpaceValidationParams(void) :
	Seed(1), MaxMandatoryExposedInfantryRunTimeSeconds(12.0f), MinVehicleLaneClearance(1.0f),
	ReferenceInfantrySpeed(WorldMetricsClass::Get_Reference_Infantry_Speed())
{ }


const char * Combat_Space_Failure_Name(CombatSpaceFailureType type)
{
	switch (type) {
		case COMBAT_FAILURE_DISCONNECTED_BASE:				return "disconnected_base";
		case COMBAT_FAILURE_NO_RESOURCE_PATH:				return "no_resource_path";
		case COMBAT_FAILURE_IMPOSSIBLE_CHOKE:				return "impossible_choke";
		case COMBAT_FAILURE_EXPOSED_INFANTRY_MARCH:		return "exposed_infantry_march";
		case COMBAT_FAILURE_UNUSABLE_VEHICLE_LANE:			return "unusable_vehicle_lane";
		default:														return "unknown";
	}
}

const char * Combat_Space_Repair_Operation_Name(CombatSpaceRepairOperationType op)
{
	switch (op) {
		case COMBAT_REPAIR_TERRAIN_FOLD_BERM:		return "terrain_fold_berm";
		case COMBAT_REPAIR_COVER_PLACEMENT:			return "cover_placement";
		case COMBAT_REPAIR_INFANTRY_SIDE_PATH:		return "infantry_side_path";
		case COMBAT_REPAIR_ROAD_SPUR:					return "road_spur";
		case COMBAT_REPAIR_RAMP:							return "ramp";
		case COMBAT_REPAIR_TUNNEL_CANDIDATE:			return "tunnel_candidate";
		case COMBAT_REPAIR_SECOND_CROSSING:			return "second_crossing";
		case COMBAT_REPAIR_MOVE_POI_RESOURCE:		return "move_poi_resource";
		default:												return "unknown";
	}
}


/***********************************************************************************************
 * local helpers                                                                                 *
 *=============================================================================================*/

//	Distance along one ray before terrain, an object or foliage occludes it --
//	SIGHTLINE_MAX_DISTANCE itself when nothing occludes within range.
static float Sightline_Distance(const Vector3 & eye,const Vector3 & direction)
{
	Vector3 end = eye + (direction * SIGHTLINE_MAX_DISTANCE);

	float fraction = 1.0f;
	if (WorldSpatialIndex::Query_Sightline(eye,end,&fraction)) {
		return SIGHTLINE_MAX_DISTANCE * WWMath::Clamp(fraction,0.0f,1.0f);
	}
	return SIGHTLINE_MAX_DISTANCE;
}

static void Sample_Sightlines_At(const Vector3 & position,DynamicVectorClass<float> * out)
{
	Vector3 eye = position;
	eye.Z += SIGHTLINE_EYE_HEIGHT;

	for (int d = 0; d < SIGHTLINE_DIRECTION_COUNT; d++) {
		float angle = ((float)d / (float)SIGHTLINE_DIRECTION_COUNT) * 2.0f * WWMATH_PI;
		Vector3 direction(WWMath::Cos(angle),WWMath::Sin(angle),0.0f);
		out->Add(Sightline_Distance(eye,direction));
	}
}

//	Insertion sort -- fine at this scale, the same tolerance every other bounded pass in this
//	chain already accepts.
static void Sort_Ascending(DynamicVectorClass<float> & values)
{
	for (int a = 1; a < values.Count(); a++) {
		float key = values[a];
		int b = a - 1;
		while ((b >= 0) && (values[b] > key)) { values[b + 1] = values[b]; b--; }
		values[b + 1] = key;
	}
}


/***********************************************************************************************
 * Clear                                                                                          *
 *=============================================================================================*/
void CombatSpaceValidator::Clear(void)
{
	Metrics = CombatSpaceMetrics();
	Failures.Delete_All();
	LastResult = true;
}


/***********************************************************************************************
 * Validate -- Phase 15 steps 1-6, 13                                                            *
 *=============================================================================================*/
bool CombatSpaceValidator::Validate(const CombatSpaceValidationParams & params,StringClass * report_out)
{
	#define REPORT(fmt, ...) { StringClass _l; _l.Format(fmt "\r\n",##__VA_ARGS__); \
										WWDEBUG_SAY(("CombatSpaceValidator: %s",_l.Peek_Buffer())); \
										if (report_out) *report_out += _l; }

	Clear();

	//
	//	Steps 2-3: sightline sampling (Section 17A.4) at every real anchor this world actually
	//	has -- both selected bases and every resolved settlement.  Terrain-only occlusion; see
	//	this file's own header comment for why.
	//
	DynamicVectorClass<float> sightlines;

	SelectedBaseSite gdi_site, nod_site;
	bool has_gdi = BaseSelectionPlanner::Get_Selected_Site(PLAYERTYPE_GDI,&gdi_site);
	bool has_nod = BaseSelectionPlanner::Get_Selected_Site(PLAYERTYPE_NOD,&nod_site);
	if (has_gdi) Sample_Sightlines_At(gdi_site.Position,&sightlines);
	if (has_nod) Sample_Sightlines_At(nod_site.Position,&sightlines);

	for (int i = 0; i < SettlementGenerator::Get_Settlement_Count(); i++) {
		ResolvedSettlement settlement;
		if (SettlementGenerator::Get_Settlement(i,&settlement)) {
			Sample_Sightlines_At(settlement.Position,&sightlines);
		}
	}

	if (sightlines.Count() > 0) {
		Sort_Ascending(sightlines);
		Metrics.SightlineSampleCount	= sightlines.Count();
		Metrics.SightlineP50			= sightlines[sightlines.Count() / 2];
		Metrics.SightlineP90			= sightlines[(sightlines.Count() * 9) / 10];
		Metrics.SightlineMax			= sightlines[sightlines.Count() - 1];
	}

	//
	//	Step 6: COMBAT_FAILURE_DISCONNECTED_BASE and COMBAT_FAILURE_EXPOSED_INFANTRY_MARCH,
	//	re-derived from the real road-graph tree rather than trusting BaseSelectionPlanner's own
	//	cached metrics blindly -- the same defense-in-depth every Check_* accessor in this chain
	//	already applies to its own generator's state.
	//
	if (has_gdi && has_nod) {
		int gdi_node = -1, nod_node = -1;
		StrategicPlanner::Find_Node_By_Id(gdi_site.Id,&gdi_node);
		StrategicPlanner::Find_Node_By_Id(nod_site.Id,&nod_node);

		float length = -1.0f;
		bool connected = (gdi_node >= 0) && (nod_node >= 0) &&
								RoadGraphGenerator::Find_Anchor_Path(gdi_node,nod_node,nullptr,&length);

		if (!connected) {
			CombatSpaceFailureRecord failure;
			failure.Type = COMBAT_FAILURE_DISCONNECTED_BASE;
			failure.Description = "no road-graph route between the selected GDI/Nod bases";
			Failures.Add(failure);
			REPORT("%s",failure.Description.Peek_Buffer());
		} else {
			Metrics.BaseToBaseTravelTimeSeconds = length / params.ReferenceInfantrySpeed;
			if (Metrics.BaseToBaseTravelTimeSeconds > params.MaxMandatoryExposedInfantryRunTimeSeconds) {
				CombatSpaceFailureRecord failure;
				failure.Type = COMBAT_FAILURE_EXPOSED_INFANTRY_MARCH;
				failure.Description.Format("base-to-base infantry march estimate %.1fs exceeds the %.1fs threshold",
														Metrics.BaseToBaseTravelTimeSeconds,
														params.MaxMandatoryExposedInfantryRunTimeSeconds);
				Failures.Add(failure);
				REPORT("%s",failure.Description.Peek_Buffer());
			}
		}
	}

	Metrics.SingleBridgeDependency = BaseSelectionPlanner::Get_Fairness_Metrics().SingleBridgeDependency;
	if (Metrics.SingleBridgeDependency) {
		CombatSpaceFailureRecord failure;
		failure.Type = COMBAT_FAILURE_IMPOSSIBLE_CHOKE;
		failure.Description = "the selected base pair's only route depends on a single bridge with no alternate";
		Failures.Add(failure);
		REPORT("%s",failure.Description.Peek_Buffer());
	}

	//
	//	Step 6: COMBAT_FAILURE_NO_RESOURCE_PATH, over every field TiberiumFieldGenerator resolved
	//	an owner for.
	//
	for (int i = 0; i < TiberiumFieldGenerator::Get_Field_Count(); i++) {
		ResolvedTiberiumField field;
		if (!TiberiumFieldGenerator::Get_Field(i,&field)) continue;
		if (field.OwningFaction == PLAYERTYPE_NEUTRAL) continue;
		if (!field.HasHarvesterRoute) {
			Metrics.OwnedFieldsWithoutRoute++;
			CombatSpaceFailureRecord failure;
			failure.Type = COMBAT_FAILURE_NO_RESOURCE_PATH;
			failure.Description.Format("owned Tiberium field '%s' has no resolved Harvester route",
													field.Id.Peek_Buffer());
			Failures.Add(failure);
			REPORT("%s",failure.Description.Peek_Buffer());
		}
	}

	for (int i = 0; i < SettlementGenerator::Get_Settlement_Count(); i++) {
		ResolvedSettlement settlement;
		if (SettlementGenerator::Get_Settlement(i,&settlement) && !settlement.RoadConnected) {
			Metrics.SettlementsWithoutRoadConnection++;
		}
	}

	//
	//	Step 6: COMBAT_FAILURE_UNUSABLE_VEHICLE_LANE, over every real road this world built.
	//
	for (int e = 0; e < RoadGraphGenerator::Get_Edge_Count(); e++) {
		RoadGraphEdge edge;
		if (!RoadGraphGenerator::Get_Edge(e,&edge)) continue;
		RoadSplineClass * road = RoadSystem::Peek_Road(edge.RoadId);
		if ((road == nullptr) || !road->Is_Tessellated()) continue;
		if (road->Get_Width() < params.MinVehicleLaneClearance) {
			CombatSpaceFailureRecord failure;
			failure.Type = COMBAT_FAILURE_UNUSABLE_VEHICLE_LANE;
			failure.Description.Format("road edge '%s' carriageway (%.2f) is narrower than the minimum vehicle lane clearance (%.2f)",
													edge.Id.Peek_Buffer(),road->Get_Width(),params.MinVehicleLaneClearance);
			Failures.Add(failure);
			REPORT("%s",failure.Description.Peek_Buffer());
		}
	}

	LastResult = (Failures.Count() == 0);

	REPORT("sightlines: %d sample(s), P50=%.1f P90=%.1f max=%.1f -- %s",
			 Metrics.SightlineSampleCount,Metrics.SightlineP50,Metrics.SightlineP90,Metrics.SightlineMax,
			 LastResult ? "PASS" : "FAIL");

	#undef REPORT
	return LastResult;
}


bool CombatSpaceValidator::Get_Failure(int index,CombatSpaceFailureRecord * out)
{
	if ((out == nullptr) || (index < 0) || (index >= Failures.Count())) return false;
	*out = Failures[index];
	return true;
}
