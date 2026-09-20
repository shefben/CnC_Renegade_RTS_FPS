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
 *                     $Archive:: /Commando/Code/Combat/runtimedoorservice.h                  $*
 *                                                                                             *
 *	Roadmap Section 44 -- real open/close behavior for a component-assembly building's door     *
 *	part.  Checked whether the real asset (models/mp buildings/c&c_mode, mgpwr_doors.w3d /       *
 *	mnpwr_doors.w3d) carries a usable baked door animation before writing this -- it does not    *
 *	(796 bytes, a hierarchy with no animation-channel-sized data), so this is real, project-     *
 *	owned motion computed from the door part's own measured geometry, not a guess at an          *
 *	animation name.  See docs/zerohour/BuildingComponentAssembly.md for the full reasoning.       *
 *                                                                                             *
 *	Follows the exact shape roadmap Section 44's own BuildingConstructionService already          *
 *	established for "timed, server-authoritative, deterministically replayable on a client":     *
 *	a static service, registered per building_id, advanced from one server_time_seconds value    *
 *	both sides read (CommanderStateClass::Get_Estimated_Server_Time), with only the moment a      *
 *	direction changes crossing the network -- never a per-frame position.                        *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef RUNTIMEDOORSERVICE_H
#define RUNTIMEDOORSERVICE_H

#include "always.h"
#include "matrix3d.h"
#include "vector3.h"

class StaticPhysClass;
class BuildingGameObj;


class RuntimeDoorService
{
	public:

		static void		Init(void);
		static void		Shutdown(void);

		/*
		**	Registered once by BuildingGameObj::Build_Runtime_Assembly for every "door"-kind
		**	assembly part it builds; re-registering the same (building id, part index) replaces
		**	the record in place (a rebuild -- e.g. coming out of the destroyed state -- starts
		**	the door closed again, which is the honest default).
		**
		**	`closed_local`/`open_local` are the part's transform relative to the *building*
		**	(building->Get_Runtime_Transform() is re-applied every Update, not baked in here),
		**	so a door stays correctly attached even though nothing in this engine moves a
		**	building once placed.  `trigger_position`/`trigger_radius` are world space, both
		**	derived from the part's own measured bounding box by the caller -- nothing here
		**	invents a number.
		*/
		static void		Register(BuildingGameObj *building,int part_index,StaticPhysClass *phys,
											const Matrix3D &closed_local,const Matrix3D &open_local,
											const Vector3 &trigger_position,float trigger_radius);

		//	Drops every door this building owns.  Called from Release_Runtime_Components before
		//	the StaticPhysClass objects themselves are torn down, so this never holds a dangling
		//	pointer even for the span of a single call.
		static void		Unregister_Building(int building_id);

		/*
		**	The authoritative half (server-only, internally gated by CombatManager::I_Am_Server):
		**	proximity against GameObjManager::Get_Smart_Game_Obj_List, direction changes, and the
		**	one dirty-bit mark per change (BuildingGameObj::Mark_Runtime_Door_State_Dirty).  The
		**	transform half runs unconditionally on both sides -- every registered door's current
		**	position is a deterministic function of (direction, fraction-at-last-change,
		**	time-of-last-change, now), so re-applying it here needs nothing from the network this
		**	frame didn't already have.
		*/
		static void		Update(float now_seconds);

		//	Network plumbing.  Get_State is Export_Rare's read, Set_State is Import_Rare's write
		//	(and is also how a client's own registration seeds its first frame).  Both are
		//	no-ops -- false / untouched -- for a (building_id,part_index) nothing has registered,
		//	safe to call speculatively before a door's descriptor entry is even known to exist.
		static bool		Get_State(int building_id,int part_index,bool *out_direction_is_opening,
											float *out_fraction_at_start,float *out_start_time_seconds);
		static void		Set_State(int building_id,int part_index,bool direction_is_opening,
											float fraction_at_start,float start_time_seconds);

		//	Pure arithmetic, exposed so it can be proven without a scene, a device or a
		//	registered door -- the same "no rand(), no clock, no floating-point accumulation
		//	across calls" purity BuildingConstructionService's own generator holds itself to.
		//	0.0 = fully closed, 1.0 = fully open.
		static float	Compute_Fraction(bool direction_is_opening,float fraction_at_start,
													 float start_time_seconds,float now_seconds,
													 float duration_seconds);

		//	Gameplay-tuning constants, not measured/derived values -- ordinary defaults, exactly
		//	like BuildingConstructionService::FOUNDATION_END/ROWS_END are.
		static const float	OPEN_CLOSE_DURATION_SECONDS;	// time to fully open or fully close
		static const float	HOLD_OPEN_SECONDS;					// stays open this long after the last visitor leaves
		static const float	TRIGGER_RADIUS_MARGIN;				// added to the part's own measured radius
		static const float	RISE_HEIGHT_MARGIN;					// multiplier on the part's own measured height
};

#endif // RUNTIMEDOORSERVICE_H
