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
int													HybridRoadRealizer::SweptSamples		= 0;
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
**	Section 12.3 -- the physical verification, against the runtime objects.
***********************************************************************************************/

//	The paved surface of every road Phase 12 created, bucketed for the swept query.
struct PavedIndex
{
	PavedIndex(void) : OriginX(0.0f),OriginY(0.0f),Cell(1.0f),CountX(0),CountY(0) { }

	float							OriginX;
	float							OriginY;
	float							Cell;
	int							CountX;
	int							CountY;
	DynamicVectorClass<int>	Head;			// per bucket: its first entry, -1 when empty
	DynamicVectorClass<int>	Next;			// per entry
	DynamicVectorClass<int>	Piece;		// per entry: the realized piece (its RoadSystem road and plan road)
	DynamicVectorClass<int>	Point;		// per entry: the segment's first tessellated point
};


//	A pier or a decoration post of a runtime bridge: a square post on the bridge's axis, bottom to top.
struct PierBox
{
	PierBox(void) : Center(0.0f,0.0f),Axis(1.0f,0.0f),Half(0.0f),Bottom(0.0f),Top(0.0f),BridgeId(-1) { }

	bool	operator == (const PierBox & that) const	{ return (Center == that.Center) && (BridgeId == that.BridgeId); }
	bool	operator != (const PierBox & that) const	{ return !(*this == that); }

	Vector2	Center;
	Vector2	Axis;
	float		Half;
	float		Bottom;
	float		Top;
	int		BridgeId;
};


//	Swept placements tested by the current verification.
static int	Swept = 0;
/*
**	How far the realized ground may stand from a solved surface.  The ground is bilinear over the terrain grid, and
**	its vertices hold the solved surfaces, so between them it can depart from a surface by no more than that surface's
**	slope over one cell diagonal -- a grade break (a deck end, a junction's level patch) inside a cell, or two roads'
**	surfaces meeting in one.  The class grade bounds the slope.
*/
static float Realized_Tolerance(const HybridConstants & k,float cell,HybridRoadClass road_class)
{
	const float grade = WWMath::Tan(Hybrid_Road_Class_Spec(road_class).MaxGradeDegrees * (WWMATH_PI / 180.0f));
	return (2.0f * k.Q) + (grade * cell * WWMath::Sqrt(2.0f));
}


//	What every swept placement reads.
struct SweepContext
{
	const HybridRoadRealizerInputs *	In;
	const PavedIndex *					Paved;
	const DynamicVectorClass<HybridRealizedPiece> *	Pieces;
	const DynamicVectorClass<PierBox> *	Piers;
	float										Spacing;
	float										Cell;					// the terrain grid's spacing
	DynamicVectorClass<Vector2>		Points;		// the plan road being swept, by station
	DynamicVectorClass<float>			Stations;
};


//	A runtime road's paved edge either side of a tessellated point: carriageway, shoulder and edge profile.
static float Paved_Left(const RoadSplineClass & road,const RoadPointClass & point)
{
	return point.Left_Half_Width() + road.Get_Shoulder_Width() + point.ShoulderExtraLeft;
}

static float Paved_Right(const RoadSplineClass & road,const RoadPointClass & point)
{
	return point.Right_Half_Width() + road.Get_Shoulder_Width() + point.ShoulderExtraRight;
}


static float Unit_Length(Vector2 * v)
{
	const float length = v->Length();
	if (length > 0.0f) *v = *v * (1.0f / length);
	return length;
}


static int Bucket_Of(const PavedIndex & index,float x,float y)
{
	const int bx = (int)::floor((double)((x - index.OriginX) / index.Cell));
	const int by = (int)::floor((double)((y - index.OriginY) / index.Cell));
	if ((bx < 0) || (by < 0) || (bx >= index.CountX) || (by >= index.CountY)) return -1;
	return (by * index.CountX) + bx;
}


static void Build_Paved_Index(const DynamicVectorClass<HybridRealizedPiece> & pieces,float cell,PavedIndex * index)
{
	float lo_x = 3.4e38f;
	float lo_y = 3.4e38f;
	float hi_x = -3.4e38f;
	float hi_y = -3.4e38f;
	float reach = 0.0f;
	for (int q = 0; q < pieces.Count(); ++q) {
		const RoadSplineClass * road = RoadSystem::Peek_Road(pieces[q].RoadId);
		if (road == nullptr) continue;
		for (int i = 0; i < road->Get_Point_Count(); ++i) {
			const RoadPointClass & p = road->Get_Point(i);
			lo_x = WWMath::Min(lo_x,p.Position.X);
			lo_y = WWMath::Min(lo_y,p.Position.Y);
			hi_x = WWMath::Max(hi_x,p.Position.X);
			hi_y = WWMath::Max(hi_y,p.Position.Y);
			reach = WWMath::Max(reach,WWMath::Max(Paved_Left(*road,p),Paved_Right(*road,p)));
		}
	}
	index->CountX = 0;
	index->CountY = 0;
	if (!(hi_x >= lo_x)) return;
	index->Cell		= cell;
	index->OriginX	= lo_x - reach - cell;
	index->OriginY	= lo_y - reach - cell;
	index->CountX	= (int)::ceil((double)((hi_x + reach + cell - index->OriginX) / cell)) + 1;
	index->CountY	= (int)::ceil((double)((hi_y + reach + cell - index->OriginY) / cell)) + 1;
	index->Head.Resize(index->CountX * index->CountY);
	for (int b = 0; b < (index->CountX * index->CountY); ++b) index->Head.Add(-1);

	for (int q = 0; q < pieces.Count(); ++q) {
		const RoadSplineClass * road = RoadSystem::Peek_Road(pieces[q].RoadId);
		if (road == nullptr) continue;
		for (int i = 0; (i + 1) < road->Get_Point_Count(); ++i) {
			const RoadPointClass & a = road->Get_Point(i);
			const RoadPointClass & b = road->Get_Point(i + 1);
			const float extent = WWMath::Max(WWMath::Max(Paved_Left(*road,a),Paved_Right(*road,a)),
														WWMath::Max(Paved_Left(*road,b),Paved_Right(*road,b)));
			const int x0 = (int)::floor((double)((WWMath::Min(a.Position.X,b.Position.X) - extent - index->OriginX) / cell));
			const int y0 = (int)::floor((double)((WWMath::Min(a.Position.Y,b.Position.Y) - extent - index->OriginY) / cell));
			const int x1 = (int)::floor((double)((WWMath::Max(a.Position.X,b.Position.X) + extent - index->OriginX) / cell));
			const int y1 = (int)::floor((double)((WWMath::Max(a.Position.Y,b.Position.Y) + extent - index->OriginY) / cell));
			for (int by = ((y0 > 0) ? y0 : 0); by <= ((y1 < (index->CountY - 1)) ? y1 : (index->CountY - 1)); ++by) {
				for (int bx = ((x0 > 0) ? x0 : 0); bx <= ((x1 < (index->CountX - 1)) ? x1 : (index->CountX - 1)); ++bx) {
					const int bucket = (by * index->CountX) + bx;
					index->Next.Add(index->Head[bucket]);
					index->Head[bucket] = index->Piece.Count();
					index->Piece.Add(q);
					index->Point.Add(i);
				}
			}
		}
	}
}


/*
**	The station on a planned road nearest `q`, searched over the plan samples a piece's segment came from.
*/
static float Plan_Station_Near(const HybridRealizedRoad & road,int first,int last,const Vector2 & q)
{
	float best = 3.4e38f;
	float station = road.Samples[first].Station;
	for (int s = first + 1; s <= last; ++s) {
		const Vector2 a = road.Samples[s - 1].Position;
		const Vector2 d = road.Samples[s].Position - a;
		const float length2 = (d.X * d.X) + (d.Y * d.Y);
		float t = (length2 > 0.0f) ? ((((q.X - a.X) * d.X) + ((q.Y - a.Y) * d.Y)) / length2) : 0.0f;
		t = WWMath::Clamp(t,0.0f,1.0f);
		const float distance = (q - (a + (d * t))).Length();
		if (distance >= best) continue;
		best		= distance;
		station	= road.Samples[s - 1].Station + ((road.Samples[s].Station - road.Samples[s - 1].Station) * t);
	}
	return station;
}


