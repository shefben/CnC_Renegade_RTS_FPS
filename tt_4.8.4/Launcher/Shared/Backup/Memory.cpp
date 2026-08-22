#include <windows.h>
#include "Memory.h"

HANDLE	g_hHeap	= NULL;

#ifdef _DEBUG
MemStats	g_MemStats;

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




SHARED_INLINE void* SHARED_CALL g_pAllocator__(size_t size, int line, const char* pFunc, const char *pName){
	LogFormated(L"g_pAllocator(%u, %i, %S, %S);\n", size, line, pFunc, pName);
	return g_pAllocator_(size);
}

SHARED_INLINE void SHARED_CALL g_pFreeer__(void* pMemory, int line, const char* pFunc, const char *pName){
	LogFormated(L"g_pFreeer(%p, %i, %S, %S);\n", pMemory, line, pFunc, pName);
	return g_pFreeer_(pMemory);
}

SHARED_INLINE void* SHARED_CALL g_pReallocator__(void *pMem, size_t size, int line, const char* pFunc, const char *pName, const char *pName2){
	LogFormated(L"g_pReallocator(%p, %u, %i, %S, %S, %S);\n", pMem, size, line, pFunc, pName, pName2);
	return g_pReallocator_(pMem, size);
}

SHARED_INLINE size_t SHARED_CALL g_pMemSize__(void *pMem, int line, const char* pFunc, const char *pName){
	LogFormated(L"g_pMemSize(%p, %i, %S, %S);\n", pMem, line, pFunc, pName);
	return g_pMemSize_(pMem);
}

void* FallThroughNew(void* pMem, size_32 size, int line, const char* pFunc, const char *pName){
	LogFormated(L"FallThroughNew(%p, %u, %i, %S, %S);\n", pMem, size, line, pFunc, pName);
	return pMem;
}

void* FallThroughDelete(void* pMem, int line, const char* pFunc, const char *pName){
	LogFormated(L"FallThroughDelete(%p, %i, %S, %S);\n", pMem, line, pFunc, pName);
	return pMem;
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
#endif //!_DEBUG


SHARED_INLINE size_t SHARED_CALL Init_MemSize(void *pMem) {
#ifndef USE_CRT_MEMORY
	g_hHeap = GetProcessHeap();
#endif //USE_CRT_MEMORY
	g_pAllocator_ = Malloc;
	g_pFreeer_ = Free;
	g_pReallocator_ = ReAlloc;
	g_pMemSize_ = MemSize;
	return g_pMemSize_(pMem);
}

SHARED_INLINE void * SHARED_CALL Init_Malloc(size_t size) {
#ifndef USE_CRT_MEMORY
	g_hHeap = GetProcessHeap();
#endif //USE_CRT_MEMORY
	g_pAllocator_ = Malloc;
	g_pFreeer_ = Free;
	g_pReallocator_ = ReAlloc;
	g_pMemSize_ = MemSize;
	return g_pAllocator_(size);
}

SHARED_INLINE void SHARED_CALL Init_Free(void *pMem) {
#ifndef USE_CRT_MEMORY
	g_hHeap = GetProcessHeap();
#endif //USE_CRT_MEMORY
	g_pAllocator_ = Malloc;
	g_pFreeer_ = Free;
	g_pReallocator_ = ReAlloc;
	g_pMemSize_ = MemSize;
	return g_pFreeer_(pMem);
}


SHARED_INLINE void *SHARED_CALL Init_ReAlloc(void *pMem, size_t size) {
#ifndef USE_CRT_MEMORY
	g_hHeap = GetProcessHeap();
#endif //USE_CRT_MEMORY
	g_pAllocator_ = Malloc;
	g_pFreeer_ = Free;
	g_pReallocator_ = ReAlloc;
	g_pMemSize_ = MemSize;
	return g_pReallocator_(pMem, size);
}


SHARED_INLINE void SHARED_CALL InitMemory(void) {
#ifndef USE_CRT_MEMORY
	if (!(g_hHeap = GetProcessHeap()))
		g_hHeap = HeapCreate(0, 8192, 0);
#endif //USE_CRT_MEMORY
}

pfnAllocator	g_pAllocator_	= Init_Malloc;
pfnFreeer		g_pFreeer_		= Init_Free;
pfnReallocator	g_pReallocator_	= Init_ReAlloc;
pfnMemSize		g_pMemSize_		= Init_MemSize;

SHARED_INLINE void *__cdecl operator new(size_t Size) {
	g_MemStats.m_NumNew;
	g_MemStats.m_NumNewAllocations++;
	return g_pAllocator_(Size);
}

SHARED_INLINE  void *__cdecl operator new(size_t Size, void *Where) {
	return Where;
}

SHARED_INLINE  void	__cdecl operator delete(void *mem) {
	g_MemStats.m_NumDelete++;
	g_MemStats.m_NumNewAllocations--;
	g_pFreeer_(mem);
}

SHARED_INLINE void *__cdecl operator new[](size_t Size) {
	g_MemStats.m_NumANew++;
	g_MemStats.m_NumANewAllocations++;
	return g_pAllocator_(Size);
}

SHARED_INLINE  void *__cdecl operator new[](size_t Size, void *Where) {
	return Where;
}

SHARED_INLINE  void	__cdecl operator delete[](void *mem) {
	g_MemStats.m_NumADelete++;
	g_MemStats.m_NumANewAllocations--;
	g_pFreeer_(mem);
}

SHARED_INLINE void SetDefaultMemoryFunctions(void) {
	InitMemory();
	g_pAllocator_ = Malloc;
	g_pFreeer_ = Free;
	g_pReallocator_ = ReAlloc;
}

void SetMemoryFunctions(pfnAllocator _pfnAllocator, pfnFreeer _pfnFreeer, pfnReallocator _pfnReallocator){
	g_pAllocator_ = _pfnAllocator;
	g_pFreeer_ = _pfnFreeer;
	g_pReallocator_ = _pfnReallocator;
}

#ifdef LOG_POOL_DATA
int g_Allocated_Objects = 0;
#endif //LOG_POOL_DATA


void *SHARED_CDECL CMemClearOperators::operator new(size_t size){
	void *pMem;
	pMem = g_pAllocator_(size);
	if (pMem)
		memset(pMem, 0, size);
	return pMem;
}

void SHARED_CDECL CMemClearOperators::operator delete(void *p){
	g_pFreeer_(p);
}

