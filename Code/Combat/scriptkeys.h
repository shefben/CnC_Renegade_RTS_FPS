/******************************************************************************
*
* FILE
*     scriptkeys.h
*
* DESCRIPTION
*     Script keys: keys a level's scripts give a meaning to, which the engine
*     itself has none for.
*
*     A script key has a logical name -- "Deploy", "Sidebar", "VDeploy" -- and
*     a key bound to it in the player's input configuration, alongside every
*     other binding.  Pressing it does nothing on the client; the press is
*     forwarded to the server, which raises GameEventBus::PlayerKey, and any
*     script that asked for that name and that player runs.
*
*     The client is deliberately ignorant.  It does not know what the key does,
*     whether anything is listening, or whether the press was acted on.  That
*     is what lets a level invent a key without shipping any client code, and
*     it is why the decision is never taken on the client side.
*
******************************************************************************/

#ifndef __SCRIPTKEYS_H__
#define __SCRIPTKEYS_H__


class	ScriptKeyManagerClass
{
public:

	//
	//	Client side.  A key bound to a logical name has just gone down.  On a
	//	listen server or in single player this is answered here; on a pure
	//	client it goes to the server as a cCsScriptKeyEvent.
	//
	static void	Local_Key_Pressed (const char *key_name);

	//
	//	Server side.  A logical key press has arrived, either from the local
	//	player or from a client.  Raises PlayerKey.
	//
	static void	Key_Pressed (const char *key_name, int player_id);
};


#endif	// __SCRIPTKEYS_H__
