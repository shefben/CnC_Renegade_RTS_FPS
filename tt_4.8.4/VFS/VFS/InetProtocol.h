#ifndef _INETPROTOCOL_H_
#define _INETPROTOCOL_H_

#include <windows.h>
#include <mshtml.h>
#include <mshtmhst.h>
#include <Exdisp.h>
#include <Urlmon.h>

IInternetSession* GetInternetSession();

typedef IUnknown* (__stdcall fnClassFactoryCreateInstance)();

#define URL_BUF_SIZE	(4096)

#define BIND_ASYNC		(1)

class CProtocolClassFactory : public IClassFactory, public IInternetProtocolInfo {
private:
	ULONG	m_RefCount;
public:
	CProtocolClassFactory();
	~CProtocolClassFactory();
	HRESULT UnwrapSpecialUrl(LPCWSTR pchUrl, wchar_t *pBuf);


	/*
	IUnknown
	*/

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
	virtual ULONG STDMETHODCALLTYPE AddRef();
	virtual ULONG STDMETHODCALLTYPE Release();

	/*
	IClassFactory
	*/

	virtual HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject);
	virtual HRESULT STDMETHODCALLTYPE LockServer(BOOL fLock);

	/*
	IInternetProtocolInfo
	*/
	
	virtual HRESULT STDMETHODCALLTYPE ParseUrl(LPCWSTR pwzUrl, PARSEACTION ParseAction, DWORD dwParseFlags, LPWSTR pwzResult, DWORD cchResult, DWORD *pcchResult, DWORD dwReserved);
	virtual HRESULT STDMETHODCALLTYPE CombineUrl(LPCWSTR pwzBaseUrl, LPCWSTR pwzRelativeUrl, DWORD dwCombineFlags, LPWSTR pwzResult, DWORD cchResult, DWORD *pcchResult, DWORD dwReserved);
	virtual HRESULT STDMETHODCALLTYPE CompareUrl(LPCWSTR pwzUrl1, LPCWSTR pwzUrl2, DWORD dwCompareFlags);
	virtual HRESULT STDMETHODCALLTYPE QueryInfo(LPCWSTR pwzUrl, QUERYOPTION OueryOption, DWORD dwQueryFlags, LPVOID pBuffer, DWORD cbBuffer, DWORD *pcbBuf, DWORD dwReserved);

};

class CVFSProtocol : public IInternetProtocol {
private:
	ULONG					m_RefCount;
	IInternetProtocolSink*	m_pProtocolSink;
	IInternetBindInfo*		m_pBindInfo;
	BINDINFO				m_BindInfo;
	DWORD					m_BindFlags;
	DWORD					m_Flags;
	wchar_t					m_URL[URL_BUF_SIZE];
	bool					m_Image;
	IStream*				m_pStream;
public:
	CVFSProtocol();
	~CVFSProtocol();

	HRESULT DoBind();
	HRESULT GetURLComponents(const wchar_t* pURL, wchar_t *pVFS, DWORD VFSSize, wchar_t* pSrc, DWORD SrcSize);
	/*
	IUnknown
	*/

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
	virtual ULONG STDMETHODCALLTYPE AddRef();
	virtual ULONG STDMETHODCALLTYPE Release();

	/*
	IInternetProtocolRoot
	*/

	virtual HRESULT STDMETHODCALLTYPE Start(LPCWSTR szUrl, IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo, DWORD grfPI, HANDLE_PTR dwReserved);
	virtual HRESULT STDMETHODCALLTYPE Continue(PROTOCOLDATA *pProtocolData);
	virtual HRESULT STDMETHODCALLTYPE Abort(HRESULT hrReason, DWORD dwOptions);
	virtual HRESULT STDMETHODCALLTYPE Terminate(DWORD dwOptions);
	virtual HRESULT STDMETHODCALLTYPE Suspend();
	virtual HRESULT STDMETHODCALLTYPE Resume();

	/*
	IInternetProtocol
	*/

	virtual HRESULT STDMETHODCALLTYPE Read(void *pv, ULONG cb, ULONG *pcbRead);
	virtual HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
	virtual HRESULT STDMETHODCALLTYPE LockRequest(DWORD dwOptions);
	virtual HRESULT STDMETHODCALLTYPE UnlockRequest();
};


#endif //_INETPROTOCOL_H_
