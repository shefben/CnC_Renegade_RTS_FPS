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
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/hybridroadrealizer.cpp                $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Hybrid world generator rewrite, item I13 -- runtime Phase 12 on the canonical owners.          *
 *                                                                                               *
 *---------------------------------------------------------------------------------------------*/

#include "hybridroadrealizer.h"

#include "baseselectionplanner.h"
#include "bridgeplanner.h"
#include "bridgesection.h"
#include "bridgesystem.h"
#include "heightfield.h"
#include "playertype.h"
#include "roadgraphgenerator.h"
#include "roadspline.h"
#include "roadsystem.h"
#include "strategicplanner.h"
#include "worldterrainsystem.h"
#include "wwdebug.h"
#include "wwmath.h"

#include <math.h>
#include <string.h>


int													HybridRoadRealizer::RealizeCount		= 0;
int													HybridRoadRealizer::TerrainChanged	= 0;
int													HybridRoadRealizer::BaselineRoads	= 0;
int													HybridRoadRealizer::BaselineBridges	= 0;
DynamicVectorClass<HybridRealizedPiece>	HybridRoadRealizer::Pieces;
DynamicVectorClass<int>							HybridRoadRealizer::BridgeIds;


//	The material every road is drawn with today: the legacy road graph's own (decoration owns the choice, I17).
static const char *	ROAD_MATERIAL	= "l11_gravel.tga";


static bool Realizer_Fail(StringClass * error_out,const char * detail)
{
	if (error_out != nullptr) *error_out = detail;
	return false;
}


//	Section 7's physical tier on the canonical road owner's class.
static int Road_Class_Of_Tier(HybridPhysicalRoadTier tier)
{
	switch (tier) {
		case HYBRID_ROAD_TIER_HIGHWAY:	return ROAD_CLASS_HIGHWAY;
		case HYBRID_ROAD_TIER_DIRT:		return ROAD_CLASS_TRACK;
		default:									break;
	}
	return ROAD_CLASS_STREET;
}


//	The legacy road graph's class for a hybrid road, for the adapters: what the road is for, not its width.
static RoadGraphClassType Graph_Class_Of(HybridRoadClass road_class)
{
	switch (road_class) {
		case HYBRID_ROAD_HIGHWAY_4:		return ROAD_GRAPH_HIGHWAY;
		case HYBRID_ROAD_PRIMARY_2:		return ROAD_GRAPH_PRIMARY;
		case HYBRID_ROAD_HIGHWAY_RAMP:	return ROAD_GRAPH_PRIMARY;
		case HYBRID_ROAD_SECONDARY_2:		return ROAD_GRAPH_SECONDARY;
		case HYBRID_ROAD_SERVICE_DIRT_2:	return ROAD_GRAPH_SERVICE;
		default:									break;
	}
	return ROAD_GRAPH_DIRT_TRACK;
}


//	The station on a realized road nearest `point`.
static float Road_Station_Of(const HybridRealizedRoad & road,const Vector2 & point)
{
	float best = 3.4e38f;
	float station = 0.0f;
	for (int i = 1; i < road.Samples.Count(); ++i) {
		const Vector2 a = road.Samples[i - 1].Position;
		const Vector2 d = road.Samples[i].Position - a;
		const float length2 = (d.X * d.X) + (d.Y * d.Y);
		float t = (length2 > 0.0f) ? ((((point.X - a.X) * d.X) + ((point.Y - a.Y) * d.Y)) / length2) : 0.0f;
		t = (t < 0.0f) ? 0.0f : ((t > 1.0f) ? 1.0f : t);
		const float distance = (point - (a + (d * t))).Length();
		if (distance >= best) continue;
		best		= distance;
		station	= road.Samples[i - 1].Station + ((road.Samples[i].Station - road.Samples[i - 1].Station) * t);
	}
	return station;
}


//	The maximal runs of a planned road that are on the ground: the stretches between its decks, ends included.
static void Road_Runs(const HybridRealizedRoad & road,DynamicVectorClass<int> * first,DynamicVectorClass<int> * last)
{
	const int count = road.Samples.Count();
	int i = 0;
	while (i < count) {
		while ((i < count) && road.Samples[i].Structure) i++;
		if (i >= count) break;
		int j = i;
		while (((j + 1) < count) && !road.Samples[j + 1].Structure) j++;
		if (j > i) {
			first->Add(i);
			last->Add(j);
		}
		i = j + 1;
	}
}


//	The plan assembly whose deck starts or ends at `point`, or -1.
static int Deck_At(const HybridRealizationPlan & plan,const Vector2 & point,float tolerance,bool * at_start)
{
	int best = -1;
	float nearest = tolerance;
	for (int b = 0; b < plan.Bridges.Count(); ++b) {
		const float to_start = (plan.Bridges[b].Start - point).Length();
		const float to_end = (plan.Bridges[b].End - point).Length();
		if (to_start <= nearest) {
			nearest		= to_start;
			best			= b;
			*at_start	= true;
		}
		if (to_end <= nearest) {
			nearest		= to_end;
			best			= b;
			*at_start	= false;
		}
	}
	return best;
}


static int Plan_Road_Index(const HybridRealizationPlan & plan,unsigned int edge_id)
{
	const HybridRealizedRoad * road = plan.Find_Road(edge_id);
	if (road == nullptr) return -1;
	for (int r = 0; r < plan.Roads.Count(); ++r) {
		if (&plan.Roads[r] == road) return r;
	}
	return -1;
}


