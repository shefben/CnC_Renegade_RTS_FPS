#ifndef _ARCHIVE_H_
#define _ARCHIVE_H_

#include "BaseTypes.h"
#include "Base.h"
#include <windows.h>

#define CHUNK_VFS_FILES			_32BIT_INVERSE('VFS\0')
#define CHUNK_EXTERN_FILES		_32BIT_INVERSE('EXTR')
#define CHUNK_DIRS				_32BIT_INVERSE('DIRS')
#define CHUNK_FILEINFO			_32BIT_INVERSE('INFO')
#define CHUNK_FILE				_32BIT_INVERSE('FILE')
#define CHUNK_DATA				_32BIT_INVERSE('DATA')
#define CHUNK_ZIP_DATA			_32BIT_INVERSE('ZIP\0')

#define MICRO_CHUNK_FILENAME	_16BIT_INVERSE('FN')
#define MICRO_CHUNK_DIRNAME		_16BIT_INVERSE('DN')

typedef void (SHARED_CALL *pfnExtractCallback)(const wchar_t *pName, size_32 progress, size_32 size, size_32 curfile, size_32 totalfiles, void *pCookie);
typedef void (SHARED_CALL *pfnExtractProgressCallback)(const wchar_t *pName, size_32 progress, size_32 size, size_32 curfile, size_32 totalfiles, void *pCookie);
typedef IStream* (SHARED_CALL *pfnStreamExtractCallback)(const wchar_t *pName, void *pCookie);

bool ExtractVFS(const wchar_t *pArchive, pfnStreamExtractCallback pCallback, void *pCookie);
bool ExtractExtern(const wchar_t *pDestDir, const wchar_t *pArchive, pfnExtractCallback pCallback, void *pCookie);


bool GenerateArchive(const wchar_t *pOut, const wchar_t *pVFS, const wchar_t *pExtern, bool zip);



#endif //_ARCHIVE_H_
