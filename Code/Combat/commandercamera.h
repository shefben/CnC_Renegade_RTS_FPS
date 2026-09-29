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
 *                     $Archive:: /Commando/Code/Combat/commandercamera.h                     $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	Strategic (overhead) camera for the commander view.  This *extends* CCameraClass rather      *
 *	than forking it -- when the overhead view is inactive every frame falls straight through     *
 *	to CCameraClass::Update() and the first-person camera behaves exactly as it always did.      *
 *                                                                                             *
 *	The transition between the two views reuses the inherited lerp machinery                    *
 *	(Set_Lerp_Time / Is_Lerping); there is no bespoke interpolation state in this class.         *
 *                                                                                             *
 *	This header lives in Combat and therefore may not reference any Code/Commando symbol --      *
 *	the same COMBAT_SRC list is compiled a second time into the 'combate' target for the         *
 *	level editor, which never links 'commando'.                                                 *
 *                                                                                             *
 *	Everything in this class is *client side presentation only*.  It never mutates game state    *
 *	and never requires server authority.                                                        *
 *                                                                                             *
 *	Mode switching is deliberately NOT public policy here -- StrategicViewManager is the only    *
 *	thing allowed to request a mode change (see strategicviewmanager.h).                         *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef __COMMANDERCAMERA_H
#define __COMMANDERCAMERA_H

#include "always.h"
#include "ccamera.h"
#include "vector3.h"


////////////////////////////////////////////////////////////////
//
//	CommanderCameraDefaults
//
//	Tunables, exposed so that no call site ever has to spell a
//	magic number.  Values come from the commander design plan.
//
////////////////////////////////////////////////////////////////
namespace CommanderCameraDefaults
{
	const float		MIN_HEIGHT				= 50.0f;		//	metres above the look-at point
	const float		MAX_HEIGHT				= 200.0f;
	const float		START_HEIGHT			= 90.0f;
	const float		PAN_SPEED				= 10.0f;		//	metres/second at MIN_HEIGHT
	const float		ZOOM_SPEED				= 5.0f;		//	metres of height per unit of wheel input
	const float		ZOOM_SMOOTH_RATE		= 8.0f;		//	1/seconds -- exponential approach to the target height
	const float		TRANSITION_TIME		= 0.75f;		//	seconds, FPS <-> overhead
	const float		PITCH					= 1.30899694f;	//	75 degrees below the horizon, in radians
	const float		BLEND_LOOK_AHEAD		= 10.0f;		//	metres, used to build the blended look-at target

	//
	//	Easing.  Three separate rates, because they are three different
	//	sensations and tuning them together makes all three wrong:
	//
	//		PAN_SMOOTH_RATE    how fast the pan velocity catches up with the
	//		                   keys.  Low feels like a heavy camera; high feels
	//		                   like a cursor.  1/seconds.
	//		PAN_DECAY_RATE     how fast it coasts to a stop once the keys are
	//		                   released.  Deliberately faster than the ramp:  a
	//		                   camera that keeps sliding after you let go feels
	//		                   broken, not smooth.  1/seconds.
	//		FOCUS_SMOOTH_RATE  how fast Focus_On closes on its target.  1/seconds.
	//
	const float		PAN_SMOOTH_RATE		= 12.0f;
	const float		PAN_DECAY_RATE			= 18.0f;
	const float		FOCUS_SMOOTH_RATE		= 6.0f;

	//	Focus_On stops when it is this close, so it does not creep forever.
	const float		FOCUS_ARRIVE_DISTANCE	= 0.25f;
};


////////////////////////////////////////////////////////////////
//
//	CommanderCamera
//
////////////////////////////////////////////////////////////////
class CommanderCamera : public CCameraClass
{
public:

	////////////////////////////////////////////////////////////////
	//	Public data types
	////////////////////////////////////////////////////////////////
	enum ViewModeType
	{
		VIEW_MODE_FIRST_PERSON	= 0,
		VIEW_MODE_OVERHEAD
	};