/***********************************************************************************************
**	Realization.
***********************************************************************************************/

//	The objects on the cleanup list, removed in reverse order: bridges, then roads.  Others' objects are kept.
static void Release_Objects(DynamicVectorClass<HybridRealizedPiece> & pieces,DynamicVectorClass<int> & bridge_ids)
{
	if (bridge_ids.Count() > 0) {
		BridgeSystem::Destroy_Geometry();
		DynamicVectorClass<BridgeClass> keep;
		for (int i = 0; i < BridgeSystem::Get_Bridge_Count(); i++) {
			BridgeClass * bridge = BridgeSystem::Peek_Bridge(i);
			if (bridge == nullptr) continue;
			bool owned = false;
			for (int j = 0; (j < bridge_ids.Count()) && !owned; j++) owned = (bridge_ids[j] == i);
			if (!owned) keep.Add(*bridge);
		}
		BridgeSystem::Clear_Bridges();
		for (int i = 0; i < keep.Count(); i++) BridgeSystem::Add_Bridge(keep[i]);
		if (keep.Count() > 0) {
			BridgeSystem::Build_Layout();
			BridgeSystem::Build_Geometry();
		}
	}
	if (pieces.Count() > 0) {
		RoadSystem::Destroy_Geometry();
		DynamicVectorClass<RoadSplineClass> keep;
		for (int i = 0; i < RoadSystem::Get_Road_Count(); i++) {
			RoadSplineClass * road = RoadSystem::Peek_Road(i);
			if (road == nullptr) continue;
			bool owned = false;
			for (int j = 0; (j < pieces.Count()) && !owned; j++) owned = (pieces[j].RoadId == i);
			if (!owned) keep.Add(*road);
		}
		RoadSystem::Clear_Roads();
		for (int i = 0; i < keep.Count(); i++) RoadSystem::Add_Road(keep[i]);
		if (keep.Count() > 0) RoadSystem::Build_Network();
	}
	pieces.Delete_All();
	bridge_ids.Delete_All();
}


void HybridRoadRealizer::Clear(void)
{
	Release_Objects(Pieces,BridgeIds);
	RealizeCount	= 0;
	TerrainChanged	= 0;
	BaselineRoads	= 0;
	BaselineBridges= 0;
}


