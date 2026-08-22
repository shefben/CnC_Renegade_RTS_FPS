/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 OpenW3D contributors.
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

#include "ssgmmanager.h"

#include "ssgmsettings.h"
#include "ssgmgamelog.h"

#include "gameeventbus.h"
#include "gameobjmanager.h"
#include "scriptcommands.h"
#include "building.h"
#include "vehicle.h"
#include "soldier.h"
#include "c4.h"
#include "damage.h"
#include "weaponmanager.h"
#include "definitionmgr.h"
#include "definition.h"
#include "playerdata.h"
#include "gamedata.h"
#include "team.h"
#include "teammanager.h"
#include "combat.h"
#include "basecontroller.h"
#include "wwdebug.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <wchar.h>


bool	SSGMManagerClass::IsRegistered			= false;

int	SSGMManagerClass::ObjectCreateToken		= 0;
int	SSGMManagerClass::LevelLoadedToken		= 0;
int	SSGMManagerClass::GameOverToken			= 0;
int	SSGMManagerClass::ChatToken				= 0;
int	SSGMManagerClass::RefillToken				= 0;
int	SSGMManagerClass::PurchaseToken			= 0;
int	SSGMManagerClass::ConsoleOutputToken	= 0;
int	SSGMManagerClass::ThinkToken				= 0;


/*
**	When each player last refilled, for the refill cooldown.  SSGM kept this on
**	a script attached to every soldier; it is one number per player and it
**	belongs to the rule that reads it.
*/
struct	SSGMRefillStruct
{
	int			PlayerID;
	unsigned		Time;

	//	The vector wants to be able to compare its elements.
	bool	operator == (const SSGMRefillStruct &other) const	{ return PlayerID == other.PlayerID; }
	bool	operator != (const SSGMRefillStruct &other) const	{ return PlayerID != other.PlayerID; }
};

static DynamicVectorClass<SSGMRefillStruct>	_RefillTimes;


//
//	The colour the server speaks in.  SSGM sent its replies through the
//	console's private-message command; a private message is a thing the engine
//	can now send directly, so it does.
//
static const int	SSGM_MESSAGE_RED		= 255;
static const int	SSGM_MESSAGE_GREEN	= 170;
static const int	SSGM_MESSAGE_BLUE		= 0;


