#include "General.h"
#include "hooksupport.h"

static const char jumpOp = '\xE9';
static const char callOp = '\xE8';
static const char nopOp = '\x90';

HANDLE hProcess = GetCurrentProcess();


#ifdef NDEBUG
void WriteMemory(void* lpBaseAddress, const void* lpBuffer, size_t nSize)
{
	TT_ASSERT(hProcess != INVALID_HANDLE_VALUE);
	DWORD dwOldProtect;
	DWORD dwOldProtect2;
	VirtualProtectEx(hProcess, lpBaseAddress, nSize, PAGE_EXECUTE_READWRITE, &dwOldProtect);
	WriteProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, NULL);
	VirtualProtectEx(hProcess, lpBaseAddress, nSize, dwOldProtect, &dwOldProtect2);
}
#else
void WriteMemory(void* lpBaseAddress, const void* lpBuffer, size_t nSize)
{
	DWORD dwOldProtect;
	DWORD dwOldProtect2;
	BOOL res = VirtualProtectEx(hProcess, lpBaseAddress, nSize, PAGE_EXECUTE_READWRITE, &dwOldProtect);
	TT_ASSERT(res);
	SIZE_T written = 0;
	res = WriteProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, &written);
	TT_ASSERT(res);
	TT_ASSERT(written == nSize);
	res = VirtualProtectEx(hProcess, lpBaseAddress, nSize, dwOldProtect, &dwOldProtect2);
	TT_ASSERT(res);
}
#endif

void WriteMemory(uint32 lpBaseAddress, const void* lpBuffer, size_t nSize)
{
	WriteMemory ((LPVOID)lpBaseAddress, lpBuffer, nSize);
}

void WriteVtable(void *location, void *function)
{
	WriteMemory(location,(void *)&function,4);
}

void WriteVtableForPlatform(void* a, void* b, void* function)
{
	void* address = 0;

	switch (Exe)
	{
		case 0: address = a; break;
		case 1: address = b; break;
	}

	TT_ASSERT(address);

	WriteVtable (address, function);
}

void WriteJump(void *location, void *function)
{
	if (!location)
		return;

	char *offset = (char *)((char *)function - (char *)location - 5);
	WriteMemory(location,&jumpOp,1);
	WriteMemory((char *)location+1,&offset,4);
}

void WriteJump(uint32 address, void* function)
{
	WriteJump ((void*)address, function);
}

void WriteJump(uint32 address, uint32 function)
{
	WriteJump ((void*)address, (void*)function);
}

void WriteJump(void *location, void *function, int nop_count)
{
	char *offset = (char *)((char *)function - (char *)location - 5);
	WriteMemory((char *)location,&jumpOp,1);
	WriteMemory((char *)location+1,&offset,4);
	for (int i = 0;i < nop_count;i++)
	{
		WriteMemory((char *)location+i+5,&nopOp,1);
	}
}

void WriteJumpForPlatform(void* a, void* b, void* function)
{
	void* address = 0;

    if (Exe == 6) InitEngine();
	switch (Exe)
	{
		case 0: address = a; break;
		case 1: address = b; break;
	}

	TT_ASSERT(address);

	WriteJump (address, function);
}

void WriteCall(void *location, void *function)
{
	char *offset = (char *)((char *)function - (char *)location - 5);
	WriteMemory(location,&callOp,1);
	WriteMemory((char *)location+1,&offset,4);
}

void WriteCall(void *location, void *function, int count)
{
	char *offset = (char *)((char *)function - (char *)location - 5);
	WriteMemory(location,&callOp,1);
	WriteMemory((char *)location+1,&offset,4);
	for (int i = 0;i < count;i++)
	{
		WriteMemory((char *)location+i+5,&nopOp,1);
	}
}

void WriteCallForPlatform(void* a, void* b, void* function)
{
	void* address = 0;

    if (Exe == 6) InitEngine();
	switch (Exe)
	{
		case 0: address = a; break;
		case 1: address = b; break;
	}

	TT_ASSERT(address);

	WriteCall (address, function);
}

void WriteCallForPlatform2(void* a, void* b, void* function, int count)
{
	void* address = 0;

	switch (Exe)
	{
		case 0: address = a; break;
		case 1: address = b; break;
	}

	TT_ASSERT(address);

	WriteCall(address, function, count);
}

void WriteNop(void *location, int count)
{
	for (int i = 0;i < count;i++)
	{
		WriteMemory((char *)location+i,&nopOp,1);
	}
}

void WriteNopForPlatform(void* a, void* b, int count)
{
	void* address = 0;

	switch (Exe)
	{
		case 0: address = a; break;
		case 1: address = b; break;
	}

	TT_ASSERT(address);

	WriteNop(address, count);
}

