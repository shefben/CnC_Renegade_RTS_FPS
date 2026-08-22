#ifndef _DOWNLOADER_H_
#define _DOWNLOADER_H_

#include <Urlmon.h>
#include <wininet.h>
#include "DownloaderAPI.h"

#include "../Shared/Memory.h"
#include "../Shared/BaseTypes.h"
#include "../Shared/BaseClasses.h"
#include "../Shared/FileStream.h"

#define NOTIMPLEMENTED return(E_NOTIMPL)

#define DOWNLOAD_CLEAR_FLAGS	(BINDF_PRAGMA_NO_CACHE | BINDF_NOWRITECACHE | BINDF_GETNEWESTVERSION | BINDF_RESYNCHRONIZE)

/*
CMemoryDownload:	Content is downloaded to memory, useful to download files with version info
CFileDownload:		Content is downloaded to file, useful for content delivery packages
CStreamDownload:	Content is downloaded to an IStream, useful for any sort of stream operation
*/


class CMemoryDownload : public IBindStatusCallback {
private:
	ptr								m_pBuffer;
	size_32							m_BufferSize;
	size_32							m_Offset;
	bool							m_LocalAlloc;
	bool							m_Canceled;
	bool							m_Async;
	EventClass						m_Event;
	IMoniker*						m_pMoniker;
	IBindCtx*						m_pBindCtx;
	CriticalSectionClass			m_CS;
	pfnDownloadProgressCallback		m_pCB;
	WPARAM							m_wParam;
	LPARAM							m_lParam;	
	size_32	m_MaxSize;
public:
	CMemoryDownload();
	~CMemoryDownload();

	bool Start(const wchar_t* pURL, ptr pBuf, size_32 bufsize, size_32 maxsize, pfnDownloadProgressCallback pCB, WPARAM wParam, LPARAM lParam, bool async);
	bool Wait();
	void Cancel();
	bool WasCanceled();
	void Close();

	size_32 GetSize();
	ptr GetBuffer();
	void LockBuffer();
	void UnlockBuffer();


	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();
	HRESULT STDMETHODCALLTYPE OnStartBinding(DWORD dwReserved, IBinding *pib);
	HRESULT STDMETHODCALLTYPE GetPriority(LONG *pnPriority);
	HRESULT STDMETHODCALLTYPE OnLowResource(DWORD reserved);
	HRESULT STDMETHODCALLTYPE OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText);
	HRESULT STDMETHODCALLTYPE OnStopBinding(HRESULT hresult, LPCWSTR szError);
	HRESULT STDMETHODCALLTYPE GetBindInfo(DWORD *grfBINDF, BINDINFO *pbindinfo);
	HRESULT STDMETHODCALLTYPE OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM *pstgmed);
	HRESULT STDMETHODCALLTYPE OnObjectAvailable(REFIID riid, IUnknown *punk);

};

class CFileDownload : public IBindStatusCallback {
private:
	CriticalSectionClass			m_CS;
	pfnDownloadProgressCallback		m_pCB;
	WPARAM							m_wParam;
	LPARAM							m_lParam;
	EventClass						m_Event;
	int								m_Canceled;
	ULONG							m_Offset;
	CFileStream						m_Stream;
	IMoniker*						m_pMoniker;
	IBindCtx*						m_pBindCtx;
	IBinding*						m_pBinding;
	ULONG							m_RefCount;
	bool							m_Async;

public:

	
	CFileDownload();
	~CFileDownload();
	bool Start(const wchar_t* pURL, const wchar_t* pFile, pfnDownloadProgressCallback pCB, WPARAM wParam, LPARAM lParam, bool async);
	void Close();
	bool Wait();
	void Cancel();
	int WasCanceled();

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();
	HRESULT STDMETHODCALLTYPE OnStartBinding(DWORD dwReserved, IBinding *pib);
	HRESULT STDMETHODCALLTYPE GetPriority(LONG *pnPriority);
	HRESULT STDMETHODCALLTYPE OnLowResource(DWORD reserved);
	HRESULT STDMETHODCALLTYPE OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText);
	HRESULT STDMETHODCALLTYPE OnStopBinding(HRESULT hresult, LPCWSTR szError);
	HRESULT STDMETHODCALLTYPE GetBindInfo(DWORD *grfBINDF, BINDINFO *pbindinfo);
	HRESULT STDMETHODCALLTYPE OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM *pstgmed);
	HRESULT STDMETHODCALLTYPE OnObjectAvailable(REFIID riid, IUnknown *punk);
};

class CStreamDownload : public IBindStatusCallback {
private:
	CriticalSectionClass			m_CS;
	pfnDownloadProgressCallback		m_pCB;
	WPARAM							m_wParam;
	LPARAM							m_lParam;
	EventClass						m_Event;
	bool							m_Canceled;
	ULONG							m_Offset;
	IStream*						m_pStream;
	IMoniker*						m_pMoniker;
	IBindCtx*						m_pBindCtx;
	ULONG							m_RefCount;
	bool							m_Async;

public:

	
	CStreamDownload();
	~CStreamDownload();
	bool Start(const wchar_t* pURL, IStream* pStream, pfnDownloadProgressCallback pCB, WPARAM wParam, LPARAM lParam, bool async);
	void Close();
	bool Wait();
	void Cancel();
	bool WasCanceled();

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();
	HRESULT STDMETHODCALLTYPE OnStartBinding(DWORD dwReserved, IBinding *pib);
	HRESULT STDMETHODCALLTYPE GetPriority(LONG *pnPriority);
	HRESULT STDMETHODCALLTYPE OnLowResource(DWORD reserved);
	HRESULT STDMETHODCALLTYPE OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText);
	HRESULT STDMETHODCALLTYPE OnStopBinding(HRESULT hresult, LPCWSTR szError);
	HRESULT STDMETHODCALLTYPE GetBindInfo(DWORD *grfBINDF, BINDINFO *pbindinfo);
	HRESULT STDMETHODCALLTYPE OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM *pstgmed);
	HRESULT STDMETHODCALLTYPE OnObjectAvailable(REFIID riid, IUnknown *punk);
};

class CCallbackDownload : IBindStatusCallback {
public:
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();
	HRESULT STDMETHODCALLTYPE OnStartBinding(DWORD dwReserved, IBinding *pib);
	HRESULT STDMETHODCALLTYPE GetPriority(LONG *pnPriority);
	HRESULT STDMETHODCALLTYPE OnLowResource(DWORD reserved);
	HRESULT STDMETHODCALLTYPE OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText);
	HRESULT STDMETHODCALLTYPE OnStopBinding(HRESULT hresult, LPCWSTR szError);
	HRESULT STDMETHODCALLTYPE GetBindInfo(DWORD *grfBINDF, BINDINFO *pbindinfo);
	HRESULT STDMETHODCALLTYPE OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM *pstgmed);
	HRESULT STDMETHODCALLTYPE OnObjectAvailable(REFIID riid, IUnknown *punk);
};

#endif //_DOWNLOADER_H_
