#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <windows.h>
#include "Base.h"
#include <stdlib.h>
#include "BaseClasses.h"
#include "StdLib.h"

extern HANDLE	g_hHeap;

//Enable this for logging objectpool info to Debugoutput
//#define LOG_POOL_DATA
#define LOG_POOL_DTOR_ONLY

#ifdef LOG_POOL_DATA
#define LOG_OBJECT_COUNT(count, gcount) LogFormated(L"%S -> %d/%d\n", __FUNCTION__, count, gcount)
#endif //LOG_POOL_DATA



SHARED_INLINE size_t SHARED_CALL MemSize(void *pMem);
SHARED_INLINE void * SHARED_CALL Malloc(size_t size);
SHARED_INLINE void SHARED_CALL Free(void *pMem);
SHARED_INLINE void *SHARED_CALL ReAlloc(void *pMem, size_t size);
SHARED_INLINE void SHARED_CALL InitMemory(void);

typedef void*	(SHARED_CALL *pfnAllocator)(size_t size);
typedef void	(SHARED_CALL *pfnFreeer)(void* pMemory);
typedef void*	(SHARED_CALL *pfnReallocator)(void *pMem, size_t size);
typedef size_t	(SHARED_CALL *pfnMemSize)(void *pMem);

void SetMemoryFunctions(pfnAllocator _pfnAllocator, pfnFreeer _pfnFreeer, pfnReallocator _pfnReallocator);

void *__cdecl operator new(size_t Size);
void *__cdecl operator new(size_t Size, void *Where);
void __cdecl operator delete(void *mem);

void *__cdecl operator new[](size_t Size);
void *__cdecl operator new[](size_t Size, void *Where);
void __cdecl operator delete[](void *mem);
void SetDefaultMemoryFunctions(void);

#ifndef USE_CRT_MEMORY
void* _cdecl malloc(size_t _Size);
void _cdecl free(void *_Memory);
#endif //!USE_CRT_MEMORY

extern pfnAllocator		g_pAllocator;
extern pfnFreeer		g_pFreeer;
extern pfnReallocator	g_pReallocator;
extern pfnMemSize		g_pMemSize;
//Mem stats
#ifdef _DEBUG
typedef struct _MemStats {

	size_t	m_CurrentUsage;
	size_t	m_CurrentAllocs;

	size_t	m_AllocatedMemory;
	size_t m_ReallocatedMemory;
	size_t m_FreedMemory;

	size_t m_NumAllocs;
	size_t	m_NumReallocs;
	size_t	m_NumFrees;

	size_t	m_NumNewAllocations;
	size_t	m_NumNew;
	size_t	m_NumDelete;
	size_t m_NumANewAllocations;
	size_t	m_NumANew;
	size_t m_NumADelete;

} MemStats, *lpMemStats;

extern MemStats	g_MemStats;



#else //_DEBUG

#endif //!_DEBUG

/*
Memory pool
*/

#ifdef LOG_POOL_DATA
extern int g_Allocated_Objects;
#endif //LOG_POOL_DATA

template<typename T, int nAlign> class ObjectPoolClass {
private:
	unsigned char*			m_Pool_Nodes;
	unsigned char*			m_Pool_List;
	int						m_nFree_Nodes;
	int						m_nTotal_Nodes;
	CriticalSectionClass	m_Critical_Section;
public:
	ObjectPoolClass(void);
	~ObjectPoolClass(void);
	T *Allocate_Object_Memory(void);
	void Free_Object_Memory(T *object);


#ifdef LOG_POOL_DATA
	static void Log_Memory();
	static int	s_Object_Count;
#endif //LOG_POOL_DATA
};

template<typename T, int nAlign> SHARED_INLINE ObjectPoolClass<T, nAlign>::ObjectPoolClass(void) {
	m_Pool_Nodes = m_Pool_List = NULL;
	m_nFree_Nodes = m_nTotal_Nodes = 0;
}
template<typename T, int nAlign> SHARED_INLINE ObjectPoolClass<T, nAlign>::~ObjectPoolClass(void) {
	void *current = m_Pool_List, *temp;

#ifdef LOG_POOL_DATA
	LOG_OBJECT_COUNT(s_Object_Count, g_Allocated_Objects);
	g_Allocated_Objects -= m_nTotal_Nodes-m_nFree_Nodes;
	s_Object_Count -= m_nTotal_Nodes-m_nFree_Nodes;
#endif //LOG_POOL_DATA
	while (current){
		temp = *(unsigned char**)current;
		g_pFreeer(current);
		current = temp;
	}
}

