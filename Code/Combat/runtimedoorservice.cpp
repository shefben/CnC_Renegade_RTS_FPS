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
 *                     $Archive:: /Commando/Code/Combat/runtimedoorservice.cpp                $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "runtimedoorservice.h"

#include "building.h"
#include "combat.h"
#include "gameobjmanager.h"
#include "smartgameobj.h"
#include "staticphys.h"

#include <cmath>


const float	RuntimeDoorService::OPEN_CLOSE_DURATION_SECONDS	= 1.0f;
const float	RuntimeDoorService::HOLD_OPEN_SECONDS					= 3.0f;
const float	RuntimeDoorService::TRIGGER_RADIUS_MARGIN			= 2.0f;
const float	RuntimeDoorService::RISE_HEIGHT_MARGIN				= 1.1f;


struct RuntimeDoorRecord
{
	RuntimeDoorRecord(void) : BuildingId(0),PartIndex(0),Phys(nullptr),Owner(nullptr),
										ClosedLocal(1),OpenLocal(1),TriggerPosition(0.0f,0.0f,0.0f),
										TriggerRadius(0.0f),DirectionIsOpening(false),
										FractionAtStart(0.0f),StartTimeSeconds(0.0f) { }
	bool	operator == (const RuntimeDoorRecord &) const	{ return false; }
	bool	operator != (const RuntimeDoorRecord &) const	{ return true; }

	int					BuildingId;
	int					PartIndex;
	StaticPhysClass *	Phys;
	BuildingGameObj *	Owner;
	Matrix3D				ClosedLocal;
	Matrix3D				OpenLocal;
	Vector3				TriggerPosition;
	float					TriggerRadius;

	bool					DirectionIsOpening;
	float					FractionAtStart;		// where the door physically was when Direction last changed
	float					StartTimeSeconds;	// server time Direction last changed
};

static DynamicVectorClass<RuntimeDoorRecord>	_Records;


static int
Find_Record (int building_id,int part_index)
{
	for (int i = 0; i < _Records.Count (); i++) {
		if ((_Records [i].BuildingId == building_id) && (_Records [i].PartIndex == part_index)) {
			return i;
		}
	}
	return -1;
}


void RuntimeDoorService::Init (void)
{
	_Records.Delete_All ();
}

void RuntimeDoorService::Shutdown (void)
{
	_Records.Delete_All ();
}


void RuntimeDoorService::Register (BuildingGameObj *building,int part_index,StaticPhysClass *phys,
												const Matrix3D &closed_local,const Matrix3D &open_local,
												const Vector3 &trigger_position,float trigger_radius)
{
	if ((building == nullptr) || (phys == nullptr)) {
		return ;
	}

	RuntimeDoorRecord record;
	record.BuildingId			= building->Get_ID ();
	record.PartIndex			= part_index;
	record.Phys					= phys;
	record.Owner				= building;
	record.ClosedLocal		= closed_local;
	record.OpenLocal			= open_local;
	record.TriggerPosition	= trigger_position;
	record.TriggerRadius	= trigger_radius;
	record.DirectionIsOpening	= false;
	record.FractionAtStart		= 0.0f;
	record.StartTimeSeconds	= 0.0f;

	int index = Find_Record (record.BuildingId,part_index);
	if (index < 0) {
		_Records.Add (record);
	} else {
		_Records [index] = record;
	}

	//	Starts closed and stays that way until Update (server) or a network Import_Rare (client)
	//	says otherwise, so the very first frame after construction already shows a closed,
	//	solid door rather than whatever an uninitialised transform would be.
	phys->Set_Transform (building->Get_Runtime_Transform () * closed_local);
	return ;
}


void RuntimeDoorService::Unregister_Building (int building_id)
{
	for (int i = _Records.Count () - 1; i >= 0; i--) {
		if (_Records [i].BuildingId == building_id) {
			_Records.Delete (i);
		}
	}
	return ;
}


float RuntimeDoorService::Compute_Fraction (bool direction_is_opening,float fraction_at_start,
															float start_time_seconds,float now_seconds,
															float duration_seconds)
{
	if (duration_seconds <= 0.0f) {
		return direction_is_opening ? 1.0f : 0.0f;
	}

	float elapsed	= now_seconds - start_time_seconds;
	float rate		= direction_is_opening ? 1.0f : -1.0f;
	float fraction	= fraction_at_start + rate * (elapsed / duration_seconds);

	if (fraction < 0.0f) fraction = 0.0f;
	if (fraction > 1.0f) fraction = 1.0f;
	return fraction;
}


