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
*     TT_SSGM.cpp
*
* DESCRIPTION
*     The scripts the server-side game manager hangs on the world: what gets
*     written to the game log, what a repair announces, what a character leaves
*     behind, and the weather.  Native port of the 4.8.4 library's SSGM_*
*     scripts from gmgame.cpp, gmbuilding.cpp, gmsoldier.cpp and gmvehicle.cpp.
*
*     The rules these scripts obey live in SSGMSettingsClass, and the manager
*     that attaches them is SSGMManagerClass; neither is a script, because
*     neither belongs to any one object.
*
******************************************************************************/

#include "scripts.h"
#include "ssgmsettings.h"
#include "ssgmgamelog.h"
#include "vehicle.h"
#include "soldier.h"
#include "damageablegameobj.h"
#include "gameobjmanager.h"
#include "basecontroller.h"

#include <stdio.h>
#include <string.h>


/*
**	Timer ids.  The 4.8.4 numbers, so a saved game written by either finds its
**	timers where it left them.
*/
enum
{
	SSGM_TIMER_REPAIRING_COOLDOWN	= 22222,
	SSGM_TIMER_REPAIRED_COOLDOWN	= 22223,
	SSGM_TIMER_BEACON_COOLDOWN		= 222222,
	SSGM_TIMER_POWERUP_EXPIRE		= 100,
	SSGM_TIMER_WEATHER				= 0,
	SSGM_TIMER_SCORES					= 1,
	SSGM_TIMER_WRECK_SKIN			= 1000,
	SSGM_TIMER_WRECK_EXPIRE			= 1001,
	SSGM_TIMER_AMMO_CRATE			= 100100,
	SSGM_TIMER_SILO_CRATE			= 100101,
};

/*
**	The customs SSGM's own scripts send each other.
*/
enum
{
	SSGM_CUSTOM_KILL_COUNT	= 1000001,
	SSGM_CUSTOM_AMMO_CRATE	= 100100,
	SSGM_CUSTOM_SILO_CRATE	= 100101,
};

//	The colour SSGM's team announcements are in.
static const int	SSGM_ANNOUNCE_RED		= 104;
static const int	SSGM_ANNOUNCE_GREEN	= 234;
static const int	SSGM_ANNOUNCE_BLUE	= 40;


/*
**	The name to show for a thing: the operator's override where they wrote one,
**	and the game's own translated name otherwise.
*/
static void	SSGM_Display_Name(GameObject* obj, WideStringClass& name)
{
	name = L"";

	if (obj == nullptr) {
		return;
	}

	StringClass override_name;
	if (SSGMSettingsClass::Get_Translation_Override(ScriptEngine::Get_Preset_Name(obj), override_name)) {
		name = override_name.Peek_Buffer();
		return;
	}

	ScriptEngine::Get_Translated_Preset_Name(obj, name);
}


/*
**	One kill, written to the game log the way the reading tools expect it.
*/
static void	SSGM_Log_Killed(GameObject* obj, GameObject* killer, const char* object_type)
{
	Vector3 victim_pos = ScriptEngine::Get_Position(obj);
	Vector3 killer_pos = ScriptEngine::Get_Position(killer);

	WideStringClass victim_name, killer_name;
	SSGM_Display_Name(obj, victim_name);
	SSGM_Display_Name(killer, killer_name);

	SSGMGameLog::Log_Gamelog("KILLED;%s;%d;%s;%d;%d;%d;%d;%d;%s;%d;%d;%d;%d;%s;%ls;%ls;%ls",
		object_type,
		ScriptEngine::Get_ID(obj), ScriptEngine::Get_Preset_Name(obj),
		int(victim_pos.Y), int(victim_pos.X), int(victim_pos.Z), int(ScriptEngine::Get_Facing(obj)),
		ScriptEngine::Get_ID(killer), ScriptEngine::Get_Preset_Name(killer),
		int(killer_pos.Y), int(killer_pos.X), int(killer_pos.Z), int(ScriptEngine::Get_Facing(killer)),
		ScriptEngine::Get_Current_Weapon(killer),
		victim_name.Peek_Buffer(), killer_name.Peek_Buffer(),
		ScriptEngine::Get_Wide_Player_Name(obj));
}


/*
**	One damage record, which is the same shape for everything that can be hurt.
*/
static void	SSGM_Log_Damaged(GameObject* obj, GameObject* damager, float amount, const char* object_type)
{
	if (amount == 0.0f) {
		return;
	}

	Vector3 victim_pos = ScriptEngine::Get_Position(obj);
	Vector3 damager_pos = ScriptEngine::Get_Position(damager);

	SSGMGameLog::Log_Gamelog("DAMAGED;%s;%d;%s;%d;%d;%d;%d;%d;%s;%d;%d;%d;%d;%f;%d;%d",
		object_type,
		ScriptEngine::Get_ID(obj), ScriptEngine::Get_Preset_Name(obj),
		int(victim_pos.Y), int(victim_pos.X), int(victim_pos.Z), int(ScriptEngine::Get_Facing(obj)),
		ScriptEngine::Get_ID(damager), ScriptEngine::Get_Preset_Name(damager),
		int(damager_pos.Y), int(damager_pos.X), int(damager_pos.Z), int(ScriptEngine::Get_Facing(damager)),
		amount,
		int(ScriptEngine::Get_Health(obj)), int(ScriptEngine::Get_Shield_Strength(obj)));
}


static void	SSGM_Log_Created(GameObject* obj, const char* object_type)
{
	Vector3 pos = ScriptEngine::Get_Position(obj);

	SSGMGameLog::Log_Gamelog("CREATED;%s;%d;%s;%d;%d;%d;%d;%d;%d;%d",
		object_type,
		ScriptEngine::Get_ID(obj), ScriptEngine::Get_Preset_Name(obj),
		int(pos.Y), int(pos.X), int(pos.Z), int(ScriptEngine::Get_Facing(obj)),
		int(ScriptEngine::Get_Max_Health(obj)), int(ScriptEngine::Get_Max_Shield_Strength(obj)),
		ScriptEngine::Get_Player_Type(obj));
}


static void	SSGM_Log_Destroyed(GameObject* obj, const char* object_type)
{
	Vector3 pos = ScriptEngine::Get_Position(obj);

	SSGMGameLog::Log_Gamelog("DESTROYED;%s;%d;%s;%d;%d;%d",
		object_type,
		ScriptEngine::Get_ID(obj), ScriptEngine::Get_Preset_Name(obj),
		int(pos.Y), int(pos.X), int(pos.Z));
}


/*
**	"<player> is repairing the <thing>", to the repairer's own team.
*/
static void	SSGM_Announce_Repair(GameObject* obj, GameObject* repairer, const char* verb)
{
	WideStringClass thing;
	SSGM_Display_Name(obj, thing);

	StringClass message;
	message.Format("%ls %s the %ls", ScriptEngine::Get_Wide_Player_Name(repairer), verb, thing.Peek_Buffer());

	ScriptEngine::Send_Message_Team(ScriptEngine::Get_Player_Type(repairer),
		SSGM_ANNOUNCE_RED, SSGM_ANNOUNCE_GREEN, SSGM_ANNOUNCE_BLUE, message);
}


/*SSGM_Building

  Writes every building event to the game log, and tells a team when one of
  their own is being repaired.

  Attached to every building by the server manager; not something a level
  designer places.
*/

