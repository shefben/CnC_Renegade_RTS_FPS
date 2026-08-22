#include "General.h"
#include "engine_string.h"
#include "HashMixFileFactoryClass.h"
#include "engine_io.h"
#include "modpackageclass.h"
#include "engine2.h"
#include "SimpleFileFactoryClass.h"

unsigned long CRC_Stringi(char  const*, unsigned long = 0); // TODO: Move



ModPackageClass::ModPackageClass() :
	crc(0)
{
}



ModPackageClass::~ModPackageClass()
{
}



void ModPackageClass::Set_Package_Filename(const char* _fileName)
{
	fileName = _fileName;
	baseName = _fileName;
	
	char* baseNameEnd = strrchr(baseName.Peek_Buffer(), '.');
	if (baseNameEnd)
		*baseNameEnd = '\0';
}



void ModPackageClass::Build_Level_List(DynamicVectorClass<StringClass>& levelList)
{
	HashMixFileFactoryClass fileFactory(fileName, RenegadeBaseFileFactory);
	DynamicVectorClass<StringClass> fileNameList;
	fileNameList.Set_Growth_Step(1000);
	fileFactory.Build_Filename_List(fileNameList);
	for (int index = 0; index < fileNameList.Count(); index++)
	{
		StringClass string = fileNameList[index];
		_strlwr(string.Peek_Buffer());
		if (strstr(string.Peek_Buffer(), ".lsd"))
			levelList.Add(string);
	}
}



bool ModPackageClass::Find_Map_From_CRC(uint32 mapCrc, StringClass* mapName)
{
	DynamicVectorClass<StringClass> levelList;
	Build_Level_List(levelList);
	
	for (int index = 0; index < levelList.Count(); ++index)
	{
		if (CRC_Stringi(levelList[index]) == mapCrc)
		{
			*mapName = levelList[index];
			return true;
		}
	}
	
	return false;
}



void ModPackageClass::Compute_CRC()
{
	crc = 0;

	FileClass* file = _TheFileFactory->Get_File(fileName);
	if (file)
	{
		file->Open();
		if(file->Is_Open())
		{

			file->Seek(12, 0);
			if(file->Read(&crc, 4) != 4 || crc == 0)
			{	
				file->Seek(0, 0);
				byte buffer[0x1000];
				int bufferLength;
				while ((bufferLength = file->Read(buffer, 0x1000)) > 0)
					crc = CRC_Memory(buffer, bufferLength, crc);
				file->Close();
				FILE* handle = fopen(file->File_Name(), "r+");
				if(handle)
				{
					fseek(handle, 12, SEEK_SET);
					fwrite(&crc, 4, 1, handle);
					fclose(handle);
				}
			}
			
			file->Close();
		}
		_TheFileFactory->Return_File(file);
	}
}



uint32 ModPackageClass::Get_CRC()
{
	if (!crc)
		Compute_CRC();

	return crc;
}



int ModPackageClass::Get_Map_Index(const char* mapName)
{
	DynamicVectorClass<StringClass> levelList;
	Build_Level_List(levelList);
	for (int index = 0; index < levelList.Count(); ++index)
		if (levelList[index].Compare_No_Case(mapName) == 0)
			return index;
	
	return 0;
}
