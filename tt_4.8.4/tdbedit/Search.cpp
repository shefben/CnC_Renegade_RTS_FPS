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
#include <stdlib.h>
#include <stdio.h>
#include <commctrl.h>
#include "TranslateDBClass.h"
#include "tdbedit.h"
#include "resource.h"
#include "search.h"
#include "PresetMgr.h"
#include "AudibleSoundDefinitionClass.h"
#include "DefinitionMgrClass.h"

HWND g_List = NULL;
HWND g_Results = NULL;
bool g_ByIndex = false;

void Add_Object(TDBObjClass*obj, int idx)
{
	LVITEM lv;
	wchar_t lBuf[256];
	ZeroMemory(&lv, sizeof(lv));
	int pos = (int)SendMessage(g_List, LVM_GETITEMCOUNT, 0, 0);
	lv.mask = LVIF_PARAM | LVIF_TEXT;
	lv.iItem = pos;
	lv.lParam = (LPARAM)idx;
	_snwprintf(lBuf, 256, L"%S", (const char *)obj->Get_ID_Desc());
	lv.pszText = lBuf;
	SendMessage(g_List, LVM_INSERTITEM, 0, (LPARAM)&lv);
	_snwprintf(lBuf, 256, L"%u", obj->Get_ID());
	lv.mask = LVIF_TEXT;
	lv.iSubItem = 1;
	SendMessage(g_List, LVM_SETITEM, 0, (LPARAM)&lv);
	_snwprintf(lBuf, 256, L"%S", (const char *)obj->Get_English_String());
	lv.mask = LVIF_TEXT;
	lv.iSubItem = 2;
	SendMessage(g_List, LVM_SETITEM, 0, (LPARAM)&lv);
}

bool Make_Window()
{
	LVCOLUMN col;
	if (g_Results) return true;
	g_Results = CreateDialog(g_hInstance, (LPWSTR)IDD_Find_Results, g_MainWnd, Dlg_Results);
	if (!g_Results) return false;
	ShowWindow(g_Results, SW_SHOW);
	col.mask = LVCF_TEXT | LVCF_WIDTH;
	col.cx = 200;
	col.pszText = L"ID (string)";
	SendMessage(g_List, LVM_INSERTCOLUMN, (WPARAM)0, (LPARAM)&col);
	col.cx = 100;
	col.pszText = L"ID (numeric)";
	SendMessage(g_List, LVM_INSERTCOLUMN, (WPARAM)1, (LPARAM)&col);
	col.cx = 600;
	col.pszText = L"English text";
	SendMessage(g_List, LVM_INSERTCOLUMN, (WPARAM)2, (LPARAM)&col);
	SendMessage(g_List, LVM_SETEXTENDEDLISTVIEWSTYLE, (WPARAM)LIST_EX_STL, (LPARAM)LIST_EX_STL);
	return true;
}

void FindByEnglishString(const wchar_t *_substr)
{
	ClearResults();	
	bool found_one = false;
	char *str = new char[wcslen(_substr)+1];
	if (!str) return;
	_snprintf(str, wcslen(_substr)+1, "%S", _substr);
	int cItems = TranslateDBClass::Get_Object_Count();
	TDBObjClass *o;
	for (int i = 0; i < cItems; i++)
	{
		o = TranslateDBClass::Get_Object(i);
		if (!o) continue;
		if (!stristr((char *)o->Get_English_String().Peek_Buffer(), (char*)str)) continue;
		if (!found_one && !g_Results)
			found_one = Make_Window();
		Add_Object(o, i);
	}
	delete[] str;
}

void FindByString(const wchar_t *_substr)
{
	ClearResults();
	bool found_one = false;
	int cItems = TranslateDBClass::Get_Object_Count();
	TDBObjClass *o;
	for (int i = 0; i < cItems; i++)
	{
		o = TranslateDBClass::Get_Object(i);
		if (!o) continue;
		if (!wcsistr((wchar_t*)((WideStringClass &)o->Get_String(TranslateDBClass::Get_Current_Language())).Peek_Buffer(), (wchar_t*)_substr)) continue;
		if (!found_one && !g_Results)
			found_one = Make_Window();
		Add_Object(o, i);
	}
}

void FindByID(const wchar_t *_substr)
{
	ClearResults();	
	bool found_one = false;
	char *str = new char[wcslen(_substr)+1];
	if (!str) return;
	_snprintf(str, wcslen(_substr)+1, "%S", _substr);
	int cItems = TranslateDBClass::Get_Object_Count();
	TDBObjClass *o;
	for (int i = 0; i < cItems; i++)
	{
		o = TranslateDBClass::Get_Object(i);
		if (!o) continue;
		if (!stristr((char*)o->Get_ID_Desc().Peek_Buffer(), (char*)str)) continue;
		if (!found_one && !g_Results)
			found_one = Make_Window();
		Add_Object(o, i);
	}
	delete[] str;
}

