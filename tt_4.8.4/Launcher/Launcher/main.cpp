#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <mmsystem.h>
#include <stdio.h>

#include "../Browser/BrowserAPI.h"
#include "../Browser/Browser.h"
#include "../Browser/InetProtocol.h"
#include "../Downloader/Downloader.h"

#include "../Core/CoreAPI.h"

#include "../Shared/memory.h"
#include <stdlib.h>
#include "../Shared/Archive.h"
#include "../Shared/MemoryStream.h"
#include "main.h"
//#include "Resource.h"
#include "../Shared/StdLib.h"
#include "../Shared/BaseClasses.h"

#include <VFSBase.h>
#include <VFSInterface.h>


HINSTANCE	g_hInstance						= NULL;
HWND		g_hMain_Wnd						= NULL;
wchar_t		g_Application_Dir[MAX_PATH+1]	= L"";
CBrowser*	g_pBrowser						= NULL;

VFS_QUERY_COREIF;

/*HWND Create_Control(const wchar_t *className, int style, int exstyle, const wchar_t *text, int x, int y, int width, int height, HWND parent, const HINSTANCE hInstance, BOOL _3D){
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
	GetClientRect(g_hMain_Wnd, &r);
	g_pBrowser->Resize(&r);
}

void mainLoop(){
	MSG msg;
	memset(&msg, 0, sizeof(MSG));
	//g_hAccelerators = LoadAcceleratorsW(g_hInstance, MAKEINTRESOURCEW(IDR_HOTKEYS));
	while (msg.message != WM_QUIT){
		if (GetMessageW(&msg, NULL, 0U, 0U)) {
			//if (IsDialogMessage(g_hMain_Wnd, &msg)) continue;
			//if (!TranslateAcceleratorW(g_hMain_Wnd, g_hAccelerators, &msg)){
			
				//if (msg.message != 280) LogFormated(L"uMsg == %d %X %I64X\n", msg.message, msg.message, msg.hwnd);
				TranslateMessage(&msg);
				DispatchMessageW(&msg);
			//}
		}
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch (uMsg) {
		case WM_CREATE:
			g_hMain_Wnd = hWnd;
			if (!Create_Controls(hWnd)){
				MessageBoxW(hWnd, L"Error creating controls.", L"Error.", MB_ICONEXCLAMATION);
				DestroyWindow(hWnd);
				break;
			}
			//Position_Windows();
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
			DestroyWindow(g_hMain_Wnd);
			break;
		case WM_NOTIFY:
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
	
}

#include "../Shared/HTMLWrite.h"
#include "../Shared/Debug.h"
#include "../Shared/MemoryStream.h"
IStream* __stdcall fnDynProt(const wchar_t *pURL, const wchar_t* pHandler, const wchar_t*const* ppArgs, int argc, IPOSTValues *pPOST){
	CHTMLWriter *pWriter;
	CMemoryBlockStream* pStream;
	int count, i;

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
}//*/

CORE_QUERY_COREIF;

void UpdateExec(wchar_t *pProgram){
	wchar_t lBuf[64];
	STARTUPINFOW sinfo;
	PROCESS_INFORMATION pinfo;
	_snwprintf(lBuf, 64, L"PID:%u", GetCurrentProcessId());

	memset(&sinfo, 0, sizeof(STARTUPINFOW));
	sinfo.cb = sizeof(STARTUPINFOW);

	memset(&pinfo, 0, sizeof(PROCESS_INFORMATION));

	if (!CreateProcessW(pProgram, lBuf, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &sinfo, &pinfo)){
		MessageBoxW(0, L"Failed to create update process.", L"Fatal error", MB_ICONERROR);
		return;
	}
	ResumeThread(pinfo.hThread);
	CloseHandle(pinfo.hProcess);
	CloseHandle(pinfo.hThread);

}

