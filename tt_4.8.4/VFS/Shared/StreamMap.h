#ifndef _STREAMMAP_H_
#define _STREAMMAP_H_

#include <windows.h>

/*
*** Stream map format ***
Char	Type		Write args	Read args	
s		ssize_32	ssize_32	&ssize_32
S		ssize_64	ssize_64	&ssize_64
u		size_32		size_32		&size_32
U		size_64		size_64		&size_64
i		int			int			&int
h		HANDLE		HANDLE		&HANDLE
p		void*		size, ptr	&size, ptr
c		char*		char*		char*
C		wchar_t*	wchar_t*	wchar_t*
b		bool		bool		&bool
v		void*		void*		&void*
*/

enum {
	SMAP_SSIZE32	= 's',
	SMAP_SSIZE64	= 'S',
	SMAP_SIZE32		= 'u',
	SMAP_SIZE64		= 'U',
	SMAP_SSIZE16	= 'w',
	SMAP_SIZE16		= 'W',
	SMAP_INT		= 'i',
	SMAP_HANDLE		= 'h',
	SMAP_BUFFER		= 'p',
	SMAP_ANSI		= 'c',
	SMAP_WCHAR		= 'C',
	SMAP_BOOL		= 'b',
	SMAP_PTR		= 'v',
};


class CStreamMap {
private:
	HANDLE	m_hPipe;
public:
	CStreamMap();
	void SetPipe(HANDLE hPipe);
	bool ReadPipe(const char* pFmt, ...);
	bool WritePipe(const char* pFmt, ...);
	bool Peek(size_32 *pAvailable, ptr pBuf, DWORD BufSize);
};

#endif //_STREAMMAP_H_
