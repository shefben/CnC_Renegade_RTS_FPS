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
 *                     $Archive:: /Commando/Code/Combat/tunnelplanner.cpp                     $*
 *                                                                                             *
 *	V2 roadmap Section 34 Phase 10.  See tunnelplanner.h and docs/procedural_world/             *
 *	RoadsBridgesTunnels.md.                                                                     *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "tunnelplanner.h"

#include "bridgeplanner.h"
#include "crc.h"
#include "escarpment.h"
#include "interiorcliffgenerator.h"
#include "roadgraphgenerator.h"
#include "roadspline.h"
#include "roadsystem.h"
#include "strategicplanner.h"
#include "terrainmask.h"
#include "watersystem.h"
#include "worldterrainsystem.h"
#include "wwdebug.h"
#include "wwmath.h"


bool											TunnelPlanner::Built		= false;
unsigned int								TunnelPlanner::Checksum	= 0;
DynamicVectorClass<ResolvedTunnel>			TunnelPlanner::Tunnels;
DynamicVectorClass<TunnelResolutionRecord>	TunnelPlanner::Diagnostics;


const char * Tunnel_Source_Name(TunnelCandidateSourceType source)
{
	switch (source) {
		case TUNNEL_SOURCE_CLIFF_BYPASS:				return "cliff_bypass";
		case TUNNEL_SOURCE_UNRESOLVED_ROAD_EDGE:	return "unresolved_road_edge";
		case TUNNEL_SOURCE_INFANTRY_FLANK:			return "infantry_flank";
		case TUNNEL_SOURCE_SETTLEMENT_UNDERPASS:	return "settlement_underpass";
		case TUNNEL_SOURCE_STRATEGIC_ALTERNATE:	return "strategic_alternate";
		default:												return "unknown";
	}
}


const char * Tunnel_Failure_Reason_Name(TunnelFailureReasonType reason)
{
	switch (reason) {
		case TUNNEL_FAILURE_NONE:					return "none";
		case TUNNEL_FAILURE_NO_CLEARANCE_DATA:	return "no_clearance_data";
		case TUNNEL_FAILURE_NO_OPEN_GROUND:		return "no_open_ground";
		case TUNNEL_FAILURE_TOO_LONG:				return "too_long";
		case TUNNEL_FAILURE_GRADE_TOO_HIGH:		return "grade_too_high";
		case TUNNEL_FAILURE_NO_OVERBURDEN:		return "no_overburden";
		case TUNNEL_FAILURE_NOT_CONTINUOUS:		return "not_continuous";
		case TUNNEL_FAILURE_BOUNDARY:				return "boundary";
		case TUNNEL_FAILURE_BASE_ZONE:			return "base_zone";
		case TUNNEL_FAILURE_MAJOR_WATER:			return "major_water";
		case TUNNEL_FAILURE_COLLISION:			return "collision";
		default:											return "unknown";
	}
}


/***********************************************************************************************
 * Data classes                                                                                *
 *=============================================================================================*/
TunnelCandidate::TunnelCandidate(void) :
	Source(TUNNEL_SOURCE_INFANTRY_FLANK), Class(TUNNEL_CLASS_INFANTRY_ONLY),
	Position(0.0f,0.0f,0.0f), Axis(1.0f,0.0f,0.0f), RoadId(-1), RoadWidth(0.0f)
{
}


bool TunnelCandidate::operator == (const TunnelCandidate & that) const
{
	return (Id == that.Id) && (Source == that.Source) && (Class == that.Class) &&
			 (Position == that.Position) && (Axis == that.Axis) && (RoadId == that.RoadId) &&
			 (RoadEdgeId == that.RoadEdgeId) && (RoadWidth == that.RoadWidth);
}


ResolvedTunnel::ResolvedTunnel(void) :
	TunnelId(-1), Class(TUNNEL_CLASS_INFANTRY_ONLY), Source(TUNNEL_SOURCE_INFANTRY_FLANK),
	RoadSplit(false), PortalA(0.0f,0.0f,0.0f), PortalB(0.0f,0.0f,0.0f),
	NavLinked(false), EntranceChunk(-1), InteriorSector(-1), ExitChunk(-1)
{
}


bool ResolvedTunnel::operator == (const ResolvedTunnel & that) const
{
	return (Id == that.Id) && (TunnelId == that.TunnelId) && (CandidateId == that.CandidateId) &&
			 (RoadEdgeId == that.RoadEdgeId) && (Class == that.Class) && (Source == that.Source) &&
			 (RoadSplit == that.RoadSplit) && (PortalA == that.PortalA) && (PortalB == that.PortalB) &&
			 (NavLinked == that.NavLinked) && (EntranceChunk == that.EntranceChunk) &&
			 (InteriorSector == that.InteriorSector) && (ExitChunk == that.ExitChunk);
}


