#ifndef _VFSMODULE_H_
#define _VFSMODULE_H_


#ifdef _M_AMD64
#define VFS_MODULE_NAME		(L"VFSx64.dll")
#else //_M_AMD64
#define VFS_MODULE_NAME		(L"VFSWin32.dll")
#endif //else _M_AMD64

#ifdef _M_AMD64
#define RPC_VFS_MODULE_NAME	(L"RPC VFSx64.dll")
#else //_M_AMD64
#define RPC_VFS_MODULE_NAME	(L"RPC VFSWin32.dll")
#endif //else _M_AMD64


#endif //!_VFSMODULE_H_