	////////////////////////////////////////////////////////////////
	//	Public constructors/destructors
	////////////////////////////////////////////////////////////////
	CommanderCamera (void);
	virtual ~CommanderCamera (void);

	////////////////////////////////////////////////////////////////
	//	Public methods
	////////////////////////////////////////////////////////////////

	//
	//	The camera self registers so that StrategicViewManager can find it without
	//	owning it, and without Combat having to know who created the main camera.
	//	Returns NULL when the running build's main camera is a plain CCameraClass,
	//	in which case the strategic view degrades to a no-op.
	//
	static CommanderCamera *	Get_Instance (void)					{ return _TheInstance; }

	//
	//	Per-frame update.  Falls through to CCameraClass::Update() whenever the
	//	overhead view is neither active nor transitioning.
	//
	virtual void					Update (void) override;

	//
	//	Mode.  Only StrategicViewManager should drive these.
	//
	ViewModeType					Get_View_Mode (void) const			{ return ViewMode; }
	bool								Is_Overhead_View (void) const		{ return ViewMode == VIEW_MODE_OVERHEAD; }
	bool								Is_Transitioning (void)				{ return Is_Lerping (); }

	void								Enter_Overhead_View (const Vector3 &look_at_position);
	void								Enter_First_Person_View (void);

	//
	//	Framing.  Pan inputs are in the camera's own screen-relative axes and are
	//	scaled by the frame time and the current height by the caller-visible
	//	pan speed.  Zoom is positive to move closer to the ground.
	//
	//	Pan is *eased*:  the pan velocity ramps toward what the keys are asking
	//	for at PAN_SMOOTH_RATE and coasts to a stop at PAN_DECAY_RATE when they
	//	are released, so tapping a key nudges the view instead of teleporting it
	//	and holding one accelerates smoothly.  The coast is applied by Update on
	//	any frame Pan was not called, which is why StrategicViewManager::Think
	//	must keep running before MainCamera->Update() (see CONTRACTS.md).
	//
	void								Pan (float right_amount, float forward_amount, float frame_seconds);
	void								Zoom (float amount);

	//
	//	Current eased pan velocity, in metres/second on the ground plane.  Zero
	//	when the camera is at rest.  Exposed for diagnostics.
	//
	const Vector3 &				Get_Pan_Velocity (void) const		{ return PanVelocity; }

	const Vector3 &				Get_Look_At_Position (void) const	{ return LookAtPosition; }
	void								Set_Look_At_Position (const Vector3 &position);

	//
	//	Ease the look-at point to 'position' rather than snapping it.  This is
	//	the "show me that" motion:  a snap loses the commander's sense of where
	//	the new place is relative to the old one, which is the whole reason to
	//	have a strategic view.  Any pan input cancels it -- the player taking
	//	the controls always wins.
	//
	void								Focus_On (const Vector3 &position);
	void								Cancel_Focus (void);
	bool								Is_Focusing (void) const			{ return FocusActive; }
	const Vector3 &				Get_Focus_Target (void) const		{ return FocusTarget; }

	float								Get_Height (void) const				{ return CurrentHeight; }
	float								Get_Target_Height (void) const		{ return TargetHeight; }
	void								Set_Height (float height);

	float								Get_Overhead_Heading (void) const	{ return OverheadHeading; }
	void								Set_Overhead_Heading (float heading);

	//
	//	Tunables
	//
	void								Set_Min_Height (float height);
	float								Get_Min_Height (void) const		{ return MinHeight; }
	void								Set_Max_Height (float height);
	float								Get_Max_Height (void) const		{ return MaxHeight; }

	void								Set_Pan_Speed (float speed)		{ PanSpeed = speed; }
	float								Get_Pan_Speed (void) const			{ return PanSpeed; }