DECLARE_SCRIPT_TT(SSGM_Building, "")
{
	//
	//	Both announcements are on a cooldown, or a building under sustained
	//	repair would say so once per damage tick.
	//
	bool announce_repairing;
	bool announce_repaired;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(announce_repairing, 1);
		SAVE_VARIABLE(announce_repaired, 2);
	}

	void Created(GameObject* obj) override
	{
		announce_repairing = true;
		announce_repaired = true;
		SSGM_Log_Created(obj, "BUILDING");
	}

	void Destroyed(GameObject* obj) override
	{
		SSGM_Log_Destroyed(obj, "BUILDING");
	}

	void Damaged(GameObject* obj, GameObject* damager, float amount) override
	{
		SSGM_Log_Damaged(obj, damager, amount, "BUILDING");

		if (!SSGMSettingsClass::ShowExtraMessages) {
			return;
		}

		//	Repair arrives as negative damage from a player.
		if (amount >= 0.0f || !ScriptEngine::Is_A_Star(damager)) {
			return;
		}

		if (announce_repairing) {
			announce_repairing = false;
			SSGM_Announce_Repair(obj, damager, "is repairing");
			ScriptEngine::Start_Timer(obj, this, 30.0f, SSGM_TIMER_REPAIRING_COOLDOWN);
		}

		if (announce_repaired && ScriptEngine::Get_Health(obj) >= ScriptEngine::Get_Max_Health(obj)) {
			announce_repaired = false;
			SSGM_Announce_Repair(obj, damager, "repaired");
			ScriptEngine::Start_Timer(obj, this, 30.0f, SSGM_TIMER_REPAIRED_COOLDOWN);
		}
	}

	void Timer_Expired(GameObject* /*obj*/, int number) override
	{
		if (number == SSGM_TIMER_REPAIRING_COOLDOWN) {
			announce_repairing = true;
		} else if (number == SSGM_TIMER_REPAIRED_COOLDOWN) {
			announce_repaired = true;
		}
	}

	void Killed(GameObject* obj, GameObject* killer) override
	{
		WideStringClass building, by;
		SSGM_Display_Name(obj, building);
		SSGM_Display_Name(killer, by);

		StringClass message;
		message.Format("%ls destroyed thanks to %ls (%ls)\n", building.Peek_Buffer(),
			ScriptEngine::Get_Wide_Player_Name(killer), by.Peek_Buffer());
		SSGMGameLog::Log_Message(message, "_BUILDING");

		if (SSGMSettingsClass::BuildingDeathPages) {
			StringClass page;
			page.Format("The %ls has been destroyed thanks to you", building.Peek_Buffer());
			ScriptEngine::Send_Message_Player(killer,
				SSGM_ANNOUNCE_RED, SSGM_ANNOUNCE_GREEN, SSGM_ANNOUNCE_BLUE, page);
		}

		SSGM_Log_Killed(obj, killer, "BUILDING");
	}
};


/*SSGM_Soldier

  Writes every player event to the game log, hands out whatever the operator
  configured this character to spawn with, and keeps the kill streak.

  Attached to every soldier by the server manager.
*/

DECLARE_SCRIPT_TT(SSGM_Soldier, "")
{
	int kills;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(kills, 1);
	}

	void Created(GameObject* obj) override
	{
		kills = 0;

		if (ScriptEngine::Is_A_Star(obj)) {

			//
			//	A forced team is applied to the player rather than to the
			//	character, so this happens before anything is granted: the
			//	change replaces the object.
			//
			if (SSGMSettingsClass::ForceTeam != -1
					&& ScriptEngine::Get_Player_Type(obj) != SSGMSettingsClass::ForceTeam) {
				ScriptEngine::Change_Team(obj, SSGMSettingsClass::ForceTeam, false);
				return;
			}

			if (SSGMSettingsClass::CharactersDropDNA) {
				ScriptEngine::Attach_Script_Once(obj, "SSGM_Drop_DNA_Powerup", "");
			}

			//
			//	Voting on a key rather than by typing.  The other three keys
			//	4.8.4 attached here -- the mine count and the two vehicle
			//	claims -- are answered by the server manager beside the chat
			//	commands that do the same thing; see docs/tt484/SSGM.md.
			//
			ScriptEngine::Attach_Script(obj, "SSGM_Log_Key", "VoteYes,!vote yes");
			ScriptEngine::Attach_Script(obj, "SSGM_Log_Key", "VoteNo,!vote no");
		}

		const DynamicVectorClass<StringClass>* granted
				= SSGMSettingsClass::Get_Weapon_Grants(ScriptEngine::Get_Preset_Name(obj));

		if (granted != nullptr) {
			for (int index = 0; index < granted->Count(); index++) {
				ScriptEngine::Give_PowerUp(obj, (*granted)[index], false);
			}
		}

		if (ScriptEngine::Is_A_Star(obj)) {
			WideStringClass character;
			SSGM_Display_Name(obj, character);

			StringClass message;
			message.Format("%ls changed character to a %ls\n",
				ScriptEngine::Get_Wide_Player_Name(obj), character.Peek_Buffer());
			SSGMGameLog::Log_Message(message, "_CHANGECHARACTER");
		}

		Vector3 pos = ScriptEngine::Get_Position(obj);
		SSGMGameLog::Log_Gamelog("CREATED;SOLDIER;%d;%s;%d;%d;%d;%d;%d;%d;%d;%ls",
			ScriptEngine::Get_ID(obj), ScriptEngine::Get_Preset_Name(obj),
			int(pos.Y), int(pos.X), int(pos.Z), int(ScriptEngine::Get_Facing(obj)),
			int(ScriptEngine::Get_Max_Health(obj)), int(ScriptEngine::Get_Max_Shield_Strength(obj)),
			ScriptEngine::Get_Player_Type(obj), ScriptEngine::Get_Wide_Player_Name(obj));
	}

	void Destroyed(GameObject* obj) override
	{
		Vector3 pos = ScriptEngine::Get_Position(obj);
		SSGMGameLog::Log_Gamelog("DESTROYED;SOLDIER;%d;%s;%d;%d;%d;%d",
			ScriptEngine::Get_ID(obj), ScriptEngine::Get_Preset_Name(obj),
			int(pos.Y), int(pos.X), int(pos.Z), int(ScriptEngine::Get_Points(obj)));
	}

	void Damaged(GameObject* obj, GameObject* damager, float amount) override
	{
		SSGM_Log_Damaged(obj, damager, amount, "SOLDIER");
	}

	//
	//	The streak.  Every second kill from three up has a name; fifteen wraps
	//	back to nothing so the next streak starts over.
	//
	void Announce_Streak(GameObject* obj)
	{
		struct StreakStruct { int Kills; const char *Sound; const char *Text; bool Everybody; };

		static const StreakStruct STREAKS[] = {
			{  3, "multikill.wav",		"Multikill!",					false },
			{  5, "killingspree.wav",	"Killingspree!",				false },
			{  7, "rampage.wav",			"Rampage!",						false },
			{  9, "ultrakill.wav",		"UltraKill!",					false },
			{ 11, "holyshit.wav",		"achieved Holy Shit!!! (11 kills)",			true },
			{ 13, "monsterkill.wav",	"achieved M-M-M-M-Monster Kill!!! (13 kills)",	true },
			{ 15, "godlike.wav",			"achieved GODLIKE!!! (15 kills)",			true },
		};

		for (int index = 0; index < (int)(sizeof(STREAKS) / sizeof(STREAKS[0])); index++) {

			if (STREAKS[index].Kills != kills) {
				continue;
			}

			if (STREAKS[index].Everybody) {
				StringClass message;
				message.Format("%ls %s", ScriptEngine::Get_Wide_Player_Name(obj), STREAKS[index].Text);
				ScriptEngine::Create_2D_WAV_Sound(STREAKS[index].Sound);
				ScriptEngine::Send_Message(255, 255, 255, message);
			} else {
				ScriptEngine::Create_2D_WAV_Sound_Player(obj, STREAKS[index].Sound);
				ScriptEngine::Send_Message_Player(obj,
					SSGM_ANNOUNCE_RED, SSGM_ANNOUNCE_GREEN, SSGM_ANNOUNCE_BLUE, STREAKS[index].Text);
			}

			if (STREAKS[index].Kills == 15) {
				kills = 0;
			}

			return;
		}
	}

	void Custom(GameObject* obj, int message, intptr_t param, GameObject* /*sender*/) override
	{
		if (message == SSGM_CUSTOM_KILL_COUNT && param == 1) {
			if (SSGMSettingsClass::PlayQuakeSounds) {
				kills++;
				Announce_Streak(obj);
			}
		} else if (message == SSGM_CUSTOM_AMMO_CRATE && param == 1) {
			ScriptEngine::Give_PowerUp(obj, "CnC_Ammo_Crate", false);
			ScriptEngine::Start_Timer(obj, this, 60.0f, SSGM_TIMER_AMMO_CRATE);
		} else if (message == SSGM_CUSTOM_SILO_CRATE && param == 1) {
			ScriptEngine::Start_Timer(obj, this, 1.0f, SSGM_TIMER_SILO_CRATE);
		}
	}

	void Timer_Expired(GameObject* obj, int number) override
	{
		if (number == SSGM_TIMER_AMMO_CRATE) {
			ScriptEngine::Give_PowerUp(obj, "CnC_Ammo_Crate", false);
			ScriptEngine::Start_Timer(obj, this, 60.0f, SSGM_TIMER_AMMO_CRATE);
		} else if (number == SSGM_TIMER_SILO_CRATE) {
			ScriptEngine::Give_Money(obj, 1.0f, true);
			ScriptEngine::Start_Timer(obj, this, 1.0f, SSGM_TIMER_SILO_CRATE);
		}
	}

	void Killed(GameObject* obj, GameObject* killer) override
	{
		SSGM_Log_Killed(obj, killer, "SOLDIER");

		if (!ScriptEngine::Is_A_Star(obj)) {
			return;
		}

		WideStringClass victim, by;
		SSGM_Display_Name(obj, victim);
		SSGM_Display_Name(killer, by);

		StringClass message;
		if (killer == nullptr) {
			message.Format("%ls was killed (%ls)\n", ScriptEngine::Get_Wide_Player_Name(obj),
				victim.Peek_Buffer());
		} else if (killer == obj) {
			message.Format("%ls killed themselves (%ls)\n", ScriptEngine::Get_Wide_Player_Name(obj),
				victim.Peek_Buffer());
		} else if (!ScriptEngine::Is_A_Star(killer)) {
			message.Format("The %ls killed %ls (%ls)\n", by.Peek_Buffer(),
				ScriptEngine::Get_Wide_Player_Name(obj), victim.Peek_Buffer());
		} else {
			message.Format("%ls killed %ls (%ls VS. %ls)\n",
				ScriptEngine::Get_Wide_Player_Name(killer), ScriptEngine::Get_Wide_Player_Name(obj),
				by.Peek_Buffer(), victim.Peek_Buffer());
		}
		SSGMGameLog::Log_Message(message, "_PLAYERKILL");

		if (SSGMSettingsClass::ExtraKillMessages) {
			StringClass announcement;

			if (killer == nullptr) {
				announcement.Format("%ls was killed", ScriptEngine::Get_Wide_Player_Name(obj));
				ScriptEngine::Send_Message_With_Team_Color(ScriptEngine::Get_Player_Type(obj), announcement);
			} else if (killer == obj) {
				announcement.Format("%ls killed themselves", ScriptEngine::Get_Wide_Player_Name(obj));
				ScriptEngine::Send_Message_With_Team_Color(ScriptEngine::Get_Player_Type(obj), announcement);
			} else if (!ScriptEngine::Is_A_Star(killer)) {
				announcement.Format("The %ls killed %ls", by.Peek_Buffer(),
					ScriptEngine::Get_Wide_Player_Name(obj));
				ScriptEngine::Send_Message_With_Team_Color(ScriptEngine::Get_Player_Type(killer), announcement);
			}
		}

		if (killer != nullptr && killer != obj) {
			ScriptEngine::Send_Custom_Event(obj, killer, SSGM_CUSTOM_KILL_COUNT, 1, 0);
		} else if (killer == obj && SSGMSettingsClass::PlayQuakeSounds) {
			ScriptEngine::Create_2D_WAV_Sound("humiliation.wav");
		}

		//
		//	Whatever this character was configured to leave on the ground.
		//
		const DynamicVectorClass<StringClass>* drops
				= SSGMSettingsClass::Get_Weapon_Drops(ScriptEngine::Get_Preset_Name(obj));

		if (drops != nullptr && drops->Count() > 0) {
			int pick = ScriptEngine::Get_Random_Int(0, drops->Count());
			GameObject* powerup = ScriptEngine::Create_Object((*drops)[pick], ScriptEngine::Get_Position(obj));
			if (powerup != nullptr) {
				ScriptEngine::Attach_Script_Once(powerup, "SSGM_Powerup_Expire", "");
			}
		}
	}
};


