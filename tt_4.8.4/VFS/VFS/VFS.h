#ifndef _VFS_H_
#define _VFS_H_

#include <windows.h>
#include "VFSStructures.h"
#include "../Shared/Memory.h"
#include "../Shared/Lists.h"
#include "../Shared/BaseTypes.h"
#include "../Shared/StdLib.h"
#include "API/VFSBase.h"
#include "API/VFSAdvanced.h"
#include "API/VFSInterface.h"
#include "API/VFSInterfaceV2.h"
#include "API/VFSCryptography.h"

class VFS;
class CDirectory;

PINTERNAL_MFT_ELEMENT Create_Element(const wchar_t *pName, size_64 ParentUID, UINT32 Flags, PINTERNAL_MFT_ELEMENT pDest);
bool ValidatePath(const wchar_t *pPath);
const wchar_t* TranslateVFSErrorCode(int error);

typedef struct _CACHEENTRY {
	void*	m_pCache;
	size_64	m_Size;
	bool	m_Free;
	int		m_ID;
} CACHEENTRY, *PCACHEENTRY;

class CMultiCache {
private:
	SList<CACHEENTRY>		m_Caches;
	CriticalSectionClass	m_CS;
	int						m_Count;
public:
	CMultiCache();
	~CMultiCache();
	void FreeCaches();
	void* Alloc(size_64 size);
	void Free(void *pMem);
};


class CEntry : public AutoPoolClass<CEntry, VFS_POOL_SIZES> {
	friend class VFS;
	friend class CFile;
private:
	PINTERNAL_MFT_ELEMENT					m_pThis_Element;
	SList2<CEntry>							m_Files;
	SList2<CEntry>							m_Directories;
	HashNode<CEntry*, HASHKEY>*				m_pThis_Child_Node;
	ExternNodeHashNode<CEntry*, HASHKEY>	m_This_Child_Node;

	HashNode<CEntry*, size_64>*				m_pThis_UID_Node;
	HashNode<CEntry*, HASHKEY>*				m_pThis_Name_Node;
	CEntry*									m_pParent;

	HASHKEY									m_HashKey;
	int										m_ID;
	static int								s_Cur_ID;
	ptr										m_pUserData;
	VFS*									m_pVFS;
public:
	CEntry*									m_pNext;
	CEntry*									m_pNext2;
	CEntry();
	~CEntry();
	void Free_Data();
	int Delete(bool DelFromParent, VFS *pVFS);
	void Add(CEntry *pEntry);
	void Remove(CEntry *pEntry);
	void SetVFS(VFS *pVFS);
	CEntry* FindChild(const wchar_t *pName);
	PINTERNAL_MFT_ELEMENT FindElement(const wchar_t *pName);
	PINTERNAL_MFT_ELEMENT GetElement();
	int UnrolledMakeFilePath(wchar_t* pDest, int DestSize, VFS* pVFS);
	CEntry* GetFirstChild();
	CEntry* GetFirstFile();
	CEntry* GetFirstDirectory();
	VFS_INLINE ptr GetUserData();

	bool Rename(VFS* pVFS, const wchar_t *pName);
	bool Reattach(CEntry* pEntry);
	bool IsChild(CEntry* pEntry, bool recursive);

#ifdef VFS_ENABLE_NO_ENUM
	bool NoEnum();
	bool UnrolledNoEnum();
#endif //VFS_ENABLE_NO_ENUM
};

//Primary VFS class (for file access)

class VFS : public IVFS {
private:
public:
	file									m_hMount;
	file									m_hFT;
	PCLUSTER								m_pCluster_Cache;
	

	VFS_HEADER								m_Header;

	CriticalSectionClass					m_Mount_Lock;

	bool									m_Names_Dirty;
	bool									m_Read_Only;
	bool									m_Opening;
	bool									m_Is_Open;

	size_64									m_UID;

	int										m_Last_Error;

	wchar_t*								m_pPath_Cache;
	wchar_t*								m_pPath_Cache2;
	void*									m_pCache;
	bool									m_FlatPriority;

#ifdef VFS_ENABLE_AES
	pfGetAESKey								m_pfGetAESKey;
	unsigned char*							m_pAESCache;
	unsigned char*							m_pAESWriteCache;
	size_64									m_AESCurrentCluster;
	int										m_AESCacheSize;
#endif //VFS_ENABLE_AES

