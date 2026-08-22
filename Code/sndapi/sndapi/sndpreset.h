#ifndef _SNDPRESET_H_
#define _SNDPRESET_H_
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "WideStringClass.h"
#include "VectorAndListTemplateclasses.h"
#include "Chunkclasses.h"
#include "Fileclass.h"
#include "mixfile.h"
#include "mmsystem.h"
#include "Interfaces.h"

#define SND_CID 0x1337

#define THREAD_PLAY_FILENAME 0xF00D
#define THREAD_PLAY_FILE 0xBAD
#define THREAD_PLAY_STOP 0xBEEF
#define THREAD_EXIT_MSG 0xC0DE


class SoundObject : public ISoundObject{
private:
	StringClass m_Filename;
	StringClass m_Preset_Name;
	unsigned long m_Preset_ID;
	bool m_Is_3D_Sound;
	void ODB_Subload(ChunkLoadClass &load);
public:
	SoundObject();
	virtual void Load(ChunkLoadClass &load);
	virtual void Save(ChunkSaveClass &save);
	virtual void Load_From_ODB(ChunkLoadClass &load);
	virtual unsigned long Get_ID();
	virtual char *Get_Filename();
	virtual char *Get_Presetname();
	virtual bool Is_3D_Sound();

	virtual void Set_ID(unsigned long ID);
	virtual void Set_Filename(char *name);
	virtual void Set_Prestname(char *name);
};

class SoundDB{
public:
	static int Get_Version();
	static DynamicVectorClass<SoundObject*> m_Objects;
	static DynamicVectorClass<cMIXFileRead *>m_Mounts;
	static bool Load_From_ODB(char *fname);
	static bool Load_From_SoundDB(char *fname);
	static bool Save_To_SoundDB(char *fname);
	static void Clear_Mounts();
	static void Clear_Objects();
	/*-- new --*/
	static ISoundObject *Find_Sound(unsigned long sound);
	static ISoundObject *Get_Sound(int idx);
	static bool Mount_MIX(char *filename);
	static FileClass *Get_Sound_File(char *filename);
	static void Play_Sound(char *filename);
	static void Play_Sound_Class(FileClass *file);
	static void Play_Sound_Object(ISoundObject *obj);
	static void Play_Sound_PresetID(unsigned long ID);
	static void Stop_Sound();
	static bool Do_Options(HKEY base_key);
	static bool Do_Convert();
	static void Apply_Options(HKEY base_key);
	~SoundDB();
};

class CSoundDatabase : public ISoundDatabase {
public:
	virtual int Get_Version();
	virtual ISoundObject *Find_Sound(unsigned long preset_ID);
	virtual ISoundObject *Get_Sound(int idx);
	virtual void Clear_Mounts();
	virtual void Clear_Objects();
	virtual bool Save_To_SoundDB(char *filename);
	virtual bool Load_From_SoundDB(char *filename);
	virtual bool Load_From_ODB(char *filename);
	virtual bool Mount_MIX(char *filename);	
	virtual void Play_Sound(char *filename);
	virtual void Play_Sound_Class(FileClass *file);
	virtual void Play_Sound_Object(ISoundObject *obj);
	virtual void Play_Sound_PresetID(unsigned long ID);
	virtual void Stop_Sound();
	virtual bool Do_Options(HKEY base_key);
	virtual bool Do_Convert();
	virtual void Apply_Options(HKEY base_key);
};

ISoundDatabase *Get_IF();

#endif //_SNDPRESET_H_