template<typename T, int nAlign> T *ObjectPoolClass<T, nAlign>::Allocate_Object_Memory(void) {
	CriticalSectionClass::LockClass local_Lock(&m_Critical_Section);
	T *temp;
	unsigned char *ptr;

	if (!m_Pool_Nodes){
		ptr = (unsigned char*)g_pAllocator((max(sizeof(T), sizeof(void*))*nAlign)+sizeof(void*));
		*(void**)ptr = m_Pool_List;
		m_Pool_List = ptr;
		ptr += sizeof(void*);
		m_Pool_Nodes = ptr;

		for (int i = 0; i < nAlign; i++)
			*(unsigned char**)(m_Pool_Nodes+(i*max(sizeof(T), sizeof(void*)))) = (unsigned char*)(m_Pool_Nodes+max(sizeof(T), sizeof(void*))+(i*max(sizeof(T), sizeof(void*))));

		m_nFree_Nodes += nAlign;
		m_nTotal_Nodes += nAlign;
		*(void**)(((unsigned char*)m_Pool_Nodes+(nAlign*max(sizeof(T), sizeof(void*)))-max(sizeof(T), sizeof(void*)))) = NULL;
	}
	temp = (T*)m_Pool_Nodes;
	m_Pool_Nodes = *(unsigned char**)m_Pool_Nodes;
	m_nFree_Nodes--;
#ifdef LOG_POOL_DATA
	g_Allocated_Objects++;
	s_Object_Count++;
#ifndef LOG_POOL_DTOR_ONLY
	LogFormated(L"%d of %d used in %S\n", m_nTotal_Nodes-m_nFree_Nodes, m_nTotal_Nodes, __FUNCTION__);
#endif //LOG_POOL_DTOR_ONLY
#endif //LOG_POOL_DATA
	return temp;
}
template<typename T, int nAlign> void ObjectPoolClass<T, nAlign>::Free_Object_Memory(T *object) {
	CriticalSectionClass::LockClass local_Lock(&m_Critical_Section);

	void *current = m_Pool_List;

	*(unsigned char**)object = m_Pool_Nodes;
	m_Pool_Nodes = (unsigned char*)object;
	m_nFree_Nodes++;
#ifdef LOG_POOL_DATA
	g_Allocated_Objects--;
	s_Object_Count--;
#ifndef LOG_POOL_DTOR_ONLY
	LogFormated(L"%d of %d used in %S\n", m_nTotal_Nodes-m_nFree_Nodes, m_nTotal_Nodes, __FUNCTION__);
#endif //LOG_POOL_DTOR_ONLY
#endif //LOG_POOL_DATA
}

#ifdef LOG_POOL_DATA
template<typename T, int nAlign> int ObjectPoolClass<T, nAlign>::s_Object_Count = 0;
#endif //LOG_POOL_DATA


template<typename T, int nAlign = 256> class AutoPoolClass {
public:
	static ObjectPoolClass<T, nAlign> Allocator;
	void *SHARED_CDECL operator new(size_t size);
	void SHARED_CDECL operator delete(void *p);
};
template<typename T, int nAlign> ObjectPoolClass<T, nAlign> AutoPoolClass<T, nAlign>::Allocator;


template<typename T, int nAlign> SHARED_INLINE void *SHARED_CDECL AutoPoolClass<T, nAlign>::operator new(size_t size) {
	return Allocator.Allocate_Object_Memory();
}
template<typename T, int nAlign> SHARED_INLINE void SHARED_CDECL AutoPoolClass<T, nAlign>::operator delete(void *p) {
	Allocator.Free_Object_Memory((T*)p);
}

//Expands as required
template<typename T, int Align = 16> class DynPtrArray {
private:
	T**	m_ppArray;
	int	m_Size;
public:
	DynPtrArray();
	~DynPtrArray();

	bool Expand(int NewSize);
	T *Get(int Index);
	void Set(int Index, T*pItem);

	T *operator[](int Index);
};

template<typename T, int Align> DynPtrArray<T, Align>::DynPtrArray(){
	m_ppArray = NULL;
	m_Size = 0;
}

template <typename T, int Align> DynPtrArray<T, Align>::~DynPtrArray(){
	if (m_ppArray){
		g_pFreeer(m_ppArray);
		m_ppArray = NULL;
		m_Size = 0;
	}
}

template <typename T, int Align> bool DynPtrArray<T, Align>::Expand(int NewSize){
	T**ppTemp;
	if (NewSize == 0){
		if (m_ppArray){
			m_Size = NULL;
			g_pFreeer(m_ppArray);
			m_ppArray = NULL;
			m_Size = 0;
		}
		return true;
	}
	if (m_ppArray && NewSize <= m_Size) return true;
	if (!(ppTemp = (T**)g_pAllocator(sizeof(T*)*NewSize))) return false;
	memset(ppTemp, 0, sizeof(T*)*NewSize);
	if (m_ppArray){
		memcpy(ppTemp, m_ppArray, m_Size*sizeof(T*));
		g_pFreeer(m_ppArray);
	}
	m_ppArray = ppTemp;
	m_Size = NewSize;
	return true;
}

template <typename T, int Align> T* DynPtrArray<T, Align>::Get(int Index){
	return (Index >= 0 && Index < m_Size && m_ppArray) ? m_ppArray[Index] : NULL;
}

