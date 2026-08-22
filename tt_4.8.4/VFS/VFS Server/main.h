#ifndef _MAIN_H_
#define _MAIN_H_
#include "../Shared/BaseClasses.h"
#include "../Shared/Console.h"
#include "../VFS/API/VFSBase.h"
#include "../VFS/API/VFSAdvanced.h"
#include "../VFS/API/VFSCryptography.h"
#include "../VFS/API/VFSInterface.h"

//#ifdef _DEBUG
extern CConsole*	g_pConsole;
//#endif //_DEBUG

extern wchar_t*	g_pPipeName;
extern wchar_t*	g_pVFSFile;
extern wchar_t*	g_pMutex;

extern IVFS*	g_pVFS;


extern EventClass	g_RefCountEvent;
extern size_32		g_RefCount;
extern bool			g_InitialRef;

#endif //_MAIN_H_
