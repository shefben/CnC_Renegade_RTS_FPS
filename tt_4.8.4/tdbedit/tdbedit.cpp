/*	Renegade Scripts.dll
	Copyright 2013 Tiberian Technologies

	This file is part of the Renegade scripts.dll
	The Renegade scripts.dll is free software; you can redistribute it and/or modify it under
	the terms of the GNU General Public License as published by the Free
	Software Foundation; either version 2, or (at your option) any later
	version. See the file COPYING for more details.
	In addition, an exemption is given to allow Run Time Dynamic Linking of this code with any closed source module that does not contain code covered by this licence.
	Only the source code to the module(s) containing the licenced code has to be released.
*/
#include "general.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <commctrl.h>
#include <commdlg.h>
#include "resource.h"
#include "tdbedit.h"
#include "search.h"
#include "RawFileClass.h"
#include "SaveLoadSystemClass.h"
#include "SList.h"
#include "PresetMgr.h"
#include "DefinitionMgrClass.h"
#include "AudibleSoundDefinitionClass.h"

#define CLASS_NAME L"TDB_WND"
#define WND_TITLE L"Advanced string editor"

HWND g_MainWnd = NULL;
HWND g_StringList = NULL;
HWND g_Tabs = NULL;
HWND g_Status_Bar = NULL;
HIMAGELIST g_hImagelist;
HINSTANCE g_hInstance;
bool g_Is_File_Open = false;
int g_Column = 0;

void Position_Windows();
void LogFormated(wchar_t *fmt, ...);
void Handle_Tab_Change(int idx, bool update_only = false);

void Set_Status_Text(wchar_t *text)
{
	SendMessage(g_Status_Bar, SB_SETTEXT, 0, (LPARAM)text);
}

void Set_Status_Text_Formated(wchar_t *format, ...)
{
	wchar_t lBuf[512];
	va_list va;
	va_start(va, format);
	_vsnwprintf(lBuf, 512, format, va);
	va_end(va);
	SendMessage(g_Status_Bar, SB_SETTEXT, 0, (LPARAM)lBuf);
}

void SelectItem(TDBObjClass*obj)
{
	unsigned int i, nStrs, ItemID = obj->Get_ID();
	LVITEM item;
	RECT r = { 0, 0, 0, 0 };
	nStrs = (int)SendMessage(g_StringList, LVM_GETITEMCOUNT, 0, 0);
	for (i = 0; i < nStrs; i++)
	{
		ZeroMemory(&item, sizeof(item));
		item.iItem = i;
		item.mask = LVIF_PARAM;
		SendMessage(g_StringList, LVM_GETITEM, 0, (LPARAM)&item);
		if (!item.lParam) continue;
		if (((TDBObjClass*)item.lParam)->Get_ID() == ItemID)
		{
			ZeroMemory(&item, sizeof(item));
			item.mask = LVIF_STATE;
			item.stateMask = LVIS_SELECTED;
			item.state = LVIS_SELECTED;
			item.iItem = i;
			SendMessage(g_StringList, LVM_SETITEM, 0, (LPARAM)&item);
			r.left = LVIR_BOUNDS;
			SendMessage(g_StringList, LVM_GETITEMRECT, i, (LPARAM)&r);
			SendMessage(g_StringList, LVM_SCROLL, 0, r.top-(r.bottom-r.top));
			return;
		}
	}
}

void Goto(int idx)
{
	unsigned int cid;
	int i, ItemID;
	if (idx < 0 || idx >= TranslateDBClass::Get_Object_Count()) return;
	TDBObjClass *obj = TranslateDBClass::Get_Object(idx);
	if (!obj) return;
	cid = obj->Get_Category_ID();
	ItemID = obj->Get_ID();
	for (i = 0; i < TranslateDBClass::Get_Category_Count(); i++)
	{
		TDBCategoryClass *cat = TranslateDBClass::Get_Category(i);
		if (cat->Get_ID() == cid)
		{
			SendMessage(g_Tabs, TCM_SETCURSEL, i, 0);
			Handle_Tab_Change(i);
			SelectItem(obj);
			return;
		}
	}
}


void MessageError()
{
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


void LogError()
{
	wchar_t lpMsgBuf[512];
	unsigned long error = GetLastError();
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		error,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_ENGLISH_US),
		(LPTSTR)lpMsgBuf,
		512, NULL );
	LogFormated(L"ERROR %d: %s", error, lpMsgBuf);
}


