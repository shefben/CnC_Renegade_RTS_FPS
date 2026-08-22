#include <windows.h>
#include <Urlmon.h>
#include <wininet.h>
#include "Downloader.h"


CStreamDownload::CStreamDownload(){
	m_pCB = NULL;
	m_lParam = NULL;
	m_wParam = NULL;
	m_pMoniker = NULL;
	m_pBindCtx = NULL;
	m_Canceled = false;
	m_RefCount = 1;
	m_pStream = NULL;
}

CStreamDownload::~CStreamDownload(){
	Close();
}

bool CStreamDownload::Start(const wchar_t* pURL, IStream* pStream, pfnDownloadProgressCallback pCB, WPARAM wParam, LPARAM lParam, bool async){

	HRESULT hr;

	m_Canceled = false;
	m_pStream = NULL;
	m_lParam = lParam;
	m_wParam = wParam;
	m_Event.Reset();

	m_pCB = pCB;

	if (!(m_pStream = pStream)) return false;
	pStream->AddRef();

	pStream = NULL;

	m_Async = async;

	if ((hr = CreateAsyncBindCtx(0, this, 0, &m_pBindCtx) != S_OK)) return false;

	if ((hr = CreateURLMonikerEx(NULL, pURL, &m_pMoniker, URL_MK_UNIFORM)) != S_OK) goto __cleanup_exit;

	hr = m_pMoniker->BindToStorage(m_pBindCtx, NULL, IID_IStream, (void**)&pStream);
	if (hr != S_OK && !(hr == MK_S_ASYNCHRONOUS && m_Async)) goto __cleanup_exit;
	if (pStream) pStream->Release();


	if (m_Canceled) goto __cleanup_exit;

	return true;

__cleanup_exit:
	Close();
	return false;
}

void CStreamDownload::Close(){
	if (m_pStream)
		m_pStream->Release();

	if (m_pBindCtx)
		m_pBindCtx->Release();

	if (m_pMoniker)
		m_pMoniker->Release();

	
	m_pCB = NULL;
	m_lParam = NULL;
	m_wParam = NULL;
	m_pMoniker = NULL;
	m_pBindCtx = NULL;
	m_pStream = NULL;
	m_Canceled = false;

}

bool CStreamDownload::Wait(){
	m_Event.Wait();
	return !m_Canceled;
}

void CStreamDownload::Cancel(){
	m_CS.Enter();
	m_Canceled = true;
	m_CS.Leave();
}

bool CStreamDownload::WasCanceled(){
	return m_Canceled;
}

HRESULT STDMETHODCALLTYPE CStreamDownload::QueryInterface(REFIID riid, void **ppvObject){
	if (!ppvObject) return E_POINTER;

	if (memcmp(&riid, &IID_IUnknown, sizeof(REFIID)) == 0 ||
		memcmp(&riid, &IID_IBindStatusCallback, sizeof(REFIID)) == 0){
			AddRef();
			*ppvObject = (IUnknown*)this;
			return S_OK;
	}
	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CStreamDownload::AddRef(){
	return ++m_RefCount;
}

ULONG STDMETHODCALLTYPE CStreamDownload::Release(){
	if (--m_RefCount == 0){
		delete this;
		return 0;
	}
	return m_RefCount;
}

HRESULT STDMETHODCALLTYPE CStreamDownload::OnStartBinding(DWORD dwReserved, IBinding *pib){
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CStreamDownload::GetPriority(LONG *pnPriority){
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE CStreamDownload::OnLowResource(DWORD reserved){
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE CStreamDownload::OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText){
	CriticalSectionClass::LockClass lock(&m_CS);
	if (m_Canceled){
		m_Event.Set();
		return E_ABORT;
	}
	if (ulStatusCode == BINDSTATUS_BEGINDOWNLOADDATA){
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

HRESULT STDMETHODCALLTYPE CStreamDownload::OnStopBinding(HRESULT hresult, LPCWSTR szError){
	CriticalSectionClass::LockClass lock(&m_CS);
	if (m_pCB && m_Canceled)
		m_pCB(0, 0, DOWNLOAD_CANCELED, m_wParam, m_lParam);
	m_Event.Set();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CStreamDownload::GetBindInfo(DWORD *grfBINDF, BINDINFO *pbindinfo){
	if (!grfBINDF) return E_ABORT;
	*grfBINDF &= ~DOWNLOAD_CLEAR_FLAGS;
	*grfBINDF |= BINDF_GETNEWESTVERSION | BINDF_RESYNCHRONIZE | BINDF_PRAGMA_NO_CACHE | BINDF_NOWRITECACHE;
	if (m_Async) *grfBINDF |= BINDF_ASYNCHRONOUS;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CStreamDownload::OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM *pstgmed){
	ULONG r, w;
	char lBuf[1024];
	CriticalSectionClass::LockClass lock(&m_CS);
	if (!pstgmed) return E_INVALIDARG;
	if (m_Canceled){
		m_Event.Set();
		return E_ABORT;
	}

	while (pstgmed->pstm->Read(lBuf, min(dwSize, sizeof(lBuf)), &r) == S_OK){

		if (m_pStream->Write(lBuf, r, &w) != S_OK) break;

		if (r != w) break;
		
		dwSize -= r;
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CStreamDownload::OnObjectAvailable(REFIID riid, IUnknown *punk){
	NOTIMPLEMENTED;
}
