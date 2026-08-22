//#include "RPC VFS_h.h"

#include <windows.h>
#include "main.h"
#include "../VFS/VFS.h"
#include "../VFS/API/VFSBase.h"
#include "../VFS/API/VFSAdvanced.h"
#include "RPC VFS_h.h"

RV_BOOL VFS_RPC_GetVersion( 
    /* [in] */ handle_t IDL_handle,
	/* [out][in] */ int *pVersion){
		*pVersion = g_pVFS->GetVersion();
		return (RV_BOOL)TRUE;
}


RV_BOOL VFS_RPC_PartialFlush( 
    /* [in] */ handle_t IDL_handle){
		g_pVFS->PartialFlush();
		return (RV_BOOL)TRUE;
}

RV_BOOL VFS_RPC_FullFlush( 
    /* [in] */ handle_t IDL_handle){
		g_pVFS->FullFlush();
		return (RV_BOOL)TRUE;
}

RV_BOOL VFS_RPC_Open( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pFilename,
    /* [in] */ RV_size_32 UserVersion,
    /* [in] */ RV_BOOL ReadOnly){
		return (RV_BOOL)(g_pVFS->Open(pFilename, UserVersion, ReadOnly ? true : false) ? TRUE : FALSE);
}

RV_BOOL VFS_RPC_Create( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pFilename,
    /* [in] */ RV_size_32 UserVersion,
    /* [in] */ RV_size_32 ClusterSize){
		return (RV_BOOL)(g_pVFS->Create(pFilename, UserVersion, ClusterSize) ? TRUE : FALSE);;
}

RV_BOOL VFS_RPC_Close( 
    /* [in] */ handle_t IDL_handle){
		g_pVFS->Close();
		return (RV_BOOL)TRUE;
}

RV_BOOL VFS_RPC_GetLastError( 
    /* [in] */ handle_t IDL_handle,
    int *pResult){
		*pResult = g_pVFS->GetLastError();
		
		return (RV_BOOL)TRUE;
}

RV_BOOL VFS_RPC_VFSCreateFile( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pName,
    /* [in] */ RV_UINT32 Mode,
    /* [out][in] */ RV_VFSFileHandle *phFile){
		SetPtr(phFile,
			g_pVFS->VFSCreateFile(pName, (UINT32)Mode)
			);
		return (RV_BOOL)(GetPtr(phFile) ? TRUE : FALSE);
}

RV_BOOL VFS_RPC_VFSCreateFiledataFile( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_FileData pFiledata,
    /* [in] */ RV_UINT32 Mode,
    /* [out][in] */ RV_VFSFileHandle *phFile){
		
		SetPtr(phFile,
			g_pVFS->VFSCreateFiledataFile(GetPtr(&pFiledata), (UINT32)Mode)
			);
		
		return (RV_BOOL)(GetPtr(phFile) ? TRUE : FALSE);
}

RV_BOOL VFS_RPC_VFSCreateUIDFile( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_size_64 UID,
    /* [in] */ RV_UINT32 Mode,
    /* [out][in] */ RV_VFSFileHandle *phFile){
		SetPtr(phFile,
			g_pVFS->VFSCreateUIDFile((size_64)UID, (UINT32)Mode)
			);
		
		return (RV_BOOL)(GetPtr(phFile) ? TRUE : FALSE);
}

RV_BOOL VFS_RPC_VFSCloseFile( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_VFSFileHandle hFile){
		g_pVFS->VFSCloseFile((VFSFileHandle)GetPtr(&hFile));
		return (RV_BOOL)TRUE;
}

RV_BOOL VFS_RPC_VFSRead( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_VFSFileHandle hFile,
    /* [size_is][in] */ RV_ptr pBuffer,
    /* [in] */ RV_size_32 Count,
    /* [in] */ RV_PPROGESSCB pCB,
    /* [out] */ RV_size_32 *pRead){
		*pRead = (RV_size_32)g_pVFS->VFSRead((VFSFileHandle)GetPtr(&hFile), (void*)pBuffer, Count, NULL);
		return (RV_BOOL)(*pRead == Count ? TRUE : FALSE);
}

