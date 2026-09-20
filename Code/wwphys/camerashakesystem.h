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
 *                     $Archive:: /Commando/Code/wwphys/camerashakesystem.h                   $*
 *                                                                                             *
 *              Original Author:: Greg Hjelstrom                                               *
 *                                                                                             *
 *                      $Author:: Greg_h                                                      $*
 *                                                                                             *
 *                     $Modtime:: 7/13/00 4:06p                                               $*
 *                                                                                             *
 *                    $Revision:: 1                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef CAMERASHAKESYSTEM_H
#define CAMERASHAKESYSTEM_H


#include "always.h"
#include "vector3.h"
#include "multilist.h"
#include "mempool.h"

class CameraClass;

/**
** CameraShakeSystemClass
** This class encapsulates all of the logic and data needed to implement camera "shakes"
** These are used to simulate explosions, earthquakes, etc.
*/
class CameraShakeSystemClass
{
public:

	CameraShakeSystemClass(void);
	~CameraShakeSystemClass(void);

	enum
	{
		FLAGS_NONE = 0,
		FLAGS_IGNOREPOSITION,
	};

	void		Add_Camera_Shake(		const Vector3 & position,
											float radius = 50.0f,
											float duration = 1.5f,
											float power = 1.0f	);
	/*
	**	Per-shot weapon response.  Roadmap Section 52A, Phase 62.
	**
	**	A separate track from the positional shakers above, because a shot is not an explosion
	**	somewhere in the world that the camera happens to be near -- it is the player's own
	**	weapon, it has no position and no radius, and it recovers in a fraction of the time.
	**
	**	Strictly cosmetic, and structurally so: the shake system only ever touches the camera
	**	transform, and it does so in PhysicsSceneClass::Apply_Camera_Shakes, which runs after
	**	the simulation has already decided where the shot went.  Aim direction, spread and hit
	**	registration are the server's and never see any of this -- the same guarantee the
	**	explosion shake has always had, for the same reason.
	*/
	void		Add_Weapon_Kick(float scale,float recovery_seconds);
	float		Get_Weapon_FOV_Kick(void) const;
	float		Get_Weapon_Roll(void) const;

	//	Master scale, 0 to 1.  Zero -- the default -- is current behaviour exactly: Add_Weapon_Kick
	//	returns without recording anything and both getters answer zero.
	static void		Set_Weapon_Response_Scale(float scale);
	static float	Get_Weapon_Response_Scale(void);

	//	The pure recovery curve, exposed because it is what a device-less check can prove.
	//	One at the moment of the shot, zero once recovered, and a fast ease between.
	static float	Compute_Recovery(float elapsed,float duration);

	void		Timestep(float dt);
	void		Update_Camera(CameraClass & camera);

protected:

	/**
	** CameraShakerClass
	** This class encapsulates the current state of a camera shaker.  It is a multi-list object
	** and is allocated in pools.
	*/
	class CameraShakerClass : public MultiListObjectClass, public AutoPoolClass<CameraShakerClass,256>
	{
	public:
		CameraShakerClass(const Vector3 & position,float radius,float duration,float power);
		~CameraShakerClass(void);

		void					Timestep(float dt)							{ ElapsedTime += dt; }
		bool					Is_Expired(void)								{ return (ElapsedTime >= Duration); }
		void					Compute_Rotations(const Vector3 & pos,Vector3 * set_angles);

	protected:

		Vector3				Position;
		float					Radius;
		float					Duration;
		float					Intensity;

		float					ElapsedTime;
		Vector3				Omega;
		Vector3				Phi;
	};

	//	Phase 62: the live weapon kick.  One value, because the player has one camera and a
	//	second shot before the first has recovered should restart the kick rather than stack
	//	into something that throws the view around.
	float		WeaponKickScale;
	float		WeaponKickElapsed;
	float		WeaponKickDuration;

	MultiListClass<CameraShakerClass>	CameraShakerList;

};


#endif //CAMERASHAKESYSTEM_H

