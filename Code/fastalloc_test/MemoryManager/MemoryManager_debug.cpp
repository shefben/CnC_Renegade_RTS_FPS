#include "platform.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "FastAllocator.h"
#include "MemTracker.h"
#include "MemoryManager.h"

PUSH_MEMORY_MACROS
#undef new
#undef delete

void CheckMemoryManagerConfiguration(int32 config)
{
	if (config != MEMORYMANAGER_DEBUGCONFIGURATION)
	{
		MessageBoxW(NULL, L"This is a friendly warning to remind you that running the debug-mode memory manager with release-mode executables is not recommended and may cause unexpected results.", L"MemoryManager.dll", MB_OK | MB_ICONWARNING);
	};
};

void SetThreadTrackingInformation(const char* file, unsigned int line, const char* function)
{
	MemoryTracker::SetThreadLocalInformation(file, function, line);
};

DECLSPEC_RESTRICT void* AllocateMemory(size_t size)
{
	return MemoryTracker::Allocate(size, AllocType_Malloc);
};

DECLSPEC_RESTRICT void* AllocateMemory(size_t size, AllocType type, const char* file, unsigned int line, const char* function)
{
	return MemoryTracker::Allocate(size, type, file, function, line);
};

void FreeMemory(void* memory)
{
	MemoryTracker::Free(memory, DeallocType_Free);
};

void FreeMemory(void* memory, DeallocType type, const char* file, unsigned int line, const char* function)
{
	MemoryTracker::Free(memory, type, file, function, line);
};

void* operator new(size_t size)
{
	MemoryTrackerThreadLocalInformation* info = MemoryTracker::GetThreadLocalInformation();
	return MemoryTracker::Allocate(size, AllocType_New, info->CurrentSourceFile, info->CurrentSourceFunction, info->CurrentSourceLine);
};

void* operator new(size_t size, const char* file, unsigned int line)
{
	return MemoryTracker::Allocate(size, AllocType_New, file, "(Unknown Function)", line);
};

void operator delete(void* memory)
{
	MemoryTrackerThreadLocalInformation* info = MemoryTracker::GetThreadLocalInformation();	
	MemoryTracker::Free(memory, DeallocType_Delete, info->CurrentSourceFile, info->CurrentSourceFunction, info->CurrentSourceLine);
};

void operator delete(void* memory, const char* file, unsigned int line)
{
	MemoryTracker::Free(memory, DeallocType_Delete, file, "(Unknown Function)", line);
};

void* operator new[](size_t size)
{
	MemoryTrackerThreadLocalInformation* info = MemoryTracker::GetThreadLocalInformation();
	return MemoryTracker::Allocate(size, AllocType_VectorNew, info->CurrentSourceFile, info->CurrentSourceFunction, info->CurrentSourceLine);
};

void* operator new[](size_t size, const char* file, unsigned int line)
{
	return MemoryTracker::Allocate(size, AllocType_VectorNew, file, "(Unknown Function)", line);
};

void operator delete[](void* memory)
{
	MemoryTrackerThreadLocalInformation* info = MemoryTracker::GetThreadLocalInformation();	
	MemoryTracker::Free(memory, DeallocType_VectorDelete, info->CurrentSourceFile, info->CurrentSourceFunction, info->CurrentSourceLine);
};

void operator delete[](void* memory, const char* file, unsigned int line)
{
	MemoryTracker::Free(memory, DeallocType_VectorDelete, file, "(Unknown Function)", line);
};

POP_MEMORY_MACROS;