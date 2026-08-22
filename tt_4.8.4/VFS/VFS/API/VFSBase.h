#ifndef _VFSBASE_H_
#define _VFSBASE_H_

#ifndef VFS_INTERNAL
#include <windows.h>
#include "VFSModule.h"

typedef UINT64	size_64;
typedef UINT32	size_32;
typedef INT64	ssize_64;
typedef INT32	ssize_32;
typedef void*	ptr;

#define VFS_ROOT_UID	((size_64)-1)
#define VFS_INVALID_UID	((size_64)-2)
#define VFS_FLAT_UID	((size_64)-3) //Flat files get this UID
#endif //VFS_INTERNAL

#ifndef _DEBUG
#define VFS_CALL	_fastcall
#ifdef _M_X64
#define VFS_INLINE	
#else //_M_X64
#define VFS_INLINE	__forceinline
#endif //_M_X64
#else //_DEBUG
#define VFS_CALL
#define VFS_INLINE
#endif //_DEBUG

#define FILE_CREATE				(1)			//Create if it doesn't exist
#define FILE_CREATE_ALWAYS		(1 << 1)	//Reset existing files
#define FILE_OPEN_ALWAYS		(1 << 2)	//Create if it doesn't exist
#define FILE_OPEN_EXISTING		(1 << 3)	//Open only existing files
#define FILE_MODE_ALL			(FILE_CREATE|FILE_CREATE_ALWAYS|FILE_OPEN_ALWAYS|FILE_OPEN_EXISTING)

#define FILE_OPEN_READ			(1 << 4)
#define FILE_OPEN_WRITE			(1 << 5)
#define FILE_OPEN_READ_WRITE	(FILE_OPEN_READ|FILE_OPEN_WRITE)

#define FILE_OPEN_MEMORY		(1 << 6)



#define VFS_DELETE_FAILED		(-1)
#define VFS_DELETE_SUCCESS		(0)
#define VFS_DELETE_SOME			(1)

#define VFS_TYPE_INTERNAL		(1)
#define VFS_TYPE_FILE			(2)
#define VFS_TYPE_DIRECTORY		(3)

#define VFS_ENUM_FILES			(1)
#define VFS_ENUM_DIRECTORIES	(2)
#define VFS_ENUM_ALL			(VFS_ENUM_FILES | VFS_ENUM_DIRECTORIES)

#define VFS_ENUM_CONTINUE		(0)
#define VFS_ENUM_STOP			(1)
#define VFS_ENUM_BREAK			(1<<1)
#define VFS_ENUM_AND_VALUE		(VFS_ENUM_CONTINUE | VFS_ENUM_STOP | VFS_ENUM_BREAK)

#define VFS_ENUM_MY_DIRS		(1<<2)
#define VFS_ENUM_MY_FILES		(1<<3)
#define VFS_ENUM_MY_AND_VALUE	(VFS_ENUM_MY_DIRS | VFS_ENUM_MY_FILES)

typedef struct _VFSFILEINFO {
	//This name
	const wchar_t*		m_pName;
	//Full path
	const wchar_t*		m_pFullName;
	//Type (file, dir, unused)
	char				m_Type;
	//Size (applies to files only)
	size_64				m_Size;
	//Can be used to query a file data ptr
	ptr					m_pFileData;
	//User data
	ptr					m_pUserData;
	//Flags
	UINT32				m_Flags;
	//UID to be used for new functions that open by UID
	size_64				m_UID;
	//Ref count on files
	size_64				m_RefCount;
	//MD5
	bool				m_GotMD5;
	unsigned char		m_MD5[16];
} VFSFILEINFO, *PVFSFILEINFO;

typedef int (VFS_CALL* pfEnumCallback)(PVFSFILEINFO pFileInfo, ptr wParam, ptr lParam);
typedef void (VFS_CALL* pfProgressCallback)(const ptr pData, int type, size_64 progress, size_64 total, ptr pCookie);

typedef struct _PROGESSCB {
	pfProgressCallback	m_pCB;
	ptr					m_pCookie;
	BOOL				m_UseInternalCache;
} PROGESSCB, *PPROGESSCB;

typedef struct _PROGRESSNEXTFILE {
} PROGRESSNEXTFILE, *LPPROGRESSNEXTFILE;

typedef ptr VFSFileHandle;

#define VFS_INVALID_FILE		((VFSFileHandle)NULL)
#define VFS_VALID_FILE(hFile)	((hFile) != VFS_INVALID_FILE)

#define PROGRESS_TYPE_START		(1)
#define PROGRESS_TYPE_NEXT		(2)
#define PROGRESS_TYPE_END		(3)
//Used for directory/file import/export stuff
#define PROGRESS_TYPE_FILEINFO	(4)
#define PROGRESS_TYPE_NEXTFILE	(5)
#define PROGRESS_TYPE_FILECOUNT	(6)
#define PROGRESS_TYPE_LASTFILE	(7)


//Default cluster sizes

#define CS_4K	(4*1024)
#define CS_8K	(8*1024)
#define CS_16K	(16*1024)
#define CS_32K	(32*1024)

/*#ifndef ptr
typedef void*ptr;
#endif //ptr//*/

interface IVFS;
interface IVFS2;

interface IVFSCoreInterface {
public:
	virtual IVFS* CreateVFS() = 0;
	virtual IVFS2* CreateVFS2() = 0;
	virtual IVFS* CreateRPCVFS() = 0;
	virtual void RegisterProtocol(const wchar_t* pName) = 0;
	virtual void RegisterForProtocol(IVFS* pVFS) = 0;
	virtual void UnregisterForProtocol(IVFS* pVFS) = 0;
	virtual IVFS* FindVFSForDomain(const wchar_t* pDomain) = 0;
};


//Allows access to V2 interface

extern "C" IVFSCoreInterface* VFS_CALL QueryCoreInterface();
extern "C" IVFSCoreInterface* VFS_CALL RPCQueryCoreInterface();

typedef IVFSCoreInterface *(VFS_CALL *pfnQueryCoreInterface)();
typedef IVFSCoreInterface *(VFS_CALL *pfnRPCQueryCoreInterface)();

#define VFS_COREIF_PROC			"QueryCoreInterface"
#define RPC_VFS_COREIF_PROFC	"RPCQueryCoreInterface"

#define VFS_QUERY_COREIF	\
	IVFSCoreInterface* VFSQueryCoreInterface(){ \
		static HMODULE s_Module = NULL; \
		static pfnQueryCoreInterface s_pfnQueryCoreInterface = NULL; \
		if (!s_Module) s_Module = LoadLibraryW(VFS_MODULE_NAME); \
		if (s_Module) s_pfnQueryCoreInterface = (pfnQueryCoreInterface)GetProcAddress(s_Module, VFS_COREIF_PROC); \
		return (s_pfnQueryCoreInterface) ? s_pfnQueryCoreInterface() : NULL; \
	}
	


#endif //_VFSBASE_H_
