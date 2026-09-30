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
 *                     $Archive:: /Commando/Code/wwphys/hybridlocalpass.cpp                   $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * See hybridlocalpass.h.  One builder, two shapes, and one rule about what is being judged: the   *
 * candidate is planned road geometry, so the road's own profile is what must hold the class grade   *
 * and turn radius, while the untouched ground only has to be close enough that the profile fits     *
 * inside its `RoadVerticalInterval`.                                                                *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#include "hybridlocalpass.h"

#include <math.h>


static inline float Max2(float a,float b)	{ return (a > b) ? a : b; }
static inline float Min2(float a,float b)	{ return (a < b) ? a : b; }

static const float	PI_OVER_180	= 3.14159265358979323846f / 180.0f;

//	The plan's swept-route sample spacing for a pass: `min(W/2, A/3)`.
static const float	SPACING_W	= 2.0f;
static const float	SPACING_A	= 3.0f;

HybridLocalPassParams::HybridLocalPassParams(void) :
	MaxCutDepth(0.0f),MaxFillHeight(0.0f),SweptVehicleWidth(0.0f),MinVehicleLaneClearance(0.0f),
	PassClass(HYBRID_ROAD_PRIMARY_2)
{
	::memset(&Constants,0,sizeof(Constants));
}


const char * Hybrid_Local_Pass_Kind_Name(HybridLocalPassKind kind)
{
	switch (kind) {
		case HYBRID_LOCAL_PASS_FLOOR:	return "floor_pass";
		case HYBRID_LOCAL_PASS_RAMP:	return "ramp_pass";
		default:								break;
	}
	return "unknown";
}


const char * Hybrid_Local_Pass_Failure_Name(HybridLocalPassFailure failure)
{
	switch (failure) {
		case HYBRID_LOCAL_PASS_OK:				return "ok";
		case HYBRID_LOCAL_PASS_NO_GEOMETRY:	return "no_geometry";
		case HYBRID_LOCAL_PASS_NO_HEIGHT:	return "no_height";
		case HYBRID_LOCAL_PASS_RUN:			return "run";
		case HYBRID_LOCAL_PASS_GRADE:			return "grade";
		case HYBRID_LOCAL_PASS_TURN_RADIUS:	return "turn_radius";
		case HYBRID_LOCAL_PASS_CROSS_SLOPE:	return "cross_slope";
		case HYBRID_LOCAL_PASS_CUT_FILL:		return "cut_fill";
		case HYBRID_LOCAL_PASS_FLOOR_WIDTH:	return "floor_width";
		case HYBRID_LOCAL_PASS_FLOOR_GRADE:	return "floor_grade";
		default:										break;
	}
	return "unknown";
}


float Hybrid_Local_Pass_Sample_Spacing(const HybridConstants & constants)
{
	return Max2(Min2(constants.W / SPACING_W,constants.A / SPACING_A),
					Max2(constants.Q,1.0e-3f));
}


/***********************************************************************************************
**	Geometry.
***********************************************************************************************/

static Vector2 Quantized(const Vector2 & v)
{
	return Vector2(Hybrid_Quantize_Position(v.X),Hybrid_Quantize_Position(v.Y));
}



/*
**	The along-obstruction run a ramp needs to make its vertical in the normal span it has.
**
**		minimum_horizontal_run = vertical_run / tan(max_grade)
**		required_tangent_run   = sqrt(max(0, minimum_horizontal_run^2 - normal_run^2))
**
**	When the normal span alone is already long enough the tangent run is zero and the ramp is straight
**	down the wall, which is the easy case the old straight-line test happened to cover.
*/
static float Required_Horizontal_Run(float vertical_run,float max_grade_radians)
{
	const float tangent = (float)::tan((double)max_grade_radians);
	if (tangent <= 1.0e-6f) return 0.0f;
	return (float)fabs((double)vertical_run) / tangent;
}


static float Required_Tangent_Run(float normal_run,float vertical_run,float max_grade_radians)
{
	const float minimum = Required_Horizontal_Run(vertical_run,max_grade_radians);
	const float squared = (minimum * minimum) - (normal_run * normal_run);
	if (squared <= 0.0f) return 0.0f;
	return (float)::sqrt((double)squared);
}


/*
**	How short a station falls: `required / available`, with no room reported as far short rather than as
**	divide-by-zero.  A station at the very end of a feature genuinely has no run that way.
*/
static const float	RATIO_NO_ROOM	= 1.0e9f;

static float Run_Ratio(float required,float available)
{
	if (required <= 0.0f) return 0.0f;
	if (available <= 0.0f) return RATIO_NO_ROOM;
	return required / available;
}


//	Keep the best (smallest) worst-ramp ratio the orientations achieved.
static void Note_Ratio(HybridLocalPassMeasurement * measure,float required_a,float available_a,
							  float required_b,float available_b)
{
	if (measure == nullptr) return;
	const float ratio_a = Run_Ratio(required_a,available_a);
	const float ratio_b = Run_Ratio(required_b,available_b);
	const float worst = (ratio_a > ratio_b) ? ratio_a : ratio_b;
	if (worst < measure->RatioBest) measure->RatioBest = worst;
}