	CMultiCache								m_MultiCache;



	SList3<CEntry>							m_Entries;
	HashList<CEntry*, size_64>*				m_pUID_Entries;
	
	SList2<INTERNAL_MFT_ELEMENT>			m_Elements;
	SList3<INTERNAL_MFT_ELEMENT>			m_Free_Elements;
	CEntry									m_Root_Dir;
	INTERNAL_MFT_ELEMENT					m_Root_Element;
	SList<OPEN_FILE>						m_Open_Files;

	wchar_t*								m_pDomain;

	ExternNodeHashList<CEntry*, HASHKEY>*	m_pChild_Entries;

	//Allocate clusters for target size
	PCLUSTER_HEADER Allocate_Clusters(size_64 size, bool flush, bool force_append, size_64 *pOffset, PCLUSTER_HEADER pClusters);
	//Load a cluster chain to memory
	PCLUSTER_HEADER Open_Cluster_Chain(size_64 base_cluster);
	//Close (free) a cluster chain
	void Free_Cluster_Chain(PCLUSTER_HEADER pChain, bool write);
	//Write a cluster chain
	void Write_Cluster_Chain(PCLUSTER_HEADER pChain);
	//Deallocate clusters
	void Release_Clusters(PCLUSTER_HEADER pClusters);
	//Return last cluster from a chain
	PCLUSTER_HEADER Get_Last_Cluster(PCLUSTER_HEADER pClusters);
	//Resize a cluster chain
	PCLUSTER_HEADER Resize_Clusters(PCLUSTER_HEADER pClusters, size_64 new_size, bool flush, size_64 *pOffset);
	//Read a raw cluster from a selected element
	size_64 Read_Raw_Cluster(PINTERNAL_MFT_ELEMENT pElement, PCLUSTER pCluster, UINT32 cluster);
	//Read a raw cluster by index
	size_64 Read_Indexed_Raw_Cluster(PCLUSTER pCluster, UINT32 cluster);

	//Read from clusters
	size_64 Cluster_Read(PINTERNAL_MFT_ELEMENT pElement, PCLUSTER_HEADER pClusterChain, size_64 offset, ptr pDest, size_64 size, PPROGESSCB pCB);
#ifdef VFS_ENABLE_AES
	size_64 AES_Cluster_Read(PINTERNAL_MFT_ELEMENT pElement, PCLUSTER_HEADER pClusterChain, size_64 offset, ptr pDest, size_64 size, PPROGESSCB pCB);
#endif //VFS_ENABLE_AES
	//Write to clusters
	size_64 Cluster_Write(PINTERNAL_MFT_ELEMENT pElement, PCLUSTER_HEADER* ppClusterChain, size_64 offset, ptr pSrc, size_64 size, PPROGESSCB pCB);
#ifdef VFS_ENABLE_AES
	size_64 AES_Cluster_Write(PINTERNAL_MFT_ELEMENT pElement, PCLUSTER_HEADER* ppClusterChain, size_64 offset, ptr pSrc, size_64 size, PPROGESSCB pCB);
#endif //VFS_ENABLE_AES
	
