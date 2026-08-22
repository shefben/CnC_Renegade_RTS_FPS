#include <windows.h>
#include "Core.h"
#include "../Shared/StdLib.h"
#include "../Shared/Memory.h"

#include "../Downloader/DownloaderAPI.h"
#include "../Downloader/Downloader.h"
#include "../Shared/Archive.h"
#include <commctrl.h>

#include "resource.h"
#include "Settings.h"
#include "GameUpdate.h"
#include "resource.h"
#include <stdlib.h>
#include <stdio.h>

#define GAME_UPDATE_DONE		(WM_USER+0x100)


struct GameUpdateCookie {
	CFileDownload*	m_pFileDownload;
	size_32*		m_pUpdates;
	size_32			m_StartUpdate;
	size_32			m_EndUpdate;
	size_32			m_CurrentUpdate;
	HWND			m_hDlg;
	HWND			m_hProgress;
	HWND			m_hStatus;
	HWND			m_hDownloadState;
	HWND			m_hButton;
	int				m_Canceled;
	HANDLE			m_hThread;
};

IStream* SHARED_CALL VFSCreateIStream(const wchar_t *pName, void *pCookie){
	LogFormated(L"File: %s\n", pName);
	return g_pVFS->VFSCreateIStreamFile(pName, FILE_CREATE_ALWAYS | FILE_OPEN_WRITE);
}

bool __stdcall GameUpdateCallback(DWORD progress, DWORD size, DWORD state, WPARAM wParam, LPARAM lParam){
	HWND hProgress;
	GameUpdateCookie *pCookie;
	pCookie = (GameUpdateCookie*)wParam;
	hProgress = (HWND)lParam;

	if (state == DOWNLOAD_BEGIN){
		SendMessageW(hProgress, PBM_SETRANGE32, 0, (LPARAM)size);
	} else if (state == DOWNLOAD_DATA){
		SendMessageW(hProgress, PBM_SETPOS, (WPARAM)progress, 0);
	} else if (state == DOWNLOAD_END){
		
		//Some dum case where it doesn't tell the size
		SendMessageW(hProgress, PBM_SETRANGE32, 0, (LPARAM)1);
		SendMessageW(hProgress, PBM_SETPOS, (WPARAM)1, 0);
		//SendMessageW(hDlg, GAME_UPDATE_DONE, 0, 0);
	} else if (state == DOWNLOAD_CANCELED){
		//Handled by thread
		return false;
	}
	return true;
}
/*
Update thread
*/