void LogFormated(wchar_t *fmt, ...)
{
	wchar_t lBuf[512];
	va_list va;
	va_start(va, fmt);
	_vsnwprintf(lBuf, 512, fmt, va);
	lBuf[511] = 0;
	va_end(va);
	OutputDebugString(lBuf);	
}

HWND Create_Object(wchar_t *className, int style, int exstyle, const wchar_t *text, int x, int y, int width, int height, HWND parent, const HINSTANCE hInstance, bool _3D = true)
{
	HWND temp;
	temp = CreateWindowEx(exstyle, className, text, WS_CHILD|WS_VISIBLE|style, x, y, width, height, parent, NULL, hInstance, NULL);
	SendMessage(temp, WM_SETFONT, (WPARAM)(HFONT)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(FALSE, 0));
	return temp;
}

bool OpenFile(wchar_t *buf, int bufsize, int *FileOffset, int *ExtensionOffset)
{
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
	of.Flags = OFN_FILEMUSTEXIST;
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
	lBuf[MAX_PATH-1] = 0;
	SetWindowText(g_MainWnd, lBuf);
	return true;
}
bool presetloaded = false;
bool OpenDDBFile()
{
	wchar_t lBuf[MAX_PATH] = L"";
	OPENFILENAME of;

	memset(&of, 0, sizeof(OPENFILENAME));

	of.lStructSize = sizeof(OPENFILENAME);
	of.hwndOwner = g_MainWnd;
	of.hInstance = NULL;
	of.lpstrFilter = L"Preset Database\0*.ddb\0\0";
	of.lpstrCustomFilter = NULL;
	of.nMaxCustFilter = 0;
	of.nFilterIndex = 0;
	of.lpstrFile = lBuf;
	of.nMaxFile = MAX_PATH;
	of.lpstrFileTitle = NULL;
	of.nMaxFileTitle = 0;
	of.lpstrInitialDir = NULL;
	of.lpstrTitle = NULL;
	of.Flags = OFN_FILEMUSTEXIST;
	of.nFileOffset = 0;
	of.nFileExtension = 0;
	of.lpstrDefExt = NULL;
	of.lCustData = NULL;
	of.lpfnHook = NULL;
	of.lpTemplateName = NULL;	

	if (!GetOpenFileName(&of)) return false;
	char fn[MAX_PATH] = "";
	wcstombs(fn,of.lpstrFile,MAX_PATH);
	RawFileClass file(fn);
	file.Open(1);
	ChunkLoadClass chunkLoader(&file);
	SaveLoadSystemClass::Load(chunkLoader, true);
	file.Close();
	presetloaded = true;
	return true;
}

bool SaveFile(wchar_t *buf, int bufsize, int *FileOffset, int *ExtensionOffset)
{
	wchar_t lBuf[MAX_PATH];
	wcscpy_s(lBuf,buf);
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
	of.Flags = OFN_OVERWRITEPROMPT;
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
	lBuf[MAX_PATH-1] = 0;
	SetWindowText(g_MainWnd, lBuf);
	return true;
}

bool SaveHeader(wchar_t *buf, int bufsize, int *FileOffset, int *ExtensionOffset)
{
	wchar_t lBuf[MAX_PATH] = L"string_ids.h";
	OPENFILENAME of;

	memset(&of, 0, sizeof(OPENFILENAME));

	of.lStructSize = sizeof(OPENFILENAME);
	of.hwndOwner = g_MainWnd;
	of.hInstance = NULL;
	of.lpstrFilter = L"C Header Files (*.h)\0*.h\0\0";
	of.lpstrCustomFilter = NULL;
	of.nMaxCustFilter = 0;
	of.nFilterIndex = 0;
	of.lpstrFile = lBuf;
	of.nMaxFile = MAX_PATH;
	of.lpstrFileTitle = NULL;
	of.nMaxFileTitle = 0;
	of.lpstrInitialDir = NULL;
	of.lpstrTitle = NULL;
	of.Flags = OFN_OVERWRITEPROMPT;
	of.nFileOffset = 0;
	of.nFileExtension = 0;
	of.lpstrDefExt = L"h";
	of.lCustData = NULL;
	of.lpfnHook = NULL;
	of.lpTemplateName = NULL;	
	if (!GetSaveFileName(&of)) return false;
	_snwprintf(buf, bufsize, L"%s", of.lpstrFile);
	if (FileOffset)
		*FileOffset = of.nFileOffset;
	if (ExtensionOffset)
		*ExtensionOffset = of.nFileExtension;
	return true;
}

