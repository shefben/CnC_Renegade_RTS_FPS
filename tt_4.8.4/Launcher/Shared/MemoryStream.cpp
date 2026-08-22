#include "MemoryStream.h"

#define PtrFromBlock(p) (((unsigned char*)(p))+(sizeof(void*)*2))
#define NextBlock(p)		(*(void**)(p))
#define PreviousBlock(p)	(*(void**)(((unsigned char*)(p))+sizeof(void*)))
#define AllocBlock(s)		(g_pAllocator((sizeof(void*)*2)+(s)))

void CMemoryBlockStream::FreeData(){

	void* pNext;
	void* pCur;

	pCur = m_pBlocks;

	while (pCur){
		pNext = NextBlock(pCur);
		g_pFreeer(pCur);
		pCur = pNext;
	}
		
	m_BlockSize = 0;
	m_NumBlocks = 0;
	m_Offset = 0;
	m_pBlocks = NULL;
	m_pCurrentBlock = NULL;
	m_pLastBlock = NULL;
	m_CurrentBlock = -1;
	m_Size = 0;
}

bool CMemoryBlockStream::Expand(size_32 size){
	size_32 neededblocks, temp;
	void *pLastBlock, *pBlocks, *pBlock, *pCur;

	temp = neededblocks = (size)/m_BlockSize+((size) % m_BlockSize ? 1 : 0);

	if (temp <= m_NumBlocks) return true;

	pLastBlock = GetLastBlock();

	if (!(pBlocks = pCur = pBlock = AllocBlock(m_BlockSize))) return false;

	NextBlock(pCur) = NULL;
	PreviousBlock(pCur) = NULL;

	if (pLastBlock){
		NextBlock(pLastBlock) = pBlocks;
		PreviousBlock(pBlocks) = pLastBlock;
	} else
		m_pBlocks = pBlocks;

	
	while (--neededblocks){
		if (!(pBlock = AllocBlock(m_BlockSize))) return false;
		NextBlock(pBlock) = NULL;
		PreviousBlock(pBlock) = pCur;
		NextBlock(pCur) = pBlock;
		pCur = pBlock;
	}

	m_pLastBlock = pCur;

	m_NumBlocks += temp;

	if (m_CurrentBlock == -1){
		m_CurrentBlock = 0;
		m_pCurrentBlock = pBlocks;
	}
	
	return true;
}

bool CMemoryBlockStream::Shrink(size_32 size){
	size_32 neededblocks, temp;

	void *pBlock, *pCur, *pNext;

	temp = neededblocks = (size)/m_BlockSize+((size) % m_BlockSize ? 1 : 0);


	if (neededblocks >= m_NumBlocks) return true;

	if (neededblocks == 0){
		pCur = m_pBlocks;

		while (pCur){
			pNext = NextBlock(pCur);
			g_pFreeer(pCur);
			pCur = pNext;
		}
		m_NumBlocks = 0;
		m_Offset = 0;
		m_pBlocks = NULL;
		m_pCurrentBlock = NULL;
		m_pLastBlock = NULL;
		m_CurrentBlock = -1;
		m_Size = 0;
		return true;
	}

	pBlock = PreviousBlock(pCur = BlockFromOffset(size, false));

	//pCur = NextBlock(pBlock);
	//NextBlock(pBlock) = NULL;

	while (pCur){
		pNext = NextBlock(pCur);
		g_pFreeer(pCur);
		pCur = pNext;
	}

	m_NumBlocks = neededblocks;
	if (m_Offset > size) m_Offset = size;

	m_pCurrentBlock = pBlock;
	m_CurrentBlock = neededblocks ? neededblocks - 1 : -1;
	m_pLastBlock = pBlock;

	return true;

}

