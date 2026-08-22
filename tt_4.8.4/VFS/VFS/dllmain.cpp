#include <windows.h>
#include "../Shared/memory.h"
#include "VFS.h"
#include "VFSStructures.h"
#include "API/VFSVersion.h"
#include "InetProtocol.h"

#include "dllmain.h"

CProtocolClassFactory	g_CF;
bool					g_Registered = false;

CVFSCoreInterface::CVFSCoreInterface(){
	m_pNameSpace = NULL;
}

CVFSCoreInterface::~CVFSCoreInterface(){
	if (g_Registered){

		GetInternetSession()->UnregisterNameSpace(&g_CF, m_pNameSpace);
		g_pFreeer(m_pNameSpace);
	}

}

IVFS* CVFSCoreInterface::CreateVFS(){
	return new VFS;
}

IVFS2* CVFSCoreInterface::CreateVFS2(){
	return NULL;
}

IVFS* CVFSCoreInterface::CreateRPCVFS(){
	return NULL;
}

void CVFSCoreInterface::RegisterProtocol(const wchar_t* pName){
	m_pNameSpace = StrDupW(pName);
	GetInternetSession()->RegisterNameSpace(&g_CF, CLSID_NULL, pName, 0, NULL, 0);
	g_Registered = true;
}

void CVFSCoreInterface::RegisterForProtocol(IVFS* pVFS){
	if (FindVFSForDomain(pVFS->VFSGetDomain())) return;
	m_VFSList.Add_Tail(pVFS);
}

void CVFSCoreInterface::UnregisterForProtocol(IVFS* pVFS){
	m_VFSList.Remove(pVFS);
}

IVFS* CVFSCoreInterface::FindVFSForDomain(const wchar_t* pDomain){
	GenericSLNode<IVFS> *pNode;
	pNode = m_VFSList.Get_Head();

	while (pNode){
		if (wcscmp(pNode->m_pData->VFSGetDomain(), pDomain) == 0)
			return pNode->m_pData;
		pNode = pNode->m_pNext;
	}

	return NULL;
}

CVFSCoreInterface	g_CVFSCoreInterface;

#ifdef _USRDLL
extern "C" IVFS *VFS_CALL CreateNewInstance(){
	return new VFS;
}
extern "C" IVFS *VFS_CALL RPCCreateNewInstance(){
	return NULL;
}
#endif //_USRDLL

extern "C" IVFSCoreInterface* VFS_CALL QueryCoreInterface(){
	return &g_CVFSCoreInterface;
}

extern "C" IVFSCoreInterface* VFS_CALL RPCQueryCoreInterface(){
	return NULL;
}

bool VFS_CALL VFSCreateInterface(int version, void **ppOutIf){
	IVFS* pVFS;
	if (!ppOutIf) return false;
	switch (version){
		case VFS_INTERFACE_FLAT:
			pVFS = new VFS;
			if (!pVFS){
				SetLastError(ERROR_OUTOFMEMORY);
				return false;
			}
			*ppOutIf = pVFS;
			SetLastError(ERROR_SUCCESS);
			return true;
			break;
			
		/*case VFS_INTERFACE_FLAT_V2:
			pVFS = new VFS2;
			if (!pVFS){
				SetLastError(ERROR_OUTOFMEMORY);
				return false;
			}
			*ppOutIf = pVFS;
			SetLastError(ERROR_SUCCESS);
			return true;
			break;//*/

		case VFS_INTERFACE_RPC:
		default:
			SetLastError(ERROR_INVALID_FLAGS);
			return false;

	}
	return false;
}

extern "C" size_32 VFS_CALL GetVFSVersion(){
	return VFS_VERSION;
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
