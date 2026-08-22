#include "platform.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "MemTracker.h"

DWORD TLSIndex = TLS_OUT_OF_INDEXES;
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{ 
	UNUSED_VARIABLE(instance);
	UNUSED_VARIABLE(reserved);
	switch (reason) 
	{ 
	case DLL_PROCESS_ATTACH: 
		// Allocate a TLS index.
		TLSIndex = TlsAlloc();
		if (TLSIndex == TLS_OUT_OF_INDEXES)
		{
			// Uhoh.
			return FALSE;
		}
		// No break: Initialize the index for first thread.
	case DLL_THREAD_ATTACH: 
		// Initialize the TLS index for this thread.
		{
			void* tls_data = SystemAllocate(sizeof(MemoryTrackerThreadLocalInformation));
			memset(tls_data, 0x00, sizeof(MemoryTrackerThreadLocalInformation));
			if (tls_data != NULL)
			{
				TlsSetValue(TLSIndex, tls_data);
			};
		}
		break; 
	case DLL_THREAD_DETACH: 
		// Release the allocated memory for this thread.
		{
			void* tls_data = TlsGetValue(TLSIndex);
			if (tls_data != NULL)
			{
				SystemFree(tls_data);
			};
		}
		break; 
	case DLL_PROCESS_DETACH: 
		// Release the allocated memory for this thread.
		void* tls_data = TlsGetValue(TLSIndex);
		if (tls_data != NULL)
		{
			SystemFree(tls_data);
		};

		// Release the TLS index.
		TlsFree(TLSIndex); 
		break; 
	} 

	return TRUE; 
};