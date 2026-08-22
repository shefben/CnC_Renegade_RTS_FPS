#ifndef TT_INCLUDE_STRIPPINGFILEFACTORYCLASS_H
#define TT_INCLUDE_STRIPPINGINGFILEFACTORYCLASS_H
#include "SimpleFileFactoryClass.h"
class StrippingFileFactoryClass : public SimpleFileFactoryClass {
public:
	FileFactoryClass *basefactory;
	virtual FileClass* Get_File(const char* fileName);
};

#endif
