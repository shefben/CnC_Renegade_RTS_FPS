#ifndef _BROWSER_H_
#define _BROWSER_H_

#include <windows.h>
#include <mshtml.h>
#include <mshtmhst.h>
#include <Exdisp.h>
#include "BrowserAPI.h"
#include "../Shared/Lists.h"

#define NOTIMPLEMENTED return(E_NOTIMPL)

class CBrowser;

class CStorage;
class COleInPlaceFrame;
class COleInPlaceSite;
class COleClientSite;
class OleItemContainer;
class CBrowserEvents2;

#define BROWSER_WND_CLASS	(L"BRWSR_WND_CLS")


/*template<typename T>
class CGlobalAllocator {
public:
	T*	m_pObject;
	CGlobalAllocator();
	~CGlobalAllocator();
};

template<typename T> CGlobalAllocator<T>::CGlobalAllocator(){
	m_pObject = (T*)GlobalAlloc(GMEM_FIXED, sizeof(T));
}

template<typename T> CGlobalAllocator<T>::~CGlobalAllocator(){
	if (m_pObject){
		GlobalFree((HGLOBAL)m_pObject);
		m_pObject = NULL;
	}
}//*/

class CGlobalContainer {
public:
	void* operator new(size_t size);
	void operator delete(void *pPtr);
};

class CStorage : public IStorage {
public:
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
	virtual ULONG STDMETHODCALLTYPE AddRef();
	virtual ULONG STDMETHODCALLTYPE Release();
	virtual HRESULT STDMETHODCALLTYPE CreateStream(const WCHAR *pwcsName, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStream **ppstm);
	virtual HRESULT STDMETHODCALLTYPE OpenStream(const WCHAR * pwcsName, void *reserved1, DWORD grfMode, DWORD reserved2, IStream **ppstm);
	virtual HRESULT STDMETHODCALLTYPE CreateStorage(const WCHAR *pwcsName, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStorage **ppstg);
	virtual HRESULT STDMETHODCALLTYPE OpenStorage(const WCHAR * pwcsName, IStorage * pstgPriority, DWORD grfMode, SNB snbExclude, DWORD reserved, IStorage **ppstg);
	virtual HRESULT STDMETHODCALLTYPE CopyTo(DWORD ciidExclude, IID const *rgiidExclude, SNB snbExclude,IStorage *pstgDest);
	virtual HRESULT STDMETHODCALLTYPE MoveElementTo(const OLECHAR *pwcsName,IStorage * pstgDest, const OLECHAR *pwcsNewName, DWORD grfFlags);
	virtual HRESULT STDMETHODCALLTYPE Commit(DWORD grfCommitFlags);
	virtual HRESULT STDMETHODCALLTYPE Revert();
	virtual HRESULT STDMETHODCALLTYPE EnumElements(DWORD reserved1, void * reserved2, DWORD reserved3, IEnumSTATSTG ** ppenum);
	virtual HRESULT STDMETHODCALLTYPE DestroyElement(const OLECHAR *pwcsName);
	virtual HRESULT STDMETHODCALLTYPE RenameElement(const WCHAR *pwcsOldName, const WCHAR *pwcsNewName);
	virtual HRESULT STDMETHODCALLTYPE SetElementTimes(const WCHAR *pwcsName, FILETIME const *pctime, FILETIME const *patime, FILETIME const *pmtime);
	virtual HRESULT STDMETHODCALLTYPE SetClass(REFCLSID clsid);
	virtual HRESULT STDMETHODCALLTYPE SetStateBits(DWORD grfStateBits, DWORD grfMask);
	virtual HRESULT STDMETHODCALLTYPE Stat(STATSTG * pstatstg, DWORD grfStatFlag);
};

class COleInPlaceFrame : public IOleInPlaceFrame {
private:
	CBrowser*	m_pBrowser;
public:
	COleInPlaceFrame();
	void SetBrowser(CBrowser* pBrowser) { m_pBrowser = pBrowser; };
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();
	HRESULT STDMETHODCALLTYPE GetWindow(HWND FAR* lphwnd);
	HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);
	HRESULT STDMETHODCALLTYPE GetBorder(LPRECT lprectBorder);
	HRESULT STDMETHODCALLTYPE RequestBorderSpace(LPCBORDERWIDTHS pborderwidths);
	HRESULT STDMETHODCALLTYPE SetBorderSpace(LPCBORDERWIDTHS pborderwidths);
	HRESULT STDMETHODCALLTYPE SetActiveObject(IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName);
	HRESULT STDMETHODCALLTYPE InsertMenus(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths);
	HRESULT STDMETHODCALLTYPE SetMenu(HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject);
	HRESULT STDMETHODCALLTYPE RemoveMenus(HMENU hmenuShared);
	HRESULT STDMETHODCALLTYPE SetStatusText(LPCOLESTR pszStatusText);
	HRESULT STDMETHODCALLTYPE EnableModeless(BOOL fEnable);
	HRESULT STDMETHODCALLTYPE TranslateAccelerator(LPMSG lpmsg, WORD wID);

	HWND	m_hWindow;
};

