#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "Console.h"
#include "StdLib.h"
#include "Base.h"
#include "BaseClasses.h"

CConsole::CConsole(bool init){
	if ((m_Inited = init)){
		AllocConsole();
		m_hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
		m_hConsoleIn = GetStdHandle(STD_INPUT_HANDLE);
		SetConsoleMode(m_hConsoleIn, 0);
	}
}

CConsole::~CConsole(){
	FreeConsole();
}

void CConsole::Init(){
	m_Inited = true;
	AllocConsole();
	m_hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
	m_hConsoleIn = GetStdHandle(STD_INPUT_HANDLE);
	SetConsoleMode(m_hConsoleIn, 0);
}

void CConsole::SetTitle(const wchar_t* pTitle){
	SetConsoleTitleW(pTitle);
}

void CConsole::SetInfo(int w, int h, int bh){
	_SMALL_RECT rect;
	COORD size;
	size.X = w;
	size.Y = bh;

	rect.Left = 0;
	rect.Top = 0;
	rect.Right = w-1;
	rect.Bottom = h-1;

	SetConsoleScreenBufferSize(m_hConsoleOut, size);
	SetConsoleWindowInfo(m_hConsoleOut, TRUE, &rect);
}

void CConsole::FillAttributes(WORD attributes){
}

void CConsole::Write(const wchar_t* pText, WORD Attributes){
	DWORD w;
	if (Attributes == 0xFFFF)
		Attributes = (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	SetConsoleTextAttribute(m_hConsoleOut, Attributes);
	WriteConsoleW(m_hConsoleOut, pText, StrLenW(pText), &w, NULL);
}

void CConsole::WriteFmt(const wchar_t* pFmt, WORD Attributes, ...){
	wchar_t lBuf[2048];
	va_list va;
	va_start(va, Attributes);
	_vsnwprintf(lBuf, 2048, pFmt, va);
	Write(lBuf, Attributes);
}

void CConsole::WriteMultiFmt(const wchar_t* pText, WORD Attributes, int count, ...){
	va_list va;
	wchar_t lBuf[2048];
	va_start(va, count);
	while (pText){
		_vsnwprintf(lBuf, 2048, pText, va);
		Write(lBuf, Attributes);
		va = va_seek(va, count);
		pText = *(wchar_t**)va;
		if (!pText) break;
		va = va_next(va);
		Attributes = *(WORD*)va;
		va = va_next(va);
		count = *(int*)va;
		va = va_next(va);
	}
}

wchar_t CConsole::ReadChar(){
	wchar_t w;
	DWORD r;
	ReadConsoleW(m_hConsoleIn, &w, 1, &r, NULL);
	return w;
}

void CConsole::GetHandles(HANDLE *phConsoleIn, HANDLE *phConsoleOut){
	if (phConsoleIn) *phConsoleIn = m_hConsoleIn;
	if (phConsoleOut) *phConsoleOut = m_hConsoleOut;
}

/*
CRemoteConsole
*/

CRemoteConsole::CRemoteConsole(){
	m_hConsoleOut = INVALID_HANDLE_VALUE;
	m_hConsoleIn = INVALID_HANDLE_VALUE;
	m_hProcess = NULL;
}

CRemoteConsole::~CRemoteConsole(){
	Shutdown();
}

bool CRemoteConsole::GetHandles(const wchar_t* pClient){
	wchar_t lBuf[256];
	EventClass _event;
	PROCESS_INFORMATION pi;
	STARTUPINFOW si;
	memset(&si, 0, sizeof(STARTUPINFOW));
	si.cb = sizeof(STARTUPINFOW);

	swprintf(lBuf, L"i:%p o:%p e:%p p:%u", &m_hConsoleIn, &m_hConsoleOut, _event.GetHandle(), GetCurrentProcessId());
	if (!CreateProcessW(pClient, lBuf, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)){
		LogError();
		return false;
	}
	CloseHandle(pi.hThread);
	if (_event.Wait(10000)){
		TerminateProcess(pi.hProcess, 0);
		CloseHandle(pi.hProcess);
		return false;
	}
	SetConsoleMode(m_hConsoleIn, 0);
	m_hProcess = pi.hProcess;
	return true;
}

void CRemoteConsole::Shutdown(){
	if (m_hProcess){
		if (m_hConsoleIn != INVALID_HANDLE_VALUE) CloseHandle(m_hConsoleIn);
		if (m_hConsoleOut != INVALID_HANDLE_VALUE) CloseHandle(m_hConsoleOut);
		m_hConsoleIn = m_hConsoleOut = INVALID_HANDLE_VALUE;
		TerminateProcess(m_hProcess, 0);
		CloseHandle(m_hProcess);
		m_hProcess = NULL;
	}
}

void CRemoteConsole::SetTitle(const wchar_t* pTitle){
	SetConsoleTitleW(pTitle);
}

void CRemoteConsole::SetInfo(int w, int h, int bh){
	_SMALL_RECT rect;
	COORD size;
	size.X = w;
	size.Y = bh;

	rect.Left = 0;
	rect.Top = 0;
	rect.Right = w-1;
	rect.Bottom = h-1;

	SetConsoleScreenBufferSize(m_hConsoleOut, size);
	SetConsoleWindowInfo(m_hConsoleOut, TRUE, &rect);
}

void CRemoteConsole::FillAttributes(WORD attributes){
}

void CRemoteConsole::Write(const wchar_t* pText, WORD Attributes){
	DWORD w;
	if (Attributes == 0xFFFF)
		Attributes = (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	SetConsoleTextAttribute(m_hConsoleOut, Attributes);
	WriteConsoleW(m_hConsoleOut, pText, StrLenW(pText), &w, NULL);
}

void CRemoteConsole::WriteFmt(const wchar_t* pFmt, WORD Attributes, ...){
	wchar_t lBuf[2048];
	va_list va;
	va_start(va, Attributes);
	_vsnwprintf(lBuf, 2048, pFmt, va);
	Write(lBuf, Attributes);
}

void CRemoteConsole::WriteMultiFmt(const wchar_t* pText, WORD Attributes, int count, ...){
	va_list va;
	wchar_t lBuf[2048];
	va_start(va, count);
	while (pText){
		_vsnwprintf(lBuf, 2048, pText, va);
		Write(lBuf, Attributes);
		va = va_seek(va, count);
		pText = *(wchar_t**)va;
		va = va_next(va);
		Attributes = *(WORD*)va;
		va = va_next(va);
		count = *(int*)va;
		va = va_next(va);
	}
}

wchar_t CRemoteConsole::ReadChar(){
	wchar_t w;
	DWORD r;
	ReadConsoleW(m_hConsoleIn, &w, 1, &r, NULL);
	return w;
}
