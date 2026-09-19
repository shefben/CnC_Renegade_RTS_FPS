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
 *                     $Archive:: /Commando/Code/Combat/rtstelemetryservice.h                 $*
 *                                                                                             *
 *	Roadmap Section 38, Zero Hour / SAGE feature 25 -- RTS telemetry / statistics.               *
 *                                                                                             *
 *	Zero Hour's donor concept is `AcademyStats`, which records a broad set of RTS behaviour     *
 *	for its in-match coaching UI.  The roadmap is explicit that the coaching UI and advice       *
 *	rules are not wanted -- only the underlying idea that a broad, cheap set of counters is      *
 *	worth keeping.  So this is a generic instrumentation service with one shape (a category, an  *
 *	optional small subtype, a count and a running total) rather than one struct per system,      *
 *	the same "one implementation, not one class per producer" discipline every prior phase in    *
 *	this pass has used.                                                                          *
 *                                                                                             *
 *	Most of the roadmap's named categories have no producer yet, because the systems that       *
 *	would report them do not exist yet either -- the power grid is Phase 34, Commander           *
 *	placement is Phase 37, PCT/MCT is Phase 38.  Recording the category now and wiring its       *
 *	caller when that phase lands is the same discipline this project already applies to          *
 *	missing art and to P26's unimplemented component contracts: the absent case is named, not    *
 *	invented early to look finished.                                                             *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef RTSTELEMETRYSERVICE_H
#define RTSTELEMETRYSERVICE_H

#include "always.h"
#include "vector.h"


/*
**	Every category the roadmap names, as one enumeration.  A category with no current producer
**	is still listed here -- see the file header -- so the phase that adds its producer only has
**	to call Record_Event, not invent a category first.
*/
enum TelemetryCategoryType
{
	TELEMETRY_BUILDING_CONSTRUCTED = 0,
	TELEMETRY_BUILDING_DESTROYED,
	TELEMETRY_PROCEDURAL_CONSTRUCTION_START,
	TELEMETRY_PROCEDURAL_CONSTRUCTION_CANCEL,
	TELEMETRY_PROCEDURAL_CONSTRUCTION_COMPLETE,
	TELEMETRY_UNIT_CREATED,
	TELEMETRY_UNIT_DESTROYED,
	TELEMETRY_VEHICLE_CREATED,
	TELEMETRY_VEHICLE_DESTROYED,
	TELEMETRY_AIRCRAFT_CREATED,
	TELEMETRY_AIRCRAFT_DESTROYED,
	TELEMETRY_TEAM_INCOME,
	TELEMETRY_POWER_GENERATION_CHANGED,
	TELEMETRY_POWER_DEMAND_CHANGED,
	TELEMETRY_POWER_LOAD_SHED,
	TELEMETRY_POWER_RESTORED,
	TELEMETRY_COMMANDER_MANUAL_POWER_OVERRIDE,
	TELEMETRY_COMMS_COVERAGE_CHANGED,
	TELEMETRY_SILO_WALLET_CAPACITY_CHANGED,
	TELEMETRY_PLACEMENT_ATTEMPT,
	TELEMETRY_PLACEMENT_REJECTED,
	TELEMETRY_PLACEMENT_SUCCESS,
	TELEMETRY_PCT_MCT_USED,
	TELEMETRY_FACTORY_USED,
	TELEMETRY_PRODUCTION_QUEUE_OUTCOME,
	TELEMETRY_AI_ORDER,
	TELEMETRY_AI_STATE_TRANSITION,
	TELEMETRY_FACTORY_EXIT_BLOCKED,
	TELEMETRY_KILL,
	TELEMETRY_DAMAGE,
	TELEMETRY_TERRAIN_FEATURE_USED,
	TELEMETRY_PATH_FAILURE,
	TELEMETRY_PATH_REPLAN,
	TELEMETRY_ASSET_PRESSURE,
	TELEMETRY_PARTICLE_PRESSURE,
	TELEMETRY_FOLIAGE_PRESSURE,
	TELEMETRY_TERRAIN_PRESSURE,

	TELEMETRY_CATEGORY_COUNT		// sentinel, not a real category
};

enum { TELEMETRY_MAX_RECORDS = 512 };


class RTSTelemetryService
{
	public:

		static void		Init(void);
		static void		Shutdown(void);

		//	Clears every counter without changing Is_Enabled -- a session boundary, or between
		//	self-check scenarios.
		static void		Reset(void);

		/*
		**	Off means Record_Event is a single branch and nothing else -- "disabled or low-
		**	overhead in normal release mode unless explicitly enabled."  There is no build-mode
		**	switch wired to this yet (no consumer needs one today), so the default is on; the
		**	one place that would flip it off later is this flag, not a second code path.
		*/
		static void		Set_Enabled(bool onoff);
		static bool		Is_Enabled(void);

		/*
		**	Record one occurrence.  subtype is category-specific and small (a team id, a unit
		**	class, a rejection reason) -- callers that do not need one pass 0.  value
		**	accumulates into a running total alongside the count, for categories where a number
		**	matters more than a tally (income, damage, wallet capacity).
		**
		**	No caller has to check Is_Enabled itself; this does, and returns immediately when
		**	off.  Core gameplay must never branch on whether telemetry accepted an event --
		**	nothing here returns a value a caller could mistakenly treat as authoritative.
		*/
		static void		Record_Event(TelemetryCategoryType category,int subtype = 0,float value = 0.0f);

		//	subtype == -1 means "every subtype of this category, summed."
		static int		Get_Event_Count(TelemetryCategoryType category,int subtype = -1);
		static float	Get_Event_Total(TelemetryCategoryType category,int subtype = -1);

		static int		Get_Refusal_Count(void);

		/*
		**	A structured snapshot -- the acceptance's own phrase -- as a flat, walkable list.
		**	A caller that does not know the category enum in advance (a test harness dump, a
		**	later debug overlay) can still enumerate everything that has been recorded.
		*/
		struct SnapshotEntryClass
		{
			SnapshotEntryClass(void) : Category(TELEMETRY_BUILDING_CONSTRUCTED),Subtype(0),Count(0),Total(0.0f) { }
			bool	operator == (const SnapshotEntryClass &) const	{ return false; }
			bool	operator != (const SnapshotEntryClass &) const	{ return true; }

			TelemetryCategoryType	Category;
			int							Subtype;
			int							Count;
			float							Total;
		};
		static int		Get_Snapshot(DynamicVectorClass<SnapshotEntryClass> & out);

		static const char *	Get_Category_Name(TelemetryCategoryType category);
};

#endif // RTSTELEMETRYSERVICE_H
