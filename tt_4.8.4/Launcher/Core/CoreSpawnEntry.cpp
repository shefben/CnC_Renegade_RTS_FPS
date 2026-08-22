#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <mmsystem.h>

#include "Core.h"
#include "../Shared/StdLib.h"
#include "../Shared/Memory.h"
#include "../Shared/HTMLWrite.h"
#include "../Shared/MemoryStream.h"
#include <stdlib.h>
#include <stdio.h>
#include <VFSInterface.h>

#include "../Downloader/DownloaderAPI.h"
#include "../Browser/BrowserAPI.h"
#include "../Browser/Browser.h"
#include "Settings.h"

#include "GameUpdate.h"

HWND		g_hMainWnd		= NULL;
IBrowser*	g_pBrowser		= NULL;
IVFS*		g_pVFS			= NULL;
bool		g_Launching		= false;

//Used to query VFS core interface
VFS_QUERY_COREIF;

HWND Create_Control(const wchar_t *className, int style, int exstyle, const wchar_t *text, int x, int y, int width, int height, HWND parent, const HINSTANCE hInstance, BOOL _3D){
	register HWND temp;
	temp = CreateWindowExW((_3D ? WS_EX_CLIENTEDGE: 0) | exstyle, className, text, WS_CHILD|WS_VISIBLE|style, x, y, width, height, parent, NULL, hInstance, NULL);
	SendMessageW(temp, WM_SETFONT, (WPARAM)(HFONT)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(FALSE, 0));
	return temp;
}

bool Create_Controls(HWND hWnd){	
	return true;
}

void Position_Controls(){
	RECT r;
	GetClientRect(g_hMainWnd, &r);
	g_pBrowser->Resize(&r);
}