/*SSGM_Vehicle

  Writes every vehicle event to the game log, tells an owner what is happening
  to their vehicle, and leaves a wreck where the operator asked for one.

  Attached to every vehicle by the server manager.
*/

DECLARE_SCRIPT_TT(SSGM_Vehicle, "")
{
	bool announce_repairing;
	bool announce_repaired;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(announce_repairing, 1);
		SAVE_VARIABLE(announce_repaired, 2);
	}

	//
	//	A base defence turret is a vehicle to the engine and a building to
	//	everybody reading the log.
	//
	const char* Log_Type(GameObject* obj)
	{
		VehicleGameObj* vehicle = (obj != nullptr) ? obj->As_VehicleGameObj() : nullptr;
		return (vehicle != nullptr && vehicle->Is_Turret()) ? "BUILDING" : "VEHICLE";
	}

	GameObject* Claimant(GameObject* obj)
	{
		VehicleGameObj* vehicle = (obj != nullptr) ? obj->As_VehicleGameObj() : nullptr;
		return (vehicle != nullptr) ? vehicle->Get_Claimant() : nullptr;
	}

	void Created(GameObject* obj) override
	{
		announce_repairing = true;
		announce_repaired = true;
		SSGM_Log_Created(obj, Log_Type(obj));
	}

	void Destroyed(GameObject* obj) override
	{
		SSGM_Log_Destroyed(obj, Log_Type(obj));
	}

	void Damaged(GameObject* obj, GameObject* damager, float amount) override
	{
		SSGM_Log_Damaged(obj, damager, amount, Log_Type(obj));

		if (!SSGMSettingsClass::ShowExtraMessages) {
			return;
		}

		//
		//	Only the harvester is announced: it is the one vehicle a team
		//	shares, so its repair is everybody's business.
		//
		VehicleGameObj* vehicle = obj->As_VehicleGameObj();
		if (vehicle == nullptr || vehicle->Get_Claimant() != nullptr) {
			return;
		}

		if (amount >= 0.0f || !ScriptEngine::Is_A_Star(damager)) {
			return;
		}

		BaseControllerClass* base = BaseControllerClass::Find_Base(ScriptEngine::Get_Player_Type(obj));
		if (base == nullptr || base->Get_Harvester_Vehicle() != vehicle) {
			return;
		}

		if (announce_repairing) {
			announce_repairing = false;
			SSGM_Announce_Repair(obj, damager, "is repairing");
			ScriptEngine::Start_Timer(obj, this, 30.0f, SSGM_TIMER_REPAIRING_COOLDOWN);
		}

		if (announce_repaired
				&& ScriptEngine::Get_Shield_Strength(obj) >= ScriptEngine::Get_Max_Shield_Strength(obj)) {
			announce_repaired = false;
			SSGM_Announce_Repair(obj, damager, "repaired");
			ScriptEngine::Create_2D_WAV_Sound_Team(ScriptEngine::Get_Player_Type(damager),
				"m00evag_dsgn0019i1evag_snd.wav");
			ScriptEngine::Start_Timer(obj, this, 30.0f, SSGM_TIMER_REPAIRED_COOLDOWN);
		}
	}

	void Timer_Expired(GameObject* /*obj*/, int number) override
	{
		if (number == SSGM_TIMER_REPAIRING_COOLDOWN) {
			announce_repairing = true;
		} else if (number == SSGM_TIMER_REPAIRED_COOLDOWN) {
			announce_repaired = true;
		}
	}

	void Custom(GameObject* obj, int type, intptr_t /*param*/, GameObject* sender) override
	{
		Vector3 pos = ScriptEngine::Get_Position(obj);
		Vector3 sender_pos = ScriptEngine::Get_Position(sender);

		if (type == CUSTOM_EVENT_VEHICLE_ENTERED) {

			if (SSGMSettingsClass::VehicleOwnership) {

				GameObject* owner = Claimant(obj);

				if (owner != nullptr && owner != sender) {
					if (ScriptEngine::Get_Player_Type(owner) != ScriptEngine::Get_Player_Type(sender)) {
						ScriptEngine::Send_Message_Player(owner, SSGM_ANNOUNCE_RED, SSGM_ANNOUNCE_GREEN,
							SSGM_ANNOUNCE_BLUE, "The enemy has stolen your vehicle.");
						obj->As_VehicleGameObj()->Set_Claimant(nullptr);
						obj->As_VehicleGameObj()->Set_Owner(nullptr);
					} else if (ScriptEngine::Get_Vehicle_Occupant(obj, 0) == sender) {
						StringClass message;
						message.Format("Player %ls has entered your vehicle.",
							ScriptEngine::Get_Wide_Player_Name(sender));
						ScriptEngine::Send_Message_Player(owner, SSGM_ANNOUNCE_RED, SSGM_ANNOUNCE_GREEN,
							SSGM_ANNOUNCE_BLUE, message);
					}
				}
			}

			SSGMGameLog::Log_Gamelog("ENTER;%d;%s;%d;%d;%d;%d;%s;%d;%d;%d",
				ScriptEngine::Get_ID(obj), ScriptEngine::Get_Preset_Name(obj),
				int(pos.Y), int(pos.X), int(pos.Z),
				ScriptEngine::Get_ID(sender), ScriptEngine::Get_Preset_Name(sender),
				int(sender_pos.Y), int(sender_pos.X), int(sender_pos.Z));

		} else if (type == CUSTOM_EVENT_VEHICLE_EXITED) {

			SSGMGameLog::Log_Gamelog("EXIT;%d;%s;%d;%d;%d;%d;%s;%d;%d;%d",
				ScriptEngine::Get_ID(obj), ScriptEngine::Get_Preset_Name(obj),
				int(pos.Y), int(pos.X), int(pos.Z),
				ScriptEngine::Get_ID(sender), ScriptEngine::Get_Preset_Name(sender),
				int(sender_pos.Y), int(sender_pos.X), int(sender_pos.Z));
		}
	}

	void Killed(GameObject* obj, GameObject* killer) override
	{
		GameObject* owner = Claimant(obj);

		if (owner != nullptr && SSGMSettingsClass::VehicleOwnership) {
			ScriptEngine::Send_Message_Player(owner, SSGM_ANNOUNCE_RED, SSGM_ANNOUNCE_GREEN,
				SSGM_ANNOUNCE_BLUE, "Your vehicle was destroyed.");
		}

		//
		//	The wreck, if the operator configured one for this vehicle.  It is
		//	told the preset it came from so it can put it back when repaired.
		//
		StringClass wreckage;
		if (SSGMSettingsClass::Get_Vehicle_Wreckage(ScriptEngine::Get_Preset_Name(obj), wreckage)) {
			GameObject* wreck = ScriptEngine::Create_Object(wreckage, ScriptEngine::Get_Position(obj));
			if (wreck != nullptr) {
				ScriptEngine::Set_Facing(wreck, ScriptEngine::Get_Facing(obj));
				ScriptEngine::Attach_Script(wreck, "SSGM_Vehicle_Wreckage", ScriptEngine::Get_Preset_Name(obj));
			}
		}

		WideStringClass vehicle_name, killer_name;
		SSGM_Display_Name(obj, vehicle_name);
		SSGM_Display_Name(killer, killer_name);

		StringClass message;
		const char* log_type = Log_Type(obj);

		if (::strcmp(log_type, "BUILDING") == 0) {
			message.Format("%ls destroyed thanks to %ls (%ls)\n", vehicle_name.Peek_Buffer(),
				ScriptEngine::Get_Wide_Player_Name(killer), killer_name.Peek_Buffer());
			SSGMGameLog::Log_Message(message, "_BUILDING");
		} else {
			GameObject* driver = ScriptEngine::Get_Vehicle_Occupant(obj, 0);
			message.Format("%ls (%ls) destroyed a %ls (Driver: %ls - Owner: %ls)\n",
				ScriptEngine::Get_Wide_Player_Name(killer), killer_name.Peek_Buffer(),
				vehicle_name.Peek_Buffer(),
				ScriptEngine::Get_Wide_Player_Name(driver),
				ScriptEngine::Get_Wide_Player_Name(owner));
			SSGMGameLog::Log_Message(message, "_VEHKILL");
		}

		SSGM_Log_Killed(obj, killer, log_type);
	}
};


