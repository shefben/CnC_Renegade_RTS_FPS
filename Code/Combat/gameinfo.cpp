/******************************************************************************
*
* FILE
*     gameinfo.cpp
*
* DESCRIPTION
*     See gameinfo.h.
*
******************************************************************************/

#include "gameinfo.h"


GameInfoInterfaceClass *	GameInfoClass::_TheGameInfo	= nullptr;


void
GameInfoClass::Set_Interface (GameInfoInterfaceClass *game_info)
{
	_TheGameInfo = game_info;
	return ;
}