bool OpenTable(wchar_t *buf, int bufsize, int *FileOffset, int *ExtensionOffset)
{
	wchar_t lBuf[MAX_PATH] = L"";
	OPENFILENAME of;

	memset(&of, 0, sizeof(OPENFILENAME));

	of.lStructSize = sizeof(OPENFILENAME);
	of.hwndOwner = g_MainWnd;
	of.hInstance = NULL;
	of.lpstrFilter = L"Text Files (*.txt)\0*.txt\0\0";
	of.lpstrCustomFilter = NULL;
	of.nMaxCustFilter = 0;
	of.nFilterIndex = 0;
	of.lpstrFile = lBuf;
	of.nMaxFile = MAX_PATH;
	of.lpstrFileTitle = NULL;
	of.nMaxFileTitle = 0;
	of.lpstrInitialDir = NULL;
	of.lpstrTitle = NULL;
	of.Flags = OFN_FILEMUSTEXIST;
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
	return true;
}

bool SaveTable(wchar_t *buf, int bufsize, int *FileOffset, int *ExtensionOffset)
{
	wchar_t lBuf[MAX_PATH] = L"string_table.txt";
	OPENFILENAME of;

	memset(&of, 0, sizeof(OPENFILENAME));

	of.lStructSize = sizeof(OPENFILENAME);
	of.hwndOwner = g_MainWnd;
	of.hInstance = NULL;
	of.lpstrFilter = L"Text Files (*.txt)\0*.txt\0\0";
	of.lpstrCustomFilter = NULL;
	of.nMaxCustFilter = 0;
	of.nFilterIndex = 0;
	of.lpstrFile = lBuf;
	of.nMaxFile = MAX_PATH;
	of.lpstrFileTitle = NULL;
	of.nMaxFileTitle = 0;
	of.lpstrInitialDir = NULL;
	of.lpstrTitle = NULL;
	of.Flags = OFN_OVERWRITEPROMPT;
	of.nFileOffset = 0;
	of.nFileExtension = 0;
	of.lpstrDefExt = L"txt";
	of.lCustData = NULL;
	of.lpfnHook = NULL;
	of.lpTemplateName = NULL;	
	if (!GetSaveFileName(&of)) return false;
	_snwprintf(buf, bufsize, L"%s", of.lpstrFile);
	if (FileOffset)
		*FileOffset = of.nFileOffset;
	if (ExtensionOffset)
		*ExtensionOffset = of.nFileExtension;
	return true;
}

void Do_Close()
{	
	SetWindowText(g_MainWnd, WND_TITLE);
	TranslateDBClass::Shutdown();
	TranslateDBClass::Add_Category("Default");
	SendMessage(g_StringList, LVM_DELETEALLITEMS, 0, 0);
	SendMessage(g_Tabs, TCM_DELETEALLITEMS, 0, 0);
	TCITEM item;
	const wchar_t *temp;
	int j;
	for (int i = 0; i < TranslateDBClass::Get_Category_Count(); i++)
	{
		j = (int)SendMessage(g_Tabs, TCM_GETITEMCOUNT, 0, 0);
		item.mask = TCIF_PARAM | TCIF_TEXT;
		item.lParam = (LPARAM)TranslateDBClass::Get_Category(i)->Get_ID();
		temp = CharToWideChar(TranslateDBClass::Get_Category(i)->Get_Name());
		item.pszText = (LPWSTR)temp;
		SendMessage(g_Tabs, TCM_INSERTITEM, (WPARAM)j, (LPARAM)&item);
		delete[] temp;
	}
	EnableMenuItem(GetMenu(g_MainWnd),ID_SAVE,MF_GRAYED);
	EnableMenuItem(GetMenu(g_MainWnd),ID_SAVEAS,MF_GRAYED);
	EnableMenuItem(GetMenu(g_MainWnd),ID_EXPORT_HEADER,MF_GRAYED);
	EnableMenuItem(GetMenu(g_MainWnd),ID_EXPORT_TABLE,MF_GRAYED);
	g_Is_File_Open = false;
	Position_Windows();
}

const char *filename = 0;

