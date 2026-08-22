#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <mmsystem.h>
#include <stdlib.h>
#include <stdio.h>

#include "../Shared/memory.h"
#include "main.h"
#include "Resource.h"
#include "../Shared/StdLib.h"
#include "../VFS/API/VFSInterface.h"
#include "../VFS/API/VFSErrorcodes.h"
#include "../VFS/API/VFSAdvanced.h"
#include "../VFS/API/VFSModule.h"

/*
Global vars
*/

HINSTANCE	g_hInstance			= NULL;

HWND		g_hMain_Wnd			= NULL;
HWND		g_hTree_View		= NULL;
HWND		g_hStatus_Bar		= NULL;
HWND		g_hProgress_Bar		= NULL;
HICON		g_Application_Icon	= NULL;
HACCEL		g_hAccelerators		= NULL;

IVFS*		g_pVFS				= NULL;

PVFSITEM	g_pExtraction_Src	= NULL;

BOOL		g_Show_Progress_Bar	= FALSE;

int			g_File_Count		= 0;
int			g_Dir_Count			= 0;
int			g_Max_Token_Count	= 0;
DWORD		g_TID				= 0;

bool		g_No_Enum			= false;

BOOL		g_Open_File			= FALSE;

wchar_t		g_Root_Title[MAX_PATH+5]		= L"";
wchar_t		g_Current_File[MAX_PATH+1]		= L"";
wchar_t		g_Current_Dir[MAX_PATH+1]		= L"";
wchar_t		g_Application_Dir[MAX_PATH+1]	= L"";
wchar_t		g_Extraction_Dir[MAX_PATH+1]	= L"";
wchar_t*	g_pName_Cache					= NULL;

#define NAME_CACHE_SIZE						((32*1024)+1)


pfCreateNewInstance	g_pfCreateNewInstance	= NULL;
HMODULE				g_hVFS					= NULL;



/*
The folder browsing callback to initalize the path
*/

int CALLBACK BrowseCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData){
	if (uMsg == BFFM_INITIALIZED){
		SendMessageW(hwnd, BFFM_SETEXPANDED, (WPARAM)TRUE, lpData);
	}
	return 0;
}

/*
Displays the browse for folder dialog
*/

BOOL Browse_For_Folder(wchar_t *pFolder){
	LPITEMIDLIST pItemList;
	BROWSEINFOW bInfo;

	memset(&bInfo, 0, sizeof(BROWSEINFO));

	bInfo.hwndOwner = g_hMain_Wnd;
	bInfo.lpszTitle = L"Please select the destination folder:";
	bInfo.pidlRoot = NULL;
	bInfo.ulFlags = BIF_NEWDIALOGSTYLE;
	bInfo.pszDisplayName = NULL;
	bInfo.lpfn = BrowseCallback;
	bInfo.lParam = (LPARAM)pFolder;
	if (!(pItemList = SHBrowseForFolderW(&bInfo)))return FALSE;

	if (!SHGetPathFromIDListW(pItemList, pFolder)){
		CoTaskMemFree(pItemList);
		return FALSE;
	}
	CoTaskMemFree(pItemList);
	return TRUE;
}

bool __OpenFile(wchar_t *buf, int bufsize, wchar_t *filter, int *FileOffset, int *ExtensionOffset){
	wchar_t lBuf[MAX_PATH] = L"";
	OPENFILENAMEW of;

	memset(&of, 0, sizeof(OPENFILENAMEW));

	of.lStructSize = sizeof(OPENFILENAMEW);
	of.hwndOwner = g_hMain_Wnd;
	of.hInstance = NULL;
	of.lpstrFilter = filter;
	of.lpstrCustomFilter = NULL;
	of.nMaxCustFilter = 0;
	of.nFilterIndex = 0;
	of.lpstrFile = lBuf;
	of.nMaxFile = MAX_PATH;
	of.lpstrFileTitle = NULL;
	of.nMaxFileTitle = 0;
	of.lpstrInitialDir = NULL;
	of.lpstrTitle = NULL;
	of.Flags = OFN_FILEMUSTEXIST; /*--*/
	of.nFileOffset = 0;
	of.nFileExtension = 0;
	of.lpstrDefExt = NULL;
	of.lCustData = NULL;
	of.lpfnHook = NULL;
	of.lpTemplateName = NULL;	

	if (!GetOpenFileNameW(&of)) return false;
	_snwprintf(buf, bufsize, L"%s", of.lpstrFile);
	if (FileOffset)
		*FileOffset = of.nFileOffset;
	if (ExtensionOffset)
		*ExtensionOffset = of.nFileExtension;
	_snwprintf(lBuf, MAX_PATH, L"%s - %s", WINDOW_TITLE, buf+of.nFileOffset);
	SetWindowTextW(g_hMain_Wnd, lBuf);
	return true;
}

bool __OpenFile2(wchar_t *buf, int bufsize, wchar_t *filter, int *FileOffset, int *ExtensionOffset){
	wchar_t lBuf[MAX_PATH] = L"";
	OPENFILENAMEW of;

	memset(&of, 0, sizeof(OPENFILENAMEW));

	of.lStructSize = sizeof(OPENFILENAMEW);
	of.hwndOwner = g_hMain_Wnd;
	of.hInstance = NULL;
	of.lpstrFilter = filter;
	of.lpstrCustomFilter = NULL;
	of.nMaxCustFilter = 0;
	of.nFilterIndex = 0;
	of.lpstrFile = lBuf;
	of.nMaxFile = MAX_PATH;
	of.lpstrFileTitle = NULL;
	of.nMaxFileTitle = 0;
	of.lpstrInitialDir = NULL;
	of.lpstrTitle = NULL;
	of.Flags = OFN_FILEMUSTEXIST; /*--*/
	of.nFileOffset = 0;
	of.nFileExtension = 0;
	of.lpstrDefExt = NULL;
	of.lCustData = NULL;
	of.lpfnHook = NULL;
	of.lpTemplateName = NULL;	

	if (!GetOpenFileNameW(&of)) return false;
	_snwprintf(buf, bufsize, L"%s", of.lpstrFile);
	if (FileOffset)
		*FileOffset = of.nFileOffset;
	if (ExtensionOffset)
		*ExtensionOffset = of.nFileExtension;
	return true;
}

