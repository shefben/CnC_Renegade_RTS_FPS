#ifndef TT_INCLUDE__SYSTEMSETTINGENTRYENUM_H
#define TT_INCLUDE__SYSTEMSETTINGENTRYENUM_H



#include "SystemSettingEntry.h"



class __declspec(novtable) SystemSettingEntryEnum :
	public SystemSettingEntry
{

private:

	int appliedValue;

public:

	SystemSettingEntryEnum() : appliedValue(0) {}
	virtual ~SystemSettingEntryEnum() {}
	virtual const char* Get_Name() = 0;
	virtual const char* Get_Help() = 0;
	virtual void Apply() { appliedValue = Get_Enum(); }
	virtual void Registry_Save(RegistryClass& key);
	virtual void Registry_Load(RegistryClass& key);
	virtual ConsoleFunctionClass* Create_Console_Function();
	virtual int Get_Enum() = 0;
	virtual void Set_Enum(int value) = 0;
	virtual int Get_Enum_Count() = 0;
	virtual const char* Get_Enum_Name(int value) = 0;

};



#endif