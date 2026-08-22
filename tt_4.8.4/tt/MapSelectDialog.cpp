#include "General.h"
#include "MapSelectDialog.h"

#include "engine_game.h"
#include "Iterator.h"
#include "ModPackageClass.h"
#include "tt.h"



void MapSelectDialog::buildDefaultMapList()
{
	MapList.Clear();
	
	StringClass fileMask;
	if (The_Game()->Is_Cnc())
		fileMask.Format("data\\%s*.mix", mapprefix);
	else
		fileMask = "data\\mp_*.mix";
	
	WIN32_FIND_DATA findData;
	HANDLE findhandle = FindFirstFile(fileMask, &findData);
	if (findhandle != INVALID_HANDLE_VALUE)
	{
		do
		{
			MapList.Add(findData.cFileName);
		} while (FindNextFile(findhandle, &findData));
		FindClose(findhandle);
	}
}



void MapSelectDialog::buildMapList(ModPackageClass* modPackage)
{
	if (modPackage)
	{
		MapList.Clear();
		
		DynamicVectorClass<StringClass> maps;
		modPackage->Build_Level_List(maps);
		
		for (Iterator<DynamicVectorClass<StringClass>> map(maps); map; ++map)
			MapList.Add(WideStringClass(*map));
	}
	else
		buildDefaultMapList();
}