	//Allocate a directory element
	CEntry* Allocate_Directory(const wchar_t *pName, size_64 ParentUID, size_64 *pUID);
	//Find an entry by UID
	CEntry* Find_Entry_By_UID(size_64 UID);
	CEntry* Find_Entry(CEntry* pParent, const wchar_t *pName);
	//Locate a dir 
	CEntry* Locate_Directory(CEntry* pBaseDir, const wchar_t *pPath, const wchar_t **ppPathTokens, int TokenCount, bool create, bool *pCreated);
	//Find a file
	CEntry *Locate_File(CEntry* pBaseDir, const wchar_t *pPath, bool Create, bool *pCreated);
	//Allocate a file element
	CEntry* Allocate_File(const wchar_t *pName, size_64 ParentUID, size_64 *pUID);
	//Locate an entry (file or dir)
	CEntry* Locate_Entry(const wchar_t *pPath, const wchar_t **ppPathTokens, int TokenCount);
	//Find a file, create if not found
	PMFT_ELEMENT Find_File(const wchar_t *pPath, bool Create);
	//Link all data together in a post-opening pass
	bool Link_Data();
	//Free all data
	void Free_Data();
	//Load FT data
	bool Load_FT();
	//Allocate an MFT element
	PINTERNAL_MFT_ELEMENT Alloc_Element();
	//Pre-allocte elements, used in FT loading
	void Pre_Allocate_Elements();
	//Release an MFT element
	void Release_Element(const PINTERNAL_MFT_ELEMENT pElem);
	//Release file data
	void Clear_File(PMFT_ELEMENT pElem);
	//Offset based
	size_64 Get_Last_Cluster(size_64 Cluster);
	void Release_Clusters(size_64 Cluster);
	//Internal enum functions
	int RecursiveEntryEnum(CEntry *pThis, pfEnumCallback pCB, PVFSFILEINFO pFileInfo, ptr wParam, ptr lParam, int what, int *pCount, wchar_t *pThisPath);
	int EntryEnum(const wchar_t* pDirectory, pfEnumCallback, ptr wParam, ptr lParam, int what);

	void CreateDir(const wchar_t *pPath);

	VFSFileHandle InternalCreateFile(CEntry *pBaseDir, const wchar_t* pName, UINT32 Mode);
	bool InternalEmbedFile(CEntry* pDir, const wchar_t *pSrcFile, const wchar_t* pDestFile, bool OverWrite, PPROGESSCB pCB);
	void RecursiveCountProc(wchar_t *pThisName, const wchar_t *pFullPath, int *pCount);
	int RecursiveEmbedProc(CEntry* pDir, wchar_t *pThisName, const wchar_t *pFullPath, int total, int *pCount, PPROGESSCB pCB);

	
	bool InternalExtractFile(CEntry* pDir, const wchar_t *pSrcFile, const wchar_t* pDestFile, PPROGESSCB pCB);
	void RecursiveCountProc2(CEntry* pEntry, int *pCount);
	int RecursiveExtractProc(CEntry* pDir, wchar_t *pThisName, const wchar_t *pFullPath, int total, int *pCount, PPROGESSCB pCB);

	
	POPEN_FILE MemoryCreateFile(const wchar_t* pName, UINT32 Mode);
	POPEN_FILE MemoryCreateFiledataFile(ptr pFiledata, UINT32 Mode);
	void MemoryCloseFile(POPEN_FILE pFile);
	size_32 MemoryRead(POPEN_FILE pFile, void* pBuffer, size_32 Count, PPROGESSCB pCB);
	size_32 MemoryWrite(POPEN_FILE pFile, const void *pBuffer, size_32 Count, PPROGESSCB pCB);
	size_64 MemorySeek(POPEN_FILE pFile, ssize_64 Offset, int Origin);
	size_64 MemoryTell(POPEN_FILE pFile);
	size_64 MemorySize(POPEN_FILE pFile);
	bool MemoryEndOfFile(POPEN_FILE pFile);
	bool MemorySetEndOfFile(POPEN_FILE pFile);
	const void* MemoryMemCache(POPEN_FILE pFile);

	POPEN_FILE FlatCreateFile(const wchar_t* pName, UINT32 Mode);
	void FlatCloseFile(POPEN_FILE pFile);
	size_32 FlatRead(POPEN_FILE pFile, void* pBuffer, size_32 Count, PPROGESSCB pCB);
	size_32 FlatWrite(POPEN_FILE pFile, const void *pBuffer, size_32 Count, PPROGESSCB pCB);
	size_64 FlatSeek(POPEN_FILE pFile, ssize_64 Offset, int Origin);
	size_64 FlatTell(POPEN_FILE pFile);
	size_64 FlatSize(POPEN_FILE pFile);
	bool FlatEndOfFile(POPEN_FILE pFile);
	bool FlatSetEndOfFile(POPEN_FILE pFile);
	const void* FlatMemCache(POPEN_FILE pFile);