bool HybridRoadRealizer::Realize(const HybridRoadRealizerInputs & in,StringClass * error_out)
{
	if ((in.Plan == nullptr) || (in.Topology == nullptr) || (in.Routes == nullptr) || (in.Interactions == nullptr) ||
		 (in.Anchors == nullptr) || !(in.Constants.W > 0.0f)) {
		return Realizer_Fail(error_out,"Phase 12 needs its plan and the frozen records");
	}
	if ((RealizeCount > 0) || (Pieces.Count() > 0) || (BridgeIds.Count() > 0)) {
		return Realizer_Fail(error_out,"Phase 12 has already realized this attempt");
	}
	RealizeCount++;
	BaselineRoads		= RoadSystem::Get_Road_Count();
	BaselineBridges	= BridgeSystem::Get_Bridge_Count();
	const HybridRealizationPlan & plan = *in.Plan;
	const HybridConstants & k = in.Constants;
	StringClass detail;

	/*
	**	Steps 1-7: the accepted cut/fill committed through WorldTerrainSystem.  The field stores heights relative to
	**	its origin; the deformation works in the world's own heights.
	*/
	HeightfieldClass * field = WorldTerrainSystem::Peek_Heightfield();
	if ((field == nullptr) || !field->Is_Valid()) return Realizer_Fail(error_out,"Phase 12 has no terrain to deform");
	{
		const int count_x = field->Get_Vertex_Count_X();
		const int count_y = field->Get_Vertex_Count_Y();
		const Vector3 & origin = field->Get_Origin();
		DynamicVectorClass<float> heights;
		heights.Resize(count_x * count_y);
		for (int iy = 0; iy < count_y; ++iy) {
			for (int ix = 0; ix < count_x; ++ix) heights.Add(field->Get_Height(ix,iy) + origin.Z);
		}
		HybridHeightGrid grid;
		grid.OriginX	= origin.X;
		grid.OriginY	= origin.Y;
		grid.Spacing	= field->Get_Cell_Size();
		grid.CountX		= count_x;
		grid.CountY		= count_y;
		HybridDeformationStats stats;
		if (!Hybrid_Deform_Terrain(plan,k,in.Hydrology,grid,&heights[0],&stats)) {
			return Realizer_Fail(error_out,"Phase 12's terrain deformation refused the plan");
		}
		for (int i = 0; i < heights.Count(); ++i) heights[i] -= origin.Z;
		if (!WorldTerrainSystem::Set_Heights(&heights[0],heights.Count())) {
			return Realizer_Fail(error_out,"WorldTerrainSystem refused Phase 12's road cut/fill");
		}
		TerrainChanged = stats.Changed;
	}

	//	Step 8, bridges first so a road end can name the deck it hands over to: one bridge per BridgeAssembly.
	for (int b = 0; b < plan.Bridges.Count(); ++b) {
		const HybridBridgeAssembly & assembly = plan.Bridges[b];
		if ((assembly.Pieces.Count() != 1) || (assembly.Pieces[0].Kind != HYBRID_BRIDGE_PIECE_WHOLE)) {
			Release_Objects(Pieces,BridgeIds);
			return Realizer_Fail(error_out,"this generator revision realizes a BridgeAssembly of one WHOLE piece only");
		}
		if ((assembly.BridgeKind < 0) || (assembly.BridgeKind >= in.BridgeDefinitions.Count()) ||
			 (BridgeSystem::Find_Definition(in.BridgeDefinitions[assembly.BridgeKind].Peek_Buffer()) == nullptr)) {
			Release_Objects(Pieces,BridgeIds);
			detail.Format("bridge assembly %u names bridge kind %d, which BridgeSystem does not define",assembly.StableId,
							  assembly.BridgeKind);
			return Realizer_Fail(error_out,detail.Peek_Buffer());
		}
		//	The single WHOLE piece is the existing one-object bridge; its deck follows the two compiled ends exactly.
		const HybridBridgePiece & whole = assembly.Pieces[0];
		BridgeClass bridge;
		StringClass name;
		name.Format("hybrid_%s_%u",(assembly.Method == HYBRID_METHOD_OVERPASS) ? "overpass" : "bridge",assembly.StableId);
		bridge.Set_Name(name.Peek_Buffer());
		bridge.Set_Definition(in.BridgeDefinitions[assembly.BridgeKind].Peek_Buffer());
		bridge.Set_Endpoints(Vector3(whole.Start.X,whole.Start.Y,whole.StartZ),Vector3(whole.End.X,whole.End.Y,whole.EndZ));
		bridge.Set_Width(whole.Width);
		bridge.Set_Deck_Height(0.0f);
		BridgeIds.Add(BridgeSystem::Add_Bridge(bridge));
	}

	//	Step 8, roads: one per stretch of a planned road between its decks, in routing order.
	const float deck_tolerance = 0.25f * k.W;
	for (int r = 0; r < plan.Roads.Count(); ++r) {

		const HybridRealizedRoad & road = plan.Roads[r];
		const HybridTopologyEdge * first_edge = in.Topology->Find_Edge(road.EdgeId);
		const bool ramp = (first_edge != nullptr) && (first_edge->Kind == HYBRID_EDGE_HIGHWAY_RAMP);
		DynamicVectorClass<int> firsts;
		DynamicVectorClass<int> lasts;
		Road_Runs(road,&firsts,&lasts);

		for (int p = 0; p < firsts.Count(); ++p) {

			const int first = firsts[p];
			const int last = lasts[p];
			RoadSplineClass spline;
			StringClass name;
			name.Format("hybrid_road_%u_%d",road.EdgeId,p);
			spline.Set_Name(name.Peek_Buffer());
			spline.Set_Width(road.CarriagewayWidth);
			spline.Set_Shoulder_Width(road.ShoulderWidth);
			spline.Set_Road_Class(Road_Class_Of_Tier(road.Tier));
			spline.Set_Material(ROAD_MATERIAL);
			spline.Set_Grade_Policy(ROAD_GRADE_NONE);			// the ground already carries the solved profile

			DynamicVectorClass<Vector3> points;
			DynamicVectorClass<RoadEdgeProfileSample> profile;
			bool shaped = false;
			float distance = 0.0f;
			for (int i = first; i <= last; ++i) {
				const HybridRealizedSample & s = road.Samples[i];
				const Vector3 point(s.Position.X,s.Position.Y,s.Z);
				if (points.Count() > 0) {
					const float step = (point - points[points.Count() - 1]).Length();
					if (step <= HYBRID_QUANTUM_POSITION) continue;
					distance += step;
				}
				points.Add(point);
				RoadEdgeProfileSample edge;
				edge.Distance				= distance;
				edge.CarriagewayLeft		= s.CarriageLeft - (0.5f * road.CarriagewayWidth);
				edge.CarriagewayRight	= s.CarriageRight - (0.5f * road.CarriagewayWidth);
				edge.ShoulderLeft			= (s.Left - s.CarriageLeft) - road.ShoulderWidth;
				edge.ShoulderRight		= (s.Right - s.CarriageRight) - road.ShoulderWidth;
				shaped = shaped || (WWMath::Fabs(edge.CarriagewayLeft) > HYBRID_QUANTUM_POSITION) ||
							(WWMath::Fabs(edge.CarriagewayRight) > HYBRID_QUANTUM_POSITION) ||
							(WWMath::Fabs(edge.ShoulderLeft) > HYBRID_QUANTUM_POSITION) ||
							(WWMath::Fabs(edge.ShoulderRight) > HYBRID_QUANTUM_POSITION);
				profile.Add(edge);
			}
			if ((points.Count() < 2) || !spline.Set_Control_Points(&points[0],points.Count()) ||
				 (shaped && !spline.Set_Edge_Profile(&profile[0],profile.Count()))) {
				Release_Objects(Pieces,BridgeIds);
				detail.Format("road %u stretch %d cannot be handed to RoadSystem",road.EdgeId,p);
				return Realizer_Fail(error_out,detail.Peek_Buffer());
			}

			//	Each end: a deck it hands over to, a ramp's gore attached to its highway's widened lane, or open.
			for (int end = 0; end < 2; ++end) {
				RoadConnectionClass & connection = (end == 0) ? spline.Get_Start_Connection() : spline.Get_End_Connection();
				const HybridRealizedSample & tip = road.Samples[(end == 0) ? first : last];
				bool at_start = true;
				const int deck = Deck_At(plan,tip.Position,deck_tolerance,&at_start);
				if (deck >= 0) {
					connection.Type		= ROAD_ENDPOINT_BRIDGE;
					connection.TargetID	= BridgeIds[deck];
					connection.Position	= Vector3(tip.Position.X,tip.Position.Y,tip.Z);
					continue;
				}
				if (ramp && (end == 1) && (last == (road.Samples.Count() - 1))) {
					const HybridInterchange * ic = in.Topology->Find_Interchange(first_edge->InterchangeId);
					const int highway = (ic != nullptr) ? Plan_Road_Index(plan,ic->HighwayEdgeIn) : -1;
					int target = -1;
					if (highway >= 0) {
						const float station = Road_Station_Of(plan.Roads[highway],tip.Position);
						for (int q = 0; (q < Pieces.Count()) && (target < 0); ++q) {
							const HybridRealizedPiece & piece = Pieces[q];
							if ((piece.RoadIndex == highway) && (station >= piece.FromStation) && (station <= piece.ToStation)) {
								target = piece.RoadId;
							}
						}
					}
					if (target < 0) {
						Release_Objects(Pieces,BridgeIds);
						detail.Format("ramp %u's gore has no realized highway road to attach to",road.EdgeId);
						return Realizer_Fail(error_out,detail.Peek_Buffer());
					}
					connection.Type		= ROAD_ENDPOINT_ATTACHED;
					connection.TargetID	= target;
					connection.Position	= Vector3(tip.Position.X,tip.Position.Y,tip.Z);
				}
			}

			HybridRealizedPiece piece;
			piece.RoadIndex	= r;
			piece.FirstSample	= first;
			piece.LastSample	= last;
			piece.FromStation	= road.Samples[first].Station;
			piece.ToStation	= road.Samples[last].Station;
			piece.RoadId		= RoadSystem::Add_Road(spline);
			Pieces.Add(piece);
		}
	}

	//	Each deck bound to the roads that hand over to it.
	for (int q = 0; q < Pieces.Count(); ++q) {
		RoadSplineClass * road = RoadSystem::Peek_Road(Pieces[q].RoadId);
		if (road == nullptr) continue;
		road->Set_ID(Pieces[q].RoadId);
		for (int end = 0; end < 2; ++end) {
			const RoadConnectionClass & connection = (end == 0) ? road->Get_Start_Connection() : road->Get_End_Connection();
			if (connection.Type != ROAD_ENDPOINT_BRIDGE) continue;
			bool at_start = true;
			const int deck = Deck_At(plan,Vector2(connection.Position.X,connection.Position.Y),deck_tolerance,&at_start);
			BridgeClass * bridge = (deck >= 0) ? BridgeSystem::Peek_Bridge(BridgeIds[deck]) : nullptr;
			if (bridge == nullptr) continue;
			if (at_start) {
				bridge->Set_Start_Road(Pieces[q].RoadId);
			} else {
				bridge->Set_End_Road(Pieces[q].RoadId);
			}
		}
	}

	if ((Pieces.Count() > 0) && !RoadSystem::Build_Network()) {
		Release_Objects(Pieces,BridgeIds);
		return Realizer_Fail(error_out,"RoadSystem could not build Phase 12's network");
	}
	if (BridgeIds.Count() > 0) {
		BridgeSystem::Build_Layout();
		BridgeSystem::Build_Geometry();
	}
	return true;
}


