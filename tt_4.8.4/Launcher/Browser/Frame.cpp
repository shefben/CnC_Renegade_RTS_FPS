#include <windows.h>
#include <mshtml.h>
#include <mshtmhst.h>
#include <Exdisp.h>
#include "Browser.h"

COleInPlaceFrame::COleInPlaceFrame(){
}

HRESULT STDMETHODCALLTYPE COleInPlaceFrame::QueryInterface(REFIID riid, LPVOID FAR* ppvObj) {
	return m_pBrowser->QueryInterface(riid, ppvObj);
}

ULONG STDMETHODCALLTYPE COleInPlaceFrame::AddRef() {
	return(1);
}

ULONG STDMETHODCALLTYPE COleInPlaceFrame::Release() {
	return(1);
}

HRESULT STDMETHODCALLTYPE COleInPlaceFrame::GetWindow(HWND FAR* lphwnd) {
	*lphwnd = m_hWindow;
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE COleInPlaceFrame::ContextSensitiveHelp(BOOL fEnterMode) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE COleInPlaceFrame::GetBorder(LPRECT lprectBorder) {
	return INPLACE_E_NOTOOLSPACE;
}

HRESULT STDMETHODCALLTYPE COleInPlaceFrame::RequestBorderSpace(LPCBORDERWIDTHS pborderwidths) {
	return INPLACE_E_NOTOOLSPACE;
}

HRESULT STDMETHODCALLTYPE COleInPlaceFrame::SetBorderSpace(LPCBORDERWIDTHS pborderwidths) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE COleInPlaceFrame::SetActiveObject(IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName) {
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE COleInPlaceFrame::InsertMenus(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE COleInPlaceFrame::SetMenu(HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject) {
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE COleInPlaceFrame::RemoveMenus(HMENU hmenuShared) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE COleInPlaceFrame::SetStatusText(LPCOLESTR pszStatusText) {
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE COleInPlaceFrame::EnableModeless(BOOL fEnable) {
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE COleInPlaceFrame::TranslateAccelerator(LPMSG lpmsg, WORD wID) {
	NOTIMPLEMENTED;
}
