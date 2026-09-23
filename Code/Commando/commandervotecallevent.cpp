/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Commando/commandervotecallevent.cpp          $*
 *                                                                                             *
 *	Roadmap Section 25.1.  See commandervotecallevent.h.                                       *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "commandervotecallevent.h"

#include "apppackettypes.h"
#include "cnetwork.h"
#include "commanderproduction.h"
#include "gameobjmanager.h"
#include "networkobjectfactory.h"
#include "networkobjectmgr.h"
#include "soldier.h"


DECLARE_NETWORKOBJECT_FACTORY (cCommanderVoteCallEvent,NETCLASSID_COMMANDERVOTECALLEVENT);


cCommanderVoteCallEvent::cCommanderVoteCallEvent(void) :
	SenderId(0),
	IsRemoveVote(0),
	CandidateId(0)
{
	Set_App_Packet_Type (APPPACKETTYPE_COMMANDERVOTECALLEVENT);
}


void cCommanderVoteCallEvent::Init(bool is_remove_vote,int candidate_id)
{
	WWASSERT (cNetwork::I_Am_Client ());

	SenderId			= cNetwork::Get_My_Id ();
	IsRemoveVote	= is_remove_vote ? 1 : 0;
	CandidateId		= candidate_id;

	Set_Network_ID (NetworkObjectMgrClass::Get_New_Client_ID ());

	if (cNetwork::I_Am_Server ()) {
		Act ();
	} else {
		Set_Object_Dirty_Bit (0,BIT_CREATION,true);
	}
}


void cCommanderVoteCallEvent::Export_Creation(BitStreamClass & packet)
{
	WWASSERT (cNetwork::I_Am_Client ());

	cNetEvent::Export_Creation (packet);

	packet.Add (SenderId);
	packet.Add (IsRemoveVote);
	packet.Add (CandidateId);

	Set_Delete_Pending ();
}


void cCommanderVoteCallEvent::Import_Creation(BitStreamClass & packet)
{
	cNetEvent::Import_Creation (packet);

	WWASSERT (cNetwork::I_Am_Server ());

	packet.Get (SenderId);
	packet.Get (IsRemoveVote);
	packet.Get (CandidateId);

	Act ();
}


/***********************************************************************************************
 * cCommanderVoteCallEvent::Act -- resolve the sender's team, then ask the service to open one   *
 *                                                                                               *
 *	Team is read from the sender's own soldier, the same reason every Commander event does it     *
 *	this way -- a client's own claim about its team is a claim a client can lie about.  The       *
 *	eligible-voter count is the one real roster walk this whole governance layer does, computed   *
 *	here rather than inside the service so `CommanderProductionService`'s vote math stays a pure   *
 *	function of plain integers -- see `Count_Connected_Teammates`'s own header comment.            *
 *=============================================================================================*/
void cCommanderVoteCallEvent::Act(void)
{
	WWASSERT (cNetwork::I_Am_Server ());

	SoldierGameObj * soldier = GameObjManager::Find_Soldier_Of_Client_ID (SenderId);
	if (soldier == nullptr) return;

	int team = soldier->Get_Player_Type ();
	int eligible = CommanderProductionService::Count_Connected_Teammates (team);

	CommanderVoteType type = (IsRemoveVote != 0) ? COMMANDER_VOTE_REMOVE : COMMANDER_VOTE_ELECT;

	CommanderProductionService::Call_Commander_Vote (team,SenderId,type,CandidateId,eligible);
}
