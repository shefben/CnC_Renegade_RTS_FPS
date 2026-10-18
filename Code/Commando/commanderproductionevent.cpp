/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Commando/commanderproductionevent.cpp        $*
 *                                                                                             *
 *	Roadmap Section 45, Phase 39 -- the client->server half of Commander production.            *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "commanderproductionevent.h"

#include "apppackettypes.h"
#include "building.h"
#include "buildingconstruction.h"
#include "buildingdescriptormanager.h"
#include "cnetwork.h"
#include "commanderproduction.h"
#include "commanderstate.h"
#include "gameobjmanager.h"
#include "networkobjectfactory.h"
#include "networkobjectmgr.h"
#include "playerdata.h"
#include "playertype.h"
#include "purchaseavailability.h"
#include "purchasesettings.h"
#include "scriptablegameobj.h"
#include "soldier.h"
#include "teampowergridservice.h"
#include "worldspatialindex.h"
#include "wwdebug.h"


DECLARE_NETWORKOBJECT_FACTORY (cCommanderProductionEvent,NETCLASSID_COMMANDERPRODUCTIONEVENT);


cCommanderProductionEvent::cCommanderProductionEvent(void) :
	SenderId(0),
	FacilityId(0),
	DefinitionId(0)
{
	Set_App_Packet_Type (APPPACKETTYPE_COMMANDERPRODUCTIONEVENT);
}


void cCommanderProductionEvent::Init(int facility_id,int definition_id)
{
	WWASSERT (cNetwork::I_Am_Client ());

	SenderId			= cNetwork::Get_My_Id ();
	FacilityId		= facility_id;
	DefinitionId	= definition_id;

	Set_Network_ID (NetworkObjectMgrClass::Get_New_Client_ID ());

	if (cNetwork::I_Am_Server ()) {
		Act ();
	} else {
		Set_Object_Dirty_Bit (0,BIT_CREATION,true);
	}
}


void cCommanderProductionEvent::Export_Creation(BitStreamClass & packet)
{
	WWASSERT (cNetwork::I_Am_Client ());

	cNetEvent::Export_Creation (packet);

	packet.Add (SenderId);
	packet.Add (FacilityId);
	packet.Add (DefinitionId);

	Set_Delete_Pending ();
}


void cCommanderProductionEvent::Import_Creation(BitStreamClass & packet)
{
	cNetEvent::Import_Creation (packet);

	WWASSERT (cNetwork::I_Am_Server ());

	packet.Get (SenderId);
	packet.Get (FacilityId);
	packet.Get (DefinitionId);

	Act ();
}


/***********************************************************************************************
 * Find_Purchase_Entry -- the cost and availability of a definition on this team's pages         *
 *                                                                                             *
 *	The price is looked up, never taken from the packet.  A client that named its own cost could *
 *	buy anything for one credit, which is the most obvious way a production request could be     *
 *	abused and therefore the one worth closing by construction.                                  *
 *=============================================================================================*/
static bool	Find_Purchase_Entry(int team,BuildingProductionKindType kind,int definition_id,
											int * out_cost,bool * out_available)
{
	PurchaseSettingsDefClass::TYPE type;
	switch (kind) {
		case BUILDING_PRODUCTION_INFANTRY:			type = PurchaseSettingsDefClass::TYPE_CLASSES;	break;
		case BUILDING_PRODUCTION_GROUND_VEHICLE:	type = PurchaseSettingsDefClass::TYPE_VEHICLES;	break;
		default:
			//	Air has no purchase page in this tree's content; see
			//	docs/assets/CommanderSidebar.md.  Refusing here is the honest answer.
			return false;
	}

	PurchaseSettingsDefClass::TEAM page_team = (team == PLAYERTYPE_GDI)
															? PurchaseSettingsDefClass::TEAM_GDI
															: PurchaseSettingsDefClass::TEAM_NOD;

	PurchaseSettingsDefClass * page = PurchaseSettingsDefClass::Find_Definition (type,page_team);
	if (page == nullptr) return false;

	for (int i = 0; i < PurchaseAvailabilityClass::ENTRY_COUNT; i++) {
		if (page->Get_Definition (i) != definition_id) continue;

		*out_cost = page->Get_Cost (i);

		//	Availability is the page's own replicated flags plus the tech level, which is exactly
		//	what the client's sidebar greyed the card out with.
		*out_available = (PurchaseAvailabilityClass::Get_Flags (type,page_team,i) == 0);
		return true;
	}
	return false;
}


void cCommanderProductionEvent::Act(void)
{
	WWASSERT (cNetwork::I_Am_Server ());

	SoldierGameObj * soldier = GameObjManager::Find_Soldier_Of_Client_ID (SenderId);
	if (soldier == nullptr) return;

	int team = soldier->Get_Player_Type ();

	/*
	**	The facility, resolved from its network id.  The two-step
	**	`As_ScriptableGameObj() -> As_BuildingGameObj()` is required rather than stylistic:
	**	`BuildingGameObj` inherits a second, unrelated `As_BuildingGameObj` from
	**	`CombatPhysObserverClass`, so calling it directly is ambiguous.
	*/
	ScriptableGameObj * scriptable = GameObjManager::Find_ScriptableGameObj (FacilityId);
	BuildingGameObj * building = (scriptable != nullptr) ? scriptable->As_BuildingGameObj () : nullptr;
	if (building == nullptr) return;

	//	An enemy or neutral building is never a Commander's factory, whatever the client says.
	if (building->Get_Player_Type () != team) return;

	/*
	**	Everything the production service needs to know about the facility, read live from the
	**	building itself -- which is what lets the service stay device-less and checkable while
	**	still being the single authority on whether a purchase is allowed.  The same call the
	**	per-tick refresh in combat.cpp makes, so a facility cannot be judged by two different
	**	rules depending on who asked.  A building with no production descriptor is not a factory.
	*/
	CommanderFacilityContext facility;
	if (!building->Build_Commander_Facility_Context (&facility)) return;

	//	Team comes from the context's own reading of the building, which the check above already
	//	confirmed matches the requester's.
	int cost = 0;
	bool available = false;
	if (!Find_Purchase_Entry (team,facility.Kind,DefinitionId,&cost,&available)) {
		return;
	}

	const BuildingModelDescriptor * descriptor = nullptr;
	const StringClass & descriptor_name = building->Get_Definition ().Get_Descriptor_Name ();
	if (descriptor_name.Get_Length () > 0) {
		descriptor = BuildingDescriptorManager::Get_Descriptor (descriptor_name.Peek_Buffer ());
	}
	if (descriptor == nullptr) return;

	PlayerDataClass * player = soldier->Get_Player_Data ();
	int credits = (player != nullptr) ? (int)player->Get_Money () : 0;

	int ticket = 0;
	CommanderProductionResultType result =
		CommanderProductionService::Request_Production (SenderId,facility,DefinitionId,cost,
																		descriptor->Get_Build_Time_Seconds (),
																		credits,available,&ticket);

	if (result != PRODUCTION_ACCEPTED) {
		WWDEBUG_SAY (("Commander production refused for client %d: %s\n",SenderId,
						  CommanderProductionService::Get_Result_Name (result)));
		return;
	}

	//	The service tracks cost but owns no wallet, so the debit happens here -- once, and only
	//	after every rule has passed.
	if (player != nullptr) {
		player->Increment_Money ((float)-cost);
	}

	CommanderStateClass::Changed ();
}
