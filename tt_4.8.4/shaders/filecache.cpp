#include "general.h"

#include "filecache.h"
//#include "scripts.h"
//#include "engine_common.h"
//#include "engine_io.h"
//#include "engine_threading.h"
//#include "engine_string.h"

/*
FileCacheEntry::FileCacheEntry(char *name = NULL,unsigned char *data = NULL, size_t size = 0)
{
	Name = name;
	Data = data;
	Size = size;
};

FileCacheEntry::FileCacheEntry(const FileCacheEntry &efs)
{
	Name = newstr(efs.Name);
	Data = new unsigned char[efs.Size];
	memcpy(Data,efs.Data,efs.Size);
	Size = efs.Size;
};	

FileCacheEntry::~FileCacheEntry()
{
	if (Name)
	{
		delete[] Name;
		Name = 0;
	}
	if (Data)
	{
		delete[] Data;
		Data = 0;
	}
};

FileCacheEntry &FileCacheEntry::operator=(const FileCacheEntry &efs)
{
	if (Name)
	{
		delete[] Name;
	}
	Name = newstr(efs.Name);
	Size = efs.Size;
	if (Data)
	{
		delete[] Data;
	}
	Data = new unsigned char[Size];
	memcpy(Data,efs.Data,Size);
	return *this;
};

*/


