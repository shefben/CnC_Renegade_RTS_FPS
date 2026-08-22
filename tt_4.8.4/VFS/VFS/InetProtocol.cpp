#include <windows.h>
//#include "BrowserAPI.h"
#include "InetProtocol.h"
#include "../Shared/StdLib.h"
#define NO_SHLWAPI_STRFCNS
#include <shlwapi.h>
#include "dllmain.h"

#include "API/VFSInterface.h"

IInternetSession* GetInternetSession(){
	IInternetSession* s_pInternetSession = NULL;
	if (!s_pInternetSession)
		if (CoInternetGetSession(0, &s_pInternetSession, 0) != S_OK)
			s_pInternetSession = NULL;

	return s_pInternetSession;
}



CProtocolClassFactory::CProtocolClassFactory(){
	m_RefCount = 1;
}

CProtocolClassFactory::~CProtocolClassFactory(){
}

HRESULT CProtocolClassFactory::UnwrapSpecialUrl(LPCWSTR pchUrl, wchar_t *pBuf){
	HRESULT hr;
	const wchar_t *pSpecial;
	wchar_t lBuf[URL_BUF_SIZE];
	DWORD size;

	hr = CoInternetParseUrl(pchUrl, PARSE_ENCODE, 0, lBuf, URL_BUF_SIZE, &size, 0);

	if (hr != S_OK) return hr;

	pSpecial = StrrChrW(lBuf, '\1');

	StrCpyW(pBuf, pSpecial ? pSpecial+1 : lBuf);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CProtocolClassFactory::QueryInterface(REFIID riid, void **ppvObject){
	if (!ppvObject) return E_INVALIDARG;
	if (memcmp(&riid, &IID_IUnknown, sizeof(IID)) == 0)
		*ppvObject = (IUnknown*)((IClassFactory*)this);
	else if (memcmp(&riid, &IID_IClassFactory, sizeof(IID)) == 0)
		*ppvObject = (IClassFactory*)this;
	else if (memcmp(&riid, &IID_IInternetProtocolInfo, sizeof(IID)) == 0)
		*ppvObject = (IInternetProtocolInfo*)this;
	else {
		*ppvObject = NULL;		
		return E_NOINTERFACE;
	}
	AddRef();
	return S_OK;
}

ULONG STDMETHODCALLTYPE CProtocolClassFactory::AddRef(){
	return ++m_RefCount;
}

ULONG STDMETHODCALLTYPE CProtocolClassFactory::Release(){
	//Will be used for static vars, so no "delete this;"
	return --m_RefCount;
}


HRESULT STDMETHODCALLTYPE CProtocolClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject){
	*ppvObject = new CVFSProtocol();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CProtocolClassFactory::LockServer(BOOL fLock){
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CProtocolClassFactory::ParseUrl(LPCWSTR pwzUrl, PARSEACTION ParseAction, DWORD dwParseFlags, LPWSTR pwzResult, DWORD cchResult, DWORD *pcchResult, DWORD dwReserved){
	HRESULT hr;
	hr = INET_E_DEFAULT_ACTION;
	wchar_t unwrapped[URL_BUF_SIZE];
	BSTR temp;
	DWORD len;

	if (!pcchResult || !pwzResult){
		hr = E_POINTER;
		goto __cleanup;
	}
	LogFormated(L"Parse action: %d\n", ParseAction);

	if (ParseAction == PARSE_DOMAIN){
		hr = UnwrapSpecialUrl(pwzUrl, unwrapped);
		if (hr != S_OK) goto __cleanup;
		len = StrLenW(unwrapped)+1;

		*pcchResult = len;

		if (len > cchResult){
			hr = S_FALSE;
			goto __cleanup;
		}

		temp = SysAllocStringLen(unwrapped, len);

		UrlGetPartW(temp, pwzResult, pcchResult, URL_PART_HOSTNAME, 0);

		SysFreeString(temp);
		hr = S_OK;
	}

__cleanup:
	return hr;
}

HRESULT STDMETHODCALLTYPE CProtocolClassFactory::CombineUrl(LPCWSTR pwzBaseUrl, LPCWSTR pwzRelativeUrl, DWORD dwCombineFlags, LPWSTR pwzResult, DWORD cchResult, DWORD *pcchResult, DWORD dwReserved){
	HRESULT hr;
	const wchar_t *pSrc;
	hr = INET_E_DEFAULT_ACTION;

	if (pwzBaseUrl && (pSrc = StrrChrW(pwzBaseUrl, '\1')))
		hr = CoInternetCombineUrl(++pSrc, pwzRelativeUrl, URL_ESCAPE_SPACES_ONLY, pwzResult, cchResult, pcchResult, 0);
	return hr;
}

HRESULT STDMETHODCALLTYPE CProtocolClassFactory::CompareUrl(LPCWSTR pwzUrl1, LPCWSTR pwzUrl2, DWORD dwCompareFlags){
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CProtocolClassFactory::QueryInfo(LPCWSTR pwzUrl, QUERYOPTION QueryOption, DWORD dwQueryFlags, LPVOID pBuffer, DWORD cbBuffer, DWORD *pcbBuf, DWORD dwReserved){
	HRESULT hr;

	hr = INET_E_DEFAULT_ACTION;

	switch (QueryOption){
		case QUERY_USES_NETWORK:
			if (!pBuffer || cbBuffer < sizeof(DWORD))
				return E_FAIL;

			if (pcbBuf) *pcbBuf = sizeof(DWORD);
			*(DWORD*)pBuffer = FALSE;
			hr = S_OK;
			break;
		case QUERY_IS_SECURE:
			if (!pBuffer || cbBuffer < sizeof(DWORD))
				return E_FAIL;

			if (pcbBuf) *pcbBuf = sizeof(DWORD);
			*(DWORD*)pBuffer = FALSE;
			hr = S_OK;
			break;
	}
	return hr;
}

/*
CVFSProtocol
*/

CVFSProtocol::CVFSProtocol(){
	m_RefCount = 1;
	m_pProtocolSink = NULL;
	m_pBindInfo = NULL;
	m_pStream = NULL;
}

CVFSProtocol::~CVFSProtocol(){
	if (m_pStream)
		m_pStream->Release();
}

HRESULT CVFSProtocol::DoBind(){
	HRESULT hr;
	wchar_t VFS[URL_BUF_SIZE];
	wchar_t Src[URL_BUF_SIZE];
	IVFS* pVFS;

	STATSTG stat;
	hr = GetURLComponents(m_URL, VFS, URL_BUF_SIZE, Src, URL_BUF_SIZE);
	//MsgFormated(L"URL: \"%s\" VFS: \"%s\" Src: \"%s\"", m_URL, VFS, Src);
	
	if (!m_pProtocolSink) return E_POINTER;

	if (hr != S_OK) return hr;
	pVFS = QueryCoreInterface()->FindVFSForDomain(VFS);
	if (!pVFS){
		m_pProtocolSink->ReportResult(E_FAIL, 0, 0);
		return E_FAIL;
	}

	m_pStream = pVFS->VFSCreateIStreamFile(Src, FILE_OPEN_EXISTING | FILE_OPEN_READ);
	if (!m_pStream){
		m_pProtocolSink->ReportResult(E_FAIL, 0, 0);
		return E_FAIL;
	}


	if (m_pStream->Stat(&stat, 0) != S_OK || stat.cbSize.HighPart != 0){
		m_pProtocolSink->ReportResult(E_FAIL, 0, 0);
		return E_FAIL;
	}

	m_pProtocolSink->ReportData(BSCF_LASTDATANOTIFICATION | BSCF_DATAFULLYAVAILABLE, stat.cbSize.LowPart, stat.cbSize.LowPart);
	m_pProtocolSink->ReportResult(S_OK, 0, 0);

	return S_OK;
}

HRESULT CVFSProtocol::GetURLComponents(const wchar_t* pURL, wchar_t *pVFS, DWORD VFSSize, wchar_t* pSrc, DWORD SrcSize){

	const wchar_t* pFile;
	wchar_t lBuf[URL_BUF_SIZE];
	int len;
	if (wcsnicmp(pURL, L"vfs://", (sizeof(L"vfs://")/sizeof(wchar_t))-1) != 0) return MK_E_SYNTAX;
	pURL = pURL+(sizeof(L"vfs://")/sizeof(wchar_t))-1;

	if (StrLenW(pURL) == 0 || (pFile = StrChrW(pURL, '/')) == NULL || (len = StrLenW(++pFile)) == 0) return MK_E_SYNTAX;

	StrCpyW(lBuf, pURL);

	*((wchar_t*)(StrChrW(lBuf, '/'))) = 0;
	if (StrLenW(lBuf) == 0) return MK_E_SYNTAX;

	if (pVFS && VFSSize)
		StrnCpyW(pVFS, lBuf, VFSSize);

	if (pSrc && SrcSize)
		StrnCpyW(pSrc, pFile, SrcSize);


	return S_OK;
}

/*
IUnknown
*/

HRESULT STDMETHODCALLTYPE CVFSProtocol::QueryInterface(REFIID riid, void **ppvObject){
	if (!ppvObject) return E_POINTER;

	if (memcmp(&riid, &IID_IUnknown, sizeof(IID)) == 0)
		*ppvObject = (IUnknown*)this;
	else if (memcmp(&riid, &IID_IInternetProtocol, sizeof(IID)) == 0 || memcmp(&riid, &IID_IInternetProtocolRoot, sizeof(IID)) == 0)
		*ppvObject = (IInternetProtocol*)this;
	else
		return E_NOINTERFACE;
	

	((IUnknown *)*ppvObject)->AddRef();
	return S_OK;
}

ULONG STDMETHODCALLTYPE CVFSProtocol::AddRef(){
	return ++m_RefCount;
}

ULONG STDMETHODCALLTYPE CVFSProtocol::Release(){
	ULONG refcount;
	refcount = --m_RefCount;
	if (m_RefCount == 0)
		delete this;
	return refcount;
}


/*
IInternetProtocolRoot
*/

HRESULT STDMETHODCALLTYPE CVFSProtocol::Start(LPCWSTR szUrl, IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo, DWORD grfPI, HANDLE_PTR dwReserved){

	HRESULT hr;

	PROTOCOLDATA protdata;

	DWORD size;

	hr = S_OK;

	if (!(grfPI & PI_PARSE_URL)){
		if (m_pProtocolSink) m_pProtocolSink->Release();
		if (pOIProtSink) (m_pProtocolSink = pOIProtSink)->AddRef();
		
		if (m_pBindInfo) m_pBindInfo->Release();
		if (pOIBindInfo) (m_pBindInfo = pOIBindInfo)->AddRef();
		
	}

	m_BindInfo.cbSize = sizeof(BINDINFO);

	hr = pOIBindInfo->GetBindInfo(&m_BindFlags, &m_BindInfo);

	hr = CoInternetParseUrl(szUrl, PARSE_ENCODE, 0, m_URL, URL_BUF_SIZE, &size, 0);
	if (hr != S_OK) return hr;

	if (m_BindInfo.szExtraInfo)
		StrCatW(m_URL, m_BindInfo.szExtraInfo);

	m_Flags = grfPI;

    protdata.grfFlags = PI_FORCE_ASYNC;
    protdata.dwState = BIND_ASYNC;
    protdata.pData = NULL;
    protdata.cbData = 0;

	m_pProtocolSink->Switch(&protdata);
	

	return E_PENDING;
}

HRESULT STDMETHODCALLTYPE CVFSProtocol::Continue(PROTOCOLDATA *pProtocolData){
	if (pProtocolData->dwState == BIND_ASYNC)
		return DoBind();
	return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CVFSProtocol::Abort(HRESULT hrReason, DWORD dwOptions){
	return m_pProtocolSink->ReportResult(E_ABORT, 0, NULL);
}

HRESULT STDMETHODCALLTYPE CVFSProtocol::Terminate(DWORD dwOptions){
	ReleaseBindInfo(&m_BindInfo);

	if (m_pProtocolSink){
		m_pProtocolSink->Release();
		m_pProtocolSink = NULL;
	}

	if (m_pBindInfo){
		m_pBindInfo->Release();
		m_pBindInfo = NULL;
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CVFSProtocol::Suspend(){
	return E_NOTIMPL; //Not available
}

HRESULT STDMETHODCALLTYPE CVFSProtocol::Resume(){
	return E_NOTIMPL; //Not available
}


/*
IInternetProtocol
*/

HRESULT STDMETHODCALLTYPE CVFSProtocol::Read(void *pv, ULONG cb, ULONG *pcbRead){

	LARGE_INTEGER offset;
	ULARGE_INTEGER newoffset;
	offset.QuadPart = 0;
	STATSTG stat;
	if (!m_pStream) return E_POINTER;
	m_pStream->Read(pv, cb, pcbRead);
	m_pStream->Seek(offset, STREAM_SEEK_CUR, &newoffset);
	m_pStream->Stat(&stat, 0);

	return (newoffset.QuadPart == stat.cbSize.QuadPart) ? S_FALSE : S_OK;
}

HRESULT STDMETHODCALLTYPE CVFSProtocol::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition){
	if (!m_pStream) return E_POINTER;
	return m_pStream->Seek(dlibMove, dwOrigin, plibNewPosition);
}

HRESULT STDMETHODCALLTYPE CVFSProtocol::LockRequest(DWORD dwOptions){
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CVFSProtocol::UnlockRequest(){
	return E_NOTIMPL;
}