void* CMemoryBlockStream::BlockFromOffset(size_32 offset, bool expand){
	size_32 blockindex;
	size_32 i;

	size_32 startdelta, enddelta;
	void* pCur;
	

	blockindex = offset/m_BlockSize;

	if (m_CurrentBlock == blockindex)
		return	m_pCurrentBlock;

	if (offset > m_Size && expand) Expand(offset);

	startdelta = m_CurrentBlock;
	enddelta = m_NumBlocks-m_CurrentBlock;

	if (blockindex >= m_NumBlocks) return NULL;

	if (blockindex == 0) return m_pBlocks;
	if (blockindex == m_NumBlocks-1) return m_pLastBlock;
	if (blockindex == m_CurrentBlock) return m_pCurrentBlock;

	if (blockindex < m_CurrentBlock){
		if (blockindex < (startdelta/2)){
			//Closest to start
			pCur = m_pBlocks;
			i = blockindex;
			goto __forward;
		} else {
			//Closest to -current block
			pCur = m_pCurrentBlock;
			i = startdelta - blockindex;
			goto __reverse;
		}
	} else {
		if (blockindex < (enddelta/2)){
			//Closest to current block
			pCur = m_pCurrentBlock;
			i = blockindex - startdelta;
			goto __forward;
		} else {
			//Closest to end
			pCur = m_pLastBlock;
			i = m_NumBlocks - blockindex - 1;
			goto __reverse;
		}
	}
__forward:
	if (!pCur) return NULL;
	while (i){
		i--;
		pCur = NextBlock(pCur);
	}

	goto __end;
__reverse:
	if (!pCur) return NULL;
	while (i){
		i--;
		pCur = PreviousBlock(pCur);
	}
__end:
	m_CurrentBlock = blockindex;
	m_pCurrentBlock = pCur;

	return m_pCurrentBlock;
}

void* CMemoryBlockStream::GetLastBlock(){
	return m_pLastBlock;
}

CMemoryBlockStream::CMemoryBlockStream(size_32 blocksize){
	m_RefCount = 1;
	m_BlockSize = blocksize;
	m_NumBlocks = 0;
	m_Offset = 0;
	m_pBlocks = NULL;
	m_pCurrentBlock = NULL;
	m_pLastBlock = NULL;
	m_CurrentBlock = -1;
	m_Size = 0;
}

CMemoryBlockStream::~CMemoryBlockStream(){
	FreeData();
}


HRESULT STDMETHODCALLTYPE CMemoryBlockStream::QueryInterface(REFIID riid, void **ppvObject){
	if (memcmp(&riid, &IID_IStream, sizeof(IID)) == 0 || memcmp(&riid, &IID_IUnknown, sizeof(IID)) == 0)
		*ppvObject = this;
	else{
		*ppvObject = NULL;
		return E_NOINTERFACE;
	}
	AddRef();
	return S_OK;
}

ULONG STDMETHODCALLTYPE CMemoryBlockStream::AddRef(){
	return (++m_RefCount);
}

ULONG STDMETHODCALLTYPE CMemoryBlockStream::Release(){

	if ((--m_RefCount == 0)){
		delete this;
		return 0;
	}

	return m_RefCount;
}


