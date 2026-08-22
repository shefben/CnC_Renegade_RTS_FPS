#ifndef _INTERFACES_H_
#define _INTERFACES_H_

#include <windows.h>
#include "Chunkclasses.h"
#include "fileclass.h"

#define Make_Version(major, minor) ((major << 16) | (minor))
#define Get_Major(version)((version >> 16) & 0xFFFF)
#define Get_Minor(version)(version & 0xFFFF)

#define SNDAPI_VERSION Make_Version(1, 2);

class ISoundObject{
public:
	virtual void Load(ChunkLoadClass &load) = 0;
	virtual void Save(ChunkSaveClass &save) = 0;
	virtual void Load_From_ODB(ChunkLoadClass &load) = 0;
	virtual unsigned long Get_ID() = 0;
	virtual char *Get_Filename() = 0;
	virtual char *Get_Presetname() = 0;
	virtual bool Is_3D_Sound() = 0;

	virtual void Set_ID(unsigned long ID) = 0;
	virtual void Set_Filename(char *name) = 0;
	virtual void Set_Prestname(char *name) = 0;
};


class ISoundDatabase{
public:
	virtual int Get_Version() = 0;
	virtual ISoundObject *Find_Sound(unsigned long preset_ID) = 0;
	virtual ISoundObject *Get_Sound(int idx) = 0;
	virtual void Clear_Mounts() = 0;
	virtual void Clear_Objects() = 0;
	virtual bool Save_To_SoundDB(char *filename) = 0;
	virtual bool Load_From_SoundDB(char *filename) = 0;
	virtual bool Load_From_ODB(char *filename) = 0;
	virtual bool Mount_MIX(char *filename) = 0;
	virtual void Play_Sound(char *filename) = 0;
	virtual void Play_Sound_Class(FileClass *file) = 0;
	virtual void Play_Sound_Object(ISoundObject *obj) = 0;
	virtual void Play_Sound_PresetID(unsigned long ID) = 0;
	virtual void Stop_Sound() = 0;
	virtual bool Do_Options(HKEY base_key) = 0;
	virtual bool Do_Convert() = 0;
	virtual void Apply_Options(HKEY base_key) = 0;
};

typedef ISoundDatabase *(*pfIF)();

__inline ISoundDatabase *Query_Database(){
	HMODULE hModule;
	if (!(hModule = LoadLibraryA("sndapi.dll"))) return NULL;
	pfIF fIF;
	fIF = (pfIF)GetProcAddress(hModule, "Get_IF");
	if (!fIF){
		FreeLibrary(hModule);
		return NULL;
	}
	return fIF();
}

#endif //_INTERFACES_H_
