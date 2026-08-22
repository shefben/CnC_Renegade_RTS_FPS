#ifndef _FILESTREAM_H_
#define _FILESTREAM_H_
#include <windows.h>
#include "StdLib.h"
#include "BaseClasses.h"

class CFileStream : public IStream {
private:
	ULONG					m_RefCount;
	file					m_hFile;
	wchar_t*				m_pFileName;
	bool					m_ReadOnly;
	bool					m_Delete;
	CriticalSectionClass	m_CS;
public:
	CFileStream();
	~CFileStream();

	bool Open(const wchar_t *pFileName, bool Delete, bool ReadOnly);
	void Close(bool _delete);

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
	virtual ULONG STDMETHODCALLTYPE AddRef();
	virtual ULONG STDMETHODCALLTYPE Release();

	virtual HRESULT STDMETHODCALLTYPE Read(void *pv, ULONG cb, ULONG *pcbRead);
	virtual HRESULT STDMETHODCALLTYPE Write(const void *pv, ULONG cb, ULONG *pcbWritten);

	virtual HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
	virtual HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER libNewSize);
	virtual HRESULT STDMETHODCALLTYPE CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
	virtual HRESULT STDMETHODCALLTYPE Commit(DWORD grfCommitFlags);
	virtual HRESULT STDMETHODCALLTYPE Revert();
	virtual HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
	virtual HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
	virtual HRESULT STDMETHODCALLTYPE Stat(STATSTG *pstatstg, DWORD grfStatFlag);
	virtual HRESULT STDMETHODCALLTYPE Clone(IStream **ppstm);
};

#endif //!_FILESTREAM_H_
