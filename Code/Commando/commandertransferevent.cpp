/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Commando/commandertransferevent.cpp          $*
 *                                                                                             *
 *	Roadmap Section 25.1.  See commandertransferevent.h.                                       *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "commandertransferevent.h"

#include "apppackettypes.h"
#include "cnetwork.h"
#include "commanderproduction.h"
#include "gameobjmanager.h"
#include "networkobjectfactory.h"
#include "networkobjectmgr.h"
#include "soldier.h"


DECLARE_NETWORKOBJECT_FACTORY (cCommanderTransferEvent,NETCLASSID_COMMANDERTRANSFEREVENT);


cCommanderTransferEvent::cCommanderTransferEvent(void) :
	SenderId(0),
	TargetId(0)
{
	Set_App_Packet_Type (APPPACKETTYPE_COMMANDERTRANSFEREVENT);
}


void cCommanderTransferEvent::Init(int target_client_id)
{
	WWASSERT (cNetwork::I_Am_Client ());

	SenderId	= cNetwork::Get_My_Id ();
	TargetId	= target_client_id;

	Set_Network_ID (NetworkObjectMgrClass::Get_New_Client_ID ());

	if (cNetwork::I_Am_Server ()) {
		Act ();
	} else {
		Set_Object_Dirty_Bit (0,BIT_CREATION,true);
	}
}


void cCommanderTransferEvent::Export_Creation(BitStreamClass & packet)
{
	WWASSERT (cNetwork::I_Am_Client ());

	cNetEvent::Export_Creation (packet);

	packet.Add (SenderId);
	packet.Add (TargetId);

	Set_Delete_Pending ();
}


void cCommanderTransferEvent::Import_Creation(BitStreamClass & packet)
{
	cNetEvent::Import_Creation (packet);

	WWASSERT (cNetwork::I_Am_Server ());

	packet.Get (SenderId);
	packet.Get (TargetId);

	Act ();
}


/***********************************************************************************************
 * cCommanderTransferEvent::Act -- hand the job to a named teammate, server-validated            *
 *                                                                                               *
 *	`target_still_eligible` is the same real roster check `Update_Commander_Vote` uses for an     *
 *	ELECT candidate, computed here for the same reason: `Transfer_Commander` stays a pure         *
 *	function of ids and a bool, so a self check can exercise its refusal rules with no soldier.    *
 *=============================================================================================*/
void cCommanderTransferEvent::Act(void)
{
	WWASSERT (cNetwork::I_Am_Server ());

	SoldierGameObj * soldier = GameObjManager::Find_Soldier_Of_Client_ID (SenderId);
	if (soldier == nullptr) return;

	int team = soldier->Get_Player_Type ();
	bool target_still_eligible = CommanderProductionService::Is_Client_Connected_On_Team (TargetId,team);

	CommanderProductionService::Transfer_Commander (team,SenderId,TargetId,target_still_eligible);
}