void ClearResults()
{
	if (!g_List) return;
	SendMessage(g_List, LVM_DELETEALLITEMS, 0, 0);
}

void Close()
{
	ClearResults();
	if (g_Results)
	{
		EndDialog(g_Results, 0);
		g_Results = NULL;
	}
}

INT_PTR CALLBACK Dlg_Results(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LPNMITEMACTIVATE act;
	LVITEM item;
	ZeroMemory(&item, sizeof(item));
	switch (uMsg)
	{
		case WM_INITDIALOG:
			g_List = GetDlgItem(hDlg, IDC_Results);
			SetWindowLong(g_List, GWL_EXSTYLE, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
			SetFocus(g_List);
			return TRUE;
			break;
		case WM_CLOSE:
			g_List = g_Results = NULL;
			return EndDialog(hDlg, 0);
			break;
		case WM_KEYUP:
			LogFormated(L"-- %d- %d\n", wParam, lParam);
			break;
		case WM_NOTIFY:
			act = (LPNMITEMACTIVATE)lParam;
			if (act->hdr.hwndFrom == g_List && act->hdr.code == NM_DBLCLK)
			{
				item.iItem = act->iItem;
				item.mask = LVIF_PARAM;
				SendMessage(g_List, LVM_GETITEM, 0, (LPARAM)&item);
				Goto((int)item.lParam);
				return TRUE;
			}
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDC_CLOSE:
				ClearResults();
				g_List = g_Results = NULL;
				return EndDialog(hDlg, 0);
			case IDC_CLEAR:
				ClearResults();
				return TRUE;			
			default:
				break;
			}
			break;
		default:
			break;			
	}	
	return FALSE;
}

void Goto_By_ID(unsigned int ID)
{
	int cItems = TranslateDBClass::Get_Object_Count();
	TDBObjClass *o;
	for (int i = 0; i < cItems; i++)
	{
		o = TranslateDBClass::Get_Object(i);
		if (!o) continue;
		if (o->Get_ID() == ID)
		{
			Goto(i);
			return;
		}
	}
}

INT_PTR CALLBACK Dlg_Goto(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BOOL translated = FALSE;
	int res;
	switch (uMsg)
	{
		case WM_INITDIALOG:
			SetFocus(GetDlgItem(hDlg, IDC_GOTO_INT));
			SendMessage(GetDlgItem(hDlg, IDC_RADIOID), BM_SETCHECK, g_ByIndex ? BST_UNCHECKED : BST_CHECKED, 0);
			SendMessage(GetDlgItem(hDlg, IDC_RADIOIDX), BM_SETCHECK, g_ByIndex ? BST_CHECKED : BST_UNCHECKED, 0);
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
			case IDC_RADIOID:
			case IDC_RADIOIDX:
				g_ByIndex = SendMessage(GetDlgItem(hDlg, IDC_RADIOID), BM_GETCHECK, 0, 0) == BST_CHECKED ? false : true;
				break;
			case IDC_OK:
				res = GetDlgItemInt(hDlg, IDC_GOTO_INT, &translated, FALSE);
				LogFormated(L"res == %d\n", res);
				if (translated)
				{
					if (!g_ByIndex)
						Goto_By_ID(res);
					else 
						Goto(res);		
					return EndDialog(hDlg, 1);
				}
			default:
				break;
			}
			break;
		default:
			break;			
	}	
	return FALSE;
}

void DoGoto()
{
	DialogBox(g_hInstance, (LPWSTR)IDD_Goto, g_MainWnd, Dlg_Goto);
}

INT_PTR CALLBACK Dlg_English_Str(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	wchar_t lBuf[256];
	switch (uMsg)
	{
		case WM_INITDIALOG:
			SetFocus(GetDlgItem(hDlg, IDC_FIND_STRING));
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
				GetDlgItemText(hDlg, IDC_FIND_STRING, lBuf, 256);
				LogFormated(L"%s\n", lBuf);
				FindByEnglishString(lBuf);
				return EndDialog(hDlg, 1);
			default:
				break;
			}
			break;
		default:
			break;			
	}	
	return FALSE;
}

void DoFindByEnglishString()
{
	DialogBox(g_hInstance, (LPWSTR)IDD_Find_Eng_String, g_MainWnd, Dlg_English_Str);	
}

