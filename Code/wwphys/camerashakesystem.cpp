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
 *                 Project Name : WWPhys                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwphys/camerashakesystem.cpp                 $*
 *                                                                                             *
 *              Original Author:: Greg Hjelstrom                                               *
 *                                                                                             *
 *                      $Author:: Greg_h                                                      $*
 *                                                                                             *
 *                     $Modtime:: 6/12/01 10:25a                                              $*
 *                                                                                             *
 *                    $Revision:: 3                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "camerashakesystem.h"
#include "camera.h"
#include "wwmemlog.h"

/*
** (gth) According to my "research" the artists say that there are several factors that
** go into a good camera shake.
** - The motion should be sinusoidal.
** - Camera rotation is more effective than camera motion (good, I won't use any translation)
** - The camera should pitch up and down a lot more than it yaws left and right.
*/

DEFINE_AUTO_POOL(CameraShakeSystemClass::CameraShakerClass,256);

const float MIN_OMEGA			= DEG_TO_RADF(12.5f*360.0f);
const float MAX_OMEGA			= DEG_TO_RADF(15.0f*360.0f);
const float END_OMEGA			= DEG_TO_RADF(360.0f);
const float MIN_PHI				= DEG_TO_RADF(0.0f);
const float MAX_PHI				= DEG_TO_RADF(360.0f);
const Vector3 AXIS_ROTATION	= Vector3(DEG_TO_RADF(7.5f),DEG_TO_RADF(15.0f),DEG_TO_RADF(5.0f));

/************************************************************************************************
**
** CameraShakeSystemClass::CameraShakerClass Implementation
**
************************************************************************************************/
CameraShakeSystemClass::CameraShakerClass::CameraShakerClass
(
	const Vector3 & position,
	float radius,
	float duration,
	float intensity
) :
	Position(position),
	Radius(radius),
	Duration(duration),
	Intensity(intensity),
	ElapsedTime(0.0f)
{
	/*
	** Initialize random sinusoid values
	*/
	Omega.X = WWMath::Random_Float(MIN_OMEGA,MAX_OMEGA);
	Omega.Y = WWMath::Random_Float(MIN_OMEGA,MAX_OMEGA);
	Omega.Z = WWMath::Random_Float(MIN_OMEGA,MAX_OMEGA);
	Phi.X = WWMath::Random_Float(MIN_PHI,MAX_PHI);
	Phi.Y = WWMath::Random_Float(MIN_PHI,MAX_PHI);
	Phi.Z = WWMath::Random_Float(MIN_PHI,MAX_PHI);
}

CameraShakeSystemClass::CameraShakerClass::~CameraShakerClass(void)
{
}


void CameraShakeSystemClass::CameraShakerClass::Compute_Rotations(const Vector3 & camera_position, Vector3 * set_angles)
{
	WWASSERT(set_angles != nullptr);

	/*
	** We want several different sinusiods, each with a different phase shift and
	** frequency.  The frequency is a function of time as well, stretching the
	** sine wave out.  These waves are modulated based on the distance from the
	** center of the "shake", the intensity of the shake, and based on the axis
	** being affected.  The vertical axis should have about 3x the amplitude of
	** the horizontal axis.
	*/
	float len2 = (camera_position - Position).Length2();
	if (len2 > Radius*Radius) {
		return;
	}

	/*
	** f(t) = intensity(t,pos) * sin( omega(t) * t + phi );
	** intensity(t,pos) = intensity * (radius/distance) * timeremaing/totaltime
	** omega(t) = start_omega + (end_omega - start_omega) * t
	** phi = random(0..start_omega)
	*/
	float intensity = Intensity * (1.0f - WWMath::Sqrt(len2) / Radius) * (1.0f - ElapsedTime / Duration);
	for (int i=0; i<3; i++) {
		float omega = Omega[i] + (END_OMEGA - Omega[i]) * ElapsedTime;
		(*set_angles)[i] += AXIS_ROTATION[i] * intensity * WWMath::Sin(omega * ElapsedTime + Phi[i]);
	}
}


/************************************************************************************************
**
** CameraShakeSystemClass Implementation
**
************************************************************************************************/
CameraShakeSystemClass::CameraShakeSystemClass(void) :
	WeaponKickScale(0.0f),
	WeaponKickElapsed(0.0f),
	WeaponKickDuration(0.0f)
{
}

CameraShakeSystemClass::~CameraShakeSystemClass(void)
{
	/*
	** delete all of the objects out of the list
	*/
	while (!CameraShakerList.Is_Empty()) {
		CameraShakerClass * obj = CameraShakerList.Remove_Head();
		CameraShakerList.Remove(obj);
		delete obj;
	}
}

void CameraShakeSystemClass::Add_Camera_Shake
(
	const Vector3 & position,
	float radius,
	float duration,
	float power
)
{
	WWMEMLOG(MEM_PHYSICSDATA);
	/*
	** Allocate a new camera shaker object.  Note that these are mem-pooled so the allocation
	** is very cheap.
	*/
	CameraShakerClass * shaker = new CameraShakerClass(position,radius,duration,power);
	CameraShakerList.Add(shaker);
}

