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
 *                     $Archive:: /Commando/Code/wwphys/hybridinteractions.cpp                $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Runtime Phase 11 -- see hybridinteractions.h.  Every method order here is the plan's and is    *
 * fixed; the solver never reorders one to find a cheaper answer and never invents a method the   *
 * Phase-6 window does not already permit.                                                       *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#include "hybridinteractions.h"
#include "hybridrealization.h"

#include <math.h>
#include <string.h>
#include "wwdebug.h"


/***********************************************************************************************
**	Small shared arithmetic, in the same shapes the other hybrid phases use.
***********************************************************************************************/

static inline float Max2(float a,float b)	{ return (a > b) ? a : b; }
static inline float Min2(float a,float b)	{ return (a < b) ? a : b; }
static inline float Clamp2(float v,float lo,float hi)	{ return (v < lo) ? lo : ((v > hi) ? hi : v); }

static const float	PI_OVER_180		= 3.14159265358979323846f / 180.0f;

//	Section 12.3's own safety constants, in the plan's own terms.
static const float	OVERPASS_SAFETY_METERS			= 0.35f;
static const float	OVERPASS_SAFETY_HEIGHT_FRACTION	= 0.10f;

//	Section 8 Phase 11's station search.
static const float	STATION_HALF_LENGTH_W	= 8.0f;		// max(8W, 4S)
static const float	STATION_HALF_LENGTH_S	= 4.0f;
static const int		STATION_CAP					= 64;

//	"station is >=4W from an unrelated already compiled crossing"
static const float	CROSSING_SEPARATION_W	= 4.0f;

//	The overpass ramp floor, Section 8 Phase 11: `L_ramp = max(10W, |delta_z| / tan(max_grade))`.
static const float	RAMP_FLOOR_W				= 10.0f;

/*
**	A smoothstep's peak slope, relative to the straight line it replaces.
**
**	Section 8 Phase 11 asks for two things at once: the ramp is "one smoothstep ramp over exactly `L_ramp`"
**	and every sample of it must hold the class maximum grade.  For `y = h * smoothstep(x/L)` the slope is
**	steepest at the midpoint, where it is `1.5 h / L` -- so a ramp measured out by `|delta_z| / tan(g)`
**	averages `g` and peaks at `1.5 g`, and the per-sample requirement refuses it.  Measured on the fixture
**	seeds that is exactly what happened to a reserved overpass obligation.
**
**	Of the two statements the per-sample limit is the stricter one, and the grade limit is not ours to
**	move, so the *reservation* grows to satisfy both: `L_ramp` keeps the plan's formula as its floor and
**	carries this factor.  See `DECISIONS.md` D-010.
*/
static const float	RAMP_SMOOTHSTEP_PEAK	= 1.5f;

//	Section 8 Phase 11's at-grade junction angle window, in degrees.
static const float	JUNCTION_MIN_DEGREES		= 30.0f;
static const float	JUNCTION_MAX_DEGREES		= 150.0f;

//	"for each road, at least max(4W, its minimum turn radius) of centerline exists on both sides"
static const float	JUNCTION_ARM_W				= 4.0f;

//	The swept proxy sample spacings the plan states for each kind of compiled interval.
static const float	PASS_SAMPLE_W				= 0.5f;		// min(W/2, A/3)
static const float	PASS_SAMPLE_A				= 3.0f;
static const float	OVERPASS_SAMPLE_W			= 3.0f;		// min(W/3, A/4)
static const float	OVERPASS_SAMPLE_A			= 4.0f;


static Vector2 Unit(const Vector2 & v)
{
	const float length = v.Length();
	if (length <= 1.0e-6f) return Vector2(0.0f,0.0f);
	return Vector2(v.X / length,v.Y / length);
}


static Vector2 Quantized(const Vector2 & v)
{
	return Vector2(Hybrid_Quantize_Position(v.X),Hybrid_Quantize_Position(v.Y));
}


//	The shortest distance from a point to a segment, and where along it the nearest point fell.
static float Distance_To_Segment(const Vector2 & point,const Vector2 & a,const Vector2 & b,float * t_out)
{
	Vector2 axis = b - a;
	const float length = axis.Length();
	if (length <= 1.0e-6f) {
		if (t_out != nullptr) *t_out = 0.0f;
		return (point - a).Length();
	}
	const float t = Clamp2((((point.X - a.X) * axis.X) + ((point.Y - a.Y) * axis.Y)) / (length * length),
								  0.0f,1.0f);
	if (t_out != nullptr) *t_out = t;
	const Vector2 nearest(a.X + (axis.X * t),a.Y + (axis.Y * t));
	return (point - nearest).Length();
}


/***********************************************************************************************
**	Names.
***********************************************************************************************/

const char * Hybrid_Interaction_Method_Name(HybridInteractionMethod method)
{
	switch (method) {
		case HYBRID_METHOD_BRIDGE:				return "bridge";
		case HYBRID_METHOD_FORD:				return "ford";
		case HYBRID_METHOD_SURFACE_PASS:		return "surface_pass";
		case HYBRID_METHOD_ROAD_CUT:			return "road_cut";
		case HYBRID_METHOD_RAMP_PASS:			return "ramp_pass";
		case HYBRID_METHOD_TUNNEL:				return "tunnel";
		case HYBRID_METHOD_AT_GRADE_JUNCTION:	return "at_grade_junction";
		case HYBRID_METHOD_OVERPASS:			return "overpass";
		default:										break;
	}
	return "unknown";
}


bool Hybrid_Interaction_Counts_As_Bridge(HybridInteractionMethod method)
{
	//	Section 8 Phase 11: "A ford never satisfies any bridge quota."
	return method == HYBRID_METHOD_BRIDGE;
}


const char * Hybrid_Interaction_Problem_Name(HybridInteractionProblem problem)
{
	switch (problem) {
		case HYBRID_PROBLEM_NONE:							return "none";
		case HYBRID_PROBLEM_NO_HARD_VALID_STATION:	return "no_hard_valid_station";
		case HYBRID_PROBLEM_SPAN_TOO_LONG:				return "span_too_long";
		case HYBRID_PROBLEM_WIDTH_TOO_NARROW:			return "width_too_narrow";
		case HYBRID_PROBLEM_APPROACH_DOES_NOT_FIT:	return "approach_does_not_fit";
		case HYBRID_PROBLEM_GRADE_TOO_STEEP:			return "grade_too_steep";
		case HYBRID_PROBLEM_TURN_TOO_TIGHT:				return "turn_too_tight";
		case HYBRID_PROBLEM_CLEARANCE:					return "clearance";
		case HYBRID_PROBLEM_CROSS_SLOPE:					return "cross_slope";
		case HYBRID_PROBLEM_RESERVE_CONFLICT:			return "reserve_conflict";
		case HYBRID_PROBLEM_METHOD_UNAVAILABLE:		return "method_unavailable";
		case HYBRID_PROBLEM_CUT_OR_FILL:			return "cut_or_fill";
		case HYBRID_PROBLEM_GORE_RESERVE:					return "gore_reserve";
		case HYBRID_PROBLEM_INTERCHANGE_FOOTPRINT:		return "interchange_footprint";
		case HYBRID_PROBLEM_PIN_LANDING:					return "pin_landing";
		case HYBRID_PROBLEM_PROFILE_CONFLICT:			return "profile_conflict";
		case HYBRID_PROBLEM_TOO_CLOSE_TO_CROSSING:	return "too_close_to_crossing";
		default:													break;
	}
	return "unknown";
}


const char * Hybrid_Vertical_Constraint_Kind_Name(HybridVerticalConstraintKind kind)
{
	switch (kind) {
		case HYBRID_VCON_STRUCTURE:		return "structure";
		case HYBRID_VCON_LANDING:			return "landing";
		case HYBRID_VCON_JUNCTION:			return "junction";
		case HYBRID_VCON_LOCAL_JUNCTION:	return "local_junction";
		case HYBRID_VCON_RAMP_GORE:		return "ramp_gore";
		case HYBRID_VCON_OVERPASS_LOWER:	return "overpass_lower";
		default:									break;
	}
	return "unknown";
}


float Hybrid_Junction_Arm_Reach(HybridRoadClass road_class,float w)
{
	return Max2(JUNCTION_ARM_W * w,Hybrid_Road_Class_Spec(road_class).MinTurnRadiusW * w);
}


const char * Hybrid_Interaction_Failure_Name(HybridInteractionFailure failure)
{
	switch (failure) {
		case HYBRID_INTERACTION_OK:					return "ok";
		case HYBRID_INTERACTION_INVALID_INPUT:		return "invalid_input";
		case HYBRID_INTERACTION_NO_SOLUTION:		return "no_solution";
		case HYBRID_INTERACTION_OBLIGATION_LOST:	return "obligation_lost";
		case HYBRID_INTERACTION_QUOTA:				return "quota";
		case HYBRID_INTERACTION_SWEEP:				return "sweep";
		case HYBRID_INTERACTION_LIMIT:				return "limit";
		case HYBRID_INTERACTION_INTERCHANGE:	return "interchange";
		case HYBRID_INTERACTION_REALIZATION:	return "realization";
		case HYBRID_INTERACTION_DETACHED:			return "detached";
		default:												break;
	}
	return "unknown";
}


/***********************************************************************************************
**	The records.
***********************************************************************************************/

const char * Hybrid_Junction_Role_Name(HybridJunctionRole role)
{
	switch (role) {
		case HYBRID_JUNCTION_ROLE_NONE:								return "none";
		case HYBRID_JUNCTION_ROLE_INCIDENTAL:						return "incidental";
		case HYBRID_JUNCTION_ROLE_SHARED_NODE:						return "shared_node";
		case HYBRID_JUNCTION_ROLE_OVERPASS_OBLIGATION:			return "overpass_obligation";
		case HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_MERGE:			return "highway_ramp_merge";
		case HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_DIVERGE:			return "highway_ramp_diverge";
		case HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_LOCAL_JUNCTION:	return "highway_ramp_local_junction";
		default:																break;
	}
	return "unknown";
}


HybridJunctionRole Hybrid_Interaction_Junction_Role(const HybridInteraction & one)
{
	if (one.Role != HYBRID_JUNCTION_ROLE_NONE) return one.Role;
	if ((one.Method != HYBRID_METHOD_AT_GRADE_JUNCTION) && (one.Method != HYBRID_METHOD_OVERPASS)) {
		return HYBRID_JUNCTION_ROLE_NONE;
	}
	if (one.Mandatory && (one.Method == HYBRID_METHOD_OVERPASS)) return HYBRID_JUNCTION_ROLE_OVERPASS_OBLIGATION;
	if (one.NodeId != 0) return HYBRID_JUNCTION_ROLE_SHARED_NODE;
	return HYBRID_JUNCTION_ROLE_INCIDENTAL;
}


void HybridInteractionSet::Reset(void)
{
	Interactions.Delete_All();
	Issues.Delete_All();
	MergeDiverge.Delete_All();
	Landings.Delete_All();
	Constraints.Delete_All();
	ConstraintsCompiled = false;
	StationsEvaluated	= 0;
	SweepSamples		= 0;
	RerouteRequests	= 0;
	Checksum				= 0;
}


const HybridInteraction * HybridInteractionSet::Find(unsigned int stable_id) const
{
	for (int i = 0; i < Interactions.Count(); ++i) {
		if (Interactions[i].StableId == stable_id) return &Interactions[i];
	}
	return nullptr;
}


const HybridInteraction * HybridInteractionSet::Find_For_Window(unsigned int edge_id,
																					 unsigned int window_id) const
{
	for (int i = 0; i < Interactions.Count(); ++i) {
		if (Interactions[i].EdgeId != edge_id) continue;
		if (Interactions[i].WindowId != window_id) continue;
		return &Interactions[i];
	}
	return nullptr;
}


int HybridInteractionSet::Count_Of_Method(HybridInteractionMethod method) const
{
	int count = 0;
	for (int i = 0; i < Interactions.Count(); ++i) {
		if (Interactions[i].Method == method) count++;
	}
	return count;
}


int HybridInteractionSet::Count_Of_Method_On_Obstruction(HybridInteractionMethod method,
																			HybridObstructionKind obstruction) const
{
	int count = 0;
	for (int i = 0; i < Interactions.Count(); ++i) {
		if (Interactions[i].Method != method) continue;
		if (Interactions[i].Obstruction != obstruction) continue;
		count++;
	}
	return count;
}


HybridInteractionParams::HybridInteractionParams(void) :
	WorldMinX(0.0f),WorldMinY(0.0f),WorldWidth(0.0f),WorldHeight(0.0f),
	MaxCutDepth(0.0f),MaxFillHeight(0.0f),RiverWidth(0.0f),
	AllowFords(false),FordCapabilityAvailable(false),TunnelsEnabled(false),TunnelAvailable(false),
	WorldUnitsPerMeter(1.0f),HeavyVehicleWidth(0.0f),HeavyVehicleHeight(0.0f),HarvesterWidth(0.0f),LightVehicleWidth(0.0f),
	LightVehicleLength(0.0f),HeavyVehicleLength(0.0f),HarvesterLength(0.0f),
	MinVehicleLaneClearance(0.0f),TierStep(1.0f),BridgeWaterClearanceMargin(0.0f),
	TerrainCellSize(0.0f),TerrainOriginX(0.0f),TerrainOriginY(0.0f),
	MaxLocalRetriesPerStage(8),MaxStationsPerCrossing(STATION_CAP),MaxProblemsPerEdge(12),
	Arena(nullptr),Counters(nullptr)
{
	for (int i = 0; i < HYBRID_ROAD_CLASS_COUNT; ++i) {
		BridgeForClass[i]			= -1;
		BridgeRequiredWidth[i]	= 0.0f;
	}
}


/***********************************************************************************************
**	The two shared Section-12 measures.
***********************************************************************************************/

float Hybrid_Resolve_Bridge_Max_Span(float definition_max_span,float planner_max_span_length)
{
	/*
	**	The definition first, the planner second.  A definition that supplies no span of its own is not a
	**	missing metric -- `BridgePlannerParams::MaxSpanLength` is the plan's named fallback source for
	**	exactly that case, and it is what the preflight cache already carries.  Only when neither supplies
	**	one is there nothing to resolve, and preflight has refused the request long before this point.
	*/
	if (definition_max_span > 0.0f) return definition_max_span;
	return planner_max_span_length;
}


float Hybrid_Overpass_Clear_Gap(float heavy_vehicle_height,float world_units_per_meter)
{
	/*
	**	Section 12.3, exactly:
	**		safety = max(0.35 * world_units_per_meter, 0.10 * heavy_vehicle_height)
	**		minimum_clear_vertical_gap = heavy_vehicle_height + safety
	*/
	const float safety = Max2(OVERPASS_SAFETY_METERS * world_units_per_meter,
									  OVERPASS_SAFETY_HEIGHT_FRACTION * heavy_vehicle_height);
	return heavy_vehicle_height + safety;
}


float Hybrid_Bridge_Clear_Width(HybridRoadClass road_class,float lane_width,float swept_vehicle_width,
											float min_lane_clearance)
{
	/*
	**	Section 12.4, exactly:
	**		max(connected road carriageway width,
	**		    required swept vehicle width + 2*fairness.min_vehicle_lane_clearance)
	*/
	return Max2(Hybrid_Carriageway_Width(road_class,lane_width),
					swept_vehicle_width + (2.0f * min_lane_clearance));
}


/***********************************************************************************************
**	The checksum.
***********************************************************************************************/

/***********************************************************************************************
**	Phase 10 and Phase 11 together, with the plan's next-envelope reroute between them.
**
**	Every method order in Section 8 Phase 11 ends the same way: when no method can compile a crossing, the
**	road may be given one more Phase-10 corridor envelope and tried again.  Phase 11 raises the request by
**	naming the edge in `HybridInteractionResult::RerouteEdgeId`; until this owner existed nothing read it, so
**	the last step of every order never ran and one uncrossable crossing failed the whole world.  Measured
**	over 200 seeds of 7 archetypes, that cost 16 of 79 solved worlds.
**
**	The loop is deliberately narrow:
**
**	  - the next envelope is strictly the one after the envelope that produced the route Phase 11 refused,
**	    so a reroute always moves outward.  Envelope 0 is never reconsidered and never returned to, and a
**	    floor that is already at or past the one being asked for ends the attempt rather than repeating it --
**	    the same route state cannot come round twice;
**	  - `next > 2` means the plan's three envelopes are spent, and the attempt fails with the interaction
**	    failure Phase 11 named, not with a reroute error;
**	  - Phase 9 is never re-run and the topology is never touched.  A frozen obligation stays on its own
**	    obstruction; only the corridor the road may use inside it widens.
**
**	The normal path runs Phase 10 once, outside this function, and what it froze is what Phase 11 consumes.
**	Phase 10 is only ever re-entered here after a reroute request, so a world that needs none pays for one
**	routing pass.
**
**	Phase 10 is re-run rather than patched, and the whole of it rather than the one edge, because a route is
**	routed to avoid the routes accepted before it: moving one edge's spline makes the later edges' avoidance
**	data stale, and splicing a new spline into the old set would leave exactly the half-updated state this
**	has to avoid.  Re-running is also what makes the step transactional for free -- the accepted set is one
**	Phase-10 run and one Phase-11 run over it, so no old crossing record, interaction, unplanned crossing or
**	checksum contribution can survive, and every edge but the rerouted one is byte-identical because the
**	search is deterministic and only that edge's starting envelope changed.
***********************************************************************************************/

bool Hybrid_Solve_Interactions_With_Reroute(HybridRouteParams & route_params,
														const HybridInteractionParams & interaction_params,
														const HybridMapIdentity & identity,
														const HybridCoarseMatrices & matrices,
														const HybridLandformSet & landforms,
														const HybridHydrologySet & hydrology,
														const HybridAnchorSet & anchors,const HybridTopology & topology,
														HybridRouteHeightSampleFunc sample,void * context,
														HybridRouteSet * routes,HybridRouteResult * route_result,
														HybridInteractionSet * interactions,
														HybridInteractionResult * interaction_result,
														int * reroutes_out,int * phase10_runs_out)
{
	if ((routes == nullptr) || (interactions == nullptr)) return false;
	if (reroutes_out != nullptr) *reroutes_out = 0;

	//	The frozen set the caller built is the first Phase-10 execution.
	int phase10_runs = 1;
	if (phase10_runs_out != nullptr) *phase10_runs_out = phase10_runs;

	/*
	**	Every pass that does not return raises one edge's envelope floor by at least one, and a floor never
	**	falls and never passes 2, so the loop cannot run more than twice per realized edge.  That is the
	**	termination proof; the explicit bound below is only its arithmetic.
	*/
	int realized = 0;
	for (int e = 0; e < topology.Edges.Count(); ++e) {
		if (topology.Edges[e].Is_Realized()) realized++;
	}
	const int attempts = 1 + (HYBRID_MAX_REROUTES_PER_EDGE * realized);

	for (int attempt = 0; attempt < attempts; ++attempt) {

		/*
		**	The first attempt consumes the routes as handed over; every later one consumes the set the reroute
		**	just rebuilt.  Either way Phase 11 is reading a complete, frozen Phase-10 result.
		*/
		if (attempt > 0) {
			//	An execution is an execution whether or not it routes: count it before asking.
			phase10_runs++;
			if (phase10_runs_out != nullptr) *phase10_runs_out = phase10_runs;
			if (!Hybrid_Build_Routes(route_params,identity,matrices,landforms,hydrology,anchors,topology,
											 sample,context,routes,route_result)) return false;
		}

		if (Hybrid_Build_Interactions(interaction_params,identity,matrices,landforms,hydrology,anchors,
												topology,*routes,sample,context,interactions,
												interaction_result)) return true;

		const unsigned int edge_id = (interaction_result != nullptr) ? interaction_result->RerouteEdgeId : 0;
		if (edge_id == 0) return false;

		//	The envelope the refused route was accepted in, and therefore the one to start after.
		const HybridRoute * refused = nullptr;
		for (int r = 0; r < routes->Routes.Count(); ++r) {
			if (routes->Routes[r].EdgeId != edge_id) continue;
			refused = &routes->Routes[r];
			break;
		}
		if (refused == nullptr) return false;

		const int next = refused->Envelope + 1;
		if (next > 2) return false;

		//	Never narrower, and never the same twice.
		int slot = -1;
		for (int f = 0; f < route_params.EnvelopeFloors.Count(); ++f) {
			if (route_params.EnvelopeFloors[f].EdgeId != edge_id) continue;
			slot = f;
			break;
		}
		if ((slot >= 0) && (route_params.EnvelopeFloors[slot].MinEnvelope >= next)) return false;

		if (slot >= 0) {
			route_params.EnvelopeFloors[slot].MinEnvelope = next;
		} else {
			HybridRouteEnvelopeFloor floor;
			floor.EdgeId		= edge_id;
			floor.MinEnvelope	= next;
			route_params.EnvelopeFloors.Add(floor);
		}

		if (reroutes_out != nullptr) (*reroutes_out)++;
	}

	return false;
}


unsigned int Hybrid_Interaction_Checksum(const HybridInteractionSet & set,unsigned int seed)
{
	HybridHashStream stream(seed);
	stream.Add_Int(set.Interactions.Count());

	for (int i = 0; i < set.Interactions.Count(); ++i) {

		const HybridInteraction & one = set.Interactions[i];
		stream.Add_Uint(one.StableId);
		stream.Add_Uint(one.EdgeId);
		stream.Add_Uint(one.OtherEdgeId);
		stream.Add_Uint(one.NodeId);
		stream.Add_Int((int)one.Obstruction);
		stream.Add_Uint(one.ObstructionRecordId);
		stream.Add_Uint(one.WindowId);
		stream.Add_Bool(one.Mandatory);
		stream.Add_Int((int)one.Method);
		stream.Add_Int((int)one.FallbackFrom);
		stream.Add_Int((int)one.RoadClass);
		stream.Add_Length(one.Station.X);
		stream.Add_Length(one.Station.Y);
		stream.Add_Length(one.ApproachA.X);
		stream.Add_Length(one.ApproachA.Y);
		stream.Add_Length(one.ApproachB.X);
		stream.Add_Length(one.ApproachB.Y);
		stream.Add_Length(one.Span);
		stream.Add_Length(one.ApproachReserve);
		stream.Add_Int(one.BridgeKind);
		stream.Add_Int(one.TunnelModule);
		stream.Add_Length(one.TunnelLength);
		stream.Add_Length(one.DeckBottomZ);
		stream.Add_Length(one.DeckTopZ);
		stream.Add_Length(one.LowerSurfaceZ);
		stream.Add_Length(one.RampLength);
		stream.Add_Bool(one.UpperIsThisEdge);
		stream.Add_Length(one.CutDepth);
		stream.Add_Length(one.DeckStartZ);
		stream.Add_Length(one.DeckEndZ);
		stream.Add_Length(one.DeckReachA);
		stream.Add_Length(one.DeckReachB);
		stream.Add_Int(one.FirstPoint);
		stream.Add_Int(one.LastPoint);
		stream.Add_Int(one.Envelope);
		stream.Add_Int(one.Path.DepartPoint);
		stream.Add_Int(one.Path.RejoinPoint);
		stream.Add_Length(one.Path.Length);
		stream.Add_Int(one.Path.Points.Count());
		for (int p = 0; p < one.Path.Points.Count(); ++p) {
			stream.Add_Length(one.Path.Points[p].X);
			stream.Add_Length(one.Path.Points[p].Y);
		}
		stream.Add_Int((int)Hybrid_Interaction_Junction_Role(one));
		stream.Add_Length(one.JunctionZ);
	}

	//	Section 12.6.15: the merge/diverge records and their widened-profile samples.
	stream.Add_Int(set.MergeDiverge.Count());
	for (int i = 0; i < set.MergeDiverge.Count(); ++i) {
		const HybridMergeDiverge & md = set.MergeDiverge[i];
		stream.Add_Uint(md.StableId);
		stream.Add_Uint(md.InteractionId);
		stream.Add_Uint(md.RampEdgeId);
		stream.Add_Uint(md.HighwayEdgeId);
		stream.Add_Uint(md.InterchangeId);
		stream.Add_Int((int)md.Role);
		stream.Add_Int((int)md.Side);
		stream.Add_Length(md.Interval.GoreStation);
		stream.Add_Length(md.Interval.From);
		stream.Add_Length(md.Interval.To);
		stream.Add_Length(md.Interval.FullFrom);
		stream.Add_Length(md.Interval.FullTo);
		stream.Add_Length(md.Interval.TaperFrom);
		stream.Add_Length(md.Interval.TaperTo);
		stream.Add_Int(md.Samples.Count());
		for (int s = 0; s < md.Samples.Count(); ++s) {
			stream.Add_Length(md.Samples[s].Station);
			stream.Add_Bool(md.Samples[s].Full);
			stream.Add_Length(md.Samples[s].U);
			stream.Add_Length(md.Samples[s].D);
			stream.Add_Length(md.Samples[s].H);
			stream.Add_Length(md.Samples[s].E);
			stream.Add_Length(md.Samples[s].Z);
		}
	}
	stream.Add_Bool(set.ConstraintsCompiled);
	stream.Add_Int(set.Constraints.Count());
	for (int i = 0; i < set.Constraints.Count(); ++i) {
		const HybridVerticalConstraint & one = set.Constraints[i];
		stream.Add_Uint(one.StableId);
		stream.Add_Uint(one.RoadEdgeId);
		stream.Add_Uint(one.InteractionId);
		stream.Add_Uint(one.OwnerEdgeId);
		stream.Add_Int((int)one.Kind);
		stream.Add_Length(one.From);
		stream.Add_Length(one.To);
		stream.Add_Bool(one.Level);
		stream.Add_Bool(one.EndsFixed);
		stream.Add_Length(one.StartZ);
		stream.Add_Length(one.EndZ);
		stream.Add_Length(one.Reach);
	}
	stream.Add_Int(set.Landings.Count());
	for (int i = 0; i < set.Landings.Count(); ++i) {
		const HybridPinLanding & landing = set.Landings[i];
		stream.Add_Uint(landing.StableId);
		stream.Add_Int((int)landing.PinKind);
		stream.Add_Length(landing.Position.X);
		stream.Add_Length(landing.Position.Y);
		stream.Add_Length(landing.Z);
		stream.Add_Length(landing.Length);
		stream.Add_Uint(landing.InteractionId);
	}
	return stream.Get();
}


/***********************************************************************************************
**	A road's realized horizontal centerline.
***********************************************************************************************/
bool Hybrid_Realized_Route(const HybridRoute & route,const HybridInteractionSet & set,HybridRoute * out)
{
	if (out == nullptr) return false;
	*out = route;
	if ((route.Points.Count() < 2) || (route.Stations.Count() != route.Points.Count())) return false;

	//	This road's own crossings that carry a path, in the order the road meets them.
	DynamicVectorClass<int> crossings;
	for (int i = 0; i < set.Interactions.Count(); ++i) {
		const HybridInteraction & one = set.Interactions[i];
		if (one.EdgeId != route.EdgeId) continue;
		if ((one.Method == HYBRID_METHOD_AT_GRADE_JUNCTION) || (one.Method == HYBRID_METHOD_OVERPASS)) continue;
		if (one.Path.Points.Count() < 2) continue;
		crossings.Add(i);
	}
	for (int i = 1; i < crossings.Count(); ++i) {
		for (int j = i; j > 0; --j) {
			if (set.Interactions[crossings[j]].Path.DepartPoint >=
				 set.Interactions[crossings[j - 1]].Path.DepartPoint) break;
			const int swap = crossings[j];
			crossings[j]		= crossings[j - 1];
			crossings[j - 1]	= swap;
		}
	}

	out->Points.Delete_All();
	out->Stations.Delete_All();
	out->Structural.Delete_All();

	int next = 0;													// the next preliminary sample not yet copied
	for (int c = 0; c < crossings.Count(); ++c) {

		const HybridInteraction & one = set.Interactions[crossings[c]];
		const HybridCrossingPath & path = one.Path;
		//	Back-to-back crossings may share the one sample where the first rejoins and the second departs.
		const bool shared = (c > 0) && (path.DepartPoint == (next - 1));
		if (((path.DepartPoint < next) && !shared) || (path.RejoinPoint >= route.Points.Count()) ||
			 (path.RejoinPoint <= path.DepartPoint)) {
			*out = route;
			return false;												// two crossings claim one stretch: not ours to merge
		}

		for (int p = next; p < path.DepartPoint; ++p) out->Points.Add(route.Points[p]);

		const int path_first = shared ? (out->Points.Count() - 1) : out->Points.Count();
		for (int p = shared ? 1 : 0; p < path.Points.Count(); ++p) out->Points.Add(path.Points[p]);

		//	The crossing's own ends, which are points of its path, re-indexed onto the realized points.
		HybridRouteStructuralSpan span;
		span.WindowId	= one.WindowId;
		span.Ingress	= one.ApproachA;
		span.Egress		= one.ApproachB;
		span.FirstPoint= path_first;
		span.LastPoint	= out->Points.Count() - 1;
		float best_a = 3.4e38f;
		float best_b = 3.4e38f;
		for (int p = path_first; p < out->Points.Count(); ++p) {
			const float to_a = (out->Points[p] - one.ApproachA).Length();
			const float to_b = (out->Points[p] - one.ApproachB).Length();
			if (to_a < best_a) {
				best_a = to_a;
				span.FirstPoint = p;
			}
			if (to_b < best_b) {
				best_b = to_b;
				span.LastPoint = p;
			}
		}
		if (one.WindowId != 0) out->Structural.Add(span);

		next = path.RejoinPoint + 1;
	}
	for (int p = next; p < route.Points.Count(); ++p) out->Points.Add(route.Points[p]);

	float length = 0.0f;
	for (int p = 0; p < out->Points.Count(); ++p) {
		if (p > 0) length += (out->Points[p] - out->Points[p - 1]).Length();
		out->Stations.Add(length);
	}
	out->Length = Hybrid_Quantize_Position(length);

	out->BoundsMin = out->Points[0];
	out->BoundsMax = out->Points[0];
	for (int p = 1; p < out->Points.Count(); ++p) {
		out->BoundsMin.X = Min2(out->BoundsMin.X,out->Points[p].X);
		out->BoundsMin.Y = Min2(out->BoundsMin.Y,out->Points[p].Y);
		out->BoundsMax.X = Max2(out->BoundsMax.X,out->Points[p].X);
		out->BoundsMax.Y = Max2(out->BoundsMax.Y,out->Points[p].Y);
	}
	return true;
}


bool Hybrid_Realized_Physical_Road(const HybridRouteSet & routes,const HybridInteractionSet & set,
											  unsigned int edge_id,HybridRoute * out,DynamicVectorClass<unsigned int> * edges_out)
{
	if (out == nullptr) return false;
	if (edges_out != nullptr) edges_out->Delete_All();

	const HybridTrunkRoad * trunk = routes.Find_Trunk(edge_id);
	if (trunk == nullptr) {
		const HybridRoute * route = routes.Find(edge_id);
		if (route == nullptr) return false;
		if (edges_out != nullptr) edges_out->Add(edge_id);
		return Hybrid_Realized_Route(*route,set,out);
	}

	for (int e = 0; e < trunk->EdgeIds.Count(); ++e) {

		const HybridRoute * route = routes.Find(trunk->EdgeIds[e]);
		if (route == nullptr) return false;
		HybridRoute piece;
		if (!Hybrid_Realized_Route(*route,set,&piece)) return false;
		if (edges_out != nullptr) edges_out->Add(trunk->EdgeIds[e]);

		if (e == 0) {
			*out = piece;
			out->EdgeId = edge_id;
			out->Points.Delete_All();
			out->Stations.Delete_All();
			out->Structural.Delete_All();
			out->Cells.Delete_All();
			out->Waypoints.Delete_All();
		}

		//	Consecutive slices share the sample they were cut at.
		const int skip = (out->Points.Count() > 0) ? 1 : 0;
		const int base = out->Points.Count() - skip;
		for (int q = skip; q < piece.Points.Count(); ++q) out->Points.Add(piece.Points[q]);
		for (int s = 0; s < piece.Structural.Count(); ++s) {
			HybridRouteStructuralSpan span = piece.Structural[s];
			span.FirstPoint	+= base;
			span.LastPoint		+= base;
			out->Structural.Add(span);
		}
		for (int c = 0; c < piece.Cells.Count(); ++c) out->Cells.Add(piece.Cells[c]);
		for (int w = 0; w < piece.Waypoints.Count(); ++w) out->Waypoints.Add(piece.Waypoints[w]);
	}

	float length = 0.0f;
	for (int q = 0; q < out->Points.Count(); ++q) {
		if (q > 0) length += (out->Points[q] - out->Points[q - 1]).Length();
		out->Stations.Add(length);
	}
	out->Length = Hybrid_Quantize_Position(length);
	if (out->Points.Count() < 2) return false;
	out->BoundsMin = out->Points[0];
	out->BoundsMax = out->Points[0];
	for (int q = 1; q < out->Points.Count(); ++q) {
		out->BoundsMin.X = Min2(out->BoundsMin.X,out->Points[q].X);
		out->BoundsMin.Y = Min2(out->BoundsMin.Y,out->Points[q].Y);
		out->BoundsMax.X = Max2(out->BoundsMax.X,out->Points[q].X);
		out->BoundsMax.Y = Max2(out->BoundsMax.Y,out->Points[q].Y);
	}
	return true;
}


/***********************************************************************************************
**	The solver's own working state.
***********************************************************************************************/

/***********************************************************************************************
**	Section 12.6.10: the interchanges Phase 10 realized, as Phase 11 sees them.
***********************************************************************************************/

//	One realized interchange: its record, its anchor and each access side's footprint polygon.
struct InterchangeView
{
	const HybridInterchange *		Record;
	Vector2								X;
	DynamicVectorClass<Vector2>	Footprint[HYBRID_INTERCHANGE_SIDE_COUNT];	// empty on a side without access

	bool	operator == (const InterchangeView & that) const	{ return Record == that.Record; }
	bool	operator != (const InterchangeView & that) const	{ return !(*this == that); }
};


