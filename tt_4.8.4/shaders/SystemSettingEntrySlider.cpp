#include "General.h"
#include "SystemSettingEntrySlider.h"

#include "RegistryClass.h"
#include "engine_math.h"



SystemSettingEntrySlider::SystemSettingEntrySlider() :
	appliedValue(0),
	minValue(0),
	maxValue(10),
	stepSize(1)
{
}



void SystemSettingEntrySlider::Registry_Save(RegistryClass& key)
{
	key.Set_Int(Get_Name(), Get_Value());
}



void SystemSettingEntrySlider::Registry_Load(RegistryClass& key)
{
	Set_Value(key.Get_Int(Get_Name(), Get_Value()));
}



RENEGADE_FUNCTION
ConsoleFunctionClass* SystemSettingEntrySlider::Create_Console_Function()
AT2(0x0041AC00, 0x0041AC00);



void SystemSettingEntrySlider::Set_Value(int _value)
{
	appliedValue = clamp(_value, minValue, maxValue);
	Set_Slider(appliedValue);
	appliedValue = Get_Slider();
}
