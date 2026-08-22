#include <windows.h>
#include <Urlmon.h>
#include <wininet.h>
#include "Downloader.h"


CFileDownload::CFileDownload(){
	m_pCB = NULL;
	m_lParam = NULL;
	m_wParam = NULL;
	m_pMoniker = NULL;
	m_pBindCtx = NULL;
	m_Canceled = DOWNLOAD_CANCEL_NOT;
	m_RefCount = 1;
}

CFileDownload::~CFileDownload(){
	Close();
}

bool CFileDownload::Start(const wchar_t* pURL, const wchar_t* pFile, pfnDownloadProgressCallback pCB, WPARAM wParam, LPARAM lParam, bool async){

	HRESULT hr;
	IStream *pStream;
	m_pBinding = NULL;

	m_Canceled = DOWNLOAD_CANCEL_NOT;
	m_lParam = lParam;
	m_wParam = wParam;
	m_Event.Reset();

	m_pCB = pCB;

	pStream = NULL;

	m_Async = async;

	if (!m_Stream.Open(pFile, false, false)) return false;

	if ((hr = CreateAsyncBindCtx(0, this, 0, &m_pBindCtx) != S_OK)) return false;

	if ((hr = CreateURLMonikerEx(NULL, pURL, &m_pMoniker, URL_MK_UNIFORM)) != S_OK) goto __cleanup_exit;

	hr = m_pMoniker->BindToStorage(m_pBindCtx, NULL, IID_IStream, (void**)&pStream);
	if (hr != S_OK && !(hr == MK_S_ASYNCHRONOUS && m_Async)) goto __cleanup_exit;
	if (pStream) pStream->Release();


	if (m_Canceled != DOWNLOAD_CANCEL_NOT) goto __cleanup_exit;

	return true;

__cleanup_exit:
	Close();
	return false;
}

void CFileDownload::Close(){
	m_Stream.Close(m_Canceled != DOWNLOAD_CANCEL_NOT);
	if (m_pBindCtx)
		m_pBindCtx->Release();

	if (m_pMoniker)
		m_pMoniker->Release();

	
	m_pCB = NULL;
	m_lParam = NULL;
	m_wParam = NULL;
	m_pMoniker = NULL;
	m_pBindCtx = NULL;

}

bool CFileDownload::Wait(){
	m_Event.Wait();
	return (m_Canceled == DOWNLOAD_CANCEL_NOT);
}

void CFileDownload::Cancel(){
	m_CS.Enter();
	m_Canceled = DOWNLOAD_CANCEL_USER;
	m_pBinding->Abort();
	m_CS.Leave();
}

int CFileDownload::WasCanceled(){
	return m_Canceled;
}

HRESULT STDMETHODCALLTYPE CFileDownload::QueryInterface(REFIID riid, void **ppvObject){
	if (!ppvObject) return E_POINTER;

	if (memcmp(&riid, &IID_IUnknown, sizeof(REFIID)) == 0 ||
		memcmp(&riid, &IID_IBindStatusCallback, sizeof(REFIID)) == 0){
			AddRef();
			*ppvObject = (IUnknown*)this;
			return S_OK;
	}
	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CFileDownload::AddRef(){
	return ++m_RefCount;
}

ULONG STDMETHODCALLTYPE CFileDownload::Release(){
	if (--m_RefCount == 0){
		delete this;
		return 0;
	}
	return m_RefCount;
}

HRESULT STDMETHODCALLTYPE CFileDownload::OnStartBinding(DWORD dwReserved, IBinding *pib){
	if ((m_pBinding = pib))
		pib->AddRef();

	return pib ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE CFileDownload::GetPriority(LONG *pnPriority){
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE CFileDownload::OnLowResource(DWORD reserved){
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE CFileDownload::OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText){
	CriticalSectionClass::LockClass lock(&m_CS);
	if (m_Canceled != DOWNLOAD_CANCEL_NOT){
		m_pBinding->Abort();
		m_Event.Set();
		return E_ABORT;
	}
	if (ulStatusCode == BINDSTATUS_BEGINDOWNLOADDATA){
		if (m_pCB)
			m_Canceled = m_pCB(0, ulProgressMax, DOWNLOAD_BEGIN, m_wParam, m_lParam) ? DOWNLOAD_CANCEL_NOT : DOWNLOAD_CANCEL_USER;
	} else if (ulStatusCode == BINDSTATUS_DOWNLOADINGDATA){
		if (m_pCB)
			m_Canceled = m_pCB(ulProgress, ulProgressMax, DOWNLOAD_DATA, m_wParam, m_lParam) ? DOWNLOAD_CANCEL_NOT : DOWNLOAD_CANCEL_USER;
	} else if (ulStatusCode == BINDSTATUS_ENDDOWNLOADDATA){
		if (m_pCB)
			m_Canceled = m_pCB(ulProgressMax, ulProgressMax, DOWNLOAD_END, m_wParam, m_lParam) ? DOWNLOAD_CANCEL_NOT : DOWNLOAD_CANCEL_USER;
	}
	if (m_Canceled != DOWNLOAD_CANCEL_NOT){
		m_pBinding->Abort();
		m_Event.Set();
		return E_ABORT;
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CFileDownload::OnStopBinding(HRESULT hresult, LPCWSTR szError){
	CriticalSectionClass::LockClass lock(&m_CS);
	if (m_pCB && (m_Canceled != DOWNLOAD_CANCEL_NOT))
		m_pCB(0, (size_32)m_Canceled, DOWNLOAD_CANCELED, m_wParam, m_lParam);
	if (m_pBinding){
		m_pBinding->Release();
		m_pBinding = NULL;
	}
	m_Event.Set();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CFileDownload::GetBindInfo(DWORD *grfBINDF, BINDINFO *pbindinfo){
	if (!grfBINDF) return E_INVALIDARG;
	*grfBINDF &= ~DOWNLOAD_CLEAR_FLAGS;
	*grfBINDF |= BINDF_GETNEWESTVERSION | BINDF_RESYNCHRONIZE | BINDF_PRAGMA_NO_CACHE | BINDF_NOWRITECACHE;
	if (m_Async) *grfBINDF |= BINDF_ASYNCHRONOUS;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CFileDownload::OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM *pstgmed){
	ULONG r, w;
	char lBuf[1024];
	CriticalSectionClass::LockClass lock(&m_CS);
	if (m_Canceled != DOWNLOAD_CANCEL_NOT){
		m_pBinding->Abort();
		m_Event.Set();
		return E_ABORT;
	}

	while (pstgmed->pstm->Read(lBuf, min(dwSize, sizeof(lBuf)), &r) == S_OK){

		if (m_Stream.Write(lBuf, r, &w) != S_OK) break;

		if (r != w) break;
		
		dwSize -= r;
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CFileDownload::OnObjectAvailable(REFIID riid, IUnknown *punk){
	NOTIMPLEMENTED;
}
