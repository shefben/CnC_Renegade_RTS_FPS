#ifndef _FILECACHE_H_
#define _FILECACHE_H_
#include "engine_vector.h"

struct FileCacheHeader
{
	unsigned int Identifier;
	unsigned int Flags;
};

struct FileCacheEntry
{
	char *Name;
	unsigned char *Data;
	size_t Size;
};

class __declspec(novtable) FileCache: public RefCountClass
{
protected:
	FileCache();
public:
	//public FileCacheEntry
};
#endif
