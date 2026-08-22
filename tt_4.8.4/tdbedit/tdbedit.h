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
#ifndef _MAIN_H_
#define _MAIN_H_
#include <windows.h>
#include <commctrl.h>

#define LIST_STL (/*LVS_SINGLESEL |*/ LVS_REPORT | LVS_SHOWSELALWAYS  )
#define LIST_EX_STL (LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES)
#define TAB_STL (TCS_BUTTONS | TCS_FLATBUTTONS)

extern HWND g_MainWnd;
extern HINSTANCE g_hInstance;
extern bool g_NoSound;
extern bool g_3DSound;
void Goto(int idx);

void Set_Status_Text(wchar_t *text);
void Set_Status_Text_Formated(wchar_t *format, ...);
void LogFormated(wchar_t *fmt, ...);
void LogError();

#endif //_MAIN_H_