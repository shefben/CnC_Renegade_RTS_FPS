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
#include "RandomStartingCredits.h"
#include "gmgame.h"

class RANDOMSTARTINGCREDITS : public Plugin
{
public:
	int Min;
	int Max;
	RANDOMSTARTINGCREDITS()
	{
		Console_Output("Loading Random Starting Credits Plugin; written by Reborn from MP-Gaming.COM\n");
		RegisterEvent(EVENT_GLOBAL_INI,this);
		RegisterEvent(EVENT_LOAD_LEVEL_HOOK,this);
	}
	~RANDOMSTARTINGCREDITS()
	{
		Console_Output("Un-Loading Random Starting Credits Plugin; written by Reborn from MP-Gaming.COM\n");
		UnregisterEvent(EVENT_GLOBAL_INI,this);
		UnregisterEvent(EVENT_LOAD_LEVEL_HOOK,this);
	}

	virtual void OnLoadGlobalINISettings(INIClass *SSGMIni)
	{
		Min = SSGMIni->Get_Int("RANDOMSTARTINGCREDITS", "Minimum", 1);
		Max = SSGMIni->Get_Int("RANDOMSTARTINGCREDITS", "Maximum", 200);
	}


	virtual void OnLoadLevel()
	{
		int rand = Commands->Get_Random_Int(Min, Max);
		The_Cnc_Game()->Set_Starting_Credits(rand);
	}

};

RANDOMSTARTINGCREDITS randomstartingcredits;



extern "C" __declspec(dllexport) Plugin* Plugin_Init()
{
	return &randomstartingcredits;
}
