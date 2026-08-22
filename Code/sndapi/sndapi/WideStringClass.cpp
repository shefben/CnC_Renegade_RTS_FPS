#include <stdio.h>
#include "WideStringClass.h"

char g_Empty_CStr[2] = "";
wchar_t g_Empty_WStr[2] = L"";

char *StringClass::Get_Empty_String(){
	return g_Empty_CStr;
}
StringClass::StringClass(int size, bool get_string){
	m_string = Get_Empty_String();
	Get_String(size, get_string);
	m_string[0] = 0;
}

StringClass::~StringClass(){
	Free_String();
}
void StringClass::Get_String(int size, bool get_string){
	if (!size || !get_string)
		return;
	Free_String();
	char *temp = (char *)malloc((size_t)size+8);
	*(int*)temp = size;
	*(int*)(temp+4) = 0;
	m_string = (temp+8);
}
void StringClass::Uninitialised_Grow(int size){
	int t_size;
	if (m_string == Get_Empty_String())
		t_size = 0;
	else
		t_size = *(int*)(m_string-8);
    if (size <= t_size)
		return;
	Get_String(size, true);
}
void StringClass::Free_String(){
	if (m_string == Get_Empty_String())
		return;
	free(m_string-8);
	m_string = Get_Empty_String();
}
void StringClass::SetString(const char *string){
	Uninitialised_Grow((int)strlen(string)+1);
	*(int*)(m_string-4) = (int)strlen(string);
	strcpy(m_string, string);
}
void StringClass::MidStr(int start, int len){
	if (m_string == Get_Empty_String()) return;
	memmove(m_string, m_string+start, len);
	m_string[len] = 0;
	*(int*)(m_string-4) = len;
}
void StringClass::leftstr(int len){
	MidStr(0, len);
}
void StringClass::rightstr(int len){
	MidStr((int)strlen(m_string)-len, len);
}
void StringClass::Cat(char *str){
	char *temp = (char*)malloc(1+(int)strlen(m_string)+(int)strlen(str));
	if (temp){
		strcpy(temp, m_string);
		strcat(temp, str);
		SetString(temp);
		free(temp);
	}
}

void StringClass::Format_Args(char const *format, va_list list){
	char buf[512];
	_vsnprintf(buf, 0x200, format, list);
	Uninitialised_Grow((int)strlen(buf)+1);
	if (m_string != Get_Empty_String())
		*(int*)(m_string-4) = (int)strlen(buf);
	memcpy(m_string, buf, strlen(buf)+1);
}
void StringClass::Format(char const *format,...){		
	va_list list;
	va_start(list, format);
	Format_Args(format, list);
	va_end(list);
}
char *StringClass::GetString(){
	return m_string;
}
/*const char *StringClass::operator = (StringClass &str){
	return m_string;
}	*/

StringClass &StringClass::operator = (const StringClass &str){	
	this->SetString(str.m_string);
	return *this;
}	

StringClass &StringClass::operator += (char *str){
	this->Cat(str);
	return *this;
}	

bool StringClass::operator == (StringClass &str){
	return (strcmp(this->m_string, str.m_string) == 0);
}
bool StringClass::operator != (StringClass &str){
	return !(strcmp(this->m_string, str.m_string) == 0);
}



wchar_t *WideStringClass::Get_Empty_String(){
	return g_Empty_WStr;
}
WideStringClass::WideStringClass(int size, bool get_string){
	m_string = Get_Empty_String();
	Get_String(size, get_string);
	m_string[0] = 0;
}
WideStringClass::~WideStringClass(){
	Free_String();
}
void WideStringClass::Get_String(int size, bool get_string){
	if (!size || !get_string)
		return;
	Free_String();
	m_string = (wchar_t*)malloc(4*size+8);
	
	*(int*)m_string = size;
	*(int*)(((char*)m_string)+4) = 0;
	m_string = (wchar_t*)(((char*)m_string)+8);
}
void WideStringClass::Uninitialised_Grow(int size){
	int t_size;
	if (m_string == Get_Empty_String())
		t_size = 0;
	else
		t_size = *(int*)(((char*)m_string)-8);
    if (size <= t_size)
		return;
	Get_String(size, true);
}
void WideStringClass::Free_String(){
	if (m_string == Get_Empty_String())
		return;
	if (m_string)
		free(((char*)m_string)-8);
	m_string = Get_Empty_String();
}
void WideStringClass::Format_Args(wchar_t const *format, va_list list){
	wchar_t buf[512];
	_vsnwprintf(buf, 0x200, format, list);
	Uninitialised_Grow((int)wcslen(buf)+1);
	if (!m_string) {
		m_string = Get_Empty_String();
		return;
	}
	if (m_string != Get_Empty_String())
		*(int*)(((char*)m_string)-8) = (int)wcslen(buf)+1;
	memcpy(m_string, buf, (wcslen(buf)+1)*sizeof(wchar_t));
}
void WideStringClass::Format(wchar_t const *format,...){		
	va_list list;
	va_start(list, format);
	Format_Args(format, list);
	va_end(list);
}
void WideStringClass::SetString(wchar_t *str){		
	Uninitialised_Grow((int)wcslen(str)+1);
	*(int*)(((char*)m_string)-4) = (int)wcslen(str);		
	wcscpy(m_string, str);
}
void WideStringClass::Convert_From(char const *text){
	this->Format(L"%S", text);
}
wchar_t *WideStringClass::GetString(){
	return (wchar_t*)m_string;
}
WideStringClass &WideStringClass::operator = (const WideStringClass &str){
	SetString(str.m_string);
	return *this;
}	
bool WideStringClass::Is_ANSI(){
	int i = 0;
	while (m_string[i]){
		if (m_string[i] > 255) return false;
		i++;
	}
	return true;
}
bool WideStringClass::Is_ASCII(){
	int i = 0;
	while (m_string[i]){
		if (m_string[i] > '~' || m_string[i] < ' ') return false;
		i++;
	}
	return true;
}

bool WideStringClass::Convert_From_2Byte(){
	int size = Get_Size();
	if (!size) return false;
	memcpy(&m_string[size], m_string, size*2);
	for (int i = 0; i < size; i++)
		m_string[i] = m_string[size+i] & 0xFF;
	return true;
}

bool WideStringClass::operator == (WideStringClass &str){
	return (wcsicmp(m_string, str.m_string) == 0);
}

bool WideStringClass::operator != (WideStringClass &str){
	return (wcsicmp(m_string, str.m_string) != 0);
}