TunnelResolutionRecord::TunnelResolutionRecord(void) :
	Source(TUNNEL_SOURCE_INFANTRY_FLANK), Resolved(false), AttemptsMade(0), Displacement(0.0f),
	FinalFailureReason(TUNNEL_FAILURE_NONE)
{
}


bool TunnelResolutionRecord::operator == (const TunnelResolutionRecord & that) const
{
	return (CandidateId == that.CandidateId) && (Source == that.Source) && (Resolved == that.Resolved) &&
			 (AttemptsMade == that.AttemptsMade) && (Displacement == that.Displacement) &&
			 (FinalFailureReason == that.FinalFailureReason);
}


TunnelPlannerParams::TunnelPlannerParams(void) :
	Seed(1),
	MaxBoreLength(BridgePlannerParams().MaxSpanLength),
	MinCover(1.0f),
	WallThickness(0.5f),
	BaseProtectedRadius(0.0f),
	MaxRepositionAttempts(4),
	RepositionStep(8.0f)
{
}


/***********************************************************************************************
 * Lifecycle                                                                                   *
 *=============================================================================================*/
void TunnelPlanner::Init(void)
{
	Clear();
}


void TunnelPlanner::Shutdown(void)
{
	Clear();
}


void TunnelPlanner::Clear(void)
{
	//	TunnelSystem's collection is this generator's alone (unlike roads/bridges, nothing else adds
	//	tunnels), so dropping it wholesale is dropping only what this generator made.
	TunnelSystem::Clear_Tunnels();
	Tunnels.Delete_All();
	Diagnostics.Delete_All();
	Built = false;
	Checksum = 0;
}


bool TunnelPlanner::Get_Tunnel(int index,ResolvedTunnel * out)
{
	if ((out == nullptr) || (index < 0) || (index >= Tunnels.Count())) {
		return false;
	}
	*out = Tunnels[index];
	return true;
}


bool TunnelPlanner::Set_Nav_Links(int index,int entrance,int interior,int exit)
{
	if ((index < 0) || (index >= Tunnels.Count())) {
		return false;
	}
	Tunnels[index].NavLinked		= (entrance >= 0) && (exit >= 0);
	Tunnels[index].EntranceChunk	= entrance;
	Tunnels[index].InteriorSector	= interior;
	Tunnels[index].ExitChunk		= exit;
	return true;
}


bool TunnelPlanner::Get_Diagnostic(int index,TunnelResolutionRecord * out)
{
	if ((out == nullptr) || (index < 0) || (index >= Diagnostics.Count())) {
		return false;
	}
	*out = Diagnostics[index];
	return true;
}


bool TunnelPlanner::Is_Candidate_Resolved(const StringClass & candidate_id)
{
	for (int i = 0; i < Tunnels.Count(); i++) {
		if (Tunnels[i].CandidateId == candidate_id) {
			return true;
		}
	}
	return false;
}


/***********************************************************************************************
 * Gather_Candidates -- step 1                                                                 *
 *                                                                                             *
 *	Two sources are real here.  An unresolved road crossing -- BridgePlanner's own diagnostic      *
 *	that it could not bridge it -- is this phase's "alternate crossing type".  A TUNNEL_CANDIDATE    *
 *	node that is not on any road edge's crossing list is an infantry flank through the cliff; one     *
 *	that is on a road edge and is still unresolved arrives through the first source.  Settlement     *
 *	underpasses and strategic alternate routes produce nothing: the road graph has no grade-          *
 *	separated crossing to put an underpass under, and no later phase feeds a route request back       *
 *	before this one runs.  Both are named, counted as zero, and documented rather than invented.       *
 *=============================================================================================*/
static StrategicNode * Find_Node(const StringClass & id,StrategicNode * storage)
{
	for (int n = 0; n < StrategicPlanner::Get_Node_Count(); n++) {
		if (!StrategicPlanner::Get_Node(n,storage)) continue;
		if (storage->Id == id) return storage;
	}
	return nullptr;
}