/*
**	One ramp's gore reserve (12.6.7): along the side frame over Hybrid_Gore_Reserve_Extent, laterally across the
**	highway from its far roadbed edge out to `E_aux`, and the independent ramp's roadbed over its `2W` gore
**	shoulder transition.
*/
struct GoreReserveView
{
	unsigned int			InterchangeId;
	unsigned int			RampEdgeId;
	Vector2					X;
	Vector2					Tau;				// the side frame: x along tau_s, y along n_s
	Vector2					Normal;
	float						X0;
	float						X1;
	float						Y0;
	float						Y1;
	const HybridRoute *	Ramp;				// L_s -> gore for an ON_RAMP and an OFF_RAMP alike

	bool	operator == (const GoreReserveView & that) const	{ return RampEdgeId == that.RampEdgeId; }
	bool	operator != (const GoreReserveView & that) const	{ return !(*this == that); }
};


//	12.6.7's side frame: tau_RIGHT = a, tau_LEFT = -a, n_s to the right of tau_s.
static void Side_Frame(const Vector2 & axis,int side,Vector2 * tau,Vector2 * normal)
{
	*tau		= (side == HYBRID_INTERCHANGE_SIDE_RIGHT) ? axis : Vector2(-axis.X,-axis.Y);
	*normal	= (side == HYBRID_INTERCHANGE_SIDE_RIGHT) ? Vector2(axis.Y,-axis.X) : Vector2(-axis.Y,axis.X);
}


static Vector2 Side_Point(const Vector2 & x_point,const Vector2 & axis,int side,float x,float y)
{
	Vector2 tau;
	Vector2 normal;
	Side_Frame(axis,side,&tau,&normal);
	return x_point + (tau * x) + (normal * y);
}


//	A ramp or crossing edge: constructed by group 1b, it has no Phase-10 envelope and is never rerouted.
static bool Is_Interchange_Element(const HybridTopologyEdge & edge)
{
	return (edge.Kind == HYBRID_EDGE_HIGHWAY_RAMP) || (edge.Kind == HYBRID_EDGE_INTERCHANGE_CROSSING);
}


/*
**	D-027: a compiled solution Phase 11 has rejected after the fact, because its structural pin cannot be realized on
**	the terrain grid even with a landing.  The re-run skips it, so its own fallback order -- the next station, the
**	permitted swap, the next method, a reroute -- decides what happens instead.
*/
struct SolutionKey
{
	SolutionKey(void) :
		Method(HYBRID_METHOD_BRIDGE),EdgeId(0),OtherEdgeId(0),WindowId(0),NodeId(0),Station(0.0f,0.0f),
		Problem(HYBRID_PROBLEM_PIN_LANDING) { }

	bool	operator == (const SolutionKey & that) const
	{
		return (Method == that.Method) && (EdgeId == that.EdgeId) && (OtherEdgeId == that.OtherEdgeId) &&
				 (WindowId == that.WindowId) && (NodeId == that.NodeId) && (Station == that.Station);
	}
	bool	operator != (const SolutionKey & that) const	{ return !(*this == that); }

	HybridInteractionMethod	Method;
	unsigned int				EdgeId;
	unsigned int				OtherEdgeId;
	unsigned int				WindowId;
	unsigned int				NodeId;
	Vector2						Station;
	HybridInteractionProblem	Problem;			// why it was rejected (not part of its identity)
};


struct InteractionBuilder
{
	const HybridInteractionParams *	P;
	const HybridMapIdentity *			Identity;
	const HybridCoarseMatrices *		Matrices;
	const HybridLandformSet *			Landforms;
	const HybridHydrologySet *			Hydrology;
	const HybridAnchorSet *				Anchors;
	const HybridTopology *				Topology;
	const HybridRouteSet *				Routes;
	HybridRouteHeightSampleFunc		Sample;
	void *									Context;
	HybridInteractionSet *				Out;

	unsigned int							NextStableId;

	//	D-027: the solutions earlier runs rejected, and the one this run rejects.
	const DynamicVectorClass<SolutionKey> *	Excluded;
	DynamicVectorClass<SolutionKey> *			Rejected;

	/*
	**	Was this solution rejected?  The same method, window and station; for a road x road solution the same pair of
	**	edges -- in order when `ordered` (an overpass: its swap is a different solution), either way round otherwise.
	*/
	bool	Is_Excluded(HybridInteractionMethod method,unsigned int edge,unsigned int other,unsigned int window,bool ordered,
							const Vector2 & station,HybridInteractionProblem * problem_out = nullptr) const
	{
		if (Excluded == nullptr) return false;
		for (int i = 0; i < Excluded->Count(); ++i) {
			const SolutionKey & key = (*Excluded)[i];
			if ((key.Method != method) || (key.WindowId != window)) continue;
			const bool same_pair = ((key.EdgeId == edge) && (key.OtherEdgeId == other)) ||
										  (!ordered && (key.EdgeId == other) && (key.OtherEdgeId == edge));
			if (!same_pair) continue;
			if ((key.Station - station).Length() > (2.0f * P->Constants.Q)) continue;
			if (problem_out != nullptr) *problem_out = key.Problem;
			return true;
		}
		return false;
	}

	//	Section 12.6.10: the interchanges Phase 10 realized, and every gore reserve of them.
	DynamicVectorClass<InterchangeView>	Interchanges;
	DynamicVectorClass<GoreReserveView>	Gores;

	//	Which test refused each station of the conflict being solved, for the refusal detail.
	int										Rejects[HYBRID_PROBLEM_COUNT];

	void	Clear_Rejects(void)
	{
		for (int i = 0; i < HYBRID_PROBLEM_COUNT; ++i) Rejects[i] = 0;
	}

	void	Note_Reject(HybridInteractionProblem problem)
	{
		if ((problem > HYBRID_PROBLEM_NONE) && (problem < HYBRID_PROBLEM_COUNT)) Rejects[problem]++;
	}

	bool	Height_At(const Vector2 & world,float * height_out) const
	{
		if (Sample == nullptr) return false;
		return Sample(world,height_out,Context);
	}
};


/*
**	Q-028: the native grid's grade budget for a road of `road_class` carrying `edges`, its decks and bores `decks`, on the
**	class's own paved edges (Hybrid_Apply_Grade_Budget): Phase 11's own gradings take the realization-aware grade Phase 12
**	grades with, and the compiled plan then takes it on the road's realized edges.  Null without a grid.
*/
static const HybridGradeBudget * Grade_Budget(const InteractionBuilder & b,HybridRoadClass road_class,
															 const DynamicVectorClass<unsigned int> & edges,
															 const DynamicVectorClass<HybridProfileStructure> & decks,HybridGradeBudget * out)
{
	const HybridInteractionParams & p = *b.P;
	if (!(p.TerrainCellSize > 0.0f)) return nullptr;
	const HybridConstants & k = p.Constants;
	unsigned int capabilities = 0;
	for (int e = 0; e < edges.Count(); ++e) {
		const HybridTopologyEdge * edge = b.Topology->Find_Edge(edges[e]);
		if (edge != nullptr) capabilities |= edge->Capabilities;
	}
	out->CellSize	= p.TerrainCellSize;
	out->OriginX	= p.TerrainOriginX;
	out->OriginY	= p.TerrainOriginY;
	out->Spacing	= Hybrid_Profile_Sample_Spacing(k);
	const float lengths[HYBRID_ENVELOPE_COUNT] = { p.LightVehicleLength,p.HeavyVehicleLength,p.HarvesterLength };
	const float widths[HYBRID_ENVELOPE_COUNT] = { p.LightVehicleWidth,p.HeavyVehicleWidth,p.HarvesterWidth };
	for (int e = 0; e < HYBRID_ENVELOPE_COUNT; ++e) {
		const bool swept = Hybrid_Envelope_Swept(capabilities,e);
		out->Length[e]	= swept ? lengths[e] : 0.0f;
		out->Width[e]	= swept ? widths[e] : 0.0f;
	}
	out->Left	= -Hybrid_Road_Inner_Edge_Offset(road_class,k.W);
	out->Right	= Hybrid_Road_Outer_Edge_Offset(road_class,k.W);
	out->Decks	= decks;
	return out;
}


/***********************************************************************************************
**	Section 12.6.10's geometry: the realized interchanges, their gore reserves and their footprints.
***********************************************************************************************/

//	Every interchange Phase 10 realized -- active, not withdrawn -- with its footprints and its gore reserves.
static void Collect_Interchanges(InteractionBuilder & b)
{
	const HybridConstants & k = b.P->Constants;
	HybridInterchangeConstants c;
	Hybrid_Interchange_Constants(k.W,&c);

	for (int i = 0; i < b.Topology->Interchanges.Count(); ++i) {

		const HybridInterchange & ic = b.Topology->Interchanges[i];
		if ((ic.Status != HYBRID_INTERCHANGE_ACTIVE) || b.Routes->Interchange_Withdrawn(ic.StableId)) continue;
		const HybridTopologyNode * anchor = b.Topology->Find_Node(ic.AnchorNode);
		if (anchor == nullptr) continue;

		InterchangeView view;
		view.Record	= &ic;
		view.X		= anchor->Position;
		for (int side = 0; side < HYBRID_INTERCHANGE_SIDE_COUNT; ++side) {
			if ((ic.AccessSides & (1u << side)) != 0) Hybrid_Interchange_Footprint(k,ic,view.X,side,&view.Footprint[side]);
		}
		b.Interchanges.Add(view);

		for (int e = 0; e < ic.RampEdges.Count(); ++e) {
			const HybridTopologyEdge * ramp = b.Topology->Find_Edge(ic.RampEdges[e]);
			if (ramp == nullptr) continue;
			GoreReserveView gore;
			gore.InterchangeId	= ic.StableId;
			gore.RampEdgeId		= ramp->StableId;
			gore.X					= view.X;
			Side_Frame(ic.Axis,ramp->RampSide,&gore.Tau,&gore.Normal);
			Hybrid_Gore_Reserve_Extent(c,ic.GoreOffset,ramp->RampRole,&gore.X0,&gore.X1);
			gore.Y0					= -c.EHwy;
			gore.Y1					= c.EAux;
			gore.Ramp				= b.Routes->Find(ramp->StableId);
			b.Gores.Add(gore);
		}
	}
}


//	Does a disc of `radius` about `point` touch any active gore reserve?
static bool Point_In_Gore_Reserve(const InteractionBuilder & b,const Vector2 & point,float radius)
{
	if (b.Gores.Count() == 0) return false;
	const HybridConstants & k = b.P->Constants;
	HybridInterchangeConstants c;
	Hybrid_Interchange_Constants(k.W,&c);

	for (int g = 0; g < b.Gores.Count(); ++g) {

		const GoreReserveView & gore = b.Gores[g];
		const Vector2 offset = point - gore.X;
		const float x = (offset.X * gore.Tau.X) + (offset.Y * gore.Tau.Y);
		const float y = (offset.X * gore.Normal.X) + (offset.Y * gore.Normal.Y);
		if ((x >= (gore.X0 - radius)) && (x <= (gore.X1 + radius)) && (y >= (gore.Y0 - radius)) && (y <= (gore.Y1 + radius))) {
			return true;
		}

		//	The independent ramp's roadbed over its gore shoulder transition, the gore being its last sample.
		if ((gore.Ramp == nullptr) || (gore.Ramp->Points.Count() < 2)) continue;
		const DynamicVectorClass<Vector2> & points = gore.Ramp->Points;
		const DynamicVectorClass<float> & stations = gore.Ramp->Stations;
		const float total = stations[stations.Count() - 1];
		for (int i = points.Count() - 1; i >= 0; --i) {
			const float from_gore = total - stations[i];
			if (from_gore > c.GoreReserveMargin) break;
			float inner = 0.0f;
			float outer = 0.0f;
			Hybrid_Ramp_Gore_Shoulders(c,from_gore,&inner,&outer);
			if ((points[i] - point).Length() <= ((0.5f * k.W) + Max2(inner,outer) + radius)) return true;
		}
	}
	return false;
}


//	Does the stretch [from, to] of a station-ordered polyline, as a band of `radius` either side, touch one?
static bool Polyline_Hits_Gore_Reserve(const InteractionBuilder & b,const DynamicVectorClass<Vector2> & points,
													const DynamicVectorClass<float> & stations,float from,float to,float radius)
{
	if ((b.Gores.Count() == 0) || (points.Count() < 2)) return false;
	const HybridConstants & k = b.P->Constants;
	const float spacing = Max2(Min2(k.W / 3.0f,0.25f * k.A),k.Q);
	const float length = Max2(to - from,0.0f);
	const int steps = (int)::ceil((double)(length / spacing));
	const int count = (steps > 0) ? steps : 1;
	for (int i = 0; i <= count; ++i) {
		const Vector2 point = Hybrid_Polyline_Point_At(points,stations,from + (length * ((float)i / (float)count)));
		if (Point_In_Gore_Reserve(b,point,radius)) return true;
	}
	return false;
}


static bool Segment_Hits_Gore_Reserve(const InteractionBuilder & b,const Vector2 & from,const Vector2 & to,float radius)
{
	if (b.Gores.Count() == 0) return false;
	DynamicVectorClass<Vector2> points;
	DynamicVectorClass<float> stations;
	points.Add(from);
	points.Add(to);
	stations.Add(0.0f);
	stations.Add((to - from).Length());
	return Polyline_Hits_Gore_Reserve(b,points,stations,0.0f,stations[1],radius);
}


//	Is `point` inside an active interchange footprint polygon, or within `margin` of one?
static bool Near_Interchange_Footprint(const InteractionBuilder & b,const Vector2 & point,float margin)
{
	for (int i = 0; i < b.Interchanges.Count(); ++i) {
		for (int side = 0; side < HYBRID_INTERCHANGE_SIDE_COUNT; ++side) {
			const DynamicVectorClass<Vector2> & polygon = b.Interchanges[i].Footprint[side];
			const int count = polygon.Count();
			if (count < 3) continue;
			bool inside = false;
			for (int p = 0, q = count - 1; p < count; q = p++) {
				const Vector2 & pi = polygon[p];
				const Vector2 & pj = polygon[q];
				if (((pi.Y > point.Y) != (pj.Y > point.Y)) &&
					 (point.X < ((((pj.X - pi.X) * (point.Y - pi.Y)) / (pj.Y - pi.Y)) + pi.X))) inside = !inside;
				if (Distance_To_Segment(point,pj,pi,nullptr) <= margin) return true;
			}
			if (inside) return true;
		}
	}
	return false;
}


//	A solution one of whose roads is an interchange element.
static bool Interchange_Owned(const InteractionBuilder & b,const HybridInteraction & one)
{
	const HybridTopologyEdge * edge = b.Topology->Find_Edge(one.EdgeId);
	const HybridTopologyEdge * other = b.Topology->Find_Edge(one.OtherEdgeId);
	return ((edge != nullptr) && Is_Interchange_Element(*edge)) || ((other != nullptr) && Is_Interchange_Element(*other));
}


static bool Fail(HybridInteractionResult * result,HybridInteractionFailure failure,const char * detail)
{
	if (result != nullptr) {
		result->Failure	= failure;
		result->Detail		= (detail != nullptr) ? detail : "";
	}
	return false;
}


/*
**	Record one unsolved conflict, and refuse the attempt when one edge accumulates more than the
**	plan's limit of twelve.  The record is kept whether the attempt survives or not.
*/
static bool Note_Problem(InteractionBuilder & b,unsigned int edge_id,unsigned int other_edge_id,
								 HybridObstructionKind obstruction,unsigned int window_id,bool mandatory,
								 HybridInteractionProblem problem,int stations_tried,const Vector2 & station)
{
	HybridInteractionIssue issue;
	issue.EdgeId			= edge_id;
	issue.OtherEdgeId		= other_edge_id;
	issue.Obstruction		= obstruction;
	issue.WindowId			= window_id;
	issue.Mandatory		= mandatory;
	issue.Problem			= problem;
	issue.StationsTried	= stations_tried;
	issue.Station			= Quantized(station);
	b.Out->Issues.Add(issue);

	int on_edge = 0;
	for (int i = 0; i < b.Out->Issues.Count(); ++i) {
		if (b.Out->Issues[i].EdgeId == edge_id) on_edge++;
	}
	return on_edge <= b.P->MaxProblemsPerEdge;
}


/***********************************************************************************************
**	The station search and the common hard-valid tests.
***********************************************************************************************/

/*
**	Section 8 Phase 11's station interval, centred on the crossing the road actually made -- or, for a
**	mandatory obligation, on its frozen waypoint, because "Phase 11 never silently moves that obligation
**	to a different obstruction".
**
**	Stations march *along the obstruction*: `+/-max(8W,4S)` of the obstruction's own arc length at
**	`min(W,A/2)`, capped at exactly 64 uniformly spaced stations including both endpoints, then quantized and
**	deduplicated.  Each is measured where it stands -- the obstruction's direction and half width there --
**	because a river does not run along its window's tangent line, and a station stepped out along that line
**	is soon not on the river at all.
*/
static float Station_Half_Length(const HybridConstants & k)
{
	return Max2(STATION_HALF_LENGTH_W * k.W,STATION_HALF_LENGTH_S * k.S);
}


static void Build_Stations(const InteractionBuilder & b,const HybridCrossingWindow & window,const Vector2 & centre,
									DynamicVectorClass<HybridObstructionStation> * out)
{
	out->Delete_All();

	const HybridConstants & k = b.P->Constants;
	const float half_length = Station_Half_Length(k);
	const float spacing = Min2(k.W,0.5f * k.A);
	if ((half_length <= 0.0f) || (spacing <= 0.0f)) return;
	const int cap = (b.P->MaxStationsPerCrossing > 1) ? b.P->MaxStationsPerCrossing : STATION_CAP;

	/*
	**	Candidate 0 is the crossing problem's own anchor (Q-022): the frozen Phase-9 waypoint for an obligation,
	**	the road's own crossing for anything else, projected onto the obstruction.  Both are the window's own
	**	station here, because Phase 10 routes every crossing straight through its window's frozen approach
	**	points and the window's centre stands on the obstruction; its frame is Step 18's own.  The literal
	**	"exactly 64 uniformly spaced stations including both endpoints" is an even count and never samples the
	**	centre of the interval, so it could drop the crossing the road actually makes from its own candidates.
	*/
	HybridObstructionStation anchor;
	if ((centre - window.Center).Length() <= k.Q) {
		anchor.Centre		= window.Center;
		anchor.Tangent		= window.Tangent;
		anchor.HalfSpan	= 0.5f * window.Span;
		anchor.Offset		= 0.0f;
	} else {
		DynamicVectorClass<HybridObstructionStation> projected;
		Hybrid_Obstruction_Stations(*b.Hydrology,*b.Landforms,window,centre,0.0f,spacing,2,&projected);
		if (projected.Count() == 0) return;
		anchor = projected[0];
		anchor.Offset = 0.0f;
	}

	/*
	**	The rest of the interval: `-half_length .. +half_length` along the obstruction at `min(W,A/2)`, or
	**	exactly `cap` uniform stations including both endpoints when there would be more, then quantized and
	**	deduplicated.  The total never exceeds `cap`: a grid station on the anchor is the anchor, and when the
	**	grid is already full the grid station nearest the anchor that is not an endpoint gives way to it.
	*/
	DynamicVectorClass<HybridObstructionStation> grid;
	Hybrid_Obstruction_Stations(*b.Hydrology,*b.Landforms,window,anchor.Centre,half_length,spacing,cap,&grid);

	int replaced = -1;
	for (int s = 0; (s < grid.Count()) && (replaced < 0); ++s) {
		if ((grid[s].Centre - anchor.Centre).Length() <= k.Q) replaced = s;
	}
	if ((replaced < 0) && (grid.Count() >= cap)) {
		float nearest = 3.4e38f;
		for (int s = 1; s < (grid.Count() - 1); ++s) {
			const float distance = (float)fabs((double)grid[s].Offset);
			if (distance >= nearest) continue;
			nearest = distance;
			replaced = s;
		}
	}

	out->Add(anchor);
	for (int s = 0; s < grid.Count(); ++s) {
		if (s == replaced) continue;
		if (out->Count() >= cap) break;
		out->Add(grid[s]);
	}
}


void Hybrid_Crossing_Station_List(const HybridInteractionParams & params,const HybridHydrologySet & hydrology,
											 const HybridLandformSet & landforms,const HybridCrossingWindow & window,
											 const Vector2 & centre,DynamicVectorClass<HybridObstructionStation> * out)
{
	InteractionBuilder b;
	::memset(&b,0,sizeof(b));
	b.P			= &params;
	b.Hydrology	= &hydrology;
	b.Landforms	= &landforms;
	Build_Stations(b,window,centre,out);
}


/*
**	A window's own two frozen approach points in the order this road drives them.  The road was routed
**	through exactly these, so a crossing at the window's own station reaches them exactly rather than through
**	a recomputation that rounds a few thousandths away from where the road actually is.
*/
static void Own_Approaches(const HybridCrossingWindow & window,const Vector2 & ingress,Vector2 * first_out,
									Vector2 * second_out)
{
	const bool from_a = ((ingress - window.ApproachA).Length() <= (ingress - window.ApproachB).Length());
	*first_out	= from_a ? window.ApproachA : window.ApproachB;
	*second_out	= from_a ? window.ApproachB : window.ApproachA;
}


//	Is this point inside the placeable world?
static bool Point_Is_Placeable(const InteractionBuilder & b,const Vector2 & world)
{
	const HybridSemanticMatrix & matrix = b.Matrices->Matrix;
	int cx = 0;
	int cy = 0;
	if (!matrix.World_To_Cell(world,&cx,&cy)) return false;
	return matrix.Has_Flag(cx,cy,HYBRID_FLAG_PLACEABLE);
}


/*
**	Does this approach reserve intersect a base or Tiberium core that has nothing to do with the road it
**	belongs to?  The road's own endpoints are exempt: an edge that ends at a base entrance or a field
**	access point is meant to be there.
*/
static bool Reserve_Hits_Unrelated_Reserve(const InteractionBuilder & b,const HybridTopologyEdge & edge,
															const Vector2 & from,const Vector2 & to)
{
	//	Which base and which field this edge is allowed inside, from its own two endpoint nodes.
	unsigned int own_base[2] = { 0,0 };
	unsigned int own_field[2] = { 0,0 };
	const HybridTopologyNode * ends[2];
	ends[0] = b.Topology->Find_Node(edge.NodeA);
	ends[1] = b.Topology->Find_Node(edge.NodeB);
	for (int e = 0; e < 2; ++e) {
		if (ends[e] == nullptr) continue;
		if (ends[e]->Kind == HYBRID_NODE_FIELD_ACCESS)	own_field[e] = ends[e]->SiteId;
		else															own_base[e] = ends[e]->SiteId;
	}

	const HybridConstants & k = b.P->Constants;
	const float spacing = Max2(Min2(k.W,0.5f * k.A),k.Q);
	const float length = (to - from).Length();
	const int steps = (int)::ceil((double)(length / spacing));
	const int count = (steps > 0) ? steps : 1;

	for (int i = 0; i <= count; ++i) {

		const float ratio = (float)i / (float)count;
		const Vector2 point(from.X + ((to.X - from.X) * ratio),from.Y + ((to.Y - from.Y) * ratio));

		for (int t = 0; t < b.Anchors->Bases.Count(); ++t) {
			const HybridBaseAnchor & base = b.Anchors->Bases[t];
			if ((base.SiteId == own_base[0]) || (base.SiteId == own_base[1])) continue;
			if ((point - base.Center).Length() <= base.ProtectedRadius) return true;
		}
		for (int t = 0; t < b.Anchors->Resources.Count(); ++t) {
			const HybridResourceAnchor & anchor = b.Anchors->Resources[t];
			if ((anchor.SiteId == own_field[0]) || (anchor.SiteId == own_field[1])) continue;
			if ((point - anchor.Center).Length() <= anchor.ReserveRadius) return true;
		}
	}
	return false;
}


/*
**	The common hard-valid crossing-station tests, in the plan's own order.  A station that fails any one
**	of them is not a candidate, and the test that refused it is what the issue record reports.
**
**	The eight tests map as follows.  "Station belongs to the intended obstruction" and "the connected road
**	can meet the station" are answered together against the route the road already has: the station has to
**	lie within the crossing interval the frozen Phase-10 centerline actually makes, because Phase 11 may
**	not move a road horizontally -- only Phase 10's next envelope can do that, and asking for one is a
**	separate, counted request.
*/
static bool Station_Is_Hard_Valid(InteractionBuilder & b,const HybridTopologyEdge & edge,
											const HybridRoute & route,const Vector2 & station,
											const Vector2 & across,float span,float approach_reserve,
											Vector2 * approach_a_out,Vector2 * approach_b_out,
											HybridInteractionProblem * problem_out)
{
	const HybridConstants & k = b.P->Constants;
	const HybridRoadClassSpec & spec = Hybrid_Road_Class_Spec(edge.RoadClass);
	const float min_radius = spec.MinTurnRadiusW * k.W;
	const float max_grade = spec.MaxGradeDegrees * PI_OVER_180;
	const float half_roadbed = 0.5f * Hybrid_Roadbed_Width(edge.RoadClass,k.W);

	b.Out->StationsEvaluated++;
	if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_NONE;

	const Vector2 axis = Unit(across);
	if (axis.Length() <= 0.0f) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_NO_HARD_VALID_STATION;
		return false;
	}

	//	The two approach-reserve endpoints: straight, on the crossing axis, one reserve out each side.
	const float reach = (0.5f * span) + approach_reserve;
	const Vector2 approach_a = Quantized(Vector2(station.X - (axis.X * reach),station.Y - (axis.Y * reach)));
	const Vector2 approach_b = Quantized(Vector2(station.X + (axis.X * reach),station.Y + (axis.Y * reach)));
	if (approach_a_out != nullptr) *approach_a_out = approach_a;
	if (approach_b_out != nullptr) *approach_b_out = approach_b;

	/*
	**	1 belongs to the station list -- every station is a point on the obstruction's own centerline
	**	(Hybrid_Obstruction_Stations) -- and 5's turn-radius half belongs to the road that reaches it, which
	**	the caller builds and judges once the crossing's controls are known (Hybrid_Build_Crossing_Path).
	**	Asking instead whether the preliminary centerline passes within some distance of the station anywhere
	**	along its length accepted stations a hundred units down the river from where the road crosses it, on
	**	the strength of the same road running along the bank further on.
	*/
	(void)route;

	//	2: both road-side approach centerlines stay inside placeable bounds.
	if (!Point_Is_Placeable(b,approach_a) || !Point_Is_Placeable(b,approach_b)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_APPROACH_DOES_NOT_FIT;
		return false;
	}

	//	4: no approach reserve intersects an unrelated BASE_PROTECTED region or Tiberium core.
	if (Reserve_Hits_Unrelated_Reserve(b,edge,approach_a,station) ||
		 Reserve_Hits_Unrelated_Reserve(b,edge,station,approach_b)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_RESERVE_CONFLICT;
		return false;
	}

	/*
	**	3 and 5 -- the straight approach reserve is available, and the connected road can meet the station
	**	within its max grade and minimum turn radius -- are answered where the road that reaches the station
	**	exists: Judge_Crossing_Path builds it with the route owner's corner construction and grades the road it
	**	belongs to with Section 12.1's own solver, the structure holding its own surface.  Measuring a grade from
	**	each approach end down to the ground *at the station* asked a canyon bridge to meet the canyon floor,
	**	which its deck never touches, and refused every deep crossing the deck exists to make.
	*/
	(void)max_grade;

	/*
	**	6 belongs to whatever actually carries the vehicle, so it is asked where the method is known rather
	**	than here.  A structure is *widened to fit* -- Section 12.4 sizes the deck to
	**	`max(carriageway, swept + 2*clearance)` and blends both approaches up to it -- so asking the
	**	carriageway to already exceed the swept width refuses every crossing a widened bridge exists to
	**	make.  Measured on the fixture seeds that was 13 of 64 stations on a `PRIMARY_2` canyon crossing,
	**	whose 6.65 carriageway can never hold an 8.80 swept envelope and never needed to.  A method that
	**	puts the vehicle on graded ground asks about its own roadbed; see `Ground_Carries_Swept_Width`.
	*/

	/*
	**	8: at least `4W` from an unrelated already compiled crossing.  Two crossings on one road that the
	**	topology deliberately shares are exempt; anything else is not.
	*/
	const float separation = CROSSING_SEPARATION_W * k.W;
	for (int i = 0; i < b.Out->Interactions.Count(); ++i) {

		const HybridInteraction & other = b.Out->Interactions[i];
		if (other.EdgeId == edge.StableId) continue;					// this road's own, shared by construction
		if ((other.Station - station).Length() >= separation) continue;
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_TOO_CLOSE_TO_CROSSING;
		return false;
	}

	//	9 (12.6.10): no approach reserve -- a bridge's, a pass's, a tunnel's portals -- intersects an active gore reserve.
	if (Segment_Hits_Gore_Reserve(b,approach_a,approach_b,half_roadbed)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_GORE_RESERVE;
		return false;
	}

	//	7 is the selected asset's own business, and is checked where the asset is chosen.
	(void)min_radius;
	return true;
}


/*
**	Can the graded roadbed itself carry the swept vehicle envelope?
**
**	This is the Section-12.4 question asked of a method that leaves the vehicle on the ground -- a ford, a
**	surface or ramp pass, a road cut -- where there is no deck to widen.  The roadbed is the graded surface,
**	shoulders included, which is what a vehicle actually has available; the carriageway is the marked lanes
**	within it.  A structural method asks the same question of its own asset instead.
*/
/*
**	The swept width this edge actually has to carry: the widest envelope among the navigation capabilities
**	Phase 9 required of it.
**
**	Read from the edge's own capability mask, never from its road class.  Class decides the cross section,
**	grade and turn radius; which vehicles an edge must carry is decided by what the edge is for, and a trail
**	does not become heavy-capable because a heavy vehicle was measured somewhere.  An edge with no
**	capability at all is not a road and carries nothing.
*/
static float Required_Swept_Width(const InteractionBuilder & b,unsigned int capabilities)
{
	return Hybrid_Required_Swept_Width(capabilities,b.P->LightVehicleWidth,b.P->HeavyVehicleWidth,b.P->HarvesterWidth);
}


/*
**	Can the roadbed itself carry that envelope?
**
**	The roadbed, and nothing added to it.  `swept + 2 * min_vehicle_lane_clearance` is the plan's *clear
**	width* -- what a bridge deck or a tunnel bore must hold, or how much continuous legal floor a canyon pass
**	needs *around* the road -- and it is not the definition of a roadbed.  Requiring the roadbed to contain it
**	made every class below `HIGHWAY_4` unable to carry any ground method at all, `PRIMARY_2` failing by 0.16
**	world units, which is an arithmetic artifact of comparing the wrong two quantities rather than anything
**	about the road.  The clearance margin is still required where the plan names it: see `Floor_Is_Legal` for
**	a pass floor, Section 12.4 for a bridge deck and Section 12.5 for a tunnel bore.
*/
static bool Ground_Carries_Swept_Width(const InteractionBuilder & b,const HybridTopologyEdge & edge)
{
	return Required_Swept_Width(b,edge.Capabilities) <= Hybrid_Edge_Roadbed_Width(edge,b.P->Constants.W);
}


/***********************************************************************************************
**	Bridge asset selection and candidate scoring.
***********************************************************************************************/

/*
**	Section 8 Phase 11's bridge asset selection.  Reject a kind whose clear width is below Section 12.4,
**	whose maximum span cannot cover the obstruction plus both abutment seats, or whose required approach
**	reserve does not fit.  Choose the smallest sufficient maximum span, then the lower `StableId`.
*/
static int Select_Bridge_Kind(const InteractionBuilder & b,HybridRoadClass road_class,
										unsigned int capabilities,float span,float available_approach,
										HybridInteractionProblem * problem_out,bool whole_deck = false)
{
	const HybridConstants & k = b.P->Constants;

	//	Section 12.4: `max(connected carriageway, required swept width + 2 * clearance)`, the swept width
	//	being what this edge must carry rather than the heaviest vehicle anywhere.
	const float required_width = Hybrid_Bridge_Clear_Width(road_class,k.W,Required_Swept_Width(b,capabilities),
																			 b.P->MinVehicleLaneClearance);


	int best = -1;
	bool width_seen = false;
	bool span_seen = false;

	for (int i = 0; i < b.P->BridgeKinds.Count(); ++i) {

		const HybridBridgeKind & kind = b.P->BridgeKinds[i];
		if (kind.ClearWidth < required_width) continue;
		width_seen = true;
		//	The measured obstruction plus both canonical abutment seats -- the kind's own cap sections.
		//	D-027: an overpass deck is sized whole, its abutment room included; a river or canyon deck adds the seats.
		if (!(kind.MaxSpan >= (whole_deck ? span : (span + kind.StartSeat + kind.EndSeat)))) continue;
		span_seen = true;

		//	The required straight, `max(8W, required_approach_length)`, against what the road offers -- measured on
		//	positions written to `Q`, so the comparison carries that rounding and nothing more.
		const float approach = Max2(STATION_HALF_LENGTH_W * k.W,kind.RequiredApproachLength);
		if (approach > (available_approach + (2.0f * k.Q))) continue;

		if (best < 0) {
			best = i;
			continue;
		}

		//	Smallest sufficient maximum span, then the lower registry StableId.
		const HybridBridgeKind & incumbent = b.P->BridgeKinds[best];
		if (kind.MaxSpan < incumbent.MaxSpan) {
			best = i;
		} else if ((kind.MaxSpan == incumbent.MaxSpan) && (kind.StableId < incumbent.StableId)) {
			best = i;
		}
	}

	if ((best < 0) && (problem_out != nullptr)) {
		if (!width_seen)		*problem_out = HYBRID_PROBLEM_WIDTH_TOO_NARROW;
		else if (!span_seen)	*problem_out = HYBRID_PROBLEM_SPAN_TOO_LONG;
		else						*problem_out = HYBRID_PROBLEM_APPROACH_DOES_NOT_FIT;
	}
	return best;
}


