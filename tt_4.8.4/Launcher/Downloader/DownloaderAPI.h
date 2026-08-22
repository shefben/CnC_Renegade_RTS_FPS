#ifndef _DOWNLOADERAPI_H_
#define _DOWNLOADERAPI_H_

#define DOWNLOAD_ERROR_SUCCESS		(0)
#define DOWNLOAD_ERROR_TOO_LARGE	(1)
#define DOWNLOAD_ERROR_ALLOC_FAILED	(2)
#define DOWNLOAD_ERROR_READ_FAILED	(3)

#define DOWNLOAD_BEGIN				((DWORD)-1)
#define DOWNLOAD_DATA				((DWORD)-2)
#define DOWNLOAD_END				((DWORD)-3)
#define DOWNLOAD_DATA_AVAILABLE		((DWORD)-4)
#define DOWNLOAD_CANCELED			((DWORD)-5)

enum {
	DOWNLOAD_CANCEL_NOT = 0, DOWNLOAD_CANCEL_USER, DOWNLOAD_CANCEL_ERROR,
};

typedef bool (__stdcall* pfnDownloadDataCallback)(const void* pData, DWORD progress, DWORD size, DWORD state, WPARAM wParam, LPARAM lParam);
typedef bool (__stdcall* pfnDownloadProgressCallback)(DWORD progress, DWORD size, DWORD state, WPARAM wParam, LPARAM lParam);

#endif //_DOWNLOADERAPI_H_
