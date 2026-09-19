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
 *                     $Archive:: /Commando/Code/Combat/teampowergridservice.h                $*
 *                                                                                             *
 *	Roadmap Section 40, Phase 34 -- finite team power grid + strategic building capabilities.    *
 *                                                                                             *
 *	The rule this phase is shaped by: "Forbidden: LegacyPoweredFlag + RTSPowerFlag + translation *
 *	glue."  The canonical per-building powered/unpowered state already exists --                *
 *	BuildingGameObj::Enable_Power/Is_Power_Enabled (building.h) -- and this service's whole job   *
 *	is to decide what that flag should be for each building, replacing the coarse decision that   *
 *	decides it today (BaseControllerClass::Power_Base, basecontroller.cpp: every power plant      *
 *	alive means every building fully powered, none alive means every building fully unpowered,    *
 *	applied with one uniform loop).  Nothing here adds a second flag; RecalculateTeam's whole      *
 *	output *is* a sequence of Enable_Power calls on the one flag that already exists.              *
 *                                                                                             *
 *	Not wired into BaseControllerClass this pass -- see the design doc for exactly why           *
 *	(basecontroller.cpp:521-552 is the precise replacement point, and doing it safely needs a     *
 *	building to actually carry a resolved BuildingDescriptor, which no production call site        *
 *	assigns yet either).  What is built for real is the algorithm itself: deterministic finite     *
 *	generation/demand/allocation, driven and checked exactly the way it would be driven for real,   *
 *	just against registrations that may carry a null BuildingGameObj* for the self check.           *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef TEAMPOWERGRIDSERVICE_H
#define TEAMPOWERGRIDSERVICE_H

#include "always.h"
#include "vector.h"
#include "wwstring.h"

class BuildingGameObj;

typedef int PowerGridMemberIdType;


class TeamPowerGridService
{
	public:

		static void		Init(void);
		static void		Shutdown(void);

		/*
		**	building may be nullptr -- a synthetic member with no real GameObj behind it, which
		**	is what makes this service's core algorithm checkable with no device and no scene.
		**	A real caller always passes a real building; RecalculateTeam only calls
		**	Enable_Power on the ones that are not null.  member_id is the roadmap's own
		**	"stable server-owned persisted PowerGridMemberId" -- the deterministic tie-break
		**	among equal-priority buildings; a caller passing the building's own network object
		**	id gets a value that is already stable and already unique.
		*/
		static bool		RegisterBuilding(BuildingGameObj * building,int team,const char * definition_name,
													 PowerGridMemberIdType member_id);
		static void		UnregisterBuilding(PowerGridMemberIdType member_id);

		//	Convenience wrappers matching the roadmap's own named lifecycle hooks; each just
		//	updates the one member's state and calls RecalculateTeam for its team.
		static void		OnBuildingDestroyed(PowerGridMemberIdType member_id);
		static void		OnBuildingRevived(PowerGridMemberIdType member_id);
		static void		OnBuildingOperationalStateChanged(PowerGridMemberIdType member_id,bool destroyed);

		//	A descriptor reload changed generation/consumption/priority/requires_power for
		//	every member currently registered against that definition name; recalculates every
		//	team any of them belong to.
		static void		OnDescriptorReload(const char * definition_name);

		/*
		**	The allocation itself.  Available/RequestedPower are Section 40's own formulas;
		**	allocation is a stable sort by effective priority (descending), MemberId ascending
		**	as the deterministic tie-break, greedy until the running total would exceed
		**	AvailablePower.  Drives Enable_Power on every non-null registered building of the
		**	team, and (for a communications-capable member) registers/unregisters/updates its
		**	WorldShroudManager sensor source to match -- "communications coverage follows
		**	online state" made concrete via Section 30's own API, not a second shroud
		**	implementation.  Call on relevant state/config changes, never every frame.
		*/
		static void		RecalculateTeam(int team);

		static float	Get_Generation(int team);
		static float	Get_Requested_Demand(int team);
		static float	Get_Allocated_Demand(int team);

		/*
		**	Commander manual control.  Each of the four calls below recalculates its member's
		**	team immediately -- "manual disable immediately frees demand" and "manual
		**	priority/on-off control is authoritative" are both read here as "takes effect this
		**	call", not "takes effect whenever something else next recalculates."  A priority
		**	override replaces the descriptor's priority for allocation ordering until explicitly
		**	cleared.  Server-authoritative in the sense that nothing here is reachable from
		**	client code -- there is no network code in this file at all -- so a real caller
		**	enforcing that is Phase 36's job.
		*/
		static bool		Set_Manual_Disabled(PowerGridMemberIdType member_id,bool disabled);
		static bool		Is_Manual_Disabled(PowerGridMemberIdType member_id);
		static bool		Set_Priority_Override(PowerGridMemberIdType member_id,int priority);
		static bool		Clear_Priority_Override(PowerGridMemberIdType member_id);

		static bool		Is_Member_Powered(PowerGridMemberIdType member_id);

		/*
		**	True only for a registered member whose DefinitionName currently resolves to a
		**	loaded BuildingModelDescriptor.  This is the query BaseControllerClass::Power_Base
		**	(basecontroller.cpp, the coarse loop this service is meant to replace one building
		**	at a time) uses to know which buildings it must leave alone, because this service
		**	already owns their Enable_Power calls -- so the two allocators never both write the
		**	same flag.  A member with no resolved descriptor (still most buildings, since only
		**	a handful of definitions have JSON content yet) is untouched by this service and
		**	stays Power_Base's alone, exactly as before this phase existed.
		*/
		static bool		Has_Resolved_Descriptor(PowerGridMemberIdType member_id);

		/*
		**	Tiberium Silo wallet expansion -- Section 40's own formula, computed and queryable,
		**	stacking every active, online-eligible Silo on the team.  Applying this number to an
		**	actual player credit cap needs a wallet-cap enforcement point this codebase does not
		**	have yet -- see the design doc's "what this phase does not build".
		*/
		static float	Get_Wallet_Capacity_Bonus(int team);

		//	Debug/telemetry -- Section 40's own named list.
		static int		Get_Powered_Count(int team);
		static int		Get_Shed_Count(int team);
		static int		Get_Manual_Off_Count(int team);
		static int		Get_Generator_Count(int team);
		static int		Get_Consumer_Count(int team);

		static int		Get_Member_Count(void);

		/*
		**	Enumeration, for Phase 40's save pass.  Section 46 is explicit about what persists
		**	from this service -- the member id, the manual enable/disable state and the priority
		**	override -- and equally explicit that generation, demand and the shed result are
		**	*recomputed* after load, never saved ("do not serialize a second independent
		**	powered/unpowered truth").  So these three are all that is exposed: there is no
		**	accessor here for a saveable allocation result, because saving one would be the bug.
		*/
		static bool		Get_Member_By_Index(int index,PowerGridMemberIdType * out_member_id,int * out_team);
		static bool		Get_Priority_Override(PowerGridMemberIdType member_id,int * out_priority);
};

#endif // TEAMPOWERGRIDSERVICE_H