/*
**	Section 8 Phase 11's bridge candidate scoring, exactly:
**
**		span_term      = span / selected_bridge.max_supported_span
**		elevation_term = abs(bank_z_a-bank_z_b) / max(tier_step,Q)
**		grade_term     = max(approach_grade_a,approach_grade_b) / max(road_class.max_grade,Q)
**		curve_term     = max(0, road_class.min_turn_radius/measured_turn_radius - 1)
**		detour_term    = added_route_length / max(8W,Q)
**		score = 4*span_term + 2*elevation_term + 3*grade_term + 2*curve_term + detour_term
*/
static float Bridge_Candidate_Score(const InteractionBuilder & b,HybridRoadClass road_class,
												const HybridBridgeKind & kind,float span,float bank_z_a,
												float bank_z_b,float grade_a,float grade_b,float measured_radius,
												float added_length)
{
	const HybridConstants & k = b.P->Constants;
	const HybridRoadClassSpec & spec = Hybrid_Road_Class_Spec(road_class);
	const float max_grade = spec.MaxGradeDegrees * PI_OVER_180;
	const float min_radius = spec.MinTurnRadiusW * k.W;

	const float span_term = span / Max2(kind.MaxSpan,k.Q);
	const float elevation_term = (float)fabs((double)(bank_z_a - bank_z_b)) / Max2(b.P->TierStep,k.Q);
	const float grade_term = Max2(grade_a,grade_b) / Max2(max_grade,k.Q);
	const float curve_term = Max2(0.0f,(min_radius / Max2(measured_radius,k.Q)) - 1.0f);
	const float detour_term = Max2(0.0f,added_length) / Max2(STATION_HALF_LENGTH_W * k.W,k.Q);

	return (4.0f * span_term) + (2.0f * elevation_term) + (3.0f * grade_term) + (2.0f * curve_term) +
			 detour_term;
}


/***********************************************************************************************
**	Section 12.3's vertical geometry.
***********************************************************************************************/

/*
**	A river bridge, exactly as Section 12.3 words it:
**
**		abutment_z    = max(bank_a_road_z, bank_b_road_z)
**		deck_bottom_z >= water_surface_z + selected_bridge.water_clearance_margin
**		deck_top_z    = max(road approach profile, deck_bottom_z + selected_bridge.structural_depth)
**
**	A canyon or ravine bridge instead takes its endpoints from the connected road profiles at the two
**	abutments, and its deck is the minimum-grade profile between them; it never descends toward the floor.
*/
static bool Bridge_Vertical_Geometry(const InteractionBuilder & b,HybridObstructionKind obstruction,
												const HybridBridgeKind & kind,const Vector2 & station,float bank_z_a,
												float bank_z_b,float * bottom_out,float * top_out,
												HybridInteractionProblem * problem_out)
{
	const float abutment_z = Max2(bank_z_a,bank_z_b);

	if (obstruction == HYBRID_OBSTRUCTION_RIVER) {

		float surface_z = 0.0f;
		unsigned int body = 0;
		if (!Hybrid_Point_In_Standing_Water(*b.Hydrology,station,&body,&surface_z)) {
			//	A channel rather than a standing body: the water surface is the local ground the channel cut.
			if (!b.Height_At(station,&surface_z)) {
				if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_CLEARANCE;
				return false;
			}
		}

		const float margin = Max2(b.P->BridgeWaterClearanceMargin,kind.RequiredClearance);
		const float bottom = surface_z + margin;
		const float top = Max2(abutment_z,bottom + kind.StructuralDepth);

		*bottom_out	= Hybrid_Quantize_Position(bottom);
		*top_out		= Hybrid_Quantize_Position(top);
		return true;
	}

	//	Canyon or ravine: the deck spans between the two road profiles and does not dip between them.
	const float top = abutment_z;
	*bottom_out	= Hybrid_Quantize_Position(top - kind.StructuralDepth);
	*top_out		= Hybrid_Quantize_Position(top);
	return true;
}


/***********************************************************************************************
**	The swept proxy validation every compiled interval gets before the gate.
***********************************************************************************************/

//	Defined with the crossing path judge below.
static float Path_Station_Of(const HybridCrossingPath & path,const Vector2 & point);


/*
**	"Before the phase gate, run lightweight swept proxy validation over every compiled bridge/pass/
**	tunnel/overpass interval using the cached collision envelopes."
**
**	The proxy is the heavy reference vehicle's width and height swept along the compiled interval: every
**	sample must stand on placeable ground, keep the class cross slope across the roadbed where the road is
**	on the ground, and keep Section 12.3's clearance where it is not.
*/
static bool Sweep_Is_Clear(InteractionBuilder & b,const HybridInteraction & one,
									HybridInteractionProblem * problem_out)
{
	const HybridConstants & k = b.P->Constants;
	const HybridRoadClassSpec & spec = Hybrid_Road_Class_Spec(one.RoadClass);
	const float max_cross = spec.MaxCrossSlopeDegrees * PI_OVER_180;
	const float half_roadbed = 0.5f * Hybrid_Roadbed_Width(one.RoadClass,k.W);

	//	The plan's own sample spacing for each kind of interval.
	float spacing = Min2(k.W / PASS_SAMPLE_A,k.A / PASS_SAMPLE_A);
	if ((one.Method == HYBRID_METHOD_SURFACE_PASS) || (one.Method == HYBRID_METHOD_RAMP_PASS)) {
		spacing = Min2(PASS_SAMPLE_W * k.W,k.A / PASS_SAMPLE_A);
	} else if (one.Method == HYBRID_METHOD_OVERPASS) {
		spacing = Min2(k.W / OVERPASS_SAMPLE_W,k.A / OVERPASS_SAMPLE_A);
	}
	spacing = Max2(spacing,k.Q);

	/*
	**	The swept line is the road the crossing is actually driven on -- its frozen path when it has one, the
	**	straight between its two ends otherwise -- and a structure's own deck or bore carries its own surface,
	**	so the ground under it is not what a vehicle drives on.
	*/
	DynamicVectorClass<Vector2> line;
	DynamicVectorClass<float> stations;
	if (one.Path.Points.Count() >= 2) {
		for (int i = 0; i < one.Path.Points.Count(); ++i) {
			line.Add(one.Path.Points[i]);
			stations.Add(one.Path.Stations[i]);
		}
	} else {
		line.Add(one.ApproachA);
		line.Add(one.ApproachB);
		stations.Add(0.0f);
		stations.Add((one.ApproachB - one.ApproachA).Length());
	}
	const float total = stations[stations.Count() - 1];

	float structure_from = total + 1.0f;
	float structure_to = total + 1.0f;
	if (one.Method == HYBRID_METHOD_OVERPASS) {
		structure_from	= 0.0f;
		structure_to	= total;
	} else if ((one.Method == HYBRID_METHOD_BRIDGE) || (one.Method == HYBRID_METHOD_TUNNEL)) {
		const Vector2 axis = Unit(one.Tangent);
		const float half_before = one.DeckReachA;
		const float half_after = one.DeckReachB;
		const Vector2 middle = (one.Method == HYBRID_METHOD_BRIDGE)
										? one.Station : ((one.ApproachA + one.ApproachB) * 0.5f);
		HybridCrossingPath probe;
		probe.Points = line;
		probe.Stations = stations;
		structure_from = Path_Station_Of(probe,middle - (axis * half_before));
		structure_to = Path_Station_Of(probe,middle + (axis * half_after));
		if (structure_to < structure_from) {
			const float swap = structure_from;
			structure_from	= structure_to;
			structure_to	= swap;
		}
	}

	const int steps = (int)::ceil((double)(total / spacing));
	const int count = (steps > 0) ? steps : 1;
	for (int i = 0; i <= count; ++i) {

		const float at = total * ((float)i / (float)count);
		const Vector2 point = Hybrid_Polyline_Point_At(line,stations,at);
		b.Out->SweepSamples++;

		if (!Point_Is_Placeable(b,point)) {
			if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_RESERVE_CONFLICT;
			return false;
		}
		if ((at >= structure_from) && (at <= structure_to)) continue;

		//	Across the road where it is: the local direction of the line, not the crossing's own axis.
		const Vector2 ahead = Hybrid_Polyline_Point_At(line,stations,Min2(at + spacing,total));
		const Vector2 behind = Hybrid_Polyline_Point_At(line,stations,Max2(at - spacing,0.0f));
		const Vector2 direction = Unit(ahead - behind);
		const Vector2 normal(-direction.Y,direction.X);

		float left = 0.0f;
		float right = 0.0f;
		if (!b.Height_At(point + (normal * half_roadbed),&left) ||
			 !b.Height_At(point - (normal * half_roadbed),&right)) {
			if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_CLEARANCE;
			return false;
		}
		const HybridVerticalInterval left_interval = Hybrid_Road_Vertical_Interval(left,b.P->MaxCutDepth,
																								  b.P->MaxFillHeight);
		const HybridVerticalInterval right_interval = Hybrid_Road_Vertical_Interval(right,b.P->MaxCutDepth,
																									b.P->MaxFillHeight);
		if (!Hybrid_Grade_Compatible(left_interval,right_interval,2.0f * half_roadbed,max_cross)) {
			if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_CROSS_SLOPE;
			return false;
		}
	}
	return true;
}


/***********************************************************************************************
**	The method orders.  Each one is the plan's, and each one is fixed.
***********************************************************************************************/

/*
**	A window's crossing axis in the direction this road drives it: from the bank its interval enters by to
**	the other.  Phase 6 named one approach `A` and the other `B` without knowing which way any road would
**	come, so the road's own ingress decides which end of a compiled crossing is first.
*/
static Vector2 Travel_Axis(const HybridCrossingWindow & window,const Vector2 & ingress)
{
	const Vector2 axis = Unit(window.ApproachB - window.ApproachA);
	const bool from_a = ((ingress - window.ApproachA).Length() <= (ingress - window.ApproachB).Length());
	return from_a ? axis : Vector2(-axis.X,-axis.Y);
}


/*
**	How much straight road is available either side of a crossing, measured on the route the road already
**	has: the arc length before the structural interval starts and after it ends.  A bridge kind whose
**	required approach does not fit inside the smaller of the two is rejected, which is the plan's own
**	"if required approach length exceeds the reserved/searchable corridor, reject that bridge candidate".
*/
static float Available_Approach(const HybridRoute & route,int first_point,int last_point)
{
	if (route.Stations.Count() < 2) return 0.0f;

	const int first = (first_point > 0) ? first_point : 0;
	const int last = (last_point < route.Stations.Count()) ? last_point : (route.Stations.Count() - 1);
	const float total = route.Stations[route.Stations.Count() - 1];
	const float before = route.Stations[first];
	const float after = total - route.Stations[last];
	return Min2(before,after);
}


//	The grade the road makes between a crossing station and one of its approach-reserve endpoints.
static bool Approach_Grade(InteractionBuilder & b,const Vector2 & station,float station_z,
									const Vector2 & end,float * grade_out,float * end_z_out)
{
	float end_z = 0.0f;
	if (!b.Height_At(end,&end_z)) return false;
	const float run = (end - station).Length();
	if (end_z_out != nullptr) *end_z_out = end_z;
	*grade_out = (run > b.P->Constants.Q)
					 ? (float)::atan((double)((float)fabs((double)(end_z - station_z)) / run)) : 0.0f;
	return true;
}


/***********************************************************************************************
**	The road that reaches a compiled crossing.
***********************************************************************************************/

/*
**	May a road stand here?  Step 18's approach-sample rules, which judged the window's own approaches --
**	placeable, no standing water, no other river's channel, no explicit wall other than the crossing's own --
**	and clear of any base or field reserve the edge has no business in.
*/
static bool Path_Point_Is_Legal(const InteractionBuilder & b,const HybridTopologyEdge & edge,
										  const HybridCrossingWindow & window,const Vector2 & point,
										  HybridInteractionProblem * problem_out)
{
	if (!Point_Is_Placeable(b,point)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_APPROACH_DOES_NOT_FIT;
		return false;
	}
	if (!Hybrid_Crossing_Ground_Is_Clear(*b.Hydrology,*b.Landforms,window.Obstruction,window.ObstructionRecordId,
													 point)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_APPROACH_DOES_NOT_FIT;
		return false;
	}
	if (Reserve_Hits_Unrelated_Reserve(b,edge,point,point)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_RESERVE_CONFLICT;
		return false;
	}
	return true;
}


//	The arc length on `path` nearest `point`.
static float Path_Station_Of(const HybridCrossingPath & path,const Vector2 & point)
{
	float best = 3.4e38f;
	float station = 0.0f;
	for (int i = 1; i < path.Points.Count(); ++i) {
		float t = 0.0f;
		const float distance = Distance_To_Segment(point,path.Points[i - 1],path.Points[i],&t);
		if (distance >= best) continue;
		best		= distance;
		station	= path.Stations[i - 1] + ((path.Stations[i] - path.Stations[i - 1]) * t);
	}
	return station;
}


/***********************************************************************************************
**	A road as it will be realized, and its structures, for Section 12.1.
***********************************************************************************************/

//	The station on a realized road of the point nearest `point`, over its samples `lo..hi`.
static float Realized_Station_Of(const HybridRoute & road,int lo,int hi,const Vector2 & point)
{
	float best = 3.4e38f;
	float station = road.Stations[lo];
	for (int i = lo + 1; i <= hi; ++i) {
		float t = 0.0f;
		const float distance = Distance_To_Segment(point,road.Points[i - 1],road.Points[i],&t);
		if (distance >= best) continue;
		best		= distance;
		station	= road.Stations[i - 1] + ((road.Stations[i] - road.Stations[i - 1]) * t);
	}
	return station;
}


//	Where a crossing path lies on the realized road: the samples its first and last points were copied to.
static bool Realized_Range(const HybridRoute & road,const HybridCrossingPath & path,int * lo_out,int * hi_out)
{
	const int count = path.Points.Count();
	if (count < 2) return false;
	for (int i = 0; i < road.Points.Count(); ++i) {
		if (road.Points[i] != path.Points[0]) continue;
		for (int j = i + 1; j < road.Points.Count(); ++j) {
			if (road.Points[j] != path.Points[count - 1]) continue;
			*lo_out = i;
			*hi_out = j;
			return true;
		}
	}
	return false;
}


/*
**	A compiled crossing's structure on the realized road it belongs to: where its deck or bore starts and
**	stops, and the surface it holds -- a river deck or an overpass deck level at its top, a canyon or ravine
**	deck and a tunnel bore carried between the two end elevations the record froze.  False when the method
**	builds no structure (a pass, a cut, a junction).
*/
static bool Compiled_Structure(const HybridInteraction & one,const HybridRoute & road,HybridProfileStructure * out)
{
	if (road.Points.Count() < 2) return false;
	const int last = road.Points.Count() - 1;

	if (one.Method == HYBRID_METHOD_OVERPASS) {
		const float centre = Realized_Station_Of(road,0,last,one.Station);
		out->From		= centre - (0.5f * one.Span);
		out->To			= centre + (0.5f * one.Span);
		out->Mode		= HYBRID_PROFILE_STRUCTURE_LEVEL;
		out->EndsFixed	= true;
		out->StartZ		= one.DeckTopZ;
		out->EndZ		= one.DeckTopZ;
		return true;
	}
	if ((one.Method != HYBRID_METHOD_BRIDGE) && (one.Method != HYBRID_METHOD_TUNNEL)) return false;

	int lo = 0;
	int hi = last;
	if (!Realized_Range(road,one.Path,&lo,&hi)) return false;

	const Vector2 axis = Unit(one.Tangent);
	const Vector2 from_point = one.Station - (axis * one.DeckReachA);
	const Vector2 to_point = one.Station + (axis * one.DeckReachB);

	out->From		= Realized_Station_Of(road,lo,hi,from_point);
	out->To			= Realized_Station_Of(road,lo,hi,to_point);
	out->EndsFixed	= true;
	const bool level = (one.Method == HYBRID_METHOD_BRIDGE) && (one.Obstruction == HYBRID_OBSTRUCTION_RIVER);
	out->Mode		= level ? HYBRID_PROFILE_STRUCTURE_LEVEL : HYBRID_PROFILE_STRUCTURE_CARRIED;
	out->StartZ		= level ? one.DeckTopZ : one.DeckStartZ;
	out->EndZ		= level ? one.DeckTopZ : one.DeckEndZ;
	if (out->To < out->From) {
		const float swap = out->From;
		out->From	= out->To;
		out->To		= swap;
	}
	return true;
}


bool Hybrid_Road_Profile_Structures(const HybridInteractionSet & set,const HybridRoute & realized,
												DynamicVectorClass<HybridProfileStructure> * out,
												const DynamicVectorClass<unsigned int> * edges,
												DynamicVectorClass<unsigned int> * interactions_out)
{
	if (out == nullptr) return false;
	out->Delete_All();
	if (interactions_out != nullptr) interactions_out->Delete_All();
	for (int i = 0; i < set.Interactions.Count(); ++i) {
		const HybridInteraction & one = set.Interactions[i];
		bool on_road = (one.EdgeId == realized.EdgeId);
		for (int e = 0; (edges != nullptr) && (e < edges->Count()) && !on_road; ++e) on_road = (one.EdgeId == (*edges)[e]);
		if (!on_road) continue;
		HybridProfileStructure structure;
		if (!Compiled_Structure(one,realized,&structure)) {
			if ((one.Method == HYBRID_METHOD_BRIDGE) || (one.Method == HYBRID_METHOD_TUNNEL) ||
				 (one.Method == HYBRID_METHOD_OVERPASS)) return false;
			continue;
		}
		out->Add(structure);
		if (interactions_out != nullptr) interactions_out->Add(one.StableId);
	}
	return true;
}


/*
**	What a compiled crossing's structure does to the road's profile.
*/
enum CrossingStructure
{
	CROSSING_STRUCTURE_NONE = 0,		// the road stays on the ground the whole way: a pass, a cut
	CROSSING_STRUCTURE_LEVEL,			// a level deck at a fixed top both approaches must reach: a river bridge
	CROSSING_STRUCTURE_CARRIED			// a deck or bore between the road's own elevations at its ends
};


/*
**	"The connected road can meet the station within its max grade and minimum turn radius", answered by
**	building that road.
**
**	`controls` are the crossing's own centerline in travel order, the first on the interval's ingress side.
**	The path is Hybrid_Build_Crossing_Path's -- the route owner's arc-straight-arc connectors at the class
**	turn radius, departing and rejoining the Phase-10 centerline inside the station search's own `max(8W,4S)`,
**	and the crossing's own controls with D-009 fillets -- and it is refused when it cannot be built, when it
**	would overlap another crossing this road already compiled, when any ground sample of it stands somewhere a
**	road may not, or when Section 12.1's own solver cannot give it a profile.  That solver is the one Phase 12
**	realizes the road with, so a crossing accepted here is one the realization can actually grade.
**
**	`structure_centre` / `structure_axis` and the two halves place a deck or a bore on the path -- a bridge's
**	reaches past its obstruction by its own start and end abutment seats, which can differ.  A LEVEL
**	structure holds every sample on it at `pin_z`; a CARRIED one holds its samples to the grade alone, on the
**	line between the ground at its two ends, and its two end elevations come back in `start_z_out` and
**	`end_z_out` -- Section 12.3's canyon deck "between those endpoint elevations", which "does not descend
**	toward the canyon floor".
*/
static bool Judge_Crossing_Path(InteractionBuilder & b,const HybridTopologyEdge & edge,const HybridRoute & route,
										  const HybridRouteStructuralSpan & interval,const HybridCrossingWindow & window,
										  const Vector2 * controls,int control_count,const Vector2 & structure_centre,
										  const Vector2 & structure_axis,float half_before,float half_after,
										  CrossingStructure structure,
										  float pin_z,HybridCrossingPath * path_out,float * start_z_out,
										  float * end_z_out,HybridInteractionProblem * problem_out)
{
	const HybridConstants & k = b.P->Constants;
	const HybridRoadClassSpec & spec = Hybrid_Road_Class_Spec(edge.RoadClass);
	const float radius = spec.MinTurnRadiusW * k.W;
	const float spacing = Max2(Min2(k.W,0.5f * k.A),k.Q);
	if (start_z_out != nullptr) *start_z_out = pin_z;
	if (end_z_out != nullptr) *end_z_out = pin_z;

	if (!Hybrid_Build_Crossing_Path(route,interval.FirstPoint,interval.LastPoint,controls,control_count,radius,
											  spacing,k.Q,Station_Half_Length(k),path_out,nullptr)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_TURN_TOO_TIGHT;
		return false;
	}

	/*
	**	Two crossings on one road may not claim the same stretch of it.  Two windows back to back may share the
	**	one sample where the first rejoins and the second departs; anything more is the same road twice.
	*/
	for (int i = 0; i < b.Out->Interactions.Count(); ++i) {
		const HybridInteraction & other = b.Out->Interactions[i];
		if ((other.EdgeId != edge.StableId) || (other.Path.Points.Count() == 0)) continue;
		if ((other.Method == HYBRID_METHOD_OVERPASS) || (other.Method == HYBRID_METHOD_AT_GRADE_JUNCTION)) continue;
		if ((path_out->RejoinPoint <= other.Path.DepartPoint) || (path_out->DepartPoint >= other.Path.RejoinPoint)) {
			continue;
		}
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_TOO_CLOSE_TO_CROSSING;
		return false;
	}

	//	Where the structure starts and stops on the path.
	const bool has_structure = (structure != CROSSING_STRUCTURE_NONE) && ((half_before + half_after) > 0.0f);
	const float total = path_out->Stations[path_out->Stations.Count() - 1];
	float structure_from = total;
	float structure_to = total;
	if (has_structure) {
		const Vector2 axis = Unit(structure_axis);
		structure_from	= Path_Station_Of(*path_out,structure_centre - (axis * half_before));
		structure_to	= Path_Station_Of(*path_out,structure_centre + (axis * half_after));
		if (structure_to < structure_from) {
			const float swap = structure_from;
			structure_from	= structure_to;
			structure_to	= swap;
		}
	}

	/*
	**	Every ground sample stands somewhere a road may, and the ground across the roadbed there can be graded
	**	level inside the class cross slope -- the same test the gate's swept proxy check applies afterwards, so
	**	a station that would fail it is refused here, where another station can still be tried.
	*/
	const float sample_spacing = Hybrid_Profile_Sample_Spacing(k);
	const float half_roadbed = 0.5f * Hybrid_Roadbed_Width(edge.RoadClass,k.W);
	const float max_cross = spec.MaxCrossSlopeDegrees * PI_OVER_180;
	for (float s = 0.0f; s <= (total + k.Q); s += sample_spacing) {
		const float at = Min2(s,total);
		if (has_structure && (at > structure_from) && (at < structure_to)) continue;
		const Vector2 point = Hybrid_Polyline_Point_At(path_out->Points,path_out->Stations,at);
		HybridInteractionProblem problem = HYBRID_PROBLEM_NONE;
		if (!Path_Point_Is_Legal(b,edge,window,point,&problem)) {
			if (problem_out != nullptr) *problem_out = problem;
			return false;
		}

		const Vector2 ahead = Hybrid_Polyline_Point_At(path_out->Points,path_out->Stations,Min2(at + sample_spacing,total));
		const Vector2 behind = Hybrid_Polyline_Point_At(path_out->Points,path_out->Stations,Max2(at - sample_spacing,0.0f));
		const Vector2 direction = Unit(ahead - behind);
		if (direction.Length() <= 0.0f) continue;
		const Vector2 normal(-direction.Y,direction.X);
		float left = 0.0f;
		float right = 0.0f;
		if (!b.Height_At(point + (normal * half_roadbed),&left) || !b.Height_At(point - (normal * half_roadbed),&right)) {
			if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_CLEARANCE;
			return false;
		}
		const HybridVerticalInterval left_interval = Hybrid_Road_Vertical_Interval(left,b.P->MaxCutDepth,
																								  b.P->MaxFillHeight);
		const HybridVerticalInterval right_interval = Hybrid_Road_Vertical_Interval(right,b.P->MaxCutDepth,
																									b.P->MaxFillHeight);
		if (!Hybrid_Grade_Compatible(left_interval,right_interval,2.0f * half_roadbed,max_cross)) {
			if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_CROSS_SLOPE;
			return false;
		}
	}

	/*
	**	And Section 12.1 can grade it -- not the path alone but the road it is part of, exactly as Phase 12 will
	**	solve it: the road as it will be realized, with every crossing already compiled on it holding its own
	**	surface and this one holding its own, from the nearest crossing still to be compiled behind it to the
	**	nearest ahead (whose deck elevations are not known yet, so the road is free there), or the road's ends.
	**	Judging the connector alone left a river deck to be reached inside its own approach reserve, when the
	**	road behind it is free to start climbing long before.
	*/
	//	The physical road this edge is part of (Q-025): a heavy family's whole trunk, or the edge itself.
	DynamicVectorClass<unsigned int> road_edges;
	const HybridTrunkRoad * trunk = b.Routes->Find_Trunk(edge.StableId);
	if (trunk != nullptr) {
		for (int e = 0; e < trunk->EdgeIds.Count(); ++e) road_edges.Add(trunk->EdgeIds[e]);
	} else {
		road_edges.Add(edge.StableId);
	}

	HybridInteractionSet trial;
	for (int i = 0; i < b.Out->Interactions.Count(); ++i) {
		const HybridInteraction & other = b.Out->Interactions[i];
		bool on_road = false;
		for (int e = 0; (e < road_edges.Count()) && !on_road; ++e) on_road = (other.EdgeId == road_edges[e]);
		if (!on_road) continue;
		if ((other.Method == HYBRID_METHOD_OVERPASS) || (other.Method == HYBRID_METHOD_AT_GRADE_JUNCTION)) continue;
		trial.Interactions.Add(other);
	}
	HybridInteraction candidate;
	candidate.StableId	= 0;
	candidate.EdgeId		= edge.StableId;
	candidate.WindowId	= window.StableId;
	candidate.Method		= HYBRID_METHOD_BRIDGE;
	candidate.ApproachA	= controls[0];
	candidate.ApproachB	= controls[control_count - 1];
	candidate.Path			= *path_out;
	trial.Interactions.Add(candidate);

	HybridRoute road;
	int lo = 0;
	int hi = 0;
	if (!Hybrid_Realized_Physical_Road(*b.Routes,trial,edge.StableId,&road,nullptr) ||
		 !Realized_Range(road,*path_out,&lo,&hi)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_TOO_CLOSE_TO_CROSSING;
		return false;
	}

	DynamicVectorClass<HybridProfileStructure> structures;
	for (int i = 0; (i + 1) < trial.Interactions.Count(); ++i) {
		HybridProfileStructure one;
		if (Compiled_Structure(trial.Interactions[i],road,&one)) structures.Add(one);
	}
	int own = -1;
	if (has_structure) {
		const Vector2 axis = Unit(structure_axis);
		HybridProfileStructure one;
		one.From		= Realized_Station_Of(road,lo,hi,structure_centre - (axis * half_before));
		one.To		= Realized_Station_Of(road,lo,hi,structure_centre + (axis * half_after));
		if (one.To < one.From) {
			const float swap = one.From;
			one.From	= one.To;
			one.To	= swap;
		}
		one.Mode			= (structure == CROSSING_STRUCTURE_LEVEL) ? HYBRID_PROFILE_STRUCTURE_LEVEL
																			 : HYBRID_PROFILE_STRUCTURE_CARRIED;
		one.EndsFixed	= (structure == CROSSING_STRUCTURE_LEVEL);
		one.StartZ		= pin_z;
		one.EndZ			= pin_z;
		own = structures.Count();
		structures.Add(one);
	}

	//	The stretch: bounded by the crossings on this road -- every logical edge of it -- still to be compiled.
	float stretch_from = 0.0f;
	float stretch_to = road.Stations[road.Stations.Count() - 1];
	const float own_from = road.Stations[lo];
	const float own_to = road.Stations[hi];
	for (int e = 0; e < road_edges.Count(); ++e) {
		const HybridRoute * slice = b.Routes->Find(road_edges[e]);
		if (slice == nullptr) continue;
		for (int s = 0; s < slice->Structural.Count(); ++s) {
			const HybridRouteStructuralSpan & span = slice->Structural[s];
			bool compiled = false;
			for (int i = 0; (i < trial.Interactions.Count()) && !compiled; ++i) {
				if (trial.Interactions[i].WindowId == span.WindowId) compiled = true;
			}
			if (compiled) continue;

			int at_in = -1;
			int at_out = -1;
			for (int i = 0; i < road.Points.Count(); ++i) {
				if ((at_in < 0) && (road.Points[i] == slice->Points[span.FirstPoint])) at_in = i;
				if (road.Points[i] == slice->Points[span.LastPoint]) at_out = i;
			}
			if ((at_in < 0) || (at_out < at_in)) continue;
			if (road.Stations[at_out] <= own_from)	stretch_from = Max2(stretch_from,road.Stations[at_out]);
			if (road.Stations[at_in] >= own_to)		stretch_to = Min2(stretch_to,road.Stations[at_in]);
		}
	}

	DynamicVectorClass<HybridProfileSample> samples;
	HybridSolvedProfile solved;
	DynamicVectorClass<float> starts;
	DynamicVectorClass<float> ends;
	HybridProfileFailure main_why = HYBRID_PROFILE_OK;
	HybridGradeBudget budget;
	if (!Hybrid_Solve_Road_Stretch(road.Points,road.Stations,stretch_from,stretch_to,structures,
											 Hybrid_Profile_Sample_Spacing(k),b.Sample,b.Context,b.P->MaxCutDepth,
											 b.P->MaxFillHeight,spec.MaxGradeDegrees * PI_OVER_180,k.Q,&samples,&solved,
											 &starts,&ends,&main_why,Grade_Budget(b,edge.RoadClass,road_edges,structures,&budget))) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_GRADE_TOO_STEEP;
		return false;
	}
	if (own >= 0) {
		if (start_z_out != nullptr) *start_z_out = starts[own];
		if (end_z_out != nullptr) *end_z_out = ends[own];
	}
	return true;
}


/*
**	The straight approach the road already has at its window's own station, measured on the road itself:
**	from each edge of the obstruction out along the crossing axis to where the road's turn begins.  Phase 10
**	runs every crossing straight through its window's frozen approach points and keeps a straight beyond each,
**	so this is at least Step 18's approach reserve; zero when the road is not straight across at all.
*/
static float Own_Straight_Approach(const InteractionBuilder & b,const HybridRoute & route,
											  const HybridRouteStructuralSpan & interval,const Vector2 & station,
											  const Vector2 & across,float span)
{
	const float q = b.P->Constants.Q;
	const Vector2 & in = route.Points[interval.FirstPoint];
	const Vector2 & out = route.Points[interval.LastPoint];

	float behind = 0.0f;
	float through = 0.0f;
	float ahead = 0.0f;
	Hybrid_Polyline_Straight_Run(route.Points,interval.FirstPoint,across,q,&behind,&through);
	Hybrid_Polyline_Straight_Run(route.Points,interval.LastPoint,across,q,nullptr,&ahead);
	if (through < ((out - in).Length() - (2.0f * q))) return 0.0f;

	const Vector2 edge_a(station.X - (across.X * 0.5f * span),station.Y - (across.Y * 0.5f * span));
	const Vector2 edge_b(station.X + (across.X * 0.5f * span),station.Y + (across.Y * 0.5f * span));
	const float side_a = ((edge_a.X - in.X) * across.X) + ((edge_a.Y - in.Y) * across.Y) + behind;
	const float side_b = ((out.X - edge_b.X) * across.X) + ((out.Y - edge_b.Y) * across.Y) + ahead;
	return Max2(0.0f,Min2(side_a,side_b));
}


/*
**	Do a bridge's two ends stand on legal dry bank?  Everything the deck reaches past the obstruction -- its
**	abutment caps, and whatever more of the bank it spans to meet the road -- has to be placeable, out of any
**	water, the crossed river's own channel included, and off any other explicit wall.
*/
static bool Abutments_Are_Legal(const InteractionBuilder & b,const HybridCrossingWindow & window,
										  const Vector2 & station,const Vector2 & across,float span,float reach_before,
										  float reach_after)
{
	const float spacing = Max2(Min2(b.P->Constants.W,0.5f * b.P->Constants.A),b.P->Constants.Q);
	for (int side = 0; side < 2; ++side) {

		const float beyond = ((side == 0) ? reach_before : reach_after) - (0.5f * span);
		if (!(beyond > 0.0f)) continue;
		const float sense = (side == 0) ? -1.0f : 1.0f;
		const int steps = (int)::ceil((double)(beyond / spacing));
		const int count = (steps > 0) ? steps : 1;
		for (int i = 1; i <= count; ++i) {
			const float out = (0.5f * span) + (beyond * ((float)i / (float)count));
			const Vector2 point(station.X + (across.X * out * sense),station.Y + (across.Y * out * sense));
			if (!Point_Is_Placeable(b,point)) return false;
			if (Hybrid_Point_In_Water(*b.Hydrology,point)) return false;
			if (!Hybrid_Crossing_Ground_Is_Clear(*b.Hydrology,*b.Landforms,window.Obstruction,
															 window.ObstructionRecordId,point)) return false;
		}
	}
	return true;
}


