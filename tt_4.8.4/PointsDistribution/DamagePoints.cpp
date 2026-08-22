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
#include "DamagePoints.h"


 
void CollectDamage::Created(GameObject *obj)
{
	/*
	if((strcmp(Commands->Get_Preset_Name(obj),"Destructible_Under_Pass"))== 0)
	{
		Set_Death_Points(obj, 0);
		Set_Damage_Points(obj, 0);
		StringClass msg;
		msg.Format("Fixing broken preset values for \"Destructible_Under_Pass\".\n");
		Console_Output(msg);
	}
	*/
	if(Get_Death_Points(obj) == 0 && Get_Damage_Points(obj) == 0)
	{
		return;
	}
	DeathPoints = Get_Death_Points(obj) + ((Get_Damage_Points(obj) * (Commands->Get_Health(obj) + Commands->Get_Shield_Strength(obj))));
	Set_Death_Points(obj, 0);
	Set_Damage_Points(obj, 0);
	for(int i = 0; i < 128; i++)
	{
		DamageInfo[i] = 0;
	}
}
 
void CollectDamage::Damaged(GameObject *obj, GameObject *Attacker, float damage)
{
	if(Commands->Is_A_Star(Attacker) && Commands->Get_Player_Type(Attacker) != Commands->Get_Player_Type(obj) && Get_Player_ID(Attacker) != Get_Player_ID(obj))
	{
		int APId = Get_Player_ID(Attacker);
		DamageInfo[APId] += damage;
	}
}	

 
void CollectDamage::Killed(GameObject *obj, GameObject *Killer)
{
	if(Commands->Get_Player_Type(obj) == 0 || Commands->Get_Player_Type(obj) == 1)
	{
		float TotalDamage;
		TotalDamage = 0;
		for(int i = 0; i < 128; i++)
		{
			if(DamageInfo[i] > 0)
			{
				TotalDamage += DamageInfo[i];
			}
		}
		for(int i = 0; i < 128; i++)
		{
			if(DamageInfo[i] > 0)
			{
				float points;
				points = (DamageInfo[i] / TotalDamage) * DeathPoints;
				Commands->Give_Points(Get_GameObj(i),points,false);
			}
		}
	}
}


ScriptRegistrant<CollectDamage> CollectDamage_Registrant("CollectDamage","");