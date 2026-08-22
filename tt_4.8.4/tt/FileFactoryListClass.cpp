#include "general.h"

#include "FileFactoryListClass.h"

FileClass* FileFactoryListClass::Get_File
   (const char* fileName)
{
   FileClass* file = 0;
   if (this->tempFactory)
   {
      file = this->tempFactory->Get_File (fileName);
      if (file)
      {
         if (file->Is_Available (0))
            return file;

         this->tempFactory->Return_File (file);
      }
   }

   for (sint32 u = 0; u < this->factories.Count(); ++u)
   {
      FileFactoryClass* factory = this->factories[u];

      file = factory->Get_File (fileName);
      if (!file)
         continue;

      if (file->Is_Available (0))
         return file;

      factory->Return_File (file);
   }

   if (this->factories.Count())
      return this->factories[0]->Get_File (fileName);

   return 0;
}

void FileFactoryListClass::Return_File
   (FileClass* file)
{
   this->factories[0]->Return_File (file);
}

void FileFactoryListClass::Add_FileFactory
   (FileFactoryClass* factory, const char* name)
{
	factories.Add(factory);
	fileNames.Add(name);
	searchStart = 0;
}

RENEGADE_FUNCTION
void FileFactoryListClass::Remove_FileFactory(FileFactoryClass*)
   AT2(0x006EC460,0x006EBD00);

REF_DEF2(FileFactoryListClass::Instance, FileFactoryListClass *, 0x0085DB70, 0x0085CD58);

FileFactoryClass *FileFactoryListClass::Remove_Temp_FileFactory()
{
	FileFactoryClass *f = tempFactory;
	tempFactory = 0;
	return f;
}
void FileFactoryListClass::Add_Temp_FileFactory(FileFactoryClass *factory)
{
	tempFactory = factory;
}



void FileFactoryListClass::Set_Search_Start(const char* factoryName)
{
	for (int factoryIndex = 0; factoryIndex < fileNames.Length(); ++factoryIndex)
	{
		if (_stricmp(fileNames[factoryIndex], factoryName) == 0)
		{
			searchStart = factoryIndex;
			return;
		}
	}
	
	searchStart = 0;
}
