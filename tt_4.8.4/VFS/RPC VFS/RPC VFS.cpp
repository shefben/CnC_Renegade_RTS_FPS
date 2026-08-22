
#include <windows.h>
#include "../Shared/Memory.h"
#include "../Shared/Lists.h"
#include "../Shared/BaseTypes.h"
#include "../Shared/StdLib.h"
#include "../VFS/API/VFSBase.h"
#include "../VFS/API/VFSAdvanced.h"
#include "../VFS/API/VFSInterface.h"
#include "../VFS/API/VFSCryptography.h"
#include "../VFS/API/VFSPipeCommands.h"
#include "../VFS/API/VFSErrorCodes.h"
#include "RPC VFS.h"
#include "RPC VFS_h.h"
#include "../VFS/VFSStructures.h"

//template<typename T>

RPCVFS::RPCVFS(){
	m_pStringBinding = NULL;
	m_RPCBindingHandle = NULL;
}

RPCVFS::~RPCVFS(){
	Close();
}

int RPCVFS::GetVersion(){
	return VFS_INTERFACE_RPC;
}

void RPCVFS::PartialFlush(){
	if (!m_RPCBindingHandle) return;
	VFS_RPC_PartialFlush(m_RPCBindingHandle);
}

void RPCVFS::FullFlush(){
	if (!m_RPCBindingHandle) return;
	VFS_RPC_FullFlush(m_RPCBindingHandle);
}

bool RPCVFS::Open(const wchar_t *pFilename, size_32 UserVersion, bool ReadOnly){
	RPC_STATUS status;

	status = RpcStringBindingComposeW(NULL, (RPC_WSTR)L"ncalrpc", NULL, (RPC_WSTR)pFilename, NULL, &m_pStringBinding);

	if (status != RPC_S_OK){
		Close();
		return false;
	}

	status = RpcBindingFromStringBinding(m_pStringBinding, &m_RPCBindingHandle);

	if (status != RPC_S_OK){
		Close();
		return false;
	}
	return true;
}

bool RPCVFS::Create(const wchar_t *pFilename, size_32 UserVersion, size_32 ClusterSize){
	return Open(pFilename, UserVersion, false);
}

void RPCVFS::Close(){
	if (m_pStringBinding) RpcStringFreeW(&m_pStringBinding);
	if (m_RPCBindingHandle) RpcBindingFree(&m_RPCBindingHandle);
}

void RPCVFS::SetOption(int option, void *pData, int datasize){

}


int RPCVFS::GetLastError(){
	int error;
	if (!m_RPCBindingHandle) return VFS_OK;
	VFS_RPC_GetLastError(m_RPCBindingHandle, &error);
	return error;
}


VFSFileHandle RPCVFS::VFSCreateFile(const wchar_t* pName, UINT32 Mode){
	RV_VFSFileHandle f;
	if (!m_RPCBindingHandle) return VFS_INVALID_FILE;
	return VFS_RPC_VFSCreateFile(m_RPCBindingHandle, pName, (RV_UINT32)Mode, &f) ? GetPtr(&f) : VFS_INVALID_FILE;
}

VFSFileHandle RPCVFS::VFSCreateFiledataFile(ptr pFiledata, UINT32 Mode){
	RV_VFSFileHandle f;
	RV_FileData fd;
	if (!m_RPCBindingHandle) return VFS_INVALID_FILE;
	SetPtr(&fd, pFiledata);
	return VFS_RPC_VFSCreateFiledataFile(m_RPCBindingHandle, fd, (RV_UINT32)Mode, &f) ? GetPtr(&f) : VFS_INVALID_FILE;
}

VFSFileHandle RPCVFS::VFSCreateUIDFile(size_64 UID, UINT32 Mode){
	RV_VFSFileHandle f;
	if (!m_RPCBindingHandle) return VFS_INVALID_FILE;
	return VFS_RPC_VFSCreateUIDFile(m_RPCBindingHandle, (RV_size_64)UID, (RV_UINT32)Mode, &f) ? GetPtr(&f) : VFS_INVALID_FILE;
}

