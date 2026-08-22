#ifndef TT_INCLUDE__FILEHASH_H
#define TT_INCLUDE__FILEHASH_H

enum GlobalFileType {
	HashMix, //mix file
	HashGameExe, //game.exe
	HashBandTest, //bandtest.dll
	HashBinkw32, //binkw32.dll
	HashMemoryManager, //memorymanager.dll
	HashMss32, //mss32.dll
	HashPatchw32, //patchw32.dll
	HashScripts2, //scripts2.dll
	HashScripts, //scripts.dll
	HashShaders, //shaders.dll
	HashTT, //tt.dll
	HashWolAPI, //wolapi.dll
	HashWolBrowser, //wolbrowser.dll
	HashTTIni, //tt.ini
	HashScopesCfg, //scopes.cfg
	HashCustomScope, //custom scope texture
	HashStealth, //stealth texture
	HashScope, //default scope texture
	HashShaderDatabase, //shaders.sdb
	HashSceneShaderDatabase, //sceneshaders.sdb
	HashFXFile, //fx file (compiled or source)
	HashW3dFile, //loose w3d file comming from folder or from unsigned/unverified mix file
	HashCamerasIni, //cameras.ini
	HashSurfaceEffectsIni, //surfaceeffects.ini
	MaxGlobalHash
};

enum MapFileType {
	HashObjectsDdb, //objects.ddb
	HashMapDdb, //<mapname>.ddb
	HashMapLsd, //<mapname>.lsd
	HashMapLdd, //<mapname>.ldd
	HashMapShaderDatabase, //<mapname>.sdb
	HashMapSM1FXCache, //<mapname>_sm1.fxc
	HashMapSM2FXCache, //<mapname>_sm2.fxc
	HashMapSM3FXCache, //<mapname>_sm3.fxc
	MaxMapHash
};

typedef void (*GlobalCheck) (char* data, int dataLength, char* _fileName, GlobalFileType _type);
typedef void (*MapCheck) (char* data, int dataLength, char* _fileName, MapFileType _type);
typedef void (*LoadCheck) (char* data, int dataLength, char* _fileName, GlobalFileType _type);
#ifdef SHADERS_EXPORTS
extern GlobalCheck CheckGlobal;
extern MapCheck CheckMap;
extern LoadCheck CheckLoad;
#endif
#endif