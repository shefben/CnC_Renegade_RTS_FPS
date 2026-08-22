#include "general.h"

#include "MixFileFactoryClass.h"
#include "SimpleFileFactoryClass.h"

struct mix_header
{
	int id;
	int index_offset;
	int tailer_offset;
	int zero;
};
RENEGADE_FUNCTION
unsigned long CRC_Stringi(char  const*, unsigned long)
AT2(0x005E6C50,0x005E64F0);
MixFileFactoryClass::MixFileFactoryClass
   (const char* filePath, SimpleFileFactoryClass& parentFileFactory) : FileFactoryClass(), Factory(0), UnkOffset(0), FileCount(0), MixFilenameOffset(0), IsValid(false), FileAdded(false)
{
	MixFilename = filePath;
	Factory = &parentFileFactory;
	Filenames.Set_Growth_Step(1000);
	FileClass *f = Factory->Get_File(filePath);
	if (f)
	{
		if (f->Is_Available())
		{
			f->Open(1);
			IsValid = true;
			mix_header header;
			IsValid = (f->Read(&header,sizeof(mix_header)) == sizeof(mix_header));
			if (IsValid)
			{
				if (header.id != 0x3158494D)
				{
					IsValid = false;
				}
			}
			FileCount = 0;
			if (IsValid)
			{
				f->Seek(header.index_offset,0);
				IsValid = (f->Read(&FileCount,4)) == 4;
				if (IsValid)
				{
					FileInformation.Resize(FileCount,0);
					FileInformation.Set_Active(FileCount);
					IsValid = (f->Read(&FileInformation[0],FileCount * sizeof(FileInfoStruct)) == (int)(FileCount * sizeof(FileInfoStruct)));
					if (IsValid)
					{
						UnkOffset = 0;
						MixFilenameOffset = header.tailer_offset;
						Factory->Return_File(f);
						return;
					}
				}
			}
			FileInformation.Resize(0,0);
		}
		Factory->Return_File(f);
	}
}

MixFileFactoryClass::~MixFileFactoryClass()
{
	FileInformation.Resize(0,0);
}

FileClass* MixFileFactoryClass::Get_File(const char* fileName)
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
			f->Bias(FileInformation[i].offset + UnkOffset,FileInformation[i].size);
		}
		return f;
	}
	return NULL;
}

void MixFileFactoryClass::Return_File(FileClass* file)
{
	if (file)
	{
		Factory->Return_File(file);
	}
}
void MixFileFactoryClass::Build_Filename_List(DynamicVectorClass<StringClass>&list)
{
	if (IsValid)
	{
		FileClass *f = Factory->Get_File(MixFilename);
		if (f->Open())
		{
			f->Seek(MixFilenameOffset,0);
			int count;
			if (f->Read(&count,4) == 4)
			{
				for (int i = 0;i < count;i++)
				{
					int size = 0;
					if (f->Read(&size,1) == 1)
					{
						StringClass name(size,false);
						if (f->Read(name.Peek_Buffer(),size) == size)
						{
							list.Add(name);
						}
					}
				}
			}
		}
		Factory->Return_File(f);
	}
}