INT_PTR CALLBACK Dlg_Str(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	wchar_t lBuf[256];
	switch (uMsg)
	{
		case WM_INITDIALOG:
			SetFocus(GetDlgItem(hDlg, IDC_FIND_STRING));
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
				GetDlgItemText(hDlg, IDC_FIND_STRING, lBuf, 256);
				FindByString(lBuf);			
				return EndDialog(hDlg, 1);
			default:
				break;
			}
			break;
		default:
			break;			
	}	
	return FALSE;
}

void DoFindByString()
{
	DialogBox(g_hInstance, (LPWSTR)IDD_Find_String, g_MainWnd, Dlg_Str);
}

INT_PTR CALLBACK Dlg_ID(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	wchar_t lBuf[256];
	switch (uMsg)
	{
		case WM_INITDIALOG:
			SetFocus(GetDlgItem(hDlg, IDC_FIND_STRING));
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
				GetDlgItemText(hDlg, IDC_FIND_STRING, lBuf, 256);
				FindByID(lBuf);			
				return EndDialog(hDlg, 1);
			default:
				break;
			}
			break;
		default:
			break;			
	}	
	return FALSE;
}

void DoFindByID()
{
	DialogBox(g_hInstance, (LPWSTR)IDD_Find_ID, g_MainWnd, Dlg_ID);	
}

extern bool presetloaded;
void StoreInfo(HWND hDlg, TDBObjClass *obj)
{
	char temp[1000];
	wchar_t temp2[1000];
	BOOL b;
	GetDlgItemTextA(hDlg,IDC_ID,temp,1000);
	obj->Set_ID_Desc(temp);
	unsigned int id = GetDlgItemInt(hDlg,IDC_ID_N,&b,false);
	if (b)
	{
		if (obj->Get_ID() != id)
		{
			obj->Set_ID(id);
		}
	}
	GetDlgItemTextA(hDlg,IDC_ENG_STRING,temp,1000);
	StringClass es = temp;
	StringClass newes;
	for (int i = 0;i < es.Get_Length();i++)
	{
		if ((es[i] == '\\') && (es[i+1] == 't'))
		{
			newes += 9;
			i++;
		}
		else if ((es[i] == '\\') && (es[i+1] == 'n'))
		{
			newes += 0xA;
			i++;
		}
		else
		{
			newes += es[i];
		}
	}
	obj->Set_English_String(newes);
	GetDlgItemText(hDlg,IDC_TEXT,temp2,1000);
	WideStringClass str = temp2;
	WideStringClass newstr;
	for (int i = 0;i < str.Get_Length();i++)
	{
		if ((str[i] == L'\\') && (str[i+1] == L't'))
		{
			newstr += L"\t";
			i++;
		}
		else if ((str[i] == L'\\') && (str[i+1] == L'n'))
		{
			newstr += L"\n";
			i++;
		}
		else
		{
			newstr += str[i];
		}
	}
	obj->Set_String(TranslateDBClass::Get_Current_Language(),newstr);
	if (presetloaded)
	{
		char lBuf[256];
		GetDlgItemTextA(hDlg, IDC_SOUND_ID, lBuf, 256);
		DefinitionClass *definition = DefinitionMgrClass::Find_Named_Definition(lBuf,false);
		if (definition)
		{
			obj->Set_Sound_ID(definition->Get_ID());
		}
		else
		{
			obj->Set_Sound_ID(0);
		}
	}
	else
	{
		int snd = GetDlgItemInt(hDlg,IDC_SOUND_ID,&b,true);
		if (b)
		{
			obj->Set_Sound_ID(snd);
		}
		else
		{
			obj->Set_Sound_ID(0);
		}
	}
}
void DoInfo(HWND hDlg, TDBObjClass *obj)
{
	unsigned long ID = obj->Get_Sound_ID();
	SetDlgItemTextA(hDlg, IDC_ID, obj->Get_ID_Desc());	
	SetDlgItemInt(hDlg, IDC_ID_N, obj->Get_ID(), TRUE);
	StringClass es = obj->Get_English_String();
	StringClass newes;
	for (int i = 0;i < es.Get_Length();i++)
	{
		if (es[i] == 9)
		{
			newes += "\\t";
		}
		else if (es[i] == 0xA)
		{
			newes += "\\n";
		}
		else
		{
			newes += es[i];
		}
	}
	SetDlgItemTextA(hDlg, IDC_ENG_STRING, newes);
	WideStringClass str = obj->Get_String(TranslateDBClass::Get_Current_Language());
	if (str)
	{
		WideStringClass newstr;
		for (int i = 0;i < str.Get_Length();i++)
		{
			if (str[i] == 9)
			{
				newstr += L"\\t";
			}
			else if (str[i] == 0xA)
			{
				newstr += L"\\n";
			}
			else
			{
				newstr += str[i];
			}
		}
		SetDlgItemText(hDlg, IDC_TEXT, newstr);
	}
	if (presetloaded)
	{
		PresetClass *preset = PresetMgrClass::Find_Preset(ID);
		if (preset && preset->Get_Definition() && preset->Get_Definition()->Get_Name())
		{
			SetDlgItemTextA(hDlg, IDC_SOUND_ID, preset->Get_Definition()->Get_Name());
			EnableWindow(GetDlgItem(hDlg,IDC_SOUND_ID),false);
		}
	}
	else
	{
		EnableWindow(GetDlgItem(hDlg,IDSOUND),false);
		SetDlgItemInt(hDlg, IDC_SOUND_ID, ID, true);
	}
}

