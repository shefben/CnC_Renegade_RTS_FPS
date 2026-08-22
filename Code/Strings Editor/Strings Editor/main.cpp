#include <windows.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <commctrl.h>
#include <commdlg.h>
#include "resource.h"
#include "Translation.h"
#include "LList.h"
#include "VectorAndListTemplateClasses.h"
#include "SoundAndSettings.h"
#include "Fileclass.h"
#include "main.h"
#include "search.h"


#define CLASS_NAME L"TDB_WND"
#define WND_TITLE L"Advanced string editor"



enum { sort_id_desc = 0, sort_id, sort_eng_text };


/*-- Window handles --*/
HWND g_MainWnd = NULL;
HWND g_StringList = NULL;
HWND g_Tabs = NULL;
HWND g_Status_Bar = NULL;
/*-- All sorts of other handles --*/
HIMAGELIST g_hImagelist;
HINSTANCE g_hInstance;
/*-- Other stuff --*/
bool g_Is_File_Open = false;
bool g_Sort_ID_Up = true;
bool g_Sort_Desc_Up = true;
bool g_Sort_Eng_Up = true;
int g_ProcSortDirections[3] = { 1, 1, 1 };
int g_Column = 0;
bool g_NoSound = true;
bool g_3DSound = true;

/*-- Forward decls --*/
void Position_Windows();
bool Sort(bool update_only = false);
void LogFormated(wchar_t *fmt, ...);
void Handle_Tab_Change(int idx, bool update_only = false);

void Set_Status_Text(wchar_t *text){
	SendMessage(g_Status_Bar, SB_SETTEXT, 0, (LPARAM)text);
}

void Set_Status_Text_Formated(wchar_t *format, ...){
	wchar_t lBuf[512];
	va_list va;
	va_start(va, format);
	_vsnwprintf(lBuf, 512, format, va);
	va_end(va);
	SendMessage(g_Status_Bar, SB_SETTEXT, 0, (LPARAM)lBuf);
}

void SelectItem(TDBObjClass*obj){
	int i, nStrs, ItemID = obj->Get_ID();
	LVITEM item;
	//POINT p;
	RECT r = { 0, 0, 0, 0 };
	nStrs = (int)SendMessage(g_StringList, LVM_GETITEMCOUNT, 0, 0);
	for (i = 0; i < nStrs; i++){
		ZeroMemory(&item, sizeof(item));
		item.iItem = i;
		item.mask = LVIF_PARAM;
		SendMessage(g_StringList, LVM_GETITEM, 0, (LPARAM)&item);
		if (!item.lParam) continue;
		if (((TDBObjClass*)item.lParam)->Get_ID() == ItemID){
			ZeroMemory(&item, sizeof(item));
			item.mask = LVIF_STATE;
			item.stateMask = LVIS_SELECTED;
			item.state = LVIS_SELECTED;
			item.iItem = i;
			SendMessage(g_StringList, LVM_SETITEM, 0, (LPARAM)&item);
			//SendMessage(g_StringList, LVM_GETITEMPOSITION, item.iItem, (LPARAM)&p);
			r.left = LVIR_BOUNDS;
			SendMessage(g_StringList, LVM_GETITEMRECT, i, (LPARAM)&r);
			SendMessage(g_StringList, LVM_SCROLL, 0, r.top-(r.bottom-r.top));
			return;
		}
	}
}

void Goto(int idx){
	int cid, i, ItemID;
	CatergoryContainer *cat;
	if (idx < 0 || idx >= TDataBase::m_Objects.m_nItemCount) return;
	TDBObjClass *obj = TDataBase::m_Objects[idx];
	if (!obj) return;
	cid = obj->Get_Category_ID();
	ItemID = obj->Get_ID();
	for (i = 0; i < TDataBase::m_Containers.m_nItemCount; i++){
		cat = TDataBase::m_Containers[i];
		if (cat->m_This->Get_ID() == cid){
			SendMessage(g_Tabs, TCM_SETCURSEL, i, 0);
			Handle_Tab_Change(i);
			SelectItem(obj);
			return;
		}
	}	
}

