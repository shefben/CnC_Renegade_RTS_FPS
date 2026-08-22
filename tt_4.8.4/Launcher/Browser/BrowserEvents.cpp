#include <windows.h>
#include <OAIDL.H>
#include <mshtml.h>
#include <mshtmhst.h>
#include <Exdisp.h>
#include <ExDispid.h>
#include "Browser.h"
#include "../Shared/StdLib.h"

/*
CBrowserEvents
*/

void CBrowserEvents::BeforeNavigate(IDispatch *pDisp, VARIANT *url,VARIANT *Flags,VARIANT *TargetFrameName,VARIANT *PostData,VARIANT *Headers,VARIANT_BOOL *Cancel){
}

void CBrowserEvents::CommandStateChange(long Command,VARIANT_BOOL Enable){
}

void CBrowserEvents::DownloadBegin(){
}

void CBrowserEvents::DownloadComplete(){
}

void CBrowserEvents::NavigateComplete(IDispatch *pDisp,VARIANT *URL){
}

void CBrowserEvents::NewWindow(IDispatch **ppDisp,VARIANT_BOOL *Cancel){
}

void CBrowserEvents::OnQuit(){
}

void CBrowserEvents::ProgressChange(long Progress,long ProgressMax){
}

void CBrowserEvents::PropertyChange(BSTR szProperty){
}

void CBrowserEvents::StatusTextChange(BSTR Text){
}

void CBrowserEvents::TitleChange(BSTR Text){
}

/*
CBrowserEvents2
*/

void CBrowserEvents2::BeforeNavigate2(IDispatch *pDisp, BSTR url, unsigned long Flags, BSTR TargetFrameName, VARIANT *PostData, BSTR Headers, BOOL *Cancel){
#ifdef _DEBUG
	LogFormated(L"Before navigate URL -> %s\n", url);
#endif //_DEBUG
}

void CBrowserEvents2::ClientToHostWindow(long *CX, long *CY){
}

void CBrowserEvents2::CommandStateChange(long Command, BOOL Enable){
}

void CBrowserEvents2::DocumentComplete(IDispatch *pDisp, BSTR URL){
}

void CBrowserEvents2::DownloadBegin(){
}

void CBrowserEvents2::DownloadComplete(){
}

void CBrowserEvents2::FileDownload(BOOL ActiveDocument, BOOL *Cancel){
}

void CBrowserEvents2::NavigateComplete2(IDispatch *pDisp, BSTR URL){
}

void CBrowserEvents2::NavigateError(IDispatch *pDisp, BSTR URL, BSTR TargetFrameName, DWORD StatusCode, BOOL *Cancel){
}

void CBrowserEvents2::NewProcess(long lCauseFlag, IDispatch *pWB2, BOOL *Cancel){
}

void CBrowserEvents2::NewWindow2(IDispatch **ppDisp, BOOL *Cancel){
}

void CBrowserEvents2::NewWindow3(IDispatch **ppDisp, BOOL *Cancel, DWORD dwFlags, BSTR bstrUrlContext, BSTR bstrUrl){
}

void CBrowserEvents2::OnFullScreen(BOOL FullScreen){
}

void CBrowserEvents2::OnMenuBar(BOOL MenuBar){
}

void CBrowserEvents2::OnQuit(){
}

void CBrowserEvents2::OnStatusBar(BOOL StatusBar){
}

void CBrowserEvents2::OnTheaterMode(BOOL TheaterMode){
}

void CBrowserEvents2::OnToolBar(BOOL ToolBar){
}

void CBrowserEvents2::OnVisible(BOOL Visible){
}

void CBrowserEvents2::PrintTemplateInstantiation(IDispatch *pDisp){
}

void CBrowserEvents2::PrintTemplateTeardown(IDispatch *pDisp){
}

void CBrowserEvents2::PrivacyImpactedStateChange(BOOL PrivacyImpacted){
}

void CBrowserEvents2::ProgressChange(long Progress, long ProgressMax){
}