/*SSGM_Vehicle_Wreckage

  A burnt-out hull.  It cannot be shot apart, it is worth nothing, and it
  disappears on its own; repair it to full and the vehicle it came from comes
  back with the last of its health.

  Parameters:

  preset			= The vehicle this is the wreck of.
*/

DECLARE_SCRIPT_TT(SSGM_Vehicle_Wreckage, "preset:string")
{
	void Created(GameObject* obj) override
	{
		//
		//	Blamo armour for the first two seconds: the explosion that made
		//	this wreck is still going off, and would otherwise destroy it
		//	the instant it appeared.
		//
		ScriptEngine::Set_Skin(obj, "Blamo");
		ScriptEngine::Start_Timer(obj, this, 2.0f, SSGM_TIMER_WRECK_SKIN);
		ScriptEngine::Start_Timer(obj, this, SSGMSettingsClass::WreckDestroySelfTime, SSGM_TIMER_WRECK_EXPIRE);

		ScriptEngine::Set_Max_Health(obj, ScriptEngine::Get_Health(obj) + ScriptEngine::Get_Shield_Strength(obj));
		ScriptEngine::Set_Max_Shield_Strength(obj, 0.0f);
		ScriptEngine::Set_Health(obj, 100.0f);

		ScriptEngine::Disable_Physical_Collisions(obj);
		ScriptEngine::Set_Player_Type(obj, -2);
		ScriptEngine::Set_Death_Points(obj, 0.0f);
		ScriptEngine::Set_Damage_Points(obj, 0.0f);
	}

	void Timer_Expired(GameObject* obj, int number) override
	{
		if (number == SSGM_TIMER_WRECK_SKIN) {
			ScriptEngine::Set_Skin(obj, "SkinVehicleLight");
		} else if (number == SSGM_TIMER_WRECK_EXPIRE) {
			ScriptEngine::Destroy_Object(obj);
		}
	}

	void Damaged(GameObject* obj, GameObject* /*damager*/, float amount) override
	{
		if (amount >= 0.0f) {
			return;
		}

		if (ScriptEngine::Get_Health(obj) < ScriptEngine::Get_Max_Health(obj)) {
			return;
		}

		//
		//	Repaired to full: the wreck goes and the vehicle comes back, just
		//	alive.  The area damage clears whatever was standing where the
		//	vehicle is about to be.
		//
		Vector3 pos = ScriptEngine::Get_Position(obj);
		pos.Z += 3.0f;
		float facing = ScriptEngine::Get_Facing(obj);
		StringClass preset = Get_Parameter("preset");

		ScriptEngine::Destroy_Object(obj);
		ScriptEngine::Damage_All_Objects_Area(999999.0f, "Laser_NoBuilding", pos, 5.0f, 2, nullptr);

		GameObject* vehicle = ScriptEngine::Create_Object(preset, pos);
		if (vehicle != nullptr) {
			ScriptEngine::Set_Facing(vehicle, facing);
			ScriptEngine::Set_Health(vehicle, 1.0f);
			ScriptEngine::Set_Shield_Strength(vehicle, 0.0f);
			ScriptEngine::Set_Player_Type(vehicle, -2);
		}
	}
};


/*SSGM_Log_Key

  Writes a fixed line to the server log when the player presses a named key.
  The line is whatever the level author put in `Write`, prefixed with the
  player's name, so a moderation tool watching the log sees "Havoc: !vote yes"
  and can act on it without the player having typed anything.

  `Key` is a logical key name, not a key: what it is bound to is the player's
  business, and the client is never told the key means anything.  See
  Code/Combat/scriptkeys.h.
*/

REGISTER_SCRIPT_TT(SSGM_Log_Key, "Key:string,Write:string")
class SSGM_Log_Key : public KeyHookScriptClass
{
	void Created(GameObject* obj) override
	{
		Install_Hook(Get_Parameter("Key"), obj);
	}

	void Key_Hook(void) override
	{
		StringClass message;
		message.Format("%ls: %s", ScriptEngine::Get_Wide_Player_Name(Owner()),
			Get_Parameter("Write"));

		SSGMGameLog::Log_Ren_Log(message);
	}
};


