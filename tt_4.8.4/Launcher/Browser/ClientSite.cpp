#include <windows.h>
#include <mshtml.h>
#include <mshtmhst.h>
#include <Exdisp.h>
#include "Browser.h"
#include "../Shared/StdLib.h"

COleClientSite::COleClientSite() {
}

HRESULT STDMETHODCALLTYPE COleClientSite::QueryInterface(REFIID riid, void **ppvObject){
	return m_pBrowser->QueryInterface(riid, ppvObject);
}

ULONG STDMETHODCALLTYPE COleClientSite::AddRef(){
	return(1);
}

ULONG STDMETHODCALLTYPE COleClientSite::Release(){
	return(1);
}

HRESULT STDMETHODCALLTYPE COleClientSite::SaveObject(){
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE COleClientSite::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker ** ppmk){
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE COleClientSite::GetContainer(LPOLECONTAINER FAR* ppContainer){
	*ppContainer = 0;

	return(E_NOINTERFACE);
}

HRESULT STDMETHODCALLTYPE COleClientSite::ShowObject(){
	return(NOERROR);
}

HRESULT STDMETHODCALLTYPE COleClientSite::OnShowWindow(BOOL fShow){
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE COleClientSite::RequestNewObjectLayout(){
	NOTIMPLEMENTED;
}