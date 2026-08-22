#ifndef TT_INCLUDE_SCRIPTMANAGER_H
#define TT_INCLUDE_SCRIPTMANAGER_H
#include "engine_vector.h"
#include "scripts.h"
#include "engine_io.h"
typedef void (*srdf) (void (*function)(ScriptClass*));
typedef void (*ds) (ScriptClass *);
typedef ScriptClass *(*cs) (const char *);
typedef int (*gsc) ();
typedef char *(*gsn) (int);
typedef char *(*gspd) (int);
typedef bool (*ssc) (ScriptCommandsClass *);
class ScriptManager {
public:
	static cs ScriptCreateFunct;
	static ds ScriptDestroyFunct;
	static bool EnableScriptCreation;
	static HMODULE hDLL;
	static SimpleDynVecClass<ScriptClass *> PendingDestroyList;
	static SimpleDynVecClass<ScriptClass *> ActiveScriptList;
	static ScriptCommandsClass EngineCommands;
	static SHADERS_API void Init();
	static SHADERS_API void Shutdown();
	static SHADERS_API void Destroy_Pending();
	static void Load_Scripts(char const *name);
	static SHADERS_API ScriptClass *Create_Script(char const *name);
	static void Request_Destroy_Script(ScriptClass *script);
	static SHADERS_API bool Save(ChunkSaveClass &saver);
	static SHADERS_API bool Load(ChunkLoadClass &loader);
	static ScriptCommandsClass Get_Script_Commands();
};
#endif