int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* lpCmdLine, int nCmdShow){	
	HMODULE hModule;
	ILauncherCore *pCore;
	wchar_t* pBuf;

	bool exec;

	//To make sure Shared.lib is linked right
	InitializeStub(NULL);

	if (!(pBuf = (wchar_t*)g_pAllocator(32*1024*sizeof(wchar_t)))){
		MessageBoxW(0, L"An error occured during initialization.", L"Error", MB_ICONERROR);
		return 0;
	}
	pCore = LauncherQueryCoreInterface(&hModule);
	if (pCore){
		if (pCore->CheckForUpdate(pBuf, 32*1024, &exec, (HINSTANCE)hModule)){
			if (*pBuf){
				UnloadCore();
				//Downloaded a content delivery package
				if (exec){
					UpdateExec(pBuf);
					goto __cleanup_exit;
				}
				ExtractExtern(L".", pBuf, NULL, NULL);
			}
			if ((pCore = LauncherQueryCoreInterface(&hModule))){
				pCore->CoreEntry(NULL, (HINSTANCE)hModule);
				UnloadCore();
			} else
				MessageBoxW(0, L"An error occured during initialization.", L"Error", MB_ICONERROR);		
		}
	} else
		MessageBoxW(0, L"An error occured during initialization.", L"Error", MB_ICONERROR);
__cleanup_exit:
	g_pFreeer(pBuf);
	return 0;
	/*INITCOMMONCONTROLSEX ex;
	WNDCLASSEXW wcls;



	g_hInstance = hInstance;
	bool eventset;
	CMemoryDownload *pDownload;
	
	IVFS* pVFS;
	bool b;
	char lBuf[512];
	GetCurrentDirectoryA(512, lBuf);

	ex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	ex.dwICC = 
		ICC_TREEVIEW_CLASSES |
		ICC_BAR_CLASSES |
		ICC_PROGRESS_CLASS;

	//GetInternetSession()->RegisterNameSpace(&g_CF, CLSID_NULL, L"dll", 0, NULL, 0);
	GetBrowserCoreInterface()->Intialize(BROWSER_REGISTER_CALL_PROT | BROWSER_REGISTER_DYN_PROT, hInstance);
	if (0 && VFSQueryCoreInterface()){
		pVFS = VFSQueryCoreInterface()->CreateVFS();
		VFSQueryCoreInterface()->RegisterProtocol(L"vfs");
		pVFS->VFSRegisterForProtocol(L"1");
		b = pVFS->Open(L"./prot.vfs", 0, false);
		if (b)
			MessageBoxW(0, L"Successfully mounted prot.vfs", L"Success", MB_ICONINFORMATION);
	} else
		pVFS = NULL;

	//SetDefaultMemoryFunctions();
	CBrowserEventDispatch cbed;
	DWORD cbedcookie;
	CBrowserEvents2 events;

	
	cbed.SetCallback(&events);

	

	/*pDownload = new CMemoryDownload;

	if (pDownload->Start(L"http://sktest.aruarose.com/test.dat", NULL, 0, NULL, NULL, 1024)){
		pDownload->Wait();
		pDownload->LockBuffer();
		void *buf = pDownload->GetBuffer();
		pDownload->UnlockBuffer();
	}

	delete pDownload;
	
	InitCommonControlsEx(&ex);
	CoInitialize(NULL);
	OleInitialize(NULL);

	CFileDownload dl;
	dl.Start(L"http://sktest.aruarose.com/test.dat", L"C:\\Launcher content\\lmaowaffles.dat", NULL, NULL, NULL);
	dl.Wait();
	dl.Close();

	//CMemoryStream stream(5);
	//stream.Test();

	
	memset(&wcls, 0, sizeof(WNDCLASSEX));
	GetCurrentDirectoryW(sizeof(g_Application_Dir)/sizeof(wchar_t), g_Application_Dir);


	wcls.lpszClassName = CLASS_NAME;
	wcls.lpszMenuName = NULL;
	wcls.cbSize = sizeof(WNDCLASSEXW);
	wcls.style = 0;
	wcls.lpfnWndProc = WndProc;
	wcls.hIcon = NULL;// g_Application_Icon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_MAIN));
	wcls.lpszMenuName = NULL;//MAKEINTRESOURCEW(IDR_MAINMENU);
	wcls.hIconSm = NULL;
	wcls.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcls.cbClsExtra = 0;
	wcls.cbWndExtra = 0;
	wcls.hbrBackground = NULL;
	wcls.hInstance = g_hInstance;
	RegisterClassExW(&wcls);

	CBrowser::InitBrowser(hInstance);
	//GetBrowserCoreInterface()->RegisterDynProtCB(fnDynProt);

	g_pBrowser = (CBrowser*)(GetBrowserCoreInterface()->CreateIBrowser());
	
	g_hMain_Wnd = CreateWindowExW(0, CLASS_NAME, WINDOW_TITLE, 
		WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, 
		NULL, g_hInstance, NULL);

	if (g_hMain_Wnd){
		eventset = g_pBrowser->AddEventCallback(&cbed, &cbedcookie);
		g_pBrowser->SetHost(g_hMain_Wnd);
		//g_pBrowser->Navigate(L"http://www.n00bstories.com/");
		//g_pBrowser->Navigate(L"vfs://1/Test page/index.html");
		//g_pBrowser->Navigate(L"vfs://1/html");
		//g_pBrowser->Navigate(L"http://sktest.aruarose.com/formtest3.html");
		g_pBrowser->Navigate(L"matt.aruarose.com/launcher.php");
		//g_pBrowser->Navigate(L"http://sktest.aruarose.com/matisgay.html");
		mainLoop();
		if (eventset) g_pBrowser->RemoveEventCallback(cbedcookie);
	} else{
		MessageBoxW(0, L"Error creating window.", L"Error", MB_ICONEXCLAMATION);
	}

	delete g_pBrowser;

	CBrowser::DeInitBrowser();
	
	

	//DeleteObject(g_Application_Icon);
	UnregisterClassW(CLASS_NAME, g_hInstance);
	OleUninitialize();
	CoUninitialize();//*/

	return 0;
	
	//ExitProcess(0);
}
