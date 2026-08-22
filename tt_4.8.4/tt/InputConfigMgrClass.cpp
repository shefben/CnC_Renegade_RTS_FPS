#include "General.h"
#include "InputConfigMgrClass.h"

REF_DEF2(InputConfigMgrClass::ConfigList,DynamicVectorClass<InputConfigClass>,0x0081C708,0x0081B8E8);
REF_DEF2(InputConfigMgrClass::CurrentConfigIndex,int,0x007F546C,0x007F4470);
void InputConfigMgrClass::Delete_Configuration(int index)
{
	if (index && index < ConfigList.Count())
	{
		bool current = false;
		if (index == CurrentConfigIndex)
		{
			CurrentConfigIndex = -1;
			current = true;
		}
		StringClass str;
		Get_Config_Path(str);
		StringClass str2;
		str2.Format("%s\\%s",str,ConfigList[index].Filename);
		DeleteFile(str2);
		ConfigList.Delete(index);
		if (current)
		{
			InputConfigMgrClass::Load_Default_Configuration();
		}
	}
}

void InputConfigMgrClass::Get_Config_Path(StringClass &str)
{
	char buf[260];
	memset(buf,0,sizeof(buf));
	GetModuleFileName(0,buf,sizeof(buf));
	char *c = strrchr(buf,'\\');
	if (c)
	{
		c[0] = 0;
	}
	str.Format("%s\\data\\config",buf);
	StringClass str2;
	str2.Format("%s\\%s",(const char *)str);
}

RENEGADE_FUNCTION
void InputConfigMgrClass::Load_Default_Configuration()
AT2(0x00409B30,0x00409B30);
