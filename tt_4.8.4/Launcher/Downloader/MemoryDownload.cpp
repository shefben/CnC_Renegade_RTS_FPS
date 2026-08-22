#include <windows.h>
#include <Urlmon.h>
#include <wininet.h>
#include "Downloader.h"


CMemoryDownload::CMemoryDownload(){
	m_Offset = 0;
	m_LocalAlloc = false;
	m_BufferSize = 0;

	m_pBuffer = NULL;
	
	m_pMoniker = NULL;
	m_pBindCtx = NULL;

	m_Canceled = false;
}


CMemoryDownload::~CMemoryDownload(){
	Close();
}

bool CMemoryDownload::Start(const wchar_t* pURL, ptr pBuf, size_32 bufsize, size_32 maxsize, pfnDownloadProgressCallback pCB, WPARAM wParam, LPARAM lParam, bool async){
	IStream* pStream;
	m_pBuffer = pBuf;
	m_BufferSize = bufsize;
	m_MaxSize = maxsize;
	HRESULT hr;

	m_Async = async;

	m_wParam = wParam;
	m_lParam = lParam;
	m_pCB = pCB;

	pStream = NULL;

	if ((hr = CreateAsyncBindCtx(0, this, 0, &m_pBindCtx) != S_OK)) return false;

	if ((hr = CreateURLMonikerEx(NULL, pURL, &m_pMoniker, URL_MK_UNIFORM)) != S_OK) goto __cleanup_exit;

	hr = m_pMoniker->BindToStorage(m_pBindCtx, NULL, IID_IStream, (void**)&pStream);

	if (hr != S_OK && !(hr == MK_S_ASYNCHRONOUS && m_Async)) goto __cleanup_exit;
	if (pStream) pStream->Release();

	return true;
__cleanup_exit:
	if (m_pBindCtx)
		m_pBindCtx->Release();
	if (m_pMoniker)
		m_pMoniker->Release();

	m_pBindCtx = NULL;
	m_pMoniker = NULL;
	return false;
}

bool CMemoryDownload::Wait(){
	m_Event.Wait();
	return !m_Canceled;
}

void CMemoryDownload::Cancel(){
	m_CS.Enter();
	m_Canceled = true;
	m_CS.Leave();
}

bool CMemoryDownload::WasCanceled(){
	return m_Canceled;
}

void CMemoryDownload::Close(){
	if (m_LocalAlloc && m_pBuffer){
		g_pFreeer(m_pBuffer);
		m_pBuffer = NULL;
	}

	if (m_pMoniker)
		m_pMoniker->Release();

	if (m_pBindCtx)
		m_pBindCtx->Release();

	m_pMoniker = NULL;
	m_pBindCtx = NULL;
}

size_32 CMemoryDownload::GetSize(){
	CriticalSectionClass::LockClass lock(&m_CS);
	return m_Offset;
}

ptr CMemoryDownload::GetBuffer(){
	CriticalSectionClass::LockClass lock(&m_CS);
	return m_pBuffer;
}

void CMemoryDownload::LockBuffer(){
	m_CS.Enter();
}

void CMemoryDownload::UnlockBuffer(){
	m_CS.Leave();
}

HRESULT STDMETHODCALLTYPE CMemoryDownload::QueryInterface(REFIID riid, void **ppvObject){
	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CMemoryDownload::AddRef(){
	return 1;
}

ULONG STDMETHODCALLTYPE CMemoryDownload::Release(){
	return 1;
}

HRESULT STDMETHODCALLTYPE CMemoryDownload::OnStartBinding(DWORD dwReserved, IBinding *pib){
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CMemoryDownload::GetPriority(LONG *pnPriority){
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE CMemoryDownload::OnLowResource(DWORD reserved){
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE CMemoryDownload::OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText){

	CriticalSectionClass::LockClass lock(&m_CS);
	if (m_Canceled){
		m_Event.Set();
		return E_ABORT;
	}
	if (ulStatusCode == BINDSTATUS_BEGINDOWNLOADDATA){
		if (ulProgressMax > m_MaxSize){
			m_Canceled = true;
			m_Event.Set();
			return E_ABORT;
		}

		
		if (!m_pBuffer){
			m_LocalAlloc = true;
			m_pBuffer = g_pAllocator(ulProgressMax);
			if (!m_pBuffer){				
				m_Canceled = true;
				m_Event.Set();
				return E_ABORT;
			}
			m_BufferSize = ulProgressMax;
		} else if(ulProgressMax > m_BufferSize){
			m_Canceled = true;
			m_Event.Set();
			return E_ABORT;
		}

		if (m_pCB)
			m_Canceled = !m_pCB(0, ulProgressMax, DOWNLOAD_BEGIN, m_wParam, m_lParam);			
	} else if (ulStatusCode == BINDSTATUS_DOWNLOADINGDATA){
		if (m_pCB)
			m_Canceled = !m_pCB(ulProgress, ulProgressMax, DOWNLOAD_DATA, m_wParam, m_lParam);
	} else if (ulStatusCode == BINDSTATUS_ENDDOWNLOADDATA){
		if (m_pCB)
			m_Canceled = !m_pCB(ulProgressMax, ulProgressMax, DOWNLOAD_END, m_wParam, m_lParam);
	}
	return m_Canceled ? E_ABORT : S_OK;
}

HRESULT STDMETHODCALLTYPE CMemoryDownload::OnStopBinding(HRESULT hresult, LPCWSTR szError){
	m_Event.Set();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CMemoryDownload::GetBindInfo(DWORD *grfBINDF, BINDINFO *pbindinfo){
	if (!grfBINDF) return E_INVALIDARG;
	*grfBINDF &= ~DOWNLOAD_CLEAR_FLAGS;
	*grfBINDF |= BINDF_GETNEWESTVERSION | BINDF_RESYNCHRONIZE | BINDF_PRAGMA_NO_CACHE | BINDF_NOWRITECACHE;
	if (m_Async) *grfBINDF |= BINDF_ASYNCHRONOUS;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CMemoryDownload::OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM *pstgmed){
	STATSTG stat;
	ULONG r;
	CriticalSectionClass::LockClass lock(&m_CS);
	if (m_Canceled){
		m_Event.Set();
		return E_ABORT;
	}
	pstgmed->pstm->Stat(&stat, STATFLAG_NONAME);
	if (stat.cbSize.QuadPart == 0) return S_OK;
	pstgmed->pstm->Read(((unsigned char*)m_pBuffer)+m_Offset, stat.cbSize.LowPart, &r);
	if (r != (dwSize-m_Offset)){		
		m_Canceled = true;
		m_Event.Set();
		return E_ABORT;
	}
	m_Offset += r;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CMemoryDownload::OnObjectAvailable(REFIID riid, IUnknown *punk){
	NOTIMPLEMENTED;
}
