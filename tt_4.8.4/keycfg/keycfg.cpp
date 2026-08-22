/*	Renegade Scripts.dll
	Keys.cfg editor
	Copyright 2009 Jonathan Wilson

	This file is part of the Renegade scripts.dll
	The Renegade scripts.dll is free software; you can redistribute it and/or modify it under
	the terms of the GNU General Public License as published by the Free
	Software Foundation; either version 2, or (at your option) any later
	version. See the file COPYING for more details.
	In addition, an exemption is given to allow Run Time Dynamic Linking of this code with any closed source module that does not contain code covered by this licence.
	Only the source code to the module(s) containing the licenced code has to be released.
*/

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x500
#include <stdio.h>
#include <windows.h>

#pragma warning (disable: 4100)
#include "resource.h"
template <class T> class SimpleVecClass {
protected:
	T *Vector;
	int VectorMax;
public:
	SimpleVecClass(int size = 0)
	{
		Vector = 0;
		VectorMax = 0;
		if (size > 0)
		{
			Resize(size);
		}
	}
	virtual ~SimpleVecClass()
	{
		if (Vector)
		{
			delete[] Vector;
			Vector = 0;
			VectorMax = 0;
		}
	}
	virtual bool Uninitialised_Grow(int newsize)
	{
		if (newsize <= VectorMax)
		{
			return true;
		}
		if (newsize > 0)
		{
			if (Vector)
			{
				delete[] Vector;
			}
			Vector = new T[newsize];
			VectorMax = newsize;
		}
		return true;
	}
	int Length() const
	{
		return VectorMax;
	}
	T &operator[](int index)
	{
		return Vector[index];
	}
	T const &operator[](int index) const
	{
		return Vector[index];
	}
	virtual bool Resize(int newsize)
	{
		if (VectorMax == newsize)
		{
			return true;
		}
		if (newsize > 0)
		{
			T *vec = new T[newsize];
			if (Vector)
			{
				int count = VectorMax;
				if (newsize < count)
				{
					count = newsize;
				}
				memcpy(vec,Vector,count*sizeof(T));
				delete[] Vector;
				Vector = 0;
			}
			Vector = vec;
			VectorMax = newsize;
		}
		else
		{
			VectorMax = 0;
			if (Vector)
			{
				delete[] Vector;
				Vector = 0;
			}
		}
		return true;
	}
	void Zero_Memory()
	{
		if (Vector != NULL)
		{
			memset(Vector,0,VectorMax * sizeof(T));
		}
	}
}; // 000C

