#include <windows.h>
#include <mshtml.h>
#include <mshtmhst.h>
#include <Exdisp.h>
#include "Browser.h"
#include "../Shared/StdLib.h"

COleInPlaceSite::COleInPlaceSite(){
}

HRESULT STDMETHODCALLTYPE COleInPlaceSite::QueryInterface(REFIID riid, void ** ppvObject){
	return m_pClientSite->QueryInterface(riid, ppvObject);

	/*if (!memcmp(&riid, &IID_IUnknown, sizeof(GUID)) || !memcmp(riid, &IID_IOleClientSite, sizeof(GUID)))
		*ppvObject = &((_IOleClientSiteEx *)This)->client;

	else if (!memcmp(&riid, &IID_IOleInPlaceSite, sizeof(GUID)))
		*ppvObject = &((_IOleClientSiteEx *)This)->inplace;
	else if (memcmp(&riid, &IID_IDocHostUIHandler, sizeof(GUID)) == 0)
		*ppvObject = &((_IOleClientSiteEx*)This)->handler;
	else {
		*ppvObject = 0;
		return(E_NOINTERFACE);
	}
	return(S_OK);
	NOTIMPLEMENTED;//*/
}

ULONG STDMETHODCALLTYPE COleInPlaceSite::AddRef(){
	return(1);
}

ULONG STDMETHODCALLTYPE COleInPlaceSite::Release(){
	return(1);
}

HRESULT STDMETHODCALLTYPE COleInPlaceSite::GetWindow(HWND FAR* lphwnd) {
	*lphwnd = m_pBrowser->m_Frame.m_hWindow;

	return(S_OK);
}

HRESULT STDMETHODCALLTYPE COleInPlaceSite::ContextSensitiveHelp(BOOL fEnterMode) {
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE COleInPlaceSite::CanInPlaceActivate() {
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE COleInPlaceSite::OnInPlaceActivate() {
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE COleInPlaceSite::OnUIActivate() {
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE COleInPlaceSite::GetWindowContext(LPOLEINPLACEFRAME FAR* lplpFrame, LPOLEINPLACEUIWINDOW FAR* lplpDoc, LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo) {
	
	*lplpFrame = 0;//&m_pClientSite->m_Frame;

	*lplpDoc = 0;

	lpFrameInfo->fMDIApp = FALSE;
	lpFrameInfo->hwndFrame = m_pBrowser->m_Frame.m_hWindow;;
	lpFrameInfo->haccel = 0;
	lpFrameInfo->cAccelEntries = 0;

	GetClientRect(lpFrameInfo->hwndFrame, lprcPosRect);
	GetClientRect(lpFrameInfo->hwndFrame, lprcClipRect);

	return(S_OK);
}

HRESULT STDMETHODCALLTYPE COleInPlaceSite::Scroll(SIZE scrollExtent) {
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE COleInPlaceSite::OnUIDeactivate(BOOL fUndoable) {
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE COleInPlaceSite::OnInPlaceDeactivate() {
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE COleInPlaceSite::DiscardUndoState() {
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE COleInPlaceSite::DeactivateAndUndo() {
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE COleInPlaceSite::OnPosRectChange(LPCRECT lprcPosRect) {
	return(S_OK);
}