void RPCVFS::VFSCloseFile(VFSFileHandle hFile){
	RV_VFSFileHandle f;
	if (!m_RPCBindingHandle) return;
	SetPtr(&f, hFile);
	VFS_RPC_VFSCloseFile(m_RPCBindingHandle, f);
}


size_32 RPCVFS::VFSRead(VFSFileHandle hFile, void* pBuffer, size_32 Count, PPROGESSCB pCB){
	RV_VFSFileHandle f;
	RV_size_32 r;
	RV_PROGESSCB cb;
	if (!m_RPCBindingHandle) return 0;
	SetPtr(&f, hFile);
	return VFS_RPC_VFSRead(m_RPCBindingHandle, f, (RV_ptr)pBuffer, (RV_size_32)Count, &cb, &r) ? (size_32)r : 0;
}

size_32 RPCVFS::VFSWrite(VFSFileHandle hFile, const void *pBuffer, size_32 Count, PPROGESSCB pCB){
	RV_VFSFileHandle f;
	RV_size_32 w;
	RV_PROGESSCB cb;
	if (!m_RPCBindingHandle) return 0;
	SetPtr(&f, hFile);
	return VFS_RPC_VFSWrite(m_RPCBindingHandle, f, (RV_ptr)pBuffer, (RV_size_32)Count, &cb, &w) ? (size_32)w : 0;
}

size_64 RPCVFS::VFSSeek(VFSFileHandle hFile, ssize_64 Offset, int Origin){
	RV_VFSFileHandle f;
	RV_size_64 pos;
	if (!m_RPCBindingHandle) return 0;
	SetPtr(&f, hFile);
	return VFS_RPC_VFSSeek(m_RPCBindingHandle, f, (RV_ssize_64)Offset, Origin, &pos) ? (size_64)pos : 0;
}

size_64 RPCVFS::VFSTell(VFSFileHandle hFile){
	RV_VFSFileHandle f;
	RV_size_64 pos;
	if (!m_RPCBindingHandle) return 0;
	SetPtr(&f, hFile);
	return VFS_RPC_VFSTell(m_RPCBindingHandle, f, &pos) ? (size_64)pos : 0;
}

size_64 RPCVFS::VFSSize(VFSFileHandle hFile){
	RV_VFSFileHandle f;
	RV_size_64 size;
	if (!m_RPCBindingHandle) return 0;
	SetPtr(&f, hFile);
	return VFS_RPC_VFSSize(m_RPCBindingHandle, f, &size) ? (size_64)size : 0;
}

bool RPCVFS::VFSEndOfFile(VFSFileHandle hFile){
	RV_VFSFileHandle f;
	if (!m_RPCBindingHandle) return false;
	SetPtr(&f, hFile);

	return VFS_RPC_VFSEndOfFile(m_RPCBindingHandle, f) ? true : false;
}

bool RPCVFS::VFSSetEndOfFile(VFSFileHandle hFile){
	RV_VFSFileHandle f;
	if (!m_RPCBindingHandle) return false;
	SetPtr(&f, hFile);

	return VFS_RPC_VFSSetEndOfFile(m_RPCBindingHandle, f) ? true : false;
}

const void* RPCVFS::VFSMemCache(VFSFileHandle hFile){
	//Not supported
	return NULL;
}

size_64 RPCVFS::VFSGetUIDFromFileHandle(VFSFileHandle hFile){
	RV_size_64 UID;
	RV_VFSFileHandle f;
	if (!m_RPCBindingHandle) return INVALID_UID;
	SetPtr(&f, hFile);

	return (VFS_RPC_VFSGetUIDFromFileHandle(m_RPCBindingHandle, f, &UID) ? (size_64)UID : INVALID_UID);
}


bool RPCVFS::VFSDeleteFiledataDirectory(ptr pDir){
	RV_FileData fd;
	if (!m_RPCBindingHandle) return false;
	SetPtr(&fd, pDir);
	return VFS_RPC_VFSDeleteFiledataDirectory(m_RPCBindingHandle, fd) ? true : false;
}