/*SSGM_Log_Mine

  Writes what happens to one piece of C4 to the game log, and tells a team when
  one of their people disarms an enemy mine.
*/

DECLARE_SCRIPT_TT(SSGM_Log_Mine, "")
{
	bool was_disarmed;
	bool was_detonated;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(was_disarmed, 1);
		SAVE_VARIABLE(was_detonated, 2);
	}

	void Created(GameObject* obj) override
	{
		was_disarmed = false;
		was_detonated = false;

		Vector3 pos = ScriptEngine::Get_Position(obj);
		SSGMGameLog::Log_Gamelog("CREATED;OBJECT;%d;%s;%d;%d;%d;%d;%d;%d;%d;%d",
			ScriptEngine::Get_ID(obj), ScriptEngine::Get_Preset_Name(obj),
			int(pos.Y), int(pos.X), int(pos.Z), int(ScriptEngine::Get_Facing(obj)),
			int(ScriptEngine::Get_Max_Health(obj)), int(ScriptEngine::Get_Max_Shield_Strength(obj)),
			ScriptEngine::Get_Player_Type(obj),
			ScriptEngine::Get_ID(ScriptEngine::Get_C4_Planter(obj)));
	}

	void Damaged(GameObject* obj, GameObject* damager, float amount) override
	{
		//
		//	Its own planter poking it for no damage is how remote C4 is set
		//	off; that is not somebody attacking it.
		//
		if (amount == 0.0f && damager == ScriptEngine::Get_C4_Planter(obj)) {
			was_detonated = true;
			return;
		}

		SSGM_Log_Damaged(obj, damager, amount, "OBJECT");
	}

	void Killed(GameObject* obj, GameObject* killer) override
	{
		was_disarmed = true;

		SSGM_Log_Killed(obj, killer, "OBJECT");

		WideStringClass mine, attached;
		SSGM_Display_Name(obj, mine);
		SSGM_Display_Name(ScriptEngine::Get_C4_Attached(obj), attached);

		StringClass message;
		message.Format("%ls has been disarmed by %ls (Owner: %ls - Attached to: %ls)\n",
			mine.Peek_Buffer(), ScriptEngine::Get_Wide_Player_Name(killer),
			ScriptEngine::Get_Wide_Player_Name(ScriptEngine::Get_C4_Planter(obj)),
			attached.Peek_Buffer());
		SSGMGameLog::Log_Message(message, "_C4");

		if (SSGMSettingsClass::ShowExtraMessages) {
			StringClass announcement;
			announcement.Format("%ls disarmed a %ls",
				ScriptEngine::Get_Wide_Player_Name(killer), mine.Peek_Buffer());
			ScriptEngine::Send_Message_Team(ScriptEngine::Get_Player_Type(killer),
				SSGM_ANNOUNCE_RED, SSGM_ANNOUNCE_GREEN, SSGM_ANNOUNCE_BLUE, announcement);
		}
	}

	void Destroyed(GameObject* obj) override
	{
		Vector3 pos = ScriptEngine::Get_Position(obj);

		if (!was_disarmed && was_detonated) {
			WideStringClass mine, attached;
			SSGM_Display_Name(obj, mine);
			SSGM_Display_Name(ScriptEngine::Get_C4_Attached(obj), attached);

			StringClass message;
			message.Format("%ls has detonated (Owner: %ls - Attached to: %ls)\n",
				mine.Peek_Buffer(),
				ScriptEngine::Get_Wide_Player_Name(ScriptEngine::Get_C4_Planter(obj)),
				attached.Peek_Buffer());
			SSGMGameLog::Log_Message(message, "_C4");

			SSGMGameLog::Log_Gamelog("DETONATED;OBJECT;%d;%s;%d;%d;%d",
				ScriptEngine::Get_ID(obj), ScriptEngine::Get_Preset_Name(obj),
				int(pos.Y), int(pos.X), int(pos.Z));
		}

		SSGM_Log_Destroyed(obj, "OBJECT");
	}
};


/*SSGM_Log_Beacon

  The same for a beacon, plus telling everybody when one is being disarmed --
  which is everybody's business, because the map may be about to end.
*/

DECLARE_SCRIPT_TT(SSGM_Log_Beacon, "")
{
	bool was_disarmed;
	bool announce_disarming;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(was_disarmed, 1);
		SAVE_VARIABLE(announce_disarming, 2);
	}

	void Created(GameObject* obj) override
	{
		was_disarmed = false;
		announce_disarming = true;

		Vector3 pos = ScriptEngine::Get_Position(obj);
		SSGMGameLog::Log_Gamelog("CREATED;OBJECT;%d;%s;%d;%d;%d;%d;%d;%d;%d;%d",
			ScriptEngine::Get_ID(obj), ScriptEngine::Get_Preset_Name(obj),
			int(pos.Y), int(pos.X), int(pos.Z), int(ScriptEngine::Get_Facing(obj)),
			int(ScriptEngine::Get_Max_Health(obj)), int(ScriptEngine::Get_Max_Shield_Strength(obj)),
			ScriptEngine::Get_Player_Type(obj),
			ScriptEngine::Get_ID(ScriptEngine::Get_Beacon_Planter(obj)));

		//
		//	A beacon exists because somebody just deployed it, so this is the
		//	deployment.  4.8.4 had a separate custom for it because its own
		//	hook could not see the creation.
		//
		WideStringClass beacon;
		SSGM_Display_Name(obj, beacon);

		StringClass message;
		message.Format("%ls deployed a %ls\n",
			ScriptEngine::Get_Wide_Player_Name(ScriptEngine::Get_Beacon_Planter(obj)),
			beacon.Peek_Buffer());
		SSGMGameLog::Log_Message(message, "_BEACON");
	}

	void Damaged(GameObject* obj, GameObject* damager, float amount) override
	{
		SSGM_Log_Damaged(obj, damager, amount, "OBJECT");

		if (!SSGMSettingsClass::ShowExtraMessages || !announce_disarming || amount <= 0.0f) {
			return;
		}

		announce_disarming = false;

		WideStringClass beacon;
		SSGM_Display_Name(obj, beacon);

		StringClass announcement;
		announcement.Format("%ls is disarming a %ls",
			ScriptEngine::Get_Wide_Player_Name(damager), beacon.Peek_Buffer());
		ScriptEngine::Send_Message_Team(ScriptEngine::Get_Player_Type(damager),
			SSGM_ANNOUNCE_RED, SSGM_ANNOUNCE_GREEN, SSGM_ANNOUNCE_BLUE, announcement);

		ScriptEngine::Start_Timer(obj, this, 15.0f, SSGM_TIMER_BEACON_COOLDOWN);
	}

	void Timer_Expired(GameObject* /*obj*/, int number) override
	{
		if (number == SSGM_TIMER_BEACON_COOLDOWN) {
			announce_disarming = true;
		}
	}

	void Killed(GameObject* obj, GameObject* killer) override
	{
		was_disarmed = true;

		SSGM_Log_Killed(obj, killer, "OBJECT");

		WideStringClass beacon;
		SSGM_Display_Name(obj, beacon);

		StringClass message;
		message.Format("%ls disarmed a %ls\n",
			ScriptEngine::Get_Wide_Player_Name(killer), beacon.Peek_Buffer());
		SSGMGameLog::Log_Message(message, "_BEACON");

		if (SSGMSettingsClass::ShowExtraMessages) {
			StringClass announcement;
			announcement.Format("%ls disarmed a %ls",
				ScriptEngine::Get_Wide_Player_Name(killer), beacon.Peek_Buffer());
			ScriptEngine::Send_Message(SSGM_ANNOUNCE_RED, SSGM_ANNOUNCE_GREEN,
				SSGM_ANNOUNCE_BLUE, announcement);
		}
	}

	void Destroyed(GameObject* obj) override
	{
		if (!was_disarmed) {
			WideStringClass beacon;
			SSGM_Display_Name(obj, beacon);

			StringClass message;
			message.Format("%ls has detonated\n", beacon.Peek_Buffer());
			SSGMGameLog::Log_Message(message, "_BEACON");
		}

		SSGM_Log_Destroyed(obj, "OBJECT");
	}
};


/*SSGM_Log_Scores

  Samples everybody's score and money once a minute into the game log.
*/

