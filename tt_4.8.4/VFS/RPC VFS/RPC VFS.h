#ifndef _RPC_VFS_H_
#define _RPC_VFS_H_

#include <windows.h>
#include "../Shared/Memory.h"
#include "../Shared/Lists.h"
#include "../Shared/BaseTypes.h"
#include "../Shared/StdLib.h"
#include "../Shared/StreamMap.h"
#include "../VFS/API/VFSBase.h"
#include "../VFS/API/VFSAdvanced.h"
#include "../VFS/API/VFSInterface.h"
#include "../VFS/API/VFSRPCInterface.h"
#include "../VFS/API/VFSCryptography.h"

//class RPCVFS : public IRPCVFS {
class RPCVFS : public IVFS {
private:
	RPC_WSTR			m_pStringBinding;
	RPC_BINDING_HANDLE	m_RPCBindingHandle;
public:
	RPCVFS();
	virtual ~RPCVFS();
	virtual int GetVersion();

	virtual void PartialFlush();
	virtual void FullFlush();

	virtual bool Open(const wchar_t *pFilename, size_32 UserVersion, bool ReadOnly);
	virtual bool Create(const wchar_t *pFilename, size_32 UserVersion, size_32 ClusterSize);
	virtual void Close();
	virtual void SetOption(int option, void *pData, int datasize);
	
	virtual int GetLastError();

	virtual VFSFileHandle VFSCreateFile(const wchar_t* pName, UINT32 Mode);
	virtual VFSFileHandle VFSCreateFiledataFile(ptr pFiledata, UINT32 Mode);
	virtual VFSFileHandle VFSCreateUIDFile(size_64 UID, UINT32 Mode);
	virtual void VFSCloseFile(VFSFileHandle hFile);

	virtual size_32 VFSRead(VFSFileHandle hFile, void* pBuffer, size_32 Count, PPROGESSCB pCB);
	virtual size_32 VFSWrite(VFSFileHandle hFile, const void *pBuffer, size_32 Count, PPROGESSCB pCB);
	virtual size_64 VFSSeek(VFSFileHandle hFile, ssize_64 Offset, int Origin);
	virtual size_64 VFSTell(VFSFileHandle hFile);
	virtual size_64 VFSSize(VFSFileHandle hFile);
	virtual bool VFSEndOfFile(VFSFileHandle hFile);
	virtual bool VFSSetEndOfFile(VFSFileHandle hFile);
	virtual const void* VFSMemCache(VFSFileHandle hFile);
	virtual size_64 VFSGetUIDFromFileHandle(VFSFileHandle hFile);

	virtual bool VFSDeleteFiledataDirectory(ptr pDir);
	virtual bool VFSDeleteFiledataFile(ptr pFile);

	virtual int VFSEnum(const wchar_t* pDirectory, pfEnumCallback pCB, ptr wParam, ptr lParam, int what);
	virtual int VFSSingleEnum(ptr pFileData, pfEnumCallback pCB, ptr wParam, ptr lParam);
	virtual bool VFSEmbedFile(const wchar_t *pSrcFile, const wchar_t* pDestFile, bool OverWrite, PPROGESSCB pCB);
	virtual bool VFSExtractFile(const wchar_t *pSrcFile, const wchar_t* pDestFile, PPROGESSCB pCB);
	virtual bool VFSEmbedDir(const wchar_t *pSrcDir, const wchar_t* pDestDir, bool OverWrite, PPROGESSCB pCB);
	virtual bool VFSExtractDir(const wchar_t *pSrcDir, const wchar_t* pDestDir, bool OverWrite, PPROGESSCB pCB);

	virtual bool VFSEmbedFiledataFile(const wchar_t *pSrcFile, ptr pDestDir, bool OverWrite, PPROGESSCB pCB);
	virtual bool VFSExtractFiledataFile(ptr pSrcFile, const wchar_t* pDestFile, PPROGESSCB pCB);
	virtual bool VFSEmbedFiledataDir(const wchar_t *pSrcDir, ptr pDestDir, bool OverWrite, PPROGESSCB pCB);
	virtual bool VFSExtractFiledataDir(ptr pSrcDir, const wchar_t* pDestDir, bool OverWrite, PPROGESSCB pCB);

