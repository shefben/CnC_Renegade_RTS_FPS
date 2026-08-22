

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0500 */
/* at Mon Aug 10 07:30:17 2009
 */
/* Compiler settings for ..\RPC VFS\RPC VFS.Idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__


#ifndef __RPC_VFS_h_h__
#define __RPC_VFS_h_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __RPC_VFS_INTERFACE_DEFINED__
#define __RPC_VFS_INTERFACE_DEFINED__

/* interface RPC_VFS */
/* [unique][version][uuid] */ 

typedef unsigned __int64 RV_UINT64;

typedef unsigned int RV_UINT32;

typedef unsigned short RV_UINT16;

typedef __int64 RV_INT64;

typedef int RV_INT32;

typedef short RV_INT16;

typedef RV_UINT64 RV_size_64;

typedef RV_UINT32 RV_size_32;

typedef RV_UINT16 RV_size_16;

typedef RV_INT64 RV_ssize_64;

typedef RV_INT32 RV_ssize_32;

typedef RV_INT16 RV_ssize_16;

typedef unsigned char *RV_ptr;

typedef int RV_BOOL;

typedef RV_size_64 RV_VFSFileHandle;

typedef RV_size_64 RV_FileData;

typedef RV_size_64 RV_Cookie;

typedef unsigned char RV_byte;

typedef RV_ptr RV_pfEnumCallback;

typedef struct _RV_PROGESSCB
    {
    RV_Cookie m_pCookie;
    RV_BOOL m_UseInternalCache;
    } 	RV_PROGESSCB;

typedef struct _RV_PROGESSCB *RV_PPROGESSCB;

typedef struct _RV_VFSFILEINFO
    {
    RV_size_64 m_NameOffset;
    RV_size_64 m_FullNameOffset;
    unsigned char m_Type;
    RV_size_64 m_Size;
    RV_FileData m_pFileData;
    RV_Cookie m_pUserData;
    RV_UINT32 m_Flags;
    RV_size_64 m_UID;
    RV_size_64 m_RefCount;
    RV_BOOL m_GotMD5;
    RV_byte m_MD5[ 16 ];
    } 	RV_VFSFILEINFO;

typedef struct _RV_VFSFILEINFO *RV_PVFSFILEINFO;

RV_BOOL VFS_RPC_GetVersion( 
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ int *pVersion);

RV_BOOL VFS_RPC_PartialFlush( 
    /* [in] */ handle_t IDL_handle);

RV_BOOL VFS_RPC_FullFlush( 
    /* [in] */ handle_t IDL_handle);

RV_BOOL VFS_RPC_Open( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pFilename,
    /* [in] */ RV_size_32 UserVersion,
    /* [in] */ RV_BOOL ReadOnly);

RV_BOOL VFS_RPC_Create( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pFilename,
    /* [in] */ RV_size_32 UserVersion,
    /* [in] */ RV_size_32 ClusterSize);

RV_BOOL VFS_RPC_Close( 
    /* [in] */ handle_t IDL_handle);

RV_BOOL VFS_RPC_GetLastError( 
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ int *pResult);

RV_BOOL VFS_RPC_VFSCreateFile( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pName,
    /* [in] */ RV_UINT32 Mode,
    /* [out][in] */ RV_VFSFileHandle *phFile);

RV_BOOL VFS_RPC_VFSCreateFiledataFile( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_FileData pFiledata,
    /* [in] */ RV_UINT32 Mode,
    /* [out][in] */ RV_VFSFileHandle *phFile);

RV_BOOL VFS_RPC_VFSCreateUIDFile( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_size_64 UID,
    /* [in] */ RV_UINT32 Mode,
    /* [out][in] */ RV_VFSFileHandle *phFile);

RV_BOOL VFS_RPC_VFSCloseFile( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_VFSFileHandle hFile);

RV_BOOL VFS_RPC_VFSRead( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_VFSFileHandle hFile,
    /* [size_is][out] */ RV_ptr pBuffer,
    /* [in] */ RV_size_32 Count,
    /* [in] */ RV_PPROGESSCB pCB,
    /* [out] */ RV_size_32 *pRead);