class CDocHostUIHandler : public IDocHostUIHandler {
private:
	CBrowser*	m_pBrowser;
public:
	CDocHostUIHandler();
	void SetBrowser(CBrowser* pBrowser) { m_pBrowser = pBrowser; };
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();
	HRESULT STDMETHODCALLTYPE ShowContextMenu(DWORD dwID, POINT *ppt, IUnknown *pcmdtReserved, IDispatch *pdispReserved);
	HRESULT STDMETHODCALLTYPE GetHostInfo(DOCHOSTUIINFO *pInfo);
	HRESULT STDMETHODCALLTYPE ShowUI(DWORD dwID, IOleInPlaceActiveObject *pActiveObject, IOleCommandTarget *pCommandTarget, IOleInPlaceFrame *pFrame, IOleInPlaceUIWindow *pDoc);
	HRESULT STDMETHODCALLTYPE HideUI();
	HRESULT STDMETHODCALLTYPE UpdateUI();
	HRESULT STDMETHODCALLTYPE EnableModeless(BOOL fEnable);
	HRESULT STDMETHODCALLTYPE OnDocWindowActivate(BOOL fActivate);
	HRESULT STDMETHODCALLTYPE OnFrameWindowActivate(BOOL fActivate);
	HRESULT STDMETHODCALLTYPE ResizeBorder(LPCRECT prcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow);
	HRESULT STDMETHODCALLTYPE TranslateAccelerator(LPMSG lpMsg, const GUID *pguidCmdGroup, DWORD nCmdID);
	HRESULT STDMETHODCALLTYPE GetOptionKeyPath(LPOLESTR *pchKey, DWORD dw);
	HRESULT STDMETHODCALLTYPE GetDropTarget(IDropTarget *pDropTarget, IDropTarget **ppDropTarget);
	HRESULT STDMETHODCALLTYPE GetExternal(IDispatch **ppDispatch);
	HRESULT STDMETHODCALLTYPE TranslateUrl(DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut);
	HRESULT STDMETHODCALLTYPE FilterDataObject(IDataObject *pDO, IDataObject **ppDORet);

	IDocHostUIHandler*	m_pOriginalHandler;
};

class COleInPlaceSite : public IOleInPlaceSite {
private:
	CBrowser*	m_pBrowser;
public:
	COleInPlaceSite();
	void SetBrowser(CBrowser* pBrowser) { m_pBrowser = pBrowser; };
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();
	HRESULT STDMETHODCALLTYPE GetWindow(HWND FAR* lphwnd);
	HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);
	HRESULT STDMETHODCALLTYPE CanInPlaceActivate();
	HRESULT STDMETHODCALLTYPE OnInPlaceActivate();
	HRESULT STDMETHODCALLTYPE OnUIActivate();
	HRESULT STDMETHODCALLTYPE GetWindowContext(LPOLEINPLACEFRAME FAR* lplpFrame,LPOLEINPLACEUIWINDOW FAR* lplpDoc,LPRECT lprcPosRect,LPRECT lprcClipRect,LPOLEINPLACEFRAMEINFO lpFrameInfo);
	HRESULT STDMETHODCALLTYPE Scroll(SIZE scrollExtent);
	HRESULT STDMETHODCALLTYPE OnUIDeactivate(BOOL fUndoable);
	HRESULT STDMETHODCALLTYPE OnInPlaceDeactivate();
	HRESULT STDMETHODCALLTYPE DiscardUndoState();
	HRESULT STDMETHODCALLTYPE DeactivateAndUndo();
	HRESULT STDMETHODCALLTYPE OnPosRectChange(LPCRECT lprcPosRect);
	
	COleClientSite*	m_pClientSite;
};

