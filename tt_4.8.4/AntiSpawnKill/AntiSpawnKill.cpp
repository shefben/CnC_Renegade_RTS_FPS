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
#include "AntiSpawnKill.h"
#include "gmgame.h"

bool waskilled[128];

class ANTISPAWNKILL : public Plugin
{
public:
	float time;
	ANTISPAWNKILL()
	{
		Console_Output("Loading Anti Spawn Kill Plugin; written by Reborn from MP-Gaming.COM\n");
		RegisterEvent(EVENT_GLOBAL_INI,this);
		RegisterEvent(EVENT_LOAD_LEVEL_HOOK,this);
		RegisterEvent(EVENT_OBJECT_CREATE_HOOK,this);
	}
	~ANTISPAWNKILL()
	{
		Console_Output("Un-Loading Anti Spawn Kill Plugin; written by Reborn from MP-Gaming.COM\n");
		UnregisterEvent(EVENT_GLOBAL_INI,this);
		UnregisterEvent(EVENT_LOAD_LEVEL_HOOK,this);
		UnregisterEvent(EVENT_OBJECT_CREATE_HOOK,this);
	}

	virtual void OnLoadGlobalINISettings(INIClass *SSGMIni)
	{
		time = SSGMIni->Get_Float("AntiSpawnKill", "Time", 0.3f);
	}

	virtual void OnLoadLevel()
	{
		for(int i = 0; i < 128; i++)
		{
			waskilled[i] = true;
		}
	}

	virtual void OnObjectCreate(void *data,GameObject *obj)
	{
		if(Commands->Is_A_Star(obj))
		{
			Attach_Script_Once(obj,"AntiSpawnKill","");
		}
	}
};

ANTISPAWNKILL antispawnkill;


void AntiSpawnKill::Created(GameObject *obj) 
{
	if(waskilled[Get_Player_ID(obj)])
	{
		waskilled[Get_Player_ID(obj)] = false;
		shield = Get_Skin(obj);
		skin = Get_Shield_Type(obj);
		Commands->Set_Shield_Type(obj, "blamo");
		Set_Skin(obj,"blamo");
		Commands->Start_Timer(obj, this, antispawnkill.time, 1);
	}
}

void AntiSpawnKill::Killed(GameObject *obj, GameObject *shooter) 
{
	waskilled[Get_Player_ID(obj)] = true;
}

void AntiSpawnKill::Destroyed(GameObject *obj) 
{	
	waskilled[Get_Player_ID(obj)] = true;
}

void AntiSpawnKill::Timer_Expired(GameObject *obj, int number) 
{
	if (number == 1) 
	{
		Commands->Set_Shield_Type(obj, shield);
		Set_Skin(obj, skin);
	}
}



ScriptRegistrant<AntiSpawnKill> AntiSpawnKill_Registrant("AntiSpawnKill","");


extern "C" __declspec(dllexport) Plugin* Plugin_Init()
{
	return &antispawnkill;
}