/***********************************************************************************************
**	Validation.  What is judged is the road, not the ground it stands on.
***********************************************************************************************/

/*
**	Walk a candidate centerline at `spacing` and require, at every sample:
**
**	  - the profile itself holds the class maximum grade.  The profile is the grade-limited interpolation
**	    between the control elevations, which is what Phase 12's vertical solver will realize, so the
**	    untouched wall underneath being steeper is not a refusal;
**	  - the profile can actually be realized: it lies inside `RoadVerticalInterval(ground)`, so the cut and
**	    fill it asks for are the cut and fill the profile is allowed;
**	  - the cross slope across the roadbed holds, measured on the ground the roadbed sits on;
**	  - the swept vehicle envelope fits the roadbed.
*/
static bool Profile_Is_Legal(const HybridLocalPassParams & params,const Vector2 * controls,int count,
									  const float * control_z,float spacing,HybridLocalPassHeightFunc sample,
									  void * context,float * steepest_out,int * steepest_leg_out,
									  HybridLocalPassFailure * failure_out)
{
	const HybridConstants & k = params.Constants;
	const HybridRoadClassSpec & spec = Hybrid_Road_Class_Spec(params.PassClass);
	const float max_grade = spec.MaxGradeDegrees * PI_OVER_180;
	const float max_cross = spec.MaxCrossSlopeDegrees * PI_OVER_180;
	const float half_roadbed = 0.5f * Hybrid_Roadbed_Width(params.PassClass,k.W);

	if (steepest_out != nullptr) *steepest_out = 0.0f;

	for (int leg = 0; leg < (count - 1); ++leg) {

		const Vector2 from = controls[leg];
		const Vector2 to = controls[leg + 1];
		Vector2 axis = to - from;
		const float length = axis.Length();
		if (length <= Max2(k.Q,1.0e-3f)) continue;
		axis *= (1.0f / length);
		const Vector2 normal(-axis.Y,axis.X);

		//	The leg's own grade, on the compiled profile.
		const float rise = control_z[leg + 1] - control_z[leg];
		const float grade = (float)::atan((double)((float)fabs((double)rise) / length));
		if ((steepest_out != nullptr) && (grade > *steepest_out)) {
			*steepest_out = grade;
			if (steepest_leg_out != nullptr) *steepest_leg_out = leg;
		}
		if (grade > (max_grade + HYBRID_QUANTUM_ANGLE)) {
			if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_GRADE;
			return false;
		}

		const int steps = (int)::ceil((double)(length / spacing));
		const int count_of = (steps > 0) ? steps : 1;

		for (int i = 0; i <= count_of; ++i) {

			const float ratio = (float)i / (float)count_of;
			const Vector2 point(from.X + ((to.X - from.X) * ratio),from.Y + ((to.Y - from.Y) * ratio));
			const float profile = control_z[leg] + (rise * ratio);

			float ground = 0.0f;
			if (!sample(point,&ground,context)) {
				if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_NO_HEIGHT;
				return false;
			}

			/*
			**	The realizability test.  `RoadVerticalInterval` is the whole of what the road may ask of the
			**	ground, so a profile outside it is a profile Phase 12 could not build even though its own grade
			**	is legal.  This is what keeps a diagonal ramp honest without touching the cut or fill limits.
			*/
			const HybridVerticalInterval interval = Hybrid_Road_Vertical_Interval(ground,params.MaxCutDepth,
																								 params.MaxFillHeight);
			if ((profile < (interval.Low - k.Q)) || (profile > (interval.High + k.Q))) {
				if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_CUT_FILL;
				return false;
			}

			/*
			**	Cross slope, as Section 12.2 step 5 defines it: "zero on ordinary straight roadbed", with the
			**	*compiled profile* required to stay inside the class maximum.  The roadbed is level across; what
			**	the ground under it does is the cut and fill that levels it.  So the test is realizability of a
			**	level roadbed at the profile height: both roadbed edges have to sit inside their own
			**	`RoadVerticalInterval`.  Judging the untouched ground's own slope across the road instead would
			**	refuse every diagonal on a face -- a ramp cut into a talus exists precisely where the ground is
			**	steep across it -- and that is what refused every native band station.
			**
			**	Where a level roadbed cannot be built, the class *may* bank up to its maximum cross slope: the
			**	edges then need to reach the profile only to within `half_roadbed * tan(max_cross)`, one up and
			**	one down.  Past that it is a cross-slope refusal, not a cut/fill one, because the level roadbed
			**	was realizable somewhere and the failing quantity is the tilt the ground demands.
			*/
			float left = 0.0f;
			float right = 0.0f;
			if (!sample(point + (normal * half_roadbed),&left,context) ||
				 !sample(point - (normal * half_roadbed),&right,context)) {
				if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_NO_HEIGHT;
				return false;
			}
			const HybridVerticalInterval left_interval = Hybrid_Road_Vertical_Interval(left,params.MaxCutDepth,
																											params.MaxFillHeight);
			const HybridVerticalInterval right_interval = Hybrid_Road_Vertical_Interval(right,
																											 params.MaxCutDepth,
																											 params.MaxFillHeight);
			const float bank = half_roadbed * (float)::tan((double)max_cross);
			const bool left_ok  = (profile >= (left_interval.Low - bank - k.Q)) &&
										 (profile <= (left_interval.High + bank + k.Q));
			const bool right_ok = (profile >= (right_interval.Low - bank - k.Q)) &&
										 (profile <= (right_interval.High + bank + k.Q));
			if (!left_ok || !right_ok) {
				if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_CROSS_SLOPE;
				return false;
			}
		}
	}
	return true;
}


