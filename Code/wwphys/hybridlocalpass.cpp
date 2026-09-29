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

/*
**	The least along-obstruction run any station is allowed to spend, in lane widths, whatever the caller
**	says is available.  A ramp needs somewhere to start, and a feature shorter than this is not one a road
**	can be fitted to at all.
*/
static const float	TANGENT_RUN_FLOOR_W	= 4.0f;


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
static float Required_Tangent_Run(float normal_run,float vertical_run,float max_grade_radians)
{
	const float tangent = (float)::tan((double)max_grade_radians);
	if (tangent <= 1.0e-6f) return 0.0f;

	const float minimum = (float)fabs((double)vertical_run) / tangent;
	const float squared = (minimum * minimum) - (normal_run * normal_run);
	if (squared <= 0.0f) return 0.0f;
	return (float)::sqrt((double)squared);
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
									  void * context,float * steepest_out,HybridLocalPassFailure * failure_out)
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
		if (grade > (max_grade + HYBRID_QUANTUM_ANGLE)) {
			if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_GRADE;
			return false;
		}
		if ((steepest_out != nullptr) && (grade > *steepest_out)) *steepest_out = grade;

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
**	One orientation of one candidate.  `lean` is +1 when the first ramp leans along +tangent.
*/
static bool Try_Orientation(const HybridLocalPassParams & params,HybridLocalPassKind kind,
									 const Vector2 & centre,const Vector2 & tangent,const Vector2 & normal,
									 float obstruction_half,float floor_half,float rim_half,
									 float approach_length,float available_run,float lean,
									 HybridLocalPassHeightFunc sample,void * context,
									 HybridLocalPassGeometry * out,HybridLocalPassFailure * failure_out)
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

		//	The normal span each ramp has: rim to floor edge.
		const float rim = (rim_half > 0.0f) ? rim_half : obstruction_half;
		const float normal_run = Max2(rim - floor_edge,k.Q);

		float floor_z = 0.0f;
		if (!sample(centre,&floor_z,context)) {
			if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_NO_HEIGHT;
			return false;
		}

		const float run_a = Required_Tangent_Run(normal_run,z_a - floor_z,max_grade);
		const float run_b = Required_Tangent_Run(normal_run,z_b - floor_z,max_grade);
		const float run = Max2(run_a,run_b);
		if (run > available_run) {
			if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_GRADE;
			return false;
		}

		const Vector2 entry = Quantized(centre + (normal * floor_edge) + (tangent * (lean * run)));
		const Vector2 exit = Quantized(centre - (normal * floor_edge) - (tangent * (lean * run)));

		if (!Floor_Is_Legal(params,entry,exit,floor_edge,sample,context,failure_out)) return false;

		float entry_z = 0.0f;
		float exit_z = 0.0f;
		if (!sample(entry,&entry_z,context) || !sample(exit,&exit_z,context)) {
			if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_NO_HEIGHT;
			return false;
		}

		controls[count]	= approach_a;	control_z[count] = z_a;		count++;
		controls[count]	= entry;			control_z[count] = entry_z;	count++;
		controls[count]	= exit;			control_z[count] = exit_z;	count++;
		controls[count]	= approach_b;	control_z[count] = z_b;		count++;

		out->HasFloor		= true;
		out->FloorEntry	= entry;
		out->FloorExit		= exit;
		out->TangentRun	= Hybrid_Quantize_Position(run);

	} else {

		/*
		**	A cliff band: one diagonal ramp from the lower approach to the upper one, leaning along the
		**	obstruction for as much run as the grade needs.  The crossing point moves along the tangent, which
		**	is what turns an unclimbable face into a climbable ramp.
		*/
		const float normal_run = 2.0f * Max2(obstruction_half,k.Q);
		const float run = Required_Tangent_Run(normal_run,z_b - z_a,max_grade);
		if (run > available_run) {
			if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_GRADE;
			return false;
		}

		const Vector2 middle = Quantized(centre + (tangent * (lean * run * 0.5f)));
		float middle_z = 0.0f;
		if (!sample(middle,&middle_z,context)) {
			if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_NO_HEIGHT;
			return false;
		}

		/*
		**	The profile through the band is the grade-limited line between the two approach elevations, not
		**	the ground's own: the ramp is what the road will be cut and filled to.
		*/
		const Vector2 ramp_a = Quantized(centre + (normal * obstruction_half) + (tangent * (lean * run)));
		const Vector2 ramp_b = Quantized(centre - (normal * obstruction_half));

		float ramp_a_z = 0.0f;
		float ramp_b_z = 0.0f;
		if (!sample(ramp_a,&ramp_a_z,context) || !sample(ramp_b,&ramp_b_z,context)) {
			if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_NO_HEIGHT;
			return false;
		}

		controls[count]	= approach_a;	control_z[count] = z_a;			count++;
		controls[count]	= ramp_a;		control_z[count] = ramp_a_z;	count++;
		controls[count]	= ramp_b;		control_z[count] = ramp_b_z;	count++;
		controls[count]	= approach_b;	control_z[count] = z_b;			count++;

		out->HasFloor		= false;
		out->TangentRun	= Hybrid_Quantize_Position(run);
		(void)middle;
		(void)middle_z;
		(void)rim_half;
		(void)floor_half;
	}

	//	Judged as road: its corners, then its profile.
	float tightest = 0.0f;
	if (!Corners_Are_Legal(params,controls,count,&tightest,failure_out)) return false;

	float steepest = 0.0f;
	if (!Profile_Is_Legal(params,controls,count,control_z,spacing,sample,context,&steepest,failure_out)) {
		return false;
	}

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
									  float approach_length,float available_tangent_run,
									  HybridLocalPassHeightFunc sample,void * context,
									  HybridLocalPassGeometry * out,HybridLocalPassFailure * failure_out)
{
	if (failure_out != nullptr) *failure_out = HYBRID_LOCAL_PASS_OK;
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

		const float available = Max2(available_tangent_run,TANGENT_RUN_FLOOR_W * params.Constants.W);
		if (Try_Orientation(params,kind,centre,tangent,normal,obstruction_half,floor_half,rim_half,
								  approach_length,available,lean,sample,context,&candidate,&failure)) {
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
	return Profile_Is_Legal(params,geometry.Controls,geometry.ControlCount,control_z,spacing,sample,context,
									&steepest,failure_out);
}
