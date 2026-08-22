#include <windows.h>
#include "Base.h"
#include "BaseTypes.h"
#include "StreamMap.h"
#include "../Shared/StdLib.h"

/*
Char	Type		Write args	Read args	
s		ssize_32	ssize_32	&ssize_32
S		ssize_64	ssize_64	&ssize_64
u		size_32		size_32		&size_32
U		size_64		size_64		&size_64
i		int			int			&int
h		HANDLE		HANDLE		&HANDLE
p		void*		size, ptr	buffersize, &size, ptr
c		char*		char*		buffersize, char*
C		wchar_t*	wchar_t*	buffersize, wchar_t*
b		bool		bool		&bool
*/

CStreamMap::CStreamMap(){
	m_hPipe = NULL;
}

void CStreamMap::SetPipe(HANDLE hPipe){
	m_hPipe = hPipe;
}

bool CStreamMap::ReadPipe(const char* pFmt, ...){
	va_list va;
	va_start(va, pFmt);
	int count, cmds;
	DWORD r;
	count = 0;
	size_32 size;
	cmds = StrLen(pFmt);

	while (*pFmt){
		switch (*pFmt){
			case SMAP_SSIZE32:
				if (ReadFile(m_hPipe, va_arg(va, ssize_32*), sizeof(ssize_32), &r, NULL))
					count++;
				break;
			case SMAP_SSIZE64:
				if (ReadFile(m_hPipe, va_arg(va, ssize_64*), sizeof(ssize_64), &r, NULL))
					count++;
				break;
			case SMAP_SIZE32:
				if (ReadFile(m_hPipe, va_arg(va, size_32*), sizeof(size_32), &r, NULL))
					count++;
				break;
			case SMAP_SIZE64:
				if (ReadFile(m_hPipe, va_arg(va, size_64*), sizeof(size_64), &r, NULL))
					count++;
				break;
			case SMAP_INT:
				if (ReadFile(m_hPipe, va_arg(va, int*), sizeof(int), &r, NULL))
					count++;
				break;
			case SMAP_HANDLE:
				if (ReadFile(m_hPipe, va_arg(va, HANDLE*), sizeof(HANDLE), &r, NULL))
					count++;
				break;
			case SMAP_BUFFER:
				ReadFile(m_hPipe, &size, sizeof(size_32), &r, NULL);
				**va_arg(va, size_32**) = size;
				if (ReadFile(m_hPipe, va_arg(va, void*), size, &r, NULL))
					count++;
				break;
			case SMAP_ANSI:
				ReadFile(m_hPipe, &size, sizeof(size_32), &r, NULL);
				**va_arg(va, size_32**) = size;
				if (ReadFile(m_hPipe, va_arg(va, char*), size, &r, NULL))
					count++;
				break;
			case SMAP_WCHAR:
				ReadFile(m_hPipe, &size, sizeof(size_32), &r, NULL);
				(**(size_32**)va) = size;
				va_arg(va, size_32**);
				if (ReadFile(m_hPipe, va_arg(va, wchar_t*), size, &r, NULL))
					count++;
				break;
			case SMAP_BOOL:
				if (ReadFile(m_hPipe, va_arg(va, bool*), sizeof(bool), &r, NULL))
					count++;
				break;
			case SMAP_PTR:
				if (ReadFile(m_hPipe, va_arg(va, void**), sizeof(void*), &r, NULL))
					count++;
				break;
			default:
				break;
		}
		pFmt++;
	}
	return (count == cmds);
}

bool CStreamMap::WritePipe(const char* pFmt, ...){
	va_list va;
	va_start(va, pFmt);
	int count, cmds;
	DWORD w;
	count = 0;
	size_32 size;

	cmds = StrLen(pFmt);

	while (*pFmt){
		switch (*pFmt){
			case SMAP_SSIZE32:
				if (WriteFile(m_hPipe, &va_arg(va, ssize_32), sizeof(ssize_32), &w, NULL))
					count++;
				break;
			case SMAP_SSIZE64:
				if (WriteFile(m_hPipe, &va_arg(va, ssize_64), sizeof(ssize_64), &w, NULL))
					count++;
				break;
			case SMAP_SIZE32:
				if (WriteFile(m_hPipe, &va_arg(va, size_32), sizeof(size_32), &w, NULL))
					count++;
				break;
			case SMAP_SIZE64:
				if (WriteFile(m_hPipe, &va_arg(va, size_64), sizeof(size_64), &w, NULL))
					count++;
				break;
			case SMAP_INT:
				if (WriteFile(m_hPipe, &va_arg(va, int), sizeof(int), &w, NULL))
					count++;
				break;
			case SMAP_HANDLE:
				if (WriteFile(m_hPipe, &va_arg(va, HANDLE), sizeof(HANDLE), &w, NULL))
					count++;
				break;
			case SMAP_BUFFER:
				size = va_arg(va, size_32);
				WriteFile(m_hPipe, &size, sizeof(size_32), &w, NULL);
				if (WriteFile(m_hPipe, va_arg(va, void**), size, &w, NULL))
					count++;
				break;
			case SMAP_ANSI:
				size = StrLen(*(char**)va)+1;
				WriteFile(m_hPipe, &size, sizeof(size_32), &w, NULL);
				if (WriteFile(m_hPipe, va_arg(va, char**), size, &w, NULL))
					count++;
				break;
			case SMAP_WCHAR:
				size = StrLenW(*(wchar_t**)va)+1;
				WriteFile(m_hPipe, &size, sizeof(size_32), &w, NULL);
				if (WriteFile(m_hPipe, va_arg(va, wchar_t**), size, &w, NULL))
					count++;
				break;
			case SMAP_BOOL:
				if (WriteFile(m_hPipe, &va_arg(va, bool), sizeof(bool), &w, NULL))
					count++;
				break;
			case SMAP_PTR:
				if (WriteFile(m_hPipe, &va_arg(va, void*), sizeof(void*), &w, NULL))
					count++;
				break;
			default:
				break;
		}
		pFmt++;
	}
	return (count == cmds);
}

bool CStreamMap::Peek(size_32 *pAvailable, ptr pBuf, DWORD BufSize){
	DWORD avail;
	if (!PeekNamedPipe(m_hPipe, pBuf, BufSize, NULL, &avail, NULL)) return false;
	if (pAvailable) *pAvailable = size_32(avail);
	return true;
}

