/*	Renegade tt.dll
	Shader developer mode console commands
	Copyright 2009 Mark Sararu
	This file is part of the renegade tt.dll.
	CONFIDENTIAL: DO NOT USE OR DISTRIBUTE WITHOUT PERMISSION
*/
#include "general.h"

#include "tt.h"
#include "shaders.h"
#include "consolecommands.h"
#include "shaderdev_console.h"
#include "DX8Wrapper.h"
#include "WaterSystemClass.h"

extern REF_DECL2(ConsoleFunctionList,DynamicVectorClass<ConsoleFunctionClass *>);
static void AddToConsoleFunctionList(ConsoleFunctionClass *cmd)
{
	ConsoleFunctionList.Add(cmd);
}

void IntializeShaderDevConsoleCommands()
{
	if (!QueryShaderDeveloperMode()) return;
	AddToConsoleFunctionList(new ResetShaders_ConsoleCommand());
	AddToConsoleFunctionList(new ResetDevice_ConsoleCommand());
	AddToConsoleFunctionList(new ResetWater_ConsoleCommand());
};


void ShadersReset();
void ResetShaders_ConsoleCommand::Activate(char *)
{
	//FIXME FIXME FIXME FIXME
	//Add file load history system to shaders.dll 
	//Add shaders.dll function to unload the active shader databases and replay history
	ShadersReset();
};

void ResetDevice_ConsoleCommand::Activate(char *)
{
	RenderDeviceSettings::Set_Settings_Dirty(true);
};

void ResetWater_ConsoleCommand::Activate(char *)
{
	INIClass* ini = Get_INI("water.ini");
	WaterSystemClass::Get_Instance()->Load_From_INI("Water", ini);
	Release_INI(ini);
}