#include "General.h"
#include "SystemSettingEntryEnum.h"

#include "RegistryClass.h"



void SystemSettingEntryEnum::Registry_Save(RegistryClass& key)
{
	key.Set_Int(this->Get_Name(), this->Get_Enum()); 
}



void SystemSettingEntryEnum::Registry_Load(RegistryClass& key)
{
	this->Set_Enum(key.Get_Int(this->Get_Name(), this->Get_Enum()));
}



RENEGADE_FUNCTION
ConsoleFunctionClass* SystemSettingEntryEnum::Create_Console_Function()
AT2(0x0041ADC0, 0x0041ADC0);
