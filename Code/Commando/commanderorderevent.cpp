/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Commando/commanderorderevent.cpp             $*
 *                                                                                             *
 *	Roadmap Section 45, Phase 39 -- the client->server half of Commander unit orders.           *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "commanderorderevent.h"

#include "action.h"
#include "actionparams.h"
#include "apppackettypes.h"
#include "cnetwork.h"
#include "commanderguardai.h"
#include "commanderproduction.h"
#include "gameobjmanager.h"
#include "networkobjectfactory.h"
#include "networkobjectmgr.h"
#include "physicalgameobj.h"
#include "scriptcommands.h"
#include "smartgameobj.h"
#include "soldier.h"

#include <string.h>


DECLARE_NETWORKOBJECT_FACTORY (cCommanderOrderEvent,NETCLASSID_COMMANDERORDEREVENT);


/*
**	Section 45's own translation of an accepted order into canonical Renegade AI, so
**	"Commander-purchased infantry are normal AI SoldierGameObj paths" holds all the way
**	through: CommanderProductionService only records that an order was issued, and this is
**	the one place that turns the recorded verb into an ActionClass call, using the exact
**	same free functions Code/Combat/scriptcommands.cpp gives mission scripts.  Priority 100
**	matches every scripted command in this tree (Mission00.cpp, DrMobius.cpp, ...) -- high
**	enough to override a unit's own idle/innate behaviour, which is the entire point of a
**	Commander order.  Observer id 0 means nothing is listening for completion, which
**	ActionClass::Notify_Completed already treats as a safe no-op.
*/
static void	Dispatch_Order (PhysicalGameObj * unit,CommanderUnitOrderType order,
									  const Vector3 & target_point,int target_id)
{
	const float ORDER_PRIORITY = 100.0f;
	const float GUARD_RADIUS	 = 15.0f;

	SmartGameObj * smart = unit->As_SmartGameObj ();
	if (smart == nullptr) return;		// no AI action layer on this object -- nothing to drive

	if (order != ORDER_NONE) {
		//	Any fresh order, ORDER_GUARD included, starts from a clean slate: without this, a
		//	unit given MOVE after being told to guard would keep fighting its own leftover
		//	GuardStateControllerClass, which drives ActionClass every server tick regardless of
		//	whatever this dispatch just set.  A no-op for a unit that was not guarding.
		CommanderGuardAI::Stop_Guarding (unit->Get_ID ());
	}

	ActionParamsStruct params;
	params.Set_Basic (0,ORDER_PRIORITY,0);

	switch (order) {

		case ORDER_NONE:
			//	Not a command -- nothing to dispatch.  Leaves whatever the unit was already
			//	doing alone, rather than stopping it.
			return;

		case ORDER_MOVE:
		case ORDER_ATTACK_MOVE:
			//	Attack-move relies on this engine's own perception/threat response -- a
			//	SmartGameObj already breaks off to engage a target it perceives regardless of
			//	its current action -- rather than a second attack-move state machine this
			//	codebase has never had.
			params.Set_Movement (target_point,1.0f,1.5f,false);
			ScriptEngine::Action_Goto (unit,params);
			break;

		case ORDER_ATTACK_TARGET: {
			ScriptableGameObj * target = (target_id != 0) ? GameObjManager::Find_ScriptableGameObj (target_id) : nullptr;
			if (target != nullptr) {
				params.Set_Attack (target,40.0f,0.0f,true);
			} else {
				params.Set_Attack (target_point,40.0f,0.0f,true);
			}
			ScriptEngine::Action_Attack (unit,params);
			break;
		}

		case ORDER_FOLLOW: {
			ScriptableGameObj * target = (target_id != 0) ? GameObjManager::Find_ScriptableGameObj (target_id) : nullptr;
			if (target != nullptr) {
				params.Set_Movement (target,1.0f,3.0f,false);
				ScriptEngine::Action_Follow_Input (unit,params);
			} else {
				ScriptEngine::Action_Reset (unit,ORDER_PRIORITY);
			}
			break;
		}

		case ORDER_PATROL:
			//	The wire format this event carries (Section 45's own struct) has one point, not
			//	a waypath -- a real back-and-forth patrol needs a second point nothing here has.
			//	Walking to the one point given is strictly more than the nothing this order did
			//	before; recorded as a limit, not silently upgraded into a loop it cannot run.
			params.Set_Movement (target_point,1.0f,1.5f,false);
			ScriptEngine::Action_Goto (unit,params);
			break;

		case ORDER_REPAIR_TARGET:
		case ORDER_RETREAT_TO_REPAIR:
			//	Neither is a distinct AI action in this codebase -- RepairBayGameObj repairs by
			//	proximity once a vehicle sits inside it -- so both orders walk to the named
			//	point rather than inventing a canonical "repair" primitive that does not exist.
			params.Set_Movement (target_point,1.0f,1.5f,false);
			ScriptEngine::Action_Goto (unit,params);
			break;

		case ORDER_GUARD:
			//	CommanderGuardAI (Code/Combat/commanderguardai.h) owns the state machine from
			//	here -- Section 37's GuardStateControllerClass, activated at the clicked point,
			//	ticked once a server frame in CombatManager::Think, translating its own intent
			//	into the same ScriptEngine calls every other case here uses.  Nothing further to
			//	dispatch this instant; the first Attack/Goto/Reset call comes from that tick.
			CommanderGuardAI::Guard (unit->Get_ID (),target_point,GUARD_RADIUS);
			break;

		case ORDER_HOLD_POSITION:
		default:
			ScriptEngine::Action_Reset (unit,ORDER_PRIORITY);
			break;
	}
}


