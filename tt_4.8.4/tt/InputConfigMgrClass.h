#ifndef TT_INCLUDE_INPUTCONFIGMGRCLASS_H
#define TT_INCLUDE_INPUTCONFIGMGRCLASS_H
#include "engine_vector.h"
#include "InputConfigClass.h"
#include "Engine_String.h"
class InputConfigMgrClass
{
public:
	static void Delete_Configuration(int index);
	static REF_DECL2(ConfigList,DynamicVectorClass<InputConfigClass>);
	static REF_DECL2(CurrentConfigIndex,int);
	static void Get_Config_Path(StringClass &);
	static void Load_Default_Configuration();
};

#endif