//	The turn at a corner, in radians: zero for straight on.
static float Corner_Turn(const Vector2 & a,const Vector2 & b,const Vector2 & c)
{
	Vector2 incoming = b - a;
	Vector2 outgoing = c - b;
	const float li = incoming.Length();
	const float lo = outgoing.Length();
	if ((li <= 0.0f) || (lo <= 0.0f)) return 0.0f;
	incoming *= (1.0f / li);
	outgoing *= (1.0f / lo);
	float dot = (incoming.X * outgoing.X) + (incoming.Y * outgoing.Y);
	if (dot > 1.0f) dot = 1.0f;
	if (dot < -1.0f) dot = -1.0f;
	return (float)::acos((double)dot);
}


/*
**	Every corner of the candidate has to be one a `PassClass` vehicle can drive.
**
**	Judged the way Phase 10 builds a corner (D-009): a circular arc of the class minimum radius, which
**	consumes `R tan(theta/2)` of the straight on each side of the vertex.  A corner is legal when the two
**	legs meeting at it can each host their share of arc, and the largest radius the legs can host is what is
**	reported.  The circumradius through three controls is *not* this test -- a floor pass's legs are tens of
**	units long and its turns are tens of degrees, so a corner a vehicle drives easily has a small circumradius
**	and was being refused for it.  A corner that turns back on itself cannot be filleted at any radius.
*/
static bool Corners_Are_Legal(const HybridLocalPassParams & params,const Vector2 * controls,int count,
										float * tightest_out,HybridLocalPassFailure * failure_out)
{
	const HybridRoadClassSpec & spec = Hybrid_Road_Class_Spec(params.PassClass);
	const float min_radius = spec.MinTurnRadiusW * params.Constants.W;
	const float quantum = Max2(params.Constants.Q,1.0e-3f);
	const float reversal = 170.0f * PI_OVER_180;

	if (tightest_out != nullptr) *tightest_out = 3.4e38f;
	if (count < 3) return true;

	//	Half-tangent every corner takes at the class radius; the ends take none.
	float tangent[HYBRID_LOCAL_PASS_MAX_CONTROLS];
	for (int i = 0; i < count; ++i) {
		tangent[i] = 0.0f;
		if ((i == 0) || (i == (count - 1))) continue;
		const float turn = Corner_Turn(controls[i - 1],controls[i],controls[i + 1]);
		if (turn >= reversal) {
			if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_TURN_RADIUS;
			return false;
		}
		if (turn <= quantum) continue;
		tangent[i] = min_radius * (float)::tan((double)(0.5f * turn));
	}

	for (int leg = 0; leg < (count - 1); ++leg) {

		const float length = (controls[leg + 1] - controls[leg]).Length();
		const float needed = tangent[leg] + tangent[leg + 1];
		if (needed > (length + quantum)) {
			if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_TURN_RADIUS;
			return false;
		}

		//	The radius this leg could host, scaling both arcs together.
		if ((tightest_out != nullptr) && (needed > quantum)) {
			const float could = min_radius * (length / needed);
			if (could < *tightest_out) *tightest_out = could;
		}
	}
	return true;
}


/*
**	Section 10.3's floor invariant, measured with the swept width rather than at the centre alone: the
**	floor has to carry the vehicle across its whole width, and it must not run steeper along itself than
**	the class allows.
*/
static bool Floor_Is_Legal(const HybridLocalPassParams & params,const Vector2 & entry,const Vector2 & exit,
									float floor_half,HybridLocalPassHeightFunc sample,void * context,
									HybridLocalPassFailure * failure_out)
{
	const HybridConstants & k = params.Constants;
	const HybridRoadClassSpec & spec = Hybrid_Road_Class_Spec(params.PassClass);
	const float max_grade = spec.MaxGradeDegrees * PI_OVER_180;

	const float required = params.SweptVehicleWidth + (2.0f * params.MinVehicleLaneClearance);
	if ((2.0f * floor_half) < required) {
		if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_FLOOR_WIDTH;
		return false;
	}

	Vector2 axis = exit - entry;
	const float length = axis.Length();
	if (length <= Max2(k.Q,1.0e-3f)) return true;
	axis *= (1.0f / length);

	//	Along the floor, on the ground: a floor that climbs faster than the class may is not drivable.
	const float spacing = Hybrid_Local_Pass_Sample_Spacing(k);
	const int steps = (int)::ceil((double)(length / spacing));
	const int count = (steps > 0) ? steps : 1;

	float previous = 0.0f;
	for (int i = 0; i <= count; ++i) {

		const float ratio = (float)i / (float)count;
		const Vector2 point(entry.X + ((exit.X - entry.X) * ratio),entry.Y + ((exit.Y - entry.Y) * ratio));

		float here = 0.0f;
		if (!sample(point,&here,context)) {
			if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_NO_HEIGHT;
			return false;
		}
		if (i > 0) {
			const float run = length / (float)count;
			const float grade = (float)::atan((double)((float)fabs((double)(here - previous)) / Max2(run,k.Q)));
			if (grade > (max_grade + HYBRID_QUANTUM_ANGLE)) {
				if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_FLOOR_GRADE;
				return false;
			}
		}
		previous = here;
	}
	return true;
}


