#ifndef TT_INCLUDE__VFSFILE_H
#define TT_INCLUDE__VFSFILE_H



#include "engine_io.h"

#include "..\VFS\VFS\API\VFSInterface.h"
#include "..\VFS\VFS\API\VFSErrorCodes.h"



class VfsFile :
	public FileClass
{

public:

	IVFS* filesystem;
	uint64 fileId;
	VFSFileHandle handle;
	WideStringClass fileName;

	VfsFile(IVFS* _filesystem, uint64 _fileId);
	VfsFile(IVFS* _filesystem, const wchar_t* _fileId);

	virtual ~VfsFile();
	virtual const char* File_Name() { TT_UNIMPLEMENTED; }
	virtual const char* Set_Name(const char* name) { TT_UNIMPLEMENTED; }
	virtual bool Create() { TT_UNIMPLEMENTED; }
	virtual bool Delete() { TT_UNIMPLEMENTED; }
	virtual bool Is_Available(int handle = 0) { return fileId != VFS_INVALID_UID; }
	virtual bool Is_Open() { return handle != 0; }
	virtual int Open(const char* name, int mode = 1) { TT_UNIMPLEMENTED; }
	virtual int Open(int mode = 1);
	virtual int Read(void* buffer, int size) { return filesystem->VFSRead(handle, buffer, size, NULL); }
	virtual int Seek(int offset, int origin) { return (uint32)filesystem->VFSSeek(handle, offset, origin); }
	virtual int Size() { return (uint32)filesystem->VFSSize(handle); }
	virtual int Write(void* buffer, int size) { return filesystem->VFSWrite(handle, buffer, size, NULL); }
	virtual void Close() { filesystem->VFSCloseFile(handle); handle = 0; }
	virtual void Error(int a, int b, const char *c) { TT_UNIMPLEMENTED; }
	virtual void Bias(int start, int length) { TT_UNIMPLEMENTED; }

};



#endif