/******************************************************************************
*
* FILE
*     csscriptkeyevent.cpp
*
* DESCRIPTION
*     See csscriptkeyevent.h.
*
******************************************************************************/

#include "csscriptkeyevent.h"

#include "apppackettypes.h"
#include "combat.h"
#include "networkobjectfactory.h"
#include "networkobjectmgr.h"
#include "scriptkeys.h"


DECLARE_NETWORKOBJECT_FACTORY(cCsScriptKeyEvent, NETCLASSID_CSSCRIPTKEYEVENT);


//
//	A logical key name is a script parameter, not free text.  Anything longer
//	than this is a client that has been tampered with, and is dropped rather
//	than truncated: a truncated name would silently match the wrong hook.
//
const int	KEY_NAME_MAX	= 64;


cCsScriptKeyEvent::cCsScriptKeyEvent (void)
	:	SenderId (0)
{
	Set_App_Packet_Type (APPPACKETTYPE_CSSCRIPTKEYEVENT);
}


void
cCsScriptKeyEvent::Init (const char *key_name)
{
	WWASSERT (CombatManager::I_Am_Only_Client ());

	SenderId	= CombatManager::Get_My_Id ();
	KeyName	= key_name;

	Set_Network_ID (NetworkObjectMgrClass::Get_New_Client_ID ());

	Set_Object_Dirty_Bit (0, BIT_CREATION, true);
	return ;
}


void
cCsScriptKeyEvent::Act (void)
{
	WWASSERT (CombatManager::I_Am_Server ());

	ScriptKeyManagerClass::Key_Pressed (KeyName, SenderId);

	Set_Delete_Pending ();
	return ;
}


void
cCsScriptKeyEvent::Export_Creation (BitStreamClass &packet)
{
	WWASSERT (CombatManager::I_Am_Only_Client ());

	NetworkObjectClass::Export_Creation (packet);

	WWASSERT (SenderId > 0);

	packet.Add (SenderId);
	packet.Add_Terminated_String (KeyName, true);

	Set_Delete_Pending ();
	return ;
}


void
cCsScriptKeyEvent::Import_Creation (BitStreamClass &packet)
{
	WWASSERT (CombatManager::I_Am_Server ());

	NetworkObjectClass::Import_Creation (packet);

	packet.Get (SenderId);

	char buffer[KEY_NAME_MAX + 1] = { 0 };
	packet.Get_Terminated_String (buffer, sizeof (buffer), true);
	KeyName = buffer;

	WWASSERT (SenderId > 0);

	//
	//	The sender id is what the client put in the packet, which is the
	//	convention every other client-to-server event here follows.  A key
	//	press claimed on another player's behalf reaches only the hooks that
	//	player's own scripts installed, so the worst it can do is press
	//	somebody else's deploy key.
	//
	Act ();
	return ;
}
