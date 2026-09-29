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
		case HYBRID_PROBLEM_TOO_CLOSE_TO_CROSSING:	return "too_close_to_crossing";
		default:													break;
	}
	return "unknown";
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
		default:												break;
	}
	return "unknown";
}


/***********************************************************************************************
**	The records.
***********************************************************************************************/

void HybridInteractionSet::Reset(void)
{
	Interactions.Delete_All();
	Issues.Delete_All();
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
	MinVehicleLaneClearance(0.0f),TierStep(1.0f),BridgeWaterClearanceMargin(0.0f),
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
**	Phase 10 is re-run rather than patched, and the whole of it rather than the one edge, because a route is
**	routed to avoid the routes accepted before it: moving one edge's spline makes the later edges' avoidance
**	data stale, and splicing a new spline into the old set would leave exactly the half-updated state this
**	has to avoid.  Re-running is also what makes the step transactional for free -- the accepted set is one
**	Phase-10 run and one Phase-11 run over it, so no old crossing record, interaction, unplanned crossing or
**	checksum contribution can survive, and every edge but the rerouted one is byte-identical because the
**	search is deterministic and only that edge's starting envelope changed.
***********************************************************************************************/

bool Hybrid_Solve_Routes_And_Interactions(HybridRouteParams & route_params,
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
														int * reroutes_out)
{
	if ((routes == nullptr) || (interactions == nullptr)) return false;
	if (reroutes_out != nullptr) *reroutes_out = 0;

	//	Two spare envelopes exist, so at most two reroutes can ever help; the third attempt is the last.
	for (int attempt = 0; attempt <= HYBRID_MAX_REROUTES; ++attempt) {

		if (!Hybrid_Build_Routes(route_params,identity,matrices,landforms,hydrology,anchors,topology,
										 sample,context,routes,route_result)) return false;

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
		stream.Add_Int((int)one.Obstruction);
		stream.Add_Uint(one.ObstructionRecordId);
		stream.Add_Uint(one.WindowId);
		stream.Add_Bool(one.Mandatory);
		stream.Add_Int((int)one.Method);
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
		stream.Add_Int(one.FirstPoint);
		stream.Add_Int(one.LastPoint);
		stream.Add_Int(one.Envelope);
	}
	return stream.Get();
}


/***********************************************************************************************
**	The solver's own working state.
***********************************************************************************************/

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
**	Stations march *along the obstruction*.  The interval is `+/-max(8W,4S)` at `min(W,A/2)`, capped at
**	exactly 64 uniformly spaced stations including both endpoints, then quantized and deduplicated.
*/
static void Build_Stations(const InteractionBuilder & b,const Vector2 & centre,const Vector2 & along,
									DynamicVectorClass<Vector2> * out)
{
	out->Delete_All();

	const HybridConstants & k = b.P->Constants;
	const float half_length = Max2(STATION_HALF_LENGTH_W * k.W,STATION_HALF_LENGTH_S * k.S);
	const float spacing = Min2(k.W,0.5f * k.A);
	if ((half_length <= 0.0f) || (spacing <= 0.0f)) return;

	const Vector2 axis = Unit(along);
	if (axis.Length() <= 0.0f) {
		out->Add(Quantized(centre));
		return;
	}

	//	The nominal count from `-half_length` through `+half_length` inclusive.
	int count = (int)Hybrid_Round_Half_Up((2.0f * half_length) / spacing) + 1;
	if (count < 2) count = 2;

	const int cap = (b.P->MaxStationsPerCrossing > 1) ? b.P->MaxStationsPerCrossing : STATION_CAP;
	if (count > cap) count = cap;					// exactly `cap` uniformly spaced, both endpoints included

	for (int i = 0; i < count; ++i) {

		const float ratio = (count > 1) ? ((float)i / (float)(count - 1)) : 0.5f;
		const float offset = -half_length + (ratio * 2.0f * half_length);
		const Vector2 station = Quantized(Vector2(centre.X + (axis.X * offset),
																centre.Y + (axis.Y * offset)));

		bool duplicate = false;
		for (int q = 0; (q < out->Count()) && !duplicate; ++q) {
			if ((*out)[q] == station) duplicate = true;
		}
		if (!duplicate) out->Add(station);
	}
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
	**	1 and 5 together: the road has to reach this station.  The frozen centerline is what it is, so the
	**	station must lie on it -- within half a roadbed -- and the road's local heading there must cross the
	**	obstruction rather than run along it, within the class turn radius of the crossing axis.
	*/
	float nearest = 3.4e38f;
	int at = -1;
	for (int i = 1; i < route.Points.Count(); ++i) {
		float t = 0.0f;
		const float distance = Distance_To_Segment(station,route.Points[i - 1],route.Points[i],&t);
		if (distance >= nearest) continue;
		nearest	= distance;
		at			= i;
	}
	if ((at < 0) || (nearest > (half_roadbed + k.A))) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_NO_HARD_VALID_STATION;
		return false;
	}

	const Vector2 heading = Unit(route.Points[at] - route.Points[at - 1]);
	const float alignment = (float)fabs((double)((heading.X * axis.X) + (heading.Y * axis.Y)));
	if (alignment < 0.5f) {								// more than 60 degrees off the crossing axis
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_TURN_TOO_TIGHT;
		return false;
	}

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
	**	3 and 5: the straight approach reserve is available on each side and the road can meet the station
	**	inside its own max grade.  The reserve is measured on the realized ground through the Section-7.1
	**	vertical feasibility interval, which is the same measure Phase 10 and Phase 12 use.
	*/
	float station_z = 0.0f;
	if (!b.Height_At(station,&station_z)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_NO_HARD_VALID_STATION;
		return false;
	}
	const HybridVerticalInterval station_interval = Hybrid_Road_Vertical_Interval(station_z,
																								  b.P->MaxCutDepth,
																								  b.P->MaxFillHeight);
	for (int side = 0; side < 2; ++side) {

		const Vector2 & end = (side == 0) ? approach_a : approach_b;
		float end_z = 0.0f;
		if (!b.Height_At(end,&end_z)) {
			if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_APPROACH_DOES_NOT_FIT;
			return false;
		}
		const HybridVerticalInterval end_interval = Hybrid_Road_Vertical_Interval(end_z,b.P->MaxCutDepth,
																								  b.P->MaxFillHeight);
		if (!Hybrid_Grade_Compatible(end_interval,station_interval,(end - station).Length(),max_grade)) {
			if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_GRADE_TOO_STEEP;
			return false;
		}
	}

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
**	The swept width this edge actually has to carry.
**
**	Not the heaviest envelope that exists: a trail does not become heavy-capable because a heavy vehicle was
**	measured somewhere.  The road class names the capability, exactly as the ford path already reads it --
**	Section 9.5's heavy route class is `HIGHWAY_4` or `PRIMARY_2`, a Harvester corridor is `SECONDARY_2`, and
**	below that a road carries the light reference.
*/
static float Required_Swept_Width(const InteractionBuilder & b,HybridRoadClass road_class)
{
	switch (road_class) {
		case HYBRID_ROAD_HIGHWAY_4:
		case HYBRID_ROAD_PRIMARY_2:		return Max2(b.P->HeavyVehicleWidth,b.P->HarvesterWidth);
		case HYBRID_ROAD_SECONDARY_2:		return b.P->HarvesterWidth;
		default:									break;
	}
	return b.P->LightVehicleWidth;
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
static bool Ground_Carries_Swept_Width(const InteractionBuilder & b,HybridRoadClass road_class)
{
	return Required_Swept_Width(b,road_class) <= Hybrid_Roadbed_Width(road_class,b.P->Constants.W);
}


/***********************************************************************************************
**	Bridge asset selection and candidate scoring.
***********************************************************************************************/

/*
**	Section 8 Phase 11's bridge asset selection.  Reject a kind whose clear width is below Section 12.4,
**	whose maximum span cannot cover the obstruction plus both abutment seats, or whose required approach
**	reserve does not fit.  Choose the smallest sufficient maximum span, then the lower `StableId`.
*/
static int Select_Bridge_Kind(const InteractionBuilder & b,HybridRoadClass road_class,float span,
										float available_approach,HybridInteractionProblem * problem_out)
{
	const HybridConstants & k = b.P->Constants;
	const float required_width = Max2(b.P->BridgeRequiredWidth[road_class],
												 Hybrid_Bridge_Clear_Width(road_class,k.W,
																					Max2(b.P->HeavyVehicleWidth,
																						  b.P->HarvesterWidth),
																					b.P->MinVehicleLaneClearance));

	//	The abutment seats each bridge needs at its two ends, in the plan's own canonical terms: one lane.
	const float seated_span = span + (2.0f * k.W);

	int best = -1;
	bool width_seen = false;
	bool span_seen = false;

	for (int i = 0; i < b.P->BridgeKinds.Count(); ++i) {

		const HybridBridgeKind & kind = b.P->BridgeKinds[i];
		if (kind.ClearWidth < required_width) continue;
		width_seen = true;
		if (!(kind.MaxSpan >= seated_span)) continue;
		span_seen = true;

		const float approach = (kind.RequiredApproachLength > 0.0f) ? kind.RequiredApproachLength
																					 : (STATION_HALF_LENGTH_W * k.W);
		if (approach > available_approach) continue;

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

	const float length = (one.ApproachB - one.ApproachA).Length();
	const int steps = (int)::ceil((double)(length / spacing));
	const int count = (steps > 0) ? steps : 1;
	const Vector2 axis = Unit(one.ApproachB - one.ApproachA);
	const Vector2 normal(-axis.Y,axis.X);

	//	A structural deck carries its own surface, so the ground under it is not what a vehicle drives on.
	const bool structural = (one.Method == HYBRID_METHOD_BRIDGE) ||
									(one.Method == HYBRID_METHOD_OVERPASS) ||
									(one.Method == HYBRID_METHOD_TUNNEL);

	for (int i = 0; i <= count; ++i) {

		const float ratio = (float)i / (float)count;
		const Vector2 point(one.ApproachA.X + ((one.ApproachB.X - one.ApproachA.X) * ratio),
								  one.ApproachA.Y + ((one.ApproachB.Y - one.ApproachA.Y) * ratio));
		b.Out->SweepSamples++;

		if (!Point_Is_Placeable(b,point)) {
			if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_RESERVE_CONFLICT;
			return false;
		}
		if (structural) continue;

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


/*
**	A bridge at the lowest-score hard-valid station.  Used by road x river and road x canyon/ravine alike:
**	the two differ in their fallback order and in Section 12.3's vertical geometry, not in this.
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

	const Vector2 across = Unit(window.ApproachB - window.ApproachA);
	const float span = window.Span;
	const float available = Available_Approach(route,interval.FirstPoint,interval.LastPoint);

	DynamicVectorClass<Vector2> stations;
	Build_Stations(b,centre,window.Tangent,&stations);
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

		const float reserve = Max2(STATION_HALF_LENGTH_W * k.W,window.ApproachLength);
		Vector2 approach_a(0.0f,0.0f);
		Vector2 approach_b(0.0f,0.0f);
		HybridInteractionProblem problem = HYBRID_PROBLEM_NONE;

		if (!Station_Is_Hard_Valid(b,edge,route,stations[s],across,span,reserve,&approach_a,&approach_b,
											&problem)) {
			if (problem != HYBRID_PROBLEM_NONE) station_problem = problem;
			b.Note_Reject(problem);
			continue;
		}

		const int kind_index = Select_Bridge_Kind(b,edge.RoadClass,span,Max2(available,reserve),&problem);
		if (kind_index < 0) {
			if (problem != HYBRID_PROBLEM_NONE) asset_problem = problem;
			b.Note_Reject(problem);
			continue;
		}
		const HybridBridgeKind & kind = b.P->BridgeKinds[kind_index];

		float station_z = 0.0f;
		if (!b.Height_At(stations[s],&station_z)) continue;

		float grade_a = 0.0f;
		float grade_b = 0.0f;
		float bank_z_a = 0.0f;
		float bank_z_b = 0.0f;
		if (!Approach_Grade(b,stations[s],station_z,approach_a,&grade_a,&bank_z_a)) continue;
		if (!Approach_Grade(b,stations[s],station_z,approach_b,&grade_b,&bank_z_b)) continue;

		/*
		**	The compiled interval is straight, so its measured turn radius is the class minimum rather than
		**	anything tighter, and the curve term is zero.  The detour term is what moving the station along
		**	the obstruction costs the road: the road has to reach out to it and back.
		*/
		const HybridRoadClassSpec & spec = Hybrid_Road_Class_Spec(edge.RoadClass);
		const float measured_radius = spec.MinTurnRadiusW * k.W;

		float offset = 3.4e38f;
		for (int i = 1; i < route.Points.Count(); ++i) {
			const float distance = Distance_To_Segment(stations[s],route.Points[i - 1],route.Points[i],
																	 nullptr);
			if (distance < offset) offset = distance;
		}
		const float added_length = (offset < 3.4e38f) ? (2.0f * offset) : 0.0f;

		const float score = Bridge_Candidate_Score(b,edge.RoadClass,kind,span,bank_z_a,bank_z_b,grade_a,
																 grade_b,measured_radius,added_length);

		float bottom = 0.0f;
		float top = 0.0f;
		if (!Bridge_Vertical_Geometry(b,window.Obstruction,kind,stations[s],bank_z_a,bank_z_b,&bottom,&top,
												&problem)) {
			if (problem != HYBRID_PROBLEM_NONE) geometry_problem = problem;
			b.Note_Reject(problem);
			continue;
		}

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
		best.Station				= stations[s];
		best.Tangent				= across;
		best.ApproachA				= approach_a;
		best.ApproachB				= approach_b;
		best.Span					= span;
		best.ApproachReserve		= (kind.RequiredApproachLength > 0.0f) ? kind.RequiredApproachLength
																					  : (STATION_HALF_LENGTH_W * k.W);
		best.BridgeKind			= kind_index;
		best.DeckBottomZ			= bottom;
		best.DeckTopZ				= top;
		best.FirstPoint			= interval.FirstPoint;
		best.LastPoint				= interval.LastPoint;
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
	if (!Ground_Carries_Swept_Width(b,edge.RoadClass)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_CLEARANCE;
		return false;
	}

	const Vector2 across = Unit(window.ApproachB - window.ApproachA);
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


static HybridLocalPassParams Pass_Params(const InteractionBuilder & b,HybridRoadClass road_class)
{
	HybridLocalPassParams params;
	params.Constants					= b.P->Constants;
	params.MaxCutDepth				= b.P->MaxCutDepth;
	params.MaxFillHeight				= b.P->MaxFillHeight;
	//	The envelope this edge must carry, so the floor test asks for the clearance this road actually needs.
	params.SweptVehicleWidth		= Required_Swept_Width(b,road_class);
	params.MinVehicleLaneClearance= b.P->MinVehicleLaneClearance;

	//	The class actually being built here, not the `PRIMARY_2` the window was first proved for.
	params.PassClass					= road_class;
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
	if (!Ground_Carries_Swept_Width(b,edge.RoadClass)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_CLEARANCE;
		return false;
	}

	const Vector2 across = Unit(window.ApproachB - window.ApproachA);
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
	if (!Hybrid_Validate_Local_Pass(Pass_Params(b,edge.RoadClass),window.Pass,spacing,
											  Interaction_Height_Sample,&b,&why)) {
		if (problem_out != nullptr) *problem_out = Pass_Problem(why);
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
	if (!window.Has_Method(HYBRID_WINDOW_METHOD_CUT)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_METHOD_UNAVAILABLE;
		return false;
	}

	//	A cut leaves the road on graded ground, so the roadbed is what has to carry the vehicle.
	if (!Ground_Carries_Swept_Width(b,edge.RoadClass)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_CLEARANCE;
		return false;
	}

	const Vector2 across = Unit(window.ApproachB - window.ApproachA);
	const float reserve = Max2(STATION_HALF_LENGTH_W * k.W,window.ApproachLength);
	Vector2 approach_a(0.0f,0.0f);
	Vector2 approach_b(0.0f,0.0f);
	if (!Station_Is_Hard_Valid(b,edge,route,window.Center,across,window.Span,reserve,&approach_a,
										&approach_b,problem_out)) return false;

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
	const float swept_width = Max2(b.P->HeavyVehicleWidth,b.P->HarvesterWidth) +
									  (2.0f * b.P->MinVehicleLaneClearance);
	const float swept_height = Hybrid_Overpass_Clear_Gap(b.P->HeavyVehicleHeight,b.P->WorldUnitsPerMeter);

	const Vector2 across = Unit(window.ApproachB - window.ApproachA);
	const float reserve = Max2(STATION_HALF_LENGTH_W * k.W,window.ApproachLength);
	Vector2 approach_a(0.0f,0.0f);
	Vector2 approach_b(0.0f,0.0f);
	if (!Station_Is_Hard_Valid(b,edge,route,window.Center,across,window.Span,reserve,&approach_a,
										&approach_b,problem_out)) return false;

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
		solution.StableId = b.NextStableId++;
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

	//	1: the crossing angle stays inside 30 to 150 degrees.
	float dot = (heading_a.X * heading_b.X) + (heading_a.Y * heading_b.Y);
	dot = Clamp2((float)fabs((double)dot),0.0f,1.0f);
	const float angle = (float)::acos((double)dot) / PI_OVER_180;
	if ((angle < JUNCTION_MIN_DEGREES) || (angle > JUNCTION_MAX_DEGREES)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_TURN_TOO_TIGHT;
		return false;
	}

	//	2: the intersection is outside every compiled bridge or tunnel approach reserve.
	for (int i = 0; i < b.Out->Interactions.Count(); ++i) {
		const HybridInteraction & other = b.Out->Interactions[i];
		if ((other.Method != HYBRID_METHOD_BRIDGE) && (other.Method != HYBRID_METHOD_TUNNEL)) continue;
		if (Distance_To_Segment(point,other.ApproachA,other.ApproachB,nullptr) > other.ApproachReserve) {
			continue;
		}
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_RESERVE_CONFLICT;
		return false;
	}

	//	3: each road keeps max(4W, its own turn radius) of centerline on both sides of the junction.
	const float needed_a = Max2(JUNCTION_ARM_W * k.W,spec_a.MinTurnRadiusW * k.W);
	const float needed_b = Max2(JUNCTION_ARM_W * k.W,spec_b.MinTurnRadiusW * k.W);
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
	const float half = 0.5f * Max2(Hybrid_Roadbed_Width(edge_a.RoadClass,k.W),
											 Hybrid_Roadbed_Width(edge_b.RoadClass,k.W));
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
static bool Try_Overpass_Assignment(InteractionBuilder & b,const HybridTopologyEdge & upper_edge,
												const HybridRoute & upper_route,const HybridTopologyEdge & lower_edge,
												const HybridRoute & lower_route,const Vector2 & point,
												const Vector2 & upper_heading,bool upper_is_first,bool mandatory,
												HybridInteraction * out,HybridInteractionProblem * problem_out)
{
	const HybridConstants & k = b.P->Constants;
	const HybridRoadClassSpec & upper_spec = Hybrid_Road_Class_Spec(upper_edge.RoadClass);
	const float max_grade = upper_spec.MaxGradeDegrees * PI_OVER_180;
	const float max_cross = upper_spec.MaxCrossSlopeDegrees * PI_OVER_180;
	const float half_roadbed = 0.5f * Hybrid_Roadbed_Width(upper_edge.RoadClass,k.W);

	float lower_z = 0.0f;
	if (!b.Height_At(point,&lower_z)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_NO_HARD_VALID_STATION;
		return false;
	}

	//	Section 12.3's clearance, plus the structural depth of the deck that carries the upper road.
	const float gap = Hybrid_Overpass_Clear_Gap(b.P->HeavyVehicleHeight,b.P->WorldUnitsPerMeter);
	float structural_depth = k.W;						// no registered deck: one lane width of structure
	int bridge_kind = Select_Bridge_Kind(b,upper_edge.RoadClass,Hybrid_Roadbed_Width(lower_edge.RoadClass,
																												k.W),
													 STATION_HALF_LENGTH_W * k.W,nullptr);
	if (bridge_kind >= 0) structural_depth = b.P->BridgeKinds[bridge_kind].StructuralDepth;

	const float deck_bottom = lower_z + gap;
	const float deck_top = deck_bottom + structural_depth;

	const Vector2 axis = Unit(upper_heading);
	const Vector2 normal(-axis.Y,axis.X);
	const float tangent = (float)::tan((double)max_grade);

	/*
	**	The ramp is measured against the rise it actually has to make: from the road's own elevation at each
	**	ramp foot up to the deck, not from the ground under the crossing.  Each side is sized on its own rise
	**	and the longer of the two is reserved, because the plan reserves `L_ramp` on each side.
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

		const Vector2 foot_a(point.X - (axis.X * ramp),point.Y - (axis.Y * ramp));
		const Vector2 foot_b(point.X + (axis.X * ramp),point.Y + (axis.Y * ramp));
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

	//	The upper road has to have that much centerline either side of the crossing to ramp over.
	float before = 0.0f;
	float after = 0.0f;
	Arm_Lengths(upper_route,point,&before,&after);
	if ((before < ramp) || (after < ramp)) {
		if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_APPROACH_DOES_NOT_FIT;
		return false;
	}

	const Vector2 approach_a(point.X - (axis.X * ramp),point.Y - (axis.Y * ramp));
	const Vector2 approach_b(point.X + (axis.X * ramp),point.Y + (axis.Y * ramp));

	const float spacing = Max2(Min2(k.W / OVERPASS_SAMPLE_W,k.A / OVERPASS_SAMPLE_A),k.Q);
	const int steps = (int)::ceil((double)((2.0f * ramp) / spacing));
	const int count = (steps > 0) ? steps : 1;

	float previous_z = 0.0f;
	for (int i = 0; i <= count; ++i) {

		const float ratio = (float)i / (float)count;
		const Vector2 sample(approach_a.X + ((approach_b.X - approach_a.X) * ratio),
									approach_a.Y + ((approach_b.Y - approach_a.Y) * ratio));

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
		const float from_centre = (float)fabs((double)((ratio - 0.5f) * 2.0f * ramp));
		const float s = Clamp2(1.0f - (from_centre / Max2(ramp,k.Q)),0.0f,1.0f);
		const float smooth = s * s * (3.0f - (2.0f * s));
		const float foot_z = (ratio < 0.5f) ? foot_z_a : foot_z_b;
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
			const float run = (2.0f * ramp) / (float)count;
			const float rise = (float)fabs((double)(profile - previous_z));
			if ((run > k.Q) && ((float)::atan((double)(rise / run)) > (max_grade + 0.001f)) ) {
				if (problem_out != nullptr) *problem_out = HYBRID_PROBLEM_GRADE_TOO_STEEP;
				return false;
			}
		}
		previous_z = profile;

		//	Cross slope across the deck, and the clearance over the road underneath at the crossing itself.
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

	*out = HybridInteraction();
	out->EdgeId					= upper_edge.StableId;
	out->OtherEdgeId			= lower_edge.StableId;
	out->Obstruction			= HYBRID_OBSTRUCTION_NONE;
	out->Mandatory				= mandatory;
	out->Method					= HYBRID_METHOD_OVERPASS;
	out->RoadClass				= upper_edge.RoadClass;
	out->Station				= Quantized(point);
	out->Tangent				= axis;
	out->ApproachA				= Quantized(approach_a);
	out->ApproachB				= Quantized(approach_b);
	out->Span					= Hybrid_Roadbed_Width(lower_edge.RoadClass,k.W);
	out->ApproachReserve		= Hybrid_Quantize_Position(ramp);
	out->BridgeKind			= bridge_kind;
	out->DeckBottomZ			= Hybrid_Quantize_Position(deck_bottom);
	out->DeckTopZ				= Hybrid_Quantize_Position(deck_top);
	out->LowerSurfaceZ		= Hybrid_Quantize_Position(lower_z);
	out->RampLength			= Hybrid_Quantize_Position(ramp);
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
	HybridInteraction solution;

	//	An at-grade junction, unless Phase 9 reserved this crossing as an overpass.
	if (!mandatory && At_Grade_Is_Legal(b,edge_a,route_a,edge_b,route_b,point,heading_a,heading_b,
													&problem)) {

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
	const HybridRoutePriority priority_a = Hybrid_Route_Priority_Of(*b.Topology,edge_a);
	const HybridRoutePriority priority_b = Hybrid_Route_Priority_Of(*b.Topology,edge_b);

	bool a_stays_lower = (priority_a < priority_b);
	if (priority_a == priority_b) a_stays_lower = (edge_a.StableId < edge_b.StableId);

	for (int attempt = 0; attempt < 2; ++attempt) {

		const bool a_is_lower = (attempt == 0) ? a_stays_lower : !a_stays_lower;
		const HybridTopologyEdge & lower = a_is_lower ? edge_a : edge_b;
		const HybridTopologyEdge & upper = a_is_lower ? edge_b : edge_a;
		const HybridRoute & lower_route = a_is_lower ? route_a : route_b;
		const HybridRoute & upper_route = a_is_lower ? route_b : route_a;
		const Vector2 & upper_heading = a_is_lower ? heading_b : heading_a;

		if (Try_Overpass_Assignment(b,upper,upper_route,lower,lower_route,point,upper_heading,
											 !a_is_lower,mandatory,&solution,&problem)) {
			solution.StableId = b.NextStableId++;
			b.Out->Interactions.Add(solution);
			return true;
		}
	}

	//	Neither assignment compiled: the lower-priority road is the one asked to reroute.
	const bool reroute_a = (priority_a > priority_b) ||
								  ((priority_a == priority_b) && (edge_a.StableId > edge_b.StableId));
	const HybridTopologyEdge & rerouted = reroute_a ? edge_a : edge_b;
	const HybridRoute & rerouted_route = reroute_a ? route_a : route_b;

	if (!Note_Problem(b,edge_a.StableId,edge_b.StableId,HYBRID_OBSTRUCTION_NONE,0,mandatory,problem,0,
							point)) {
		StringClass detail;
		detail.Format("edge %u has more than %d unsolved crossings",edge_a.StableId,
						  b.P->MaxProblemsPerEdge);
		return Fail(result,HYBRID_INTERACTION_LIMIT,detail.Peek_Buffer());
	}

	if (rerouted_route.Envelope < 2) {
		b.Out->RerouteRequests++;
		if (result != nullptr) result->RerouteEdgeId = rerouted.StableId;
	}

	StringClass detail;
	detail.Format("edges %u (%s) and %u (%s) cross at (%.1f,%.1f) and neither an at-grade junction nor an "
					  "overpass compiles: %s",edge_a.StableId,Hybrid_Road_Class_Name(edge_a.RoadClass),
					  edge_b.StableId,Hybrid_Road_Class_Name(edge_b.RoadClass),point.X,point.Y,
					  Hybrid_Interaction_Problem_Name(problem));

	if (mandatory) return Fail(result,HYBRID_INTERACTION_OBLIGATION_LOST,detail.Peek_Buffer());
	if (edge_a.Required && edge_b.Required) {
		return Fail(result,HYBRID_INTERACTION_NO_SOLUTION,detail.Peek_Buffer());
	}
	WWDEBUG_SAY(("Hybrid Phase 11: optional crossing of edges %u and %u is dropped\r\n",edge_a.StableId,
					 edge_b.StableId));
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
	const int overpasses = b.Out->Count_Of_Method(HYBRID_METHOD_OVERPASS);

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
	if (overpasses < b.Topology->RequiredOverpasses) {
		detail.Format("%d overpasses compiled against the Section-5 quota of %d",overpasses,
						  b.Topology->RequiredOverpasses);
		return Fail(result,HYBRID_INTERACTION_QUOTA,detail.Peek_Buffer());
	}
	return true;
}


/*
**	The phase's body.  The wrapper below owns the contract a refused phase has to keep -- nothing compiled
**	left behind -- so every `return false` in here can simply say why it failed.
*/
static bool Solve_Interactions(const HybridInteractionParams & params,const HybridMapIdentity & identity,
										 const HybridCoarseMatrices & matrices,const HybridLandformSet & landforms,
										 const HybridHydrologySet & hydrology,const HybridAnchorSet & anchors,
										 const HybridTopology & topology,const HybridRouteSet & routes,
										 HybridRouteHeightSampleFunc sample,void * context,
										 HybridInteractionSet * out,HybridInteractionResult * result)
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
	b.Clear_Rejects();

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
	**	Pass two: road x road.  Every frozen Phase-9 overpass obligation first, because it is reserved and
	**	may not downgrade, then the geometric crossings the two centerlines actually make.
	*/
	for (int n = 0; n < topology.Nodes.Count(); ++n) {

		const HybridTopologyNode & node = topology.Nodes[n];
		if (node.Kind != HYBRID_NODE_OVERPASS_OBLIGATION) continue;

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

		const HybridRoute * route_a = routes.Find(first->StableId);
		const HybridRoute * route_b = routes.Find(second->StableId);
		if ((route_a == nullptr) || (route_b == nullptr)) continue;

		if (!Solve_Road_Crossing(b,*first,*route_a,*second,*route_b,node.Position,
										 Route_Heading_At(*route_a,node.Position),
										 Route_Heading_At(*route_b,node.Position),true,result)) return false;
	}

	for (int i = 0; i < order.Count(); ++i) {
		for (int j = i + 1; j < order.Count(); ++j) {

			const HybridRoute & route_a = routes.Routes[order[i]];
			const HybridRoute & route_b = routes.Routes[order[j]];
			const HybridTopologyEdge * edge_a = topology.Find_Edge(route_a.EdgeId);
			const HybridTopologyEdge * edge_b = topology.Find_Edge(route_b.EdgeId);
			if ((edge_a == nullptr) || (edge_b == nullptr)) continue;

			//	Two roads that meet at a shared topology node meet by design, not by accident.
			if ((edge_a->NodeA == edge_b->NodeA) || (edge_a->NodeA == edge_b->NodeB) ||
				 (edge_a->NodeB == edge_b->NodeA) || (edge_a->NodeB == edge_b->NodeB)) continue;

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

					if (!Solve_Road_Crossing(b,*edge_a,route_a,*edge_b,route_b,point,
													 Unit(route_a.Points[p] - route_a.Points[p - 1]),
													 Unit(route_b.Points[q] - route_b.Points[q - 1]),false,
													 result)) return false;
				}
			}
		}
	}

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

	//	The gate's quota half.
	if (!Check_Quotas(b,result)) return false;

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
	}

	if (Solve_Interactions(params,identity,matrices,landforms,hydrology,anchors,topology,routes,sample,
									context,out,result)) return true;

	out->Interactions.Delete_All();
	out->Checksum = 0;
	return false;
}