/***********************************************************************************************
**	Construction.
***********************************************************************************************/

/*
**	How many times the run a ramp needs may be recomputed from the foot it reaches.
**
**	The run and the foot depend on each other: the run is what the drop to the foot requires, and the foot
**	lands that run along the obstruction, where the floor may stand lower than it does at the station.  So the
**	run is solved rather than estimated -- each step lengthens it to what the foot it just reached asks for,
**	which is monotone and therefore terminates: either it settles, or it passes what the obstruction offers
**	and the station has no legal descent.  This is not a search: there is one candidate at every step.
*/
static const int	RAMP_RUN_REFINEMENTS	= 8;


/*
**	Solve one ramp: the along-obstruction run that lets it reach the floor inside the class grade, and the
**	foot it reaches.  `normal_side` picks which side of the floor the foot is on, `tangent_dir` which way the
**	ramp leans.
*/
static bool Solve_Ramp_Foot(float normal_run,float rim_z,float max_grade,float floor_edge,
									 const Vector2 & centre,const Vector2 & normal,const Vector2 & tangent,
									 float normal_side,float tangent_dir,float available,float quantum,
									 HybridLocalPassHeightFunc sample,void * context,float * run_out,
									 Vector2 * foot_out,float * foot_z_out,HybridLocalPassFailure * failure_out)
{
	float run = 0.0f;

	for (int refinement = 0; refinement <= RAMP_RUN_REFINEMENTS; ++refinement) {

		const Vector2 foot = Quantized(centre + (normal * (normal_side * floor_edge)) +
												 (tangent * (tangent_dir * run)));
		float foot_z = 0.0f;
		if (!sample(foot,&foot_z,context)) {
			if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_NO_HEIGHT;
			return false;
		}

		const float needed = Required_Tangent_Run(normal_run,rim_z - foot_z,max_grade);
		if (needed > available) {
			if (run_out != nullptr) *run_out = needed;
			/*
			**	Which of the two things went wrong is worth saying, because they are different defects.  On the
			**	first step the foot is at the station itself, so the run is what *this* descent costs and the
			**	obstruction is simply too short for it.  On a later step the run had already grown, which only
			**	happens when the floor falls away along the obstruction faster than the ramp gains on it -- and
			**	that is the floor's own grade, not a shortage of feature.
			*/
			if (failure_out != nullptr) {
				*failure_out = (refinement == 0) ? HYBRID_LOCAL_PASS_RUN : HYBRID_LOCAL_PASS_FLOOR_GRADE;
			}
			return false;
		}

		if (needed <= (run + quantum)) {
			if (run_out != nullptr) *run_out = run;
			if (foot_out != nullptr) *foot_out = foot;
			if (foot_z_out != nullptr) *foot_z_out = foot_z;
			return true;
		}
		run = needed;
	}

	//	The floor falls away along the obstruction faster than a ramp can gain on it.
	if (run_out != nullptr) *run_out = run;
	if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_FLOOR_GRADE;
	return false;
}

