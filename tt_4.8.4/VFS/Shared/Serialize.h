#ifndef _SERIALIZE_H_
#define _SERIALIZE_H_

#include <windows.h>
#include "Base.h"
#include "BaseClasses.h"
#include "Memory.h"
#include "Lists.h"

#define SERIALIZE_CACHE_SIZE	(1*1024*1024)

/*enum {
	SP_BUFFER	= 0,
	SP_SIZE32,
	SP_SIZE64,
	SP_HANDLE,
};

typedef struct DeserializeParam : AutoPoolClass<DeserializeParam, 8> {
	size_32	m_Type;

	union {
		void*		m_pData;
		size_32*	m_pSize32;
		size_64*	m_pSize64;
		HANDLE*		m_pHandle;
	};
};//*/

class CSerialize {
private:
	void*	m_pData;
	size_32	m_Offset;
public:
	CSerialize();
	~CSerialize();
	
	size_32 GetSize();
	void* GetBuffer();
	void Reset();

	void WriteSize32(size_32 val);
	void WriteSize64(size_64 val);
	void WriteHandle(HANDLE val);
	void WritePointer(ptr val);
	void WriteRaw(ptr val, size_32 size);
};

class CDeSerialize {
private:
	void*	m_pData;
	size_32	m_Offset;
public:
	CDeSerialize();
	~CDeSerialize();
	
	void* GetBuffer();
	void Reset();

	size_32 GetSize32();
	size_64 GetSize64();
	HANDLE GetHandle();
	ptr GetPointer();
	ptr GetRaw(size_32 size);
};

#endif //_SERIALIZE_H_
