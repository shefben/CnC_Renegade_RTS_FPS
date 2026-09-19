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
 *                     $Archive:: /Commando/Code/Combat/statusoverlaysystem.h                 $*
 *                                                                                             *
 *	Roadmap Section 32, Zero Hour / SAGE feature 17 -- waypoint / status-circle renderers.       *
 *                                                                                             *
 *	The Combat-layer owner of StatusOverlayRenderer (Code/ww3d2), adding the one thing that      *
 *	pool cannot know about itself: who a ring belongs to, and therefore who is allowed to see    *
 *	it.  ww3d2 sits below wwphys, so the pool has no access to WorldShroudManager; this class    *
 *	does, and is where the acceptance line "respect shroud/per-player visibility" actually        *
 *	becomes true, by calling StatusOverlayRenderer::Set_Visible once a tick rather than by the    *
 *	pool doing its own shroud query.                                                             *
 *                                                                                             *
 *	Ownership rule: a ring with no owning team (Team < 0 -- an unowned construction footprint     *
 *	or a placement-validity ring, which are never "someone else's secret") or a ring owned by     *
 *	the current viewer's own team is always visible.  A ring owned by a different team is only    *
 *	visible where WorldShroudManager says that team's position is VISIBLE or RADAR_DETECTED to    *
 *	the viewer -- the same two states Section 31's tactical model treats as "there is something   *
 *	real to report here."                                                                        *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef STATUSOVERLAYSYSTEM_H
#define STATUSOVERLAYSYSTEM_H

#include "always.h"
#include "statusoverlayrenderer.h"
#include "vector3.h"
#include "vector4.h"


class StatusOverlaySystem
{
	public:

		/*
		**	team < 0 means unowned -- always visible to everyone, never shroud-gated.  Creates
		**	the scene object the first time any overlay is added, the same lazy pattern
		**	BulletManager::Create_Tracer uses for TracerSystemClass.
		*/
		static StatusOverlayHandleType	Add_Overlay(StatusOverlayKindType kind,const Vector3 & position,
																	float radius,const Vector4 & color,int team,float lifetime);
		static bool		Update_Overlay(StatusOverlayHandleType handle,const Vector3 & position,
												 float radius,const Vector4 & color);
		static bool		Remove_Overlay(StatusOverlayHandleType handle);
		static void		Remove_All(void);

		static int		Get_Overlay_Team(StatusOverlayHandleType handle);

		//	Splits out from Add_Overlay so the ownership table can be exercised without also
		//	triggering the lazy COMBAT_SCENE registration -- what the device-less self check
		//	uses.  Add_Overlay calls this itself; a caller going through Add_Overlay never
		//	needs to call it separately.
		static void		Set_Overlay_Team(StatusOverlayHandleType handle,int team);

		/*
		**	Ages the pool and recomputes Set_Visible for every live handle against the current
		**	viewer's team (COMBAT_STAR, when one exists).  Called once a Combat tick from
		**	CombatManager::Think, the same place BulletManager::Update and DebrisManager::Update
		**	are called from.
		*/
		static void		Update(void);

		/*
		**	The policy above, exposed as a pure query so it can be checked with no device, no
		**	scene and no COMBAT_STAR -- what Update() calls Set_Visible from.
		*/
		static bool		Is_Overlay_Visible_To(StatusOverlayHandleType handle,int viewing_team);

		static void		Shutdown(void);
};

#endif // STATUSOVERLAYSYSTEM_H
