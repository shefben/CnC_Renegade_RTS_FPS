/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 Electronic Arts Inc.
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

#include "gameeventlisteners.h"

#include "gameeventbus.h"
#include "player.h"
#include "playermanager.h"
#include "ssgmmanager.h"
#include "consolefunction.h"


static int	_ClientQueryToken	= 0;
static int	_ServerStartupToken	= 0;
static int	_ServerShutdownToken	= 0;
static int	_ConsoleInputToken	= 0;


//-----------------------------------------------------------------------------
//
//	Answer with the ids of the connected players, optionally narrowed to one
//	team.  Answered is set even when the list comes back empty: "nobody is on
//	that team" is a real answer, and the caller has to be able to tell it apart
//	from nobody having answered at all.
//
static void
Client_Query_Handler (ClientQueryEventClass &event, void * /* data */)
{
	for (	SLNode<cPlayer> *player_node = cPlayerManager::Get_Player_Object_List ()->Head ();
			player_node != nullptr;
			player_node = player_node->Next ()) {

		cPlayer *player = player_node->Data ();
		if (player == nullptr || player->Get_Is_Active ().Is_False ()) {
			continue;
		}

		if (event.Team != -1 && player->Get_Player_Type () != event.Team) {
			continue;
		}

		event.ClientIDs.Add (player->Get_Id ());
	}

	event.Answered = true;
	return ;
}


//-----------------------------------------------------------------------------
//
//	The server-side game manager exists only while this machine is a server,
//	because every rule it applies is a decision only a server gets to make.
//
//
//	A line typed at something other than this machine's own console -- the
//	server's remote administration port, for instance.  It is a console
//	command like any other once it gets here.
//
static void
Console_Input_Handler (ConsoleInputEventClass &event, void * /*data*/)
{
	if (event.Text != nullptr && event.Text[0] != 0) {
		ConsoleFunctionManager::Parse_Input (event.Text);
	}

	return ;
}


static void
Server_Startup_Handler (ServerLifecycleEventClass & /*event*/, void * /*data*/)
{
	SSGMManagerClass::Register ();
	return ;
}


static void
Server_Shutdown_Handler (ServerLifecycleEventClass & /*event*/, void * /*data*/)
{
	SSGMManagerClass::Unregister ();
	return ;
}


void
GameEventListeners::Register (void)
{
	if (_ClientQueryToken == 0) {
		_ClientQueryToken = GameEventBus::ClientQuery.Register (Client_Query_Handler);
	}

	if (_ServerStartupToken == 0) {
		_ServerStartupToken = GameEventBus::ServerStartup.Register (Server_Startup_Handler);
		_ServerShutdownToken = GameEventBus::ServerShutdown.Register (Server_Shutdown_Handler);
	}

	if (_ConsoleInputToken == 0) {
		_ConsoleInputToken = GameEventBus::ConsoleInput.Register (Console_Input_Handler);
	}

	return ;
}


//-----------------------------------------------------------------------------
void
GameEventListeners::Unregister (void)
{
	if (_ClientQueryToken != 0) {
		GameEventBus::ClientQuery.Unregister (_ClientQueryToken);
		_ClientQueryToken = 0;
	}

	if (_ServerStartupToken != 0) {
		GameEventBus::ServerStartup.Unregister (_ServerStartupToken);
		GameEventBus::ServerShutdown.Unregister (_ServerShutdownToken);
		_ServerStartupToken = 0;
		_ServerShutdownToken = 0;
	}

	if (_ConsoleInputToken != 0) {
		GameEventBus::ConsoleInput.Unregister (_ConsoleInputToken);
		_ConsoleInputToken = 0;
	}

	SSGMManagerClass::Unregister ();

	return ;
}