static bool Node_On_Any_Road_Edge(const StringClass & id)
{
	for (int e = 0; e < RoadGraphGenerator::Get_Edge_Count(); e++) {
		RoadGraphEdge edge;
		if (!RoadGraphGenerator::Get_Edge(e,&edge)) continue;
		for (int c = 0; c < edge.UnresolvedCrossingIds.Count(); c++) {
			if (edge.UnresolvedCrossingIds[c] == id) return true;
		}
	}
	return false;
}


//	The plan direction straight through an interior cliff at `position`: across the face of the
//	escarpment that owns the feature there.
static bool Cliff_Crossing_Axis(const Vector3 & position,Vector3 * axis_out,InteriorCliffType * type_out)
{
	for (int i = 0; i < InteriorCliffGenerator::Get_Feature_Count(); i++) {
		InteriorCliffFeature f;
		if (!InteriorCliffGenerator::Get_Feature(i,&f) || !f.HasBypassPoint) continue;
		float dx = f.BypassPoint.X - position.X;
		float dy = f.BypassPoint.Y - position.Y;
		if (((dx * dx) + (dy * dy)) > 0.01f) continue;

		EscarpmentClass * esc = EscarpmentSystem::Peek_Escarpment(f.EscarpmentId);
		if ((esc == nullptr) || (esc->Get_Point_Count() < 2)) return false;

		int best = 1;
		float best_d = 1.0e30f;
		for (int p = 1; p < esc->Get_Point_Count(); p++) {
			Vector3 mid = (esc->Get_Point(p - 1).Position + esc->Get_Point(p).Position) * 0.5f;
			float d = ((mid.X - position.X) * (mid.X - position.X)) + ((mid.Y - position.Y) * (mid.Y - position.Y));
			if (d < best_d) { best_d = d; best = p; }
		}
		Vector3 dir = esc->Get_Point(best).Position - esc->Get_Point(best - 1).Position;
		dir.Z = 0.0f;
		if (dir.Length() <= WWMATH_EPSILON) return false;
		dir.Normalize();
		axis_out->Set(dir.Y,-dir.X,0.0f);		// right of travel is across the face, downhill
		if (type_out != nullptr) *type_out = f.Type;
		return true;
	}
	return false;
}


int TunnelPlanner::Gather_Candidates(const TunnelPlannerParams & params,DynamicVectorClass<TunnelCandidate> & out)
{
	int added = 0;

	//	Road crossings BridgePlanner could not resolve.
	for (int d = 0; d < BridgePlanner::Get_Diagnostic_Count(); d++) {
		CrossingResolutionRecord rec;
		if (!BridgePlanner::Get_Diagnostic(d,&rec) || rec.Resolved) continue;

		StrategicNode storage;
		StrategicNode * node = Find_Node(rec.CrossingNodeId,&storage);
		if (node == nullptr) continue;

		RoadNavInfoClass info;
		if (!RoadSystem::Find_Nearest_Road(node->Position,64.0f,&info) || (info.RoadID < 0)) continue;
		RoadSplineClass * road = RoadSystem::Peek_Road(info.RoadID);
		if (road == nullptr) continue;

		RoadGraphClassType road_class = ROAD_GRAPH_PRIMARY;
		for (int e = 0; e < RoadGraphGenerator::Get_Edge_Count(); e++) {
			RoadGraphEdge edge;
			if (RoadGraphGenerator::Get_Edge(e,&edge) && (edge.Id == rec.RoadEdgeId)) { road_class = edge.Class; break; }
		}

		TunnelCandidate c;
		c.Id			= rec.CrossingNodeId;
		c.Source		= (node->Type == STRATEGIC_NODE_TUNNEL_CANDIDATE) ? TUNNEL_SOURCE_CLIFF_BYPASS : TUNNEL_SOURCE_UNRESOLVED_ROAD_EDGE;
		//	Section 12A.1: a road a person might also walk -- a street, a service road, a track --
		//	gets the mixed envelope; highways and primary/secondary roads the vehicle one.
		c.Class		= ((road_class == ROAD_GRAPH_CITY_STREET) || (road_class == ROAD_GRAPH_SERVICE) ||
							(road_class == ROAD_GRAPH_DIRT_TRACK)) ? TUNNEL_CLASS_MIXED : TUNNEL_CLASS_VEHICLE;
		c.Position	= node->Position;
		c.Axis		= Vector3(info.Tangent.X,info.Tangent.Y,0.0f);
		if (c.Axis.Length() <= WWMATH_EPSILON) continue;
		c.Axis.Normalize();
		c.RoadId		= info.RoadID;
		c.RoadEdgeId	= rec.RoadEdgeId;
		c.RoadWidth	= road->Get_Width();
		out.Add(c);
		added++;
	}

	//	Tunnel candidates no road passes through: flanks and caves.
	for (int n = 0; n < StrategicPlanner::Get_Node_Count(); n++) {
		StrategicNode node;
		if (!StrategicPlanner::Get_Node(n,&node)) continue;
		if (node.Type != STRATEGIC_NODE_TUNNEL_CANDIDATE) continue;
		if (Node_On_Any_Road_Edge(node.Id)) continue;

		Vector3 axis;
		InteriorCliffType type = INTERIOR_CLIFF_ESCARPMENT;
		if (!Cliff_Crossing_Axis(node.Position,&axis,&type)) continue;

		TunnelCandidate c;
		c.Id			= node.Id;
		c.Source		= TUNNEL_SOURCE_INFANTRY_FLANK;
		//	Natural rock -- a mountain face or a canyon wall -- is a cave; anything cut by hand is a
		//	lined infantry tunnel.  Both carry the infantry envelope.
		c.Class		= ((type == INTERIOR_CLIFF_MOUNTAIN_FACE) || (type == INTERIOR_CLIFF_CANYON_WALL))
							? TUNNEL_CLASS_CAVE_SHORTCUT : TUNNEL_CLASS_INFANTRY_ONLY;
		c.Position	= node.Position;
		c.Axis		= axis;
		out.Add(c);
		added++;
	}

	(void)params;
	return added;
}


