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
 *                     $Archive:: /Commando/Code/Combat/commandermodecontroller.cpp           $*
 *                                                                                             *
 *	Roadmap Section 41, Phase 35 -- Commander Mode camera / embodied player.                     *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "commandermodecontroller.h"

#include "castres.h"
#include "ccamera.h"
#include "combat.h"
#include "lineseg.h"
#include "phys.h"
#include "physcoltest.h"
#include "rendobj.h"
#include "worldspatialindex.h"
#include "wwmath.h"


static const float	COMMANDER_DEFAULT_ZOOM_MIN		= 50.0f;
static const float	COMMANDER_DEFAULT_ZOOM_MAX		= 2000.0f;
static const float	COMMANDER_DEFAULT_TILT_MIN		= 15.0f;		// degrees above the horizon
static const float	COMMANDER_DEFAULT_TILT_MAX		= 85.0f;
static const float	COMMANDER_PICK_RAY_LENGTH		= 10000.0f;


static bool							_Active			= false;
static SoldierGameObj *			_Star				= nullptr;

static Vector3						_WorldMin (-100000.0f,-100000.0f,-100000.0f);
static Vector3						_WorldMax ( 100000.0f, 100000.0f, 100000.0f);
static float						_ZoomMin			= COMMANDER_DEFAULT_ZOOM_MIN;
static float						_ZoomMax			= COMMANDER_DEFAULT_ZOOM_MAX;
static float						_TiltMin			= COMMANDER_DEFAULT_TILT_MIN;
static float						_TiltMax			= COMMANDER_DEFAULT_TILT_MAX;

static Vector3						_Focus (0.0f,0.0f,0.0f);
static float						_Zoom				= 400.0f;
static float						_Tilt				= 55.0f;
static float						_Rotation		= 0.0f;


static float	Clamp(float value,float lo,float hi)
{
	if (value < lo) return lo;
	if (value > hi) return hi;
	return value;
}


void CommanderModeController::Init(void)
{
	_Active	= false;
	_Star		= nullptr;

	_WorldMin.Set (-100000.0f,-100000.0f,-100000.0f);
	_WorldMax.Set ( 100000.0f, 100000.0f, 100000.0f);
	_ZoomMin = COMMANDER_DEFAULT_ZOOM_MIN;
	_ZoomMax = COMMANDER_DEFAULT_ZOOM_MAX;
	_TiltMin = COMMANDER_DEFAULT_TILT_MIN;
	_TiltMax = COMMANDER_DEFAULT_TILT_MAX;

	_Focus.Set (0.0f,0.0f,0.0f);
	_Zoom		= 400.0f;
	_Tilt		= 55.0f;
	_Rotation= 0.0f;
}

void CommanderModeController::Shutdown(void)
{
	_Active	= false;
	_Star		= nullptr;
}


bool CommanderModeController::Enter_Commander_Mode(SoldierGameObj * star)
{
	if (star == nullptr) return false;
	if (_Active) return false;

	_Active	= true;
	_Star		= star;
	return true;
}

void CommanderModeController::Exit_Commander_Mode(void)
{
	_Active	= false;
	_Star		= nullptr;
}

bool CommanderModeController::Is_Active(void)						{ return _Active; }
CommanderCameraModeType CommanderModeController::Get_Mode(void)	{ return _Active ? COMMANDER_MODE : FPS_MODE; }
SoldierGameObj * CommanderModeController::Get_Embodied_Star(void)	{ return _Star; }


void CommanderModeController::Set_World_Bounds(const Vector3 & minimum,const Vector3 & maximum)
{
	_WorldMin = minimum;
	_WorldMax = maximum;
	Set_Focus (_Focus);		// re-clamp whatever focus is already set
}

void CommanderModeController::Set_Zoom_Range(float minimum,float maximum)
{
	if (minimum <= 0.0f || maximum < minimum) return;
	_ZoomMin = minimum;
	_ZoomMax = maximum;
	Set_Zoom (_Zoom);
}

void CommanderModeController::Set_Tilt_Range(float minimum_degrees,float maximum_degrees)
{
	if (maximum_degrees < minimum_degrees) return;
	_TiltMin = minimum_degrees;
	_TiltMax = maximum_degrees;
	Set_Tilt (_Tilt);
}


