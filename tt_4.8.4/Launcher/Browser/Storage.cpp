#include <windows.h>
#include <mshtml.h>
#include <mshtmhst.h>
#include <Exdisp.h>
#include "Browser.h"

HRESULT STDMETHODCALLTYPE CStorage::QueryInterface(REFIID riid, LPVOID FAR* ppvObj) {
	NOTIMPLEMENTED;
}

ULONG STDMETHODCALLTYPE CStorage::AddRef() {
	return(1);
}

ULONG STDMETHODCALLTYPE CStorage::Release() {
	return(1);
}

HRESULT STDMETHODCALLTYPE CStorage::CreateStream(const WCHAR *pwcsName, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStream **ppstm) {
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE CStorage::OpenStream(const WCHAR * pwcsName, void *reserved1, DWORD grfMode, DWORD reserved2, IStream **ppstm) {
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE CStorage::CreateStorage(const WCHAR *pwcsName, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStorage **ppstg) {
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE CStorage::OpenStorage(const WCHAR * pwcsName, IStorage * pstgPriority, DWORD grfMode, SNB snbExclude, DWORD reserved, IStorage **ppstg) {
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE CStorage::CopyTo(DWORD ciidExclude, IID const *rgiidExclude, SNB snbExclude,IStorage *pstgDest) {
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE CStorage::MoveElementTo(const OLECHAR *pwcsName,IStorage * pstgDest, const OLECHAR *pwcsNewName, DWORD grfFlags) {
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE CStorage::Commit(DWORD grfCommitFlags) {
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE CStorage::Revert() {
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE CStorage::EnumElements(DWORD reserved1, void * reserved2, DWORD reserved3, IEnumSTATSTG ** ppenum) {
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE CStorage::DestroyElement(const OLECHAR *pwcsName) {
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE CStorage::RenameElement(const WCHAR *pwcsOldName, const WCHAR *pwcsNewName) {
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE CStorage::SetElementTimes(const WCHAR *pwcsName, FILETIME const *pctime, FILETIME const *patime, FILETIME const *pmtime) {
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE CStorage::SetClass(REFCLSID clsid) {
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE CStorage::SetStateBits(DWORD grfStateBits, DWORD grfMask) {
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE CStorage::Stat(STATSTG * pstatstg, DWORD grfStatFlag) {
	NOTIMPLEMENTED;
}
