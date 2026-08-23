/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 OpenW3D contributors.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/***********************************************************************************************
 *
 *	SSGMManagerClass -- the server-side game manager, natively.
 *
 *	This is what the 4.8.4 package called SSGM: the layer a server operator configures rather
 *	than a level designer.  It takes buildings out of play, hands vehicles to whoever bought
 *	them, prices purchases, answers the chat commands players type, and narrates the game down
 *	the log socket.
 *
 *	4.8.4 installed it by overwriting engine memory with a list of hook functions and then
 *	fanning each hook out to loaded plugin DLLs.  Neither half is reproduced here: the events it
 *	hooked are raised natively by the functions that own them, so this registers on the event bus
 *	like anything else, and the plugin loader is declined under roadmap directive 0.5 -- the bus
 *	is the extension point now, and it does not need a DLL to reach.
 *
 *	Server only.  Register is called on a machine that is running the game; a pure client
 *	registers nothing, which is why every handler may assume it is authoritative.
 *
 **********************************************************************************************/

#ifndef	SSGMMANAGER_H
#define	SSGMMANAGER_H

#ifndef	ALWAYS_H
	#include "always.h"
#endif


class	SSGMManagerClass
{
public:

	////////////////////////////////////////////////////////////////
	//	Public methods
	////////////////////////////////////////////////////////////////

	//
	//	Read ssgm.ini, open the log socket if one was asked for, and subscribe
	//	to the events the manager acts on.  Safe to call twice.
	//
	static void		Register (void);
	static void		Unregister (void);

	static bool		Is_Registered (void)		{ return IsRegistered; }

private:

	////////////////////////////////////////////////////////////////
	//	Event handlers
	////////////////////////////////////////////////////////////////

	static void		On_Object_Create (class ObjectCreateEventClass &event, void *data);
	static void		On_Level_Loaded (class LevelEventClass &event, void *data);
	static void		On_Game_Over (class GameOverEventClass &event, void *data);
	static void		On_Chat (class ChatEventClass &event, void *data);
	static void		On_Player_Key (class PlayerKeyEventClass &event, void *data);
	static void		On_Refill (class RefillEventClass &event, void *data);
	static void		On_Purchase (class PurchaseEventClass &event, void *data);
	static void		On_Console_Output (class ConsoleOutputEventClass &event, void *data);
	static void		On_Think (class ThinkEventClass &event, void *data);

	////////////////////////////////////////////////////////////////
	//	Private methods
	////////////////////////////////////////////////////////////////

	//
	//	Apply the settings to the level that has just loaded: take the
	//	disabled buildings out of play and make the rest invulnerable if the
	//	operator asked for that.
	//
	static void		Apply_Building_Settings (void);

	//	The chat commands, answered before the message is distributed.
	static bool		Handle_Chat_Command (int player_id, const wchar_t *message);

	//
	//	The same three answers on a key.  4.8.4 reached them through three
	//	scripts attached to every player; they are commands of this layer,
	//	not behaviour of a character, so they live beside the chat words
	//	that do the same thing.
	//
	static bool		Handle_Key_Command (int player_id, const char *key_name);

	static void		Bind_Vehicle (class SoldierGameObj *soldier);
	static void		Unbind_Vehicle (class SoldierGameObj *soldier, bool silent);
	static void		Lock_Vehicle (class SoldierGameObj *soldier);
	static void		Unlock_Vehicle (class SoldierGameObj *soldier, bool silent);
	static void		Kick_Vehicle_Driver (class SoldierGameObj *soldier);
	static void		Report_Mines (class SoldierGameObj *soldier);

	//	One line of private text to one player, in the server's own colour.
	static void		Tell (class SoldierGameObj *soldier, const char *format, ...);

	////////////////////////////////////////////////////////////////
	//	Private member data
	////////////////////////////////////////////////////////////////

	static bool		IsRegistered;

	static int		ObjectCreateToken;
	static int		LevelLoadedToken;
	static int		GameOverToken;
	static int		ChatToken;
	static int		PlayerKeyToken;
	static int		RefillToken;
	static int		PurchaseToken;
	static int		ConsoleOutputToken;
	static int		ThinkToken;
};

#endif	// SSGMMANAGER_H
