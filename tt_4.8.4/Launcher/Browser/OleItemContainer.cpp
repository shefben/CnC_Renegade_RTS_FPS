#include <windows.h>
#include <mshtml.h>
#include <mshtmhst.h>
#include <Exdisp.h>
#include "Browser.h"

COleItemContainer::COleItemContainer(){
}

HRESULT STDMETHODCALLTYPE COleItemContainer::QueryInterface(REFIID riid, void ** ppvObject){
	return m_pBrowser->QueryInterface(riid, ppvObject);
}

ULONG STDMETHODCALLTYPE COleItemContainer::AddRef(){
	return 1;
}

ULONG STDMETHODCALLTYPE COleItemContainer::Release(){
	return 1;
}

HRESULT STDMETHODCALLTYPE COleItemContainer::ParseDisplayName(IBindCtx *pbc, LPOLESTR pszDisplayName, ULONG *pchEaten, IMoniker **ppmkOut){
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE COleItemContainer::EnumObjects(DWORD grfFlags, IEnumUnknown **ppenum){
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE COleItemContainer::LockContainer(BOOL fLock){
	return S_OK;
}

HRESULT STDMETHODCALLTYPE COleItemContainer::GetObject(LPOLESTR pszItem, DWORD dwSpeedNeeded, IBindCtx *pbc, REFIID riid, void **ppvObject){
	if (!pszItem || !ppvObject) return E_INVALIDARG;
	*ppvObject = NULL;
	return MK_E_NOOBJECT;
}

HRESULT STDMETHODCALLTYPE COleItemContainer::GetObjectStorage(LPOLESTR pszItem, IBindCtx *pbc, REFIID riid, void **ppvStorage){
	if (!pszItem || !ppvStorage) return E_INVALIDARG;
	*ppvStorage = NULL;
	return MK_E_NOOBJECT;
}

HRESULT STDMETHODCALLTYPE COleItemContainer::IsRunning(LPOLESTR pszItem){
	if (!pszItem) return E_INVALIDARG;
	return MK_E_NOOBJECT;
}
