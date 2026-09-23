/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Commando/commandervotecastevent.cpp          $*
 *                                                                                             *
 *	Roadmap Section 25.1.  See commandervotecastevent.h.                                       *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "commandervotecastevent.h"

#include "apppackettypes.h"
#include "cnetwork.h"
#include "commanderproduction.h"
#include "gameobjmanager.h"
#include "networkobjectfactory.h"
#include "networkobjectmgr.h"
#include "soldier.h"


DECLARE_NETWORKOBJECT_FACTORY (cCommanderVoteCastEvent,NETCLASSID_COMMANDERVOTECASTEVENT);


cCommanderVoteCastEvent::cCommanderVoteCastEvent(void) :
	SenderId(0),
	YesVote(0)
{
	Set_App_Packet_Type (APPPACKETTYPE_COMMANDERVOTECASTEVENT);
}


void cCommanderVoteCastEvent::Init(bool yes_vote)
{
	WWASSERT (cNetwork::I_Am_Client ());

	SenderId	= cNetwork::Get_My_Id ();
	YesVote	= yes_vote ? 1 : 0;

	Set_Network_ID (NetworkObjectMgrClass::Get_New_Client_ID ());

	if (cNetwork::I_Am_Server ()) {
		Act ();
	} else {
		Set_Object_Dirty_Bit (0,BIT_CREATION,true);
	}
}


void cCommanderVoteCastEvent::Export_Creation(BitStreamClass & packet)
{
	WWASSERT (cNetwork::I_Am_Client ());

	cNetEvent::Export_Creation (packet);

	packet.Add (SenderId);
	packet.Add (YesVote);

	Set_Delete_Pending ();
}


void cCommanderVoteCastEvent::Import_Creation(BitStreamClass & packet)
{
	cNetEvent::Import_Creation (packet);

	WWASSERT (cNetwork::I_Am_Server ());

	packet.Get (SenderId);
	packet.Get (YesVote);

	Act ();
}


void cCommanderVoteCastEvent::Act(void)
{
	WWASSERT (cNetwork::I_Am_Server ());

	SoldierGameObj * soldier = GameObjManager::Find_Soldier_Of_Client_ID (SenderId);
	if (soldier == nullptr) return;

	int team = soldier->Get_Player_Type ();
	CommanderProductionService::Cast_Commander_Vote (team,SenderId,YesVote != 0);
}
