#include "General.h"
#include "cRegistryBool.h"
#include "RegistryClass.h"

cRegistryBool::cRegistryBool(const char *key, const char *value,bool data)
{
	bool newdata;
	if (key)
	{
		strcpy(Key,key);
		strcpy(Value,value);
		RegistryClass reg(Key,true);
		Data = reg.Get_Bool(Value,data);
		newdata = Data;
	}
	else
	{
		Key[0] = 0;
		Value[0] = 0;
		newdata = data;
	}
	Set(newdata);
}

void cRegistryBool::Set(bool data)
{
	Data = data;
	if (Key[0])
	{
		RegistryClass reg(Key,true);
		reg.Set_Bool(Value,Data);
	}
}

void cRegistryBool::Toggle()
{
	Set(Data == 0);
}
