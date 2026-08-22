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
#include "SuddenDeath.h"
#include "gmgame.h"
#include "Iterator.h"
#include "CommandLineParser.h"
#include "GameObjManager.h"
#include "VehicleGameObj.h"
#include "BuildingGameObj.h"
#include "engine_tt.h"

static int getClientIdByIdentifier(const char* clientIdentifier)
{
	TT_ASSERT(clientIdentifier);
	const cPlayer* player = Find_Player(atoi(clientIdentifier));
	int result;
	if (player)
		result = player->Get_Id();
	else
		result = -1;
	return result;
}

static bool isClientId(const int id)
{
	return id > 0 && id < 128 && Find_Player(id);
}


class CommandSUDDENDEATH :
	public ConsoleFunctionClass
{
public:
	const char* Get_Name() 
	{ 
		return "SuddenDeath"; 
	}
	const char* Get_Help() 
	{ 
		return "SUDDENDEATH - Causes the game to go to sudden death mode."; 
	}
	void Activate(const char* argumentsString)
	{
		ActivateSuddenDeath();
	}
};



bool SuddenDeath = false;

class SUDDENDEATH : public Plugin
{
public:
	DynamicVectorClass<StringClass> BaseDefenseNames;
	bool ActivateOnRefs;
	SUDDENDEATH()
	{
		Console_Output("Loading Sudden Death Plugin; written by Reborn from MP-Gaming.COM\n");
		ConsoleFunctionList.Add(new CommandSUDDENDEATH);
		Sort_Function_List();
		Verbose_Help_File();
		RegisterEvent(EVENT_GLOBAL_INI,this);
		RegisterEvent(EVENT_LOAD_LEVEL_HOOK,this);
		RegisterEvent(EVENT_OBJECT_CREATE_HOOK,this);
	}
	~SUDDENDEATH()
	{
		Console_Output("Un-Loading Sudden Death Plugin; written by Reborn from MP-Gaming.COM\n");
		Delete_Console_Function("SuddenDeath");
		UnregisterEvent(EVENT_GLOBAL_INI,this);
		UnregisterEvent(EVENT_LOAD_LEVEL_HOOK,this);
		UnregisterEvent(EVENT_OBJECT_CREATE_HOOK,this);
	}

	virtual void OnLoadGlobalINISettings(INIClass *SSGMIni)
	{
		int count = SSGMIni->Entry_Count("BaseDefensePresetNames");
		for (int i = 0; i < count; i++)
		{
			const char* entry_name = SSGMIni->Get_Entry("BaseDefensePresetNames", i);
			StringClass entry_string; 
			SSGMIni->Get_String(entry_string, "BaseDefensePresetNames", entry_name, "42");
			BaseDefenseNames.Add(entry_string.Peek_Buffer());
		}
		ActivateOnRefs = SSGMIni->Get_Bool("SuddenDeath", "ActivateOnRefs", true);
	}

	virtual void OnLoadLevel()
	{
		SuddenDeath = false;
		AttachScriptToBuildings();
	}

	virtual void OnObjectCreate(void *data,GameObject *obj)
	{
		/*
		if(Is_Building(obj))
		{
			Commands->Attach_Script(obj,"SuddenDeathBuilding","");
		}
		*/
	}


};

SUDDENDEATH suddendeath;


void ActivateSuddenDeath()
{
	SuddenDeath = true;
	SuddenDeathMessages();
	DisableDefences();
}

void SuddenDeathMessages()
{
	Commands->Create_2D_WAV_Sound("c&c cloaking.wav");
	StringClass str;
	str.Format("msg Sudden Death mode has been activated!");
	Console_Input(str);
	Console_Input(str);
	str.Format("msg The next team to kill a building will win the map!");
	Console_Input(str);
	str.Format("msg Base defenses are now going offline...");
	Console_Input(str);
}


void DisableDefences()
{
	SLNode<BaseGameObj> *x = GameObjManager::GameObjList.Head();
	while (x)
	{
		GameObject *o = (GameObject *)x->Data();
		if (o)
		{
			for(int i = 0; i < suddendeath.BaseDefenseNames.Count(); i++)
			{
				if((strcmp(Commands->Get_Preset_Name(o), suddendeath.BaseDefenseNames[i]))== 0)
				{
					Commands->Destroy_Object(o);
				}
			}
		}
		x = x->Next();
	}
	if(Find_Base_Defense(0))
	{	
		Commands->Set_Building_Power(Find_Base_Defense(0), false);
	}
	if(Find_Base_Defense(1))
	{
		Commands->Set_Building_Power(Find_Base_Defense(1), false);
	}
}


void AttachScriptToBuildings()
{
	SLNode<BuildingGameObj> *x = GameObjManager::BuildingGameObjList.Head();
	while (x)
	{
		GameObject *o = (GameObject *)x->Data();
		if (o && o->As_BuildingGameObj())
		{
			Commands->Attach_Script(o, "SuddenDeathBuilding", "");
		}
		x = x->Next();
	}
}


void SuddenDeathBuilding::Killed(GameObject *obj, GameObject *Killer)
{
	if(SuddenDeath == true) //Building kill while sudden death mode is active means GG n00bs!
	{
		int team;
		team = Get_Object_Type(obj);
		if(team == 1)
		{
			Destroy_Base(0);
		}
		else
		{
			Destroy_Base(1);
		}
	}
	if(The_Game()->Get_Game_Duration_S() <= 300) //If it less than 5 minutes into the game.
	{
		if(obj->As_BuildingGameObj() && obj->As_BuildingGameObj()->As_RefineryGameObj())
		{
			if(suddendeath.ActivateOnRefs == true)
			{
				GameObject *EnemyRef;
				if(Get_Object_Type(obj) == 1)
				{
					EnemyRef = Find_Refinery(0);
				}
				else
				{
					EnemyRef = Find_Refinery(1);
				}
				if(!EnemyRef || Is_Building_Dead(EnemyRef))
				{
					ActivateSuddenDeath();
				}
			}
		}
	}
}


ScriptRegistrant<SuddenDeathBuilding> SuddenDeathBuilding_Registrant("SuddenDeathBuilding","");

extern "C" __declspec(dllexport) Plugin* Plugin_Init()
{
	return &suddendeath;
}
