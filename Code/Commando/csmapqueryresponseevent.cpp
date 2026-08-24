/******************************************************************************
*
* FILE
*     csmapqueryresponseevent.cpp
*
* DESCRIPTION
*     See csmapqueryresponseevent.h.
*
******************************************************************************/

#include "csmapqueryresponseevent.h"

#include "apppackettypes.h"
#include "cnetwork.h"
#include "consolefunction.h"
#include "networkobjectfactory.h"
#include "networkobjectmgr.h"
#include "player.h"
#include "playermanager.h"

DECLARE_NETWORKOBJECT_FACTORY(cCsMapQueryResponseEvent, NETCLASSID_CSMAPQUERYRESPONSEEVENT);


//-----------------------------------------------------------------------------
cCsMapQueryResponseEvent::cCsMapQueryResponseEvent (void)
{
	ClientId	= -1;
	MapName	= "";
	HasMap	= false;

	Set_App_Packet_Type (APPPACKETTYPE_CSMAPQUERYRESPONSEEVENT);
}


//-----------------------------------------------------------------------------
void	cCsMapQueryResponseEvent::Init (const StringClass &map_name, bool has_map)
{
	WWASSERT (cNetwork::I_Am_Client ());

	ClientId	= cNetwork::Get_My_Id ();
	MapName	= map_name;
	HasMap	= has_map;

	if (MapName.Get_Length () >= MAX_MAP_NAME_LENGTH) {
		MapName.Erase (MAX_MAP_NAME_LENGTH - 1, MapName.Get_Length () - (MAX_MAP_NAME_LENGTH - 1));
	}

	Set_Network_ID (NetworkObjectMgrClass::Get_New_Client_ID ());

	if (cNetwork::I_Am_Server ()) {
		Act ();
	} else {
		Set_Object_Dirty_Bit (0, BIT_CREATION, true);
	}

	return ;
}


//-----------------------------------------------------------------------------
void	cCsMapQueryResponseEvent::Act (void)
{
	WWASSERT (cNetwork::I_Am_Server ());

	//
	//	Name the player as well as the id.  Whoever typed the question is reading
	//	the answer some seconds later and will not remember which id was which.
	//
	StringClass	name;
	cPlayer *	player = cPlayerManager::Find_Player (ClientId);
	if (player != nullptr) {
		player->Get_Name ().Convert_To (name);
	}

	ConsoleFunctionClass::Print ("Player %d (%s) %s the map %s\n",
		ClientId,
		name.Peek_Buffer (),
		HasMap ? "has" : "does not have",
		MapName.Peek_Buffer ());

	Set_Delete_Pending ();
	return ;
}


//-----------------------------------------------------------------------------
void	cCsMapQueryResponseEvent::Export_Creation (BitStreamClass &packet)
{
	WWASSERT (cNetwork::I_Am_Only_Client ());

	cNetEvent::Export_Creation (packet);

	packet.Add (ClientId);
	packet.Add (HasMap);
	packet.Add_Terminated_String (MapName.Peek_Buffer (), true);

	Set_Delete_Pending ();
	return ;
}


//-----------------------------------------------------------------------------
void	cCsMapQueryResponseEvent::Import_Creation (BitStreamClass &packet)
{
	WWASSERT (cNetwork::I_Am_Server ());

	cNetEvent::Import_Creation (packet);

	packet.Get (ClientId);
	packet.Get (HasMap);
	packet.Get_Terminated_String (MapName.Get_Buffer (MAX_MAP_NAME_LENGTH), MAX_MAP_NAME_LENGTH, true);

	Act ();
	return ;
}
