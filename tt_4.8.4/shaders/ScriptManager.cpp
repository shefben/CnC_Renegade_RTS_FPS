#include "General.h"
#include "ScriptManager.h"
#include "engine_obj.h"
#include "SaveLoadSystemClass.h"
#include "GameObjObserverManager.h"
#include "CombatManager.h"
#include "cGameType.h"

cs ScriptManager::ScriptCreateFunct = 0;
ds ScriptManager::ScriptDestroyFunct = 0;
bool ScriptManager::EnableScriptCreation = true;
HMODULE ScriptManager::hDLL;
SimpleDynVecClass<ScriptClass *> ScriptManager::PendingDestroyList;
SimpleDynVecClass<ScriptClass *> ScriptManager::ActiveScriptList;
ScriptCommandsClass ScriptManager::EngineCommands;

void ScriptManager::Init()
{
	hDLL = 0;
	EngineCommands = Get_Script_Commands();
	Load_Scripts("SCRIPTS.DLL");
}

void ScriptManager::Shutdown()
{
	Destroy_Pending();
	while (ActiveScriptList.Count())
	{
		ScriptDestroyFunct(ActiveScriptList[0]);
		ActiveScriptList.Delete(0);
	}
	if (hDLL)
	{
		FreeLibrary(hDLL);
	}
}

void ScriptManager::Destroy_Pending()
{
	for (int i = 0;i < PendingDestroyList.Count();i++)
	{
		ScriptableGameObj *owner = PendingDestroyList[i]->Owner();
		if (owner)
		{
			owner->Remove_Observer(PendingDestroyList[i]);
		}
		ScriptDestroyFunct(PendingDestroyList[i]);
	}
	PendingDestroyList.Delete_All();
}

void ScriptManager::Load_Scripts(char const *name)
{
	if (IS_SOLOPLAY || !CombatManager::I_Am_Only_Client())
	{
		int LastError;
		hDLL = LoadLibrary(name);
		LastError = GetLastError();
		if (!hDLL)
		{
			FILE *f = fopen("dllload.txt","at");
			if (f)
			{
				char *errorMessage = new char[2048]; //Ugly, but whatever.
				FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,NULL,LastError,0,errorMessage,2048,NULL);
				fprintf(f,"[scripts.dll] Failed to load, error %d. %s\n",LastError,errorMessage);
				delete[] errorMessage;
				fclose(f);
			}
			MessageBox(NULL,"Unfortunately, there was an error loading scripts.dll. Please report this bug!","Error",MB_OK|MB_ICONEXCLAMATION|MB_TOPMOST);
			ExitProcess(LastError);
		}
		ScriptCreateFunct = (cs)GetProcAddress(hDLL,"Create_Script");
		ScriptDestroyFunct = (ds)GetProcAddress(hDLL,"Destroy_Script");
		srdf SetRequestDestroyFunc = (srdf)GetProcAddress(hDLL,"Set_Request_Destroy_Func");
		if (SetRequestDestroyFunc)
		{
			SetRequestDestroyFunc(Request_Destroy_Script);
		}
		if (CombatManager::Are_Observers_Active())
		{
			ssc SetScriptCommands = (ssc)GetProcAddress(hDLL,"Set_Script_Commands");
			if (SetScriptCommands)
			{
				if (!SetScriptCommands(&EngineCommands))
				{
					ScriptCreateFunct = 0;
				}
			}
		}
	}
}

ScriptClass *ScriptManager::Create_Script(char const *name)
{
	if (EnableScriptCreation && ScriptCreateFunct)
	{
		ScriptClass *script = ScriptCreateFunct(name);
		if (script)
		{
			script->Set_ID(GameObjObserverManager::NextID);
			GameObjObserverManager::NextID++;
			ActiveScriptList.Add(script);
			return script;
		}
	}
	return 0;
}

void ScriptManager::Request_Destroy_Script(ScriptClass *script)
{
	ActiveScriptList.Delete(script);
	if (PendingDestroyList.Find_Index(script) == -1)
	{
		PendingDestroyList.Add(script);
	}
}

