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
 *                     $Archive:: /Commando/Code/Combat/transportationnetworkvalidator.cpp    $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "transportationnetworkvalidator.h"

#include "bridgesection.h"
#include "bridgesystem.h"
#include "playertype.h"
#include "roadspline.h"
#include "roadsystem.h"
#include "strategicplanner.h"
#include "worldmetrics.h"
#include "worldtraversability.h"
#include "wwdebug.h"
#include "wwmath.h"


int	TransportationNetworkValidator::UnboundAbutmentCount			= 0;
int	TransportationNetworkValidator::BridgeNoRoadCount				= 0;
int	TransportationNetworkValidator::BridgeNarrowerCount			= 0;
int	TransportationNetworkValidator::DuplicateRoadCount				= 0;
int	TransportationNetworkValidator::ExcessiveGradeCount			= 0;
int	TransportationNetworkValidator::InsufficientClearanceCount	= 0;
bool	TransportationNetworkValidator::RequiredBaseRoutesChecked	= false;
bool	TransportationNetworkValidator::RequiredBaseRoutesConnected	= false;
int	TransportationNetworkValidator::UnreachableCellCount			= 0;
bool	TransportationNetworkValidator::LastResult							= true;


TransportationValidationParams::TransportationValidationParams(void) :
	MaxBuildableSlopeRadians (0.6f),		// ~34 degrees; overwritten by every real caller
	ApproachSampleDistance (8.0f)
{
}


void TransportationNetworkValidator::Clear(void)
{
	UnboundAbutmentCount				= 0;
	BridgeNoRoadCount					= 0;
	BridgeNarrowerCount					= 0;
	DuplicateRoadCount					= 0;
	ExcessiveGradeCount					= 0;
	InsufficientClearanceCount		= 0;
	RequiredBaseRoutesChecked			= false;
	RequiredBaseRoutesConnected		= false;
	UnreachableCellCount				= 0;
	LastResult								= true;
}


//	Where a road's own tessellated centre line actually is at a given distance along it --
//	the same linear interpolation RoadSplineClass::Trim_Start/Trim_End already use to leave a
//	point exactly at a cut, read-only here.
static bool Position_At_Distance(const RoadSplineClass & road,float distance,Vector3 * position_out)
{
	int count = road.Get_Point_Count();
	if (count < 2) {
		return false;
	}
	if (distance <= 0.0f) {
		*position_out = road.Get_Point(0).Position;
		return true;
	}
	if (distance >= road.Get_Length()) {
		*position_out = road.Get_Point(count - 1).Position;
		return true;
	}
	for (int i = 1; i < count; i++) {
		if (road.Get_Point(i).Distance >= distance) {
			const RoadPointClass & a = road.Get_Point(i - 1);
			const RoadPointClass & b = road.Get_Point(i);
			float span = b.Distance - a.Distance;
			float t = (span > WWMATH_EPSILON) ? ((distance - a.Distance) / span) : 0.0f;
			*position_out = a.Position + ((b.Position - a.Position) * t);
			return true;
		}
	}
	*position_out = road.Get_Point(count - 1).Position;
	return true;
}


//	The road's own climb over ApproachSampleDistance, measured back from whichever end abuts a
//	bridge.  False when the road is too short to sample or the sample is degenerate (vertical,
//	zero horizontal run) rather than reporting a meaningless number.
static bool Compute_Approach_Grade(RoadSplineClass * road,bool at_start,float sample_distance,
												float * grade_radians_out)
{
	if ((road == nullptr) || !road->Is_Tessellated()) {
		return false;
	}
	float length = road->Get_Length();
	if (length < WWMATH_EPSILON) {
		return false;
	}

	float d = WWMath::Min(sample_distance,length);
	Vector3 near_point = at_start ? road->Get_Point(0).Position
											 : road->Get_Point(road->Get_Point_Count() - 1).Position;

	Vector3 far_point;
	bool found = at_start ? Position_At_Distance(*road,d,&far_point)
								 : Position_At_Distance(*road,length - d,&far_point);
	if (!found) {
		return false;
	}

	float rise = WWMath::Fabs(far_point.Z - near_point.Z);
	Vector3 flat_delta (far_point.X - near_point.X,far_point.Y - near_point.Y,0.0f);
	float run = flat_delta.Length ();
	if (run < WWMATH_EPSILON) {
		return false;		// degenerate sample -- not enough horizontal distance to judge a grade
	}

	*grade_radians_out = WWMath::Atan2 (rise,run);
	return true;
}


