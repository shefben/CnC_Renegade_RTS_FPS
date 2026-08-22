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