#include "General.h"
#include "cRegistryInt.h"
#include "RegistryClass.h"

cRegistryInt::cRegistryInt(const char *key, const char *value,int data)
{
	int newdata;
	if (key)
	{
		strcpy(Key,key);
		strcpy(Value,value);
		RegistryClass reg(Key,true);
		Data = reg.Get_Int(Value,data);
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

void cRegistryInt::Set(int data)
{
	Data = data;
	if (Key[0])
	{
		RegistryClass reg(Key,true);
		reg.Set_Int(Value,Data);
	}
}
