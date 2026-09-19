/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 OpenW3D contributors
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
 *	Roadmap Section 28, Zero Hour / SAGE feature 13 -- debris rendering.                        *
 *                                                                                             *
 *	The Combat-layer half.  Code/ww3d2/debrisrenderer.h owns VISUAL_DEBRIS, a pool this layer   *
 *	cannot see the machinery of on purpose; this file owns GAMEPLAY_DEBRIS, the tier the        *
 *	roadmap says "may use existing WWPhys objects when collision matters", and the glue that    *
 *	decides which tier a given destruction event's fragments get and drains BridgeSystem's      *
 *	pending-debris queue, which has stood since P15-A recording what came off a broken span     *
 *	with nothing downstream ever reading it.                                                    *
 *                                                                                             *
 *	GAMEPLAY_DEBRIS fragments are RigidBodyClass instances -- exactly the class vehicles are     *
 *	already built from -- given a named model, a collision group and a ballistic kick, then      *
 *	added to the combat scene the same way WeaponClass::Make_Shell_Eject already adds a          *
 *	ProjectileClass shell casing.  They are not networked and are not GameObjs: every client     *
 *	that sees the destruction event spawns its own, so no fragment's position or existence is    *
 *	ever replicated, which is the acceptance's own wording -- "does not require every visible    *
 *	fragment to be a complete networked GameObj" -- applied literally.  A fragment with no       *
 *	named model, or arriving once the gameplay pool is full, falls back to the visual tier       *
 *	instead of being refused outright, so a destruction event never draws nothing for want of    *
 *	art that has not been made yet.                                                              *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef DEBRISSYSTEM_H
#define DEBRISSYSTEM_H

#include "always.h"
#include "vector3.h"
#include "vector4.h"

class BuildingGameObj;


class DebrisManager
{
	public:

		static void	Init(void);
		static void	Shutdown(void);
		static void	Reset(void);

		//	Off means every Spawn call is dropped in both tiers.
		static void	Set_Enabled(bool onoff);
		static bool	Is_Enabled(void);

		//	Call once a simulation frame.  Ages both tiers and drains whatever BridgeSystem has
		//	queued since the last call.
		static void	Update(void);

		/*
		**	Scatters up to 'total_count' fragments in a sphere of 'radius' around 'center'.  Up
		**	to 'gameplay_count' of them try the real-collision tier first, using 'model_name' if
		**	it is non-null and resolves; every fragment that tier turns down -- the remainder of
		**	'total_count', a missing model, or a full gameplay pool -- falls back to the pooled
		**	visual tier, so the count asked for is always what appears, just not always with
		**	collision.
		*/
		static void	Spawn(const Vector3 & center,float radius,int total_count,int gameplay_count,
						  const char * model_name,const Vector4 & color,float lifetime);

		//	The one consumer with no named debris model of its own: always the visual tier.
		static void	Spawn_Building_Debris(BuildingGameObj * building);

		//	How many real fragments may be alive at once.  Small on purpose -- see the .cpp --
		//	because a RigidBodyClass is a full physics object, not an array entry.
		static void	Set_Max_Gameplay(int count);
		static int	Get_Max_Gameplay(void);

		static int	Get_Live_Gameplay_Count(void);
		static int	Get_Dropped_Gameplay_Count(void);
};

#endif // DEBRISSYSTEM_H
