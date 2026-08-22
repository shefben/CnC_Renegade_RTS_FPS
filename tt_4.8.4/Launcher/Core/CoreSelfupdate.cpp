#include <windows.h>
#include "Core.h"
#include "../Shared/StdLib.h"
#include "../Shared/Memory.h"

#include "../Downloader/DownloaderAPI.h"
#include "../Downloader/Downloader.h"
#include <commctrl.h>

#include "resource.h"
#include "Settings.h"

EventClass	g_DialogEvent;

void HandleContentData(const wchar_t* pContentPackage){
}

#define DOWNLOAD_DONE		(WM_USER+0x100)



bool __stdcall SelfUpdateCallback(DWORD progress, DWORD size, DWORD state, WPARAM wParam, LPARAM lParam){
	HWND hDlg;
	HWND hProgress;
	hDlg = (HWND)wParam;
	hProgress = (HWND)lParam;

	if (state == DOWNLOAD_BEGIN){
		SendMessageW(hProgress, PBM_SETRANGE32, 0, (LPARAM)size);
	} else if (state == DOWNLOAD_DATA){
		SendMessageW(hProgress, PBM_SETPOS, (WPARAM)progress, 0);
	} else if (state == DOWNLOAD_END){
		
		//Some dum case where it doesn't tell the size
		SendMessageW(hProgress, PBM_SETRANGE32, 0, (LPARAM)1);
		SendMessageW(hProgress, PBM_SETPOS, (WPARAM)1, 0);
		SendMessageW(hDlg, DOWNLOAD_DONE, 0, 0);
	} else if (state == DOWNLOAD_CANCELED){
		SendMessageW(hDlg, DOWNLOAD_DONE, 0, 0);
		return false;
	}
	return true;
}

INT_PTR CALLBACK SelfUpdateDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam){
	CFileDownload *pDownload;
	switch (uMsg){
		case WM_INITDIALOG:
			SetWindowLongPtrW(hDlg, GWLP_USERDATA, (LONG_PTR)lParam);
			pDownload = (CFileDownload*)lParam;
			if (!pDownload->Start(LAUNCHER_PACKAGE_URL, LAUNCHER_TEMP_PACKAGE_FILE, SelfUpdateCallback, (WPARAM)hDlg, (LPARAM)GetDlgItem(hDlg, IDC_DOWNLOADPROGRESS), true)){
				pDownload->Cancel();
				EndDialog(hDlg, 0);
			}
			return TRUE;
			break;
		case DOWNLOAD_DONE:
__user_canceled:
			if ((pDownload = (CFileDownload*)GetWindowLongPtrW(hDlg, GWLP_USERDATA))){
				if (pDownload->WasCanceled()){
					SetWindowTextW(GetDlgItem(hDlg, IDC_STATUS_TEXT), L"Update cancled by user.");
				} else 
					SetWindowTextW(GetDlgItem(hDlg, IDC_STATUS_TEXT), L"Update was successfully downloaded.");
				SetWindowTextW(GetDlgItem(hDlg, IDC_CANCEL_DONE), L"Close");
				SetWindowLongPtrW(hDlg, GWLP_USERDATA, (LONG_PTR)NULL);
			}
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam)){
		case IDC_CANCEL_DONE:
			if ((pDownload = (CFileDownload*)GetWindowLongPtrW(hDlg, GWLP_USERDATA))){
				pDownload->Cancel();
				goto __user_canceled;
			} else{
				EndDialog(hDlg, 0);
			}
			return TRUE;
		default:
			break;
			}
		case WM_CLOSE:
			//EndDialog(hDlg, 0); //Ignored
			return TRUE;
		default:
			return FALSE;
	}
	return FALSE;
}


bool CoreSelfUpdate(wchar_t* pBuf, size_32 bufsize, bool *pExec){
#ifdef TEST_NO_SELFUPDATE
	return true;
#else //TEST_NO_SELFUPDATE
	CMemoryDownload mdl;
	CFileDownload fdl;
	size_32 version;
	bool canceled;

	*pBuf = 0;
	*pExec = false;


	if (!mdl.Start(LAUNCHER_VERSION_FILE_URL, &version, 4, 4, NULL, 0, 0, false) || !mdl.Wait()){
		MessageBoxW(0, L"Failed to get launcher version", L"Error", MB_ICONERROR);
		return false;
	}

	if (version <= LAUNCHER_VERSION) return true;
	DialogBoxParamW(g_hInstance, MAKEINTRESOURCEW(IDD_SELFUPDATE), HWND_DESKTOP, SelfUpdateDlgProc, (LPARAM)&fdl); 

	canceled = !fdl.Wait();
	fdl.Close();

	GetCurrentDirectoryW(bufsize, pBuf);
	StrCatW(pBuf, L"\\");
	StrCatW(pBuf, LAUNCHER_TEMP_PACKAGE_FILE);

	return !canceled;
#endif //!TEST_NO_SELFUPDATE
}