/***********************************************************************************************
**	Verification: the runtime objects against the frozen plan.
***********************************************************************************************/

bool HybridRoadRealizer::Verify(const HybridRoadRealizerInputs & in,StringClass * error_out)
{
	if ((in.Plan == nullptr) || (in.Topology == nullptr) || (in.Routes == nullptr)) {
		return Realizer_Fail(error_out,"Phase 12's verification needs its plan");
	}
	const HybridRealizationPlan & plan = *in.Plan;
	const HybridConstants & k = in.Constants;
	StringClass detail;

	//	Exactly once, and nothing else added.
	int expected = 0;
	for (int r = 0; r < plan.Roads.Count(); ++r) {
		DynamicVectorClass<int> firsts;
		DynamicVectorClass<int> lasts;
		Road_Runs(plan.Roads[r],&firsts,&lasts);
		expected += firsts.Count();
	}
	if ((RealizeCount != 1) || (Pieces.Count() != expected) || (BridgeIds.Count() != plan.Bridges.Count()) ||
		 (RoadSystem::Get_Road_Count() != (BaselineRoads + Pieces.Count())) ||
		 (BridgeSystem::Get_Bridge_Count() != (BaselineBridges + BridgeIds.Count()))) {
		detail.Format("Phase 12 realized %d times: %d of %d road stretches, %d of %d bridges; RoadSystem holds %d roads, "
						  "BridgeSystem %d bridges",RealizeCount,Pieces.Count(),expected,BridgeIds.Count(),plan.Bridges.Count(),
						  RoadSystem::Get_Road_Count(),BridgeSystem::Get_Bridge_Count());
		return Realizer_Fail(error_out,detail.Peek_Buffer());
	}

	//	Every road point on its planned centerline at its solved height.
	const HeightfieldClass * field = WorldTerrainSystem::Peek_Heightfield();
	const float cell = (field != nullptr) ? field->Get_Cell_Size() : k.W;
	for (int q = 0; q < Pieces.Count(); ++q) {
		const HybridRealizedPiece & piece = Pieces[q];
		const HybridRealizedRoad & planned = plan.Roads[piece.RoadIndex];
		const RoadSplineClass * road = RoadSystem::Peek_Road(piece.RoadId);
		if ((road == nullptr) || !road->Is_Tessellated() || (road->Get_Width() != planned.CarriagewayWidth) ||
			 (road->Get_Shoulder_Width() != planned.ShoulderWidth)) {
			detail.Format("road stretch %d of edge %u is missing, untessellated or not at its planned width",q,planned.EdgeId);
			return Realizer_Fail(error_out,detail.Peek_Buffer());
		}
		const float max_grade = Hybrid_Road_Class_Spec(planned.RoadClass).MaxGradeDegrees * (WWMATH_PI / 180.0f);
		const float tolerance_z = (WWMath::Tan(max_grade) * cell) + (2.0f * k.Q);
		const float tolerance_xy = 0.05f * k.W;
		for (int i = 0; i < road->Get_Point_Count(); ++i) {
			const Vector3 & at = road->Get_Point(i).Position;
			float best = 3.4e38f;
			float z = 0.0f;
			for (int s = piece.FirstSample + 1; s <= piece.LastSample; ++s) {
				const HybridRealizedSample & a = planned.Samples[s - 1];
				const HybridRealizedSample & b = planned.Samples[s];
				const Vector2 d = b.Position - a.Position;
				const float length2 = (d.X * d.X) + (d.Y * d.Y);
				float t = (length2 > 0.0f) ? ((((at.X - a.Position.X) * d.X) + ((at.Y - a.Position.Y) * d.Y)) / length2) : 0.0f;
				t = (t < 0.0f) ? 0.0f : ((t > 1.0f) ? 1.0f : t);
				const float off = (Vector2(at.X,at.Y) - (a.Position + (d * t))).Length();
				if (off >= best) continue;
				best	= off;
				z		= a.Z + ((b.Z - a.Z) * t);
			}
			if ((best > tolerance_xy) || (WWMath::Fabs(at.Z - z) > tolerance_z)) {
				detail.Format("road stretch %d of edge %u leaves its frozen record at (%.2f,%.2f): %.3f off the centerline, "
								  "%.3f off the solved height %.3f",q,planned.EdgeId,at.X,at.Y,best,at.Z - z,z);
				return Realizer_Fail(error_out,detail.Peek_Buffer());
			}
		}
	}

	//	Every bridge at its assembly, bound at both decked ends.
	for (int b = 0; b < plan.Bridges.Count(); ++b) {
		const HybridBridgeAssembly & assembly = plan.Bridges[b];
		const BridgeClass * bridge = BridgeSystem::Peek_Bridge(BridgeIds[b]);
		const HybridBridgePiece & whole = assembly.Pieces[0];
		const bool ends = (bridge != nullptr) &&
								((Vector3(whole.Start.X,whole.Start.Y,whole.StartZ) - bridge->Get_Start()).Length() <= k.Q) &&
								((Vector3(whole.End.X,whole.End.Y,whole.EndZ) - bridge->Get_End()).Length() <= k.Q);
		if (!ends || (bridge->Get_Width() != whole.Width) || (bridge->Get_Start_Road() < 0) || (bridge->Get_End_Road() < 0) ||
			 (bridge->Get_Start_Road() == bridge->Get_End_Road()) ||
			 ((assembly.BridgeKind >= 0) && (assembly.BridgeKind < in.BridgeDefinitions.Count()) &&
			  (in.BridgeDefinitions[assembly.BridgeKind] != bridge->Get_Definition()))) {
			detail.Format("bridge assembly %u is not realized at its compiled ends, width and kind, bound at both ends",
							  assembly.StableId);
			return Realizer_Fail(error_out,detail.Peek_Buffer());
		}
	}

	//	No RoadSystem-discovered junction inside an active interchange's parallel interval or gore reserve.
	HybridInterchangeConstants c;
	Hybrid_Interchange_Constants(k.W,&c);
	for (int i = 0; i < in.Topology->Interchanges.Count(); ++i) {
		const HybridInterchange & ic = in.Topology->Interchanges[i];
		if ((ic.Status != HYBRID_INTERCHANGE_ACTIVE) || in.Routes->Interchange_Withdrawn(ic.StableId)) continue;
		const HybridTopologyNode * anchor = in.Topology->Find_Node(ic.AnchorNode);
		if (anchor == nullptr) continue;
		for (int e = 0; e < ic.RampEdges.Count(); ++e) {
			const HybridTopologyEdge * ramp = in.Topology->Find_Edge(ic.RampEdges[e]);
			if (ramp == nullptr) continue;
			float x0 = 0.0f;
			float x1 = 0.0f;
			Hybrid_Gore_Reserve_Extent(c,ic.GoreOffset,ramp->RampRole,&x0,&x1);
			const Vector2 tau = (ramp->RampSide == HYBRID_INTERCHANGE_SIDE_RIGHT) ? ic.Axis : Vector2(-ic.Axis.X,-ic.Axis.Y);
			const Vector2 normal = (ramp->RampSide == HYBRID_INTERCHANGE_SIDE_RIGHT) ? Vector2(ic.Axis.Y,-ic.Axis.X)
																											 : Vector2(-ic.Axis.Y,ic.Axis.X);
			for (int j = 0; j < RoadSystem::Get_Junction_Count(); ++j) {
				const RoadJunctionClass * junction = RoadSystem::Peek_Junction(j);
				if (junction == nullptr) continue;
				const Vector2 offset(junction->Position.X - anchor->Position.X,junction->Position.Y - anchor->Position.Y);
				const float x = (offset.X * tau.X) + (offset.Y * tau.Y);
				const float y = (offset.X * normal.X) + (offset.Y * normal.Y);
				if ((x >= x0) && (x <= x1) && (y >= -c.EHwy) && (y <= c.EAux)) {
					detail.Format("RoadSystem discovered a junction at (%.1f,%.1f) inside ramp %u's gore reserve",
									  junction->Position.X,junction->Position.Y,ramp->StableId);
					return Realizer_Fail(error_out,detail.Peek_Buffer());
				}
			}
		}
	}
	return true;
}


