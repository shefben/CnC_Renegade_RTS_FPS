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

#include "apppackettypes.h"
#include "cnetwork.h"
#include "commanderproduction.h"
#include "gameobjmanager.h"
#include "networkobjectfactory.h"
#include "networkobjectmgr.h"
#include "physicalgameobj.h"
#include "soldier.h"

#include <string.h>


DECLARE_NETWORKOBJECT_FACTORY (cCommanderOrderEvent,NETCLASSID_COMMANDERORDEREVENT);


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

		CommanderProductionService::Issue_Order (SenderId,unit_id,unit->Get_Player_Type (),
																 is_commander_ai,
																 (CommanderUnitOrderType)Order,
																 TargetPoint,TargetId);
	}
}
