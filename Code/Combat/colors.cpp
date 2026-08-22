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

/***********************************************************************************************
 ***                            Confidential - Westwood Studios                              ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/colors.cpp                            $*
 *                                                                                             *
 *                      $Author:: Tom_s                                                       $*
 *                                                                                             *
 *                     $Modtime:: 9/25/00 3:44p                                               $*
 *                                                                                             *
 *                    $Revision:: 2                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "colors.h"

#include "playertype.h"
#include "ttsettings.h"
#include "wwdebug.h"


//
//	The house colours.  These reach a long way -- the name over a player's
//	head, the radar blips, the flag on their back -- so a server that
//	re-skins its teams sets them in one place.  The defaults are the stock
//	Nod red and GDI gold.
//
Vector3 Get_Color_For_Team(int team)
{
	WWASSERT(team == PLAYERTYPE_NOD || team == PLAYERTYPE_GDI);

	const float *color =	(team == PLAYERTYPE_NOD) ?
							TTSettingsClass::NodHouseColor :
							TTSettingsClass::GDIHouseColor;

	return Vector3(color[0], color[1], color[2]);
}

Vector3 Get_Public_Text_Color(void)
{
	return Vector3(	TTSettingsClass::PublicMessageColor[0],
						TTSettingsClass::PublicMessageColor[1],
						TTSettingsClass::PublicMessageColor[2]);
}

Vector3 Get_Private_Text_Color(void)
{
	return Vector3(	TTSettingsClass::PrivateMessageColor[0],
						TTSettingsClass::PrivateMessageColor[1],
						TTSettingsClass::PrivateMessageColor[2]);
}
