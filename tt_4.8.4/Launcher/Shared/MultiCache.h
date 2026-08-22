#ifndef _MULTICACHE_H_
#define _MULTICACHE_H_

#include "Lists.h"
#include "Memory.h"

#define MULTI_CACHE_ALIGN				(64*1024) //64k
#define MULTI_MAX_CACHE_SIZE			(5*1024*1024) //5m

typedef struct _CACHEENTRY {
	void*	m_pCache;
	size_64	m_Size;
	bool	m_Free;
	int		m_ID;
} CACHEENTRY, *PCACHEENTRY;

class CMultiCache {
private:
	SList<CACHEENTRY>		m_Caches;
	CriticalSectionClass	m_CS;
	int						m_Count;
public:
	CMultiCache();
	~CMultiCache();
	void FreeCaches();
	void* Alloc(size_64 size);
	void Free(void *pMem);
};

#endif //_MULTICACHE_H_
