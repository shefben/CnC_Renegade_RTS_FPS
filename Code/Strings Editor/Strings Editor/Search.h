#ifndef _SEARCH_H_
#define _SEARCH_H_

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "translation.h"
#include "..\..\sound_preset\sndapi\interfaces.h"

void FindByEnglishString(const wchar_t *_substr);
void FindByString(const wchar_t *_substr);
void FindByID(const wchar_t *_substr);
void DoGoto();
void ClearResults();
void Close();
void DoFindByEnglishString();
void DoFindByString();
void DoFindByID();
void TDBEdit(TDBObjClass *obj);
bool Has_Sound(TDBObjClass *obj, ISoundObject **Snd, bool *dodisp);

INT_PTR CALLBACK Dlg_Results(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK Dlg_Goto(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK Dlg_Str(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif //_SEARCH_H_