	virtual bool VFSQueryFileInfo(PVFSFILEINFO pFileInfo, ptr pFileData, wchar_t *pDest, int DestSize);
	virtual bool VFSSetFileUserData(ptr pFileData, ptr pUserData, size_32 size);
	virtual bool VFSGetFileUserData(ptr pFileData, ptr pUserData, size_32 size);
	virtual void VFSSetRuntimeUserData(ptr pFileData, ptr pUserData);
	virtual ptr VFSGetRuntimeUserData(ptr pFileData);
	virtual ptr VFSGetRootFiledata();
	virtual ptr VFSGetFileData(const wchar_t* pPath);
	virtual bool VFSFileExists(const wchar_t* pFile);
	virtual bool VFSPathExists(const wchar_t* pPath);
	virtual size_64 VFSFileSize(const wchar_t* pPath);

	virtual int VFSSetNoEnum(ptr pFileData, int noenum);
	virtual int VFSGetNoEnum(ptr pFileData);
	virtual void VFSSetFileOption(VFSFileHandle hFile, int fileoption, ptr pData, int size);
	virtual size_64 VFSGetFileUID(const wchar_t* pPath);
	virtual size_64 VFSGetDirectoryUID(const wchar_t* pPath);
	virtual size_64 VFSGetEntryUID(const wchar_t* pPath);
	virtual size_64 VFSGetFiledataUID(ptr pFileData);

	virtual bool VFSDeleteFile(const wchar_t *pFile);
	virtual bool VFSDeleteDirectory(const wchar_t *pDir);

	virtual int VFSGetSettings();
	virtual size_32 VFSGetClusterSize();

	virtual void DumpFTInfo();

	virtual const wchar_t* TranslateErrorCode(int error);

	virtual bool VFSGetMD5(const wchar_t* pFile, unsigned char *pMD5, bool Store);
	virtual bool VFSGetFileDataMD5(ptr pFileData, unsigned char *pMD5, bool Store);
	virtual bool VFSQueryMD5(const wchar_t* pFile, unsigned char *pMD5);
	virtual bool VFSQueryFileDataMD5(ptr pFileData, unsigned char *pMD5);

	virtual IStream* VFSCreateIStreamFile(const wchar_t* pName, UINT32 Mode);
	virtual IStream* VFSCreateIStreamFiledataFile(ptr pFiledata, UINT32 Mode);

	virtual bool VFSFileDataFileExists(ptr pFileData, const wchar_t* pFile);
	virtual bool VFSFileDataPathExists(ptr pFileData, const wchar_t* pPath);

	virtual bool VFSRenameFile(const wchar_t* pFileName, const wchar_t* pNewName);
	virtual bool VFSRenameFolder(const wchar_t* pPathName, const wchar_t* pNewName);

	virtual bool VFSRenameFiledataFile(ptr pFileData, const wchar_t* pNewName);
	virtual bool VFSRenameFiledataFolder(ptr pFileData, const wchar_t* pNewName);

	virtual ptr VFSCreateDirectory(ptr pFiledata, const wchar_t* pBaseName, wchar_t *pNewName, size_32 NewNameSize);

	virtual bool VFSRegisterForProtocol(const wchar_t* pDomain);
	virtual void VFSUnregisterForProtocol();

	virtual const wchar_t* VFSGetDomain();
	virtual ptr VFSFileDataFromFilehandle(VFSFileHandle hFile);

	virtual bool VFSMoveFiledataEntry(ptr pEntry, ptr pNewDir);
	virtual bool VFSMoveEntry(const wchar_t *pPath, ptr pNewDir);
};

#endif //_RPC_VFS_H_
