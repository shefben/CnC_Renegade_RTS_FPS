/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Commando/commanderplacementevent.cpp         $*
 *                                                                                             *
 *	Roadmap Section 43, Phase 37 -- the client->server half of Commander building placement.    *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "commanderplacementevent.h"

#include "apppackettypes.h"
#include "buildingconstruction.h"
#include "buildingdescriptormanager.h"
#include "cnetwork.h"
#include "commanderplacement.h"
#include "commanderproduction.h"
#include "commanderstate.h"
#include "gameobjmanager.h"
#include "networkobjectfactory.h"
#include "networkobjectmgr.h"
#include "playerdata.h"
#include "playertype.h"
#include "soldier.h"
#include "timemgr.h"
#include "wwdebug.h"

#include <stdlib.h>


DECLARE_NETWORKOBJECT_FACTORY (cCommanderPlacementEvent,NETCLASSID_COMMANDERPLACEMENTEVENT);


cCommanderPlacementEvent::cCommanderPlacementEvent(void) :
	SenderId(0),
	DescriptorName(""),
	Position(0.0f,0.0f,0.0f),
	RotationDegrees(0.0f),
	SequenceId(0)
{
	Set_App_Packet_Type (APPPACKETTYPE_COMMANDERPLACEMENTEVENT);
}


void cCommanderPlacementEvent::Init(const char * descriptor_name,const Vector3 & position,
												float rotation_degrees,int sequence_id)
{
	WWASSERT (cNetwork::I_Am_Client ());

	SenderId			= cNetwork::Get_My_Id ();
	DescriptorName	= (descriptor_name != nullptr) ? descriptor_name : "";
	Position			= position;
	RotationDegrees	= rotation_degrees;
	SequenceId		= sequence_id;

	Set_Network_ID (NetworkObjectMgrClass::Get_New_Client_ID ());

	if (cNetwork::I_Am_Server ()) {
		Act ();
	} else {
		Set_Object_Dirty_Bit (0,BIT_CREATION,true);
	}
}


void cCommanderPlacementEvent::Export_Creation(BitStreamClass & packet)
{
	WWASSERT (cNetwork::I_Am_Client ());

	cNetEvent::Export_Creation (packet);

	packet.Add (SenderId);
	packet.Add_Terminated_String (DescriptorName.Peek_Buffer (),true);
	packet.Add (Position.X);
	packet.Add (Position.Y);
	packet.Add (Position.Z);
	packet.Add (RotationDegrees);
	packet.Add (SequenceId);

	Set_Delete_Pending ();
}


void cCommanderPlacementEvent::Import_Creation(BitStreamClass & packet)
{
	cNetEvent::Import_Creation (packet);

	WWASSERT (cNetwork::I_Am_Server ());

	char name [128] = { 0 };

	packet.Get (SenderId);
	packet.Get_Terminated_String (name,sizeof (name),true);
	packet.Get (Position.X);
	packet.Get (Position.Y);
	packet.Get (Position.Z);
	packet.Get (RotationDegrees);
	packet.Get (SequenceId);

	DescriptorName = name;

	Act ();
}


void cCommanderPlacementEvent::Act(void)
{
	WWASSERT (cNetwork::I_Am_Server ());

	/*
	**	The team comes from the sender's own soldier, never from the packet.  A client that could
	**	name its own team could place Nod buildings for GDI, and Section 43's "never trust client
	**	green/red state" is really an instance of the broader rule that a client names nothing a
	**	server can look up.
	*/
	SoldierGameObj * soldier = GameObjManager::Find_Soldier_Of_Client_ID (SenderId);
	if (soldier == nullptr) {
		return;
	}

	int team = soldier->Get_Player_Type ();

	//	Commander authority, checked here rather than trusted: only a team's Commander places.
	if (CommanderProductionService::Get_Team_Commander (team) != SenderId) {
		return;
	}

	CommanderPlacementRequest request;
	request.DescriptorName	= DescriptorName;
	request.Position			= Position;
	request.RotationDegrees	= RotationDegrees;
	request.Team				= team;
	request.SequenceId		= SequenceId;

	//	The same function the client's ghost colours itself with.  An honest client is never
	//	refused here, and a lying one gains nothing.
	CommanderPlacementResult result = CommanderPlacementValidator::Validate_Server_Request (request);
	if (!result.Is_Valid ()) {
		WWDEBUG_SAY (("Commander placement refused for client %d: %s\n",SenderId,
						  CommanderPlacementValidator::Get_Result_Name (result.Result)));
		return;
	}

	const BuildingModelDescriptor * descriptor =
		BuildingDescriptorManager::Get_Descriptor (DescriptorName.Peek_Buffer ());
	if (descriptor == nullptr) {
		return;			// Validate already refused this case; belt and braces
	}

	/*
	**	Charge the Commander.  Done before the construction starts, and only on success, so a
	**	refused placement never costs anything and an accepted one cannot be free.
	*/
	PlayerDataClass * player = soldier->Get_Player_Data ();
	if (player != nullptr) {
		if (player->Get_Money () < (float)descriptor->Build.Cost) {
			return;
		}
		player->Increment_Money ((float)-descriptor->Build.Cost);
	}

	Matrix3D transform (1);
	CommanderPlacementValidator::Build_Placement_Transform (request,result,&transform);

	/*
	**	The seed the whole procedural build-up is generated from.  Chosen here, on the server,
	**	once -- which is why `Begin_Construction` and `Begin_Construction_From_State` are separate
	**	entry points: only one of them is allowed to pick it.
	*/
	unsigned int seed = (unsigned int)(SequenceId * 2654435761u) ^
							  (unsigned int)(TimeManager::Get_Total_Seconds () * 1000.0f);

	/*
	**	Model bounds for the block layout.  The descriptor's footprint gives width and depth; the
	**	height has no home in Section 39's schema, so it is derived from the footprint.  A real
	**	caller with the prototype resident should pass the model's own bounds instead -- this is
	**	the honest stand-in until the creation path that instantiates the building exists, and it
	**	is recorded in WIP_features.md rather than pretended away.
	*/
	AABoxClass bounds;
	bounds.Extent = Vector3 (descriptor->Footprint.Width * 0.5f,
									 descriptor->Footprint.Depth * 0.5f,
									 descriptor->Footprint.Width * 0.35f);
	bounds.Center = Vector3 (0.0f,0.0f,bounds.Extent.Z);

	/*
	**	The building id.  A real building object does not exist yet -- creating the canonical
	**	`BuildingGameObj` in CONSTRUCTING state is the remaining half of Phase 38 -- so the
	**	construction is keyed by the placement sequence for now.  Negative, so it can never
	**	collide with a real network object id once that half lands.
	*/
	int building_id = -(SequenceId + 1);

	BuildingConstructionService::Begin_Construction (building_id,DescriptorName.Peek_Buffer (),
																	 transform,bounds,
																	 TimeManager::Get_Total_Seconds (),seed);

	//	Tell every client at once: the construction list just changed.
	CommanderStateClass::Changed ();
}
