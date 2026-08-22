/*	Renegade Scripts.dll
	Main code for shaders.dll implementation
	Copyright 2009 Jonathan Wilson

	This file is part of the Renegade scripts.dll
	The Renegade scripts.dll is free software; you can redistribute it and/or modify it under
	the terms of the GNU General Public License as published by the Free
	Software Foundation; either version 2, or (at your option) any later
	version. See the file COPYING for more details.
	In addition, an exemption is given to allow Run Time Dynamic Linking of this code with any closed source module that does not contain code covered by this licence.
	Only the source code to the module(s) containing the licenced code has to be released.
*/
#include "general.h"

#include "shader.h"
#include "shaderstatemanager.h"
#include "TextureController.h"
#include "straw.h"
#include "FileHash.h"
#include "RadarManager.h"
#include "scripts.h"
#include "engine_3d.h"
#include "engine_game.h"

int ShaderDetail = 0;
int SceneShaderDetail = 0;
GlobalCheck CheckGlobal;
MapCheck CheckMap;
LoadCheck CheckLoad;


bool RenderScreenFade = true;


extern char *MapTexture;
extern Vector2 MapOffset;
ScriptNotify scriptnotify;

ScriptCommands* Commands;



#if D3DX_SDK_VERSION != 36
#error The Renegade shaders.dll requires the DirectX November 2007 SDK installed in order to produce a proper executable. If you wish to use a different version of the SDK, modify the delay load dll in linker settings to point to the newer d3dx.dll
#endif


bool __declspec(dllexport) QueryShaderDeveloperMode()
{
	//FIXME FIXME FIXME
	//This option needs to be read from the global shaders.ini or shaders.shd and that file must be enforced.
	return true;
};

void ReleaseResources()
{
	ResetDeclarationBuffer();
	if (ShaderDetail)
	{
		ShaderController.OnDeviceLost();
	}
}

void ReloadResources()
{
	StateManager::Reset();
	if (ShaderDetail)
	{
		ShaderController.OnDeviceReset();
	}
}

void DestroyResources()
{
	RenderTargetController::Instance()->Release();
	ResetDeclarationBuffer();
	StateManager::Shutdown();
	if (ShaderDetail)
	{
		ShaderController.UnloadDatabase();
	}
	EffectSystem::Destroy();
}

void ReadMapINI();
void __declspec(dllexport) MapLoaded()
{
	RadarManager::BracketObj = 0;
	CalculateTangents = true;
	if (ShaderDetail)
	{
		char *c = newstr(The_Game()->MapName);
		char *c2 = strchr(c,'.');
		if (c2)
		{
			c2[1] = 's';
			c2[2] = 'h';
			c2[3] = 'd';
		}
		FileClass *f = Get_Data_File(c);
		if (f)
		{
			f->Open(1);
			ShaderController.AppendDatabase(f);
			Close_Data_File(f);
		}
		delete[] c;
	}
	ReadMapINI();
}

void __declspec(dllexport) MapUnloaded()
{
	CalculateTangents = false;
}

void FrameStart()
{
	//TODO: Scene Shader Hooks
}

void FrameEnd()
{
	//TODO: Scene Shader Hooks;
}

void __declspec(dllexport) ShaderSet(const char *value,const char *value2)
{
	value;
	value2;
}

void __declspec(dllexport) ShaderSetObj(const char *value,const char *value2,GameObject *obj)
{
	value;
	value2;
	obj;
}

void __declspec(dllexport) ScopeTrigger(bool enabled)
{
}

void __declspec(dllexport) ScopeChange(int scope)
{
}

void __declspec(dllexport) ShaderDetailChanged(int detail)
{
	HKEY key;
	RegOpenKeyEx(HKEY_LOCAL_MACHINE,(char *)0x007F5274,0,KEY_ALL_ACCESS,&key);
	RegSetValueEx(key,"ShaderDetail",0,REG_DWORD,(BYTE *)&detail,4);
	RegCloseKey(key);
};

void __declspec(dllexport) SceneShaderDetailChanged(int detail)
{
	HKEY key;
	RegOpenKeyEx(HKEY_LOCAL_MACHINE,(char *)0x007F5274,0,KEY_ALL_ACCESS,&key);
	RegSetValueEx(key,"SceneShaderDetail",0,REG_DWORD,(BYTE *)&detail,4);
	RegCloseKey(key);
};

const unsigned char Code[19] = {0x6A,0x40,0xFF,0x74,0x24,0x0C,0xFF,0x74,0x24,0x0C,0xE8,0xC0,0xFF,0xFF,0xFF,0x83,0xC4,0x0C,0xC3};

extern bool suppressSceneTextureUpdate;
void __declspec(dllexport) ShadersReset()
{
	ShaderController.UnloadDatabase();
	FileClass *shaderdb = Get_Data_File("shaders.shd");
	if (!ShaderController.AppendDatabase(shaderdb))
	{
		INIClass *ini = Get_INI("shaders.ini");
		ShaderController.LoadDatabaseFromINI(ini);
		Release_INI(ini);
	}
	Close_Data_File(shaderdb);
	ShaderController.Initialize();
};

void SHADERS_API InitShaders()
{
	Commands = (ScriptCommands*)0x0085F490;
	if (!memcmp((void *)0x0078CE49,(void *)Code,19))
	{
		ShaderCaps::Initialize();
		StateManager::Initialize();
		
		if ((ShaderCaps::VertexShaderVersion) && (ShaderCaps::PixelShaderVersion))
		{
			ShaderDetail = Get_Registry_Int("ShaderDetail",2);
			SceneShaderDetail = Get_Registry_Int("SceneShaderDetail",0);
		}
		else
		{
			ShaderDetail = 0;
			SceneShaderDetail = 0;
		}

		MapTexture = new char[260];
		MapOffset = Vector2(0,0);

		if (ShaderDetail)
		{
			EffectSystem::Initialize();
			
			FileClass *shaderdb = Get_Data_File("shaders.shd");
			if (!ShaderController.AppendDatabase(shaderdb))
			{
				INIClass *ini = Get_INI("shaders.ini");
				ShaderController.LoadDatabaseFromINI(ini);
				Release_INI(ini);
			}
			Close_Data_File(shaderdb);
			ShaderController.Initialize();
		}

		{
		RenderTargetController* rtc = RenderTargetController::Instance();
		INIClass *ini = Get_INI("leethax.ini");
		if (ini)
		{
			rtc->LoadFromINI(ini);
		};
		}

	}	
};

void SHADERS_API UnloadShaders()
{
	delete[] MapTexture;
	StateManager::Shutdown();
};

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			TT_ASSERT(Exe != 6);
			break;
		case DLL_PROCESS_DETACH:
			UnloadShaders();
			break;
	}
	return TRUE;
}

void __declspec(dllexport) SetScriptNotify(ScriptNotify notify)
{
	scriptnotify = notify;
}

void __declspec(dllexport) SetHashChecks(MapCheck m,GlobalCheck g,LoadCheck l)
{
	CheckGlobal = g;
	CheckMap = m;
	CheckLoad = l;
}
