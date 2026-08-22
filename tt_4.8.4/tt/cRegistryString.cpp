#include "General.h"
#include "cRegistryString.h"
#include "RegistryClass.h"

cRegistryString::cRegistryString(const char *key, const char *value,const char *data)
{
	const char *newdata;
	if (key)
	{
		strcpy(Key,key);
		strcpy(Value,value);
		RegistryClass reg(Key,true);
		reg.Get_String(Value,Data,200,data);
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

void cRegistryString::Set(const char *data)
{
	strcpy(Data,data);
	if (Key[0])
	{
		RegistryClass reg(Key,true);
		reg.Set_String(Value,Data);
	}
}
