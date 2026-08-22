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
#include "NoPoints.h"
#include "gmgame.h"
#include "GameObjManager.h"

class NOPOINTS : public Plugin
{
public:
	int Points;
	NOPOINTS()
	{
		Console_Output("Loading No Points Plugin; written by Reborn from MP-Gaming.COM\n");
		RegisterEvent(EVENT_LOAD_LEVEL_HOOK,this);
		RegisterEvent(EVENT_OBJECT_CREATE_HOOK,this);
	}
	~NOPOINTS()
	{
		Console_Output("Un-Loading No Points Plugin; written by Reborn from MP-Gaming.COM\n");
		UnregisterEvent(EVENT_LOAD_LEVEL_HOOK,this);
		UnregisterEvent(EVENT_OBJECT_CREATE_HOOK,this);
	}

	virtual void OnLoadLevel()
	{
		SLNode<BaseGameObj> *x = GameObjManager::GameObjList.Head();
		while (x)
		{
			GameObject *o = (GameObject *)x->Data();
			if (o && Commands->Get_Health(o) > 0)
			{
				Attach_Script_Once(o, "nullify", "");
			}
			x = x->Next();
		}
	}

	virtual void OnObjectCreate(void *data,GameObject *obj)
	{
		Attach_Script_Once(obj, "nullify", "");
	}
};

NOPOINTS nopoints;


void nullify::Created(GameObject *obj)
{
	DamageCredits = Get_Damage_Points(obj);
	DeathCredits = Get_Death_Points(obj);
	Set_Death_Points(obj, 0);
	Set_Damage_Points(obj, 0);
}


void nullify::Damaged(GameObject *obj, GameObject *Attacker, float damage)
{
	Commands->Give_Money(Attacker, damage * DamageCredits, false);
}


void nullify::Killed(GameObject *obj, GameObject *shooter)
{
	Commands->Give_Money(shooter, DeathCredits, false);
}


ScriptRegistrant<nullify> nullify_Registrant("nullify","");


extern "C" __declspec(dllexport) Plugin* Plugin_Init()
{
	return &nopoints;
}