RV_BOOL VFS_RPC_VFSWrite( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_VFSFileHandle hFile,
    /* [size_is][in] */ const RV_ptr pBuffer,
    /* [in] */ RV_size_32 Count,
    /* [in] */ RV_PPROGESSCB pCB,
    /* [out] */ RV_size_32 *pWritten);

RV_BOOL VFS_RPC_VFSSeek( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_VFSFileHandle hFile,
    /* [in] */ RV_ssize_64 Offset,
    /* [in] */ int Origin,
    /* [out] */ RV_size_64 *pNewOffset);

RV_BOOL VFS_RPC_VFSTell( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_VFSFileHandle hFile,
    /* [out] */ RV_size_64 *pOffset);

RV_BOOL VFS_RPC_VFSSize( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_VFSFileHandle hFile,
    /* [out] */ RV_size_64 *pSize);

RV_BOOL VFS_RPC_VFSEndOfFile( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_VFSFileHandle hFile);

RV_BOOL VFS_RPC_VFSSetEndOfFile( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_VFSFileHandle hFile);

RV_BOOL VFS_RPC_VFSMemCache( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_VFSFileHandle hFile,
    /* [out] */ RV_ptr *pCache);

RV_BOOL VFS_RPC_VFSGetUIDFromFileHandle( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_VFSFileHandle hFile,
    /* [out] */ RV_size_64 *pUID);

RV_BOOL VFS_RPC_VFSDeleteFiledataDirectory( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_FileData pDir);

RV_BOOL VFS_RPC_VFSDeleteFiledataFile( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_FileData pFile);

RV_BOOL VFS_RPC_VFSEnum( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pDirectory,
    /* [in] */ RV_pfEnumCallback pCB,
    /* [in] */ RV_FileData wParam,
    /* [in] */ RV_FileData lParam,
    /* [in] */ int what,
    /* [out] */ int *pResult);

RV_BOOL VFS_RPC_VFSSingleEnum( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_FileData pFileData,
    /* [in] */ RV_pfEnumCallback pCB,
    /* [in] */ RV_FileData wParam,
    /* [in] */ RV_FileData lParam,
    /* [out] */ int *pResult);

RV_BOOL VFS_RPC_VFSEmbedFile( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pSrcFile,
    /* [in] */ const wchar_t *pDestFile,
    /* [in] */ RV_BOOL OverWrite,
    /* [in] */ RV_PPROGESSCB pCB);

RV_BOOL VFS_RPC_VFSExtractFile( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pSrcFile,
    /* [string][in] */ const wchar_t *pDestFile,
    RV_PPROGESSCB pCB);

RV_BOOL VFS_RPC_VFSEmbedDir( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pSrcDir,
    /* [string][in] */ const wchar_t *pDestDir,
    /* [in] */ RV_BOOL OverWrite,
    /* [in] */ RV_PPROGESSCB pCB);

RV_BOOL VFS_RPC_VFSExtractDir( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pSrcDir,
    /* [string][in] */ const wchar_t *pDestDir,
    /* [in] */ RV_BOOL OverWrite,
    /* [in] */ RV_PPROGESSCB pCB);

RV_BOOL VFS_RPC_VFSEmbedFiledataFile( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pSrcFile,
    /* [in] */ RV_FileData pDestDir,
    /* [in] */ RV_BOOL OverWrite,
    /* [in] */ RV_PPROGESSCB pCB);

RV_BOOL VFS_RPC_VFSExtractFiledataFile( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_FileData pSrcFile,
    /* [string][in] */ const wchar_t *pDestFile,
    /* [in] */ RV_PPROGESSCB pCB);

RV_BOOL VFS_RPC_VFSEmbedFiledataDir( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pSrcDir,
    /* [in] */ RV_FileData pDestDir,
    /* [in] */ RV_BOOL OverWrite,
    /* [in] */ RV_PPROGESSCB pCB);

RV_BOOL VFS_RPC_VFSExtractFiledataDir( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_FileData pSrcDir,
    /* [string][in] */ const wchar_t *pDestDir,
    /* [in] */ RV_BOOL OverWrite,
    /* [in] */ RV_PPROGESSCB pCB);

