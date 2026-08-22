#ifndef TT_INCLUDE__SYSTEMSETTINGS_H
#define TT_INCLUDE__SYSTEMSETTINGS_H



class ConsoleFunctionClass;
class SystemSettingEntry;
template<class T> class DynamicVectorClass;



class SystemSettings
{

private:
	
	static REF_DECL2(SettingList, DynamicVectorClass<SystemSettingEntry*>);

public:

	SHADERS_API static void Apply_All();
	static void Registry_Save(const char* keyName);
	SHADERS_API static void Registry_Load(const char* keyName);
	SHADERS_API static void Add_Console_Functions(DynamicVectorClass<ConsoleFunctionClass*>&);
	SHADERS_API static void Init();
	static void Shutdown();
};



#endif