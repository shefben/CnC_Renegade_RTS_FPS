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
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commandercamera.cpp                   $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "commandercamera.h"

#include "matrix3d.h"
#include "timemgr.h"
#include "wwdebug.h"
#include "wwmath.h"


////////////////////////////////////////////////////////////////
//	Static member data
////////////////////////////////////////////////////////////////
CommanderCamera *		CommanderCamera::_TheInstance		= NULL;


////////////////////////////////////////////////////////////////
//
//	CommanderCamera
//
////////////////////////////////////////////////////////////////
CommanderCamera::CommanderCamera (void)
	:	ViewMode (VIEW_MODE_FIRST_PERSON),
		LookAtPosition (0.0f, 0.0f, 0.0f),
		CurrentHeight (CommanderCameraDefaults::START_HEIGHT),
		TargetHeight (CommanderCameraDefaults::START_HEIGHT),
		OverheadHeading (0.0f),
		OverheadPitch (CommanderCameraDefaults::PITCH),
		MinHeight (CommanderCameraDefaults::MIN_HEIGHT),
		MaxHeight (CommanderCameraDefaults::MAX_HEIGHT),
		PanSpeed (CommanderCameraDefaults::PAN_SPEED),
		ZoomSpeed (CommanderCameraDefaults::ZOOM_SPEED),
		ZoomSmoothRate (CommanderCameraDefaults::ZOOM_SMOOTH_RATE),
		TransitionTime (CommanderCameraDefaults::TRANSITION_TIME),
		PanSmoothRate (CommanderCameraDefaults::PAN_SMOOTH_RATE),
		PanDecayRate (CommanderCameraDefaults::PAN_DECAY_RATE),
		FocusSmoothRate (CommanderCameraDefaults::FOCUS_SMOOTH_RATE),
		PanVelocity (0.0f, 0.0f, 0.0f),
		PanRequestRight (0.0f),
		PanRequestForward (0.0f),
		PanRequested (false),
		FocusTarget (0.0f, 0.0f, 0.0f),
		FocusActive (false),
		EasedTransition (true)
{
	//
	//	Last one created wins.  In practice exactly one is ever created (the main
	//	combat camera); the level editor creates none at all.
	//
	_TheInstance = this;
	return ;
}