	void								Set_Zoom_Speed (float speed)		{ ZoomSpeed = speed; }
	float								Get_Zoom_Speed (void) const		{ return ZoomSpeed; }

	void								Set_Zoom_Smooth_Rate (float rate)	{ ZoomSmoothRate = rate; }
	float								Get_Zoom_Smooth_Rate (void) const	{ return ZoomSmoothRate; }

	void								Set_Transition_Time (float seconds){ TransitionTime = seconds; }
	float								Get_Transition_Time (void) const	{ return TransitionTime; }

	void								Set_Pan_Smooth_Rate (float rate)	{ PanSmoothRate = rate; }
	float								Get_Pan_Smooth_Rate (void) const	{ return PanSmoothRate; }

	void								Set_Pan_Decay_Rate (float rate)	{ PanDecayRate = rate; }
	float								Get_Pan_Decay_Rate (void) const	{ return PanDecayRate; }

	void								Set_Focus_Smooth_Rate (float rate){ FocusSmoothRate = rate; }
	float								Get_Focus_Smooth_Rate (void) const{ return FocusSmoothRate; }

	//
	//	Whether the FPS <-> overhead transition is eased or linear.  Eased by
	//	default; linear exists so a test can assert the endpoint arithmetic
	//	without the curve in the way.
	//
	void								Set_Eased_Transition (bool onoff)	{ EasedTransition = onoff; }
	bool								Is_Eased_Transition (void) const	{ return EasedTransition; }

	void								Set_Overhead_Pitch (float radians);
	float								Get_Overhead_Pitch (void) const	{ return OverheadPitch; }

	void								Reset_Tunables (void);

protected:

	////////////////////////////////////////////////////////////////
	//	Protected methods
	////////////////////////////////////////////////////////////////

	//
	//	Where the eye sits for the current look-at point, height, heading and pitch.
	//
	Vector3							Get_Overhead_Eye_Position (void) const;
	void								Update_Height (float frame_seconds);
	void								Update_Pan (float frame_seconds);
	void								Update_Focus (float frame_seconds);
	void								Apply_Overhead_Transform (const Vector3 &eye, const Vector3 &target);

	//
	//	Ease-in-ease-out on 0..1.  Used for the view transition, where a linear
	//	blend starts and stops with a visible jerk because the eye position is
	//	travelling tens of metres.
	//
	static float					Smooth_Step (float t);

	////////////////////////////////////////////////////////////////
	//	Protected member data
	////////////////////////////////////////////////////////////////
	ViewModeType					ViewMode;

	Vector3							LookAtPosition;
	float								CurrentHeight;
	float								TargetHeight;

	//
	//	The overhead orientation is kept separate from the inherited Heading/Tilt on
	//	purpose: those two belong to the first-person camera and are driven by the
	//	mouse.  Overwriting them would corrupt the view the player returns to.
	//
	float								OverheadHeading;
	float								OverheadPitch;

	//
	//	Tunables
	//
	float								MinHeight;
	float								MaxHeight;
	float								PanSpeed;
	float								ZoomSpeed;
	float								ZoomSmoothRate;
	float								TransitionTime;
	float								PanSmoothRate;
	float								PanDecayRate;
	float								FocusSmoothRate;

	//
	//	Eased panning state.  PanVelocity is metres/second on the ground plane;
	//	PanRequest* is what the keys asked for this frame and is consumed (and
	//	cleared) by Update.
	//
	Vector3							PanVelocity;
	float								PanRequestRight;
	float								PanRequestForward;
	bool								PanRequested;

	//
	//	Focus_On state.
	//
	Vector3							FocusTarget;
	bool								FocusActive;
	bool								EasedTransition;

	////////////////////////////////////////////////////////////////
	//	Static member data
	////////////////////////////////////////////////////////////////
	static CommanderCamera *	_TheInstance;
};


#endif //__COMMANDERCAMERA_H
