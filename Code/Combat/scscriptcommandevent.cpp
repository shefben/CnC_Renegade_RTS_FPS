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

#include "scscriptcommandevent.h"

#include "apppackettypes.h"
#include "bitpackids.h"
#include "combat.h"
#include "gameeventbus.h"
#include "backgroundmgr.h"
#include "hud.h"
#include "mapmgr.h"
#include "ttsettings.h"
#include "gamehint.h"
#include "playerterminal.h"
#include "soldier.h"
#include "messagewindow.h"
#include "networkobjectfactory.h"
#include "scriptcommands.h"
#include "smartgameobj.h"
#include "widestring.h"


DECLARE_NETWORKOBJECT_FACTORY(cScScriptCommandEvent, NETCLASSID_SCSCRIPTCOMMANDEVENT);


//-----------------------------------------------------------------------------
cScScriptCommandEvent::cScScriptCommandEvent (void)
	:	Command (SCRIPT_CLIENT_CMD_COUNT),
		IntParam1 (0),
		IntParam2 (0),
		IntParam3 (0),
		IntParam4 (0),
		IntParam5 (0),
		FloatParam1 (0.0f),
		FloatParam2 (0.0f),
		Color (1.0f, 1.0f, 1.0f),
		Position (0.0f, 0.0f, 0.0f)
{
	Set_App_Packet_Type (APPPACKETTYPE_SCSCRIPTCOMMANDEVENT);
}


//-----------------------------------------------------------------------------
void
cScScriptCommandEvent::Init (ScriptClientCommandEnum command, int client_id)
{
	Command = command;
	Send_To (client_id);
}


//-----------------------------------------------------------------------------
//
//	Only the game knows who is connected and which team they chose, so the team
//	is resolved over the event bus.  Nobody answers in the editor, and the
//	command quietly does nothing there.
//
void
cScScriptCommandEvent::Init_For_Team (ScriptClientCommandEnum command, int team)
{
	Command = command;

	DynamicVectorClass<int> client_ids;
	if (!GameEventBus::Raise_Client_Query (team, client_ids)) {
		Set_Delete_Pending ();
		return ;
	}

	for (int index = 0; index < client_ids.Count (); index ++) {
		Send_To (client_ids[index]);
	}

	if (client_ids.Count () == 0) {
		Set_Delete_Pending ();
	}
}


//-----------------------------------------------------------------------------
//
//	`client_id` of -1 means everyone.  A dirty bit only reaches remote clients,
//	so a listen server that is itself the addressee runs the command directly.
//
void
cScScriptCommandEvent::Send_To (int client_id)
{
	if (client_id == -1) {

		Set_Object_Dirty_Bit (BIT_CREATION, true);

		if (CombatManager::I_Am_Client ()) {
			Act ();
		}

	} else if (CombatManager::I_Am_Client () && client_id == CombatManager::Get_My_Id ()) {

		Act ();

	} else {

		Set_Object_Dirty_Bit (client_id, BIT_CREATION, true);
	}
}


