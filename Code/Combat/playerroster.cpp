/******************************************************************************
*
* FILE
*     playerroster.cpp
*
* DESCRIPTION
*     See playerroster.h.
*
******************************************************************************/

#include "playerroster.h"

PlayerRosterInterfaceClass *	PlayerRosterClass::_TheRoster	= nullptr;

void
PlayerRosterClass::Set_Interface (PlayerRosterInterfaceClass *roster)
{
	_TheRoster = roster;
	return ;
}
