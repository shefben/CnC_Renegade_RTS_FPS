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
 *                     $Archive:: /Commando/Code/Combat/worlddebugdisplay.h                   $*
 *                                                                                             *
 *	Roadmap Section 33 -- the Combat-layer owner of DebugRenderSystem (Code/ww3d2).  Same split  *
 *	as Section 32's status rings: ww3d2 cannot reach GameObjManager or any wwphys world           *
 *	service, so the per-layer queries that turn a toggle into real geometry live here.           *
 *                                                                                             *
 *	Wired for real this pass: DEBUG_LAYER_BUILDING_FOOTPRINTS, from BuildingGameObj::Get_Bounds  *
 *	over GameObjManager's own list.  Every other layer is enumerated and toggleable, and draws   *
 *	nothing, because the system it would visualize either has no public per-instance             *
 *	introspection to add without expanding this pass's scope well past Section 33 itself, or     *
 *	does not exist yet (building semantic nodes are Phase 39, MCT/purchase-terminal anchors are   *
 *	Phase 38, the Commander cursor is Phase 35/37).  See the design doc for the full list.        *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef WORLDDEBUGDISPLAY_H
#define WORLDDEBUGDISPLAY_H

#include "always.h"
#include "debugrendersystem.h"


class WorldDebugDisplay
{
	public:

		//	Called once a Combat tick.  Clears the previous frame's lines and, for every layer
		//	this pass gives a real producer, submits this frame's geometry.  Lazily creates the
		//	scene object the first time any layer is turned on.
		static void		Update(void);

		static void		Shutdown(void);
};

#endif // WORLDDEBUGDISPLAY_H
