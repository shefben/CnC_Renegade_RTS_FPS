#include <windows.h>
#include "VFS.h"
#include "VFSStructures.h"
#include "../Shared/Memory.h"
#include "../Shared/Lists.h"
#include "../Shared/BaseTypes.h"
#include "../Shared/StdLib.h"
#include "../Shared/MD5.h"
#include "API/VFSErrorcodes.h"
#include "API/VFSVersion.h"


CVFSIStream::CVFSIStream(IVFS* pVFS, VFSFileHandle hFile){
	m_pVFS = pVFS;
	m_hFile = hFile;
	m_RefCount = 1;
}

CVFSIStream::~CVFSIStream(){
	if (m_pVFS)
		m_pVFS->VFSCloseFile(m_hFile);
}

HRESULT STDMETHODCALLTYPE CVFSIStream::QueryInterface(REFIID riid, void **ppvObject){
	if (memcmp(&riid, &IID_IStream, sizeof(IID)) == 0 || memcmp(&riid, &IID_IUnknown, sizeof(IID)) == 0)
		*ppvObject = this;
	else{
		*ppvObject = NULL;
		return E_NOINTERFACE;
	}
	AddRef();
	return S_OK;
}

ULONG STDMETHODCALLTYPE CVFSIStream::AddRef(){
	return ++m_RefCount;
}

ULONG STDMETHODCALLTYPE CVFSIStream::Release(){
	ULONG RefCount;
	RefCount = m_RefCount;
	if ((--m_RefCount) == 0)
		delete this;

	return RefCount-1;
}


HRESULT STDMETHODCALLTYPE CVFSIStream::Read(void *pv, ULONG cb, ULONG *pcbRead){
	size_32 r;
	if (!pv || !m_pVFS || (m_hFile == VFS_INVALID_FILE)) return STG_E_INVALIDPOINTER;
	r = m_pVFS->VFSRead(m_hFile, pv, (size_32)cb, NULL);
	if (pcbRead) *pcbRead = (ULONG)r;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CVFSIStream::Write(const void *pv, ULONG cb, ULONG *pcbWritten){
	size_32 w;
	if (!pv || !m_pVFS || (m_hFile == VFS_INVALID_FILE)) return STG_E_INVALIDPOINTER;
	w = m_pVFS->VFSWrite(m_hFile, pv, (size_32)cb, NULL);
	if (pcbWritten) *pcbWritten = (ULONG)w;
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CVFSIStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition){
	size_64 NewOffset;
	if (!m_pVFS || (m_hFile == VFS_INVALID_FILE)) return STG_E_INVALIDPOINTER;
	switch (dwOrigin){
		case STREAM_SEEK_SET:
			if (dlibMove.HighPart != 0) return E_INVALIDARG;
			NewOffset = m_pVFS->VFSSeek(m_hFile, *(ssize_32*)&dlibMove.QuadPart, SEEK_SET);
			if (plibNewPosition) plibNewPosition->QuadPart = (ULONGLONG)NewOffset;
			return S_OK;
		case STREAM_SEEK_CUR:
			if (dlibMove.HighPart != 0 && dlibMove.HighPart != -1) return E_INVALIDARG;

			NewOffset = m_pVFS->VFSSeek(m_hFile, *(ssize_32*)&dlibMove.QuadPart, SEEK_CUR);

			if (plibNewPosition) plibNewPosition->QuadPart = (ULONGLONG)NewOffset;
			return S_OK;
		case STREAM_SEEK_END:
			if (dlibMove.HighPart != 0 && dlibMove.HighPart != -1) return E_INVALIDARG;
			NewOffset = m_pVFS->VFSSeek(m_hFile, *(ssize_32*)&dlibMove.QuadPart, SEEK_END);

			if (plibNewPosition) plibNewPosition->QuadPart = (ULONGLONG)NewOffset;
			return S_OK;
		default:
			return STG_E_INVALIDFUNCTION;
	}
}

HRESULT STDMETHODCALLTYPE CVFSIStream::SetSize(ULARGE_INTEGER libNewSize){
	size_64 offset;
	HRESULT hr;
	if (!m_pVFS || (m_hFile == VFS_INVALID_FILE)) return STG_E_INVALIDPOINTER;
	if (libNewSize.HighPart != 0) return E_INVALIDARG;
	offset = m_pVFS->VFSTell(m_hFile);
	m_pVFS->VFSSeek(m_hFile, (ssize_64)libNewSize.QuadPart, SEEK_SET);
	hr = m_pVFS->VFSSetEndOfFile(m_hFile) ? S_OK : E_FAIL;
	m_pVFS->VFSSeek(m_hFile, offset, SEEK_SET);
	return hr;
}

HRESULT STDMETHODCALLTYPE CVFSIStream::CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten){
	char lBuf[2048];
	ULONG r, w;
	if (pcbRead) pcbRead->QuadPart = 0;
	if (pcbWritten) pcbWritten->QuadPart = 0;

	if (!pstm) return STG_E_INVALIDPOINTER;
	if (!m_pVFS || (m_hFile == VFS_INVALID_FILE)) return STG_E_INVALIDPOINTER;
	while (cb.QuadPart > 0 && Read(lBuf, (ULONG)(cb.QuadPart > 2048 ? 2048 : cb.QuadPart), &r) == S_OK && r){
		cb.QuadPart -= r;
		if (pcbRead) pcbRead->QuadPart += r;

		if (Write(lBuf, r, &w) == S_OK && w){
			if (pcbWritten) pcbWritten->QuadPart += w;
		} else
			break;
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CVFSIStream::Commit(DWORD grfCommitFlags){
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CVFSIStream::Revert(){
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CVFSIStream::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType){
	return STG_E_INVALIDFUNCTION;
}

HRESULT STDMETHODCALLTYPE CVFSIStream::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType){
	return STG_E_INVALIDFUNCTION;
}

HRESULT STDMETHODCALLTYPE CVFSIStream::Stat(STATSTG *pstatstg, DWORD grfStatFlag){
	POPEN_FILE pFile;

	if (!m_pVFS || (m_hFile == VFS_INVALID_FILE)) return STG_E_INVALIDPOINTER;
	if (!pstatstg) return STG_E_INVALIDPOINTER;

	m_pVFS->VFSGetUIDFromFileHandle(m_hFile);

	if (m_pVFS->VFSGetUIDFromFileHandle(m_hFile) == VFS_INVALID_UID) return STG_E_INVALIDPOINTER;

	pFile = (POPEN_FILE)m_hFile;
	memset(pstatstg, 0, sizeof(STATSTG));
	pstatstg->cbSize.QuadPart = m_pVFS->VFSSize(m_hFile);
	

	pstatstg->grfMode = (pFile->m_Read_Access && pFile->m_Write_Access ? STGM_READWRITE : (pFile->m_Read_Access ? STGM_READ : (pFile->m_Write_Access ? STGM_WRITE : 0)));
	pstatstg->type = STGTY_STREAM;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CVFSIStream::Clone(IStream **ppstm){
	if (!ppstm) return STG_E_INVALIDPOINTER;
	*ppstm = NULL;
	return E_NOTIMPL;
}
