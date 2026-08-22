#ifndef TT_INCLUDE__FILEFACTORYLISTCLASS_H
#define TT_INCLUDE__FILEFACTORYLISTCLASS_H
#include "engine_io.h"
#include "engine_string.h"
extern char* lastFileName;

class FileFactoryListClass : public FileFactoryClass
{

	static REF_DECL2(Instance, FileFactoryListClass *);

	FileFactoryClass* tempFactory; // 0004
	SimpleDynVecClass<FileFactoryClass*> factories; // 0008
	DynamicVectorClass<StringClass> fileNames; // 0018
	int searchStart; // 0030

public:

	static FileFactoryListClass* Get_Instance() { return Instance; }

	virtual ~FileFactoryListClass();
	virtual FileClass* Get_File(const char* fileName);
	virtual void Return_File(FileClass* file);
	
	FileFactoryListClass();
	void Add_FileFactory(FileFactoryClass* factory, const char* factoryName);
	void Remove_FileFactory(FileFactoryClass* factory);
	void Remove_FileFactory();
	void Add_Temp_FileFactory(FileFactoryClass* factory);
	FileFactoryClass *Remove_Temp_FileFactory();
	void Set_Search_Start(const char* factoryName);
	void Reset_Search_Start() { searchStart = 0; }
	FileFactoryClass *Peek_Temp_FileFactory();
	int Get_Factory_Count();
	FileFactoryClass *Get_Factory(int);

}; // 0034


#endif
