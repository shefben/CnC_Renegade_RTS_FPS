#ifndef _COREAPI_H_
#define _COREAPI_H_

#include <windows.h>
#include "../Shared/BaseTypes.h"

#define CORE_MODULE_NAME			(L"Core.dll")
#define CORE_INTERFACE_QUERY_NAME	("GetLauncherCore")



interface ILauncherCore {
	virtual bool CheckForUpdate(wchar_t* pBuf, size_32 bufsize, bool *pExec, HINSTANCE hInstance) = 0;
	virtual void CoreEntry(wchar_t* pPatchFileName, HINSTANCE hInstance) = 0;
};

extern "C" ILauncherCore* __stdcall GetLauncherCore();
typedef ILauncherCore* (__stdcall *pfnGetLauncherCore)();



#define CORE_QUERY_COREIF	\
	static HMODULE __s_Module = NULL; \
	static pfnGetLauncherCore __s_pfnGetLauncherCore = NULL; \
	ILauncherCore* LauncherQueryCoreInterface(HMODULE *phModule){ \
		if (!__s_Module) __s_Module = LoadLibraryW(CORE_MODULE_NAME); \
		if (__s_Module) __s_pfnGetLauncherCore = (pfnGetLauncherCore)GetProcAddress(__s_Module, CORE_INTERFACE_QUERY_NAME); \
		if (phModule) *phModule = __s_Module; \
		return (__s_pfnGetLauncherCore) ? __s_pfnGetLauncherCore() : NULL; \
	} \
	void UnloadCore(){ \
		FreeLibrary(__s_Module); \
		__s_pfnGetLauncherCore = NULL; \
		__s_Module = NULL; \
	}

	


#endif //_COREAPI_H_