/*
**	One orientation of one candidate.  `lean` is +1 when the first ramp leans along +tangent.
*/
static bool Try_Orientation(const HybridLocalPassParams & params,HybridLocalPassKind kind,
									 const Vector2 & centre,const Vector2 & tangent,const Vector2 & normal,
									 float obstruction_half,float floor_half,float rim_half,
									 const HybridLocalPassBand * band,
									 float approach_length,float available_plus,float available_minus,float lean,
									 HybridLocalPassHeightFunc sample,void * context,
									 HybridLocalPassGeometry * out,HybridLocalPassFailure * failure_out,
									 HybridLocalPassMeasurement * measure)
{
	const HybridConstants & k = params.Constants;
	const HybridRoadClassSpec & spec = Hybrid_Road_Class_Spec(params.PassClass);
	const float max_grade = spec.MaxGradeDegrees * PI_OVER_180;
	const float min_radius = spec.MinTurnRadiusW * k.W;
	const float spacing = Hybrid_Local_Pass_Sample_Spacing(k);

	const Vector2 approach_a = Quantized(centre + (normal * (obstruction_half + approach_length)));
	const Vector2 approach_b = Quantized(centre - (normal * (obstruction_half + approach_length)));

	float z_a = 0.0f;
	float z_b = 0.0f;
	if (!sample(approach_a,&z_a,context) || !sample(approach_b,&z_b,context)) {
		if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_NO_HEIGHT;
		return false;
	}

	Vector2 controls[HYBRID_LOCAL_PASS_MAX_CONTROLS];
	float control_z[HYBRID_LOCAL_PASS_MAX_CONTROLS];
	int count = 0;

	if (kind == HYBRID_LOCAL_PASS_FLOOR) {

		/*
		**	A descent, the floor, and an ascent.  The two ramps lean *opposite* ways along the obstruction so
		**	the route comes back across the station instead of drifting away down the ravine: the descent
		**	leans `+lean`, the ascent `-lean`, and the floor segment carries the road between the two feet.
		*/
		const float floor_edge = Max2(floor_half,0.0f);
		if (obstruction_half <= floor_edge) {
			if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_NO_GEOMETRY;
			return false;
		}

		//	The normal span each ramp has: rim to floor edge.  The rim is the obstruction's own edge.
		const float rim = Min2((rim_half > 0.0f) ? rim_half : obstruction_half,obstruction_half);
		const float normal_run = Max2(rim - floor_edge,k.Q);

		/*
		**	The descent starts at the rim, not out at the approach point.
		**
		**	The approach is reserved ground outside the obstruction, where a road sits on the ground: only the
		**	wall has to be descended.  Spreading the same drop over the flat approach as well leaves the
		**	profile buried under the rim and hanging above the floor -- cut and fill in the shape of the road
		**	rather than of the ground -- and on the generated worlds that artifact alone refused most canyon
		**	and ravine stations on `RoadVerticalInterval`.  So the candidate carries the two rim points, and
		**	the graded legs are rim to floor and floor to rim.
		*/
		const Vector2 rim_a = Quantized(centre + (normal * rim));
		const Vector2 rim_b = Quantized(centre - (normal * rim));

		float floor_z = 0.0f;
		float rim_a_z = 0.0f;
		float rim_b_z = 0.0f;
		if (!sample(centre,&floor_z,context) || !sample(rim_a,&rim_a_z,context) ||
			 !sample(rim_b,&rim_b_z,context)) {
			if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_NO_HEIGHT;
			return false;
		}

		/*
		**	Each ramp buys its own vertical, so each needs only the run its own vertical asks for -- and it
		**	spends that run in its own direction.  The descent leans `+lean` and the ascent `-lean`, so the two
		**	are judged against the two directional availabilities separately: they are rarely equal, because a
		**	station sits somewhere along the feature rather than at its middle.
		*/
		const float vertical_a = rim_a_z - floor_z;
		const float vertical_b = rim_b_z - floor_z;
		const float estimate_a = Required_Tangent_Run(normal_run,vertical_a,max_grade);
		const float estimate_b = Required_Tangent_Run(normal_run,vertical_b,max_grade);
		const float available_a = (lean > 0.0f) ? available_plus : available_minus;
		const float available_b = (lean > 0.0f) ? available_minus : available_plus;

		if (measure != nullptr) {
			measure->Valid						= true;
			measure->NormalRun				= normal_run;
			measure->VerticalA				= vertical_a;
			measure->VerticalB				= vertical_b;
			measure->RequiredHorizontalA	= Required_Horizontal_Run(vertical_a,max_grade);
			measure->RequiredHorizontalB	= Required_Horizontal_Run(vertical_b,max_grade);
			measure->RequiredTangentA		= estimate_a;
			measure->RequiredTangentB		= estimate_b;
			measure->MaxGradeRadians		= max_grade;
			measure->FloorHalfWidth			= floor_edge;
			measure->RimHalfWidth			= rim;
			measure->Depth						= Max2((float)fabs((double)vertical_a),
																  (float)fabs((double)vertical_b));
			Note_Ratio(measure,estimate_a,available_a,estimate_b,available_b);
		}

		//	The descent, then the ascent: each solved against its own direction's room.
		float run_a = 0.0f;
		float run_b = 0.0f;
		Vector2 entry(0.0f,0.0f);
		Vector2 exit(0.0f,0.0f);
		float entry_z = 0.0f;
		float exit_z = 0.0f;

		const bool solved_a = Solve_Ramp_Foot(normal_run,rim_a_z,max_grade,floor_edge,centre,normal,tangent,
														  +1.0f,lean,available_a,k.Q,sample,context,&run_a,&entry,
														  &entry_z,failure_out);
		const bool solved_b = solved_a &&
									 Solve_Ramp_Foot(normal_run,rim_b_z,max_grade,floor_edge,centre,normal,tangent,
														  -1.0f,-lean,available_b,k.Q,sample,context,&run_b,&exit,
														  &exit_z,failure_out);
		if (measure != nullptr) {
			measure->RequiredTangentA = run_a;
			measure->RequiredTangentB = run_b;
			Note_Ratio(measure,run_a,available_a,run_b,available_b);
		}
		if (!solved_a || !solved_b) return false;

		if (!Floor_Is_Legal(params,entry,exit,floor_edge,sample,context,failure_out)) return false;

		controls[count]	= approach_a;	control_z[count] = z_a;			count++;
		controls[count]	= rim_a;			control_z[count] = rim_a_z;	count++;
		controls[count]	= entry;			control_z[count] = entry_z;	count++;
		controls[count]	= exit;			control_z[count] = exit_z;		count++;
		controls[count]	= rim_b;			control_z[count] = rim_b_z;	count++;
		controls[count]	= approach_b;	control_z[count] = z_b;			count++;

		//	The floor's own longitudinal grade, on the segment that was actually built.
		if (measure != nullptr) {
			const float floor_span = (exit - entry).Length();
			if (floor_span > Max2(k.Q,1.0e-3f)) {
				measure->FloorGrade = (float)::atan((double)((float)fabs((double)(exit_z - entry_z)) /
																		  floor_span));
			}
		}

		out->HasFloor		= true;
		out->FloorEntry	= entry;
		out->FloorExit		= exit;
		out->FlatHalfWidth= Hybrid_Quantize_Position(floor_edge);
		out->TangentRun	= Hybrid_Quantize_Position(Max2(run_a,run_b));

	} else {

		/*
		**	A cliff band, as D-004 realizes it: both approaches stand on the same lower bench, the band rises
		**	over the near talus face, holds full height across a shelf, and falls over the far face.  The pass
		**	climbs the near face on the diagonal, runs along the shelf, and descends the far face on the
		**	diagonal -- and both ramps lean the *same* way along the cliff.
		**
		**	Why the same way, when a floor pass leans its two ramps opposite ways: a native band is 18..36 high
		**	and its face a few lane widths wide, so at the class grade each ramp spends 80 or more units along
		**	the cliff for 5 across it -- the ramp runs nearly parallel to the cliff.  Ramps leaning opposite
		**	ways would then meet the shelf traverse at close to 180 degrees on a shelf 3 to 10 wide: a hairpin,
		**	which no class radius can drive and which the plan forbids as a switchback.  Leaning the same way,
		**	the traverse continues the ramps' direction and every corner is a few degrees -- except the two at
		**	the toes, where the road turns off its approach onto the face, and those are ordinary class-radius
		**	corners on legs long enough to host them.  The crossing therefore exits some distance along the
		**	cliff from where it entered; that distance is bounded by the cliff's own directional arc length,
		**	exactly as a floor pass's lean is, and it is the honest shape of a road over a raised band.
		**
		**	The old rule -- one grade line between the two approach elevations -- described a *step*, and on a
		**	band produced a flat profile running straight under 18..36 of terrain, refused on cut/fill at every
		**	cliff station of every world.
		*/
		if (band == nullptr) {
			if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_NO_GEOMETRY;
			return false;
		}
		if (!((band->ToeA > band->ShelfA) && (band->ShelfA > band->ShelfB) && (band->ShelfB > band->ToeB))) {
			if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_NO_GEOMETRY;
			return false;
		}

		const float shelf_half = 0.5f * (band->ShelfA - band->ShelfB);
		const float shelf_mid = 0.5f * (band->ShelfA + band->ShelfB);
		const Vector2 shelf_centre = centre + (normal * shelf_mid);

		//	Each ramp climbs from its toe on the bench to the shelf centreline: that is its normal run.
		const float normal_run_a = Max2(band->ToeA - shelf_mid,k.Q);
		const float normal_run_b = Max2(shelf_mid - band->ToeB,k.Q);

		float shelf_z = 0.0f;
		if (!sample(shelf_centre,&shelf_z,context)) {
			if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_NO_HEIGHT;
			return false;
		}

		//	The bench is read where the approaches stand, clear of the toe's own edge.
		const float vertical_a = shelf_z - z_a;
		const float vertical_b = shelf_z - z_b;
		const float estimate_a = Required_Tangent_Run(normal_run_a,vertical_a,max_grade);
		const float estimate_b = Required_Tangent_Run(normal_run_b,vertical_b,max_grade);

		/*
		**	Both ramps and the traverse spend run in the lean direction, so all of it comes out of that one
		**	directional availability.  The other direction is not used and is not charged.
		*/
		const float available = (lean > 0.0f) ? available_plus : available_minus;

		if (measure != nullptr) {
			measure->Valid						= true;
			measure->NormalRun				= Min2(normal_run_a,normal_run_b);
			measure->VerticalA				= vertical_a;
			measure->VerticalB				= vertical_b;
			measure->RequiredHorizontalA	= Required_Horizontal_Run(vertical_a,max_grade);
			measure->RequiredHorizontalB	= Required_Horizontal_Run(vertical_b,max_grade);
			measure->RequiredTangentA		= estimate_a;
			measure->RequiredTangentB		= estimate_b;
			measure->MaxGradeRadians		= max_grade;
			measure->FloorHalfWidth			= shelf_half;
			measure->RimHalfWidth			= obstruction_half;
			measure->Depth						= Max2((float)fabs((double)vertical_a),
																  (float)fabs((double)vertical_b));
			Note_Ratio(measure,estimate_a + estimate_b,available,0.0f,available);
		}

		/*
		**	The ascent: from the toe at the station, leaning `lean` along the cliff, solved against the shelf it
		**	actually reaches.  Its foot is where the shelf traverse begins.
		*/
		float run_a = 0.0f;
		Vector2 entry(0.0f,0.0f);
		float entry_z = 0.0f;
		if (!Solve_Ramp_Foot(normal_run_a,z_a,max_grade,shelf_mid,centre,normal,tangent,+1.0f,lean,available,
									k.Q,sample,context,&run_a,&entry,&entry_z,failure_out)) return false;

		/*
		**	The traverse along the shelf: as long as the two corners at its ends need for their class-radius
		**	fillets, and no longer -- both corners are the small angle between a ramp and the shelf, so this is
		**	a short leg.  It continues in the lean direction, so it comes out of the same availability.
		*/
		const Vector2 ascent = entry - Quantized(centre + (normal * band->ToeA));
		const float ascent_length = ascent.Length();
		float traverse = 0.0f;
		if (ascent_length > k.Q) {
			const Vector2 along = tangent * lean;
			const float dot = ((ascent.X * along.X) + (ascent.Y * along.Y)) / ascent_length;
			const float bend = (float)::acos((double)((dot > 1.0f) ? 1.0f : ((dot < -1.0f) ? -1.0f : dot)));
			traverse = 2.0f * min_radius * (float)::tan((double)(0.5f * bend));
		}
		traverse = Hybrid_Quantize_Position(Max2(traverse,k.Q));

		if ((run_a + traverse) > available) {
			if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_RUN;
			return false;
		}
		const Vector2 exit = Quantized(entry + (tangent * (lean * traverse)));
		float exit_z = 0.0f;
		if (!sample(exit,&exit_z,context)) {
			if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_NO_HEIGHT;
			return false;
		}

		/*
		**	The descent: from the traverse's end, leaning the same way, down the far face to the bench.  The
		**	far toe is wherever that lands along the cliff, so the far approach stands opposite it rather than
		**	opposite the station; both are frozen with the geometry.  The run is solved against the bench it
		**	reaches, with what is left of the availability after the ascent and the traverse.
		*/
		float run_b = 0.0f;
		Vector2 toe_b(0.0f,0.0f);
		float toe_b_z = 0.0f;
		{
			const Vector2 exit_on_toe_line = exit - (normal * (shelf_mid - band->ToeB));
			//	Solve from the bench up to the shelf, as the ascent did, then read it as a descent.
			if (!Solve_Ramp_Foot(normal_run_b,exit_z,max_grade,0.0f,exit_on_toe_line,normal,tangent,0.0f,lean,
										available - run_a - traverse,k.Q,sample,context,&run_b,&toe_b,&toe_b_z,
										failure_out)) return false;
		}

		//	The far approach stands off the far toe, along the normal, as far out as the near one does.
		const Vector2 far_approach = Quantized(toe_b - (normal * (obstruction_half + approach_length -
																					  (shelf_mid - band->ToeB))));
		float far_z = 0.0f;
		if (!sample(far_approach,&far_z,context)) {
			if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_NO_HEIGHT;
			return false;
		}

		//	The shelf is the flat this pass crosses, and it has to carry the envelope like a floor does.
		if (!Floor_Is_Legal(params,entry,exit,shelf_half,sample,context,failure_out)) return false;

		if (measure != nullptr) {
			measure->RequiredTangentA = run_a;
			measure->RequiredTangentB = run_b;
			Note_Ratio(measure,run_a + traverse + run_b,available,0.0f,available);
			const float shelf_span = (exit - entry).Length();
			if (shelf_span > Max2(k.Q,1.0e-3f)) {
				measure->FloorGrade = (float)::atan((double)((float)fabs((double)(exit_z - entry_z)) /
																		  shelf_span));
			}
		}

		const Vector2 toe_a = Quantized(centre + (normal * band->ToeA));
		float toe_a_z = 0.0f;
		if (!sample(toe_a,&toe_a_z,context)) {
			if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_NO_HEIGHT;
			return false;
		}

		controls[count]	= approach_a;		control_z[count] = z_a;			count++;
		controls[count]	= toe_a;				control_z[count] = toe_a_z;	count++;
		controls[count]	= entry;				control_z[count] = entry_z;	count++;
		controls[count]	= exit;				control_z[count] = exit_z;		count++;
		controls[count]	= toe_b;				control_z[count] = toe_b_z;	count++;
		controls[count]	= far_approach;	control_z[count] = far_z;		count++;

		out->HasFloor		= true;
		out->FloorEntry	= entry;
		out->FloorExit		= exit;
		out->FlatHalfWidth= Hybrid_Quantize_Position(shelf_half);
		out->TangentRun	= Hybrid_Quantize_Position(run_a + traverse + run_b);
		(void)rim_half;
		(void)floor_half;
		(void)approach_b;
		(void)estimate_b;
	}

	//	Judged as road: its corners, then its profile.
	float tightest = 0.0f;
	if (!Corners_Are_Legal(params,controls,count,&tightest,failure_out)) return false;

	float steepest = 0.0f;
	int steepest_leg = -1;
	const bool legal = Profile_Is_Legal(params,controls,count,control_z,spacing,sample,context,&steepest,
													&steepest_leg,failure_out);
	if (measure != nullptr) {
		measure->ProfileGrade	= steepest;
		measure->ProfileLeg		= steepest_leg;
	}
	if (!legal) return false;

	out->Kind				= kind;
	out->ControlCount		= count;
	out->SteepestGrade	= steepest;
	out->TightestRadius	= tightest;
	out->ProvedSweptWidth= params.SweptVehicleWidth;

	float length = 0.0f;
	for (int i = 0; i < count; ++i) {
		out->Controls[i] = controls[i];
		if (i > 0) length += (controls[i] - controls[i - 1]).Length();
	}
	out->Length = Hybrid_Quantize_Position(length);
	return true;
}


