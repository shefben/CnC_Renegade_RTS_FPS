#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <mmsystem.h>
#include <d3d9.h>

#include "Core.h"
#include "../Shared/StdLib.h"
#include "../Shared/Memory.h"
#include "../Shared/HTMLWrite.h"
#include "../Shared/MemoryStream.h"
#include <stdlib.h>
#include <stdio.h>

#include "../Downloader/DownloaderAPI.h"
#include "../Browser/BrowserAPI.h"
#include "../Browser/Browser.h"
#include "Settings.h"


IStream* TriggerDetectShow(IPOSTValues *pPostValues){
	LPDIRECT3D9 pD3D9;
	CMemoryBlockStream *pMemStream;
	CHTMLWriter* pWriter;
	UINT i, count;
	D3DADAPTER_IDENTIFIER9 identifier;

	pMemStream = NULL;
	pWriter = NULL;
	pD3D9 = NULL;

	if (!(pD3D9 = Direct3DCreate9(D3D_SDK_VERSION))) goto __cleanup_exit;
	if (!(pMemStream = new CMemoryBlockStream(4096))) goto __cleanup_exit;
	if (!(pWriter = new CHTMLWriter(pMemStream))) goto __cleanup_exit_stream;

	pWriter->OpenTag(L"html");
	pWriter->OpenTag(L"body");

	count = pD3D9->GetAdapterCount();

	for (i = 0; i < count; i++){
		pD3D9->GetAdapterIdentifier(i, 0, &identifier);
		pWriter->WriteStringFmt(L"Adapter %u/%u: %S", i+1, count, identifier.Description);
		pWriter->WriteClosedTag(L"br");
	}

	pWriter->CloseTag(); //body
	pWriter->CloseTag(); //html
	pWriter->Finalize();

	goto __cleanup_exit;
__cleanup_exit_stream:
__cleanup_exit:
	if (pD3D9)
		pD3D9->Release();

	if (pWriter)
		delete pWriter;

	return pMemStream;
}

IStream* TriggerDetectApply(IPOSTValues *pPostValues){
	return NULL;
}