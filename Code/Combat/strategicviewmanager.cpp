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
 *                     $Archive:: /Commando/Code/Combat/strategicviewmanager.cpp              $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "strategicviewmanager.h"

#include "chunkio.h"
#include "combat.h"
#include "commandercamera.h"
#include "commandermodecontroller.h"
#include "debug.h"
#include "hud.h"
#include "input.h"
#include "weaponview.h"
#include "matrix3d.h"
#include "physicalgameobj.h"
#include "soldier.h"
#include "timemgr.h"
#include "wwdebug.h"
#include "wwmath.h"


////////////////////////////////////////////////////////////////
//	Save/load chunk ids
////////////////////////////////////////////////////////////////
enum
{
	CHUNKID_VARIABLES		= 0x02150900
};

enum
{
	MICROCHUNKID_LOOK_AT_POSITION		= 1,
	MICROCHUNKID_CAMERA_HEIGHT,
	MICROCHUNKID_OVERHEAD_HEADING,
	MICROCHUNKID_PLAYER_TYPE,
	MICROCHUNKID_FALLBACK_PAN_DISTANCE
};


////////////////////////////////////////////////////////////////
//	Local constants
////////////////////////////////////////////////////////////////
static const float	DEFAULT_MAX_PAN_DISTANCE	= 100.0f;		//	metres from the base centre


////////////////////////////////////////////////////////////////
//
//	DefaultStrategicViewBounds
//
//	Permissive stub used until the BaseBoundary adapter (A3) is
//	installed.  It reports no known base centre, which makes the
//	clamp fall back to the position the view was opened at, and a
//	generous radius.
//
////////////////////////////////////////////////////////////////
class DefaultStrategicViewBounds : public IStrategicViewBounds
{
public:
	virtual bool	Get_Base_Center (int /*player_type*/, Vector3 & /*center*/) const	{ return false; }
	virtual float	Get_Max_Pan_Distance (int /*player_type*/) const						{ return DEFAULT_MAX_PAN_DISTANCE; }
};


////////////////////////////////////////////////////////////////
//
//	DefaultStrategicViewPermission
//
//	Permissive stub used until the CommanderPermissions adapter (A1)
//	is installed.
//
////////////////////////////////////////////////////////////////
class DefaultStrategicViewPermission : public IStrategicViewPermission
{
public:
	virtual bool	Is_Strategic_View_Permitted (int /*player_type*/) const	{ return true; }
};


static DefaultStrategicViewBounds			_DefaultBounds;
static DefaultStrategicViewPermission		_DefaultPermission;


////////////////////////////////////////////////////////////////
//	Static member data
////////////////////////////////////////////////////////////////
StrategicViewManager::ViewModeType	StrategicViewManager::CurrentViewMode	= StrategicViewManager::VIEW_FIRST_PERSON;
int											StrategicViewManager::PlayerType			= 0;
bool											StrategicViewManager::InputEnabled		= true;
bool											StrategicViewManager::ExitOnMenuToggle	= true;
bool											StrategicViewManager::IsInitialized		= false;
bool											StrategicViewManager::WasHUDEnabled		= true;
float											StrategicViewManager::FallbackMaxPanDistance = DEFAULT_MAX_PAN_DISTANCE;
Vector3										StrategicViewManager::OpenPosition		(0.0f, 0.0f, 0.0f);

IStrategicViewBounds *					StrategicViewManager::BoundsProvider		= NULL;
IStrategicViewPermission *				StrategicViewManager::PermissionProvider	= NULL;


