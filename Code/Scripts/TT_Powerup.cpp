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

/******************************************************************************
*
* FILE
*     TT_Powerup.cpp
*
* DESCRIPTION
*     What a powerup does when somebody picks it up, and what a spawner does
*     while it waits.  Native port of the 4.8.4 library's jfwpow.cpp; the
*     scripts keep their registered names so existing levels still find them.
*
******************************************************************************/

#include "scripts.h"


/*JFW_Spawn_Timer

  Waits for a custom, then after a delay tells another object about it and
  creates something at a fixed place.

  Parameters:

  Time				= Seconds to wait.
  TimerNum			= Timer id to use.
  SpawnCustom		= Custom that starts the wait.
  TriggerCustom		= Custom to send when the wait is over.
  TriggerID			= Object to send it to.
  SpawnPreset		= What to create.
  SpawnLocation		= Where to create it.
*/

DECLARE_SCRIPT_TT(JFW_Spawn_Timer, "Time:float,TimerNum:int,SpawnCustom:int,TriggerCustom:int,TriggerID:int,SpawnPreset:string,SpawnLocation:vector3")
{
	void Custom(GameObject* obj, int type, int /*param*/, GameObject* /*sender*/) override
	{
		if (type == Get_Int_Parameter("SpawnCustom")) {
			ScriptEngine::Start_Timer(obj, this, Get_Float_Parameter("Time"), Get_Int_Parameter("TimerNum"));
		}
	}

	void Timer_Expired(GameObject* obj, int /*number*/) override
	{
		ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(Get_Int_Parameter("TriggerID")),
			Get_Int_Parameter("TriggerCustom"), 0, 0);
		ScriptEngine::Create_Object(Get_Parameter("SpawnPreset"), Get_Vector3_Parameter("SpawnLocation"));
	}
};


/*JFW_Warp_On_Pickup_Team

  Moves whoever picks this powerup up to a fixed position, if they are on the
  right side.

  Parameters:

  Position			= Where to send them.
  Player_Type		= Who this applies to.
*/

DECLARE_SCRIPT_TT(JFW_Warp_On_Pickup_Team, "Position:vector3,Player_Type:int")
{
	void Custom(GameObject* /*obj*/, int type, int /*param*/, GameObject* sender) override
	{
		if (type != CUSTOM_EVENT_POWERUP_GRANTED) {
			return;
		}

		if (!Is_Player_Type(sender, Get_Int_Parameter("Player_Type"))) {
			return;
		}

		ScriptEngine::Set_Position(sender, Get_Vector3_Parameter("Position"));
	}
};


/*JFW_Refill_Powerup

  Restores ammunition, health and armor to whoever picks it up.
*/

DECLARE_SCRIPT_TT(JFW_Refill_Powerup, "")
{
	void Custom(GameObject* /*obj*/, int type, int /*param*/, GameObject* sender) override
	{
		if (type == CUSTOM_EVENT_POWERUP_GRANTED) {
			ScriptEngine::Grant_Refill(sender);
		}
	}
};


/*JFW_Vehicle_Powerup

  Sends a custom to the vehicle the sender is riding in.

  Parameters:

  Message			= Custom to send.
*/

DECLARE_SCRIPT_TT(JFW_Vehicle_Powerup, "Message:int")
{
	void Custom(GameObject* obj, int /*type*/, int /*param*/, GameObject* sender) override
	{
		GameObject* vehicle = ScriptEngine::Get_Vehicle(sender);
		if (vehicle != nullptr) {
			ScriptEngine::Send_Custom_Event(obj, vehicle, Get_Int_Parameter("Message"), 0, 0);
		}
	}
};


/*JFW_Soldier_Powerup

  Sends a custom straight back to the sender.

  Parameters:

  Message			= Custom to send.
*/

DECLARE_SCRIPT_TT(JFW_Soldier_Powerup, "Message:int")
{
	void Custom(GameObject* obj, int /*type*/, int /*param*/, GameObject* sender) override
	{
		ScriptEngine::Send_Custom_Event(obj, sender, Get_Int_Parameter("Message"), 0, 0);
	}
};


/*JFW_Vehicle_Powerup_Preset

  Sends a custom to the vehicle the sender is riding in, but only if it is the
  named one.

  Parameters:

  Message			= Custom to send.
  Preset			= Vehicle preset this applies to.
*/

DECLARE_SCRIPT_TT(JFW_Vehicle_Powerup_Preset, "Message:int,Preset:string")
{
	void Custom(GameObject* obj, int /*type*/, int /*param*/, GameObject* sender) override
	{
		GameObject* vehicle = ScriptEngine::Get_Vehicle(sender);
		if (vehicle == nullptr) {
			return;
		}

		if (::stricmp(ScriptEngine::Get_Preset_Name(vehicle), Get_Parameter("Preset")) != 0) {
			return;
		}

		ScriptEngine::Send_Custom_Event(obj, vehicle, Get_Int_Parameter("Message"), 0, 0);
	}
};


