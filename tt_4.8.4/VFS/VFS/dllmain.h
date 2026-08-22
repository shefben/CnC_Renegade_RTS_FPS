#ifndef _DLLMAIN_H_
#define _DLLMAIN_H_

#include "API/VFSBase.h"
#include "../Shared/Lists.h"

class CVFSCoreInterface : public IVFSCoreInterface{
private:
	wchar_t*	m_pNameSpace;
public:
	SList<IVFS>	m_VFSList;
	CVFSCoreInterface();
	~CVFSCoreInterface();
	virtual IVFS* CreateVFS();
	virtual IVFS2* CreateVFS2();
	virtual IVFS* CreateRPCVFS();
	virtual void RegisterProtocol(const wchar_t* pName);
	virtual void RegisterForProtocol(IVFS* pVFS);
	virtual void UnregisterForProtocol(IVFS* pVFS);

	IVFS* FindVFSForDomain(const wchar_t* pDomain);
};

extern "C" IVFSCoreInterface* VFS_CALL QueryCoreInterface();

#endif //!_DLLMAIN_H_