////////////////////////////////////////////////////////////////
//
//	Init
//
////////////////////////////////////////////////////////////////
void
StrategicViewManager::Init (void)
{
	CurrentViewMode			= VIEW_FIRST_PERSON;
	PlayerType					= 0;
	InputEnabled				= true;
	ExitOnMenuToggle			= true;
	FallbackMaxPanDistance	= DEFAULT_MAX_PAN_DISTANCE;
	OpenPosition				= Vector3 (0.0f, 0.0f, 0.0f);

	//
	//	The providers are deliberately NOT cleared here.  They are installed by
	//	the Commando side (CommanderViewAdapters) and describe the running build,
	//	not the running game -- clearing them made Init un-install them, which is
	//	how the strategic view ended up anchoring on the player instead of the
	//	base and letting non-commanders in.  Shutdown is what un-installs.
	//

	//
	//	Make sure the strategic view has usable key bindings even when the
	//	installed input configuration predates them.
	//
	//	OpenW3D: the commander pan/zoom defaults are rows in input.cpp's _BuiltinDefaultKeys.

	IsInitialized = true;
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Shutdown
//
////////////////////////////////////////////////////////////////
void
StrategicViewManager::Shutdown (void)
{
	//
	//	Never leave the game stuck in the overhead view.
	//
	CommanderCamera *camera = CommanderCamera::Get_Instance ();
	if (camera != NULL) {
		camera->Enter_First_Person_View ();
	}

	//
	//	Unconditionally, and even when there is no camera:  leaving the input
	//	muted or the weapon hidden would follow the player into the next game.
	//
	Apply_View_Presentation (false);

	//
	//	Providers survive shutdown for the same reason Init does not clear them:
	//	they belong to the build, not to the game.  They are only ever replaced
	//	by an explicit Set_*_Provider call.
	//
	CurrentViewMode		= VIEW_FIRST_PERSON;
	IsInitialized			= false;
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Set_Bounds_Provider
//
////////////////////////////////////////////////////////////////
void
StrategicViewManager::Set_Bounds_Provider (IStrategicViewBounds *provider)
{
	BoundsProvider = provider;
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Set_Permission_Provider
//
////////////////////////////////////////////////////////////////
void
StrategicViewManager::Set_Permission_Provider (IStrategicViewPermission *provider)
{
	PermissionProvider = provider;
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Get_Bounds_Provider
//
////////////////////////////////////////////////////////////////
IStrategicViewBounds *
StrategicViewManager::Get_Bounds_Provider (void)
{
	return (BoundsProvider != NULL) ? BoundsProvider : &_DefaultBounds;
}


////////////////////////////////////////////////////////////////
//
//	Get_Permission_Provider
//
////////////////////////////////////////////////////////////////
IStrategicViewPermission *
StrategicViewManager::Get_Permission_Provider (void)
{
	return (PermissionProvider != NULL) ? PermissionProvider : &_DefaultPermission;
}


////////////////////////////////////////////////////////////////
//
//	Set_Fallback_Max_Pan_Distance
//
////////////////////////////////////////////////////////////////
void
StrategicViewManager::Set_Fallback_Max_Pan_Distance (float distance)
{
	FallbackMaxPanDistance = distance;
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Get_View_Mode_Name
//
////////////////////////////////////////////////////////////////
const char *
StrategicViewManager::Get_View_Mode_Name (ViewModeType mode)
{
	return (mode == VIEW_OVERHEAD) ? "overhead" : "fps";
}


////////////////////////////////////////////////////////////////
//
//	Get_Anchor_Position
//
//	Where the overhead view should open.  The base centre if the
//	bounds provider knows one, otherwise the player, otherwise the
//	camera itself.
//
////////////////////////////////////////////////////////////////
bool
StrategicViewManager::Get_Anchor_Position (Vector3 &position)
{
	if (Get_Bounds_Provider ()->Get_Base_Center (PlayerType, position)) {
		return true;
	}

	SoldierGameObj *star = COMBAT_STAR;
	if (star != NULL) {
		star->Get_Position (&position);
		return true;
	}

	CommanderCamera *camera = CommanderCamera::Get_Instance ();
	if (camera != NULL) {
		position = camera->Get_Transform ().Get_Translation ();
		position.Z = 0.0f;
		return true;
	}

	return false;
}


////////////////////////////////////////////////////////////////
//
//	Request_View_Mode
//
//	The single point at which the camera mode may change.
//
////////////////////////////////////////////////////////////////
bool
StrategicViewManager::Request_View_Mode (ViewModeType mode)
{
	CommanderCamera *camera = CommanderCamera::Get_Instance ();
	if (camera == NULL) {

		//
		//	The running build's main camera is a plain CCameraClass (or there is
		//	no camera at all, as in the level editor).  Nothing to switch.
		//
		return false;
	}

	if (mode == CurrentViewMode) {
		return true;
	}

	if (mode == VIEW_OVERHEAD) {

		//
		//	Commander-only.  Never assume the local player may do this.
		//
		if (Get_Permission_Provider ()->Is_Strategic_View_Permitted (PlayerType) == false) {
			return false;
		}

		Vector3 anchor (0.0f, 0.0f, 0.0f);
		if (Get_Anchor_Position (anchor) == false) {
			return false;
		}

		OpenPosition = anchor;
		camera->Enter_Overhead_View (anchor);
		CurrentViewMode = VIEW_OVERHEAD;
		Apply_View_Presentation (true);

	} else {

		camera->Enter_First_Person_View ();
		CurrentViewMode = VIEW_FIRST_PERSON;
		Apply_View_Presentation (false);
	}

	return true;
}


////////////////////////////////////////////////////////////////
//
//	Apply_View_Presentation
//
//	Everything that has to change about what the player sees and
//	what their keys do when the view mode changes.
//
//	Three separate things were wrong without this, and they are all
//	the same mistake -- the overhead view swapped the camera and
//	nothing else, so the first-person presentation carried on
//	underneath it:
//
//		*	the held weapon was still drawn across the view, because
//			WeaponViewClass renders whenever COMBAT_STAR has a weapon
//		*	the combat HUD (crosshair, health, ammo) still drew, all
//			of it meaningless from 90 metres up
//		*	the movement and mouse-look keys still drove the soldier,
//			so panning the camera also walked the body
//
//	The first-person state is restored exactly, so leaving the view
//	puts the player back where they were.
//
////////////////////////////////////////////////////////////////
void
StrategicViewManager::Apply_View_Presentation (bool overhead)
{
	//
	//	Keys drive the camera, not the body.  Selective:  the commander
	//	functions stay live so the view can still be panned and closed.
	//
	Input::Commander_UI_Enable (overhead);

	//
	//	The gun in the corner of the screen.
	//
	WeaponViewClass::Enable (overhead == false);

	//
	//	The combat HUD.  Remembered rather than assumed, so a player who had
	//	already hidden the HUD does not get it handed back on the way out.
	//
	if (overhead) {
		WasHUDEnabled = HUDClass::Is_Enabled ();
		HUDClass::Enable (false);
	} else {
		HUDClass::Enable (WasHUDEnabled);
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Toggle_View_Mode
//
////////////////////////////////////////////////////////////////
bool
StrategicViewManager::Toggle_View_Mode (void)
{
	return Request_View_Mode ((CurrentViewMode == VIEW_OVERHEAD) ? VIEW_FIRST_PERSON : VIEW_OVERHEAD);
}


////////////////////////////////////////////////////////////////
//
//	Think
//
//	Client-side only.  Nothing below mutates game state.
//
////////////////////////////////////////////////////////////////
void
StrategicViewManager::Think (void)
{
	if (IsInitialized == false) {
		return ;
	}

	CommanderCamera *camera = CommanderCamera::Get_Instance ();
	if (camera == NULL) {
		return ;
	}

	if (InputEnabled) {

		//
		//	Entering and leaving the view.
		//
		//	OpenW3D: entering and leaving are CommanderModeController's (the Commander Mode toggle
		//	key and the sidebar); only the menu key's implicit exit is handled here.
		if (CurrentViewMode == VIEW_OVERHEAD) {

			const bool exit_requested =
				(ExitOnMenuToggle && Input::Get_State (INPUT_FUNCTION_MENU_TOGGLE));

			if (exit_requested) {
				CommanderModeController::Exit_Commander_Mode ();
			}
		}
	}

	//
	//	If the permission was revoked while we were up there (command transferred,
	//	player changed team) drop straight back to first person.
	//
	if (	CurrentViewMode == VIEW_OVERHEAD &&
			Get_Permission_Provider ()->Is_Strategic_View_Permitted (PlayerType) == false)
	{
		Request_View_Mode (VIEW_FIRST_PERSON);
	}

	if (CurrentViewMode == VIEW_OVERHEAD) {
		const float frame_seconds = TimeManager::Get_Frame_Real_Seconds ();

		if (InputEnabled) {
			Update_Input (camera, frame_seconds);
		}

		Clamp_To_Base (camera);
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Update_Input
//
////////////////////////////////////////////////////////////////
void
StrategicViewManager::Update_Input (CommanderCamera *camera, float frame_seconds)
{
	WWASSERT (camera != NULL);

	//
	//	Pan.  Analogue amounts so a joystick or an accelerated key works too.
	//
	const float forward	= Input::Get_Amount (INPUT_FUNCTION_COMMANDER_PAN_FORWARD) -
								  Input::Get_Amount (INPUT_FUNCTION_COMMANDER_PAN_BACKWARD);
	const float right		= Input::Get_Amount (INPUT_FUNCTION_COMMANDER_PAN_RIGHT) -
								  Input::Get_Amount (INPUT_FUNCTION_COMMANDER_PAN_LEFT);

	camera->Pan (right, forward, frame_seconds);

	//
	//	Zoom.  The mouse wheel sliders deliver a magnitude per notch; the camera
	//	turns that into a target height and eases toward it, so this is smooth
	//	without any interpolation code here.
	//
	const float zoom = Input::Get_Amount (INPUT_FUNCTION_COMMANDER_ZOOM_IN) -
							 Input::Get_Amount (INPUT_FUNCTION_COMMANDER_ZOOM_OUT);

	camera->Zoom (zoom);
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Clamp_To_Base
//
//	The camera may never leave the team's base area.  Until the
//	BaseBoundary adapter is installed the centre falls back to
//	wherever the view was opened, which keeps the clamp meaningful
//	rather than silently unbounded.
//
////////////////////////////////////////////////////////////////
void
StrategicViewManager::Clamp_To_Base (CommanderCamera *camera)
{
	WWASSERT (camera != NULL);

	IStrategicViewBounds *bounds = Get_Bounds_Provider ();

	Vector3 center = OpenPosition;
	bounds->Get_Base_Center (PlayerType, center);

	float max_distance = bounds->Get_Max_Pan_Distance (PlayerType);
	if (max_distance <= 0.0f) {
		max_distance = FallbackMaxPanDistance;
	}

	if (max_distance <= 0.0f) {
		return ;
	}

	Vector3 look_at = camera->Get_Look_At_Position ();

	//
	//	Clamp in the ground plane only -- the height is the zoom axis and is
	//	clamped separately by the camera.
	//
	Vector3 offset (look_at.X - center.X, look_at.Y - center.Y, 0.0f);

	const float distance = offset.Length ();
	if (distance > max_distance && distance > WWMATH_EPSILON) {
		offset *= (max_distance / distance);
		look_at.X = center.X + offset.X;
		look_at.Y = center.Y + offset.Y;
		camera->Set_Look_At_Position (look_at);
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Get_Camera_Position
//
////////////////////////////////////////////////////////////////
bool
StrategicViewManager::Get_Camera_Position (Vector3 &position)
{
	CommanderCamera *camera = CommanderCamera::Get_Instance ();
	if (camera == NULL) {
		return false;
	}

	position = camera->Get_Transform ().Get_Translation ();
	return true;
}


////////////////////////////////////////////////////////////////
//
//	Get_Look_At_Position
//
////////////////////////////////////////////////////////////////
bool
StrategicViewManager::Get_Look_At_Position (Vector3 &position)
{
	CommanderCamera *camera = CommanderCamera::Get_Instance ();
	if (camera == NULL) {
		return false;
	}

	position = camera->Get_Look_At_Position ();
	return true;
}


////////////////////////////////////////////////////////////////
//
//	Get_Camera_Height
//
////////////////////////////////////////////////////////////////
float
StrategicViewManager::Get_Camera_Height (void)
{
	CommanderCamera *camera = CommanderCamera::Get_Instance ();
	if (camera == NULL) {
		return 0.0f;
	}

	return camera->Get_Height ();
}


////////////////////////////////////////////////////////////////
//
//	Save
//
////////////////////////////////////////////////////////////////
bool
StrategicViewManager::Save (ChunkSaveClass &csave)
{
	CommanderCamera *camera = CommanderCamera::Get_Instance ();

	Vector3	look_at (0.0f, 0.0f, 0.0f);
	float		height	= 0.0f;
	float		heading	= 0.0f;

	if (camera != NULL) {
		look_at	= camera->Get_Look_At_Position ();
		height	= camera->Get_Target_Height ();
		heading	= camera->Get_Overhead_Heading ();
	}

	csave.Begin_Chunk (CHUNKID_VARIABLES);
		WRITE_MICRO_CHUNK (csave, MICROCHUNKID_LOOK_AT_POSITION,		look_at);
		WRITE_MICRO_CHUNK (csave, MICROCHUNKID_CAMERA_HEIGHT,			height);
		WRITE_MICRO_CHUNK (csave, MICROCHUNKID_OVERHEAD_HEADING,		heading);
		WRITE_MICRO_CHUNK (csave, MICROCHUNKID_PLAYER_TYPE,			PlayerType);
		WRITE_MICRO_CHUNK (csave, MICROCHUNKID_FALLBACK_PAN_DISTANCE,	FallbackMaxPanDistance);
	csave.End_Chunk ();

	return true;
}


////////////////////////////////////////////////////////////////
//
//	Load
//
////////////////////////////////////////////////////////////////
bool
StrategicViewManager::Load (ChunkLoadClass &cload)
{
	Vector3	look_at (0.0f, 0.0f, 0.0f);
	float		height	= 0.0f;
	float		heading	= 0.0f;

	while (cload.Open_Chunk ()) {

		switch (cload.Cur_Chunk_ID ()) {

			case CHUNKID_VARIABLES:
				while (cload.Open_Micro_Chunk ()) {
					switch (cload.Cur_Micro_Chunk_ID ()) {

						READ_MICRO_CHUNK (cload, MICROCHUNKID_LOOK_AT_POSITION,		look_at);
						READ_MICRO_CHUNK (cload, MICROCHUNKID_CAMERA_HEIGHT,			height);
						READ_MICRO_CHUNK (cload, MICROCHUNKID_OVERHEAD_HEADING,		heading);
						READ_MICRO_CHUNK (cload, MICROCHUNKID_PLAYER_TYPE,				PlayerType);
						READ_MICRO_CHUNK (cload, MICROCHUNKID_FALLBACK_PAN_DISTANCE,	FallbackMaxPanDistance);

						default:
							Debug_Say (("Unhandled Micro Chunk:%d File:%s Line:%d\r\n", cload.Cur_Micro_Chunk_ID (), __FILE__, __LINE__));
							break;
					}
					cload.Close_Micro_Chunk ();
				}
				break;

			default:
				Debug_Say (("Unhandled Chunk:%d File:%s Line:%d\r\n", cload.Cur_Chunk_ID (), __FILE__, __LINE__));
				break;
		}

		cload.Close_Chunk ();
	}

	//
	//	The live mode is never restored -- a save taken from the overhead view
	//	comes back in first person and the player re-opens the view.
	//
	CurrentViewMode	= VIEW_FIRST_PERSON;
	OpenPosition		= look_at;

	CommanderCamera *camera = CommanderCamera::Get_Instance ();
	if (camera != NULL) {
		camera->Set_Look_At_Position (look_at);
		camera->Set_Overhead_Heading (heading);
		if (height > 0.0f) {
			camera->Set_Height (height);
		}
	}

	return true;
}
