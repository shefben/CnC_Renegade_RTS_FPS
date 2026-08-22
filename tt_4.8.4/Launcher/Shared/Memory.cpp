#include <windows.h>
#include "Memory.h"

HANDLE	g_hHeap	= NULL;

#ifdef _DEBUG
MemStats	g_MemStats;

#ifndef USE_CRT_MEMORY
void* _cdecl malloc(size_t _Size){
	return g_pAllocator(_Size);
}

void _cdecl free(void *_Memory){
	g_pFreeer(_Memory);
}
#endif //!USE_CRT_MEMORY

SHARED_INLINE size_t SHARED_CALL MemSize(void *pMem) {
#ifdef USE_CRT_MEMORY
	return 0; //Not supported
#else //USE_CRT_MEMORY
	return (size_t)HeapSize(g_hHeap, 0, pMem);
#endif //USE_CRT_MEMORY
}

SHARED_INLINE void * SHARED_CALL Malloc(size_t size) {
	void *p;
	size_t t;
#ifdef USE_CRT_MEMORY
	p = malloc(size);
#else //USE_CRT_MEMORY
	p = HeapAlloc(g_hHeap, 0, size);
#endif //USE_CRT_MEMORY
	g_MemStats.m_AllocatedMemory += t = MemSize(p);
	g_MemStats.m_CurrentUsage += t;
	g_MemStats.m_NumAllocs++;
	g_MemStats.m_CurrentAllocs++;
	return p;
}

SHARED_INLINE void SHARED_CALL Free(void *pMem) {
	size_t t;
	g_MemStats.m_FreedMemory += t = MemSize(pMem);
	g_MemStats.m_CurrentUsage -= t;
	g_MemStats.m_NumFrees++;
	g_MemStats.m_CurrentAllocs--;
#ifdef USE_CRT_MEMORY
	free(pMem);
#else //USE_CRT_MEMORY
	HeapFree(g_hHeap, 0, pMem);
#endif //USE_CRT_MEMORY
}


SHARED_INLINE void *SHARED_CALL ReAlloc(void *pMem, size_t size) {
	void *p;
	size_t t;
	g_MemStats.m_CurrentUsage -= MemSize(pMem);
#ifdef USE_CRT_MEMORY
	p = realloc(pMem, size);
#else //USE_CRT_MEMORY
	p = HeapReAlloc(g_hHeap, 0, pMem, size);
#endif //USE_CRT_MEMORY
	
	g_MemStats.m_ReallocatedMemory += t = MemSize(p);
	g_MemStats.m_CurrentUsage += t;
	g_MemStats.m_NumReallocs++;
	return p;
}



void *__cdecl operator new(size_t Size) {
	g_MemStats.m_NumNew++;
	g_MemStats.m_NumNewAllocations++;
	return g_pAllocator(Size);
}

 void *__cdecl operator new(size_t Size, void *Where) {
	return Where;
}

 void	__cdecl operator delete(void *mem) {
	g_MemStats.m_NumDelete++;
	g_MemStats.m_NumNewAllocations--;
	g_pFreeer(mem);
}

void *__cdecl operator new[](size_t Size) {
	g_MemStats.m_NumANew++;
	g_MemStats.m_NumANewAllocations++;
	return g_pAllocator(Size);
}

 void *__cdecl operator new[](size_t Size, void *Where) {
	return Where;
}

 void	__cdecl operator delete[](void *mem) {
	g_MemStats.m_NumADelete++;
	g_MemStats.m_NumANewAllocations--;
	g_pFreeer(mem);
}

#else //_DEBUG


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

void *__cdecl operator new(size_t Size) {
	return g_pAllocator(Size);
}

 void *__cdecl operator new(size_t Size, void *Where) {
	return Where;
}

 void	__cdecl operator delete(void *mem) {
	g_pFreeer(mem);
}

void *__cdecl operator new[](size_t Size) {
	return g_pAllocator(Size);
}

 void *__cdecl operator new[](size_t Size, void *Where) {
	return Where;
}

 void	__cdecl operator delete[](void *mem) {
	g_pFreeer(mem);
}
#endif //!_DEBUG


SHARED_INLINE size_t SHARED_CALL Init_MemSize(void *pMem) {
#ifndef USE_CRT_MEMORY
	g_hHeap = GetProcessHeap();
#endif //USE_CRT_MEMORY
	g_pAllocator = Malloc;
	g_pFreeer = Free;
	g_pReallocator = ReAlloc;
	g_pMemSize = MemSize;
	return g_pMemSize(pMem);
}

SHARED_INLINE void * SHARED_CALL Init_Malloc(size_t size) {
#ifndef USE_CRT_MEMORY
	g_hHeap = GetProcessHeap();
#endif //USE_CRT_MEMORY
	g_pAllocator = Malloc;
	g_pFreeer = Free;
	g_pReallocator = ReAlloc;
	g_pMemSize = MemSize;
	return g_pAllocator(size);
}

SHARED_INLINE void SHARED_CALL Init_Free(void *pMem) {
#ifndef USE_CRT_MEMORY
	g_hHeap = GetProcessHeap();
#endif //USE_CRT_MEMORY
	g_pAllocator = Malloc;
	g_pFreeer = Free;
	g_pReallocator = ReAlloc;
	g_pMemSize = MemSize;
	return g_pFreeer(pMem);
}


SHARED_INLINE void *SHARED_CALL Init_ReAlloc(void *pMem, size_t size) {
#ifndef USE_CRT_MEMORY
	g_hHeap = GetProcessHeap();
#endif //USE_CRT_MEMORY
	g_pAllocator = Malloc;
	g_pFreeer = Free;
	g_pReallocator = ReAlloc;
	g_pMemSize = MemSize;
	return g_pReallocator(pMem, size);
}


SHARED_INLINE void SHARED_CALL InitMemory(void) {
#ifndef USE_CRT_MEMORY
	if (!(g_hHeap = GetProcessHeap()))
		g_hHeap = HeapCreate(0, 8192, 0);
#endif //USE_CRT_MEMORY
}

pfnAllocator	g_pAllocator	= Init_Malloc;
pfnFreeer		g_pFreeer		= Init_Free;
pfnReallocator	g_pReallocator	= Init_ReAlloc;
pfnMemSize		g_pMemSize		= Init_MemSize;

SHARED_INLINE void SetDefaultMemoryFunctions(void) {
	InitMemory();
	g_pAllocator = Malloc;
	g_pFreeer = Free;
	g_pReallocator = ReAlloc;
}

void SetMemoryFunctions(pfnAllocator _pfnAllocator, pfnFreeer _pfnFreeer, pfnReallocator _pfnReallocator){
	g_pAllocator = _pfnAllocator;
	g_pFreeer = _pfnFreeer;
	g_pReallocator = _pfnReallocator;
}

#ifdef LOG_POOL_DATA
int g_Allocated_Objects = 0;
#endif //LOG_POOL_DATA


void *SHARED_CDECL CMemClearOperators::operator new(size_t size){
	void *pMem;
	pMem = g_pAllocator(size);
	if (pMem)
		memset(pMem, 0, size);
	return pMem;
}

void SHARED_CDECL CMemClearOperators::operator delete(void *p){
	g_pFreeer(p);
}

#ifdef LOG_POOL_DATA
extern int g_Allocated_SizeObjects;
#endif //LOG_POOL_DATA

