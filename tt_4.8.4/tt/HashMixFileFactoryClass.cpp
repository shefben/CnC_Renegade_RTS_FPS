#include "general.h"

#include "HashMixFileFactoryClass.h"
#include "SimpleFileFactoryClass.h"
#include "SignBufferedFileClass.h"
#include "FileHashEvent.h"
HashMixFileFactoryClass::HashMixFileFactoryClass
(const char* filePath, SimpleFileFactoryClass& parentFileFactory) : MixFileFactoryClass(filePath,parentFileFactory), IsSigned(true)
{
	//FileHashEvent *e = new FileHashEvent(); //signature TODO
	//e->InitSigned(filepath,HashMix,true); //signature TODO
	//signature check init here, set IsSigned
	IsSigned = true;
}

HashMixFileFactoryClass::~HashMixFileFactoryClass()
{
	//signature check free here
}
unsigned long CRC_Stringi(char  const*, unsigned long);
FileClass* HashMixFileFactoryClass::Get_File(const char* fileName)
{
	if (FileInformation.Count())
	{
		unsigned long crc = CRC_Stringi(fileName,0);
		int i;
		bool match = false;
		for (i = 0;i < FileInformation.Count();i++)
		{
			if (FileInformation[i].id == crc)
			{
				match = true;
				break;
			}
		}
		if (!match)
		{
			return NULL;
		}
		FileClass *f = Factory->Get_File(MixFilename);
		if (f)
		{
			((SignBufferedFileClass *)f)->IsSigned = IsSigned;
			f->Bias(FileInformation[i].offset + UnkOffset,FileInformation[i].size);
		}
		return f;
	}
	return NULL;
}
void HashMixFileFactoryClass::Return_File(FileClass* file)
{
	if (file)
	{
		Factory->Return_File(file);
	}
	//signature check stuff here if needed
}