void MainLoop(){
	MSG msg;
	memset(&msg, 0, sizeof(MSG));
	while (msg.message != WM_QUIT){
		if (GetMessageW(&msg, NULL, 0U, 0U)){
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}
}

IStream* __stdcall DynProtHandler(const wchar_t *pURL, const wchar_t* pHandler, const wchar_t*const* ppArgs, int argc, IPOSTValues *pPOST){
	CHTMLWriter *pWriter;
	CMemoryBlockStream* pStream;
	int count, i;

	if (wcsicmp(pHandler, L"tds") == 0)
		return TriggerDetectShow(pPOST);
	else if (wcsicmp(pHandler, L"tda") == 0)
		return TriggerDetectApply(pPOST);

	if (!(pStream = new CMemoryBlockStream(8192))) return NULL;
	if (!(pWriter = new CHTMLWriter(pStream))){
		pStream->Release();
		return NULL;
	}

	count = pPOST->GetValueCount();
	pWriter->OpenTag(L"<html>");
	pWriter->OpenTag(L"<body>");
	pWriter->WriteString(L"POST values:");
	pWriter->WriteClosedTag(L"br");

	pWriter->AddAttribute(L"href", L"http://www.google.com");
	pWriter->OpenTag(L"a");
	pWriter->OpenTag(L"b");
	pWriter->WriteString(L"Attribute test!");
	pWriter->WriteClosedTag(L"br");
	pWriter->CloseTag();
	pWriter->CloseTag();

	pWriter->WriteStringFmt(L"Number of values: %d", count);
	pWriter->WriteClosedTag(L"br");


	for (i = 0; i < count; i++){
		switch (pPOST->GetValueType(i)){
			case POST_VALUE_TYPE_STRING:
				pWriter->WriteStringFmt(L"%d/%d %s: \"%s\"", i+1, count, pPOST->GetValueName(i), pPOST->GetValueString(i));
				break;
			case POST_VALUE_TYPE_STREAM:
				pWriter->WriteStringFmt(L"%d/%d %s: IStream", i+1, count, pPOST->GetValueName(i));
				break;
			case POST_VALUE_TYPE_BINARY:
				pWriter->WriteStringFmt(L"%d/%d %s: binary", i+1, count, pPOST->GetValueName(i));
				break;
			case POST_VALUE_TYPE_FAIL:
				pWriter->WriteStringFmt(L"%d/%d %s: error", i+1, count, pPOST->GetValueName(i));
				break;
		}
		pWriter->WriteClosedTag(L"br");
	}

	pWriter->CloseTag();
	pWriter->CloseTag();
	pWriter->Finalize();
	delete pWriter;


	return pStream;
}

//Signal launch process
void LaunchGame(IPOSTValues *pPostValues){
	g_Launching = true;
	CoreSignalExit();
}

//Actual launching here
void DoLaunchGame(){
	STARTUPINFOW sinfo;
	PROCESS_INFORMATION pi;
	memset(&sinfo, 0, sizeof(STARTUPINFOW));
	sinfo.cb = sizeof(STARTUPINFOW);
	if (!(CreateProcessW(L"trose.exe", L"lol", NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &sinfo, &pi))){
		MessageBoxW(0, L"Failed to start AruaROSE.", L"Error", MB_ICONERROR);
		return;
	}
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
}
void __stdcall CallProtHandler(const wchar_t *pURL, const wchar_t* pHandler, const wchar_t*const* ppArgs, int argc, IPOSTValues *pPOST){
	if (wcsicmp(pHandler, L"launch") == 0){
		LaunchGame(pPOST);
		return;
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch (uMsg) {
		case WM_CREATE:
			g_hMainWnd = hWnd;
			if (!Create_Controls(hWnd)){
				MessageBoxW(hWnd, L"Error creating controls.", L"Error.", MB_ICONEXCLAMATION);
				DestroyWindow(hWnd);
				break;
			}
			break;
		case WM_SIZE:
			Position_Controls();			
			break;
		case WM_COMMAND:		

			switch (LOWORD(wParam)){
		case 0:
		default:
			break;
			}
			break;
		case WM_CLOSE:
			DestroyWindow(g_hMainWnd);
			break;
		case WM_NOTIFY:
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}
	return 0;
	
}

void CoreSpawnEntry(const wchar_t* pPatchFileName, HINSTANCE hInstance){
	INITCOMMONCONTROLSEX ex;
	WNDCLASSEXW wcls;	
	char lBuf[512];
	IVFSCoreInterface *pCoreIF;
	int res;

	if (!(pCoreIF = VFSQueryCoreInterface())){
		MessageBoxW(0, L"Failed to get VFS core interface.", L"Fatal error", MB_ICONERROR);
		return;
	}

	if (!(g_pVFS = pCoreIF->CreateVFS())){
		MessageBoxW(0, L"Failed to create VFS instance.", L"Fatal error", MB_ICONERROR);
		return;
	}
	
	if (!g_pVFS->Open(ARUA_VFS, ARUA_VFS_USER_VERSION, false) && !g_pVFS->Create(ARUA_VFS, ARUA_VFS_USER_VERSION, 64*1024)){
		MessageBoxW(0, L"Failed to open VFS.\nPlease make sure that all game and launcher instances are closed.", L"Fatal error", MB_ICONERROR);
		delete g_pVFS;
		return;
	}

	if (pPatchFileName)
		HandleContentData(pPatchFileName);

__recheck_update:
	if ((res = GameUpdate()) != CANCEL_OK){
		if (res == CANCEL_USER)
			MessageBoxW(0, L"Game update was canceled by the user.\nThe game must be fully patched before you can play.", L"Information", MB_ICONINFORMATION);
		else
			MessageBoxW(0, L"Game update failed.\nThe game must be fully patched before you can play.", L"Information", MB_ICONERROR);
		g_Launching = false;
		goto __exit;
	} else if (g_Launching)
		goto __exit;

	pCoreIF->RegisterProtocol(L"vfs");
	g_pVFS->VFSRegisterForProtocol(ARUA_VFS_DOMAIN);

	GetCurrentDirectoryA(512, lBuf);

	ex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	ex.dwICC = 
		ICC_TREEVIEW_CLASSES |
		ICC_BAR_CLASSES |
		ICC_PROGRESS_CLASS;

	GetBrowserCoreInterface()->Intialize(BROWSER_REGISTER_CALL_PROT | BROWSER_REGISTER_DYN_PROT, hInstance);
	InitCommonControlsEx(&ex);
	
	memset(&wcls, 0, sizeof(WNDCLASSEX));


	wcls.lpszClassName = CLASS_NAME;
	wcls.lpszMenuName = NULL;
	wcls.cbSize = sizeof(WNDCLASSEXW);
	wcls.style = 0;
	wcls.lpfnWndProc = WndProc;
	wcls.hIcon = NULL;// g_Application_Icon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_MAIN));
	wcls.lpszMenuName = NULL;
	wcls.hIconSm = NULL;
	wcls.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcls.cbClsExtra = 0;
	wcls.cbWndExtra = 0;
	wcls.hbrBackground = NULL;
	wcls.hInstance = g_hInstance;
	RegisterClassExW(&wcls);

	CBrowser::InitBrowser(hInstance);

	g_pBrowser = (GetBrowserCoreInterface()->CreateIBrowser());
	GetBrowserCoreInterface()->RegisterDynProtCB(DynProtHandler);
	GetBrowserCoreInterface()->RegisterCallProtCB(CallProtHandler);
	
	g_hMainWnd = CreateWindowExW(0, CLASS_NAME, WINDOW_TITLE, 
		WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, 
		NULL, g_hInstance, NULL);

	if (g_hMainWnd){
		g_pBrowser->SetHost(g_hMainWnd);
		//g_pBrowser->Navigate(L"http://www.n00bstories.com/");
		//g_pBrowser->Navigate(L"vfs://1/Test page/index.html");
		//g_pBrowser->Navigate(L"vfs://1/html");
		g_pBrowser->Navigate(L"http://sktest.aruarose.com/formtest3.html");
		//g_pBrowser->Navigate(L"matt.aruarose.com/launcher.php");
		//g_pBrowser->Navigate(L"http://sktest.aruarose.com/matisgay.html");
		MainLoop();
	} else{
		MessageBoxW(0, L"Error creating window.", L"Error", MB_ICONEXCLAMATION);
	}

	delete g_pBrowser;

	CBrowser::DeInitBrowser();

	
	

	UnregisterClassW(CLASS_NAME, hInstance);

	if (g_Launching){
		goto __recheck_update;
	}

	
__exit:
	g_pVFS->Close();

	pCoreIF->UnregisterForProtocol(g_pVFS);


	delete g_pVFS;

	if (g_Launching){
		DoLaunchGame();
	}
}

void GameSignalExit(){
	PostQuitMessage(0);
}
