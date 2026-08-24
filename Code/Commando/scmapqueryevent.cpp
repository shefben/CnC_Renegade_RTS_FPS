/******************************************************************************
*
* FILE
*     scmapqueryevent.cpp
*
* DESCRIPTION
*     See scmapqueryevent.h.
*
******************************************************************************/

#include "scmapqueryevent.h"

#include "apppackettypes.h"
#include "cnetwork.h"
#include "csmapqueryresponseevent.h"
#include "networkobjectfactory.h"
#include "rawfile.h"

#include <stdio.h>

DECLARE_NETWORKOBJECT_FACTORY(cScMapQueryEvent, NETCLASSID_SCMAPQUERYEVENT);


/*
**	The same two step the server uses on its own map cycle: the name as given,
**	then the name under data/.  A client answering by a different rule than the
**	server checks by would make the answer worth less than not asking.
*/
static bool	Map_Is_Present (const StringClass &map_name)
{
	if (map_name.Is_Empty ()) {
		return false;
	}

	RawFileClass file (map_name.Peek_Buffer ());
	if (file.Is_Available ()) {
		return true;
	}

	StringClass under_data;
	under_data.Format ("data/%s", map_name.Peek_Buffer ());
	file.Set_Name (under_data.Peek_Buffer ());

	return file.Is_Available () ? true : false;
}


//-----------------------------------------------------------------------------
cScMapQueryEvent::cScMapQueryEvent (void)
{
	MapName = "";

	Set_App_Packet_Type (APPPACKETTYPE_SCMAPQUERYEVENT);
}


//-----------------------------------------------------------------------------
void	cScMapQueryEvent::Init (int client_id, const StringClass &map_name)
{
	WWASSERT (cNetwork::I_Am_Server ());
	WWASSERT (client_id >= 0);

	MapName = map_name;
	if (MapName.Get_Length () >= MAX_MAP_NAME_LENGTH) {
		MapName.Erase (MAX_MAP_NAME_LENGTH - 1, MapName.Get_Length () - (MAX_MAP_NAME_LENGTH - 1));
	}

	//
	//	A listen server asking its own client half does not need the wire.
	//
	if (cNetwork::I_Am_Client () && (cNetwork::Get_My_Id () == client_id)) {
		Act ();
	} else {
		Set_Object_Dirty_Bit (client_id, BIT_CREATION, true);
	}

	return ;
}


//-----------------------------------------------------------------------------
void	cScMapQueryEvent::Act (void)
{
	WWASSERT (cNetwork::I_Am_Client ());

	cCsMapQueryResponseEvent *response = new cCsMapQueryResponseEvent;
	response->Init (MapName, Map_Is_Present (MapName));

	Set_Delete_Pending ();
	return ;
}


//-----------------------------------------------------------------------------
void	cScMapQueryEvent::Export_Creation (BitStreamClass &packet)
{
	WWASSERT (cNetwork::I_Am_Server ());

	cNetEvent::Export_Creation (packet);

	packet.Add_Terminated_String (MapName.Peek_Buffer (), true);

	Set_Delete_Pending ();
	return ;
}


//-----------------------------------------------------------------------------
void	cScMapQueryEvent::Import_Creation (BitStreamClass &packet)
{
	WWASSERT (cNetwork::I_Am_Only_Client ());

	cNetEvent::Import_Creation (packet);

	packet.Get_Terminated_String (MapName.Get_Buffer (MAX_MAP_NAME_LENGTH), MAX_MAP_NAME_LENGTH, true);

	Act ();
	return ;
}
