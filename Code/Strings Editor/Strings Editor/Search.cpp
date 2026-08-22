#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <commctrl.h>
#include "translation.h"
#include "main.h"
#include "resource.h"
#include "search.h"
#include "VectorAndListTemplateClasses.h"
#include "SoundAndSettings.h"
#include "..\..\sound_preset\sndapi\interfaces.h"

HWND g_List = NULL;
HWND g_Results = NULL;
bool g_ByIndex = false;

void Add_Object(TDBObjClass*obj, int idx){
	LVITEM lv;
	wchar_t lBuf[256];
	ZeroMemory(&lv, sizeof(lv));
	int pos = (int)SendMessage(g_List, LVM_GETITEMCOUNT, 0, 0);
	lv.mask = LVIF_PARAM | LVIF_TEXT;
	lv.iItem = pos;
	lv.lParam = (LPARAM)idx;
	_snwprintf(lBuf, 256, L"%S", obj->Get_ID_Desc());
	lv.pszText = lBuf;
	SendMessage(g_List, LVM_INSERTITEM, 0, (LPARAM)&lv);
	_snwprintf(lBuf, 256, L"%d", obj->Get_ID());
	lv.mask = LVIF_TEXT;
	lv.iSubItem = 1;
	SendMessage(g_List, LVM_SETITEM, 0, (LPARAM)&lv);
	_snwprintf(lBuf, 256, L"%S", obj->Get_English_String());
	lv.mask = LVIF_TEXT;
	lv.iSubItem = 2;
	SendMessage(g_List, LVM_SETITEM, 0, (LPARAM)&lv);
}

