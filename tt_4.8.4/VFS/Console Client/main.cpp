
#include <windows.h>
#include <conio.h>
#include <stdio.h>
#include "../Shared/Memory.h"
#include "../Shared/Console.h"
#include "../Shared/StdLib.h"


void*	g_pRemoteConsoleIn	 = NULL;
void*	g_pRemoteConsoleOut	 = NULL;
DWORD	g_ParentProcessID	= 0;
HANDLE	g_hEvent			= INVALID_HANDLE_VALUE;

void ParseCommandLine(wchar_t *pCommandLine){
	int count, i;
	wchar_t *pCmds[128];
	//count = TokenizeW(pCommandLine, pCmds, 128, NULL, NULL, FALSE, FALSE, FALSE);
	count = TokenizeW(pCommandLine, pCmds, 128, NULL, FALSE, FALSE);

	for (i = 0; i < count; i++){
		if (wcsncmp(pCmds[i], L"i:", 2) == 0){
			if (swscanf(pCmds[i]+2, L"%p", &g_pRemoteConsoleIn) == 0)
				g_pRemoteConsoleIn = NULL;
		}else if (wcsncmp(pCmds[i], L"o:", 2) == 0){
			if (swscanf(pCmds[i]+2, L"%p", &g_pRemoteConsoleOut) == 0)
				g_pRemoteConsoleIn = NULL;
		}else if (wcsncmp(pCmds[i], L"e:", 2) == 0){
			if (swscanf(pCmds[i]+2, L"%p", &g_hEvent) == 0)
				g_pRemoteConsoleIn = NULL;
		}else if (wcsncmp(pCmds[i], L"p:", 2) == 0){
			if (swscanf(pCmds[i]+2, L"%u", &g_ParentProcessID) == 0)
				g_pRemoteConsoleIn = NULL;
		}
	}
}



int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* lpCmdLine, int nCmdShow){
	SetDefaultMemoryFunctions();
	wchar_t *pCmdLine;
	HANDLE hProcess, hEvent;
	int res;
	CConsole console(false);
	HANDLE hIn, hOut;
	HANDLE _hIn, _hOut;
	SIZE_T w;

	hEvent = INVALID_HANDLE_VALUE;
	hProcess = NULL;
	
	res = 0;

	pCmdLine = StrDupW(GetCommandLineW());
	if (!pCmdLine){
		return 1;
	}

	ParseCommandLine(pCmdLine);

	hProcess = OpenProcess(PROCESS_DUP_HANDLE | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | SYNCHRONIZE, FALSE, g_ParentProcessID);
	if (!hProcess) goto __exit;

	if (!DuplicateHandle(hProcess, g_hEvent, GetCurrentProcess(), &hEvent, 0, FALSE, DUPLICATE_SAME_ACCESS)) goto __exit;

	console.Init();

	console.GetHandles(&_hIn, &_hOut);
	if (!DuplicateHandle(GetCurrentProcess(), _hOut, hProcess, &hOut, 0, FALSE, DUPLICATE_SAME_ACCESS)){
		LogErrorMsg();
		MsgFormated(L"out");
		goto __exit;
	}
	if (!DuplicateHandle(GetCurrentProcess(), _hIn, hProcess, &hIn, 0, FALSE, DUPLICATE_SAME_ACCESS)){
		LogErrorMsg();
		MsgFormated(L"in");
		goto __exit;
	}
	WriteProcessMemory(hProcess, g_pRemoteConsoleIn, &hIn, sizeof(HANDLE), &w);
	WriteProcessMemory(hProcess, g_pRemoteConsoleOut, &hOut, sizeof(HANDLE), &w);

	SetEvent(hEvent);
	LogErrorMsg();
	WaitForSingleObject(hProcess, INFINITE);



__exit:
	if (hEvent != INVALID_HANDLE_VALUE)
		CloseHandle(hEvent);
	if (hProcess != NULL)
		CloseHandle(hProcess);

	g_pFreeer(pCmdLine);

	return res;
}