/*
**	Is `q` on the paved roadbed of a road Phase 12 created?  Each segment's band between its two tessellated
**	points, and the joint at each interior point; a road's own ends are square, so a vehicle past one is not on it.
**	Where roadbeds overlap -- a junction, a gore -- the surface is the earliest road's (12.2 step 5): `owner_out`
**	receives that road's plan index and `station_out` the station on it.
*/
static bool On_Paved_Road(const PavedIndex & index,const DynamicVectorClass<HybridRealizedPiece> & pieces,
								  const HybridRealizationPlan & plan,const Vector2 & q,float slack,int * owner_out,float * station_out)
{
	*owner_out = -1;
	const int bucket = (index.CountX > 0) ? Bucket_Of(index,q.X,q.Y) : -1;
	if (bucket < 0) return false;
	int owner_entry = -1;
	for (int e = index.Head[bucket]; e >= 0; e = index.Next[e]) {
		const HybridRealizedPiece & piece = pieces[index.Piece[e]];
		if ((*owner_out >= 0) && (piece.RoadIndex >= *owner_out)) continue;
		const RoadSplineClass * road = RoadSystem::Peek_Road(piece.RoadId);
		if (road == nullptr) continue;
		const RoadPointClass & a = road->Get_Point(index.Point[e]);
		const RoadPointClass & b = road->Get_Point(index.Point[e] + 1);
		const Vector2 off(q.X - a.Position.X,q.Y - a.Position.Y);
		const Vector2 d(b.Position.X - a.Position.X,b.Position.Y - a.Position.Y);
		const float length2 = (d.X * d.X) + (d.Y * d.Y);
		bool inside = false;
		if (length2 > 0.0f) {
			const float t = ((off.X * d.X) + (off.Y * d.Y)) / length2;
			if ((t >= 0.0f) && (t <= 1.0f)) {
				const float lateral = ((d.X * off.Y) - (d.Y * off.X)) / WWMath::Sqrt(length2);
				const float left = Paved_Left(*road,a) + ((Paved_Left(*road,b) - Paved_Left(*road,a)) * t);
				const float right = Paved_Right(*road,a) + ((Paved_Right(*road,b) - Paved_Right(*road,a)) * t);
				inside = (lateral <= (left + slack)) && (-lateral <= (right + slack));
			}
		}
		if (!inside && (index.Point[e] > 0)) {
			const bool left_side = ((off.X * a.Side.X) + (off.Y * a.Side.Y)) >= 0.0f;
			inside = (off.Length() <= ((left_side ? Paved_Left(*road,a) : Paved_Right(*road,a)) + slack));
		}
		if (!inside) continue;
		*owner_out	= piece.RoadIndex;
		owner_entry	= e;
	}
	if (owner_entry < 0) return false;
	const HybridRealizedPiece & piece = pieces[index.Piece[owner_entry]];
	*station_out = Plan_Station_Near(plan.Roads[piece.RoadIndex],piece.FirstSample,piece.LastSample,q);
	return true;
}


static float Deck_Thickness(int bridge_id)
{
	const BridgeClass * bridge = BridgeSystem::Peek_Bridge(bridge_id);
	const BridgeDefinitionClass * definition = (bridge != nullptr) ? BridgeSystem::Find_Definition(bridge->Get_Definition()) : nullptr;
	return (definition != nullptr) ? definition->Get_Deck_Thickness() : 0.0f;
}


/*
**	The runtime surface a vehicle expecting `expected` stands on at `q`: a deck when one is there and nearer the
**	expectation than the ground, the terrain otherwise.  `overhead_out` receives the bottom of a deck above a vehicle
**	standing on the ground, or a large value when there is none.
*/
static bool Surface_At(const Vector2 & q,float expected,float * support_out,bool * on_deck_out,float * overhead_out)
{
	float ground = 0.0f;
	if (!WorldTerrainSystem::Sample_Height(q.X,q.Y,&ground)) return false;
	float deck = 0.0f;
	int bridge = -1;
	*overhead_out = 3.4e38f;
	*on_deck_out = false;
	*support_out = ground;
	if (BridgeSystem::Conform_Point(q.X,q.Y,&deck,&bridge)) {
		if (WWMath::Fabs(deck - expected) <= WWMath::Fabs(ground - expected)) {
			*support_out = deck;
			*on_deck_out = true;
		} else {
			const float bottom = deck - Deck_Thickness(bridge);
			if (bottom > expected) *overhead_out = bottom;
		}
	}
	return true;
}


//	The planned road at `station`: its solved height and its paved edges left and right of travel.
static void Plan_At(const HybridRealizedRoad & road,float station,float * z_out,float * left_out,float * right_out)
{
	const int count = road.Samples.Count();
	int lo = 0;
	int hi = count - 1;
	while ((hi - lo) > 1) {
		const int mid = (lo + hi) / 2;
		if (road.Samples[mid].Station <= station) {
			lo = mid;
		} else {
			hi = mid;
		}
	}
	const HybridRealizedSample & a = road.Samples[lo];
	const HybridRealizedSample & b = road.Samples[hi];
	const float span = b.Station - a.Station;
	float t = (span > 0.0f) ? ((station - a.Station) / span) : 0.0f;
	t = (t < 0.0f) ? 0.0f : ((t > 1.0f) ? 1.0f : t);
	*z_out		= a.Z + ((b.Z - a.Z) * t);
	*left_out	= a.Left + ((b.Left - a.Left) * t);
	*right_out	= a.Right + ((b.Right - a.Right) * t);
}


//	Two boxes in plan, each a centre, a unit axis and the half extents along it and across it: do they overlap?
static bool Boxes_Overlap(const Vector2 & c1,const Vector2 & u1,float a1,float b1,
								  const Vector2 & c2,const Vector2 & u2,float a2,float b2)
{
	const Vector2 v1(-u1.Y,u1.X);
	const Vector2 v2(-u2.Y,u2.X);
	const Vector2 d = c2 - c1;
	const Vector2 axes[4] = { u1,v1,u2,v2 };
	for (int i = 0; i < 4; ++i) {
		const Vector2 & n = axes[i];
		const float r1 = (a1 * WWMath::Fabs((u1.X * n.X) + (u1.Y * n.Y))) + (b1 * WWMath::Fabs((v1.X * n.X) + (v1.Y * n.Y)));
		const float r2 = (a2 * WWMath::Fabs((u2.X * n.X) + (u2.Y * n.Y))) + (b2 * WWMath::Fabs((v2.X * n.X) + (v2.Y * n.Y)));
		if (WWMath::Fabs((d.X * n.X) + (d.Y * n.Y)) > (r1 + r2)) return false;
	}
	return true;
}


//	Every pier and decoration post on the map, as a box in plan with its vertical extent.
static void Collect_Piers(DynamicVectorClass<PierBox> * out)
{
	for (int b = 0; b < BridgeSystem::Get_Bridge_Count(); ++b) {
		const BridgeClass * bridge = BridgeSystem::Peek_Bridge(b);
		if (bridge == nullptr) continue;
		const float thickness = Deck_Thickness(b);
		Vector2 axis(bridge->Get_End().X - bridge->Get_Start().X,bridge->Get_End().Y - bridge->Get_Start().Y);
		if (!(Unit_Length(&axis) > 0.0f)) axis.Set(1.0f,0.0f);
		for (int s = 0; s < bridge->Get_Section_Count(); ++s) {
			const BridgeSectionClass & section = bridge->Get_Section(s);
			if ((section.Kind != BRIDGE_SECTION_SUPPORT) && (section.Kind != BRIDGE_SECTION_DECORATION)) continue;
			PierBox box;
			box.Center.Set(section.Start.X,section.Start.Y);
			box.Axis			= axis;
			box.Half			= section.HalfWidth;
			box.BridgeId	= b;
			if (section.Kind == BRIDGE_SECTION_SUPPORT) {
				box.Top		= section.Start.Z - thickness;
				box.Bottom	= box.Top - section.Drop;
			} else {
				box.Bottom	= section.Start.Z;
				box.Top		= section.Start.Z + (4.0f * section.HalfWidth);
			}
			out->Add(box);
		}
	}
}


