#include "General.h"
#include "LoggingFileFactoryClass.h"

FileClass* LoggingFileFactoryClass::Get_File(const char* fileName)
{
	return basefactory->Get_File(fileName);
}