DWORD CALLBACK UpdateThreadProc(void *pArg){
	GameUpdateCookie* pCookie;
	wchar_t lBuf[1024];
	wchar_t lBuf2[512];
	VFSFileHandle hFile;

	size_32 u, c, e;

	pCookie = (GameUpdateCookie*)pArg;
	pCookie->m_CurrentUpdate = 0;


	u = pCookie->m_EndUpdate-pCookie->m_StartUpdate;
	for (c = pCookie->m_StartUpdate, e = pCookie->m_EndUpdate; (c < e) && pCookie->m_Canceled == CANCEL_OK; c++){
		_snwprintf(lBuf, 1024, GAME_PACKAGE_URL_STRING, pCookie->m_pUpdates[c]);
		//Download here
		_snwprintf(lBuf2, 512, L"Downloading update %u of %u...", pCookie->m_CurrentUpdate+1, u);
		SetWindowTextW(pCookie->m_hStatus, lBuf2);

		if (!pCookie->m_pFileDownload->Start(lBuf, LAUNCHER_TEMP_PACKAGE_FILE, GameUpdateCallback, (WPARAM)pCookie, (LPARAM)pCookie->m_hProgress, false)){
			break;
		}
		if (pCookie->m_pFileDownload->WasCanceled() != DOWNLOAD_CANCEL_NOT){
			pCookie->m_Canceled = pCookie->m_pFileDownload->WasCanceled();
			pCookie->m_pFileDownload->Close();
			break;
		}
		pCookie->m_pFileDownload->Close();
		_snwprintf(lBuf2, 512, L"Installing update %u of %u...", pCookie->m_CurrentUpdate+1, u);
		SetWindowTextW(pCookie->m_hStatus, lBuf2);
		//Disable cancel button during patch installation to prevent VFS corruption
		EnableWindow(pCookie->m_hButton, FALSE);

		if (!ExtractExtern(g_ApplicationDir, LAUNCHER_TEMP_PACKAGE_FILE, NULL, NULL) || !ExtractVFS(LAUNCHER_TEMP_PACKAGE_FILE, VFSCreateIStream, NULL)){			
			pCookie->m_Canceled = CANCEL_ERROR;
			EnableWindow(pCookie->m_hButton, TRUE);
			break;
		}
		pCookie->m_CurrentUpdate++;
		pCookie->m_pFileDownload->Close();

		if ((hFile = g_pVFS->VFSCreateFile(GAME_VERSION_FILE, FILE_CREATE_ALWAYS | FILE_OPEN_WRITE)) == VFS_INVALID_FILE){
			EnableWindow(pCookie->m_hButton, TRUE);
			break;
		}
		g_pVFS->VFSWrite(hFile, &pCookie->m_pUpdates[c], sizeof(size_32), NULL);
		g_pVFS->VFSCloseFile(hFile);
		EnableWindow(pCookie->m_hButton, TRUE);
		//"Emulate" download delay
		//Sleep(1000);
	}
	PostMessageW(pCookie->m_hDlg, GAME_UPDATE_DONE, 0, 0);

	return 0;


}