/*-- File open dialog --*/

bool __SaveFile(wchar_t *buf, int bufsize, wchar_t *filter, wchar_t *def_ext, int *FileOffset, int *ExtensionOffset){
	wchar_t lBuf[MAX_PATH] = L"";
	wcscpy(lBuf, buf);
	OPENFILENAMEW of;

	memset(&of, 0, sizeof(OPENFILENAMEW));

	of.lStructSize = sizeof(OPENFILENAMEW);
	of.hwndOwner = g_hMain_Wnd;
	of.hInstance = NULL;
	of.lpstrFilter = filter;
	of.lpstrCustomFilter = NULL;
	of.nMaxCustFilter = 0;
	of.nFilterIndex = 0;
	of.lpstrFile = lBuf;
	of.nMaxFile = MAX_PATH;
	of.lpstrFileTitle = NULL;
	of.nMaxFileTitle = 0;
	of.lpstrInitialDir = NULL;
	of.lpstrTitle = NULL;
	of.Flags = OFN_OVERWRITEPROMPT; /*--*/
	of.nFileOffset = 0;
	of.nFileExtension = 0;
	of.lpstrDefExt = def_ext;
	of.lCustData = NULL;
	of.lpfnHook = NULL;
	of.lpTemplateName = NULL;	
	if (!GetSaveFileNameW(&of)) return false;
	_snwprintf(buf, bufsize, L"%s", of.lpstrFile);
	if (FileOffset)
		*FileOffset = of.nFileOffset;
	if (ExtensionOffset)
		*ExtensionOffset = of.nFileExtension;
	_snwprintf(lBuf, MAX_PATH, L"%s - %s", WINDOW_TITLE, buf+of.nFileOffset);
	SetWindowText(g_hMain_Wnd, lBuf);
	return true;
}

/*
Create a control
*/

HWND Create_Control(const wchar_t *className, int style, int exstyle, const wchar_t *text, int x, int y, int width, int height, HWND parent, const HINSTANCE hInstance, BOOL _3D){
	register HWND temp;
	temp = CreateWindowExW((_3D ? WS_EX_CLIENTEDGE: 0) | exstyle, className, text, WS_CHILD|WS_VISIBLE|style, x, y, width, height, parent, NULL, hInstance, NULL);
	SendMessageW(temp, WM_SETFONT, (WPARAM)(HFONT)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(FALSE, 0));
	return temp;
}

/*
Create all controls
*/
BOOL Create_Controls(HWND hWnd){
	int widths[1] = {-1};

	if (g_hStatus_Bar = Create_Control(STATUSCLASSNAMEW, 0, 0, 0, 0, 0, 0, 0, hWnd, g_hInstance, FALSE)){
		SendMessage(g_hStatus_Bar, SB_SETPARTS, (WPARAM)1, (LPARAM)widths);
		//SendMessage(g_hStatus_Bar, SB_SETMINHEIGHT, (WPARAM)STATUSBAR_HEIGHT, 0);
		SendMessage(g_hStatus_Bar, WM_SIZE, 0, 0);
	}
	g_hTree_View = Create_Control(WC_TREEVIEWW, TREEVIEW_STYLE, 0, 0, 0, 0, 0, 0, hWnd, g_hInstance, TRUE);

	if (g_hProgress_Bar = Create_Control(PROGRESS_CLASSW, PBS_SMOOTH, 0, 0, 0, 0, 0, 0, g_hStatus_Bar, g_hInstance, FALSE)){
		SendMessage(g_hProgress_Bar, PBM_SETBARCOLOR, 0, (LPARAM)RGB(0,0,0));
	}
	return (g_hStatus_Bar && g_hTree_View && g_hProgress_Bar);
}

/*
Position all controls
*/

void Position_Controls(){
	int parts[3] = { -1, -1, -1 };
	RECT m, s;
	GetClientRect(g_hMain_Wnd, &m);
	GetClientRect(g_hStatus_Bar, &s);
	SetWindowPos(g_hTree_View, 0, 0, 0, m.right-m.left, m.bottom-s.bottom, SWP_NOZORDER);
	if (g_Show_Progress_Bar){
		parts[0] = (s.right-s.left)-120;
		parts[1] = (s.right-s.left)-20;
		SendMessageW(g_hStatus_Bar, SB_SETPARTS, (WPARAM)3, (LPARAM)&parts);
		SendMessageW(g_hStatus_Bar, SB_GETRECT, (WPARAM)1, (LPARAM)&m);
		SetWindowPos(g_hProgress_Bar, 0, m.left, m.top, m.right-m.left, m.bottom-m.top, SWP_NOZORDER);
		SendMessageW(g_hProgress_Bar, PBM_SETPOS, (WPARAM)100, 0);
	} else
		SendMessageW(g_hStatus_Bar, SB_SETPARTS, (WPARAM)1, (LPARAM)parts);
}

/*
Set the statusbar text
*/

void Set_Status_Text(const wchar_t *pStr){
	SendMessageW(g_hStatus_Bar, SB_SETTEXTW, 0, (LPARAM)pStr);
}

/*
Set the statusbar text with variable paramters
*/

void Set_Status_Text_Fmt(const wchar_t *pFmt, ...){
	wchar_t lBuf[512];
	va_list va;
	va_start(va, pFmt);
	_vsnwprintf(lBuf, 512, pFmt, va);
	va_end(va);
	SendMessageW(g_hStatus_Bar, SB_SETTEXTW, 0, (LPARAM)lBuf);
}

/*
Enable or disable the progres bar and update position & statusbar
*/

__inline void Enable_Progress_Bar(BOOL enable){
	g_Show_Progress_Bar = enable;
	ShowWindow(g_hProgress_Bar, (enable ? SW_SHOW : SW_HIDE));
	Position_Controls();
}

/*
Lock all controls
*/

