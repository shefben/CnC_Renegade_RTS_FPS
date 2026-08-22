#include <windows.h>
#include "BrowserAPI.h"
#include "InetProtocol.h"
#include "../Shared/StdLib.h"
#include "../Shared/Memory.h"
#define NO_SHLWAPI_STRFCNS
#include <shlwapi.h>
#include "Browser.h"
#include <Mlang.h>

/*wchar_t HexDigitToNum(wchar_t chr){
	if (chr >= '0' && chr <= '9') return chr-'0';
	if (chr >= 'a' && chr <= 'f') return chr-'a'+10;
	if (chr >= 'A' && chr <= 'F') return chr-'A'+10;
	return chr;
}*/



/*
CDLLProtocolClassFactory
*/

CDLLProtocolClassFactory::CDLLProtocolClassFactory(){
	m_RefCount = 1;
}

CDLLProtocolClassFactory::~CDLLProtocolClassFactory(){
}

HRESULT CDLLProtocolClassFactory::UnwrapSpecialUrl(LPCWSTR pchUrl, wchar_t *pBuf){
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

HRESULT STDMETHODCALLTYPE CDLLProtocolClassFactory::QueryInterface(REFIID riid, void **ppvObject){
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

ULONG STDMETHODCALLTYPE CDLLProtocolClassFactory::AddRef(){
	return ++m_RefCount;
}

ULONG STDMETHODCALLTYPE CDLLProtocolClassFactory::Release(){
	//Will be used for static vars, so no "delete this;"
	return --m_RefCount;
}


HRESULT STDMETHODCALLTYPE CDLLProtocolClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject){
	*ppvObject = new CDLLProtocol();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDLLProtocolClassFactory::LockServer(BOOL fLock){
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDLLProtocolClassFactory::ParseUrl(LPCWSTR pwzUrl, PARSEACTION ParseAction, DWORD dwParseFlags, LPWSTR pwzResult, DWORD cchResult, DWORD *pcchResult, DWORD dwReserved){
	HRESULT hr;
	hr = INET_E_DEFAULT_ACTION;
	wchar_t unwrapped[URL_BUF_SIZE];
	BSTR temp;
	DWORD len;

	if (!pcchResult || !pwzResult){
		hr = E_POINTER;
		goto __cleanup;
	}
#ifdef _DEBUG
	LogFormated(L"Parse action: %d\n", ParseAction);
#endif //_DEBUG

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

HRESULT STDMETHODCALLTYPE CDLLProtocolClassFactory::CombineUrl(LPCWSTR pwzBaseUrl, LPCWSTR pwzRelativeUrl, DWORD dwCombineFlags, LPWSTR pwzResult, DWORD cchResult, DWORD *pcchResult, DWORD dwReserved){
	HRESULT hr;
	const wchar_t *pSrc;
	hr = INET_E_DEFAULT_ACTION;

	if (pwzBaseUrl && (pSrc = StrrChrW(pwzBaseUrl, '\1')))
		hr = CoInternetCombineUrl(++pSrc, pwzRelativeUrl, URL_ESCAPE_SPACES_ONLY, pwzResult, cchResult, pcchResult, 0);
	return hr;
}

HRESULT STDMETHODCALLTYPE CDLLProtocolClassFactory::CompareUrl(LPCWSTR pwzUrl1, LPCWSTR pwzUrl2, DWORD dwCompareFlags){
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDLLProtocolClassFactory::QueryInfo(LPCWSTR pwzUrl, QUERYOPTION QueryOption, DWORD dwQueryFlags, LPVOID pBuffer, DWORD cbBuffer, DWORD *pcbBuf, DWORD dwReserved){
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
		case QUERY_CAN_NAVIGATE:
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

CDLLProtocol::CDLLProtocol(){
	m_RefCount = 1;
	m_pProtocolSink = NULL;
	m_pBindInfo = NULL;
}

CDLLProtocol::~CDLLProtocol(){
}

HRESULT CDLLProtocol::DoBind(){
	HRESULT hr;
	int count;
	count = DLL_MAX_PARAMS+2;
	wchar_t *pArray[DLL_MAX_PARAMS+2]; //Maximum params + dll and function name
	wchar_t *pNewURL;

	pNewURL = StrDupW(m_URL);

	if (!pNewURL) return E_OUTOFMEMORY;
	
	hr = GetURLComponents(pNewURL, pArray, &count);

	if (hr != S_OK) goto __exit;

	if (count < 2){
		hr = MK_E_SYNTAX;
		goto __exit;
	}

#ifdef _DEBUG
	LogFormated(L"Dll: %s; function: %s\n", pArray[0], pArray[1]);

	for (int i = 2; i < count; i++)
		LogFormated(L"Dll arg %d: %s\n", i-2, pArray[i]);
#endif //_DEBUG

	
	

__exit:
	g_pFreeer(pNewURL);
	return hr;
}

HRESULT CDLLProtocol::GetURLComponents(wchar_t* pURL, wchar_t** ppParsed, int *pCount){
	int c;
	if (wcsnicmp(pURL, L"dll://", (sizeof(L"dll://")/sizeof(wchar_t))-1) != 0) return MK_E_SYNTAX;
	pURL = pURL+(sizeof(L"dll://")/sizeof(wchar_t))-1;

	if (StrLenW(pURL) == 0) return MK_E_SYNTAX;
	
	c = TokenizeW(pURL, ppParsed, *pCount, L"/\\", NULL, FALSE, FALSE, FALSE, TRUE);

	*pCount = c;


	return S_OK;
}

/*
IUnknown
*/

HRESULT STDMETHODCALLTYPE CDLLProtocol::QueryInterface(REFIID riid, void **ppvObject){
	if (!ppvObject) return E_POINTER;

	if (memcmp(&riid, &IID_IUnknown, sizeof(IID)) == 0)
		*ppvObject = (IUnknown*)this;
	else if (memcmp(&riid, &IID_IInternetProtocol, sizeof(IID)) == 0 || memcmp(&riid, &IID_IInternetProtocolRoot, sizeof(IID)) == 0)
		*ppvObject = (IInternetProtocol*)this;
	else{
#ifdef _DEBUG
		//79EAC9EB-BAF9-11CE-8C82-00AA004BA90B: IInternetPriority
		//C7A98E66-1010-492C-A1C8-C809E1F75905: IInternetProtocolEx
		LogFormated(L"Unknown IID: ");
		LogIID(&riid);
		LogFormated(L"\n");
#endif //_DEBUG
		return E_NOINTERFACE;
	}

	((IUnknown *)*ppvObject)->AddRef();
	return S_OK;
}

ULONG STDMETHODCALLTYPE CDLLProtocol::AddRef(){
	return ++m_RefCount;
}

ULONG STDMETHODCALLTYPE CDLLProtocol::Release(){
	ULONG refcount;
	refcount = --m_RefCount;
	if (m_RefCount == 0)
		delete this;
	return refcount;
}


/*
IInternetProtocolRoot
*/

HRESULT STDMETHODCALLTYPE CDLLProtocol::Start(LPCWSTR szUrl, IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo, DWORD grfPI, HANDLE_PTR dwReserved){

	HRESULT hr;

	PROTOCOLDATA protdata;

	DWORD size;

	hr = S_OK;
#ifdef _DEBUG
	LogFormated(L"-> %s\n", szUrl);
#endif //_DEBUG

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

HRESULT STDMETHODCALLTYPE CDLLProtocol::Continue(PROTOCOLDATA *pProtocolData){
	if (pProtocolData->dwState == BIND_ASYNC)
		return DoBind();
	return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CDLLProtocol::Abort(HRESULT hrReason, DWORD dwOptions){
	return m_pProtocolSink->ReportResult(E_ABORT, 0, NULL);
}

HRESULT STDMETHODCALLTYPE CDLLProtocol::Terminate(DWORD dwOptions){
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

HRESULT STDMETHODCALLTYPE CDLLProtocol::Suspend(){
	return E_NOTIMPL; //Not available
}

HRESULT STDMETHODCALLTYPE CDLLProtocol::Resume(){
	return E_NOTIMPL; //Not available
}


/*
IInternetProtocol
*/

HRESULT STDMETHODCALLTYPE CDLLProtocol::Read(void *pv, ULONG cb, ULONG *pcbRead){
	return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CDLLProtocol::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition){
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDLLProtocol::LockRequest(DWORD dwOptions){
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDLLProtocol::UnlockRequest(){
	return E_NOTIMPL;
}

/*
Dynamic protocol
*/

CDynProtocolClassFactory::CDynProtocolClassFactory(){
	m_RefCount = 1;
}

CDynProtocolClassFactory::~CDynProtocolClassFactory(){
}

HRESULT CDynProtocolClassFactory::UnwrapSpecialUrl(LPCWSTR pchUrl, wchar_t *pBuf){
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

HRESULT STDMETHODCALLTYPE CDynProtocolClassFactory::QueryInterface(REFIID riid, void **ppvObject){
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

ULONG STDMETHODCALLTYPE CDynProtocolClassFactory::AddRef(){
	return ++m_RefCount;
}

ULONG STDMETHODCALLTYPE CDynProtocolClassFactory::Release(){
	//Will be used for static vars, so no "delete this;"
	return --m_RefCount;
}


HRESULT STDMETHODCALLTYPE CDynProtocolClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject){
	*ppvObject = new CDynProtocol();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDynProtocolClassFactory::LockServer(BOOL fLock){
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDynProtocolClassFactory::ParseUrl(LPCWSTR pwzUrl, PARSEACTION ParseAction, DWORD dwParseFlags, LPWSTR pwzResult, DWORD cchResult, DWORD *pcchResult, DWORD dwReserved){
	HRESULT hr;
	hr = INET_E_DEFAULT_ACTION;
	wchar_t unwrapped[URL_BUF_SIZE];
	BSTR temp;
	DWORD len;

	if (!pcchResult || !pwzResult){
		hr = E_POINTER;
		goto __cleanup;
	}
#ifdef _DEBUG
	LogFormated(L"Parse action: %d\n", ParseAction);
#endif //_DEBUG

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

HRESULT STDMETHODCALLTYPE CDynProtocolClassFactory::CombineUrl(LPCWSTR pwzBaseUrl, LPCWSTR pwzRelativeUrl, DWORD dwCombineFlags, LPWSTR pwzResult, DWORD cchResult, DWORD *pcchResult, DWORD dwReserved){
	HRESULT hr;
	const wchar_t *pSrc;
	hr = INET_E_DEFAULT_ACTION;

	if (pwzBaseUrl && (pSrc = StrrChrW(pwzBaseUrl, '\1')))
		hr = CoInternetCombineUrl(++pSrc, pwzRelativeUrl, URL_ESCAPE_SPACES_ONLY, pwzResult, cchResult, pcchResult, 0);
	return hr;
}

HRESULT STDMETHODCALLTYPE CDynProtocolClassFactory::CompareUrl(LPCWSTR pwzUrl1, LPCWSTR pwzUrl2, DWORD dwCompareFlags){
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDynProtocolClassFactory::QueryInfo(LPCWSTR pwzUrl, QUERYOPTION QueryOption, DWORD dwQueryFlags, LPVOID pBuffer, DWORD cbBuffer, DWORD *pcbBuf, DWORD dwReserved){
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

CDynProtocol::CDynProtocol(){
	m_RefCount = 1;
	m_pProtocolSink = NULL;
	m_pBindInfo = NULL;
	m_pStream = NULL;
}

CDynProtocol::~CDynProtocol(){
	if (m_pStream) m_pStream->Release();
}

HRESULT CDynProtocol::DoBind(){
	HRESULT hr;
	int count;
	STATSTG stat;
	count = DLL_MAX_PARAMS+1;
	wchar_t *pArray[DYN_MAX_PARAMS+1]; //Maximum params + handler name
	wchar_t *pNewURL;

	pNewURL = StrDupW(m_URL);

	if (!pNewURL) return E_OUTOFMEMORY;
	
	hr = GetURLComponents(pNewURL, pArray, &count);

	if (hr != S_OK) goto __exit;

	if (count < 1){
		hr = MK_E_SYNTAX;
		goto __exit;
	}

#ifdef _DEBUG
	LogFormated(L"Dyn handler: %s%s\n", pArray[0]);

	for (int i = 1; i < count; i++)
		LogFormated(L"Dyn arg %d: %s\n", i-1, pArray[i]);
#endif //_DEBUG

	m_pStream = CBrowserCoreInterface::CallDynProtCB(m_URL, pArray[0], &pArray[1], count-1, &m_POSTValues);

	if (m_pStream){
		m_pStream->Stat(&stat, 0);
		m_pProtocolSink->ReportData(BSCF_LASTDATANOTIFICATION | BSCF_DATAFULLYAVAILABLE, stat.cbSize.LowPart, stat.cbSize.LowPart);
		m_pProtocolSink->ReportResult(S_OK, 0, 0);
		hr = S_OK;
		goto __exit;
	}
	

__exit:
	g_pFreeer(pNewURL);
	return hr;
}

HRESULT CDynProtocol::GetURLComponents(wchar_t* pURL, wchar_t** ppParsed, int *pCount){
	int c;
	if (wcsnicmp(pURL, L"dyn://", (sizeof(L"dyn://")/sizeof(wchar_t))-1) != 0) return MK_E_SYNTAX;
	pURL = pURL+(sizeof(L"dyn://")/sizeof(wchar_t))-1;

	if (StrLenW(pURL) == 0) return MK_E_SYNTAX;
	
	c = TokenizeW(pURL, ppParsed, *pCount, L"/\\", NULL, FALSE, FALSE, FALSE, TRUE);

	*pCount = c;


	return S_OK;
}

/*
IUnknown
*/

HRESULT STDMETHODCALLTYPE CDynProtocol::QueryInterface(REFIID riid, void **ppvObject){
	if (!ppvObject) return E_POINTER;

	if (memcmp(&riid, &IID_IUnknown, sizeof(IID)) == 0)
		*ppvObject = (IUnknown*)this;
	else if (memcmp(&riid, &IID_IInternetProtocol, sizeof(IID)) == 0 || memcmp(&riid, &IID_IInternetProtocolRoot, sizeof(IID)) == 0)
		*ppvObject = (IInternetProtocol*)this;
	else{
#ifdef _DEBUG
		//79EAC9EB-BAF9-11CE-8C82-00AA004BA90B: IInternetPriority
		//C7A98E66-1010-492C-A1C8-C809E1F75905: IInternetProtocolEx
		LogFormated(L"Unknown IID: ");
		LogIID(&riid);
		LogFormated(L"\n");
#endif //_DEBUG
		return E_NOINTERFACE;
	}

	((IUnknown *)*ppvObject)->AddRef();
	return S_OK;
}

ULONG STDMETHODCALLTYPE CDynProtocol::AddRef(){
	return ++m_RefCount;
}

ULONG STDMETHODCALLTYPE CDynProtocol::Release(){
	ULONG refcount;
	refcount = --m_RefCount;
	if (m_RefCount == 0)
		delete this;
	return refcount;
}


/*
IInternetProtocolRoot
*/

HRESULT STDMETHODCALLTYPE CDynProtocol::Start(LPCWSTR szUrl, IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo, DWORD grfPI, HANDLE_PTR dwReserved){

	HRESULT hr;

	PROTOCOLDATA protdata;

	DWORD size;

	hr = S_OK;
#ifdef _DEBUG
	LogFormated(L"-> %s\n", szUrl);
#endif //_DEBUG

	if (!(grfPI & PI_PARSE_URL)){
		if (m_pProtocolSink) m_pProtocolSink->Release();
		if (pOIProtSink) (m_pProtocolSink = pOIProtSink)->AddRef();
		
		if (m_pBindInfo) m_pBindInfo->Release();
		if (pOIBindInfo) (m_pBindInfo = pOIBindInfo)->AddRef();
		
	}

	m_BindInfo.cbSize = sizeof(BINDINFO);

	hr = pOIBindInfo->GetBindInfo(&m_BindFlags, &m_BindInfo);

	m_POSTValues.ParseFromBindInfo(pOIBindInfo);

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

HRESULT STDMETHODCALLTYPE CDynProtocol::Continue(PROTOCOLDATA *pProtocolData){
	if (pProtocolData->dwState == BIND_ASYNC)
		return DoBind();
	return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CDynProtocol::Abort(HRESULT hrReason, DWORD dwOptions){
	return m_pProtocolSink->ReportResult(E_ABORT, 0, NULL);
}

HRESULT STDMETHODCALLTYPE CDynProtocol::Terminate(DWORD dwOptions){
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

HRESULT STDMETHODCALLTYPE CDynProtocol::Suspend(){
	return E_NOTIMPL; //Not available
}

HRESULT STDMETHODCALLTYPE CDynProtocol::Resume(){
	return E_NOTIMPL; //Not available
}


/*
IInternetProtocol
*/

HRESULT STDMETHODCALLTYPE CDynProtocol::Read(void *pv, ULONG cb, ULONG *pcbRead){
	HRESULT hr;
	if (!m_pStream) return S_FALSE;
	hr = m_pStream->Read(pv, cb, &cb);
	if (pcbRead) *pcbRead = cb;
	if (hr != S_OK) return S_FALSE;
	return cb ? S_OK : S_FALSE;
}

HRESULT STDMETHODCALLTYPE CDynProtocol::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition){
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDynProtocol::LockRequest(DWORD dwOptions){
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDynProtocol::UnlockRequest(){
	return E_NOTIMPL;
}

/*
CCallProtocolClassFactory
*/

CCallProtocolClassFactory::CCallProtocolClassFactory(){
	m_RefCount = 1;
}

CCallProtocolClassFactory::~CCallProtocolClassFactory(){
}

HRESULT CCallProtocolClassFactory::UnwrapSpecialUrl(LPCWSTR pchUrl, wchar_t *pBuf){
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

HRESULT STDMETHODCALLTYPE CCallProtocolClassFactory::QueryInterface(REFIID riid, void **ppvObject){
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

ULONG STDMETHODCALLTYPE CCallProtocolClassFactory::AddRef(){
	return ++m_RefCount;
}

ULONG STDMETHODCALLTYPE CCallProtocolClassFactory::Release(){
	//Will be used for static vars, so no "delete this;"
	return --m_RefCount;
}


HRESULT STDMETHODCALLTYPE CCallProtocolClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject){
	*ppvObject = new CCallProtocol();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CCallProtocolClassFactory::LockServer(BOOL fLock){
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CCallProtocolClassFactory::ParseUrl(LPCWSTR pwzUrl, PARSEACTION ParseAction, DWORD dwParseFlags, LPWSTR pwzResult, DWORD cchResult, DWORD *pcchResult, DWORD dwReserved){
	HRESULT hr;
	hr = INET_E_DEFAULT_ACTION;
	wchar_t unwrapped[URL_BUF_SIZE];
	BSTR temp;
	DWORD len;

	if (!pcchResult || !pwzResult){
		hr = E_POINTER;
		goto __cleanup;
	}
#ifdef _DEBUG
	LogFormated(L"Parse action: %d\n", ParseAction);
#endif //_DEBUG

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

HRESULT STDMETHODCALLTYPE CCallProtocolClassFactory::CombineUrl(LPCWSTR pwzBaseUrl, LPCWSTR pwzRelativeUrl, DWORD dwCombineFlags, LPWSTR pwzResult, DWORD cchResult, DWORD *pcchResult, DWORD dwReserved){
	HRESULT hr;
	const wchar_t *pSrc;
	hr = INET_E_DEFAULT_ACTION;

	if (pwzBaseUrl && (pSrc = StrrChrW(pwzBaseUrl, '\1')))
		hr = CoInternetCombineUrl(++pSrc, pwzRelativeUrl, URL_ESCAPE_SPACES_ONLY, pwzResult, cchResult, pcchResult, 0);
	return hr;
}

HRESULT STDMETHODCALLTYPE CCallProtocolClassFactory::CompareUrl(LPCWSTR pwzUrl1, LPCWSTR pwzUrl2, DWORD dwCompareFlags){
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCallProtocolClassFactory::QueryInfo(LPCWSTR pwzUrl, QUERYOPTION QueryOption, DWORD dwQueryFlags, LPVOID pBuffer, DWORD cbBuffer, DWORD *pcbBuf, DWORD dwReserved){
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
		case QUERY_CAN_NAVIGATE:
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
CCallProtocol
*/

CCallProtocol::CCallProtocol(){
	m_RefCount = 1;
	m_pProtocolSink = NULL;
	m_pBindInfo = NULL;
}

CCallProtocol::~CCallProtocol(){
	if (m_pBindInfo) m_pBindInfo->Release();
	if (m_pProtocolSink) m_pProtocolSink->Release();
}

HRESULT CCallProtocol::DoBind(){
	HRESULT hr;
	int count;
	count = CALL_MAX_PARAMS+1;
	wchar_t *pArray[CALL_MAX_PARAMS+1]; //Maximum params + dll and function name
	wchar_t *pNewURL;

	pNewURL = StrDupW(m_URL);

	if (!pNewURL) return E_OUTOFMEMORY;
	
	hr = GetURLComponents(pNewURL, pArray, &count);

	if (hr != S_OK) goto __exit;

	if (count < 1){
		hr = MK_E_SYNTAX;
		goto __exit;
	}

#ifdef _DEBUG
	LogFormated(L"Call: function: %s\n", pArray[0]);

	for (int i = 1; i < count; i++)
		LogFormated(L"Call arg %d: %s\n", i-1, pArray[i]);

	
	/*for (i = 0; i < datacount; i++)
		LogFormated(L"Data %d: %s - %s - %d\n", i, PostDatas[i].m_pName, PostDatas[i].m_pValue, PostDatas[i].m_Size);//*/
#endif //_DEBUG

	CBrowserCoreInterface::CallCallProtCB(m_URL, pArray[0], &pArray[1], count-1, &m_POSTValues);
	

__exit:
	g_pFreeer(pNewURL);
	return hr;
}

HRESULT CCallProtocol::GetURLComponents(wchar_t* pURL, wchar_t** ppParsed, int *pCount){
	int c;
	if (wcsnicmp(pURL, L"call://", (sizeof(L"call://")/sizeof(wchar_t))-1) != 0) return MK_E_SYNTAX;
	pURL = pURL+(sizeof(L"call://")/sizeof(wchar_t))-1;

	if (StrLenW(pURL) == 0) return MK_E_SYNTAX;
	
	c = TokenizeW(pURL, ppParsed, *pCount, L"/\\", NULL, FALSE, FALSE, FALSE, TRUE);

	*pCount = c;


	return S_OK;
}

/*
IUnknown
*/

HRESULT STDMETHODCALLTYPE CCallProtocol::QueryInterface(REFIID riid, void **ppvObject){
	if (!ppvObject) return E_POINTER;

	if (memcmp(&riid, &IID_IUnknown, sizeof(IID)) == 0)
		*ppvObject = (IUnknown*)this;
	else if (memcmp(&riid, &IID_IInternetProtocol, sizeof(IID)) == 0 || memcmp(&riid, &IID_IInternetProtocolRoot, sizeof(IID)) == 0)
		*ppvObject = (IInternetProtocol*)this;
	else{
#ifdef _DEBUG
		//79EAC9EB-BAF9-11CE-8C82-00AA004BA90B: IInternetPriority
		//C7A98E66-1010-492C-A1C8-C809E1F75905: IInternetProtocolEx
		LogFormated(L"Unknown IID: ");
		LogIID(&riid);
		LogFormated(L"\n");
#endif //_DEBUG
		return E_NOINTERFACE;
	}

	((IUnknown *)*ppvObject)->AddRef();
	return S_OK;
}

ULONG STDMETHODCALLTYPE CCallProtocol::AddRef(){
	return ++m_RefCount;
}

ULONG STDMETHODCALLTYPE CCallProtocol::Release(){
	ULONG refcount;
	refcount = --m_RefCount;
	if (m_RefCount == 0)
		delete this;
	return refcount;
}


/*
IInternetProtocolRoot
*/

HRESULT STDMETHODCALLTYPE CCallProtocol::Start(LPCWSTR szUrl, IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo, DWORD grfPI, HANDLE_PTR dwReserved){

	HRESULT hr;

	PROTOCOLDATA protdata;

	DWORD size;


	hr = S_OK;
#ifdef _DEBUG
	LogFormated(L"-> %s\n", szUrl);
#endif //_DEBUG

	if (!(grfPI & PI_PARSE_URL)){
		if (m_pProtocolSink) m_pProtocolSink->Release();
		if (pOIProtSink) (m_pProtocolSink = pOIProtSink)->AddRef();
		
		if (m_pBindInfo) m_pBindInfo->Release();
		if (pOIBindInfo) (m_pBindInfo = pOIBindInfo)->AddRef();
		
	}

	m_BindInfo.cbSize = sizeof(BINDINFO);

	pOIBindInfo->GetBindInfo(&m_BindFlags, &m_BindInfo);
	m_POSTValues.ParseFromBindInfo(pOIBindInfo);


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

HRESULT STDMETHODCALLTYPE CCallProtocol::Continue(PROTOCOLDATA *pProtocolData){
	if (pProtocolData->dwState == BIND_ASYNC)
		return DoBind();
	return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CCallProtocol::Abort(HRESULT hrReason, DWORD dwOptions){
	return m_pProtocolSink->ReportResult(E_ABORT, 0, NULL);
}

HRESULT STDMETHODCALLTYPE CCallProtocol::Terminate(DWORD dwOptions){
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

HRESULT STDMETHODCALLTYPE CCallProtocol::Suspend(){
	return E_NOTIMPL; //Not available
}

HRESULT STDMETHODCALLTYPE CCallProtocol::Resume(){
	return E_NOTIMPL; //Not available
}


/*
IInternetProtocol
*/

HRESULT STDMETHODCALLTYPE CCallProtocol::Read(void *pv, ULONG cb, ULONG *pcbRead){
	return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CCallProtocol::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition){
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCallProtocol::LockRequest(DWORD dwOptions){
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCallProtocol::UnlockRequest(){
	return E_NOTIMPL;
}