bool Hybrid_Build_Local_Pass(const HybridLocalPassParams & params,HybridLocalPassKind kind,
									  const Vector2 & centre,const Vector2 & tangent,const Vector2 & normal,
									  float obstruction_half,float floor_half,float rim_half,
									  const HybridLocalPassBand * band,
									  float approach_length,float available_run_plus,float available_run_minus,
									  HybridLocalPassHeightFunc sample,void * context,
									  HybridLocalPassGeometry * out,HybridLocalPassFailure * failure_out,
									  HybridLocalPassMeasurement * measure_out)
{
	if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_OK;
	if (measure_out != nullptr) {
		*measure_out = HybridLocalPassMeasurement();
		measure_out->AvailablePlus	= available_run_plus;
		measure_out->AvailableMinus= available_run_minus;
		measure_out->RatioBest		= RATIO_NO_ROOM;
	}
	if ((out == nullptr) || (sample == nullptr)) return false;
	*out = HybridLocalPassGeometry();

	if ((tangent.Length() <= 0.0f) || (normal.Length() <= 0.0f) || !(obstruction_half > 0.0f) ||
		 !(params.Constants.W > 0.0f)) {
		if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_NO_GEOMETRY;
		return false;
	}

	//	`+tangent` first, then `-tangent`.  The first fully legal candidate wins; nothing is drawn.
	HybridLocalPassFailure worst = HYBRID_LOCAL_PASS_NO_GEOMETRY;
	for (int orientation = 0; orientation < 2; ++orientation) {

		const float lean = (orientation == 0) ? 1.0f : -1.0f;
		HybridLocalPassGeometry candidate;
		HybridLocalPassFailure failure = HYBRID_LOCAL_PASS_OK;

		const bool built = Try_Orientation(params,kind,centre,tangent,normal,obstruction_half,floor_half,
													  rim_half,band,approach_length,available_run_plus,available_run_minus,
													  lean,sample,context,&candidate,&failure,measure_out);
		if (measure_out != nullptr) measure_out->Failure[orientation] = failure;
		if (built) {
			candidate.Orientation = orientation;
			*out = candidate;
			return true;
		}
		if (orientation == 0) worst = failure;
	}

	if (failure_out != nullptr) *failure_out = worst;
	return false;
}