class COleCommandTarget : public IOleCommandTarget {
private:
	CBrowser*	m_pBrowser;
public:
	COleCommandTarget();
	void SetBrowser(CBrowser* pBrowser) { m_pBrowser = pBrowser; };
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();

	HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[],OLECMDTEXT *pCmdText);
	HRESULT STDMETHODCALLTYPE Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);

	bool m_SurpressScriptErrors;
};

class COleClientSite : public IOleClientSite {
private:
	CBrowser*	m_pBrowser;
public:
	COleClientSite();
	void SetBrowser(CBrowser* pBrowser) { m_pBrowser = pBrowser; };
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();
	HRESULT STDMETHODCALLTYPE SaveObject();
	HRESULT STDMETHODCALLTYPE GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker ** ppmk);
	HRESULT STDMETHODCALLTYPE GetContainer(LPOLECONTAINER FAR* ppContainer);
	HRESULT STDMETHODCALLTYPE ShowObject();
	HRESULT STDMETHODCALLTYPE OnShowWindow(BOOL fShow);
	HRESULT STDMETHODCALLTYPE RequestNewObjectLayout();

};



class COleItemContainer : public IOleItemContainer {
private:
	CBrowser*	m_pBrowser;
public:
	COleItemContainer();
	void SetBrowser(CBrowser* pBrowser) { m_pBrowser = pBrowser; };
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();
    HRESULT STDMETHODCALLTYPE ParseDisplayName(IBindCtx *pbc, LPOLESTR pszDisplayName, ULONG *pchEaten, IMoniker **ppmkOut);
	HRESULT STDMETHODCALLTYPE EnumObjects(DWORD grfFlags, IEnumUnknown **ppenum);
	HRESULT STDMETHODCALLTYPE LockContainer(BOOL fLock);
	HRESULT STDMETHODCALLTYPE GetObject(LPOLESTR pszItem, DWORD dwSpeedNeeded, IBindCtx *pbc, REFIID riid, void **ppvObject);
	HRESULT STDMETHODCALLTYPE GetObjectStorage(LPOLESTR pszItem, IBindCtx *pbc, REFIID riid, void **ppvStorage);
	HRESULT STDMETHODCALLTYPE IsRunning(LPOLESTR pszItem);
};

class CBrowserEvents2 : public IBrowserEvents2 {
public:
	virtual void BeforeNavigate2(IDispatch *pDisp, BSTR url, unsigned long Flags, BSTR TargetFrameName, VARIANT *PostData, BSTR Headers, BOOL *Cancel);
	virtual void ClientToHostWindow(long *CX, long *CY);
	virtual void CommandStateChange(long Command, BOOL Enable);
	virtual void DocumentComplete(IDispatch *pDisp, BSTR URL);
	virtual void DownloadBegin();
	virtual void DownloadComplete();
	virtual void FileDownload(BOOL ActiveDocument, BOOL *Cancel);
	virtual void NavigateComplete2(IDispatch *pDisp, BSTR URL);
	virtual void NavigateError(IDispatch *pDisp, BSTR URL, BSTR TargetFrameName, DWORD StatusCode, BOOL *Cancel);
	virtual void NewProcess(long lCauseFlag, IDispatch *pWB2, BOOL *Cancel);
	virtual void NewWindow2(IDispatch **ppDisp, BOOL *Cancel);
	virtual void NewWindow3(IDispatch **ppDisp, BOOL *Cancel, DWORD dwFlags, BSTR bstrUrlContext, BSTR bstrUrl);
	virtual void OnFullScreen(BOOL FullScreen);
	virtual void OnMenuBar(BOOL MenuBar);
	virtual void OnQuit();
	virtual void OnStatusBar(BOOL StatusBar);
	virtual void OnTheaterMode(BOOL TheaterMode);
	virtual void OnToolBar(BOOL ToolBar);
	virtual void OnVisible(BOOL Visible);
	virtual void PrintTemplateInstantiation(IDispatch *pDisp);
	virtual void PrintTemplateTeardown(IDispatch *pDisp);
	virtual void PrivacyImpactedStateChange(BOOL PrivacyImpacted);
	virtual void ProgressChange(long Progress, long ProgressMax);
	virtual void PropertyChange(BSTR szProperty);
	virtual void RedirectXDomainBlocked(IDispatch *pDisp, BSTR StartURL, BSTR RedirectURL, BSTR Frame, DWORD StatusCode);
	virtual void SetPhishingFilterStatus(long PhishingFilterStatus);
	virtual void SetSecureLockIcon(DWORD SecureLockIcon);
	virtual void StatusTextChange(BSTR Text);
	virtual void ThirdPartyUrlBlocked(BSTR URL, DWORD dwCount);
	virtual void TitleChange(BSTR Text);
	//virtual DWebBrowserEvents2::UpdatePageStatus;
	virtual void WindowClosing(BOOL IsChildWindow, BOOL *Cancel);
	virtual void WindowSetHeight(long Height);
	virtual void WindowSetLeft(long Left);
	virtual void WindowSetResizable(BOOL Resizable);
	virtual void WindowSetTop(long Top);
	virtual void WindowSetWidth(long Width);
	virtual void WindowStateChanged(DWORD dwFlags, DWORD dwValidFlagsMask);
};