class ScriptSaver {
public:
	ChunkSaveClass *csave;
};
class ScriptLoader {
public:
	ChunkLoadClass *cload;
};

bool ScriptManager::Save(ChunkSaveClass &csave)
{
	for (int i = 0;i < ActiveScriptList.Count();i++)
	{
		ScriptClass *script = ActiveScriptList[i];
		char params[256];
		csave.Begin_Chunk(131001134);
		csave.Begin_Chunk(131001135);
		StringClass name = script->Get_Name();
		csave.Begin_Micro_Chunk(1);
		csave.Write(name.Peek_Buffer(),name.Get_Length());
		csave.End_Micro_Chunk();
		script->Get_Parameters_String(params,256);
		csave.Begin_Micro_Chunk(2);
		csave.Write(params,strlen(params));
		csave.End_Micro_Chunk();
		csave.Begin_Micro_Chunk(3);
		csave.Write(&script,4);
		csave.End_Micro_Chunk();
		csave.Begin_Micro_Chunk(4);
		ScriptableGameObj **obj = script->Get_Owner_Ptr();
		csave.Write(&obj,4);
		csave.End_Micro_Chunk();
		csave.Begin_Micro_Chunk(5);
		int id = script->Get_ID();
		csave.Write(&id,4);
		csave.End_Micro_Chunk();
		csave.End_Chunk();
		if (CombatManager::Are_Observers_Active())
		{
			ScriptSaver save;
			save.csave = &csave;
			csave.Begin_Chunk(131001136);
			script->Save(save);
			csave.End_Chunk();
		}
		csave.End_Chunk();
	}
	return true;
}

bool ScriptManager::Load(ChunkLoadClass &cload)
{
	ScriptClass *script = 0;
	void *scriptptr = 0;
	void *objptr = 0;
	int id = 0;
	while (cload.Open_Chunk())
	{
		cload.Open_Chunk();
		while (cload.Open_Micro_Chunk())
		{
			switch (cload.Cur_Micro_Chunk_ID())
			{
			case 1:
				{
					StringClass name;
					char *buffer = name.Get_Buffer(cload.Cur_Micro_Chunk_Length());
					cload.Read(buffer,cload.Cur_Micro_Chunk_Length());
					script = Create_Script(buffer);
				}
				break;
			case 2:
				if (script)
				{
					StringClass params;
					char *buffer = params.Get_Buffer(cload.Cur_Micro_Chunk_Length());
					cload.Read(buffer,cload.Cur_Micro_Chunk_Length());
					script->Set_Parameters_String(params);
				}
				break;
			case 3:
				cload.Read(&scriptptr,4);
				break;
			case 4:
				cload.Read(&objptr,4);
				break;
			case 5:
				cload.Read(&id,4);
				break;
			}
			cload.Close_Micro_Chunk();
		}
		cload.Close_Chunk();
		if (script)
		{
			if (id != -1)
			{
				script->Set_ID(id);
			}
			if (cload.Open_Chunk())
			{
				ScriptLoader load;
				load.cload = &cload;
				script->Load(load);
				cload.Close_Chunk();
			}
			if (scriptptr)
			{
				SaveLoadSystemClass::Register_Pointer(scriptptr,script);
			}
			script->Get_Owner_Ptr();
			*(script->Get_Owner_Ptr()) = (GameObject *)objptr;
			SaveLoadSystemClass::Request_Pointer_Remap((void **)script->Get_Owner_Ptr());
		}
		else
		{
			SaveLoadSystemClass::Register_Pointer(scriptptr,0);
		}
		cload.Close_Chunk();
	}
	return true;
}

RENEGADE_FUNCTION
ScriptCommandsClass ScriptManager::Get_Script_Commands()
AT2(0x0072BD00,0x0072B5A0);

RENEGADE_FUNCTION
void SaveLoadSystemClass::Register_Pointer
   (void*, void*)
   AT2(0x00525D20,0x005255C0);

RENEGADE_FUNCTION
void SaveLoadSystemClass::Request_Pointer_Remap
   (void**)
   AT2(0x00525D40,0x005255E0);