RV_BOOL VFS_RPC_VFSWrite( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_VFSFileHandle hFile,
    /* [size_is][in] */ const RV_ptr pBuffer,
    /* [in] */ RV_size_32 Count,
    /* [in] */ RV_PPROGESSCB pCB,
    /* [out] */ RV_size_32 *pWritten){
		*pWritten = (RV_size_32)g_pVFS->VFSWrite((VFSFileHandle)GetPtr(&hFile), (void*)pBuffer, (size_32)Count, NULL);
		return (RV_BOOL)(*pWritten == Count ? TRUE : FALSE);
}

RV_BOOL VFS_RPC_VFSSeek( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_VFSFileHandle hFile,
    /* [in] */ RV_ssize_64 Offset,
    /* [in] */ int Origin,
    /* [out] */ RV_size_64 *pNewOffset){

		*pNewOffset = (RV_size_64)g_pVFS->VFSSeek((VFSFileHandle)GetPtr(&hFile), (ssize_64)Offset, Origin);
		return (RV_BOOL)TRUE;
}

RV_BOOL VFS_RPC_VFSTell( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_VFSFileHandle hFile,
    /* [out] */ RV_size_64 *pOffset){
		*pOffset = (RV_size_64)g_pVFS->VFSTell((VFSFileHandle)GetPtr(&hFile));
		return (RV_BOOL)TRUE;
}

RV_BOOL VFS_RPC_VFSSize( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_VFSFileHandle hFile,
    /* [out] */ RV_size_64 *pSize){
		*pSize = (RV_size_64)g_pVFS->VFSSize((VFSFileHandle)GetPtr(&hFile));
		return (RV_BOOL)TRUE;
}

RV_BOOL VFS_RPC_VFSEndOfFile( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_VFSFileHandle hFile){
		return (RV_BOOL)(g_pVFS->VFSEndOfFile((VFSFileHandle)GetPtr(&hFile)) ? TRUE : FALSE);
}

RV_BOOL VFS_RPC_VFSSetEndOfFile( 
    /* [in] */ handle_t IDL_handle,
	/* [in] */ RV_VFSFileHandle hFile){
		return (RV_BOOL)(g_pVFS->VFSSetEndOfFile((VFSFileHandle)GetPtr(&hFile)) ? TRUE : FALSE);
}

RV_BOOL VFS_RPC_VFSMemCache( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_VFSFileHandle hFile,
    /* [out] */ RV_ptr *pCache){
		//Currently not supported
		*pCache = NULL;
		return (RV_BOOL)TRUE;
}

RV_BOOL VFS_RPC_VFSGetUIDFromFileHandle( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_VFSFileHandle hFile,
    /* [out] */ RV_size_64 *pUID){
		*pUID = (RV_size_64)g_pVFS->VFSGetUIDFromFileHandle((VFSFileHandle)GetPtr(&hFile));
		return (RV_BOOL)TRUE;
}

RV_BOOL VFS_RPC_VFSDeleteFiledataDirectory( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_FileData pDir){
		
		return (RV_BOOL)(g_pVFS->VFSDeleteFiledataDirectory((VFSFileHandle)GetPtr(&pDir)) ? TRUE : FALSE);
}

RV_BOOL VFS_RPC_VFSDeleteFiledataFile( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_FileData pFile){
		return (RV_BOOL)(g_pVFS->VFSDeleteFiledataFile((VFSFileHandle)GetPtr(&pFile)) ? TRUE : FALSE);
}

RV_BOOL VFS_RPC_VFSEnum( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pDirectory,
    /* [in] */ RV_pfEnumCallback pCB,
    /* [in] */ RV_FileData wParam,
    /* [in] */ RV_FileData lParam,
    /* [in] */ int what,
    /* [out] */ int *pResult){
		//Currently not supported
		return (RV_BOOL)TRUE;
}

RV_BOOL VFS_RPC_VFSSingleEnum( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_FileData pFileData,
    /* [in] */ RV_pfEnumCallback pCB,
    /* [in] */ RV_FileData wParam,
    /* [in] */ RV_FileData lParam,
    /* [out] */ int *pResult){
		//Currently not supported
		return (RV_BOOL)TRUE;
}

