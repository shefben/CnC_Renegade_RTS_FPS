/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Commando/commanderclaimevent.cpp             $*
 *                                                                                             *
 *	Roadmap Section 45.  See commanderclaimevent.h.                                             *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "commanderclaimevent.h"

#include "apppackettypes.h"
#include "cnetwork.h"
#include "commanderproduction.h"
#include "gameobjmanager.h"
#include "networkobjectfactory.h"
#include "networkobjectmgr.h"
#include "soldier.h"


DECLARE_NETWORKOBJECT_FACTORY (cCommanderClaimEvent,NETCLASSID_COMMANDERCLAIMEVENT);


cCommanderClaimEvent::cCommanderClaimEvent(void) :
	SenderId(0),
	WantCommand(0)
{
	Set_App_Packet_Type (APPPACKETTYPE_COMMANDERCLAIMEVENT);
}


void cCommanderClaimEvent::Init(bool want_command)
{
	WWASSERT (cNetwork::I_Am_Client ());

	SenderId		= cNetwork::Get_My_Id ();
	WantCommand	= want_command ? 1 : 0;

	Set_Network_ID (NetworkObjectMgrClass::Get_New_Client_ID ());

	if (cNetwork::I_Am_Server ()) {
		Act ();
	} else {
		Set_Object_Dirty_Bit (0,BIT_CREATION,true);
	}
}


void cCommanderClaimEvent::Export_Creation(BitStreamClass & packet)
{
	WWASSERT (cNetwork::I_Am_Client ());

	cNetEvent::Export_Creation (packet);

	packet.Add (SenderId);
	packet.Add (WantCommand);

	Set_Delete_Pending ();
}


void cCommanderClaimEvent::Import_Creation(BitStreamClass & packet)
{
	cNetEvent::Import_Creation (packet);

	WWASSERT (cNetwork::I_Am_Server ());

	packet.Get (SenderId);
	packet.Get (WantCommand);

	Act ();
}


/***********************************************************************************************
 * cCommanderClaimEvent::Act -- grant, refuse or release, on the server                          *
 *                                                                                               *
 *	The sender's team is looked up from their soldier rather than taken from the packet, for the  *
 *	same reason `cCommanderOrderEvent::Act` does it: a value a client supplies about which team   *
 *	it is on is a value a client can lie about, and this one decides who may spend the team's      *
 *	money.                                                                                         *
 *                                                                                               *
 *	First claimant wins.  A second player asking while someone else holds the job is refused and  *
 *	nothing changes -- deliberately silent rather than an error, because the client re-asks every  *
 *	time it opens Commander Mode and a refusal is the ordinary case, not a fault.  Releasing only  *
 *	clears the slot if the sender is the one holding it, so a player who never had it cannot take  *
 *	it away from whoever does.                                                                     *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void cCommanderClaimEvent::Act(void)
{
	WWASSERT (cNetwork::I_Am_Server ());

	SoldierGameObj * soldier = GameObjManager::Find_Soldier_Of_Client_ID (SenderId);
	if (soldier == nullptr) return;

	int team = soldier->Get_Player_Type ();
	int current = CommanderProductionService::Get_Team_Commander (team);

	if (WantCommand != 0) {

		if ((current == 0) || (current == SenderId)) {
			CommanderProductionService::Set_Team_Commander (team,SenderId);
		}

	} else if (current == SenderId) {
		CommanderProductionService::Set_Team_Commander (team,0);
	}
}
