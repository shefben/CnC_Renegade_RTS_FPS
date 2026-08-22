#include "platform.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
	UNUSED_VARIABLE(instance);
	UNUSED_VARIABLE(reason);
	UNUSED_VARIABLE(reserved);
	return TRUE;
} 