void Lock_Controls(BOOL lock){
	register HMENU menu = GetMenu(g_hMain_Wnd);
	register UINT flags = (lock ? MF_GRAYED : MF_ENABLED) | MF_BYCOMMAND;
	EnableMenuItem(menu, ID_NEW, flags);
	EnableMenuItem(menu, ID_OPEN, flags);
	EnableMenuItem(menu, ID_CLOSE, flags);
	EnableMenuItem(menu, ID_EXIT, flags);

	EnableMenuItem(menu, ID_EXTRACT, flags);
	EnableMenuItem(menu, ID_EXTRACTALL, flags);
	EnableMenuItem(menu, ID_FIND, flags);
	EnableMenuItem(menu, ID_FINDNEXT, flags);

	EnableMenuItem(menu, ID_CREATE_DIR, flags);
	EnableMenuItem(menu, ID_RENAME, flags);
	EnableMenuItem(menu, ID_IMPORT_DIR, flags);
	EnableMenuItem(menu, ID_IMPORT_FILE, flags);

	menu = GetSystemMenu(g_hMain_Wnd, FALSE);
	//EnableMenuItem(menu, SC_CLOSE, flags);

	EnableWindow(g_hTree_View, lock ? FALSE : TRUE);
	DrawMenuBar(g_hMain_Wnd);
}

/*
Set progress range
*/

void Init_Progress(int max_size){
	SendMessageW(g_hProgress_Bar, PBM_SETRANGE32, 0, (LPARAM)max_size);
}

/*
Set current progress
*/

void Set_Progress(int progress){
	SendMessage(g_hProgress_Bar, PBM_SETPOS, (WPARAM)progress, 0);
}


/*
Extract a dir content
*/

void Extract_Dir(int Dir, const char *basedir, void *pCache){
	/*char path[MAX_PATH];
	register int len = 0;
	register unsigned long start, time_;
	int fcount = 0;
	StrCpy(path, basedir);
	len = StrLen(path);
	pSFFS = &g_pSFFSs[g_Current_SFFS];
	if (!Make_Path(Dir, pSFFS, path, MAX_PATH-len, path+len)){
		Set_Status_Text("An error occurded during path creation.");
	} else{
		start = timeGetTime();
		//Enable_Progress_Bar(TRUE);
		//Init_Progess(pDir->m_Total_File_Count);
		Extract_Path(Dir, path, MAX_PATH-len, path+len, &fcount, pCache);
		Enable_Progress_Bar(FALSE);
		time_ = (timeGetTime()-start) / 1000;
		Set_Status_Text_Fmt("Extracted %d files in %d minutes, %.2d seconds.", fcount, time_ / 60, time_ % 60);
	}//*/
}

void VFS_CALL ExtractAllCB(const ptr pData, int type, size_64 progress, size_64 total, ptr pCookie){
	//wchar_t lBuf[1024];
	if (type == PROGRESS_TYPE_FILECOUNT){
		Enable_Progress_Bar(TRUE);
		Init_Progress((int)total);
		Set_Progress(0);
	} else if (type == PROGRESS_TYPE_LASTFILE)
		Enable_Progress_Bar(FALSE);
	else if (type == PROGRESS_TYPE_NEXTFILE){
		//_snwprintf(lBuf, 1024, L"%I64u/%I64u %s", progress, total, pData);
		Set_Progress((int)progress);
		Set_Status_Text_Fmt(L"%I64u/%I64u %s", progress, total, pData);

	}
	

}

/*
The extract thread
*/

DWORD CALLBACK Extraction_Thread(LPVOID pArg){
	Lock_Controls(TRUE);
	//VFSFILEINFO info;
	PROGESSCB cb;
	cb.m_pCB = ExtractAllCB;
	//g_pVFS->VFSQueryFileInfo(&info, g_pExtraction_Src->m_pFileData, g_pName_Cache, 32*1024);
	g_pVFS->VFSExtractDir(L"\\", g_Extraction_Dir, TRUE, &cb);
	Lock_Controls(FALSE);
	/*int	e_code	= 0;

	if (!g_pExtraction_Src){
		g_TID = 0;
		ExitThread(0);
	}

	EnterCriticalSection(&g_Cache_CS);
	if (!(g_pCache = Malloc(CACHE_SIZE))) {
		LeaveCriticalSection(&g_Cache_CS);
		g_TID = 0;
		ExitThread(0);
	}
	LeaveCriticalSection(&g_Cache_CS);

	Lock_Controls(TRUE);
	if (g_pExtraction_Src->m_pEntry->m_Type == TYPE_FILE){
		e_code = ExtractFile(&g_pSFFSs[g_Current_SFFS], g_pExtraction_Src->m_pEntry, g_Extraction_Dir, TRUE);
		if (e_code != EX_ERROR_OK){
			Set_Status_Text_Fmt("Error %x occured during file extraction.");
		}
	} else if (g_pExtraction_Src->m_pEntry->m_Type == TYPE_DIRECTORY){
		StrCat(g_Extraction_Dir, "\\");
		Extract_Dir(g_pExtraction_Src->m_This_Index, g_Extraction_Dir, g_pCache);
	}
	Lock_Controls(FALSE);

	if (g_pCache) Free(g_pCache);
	g_pCache = NULL;//*/
	g_TID = 0;
	ExitThread(0);
}


/*
Free thread data
*/

void Free_Thread_Data(){
	register HANDLE hThread;

	if (g_TID){;
		if (hThread = OpenThread(THREAD_ALL_ACCESS, TRUE, g_TID)){
			TerminateThread(hThread, 0);
			CloseHandle(hThread);
		}
		g_TID = 0;

	}
}