DECLARE_SCRIPT_TT(SSGM_Log_Scores, "")
{
	void Created(GameObject* obj) override
	{
		ScriptEngine::Start_Timer(obj, this, 60.0f, SSGM_TIMER_SCORES);
	}

	void Timer_Expired(GameObject* obj, int number) override
	{
		if (number != SSGM_TIMER_SCORES) {
			return;
		}

		ScriptEngine::Start_Timer(obj, this, 60.0f, SSGM_TIMER_SCORES);

		for (SLNode<SoldierGameObj>* node = GameObjManager::Get_Star_Game_Obj_List()->Head();
				node != nullptr; node = node->Next()) {

			SoldierGameObj* player = node->Data();
			if (player == nullptr) {
				continue;
			}

			SSGMGameLog::Log_Gamelog("SCORE;%d;%d;%d", ScriptEngine::Get_ID(player),
				(int)ScriptEngine::Get_Points(player), (int)ScriptEngine::Get_Money(player));
		}
	}
};


/*SSGM_Random_Weather

  Drifts the cloud cover up and down at random and hangs the rest of the
  weather off it, so a map that asked for rain gets rain that comes and goes.
*/

DECLARE_SCRIPT_TT(SSGM_Random_Weather, "")
{
	float target_clouds;
	float current_clouds;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(target_clouds, 1);
		SAVE_VARIABLE(current_clouds, 2);
	}

	void Created(GameObject* obj) override
	{
		target_clouds = ScriptEngine::Get_Random(0.0f, 1.0f);
		current_clouds = ScriptEngine::Get_Random(0.0f, 1.0f);
		ScriptEngine::Set_Clouds(current_clouds, current_clouds, true);
		ScriptEngine::Start_Timer(obj, this, 0.5f, SSGM_TIMER_WEATHER);
	}

	void Timer_Expired(GameObject* obj, int number) override
	{
		if (number != SSGM_TIMER_WEATHER) {
			return;
		}

		//
		//	Creep towards the target a thousandth at a time; when it is
		//	reached, pick a new one.
		//
		const float STEP = 0.001f;

		if (current_clouds < target_clouds) {
			current_clouds = ((current_clouds + STEP) >= target_clouds)
					? target_clouds : (current_clouds + STEP);
		} else if (current_clouds > target_clouds) {
			current_clouds = ((current_clouds - STEP) <= target_clouds)
					? target_clouds : (current_clouds - STEP);
		}

		ScriptEngine::Set_Clouds(current_clouds, current_clouds, true);
		ScriptEngine::Set_Wind(0.0f, 10.0f * current_clouds, 1.0f, true);

		//
		//	Nothing falls out of a clear sky: below 0.625 cover, the
		//	precipitation for this map's weather is zero.
		//
		bool overcast = (current_clouds >= 0.625f);
		float amount = overcast ? (((8.0f * current_clouds) - 5.0f) / 3.0f) : 0.0f;

		switch (SSGMSettingsClass::WeatherType)
		{
			case SSGM_WEATHER_SNOW:
				ScriptEngine::Set_Snow(amount, amount, true);
				break;

			case SSGM_WEATHER_ASH:
				ScriptEngine::Set_Ash(amount, amount, true);
				break;

			default:
			{
				float rain = overcast ? ((1.875f * current_clouds) - 0.125f) : 0.0f;
				ScriptEngine::Set_Rain(rain, rain, true);

				float lightning = (current_clouds >= 0.875f)
						? ((8.0f * (current_clouds + 0.125f)) - 8.0f) : 0.0f;
				ScriptEngine::Set_Lightning(lightning, 0.0f, 1.0f, 0.0f, 1.0f, true);
				break;
			}
		}

		if (current_clouds == target_clouds) {
			target_clouds = ScriptEngine::Get_Random(0.0f, 1.0f);
		}

		ScriptEngine::Start_Timer(obj, this, 0.5f, SSGM_TIMER_WEATHER);
	}
};


/*SSGM_Powerup_Expire

  Takes a dropped powerup away after the configured time, so a busy map does
  not fill up with them.
*/

DECLARE_SCRIPT_TT(SSGM_Powerup_Expire, "")
{
	void Created(GameObject* obj) override
	{
		ScriptEngine::Start_Timer(obj, this, SSGMSettingsClass::PowerupExpireTime, SSGM_TIMER_POWERUP_EXPIRE);
	}

	void Timer_Expired(GameObject* obj, int number) override
	{
		if (number == SSGM_TIMER_POWERUP_EXPIRE) {
			ScriptEngine::Expire_Powerup(obj);
			ScriptEngine::Destroy_Object(obj);
		}
	}
};


/*
**	What a picked-up weapon is called, and the acknowledgement it plays.
**
**	4.8.4 wrote this out as a two-hundred-line chain of preset-name tests; it
**	is a table, and the only thing that varies down it is these three strings.
**	The fourth column is the weapon a stealth black hand loses when the server
**	will not let one carry dropped weapons.
*/
struct SSGMPowerupSoundStruct
{
	const char *	Preset;
	const char *	Text;
	const char *	Sound;
	const char *	DeniedWeapon;
};

static const SSGMPowerupSoundStruct	SSGM_POWERUP_SOUNDS[] =
{
	{ "CnC_POW_MineRemote_02",			"Remote C4 acquired.",			"m00pac4_aqob0004i1evag_snd.wav",	"CnC_Weapon_MineRemote_Player_2Max" },
	{ "CnC_MineProximity_05",			"Proximity C4 acquired.",		"m00pacp_aqob0004i1evag_snd.wav",	"Weapon_MineProximity_Player" },
	{ "CnC_POW_MineTimed_Player_02",	"Timed C4 acquired.",			"m00pact_aqob0004i1evag_snd.wav",	"CnC_Weapon_MineTimed_Player_2Max" },
	{ "POW_Chaingun_Player",			"Chaingun acquired.",			"m00pwcg_aqob0004i1evag_snd.wav",	"Weapon_Chaingun_Player" },
	{ "POW_ChemSprayer_Player",		"Chemsprayer acquired.",		"m00pwcs_aqob0004i1evag_snd.wav",	"Weapon_ChemSprayer_Player" },
	{ "POW_Flamethrower_Player",		"Flamethrower acquired.",		"m00pwft_aqob0001i1evag_snd.wav",	"Weapon_Flamethrower_Player" },
	{ "POW_GrenadeLauncher_Player",	"Grenade Launcher acquired.",	"m00pwgl_aqob0004i1evag_snd.wav",	"Weapon_GrenadeLauncher_Player" },
	{ "POW_LaserChaingun_Player",		"Laser Chaingun acquired.",	"m00pwlc_aqob0004i1evag_snd.wav",	"Weapon_LaserChaingun_Player" },
	{ "CnC_POW_IonCannonBeacon_Player",	"Ion Cannon Beacon acquired.",	"m00paib_aqob0004i1evag_snd.wav",	nullptr },
	{ "POW_IonCannonBeacon_Player",	"10 Second Superweapon Beacon acquired.",	"m00paib_aqob0004i1evag_snd.wav",	nullptr },
	{ "POW_Nuclear_Missle_Beacon",	"10 Second Superweapon Beacon acquired.",	"nukeavail.wav",	nullptr },
};

//	The one character a server may keep dropped weapons away from.
static const char * const	SSGM_STEALTH_SOLDIER	= "CnC_Nod_FlameThrower_2SF";


/*SSGM_Powerup

  Says what a player just picked up, and takes it straight back off a stealth
  black hand when the server does not allow that.
*/