HRESULT STDMETHODCALLTYPE CMemoryBlockStream::Read(void *pv, ULONG cb, ULONG *pcbRead){
	CriticalSectionClass::LockClass lock(&m_CS);
	void* pBlock;
	size_32 left, o;
	if (pcbRead) *pcbRead = 0;

	if (cb == 0)
		return E_INVALIDARG;
	if (pv == NULL || m_pBlocks == NULL)
		return STG_E_INVALIDPOINTER;

	if (m_Offset > m_Size){
		if (pcbRead) *pcbRead = 0;
		return E_FAIL;
	}

	pBlock = BlockFromOffset(m_Offset, false);

	if (!pBlock) return E_FAIL;

	left = m_BlockSize - (m_Offset % m_BlockSize);

	o = 0;
	if ((cb + m_Offset) > m_Size)
		cb = m_Size-m_Offset;
	while (cb > 0){
		memcpy(((unsigned char*)pv)+o, PtrFromBlock(pBlock)+m_BlockSize-left, min(left, cb));
		o += min(left, cb);
		cb -= min(left, cb);

		if (cb == 0) break;
		left = m_BlockSize;

		if (!(pBlock = NextBlock(pBlock))) break;

	}

	if (pcbRead) *pcbRead = o;
	m_Offset += o;

	return (o > 0) ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE CMemoryBlockStream::Write(const void *pv, ULONG cb, ULONG *pcbWritten){
	CriticalSectionClass::LockClass lock(&m_CS);
	void* pBlock;
	size_32 left, o;
	if (pcbWritten) *pcbWritten = 0;

	if (cb == 0)
		return E_INVALIDARG;
	if (pv == NULL)
		return STG_E_INVALIDPOINTER;

	if (!Expand(m_Offset+cb)) return E_FAIL;
	


	pBlock = BlockFromOffset(m_Offset, false);

	if (!pBlock) return E_FAIL;

	left = m_BlockSize - (m_Offset % m_BlockSize);

	o = 0;
	while (cb > 0){
		memcpy(PtrFromBlock(pBlock)+m_BlockSize-left, ((unsigned char*)pv)+o, min(left, cb));
		o += min(left, cb);
		cb -= min(left, cb);

		if (cb == 0) break;
		left = m_BlockSize;

		if (!(pBlock = NextBlock(pBlock))) break;

	}
	if (m_Offset+o > m_Size)
		m_Size = m_Offset+o;

	if (pcbWritten) *pcbWritten = o;
	m_Offset += o;

	return (o > 0) ? S_OK : E_FAIL;
}


HRESULT STDMETHODCALLTYPE CMemoryBlockStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition){
	CriticalSectionClass::LockClass lock(&m_CS);
	if (plibNewPosition) plibNewPosition->QuadPart = 0;
	if (dlibMove.HighPart != 0) return STG_E_INVALIDPARAMETER;

	
	switch (dwOrigin){
		case STREAM_SEEK_SET:
			m_Offset = dlibMove.LowPart;
			break;
		case STREAM_SEEK_CUR:
			m_Offset += dlibMove.LowPart;
			break;
		case STREAM_SEEK_END:
			m_Offset = m_Size+dlibMove.LowPart;
			break;
		default:
			return STG_E_SEEKERROR;
	}

	if (plibNewPosition) plibNewPosition->LowPart = m_Offset;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CMemoryBlockStream::SetSize(ULARGE_INTEGER libNewSize){
	CriticalSectionClass::LockClass lock(&m_CS);
	HRESULT hr;
	if (libNewSize.HighPart != 0) return STG_E_INVALIDPARAMETER;

	if (libNewSize.LowPart == m_Size) return S_OK;
	
	hr = ((libNewSize.LowPart < m_Size) ? Shrink(libNewSize.LowPart) : Expand(libNewSize.LowPart)) ? S_OK : E_FAIL;
	if (hr == S_OK){
		m_Size = libNewSize.LowPart;
		if (m_Offset > m_Size)
			m_Offset = m_Size;
	}
	return hr;
}

HRESULT STDMETHODCALLTYPE CMemoryBlockStream::CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten){
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMemoryBlockStream::Commit(DWORD grfCommitFlags){
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMemoryBlockStream::Revert(){
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMemoryBlockStream::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType){
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMemoryBlockStream::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType){
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMemoryBlockStream::Stat(STATSTG *pstatstg, DWORD grfStatFlag){
	if (!pstatstg) E_POINTER;

	memset(pstatstg, 0, sizeof(STATSTG));
	pstatstg->cbSize.QuadPart = m_Size;
	pstatstg->grfMode = STGM_READWRITE;
	pstatstg->type = STGTY_STREAM;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CMemoryBlockStream::Clone(IStream **ppstm){
	return E_NOTIMPL;
}