RV_BOOL VFS_RPC_VFSQueryFileInfo( 
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ RV_PVFSFILEINFO pFileInfo,
    /* [in] */ RV_FileData pFileData,
    /* [size_is][string][out][in] */ wchar_t *pDest,
    /* [in] */ int DestSize);

RV_BOOL VFS_RPC_VFSSetFileUserData( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_FileData pFileData,
    /* [in] */ RV_ptr pUserData,
    /* [in] */ RV_size_32 size);

RV_BOOL VFS_RPC_VFSGetFileUserData( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_FileData pFileData,
    /* [out][in] */ RV_ptr pUserData,
    /* [in] */ RV_size_32 size);

RV_BOOL VFS_RPC_VFSSetRuntimeUserData( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_FileData pFileData,
    /* [in] */ RV_Cookie pUserData);

RV_BOOL VFS_RPC_VFSGetRuntimeUserData( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_FileData pFileData,
    /* [out][in] */ RV_Cookie *pUserData);

RV_BOOL VFS_RPC_VFSGetRootFiledata( 
    /* [in] */ handle_t IDL_handle,
    /* [out] */ RV_FileData *ppFileData);

RV_BOOL VFS_RPC_VFSGetFileData( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pPath,
    /* [out] */ RV_FileData *ppFileData);

RV_BOOL VFS_RPC_VFSFileExists( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pFile);

RV_BOOL VFS_RPC_VFSPathExists( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pPath);

RV_BOOL VFS_RPC_VFSFileSize( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pPath,
    /* [out] */ RV_size_64 *pSize);

RV_BOOL VFS_RPC_VFSSetNoEnum( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_FileData pFileData,
    /* [in] */ int noenum,
    /* [out] */ int *pOldValue);

RV_BOOL VFS_RPC_VFSGetNoEnum( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_FileData pFileData,
    /* [out] */ int *pValue);

RV_BOOL VFS_RPC_VFSSetFileOption( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_VFSFileHandle hFile,
    /* [in] */ int fileoption,
    /* [in] */ RV_ptr pData,
    /* [in] */ int size);

RV_BOOL VFS_RPC_VFSGetFileUID( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pPath,
    /* [out] */ RV_size_64 *pUID);

RV_BOOL VFS_RPC_VFSGetDirectoryUID( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pPath,
    /* [out] */ RV_size_64 *pUID);

RV_BOOL VFS_RPC_VFSGetEntryUID( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pPath,
    /* [out] */ RV_size_64 *pUID);

RV_BOOL VFS_RPC_VFSDeleteFile( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pFile);

RV_BOOL VFS_RPC_VFSDeleteDirectory( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pDir);

RV_BOOL VFS_RPC_VFSGetSettings( 
    /* [in] */ handle_t IDL_handle,
    /* [out] */ int *pSettings);

RV_BOOL VFS_RPC_VFSGetClusterSize( 
    /* [in] */ handle_t IDL_handle,
    /* [out] */ RV_size_32 *pClusterSize);

RV_BOOL VFS_RPC_DumpFTInfo( 
    /* [in] */ handle_t IDL_handle);

RV_BOOL VFS_RPC_TranslateErrorCode( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ int error,
    /* [string][out] */ wchar_t **pError);

RV_BOOL VFS_RPC_VFSGetMD5( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pFile,
    /* [out] */ RV_byte pMD5[ 16 ],
    /* [in] */ RV_BOOL Store);

RV_BOOL VFS_RPC_VFSGetFileDataMD5( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_FileData pFileData,
    /* [out] */ RV_byte pMD5[ 16 ],
    /* [in] */ RV_BOOL Store);

RV_BOOL VFS_RPC_VFSQueryMD5( 
    /* [in] */ handle_t IDL_handle,
    /* [string][in] */ const wchar_t *pFile,
    /* [out] */ RV_byte pMD5[ 16 ]);

RV_BOOL VFS_RPC_VFSQueryFileDataMD5( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ RV_FileData pFileData,
    /* [out] */ RV_byte pMD5[ 16 ]);



extern RPC_IF_HANDLE RPC_VFS_v1_0_c_ifspec;
extern RPC_IF_HANDLE RPC_VFS_v1_0_s_ifspec;
#endif /* __RPC_VFS_INTERFACE_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