/*void Recursive_Tree_Pass(PSFFS pSFFS, int This, HTREEITEM hParent, char *pBuf){
	/*int start;
	short sib;
	TVINSERTSTRUCT tv;
	PFS_DATA pData;

	MemSet(&tv, 0, sizeof(TVINSERTSTRUCT));
	tv.hInsertAfter = TVI_ROOT;
	tv.itemex.mask = TVIF_TEXT | TVIF_PARAM;
	tv.itemex.lParam = (LPARAM)(pData = &g_pFS_Data[This]);
	tv.itemex.pszText = pBuf;
	tv.hParent = hParent;
	pData = &g_pFS_Data[This];
	StrnCpy(pBuf, pData->m_pEntry->m_Name, 12);


	if (pData->m_pEntry->m_Type == TYPE_DIRECTORY){
		pData->m_hTreeItem = (HTREEITEM)SendMessage(g_hTree_View, TVM_INSERTITEM, 0, (LPARAM)&tv);
		sib = pData->m_pEntry->m_First_Child;
		while (sib != -1){
			Recursive_Tree_Pass(pSFFS, sib, pData->m_hTreeItem, pBuf);
			sib = pSFFS->m_Entries[sib].m_Sibling;
		}
	} else if (pData->m_pEntry->m_Type == TYPE_FILE){
		pData->m_hTreeItem = (HTREEITEM)SendMessage(g_hTree_View, TVM_INSERTITEM, 0, (LPARAM)&tv);
	}

}//*/


int VFS_CALL EnumCallback(PVFSFILEINFO pFileInfo, ptr wParam, ptr lParam){
	HTREEITEM hItem;
	TVINSERTSTRUCTW tv;

	PVFSITEM pItem;

	pItem = new VFSITEM;

	pItem->m_TotalSize = 0;
	pItem->m_FileCount = 0;
	pItem->m_DirectoryCount = 0;
	pItem->m_pFileData = pFileInfo->m_pFileData;
	pItem->m_Type = pFileInfo->m_Type;
	pItem->m_pParentFileData = lParam;

	g_pVFS->VFSSetRuntimeUserData(pFileInfo->m_pFileData, (ptr)pItem);

	memset(&tv, 0, sizeof(TVINSERTSTRUCTW));
	tv.hInsertAfter = TVI_ROOT;
	tv.itemex.mask = TVIF_TEXT | TVIF_PARAM;
	tv.itemex.lParam = (LPARAM)pItem;
	tv.itemex.pszText = (wchar_t*)pFileInfo->m_pName;
	tv.hParent = *((HTREEITEM*)wParam);

	pItem->m_hTreeItem = hItem = (HTREEITEM)SendMessageW(g_hTree_View, TVM_INSERTITEMW, 0, (LPARAM)&tv);
	

	if (pFileInfo->m_Type == VFS_TYPE_DIRECTORY){
		g_pVFS->VFSSingleEnum(pFileInfo->m_pFileData, EnumCallback, (ptr)&hItem, pFileInfo->m_pFileData);		
	}
	return VFS_ENUM_CONTINUE;
}

int VFS_CALL CountEnumCallback(PVFSFILEINFO pFileInfo, ptr wParam, ptr lParam){

	PVFSITEM pItem;
	PVFSITEM pParent;

	pItem = (PVFSITEM)pFileInfo->m_pUserData;

	pParent = (PVFSITEM)lParam;

	pItem->m_TotalSize = 0;
	pItem->m_FileCount = pItem->m_DirectoryCount = 0;


	if (pFileInfo->m_Type == VFS_TYPE_DIRECTORY){
		if (pParent) pParent->m_DirectoryCount++;
		g_pVFS->VFSSingleEnum(pFileInfo->m_pFileData, CountEnumCallback, NULL, (ptr)pItem);
	} else {
		if (pParent) pParent->m_FileCount++;
		pItem->m_TotalSize += pFileInfo->m_Size;
	}

	if (pParent){
		pParent->m_TotalSize += pItem->m_TotalSize;
		pParent->m_FileCount += pItem->m_FileCount;
		pParent->m_DirectoryCount += pItem->m_DirectoryCount;
	}
	//LogFormated(L"%s -> total size: %I64u dirs: %d files: %d\n", pFileInfo->m_pFullName, pItem->m_TotalSize, pItem->m_DirectoryCount, pItem->m_FileCount);
	return VFS_ENUM_CONTINUE;
}

void UpdateUnrolled(PVFSITEM pThisItem, PVFSITEM pItem){
	PVFSITEM _pItem;
	if (pThisItem->m_Type == VFS_TYPE_FILE){
		pItem->m_TotalSize += pThisItem->m_TotalSize;
		pItem->m_FileCount++;
	} else {
		pItem->m_TotalSize += pThisItem->m_TotalSize;
		pItem->m_FileCount += pThisItem->m_FileCount;
		pItem->m_DirectoryCount += pThisItem->m_DirectoryCount + 1;
	}

	if (pItem->m_Type == VFS_TYPE_DIRECTORY && pItem->m_pParentFileData){
		_pItem = (PVFSITEM)g_pVFS->VFSGetRuntimeUserData(pItem->m_pParentFileData);
		if (_pItem)
			UpdateUnrolled(pThisItem, _pItem);
	}
}

HTREEITEM AddFileToTree(HTREEITEM hParentItem, PVFSFILEINFO pFileInfo, ptr pParentFileData){

	HTREEITEM hItem;
	TVINSERTSTRUCTW tv;

	PVFSITEM pItem, pParentItem;

	pParentItem = (PVFSITEM)g_pVFS->VFSGetRuntimeUserData(pParentFileData);

	pItem = new VFSITEM;

	pItem->m_TotalSize = pFileInfo->m_Size;
	pItem->m_FileCount = 0;
	pItem->m_DirectoryCount = 0;
	pItem->m_pFileData = pFileInfo->m_pFileData;
	pItem->m_Type = pFileInfo->m_Type;
	pItem->m_pParentFileData = pParentFileData;

	g_pVFS->VFSSetRuntimeUserData(pFileInfo->m_pFileData, (ptr)pItem);

	memset(&tv, 0, sizeof(TVINSERTSTRUCTW));
	tv.hInsertAfter = TVI_ROOT;
	tv.itemex.mask = TVIF_TEXT | TVIF_PARAM;
	tv.itemex.lParam = (LPARAM)pItem;
	tv.itemex.pszText = (wchar_t*)pFileInfo->m_pName;
	tv.hParent = hParentItem;

	pItem->m_hTreeItem = hItem = (HTREEITEM)SendMessageW(g_hTree_View, TVM_INSERTITEMW, 0, (LPARAM)&tv);
	UpdateUnrolled(pItem, pParentItem);
	return hItem;
}