//	Does this road's own tessellated centre line run through the bridge's footprint -- inside
//	its span, laterally within its own deck half-width -- without being one of the two roads the
//	bridge itself is connected to.  That is exactly "a duplicated road running through bridge
//	geometry": the crossing exists, a bridge stands there, and the original road was never cut.
static bool Road_Passes_Through_Bridge(const RoadSplineClass & road,const BridgeClass & bridge)
{
	float bridge_length = bridge.Get_Length ();
	if (bridge_length < WWMATH_EPSILON) {
		return false;
	}
	Vector3 direction = bridge.Get_Direction ();
	Vector3 start = bridge.Get_Start ();
	float half_width = (bridge.Get_Width () > 0.0f ? bridge.Get_Width () : 8.0f) * 0.5f;

	for (int i = 0; i < road.Get_Point_Count (); i++) {

		const Vector3 & point = road.Get_Point (i).Position;
		Vector3 to_point = point - start;
		float along = Vector3::Dot_Product (to_point,direction);

		//	Inset from both ends -- the abutments themselves are legitimately close to any road
		//	that terminates there, so only genuinely *inside* the span counts.
		if ((along < half_width) || (along > (bridge_length - half_width))) {
			continue;
		}

		Vector3 projected = start + (direction * along);
		float lateral = (point - projected).Length ();
		if (lateral <= half_width) {
			return true;
		}
	}
	return false;
}


/***********************************************************************************************
 * Validate -- the gap report's item 6                                                           *
 *=============================================================================================*/