RV_BOOL VFS_RPC_VFSEmbedFile( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pSrcFile,
    /* [in] */ const wchar_t *pDestFile,
    /* [in] */ RV_BOOL OverWrite,
    /* [in] */ RV_PPROGESSCB pCB){
		return (RV_BOOL)(g_pVFS->VFSEmbedFile(pSrcFile, pDestFile, OverWrite ? true : false, NULL) ? TRUE : FALSE);
}

RV_BOOL VFS_RPC_VFSExtractFile( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pSrcFile,
    /* [string][in] */ const wchar_t *pDestFile,
    RV_PPROGESSCB pCB){
		return (RV_BOOL)(g_pVFS->VFSExtractFile(pSrcFile, pDestFile, NULL) ? TRUE : FALSE);
}

RV_BOOL VFS_RPC_VFSEmbedDir( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pSrcDir,
    /* [string][in] */ const wchar_t *pDestDir,
    /* [in] */ RV_BOOL OverWrite,
    /* [in] */ RV_PPROGESSCB pCB){
		return (RV_BOOL)(g_pVFS->VFSEmbedDir(pSrcDir, pDestDir, OverWrite ? true : false, NULL) ? TRUE : FALSE);
}

RV_BOOL VFS_RPC_VFSExtractDir( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pSrcDir,
    /* [string][in] */ const wchar_t *pDestDir,
    /* [in] */ RV_BOOL OverWrite,
    /* [in] */ RV_PPROGESSCB pCB){
		return (RV_BOOL)(g_pVFS->VFSExtractDir(pSrcDir, pDestDir, OverWrite ? true : false, NULL) ? TRUE : FALSE);
}

RV_BOOL VFS_RPC_VFSEmbedFiledataFile( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pSrcFile,
    /* [in] */ RV_FileData pDestDir,
    /* [in] */ RV_BOOL OverWrite,
    /* [in] */ RV_PPROGESSCB pCB){
		return (RV_BOOL)(g_pVFS->VFSEmbedFiledataFile(pSrcFile, (ptr)GetPtr(&pDestDir), OverWrite ? true : false, NULL) ? TRUE : FALSE);
}

RV_BOOL VFS_RPC_VFSExtractFiledataFile( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_FileData pSrcFile,
    /* [string][in] */ const wchar_t *pDestFile,
    /* [in] */ RV_PPROGESSCB pCB){
		return (RV_BOOL)(g_pVFS->VFSExtractFiledataFile((ptr)GetPtr(&pSrcFile), pDestFile, NULL) ? TRUE : FALSE);
}

RV_BOOL VFS_RPC_VFSEmbedFiledataDir( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pSrcDir,
    /* [in] */ RV_FileData pDestDir,
    /* [in] */ RV_BOOL OverWrite,
    /* [in] */ RV_PPROGESSCB pCB){
		return (RV_BOOL)(g_pVFS->VFSEmbedFiledataDir(pSrcDir, (ptr)GetPtr(&pDestDir), OverWrite ? true : false, NULL) ? TRUE : FALSE);
}

RV_BOOL VFS_RPC_VFSExtractFiledataDir( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_FileData pSrcDir,
    /* [string][in] */ const wchar_t *pDestDir,
    /* [in] */ RV_BOOL OverWrite,
    /* [in] */ RV_PPROGESSCB pCB){
		return (RV_BOOL)(g_pVFS->VFSExtractFiledataDir((ptr)GetPtr(&pSrcDir), pDestDir, OverWrite ? true : false, NULL) ? TRUE : FALSE);
}

