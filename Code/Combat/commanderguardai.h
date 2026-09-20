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
 *                     $Archive:: /Commando/Code/Combat/commanderguardai.h                    $*
 *                                                                                             *
 *	Roadmap Section 37/45 -- the caller GuardStateController.h's own header says it was          *
 *	waiting for: "the caller -- eventually Phase 39's Commander AI unit, not built this pass --  *
 *	is the one that turns an intent into an ActionClass::Attack/Goto call".  That caller is      *
 *	this file.  It owns one GuardStateControllerClass instance per unit under an ORDER_GUARD,    *
 *	feeds it a position and (Section 3A.4/3A.6) a nearest-human distance every server tick, and  *
 *	translates whatever intent comes back into the same ScriptEngine calls Dispatch_Order        *
 *	already uses for every other order -- so guard is a fifth verb reaching the same AI, not a   *
 *	second AI layer next to it.                                                                  *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef COMMANDERGUARDAI_H
#define COMMANDERGUARDAI_H

#include "always.h"
#include "vector3.h"

class CommanderGuardAI
{
	public:

		static void		Init(void);
		static void		Shutdown(void);

		//	Server-side.  Begins (or re-anchors) guarding for this unit id at this world point;
		//	idempotent-safe to call every time a fresh ORDER_GUARD arrives, including a second
		//	one at a different point -- GuardStateControllerClass::Activate on its own is not
		//	(it is a no-op on an already-active controller), so this deactivates first when one
		//	is already running.
		static void		Guard(int unit_id,const Vector3 & anchor,float radius);

		//	Any other order supersedes a guard order; called once at the top of every other
		//	Dispatch_Order case so a unit given a fresh MOVE/ATTACK/etc. is not still fighting
		//	its own old guard controller's Update() every frame.  Cheap no-op if not guarding.
		static void		Stop_Guarding(int unit_id);

		static bool		Is_Guarding(int unit_id);

		//	A hostile hit landed on a guarding unit -- routed from
		//	CommanderPopulationObserverClass::Damaged, the observer already attached to every
		//	Commander-produced unit for population accounting.  No-op for a unit not guarding.
		static void		Report_Damage(int unit_id,int attacker_id,const Vector3 & attacker_position,float now);

		//	Once a server tick.  Advances every active controller and drives its intent into the
		//	unit's real AI action; drops a controller whose unit no longer resolves.
		static void		Update(float now,float seconds);

		static int		Get_Active_Count(void);
};

#endif // COMMANDERGUARDAI_H
