#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <stdlib.h>
#include <stdio.h>


#include "../Shared/memory.h"
#include "../Shared/MemoryStream.h"
#include "main.h"
#include "../Shared/Archive.h"
#include "../Shared/StdLib.h"

void ParseCommandLine(wchar_t *pCmdLine, wchar_t **ppOut, wchar_t **ppVFS, wchar_t **ppExtern, wchar_t **ppVersions, bool *pZIP){
	int count, i;
	wchar_t* pOut[32];

	*ppOut = NULL;
	*ppVFS = NULL;
	*ppExtern = NULL;
	*ppVersions = NULL;

	count = TokenizeW(pCmdLine, pOut, 32, NULL, NULL, FALSE, FALSE, FALSE, FALSE);

	for (i = 0; i < count; i++){
		if (wcsnicmp(pOut[i], L"out:", 4) == 0)
			*ppOut = pOut[i]+4;
		else if (wcsnicmp(pOut[i], L"vfs:", 4) == 0)
			*ppVFS = pOut[i]+4;
		else if (wcsnicmp(pOut[i], L"extern:", 7) == 0)
			*ppExtern = pOut[i]+7;
		else if (wcsnicmp(pOut[i], L"zip:", 4) == 0)
			*pZIP = true;
		else if (wcsnicmp(pOut[i], L"versions:", 9) == 0)
			*ppVersions = pOut[i]+9;
	}
}

int __cdecl VersionSort(const void *p0, const void *p1){
	return *(size_32*)p0- *(size_32*)p1;
}

int MakeVersionList(const wchar_t*pOut, wchar_t *pList){
	wchar_t* pVersions[512];
	size_32 Versions[512];
	file f;
	int count, i, cur, c;
	wchar_t* pEnd;
	count = TokenizeW(pList, pVersions, 512, L",", NULL, FALSE, FALSE, FALSE, FALSE);
	if (!count) return 0;

	c = count;
	for (i = 0, cur = 0; i < count; i++){
		Versions[cur] = wcstol(pVersions[i], &pEnd, 10);

		if (pEnd == pVersions[i])
			c--;
		else
			cur++;
	}
	if ((count = c) == 0) return 5;
	qsort(Versions, c, sizeof(size_32), VersionSort);

	if ((f = fopen_W(pOut, L"w")) == INVALID_FILE) return 6;

	fwrite_(Versions, sizeof(size_32)*count, f);

	fclose_(f);
	return 0;
}

int _cdecl main(int argc, char**argv){
	wchar_t* pCmdLine;
	wchar_t *pOut;
	wchar_t *pVFS;
	wchar_t *pExtern;
	wchar_t *pVersions;
	bool zip;
	int res;

	res = 0;
	static wchar_t s_Buf[10*1024];

	if (!(pCmdLine = StrDupW(GetCommandLineW()))) return 1;

	ParseCommandLine(pCmdLine, &pOut, &pVFS, &pExtern, &pVersions, &zip);

	if (!pOut || !(pVFS || pExtern || pVersions) || ((pVFS || pExtern) && pVersions)){
		res = 3;
		goto __cleanup_exit;
	}

	if (pVersions)
		res = MakeVersionList(pOut, pVersions);
	else
		res = GenerateArchive(pOut, pVFS, pExtern, zip) ? 0 : 4;


__cleanup_exit:
	g_pFreeer(pCmdLine);
	return res;
}
