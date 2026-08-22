#include "General.h"
#include "StrippingFileFactoryClass.h"

FileClass* StrippingFileFactoryClass::Get_File(const char* fileName)
{
	StringClass string;
	Strip_Path_From_Filename(string,fileName);
	return basefactory->Get_File(string);
}
