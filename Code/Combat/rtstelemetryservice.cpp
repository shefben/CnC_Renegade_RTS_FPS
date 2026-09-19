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
 *                     $Archive:: /Commando/Code/Combat/rtstelemetryservice.cpp               $*
 *                                                                                             *
 *	Roadmap Section 38, Zero Hour / SAGE feature 25 -- RTS telemetry / statistics.               *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "rtstelemetryservice.h"


struct TelemetryRecordClass
{
	TelemetryRecordClass(void) : Category(TELEMETRY_BUILDING_CONSTRUCTED),Subtype(0),Count(0),Total(0.0f) { }
	bool	operator == (const TelemetryRecordClass &) const	{ return false; }
	bool	operator != (const TelemetryRecordClass &) const	{ return true; }

	TelemetryCategoryType	Category;
	int							Subtype;
	int							Count;
	float							Total;
};

static bool		_Enabled = true;
static int		_RefusalCount = 0;
static DynamicVectorClass<TelemetryRecordClass>	_Records;

static const char * const	_CategoryNames [TELEMETRY_CATEGORY_COUNT] =
{
	"building_constructed",
	"building_destroyed",
	"procedural_construction_start",
	"procedural_construction_cancel",
	"procedural_construction_complete",
	"unit_created",
	"unit_destroyed",
	"vehicle_created",
	"vehicle_destroyed",
	"aircraft_created",
	"aircraft_destroyed",
	"team_income",
	"power_generation_changed",
	"power_demand_changed",
	"power_load_shed",
	"power_restored",
	"commander_manual_power_override",
	"comms_coverage_changed",
	"silo_wallet_capacity_changed",
	"placement_attempt",
	"placement_rejected",
	"placement_success",
	"pct_mct_used",
	"factory_used",
	"production_queue_outcome",
	"ai_order",
	"ai_state_transition",
	"factory_exit_blocked",
	"kill",
	"damage",
	"terrain_feature_used",
	"path_failure",
	"path_replan",
	"asset_pressure",
	"particle_pressure",
	"foliage_pressure",
	"terrain_pressure",
};


void RTSTelemetryService::Init(void)
{
	_Enabled = true;
	_RefusalCount = 0;
	_Records.Delete_All();
}

void RTSTelemetryService::Shutdown(void)
{
	_Records.Delete_All();
}

void RTSTelemetryService::Reset(void)
{
	_Records.Delete_All();
	_RefusalCount = 0;
}

void RTSTelemetryService::Set_Enabled(bool onoff)		{ _Enabled = onoff; }
bool RTSTelemetryService::Is_Enabled(void)					{ return _Enabled; }


void RTSTelemetryService::Record_Event(TelemetryCategoryType category,int subtype,float value)
{
	if (!_Enabled) {
		return;
	}
	if (category < 0 || category >= TELEMETRY_CATEGORY_COUNT) {
		return;
	}

	for (int i = 0; i < _Records.Count (); i++) {
		if (_Records [i].Category == category && _Records [i].Subtype == subtype) {
			_Records [i].Count++;
			_Records [i].Total += value;
			return;
		}
	}

	if (_Records.Count () >= TELEMETRY_MAX_RECORDS) {
		_RefusalCount++;
		return;
	}

	TelemetryRecordClass record;
	record.Category	= category;
	record.Subtype		= subtype;
	record.Count		= 1;
	record.Total		= value;
	_Records.Add (record);
}


int RTSTelemetryService::Get_Event_Count(TelemetryCategoryType category,int subtype)
{
	int total = 0;
	for (int i = 0; i < _Records.Count (); i++) {
		if (_Records [i].Category != category) continue;
		if (subtype >= 0 && _Records [i].Subtype != subtype) continue;
		total += _Records [i].Count;
	}
	return total;
}

float RTSTelemetryService::Get_Event_Total(TelemetryCategoryType category,int subtype)
{
	float total = 0.0f;
	for (int i = 0; i < _Records.Count (); i++) {
		if (_Records [i].Category != category) continue;
		if (subtype >= 0 && _Records [i].Subtype != subtype) continue;
		total += _Records [i].Total;
	}
	return total;
}

int RTSTelemetryService::Get_Refusal_Count(void)		{ return _RefusalCount; }


int RTSTelemetryService::Get_Snapshot(DynamicVectorClass<SnapshotEntryClass> & out)
{
	out.Delete_All ();
	for (int i = 0; i < _Records.Count (); i++) {
		SnapshotEntryClass entry;
		entry.Category	= _Records [i].Category;
		entry.Subtype	= _Records [i].Subtype;
		entry.Count		= _Records [i].Count;
		entry.Total		= _Records [i].Total;
		out.Add (entry);
	}
	return out.Count ();
}


const char * RTSTelemetryService::Get_Category_Name(TelemetryCategoryType category)
{
	if (category < 0 || category >= TELEMETRY_CATEGORY_COUNT) {
		return "unknown";
	}
	return _CategoryNames [category];
}
