/******************************************************************************
*
* FILE
*     scriptkeys.cpp
*
* DESCRIPTION
*     See scriptkeys.h.
*
******************************************************************************/

#include "scriptkeys.h"

#include "combat.h"
#include "csscriptkeyevent.h"
#include "gameeventbus.h"


////////////////////////////////////////////////////////////////////////////
//
//	Local_Key_Pressed
//
////////////////////////////////////////////////////////////////////////////
void
ScriptKeyManagerClass::Local_Key_Pressed (const char *key_name)
{
	if (key_name == nullptr || key_name[0] == 0) {
		return ;
	}

	//
	//	A listen server and single player answer their own key presses; there
	//	is nowhere to send them.
	//
	if (CombatManager::I_Am_Server ()) {
		Key_Pressed (key_name, CombatManager::Get_My_Id ());
		return ;
	}

	if (CombatManager::I_Am_Only_Client ()) {
		//
		//	Sent whether or not anything is listening.  The client cannot know
		//	which keys the level's scripts have asked for, and telling it would
		//	be telling it something about the level it has not seen.
		//
		cCsScriptKeyEvent *event = new cCsScriptKeyEvent;
		event->Init (key_name);
	}

	return ;
}


////////////////////////////////////////////////////////////////////////////
//
//	Key_Pressed
//
////////////////////////////////////////////////////////////////////////////
void
ScriptKeyManagerClass::Key_Pressed (const char *key_name, int player_id)
{
	WWASSERT (CombatManager::I_Am_Server ());

	if (key_name == nullptr || key_name[0] == 0 || player_id == -1) {
		return ;
	}

	GameEventBus::Raise_Player_Key (player_id, key_name);
	return ;
}
