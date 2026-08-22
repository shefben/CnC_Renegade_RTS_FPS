#include "General.h"
#include "RandomStringClass.h"



RandomStringClass::RandomStringClass() :
	random(0)
{
}



void RandomStringClass::Add_String(const char* string)
{
	strings.Add(string);
}



const StringClass* RandomStringClass::Get_String()
{
	if (strings.Count() == 0)
		return NULL;
	else
		return &strings[random() % strings.Count()];
}
