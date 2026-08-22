#ifndef _VFSINTERFACE_H_
#define _VFSINTERFACE_H_

#include "VFSBase.h"

enum {
	VFS_INTERFACE_FLAT = 0,
	VFS_INTERFACE_FLAT_V2,
	VFS_INTERFACE_RPC,
};

interface IVFS {
public:
	virtual ~IVFS() {};

	virtual int GetVersion() = 0;

	virtual void PartialFlush() = 0;
	virtual void FullFlush() = 0;

	virtual bool Open(const wchar_t *pFilename, size_32 UserVersion, bool ReadOnly) = 0;
	virtual bool Create(const wchar_t *pFilename, size_32 UserVersion, size_32 ClusterSize) = 0;
	virtual void Close() = 0;
	virtual void SetOption(int option, void *pData, int datasize) = 0;
	
	virtual int GetLastError() = 0;

	virtual VFSFileHandle VFSCreateFile(const wchar_t* pName, UINT32 Mode) = 0;
	virtual VFSFileHandle VFSCreateFiledataFile(ptr pFiledata, UINT32 Mode) = 0;
	virtual VFSFileHandle VFSCreateUIDFile(size_64 UID, UINT32 Mode) = 0;
	virtual void VFSCloseFile(VFSFileHandle hFile) = 0;

	virtual size_32 VFSRead(VFSFileHandle hFile, void* pBuffer, size_32 Count, PPROGESSCB pCB) = 0;
	virtual size_32 VFSWrite(VFSFileHandle hFile, const void *pBuffer, size_32 Count, PPROGESSCB pCB) = 0;
	virtual size_64 VFSSeek(VFSFileHandle hFile, ssize_64 Offset, int Origin) = 0;
	virtual size_64 VFSTell(VFSFileHandle hFile) = 0;
	virtual size_64 VFSSize(VFSFileHandle hFile) = 0;
	virtual bool VFSEndOfFile(VFSFileHandle hFile) = 0;
	virtual bool VFSSetEndOfFile(VFSFileHandle hFile) = 0;
	virtual const void* VFSMemCache(VFSFileHandle hFile) = 0;
	virtual size_64 VFSGetUIDFromFileHandle(VFSFileHandle hFile) = 0;

	virtual bool VFSDeleteFiledataDirectory(ptr pDir) = 0;
	virtual bool VFSDeleteFiledataFile(ptr pFile) = 0;

	virtual int VFSEnum(const wchar_t* pDirectory, pfEnumCallback pCB, ptr wParam, ptr lParam, int what) = 0;
	virtual int VFSSingleEnum(ptr pFileData, pfEnumCallback pCB, ptr wParam, ptr lParam) = 0;
	virtual bool VFSEmbedFile(const wchar_t *pSrcFile, const wchar_t* pDestFile, bool OverWrite, PPROGESSCB pCB) = 0;
	virtual bool VFSExtractFile(const wchar_t *pSrcFile, const wchar_t* pDestFile, PPROGESSCB pCB) = 0;
	virtual bool VFSEmbedDir(const wchar_t *pSrcDir, const wchar_t* pDestDir, bool OverWrite, PPROGESSCB pCB) = 0;
	virtual bool VFSExtractDir(const wchar_t *pSrcDir, const wchar_t* pDestDir, bool OverWrite, PPROGESSCB pCB) = 0;

	virtual bool VFSEmbedFiledataFile(const wchar_t *pSrcFile, ptr pDestDir, bool OverWrite, PPROGESSCB pCB) = 0;
	virtual bool VFSExtractFiledataFile(ptr pSrcFile, const wchar_t* pDestFile, PPROGESSCB pCB) = 0;
	virtual bool VFSEmbedFiledataDir(const wchar_t *pSrcDir, ptr pDestDir, bool OverWrite, PPROGESSCB pCB) = 0;
	virtual bool VFSExtractFiledataDir(ptr pSrcDir, const wchar_t* pDestDir, bool OverWrite, PPROGESSCB pCB) = 0;

	virtual bool VFSQueryFileInfo(PVFSFILEINFO pFileInfo, ptr pFileData, wchar_t *pDest, int DestSize) = 0;
	virtual bool VFSSetFileUserData(ptr pFileData, ptr pUserData, size_32 size) = 0;
	virtual bool VFSGetFileUserData(ptr pFileData, ptr pUserData, size_32 size) = 0;
	virtual void VFSSetRuntimeUserData(ptr pFileData, ptr pUserData) = 0;
	virtual ptr VFSGetRuntimeUserData(ptr pFileData) = 0;
	virtual ptr VFSGetRootFiledata() = 0;
	virtual ptr VFSGetFileData(const wchar_t* pPath) = 0;
	virtual bool VFSFileExists(const wchar_t* pFile) = 0;
	virtual bool VFSPathExists(const wchar_t* pPath) = 0;
	virtual size_64 VFSFileSize(const wchar_t* pPath) = 0;