/***********************************************************************************************
 * Plan_Bore -- steps 2-6 and 13 for one candidate                                              *
 *                                                                                             *
 *	Walk outward from the barrier both ways along the axis until the ground has been passable      *
 *	for the class (slope within its grade) for the class's own minimum straight-at-portal run:     *
 *	that is open ground, and the bore's two ends.  If the grade between them is too steep, the       *
 *	higher end walks further out -- a bore through a step climbs inside the rock, and the long       *
 *	ramp belongs on the high side, where the floor stays under the ground.  Then the bore must        *
 *	actually go under something (overburden), must not float over a dip, must stay inside the          *
 *	world and clear of protected base zones, water and other bores.                                     *
 *=============================================================================================*/
static bool Sample(const Vector3 & origin,const Vector3 & axis,float s,float * h_out,float * slope_out)
{
	float x = origin.X + (axis.X * s);
	float y = origin.Y + (axis.Y * s);
	if (!WorldTerrainSystem::Sample_Height(x,y,h_out)) return false;
	if ((slope_out != nullptr) && !WorldTerrainSystem::Sample_Slope(x,y,slope_out)) return false;
	return true;
}


static float Point_Segment_Distance_2D(const Vector3 & p,const Vector3 & a,const Vector3 & b)
{
	float dx = b.X - a.X, dy = b.Y - a.Y;
	float len2 = (dx * dx) + (dy * dy);
	float t = (len2 > WWMATH_EPSILON) ? ((((p.X - a.X) * dx) + ((p.Y - a.Y) * dy)) / len2) : 0.0f;
	t = WWMath::Clamp(t,0.0f,1.0f);
	float cx = a.X + (dx * t) - p.X;
	float cy = a.Y + (dy * t) - p.Y;
	return WWMath::Sqrt((cx * cx) + (cy * cy));
}


static float Segment_Distance_2D(const Vector3 & a0,const Vector3 & a1,const Vector3 & b0,const Vector3 & b1)
{
	float d1x = a1.X - a0.X, d1y = a1.Y - a0.Y;
	float d2x = b1.X - b0.X, d2y = b1.Y - b0.Y;
	float den = (d1x * d2y) - (d1y * d2x);
	if (WWMath::Fabs(den) > WWMATH_EPSILON) {
		float ox = b0.X - a0.X, oy = b0.Y - a0.Y;
		float u = ((ox * d2y) - (oy * d2x)) / den;
		float v = ((ox * d1y) - (oy * d1x)) / den;
		if ((u >= 0.0f) && (u <= 1.0f) && (v >= 0.0f) && (v <= 1.0f)) return 0.0f;
	}
	float d = Point_Segment_Distance_2D(a0,b0,b1);
	d = WWMath::Min(d,Point_Segment_Distance_2D(a1,b0,b1));
	d = WWMath::Min(d,Point_Segment_Distance_2D(b0,a0,a1));
	d = WWMath::Min(d,Point_Segment_Distance_2D(b1,a0,a1));
	return d;
}


