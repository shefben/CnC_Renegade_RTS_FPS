#ifndef _BROWSERAPI_H_
#define _BROWSERAPI_H_

#include <windows.h>
#include <urlmon.h>
#include "../Shared/BaseTypes.h"

#define BROWSER_REGISTER_DLL_PROT	(1)
#define BROWSER_REGISTER_DYN_PROT	(2)
#define BROWSER_REGISTER_CALL_PROT	(4)

IInternetSession* GetInternetSession();

typedef struct _POSTDATA {
	const wchar_t*	m_pName;
	union {
		const wchar_t*	m_pValue;
		unsigned char*	m_pData;
	};
	size_t			m_Size;
} POSTDATA, *LPPOSTDATA;



#define POST_VALUE_TYPE_FAIL	(-1)
#define POST_VALUE_TYPE_STRING	(0)
#define POST_VALUE_TYPE_BINARY	(1)
#define POST_VALUE_TYPE_STREAM	(2)

interface IPOSTValues {
public:
	virtual ~IPOSTValues() {};

	virtual int GetValueCount() PURE;
	virtual const wchar_t* GetValueName(int index) PURE;
	virtual int GetValueType(int index) PURE;
	virtual size_32 GetValueSize(int index) PURE;
	virtual const void* GetValueBinary(int index) PURE;
	virtual const wchar_t* GetValueString(int index) PURE;
	virtual IStream* GetStream(int index) PURE;

	virtual int GetValueTypeByName(const wchar_t* pName) PURE;
	virtual size_32 GetValueSizeByName(const wchar_t* pName) PURE;
	virtual const void* GetValueBinaryByName(const wchar_t* pName) PURE;
	virtual const wchar_t* GetValueStringByName(const wchar_t* pName) PURE;
	virtual IStream* GetStreamByName(const wchar_t* pName) PURE;
};

interface IDynStream {
public:
	virtual void Release() PURE;
	virtual HRESULT Read(void *pBuf, ULONG cb, ULONG *pcb) PURE;
};

interface IBrowser {
public:
	virtual ~IBrowser() {};
	virtual void SetHost(HWND hParent) = NULL;
	virtual void Resize(RECT *pRect) = NULL;
	virtual bool Navigate(const wchar_t *pURL) = NULL;
	virtual bool Forward() = NULL;
	virtual bool Back() = NULL;
	virtual bool Home() = NULL;
	virtual bool SurpressScriptErrors(bool surpress) = NULL;
	virtual bool AddEventCallback(IDispatch* pCallback, DWORD *pCookie) = NULL;
	virtual bool RemoveEventCallback(DWORD Cookie) = NULL;
};

interface IBrowserEvents {
	virtual void BeforeNavigate(IDispatch *pDisp, VARIANT *url,VARIANT *Flags,VARIANT *TargetFrameName,VARIANT *PostData,VARIANT *Headers,VARIANT_BOOL *Cancel) PURE;
	virtual void CommandStateChange(long Command,VARIANT_BOOL Enable) PURE;
	virtual void DownloadBegin() PURE;
	virtual void DownloadComplete() PURE;
	virtual void NavigateComplete(IDispatch *pDisp,VARIANT *URL) PURE;
	virtual void NewWindow(IDispatch **ppDisp,VARIANT_BOOL *Cancel) PURE;
	virtual void OnQuit() PURE;
	virtual void ProgressChange(long Progress,long ProgressMax) PURE;
	virtual void PropertyChange(BSTR szProperty) PURE;
	virtual void StatusTextChange(BSTR Text) PURE;
	virtual void TitleChange(BSTR Text) PURE;
	//virtual DWebBrowserEvents::WindowActivate Event PURE;
	//virtual DWebBrowserEvents::WindowMove Event PURE;
	//virtual DWebBrowserEvents::WindowResize Event PURE;
};

