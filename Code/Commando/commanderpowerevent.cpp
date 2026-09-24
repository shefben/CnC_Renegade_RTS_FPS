/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Commando/commanderpowerevent.cpp             $*
 *                                                                                             *
 *	Roadmap Section 42.  See commanderpowerevent.h.                                             *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "commanderpowerevent.h"

#include "apppackettypes.h"
#include "building.h"
#include "cnetwork.h"
#include "commanderproduction.h"
#include "commanderstate.h"
#include "gameobjmanager.h"
#include "networkobjectfactory.h"
#include "networkobjectmgr.h"
#include "soldier.h"
#include "teampowergridservice.h"

DECLARE_NETWORKOBJECT_FACTORY (cCommanderPowerEvent,NETCLASSID_COMMANDERPOWEREVENT);

//	The descriptor schema only refuses a negative priority; this is the ceiling a panel click
//	can reach, well above anything a shipped descriptor uses (100).
static const int	MAX_PANEL_PRIORITY = 1000;

cCommanderPowerEvent::cCommanderPowerEvent(void) :
	SenderId(0),
	BuildingId(0),
	Action(ACTION_SET_ENABLED),
	Value(0)
{
	Set_App_Packet_Type (APPPACKETTYPE_COMMANDERPOWEREVENT);
}

void cCommanderPowerEvent::Init(int building_id,int action,int value)
{
	WWASSERT (cNetwork::I_Am_Client ());

	SenderId		= cNetwork::Get_My_Id ();
	BuildingId	= building_id;
	Action		= action;
	Value			= value;

	Set_Network_ID (NetworkObjectMgrClass::Get_New_Client_ID ());

	if (cNetwork::I_Am_Server ()) {
		Act ();
	} else {
		Set_Object_Dirty_Bit (0,BIT_CREATION,true);
	}
}

void cCommanderPowerEvent::Export_Creation(BitStreamClass & packet)
{
	WWASSERT (cNetwork::I_Am_Client ());

	cNetEvent::Export_Creation (packet);

	packet.Add (SenderId);
	packet.Add (BuildingId);
	packet.Add (Action);
	packet.Add (Value);

	Set_Delete_Pending ();
}

void cCommanderPowerEvent::Import_Creation(BitStreamClass & packet)
{
	cNetEvent::Import_Creation (packet);

	WWASSERT (cNetwork::I_Am_Server ());

	packet.Get (SenderId);
	packet.Get (BuildingId);
	packet.Get (Action);
	packet.Get (Value);

	Act ();
}

/***********************************************************************************************
 * cCommanderPowerEvent::Act -- apply one panel click, server-validated                          *
 *                                                                                             *
 *	The same authority test every Commander event uses (the sender is the team's recorded         *
 *	Commander), plus the building's own team, so a client cannot switch off the enemy's base.     *
 *=============================================================================================*/
void cCommanderPowerEvent::Act(void)
{
	WWASSERT (cNetwork::I_Am_Server ());

	SoldierGameObj * soldier = GameObjManager::Find_Soldier_Of_Client_ID (SenderId);
	if (soldier == nullptr) return;

	int team = soldier->Get_Player_Type ();
	if (CommanderProductionService::Get_Team_Commander (team) != SenderId) return;

	ScriptableGameObj * scriptable = GameObjManager::Find_ScriptableGameObj (BuildingId);
	BuildingGameObj * building = (scriptable != nullptr) ? scriptable->As_BuildingGameObj () : nullptr;
	if ((building == nullptr) || building->Is_Destroyed ()) return;
	if (building->Get_Player_Type () != team) return;

	switch (Action) {

		case ACTION_SET_ENABLED:
			TeamPowerGridService::Set_Manual_Disabled (BuildingId,Value == 0);
			break;

		case ACTION_SET_PRIORITY:
		{
			int priority = Value;
			if (priority < 0) priority = 0;
			if (priority > MAX_PANEL_PRIORITY) priority = MAX_PANEL_PRIORITY;
			TeamPowerGridService::Set_Priority_Override (BuildingId,priority);
			break;
		}

		case ACTION_CLEAR_PRIORITY:
			TeamPowerGridService::Clear_Priority_Override (BuildingId);
			break;

		default:
			return;
	}

	//	The panel reads the per-building overrides from the Commander mirror; mark it dirty
	//	now rather than waiting for a team total to change, since toggling a building whose
	//	demand is already unmet changes no total at all.
	CommanderStateClass::Changed ();
}