void Toogle_NoSound(){
	HMENU hMenu = GetMenu(g_MainWnd);
	if (!hMenu) return;
	MENUITEMINFO info;
	memset(&info, 0, sizeof(info));
	info.cbSize = sizeof(info);
	info.fMask = MIIM_STATE;	
	if (!GetMenuItemInfo(hMenu, ID_SHOWNO, FALSE, &info)) return;
	info.fState = (info.fState & MFS_CHECKED) ? (info.fState & ~MFS_CHECKED) : (info.fState | MFS_CHECKED);
	g_NoSound = (info.fState & MFS_CHECKED) ? true : false;
	SetMenuItemInfo(hMenu, ID_SHOWNO, FALSE, &info);
}

void Toggle_3DSound(){
	HMENU hMenu = GetMenu(g_MainWnd);
	if (!hMenu) return;
	MENUITEMINFO info;
	memset(&info, 0, sizeof(info));
	info.cbSize = sizeof(info);
	info.fMask = MIIM_STATE;	
	if (!GetMenuItemInfo(hMenu, ID_SHOW3D, FALSE, &info)) return;
	info.fState = (info.fState & MFS_CHECKED) ? (info.fState & ~MFS_CHECKED) : (info.fState | MFS_CHECKED);
	g_3DSound = (info.fState & MFS_CHECKED) ? true : false;
	SetMenuItemInfo(hMenu, ID_SHOW3D, FALSE, &info);
}

/*-- Show the last error in a message box --*/

void MessageError(){
	wchar_t lpMsgBuf[512];
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_ENGLISH_US),
		(LPTSTR)lpMsgBuf,
		512, NULL );
	MessageBox(0, lpMsgBuf, L"ERROR", MB_ICONERROR);	
}

/*-- Convert a string to a widestring --*/
wchar_t *s2wdup(const char *str){
	int i = (int)strlen(str)+1;
	wchar_t *temp = (wchar_t*)malloc(i*2);
	_swprintf(temp, L"%S", str);
	return temp;
}

/*-- Show the last error as debugstring --*/

void LogError(){
	wchar_t lpMsgBuf[512];
	unsigned long error = GetLastError();
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		error,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_ENGLISH_US),
		(LPTSTR)lpMsgBuf,
		512, NULL );
	//wcscat(lpMsgBuf, L"\n");
	//OutputDebugStr(lpMsgBuf);	
	LogFormated(L"ERROR %d: %s", error, lpMsgBuf);
}

/*-- Log formated --*/

void LogFormated(wchar_t *fmt, ...){
	wchar_t lBuf[512];
	va_list va;
	va_start(va, fmt);
	_vsnwprintf(lBuf, 512, fmt, va);
	va_end(va);
	OutputDebugStr(lBuf);	
}

/*-- Inline function to create controls and windows --*/

__inline HWND Create_Object(wchar_t *className, int style, int exstyle, const wchar_t *text, int x, int y, int width, int height, HWND parent, const HINSTANCE hInstance, bool _3D = true){
	HWND temp;
	temp = CreateWindowEx(/*(_3D ? WS_EX_CLIENTEDGE  : 0 )|*/ exstyle, className, text, WS_CHILD|WS_VISIBLE|style, x, y, width, height, parent, NULL, hInstance, NULL);
	SendMessage(temp, WM_SETFONT, (WPARAM)(HFONT)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(FALSE, 0));
	return temp;
}

/*-- File open dialog --*/

bool __OpenFile(wchar_t *buf, int bufsize, int *FileOffset, int *ExtensionOffset){
	wchar_t lBuf[MAX_PATH] = L"";
	OPENFILENAME of;

	memset(&of, 0, sizeof(OPENFILENAME));

	of.lStructSize = sizeof(OPENFILENAME);
	of.hwndOwner = g_MainWnd;
	of.hInstance = NULL;
	of.lpstrFilter = L"String table\0*.tdb\0\0";
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

	if (!GetOpenFileName(&of)) return false;
	_snwprintf(buf, bufsize, L"%s", of.lpstrFile);
	if (FileOffset)
		*FileOffset = of.nFileOffset;
	if (ExtensionOffset)
		*ExtensionOffset = of.nFileExtension;
	_snwprintf(lBuf, MAX_PATH, L"%s - %s", WND_TITLE, buf+of.nFileOffset);
	SetWindowText(g_MainWnd, lBuf);
	return true;
}

