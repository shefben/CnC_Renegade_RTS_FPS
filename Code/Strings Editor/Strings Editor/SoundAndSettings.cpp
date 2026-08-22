#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "main.h"
#include "SoundAndSettings.h"
#include "resource.h"
#include "..\..\sndapi\sndapi\interfaces.h"

#define MAIN_KEY HKEY_LOCAL_MACHINE
#define SUB_KEY L"SOFTWARE\\IEE\\String edit\\"

ISoundDatabase *g_iSoundDatabase = NULL;
RECT g_rPosition = { 0, 0, 0, 0 };
HKEY g_hKey = NULL;

void Apply_Window_Pos(RECT *r){
	SetWindowPos(g_MainWnd, NULL, r->left, r->top, r->right-r->left, r->bottom-r->top, SWP_NOZORDER);
}

#pragma warning (disable : 4311)

void Set_Menu_Items(){
	HMENU hMenu = GetMenu(g_MainWnd), hSubMenu;
	int cItems = GetMenuItemCount(hMenu);
	hSubMenu = CreatePopupMenu();
	AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hSubMenu, L"sndapi");
	AppendMenu(hSubMenu, MF_STRING, ID_SNDAPIOPTIONS, L"Options");
	AppendMenu(hSubMenu, MF_STRING, ID_SNDAPICONVERT, L"Convert");
}

#pragma warning (default : 4311)

void Load_Settings(){
	HKEY hKey;
	unsigned long type, size = sizeof(RECT);
	if (g_iSoundDatabase = Query_Database())
		Set_Menu_Items();
	if (RegCreateKeyEx(MAIN_KEY, SUB_KEY, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL) != ERROR_SUCCESS) return;
	g_hKey = hKey;
	
	if (RegQueryValueEx(hKey, L"windowpos", NULL, &type, (unsigned char*)&g_rPosition, &size) == ERROR_SUCCESS){
		if (size == sizeof(RECT) && type == REG_BINARY)
			Apply_Window_Pos(&g_rPosition);
	}
	
	if (g_iSoundDatabase)
		g_iSoundDatabase->Apply_Options(hKey);	
}

void Save_Settings(){
	HKEY hKey = g_hKey;
	if (!hKey)
		if (RegCreateKeyEx(MAIN_KEY, SUB_KEY, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL) != ERROR_SUCCESS) return;
	GetWindowRect(g_MainWnd, &g_rPosition);
	RegSetValueEx(hKey, L"windowpos", 0, REG_BINARY, (unsigned char*)&g_rPosition, sizeof(RECT));
}