HTREEITEM TreeViewInsertItem(HWND tree,const char *text,HTREEITEM parent,HTREEITEM insertafter)
{
	TVINSERTSTRUCTA str;
	str.hParent = parent;
	str.hInsertAfter = insertafter;
	str.item.mask = TVIF_TEXT;
	str.item.pszText = (LPSTR)text;
	return (HTREEITEM)SendMessage(tree,TVM_INSERTITEMA,0,(LPARAM)&str);
}
struct TreeItemData
{
	int type;
	void *ptr;
};
void TreeViewSetData(HWND tree,HTREEITEM item,void *data,int type)
{
	TVITEM tv;
	tv.mask = TVIF_PARAM;
	tv.hItem = item;
	SendMessage(tree,TVM_GETITEM,0,(LPARAM)&tv);
	if (!tv.lParam)
	{
		TreeItemData *data2 = new TreeItemData;
		data2->type = type;
		data2->ptr = data;
		tv.mask = TVIF_PARAM;
		tv.hItem = item;
		tv.lParam = (LPARAM)data2;
		SendMessage(tree,TVM_SETITEM,0,(LPARAM)&tv);
	}
}

void *TreeViewGetData(HWND tree,HTREEITEM item,int type)
{
	if (item)
	{
		TVITEM tv;
		tv.mask = TVIF_PARAM;
		tv.hItem = item;
		SendMessage(tree,TVM_GETITEM,0,(LPARAM)&tv);
		TreeItemData *data = (TreeItemData *)tv.lParam;
		if (data != 0)
		{
			if (data->type == type)
			{
				return data->ptr;
			}
		}
	}
	return 0;
}

void Sort_Tree(HWND tree,HTREEITEM item)
{
	for (HTREEITEM i = (HTREEITEM)SendMessage(tree,TVM_GETNEXTITEM,TVGN_CHILD,(LPARAM)item);i;i = (HTREEITEM)SendMessage(tree,TVM_GETNEXTITEM,TVGN_NEXT,(LPARAM)i))
	{
		SendMessage(tree,TVM_SORTCHILDREN,0,(LPARAM)i);
		TVITEM tv;
		tv.mask = TVIF_CHILDREN;
		tv.hItem = i;
		SendMessage(tree,TVM_GETITEM,0,(LPARAM)&tv);
		if (tv.cChildren)
		{
			Sort_Tree(tree,i);
		}
	}
}
HTREEITEM Selection;
PresetClass *selpreset;
void Populate_Children(HWND tree,HTREEITEM item,uint32 id)
{
	PresetClass *preset2 = PresetMgrClass::Find_Preset(id);
	for (int i = 0;i < preset2->Get_Child_Count();i++)
	{
		PresetClass *preset = preset2->Get_Child(i);
		const char *str = 0;
		if (preset->Get_Definition())
		{
			str = preset->Get_Definition()->Get_Name();
		}
		HTREEITEM newitem = TreeViewInsertItem(tree,str,item,TVI_LAST);
		if (newitem)
		{
			if (selpreset == preset)
			{
				Selection = newitem;
			}
			TreeViewSetData(tree,newitem,preset,1);
			if (preset->Get_Definition())
			{
				preset->Set_Definition_ID(preset->Get_Definition()->Get_ID());
			}
			Populate_Children(tree,newitem,preset->Get_Definition_ID());
		}
	}
}