void CBrowserEvents2::PropertyChange(BSTR szProperty){
}

void CBrowserEvents2::RedirectXDomainBlocked(IDispatch *pDisp, BSTR StartURL, BSTR RedirectURL, BSTR Frame, DWORD StatusCode){
}

void CBrowserEvents2::SetPhishingFilterStatus(long PhishingFilterStatus){
}

void CBrowserEvents2::SetSecureLockIcon(DWORD SecureLockIcon){
}

void CBrowserEvents2::StatusTextChange(BSTR Text){
}

void CBrowserEvents2::ThirdPartyUrlBlocked(BSTR URL, DWORD dwCount){
}

void CBrowserEvents2::TitleChange(BSTR Text){
}

void CBrowserEvents2::WindowClosing(BOOL IsChildWindow, BOOL *Cancel){
}

void CBrowserEvents2::WindowSetHeight(long Height){
}

void CBrowserEvents2::WindowSetLeft(long Left){
}

void CBrowserEvents2::WindowSetResizable(BOOL Resizable){
}

void CBrowserEvents2::WindowSetTop(long Top){
}

void CBrowserEvents2::WindowSetWidth(long Width){
}

void CBrowserEvents2::WindowStateChanged(DWORD dwFlags, DWORD dwValidFlagsMask){
}


/*
CBrowserEventDispatch
*/

CBrowserEventDispatch::CBrowserEventDispatch(){
	m_RefCount = 1;
	m_pCB = NULL;
}