/*
**	A planned road's runtime frame at `plan_point`: the runtime road carrying that station (none on a deck), the point
**	of it nearest, the unit direction and left side there, and its paved edges -- all interpolated between the two
**	tessellated points either side, the way the runtime road's own ribbon and mask read them.
*/
static bool Runtime_Frame(const SweepContext & x,int road_index,float station,const Vector2 plan_point,Vector2 * point_out,
								  Vector2 * direction_out,Vector2 * left_out,float * left_edge_out,float * right_edge_out)
{
	const float slack = x.In->Constants.Q;
	for (int q = 0; q < x.Pieces->Count(); ++q) {
		const HybridRealizedPiece & piece = (*x.Pieces)[q];
		if ((piece.RoadIndex != road_index) || (station < (piece.FromStation - slack)) || (station > (piece.ToStation + slack))) continue;
		const RoadSplineClass * road = RoadSystem::Peek_Road(piece.RoadId);
		if ((road == nullptr) || (road->Get_Point_Count() < 2)) continue;
		float best = 3.4e38f;
		for (int i = 1; i < road->Get_Point_Count(); ++i) {
			const RoadPointClass & a = road->Get_Point(i - 1);
			const RoadPointClass & b = road->Get_Point(i);
			const Vector2 pa(a.Position.X,a.Position.Y);
			const Vector2 d(b.Position.X - a.Position.X,b.Position.Y - a.Position.Y);
			const float length2 = (d.X * d.X) + (d.Y * d.Y);
			float s = (length2 > 0.0f) ? ((((plan_point.X - pa.X) * d.X) + ((plan_point.Y - pa.Y) * d.Y)) / length2) : 0.0f;
			s = WWMath::Clamp(s,0.0f,1.0f);
			const Vector2 at = pa + (d * s);
			const float distance = (plan_point - at).Length();
			if (distance >= best) continue;
			best = distance;
			*point_out = at;
			*direction_out = Vector2(a.Tangent.X + ((b.Tangent.X - a.Tangent.X) * s),a.Tangent.Y + ((b.Tangent.Y - a.Tangent.Y) * s));
			Unit_Length(direction_out);
			left_out->Set(-direction_out->Y,direction_out->X);
			*left_edge_out = Paved_Left(*road,a) + ((Paved_Left(*road,b) - Paved_Left(*road,a)) * s);
			*right_edge_out = Paved_Right(*road,a) + ((Paved_Right(*road,b) - Paved_Right(*road,a)) * s);
		}
		return true;
	}
	return false;
}


/*
**	The canonical swept/collision query of one envelope along one planned road, at every one of its solved samples
**	(Section 12.1's `min(W/2, A/3)`, every structure end and piece seam among them).  The envelope stands along the
**	road's own direction, kept wholly on the road at its two ends, and across it on the road's paved roadbed midpoint
**	-- or, where that roadbed alone is narrower than the envelope (a ramp's gore transition, D-024), flush with one of
**	its paved edges over the paved surface it has joined.  Its footprint -- three stations along it, three offsets
**	across it -- must stand on paved runtime surface (a road Phase 12 created, or a deck); on the solved surface within
**	the support tolerance, the surface being the earliest road's where roadbeds overlap (12.2 step 5); at the class
**	grade and cross slope; under no deck closer than its height plus Section 12.3's safety; and clear of every pier.
*/
static bool Sweep_Road(SweepContext & x,int road_index,const HybridSweptEnvelope & envelope,const char * which,
							  StringClass * error_out)
{
	const HybridRealizedRoad & road = x.In->Plan->Roads[road_index];
	const HybridConstants & k = x.In->Constants;
	const HybridRoadClassSpec & spec = Hybrid_Road_Class_Spec(road.RoadClass);
	const float max_grade = WWMath::Tan(spec.MaxGradeDegrees * (WWMATH_PI / 180.0f));
	const float max_cross = WWMath::Tan(spec.MaxCrossSlopeDegrees * (WWMATH_PI / 180.0f));
	const int count = road.Samples.Count();
	if ((count < 2) || !(envelope.Length > 0.0f) || !(envelope.Width > 0.0f)) return true;
	x.Points.Delete_All();
	x.Stations.Delete_All();
	for (int i = 0; i < count; ++i) {
		x.Points.Add(road.Samples[i].Position);
		x.Stations.Add(road.Samples[i].Station);
	}
	const float total = road.Samples[count - 1].Station;
	const float half_length = 0.5f * envelope.Length;
	const float half_width = 0.5f * envelope.Width;
	const float gap = Hybrid_Overpass_Clear_Gap(envelope.Height,k.WorldUnitsPerMeter);
	const float first = WWMath::Min(half_length,0.5f * total);
	const float last = WWMath::Max(total - half_length,0.5f * total);
	const float tolerance = Realized_Tolerance(k,x.Cell,road.RoadClass);
	StringClass detail;

	for (int i = 0; i < count; ++i) {
		const float centre_station = WWMath::Clamp(road.Samples[i].Station,first,last);
		Swept++;

		//	The road under the footprint's three stations.
		Vector2 point[3];
		Vector2 direction[3];
		Vector2 left[3];
		float expected[3];
		float left_edge[3];
		float right_edge[3];
		bool on_road[3];
		for (int a = 0; a < 3; ++a) {
			const float at = WWMath::Clamp(centre_station + (half_length * (float)(a - 1)),0.0f,total);
			point[a] = Hybrid_Polyline_Point_At(x.Points,x.Stations,at);
			Plan_At(road,at,&expected[a],&left_edge[a],&right_edge[a]);
			//	On the ground the runtime road's own frame and edges; on a deck the plan's, the deck carrying it.
			on_road[a] = Runtime_Frame(x,road_index,at,point[a],&point[a],&direction[a],&left[a],&left_edge[a],&right_edge[a]);
			if (!on_road[a]) {
				direction[a] = Hybrid_Polyline_Point_At(x.Points,x.Stations,WWMath::Min(at + x.Spacing,total)) -
									Hybrid_Polyline_Point_At(x.Points,x.Stations,WWMath::Max(at - x.Spacing,0.0f));
				Unit_Length(&direction[a]);
				left[a].Set(-direction[a].Y,direction[a].X);
			}
		}

		//	The placement across the road: the roadbed midpoint, else flush with its left or its right paved edge.
		Vector2 q[3][3];
		float surface[3][3];
		float limit[3][3];
		float centre[3];
		bool found = false;
		for (int placement = 0; (placement < 3) && !found; ++placement) {
			bool all = true;
			for (int a = 0; (a < 3) && all; ++a) {
				centre[a] = (placement == 0) ? (0.5f * (left_edge[a] - right_edge[a]))
							 : ((placement == 1) ? (left_edge[a] - half_width) : (half_width - right_edge[a]));
				for (int b = 0; (b < 3) && all; ++b) {
					const float offset = centre[a] + (half_width * (float)(b - 1));
					q[a][b] = point[a] + (left[a] * offset);
					int owner = -1;
					float owner_station = 0.0f;
					surface[a][b] = expected[a];
					limit[a][b] = tolerance;
					const bool own = on_road[a] && (offset <= (left_edge[a] + k.Q)) && (-offset <= (right_edge[a] + k.Q));
					if (On_Paved_Road(*x.Paved,*x.Pieces,*x.In->Plan,q[a][b],k.Q,&owner,&owner_station) || own) {
						//	On another road's roadbed -- a junction, a gore -- the surface is the earlier road's, as solved.
						if ((owner >= 0) && (owner < road_index)) {
							float ignored_left = 0.0f;
							float ignored_right = 0.0f;
							Plan_At(x.In->Plan->Roads[owner],owner_station,&surface[a][b],&ignored_left,&ignored_right);
							limit[a][b] = WWMath::Max(tolerance,Realized_Tolerance(k,x.Cell,x.In->Plan->Roads[owner].RoadClass));
						}
						continue;
					}
					float deck = 0.0f;
					all = BridgeSystem::Conform_Point(q[a][b].X,q[a][b].Y,&deck,nullptr) &&
							(WWMath::Fabs(deck - expected[a]) <= tolerance);
				}
			}
			found = all;
		}
		if (!found) {
			detail.Format("road %u: the %.2f-wide %s envelope leaves the paved surface at station %.2f (%.3f + %.3f paved)",
							  road.EdgeId,envelope.Width,which,centre_station,left_edge[1],right_edge[1]);
			return Realizer_Fail(error_out,detail.Peek_Buffer());
		}

		//	What it stands on there, and what is above it.
		float support[3][3];
		for (int a = 0; a < 3; ++a) {
			for (int b = 0; b < 3; ++b) {
				bool on_deck = false;
				float overhead = 0.0f;
				if (!Surface_At(q[a][b],surface[a][b],&support[a][b],&on_deck,&overhead)) {
					detail.Format("road %u: the %s envelope leaves the terrain at (%.2f,%.2f)",road.EdgeId,which,q[a][b].X,q[a][b].Y);
					return Realizer_Fail(error_out,detail.Peek_Buffer());
				}
				const float off = support[a][b] - surface[a][b];
				if (WWMath::Fabs(off) > limit[a][b]) {
					detail.Format("road %u: the %s envelope stands %.3f off the solved surface %.3f at (%.2f,%.2f)",road.EdgeId,which,
									  off,surface[a][b],q[a][b].X,q[a][b].Y);
					return Realizer_Fail(error_out,detail.Peek_Buffer());
				}
				if ((overhead - surface[a][b]) < (gap - k.Q)) {
					detail.Format("road %u: a deck %.3f above the road at (%.2f,%.2f) leaves the %s envelope under %.3f",
									  road.EdgeId,overhead - surface[a][b],q[a][b].X,q[a][b].Y,which,gap);
					return Realizer_Fail(error_out,detail.Peek_Buffer());
				}
				//	The road's own solved surface plus what the realized ground adds, for its grade and cross slope.
				support[a][b] = expected[a] + off;
			}
		}

		//	Every pier and post the footprint's box meets at the height the envelope occupies.
		const Vector2 footprint = point[1] + (left[1] * centre[1]);
		for (int p = 0; p < x.Piers->Count(); ++p) {
			const PierBox & pier = (*x.Piers)[p];
			if ((pier.Top <= expected[1]) || (pier.Bottom >= (expected[1] + envelope.Height))) continue;
			if (!Boxes_Overlap(footprint,direction[1],half_length,half_width,pier.Center,pier.Axis,pier.Half,pier.Half)) continue;
			detail.Format("road %u: the %s envelope meets a pier of bridge %d at (%.2f,%.2f)",road.EdgeId,which,pier.BridgeId,
							  pier.Center.X,pier.Center.Y);
			return Realizer_Fail(error_out,detail.Peek_Buffer());
		}

		//	The class grade along the footprint and its cross slope.
		const float length = WWMath::Max(2.0f * half_length,k.Q);
		const float grade = WWMath::Fabs(support[2][1] - support[0][1]) / length;
		const float cross = WWMath::Fabs(support[1][2] - support[1][0]) / WWMath::Max(2.0f * half_width,k.Q);
		if ((grade > (max_grade + ((2.0f * k.Q) / length))) || (cross > (max_cross + ((2.0f * k.Q) / (2.0f * half_width))))) {
			detail.Format("road %u: the %s envelope stands on a %.4f grade and a %.4f cross slope at station %.2f (class %.4f / %.4f)",
							  road.EdgeId,which,grade,cross,centre_station,max_grade,max_cross);
			return Realizer_Fail(error_out,detail.Peek_Buffer());
		}
	}
	return true;
}


