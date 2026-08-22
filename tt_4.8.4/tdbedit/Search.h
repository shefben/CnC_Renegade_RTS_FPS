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
#ifndef _SEARCH_H_
#define _SEARCH_H_

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "TranslateDBClass.h"

void FindByEnglishString(const wchar_t *_substr);
void FindByString(const wchar_t *_substr);
void FindByID(const wchar_t *_substr);
void DoGoto();
void ClearResults();
void Close();
void DoFindByEnglishString();
void DoFindByString();
void DoFindByID();
int TDBEdit(TDBObjClass *obj);

INT_PTR CALLBACK Dlg_Results(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK Dlg_Goto(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK Dlg_Str(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif //_SEARCH_H_