/*
**	A bridge at the lowest-score hard-valid station.  Used by road x river and road x canyon/ravine alike: the
**	two differ in their fallback order and in Section 12.3's vertical geometry, not in this.
**
**	Revised D-009 (Q-022): the window's own station is the crossing Phase 10 already aligned the road with, so
**	the road crosses there exactly as routed and the straight approach it offers is measured on it.  Any other
**	station gets a connector, built by the route owner's own corner construction, that turns before the
**	station's approach endpoint and runs straight from there to the obstruction for exactly the approach the
**	selected kind needs; a station no legal connector reaches is not hard-valid.  An approach is never
**	shortened to fit, and a kind whose approach the road cannot give is rejected.
*/
static bool Compile_Bridge(InteractionBuilder & b,const HybridTopologyEdge & edge,const HybridRoute & route,
									const HybridRouteStructuralSpan & interval,const HybridCrossingWindow & window,
									bool mandatory,const Vector2 & centre,HybridInteraction * out,
									HybridInteractionProblem * problem_out,int * stations_out)
{
	const HybridConstants & k = b.P->Constants;
	if (!window.Has_Method(HYBRID_WINDOW_METHOD_BRIDGE)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_METHOD_UNAVAILABLE;
		return false;
	}

	const Vector2 ingress = route.Points[interval.FirstPoint];
	const Vector2 egress = route.Points[interval.LastPoint];

	DynamicVectorClass<HybridObstructionStation> stations;
	Build_Stations(b,window,centre,&stations);
	if (stations_out != nullptr) *stations_out = stations.Count();

	/*
	**	Which refusal to report.  A station that passed every common test and then failed on the asset is a
	**	more useful diagnosis than one that never reached the asset at all, so the furthest stage any station
	**	got to is what names the failure.  Keeping simply the last rejection reports whichever station
	**	happened to be evaluated last, which on a 64-station search is arbitrary.
	*/
	HybridInteractionProblem station_problem = HYBRID_PROBLEM_NO_HARD_VALID_STATION;
	HybridInteractionProblem asset_problem = HYBRID_PROBLEM_NONE;
	HybridInteractionProblem geometry_problem = HYBRID_PROBLEM_NONE;
	bool found = false;
	float best_score = 0.0f;
	HybridInteraction best;

	for (int s = 0; s < stations.Count(); ++s) {

		/*
		**	The crossing at this station runs across the obstruction *here*, from the bank the road arrives on
		**	to the other, and spans the obstruction's own width here.  At the window's own station that is the
		**	axis the road was routed along.
		*/
		const Vector2 & station = stations[s].Centre;
		const bool own_station = (station == window.Center);
		HybridInteractionProblem excluded_problem = HYBRID_PROBLEM_PIN_LANDING;
		if (b.Is_Excluded(HYBRID_METHOD_BRIDGE,edge.StableId,0,window.StableId,true,station,&excluded_problem)) {
			geometry_problem = excluded_problem;
			b.Note_Reject(excluded_problem);
			continue;
		}
		Vector2 across(-stations[s].Tangent.Y,stations[s].Tangent.X);
		if ((((ingress.X - station.X) * across.X) + ((ingress.Y - station.Y) * across.Y)) > 0.0f) across = -across;
		if (own_station) across = Unit(egress - ingress);
		const float span = own_station ? window.Span : Hybrid_Quantize_Position(2.0f * stations[s].HalfSpan);

		HybridInteractionProblem problem = HYBRID_PROBLEM_NONE;

		//	The asset first: its seats and its approach decide where the deck ends and the road meets it.
		const float available = own_station ? Own_Straight_Approach(b,route,interval,station,across,span) : 3.4e38f;
		const int kind_index = Select_Bridge_Kind(b,edge.RoadClass,edge.Capabilities,span,available,&problem);
		if (kind_index < 0) {
			if (problem != HYBRID_PROBLEM_NONE) asset_problem = problem;
			b.Note_Reject(problem);
			continue;
		}
		const HybridBridgeKind & kind = b.P->BridgeKinds[kind_index];
		const float reserve = Max2(STATION_HALF_LENGTH_W * k.W,kind.RequiredApproachLength);

		Vector2 approach_a(0.0f,0.0f);
		Vector2 approach_b(0.0f,0.0f);
		if (!Station_Is_Hard_Valid(b,edge,route,station,across,span,reserve,&approach_a,&approach_b,&problem)) {
			if (problem != HYBRID_PROBLEM_NONE) station_problem = problem;
			b.Note_Reject(problem);
			continue;
		}
		if (own_station) Own_Approaches(window,ingress,&approach_a,&approach_b);

		float station_z = 0.0f;
		if (!b.Height_At(station,&station_z)) continue;

		float grade_a = 0.0f;
		float grade_b = 0.0f;
		float bank_z_a = 0.0f;
		float bank_z_b = 0.0f;
		if (!Approach_Grade(b,station,station_z,approach_a,&grade_a,&bank_z_a)) continue;
		if (!Approach_Grade(b,station,station_z,approach_b,&grade_b,&bank_z_b)) continue;

		/*
		**	Section 12.3's `abutment_z` is the road's elevation where it meets the deck, and the road meets it at
		**	the deck's two ends -- not at the approach reserve ends, which can stand a whole tier step away
		**	across a ravine.  A deck reaches past the obstruction by at least the kind's own abutment seats.  A
		**	river deck, which is level, also has to reach bank the road can meet it from: the channel is carved
		**	wider than the water (Phase 7's bank blend), and a deck that stopped at the water's edge would leave
		**	its abutment in the carved slope, below the reach of the road's fill.  So each end moves out along
		**	the axis until the ground there is within `grading.max_fill_height` of the deck -- the legacy bridge
		**	planner's own bank search, stated in the road's own vertical interval -- and the kind is re-chosen
		**	for the longer deck.  It may not reach past the straight approach.
		*/
		const bool level_deck = (window.Obstruction == HYBRID_OBSTRUCTION_RIVER);
		const float reach_step = Hybrid_Profile_Sample_Spacing(k);
		float reach_before = (0.5f * span) + kind.StartSeat;
		float reach_after = (0.5f * span) + kind.EndSeat;
		float bottom = 0.0f;
		float top = 0.0f;
		bool settled = false;
		bool refused = false;
		int chosen = kind_index;
		for (int pass = 0; (pass < 8) && !settled && !refused; ++pass) {

			const HybridBridgeKind & now = b.P->BridgeKinds[chosen];
			reach_before	= Max2(reach_before,(0.5f * span) + now.StartSeat);
			reach_after		= Max2(reach_after,(0.5f * span) + now.EndSeat);

			float seat_z_a = 0.0f;
			float seat_z_b = 0.0f;
			if (!b.Height_At(station - (across * reach_before),&seat_z_a) ||
				 !b.Height_At(station + (across * reach_after),&seat_z_b)) {
				refused = true;
				break;
			}
			if (!Bridge_Vertical_Geometry(b,window.Obstruction,now,station,seat_z_a,seat_z_b,&bottom,&top,
													&problem)) {
				if (problem != HYBRID_PROBLEM_NONE) geometry_problem = problem;
				refused = true;
				break;
			}
			if (!level_deck) {
				settled = true;
				break;
			}

			bool moved = false;
			for (int side = 0; (side < 2) && !refused; ++side) {
				float & reach = (side == 0) ? reach_before : reach_after;
				const float sense = (side == 0) ? -1.0f : 1.0f;
				for (;;) {
					float ground = 0.0f;
					const Vector2 at(station.X + (across.X * reach * sense),station.Y + (across.Y * reach * sense));
					if (!b.Height_At(at,&ground)) {
						refused = true;
						break;
					}
					if ((ground + b.P->MaxFillHeight) >= top) break;
					reach += reach_step;
					moved = true;
					if (reach > ((0.5f * span) + reserve)) {
						problem = HYBRID_PROBLEM_APPROACH_DOES_NOT_FIT;
						geometry_problem = problem;
						refused = true;
						break;
					}
				}
			}
			if (refused) break;

			//	A longer deck may need a kind with a longer span.
			const float deck = reach_before + reach_after;
			const float measured = deck - now.StartSeat - now.EndSeat;
			if (deck > now.MaxSpan) {
				const int again = Select_Bridge_Kind(b,edge.RoadClass,edge.Capabilities,measured,available,&problem);
				if ((again < 0) || (again == chosen)) {
					if (problem == HYBRID_PROBLEM_NONE) problem = HYBRID_PROBLEM_SPAN_TOO_LONG;
					asset_problem = problem;
					refused = true;
					break;
				}
				chosen = again;
				continue;
			}
			if (!moved) settled = true;
		}
		if (!settled || refused) {
			b.Note_Reject(problem);
			continue;
		}
		const HybridBridgeKind & built = b.P->BridgeKinds[chosen];
		const float half_before = Hybrid_Quantize_Position(reach_before);
		const float half_after = Hybrid_Quantize_Position(reach_after);

		if (!Abutments_Are_Legal(b,window,station,across,span,half_before,half_after)) {
			geometry_problem = HYBRID_PROBLEM_APPROACH_DOES_NOT_FIT;
			b.Note_Reject(HYBRID_PROBLEM_APPROACH_DOES_NOT_FIT);
			continue;
		}

		/*
		**	The road that crosses at this station: the routed road itself at the window's own station, a
		**	connector anywhere else.  A river deck is level at its top and both approaches are graded up to it,
		**	exactly as Phase 12 will grade them.  A canyon or ravine deck runs between the two road profiles at
		**	its ends and never dips toward the floor, so its two ends are whatever the road's own profile makes
		**	them, and its top and underside follow from them.
		*/
		HybridCrossingPath path;
		const Vector2 controls[2] = { approach_a,approach_b };
		float start_z = top;
		float end_z = top;
		if (!Judge_Crossing_Path(b,edge,route,interval,window,controls,2,station,across,half_before,half_after,
										 level_deck ? CROSSING_STRUCTURE_LEVEL : CROSSING_STRUCTURE_CARRIED,top,&path,
										 &start_z,&end_z,&problem)) {
			if (problem != HYBRID_PROBLEM_NONE) geometry_problem = problem;
			b.Note_Reject(problem);
			continue;
		}

		/*
		**	Every corner of that road is a fillet at the class radius, so its tightest measured radius is the
		**	class minimum and the curve term is zero.  The detour term is the plan's `added_route_length`: the
		**	road's length through the two approach-reserve endpoints less the straight distance between its ends.
		*/
		const HybridRoadClassSpec & spec = Hybrid_Road_Class_Spec(edge.RoadClass);
		const float measured_radius = spec.MinTurnRadiusW * k.W;
		const float score = Bridge_Candidate_Score(b,edge.RoadClass,built,span,bank_z_a,bank_z_b,grade_a,
																 grade_b,measured_radius,path.AddedLength);

		//	Lower score wins; a tie keeps the earlier station, which is the Section 0.3 stable order here.
		if (found && !(score < best_score)) continue;

		best = HybridInteraction();
		best.EdgeId					= edge.StableId;
		best.Obstruction			= window.Obstruction;
		best.ObstructionRecordId= window.ObstructionRecordId;
		best.WindowId				= window.StableId;
		best.Mandatory				= mandatory;
		best.Method					= HYBRID_METHOD_BRIDGE;
		best.RoadClass				= edge.RoadClass;
		best.Station				= station;
		best.Tangent				= across;
		best.ApproachA				= approach_a;
		best.ApproachB				= approach_b;
		best.Span					= span;
		best.ApproachReserve		= reserve;
		best.BridgeKind			= chosen;
		best.DeckReachA			= half_before;
		best.DeckReachB			= half_after;
		if (!level_deck) {
			top		= Hybrid_Quantize_Position(Max2(start_z,end_z));
			bottom	= Hybrid_Quantize_Position(Min2(start_z,end_z) - built.StructuralDepth);
		}
		best.DeckBottomZ			= bottom;
		best.DeckTopZ				= top;
		best.DeckStartZ			= Hybrid_Quantize_Position(start_z);
		best.DeckEndZ				= Hybrid_Quantize_Position(end_z);
		best.FirstPoint			= interval.FirstPoint;
		best.LastPoint				= interval.LastPoint;
		best.Path					= path;
		best.Envelope				= route.Envelope;
		best.Score					= score;
		best_score					= score;
		found							= true;
	}

	if (!found) {
		if (problem_out != nullptr) {
			if (geometry_problem != HYBRID_PROBLEM_NONE)		*problem_out = geometry_problem;
			else if (asset_problem != HYBRID_PROBLEM_NONE)	*problem_out = asset_problem;
			else															*problem_out = station_problem;
		}
		return false;
	}
	*out = best;
	return true;
}


/*
**	A ford.  Section 8 Phase 11 makes every one of its conditions explicit, and all of them have to hold:
**	fords enabled by the plan, not a mandatory bridge obligation, a light road class, a canonical
**	fordability query that exists and answers yes for the required capability, and the window's own ford
**	permission -- which Phase 6 computed from the frozen preflight metrics and this phase may not re-derive.
*/
static bool Compile_Ford(InteractionBuilder & b,const HybridTopologyEdge & edge,const HybridRoute & route,
								 const HybridRouteStructuralSpan & interval,const HybridCrossingWindow & window,
								 bool mandatory,HybridInteraction * out,HybridInteractionProblem * problem_out)
{
	const HybridConstants & k = b.P->Constants;

	if (!b.P->AllowFords || !b.P->FordCapabilityAvailable || mandatory) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_METHOD_UNAVAILABLE;
		return false;
	}

	//	"HIGHWAY_4 and PRIMARY_2 never ford."
	if ((edge.RoadClass != HYBRID_ROAD_SECONDARY_2) && (edge.RoadClass != HYBRID_ROAD_SERVICE_DIRT_2) &&
		 (edge.RoadClass != HYBRID_ROAD_TRAIL_1)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_METHOD_UNAVAILABLE;
		return false;
	}

	//	The capability the route actually needs: a Harvester corridor asks for the Harvester ford.
	const unsigned int needed = (edge.RoadClass == HYBRID_ROAD_SECONDARY_2)
										 ? HYBRID_WINDOW_METHOD_FORD_HARVESTER : HYBRID_WINDOW_METHOD_FORD_LIGHT;
	if (!window.Has_Method(needed)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_METHOD_UNAVAILABLE;
		return false;
	}

	//	A ford puts the vehicle on the riverbed, so the graded roadbed is what has to carry it.
	if (!Ground_Carries_Swept_Width(b,edge)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_CLEARANCE;
		return false;
	}

	const Vector2 across = Travel_Axis(window,route.Points[interval.FirstPoint]);
	const float reserve = Max2(STATION_HALF_LENGTH_W * k.W,window.ApproachLength);
	Vector2 approach_a(0.0f,0.0f);
	Vector2 approach_b(0.0f,0.0f);
	if (!Station_Is_Hard_Valid(b,edge,route,window.Center,across,window.Span,reserve,&approach_a,
										&approach_b,problem_out)) return false;

	*out = HybridInteraction();
	out->EdgeId					= edge.StableId;
	out->Obstruction			= window.Obstruction;
	out->ObstructionRecordId= window.ObstructionRecordId;
	out->WindowId				= window.StableId;
	out->Method					= HYBRID_METHOD_FORD;
	out->RoadClass				= edge.RoadClass;
	out->Station				= Quantized(window.Center);
	out->Tangent				= across;
	out->ApproachA				= approach_a;
	out->ApproachB				= approach_b;
	out->Span					= window.Span;
	out->ApproachReserve		= reserve;
	out->FirstPoint			= interval.FirstPoint;
	out->LastPoint				= interval.LastPoint;
	out->Envelope				= route.Envelope;
	return true;
}


/*
**	The realized macro terrain, as the shared local-pass builder wants it.
*/
static bool Interaction_Height_Sample(const Vector2 & world,float * height_out,void * context)
{
	InteractionBuilder * b = (InteractionBuilder *)context;
	if ((b == nullptr) || (height_out == nullptr)) return false;
	return b->Height_At(world,height_out);
}


static HybridLocalPassParams Pass_Params(const InteractionBuilder & b,const HybridTopologyEdge & edge)
{
	HybridLocalPassParams params;
	params.Constants					= b.P->Constants;
	params.MaxCutDepth				= b.P->MaxCutDepth;
	params.MaxFillHeight				= b.P->MaxFillHeight;
	//	The envelope this edge must carry, so the floor test asks for the clearance this road actually needs.
	params.SweptVehicleWidth		= Required_Swept_Width(b,edge.Capabilities);
	params.MinVehicleLaneClearance= b.P->MinVehicleLaneClearance;

	//	The class actually being built here, not the `PRIMARY_2` the window was first proved for.
	params.PassClass					= edge.RoadClass;
	return params;
}


//	Say what the pass broke in this phase's own vocabulary.
static HybridInteractionProblem Pass_Problem(HybridLocalPassFailure failure)
{
	switch (failure) {
		case HYBRID_LOCAL_PASS_NO_HEIGHT:		return HYBRID_PROBLEM_NO_HARD_VALID_STATION;
		case HYBRID_LOCAL_PASS_RUN:
		case HYBRID_LOCAL_PASS_GRADE:
		case HYBRID_LOCAL_PASS_FLOOR_GRADE:		return HYBRID_PROBLEM_GRADE_TOO_STEEP;
		case HYBRID_LOCAL_PASS_TURN_RADIUS:		return HYBRID_PROBLEM_TURN_TOO_TIGHT;
		case HYBRID_LOCAL_PASS_CROSS_SLOPE:		return HYBRID_PROBLEM_CROSS_SLOPE;
		case HYBRID_LOCAL_PASS_CUT_FILL:			return HYBRID_PROBLEM_CUT_OR_FILL;
		case HYBRID_LOCAL_PASS_FLOOR_WIDTH:		return HYBRID_PROBLEM_CLEARANCE;
		default:											break;
	}
	return HYBRID_PROBLEM_METHOD_UNAVAILABLE;
}


/*
**	A preplanned surface pass or ramp pass: the road descends and climbs across the obstruction on the
**	ground rather than over it.  The plan asks for the *full swept route* to be sampled at `min(W/2,A/3)`
**	with the road-class max grade, turn radius, cross slope and vehicle clearance required at every sample,
**	and only at a window Phase 6 already marked pass-capable.
*/
static bool Compile_Pass(InteractionBuilder & b,const HybridTopologyEdge & edge,const HybridRoute & route,
								 const HybridRouteStructuralSpan & interval,const HybridCrossingWindow & window,
								 HybridInteractionMethod method,HybridInteraction * out,
								 HybridInteractionProblem * problem_out)
{
	const HybridConstants & k = b.P->Constants;
	if (!window.Has_Method(HYBRID_WINDOW_METHOD_PASS)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_METHOD_UNAVAILABLE;
		return false;
	}

	//	A pass drives across the ground, so the graded roadbed is what has to carry the vehicle.
	if (!Ground_Carries_Swept_Width(b,edge)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_CLEARANCE;
		return false;
	}

	const Vector2 across = Travel_Axis(window,route.Points[interval.FirstPoint]);
	const float reserve = Max2(STATION_HALF_LENGTH_W * k.W,window.ApproachLength);
	Vector2 approach_a(0.0f,0.0f);
	Vector2 approach_b(0.0f,0.0f);
	if (!Station_Is_Hard_Valid(b,edge,route,window.Center,across,window.Span,reserve,&approach_a,
										&approach_b,problem_out)) return false;

	/*
	**	The pass Phase 6 froze, revalidated here on the realized terrain at this phase's own spacing.
	**
	**	What is validated is the *stored centerline*, not the straight line between the two approaches: a road
	**	does not cross a canyon by driving off the rim, and judging that line is what made this method
	**	unreachable.  Phase 6 proved a candidate against its provisional height and froze the geometry; this
	**	phase re-reads every control elevation from the macro terrain and re-measures the grade, the corners,
	**	the cross slope, the floor and each sample's `RoadVerticalInterval` for itself at `min(W/2, A/3)`, so a
	**	`pass_capable` flag is never permission to skip validation.  What it must not do is invent a second
	**	path: two phases each drawing their own centerline for one crossing is how they come to disagree, and
	**	Phase 12 realizes this one.
	*/
	if (window.Pass.ControlCount < 2) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_METHOD_UNAVAILABLE;
		return false;
	}

	//	The frozen shape decides which method this is, not the caller: a canyon never becomes a ramp pass.
	const HybridLocalPassKind expected = (method == HYBRID_METHOD_RAMP_PASS) ? HYBRID_LOCAL_PASS_RAMP
																							  : HYBRID_LOCAL_PASS_FLOOR;
	if (window.Pass.Kind != expected) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_METHOD_UNAVAILABLE;
		return false;
	}

	const float spacing = Max2(Min2(PASS_SAMPLE_W * k.W,k.A / PASS_SAMPLE_A),k.Q);
	HybridLocalPassFailure why = HYBRID_LOCAL_PASS_OK;
	if (!Hybrid_Validate_Local_Pass(Pass_Params(b,edge),window.Pass,spacing,
											  Interaction_Height_Sample,&b,&why)) {
		if (problem_out != nullptr) *problem_out = Pass_Problem(why);
		return false;
	}

	/*
	**	The frozen controls in the order this road drives them: Phase 6 built the pass from one side of its
	**	window, and a road may arrive from either.  The road reaches the first control and leaves from the
	**	last through the route owner's own fillets -- a band pass leaves the cliff some distance along it from
	**	where it entered, and its far approach is not the point opposite the station.
	*/
	Vector2 controls[HYBRID_LOCAL_PASS_MAX_CONTROLS];
	const int control_count = window.Pass.ControlCount;
	const Vector2 & ingress = route.Points[interval.FirstPoint];
	//	Which end is on the bank the road arrives from is a matter of side, not of distance: a band pass's far
	//	approach can stand a long way along the cliff from the road and still be on the far side of it.
	const Vector2 side(-window.Tangent.Y,window.Tangent.X);
	const float ingress_side = ((ingress.X - window.Center.X) * side.X) + ((ingress.Y - window.Center.Y) * side.Y);
	const float first_side = ((window.Pass.Controls[0].X - window.Center.X) * side.X) +
									 ((window.Pass.Controls[0].Y - window.Center.Y) * side.Y);
	const bool reversed = ((ingress_side > 0.0f) != (first_side > 0.0f));
	for (int c = 0; c < control_count; ++c) {
		controls[c] = window.Pass.Controls[reversed ? (control_count - 1 - c) : c];
	}

	HybridCrossingPath path;
	if (!Judge_Crossing_Path(b,edge,route,interval,window,controls,control_count,window.Center,across,0.0f,0.0f,
									 CROSSING_STRUCTURE_NONE,0.0f,&path,nullptr,nullptr,problem_out)) {
		return false;
	}

	*out = HybridInteraction();
	out->EdgeId					= edge.StableId;
	out->Obstruction			= window.Obstruction;
	out->ObstructionRecordId= window.ObstructionRecordId;
	out->WindowId				= window.StableId;
	out->Method					= method;
	out->RoadClass				= edge.RoadClass;
	out->Station				= Quantized(window.Center);
	out->Tangent				= across;

	//	The compiled ends are the frozen pass's own, in travel order; Phase 12 realizes what Phase 6 froze.
	out->ApproachA				= controls[0];
	out->ApproachB				= controls[control_count - 1];
	(void)approach_a;
	(void)approach_b;
	out->Path					= path;
	out->Span					= window.Span;
	out->ApproachReserve		= reserve;
	out->FirstPoint			= interval.FirstPoint;
	out->LastPoint				= interval.LastPoint;
	out->Envelope				= route.Envelope;
	return true;
}


/*
**	A road cut through an explicit cliff or ridge face.  Section 8 Phase 11's four conditions, in order:
**	the minimum centerline correction fits `grading.max_cut_depth`, no sample needs fill above
**	`grading.max_fill_height`, the cut stays inside `RoadDeformationHalfWidth(road_class)`, and the outer
**	blend reaches untouched terrain without meeting another immutable wall, base or Tiberium core.
*/
static bool Compile_Road_Cut(InteractionBuilder & b,const HybridTopologyEdge & edge,
									  const HybridRoute & route,const HybridRouteStructuralSpan & interval,
									  const HybridCrossingWindow & window,HybridInteraction * out,
									  HybridInteractionProblem * problem_out)
{
	const HybridConstants & k = b.P->Constants;
	//	Planned for *this* class: the Phase-6 cut test depends on the class, so another class's bit is no
	//	evidence at all.  What follows then revalidates it on the realized terrain for this road.
	if (!window.Has_Method(HYBRID_WINDOW_METHOD_CUT) || !window.Cut_Planned_For(edge.RoadClass)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_METHOD_UNAVAILABLE;
		return false;
	}

	//	A cut leaves the road on graded ground, so the roadbed is what has to carry the vehicle.
	if (!Ground_Carries_Swept_Width(b,edge)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_CLEARANCE;
		return false;
	}

	const Vector2 across = Travel_Axis(window,route.Points[interval.FirstPoint]);
	const float reserve = Max2(STATION_HALF_LENGTH_W * k.W,window.ApproachLength);
	Vector2 approach_a(0.0f,0.0f);
	Vector2 approach_b(0.0f,0.0f);
	if (!Station_Is_Hard_Valid(b,edge,route,window.Center,across,window.Span,reserve,&approach_a,
										&approach_b,problem_out)) return false;
	Own_Approaches(window,route.Points[interval.FirstPoint],&approach_a,&approach_b);

	float z_a = 0.0f;
	float z_b = 0.0f;
	if (!b.Height_At(approach_a,&z_a) || !b.Height_At(approach_b,&z_b)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_NO_HARD_VALID_STATION;
		return false;
	}

	const float spacing = Max2(Min2(PASS_SAMPLE_W * k.W,k.A / PASS_SAMPLE_A),k.Q);
	const float length = (approach_b - approach_a).Length();
	const int steps = (int)::ceil((double)(length / spacing));
	const int count = (steps > 0) ? steps : 1;
	const float half_deformation = Hybrid_Road_Deformation_Half_Width(edge.RoadClass,k.W,k.A);
	const Vector2 axis = Unit(approach_b - approach_a);
	const Vector2 normal(-axis.Y,axis.X);

	float deepest_cut = 0.0f;
	for (int i = 0; i <= count; ++i) {

		const float ratio = (float)i / (float)count;
		const Vector2 point(approach_a.X + ((approach_b.X - approach_a.X) * ratio),
								  approach_a.Y + ((approach_b.Y - approach_a.Y) * ratio));

		float ground = 0.0f;
		if (!b.Height_At(point,&ground)) {
			if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_NO_HARD_VALID_STATION;
			return false;
		}

		//	The centerline the cut has to carry: the straight profile between the two approach elevations.
		const float profile = z_a + ((z_b - z_a) * ratio);
		const float correction = profile - ground;

		//	A cut or fill the class does not allow is exactly that, and is no longer reported as a steep grade.
		if (correction < 0.0f) {
			const float cut = -correction;
			if (cut > b.P->MaxCutDepth) {
				if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_CUT_OR_FILL;
				return false;
			}
			if (cut > deepest_cut) deepest_cut = cut;
		} else if (correction > b.P->MaxFillHeight) {
			if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_CUT_OR_FILL;
			return false;
		}

		/*
		**	The outer blend has to reach untouched terrain: at the edge of the road's own deformation width
		**	plus the blend, the ground must not be another immutable wall, a base or a Tiberium core.
		*/
		const float blend = half_deformation + Hybrid_Road_Outer_Blend_Width(k.W,k.A);
		for (int side = -1; side <= 1; side += 2) {

			const Vector2 edge_point(point.X + (normal.X * blend * (float)side),
											 point.Y + (normal.Y * blend * (float)side));
			if (!Point_Is_Placeable(b,edge_point)) {
				if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_RESERVE_CONFLICT;
				return false;
			}
			if (Hybrid_Point_In_Immutable_Landform(*b.Landforms,edge_point) &&
				 (window.ObstructionRecordId != 0)) {
				//	Another wall, not the one this cut crosses: the blend would have nowhere to land.
				if ((edge_point - window.Center).Length() > (window.Span + k.A)) {
					if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_RESERVE_CONFLICT;
					return false;
				}
			}
			if (Reserve_Hits_Unrelated_Reserve(b,edge,edge_point,edge_point)) {
				if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_RESERVE_CONFLICT;
				return false;
			}
		}
	}

	//	The road through the cut is its own straight interval; Section 12.1 has to be able to grade it.
	HybridCrossingPath path;
	const Vector2 controls[2] = { approach_a,approach_b };
	if (!Judge_Crossing_Path(b,edge,route,interval,window,controls,2,window.Center,across,0.0f,0.0f,
									 CROSSING_STRUCTURE_NONE,0.0f,&path,nullptr,nullptr,problem_out)) {
		return false;
	}

	*out = HybridInteraction();
	out->EdgeId					= edge.StableId;
	out->Obstruction			= window.Obstruction;
	out->ObstructionRecordId= window.ObstructionRecordId;
	out->WindowId				= window.StableId;
	out->Method					= HYBRID_METHOD_ROAD_CUT;
	out->RoadClass				= edge.RoadClass;
	out->Station				= Quantized(window.Center);
	out->Tangent				= across;
	out->ApproachA				= approach_a;
	out->ApproachB				= approach_b;
	out->Span					= window.Span;
	out->ApproachReserve		= reserve;
	out->CutDepth				= Hybrid_Quantize_Position(deepest_cut);
	out->Path					= path;
	out->FirstPoint			= interval.FirstPoint;
	out->LastPoint				= interval.LastPoint;
	out->Envelope				= route.Envelope;
	return true;
}


/*
**	A tunnel.  Section 12.5: never mandatory, a fallback only, and only when every one of its conditions
**	holds.  The module sequence is the shortest sufficient one, tie broken by `StableId`; the geometry is
**	whatever the registered modules already describe, and this phase invents none of it.
*/
static bool Compile_Tunnel(InteractionBuilder & b,const HybridTopologyEdge & edge,const HybridRoute & route,
									const HybridRouteStructuralSpan & interval,const HybridCrossingWindow & window,
									HybridInteraction * out,HybridInteractionProblem * problem_out)
{
	const HybridConstants & k = b.P->Constants;

	if (!b.P->TunnelsEnabled || !b.P->TunnelAvailable ||
		 !window.Has_Method(HYBRID_WINDOW_METHOD_TUNNEL)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_METHOD_UNAVAILABLE;
		return false;
	}

	const HybridRoadClassSpec & spec = Hybrid_Road_Class_Spec(edge.RoadClass);
	const float max_grade = spec.MaxGradeDegrees * PI_OVER_180;
	//	Section 12.5's bore holds this edge's own envelope plus the lane clearance either side.
	const float swept_width = Required_Swept_Width(b,edge.Capabilities) + (2.0f * b.P->MinVehicleLaneClearance);
	const float swept_height = Hybrid_Overpass_Clear_Gap(b.P->HeavyVehicleHeight,b.P->WorldUnitsPerMeter);

	const Vector2 across = Travel_Axis(window,route.Points[interval.FirstPoint]);
	const float reserve = Max2(STATION_HALF_LENGTH_W * k.W,window.ApproachLength);
	Vector2 approach_a(0.0f,0.0f);
	Vector2 approach_b(0.0f,0.0f);
	if (!Station_Is_Hard_Valid(b,edge,route,window.Center,across,window.Span,reserve,&approach_a,
										&approach_b,problem_out)) return false;
	Own_Approaches(window,route.Points[interval.FirstPoint],&approach_a,&approach_b);

	//	Both portals clear of water, base, Tiberium and any bridge approach reserve already compiled.
	for (int side = 0; side < 2; ++side) {

		const Vector2 & portal = (side == 0) ? approach_a : approach_b;
		if (Hybrid_Point_In_Water(*b.Hydrology,portal)) {
			if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_RESERVE_CONFLICT;
			return false;
		}
		if (Reserve_Hits_Unrelated_Reserve(b,edge,portal,portal)) {
			if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_RESERVE_CONFLICT;
			return false;
		}
		for (int i = 0; i < b.Out->Interactions.Count(); ++i) {
			const HybridInteraction & other = b.Out->Interactions[i];
			if (other.Method != HYBRID_METHOD_BRIDGE) continue;
			if (Distance_To_Segment(portal,other.ApproachA,other.ApproachB,nullptr) >
				 other.ApproachReserve) continue;
			if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_RESERVE_CONFLICT;
			return false;
		}
	}

	//	The shortest sufficient module sequence, then the lower StableId.
	const float bore = (approach_b - approach_a).Length();
	int best = -1;
	float best_length = 0.0f;
	for (int i = 0; i < b.P->TunnelModules.Count(); ++i) {

		const HybridTunnelModule & module_kind = b.P->TunnelModules[i];
		if (!module_kind.PortalCompatible) continue;
		if (!(module_kind.Length > 0.0f)) continue;
		if ((module_kind.ClearWidth > 0.0f) && (module_kind.ClearWidth < swept_width)) continue;
		if ((module_kind.ClearHeight > 0.0f) && (module_kind.ClearHeight < swept_height)) continue;
		if ((module_kind.MaxGradeRadians > 0.0f) && (module_kind.MaxGradeRadians < max_grade)) continue;

		const int modules = (int)::ceil((double)(bore / module_kind.Length));
		const float total = (float)((modules > 0) ? modules : 1) * module_kind.Length;

		if (best < 0) {
			best			= i;
			best_length	= total;
			continue;
		}
		if (total < best_length) {
			best			= i;
			best_length	= total;
		} else if ((total == best_length) && (module_kind.StableId < b.P->TunnelModules[best].StableId)) {
			best = i;
		}
	}
	if (best < 0) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_METHOD_UNAVAILABLE;
		return false;
	}

	//	The road to each portal; the bore between them is the structure, and carries no ground of its own.
	HybridCrossingPath path;
	const Vector2 controls[2] = { approach_a,approach_b };
	const Vector2 middle((approach_a.X + approach_b.X) * 0.5f,(approach_a.Y + approach_b.Y) * 0.5f);
	float portal_z_a = 0.0f;
	float portal_z_b = 0.0f;
	if (!Judge_Crossing_Path(b,edge,route,interval,window,controls,2,middle,across,0.5f * bore,0.5f * bore,
									 CROSSING_STRUCTURE_CARRIED,0.0f,&path,&portal_z_a,&portal_z_b,problem_out)) {
		return false;
	}

	*out = HybridInteraction();
	out->EdgeId					= edge.StableId;
	out->Obstruction			= window.Obstruction;
	out->ObstructionRecordId= window.ObstructionRecordId;
	out->WindowId				= window.StableId;
	out->Method					= HYBRID_METHOD_TUNNEL;
	out->RoadClass				= edge.RoadClass;
	out->Station				= Quantized(window.Center);
	out->Tangent				= across;
	out->ApproachA				= approach_a;
	out->ApproachB				= approach_b;
	out->Span					= window.Span;
	out->ApproachReserve		= reserve;
	out->TunnelModule			= best;
	out->TunnelLength			= Hybrid_Quantize_Position(best_length);
	out->DeckReachA			= Hybrid_Quantize_Position(0.5f * bore);
	out->DeckReachB			= Hybrid_Quantize_Position(0.5f * bore);
	out->Path					= path;
	out->DeckStartZ			= Hybrid_Quantize_Position(portal_z_a);
	out->DeckEndZ				= Hybrid_Quantize_Position(portal_z_b);
	out->FirstPoint			= interval.FirstPoint;
	out->LastPoint				= interval.LastPoint;
	out->Envelope				= route.Envelope;
	return true;
}