void CommanderModeController::Set_Focus(const Vector3 & world_point)
{
	_Focus.X = Clamp (world_point.X,_WorldMin.X,_WorldMax.X);
	_Focus.Y = Clamp (world_point.Y,_WorldMin.Y,_WorldMax.Y);
	_Focus.Z = Clamp (world_point.Z,_WorldMin.Z,_WorldMax.Z);
}

void CommanderModeController::Pan(const Vector3 & delta)
{
	Set_Focus (_Focus + delta);
}

Vector3 CommanderModeController::Get_Focus(void)	{ return _Focus; }


void CommanderModeController::Set_Zoom(float height)		{ _Zoom = Clamp (height,_ZoomMin,_ZoomMax); }
void CommanderModeController::Zoom(float delta)				{ Set_Zoom (_Zoom + delta); }
float CommanderModeController::Get_Zoom(void)					{ return _Zoom; }

void CommanderModeController::Set_Tilt(float degrees)		{ _Tilt = Clamp (degrees,_TiltMin,_TiltMax); }
float CommanderModeController::Get_Tilt(void)					{ return _Tilt; }

void CommanderModeController::Set_Rotation(float degrees)
{
	_Rotation = WWMath::Wrap (degrees,0.0f,360.0f);
}
float CommanderModeController::Get_Rotation(void)				{ return _Rotation; }


Matrix3D CommanderModeController::Compute_Camera_Transform(void)
{
	float tilt_rad		= DEG_TO_RADF (_Tilt);
	float rotation_rad	= DEG_TO_RADF (_Rotation);

	//	Standard orbit-camera offset from the focus point: horizontal distance shrinks as tilt
	//	steepens toward looking straight down, height grows the same way.  Z is up, matching
	//	every other world-space convention in this project (WorldTerrainSystem::Sample_Height,
	//	Section 30's shroud grid).
	float horizontal_distance	= _Zoom * WWMath::Cos (tilt_rad);
	float height					= _Zoom * WWMath::Sin (tilt_rad);

	Vector3 camera_position (
		_Focus.X + horizontal_distance * WWMath::Sin (rotation_rad),
		_Focus.Y - horizontal_distance * WWMath::Cos (rotation_rad),
		_Focus.Z + height);

	Vector3 forward = _Focus - camera_position;
	if (forward.Length () < 0.001f) {
		forward.Set (0.0f,1.0f,0.0f);		// directly overhead; pick an arbitrary forward
	} else {
		forward.Normalize ();
	}

	Vector3 world_up (0.0f,0.0f,1.0f);
	Vector3 right = Vector3::Cross_Product (forward,world_up);
	if (right.Length () < 0.001f) {
		right.Set (1.0f,0.0f,0.0f);			// forward parallel to world up; pick an arbitrary right
	} else {
		right.Normalize ();
	}
	Vector3 up = Vector3::Cross_Product (right,forward);
	up.Normalize ();

	Matrix3D transform;
	transform.Set (forward,right,up,camera_position);
	return transform;
}


bool CommanderModeController::Pick_At_Cursor(const Vector3 & ray_origin,const Vector3 & ray_direction,
															 Vector3 * out_world_point)
{
	if (out_world_point == nullptr) return false;

	float length = ray_direction.Length ();
	if (length < 0.0001f) return false;
	Vector3 direction = ray_direction * (1.0f / length);

	Vector3 far_point = ray_origin + direction * COMMANDER_PICK_RAY_LENGTH;
	LineSegClass ray (ray_origin,far_point);
	CastResultStruct result;

	//	Collision group 0: a Commander cursor pick is asking "what is the ground/geometry
	//	here", not making a gameplay/team-filtered damage query, so no group restricts it.
	PhysRayCollisionTestClass raytest (ray,&result,0);

	if (!WorldSpatialIndex::Query_Ray (raytest)) {
		return false;
	}

	*out_world_point = ray_origin + direction * (result.Fraction * COMMANDER_PICK_RAY_LENGTH);
	return true;
}


void CommanderModeController::Update(float seconds,bool star_is_alive)
{
	(void)seconds;

	if (!_Active) return;

	if (!star_is_alive) {
		Exit_Commander_Mode ();		// "death immediately exits Commander Mode"
		return;
	}

	if (COMBAT_CAMERA != nullptr) {
		COMBAT_CAMERA->Set_Transform (Compute_Camera_Transform ());
	}
}
