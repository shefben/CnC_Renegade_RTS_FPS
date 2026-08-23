/******************************************************************************
*
* FILE
*     gameinfo.h
*
* DESCRIPTION
*     What a script may ask about the match that is running: how long is left,
*     who won, what the map is called, how each side is doing.
*
*     None of that belongs to Combat.  A match is a Commando notion -- there is
*     no match at all in the level editor, which links Combat and not Commando
*     -- so this is the shape of the question and Commando answers it.  A
*     script that asks before anything has answered gets a sensible nothing
*     rather than a crash.
*
******************************************************************************/

#ifndef __GAMEINFO_H__
#define __GAMEINFO_H__

#include "always.h"
#include "wwstring.h"


class	GameInfoInterfaceClass
{
public:
	virtual ~GameInfoInterfaceClass (void)		{ }

	//
	//	The clock.  Time remaining is what the players can see; the limit is
	//	what it started from.  Both are writable, because a level's own
	//	objectives may extend or cut a match short.
	//
	virtual float					Get_Time_Remaining_Seconds (void) = 0;
	virtual void					Set_Time_Remaining_Seconds (float seconds) = 0;
	virtual int						Get_Time_Limit_Minutes (void) = 0;
	virtual void					Set_Time_Limit_Minutes (int minutes) = 0;

	//
	//	How long the match has been running, which is not the inverse of the
	//	clock: a match with no time limit still has a duration.
	//
	virtual unsigned int			Get_Duration_Seconds (void) = 0;

	//
	//	How the match ended and who won.  Both are meaningless until it has.
	//
	virtual int						Get_Win_Type (void) = 0;
	virtual int						Get_Winner_ID (void) = 0;

	virtual int						Get_Max_Players (void) = 0;
	virtual const char *			Get_Map_Name (void) = 0;
	virtual const unichar_t *	Get_Title (void) = 0;

	//
	//	Per-side standings.  Credits are the sum of what the side's players are
	//	carrying, not a pool.
	//
	virtual float					Get_Team_Score (int player_type) = 0;
	virtual int						Get_Team_Credits (int player_type) = 0;
};


class	GameInfoClass
{
public:

	//
	//	Commando installs the real answer at startup.  Nothing installs one in
	//	the editor, and nothing has to.
	//
	static void								Set_Interface (GameInfoInterfaceClass *game_info);
	static GameInfoInterfaceClass *	Peek_Interface (void)		{ return _TheGameInfo; }

private:

	static GameInfoInterfaceClass *	_TheGameInfo;
};


#endif	// __GAMEINFO_H__
