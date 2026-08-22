#include "Lists.h"
#include "Memory.h"
#include "StdLib.h"
#include "MultiCache.h"

CMultiCache::CMultiCache(){
	m_Count = 0;
}

CMultiCache::~CMultiCache(){
	FreeCaches();
}

void CMultiCache::FreeCaches(){
	GenericSLNode<CACHEENTRY>* pNode;
	CriticalSectionClass::LockClass lock(&m_CS);
	pNode = m_Caches.Get_Head();
	while (pNode){
		g_pFreeer(pNode->m_pData->m_pCache);
		g_pFreeer(pNode->m_pData);
		pNode = pNode->m_pNext;
	}
	m_Count = 0;

	m_Caches.Remove_All();
}

void* CMultiCache::Alloc(size_64 size){
	GenericSLNode<CACHEENTRY>* pNode;
	PCACHEENTRY pEntry;
	CriticalSectionClass::LockClass lock(&m_CS);
	size = ((size/MULTI_CACHE_ALIGN)+((size%MULTI_CACHE_ALIGN) ? 1 : 0))*MULTI_CACHE_ALIGN;
	if (size > MULTI_MAX_CACHE_SIZE)
		return NULL;
	
	pNode = m_Caches.Get_Head();
	while (pNode){
		if (pNode->m_pData->m_Free && pNode->m_pData->m_Size >= size){
			pNode->m_pData->m_Free = false;
			return pNode->m_pData->m_pCache;
		}
		pNode = pNode->m_pNext;
	}
	pEntry = (PCACHEENTRY)g_pAllocator(sizeof(CACHEENTRY));
	if (!pEntry) return NULL;
	pEntry->m_Free = false;
	pEntry->m_pCache = g_pAllocator((size_t)size);
	pEntry->m_Size = size;
	m_Caches.Add_Tail(pEntry);
	return pEntry->m_pCache;
}

void CMultiCache::Free(void *pMem){
	GenericSLNode<CACHEENTRY>* pNode;
	CriticalSectionClass::LockClass lock(&m_CS);
	pNode = m_Caches.Get_Head();
	while (pNode){
		if (pNode->m_pData->m_pCache == pMem){
			pNode->m_pData->m_Free = true;
			return;
		}
		pNode = pNode->m_pNext;
	}
}