	POPEN_FILE ArchiveCreateFile(const wchar_t* pName, UINT32 Mode);
	POPEN_FILE ArchiveCreateFiledataFile(ptr pFiledata, UINT32 Mode);
	void ArchiveCloseFile(POPEN_FILE pFile);
	size_32 ArchiveRead(POPEN_FILE pFile, void* pBuffer, size_32 Count, PPROGESSCB pCB);
	size_32 ArchiveWrite(POPEN_FILE pFile, const void *pBuffer, size_32 Count, PPROGESSCB pCB);
	size_64 ArchiveSeek(POPEN_FILE pFile, ssize_64 Offset, int Origin);
	size_64 ArchiveTell(POPEN_FILE pFile);
	size_64 ArchiveSize(POPEN_FILE pFile);
	bool ArchiveEndOfFile(POPEN_FILE pFile);
	bool ArchiveSetEndOfFile(POPEN_FILE pFile);
	const void* ArchiveMemCache(POPEN_FILE pFile);
public:


	VFS();
	virtual ~VFS();
	virtual int GetVersion();

	virtual void PartialFlush();
	virtual void FullFlush();

	virtual bool Open(const wchar_t *pFilename, size_32 UserVersion, bool ReadOnly);
	virtual bool Create(const wchar_t *pFilename, size_32 UserVersion, size_32 ClusterSize);
	virtual void Close();
	virtual void SetOption(int option, void *pData, int datasize);
	
	virtual int GetLastError();

	virtual VFSFileHandle VFSCreateFile(const wchar_t* pName, UINT32 Mode);
	virtual VFSFileHandle VFSCreateFiledataFile(ptr pFiledata, UINT32 Mode);
	virtual VFSFileHandle VFSCreateUIDFile(size_64 UID, UINT32 Mode);
	virtual void VFSCloseFile(VFSFileHandle hFile);

	virtual size_32 VFSRead(VFSFileHandle hFile, void* pBuffer, size_32 Count, PPROGESSCB pCB);
	virtual size_32 VFSWrite(VFSFileHandle hFile, const void *pBuffer, size_32 Count, PPROGESSCB pCB);
	virtual size_64 VFSSeek(VFSFileHandle hFile, ssize_64 Offset, int Origin);
	virtual size_64 VFSTell(VFSFileHandle hFile);
	virtual size_64 VFSSize(VFSFileHandle hFile);
	virtual bool VFSEndOfFile(VFSFileHandle hFile);
	virtual bool VFSSetEndOfFile(VFSFileHandle hFile);
	virtual const void* VFSMemCache(VFSFileHandle hFile);
	virtual size_64 VFSGetUIDFromFileHandle(VFSFileHandle hFile);

	virtual bool VFSDeleteFiledataDirectory(ptr pDir);
	virtual bool VFSDeleteFiledataFile(ptr pFile);

	virtual int VFSEnum(const wchar_t* pDirectory, pfEnumCallback pCB, ptr wParam, ptr lParam, int what);
	virtual int VFSSingleEnum(ptr pFileData, pfEnumCallback pCB, ptr wParam, ptr lParam);
	virtual bool VFSEmbedFile(const wchar_t *pSrcFile, const wchar_t* pDestFile, bool OverWrite, PPROGESSCB pCB);
	virtual bool VFSExtractFile(const wchar_t *pSrcFile, const wchar_t* pDestFile, PPROGESSCB pCB);
	virtual bool VFSEmbedDir(const wchar_t *pSrcDir, const wchar_t* pDestDir, bool OverWrite, PPROGESSCB pCB);
	virtual bool VFSExtractDir(const wchar_t *pSrcDir, const wchar_t* pDestDir, bool OverWrite, PPROGESSCB pCB);


	virtual bool VFSEmbedFiledataFile(const wchar_t *pSrcFile, ptr pDestDir, bool OverWrite, PPROGESSCB pCB);
	virtual bool VFSExtractFiledataFile(ptr pSrcFile, const wchar_t* pDestFile, PPROGESSCB pCB);
	virtual bool VFSEmbedFiledataDir(const wchar_t *pSrcDir, ptr pDestDir, bool OverWrite, PPROGESSCB pCB);
	virtual bool VFSExtractFiledataDir(ptr pSrcDir, const wchar_t* pDestDir, bool OverWrite, PPROGESSCB pCB);

