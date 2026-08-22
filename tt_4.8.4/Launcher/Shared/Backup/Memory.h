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

SHARED_INLINE void *__cdecl operator new(size_t Size);
SHARED_INLINE  void *__cdecl operator new(size_t Size, void *Where);
SHARED_INLINE  void	__cdecl operator delete(void *mem);

SHARED_INLINE void *__cdecl operator new[](size_t Size);
SHARED_INLINE  void *__cdecl operator new[](size_t Size, void *Where);
SHARED_INLINE  void	__cdecl operator delete[](void *mem);
SHARED_INLINE void SetDefaultMemoryFunctions(void);

extern pfnAllocator		g_pAllocator_;
extern pfnFreeer		g_pFreeer_;
extern pfnReallocator	g_pReallocator_;
extern pfnMemSize		g_pMemSize_;
//Mem stats
#ifdef _DEBUG
typedef struct _MemStats {

	size_32	m_CurrentUsage;
	size_32	m_CurrentAllocs;

	size_32	m_AllocatedMemory;
	size_32 m_ReallocatedMemory;
	size_32 m_FreedMemory;

	size_32 m_NumAllocs;
	size_32	m_NumReallocs;
	size_32	m_NumFrees;

	size_32	m_NumNewAllocations;
	size_32	m_NumNew;
	size_32	m_NumDelete;
	size_32 m_NumANewAllocations;
	size_32	m_NumANew;
	size_32 m_NumADelete;

} MemStats, *lpMemStats;

extern MemStats	g_MemStats;

SHARED_INLINE void* SHARED_CALL g_pAllocator__(size_t size, int line, const char* pFunc, const char *pName);
SHARED_INLINE void SHARED_CALL g_pFreeer__(void* pMemory, int line, const char* pFunc, const char *pName);
SHARED_INLINE void* SHARED_CALL g_pReallocator__(void *pMem, size_t size, int line, const char* pFunc, const char *pName, const char *pName2);
SHARED_INLINE size_t SHARED_CALL g_pMemSize__(void *pMem, int line, const char* pFunc, const char *pName);


#define g_pAllocator(size)			g_pAllocator__((size), __LINE__, __FUNCTION__, #size)
#define g_pFreeer(pMemory)			g_pFreeer__((pMemory), __LINE__, __FUNCTION__, #pMemory)
#define g_pReallocator(pMem, size)	g_pReallocator((pMem), size, __LINE__, __FUNCTION__, #pMem, #size)
#define g_pMemSize(pMem)			g_pMemSize__((pMem), __LINE__, __FUNCTION__, #pMem)


void* FallThroughNew(void* pMem, size_32 size, int line, const char* pFunc, const char *pName);
void* FallThroughDelete(void* pMem, int line, const char* pFunc, const char *pName);
#define _NEW(t, p)						((t*)FallThroughNew((p), sizeof(t), __LINE__, __FUNCTION__, #t))
#define _DELETE(t, p)					delete (t*)FallThroughDelete(p, __LINE__, __FUNCTION__, #p)

#else //_DEBUG
#define	g_pAllocator	g_pAllocator_
#define	g_pFreeer		g_pFreeer_
#define	g_pReallocator	g_pReallocator_
#define	g_pMemSize		g_pMemSize_

#define NEW		new
#define DELETE	delete

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

#endif //_MEMORY_H_