bool RPCVFS::VFSDeleteFiledataFile(ptr pFile){
	RV_FileData fd;
	if (!m_RPCBindingHandle) return false;
	SetPtr(&fd, pFile);
	return VFS_RPC_VFSDeleteFiledataFile(m_RPCBindingHandle, fd) ? true : false;
}


int RPCVFS::VFSEnum(const wchar_t* pDirectory, pfEnumCallback pCB, ptr wParam, ptr lParam, int what){
	//Not supported yet
	return 0;
}

int RPCVFS::VFSSingleEnum(ptr pFileData, pfEnumCallback pCB, ptr wParam, ptr lParam){
	//Not supported yet
	return 0;
}

bool RPCVFS::VFSEmbedFile(const wchar_t *pSrcFile, const wchar_t* pDestFile, bool OverWrite, PPROGESSCB pCB){
	RV_PROGESSCB cb;
	if (!m_RPCBindingHandle) return false;
	return VFS_RPC_VFSEmbedFile(m_RPCBindingHandle, pSrcFile, pDestFile, (RV_BOOL)(OverWrite ? TRUE : FALSE), &cb) ? true : false;
}

bool RPCVFS::VFSExtractFile(const wchar_t *pSrcFile, const wchar_t* pDestFile, PPROGESSCB pCB){
	RV_PROGESSCB cb;
	if (!m_RPCBindingHandle) return false;
	return VFS_RPC_VFSExtractFile(m_RPCBindingHandle, pSrcFile, pDestFile, &cb) ? true : false;
	
}

bool RPCVFS::VFSEmbedDir(const wchar_t *pSrcDir, const wchar_t* pDestDir, bool OverWrite, PPROGESSCB pCB){
	RV_PROGESSCB cb;
	if (!m_RPCBindingHandle) return false;
	return VFS_RPC_VFSEmbedDir(m_RPCBindingHandle, pSrcDir, pDestDir, (RV_BOOL)(OverWrite ? TRUE : FALSE), &cb) ? true : false;
}

bool RPCVFS::VFSExtractDir(const wchar_t *pSrcDir, const wchar_t* pDestDir, bool OverWrite, PPROGESSCB pCB){
	RV_PROGESSCB cb;
	if (!m_RPCBindingHandle) return false;
	return VFS_RPC_VFSExtractDir(m_RPCBindingHandle, pSrcDir, pDestDir, (RV_BOOL)(OverWrite ? TRUE : FALSE), &cb) ? true : false;
}


bool RPCVFS::VFSEmbedFiledataFile(const wchar_t *pSrcFile, ptr pDestDir, bool OverWrite, PPROGESSCB pCB){
	RV_FileData fd;
	RV_PROGESSCB cb;
	if (!m_RPCBindingHandle) return false;
	SetPtr(&fd, pDestDir);
	return VFS_RPC_VFSEmbedFiledataFile(m_RPCBindingHandle, pSrcFile, fd, (RV_BOOL)(OverWrite ? TRUE : FALSE), &cb) ? true : false;
}

bool RPCVFS::VFSExtractFiledataFile(ptr pSrcFile, const wchar_t* pDestFile, PPROGESSCB pCB){
	RV_FileData fd;
	RV_PROGESSCB cb;
	if (!m_RPCBindingHandle) return false;
	SetPtr(&fd, pSrcFile);

	return VFS_RPC_VFSExtractFiledataFile(m_RPCBindingHandle, fd, pDestFile, &cb) ? true : false;
}

bool RPCVFS::VFSEmbedFiledataDir(const wchar_t *pSrcDir, ptr pDestDir, bool OverWrite, PPROGESSCB pCB){
	RV_FileData fd;
	RV_PROGESSCB cb;
	if (!m_RPCBindingHandle) return false;
	SetPtr(&fd, pDestDir);
	return VFS_RPC_VFSEmbedFiledataDir(m_RPCBindingHandle, pSrcDir, fd, (RV_BOOL)(OverWrite ? TRUE : FALSE), &cb) ? true : false;
}

