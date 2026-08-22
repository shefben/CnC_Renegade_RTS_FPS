#include "General.h"
#include "SimpleFileFactoryClass.h"
#include "SignBufferedFileClass.h"

RENEGADE_FUNCTION
void SimpleFileFactoryClass::Set_Sub_Directory
   (const char*)
   AT2(0x005E3E50,0x005E36F0);

RENEGADE_FUNCTION
void SimpleFileFactoryClass::Append_Sub_Directory
   (const char*)
   AT2(0x005E4090,0x005E3930);

SimpleFileFactoryClass::SimpleFileFactoryClass()
   : IsStripPath(false)
{
}

bool Is_Full_Path(const char *filePath)
{
	if (!filePath)
		return false;
	return filePath[1] == ':' || (filePath[0] == '\\' && filePath[1] == '\\');
}

FileClass* SimpleFileFactoryClass::Get_File(const char* filePath)
{
	StringClass fileName(248, true);
	StringClass fileName2(248, true);
	if (IsStripPath)
	{
		const char* fileNameSeparator = strrchr(filePath, '/');
		if (fileNameSeparator)
			fileName = fileNameSeparator + 1;
		else
			fileName = filePath;
	}
	else
		fileName = filePath;
	
	
	BufferedFileClass *file = new SignBufferedFileClass;
	if (!Is_Full_Path(fileName))
	{
		CriticalSectionClass::LockClass lock(Mutex);
		if (!SubDirectory.Is_Empty())
		{
			if (strchr(SubDirectory, ';'))
			{
				StringClass SubDir = SubDirectory;
				for (const char* token = strtok(SubDir.Peek_Buffer(), ";"); token; token = strtok(NULL, ";"))
				{
					fileName2.Format("%s%s", token, fileName);
					file->Set_Name(fileName2);
					if (file->Open(1))
					{
						file->Close();
						break;
					}
				}
			}
			else
				fileName2.Format("%s%s", SubDirectory, fileName);
		}
	}
	file->Set_Name(fileName2);
	return file;
}

void SimpleFileFactoryClass::Return_File(FileClass* file)
{
	delete file;
}