RV_BOOL VFS_RPC_VFSQueryFileInfo( 
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ RV_PVFSFILEINFO pFileInfo,
    /* [in] */ RV_FileData pFileData,
    /* [size_is][string][out][in] */ wchar_t *pDest,
    /* [in] */ int DestSize){
		VFSFILEINFO info;
		if (!g_pVFS->VFSQueryFileInfo(&info, (ptr)GetPtr(&pFileData), pDest, DestSize)) return (RV_BOOL)FALSE;

		pFileInfo->m_Type = info.m_Type;
		pFileInfo->m_Size = (RV_size_64)info.m_Size;
		SetPtr(&pFileInfo->m_pFileData, info.m_pFileData);
		SetPtr(&pFileInfo->m_pUserData, info.m_pUserData);
		pFileInfo->m_Flags = (RV_UINT32)info.m_Flags;
		pFileInfo->m_UID = (RV_size_64)info.m_UID;
		pFileInfo->m_RefCount = (RV_size_64)info.m_RefCount;

		pFileInfo->m_NameOffset = pDest-info.m_pName;
		pFileInfo->m_FullNameOffset = pDest-info.m_pFullName;
		pFileInfo->m_GotMD5 = (RV_BOOL)(info.m_GotMD5 ? TRUE : FALSE);
		memcpy(pFileInfo->m_MD5, info.m_MD5, 16);

		return (RV_BOOL)TRUE;
}

RV_BOOL VFS_RPC_VFSSetFileUserData( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_FileData pFileData,
    /* [in] */ RV_ptr pUserData,
	/* [in] */ RV_size_32 size){
		g_pVFS->VFSSetFileUserData((ptr)GetPtr(&pFileData), (ptr)pUserData, (size_32)size);
		return (RV_BOOL)TRUE;
}

RV_BOOL VFS_RPC_VFSGetFileUserData( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_FileData pFileData,
    /* [out][in] */ RV_ptr pUserData,
	/* [in] */ RV_size_32 size){
		g_pVFS->VFSGetFileUserData((ptr)GetPtr(&pFileData), (ptr)pUserData, (size_32)size);
		return (RV_BOOL)TRUE;
}

RV_BOOL VFS_RPC_VFSSetRuntimeUserData( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_FileData pFileData,
	/* [in] */ RV_Cookie pUserData){
		g_pVFS->VFSSetRuntimeUserData((ptr)GetPtr(&pFileData), (ptr)GetPtr(pUserData));
		return (RV_BOOL)TRUE;
}

RV_BOOL VFS_RPC_VFSGetRuntimeUserData( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_FileData pFileData,
	/* [out][in] */ RV_Cookie *pUserData){
		SetPtr(pUserData, g_pVFS->VFSGetRuntimeUserData((ptr)GetPtr(&pFileData)));
		return (RV_BOOL)TRUE;


}

RV_BOOL VFS_RPC_VFSGetRootFiledata(
    /* [in] */ handle_t IDL_handle,
    /* [out] */ RV_FileData *ppFileData){
		SetPtr(ppFileData, g_pVFS->VFSGetRootFiledata());
		return (RV_BOOL)TRUE;
}

RV_BOOL VFS_RPC_VFSGetFileData( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pPath,
    /* [out] */ RV_FileData *ppFileData){
		return (RV_BOOL)TRUE;
}

RV_BOOL VFS_RPC_VFSFileExists( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pFile){
		return (RV_BOOL)(g_pVFS->VFSFileExists(pFile) ? TRUE : FALSE);
}

RV_BOOL VFS_RPC_VFSPathExists( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pPath){
		return (RV_BOOL)(g_pVFS->VFSPathExists(pPath) ? TRUE : FALSE);
}

RV_BOOL VFS_RPC_VFSFileSize( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pPath,
    /* [out] */ RV_size_64 *pSize){
		*pSize = g_pVFS->VFSFileSize(pPath);
		return (RV_BOOL)TRUE;
}

RV_BOOL VFS_RPC_VFSSetNoEnum( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_FileData pFileData,
    /* [in] */ int noenum,
    /* [out] */ int *pOldValue){
		*pOldValue = g_pVFS->VFSSetNoEnum(GetPtr(&pFileData), noenum);
		return (RV_BOOL)TRUE;
}

RV_BOOL VFS_RPC_VFSGetNoEnum( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_FileData pFileData,
    /* [out] */ int *pValue){
		*pValue = g_pVFS->VFSGetNoEnum(GetPtr(&pFileData));
		return (RV_BOOL)TRUE;
}