////////////////////////////////////////////////////////////////
//
//	Tell
//
////////////////////////////////////////////////////////////////
void
SSGMManagerClass::Tell (SoldierGameObj *soldier, const char *format, ...)
{
	if (soldier == nullptr) {
		return ;
	}

	char text[512];
	va_list args;
	va_start (args, format);
	::_vsnprintf (text, sizeof (text), format, args);
	va_end (args);
	text[sizeof (text) - 1] = 0;

	ScriptEngine::Send_Message_Player (soldier, SSGM_MESSAGE_RED, SSGM_MESSAGE_GREEN,
			SSGM_MESSAGE_BLUE, text);
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Register
//
////////////////////////////////////////////////////////////////
void
SSGMManagerClass::Register (void)
{
	if (IsRegistered) {
		return ;
	}

	SSGMSettingsClass::Load ();
	SSGMGameLog::Init (SSGMSettingsClass::LogPort);
	SSGMGameLog::Log_Message ("Server-side game manager started\n", "_GENERAL");

	ObjectCreateToken		= GameEventBus::ObjectCreate.Register (On_Object_Create);
	LevelLoadedToken		= GameEventBus::LevelLoaded.Register (On_Level_Loaded);
	GameOverToken			= GameEventBus::GameOver.Register (On_Game_Over);
	ChatToken				= GameEventBus::Chat.Register (On_Chat);
	RefillToken				= GameEventBus::Refill.Register (On_Refill);
	PurchaseToken			= GameEventBus::Purchase.Register (On_Purchase);
	ConsoleOutputToken	= GameEventBus::ConsoleOutput.Register (On_Console_Output);
	ThinkToken				= GameEventBus::Think.Register (On_Think);

	IsRegistered = true;
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Unregister
//
////////////////////////////////////////////////////////////////
void
SSGMManagerClass::Unregister (void)
{
	if (!IsRegistered) {
		return ;
	}

	GameEventBus::ObjectCreate.Unregister (ObjectCreateToken);
	GameEventBus::LevelLoaded.Unregister (LevelLoadedToken);
	GameEventBus::GameOver.Unregister (GameOverToken);
	GameEventBus::Chat.Unregister (ChatToken);
	GameEventBus::Refill.Unregister (RefillToken);
	GameEventBus::Purchase.Unregister (PurchaseToken);
	GameEventBus::ConsoleOutput.Unregister (ConsoleOutputToken);
	GameEventBus::Think.Unregister (ThinkToken);

	ObjectCreateToken	= 0;
	LevelLoadedToken	= 0;
	GameOverToken		= 0;
	ChatToken			= 0;
	RefillToken			= 0;
	PurchaseToken		= 0;
	ConsoleOutputToken	= 0;
	ThinkToken			= 0;

	_RefillTimes.Delete_All ();

	SSGMGameLog::Shutdown ();
	SSGMSettingsClass::Shutdown ();

	IsRegistered = false;
	return ;
}


////////////////////////////////////////////////////////////////
//
//	On_Object_Create
//
////////////////////////////////////////////////////////////////
void
SSGMManagerClass::On_Object_Create (ObjectCreateEventClass &event, void * /*data*/)
{
	ScriptableGameObj *obj = (event.Object != nullptr) ? event.Object->As_ScriptableGameObj () : nullptr;
	if (obj == nullptr) {
		return ;
	}

	if (obj->As_BuildingGameObj () != nullptr) {
		ScriptEngine::Attach_Script_Once (obj, "SSGM_Building", "");
		return ;
	}

	PhysicalGameObj *physical = obj->As_PhysicalGameObj ();

	if (obj->As_SoldierGameObj () != nullptr) {

		//
		//	A server that took the extra weapons away takes them from the
		//	character's own grant script, before it has run.
		//
		if (SSGMSettingsClass::DisableExtraWeapons) {
			ScriptEngine::Remove_Script (obj, "M00_GrantPowerup_Created");
		}

		ScriptEngine::Attach_Script_Once (obj, "SSGM_Soldier", "");
		return ;
	}

	if (obj->As_VehicleGameObj () != nullptr) {
		ScriptEngine::Attach_Script_Once (obj, "SSGM_Vehicle", "");
		return ;
	}

	if (physical == nullptr) {
		return ;
	}

	if (physical->As_PowerUpGameObj () != nullptr) {
		if (SSGMSettingsClass::PlayPowerupSounds) {
			ScriptEngine::Attach_Script_Once (obj, "SSGM_Powerup", "");
		}
		return ;
	}

	if (physical->As_BeaconGameObj () != nullptr) {
		ScriptEngine::Attach_Script_Once (obj, "SSGM_Log_Beacon", "");
		return ;
	}

	if (physical->As_C4GameObj () != nullptr) {
		ScriptEngine::Attach_Script_Once (obj, "SSGM_Log_Mine", "");
		return ;
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Apply_Building_Settings
//
////////////////////////////////////////////////////////////////
void
SSGMManagerClass::Apply_Building_Settings (void)
{
	ArmorType blamo = ArmorWarheadManager::Get_Armor_Type ("Blamo");

	for (	SLNode<BuildingGameObj> *node = GameObjManager::Get_Building_Game_Obj_List ()->Head ();
			node != nullptr;
			node = node->Next ()) {

		BuildingGameObj *building = node->Data ();
		if (building == nullptr) {
			continue;
		}

		bool disabled = false;

		switch (building->Get_Definition ().Get_Type ())
		{
			case BuildingConstants::TYPE_POWER_PLANT:
				disabled = SSGMSettingsClass::DisablePowerPlants;			break;
			case BuildingConstants::TYPE_SOLDIER_FACTORY:
				disabled = SSGMSettingsClass::DisableSoldierFactories;	break;
			case BuildingConstants::TYPE_VEHICLE_FACTORY:
				disabled = SSGMSettingsClass::DisableVehicleFactories;	break;
			case BuildingConstants::TYPE_REFINERY:
				disabled = SSGMSettingsClass::DisableRefineries;			break;
			case BuildingConstants::TYPE_COM_CENTER:
				disabled = SSGMSettingsClass::DisableCommCenters;			break;
			case BuildingConstants::TYPE_REPAIR_BAY:
				disabled = SSGMSettingsClass::DisableRepairPads;			break;
			case BuildingConstants::TYPE_SHRINE:
				disabled = SSGMSettingsClass::DisableShrines;				break;
			case BuildingConstants::TYPE_HELIPAD:
				disabled = SSGMSettingsClass::DisableHelipads;				break;
			case BuildingConstants::TYPE_CONYARD:
				disabled = SSGMSettingsClass::DisableConstructionYards;	break;
			case BuildingConstants::TYPE_BASE_DEFENSE:
				disabled = SSGMSettingsClass::DisableBaseDefenses;			break;

			//
			//	DisableTechCenters, DisableSpecials and DisableNavalFactories
			//	are read and carried, but this tree has no building type they
			//	could match: those three are 4.8.4 building types that arrive
			//	with the buildings themselves.
			//
			default:
				break;
		}

		if (disabled) {
			ScriptEngine::Apply_Damage (building, 99999.0F, "Death", nullptr);
		} else if (SSGMSettingsClass::InvincibleBuildings) {
			building->Get_Defense_Object ()->Set_Shield_Type (blamo);
		}
	}

	//
	//	A base defence turret is a vehicle rather than a building, but it is
	//	the base defence setting that governs it.
	//
	for (	SLNode<SmartGameObj> *node = GameObjManager::Get_Smart_Game_Obj_List ()->Head ();
			node != nullptr;
			node = node->Next ()) {

		SmartGameObj *smart = node->Data ();
		VehicleGameObj *vehicle = (smart != nullptr) ? smart->As_VehicleGameObj () : nullptr;

		if (vehicle == nullptr || !vehicle->Is_Turret ()) {
			continue;
		}

		if (SSGMSettingsClass::DisableBaseDefenses) {
			ScriptEngine::Apply_Damage (vehicle, 99999.0F, "Death", nullptr);
		} else if (SSGMSettingsClass::InvincibleBuildings) {
			vehicle->Get_Defense_Object ()->Set_Shield_Type (blamo);
		}
	}

	//
	//	And a harvester follows its refinery.  SSGM tested the preset name for
	//	this because a script could not ask anything else; a base controller
	//	knows which vehicle is its own harvester.
	//
	for (int team = PLAYERTYPE_NOD; team <= PLAYERTYPE_GDI; team ++) {

		BaseControllerClass *base = BaseControllerClass::Find_Base (team);
		VehicleGameObj *harvester = (base != nullptr) ? base->Get_Harvester_Vehicle () : nullptr;

		if (harvester == nullptr) {
			continue;
		}

		if (SSGMSettingsClass::DisableRefineries) {
			ScriptEngine::Apply_Damage (harvester, 99999.0F, "Death", nullptr);
		} else if (SSGMSettingsClass::InvincibleBuildings) {
			harvester->Get_Defense_Object ()->Set_Shield_Type (blamo);
		}
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	On_Level_Loaded
//
////////////////////////////////////////////////////////////////
void
SSGMManagerClass::On_Level_Loaded (LevelEventClass &event, void * /*data*/)
{
	//
	//	An operator who edited the file between maps gets the new settings
	//	without restarting the server.  The log port is not re-read: a socket
	//	that clients are connected to is not something to move underneath
	//	them, and SSGM does not move it either.
	//
	if (SSGMSettingsClass::File_Has_Changed ()) {
		SSGMGameLog::Log_Message ("Reloading ssgm.ini\n", "_GENERAL");
		SSGMSettingsClass::Load ();
	}

	SSGMSettingsClass::Load_Map (event.LevelName);

	Apply_Building_Settings ();

	_RefillTimes.Delete_All ();

	SSGMGameLog::Log_Gamelog ("MAP;%s", (event.LevelName != nullptr) ? event.LevelName : "");

	//
	//	The two objects SSGM puts in every level: the score sampler, and the
	//	weather driver when a map asked for weather.
	//
	Vector3 origin (0.0F, 0.0F, 0.0F);

	GameObject *logger = ScriptEngine::Create_Object ("Invisible_Object", origin);
	if (logger != nullptr) {
		ScriptEngine::Attach_Script (logger, "SSGM_Log_Scores", "");
	}

	if (SSGMSettingsClass::WeatherType != SSGM_WEATHER_NONE) {
		GameObject *weather = ScriptEngine::Create_Object ("Invisible_Object", origin);
		if (weather != nullptr) {
			ScriptEngine::Attach_Script (weather, "SSGM_Random_Weather", "");
		}
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	On_Game_Over
//
////////////////////////////////////////////////////////////////
void
SSGMManagerClass::On_Game_Over (GameOverEventClass &event, void * /*data*/)
{
	cGameData *game = The_Game ();
	if (game == nullptr) {
		return ;
	}

	const char *win_type = "Unknown";
	switch (game->Get_Win_Type ())
	{
		case cGameData::WIN_TYPE_FORCED:			win_type = "Server Shutdown";							break;
		case cGameData::WIN_TYPE_COWARDICE:		win_type = "Opponents abandoned the game";		break;
		case cGameData::WIN_TYPE_TIME:			win_type = "High score when time limit expired";	break;
		case cGameData::WIN_TYPE_BASE_DESTRUCTION:	win_type = "Building Destruction";				break;
		case cGameData::WIN_TYPE_BEACON:			win_type = "Pedestal Beacon";							break;
		default:												break;
	}

	const char *winner = (event.WinnerTeam == PLAYERTYPE_GDI) ? "GDI"
							 : ((event.WinnerTeam == PLAYERTYPE_NOD) ? "Nod" : "Nobody");

	cTeam *nod = cTeamManager::Find_Team (PLAYERTYPE_NOD);
	cTeam *gdi = cTeamManager::Find_Team (PLAYERTYPE_GDI);

	StringClass message;
	message.Format ("Game on %s has ended. Won by %s by %s.\n",
			game->Get_Map_Name ().Peek_Buffer (), winner, win_type);
	SSGMGameLog::Log_Message (message, "_GENERAL");

	SSGMGameLog::Log_Gamelog ("WIN;%s;%s;%d;%d", winner, win_type,
			(nod != nullptr) ? (int)nod->Get_Score () : 0,
			(gdi != nullptr) ? (int)gdi->Get_Score () : 0);

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Bind_Vehicle
//
////////////////////////////////////////////////////////////////
void
SSGMManagerClass::Bind_Vehicle (SoldierGameObj *soldier)
{
	GameObject *riding = ScriptEngine::Get_Vehicle (soldier);
	VehicleGameObj *vehicle = (riding != nullptr) ? riding->As_VehicleGameObj () : nullptr;

	if (vehicle == nullptr) {
		Tell (soldier, "You have to be in a vehicle to bind.");
		return ;
	}

	if (vehicle->Get_Claimant () == soldier) {
		Tell (soldier, "This vehicle is already bound to you.");
		return ;
	}

	if (vehicle->Get_Claimant () != nullptr) {
		Tell (soldier, "The vehicle you are in is already bound.");
		return ;
	}

	if (ScriptEngine::Get_Vehicle_Driver (vehicle) != soldier) {
		Tell (soldier, "You are not the driver of the vehicle.");
		return ;
	}

	//
	//	A player only has one vehicle, so claiming this one releases whatever
	//	they had before.
	//
	Unbind_Vehicle (soldier, true);

	vehicle->Set_Claimant (soldier);
	Tell (soldier, "Your vehicle has been bound to you.");

	if (SSGMSettingsClass::VehicleLockIcons) {
		ScriptEngine::Attach_Script_Once (vehicle, "SSGM_Vehicle_Icon", "");
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Find_Claimed_Vehicle
//
//	Whose vehicle is whose is a field on the vehicle now, so this is the whole
//	of what SSGM needed a walk of every vehicle in the level for.
////////////////////////////////////////////////////////////////
static VehicleGameObj *
Find_Claimed_Vehicle (SoldierGameObj *soldier)
{
	if (soldier == nullptr) {
		return nullptr;
	}

	for (	SLNode<SmartGameObj> *node = GameObjManager::Get_Smart_Game_Obj_List ()->Head ();
			node != nullptr;
			node = node->Next ()) {

		SmartGameObj *smart = node->Data ();
		VehicleGameObj *vehicle = (smart != nullptr) ? smart->As_VehicleGameObj () : nullptr;

		if (vehicle != nullptr && vehicle->Get_Claimant () == soldier) {
			return vehicle;
		}
	}

	return nullptr;
}


////////////////////////////////////////////////////////////////
//
//	Unbind_Vehicle
//
////////////////////////////////////////////////////////////////
void
SSGMManagerClass::Unbind_Vehicle (SoldierGameObj *soldier, bool silent)
{
	VehicleGameObj *vehicle = Find_Claimed_Vehicle (soldier);

	if (vehicle == nullptr) {
		if (!silent) {
			Tell (soldier, "You have no vehicle bound to you.");
		}
		return ;
	}

	vehicle->Set_Owner (nullptr);
	vehicle->Set_Claimant (nullptr);
	ScriptEngine::Remove_Script (vehicle, "SSGM_Vehicle_Icon");

	if (!silent) {
		Tell (soldier, "Your vehicle has been unbound.");
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Lock_Vehicle
//
////////////////////////////////////////////////////////////////
void
SSGMManagerClass::Lock_Vehicle (SoldierGameObj *soldier)
{
	VehicleGameObj *vehicle = Find_Claimed_Vehicle (soldier);

	if (vehicle == nullptr) {
		Tell (soldier, "You have no vehicle bound to you.");
		return ;
	}

	if (vehicle->Get_Owner () != nullptr) {
		Tell (soldier, "Your vehicle is already locked.");
		return ;
	}

	vehicle->Set_Owner (soldier);
	Tell (soldier, "Your vehicle has been locked.");

	//
	//	Locking it while somebody else is driving takes it back.
	//
	if (ScriptEngine::Get_Vehicle_Driver (vehicle) != soldier) {
		ScriptEngine::Force_Occupant_Exit (vehicle, 0);
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Unlock_Vehicle
//
////////////////////////////////////////////////////////////////
void
SSGMManagerClass::Unlock_Vehicle (SoldierGameObj *soldier, bool silent)
{
	VehicleGameObj *vehicle = Find_Claimed_Vehicle (soldier);

	if (vehicle == nullptr) {
		if (!silent) {
			Tell (soldier, "You have no vehicle bound to you.");
		}
		return ;
	}

	if (vehicle->Get_Owner () == nullptr) {
		if (!silent) {
			Tell (soldier, "Your vehicle is not locked.");
		}
		return ;
	}

	vehicle->Set_Owner (nullptr);

	if (!silent) {
		Tell (soldier, "Your vehicle has been unlocked.");
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Kick_Vehicle_Driver
//
////////////////////////////////////////////////////////////////
void
SSGMManagerClass::Kick_Vehicle_Driver (SoldierGameObj *soldier)
{
	VehicleGameObj *vehicle = Find_Claimed_Vehicle (soldier);

	if (vehicle == nullptr) {
		Tell (soldier, "You have no vehicle bound to you.");
		return ;
	}

	if (ScriptEngine::Get_Vehicle_Driver (vehicle) == soldier) {
		Tell (soldier, "You cannot kick yourself out of your vehicle.");
		return ;
	}

	ScriptEngine::Force_Occupant_Exit (vehicle, 0);
	Tell (soldier, "The driver has been kicked out of your vehicle.");
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Report_Mines
//
////////////////////////////////////////////////////////////////
void
SSGMManagerClass::Report_Mines (SoldierGameObj *soldier)
{
	int team		= soldier->Get_Player_Type ();
	int remote	= 0;
	int proximity	= 0;

	for (	SLNode<BaseGameObj> *node = GameObjManager::Get_Game_Obj_List ()->Head ();
			node != nullptr;
			node = node->Next ()) {

		BaseGameObj *base = node->Data ();
		PhysicalGameObj *physical = (base != nullptr) ? base->As_PhysicalGameObj () : nullptr;
		C4GameObj *c4 = (physical != nullptr) ? physical->As_C4GameObj () : nullptr;

		if (c4 == nullptr || c4->Get_Player_Type () != team) {
			continue;
		}

		const AmmoDefinitionClass *ammo = c4->Peek_Ammo_Definition ();
		if (ammo == nullptr) {
			continue;
		}

		if ((int)ammo->AmmoType == (int)AmmoDefinitionClass::AMMO_TYPE_C4_PROXIMITY) {
			proximity ++;
		} else if ((int)ammo->AmmoType == (int)AmmoDefinitionClass::AMMO_TYPE_C4_REMOTE) {
			remote ++;
		}
	}

	//
	//	SSGM printed a limit here as well, read from a 4.8.4 engine value.
	//	This tree caps nothing, so there is no limit to report.
	//
	Tell (soldier, "Remote: %d - Proximity: %d - Total: %d", remote, proximity, remote + proximity);
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Handle_Chat_Command
//
////////////////////////////////////////////////////////////////
bool
SSGMManagerClass::Handle_Chat_Command (int player_id, const wchar_t *message)
{
	if (message == nullptr || message[0] != L'!') {
		return false;
	}

	SoldierGameObj *soldier = GameObjManager::Find_Soldier_Of_Client_ID (player_id);
	if (soldier == nullptr) {
		return false;
	}

	//
	//	Narrow the command word: everything here is ASCII, and comparing a
	//	wide string against a literal is the only reason this is wide at all.
	//
	char command[32];
	int length = 0;
	while (length < (int)sizeof (command) - 1 && message[length] != 0 && message[length] != L' ') {
		command[length] = (char)message[length];
		length ++;
	}
	command[length] = 0;

	if (SSGMSettingsClass::VehicleOwnership) {

		if (::_stricmp (command, "!bind") == 0) {
			Bind_Vehicle (soldier);
			return true;
		}

		if (::_stricmp (command, "!lock") == 0) {
			Lock_Vehicle (soldier);
			return true;
		}

		if (::_stricmp (command, "!bl") == 0) {
			Bind_Vehicle (soldier);
			Lock_Vehicle (soldier);
			return true;
		}

		if (::_stricmp (command, "!unbind") == 0) {
			Unbind_Vehicle (soldier, false);
			return true;
		}

		if (::_stricmp (command, "!unlock") == 0) {
			Unlock_Vehicle (soldier, false);
			return true;
		}

		if (::_stricmp (command, "!vkick") == 0) {
			Kick_Vehicle_Driver (soldier);
			return true;
		}
	}

	if (::_stricmp (command, "!c4") == 0) {
		Report_Mines (soldier);
		return true;
	}

	return false;
}


////////////////////////////////////////////////////////////////
//
//	On_Chat
//
////////////////////////////////////////////////////////////////
void
SSGMManagerClass::On_Chat (ChatEventClass &event, void * /*data*/)
{
	//
	//	A command is not a message: it is answered and it does not go out to
	//	the other players.
	//
	if (Handle_Chat_Command (event.PlayerID, event.Message)) {
		event.Allow = false;
		event.Stop_Dispatch ();
		return ;
	}

	const char *type = (event.Type == TEXT_MESSAGE_TEAM) ? "TEAM" : "PUBLIC";
	SSGMGameLog::Log_Gamelog ("CHAT;%s;%d;%ls", type, event.PlayerID,
			(event.Message != nullptr) ? event.Message : L"");

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Refill_Is_Allowed
//
//	True when this player's cooldown has expired, and records the refill when
//	it has.  A limit of zero means no cooldown, which is the default.
////////////////////////////////////////////////////////////////
static bool
Refill_Is_Allowed (int player_id)
{
	if (SSGMSettingsClass::RefillLimit <= 0) {
		return true;
	}

	cGameData *game = The_Game ();
	unsigned now = (game != nullptr) ? game->Get_Game_Duration_S () : 0;

	for (int index = 0; index < _RefillTimes.Count (); index ++) {

		if (_RefillTimes[index].PlayerID != player_id) {
			continue;
		}

		if ((now - _RefillTimes[index].Time) < (unsigned)SSGMSettingsClass::RefillLimit) {
			return false;
		}

		_RefillTimes[index].Time = now;
		return true;
	}

	SSGMRefillStruct entry;
	entry.PlayerID	= player_id;
	entry.Time		= now;
	_RefillTimes.Add (entry);
	return true;
}


////////////////////////////////////////////////////////////////
//
//	On_Refill
//
////////////////////////////////////////////////////////////////
void
SSGMManagerClass::On_Refill (RefillEventClass &event, void * /*data*/)
{
	int player_id = ScriptEngine::Get_Player_ID (event.Purchaser);
	if (player_id <= 0) {
		return ;
	}

	if (!Refill_Is_Allowed (player_id)) {
		event.Allow = false;
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	On_Purchase
//
////////////////////////////////////////////////////////////////
void
SSGMManagerClass::On_Purchase (PurchaseEventClass &event, void * /*data*/)
{
	//
	//	Nothing an operator took out of the game is for sale, and a server
	//	with beacons off does not sell them either.
	//
	if (SSGMSettingsClass::Is_Preset_Disabled ((int)event.PresetID)) {
		event.Status = PURCHASE_STATUS_OUT_OF_STOCK;
		return ;
	}

	if (event.Category == PURCHASE_CATEGORY_POWERUP && SSGMSettingsClass::DisableBeacons) {
		event.Status = PURCHASE_STATUS_OUT_OF_STOCK;
		return ;
	}

	//
	//	Buying a character is how a player refills, so it answers to the same
	//	cooldown.
	//
	if (event.Category == PURCHASE_CATEGORY_CHARACTER) {
		int player_id = ScriptEngine::Get_Player_ID (event.Purchaser);
		if (player_id > 0 && !Refill_Is_Allowed (player_id)) {
			event.Status = PURCHASE_STATUS_OUT_OF_STOCK;
			return ;
		}
	}

	const char *preset = ScriptEngine::Get_Definition_Name ((int)event.PresetID);
	if (preset == nullptr) {
		preset = "";
	}

	StringClass shown = preset;
	SSGMSettingsClass::Get_Translation_Override (preset, shown);

	static const char * const	CATEGORY_NAME[] = { "POWERUP", "VEHICLE", "CHARACTER" };

	StringClass message;
	message.Format ("Purchase: %ls - %s\n",
			ScriptEngine::Get_Wide_Player_Name (event.Purchaser), shown.Peek_Buffer ());
	SSGMGameLog::Log_Message (message, "_PURCHASE");

	SSGMGameLog::Log_Gamelog ("PURCHASED;%s;%ls;%s;%s", CATEGORY_NAME[event.Category],
			ScriptEngine::Get_Wide_Player_Name (event.Purchaser), preset, shown.Peek_Buffer ());

	if (SSGMSettingsClass::FreePurchases) {
		event.Status = PURCHASE_STATUS_ALLOW_FREE;
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	On_Console_Output
//
////////////////////////////////////////////////////////////////
void
SSGMManagerClass::On_Console_Output (ConsoleOutputEventClass &event, void * /*data*/)
{
	SSGMGameLog::Send_Console (event.Text);
	return ;
}


////////////////////////////////////////////////////////////////
//
//	On_Think
//
////////////////////////////////////////////////////////////////
void
SSGMManagerClass::On_Think (ThinkEventClass & /*event*/, void * /*data*/)
{
	SSGMGameLog::Think ();
	return ;
}