void GetCounts(){
	ptr pFiledata;
	PVFSITEM pItem;
	if (!(pFiledata = g_pVFS->VFSGetRootFiledata())) return;
	if (!(pItem = (PVFSITEM)g_pVFS->VFSGetRuntimeUserData(pFiledata))) return;

	
	pItem->m_TotalSize = 0;
	pItem->m_FileCount = pItem->m_DirectoryCount = 0;

	g_pVFS->VFSSingleEnum(NULL, CountEnumCallback, NULL, pItem);
}

/*
Parse header and get fileentries/etc
*/

bool Parse(const wchar_t* fname, int extension){
	DWORD d;
	HTREEITEM hItem;
	PVFSITEM pItem;
	VFSFILEINFO finfo;

	if (!g_pVFS->Open(fname, 'AURA', false)) return false;

	TVINSERTSTRUCTW tv;


	pItem = new VFSITEM;

	g_pVFS->VFSQueryFileInfo(&finfo, g_pVFS->VFSGetRootFiledata(), NULL, 0);

	pItem->m_TotalSize = 0;
	pItem->m_FileCount = 0;
	pItem->m_DirectoryCount = 0;
	pItem->m_pFileData = finfo.m_pFileData;
	pItem->m_pParentFileData = NULL;
	g_pVFS->VFSSetRuntimeUserData(finfo.m_pFileData, (ptr)pItem);

	memset(&tv, 0, sizeof(TVINSERTSTRUCTW));
	tv.hInsertAfter = TVI_ROOT;
	tv.itemex.mask = TVIF_TEXT | TVIF_PARAM;
	tv.itemex.lParam = (LPARAM)pItem;
	tv.itemex.pszText = (wchar_t*)L"root";
	tv.hParent = TVI_ROOT;
	pItem->m_Type = finfo.m_Type;

	
	pItem->m_hTreeItem = hItem = (HTREEITEM)SendMessageW(g_hTree_View, TVM_INSERTITEMW, 0, (LPARAM)&tv);

	g_pVFS->VFSSingleEnum(NULL, EnumCallback, (ptr)&hItem, (ptr)finfo.m_pFileData);
	g_No_Enum = (g_pVFS->VFSGetSettings() & VFS_SETTING_NO_ENUM) ? true : false;
	d = timeGetTime();
	GetCounts();
	LogFormated(L"GetCounts() took %.3f seconds.\n", ((float)(timeGetTime()-d))/1000.0f);
	return true;
}

HTREEITEM AddObject(HTREEITEM hParent, const wchar_t* pName, int type, ptr pParentFileData, ptr pFileData){
	HTREEITEM hItem;
	TVINSERTSTRUCTW tv;

	PVFSITEM pItem;

	pItem = new VFSITEM;

	pItem->m_TotalSize = 0;
	pItem->m_FileCount = 0;
	pItem->m_DirectoryCount = 0;
	pItem->m_pFileData = pFileData;
	pItem->m_Type = type;
	pItem->m_pParentFileData = pParentFileData;

	g_pVFS->VFSSetRuntimeUserData(pFileData, (ptr)pItem);

	memset(&tv, 0, sizeof(TVINSERTSTRUCTW));
	tv.hInsertAfter = TVI_ROOT;
	tv.itemex.mask = TVIF_TEXT | TVIF_PARAM;
	tv.itemex.lParam = (LPARAM)pItem;
	tv.itemex.pszText = (wchar_t*)pName;
	tv.hParent = hParent;

	return pItem->m_hTreeItem = hItem = (HTREEITEM)SendMessageW(g_hTree_View, TVM_INSERTITEMW, 0, (LPARAM)&tv);
	
}


/*
Create new VFS
*/

bool Create(const wchar_t* fname, int extension){
	//DWORD d;
	HTREEITEM hItem;
	PVFSITEM pItem;
	VFSFILEINFO finfo;

	if (!g_pVFS->Create(fname, 'VFSS', CS_16K)) return false;

	TVINSERTSTRUCTW tv;


	pItem = new VFSITEM;

	g_pVFS->VFSQueryFileInfo(&finfo, g_pVFS->VFSGetRootFiledata(), NULL, 0);

	pItem->m_TotalSize = 0;
	pItem->m_FileCount = 0;
	pItem->m_DirectoryCount = 0;
	pItem->m_pFileData = finfo.m_pFileData;
	pItem->m_pParentFileData = NULL;
	g_pVFS->VFSSetRuntimeUserData(finfo.m_pFileData, (ptr)pItem);

	memset(&tv, 0, sizeof(TVINSERTSTRUCTW));
	tv.hInsertAfter = TVI_ROOT;
	tv.itemex.mask = TVIF_TEXT | TVIF_PARAM;
	tv.itemex.lParam = (LPARAM)pItem;
	tv.itemex.pszText = (wchar_t*)L"root";
	tv.hParent = TVI_ROOT;
	pItem->m_Type = finfo.m_Type;

	
	pItem->m_hTreeItem = hItem = (HTREEITEM)SendMessageW(g_hTree_View, TVM_INSERTITEMW, 0, (LPARAM)&tv);

	//g_pVFS->VFSSingleEnum(NULL, EnumCallback, (ptr)&hItem, NULL);
	//g_No_Enum = (g_pVFS->VFSGetSettings() & VFS_SETTING_NO_ENUM) ? true : false;
	//d = timeGetTime();
	//GetCounts();
	//LogFormated(L"GetCounts() took %.3f seconds.\n", ((float)(timeGetTime()-d))/1000.0f);
	return true;
}


/*
Free all data
*/

void Free_Data(){
	g_pVFS->Close();
	SendMessageW(g_hTree_View, TVM_DELETEITEM, 0, 0);
	Free_Thread_Data();
	Lock_Controls(FALSE);
	Enable_Progress_Bar(FALSE);
}


/*
Procession file opening request
*/

