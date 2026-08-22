#ifndef TT_INCLUDE_SIMPLEFILEFACTORYCLASS_H
#define TT_INCLUDE_SIMPLEFILEFACTORYCLASS_H
#include "engine_io.h"
class SimpleFileFactoryClass : public FileFactoryClass {
public:
	StringClass SubDirectory;
	bool IsStripPath;
	CriticalSectionClass Mutex;
	SimpleFileFactoryClass();
	virtual ~SimpleFileFactoryClass() {};
	virtual FileClass* Get_File    (const char* cFile);
	virtual void       Return_File (FileClass* oFile);
	void Set_Sub_Directory(const char* sub_directory);
	void Append_Sub_Directory(const char* sub_directory);
};
#endif