/*-- File open dialog --*/

bool __SaveFile(wchar_t *buf, int bufsize, int *FileOffset, int *ExtensionOffset){
	wchar_t lBuf[MAX_PATH] = L"";
	OPENFILENAME of;

	memset(&of, 0, sizeof(OPENFILENAME));

	of.lStructSize = sizeof(OPENFILENAME);
	of.hwndOwner = g_MainWnd;
	of.hInstance = NULL;
	of.lpstrFilter = L"String table\0*.tdb\0\0";
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
	of.lpstrDefExt = L"tdb";
	of.lCustData = NULL;
	of.lpfnHook = NULL;
	of.lpTemplateName = NULL;	
	if (!GetSaveFileName(&of)) return false;
	_snwprintf(buf, bufsize, L"%s", of.lpstrFile);
	if (FileOffset)
		*FileOffset = of.nFileOffset;
	if (ExtensionOffset)
		*ExtensionOffset = of.nFileExtension;
	_snwprintf(lBuf, MAX_PATH, L"%s - %s", WND_TITLE, buf+of.nFileOffset);
	SetWindowText(g_MainWnd, lBuf);
	return true;
}

/*-- The sort procedure --*/

int SortProc(TDBObjClass *obj1, TDBObjClass *obj2, int col){
	if (!obj1 || !obj2) return 0;
	int res = 0;
	switch (col){
		case 1:
			if (obj1->Get_ID() < obj2->Get_ID())
				res = -1;
			else if (obj1->Get_ID() > obj2->Get_ID())
				res = 1;
			else res = 0;			
			break;
		case 0:
			res = stricmp(obj1->Get_ID_Desc(), obj2->Get_ID_Desc());			
			break;
		case 2:
			res = stricmp(obj1->Get_English_String(), obj2->Get_English_String());			
			break;
		default:
			break;
	}
	res *= g_ProcSortDirections[col];
	return res;
}

/*-- Close file and free everything --*/

void Do_Close(){	
	SetWindowText(g_MainWnd, WND_TITLE);
	TDataBase::Free_All();
	SendMessage(g_StringList, LVM_DELETEALLITEMS, 0, 0);
	SendMessage(g_Tabs, TCM_DELETEALLITEMS, 0, 0);
	g_Is_File_Open = false;
	Position_Windows();
}

/*-- Open and apply a file --*/

void Do_Open(){
	Do_Close();
	TCITEM item;
	RawFileClass r;
	ChunkLoadClass load(&r);
	wchar_t *temp;
	int j;

	if (g_Is_File_Open){
		TDataBase::Free_All();
		g_Is_File_Open = false;
	}
	wchar_t lName[MAX_PATH];
	if (!__OpenFile(lName, MAX_PATH, NULL, NULL)) return;	
	r.Open(lName, fo_read);
	if (!r.Is_Open()) return;
	g_Is_File_Open = true;

	TDataBase::Load(load);
	TDataBase::Build();
	for (int i = 0; i < TDataBase::m_Containers.m_nItemCount; i++){
		j = (int)SendMessage(g_Tabs, TCM_GETITEMCOUNT, 0, 0);
		item.mask = TCIF_PARAM | TCIF_TEXT;
		item.lParam = (LPARAM)TDataBase::m_Containers[i];
		temp = s2wdup(TDataBase::m_Containers[i]->m_This->Get_Name());
		item.pszText = (LPWSTR)temp;
		SendMessage(g_Tabs, TCM_INSERTITEM, (WPARAM)j, (LPARAM)&item);
		free(temp);
	}
	r.Close();
	Position_Windows();
	Sort();
}

/*-- Handle tab change --*/