/*
**	Every BridgeAssembly against the runtime bridge built from it: the compiled span and deck ends; the clear width
**	Section 12.4 requires for the road it carries; the deck surface along its whole length at the compiled elevation;
**	the deck bottom over water at or above the compiled one, and an overpass's clear gap over the road beneath; bound
**	at both ends to roads that stop on its deck ends; traversable; and the definition's own stated clearance.
*/
static bool Verify_Bridges(const HybridRoadRealizerInputs & in,const DynamicVectorClass<int> & bridge_ids,
									const DynamicVectorClass<HybridRealizedPiece> & pieces,float cell,StringClass * error_out)
{
	const HybridRealizationPlan & plan = *in.Plan;
	const HybridConstants & k = in.Constants;
	const float tolerance = 2.0f * k.Q;
	StringClass detail;

	for (int b = 0; b < plan.Bridges.Count(); ++b) {
		const HybridBridgeAssembly & assembly = plan.Bridges[b];
		const int id = bridge_ids[b];
		const BridgeClass * bridge = BridgeSystem::Peek_Bridge(id);
		const HybridInteraction * compiled = (in.Interactions != nullptr) ? in.Interactions->Find(assembly.InteractionId) : nullptr;
		const HybridTopologyEdge * edge = in.Topology->Find_Edge(assembly.EdgeId);
		if ((bridge == nullptr) || (compiled == nullptr) || (edge == nullptr)) {
			detail.Format("bridge assembly %u has no runtime bridge or compiled record",assembly.StableId);
			return Realizer_Fail(error_out,detail.Peek_Buffer());
		}

		//	Span: the compiled deck reach, the assembly and the runtime bridge all the same length.
		const float compiled_span = compiled->DeckReachA + compiled->DeckReachB;
		const float assembly_span = (assembly.End - assembly.Start).Length();
		const Vector3 runtime = bridge->Get_End() - bridge->Get_Start();
		const float runtime_span = WWMath::Sqrt((runtime.X * runtime.X) + (runtime.Y * runtime.Y));
		if ((WWMath::Fabs(compiled_span - assembly_span) > tolerance) || (WWMath::Fabs(runtime_span - assembly_span) > tolerance)) {
			detail.Format("bridge assembly %u spans %.3f at runtime, %.3f assembled, %.3f compiled",assembly.StableId,runtime_span,
							  assembly_span,compiled_span);
			return Realizer_Fail(error_out,detail.Peek_Buffer());
		}

		//	Clear width: Section 12.4 for the vehicles the carried road must take, on the bridge and every deck section.
		const float swept = Hybrid_Required_Swept_Width(edge->Capabilities,in.Light.Width,in.Heavy.Width,in.Harvester.Width);
		const float required = Hybrid_Bridge_Clear_Width(edge->RoadClass,k.W,swept,in.MinVehicleLaneClearance);
		float narrowest = bridge->Get_Width();
		for (int s = 0; s < bridge->Get_Section_Count(); ++s) {
			const BridgeSectionClass & section = bridge->Get_Section(s);
			if ((section.Kind == BRIDGE_SECTION_START_CAP) || (section.Kind == BRIDGE_SECTION_SPAN) ||
				 (section.Kind == BRIDGE_SECTION_END_CAP)) {
				narrowest = WWMath::Min(narrowest,2.0f * section.HalfWidth);
			}
		}
		if (narrowest < (required - tolerance)) {
			detail.Format("bridge assembly %u is %.3f clear at its narrowest; road %u needs %.3f",assembly.StableId,narrowest,
							  assembly.EdgeId,required);
			return Realizer_Fail(error_out,detail.Peek_Buffer());
		}

		//	The deck, end to end and edge to edge, at the compiled elevation.
		const Vector2 along = assembly.End - assembly.Start;
		Vector2 axis = along;
		Unit_Length(&axis);
		const Vector2 across(-axis.Y,axis.X);
		const float spacing = Hybrid_Profile_Sample_Spacing(k);
		int steps = (int)::ceil((double)(assembly_span / spacing));
		if (steps < 1) steps = 1;
		const float thickness = Deck_Thickness(id);
		const float reach = (0.5f * required) - tolerance;
		for (int s = 0; s <= steps; ++s) {
			const float t = (float)s / (float)steps;
			const float z = assembly.StartZ + ((assembly.EndZ - assembly.StartZ) * t);
			for (int side = -1; side <= 1; ++side) {
				const Vector2 q = assembly.Start + (along * t) + (across * (reach * (float)side));
				float deck = 0.0f;
				int found = -1;
				if (!BridgeSystem::Conform_Point(q.X,q.Y,&deck,&found) || ((found != id) && (deck < z))) {
					detail.Format("bridge assembly %u carries nothing at (%.2f,%.2f), %.2f along its deck",assembly.StableId,q.X,q.Y,
									  t * assembly_span);
					return Realizer_Fail(error_out,detail.Peek_Buffer());
				}
				if ((found == id) && (WWMath::Fabs(deck - z) > tolerance)) {
					detail.Format("bridge assembly %u's deck stands at %.3f, compiled %.3f, at (%.2f,%.2f)",assembly.StableId,deck,z,
									  q.X,q.Y);
					return Realizer_Fail(error_out,detail.Peek_Buffer());
				}
			}
			//	Over water, the deck bottom no lower than compiled.
			if ((assembly.Method == HYBRID_METHOD_BRIDGE) && (compiled->Obstruction == HYBRID_OBSTRUCTION_RIVER) &&
				 ((z - thickness) < (compiled->DeckBottomZ - tolerance))) {
				detail.Format("bridge assembly %u's deck bottom %.3f is under its compiled %.3f",assembly.StableId,z - thickness,
								  compiled->DeckBottomZ);
				return Realizer_Fail(error_out,detail.Peek_Buffer());
			}
		}
		const float realized = Realized_Tolerance(k,cell,edge->RoadClass);
		if (assembly.Method == HYBRID_METHOD_OVERPASS) {
			const HybridTopologyEdge * lower = in.Topology->Find_Edge(compiled->OtherEdgeId);
			const float lower_tolerance = (lower != nullptr) ? Realized_Tolerance(k,cell,lower->RoadClass) : realized;
			float ground = 0.0f;
			const Vector2 from_start = compiled->Station - assembly.Start;
			const float at = WWMath::Clamp(((from_start.X * axis.X) + (from_start.Y * axis.Y)) / WWMath::Max(assembly_span,k.Q),
													 0.0f,1.0f);
			const float z = assembly.StartZ + ((assembly.EndZ - assembly.StartZ) * at);
			const float gap = Hybrid_Overpass_Clear_Gap(in.Heavy.Height,k.WorldUnitsPerMeter);
			if (!WorldTerrainSystem::Sample_Height(compiled->Station.X,compiled->Station.Y,&ground) ||
				 ((z - thickness - ground) < (gap - lower_tolerance)) || (WWMath::Fabs(ground - compiled->LowerSurfaceZ) > lower_tolerance)) {
				detail.Format("overpass assembly %u: the road beneath stands at %.3f (compiled %.3f), %.3f under the deck bottom; "
								  "Section 12.3 needs %.3f",assembly.StableId,ground,compiled->LowerSurfaceZ,z - thickness - ground,gap);
				return Realizer_Fail(error_out,detail.Peek_Buffer());
			}
		}

		//	Attached at both ends: a road bound to each end that stops on the deck end, at its surface.
		for (int end = 0; end < 2; ++end) {
			const int road_id = (end == 0) ? bridge->Get_Start_Road() : bridge->Get_End_Road();
			const RoadSplineClass * road = RoadSystem::Peek_Road(road_id);
			const Vector2 tip = (end == 0) ? assembly.Start : assembly.End;
			const float tip_z = (end == 0) ? assembly.StartZ : assembly.EndZ;
			bool attached = false;
			for (int q = 0; (road != nullptr) && (q < pieces.Count()) && !attached; ++q) {
				if (pieces[q].RoadId != road_id) continue;
				for (int r = 0; (r < 2) && !attached; ++r) {
					const RoadConnectionClass & connection = (r == 0) ? road->Get_Start_Connection() : road->Get_End_Connection();
					const RoadPointClass & point = road->Get_Point((r == 0) ? 0 : (road->Get_Point_Count() - 1));
					attached = (connection.Type == ROAD_ENDPOINT_BRIDGE) && (connection.TargetID == id) &&
								  ((Vector2(point.Position.X,point.Position.Y) - tip).Length() <= tolerance) &&
								  (WWMath::Fabs(point.Position.Z - tip_z) <= realized);
				}
			}
			if (!attached) {
				detail.Format("bridge assembly %u is not attached at its %s end",assembly.StableId,(end == 0) ? "start" : "end");
				return Realizer_Fail(error_out,detail.Peek_Buffer());
			}
		}
		if (!BridgeSystem::Is_Traversable(id)) {
			detail.Format("bridge assembly %u is realized but not traversable",assembly.StableId);
			return Realizer_Fail(error_out,detail.Peek_Buffer());
		}
		BridgeClearanceReportStruct report;
		BridgeSystem::Check_Clearance(id,&report,spacing);
		if (report.Stated && report.Measured && !report.Satisfied) {
			detail.Format("bridge assembly %u leaves %.3f under its deck, %.2f along it; its definition promises %.3f",
							  assembly.StableId,report.MinimumClearance,report.WorstDistanceAlong,report.RequiredClearance);
			return Realizer_Fail(error_out,detail.Peek_Buffer());
		}
	}
	return true;
}


