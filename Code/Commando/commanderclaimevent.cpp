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
 * cCommanderClaimEvent::Act -- grant, refuse, vote or release, on the server                    *
 *                                                                                               *
 *	The sender's team is looked up from their soldier rather than taken from the packet, for the  *
 *	same reason `cCommanderOrderEvent::Act` does it: a value a client supplies about which team   *
 *	it is on is a value a client can lie about, and this one decides who may spend the team's      *
 *	money.                                                                                         *
 *                                                                                               *
 *	Roadmap Section 25.1: "No first-player-wins permanent rule."  A vacant seat with exactly one  *
 *	connected teammate still grants instantly -- that is not a rule so much as the absence of a    *
 *	contest, and it is what keeps a solo or skirmish match's Commander assignment costing zero     *
 *	latency, the invariant this class's own header documents.  A vacant seat with more than one     *
 *	connected teammate instead opens a real Section 25.1 "initial volunteer/candidate period":      *
 *	the claimant becomes an ELECT candidate and the team gets `COMMANDER_VOTE_WINDOW_SECONDS` to    *
 *	object via `commander_vote`, not an instant, undisputable grant.  A second claim arriving       *
 *	while that vote is already open is silently ignored, same as the old "someone already holds     *
 *	it" refusal -- the right response to a live vote is to vote on it, not to file a second claim.  *
 *                                                                                               *
 *	Releasing only clears the slot if the sender is the one holding it, so a player who never had   *
 *	it cannot take it away from whoever does -- unchanged, and still the only way to give the job    *
 *	up instantly; removing someone else's command now goes through `commander_vote remove`.         *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void cCommanderClaimEvent::Act(void)
{
	WWASSERT (cNetwork::I_Am_Server ());

	SoldierGameObj * soldier = GameObjManager::Find_Soldier_Of_Client_ID (SenderId);
	if (soldier == nullptr) return;

	int team = soldier->Get_Player_Type ();
	int current = CommanderProductionService::Get_Team_Commander (team);

	if (WantCommand != 0) {

		if (current == SenderId) {
			//	Already the Commander; nothing to do.  Kept as its own branch rather than
			//	folding into the block below so a re-claim of a seat already held never starts
			//	a pointless vote of one.

		} else if (current == 0) {

			int eligible = CommanderProductionService::Count_Connected_Teammates (team);
			if (eligible <= 1) {
				CommanderProductionService::Set_Team_Commander (team,SenderId);
			} else {
				CommanderProductionService::Call_Commander_Vote (
					team,SenderId,COMMANDER_VOTE_ELECT,SenderId,eligible);
			}
		}
		//	else: seat is held by someone else -- refused, silently, as before.

	} else if (current == SenderId) {
		CommanderProductionService::Set_Team_Commander (team,0);
	}
}
