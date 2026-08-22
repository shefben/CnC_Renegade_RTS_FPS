#include <windows.h>
#include "FileStream.h"
#include "Memory.h"

CFileStream::CFileStream(){
	m_hFile = INVALID_FILE;
	m_RefCount = 1;
	m_pFileName = NULL;
	m_ReadOnly = false;
	m_Delete = false;
}

CFileStream::~CFileStream(){
	Close(false);
}

bool CFileStream::Open(const wchar_t *pFileName, bool Delete, bool ReadOnly){
	wchar_t lBuf[512];
	wchar_t TempName[512];

	Close(false);

	if (ReadOnly && !pFileName) return false;
	if (Delete && pFileName) return false;

	if (!pFileName){
		GetTempPathW(512, lBuf);
		GetTempFileNameW(lBuf, L"FST", 'FST', TempName);
		
		//GetTempFileNameW(L"C:\\Launcher Content", L"FSTRM", 0, TempName);

		m_pFileName = StrDupW(TempName);
	} else
		m_pFileName = StrDupW(pFileName);

	if (!m_pFileName) return false;

	m_ReadOnly = ReadOnly;
	m_Delete = Delete;

	if (!MakeDirFromFilenameW(m_pFileName, NULL, NULL)){
		g_pFreeer(m_pFileName);
		m_pFileName = NULL;
		return false;
	}

	m_hFile = fopen_W(m_pFileName, ReadOnly ? L"r" : L"w+");
	if (m_hFile == INVALID_FILE){
		g_pFreeer(m_pFileName);
		m_pFileName = NULL;
		return false;
	}

	return true;

}

void CFileStream::Close(bool _delete){
	if (m_hFile == INVALID_FILE) return;
	fclose_(m_hFile);
	if (!m_ReadOnly && (m_Delete || _delete))
		DeleteFileW(m_pFileName);

	g_pFreeer(m_pFileName);
	m_pFileName = NULL;
	m_hFile = INVALID_FILE;
}


HRESULT STDMETHODCALLTYPE CFileStream::QueryInterface(REFIID riid, void **ppvObject){
	if (memcmp(&riid, &IID_IStream, sizeof(IID)) == 0 || memcmp(&riid, &IID_IUnknown, sizeof(IID)) == 0)
		*ppvObject = this;
	else{
		*ppvObject = NULL;
		return E_NOINTERFACE;
	}
	AddRef();
	return S_OK;
}

ULONG STDMETHODCALLTYPE CFileStream::AddRef(){
	return ++m_RefCount;
}

ULONG STDMETHODCALLTYPE CFileStream::Release(){
	if ((--m_RefCount == 0)){
		delete this;
		return 0;
	}

	return m_RefCount;
}


HRESULT STDMETHODCALLTYPE CFileStream::Read(void *pv, ULONG cb, ULONG *pcbRead){
	CriticalSectionClass::LockClass lock(&m_CS);
	size_32 r;
	if (pcbRead) *pcbRead = 0;
	if (m_hFile == INVALID_FILE) return STG_E_INVALIDHANDLE;
	if (!pv) return STG_E_INVALIDPOINTER;
	if (!cb) return E_INVALIDARG;

	r = fread_(pv, cb, m_hFile);

	if (pcbRead) *pcbRead = r;

	return (r > 0) ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE CFileStream::Write(const void *pv, ULONG cb, ULONG *pcbWritten){
	CriticalSectionClass::LockClass lock(&m_CS);
	size_32 w;
	if (pcbWritten) *pcbWritten = 0;
	if (m_hFile == INVALID_FILE) return STG_E_INVALIDHANDLE;
	if (m_ReadOnly) return STG_E_ACCESSDENIED;
	if (!pv) return STG_E_INVALIDPOINTER;
	if (!cb) return E_INVALIDARG;
	

	w = fwrite_((void*)pv, cb, m_hFile);

	if (pcbWritten) *pcbWritten = w;

	return (w > 0) ? S_OK : E_FAIL;
}


HRESULT STDMETHODCALLTYPE CFileStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition){
	CriticalSectionClass::LockClass lock(&m_CS);
	size_64 newpos;
	if (plibNewPosition) plibNewPosition->QuadPart = 0;
	if (m_hFile == INVALID_FILE) return STG_E_INVALIDHANDLE;
	if (dlibMove.HighPart != 0) return STG_E_INVALIDFUNCTION;
	if (!(dwOrigin >= STREAM_SEEK_SET && dwOrigin <= STREAM_SEEK_END)) return E_INVALIDARG;

	newpos = fseek_64(m_hFile, *(ssize_64*)&dlibMove.QuadPart, (INT32)dwOrigin);
	if (plibNewPosition) plibNewPosition->QuadPart = newpos;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CFileStream::SetSize(ULARGE_INTEGER libNewSize){
	CriticalSectionClass::LockClass lock(&m_CS);
	size_64 offset;
	if (m_hFile == INVALID_FILE) return STG_E_INVALIDHANDLE;
	if (m_ReadOnly) return STG_E_ACCESSDENIED;
	if (libNewSize.HighPart != 0) return STG_E_INVALIDFUNCTION;

	offset = ftell_64(m_hFile);

	if (libNewSize.QuadPart < offset)
		offset = libNewSize.QuadPart;

	fseek_64(m_hFile, libNewSize.QuadPart, SEEK_SET);

	fseek_64(m_hFile, offset, SEEK_SET);

	return (fsize_64(m_hFile) == libNewSize.QuadPart) ? S_OK : STG_E_MEDIUMFULL;
	
}

HRESULT STDMETHODCALLTYPE CFileStream::CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten){
	return STG_E_UNIMPLEMENTEDFUNCTION;
}

HRESULT STDMETHODCALLTYPE CFileStream::Commit(DWORD grfCommitFlags){
	return STG_E_UNIMPLEMENTEDFUNCTION;
}

HRESULT STDMETHODCALLTYPE CFileStream::Revert(){
	return STG_E_UNIMPLEMENTEDFUNCTION;
}

HRESULT STDMETHODCALLTYPE CFileStream::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType){
	return STG_E_UNIMPLEMENTEDFUNCTION;
}

HRESULT STDMETHODCALLTYPE CFileStream::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType){
	return STG_E_UNIMPLEMENTEDFUNCTION;
}

HRESULT STDMETHODCALLTYPE CFileStream::Stat(STATSTG *pstatstg, DWORD grfStatFlag){
	CriticalSectionClass::LockClass lock(&m_CS);
	LPOLESTR pName;
	if (m_hFile == INVALID_FILE) return STG_E_INVALIDHANDLE;
	if (!pstatstg) return STG_E_INVALIDPOINTER;

	memset(pstatstg, 0, sizeof(STATSTG));
	pstatstg->cbSize.QuadPart = fsize_64(m_hFile);

	pstatstg->grfMode = m_ReadOnly ? STGM_READ : STGM_READWRITE;

	if (grfStatFlag == STATFLAG_DEFAULT){
		pName = (LPOLESTR)CoTaskMemAlloc((StrLenW(m_pFileName)+1)*sizeof(wchar_t));
		if (pName){
			StrCpyW(pName, m_pFileName);
			pstatstg->pwcsName = pName;
		}
	}

	pstatstg->type = STGTY_STREAM;
	
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CFileStream::Clone(IStream **ppstm){
	return STG_E_UNIMPLEMENTEDFUNCTION;
}