	virtual bool VFSQueryFileInfo(PVFSFILEINFO pFileInfo, ptr pFileData, wchar_t *pDest, int DestSize);
	virtual bool VFSSetFileUserData(ptr pFileData, ptr pUserData, size_32 size);
	virtual bool VFSGetFileUserData(ptr pFileData, ptr pUserData, size_32 size);
	virtual void VFSSetRuntimeUserData(ptr pFileData, ptr pUserData);
	virtual ptr VFSGetRuntimeUserData(ptr pFileData);
	virtual ptr VFSGetRootFiledata();
	virtual ptr VFSGetFileData(const wchar_t* pPath);
	virtual bool VFSFileExists(const wchar_t* pFile);
	virtual bool VFSPathExists(const wchar_t* pPath);
	virtual size_64 VFSFileSize(const wchar_t* pPath);

	virtual int VFSSetNoEnum(ptr pFileData, int noenum);
	virtual int VFSGetNoEnum(ptr pFileData);
	virtual void VFSSetFileOption(VFSFileHandle hFile, int fileoption, ptr pData, int size);
	virtual size_64 VFSGetFileUID(const wchar_t* pPath);
	virtual size_64 VFSGetDirectoryUID(const wchar_t* pPath);
	virtual size_64 VFSGetEntryUID(const wchar_t* pPath);
	virtual size_64 VFSGetFiledataUID(ptr pFileData);

	virtual bool VFSDeleteFile(const wchar_t *pFile);
	virtual bool VFSDeleteDirectory(const wchar_t *pDir);

	virtual int VFSGetSettings();

	virtual size_32 VFSGetClusterSize();
	virtual void DumpFTInfo();

	virtual const wchar_t* TranslateErrorCode(int error);

	virtual bool VFSGetMD5(const wchar_t* pFile, unsigned char *pMD5, bool Store);
	virtual bool VFSGetFileDataMD5(ptr pFileData, unsigned char *pMD5, bool Store);
	virtual bool VFSQueryMD5(const wchar_t* pFile, unsigned char *pMD5);
	virtual bool VFSQueryFileDataMD5(ptr pFileData, unsigned char *pMD5);

	
	virtual IStream* VFSCreateIStreamFile(const wchar_t* pName, UINT32 Mode);
	virtual IStream* VFSCreateIStreamFiledataFile(ptr pFiledata, UINT32 Mode);

	virtual bool VFSFileDataFileExists(ptr pFileData, const wchar_t* pFile);
	virtual bool VFSFileDataPathExists(ptr pFileData, const wchar_t* pPath);

	virtual bool VFSRenameFile(const wchar_t* pFileName, const wchar_t* pNewName);
	virtual bool VFSRenameFolder(const wchar_t* pPathName, const wchar_t* pNewName);

	virtual bool VFSRenameFiledataFile(ptr pFileData, const wchar_t* pNewName);
	virtual bool VFSRenameFiledataFolder(ptr pFileData, const wchar_t* pNewName);

	virtual ptr VFSCreateDirectory(ptr pFiledata, const wchar_t* pBaseName, wchar_t *pNewName, size_32 NewNameSize);

	virtual bool VFSRegisterForProtocol(const wchar_t* pDomain);
	virtual void VFSUnregisterForProtocol();
	virtual const wchar_t* VFSGetDomain();
	virtual ptr VFSFileDataFromFilehandle(VFSFileHandle hFile);
	virtual bool VFSMoveFiledataEntry(ptr pEntry, ptr pNewDir);
	virtual bool VFSMoveEntry(const wchar_t *pPath, ptr pNewDir);
};

/*class VFS2 : public IVFS2, public VFS {
public:
	VFS2();
	virtual ~VFS2();
	virtual int GetVersion();
};//*/


class CVFSIStream : public IStream {
private:
	
	IVFS*			m_pVFS;
	VFSFileHandle	m_hFile;
	ULONG			m_RefCount;

public:
	CVFSIStream(IVFS* pVFS, VFSFileHandle hFile);
	~CVFSIStream();

	//Interface functions

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

#endif //_VFS_H_