DECLARE_SCRIPT_TT(SSGM_Powerup, "")
{
	void Custom(GameObject* obj, int message, intptr_t /*param*/, GameObject* sender) override
	{
		if (message != CUSTOM_EVENT_POWERUP_GRANTED || sender == nullptr) {
			return;
		}

		const char* preset = ScriptEngine::Get_Preset_Name(obj);

		for (int index = 0; index < (int)(sizeof(SSGM_POWERUP_SOUNDS) / sizeof(SSGM_POWERUP_SOUNDS[0]));
				index++) {

			const SSGMPowerupSoundStruct& entry = SSGM_POWERUP_SOUNDS[index];

			if (::strstr(preset, entry.Preset) == nullptr) {
				continue;
			}

			ScriptEngine::Send_Message_Player(sender,
				SSGM_ANNOUNCE_RED, SSGM_ANNOUNCE_GREEN, SSGM_ANNOUNCE_BLUE, entry.Text);
			ScriptEngine::Create_2D_WAV_Sound_Player(sender, entry.Sound);

			if (entry.DeniedWeapon != nullptr
					&& !SSGMSettingsClass::SBHCanPickupDropWeapons
					&& ::stricmp(ScriptEngine::Get_Preset_Name(sender), SSGM_STEALTH_SOLDIER) == 0) {
				ScriptEngine::Remove_Weapon(sender, entry.DeniedWeapon);
			}

			return;
		}
	}
};


/*
**	The character a backpack turns whoever picks it up into.  Where 4.8.4
**	listed alternates, one is picked at random -- they are the same character
**	wearing different clothes.
*/
struct SSGMCharacterPowerupStruct
{
	const char *	ScriptName;
	const char *	Presets[4];
};

static const SSGMCharacterPowerupStruct	SSGM_CHARACTER_POWERUPS[] =
{
	{ "SSGM_Powerup_SBH",				{ "CnC_Nod_FlameThrower_2SF" } },
	{ "SSGM_Powerup_Havoc",				{ "CnC_GDI_MiniGunner_3Boss", "CnC_GDI_MiniGunner_3Boss_ALT2",
													  "CnC_GDI_MiniGunner_3Boss_ALT3", "CnC_GDI_MiniGunner_3Boss_ALT4" } },
	{ "SSGM_Powerup_Deadeye",			{ "CnC_GDI_MiniGunner_2SF" } },
	{ "SSGM_Powerup_BHSniper",			{ "CnC_Nod_Minigunner_2SF" } },
	{ "SSGM_Powerup_Sakura",			{ "CnC_Nod_Minigunner_3Boss", "CnC_Nod_Minigunner_3Boss_ALT2" } },
	{ "SSGM_Powerup_SydneyPower",		{ "CnC_Sydney_PowerSuit", "CnC_Sydney_PowerSuit_ALT2" } },
	{ "SSGM_Powerup_Raveshaw",			{ "CnC_Nod_RocketSoldier_3Boss", "CnC_Nod_RocketSoldier_3Boss_ALT2" } },
	{ "SSGM_Powerup_Mobius",			{ "CnC_Ignatio_Mobius", "CnC_Ignatio_Mobius_ALT2" } },
	{ "SSGM_Powerup_Mendoza",			{ "CnC_Nod_FlameThrower_3Boss", "CnC_Nod_FlameThrower_3Boss_ALT2" } },
	{ "SSGM_Powerup_Hotwire",			{ "CnC_GDI_Engineer_2SF" } },
	{ "SSGM_Powerup_Technician",		{ "CnC_Nod_Technician_0" } },
	{ "SSGM_Powerup_Gunner",			{ "CnC_GDI_RocketSoldier_2SF" } },
	{ "SSGM_Powerup_LCG",				{ "CnC_Nod_RocketSoldier_2SF" } },
	{ "SSGM_Powerup_Patch",				{ "CnC_GDI_Grenadier_2SF" } },
	{ "SSGM_Powerup_Nod_Officer",		{ "CnC_Nod_Minigunner_1Off" } },
	{ "SSGM_Powerup_GDI_Officer",		{ "CnC_GDI_MiniGunner_1Off" } },
	{ "SSGM_Powerup_GDI_Sydney",		{ "CnC_Sydney" } },
	{ "SSGM_Powerup_Chemwar",			{ "CnC_Nod_FlameThrower_1Off" } },
	{ "SSGM_Powerup_GDI_RSoldier",	{ "CnC_GDI_RocketSoldier_1Off" } },
	{ "SSGM_Powerup_Nod_RSoldier",	{ "CnC_Nod_RocketSoldier_1Off" } },
};


/*
**	Turns whoever picks this backpack up into the character it belongs to.
**
**	Twenty of 4.8.4's scripts are this one behaviour under twenty names, and
**	which character each one means is fixed by the name -- so it is a virtual
**	answered by the subclass, not a script parameter a level could get wrong.
**	The base is not registered; only the twenty names below are.
*/
class SSGM_Character_Powerup : public ScriptImpClass
{
public:

	virtual int Which(void)		{ return 0; }

	void Custom(GameObject* /*obj*/, int message, intptr_t /*param*/, GameObject* sender) override
	{
		if (message != CUSTOM_EVENT_POWERUP_GRANTED || sender == nullptr) {
			return;
		}

		int which = Which();
		if (which < 0 || which >= (int)(sizeof(SSGM_CHARACTER_POWERUPS) / sizeof(SSGM_CHARACTER_POWERUPS[0]))) {
			return;
		}

		const SSGMCharacterPowerupStruct& entry = SSGM_CHARACTER_POWERUPS[which];

		int count = 0;
		while (count < 4 && entry.Presets[count] != nullptr) {
			count++;
		}

		if (count == 0) {
			return;
		}

		ScriptEngine::Change_Character(sender, entry.Presets[ScriptEngine::Get_Random_Int(0, count)]);
	}
};


class SSGM_Character_Powerup_00 : public SSGM_Character_Powerup { int Which(void) override { return 0; } };
class SSGM_Character_Powerup_01 : public SSGM_Character_Powerup { int Which(void) override { return 1; } };
class SSGM_Character_Powerup_02 : public SSGM_Character_Powerup { int Which(void) override { return 2; } };
class SSGM_Character_Powerup_03 : public SSGM_Character_Powerup { int Which(void) override { return 3; } };
class SSGM_Character_Powerup_04 : public SSGM_Character_Powerup { int Which(void) override { return 4; } };
class SSGM_Character_Powerup_05 : public SSGM_Character_Powerup { int Which(void) override { return 5; } };
class SSGM_Character_Powerup_06 : public SSGM_Character_Powerup { int Which(void) override { return 6; } };
class SSGM_Character_Powerup_07 : public SSGM_Character_Powerup { int Which(void) override { return 7; } };
class SSGM_Character_Powerup_08 : public SSGM_Character_Powerup { int Which(void) override { return 8; } };
class SSGM_Character_Powerup_09 : public SSGM_Character_Powerup { int Which(void) override { return 9; } };
class SSGM_Character_Powerup_10 : public SSGM_Character_Powerup { int Which(void) override { return 10; } };
class SSGM_Character_Powerup_11 : public SSGM_Character_Powerup { int Which(void) override { return 11; } };
class SSGM_Character_Powerup_12 : public SSGM_Character_Powerup { int Which(void) override { return 12; } };
class SSGM_Character_Powerup_13 : public SSGM_Character_Powerup { int Which(void) override { return 13; } };
class SSGM_Character_Powerup_14 : public SSGM_Character_Powerup { int Which(void) override { return 14; } };
class SSGM_Character_Powerup_15 : public SSGM_Character_Powerup { int Which(void) override { return 15; } };
class SSGM_Character_Powerup_16 : public SSGM_Character_Powerup { int Which(void) override { return 16; } };
class SSGM_Character_Powerup_17 : public SSGM_Character_Powerup { int Which(void) override { return 17; } };
class SSGM_Character_Powerup_18 : public SSGM_Character_Powerup { int Which(void) override { return 18; } };
class SSGM_Character_Powerup_19 : public SSGM_Character_Powerup { int Which(void) override { return 19; } };

