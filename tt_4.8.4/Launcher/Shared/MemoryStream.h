#ifndef _MEMORYSTREAM_H_
#define _MEMORYSTREAM_H_

#include <windows.h>
#include "memory.h"
#include "BaseClasses.h"

class CMemoryBlockStream : public IStream {
private:
	ULONG					m_RefCount;
	size_32					m_BlockSize;
	size_32					m_Size;
	size_32					m_NumBlocks;
	size_32					m_Offset;
	void*					m_pBlocks;
	void*					m_pLastBlock;
	void*					m_pCurrentBlock;
	size_32					m_CurrentBlock;
	CriticalSectionClass	m_CS;
	void FreeData();
	void* BlockFromOffset(size_32 offset, bool expand);
	bool Expand(size_32 size);
	bool Shrink(size_32 size);
	void* GetLastBlock();
public:
	CMemoryBlockStream(size_32 blocksize);
	~CMemoryBlockStream();

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
	virtual ULONG STDMETHODCALLTYPE AddRef();
	virtual ULONG STDMETHODCALLTYPE Release();

	virtual HRESULT STDMETHODCALLTYPE Read(void *pv, ULONG cb, ULONG *pcbRead);
	virtual HRESULT STDMETHODCALLTYPE Write(const void *pv, ULONG cb, ULONG *pcbWritten);

	virtual HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
	virtual HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER libNewSize);
	virtual HRESULT STDMETHODCALLTYPE CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
	virtual HRESULT STDMETHODCALLTYPE Commit(DWORD grfCommitFlags);
	virtual HRESULT STDMETHODCALLTYPE Revert();
	virtual HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
	virtual HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
	virtual HRESULT STDMETHODCALLTYPE Stat(STATSTG *pstatstg, DWORD grfStatFlag);
	virtual HRESULT STDMETHODCALLTYPE Clone(IStream **ppstm);
};

#endif //!_MEMORYSTREAM_H_
