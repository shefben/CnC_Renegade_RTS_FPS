#ifndef TT_INCLUDE__SYSTEMSETTINGENTRY_H
#define TT_INCLUDE__SYSTEMSETTINGENTRY_H



class RegistryClass;
class ConsoleFunctionClass;



class __declspec(novtable) SystemSettingEntry
{

public:

	virtual ~SystemSettingEntry() {}
	virtual const char* Get_Name() = 0;
	virtual const char* Get_Help() = 0;
	virtual void Apply() = 0;
	virtual void Registry_Save(RegistryClass& key) = 0;
	virtual void Registry_Load(RegistryClass& key) = 0;
	virtual ConsoleFunctionClass* Create_Console_Function() = 0;

};



#endif