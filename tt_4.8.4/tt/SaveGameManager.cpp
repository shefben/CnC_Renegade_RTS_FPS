#include "General.h"
#include "SaveGameManager.h"

#include "engine_io.h"
#include "SaveLoadSystemClass.h"
#include "CombatManager.h"
#include "DataSafe.h"
#include "DefinitionMgrClass.h"
#include "FileHashEvent.h"
#include "SaveLoadSubSystemClass.h"
#include "BackgroundMgrClass.h"
#include "WeatherMgrClass.h"
#include "FileFactoryListClass.h"



const char *SaveGameManager::DefaultDefinitionFilename = "Objects.DDB";
REF_DEF2(SaveGameManager::MapFilename, StringClass, 0x008572E8, 0x008564D0);
REF_DEF2(SaveGameManager::CurrentGameFilename, StringClass, 0x008572EC, 0x008564D4);
REF_DEF2(SaveGameManager::Description, WideStringClass, 0x008572F0, 0x008564D8);
REF_DEF2(SaveGameManager::MissionDescriptionID, unsigned int, 0x008572F4, 0x008564DC);
REF_DEF2(_load_map_name, StringClass, 0x00855EAC, 0x00855094);
void SaveGameManager::Load_Definitions
   (const char* cFile)
{
   SaveGameManager::Load_Save_Load_System (cFile, true);
}
void SaveGameManager::Load_Save_Load_System
   (const char* fileName, bool auto_post_load)
{
   FileClass* file = _TheFileFactory->Get_File (fileName);
   if (!file)
      return;

   file->Open (1);
   ChunkLoadClass chunkLoader (file);
   SaveLoadSystemClass::Load (chunkLoader, auto_post_load);
   file->Close();

   _TheFileFactory->Return_File (file);
}
void SaveGameManager::Load_Level()
{
	FileClass *f = Get_Data_File(MapFilename);
	if ((f) && (f->Is_Available()))
	{
		unsigned int size = f->Size();
		char *data = new char[size];
		f->Open(1);
		f->Read(data,size);
		CheckMap(data,size,MapFilename.Peek_Buffer(),HashMapLsd);
		delete[] data;
		f->Close();
		Close_Data_File(f);
	}
	Load_Save_Load_System(MapFilename,false);
}
void SaveGameManager::Peek_Map_Name(char  const *name, StringClass &mapname)
{
	FileClass *f = Get_Data_File(name);
	f->Open(1);
	ChunkLoadClass cload(f);
	bool b = false;
	while (cload.Open_Chunk() && !b)
	{
		if (cload.Cur_Chunk_ID() == 0x3C51C460)
		{
			while (!b)
			{
				cload.Open_Micro_Chunk();
				if (cload.Cur_Micro_Chunk_ID() == 1)
				{
					mapname.Get_Buffer(cload.Cur_Micro_Chunk_Length());
					cload.Read(mapname.Peek_Buffer(),cload.Cur_Micro_Chunk_Length());
					b = true;
				}
				cload.Close_Micro_Chunk();
			}
		}
		cload.Close_Chunk();
	}
	f->Close();
	Close_Data_File(f);
}
void SaveGameManager::Load_Game(char  const* name)
{
	CurrentGameFilename = name;
	FileClass *f = Get_Data_File(name);
	f->Open(1);
	ChunkLoadClass cload(f);
	while (cload.Open_Chunk())
	{
		if (cload.Cur_Chunk_ID() == 0x3C51C460)
		{
			while (cload.Open_Micro_Chunk())
			{
				switch (cload.Cur_Micro_Chunk_ID())
				{
				case 1:
					MapFilename.Get_Buffer(cload.Cur_Micro_Chunk_Length());
					cload.Read(MapFilename.Peek_Buffer(),cload.Cur_Micro_Chunk_Length());
					break;
				case 2:
					unsigned int temp;
					cload.Read(&temp,4);
					MissionDescriptionID = temp;
					break;
				case 3:
					Description.Get_Buffer(cload.Cur_Micro_Chunk_Length());
					cload.Read(Description.Peek_Buffer(),cload.Cur_Micro_Chunk_Length());
					break;
				}
				cload.Close_Micro_Chunk();
			}
			StringClass ddbname = MapFilename;
			ddbname.Erase(ddbname.Get_Length()-4,4);
			ddbname += ".ddb";
			FileClass *f2 = Get_Data_File(ddbname);
			if ((f2) && (f2->Is_Available()))
			{
				unsigned int size = f2->Size();
				char *data = new char[size];
				f2->Open(1);
				f2->Read(data,size);
				CheckMap(data,size,ddbname.Peek_Buffer(),HashMapDdb);
				delete[] data;
				f2->Close();
				Close_Data_File(f2);
			}
			Load_Definitions(ddbname);
			Load_Level();
		}
		else if (cload.Cur_Chunk_ID() == 0x3C51C461)
		{
			if (CombatManager::I_Am_Server())
			{
				SaveLoadSystemClass::Load(cload,false);
			}
		}
		cload.Close_Chunk();
	}
	f->Close();
	Close_Data_File(f);
}

class AnimatedSoundMgrClass
{
public:
	static void Shutdown();
	static void Initialize(char const *);
};

RENEGADE_FUNCTION
void AnimatedSoundMgrClass::Initialize(char const *)
AT2(0x00579FA0,0x00579840);

RENEGADE_FUNCTION
void Set_Current_Map_Name(const char *)
AT2(0x006DA920,0x006DA1C0);