/*
**	One road-obstruction conflict, through the fixed method order its obstruction kind decides.
**
**	A mandatory bridge obligation skips the canyon fallbacks entirely -- "it must compile as a bridge or the
**	edge/attempt fails" -- and no obligation is ever moved to a different obstruction.  When every method in
**	the order fails, the solver asks for one more Phase-10 envelope if the route has not spent them all,
**	which is the only rerouting this phase may cause.
*/
static bool Solve_Obstruction(InteractionBuilder & b,const HybridTopologyEdge & edge,
										const HybridRoute & route,const HybridRouteStructuralSpan & interval,
										HybridInteractionResult * result)
{
	const HybridCrossingWindow * window = b.Hydrology->Find_Window(interval.WindowId);
	if (window == nullptr) {
		StringClass detail;
		detail.Format("route %u names crossing window %u, which no longer exists",edge.StableId,
						  interval.WindowId);
		return Fail(result,HYBRID_INTERACTION_INVALID_INPUT,detail.Peek_Buffer());
	}

	//	Is this crossing a frozen Phase-9 obligation, and is it obliged to be a bridge?
	bool mandatory = false;
	Vector2 centre = window->Center;
	for (int w = 0; w < edge.Waypoints.Count(); ++w) {
		const HybridTopologyNode * node = b.Topology->Find_Node(edge.Waypoints[w]);
		if (node == nullptr) continue;
		if (node->WindowId != window->StableId) continue;
		if (node->Kind != HYBRID_NODE_CROSSING_OBLIGATION) continue;
		mandatory	= true;
		centre		= node->Position;					// the interval centres on the frozen waypoint
	}

	HybridInteraction solution;
	int stations = 0;
	bool solved = false;
	b.Clear_Rejects();

	/*
	**	Each method in the order reports its own reason, and the *primary* method's reason is the one the
	**	issue record keeps.  Letting a fallback overwrite it loses the diagnosis that matters: a canyon
	**	crossing whose every bridge kind was too short would be recorded as "no tunnel available", which
	**	names the last thing tried rather than the thing that failed.
	*/
	HybridInteractionProblem primary = HYBRID_PROBLEM_NONE;
	HybridInteractionProblem fallback = HYBRID_PROBLEM_NONE;

	if ((window->Obstruction == HYBRID_OBSTRUCTION_RIVER) ||
		 (window->Obstruction == HYBRID_OBSTRUCTION_LAKE) ||
		 (window->Obstruction == HYBRID_OBSTRUCTION_COAST)) {

		//	Road x river: bridge, then ford, then one more envelope, then fail.
		solved = Compile_Bridge(b,edge,route,interval,*window,mandatory,centre,&solution,&primary,&stations);
		if (!solved) solved = Compile_Ford(b,edge,route,interval,*window,mandatory,&solution,&fallback);

	} else if ((window->Obstruction == HYBRID_OBSTRUCTION_CANYON) ||
				  (window->Obstruction == HYBRID_OBSTRUCTION_RAVINE)) {

		//	Road x canyon/ravine: bridge, then a surface pass, then a tunnel, then one more envelope.
		solved = Compile_Bridge(b,edge,route,interval,*window,mandatory,centre,&solution,&primary,&stations);
		if (!solved && !mandatory) {
			solved = Compile_Pass(b,edge,route,interval,*window,HYBRID_METHOD_SURFACE_PASS,&solution,
										 &fallback);
			if (!solved) solved = Compile_Tunnel(b,edge,route,interval,*window,&solution,&fallback);
		}

	} else {

		//	Road x cliff/ridge: a road cut, then a ramp pass, then a tunnel, then one more envelope.
		solved = Compile_Road_Cut(b,edge,route,interval,*window,&solution,&primary);
		if (!solved) solved = Compile_Pass(b,edge,route,interval,*window,HYBRID_METHOD_RAMP_PASS,&solution,
													 &fallback);
		if (!solved) solved = Compile_Tunnel(b,edge,route,interval,*window,&solution,&fallback);
	}

	const HybridInteractionProblem problem = (primary != HYBRID_PROBLEM_NONE) ? primary : fallback;

	if (solved) {
		solution.StableId		= b.NextStableId++;
		solution.FallbackFrom= primary;				// NONE when the first method in the order compiled
		b.Out->Interactions.Add(solution);
		return true;
	}

	if (!Note_Problem(b,edge.StableId,0,window->Obstruction,window->StableId,mandatory,problem,stations,
							window->Center)) {
		StringClass detail;
		detail.Format("edge %u has more than %d unsolved crossings",edge.StableId,b.P->MaxProblemsPerEdge);
		return Fail(result,HYBRID_INTERACTION_LIMIT,detail.Peek_Buffer());
	}

	/*
	**	One more Phase-10 corridor envelope, if the route has not spent them all.  This is the last phase
	**	allowed to ask for one, and asking is counted.
	*/
	if (route.Envelope < 2) {
		b.Out->RerouteRequests++;
		if (result != nullptr) result->RerouteEdgeId = edge.StableId;
	}

	StringClass rejects;
	for (int r = HYBRID_PROBLEM_NONE + 1; r < HYBRID_PROBLEM_COUNT; ++r) {
		if (b.Rejects[r] == 0) continue;
		StringClass one;
		one.Format("%s %d, ",Hybrid_Interaction_Problem_Name((HybridInteractionProblem)r),b.Rejects[r]);
		rejects += one;
	}

	StringClass detail;
	detail.Format("edge %u (%s) cannot cross %s window %u: %s (%d stations tried, envelope %d; refused by "
					  "%s)",edge.StableId,Hybrid_Road_Class_Name(edge.RoadClass),
					  Hybrid_Obstruction_Kind_Name(window->Obstruction),window->StableId,
					  Hybrid_Interaction_Problem_Name(problem),stations,route.Envelope,
					  (rejects.Get_Length() > 0) ? rejects.Peek_Buffer() : "nothing recorded");

	if (mandatory) return Fail(result,HYBRID_INTERACTION_OBLIGATION_LOST,detail.Peek_Buffer());
	if (edge.Required) return Fail(result,HYBRID_INTERACTION_NO_SOLUTION,detail.Peek_Buffer());

	//	An optional link that cannot cross is dropped, exactly as Phase 10 drops one it cannot route.
	WWDEBUG_SAY(("Hybrid Phase 11: optional edge %u has no crossing solution and is dropped\r\n",
					 edge.StableId));
	return true;
}

/***********************************************************************************************
**	Road x road.
***********************************************************************************************/

//	Where two segments cross, if they do.
void HybridRoadCrossingCensus::Reset(void)
{
	for (int i = 0; i < HYBRID_ROAD_MEETING_COUNT; ++i) Meetings[i] = 0;
	IncidentalNearShared	= 0;
	IncidentalBeyondJunction = 0;
	WaypointShared			= 0;
	FailedEndKindA			= -1;
	FailedEndKindB			= -1;
	FailedWaypointShared	= false;
	FailedMeeting			= -1;
	FailedKindA				= -1;
	FailedKindB				= -1;
	FailedAngle				= 0.0f;
	FailedEndA				= 0.0f;
	FailedEndB				= 0.0f;
	JunctionProblem		= 0;
	OverpassProblem[0]	= 0;
	OverpassProblem[1]	= 0;
}


static bool Segments_Cross(const Vector2 & a0,const Vector2 & a1,const Vector2 & b0,const Vector2 & b1,
									Vector2 * point_out)
{
	const Vector2 r = a1 - a0;
	const Vector2 s = b1 - b0;
	const float denominator = (r.X * s.Y) - (r.Y * s.X);
	if ((float)fabs((double)denominator) <= 1.0e-9f) return false;		// parallel or degenerate

	const Vector2 delta = b0 - a0;
	const float t = ((delta.X * s.Y) - (delta.Y * s.X)) / denominator;
	const float u = ((delta.X * r.Y) - (delta.Y * r.X)) / denominator;
	if ((t < 0.0f) || (t > 1.0f) || (u < 0.0f) || (u > 1.0f)) return false;

	if (point_out != nullptr) *point_out = Vector2(a0.X + (r.X * t),a0.Y + (r.Y * t));
	return true;
}


//	How much centerline a route has either side of one of its own samples.
static void Arm_Lengths(const HybridRoute & route,const Vector2 & point,float * before_out,float * after_out)
{
	*before_out	= 0.0f;
	*after_out	= 0.0f;
	if (route.Stations.Count() < 2) return;

	float nearest = 3.4e38f;
	int at = 0;
	for (int i = 0; i < route.Points.Count(); ++i) {
		const float distance = (route.Points[i] - point).Length();
		if (distance >= nearest) continue;
		nearest	= distance;
		at			= i;
	}
	const float total = route.Stations[route.Stations.Count() - 1];
	*before_out	= route.Stations[at];
	*after_out	= total - route.Stations[at];
}


/*
**	Is `point` inside a compiled bridge's or tunnel's approach reserve?  The reserve is the straight road the
**	structure owns, from `ApproachA` to `ApproachB` through the deck or bore, at that road's own roadbed width.
**	The test this replaces used the reserve's *length* as a radius around the whole straight, which refused a
**	junction 23 units beyond the end of a bridge's approach -- outside the reserve altogether.
*/
static bool Inside_Approach_Reserve(const InteractionBuilder & b,const Vector2 & point)
{
	for (int i = 0; i < b.Out->Interactions.Count(); ++i) {
		const HybridInteraction & other = b.Out->Interactions[i];
		if ((other.Method != HYBRID_METHOD_BRIDGE) && (other.Method != HYBRID_METHOD_TUNNEL)) continue;
		const float half = 0.5f * Hybrid_Roadbed_Width(other.RoadClass,b.P->Constants.W);
		if (Distance_To_Segment(point,other.ApproachA,other.ApproachB,nullptr) <= half) return true;
	}
	return false;
}


/*
**	Section 8 Phase 11's at-grade junction conditions, all five of them.
*/
static bool At_Grade_Is_Legal(InteractionBuilder & b,const HybridTopologyEdge & edge_a,
										const HybridRoute & route_a,const HybridTopologyEdge & edge_b,
										const HybridRoute & route_b,const Vector2 & point,
										const Vector2 & heading_a,const Vector2 & heading_b,
										HybridInteractionProblem * problem_out)
{
	const HybridConstants & k = b.P->Constants;
	const HybridRoadClassSpec & spec_a = Hybrid_Road_Class_Spec(edge_a.RoadClass);
	const HybridRoadClassSpec & spec_b = Hybrid_Road_Class_Spec(edge_b.RoadClass);

	/*
	**	D-027 / D-029: a junction here was rejected -- its structural pin the grid cannot realize, or its height in a profile
	**	that cannot hold it -- and the overpass order follows.
	*/
	if (b.Is_Excluded(HYBRID_METHOD_AT_GRADE_JUNCTION,edge_a.StableId,edge_b.StableId,0,false,Quantized(point),problem_out)) {
		return false;
	}

	//	1: the crossing angle stays inside 30 to 150 degrees.
	float dot = (heading_a.X * heading_b.X) + (heading_a.Y * heading_b.Y);
	dot = Clamp2((float)fabs((double)dot),0.0f,1.0f);
	const float angle = (float)::acos((double)dot) / PI_OVER_180;
	if ((angle < JUNCTION_MIN_DEGREES) || (angle > JUNCTION_MAX_DEGREES)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_TURN_TOO_TIGHT;
		return false;
	}

	//	2: the intersection is outside every compiled bridge or tunnel approach reserve.
	if (Inside_Approach_Reserve(b,point)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_RESERVE_CONFLICT;
		return false;
	}

	//	2b (12.6.10): ...and outside every active interchange footprint expanded by RoadOuterBlendWidth.
	if (Near_Interchange_Footprint(b,point,Hybrid_Road_Outer_Blend_Width(k.W,k.A))) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_INTERCHANGE_FOOTPRINT;
		return false;
	}

	//	3: each road keeps max(4W, its own turn radius) of centerline on both sides of the junction.
	const float needed_a = Hybrid_Junction_Arm_Reach(edge_a.RoadClass,k.W);
	const float needed_b = Hybrid_Junction_Arm_Reach(edge_b.RoadClass,k.W);
	float before = 0.0f;
	float after = 0.0f;
	Arm_Lengths(route_a,point,&before,&after);
	if ((before < needed_a) || (after < needed_a)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_APPROACH_DOES_NOT_FIT;
		return false;
	}
	Arm_Lengths(route_b,point,&before,&after);
	if ((before < needed_b) || (after < needed_b)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_APPROACH_DOES_NOT_FIT;
		return false;
	}

	/*
	**	4: the local vertical correction fits the cut and fill allowance.  The junction surface is one
	**	elevation both roads have to reach, so each arm's ground has to be gradeable to it within that
	**	road's own max grade over the arm length the junction reserves.
	*/
	float junction_z = 0.0f;
	if (!b.Height_At(point,&junction_z)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_NO_HARD_VALID_STATION;
		return false;
	}
	const HybridVerticalInterval junction_interval = Hybrid_Road_Vertical_Interval(junction_z,
																									b.P->MaxCutDepth,
																									b.P->MaxFillHeight);
	for (int arm = 0; arm < 4; ++arm) {

		const Vector2 & heading = (arm < 2) ? heading_a : heading_b;
		const float sign = ((arm % 2) == 0) ? 1.0f : -1.0f;
		const float reach = (arm < 2) ? needed_a : needed_b;
		const float max_grade = ((arm < 2) ? spec_a.MaxGradeDegrees : spec_b.MaxGradeDegrees) * PI_OVER_180;

		const Vector2 end(point.X + (heading.X * reach * sign),point.Y + (heading.Y * reach * sign));
		float end_z = 0.0f;
		if (!b.Height_At(end,&end_z)) continue;					// off the realized ground: not this test's call
		const HybridVerticalInterval end_interval = Hybrid_Road_Vertical_Interval(end_z,b.P->MaxCutDepth,
																								  b.P->MaxFillHeight);
		if (!Hybrid_Grade_Compatible(junction_interval,end_interval,reach,max_grade)) {
			if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_GRADE_TOO_STEEP;
			return false;
		}
	}

	//	5: the shared junction surface satisfies the larger of the two cross-slope limits.
	const float max_cross = Max2(spec_a.MaxCrossSlopeDegrees,spec_b.MaxCrossSlopeDegrees) * PI_OVER_180;
	const float half = 0.5f * Max2(Hybrid_Edge_Roadbed_Width(edge_a,k.W),Hybrid_Edge_Roadbed_Width(edge_b,k.W));
	const Vector2 normal(-heading_a.Y,heading_a.X);
	float left = 0.0f;
	float right = 0.0f;
	if (b.Height_At(point + (normal * half),&left) && b.Height_At(point - (normal * half),&right)) {

		const HybridVerticalInterval left_interval = Hybrid_Road_Vertical_Interval(left,b.P->MaxCutDepth,
																								  b.P->MaxFillHeight);
		const HybridVerticalInterval right_interval = Hybrid_Road_Vertical_Interval(right,b.P->MaxCutDepth,
																									b.P->MaxFillHeight);
		if (!Hybrid_Grade_Compatible(left_interval,right_interval,2.0f * half,max_cross)) {
			if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_CROSS_SLOPE;
			return false;
		}
	}
	return true;
}


/*
**	Section 8 Phase 11's overpass construction rule, with its one permitted swap.
**
**	The lower road keeps its frozen horizontal spline and its Section-7.1 vertical intervals.  `delta_z` is
**	the extra deck elevation Section 12.3 asks for after structural depth, and each side reserves
**	`L_ramp = max(10W, |delta_z| / tan(upper_class.max_grade))`.  The profile is one smoothstep ramp over
**	exactly `L_ramp` a side with a level deck through the crossing, sampled at `min(W/3,A/4)`, and every
**	sample has to hold max grade, cross slope, the cut/fill interval and the clearance.
*/
/*
**	D-027: a road's actual paved roadbed either side of its travel at `point` -- shoulder included, a ramp's effective
**	asymmetric edges (D-022, D-024), a HIGHWAY_4 widened to the merge/diverge E(x) over every active parallel interval
**	(12.6.11).  `heading` is the road's direction there.
*/
static void Paved_Edges_At(const InteractionBuilder & b,const HybridTopologyEdge & edge,const HybridRoute & route,const Vector2 & point,
									const Vector2 & heading,float * left_out,float * right_out)
{
	const HybridConstants & k = b.P->Constants;
	HybridInterchangeConstants c;
	Hybrid_Interchange_Constants(k.W,&c);
	const float half = 0.5f * Hybrid_Edge_Roadbed_Width(edge,k.W);
	*left_out	= half;
	*right_out	= half;

	if ((edge.Kind == HYBRID_EDGE_HIGHWAY_RAMP) && (route.Points.Count() >= 2)) {
		//	Stored L_s -> gore; inner is left of the ramp's traffic, which an OFF_RAMP's stored order runs against.
		float station = 0.0f;
		float best = 3.4e38f;
		for (int i = 1; i < route.Points.Count(); ++i) {
			float t_on = 0.0f;
			const float distance = Distance_To_Segment(point,route.Points[i - 1],route.Points[i],&t_on);
			if (distance >= best) continue;
			best		= distance;
			station	= route.Stations[i - 1] + ((route.Stations[i] - route.Stations[i - 1]) * t_on);
		}
		float inner = 0.0f;
		float outer = 0.0f;
		Hybrid_Ramp_Effective_Edges(c,edge.RampRoadbedExtra,route.Stations[route.Stations.Count() - 1] - station,&inner,&outer);
		const bool against = (edge.RampRole == HYBRID_RAMP_ROLE_OFF);
		*left_out	= against ? outer : inner;
		*right_out	= against ? inner : outer;
		return;
	}
	if (edge.RoadClass != HYBRID_ROAD_HIGHWAY_4) return;

	const Vector2 left_of_travel(-heading.Y,heading.X);
	for (int i = 0; i < b.Interchanges.Count(); ++i) {
		const HybridInterchange & ic = *b.Interchanges[i].Record;
		if ((ic.HighwayEdgeIn != edge.StableId) && (ic.HighwayEdgeOut != edge.StableId)) continue;
		for (int e = 0; e < ic.RampEdges.Count(); ++e) {
			const HybridTopologyEdge * ramp = b.Topology->Find_Edge(ic.RampEdges[e]);
			if (ramp == nullptr) continue;
			Vector2 tau;
			Vector2 normal;
			Side_Frame(ic.Axis,ramp->RampSide,&tau,&normal);
			const Vector2 offset = point - b.Interchanges[i].X;
			const float x = (offset.X * tau.X) + (offset.Y * tau.Y);
			const bool on = (ramp->RampRole == HYBRID_RAMP_ROLE_ON);
			const float along = on ? (x - ic.GoreOffset) : (x + ic.GoreOffset + c.DivergeLength);
			const float span = on ? c.MergeLength : c.DivergeLength;
			if ((along < 0.0f) || (along > span)) continue;
			HybridAuxiliaryLaneSection section;
			Hybrid_Auxiliary_Lane_Section(c,ramp->RampRole,along,&section);
			if (((left_of_travel.X * normal.X) + (left_of_travel.Y * normal.Y)) > 0.0f) {
				*left_out = Max2(*left_out,section.E);
			} else {
				*right_out = Max2(*right_out,section.E);
			}
		}
	}
}


/*
**	D-027's overpass deck reach on one side: from the crossing along the upper road (`direction`) to where the whole deck
**	width -- its centerline and both edges -- has left the lower road's actual paved roadbed, then the upper road's
**	outer blend and L_pin beyond.  The lower roadbed is read where the deck leaves it, so a widening there counts.
*/
static float Overpass_Reach(const InteractionBuilder & b,const HybridTopologyEdge & lower_edge,const HybridRoute & lower_route,
									 const Vector2 & point,const Vector2 & direction,const Vector2 & under,float deck_half)
{
	const HybridConstants & k = b.P->Constants;
	const Vector2 lower_left(-under.Y,under.X);
	const Vector2 upper_left(-direction.Y,direction.X);
	const float rise = (direction.X * lower_left.X) + (direction.Y * lower_left.Y);
	const float spread = (float)fabs((double)((upper_left.X * lower_left.X) + (upper_left.Y * lower_left.Y)));
	float exit = 0.0f;
	Vector2 at = point;
	for (int pass = 0; pass < 2; ++pass) {
		float left = 0.0f;
		float right = 0.0f;
		Paved_Edges_At(b,lower_edge,lower_route,at,under,&left,&right);
		if (pass > 0) {
			float left0 = 0.0f;
			float right0 = 0.0f;
			Paved_Edges_At(b,lower_edge,lower_route,point,under,&left0,&right0);
			left	= Max2(left,left0);
			right	= Max2(right,right0);
		}
		const float edge = (rise > 0.0f) ? left : right;
		exit = (edge + (deck_half * spread)) / Max2((float)fabs((double)rise),1.0e-3f);
		//	Where the deck leaves the lower road, along the lower road.
		const Vector2 out = point + (direction * exit);
		const float along = ((out.X - point.X) * under.X) + ((out.Y - point.Y) * under.Y);
		at = point + (under * along);
	}
	return exit + Hybrid_Overpass_Deck_Allowance(k.W,k.A,b.P->TerrainCellSize);
}


static bool Try_Overpass_Assignment(InteractionBuilder & b,const HybridTopologyEdge & upper_edge,
												const HybridRoute & upper_route,const HybridTopologyEdge & lower_edge,
												const HybridRoute & lower_route,const Vector2 & point,
												const Vector2 & upper_heading,const Vector2 & lower_heading,
												bool upper_is_first,bool mandatory,HybridInteraction * out,
												HybridInteractionProblem * problem_out)
{
	const HybridConstants & k = b.P->Constants;
	const HybridRoadClassSpec & upper_spec = Hybrid_Road_Class_Spec(upper_edge.RoadClass);
	const float max_grade = upper_spec.MaxGradeDegrees * PI_OVER_180;
	const float max_cross = upper_spec.MaxCrossSlopeDegrees * PI_OVER_180;
	const float half_roadbed = 0.5f * Hybrid_Edge_Roadbed_Width(upper_edge,k.W);

	//	D-027: this assignment's structural pins could not be realized on the grid; the swap or a reroute follows.
	if (b.Is_Excluded(HYBRID_METHOD_OVERPASS,upper_edge.StableId,lower_edge.StableId,0,true,Quantized(point),problem_out)) {
		return false;
	}

	float lower_z = 0.0f;
	if (!b.Height_At(point,&lower_z)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_NO_HARD_VALID_STATION;
		return false;
	}

	//	Section 12.3's clearance, plus the structural depth of the deck that carries the upper road.
	const float gap = Hybrid_Overpass_Clear_Gap(b.P->HeavyVehicleHeight,b.P->WorldUnitsPerMeter);
	float structural_depth = k.W;						// no registered deck: one lane width of structure
	/*
	**	The deck's approach is the upper road's own centerline either side of the crossing -- the ramps run
	**	along it -- so what a kind's required approach has to fit inside is the shorter of those two arms,
	**	exactly as a bridge's has to fit inside the road on either side of its interval.
	*/
	float arm_before = 0.0f;
	float arm_after = 0.0f;
	Arm_Lengths(upper_route,point,&arm_before,&arm_after);

	/*
	**	D-027: the deck reaches, each side, from the crossing to where its whole width has left the lower road's actual
	**	paved roadbed (asymmetric and widened edges included, its shoulders counted once, in that roadbed), then the upper
	**	road's outer blend and L_pin, so the upper road's own ground, blend and structural landing never stand on the lower
	**	roadbed.  The registered kind has to span that whole deck.
	*/
	const Vector2 axis = Unit(upper_heading);
	const Vector2 under = Unit(lower_heading);
	const float sine = (float)fabs((double)((axis.X * under.Y) - (axis.Y * under.X)));
	if (sine <= 1.0e-3f) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_TURN_TOO_TIGHT;
		return false;
	}
	const float deck_width = Max2(Hybrid_Edge_Roadbed_Width(upper_edge,k.W),
											Hybrid_Bridge_Clear_Width(upper_edge.RoadClass,k.W,
																			  Required_Swept_Width(b,upper_edge.Capabilities),
																			  b.P->MinVehicleLaneClearance));
	const float reach_before = Overpass_Reach(b,lower_edge,lower_route,point,-axis,under,0.5f * deck_width);
	const float reach_after = Overpass_Reach(b,lower_edge,lower_route,point,axis,under,0.5f * deck_width);
	const float deck_length = reach_before + reach_after;
	int bridge_kind = Select_Bridge_Kind(b,upper_edge.RoadClass,upper_edge.Capabilities,deck_length,
													 Min2(arm_before,arm_after),nullptr,true);
	/*
	**	The deck is a registered bridge kind, realized through BridgeSystem in Phase 12; a road x road crossing
	**	that no registered kind can carry is not an overpass this generator can build, and it says so rather
	**	than inventing a structure of its own.
	*/
	if (bridge_kind < 0) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_METHOD_UNAVAILABLE;
		return false;
	}
	structural_depth = b.P->BridgeKinds[bridge_kind].StructuralDepth;

	const float deck_bottom = lower_z + gap;
	const float deck_top = deck_bottom + structural_depth;
	const float tangent = (float)::tan((double)max_grade);

	/*
	**	Where the crossing stands on the upper road.
	*/
	const float total = upper_route.Stations[upper_route.Stations.Count() - 1];
	float centre_station = 0.0f;
	{
		float best = 3.4e38f;
		for (int i = 1; i < upper_route.Points.Count(); ++i) {
			float t_on = 0.0f;
			const float distance = Distance_To_Segment(point,upper_route.Points[i - 1],upper_route.Points[i],&t_on);
			if (distance >= best) continue;
			best = distance;
			centre_station = upper_route.Stations[i - 1] +
								  ((upper_route.Stations[i] - upper_route.Stations[i - 1]) * t_on);
		}
	}

	//	The deck is a straight bridge, so the upper road has to run straight under all of it.
	{
		int at = 0;
		float nearest = 3.4e38f;
		for (int i = 0; i < upper_route.Points.Count(); ++i) {
			const float distance = (upper_route.Points[i] - point).Length();
			if (distance >= nearest) continue;
			nearest = distance;
			at = i;
		}
		float behind = 0.0f;
		float ahead = 0.0f;
		Hybrid_Polyline_Straight_Run(upper_route.Points,at,axis,k.Q,&behind,&ahead);
		const Vector2 offset = point - upper_route.Points[at];
		const float along = (offset.X * axis.X) + (offset.Y * axis.Y);
		if ((behind < (reach_before - along - (2.0f * k.Q))) || (ahead < (reach_after + along - (2.0f * k.Q)))) {
			if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_TURN_TOO_TIGHT;
			return false;
		}
	}

	/*
	**	The ramp is measured against the rise it actually has to make: from the road's own elevation at each
	**	ramp foot up to the deck, not from the ground under the crossing.  Each side is sized on its own rise
	**	and the longer of the two is reserved, because the plan reserves `L_ramp` on each side.  The feet stand
	**	on the upper road's own centerline, `L_ramp` beyond each end of the deck.
	**
	**	`L_ramp` carries the smoothstep's peak-slope factor as well as the plan's `10W` floor, so the shape
	**	the plan asks for and the per-sample grade it requires are both satisfied.  It is found iteratively
	**	because the foot moves as the ramp lengthens; two passes settle it on this geometry, and the loop is
	**	bounded regardless.
	*/
	float ramp = Max2(RAMP_FLOOR_W * k.W,
							(RAMP_SMOOTHSTEP_PEAK * (float)fabs((double)(deck_top - lower_z))) /
							Max2(tangent,k.Q));
	float foot_z_a = lower_z;
	float foot_z_b = lower_z;

	for (int pass = 0; pass < 4; ++pass) {

		const float foot_from = centre_station - reach_before - ramp;
		const float foot_to = centre_station + reach_after + ramp;
		if ((foot_from < 0.0f) || (foot_to > total)) break;			// the arm test below names it
		const Vector2 foot_a = Hybrid_Polyline_Point_At(upper_route.Points,upper_route.Stations,foot_from);
		const Vector2 foot_b = Hybrid_Polyline_Point_At(upper_route.Points,upper_route.Stations,foot_to);
		if (!b.Height_At(foot_a,&foot_z_a) || !b.Height_At(foot_b,&foot_z_b)) {
			if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_NO_HARD_VALID_STATION;
			return false;
		}
		const float rise = Max2((float)fabs((double)(deck_top - foot_z_a)),
										(float)fabs((double)(deck_top - foot_z_b)));
		const float needed = Max2(RAMP_FLOOR_W * k.W,
										  (RAMP_SMOOTHSTEP_PEAK * rise) / Max2(tangent,k.Q));
		if (needed <= (ramp + k.Q)) break;
		ramp = needed;
	}

	/*
	**	`L_ramp` is a minimum, so the recorded length is the quantum at or above it: rounding to nearest would
	**	put a ramp sized exactly on the `10W` floor up to half a quantum under it.
	*/
	{
		const float rounded = Hybrid_Quantize_Position(ramp);
		ramp = (rounded < ramp) ? (rounded + HYBRID_QUANTUM_POSITION) : rounded;
	}

	//	The upper road has to have that much centerline either side of the crossing to ramp over.
	const float from_station = centre_station - reach_before - ramp;
	const float to_station = centre_station + reach_after + ramp;
	if ((from_station < 0.0f) || (to_station > total)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_APPROACH_DOES_NOT_FIT;
		return false;
	}

	const Vector2 approach_a = Quantized(Hybrid_Polyline_Point_At(upper_route.Points,upper_route.Stations,
																					  from_station));
	const Vector2 approach_b = Quantized(Hybrid_Polyline_Point_At(upper_route.Points,upper_route.Stations,
																					  to_station));

	/*
	**	12.6.10 items 1-2: the overpass ramp interval -- the upper road from foot to foot, a swapped assignment
	**	with the highway on top included -- stays out of every active gore reserve; a conflict is this assignment's
	**	failure under the existing swap rule.
	*/
	if (Polyline_Hits_Gore_Reserve(b,upper_route.Points,upper_route.Stations,from_station,to_station,half_roadbed)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_GORE_RESERVE;
		return false;
	}

	const float spacing = Max2(Min2(k.W / OVERPASS_SAMPLE_W,k.A / OVERPASS_SAMPLE_A),k.Q);
	const float length = to_station - from_station;
	const int steps = (int)::ceil((double)(length / spacing));
	const int count = (steps > 0) ? steps : 1;

	float previous_z = 0.0f;
	for (int i = 0; i <= count; ++i) {

		const float along = from_station + (length * ((float)i / (float)count));
		const Vector2 sample = Hybrid_Polyline_Point_At(upper_route.Points,upper_route.Stations,along);

		float ground = 0.0f;
		if (!b.Height_At(sample,&ground)) {
			if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_NO_HARD_VALID_STATION;
			return false;
		}

		/*
		**	One smoothstep ramp over exactly `L_ramp` on each side, and a level deck through the crossing.
		**	The ramp runs from the road's own elevation at its foot up to the deck -- a compiled profile, not
		**	the ground offset by one.  Following the terrain instead would put the terrain's own slope into the
		**	road's grade, which is what the vertical-profile solver exists to avoid.
		*/
		const float from_deck = (along < centre_station) ? Max2((centre_station - along) - reach_before,0.0f)
																		 : Max2((along - centre_station) - reach_after,0.0f);
		const float s = Clamp2(1.0f - (from_deck / Max2(ramp,k.Q)),0.0f,1.0f);
		const float smooth = s * s * (3.0f - (2.0f * s));
		const float foot_z = (along < centre_station) ? foot_z_a : foot_z_b;
		const float profile = foot_z + ((deck_top - foot_z) * smooth);

		//	Cut and fill: the ramp's own feet have to land inside the vertical interval of the ground.
		const HybridVerticalInterval interval_here = Hybrid_Road_Vertical_Interval(ground,b.P->MaxCutDepth,
																								  b.P->MaxFillHeight);
		if ((smooth <= 0.0f) && ((profile < interval_here.Low) || (profile > interval_here.High))) {
			if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_GRADE_TOO_STEEP;
			return false;
		}

		//	Max grade along the ramp, on the compiled profile rather than on the ground under it.
		if (i > 0) {
			const float run = length / (float)count;
			const float rise = (float)fabs((double)(profile - previous_z));
			if ((run > k.Q) && ((float)::atan((double)(rise / run)) > (max_grade + 0.001f)) ) {
				if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_GRADE_TOO_STEEP;
				return false;
			}
		}
		previous_z = profile;

		//	Cross slope across the road where it is, measured across its own local direction.
		const Vector2 ahead = Hybrid_Polyline_Point_At(upper_route.Points,upper_route.Stations,
																	  Min2(along + spacing,total));
		const Vector2 behind = Hybrid_Polyline_Point_At(upper_route.Points,upper_route.Stations,
																		Max2(along - spacing,0.0f));
		const Vector2 direction = Unit(ahead - behind);
		const Vector2 normal(-direction.Y,direction.X);
		float left = 0.0f;
		float right = 0.0f;
		if (b.Height_At(sample + (normal * half_roadbed),&left) &&
			 b.Height_At(sample - (normal * half_roadbed),&right)) {

			const HybridVerticalInterval left_interval = Hybrid_Road_Vertical_Interval(left,b.P->MaxCutDepth,
																									  b.P->MaxFillHeight);
			const HybridVerticalInterval right_interval = Hybrid_Road_Vertical_Interval(right,
																										b.P->MaxCutDepth,
																										b.P->MaxFillHeight);
			if ((smooth <= 0.0f) &&
				 !Hybrid_Grade_Compatible(left_interval,right_interval,2.0f * half_roadbed,max_cross)) {
				if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_CROSS_SLOPE;
				return false;
			}
		}
	}

	/*
	**	The clearance itself, at the crossing.  `deck_bottom` was built as `lower_z + gap`, so the difference
	**	is that gap up to the rounding of one addition -- the comparison carries the quantum rather than
	**	asking two floats to be exactly equal.
	*/
	if ((deck_bottom - lower_z) < (gap - k.Q)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_CLEARANCE;
		return false;
	}

	/*
	**	The upper road's own centerline over both ramps and the deck, frozen as the path Phase 12 realizes, and
	**	graded by Section 12.1 itself with the deck held level at its top: the smoothstep above is the plan's
	**	construction rule, and the solver is what Phase 12 will actually build the road with.
	*/
	HybridCrossingPath path;
	path.DepartPoint = 0;
	path.RejoinPoint = upper_route.Points.Count() - 1;
	for (int i = 0; i < upper_route.Points.Count(); ++i) {
		if (upper_route.Stations[i] <= from_station) path.DepartPoint = i;
		if (upper_route.Stations[i] >= to_station) {
			path.RejoinPoint = i;
			break;
		}
	}
	path.Points.Add(approach_a);
	path.Stations.Add(0.0f);
	for (int i = path.DepartPoint + 1; i < path.RejoinPoint; ++i) {
		if ((upper_route.Stations[i] <= (from_station + k.Q)) || (upper_route.Stations[i] >= (to_station - k.Q))) {
			continue;
		}
		path.Points.Add(upper_route.Points[i]);
		path.Stations.Add(upper_route.Stations[i] - from_station);
	}
	path.Points.Add(approach_b);
	path.Stations.Add(length);
	path.Length = Hybrid_Quantize_Position(length);
	path.AddedLength = 0.0f;

	/*
	**	Section 12.1 grades the upper road whole, as Phase 12 will build it: its realized centerline with every
	**	structure already compiled on it holding its own surface and this deck held level at its top.  Pass two
	**	runs on realized roads, so the bridges and tunnels of pass one are on `upper_route` already; the ramps'
	**	smoothstep above is the plan's construction rule, and this is what decides whether the road can carry it.
	**	Grading the ramps alone left their feet free, so a deck could be accepted that the road beyond its feet
	**	cannot climb to.
	*/
	{
		//	The upper road as one physical road (Q-025): a heavy family's whole trunk, or the edge itself.
		HybridRoute road;
		DynamicVectorClass<unsigned int> road_edges;
		if (!Hybrid_Realized_Physical_Road(*b.Routes,*b.Out,upper_edge.StableId,&road,&road_edges)) {
			if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_TOO_CLOSE_TO_CROSSING;
			return false;
		}
		DynamicVectorClass<HybridProfileStructure> structures;
		for (int i = 0; i < b.Out->Interactions.Count(); ++i) {
			const HybridInteraction & other = b.Out->Interactions[i];
			bool on_road = false;
			for (int e = 0; (e < road_edges.Count()) && !on_road; ++e) on_road = (other.EdgeId == road_edges[e]);
			if (!on_road) continue;
			HybridProfileStructure one;
			if (Compiled_Structure(other,road,&one)) structures.Add(one);
		}
		const float road_centre = Realized_Station_Of(road,0,road.Points.Count() - 1,point);
		HybridProfileStructure deck;
		deck.From		= road_centre - reach_before;
		deck.To			= road_centre + reach_after;
		deck.Mode		= HYBRID_PROFILE_STRUCTURE_LEVEL;
		deck.EndsFixed	= true;
		deck.StartZ		= Hybrid_Quantize_Position(deck_top);
		deck.EndZ		= deck.StartZ;
		structures.Add(deck);

		DynamicVectorClass<HybridProfileSample> samples;
		HybridSolvedProfile solved;
		HybridGradeBudget budget;
		if (!Hybrid_Solve_Road_Stretch(road.Points,road.Stations,0.0f,road.Stations[road.Stations.Count() - 1],structures,
												 Hybrid_Profile_Sample_Spacing(k),b.Sample,b.Context,b.P->MaxCutDepth,
												 b.P->MaxFillHeight,max_grade,k.Q,&samples,&solved,nullptr,nullptr,nullptr,
												 Grade_Budget(b,upper_edge.RoadClass,road_edges,structures,&budget))) {
			if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_GRADE_TOO_STEEP;
			return false;
		}
	}

	*out = HybridInteraction();
	out->EdgeId					= upper_edge.StableId;
	out->OtherEdgeId			= lower_edge.StableId;
	out->Obstruction			= HYBRID_OBSTRUCTION_NONE;
	out->Mandatory				= mandatory;
	out->Method					= HYBRID_METHOD_OVERPASS;
	out->RoadClass				= upper_edge.RoadClass;
	out->Station				= Quantized(point);
	out->Tangent				= axis;
	out->ApproachA				= approach_a;
	out->ApproachB				= approach_b;
	out->Span					= Hybrid_Quantize_Position(deck_length);
	out->ApproachReserve		= ramp;
	out->BridgeKind			= bridge_kind;
	out->DeckBottomZ			= Hybrid_Quantize_Position(deck_bottom);
	out->DeckTopZ				= Hybrid_Quantize_Position(deck_top);
	out->DeckStartZ			= out->DeckTopZ;
	out->DeckEndZ				= out->DeckTopZ;
	out->DeckReachA			= Hybrid_Quantize_Position(reach_before);
	out->DeckReachB			= Hybrid_Quantize_Position(reach_after);
	out->LowerSurfaceZ		= Hybrid_Quantize_Position(lower_z);
	out->RampLength			= ramp;
	out->Path					= path;
	out->UpperIsThisEdge		= upper_is_first;
	out->Envelope				= upper_route.Envelope;
	(void)lower_route;
	return true;
}