REGISTER_SCRIPT_TT_NAMED(SSGM_Character_Powerup_00, "SSGM_Powerup_SBH", "")
REGISTER_SCRIPT_TT_NAMED(SSGM_Character_Powerup_01, "SSGM_Powerup_Havoc", "")
REGISTER_SCRIPT_TT_NAMED(SSGM_Character_Powerup_02, "SSGM_Powerup_Deadeye", "")
REGISTER_SCRIPT_TT_NAMED(SSGM_Character_Powerup_03, "SSGM_Powerup_BHSniper", "")
REGISTER_SCRIPT_TT_NAMED(SSGM_Character_Powerup_04, "SSGM_Powerup_Sakura", "")
REGISTER_SCRIPT_TT_NAMED(SSGM_Character_Powerup_05, "SSGM_Powerup_SydneyPower", "")
REGISTER_SCRIPT_TT_NAMED(SSGM_Character_Powerup_06, "SSGM_Powerup_Raveshaw", "")
REGISTER_SCRIPT_TT_NAMED(SSGM_Character_Powerup_07, "SSGM_Powerup_Mobius", "")
REGISTER_SCRIPT_TT_NAMED(SSGM_Character_Powerup_08, "SSGM_Powerup_Mendoza", "")
REGISTER_SCRIPT_TT_NAMED(SSGM_Character_Powerup_09, "SSGM_Powerup_Hotwire", "")
REGISTER_SCRIPT_TT_NAMED(SSGM_Character_Powerup_10, "SSGM_Powerup_Technician", "")
REGISTER_SCRIPT_TT_NAMED(SSGM_Character_Powerup_11, "SSGM_Powerup_Gunner", "")
REGISTER_SCRIPT_TT_NAMED(SSGM_Character_Powerup_12, "SSGM_Powerup_LCG", "")
REGISTER_SCRIPT_TT_NAMED(SSGM_Character_Powerup_13, "SSGM_Powerup_Patch", "")
REGISTER_SCRIPT_TT_NAMED(SSGM_Character_Powerup_14, "SSGM_Powerup_Nod_Officer", "")
REGISTER_SCRIPT_TT_NAMED(SSGM_Character_Powerup_15, "SSGM_Powerup_GDI_Officer", "")
REGISTER_SCRIPT_TT_NAMED(SSGM_Character_Powerup_16, "SSGM_Powerup_GDI_Sydney", "")
REGISTER_SCRIPT_TT_NAMED(SSGM_Character_Powerup_17, "SSGM_Powerup_Chemwar", "")
REGISTER_SCRIPT_TT_NAMED(SSGM_Character_Powerup_18, "SSGM_Powerup_GDI_RSoldier", "")
REGISTER_SCRIPT_TT_NAMED(SSGM_Character_Powerup_19, "SSGM_Powerup_Nod_RSoldier", "")


/*
**	Which backpack a dead character leaves behind.  The preset is matched as a
**	prefix, which is how one row covers a character and all its alternates.
*/
struct SSGMDnaDropStruct
{
	const char *	Preset;
	const char *	Powerup;
	const char *	Script;
};

static const SSGMDnaDropStruct	SSGM_DNA_DROPS[] =
{
	{ "CnC_GDI_MiniGunner_1Off",			"POW_Tissue_Nanites",	"SSGM_Powerup_GDI_Officer" },
	{ "CnC_Nod_MiniGunner_1Off",			"POW_Tissue_Nanites",	"SSGM_Powerup_Nod_Officer" },
	{ "CnC_Nod_Mutant_0_Mutant",			"POW_Tissue_Nanites",	"SSGM_Powerup_Chemwar" },
	{ "CnC_Nod_RocketSoldier_2SF",		"POW_Tissue_Nanites",	"SSGM_Powerup_LCG" },
	{ "CnC_Nod_FlameThrower_2SF",			"POW_Stealth_Suit",		"SSGM_Powerup_SBH" },
	{ "CnC_Nod_Technician_0",				"POW_Tissue_Nanites",	"SSGM_Powerup_Technician" },
	{ "CnC_GDI_Engineer_2SF",				"POW_Tissue_Nanites",	"SSGM_Powerup_Hotwire" },
	{ "CnC_Sydney_PowerSuit",				"POW_Tissue_Nanites",	"SSGM_Powerup_SydneyPower" },
	{ "CnC_Nod_RocketSoldier_3Boss",		"POW_Tissue_Nanites",	"SSGM_Powerup_Raveshaw" },
	{ "CnC_Nod_Minigunner_3Boss",			"POW_Tissue_Nanites",	"SSGM_Powerup_Sakura" },
	{ "CnC_Nod_MiniGunner_3Boss",			"POW_Tissue_Nanites",	"SSGM_Powerup_Sakura" },
	{ "CnC_GDI_MiniGunner_3Boss",			"POW_Tissue_Nanites",	"SSGM_Powerup_Havoc" },
	{ "CnC_GDI_MiniGunner_2SF_Logan",	"POW_Tissue_Nanites",	"SSGM_Powerup_Havoc" },
	{ "CnC_GDI_RocketSoldier_1Off",		"POW_Tissue_Nanites",	"SSGM_Powerup_GDI_RSoldier" },
	{ "CnC_Nod_RocketSoldier_1Off",		"POW_Tissue_Nanites",	"SSGM_Powerup_Nod_RSoldier" },
	{ "CnC_GDI_RocketSoldier_2SF_Secret",	"POW_Tissue_Nanites",	"SSGM_Powerup_Gunner" },
	{ "CnC_GDI_MiniGunner_2SF",			"POW_Tissue_Nanites",	"SSGM_Powerup_Deadeye" },
	{ "CnC_Nod_Minigunner_2SF",			"POW_Tissue_Nanites",	"SSGM_Powerup_BHSniper" },
	{ "CnC_GDI_Grenadier_2SF",				"POW_Tissue_Nanites",	"SSGM_Powerup_Patch" },
	{ "CnC_Ignatio_Mobius",					"POW_Tissue_Nanites",	"SSGM_Powerup_Mobius" },
	{ "CnC_Nod_FlameThrower_3Boss",		"POW_Tissue_Nanites",	"SSGM_Powerup_Mendoza" },
	{ "CnC_Sydney",								"POW_Tissue_Nanites",	"SSGM_Powerup_GDI_Sydney" },
};


/*SSGM_Drop_DNA_Powerup

  A specialist character leaves a backpack behind when they die, one time in
  five, that turns whoever picks it up into that character.
*/

DECLARE_SCRIPT_TT(SSGM_Drop_DNA_Powerup, "")
{
	void Destroyed(GameObject* obj) override
	{
		//	One in five, which is 4.8.4's two-in-ten.
		if (ScriptEngine::Get_Random_Int(1, 11) > 2) {
			return;
		}

		const char* preset = ScriptEngine::Get_Preset_Name(obj);

		for (int index = 0; index < (int)(sizeof(SSGM_DNA_DROPS) / sizeof(SSGM_DNA_DROPS[0])); index++) {

			if (::strstr(preset, SSGM_DNA_DROPS[index].Preset) == nullptr) {
				continue;
			}

			GameObject* backpack = ScriptEngine::Create_Object(SSGM_DNA_DROPS[index].Powerup,
					ScriptEngine::Get_Position(obj));

			if (backpack != nullptr) {
				ScriptEngine::Set_Powerup_Always_Allow_Grant(backpack, true);
				ScriptEngine::Attach_Script_Once(backpack, SSGM_DNA_DROPS[index].Script, "");
				ScriptEngine::Attach_Script_Once(backpack, "SSGM_Powerup_Expire", "");
			}

			return;
		}
	}
};


/*SSGM_Vehicle_Icon

  The marker that hangs over a claimed vehicle so its owner can find it and
  everybody else can see it is taken.
*/

DECLARE_SCRIPT_TT(SSGM_Vehicle_Icon, "")
{
	int icon_id;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(icon_id, 1);
	}

	void Created(GameObject* obj) override
	{
		icon_id = 0;

		VehicleGameObj* vehicle = obj->As_VehicleGameObj();
		if (vehicle == nullptr || vehicle->Is_Stealth_Enabled()) {
			return;
		}

		GameObject* icon = ScriptEngine::Create_Object_At_Bone(obj, "Invisible_Object", "muzzlea0");
		if (icon == nullptr) {
			return;
		}

		ScriptEngine::Set_Model(icon, "o_em_apc");
		ScriptEngine::Attach_To_Object_Bone(icon, obj, "muzzlea0");
		icon_id = ScriptEngine::Get_ID(icon);
	}

	void Remove_Icon(void)
	{
		if (icon_id != 0) {
			ScriptEngine::Destroy_Object(ScriptEngine::Find_Object(icon_id));
			icon_id = 0;
		}
	}

	void Destroyed(GameObject* /*obj*/) override		{ Remove_Icon(); }
	void Detach(GameObject* obj) override				{ ScriptImpClass::Detach(obj); Remove_Icon(); }
};
