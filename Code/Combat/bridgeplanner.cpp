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
 *                     $Archive:: /Commando/Code/Combat/bridgeplanner.cpp                     $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "bridgeplanner.h"

#include "bridgesection.h"
#include "bridgesystem.h"
#include "crc.h"
#include "escarpment.h"
#include "roadgraphgenerator.h"
#include "roadspline.h"
#include "roadsystem.h"
#include "strategicplanner.h"
#include "terrainmask.h"
#include "watersystem.h"
#include "worldboundsservice.h"
#include "worldterrainsystem.h"
#include "wwdebug.h"
#include "wwmath.h"


bool				BridgePlanner::Built			= false;
unsigned int	BridgePlanner::Checksum		= 0;

DynamicVectorClass<int>							BridgePlanner::OwnedBridgeIds;
DynamicVectorClass<ResolvedBridge>			BridgePlanner::Bridges;
DynamicVectorClass<CrossingResolutionRecord>	BridgePlanner::Diagnostics;


//	The bank-point search step and bound, world units -- a bridge's own span is measured by
//	walking outward from the crossing point until the obstruction (water or cliff face) clears,
//	capped so a search over an implausibly wide obstruction gives up rather than looping.
static const float BANK_SEARCH_STEP		= 4.0f;
static const int	 BANK_SEARCH_MAX_STEPS	= 60;		// 240 world units each side

//	How close to an escarpment face counts as "still on the cliff", for the same bank search.
static const float CLIFF_FACE_TEST_DISTANCE = 3.0f;

//	Deck structural depth, added on top of the clearance margin -- Section 11A.3's own third
//	term, a fixed stand-in until a real per-definition value is confirmed.
static const float DECK_STRUCTURAL_DEPTH = 0.6f;

//	Cumulative thresholds for the deterministic initial-damage-state draw, PRISTINE-heaviest.
static const float DAMAGE_STATE_THRESHOLD[BRIDGE_STATE_COUNT] = { 0.70f, 0.85f, 0.95f, 1.00f };

static const float CHECKSUM_QUANTUM = 1000.0f;

//	Gap report item 5's own fallback bounds, world units/steps.  All deterministic: no stage
//	below ever draws a hash or consults the clock, only a fixed, ordered search.
static const int	  FALLBACK_REPOSITION_MAX_STEPS	= 6;		// stage 1, each side -- 12 tries, <= 24 world units
static const int	  FALLBACK_APPROACH_MAX_STEPS		= 4;		// stage 2
static const float FALLBACK_APPROACH_STEP				= BANK_SEARCH_STEP;

//	A cap length shorter than this (or no bridge definition to ask) still gets a floor to
//	compute a grade against, rather than a near-zero run producing a near-vertical one.
static const float MIN_APPROACH_RUN = 4.0f;

//	How far apart two bridges' own centre lines have to stay -- the collision check gap report
//	item 5 asks for, against whatever this same generation pass has already committed.
static const float BRIDGE_MIN_SEPARATION = 8.0f;

//	How finely the span between the two abutments is resampled for the clearance check, so a
//	sag or a rise in the middle that the two end heights alone would not show is still caught.
static const float CLEARANCE_SAMPLE_SPACING = 8.0f;


static const char * Road_Graph_Class_Bridge_Name(RoadGraphClassType cls)
{
	switch (cls) {
		case ROAD_GRAPH_HIGHWAY:
		case ROAD_GRAPH_PRIMARY:			return "ow_bridge_steel_truss";
		case ROAD_GRAPH_SECONDARY:
		case ROAD_GRAPH_CITY_STREET:		return "ow_bridge_concrete";
		case ROAD_GRAPH_SERVICE:
		case ROAD_GRAPH_DIRT_TRACK:
		default:									return "ow_bridge_timber";
	}
}


BridgePlannerParams::BridgePlannerParams(void) :
	Seed (1),
	WaterClearanceMargin (2.0f),
	MaxSpanLength (200.0f),
	MaxApproachGradeRadians (0.5f),
	MaxRerouteAttemptsPerEdge (3)
{
}


ResolvedBridge::ResolvedBridge(void) :
	BridgeId (-1),
	InitialState (BRIDGE_STATE_PRISTINE),
	CountsAsConnectivity (true),
	StrategicImportance (0.0f)
{
}


bool ResolvedBridge::operator == (const ResolvedBridge & that) const
{
	return (Id == that.Id) && (BridgeId == that.BridgeId) && (CrossingNodeId == that.CrossingNodeId) &&
			 (RoadEdgeId == that.RoadEdgeId) && (InitialState == that.InitialState) &&
			 (CountsAsConnectivity == that.CountsAsConnectivity) && (StrategicImportance == that.StrategicImportance);
}


CrossingResolutionRecord::CrossingResolutionRecord(void) :
	Resolved (false),
	AttemptsMade (0),
	StageResolvedAt (CROSSING_STAGE_ORIGINAL_POSITION),
	Displacement (0.0f),
	FinalFailureReason (CROSSING_FAILURE_NONE)
{
}


