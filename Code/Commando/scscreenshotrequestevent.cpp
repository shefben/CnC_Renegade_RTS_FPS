/******************************************************************************
*
* FILE
*     scscreenshotrequestevent.cpp
*
* DESCRIPTION
*     See scscreenshotrequestevent.h.
*
******************************************************************************/

#include "scscreenshotrequestevent.h"

#include "apppackettypes.h"
#include "cnetwork.h"
#include "networkobjectfactory.h"
#include "remotescreenshot.h"

DECLARE_NETWORKOBJECT_FACTORY(cScScreenshotRequestEvent, NETCLASSID_SCSCREENSHOTREQUESTEVENT);


//-----------------------------------------------------------------------------
cScScreenshotRequestEvent::cScScreenshotRequestEvent (void)
{
	Url = "";

	Set_App_Packet_Type (APPPACKETTYPE_SCSCREENSHOTREQUESTEVENT);
}


//-----------------------------------------------------------------------------
void	cScScreenshotRequestEvent::Init (int client_id, const StringClass &url)
{
	WWASSERT (cNetwork::I_Am_Server ());
	WWASSERT (client_id >= 0);

	Url = url;
	if (Url.Get_Length () >= MAX_URL_LENGTH) {
		Url.Erase (MAX_URL_LENGTH - 1, Url.Get_Length () - (MAX_URL_LENGTH - 1));
	}

	if (cNetwork::I_Am_Client () && (cNetwork::Get_My_Id () == client_id)) {
		Act ();
	} else {
		Set_Object_Dirty_Bit (client_id, BIT_CREATION, true);
	}

	return ;
}


//-----------------------------------------------------------------------------
void	cScScreenshotRequestEvent::Act (void)
{
	WWASSERT (cNetwork::I_Am_Client ());

	RemoteScreenshot::Request (Url.Peek_Buffer ());

	Set_Delete_Pending ();
	return ;
}


//-----------------------------------------------------------------------------
void	cScScreenshotRequestEvent::Export_Creation (BitStreamClass &packet)
{
	WWASSERT (cNetwork::I_Am_Server ());

	cNetEvent::Export_Creation (packet);

	packet.Add_Terminated_String (Url.Peek_Buffer (), true);

	Set_Delete_Pending ();
	return ;
}


//-----------------------------------------------------------------------------
void	cScScreenshotRequestEvent::Import_Creation (BitStreamClass &packet)
{
	WWASSERT (cNetwork::I_Am_Only_Client ());

	cNetEvent::Import_Creation (packet);

	packet.Get_Terminated_String (Url.Get_Buffer (MAX_URL_LENGTH), MAX_URL_LENGTH, true);

	Act ();
	return ;
}