void Do_Open()
{
	Do_Close();
	TranslateDBClass::Shutdown();
	TCITEM item;
	const wchar_t *temp;
	int j;
	g_Is_File_Open = false;
	wchar_t lName[MAX_PATH];
	if (!OpenFile(lName, MAX_PATH, NULL, NULL)) return;	
	if (filename)
	{
		delete[] filename;
	}
	filename = WideCharToChar(lName);
	RawFileClass r(filename);
	r.Open(1);
	if (!r.Is_Open()) return;
	EnableMenuItem(GetMenu(g_MainWnd),ID_SAVE,MF_ENABLED);
	EnableMenuItem(GetMenu(g_MainWnd),ID_SAVEAS,MF_ENABLED);
	EnableMenuItem(GetMenu(g_MainWnd),ID_EXPORT_HEADER,MF_ENABLED);
	EnableMenuItem(GetMenu(g_MainWnd),ID_EXPORT_TABLE,MF_ENABLED);
	g_Is_File_Open = true;

	ChunkLoadClass cload (&r);
	SaveLoadSystemClass::Load(cload,true);
	SendMessage(g_Tabs, TCM_DELETEALLITEMS, 0, 0);
	for (int i = 0; i < TranslateDBClass::Get_Category_Count(); i++)
	{
		j = (int)SendMessage(g_Tabs, TCM_GETITEMCOUNT, 0, 0);
		item.mask = TCIF_PARAM | TCIF_TEXT;
		item.lParam = (LPARAM)TranslateDBClass::Get_Category(i)->Get_ID();
		temp = CharToWideChar(TranslateDBClass::Get_Category(i)->Get_Name());
		item.pszText = (LPWSTR)temp;
		SendMessage(g_Tabs, TCM_INSERTITEM, (WPARAM)j, (LPARAM)&item);
		delete[] temp;
	}
	r.Close();
	Position_Windows();
}

void Do_Import_Table()
{
	Do_Close();
	TranslateDBClass::Shutdown();
	TCITEM item;
	const wchar_t *temp;
	int j;

	// We don't actually have a string database file open, so reset file status variables
	// and set a default name for the file
	g_Is_File_Open = false;
	if (filename)
	{
		delete[] filename;
	}
	filename = newstr("strings.tdb");

	// Get file to import
	wchar_t lName[MAX_PATH];
	if (!OpenTable(lName, MAX_PATH, NULL, NULL)) return;

	// Attempt to import file...
	const char *c = WideCharToChar(lName);
	TranslateDBClass::Import_Strings(c);

	// Disable SAVE because we don't have a file path yet and it would crash
	EnableMenuItem(GetMenu(g_MainWnd),ID_SAVE,MF_GRAYED);
	EnableMenuItem(GetMenu(g_MainWnd),ID_SAVEAS,MF_ENABLED);
	EnableMenuItem(GetMenu(g_MainWnd),ID_EXPORT_HEADER,MF_ENABLED);
	EnableMenuItem(GetMenu(g_MainWnd),ID_EXPORT_TABLE,MF_ENABLED);

	SendMessage(g_Tabs, TCM_DELETEALLITEMS, 0, 0);
	for (int i = 0; i < TranslateDBClass::Get_Category_Count(); i++)
	{
		j = (int)SendMessage(g_Tabs, TCM_GETITEMCOUNT, 0, 0);
		item.mask = TCIF_PARAM | TCIF_TEXT;
		item.lParam = (LPARAM)TranslateDBClass::Get_Category(i)->Get_ID();
		temp = CharToWideChar(TranslateDBClass::Get_Category(i)->Get_Name());
		item.pszText = (LPWSTR)temp;
		SendMessage(g_Tabs, TCM_INSERTITEM, (WPARAM)j, (LPARAM)&item);
		delete[] temp;
	}
	Position_Windows();
}