bool TransportationNetworkValidator::Validate(const TransportationValidationParams & params,
															  StringClass * report_out)
{
	Clear ();

	#define REPORT(fmt, ...) { StringClass _l; _l.Format(fmt "\r\n",##__VA_ARGS__); \
										WWDEBUG_SAY(("TransportationNetworkValidator: %s",_l.Peek_Buffer())); \
										if (report_out) *report_out += _l; }

	//	1. Every road end that says it hands over to a bridge actually found one within
	//	BridgeSystem::Connect_Roads' own tolerance.  A TargetID still -1 here means Connect_Roads
	//	ran and could not bind it -- "a road ending at an abutment" with nothing on the other end.
	for (int r = 0; r < RoadSystem::Get_Road_Count (); r++) {
		RoadSplineClass * road = RoadSystem::Peek_Road (r);
		if (road == nullptr) continue;

		if ((road->Get_Start_Connection ().Type != ROAD_ENDPOINT_OPEN) &&
			 (road->Get_Start_Connection ().TargetID < 0)) {
			UnboundAbutmentCount++;
		}
		if ((road->Get_End_Connection ().Type != ROAD_ENDPOINT_OPEN) &&
			 (road->Get_End_Connection ().TargetID < 0)) {
			UnboundAbutmentCount++;
		}
	}
	if (UnboundAbutmentCount > 0) {
		REPORT("%d road end(s) name a crossing connection with no bridge/tunnel bound to it",
				 UnboundAbutmentCount);
	}

	//	2-5. Per bridge: connected on both sides, wide enough for the road(s) it carries, its
	//	approach(es) legal, and terrain clearance satisfied where the definition states one.
	for (int b = 0; b < BridgeSystem::Get_Bridge_Count (); b++) {

		BridgeClass * bridge = BridgeSystem::Peek_Bridge (b);
		if (bridge == nullptr) continue;

		int start_road_id	= bridge->Get_Start_Road ();
		int end_road_id		= bridge->Get_End_Road ();

		if ((start_road_id < 0) && (end_road_id < 0)) {
			BridgeNoRoadCount++;
			REPORT("bridge '%s' has no road connected at either abutment",bridge->Get_Name ());
		}

		RoadSplineClass * start_road	= (start_road_id >= 0) ? RoadSystem::Peek_Road (start_road_id) : nullptr;
		RoadSplineClass * end_road		= (end_road_id >= 0)   ? RoadSystem::Peek_Road (end_road_id)   : nullptr;

		float bridge_width = bridge->Get_Width ();
		if (bridge_width > 0.0f) {
			if ((start_road != nullptr) && (start_road->Get_Total_Width () > bridge_width)) {
				BridgeNarrowerCount++;
				REPORT("bridge '%s' (width %.1f) is narrower than road %d (width %.1f) at its start abutment",
						 bridge->Get_Name (),bridge_width,start_road_id,start_road->Get_Total_Width ());
			}
			if ((end_road != nullptr) && (end_road->Get_Total_Width () > bridge_width)) {
				BridgeNarrowerCount++;
				REPORT("bridge '%s' (width %.1f) is narrower than road %d (width %.1f) at its end abutment",
						 bridge->Get_Name (),bridge_width,end_road_id,end_road->Get_Total_Width ());
			}

			//	The established vehicle-metrics service, not a guessed number: the widest
			//	wheeled/tracked definition actually loaded is the real floor a deck has to clear
			//	regardless of what the connected road itself happens to be authored at.  Absent
			//	when nothing of that class is loaded (a dedicated server with no vehicle content),
			//	which is the honest answer, not a violation.
			WorldMetricsStruct vehicle_metrics;
			if (WorldMetricsClass::Get_Class_Metrics (LOCOMOTION_WHEELED,&vehicle_metrics) &&
				 (vehicle_metrics.CorridorWidth > bridge_width)) {
				BridgeNarrowerCount++;
				REPORT("bridge '%s' (width %.1f) is narrower than the widest wheeled vehicle's own corridor (%.1f)",
						 bridge->Get_Name (),bridge_width,vehicle_metrics.CorridorWidth);
			}
		}

		if (start_road != nullptr) {
			bool at_start = (start_road->Get_Start_Connection ().TargetID == bridge->Get_ID ());
			float grade;
			if (Compute_Approach_Grade (start_road,at_start,params.ApproachSampleDistance,&grade) &&
				 (grade > params.MaxBuildableSlopeRadians)) {
				ExcessiveGradeCount++;
				REPORT("bridge '%s' start approach (road %d) climbs at %.1f degrees, over the %.1f degree limit",
						 bridge->Get_Name (),start_road_id,
						 grade * (180.0f / WWMATH_PI),params.MaxBuildableSlopeRadians * (180.0f / WWMATH_PI));
			}
		}
		if (end_road != nullptr) {
			bool at_start = (end_road->Get_Start_Connection ().TargetID == bridge->Get_ID ());
			float grade;
			if (Compute_Approach_Grade (end_road,at_start,params.ApproachSampleDistance,&grade) &&
				 (grade > params.MaxBuildableSlopeRadians)) {
				ExcessiveGradeCount++;
				REPORT("bridge '%s' end approach (road %d) climbs at %.1f degrees, over the %.1f degree limit",
						 bridge->Get_Name (),end_road_id,
						 grade * (180.0f / WWMATH_PI),params.MaxBuildableSlopeRadians * (180.0f / WWMATH_PI));
			}
		}

		BridgeClearanceReportStruct clearance;
		if (BridgeSystem::Check_Clearance (bridge->Get_ID (),&clearance) &&
			 clearance.Stated && clearance.Measured && !clearance.Satisfied) {
			InsufficientClearanceCount++;
			REPORT("bridge '%s' clearance is %.1f, %.1f short of the %.1f it states, near %.1f along the span",
					 bridge->Get_Name (),clearance.MinimumClearance,
					 clearance.RequiredClearance - clearance.MinimumClearance,
					 clearance.RequiredClearance,clearance.WorstDistanceAlong);
		}

		//	6. Every other road in the network -- not this bridge's own two connections -- kept
		//	out of its footprint.  A pass proven clean on every world this integration can
		//	currently generate (Trim_Road_Range always removes the middle span), kept as a
		//	regression net rather than an assumption.
		for (int r = 0; r < RoadSystem::Get_Road_Count (); r++) {
			if ((r == start_road_id) || (r == end_road_id)) continue;
			RoadSplineClass * other = RoadSystem::Peek_Road (r);
			if ((other == nullptr) || !other->Is_Tessellated ()) continue;
			if (Road_Passes_Through_Bridge (*other,*bridge)) {
				DuplicateRoadCount++;
				REPORT("road %d runs through bridge '%s' instead of being cut at it",r,bridge->Get_Name ());
			}
		}
	}

	//	7. Required connectivity: every StrategicPlanner BASE_CANDIDATE, for the vehicle
	//	locomotion class, ought to be able to reach every base of the opposite faction -- if the
	//	terrain has a wheeled-vehicle graph to build at all (a level with no terrain has nothing
	//	to check here, the honest WorldTraversabilityGraph::Build failure, not a violation).
	if (WorldTraversabilityGraph::Build (LOCOMOTION_WHEELED)) {

		DynamicVectorClass<Vector3> gdi_bases;
		DynamicVectorClass<Vector3> nod_bases;
		for (int n = 0; n < StrategicPlanner::Get_Node_Count (); n++) {
			StrategicNode node;
			if (!StrategicPlanner::Get_Node (n,&node)) continue;
			if (node.Type != STRATEGIC_NODE_BASE_CANDIDATE) continue;
			if (node.Faction == PLAYERTYPE_GDI) gdi_bases.Add (node.Position);
			else if (node.Faction == PLAYERTYPE_NOD) nod_bases.Add (node.Position);
		}

		if ((gdi_bases.Count () > 0) && (nod_bases.Count () > 0)) {
			RequiredBaseRoutesChecked = true;
			RequiredBaseRoutesConnected = true;
			for (int g = 0; g < gdi_bases.Count (); g++) {
				for (int n = 0; n < nod_bases.Count (); n++) {
					if (!WorldTraversabilityGraph::Is_Reachable (gdi_bases[g],nod_bases[n])) {
						RequiredBaseRoutesConnected = false;
						REPORT("no wheeled-vehicle route between GDI base candidate %d and Nod base candidate %d",
								 g,n);
					}
				}
			}
		}

		int start_node = -1;
		for (int i = 0; i < WorldTraversabilityGraph::Get_Node_Count (); i++) {
			const TraversabilityNodeStruct * node = WorldTraversabilityGraph::Peek_Node (i);
			if ((node != nullptr) && node->Passable) { start_node = i; break; }
		}
		if (start_node >= 0) {
			UnreachableCellCount = WorldTraversabilityGraph::Count_Unreachable (start_node);
			if (UnreachableCellCount > 0) {
				REPORT("%d passable cell(s) unreachable from the rest of the wheeled-vehicle network",
						 UnreachableCellCount);
			}
		}

		WorldTraversabilityGraph::Destroy ();
	}

	LastResult = (UnboundAbutmentCount == 0) && (BridgeNoRoadCount == 0) && (BridgeNarrowerCount == 0) &&
					 (DuplicateRoadCount == 0) && (ExcessiveGradeCount == 0) && (InsufficientClearanceCount == 0) &&
					 (!RequiredBaseRoutesChecked || RequiredBaseRoutesConnected) && (UnreachableCellCount == 0);

	WWDEBUG_SAY(("TransportationNetworkValidator: %s -- %d unbound abutment(s), %d unconnected bridge(s), "
					 "%d narrow bridge(s), %d unsplit crossing(s), %d excessive approach(es), "
					 "%d clearance violation(s), required-base routes %s, %d unreachable cell(s)\r\n",
					 LastResult ? "PASS" : "FAIL",
					 UnboundAbutmentCount,BridgeNoRoadCount,BridgeNarrowerCount,DuplicateRoadCount,
					 ExcessiveGradeCount,InsufficientClearanceCount,
					 !RequiredBaseRoutesChecked ? "not checked" : (RequiredBaseRoutesConnected ? "ok" : "BROKEN"),
					 UnreachableCellCount));

	#undef REPORT
	return LastResult;
}
