#include <windows.h>
#include <Urlmon.h>
#include <wininet.h>
#include "Downloader.h"

HRESULT STDMETHODCALLTYPE CCallbackDownload::QueryInterface(REFIID riid, void **ppvObject){
	NOTIMPLEMENTED;
}

ULONG STDMETHODCALLTYPE CCallbackDownload::AddRef(){
	return 1;
}

ULONG STDMETHODCALLTYPE CCallbackDownload::Release(){
	return 1;
}

HRESULT STDMETHODCALLTYPE CCallbackDownload::OnStartBinding(DWORD dwReserved, IBinding *pib){
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE CCallbackDownload::GetPriority(LONG *pnPriority){
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE CCallbackDownload::OnLowResource(DWORD reserved){
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE CCallbackDownload::OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText){
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE CCallbackDownload::OnStopBinding(HRESULT hresult, LPCWSTR szError){
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE CCallbackDownload::GetBindInfo(DWORD *grfBINDF, BINDINFO *pbindinfo){
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE CCallbackDownload::OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM *pstgmed){
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE CCallbackDownload::OnObjectAvailable(REFIID riid, IUnknown *punk){
	NOTIMPLEMENTED;
}
