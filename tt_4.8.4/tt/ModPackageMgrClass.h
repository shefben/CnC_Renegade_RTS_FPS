#ifndef TT_INCLUDE__MODPACKAGEMGRCLASS_H
#define TT_INCLUDE__MODPACKAGEMGRCLASS_H



#include "engine_string.h"



class ModPackageClass;
template<typename T> class DynamicVectorClass;



class ModPackageMgrClass
{
	
	static REF_DECL2(PackageList, DynamicVectorClass<ModPackageClass>);
	static REF_DECL2(CurrentPackage, ModPackageClass);

public:

	static void Initialize();
	static void Shutdown();
	static void Build_List();
	static void Reset_List();
	static void Set_Current_Package(const char* packageName);
	static void Set_Current_Package(int index);
	static ModPackageClass* Find_Package(const char* packageName);
	static bool Get_Mod_Map_Name_From_CRC_Index(uint32 crc, int mapIndex, StringClass* modName, StringClass* mapName);
	static bool Get_Mod_Map_Name_From_CRC(uint32 modCrc, uint32 mapCrc, StringClass* modName, StringClass* mapName);
	static bool Find_Filename_From_CRC(const char* mapNameMask, uint32 mapCrc, StringClass* mapName);
	static bool Find_Package_From_CRC(uint32 modCrc, StringClass* modName);
	static void Load_Current_Mod();
	static void Unload_Current_Mod();

};



#endif