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
#include "PointsDistribution.h"
#include "DamagePoints.h"
#include "gmgame.h"
#include "Iterator.h"
#include "SoldierGameObj.h"

class POINTSDISTRIBUTION : public Plugin
{
public:
	bool EnabledForInfantry, EnabledForVehicles, EnabledForBeacons, EnabledForC4, EnabledForBuildings; 
	POINTSDISTRIBUTION()
	{
		Console_Output("Loading Points Distribution Plugin; written by Reborn from MP-Gaming.COM\n");
		RegisterEvent(EVENT_OBJECT_CREATE_HOOK,this);
		RegisterEvent(EVENT_GLOBAL_INI,this);
	}
	~POINTSDISTRIBUTION()
	{
		Console_Output("Un-loading Points Distribution Plugin; written by Reborn from MP-Gaming.COM\n");
		UnregisterEvent(EVENT_OBJECT_CREATE_HOOK,this);
		UnregisterEvent(EVENT_GLOBAL_INI,this);
	}

	virtual void OnLoadGlobalINISettings(INIClass *SSGMIni)
	{
		EnabledForInfantry = SSGMIni->Get_Bool("PointsDistribution", "EnabledForInfantry", true);
		EnabledForVehicles = SSGMIni->Get_Bool("PointsDistribution", "EnabledForVehicles", true);
		EnabledForBeacons = SSGMIni->Get_Bool("PointsDistribution", "EnabledForBeacons", true);
		EnabledForC4 = SSGMIni->Get_Bool("PointsDistribution", "EnabledForC4", true);
		EnabledForBuildings = SSGMIni->Get_Bool("PointsDistribution", "EnabledForBuildings", true);
	}

	virtual void OnObjectCreate(void *data,GameObject *obj)
	{
		if (obj->As_SoldierGameObj() && EnabledForInfantry == true)
		{
			Attach_Script_Once(obj,"CollectDamage","");
		}
		else if (obj->As_VehicleGameObj() && EnabledForVehicles == true) 
		{
			Attach_Script_Once(obj,"CollectDamage","");
		}
		else if (obj->As_PhysicalGameObj() && obj->As_PhysicalGameObj()->As_BeaconGameObj() && EnabledForBeacons == true) 
		{
			Attach_Script_Once(obj,"CollectDamage","");
		}
		else if (obj->As_PhysicalGameObj() && obj->As_PhysicalGameObj()->As_C4GameObj() && EnabledForC4 == true) 
		{
			Attach_Script_Once(obj,"CollectDamage","");
		}
		else if(obj->As_BuildingGameObj() && EnabledForBuildings == true)
		{
			Attach_Script_Once(obj,"CollectDamage","");
		}
		/*
		StringClass msg;
		msg.Format("Preset name:%s\nDeath Points: %f\nDamage Points:%f\nHealth: %f\nArmor: %f\nSkin: %s\n", Commands->Get_Preset_Name(obj), Get_Death_Points(obj), Get_Damage_Points(obj), Commands->Get_Health(obj), Commands->Get_Shield_Strength(obj), Get_Skin(obj));
		Console_Output(msg);
		*/
	}
};

POINTSDISTRIBUTION pointsdistribution;


extern "C" __declspec(dllexport) Plugin* Plugin_Init()
{
	return &pointsdistribution;
}
