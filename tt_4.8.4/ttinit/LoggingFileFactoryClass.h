#ifndef TT_INCLUDE_LOGGINGFILEFACTORYCLASS_H
#define TT_INCLUDE_LOGGINGFILEFACTORYCLASS_H
#include "SimpleFileFactoryClass.h"
class LoggingFileFactoryClass : public SimpleFileFactoryClass {
public:
	FileFactoryClass *basefactory;
	virtual FileClass* Get_File(const char* fileName);
};

#endif