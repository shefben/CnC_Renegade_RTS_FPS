/******************************************************************************
*
* FILE
*     gameinfoimpl.cpp
*
* DESCRIPTION
*     Commando's answer to Combat's GameInfoInterfaceClass.
*
*     Everything here is a one-line forward to cGameData or cTeamManager.  It
*     exists so that Combat -- and therefore the script library, and therefore
*     the level editor -- can ask about the running match without linking
*     against any of it.
*
******************************************************************************/

#include "gameinfoimpl.h"

#include "gamedata.h"
#include "player.h"
#include "playermanager.h"
#include "team.h"
#include "teammanager.h"


static GameInfoImplClass	_TheGameInfoImpl;


void
GameInfoImplClass::Register (void)
{
	GameInfoClass::Set_Interface (&_TheGameInfoImpl);
	return ;
}


float
GameInfoImplClass::Get_Time_Remaining_Seconds (void)
{
	cGameData *game = The_Game ();
	return (game != nullptr) ? game->Get_Time_Remaining_Seconds () : 0.0f;
}


void
GameInfoImplClass::Set_Time_Remaining_Seconds (float seconds)
{
	cGameData *game = The_Game ();
	if (game != nullptr) {
		game->Set_Time_Remaining_Seconds (seconds);
	}

	return ;
}


int
GameInfoImplClass::Get_Time_Limit_Minutes (void)
{
	cGameData *game = The_Game ();
	return (game != nullptr) ? game->Get_Time_Limit_Minutes () : 0;
}


void
GameInfoImplClass::Set_Time_Limit_Minutes (int minutes)
{
	cGameData *game = The_Game ();
	if (game != nullptr) {
		game->Set_Time_Limit_Minutes (minutes);
	}

	return ;
}


unsigned int
GameInfoImplClass::Get_Duration_Seconds (void)
{
	cGameData *game = The_Game ();
	return (game != nullptr) ? game->Get_Game_Duration_S () : 0;
}


int
GameInfoImplClass::Get_Win_Type (void)
{
	cGameData *game = The_Game ();
	return (game != nullptr) ? (int)game->Get_Win_Type () : 0;
}


int
GameInfoImplClass::Get_Winner_ID (void)
{
	cGameData *game = The_Game ();
	return (game != nullptr) ? game->Get_Winner_ID () : -1;
}


int
GameInfoImplClass::Get_Max_Players (void)
{
	cGameData *game = The_Game ();
	return (game != nullptr) ? game->Get_Max_Players () : 0;
}


const char *
GameInfoImplClass::Get_Map_Name (void)
{
	cGameData *game = The_Game ();
	return (game != nullptr) ? game->Get_Map_Name ().Peek_Buffer () : nullptr;
}


const unichar_t *
GameInfoImplClass::Get_Title (void)
{
	cGameData *game = The_Game ();
	return (game != nullptr) ? game->Get_Game_Title () : nullptr;
}


float
GameInfoImplClass::Get_Team_Score (int player_type)
{
	cTeam *team = cTeamManager::Find_Team (player_type);
	return (team != nullptr) ? team->Get_Score () : 0.0f;
}


int
GameInfoImplClass::Get_Team_Credits (int player_type)
{
	cTeam *team = cTeamManager::Find_Team (player_type);
	return (team != nullptr) ? team->Tally_Money () : 0;
}