static void
Apply_Transform (RuntimeDoorRecord &record,float now_seconds)
{
	if ((record.Phys == nullptr) || (record.Owner == nullptr)) {
		return ;
	}

	float fraction = RuntimeDoorService::Compute_Fraction (record.DirectionIsOpening,record.FractionAtStart,
																				 record.StartTimeSeconds,now_seconds,
																				 RuntimeDoorService::OPEN_CLOSE_DURATION_SECONDS);

	Vector3 closed_t = record.ClosedLocal.Get_Translation ();
	Vector3 open_t		= record.OpenLocal.Get_Translation ();
	Vector3 t			= closed_t + (open_t - closed_t) * fraction;

	//	Rotation never changes for a vertical-retract door (see docs/zerohour/
	//	BuildingComponentAssembly.md), so the closed transform's basis is reused as-is -- only
	//	the translation the door rises along is interpolated.
	Matrix3D local = record.ClosedLocal;
	local.Set_Translation (t);

	record.Phys->Set_Transform (record.Owner->Get_Runtime_Transform () * local);
	return ;
}


static bool
Anyone_Near (const Vector3 &position,float radius)
{
	float radius2 = radius * radius;

	SLNode<SmartGameObj> *node = GameObjManager::Get_Smart_Game_Obj_List ()->Head ();
	for ( ; node != nullptr; node = node->Next ()) {
		SmartGameObj *obj = node->Data ();
		if (obj == nullptr) {
			continue;
		}

		Vector3 obj_pos (0.0f,0.0f,0.0f);
		obj->Get_Position (&obj_pos);

		if ((obj_pos - position).Length2 () <= radius2) {
			return true;
		}
	}

	return false;
}


void RuntimeDoorService::Update (float now_seconds)
{
	if (CombatManager::I_Am_Server ()) {
		for (int i = 0; i < _Records.Count (); i++) {
			RuntimeDoorRecord &record = _Records [i];
			if (record.Owner == nullptr) {
				continue;
			}

			//	Not "near": that identifier is a legacy Windows SDK macro (empty expansion,
			//	segmented-memory-model leftover) and silently breaks this declaration.
			bool someone_close = Anyone_Near (record.TriggerPosition,record.TriggerRadius);

			if (someone_close && !record.DirectionIsOpening) {

				//	Closed or mid-close, and someone just arrived -- start (or resume) opening
				//	from wherever the door physically is right now, not from scratch.  This is
				//	what keeps a direction reversal continuous instead of snapping.
				record.FractionAtStart		= Compute_Fraction (record.DirectionIsOpening,record.FractionAtStart,
																				  record.StartTimeSeconds,now_seconds,
																				  OPEN_CLOSE_DURATION_SECONDS);
				record.DirectionIsOpening	= true;
				record.StartTimeSeconds	= now_seconds;
				record.Owner->Mark_Runtime_Door_State_Dirty ();

			} else if (!someone_close && record.DirectionIsOpening) {

				//	Fully open (or opening) and empty for the hold duration -- start closing.
				//	"Fully open since" is derived rather than stored: the time fraction 1.0 was
				//	first reached at this rate, given where/when the current opening began.
				float fully_open_since = record.StartTimeSeconds +
													 (1.0f - record.FractionAtStart) * OPEN_CLOSE_DURATION_SECONDS;

				if (now_seconds - fully_open_since >= HOLD_OPEN_SECONDS) {
					record.FractionAtStart		= Compute_Fraction (record.DirectionIsOpening,record.FractionAtStart,
																					  record.StartTimeSeconds,now_seconds,
																					  OPEN_CLOSE_DURATION_SECONDS);
					record.DirectionIsOpening	= false;
					record.StartTimeSeconds	= now_seconds;
					record.Owner->Mark_Runtime_Door_State_Dirty ();
				}
			}
		}
	}

	for (int i = 0; i < _Records.Count (); i++) {
		Apply_Transform (_Records [i],now_seconds);
	}

	return ;
}


bool RuntimeDoorService::Get_State (int building_id,int part_index,bool *out_direction_is_opening,
												 float *out_fraction_at_start,float *out_start_time_seconds)
{
	int index = Find_Record (building_id,part_index);
	if (index < 0) {
		return false;
	}

	if (out_direction_is_opening != nullptr)	*out_direction_is_opening	= _Records [index].DirectionIsOpening;
	if (out_fraction_at_start != nullptr)		*out_fraction_at_start		= _Records [index].FractionAtStart;
	if (out_start_time_seconds != nullptr)		*out_start_time_seconds	= _Records [index].StartTimeSeconds;
	return true;
}


void RuntimeDoorService::Set_State (int building_id,int part_index,bool direction_is_opening,
												 float fraction_at_start,float start_time_seconds)
{
	int index = Find_Record (building_id,part_index);
	if (index < 0) {
		return ;
	}

	_Records [index].DirectionIsOpening	= direction_is_opening;
	_Records [index].FractionAtStart		= fraction_at_start;
	_Records [index].StartTimeSeconds	= start_time_seconds;
	return ;
}
