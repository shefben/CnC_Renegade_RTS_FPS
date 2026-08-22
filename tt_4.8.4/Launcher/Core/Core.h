#ifndef _CORE_H_
#define _CORE_H_

#include <windows.h>
#include "CoreAPI.h"

#include <VFSInterface.h>
#include "../Browser/BrowserAPI.h"


enum {
	CANCEL_OK = 0, CANCEL_USER, CANCEL_ERROR,
};


bool CoreSelfUpdate(wchar_t* pBuf, size_32 buffsize, bool *pExec);
void HandleContentData(const wchar_t* pContentPackage);
void CoreSpawnEntry(const wchar_t* pPatchFileName, HINSTANCE hInstance);

void CoreSignalExit();

void GameSignalExit();

#define _CoreDoGameSignalExit GameSignalExit

IStream* TriggerDetectShow(IPOSTValues *pPostValues);
IStream* TriggerDetectApply(IPOSTValues *pPostValues);

extern	HINSTANCE	g_hInstance;
extern	wchar_t		g_ApplicationDir[1024];
extern	HWND		g_hMainWnd;
extern	IBrowser*	g_pBrowser;

extern	IVFS*		g_pVFS;

#endif //_CORE_H_
