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
 *                     $Archive:: /Commando/Code/Combat/commandermodecontroller.h             $*
 *                                                                                             *
 *	Roadmap Section 41, Phase 35 -- Commander Mode camera / embodied player.                     *
 *                                                                                             *
 *	"Add a strategic camera without replacing/degrading FPS controls" is the whole shape of      *
 *	this file.  It never touches CCameraClass's own update logic (Code/Combat/ccamera.cpp) --    *
 *	that class already handles first-person, third-person, sniper and cinematic-host-model       *
 *	camera behaviour, and rewriting any of it under time pressure this late in a long pass        *
 *	would risk exactly the FPS regression the acceptance forbids.  Instead this is an external,   *
 *	narrow hook: while Commander Mode is active, the one call site that would have called         *
 *	CCameraClass::Update (Code/Combat/combat.cpp's "Camera 1" block) calls this class's Update     *
 *	instead, which computes its own transform and applies it directly.  The instant Commander     *
 *	Mode ends, that call site goes back to calling CCameraClass::Update exactly as before -- FPS   *
 *	behaviour outside Commander Mode is provably unchanged because nothing about it was ever       *
 *	touched.                                                                                      *
 *                                                                                             *
 *	Embodiment is enforced by construction, not by convention: Enter_Commander_Mode refuses a     *
 *	null star, and this class never constructs a GameObj of any kind -- there is no code path      *
 *	here that could spawn a second player entity.  It also never *dereferences* the star it is     *
 *	handed (SoldierGameObj is only forward-declared) -- the one fact this class needs about the    *
 *	star's state, whether it is still alive, is a parameter the real caller supplies each Update    *
 *	call, the same "pass in the fact, get back a decision" split Section 37's                       *
 *	GuardStateControllerClass already uses, and for the same reason: it makes "death immediately    *
 *	exits Commander Mode" checkable with no device, no scene and no live SoldierGameObj.             *
 *                                                                                             *
 *	Intelligence: this class never calls WorldShroudManager::RegisterStrategicSensorSource --      *
 *	provable by grep, the same proof style Section 30's own camera-independence rule already        *
 *	established.  "Camera movement alone reveals no hidden enemy" holds because there is no path     *
 *	from anything in this file into the shroud at all.                                              *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef COMMANDERMODECONTROLLER_H
#define COMMANDERMODECONTROLLER_H

#include "always.h"
#include "matrix3d.h"
#include "vector3.h"

class SoldierGameObj;

enum CommanderCameraModeType
{
	FPS_MODE = 0,
	COMMANDER_MODE,
};


class CommanderModeController
{
	public:

		static void		Init(void);
		static void		Shutdown(void);

		/*
		**	Refuses a null star -- there is nothing to embody.  Refuses re-entering while
		**	already active (one mode at a time, the same body throughout).  Does not touch the
		**	camera itself; the first real transform is applied by the next Update call, the
		**	same lazy-effect discipline every scene-touching service in this project uses.
		*/
		static bool		Enter_Commander_Mode(SoldierGameObj * star);

		//	Returning to FPS restores camera/input to the same surviving soldier because this
		//	class never altered anything about that soldier in the first place -- it only
		//	stopped being the thing driving the camera.  Safe to call when already inactive.
		static void		Exit_Commander_Mode(void);

		static bool							Is_Active(void);
		static CommanderCameraModeType	Get_Mode(void);
		static SoldierGameObj *			Get_Embodied_Star(void);

		/*
		**	Bounds.  World bounds clamp Set_Focus/Pan to the map; the zoom/tilt ranges clamp
		**	Set_Zoom/Set_Tilt the same way.  Sensible defaults are set by Init, so a caller that
		**	never configures these still gets a camera that cannot pan or zoom to somewhere
		**	nonsensical.
		*/
		static void		Set_World_Bounds(const Vector3 & minimum,const Vector3 & maximum);
		static void		Set_Zoom_Range(float minimum,float maximum);
		static void		Set_Tilt_Range(float minimum_degrees,float maximum_degrees);

		static void		Set_Focus(const Vector3 & world_point);
		static void		Pan(const Vector3 & delta);
		static Vector3	Get_Focus(void);

		static void		Set_Zoom(float height);
		static void		Zoom(float delta);
		static float	Get_Zoom(void);

		static void		Set_Tilt(float degrees);
		static float	Get_Tilt(void);

		//	Rotation wraps rather than clamps -- a full turn is always allowed, "rotation if
		//	desired" from the roadmap's own wording.
		static void		Set_Rotation(float degrees);
		static float	Get_Rotation(void);

		//	Pure function of the current Focus/Zoom/Tilt/Rotation -- no device, no scene.  What
		//	the self check exercises directly, and what Update applies to the real camera.
		static Matrix3D	Compute_Camera_Transform(void);

		/*
		**	Cursor raycast / picking against real terrain and arbitrary W3D geometry, through
		**	Section 16's existing WorldSpatialIndex::Query_Ray -- no second raytest
		**	implementation.  Returns false (leaving *out_world_point untouched) when nothing is
		**	hit, which is also everything this can do with no physics scene under it.
		*/
		static bool		Pick_At_Cursor(const Vector3 & ray_origin,const Vector3 & ray_direction,
													Vector3 * out_world_point);

		/*
		**	Driven once a Combat tick while Is_Active().  star_is_alive is the one fact about
		**	the embodied star this class needs and does not look up itself -- see the header
		**	comment.  A false value exits Commander Mode immediately and applies no camera
		**	transform this call.  Otherwise computes and applies Compute_Camera_Transform to
		**	the real scene camera (COMBAT_CAMERA).
		*/
		static void		Update(float seconds,bool star_is_alive);
};

#endif // COMMANDERMODECONTROLLER_H