bool CrossingResolutionRecord::operator == (const CrossingResolutionRecord & that) const
{
	return (CrossingNodeId == that.CrossingNodeId) && (RoadEdgeId == that.RoadEdgeId) &&
			 (Resolved == that.Resolved) && (AttemptsMade == that.AttemptsMade) &&
			 (StageResolvedAt == that.StageResolvedAt) && (Displacement == that.Displacement) &&
			 (FinalFailureReason == that.FinalFailureReason);
}


const char * Crossing_Failure_Reason_Name(CrossingFailureReasonType reason)
{
	switch (reason) {
		case CROSSING_FAILURE_SPAN_TOO_LONG:					return "SPAN_TOO_LONG";
		case CROSSING_FAILURE_APPROACH_GRADE_TOO_HIGH:	return "APPROACH_GRADE_TOO_HIGH";
		case CROSSING_FAILURE_NO_VALID_ABUTMENT:			return "NO_VALID_ABUTMENT";
		case CROSSING_FAILURE_INSUFFICIENT_CLEARANCE:		return "INSUFFICIENT_CLEARANCE";
		case CROSSING_FAILURE_ROAD_WIDTH_MISMATCH:			return "ROAD_WIDTH_MISMATCH";
		case CROSSING_FAILURE_COLLISION:						return "COLLISION";
		case CROSSING_FAILURE_OUT_OF_BOUNDS:					return "OUT_OF_BOUNDS";
		case CROSSING_FAILURE_NO_ALTERNATE_ROUTE:			return "NO_ALTERNATE_ROUTE";
		default:														return "NONE";
	}
}


void BridgePlanner::Init(void)
{
	Clear();
}


void BridgePlanner::Shutdown(void)
{
	Clear();
}


void BridgePlanner::Clear(void)
{
	if (OwnedBridgeIds.Count() > 0) {
		BridgeSystem::Destroy_Geometry();

		DynamicVectorClass<BridgeClass> keep;
		for (int i = 0; i < BridgeSystem::Get_Bridge_Count(); i++) {
			BridgeClass * bridge = BridgeSystem::Peek_Bridge(i);
			if (bridge == nullptr) continue;

			bool owned = false;
			for (int j = 0; j < OwnedBridgeIds.Count(); j++) {
				if (OwnedBridgeIds[j] == i) { owned = true; break; }
			}
			if (!owned) keep.Add(*bridge);
		}

		BridgeSystem::Clear_Bridges();
		for (int i = 0; i < keep.Count(); i++) {
			BridgeSystem::Add_Bridge(keep[i]);
		}
		if (keep.Count() > 0) {
			BridgeSystem::Build_Layout();
			BridgeSystem::Build_Geometry();
		}
	}

	OwnedBridgeIds.Delete_All();
	Bridges.Delete_All();
	Diagnostics.Delete_All();

	Built		= false;
	Checksum	= 0;
}


bool BridgePlanner::Get_Bridge(int index,ResolvedBridge * out)
{
	if ((index < 0) || (index >= Bridges.Count()) || (out == nullptr)) {
		return false;
	}
	*out = Bridges[index];
	return true;
}


bool BridgePlanner::Get_Diagnostic(int index,CrossingResolutionRecord * out)
{
	if ((index < 0) || (index >= Diagnostics.Count()) || (out == nullptr)) {
		return false;
	}
	*out = Diagnostics[index];
	return true;
}


int BridgePlanner::Get_Unresolved_Count(void)
{
	int count = 0;
	for (int i = 0; i < Diagnostics.Count(); i++) {
		if (!Diagnostics[i].Resolved) count++;
	}
	return count;
}


//	Is this point still on the obstruction the bridge is meant to cross.
static bool On_Obstruction(const Vector3 & point,bool is_water)
{
	if (is_water) {
		return WaterSystem::Is_Point_In_Water(point.X,point.Y);
	}
	float upper,lower;
	return EscarpmentSystem::Sample_Face(point.X,point.Y,CLIFF_FACE_TEST_DISTANCE,&upper,&lower);
}


