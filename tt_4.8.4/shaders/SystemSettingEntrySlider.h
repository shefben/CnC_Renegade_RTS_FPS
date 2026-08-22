#ifndef TT_INCLUDE__SYSTEMSETTINGENTRYSLIDER_H
#define TT_INCLUDE__SYSTEMSETTINGENTRYSLIDER_H



#include "SystemSettingEntry.h"

class RegistryClass;



class __declspec(novtable) SystemSettingEntrySlider :
	public SystemSettingEntry
{

private:

	int appliedValue; // 0004
	int minValue; // 0008
	int maxValue; // 000C
	int stepSize; // 0010

public:

	virtual ~SystemSettingEntrySlider() {}
	virtual void Apply() { appliedValue = Get_Slider(); }
	virtual void Registry_Save(RegistryClass& key);
	virtual void Registry_Load(RegistryClass& key);
	virtual ConsoleFunctionClass* Create_Console_Function();
	virtual int Get_Slider() = 0;
	virtual void Set_Slider(int _slider) = 0;

	SystemSettingEntrySlider();
	int Get_Value() { return appliedValue; }
	void Set_Value(int _value);

	int Get_Step_Size() { return stepSize; }
	void Set_Step_Size(int _stepSize) { stepSize = _stepSize; }
	void Set_Range(int _minValue, int _maxValue) { minValue = _minValue; maxValue = _maxValue; }

};



#endif