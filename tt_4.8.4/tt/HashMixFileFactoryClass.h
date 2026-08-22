#ifndef TT_INCLUDE__HASHMIXFILEFACTORYCLASS_H
#define TT_INCLUDE__HASHMIXFILEFACTORYCLASS_H
#include "MixFileFactoryClass.h"
class HashMixFileFactoryClass : public MixFileFactoryClass
{
protected:
	bool IsSigned;
public:
	HashMixFileFactoryClass(const char*, SimpleFileFactoryClass&);
	virtual ~HashMixFileFactoryClass();
	virtual FileClass* Get_File(const char* fileName);
	virtual void Return_File(FileClass* file);
};

#endif