void Handle_Tab_Change(int idx, bool update_only){
	TCITEM item;
	LVITEM lv;
	TDBObjClass *o;
	bool has_snd;
	bool do_disp;
	wchar_t lBuf[256];
	item.mask = TCIF_PARAM;
	if (!SendMessage(g_Tabs, TCM_GETITEM, (WPARAM)idx, (LPARAM)&item)) return;
	if (!update_only)
		SendMessage(g_StringList, LVM_DELETEALLITEMS, 0, 0);
	CatergoryContainer *cat = (CatergoryContainer*)item.lParam;
	if (!cat) return;
	memset(&lv, 0, sizeof(lv));
	lv.iItem = 0;
	int _idx = 0;
	for (int i = 0; i < cat->m_Objects.m_nItemCount; i++){
		o = cat->m_Objects[i];
		has_snd = Has_Sound(o, NULL, &do_disp);
		if (!has_snd && !do_disp) continue;
		lv.iSubItem = 0;
		lv.iItem = _idx;
		lv.mask = LVIF_TEXT | LVIF_PARAM;
		lv.lParam = (LPARAM)cat->m_Objects[i];
		lv.pszText = lBuf;
		_snwprintf(lBuf, 256, L"%S", o->Get_ID_Desc());
		if (update_only)
			SendMessage(g_StringList, LVM_SETITEM, 0, (LPARAM)&lv);
		else
			SendMessage(g_StringList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lv);
		lv.iSubItem = 1;
		lv.iItem = _idx;
		lv.mask = LVIF_TEXT;
		lv.pszText = lBuf;
		_snwprintf(lBuf, 256, L"%d", o->Get_ID());
		SendMessage(g_StringList, LVM_SETITEM, 0, (LPARAM)&lv);

		lv.iSubItem = 2;
		lv.iItem = _idx;
		lv.mask = LVIF_TEXT; 
		lv.pszText = lBuf;
		_snwprintf(lBuf, 256, L"%s", has_snd ? L"Yes" : L"No");		
		SendMessage(g_StringList, LVM_SETITEM, 0, (LPARAM)&lv);

		lv.iSubItem = 3;
		lv.iItem = _idx;
		lv.mask = LVIF_TEXT;
		lv.pszText = lBuf;
		_snwprintf(lBuf, 256, L"%S", o->Get_English_String());
		SendMessage(g_StringList, LVM_SETITEM, 0, (LPARAM)&lv);
		idx++;
	}
}

bool InsertIntoSortedArray(DynamicVectorClass<TDBObjClass*> *objs, TDBObjClass *obj){
	int cItems = objs->m_nItemCount;	
	for (int i = 0; i < cItems; i++){
		if (SortProc((*objs)[i], obj, g_Column) < 0){
			return objs->Insert(i, obj);			
		}
	}
	return objs->Add(obj);	
}

bool Sort(bool update_only){
	DynamicVectorClass<TDBObjClass*> v;
	DynamicVectorClass<TDBObjClass*> *p;
	TCITEM item;
	int idx;
	int cItems;
	CatergoryContainer *cat;
	idx = (int)SendMessage(g_Tabs, TCM_GETCURSEL, 0, 0);
	item.mask = TCIF_PARAM;
	SendMessage(g_Tabs, TCM_GETITEM, (WPARAM)idx, (LPARAM)&item);
	cat = (CatergoryContainer*) item.lParam;
	if (!cat) return false;
	p = &cat->m_Objects;
	cItems = cat->m_Objects.m_nItemCount;
	for (int i = 0; i < cItems; i++){
		if (!InsertIntoSortedArray(&v, (*p)[i]))
			return false;
	}
	cat->m_Objects = v;
	Handle_Tab_Change(idx, update_only);
	return true;
	//SortProc
}

/*-- Re-position/size the controls --*/

void Position_Windows(){
	RECT mrect, srect, trect;
	GetClientRect(g_MainWnd, &mrect);
	GetClientRect(g_Tabs, &trect);
	GetClientRect(g_Status_Bar, &srect);
	TabCtrl_AdjustRect(g_Tabs, FALSE, &trect);
	SetWindowPos(g_Tabs, 0, mrect.left, mrect.top, mrect.right, trect.top+10, SWP_NOZORDER);	
	SetWindowPos(g_StringList, HWND_TOP, mrect.left, trect.top+10, mrect.right, mrect.bottom-srect.bottom-trect.top-10, 0);	
}

/*-- Create the controls --*/