interface IBrowserEvents2 {
public:
	virtual void BeforeNavigate2(IDispatch *pDisp, BSTR url, unsigned long Flags, BSTR TargetFrameName, VARIANT *PostData, BSTR Headers, BOOL *Cancel) PURE;
	virtual void ClientToHostWindow(long *CX, long *CY) PURE;
	virtual void CommandStateChange(long Command, BOOL Enable) PURE;
	virtual void DocumentComplete(IDispatch *pDisp, BSTR URL) PURE;
	virtual void DownloadBegin() PURE;
	virtual void DownloadComplete() PURE;
	virtual void FileDownload(BOOL ActiveDocument, BOOL *Cancel) PURE;
	virtual void NavigateComplete2(IDispatch *pDisp, BSTR URL) PURE;
	virtual void NavigateError(IDispatch *pDisp, BSTR URL, BSTR TargetFrameName, DWORD StatusCode, BOOL *Cancel) PURE;
	virtual void NewProcess(long lCauseFlag, IDispatch *pWB2, BOOL *Cancel) PURE;
	virtual void NewWindow2(IDispatch **ppDisp, BOOL *Cancel) PURE;
	virtual void NewWindow3(IDispatch **ppDisp, BOOL *Cancel, DWORD dwFlags, BSTR bstrUrlContext, BSTR bstrUrl) PURE;
	virtual void OnFullScreen(BOOL FullScreen) PURE;
	virtual void OnMenuBar(BOOL MenuBar) PURE;
	virtual void OnQuit() PURE;
	virtual void OnStatusBar(BOOL StatusBar) PURE;
	virtual void OnTheaterMode(BOOL TheaterMode) PURE;
	virtual void OnToolBar(BOOL ToolBar) PURE;
	virtual void OnVisible(BOOL Visible) PURE;
	virtual void PrintTemplateInstantiation(IDispatch *pDisp) PURE;
	virtual void PrintTemplateTeardown(IDispatch *pDisp) PURE;
	virtual void PrivacyImpactedStateChange(BOOL PrivacyImpacted) PURE;
	virtual void ProgressChange(long Progress, long ProgressMax) PURE;
	virtual void PropertyChange(BSTR szProperty) PURE;
	virtual void RedirectXDomainBlocked(IDispatch *pDisp, BSTR StartURL, BSTR RedirectURL, BSTR Frame, DWORD StatusCode) PURE;
	virtual void SetPhishingFilterStatus(long PhishingFilterStatus) PURE;
	virtual void SetSecureLockIcon(DWORD SecureLockIcon) PURE;
	virtual void StatusTextChange(BSTR Text) PURE;
	virtual void ThirdPartyUrlBlocked(BSTR URL, DWORD dwCount) PURE;
	virtual void TitleChange(BSTR Text) PURE;
	//virtual DWebBrowserEvents2::UpdatePageStatus PURE;
	virtual void WindowClosing(BOOL IsChildWindow, BOOL *Cancel) PURE;
	virtual void WindowSetHeight(long Height) PURE;
	virtual void WindowSetLeft(long Left) PURE;
	virtual void WindowSetResizable(BOOL Resizable) PURE;
	virtual void WindowSetTop(long Top) PURE;
	virtual void WindowSetWidth(long Width) PURE;
	virtual void WindowStateChanged(DWORD dwFlags, DWORD dwValidFlagsMask) PURE;
};

typedef HRESULT (__stdcall fnStreamOpen)(BSTR pPath, void **ppStreamSrc, bool r, bool w);
typedef HRESULT (__stdcall fnStreamClose)(void *pStreamSrc);
typedef HRESULT (__stdcall fnStreamRead)(void* pStreamSrc, void *pBuf, DWORD Read, DWORD *pRead);
typedef HRESULT (__stdcall fnStreamWrite)(void* pStreamSrc, const void *pBuf, DWORD Write, DWORD *pWritten);
typedef HRESULT (__stdcall fnStreamSeek)(void* pStreamSrc, LONG Offset, int Origin, LONG *pNewOffset);
typedef HRESULT (__stdcall fnStreamSize)(void* pStreamSrc, DWORD *pSize);
typedef HRESULT	(__stdcall fnStreamTell)(void* pStreamSrc, DWORD *pOffset);
typedef HRESULT (__stdcall fnStreamSetSize)(void* pStreamSrc, DWORD Size);

typedef void (__stdcall fnDllProt)(const wchar_t*const* ppArgs, int argc);

typedef IStream* (__stdcall *pfnDynProt)(const wchar_t *pURL, const wchar_t* pHandler, const wchar_t*const* ppArgs, int argc, IPOSTValues *pPOST);
typedef void (__stdcall *pfnCallProt)(const wchar_t *pURL, const wchar_t* pHandler, const wchar_t*const* ppArgs, int argc, IPOSTValues *pPOST);


interface IBrowserCoreInterface {
public:
	virtual void Intialize(DWORD flags, HINSTANCE hInstance) const PURE;
	virtual IBrowser* CreateIBrowser() const PURE;
	virtual void RegisterDynProtCB(pfnDynProt _pfnDynProt) const PURE;
	virtual void RegisterCallProtCB(pfnCallProt _pfnCallProt) const PURE;
};


const IBrowserCoreInterface* GetBrowserCoreInterface();

#endif //_BROWSERAPI_H_
