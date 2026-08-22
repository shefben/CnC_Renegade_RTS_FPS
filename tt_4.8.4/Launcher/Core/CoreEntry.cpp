#include <windows.h>
#include "../Shared/StdLib.h"
#include "../Shared/BaseTypes.h"

#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <mmsystem.h>

#include "CoreAPI.h"
#include "Core.h"

HINSTANCE	g_hInstance = NULL;
wchar_t		g_ApplicationDir[1024];

class CLauncherCore : public ILauncherCore {
public:
	virtual bool CheckForUpdate(wchar_t* pBuf, size_32 bufsize, bool *pExec, HINSTANCE hInstance);
	virtual void CoreEntry(wchar_t* pPatchFileName, HINSTANCE hInstance);
};

void CoreSharedInitialize(){
	INITCOMMONCONTROLSEX ex;
	
	InitializeStub(NULL);

	ex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	ex.dwICC = 
		ICC_TREEVIEW_CLASSES |
		ICC_BAR_CLASSES |
		ICC_PROGRESS_CLASS;

	OleInitialize(NULL);	
	InitCommonControlsEx(&ex);
	
	GetCurrentDirectoryW(sizeof(g_ApplicationDir)/sizeof(wchar_t), g_ApplicationDir);

}

void CoreSharedUninitialize(){
	OleUninitialize();
}

bool CLauncherCore::CheckForUpdate(wchar_t* pBuf, size_32 bufsize, bool *pExec, HINSTANCE hInstance){
	bool res;
	CoreSharedInitialize();
	*pBuf = 0;
	g_hInstance = hInstance;
	res = CoreSelfUpdate(pBuf, bufsize, pExec);
	CoreSharedUninitialize();
	return res;
}

void CLauncherCore::CoreEntry(wchar_t* pPatchFileName, HINSTANCE hInstance){
	CoreSharedInitialize();
	g_hInstance = hInstance;
	CoreSpawnEntry(pPatchFileName, hInstance);
	CoreSharedUninitialize();
}

CLauncherCore	g_CLauncherCore;

ILauncherCore* __stdcall GetLauncherCore(){
	return &g_CLauncherCore;
}

void CoreSignalExit(){
#ifdef _CoreDoGameSignalExit
	_CoreDoGameSignalExit();
#endif //_CoreDoGameSignalExit
}