bool TunnelPlanner::Plan_Bore(const TunnelCandidate & candidate,const TunnelPlannerParams & params,
										TunnelClass * tunnel_out,TunnelFailureReasonType * reason_out)
{
	#define REFUSE(r) { if (reason_out != nullptr) *reason_out = (r); return false; }

	if (reason_out != nullptr) *reason_out = TUNNEL_FAILURE_NONE;

	TunnelClearanceClass clr = params.ClassClearance[candidate.Class];
	if (candidate.Class == TUNNEL_CLASS_UNDERPASS) {
		clr = params.ClassClearance[TUNNEL_CLASS_VEHICLE];
	}
	if ((candidate.RoadWidth > 0.0f) && Tunnel_Class_Carries_Vehicles(candidate.Class)) {
		//	Section 12A.3's road cross-section handoff: the bore is never narrower than the road.
		clr.InternalWidth = WWMath::Max(clr.InternalWidth,candidate.RoadWidth);
	}
	if (!clr.Is_Valid()) REFUSE(TUNNEL_FAILURE_NO_CLEARANCE_DATA);

	if (WaterSystem::Is_Point_In_Water(candidate.Position.X,candidate.Position.Y)) REFUSE(TUNNEL_FAILURE_MAJOR_WATER);

	HeightfieldClass * field = WorldTerrainSystem::Peek_Heightfield();
	if (field == nullptr) REFUSE(TUNNEL_FAILURE_NO_OPEN_GROUND);

	Vector3 axis(candidate.Axis.X,candidate.Axis.Y,0.0f);
	if (axis.Length() <= WWMATH_EPSILON) REFUSE(TUNNEL_FAILURE_NO_OPEN_GROUND);
	axis.Normalize();

	float step = WWMath::Max(0.25f,field->Get_Cell_Size() * 0.5f);
	float half_reach = params.MaxBoreLength * 0.5f;
	float straight = (clr.MinPortalStraight > 0.0f) ? clr.MinPortalStraight : (2.0f * step);

	/*
	**	Open ground each way: the first station starting a run of passable samples `straight`
	**	long that also lies a full roof clearance below the highest ground crossed so far -- so a
	**	flat ridge crest is never mistaken for a portal.  A side that never drops that far (the
	**	high bench of a step) falls back to its first passable run, and the grade pass below
	**	lengthens the ramp into it.
	*/
	float roof_estimate = clr.Get_Bore_Height() + params.WallThickness + params.MinCover;
	float h_p;
	if (!Sample(candidate.Position,axis,0.0f,&h_p,nullptr)) REFUSE(TUNNEL_FAILURE_NO_OPEN_GROUND);

	float end_s[2] = { 0.0f, 0.0f };
	for (int side = 0; side < 2; side++) {
		float sign = (side == 0) ? -1.0f : 1.0f;
		float max_seen = h_p;
		float run_below = -1.0f, run_any = -1.0f;
		float found_below = -1.0f, found_any = -1.0f;
		for (float s = step; s <= half_reach; s += step) {
			float h,slope;
			if (!Sample(candidate.Position,axis,sign * s,&h,&slope)) break;
			max_seen = WWMath::Max(max_seen,h);
			bool passable = (slope <= clr.MaxGradeRadians);
			bool below = passable && (h <= (max_seen - roof_estimate));
			if (below) {
				if (run_below < 0.0f) run_below = s;
				if ((s - run_below) >= straight) { found_below = run_below; break; }
			} else {
				run_below = -1.0f;
			}
			if (passable) {
				if (run_any < 0.0f) run_any = s;
				if ((found_any < 0.0f) && ((s - run_any) >= straight)) found_any = run_any;
			} else {
				run_any = -1.0f;
			}
		}
		float found = (found_below >= 0.0f) ? found_below : found_any;
		if (found < 0.0f) REFUSE(TUNNEL_FAILURE_NO_OPEN_GROUND);
		end_s[side] = sign * found;
	}

	float h_a,h_b;
	if (!Sample(candidate.Position,axis,end_s[0],&h_a,nullptr) ||
		 !Sample(candidate.Position,axis,end_s[1],&h_b,nullptr)) REFUSE(TUNNEL_FAILURE_NO_OPEN_GROUND);

	//	Grade: lengthen on the high side until the ramp fits, bounded by the bore length.
	float tan_grade = tanf(clr.MaxGradeRadians);
	for (int guard = 0; guard < 4096; guard++) {
		float plan = end_s[1] - end_s[0];
		if (plan > params.MaxBoreLength) REFUSE(TUNNEL_FAILURE_TOO_LONG);
		if (WWMath::Fabs(h_b - h_a) <= (plan * tan_grade)) break;
		bool extend_b = (h_b > h_a);
		float s = extend_b ? (end_s[1] + step) : (end_s[0] - step);
		float h;
		if (!Sample(candidate.Position,axis,s,&h,nullptr)) REFUSE(TUNNEL_FAILURE_GRADE_TOO_HIGH);
		if (extend_b) { end_s[1] = s; h_b = h; } else { end_s[0] = s; h_a = h; }
	}
	float plan = end_s[1] - end_s[0];
	if (WWMath::Fabs(h_b - h_a) > (plan * tan_grade)) REFUSE(TUNNEL_FAILURE_GRADE_TOO_HIGH);

	TunnelClass t;
	t.Name.Format("tunnel_%s",candidate.Id.Peek_Buffer());
	t.Class				= candidate.Class;
	t.ModuleName		= (candidate.Class == TUNNEL_CLASS_CAVE_SHORTCUT) ? "cave_rock" : "tunnel_lined";
	t.Start				= Vector3(candidate.Position.X + axis.X * end_s[0],candidate.Position.Y + axis.Y * end_s[0],h_a);
	t.End					= Vector3(candidate.Position.X + axis.X * end_s[1],candidate.Position.Y + axis.Y * end_s[1],h_b);
	t.Clearance			= clr;
	t.WallThickness	= params.WallThickness;
	t.MinCover			= params.MinCover;

	//	Overburden and continuity, along the bore itself.
	float roof_clear = t.Get_Outer_Height() + t.MinCover;
	float best_cover = -1.0e30f;
	for (float s = 0.0f; s <= plan; s += step) {
		float h;
		if (!Sample(t.Start,axis,s,&h,nullptr)) REFUSE(TUNNEL_FAILURE_NOT_CONTINUOUS);
		float floor = t.Get_Floor_Height_At(s);
		if (h < (floor - (t.Clearance.Get_Bore_Height() * 0.5f))) REFUSE(TUNNEL_FAILURE_NOT_CONTINUOUS);
		best_cover = WWMath::Max(best_cover,h - (floor + t.Get_Outer_Height()));

		//	Step 2: water anywhere the bore is open to the sky -- the approaches and the mouths.
		if (h < (floor + roof_clear)) {
			if (WaterSystem::Is_Point_In_Water(t.Start.X + axis.X * s,t.Start.Y + axis.Y * s)) {
				REFUSE(TUNNEL_FAILURE_MAJOR_WATER);
			}
		}
	}
	if (best_cover < t.MinCover) REFUSE(TUNNEL_FAILURE_NO_OVERBURDEN);

	if (TunnelSystem::Breaches_Boundary(t)) REFUSE(TUNNEL_FAILURE_BOUNDARY);

	//	Step 2: protected base zones, measured against the base candidates -- base selection runs
	//	after this phase and chooses among exactly those nodes, with the same footprint radius.
	float reach = params.BaseProtectedRadius + t.Get_Outer_Half_Width();
	if (params.BaseProtectedRadius > 0.0f) {
		for (int n = 0; n < StrategicPlanner::Get_Node_Count(); n++) {
			StrategicNode node;
			if (!StrategicPlanner::Get_Node(n,&node)) continue;
			if (node.Type != STRATEGIC_NODE_BASE_CANDIDATE) continue;
			if (Point_Segment_Distance_2D(node.Position,t.Start,t.End) < reach) REFUSE(TUNNEL_FAILURE_BASE_ZONE);
		}
	}

	//	Another bore.
	for (int i = 0; i < TunnelSystem::Get_Tunnel_Count(); i++) {
		const TunnelClass * other = TunnelSystem::Peek_Tunnel(i);
		if (other == nullptr) continue;
		float gap = other->Get_Outer_Half_Width() + t.Get_Outer_Half_Width() + t.WallThickness;
		if (Segment_Distance_2D(t.Start,t.End,other->Start,other->End) < gap) REFUSE(TUNNEL_FAILURE_COLLISION);
	}

	if (tunnel_out != nullptr) *tunnel_out = t;
	return true;

	#undef REFUSE
}