void Handle_Tab_Change(int idx, bool update_only)
{
	TCITEM item;
	LVITEM lv;
	wchar_t lBuf[256];
	item.mask = TCIF_PARAM;
	if (!SendMessage(g_Tabs, TCM_GETITEM, (WPARAM)idx, (LPARAM)&item)) return;
	if (!update_only)
		SendMessage(g_StringList, LVM_DELETEALLITEMS, 0, 0);
	uint32 cat = (uint32)item.lParam;
	memset(&lv, 0, sizeof(lv));
	lv.iItem = 0;
	int _idx = 0;
	TDBObjClass *o;
	for (o = TranslateDBClass::Get_First_Object(cat);o;o = TranslateDBClass::Get_Next_Object(cat,o))
	{
		lv.iSubItem = 0;
		lv.iItem = _idx;
		lv.mask = LVIF_TEXT | LVIF_PARAM;
		lv.lParam = (LPARAM)o;
		lv.pszText = lBuf;
		_snwprintf(lBuf, 256, L"%S", (const char *)o->Get_ID_Desc());
		if (update_only)
			SendMessage(g_StringList, LVM_SETITEM, 0, (LPARAM)&lv);
		else
			SendMessage(g_StringList, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lv);
		lv.iSubItem = 1;
		lv.iItem = _idx;
		lv.mask = LVIF_TEXT;
		lv.pszText = lBuf;
		_snwprintf(lBuf, 256, L"%u", o->Get_ID());
		SendMessage(g_StringList, LVM_SETITEM, 0, (LPARAM)&lv);

		lv.iSubItem = 2;
		lv.iItem = _idx;
		lv.mask = LVIF_TEXT; 
		lv.pszText = lBuf;
		unsigned int id = o->Get_Sound_ID();
		if (id != 0xFFFFFFFF)
		{
			if (presetloaded)
			{
				PresetClass *preset = PresetMgrClass::Find_Preset(id);
				if (preset && preset->Get_Definition() && preset->Get_Definition()->Get_Name())
				{
					_snwprintf(lBuf, 256, L"%S", preset->Get_Definition()->Get_Name());
				}
				else
				{
					_snwprintf(lBuf, 256, L"%s", L"");
				}
			}
			else
			{
				_snwprintf(lBuf, 256, L"%u", id);
			}
		}
		else
		{
			_snwprintf(lBuf, 256, L"%s", L"");
		}
		SendMessage(g_StringList, LVM_SETITEM, 0, (LPARAM)&lv);

		lv.iSubItem = 3;
		lv.iItem = _idx;
		lv.mask = LVIF_TEXT;
		lv.pszText = lBuf;
		_snwprintf(lBuf, 256, L"%S", (const char *)o->Get_English_String());
		SendMessage(g_StringList, LVM_SETITEM, 0, (LPARAM)&lv);
		idx++;
	}
}

void Position_Windows()
{
	RECT mrect, srect, trect;
	GetClientRect(g_MainWnd, &mrect);
	GetClientRect(g_Tabs, &trect);
	GetClientRect(g_Status_Bar, &srect);
	TabCtrl_AdjustRect(g_Tabs, FALSE, &trect);
	SetWindowPos(g_Tabs, 0, mrect.left, mrect.top, mrect.right, trect.top+10, SWP_NOZORDER);	
	SetWindowPos(g_StringList, HWND_TOP, mrect.left, trect.top+10, mrect.right, mrect.bottom-srect.bottom-trect.top-10, 0);	
}


bool Create_Windows(HWND hWnd)
{
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

	col.cx = 200;
	col.pszText = L"Sound ID";
	SendMessage(g_StringList, LVM_INSERTCOLUMN, (WPARAM)2, (LPARAM)&col);

	col.cx = 500;
	col.pszText = L"English text";
	SendMessage(g_StringList, LVM_INSERTCOLUMN, (WPARAM)3, (LPARAM)&col);
	TCITEM item;
	const wchar_t *temp;
	int j;
	for (int i = 0; i < TranslateDBClass::Get_Category_Count(); i++)
	{
		j = (int)SendMessage(g_Tabs, TCM_GETITEMCOUNT, 0, 0);
		item.mask = TCIF_PARAM | TCIF_TEXT;
		item.lParam = (LPARAM)TranslateDBClass::Get_Category(i)->Get_ID();
		temp = CharToWideChar(TranslateDBClass::Get_Category(i)->Get_Name());
		item.pszText = (LPWSTR)temp;
		SendMessage(g_Tabs, TCM_INSERTITEM, (WPARAM)j, (LPARAM)&item);
		delete[] temp;
	}
	return (g_Status_Bar && g_Tabs && g_StringList);
}