bool RPCVFS::VFSExtractFiledataDir(ptr pSrcDir, const wchar_t* pDestDir, bool OverWrite, PPROGESSCB pCB){
	RV_FileData fd;
	RV_PROGESSCB cb;
	if (!m_RPCBindingHandle) return false;
	SetPtr(&fd, pSrcDir);

	return VFS_RPC_VFSExtractFiledataDir(m_RPCBindingHandle, fd, pDestDir, (RV_BOOL)(OverWrite ? TRUE : FALSE), &cb) ? true : false;
}


bool RPCVFS::VFSQueryFileInfo(PVFSFILEINFO pFileInfo, ptr pFileData, wchar_t *pDest, int DestSize){
	RV_VFSFILEINFO info;
	RV_FileData fd;
	if (!m_RPCBindingHandle) return false;
	SetPtr(&fd, pFileData);

	if (!VFS_RPC_VFSQueryFileInfo(m_RPCBindingHandle, &info, fd, pDest, DestSize)) return false;

	pFileInfo->m_Flags = (UINT32)info.m_Flags;
	pFileInfo->m_pFileData = GetPtr(&info.m_pFileData);
	pFileInfo->m_pUserData = GetPtr(&info.m_pUserData);
	pFileInfo->m_RefCount = (size_64)info.m_RefCount;
	pFileInfo->m_Size = (size_64)info.m_Size;
	pFileInfo->m_Type = info.m_Type;
	pFileInfo->m_pName = pDest ? pDest + info.m_NameOffset : NULL;
	pFileInfo->m_pFullName = pDest ? pDest + info.m_FullNameOffset : NULL;
	pFileInfo->m_GotMD5 = info.m_GotMD5 ? true : false;
	memcpy(pFileInfo->m_MD5, info.m_MD5, 16);

	return true;
}

bool RPCVFS::VFSSetFileUserData(ptr pFileData, ptr pUserData, size_32 size){
	RV_FileData fd;
	if (!m_RPCBindingHandle) return false;
	SetPtr(&fd, pFileData);
	return VFS_RPC_VFSSetFileUserData(m_RPCBindingHandle, fd, (RV_ptr)pUserData, (RV_size_32)size) ? true : false;
}

bool RPCVFS::VFSGetFileUserData(ptr pFileData, ptr pUserData, size_32 size){
	RV_FileData fd;
	if (!m_RPCBindingHandle) return false;
	SetPtr(&fd, pFileData);
	return VFS_RPC_VFSGetFileUserData(m_RPCBindingHandle, fd, (RV_ptr)pUserData, (RV_size_32)size) ? true : false;
}

void RPCVFS::VFSSetRuntimeUserData(ptr pFileData, ptr pUserData){
	RV_Cookie cookie;
	RV_FileData fd;
	if (!m_RPCBindingHandle) return;
	SetPtr(&cookie, pUserData);
	SetPtr(&fd, pFileData);
	VFS_RPC_VFSSetRuntimeUserData(m_RPCBindingHandle, fd, cookie);
}

ptr RPCVFS::VFSGetRuntimeUserData(ptr pFileData){
	RV_Cookie cookie;
	RV_FileData fd;
	if (!m_RPCBindingHandle) return NULL;
	SetPtr(&fd, pFileData);
	VFS_RPC_VFSGetRuntimeUserData(m_RPCBindingHandle, fd, &cookie);
	return GetPtr(&cookie);
}

ptr RPCVFS::VFSGetRootFiledata(){
	RV_FileData fd;
	if (!m_RPCBindingHandle) return NULL;
	VFS_RPC_VFSGetRootFiledata(m_RPCBindingHandle, &fd);
	return GetPtr(&fd);
}

ptr RPCVFS::VFSGetFileData(const wchar_t* pPath){
	RV_FileData fd;
	if (!m_RPCBindingHandle) return NULL;
	VFS_RPC_VFSGetFileData(m_RPCBindingHandle, pPath, &fd);
	
	return GetPtr(&fd);
}