/***********************************************************************************************
 * Resolve_Candidate -- bounded retries, realize, step 14's road split                         *
 *                                                                                             *
 *	A road tunnel is tried once: it must stay on its road, and moving it sideways would move it   *
 *	off.  A flank is tried at deterministic sideways offsets along the cliff -- 0, then +/-1,       *
 *	+/-2 steps, the sign order drawn from the seed -- bounded by MaxRepositionAttempts.              *
 *=============================================================================================*/
bool TunnelPlanner::Resolve_Candidate(const TunnelCandidate & candidate,const TunnelPlannerParams & params)
{
	TunnelResolutionRecord record;
	record.CandidateId	= candidate.Id;
	record.Source			= candidate.Source;

	int max_attempts = (candidate.RoadId >= 0) ? 1 : ((params.MaxRepositionAttempts > 1) ? params.MaxRepositionAttempts : 1);
	float first_sign = (TerrainMaskClass::Hash_Value(Diagnostics.Count(),0,(int)params.Seed) < 0.5f) ? 1.0f : -1.0f;
	Vector3 lateral(-candidate.Axis.Y,candidate.Axis.X,0.0f);

	TunnelClass planned;
	bool ok = false;
	float displacement = 0.0f;
	TunnelFailureReasonType reason = TUNNEL_FAILURE_NONE;

	for (int attempt = 0; (attempt < max_attempts) && !ok; attempt++) {
		int k = (attempt + 1) / 2;
		float sign = ((attempt % 2) == 1) ? first_sign : -first_sign;
		displacement = (attempt == 0) ? 0.0f : (sign * k * params.RepositionStep);

		TunnelCandidate shifted = candidate;
		shifted.Position = candidate.Position + (lateral * displacement);
		record.AttemptsMade++;
		ok = Plan_Bore(shifted,params,&planned,&reason);
	}

	record.Resolved				= ok;
	record.Displacement			= ok ? displacement : 0.0f;
	record.FinalFailureReason	= ok ? TUNNEL_FAILURE_NONE : reason;
	Diagnostics.Add(record);

	if (!ok) {
		WWDEBUG_SAY(("TunnelPlanner: candidate '%s' (%s, %s) unresolved after %d attempt(s) -- %s\r\n",
						 candidate.Id.Peek_Buffer(),Tunnel_Source_Name(candidate.Source),
						 Tunnel_Class_Name(candidate.Class),record.AttemptsMade,Tunnel_Failure_Reason_Name(reason)));
		return false;
	}

	int tunnel_id = TunnelSystem::Add_Tunnel(planned);

	ResolvedTunnel resolved;
	resolved.Id				= planned.Name;
	resolved.TunnelId		= tunnel_id;
	resolved.CandidateId	= candidate.Id;
	resolved.Class			= candidate.Class;
	resolved.Source		= candidate.Source;
	resolved.PortalA		= planned.Start;
	resolved.PortalB		= planned.End;

	//	Step 14: a vehicle tunnel that resolves a pending road edge takes the road over between its
	//	portals -- the road is cut at both, and both new ends hand over to this tunnel.
	if (candidate.RoadId >= 0) {
		resolved.RoadEdgeId = candidate.RoadEdgeId;
		RoadSplineClass * road = RoadSystem::Peek_Road(candidate.RoadId);
		if (road != nullptr) {
			int index_a,index_b;
			float lateral_a,lateral_b,along_a,along_b;
			float tolerance = road->Get_Width() + planned.Get_Outer_Half_Width();
			if (road->Find_Closest_Point(planned.Start,&index_a,&lateral_a,&along_a) &&
				 road->Find_Closest_Point(planned.End,&index_b,&lateral_b,&along_b) &&
				 (WWMath::Fabs(lateral_a) <= tolerance) && (WWMath::Fabs(lateral_b) <= tolerance)) {

				float distance_a = WWMath::Min(along_a,along_b);
				float distance_b = WWMath::Max(along_a,along_b);
				int segment_a_id,segment_b_id;
				if (RoadSystem::Trim_Road_Range(candidate.RoadId,distance_a,distance_b,&segment_a_id,&segment_b_id)) {
					if (segment_a_id != -1) {
						RoadSplineClass * a = RoadSystem::Peek_Road(segment_a_id);
						if ((a != nullptr) && (a->Get_Point_Count() > 0)) {
							RoadConnectionClass & connection = a->Get_End_Connection();
							connection.Type		= ROAD_ENDPOINT_TUNNEL;
							connection.TargetID	= tunnel_id;
							connection.Position	= a->Get_Point(a->Get_Point_Count() - 1).Position;
							connection.Direction	= a->Get_Point(a->Get_Point_Count() - 1).Tangent;
						}
					}
					if (segment_b_id != -1) {
						RoadSplineClass * b = RoadSystem::Peek_Road(segment_b_id);
						if ((b != nullptr) && (b->Get_Point_Count() > 0)) {
							RoadConnectionClass & connection = b->Get_Start_Connection();
							connection.Type		= ROAD_ENDPOINT_TUNNEL;
							connection.TargetID	= tunnel_id;
							connection.Position	= b->Get_Point(0).Position;
							connection.Direction	= -b->Get_Point(0).Tangent;
						}
						RoadGraphGenerator::Adopt_Owned_Road_Id(segment_b_id);
					}
					resolved.RoadSplit = true;
				}
			}
		}
		if (!resolved.RoadSplit) {
			WWDEBUG_SAY(("TunnelPlanner: tunnel %d resolves '%s' but road %d could not be split at its portals\r\n",
							 tunnel_id,candidate.Id.Peek_Buffer(),candidate.RoadId));
		}
	}

	Tunnels.Add(resolved);
	return true;
}