/*JFW_Powerup_Send_Custom

  Tells one particular object that this powerup was picked up.

  Parameters:

  ID				= Object to tell.
  Message			= Custom to send.
*/

DECLARE_SCRIPT_TT(JFW_Powerup_Send_Custom, "ID:int,Message:int")
{
	void Custom(GameObject* obj, int type, int /*param*/, GameObject* /*sender*/) override
	{
		if (type == CUSTOM_EVENT_POWERUP_GRANTED) {
			ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(Get_Int_Parameter("ID")),
				Get_Int_Parameter("Message"), 0, 0);
		}
	}
};


/*JFW_Powerup_Send_Custom_Team

  As above, but tells a different object depending on which side picked it up.

  Parameters:

  GDIID				= Object to tell when GDI picked it up.
  GDIMessage		= Custom to send them.
  NodID				= Object to tell when Nod picked it up.
  NodMessage		= Custom to send them.
*/

DECLARE_SCRIPT_TT(JFW_Powerup_Send_Custom_Team, "GDIID:int,GDIMessage:int,NodID:int,NodMessage:int")
{
	void Custom(GameObject* obj, int type, int /*param*/, GameObject* sender) override
	{
		if (type != CUSTOM_EVENT_POWERUP_GRANTED) {
			return;
		}

		if (ScriptEngine::Get_Player_Type(sender) == SCRIPT_PLAYERTYPE_NOD) {
			ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(Get_Int_Parameter("NodID")),
				Get_Int_Parameter("NodMessage"), 0, 0);
		} else {
			ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(Get_Int_Parameter("GDIID")),
				Get_Int_Parameter("GDIMessage"), 0, 0);
		}
	}
};


/*JFW_Give_Powerup_Create

  Gives its object a powerup as soon as it exists, then goes away.

  Parameters:

  Powerup			= What to give.
*/

DECLARE_SCRIPT_TT(JFW_Give_Powerup_Create, "Powerup:string")
{
	void Created(GameObject* obj) override
	{
		ScriptEngine::Give_PowerUp(obj, Get_Parameter("Powerup"), false);
		Destroy_Script();
	}
};


/*JFW_Change_Character_Powerup

  Swaps the picker-up's character for another one.

  Parameters:

  Preset			= Character to change them into.
*/

DECLARE_SCRIPT_TT(JFW_Change_Character_Powerup, "Preset:string")
{
	void Custom(GameObject* /*obj*/, int type, int /*param*/, GameObject* sender) override
	{
		if (type == CUSTOM_EVENT_POWERUP_GRANTED) {
			ScriptEngine::Change_Character(sender, Get_Parameter("Preset"));
		}
	}
};


/*JFW_Change_Character_Powerup_2

  As above, with a different character for each side.

  Parameters:

  GDI_Preset		= Character for GDI.
  Nod_Preset		= Character for Nod.
*/

DECLARE_SCRIPT_TT(JFW_Change_Character_Powerup_2, "GDI_Preset:string,Nod_Preset:string")
{
	void Custom(GameObject* /*obj*/, int type, int /*param*/, GameObject* sender) override
	{
		if (type != CUSTOM_EVENT_POWERUP_GRANTED) {
			return;
		}

		int player_type = ScriptEngine::Get_Player_Type(sender);

		if (player_type == SCRIPT_PLAYERTYPE_NOD) {
			ScriptEngine::Change_Character(sender, Get_Parameter("Nod_Preset"));
		} else if (player_type == SCRIPT_PLAYERTYPE_GDI) {
			ScriptEngine::Change_Character(sender, Get_Parameter("GDI_Preset"));
		}
	}
};


/*JFW_Clear_Money_Create

  Takes everything its object has as soon as it exists, then goes away.
*/

DECLARE_SCRIPT_TT(JFW_Clear_Money_Create, "")
{
	void Created(GameObject* obj) override
	{
		ScriptEngine::Give_Money(obj, -ScriptEngine::Get_Money(obj), false);
		Destroy_Script();
	}
};


/*JFW_Give_Money

  Pays whoever sends the custom, or their whole team.

  Parameters:

  Message			= Custom that triggers the payment.
  Entire_Team		= Pay the sender's whole team rather than just them.
  Amount			= How much.
*/

DECLARE_SCRIPT_TT(JFW_Give_Money, "Message:int,Entire_Team:int,Amount:int")
{
	void Custom(GameObject* /*obj*/, int type, int /*param*/, GameObject* sender) override
	{
		if (type != Get_Int_Parameter("Message")) {
			return;
		}

		ScriptEngine::Give_Money(sender, (float)Get_Int_Parameter("Amount"),
			Get_Int_Parameter("Entire_Team") != 0);
	}
};