const char *newcat = 0;
INT_PTR CALLBACK Dlg_Cat(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	wchar_t lBuf[256];
	switch (uMsg)
	{
		case WM_INITDIALOG:
			if (newcat)
			{
				delete[] newcat;
				newcat = 0;
			}
			SetFocus(GetDlgItem(hDlg, IDC_CATEGORY));
			return TRUE;
			break;
		case WM_CLOSE:
			return EndDialog(hDlg, 0);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDC_CANCEL:
				return EndDialog(hDlg, 0);
			case IDC_OK:
				GetDlgItemText(hDlg, IDC_CATEGORY, lBuf, 256);
				if (lBuf[0])
				{
					newcat = WideCharToChar(lBuf);
					return EndDialog(hDlg, 1);
				}
				return EndDialog(hDlg, 0);
			default:
				break;
			}
			break;
		default:
			break;			
	}	
	return FALSE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LPNMHDR not;
	LPNMITEMACTIVATE act;
	LVITEM item;
	switch (uMsg)
	{
		case WM_CREATE:
			g_MainWnd = hWnd;
			if (!Create_Windows(hWnd))
			{
				MessageBox(hWnd, L"Error creating controls.", L"Error.", MB_ICONEXCLAMATION);
				DestroyWindow(hWnd);
				break;
			}
			break;
		case WM_SIZE:			
			SendMessage(g_Status_Bar, uMsg, wParam, lParam);
			Position_Windows();
			break;
		case WM_COMMAND:		
			switch (LOWORD(wParam))
			{
			case ID_NEW:
				Do_Close();
				break;
			case ID_OPEN:
				Do_Open();
				break;
			case ID_OPEN_DDB:
				OpenDDBFile();
				Handle_Tab_Change((int)SendMessage(g_Tabs, TCM_GETCURSEL, 0, 0));
				break;
			case ID_SAVE:
				if (filename)
				{
					HANDLE h = CreateFileA(filename,0x40000000,0,0,2,0,0);
					if (h != INVALID_HANDLE_VALUE)
					{
						RawFileClass r;
						r.Attach(h,1);
						ChunkSaveClass csave(&r);
						SaveLoadSystemClass::Save(csave,_TheTranslateDB);
					}
				}
				else
				{
					wchar_t lName[MAX_PATH];
					SaveFile(lName,MAX_PATH,0,0);
					const char *c = WideCharToChar(lName);
					HANDLE h = CreateFileA(c,0x40000000,0,0,2,0,0);
					if (h != INVALID_HANDLE_VALUE)
					{
						RawFileClass r;
						r.Attach(h,1);
						ChunkSaveClass csave(&r);
						SaveLoadSystemClass::Save(csave,_TheTranslateDB);
					}
					delete[] c;
				}
				break;
			case ID_SAVEAS:
				{
					wchar_t lName[MAX_PATH];
					if (filename)
					{
						const wchar_t *wcs = CharToWideChar(filename);
						wcscpy(lName,wcs);
						delete[] wcs;
					}
					SaveFile(lName,MAX_PATH,0,0);
					const char *c = WideCharToChar(lName);
					HANDLE h = CreateFileA(c,0x40000000,0,0,2,0,0);
					if (h != INVALID_HANDLE_VALUE)
					{
						RawFileClass r;
						r.Attach(h,1);
						ChunkSaveClass csave(&r);
						SaveLoadSystemClass::Save(csave,_TheTranslateDB);

						/* Enable Save item (if Import Table was used this will still be
						disabled since no filename was available */
						EnableMenuItem(GetMenu(g_MainWnd),ID_SAVE,MF_ENABLED);
					}
					delete[] c;
				}
				break;
			case ID_CLOSE_FILE:
				Do_Close();
				break;
			case ID_REMOVESTRING:
				{
					int sel = (int)SendMessage(g_StringList, LVM_GETSELECTIONMARK, 0, 0);
					if (sel != -1)
					{
						item.iItem = sel;
						item.mask = LVIF_PARAM;
						SendMessage(g_StringList, LVM_GETITEM, 0, (LPARAM)&item);
						TDBObjClass *obj = (TDBObjClass*)item.lParam;
						TranslateDBClass::Remove_Object(obj->Get_ID() - 1000);
						Handle_Tab_Change((int)SendMessage(g_Tabs, TCM_GETCURSEL, 0, 0),false);
					}
				}
				break;
			case ID_ADDSTRING:
				{
					TCITEM item2;
					item2.mask = TCIF_PARAM;
					if (!SendMessage(g_Tabs, TCM_GETITEM, (WPARAM)(int)SendMessage(g_Tabs, TCM_GETCURSEL, 0, 0), (LPARAM)&item2)) break;
					TDBObjClass *obj = new TDBObjClass;
					obj->Set_Category_ID((uint32)item2.lParam);
					obj->Set_String(0,L"");
					TranslateDBClass::Add_Object(obj);
					Handle_Tab_Change((int)SendMessage(g_Tabs, TCM_GETCURSEL, 0, 0),false);
				}
				break;
			case ID_ADDCATEGORY:
				{
					DialogBox(g_hInstance,(LPWSTR)IDD_New_Category, g_MainWnd, Dlg_Cat);
					if (newcat)
					{
						TranslateDBClass::Add_Category(newcat);
						TCITEM item2;
						const wchar_t *temp;
						int j;
						SendMessage(g_Tabs, TCM_DELETEALLITEMS, 0, 0);
						for (int i = 0; i < TranslateDBClass::Get_Category_Count(); i++)
						{
							j = (int)SendMessage(g_Tabs, TCM_GETITEMCOUNT, 0, 0);
							item2.mask = TCIF_PARAM | TCIF_TEXT;
							item2.lParam = (LPARAM)TranslateDBClass::Get_Category(i)->Get_ID();
							temp = CharToWideChar(TranslateDBClass::Get_Category(i)->Get_Name());
							item2.pszText = (LPWSTR)temp;
							SendMessage(g_Tabs, TCM_INSERTITEM, (WPARAM)j, (LPARAM)&item2);
							delete[] temp;
						}
					}
				}
				break;
			case ID_REMOVECATEGORY:
				{
					TCITEM item2;
					item2.mask = TCIF_PARAM;
					if (!SendMessage(g_Tabs, TCM_GETITEM, (WPARAM)(int)SendMessage(g_Tabs, TCM_GETCURSEL, 0, 0), (LPARAM)&item2)) break;
					if (TranslateDBClass::Get_Category_Count() >= 2)
					{
						for (int i = 0;i < TranslateDBClass::Get_Category_Count();i++)
						{
							if (TranslateDBClass::Get_Category(i)->Get_ID() == (uint32)item2.lParam)
							{
								TranslateDBClass::Remove_Category(i);
								const wchar_t *temp;
								int j;
								SendMessage(g_Tabs, TCM_DELETEALLITEMS, 0, 0);
								for (int i2 = 0; i2 < TranslateDBClass::Get_Category_Count(); i2++)
								{
									j = (int)SendMessage(g_Tabs, TCM_GETITEMCOUNT, 0, 0);
									item2.mask = TCIF_PARAM | TCIF_TEXT;
									item2.lParam = (LPARAM)TranslateDBClass::Get_Category(i2)->Get_ID();
									temp = CharToWideChar(TranslateDBClass::Get_Category(i2)->Get_Name());
									item2.pszText = (LPWSTR)temp;
									SendMessage(g_Tabs, TCM_INSERTITEM, (WPARAM)j, (LPARAM)&item2);
									delete[] temp;
								}
								Handle_Tab_Change((int)SendMessage(g_Tabs, TCM_GETCURSEL, 0, 0),false);
								break;
							}
						}
					}
				}
				break;
			case ID_EXPORT_HEADER:
				{
					wchar_t lName[MAX_PATH];
					SaveHeader(lName,MAX_PATH,0,0);
					const char *c = WideCharToChar(lName);
					TranslateDBClass::Export_C_Header(c);
				}
				break;
			case ID_IMPORT_TABLE:
				Do_Import_Table();
				break;
			case ID_EXPORT_TABLE:
				{
					wchar_t lName[MAX_PATH];
					SaveTable(lName,MAX_PATH,0,0);
					const char *c = WideCharToChar(lName);
					TranslateDBClass::Export_Table(c);
				}
				break;
			case ID_EXIT:
				DestroyWindow(g_MainWnd);
				break;
			case ID_GOTO:		
				DoGoto();
				break;
			case ID_FIND_BY_ID:
				DoFindByID();
				break;
			case ID_FIND_BY_STRING:
				DoFindByString();
				break;
			case ID_FIND_BY_ENG_STRING:
				DoFindByEnglishString();
				break;
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
			if (act->hdr.hwndFrom == g_StringList && act->hdr.code == NM_DBLCLK && act->iItem != -1)
			{
				item.iItem = act->iItem;
				item.mask = LVIF_PARAM;
				SendMessage(g_StringList, LVM_GETITEM, 0, (LPARAM)&item);
				if (item.lParam)
				{
					if (TDBEdit((TDBObjClass*)item.lParam))
					{
						Handle_Tab_Change((int)SendMessage(g_Tabs, TCM_GETCURSEL, 0, 0),false);
					}
				}
				return TRUE;
			}
			if (act->hdr.hwndFrom == g_StringList && act->hdr.code == NM_RCLICK && act->iItem != -1)
			{
				item.iItem = act->iItem;
				item.mask = LVIF_PARAM;
				item.iSubItem = 2;
				if (!SendMessage(g_StringList, LVM_GETITEM, 0, (LPARAM)&item)) return TRUE;				
				return TRUE;
			}
			
			if (not->hwndFrom == g_Tabs)
			{
				if (not->code == TCN_SELCHANGE)
					Handle_Tab_Change((int)SendMessage(g_Tabs, TCM_GETCURSEL, 0, 0));
			}
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
	
}

void mainLoop()
{
	MSG msg;		
	memset(&msg, 0, sizeof(MSG));	
	while (msg.message != WM_QUIT)
	{
		if (GetMessage(&msg, NULL, 0U, 0U))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}




#pragma warning(suppress: 28251) //warning C28251: Inconsistent annotation for function
int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* lpCmdLine,int nCmdShow)
{
	INITCOMMONCONTROLSEX ex;
	WNDCLASSEX wcls;	
	ATOM atm;
	TranslateDBClass::Initialize();
	TranslateDBClass::Add_Category("Default");
	g_hInstance = hInstance;
	
	ex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	ex.dwICC = 
		ICC_TREEVIEW_CLASSES |
		ICC_BAR_CLASSES |
		ICC_PROGRESS_CLASS |
		ICC_LISTVIEW_CLASSES |
		ICC_TAB_CLASSES;
	InitCommonControlsEx(&ex);

	memset(&wcls, 0, sizeof(WNDCLASSEX));

	wcls.lpszClassName = CLASS_NAME;
	wcls.lpszMenuName = (LPCWSTR)IDR_MAIN;
	wcls.cbSize = sizeof(WNDCLASSEX);
	wcls.style = 0;
	wcls.lpfnWndProc = WndProc;
	wcls.hIcon = LoadIcon(g_hInstance, (LPCWSTR)IDI_MAIN);
	wcls.hIconSm = LoadIcon(g_hInstance, (LPCWSTR)IDI_MAIN);
	wcls.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcls.cbClsExtra = 0;
	wcls.cbWndExtra = 0;
	wcls.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wcls.hInstance =hInstance;
	atm = RegisterClassEx(&wcls);

	g_MainWnd = CreateWindowEx(0, CLASS_NAME, WND_TITLE, 
		WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 1024, 768, NULL, 
		NULL, g_hInstance, NULL);

	if (g_MainWnd)
	{		
		Position_Windows();
		mainLoop();
	} else{
		LogError();
		MessageBox(0, L"Error creating window.", L"Error", MB_ICONEXCLAMATION);
	}
	if (filename)
	{
		delete[] filename;
	}
	if (newcat)
	{
		delete[] newcat;
	}
	UnregisterClass(CLASS_NAME, hInstance);
	return 0;
}

bool SaveLoadSystemClass::Load(ChunkLoadClass& chunkLoader, bool auto_post_load)
{
	SaveLoadSystemClass::PointerRemapper.Reset();

	bool allSucceeded = true;

	for (; chunkLoader.Open_Chunk(); chunkLoader.Close_Chunk())
	{
		// DEPRECATED: SaveLoadStatus::Inc_Status_Count();
		SaveLoadSubSystemClass* subSystem = SaveLoadSystemClass::Find_Sub_System (chunkLoader.Cur_Chunk_ID());
		if (subSystem)
		{
		   // SaveLoadStatus::Set_Status_Text (subSystem->Name(), 1);
		   allSucceeded = allSucceeded && subSystem->Load (chunkLoader);
		}
	}

   SaveLoadSystemClass::PointerRemapper.Process();
   SaveLoadSystemClass::PointerRemapper.Reset();

   if (auto_post_load)
      SaveLoadSystemClass::Post_Load_Processing (0);

   return allSucceeded;
}

bool SaveLoadSystemClass::Save(ChunkSaveClass &chunkSaver, SaveLoadSubSystemClass &subSystem)
{
	if (subSystem.Contains_Data())
	{
		chunkSaver.Begin_Chunk(subSystem.Chunk_ID());
		bool b = subSystem.Save(chunkSaver);
		chunkSaver.End_Chunk();
		return b;
	}
	return false;
}

SList<PostLoadableClass> PostLoadList;

void SaveLoadSystemClass::Post_Load_Processing(void(*func)())
{
	for (PostLoadableClass *post = PostLoadList.Remove_Head();post;post = PostLoadList.Remove_Head())
	{
		post->On_Post_Load();
		post->Set_Post_Load_Registered(false);
	}
}

void SaveLoadSystemClass::Register_Post_Load_Callback(PostLoadableClass *post)
{
	if (!post->Is_Post_Load_Registered())
	{
		post->Set_Post_Load_Registered(true);
		PostLoadList.Add_Head(post);
	}
}