void Do_Open(){
	int filenameoff, extoff;
	wchar_t lBuf[1024];
	register wchar_t *pStr;
	SetWindowTextW(g_hMain_Wnd, WINDOW_TITLE);
	Free_Data();
	if (!__OpenFile(lBuf, sizeof(lBuf)/sizeof(wchar_t), L"VFS\0*.vfs\0\0", &filenameoff, &extoff)) return;
	Lock_Controls(TRUE);
	StrnnCpyW(g_Root_Title, lBuf+filenameoff, sizeof(g_Root_Title), MAX_PATH);
	Set_Status_Text(L"Opening...");
	if (!Parse(lBuf, extoff)){
		SetWindowTextW(g_hMain_Wnd, WINDOW_TITLE);
		g_Root_Title[0] = 0;
	} else {
		g_Open_File = TRUE;
		pStr = Strip_File_PathW(lBuf);
		StrCpyW(g_Current_File, pStr);
		//StrnCpyW(g_Current_Dir, lBuf, PtrToInt(pStr-lBuf)/sizeof(wchar_t));
	}
	Set_Status_Text(L"Ready.");
	Lock_Controls(FALSE);//*/
}

/*
Procession file new request
*/

void Do_New(){
	int filenameoff, extoff;
	wchar_t lBuf[1024];
	register wchar_t *pStr;
	SetWindowTextW(g_hMain_Wnd, WINDOW_TITLE);
	Free_Data();
	lBuf[0] = 0;
	if (!__SaveFile(lBuf, sizeof(lBuf)/sizeof(wchar_t), L"VFS\0*.vfs\0\0", L"*.vfs", &filenameoff, &extoff)) return;
	Lock_Controls(TRUE);
	StrnnCpyW(g_Root_Title, lBuf+filenameoff, sizeof(g_Root_Title), MAX_PATH);
	Set_Status_Text(L"Creating...");
	if (!Create(lBuf, extoff)){
		SetWindowTextW(g_hMain_Wnd, WINDOW_TITLE);
		g_Root_Title[0] = 0;
	} else {
		g_Open_File = TRUE;
		pStr = Strip_File_PathW(lBuf);
		StrCpyW(g_Current_File, pStr);
		//StrnCpyW(g_Current_Dir, lBuf, PtrToInt(pStr-lBuf)/sizeof(wchar_t));
	}
	Set_Status_Text(L"Ready.");
	Lock_Controls(FALSE);//*/
}

/*
Process file closing request
*/

void Do_Close(){
	if (!g_Open_File) return;
	//Log_Formated("Memory usage before closing: %d\n", g_Active_Memory);
	Free_Data();
	SetWindowTextW(g_hMain_Wnd, WINDOW_TITLE);
	Enable_Progress_Bar(FALSE);
	Set_Status_Text(L"Ready.");
	//Log_Formated("Memory usage after closing: %d\n", g_Active_Memory);
	g_Open_File = FALSE;//*/
}

/*
Extract single file
*/

void Do_Extract(){
	/*if (!g_Open_File) return;
	if (!g_pExtraction_Src) return;
	if (g_pExtraction_Src->m_pEntry->m_Type == TYPE_FILE){
		//pFile = (LPENTRYFILE)g_pEntry_Header;
		//StrCpy(g_Extraction_Dir, g_Current_Dir);
		crt__snprintf(g_Extraction_Dir, sizeof(g_Extraction_Dir), "%s%.12s", g_Current_Dir, g_pExtraction_Src->m_pEntry->m_Name);
		//StrnCat(g_Extraction_Dir, pFile->m_pTitle, sizeof(g_Extraction_Dir));
		if (__SaveFile(g_Extraction_Dir, sizeof(g_Extraction_Dir), "All files\0*.*\0\0", "*.*", NULL, NULL))
			CloseHandle(CreateThread(NULL, 0, Extraction_Thread, NULL, 0, &g_TID));
	} else if (g_pExtraction_Src->m_pEntry->m_Type == TYPE_DIRECTORY){
		StrCpy(g_Extraction_Dir, g_Current_Dir);
		if (Browse_For_Folder(g_Extraction_Dir))
			CloseHandle(CreateThread(NULL, 0, Extraction_Thread, NULL, 0, &g_TID));
	}//*/
}

/*
Extract all files
*/

void Do_Extract_All(){

	if (!g_Open_File) return;
	Free_Thread_Data();
	//g_pExtraction_Src = &g_pFS_Data[0];
	StrCpyW(g_Extraction_Dir, L"F:\\weelol\\Neuer Ordner (2)");
	if (Browse_For_Folder(g_Extraction_Dir))
		CloseHandle(CreateThread(NULL, 0, Extraction_Thread, NULL, 0, &g_TID));//*/
}

void VFS_CALL FileImportCB(const ptr pData, int type, size_64 progress, size_64 total, ptr pCookie){
	PVFSFILEINFO fInfo;
	switch (type){
		case PROGRESS_TYPE_START:
			Enable_Progress_Bar(TRUE);
			Init_Progress(0x7FFFFFFF);
			return;
		case PROGRESS_TYPE_NEXT:
			Set_Progress((int)(((float)0x7FFFFFFF)/(float)total*(float)progress));
			return;
		case PROGRESS_TYPE_END:
			Enable_Progress_Bar(FALSE);
			return;
		case PROGRESS_TYPE_FILEINFO:
			fInfo = (PVFSFILEINFO)pData;
			*((HTREEITEM*)pCookie) = AddFileToTree(g_pExtraction_Src->m_hTreeItem, fInfo, g_pExtraction_Src->m_pFileData);
			return;
	}
	return;
}

DWORD CALLBACK Import_Thread(LPVOID pArg){
	int filenameoff, extoff;
	wchar_t lBuf[1024];
	PROGESSCB cb;

	HTREEITEM hItem;

	cb.m_pCB = FileImportCB;
	cb.m_pCookie = &hItem;
	cb.m_UseInternalCache = TRUE;

	//Lock_Controls(TRUE);
	if (!g_Open_File || !g_pExtraction_Src) goto __exit;
	if (!__OpenFile2(lBuf, sizeof(lBuf)/sizeof(wchar_t), L"All files\0*.*\0\0", &filenameoff, &extoff)) goto __exit;
	Set_Status_Text_Fmt(L"Importing %s...", lBuf+filenameoff);
	if (!g_pVFS->VFSEmbedFiledataFile(lBuf+filenameoff, g_pExtraction_Src->m_pFileData, false, &cb)){
		goto __exit;
	}
	//Lock_Controls(FALSE);
	Set_Status_Text(L"");
	SendMessageW(g_hTree_View, TVM_SELECTITEM, (WPARAM)(TVGN_CARET), (LPARAM)hItem);
	SetFocus(g_hTree_View);
	ExitThread(0);

__exit:
	Lock_Controls(FALSE);
	Set_Status_Text(L"");
	g_TID = 0;
	ExitThread(0);
}


