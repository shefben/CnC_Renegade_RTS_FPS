/*	Renegade Scripts.dll
	Copyright 2013 Tiberian Technologies

	This file is part of the Renegade scripts.dll
	The Renegade scripts.dll is free software; you can redistribute it and/or modify it under
	the terms of the GNU General Public License as published by the Free
	Software Foundation; either version 2, or (at your option) any later
	version. See the file COPYING for more details.
	In addition, an exemption is given to allow Run Time Dynamic Linking of this code with any closed source module that does not contain code covered by this licence.
	Only the source code to the module(s) containing the licenced code has to be released.
*/
#include "General.h"
#include "ExamplePlugin.h"
#include "engine_tt.h"
#include "engine_io.h"
#include "gmgame.h"

ExamplePlugin::ExamplePlugin()
{
	Console_Output(__FUNCTION__ "\n");
	helloWorldString = "Hello World!";
	RegisterEvent(EVENT_GLOBAL_INI,this);
	RegisterEvent(EVENT_MAP_INI,this);
	RegisterEvent(EVENT_CHAT_HOOK,this);
	RegisterEvent(EVENT_OBJECT_CREATE_HOOK,this);
	RegisterEvent(EVENT_LOAD_LEVEL_HOOK,this);
	RegisterEvent(EVENT_GAME_OVER_HOOK,this);
	RegisterEvent(EVENT_PLAYER_JOIN_HOOK,this);
	RegisterEvent(EVENT_PLAYER_LEAVE_HOOK,this);
	RegisterEvent(EVENT_REFILL_HOOK,this);
	RegisterEvent(EVENT_POWERUP_PURCHASE_HOOK,this);
	RegisterEvent(EVENT_VEHICLE_PURCHASE_HOOK,this);
	RegisterEvent(EVENT_CHARACTER_PURCHASE_HOOK,this);
	RegisterEvent(EVENT_THINK_HOOK,this);
	RegisterEvent(EVENT_DIALOG_HOOK,this);
}

ExamplePlugin::~ExamplePlugin()
{
	Console_Output(__FUNCTION__ "\n");
	UnregisterEvent(EVENT_GLOBAL_INI,this);
	UnregisterEvent(EVENT_MAP_INI,this);
	UnregisterEvent(EVENT_CHAT_HOOK,this);
	UnregisterEvent(EVENT_OBJECT_CREATE_HOOK,this);
	UnregisterEvent(EVENT_LOAD_LEVEL_HOOK,this);
	UnregisterEvent(EVENT_GAME_OVER_HOOK,this);
	UnregisterEvent(EVENT_PLAYER_JOIN_HOOK,this);
	UnregisterEvent(EVENT_PLAYER_LEAVE_HOOK,this);
	UnregisterEvent(EVENT_REFILL_HOOK,this);
	UnregisterEvent(EVENT_POWERUP_PURCHASE_HOOK,this);
	UnregisterEvent(EVENT_VEHICLE_PURCHASE_HOOK,this);
	UnregisterEvent(EVENT_CHARACTER_PURCHASE_HOOK,this);
	UnregisterEvent(EVENT_THINK_HOOK,this);
	UnregisterEvent(EVENT_DIALOG_HOOK,this);
}

void ExamplePlugin::OnLoadGlobalINISettings(INIClass *SSGMIni)
{
	SSGMIni->Get_String(helloWorldString, "General", "Hello World!");
	Console_Output(__FUNCTION__ "\n");
}

void ExamplePlugin::OnFreeData()
{
	Console_Output(__FUNCTION__ "\n");
}

void ExamplePlugin::OnLoadMapINISettings(INIClass *SSGMIni)
{
	Console_Output(__FUNCTION__ "\n");
}

void ExamplePlugin::OnFreeMapData()
{
	Console_Output(__FUNCTION__ "\n");
}

bool ExamplePlugin::OnChat(int PlayerID,TextMessageEnum Type,const wchar_t *Message,int recieverID)
{
	Console_Output(__FUNCTION__ "\n");
	return true;
}

void ExamplePlugin::OnObjectCreate(void *data,GameObject *obj)
{
	Console_Output(__FUNCTION__ "\n");
	Attach_Script_Once(obj, "Plugin_Example_Script", "");
}

void ExamplePlugin::OnLoadLevel()
{
	Console_Output(__FUNCTION__ "\n");
	Console_Output("%s\n", helloWorldString);
}

void ExamplePlugin::OnGameOver()
{
	Console_Output(__FUNCTION__ "\n");
}

void ExamplePlugin::OnPlayerJoin(int PlayerID,const char *PlayerName)
{
	Console_Output(__FUNCTION__ "\n");
}

void ExamplePlugin::OnPlayerLeave(int PlayerID)
{
	Console_Output(__FUNCTION__ "\n");
}

bool ExamplePlugin::OnRefill(GameObject *purchaser)
{
	Console_Output(__FUNCTION__ "\n");
	return true;
}

PurchaseStatus ExamplePlugin::OnPowerupPurchase(BaseControllerClass *base,GameObject *purchaser,unsigned int cost,unsigned int preset,const char *data)
{
	Console_Output(__FUNCTION__ "\n");
	return PurchaseStatus_Allow;
}

PurchaseStatus ExamplePlugin::OnVehiclePurchase(BaseControllerClass *base,GameObject *purchaser,unsigned int cost,unsigned int preset,const char *data)
{
	Console_Output(__FUNCTION__ "\n");
	return PurchaseStatus_Allow;
}

PurchaseStatus ExamplePlugin::OnCharacterPurchase(BaseControllerClass *base,GameObject *purchaser,unsigned int cost,unsigned int preset,const char *data)
{
	Console_Output(__FUNCTION__ "\n");
	return PurchaseStatus_Allow;
}

void ExamplePlugin::OnThink()
{
	Console_Output(__FUNCTION__ "\n");
}

void ExamplePlugin::OnDialog(int PlayerID, int DialogID, int ControlID, DialogMessageType MessageType)
{
	Console_Output(__FUNCTION__ "\n");
}



void Example_Script::Created(GameObject *obj)
{
	printf(__FUNCTION__ "(%s)\n", Commands->Get_Preset_Name(obj));
}

void Example_Script::Destroyed(GameObject *obj)
{
	printf(__FUNCTION__ "(%s)\n", Commands->Get_Preset_Name(obj));
}

ScriptRegistrant<Example_Script> Plugin_Example_Script_Registrant("Example_Script","");



ExamplePlugin examplePlugin;

extern "C" __declspec(dllexport) Plugin* Plugin_Init()
{
	return &examplePlugin;
}