/*
**	One road x road crossing.  An at-grade junction when every condition holds; otherwise an overpass, with
**	the plan's exact assignment order and its single permitted swap; otherwise a reroute request for the
**	lower-priority road.  A reserved Phase-9 `OVERPASS_OBLIGATION` always compiles as an overpass and never
**	downgrades to a junction.
*/
static bool Solve_Road_Crossing(InteractionBuilder & b,const HybridTopologyEdge & edge_a,
											const HybridRoute & route_a,const HybridTopologyEdge & edge_b,
											const HybridRoute & route_b,const Vector2 & point,
											const Vector2 & heading_a,const Vector2 & heading_b,bool mandatory,
											HybridInteractionResult * result)
{
	HybridInteractionProblem problem = HYBRID_PROBLEM_NONE;
	HybridInteractionProblem junction_problem = HYBRID_PROBLEM_NONE;
	HybridInteractionProblem overpass_problem[2] = { HYBRID_PROBLEM_NONE,HYBRID_PROBLEM_NONE };
	HybridInteraction solution;

	//	An at-grade junction, unless Phase 9 reserved this crossing as an overpass.
	if (!mandatory && At_Grade_Is_Legal(b,edge_a,route_a,edge_b,route_b,point,heading_a,heading_b,
													&junction_problem)) {

		solution = HybridInteraction();
		solution.StableId			= b.NextStableId++;
		solution.EdgeId			= edge_a.StableId;
		solution.OtherEdgeId		= edge_b.StableId;
		solution.Method			= HYBRID_METHOD_AT_GRADE_JUNCTION;
		solution.RoadClass		= edge_a.RoadClass;
		solution.Station			= Quantized(point);
		solution.Tangent			= heading_a;
		solution.ApproachA		= Quantized(point);
		solution.ApproachB		= Quantized(point);
		solution.Envelope			= route_a.Envelope;
		b.Out->Interactions.Add(solution);
		return true;
	}

	/*
	**	The assignment order: the higher-priority road stays lower.  A tie on class keeps the lower
	**	`StableId` underneath.  Exactly one swap is permitted after that.
	*/
	const int priority_a = Hybrid_Route_Priority_Rank(Hybrid_Route_Priority_Of(*b.Topology,edge_a));
	const int priority_b = Hybrid_Route_Priority_Rank(Hybrid_Route_Priority_Of(*b.Topology,edge_b));

	bool a_stays_lower = (priority_a < priority_b);
	if (priority_a == priority_b) a_stays_lower = (edge_a.StableId < edge_b.StableId);

	for (int attempt = 0; attempt < 2; ++attempt) {

		const bool a_is_lower = (attempt == 0) ? a_stays_lower : !a_stays_lower;
		const HybridTopologyEdge & lower = a_is_lower ? edge_a : edge_b;
		const HybridTopologyEdge & upper = a_is_lower ? edge_b : edge_a;
		const HybridRoute & lower_route = a_is_lower ? route_a : route_b;
		const HybridRoute & upper_route = a_is_lower ? route_b : route_a;
		const Vector2 & upper_heading = a_is_lower ? heading_b : heading_a;
		const Vector2 & lower_heading = a_is_lower ? heading_a : heading_b;

		if (Try_Overpass_Assignment(b,upper,upper_route,lower,lower_route,point,upper_heading,lower_heading,
											 !a_is_lower,mandatory,&solution,&problem)) {
			solution.StableId = b.NextStableId++;
			b.Out->Interactions.Add(solution);
			return true;
		}
		overpass_problem[attempt] = problem;
	}

	//	Neither assignment compiled: the lower-priority road is the one asked to reroute.
	const bool reroute_a = (priority_a > priority_b) ||
								  ((priority_a == priority_b) && (edge_a.StableId > edge_b.StableId));
	const HybridTopologyEdge & rerouted = reroute_a ? edge_a : edge_b;
	const HybridRoute & rerouted_route = reroute_a ? route_a : route_b;

	if (result != nullptr) {
		HybridRoadCrossingCensus & census = result->RoadCrossings;
		census.FailedMeeting		= mandatory ? HYBRID_ROAD_MEETING_OBLIGATION : HYBRID_ROAD_MEETING_INCIDENTAL;
		census.FailedKindA		= (int)edge_a.Kind;
		census.FailedKindB		= (int)edge_b.Kind;
		const float cosine		= (float)fabs((double)((heading_a.X * heading_b.X) + (heading_a.Y * heading_b.Y)));
		census.FailedAngle		= (float)acos((double)Min2(cosine,1.0f)) / PI_OVER_180;
		const HybridTopologyNode * a0 = b.Topology->Find_Node(edge_a.NodeA);
		const HybridTopologyNode * a1 = b.Topology->Find_Node(edge_a.NodeB);
		const HybridTopologyNode * b0 = b.Topology->Find_Node(edge_b.NodeA);
		const HybridTopologyNode * b1 = b.Topology->Find_Node(edge_b.NodeB);
		census.FailedEndA			= ((a0 != nullptr) && (a1 != nullptr))
											? Min2((a0->Position - point).Length(),(a1->Position - point).Length()) : -1.0f;
		census.FailedEndB			= ((b0 != nullptr) && (b1 != nullptr))
											? Min2((b0->Position - point).Length(),(b1->Position - point).Length()) : -1.0f;
		if ((a0 != nullptr) && (a1 != nullptr)) {
			census.FailedEndKindA = (int)(((a0->Position - point).Length() <= (a1->Position - point).Length()) ? a0->Kind : a1->Kind);
		}
		if ((b0 != nullptr) && (b1 != nullptr)) {
			census.FailedEndKindB = (int)(((b0->Position - point).Length() <= (b1->Position - point).Length()) ? b0->Kind : b1->Kind);
		}
		census.FailedWaypointShared = false;
		for (int w = 0; w < edge_a.Waypoints.Count(); ++w) {
			if ((edge_a.Waypoints[w] == edge_b.NodeA) || (edge_a.Waypoints[w] == edge_b.NodeB)) census.FailedWaypointShared = true;
		}
		for (int w = 0; w < edge_b.Waypoints.Count(); ++w) {
			if ((edge_b.Waypoints[w] == edge_a.NodeA) || (edge_b.Waypoints[w] == edge_a.NodeB)) census.FailedWaypointShared = true;
		}
		census.JunctionProblem	= (int)junction_problem;
		census.OverpassProblem[0]= (int)overpass_problem[0];
		census.OverpassProblem[1]= (int)overpass_problem[1];
	}

	if (!Note_Problem(b,edge_a.StableId,edge_b.StableId,HYBRID_OBSTRUCTION_NONE,0,mandatory,problem,0,
							point)) {
		StringClass detail;
		detail.Format("edge %u has more than %d unsolved crossings",edge_a.StableId,
						  b.P->MaxProblemsPerEdge);
		return Fail(result,HYBRID_INTERACTION_LIMIT,detail.Peek_Buffer());
	}

	StringClass detail;
	detail.Format("edges %u (%s) and %u (%s) cross at (%.1f,%.1f) and neither an at-grade junction (%s) nor an "
					  "overpass (%s) compiles",edge_a.StableId,Hybrid_Road_Class_Name(edge_a.RoadClass),
					  edge_b.StableId,Hybrid_Road_Class_Name(edge_b.RoadClass),point.X,point.Y,
					  mandatory ? "reserved overpass" : Hybrid_Interaction_Problem_Name(junction_problem),
					  Hybrid_Interaction_Problem_Name(problem));

	//	12.6.10: a constructed ramp or crossing has no Phase-10 envelope, so a rule that would reroute it is exhausted.
	if (Is_Interchange_Element(rerouted)) return Fail(result,HYBRID_INTERACTION_INTERCHANGE,detail.Peek_Buffer());

	if (rerouted_route.Envelope < 2) {
		b.Out->RerouteRequests++;
		if (result != nullptr) result->RerouteEdgeId = rerouted.StableId;
	}

	if (mandatory) return Fail(result,HYBRID_INTERACTION_OBLIGATION_LOST,detail.Peek_Buffer());
	if (edge_a.Required && edge_b.Required) {
		return Fail(result,HYBRID_INTERACTION_NO_SOLUTION,detail.Peek_Buffer());
	}
	WWDEBUG_SAY(("Hybrid Phase 11: optional crossing of edges %u and %u is dropped\r\n",edge_a.StableId,
					 edge_b.StableId));
	return true;
}

/***********************************************************************************************
**	Topology junctions: where Phase 9 joined two roads at a node they share.
***********************************************************************************************/

//	Defined with the phase below: the route's local heading nearest a point.
static Vector2 Route_Heading_At(const HybridRoute & route,const Vector2 & point);


/*
**	Does `edge` carry `node` -- as an endpoint, or as one of its waypoints?  Its ends are the ones Phase 10 realized
**	(12.6.9): a retargeted arrival ends at its local junction while its interchange is active and at X once it is
**	withdrawn.
*/
static bool Edge_Carries_Node(const InteractionBuilder & b,const HybridTopologyEdge & edge,unsigned int node)
{
	if ((Hybrid_Route_Edge_End(*b.Topology,*b.Routes,edge,false) == node) ||
		 (Hybrid_Route_Edge_End(*b.Topology,*b.Routes,edge,true) == node)) return true;
	for (int w = 0; w < edge.Waypoints.Count(); ++w) {
		if (edge.Waypoints[w] == node) return true;
	}
	return false;
}


/*
**	The node two edges meet at by design, if one is near `point`: a node both carry that is not a structure of
**	its own (an overpass obligation is a reserved crossing, compiled above; a crossing obligation is a bridge
**	no other road may end on).  "Near" is the junction's own reach there -- the longer of the two roads'
**	`max(4W, minimum turn radius)` arms -- which is the road the junction surface owns.
*/
static unsigned int Designed_Meeting_Node(const InteractionBuilder & b,const HybridTopologyEdge & edge_a,
														const HybridTopologyEdge & edge_b,const Vector2 & point)
{
	const HybridConstants & k = b.P->Constants;
	const float reach = Max2(Hybrid_Junction_Arm_Reach(edge_a.RoadClass,k.W),
									 Hybrid_Road_Class_Spec(edge_b.RoadClass).MinTurnRadiusW * k.W);
	const unsigned int candidates[2] = { Hybrid_Route_Edge_End(*b.Topology,*b.Routes,edge_a,false),
													 Hybrid_Route_Edge_End(*b.Topology,*b.Routes,edge_a,true) };
	for (int pass = 0; pass < 2; ++pass) {
		const int count = (pass == 0) ? 2 : edge_a.Waypoints.Count();
		for (int i = 0; i < count; ++i) {
			const unsigned int node_id = (pass == 0) ? candidates[i] : edge_a.Waypoints[i];
			if (!Edge_Carries_Node(b,edge_b,node_id)) continue;
			const HybridTopologyNode * node = b.Topology->Find_Node(node_id);
			if (node == nullptr) continue;
			if ((node->Kind == HYBRID_NODE_OVERPASS_OBLIGATION) || (node->Kind == HYBRID_NODE_CROSSING_OBLIGATION)) continue;
			if ((node->Position - point).Length() <= reach) return node_id;
		}
	}
	return 0;
}


//	One road's arms at a node: the realized road leaving it backward and forward, where it does.
struct JunctionArm
{
	unsigned int	EdgeId;
	Vector2			Heading;			// unit, leaving the node
	float				Length;			// centerline the arm has before the road ends
	float				Need;				// max(4W, the road's own minimum turn radius)
	float				MaxGrade;
	float				MaxCross;
	float				HalfRoadbed;

	//	DynamicVectorClass<T> requires == and !=.
	bool	operator == (const JunctionArm & that) const	{ return (EdgeId == that.EdgeId) && (Heading == that.Heading); }
	bool	operator != (const JunctionArm & that) const	{ return !(*this == that); }
};


static void Junction_Arms(const InteractionBuilder & b,const HybridTopologyEdge & edge,const HybridRoute & route,
								  const Vector2 & node,DynamicVectorClass<JunctionArm> * arms)
{
	if (route.Points.Count() < 2) return;
	const HybridConstants & k = b.P->Constants;
	const HybridRoadClassSpec & spec = Hybrid_Road_Class_Spec(edge.RoadClass);

	int at = 0;
	float nearest = 3.4e38f;
	for (int i = 0; i < route.Points.Count(); ++i) {
		const float distance = (route.Points[i] - node).Length();
		if (distance >= nearest) continue;
		nearest	= distance;
		at			= i;
	}
	const float total = route.Stations[route.Stations.Count() - 1];

	JunctionArm arm;
	arm.EdgeId		= edge.StableId;
	arm.Need			= Hybrid_Junction_Arm_Reach(edge.RoadClass,k.W);
	arm.MaxGrade	= spec.MaxGradeDegrees * PI_OVER_180;
	arm.MaxCross	= spec.MaxCrossSlopeDegrees * PI_OVER_180;
	arm.HalfRoadbed= 0.5f * Hybrid_Edge_Roadbed_Width(edge,k.W);
	if (at > 0) {
		arm.Heading	= Unit(Hybrid_Polyline_Point_At(route.Points,route.Stations,Max2(route.Stations[at] - k.W,0.0f)) - node);
		arm.Length	= route.Stations[at];
		arms->Add(arm);
	}
	if (at < (route.Points.Count() - 1)) {
		arm.Heading	= Unit(Hybrid_Polyline_Point_At(route.Points,route.Stations,Min2(route.Stations[at] + k.W,total)) - node);
		arm.Length	= total - route.Stations[at];
		arms->Add(arm);
	}
}


/*
**	The junction the topology made at a node: the roads do not cross there, they meet -- one ending at the
**	other, or several starting from one base entrance, whose entrance corridor every road through it shares by
**	construction.  So the at-grade test for two *crossing* roads -- a 30..150 degree angle between them and
**	`max(4W, R)` of road on both sides -- is not this junction's test: a fork leaves its node on one heading by
**	design, and a road that ends at the node has one side.  What the shared junction surface itself must satisfy
**	is kept, on every arm the roads have there:
**
**	1. the node is outside every compiled bridge or tunnel approach reserve;
**	2. the surface is gradeable to each arm inside cut and fill and that road's grade, over the arm's own reach
**	   (`max(4W, R)`, or the road's whole length where it is shorter);
**	3. the surface satisfies the larger of the roads' cross-slope limits.
*/
static bool Topology_Junction_Is_Legal(InteractionBuilder & b,const Vector2 & node,
													const DynamicVectorClass<JunctionArm> & arms,
													HybridInteractionProblem * problem_out)
{
	const HybridConstants & k = b.P->Constants;

	if (Inside_Approach_Reserve(b,node)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_RESERVE_CONFLICT;
		return false;
	}

	float junction_z = 0.0f;
	if (!b.Height_At(node,&junction_z)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_NO_HARD_VALID_STATION;
		return false;
	}
	const HybridVerticalInterval junction_interval = Hybrid_Road_Vertical_Interval(junction_z,b.P->MaxCutDepth,
																									b.P->MaxFillHeight);
	float max_cross = 0.0f;
	float half = 0.0f;
	for (int i = 0; i < arms.Count(); ++i) {
		const float reach = Min2(arms[i].Need,arms[i].Length);
		if (reach <= k.Q) continue;
		const Vector2 end = node + (arms[i].Heading * reach);
		float end_z = 0.0f;
		max_cross	= Max2(max_cross,arms[i].MaxCross);
		half			= Max2(half,arms[i].HalfRoadbed);
		if (!b.Height_At(end,&end_z)) continue;
		const HybridVerticalInterval end_interval = Hybrid_Road_Vertical_Interval(end_z,b.P->MaxCutDepth,
																								  b.P->MaxFillHeight);
		if (!Hybrid_Grade_Compatible(junction_interval,end_interval,reach,arms[i].MaxGrade)) {
			if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_GRADE_TOO_STEEP;
			return false;
		}
	}

	//	The surface is as wide as the widest road through it, judged across each arm's own direction.
	for (int i = 0; i < arms.Count(); ++i) {
		const Vector2 normal(-arms[i].Heading.Y,arms[i].Heading.X);
		float left = 0.0f;
		float right = 0.0f;
		if (!b.Height_At(node + (normal * half),&left) || !b.Height_At(node - (normal * half),&right)) continue;
		const HybridVerticalInterval left_interval = Hybrid_Road_Vertical_Interval(left,b.P->MaxCutDepth,
																								  b.P->MaxFillHeight);
		const HybridVerticalInterval right_interval = Hybrid_Road_Vertical_Interval(right,b.P->MaxCutDepth,
																									b.P->MaxFillHeight);
		if (!Hybrid_Grade_Compatible(left_interval,right_interval,2.0f * half,max_cross)) {
			if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_CROSS_SLOPE;
			return false;
		}
	}
	return true;
}


/*
**	Every node the topology joins two or more realized roads at, compiled as the junction the topology meant.
**	The one meeting that is not a junction is a family trunk passing a waypoint of its own -- two consecutive
**	segments of one route, which is one road.  A junction the rules refuse asks the lowest-priority road
**	there for its next envelope, and otherwise fails the phase: there is no overpass for two roads that end
**	at the same point.
*/
static bool Compile_Topology_Junctions(InteractionBuilder & b,const DynamicVectorClass<HybridRoute> & realized,
													HybridInteractionResult * result)
{
	const HybridTopology & topology = *b.Topology;

	for (int n = 0; n < topology.Nodes.Count(); ++n) {

		const HybridTopologyNode & node = topology.Nodes[n];
		if ((node.Kind == HYBRID_NODE_OVERPASS_OBLIGATION) || (node.Kind == HYBRID_NODE_CROSSING_OBLIGATION) ||
			 (node.Kind == HYBRID_NODE_BASE)) continue;

		DynamicVectorClass<int> incident;
		for (int r = 0; r < realized.Count(); ++r) {
			const HybridTopologyEdge * edge = topology.Find_Edge(realized[r].EdgeId);
			if ((edge == nullptr) || !Edge_Carries_Node(b,*edge,node.StableId)) continue;
			//	A ramp's NodeB names its anchor X; the ramp itself ends at its gore and never reaches X (12.6.10).
			if ((edge->Kind == HYBRID_EDGE_HIGHWAY_RAMP) && (edge->NodeB == node.StableId)) continue;
			incident.Add(r);
		}
		if (incident.Count() < 2) continue;

		if (incident.Count() == 2) {
			const HybridTopologyEdge * first = topology.Find_Edge(realized[incident[0]].EdgeId);
			const HybridTopologyEdge * second = topology.Find_Edge(realized[incident[1]].EdgeId);
			if ((first->Kind == HYBRID_EDGE_FAMILY_TRUNK) && (second->Kind == HYBRID_EDGE_FAMILY_TRUNK) &&
				 (first->RouteFamily == second->RouteFamily)) continue;
		}
		if (result != nullptr) result->RoadCrossings.Meetings[HYBRID_ROAD_MEETING_SHARED]++;

		/*
		**	The through road is the highest-priority one; the lowest-priority one is the one rerouted -- never a
		**	constructed interchange element (12.6.10), which has no envelope to move to.
		*/
		int through = incident[0];
		int lowest = -1;
		DynamicVectorClass<JunctionArm> arms;
		for (int i = 0; i < incident.Count(); ++i) {
			const HybridRoute & route = realized[incident[i]];
			const HybridTopologyEdge * edge = topology.Find_Edge(route.EdgeId);
			Junction_Arms(b,*edge,route,node.Position,&arms);
			const HybridTopologyEdge * held_through = topology.Find_Edge(realized[through].EdgeId);
			const int priority = Hybrid_Route_Priority_Rank(Hybrid_Route_Priority_Of(topology,*edge));
			const int through_priority = Hybrid_Route_Priority_Rank(Hybrid_Route_Priority_Of(topology,*held_through));
			if ((priority < through_priority) || ((priority == through_priority) && (edge->StableId < held_through->StableId))) {
				through = incident[i];
			}
			if (Is_Interchange_Element(*edge)) continue;
			if (lowest < 0) {
				lowest = incident[i];
				continue;
			}
			const HybridTopologyEdge * held_lowest = topology.Find_Edge(realized[lowest].EdgeId);
			const int lowest_priority = Hybrid_Route_Priority_Rank(Hybrid_Route_Priority_Of(topology,*held_lowest));
			if ((priority > lowest_priority) || ((priority == lowest_priority) && (edge->StableId > held_lowest->StableId))) {
				lowest = incident[i];
			}
		}
		const bool local_junction = (node.Kind == HYBRID_NODE_INTERCHANGE_LOCAL_JUNCTION);

		HybridInteractionProblem problem = HYBRID_PROBLEM_NONE;
		const bool rejected = b.Is_Excluded(HYBRID_METHOD_AT_GRADE_JUNCTION,realized[through].EdgeId,
														realized[(through == incident[0]) ? incident[1] : incident[0]].EdgeId,0,false,
														Quantized(node.Position),&problem);
		if (!rejected && Topology_Junction_Is_Legal(b,node.Position,arms,&problem)) {
			const HybridRoute & main = realized[through];
			const HybridTopologyEdge * main_edge = topology.Find_Edge(main.EdgeId);
			HybridInteraction solution;
			solution.StableId		= b.NextStableId++;
			solution.EdgeId		= main.EdgeId;
			solution.OtherEdgeId	= realized[(through == incident[0]) ? incident[1] : incident[0]].EdgeId;
			solution.NodeId		= node.StableId;
			solution.Method		= HYBRID_METHOD_AT_GRADE_JUNCTION;
			solution.RoadClass	= main_edge->RoadClass;
			solution.Station		= Quantized(node.Position);
			solution.Tangent		= Route_Heading_At(main,node.Position);
			solution.ApproachA	= Quantized(node.Position);
			solution.ApproachB	= Quantized(node.Position);
			solution.Envelope		= main.Envelope;
			if (local_junction) solution.Role = HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_LOCAL_JUNCTION;
			b.Out->Interactions.Add(solution);
			continue;
		}

		const HybridRoute & rerouted = realized[(lowest >= 0) ? lowest : through];
		const HybridTopologyEdge * rerouted_edge = topology.Find_Edge(rerouted.EdgeId);
		const HybridTopologyEdge * main_edge = topology.Find_Edge(realized[through].EdgeId);
		if (!Note_Problem(b,main_edge->StableId,rerouted_edge->StableId,HYBRID_OBSTRUCTION_NONE,0,true,problem,0,
								node.Position)) {
			StringClass detail;
			detail.Format("edge %u has more than %d unsolved crossings",main_edge->StableId,b.P->MaxProblemsPerEdge);
			return Fail(result,HYBRID_INTERACTION_LIMIT,detail.Peek_Buffer());
		}
		if ((lowest >= 0) && (rerouted.Envelope < 2)) {
			b.Out->RerouteRequests++;
			if (result != nullptr) result->RerouteEdgeId = rerouted.EdgeId;
		}
		if (result != nullptr) {
			result->RoadCrossings.FailedMeeting		= HYBRID_ROAD_MEETING_SHARED;
			result->RoadCrossings.FailedKindA		= (int)main_edge->Kind;
			result->RoadCrossings.FailedKindB		= (int)rerouted_edge->Kind;
			result->RoadCrossings.JunctionProblem	= (int)problem;
		}
		StringClass detail;
		detail.Format("the junction the topology makes at node %u (%s, %d roads) is refused: %s",node.StableId,
						  Hybrid_Topology_Node_Kind_Name(node.Kind),incident.Count(),Hybrid_Interaction_Problem_Name(problem));
		return Fail(result,HYBRID_INTERACTION_NO_SOLUTION,detail.Peek_Buffer());
	}
	return true;
}


/***********************************************************************************************
**	Section 12.6.10: the interchanges' intended junctions, elevations, merge/diverge records and swept proxy.
**	A failure here never withdraws the interchange; it follows the existing Phase-11 rules and, the ramps and
**	crossings having no Phase-10 envelope, otherwise fails the phase.
***********************************************************************************************/

/*
**	A grade-separated DIAMOND_ACCESS crossing is its interchange's reserved overpass: compiled by the existing
**	overpass rule, mandatory (it never downgrades to a junction), with the highway the lower road and its two
**	edges at X one continuous road -- the physical trunk the 12.6.9 alignment made straight through X.
*/
static bool Compile_Interchange_Crossings(InteractionBuilder & b,const DynamicVectorClass<HybridRoute> & realized,
														HybridInteractionResult * result)
{
	for (int i = 0; i < b.Interchanges.Count(); ++i) {

		const HybridInterchange & ic = *b.Interchanges[i].Record;
		if ((ic.Form != HYBRID_INTERCHANGE_DIAMOND_ACCESS) || (ic.CrossingMode != HYBRID_INTERCHANGE_CROSSING_GRADE_SEPARATED)) {
			continue;
		}
		const HybridTopologyEdge * crossing = (ic.CrossingEdges.Count() > 0) ? b.Topology->Find_Edge(ic.CrossingEdges[0]) : nullptr;
		const HybridTopologyEdge * highway = b.Topology->Find_Edge(ic.HighwayEdgeIn);
		const HybridRoute * crossing_route = nullptr;
		for (int r = 0; (r < realized.Count()) && (crossing != nullptr); ++r) {
			if (realized[r].EdgeId == crossing->StableId) crossing_route = &realized[r];
		}
		HybridRoute highway_road;
		if ((crossing == nullptr) || (highway == nullptr) || (crossing_route == nullptr) ||
			 !Hybrid_Realized_Physical_Road(*b.Routes,*b.Out,ic.HighwayEdgeIn,&highway_road,nullptr)) {
			StringClass detail;
			detail.Format("interchange %u has no realized crossing or highway",ic.StableId);
			return Fail(result,HYBRID_INTERACTION_INTERCHANGE,detail.Peek_Buffer());
		}
		if (result != nullptr) result->RoadCrossings.Meetings[HYBRID_ROAD_MEETING_OBLIGATION]++;
		const Vector2 & x_point = b.Interchanges[i].X;
		if (!Solve_Road_Crossing(b,*crossing,*crossing_route,*highway,highway_road,x_point,
										 Route_Heading_At(*crossing_route,x_point),ic.Axis,true,result)) return false;
	}
	return true;
}


//	Section 12.1 over one whole physical road, every structure compiled on it holding its own surface.
static bool Solve_Physical_Road(InteractionBuilder & b,unsigned int edge_id,HybridRoute * road,
										  DynamicVectorClass<HybridProfileSample> * samples,HybridSolvedProfile * solved)
{
	const HybridConstants & k = b.P->Constants;
	const HybridTopologyEdge * edge = b.Topology->Find_Edge(edge_id);
	DynamicVectorClass<unsigned int> edges;
	DynamicVectorClass<HybridProfileStructure> structures;
	if ((edge == nullptr) || !Hybrid_Realized_Physical_Road(*b.Routes,*b.Out,edge_id,road,&edges) ||
		 !Hybrid_Road_Profile_Structures(*b.Out,*road,&structures,&edges)) return false;
	const float max_grade = Hybrid_Road_Class_Spec(edge->RoadClass).MaxGradeDegrees * PI_OVER_180;
	HybridGradeBudget budget;
	return Hybrid_Solve_Road_Stretch(road->Points,road->Stations,0.0f,road->Stations[road->Stations.Count() - 1],structures,
												Hybrid_Profile_Sample_Spacing(k),b.Sample,b.Context,b.P->MaxCutDepth,
												b.P->MaxFillHeight,max_grade,k.Q,samples,solved,nullptr,nullptr,nullptr,
												Grade_Budget(b,edge->RoadClass,edges,structures,&budget));
}


//	A solved profile's elevation at `station`, between the two samples about it.
static float Profile_Z_At(const DynamicVectorClass<HybridProfileSample> & samples,const HybridSolvedProfile & solved,
								  float station)
{
	const int count = (samples.Count() < solved.Z.Count()) ? samples.Count() : solved.Z.Count();
	if (count <= 0) return 0.0f;
	if (station <= samples[0].Station) return solved.Z[0];
	for (int i = 1; i < count; ++i) {
		if (station > samples[i].Station) continue;
		const float span = samples[i].Station - samples[i - 1].Station;
		const float t_on = (span > 0.0f) ? ((station - samples[i - 1].Station) / span) : 0.0f;
		return solved.Z[i - 1] + ((solved.Z[i] - solved.Z[i - 1]) * t_on);
	}
	return solved.Z[count - 1];
}


/*
**	12.6.10 item 4: one ramp graded whole by Section 12.1 from L_s to its gore, the gore pinned to the highway's
**	profile there and, once it is known, the L_s end pinned to z_L.
*/
static bool Solve_Ramp_Profile(InteractionBuilder & b,const HybridRoute & ramp,bool pin_local,float z_local,float z_gore,
										 HybridSolvedProfile * solved)
{
	const HybridConstants & k = b.P->Constants;
	if (ramp.Points.Count() < 2) return false;
	DynamicVectorClass<HybridProfileSample> samples;
	const float total = ramp.Stations[ramp.Stations.Count() - 1];
	if (!Hybrid_Sample_Polyline_Range(ramp.Points,ramp.Stations,0.0f,total,Hybrid_Profile_Sample_Spacing(k),b.Sample,
												 b.Context,&samples) || (samples.Count() < 2)) return false;
	if (pin_local) {
		samples[0].Fixed	= true;
		samples[0].FixedZ	= Hybrid_Quantize_Position(z_local);
	}
	samples[samples.Count() - 1].Fixed	= true;
	samples[samples.Count() - 1].FixedZ	= Hybrid_Quantize_Position(z_gore);
	HybridProfileFailure failure = (HybridProfileFailure)0;
	const float max_grade = Hybrid_Road_Class_Spec(HYBRID_ROAD_HIGHWAY_RAMP).MaxGradeDegrees * PI_OVER_180;
	DynamicVectorClass<unsigned int> edges;
	edges.Add(ramp.EdgeId);
	const DynamicVectorClass<HybridProfileStructure> none;
	HybridGradeBudget budget;
	const HybridGradeBudget * budgeted = Grade_Budget(b,HYBRID_ROAD_HIGHWAY_RAMP,edges,none,&budget);
	if (budgeted != nullptr) Hybrid_Apply_Grade_Budget(*budgeted,max_grade,k.Q,&samples);
	return Hybrid_Solve_Vertical_Profile(samples,b.P->MaxCutDepth,b.P->MaxFillHeight,max_grade,k.Q,solved,&failure);
}