void Do_Import_File(){
	if (!g_Open_File) return;
	Free_Thread_Data();
	CloseHandle(CreateThread(NULL, 0, Import_Thread, NULL, 0, &g_TID));//*/
}

void Do_Import_Dir(){
}

void Do_Create_Dir(){
	wchar_t lBuf[512];
	HTREEITEM hItem;
	ptr pFileData;
	if (!g_Open_File || !g_pExtraction_Src || g_pExtraction_Src->m_Type != VFS_TYPE_DIRECTORY) return;
	if (!(pFileData = g_pVFS->VFSCreateDirectory(g_pExtraction_Src->m_pFileData, L"New folder", lBuf, 512))) return;

	g_pExtraction_Src->m_DirectoryCount++;

	hItem = AddObject(g_pExtraction_Src->m_hTreeItem, lBuf, VFS_TYPE_DIRECTORY, g_pExtraction_Src->m_pFileData, pFileData);
	SendMessageW(g_hTree_View, TVM_EXPAND, (WPARAM)TVE_EXPAND, (LPARAM)g_pExtraction_Src->m_hTreeItem);
	SetFocus(g_hTree_View);
	SendMessageW(g_hTree_View, TVM_EDITLABELW, 0, (LPARAM)hItem);
}

void Do_Delete(){
	PVFSITEM pItem, pParent;
	VFSFILEINFO info;
	if (!g_pExtraction_Src) return;
	if (g_pVFS->VFSGetFiledataUID(g_pExtraction_Src) == VFS_ROOT_UID) return;
	g_pVFS->VFSQueryFileInfo(&info, g_pExtraction_Src->m_pFileData, g_pName_Cache, NAME_CACHE_SIZE);
	if (g_pExtraction_Src->m_Type == VFS_TYPE_FILE){
		Lock_Controls(TRUE);
		pItem = (PVFSITEM)g_pVFS->VFSGetRuntimeUserData(g_pExtraction_Src->m_pFileData);
		pParent = (PVFSITEM)g_pVFS->VFSGetRuntimeUserData(g_pExtraction_Src->m_pParentFileData);
		Set_Status_Text_Fmt(L"Deleting %s...", info.m_pName);
		if (!g_pVFS->VFSDeleteFiledataFile(g_pExtraction_Src->m_pFileData)){
			Lock_Controls(FALSE);
			Set_Status_Text_Fmt(L"Failed to delete %s", info.m_pName);
			return;
		}
		pItem->m_TotalSize *= -1;
		UpdateUnrolled(pItem, pParent);
		SendMessageW(g_hTree_View, TVM_DELETEITEM, 0, (LPARAM)g_pExtraction_Src->m_hTreeItem);
		g_pExtraction_Src = NULL;
		SendMessageW(g_hTree_View, TVM_SELECTITEM, (WPARAM)(TVGN_CARET), (LPARAM)pParent->m_hTreeItem);
		delete pItem;
		g_pVFS->PartialFlush();
		Lock_Controls(FALSE);
		Set_Status_Text(L"Ready.");
		return;
	}

}

void Do_Rename_File(){
	if (!g_Open_File || !g_pExtraction_Src) return;
	SetFocus(g_hTree_View);
	SendMessageW(g_hTree_View, TVM_EDITLABELW, 0, (LPARAM)g_pExtraction_Src->m_hTreeItem);
}

BOOL Finish_Rename_File(LPNMTVDISPINFOW pDispInfo){
	PVFSITEM pItem;
	pItem = (PVFSITEM)pDispInfo->item.lParam;
	if (!pItem) return FALSE;

	if (!pDispInfo->item.pszText) return FALSE;

	switch(pItem->m_Type){
		case VFS_TYPE_DIRECTORY:
			return g_pVFS->VFSRenameFiledataFolder(pItem->m_pFileData, pDispInfo->item.pszText) ? TRUE : FALSE;
		case VFS_TYPE_FILE:
			return g_pVFS->VFSRenameFiledataFile(pItem->m_pFileData, pDispInfo->item.pszText) ? TRUE : FALSE;
		default:
			return FALSE;

	}
}

/*
Find a file
*/

void Do_Find(){
}

/*
Find next file
*/

void Do_Find_Next(){
}

const wchar_t* GetNoEnumName(int noenum){
	switch (noenum){
		case MFT_ELEMENT_NO_ENUM:
			return L"no enum";
		case MFT_ELEMENT_NO_ENUM_ALL:
			return L"no enum all";
		default:
			return L"enum";
	}
}

/*
Handle selection change
*/

void Do_Selection_Change(LPNMTREEVIEW pNotification){
	PVFSITEM pItem;
	if (pNotification->itemNew.hItem == NULL){
		Set_Status_Text(L"");
		g_pExtraction_Src = NULL;
		return;
	}

	pItem = (PVFSITEM)pNotification->itemNew.lParam;

	if (pItem && pItem->m_Type == VFS_TYPE_FILE){
		if (g_No_Enum)
			Set_Status_Text_Fmt(L"%I64u bytes", pItem->m_TotalSize);
		else			
			Set_Status_Text_Fmt(L"%I64u bytes; %s", pItem->m_TotalSize, GetNoEnumName(g_pVFS->VFSGetNoEnum(pItem->m_pFileData)));
		g_pExtraction_Src = pItem;
	} else if (pItem && pItem->m_Type == VFS_TYPE_DIRECTORY){
		if (g_No_Enum)
			Set_Status_Text_Fmt(L"%I64u bytes in %d directories and %d files",
				pItem->m_TotalSize,
				pItem->m_DirectoryCount,
				pItem->m_FileCount
				);
		else
			Set_Status_Text_Fmt(L"%I64u bytes in %d directories and %d files; %s",
				pItem->m_TotalSize,
				pItem->m_DirectoryCount,
				pItem->m_FileCount,
				GetNoEnumName(g_pVFS->VFSGetNoEnum(pItem->m_pFileData))
				);
		g_pExtraction_Src = pItem;
	} else {
		g_pExtraction_Src = NULL;
		Set_Status_Text(L"");
	}
}