class CBrowserEventDispatch : public IDispatch {
private:
	ULONG				m_RefCount;
	IBrowserEvents2*	m_pCB;
public:
	CBrowserEventDispatch();
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,void __RPC_FAR *__RPC_FAR *ppvObject);
	virtual ULONG STDMETHODCALLTYPE AddRef();
	virtual ULONG STDMETHODCALLTYPE Release();
	virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo);
	virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo,LCID lcid,ITypeInfo **ppTInfo);
	virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid,LPOLESTR *rgszNames,UINT cNames,LCID lcid,DISPID *rgDispId);
	virtual HRESULT STDMETHODCALLTYPE Invoke(DISPID dispIdMember,REFIID riid,LCID lcid,WORD wFlags,DISPPARAMS *pDispParams,VARIANT *pVarResult,EXCEPINFO *pExcepInfo,UINT *puArgErr);


	HRESULT BeforeNavigate2(DISPPARAMS *pParams);
	HRESULT ClientToHostWindow(DISPPARAMS *pParams);
	HRESULT CommandStateChange(DISPPARAMS *pParams);
	HRESULT DocumentComplete(DISPPARAMS *pParams);
	HRESULT DownloadBegin(DISPPARAMS *pParams);
	HRESULT DownloadComplete(DISPPARAMS *pParams);
	HRESULT FileDownload(DISPPARAMS *pParams);
	HRESULT NavigateComplete2(DISPPARAMS *pParams);
	HRESULT NavigateError(DISPPARAMS *pParams);
	HRESULT NewProcess(DISPPARAMS *pParams);
	HRESULT NewWindow2(DISPPARAMS *pParams);
	HRESULT NewWindow3(DISPPARAMS *pParams);
	HRESULT OnFullScreen(DISPPARAMS *pParams);
	HRESULT OnMenuBar(DISPPARAMS *pParams);
	HRESULT OnQuit(DISPPARAMS *pParams);
	HRESULT OnStatusBar(DISPPARAMS *pParams);
	HRESULT OnTheaterMode(DISPPARAMS *pParams);
	HRESULT OnToolBar(DISPPARAMS *pParams);
	HRESULT OnVisible(DISPPARAMS *pParams);
	HRESULT PrintTemplateInstantiation(DISPPARAMS *pParams);
	HRESULT PrintTemplateTeardown(DISPPARAMS *pParams);
	HRESULT PrivacyImpactedStateChange(DISPPARAMS *pParams);
	HRESULT ProgressChange(DISPPARAMS *pParams);
	HRESULT PropertyChange(DISPPARAMS *pParams);
	HRESULT RedirectXDomainBlocked(DISPPARAMS *pParams);
	HRESULT SetPhishingFilterStatus(DISPPARAMS *pParams);
	HRESULT SetSecureLockIcon(DISPPARAMS *pParams);
	HRESULT StatusTextChange(DISPPARAMS *pParams);
	HRESULT ThirdPartyUrlBlocked(DISPPARAMS *pParams);
	HRESULT TitleChange(DISPPARAMS *pParams);
	//DWebBrowserEvents2::UpdatePageStatus PURE;
	HRESULT WindowClosing(DISPPARAMS *pParams);
	HRESULT WindowSetHeight(DISPPARAMS *pParams);
	HRESULT WindowSetLeft(DISPPARAMS *pParams);
	HRESULT WindowSetResizable(DISPPARAMS *pParams);
	HRESULT WindowSetTop(DISPPARAMS *pParams);
	HRESULT WindowSetWidth(DISPPARAMS *pParams);
	HRESULT WindowStateChanged(DISPPARAMS *pParams);

	void SetCallback(IBrowserEvents2 *pCB);
};

