#include "General.h"
#include "SystemSettingEntryBool.h"



RENEGADE_FUNCTION
void SystemSettingEntryBool::Registry_Save(RegistryClass&)
AT2(0x0041A9D0, 0x0041A9D0);



RENEGADE_FUNCTION
void SystemSettingEntryBool::Registry_Load(RegistryClass&)
AT2(0x0041A9F0, 0x0041A9F0);



RENEGADE_FUNCTION
ConsoleFunctionClass* SystemSettingEntryBool::Create_Console_Function()
AT2(0x0041AA30, 0x0041AA30);



void SystemSettingEntryBool::Set_State(bool value)
{
	appliedValue = value;
	Set_Bool(value);
	Apply();
}