//	Walk outward from the crossing point along the nearest road's own local tangent -- the
//	direction the road is actually travelling through the obstruction, not across it -- until
//	the obstruction clears on each side, bounded.  A road that meets an obstruction usually
//	meets it roughly head-on, so this is the axis the obstruction is actually narrow along.
//	Returns false if the search never clears within BANK_SEARCH_MAX_STEPS on either side (an
//	obstruction wider than this bridge planner will consider spanning).
static bool Find_Bridge_Banks(const Vector3 & crossing_position,int road_id,bool is_water,
										 Vector3 * start_out,Vector3 * end_out,float * span_out)
{
	Vector3 tangent(1.0f,0.0f,0.0f);
	RoadSplineClass * road = RoadSystem::Peek_Road(road_id);
	if (road != nullptr) {
		int best = -1;
		float best_distance = 0.0f;
		for (int i = 0; i < road->Get_Point_Count(); i++) {
			float d = (road->Get_Point(i).Position - crossing_position).Length();
			if ((best < 0) || (d < best_distance)) { best = i; best_distance = d; }
		}
		if (best >= 0) tangent = road->Get_Point(best).Tangent;
	}

	//	Flattened to the plan: a bridge's own span is a question in X/Y, and Z comes from the
	//	terrain sample below.
	Vector3 walk (tangent.X,tangent.Y,0.0f);
	float walk_length = walk.Length();
	walk = (walk_length > 0.0001f) ? (walk / walk_length) : Vector3(1.0f,0.0f,0.0f);

	float extent_pos = -1.0f;
	for (int i = 1; i <= BANK_SEARCH_MAX_STEPS; i++) {
		Vector3 p = crossing_position + (walk * (BANK_SEARCH_STEP * (float)i));
		if (!On_Obstruction(p,is_water)) { extent_pos = BANK_SEARCH_STEP * (float)i; break; }
	}

	float extent_neg = -1.0f;
	for (int i = 1; i <= BANK_SEARCH_MAX_STEPS; i++) {
		Vector3 p = crossing_position - (walk * (BANK_SEARCH_STEP * (float)i));
		if (!On_Obstruction(p,is_water)) { extent_neg = BANK_SEARCH_STEP * (float)i; break; }
	}

	if ((extent_pos < 0.0f) || (extent_neg < 0.0f)) return false;

	Vector3 start = crossing_position - (walk * extent_neg);
	Vector3 end   = crossing_position + (walk * extent_pos);
	float h;
	if (WorldTerrainSystem::Sample_Height(start.X,start.Y,&h)) start.Z = h;
	if (WorldTerrainSystem::Sample_Height(end.X,end.Y,&h))   end.Z   = h;

	*start_out	= start;
	*end_out		= end;
	*span_out	= extent_pos + extent_neg;
	return true;
}


//	Point-to-segment and segment-to-segment minimum distance in the plan (Z ignored, same as
//	every other planform question this generator asks).  Exact for two segments that do not
//	cross; a safe (near-zero) result for two that do, which the caller treats as a collision
//	either way, so under-estimating on that side costs nothing.
static float Point_Segment_Distance_2D(const Vector3 & p,const Vector3 & a,const Vector3 & b)
{
	float dx = b.X - a.X;
	float dy = b.Y - a.Y;
	float len_sq = (dx * dx) + (dy * dy);
	float t = 0.0f;
	if (len_sq > WWMATH_EPSILON) {
		t = (((p.X - a.X) * dx) + ((p.Y - a.Y) * dy)) / len_sq;
		if (t < 0.0f) t = 0.0f;
		if (t > 1.0f) t = 1.0f;
	}
	float cx = a.X + (dx * t);
	float cy = a.Y + (dy * t);
	float ddx = p.X - cx;
	float ddy = p.Y - cy;
	return WWMath::Sqrt((ddx * ddx) + (ddy * ddy));
}


static float Segment_Distance_2D(const Vector3 & a0,const Vector3 & a1,const Vector3 & b0,const Vector3 & b1)
{
	float d = Point_Segment_Distance_2D(a0,b0,b1);
	d = WWMath::Min(d,Point_Segment_Distance_2D(a1,b0,b1));
	d = WWMath::Min(d,Point_Segment_Distance_2D(b0,a0,a1));
	d = WWMath::Min(d,Point_Segment_Distance_2D(b1,a0,a1));
	return d;
}


/***********************************************************************************************
 * Evaluate_Crossing_Candidate -- every legality check, against one candidate position           *
 *                                                                                             *
 *	Gap report item 5's own seven failure categories, all checkable before anything is          *
 *	committed to BridgeSystem or RoadSystem -- no candidate is ever built-then-discarded, since  *
 *	neither system has a per-object undo cheap enough to try that in a search loop.  Returns     *
 *	true and the winning geometry only when every check passes.                                 *
 *=============================================================================================*/