/***********************************************************************************************
**	The migration adapters.
***********************************************************************************************/

static float Ground_Z(const Vector2 & at)
{
	float z = 0.0f;
	if (!WorldTerrainSystem::Sample_Height(at.X,at.Y,&z)) z = 0.0f;
	return z;
}


//	One strategic node, once per key: a topology node id, or a tagged anchor / crossing key.
static int Adapter_Node(DynamicVectorClass<StrategicNode> & nodes,DynamicVectorClass<unsigned int> & keys,unsigned int key,
								const char * id,StrategicNodeType type,int faction,const Vector2 & at)
{
	for (int i = 0; i < keys.Count(); ++i) {
		if (keys[i] == key) return i;
	}
	StrategicNode node;
	node.Id				= id;
	node.Type			= type;
	node.Faction		= faction;
	node.Position.Set(at.X,at.Y,Ground_Z(at));
	node.Suitability	= 1.0f;
	nodes.Add(node);
	keys.Add(key);
	return nodes.Count() - 1;
}


static const unsigned int	KEY_BASE			= 0x10000000u;
static const unsigned int	KEY_FIELD		= 0x20000000u;
static const unsigned int	KEY_CROSSING	= 0x30000000u;
static const unsigned int	KEY_APPROACH_A	= 0x40000000u;
static const unsigned int	KEY_APPROACH_B	= 0x50000000u;


