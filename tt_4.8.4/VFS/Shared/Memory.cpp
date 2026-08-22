#include <windows.h>
#include "Memory.h"

HANDLE	g_hHeap	= NULL;

SHARED_INLINE size_t SHARED_CALL MemSize(void *pMem) {
#ifdef USE_CRT_MEMORY
	return 0; //Not supported
#else //USE_CRT_MEMORY
	return (size_t)HeapSize(g_hHeap, 0, pMem);
#endif //USE_CRT_MEMORY
}

SHARED_INLINE void * SHARED_CALL Malloc(size_t size) {
#ifdef USE_CRT_MEMORY
	return malloc(size);
#else //USE_CRT_MEMORY
	return HeapAlloc(g_hHeap, 0, size);
#endif //USE_CRT_MEMORY
}

SHARED_INLINE void SHARED_CALL Free(void *pMem) {
#ifdef USE_CRT_MEMORY
	free(pMem);
#else //USE_CRT_MEMORY
	HeapFree(g_hHeap, 0, pMem);
#endif //USE_CRT_MEMORY
}

SHARED_INLINE void *SHARED_CALL ReAlloc(void *pMem, size_t size) {
#ifdef USE_CRT_MEMORY
	return realloc(pMem, size);
#else //USE_CRT_MEMORY
	return HeapReAlloc(g_hHeap, 0, pMem, size);
#endif //USE_CRT_MEMORY
}

SHARED_INLINE void SHARED_CALL InitMemory(void) {
#ifndef USE_CRT_MEMORY
	if (!(g_hHeap = GetProcessHeap()))
		g_hHeap = HeapCreate(0, 8192, 0);
#endif //USE_CRT_MEMORY
}

fnAllocator*	g_pAllocator;
fnFreeer*		g_pFreeer;
fnReallocator*	g_pReallocator;

SHARED_INLINE void *__cdecl operator new(size_t Size) {
	return g_pAllocator(Size);
}

SHARED_INLINE  void *__cdecl operator new(size_t Size, void *Where) {
	return Where;
}

SHARED_INLINE  void	__cdecl operator delete(void *mem) {
	g_pFreeer(mem);
}

SHARED_INLINE void *__cdecl operator new[](size_t Size) {
	return g_pAllocator(Size);
}

SHARED_INLINE  void *__cdecl operator new[](size_t Size, void *Where) {
	return Where;
}

SHARED_INLINE  void	__cdecl operator delete[](void *mem) {
	g_pFreeer(mem);
}

SHARED_INLINE void SetDefaultMemoryFunctions(void) {
	InitMemory();
	g_pAllocator = Malloc;
	g_pFreeer = Free;
	g_pReallocator = ReAlloc;
}

void Set_Memory_Functions(fnAllocator *pfnAllocator, fnFreeer *pfnFreeer, fnReallocator *pfnReallocator){
	g_pAllocator = pfnAllocator;
	g_pFreeer = pfnFreeer;
	g_pReallocator = pfnReallocator;
}

#ifdef LOG_POOL_DATA
int g_Allocated_Objects = 0;
#endif //LOG_POOL_DATA