bool Create_Windows(HWND hWnd){
	LVCOLUMN col;
	g_Status_Bar = Create_Object(STATUSCLASSNAME, 0, 0, NULL, 0, 0, 0, 0, hWnd, g_hInstance, false);
	g_Tabs = Create_Object(WC_TABCONTROL, TAB_STL, 0, NULL, 0, 20, 500, 500, hWnd, g_hInstance, false);
	g_StringList = Create_Object(WC_LISTVIEW, LIST_STL, 0, NULL, 0, 100, 500, 40, hWnd, g_hInstance, true);	
	SendMessage(g_StringList, LVM_SETEXTENDEDLISTVIEWSTYLE, (WPARAM)(LIST_EX_STL), (LPARAM)(LIST_EX_STL));
	col.mask = LVCF_TEXT | LVCF_WIDTH;
	col.cx = 200;
	col.pszText = L"ID (string)";
	SendMessage(g_StringList, LVM_INSERTCOLUMN, (WPARAM)0, (LPARAM)&col);
	col.cx = 100;
	col.pszText = L"ID (numeric)";
	SendMessage(g_StringList, LVM_INSERTCOLUMN, (WPARAM)1, (LPARAM)&col);

	col.cx = 40;
	col.pszText = L"SND";
	SendMessage(g_StringList, LVM_INSERTCOLUMN, (WPARAM)2, (LPARAM)&col);

	col.cx = 600;
	col.pszText = L"English text";
	SendMessage(g_StringList, LVM_INSERTCOLUMN, (WPARAM)3, (LPARAM)&col);

	return (g_Status_Bar && g_Tabs && g_StringList);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	LPNMHDR not;
	LPNMLISTVIEW lv;
	LPNMITEMACTIVATE act;
	LVITEM item;
	ISoundObject *snd;
	switch (uMsg) {
		case WM_CREATE:
			g_MainWnd = hWnd;
			if (!Create_Windows(hWnd)){
				MessageBox(hWnd, L"Error creating controls.", L"Error.", MB_ICONEXCLAMATION);
				DestroyWindow(hWnd);
				break;
			}
			Load_Settings();
			//Position_Windows();
			break;
		case WM_SIZE:			
			SendMessage(g_Status_Bar, uMsg, wParam, lParam);
			Position_Windows();
			break;
		case WM_COMMAND:		

			switch (LOWORD(wParam)){
				/*-- New file --*/
		case ID_SHOWNO:
			Toogle_NoSound();
			Handle_Tab_Change((int)SendMessage(g_Tabs, TCM_GETCURSEL, 0, 0));
			break;
		case ID_SHOW3D:
			Toggle_3DSound();
			Handle_Tab_Change((int)SendMessage(g_Tabs, TCM_GETCURSEL, 0, 0));
			break;
		case ID_NEW:
			break;
			/*-- Open existing file --*/
		case ID_OPEN:
			/*if (__OpenFile(lBuf, sizeof(lBuf), NULL, NULL))
				Open_File(lBuf);*/
			Do_Open();
			break;
			/*-- Save file --*/
		case ID_SAVE:
			break;
			/*-- Save file under a different filename --*/
		case ID_SAVEAS:
			break;
			/*-- Close the file --*/
		case ID_CLOSE_FILE:
			Do_Close();
			//Close_File();
			break;
			/*-- Exit the program --*/
		case ID_EXIT:
			DestroyWindow(g_MainWnd);
			break;
			/*-- Got to entry by integer ID --*/
		case ID_GOTO:		
			DoGoto();
			break;
			/*-- Find a string by the ID description --*/
		case ID_FIND_BY_ID:
			DoFindByID();
			break;
			/*-- Find by the actual string --*/
		case ID_FIND_BY_STRING:
			DoFindByString();
			break;
			/*-- Find by the english string --*/
		case ID_FIND_BY_ENG_STRING:
			DoFindByEnglishString();
			break;
		case ID_SNDAPIOPTIONS:
			if (g_iSoundDatabase && g_hKey){
				g_iSoundDatabase->Do_Options(g_hKey);
				g_iSoundDatabase->Apply_Options(g_hKey);
			}
			break;
		case ID_SNDAPICONVERT:
			if (g_iSoundDatabase)
				g_iSoundDatabase->Do_Convert();
			break;
			/*-- Nothing yet --*/
		default:
			break;
			}
			break;
		case WM_CLOSE:
			DestroyWindow(g_MainWnd);
			break;
		case WM_NOTIFY:

			not = (LPNMHDR)lParam;
			act = (LPNMITEMACTIVATE)lParam;
			if (act->hdr.hwndFrom == g_StringList && act->hdr.code == NM_DBLCLK && act->iItem != -1){
				item.iItem = act->iItem;
				item.mask = LVIF_PARAM;
				SendMessage(g_StringList, LVM_GETITEM, 0, (LPARAM)&item);
				if (item.lParam)
					TDBEdit((TDBObjClass*)item.lParam);
				//Goto((int)item.lParam);

				return TRUE;
			}
			if (act->hdr.hwndFrom == g_StringList && act->hdr.code == NM_RCLICK && act->iItem != -1){
				item.iItem = act->iItem;
				item.mask = LVIF_PARAM;
				item.iSubItem = 2;
				if (!SendMessage(g_StringList, LVM_GETITEM, 0, (LPARAM)&item)) return TRUE;				
				if (item.lParam && g_iSoundDatabase){
					if (Has_Sound((TDBObjClass*)item.lParam, &snd, NULL))
						g_iSoundDatabase->Play_Sound_Object(snd);
				}

				return TRUE;
			}
			
			//LogFormated(L"C: %.8x W: %.8x I: %.8x\n", not->code, not->hwndFrom, not->idFrom);
			if (not->hwndFrom == g_Tabs){
				if (not->code == TCN_SELCHANGE)
					Handle_Tab_Change((int)SendMessage(g_Tabs, TCM_GETCURSEL, 0, 0));
			}
			if (not->hwndFrom == g_StringList){
				if (not->code == LVN_COLUMNCLICK){
					lv = (LPNMLISTVIEW)not;
					if (g_Column == lv->iSubItem)
						g_ProcSortDirections[lv->iSubItem] *= -1;
					else {
						g_Column = lv->iSubItem;
						g_ProcSortDirections[lv->iSubItem] = -1;
					}

					/*if (lv->iSubItem != 0) g_ProcSortDirections[0] = 1;
					if (lv->iSubItem != 1) g_ProcSortDirections[1] = 1;
					if (lv->iSubItem != 2) g_ProcSortDirections[2] = 1;*/
					
					Sort(true);
					return true;				
				}
			}
			break;
		case WM_DESTROY:
			Save_Settings();
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
	
}

void mainLoop(){
	MSG msg;		
	memset(&msg, 0, sizeof(MSG));	
	while (msg.message != WM_QUIT){
		if (GetMessage(&msg, NULL, 0U, 0U)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		//Sleep(10);
	}
}




int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* lpCmdLine,int nCmdShow){
	INITCOMMONCONTROLSEX ex;
	WNDCLASSEX wcls;	
	ATOM atm;

	g_hInstance = hInstance;
	
	/*--Initialize the needed classes --*/

	ex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	ex.dwICC = 
		ICC_TREEVIEW_CLASSES |
		ICC_BAR_CLASSES |
		ICC_PROGRESS_CLASS |
		ICC_LISTVIEW_CLASSES |
		ICC_TAB_CLASSES;
	InitCommonControlsEx(&ex);

	/*-- Initialize the window stuff --*/

	memset(&wcls, 0, sizeof(WNDCLASSEX));

	wcls.lpszClassName = CLASS_NAME;
	wcls.lpszMenuName = (LPCWSTR)IDR_MAIN;
	wcls.cbSize = sizeof(WNDCLASSEX);
	wcls.style = 0;
	wcls.lpfnWndProc = WndProc;
	wcls.hIcon = NULL;//LoadIcon(g_hInstance, (LPCSTR)MAIN_ICON);
	wcls.hIconSm = NULL;//LoadIcon(g_hInstance, (LPCSTR)SMALL_ICON);
	wcls.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcls.cbClsExtra = 0;
	wcls.cbWndExtra = 0;
	wcls.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wcls.hInstance =hInstance;
	atm = RegisterClassEx(&wcls);

	g_MainWnd = CreateWindowEx(0, CLASS_NAME, WND_TITLE, 
		WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, 
		NULL, g_hInstance, NULL);

	if (g_MainWnd){		
		Position_Windows();
		mainLoop();
	} else{
		LogError();
		MessageBox(0, L"Error creating window.", L"Error", MB_ICONEXCLAMATION);
	}

	UnregisterClass(CLASS_NAME, hInstance);
	return 0;
}