static int Faction_Of_Team(int team)
{
	return (team == 0) ? PLAYERTYPE_GDI : ((team == 1) ? PLAYERTYPE_NOD : PLAYERTYPE_NEUTRAL);
}


//	The strategic node a topology node is published as: a base is its anchor, every other node a route point.
static int Topology_Node(DynamicVectorClass<StrategicNode> & nodes,DynamicVectorClass<unsigned int> & keys,
								 const HybridRoadRealizerInputs & in,unsigned int node_id)
{
	const HybridTopologyNode * node = in.Topology->Find_Node(node_id);
	if (node == nullptr) return -1;
	StringClass id;
	if (node->Kind == HYBRID_NODE_BASE) {
		const HybridBaseAnchor * base = in.Anchors->Find_Base(node->SiteId);
		if (base != nullptr) {
			id.Format("hybrid_base_%u",base->SiteId);
			return Adapter_Node(nodes,keys,KEY_BASE | base->SiteId,id.Peek_Buffer(),STRATEGIC_NODE_BASE_CANDIDATE,
									  Faction_Of_Team(base->Team),base->Center);
		}
	}
	id.Format("hybrid_node_%u",node_id);
	return Adapter_Node(nodes,keys,node_id,id.Peek_Buffer(),STRATEGIC_NODE_ROUTE_POINT,PLAYERTYPE_NEUTRAL,node->Position);
}


static void Adapter_Edge(DynamicVectorClass<RoadGraphEdge> & edges,const char * id,int road_id,int from,int to,
								 RoadGraphClassType graph_class,float length,const char * required_crossing)
{
	RoadGraphEdge edge;
	edge.Id							= id;
	edge.RoadId						= road_id;
	edge.FromNodeIndex			= from;
	edge.ToNodeIndex				= to;
	edge.Class						= graph_class;
	edge.Length						= length;
	edge.RequiredBridgeCrossingId	= (required_crossing != nullptr) ? required_crossing : "";
	edges.Add(edge);
}


