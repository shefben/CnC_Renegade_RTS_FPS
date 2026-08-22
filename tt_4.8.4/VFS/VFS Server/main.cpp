
#include <windows.h>
#include <conio.h>
#include <stdio.h>
//#include "VFSPipes.h"
#include "../Shared/Memory.h"
#include "../Shared/Console.h"
#include "../Shared/StdLib.h"
#include "../VFS/API/VFSBase.h"
#include "../VFS/VFS.h"
#include "../VFS/API/VFSAdvanced.h"
#include "../VFS/API/VFSCryptography.h"
#include "../VFS/API/VFSInterface.h"
#include "../VFS/API/VFSModule.h"
#include "RPC VFS_h.h"


/*extern "C" {
	extern RPC_IF_HANDLE RPC_VFS_v1_0_c_ifspec;
	extern RPC_IF_HANDLE RPC_VFS_v1_0_s_ifspec;
}//*/

//#ifdef _DEBUG
CConsole	g_Console(true);
CConsole*	g_pConsole = &g_Console;
//#endif //_DEBUG

wchar_t*	g_pEndPoint		= NULL;
wchar_t*	g_pVFSFile		= NULL;
wchar_t*	g_pMutex		= NULL;

EventClass	g_RefCountEvent;
size_32		g_RefCount		= 1;
bool		g_InitialRef	= true;
bool		g_Create		= false;

HANDLE		g_hPipe			= NULL;

IVFS*		g_pVFS			= NULL;

#ifdef _USRDLL

HMODULE	g_hVFS	= NULL;

int CreateVFS(){
	pfCreateNewInstance pf;
	if (!(g_hVFS = LoadLibraryW(VFS_MODULE_NAME))){
#ifdef _DEBUG
		g_pConsole->Write(L"Failed to load VFS module.\n", CON_RED);
		g_pConsole->Write(L"Press any key to continue.\n");
		g_pConsole->ReadChar();
#endif //_DEBUG
		return 10;
	}

	if (!(pf = (pfCreateNewInstance)GetProcAddress(g_hVFS, VFS_INSTANCE_PROC))){
		FreeLibrary(g_hVFS);
#ifdef _DEBUG
		g_pConsole->Write(L"Failed to get creation procedure.\n", CON_RED);
		g_pConsole->Write(L"Press any key to continue.\n");
		g_pConsole->ReadChar();
#endif //_DEBUG
		return 11;
	}

	if (!(g_pVFS = pf())){
		FreeLibrary(g_hVFS);
#ifdef _DEBUG
		g_pConsole->Write(L"Failed to create VFS.\n", CON_RED);
		g_pConsole->Write(L"Press any key to continue.\n");
		g_pConsole->ReadChar();
#endif //_DEBUG
		return 12;
	}
	return 0;
}

#else //_USRDLL

int CreateVFS(){
	return (g_pVFS = new VFS()) ? 0 : 13;
}

#endif //else _USRDLL


void ParseCommandLine(wchar_t *pCommandLine){
	int count, i;
	wchar_t *pCmds[128];
	//count = TokenizeW(pCommandLine, pCmds, 128, NULL, NULL, FALSE, FALSE, FALSE);
	count = TokenizeW(pCommandLine, pCmds, 128, NULL, FALSE, FALSE);

	for (i = 0; i < count; i++){
		if (wcsncmp(pCmds[i], L"endpoint:", 5) == 0)
			g_pEndPoint = pCmds[i]+9;
		else if (wcsncmp(pCmds[i], L"vfs:", 4) == 0)
			g_pVFSFile = pCmds[i]+4;
		else if (wcsncmp(pCmds[i], L"mutex:", 6) == 0)
			g_pMutex = pCmds[i]+6;
		else if (wcscmp(pCmds[i], L"create") == 0)
			g_Create = true;
	}
}