cCommanderOrderEvent::cCommanderOrderEvent(void) :
	SenderId(0),
	Order(0),
	TargetPoint(0.0f,0.0f,0.0f),
	TargetId(0),
	UnitCount(0)
{
	::memset (UnitIds,0,sizeof (UnitIds));
	Set_App_Packet_Type (APPPACKETTYPE_COMMANDERORDEREVENT);
}


void cCommanderOrderEvent::Init(int order,const Vector3 & target_point,int target_id,
										  const int * unit_ids,int unit_count)
{
	WWASSERT (cNetwork::I_Am_Client ());

	SenderId		= cNetwork::Get_My_Id ();
	Order			= order;
	TargetPoint	= target_point;
	TargetId		= target_id;

	if (unit_count < 0) unit_count = 0;
	if (unit_count > MAX_UNITS) unit_count = MAX_UNITS;
	UnitCount = unit_count;

	for (int i = 0; i < UnitCount; i++) {
		UnitIds [i] = (unit_ids != nullptr) ? unit_ids [i] : 0;
	}

	Set_Network_ID (NetworkObjectMgrClass::Get_New_Client_ID ());

	if (cNetwork::I_Am_Server ()) {
		Act ();
	} else {
		Set_Object_Dirty_Bit (0,BIT_CREATION,true);
	}
}


void cCommanderOrderEvent::Export_Creation(BitStreamClass & packet)
{
	WWASSERT (cNetwork::I_Am_Client ());

	cNetEvent::Export_Creation (packet);

	packet.Add (SenderId);
	packet.Add (Order);
	packet.Add (TargetPoint.X);
	packet.Add (TargetPoint.Y);
	packet.Add (TargetPoint.Z);
	packet.Add (TargetId);
	packet.Add (UnitCount);
	for (int i = 0; i < UnitCount; i++) {
		packet.Add (UnitIds [i]);
	}

	Set_Delete_Pending ();
}


void cCommanderOrderEvent::Import_Creation(BitStreamClass & packet)
{
	cNetEvent::Import_Creation (packet);

	WWASSERT (cNetwork::I_Am_Server ());

	packet.Get (SenderId);
	packet.Get (Order);
	packet.Get (TargetPoint.X);
	packet.Get (TargetPoint.Y);
	packet.Get (TargetPoint.Z);
	packet.Get (TargetId);
	packet.Get (UnitCount);

	//	Clamped before it is used as a loop bound, not after: a hostile packet naming a hundred
	//	units must read exactly MAX_UNITS ids or the stream would desync for everything after it.
	if (UnitCount < 0) UnitCount = 0;
	if (UnitCount > MAX_UNITS) UnitCount = MAX_UNITS;

	for (int i = 0; i < UnitCount; i++) {
		packet.Get (UnitIds [i]);
	}

	Act ();
}


void cCommanderOrderEvent::Act(void)
{
	WWASSERT (cNetwork::I_Am_Server ());

	SoldierGameObj * soldier = GameObjManager::Find_Soldier_Of_Client_ID (SenderId);
	if (soldier == nullptr) return;

	int team = soldier->Get_Player_Type ();
	if (CommanderProductionService::Get_Team_Commander (team) != SenderId) {
		return;
	}

	for (int i = 0; i < UnitCount; i++) {

		int unit_id = UnitIds [i];
		if (unit_id == 0) continue;

		PhysicalGameObj * unit = GameObjManager::Find_PhysicalGameObj (unit_id);
		if (unit == nullptr) continue;			// died between the click and the packet

		/*
		**	Whether this unit is Commander AI rather than a human's, looked up rather than taken
		**	from the packet.  A soldier with a controlling player behind it is a human-driven
		**	unit and refuses every Commander order -- Section 3A.4's priority rule, enforced at
		**	the one place an order can enter the server.
		**
		**	Commander-produced AI has no controlling player, which is exactly what distinguishes
		**	it; Section 45's "AI consumes no fake human player slots" is what makes that test
		**	sound rather than a heuristic.
		*/
		bool is_commander_ai = true;

		SoldierGameObj * unit_soldier = unit->As_SoldierGameObj ();
		if ((unit_soldier != nullptr) && (unit_soldier->Get_Player_Data () != nullptr)) {
			is_commander_ai = false;
		}

		bool accepted = CommanderProductionService::Issue_Order (SenderId,unit_id,unit->Get_Player_Type (),
																					is_commander_ai,
																					(CommanderUnitOrderType)Order,
																					TargetPoint,TargetId);

		//	The service records the order regardless of caller; only an *accepted* one drives
		//	the actual AI -- refused orders (wrong team, human-driven unit) must change nothing
		//	about what the unit is doing, the same as if the packet had never arrived.
		if (accepted) {
			Dispatch_Order (unit,(CommanderUnitOrderType)Order,TargetPoint,TargetId);
		}
	}
}