static bool Evaluate_Crossing_Candidate(
	const Vector3 & crossing_position,int road_id,bool is_water,RoadGraphClassType road_class,
	const BridgePlannerParams & params,float approach_run_bonus,
	Vector3 * start_out,Vector3 * end_out,float * deck_height_out,
	CrossingFailureReasonType * reason_out)
{
	*reason_out = CROSSING_FAILURE_NONE;

	Vector3 start,end;
	float span;
	if (!Find_Bridge_Banks(crossing_position,road_id,is_water,&start,&end,&span)) {
		*reason_out = CROSSING_FAILURE_NO_VALID_ABUTMENT;
		return false;
	}
	if (span > params.MaxSpanLength) {
		*reason_out = CROSSING_FAILURE_SPAN_TOO_LONG;
		return false;
	}

	if (WorldBoundsService::Is_Configured()) {
		if ((WorldBoundsService::Classify_Point(Vector2(start.X,start.Y)) >= WORLD_ZONE_CLIFF_BAND) ||
			 (WorldBoundsService::Classify_Point(Vector2(end.X,end.Y))   >= WORLD_ZONE_CLIFF_BAND) ||
			 (WorldBoundsService::Classify_Point(Vector2(crossing_position.X,crossing_position.Y)) >= WORLD_ZONE_CLIFF_BAND)) {
			*reason_out = CROSSING_FAILURE_OUT_OF_BOUNDS;
			return false;
		}
	}

	//	11A.2: the lowest elevation that actually clears the water, never more.  Zero for an
	//	interior-cliff crossing -- the deck follows the two bank heights directly.
	float deck_height = 0.0f;
	if (is_water) {
		float water_height;
		if (WaterSystem::Get_Water_Height(crossing_position.X,crossing_position.Y,&water_height)) {
			float higher_abutment = (start.Z > end.Z) ? start.Z : end.Z;
			float required = water_height + params.WaterClearanceMargin + DECK_STRUCTURAL_DEPTH;
			if (required > higher_abutment) {
				deck_height = required - higher_abutment;
			}
		}
	}

	const char * bridge_name = Road_Graph_Class_Bridge_Name(road_class);
	BridgeDefinitionClass * definition = BridgeSystem::Find_Definition(bridge_name);

	//	Grade.  The deck sits level at the higher abutment plus deck_height; whichever abutment
	//	is lower climbs the whole difference over the assumed approach run -- the bridge
	//	definition's own start-cap length, floored and widened by the caller's own
	//	approach_run_bonus (fallback stage 2's "move the transition farther from the obstacle").
	{
		float deck_surface = WWMath::Max(start.Z,end.Z) + deck_height;
		float approach_run = MIN_APPROACH_RUN;
		if (definition != nullptr) {
			float cap_length = definition->Get_Section_Length(BRIDGE_SECTION_START_CAP);
			if (cap_length > approach_run) approach_run = cap_length;
		}
		approach_run += approach_run_bonus;

		float rise_start = WWMath::Fabs(deck_surface - start.Z);
		float rise_end   = WWMath::Fabs(deck_surface - end.Z);
		float grade_start = WWMath::Atan2(rise_start,approach_run);
		float grade_end   = WWMath::Atan2(rise_end,approach_run);
		float worst_grade = WWMath::Max(grade_start,grade_end);
		if (worst_grade > params.MaxApproachGradeRadians) {
			*reason_out = CROSSING_FAILURE_APPROACH_GRADE_TOO_HIGH;
			return false;
		}
	}

	//	Clearance.  The deck-height formula above already derives the lowest legal elevation
	//	from the two abutment heights alone; resample the terrain the span actually crosses in
	//	case its middle is not well described by a straight line between the two ends.
	if (is_water && (definition != nullptr) && definition->States_Clearance()) {
		float deck_surface = WWMath::Max(start.Z,end.Z) + deck_height;
		Vector3 delta = end - start;
		float span_length = delta.Length();
		if (span_length > WWMATH_EPSILON) {
			int samples = (int)(span_length / CLEARANCE_SAMPLE_SPACING) + 1;
			for (int i = 0; i <= samples; i++) {
				float t = (float)i / (float)samples;
				Vector3 p = start + (delta * t);
				float terrain_h;
				if (WorldTerrainSystem::Sample_Height(p.X,p.Y,&terrain_h)) {
					float clearance = (deck_surface - DECK_STRUCTURAL_DEPTH) - terrain_h;
					if (clearance < definition->Get_Required_Clearance()) {
						*reason_out = CROSSING_FAILURE_INSUFFICIENT_CLEARANCE;
						return false;
					}
				}
			}
		}
	}

	//	Width.  Against the carriageway alone (RoadSplineClass::Get_Width), not
	//	Get_Total_Width's shoulders -- a deck that omits the full shoulder either side of the
	//	carriageway is still wide enough for every vehicle the road itself carries, and Section
	//	10A.2 keeps shoulders as their own cross-section field precisely so this question and
	//	"does the vehicle fit" stay separate ones.  A road narrower than the bridge is fine; the
	//	reverse is the failure item 3/4 exist to catch -- a deck a connected road's own vehicles
	//	do not actually fit on.
	if (definition != nullptr) {
		RoadSplineClass * road = RoadSystem::Peek_Road(road_id);
		if ((road != nullptr) && (definition->Get_Width() > 0.0f) &&
			 (definition->Get_Width() < road->Get_Width())) {
			*reason_out = CROSSING_FAILURE_ROAD_WIDTH_MISMATCH;
			return false;
		}
	}

	//	Collision, against every bridge this same generation pass has already committed --
	//	nothing else procedural exists yet at Phase 9's own point in the pipeline.
	{
		float half_width = ((definition != nullptr) && (definition->Get_Width() > 0.0f))
									? (definition->Get_Width() * 0.5f) : 4.0f;
		for (int i = 0; i < BridgeSystem::Get_Bridge_Count(); i++) {
			BridgeClass * other = BridgeSystem::Peek_Bridge(i);
			if (other == nullptr) continue;
			float other_half = (other->Get_Width() > 0.0f) ? (other->Get_Width() * 0.5f) : 4.0f;
			float d = Segment_Distance_2D(start,end,other->Get_Start(),other->Get_End());
			if (d < (half_width + other_half + BRIDGE_MIN_SEPARATION)) {
				*reason_out = CROSSING_FAILURE_COLLISION;
				return false;
			}
		}
	}

	*start_out			= start;
	*end_out				= end;
	*deck_height_out	= deck_height;
	return true;
}