template <typename T, int Align> void DynPtrArray<T, Align>::Set(int Index, T*pItem){
	if (!Expand(Index)) return
	if (Index >= 0 && Index < m_Size)
		m_ppArray[Index] = pItem;
}

template <typename T, int Align> T* DynPtrArray<T, Align>::operator[](int Index){
	return (Index >= 0 && Index < m_Size && m_ppArray) ? m_ppArray[Index] : NULL;
}//*/

class CMemClearOperators {
public:
	void *SHARED_CDECL operator new(size_t size);
	void SHARED_CDECL operator delete(void *p);
};

#ifdef LOG_POOL_DATA
extern int g_Allocated_SizeObjects;
#endif //LOG_POOL_DATA

template<int size, int nAlign> class SizedPoolClass {
private:
	unsigned char*			m_Pool_Nodes;
	unsigned char*			m_Pool_List;
	int						m_nFree_Nodes;
	int						m_nTotal_Nodes;
	CriticalSectionClass	m_Critical_Section;
public:
	SizedPoolClass(void);
	~SizedPoolClass(void);
	void *Allocate_Object_Memory(void);
	void Free_Object_Memory(void *object);


#ifdef LOG_POOL_DATA
	static void Log_Memory();
	static int	s_Object_Count;
#endif //LOG_POOL_DATA
};

template<int size, int nAlign> SHARED_INLINE SizedPoolClass<size, nAlign>::SizedPoolClass(void) {
	m_Pool_Nodes = m_Pool_List = NULL;
	m_nFree_Nodes = m_nTotal_Nodes = 0;
}
template<int size, int nAlign> SHARED_INLINE SizedPoolClass<size, nAlign>::~SizedPoolClass(void) {
	void *current = m_Pool_List, *temp;

#ifdef LOG_POOL_DATA
	LOG_OBJECT_COUNT(s_Object_Count, g_Allocated_Objects);
	g_Allocated_SizeObjects -= m_nTotal_Nodes-m_nFree_Nodes;
	s_Object_Count -= m_nTotal_Nodes-m_nFree_Nodes;
#endif //LOG_POOL_DATA
	while (current){
		temp = *(unsigned char**)current;
		g_pFreeer(current);
		current = temp;
	}
}

template<int size, int nAlign> void *SizedPoolClass<size, nAlign>::Allocate_Object_Memory(void) {
	CriticalSectionClass::LockClass local_Lock(&m_Critical_Section);
	void *temp;
	unsigned char *ptr;

	if (!m_Pool_Nodes){
		ptr = (unsigned char*)g_pAllocator((max(size, sizeof(void*))*nAlign)+sizeof(void*));
		*(void**)ptr = m_Pool_List;
		m_Pool_List = ptr;
		ptr += sizeof(void*);
		m_Pool_Nodes = ptr;

		for (int i = 0; i < nAlign; i++)
			*(unsigned char**)(m_Pool_Nodes+(i*max(size, sizeof(void*)))) = (unsigned char*)(m_Pool_Nodes+max(size, sizeof(void*))+(i*max(size, sizeof(void*))));

		m_nFree_Nodes += nAlign;
		m_nTotal_Nodes += nAlign;
		*(void**)(((unsigned char*)m_Pool_Nodes+(nAlign*max(size, sizeof(void*)))-max(size, sizeof(void*)))) = NULL;
	}
	temp = (void*)m_Pool_Nodes;
	m_Pool_Nodes = *(unsigned char**)m_Pool_Nodes;
	m_nFree_Nodes--;
#ifdef LOG_POOL_DATA
	g_Allocated_SizeObjects++;
	s_Object_Count++;
#ifndef LOG_POOL_DTOR_ONLY
	LogFormated(L"%d of %d used in %S\n", m_nTotal_Nodes-m_nFree_Nodes, m_nTotal_Nodes, __FUNCTION__);
#endif //LOG_POOL_DTOR_ONLY
#endif //LOG_POOL_DATA
	return temp;
}
template<int size, int nAlign> void SizedPoolClass<size, nAlign>::Free_Object_Memory(void *object) {
	CriticalSectionClass::LockClass local_Lock(&m_Critical_Section);

	void *current = m_Pool_List;

	*(unsigned char**)object = m_Pool_Nodes;
	m_Pool_Nodes = (unsigned char*)object;
	m_nFree_Nodes++;
#ifdef LOG_POOL_DATA
	g_Allocated_SizeObjects--;
	s_Object_Count--;
#ifndef LOG_POOL_DTOR_ONLY
	LogFormated(L"%d of %d used in %S\n", m_nTotal_Nodes-m_nFree_Nodes, m_nTotal_Nodes, __FUNCTION__);
#endif //LOG_POOL_DTOR_ONLY
#endif //LOG_POOL_DATA
}

#ifdef LOG_POOL_DATA
template<int size, int nAlign> int SizedPoolClass<size, nAlign>::s_Object_Count = 0;
#endif //LOG_POOL_DATA



#endif //_MEMORY_H_
