#include "General.h"
#include "SystemSettings.h"

#include "RegistryClass.h"
#include "SystemSettingEntry.h"
#include "SystemSettingEntryTextureFilterMode.h"
#include "engine_vector.h"



REF_DEF2(SystemSettings::SettingList, DynamicVectorClass<SystemSettingEntry*>, 0x0081DC20, 0x0081CE00);

RENEGADE_FUNCTION
void SystemSettings::Init()
AT1(0x0041B020);


RENEGADE_FUNCTION
void SystemSettings::Apply_All()
AT2(0x0041A870, 0x0041A870);



void SystemSettings::Add_Console_Functions(DynamicVectorClass<ConsoleFunctionClass*>& consoleFunctions)
{
#ifdef DEBUG
	for (int i = 0; i < SettingList.Count(); i++)
		consoleFunctions.Add(SettingList[i]->Create_Console_Function());
#endif
}



void SystemSettings::Registry_Load(const char* registryKeyName)
{
	for (int i = 0; i < SettingList.Count(); i++)
	{
		const char *str = SettingList[i]->Get_Name();
		if (strcmp(str, "Texture_Filter_Mode") == 0)
		{
			delete SettingList[i];
			SettingList[i] = new SystemSettingEntryTextureFilterMode();
		}
		if (strcmp(str, "Static_LOD_Budget") == 0)
		{
			delete SettingList[i];
			SettingList.Delete(i);
			i--;
		}
		if (strcmp(str, "Dynamic_LOD_Budget") == 0)
		{
			delete SettingList[i];
			SettingList.Delete(i);
			i--;
		}
	}

	RegistryClass key(registryKeyName);

	for (int i = 0; i < SettingList.Count(); i++)
		SettingList[i]->Registry_Load(key);
}