RENEGADE_FUNCTION
void SaveGameManager::Pre_Load_Game(const char* rawMapName, StringClass& lddName, StringClass& lsdName)
AT2(0x006A37E0, 0x006A3080);
/*
{
	if (FileFactoryListClass::Instance)
		FileFactoryListClass::Instance->searchStart = 0;

	char extension[_MAX_EXT];
	char mapBaseName[_MAX_ NAME];
	_splitpath(rawMapName, 0, 0, &mapBaseName, &extension);

	SystemInfoLog::Set_Current_Level(&mapBaseName);

	if (_strcmpi(extension, ".mix") == 0)
	{
		lddName.Format("%s.ldd", &mapBaseName);
		lsdName.Format("%s.lsd", &mapBaseName);

		StringClass thuName;
		thuName.Format("%s.thu", &mapBaseName);
		ThumbnailManagerClass::Add_Thumbnail_Manager(thuName, rawMapName);

		if (lstrcmpi(mapBaseName, "M09") == 0)
			if (FileFactoryListClass::Instance)
				FileFactoryListClass::Instance.Set_Search_Start(rawMapName);
	}
	else if (_strcmpi(&extension, ".lsd") == 0)
	{
		lsdName = rawMapName;
		lddName.Format("%s.ldd", mapBaseName);
	}
	else
	{
		lddName = rawMapName;

		StringClass internalRawMapName;
		if (SaveGameManager::Peek_Map_Name(rawMapName, internalRawMapName))
		{
			_splitpath(FullPath, NULL, NULL, &mapBaseName, NULL);

			StringClass mixName;
			mixName.Format("%s.mix", &mapBaseName);
			lsdName.Format("%s.lsd", &mapBaseName);
			lddName = rawMapName;

			if (lstrcmpi(mapBaseName, "M09") == 0)
				if (FileFactoryListClass::Instance)
					FileFactoryListClass::Instance.Set_Search_Start(mixName);

			ThumbnailManagerClass::Add_Thumbnail_Manager(thuName, mixName);
		}
	}
}
*/


void Level_Loader_Thread()
{
	CombatManager::Set_Load_Progress(0);
	GenericDataSafeClass::Set_Preferred_Thread(GetCurrentThreadId());
	CombatManager::Inc_Load_Progress();
	DefinitionMgrClass::Free_Definitions();
	FileClass *fx = Get_Data_File(SaveGameManager::DefaultDefinitionFilename);
	if ((fx) && (fx->Is_Available()))
	{
		unsigned int size = fx->Size();
		char *data = new char[size];
		fx->Open(1);
		fx->Read(data,size);
		CheckMap(data,size,(char *)SaveGameManager::DefaultDefinitionFilename,HashObjectsDdb);
		delete[] data;
		fx->Close();
		Close_Data_File(fx);
	}
	SaveGameManager::Load_Definitions(SaveGameManager::DefaultDefinitionFilename);
	CombatManager::Inc_Load_Progress();
	AnimatedSoundMgrClass::Initialize(0);
	StringClass mapldd = _load_map_name;
	StringClass maplsd = _load_map_name;
	Set_Current_Map_Name(_load_map_name);
	SaveGameManager::Pre_Load_Game(_load_map_name,mapldd,maplsd);
	CombatManager::Set_Last_LSD_Name(maplsd);
	CombatManager::Inc_Load_Progress();
	CombatManager::Inc_Load_Progress();
	CombatManager::Inc_Load_Progress();
	FileClass *f = Get_Data_File(mapldd);
	if ((f) && (f->Is_Available()))
	{
		unsigned int size = f->Size();
		char *data = new char[size];
		f->Open(1);
		f->Read(data,size);
		CheckMap(data,size,mapldd.Peek_Buffer(),HashMapLdd);
		delete[] data;
		f->Close();
		Close_Data_File(f);
	}
	SaveGameManager::Load_Game(mapldd);
	CombatManager::Inc_Load_Progress();
}




RENEGADE_FUNCTION
void SaveGameManager::Save_Game(const char*, ...)
AT2(0x006A35C0, 0x006A2E60);



RENEGADE_FUNCTION
UNK SaveGameManager::Smart_Peek_Description(const char*, WideStringClass&, WideStringClass&)
AT2(0x006A3F30, 0x006A37D0);



RENEGADE_FUNCTION
UNK SaveGameManager::Peek_Description(const char*, WideStringClass&, WideStringClass&)
AT2(0x006A40C0, 0x006A3960);

REF_DEF2(_TheMapMgrSaveLoadSubsystem,SaveLoadSubSystemClass,0x0085E2C8,0x0085D4A0);
REF_DEF2(_StaticAudioSaveLoadSubsystem,SaveLoadSubSystemClass,0x0089DCC0,0x0089CEA8);
REF_DEF2(_PhysStaticObjectsSaveSystem,SaveLoadSubSystemClass,0x0089DB60,0x0089CD48);
REF_DEF2(_PhysStaticDataSaveSystem,SaveLoadSubSystemClass,0x0089DB70,0x0089CD58);
extern WeatherMgrClass *_TheWeatherMgr;
extern BackgroundMgrClass *_TheBackgroundMgr;
void SaveGameManager::Save_Level()
{
	SaveGameManager::Save_Save_Load_System(SaveGameManager::MapFilename,&_PhysStaticDataSaveSystem,&_PhysStaticObjectsSaveSystem,&_StaticAudioSaveLoadSubsystem,_TheBackgroundMgr,_TheWeatherMgr,&_TheMapMgrSaveLoadSubsystem,0);
}

RENEGADE_FUNCTION
UNK SaveGameManager::Save_Definitions(const char*)
AT2(0x006A4440, 0x006A3CE0);



RENEGADE_FUNCTION
void SaveGameManager::Save_Save_Load_System(const char*, ...)
AT2(0x006A44C0, 0x006A3D60);