	virtual int VFSSetNoEnum(ptr pFileData, int noenum) = 0;
	virtual int VFSGetNoEnum(ptr pFileData) = 0;
	virtual void VFSSetFileOption(VFSFileHandle hFile, int fileoption, ptr pData, int size) = 0;
	virtual size_64 VFSGetFileUID(const wchar_t* pPath) = 0;
	virtual size_64 VFSGetDirectoryUID(const wchar_t* pPath) = 0;
	virtual size_64 VFSGetEntryUID(const wchar_t* pPath) = 0;
	virtual size_64 VFSGetFiledataUID(ptr pFileData) = 0;

	virtual bool VFSDeleteFile(const wchar_t *pFile) = 0;
	virtual bool VFSDeleteDirectory(const wchar_t *pDir) = 0;

	virtual int VFSGetSettings() = 0;
	virtual size_32 VFSGetClusterSize() = 0;

	virtual void DumpFTInfo() = 0;

	virtual const wchar_t* TranslateErrorCode(int error) = 0;

	//Generates the MD5 [and stores to MFT]
	virtual bool VFSGetMD5(const wchar_t* pFile, unsigned char *pMD5, bool Store) = 0;
	virtual bool VFSGetFileDataMD5(ptr pFileData, unsigned char *pMD5, bool Store) = 0;
	//Read MD5 from MFT
	virtual bool VFSQueryMD5(const wchar_t* pFile, unsigned char *pMD5) = 0;
	virtual bool VFSQueryFileDataMD5(ptr pFileData, unsigned char *pMD5) = 0;

	virtual IStream* VFSCreateIStreamFile(const wchar_t* pName, UINT32 Mode) = 0;
	virtual IStream* VFSCreateIStreamFiledataFile(ptr pFiledata, UINT32 Mode) = 0;

	virtual bool VFSFileDataFileExists(ptr pFileData, const wchar_t* pFile) = 0;
	virtual bool VFSFileDataPathExists(ptr pFileData, const wchar_t* pPath) = 0;

	virtual bool VFSRenameFile(const wchar_t* pFileName, const wchar_t* pNewName) = 0;
	virtual bool VFSRenameFolder(const wchar_t* pPathName, const wchar_t* pNewName) = 0;

	virtual bool VFSRenameFiledataFile(ptr pFileData, const wchar_t* pNewName) = 0;
	virtual bool VFSRenameFiledataFolder(ptr pFileData, const wchar_t* pNewName) = 0;

	virtual ptr VFSCreateDirectory(ptr pFiledata, const wchar_t* pBaseName, wchar_t *pNewName, size_32 NewNameSize) = 0;

	virtual bool VFSRegisterForProtocol(const wchar_t* pDomain) = 0;
	virtual void VFSUnregisterForProtocol() = 0;
	virtual const wchar_t* VFSGetDomain() = 0;
	virtual ptr VFSFileDataFromFilehandle(VFSFileHandle hFile) = 0;
	virtual bool VFSMoveFiledataEntry(ptr pEntry, ptr pNewDir) = 0;
	virtual bool VFSMoveEntry(const wchar_t *pPath, ptr pNewDir) = 0;
};

//Kept for backward compat
extern "C" IVFS* VFS_CALL CreateNewInstance();
extern "C" IVFS* VFS_CALL RPCCreateNewInstance();
extern "C" size_32 VFS_CALL GetVFSVersion();

typedef IVFS *(VFS_CALL *pfCreateNewInstance)();
typedef IVFS *(VFS_CALL *pfRPCCreateNewInstance)();
typedef size_32 (VFS_CALL *pfGetVFSVersion)();

#define VFS_INSTANCE_PROC		"CreateNewInstance"
#define RPC_VFS_INSTANCE_PROC	"RPCCreateNewInstance"
#define VFS_VERSION_PROC		"GetVFSVersion"

//Allows access to V2 interface

extern "C" bool VFS_CALL VFSCreateInterface(int version, void **ppOutIf);
typedef bool (VFS_CALL *pfVFSCreateInterface)(int version, void **ppOutIf);
#define VFS_CREATEINTERFACE_PROC	"VFSCreateInterface"

#endif //_VFSINTERFACE_H_
