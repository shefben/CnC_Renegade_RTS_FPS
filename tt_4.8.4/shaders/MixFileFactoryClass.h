#ifndef TT_INCLUDE__MIXFILEFACTORYCLASS_H
#define TT_INCLUDE__MIXFILEFACTORYCLASS_H
#include "engine_common.h"
#include "engine_vector.h"
#include "engine_io.h"
#include "engine_string.h"
class SimpleFileFactoryClass;
class MixFileFactoryClass : public FileFactoryClass
{
public:
   struct FileInfoStruct
   {
	   unsigned int id;
	   unsigned int offset;
	   unsigned int size;
       bool operator== (const FileInfoStruct &src)
	   {
		   return false;
	   }
       bool operator!= (const FileInfoStruct &src)
	   {
		   return true;
	   }
   };
   struct AddInfoStruct
   {
	   const char *Filename1;
	   const char *Filename2;
       bool operator== (const AddInfoStruct &src)
	   {
		   return false;
	   }
       bool operator!= (const AddInfoStruct &src)
	   {
		   return true;
	   }
   };
protected:
   SimpleFileFactoryClass *Factory; //4
   DynamicVectorClass<FileInfoStruct> FileInformation; //8
   StringClass MixFilename; //32
   unsigned int UnkOffset; //36
   unsigned int FileCount; //40
   unsigned int MixFilenameOffset; //44
   bool IsValid; //48
   DynamicVectorClass<StringClass> Filenames; //52
   DynamicVectorClass<AddInfoStruct> AddedFiles; //76
   bool FileAdded; //100
public:
   MixFileFactoryClass(const char*, SimpleFileFactoryClass&);
   virtual ~MixFileFactoryClass ();
   
   virtual FileClass* Get_File    (const char* fileName);
   virtual void       Return_File (FileClass* file);
   void Build_Filename_List(DynamicVectorClass<StringClass>&list);
};


#endif
