#include <windows.h>
#include <mshtml.h>
#include <mshtmhst.h>
#include <Exdisp.h>
#include "Browser.h"
#include "../Shared/StdLib.h"

COleCommandTarget::COleCommandTarget(){
}

HRESULT STDMETHODCALLTYPE COleCommandTarget::QueryInterface(REFIID riid, void ** ppvObject){
	return m_pBrowser->QueryInterface(riid, ppvObject);
}

ULONG STDMETHODCALLTYPE COleCommandTarget::AddRef(){
	return (1);
}

ULONG STDMETHODCALLTYPE COleCommandTarget::Release(){
	return (1);
}

HRESULT STDMETHODCALLTYPE COleCommandTarget::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText){
	ULONG u;
	u = 0;
	for(u = 0; u < cCmds; u++)
		prgCmds[u].cmdf = 0;
	return S_OK;
		//LogFormated(L"QueryStatus for: %d\n", prgCmds[u].cmdID);
	NOTIMPLEMENTED;
	return OLECMDERR_E_UNKNOWNGROUP;
}

HRESULT STDMETHODCALLTYPE COleCommandTarget::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut){
	HRESULT hr;
	if (pguidCmdGroup && IsEqualGUID(*pguidCmdGroup, CGID_DocHostCommandHandler)){
		hr = OLECMDERR_E_NOTSUPPORTED;
		if (nCmdID == OLECMDID_SHOWSCRIPTERROR){
			pvaOut->vt = VT_BOOL;
			pvaOut->boolVal = m_SurpressScriptErrors ? VARIANT_FALSE : VARIANT_TRUE;
			hr = S_OK;
		}
	} else
		hr = OLECMDERR_E_UNKNOWNGROUP;

	return hr;
}