/*
**	Every realized road sample against its solved surface and cross slope: at each tessellated point of each runtime
**	road, the realized ground at its centre and at its two paved edges against the solved surface there -- the road's
**	own, level across, or an earlier road's where their roadbeds overlap (12.2 step 5) -- within the realization
**	tolerance, and the cross slope those departures put across the roadbed within the class limit.  The grade is the
**	swept query's, over each envelope's length.
*/
static bool Verify_Road_Surfaces(SweepContext & x,StringClass * error_out)
{
	const HybridConstants & k = x.In->Constants;
	const HybridRealizationPlan & plan = *x.In->Plan;
	StringClass detail;
	for (int q = 0; q < x.Pieces->Count(); ++q) {
		const HybridRealizedPiece & piece = (*x.Pieces)[q];
		const HybridRealizedRoad & planned = plan.Roads[piece.RoadIndex];
		const RoadSplineClass * road = RoadSystem::Peek_Road(piece.RoadId);
		if (road == nullptr) continue;													// Verify names it
		const HybridRoadClassSpec & spec = Hybrid_Road_Class_Spec(planned.RoadClass);
		const float max_cross = WWMath::Tan(spec.MaxCrossSlopeDegrees * (WWMATH_PI / 180.0f));
		for (int i = 0; i < road->Get_Point_Count(); ++i) {
			const RoadPointClass & p = road->Get_Point(i);
			const Vector2 centre(p.Position.X,p.Position.Y);
			const Vector2 side(p.Side.X,p.Side.Y);
			const float left = Paved_Left(*road,p);
			const float right = Paved_Right(*road,p);
			float own = 0.0f;
			float ignored_left = 0.0f;
			float ignored_right = 0.0f;
			Plan_At(planned,Plan_Station_Near(planned,piece.FirstSample,piece.LastSample,centre),&own,&ignored_left,&ignored_right);

			float departure[3];
			for (int b = 0; b < 3; ++b) {
				const Vector2 at = (b == 0) ? (centre - (side * right)) : ((b == 1) ? centre : (centre + (side * left)));
				float surface = own;
				float tolerance = Realized_Tolerance(k,x.Cell,planned.RoadClass);
				int owner = -1;
				float owner_station = 0.0f;
				if (On_Paved_Road(*x.Paved,*x.Pieces,plan,at,k.Q,&owner,&owner_station) && (owner >= 0) && (owner < piece.RoadIndex)) {
					Plan_At(plan.Roads[owner],owner_station,&surface,&ignored_left,&ignored_right);
					tolerance = WWMath::Max(tolerance,Realized_Tolerance(k,x.Cell,plan.Roads[owner].RoadClass));
				}
				float support = 0.0f;
				bool on_deck = false;
				float overhead = 0.0f;
				if (!Surface_At(at,surface,&support,&on_deck,&overhead)) {
					detail.Format("road %u leaves the terrain at (%.2f,%.2f)",planned.EdgeId,at.X,at.Y);
					return Realizer_Fail(error_out,detail.Peek_Buffer());
				}
				departure[b] = support - surface;
				if (WWMath::Fabs(departure[b]) > tolerance) {
					detail.Format("road %u's realized ground stands %.3f off its solved surface %.3f at (%.2f,%.2f)",planned.EdgeId,
									  departure[b],surface,at.X,at.Y);
					return Realizer_Fail(error_out,detail.Peek_Buffer());
				}
			}
			const float cross = WWMath::Fabs(departure[2] - departure[0]) / WWMath::Max(left + right,k.Q);
			if (cross > (max_cross + ((2.0f * k.Q) / WWMath::Max(left + right,k.Q)))) {
				detail.Format("road %u's roadbed is realized at a %.4f cross slope at (%.2f,%.2f), class %.4f",planned.EdgeId,cross,
								  centre.X,centre.Y,max_cross);
				return Realizer_Fail(error_out,detail.Peek_Buffer());
			}
		}
	}
	return true;
}


//	The side frame of one interchange side (12.6.7): `tau` along the side's carriageway, `normal` outward.
static void Side_Frame(const Vector2 & axis,int side,Vector2 * tau,Vector2 * normal)
{
	*tau		= (side == HYBRID_INTERCHANGE_SIDE_RIGHT) ? axis : Vector2(-axis.X,-axis.Y);
	*normal	= (side == HYBRID_INTERCHANGE_SIDE_RIGHT) ? Vector2(axis.Y,-axis.X) : Vector2(-axis.Y,axis.X);
}


//	The circle through three points: its radius, and the unit direction of travel at `c` (along a -> b -> c).
static bool Circle_Through(const Vector2 & a,const Vector2 & b,const Vector2 & c,float * radius_out,Vector2 * heading_out)
{
	const float d = 2.0f * ((a.X * (b.Y - c.Y)) + (b.X * (c.Y - a.Y)) + (c.X * (a.Y - b.Y)));
	if (WWMath::Fabs(d) <= 1.0e-6f) {
		*radius_out = 3.4e38f;
		*heading_out = c - b;
		Unit_Length(heading_out);
		return false;
	}
	const float a2 = (a.X * a.X) + (a.Y * a.Y);
	const float b2 = (b.X * b.X) + (b.Y * b.Y);
	const float c2 = (c.X * c.X) + (c.Y * c.Y);
	const Vector2 centre(((a2 * (b.Y - c.Y)) + (b2 * (c.Y - a.Y)) + (c2 * (a.Y - b.Y))) / d,
								((a2 * (c.X - b.X)) + (b2 * (a.X - c.X)) + (c2 * (b.X - a.X))) / d);
	Vector2 radial = c - centre;
	*radius_out = Unit_Length(&radial);
	//	Travel is perpendicular to the radius, turning the way a -> b -> c turns.
	const float turn = ((b.X - a.X) * (c.Y - b.Y)) - ((b.Y - a.Y) * (c.X - b.X));
	*heading_out = (turn > 0.0f) ? Vector2(-radial.Y,radial.X) : Vector2(radial.Y,-radial.X);
	return true;
}


