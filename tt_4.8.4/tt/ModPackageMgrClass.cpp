#include "General.h"
#include "ModPackageMgrClass.h"

#include "ModPackageClass.h"
#include "FileFactoryListClass.h"
#include "cGameType.h"
#include "HashMixFileFactoryClass.h"
#include "RegistryClass.h"
#include "_globals.h"
#include "SimpleFileFactoryClass.h"

unsigned long CRC_Stringi(char  const*, unsigned long = 0); // TODO: Move



const char* CURR_MOD_REG_VALUE = "CurrModPackage";

REF_DEF2(ModPackageMgrClass::CurrentPackage, ModPackageClass, 0x0082FDC0, 0x0082EFA8);
REF_DEF2(ModPackageMgrClass::PackageList, DynamicVectorClass<ModPackageClass>, 0x0082FDD0, 0x0082EFB8);
extern REF_DECL2(RenegadeBaseFileFactory, SimpleFileFactoryClass);

void ModPackageMgrClass::Initialize()
{
	Reset_List();
}



void ModPackageMgrClass::Shutdown()
{
	Reset_List();
}



void ModPackageMgrClass::Build_List()
{
	WIN32_FIND_DATA findData;
	HANDLE findHandle = FindFirstFile("data\\*.pkg", &findData);
	
	if (findHandle != INVALID_HANDLE_VALUE)
	{
		do
		{
			ModPackageClass modPackage;
			modPackage.Set_Package_Filename(findData.cFileName);
			PackageList.Add(modPackage);
		} while (FindNextFile(findHandle, &findData));
		FindClose(findHandle);
	}
}



void ModPackageMgrClass::Reset_List()
{
	PackageList.Delete_All();
}



void ModPackageMgrClass::Set_Current_Package(const char* packageName)
{
	CurrentPackage.Set_Package_Filename(packageName);
	RegistryClass registry(Build_Registry_Location_String((char *)0x007F5274, 0, "Options"), true);
	if (registry.Is_Valid())
		registry.Set_String(CURR_MOD_REG_VALUE, packageName);
}



void ModPackageMgrClass::Set_Current_Package(int index)
{
	TT_ASSERT((uint)index < (uint)PackageList.Count());
	Set_Current_Package(PackageList[index].Get_Package_Filename());
}



ModPackageClass* ModPackageMgrClass::Find_Package(const char* packageName)
{
	for (int i = 0; i < PackageList.Count(); ++i)
		if (PackageList[i].Get_Package_Filename().Compare_No_Case(packageName) == 0)
			return &PackageList[i];
	
	return NULL;
}



bool ModPackageMgrClass::Get_Mod_Map_Name_From_CRC_Index(uint32 modCrc, int mapIndex, StringClass* modName, StringClass* mapName)
{
	if (Find_Package_From_CRC(modCrc, modName))
	{
		ModPackageClass* modPackage = Find_Package(*modName);
		if (modPackage)
		{
          DynamicVectorClass<StringClass> levelList;
          modPackage->Build_Level_List(levelList);
          if ((uint)mapIndex < (uint)levelList.Count())
          {
              *mapName = levelList[mapIndex];
              return true;
          }
      }
  }
  return false;
}




bool ModPackageMgrClass::Get_Mod_Map_Name_From_CRC(uint32 modCrc, uint32 mapCrc, StringClass* modName, StringClass* mapName)
{
	if (modCrc)
	{
		
		if (Find_Filename_From_CRC("*.pkg", modCrc, modName))
		{
			ModPackageClass* modPackage = Find_Package(*modName);
			if (modPackage)
				return modPackage->Find_Map_From_CRC(mapCrc, mapName);
		}
		
		return false;
	}
	else
	{
		Find_Filename_From_CRC("*.mix", mapCrc, mapName);
		return true;
	}
}



bool ModPackageMgrClass::Find_Filename_From_CRC(const char* mapNameMask, uint32 mapCrc, StringClass* mapName)
{
	TT_ASSERT(mapNameMask);
	TT_ASSERT(mapName);
	
	bool result = false;

	StringClass findMask;
	findMask.Format("data\\%s", mapNameMask);
	
	WIN32_FIND_DATA findData = {0};
	HANDLE findHandle = FindFirstFile(findMask, &findData);
	if (findHandle != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (CRC_Stringi(findData.cFileName) == mapCrc)
			{
				*mapName = findData.cFileName;
				result = true;
				break;
			}
		} while (FindNextFile(findHandle, &findData));
		FindClose(findHandle);
	}
	return result;
}



bool ModPackageMgrClass::Find_Package_From_CRC(uint32 modCrc, StringClass* modName)
{
	TT_ASSERT(modName);

	for (int i = 0; i < PackageList.Count(); ++i)
	{
		if (PackageList[i].Get_CRC() == modCrc)
		{
			*modName = PackageList[i].Get_Package_Filename();
			return true;
		}
	}
	return false;
}



void ModPackageMgrClass::Load_Current_Mod()
{
	TT_ASSERT(FileFactoryListClass::Get_Instance());

	delete FileFactoryListClass::Get_Instance()->Remove_Temp_FileFactory();

	if (IS_MULTIPLAY && !CurrentPackage.Get_Package_Filename().Is_Empty())
		FileFactoryListClass::Get_Instance()->Add_Temp_FileFactory(new HashMixFileFactoryClass(CurrentPackage.Get_Package_Filename(),RenegadeBaseFileFactory));
}



void ModPackageMgrClass::Unload_Current_Mod()
{
	TT_ASSERT(FileFactoryListClass::Get_Instance());

	if (!CurrentPackage.Get_Package_Filename().Is_Empty())
		delete FileFactoryListClass::Get_Instance()->Remove_Temp_FileFactory();
}
