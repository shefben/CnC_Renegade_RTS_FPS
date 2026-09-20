/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commanderguardai.cpp                  $*
 *                                                                                             *
 *	Roadmap Section 37/45 -- see the comment in commanderguardai.h.                             *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "commanderguardai.h"

#include "actionparams.h"
#include "damageablegameobj.h"
#include "gameobjmanager.h"
#include "guardstatecontroller.h"
#include "physicalgameobj.h"
#include "scriptablegameobj.h"
#include "scriptcommands.h"
#include "smartgameobj.h"
#include "soldier.h"
#include "vector.h"
#include "worldspatialindex.h"


//	Roadmap Section 3A.4/3A.6 -- how far out a guarding unit looks for a human to yield to.
//	Wide enough to catch a human about to path through the guard point, narrow enough that a
//	guard in one corner of the map does not scan for humans clear across it.
static const float		HUMAN_SCAN_RADIUS	= 40.0f;
static const float		ORDER_PRIORITY			= 100.0f;		// matches Dispatch_Order's own


struct CommanderGuardRecordStruct
{
	CommanderGuardRecordStruct(void) : UnitId(0) { }

	bool	operator == (const CommanderGuardRecordStruct &) const	{ return false; }
	bool	operator != (const CommanderGuardRecordStruct &) const	{ return true; }

	int							UnitId;
	GuardStateControllerClass	Controller;
};

static DynamicVectorClass<CommanderGuardRecordStruct *>		_Records;


static int Find_Record(int unit_id)
{
	for (int i = 0; i < _Records.Count(); i++) {
		if (_Records[i]->UnitId == unit_id) { return i; }
	}
	return -1;
}


void CommanderGuardAI::Init(void)
{
	Shutdown();
}


void CommanderGuardAI::Shutdown(void)
{
	for (int i = 0; i < _Records.Count(); i++) {
		_Records[i]->Controller.Deactivate();
		delete _Records[i];
	}
	_Records.Delete_All();
}


void CommanderGuardAI::Guard(int unit_id,const Vector3 & anchor,float radius)
{
	if (unit_id == 0) { return; }

	int index = Find_Record(unit_id);
	if (index < 0) {
		CommanderGuardRecordStruct * record = new CommanderGuardRecordStruct;
		record->UnitId = unit_id;
		_Records.Add(record);
		index = _Records.Count() - 1;
	}

	//	Activate is idempotent on an already-active controller -- it will not move the anchor --
	//	so a fresh guard order at a new point has to deactivate first.
	_Records[index]->Controller.Deactivate();
	_Records[index]->Controller.Activate(anchor,radius);
}


void CommanderGuardAI::Stop_Guarding(int unit_id)
{
	int index = Find_Record(unit_id);
	if (index < 0) { return; }

	_Records[index]->Controller.Deactivate();
	delete _Records[index];
	_Records.Delete(index);
}


bool CommanderGuardAI::Is_Guarding(int unit_id)
{
	int index = Find_Record(unit_id);
	return (index >= 0) && _Records[index]->Controller.Is_Active();
}


void CommanderGuardAI::Report_Damage(int unit_id,int attacker_id,const Vector3 & attacker_position,float now)
{
	int index = Find_Record(unit_id);
	if (index < 0) { return; }

	_Records[index]->Controller.Report_Damage(attacker_id,attacker_position,now);
}


int CommanderGuardAI::Get_Active_Count(void)
{
	return _Records.Count();
}


/*
**	Turns one controller's intent into the same ActionClass calls Dispatch_Order uses for
**	every other Commander order -- guard is a fifth verb reaching the same AI, not a second
**	AI layer standing next to it.
*/
static void Dispatch_Guard_Intent(PhysicalGameObj * unit,GuardAIIntentType intent,
											  const Vector3 & intent_position,int target_id)
{
	ActionParamsStruct params;
	params.Set_Basic(0,ORDER_PRIORITY,0);

	switch (intent) {

		case GUARD_INTENT_NONE:
			//	Nothing changed; the unit keeps doing whatever it was already doing.
			return;

		case GUARD_INTENT_STAND:
			ScriptEngine::Action_Reset(unit,ORDER_PRIORITY);
			break;

		case GUARD_INTENT_ATTACK: {
			ScriptableGameObj * target = (target_id != 0) ? GameObjManager::Find_ScriptableGameObj(target_id) : nullptr;
			if (target != nullptr) {
				params.Set_Attack(target,40.0f,0.0f,true);
			} else {
				params.Set_Attack(intent_position,40.0f,0.0f,true);
			}
			ScriptEngine::Action_Attack(unit,params);
			break;
		}

		case GUARD_INTENT_GOTO:
			params.Set_Movement(intent_position,1.0f,1.5f,false);
			ScriptEngine::Action_Goto(unit,params);
			break;
	}
}