/***********************************************************************************************
 * Resolve_Crossing_Position -- gap report item 5, fallback stages 0-2                           *
 *                                                                                             *
 *	Stage 0: the original crossing position.  Stage 1: a bounded, deterministic search along     *
 *	the obstacle itself (perpendicular to the road -- the axis a riverbank or a cliff face       *
 *	actually runs along), smallest displacement first.  Stage 2: only ever tried after a grade    *
 *	failure specifically, since nothing about a longer assumed approach run fixes a span, an       *
 *	abutment, a bounds or a width/collision problem -- a bounded search widening the approach       *
 *	run instead of moving anything.  Returns the winning candidate and which stage found it, or      *
 *	false with the last failure reason any stage actually produced.                                   *
 *=============================================================================================*/
static bool Resolve_Crossing_Position(
	const Vector3 & original_position,int road_id,bool is_water,RoadGraphClassType road_class,
	const BridgePlannerParams & params,
	Vector3 * start_out,Vector3 * end_out,float * deck_height_out,
	CrossingFallbackStageType * stage_out,float * displacement_out,int * attempts_out,
	CrossingFailureReasonType * reason_out)
{
	*attempts_out = 0;

	(*attempts_out)++;
	if (Evaluate_Crossing_Candidate(original_position,road_id,is_water,road_class,params,0.0f,
												start_out,end_out,deck_height_out,reason_out)) {
		*stage_out			= CROSSING_STAGE_ORIGINAL_POSITION;
		*displacement_out	= 0.0f;
		return true;
	}

	Vector3 tangent(1.0f,0.0f,0.0f);
	RoadSplineClass * road = RoadSystem::Peek_Road(road_id);
	if (road != nullptr) {
		int index; float lateral,along;
		if (road->Find_Closest_Point(original_position,&index,&lateral,&along)) {
			tangent = road->Get_Point(index).Tangent;
		}
	}
	Vector3 perp(-tangent.Y,tangent.X,0.0f);
	float perp_length = perp.Length();
	perp = (perp_length > 0.0001f) ? (perp / perp_length) : Vector3(0.0f,1.0f,0.0f);

	for (int step = 1; step <= FALLBACK_REPOSITION_MAX_STEPS; step++) {
		for (int side = 0; side < 2; side++) {
			float sign = (side == 0) ? 1.0f : -1.0f;
			Vector3 candidate = original_position + (perp * (BANK_SEARCH_STEP * (float)step * sign));
			(*attempts_out)++;
			if (Evaluate_Crossing_Candidate(candidate,road_id,is_water,road_class,params,0.0f,
														start_out,end_out,deck_height_out,reason_out)) {
				*stage_out			= CROSSING_STAGE_LOCAL_REPOSITION;
				*displacement_out	= BANK_SEARCH_STEP * (float)step;
				return true;
			}
		}
	}

	if (*reason_out == CROSSING_FAILURE_APPROACH_GRADE_TOO_HIGH) {
		for (int step = 1; step <= FALLBACK_APPROACH_MAX_STEPS; step++) {
			(*attempts_out)++;
			if (Evaluate_Crossing_Candidate(original_position,road_id,is_water,road_class,params,
														FALLBACK_APPROACH_STEP * (float)step,
														start_out,end_out,deck_height_out,reason_out)) {
				*stage_out			= CROSSING_STAGE_EXTENDED_APPROACH;
				*displacement_out	= 0.0f;
				return true;
			}
		}
	}

	return false;
}


/***********************************************************************************************
 * Generate -- Phase 9 steps 1-9, gap report item 5's own fallback chain (steps 1-6)              *
 *=============================================================================================*/
