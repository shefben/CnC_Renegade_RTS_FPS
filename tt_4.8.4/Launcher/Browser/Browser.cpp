#include <windows.h>
#include "BrowserAPI.h"
#include "Browser.h"
#include "../Shared/StdLib.h"

void* CGlobalContainer::operator new(size_t size){
	return (void*)GlobalAlloc(GMEM_FIXED, size);
}
void CGlobalContainer::operator delete(void *pPtr){
	GlobalFree((HGLOBAL)pPtr);
}

CStorage	g_Storage;

HINSTANCE		CBrowser::s_hInstance = 0;

SList<CBrowser>	CBrowser::s_BrowserList;

CBrowser* CBrowser::BrowserFromHWND(HWND hWnd){
	GenericSLNode<CBrowser>* pNode;

	pNode = s_BrowserList.Get_Head();

	while (pNode){
		if (pNode->m_pData->m_hServerWindow == hWnd)
			return pNode->m_pData;

		pNode = pNode->m_pNext;
	}

	return NULL;
}

BOOL CALLBACK CBrowser::EnumChildProc(HWND hwnd, LPARAM lParam){
	wchar_t lBuf[64];
	GetClassNameW(hwnd, lBuf, 64);
	if (wcscmp(lBuf, L"Internet Explorer_Server") == 0){
		*(HWND*)lParam = hwnd;
		return false;
	} else
		return true;
}

HWND CBrowser::FindServerWindow(){
	HWND hWnd;
	hWnd = NULL;
	EnumChildWindows(m_hWnd, CBrowser::EnumChildProc, (LPARAM)&hWnd);
	return hWnd;
}

void CBrowser::SwitchContext(bool _CBrowser, HWND hWnd){
	//m_pBrowser->get_Document
	if (_CBrowser){
		SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)this);
		//SetWindowLongPtrW(hWnd, GWLP_WNDPROC, (LONG_PTR)CBrowser::BrowserWndProc);
	} else {
		SetWindowLongPtrW(hWnd, GWLP_USERDATA, m_pBrowserUserdata);
		//SetWindowLongPtrW(hWnd, GWLP_WNDPROC, (LONG_PTR)m_OldProc);
	}
}

