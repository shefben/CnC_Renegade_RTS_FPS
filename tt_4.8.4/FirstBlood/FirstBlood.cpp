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
#include "FirstBlood.h"
#include "gmgame.h"

bool first = false;

class FIRSTBLOOD : public Plugin
{
public:
	int Points;
	FIRSTBLOOD()
	{
		Console_Output("Loading First Blood Plugin; written by Reborn from MP-Gaming.COM\n");
		RegisterEvent(EVENT_GLOBAL_INI,this);
		RegisterEvent(EVENT_LOAD_LEVEL_HOOK,this);
		RegisterEvent(EVENT_OBJECT_CREATE_HOOK,this);
	}
	~FIRSTBLOOD()
	{
		Console_Output("Un-Loading First Blood Plugin; written by Reborn from MP-Gaming.COM\n");
		UnregisterEvent(EVENT_GLOBAL_INI,this);
		UnregisterEvent(EVENT_LOAD_LEVEL_HOOK,this);
		UnregisterEvent(EVENT_OBJECT_CREATE_HOOK,this);
	}

	virtual void OnLoadGlobalINISettings(INIClass *SSGMIni)
	{
		Points = SSGMIni->Get_Int("FirstBlood", "Points", 300);
	}

	virtual void OnLoadLevel()
	{
		first = false;
	}

	virtual void OnObjectCreate(void *data,GameObject *obj)
	{
		if(Commands->Is_A_Star(obj))
		{
			Attach_Script_Once(obj,"FirstBlood","");
		}
	}
};

FIRSTBLOOD firstblood;


void FirstBlood::Killed(GameObject *obj, GameObject *Killer)
{
	if(first == false)
	{
		if(Commands->Is_A_Star(Killer) && Killer != obj)
		{
			Commands->Give_Points(Killer, (float)firstblood.Points, false);
			StringClass Msg;
			const char *str = Get_Player_Name(Killer);
			Msg.Format("msg %s got the first kill, and was awarded %i points for it!", str, firstblood.Points);
			delete[] str;
			Console_Input(Msg);
			first = true;
			if (PlayQuakeSounds)
			{
				Commands->Create_2D_WAV_Sound("firstblood.wav");
			}
		}
	}
}

ScriptRegistrant<FirstBlood> FirstBlood_Registrant("FirstBlood","");

extern "C" __declspec(dllexport) Plugin* Plugin_Init()
{
	return &firstblood;
}