bool BridgePlanner::Generate(const BridgePlannerParams & params,StringClass * error_out)
{
	#define FAIL(msg) { if (error_out) *error_out = msg; Clear(); return false; }

	Clear();

	if (!RoadGraphGenerator::Has_Graph())				FAIL("no road graph -- generate the road graph first");
	if (params.WaterClearanceMargin <= 0.0f)			FAIL("water clearance margin must be positive");
	if (params.MaxSpanLength <= 0.0f)					FAIL("max span length must be positive");
	if (params.MaxApproachGradeRadians <= 0.0f)		FAIL("max approach grade must be positive");
	if (params.MaxRerouteAttemptsPerEdge < 0)			FAIL("max reroute attempts must not be negative");

	int bridge_index = 0;
	int road_splits  = 0;

	for (int e = 0; e < RoadGraphGenerator::Get_Edge_Count(); e++) {

		RoadGraphEdge first_edge;
		if (!RoadGraphGenerator::Get_Edge(e,&first_edge)) continue;

		int current_road_id			= first_edge.RoadId;
		int reroute_attempts_used	= 0;
		bool edge_has_committed_bridge = false;
		int crossing_cursor			= 0;

		//	Stage 4/5 (reroute) operate on the whole edge, not one crossing -- a reroute
		//	replaces the edge's own road and its own crossing list, so that list is re-read
		//	from RoadGraphGenerator on every iteration rather than snapshotted once.  Bounded:
		//	each reroute costs one of MaxRerouteAttemptsPerEdge, and once any crossing on this
		//	edge has already been built into a real bridge/road split, a reroute is refused
		//	outright (see the comment where that refusal happens) -- both together mean this
		//	loop terminates in a number of iterations bounded by
		//	(MaxRerouteAttemptsPerEdge + 1) times the longest crossing list any one reroute of
		//	this edge ever produces, never open-ended.
		while (true) {

			RoadGraphEdge live_edge;
			if (!RoadGraphGenerator::Get_Edge(e,&live_edge)) break;
			if (crossing_cursor >= live_edge.UnresolvedCrossingIds.Count()) break;

			const StringClass crossing_id = live_edge.UnresolvedCrossingIds[crossing_cursor];

			StrategicNode crossing_node;
			bool found = false;
			for (int n = 0; n < StrategicPlanner::Get_Node_Count(); n++) {
				StrategicNode candidate;
				if (!StrategicPlanner::Get_Node(n,&candidate)) continue;
				if (candidate.Id == crossing_id) { crossing_node = candidate; found = true; break; }
			}
			if (!found) { crossing_cursor++; continue; }

			bool is_water = WaterSystem::Is_Point_In_Water(crossing_node.Position.X,crossing_node.Position.Y);

			Vector3 start,end;
			float deck_height = 0.0f;
			CrossingFallbackStageType stage = CROSSING_STAGE_ORIGINAL_POSITION;
			float displacement = 0.0f;
			int attempts = 0;
			CrossingFailureReasonType reason = CROSSING_FAILURE_NONE;

			bool resolved = Resolve_Crossing_Position(crossing_node.Position,current_road_id,is_water,
																	  live_edge.Class,params,
																	  &start,&end,&deck_height,
																	  &stage,&displacement,&attempts,&reason);

			if (!resolved) {

				//	Stage 3: alternate crossing type.  No tunnel system exists in this codebase
				//	to fall back to -- BridgePlanner already builds a bridge for a
				//	TUNNEL_CANDIDATE node exactly as it does for a CROSSING_CANDIDATE one, since
				//	nothing else exists that could -- so this stage is honestly a no-op rather
				//	than a fabricated crossing type, and the chain proceeds to stage 4 carrying
				//	whichever failure reason stages 0-2 last produced.

				//	Stage 4: reroute.  Refused, not merely skipped, once this edge has already
				//	committed a real bridge and road split for an earlier crossing -- replacing
				//	the edge's own road out from under an already-spliced bridge would orphan
				//	it (Reroute_Edge_Avoiding removes the old road entirely), and undoing a
				//	committed split/bridge is not a capability either RoadSystem or BridgeSystem
				//	has.  Edges with more than one crossing are the rare case (Phase 8's own
				//	routing tends to produce one crossing per edge); when it does happen and an
				//	edge's second crossing fails after its first already succeeded, that failure
				//	goes straight to stage 6 rather than stage 4.
				bool reroute_allowed = !edge_has_committed_bridge &&
												(reroute_attempts_used < params.MaxRerouteAttemptsPerEdge);
				if (reroute_allowed) {
					reroute_attempts_used++;
					StringClass reroute_error;
					if (RoadGraphGenerator::Reroute_Edge_Avoiding(live_edge.Id,crossing_node.Position,&reroute_error)) {
						RoadGraphEdge rerouted_edge;
						if (RoadGraphGenerator::Get_Edge(e,&rerouted_edge)) {
							current_road_id = rerouted_edge.RoadId;
						}
						crossing_cursor = 0;		// the replacement route's own crossing list starts fresh
						continue;
					}
					reason = CROSSING_FAILURE_NO_ALTERNATE_ROUTE;
				}

				//	Stage 6: reject.  Every permitted recovery has failed for this one crossing;
				//	record why and move on to whatever the edge's current crossing list still
				//	carries.  One crossing failing does not abandon the rest of this edge or the
				//	rest of the network -- deciding whether the *world* is still acceptable is
				//	TransportationNetworkValidator's own job, downstream of this; Get_Unresolved_
				//	Count is exactly what that caller checks to learn this happened at all.
				CrossingResolutionRecord record;
				record.CrossingNodeId		= crossing_id;
				record.RoadEdgeId			= live_edge.Id;
				record.Resolved				= false;
				record.AttemptsMade		= attempts + reroute_attempts_used;
				record.FinalFailureReason	= (reason != CROSSING_FAILURE_NONE) ? reason : CROSSING_FAILURE_NO_VALID_ABUTMENT;
				Diagnostics.Add(record);

				WWDEBUG_SAY(("BridgePlanner: crossing '%s' on edge '%s' unresolved after %d attempt(s) -- %s\r\n",
								 crossing_id.Peek_Buffer(),live_edge.Id.Peek_Buffer(),record.AttemptsMade,
								 Crossing_Failure_Reason_Name(record.FinalFailureReason)));

				crossing_cursor++;
				continue;
			}

			//	Resolved -- build the real bridge and splice the road at whichever position/
			//	approach the fallback chain actually won at.
			const char * bridge_name = Road_Graph_Class_Bridge_Name(live_edge.Class);

			BridgeClass bridge;
			StringClass name; name.Format("bridge_%d",bridge_index);
			bridge.Set_Name(name.Peek_Buffer());
			bridge.Set_Definition(bridge_name);
			bridge.Set_Endpoints(start,end);
			bridge.Set_Deck_Height(deck_height);

			int bridge_id = BridgeSystem::Add_Bridge(bridge);
			OwnedBridgeIds.Add(bridge_id);

			//	Item 3 of the gap report, Phase 8 reopened: split the road this bridge crosses
			//	at the two abutments, so the original centre line stops being one unbroken road
			//	that happens to have a bridge standing on top of it.
			RoadSplineClass * live_road = RoadSystem::Peek_Road(current_road_id);
			if (live_road != nullptr) {
				int index_a,index_b;
				float lateral_a,lateral_b,along_a,along_b;
				if (live_road->Find_Closest_Point(start,&index_a,&lateral_a,&along_a) &&
					 live_road->Find_Closest_Point(end,&index_b,&lateral_b,&along_b)) {

					float distance_a = (along_a < along_b) ? along_a : along_b;
					float distance_b = (along_a < along_b) ? along_b : along_a;

					int segment_a_id,segment_b_id;
					if (RoadSystem::Trim_Road_Range(current_road_id,distance_a,distance_b,
																&segment_a_id,&segment_b_id)) {

						if (segment_a_id != -1) {
							RoadSplineClass * a = RoadSystem::Peek_Road(segment_a_id);
							if (a != nullptr) {
								int last = a->Get_Point_Count() - 1;
								RoadConnectionClass & connection = a->Get_End_Connection();
								connection.Type		= ROAD_ENDPOINT_BRIDGE;
								connection.TargetID	= bridge_id;
								if (last >= 0) {
									connection.Position		= a->Get_Point(last).Position;
									connection.Direction	= a->Get_Point(last).Tangent;
								}
							}
						}
						if (segment_b_id != -1) {
							RoadSplineClass * b = RoadSystem::Peek_Road(segment_b_id);
							if (b != nullptr) {
								RoadConnectionClass & connection = b->Get_Start_Connection();
								connection.Type		= ROAD_ENDPOINT_BRIDGE;
								connection.TargetID	= bridge_id;
								if (b->Get_Point_Count() > 0) {
									connection.Position		= b->Get_Point(0).Position;
									connection.Direction	= -b->Get_Point(0).Tangent;
								}
							}
						}

						if (segment_b_id != -1) {
							RoadGraphGenerator::Adopt_Owned_Road_Id(segment_b_id);
						}

						road_splits++;
						current_road_id = (segment_b_id != -1) ? segment_b_id : current_road_id;

					} else {
						WWDEBUG_SAY(("BridgePlanner: bridge %d placed but road %d could not be split "
										 "at it (interval %.1f-%.1f of %.1f) -- road runs through unsplit\r\n",
										 bridge_id,current_road_id,distance_a,distance_b,live_road->Get_Length()));
					}
				}
			}

			ResolvedBridge resolved_bridge;
			resolved_bridge.Id				= name;
			resolved_bridge.BridgeId		= bridge_id;
			resolved_bridge.CrossingNodeId	= crossing_id;
			resolved_bridge.RoadEdgeId		= live_edge.Id;

			float draw = TerrainMaskClass::Hash_Value(bridge_index,0,(int)params.Seed);
			int state = BRIDGE_STATE_BROKEN;
			for (int s = 0; s < BRIDGE_STATE_COUNT; s++) {
				if (draw <= DAMAGE_STATE_THRESHOLD[s]) { state = s; break; }
			}
			resolved_bridge.InitialState			= state;
			resolved_bridge.CountsAsConnectivity	= (state != BRIDGE_STATE_BROKEN);

			if (live_edge.Class == ROAD_GRAPH_HIGHWAY)			resolved_bridge.StrategicImportance = 1.0f;
			else if (live_edge.Class == ROAD_GRAPH_PRIMARY)	resolved_bridge.StrategicImportance = 0.6f;
			else if ((live_edge.Class == ROAD_GRAPH_SECONDARY) ||
						(live_edge.Class == ROAD_GRAPH_CITY_STREET))	resolved_bridge.StrategicImportance = 0.4f;
			else																resolved_bridge.StrategicImportance = 0.2f;

			Bridges.Add(resolved_bridge);

			CrossingResolutionRecord record;
			record.CrossingNodeId		= crossing_id;
			record.RoadEdgeId			= live_edge.Id;
			record.Resolved				= true;
			record.AttemptsMade		= attempts;
			record.StageResolvedAt	= stage;
			record.Displacement		= displacement;
			Diagnostics.Add(record);

			if (stage != CROSSING_STAGE_ORIGINAL_POSITION) {
				WWDEBUG_SAY(("BridgePlanner: crossing '%s' on edge '%s' resolved at fallback stage %d "
								 "(%d attempt(s), %.1f world unit(s) off the original point)\r\n",
								 crossing_id.Peek_Buffer(),live_edge.Id.Peek_Buffer(),(int)stage,attempts,displacement));
			}

			edge_has_committed_bridge = true;
			bridge_index++;
			crossing_cursor++;
		}
	}

	if (OwnedBridgeIds.Count() > 0) {
		BridgeSystem::Build_Layout();
		BridgeSystem::Conform_Abutments();

		//	Connect_Roads' own default tolerance (8 units) assumes a road that already runs
		//	straight into the bridge's own abutment.  Find_Bridge_Banks instead finds the two
		//	banks along the road's own tangent from the crossing point, so a bridge's own
		//	Start/End can sit somewhat off from where Trim_Road_Range actually cut that road.
		//	The bound this search itself can produce is exactly the bank search's own reach, so
		//	that is the tolerance Connect_Roads is given: it can bind anything this generator
		//	itself could have placed a bridge for.
		BridgeSystem::Connect_Roads(BANK_SEARCH_STEP * (float)BANK_SEARCH_MAX_STEPS);
		BridgeSystem::Build_Geometry();

		for (int i = 0; i < Bridges.Count(); i++) {
			BridgeSystem::Set_Damage_State(Bridges[i].BridgeId,Bridges[i].InitialState);
		}
	}

	{
		CRCEngine crc;
		for (int i = 0; i < Bridges.Count(); i++) {
			const ResolvedBridge & b = Bridges[i];
			int q_state			= b.InitialState;
			int q_counts		= b.CountsAsConnectivity ? 1 : 0;
			int q_importance	= (int)(b.StrategicImportance * CHECKSUM_QUANTUM);
			crc(&q_state,sizeof(q_state));
			crc(&q_counts,sizeof(q_counts));
			crc(&q_importance,sizeof(q_importance));
		}
		Checksum = (unsigned int)crc;
	}

	Built = true;

	WWDEBUG_SAY(("BridgePlanner: %d bridge(s), %d road split at its own crossing, %d unsplit, "
					 "%d crossing(s) unresolved after every fallback, checksum 0x%08X\r\n",
					 Bridges.Count(),road_splits,Bridges.Count() - road_splits,
					 Get_Unresolved_Count(),Checksum));

	#undef FAIL
	return true;
}