/***********************************************************************************************
 * Generate                                                                                    *
 *=============================================================================================*/
bool TunnelPlanner::Generate(const TunnelPlannerParams & params,StringClass * error_out)
{
	Clear();

	if (!WorldTerrainSystem::Has_Chunk_Registry()) {
		if (error_out != nullptr) *error_out = "no chunk registry -- tunnels are planned against real terrain";
		return false;
	}

	DynamicVectorClass<TunnelCandidate> candidates;
	Gather_Candidates(params,candidates);

	for (int i = 0; i < candidates.Count(); i++) {
		Resolve_Candidate(candidates[i],params);
	}

	//	Realize: module chains, then the ground and cliff cuts, then geometry where a scene exists.
	TunnelSystem::Build_Layout();
	TunnelSystem::Apply_Portal_Cuts();
	TunnelSystem::Build_Geometry();

	{
		CRCEngine crc;
		int count = Tunnels.Count();
		crc(&count,sizeof(count));
		unsigned int system_checksum = TunnelSystem::Get_Checksum();
		crc(&system_checksum,sizeof(system_checksum));
		for (int i = 0; i < Diagnostics.Count(); i++) {
			int q[3] = { Diagnostics[i].Resolved ? 1 : 0,Diagnostics[i].AttemptsMade,(int)Diagnostics[i].FinalFailureReason };
			crc(q,sizeof(q));
		}
		Checksum = (unsigned int)crc;
	}

	Built = true;
	WWDEBUG_SAY(("TunnelPlanner: %d candidate(s), %d tunnel(s), %d cell(s) opened, checksum %08X\r\n",
					 candidates.Count(),Tunnels.Count(),TunnelSystem::Get_Opened_Cell_Count(),Checksum));
	return true;
}