bool Make_Window(){
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

void FindByEnglishString(const wchar_t *_substr){
	ClearResults();	
	bool found_one = false;
	bool b;
	char *str = (char*)malloc(wcslen(_substr)+1);
	if (!str) return;
	_snprintf(str, wcslen(_substr)+1, "%S", _substr);
	DynamicVectorClass<TDBObjClass*> *p = &TDataBase::m_Objects;
	int cItems = p->m_nItemCount;
	TDBObjClass *o;
	for (int i = 0; i < cItems; i++){
		o = (*p)[i];
		if (!o) continue;
		Has_Sound(o, NULL, &b);
		if (!stristr((char*)o->Get_English_String(), (char*)str) || !b) continue;
		if (!found_one && !g_Results)
			found_one = Make_Window();
		Add_Object(o, i);
	}
	free(str);
}

void FindByString(const wchar_t *_substr){
	ClearResults();
	bool found_one = false;
	bool b;
	DynamicVectorClass<TDBObjClass*> *p = &TDataBase::m_Objects;
	int cItems = p->m_nItemCount;
	TDBObjClass *o;
	for (int i = 0; i < cItems; i++){
		o = (*p)[i];
		if (!o) continue;
		Has_Sound(o, NULL, &b);
		if (!wcsistr((wchar_t*)o->Get_String(), (wchar_t*)_substr) || !b) continue;
		if (!found_one && !g_Results)
			found_one = Make_Window();
		Add_Object(o, i);
	}
}

void FindByID(const wchar_t *_substr){
	ClearResults();	
	bool found_one = false;
	bool b;
	char *str = (char*)malloc(wcslen(_substr)+1);
	if (!str) return;
	_snprintf(str, wcslen(_substr)+1, "%S", _substr);
	DynamicVectorClass<TDBObjClass*> *p = &TDataBase::m_Objects;
	int cItems = p->m_nItemCount;
	TDBObjClass *o;
	for (int i = 0; i < cItems; i++){
		o = (*p)[i];
		if (!o) continue;
		Has_Sound(o, NULL, &b);
		if (!stristr((char*)o->Get_ID_Desc(), (char*)str) || !b) continue;
		if (!found_one && !g_Results)
			found_one = Make_Window();
		Add_Object(o, i);
	}
	free(str);
}

void ClearResults(){
	if (!g_List) return;
	SendMessage(g_List, LVM_DELETEALLITEMS, 0, 0);
}

void Close(){
	ClearResults();
	if (g_Results){
		EndDialog(g_Results, 0);
		g_Results = NULL;
	}
}

INT_PTR CALLBACK Dlg_Results(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam){
	BOOL translated = FALSE;
	LPNMITEMACTIVATE act;
	LVITEM item;
	ZeroMemory(&item, sizeof(item));
	
	switch (uMsg){
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
			if (act->hdr.hwndFrom == g_List && act->hdr.code == NM_DBLCLK){
				item.iItem = act->iItem;
				item.mask = LVIF_PARAM;
				SendMessage(g_List, LVM_GETITEM, 0, (LPARAM)&item);
				Goto((int)item.lParam);
				return TRUE;
			}
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam)){
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

void Goto_By_ID(int ID){
	DynamicVectorClass<TDBObjClass*> *p = &TDataBase::m_Objects;
	int cItems = p->m_nItemCount;
	TDBObjClass *o;
	for (int i = 0; i < cItems; i++){
		o = (*p)[i];
		if (!o) continue;
		if (o->Get_ID() == ID){
			Goto(i);
			return;
		}
	}
}

INT_PTR CALLBACK Dlg_Goto(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam){
	BOOL translated = FALSE;
	int res;
	switch (uMsg){
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
			switch (LOWORD(wParam)){
		case IDC_CANCEL:
			return EndDialog(hDlg, 0);
		case IDC_RADIOID:
		case IDC_RADIOIDX:
			
			g_ByIndex = SendMessage(GetDlgItem(hDlg, IDC_RADIOID), BM_GETCHECK, 0, 0) == BST_CHECKED ? false : true;
			break;
		case IDC_OK:
			res = GetDlgItemInt(hDlg, IDC_GOTO_INT, &translated, FALSE);
			LogFormated(L"res == %d\n", res);
			if (translated){
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

void DoGoto(){
	DialogBox(g_hInstance, (LPWSTR)IDD_Goto, g_MainWnd, Dlg_Goto);
	//if (res != 1) return;
	//if (!Make_Window()) return;
}

INT_PTR CALLBACK Dlg_English_Str(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam){
	BOOL translated = FALSE;
	wchar_t lBuf[256];
	switch (uMsg){
		case WM_INITDIALOG:
			SetFocus(GetDlgItem(hDlg, IDC_FIND_STRING));
			return TRUE;
			break;
		case WM_CLOSE:
			return EndDialog(hDlg, 0);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam)){
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

void DoFindByEnglishString(){
	DialogBox(g_hInstance, (LPWSTR)IDD_Find_Eng_String, g_MainWnd, Dlg_English_Str);	
}

INT_PTR CALLBACK Dlg_Str(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam){
	BOOL translated = FALSE;
	wchar_t lBuf[256];
	switch (uMsg){
		case WM_INITDIALOG:
			SetFocus(GetDlgItem(hDlg, IDC_FIND_STRING));
			return TRUE;
			break;
		case WM_CLOSE:
			return EndDialog(hDlg, 0);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam)){
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

void DoFindByString(){
	DialogBox(g_hInstance, (LPWSTR)IDD_Find_String, g_MainWnd, Dlg_Str);
}

INT_PTR CALLBACK Dlg_ID(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam){
	BOOL translated = FALSE;
	wchar_t lBuf[256];
	switch (uMsg){
		case WM_INITDIALOG:
			SetFocus(GetDlgItem(hDlg, IDC_FIND_STRING));
			return TRUE;
			break;
		case WM_CLOSE:
			return EndDialog(hDlg, 0);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam)){
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

void DoFindByID(){
	DialogBox(g_hInstance, (LPWSTR)IDD_Find_ID, g_MainWnd, Dlg_ID);	
}

#pragma warning (disable : 4311 4312)

bool Has_Sound(TDBObjClass *obj, ISoundObject **Snd, bool *dodisp){
	ISoundObject *sobj = NULL;
	if (Snd)
		*Snd = NULL;
	if (dodisp)
		*dodisp = false;

	unsigned long ID = obj->Get_Sound_ID();	
	if (ID == (unsigned long)-1 && !g_NoSound)
		return false;
	else if (!g_iSoundDatabase)
		return true;
	else {
		if (ID == (unsigned long)-1){
			if (dodisp) *dodisp = true;
			return false;
		}
		sobj = g_iSoundDatabase->Find_Sound(ID);
		if (!sobj)
			return false;
		else if (sobj->Is_3D_Sound() && !g_3DSound)
			return false;
		else {
			if (Snd) *Snd = sobj;
			if (dodisp) *dodisp = true;
			return true;
		}
	}
}

void DoInfo(HWND hDlg, TDBObjClass *obj){
	wchar_t lBuf[512];
	ISoundObject *sobj = NULL;
	unsigned long ID = obj->Get_Sound_ID();
	SetDlgItemTextA(hDlg, IDC_ID, obj->Get_ID_Desc());	
	SetDlgItemInt(hDlg, IDC_ID_N, obj->Get_ID(), TRUE);	
	SetDlgItemTextA(hDlg, IDC_ENG_STRING, obj->Get_English_String());
	SetDlgItemText(hDlg, IDC_TEXT, obj->Get_String());
	if (ID == (unsigned long)-1)
		SetDlgItemText(hDlg, IDC_SOUND_ID, L"<No sound attached>");
	else if (!g_iSoundDatabase)
		SetDlgItemInt(hDlg, IDC_SOUND_ID, ID, FALSE);		
	else {
		sobj = g_iSoundDatabase->Find_Sound(ID);
		if (!sobj)
			SetDlgItemText(hDlg, IDC_SOUND_ID, L"<Sound not found in database>");
		else if (sobj->Is_3D_Sound() && !g_3DSound)
			SetDlgItemText(hDlg, IDC_SOUND_ID, L"<It's a 3D sound and therefore not working>");
		else {
			EnableWindow(GetDlgItem(hDlg, IDC_PLAY), TRUE);
			_snwprintf(lBuf, 512, L"%S", sobj->Get_Filename());
			SetDlgItemText(hDlg, IDC_SOUND_ID, lBuf);
			SetWindowLong(hDlg, GWL_USERDATA, (LONG)sobj);
		}
	}
}

INT_PTR CALLBACK Dlg_Info(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam){
	BOOL translated = FALSE;
	ISoundObject *o;
	switch (uMsg){
		case WM_INITDIALOG:
			DoInfo(hDlg, (TDBObjClass*)lParam);
			return TRUE;
			break;
		case WM_CLOSE:
			SetFocus(g_List);
			return EndDialog(hDlg, 0);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam)){
		case IDC_CLOSE:
			SetFocus(g_List);
			return EndDialog(hDlg, 0);
		case IDC_PLAY:
			if (g_iSoundDatabase && (o = (ISoundObject*)GetWindowLong(hDlg, GWL_USERDATA))){
				g_iSoundDatabase->Play_Sound_Object(o);
			}
			break;
		/*case IDC_OK:
			GetDlgItemText(hDlg, IDC_FIND_STRING, lBuf, 256);
			FindByID(lBuf);			
			return EndDialog(hDlg, 1);*/
			
		default:
			break;
			}
			break;
		default:
			break;			
	}	
	return FALSE;
}

#pragma warning (default : 4311 4312)

void TDBEdit(TDBObjClass *obj){
	DialogBoxParam(g_hInstance, (LPWSTR)IDD_Info, g_MainWnd, Dlg_Info, (LPARAM)obj);
	SetFocus(g_List);
}