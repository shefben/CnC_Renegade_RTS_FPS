#ifndef _WIDESTRINGCLASS_H_
#define _WIDESTRINGCLASS_H_
#pragma once

//#include <string>
#include <windows.h>

class StringClass {
public:
	char *m_string;
	static char *Get_Empty_String();
	StringClass(int size = 0, bool get_string = false);
	~StringClass();
	void Get_String(int size, bool get_string);
	void Uninitialised_Grow(int size);
	void Free_String();
	void SetString(const char *string);
	void MidStr(int start, int len);
	void leftstr(int len);
	void rightstr(int len);
	void Cat(char *str);
	void Format_Args(char const *format, va_list list);
	void Format(char const *format,...);
	char *GetString();
	StringClass &operator = (const StringClass &str);
	StringClass &operator += (char *str);
	bool operator == (StringClass &str);
	bool operator != (StringClass &str);
	__inline char *operator ()() { return m_string; };
};


class WideStringClass {
public:
	wchar_t *m_string;
	wchar_t *Get_Empty_String();
	WideStringClass(int size = 0, bool get_string = false);
	~WideStringClass();
	void Get_String(int size, bool get_string);
	void Uninitialised_Grow(int size);
	void Free_String();
	void Format_Args(wchar_t const *format, va_list list);
	void Format(wchar_t const *format,...);
	void SetString(wchar_t *str);
	void Convert_From(char const *text);
	wchar_t *GetString();
	WideStringClass &operator = (const WideStringClass &str);
	bool Is_ANSI();
	bool Is_ASCII();	
	bool operator == (WideStringClass &str);
	bool operator != (WideStringClass &str);
	bool Convert_From_2Byte();
	__inline int *Strlen(){ return m_string == Get_Empty_String() ? NULL : (int*)((char*)m_string-4); };
	__inline int Get_Size(){ return m_string == Get_Empty_String() ? 0 : *(int*)((char*)m_string-8);};
	__inline wchar_t *operator ()(){ return m_string; };

};

#endif //_WIDESTRINGCLASS_H_