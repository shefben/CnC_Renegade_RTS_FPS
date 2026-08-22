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
#include "CharacterRefund.h"
#include "gmgame.h"


bool waskilled[128];
int currval[128];


class CHARACTERREFUND : public Plugin
{
public:
	float multiplier;
	CHARACTERREFUND()
	{
		Console_Output("Loading Character Refund Plugin; written by Reborn from MP-Gaming.COM\n");
		RegisterEvent(EVENT_GLOBAL_INI,this);
		RegisterEvent(EVENT_LOAD_LEVEL_HOOK,this);
		RegisterEvent(EVENT_OBJECT_CREATE_HOOK,this);
	}
	~CHARACTERREFUND()
	{
		Console_Output("Un-Loading Character Refund Plugin; written by Reborn from MP-Gaming.COM\n");
		UnregisterEvent(EVENT_GLOBAL_INI,this);
		UnregisterEvent(EVENT_LOAD_LEVEL_HOOK,this);
		UnregisterEvent(EVENT_OBJECT_CREATE_HOOK,this);
	}

	virtual void OnLoadGlobalINISettings(INIClass *SSGMIni)
	{
		multiplier = SSGMIni->Get_Float("CharacterRefund", "FractionOfRefund", 1.0f);
	}

	virtual void OnLoadLevel()
	{
		for(int i = 0; i < 128; i++)
		{
			waskilled[i] = false;
			currval[i] = 0;
		}
	}

	virtual void OnObjectCreate(void *data,GameObject *obj)
	{
		if(Commands->Is_A_Star(obj))
		{
			Attach_Script_Once(obj,"CharacterRefund","");
		}
	}
};

CHARACTERREFUND characterrefund;


void CharacterRefund::Created(GameObject *obj) {
	if(waskilled[Get_Player_ID(obj)])
	{
		//Player was killed, they do not qualify for a refund
	}
	else
	{
		//Player was not killed, they have just switched characters and do deserve a refund.
		Commands->Give_Money(obj, (float)currval[Get_Player_ID(obj)] * characterrefund.multiplier, 0);
	}
	waskilled[Get_Player_ID(obj)] = false;
	currval[Get_Player_ID(obj)] = Get_Cost((Commands->Get_Preset_Name(obj)));
}


void CharacterRefund::Killed(GameObject *obj, GameObject *shooter) 
{
	waskilled[Get_Player_ID(obj)] = true;
}

void CharacterRefund::Destroyed(GameObject *obj) 
{	
	waskilled[Get_Player_ID(obj)] = true;
}

ScriptRegistrant<CharacterRefund> CharacterRefund_Registrant("CharacterRefund","");



extern "C" __declspec(dllexport) Plugin* Plugin_Init()
{
	return &characterrefund;
}
