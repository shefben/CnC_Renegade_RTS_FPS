#include "General.h"
#include "VfsFile.h"



VfsFile::VfsFile(IVFS* _filesystem, uint64 _fileId) :
	filesystem(_filesystem),
	fileId(_fileId),
	handle(NULL)
{
}



VfsFile::VfsFile(IVFS* _filesystem, const wchar_t* _fileName) :
	filesystem(_filesystem),
	fileId(VFS_INVALID_UID),
	handle(NULL),
	fileName(_fileName)
{
}



VfsFile::~VfsFile()
{
	Close();
}


int VfsFile::Open(int mode)
{
	TT_ASSERT(!Is_Open());

	uint32 openFlags;
	switch (mode)
	{
		case 0: openFlags = FILE_OPEN_ALWAYS | FILE_OPEN_READ_WRITE; break;
		case 1: openFlags = FILE_OPEN_EXISTING | FILE_OPEN_READ | FILE_OPEN_MEMORY; break;
		case 2: openFlags = FILE_CREATE_ALWAYS | FILE_OPEN_WRITE; break;
		default: TT_UNREACHABLE;
	}

	if (fileId != VFS_INVALID_UID)
		handle = filesystem->VFSCreateUIDFile(fileId, openFlags);
	else
		handle = filesystem->VFSCreateFile(fileName, openFlags);

	return Is_Open();
}