INT_PTR CALLBACK Dlg_Sound(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
			{
				Selection = 0;
				selpreset = (PresetClass *)lParam;
				HWND tree = GetDlgItem(hDlg,IDC_TREE);
				SendMessage(tree,WM_SETREDRAW,0,0);
				SendMessage(tree,TVM_SELECTITEM,TVGN_CARET,0);
				SendMessage(tree,TVM_DELETEITEM,0,(LPARAM)TVI_ROOT);
				for (PresetClass *i = PresetMgrClass::Find_First_Preset(20480,1,true);i;i = PresetMgrClass::Find_Next_Preset(i,20480,1,true))
				{
					if (!i->Get_Parent() || i->Get_Definition()->Get_Class_ID() == 57344)
					{
						const char *str = 0;
						if (i->Get_Definition())
						{
							str = i->Get_Definition()->Get_Name();
						}
						HTREEITEM newitem = TreeViewInsertItem(tree,str,TVI_ROOT,TVI_LAST);
						if (newitem)
						{
							if (selpreset == i)
							{
								Selection = newitem;
							}
							TreeViewSetData(tree,newitem,i,1);
							if (i->Get_Definition())
							{
								i->Set_Definition_ID(i->Get_Definition()->Get_ID());
							}
							Populate_Children(tree,newitem,i->Get_Definition_ID());
						}
					}
				}
				HTREEITEM none = TreeViewInsertItem(tree,"<None>",TVI_ROOT,TVI_FIRST);
				SendMessage(tree,TVM_SORTCHILDREN,0,(LPARAM)TVI_ROOT);
				Sort_Tree(tree,TVI_ROOT);
				SendMessage(tree,WM_SETREDRAW,1,0);
				if (Selection)
				{
					SendMessage(tree,TVM_ENSUREVISIBLE,0,(LPARAM)Selection);
					SendMessage(tree,TVM_SELECTITEM,TVGN_CARET,(LPARAM)Selection);
				}
				else
				{
					SendMessage(tree,TVM_SELECTITEM,TVGN_CARET,(LPARAM)none);
				}
			}
			return TRUE;
			break;
		case WM_CLOSE:
			return EndDialog(hDlg, -1);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDCANCEL:
				return EndDialog(hDlg, -1);
			case IDOK:
				{
					HWND tree = GetDlgItem(hDlg,IDC_TREE);
					HTREEITEM i = (HTREEITEM)SendMessage(tree,TVM_GETNEXTITEM,TVGN_CARET,0);
					PresetClass *preset = (PresetClass *)TreeViewGetData(tree,i,1);
					if (preset)
					{
						return EndDialog(hDlg, preset->Get_Definition_ID());
					}
					else
					{
						return EndDialog(hDlg, 0);
					}
				}
			default:
				break;
			}
			break;
		default:
			break;
	}
	return FALSE;
}

TDBObjClass *obj;
INT_PTR CALLBACK Dlg_Info(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
			DoInfo(hDlg, (TDBObjClass*)lParam);
			obj = (TDBObjClass*)lParam;
			return TRUE;
			break;
		case WM_CLOSE:
			SetFocus(g_List);
			return EndDialog(hDlg, 0);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDCANCEL:
				SetFocus(g_List);
				return EndDialog(hDlg, 0);
			case IDOK:
				StoreInfo(hDlg, obj);
				SetFocus(g_List);
				return EndDialog(hDlg, 1);
			case IDSOUND:
				{
					char lBuf[256];
					GetDlgItemTextA(hDlg, IDC_SOUND_ID, lBuf, 256);
					DefinitionClass *definition = DefinitionMgrClass::Find_Named_Definition(lBuf,false);
					PresetClass *preset = 0;
					if (definition)
					{
						preset = PresetMgrClass::Find_Preset(definition->Get_ID());
					}
					int ID = DialogBoxParam(g_hInstance, (LPWSTR)IDD_SOUNDLIST, hDlg, Dlg_Sound,(LPARAM)preset);
					if (ID != -1)
					{
						PresetClass *presetx = PresetMgrClass::Find_Preset(ID);
						if (presetx && presetx->Get_Definition() && presetx->Get_Definition()->Get_Name())
						{
							SetDlgItemTextA(hDlg, IDC_SOUND_ID, presetx->Get_Definition()->Get_Name());
						}
						else
						{
							SetDlgItemTextA(hDlg, IDC_SOUND_ID, "");
						}
					}
				}
				return TRUE;
			default:
				break;
			}
			break;
		default:
			break;			
	}	
	return FALSE;
}

int TDBEdit(TDBObjClass *obj2)
{
	int i = DialogBoxParam(g_hInstance, (LPWSTR)IDD_Info, g_MainWnd, Dlg_Info, (LPARAM)obj2);
	SetFocus(g_List);
	return i;
}