bool RPCVFS::VFSFileExists(const wchar_t* pFile){
	if (!m_RPCBindingHandle) return false;
	return VFS_RPC_VFSFileExists(m_RPCBindingHandle, pFile) ? true : false;
}

bool RPCVFS::VFSPathExists(const wchar_t* pPath){
	if (!m_RPCBindingHandle) return false;
	return VFS_RPC_VFSPathExists(m_RPCBindingHandle, pPath) ? true : false;
}

size_64 RPCVFS::VFSFileSize(const wchar_t* pPath){
	RV_size_64 size;
	if (!m_RPCBindingHandle) return 0;
	VFS_RPC_VFSFileSize(m_RPCBindingHandle, pPath, &size);
	return (size_64)size;
}


int RPCVFS::VFSSetNoEnum(ptr pFileData, int noenum){
	int _noenum;
	RV_FileData data;
	if (!m_RPCBindingHandle) return 0;
	SetPtr(&data, pFileData);
	VFS_RPC_VFSSetNoEnum(m_RPCBindingHandle, data, noenum, &_noenum);
	return _noenum;
}

int RPCVFS::VFSGetNoEnum(ptr pFileData){
	int noenum;
	RV_FileData data;
	if (!m_RPCBindingHandle) return 0;
	SetPtr(&data, pFileData);
	VFS_RPC_VFSGetNoEnum(m_RPCBindingHandle, data, &noenum);
	return noenum;
}

void RPCVFS::VFSSetFileOption(VFSFileHandle hFile, int fileoption, ptr pData, int size){
	RV_VFSFileHandle _hFile;
	if (!m_RPCBindingHandle) return;
	SetPtr(&_hFile, hFile);
	VFS_RPC_VFSSetFileOption(m_RPCBindingHandle, _hFile, fileoption, (RV_ptr)pData, size);
}

size_64 RPCVFS::VFSGetFileUID(const wchar_t* pPath){
	RV_size_64 UID;
	if (!m_RPCBindingHandle) return INVALID_UID;
	VFS_RPC_VFSGetFileUID(m_RPCBindingHandle, pPath, &UID);
	return (size_64)UID;
}

size_64 RPCVFS::VFSGetDirectoryUID(const wchar_t* pPath){
	RV_size_64 UID;
	if (!m_RPCBindingHandle) return INVALID_UID;
	VFS_RPC_VFSGetDirectoryUID(m_RPCBindingHandle, pPath, &UID);
	return (size_64)UID;
}

size_64 RPCVFS::VFSGetEntryUID(const wchar_t* pPath){
	RV_size_64 UID;
	if (!m_RPCBindingHandle) return INVALID_UID;
	VFS_RPC_VFSGetEntryUID(m_RPCBindingHandle, pPath, &UID);
	return (size_64)UID;
}

size_64 RPCVFS::VFSGetFiledataUID(ptr pFileData){
	return INVALID_UID;
}

bool RPCVFS::VFSDeleteFile(const wchar_t *pFile){
	if (!m_RPCBindingHandle) return false;
	return VFS_RPC_VFSDeleteFile(m_RPCBindingHandle, pFile) ? true : false;
}

bool RPCVFS::VFSDeleteDirectory(const wchar_t *pDir){
	if (!m_RPCBindingHandle) return false;
	return VFS_RPC_VFSDeleteDirectory(m_RPCBindingHandle, pDir) ? true : false;
}


int RPCVFS::VFSGetSettings(){
	int settings;
	if (!m_RPCBindingHandle) return 0;
	VFS_RPC_VFSGetSettings(m_RPCBindingHandle, &settings);

	return settings;
}

size_32 RPCVFS::VFSGetClusterSize(){
	RV_size_32 size;
	if (!m_RPCBindingHandle) return 0;
	VFS_RPC_VFSGetClusterSize(m_RPCBindingHandle, &size);
	return (size_32)size;
}

void RPCVFS::DumpFTInfo(){
	if (!m_RPCBindingHandle) return;
	VFS_RPC_DumpFTInfo(m_RPCBindingHandle);
}