class CBrowser : public IBrowser, public CBrowserEvents2, public CBrowserEventDispatch { //, public IServiceProvider {

	friend class COleInPlaceFrame;
	friend class COleInPlaceSite;
	friend class COleClientSite;
	friend class OleItemContainer;
private:
	COleInPlaceFrame		m_Frame;
	CDocHostUIHandler		m_DocHost;
	COleInPlaceSite			m_InPlaceSite;
	COleCommandTarget		m_CommandTarget;
	COleClientSite			m_Site;
	COleItemContainer		m_Container;

	
	//CGlobalAllocator<COleClientSite>	m_Site;
	IWebBrowser2*			m_pBrowser;
	IOleObject*				m_pBrowserObject;
	IOleInPlaceObject*		m_pInPlaceObject;
	COleItemContainer*		m_pContainer;
	HWND					m_hWnd;
	HWND					m_hWndBrowser;
	LONG_PTR				m_pBrowserUserdata;

	WNDPROC					m_OldProc;
	HWND					m_hServerWindow;

	DWORD					m_Cookie;

	static HINSTANCE		s_hInstance;
	static SList<CBrowser>	s_BrowserList;

	static CBrowser* BrowserFromHWND(HWND hWnd);

	static BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam);
	HWND FindServerWindow();


	void SwitchContext(bool _CBrowser, HWND hWnd);

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK BrowserWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void HookWindow();
public:
	static void InitBrowser(HINSTANCE hInstance);
	static void DeInitBrowser();

	void DocumentComplete(IDispatch *pDisp, BSTR URL);

public:

	
	//HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void **ppvObject);
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();

	CBrowser();
	virtual ~CBrowser();
	virtual void SetHost(HWND hParent);
	virtual void Resize(RECT *pRect);
	virtual bool Navigate(const wchar_t *pURL);
	virtual bool Forward();
	virtual bool Back();
	virtual bool Home();
	virtual bool SurpressScriptErrors(bool surpress);
	virtual bool AddEventCallback(IDispatch* pCallback, DWORD *pCookie);
	virtual bool RemoveEventCallback(DWORD Cookie);
};

class CBrowserEvents : public IBrowserEvents {
	virtual void BeforeNavigate(IDispatch *pDisp, VARIANT *url,VARIANT *Flags,VARIANT *TargetFrameName,VARIANT *PostData,VARIANT *Headers,VARIANT_BOOL *Cancel);
	virtual void CommandStateChange(long Command,VARIANT_BOOL Enable);
	virtual void DownloadBegin();
	virtual void DownloadComplete();
	virtual void NavigateComplete(IDispatch *pDisp,VARIANT *URL);
	virtual void NewWindow(IDispatch **ppDisp,VARIANT_BOOL *Cancel);
	virtual void OnQuit();
	virtual void ProgressChange(long Progress,long ProgressMax);
	virtual void PropertyChange(BSTR szProperty);
	virtual void StatusTextChange(BSTR Text);
	virtual void TitleChange(BSTR Text);
	//virtual DWebBrowserEvents::WindowActivate Event;
	//virtual DWebBrowserEvents::WindowMove Event;
	//virtual DWebBrowserEvents::WindowResize Event;
};
class CInternalBrowserEventDispatch : public IDispatch {
private:
	CBrowser* pBrowser;
};

class CBrowserCoreInterface : public IBrowserCoreInterface {
public:
	static IStream* CallDynProtCB(const wchar_t *pURL, const wchar_t *pHandler, const wchar_t* const* ppArgs, int argc, IPOSTValues *pPOST);
	static void CallCallProtCB(const wchar_t *pURL, const wchar_t *pHandler, const wchar_t* const* ppArgs, int argc, IPOSTValues *pPOST);
private:
	static pfnDynProt	s_pfnDynProt;
	static pfnCallProt	s_pfnCallProt;
	static DWORD		s_InitFlags;
public:
	CBrowserCoreInterface();
	~CBrowserCoreInterface();
	void Intialize(DWORD flags, HINSTANCE hInstance) const;
	IBrowser* CreateIBrowser() const;
	void RegisterDynProtCB(pfnDynProt _pfnDynProt) const;
	void RegisterCallProtCB(pfnCallProt _pfnCallProt) const;
};


#endif //_BROWSER_H_