////////////////////////////////////////////////////////////////
//
//	~CommanderCamera
//
////////////////////////////////////////////////////////////////
CommanderCamera::~CommanderCamera (void)
{
	if (_TheInstance == this) {
		_TheInstance = NULL;
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Reset_Tunables
//
////////////////////////////////////////////////////////////////
void
CommanderCamera::Reset_Tunables (void)
{
	MinHeight		= CommanderCameraDefaults::MIN_HEIGHT;
	MaxHeight		= CommanderCameraDefaults::MAX_HEIGHT;
	PanSpeed			= CommanderCameraDefaults::PAN_SPEED;
	ZoomSpeed		= CommanderCameraDefaults::ZOOM_SPEED;
	ZoomSmoothRate		= CommanderCameraDefaults::ZOOM_SMOOTH_RATE;
	TransitionTime		= CommanderCameraDefaults::TRANSITION_TIME;
	OverheadPitch		= CommanderCameraDefaults::PITCH;
	PanSmoothRate		= CommanderCameraDefaults::PAN_SMOOTH_RATE;
	PanDecayRate		= CommanderCameraDefaults::PAN_DECAY_RATE;
	FocusSmoothRate	= CommanderCameraDefaults::FOCUS_SMOOTH_RATE;
	EasedTransition	= true;

	Set_Height (CurrentHeight);
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Smooth_Step
//
//	The classic 3t^2 - 2t^3.  Zero derivative at both ends, which is
//	exactly what a view transition needs:  the eye travels tens of
//	metres, so a linear blend visibly jerks at both ends.
//
////////////////////////////////////////////////////////////////
float
CommanderCamera::Smooth_Step (float t)
{
	t = WWMath::Clamp (t, 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}


////////////////////////////////////////////////////////////////
//
//	Set_Min_Height
//
////////////////////////////////////////////////////////////////
void
CommanderCamera::Set_Min_Height (float height)
{
	MinHeight = height;
	if (MaxHeight < MinHeight) {
		MaxHeight = MinHeight;
	}

	Set_Height (TargetHeight);
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Set_Max_Height
//
////////////////////////////////////////////////////////////////
void
CommanderCamera::Set_Max_Height (float height)
{
	MaxHeight = height;
	if (MaxHeight < MinHeight) {
		MinHeight = MaxHeight;
	}

	Set_Height (TargetHeight);
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Set_Overhead_Pitch
//
////////////////////////////////////////////////////////////////
void
CommanderCamera::Set_Overhead_Pitch (float radians)
{
	//
	//	Keep the pitch strictly between the horizon and straight down, otherwise
	//	the eye offset degenerates.
	//
	const float MIN_PITCH = 0.17453293f;	//	10 degrees
	const float MAX_PITCH = 1.57079633f;	//	90 degrees

	OverheadPitch = WWMath::Clamp (radians, MIN_PITCH, MAX_PITCH);
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Set_Overhead_Heading
//
////////////////////////////////////////////////////////////////
void
CommanderCamera::Set_Overhead_Heading (float heading)
{
	OverheadHeading = WWMath::Wrap (heading, -WWMATH_PI, WWMATH_PI);
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Set_Height
//
////////////////////////////////////////////////////////////////
void
CommanderCamera::Set_Height (float height)
{
	TargetHeight	= WWMath::Clamp (height, MinHeight, MaxHeight);
	CurrentHeight	= WWMath::Clamp (CurrentHeight, MinHeight, MaxHeight);
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Set_Look_At_Position
//
////////////////////////////////////////////////////////////////
void
CommanderCamera::Set_Look_At_Position (const Vector3 &position)
{
	//
	//	An explicit set is a snap, so it also abandons any eased focus and any
	//	residual pan velocity -- otherwise the camera would drift away from the
	//	position the caller just insisted on.
	//
	LookAtPosition	= position;
	FocusActive		= false;
	PanVelocity.Set (0.0f, 0.0f, 0.0f);
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Focus_On
//
////////////////////////////////////////////////////////////////
void
CommanderCamera::Focus_On (const Vector3 &position)
{
	FocusTarget	= position;
	FocusActive	= true;
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Cancel_Focus
//
////////////////////////////////////////////////////////////////
void
CommanderCamera::Cancel_Focus (void)
{
	FocusActive = false;
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Enter_Overhead_View
//
////////////////////////////////////////////////////////////////
void
CommanderCamera::Enter_Overhead_View (const Vector3 &look_at_position)
{
	if (ViewMode == VIEW_MODE_OVERHEAD) {
		return ;
	}

	ViewMode			= VIEW_MODE_OVERHEAD;
	LookAtPosition	= look_at_position;

	//
	//	Enter at rest.  Inheriting a pan velocity or a focus from the last time
	//	the view was open would make the world appear to drift the moment it
	//	opens, which reads as the camera being out of control.
	//
	PanVelocity.Set (0.0f, 0.0f, 0.0f);
	PanRequested	= false;
	FocusActive		= false;

	//
	//	Come in facing the same way the player was facing, so the world does not
	//	appear to spin under them as the view rises.
	//
	Set_Overhead_Heading (Get_Heading ());

	CurrentHeight	= WWMath::Clamp (CurrentHeight, MinHeight, MaxHeight);
	TargetHeight	= CurrentHeight;

	//
	//	Reuse the inherited transition machinery rather than rolling our own.
	//
	Set_Lerp_Time (TransitionTime);
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Enter_First_Person_View
//
////////////////////////////////////////////////////////////////
void
CommanderCamera::Enter_First_Person_View (void)
{
	if (ViewMode == VIEW_MODE_FIRST_PERSON) {
		return ;
	}

	ViewMode = VIEW_MODE_FIRST_PERSON;
	Set_Lerp_Time (TransitionTime);
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Pan
//
//	'right_amount' and 'forward_amount' are unitless input amounts in
//	[-1,1].  Panning is faster the further out the camera is zoomed,
//	which is what every RTS does and what feels correct.
//
//	This records the request and applies the easing itself, then leaves
//	PanRequested set so that Update knows a request arrived this frame
//	and does not additionally apply the coast.  'frame_seconds' is the
//	caller's frame time and is used for the ramp, so the feel does not
//	change with frame rate.
//
////////////////////////////////////////////////////////////////
void
CommanderCamera::Pan (float right_amount, float forward_amount, float frame_seconds)
{
	if (frame_seconds <= 0.0f) {
		return ;
	}

	PanRequestRight	= right_amount;
	PanRequestForward	= forward_amount;
	PanRequested		= true;

	//
	//	Any pan input means the player has taken the controls, so an eased
	//	Focus_On in flight is abandoned rather than fought.
	//
	if (right_amount != 0.0f || forward_amount != 0.0f) {
		FocusActive = false;
	}

	Update_Pan (frame_seconds);
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Update_Pan
//
//	Eases the pan velocity toward what the keys are asking for -- or
//	toward zero when nothing is -- and integrates it into the look-at
//	point.  Ramping up and coasting down use different rates:  see
//	CommanderCameraDefaults.
//
////////////////////////////////////////////////////////////////
void
CommanderCamera::Update_Pan (float frame_seconds)
{
	if (frame_seconds <= 0.0f) {
		return ;
	}

	//
	//	Screen "forward" is the direction the camera is facing projected onto the
	//	ground plane; screen "right" is 90 degrees clockwise from it.
	//
	const float cos_heading = WWMath::Cos (OverheadHeading);
	const float sin_heading = WWMath::Sin (OverheadHeading);

	float height_scale = 1.0f;
	if (MinHeight > 0.0f) {
		height_scale = CurrentHeight / MinHeight;
	}

	const float speed = PanSpeed * height_scale;

	Vector3 target (0.0f, 0.0f, 0.0f);
	float		rate = PanDecayRate;

	if (PanRequested && (PanRequestRight != 0.0f || PanRequestForward != 0.0f)) {

		target.X	= (cos_heading * PanRequestForward + sin_heading * PanRequestRight) * speed;
		target.Y	= (sin_heading * PanRequestForward - cos_heading * PanRequestRight) * speed;
		rate		= PanSmoothRate;
	}

	//
	//	Nothing asked for movement and we are already still:  do not touch the
	//	look-at point at all, so a parked camera is bit-for-bit stationary.
	//
	if (target.X == 0.0f && target.Y == 0.0f
		&& PanVelocity.X == 0.0f && PanVelocity.Y == 0.0f) {
		return ;
	}

	float blend = 1.0f;
	if (rate > 0.0f) {
		blend = WWMath::Clamp (frame_seconds * rate, 0.0f, 1.0f);
	}

	PanVelocity.X += (target.X - PanVelocity.X) * blend;
	PanVelocity.Y += (target.Y - PanVelocity.Y) * blend;
	PanVelocity.Z = 0.0f;

	LookAtPosition.X += PanVelocity.X * frame_seconds;
	LookAtPosition.Y += PanVelocity.Y * frame_seconds;
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Update_Focus
//
//	Eases the look-at point toward a Focus_On target and stops once it
//	is close enough, so it does not creep for the rest of the round.
//
////////////////////////////////////////////////////////////////
void
CommanderCamera::Update_Focus (float frame_seconds)
{
	if (FocusActive == false || frame_seconds <= 0.0f) {
		return ;
	}

	float blend = 1.0f;
	if (FocusSmoothRate > 0.0f) {
		blend = WWMath::Clamp (frame_seconds * FocusSmoothRate, 0.0f, 1.0f);
	}

	LookAtPosition.X += (FocusTarget.X - LookAtPosition.X) * blend;
	LookAtPosition.Y += (FocusTarget.Y - LookAtPosition.Y) * blend;
	LookAtPosition.Z += (FocusTarget.Z - LookAtPosition.Z) * blend;

	Vector3 remaining = FocusTarget - LookAtPosition;

	if (remaining.Length () <= CommanderCameraDefaults::FOCUS_ARRIVE_DISTANCE) {
		LookAtPosition	= FocusTarget;
		FocusActive		= false;
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Zoom
//
//	Positive 'amount' moves the camera closer to the ground.  Only the
//	*target* height moves here; Update_Height eases the actual height
//	toward it so a wheel flick never snaps the view.
//
////////////////////////////////////////////////////////////////
void
CommanderCamera::Zoom (float amount)
{
	if (amount == 0.0f) {
		return ;
	}

	TargetHeight = WWMath::Clamp (TargetHeight - (amount * ZoomSpeed), MinHeight, MaxHeight);
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Update_Height
//
////////////////////////////////////////////////////////////////
void
CommanderCamera::Update_Height (float frame_seconds)
{
	float blend = 1.0f;
	if (frame_seconds > 0.0f && ZoomSmoothRate > 0.0f) {
		blend = WWMath::Clamp (frame_seconds * ZoomSmoothRate, 0.0f, 1.0f);
	}

	CurrentHeight += (TargetHeight - CurrentHeight) * blend;
	CurrentHeight = WWMath::Clamp (CurrentHeight, MinHeight, MaxHeight);
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Get_Overhead_Eye_Position
//
////////////////////////////////////////////////////////////////
Vector3
CommanderCamera::Get_Overhead_Eye_Position (void) const
{
	//
	//	The eye sits CurrentHeight above the look-at point and pulled back along
	//	-heading by however far the pitch demands.
	//
	const float tan_pitch	= WWMath::Tan (OverheadPitch);
	float horizontal			= 0.0f;
	if (tan_pitch > WWMATH_EPSILON) {
		horizontal = CurrentHeight / tan_pitch;
	}

	Vector3 eye = LookAtPosition;
	eye.X -= WWMath::Cos (OverheadHeading) * horizontal;
	eye.Y -= WWMath::Sin (OverheadHeading) * horizontal;
	eye.Z += CurrentHeight;

	return eye;
}


////////////////////////////////////////////////////////////////
//
//	Apply_Overhead_Transform
//
////////////////////////////////////////////////////////////////
void
CommanderCamera::Apply_Overhead_Transform (const Vector3 &eye, const Vector3 &target)
{
	Matrix3D tm (1);
	tm.Look_At (eye, target, 0.0f);

	Set_Clip_Planes (NearClipPlane, FarClipPlane);
	Set_Transform (tm);
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Update
//
////////////////////////////////////////////////////////////////
void
CommanderCamera::Update (void)
{
	//
	//	Read the transition fraction *before* the base class consumes it.  1 means
	//	"entirely at the previous view", 0 means "entirely at the new one".
	//
	const bool	transitioning	= (LerpTimeTotal > 0.0f);
	float			lerp				= 0.0f;
	if (transitioning) {
		lerp = WWMath::Clamp (LerpTimeRemaining / LerpTimeTotal, 0.0f, 1.0f);
	}

	//
	//	The overwhelmingly common case: nothing commander related is happening, so
	//	the first-person camera runs completely unmodified.
	//
	if (ViewMode == VIEW_MODE_FIRST_PERSON && transitioning == false) {

		//
		//	The pan request cannot survive into a frame the overhead view is not
		//	running, or re-entering the view would resume a motion the player
		//	asked for minutes ago.
		//
		PanRequested	= false;
		PanVelocity.Set (0.0f, 0.0f, 0.0f);

		CCameraClass::Update ();
		return ;
	}

	const float frame_seconds = TimeManager::Get_Frame_Seconds ();
	Update_Height (frame_seconds);

	//
	//	If nothing called Pan this frame the keys are released, so coast to a
	//	stop.  Pan already integrated its own frame, hence the flag.
	//
	if (PanRequested == false) {
		Update_Pan (frame_seconds);
	}

	PanRequested		= false;
	PanRequestRight	= 0.0f;
	PanRequestForward	= 0.0f;

	Update_Focus (frame_seconds);

	const Vector3 overhead_eye		= Get_Overhead_Eye_Position ();
	const Vector3 overhead_target	= LookAtPosition;

	if (transitioning == false) {

		//
		//	Settled in the overhead view.  We deliberately do not run the base
		//	update here -- its profile lag and collision sweep are first-person
		//	behaviours and would fight the strategic framing.
		//
		Apply_Overhead_Transform (overhead_eye, overhead_target);
		return ;
	}

	//
	//	Mid transition.  Let the base class produce the first-person transform (it
	//	also ticks the lerp timers down for us) and blend the two views by their
	//	eye position and their look-at target, which keeps the move smooth in both
	//	position and orientation.
	//
	CCameraClass::Update ();

	const Matrix3D	fps_tm		= Get_Transform ();
	const Vector3	fps_eye		= fps_tm.Get_Translation ();
	const Vector3	fps_target	= fps_eye - (fps_tm.Get_Z_Vector () * CommanderCameraDefaults::BLEND_LOOK_AHEAD);

	//
	//	When entering the overhead view the overhead weight rises from 0 to 1;
	//	when leaving it, it falls from 1 to 0.
	//
	float overhead_weight = (ViewMode == VIEW_MODE_OVERHEAD) ? (1.0f - lerp) : lerp;

	//
	//	Ease it.  The eye travels tens of metres during this blend, so a linear
	//	weight starts and stops with a jerk that reads as a bug.
	//
	if (EasedTransition) {
		overhead_weight = Smooth_Step (overhead_weight);
	}

	const Vector3 eye		= ::Lerp (fps_eye, overhead_eye, overhead_weight);
	const Vector3 target	= ::Lerp (fps_target, overhead_target, overhead_weight);

	Apply_Overhead_Transform (eye, target);
	return ;
}