//-----------------------------------------------------------------------------
void
cScScriptCommandEvent::Act (void)
{
	switch (Command) {

		case SCRIPT_CLIENT_CMD_SEND_MESSAGE:
		{
			MessageWindowClass *window = CombatManager::Get_Message_Window ();
			if (window != nullptr) {
				WideStringClass message;
				message.Convert_From (Text);
				window->Add_Message (message, Color);
			}
			break;
		}

		case SCRIPT_CLIENT_CMD_CREATE_2D_SOUND:
			ScriptEngine::Create_2D_Sound (Text);
			break;

		case SCRIPT_CLIENT_CMD_CREATE_2D_WAV_SOUND:
			ScriptEngine::Create_2D_WAV_Sound (Text);
			break;

		case SCRIPT_CLIENT_CMD_CREATE_SOUND:
			ScriptEngine::Create_Sound (Text, Position, nullptr);
			break;

		case SCRIPT_CLIENT_CMD_SET_HUD_HELP_TEXT:
			ScriptEngine::Set_HUD_Help_Text (IntParam1, Color);
			break;

		case SCRIPT_CLIENT_CMD_SET_SCREEN_FADE_COLOR:
			ScriptEngine::Set_Screen_Fade_Color (Color.X, Color.Y, Color.Z, FloatParam1);
			break;

		case SCRIPT_CLIENT_CMD_SET_SCREEN_FADE_OPACITY:
			ScriptEngine::Set_Screen_Fade_Opacity (FloatParam1, FloatParam2);
			break;

		case SCRIPT_CLIENT_CMD_FORCE_CAMERA_LOOK:
			ScriptEngine::Force_Camera_Look (Position);
			break;

		case SCRIPT_CLIENT_CMD_ENABLE_RADAR:
			ScriptEngine::Enable_Radar (IntParam1 != 0);
			break;

		case SCRIPT_CLIENT_CMD_SET_BACKGROUND_MUSIC:
			ScriptEngine::Set_Background_Music (Text);
			break;

		case SCRIPT_CLIENT_CMD_FADE_BACKGROUND_MUSIC:
			ScriptEngine::Fade_Background_Music (Text, IntParam1, IntParam2);
			break;

		case SCRIPT_CLIENT_CMD_STOP_BACKGROUND_MUSIC:
			ScriptEngine::Stop_Background_Music ();
			break;

		case SCRIPT_CLIENT_CMD_DISPLAY_GDI_TERMINAL:
			ScriptEngine::Display_GDI_Player_Terminal ();
			break;

		case SCRIPT_CLIENT_CMD_DISPLAY_NOD_TERMINAL:
			ScriptEngine::Display_NOD_Player_Terminal ();
			break;

		case SCRIPT_CLIENT_CMD_SET_INFO_TEXTURE:
			if (Text.Is_Empty ()) {
				HUDClass::Clear_Info_Texture ();
			} else {
				HUDClass::Set_Info_Texture (Text);
			}
			break;

		case SCRIPT_CLIENT_CMD_LOAD_HUD_INI:
			TTSettingsClass::Load_UI_File (Text);
			break;

		case SCRIPT_CLIENT_CMD_CHANGE_RADAR_MAP:
			MapMgrClass::Set_Map_Texture (Text);
			MapMgrClass::Set_Map_Scale (Vector2 (FloatParam1, FloatParam1));
			MapMgrClass::Set_Map_Center (Vector2 (Position.X, Position.Y));
			break;

		case SCRIPT_CLIENT_CMD_CREATE_LIGHTNING:
			BackgroundMgrClass::Create_Lightning_Bolt (Position, Color, FloatParam1);
			break;

		case SCRIPT_CLIENT_CMD_DISPLAY_GAME_HINT:
			GameHintClass::Show (IntParam1, Text, IntParam2, IntParam3, IntParam4, IntParam5, Text2);
			break;

		case SCRIPT_CLIENT_CMD_DISPLAY_OBJECTIVES_TEXT:
			PlayerTerminalClass::Get_Instance ()->Display_Objectives_Text (Text);
			break;

		case SCRIPT_CLIENT_CMD_CREATE_3D_WAV_SOUND_AT_BONE:
			ScriptEngine::Create_3D_WAV_Sound_At_Bone (Text,
					ScriptEngine::Find_Object (IntParam1), Text2);
			break;

		case SCRIPT_CLIENT_CMD_SET_EMOT_ICON:
		{
			GameObject *		target	= ScriptEngine::Find_Object (IntParam1);
			SoldierGameObj *	soldier	= (target != nullptr) ? target->As_SoldierGameObj () : nullptr;
			if (soldier != nullptr) {
				soldier->Set_Emot_Icon (Text, FloatParam1);
			}
			break;
		}

		case SCRIPT_CLIENT_CMD_SET_GLOBAL_STEALTH_DISABLE:
			SmartGameObj::Set_Global_Stealth_Disabled (IntParam1 != 0);
			break;

		case SCRIPT_CLIENT_CMD_SET_CAMERA_HOST:
			ScriptEngine::Set_Camera_Host ((IntParam1 != 0)
					? ScriptEngine::Find_Object (IntParam1) : nullptr);
			break;

		case SCRIPT_CLIENT_CMD_SET_SUBOBJECT_ANIMATION:
		{
			GameObject *object = ScriptEngine::Find_Object (IntParam1);
			if (object != nullptr) {
				ScriptEngine::Set_Animation (object, Text, (IntParam2 & 1) != 0, Text2,
					FloatParam1, FloatParam2, (IntParam2 & 2) != 0);
			}
			break;
		}

		default:
			WWDEBUG_SAY (("cScScriptCommandEvent::Act: unknown command %d\n", Command));
			break;
	}

	Set_Delete_Pending ();
}


//-----------------------------------------------------------------------------
void
cScScriptCommandEvent::Export_Creation (BitStreamClass &packet)
{
	WWASSERT (CombatManager::I_Am_Server ());

	NetworkObjectClass::Export_Creation (packet);

	packet.Add ((BYTE)Command);
	packet.Add (IntParam1);
	packet.Add (IntParam2);
	packet.Add (IntParam3);
	packet.Add (IntParam4);
	packet.Add (IntParam5);
	packet.Add (FloatParam1);
	packet.Add (FloatParam2);
	packet.Add (Color.X);
	packet.Add (Color.Y);
	packet.Add (Color.Z);
	packet.Add (Position.X, BITPACK_WORLD_POSITION_X);
	packet.Add (Position.Y, BITPACK_WORLD_POSITION_Y);
	packet.Add (Position.Z, BITPACK_WORLD_POSITION_Z);
	packet.Add_Terminated_String (Text, true);
	packet.Add_Terminated_String (Text2, true);

	Set_Delete_Pending ();
}


//-----------------------------------------------------------------------------
void
cScScriptCommandEvent::Import_Creation (BitStreamClass &packet)
{
	WWASSERT (CombatManager::I_Am_Only_Client ());

	NetworkObjectClass::Import_Creation (packet);

	BYTE command = 0;
	packet.Get (command);
	Command = command;

	packet.Get (IntParam1);
	packet.Get (IntParam2);
	packet.Get (IntParam3);
	packet.Get (IntParam4);
	packet.Get (IntParam5);
	packet.Get (FloatParam1);
	packet.Get (FloatParam2);
	packet.Get (Color.X);
	packet.Get (Color.Y);
	packet.Get (Color.Z);
	packet.Get (Position.X, BITPACK_WORLD_POSITION_X);
	packet.Get (Position.Y, BITPACK_WORLD_POSITION_Y);
	packet.Get (Position.Z, BITPACK_WORLD_POSITION_Z);

	char buffer[512] = { 0 };
	packet.Get_Terminated_String (buffer, sizeof (buffer), true);
	Text = buffer;

	buffer[0] = 0;
	packet.Get_Terminated_String (buffer, sizeof (buffer), true);
	Text2 = buffer;

	Act ();
}