RV_BOOL VFS_RPC_VFSSetFileOption( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_VFSFileHandle hFile,
    /* [in] */ int fileoption,
    /* [in] */ RV_ptr pData,
    /* [in] */ int size){
		g_pVFS->VFSSetFileOption(GetPtr(hFile), fileoption, (ptr)pData, size);
		return (RV_BOOL)TRUE;
}

RV_BOOL VFS_RPC_VFSGetFileUID( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pPath,
    /* [out] */ RV_size_64 *pUID){
		*pUID = (RV_size_64)g_pVFS->VFSGetFileUID(pPath);
		return (RV_BOOL)TRUE;
}

RV_BOOL VFS_RPC_VFSGetDirectoryUID( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pPath,
    /* [out] */ RV_size_64 *pUID){
		*pUID = (RV_size_64)g_pVFS->VFSGetDirectoryUID(pPath);
		return (RV_BOOL)TRUE;
}

RV_BOOL VFS_RPC_VFSGetEntryUID( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pPath,
    /* [out] */ RV_size_64 *pUID){
		*pUID = (RV_size_64)g_pVFS->VFSGetEntryUID(pPath);
		return (RV_BOOL)TRUE;
}

RV_BOOL VFS_RPC_VFSDeleteFile( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pFile){
		return (RV_BOOL)(g_pVFS->VFSDeleteFile(pFile) ? TRUE : FALSE);
}

RV_BOOL VFS_RPC_VFSDeleteDirectory( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pDir){
		return (RV_BOOL)(g_pVFS->VFSDeleteDirectory(pDir) ? TRUE : FALSE);
}

RV_BOOL VFS_RPC_VFSGetSettings( 
    /* [in] */ handle_t IDL_handle,
    /* [out] */ int *pSettings){
		*pSettings = g_pVFS->VFSGetSettings();
		return (RV_BOOL)TRUE;
}


RV_BOOL VFS_RPC_VFSGetClusterSize( 
    /* [in] */ handle_t IDL_handle,
	/* [out] */ RV_size_32 *pClusterSize){
		*pClusterSize = (RV_size_32)g_pVFS->VFSGetClusterSize();
		return TRUE;
}


RV_BOOL VFS_RPC_DumpFTInfo( 
    /* [in] */ handle_t IDL_handle){
		g_pVFS->DumpFTInfo();
		return (RV_BOOL)TRUE;
}

RV_BOOL VFS_RPC_TranslateErrorCode( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ int error,
    /* [string][out] */ wchar_t **pError){
		return (RV_BOOL)FALSE;
}

RV_BOOL VFS_RPC_VFSGetMD5( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pFile,
    /* [out] */ RV_byte pMD5[ 16 ],
	/* [in] */ RV_BOOL Store){
		return (RV_BOOL)(g_pVFS->VFSGetMD5(pFile, (unsigned char*)pMD5, Store ? true : false) ? TRUE : FALSE);
}

RV_BOOL VFS_RPC_VFSGetFileDataMD5( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_FileData pFileData,
    /* [out] */ RV_byte pMD5[ 16 ],
	/* [in] */ RV_BOOL Store){
		return (RV_BOOL)(g_pVFS->VFSGetFileDataMD5(GetPtr(&pFileData), (unsigned char*)pMD5, Store ? true : false) ? TRUE : FALSE);
}

RV_BOOL VFS_RPC_VFSQueryMD5( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pFile,
	/* [out] */ RV_byte pMD5[ 16 ]){
		return (RV_BOOL)(g_pVFS->VFSQueryMD5(pFile, (unsigned char*)pMD5) ? TRUE : FALSE);
}

RV_BOOL VFS_RPC_VFSQueryFileDataMD5( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_FileData pFileData,
	/* [out] */ RV_byte pMD5[ 16 ]){
		return (RV_BOOL)(g_pVFS->VFSQueryFileDataMD5(GetPtr(&pFileData), (unsigned char*)pMD5) ? TRUE : FALSE);
}


void __RPC_USER RV_VFSFileHandle_rundown(RV_VFSFileHandle hFile){
}

void __RPC_USER RV_FileData_rundown(RV_FileData hFileData){
}
