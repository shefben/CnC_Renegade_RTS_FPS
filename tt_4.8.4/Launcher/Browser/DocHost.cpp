#include <windows.h>
#include <mshtml.h>
#include <mshtmhst.h>
#include <Exdisp.h>
#include "Browser.h"
#include "../Shared/StdLib.h"

CDocHostUIHandler::CDocHostUIHandler(){
}

HRESULT STDMETHODCALLTYPE CDocHostUIHandler::QueryInterface(REFIID riid, void **ppvObject){
	return m_pBrowser->QueryInterface(riid, ppvObject);
}

ULONG STDMETHODCALLTYPE CDocHostUIHandler::AddRef(){
	return (1);
}

ULONG STDMETHODCALLTYPE CDocHostUIHandler::Release(){
	return (1);
}

HRESULT STDMETHODCALLTYPE CDocHostUIHandler::ShowContextMenu(DWORD dwID, POINT *ppt, IUnknown *pcmdtReserved, IDispatch *pdispReserved){
	return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CDocHostUIHandler::GetHostInfo(DOCHOSTUIINFO *pInfo){
	pInfo->dwFlags |= DOCHOSTUIFLAG_SCROLL_NO | DOCHOSTUIFLAG_NO3DBORDER;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDocHostUIHandler::ShowUI(DWORD dwID, IOleInPlaceActiveObject *pActiveObject, IOleCommandTarget *pCommandTarget, IOleInPlaceFrame *pFrame, IOleInPlaceUIWindow *pDoc){
	//_IDocHostUIHandler *pHandler = (_IDocHostUIHandler*)This;
	return S_OK;
	NOTIMPLEMENTED;
	//return pHandler->m_pOriginal_Handler->lpVtbl->ShowUI(pHandler->m_pOriginal_Handler, dwID, pActiveObject, pCommandTarget, pFrame, pDoc);
}

HRESULT STDMETHODCALLTYPE CDocHostUIHandler::HideUI(){
	//_IDocHostUIHandler *pHandler = (_IDocHostUIHandler*)This;
	return S_OK;
	NOTIMPLEMENTED;
	//return pHandler->m_pOriginal_Handler->lpVtbl->HideUI(pHandler->m_pOriginal_Handler);
}

HRESULT STDMETHODCALLTYPE CDocHostUIHandler::UpdateUI(){
	return S_OK;
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE CDocHostUIHandler::EnableModeless(BOOL fEnable){
	return S_OK;
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE CDocHostUIHandler::OnDocWindowActivate(BOOL fActivate){
	return S_OK;
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE CDocHostUIHandler::OnFrameWindowActivate(BOOL fActivate){
	return S_OK;
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE CDocHostUIHandler::ResizeBorder(LPCRECT prcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow){
	return S_OK;
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE CDocHostUIHandler::TranslateAccelerator(LPMSG lpMsg, const GUID *pguidCmdGroup, DWORD nCmdID){
	return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CDocHostUIHandler::GetOptionKeyPath(LPOLESTR *pchKey, DWORD dw){
	*pchKey = NULL;
	return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CDocHostUIHandler::GetDropTarget(IDropTarget *pDropTarget, IDropTarget **ppDropTarget){
	*ppDropTarget = NULL;
	return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CDocHostUIHandler::GetExternal(IDispatch **ppDispatch){
	*ppDispatch = NULL;
	return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CDocHostUIHandler::TranslateUrl(DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut){
#ifdef _DEBUG
	LogFormated(L"URL in: %s\n", pchURLIn);
#endif //_DEBUG
	return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CDocHostUIHandler::FilterDataObject(IDataObject *pDO, IDataObject **ppDORet){
	*ppDORet = NULL;
	return S_FALSE;
}