INT_PTR CALLBACK GameUpdateDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam){
	GameUpdateCookie *pCookie;
	switch (uMsg){
		case WM_INITDIALOG:
			SetWindowLongPtrW(hDlg, GWLP_USERDATA, (LONG_PTR)lParam);
			pCookie = (GameUpdateCookie*)lParam;
			pCookie->m_hDlg = hDlg;
			pCookie->m_hProgress = GetDlgItem(hDlg, IDC_DOWNLOADPROGRESS);
			pCookie->m_hDownloadState = GetDlgItem(hDlg, IDC_SIZE_STATE);
			pCookie->m_hStatus = GetDlgItem(hDlg, IDC_STATUS_TEXT);
			pCookie->m_hButton = GetDlgItem(hDlg, IDC_CANCEL_DONE);
			ResumeThread(pCookie->m_hThread);
			Sleep(0);
			/*if (!pDownload->Start(LAUNCHER_PACKAGE_URL, LAUNCHER_TEMP_PACKAGE_FILE, SelfUpdateCallback, (WPARAM)hDlg, (LPARAM)GetDlgItem(hDlg, IDC_DOWNLOADPROGRESS), true)){
				pDownload->Cancel();
				EndDialog(hDlg, 0);
			}//*/
			return TRUE;
			break;
		case GAME_UPDATE_DONE:
__user_canceled:
			if ((pCookie = (GameUpdateCookie*)GetWindowLongPtrW(hDlg, GWLP_USERDATA))){
				if (WaitForSingleObject(pCookie->m_hThread, 2000) == WAIT_TIMEOUT){
				//if (WaitForSingleObject(pCookie->m_hThread, INFINITE) == WAIT_TIMEOUT){
					//Thread didn't exit within five seconds, terminate it
					TerminateThread(pCookie->m_hThread, 0);
				}

				SetWindowTextW(GetDlgItem(hDlg, IDC_SIZE_STATE), L"");
				if (pCookie->m_Canceled == CANCEL_USER){
					SetWindowTextW(GetDlgItem(hDlg, IDC_STATUS_TEXT), L"Update cancled by user.");
					EndDialog(hDlg, 0);
					return 0;
				} else if (pCookie->m_Canceled == CANCEL_ERROR){
					SetWindowTextW(GetDlgItem(hDlg, IDC_STATUS_TEXT), L"The update process was cancelled because of an error.");
				} else 
					SetWindowTextW(GetDlgItem(hDlg, IDC_STATUS_TEXT), L"All updates were installed.");
				SetWindowTextW(GetDlgItem(hDlg, IDC_CANCEL_DONE), L"Close");
				SetWindowLongPtrW(hDlg, GWLP_USERDATA, (LONG_PTR)NULL);
			}
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam)){
		case IDC_CANCEL_DONE:
			if ((pCookie = (GameUpdateCookie*)GetWindowLongPtrW(hDlg, GWLP_USERDATA))){
				SuspendThread(pCookie->m_hThread);
				if (MessageBoxW(0,
					L"Do you really want to cancel the update process?\nYou have to update to the latest version if you wish to play the game.",
					L"Update cancel", MB_YESNO) == IDYES){
					pCookie->m_Canceled = CANCEL_USER;
					pCookie->m_pFileDownload->Cancel();
					ResumeThread(pCookie->m_hThread);
					goto __user_canceled;
				}
				ResumeThread(pCookie->m_hThread);
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

size_32 GetLocalVersion(){
	VFSFileHandle hFile;
	size_32 version;

	hFile = g_pVFS->VFSCreateFile(GAME_VERSION_FILE, FILE_OPEN_EXISTING | FILE_OPEN_READ);
	if (hFile == VFS_INVALID_FILE) return 0;

	if (g_pVFS->VFSRead(hFile, &version, sizeof(size_32), NULL) != sizeof(size_32)){
		version = 0;
		goto __exit;
	}
__exit:
	g_pVFS->VFSCloseFile(hFile);
	return version;
}

int GameUpdate(){
	size_32 lversion;
	CMemoryDownload mdl;
	CFileDownload fdl;
	GameUpdateCookie cookie;

	size_32* pVersions;
	DWORD tid;
	int res;

	int i, count;

	lversion = GetLocalVersion();

	if (!mdl.Start(GAME_VERSION_FILE_URL, NULL, 0, 1024*1024, NULL, NULL, NULL, false) || !mdl.Wait()) return CANCEL_ERROR;

	mdl.LockBuffer();

	if (mdl.GetSize() == 0){
		mdl.UnlockBuffer();
		mdl.Close();
		return CANCEL_OK;
	}

	if ((mdl.GetSize() % 4) != 0){
		mdl.UnlockBuffer();
		mdl.Close();
		return CANCEL_ERROR;
	}

	pVersions = (size_32*)mdl.GetBuffer();

	count = mdl.GetSize()/sizeof(size_32);

	for (i = 0; i < count; i++){
		if (lversion < pVersions[i])
			break;
	}

	//Check if we're latest or newer (test builds)
	if (i == count){
		mdl.UnlockBuffer();
		mdl.Close();
		return CANCEL_OK;
	}
	cookie.m_Canceled = CANCEL_OK;
	cookie.m_CurrentUpdate = 0;
	cookie.m_hDlg = NULL;
	cookie.m_pFileDownload = &fdl;
	cookie.m_pUpdates = pVersions;
	cookie.m_StartUpdate = i;
	cookie.m_EndUpdate = count;
	if (cookie.m_hThread = CreateThread(NULL, 0, UpdateThreadProc, &cookie, CREATE_SUSPENDED, &tid)){
		DialogBoxParamW(g_hInstance, MAKEINTRESOURCEW(IDD_GAMEUPDATE), HWND_DESKTOP, GameUpdateDlgProc, (LPARAM)&cookie);
		res = (cookie.m_Canceled == CANCEL_OK);
		TerminateThread(cookie.m_hThread, 0);
		CloseHandle(cookie.m_hThread);
		res = cookie.m_Canceled;
	}else
		res = CANCEL_ERROR;

	mdl.UnlockBuffer();
	mdl.Close();


	return res;
}