bool Hybrid_Validate_Local_Pass(const HybridLocalPassParams & params,
										  const HybridLocalPassGeometry & geometry,float sample_spacing,
										  HybridLocalPassHeightFunc sample,void * context,
										  HybridLocalPassFailure * failure_out)
{
	if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_OK;
	if ((sample == nullptr) || (geometry.ControlCount < 2)) {
		if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_NO_GEOMETRY;
		return false;
	}

	//	The stored control elevations are not carried: they are re-read, so a changed height field is seen.
	float control_z[HYBRID_LOCAL_PASS_MAX_CONTROLS];
	for (int i = 0; i < geometry.ControlCount; ++i) {
		if (sample(geometry.Controls[i],&control_z[i],context)) continue;
		if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_NO_HEIGHT;
		return false;
	}

	float tightest = 0.0f;
	if (!Corners_Are_Legal(params,geometry.Controls,geometry.ControlCount,&tightest,failure_out)) {
		return false;
	}

	//	The flat's width *across* the obstruction, as stored: the distance between its two ends is its length.
	if (geometry.HasFloor) {
		if (!Floor_Is_Legal(params,geometry.FloorEntry,geometry.FloorExit,geometry.FlatHalfWidth,sample,
								  context,failure_out)) return false;
	}

	const float spacing = (sample_spacing > 0.0f) ? sample_spacing
															 : Hybrid_Local_Pass_Sample_Spacing(params.Constants);
	float steepest = 0.0f;
	int steepest_leg = -1;
	return Profile_Is_Legal(params,geometry.Controls,geometry.ControlCount,control_z,spacing,sample,context,
									&steepest,&steepest_leg,failure_out);
}