/*
**	12.6.10 items 3-5, once every crossing on the roads involved is compiled:
**	- the highway graded whole, through X, with every structure on it;
**	- each L_s elevation z_L: DIAMOND_ACCESS, the crossing's profile at L_s; SIDE_ACCESS, the lower-StableId ramp of
**	  that side solved with its gore pinned to the highway profile -- carried by that local junction's record;
**	- every ramp judged whole between its pins (gore = highway profile at the gore, L_s = z_L);
**	- one HIGHWAY_RAMP_MERGE / DIVERGE record per ramp and its merge/diverge samples at min(W/3, A/4), in the
**	  interval's own frame (chain stations relative to X), with the highway's surface elevation.
*/
static bool Compile_Interchange_Elevations(InteractionBuilder & b,HybridInteractionResult * result)
{
	const HybridConstants & k = b.P->Constants;
	HybridInterchangeConstants c;
	Hybrid_Interchange_Constants(k.W,&c);
	const float spacing = Max2(Min2(k.W / 3.0f,0.25f * k.A),k.Q);
	StringClass detail;

	for (int i = 0; i < b.Interchanges.Count(); ++i) {

		const InterchangeView & view = b.Interchanges[i];
		const HybridInterchange & ic = *view.Record;

		HybridRoute highway;
		DynamicVectorClass<HybridProfileSample> highway_samples;
		HybridSolvedProfile highway_solved;
		if (!Solve_Physical_Road(b,ic.HighwayEdgeIn,&highway,&highway_samples,&highway_solved)) {
			detail.Format("the highway through interchange %u cannot be graded by Section 12.1",ic.StableId);
			return Fail(result,HYBRID_INTERACTION_INTERCHANGE,detail.Peek_Buffer());
		}
		const int last = highway.Points.Count() - 1;

		//	The highway's surface under each gore: the straight highway has no cross slope.
		DynamicVectorClass<float> gore_z;
		for (int e = 0; e < ic.RampEdges.Count(); ++e) {
			const HybridTopologyEdge * ramp = b.Topology->Find_Edge(ic.RampEdges[e]);
			if (ramp == nullptr) {
				detail.Format("interchange %u names ramp %u, which is not in the topology",ic.StableId,ic.RampEdges[e]);
				return Fail(result,HYBRID_INTERACTION_INTERCHANGE,detail.Peek_Buffer());
			}
			const float x = (ramp->RampRole == HYBRID_RAMP_ROLE_ON) ? ic.GoreOffset : -ic.GoreOffset;
			const Vector2 gore = Side_Point(view.X,ic.Axis,ramp->RampSide,x,c.DAux);
			gore_z.Add(Profile_Z_At(highway_samples,highway_solved,Realized_Station_Of(highway,0,last,gore)));
		}

		//	z_L on each access side, carried by its local junction.
		float z_local[HYBRID_INTERCHANGE_SIDE_COUNT];
		for (int side = 0; side < HYBRID_INTERCHANGE_SIDE_COUNT; ++side) {

			z_local[side] = 0.0f;
			if ((ic.AccessSides & (1u << side)) == 0) continue;
			const HybridTopologyNode * local = b.Topology->Find_Node(ic.LocalJunction[side]);
			if (local == nullptr) {
				detail.Format("interchange %u has no local junction on side %d",ic.StableId,side);
				return Fail(result,HYBRID_INTERACTION_INTERCHANGE,detail.Peek_Buffer());
			}

			if (ic.Form == HYBRID_INTERCHANGE_DIAMOND_ACCESS) {
				unsigned int crossing_id = 0;
				for (int e = 0; (e < ic.CrossingEdges.Count()) && (crossing_id == 0); ++e) {
					const HybridTopologyEdge * crossing = b.Topology->Find_Edge(ic.CrossingEdges[e]);
					if ((crossing != nullptr) && ((crossing->NodeA == local->StableId) || (crossing->NodeB == local->StableId))) {
						crossing_id = crossing->StableId;
					}
				}
				HybridRoute road;
				DynamicVectorClass<HybridProfileSample> samples;
				HybridSolvedProfile solved;
				if ((crossing_id == 0) || !Solve_Physical_Road(b,crossing_id,&road,&samples,&solved)) {
					detail.Format("the crossing of interchange %u cannot be graded by Section 12.1",ic.StableId);
					return Fail(result,HYBRID_INTERACTION_INTERCHANGE,detail.Peek_Buffer());
				}
				z_local[side] = Profile_Z_At(samples,solved,Realized_Station_Of(road,0,road.Points.Count() - 1,local->Position));
			} else {
				int pick = -1;
				for (int e = 0; e < ic.RampEdges.Count(); ++e) {
					const HybridTopologyEdge * ramp = b.Topology->Find_Edge(ic.RampEdges[e]);
					if (ramp->RampSide != side) continue;
					if ((pick < 0) || (ramp->StableId < ic.RampEdges[pick])) pick = e;
				}
				const HybridRoute * route = (pick >= 0) ? b.Routes->Find(ic.RampEdges[pick]) : nullptr;
				HybridSolvedProfile solved;
				if ((route == nullptr) || !Solve_Ramp_Profile(b,*route,false,0.0f,gore_z[pick],&solved) || (solved.Z.Count() == 0)) {
					detail.Format("interchange %u: the lower ramp of side %d cannot be graded to its gore",ic.StableId,side);
					return Fail(result,HYBRID_INTERACTION_INTERCHANGE,detail.Peek_Buffer());
				}
				z_local[side] = solved.Z[0];
			}
			z_local[side] = Hybrid_Quantize_Position(z_local[side]);

			int records = 0;
			for (int s = 0; s < b.Out->Interactions.Count(); ++s) {
				HybridInteraction & one = b.Out->Interactions[s];
				if ((one.NodeId != local->StableId) || (one.Role != HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_LOCAL_JUNCTION)) continue;
				one.JunctionZ = z_local[side];
				records++;
			}
			if (records != 1) {
				detail.Format("local junction %u of interchange %u has %d compiled junction records",local->StableId,ic.StableId,
								  records);
				return Fail(result,HYBRID_INTERACTION_INTERCHANGE,detail.Peek_Buffer());
			}
		}

		//	Every ramp, whole, between z_L and the highway at its gore.
		for (int e = 0; e < ic.RampEdges.Count(); ++e) {
			const HybridTopologyEdge * ramp = b.Topology->Find_Edge(ic.RampEdges[e]);
			const HybridRoute * route = b.Routes->Find(ramp->StableId);
			HybridSolvedProfile solved;
			if ((route == nullptr) || !Solve_Ramp_Profile(b,*route,true,z_local[ramp->RampSide],gore_z[e],&solved)) {
				detail.Format("ramp %u of interchange %u cannot be graded from z_L %.2f to the highway's %.2f at its gore",
								  ramp->StableId,ic.StableId,z_local[ramp->RampSide],gore_z[e]);
				return Fail(result,HYBRID_INTERACTION_INTERCHANGE,detail.Peek_Buffer());
			}
		}

		//	One merge or diverge record per ramp, with its widened-profile samples.
		for (int e = 0; e < ic.RampEdges.Count(); ++e) {

			const HybridTopologyEdge & ramp = *b.Topology->Find_Edge(ic.RampEdges[e]);
			const bool on = (ramp.RampRole == HYBRID_RAMP_ROLE_ON);
			const HybridRampInterval & interval = ramp.RampInterval;

			HybridInteraction one;
			one.StableId		= b.NextStableId++;
			one.EdgeId			= ramp.StableId;
			one.OtherEdgeId	= (interval.From >= 0.0f) ? ic.HighwayEdgeOut : ic.HighwayEdgeIn;
			one.Method			= HYBRID_METHOD_AT_GRADE_JUNCTION;
			one.RoadClass		= HYBRID_ROAD_HIGHWAY_RAMP;
			one.Station			= Quantized(Side_Point(view.X,ic.Axis,ramp.RampSide,on ? ic.GoreOffset : -ic.GoreOffset,c.DAux));
			one.Tangent			= ic.Axis;
			one.ApproachA		= one.Station;
			one.ApproachB		= one.Station;
			one.Role				= on ? HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_MERGE : HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_DIVERGE;
			one.JunctionZ		= Hybrid_Quantize_Position(gore_z[e]);
			b.Out->Interactions.Add(one);

			HybridMergeDiverge md;
			md.StableId			= b.Out->MergeDiverge.Count() + 1;
			md.InteractionId	= one.StableId;
			md.RampEdgeId		= ramp.StableId;
			md.HighwayEdgeId	= one.OtherEdgeId;
			md.InterchangeId	= ic.StableId;
			md.Role				= one.Role;
			md.Side				= ramp.RampSide;
			md.Interval			= interval;
			const float span = on ? c.MergeLength : c.DivergeLength;
			const float length = interval.To - interval.From;
			const int steps = (int)::ceil((double)(length / spacing));
			const int count = (steps > 0) ? steps : 1;
			for (int s = 0; s <= count; ++s) {
				const float chain = interval.From + (length * ((float)s / (float)count));
				const float x = (ramp.RampSide == HYBRID_INTERCHANGE_SIDE_RIGHT) ? chain : -chain;
				const float along = on ? (x - ic.GoreOffset) : (x + ic.GoreOffset + c.DivergeLength);
				HybridAuxiliaryLaneSection section;
				Hybrid_Auxiliary_Lane_Section(c,ramp.RampRole,Clamp2(along,0.0f,span),&section);
				const Vector2 centre = view.X + (ic.Axis * chain);
				HybridMergeDivergeSample sample;
				sample.Station	= Hybrid_Quantize_Position(chain);
				sample.Full		= section.Full;
				sample.U			= section.U;
				sample.D			= Hybrid_Quantize_Position(section.D);
				sample.H			= Hybrid_Quantize_Position(section.H);
				sample.E			= Hybrid_Quantize_Position(section.E);
				sample.Z			= Hybrid_Quantize_Position(Profile_Z_At(highway_samples,highway_solved,
																					  Realized_Station_Of(highway,0,last,centre)));
				md.Samples.Add(sample);
			}
			b.Out->MergeDiverge.Add(md);
		}
	}
	return true;
}


/*
**	12.6.10 item 6: the swept proxy over every ramp, parallel interval and local junction, with the vehicles the
**	ramp's own capabilities require.  It is the proxy Sweep_Is_Clear runs over a compiled interval: the swept
**	envelope stands on placeable ground and the class cross slope holds across the roadbed.
*/
static bool Sweep_Interchanges(InteractionBuilder & b,HybridInteractionResult * result)
{
	const HybridConstants & k = b.P->Constants;
	HybridInterchangeConstants c;
	Hybrid_Interchange_Constants(k.W,&c);
	const float spacing = Max2(Min2(k.W / 3.0f,0.25f * k.A),k.Q);
	const float ramp_cross = Hybrid_Road_Class_Spec(HYBRID_ROAD_HIGHWAY_RAMP).MaxCrossSlopeDegrees * PI_OVER_180;
	const float highway_cross = Hybrid_Road_Class_Spec(HYBRID_ROAD_HIGHWAY_4).MaxCrossSlopeDegrees * PI_OVER_180;
	StringClass detail;

	for (int i = 0; i < b.Interchanges.Count(); ++i) {

		const InterchangeView & view = b.Interchanges[i];
		const HybridInterchange & ic = *view.Record;
		float half_swept = 0.0f;

		/*
		**	Each ramp, L_s to its gore.  D-024: the vehicle its capabilities require fits inside the paved surface --
		**	the ramp's own effective roadbed, or, where that roadbed has joined the highway's (the gore shoulder
		**	transition, the gore), the combined surface from the highway's median edge out to the ramp's outer edge.
		**	Generic placeable ground beside the roadbed never counts.
		*/
		const float median_edge = 0.5f * Hybrid_Road_Class_Spec(HYBRID_ROAD_HIGHWAY_4).MedianW * k.W;
		for (int e = 0; e < ic.RampEdges.Count(); ++e) {

			const HybridTopologyEdge & ramp = *b.Topology->Find_Edge(ic.RampEdges[e]);
			const HybridRoute * route = b.Routes->Find(ramp.StableId);
			if ((route == nullptr) || (route->Points.Count() < 2)) continue;		// the gate names it
			const float swept = Required_Swept_Width(b,ramp.Capabilities);
			half_swept = Max2(half_swept,0.5f * swept);
			Vector2 tau;
			Vector2 normal;
			Side_Frame(ic.Axis,ramp.RampSide,&tau,&normal);
			const float total = route->Stations[route->Stations.Count() - 1];
			const int steps = (int)::ceil((double)(total / spacing));
			const int count = (steps > 0) ? steps : 1;

			for (int s = 0; s <= count; ++s) {
				const float at = total * ((float)s / (float)count);
				const Vector2 point = Hybrid_Polyline_Point_At(route->Points,route->Stations,at);
				const Vector2 ahead = Hybrid_Polyline_Point_At(route->Points,route->Stations,Min2(at + spacing,total));
				const Vector2 behind = Hybrid_Polyline_Point_At(route->Points,route->Stations,Max2(at - spacing,0.0f));
				const Vector2 direction = Unit(ahead - behind);
				//	Inner is left of the ramp's traffic; an OFF_RAMP is stored L_s -> gore, against it.
				Vector2 inward(-direction.Y,direction.X);
				if (ramp.RampRole == HYBRID_RAMP_ROLE_OFF) inward = -inward;
				float to_inner = 0.0f;
				float to_outer = 0.0f;
				Hybrid_Ramp_Effective_Edges(c,ramp.RampRoadbedExtra,total - at,&to_inner,&to_outer);
				const Vector2 inner_edge = point + (inward * to_inner);
				const Vector2 outer_edge = point - (inward * to_outer);
				b.Out->SweepSamples++;

				float paved = to_inner + to_outer;
				{
					//	Joined to the highway: the ramp's inner edge is on the highway's ramp-side roadbed here.
					const Vector2 inner_offset = inner_edge - view.X;
					const Vector2 outer_offset = outer_edge - view.X;
					const float x = (inner_offset.X * tau.X) + (inner_offset.Y * tau.Y);
					const float y_inner = (inner_offset.X * normal.X) + (inner_offset.Y * normal.Y);
					const float y_outer = (outer_offset.X * normal.X) + (outer_offset.Y * normal.Y);
					float highway_edge = c.EHwy;
					const bool on = (ramp.RampRole == HYBRID_RAMP_ROLE_ON);
					const float along = on ? (x - ic.GoreOffset) : (x + ic.GoreOffset + c.DivergeLength);
					const float span = on ? c.MergeLength : c.DivergeLength;
					if ((along >= 0.0f) && (along <= span)) {
						HybridAuxiliaryLaneSection section;
						Hybrid_Auxiliary_Lane_Section(c,ramp.RampRole,along,&section);
						highway_edge = section.E;
					}
					if (y_inner <= (highway_edge + k.Q)) paved = Max2(paved,y_outer - median_edge);
				}
				if (paved < (swept - k.Q)) {
					detail.Format("ramp %u of interchange %u: a %.2f-wide vehicle does not fit the %.2f of paved surface at "
									  "station %.1f",ramp.StableId,ic.StableId,swept,paved,at);
					return Fail(result,HYBRID_INTERACTION_SWEEP,detail.Peek_Buffer());
				}
				if (!Point_Is_Placeable(b,point) || !Point_Is_Placeable(b,inner_edge) || !Point_Is_Placeable(b,outer_edge)) {
					detail.Format("ramp %u of interchange %u: its roadbed leaves placeable ground at station %.1f",
									  ramp.StableId,ic.StableId,at);
					return Fail(result,HYBRID_INTERACTION_SWEEP,detail.Peek_Buffer());
				}
				float z_in = 0.0f;
				float z_out = 0.0f;
				if (!b.Height_At(inner_edge,&z_in) || !b.Height_At(outer_edge,&z_out) ||
					 !Hybrid_Grade_Compatible(Hybrid_Road_Vertical_Interval(z_in,b.P->MaxCutDepth,b.P->MaxFillHeight),
													  Hybrid_Road_Vertical_Interval(z_out,b.P->MaxCutDepth,b.P->MaxFillHeight),
													  to_inner + to_outer,ramp_cross)) {
					detail.Format("ramp %u of interchange %u: cross slope at station %.1f",ramp.StableId,ic.StableId,at);
					return Fail(result,HYBRID_INTERACTION_SWEEP,detail.Peek_Buffer());
				}
			}
		}

		//	Each parallel interval: the auxiliary lane's swept envelope, and the widened roadbed's cross slope.
		for (int m = 0; m < b.Out->MergeDiverge.Count(); ++m) {
			const HybridMergeDiverge & md = b.Out->MergeDiverge[m];
			if (md.InterchangeId != ic.StableId) continue;
			Vector2 tau;
			Vector2 normal;
			Side_Frame(ic.Axis,md.Side,&tau,&normal);
			for (int s = 0; s < md.Samples.Count(); ++s) {
				const HybridMergeDivergeSample & sample = md.Samples[s];
				const Vector2 centre = view.X + (ic.Axis * sample.Station);
				const Vector2 lane = centre + (normal * sample.D);
				b.Out->SweepSamples++;
				//	D-024: the combined HIGHWAY_4 + auxiliary-lane surface on the ramp side carries the vehicle.
				if ((sample.E - median_edge) < ((2.0f * half_swept) - k.Q)) {
					detail.Format("the parallel interval of ramp %u: %.2f of paved surface for a %.2f-wide vehicle at chain "
									  "station %.1f",md.RampEdgeId,sample.E - median_edge,2.0f * half_swept,sample.Station);
					return Fail(result,HYBRID_INTERACTION_SWEEP,detail.Peek_Buffer());
				}
				if (!Point_Is_Placeable(b,lane) || !Point_Is_Placeable(b,centre + (normal * sample.E))) {
					detail.Format("the parallel interval of ramp %u leaves placeable ground at chain station %.1f",md.RampEdgeId,
									  sample.Station);
					return Fail(result,HYBRID_INTERACTION_SWEEP,detail.Peek_Buffer());
				}
				float z_centre = 0.0f;
				float z_edge = 0.0f;
				if (!b.Height_At(centre,&z_centre) || !b.Height_At(centre + (normal * sample.E),&z_edge) ||
					 !Hybrid_Grade_Compatible(Hybrid_Road_Vertical_Interval(z_centre,b.P->MaxCutDepth,b.P->MaxFillHeight),
													  Hybrid_Road_Vertical_Interval(z_edge,b.P->MaxCutDepth,b.P->MaxFillHeight),
													  sample.E,highway_cross)) {
					detail.Format("the parallel interval of ramp %u: cross slope at chain station %.1f",md.RampEdgeId,sample.Station);
					return Fail(result,HYBRID_INTERACTION_SWEEP,detail.Peek_Buffer());
				}
			}
		}

		//	Each local junction: every road there over its junction reach.
		for (int side = 0; side < HYBRID_INTERCHANGE_SIDE_COUNT; ++side) {
			if ((ic.AccessSides & (1u << side)) == 0) continue;
			const HybridTopologyNode * local = b.Topology->Find_Node(ic.LocalJunction[side]);
			if (local == nullptr) continue;													// the gate names it
			for (int r = 0; r < b.Routes->Routes.Count(); ++r) {
				const HybridRoute & route = b.Routes->Routes[r];
				const HybridTopologyEdge * edge = b.Topology->Find_Edge(route.EdgeId);
				if ((edge == nullptr) || (route.Points.Count() < 2)) continue;
				if ((edge->NodeA != local->StableId) && (edge->NodeB != local->StableId)) continue;
				const HybridRoadClassSpec & spec = Hybrid_Road_Class_Spec(edge->RoadClass);
				const float total = route.Stations[route.Stations.Count() - 1];
				const float reach = Min2(Hybrid_Junction_Arm_Reach(edge->RoadClass,k.W),total);
				const bool from_start = ((route.Points[0] - local->Position).Length() <=
												 (route.Points[route.Points.Count() - 1] - local->Position).Length());
				const float half = Max2(0.5f * Hybrid_Edge_Roadbed_Width(*edge,k.W),
												0.5f * Required_Swept_Width(b,edge->Capabilities));
				const int steps = (int)::ceil((double)(reach / spacing));
				const int count = (steps > 0) ? steps : 1;
				for (int s = 0; s <= count; ++s) {
					const float run = reach * ((float)s / (float)count);
					const float at = from_start ? run : (total - run);
					const Vector2 point = Hybrid_Polyline_Point_At(route.Points,route.Stations,at);
					const Vector2 ahead = Hybrid_Polyline_Point_At(route.Points,route.Stations,Min2(at + spacing,total));
					const Vector2 behind = Hybrid_Polyline_Point_At(route.Points,route.Stations,Max2(at - spacing,0.0f));
					const Vector2 direction = Unit(ahead - behind);
					const Vector2 left(-direction.Y,direction.X);
					b.Out->SweepSamples++;
					if (!Point_Is_Placeable(b,point) || !Point_Is_Placeable(b,point + (left * half)) ||
						 !Point_Is_Placeable(b,point - (left * half))) {
						detail.Format("local junction %u of interchange %u: edge %u's swept envelope leaves placeable ground",
										  local->StableId,ic.StableId,edge->StableId);
						return Fail(result,HYBRID_INTERACTION_SWEEP,detail.Peek_Buffer());
					}
				}
			}
		}
	}
	return true;
}


/*
**	The Phase-11 gate's interchange clause (Section 8 P11): every realized interchange has one merge or diverge record
**	per ramp, one local-junction record per L_s, its DIAMOND_ACCESS crossing compiled -- the reserved overpass, or
**	the intended junction at X -- and no incidental solution between its own elements and its own highway; and no
**	bridge, tunnel or overpass reserve anywhere intersects an active gore reserve.
*/
static bool Check_Interchanges_Gate(InteractionBuilder & b,HybridInteractionResult * result)
{
	const HybridConstants & k = b.P->Constants;
	StringClass detail;

	for (int i = 0; i < b.Interchanges.Count(); ++i) {

		const HybridInterchange & ic = *b.Interchanges[i].Record;

		for (int e = 0; e < ic.RampEdges.Count(); ++e) {
			const HybridTopologyEdge * ramp = b.Topology->Find_Edge(ic.RampEdges[e]);
			const HybridJunctionRole role = (ramp->RampRole == HYBRID_RAMP_ROLE_ON) ? HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_MERGE
																										  : HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_DIVERGE;
			int records = 0;
			for (int s = 0; s < b.Out->Interactions.Count(); ++s) {
				const HybridInteraction & one = b.Out->Interactions[s];
				if ((one.EdgeId == ramp->StableId) && (one.Role == role)) records++;
			}
			int samples = 0;
			for (int m = 0; m < b.Out->MergeDiverge.Count(); ++m) {
				if (b.Out->MergeDiverge[m].RampEdgeId == ramp->StableId) samples++;
			}
			if ((records != 1) || (samples != 1)) {
				detail.Format("interchange %u: ramp %u has %d %s records and %d merge/diverge sample sets",ic.StableId,
								  ramp->StableId,records,Hybrid_Junction_Role_Name(role),samples);
				return Fail(result,HYBRID_INTERACTION_INTERCHANGE,detail.Peek_Buffer());
			}
		}

		for (int side = 0; side < HYBRID_INTERCHANGE_SIDE_COUNT; ++side) {
			if ((ic.AccessSides & (1u << side)) == 0) continue;
			int records = 0;
			for (int s = 0; s < b.Out->Interactions.Count(); ++s) {
				const HybridInteraction & one = b.Out->Interactions[s];
				if ((one.NodeId == ic.LocalJunction[side]) && (one.Role == HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_LOCAL_JUNCTION)) records++;
			}
			if (records != 1) {
				detail.Format("interchange %u: local junction %u has %d local-junction records",ic.StableId,ic.LocalJunction[side],
								  records);
				return Fail(result,HYBRID_INTERACTION_INTERCHANGE,detail.Peek_Buffer());
			}
		}

		if (ic.Form == HYBRID_INTERCHANGE_DIAMOND_ACCESS) {
			int crossings = 0;
			for (int s = 0; s < b.Out->Interactions.Count(); ++s) {
				const HybridInteraction & one = b.Out->Interactions[s];
				if (ic.CrossingMode == HYBRID_INTERCHANGE_CROSSING_GRADE_SEPARATED) {
					const bool pair = (ic.CrossingEdges.Count() > 0) &&
											(((one.EdgeId == ic.CrossingEdges[0]) &&
											  ((one.OtherEdgeId == ic.HighwayEdgeIn) || (one.OtherEdgeId == ic.HighwayEdgeOut))) ||
											 ((one.OtherEdgeId == ic.CrossingEdges[0]) &&
											  ((one.EdgeId == ic.HighwayEdgeIn) || (one.EdgeId == ic.HighwayEdgeOut))));
					if (pair && (Hybrid_Interaction_Junction_Role(one) == HYBRID_JUNCTION_ROLE_OVERPASS_OBLIGATION)) crossings++;
				} else if ((one.Method == HYBRID_METHOD_AT_GRADE_JUNCTION) && (one.NodeId == ic.AnchorNode)) {
					crossings++;
				}
			}
			if (crossings != 1) {
				detail.Format("interchange %u: its DIAMOND_ACCESS crossing has %d compiled solutions",ic.StableId,crossings);
				return Fail(result,HYBRID_INTERACTION_INTERCHANGE,detail.Peek_Buffer());
			}
		}

		for (int s = 0; s < b.Out->Interactions.Count(); ++s) {
			const HybridInteraction & one = b.Out->Interactions[s];
			if (Hybrid_Interaction_Junction_Role(one) != HYBRID_JUNCTION_ROLE_INCIDENTAL) continue;
			const HybridTopologyEdge * edge = b.Topology->Find_Edge(one.EdgeId);
			const HybridTopologyEdge * other = b.Topology->Find_Edge(one.OtherEdgeId);
			if ((edge == nullptr) || (other == nullptr)) continue;
			const bool own_a = (edge->InterchangeId == ic.StableId) && Is_Interchange_Element(*edge);
			const bool own_b = (other->InterchangeId == ic.StableId) && Is_Interchange_Element(*other);
			const bool highway_a = (edge->StableId == ic.HighwayEdgeIn) || (edge->StableId == ic.HighwayEdgeOut);
			const bool highway_b = (other->StableId == ic.HighwayEdgeIn) || (other->StableId == ic.HighwayEdgeOut);
			if ((own_a && highway_b) || (own_b && highway_a)) {
				detail.Format("interchange %u: its element %u was rediscovered as an incidental crossing of its own highway",
								  ic.StableId,own_a ? edge->StableId : other->StableId);
				return Fail(result,HYBRID_INTERACTION_INTERCHANGE,detail.Peek_Buffer());
			}
		}
	}

	//	No bridge, tunnel or overpass reserve intersects an active gore reserve.
	for (int s = 0; s < b.Out->Interactions.Count(); ++s) {
		const HybridInteraction & one = b.Out->Interactions[s];
		const bool structure = Hybrid_Interaction_Counts_As_Bridge(one.Method) || (one.Method == HYBRID_METHOD_TUNNEL) ||
									  (one.Method == HYBRID_METHOD_OVERPASS);
		if (!structure) continue;
		const float radius = 0.5f * Hybrid_Roadbed_Width(one.RoadClass,k.W);
		const bool hits = ((one.Method == HYBRID_METHOD_OVERPASS) && (one.Path.Points.Count() >= 2))
								? Polyline_Hits_Gore_Reserve(b,one.Path.Points,one.Path.Stations,0.0f,
																	  one.Path.Stations[one.Path.Stations.Count() - 1],radius)
								: Segment_Hits_Gore_Reserve(b,one.ApproachA,one.ApproachB,radius);
		if (hits) {
			detail.Format("the compiled %s on edge %u intersects an active gore reserve",Hybrid_Interaction_Method_Name(one.Method),
							  one.EdgeId);
			return Fail(result,HYBRID_INTERACTION_INTERCHANGE,detail.Peek_Buffer());
		}
	}
	return true;
}


/***********************************************************************************************
**	The phase itself, and its gate.
***********************************************************************************************/

//	The route's local heading nearest a point, which is the direction the road crosses there.
static Vector2 Route_Heading_At(const HybridRoute & route,const Vector2 & point)
{
	float nearest = 3.4e38f;
	int at = -1;
	for (int i = 1; i < route.Points.Count(); ++i) {
		const float distance = Distance_To_Segment(point,route.Points[i - 1],route.Points[i],nullptr);
		if (distance >= nearest) continue;
		nearest	= distance;
		at			= i;
	}
	if (at < 0) return Vector2(1.0f,0.0f);
	return Unit(route.Points[at] - route.Points[at - 1]);
}


/*
**	Section 5's bridge and overpass quotas, checked against what actually compiled.  A ford never counts,
**	which is why the count asks for the method rather than for the crossing.
*/
/*
**	Revised D-009 (Q-022): a crossing that is not attached to its road is not a solution.  For every compiled
**	crossing, on the road as it will actually be realized:
**
**		- its path starts and ends on the road's own Phase-10 samples, so it is joined at both ends;
**		- both compiled ends lie on that path, within the position quantum;
**		- the road enters the crossing along its axis and leaves along it, with the turn onto it finished on
**		  open road before the end -- it keeps the lead straight there unless the road itself begins or ends;
**		- a bridge has the full straight approach its selected kind needs, `max(8W, required_approach_length)`,
**		  from each edge of the obstruction out to where the road's turn begins.
**
**	A Phase-11 success with disconnected geometry is no longer a success.
*/
static int Nearest_Index(const DynamicVectorClass<Vector2> & points,const Vector2 & point,float * distance_out)
{
	int best = -1;
	float nearest = 3.4e38f;
	for (int i = 0; i < points.Count(); ++i) {
		const float distance = (points[i] - point).Length();
		if (distance >= nearest) continue;
		nearest = distance;
		best = i;
	}
	if (distance_out != nullptr) *distance_out = nearest;
	return best;
}


