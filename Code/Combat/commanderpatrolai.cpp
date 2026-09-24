/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commanderpatrolai.cpp                 $*
 *                                                                                             *
 *	Roadmap Section 45.  See commanderpatrolai.h.                                               *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "commanderpatrolai.h"

#include "actionparams.h"
#include "damageablegameobj.h"
#include "gameobjmanager.h"
#include "physicalgameobj.h"
#include "scriptcommands.h"
#include "vector.h"


const float	CommanderPatrolAI::ARRIVE_RADIUS			= 3.0f;
const float	CommanderPatrolAI::LEG_TIMEOUT_SECONDS	= 60.0f;

static const float	ORDER_PRIORITY = 200.0f;		// Dispatch_Order's own, so a patrol leg outranks idle behaviour

struct PatrolRecord
{
	PatrolRecord(void) : UnitId(0),PointA(0.0f,0.0f,0.0f),PointB(0.0f,0.0f,0.0f),Leg(1),SecondsOnLeg(0.0f) { }

	bool	operator == (const PatrolRecord &) const	{ return false; }
	bool	operator != (const PatrolRecord &) const	{ return true; }

	int		UnitId;
	Vector3	PointA;
	Vector3	PointB;
	int		Leg;				// 0 = heading to A, 1 = heading to B
	float		SecondsOnLeg;
};

static DynamicVectorClass<PatrolRecord>	_Patrols;


static int	Find_Patrol(int unit_id)
{
	for (int i = 0; i < _Patrols.Count (); i++) {
		if (_Patrols [i].UnitId == unit_id) return i;
	}
	return -1;
}


static void	Send_Leg(PhysicalGameObj * unit,const PatrolRecord & record)
{
	ActionParamsStruct params;
	params.Set_Basic (0,ORDER_PRIORITY,0);
	params.Set_Movement ((record.Leg == 0) ? record.PointA : record.PointB,1.0f,1.5f,false);
	ScriptEngine::Action_Goto (unit,params);
}


void CommanderPatrolAI::Init(void)		{ _Patrols.Delete_All (); }
void CommanderPatrolAI::Shutdown(void)	{ _Patrols.Delete_All (); }


void CommanderPatrolAI::Patrol(int unit_id,const Vector3 & point_a,const Vector3 & point_b)
{
	if (unit_id == 0) return;

	PatrolRecord record;
	record.UnitId	= unit_id;
	record.PointA	= point_a;
	record.PointB	= point_b;
	record.Leg		= 1;

	int index = Find_Patrol (unit_id);
	if (index >= 0) {
		_Patrols [index] = record;
	} else {
		_Patrols.Add (record);
	}

	PhysicalGameObj * unit = GameObjManager::Find_PhysicalGameObj (unit_id);
	if (unit != nullptr) {
		Send_Leg (unit,record);
	}
}


void CommanderPatrolAI::Stop_Patrolling(int unit_id)
{
	int index = Find_Patrol (unit_id);
	if (index >= 0) {
		_Patrols.Delete (index);
	}
}


bool	CommanderPatrolAI::Is_Patrolling(int unit_id)	{ return Find_Patrol (unit_id) >= 0; }
int	CommanderPatrolAI::Get_Active_Count(void)			{ return _Patrols.Count (); }


int CommanderPatrolAI::Next_Leg(const Vector3 & position,const Vector3 & point_a,const Vector3 & point_b,
										  int current_leg,float seconds_on_leg)
{
	const Vector3 & goal = (current_leg == 0) ? point_a : point_b;

	Vector3 delta = goal - position;
	delta.Z = 0.0f;

	bool arrived	= delta.Length2 () <= (ARRIVE_RADIUS * ARRIVE_RADIUS);
	bool timed_out	= seconds_on_leg >= LEG_TIMEOUT_SECONDS;

	return (arrived || timed_out) ? (1 - current_leg) : current_leg;
}


void CommanderPatrolAI::Update(float seconds)
{
	for (int i = _Patrols.Count () - 1; i >= 0; i--) {

		PatrolRecord & record = _Patrols [i];

		//	A dead or removed unit's patrol ends with it.
		PhysicalGameObj * unit = GameObjManager::Find_PhysicalGameObj (record.UnitId);
		DamageableGameObj * damageable = (unit != nullptr) ? (DamageableGameObj *)unit : nullptr;
		if ((unit == nullptr) || unit->Is_Delete_Pending () ||
			 (damageable->Get_Defense_Object () == nullptr) ||
			 (damageable->Get_Defense_Object ()->Get_Health () <= 0.0f)) {
			_Patrols.Delete (i);
			continue;
		}

		record.SecondsOnLeg += seconds;

		Vector3 position;
		unit->Get_Position (&position);

		int leg = Next_Leg (position,record.PointA,record.PointB,record.Leg,record.SecondsOnLeg);
		if (leg != record.Leg) {
			record.Leg				= leg;
			record.SecondsOnLeg	= 0.0f;
			Send_Leg (unit,record);
		}
	}
}