template <class T> class SimpleDynVecClass : public SimpleVecClass<T> {
protected:
	int ActiveCount;
	bool Grow(int new_size_hint)
	{
		int new_size = max(VectorMax + VectorMax/4,VectorMax + 4);
		new_size = max(new_size,new_size_hint);
		return Resize(new_size);
	}
	bool Shrink(void)
	{
		if (ActiveCount < VectorMax/4)
		{
			return Resize(ActiveCount);
		}
		return true;
	}
	int Find_Index(T const & object)
	{
		for (int i = 0;i < Count();i++)
		{
			if (Vector[index] == object)
			{
				return index;
			}
		}
		return -1;
	}
public:
	SimpleDynVecClass(int size = 0) : SimpleVecClass<T>(size)
	{
		ActiveCount = size;
	}
	int Count() const
	{
		return ActiveCount;
	}
	T &operator[](int index)
	{
		return Vector[index];
	}
	T const &operator[](int index) const
	{
		return Vector[index];
	}
	~SimpleDynVecClass()
	{
		if (Vector)
		{
			delete[] Vector;
			Vector = 0;
		}
	}
	bool Resize(int newsize)
	{
		if (SimpleVecClass<T>::Resize(newsize))
		{
			if (VectorMax < ActiveCount)
			{
				ActiveCount = VectorMax;
			}
			return true;
		}
		return false;
	}
	bool Add(T const& data, int new_size_hint = 0)
	{
		if (ActiveCount >= VectorMax)
		{
			if (!Grow(new_size_hint))
			{
				return false;
			}
		}
		Vector[ActiveCount++] = data;
		return true;
	}
	T *Add_Multiple(int number_to_add)
	{
		int index = ActiveCount;
		ActiveCount += number_to_add;
		if (ActiveCount >= VectorMax)
		{
			Grow( ActiveCount );
		}
		return &Vector[index];
	}
	bool Delete(int index,bool allow_shrink = true)
	{
		if (index < ActiveCount-1)
		{
			memmove(&(Vector[index]),&(Vector[index+1]),(ActiveCount - index - 1) * sizeof(T));
		}
		ActiveCount--;
		if (allow_shrink)
		{
			Shrink();
		}
		return true;
	}
	bool Delete(T const & object,bool allow_shrink = true)
	{
		int id = Find_Index(object);
		if (id != -1)
		{
			return Delete(id,allow_shrink);
		}
		return false;
	}
	bool Delete_Range(int start,int count,bool allow_shrink = true)
	{
		if (start < ActiveCount - count)
		{
			memmove(&(Vector[start]),&(Vector[start + count]),(ActiveCount - start - count) * sizeof(T));
		}
		ActiveCount -= count;
		if (allow_shrink)
		{
			Shrink();
		}
		return true;
	}
	void Delete_All(bool allow_shrink = true)
	{
		ActiveCount = 0;
		if (allow_shrink)
		{
			Shrink();
		}
	}
};
struct Key {
	char *LogicalKeyName;
	int KeyNumber;
};
SimpleDynVecClass<Key *> KeyList = SimpleDynVecClass<Key *>(0);
struct keynames {
	const char *translatedkeyname;
	const char *keyname;
};
const keynames keynamearr[115] = {
{"None","None_Key"},
{"F1","F1_Key"},
{"F2","F2_Key"},
{"F3","F3_Key"},
{"F4","F4_Key"},
{"F5","F5_Key"},
{"F6","F6_Key"},
{"F7","F7_Key"},
{"F8","F8_Key"},
{"F9","F9_Key"},
{"F10","F10_Key"},
{"F11","F11_Key"},
{"F12","F12_Key"},
{"0","0_Key"},
{"1","1_Key"},
{"2","2_Key"},
{"3","3_Key"},
{"4","4_Key"},
{"5","5_Key"},
{"6","6_Key"},
{"7","7_Key"},
{"8","8_Key"},
{"9","9_Key"},
{"A","A_Key"},
{"B","B_Key"},
{"C","C_Key"},
{"D","D_Key"},
{"E","E_Key"},
{"F","F_Key"},
{"G","G_Key"},
{"H","H_Key"},
{"I","I_Key"},
{"J","J_Key"},
{"K","K_Key"},
{"L","L_Key"},
{"M","M_Key"},
{"N","N_Key"},
{"O","O_Key"},
{"P","P_Key"},
{"Q","Q_Key"},
{"R","R_Key"},
{"S","S_Key"},
{"T","T_Key"},
{"U","U_Key"},
{"V","V_Key"},
{"W","W_Key"},
{"X","X_Key"},
{"Y","Y_Key"},
{"Z","Z_Key"},
{"MINUS","Minus_Key"},
{"EQUALS","Equals_Key"},
{"BACK","Backspace_Key"},
{"TAB","Tab_Key"},
{"LBRACKET","Left_Bracket_Key"},
{"RBRACKET","Right_Bracket_Key"},
{"RETURN","Enter_Key"},
{"SEMICOLON","Semicolon_Key"},
{"'","Apostrophe_Key"},
{"GRAVE/~","Grave_Key"},
{"BACKSLASH","Backslash_Key"},
{"COMMA","Comma_Key"},
{"PERIOD","Period_Key"},
{"SLASH","Slash_Key"},
{"SPACE","Space_Bar_Key"},
{"CAPS LOCK","Caps_Lock_Key"},
{"NUM LOCK","Num_Lock_Key"},
{"SCROLL LOCK","Scroll_Lock_Key"},
{"ESCAPE","Escape_Key"},
{"NUMPAD0","Keypad_0_Key"},
{"NUMPAD1","Keypad_1_Key"},
{"NUMPAD2","Keypad_2_Key"},
{"NUMPAD3","Keypad_3_Key"},
{"NUMPAD4","Keypad_4_Key"},
{"NUMPAD5","Keypad_5_Key"},
{"NUMPAD6","Keypad_6_Key"},
{"NUMPAD7","Keypad_7_Key"},
{"NUMPAD8","Keypad_8_Key"},
{"NUMPAD9","Keypad_9_Key"},
{"NUMPAD MINUS","Keypad_Minus_Key"},
{"NUMPAD MULTIPLY","Keypad_Star_Key"},
{"NUMPAD PLUS","Keypad_Plus_Key"},
{"NUMPAD DECIMAL","Keypad_Period_Key"},
{"NUMPAD ENTER","Keypad_Enter_Key"},
{"NUMPAD DIVIDE","Keypad_Slash_Key"},
{"HOME","Home_Key"},
{"PAGE UP","Page_Up_Key"},
{"END","End_Key"},
{"PAGE DOWN","Page_Down_Key"},
{"INSERT","Insert_Key"},
{"DELETE","Delete_Key"},
{"ARROW UP","Up_Key"},
{"ARROW DOWN","Down_Key"},
{"ARROW LEFT","Left_Key"},
{"ARROW RIGHT","Right_Key"},
{"SYSRQ","Sys_Req_Key"},
{"CONTROL","Control_Key"},
{"LEFT CONTROL","Left_Control_Key"},
{"RIGHT CONTROL","Right_Control_Key"},
{"SHIFT","Shift_Key"},
{"LEFT SHIFT","Left_Shift_Key"},
{"RIGHT SHIFT","Right_Shift_Key"},
{"ALT","Alt_Key"},
{"LEFT ALT","Left_Alt_Key"},
{"RIGHT ALT","Right_Alt_Key"},
{"WIN","Windows_Key"},
{"LEFT WIN","Left_Windows_Key"},
{"RIGHT WIN","Right_Windows_Key"},
{"APPS","App_Menu_Key"},
{"MOUSE LEFT BUTTON","Left_Mouse_Button"},
{"MOUSE RIGHT BUTTON","Right_Mouse_Button"},
{"MOUSE CENTER BUTTON","Center_Mouse_Button"},
{"MOUSE LEFT SIDE BUTTON","Left_Side_Mouse_Button"},
{"MOUSE RIGHT SIDE BUTTON","Right_Side_Mouse_Button"},
{"MOUSE WHEEL FORWARD","Mouse_Wheel_Forward"},
{"MOUSE WHEEL BACKWARD","Mouse_Wheel_Backward"}};
int getkey(const char *key)
{
	for (int i = 0;i < 115; i++)
	{
		if (!_stricmp(key,keynamearr[i].keyname))
		{
			return i;
		}
	}
	return -1;
}
BOOL CALLBACK DlgProc(HWND hwnd,UINT Message,WPARAM wParam,LPARAM lParam)
{
	switch(Message)
	{
		case WM_INITDIALOG:
			{
				SendMessage(hwnd,WM_SETICON,ICON_SMALL,(LPARAM)LoadImage(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_APP),IMAGE_ICON,32,32,LR_DEFAULTCOLOR));
				SendMessage(hwnd,WM_SETICON,ICON_BIG,(LPARAM)LoadImage(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_APP),IMAGE_ICON,32,32,LR_DEFAULTCOLOR));
				for (int i = 0;i < 115; i++)
				{
					SendDlgItemMessage(hwnd,IDC_PHYSICAL,LB_ADDSTRING,0,(LPARAM)keynamearr[i].translatedkeyname);
				}
				SendDlgItemMessage(hwnd,IDC_PHYSICAL,LB_SETCURSEL,(WPARAM)0,(LPARAM)0);
				FILE *f = fopen("data\\keys.cfg","rt");
				if (f)
				{
					char data[100];
					while (fgets(data,100,f))
					{
						Key *k = new Key;
						char *a = strtok(data,"=");
						char *b = strtok(NULL,"=");
						k->LogicalKeyName = _strdup(a);
						b[strlen(b)-1] = 0;
						k->KeyNumber = getkey(b);
						KeyList.Add(k);
						SendDlgItemMessage(hwnd,IDC_LOGICAL,LB_ADDSTRING,0,(LPARAM)k->LogicalKeyName);
					}
					fclose(f);
					SendDlgItemMessage(hwnd,IDC_LOGICAL,LB_SETCURSEL,(WPARAM)0,(LPARAM)0);
					int pos = KeyList[0]->KeyNumber;
					SendDlgItemMessage(hwnd,IDC_PHYSICAL,LB_SETCURSEL,pos,0);
				}
				break;
			}
		case WM_CLOSE:
			EndDialog(hwnd,0);
		break;
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDC_LOGICAL:
					switch(HIWORD(wParam))
					{
						case LBN_SELCHANGE:
						{
							int index = SendDlgItemMessage(hwnd,IDC_LOGICAL,LB_GETCURSEL,(WPARAM)0,(LPARAM)0);
							int pos = KeyList[index]->KeyNumber;
							SendDlgItemMessage(hwnd,IDC_PHYSICAL,LB_SETCURSEL,pos,0);
						}
					}
				break;
				case IDC_PHYSICAL:
					switch(HIWORD(wParam))
					{
						case LBN_SELCHANGE:
						{
							int index = SendDlgItemMessage(hwnd,IDC_LOGICAL,LB_GETCURSEL,(WPARAM)0,(LPARAM)0);
							if (index != -1)
							{
								int num = SendDlgItemMessage(hwnd,IDC_PHYSICAL,LB_GETCURSEL,(WPARAM)0,(LPARAM)0);
								KeyList[index]->KeyNumber = num;
							}
						}
					}
				break;
				case IDC_ADD:
					switch(HIWORD(wParam))
					{
						case BN_CLICKED:
						{
							int len = SendDlgItemMessage(hwnd,IDC_NEWKEY,WM_GETTEXTLENGTH,0,0);
							char *a = new char[len+1];
							SendDlgItemMessage(hwnd,IDC_NEWKEY,WM_GETTEXT,(WPARAM)len+1,(LPARAM)a);
							Key *k = new Key;
							k->LogicalKeyName = _strdup(a);
							k->KeyNumber = 0;
							KeyList.Add(k);
							int index = SendDlgItemMessage(hwnd,IDC_LOGICAL,LB_ADDSTRING,0,(LPARAM)k->LogicalKeyName);
							SendDlgItemMessage(hwnd,IDC_LOGICAL,LB_SETCURSEL,(WPARAM)index,(LPARAM)0);
							SendDlgItemMessage(hwnd,IDC_PHYSICAL,LB_SETCURSEL,(WPARAM)0,(LPARAM)0);
						}
					}
				break;
				case IDC_QUIT:
					switch(HIWORD(wParam))
					{
						case BN_CLICKED:
						{
							EndDialog(hwnd,0);
						}
					}
				break;
				case IDC_SAVE:
					switch(HIWORD(wParam))
					{
						case BN_CLICKED:
						{
							FILE *f = fopen("data\\keys.cfg","wt");
							if (f)
							{
								int x = KeyList.Count();
								for (int i = 0;i < x;i++)
								{
									fprintf(f,"%s=%s\n",KeyList[i]->LogicalKeyName,keynamearr[KeyList[i]->KeyNumber].keyname);
								}
								fclose(f);
							}
							else
							{
								MessageBox(hwnd,"Cannot save keys.cfg, make sure keycfg.exe is in your renegade folder","Error",MB_OK);
							}
							EndDialog(hwnd,0);
						}
					}
				break;
			}
		default:
			return FALSE;
	}
	return TRUE;
}

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,int nCmdShow)
{
	return DialogBox(hInstance,MAKEINTRESOURCE(IDD_MAIN),NULL,DlgProc);
}
