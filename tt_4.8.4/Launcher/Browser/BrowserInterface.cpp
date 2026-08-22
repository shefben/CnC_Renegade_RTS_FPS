#include <windows.h>
#include <stdio.h>
#include "../Shared/StdLib.h"
#include "../Shared/Memory.h"
#include "../Shared/Debug.h"
#include "Browser.h"
#include "BrowserAPI.h"
#include "InetProtocol.h"
#include <stdlib.h>

CDLLProtocolClassFactory	g_DllCF;
CDynProtocolClassFactory	g_DynCF;
CCallProtocolClassFactory	g_CallCF;

pfnDynProt	CBrowserCoreInterface::s_pfnDynProt		= NULL;
pfnCallProt	CBrowserCoreInterface::s_pfnCallProt	= NULL;
DWORD		CBrowserCoreInterface::s_InitFlags		= 0;


IStream* CBrowserCoreInterface::CallDynProtCB(const wchar_t *pURL, const wchar_t *pHandler, const wchar_t* const* ppArgs, int argc, IPOSTValues *pPOST){

#if _DEBUG
	if (wcscmp(pHandler, L"memstats") == 0)
		return GenerateMemStatsPage();
#endif //_DEBUG

	if (s_pfnDynProt)
		return s_pfnDynProt(pURL, pHandler, ppArgs, argc, pPOST);

	return NULL;
}

void CBrowserCoreInterface::CallCallProtCB(const wchar_t *pURL, const wchar_t *pHandler, const wchar_t* const* ppArgs, int argc, IPOSTValues *pPOST){
	if (s_pfnCallProt)
		s_pfnCallProt(pURL, pHandler, ppArgs, argc, pPOST);
}

CBrowserCoreInterface::CBrowserCoreInterface(){
}

CBrowserCoreInterface::~CBrowserCoreInterface(){
	if (s_InitFlags & BROWSER_REGISTER_DLL_PROT)
		GetInternetSession()->UnregisterNameSpace(&g_DllCF, L"dll");
	if (s_InitFlags & BROWSER_REGISTER_DYN_PROT)
		GetInternetSession()->UnregisterNameSpace(&g_DynCF, L"dyn");
	if (s_InitFlags & BROWSER_REGISTER_CALL_PROT)
		GetInternetSession()->UnregisterNameSpace(&g_CallCF, L"call");
}

void CBrowserCoreInterface::Intialize(DWORD flags, HINSTANCE hInstance) const{
	if (flags & BROWSER_REGISTER_DLL_PROT)
		GetInternetSession()->RegisterNameSpace(&g_DllCF, IID_NULL, L"dll", 0, NULL, 0);
	if (flags & BROWSER_REGISTER_DYN_PROT)
		GetInternetSession()->RegisterNameSpace(&g_DynCF, IID_NULL, L"dyn", 0, NULL, 0);
	if (flags & BROWSER_REGISTER_CALL_PROT)
		GetInternetSession()->RegisterNameSpace(&g_CallCF, IID_NULL, L"call", 0, NULL, 0);
	s_InitFlags = flags;
	CBrowser::InitBrowser(hInstance);
}

IBrowser* CBrowserCoreInterface::CreateIBrowser() const{
	return new CBrowser();
}

void CBrowserCoreInterface::RegisterDynProtCB(pfnDynProt _pfnDynProt) const{
	s_pfnDynProt = _pfnDynProt;
}

void CBrowserCoreInterface::RegisterCallProtCB(pfnCallProt _pfnCallProt) const{
	s_pfnCallProt = _pfnCallProt;
}

CBrowserCoreInterface	g_CBrowserCoreInterface;

const IBrowserCoreInterface* GetBrowserCoreInterface(){
	return &g_CBrowserCoreInterface;
}