/*
Message handling loop
*/

void mainLoop(){
	MSG msg;
	memset(&msg, 0, sizeof(MSG));
	g_hAccelerators = LoadAcceleratorsW(g_hInstance, MAKEINTRESOURCEW(IDR_HOTKEYS));
	while (msg.message != WM_QUIT){
		if (GetMessageW(&msg, NULL, 0U, 0U)) {
			if (!TranslateAcceleratorW(g_hMain_Wnd, g_hAccelerators, &msg)){
				TranslateMessage(&msg);
				DispatchMessageW(&msg);
			}
		}
	}
}

/*
Main window proc
*/

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	LPNMTREEVIEW pNotification;
	LPNMTVDISPINFOW pDispInfo;
	switch (uMsg) {
		case WM_CREATE:
			g_hMain_Wnd = hWnd;
			if (!Create_Controls(hWnd)){
				MessageBoxA(hWnd, "Error creating controls.", "Error.", MB_ICONEXCLAMATION);
				DestroyWindow(hWnd);
				break;
			}
			//Position_Windows();
			break;
		case WM_SIZE:
			SendMessage(g_hStatus_Bar, uMsg, wParam, lParam);
			Position_Controls();			
			break;
		case WM_COMMAND:		

			switch (LOWORD(wParam)){
		case ID_OPEN:
			Do_Open();
			break;
		case ID_NEW:
			Do_New();
			break;
		case ID_CLOSE:
			Do_Close();
			break;
		case ID_EXTRACT:
			Do_Extract();
			break;
		case ID_EXTRACTALL:
			Do_Extract_All();
			break;
		case ID_IMPORT_DIR:
			Do_Import_Dir();
			break;
		case ID_IMPORT_FILE:
			Do_Import_File();
			break;
		case ID_CREATE_DIR:
			Do_Create_Dir();
			break;
		case ID_DELETE:
			Do_Delete();
			break;
		case ID_EXIT:
			DestroyWindow(g_hMain_Wnd);
			break;
		case ID_FIND:
			Do_Find();
			break;
		case ID_FINDNEXT:
			Do_Find_Next();
			break;
		case ID_RENAME:
			Do_Rename_File();
			break;
		default:
			break;
			}
			break;
		case WM_CLOSE:
			DestroyWindow(g_hMain_Wnd);
			break;
		case WM_NOTIFY:
			pNotification = (LPNMTREEVIEW)lParam;
			pDispInfo = (LPNMTVDISPINFO)lParam;
			if (pNotification->hdr.code == TVN_SELCHANGED) Do_Selection_Change(pNotification);
			if (pNotification->hdr.code == TVN_BEGINLABELEDITW)
				return FALSE;
			if (pNotification->hdr.code == TVN_ENDLABELEDITW)
				return Finish_Rename_File(pDispInfo);
			break;
		case WM_DESTROY:
			Do_Close();
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
	
}

/*
Application entry point
*/

int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* lpCmdLine, int nCmdShow){	
	INITCOMMONCONTROLSEX ex;
	WNDCLASSEXW wcls;
	int i;

	i = _MSC_VER;

	g_hInstance = hInstance;

	ex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	ex.dwICC = 
		ICC_TREEVIEW_CLASSES |
		ICC_BAR_CLASSES |
		ICC_PROGRESS_CLASS;

	SetDefaultMemoryFunctions();

	g_hVFS = LoadLibraryW(VFS_MODULE_NAME);
	if (!g_hVFS || !(g_pfCreateNewInstance = (pfCreateNewInstance)GetProcAddress(g_hVFS, VFS_INSTANCE_PROC))){
		MessageBoxW(0, L"Failed to load a required library.", L"Fatal error", MB_ICONHAND);
		return 0;
	}
	if (!(g_pVFS = g_pfCreateNewInstance())){
		MessageBoxW(0, L"Failed to create a VFS instance.", L"Fatal error", MB_ICONHAND);
		return 0;
	}

	
	InitCommonControlsEx(&ex);
	CoInitialize(NULL);

	
	memset(&wcls, 0, sizeof(WNDCLASSEX));
	GetCurrentDirectory(sizeof(g_Application_Dir), g_Application_Dir);


	wcls.lpszClassName = CLASS_NAME;
	wcls.lpszMenuName = NULL;
	wcls.cbSize = sizeof(WNDCLASSEX);
	wcls.style = 0;
	wcls.lpfnWndProc = WndProc;
	wcls.hIcon = NULL;// g_Application_Icon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_MAIN));
	wcls.lpszMenuName = MAKEINTRESOURCEW(IDR_MAINMENU);
	wcls.hIconSm = NULL;
	wcls.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcls.cbClsExtra = 0;
	wcls.cbWndExtra = 0;
	wcls.hbrBackground = NULL;
	wcls.hInstance = g_hInstance;
	RegisterClassExW(&wcls);
	g_pName_Cache = (wchar_t*)g_pAllocator((32*1024)+1);


	
	g_hMain_Wnd = CreateWindowExW(0, CLASS_NAME, WINDOW_TITLE, 
		WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, 
		NULL, g_hInstance, NULL);

	if (g_hMain_Wnd){
		mainLoop();
	} else{
		MessageBoxA(0, "Error creating window.", "Error", MB_ICONEXCLAMATION);
	}

	g_pFreeer(g_pName_Cache);

	DeleteObject(g_Application_Icon);
	UnregisterClassW(CLASS_NAME, g_hInstance);

	delete g_pVFS;
	FreeLibrary(g_hVFS);
	
	ExitProcess(0);
}