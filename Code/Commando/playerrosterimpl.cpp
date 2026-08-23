/******************************************************************************
*
* FILE
*     playerrosterimpl.cpp
*
* DESCRIPTION
*     Commando's answer to Combat's PlayerRosterInterfaceClass.
*
*     Everything here is a one-line forward to cPlayerManager.  It exists so
*     that Combat -- and therefore the script library, and therefore the level
*     editor -- can ask who is playing without linking against any of it.
*
******************************************************************************/

#include "playerrosterimpl.h"

#include "player.h"
#include "playermanager.h"
#include "playertype.h"


static PlayerRosterImplClass	_ThePlayerRosterImpl;


void
PlayerRosterImplClass::Register (void)
{
	PlayerRosterClass::Set_Interface (&_ThePlayerRosterImpl);
	return ;
}


const unichar_t *
PlayerRosterImplClass::Get_Player_Name (int player_id)
{
	cPlayer *player = cPlayerManager::Find_Player (player_id);
	return (player != nullptr) ? player->Get_Name ().Peek_Buffer () : nullptr;
}


int
PlayerRosterImplClass::Get_Player_Type (int player_id)
{
	cPlayer *player = cPlayerManager::Find_Player (player_id);
	return (player != nullptr) ? player->Get_Player_Type () : PLAYERTYPE_NEUTRAL;
}


int
PlayerRosterImplClass::Get_Player_Count (void)
{
	return cPlayerManager::Count ();
}


//
//	The index is a position in the roster list and is not stable between
//	calls; the ID it yields is what a caller keeps.
//
int
PlayerRosterImplClass::Get_Player_ID_By_Index (int index)
{
	if (index < 0) {
		return -1;
	}

	SList<cPlayer> *list = cPlayerManager::Get_Player_Object_List ();
	if (list == nullptr) {
		return -1;
	}

	int position = 0;
	for (SLNode<cPlayer> *node = list->Head (); node != nullptr; node = node->Next ()) {
		if (position == index) {
			cPlayer *player = node->Data ();
			return (player != nullptr) ? player->Get_Id () : -1;
		}

		position ++;
	}

	return -1;
}