int BridgePlanner::Check_Traversable_States_Pass(StringClass * report)
{
	int faults = 0;
	for (int i = 0; i < Bridges.Count(); i++) {
		if (Bridges[i].InitialState == BRIDGE_STATE_BROKEN) continue;
		if (!BridgeSystem::Is_Traversable(Bridges[i].BridgeId)) {
			faults++;
			if (report) { StringClass l; l.Format("bridge '%s' is not traversable in a non-broken state\r\n",Bridges[i].Id.Peek_Buffer()); *report += l; }
		}
	}
	return faults;
}


int BridgePlanner::Check_Broken_States_Block(StringClass * report)
{
	int faults = 0;
	for (int i = 0; i < Bridges.Count(); i++) {
		if (Bridges[i].InitialState != BRIDGE_STATE_BROKEN) continue;
		if (BridgeSystem::Is_Traversable(Bridges[i].BridgeId)) {
			faults++;
			if (report) { StringClass l; l.Format("bridge '%s' is broken but still reports traversable\r\n",Bridges[i].Id.Peek_Buffer()); *report += l; }
		}
	}
	return faults;
}


int BridgePlanner::Check_No_Required_Connectivity_Lost(StringClass * report)
{
	//	Structural by construction: CountsAsConnectivity is set false for exactly the bridges
	//	whose InitialState is BROKEN, nothing downstream reads a broken bridge as connectivity.
	//	This regression net confirms the flag and the state actually agree.
	int faults = 0;
	for (int i = 0; i < Bridges.Count(); i++) {
		bool should_count = (Bridges[i].InitialState != BRIDGE_STATE_BROKEN);
		if (Bridges[i].CountsAsConnectivity != should_count) {
			faults++;
			if (report) { StringClass l; l.Format("bridge '%s' connectivity flag disagreed with its own state\r\n",Bridges[i].Id.Peek_Buffer()); *report += l; }
		}
	}
	return faults;
}
