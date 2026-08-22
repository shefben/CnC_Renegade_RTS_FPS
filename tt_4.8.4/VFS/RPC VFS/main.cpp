#include <windows.h>
#include "../Shared/Memory.h"
#include "../Shared/Lists.h"
#include "../Shared/BaseTypes.h"
#include "../Shared/StdLib.h"
#include "../VFS/API/VFSErrorcodes.h"
#include "../VFS/API/VFSInterface.h"
#include "../VFS/API/VFSVersion.h"
#include "RPC VFS.h"


#ifdef _USRDLL
extern "C" IVFS *VFS_CALL CreateNewInstance(){
	return NULL;
}
extern "C" IVFS *VFS_CALL RPCCreateNewInstance(){
	return new RPCVFS();
}
#endif //_USRDLL

extern "C" size_32 VFS_CALL GetVFSVersion(){
	return VFS_VERSION;
}

bool VFS_CALL VFSCreateInterface(int version, void **ppOutIf){
	IVFS* pVFS;
	if (!ppOutIf) return false;
	switch (version){
		case VFS_INTERFACE_RPC:
			pVFS = new RPCVFS;
			if (!pVFS){
				SetLastError(ERROR_OUTOFMEMORY);
				return false;
			}
			*ppOutIf = pVFS;
			SetLastError(ERROR_SUCCESS);
			return true;
			break;

		case VFS_INTERFACE_FLAT:
		case VFS_INTERFACE_FLAT_V2:
		default:
			SetLastError(ERROR_INVALID_FLAGS);
			return false;

	}
	return false;
}

int __declspec(dllexport) __stdcall DllMain(HMODULE hModule, DWORD ul_reason_for_call, void* lpReserved) {
	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
			SetDefaultMemoryFunctions();
			break;
		case DLL_PROCESS_DETACH:
			break;
	}
	return 1;
}

void __RPC_FAR * __RPC_API midl_user_allocate(size_t nLen){
	return (g_pAllocator(nLen));
}

void __RPC_API midl_user_free(void __RPC_FAR * lpvPointer){
    if(NULL != lpvPointer)
		g_pFreeer(lpvPointer);
}