HRESULT STDMETHODCALLTYPE CBrowserEventDispatch::QueryInterface(REFIID riid,void __RPC_FAR *__RPC_FAR *ppvObject){
	if (!ppvObject) return E_POINTER;
	if (memcmp(&riid, &DIID_DWebBrowserEvents2, sizeof(IID)) == 0){
		*ppvObject = this;
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CBrowserEventDispatch::AddRef(){
	m_RefCount++;
	return m_RefCount;
}

ULONG STDMETHODCALLTYPE CBrowserEventDispatch::Release(){
	m_RefCount--;
	if (m_RefCount == 0){
		delete this;
		return 0;
	};
	return m_RefCount;
}

HRESULT STDMETHODCALLTYPE CBrowserEventDispatch::GetTypeInfoCount(UINT *pctinfo){
	if (pctinfo) *pctinfo = 0;
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE CBrowserEventDispatch::GetTypeInfo(UINT iTInfo,LCID lcid,ITypeInfo **ppTInfo){
	if (ppTInfo) *ppTInfo = NULL;
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE CBrowserEventDispatch::GetIDsOfNames(REFIID riid,LPOLESTR *rgszNames,UINT cNames,LCID lcid,DISPID *rgDispId){
	if (rgszNames) *rgszNames = NULL;
	if (rgDispId) *rgDispId = NULL;
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE CBrowserEventDispatch::Invoke(DISPID dispIdMember,REFIID riid,LCID lcid,WORD wFlags,DISPPARAMS *pDispParams,VARIANT *pVarResult,EXCEPINFO *pExcepInfo,UINT *puArgErr){
	//LogFormated(L"Event %u!\n", dispIdMember);
	//DISP_E_PARAMNOTOPTIONAL
	switch (dispIdMember){
		case DISPID_BEFORENAVIGATE2:
			return BeforeNavigate2(pDispParams);
		case DISPID_CLIENTTOHOSTWINDOW:
			return ClientToHostWindow(pDispParams);
		case DISPID_COMMANDSTATECHANGE:
			return CommandStateChange(pDispParams);
		case DISPID_DOCUMENTCOMPLETE:
			return DocumentComplete(pDispParams);
		case DISPID_DOWNLOADBEGIN:
			return DownloadBegin(pDispParams);
		case DISPID_DOWNLOADCOMPLETE:
			return DownloadComplete(pDispParams);
		case DISPID_FILEDOWNLOAD:
			return FileDownload(pDispParams);
		case DISPID_NAVIGATECOMPLETE2:
			return NavigateComplete2(pDispParams);
		case DISPID_NAVIGATEERROR:
			return NavigateError(pDispParams);
			//case DISPID_NEWPROCESS //IE8
		case DISPID_NEWWINDOW2:
			return NewWindow2(pDispParams);
		case DISPID_NEWWINDOW3:
			return NewWindow3(pDispParams);
		case DISPID_ONFULLSCREEN:
			return OnFullScreen(pDispParams);
		case DISPID_ONMENUBAR:
			return OnMenuBar(pDispParams);
		case DISPID_ONQUIT:
			return OnQuit(pDispParams);
		case DISPID_ONSTATUSBAR:
			return OnStatusBar(pDispParams);
		case DISPID_ONTHEATERMODE:
			return OnTheaterMode(pDispParams);
		case DISPID_ONTOOLBAR:
			return OnToolBar(pDispParams);
		case DISPID_ONVISIBLE:
			return OnVisible(pDispParams);
		case DISPID_PRINTTEMPLATEINSTANTIATION:
			return PrintTemplateInstantiation(pDispParams);
		case DISPID_PRINTTEMPLATETEARDOWN:
			return PrintTemplateTeardown(pDispParams);
		case DISPID_PRIVACYIMPACTEDSTATECHANGE:
			return PrivacyImpactedStateChange(pDispParams);
		case DISPID_PROGRESSCHANGE:
			return ProgressChange(pDispParams);
		case DISPID_PROPERTYCHANGE:
			return PropertyChange(pDispParams);
			//case DISPID_REDIRECTXDOMAINBLOCKED //IE8
		case DISPID_SETPHISHINGFILTERSTATUS:
			return SetPhishingFilterStatus(pDispParams);
		case DISPID_SETSECURELOCKICON:
			return SetSecureLockIcon(pDispParams);
		case DISPID_STATUSTEXTCHANGE:
			return StatusTextChange(pDispParams);
			//case DISPID_THIRDPARTYURLBLOCKED //IE8
		case DISPID_TITLECHANGE:
			return TitleChange(pDispParams);
		case DISPID_WINDOWCLOSING:
			return WindowClosing(pDispParams);
		case DISPID_WINDOWSETHEIGHT:
			return WindowSetHeight(pDispParams);
		case DISPID_WINDOWSETLEFT:
			return WindowSetLeft(pDispParams);
		case DISPID_WINDOWSETRESIZABLE:
			return WindowSetResizable(pDispParams);
		case DISPID_WINDOWSETTOP:
			return WindowSetTop(pDispParams);
		case DISPID_WINDOWSETWIDTH:
			return WindowSetWidth(pDispParams);
		case DISPID_WINDOWSTATECHANGED:
			return WindowStateChanged(pDispParams);
		default:
			return DISP_E_MEMBERNOTFOUND;
	}
	return DISP_E_MEMBERNOTFOUND;
}

enum {
	BeforeNavigate2_Cancel, BeforeNavigate2_Headers, BeforeNavigate2_PostData, BeforeNavigate2_TargetFrameName, BeforeNavigate2_Flags, BeforeNavigate2_url, BeforeNavigate2_pDisp, BeforeNavigate2_ArgCount,
};

HRESULT CBrowserEventDispatch::BeforeNavigate2(DISPPARAMS *pParams){
	BOOL cancel;
	if (!m_pCB) return DISP_E_PARAMNOTOPTIONAL;
	if (!pParams) return DISP_E_PARAMNOTOPTIONAL;
	if (pParams->cArgs != BeforeNavigate2_ArgCount) return DISP_E_BADPARAMCOUNT;
	m_pCB->BeforeNavigate2(
		pParams->rgvarg[BeforeNavigate2_pDisp].pdispVal,
		pParams->rgvarg[BeforeNavigate2_url].pvarVal->bstrVal,
		pParams->rgvarg[BeforeNavigate2_Flags].pvarVal->lVal,
		pParams->rgvarg[BeforeNavigate2_TargetFrameName].pvarVal->bstrVal,
		pParams->rgvarg[BeforeNavigate2_PostData].pvarVal,
		pParams->rgvarg[BeforeNavigate2_Headers].pvarVal->bstrVal,
		&cancel
		);
	pParams->rgvarg[BeforeNavigate2_Cancel].pvarVal->boolVal = (cancel == TRUE ? VARIANT_TRUE : VARIANT_FALSE);


	return S_OK;
}

HRESULT CBrowserEventDispatch::ClientToHostWindow(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

HRESULT CBrowserEventDispatch::CommandStateChange(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

enum {
	DocumentComplete_URL, DocumentComplete_pDisp, DocumentComplete_ArgCount,
};

HRESULT CBrowserEventDispatch::DocumentComplete(DISPPARAMS *pParams){
	if (!m_pCB) return DISP_E_PARAMNOTOPTIONAL;
	if (!pParams) return DISP_E_PARAMNOTOPTIONAL;
	if (pParams->cArgs != DocumentComplete_ArgCount) return DISP_E_BADPARAMCOUNT;

	m_pCB->DocumentComplete(
		pParams->rgvarg[DocumentComplete_pDisp].pdispVal,
		pParams->rgvarg[DocumentComplete_URL].pvarVal->bstrVal
		);

	return S_OK;
}

HRESULT CBrowserEventDispatch::DownloadBegin(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

HRESULT CBrowserEventDispatch::DownloadComplete(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

HRESULT CBrowserEventDispatch::FileDownload(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

HRESULT CBrowserEventDispatch::NavigateComplete2(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

HRESULT CBrowserEventDispatch::NavigateError(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

HRESULT CBrowserEventDispatch::NewProcess(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

HRESULT CBrowserEventDispatch::NewWindow2(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

HRESULT CBrowserEventDispatch::NewWindow3(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

HRESULT CBrowserEventDispatch::OnFullScreen(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

HRESULT CBrowserEventDispatch::OnMenuBar(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

HRESULT CBrowserEventDispatch::OnQuit(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

HRESULT CBrowserEventDispatch::OnStatusBar(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

HRESULT CBrowserEventDispatch::OnTheaterMode(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

HRESULT CBrowserEventDispatch::OnToolBar(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

HRESULT CBrowserEventDispatch::OnVisible(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

HRESULT CBrowserEventDispatch::PrintTemplateInstantiation(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

HRESULT CBrowserEventDispatch::PrintTemplateTeardown(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

HRESULT CBrowserEventDispatch::PrivacyImpactedStateChange(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

HRESULT CBrowserEventDispatch::ProgressChange(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

HRESULT CBrowserEventDispatch::PropertyChange(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

HRESULT CBrowserEventDispatch::RedirectXDomainBlocked(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

HRESULT CBrowserEventDispatch::SetPhishingFilterStatus(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

HRESULT CBrowserEventDispatch::SetSecureLockIcon(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

HRESULT CBrowserEventDispatch::StatusTextChange(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

HRESULT CBrowserEventDispatch::ThirdPartyUrlBlocked(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

HRESULT CBrowserEventDispatch::TitleChange(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

//DWebBrowserEvents2::UpdatePageStatus PURE{
//	return DISP_E_MEMBERNOTFOUND;
//}

HRESULT CBrowserEventDispatch::WindowClosing(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

HRESULT CBrowserEventDispatch::WindowSetHeight(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

HRESULT CBrowserEventDispatch::WindowSetLeft(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

HRESULT CBrowserEventDispatch::WindowSetResizable(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

HRESULT CBrowserEventDispatch::WindowSetTop(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

HRESULT CBrowserEventDispatch::WindowSetWidth(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

HRESULT CBrowserEventDispatch::WindowStateChanged(DISPPARAMS *pParams){
	return DISP_E_MEMBERNOTFOUND;
}

void CBrowserEventDispatch::SetCallback(IBrowserEvents2 *pCB){
	m_pCB = pCB;
}