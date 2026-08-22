#include "General.h"
#include "cRegistryFloat.h"
#include "RegistryClass.h"

cRegistryFloat::cRegistryFloat(const char *key, const char *value,float data)
{
	float newdata;
	if (key)
	{
		strcpy(Key,key);
		strcpy(Value,value);
		RegistryClass reg(Key,true);
		Data = reg.Get_Float(Value,data);
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

void cRegistryFloat::Set(float data)
{
	Data = data;
	if (Key[0])
	{
		RegistryClass reg(Key,true);
		reg.Set_Float(Value,Data);
	}
}