const wchar_t* RPCVFS::TranslateErrorCode(int error){
	return L"<<< ABC >>>";//TranslateVFSErrorCode(error);
}


bool RPCVFS::VFSGetMD5(const wchar_t* pFile, unsigned char *pMD5, bool Store){
	RV_byte md5[16];
	RV_BOOL res;
	if (!m_RPCBindingHandle) return false;
	res = VFS_RPC_VFSGetMD5(m_RPCBindingHandle, pFile, md5, (RV_BOOL)(Store ? TRUE : FALSE));
	memcpy(pMD5, md5, 16);
	return (BOOL)res == TRUE ? true : false;
}

bool RPCVFS::VFSGetFileDataMD5(ptr pFileData, unsigned char *pMD5, bool Store){
	RV_byte md5[16];
	RV_BOOL res;
	RV_FileData fd;
	if (!m_RPCBindingHandle) return false;
	SetPtr(&fd, pFileData);
	res = VFS_RPC_VFSGetFileDataMD5(m_RPCBindingHandle, fd, md5, (RV_BOOL)(Store ? TRUE : FALSE));
	memcpy(pMD5, md5, 16);
	return (BOOL)res == TRUE ? true : false;
}

bool RPCVFS::VFSQueryMD5(const wchar_t* pFile, unsigned char *pMD5){
	RV_byte md5[16];
	RV_BOOL res;
	if (!m_RPCBindingHandle) return false;
	res = VFS_RPC_VFSQueryMD5(m_RPCBindingHandle, pFile, md5);
	memcpy(pMD5, md5, 16);
	return (BOOL)res == TRUE ? true : false;
}

bool RPCVFS::VFSQueryFileDataMD5(ptr pFileData, unsigned char *pMD5){
	RV_byte md5[16];
	RV_BOOL res;
	RV_FileData fd;
	if (!m_RPCBindingHandle) return false;
	SetPtr(&fd, pFileData);
	res = VFS_RPC_VFSQueryFileDataMD5(m_RPCBindingHandle, fd, md5);
	memcpy(pMD5, md5, 16);
	return (BOOL)res == TRUE ? true : false;
}

IStream* RPCVFS::VFSCreateIStreamFile(const wchar_t* pName, UINT32 Mode){
	return NULL; //Not yet supported
}

IStream* RPCVFS::VFSCreateIStreamFiledataFile(ptr pFiledata, UINT32 Mode){
	return NULL; //Not yet supported
}

bool RPCVFS::VFSFileDataFileExists(ptr pFileData, const wchar_t* pFile){
	return false;
}

bool RPCVFS::VFSFileDataPathExists(ptr pFileData, const wchar_t* pPath){
	return false;
}

bool RPCVFS::VFSRenameFile(const wchar_t* pFileName, const wchar_t* pNewName){
	return false;
}

bool RPCVFS::VFSRenameFolder(const wchar_t* pPathName, const wchar_t* pNewName){
	return false;
}

bool RPCVFS::VFSRenameFiledataFile(ptr pFileData, const wchar_t* pNewName){
	return false;
}

bool RPCVFS::VFSRenameFiledataFolder(ptr pFileData, const wchar_t* pNewName){
	return false;
}

ptr RPCVFS::VFSCreateDirectory(ptr pFiledata, const wchar_t* pBaseName, wchar_t *pNewName, size_32 NewNameSize){
	return NULL;
}

bool RPCVFS::VFSRegisterForProtocol(const wchar_t* pDomain){
	return false;
}

void RPCVFS::VFSUnregisterForProtocol(){
}

const wchar_t* RPCVFS::VFSGetDomain(){
	return NULL;
}

ptr RPCVFS::VFSFileDataFromFilehandle(VFSFileHandle hFile){
	return NULL;
}

bool RPCVFS::VFSMoveFiledataEntry(ptr pEntry, ptr pNewDir){
	return false;
}

bool RPCVFS::VFSMoveEntry(const wchar_t *pPath, ptr pNewDir){
	return false;
}