/*
**	How far a full-scale shot pushes the view.  Both are small on purpose: this has to be felt
**	rather than seen, and anything that actually moves the crosshair off the target would be a
**	gameplay change wearing a cosmetic label.
*/
static const float WEAPON_FOV_KICK	= DEG_TO_RADF(1.6f);
static const float WEAPON_ROLL_KICK	= DEG_TO_RADF(0.5f);

/*
**	Master scale for the whole of Phase 62.  Zero is current behaviour, exactly.
*/
static float	_WeaponResponseScale = 0.0f;


void CameraShakeSystemClass::Set_Weapon_Response_Scale(float scale)
{
	_WeaponResponseScale = WWMath::Clamp(scale,0.0f,1.0f);
}


float CameraShakeSystemClass::Get_Weapon_Response_Scale(void)
{
	return _WeaponResponseScale;
}


/***********************************************************************************************
 * CameraShakeSystemClass::Compute_Recovery -- the curve a shot recovers along                 *
 *                                                                                             *
 * One at the instant of the shot, zero once recovered, cubed on the way down so most of the   *
 * movement is gone almost immediately and the last of it eases out.  A linear recovery reads  *
 * as the camera being dragged back rather than springing back.                                *
 *=============================================================================================*/
float CameraShakeSystemClass::Compute_Recovery(float elapsed,float duration)
{
	if (duration <= 0.0f) {
		return 0.0f;
	}
	if (elapsed <= 0.0f) {
		return 1.0f;
	}
	if (elapsed >= duration) {
		return 0.0f;
	}

	float remaining = 1.0f - (elapsed / duration);
	return remaining * remaining * remaining;
}


/***********************************************************************************************
 * CameraShakeSystemClass::Add_Weapon_Kick --                                                  *
 *                                                                                             *
 * A second shot restarts the kick rather than adding to it.  Stacking would mean a machine    *
 * gun walks the view off the screen, which is the exact failure that makes a camera response  *
 * stop being cosmetic.                                                                         *
 *=============================================================================================*/
void CameraShakeSystemClass::Add_Weapon_Kick(float scale,float recovery_seconds)
{
	if (_WeaponResponseScale <= 0.0f) {
		return;
	}
	if ((scale <= 0.0f) || (recovery_seconds <= 0.0f)) {
		return;
	}

	WeaponKickScale = scale * _WeaponResponseScale;
	WeaponKickElapsed = 0.0f;
	WeaponKickDuration = recovery_seconds;
}


float CameraShakeSystemClass::Get_Weapon_FOV_Kick(void) const
{
	return WEAPON_FOV_KICK * WeaponKickScale *
			 Compute_Recovery(WeaponKickElapsed,WeaponKickDuration);
}


float CameraShakeSystemClass::Get_Weapon_Roll(void) const
{
	return WEAPON_ROLL_KICK * WeaponKickScale *
			 Compute_Recovery(WeaponKickElapsed,WeaponKickDuration);
}


void CameraShakeSystemClass::Timestep(float dt)
{
	//	Phase 62: the weapon kick ages here with everything else, so it recovers in wall-clock
	//	time regardless of frame rate.
	if (WeaponKickDuration > 0.0f) {
		WeaponKickElapsed += dt;
		if (WeaponKickElapsed >= WeaponKickDuration) {
			WeaponKickScale = 0.0f;
			WeaponKickElapsed = 0.0f;
			WeaponKickDuration = 0.0f;
		}
	}

	/*
	** Allow each camera shaker to timestep.  Any that expire are added to a temporary
	** list for deletion.
	*/
	MultiListClass<CameraShakerClass> deletelist;
	MultiListIterator<CameraShakerClass> iterator(&CameraShakerList);
	for (iterator.First(); !iterator.Is_Done(); iterator.Next()) {
		CameraShakerClass * obj = iterator.Peek_Obj();
		obj->Timestep(dt);
		if (obj->Is_Expired()) {
			deletelist.Add(obj);
		}
	}

	/*
	** Remove and delete all the ones that expired
	*/
	while (!deletelist.Is_Empty()) {
		CameraShakerClass * obj = deletelist.Remove_Head();
		CameraShakerList.Remove(obj);
		delete obj;
	}
}

void CameraShakeSystemClass::Update_Camera(CameraClass & camera)
{
	MultiListIterator<CameraShakerClass> iterator(&CameraShakerList);

	Vector3 angles(0,0,0);
	Vector3 camera_position;
	Matrix3D camera_transform;

	camera_transform = camera.Get_Transform();
	camera_transform.Get_Translation(&camera_position);

	/*
	** Accumulate the effects of any active camera shakers
	*/
	for (iterator.First(); !iterator.Is_Done(); iterator.Next()) {
		iterator.Peek_Obj()->Compute_Rotations(camera_position,&angles);
	}

	//	Phase 62: the weapon's own roll, added before the clamp so it shares the same ceiling
	//	the explosion shake has and cannot combine with one to tip the view over.
	angles.Z += Get_Weapon_Roll();

	/*
	** Clamp the result
	*/
	for (int i=0; i<3; i++) {
		WWMath::Clamp(angles[i],-AXIS_ROTATION[i],AXIS_ROTATION[i]);
	}

	/*
	** Apply to the camera
	*/
	camera_transform.Rotate_X(angles.X);
	camera_transform.Rotate_Y(angles.Y);
	camera_transform.Rotate_Z(angles.Z);
	camera.Set_Transform(camera_transform);
}