/*
**	The one place this file resolves a GameObj at all outside the drive loop below: whether a
**	human is close enough to this guarding unit to yield to, and the closest one if so.  A
**	sphere query bounded to HUMAN_SCAN_RADIUS, not a walk of every player -- the same primitive
**	P25's blast radius and P30's own predecessor phases already use for "who is near this
**	point".
*/
static bool Find_Nearest_Human(const Vector3 & position,Vector3 * out_position,float * out_distance)
{
	GameObjQueryListClass candidates;
	WorldSpatialIndex::Query_Game_Objects_In_Sphere(position,HUMAN_SCAN_RADIUS,candidates,
																	 WorldSpatialIndex::QUERY_DYNAMIC);

	bool found = false;
	float best_distance2 = 0.0f;
	Vector3 best_position(0.0f,0.0f,0.0f);

	for (int i = 0; i < candidates.Count(); i++) {

		PhysicalGameObj * candidate = candidates[i];
		if (candidate == nullptr) { continue; }

		SoldierGameObj * soldier = candidate->As_SoldierGameObj();
		if ((soldier == nullptr) || (soldier->Get_Player_Data() == nullptr)) {
			continue;			// not a human-driven soldier
		}

		Vector3 soldier_pos;
		soldier->Get_Position(&soldier_pos);
		float distance2 = (soldier_pos - position).Length2();

		if (!found || (distance2 < best_distance2)) {
			found				= true;
			best_distance2	= distance2;
			best_position	= soldier_pos;
		}
	}

	if (found && (out_position != nullptr))	{ *out_position = best_position; }
	if (out_distance != nullptr)					{ *out_distance = found ? sqrtf(best_distance2) : -1.0f; }
	return found;
}


void CommanderGuardAI::Update(float now,float seconds)
{
	if (_Records.Count() == 0) { return; }

	GuardStateControllerClass::Begin_Tick();

	for (int i = _Records.Count() - 1; i >= 0; i--) {

		CommanderGuardRecordStruct * record = _Records[i];

		PhysicalGameObj * unit = GameObjManager::Find_PhysicalGameObj(record->UnitId);
		if (unit == nullptr) {
			//	The unit died or was removed without an order ever superseding the guard --
			//	CommanderPopulationObserverClass::Killed already told CommanderProductionService,
			//	this is just this file's own bookkeeping catching up.
			record->Controller.Deactivate();
			delete record;
			_Records.Delete(i);
			continue;
		}

		Vector3 position;
		unit->Get_Position(&position);

		//	PhysicalGameObj multiply-inherits As_DamageableGameObj from both DamageableGameObj
		//	and CombatPhysObserverClass -- ambiguous called directly, so the pointer is narrowed
		//	to the single DamageableGameObj base first, the same fix combat.cpp's own facility
		//	lookup uses.
		int target_id = record->Controller.Get_Current_Target_Id();
		bool target_valid = false;
		Vector3 target_position = position;
		if (target_id != 0) {
			ScriptableGameObj * target_scriptable = GameObjManager::Find_ScriptableGameObj(target_id);
			DamageableGameObj * target_damageable = (target_scriptable != nullptr)
																	  ? target_scriptable->As_DamageableGameObj() : nullptr;
			if ((target_damageable != nullptr) && (target_damageable->Get_Defense_Object() != nullptr) &&
				 (target_damageable->Get_Defense_Object()->Get_Health() > 0.0f)) {
				target_valid = true;
				target_damageable->Get_Position(&target_position);
			}
		}

		Vector3 human_position(0.0f,0.0f,0.0f);
		float human_distance = -1.0f;
		bool human_found = Find_Nearest_Human(position,&human_position,&human_distance);

		GuardAIIntentType intent = record->Controller.Update(now,seconds,position,
																				 target_valid,target_position,
																				 human_found ? &human_position : nullptr,
																				 human_distance);

		Dispatch_Guard_Intent(unit,intent,record->Controller.Get_Intent_Position(),
									  record->Controller.Get_Current_Target_Id());
	}
}