//	The distance from `point` to a polyline, and the station of its nearest point.
static float Polyline_Distance(const DynamicVectorClass<Vector2> & points,const DynamicVectorClass<float> & stations,
										 const Vector2 & point,float * station_out)
{
	float best = 3.4e38f;
	for (int i = 1; i < points.Count(); ++i) {
		const Vector2 d = points[i] - points[i - 1];
		const float length2 = (d.X * d.X) + (d.Y * d.Y);
		float t = (length2 > 0.0f) ? ((((point.X - points[i - 1].X) * d.X) + ((point.Y - points[i - 1].Y) * d.Y)) / length2) : 0.0f;
		t = WWMath::Clamp(t,0.0f,1.0f);
		const float distance = (point - (points[i - 1] + (d * t))).Length();
		if (distance >= best) continue;
		best = distance;
		if (station_out != nullptr) *station_out = stations[i - 1] + ((stations[i] - stations[i - 1]) * t);
	}
	return best;
}


//	The runtime road surface of a planned road at the point of it nearest `point`: the tessellated point heights.
static bool Runtime_Height_Near(const DynamicVectorClass<HybridRealizedPiece> & pieces,int road_index,const Vector2 & point,
										  float * z_out,Vector2 * tangent_out,int * road_id_out)
{
	float best = 3.4e38f;
	for (int q = 0; q < pieces.Count(); ++q) {
		if (pieces[q].RoadIndex != road_index) continue;
		const RoadSplineClass * road = RoadSystem::Peek_Road(pieces[q].RoadId);
		if (road == nullptr) continue;
		for (int i = 1; i < road->Get_Point_Count(); ++i) {
			const Vector3 & a = road->Get_Point(i - 1).Position;
			const Vector3 & b = road->Get_Point(i).Position;
			const Vector2 d(b.X - a.X,b.Y - a.Y);
			const float length2 = (d.X * d.X) + (d.Y * d.Y);
			float t = (length2 > 0.0f) ? ((((point.X - a.X) * d.X) + ((point.Y - a.Y) * d.Y)) / length2) : 0.0f;
			t = WWMath::Clamp(t,0.0f,1.0f);
			const float distance = (point - (Vector2(a.X,a.Y) + (d * t))).Length();
			if (distance >= best) continue;
			best = distance;
			*z_out = a.Z + ((b.Z - a.Z) * t);
			*tangent_out = d;
			Unit_Length(tangent_out);
			*road_id_out = pieces[q].RoadId;
		}
	}
	return best < 3.4e38f;
}