LRESULT CALLBACK CBrowser::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	CREATESTRUCTW *pCS;
	CBrowser* pBrowser;
	IOleInPlaceActiveObject* pInPlaceObject;
	IDispatch* pDispatch;
	MSG msg;
	HRESULT hr;
	RECT r;

	hr = S_FALSE;

	if (uMsg == WM_CREATE){
		if (pCS = (CREATESTRUCTW*)lParam){
			SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)pCS->lpCreateParams);
		}
		
	} else if (uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST){
		pBrowser = (CBrowser*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
		if (pBrowser){
			if (pBrowser->m_pBrowser->get_Document(&pDispatch) == S_OK){
				if (pDispatch->QueryInterface(IID_IOleInPlaceActiveObject, (void**)&pInPlaceObject) == S_OK){
					memset(&msg, 0, sizeof(MSG));
					msg.hwnd = hWnd;
					msg.message = uMsg;
					msg.wParam = wParam;
					msg.lParam = lParam;
					hr = pInPlaceObject->TranslateAccelerator(&msg);
					pInPlaceObject->Release();
				}
				pDispatch->Release();
			}
		}
		if (hr == S_OK) return 0;
	} else if (uMsg == WM_SIZE){
		pBrowser = (CBrowser*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
		if (pBrowser && pBrowser->m_pInPlaceObject){
			r.top = r.left = 0;
			r.right = LOWORD(lParam);
			r.bottom = HIWORD(lParam);
			
			pBrowser->m_pInPlaceObject->SetObjectRects(&r, NULL);

		}

	}
	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CBrowser::BrowserWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	CBrowser* pBrowser;
	LRESULT lr;
	HRESULT hr;
	IOleInPlaceActiveObject* pInPlaceObject;
	MSG msg;

	pBrowser = BrowserFromHWND(hWnd);
	if (!pBrowser) return 0;
	//LogFormated(L"uMsg == %d %X\n", uMsg, uMsg);
	//if (uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST){
	if (uMsg == WM_COMMAND)
		uMsg = uMsg;
	if (pBrowser->m_pBrowser->QueryInterface(IID_IOleInPlaceActiveObject, (void**)&pInPlaceObject) == S_OK){
		
		memset(&msg, 0, sizeof(MSG));
		msg.hwnd = hWnd;
		msg.message = uMsg;
		msg.wParam = wParam;
		msg.lParam = lParam;
		hr = pInPlaceObject->TranslateAccelerator(&msg);

		//if (hr == S_OK) return hr;
	}
	//}
	if (uMsg == WM_PASTE)
		LogFormated(L"Got WM_COPY\n");
	//pBrowser->SwitchContext(false, hWnd);
	lr = CallWindowProcW(pBrowser->m_OldProc, hWnd, uMsg, wParam, lParam);
	
	//pBrowser->SwitchContext(true, hWnd);
	return lr;
}

void CBrowser::HookWindow(){
	HWND hWnd;

	hWnd = FindServerWindow();
	if (!hWnd || (hWnd == m_hServerWindow)) return;
	m_hServerWindow = hWnd;

	m_OldProc = (WNDPROC)GetWindowLongPtrW(m_hServerWindow, GWLP_WNDPROC);
	//m_pBrowserUserdata = GetWindowLongPtrW(m_hServerWindow, GWLP_USERDATA);
	SetWindowLongPtrW(m_hServerWindow, GWLP_WNDPROC, (LONG_PTR)CBrowser::BrowserWndProc);
	//SwitchContext(true, m_hServerWindow);
}

void CBrowser::InitBrowser(HINSTANCE hInstance){
	WNDCLASSEXW wcls;	
	
	wcls.lpszClassName = BROWSER_WND_CLASS;
	wcls.lpszMenuName = NULL;
	wcls.cbSize = sizeof(WNDCLASSEXW);
	wcls.style = 0;
	wcls.lpfnWndProc = CBrowser::WndProc;
	wcls.hIcon = NULL;// g_Application_Icon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_MAIN));
	wcls.lpszMenuName = NULL;//MAKEINTRESOURCEW(IDR_MAINMENU);
	wcls.hIconSm = NULL;
	wcls.hCursor = NULL;// LoadCursor(NULL, IDC_ARROW);
	wcls.cbClsExtra = 0;
	wcls.cbWndExtra = 0;
	wcls.hbrBackground = NULL;
	wcls.hInstance = s_hInstance = hInstance;
	RegisterClassExW(&wcls);
	
}

void CBrowser::DeInitBrowser(){
	UnregisterClassW(BROWSER_WND_CLASS, s_hInstance);
}

void CBrowser::DocumentComplete(IDispatch *pDisp, BSTR URL){
	HookWindow();
	//LogFormated(L"Document complete: %s\n", URL);
}

/*HRESULT STDMETHODCALLTYPE CBrowser::QueryService(REFGUID guidService, REFIID riid, void **ppvObject){
	if (!ppvObject) return E_INVALIDARG;
	*ppvObject = NULL;
	if (
		memcmp(&guidService, &SID_SEditCommandTarget, sizeof(REFGUID)) == 0 && 
		memcmp(&riid, &IID_IOleCommandTarget, sizeof(riid)) == 0)
		*ppvObject = (IOleCommandTarget*)this;
	
	if (!(*ppvObject))
		return E_NOINTERFACE;
	

	((IUnknown*)*ppvObject)->AddRef();
	return S_OK;

}//*/

HRESULT STDMETHODCALLTYPE CBrowser::QueryInterface(REFIID riid, void ** ppvObject){
	if (!ppvObject) return E_INVALIDARG;
	*ppvObject = NULL;
	if (memcmp(&riid, &IID_IUnknown, sizeof(GUID)) == 0)
		*ppvObject = (IUnknown*)this;
	else if(
		memcmp(&riid, &IID_IOleWindow, sizeof(GUID)) == 0 || 
		memcmp(&riid, &IID_IOleInPlaceUIWindow, sizeof(GUID)) == 0 || 
		memcmp(&riid, &IID_IOleInPlaceFrame, sizeof(GUID)) == 0
		)
		*ppvObject = (IOleInPlaceFrame*)&m_Frame;
	else if(memcmp(&riid, &IID_IOleClientSite, sizeof(GUID)) == 0)
		*ppvObject = (IOleClientSite*)&m_Site;
	else if (memcmp(&riid, &IID_IOleInPlaceSite, sizeof(GUID)) == 0)
		*ppvObject = (IOleInPlaceSite*)&m_InPlaceSite;
	else if (memcmp(&riid, &IID_IDocHostUIHandler, sizeof(GUID)) == 0)
		*ppvObject = (IDocHostUIHandler*)&m_DocHost;
	else if (memcmp(&riid, &IID_IOleCommandTarget, sizeof(GUID)) == 0)
		*ppvObject = (IOleCommandTarget*)&m_CommandTarget;
	else if (
		memcmp(&riid, &IID_IOleItemContainer, sizeof(GUID)) == 0 ||
		memcmp(&riid, &IID_IOleContainer, sizeof(GUID)) == 0 ||
		memcmp(&riid, &IID_IParseDisplayName, sizeof(GUID)) == 0
		)
		*ppvObject = (IOleItemContainer*)&m_Container;
	else if (memcmp(&riid, &IID_IDispatch, sizeof(GUID)) == 0)
		*ppvObject = (IDispatch*)this;
	else if (memcmp(&riid, &IID_IServiceProvider, sizeof(GUID)) == 0)
		*ppvObject = *ppvObject;//(IServiceProvider*)this;

	if (!(*ppvObject)){
		/*LogFormated(L"Unhandled IID: ");
		LogIID(&riid);
		LogFormated(L"\n");//*/
		return E_NOINTERFACE;
	}

	((IUnknown*)*ppvObject)->AddRef();
	return S_OK;
}

ULONG STDMETHODCALLTYPE CBrowser::AddRef(){
	return 1;
}

ULONG STDMETHODCALLTYPE CBrowser::Release(){
	return 1;
}

CBrowser::CBrowser(){
	s_BrowserList.Add_Tail(this);

	m_Frame.SetBrowser(this);
	m_DocHost.SetBrowser(this);
	m_InPlaceSite.SetBrowser(this);
	m_CommandTarget.SetBrowser(this);
	m_Site.SetBrowser(this);
	m_Container.SetBrowser(this);

	
	m_OldProc = NULL;
	m_pBrowser = NULL;
	m_hWnd = NULL;
	m_pBrowserObject = NULL;
	m_pInPlaceObject = NULL;
	m_hServerWindow = NULL;

	m_InPlaceSite.m_pClientSite = &m_Site;
	m_CommandTarget.m_SurpressScriptErrors = true;
	if (FAILED(OleCreate(CLSID_WebBrowser, IID_IOleObject, OLERENDER_DRAW, 0, &m_Site, &g_Storage, (void**)&m_pBrowserObject))){
		m_pBrowser = NULL;
		m_pBrowserObject = NULL;
		return;
	}//*/
	//hr = CoCreateInstance(CLSID_WebBrowser, NULL, CLSCTX_ALL, IID_IUnknown, (void**)(&m_pBrowserObject));

	m_pBrowserObject->SetHostNames(L"BROWSER OBJECT", NULL);

	if (FAILED(OleSetContainedObject(m_pBrowserObject, TRUE))) goto __exit;
	if (FAILED(m_pBrowserObject->QueryInterface(IID_IWebBrowser2, (void**)&m_pBrowser))) goto __exit;
	if (FAILED(m_pBrowserObject->QueryInterface(IID_IOleInPlaceObject, (void**)&m_pInPlaceObject))) goto __exit;
	m_pBrowserObject->SetClientSite(&m_Site);
	SetCallback(this);
	AddEventCallback(this, &m_Cookie);

	return;
__exit:
	if (m_pInPlaceObject)
		m_pInPlaceObject->Release();
	if (m_pBrowser)
		m_pBrowser->Release();
	if (m_pBrowserObject)
		m_pBrowserObject->Release();
	m_pInPlaceObject = NULL;
	m_pBrowser = NULL;
	m_pBrowserObject = NULL;
}

CBrowser::~CBrowser(){
	RemoveEventCallback(m_Cookie);

	if (m_pInPlaceObject)
		m_pInPlaceObject->Release();
	if (m_pBrowser)
		m_pBrowser->Release();
	if (m_pBrowserObject)
		m_pBrowserObject->Release();


	m_pInPlaceObject = NULL;
	m_pBrowser = NULL;
	m_pBrowserObject = NULL;
	s_BrowserList.Remove(this);
}

void CBrowser::SetHost(HWND hParent){
	RECT r;
	if (m_hWnd){
		m_pBrowserObject->DoVerb(OLEIVERB_HIDE, NULL, &m_Site, 0, NULL, &r);
		DestroyWindow(m_hWnd);
		m_hWnd = NULL;
		if (m_hWndBrowser && m_OldProc)
			SetWindowLongPtrW(m_hWndBrowser, GWLP_WNDPROC, (LONG_PTR)m_OldProc);
	}
	if (!hParent)
		return;
	GetClientRect(hParent, &r);
	if ((m_hWnd = CreateWindowExW(0, BROWSER_WND_CLASS, L"0000", WS_VISIBLE | WS_CHILD, 0, 0, r.right, r.bottom, hParent, NULL, s_hInstance, this))){
		m_Frame.m_hWindow = m_hWnd;
		//m_pBrowserObject->DoVerb(OLEIVERB_SHOW, NULL, m_pSite, 0, m_hWnd, &r);
		m_pBrowserObject->DoVerb(OLEIVERB_INPLACEACTIVATE, NULL, &m_Site, 0, m_hWnd, &r);
		HookWindow();
		
		/*if (m_pBrowser->QueryInterface(IID_IOleInPlaceActiveObject, (void**)&pInPlaceObject) == S_OK){
			if (pInPlaceObject->GetWindow(&m_hWndBrowser) != S_OK)
				m_hWndBrowser = NULL;
			else {
				//m_OldProc = (WNDPROC)GetWindowLongPtrW(m_hWndBrowser, GWLP_WNDPROC);
				//m_pBrowserUserdata = GetWindowLongPtrW(m_hWndBrowser, GWLP_USERDATA);
				//SwitchContext(true, m_hWndBrowser);
			}
			pInPlaceObject->Release();
		}//*/

		//m_pBrowser->put_Left(r.left);
		//m_pBrowser->put_Top(r.top);
		//m_pBrowser->put_Width(r.right);
		//m_pBrowser->put_Height(r.bottom);
	}

}

void CBrowser::Resize(RECT *pRect){
	MoveWindow(m_hWnd, pRect->left, pRect->top, pRect->right, pRect->bottom, TRUE);
}

bool CBrowser::Navigate(const wchar_t *pURL){
	VARIANT v, empty;
	BSTR str;
	bool result;

	V_VT(&v) = VT_BSTR;
	V_VT(&empty) = VT_EMPTY;

	if (!m_pBrowser) return false;

	str = SysAllocString(pURL);
	if (!str) return false;
	V_BSTR(&v) = str;

	result = (SUCCEEDED(m_pBrowser->Navigate2(&v, &empty, &empty, &empty, &empty)) ? true : false);


	SysFreeString(str);
	return result;
}

bool CBrowser::Forward(){
	if (!m_pBrowser) return false;
	return SUCCEEDED(m_pBrowser->GoForward()) ? true : false;
}

bool CBrowser::Back(){
	if (!m_pBrowser) return false;
	return SUCCEEDED(m_pBrowser->GoBack()) ? true : false;
}

bool CBrowser::Home(){
	if (!m_pBrowser) return false;
	return SUCCEEDED(m_pBrowser->GoHome()) ? true : false;
}

bool CBrowser::SurpressScriptErrors(bool surpress){
	bool old;
	if (!m_pBrowser) return false;
	old = m_CommandTarget.m_SurpressScriptErrors;
	m_CommandTarget.m_SurpressScriptErrors = surpress;
	return surpress;
}

bool CBrowser::AddEventCallback(IDispatch* pCallback, DWORD *pCookie){
	IConnectionPoint* pConnectionPoint;
	IConnectionPointContainer* pConnectionPointContainer;
	bool res;

	pConnectionPoint = NULL;
	pConnectionPointContainer = NULL;
	if (!m_pBrowser) return false;
	if (m_pBrowser->QueryInterface(IID_IConnectionPointContainer, (void**)&pConnectionPointContainer) != S_OK) return false;
	if (pConnectionPointContainer->FindConnectionPoint(DIID_DWebBrowserEvents2, &pConnectionPoint) != S_OK){
		pConnectionPointContainer->Release();
		return false;
	}

	res = false;

	if (pConnectionPoint->Advise(pCallback, pCookie) == S_OK){
		res = true;
		pCallback->AddRef();
	}

	pConnectionPoint->Release();
	pConnectionPointContainer->Release();
	return res;
}

bool CBrowser::RemoveEventCallback(DWORD Cookie){
	IConnectionPoint* pConnectionPoint;
	IConnectionPointContainer* pConnectionPointContainer;
	bool res;

	pConnectionPoint = NULL;
	pConnectionPointContainer = NULL;
	if (!m_pBrowser) return false;
	if (m_pBrowser->QueryInterface(IID_IConnectionPointContainer, (void**)&pConnectionPointContainer) != S_OK) return false;
	if (pConnectionPointContainer->FindConnectionPoint(DIID_DWebBrowserEvents2, &pConnectionPoint) != S_OK){
		pConnectionPointContainer->Release();
		return false;
	}

	res = false;

	if (pConnectionPoint->Unadvise(Cookie) == S_OK)
		res = true;

	pConnectionPoint->Release();
	pConnectionPointContainer->Release();

	return res;
}
