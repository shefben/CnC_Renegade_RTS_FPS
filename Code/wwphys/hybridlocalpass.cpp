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


//	The circumradius of a corner; a straight run reports a huge radius.
static float Corner_Radius(const Vector2 & a,const Vector2 & b,const Vector2 & c)
{
	const float ab = (b - a).Length();
	const float bc = (c - b).Length();
	const float ca = (a - c).Length();
	const float cross = ((b.X - a.X) * (c.Y - a.Y)) - ((b.Y - a.Y) * (c.X - a.X));
	const float area = 0.5f * (float)fabs((double)cross);
	if (area <= 1.0e-4f) return 3.4e38f;
	return (ab * bc * ca) / (4.0f * area);
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

			//	The cross slope across the roadbed, on the ground the roadbed sits on.
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
			if (!Hybrid_Grade_Compatible(left_interval,right_interval,2.0f * half_roadbed,max_cross)) {
				if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_CROSS_SLOPE;
				return false;
			}
		}
	}
	return true;
}


//	Every corner of the candidate has to be one a `PassClass` vehicle can drive.
static bool Corners_Are_Legal(const HybridLocalPassParams & params,const Vector2 * controls,int count,
										float * tightest_out,HybridLocalPassFailure * failure_out)
{
	const HybridRoadClassSpec & spec = Hybrid_Road_Class_Spec(params.PassClass);
	const float min_radius = spec.MinTurnRadiusW * params.Constants.W;

	if (tightest_out != nullptr) *tightest_out = 3.4e38f;

	for (int i = 1; i < (count - 1); ++i) {

		const float radius = Corner_Radius(controls[i - 1],controls[i],controls[i + 1]);
		if ((tightest_out != nullptr) && (radius < *tightest_out)) *tightest_out = radius;
		if (radius >= (min_radius - params.Constants.Q)) continue;
		if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_TURN_RADIUS;
		return false;
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
		out->TangentRun	= Hybrid_Quantize_Position(Max2(run_a,run_b));

	} else {

		/*
		**	A cliff band: one diagonal ramp from the lower approach to the upper one, leaning along the
		**	obstruction for as much run as the grade needs.  The crossing point moves along the tangent, which
		**	is what turns an unclimbable face into a climbable ramp.
		*/
		const float normal_run = 2.0f * Max2(obstruction_half,k.Q);
		const float vertical = z_b - z_a;
		const float run = Required_Tangent_Run(normal_run,vertical,max_grade);

		//	One ramp, so one direction: the one it leans along.
		const float available = (lean > 0.0f) ? available_plus : available_minus;

		if (measure != nullptr) {
			measure->Valid						= true;
			measure->NormalRun				= normal_run;
			measure->VerticalA				= vertical;
			measure->RequiredHorizontalA	= Required_Horizontal_Run(vertical,max_grade);
			measure->RequiredTangentA		= run;
			measure->MaxGradeRadians		= max_grade;
			measure->RimHalfWidth			= obstruction_half;
			measure->Depth						= (float)fabs((double)vertical);
			Note_Ratio(measure,run,available,0.0f,available);
		}

		if (run > available) {
			if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_RUN;
			return false;
		}

		const Vector2 ramp_a = Quantized(centre + (normal * obstruction_half) + (tangent * (lean * run)));
		const Vector2 ramp_b = Quantized(centre - (normal * obstruction_half));

		controls[count]	= approach_a;	count++;
		controls[count]	= ramp_a;		count++;
		controls[count]	= ramp_b;		count++;
		controls[count]	= approach_b;	count++;

		/*
		**	The ramp's elevations are the grade-limited line between the two approach elevations, spread over
		**	the plan distance the ramp actually travels -- *not* the ground at the ramp feet.
		**
		**	That distinction is the whole point of a surface ramp: the road is cut and filled to a grade the
		**	untouched face does not have, so sampling the face here would judge the cliff instead of the road.
		**	Measured on the generated worlds, sampling it was what refused every cliff station: a band whose
		**	benches both stand at 0 and whose face rises 31.7 between them produced a "profile" of 0, 31.7, 0,
		**	0 and a 47-degree leg, when the road across it is a straight, level line.  Whether that line can be
		**	built out of this ground is the `RoadVerticalInterval` test below, which is where a band needing
		**	more cut than the class allows is refused -- and refused as cut, so a ramp never silently becomes
		**	a road cut.
		*/
		float travelled[HYBRID_LOCAL_PASS_MAX_CONTROLS];
		travelled[0] = 0.0f;
		for (int i = 1; i < count; ++i) {
			travelled[i] = travelled[i - 1] + (controls[i] - controls[i - 1]).Length();
		}
		const float total = travelled[count - 1];
		for (int i = 0; i < count; ++i) {
			const float ratio = (total > Max2(k.Q,1.0e-3f)) ? (travelled[i] / total) : 0.0f;
			control_z[i] = z_a + (vertical * ratio);
		}

		out->HasFloor		= false;
		out->TangentRun	= Hybrid_Quantize_Position(run);
		(void)rim_half;
		(void)floor_half;
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

	out->Kind			= kind;
	out->ControlCount	= count;
	out->SteepestGrade= steepest;
	out->TightestRadius= tightest;

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
													  rim_half,approach_length,available_run_plus,available_run_minus,
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

	if (geometry.HasFloor) {
		const float floor_half = 0.5f * (geometry.FloorExit - geometry.FloorEntry).Length();
		if (!Floor_Is_Legal(params,geometry.FloorEntry,geometry.FloorExit,floor_half,sample,context,
								  failure_out)) return false;
	}

	const float spacing = (sample_spacing > 0.0f) ? sample_spacing
															 : Hybrid_Local_Pass_Sample_Spacing(params.Constants);
	float steepest = 0.0f;
	int steepest_leg = -1;
	return Profile_Is_Legal(params,geometry.Controls,geometry.ControlCount,control_z,spacing,sample,context,
									&steepest,&steepest_leg,failure_out);
}