/*
**	Section 12.6.13's ramp list (Phase 19A check 21) for every active interchange, from the realized records and the
**	runtime objects:
**	- both ramp endpoints attached to their intended roads: the canonical junction at L_s (its surface at z_L) and the
**	  highway at the gore (ROAD_ENDPOINT_ATTACHED);
**	- the runtime ramp on its realized centerline; gore metadata matching it: the gore at d_aux, at the highway's
**	  realized surface, tangent to the highway within 0.001 rad, its cross section inner 2.375W / outer 3.825W with
**	  no inner shoulder and the 0.45W outer one;
**	- no turn under R_r; the convergence leg at 15 degrees;
**	- the merge/diverge record's lengths, subdivisions, d_aux and E_aux, and the runtime highway's ramp-side
**	  carriageway and roadbed equal to it over the whole interval, continuous;
**	- no accidental junction with another road; the navigation capabilities the interchange needs carried by every
**	  ramp (their swept envelopes are run by the swept query).
*/
static bool Verify_Interchanges(const HybridRoadRealizerInputs & in,const DynamicVectorClass<HybridRealizedPiece> & pieces,
										  float cell,StringClass * error_out)
{
	const HybridRealizationPlan & plan = *in.Plan;
	const HybridConstants & k = in.Constants;
	HybridInterchangeConstants c;
	Hybrid_Interchange_Constants(k.W,&c);
	const float edge_tolerance = 2.0f * k.Q;
	const float tolerance = Realized_Tolerance(k,cell,HYBRID_ROAD_HIGHWAY_RAMP);
	StringClass detail;

	for (int i = 0; i < in.Topology->Interchanges.Count(); ++i) {
		const HybridInterchange & ic = in.Topology->Interchanges[i];
		if ((ic.Status != HYBRID_INTERCHANGE_ACTIVE) || in.Routes->Interchange_Withdrawn(ic.StableId)) continue;
		const HybridTopologyNode * anchor = in.Topology->Find_Node(ic.AnchorNode);
		const int highway = Plan_Road_Index(plan,ic.HighwayEdgeIn);
		if ((anchor == nullptr) || (highway < 0)) {
			detail.Format("interchange %u has no realized highway",ic.StableId);
			return Realizer_Fail(error_out,detail.Peek_Buffer());
		}
		HybridRampCandidate candidate;
		Hybrid_Ramp_Candidate(c,ic.SelectedK,0.0f,&candidate);
		candidate.LocalOffset			= ic.LocalOffset;
		candidate.GoreOffset				= ic.GoreOffset;
		candidate.StraightHalfExtent	= ic.StraightHalfExtent;

		for (int e = 0; e < ic.RampEdges.Count(); ++e) {
			const HybridTopologyEdge * ramp = in.Topology->Find_Edge(ic.RampEdges[e]);
			const HybridTopologyNode * local = (ramp != nullptr) ? in.Topology->Find_Node(ramp->NodeA) : nullptr;
			const HybridRoute * route = (ramp != nullptr) ? in.Routes->Find(ramp->StableId) : nullptr;
			const int road_index = (ramp != nullptr) ? Plan_Road_Index(plan,ramp->StableId) : -1;
			const HybridRealizedPiece * piece = nullptr;
			for (int q = 0; (q < pieces.Count()) && (piece == nullptr); ++q) {
				if (pieces[q].RoadIndex == road_index) piece = &pieces[q];
			}
			const RoadSplineClass * runtime = (piece != nullptr) ? RoadSystem::Peek_Road(piece->RoadId) : nullptr;
			HybridRoute realized;
			if ((ramp == nullptr) || (local == nullptr) || (route == nullptr) || (runtime == nullptr) ||
				 (runtime->Get_Point_Count() < 3) || !Hybrid_Realized_Route(*route,*in.Interactions,&realized) ||
				 (realized.Points.Count() < 3)) {
				detail.Format("interchange %u: ramp %u is not realized as one runtime road",ic.StableId,ic.RampEdges[e]);
				return Realizer_Fail(error_out,detail.Peek_Buffer());
			}
			const int last = runtime->Get_Point_Count() - 1;
			const Vector3 & start = runtime->Get_Point(0).Position;
			const Vector3 & end = runtime->Get_Point(last).Position;
			Vector2 tau;
			Vector2 normal;
			Side_Frame(ic.Axis,ramp->RampSide,&tau,&normal);
			const float gore_x = (ramp->RampRole == HYBRID_RAMP_ROLE_OFF) ? -ic.GoreOffset : ic.GoreOffset;
			const Vector2 gore = anchor->Position + (tau * gore_x) + (normal * c.DAux);

			//	The runtime ramp on its realized centerline.
			for (int p = 0; p <= last; ++p) {
				const Vector3 & at = runtime->Get_Point(p).Position;
				if (Polyline_Distance(realized.Points,realized.Stations,Vector2(at.X,at.Y),nullptr) > edge_tolerance) {
					detail.Format("interchange %u: ramp %u leaves its realized centerline at (%.2f,%.2f)",ic.StableId,ramp->StableId,
									  at.X,at.Y);
					return Realizer_Fail(error_out,detail.Peek_Buffer());
				}
			}

			//	L_s: the canonical junction there, holding the compiled z_L.
			float z_l = 0.0f;
			bool compiled_local = false;
			for (int r = 0; (r < in.Interactions->Interactions.Count()) && !compiled_local; ++r) {
				const HybridInteraction & one = in.Interactions->Interactions[r];
				if ((one.Role != HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_LOCAL_JUNCTION) || (one.NodeId != local->StableId)) continue;
				z_l = one.JunctionZ;
				compiled_local = true;
			}
			bool junction = false;
			for (int j = 0; (j < RoadSystem::Get_Junction_Count()) && !junction; ++j) {
				const RoadJunctionClass * one = RoadSystem::Peek_Junction(j);
				junction = (one != nullptr) && one->Has_Arm(piece->RoadId) && (one->ArmCount >= 2) &&
							  ((Vector2(one->Position.X,one->Position.Y) - local->Position).Length() <= k.W);
			}
			float ground = 0.0f;
			if (((Vector2(start.X,start.Y) - local->Position).Length() > edge_tolerance) || !compiled_local || !junction ||
				 !WorldTerrainSystem::Sample_Height(local->Position.X,local->Position.Y,&ground) ||
				 (WWMath::Fabs(start.Z - z_l) > tolerance) || (WWMath::Fabs(ground - z_l) > tolerance)) {
				detail.Format("interchange %u: ramp %u is not attached at its local junction %u (junction %d, ramp %.3f, ground %.3f, "
								  "z_L %.3f)",ic.StableId,ramp->StableId,local->StableId,junction ? 1 : 0,start.Z,ground,z_l);
				return Realizer_Fail(error_out,detail.Peek_Buffer());
			}

			//	The gore: attached to the highway, at d_aux, at the highway's realized surface.
			const RoadConnectionClass & attachment = runtime->Get_End_Connection();
			float highway_z = 0.0f;
			Vector2 highway_tangent(0.0f,0.0f);
			int highway_id = -1;
			if ((attachment.Type != ROAD_ENDPOINT_ATTACHED) ||
				 !Runtime_Height_Near(pieces,highway,gore,&highway_z,&highway_tangent,&highway_id) ||
				 (attachment.TargetID != highway_id) || ((Vector2(end.X,end.Y) - gore).Length() > edge_tolerance) ||
				 (WWMath::Fabs(end.Z - highway_z) > tolerance)) {
				detail.Format("interchange %u: ramp %u's gore is not attached to the highway at d_aux (%.3f off, %.3f above it)",
								  ic.StableId,ramp->StableId,(Vector2(end.X,end.Y) - gore).Length(),end.Z - highway_z);
				return Realizer_Fail(error_out,detail.Peek_Buffer());
			}

			//	Tangent to the highway at the gore, and no turn under R_r, on the realized centerline (the arcs' own samples).
			const int count = realized.Points.Count();
			float radius = 0.0f;
			Vector2 heading(0.0f,0.0f);
			const bool from_gore = ((realized.Points[count - 1] - gore).Length() <= edge_tolerance);
			if (from_gore) {
				Circle_Through(realized.Points[count - 3],realized.Points[count - 2],realized.Points[count - 1],&radius,&heading);
			} else {
				Circle_Through(realized.Points[2],realized.Points[1],realized.Points[0],&radius,&heading);
			}
			const float heading_error = WWMath::Fabs((heading.X * ic.Axis.Y) - (heading.Y * ic.Axis.X));
			const float spacing = Hybrid_Profile_Sample_Spacing(k);
			const float point_spacing = realized.Stations[count - 1] / (float)(count - 1);
			int baseline = (int)::floor((double)((c.RampRadius / WWMath::Max(point_spacing,k.Q)) + 0.5f));
			if (baseline < 1) baseline = 1;
			float tightest = 3.4e38f;
			for (int p = baseline; (p + baseline) < count; ++p) {
				float r = 0.0f;
				Vector2 ignored;
				if (Circle_Through(realized.Points[p - baseline],realized.Points[p],realized.Points[p + baseline],&r,&ignored)) {
					tightest = WWMath::Min(tightest,r);
				}
			}
			if (!from_gore || (WWMath::Asin(WWMath::Min(heading_error,1.0f)) > 0.001f) || (tightest < (0.99f * c.RampRadius))) {
				detail.Format("interchange %u: ramp %u meets its gore %.5f rad off the highway, turns at %.3f (R_r %.3f)",ic.StableId,
								  ramp->StableId,WWMath::Asin(WWMath::Min(heading_error,1.0f)),tightest,c.RampRadius);
				return Realizer_Fail(error_out,detail.Peek_Buffer());
			}

			/*
			**	The convergence leg V1 -> V2 at 15 degrees: the realized ramp runs along its straight, between V1's corner
			**	arc and V2's (whose tangent length is T15), through that straight's midpoint and in its direction.
			*/
			Vector2 polygon[4];
			Hybrid_Ramp_Control_Polygon(c,candidate,ramp->RampRole,polygon);
			const bool off = (ramp->RampRole == HYBRID_RAMP_ROLE_OFF);
			const Vector2 corner_l = anchor->Position + (tau * polygon[off ? 3 : 0].X) + (normal * polygon[off ? 3 : 0].Y);
			const Vector2 leg_from = anchor->Position + (tau * polygon[off ? 2 : 1].X) + (normal * polygon[off ? 2 : 1].Y);
			const Vector2 leg_to = anchor->Position + (tau * polygon[off ? 1 : 2].X) + (normal * polygon[off ? 1 : 2].Y);
			Vector2 arrive = leg_from - corner_l;
			Unit_Length(&arrive);
			Vector2 leg = leg_to - leg_from;
			const float leg_length = Unit_Length(&leg);
			const float turn_v1 = WWMath::Acos(WWMath::Clamp((arrive.X * leg.X) + (arrive.Y * leg.Y),-1.0f,1.0f));
			const float straight_from = c.RampRadius * WWMath::Tan(0.5f * turn_v1);
			const float straight = leg_length - straight_from - c.T15;
			const float leg_angle = WWMath::Asin(WWMath::Min(WWMath::Fabs((leg.X * ic.Axis.Y) - (leg.Y * ic.Axis.X)),1.0f));
			const Vector2 middle = leg_from + (leg * (straight_from + (0.5f * straight)));
			float leg_station = 0.0f;
			const float leg_off = Polyline_Distance(realized.Points,realized.Stations,middle,&leg_station);
			const float window = WWMath::Min(0.5f * k.W,0.5f * straight);
			float leg_turn = 0.0f;
			if (window > (2.0f * k.Q)) {
				Vector2 leg_heading = Hybrid_Polyline_Point_At(realized.Points,realized.Stations,leg_station + window) -
											 Hybrid_Polyline_Point_At(realized.Points,realized.Stations,leg_station - window);
				Unit_Length(&leg_heading);
				leg_turn = WWMath::Fabs((leg_heading.X * leg.Y) - (leg_heading.Y * leg.X));
			}
			if ((straight < -edge_tolerance) || (WWMath::Fabs(leg_angle - c.ThetaC) > 0.001f) || (leg_off > edge_tolerance) ||
				 (leg_turn > ((2.0f * k.Q) / WWMath::Max(window,k.Q)))) {
				detail.Format("interchange %u: ramp %u's convergence leg is %.5f rad to the highway, %.4f off its midpoint",
								  ic.StableId,ramp->StableId,leg_angle,leg_off);
				return Realizer_Fail(error_out,detail.Peek_Buffer());
			}

			//	The gore cross section: inner 2.375W, outer 3.825W from the highway centre, no inner shoulder, 0.45W outer.
			{
				const RoadPointClass & tip = runtime->Get_Point(last);
				const bool inner_left = (((tip.Side.X * -normal.X) + (tip.Side.Y * -normal.Y)) > 0.0f);
				const float inner_carriage = inner_left ? tip.Left_Half_Width() : tip.Right_Half_Width();
				const float outer_carriage = inner_left ? tip.Right_Half_Width() : tip.Left_Half_Width();
				const float inner_paved = inner_left ? Paved_Left(*runtime,tip) : Paved_Right(*runtime,tip);
				const float outer_paved = inner_left ? Paved_Right(*runtime,tip) : Paved_Left(*runtime,tip);
				if ((WWMath::Fabs((c.DAux - inner_paved) - c.HHwy) > edge_tolerance) ||
					 (WWMath::Fabs(inner_paved - inner_carriage) > edge_tolerance) ||
					 (WWMath::Fabs((c.DAux + outer_paved) - c.EAux) > edge_tolerance) ||
					 (WWMath::Fabs((outer_paved - outer_carriage) - c.HighwayShoulder) > edge_tolerance)) {
					detail.Format("interchange %u: ramp %u's gore cross section is inner %.3f / %.3f, outer %.3f / %.3f",ic.StableId,
									  ramp->StableId,c.DAux - inner_carriage,c.DAux - inner_paved,c.DAux + outer_carriage,c.DAux + outer_paved);
					return Realizer_Fail(error_out,detail.Peek_Buffer());
				}
			}

			//	Every ramp carries the capabilities the interchange needs.
			if ((plan.Roads[road_index].Capabilities & ramp->Capabilities) != ramp->Capabilities) {
				detail.Format("interchange %u: ramp %u does not carry its navigation capabilities",ic.StableId,ramp->StableId);
				return Realizer_Fail(error_out,detail.Peek_Buffer());
			}

			//	No accidental junction: every RoadSystem junction the ramp is an arm of is its local junction.
			for (int j = 0; j < RoadSystem::Get_Junction_Count(); ++j) {
				const RoadJunctionClass * one = RoadSystem::Peek_Junction(j);
				if ((one == nullptr) || !one->Has_Arm(piece->RoadId)) continue;
				if ((Vector2(one->Position.X,one->Position.Y) - local->Position).Length() <= k.W) continue;
				detail.Format("interchange %u: ramp %u meets another road at (%.2f,%.2f)",ic.StableId,ramp->StableId,one->Position.X,
								  one->Position.Y);
				return Realizer_Fail(error_out,detail.Peek_Buffer());
			}

			//	The merge/diverge record: its interval and subdivisions, and every sample by the Q-026 section.
			const HybridMergeDiverge * md = nullptr;
			for (int m = 0; (m < in.Interactions->MergeDiverge.Count()) && (md == nullptr); ++m) {
				if (in.Interactions->MergeDiverge[m].RampEdgeId == ramp->StableId) md = &in.Interactions->MergeDiverge[m];
			}
			const bool on = (ramp->RampRole == HYBRID_RAMP_ROLE_ON);
			const float span = on ? c.MergeLength : c.DivergeLength;
			const float full = on ? c.MergeFullLength : c.DivergeFullLength;
			if ((md == nullptr) || (md->Samples.Count() < 2) ||
				 (WWMath::Fabs((md->Interval.To - md->Interval.From) - span) > edge_tolerance) ||
				 (WWMath::Fabs((md->Interval.FullTo - md->Interval.FullFrom) - full) > edge_tolerance) ||
				 (WWMath::Fabs((md->Interval.TaperTo - md->Interval.TaperFrom) - c.TaperLength) > edge_tolerance)) {
				detail.Format("interchange %u: ramp %u's merge/diverge interval is not %.3f = %.3f full + %.3f taper",ic.StableId,
								  ramp->StableId,span,full,c.TaperLength);
				return Realizer_Fail(error_out,detail.Peek_Buffer());
			}
			for (int s = 0; s < md->Samples.Count(); ++s) {
				const HybridMergeDivergeSample & sample = md->Samples[s];
				const Vector2 point = anchor->Position + (ic.Axis * sample.Station);
				const Vector2 offset = point - anchor->Position;
				const float x = (offset.X * tau.X) + (offset.Y * tau.Y);
				const float along = on ? (x - ic.GoreOffset) : (x + ic.GoreOffset + c.DivergeLength);
				HybridAuxiliaryLaneSection section;
				Hybrid_Auxiliary_Lane_Section(c,ramp->RampRole,WWMath::Clamp(along,0.0f,span),&section);
				if ((WWMath::Fabs(sample.E - section.E) > edge_tolerance) || (WWMath::Fabs(sample.H - section.H) > edge_tolerance) ||
					 (WWMath::Fabs(sample.D - section.D) > edge_tolerance) ||
					 (sample.Full && ((WWMath::Fabs(sample.D - c.DAux) > edge_tolerance) || (WWMath::Fabs(sample.E - c.EAux) > edge_tolerance)))) {
					detail.Format("interchange %u: ramp %u's merge/diverge sample at chain %.3f is d %.3f E %.3f, the section %.3f / %.3f",
									  ic.StableId,ramp->StableId,sample.Station,sample.D,sample.E,section.D,section.E);
					return Realizer_Fail(error_out,detail.Peek_Buffer());
				}
			}

			//	The runtime highway's ramp side over the interval: carriageway H and roadbed E as the record, continuous.
			const float chain_lo = WWMath::Min(md->Samples[0].Station,md->Samples[md->Samples.Count() - 1].Station);
			const float chain_hi = WWMath::Max(md->Samples[0].Station,md->Samples[md->Samples.Count() - 1].Station);
			int checked = 0;
			for (int q = 0; q < pieces.Count(); ++q) {
				if (pieces[q].RoadIndex != highway) continue;
				const RoadSplineClass * road = RoadSystem::Peek_Road(pieces[q].RoadId);
				if (road == nullptr) continue;
				for (int p = 0; p < road->Get_Point_Count(); ++p) {
					const RoadPointClass & point = road->Get_Point(p);
					const Vector2 offset(point.Position.X - anchor->Position.X,point.Position.Y - anchor->Position.Y);
					const float chain = (offset.X * ic.Axis.X) + (offset.Y * ic.Axis.Y);
					if ((chain < chain_lo) || (chain > chain_hi)) continue;
					float widened = md->Samples[0].E;
					float carriage = md->Samples[0].H;
					for (int s = 1; s < md->Samples.Count(); ++s) {
						const HybridMergeDivergeSample & a = md->Samples[s - 1];
						const HybridMergeDivergeSample & b = md->Samples[s];
						if ((chain < WWMath::Min(a.Station,b.Station)) || (chain > WWMath::Max(a.Station,b.Station))) continue;
						const float t = (b.Station != a.Station) ? WWMath::Clamp((chain - a.Station) / (b.Station - a.Station),0.0f,1.0f) : 0.0f;
						widened	= a.E + ((b.E - a.E) * t);
						carriage	= a.H + ((b.H - a.H) * t);
						break;
					}
					const bool ramp_left = (((point.Side.X * normal.X) + (point.Side.Y * normal.Y)) > 0.0f);
					const float runtime_e = ramp_left ? Paved_Left(*road,point) : Paved_Right(*road,point);
					const float runtime_h = ramp_left ? point.Left_Half_Width() : point.Right_Half_Width();
					checked++;
					if ((WWMath::Fabs(runtime_e - widened) > edge_tolerance) || (WWMath::Fabs(runtime_h - carriage) > edge_tolerance)) {
						detail.Format("interchange %u: the highway's ramp side at chain %.3f is h %.3f E %.3f; ramp %u's record says "
										  "%.3f / %.3f",ic.StableId,chain,runtime_h,runtime_e,ramp->StableId,carriage,widened);
						return Realizer_Fail(error_out,detail.Peek_Buffer());
					}
				}
			}
			if (checked < 2) {
				detail.Format("interchange %u: no runtime highway carries ramp %u's parallel interval",ic.StableId,ramp->StableId);
				return Realizer_Fail(error_out,detail.Peek_Buffer());
			}
		}
	}
	return true;
}