int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* lpCmdLine, int nCmdShow){
	SetDefaultMemoryFunctions();
	wchar_t *pCmdLine;
	HANDLE hMutex;
	int res;
	RPC_STATUS status;

	res = 0;

#ifdef _DEBUG
	g_pConsole->SetTitle(L"VFS Server");
	g_pConsole->SetInfo(100, 80, 500);
#endif //_DEBUG

	pCmdLine = StrDupW(GetCommandLineW());
	if (!pCmdLine){
#ifdef _DEBUG
		g_pConsole->Write(L"Failed to duplicate commandline.\n", CON_RED);
		g_pConsole->Write(L"Press any key to continue.\n");
		g_pConsole->ReadChar();
#endif //_DEBUG
		return 1;
	}
	ParseCommandLine(pCmdLine);
	//AllocConsole();

	if (!g_pEndPoint){
#ifdef _DEBUG
		g_pConsole->Write(L"No endpoint specified.\n", CON_RED);
		g_pConsole->Write(L"Press any key to continue.\n");
		g_pConsole->ReadChar();
#endif //_DEBUG
		g_pFreeer(pCmdLine);
		return 2;
	}

	if (!g_pVFSFile){
#ifdef _DEBUG
		g_pConsole->Write(L"No pipe name specified.\n", CON_RED);
		g_pConsole->Write(L"Press any key to continue.\n");
		g_pConsole->ReadChar();
#endif //_DEBUG
		g_pFreeer(pCmdLine);
		return 3;
	}

	//g_pMutex = L"Global\\lolmutex";

	if (g_pMutex){
		hMutex = CreateMutexW(NULL, TRUE, g_pMutex);
		if (GetLastError() == ERROR_ALREADY_EXISTS){
#ifdef _DEBUG
			g_pConsole->WriteFmt(L"A VFS server instance is already running for \"%s\".\n", CON_RED, g_pMutex);
			g_pConsole->Write(L"Press any key to continue.\n");
			g_pConsole->ReadChar();
#endif //_DEBUG
			g_pFreeer(pCmdLine);
			CloseHandle(hMutex);
			return -1;
		} else if (GetLastError() != ERROR_SUCCESS){
#ifdef _DEBUG
			g_pConsole->WriteFmt(L"Failed to create mutex \"%s\".\n", CON_RED, g_pMutex);
			g_pConsole->Write(L"Press any key to continue.\n");
			g_pConsole->ReadChar();
#endif //_DEBUG
			g_pFreeer(pCmdLine);
			return 4;
		}
	} else
		hMutex = NULL;

	if ((res = CreateVFS()) != 0)
		goto __exit;

	if (!(g_Create ? g_pVFS->Create(g_pVFSFile, 'VFSS', 16*1024) : g_pVFS->Open(g_pVFSFile, 'VFSS', false))){
#ifdef _DEBUG
		g_pConsole->WriteFmt(g_Create ?
			L"Failed to create VFS \"%s\"\n" : L"Failed to open VFS \"%s\"\n", CON_RED, g_pVFSFile);
		g_pConsole->Write(L"Press any key to continue.\n");
		g_pConsole->ReadChar();
#endif //_DEBUG
		res = 8;
		goto __exit;
	}

#ifdef _DEBUG

	g_pConsole->WriteMultiFmt(L"Endpoint:  ", CON_WHITE, 0, L"\"%s\"\n", CON_GREEN, 1, g_pEndPoint, NULL);
	g_pConsole->WriteMultiFmt(L"VFS file:  ", CON_WHITE, 0, L"\"%s\"\n", CON_GREEN, 1, g_pVFSFile, NULL);
	if (g_pMutex)
		g_pConsole->WriteMultiFmt(L"Mutex:     ", CON_WHITE, 0, L"\"%s\"\n", CON_GREEN, 1, g_pMutex, NULL);
#endif //_DEBUG

	status = RpcServerUseProtseqEpW((RPC_WSTR)L"ncalrpc", 20, (RPC_WSTR)g_pEndPoint, NULL);

	if (status != RPC_S_OK){
#ifdef _DEBUG
		g_pConsole->WriteFmt(L"Failed to select protocol sequence entpoint: %d.\n", CON_RED, status);
		g_pConsole->Write(L"Press any key to continue.\n");
		g_pConsole->ReadChar();
#endif //_DEBUG
		res = 5;
		goto __exit;
	}

	status = RpcServerRegisterIfEx(RPC_VFS_v1_0_s_ifspec, NULL, NULL, RPC_C_AUTHN_LEVEL_NONE, RPC_C_LISTEN_MAX_CALLS_DEFAULT, NULL);
	if (status != RPC_S_OK){
#ifdef _DEBUG
		g_pConsole->WriteFmt(L"Failed to register interface.\n", CON_RED, status);
		g_pConsole->Write(L"Press any key to continue.\n");
		g_pConsole->ReadChar();
#endif //_DEBUG
		res = 6;
		goto __exit;
	}

	
	status = RpcServerListen(1, 20, 0);
	if (status != RPC_S_OK){
#ifdef _DEBUG
		g_pConsole->WriteFmt(L"Failed to listen.\n", CON_RED, status);
		g_pConsole->Write(L"Press any key to continue.\n");
		g_pConsole->ReadChar();
#endif //_DEBUG
		res = 7;
		goto __exit;
	}
	g_RefCountEvent.Wait();

__exit:

	g_pFreeer(pCmdLine);
#ifdef _DEBUG
	g_pConsole->Write(L"Press any key to continue.\n");
	g_pConsole->ReadChar();
#endif //_DEBUG
	if (hMutex) CloseHandle(hMutex);
	//FreeConsole();
	return res;
}

void __RPC_FAR * __RPC_API midl_user_allocate(size_t nLen){
	return (g_pAllocator(nLen));
}

void __RPC_API midl_user_free(void __RPC_FAR * lpvPointer){
    if(NULL != lpvPointer)
		g_pFreeer(lpvPointer);
}