bool HybridRoadRealizer::Publish_Adapters(const HybridRoadRealizerInputs & in,StringClass * error_out)
{
	if ((in.Plan == nullptr) || (in.Topology == nullptr) || (in.Routes == nullptr) || (in.Interactions == nullptr) ||
		 (in.Anchors == nullptr)) {
		return Realizer_Fail(error_out,"the Phase-12 adapters need the frozen records");
	}
	const HybridRealizationPlan & plan = *in.Plan;
	const HybridTopology & topology = *in.Topology;
	StringClass detail;

	DynamicVectorClass<StrategicNode> nodes;
	DynamicVectorClass<unsigned int> keys;
	DynamicVectorClass<RoadGraphEdge> edges;
	DynamicVectorClass<ResolvedBridge> bridges;

	//	The frozen bases and fields, first, so their indices are stable.
	for (int b = 0; b < in.Anchors->Bases.Count(); ++b) {
		const HybridBaseAnchor & base = in.Anchors->Bases[b];
		StringClass id;
		id.Format("hybrid_base_%u",base.SiteId);
		Adapter_Node(nodes,keys,KEY_BASE | base.SiteId,id.Peek_Buffer(),STRATEGIC_NODE_BASE_CANDIDATE,Faction_Of_Team(base.Team),
						 base.Center);
	}
	for (int f = 0; f < in.Anchors->Resources.Count(); ++f) {
		const HybridResourceAnchor & field = in.Anchors->Resources[f];
		StringClass id;
		id.Format("hybrid_field_%u",field.SiteId);
		Adapter_Node(nodes,keys,KEY_FIELD | field.SiteId,id.Peek_Buffer(),STRATEGIC_NODE_RESOURCE_CANDIDATE,PLAYERTYPE_NEUTRAL,
						 field.Center);
	}

	for (int e = 0; e < topology.Edges.Count(); ++e) {

		const HybridTopologyEdge & edge = topology.Edges[e];
		if (Hybrid_Route_Edge_Withdrawn(topology,*in.Routes,edge)) continue;

		const int from = Topology_Node(nodes,keys,in,Hybrid_Route_Edge_End(topology,*in.Routes,edge,false));
		const int to = Topology_Node(nodes,keys,in,Hybrid_Route_Edge_End(topology,*in.Routes,edge,true));
		if ((from < 0) || (to < 0)) continue;
		StringClass id;

		//	A base's internal link to its entrance: no road of its own, but the graph has to reach the base.
		if (!edge.Is_Realized()) {
			id.Format("hybrid_edge_%u",edge.StableId);
			const Vector3 d = nodes[to].Position - nodes[from].Position;
			Adapter_Edge(edges,id.Peek_Buffer(),-1,from,to,ROAD_GRAPH_SERVICE,Vector2(d.X,d.Y).Length(),nullptr);
			continue;
		}

		const int road_index = Plan_Road_Index(plan,edge.StableId);
		if (road_index < 0) continue;					// an optional edge Phase 10 dropped
		const HybridRealizedRoad & road = plan.Roads[road_index];
		const HybridTopologyNode * node_a = topology.Find_Node(Hybrid_Route_Edge_End(topology,*in.Routes,edge,false));
		const HybridTopologyNode * node_b = topology.Find_Node(Hybrid_Route_Edge_End(topology,*in.Routes,edge,true));
		const float s_a = Road_Station_Of(road,node_a->Position);
		const float s_b = Road_Station_Of(road,node_b->Position);
		const bool forward = (s_a <= s_b);

		//	This edge's river and canyon bridges, in travel order: each splits the edge at its two deck ends.
		DynamicVectorClass<int> decks;
		DynamicVectorClass<float> deck_station;
		for (int b = 0; b < plan.Bridges.Count(); ++b) {
			const HybridBridgeAssembly & assembly = plan.Bridges[b];
			if ((assembly.Method != HYBRID_METHOD_BRIDGE) || (assembly.EdgeId != edge.StableId)) continue;
			const float station = Road_Station_Of(road,(assembly.Start + assembly.End) * 0.5f);
			int slot = decks.Count();
			decks.Add(b);
			deck_station.Add(station);
			while ((slot > 0) && (forward ? (deck_station[slot - 1] > station) : (deck_station[slot - 1] < station))) {
				decks[slot]				= decks[slot - 1];
				deck_station[slot]	= deck_station[slot - 1];
				slot--;
			}
			decks[slot]				= b;
			deck_station[slot]	= station;
		}

		const RoadGraphClassType graph_class = Graph_Class_Of(edge.RoadClass);
		int at = from;
		float at_station = s_a;
		for (int d = 0; d <= decks.Count(); ++d) {

			//	The open road up to the next deck's near end, or to the edge's far node.
			int next = to;
			float next_station = s_b;
			int near_node = -1;
			int far_node = -1;
			float far_station = 0.0f;
			if (d < decks.Count()) {
				const HybridBridgeAssembly & assembly = plan.Bridges[decks[d]];
				const float start_station = Road_Station_Of(road,assembly.Start);
				const float end_station = Road_Station_Of(road,assembly.End);
				const bool start_near = forward ? (start_station <= end_station) : (start_station >= end_station);
				StringClass near_id;
				StringClass far_id;
				near_id.Format("hybrid_bridge_%u_%c",assembly.StableId,start_near ? 'a' : 'b');
				far_id.Format("hybrid_bridge_%u_%c",assembly.StableId,start_near ? 'b' : 'a');
				near_node = Adapter_Node(nodes,keys,(start_near ? KEY_APPROACH_A : KEY_APPROACH_B) | assembly.StableId,
												 near_id.Peek_Buffer(),STRATEGIC_NODE_ROUTE_POINT,PLAYERTYPE_NEUTRAL,
												 start_near ? assembly.Start : assembly.End);
				far_node = Adapter_Node(nodes,keys,(start_near ? KEY_APPROACH_B : KEY_APPROACH_A) | assembly.StableId,
												far_id.Peek_Buffer(),STRATEGIC_NODE_ROUTE_POINT,PLAYERTYPE_NEUTRAL,
												start_near ? assembly.End : assembly.Start);
				next				= near_node;
				next_station	= start_near ? start_station : end_station;
				far_station		= start_near ? end_station : start_station;
			}

			const float middle = 0.5f * (at_station + next_station);
			int road_id = -1;
			for (int q = 0; (q < Pieces.Count()) && (road_id < 0); ++q) {
				if ((Pieces[q].RoadIndex == road_index) && (middle >= Pieces[q].FromStation) && (middle <= Pieces[q].ToStation)) {
					road_id = Pieces[q].RoadId;
				}
			}
			if (decks.Count() == 0)	id.Format("hybrid_edge_%u",edge.StableId);
			else							id.Format("hybrid_edge_%u_%d",edge.StableId,d);
			Adapter_Edge(edges,id.Peek_Buffer(),road_id,at,next,graph_class,WWMath::Fabs(next_station - at_station),nullptr);
			if (d == decks.Count()) break;

			//	The deck itself: the bridge's own edge, from one bank to the other.
			const HybridBridgeAssembly & assembly = plan.Bridges[decks[d]];
			const HybridInteraction * compiled = in.Interactions->Find(assembly.InteractionId);
			StringClass crossing_id;
			crossing_id.Format("hybrid_crossing_%u",assembly.StableId);
			Adapter_Node(nodes,keys,KEY_CROSSING | assembly.StableId,crossing_id.Peek_Buffer(),STRATEGIC_NODE_CROSSING_CANDIDATE,
							 PLAYERTYPE_NEUTRAL,(assembly.Start + assembly.End) * 0.5f);
			StringClass deck_edge;
			deck_edge.Format("hybrid_edge_%u_bridge_%u",edge.StableId,assembly.StableId);
			const bool mandatory = (compiled != nullptr) && compiled->Mandatory;
			Adapter_Edge(edges,deck_edge.Peek_Buffer(),-1,near_node,far_node,graph_class,WWMath::Fabs(far_station - next_station),
							 mandatory ? crossing_id.Peek_Buffer() : nullptr);

			ResolvedBridge resolved;
			resolved.Id							.Format("hybrid_bridge_%u",assembly.StableId);
			resolved.BridgeId					= BridgeIds[decks[d]];
			resolved.CrossingNodeId			= crossing_id;
			resolved.RoadEdgeId				= deck_edge;
			resolved.InitialState			= BRIDGE_STATE_PRISTINE;		// generated bridges start pristine (Section 12)
			resolved.CountsAsConnectivity	= true;
			resolved.StrategicImportance	= (graph_class == ROAD_GRAPH_HIGHWAY) ? 1.0f
													: ((graph_class == ROAD_GRAPH_PRIMARY) ? 0.6f
													: ((graph_class == ROAD_GRAPH_SECONDARY) ? 0.4f : 0.2f));
			bridges.Add(resolved);

			at				= far_node;
			at_station	= far_station;
		}
	}

	//	Each field reached from its access point: no road mesh there (Section 13's apron), but the graph has to reach it.
	for (int n = 0; n < topology.Nodes.Count(); ++n) {
		const HybridTopologyNode & node = topology.Nodes[n];
		if (node.Kind != HYBRID_NODE_FIELD_ACCESS) continue;
		const HybridResourceAnchor * field = in.Anchors->Find_Resource(node.SiteId);
		if (field == nullptr) continue;
		StringClass field_id;
		field_id.Format("hybrid_field_%u",field->SiteId);
		const int access = Topology_Node(nodes,keys,in,node.StableId);
		const int target = Adapter_Node(nodes,keys,KEY_FIELD | field->SiteId,field_id.Peek_Buffer(),
												  STRATEGIC_NODE_RESOURCE_CANDIDATE,PLAYERTYPE_NEUTRAL,field->Center);
		StringClass id;
		id.Format("hybrid_apron_%u",node.StableId);
		Adapter_Edge(edges,id.Peek_Buffer(),-1,access,target,ROAD_GRAPH_SERVICE,(field->Center - node.Position).Length(),nullptr);
	}

	//	The frozen base pair.  Phase 8 measured its fairness and Phase 9 gave it edge-disjoint heavy routes.
	const HybridBaseAnchor * gdi = in.Anchors->Base_Of_Team(0);
	const HybridBaseAnchor * nod = in.Anchors->Base_Of_Team(1);
	if ((gdi == nullptr) || (nod == nullptr)) return Realizer_Fail(error_out,"the hybrid anchors have no base pair");
	SelectedBaseSite sites[2];
	const HybridBaseAnchor * pair[2] = { gdi,nod };
	for (int s = 0; s < 2; ++s) {
		sites[s].Id.Format("hybrid_base_%u",pair[s]->SiteId);
		sites[s].Faction									= Faction_Of_Team(pair[s]->Team);
		sites[s].Position.Set(pair[s]->Center.X,pair[s]->Center.Y,Ground_Z(pair[s]->Center));
		sites[s].ProtectedRadius						= pair[s]->ProtectedRadius;
		sites[s].InitialResourceTravelTimeSeconds	= -1.0f;
		sites[s].VehicleRouteCount						= -1;
		sites[s].InfantryRouteCount					= -1;
	}
	BaseFairnessMetrics metrics;
	metrics.SingleBridgeDependency	= false;
	metrics.PassedThresholds			= true;

	const unsigned int checksum = plan.Checksum;
	StrategicPlanner::Adopt_Plan(nodes,checksum ^ 0x53504C4Eu);
	RoadGraphGenerator::Adopt_Graph(edges,checksum ^ 0x52474547u);
	BridgePlanner::Adopt_Bridges(bridges,checksum ^ 0x42524447u);
	BaseSelectionPlanner::Adopt_Selection(sites[0],sites[1],metrics,checksum ^ 0x42415345u);
	return true;
}
