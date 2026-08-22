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


static int	_ClientQueryToken	= 0;


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
void
GameEventListeners::Register (void)
{
	if (_ClientQueryToken == 0) {
		_ClientQueryToken = GameEventBus::ClientQuery.Register (Client_Query_Handler);
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

	return ;
}
