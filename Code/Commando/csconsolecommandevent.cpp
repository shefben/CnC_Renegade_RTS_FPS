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
 *                     $Archive:: /Commando/Code/Commando/csconsolecommandevent.cpp               $*
 *                                                                                             *
 *                      $Author:: Tom_s                                                       $*
 *                                                                                             *
 *                     $Modtime:: 10/11/01 1:02p                                              $*
 *                                                                                             *
 *                    $Revision:: 1                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "csconsolecommandevent.h"
#include "playermanager.h"
#include "player.h"

#include <stdio.h>
#include <stdlib.h>

#include "networkobjectfactory.h"
#include "gamemode.h"
#include "cnetwork.h"
#include "consolefunction.h"
#include "apppackettypes.h"
#include "networkobjectmgr.h"


DECLARE_NETWORKOBJECT_FACTORY(cCsConsoleCommandEvent, NETCLASSID_CSCONSOLECOMMANDEVENT);

//-----------------------------------------------------------------------------
cCsConsoleCommandEvent::cCsConsoleCommandEvent(void)
{
	SenderId = 0;
	::strcpy(Command, "");

	Set_App_Packet_Type(APPPACKETTYPE_CSCONSOLECOMMANDEVENT);
}

//-----------------------------------------------------------------------------
void
cCsConsoleCommandEvent::Init(LPCSTR command)
{
	WWASSERT(cNetwork::I_Am_Only_Client());

	WWASSERT(command != nullptr);
   WWASSERT(::strlen(command) > 0);
   WWASSERT(::strlen(command) < sizeof(Command));

	SenderId = cNetwork::Get_My_Id();
	::strcpy(Command, command);

	Set_Network_ID(NetworkObjectMgrClass::Get_New_Client_ID());

	Set_Object_Dirty_Bit(0, BIT_CREATION, true);
}

//-----------------------------------------------------------------------------
void
cCsConsoleCommandEvent::Act(void)
{
   WWASSERT(cNetwork::I_Am_Server());

	//
	//	Anybody with a modified client could send this, and the server used to
	//	do as it was told.  It is honoured only for a client the server has
	//	already marked invulnerable, which is the same gate the money and
	//	score events use and is only reachable through cGodModeEvent.
	//
	cPlayer * p_sender = cPlayerManager::Find_Player(SenderId);
	if (p_sender == nullptr || p_sender->Invulnerable.Is_False()) {
		Set_Delete_Pending();
		return;
	}

	if (GameModeManager::Find("Combat")->Is_Active()) {
		ConsoleFunctionManager::Parse_Input(Command);
	}
}

//-----------------------------------------------------------------------------
void
cCsConsoleCommandEvent::Export_Creation(BitStreamClass & packet)
{
   WWASSERT(cNetwork::I_Am_Only_Client());

	cNetEvent::Export_Creation(packet);

	packet.Add(SenderId);
	packet.Add_Terminated_String(Command);

	Set_Delete_Pending();
}

//-----------------------------------------------------------------------------
void
cCsConsoleCommandEvent::Import_Creation(BitStreamClass & packet)
{
	WWASSERT(cNetwork::I_Am_Server());

	cNetEvent::Import_Creation(packet);

	packet.Get(SenderId);
	packet.Get_Terminated_String(Command, sizeof(Command));

	Act();

	Set_Delete_Pending();
}
