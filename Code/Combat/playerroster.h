/******************************************************************************
*
* FILE
*     playerroster.h
*
* DESCRIPTION
*     Who is playing, asked by number rather than by body.
*
*     A script that knows only a player ID -- a kill message, a chat command,
*     a scoreboard line -- still has to be able to name that player and find
*     their soldier.  The roster those questions read is a Commando notion:
*     there is no roster at all in the level editor, which links Combat and
*     not Commando.  So this is the shape of the question, and Commando
*     answers it, exactly as the game-info seam does for the match itself.
*
*     A script that asks before anything has answered gets a sensible nothing
*     rather than a crash.
*
******************************************************************************/

#ifndef __PLAYERROSTER_H__
#define __PLAYERROSTER_H__

#include "always.h"
#include "wwstring.h"


class	PlayerRosterInterfaceClass
{
public:
	virtual ~PlayerRosterInterfaceClass (void)		{ }

	//
	//	A player's name and side.  Both answer nothing for an ID that no longer
	//	belongs to anybody, which is the ordinary case a moment after somebody
	//	disconnects.  Their soldier is not asked for here: Combat can find that
	//	itself through GameObjManager::Find_Soldier_Of_Client_ID.
	//
	virtual const unichar_t *		Get_Player_Name (int player_id) = 0;
	virtual int							Get_Player_Type (int player_id) = 0;

	//
	//	Walking the roster.  The index is a position in the list and means
	//	nothing between calls; the ID it yields is what to hold on to.
	//
	virtual int							Get_Player_Count (void) = 0;
	virtual int							Get_Player_ID_By_Index (int index) = 0;
};


class	PlayerRosterClass
{
public:

	static void									Set_Interface (PlayerRosterInterfaceClass *roster);
	static PlayerRosterInterfaceClass *	Peek_Interface (void)		{ return _TheRoster; }

private:

	static PlayerRosterInterfaceClass *	_TheRoster;
};

#endif	// __PLAYERROSTER_H__