/***********************************************************************************************
 * Checks                                                                                      *
 *=============================================================================================*/
int TunnelPlanner::Check_No_Boundary_Breach(StringClass * report)
{
	int failures = 0;
	for (int i = 0; i < TunnelSystem::Get_Tunnel_Count(); i++) {
		const TunnelClass * t = TunnelSystem::Peek_Tunnel(i);
		if ((t != nullptr) && TunnelSystem::Breaches_Boundary(*t)) {
			failures++;
			if (report != nullptr) {
				StringClass line;
				line.Format("tunnel %d '%s' breaches the world boundary\r\n",t->ID,t->Name.Peek_Buffer());
				*report += line;
			}
		}
	}
	return failures;
}


int TunnelPlanner::Check_Road_Tunnels_Bound(StringClass * report)
{
	int failures = 0;
	for (int i = 0; i < Tunnels.Count(); i++) {
		const ResolvedTunnel & r = Tunnels[i];
		if (r.RoadEdgeId.Get_Length() == 0) continue;

		int bound = 0;
		for (int road = 0; road < RoadSystem::Get_Road_Count(); road++) {
			RoadSplineClass * spline = RoadSystem::Peek_Road(road);
			if (spline == nullptr) continue;
			if ((spline->Get_Start_Connection().Type == ROAD_ENDPOINT_TUNNEL) && (spline->Get_Start_Connection().TargetID == r.TunnelId)) bound++;
			if ((spline->Get_End_Connection().Type == ROAD_ENDPOINT_TUNNEL) && (spline->Get_End_Connection().TargetID == r.TunnelId)) bound++;
		}
		if (!r.RoadSplit || (bound < 1)) {
			failures++;
			if (report != nullptr) {
				StringClass line;
				line.Format("road tunnel '%s' resolves edge '%s' but its road is bound at %d portal(s)\r\n",
								r.Id.Peek_Buffer(),r.RoadEdgeId.Peek_Buffer(),bound);
				*report += line;
			}
		}
	}
	return failures;
}
