#ifndef TT_INCLUDE__SAVEGAMEMANAGER_H
#define TT_INCLUDE__SAVEGAMEMANAGER_H
#include "engine_string.h"

class SaveGameManager
{

public:

	static REF_DECL2(MapFilename, StringClass);
	static REF_DECL2(CurrentGameFilename, StringClass);
	static REF_DECL2(Description, WideStringClass);
	static REF_DECL2(MissionDescriptionID, unsigned int);
	static const char* DefaultDefinitionFilename;

public:

	static void Save_Game(const char*, ...);
	static void Pre_Load_Game(const char*, StringClass&, StringClass&);
	static void Load_Game(const char* name);
	static UNK Smart_Peek_Description(const char*, WideStringClass&, WideStringClass&);
	static UNK Peek_Description(const char*, WideStringClass&, WideStringClass&);
	static void Peek_Map_Name(const char* name, StringClass& mapname);
	static void Save_Level();
	static void Load_Level();
	static UNK Save_Definitions(const char* filePath);
	static void Load_Definitions(const char* filePath);
	static void Save_Save_Load_System(const char* fileName, ...);
	static void Load_Save_Load_System(const char* fileName, bool);

};


#endif
