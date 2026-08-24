/******************************************************************************
*
* FILE
*     csscreenshotresultevent.cpp
*
* DESCRIPTION
*     See csscreenshotresultevent.h.
*
******************************************************************************/

#include "csscreenshotresultevent.h"

#include "apppackettypes.h"
#include "cnetwork.h"
#include "consolefunction.h"
#include "networkobjectfactory.h"
#include "networkobjectmgr.h"
#include "player.h"
#include "playermanager.h"
#include "remotescreenshot.h"
#include "wwstring.h"

DECLARE_NETWORKOBJECT_FACTORY(cCsScreenshotResultEvent, NETCLASSID_CSSCREENSHOTRESULTEVENT);


//-----------------------------------------------------------------------------
cCsScreenshotResultEvent::cCsScreenshotResultEvent (void)
{
	ClientId	= -1;
	Result	= REMOTE_SCREENSHOT_UPLOAD_FAILED;

	Set_App_Packet_Type (APPPACKETTYPE_CSSCREENSHOTRESULTEVENT);
}


//-----------------------------------------------------------------------------
void	cCsScreenshotResultEvent::Init (int result)
{
	WWASSERT (cNetwork::I_Am_Client ());

	ClientId	= cNetwork::Get_My_Id ();
	Result	= result;

	Set_Network_ID (NetworkObjectMgrClass::Get_New_Client_ID ());

	if (cNetwork::I_Am_Server ()) {
		Act ();
	} else {
		Set_Object_Dirty_Bit (0, BIT_CREATION, true);
	}

	return ;
}


//-----------------------------------------------------------------------------
void	cCsScreenshotResultEvent::Act (void)
{
	WWASSERT (cNetwork::I_Am_Server ());

	StringClass	name;
	cPlayer *	player = cPlayerManager::Find_Player (ClientId);
	if (player != nullptr) {
		player->Get_Name ().Convert_To (name);
	}

	ConsoleFunctionClass::Print ("Player %d (%s) %s\n",
		ClientId, name.Peek_Buffer (), RemoteScreenshot::Get_Result_Name (Result));

	Set_Delete_Pending ();
	return ;
}


//-----------------------------------------------------------------------------
void	cCsScreenshotResultEvent::Export_Creation (BitStreamClass &packet)
{
	WWASSERT (cNetwork::I_Am_Only_Client ());

	cNetEvent::Export_Creation (packet);

	packet.Add (ClientId);
	packet.Add (Result);

	Set_Delete_Pending ();
	return ;
}


//-----------------------------------------------------------------------------
void	cCsScreenshotResultEvent::Import_Creation (BitStreamClass &packet)
{
	WWASSERT (cNetwork::I_Am_Server ());

	cNetEvent::Import_Creation (packet);

	packet.Get (ClientId);
	packet.Get (Result);

	Act ();
	return ;
}