static bool Check_Attachment(InteractionBuilder & b,HybridInteractionResult * result)
{
	const HybridConstants & k = b.P->Constants;
	const float q = k.Q;
	const float slack = 2.0f * q;
	const float straight = Min2(k.W,0.5f * k.A);

	for (int r = 0; r < b.Routes->Routes.Count(); ++r) {

		const HybridRoute & route = b.Routes->Routes[r];
		HybridRoute realized;
		if (!Hybrid_Realized_Route(route,*b.Out,&realized)) {
			StringClass detail;
			detail.Format("two compiled crossings on edge %u claim one stretch of it",route.EdgeId);
			return Fail(result,HYBRID_INTERACTION_DETACHED,detail.Peek_Buffer());
		}

		for (int i = 0; i < b.Out->Interactions.Count(); ++i) {

			const HybridInteraction & one = b.Out->Interactions[i];
			if (one.EdgeId != route.EdgeId) continue;
			if (one.Method == HYBRID_METHOD_AT_GRADE_JUNCTION) continue;

			StringClass detail;
			const char * what = Hybrid_Interaction_Method_Name(one.Method);
			const HybridCrossingPath & path = one.Path;
			const int count = path.Points.Count();

			/*
			**	An overpass rides the upper road's own centerline: its ends are points of that road, not samples
			**	of it, and there is no turn onto it to check.
			*/
			if (one.Method == HYBRID_METHOD_OVERPASS) {
				float off_a = 3.4e38f;
				float off_b = 3.4e38f;
				for (int s = 1; s < realized.Points.Count(); ++s) {
					off_a = Min2(off_a,Distance_To_Segment(one.ApproachA,realized.Points[s - 1],realized.Points[s],nullptr));
					off_b = Min2(off_b,Distance_To_Segment(one.ApproachB,realized.Points[s - 1],realized.Points[s],nullptr));
				}
				if ((count < 2) || (off_a > slack) || (off_b > slack)) {
					detail.Format("the overpass on edge %u is not on its upper road: its ends stand %.4f and %.4f off it",
									  route.EdgeId,off_a,off_b);
					return Fail(result,HYBRID_INTERACTION_DETACHED,detail.Peek_Buffer());
				}
				continue;
			}

			if ((count < 2) || (path.DepartPoint < 0) || (path.RejoinPoint >= route.Points.Count()) ||
				 ((path.Points[0] - route.Points[path.DepartPoint]).Length() > q) ||
				 ((path.Points[count - 1] - route.Points[path.RejoinPoint]).Length() > q)) {
				detail.Format("the compiled %s on edge %u is not joined to its road at both ends",what,route.EdgeId);
				return Fail(result,HYBRID_INTERACTION_DETACHED,detail.Peek_Buffer());
			}

			float off_a = 0.0f;
			float off_b = 0.0f;
			Nearest_Index(path.Points,one.ApproachA,&off_a);
			Nearest_Index(path.Points,one.ApproachB,&off_b);
			if ((off_a > slack) || (off_b > slack)) {
				detail.Format("the compiled %s on edge %u has its ends %.4f and %.4f off its own path",what,
								  route.EdgeId,off_a,off_b);
				return Fail(result,HYBRID_INTERACTION_DETACHED,detail.Peek_Buffer());
			}

			//	The axes the road has to enter and leave by, in the order it drives them.
			Vector2 axis_in = Unit(one.ApproachB - one.ApproachA);
			Vector2 axis_out = axis_in;
			if ((one.Method == HYBRID_METHOD_SURFACE_PASS) || (one.Method == HYBRID_METHOD_RAMP_PASS)) {
				const HybridCrossingWindow * window = b.Hydrology->Find_Window(one.WindowId);
				if ((window == nullptr) || (window->Pass.ControlCount < 2)) {
					detail.Format("the compiled %s on edge %u has no frozen pass",what,route.EdgeId);
					return Fail(result,HYBRID_INTERACTION_DETACHED,detail.Peek_Buffer());
				}
				const HybridLocalPassGeometry & pass = window->Pass;
				const int n = pass.ControlCount;
				if (one.ApproachA == pass.Controls[0]) {
					axis_in	= Unit(pass.Controls[1] - pass.Controls[0]);
					axis_out	= Unit(pass.Controls[n - 1] - pass.Controls[n - 2]);
				} else {
					axis_in	= Unit(pass.Controls[n - 2] - pass.Controls[n - 1]);
					axis_out	= Unit(pass.Controls[0] - pass.Controls[1]);
				}
			}

			float at_a = 0.0f;
			float at_b = 0.0f;
			const int index_a = Nearest_Index(realized.Points,one.ApproachA,&at_a);
			const int index_b = Nearest_Index(realized.Points,one.ApproachB,&at_b);
			if ((index_a < 0) || (index_b <= index_a) || (at_a > slack) || (at_b > slack)) {
				detail.Format("the compiled %s on edge %u is not on the realized road",what,route.EdgeId);
				return Fail(result,HYBRID_INTERACTION_DETACHED,detail.Peek_Buffer());
			}

			float behind = 0.0f;
			float ahead = 0.0f;
			Hybrid_Polyline_Straight_Run(realized.Points,index_a,axis_in,q,&behind,nullptr);
			Hybrid_Polyline_Straight_Run(realized.Points,index_b,axis_out,q,nullptr,&ahead);
			const int last = realized.Points.Count() - 1;
			const float before_room = Min2(straight,realized.Stations[index_a]);
			const float after_room = Min2(straight,realized.Stations[last] - realized.Stations[index_b]);
			if ((behind < (before_room - slack)) || (ahead < (after_room - slack))) {
				detail.Format("the road is not aligned with the compiled %s on edge %u: %.3f of %.3f straight before "
								  "it, %.3f of %.3f after it",what,route.EdgeId,behind,before_room,ahead,after_room);
				return Fail(result,HYBRID_INTERACTION_DETACHED,detail.Peek_Buffer());
			}

			if ((one.Method == HYBRID_METHOD_BRIDGE) && (one.BridgeKind >= 0) &&
				 (one.BridgeKind < b.P->BridgeKinds.Count())) {

				const HybridBridgeKind & kind = b.P->BridgeKinds[one.BridgeKind];
				const float required = Max2(STATION_HALF_LENGTH_W * k.W,kind.RequiredApproachLength);
				const Vector2 edge_a = one.Station - (axis_in * (0.5f * one.Span));
				const Vector2 edge_b = one.Station + (axis_in * (0.5f * one.Span));
				const Vector2 & a = realized.Points[index_a];
				const Vector2 & c = realized.Points[index_b];
				const float side_a = ((edge_a.X - a.X) * axis_in.X) + ((edge_a.Y - a.Y) * axis_in.Y) + behind;
				const float side_b = ((c.X - edge_b.X) * axis_in.X) + ((c.Y - edge_b.Y) * axis_in.Y) + ahead;
				if ((side_a < (required - slack)) || (side_b < (required - slack))) {
					detail.Format("the bridge on edge %u has %.3f and %.3f of straight approach where its kind needs "
									  "%.3f",route.EdgeId,side_a,side_b,required);
					return Fail(result,HYBRID_INTERACTION_DETACHED,detail.Peek_Buffer());
				}
			}
		}
	}
	return true;
}


static bool Check_Quotas(InteractionBuilder & b,HybridInteractionResult * result)
{
	int river_bridges = 0;
	int canyon_bridges = 0;
	for (int i = 0; i < b.Out->Interactions.Count(); ++i) {

		const HybridInteraction & one = b.Out->Interactions[i];
		if (!Hybrid_Interaction_Counts_As_Bridge(one.Method)) continue;

		if ((one.Obstruction == HYBRID_OBSTRUCTION_RIVER) ||
			 (one.Obstruction == HYBRID_OBSTRUCTION_LAKE) ||
			 (one.Obstruction == HYBRID_OBSTRUCTION_COAST)) {
			river_bridges++;
		} else if ((one.Obstruction == HYBRID_OBSTRUCTION_CANYON) ||
					  (one.Obstruction == HYBRID_OBSTRUCTION_RAVINE)) {
			canyon_bridges++;
		}
	}
	/*
	**	Q-024: every overpass Phase 9 reserved compiles as its own overpass -- no more, no fewer.  An interchange's
	**	own DIAMOND_ACCESS overpass is not one of Section 5's (12.6.4 step 15 never changes a quota object).
	*/
	int overpasses = 0;
	for (int i = 0; i < b.Out->Interactions.Count(); ++i) {
		const HybridInteraction & one = b.Out->Interactions[i];
		if ((one.Method == HYBRID_METHOD_OVERPASS) && one.Mandatory && !Interchange_Owned(b,one)) overpasses++;
	}

	StringClass detail;
	if (river_bridges < b.Topology->RequiredRiverBridges) {
		detail.Format("%d river bridges compiled against the Section-5 quota of %d",river_bridges,
						  b.Topology->RequiredRiverBridges);
		return Fail(result,HYBRID_INTERACTION_QUOTA,detail.Peek_Buffer());
	}
	if (canyon_bridges < b.Topology->RequiredCanyonBridges) {
		detail.Format("%d canyon/ravine bridges compiled against the Section-5 quota of %d",canyon_bridges,
						  b.Topology->RequiredCanyonBridges);
		return Fail(result,HYBRID_INTERACTION_QUOTA,detail.Peek_Buffer());
	}
	if (overpasses != b.Topology->ReservedOverpassCount) {
		detail.Format("%d reserved overpasses compiled of the %d Phase 9 reserved",overpasses,
						  b.Topology->ReservedOverpassCount);
		return Fail(result,HYBRID_INTERACTION_QUOTA,detail.Peek_Buffer());
	}
	return true;
}


/*
**	The phase's body.  The wrapper below owns the contract a refused phase has to keep -- nothing compiled
**	left behind -- so every `return false` in here can simply say why it failed.
*/
/*
**	D-027 -- structural pins against the terrain grid, before Phase 12.
**
**	Phase 11 builds the very plan Phase 12 will realize (Hybrid_Plan_Realization over the compiled records) and judges
**	every structural pin of it on the native terrain grid with Phase 12's own deformation (Hybrid_Pin_Rasterization).
**	A pin the grid realizes directly keeps its profile.  One it cannot gets a STRUCTURAL_PIN_LANDING -- every ground road
**	through it level at the pin's elevation for exactly L_pin -- and the plan is solved again with it, so the landing is
**	part of vertical feasibility.  A pin that still fails, or a landing that leaves its road no legal profile, rejects the
**	solution the pin belongs to, and that solution's own fallback order decides what replaces it.  An interchange element
**	is never rerouted: its failure fails the phase.  Phase 12 never meets the contradiction first.
*/
static const int PIN_LANDING_ROUNDS = 6;
//	Each run excludes one more compiled solution, and a window's whole station order may have to be walked.
static const int MAX_PIN_REJECTION_RUNS = 256;


//	The distance from `point` to a polyline.
static float Point_To_Polyline(const DynamicVectorClass<Vector2> & points,const Vector2 & point)
{
	float best = 3.4e38f;
	for (int i = 1; i < points.Count(); ++i) best = Min2(best,Distance_To_Segment(point,points[i - 1],points[i],nullptr));
	return best;
}


//	Can this compiled solution be excluded and the phase re-solved, its own fallback order picking what replaces it?
static bool Rejectable(const InteractionBuilder & b,unsigned int interaction_id)
{
	const HybridInteraction * one = b.Out->Find(interaction_id);
	if (one == nullptr) return false;
	const HybridTopologyEdge * edge = b.Topology->Find_Edge(one->EdgeId);
	const HybridTopologyEdge * other = (one->OtherEdgeId != 0) ? b.Topology->Find_Edge(one->OtherEdgeId) : nullptr;
	const HybridJunctionRole role = Hybrid_Interaction_Junction_Role(*one);
	if ((role == HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_LOCAL_JUNCTION) || (role == HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_MERGE) ||
		 (role == HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_DIVERGE)) return false;
	if (((edge != nullptr) && Is_Interchange_Element(*edge)) || ((other != nullptr) && Is_Interchange_Element(*other))) return false;
	return (one->Method == HYBRID_METHOD_BRIDGE) || (one->Method == HYBRID_METHOD_OVERPASS) ||
			 (one->Method == HYBRID_METHOD_AT_GRADE_JUNCTION);
}


static bool Reject_Solution(InteractionBuilder & b,unsigned int interaction_id,const char * why,HybridInteractionResult * result,
									 HybridInteractionProblem problem = HYBRID_PROBLEM_PIN_LANDING)
{
	const HybridInteraction * one = b.Out->Find(interaction_id);
	StringClass detail;
	if (one == nullptr) {
		detail.Format("a structural pin of no compiled solution: %s",why);
		return Fail(result,HYBRID_INTERACTION_REALIZATION,detail.Peek_Buffer());
	}
	const HybridTopologyEdge * edge = b.Topology->Find_Edge(one->EdgeId);
	const HybridTopologyEdge * other = (one->OtherEdgeId != 0) ? b.Topology->Find_Edge(one->OtherEdgeId) : nullptr;
	const HybridJunctionRole role = Hybrid_Interaction_Junction_Role(*one);
	const bool interchange = (role == HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_LOCAL_JUNCTION) ||
									 (role == HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_MERGE) || (role == HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_DIVERGE) ||
									 ((edge != nullptr) && Is_Interchange_Element(*edge)) || ((other != nullptr) && Is_Interchange_Element(*other));
	detail.Format("the %s on edge %u at (%.1f,%.1f): %s",Hybrid_Interaction_Method_Name(one->Method),one->EdgeId,one->Station.X,
					  one->Station.Y,why);
	if (interchange) return Fail(result,HYBRID_INTERACTION_INTERCHANGE,detail.Peek_Buffer());
	if (b.Rejected != nullptr) {
		SolutionKey key;
		key.Method			= one->Method;
		key.EdgeId			= one->EdgeId;
		key.OtherEdgeId	= one->OtherEdgeId;
		key.WindowId		= one->WindowId;
		key.NodeId			= one->NodeId;
		key.Station			= one->Station;
		key.Problem			= problem;
		b.Rejected->Add(key);
	}
	return Fail(result,HYBRID_INTERACTION_REALIZATION,detail.Peek_Buffer());
}


/*
**	D-029: a road's whole profile is infeasible with its fixed heights.  The solution blamed is the one a conflicting
**	constraint comes from -- a junction, a landing's owner, an overpass or a bridge -- the later-compiled one when both
**	sides could be; it is rejected and Phase 11 re-solved without it.  A conflict naming only interchange elements fails
**	the phase there; one naming no constraint (the ground alone) asks for the road's one more envelope, then fails.
*/
static bool Profile_Conflict(InteractionBuilder & b,const HybridRealizationResult & planned,HybridInteractionResult * result)
{
	const HybridConstraintConflict & c = planned.Conflict;
	StringClass detail;
	unsigned int blamed = 0;
	HybridVerticalConstraintKind blamed_kind = HYBRID_VCON_KIND_COUNT;
	bool interchange = false;
	for (int side = 0; side < 2; ++side) {
		const bool has = (side == 0) ? c.HasA : c.HasB;
		if (!has) continue;
		const HybridVerticalConstraint & record = (side == 0) ? c.A : c.B;
		unsigned int owner = record.InteractionId;
		if ((record.Kind == HYBRID_VCON_LOCAL_JUNCTION) || (record.Kind == HYBRID_VCON_RAMP_GORE)) interchange = true;
		if (!Rejectable(b,owner)) {
			const HybridInteraction * one = b.Out->Find(owner);
			const HybridTopologyEdge * edge = (one != nullptr) ? b.Topology->Find_Edge(one->EdgeId) : nullptr;
			if ((edge != nullptr) && Is_Interchange_Element(*edge)) interchange = true;
			continue;
		}
		if (owner < blamed) continue;
		if ((owner > blamed) || (record.Kind == HYBRID_VCON_LANDING)) blamed_kind = record.Kind;
		blamed = owner;
	}
	if (blamed != 0) {
		//	A solution's own STRUCTURAL_PIN_LANDING the road cannot carry stays D-027's refusal.
		const HybridInteractionProblem problem = (blamed_kind == HYBRID_VCON_LANDING) ? HYBRID_PROBLEM_PIN_LANDING
																											: HYBRID_PROBLEM_PROFILE_CONFLICT;
		detail.Format("%s: %s",Hybrid_Interaction_Problem_Name(problem),planned.Detail.Peek_Buffer());
		return Reject_Solution(b,blamed,detail.Peek_Buffer(),result,problem);
	}
	if (interchange) {
		detail.Format("an interchange's fixed heights leave a road no profile: %s",planned.Detail.Peek_Buffer());
		return Fail(result,HYBRID_INTERACTION_INTERCHANGE,detail.Peek_Buffer());
	}
	const HybridRoute * route = b.Routes->Find(planned.EdgeId);
	if ((route != nullptr) && (route->Envelope < 2)) {
		b.Out->RerouteRequests++;
		if (result != nullptr) result->RerouteEdgeId = planned.EdgeId;
	}
	detail.Format("Phase 12's plan cannot be realized: %s",planned.Detail.Peek_Buffer());
	return Fail(result,HYBRID_INTERACTION_REALIZATION,detail.Peek_Buffer());
}


/*
**	Phase 11's last step (D-027, D-029).  The canonical vertical constraints are compiled -- the roads solved in routing
**	order, each with every fixed height on it together, a junction's height the earlier road's own solved surface -- and
**	accepted only when every road's whole profile is feasible.  On the native grid the plan they make is then judged as
**	Phase 12 will realize it; a structural pin the grid cannot realize gets its landing and the constraints are compiled
**	again.  The constraints are frozen on the set for Phase 12 to consume unchanged.
*/
static bool Compile_Realization(InteractionBuilder & b,HybridInteractionResult * result)
{
	const HybridInteractionParams & p = *b.P;
	const HybridConstants & k = p.Constants;
	const bool grid_known = (p.TerrainCellSize > 0.0f);

	HybridRealizationParams params;
	params.Constants					= k;
	params.MaxCutDepth				= p.MaxCutDepth;
	params.MaxFillHeight				= p.MaxFillHeight;
	params.LightVehicleWidth		= p.LightVehicleWidth;
	params.HeavyVehicleWidth		= p.HeavyVehicleWidth;
	params.HarvesterWidth			= p.HarvesterWidth;
	params.MinVehicleLaneClearance= p.MinVehicleLaneClearance;
	params.TerrainCellSize			= p.TerrainCellSize;
	params.TerrainOriginX			= p.TerrainOriginX;
	params.TerrainOriginY			= p.TerrainOriginY;
	params.LightVehicleLength		= p.LightVehicleLength;
	params.HeavyVehicleLength		= p.HeavyVehicleLength;
	params.HarvesterLength			= p.HarvesterLength;
	HybridHeightGrid grid;
	grid.OriginX	= p.TerrainOriginX;
	grid.OriginY	= p.TerrainOriginY;
	grid.Spacing	= p.TerrainCellSize;
	grid.CountX		= 0;
	grid.CountY		= 0;
	const float length = Hybrid_Pin_Landing_Length(p.TerrainCellSize);
	HybridPinEnvelopes envelopes;
	envelopes.Length[HYBRID_ENVELOPE_LIGHT]		= p.LightVehicleLength;
	envelopes.Width[HYBRID_ENVELOPE_LIGHT]		= p.LightVehicleWidth;
	envelopes.Length[HYBRID_ENVELOPE_HEAVY]		= p.HeavyVehicleLength;
	envelopes.Width[HYBRID_ENVELOPE_HEAVY]		= p.HeavyVehicleWidth;
	envelopes.Length[HYBRID_ENVELOPE_HARVESTER]	= p.HarvesterLength;
	envelopes.Width[HYBRID_ENVELOPE_HARVESTER]	= p.HarvesterWidth;
	StringClass detail;

	for (int round = 0; round < PIN_LANDING_ROUNDS; ++round) {

		HybridRealizationPlan plan;
		HybridRealizationResult planned;
		DynamicVectorClass<HybridVerticalConstraint> constraints;
		if (!Hybrid_Compile_Vertical_Constraints(params,*b.Topology,*b.Routes,*b.Out,b.Sample,b.Context,&constraints,&plan,
															  &planned)) {
			if (planned.Failure == HYBRID_REALIZATION_PROFILE) return Profile_Conflict(b,planned,result);
			detail.Format("Phase 12's plan cannot be realized: %s (%s)",Hybrid_Realization_Failure_Name(planned.Failure),
							  planned.Detail.Peek_Buffer());
			return Fail(result,HYBRID_INTERACTION_REALIZATION,detail.Peek_Buffer());
		}
		b.Out->Constraints			= constraints;
		b.Out->ConstraintsCompiled	= true;
		if (!grid_known) return true;

		DynamicVectorClass<HybridRasterIssue> issues;
		if (Hybrid_Plan_Rasterization(plan,k,b.Hydrology,grid,b.Sample,b.Context,p.MaxCutDepth,p.MaxFillHeight,envelopes,&issues)) {
			return true;
		}
		b.Out->ConstraintsCompiled = false;
		bool added = false;
		for (int i = 0; i < issues.Count(); ++i) {
			const HybridRasterIssue & issue = issues[i];
			if (issue.Pin < 0) continue;
			const HybridRealizedPin & pin = plan.Pins[issue.Pin];
			unsigned int owner = pin.InteractionId;
			if (pin.Kind == HYBRID_PIN_DECK_END) {
				const HybridBridgeAssembly * assembly = plan.Find_Bridge(pin.AssemblyId);
				owner = (assembly != nullptr) ? assembly->InteractionId : 0;
			}
			bool landed = false;
			for (int l = 0; (l < b.Out->Landings.Count()) && !landed; ++l) {
				landed = ((b.Out->Landings[l].Position - pin.Position).Length() <= (2.0f * k.Q));
			}
			if (landed) {
				detail.Format("%s, with its STRUCTURAL_PIN_LANDING",issue.Detail.Peek_Buffer());
				return Reject_Solution(b,owner,detail.Peek_Buffer(),result);
			}
			HybridPinLanding landing;
			landing.StableId			= b.Out->Landings.Count() + 1;
			landing.PinKind			= pin.Kind;
			landing.Position			= pin.Position;
			landing.Z					= pin.Z;
			landing.Length				= length;
			landing.InteractionId	= owner;
			b.Out->Landings.Add(landing);
			added = true;
		}
		if (!added) {
			/*
			**	Ordinary roadbed the grid cannot carry, away from every pin: Phase 11 says so, with that road's one more
			**	envelope, never Phase 12.
			*/
			for (int i = 0; i < issues.Count(); ++i) {
				if (issues[i].Pin >= 0) continue;
				const unsigned int edge = plan.Roads[issues[i].Road].EdgeId;
				const HybridRoute * route = b.Routes->Find(edge);
				if ((route != nullptr) && (route->Envelope < 2)) {
					b.Out->RerouteRequests++;
					if (result != nullptr) result->RerouteEdgeId = edge;
				}
				detail.Format("the terrain grid cannot realize edge %u: %s",edge,issues[i].Detail.Peek_Buffer());
				return Fail(result,HYBRID_INTERACTION_REALIZATION,detail.Peek_Buffer());
			}
		}
	}
	return Fail(result,HYBRID_INTERACTION_LIMIT,"the structural pin landings did not settle");
}


static bool Solve_Interactions(const HybridInteractionParams & params,const HybridMapIdentity & identity,
										 const HybridCoarseMatrices & matrices,const HybridLandformSet & landforms,
										 const HybridHydrologySet & hydrology,const HybridAnchorSet & anchors,
										 const HybridTopology & topology,const HybridRouteSet & routes,
										 HybridRouteHeightSampleFunc sample,void * context,
										 HybridInteractionSet * out,HybridInteractionResult * result,
										 const DynamicVectorClass<SolutionKey> * excluded,DynamicVectorClass<SolutionKey> * rejected)
{

	const HybridConstants & k = params.Constants;
	if ((sample == nullptr) || (routes.Routes.Count() == 0) || (topology.Edges.Count() == 0) ||
		 !(k.W > 0.0f) || !(k.A > 0.0f) || !(k.S > 0.0f)) {
		return Fail(result,HYBRID_INTERACTION_INVALID_INPUT,
						"Phase 11 needs the frozen Phase-9 topology, the Phase-10 routes and the world constants");
	}

	InteractionBuilder b;
	b.P					= &params;
	b.Identity			= &identity;
	b.Matrices			= &matrices;
	b.Landforms			= &landforms;
	b.Hydrology			= &hydrology;
	b.Anchors			= &anchors;
	b.Topology			= &topology;
	b.Routes				= &routes;
	b.Sample				= sample;
	b.Context			= context;
	b.Out					= out;
	b.NextStableId		= 1;
	b.Excluded			= excluded;
	b.Rejected			= rejected;
	b.Clear_Rejects();
	Collect_Interchanges(b);

	/*
	**	"Interactions are processed by road edge `StableId`, then increasing along-road distance."  The route
	**	set is in Phase-10 priority order, so the order is taken here rather than assumed.
	*/
	DynamicVectorClass<int> order;
	for (int i = 0; i < routes.Routes.Count(); ++i) order.Add(i);
	for (int i = 1; i < order.Count(); ++i) {
		for (int j = i; j > 0; --j) {
			if (routes.Routes[order[j]].EdgeId >= routes.Routes[order[j - 1]].EdgeId) break;
			const int swap = order[j];
			order[j]			= order[j - 1];
			order[j - 1]	= swap;
		}
	}

	//	Pass one: every road's own crossings of rivers, canyons, ravines, cliffs and ridges.
	for (int r = 0; r < order.Count(); ++r) {

		const HybridRoute & route = routes.Routes[order[r]];
		const HybridTopologyEdge * edge = topology.Find_Edge(route.EdgeId);
		if (edge == nullptr) continue;

		//	Increasing along-road distance: the structural intervals in the order the road meets them.
		DynamicVectorClass<int> intervals;
		for (int s = 0; s < route.Structural.Count(); ++s) intervals.Add(s);
		for (int i = 1; i < intervals.Count(); ++i) {
			for (int j = i; j > 0; --j) {
				if (route.Structural[intervals[j]].FirstPoint >=
					 route.Structural[intervals[j - 1]].FirstPoint) break;
				const int swap = intervals[j];
				intervals[j]		= intervals[j - 1];
				intervals[j - 1]	= swap;
			}
		}

		for (int s = 0; s < intervals.Count(); ++s) {
			if (!Solve_Obstruction(b,*edge,route,route.Structural[intervals[s]],result)) return false;
		}
	}

	/*
	**	Pass two reads every road where it is now realized.  A crossing compiled in pass one may have moved a
	**	stretch of road onto its own path, and a road x road crossing found on the preliminary centerline there
	**	would be a crossing with a road that is no longer there.
	*/
	DynamicVectorClass<HybridRoute> realized;
	for (int i = 0; i < routes.Routes.Count(); ++i) {
		HybridRoute one;
		if (!Hybrid_Realized_Route(routes.Routes[i],*out,&one)) {
			return Fail(result,HYBRID_INTERACTION_INVALID_INPUT,"two compiled crossings claim one stretch of a road");
		}
		realized.Add(one);
	}

	/*
	**	Pass two: road x road.  Every frozen Phase-9 overpass obligation first, because it is reserved and
	**	may not downgrade, then the geometric crossings the two centerlines actually make.
	*/
	for (int n = 0; n < topology.Nodes.Count(); ++n) {

		const HybridTopologyNode & node = topology.Nodes[n];
		if (node.Kind != HYBRID_NODE_OVERPASS_OBLIGATION) continue;
		bool interchange_owned = false;
		for (int i = 0; i < topology.Interchanges.Count(); ++i) {
			if (topology.Interchanges[i].CrossingObligation == node.StableId) interchange_owned = true;
		}
		if (interchange_owned) continue;			// its interchange's own crossing, compiled below (12.6.10)

		//	The two edges that share this obligation, in StableId order.
		const HybridTopologyEdge * first = nullptr;
		const HybridTopologyEdge * second = nullptr;
		for (int e = 0; e < topology.Edges.Count(); ++e) {

			const HybridTopologyEdge & edge = topology.Edges[e];
			bool carries = false;
			for (int w = 0; (w < edge.Waypoints.Count()) && !carries; ++w) {
				if (edge.Waypoints[w] == node.StableId) carries = true;
			}
			if (!carries) continue;
			if (first == nullptr)			first = &edge;
			else if (second == nullptr)	second = &edge;
		}
		if ((first == nullptr) || (second == nullptr)) continue;
		if (result != nullptr) result->RoadCrossings.Meetings[HYBRID_ROAD_MEETING_OBLIGATION]++;

		const HybridRoute * route_a = nullptr;
		const HybridRoute * route_b = nullptr;
		for (int r = 0; r < realized.Count(); ++r) {
			if (realized[r].EdgeId == first->StableId)	route_a = &realized[r];
			if (realized[r].EdgeId == second->StableId)	route_b = &realized[r];
		}
		if ((route_a == nullptr) || (route_b == nullptr)) continue;

		if (!Solve_Road_Crossing(b,*first,*route_a,*second,*route_b,node.Position,
										 Route_Heading_At(*route_a,node.Position),
										 Route_Heading_At(*route_b,node.Position),true,result)) return false;
	}

	//	Each realized interchange's reserved DIAMOND_ACCESS overpass (12.6.10).
	if (!Compile_Interchange_Crossings(b,realized,result)) return false;

	//	The junctions the topology makes, then the crossings it does not.
	if (!Compile_Topology_Junctions(b,realized,result)) return false;

	for (int i = 0; i < order.Count(); ++i) {
		for (int j = i + 1; j < order.Count(); ++j) {

			const HybridRoute & route_a = realized[order[i]];
			const HybridRoute & route_b = realized[order[j]];
			const HybridTopologyEdge * edge_a = topology.Find_Edge(route_a.EdgeId);
			const HybridTopologyEdge * edge_b = topology.Find_Edge(route_b.EdgeId);
			if ((edge_a == nullptr) || (edge_b == nullptr)) continue;

			if (result != nullptr) {
				bool waypoint = false;
				for (int w = 0; w < edge_a->Waypoints.Count(); ++w) {
					if ((edge_a->Waypoints[w] == edge_b->NodeA) || (edge_a->Waypoints[w] == edge_b->NodeB)) waypoint = true;
				}
				for (int w = 0; w < edge_b->Waypoints.Count(); ++w) {
					if ((edge_b->Waypoints[w] == edge_a->NodeA) || (edge_b->Waypoints[w] == edge_a->NodeB)) waypoint = true;
				}
				if (waypoint) result->RoadCrossings.WaypointShared++;
			}

			DynamicVectorClass<Vector2> solved;
			for (int p = 1; p < route_a.Points.Count(); ++p) {
				for (int q = 1; q < route_b.Points.Count(); ++q) {

					Vector2 point(0.0f,0.0f);
					if (!Segments_Cross(route_a.Points[p - 1],route_a.Points[p],route_b.Points[q - 1],
											  route_b.Points[q],&point)) continue;

					//	One solution per crossing: two adjacent segment pairs can report the same one.
					bool already = false;
					for (int s = 0; (s < solved.Count()) && !already; ++s) {
						if ((solved[s] - point).Length() <= Max2(k.W,k.Q)) already = true;
					}
					if (already) continue;
					solved.Add(point);

					/*
					**	Two roads that share a topology node -- as ends, or one ending at the other's waypoint -- meet
					**	by design there, and that meeting is the junction Compile_Topology_Junctions builds.  Only the
					**	stretch the junction owns is theirs: the same two roads crossing again further off is an
					**	ordinary crossing and is solved as one.
					*/
					if (Designed_Meeting_Node(b,*edge_a,*edge_b,point) != 0) continue;
					if (result != nullptr) {
						bool share = false;
						const unsigned int ends[2] = { Hybrid_Route_Edge_End(topology,routes,*edge_a,false),
																 Hybrid_Route_Edge_End(topology,routes,*edge_a,true) };
						for (int e = 0; e < 2; ++e) if (Edge_Carries_Node(b,*edge_b,ends[e])) share = true;
						for (int w = 0; w < edge_a->Waypoints.Count(); ++w) if (Edge_Carries_Node(b,*edge_b,edge_a->Waypoints[w])) share = true;
						if (share) result->RoadCrossings.IncidentalBeyondJunction++;
					}

					//	A crossing inside one road's own structural interval is that crossing, not a junction.
					bool structural = false;
					for (int s = 0; (s < route_a.Structural.Count()) && !structural; ++s) {
						if (Distance_To_Segment(point,route_a.Structural[s].Ingress,
														route_a.Structural[s].Egress,nullptr) <= k.W) structural = true;
					}
					for (int s = 0; (s < route_b.Structural.Count()) && !structural; ++s) {
						if (Distance_To_Segment(point,route_b.Structural[s].Ingress,
														route_b.Structural[s].Egress,nullptr) <= k.W) structural = true;
					}
					if (structural) continue;

					//	Already compiled as a reserved obligation?
					bool reserved = false;
					for (int s = 0; (s < out->Interactions.Count()) && !reserved; ++s) {
						const HybridInteraction & one = out->Interactions[s];
						if (one.Method != HYBRID_METHOD_OVERPASS) continue;
						if ((one.Station - point).Length() <= Max2(k.S,k.A)) reserved = true;
					}
					if (reserved) continue;

					if (result != nullptr) result->RoadCrossings.Meetings[HYBRID_ROAD_MEETING_INCIDENTAL]++;
					if (!Solve_Road_Crossing(b,*edge_a,route_a,*edge_b,route_b,point,
													 Unit(route_a.Points[p] - route_a.Points[p - 1]),
													 Unit(route_b.Points[q] - route_b.Points[q - 1]),false,
													 result)) return false;
				}
			}
		}
	}

	//	Every crossing on the interchanges' roads is compiled: their elevations and merge/diverge records (12.6.10).
	if (!Compile_Interchange_Elevations(b,result)) return false;

	//	D-027 / D-029: the canonical vertical constraints, every road's whole profile, and every structural pin on the grid.
	if (!Compile_Realization(b,result)) return false;

	/*
	**	"Before the phase gate, run lightweight swept proxy validation over every compiled bridge/pass/
	**	tunnel/overpass interval using the cached collision envelopes."
	*/
	for (int i = 0; i < out->Interactions.Count(); ++i) {

		HybridInteractionProblem problem = HYBRID_PROBLEM_NONE;
		if (Sweep_Is_Clear(b,out->Interactions[i],&problem)) continue;

		StringClass detail;
		detail.Format("the compiled %s on edge %u failed its swept proxy check: %s",
						  Hybrid_Interaction_Method_Name(out->Interactions[i].Method),
						  out->Interactions[i].EdgeId,Hybrid_Interaction_Problem_Name(problem));
		return Fail(result,HYBRID_INTERACTION_SWEEP,detail.Peek_Buffer());
	}

	//	...and over every ramp, parallel interval and local junction (12.6.10 item 6).
	if (!Sweep_Interchanges(b,result)) return false;

	//	Every compiled crossing attached to the road that drives it.
	if (!Check_Attachment(b,result)) return false;

	//	The gate's quota half.
	if (!Check_Quotas(b,result)) return false;

	//	The gate's interchange clause.
	if (!Check_Interchanges_Gate(b,result)) return false;

	out->Checksum = Hybrid_Interaction_Checksum(*out,identity.Checksum);
	return true;
}


/*
**	Runtime Phase 11.  A refused phase leaves no compiled solution behind -- Phase 12 must never find a
**	half-solved world -- while the issue records are kept, because they are the diagnostics a refused
**	attempt is read from.
*/
bool Hybrid_Build_Interactions(const HybridInteractionParams & params,const HybridMapIdentity & identity,
										 const HybridCoarseMatrices & matrices,const HybridLandformSet & landforms,
										 const HybridHydrologySet & hydrology,const HybridAnchorSet & anchors,
										 const HybridTopology & topology,const HybridRouteSet & routes,
										 HybridRouteHeightSampleFunc sample,void * context,
										 HybridInteractionSet * out,HybridInteractionResult * result)
{
	if (out == nullptr) return false;
	out->Reset();
	if (result != nullptr) {
		result->Failure			= HYBRID_INTERACTION_OK;
		result->Detail				= "";
		result->RerouteEdgeId	= 0;
		result->RoadCrossings.Reset();
	}

	/*
	**	D-027: a solution rejected for its structural pin is excluded and the phase solved again, deterministically, so its
	**	own fallback order picks what replaces it.  Bounded: each run excludes at least one more solution.
	*/
	DynamicVectorClass<SolutionKey> excluded;
	StringClass rejections;
	for (int run = 0; run <= MAX_PIN_REJECTION_RUNS; ++run) {
		out->Reset();
		if (result != nullptr) {
			result->Failure			= HYBRID_INTERACTION_OK;
			result->Detail				= "";
			result->RerouteEdgeId	= 0;
			result->RoadCrossings.Reset();
			result->RejectedSolutions	= excluded.Count();
			result->Rejections			= rejections;
		}
		DynamicVectorClass<SolutionKey> rejected;
		if (Solve_Interactions(params,identity,matrices,landforms,hydrology,anchors,topology,routes,sample,
										context,out,result,&excluded,&rejected)) return true;
		if (rejected.Count() == 0) break;
		for (int r = 0; r < rejected.Count(); ++r) excluded.Add(rejected[r]);
		if (result != nullptr) {
			rejections += result->Detail;
			rejections += " | ";
			result->RejectedSolutions	= excluded.Count();
			result->Rejections			= rejections;
		}
	}

	out->Interactions.Delete_All();
	out->MergeDiverge.Delete_All();
	out->Landings.Delete_All();
	out->Checksum = 0;
	return false;
}