bool HybridRoadRealizer::Verify_Physical(const HybridRoadRealizerInputs & in,StringClass * error_out)
{
	if ((in.Plan == nullptr) || (in.Topology == nullptr) || (in.Routes == nullptr) || (in.Interactions == nullptr) ||
		 (Pieces.Count() == 0 && in.Plan->Roads.Count() > 0)) {
		return Realizer_Fail(error_out,"Phase 12's physical verification needs its realized plan");
	}
	if (!(in.Light.Width > 0.0f) || !(in.Light.Length > 0.0f)) {
		return Realizer_Fail(error_out,"Phase 12's physical verification has no reference vehicle envelope");
	}
	const HybridConstants & k = in.Constants;
	const HeightfieldClass * field = WorldTerrainSystem::Peek_Heightfield();
	const float cell = (field != nullptr) ? field->Get_Cell_Size() : k.W;

	PavedIndex paved;
	Build_Paved_Index(Pieces,4.0f * k.W,&paved);
	DynamicVectorClass<PierBox> piers;
	Collect_Piers(&piers);

	SweepContext x;
	x.In						= &in;
	x.Paved					= &paved;
	x.Pieces					= &Pieces;
	x.Piers					= &piers;
	x.Spacing				= Hybrid_Profile_Sample_Spacing(k);
	x.Cell					= cell;
	Swept = 0;
	for (int r = 0; r < in.Plan->Roads.Count(); ++r) {
		const HybridRealizedRoad & road = in.Plan->Roads[r];
		if (!Sweep_Road(x,r,in.Light,"light",error_out)) return false;
		if (((road.Capabilities & HYBRID_NAV_HEAVY) != 0) && !Sweep_Road(x,r,in.Heavy,"heavy",error_out)) return false;
		if (((road.Capabilities & HYBRID_NAV_HARVESTER) != 0) && !Sweep_Road(x,r,in.Harvester,"Harvester",error_out)) return false;
	}
	SweptSamples = Swept;

	return Verify_Road_Surfaces(x,error_out) && Verify_Bridges(in,BridgeIds,Pieces,cell,error_out) &&
			 Verify_Interchanges(in,Pieces,cell,error_out);
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
