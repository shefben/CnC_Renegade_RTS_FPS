#ifndef TT_INCLUDE__SYSTEMSETTINGENTRYBOOL_H
#define TT_INCLUDE__SYSTEMSETTINGENTRYBOOL_H



#include "SystemSettingEntry.h"



class __declspec(novtable) SystemSettingEntryBool :
	public SystemSettingEntry
{

private:

	bool appliedValue;

public:

	SystemSettingEntryBool() : appliedValue(false) {}
	virtual ~SystemSettingEntryBool();
	virtual void Apply() { appliedValue = Get_Bool() != 0; }
	virtual void Registry_Save(RegistryClass& key);
	virtual void Registry_Load(RegistryClass& key);
	virtual ConsoleFunctionClass* Create_Console_Function();
	virtual int Get_Bool() = 0;
	virtual void Set_Bool(int value) = 0;
	
	bool Get_State() { return appliedValue; }
	void Set_State(bool _appliedValue);

};



#endif