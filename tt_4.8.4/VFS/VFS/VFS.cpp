#include <windows.h>
#include "VFS.h"
#include "VFSStructures.h"
#include "../Shared/Memory.h"
#include "../Shared/Lists.h"
#include "../Shared/BaseTypes.h"
#include "../Shared/StdLib.h"
#include "../Shared/MD5.h"
#include "API/VFSErrorcodes.h"
#include "API/VFSVersion.h"

#ifdef VFS_ENABLE_AES
#include "../Shared/AES/aes.h"
#endif //VFS_ENABLE_AES

#include <stdio.h>

const wchar_t* TranslateVFSErrorCode(int error){
	static wchar_t s_Buf[16];
	switch (error){
		case VFS_OK:							return L"Success";
		case VFS_FAILED_TO_CREATE_FILE:			return L"Failed to create archieve file";
		case VFS_FAILED_TO_OPEN_FILE:			return L"Failed to open archieve file";
		case VFS_READ_NO_CLUSTERS:				return L"No clusters exist";
		case VFS_CANNOT_OPEN_CHAIN:				return L"Failed to open cluster chain";
		case VFS_OUT_OF_BOUNDS:					return L"Out of boundaries";
		case VFS_CANNOT_ALLOCATE:				return L"Failed to allocate clusters";
		case VFS_CANNOT_RESIZE:					return L"Failed to resize clusters";
		case VFS_CANNOT_CREATE_CACHE:			return L"Failed to create cache";
		case VFS_INVALID_VERSION:				return L"Invalid version";
		case VFS_INVALID_FOURC_CC:				return L"Invalid FourCC";
		case VFS_INVALID_USER_VERSION:			return L"Invalud user version";
		case VFS_READ_ONLY:						return L"Read only";
		case VFS_PARSING_FAILURE:				return L"Parse failure";
		case VFS_CANNOT_CREATE_FILE:			return L"Failed to create file";
		case VFS_CANNOT_OPEN_FILE:				return L"Failed to open file";
		case VFS_WRITE_LOCKED_FILE:				return L"File is write locked";
		case VFS_READ_LOCKED_FILE:				return L"File is read locked";
		case VFS_INVALID_OPEN_MODE:				return L"Invalid open mode";
		case VFS_FILE_IN_USE:					return L"File in use";
		case VFS_FILE_EXISTS:					return L"File already exists";
		case VFS_FAILED_TO_OPEN_CLUSTER_FILE:	return L"Failed to open cluster file";
		case VFS_FAILED_TO_CREATE_CLUSTER_FILE:	return L"Failed to create cluster file";
		case VFS_CANNOT_FIND_ENTRY:				return L"Failed to locate entry";
		case VFS_ERROR_NOT_A_FILE:				return L"Entry is not a file";
		case VFS_ERROR_NOT_A_DIRECTORY:			return L"Entry is not a directory";
		case VFS_ERROR_INVALID_UID:				return L"Invalid entry UID";
		default:								return swprintf(s_Buf, L"Unknown error code: %.8X", error) ? s_Buf : s_Buf;
	}
}

const wchar_t* Get_Type_Name(int type){
	switch (type){
		case MFT_ELEMENT_UNUSED:
			return L"Unused";
		case MFT_ELEMENT_DIRECTORY:
			return L"Directory";
		case MFT_ELEMENT_FILE:
			return L"File";
		default:
			return L"Unknown";
	}
}

bool ValidatePath(const wchar_t *pPath){
	int len, i, totallen, tokencount;
	bool res;
	const wchar_t *pTokens[128];
	wchar_t *pTemp, *_pPath;

	res = true;
	if (StrLenW(pPath) > ((1024*32)-1)) return false;
	_pPath = NULL;
	_pPath = StrDupW(pPath);
	if (!_pPath) return false;
	tokencount = Get_Path_Token_CountW(_pPath);
	if (tokencount < 1 || tokencount > 128) goto __exit;

	totallen = 0;
	pTemp = _pPath;
	for (i = 0; i < tokencount; i++){
		pTemp = Get_Path_TokenW(pTemp, &len, 0, FALSE);
		if (!pTemp || len > 255) goto __exit;
		*(pTemp+len) = 0;
		pTokens[i] = pTemp;
		pTemp += len + 1;
	}
	res = true;
__exit:
	if (_pPath) g_pFreeer(_pPath);
	return res;
}

const wchar_t* Get_Type_Name2(int type){
	switch (type){
		case MFT_ELEMENT_UNUSED:
			return L"unused";
		case MFT_ELEMENT_DIRECTORY:
			return L"directory";
		case MFT_ELEMENT_FILE:
			return L"file";
		default:
			return L"Uknown";
	}
}

const wchar_t* Get_Type_Namep(int type){
	switch (type){
		case MFT_ELEMENT_UNUSED:
			return L"Unused   ";
		case MFT_ELEMENT_DIRECTORY:
			return L"Directory";
		case MFT_ELEMENT_FILE:
			return L"File     ";
		default:
			return L"Uknown";
	}
}

const wchar_t* Get_Type_Name2p(int type){
	switch (type){
		case MFT_ELEMENT_UNUSED:
			return L"unused   ";
		case MFT_ELEMENT_DIRECTORY:
			return L"directory";
		case MFT_ELEMENT_FILE:
			return L"file     ";
		default:
			return L"Uknown   ";
	}
}

PVFSFILEINFO MFTElementToFileInfo(PVFSFILEINFO pInfo, PINTERNAL_MFT_ELEMENT pElem, wchar_t *pThisPath, int *pNewLen, wchar_t *pPathCache, CEntry* pEntry){
	pInfo->m_Type = pElem->m_Element.m_Type;
	pInfo->m_pName = pElem->m_Element.m_Name;
	pInfo->m_pFullName = pPathCache;
	pInfo->m_pFileData = (ptr)pEntry;
	pInfo->m_pUserData = pEntry->GetUserData();
	pInfo->m_Flags = pElem->m_Element.m_Flags;
	pThisPath[0] = '\\';
	StrCpyW(pThisPath+1, pElem->m_Element.m_Name);
	if (*pNewLen) *pNewLen = StrLenW(pElem->m_Element.m_Name)+1;
	pInfo->m_Size = (pElem->m_Element.m_Type == MFT_ELEMENT_FILE) ? pElem->m_Element.m_File_Size : 0;
	return pInfo;
}

PVFSFILEINFO MFTElementToFileInfo2(PVFSFILEINFO pInfo, PINTERNAL_MFT_ELEMENT pElem, CEntry* pEntry){
	pInfo->m_Type = pElem->m_Element.m_Type;
	pInfo->m_pName = pElem->m_Element.m_Name;
	pInfo->m_pFullName = pElem->m_Element.m_Name;
	pInfo->m_pFileData = (ptr)pEntry;
	pInfo->m_pUserData = pEntry->GetUserData();
	pInfo->m_Flags = pElem->m_Element.m_Flags;
	pInfo->m_Size = (pElem->m_Element.m_Type == MFT_ELEMENT_FILE) ? pElem->m_Element.m_File_Size : 0;
	return pInfo;
}

PINTERNAL_MFT_ELEMENT Create_Element(const wchar_t *pName, size_64 ParentUID, UINT32 Flags, PINTERNAL_MFT_ELEMENT pDest){

	if (StrLenW(pName) > NAME_LENGTH) return NULL;
	if (!pDest)
		pDest = new INTERNAL_MFT_ELEMENT;
	if (!pDest) return NULL;

	memset(&pDest->m_Element, 0, sizeof(MFT_ELEMENT));

	pDest->m_Element.m_First_Cluster = NO_CLUSTERS;
	pDest->m_Element.m_File_Size = 0;
	pDest->m_Element.m_This_Element = 0;
	pDest->m_Element.m_Ref_Count = 0;
	pDest->m_Element.m_Parent_UID = ParentUID;
	pDest->m_Element.m_Checksum = 0;
	pDest->m_Element.m_Flags = Flags;

	pDest->m_Element.m_GotMD5 = 0;

	pDest->m_pNext = NULL;
	pDest->m_pNext2 = NULL;
	pDest->m_pCluster_Chain = NULL;
	StrnCpyW(pDest->m_Element.m_Name, pName, NAME_LENGTH+1);
	return pDest;
}


PINTERNAL_MFT_ELEMENT Reset_Element(PINTERNAL_MFT_ELEMENT pDest){
	size_64 offset;
	offset = pDest->m_Element.m_This_Element;
	memset(&pDest->m_Element, 0, sizeof(MFT_ELEMENT));
	pDest->m_Element.m_This_Element = offset;
	pDest->m_Element.m_First_Cluster = NO_CLUSTERS;
	pDest->m_Element.m_GotMD5 = 0;
	return pDest;
}


CMultiCache::CMultiCache(){
	m_Count = 0;
}

CMultiCache::~CMultiCache(){
	FreeCaches();
}

void CMultiCache::FreeCaches(){
	GenericSLNode<CACHEENTRY>* pNode;
	CriticalSectionClass::LockClass lock(&m_CS);
	pNode = m_Caches.Get_Head();
	while (pNode){
		g_pFreeer(pNode->m_pData->m_pCache);
		g_pFreeer(pNode->m_pData);
		pNode = pNode->m_pNext;
	}

	m_Caches.Remove_All();
}

void* CMultiCache::Alloc(size_64 size){
	GenericSLNode<CACHEENTRY>* pNode;
	PCACHEENTRY pEntry;
	CriticalSectionClass::LockClass lock(&m_CS);
	size = ((size/VFS_CACHE_ALIGN)+((size%VFS_CACHE_ALIGN) ? 1 : 0))*VFS_CACHE_ALIGN;
	if (size > VFS_MAX_CACHE_SIZE)
		return NULL;
	
	pNode = m_Caches.Get_Head();
	while (pNode){
		if (pNode->m_pData->m_Free && pNode->m_pData->m_Size >= size){
			pNode->m_pData->m_Free = false;
			return pNode->m_pData->m_pCache;
		}
		pNode = pNode->m_pNext;
	}
	pEntry = (PCACHEENTRY)g_pAllocator(sizeof(CACHEENTRY));
	if (!pEntry) return NULL;
	pEntry->m_Free = false;
	pEntry->m_pCache = g_pAllocator((size_t)size);
	pEntry->m_Size = size;
	m_Caches.Add_Tail(pEntry);
	return pEntry->m_pCache;
}

void CMultiCache::Free(void *pMem){
	GenericSLNode<CACHEENTRY>* pNode;
	CriticalSectionClass::LockClass lock(&m_CS);
	pNode = m_Caches.Get_Head();
	while (pNode){
		if (pNode->m_pData->m_pCache == pMem){
			pNode->m_pData->m_Free = true;
			return;
		}
		pNode = pNode->m_pNext;
	}
}

int CEntry::s_Cur_ID = 0;

CEntry::CEntry(){
	m_pVFS = NULL;
	//m_pChilds = new HashList<CEntry*, HASHKEY>(0x80);
	m_pThis_Child_Node = NULL;
	m_pThis_UID_Node = NULL;
	m_pThis_Name_Node = NULL;
	m_pParent = NULL;
	m_HashKey = 0;
	m_ID = s_Cur_ID++;
	m_pUserData = NULL;

	//m_pNext = m_pNext = NULL;
}

CEntry::~CEntry(){
	//delete m_pChilds;
}

void CEntry::Free_Data(){
	//m_pChilds->FreeData(false);
	m_Directories.Remove_All();
	m_Files.Remove_All();
	m_pThis_Child_Node = NULL;
	m_pThis_UID_Node = NULL;
	m_pThis_Name_Node = NULL;
	m_pParent = NULL;
	m_HashKey = 0;
}

int CEntry::Delete(bool DelFromParent, VFS *pVFS){
	int retval;
	CEntry *pNode, *pTemp;

	if (!pVFS) return false;
	retval = VFS_DELETE_SUCCESS;
	if (m_pThis_Element->m_Element.m_Type == MFT_ELEMENT_DIRECTORY){

		pNode = m_Directories.Get_Head();

		while (pNode){
			pTemp = pNode->m_pNext;
			if (pNode->Delete(true, pVFS) == VFS_DELETE_FAILED)
				retval = (retval == VFS_DELETE_SOME) ? VFS_DELETE_SOME : VFS_DELETE_FAILED;
			else {
				retval = (retval == VFS_DELETE_SUCCESS) ? VFS_DELETE_SUCCESS : VFS_DELETE_SOME;
				pVFS->m_Entries.Remove(pNode);
				pVFS->m_pUID_Entries->Delete(pNode->m_pThis_Element->m_Element.m_UID, pNode);
				pVFS->m_pChild_Entries->Delete(pNode->m_HashKey, pNode);
				pVFS->Release_Element(pNode->m_pThis_Element);
				delete pNode;
				
			} 

			pNode = pTemp;
		}

		pNode = m_Files.Get_Head();

		while (pNode){
			pTemp = pNode->m_pNext;
			if (pNode->Delete(true, pVFS) == VFS_DELETE_FAILED)
				retval = (retval == VFS_DELETE_SOME) ? VFS_DELETE_SOME : VFS_DELETE_FAILED;
			else {
				retval = (retval == VFS_DELETE_SUCCESS) ? VFS_DELETE_SUCCESS : VFS_DELETE_SOME;
				pVFS->m_Entries.Remove(pNode);
				pVFS->m_pUID_Entries->Delete(pNode->m_pThis_Element->m_Element.m_UID, pNode);
				pVFS->m_pChild_Entries->Delete(pNode->m_HashKey, pNode);
				pVFS->Release_Element(pNode->m_pThis_Element);
				delete pNode;
				
			} 

			pNode = pTemp;
		}

	} else if (m_pThis_Element->m_Element.m_Type == MFT_ELEMENT_FILE)
		retval = (m_pThis_Element->m_Element.m_Ref_Count == 0) ? VFS_DELETE_SUCCESS : VFS_DELETE_FAILED;

	if (retval >= VFS_DELETE_SUCCESS){


		if (DelFromParent && m_pParent)
			m_pParent->Remove(this);

		if (pVFS){
			pVFS->m_Entries.Remove(this);
			pVFS->Release_Element(m_pThis_Element);
		}
	}
	if (retval >= VFS_DELETE_SUCCESS)
		Free_Data();
	return retval;
}

void CEntry::Add(CEntry *pEntry){
	if (pEntry->m_pThis_Element->m_Element.m_Type == MFT_ELEMENT_DIRECTORY)
		m_Directories.Add_Head(pEntry);
	else if (pEntry->m_pThis_Element->m_Element.m_Type == MFT_ELEMENT_FILE)
		m_Files.Add_Head(pEntry);

	pEntry->m_pThis_Element->m_Element.m_Parent_UID = m_pThis_Element->m_Element.m_UID;
	pEntry->m_pThis_Element->m_Element.m_Dirty = 1;
	pEntry->m_pParent = this;
}

void CEntry::Remove(CEntry *pEntry){
	//m_pChilds->Delete(pEntry->m_HashKey, pEntry);
	if (pEntry->m_pThis_Element->m_Element.m_Type == MFT_ELEMENT_DIRECTORY)
		m_Directories.Remove(pEntry);
	else if (pEntry->m_pThis_Element->m_Element.m_Type == MFT_ELEMENT_FILE)
		m_Files.Remove(pEntry);

	pEntry->m_pParent = NULL;
}

void CEntry::SetVFS(VFS *pVFS){
	m_pVFS = pVFS;
}

CEntry* CEntry::FindChild(const wchar_t *pName){
	/*HASHKEY hKey;
	HashNode<CEntry*, HASHKEY> *pHashNode;
	hKey = Get_Str_HashW(pName, TRUE);
	if ((pHashNode = m_pChilds->Search(hKey))){
		if (wcsicmp(pName, pHashNode->m_Data->m_pThis_Element->m_Element.m_Name) == 0)
			return pHashNode->m_Data;

		while ((pHashNode = m_pChilds->SearchContinue(pHashNode, hKey)))
			if (wcsicmp(pName, pHashNode->m_Data->m_pThis_Element->m_Element.m_Name) == 0)
				return pHashNode->m_Data;
	}//*/
	return NULL;
}

PINTERNAL_MFT_ELEMENT CEntry::FindElement(const wchar_t *pName){
	return NULL;
}

PINTERNAL_MFT_ELEMENT CEntry::GetElement(){
	return m_pThis_Element;
}

int CEntry::UnrolledMakeFilePath(wchar_t* pDest, int DestSize, VFS* pVFS){
	int offset, len;
	offset = 0;
	if (m_pParent && &pVFS->m_Root_Dir != m_pParent)
		if (offset = m_pParent->UnrolledMakeFilePath(pDest, DestSize, pVFS) == -1) return -1;

	len = StrLenW(m_pThis_Element->m_Element.m_Name);
	if (len + offset + 1 > DestSize) return -1; 
	memcpy(pDest+offset, m_pThis_Element->m_Element.m_Name, sizeof(wchar_t)*(len+1));
	return offset + len;
}

CEntry* CEntry::GetFirstChild(){
	return NULL;//m_Childs.Get_Head();
}

CEntry* CEntry::GetFirstFile(){
	return m_Files.Get_Head();
}

CEntry* CEntry::GetFirstDirectory(){
	return m_Directories.Get_Head();
}

ptr CEntry::GetUserData(){
	return m_pUserData;
}

bool CEntry::Rename(VFS* pVFS, const wchar_t *pName){
	if (!m_pParent || m_pThis_Element->m_Element.m_UID == ROOT_UID) return false;
	if (pVFS->VFSFileDataFileExists(m_pParent, pName) || pVFS->VFSFileDataPathExists(m_pParent, pName)) return false;

	pVFS->m_pChild_Entries->Delete(m_HashKey, this);
	m_HashKey = Get_Str_HashW(pName, TRUE);
	pVFS->m_pChild_Entries->Insert(m_HashKey, &m_This_Child_Node, this);

	StrnCpyW(m_pThis_Element->m_Element.m_Name, pName, NAME_LENGTH);
	m_pThis_Element->m_Element.m_Dirty = 1;
	pVFS->PartialFlush();

	return true;
}

bool CEntry::Reattach(CEntry* pEntry){
	if (!m_pParent || m_pThis_Element->m_Element.m_UID == ROOT_UID) return false;
	if (IsChild(this, true)) return false;
	m_pParent->Remove(pEntry);
	pEntry->Add(this);
	return true;
}

bool CEntry::IsChild(CEntry* pEntry, bool recursive){
	CEntry* _pEntry;
	if (m_Files.Is_In_List(pEntry)) return true;

	if (m_Directories.Is_In_List(pEntry)) return true;

	if (recursive){
		_pEntry = m_Directories.Get_Head();

		while (_pEntry){
			if (_pEntry->IsChild(pEntry, true)) return true;
			_pEntry = _pEntry->m_pNext;
		}
	}
	return false;
}

#ifdef VFS_ENABLE_NO_ENUM
bool CEntry::NoEnum(){
	if (m_pThis_Element->m_Element.m_No_Enum != MFT_ELEMENT_ENUM) return true;
	return (m_pParent) ? m_pParent->UnrolledNoEnum() : false;
}

bool CEntry::UnrolledNoEnum(){
	if (m_pThis_Element->m_Element.m_No_Enum == MFT_ELEMENT_NO_ENUM_ALL) return true;
	return (m_pParent) ? m_pParent->UnrolledNoEnum() : false;
}
#endif //VFS_ENABLE_NO_ENUM

VFS::VFS(){
	m_hMount = INVALID_FILE;
	m_hFT = INVALID_FILE;
	m_pCluster_Cache = NULL;
	m_pPath_Cache = (wchar_t*)g_pAllocator(sizeof(wchar_t)*VFS_PATH_CACHE_SIZE);
	m_pPath_Cache2 = (wchar_t*)g_pAllocator(sizeof(wchar_t)*VFS_PATH_CACHE_SIZE);
	m_pPath_Cache[0] = 0;
	m_pCache = g_pAllocator(VFS_CACHE_SIZE);
	m_pChild_Entries = NULL;
	m_pDomain = NULL;
#ifdef VFS_ENABLE_AES
	m_pAESCache = m_pAESWriteCache = NULL;
#endif //VFS_ENABLE_AES
	m_Is_Open = false;
}

VFS::~VFS(){
	Close();
	g_pFreeer(m_pPath_Cache);
	g_pFreeer(m_pPath_Cache2);
	g_pFreeer(m_pCache);
	m_pPath_Cache = NULL;
	VFSUnregisterForProtocol();
}

int VFS::GetVersion(){
	return VFS_INTERFACE_FLAT;
}

bool VFS::Open(const wchar_t *pFilename, size_32 UserVersion, bool ReadOnly){
	wchar_t* pTemp;
	Close();
	CriticalSectionClass::LockClass lock(&m_Mount_Lock);

	m_hFT = fopen_W(pFilename, ReadOnly ? L"r" : L"r+");
	//MessageBoxW(0, ReadOnly ? L"Read only" : L"Read/write", 0, 0);
	if (!VALID_FILE(m_hFT)){
		m_Last_Error = VFS_FAILED_TO_OPEN_FILE;
		return false;
	}

	
	fread_(&m_Header, sizeof(VFS_HEADER), m_hFT);

	if (m_Header.m_Four_CC != VFS_FOUR_CC){
		m_Last_Error = VFS_INVALID_FOURC_CC;
		fclose_(m_hFT);
		m_hFT = INVALID_FILE;
		return false;
	}

	if (m_Header.m_Version != VFS_VERSION){
		m_Last_Error = VFS_INVALID_VERSION;
		fclose_(m_hFT);
		m_hFT = INVALID_FILE;
		return false;
	}

#ifndef VFS_NO_USERVERSION_CHECK
	if (m_Header.m_User_Version != UserVersion){
		m_Last_Error = VFS_INVALID_USER_VERSION;
		fclose_(m_hFT);
		m_hFT = INVALID_FILE;
		return false;
	}
#endif //!VFS_NO_USERVERSION_CHECK

	
	StrCpyW(m_pPath_Cache, pFilename);
	/*StrCpyW(m_Header.m_Cluster_Name+StrLenW(m_Header.m_Cluster_Name)-3, L"CLU");//*/
	pTemp = (wchar_t*)ULongToPtr(max(PtrToUlong(StrrChrW(m_pPath_Cache, '/')), PtrToUlong(StrrChrW(m_pPath_Cache, '\\'))));
	if (!pTemp) pTemp = m_pPath_Cache-1;
	StrCpyW(pTemp+1, m_Header.m_Cluster_Name);//*/
	
	m_hMount = fopen_W(m_pPath_Cache, ReadOnly ? L"r" : L"r+");
	if (!VALID_FILE(m_hMount)){
		MessageBoxW(0, m_pPath_Cache, 0, 0);
		fclose_(m_hFT);
		m_hFT = INVALID_FILE;
		m_hMount = INVALID_FILE;
		m_Last_Error = VFS_FAILED_TO_OPEN_CLUSTER_FILE;
		return false;
	}

	m_Read_Only = ReadOnly;
	//fread_(&m_Header, sizeof(VFS_HEADER), m_hFT);

	/*if (m_Header.m_Four_CC != VFS_FOUR_CC){
		m_Last_Error = VFS_INVALID_FOURC_CC;
		return false;
	}

	if (m_Header.m_Version != VFS_VERSION){
		m_Last_Error = VFS_INVALID_VERSION;
		return false;
	}

	if (m_Header.m_User_Version != UserVersion){
		m_Last_Error = VFS_INVALID_USER_VERSION;
		return false;
	}//*/

	m_pCluster_Cache = (PCLUSTER)g_pAllocator(sizeof(struct CLUSTER_HEADER)+m_Header.m_Cluster_Size);
	if (!m_pCluster_Cache){
		m_Last_Error = VFS_CANNOT_CREATE_CACHE;
		Close();
		return false;
	}
	m_Read_Only = ReadOnly;
	m_Names_Dirty = false;
	
	m_pUID_Entries = new HashList<CEntry*, size_64>(0x100);
	m_pChild_Entries = new ExternNodeHashList<CEntry*, HASHKEY>(0xFFFF);

#ifdef VFS_ENABLE_AES
	m_AESCacheSize = m_Header.m_Cluster_Size;
	if (m_AESCacheSize & (AES_BLOCK_SIZE-1))
		m_AESCacheSize = (m_AESCacheSize+AES_BLOCK_SIZE) & ~((AES_BLOCK_SIZE-1));
	m_pAESCache = (unsigned char*)g_pAllocator(m_AESCacheSize);
	m_pAESWriteCache = (unsigned char*)g_pAllocator(m_AESCacheSize);
	memset(m_pAESCache, 0, m_AESCacheSize);
	memset(m_pAESWriteCache, 0, m_AESCacheSize);
#endif //VFS_ENABLE_AES

	fseek_64(m_hFT, 0, SEEK_SET);
	m_UID = 0;
	Create_Element(L"$ROOT$", ROOT_UID, MFT_ELEMENT_FLAG_DIRECTORY, (m_Root_Dir.m_pThis_Element = &m_Root_Element));
	m_pUID_Entries->Insert(m_Root_Dir.m_pThis_Element->m_Element.m_UID, &m_Root_Dir.m_pThis_UID_Node, &m_Root_Dir);

	if (!Load_FT()){
		Close();
		m_Last_Error = VFS_PARSING_FAILURE;
		return false;
	}
	
	
	m_Last_Error = VFS_OK;
	m_Opening = false;
	m_FlatPriority = true;
	m_Is_Open = true;
	return true;
}


bool VFS::Create(const wchar_t *pFilename, size_32 UserVersion, size_32 ClusterSize){
	wchar_t* pTemp;
	Close();
	CriticalSectionClass::LockClass lock(&m_Mount_Lock);

	m_hFT = fopen_W(pFilename, L"w+");
	if (!VALID_FILE(m_hFT)){
		fclose_(m_hMount);
		m_hMount = INVALID_FILE;
		m_Last_Error = VFS_FAILED_TO_CREATE_FILE;
		return false;
	}

	StrCpyW(m_pPath_Cache, pFilename);
	pTemp = (wchar_t*)ULongToPtr(max(PtrToUlong(StrrChrW(m_pPath_Cache, '/')), PtrToUlong(StrrChrW(m_pPath_Cache, '\\'))));
	swprintf(m_Header.m_Cluster_Name, L"%.8X%.8X.CLU", Get_Str_HashW(pTemp ? pTemp : L"VFS ", TRUE), Get_Str_HashW(pFilename, TRUE));
	if (pTemp) *(pTemp + 1) = 0;


	_snwprintf(m_pPath_Cache2, VFS_PATH_CACHE_SIZE, L"%s\\%s", pTemp ? m_pPath_Cache : L".\\", m_Header.m_Cluster_Name);
	
	m_hMount = fopen_W(m_pPath_Cache2, L"w+");
	if (!VALID_FILE(m_hMount)){
		m_Last_Error = VFS_FAILED_TO_CREATE_FILE;
		return false;
	}


	m_pCluster_Cache = (PCLUSTER)g_pAllocator(sizeof(struct CLUSTER_HEADER)+ClusterSize);
	if (!m_pCluster_Cache){
		m_Last_Error = VFS_CANNOT_CREATE_CACHE;
		Close();
		return false;
	}
	m_Read_Only = false;
	m_Names_Dirty = false;

	m_Header.m_Four_CC = VFS_FOUR_CC;
	m_Header.m_Version = VFS_VERSION;
	m_Header.m_User_Version = UserVersion;

	m_Header.m_Cluster_Size = ClusterSize;

	m_Header.m_First_Unused = NO_CLUSTERS;
	//fseek_64(m_hMount, , SEEK_SET);
	
	m_pUID_Entries = new HashList<CEntry*, size_64>(0x100);

#ifdef VFS_ENABLE_AES
	m_AESCacheSize = m_Header.m_Cluster_Size;
	if (m_AESCacheSize & (AES_BLOCK_SIZE-1))
		m_AESCacheSize = (m_AESCacheSize+AES_BLOCK_SIZE) & ~((AES_BLOCK_SIZE-1));
	m_pAESCache = (unsigned char*)g_pAllocator(m_AESCacheSize);
	m_pAESWriteCache = (unsigned char*)g_pAllocator(m_AESCacheSize);
	memset(m_pAESCache, 0, m_AESCacheSize);
	memset(m_pAESWriteCache, 0, m_AESCacheSize);
#endif //VFS_ENABLE_AES

	fseek_64(m_hMount, 0, SEEK_SET);
	fwrite_(&m_Header, sizeof(VFS_HEADER), m_hFT);
	m_UID = 0;
	Create_Element(L"$ROOT$", ROOT_UID, MFT_ELEMENT_FLAG_DIRECTORY, (m_Root_Dir.m_pThis_Element = &m_Root_Element));
	m_pUID_Entries->Insert(m_Root_Dir.m_pThis_Element->m_Element.m_UID, &m_Root_Dir.m_pThis_UID_Node, &m_Root_Dir);

	m_Last_Error = VFS_OK;
	m_Opening = false;
	m_pChild_Entries = new ExternNodeHashList<CEntry*, HASHKEY>(0xFFFF);
	m_FlatPriority = true;
	m_Is_Open = true;
	return true;
}

void VFS::Close(){

	m_MultiCache.FreeCaches();
	m_Is_Open = false;

#ifdef VFS_ENABLE_AES
	m_pfGetAESKey = NULL;
	if (m_pAESCache)
		g_pFreeer(m_pAESCache);
	if (m_pAESWriteCache)
		g_pFreeer(m_pAESWriteCache);
	m_pAESCache = NULL;
	m_pAESWriteCache = NULL;
#endif //VFS_ENABLE_AES
	if (m_hMount == INVALID_FILE) return;
	CriticalSectionClass::LockClass lock(&m_Mount_Lock);

	if (!m_Read_Only && !m_Opening){
		fseek_64(m_hFT, 0, SEEK_SET);
		fwrite_(&m_Header, sizeof(VFS_HEADER), m_hFT);
		FullFlush();
	}
	if (VALID_FILE(m_hMount))
		fclose_(m_hMount);
	if (VALID_FILE(m_hFT))
		fclose_(m_hFT);

	m_hMount = INVALID_FILE;
	m_hFT = INVALID_FILE;
	Free_Data();
}

void VFS::SetOption(int option, void *pData, int datasize){
#ifdef VFS_ENABLE_AES
	if (option == VFS_OPTION_AES_KEYCB && datasize == sizeof(pfGetAESKey)){
		m_pfGetAESKey = (pfGetAESKey)pData;
		return;
	}
#endif //VFS_ENABLE_AES
}

int VFS::GetLastError(){
	return m_Last_Error;
}

void VFS::PartialFlush(){
	PINTERNAL_MFT_ELEMENT pNode;
	if (m_Read_Only){
		m_Last_Error = VFS_READ_ONLY;
		return;
	}

	fseek_64(m_hFT, 0, SEEK_SET);
	fwrite_(&m_Header, sizeof(VFS_HEADER), m_hFT);

	pNode = m_Elements.Get_Head();
	while (pNode){
		if (pNode->m_Element.m_Dirty == MFT_ELEMENT_DIRTY){
			pNode->m_Element.m_Dirty = MFT_ELEMENT_NOT_DIRTY;
			fseek_64(m_hFT, pNode->m_Element.m_This_Element, SEEK_SET);
			fwrite_(&pNode->m_Element, sizeof(MFT_ELEMENT), m_hFT);
		}
		pNode = pNode->m_pNext;
	}

	/*pNode = m_Free_Elements.Get_Head();
	while (pNode){
		if (pNode->m_pData->m_Flags & MFT_ELEMENT_FLAG_DIRTY){
			fseek_64(m_hFT, pNode->m_pData->m_This_Element, SEEK_SET);
			fwrite_(pNode->m_pData, sizeof(MFT_ELEMENT), m_hFT);
		}
		pNode = pNode->m_pNext;
	}*/
}

void VFS::FullFlush(){
	PINTERNAL_MFT_ELEMENT pNode;
	if (m_Read_Only){
		m_Last_Error = VFS_READ_ONLY;
		return;
	}

	fseek_64(m_hFT, 0, SEEK_SET);
	fwrite_(&m_Header, sizeof(VFS_HEADER), m_hFT);

	pNode = m_Elements.Get_Head();
	while (pNode){
		pNode->m_Element.m_Dirty = MFT_ELEMENT_NOT_DIRTY;
		fseek_64(m_hFT, pNode->m_Element.m_This_Element, SEEK_SET);
		fwrite_(pNode, sizeof(MFT_ELEMENT), m_hFT);
		pNode = pNode->m_pNext;

	}

	/*pNode = m_Free_Elements.Get_Head();
	while (pNode){
		fwrite_(pNode->m_pData, sizeof(MFT_ELEMENT), m_hFT);
		pNode = pNode->m_pNext;
	}*/
}

PCLUSTER_HEADER VFS::Open_Cluster_Chain(size_64 base_cluster){
	struct CLUSTER_HEADER cluster_hdr;
	PCLUSTER_HEADER pClusters;
	UINT32 count, i;

	fseek_64(m_hMount, base_cluster, SEEK_SET);
	fread_(&cluster_hdr, sizeof(struct CLUSTER_HEADER), m_hMount);

	if (cluster_hdr.m_Previous_Cluster != HEAD_CLUSTER) return NULL;

	count = (UINT32)cluster_hdr.m_Cluster_Count;
	pClusters = (PCLUSTER_HEADER)g_pAllocator(sizeof(struct CLUSTER_HEADER)*count);

	if (!pClusters) return NULL;

	memcpy(&pClusters[0], &cluster_hdr, sizeof(struct CLUSTER_HEADER));

	for (i = 1; i < count; i++){
		fseek_64(m_hMount, pClusters[i-1].m_Next_Cluster, SEEK_SET);
		fread_(&pClusters[i], sizeof(struct CLUSTER_HEADER), m_hMount);
	}
	return pClusters;
}

void VFS::Free_Cluster_Chain(PCLUSTER_HEADER pChain, bool write){
	int i;
	i = 0;
	if (write && m_Read_Only){
		m_Last_Error = VFS_READ_ONLY;
		return;
	} else if (write){
		do {
			fseek_64(m_hMount, pChain[i].m_This_Cluster, SEEK_SET);
			printf("%.8X - %.8X - %.8X\n", (size_32)pChain[i].m_This_Cluster, (size_32)(pChain[i].m_This_Cluster),
				(size_32)(pChain[i].m_This_Data));
			fwrite_(&pChain[i], sizeof(struct CLUSTER_HEADER), m_hMount);
		} while (pChain[i++].m_Next_Cluster != TAIL_CLUSTER);
	}
	g_pFreeer(pChain);
}

void VFS::Write_Cluster_Chain(PCLUSTER_HEADER pChain){
	int i;
	i = 0;
	if (m_Read_Only){
		m_Last_Error = VFS_READ_ONLY;
		return;
	}
	do {
		fseek_64(m_hMount, pChain[i].m_This_Cluster, SEEK_SET);
		fwrite_(&pChain[i], sizeof(struct CLUSTER_HEADER), m_hMount);
	} while (pChain[i++].m_Next_Cluster != TAIL_CLUSTER);
}

PCLUSTER_HEADER VFS::Allocate_Clusters(size_64 size, bool flush, bool force_append, size_64 *pOffset, PCLUSTER_HEADER pClusters){
	size_64 realsize;
	size_64 cluster_offset;
	size_64 prev_cluster;
	size_64 current_cluster;
	size_64 cluster_count;
	size_64 i, j;
	PCLUSTER_HEADER pCluster;
	CLUSTER_HEADER cluster_hdr;
	//CLUSTER cluster;

	if (m_Read_Only){
		m_Last_Error = VFS_READ_ONLY;
		return 0;
	}

	realsize = (j = cluster_count = ((size / m_Header.m_Cluster_Size)+((size % m_Header.m_Cluster_Size) ? 1 : 0))) * m_Header.m_Cluster_Size;

	
	if (!realsize) {
		if (pOffset) *pOffset = NO_CLUSTERS;
		return NULL;
	}

	if (pClusters)
		pCluster = pClusters;
	else if (!(pCluster = (PCLUSTER_HEADER)g_pAllocator(sizeof(CLUSTER_HEADER)*(size_t)cluster_count))){
		if (pOffset) *pOffset = NO_CLUSTERS;
		return NULL;
	}
	for (i = 0; i < cluster_count; i++) memset(&pCluster[i], 0, sizeof(CLUSTER_HEADER));
	
	i = 0;
	prev_cluster = HEAD_CLUSTER;
	pCluster[0].m_Cluster_Count = (size_32)cluster_count;
	
	if (!force_append && m_Header.m_First_Unused != NO_CLUSTERS){
		fseek_64(m_hMount, (cluster_offset = m_Header.m_First_Unused), SEEK_SET);
		do {
			fread_(&pCluster[i], sizeof(struct CLUSTER_HEADER), m_hMount);
			fseek_64(m_hMount, pCluster[i].m_Next_Cluster, SEEK_SET);
			i++;
		} while (--j && pCluster[i-1].m_Next_Cluster != TAIL_CLUSTER);
		--i;
		pCluster[0].m_Previous_Cluster = HEAD_CLUSTER;

		if (pCluster[i].m_Next_Cluster != TAIL_CLUSTER){
			fseek_64(m_hMount, (m_Header.m_First_Unused = pCluster[i].m_Next_Cluster), SEEK_SET);
			fread_(&cluster_hdr, sizeof(CLUSTER_HEADER), m_hMount);
			fseek_64(m_hMount, pCluster[i].m_Next_Cluster, SEEK_SET);
			cluster_hdr.m_Previous_Cluster = HEAD_CLUSTER;
			fwrite_(&cluster_hdr, sizeof(CLUSTER_HEADER), m_hMount);
			m_Header.m_First_Unused = pCluster[i].m_Next_Cluster;

			pCluster[i].m_Next_Cluster = TAIL_CLUSTER;
			fseek_64(m_hMount, pCluster[i].m_This_Cluster, SEEK_SET);
			fwrite_(&pCluster[i], sizeof(struct CLUSTER_HEADER), m_hMount);
		} else {
			pCluster[i].m_Next_Cluster = j ? fsize_64(m_hMount) : TAIL_CLUSTER;
			prev_cluster = pCluster[i].m_This_Cluster;
			fseek_64(m_hMount, pCluster[i].m_This_Cluster, SEEK_SET);
			fwrite_(&pCluster[i], sizeof(CLUSTER_HEADER), m_hMount);
			m_Header.m_First_Unused = NO_CLUSTERS;
		}
		i++;
	}

	//if (m_pCluster_Cache) m_pCluster_Cache->m_Header.m_Dirty = 0;
	if (i < cluster_count){
		memset(m_pCluster_Cache, 0, (m_Header.m_Cluster_Size+sizeof(CLUSTER_HEADER)));
		current_cluster = fsize_64(m_hMount);
		if (i == 0)
			cluster_offset = current_cluster;

		fseek_64(m_hMount, current_cluster, SEEK_SET);
		for (; i < cluster_count; i++){
			m_pCluster_Cache->m_Header.m_Previous_Cluster = prev_cluster;
			m_pCluster_Cache->m_Header.m_Next_Cluster = (i == cluster_count-1) ? TAIL_CLUSTER : current_cluster + (m_Header.m_Cluster_Size+sizeof(CLUSTER_HEADER));
			m_pCluster_Cache->m_Header.m_This_Data = (m_pCluster_Cache->m_Header.m_This_Cluster = current_cluster)+sizeof(CLUSTER_HEADER);
			memcpy(&pCluster[i], &m_pCluster_Cache->m_Header, sizeof(struct CLUSTER_HEADER));
			fwrite_(m_pCluster_Cache, (m_Header.m_Cluster_Size+sizeof(struct CLUSTER_HEADER)), m_hMount);
			prev_cluster = current_cluster;
			current_cluster += m_Header.m_Cluster_Size+sizeof(struct CLUSTER_HEADER);
		}
	}

	pCluster[0].m_Cluster_Count = cluster_count;
	fseek_64(m_hMount, pCluster[0].m_This_Cluster, SEEK_SET);
	fwrite_(&pCluster[0], sizeof(CLUSTER_HEADER), m_hMount);

	if (pOffset) *pOffset = pCluster[0].m_This_Cluster;

	//if (flush) MFT_Flush_To_Disk(pMFTData, TRUE, FALSE);
	return pCluster;
}

PCLUSTER_HEADER VFS::Get_Last_Cluster(PCLUSTER_HEADER pClusters){
	int i;
	for (i = 0; pClusters[i].m_Next_Cluster != TAIL_CLUSTER; i++)
		;
	return &(pClusters[i]);
}

void VFS::Release_Clusters(PCLUSTER_HEADER pClusters){
	PCLUSTER_HEADER pLast_Cluster;
	struct CLUSTER_HEADER cluster_hdr;
	size_64 last;

	if (m_Read_Only){
		m_Last_Error = VFS_READ_ONLY;
		return;
	}

	pLast_Cluster = Get_Last_Cluster(pClusters);

	if (m_Header.m_First_Unused != NO_CLUSTERS){
		fseek_64(m_hMount, m_Header.m_First_Unused, SEEK_SET);
		fread_(&cluster_hdr, sizeof(struct CLUSTER_HEADER), m_hMount);


		pLast_Cluster->m_Next_Cluster = m_Header.m_First_Unused;
		cluster_hdr.m_Previous_Cluster = pLast_Cluster->m_This_Cluster;


		fseek_64(m_hMount, m_Header.m_First_Unused, SEEK_SET);
		fwrite_(&cluster_hdr, sizeof(struct CLUSTER_HEADER), m_hMount);

		last = cluster_hdr.m_Last_Cluster;
	} else {
		pLast_Cluster->m_Next_Cluster = TAIL_CLUSTER;
		last = pLast_Cluster->m_This_Cluster;
	}
	pClusters[0].m_Last_Cluster = last;
	pClusters[0].m_Previous_Cluster = HEAD_CLUSTER;

	fseek_64(m_hMount, pClusters[0].m_This_Cluster, SEEK_SET);
	fwrite_(&(pClusters[0]), sizeof(struct CLUSTER_HEADER), m_hMount);

	fseek_64(m_hMount, pLast_Cluster->m_This_Cluster, SEEK_SET);
	fwrite_(pLast_Cluster, sizeof(struct CLUSTER_HEADER), m_hMount);

	m_Header.m_First_Unused = pClusters[0].m_This_Cluster;
}


size_64 VFS::Get_Last_Cluster(size_64 Cluster){
	CLUSTER_HEADER cluster;
	fseek_64(m_hMount, Cluster, SEEK_SET);
	fread_(&cluster, sizeof(CLUSTER_HEADER), m_hMount);
	while (cluster.m_Next_Cluster != TAIL_CLUSTER){
		fseek_64(m_hMount, cluster.m_Next_Cluster, SEEK_SET);
		fread_(&cluster, sizeof(CLUSTER_HEADER), m_hMount);
	}
	return cluster.m_This_Cluster;
}

void VFS::Release_Clusters(size_64 Cluster){
	size_64 last;
	CLUSTER_HEADER cluster_hdr, temp;


	if (m_Read_Only){
		m_Last_Error = VFS_READ_ONLY;
		return;
	}

	last = Get_Last_Cluster(Cluster);

	if (m_Header.m_First_Unused != NO_CLUSTERS){
		fseek_64(m_hMount, m_Header.m_First_Unused, SEEK_SET);
		fread_(&cluster_hdr, sizeof(CLUSTER_HEADER), m_hMount);

		cluster_hdr.m_Previous_Cluster = last;

		fseek_64(m_hMount, m_Header.m_First_Unused, SEEK_SET);
		fwrite_(&cluster_hdr, sizeof(CLUSTER_HEADER), m_hMount);

		fseek_64(m_hMount, last, SEEK_SET);
		fread_(&temp, sizeof(CLUSTER_HEADER), m_hMount);

		temp.m_Next_Cluster = m_Header.m_First_Unused;

		fseek_64(m_hMount, last, SEEK_SET);
		fread_(&temp, sizeof(CLUSTER_HEADER), m_hMount);
	}
	m_Header.m_First_Unused = Cluster;
	fseek_64(m_hFT, 0, SEEK_SET);
	fwrite_(&m_Header, sizeof(VFS_HEADER), m_hFT);
}

int VFS::RecursiveEntryEnum(CEntry *pThis, pfEnumCallback pCB, PVFSFILEINFO pFileInfo, ptr wParam, ptr lParam, int what, int *pCount, wchar_t *pThisPath){
	int retval;
	int len;
	CEntry *pNode;

	if (what & VFS_ENUM_MY_DIRS){
		pNode = pThis->m_Directories.Get_Head();
		while (pNode){
#ifdef VFS_ENABLE_NO_ENUM
			if (!pNode->NoEnum()){
#endif //VFS_ENABLE_NO_ENUM
				retval = pCB(MFTElementToFileInfo(pFileInfo, pNode->m_pThis_Element, pThisPath, &len, m_pPath_Cache, pNode), lParam, wParam);
				if (*pCount) *pCount++;
				if (retval & VFS_ENUM_MY_AND_VALUE)
					retval = RecursiveEntryEnum(pNode, pCB, pFileInfo, lParam, wParam, retval & VFS_ENUM_MY_AND_VALUE, pCount, pThisPath+len);
				pThisPath[0] = 0;
				if ((retval & VFS_ENUM_AND_VALUE) == VFS_ENUM_STOP || (retval & VFS_ENUM_AND_VALUE) == VFS_ENUM_BREAK) return (retval == VFS_ENUM_STOP) ? VFS_ENUM_STOP : VFS_ENUM_CONTINUE | (what & VFS_ENUM_MY_AND_VALUE);
#ifdef VFS_ENABLE_NO_ENUM
			}
#endif //VFS_ENABLE_NO_ENUM
			pNode = pNode->m_pNext;
		}
	}

	if (what & VFS_ENUM_MY_FILES){
		pNode = pThis->m_Files.Get_Head();
		while (pNode){
#ifdef VFS_ENABLE_NO_ENUM
			if (!pNode->NoEnum()){
#endif //VFS_ENABLE_NO_ENUM
				retval = pCB(MFTElementToFileInfo(pFileInfo, pNode->m_pThis_Element, pThisPath, &len, m_pPath_Cache, pNode), lParam, wParam);
				pThisPath[0] = 0;
				if (*pCount) *pCount++;
				if ((retval & VFS_ENUM_AND_VALUE) == VFS_ENUM_STOP || (retval & VFS_ENUM_AND_VALUE) == VFS_ENUM_BREAK) return ((retval == VFS_ENUM_STOP) ? VFS_ENUM_STOP : VFS_ENUM_CONTINUE | (what & VFS_ENUM_MY_AND_VALUE));
#ifdef VFS_ENABLE_NO_ENUM
			}
#endif //VFS_ENABLE_NO_ENUM
			pNode = pNode->m_pNext;
		}
	}
	return VFS_ENUM_CONTINUE | (what & VFS_ENUM_MY_AND_VALUE);
}

int VFS::EntryEnum(const wchar_t* pDirectory, pfEnumCallback pCB, ptr wParam, ptr lParam, int what){
	CEntry* pDir;
	int level, num, len, newlen;
	int ret;
	CEntry* pNode;
	VFSFILEINFO finfo;
	m_pPath_Cache[0] = 0;
	level = 0;
	num = 0;

	StrnCpyW(m_pPath_Cache, pDirectory ? pDirectory : L"", VFS_PATH_CACHE_SIZE);
	len = StrLenW(m_pPath_Cache);

	pDir = (!pDirectory) ? &m_Root_Dir : Locate_Directory(NULL, pDirectory, NULL, 0, false, NULL);
	if (!pDir) return -1;

	if (what & VFS_ENUM_DIRECTORIES){
		pNode = pDir->m_Directories.Get_Head();
		while (pNode){
#ifdef VFS_ENABLE_NO_ENUM
			if (!pNode->NoEnum()){
#endif //VFS_ENABLE_NO_ENUM
				ret = pCB(MFTElementToFileInfo(&finfo, pNode->m_pThis_Element, m_pPath_Cache+len, &newlen, m_pPath_Cache, pNode), lParam, wParam);
				num++;
				if (ret & VFS_ENUM_MY_AND_VALUE)
					ret = RecursiveEntryEnum(pNode, pCB, &finfo, lParam, wParam, ret & VFS_ENUM_MY_AND_VALUE, &num, m_pPath_Cache+len+newlen);
				(m_pPath_Cache+len)[0] = 0;
				if ((ret & VFS_ENUM_AND_VALUE) == VFS_ENUM_STOP || (ret & VFS_ENUM_AND_VALUE) == VFS_ENUM_BREAK) return num;
#ifdef VFS_ENABLE_NO_ENUM
			}
#endif //VFS_ENABLE_NO_ENUM
			pNode = pNode->m_pNext;			
		}
	}

	if (what & VFS_ENUM_FILES){
		pNode = pDir->m_Files.Get_Head();
		while (pNode){
#ifdef VFS_ENABLE_NO_ENUM
			if (!pNode->NoEnum()){
#endif //VFS_ENABLE_NO_ENUM
				num++;
				ret = pCB(MFTElementToFileInfo(&finfo, pNode->m_pThis_Element, m_pPath_Cache+len, NULL, m_pPath_Cache, pNode), lParam, wParam);
				(m_pPath_Cache+len)[0] = 0;
				if ((ret & VFS_ENUM_AND_VALUE) == VFS_ENUM_STOP || (ret & VFS_ENUM_AND_VALUE) == VFS_ENUM_BREAK) return num;
#ifdef VFS_ENABLE_NO_ENUM
			}
#endif //VFS_ENABLE_NO_ENUM
			pNode = pNode->m_pNext;
		}
	}	

	return num;
}

void VFS::CreateDir(const wchar_t *pPath){
}

VFSFileHandle VFS::InternalCreateFile(CEntry *pBaseDir, const wchar_t* pName, UINT32 Mode){
	CEntry *pEntry;
	PINTERNAL_MFT_ELEMENT pElem;
	POPEN_FILE pFile;
	bool created;
	m_Last_Error = VFS_OK;
	if ((Mode & FILE_MODE_ALL) == FILE_CREATE){
		pEntry = Locate_File(pBaseDir, pName, true, &created);
		if (!pEntry){
			m_Last_Error = VFS_CANNOT_CREATE_FILE;
			return NULL;
		}
		if (!created){
			m_Last_Error = VFS_FILE_EXISTS;
			return NULL;
		}
		pElem = pEntry->m_pThis_Element;
		if (!(Mode & FILE_OPEN_WRITE)){
			m_Last_Error = VFS_INVALID_OPEN_MODE;
			return NULL;
		}
		if ((Mode & FILE_OPEN_READ_WRITE) && pElem->m_Element.m_Write_Locked){
			m_Last_Error = VFS_WRITE_LOCKED_FILE;
			return NULL;
		}
		if ((Mode & FILE_OPEN_READ_WRITE) && pElem->m_Element.m_Read_Locked){
			m_Last_Error = VFS_READ_LOCKED_FILE;
			return NULL;
		}
		pElem->m_Element.m_Dirty = MFT_ELEMENT_DIRTY;
		if (!(pFile = new OPEN_FILE)){
			pEntry->Delete(true, this);
			delete pEntry;
			return NULL;
		}
		pFile->m_Offset = 0;
		pFile->m_pMFT_Element = pElem;
		pFile->m_pCache = NULL;
		pFile->m_pFileData = pEntry;

		pFile->m_Write_Access = (Mode & FILE_OPEN_WRITE) ? 1 : 0;
		pFile->m_Read_Access = (Mode & FILE_OPEN_READ) ? 1 : 0;

		if (!pElem->m_Element.m_Ref_Count){
			pFile->m_pMFT_Element->m_Element.m_Write_Locked = (Mode & FILE_OPEN_WRITE) ? MFT_ELEMENT_LOCKED : MFT_ELEMENT_NOT_LOCKED;
			pFile->m_pMFT_Element->m_Element.m_Read_Locked = (Mode & FILE_OPEN_READ) ? MFT_ELEMENT_LOCKED : MFT_ELEMENT_NOT_LOCKED;
		}
		pElem->m_Element.m_Ref_Count++;
		pFile->m_Mode = VFS_FILE_MODE_ARCHIVE;
		m_Open_Files.Add_Tail(pFile);
		return OpenFileToFileHandle(pFile);
	}
	if ((Mode & FILE_MODE_ALL) == FILE_CREATE_ALWAYS){
		pEntry = Locate_File(pBaseDir, pName, true, NULL);
		if (!pEntry){
			m_Last_Error = VFS_CANNOT_CREATE_FILE;
			return NULL;
		}
		pElem = pEntry->m_pThis_Element;
		if (!(Mode & FILE_OPEN_WRITE)){
			m_Last_Error = VFS_INVALID_OPEN_MODE;
			return NULL;
		}
		if ((Mode & FILE_OPEN_READ_WRITE) && pElem->m_Element.m_Write_Locked){
			m_Last_Error = VFS_WRITE_LOCKED_FILE;
			return NULL;
		}
		if ((Mode & FILE_OPEN_READ_WRITE) && pElem->m_Element.m_Read_Locked){
			m_Last_Error = VFS_READ_LOCKED_FILE;
			return NULL;
		}
		if (pElem->m_Element.m_First_Cluster != NO_CLUSTERS){
			Release_Clusters(pElem->m_Element.m_First_Cluster);
			pElem->m_Element.m_First_Cluster = NO_CLUSTERS;
		}
		pElem->m_Element.m_Dirty = MFT_ELEMENT_DIRTY;
		if (!(pFile = new OPEN_FILE)){
			pEntry->Delete(true, this);
			delete pEntry;
			return NULL;
		}
		pElem->m_Element.m_File_Size = 0;
		pFile->m_Offset = 0;
		pFile->m_pMFT_Element = pElem;
		pFile->m_Write_Access = 1;
		pFile->m_pCache = NULL;
		pFile->m_Read_Access = (Mode & FILE_OPEN_READ) ? 1 : 0;
		if (!pElem->m_Element.m_Ref_Count){
			pFile->m_pMFT_Element->m_Element.m_Write_Locked = MFT_ELEMENT_LOCKED;
			pFile->m_pMFT_Element->m_Element.m_Read_Locked = (Mode & FILE_OPEN_READ) ? MFT_ELEMENT_LOCKED : MFT_ELEMENT_NOT_LOCKED;
		}
		pFile->m_Mode = VFS_FILE_MODE_ARCHIVE;
		pElem->m_Element.m_Ref_Count++;
		m_Open_Files.Add_Tail(pFile);
		pFile->m_pFileData = pEntry;
		return OpenFileToFileHandle(pFile);
	}
	if ((Mode & FILE_MODE_ALL) == FILE_OPEN_ALWAYS){
		pEntry = Locate_File(pBaseDir, pName, true, &created);
		if (!pEntry){
			m_Last_Error = VFS_CANNOT_CREATE_FILE;
			return NULL;
		}
		pElem = pEntry->m_pThis_Element;
		if (created && !(Mode & FILE_OPEN_WRITE)){
			m_Last_Error = VFS_INVALID_OPEN_MODE;
			return NULL;
		}
		if ((Mode & FILE_OPEN_READ_WRITE) && pElem->m_Element.m_Write_Locked){
			m_Last_Error = VFS_WRITE_LOCKED_FILE;
			return NULL;
		}
		if ((Mode & FILE_OPEN_READ_WRITE) && pElem->m_Element.m_Read_Locked){
			m_Last_Error = VFS_READ_LOCKED_FILE;
			return NULL;
		}
		pElem->m_Element.m_Dirty = MFT_ELEMENT_DIRTY;
		if (!(pFile = new OPEN_FILE)){
			pEntry->Delete(true, this);
			delete pEntry;
			return NULL;
		}
		pFile->m_Offset = 0;
		pFile->m_pMFT_Element = pElem;
		pFile->m_pCache = NULL;

		pFile->m_Write_Access = (Mode & FILE_OPEN_WRITE) ? 1 : 0;
		pFile->m_Read_Access = (Mode & FILE_OPEN_READ) ? 1 : 0;

		if (!pElem->m_Element.m_Ref_Count){
			pFile->m_pMFT_Element->m_Element.m_Write_Locked = (Mode & FILE_OPEN_WRITE) ? MFT_ELEMENT_LOCKED : MFT_ELEMENT_NOT_LOCKED;
			pFile->m_pMFT_Element->m_Element.m_Read_Locked = (Mode & FILE_OPEN_READ) ? MFT_ELEMENT_LOCKED : MFT_ELEMENT_NOT_LOCKED;
		}
		pElem->m_Element.m_Ref_Count++;
		pFile->m_Mode = VFS_FILE_MODE_ARCHIVE;
		m_Open_Files.Add_Tail(pFile);
		pFile->m_pFileData = pEntry;
		return OpenFileToFileHandle(pFile);
	}
	if ((Mode & FILE_MODE_ALL) == FILE_OPEN_EXISTING){
		pEntry = Locate_File(pBaseDir, pName, false, &created);
		if (!pEntry){
			m_Last_Error = VFS_CANNOT_CREATE_FILE;
			return NULL;
		}
		pElem = pEntry->m_pThis_Element;
		if (!(Mode & FILE_OPEN_READ_WRITE)){
			m_Last_Error = VFS_INVALID_OPEN_MODE;
			return NULL;
		}
		if ((Mode & FILE_OPEN_READ_WRITE) && pElem->m_Element.m_Write_Locked){
			m_Last_Error = VFS_WRITE_LOCKED_FILE;
			return NULL;
		}
		if ((Mode & FILE_OPEN_READ_WRITE) && pElem->m_Element.m_Read_Locked){
			m_Last_Error = VFS_READ_LOCKED_FILE;
			return NULL;
		}
		pElem->m_Element.m_Dirty = MFT_ELEMENT_DIRTY;
		if (!(pFile = new OPEN_FILE)){
			pEntry->Delete(true, this);
			delete pEntry;
			return NULL;
		}
		pFile->m_Offset = 0;
		pFile->m_pMFT_Element = pElem;
		pFile->m_pCache = NULL;

		pFile->m_Write_Access = (Mode & FILE_OPEN_WRITE) ? 1 : 0;
		pFile->m_Read_Access = (Mode & FILE_OPEN_READ) ? 1 : 0;

		if (!pElem->m_Element.m_Ref_Count){
			pFile->m_pMFT_Element->m_Element.m_Write_Locked = (Mode & FILE_OPEN_WRITE) ? MFT_ELEMENT_LOCKED : MFT_ELEMENT_NOT_LOCKED;
			pFile->m_pMFT_Element->m_Element.m_Read_Locked = (Mode & FILE_OPEN_READ) ? MFT_ELEMENT_LOCKED : MFT_ELEMENT_NOT_LOCKED;
		}
		pElem->m_Element.m_Ref_Count++;
		pFile->m_Mode = VFS_FILE_MODE_ARCHIVE;
		m_Open_Files.Add_Tail(pFile);
		pFile->m_pFileData = pEntry;
		return OpenFileToFileHandle(pFile);
	}
	m_Last_Error = VFS_INVALID_OPEN_MODE;
	return NULL;
}

bool VFS::InternalEmbedFile(CEntry* pDir, const wchar_t *pSrcFile, const wchar_t* pDestFile, bool OverWrite, PPROGESSCB pCB){
	VFSFileHandle hFile;
	file f;
	size_32 r;
	size_64 size, fsize, offset;
	if (!VALID_FILE(f = fopen_W(pSrcFile, L"r"))){
		m_Last_Error = VFS_FAILED_TO_OPEN_FILE;
		return false;
	}
	if (!VFS_VALID_FILE(hFile = InternalCreateFile(pDir, pDestFile, (OverWrite ? FILE_CREATE_ALWAYS : FILE_CREATE) | FILE_OPEN_WRITE))){
		m_Last_Error = VFS_FAILED_TO_CREATE_FILE;
		fclose_(f);
		return false;
	}
	
	fsize = size = fsize_64(f);
	
	offset = 0;
	VFSSeek(hFile, (ssize_64)size, SEEK_SET);
	VFSSeek(hFile, 0, SEEK_SET);
	if (pCB) pCB->m_pCB(NULL, PROGRESS_TYPE_START, 0, fsize, pCB->m_pCookie);
	while (size && (r = fread_(m_pCache, (size_32)min(size, VFS_CACHE_SIZE), f))){
		size -= r;

		offset += r;
		if (pCB) pCB->m_pCB(m_pCache, PROGRESS_TYPE_NEXT, offset, fsize, pCB->m_pCookie);

		if (VFSWrite(hFile, m_pCache, r, NULL) != r) goto __failed;
	}
	if (pCB) pCB->m_pCB(NULL, PROGRESS_TYPE_END, offset, fsize, pCB->m_pCookie);
	if (size != 0) goto __failed;
	VFSCloseFile(hFile);
	fclose_(f);
	m_Last_Error = VFS_OK;
	return true;
__failed:
	VFSCloseFile(hFile);
	fclose_(f);
	VFSDeleteFile(pDestFile);
	return false;
}

void VFS::RecursiveCountProc(wchar_t *pThisName, const wchar_t *pFullPath, int *pCount){
	int len;
	int count;
	HANDLE hFind;
	WIN32_FIND_DATAW fd;
	pThisName[0] = '\\';
	pThisName[1] = '*';
	pThisName[2] = 0;
	hFind = FindFirstFileW(pFullPath, &fd);;
	pThisName[0] = 0;
	count = 0;
	if (hFind != INVALID_HANDLE_VALUE){
		do {
			if (memcmp(fd.cFileName, L".", 2*sizeof(wchar_t)) == 0 || memcmp(fd.cFileName, L"..", 3*sizeof(wchar_t)) == 0) continue;
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
				len = StrLenW(fd.cFileName);
				pThisName[0] = '\\';
				memcpy(pThisName+1, fd.cFileName, sizeof(wchar_t)*(len+1));
				RecursiveCountProc(pThisName+1+len, pFullPath, pCount);
				pThisName[0] = 0;
			} else
				if (pCount) (*pCount)++;

		} while (FindNextFileW(hFind, &fd));
		FindClose(hFind);
	}
}

int VFS::RecursiveEmbedProc(CEntry* pDir, wchar_t *pThisName, const wchar_t *pFullPath, int total, int *pCount, PPROGESSCB pCB){
	CEntry* pThisDir;
	int len;
	HANDLE hFind;
	WIN32_FIND_DATAW fd;
	pThisName[0] = '\\';
	pThisName[1] = '*';
	pThisName[2] = 0;
	hFind = FindFirstFileW(pFullPath, &fd);;
	pThisName[0] = 0;
	if (hFind != INVALID_HANDLE_VALUE){
		do {
			if (memcmp(fd.cFileName, L".", 2*sizeof(wchar_t)) == 0 || memcmp(fd.cFileName, L"..", 3*sizeof(wchar_t)) == 0) continue;
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
				pThisDir = Locate_Directory(pDir, fd.cFileName, NULL, 0, true, NULL);
				if (!pThisDir) continue;
				len = StrLenW(fd.cFileName);
				pThisName[0] = '\\';
				memcpy(pThisName+1, fd.cFileName, sizeof(wchar_t)*(len+1));
				RecursiveEmbedProc(pThisDir, pThisName+len+1, pFullPath, total, pCount, pCB);
				//count += RecursiveCountProc(pThisName+1+len, pFullPath);
				pThisName[0] = 0;
			} else{
				len = StrLenW(fd.cFileName);
				pThisName[0] = '\\';
				memcpy(pThisName+1, fd.cFileName, sizeof(wchar_t)*(len+1));
				if (pCount) ++(*pCount);
				if (pCB) pCB->m_pCB(fd.cFileName, PROGRESS_TYPE_NEXTFILE, pCount ? *pCount : 0, total, pCB->m_pCookie);
				InternalEmbedFile(pDir, pFullPath, fd.cFileName, true, pCB);
				//if (pCB) pCB->m_pCB((ptr)pFullPath, PROGRESS_TYPE_NEXT, pCount ? ++(*pCount) : 0, total, pCB->m_pCookie);
				pThisName[0] = 0;
			}
		} while (FindNextFileW(hFind, &fd));
		FindClose(hFind);
	}
	return pCount ? *pCount : 0;
}

bool VFS::InternalExtractFile(CEntry* pDir, const wchar_t *pSrcFile, const wchar_t* pDestFile, PPROGESSCB pCB){
	VFSFileHandle hFile;
	file f;
	size_32 r;
	size_64 size, fsize, offset;
	if (!VALID_FILE(f = fopen_W(pDestFile, L"w"))){
		m_Last_Error = VFS_FAILED_TO_CREATE_FILE;
		return false;
	}
	if (!VFS_VALID_FILE(hFile = InternalCreateFile(pDir, pSrcFile, FILE_OPEN_EXISTING|FILE_OPEN_READ))){
		m_Last_Error = VFS_FAILED_TO_OPEN_FILE;
		fclose_(f);
		return false;
	}
	fsize = size = VFSSize(hFile);
	offset = 0;
	if (pCB) pCB->m_pCB(NULL, PROGRESS_TYPE_START, 0, fsize, pCB->m_pCookie);
	while (size && (r = VFSRead(hFile, m_pCache, (size_32)min(size, VFS_CACHE_SIZE), NULL))){
		size -= r;

		offset += r;
		if (pCB) pCB->m_pCB(m_pCache, PROGRESS_TYPE_NEXT, offset, fsize, pCB->m_pCookie);

		if (fwrite_(m_pCache, r, f) != r) goto __failed;
	}//*/
	if (pCB) pCB->m_pCB(NULL, PROGRESS_TYPE_END, offset, fsize, pCB->m_pCookie);
	if (size != 0) goto __failed;
	VFSCloseFile(hFile);
	fclose_(f);
	m_Last_Error = VFS_OK;
	return true;
__failed:
	VFSCloseFile(hFile);
	fclose_(f);
	VFSDeleteFile(pDestFile);
	return false;
}

void VFS::RecursiveCountProc2(CEntry* pEntry, int *pCount){
	CEntry* pNode;
	pNode = pEntry->m_Directories.Get_Head();
	while (pNode){
		RecursiveCountProc2(pNode, pCount);
		pNode = pNode->m_pNext;
	}
	(*pCount) += pEntry->m_Files.Get_Count();
}

int VFS::RecursiveExtractProc(CEntry* pDir, wchar_t *pThisName, const wchar_t *pFullPath, int total, int *pCount, PPROGESSCB pCB){
	CEntry* pNode;
	int len;
	wchar_t *pTemp;
	pNode = pDir->m_Directories.Get_Head();
	while (pNode){
		pThisName[0] = '\\';
		len = StrLenW(pTemp = pNode->m_pThis_Element->m_Element.m_Name);
		memcpy(pThisName+1, pTemp, sizeof(wchar_t)*(len+1));
		if (CreateDirectoryW(pFullPath, NULL) || ::GetLastError() == ERROR_ALREADY_EXISTS)
			RecursiveExtractProc(pNode, pThisName+len+1, pFullPath, total, pCount, pCB);
		pThisName[0] = 0;
		pNode = pNode->m_pNext;
	}

	pNode = pDir->m_Files.Get_Head();
	while (pNode){
		pThisName[0] = '\\';
		len = StrLenW(pTemp = pNode->m_pThis_Element->m_Element.m_Name);
		memcpy(pThisName+1, pTemp, sizeof(wchar_t)*(len+1));

		if (pCount) ++(*pCount);
		if (pCB) pCB->m_pCB(pTemp, PROGRESS_TYPE_NEXTFILE, pCount ? *pCount : 0, total, pCB->m_pCookie);

		InternalExtractFile(pDir, pTemp, pFullPath, pCB);

		
		//if (pCB) pCB->m_pCB((ptr)pFullPath, PROGRESS_TYPE_NEXT, pCount ? ++(*pCount) : 0, total, pCB->m_pCookie);
		pThisName[0] = 0;
		pNode = pNode->m_pNext;
	}


	return 0;
}

//Memory file functions

POPEN_FILE VFS::MemoryCreateFile(const wchar_t* pName, UINT32 Mode){
	VFSFileHandle hFile;
	POPEN_FILE pFile;

	if ((Mode & FILE_MODE_ALL) != FILE_OPEN_EXISTING || (Mode & FILE_OPEN_READ_WRITE) != FILE_OPEN_READ){
		m_Last_Error = VFS_INVALID_OPEN_MODE;
		return NULL;
	}
	hFile = VFSCreateFile(pName, Mode & (~(UINT32)FILE_OPEN_MEMORY));
	if (!VFS_VALID_FILE(hFile)){
		m_Last_Error = VFS_CANNOT_OPEN_FILE;
		return NULL;
	}

	pFile = new OPEN_FILE;

	pFile->m_Mode = VFS_FILE_MODE_MEMORY;
	pFile->m_hFlat = INVALID_FILE;
	pFile->m_pCache = m_MultiCache.Alloc(VFSSize(hFile));
	if (!pFile->m_pCache){
		delete pFile;
		m_Last_Error = VFS_OUT_OF_BOUNDS;
		//VFSCloseFile(hFile);
		return (POPEN_FILE)hFile;
		return NULL;
	}
	pFile->m_UID = ((POPEN_FILE)hFile)->m_UID;
	pFile->m_Offset = 0;
	pFile->m_pMFT_Element = NULL;
	pFile->m_Read_Access = 1;
	pFile->m_Size = VFSSize(hFile);
	pFile->m_Write_Access = 0;
	pFile->m_pFileData = ((POPEN_FILE)hFile)->m_pFileData;
	m_Open_Files.Add_Head(pFile);

	VFSRead(hFile, pFile->m_pCache, (size_32)pFile->m_Size, NULL);
	VFSCloseFile(hFile);

	return pFile;
}


POPEN_FILE VFS::MemoryCreateFiledataFile(ptr pFiledata, UINT32 Mode){
	VFSFileHandle hFile;
	POPEN_FILE pFile;

	if ((Mode & FILE_MODE_ALL) != FILE_OPEN_EXISTING || (Mode & FILE_OPEN_READ_WRITE) != FILE_OPEN_READ){
		m_Last_Error = VFS_INVALID_OPEN_MODE;
		return NULL;
	}
	hFile = VFSCreateFiledataFile(pFiledata, Mode & (~(UINT32)FILE_OPEN_MEMORY));
	if (!VFS_VALID_FILE(hFile)){
		m_Last_Error = VFS_CANNOT_OPEN_FILE;
		return NULL;
	}

	pFile = new OPEN_FILE;

	pFile->m_Mode = VFS_FILE_MODE_MEMORY;
	pFile->m_hFlat = INVALID_FILE;
	pFile->m_pCache = m_MultiCache.Alloc(VFSSize(hFile));
	if (!pFile->m_pCache){
		delete pFile;
		m_Last_Error = VFS_OUT_OF_BOUNDS;
		//VFSCloseFile(hFile);
		return (POPEN_FILE)hFile;
		return NULL;
	}
	pFile->m_UID = ((POPEN_FILE)hFile)->m_UID;
	pFile->m_Offset = 0;
	pFile->m_pMFT_Element = NULL;
	pFile->m_Read_Access = 1;
	pFile->m_Size = VFSSize(hFile);
	pFile->m_Write_Access = 0;
	pFile->m_pFileData = ((POPEN_FILE)hFile)->m_pFileData;
	m_Open_Files.Add_Head(pFile);

	VFSRead(hFile, pFile->m_pCache, (size_32)pFile->m_Size, NULL);
	VFSCloseFile(hFile);

	return pFile;
}

void VFS::MemoryCloseFile(POPEN_FILE pFile){
	m_MultiCache.Free(pFile->m_pCache);
	m_Open_Files.Remove(pFile);
	delete pFile;
}

size_32 VFS::MemoryRead(POPEN_FILE pFile, void* pBuffer, size_32 Count, PPROGESSCB pCB){
	size_32 res;
	memcpy(pBuffer, ((char*)pFile->m_pCache)+pFile->m_Offset, res = (size_32)(min((size_64)Count, pFile->m_Size-pFile->m_Offset)));
	pFile->m_Offset += res;
	return res;
}

size_32 VFS::MemoryWrite(POPEN_FILE pFile, const void *pBuffer, size_32 Count, PPROGESSCB pCB){
	return 0;
	size_32 res;
	memcpy(((char*)pFile->m_pCache)+pFile->m_Offset, pBuffer, res = (size_32)(min((size_64)Count, pFile->m_Size-pFile->m_Offset)));
	pFile->m_Offset += res;
	return res;
}

size_64 VFS::MemorySeek(POPEN_FILE pFile, ssize_64 Offset, int Origin){
	switch (Origin){
		case SEEK_SET:
			return (size_64)((Offset < 0 || Offset > (ssize_64)pFile->m_Size) ? -1 : pFile->m_Offset = Offset);
		case SEEK_CUR:
			return (size_64)((pFile->m_Offset+Offset < 0 || pFile->m_Offset+Offset > (ssize_64)pFile->m_Size) ? -1 : pFile->m_Offset += Offset);
		case SEEK_END:
			return (size_64)((Offset > 0 || (-Offset) > (ssize_64)pFile->m_Size) ? -1 : pFile->m_Offset = pFile->m_Size + Offset);
		default: return (size_64)-1;
	}
}

size_64 VFS::MemoryTell(POPEN_FILE pFile){
	return (size_64)pFile->m_Offset;
}

size_64 VFS::MemorySize(POPEN_FILE pFile){
	return pFile->m_Size;
}

bool VFS::MemoryEndOfFile(POPEN_FILE pFile){
	return (pFile->m_Offset >= (ssize_64)pFile->m_Size);
}

bool VFS::MemorySetEndOfFile(POPEN_FILE pFile){
	return false;
}

const void* VFS::MemoryMemCache(POPEN_FILE pFile){
	return pFile->m_pCache;
}

//Flat file functions

POPEN_FILE VFS::FlatCreateFile(const wchar_t* pName, UINT32 Mode){
	POPEN_FILE pFile;
	wchar_t *pOpenMode;
	pOpenMode = NULL;
	file f;
	if ((Mode & FILE_MODE_ALL) == FILE_OPEN_EXISTING && (Mode & FILE_OPEN_READ_WRITE) == FILE_OPEN_READ)
		pOpenMode = L"r";

	if (pOpenMode && VALID_FILE((f = fopen_W(pName, pOpenMode)))){
		pFile = new OPEN_FILE;
		pFile->m_hFlat = f;
		pFile->m_UID = VFS_FLAT_UID;
		pFile->m_Mode = VFS_FILE_MODE_FLAT;
		pFile->m_Offset = 0;
		pFile->m_pCache = NULL;
		pFile->m_pMFT_Element = NULL;
		pFile->m_Read_Access = (Mode & FILE_OPEN_READ) ? 1 : 0;
		pFile->m_Write_Access = (Mode & FILE_OPEN_WRITE) ? 1 : 0;
		pFile->m_pFileData = NULL;
		m_Last_Error = VFS_OK;
		m_Open_Files.Add_Head(pFile);
		return pFile;
	}
	m_Last_Error = VFS_INVALID_OPEN_MODE;

	return NULL;
}

void VFS::FlatCloseFile(POPEN_FILE pFile){
	fclose_(pFile->m_hFlat);
	if (pFile->m_pCache)
		m_MultiCache.Free(pFile->m_pCache);
	m_Open_Files.Remove(pFile);
	delete pFile;
}

size_32 VFS::FlatRead(POPEN_FILE pFile, void* pBuffer, size_32 Count, PPROGESSCB pCB){
	return fread_(pBuffer, Count, pFile->m_hFlat);
}

size_32 VFS::FlatWrite(POPEN_FILE pFile, const void *pBuffer, size_32 Count, PPROGESSCB pCB){
	return fwrite_((void*)pBuffer, Count, pFile->m_hFlat);
}

size_64 VFS::FlatSeek(POPEN_FILE pFile, ssize_64 Offset, int Origin){
	return fseek_64(pFile->m_hFlat, Offset, Origin);
}

size_64 VFS::FlatTell(POPEN_FILE pFile){
	return ftell_64(pFile->m_hFlat);
}

size_64 VFS::FlatSize(POPEN_FILE pFile){
	return fsize_64(pFile->m_hFlat);
}

bool VFS::FlatEndOfFile(POPEN_FILE pFile){
	return (ftell_64(pFile->m_hFlat) >= fsize_64(pFile->m_hFlat));
}

bool VFS::FlatSetEndOfFile(POPEN_FILE pFile){
	fseteof_(pFile->m_hFlat);
	return true;
}

const void* VFS::FlatMemCache(POPEN_FILE pFile){
	return NULL;
}

//VFS file functions

POPEN_FILE VFS::ArchiveCreateFile(const wchar_t* pName, UINT32 Mode){
	CEntry *pEntry;
	PINTERNAL_MFT_ELEMENT pElem;
	POPEN_FILE pFile;
	bool created;
	m_Last_Error = VFS_OK;
	if ((Mode & FILE_MODE_ALL) == FILE_CREATE){
		pEntry = Locate_File(NULL, pName, true, &created);
		if (!pEntry){
			m_Last_Error = VFS_CANNOT_CREATE_FILE;
			return NULL;
		}
		if (!created){
			m_Last_Error = VFS_FILE_EXISTS;
			return NULL;
		}
		pElem = pEntry->m_pThis_Element;
		if (!(Mode & FILE_OPEN_WRITE)){
			m_Last_Error = VFS_INVALID_OPEN_MODE;
			return NULL;
		}
		if ((Mode & FILE_OPEN_READ_WRITE) && pElem->m_Element.m_Write_Locked){
			m_Last_Error = VFS_WRITE_LOCKED_FILE;
			return NULL;
		}
		if ((Mode & FILE_OPEN_READ_WRITE) && pElem->m_Element.m_Read_Locked){
			m_Last_Error = VFS_READ_LOCKED_FILE;
			return NULL;
		}
		pElem->m_Element.m_Dirty = MFT_ELEMENT_DIRTY;
		if (!(pFile = new OPEN_FILE)){
			goto __cleanup_free;
		}
		pFile->m_Offset = 0;
		pFile->m_pMFT_Element = pElem;

		pFile->m_pCache = NULL;

#ifdef VFS_ENABLE_AES
		if ((Mode & VFS_OPEN_AES_FLAG) == VFS_OPEN_AES_FLAG)
			pFile->m_pMFT_Element->m_Element.m_Encrypted = VFS_FLAG_ENCRYPTED;
#endif //VFS_ENABLE_AES

		pFile->m_Write_Access = (Mode & FILE_OPEN_WRITE) ? 1 : 0;
		pFile->m_Read_Access = (Mode & FILE_OPEN_READ) ? 1 : 0;

		if (!pElem->m_Element.m_Ref_Count){
			pFile->m_pMFT_Element->m_Element.m_Write_Locked = (Mode & FILE_OPEN_WRITE) ? MFT_ELEMENT_LOCKED : MFT_ELEMENT_NOT_LOCKED;
			pFile->m_pMFT_Element->m_Element.m_Read_Locked = (Mode & FILE_OPEN_READ) ? MFT_ELEMENT_LOCKED : MFT_ELEMENT_NOT_LOCKED;
		}
		pElem->m_Element.m_Ref_Count++;
		pFile->m_Mode = VFS_FILE_MODE_ARCHIVE;
		pFile->m_UID = pElem->m_Element.m_UID;
		pFile->m_pFileData = pEntry;
		m_Open_Files.Add_Tail(pFile);
		return pFile;
	}
	if ((Mode & FILE_MODE_ALL) == FILE_CREATE_ALWAYS){
		pEntry = Locate_File(NULL, pName, true, NULL);
		if (!pEntry){
			m_Last_Error = VFS_CANNOT_CREATE_FILE;
			return NULL;
		}
		pElem = pEntry->m_pThis_Element;
		if (!(Mode & FILE_OPEN_WRITE)){
			m_Last_Error = VFS_INVALID_OPEN_MODE;
			return NULL;
		}
		if ((Mode & FILE_OPEN_READ_WRITE) && pElem->m_Element.m_Write_Locked){
			m_Last_Error = VFS_WRITE_LOCKED_FILE;
			return NULL;
		}
		if ((Mode & FILE_OPEN_READ_WRITE) && pElem->m_Element.m_Read_Locked){
			m_Last_Error = VFS_READ_LOCKED_FILE;
			return NULL;
		}
		if (pElem->m_Element.m_First_Cluster != NO_CLUSTERS){
			Release_Clusters(pElem->m_Element.m_First_Cluster);
			pElem->m_Element.m_First_Cluster = NO_CLUSTERS;
		}
		pElem->m_Element.m_File_Size = 0;
		pElem->m_Element.m_Dirty = MFT_ELEMENT_DIRTY;
		if (!(pFile = new OPEN_FILE)){
			goto __cleanup_free;
		}
		pFile->m_Offset = 0;
		pFile->m_pMFT_Element = pElem;
#ifdef VFS_ENABLE_AES
		if ((Mode & FILE_OPEN_AES))
			pFile->m_pMFT_Element->m_Element.m_Encrypted = VFS_FLAG_ENCRYPTED;
#endif //VFS_ENABLE_AES

		
		pFile->m_pCache = NULL;

		pFile->m_Write_Access = 1;
		pFile->m_Read_Access = (Mode & FILE_OPEN_READ) ? 1 : 0;
		if (!pElem->m_Element.m_Ref_Count){
			pFile->m_pMFT_Element->m_Element.m_Write_Locked = MFT_ELEMENT_LOCKED;
			pFile->m_pMFT_Element->m_Element.m_Read_Locked = (Mode & FILE_OPEN_READ) ? MFT_ELEMENT_LOCKED : MFT_ELEMENT_NOT_LOCKED;
		}
		pElem->m_Element.m_Ref_Count++;
		pFile->m_Mode = VFS_FILE_MODE_ARCHIVE;
		pFile->m_UID = pElem->m_Element.m_UID;
		pFile->m_pFileData = pEntry;
		m_Open_Files.Add_Tail(pFile);
		return pFile;
	}
	if ((Mode & FILE_MODE_ALL) == FILE_OPEN_ALWAYS){
		pEntry = Locate_File(NULL, pName, true, &created);
		if (!pEntry){
			m_Last_Error = VFS_CANNOT_CREATE_FILE;
			return NULL;
		}
		pElem = pEntry->m_pThis_Element;
		if (created && !(Mode & FILE_OPEN_WRITE)){
			m_Last_Error = VFS_INVALID_OPEN_MODE;
			return NULL;
		}
		if ((Mode & FILE_OPEN_READ_WRITE) && pElem->m_Element.m_Write_Locked){
			m_Last_Error = VFS_WRITE_LOCKED_FILE;
			return NULL;
		}
		if ((Mode & FILE_OPEN_READ_WRITE) && pElem->m_Element.m_Read_Locked){
			m_Last_Error = VFS_READ_LOCKED_FILE;
			return NULL;
		}
		pElem->m_Element.m_Dirty = MFT_ELEMENT_DIRTY;
		if (!(pFile = new OPEN_FILE)){
			if (created) goto __cleanup_free;
			return NULL;
		}
		pFile->m_Offset = 0;
		pFile->m_pMFT_Element = pElem;

#ifdef VFS_ENABLE_AES
		if ((Mode & VFS_OPEN_AES_FLAG) == VFS_OPEN_AES_FLAG)
			pFile->m_pMFT_Element->m_Element.m_Encrypted = VFS_FLAG_ENCRYPTED;
#endif //VFS_ENABLE_AES

		
		pFile->m_pCache = NULL;

		pFile->m_Write_Access = (Mode & FILE_OPEN_WRITE) ? 1 : 0;
		pFile->m_Read_Access = (Mode & FILE_OPEN_READ) ? 1 : 0;

		if (!pElem->m_Element.m_Ref_Count){
			pFile->m_pMFT_Element->m_Element.m_Write_Locked = (Mode & FILE_OPEN_WRITE) ? MFT_ELEMENT_LOCKED : MFT_ELEMENT_NOT_LOCKED;
			pFile->m_pMFT_Element->m_Element.m_Read_Locked = (Mode & FILE_OPEN_READ) ? MFT_ELEMENT_LOCKED : MFT_ELEMENT_NOT_LOCKED;
		}
		pElem->m_Element.m_Ref_Count++;
		pFile->m_Mode = VFS_FILE_MODE_ARCHIVE;
		pFile->m_UID = pElem->m_Element.m_UID;
		pFile->m_pFileData = pEntry;
		m_Open_Files.Add_Tail(pFile);
		return pFile;
	}
	if ((Mode & FILE_MODE_ALL) == FILE_OPEN_EXISTING){
		pEntry = Locate_File(NULL, pName, false, &created);
		if (!pEntry){
			m_Last_Error = VFS_CANNOT_CREATE_FILE;
			return NULL;
		}
		pElem = pEntry->m_pThis_Element;
		if (!(Mode & FILE_OPEN_READ_WRITE)){
			m_Last_Error = VFS_INVALID_OPEN_MODE;
			return NULL;
		}
		if ((Mode & FILE_OPEN_READ_WRITE) && pElem->m_Element.m_Write_Locked){
			m_Last_Error = VFS_WRITE_LOCKED_FILE;
			return NULL;
		}
		if ((Mode & FILE_OPEN_WRITE) && pElem->m_Element.m_Read_Locked){
			m_Last_Error = VFS_READ_LOCKED_FILE;
			return NULL;
		}
		pElem->m_Element.m_Dirty = MFT_ELEMENT_DIRTY;
		if (!(pFile = new OPEN_FILE)){
			return NULL;
		}
		pFile->m_Offset = 0;
		pFile->m_pMFT_Element = pElem;

		pFile->m_Write_Access = (Mode & FILE_OPEN_WRITE) ? 1 : 0;
		pFile->m_Read_Access = (Mode & FILE_OPEN_READ) ? 1 : 0;

		
		pFile->m_pCache = NULL;

		if (!pElem->m_Element.m_Ref_Count){
			pFile->m_pMFT_Element->m_Element.m_Write_Locked = (Mode & FILE_OPEN_WRITE) ? MFT_ELEMENT_LOCKED : MFT_ELEMENT_NOT_LOCKED;
			pFile->m_pMFT_Element->m_Element.m_Read_Locked = (Mode & FILE_OPEN_READ) ? MFT_ELEMENT_LOCKED : MFT_ELEMENT_NOT_LOCKED;
		}
		pFile->m_Mode = VFS_FILE_MODE_ARCHIVE;
		pElem->m_Element.m_Ref_Count++;
		pFile->m_UID = pElem->m_Element.m_UID;
		pFile->m_pFileData = pEntry;
		m_Open_Files.Add_Tail(pFile);
		return pFile;
	}
	m_Last_Error = VFS_INVALID_OPEN_MODE;
	return NULL;

__cleanup_free:	
	pEntry->Delete(true, this);
	delete pEntry;
	return NULL;
}


POPEN_FILE VFS::ArchiveCreateFiledataFile(ptr pFiledata, UINT32 Mode){
	CEntry *pEntry;
	PINTERNAL_MFT_ELEMENT pElem;
	POPEN_FILE pFile;
	m_Last_Error = VFS_OK;
	pEntry = (CEntry*)pFiledata;
	if ((Mode & FILE_MODE_ALL) == FILE_OPEN_EXISTING){
		if (!pEntry){
			m_Last_Error = VFS_CANNOT_OPEN_FILE;
			return NULL;
		}
		pElem = pEntry->m_pThis_Element;
		if (!(Mode & FILE_OPEN_READ_WRITE)){
			m_Last_Error = VFS_INVALID_OPEN_MODE;
			return NULL;
		}
		if ((Mode & FILE_OPEN_READ_WRITE) && pElem->m_Element.m_Write_Locked){
			m_Last_Error = VFS_WRITE_LOCKED_FILE;
			return NULL;
		}
		if ((Mode & FILE_OPEN_READ_WRITE) && pElem->m_Element.m_Read_Locked){
			m_Last_Error = VFS_READ_LOCKED_FILE;
			return NULL;
		}
		pElem->m_Element.m_Dirty = MFT_ELEMENT_DIRTY;
		if (!(pFile = new OPEN_FILE)){
			return NULL;
		}
		pFile->m_Offset = 0;
		pFile->m_pMFT_Element = pElem;

		pFile->m_Write_Access = (Mode & FILE_OPEN_WRITE) ? 1 : 0;
		pFile->m_Read_Access = (Mode & FILE_OPEN_READ) ? 1 : 0;

		if (!pElem->m_Element.m_Ref_Count){
			pFile->m_pMFT_Element->m_Element.m_Write_Locked = (Mode & FILE_OPEN_WRITE) ? MFT_ELEMENT_LOCKED : MFT_ELEMENT_NOT_LOCKED;
			pFile->m_pMFT_Element->m_Element.m_Read_Locked = (Mode & FILE_OPEN_READ) ? MFT_ELEMENT_LOCKED : MFT_ELEMENT_NOT_LOCKED;
		}
		pElem->m_Element.m_Ref_Count++;
		pFile->m_Mode = VFS_FILE_MODE_ARCHIVE;
		pFile->m_UID = pElem->m_Element.m_UID;
		pFile->m_pFileData = pEntry;
		m_Open_Files.Add_Tail(pFile);
		return pFile;
	}
	m_Last_Error = VFS_INVALID_OPEN_MODE;
	return NULL;
}

void VFS::ArchiveCloseFile(POPEN_FILE pFile){

	if ((--pFile->m_pMFT_Element->m_Element.m_Ref_Count) == 0){
		pFile->m_pMFT_Element->m_Element.m_Write_Locked = MFT_ELEMENT_NOT_LOCKED;
		pFile->m_pMFT_Element->m_Element.m_Read_Locked = MFT_ELEMENT_NOT_LOCKED;
		if (pFile->m_pMFT_Element->m_pCluster_Chain){
			g_pFreeer(pFile->m_pMFT_Element->m_pCluster_Chain);
			pFile->m_pMFT_Element->m_pCluster_Chain = NULL;
		}
	}
	if (pFile->m_pCache)
		m_MultiCache.Free(pFile->m_pCache);
	m_Open_Files.Remove(pFile);
	delete pFile;
}

size_32 VFS::ArchiveRead(POPEN_FILE pFile, void* pBuffer, size_32 Count, PPROGESSCB pCB){
	size_32 res;
	pFile->m_Offset += (res = (pFile->m_Read_Access) ?
#ifdef VFS_ENABLE_AES
		(pFile->m_pMFT_Element->m_Element.m_Encrypted ?
		(size_32)AES_Cluster_Read(pFile->m_pMFT_Element, NULL, pFile->m_Offset, pBuffer, min((size_64)Count, pFile->m_pMFT_Element->m_Element.m_File_Size-pFile->m_Offset), pCB) : 
#endif //VFS_ENABLE_AES
		(size_32)Cluster_Read(pFile->m_pMFT_Element, NULL, pFile->m_Offset, pBuffer, min((size_64)Count, pFile->m_pMFT_Element->m_Element.m_File_Size-pFile->m_Offset), pCB)
#ifdef VFS_ENABLE_AES
		)
#endif //VFS_ENABLE_AES
		
		: 0);

	//pFile->m_Offset += (res = (size_32)Cluster_Read(pFile->m_pMFT_Element, NULL, pFile->m_Offset, pBuffer, (size_64)Count));
	return res;
}

size_32 VFS::ArchiveWrite(POPEN_FILE pFile, const void *pBuffer, size_32 Count, PPROGESSCB pCB){
	size_32 res;
	
	pFile->m_Offset += (res = (pFile->m_Write_Access) ?
#ifdef VFS_ENABLE_AES
		(pFile->m_pMFT_Element->m_Element.m_Encrypted ? 
		(size_32)AES_Cluster_Write(pFile->m_pMFT_Element, NULL, pFile->m_Offset, (ptr)pBuffer, (size_64)Count, pCB) :
#endif //VFS_ENABLE_AES
		(size_32)Cluster_Write(pFile->m_pMFT_Element, NULL, pFile->m_Offset, (ptr)pBuffer, (size_64)Count, pCB)
#ifdef VFS_ENABLE_AES
	)
#endif //VFS_ENABLE_AES
		: 0);

	if (res != 0)
		pFile->m_pMFT_Element->m_Element.m_GotMD5 = 0;
	return res;
}

size_64 VFS::ArchiveSeek(POPEN_FILE pFile, ssize_64 Offset, int Origin){
	/*switch (Origin){
		case SEEK_SET:
			pFile->m_Offset = (pFile->m_Write_Access ? Offset : min(Offset, (ssize_64) pFile->m_pMFT_Element->m_Element.m_File_Size));
			break;
		case SEEK_CUR:
			pFile->m_Offset += ((Offset >= 0) ? ((pFile->m_Write_Access ? Offset : min(Offset, (ssize_64) pFile->m_pMFT_Element->m_Element.m_File_Size))) : -(max(0, pFile->m_Offset+Offset)));
			break;
		case SEEK_END:
			pFile->m_Offset += (Offset > 0 && (pFile->m_Write_Access) ? max(Offset, 0) : -(max(0, pFile->m_Offset+Offset)));
			break;
		default:
			break;
	}//*/
	switch (Origin){
		case SEEK_SET:
			if (Offset < 0 || (!pFile->m_Write_Access && Offset > (ssize_64)pFile->m_pMFT_Element->m_Element.m_File_Size)) return (size_64)-1;
			pFile->m_Offset = (pFile->m_Write_Access ? Offset : min(Offset, (ssize_64) pFile->m_pMFT_Element->m_Element.m_File_Size));
			break;
		case SEEK_CUR:
			if ((pFile->m_Offset+Offset) < 0 || (!pFile->m_Write_Access && (pFile->m_Offset+Offset) > (ssize_64)pFile->m_pMFT_Element->m_Element.m_File_Size)) return (size_64)-1;
			pFile->m_Offset += ((Offset >= 0) ? ((pFile->m_Write_Access ? Offset : min(Offset, (ssize_64) pFile->m_pMFT_Element->m_Element.m_File_Size))) : -(max(0, pFile->m_Offset+Offset)));
			break;
		case SEEK_END:
			if ((pFile->m_Offset+Offset) < 0 || (!pFile->m_Write_Access && (pFile->m_Offset+Offset) > (ssize_64)pFile->m_pMFT_Element->m_Element.m_File_Size)) return (size_64)-1;			
			pFile->m_Offset += (Offset > 0 && (pFile->m_Write_Access) ? max(Offset, 0) : -(max(0, pFile->m_Offset+Offset)));
			break;
		default:
			break;
	}
	if (pFile->m_Offset > (ssize_64)(pFile->m_pMFT_Element->m_Element.m_File_Size))
		VFSSetEndOfFile(OpenFileToFileHandle(pFile));
	return pFile->m_Offset;
}

size_64 VFS::ArchiveTell(POPEN_FILE pFile){
	return pFile->m_Offset;
}

size_64 VFS::ArchiveSize(POPEN_FILE pFile){
	return pFile->m_pMFT_Element->m_Element.m_File_Size;
}

bool VFS::ArchiveEndOfFile(POPEN_FILE pFile){

	return (pFile->m_Offset >= (ssize_64)pFile->m_pMFT_Element->m_Element.m_File_Size);
}

bool VFS::ArchiveSetEndOfFile(POPEN_FILE pFile){
	PCLUSTER_HEADER pClusters;

	pFile->m_pMFT_Element->m_Element.m_GotMD5 = 0;

	if (pFile->m_Offset == 0){
		if (pFile->m_pMFT_Element->m_pCluster_Chain){
			Release_Clusters(pFile->m_pMFT_Element->m_pCluster_Chain);
			pFile->m_pMFT_Element->m_pCluster_Chain = NULL;
		}
		pFile->m_pMFT_Element->m_Element.m_First_Cluster = NO_CLUSTERS;
		pFile->m_pMFT_Element->m_Element.m_File_Size = 0;
		return true;
	}
	
	if (pFile->m_pMFT_Element->m_Element.m_First_Cluster == NO_CLUSTERS){
		pFile->m_pMFT_Element->m_pCluster_Chain = Allocate_Clusters(pFile->m_pMFT_Element->m_Element.m_File_Size = pFile->m_Offset, true, false, &pFile->m_pMFT_Element->m_Element.m_First_Cluster, NULL);
		return true;
	}
	pClusters = pFile->m_pMFT_Element->m_pCluster_Chain ? pFile->m_pMFT_Element->m_pCluster_Chain : Open_Cluster_Chain(pFile->m_pMFT_Element->m_Element.m_First_Cluster);
	if (!pClusters) return false;

	pFile->m_pMFT_Element->m_pCluster_Chain = Resize_Clusters(pClusters, pFile->m_pMFT_Element->m_Element.m_File_Size = (size_64)pFile->m_Offset, false, NULL);
	return true;
}

const void* VFS::ArchiveMemCache(POPEN_FILE pFile){
	size_64 size;
	void *pMem;
	if (pFile->m_Read_Access == 0 || pFile->m_Write_Access == 1) return NULL;
	if (pFile->m_pCache) return pFile->m_pCache;
	size = pFile->m_pMFT_Element->m_Element.m_File_Size;
	if (size > VFS_MAX_CACHE_SIZE) return NULL;
	
	pMem = m_MultiCache.Alloc(size);
	if (VFSRead(OpenFileToFileHandle(pFile), pMem, (size_32)size, NULL) != (size_32)size){
		m_MultiCache.Free(pMem);
		return NULL;
	}
	pFile->m_pCache = pMem;
	return pMem;
}

PCLUSTER_HEADER VFS::Resize_Clusters(PCLUSTER_HEADER pClusters, size_64 new_size, bool flush, size_64 *pOffset){
	size_64 realsize;
	size_64 cluster_count;
	size_64 old_count;
	size_64 old_size;
	size_64 offset;
	size_64 j;
	PCLUSTER_HEADER pCluster;


	if (m_Read_Only){
		m_Last_Error = VFS_READ_ONLY;
		return 0;
	}

	for (old_count = 0; pClusters[old_count].m_Next_Cluster != TAIL_CLUSTER; old_count++)
		;
	old_count++;

	old_size = old_count*m_Header.m_Cluster_Size;

	realsize = (j = cluster_count = ((new_size / m_Header.m_Cluster_Size)+((new_size % m_Header.m_Cluster_Size) ? 1 : 0))) * m_Header.m_Cluster_Size;

	if (!pClusters)
		return Allocate_Clusters(new_size, flush, false, pOffset, NULL);

	if (realsize == old_size)
		return pClusters; //Fits in old clusters

	if (realsize > old_size){ //Need more space
		if (!(pCluster = (PCLUSTER_HEADER)g_pAllocator(sizeof(struct CLUSTER_HEADER)*(size_t)cluster_count)))
			return NULL;

		memcpy(pCluster, pClusters, (size_t)old_count*sizeof(struct CLUSTER_HEADER));
		Allocate_Clusters(realsize-old_size, FALSE, FALSE, &offset, &pCluster[old_count]);

		pCluster[old_count-1].m_Next_Cluster = pCluster[old_count].m_This_Cluster;
		fseek_64(m_hMount, pCluster[old_count-1].m_This_Cluster, SEEK_SET);
		fwrite_(&pCluster[old_count-1], sizeof(struct CLUSTER_HEADER), m_hMount);

		pCluster[old_count].m_Previous_Cluster = pCluster[old_count-1].m_This_Cluster;
		fseek_64(m_hMount, pCluster[old_count].m_This_Cluster, SEEK_SET);
		fwrite_(&pCluster[old_count], sizeof(struct CLUSTER_HEADER), m_hMount);

		g_pFreeer(pClusters);

		pCluster[0].m_Cluster_Count = cluster_count;
		fseek_64(m_hMount, pCluster[0].m_This_Cluster, SEEK_SET);
		fwrite_(&pCluster[0], sizeof(struct CLUSTER_HEADER), m_hMount);

		//if (flush) MFT_Flush_To_Disk(pMFTData);
		return pCluster;

	} else { // need less space

		if (!(pCluster = (PCLUSTER_HEADER)g_pAllocator(sizeof(struct CLUSTER_HEADER)*(size_t)cluster_count)))
			return NULL;

		memcpy(pCluster, pClusters, (size_t)cluster_count*sizeof(struct CLUSTER_HEADER));
		pCluster[cluster_count-1].m_Next_Cluster = TAIL_CLUSTER;

		fseek_64(m_hMount, pCluster[cluster_count-1].m_This_Cluster, SEEK_SET);
		fwrite_(&pCluster[cluster_count-1], sizeof(struct CLUSTER_HEADER), m_hMount);
		Release_Clusters(&pClusters[cluster_count]);

		pCluster[0].m_Cluster_Count = cluster_count;
		fseek_64(m_hMount, pCluster[0].m_This_Cluster, SEEK_SET);
		fwrite_(&pCluster[0], sizeof(struct CLUSTER_HEADER), m_hMount);
		
		g_pFreeer(pClusters);
		//if (flush) MFT_Flush_To_Disk(pMFTData);
		return pCluster;
	}
}

size_64 VFS::Read_Raw_Cluster(PINTERNAL_MFT_ELEMENT pElement, PCLUSTER pCluster, UINT32 cluster){
	size_32 cluster_remaining_size;
	PCLUSTER_HEADER pClusterChain;
	
	if (!(pClusterChain = pElement->m_pCluster_Chain)){
		if (pElement->m_Element.m_First_Cluster == NO_CLUSTERS) return 0;
		if (!(pClusterChain = pElement->m_pCluster_Chain = Open_Cluster_Chain(pElement->m_Element.m_First_Cluster))) return 0;
		
	}
	if (cluster > pClusterChain[0].m_Cluster_Count-1)return 0;

	cluster_remaining_size = (size_32)min(m_Header.m_Cluster_Size, pElement->m_Element.m_File_Size-((size_64)cluster*m_Header.m_Cluster_Size));
	fseek_64(m_hMount, pClusterChain[cluster].m_This_Cluster, SEEK_SET);
	cluster_remaining_size = fread_(pCluster, sizeof(struct CLUSTER_HEADER)+cluster_remaining_size, m_hMount);
	return (size_64)cluster_remaining_size;
}

size_64 VFS::Read_Indexed_Raw_Cluster(PCLUSTER pCluster, UINT32 cluster){
	size_64 offset;

	if ((size_64)((sizeof(struct CLUSTER_HEADER)+m_Header.m_Cluster_Size)*(size_64)(cluster+1)) > fsize_64(m_hMount)) return NO_CLUSTERS;
	offset = (size_64)((sizeof(struct CLUSTER_HEADER)+m_Header.m_Cluster_Size)*(size_64)cluster);

	fseek_64(m_hMount, offset, SEEK_SET);
	fread_(pCluster, sizeof(struct CLUSTER_HEADER)+m_Header.m_Cluster_Size, m_hMount);
	return (size_64)offset;
}

size_64 VFS::Cluster_Read(PINTERNAL_MFT_ELEMENT pElement, PCLUSTER_HEADER pClusterChain, size_64 offset, ptr pDest, size_64 size, PPROGESSCB pCB){
	size_64 real_size;
	size_64 cluster_offset;
	size_32 r;
	size_32 cluster_remaining_size;
	UINT32 cluster_count;
	UINT32 first_cluster;
	

	if ((!pClusterChain && offset >= pElement->m_Element.m_File_Size) || (pClusterChain && offset >= pClusterChain[0].m_Cluster_Count*m_Header.m_Cluster_Size)){
		m_Last_Error = VFS_OUT_OF_BOUNDS;
		return 0;
	}

	real_size = pElement ? min(size, pElement->m_Element.m_File_Size-offset) : pClusterChain[0].m_Cluster_Count*m_Header.m_Cluster_Size;

	if (!pClusterChain && !(pClusterChain = pElement->m_pCluster_Chain)){
		if (pElement->m_Element.m_First_Cluster == NO_CLUSTERS){
			m_Last_Error = VFS_READ_NO_CLUSTERS;
			return 0;
		}
		if (!(pClusterChain = pElement->m_pCluster_Chain = Open_Cluster_Chain(pElement->m_Element.m_First_Cluster))){
			m_Last_Error = VFS_CANNOT_OPEN_CHAIN;
			return 0;
		}
	}
	cluster_count = (UINT32)pClusterChain[0].m_Cluster_Count;

	first_cluster = (size_32)(offset/m_Header.m_Cluster_Size);
	cluster_offset = pClusterChain[first_cluster].m_This_Data+(offset % m_Header.m_Cluster_Size);
	fseek_64(m_hMount, cluster_offset, SEEK_SET);

	real_size = size;
	size = 0;
	cluster_remaining_size = (size_32)(m_Header.m_Cluster_Size-(offset % m_Header.m_Cluster_Size));
	if (pCB) pCB->m_pCB(NULL, PROGRESS_TYPE_START, 0, real_size, pCB->m_pCookie);
	if (pCB && pCB->m_UseInternalCache){
		while (r = fread_(m_pCache, (size_32)min(real_size, min(VFS_CACHE_SIZE, real_size)), m_hMount)){
			size += r;
			real_size -= r;
			if (pCB) pCB->m_pCB(m_pCache, PROGRESS_TYPE_NEXT, size, real_size+size, pCB->m_pCookie);

			if ((++first_cluster) == pClusterChain[0].m_Cluster_Count) break;

			fseek_64(m_hMount, pClusterChain[first_cluster].m_This_Data, SEEK_SET);
			cluster_remaining_size = m_Header.m_Cluster_Size;
		}

	} else {
		while (r = fread_(pDest, (size_32)min(real_size, min(cluster_remaining_size, real_size)), m_hMount)){
			pDest = (ptr)(((UINT8*)pDest) + r);
			size += r;
			real_size -= r;
			if (pCB) pCB->m_pCB((ptr)(((UINT8*)pDest) - size), PROGRESS_TYPE_NEXT, size, real_size+size, pCB->m_pCookie);

			if ((++first_cluster) == pClusterChain[0].m_Cluster_Count) break;

			fseek_64(m_hMount, pClusterChain[first_cluster].m_This_Data, SEEK_SET);
			cluster_remaining_size = m_Header.m_Cluster_Size;
		}
	}
	if (pCB) pCB->m_pCB(NULL, PROGRESS_TYPE_END, size, real_size+size, pCB->m_pCookie);
	m_Last_Error = VFS_OK;
	return size;
}

#ifdef VFS_ENABLE_AES

size_64 VFS::AES_Cluster_Read(PINTERNAL_MFT_ELEMENT pElement, PCLUSTER_HEADER pClusterChain, size_64 offset, ptr pDest, size_64 size, PPROGESSCB pCB){
	size_64 real_size;
	size_64 cluster_offset;
	size_32 r;
	size_32 cluster_remaining_size;
	size_32 readstart;
	UINT32 cluster_count;
	UINT32 first_cluster;
	aes_decrypt_ctx ctx;
	unsigned char key[16];
	unsigned char iv[16];
	unsigned char liv[16];

	if (pElement->m_Element.m_Encrypted){
		if (!m_pfGetAESKey || !m_pfGetAESKey(pElement->m_Element.m_KeyIndex, key, iv)) return 0;
	}
	

	if ((!pClusterChain && offset >= pElement->m_Element.m_File_Size) || (pClusterChain && offset >= pClusterChain[0].m_Cluster_Count*m_Header.m_Cluster_Size)){
		m_Last_Error = VFS_OUT_OF_BOUNDS;
		return 0;
	}

	real_size = pElement ? min(size, pElement->m_Element.m_File_Size-offset) : pClusterChain[0].m_Cluster_Count*m_Header.m_Cluster_Size;

	if (!pClusterChain && !(pClusterChain = pElement->m_pCluster_Chain)){
		if (pElement->m_Element.m_First_Cluster == NO_CLUSTERS){
			m_Last_Error = VFS_READ_NO_CLUSTERS;
			return 0;
		}
		if (!(pClusterChain = pElement->m_pCluster_Chain = Open_Cluster_Chain(pElement->m_Element.m_First_Cluster))){
			m_Last_Error = VFS_CANNOT_OPEN_CHAIN;
			return 0;
		}
	}
	cluster_count = (UINT32)pClusterChain[0].m_Cluster_Count;

	first_cluster = (size_32)(offset/m_Header.m_Cluster_Size);
	readstart = (offset % m_Header.m_Cluster_Size);
	cluster_offset = pClusterChain[first_cluster].m_This_Data;
	fseek_64(m_hMount, cluster_offset, SEEK_SET);

	real_size = size;
	size = 0;
	cluster_remaining_size = (size_32)(m_Header.m_Cluster_Size-(offset % m_Header.m_Cluster_Size));
	if (pCB) pCB->m_pCB(NULL, PROGRESS_TYPE_START, 0, real_size, pCB->m_pCookie);

	while (fread_(m_pAESCache, m_Header.m_Cluster_Size, m_hMount) == m_Header.m_Cluster_Size){
		memcpy(liv, iv, 16);
		aes_decrypt_key128(key, &ctx);
		aes_cbc_decrypt(m_pAESCache, m_pAESCache, m_AESCacheSize, liv, &ctx);
		r = m_Header.m_Cluster_Size-readstart;
		r = (size_32)min(r, real_size);
		memcpy(pDest, m_pAESCache+readstart, r);
		readstart = 0;
		pDest = (ptr)(((UINT8*)pDest) + r);
		size += r;
		real_size -= r;
		if (pCB) pCB->m_pCB((ptr)(((UINT8*)pDest) - size), PROGRESS_TYPE_NEXT, size, real_size+size, pCB->m_pCookie);
		if ((++first_cluster) == pClusterChain[0].m_Cluster_Count) break;

		fseek_64(m_hMount, pClusterChain[first_cluster].m_This_Data, SEEK_SET);
		cluster_remaining_size = m_Header.m_Cluster_Size;
	}
	if (pCB) pCB->m_pCB(NULL, PROGRESS_TYPE_END, size, real_size+size, pCB->m_pCookie);
	m_Last_Error = VFS_OK;
	return size;
}

#endif //VFS_ENABLE_AES

size_64 VFS::Cluster_Write(PINTERNAL_MFT_ELEMENT pElement, PCLUSTER_HEADER* ppClusterChain, size_64 offset, ptr pSrc, size_64 size, PPROGESSCB pCB){
	size_64 real_size;
	size_64 cluster_offset;
	size_32 w;
	size_32 cluster_remaining_size;
	UINT32 cluster_count;
	UINT32 first_cluster;
	PCLUSTER_HEADER pClusterChain;

	if (m_Read_Only){
		m_Last_Error = VFS_READ_ONLY;
		return 0;
	}

	pClusterChain = ppClusterChain ? *ppClusterChain : NULL;
	

	if (!pClusterChain){
		if (pElement->m_Element.m_First_Cluster == NO_CLUSTERS){
			if (pElement->m_Element.m_File_Size < offset+size) pElement->m_Element.m_File_Size = offset+size;
			if (!(pClusterChain = pElement->m_pCluster_Chain = Allocate_Clusters(pElement->m_Element.m_File_Size, FALSE, FALSE, &pElement->m_Element.m_First_Cluster, NULL))){
				m_Last_Error = VFS_CANNOT_ALLOCATE;
				return 0;
			}
		} else if (pElement->m_pCluster_Chain == NULL){
			if (!(pClusterChain = Open_Cluster_Chain(pElement->m_Element.m_First_Cluster))){
				m_Last_Error = VFS_CANNOT_OPEN_CHAIN;
				return 0;
			}
			if (pElement->m_Element.m_File_Size < offset+size) pElement->m_Element.m_File_Size = offset+size;
			if (!(pClusterChain = pElement->m_pCluster_Chain = Resize_Clusters(pClusterChain, pElement->m_Element.m_File_Size, FALSE, NULL))){
				m_Last_Error = VFS_CANNOT_RESIZE;
				return 0;
			}
		} else if (size+offset > pElement->m_Element.m_File_Size){
			if (pElement->m_Element.m_File_Size < offset+size) pElement->m_Element.m_File_Size = offset+size;
			if (!(pClusterChain = pElement->m_pCluster_Chain = Resize_Clusters(pElement->m_pCluster_Chain, pElement->m_Element.m_File_Size, FALSE, NULL))){
				m_Last_Error = VFS_CANNOT_RESIZE;
				return 0;
			}
		} else {
			pClusterChain = pElement->m_pCluster_Chain;
		}
	} else if (size+offset > pClusterChain[0].m_Cluster_Count*m_Header.m_Cluster_Size){
		if (!(pClusterChain = Resize_Clusters(pClusterChain, offset+size, FALSE, NULL))){
			m_Last_Error = VFS_CANNOT_RESIZE;
			return 0;
		}
	}

	real_size = size;

	cluster_count = (UINT32)pClusterChain[0].m_Cluster_Count;// Get_Cluster_Count(pClusterChain);

	first_cluster = (size_32)(offset/m_Header.m_Cluster_Size);

	cluster_offset = pClusterChain[first_cluster].m_This_Data+(offset % m_Header.m_Cluster_Size);
	fseek_64(m_hMount, cluster_offset, SEEK_SET);
	size = 0;
	cluster_remaining_size = (size_32)(m_Header.m_Cluster_Size-(offset % m_Header.m_Cluster_Size));

	if (pCB) pCB->m_pCB(NULL, PROGRESS_TYPE_START, 0, real_size, pCB->m_pCookie);
	while (w = fwrite_(pSrc, (size_32)min(real_size, cluster_remaining_size), m_hMount)){
		//((UINT8*)pDest) += w;
		pSrc = (ptr)(((UINT8*)pSrc) + w);
		size += w;
		real_size -= w;

		if (pCB) pCB->m_pCB((ptr)(((UINT8*)pSrc) - size), PROGRESS_TYPE_NEXT, size, real_size+size, pCB->m_pCookie);

		if (!real_size)
			break;
		if ((++first_cluster) == pClusterChain[0].m_Cluster_Count) break;

		fseek_64(m_hMount, pClusterChain[first_cluster].m_This_Data, SEEK_SET);
		cluster_remaining_size = m_Header.m_Cluster_Size;
	}
	if (pCB) pCB->m_pCB(NULL, PROGRESS_TYPE_END, size, real_size+size, pCB->m_pCookie);
	m_Last_Error = VFS_OK;

	if (ppClusterChain) *ppClusterChain = pClusterChain;

	return size;
}

#ifdef VFS_ENABLE_AES

size_64 VFS::AES_Cluster_Write(PINTERNAL_MFT_ELEMENT pElement, PCLUSTER_HEADER* ppClusterChain, size_64 offset, ptr pSrc, size_64 size, PPROGESSCB pCB){
	size_64 real_size;
	size_64 cluster_offset;
	size_32 w;
	size_32 cluster_remaining_size;
	UINT32 cluster_count;
	UINT32 first_cluster;
	PCLUSTER_HEADER pClusterChain;
	size_32 datastart;
	aes_decrypt_ctx ctx;
	aes_encrypt_ctx ectx;
	unsigned char key[16];
	unsigned char iv[16];
	unsigned char liv[16];

	if (m_Read_Only){
		m_Last_Error = VFS_READ_ONLY;
		return 0;
	}

	pClusterChain = ppClusterChain ? *ppClusterChain : NULL;
	
	if (pElement->m_Element.m_Encrypted){
		if (!m_pfGetAESKey || !m_pfGetAESKey(pElement->m_Element.m_KeyIndex, key, iv)) return 0;
	}


	if (!pClusterChain){
		if (pElement->m_Element.m_First_Cluster == NO_CLUSTERS){
			if (pElement->m_Element.m_File_Size < offset+size) pElement->m_Element.m_File_Size = offset+size;
			if (!(pClusterChain = pElement->m_pCluster_Chain = Allocate_Clusters(pElement->m_Element.m_File_Size, FALSE, FALSE, &pElement->m_Element.m_First_Cluster, NULL))){
				m_Last_Error = VFS_CANNOT_ALLOCATE;
				return 0;
			}
		} else if (pElement->m_pCluster_Chain == NULL){
			if (!(pClusterChain = Open_Cluster_Chain(pElement->m_Element.m_First_Cluster))){
				m_Last_Error = VFS_CANNOT_OPEN_CHAIN;
				return 0;
			}
			if (pElement->m_Element.m_File_Size < offset+size) pElement->m_Element.m_File_Size = offset+size;
			if (!(pClusterChain = pElement->m_pCluster_Chain = Resize_Clusters(pClusterChain, pElement->m_Element.m_File_Size, FALSE, NULL))){
				m_Last_Error = VFS_CANNOT_RESIZE;
				return 0;
			}
		} else if (size+offset > pElement->m_Element.m_File_Size){
			if (pElement->m_Element.m_File_Size < offset+size) pElement->m_Element.m_File_Size = offset+size;
			if (!(pClusterChain = pElement->m_pCluster_Chain = Resize_Clusters(pElement->m_pCluster_Chain, pElement->m_Element.m_File_Size, FALSE, NULL))){
				m_Last_Error = VFS_CANNOT_RESIZE;
				return 0;
			}
		} else {
			pClusterChain = pElement->m_pCluster_Chain;
		}
	} else if (size+offset > pClusterChain[0].m_Cluster_Count*m_Header.m_Cluster_Size){
		if (!(pClusterChain = Resize_Clusters(pClusterChain, offset+size, FALSE, NULL))){
			m_Last_Error = VFS_CANNOT_RESIZE;
			return 0;
		}
	}

	real_size = size;

	cluster_count = (UINT32)pClusterChain[0].m_Cluster_Count;// Get_Cluster_Count(pClusterChain);

	first_cluster = (size_32)(offset/m_Header.m_Cluster_Size);

	cluster_offset = pClusterChain[first_cluster].m_This_Data;
	datastart = (offset % m_Header.m_Cluster_Size);
	fseek_64(m_hMount, cluster_offset, SEEK_SET);
	size = 0;
	cluster_remaining_size = (size_32)(m_Header.m_Cluster_Size-(offset % m_Header.m_Cluster_Size));

	//Read/decrypt cluster if not cached
	if (cluster_offset != m_AESCurrentCluster){
			fseek_64(m_hMount, m_AESCurrentCluster = cluster_offset, SEEK_SET);
			fread_(m_pAESWriteCache, m_Header.m_Cluster_Size, m_hMount);
			memcpy(liv, iv, 16);
			aes_decrypt_key128(key, &ctx);
			aes_cbc_decrypt(m_pAESWriteCache, m_pAESWriteCache, m_AESCacheSize, liv, &ctx);
			fseek_64(m_hMount, cluster_offset, SEEK_SET);
	}

	memcpy(m_pAESWriteCache+datastart, pSrc, w = (size_32)min(real_size-datastart, m_Header.m_Cluster_Size));

	if (pCB) pCB->m_pCB(NULL, PROGRESS_TYPE_START, 0, real_size, pCB->m_pCookie);

	memcpy(liv, iv, 16);
	aes_encrypt_key128(key, &ectx);
	BOOL b;
	b = aes_cbc_encrypt(m_pAESWriteCache, m_pAESCache, m_AESCacheSize, liv, &ectx);

	while (fwrite_(m_pAESCache, m_Header.m_Cluster_Size, m_hMount) == m_Header.m_Cluster_Size){
		//((UINT8*)pDest) += w;
		pSrc = (ptr)(((UINT8*)pSrc) + w);
		size += w;
		real_size -= w;

		if (!real_size)
			break;

		if ((++first_cluster) == pClusterChain[0].m_Cluster_Count) break;
		cluster_offset = pClusterChain[first_cluster].m_This_Data;

		if (real_size < m_Header.m_Cluster_Size){
			fseek_64(m_hMount, cluster_offset, SEEK_SET);
			fread_(m_pAESWriteCache, m_Header.m_Cluster_Size, m_hMount);
			memcpy(liv, iv, 16);
			aes_decrypt_key128(key, &ctx);
			aes_cbc_decrypt(m_pAESWriteCache, m_pAESWriteCache, m_AESCacheSize, liv, &ctx);
		}


		
		memcpy(m_pAESWriteCache, pSrc, w = (size_32)min(real_size, m_Header.m_Cluster_Size));
		
		memcpy(liv, iv, 16);
		aes_encrypt_key128(key, &ectx);
		aes_cbc_encrypt(m_pAESWriteCache, m_pAESCache, m_AESCacheSize, liv, &ectx);

		if (pCB) pCB->m_pCB((ptr)(((UINT8*)pSrc) - size), PROGRESS_TYPE_NEXT, size, real_size+size, pCB->m_pCookie);

		if (!real_size)
			break;

		fseek_64(m_hMount, m_AESCurrentCluster = cluster_offset, SEEK_SET);
		cluster_remaining_size = m_Header.m_Cluster_Size;
	}
	if (pCB) pCB->m_pCB(NULL, PROGRESS_TYPE_END, size, real_size+size, pCB->m_pCookie);
	m_Last_Error = VFS_OK;

	if (ppClusterChain) *ppClusterChain = pClusterChain;

	return size;
}

#endif //VFS_ENABLE_AES

CEntry* VFS::Allocate_Directory(const wchar_t *pName, size_64 ParentUID, size_64 *pUID){
	PINTERNAL_MFT_ELEMENT pElem;
	CEntry *pEntry, *pParent;
	pElem = Alloc_Element();
	if (!pElem) return NULL;
	pEntry = new CEntry;
	if (!pEntry)
		return NULL;
	
	pElem->m_Element.m_Type = MFT_ELEMENT_DIRECTORY;
	StrnCpyW(pElem->m_Element.m_Name, pName, NAME_LENGTH);
	if (pUID) *pUID = pElem->m_Element.m_UID;
	pElem->m_Element.m_Parent_UID = ParentUID;
#ifdef VFS_ENABLE_AES
	pElem->m_Element.m_Encrypted = 0;
#endif //VFS_ENABLE_AES

	pParent = Find_Entry_By_UID(ParentUID);
	pEntry->m_HashKey = Get_Str_HashW(pName, TRUE);
	m_pUID_Entries->Insert(pElem->m_Element.m_UID, &pEntry->m_pThis_UID_Node, pEntry);
	m_pChild_Entries->Insert(pEntry->m_HashKey, &pEntry->m_This_Child_Node, pEntry);

	if (pParent){
		//pParent->m_pChilds->Insert(pEntry->m_HashKey, &pEntry->m_pThis_Child_Node, pEntry);
		//pParent->m_Childs.Add_Tail(pEntry);
		pParent->m_Directories.Add_Tail(pEntry);
		pEntry->m_pParent = pParent;
	}
	pEntry->m_pThis_Element = pElem;
	m_Entries.Add_Tail(pEntry);
	return pEntry;
}

CEntry* VFS::Find_Entry_By_UID(size_64 UID){
	HashNode<CEntry*, size_64> *pHashNode;
	if (UID == ROOT_UID)
		return &m_Root_Dir;
	pHashNode = m_pUID_Entries->Search(UID);
	if (pHashNode){
		do {
			if (pHashNode->m_Data->m_pThis_Element->m_Element.m_UID == UID)
				return pHashNode->m_Data;
		} while (pHashNode = m_pUID_Entries->SearchContinue(pHashNode, UID));
	}
	return NULL;
}

CEntry* VFS::Find_Entry(CEntry* pParent, const wchar_t *pName){
	HASHKEY hash;
	size_64 UID;
	PMFT_ELEMENT pElem;
	ExternNodeHashNode<CEntry*, HASHKEY> *pNode;
	UID = pParent->m_pThis_Element->m_Element.m_UID;
	hash = Get_Str_HashW(pName, TRUE);
	if ((pNode = m_pChild_Entries->Search(hash))){
		do {
			pElem = &pNode->m_Data->m_pThis_Element->m_Element;
			if (pElem->m_Parent_UID == UID && wcsicmp(pName, pElem->m_Name) == 0)
				return pNode->m_Data;
		} while ((pNode = m_pChild_Entries->SearchContinue(pNode, hash)));
	}
	return NULL;
}

CEntry* VFS::Locate_Directory(CEntry* pBaseDir, const wchar_t *pPath, const wchar_t **ppPathTokens, int TokenCount, bool create, bool *pCreated){
	int len, i, totallen;
	const wchar_t *pTokens[128];
	wchar_t *pTemp, *_pPath;

	CEntry *pDir, *pLastEntry;
	
	_pPath = NULL;
	if(pCreated) *pCreated = false;
	if (!ppPathTokens){
		_pPath = StrDupW(pPath);
		if (!_pPath) return NULL;
		TokenCount = Get_Path_Token_CountW(_pPath);
		if (TokenCount < 1 || TokenCount > 128) goto __exit;

		totallen = 0;
		pTemp = _pPath;
		for (i = 0; i < TokenCount; i++){
			pTemp = Get_Path_TokenW(pTemp, &len, 0, FALSE);
			if (!pTemp || len > 255) goto __exit;
			*(pTemp+len) = 0;
			pTokens[i] = pTemp;
			pTemp += len + 1;
		}
		ppPathTokens = pTokens;
	} else if (TokenCount < 1) return &m_Root_Dir;
	
	pDir = pBaseDir ? pBaseDir : &m_Root_Dir;
	pLastEntry = NULL;
	for (i = 0; i < TokenCount; i++){
		pLastEntry = pDir;
		pDir = Find_Entry(pDir, ppPathTokens[i]);
		//pDir = pDir->FindChild(ppPathTokens[i]);
		if (!pDir || !(pDir->m_pThis_Element->m_Element.m_Type == MFT_ELEMENT_DIRECTORY)) break;
	}

	if (!pDir && create){
		//if (!pLastEntry) pLastEntry = &m_Root_Dir;
		pDir = pLastEntry ? pLastEntry : &m_Root_Dir;

		for (; i < TokenCount; i++){
			pDir = Allocate_Directory(ppPathTokens[i], pDir->m_pThis_Element->m_Element.m_UID, NULL);
		}
		PartialFlush();
		if(pCreated) *pCreated = true;
	}
	
	if (_pPath) g_pFreeer(_pPath);
	return pDir;
__exit:
	g_pFreeer(_pPath);
	return (TokenCount < 1) ? &m_Root_Dir : NULL;
}

CEntry *VFS::Locate_File(CEntry* pBaseDir, const wchar_t *pPath, bool create, bool *pCreated){
	int len, i, totallen;
	const wchar_t *pTokens[128];
	wchar_t *pTemp, *_pPath;

	CEntry *pDir, *pFile;
	const wchar_t **ppPathTokens;
	int TokenCount;
	
	if(pCreated) *pCreated = false;

	if (pBaseDir && pBaseDir->m_pThis_Element->m_Element.m_Type != MFT_ELEMENT_DIRECTORY) return NULL;

	_pPath = NULL;
	_pPath = StrDupW(pPath);
	if (!_pPath) return NULL;
	TokenCount = Get_Path_Token_CountW(_pPath);
	if (TokenCount < 1 || TokenCount > 128) goto __exit;

	totallen = 0;
	pTemp = _pPath;
	for (i = 0; i < TokenCount; i++){
		pTemp = Get_Path_TokenW(pTemp, &len, 0, FALSE);
		if (!pTemp || len > 255) goto __exit;
		*(pTemp+len) = 0;
		pTokens[i] = pTemp;
		pTemp += len + 1;
	}
	ppPathTokens = pTokens;

	pDir = (pBaseDir && TokenCount == 1) ? pBaseDir : Locate_Directory(pBaseDir, NULL, ppPathTokens, TokenCount-1, create, NULL);
	if (!pDir) goto __exit;
	pFile = Find_Entry(pDir, (ppPathTokens[TokenCount-1]));

	
	
	if (!pFile && create){
		pFile = Allocate_File(ppPathTokens[TokenCount-1], pDir->m_pThis_Element->m_Element.m_UID, NULL);
		PartialFlush();
		if(pCreated) *pCreated = true;
	} else if (pFile && pFile->m_pThis_Element->m_Element.m_Type != MFT_ELEMENT_FILE)
		goto __exit;
	
	if (_pPath) g_pFreeer(_pPath);
	return pFile;
__exit:
	g_pFreeer(_pPath);
	return NULL;
}

CEntry* VFS::Allocate_File(const wchar_t *pName, size_64 ParentUID, size_64 *pUID){
	PINTERNAL_MFT_ELEMENT pElem;
	CEntry *pEntry, *pParent;
	pElem = Alloc_Element();
	if (!pElem) return NULL;
	pEntry = new CEntry;
	if (!pEntry)
		return NULL;
	
	pElem->m_Element.m_Type = MFT_ELEMENT_FILE;
	StrnCpyW(pElem->m_Element.m_Name, pName, NAME_LENGTH);
	if (pUID) *pUID = pElem->m_Element.m_UID;
	pElem->m_Element.m_Parent_UID = ParentUID;
#ifdef VFS_ENABLE_AES
	pElem->m_Element.m_Encrypted = 0;
#endif //VFS_ENABLE_AES

	pParent = Find_Entry_By_UID(ParentUID);
	pEntry->m_HashKey = Get_Str_HashW(pName, TRUE);
	m_pUID_Entries->Insert(pElem->m_Element.m_UID, &pEntry->m_pThis_UID_Node, pEntry);
	m_pChild_Entries->Insert(pEntry->m_HashKey, &pEntry->m_This_Child_Node, pEntry);

	if (pParent){
		//pParent->m_pChilds->Insert(pEntry->m_HashKey, &pEntry->m_pThis_Child_Node, pEntry);
		//pParent->m_Childs.Add_Tail(pEntry);
		pParent->m_Files.Add_Tail(pEntry);
		pEntry->m_pParent = pParent;
	}
	pEntry->m_pThis_Element = pElem;
	m_Entries.Add_Tail(pEntry);
	return pEntry;
}

CEntry* VFS::Locate_Entry(const wchar_t *pPath, const wchar_t **ppPathTokens, int TokenCount){
	int len, i, totallen;
	const wchar_t *pTokens[128];
	wchar_t *pTemp, *_pPath;

	CEntry *pEntry;

	_pPath = NULL;
	if (!ppPathTokens){
		_pPath = StrDupW(pPath);
		if (!_pPath) return NULL;
		TokenCount = Get_Path_Token_CountW(_pPath);
		if (TokenCount < 1 || TokenCount > 128) goto __exit;

		totallen = 0;
		pTemp = _pPath;
		for (i = 0; i < TokenCount; i++){
			pTemp = Get_Path_TokenW(pTemp, &len, 0, FALSE);
			if (!pTemp || len > 255) goto __exit;
			*(pTemp+len) = 0;
			pTokens[i] = pTemp;
			pTemp += len + 1;
		}
		ppPathTokens = pTokens;
	} else if (TokenCount < 1) return NULL;
	
	pEntry = &m_Root_Dir;
	for (i = 0; i < TokenCount; i++){
		//pEntry = pEntry->FindChild(ppPathTokens[i]);
		pEntry = Find_Entry(pEntry, ppPathTokens[i]);
		if (!pEntry) break;
	}
	
	if (_pPath) g_pFreeer(_pPath);
	return pEntry;
__exit:
	g_pFreeer(_pPath);
	return NULL;
}

PMFT_ELEMENT VFS::Find_File(const wchar_t *pPath, bool Create){
	int TokenCount;
	int len, i, totallen;
	const wchar_t *pTokens[128];
	wchar_t *pTemp, *_pPath;

	_pPath = StrDupW(pPath);
	if (!_pPath) return NULL;
	TokenCount = Get_Path_Token_CountW(_pPath);
	if (TokenCount < 1 || TokenCount > 128) goto __exit;

	totallen = 0;
	pTemp = _pPath;
	for (i = 0; i < TokenCount; i++){
		pTemp = Get_Path_TokenW(pTemp, &len, 0, FALSE);
		if (!pTemp || len > 255) goto __exit;
		*(pTemp+len) = 0;
		pTokens[i] = pTemp;
		pTemp += len + 1;
	}
	g_pFreeer(_pPath);
	return NULL;
__exit:
	g_pFreeer(_pPath);
	return NULL;
}

bool VFS::Link_Data(){
	//CEntry*pENode;
	CEntry *pEntry, *pTemp;
	PINTERNAL_MFT_ELEMENT pElem;

	pElem = m_Elements.Get_Head();
	while (pElem){
		if (pElem->m_Element.m_Type == MFT_ELEMENT_UNUSED){
			pElem = pElem->m_pNext;
			continue;
		}
		pEntry = new CEntry;
		if (!pEntry)
			return false;
		if (!m_pUID_Entries->Insert(pElem->m_Element.m_UID, &pEntry->m_pThis_UID_Node, pEntry))
			return false;
		pEntry->m_pThis_Element = pElem;
		pEntry->m_HashKey = Get_Str_HashW(pElem->m_Element.m_Name, TRUE);
		if (!m_pChild_Entries->Insert(pEntry->m_HashKey, &pEntry->m_This_Child_Node, pEntry))
			return false;
		m_Entries.Add_Tail(pEntry);
		pElem = pElem->m_pNext;
	}
	pEntry = m_Entries.Get_Head();
	while (pEntry){
		pTemp = Find_Entry_By_UID(pEntry->m_pThis_Element->m_Element.m_Parent_UID);
		if (!pTemp)
			return false;
		pEntry->m_pParent = pTemp;
		/*if (!pTemp->m_pChilds->Insert(pEntry->m_HashKey, &pEntry->m_pThis_Child_Node, pEntry))
			return false;//*/
		//pTemp->m_Childs.Add_Tail(pEntry);
		((pEntry->m_pThis_Element->m_Element.m_Type == MFT_ELEMENT_FILE) ? pTemp->m_Files : pTemp->m_Directories).Add_Tail(pEntry);
		pEntry = pEntry->m_pNext2;
	}
	return true;
}

void VFS::Free_Data(){
	CEntry *pEntry, *pTemp;
	PINTERNAL_MFT_ELEMENT pElem, pTemp2;

	pEntry = m_Entries.Get_Head();
	while (pEntry){
		pTemp = pEntry->m_pNext2;
		delete pEntry;
		pEntry = pTemp;
	}

	pElem = m_Elements.Get_Head();
	while (pElem){
		pTemp2 = pElem->m_pNext;
		delete pElem;
		pElem = pTemp2;
	}
	m_Entries.Remove_All();
	m_Elements.Remove_All();
	m_Free_Elements.Remove_All();
	m_Root_Dir.Free_Data();

	delete m_pUID_Entries;

	delete m_pChild_Entries;

	m_pUID_Entries = NULL;
	m_pChild_Entries = NULL;
}


bool VFS::Load_FT(){
	MFT_ELEMENT Elem;
	PINTERNAL_MFT_ELEMENT pElem;
	size_64 UID;
	UID = 0;
	Pre_Allocate_Elements();
	fseek_64(m_hFT, sizeof(VFS_HEADER), SEEK_SET);
	while (fread_(&Elem, sizeof(MFT_ELEMENT), m_hFT) == sizeof(MFT_ELEMENT)){
		pElem = Alloc_Element();
		if (!pElem) return false;

		memcpy(&pElem->m_Element, &Elem, sizeof(MFT_ELEMENT));

		pElem->m_pCluster_Chain = NULL;
		pElem->m_Element.m_Ref_Count = 0;
		pElem->m_Element.m_Dirty = MFT_ELEMENT_NOT_DIRTY;
		pElem->m_Element.m_Write_Locked = MFT_ELEMENT_NOT_LOCKED;
		pElem->m_Element.m_Read_Locked = MFT_ELEMENT_NOT_LOCKED;
		
		if (pElem->m_Element.m_Type == MFT_ELEMENT_UNUSED)
			m_Free_Elements.Add_Tail(pElem);
		if (pElem->m_Element.m_UID > UID)
			UID = pElem->m_Element.m_UID;
	}
	m_UID = UID;
	return Link_Data();
}

PINTERNAL_MFT_ELEMENT VFS::Alloc_Element(){
	PINTERNAL_MFT_ELEMENT pElem;
	if ((pElem = m_Free_Elements.Get_Head()))
		m_Free_Elements.Remove(pElem);
	else {
		pElem = Create_Element(L"", 0, MFT_ELEMENT_FLAG_DIRTY, NULL);
		pElem->m_Element.m_This_Element = fsize_64(m_hFT);
		if (!m_Opening && !m_Read_Only){
			pElem->m_Element.m_UID = ++m_UID;
			fseek_64(m_hFT, pElem->m_Element.m_This_Element, SEEK_SET);
			fwrite_(&pElem->m_Element, sizeof(MFT_ELEMENT), m_hFT);
		}
		m_Elements.Add_Tail(pElem);
	}
	return pElem;
}

void VFS::Pre_Allocate_Elements(){
	int count, i;
	PINTERNAL_MFT_ELEMENT pElem;
	count = (int)((fsize_(m_hFT)-sizeof(VFS_HEADER))/sizeof(MFT_ELEMENT));
	fseek_(m_hFT, sizeof(VFS_HEADER), SEEK_SET);
	for (i = 0; i < count; i++){
		pElem = new INTERNAL_MFT_ELEMENT;
		fread_(&pElem->m_Element, sizeof(MFT_ELEMENT), m_hFT);
		Reset_Element(pElem);
		pElem->m_Element.m_Type = MFT_ELEMENT_UNUSED;
		m_Elements.Add_Tail(pElem);
		m_Free_Elements.Add_Tail(pElem);
	}
}

void VFS::Release_Element(const PINTERNAL_MFT_ELEMENT pElem){
	if (pElem->m_Element.m_Type != MFT_ELEMENT_UNUSED){
		pElem->m_Element.m_Type = MFT_ELEMENT_UNUSED;
		pElem->m_Element.m_Dirty = MFT_ELEMENT_DIRTY;
		if (pElem->m_pCluster_Chain){
			g_pFreeer(pElem->m_pCluster_Chain);
			pElem->m_pCluster_Chain = NULL;
		}

		if (pElem->m_Element.m_First_Cluster != NO_CLUSTERS){
			Release_Clusters(pElem->m_Element.m_First_Cluster);
			pElem->m_Element.m_First_Cluster = NO_CLUSTERS;
		}

		m_Free_Elements.Add_Tail(pElem);
	}
	PartialFlush();
}

void VFS::Clear_File(PMFT_ELEMENT pElem){

}

/*
VFS file functions
*/

VFSFileHandle VFS::VFSCreateFile(const wchar_t* pName, UINT32 Mode){
	POPEN_FILE pFile;
	CriticalSectionClass::LockClass lock(&m_Mount_Lock);
	if (!m_Is_Open) return NULL;


	
	if (Mode & FILE_OPEN_MEMORY)
		return OpenFileToFileHandle(MemoryCreateFile(pName, Mode));

	if (m_FlatPriority)
		pFile = (pFile = FlatCreateFile(pName, Mode)) ? pFile : ArchiveCreateFile(pName, Mode);
	else
		pFile = (pFile = ArchiveCreateFile(pName, Mode)) ? pFile : FlatCreateFile(pName, Mode);

	return OpenFileToFileHandle(pFile);
}


VFSFileHandle VFS::VFSCreateFiledataFile(ptr pFiledata, UINT32 Mode){
	CriticalSectionClass::LockClass lock(&m_Mount_Lock);
	if (!m_Is_Open) return NULL;

	if (Mode & FILE_OPEN_MEMORY)
		return OpenFileToFileHandle(MemoryCreateFiledataFile(pFiledata, Mode));

	return OpenFileToFileHandle(ArchiveCreateFiledataFile(pFiledata, Mode));
}

VFSFileHandle VFS::VFSCreateUIDFile(size_64 UID, UINT32 Mode){
	CriticalSectionClass::LockClass lock(&m_Mount_Lock);
	if (!m_Is_Open) return NULL;
	CEntry *pEntry;
	if ((Mode & FILE_MODE_ALL) != FILE_OPEN_EXISTING){
		m_Last_Error = VFS_INVALID_OPEN_MODE;
		return NULL;
	}
	if (UID == ROOT_UID || UID == INVALID_UID){
		return NULL;
	}
	if (!(pEntry = Find_Entry_By_UID(UID))){
		m_Last_Error = VFS_ERROR_INVALID_UID;
		return NULL;
	}
	if (pEntry->m_pThis_Element->m_Element.m_Type != MFT_ELEMENT_FILE){
		m_Last_Error = VFS_ERROR_NOT_A_FILE;
		return NULL;
	}
	return VFSCreateFiledataFile(pEntry, Mode);
}

void VFS::VFSCloseFile(VFSFileHandle hFile){
	POPEN_FILE pFile = FileHandleToOpenFile(hFile);
	CriticalSectionClass::LockClass lock(&m_Mount_Lock);
	if (!m_Is_Open) return;
	if (!m_Open_Files.Is_In_List(pFile)) return;
	(pFile->m_Mode == VFS_FILE_MODE_ARCHIVE) ?  ArchiveCloseFile(pFile) : ((pFile->m_Mode == VFS_FILE_MODE_FLAT) ? FlatCloseFile(pFile) : MemoryCloseFile(pFile));
}

size_32 VFS::VFSRead(VFSFileHandle hFile, void* pBuffer, size_32 Count, PPROGESSCB pCB){
	POPEN_FILE pFile = FileHandleToOpenFile(hFile);
	CriticalSectionClass::LockClass lock(&m_Mount_Lock);
	if (!m_Is_Open) return 0;
	if (!m_Open_Files.Is_In_List(pFile)) return 0;
	return (pFile->m_Mode == VFS_FILE_MODE_ARCHIVE) ? ArchiveRead(pFile, pBuffer, Count, pCB) : ((pFile->m_Mode == VFS_FILE_MODE_FLAT) ? FlatRead(pFile, pBuffer, Count, pCB) : MemoryRead(pFile, pBuffer, Count, pCB));
}

size_32 VFS::VFSWrite(VFSFileHandle hFile, const void *pBuffer, size_32 Count, PPROGESSCB pCB){
	POPEN_FILE pFile = FileHandleToOpenFile(hFile);
	CriticalSectionClass::LockClass lock(&m_Mount_Lock);
	if (!m_Is_Open) return 0;
	if (!m_Open_Files.Is_In_List(pFile)) return 0;
	return (pFile->m_Mode == VFS_FILE_MODE_ARCHIVE) ? ArchiveWrite(pFile, pBuffer, Count, pCB) : ((pFile->m_Mode == VFS_FILE_MODE_FLAT) ? FlatWrite(pFile, pBuffer, Count, pCB) : MemoryWrite(pFile, pBuffer, Count, pCB));
}

size_64 VFS::VFSSeek(VFSFileHandle hFile, ssize_64 Offset, int Origin){
	POPEN_FILE pFile = FileHandleToOpenFile(hFile);
	CriticalSectionClass::LockClass lock(&m_Mount_Lock);
	if (!m_Is_Open) return 0;
	if (!m_Open_Files.Is_In_List(pFile)) return 0;
	return (pFile->m_Mode == VFS_FILE_MODE_ARCHIVE) ? ArchiveSeek(pFile, Offset, Origin) : ((pFile->m_Mode == VFS_FILE_MODE_FLAT) ? FlatSeek(pFile, Offset, Origin) : MemorySeek(pFile, Offset, Origin));
}

size_64 VFS::VFSTell(VFSFileHandle hFile){
	POPEN_FILE pFile = FileHandleToOpenFile(hFile);
	CriticalSectionClass::LockClass lock(&m_Mount_Lock);
	if (!m_Is_Open) return 0;
	if (!m_Open_Files.Is_In_List(pFile)) return 0;
	return (pFile->m_Mode == VFS_FILE_MODE_ARCHIVE) ? ArchiveTell(pFile) : ((pFile->m_Mode == VFS_FILE_MODE_FLAT) ? FlatTell(pFile) : MemoryTell(pFile));
}

size_64 VFS::VFSSize(VFSFileHandle hFile){
	POPEN_FILE pFile = FileHandleToOpenFile(hFile);
	CriticalSectionClass::LockClass lock(&m_Mount_Lock);
	if (!m_Is_Open) return 0;
	if (!m_Open_Files.Is_In_List(pFile)) return 0;
	return (pFile->m_Mode == VFS_FILE_MODE_ARCHIVE) ? ArchiveSize(pFile) : ((pFile->m_Mode == VFS_FILE_MODE_FLAT) ? FlatSize(pFile) : MemorySize(pFile));
}

bool VFS::VFSEndOfFile(VFSFileHandle hFile){
	POPEN_FILE pFile = FileHandleToOpenFile(hFile);
	CriticalSectionClass::LockClass lock(&m_Mount_Lock);
	if (!m_Is_Open) return false;
	if (!m_Open_Files.Is_In_List(pFile)) return false;
	return (pFile->m_Mode == VFS_FILE_MODE_ARCHIVE) ? ArchiveEndOfFile(pFile) : ((pFile->m_Mode == VFS_FILE_MODE_FLAT) ? FlatEndOfFile(pFile) : MemoryEndOfFile(pFile));
}

bool VFS::VFSSetEndOfFile(VFSFileHandle hFile){
	POPEN_FILE pFile = FileHandleToOpenFile(hFile);
	CriticalSectionClass::LockClass lock(&m_Mount_Lock);
	if (!m_Is_Open) return false;
	if (!m_Open_Files.Is_In_List(pFile)) return false;
	return (pFile->m_Mode == VFS_FILE_MODE_ARCHIVE) ? ArchiveSetEndOfFile(pFile) : ((pFile->m_Mode == VFS_FILE_MODE_FLAT) ? FlatSetEndOfFile(pFile) : MemorySetEndOfFile(pFile));
}

const void* VFS::VFSMemCache(VFSFileHandle hFile){
	POPEN_FILE pFile = FileHandleToOpenFile(hFile);
	CriticalSectionClass::LockClass lock(&m_Mount_Lock);
	if (!m_Is_Open) return NULL;
	if (!m_Open_Files.Is_In_List(pFile)) return NULL;
	return (pFile->m_Mode == VFS_FILE_MODE_ARCHIVE) ? ArchiveMemCache(pFile) : ((pFile->m_Mode == VFS_FILE_MODE_FLAT) ? FlatMemCache(pFile) : MemoryMemCache(pFile));
}

size_64 VFS::VFSGetUIDFromFileHandle(VFSFileHandle hFile){
	POPEN_FILE pFile = FileHandleToOpenFile(hFile);
	CriticalSectionClass::LockClass lock(&m_Mount_Lock);

	if (!m_Is_Open) return INVALID_UID;
	if (!m_Open_Files.Is_In_List(pFile)) return INVALID_UID;
	return (pFile->m_UID);
}

bool VFS::VFSDeleteFiledataDirectory(ptr pDir){
	CEntry *pEntry;
	CriticalSectionClass::LockClass lock(&m_Mount_Lock);
	if (!m_Is_Open) return NULL;
	if (!pDir)
		return (m_Root_Dir.Delete(false, this) == VFS_DELETE_SUCCESS);
	
	if (!(pEntry = (CEntry*)pDir))
		return false;

	if (pEntry->m_pThis_Element->m_Element.m_Type != MFT_ELEMENT_DIRECTORY) return false;

	if (pEntry->Delete(true, this) == VFS_DELETE_SUCCESS){
		delete pEntry;
		return true;
	}
	m_Last_Error = VFS_DELETION_FAILED;
	return false;
}

bool VFS::VFSDeleteFiledataFile(ptr pFile){
	CEntry *pEntry;
	CriticalSectionClass::LockClass lock(&m_Mount_Lock);
	if (!(pEntry = (CEntry*)pFile))
		return false;
	if (pEntry->m_pThis_Element->m_Element.m_Type != MFT_ELEMENT_FILE) return false;

	if (pEntry->Delete(true, this) == VFS_DELETE_SUCCESS){
		delete pEntry;
		return true;
	}
	m_Last_Error = VFS_DELETION_FAILED;
	return false;
}

int VFS::VFSEnum(const wchar_t* pDirectory, pfEnumCallback pCB, ptr wParam, ptr lParam, int what){
	return EntryEnum(pDirectory, pCB, lParam, wParam, what);
}

int VFS::VFSSingleEnum(ptr pFileData, pfEnumCallback pCB, ptr wParam, ptr lParam){
	CEntry *pEntry, *pTemp;
	//GenericSLNode<CEntry>* pNode;
	VFSFILEINFO fInfo;
	int retval;

	pTemp = pEntry = pFileData ? (CEntry*)pFileData : &m_Root_Dir;

#ifdef VFS_ENABLE_NO_ENUM
	if (pEntry->NoEnum()) return VFS_ENUM_STOP;
#endif //VFS_ENABLE_NO_ENUM

	pEntry = pEntry->m_Directories.Get_Head();
	while (pEntry){
#ifdef VFS_ENABLE_NO_ENUM
		if (!pEntry->NoEnum()){
#endif //VFS_ENABLE_NO_ENUM
			if ((retval = (pCB(MFTElementToFileInfo2(&fInfo, pEntry->m_pThis_Element, pEntry), wParam, lParam) & VFS_ENUM_AND_VALUE)) == VFS_ENUM_STOP)
				return VFS_ENUM_STOP;
			if (retval == VFS_ENUM_BREAK) return VFS_ENUM_CONTINUE;
#ifdef VFS_ENABLE_NO_ENUM
		}
#endif //VFS_ENABLE_NO_ENUM
		pEntry = pEntry->m_pNext;
	}

	pEntry = pTemp->m_Files.Get_Head();
	while (pEntry){
#ifdef VFS_ENABLE_NO_ENUM
		if (!pEntry->NoEnum()){
#endif //VFS_ENABLE_NO_ENUM
			if ((retval = (pCB(MFTElementToFileInfo2(&fInfo, pEntry->m_pThis_Element, pEntry), wParam, lParam) & VFS_ENUM_AND_VALUE)) == VFS_ENUM_STOP)
				return VFS_ENUM_STOP;
			if (retval == VFS_ENUM_BREAK) return VFS_ENUM_CONTINUE;
#ifdef VFS_ENABLE_NO_ENUM
		}
#endif //VFS_ENABLE_NO_ENUM
		pEntry = pEntry->m_pNext;
	}
	return VFS_ENUM_CONTINUE;
}

bool VFS::VFSEmbedFile(const wchar_t *pSrcFile, const wchar_t* pDestFile, bool OverWrite, PPROGESSCB pCB){
	VFSFileHandle hFile;
	file f;
	size_32 r;
	size_64 size, fsize, offset;

	VFSFILEINFO info;

	CriticalSectionClass::LockClass lock(&m_Mount_Lock);
	if (!VALID_FILE(f = fopen_W(pSrcFile, L"r"))){
		m_Last_Error = VFS_FAILED_TO_OPEN_FILE;
		return false;
	}
	if (!VFS_VALID_FILE(hFile = VFSCreateFile(pDestFile, (OverWrite ? FILE_CREATE_ALWAYS : FILE_CREATE) | FILE_OPEN_WRITE))){
		m_Last_Error = VFS_FAILED_TO_CREATE_FILE;
		fclose_(f);
		return false;
	}
	
	fsize = size = fsize_64(f);
	
	offset = 0;
	VFSSeek(hFile, (ssize_64)size, SEEK_SET);
	VFSSeek(hFile, 0, SEEK_SET);
	if (pCB) pCB->m_pCB(NULL, PROGRESS_TYPE_START, 0, fsize, pCB->m_pCookie);
	while (size && (r = fread_(m_pCache, (size_32)min(size, VFS_CACHE_SIZE), f))){
		size -= r;

		offset += r;
		if (pCB) pCB->m_pCB(m_pCache, PROGRESS_TYPE_NEXT, offset, fsize, pCB->m_pCookie);

		if (VFSWrite(hFile, m_pCache, r, NULL) != r) goto __failed;
	}
	if (pCB) pCB->m_pCB(NULL, PROGRESS_TYPE_END, offset, fsize, pCB->m_pCookie);
	if (pCB){
		VFSQueryFileInfo(&info, VFSFileDataFromFilehandle((VFSFileHandle)hFile), m_pPath_Cache, VFS_PATH_CACHE_SIZE);
		pCB->m_pCB(&info, PROGRESS_TYPE_FILEINFO, 0, 0, pCB->m_pCookie);
	}
	if (size != 0) goto __failed;
	VFSCloseFile(hFile);
	fclose_(f);
	m_Last_Error = VFS_OK;
	return true;
__failed:
	VFSCloseFile(hFile);
	fclose_(f);
	VFSDeleteFile(pDestFile);
	return false;
}

bool VFS::VFSExtractFile(const wchar_t *pSrcFile, const wchar_t* pDestFile, PPROGESSCB pCB){
	VFSFileHandle hFile;
	file f;
	size_32 r;
	size_64 size, fsize, offset;
	CriticalSectionClass::LockClass lock(&m_Mount_Lock);
	if (!VALID_FILE(f = fopen_W(pDestFile, L"w"))){
		m_Last_Error = VFS_FAILED_TO_CREATE_FILE;
		return false;
	}
	if (!VFS_VALID_FILE(hFile = VFSCreateFile(pSrcFile, FILE_OPEN_EXISTING|FILE_OPEN_READ))){
		m_Last_Error = VFS_FAILED_TO_OPEN_FILE;
		fclose_(f);
		return false;
	}
	fsize = size = VFSSize(hFile);
	offset = 0;
	if (pCB) pCB->m_pCB(NULL, PROGRESS_TYPE_START, 0, fsize, pCB->m_pCookie);
	while (size && (r = VFSRead(hFile, m_pCache, (size_32)min(size, VFS_CACHE_SIZE), NULL))){
		size -= r;

		offset += r;
		if (pCB) pCB->m_pCB(m_pCache, PROGRESS_TYPE_NEXT, offset, fsize, pCB->m_pCookie);

		if (fwrite_(m_pCache, r, f) != r) goto __failed;
	}//*/
	if (pCB) pCB->m_pCB(NULL, PROGRESS_TYPE_END, offset, fsize, pCB->m_pCookie);
	if (size != 0) goto __failed;
	VFSCloseFile(hFile);
	fclose_(f);
	m_Last_Error = VFS_OK;
	return true;
__failed:
	VFSCloseFile(hFile);
	fclose_(f);
	VFSDeleteFile(pDestFile);
	return false;
}


bool VFS::VFSEmbedDir(const wchar_t *pSrcDir, const wchar_t* pDestDir, bool OverWrite, PPROGESSCB pCB){
	CEntry *pDir;
	int len;
	int total;
	int count;
	if (!pSrcDir) return NULL;
	StrCpyW(m_pPath_Cache2, pSrcDir);
	pDir = Locate_Directory(NULL, pDestDir, NULL, 0, true, NULL);
	if (!pDir) return false;
	len = StrLenW(pSrcDir);
	total = 0;
	count = 0;
	RecursiveCountProc(m_pPath_Cache2+len, m_pPath_Cache2, &total);
	//if (pCB) pCB->m_pCB(NULL, PROGRESS_TYPE_START, 0, (size_64)total, pCB->m_pCookie);
	if (total){
		RecursiveEmbedProc(pDir, m_pPath_Cache2+len, m_pPath_Cache2, total, &count, pCB);
	}
	//if (pCB) pCB->m_pCB(NULL, PROGRESS_TYPE_END, (size_64)count, (size_64)total, pCB->m_pCookie);
	LogFormated(L"Count: %d\n", count);
	return true;
}

bool VFS::VFSExtractDir(const wchar_t *pSrcDir, const wchar_t* pDestDir, bool OverWrite, PPROGESSCB pCB){
	CEntry *pDir;
	int len;
	int total;
	int count;
	if (!pDestDir) return NULL;
	StrCpyW(m_pPath_Cache2, pDestDir);
	len = StrLenW(m_pPath_Cache2);
	pDir = Locate_Directory(NULL, pSrcDir, NULL, 0, false, NULL);
	if (!pDir) return false;
	total = 0;
	count = 0;
	RecursiveCountProc2(pDir, &total);
	//if (pCB) pCB->m_pCB(NULL, PROGRESS_TYPE_START, 0, (size_64)total, pCB->m_pCookie);
	if (pCB) pCB->m_pCB(NULL, PROGRESS_TYPE_FILECOUNT, 0, (size_64)total, pCB->m_pCookie);
	if (total){
		RecursiveExtractProc(pDir, m_pPath_Cache2+len, m_pPath_Cache2, total, &count, pCB);
	}
	if (pCB) pCB->m_pCB(NULL, PROGRESS_TYPE_LASTFILE, 0, (size_64)total, pCB->m_pCookie);
	//if (pCB) pCB->m_pCB(NULL, PROGRESS_TYPE_END, (size_64)count, (size_64)total, pCB->m_pCookie);
	return true;
}


bool VFS::VFSEmbedFiledataFile(const wchar_t *pSrcFile, ptr pDestDir, bool OverWrite, PPROGESSCB pCB){
	VFSFileHandle hFile;
	file f;
	size_32 r;
	size_64 size, fsize, offset;
	wchar_t *pTemp;
	CEntry *pFile;
	VFSFILEINFO info;
	bool created;
	CriticalSectionClass::LockClass lock(&m_Mount_Lock);
	if (!VALID_FILE(f = fopen_W(pSrcFile, L"r"))){
		m_Last_Error = VFS_FAILED_TO_OPEN_FILE;
		return false;
	}
	pTemp = (wchar_t*)ULongToPtr(max(PtrToUlong(StrrChrW(m_pPath_Cache, '/')), PtrToUlong(StrrChrW(m_pPath_Cache, '\\'))));

	pFile = Locate_File((CEntry*)pDestDir, pTemp ? pTemp + 1 : pSrcFile, true, &created);

	if (!pFile){
		m_Last_Error = VFS_FAILED_TO_CREATE_FILE;
		fclose_(f);
		return false;
	}
	if (!created && !OverWrite){
		m_Last_Error = VFS_FILE_EXISTS;
		fclose_(f);
		return false;
	}

	if (!pFile || !VFS_VALID_FILE(hFile = VFSCreateFiledataFile(pFile, FILE_OPEN_EXISTING | FILE_OPEN_WRITE))){
		m_Last_Error = VFS_FAILED_TO_CREATE_FILE;
		fclose_(f);
		if (created){
			pFile->Delete(true, this);
			delete pFile;
		}
		return false;
	}
	
	fsize = size = fsize_64(f);
	
	offset = 0;
	VFSSeek(hFile, (ssize_64)size, SEEK_SET);
	VFSSeek(hFile, 0, SEEK_SET);
	if (pCB) pCB->m_pCB(NULL, PROGRESS_TYPE_START, 0, fsize, pCB->m_pCookie);
	while (size && (r = fread_(m_pCache, (size_32)min(size, VFS_CACHE_SIZE), f))){
		size -= r;

		offset += r;
		if (pCB) pCB->m_pCB(m_pCache, PROGRESS_TYPE_NEXT, offset, fsize, pCB->m_pCookie);

		if (VFSWrite(hFile, m_pCache, r, NULL) != r) goto __failed;
	}
	if (pCB) pCB->m_pCB(NULL, PROGRESS_TYPE_END, offset, fsize, pCB->m_pCookie);
	if (pCB){
		VFSQueryFileInfo(&info, VFSFileDataFromFilehandle((VFSFileHandle)hFile), m_pPath_Cache, VFS_PATH_CACHE_SIZE);
		pCB->m_pCB(&info, PROGRESS_TYPE_FILEINFO, 0, 0, pCB->m_pCookie);
	}
	if (size != 0) goto __failed;
	VFSCloseFile(hFile);
	fclose_(f);
	m_Last_Error = VFS_OK;
	return true;
__failed:
	VFSCloseFile(hFile);
	fclose_(f);
	VFSDeleteFiledataFile(pFile);
	return false;
}

bool VFS::VFSExtractFiledataFile(ptr pSrcFile, const wchar_t* pDestFile, PPROGESSCB pCB){
	VFSFileHandle hFile;
	file f;
	size_32 r;
	size_64 size, fsize, offset;
	CriticalSectionClass::LockClass lock(&m_Mount_Lock);
	if (!VALID_FILE(f = fopen_W(pDestFile, L"w"))){
		m_Last_Error = VFS_FAILED_TO_CREATE_FILE;
		return false;
	}
	if (!VFS_VALID_FILE(hFile = VFSCreateFiledataFile(pSrcFile, FILE_OPEN_EXISTING|FILE_OPEN_READ))){
		m_Last_Error = VFS_FAILED_TO_OPEN_FILE;
		fclose_(f);
		return false;
	}
	fsize = size = VFSSize(hFile);
	offset = 0;
	if (pCB) pCB->m_pCB(NULL, PROGRESS_TYPE_START, 0, fsize, pCB->m_pCookie);
	while (size && (r = VFSRead(hFile, m_pCache, (size_32)min(size, VFS_CACHE_SIZE), NULL))){
		size -= r;

		offset += r;
		if (pCB) pCB->m_pCB(m_pCache, PROGRESS_TYPE_NEXT, offset, fsize, pCB->m_pCookie);

		if (fwrite_(m_pCache, r, f) != r) goto __failed;
	}//*/
	if (pCB) pCB->m_pCB(NULL, PROGRESS_TYPE_END, offset, fsize, pCB->m_pCookie);
	if (size != 0) goto __failed;
	VFSCloseFile(hFile);
	fclose_(f);
	m_Last_Error = VFS_OK;
	return true;
__failed:
	VFSCloseFile(hFile);
	fclose_(f);
	VFSDeleteFile(pDestFile);
	return false;
}

bool VFS::VFSEmbedFiledataDir(const wchar_t *pSrcDir, ptr pDestDir, bool OverWrite, PPROGESSCB pCB){
	CEntry *pDir;
	int len;
	int total;
	int count;
	if (!pSrcDir) return NULL;
	pDir = pDestDir ? (CEntry*)pDestDir : &m_Root_Dir;

	StrCpyW(m_pPath_Cache2, pSrcDir);
	if (!pDir) return false;
	len = StrLenW(pSrcDir);
	total = 0;
	count = 0;
	RecursiveCountProc(m_pPath_Cache2+len, m_pPath_Cache2, &total);
	//if (pCB) pCB->m_pCB(NULL, PROGRESS_TYPE_START, 0, (size_64)total, pCB->m_pCookie);
	if (pCB) pCB->m_pCB(NULL, PROGRESS_TYPE_FILECOUNT, 0, (size_64)total, pCB->m_pCookie);
	if (total){
		RecursiveEmbedProc(pDir, m_pPath_Cache2+len, m_pPath_Cache2, total, &count, pCB);
	}
	if (pCB) pCB->m_pCB(NULL, PROGRESS_TYPE_LASTFILE, 0, (size_64)total, pCB->m_pCookie);
	//if (pCB) pCB->m_pCB(NULL, PROGRESS_TYPE_END, (size_64)count, (size_64)total, pCB->m_pCookie);
	LogFormated(L"Count: %d\n", count);
	return true;
}

bool VFS::VFSExtractFiledataDir(ptr pSrcDir, const wchar_t* pDestDir, bool OverWrite, PPROGESSCB pCB){
	CEntry *pDir;
	int len;
	int total;
	int count;
	if (!pDestDir) return NULL;

	pDir = pSrcDir ? (CEntry*)pSrcDir : &m_Root_Dir;
	StrCpyW(m_pPath_Cache2, pDestDir);
	len = StrLenW(m_pPath_Cache2);

	if (!pDir) return false;
	total = 0;
	count = 0;
	RecursiveCountProc2(pDir, &total);
	//if (pCB) pCB->m_pCB(NULL, PROGRESS_TYPE_START, 0, (size_64)total, pCB->m_pCookie);
	if (pCB) pCB->m_pCB(NULL, PROGRESS_TYPE_FILECOUNT, 0, (size_64)total, pCB->m_pCookie);
	if (total){
		RecursiveExtractProc(pDir, m_pPath_Cache2+len, m_pPath_Cache2, total, &count, pCB);
	}
	if (pCB) pCB->m_pCB(NULL, PROGRESS_TYPE_LASTFILE, 0, (size_64)total, pCB->m_pCookie);
	//if (pCB) pCB->m_pCB(NULL, PROGRESS_TYPE_END, (size_64)count, (size_64)total, pCB->m_pCookie);
	return true;
}

bool VFS::VFSQueryFileInfo(PVFSFILEINFO pFileInfo, ptr pFileData, wchar_t *pDest, int DestSize){
	CEntry *pEntry;
	if (!pDest){
		pDest = m_pPath_Cache;
		DestSize = VFS_PATH_CACHE_SIZE;
	}
	pEntry = (CEntry*)pFileData;
	if (pEntry->UnrolledMakeFilePath(pDest, DestSize, this) == -1) return false;
	pFileInfo->m_pFileData = pFileData;
	pFileInfo->m_pName = pEntry->m_pThis_Element->m_Element.m_Name;
	pFileInfo->m_pFullName = pDest;	
	pFileInfo->m_pUserData = pEntry->m_pUserData;
	pFileInfo->m_Size = ((pFileInfo->m_Type = pEntry->m_pThis_Element->m_Element.m_Type) == VFS_TYPE_FILE) ? pEntry->m_pThis_Element->m_Element.m_File_Size : 0;
	pFileInfo->m_Flags = pEntry->m_pThis_Element->m_Element.m_Flags;
	return true;
}

bool VFS::VFSSetFileUserData(ptr pFileData, ptr pUserData, size_32 size){
	memcpy(((CEntry*)pFileData)->m_pThis_Element->m_Element.m_Data, pUserData, min(size, 8));
	((CEntry*)pFileData)->m_pThis_Element->m_Element.m_Dirty = MFT_ELEMENT_DIRTY;
	return true;
}

bool VFS::VFSGetFileUserData(ptr pFileData, ptr pUserData, size_32 size){
	memcpy(pUserData, ((CEntry*)pFileData)->m_pThis_Element->m_Element.m_Data, min(size, 8));
	return true;
}

void VFS::VFSSetRuntimeUserData(ptr pFileData, ptr pUserData){
	((CEntry*)pFileData)->m_pUserData = pUserData;
}

ptr VFS::VFSGetRuntimeUserData(ptr pFileData){
	return ((CEntry*)pFileData)->m_pUserData;
}

ptr VFS::VFSGetRootFiledata(){
	return (ptr)&m_Root_Dir;
}

ptr VFS::VFSGetFileData(const wchar_t* pPath){
	return (ptr)Locate_Entry(pPath, NULL, 0);
}
bool VFS::VFSFileExists(const wchar_t* pFile){
	WIN32_FILE_ATTRIBUTE_DATA info;
	if (m_FlatPriority)
		return (GetFileAttributesExW(pFile, GetFileExInfoStandard, &info) != 0 && (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) ? TRUE : (Locate_File(NULL, pFile, false, NULL) ? TRUE : FALSE);
	else
		return (Locate_File(NULL, pFile, false, NULL)) ? TRUE : ((GetFileAttributesExW(pFile, GetFileExInfoStandard, &info) != 0 && (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) ? TRUE : FALSE);

}

bool VFS::VFSPathExists(const wchar_t* pPath){
	return Locate_Directory(NULL, pPath, NULL, 0, false, NULL) ? true : false;
}

size_64 VFS::VFSFileSize(const wchar_t* pPath){
	CEntry* pFile;
	pFile = Locate_File(NULL, pPath, FALSE, NULL);
	return pFile ? pFile->GetElement()->m_Element.m_File_Size : 0;
}

int VFS::VFSSetNoEnum(ptr pFileData, int noenum){
	int res;
	res = ((CEntry*)pFileData)->m_pThis_Element->m_Element.m_No_Enum;
	((CEntry*)pFileData)->m_pThis_Element->m_Element.m_No_Enum = noenum;
	return res;
}
int VFS::VFSGetNoEnum(ptr pFileData){
	return ((CEntry*)pFileData)->m_pThis_Element->m_Element.m_No_Enum;
}

void VFS::VFSSetFileOption(VFSFileHandle hFile, int fileoption, ptr pData, int size){
	POPEN_FILE pFile;
	pFile = FileHandleToOpenFile(hFile);
#ifdef VFS_ENABLE_AES
	if (fileoption == FILE_OPTION_AES_KEY && size == sizeof(int)){
		pFile->m_pMFT_Element->m_Element.m_KeyIndex = PtrToUlong(pData) & (127);
		return;
	}
#endif //VFS_ENABLE_AES
}

size_64 VFS::VFSGetFileUID(const wchar_t* pPath){
	CEntry* pEntry;
	CriticalSectionClass::LockClass lock(&m_Mount_Lock);
	pEntry = Locate_Entry(pPath, NULL, 0);
	if (!pEntry){
		m_Last_Error = VFS_CANNOT_FIND_ENTRY;
		return INVALID_UID;
	}
	if (pEntry->m_pThis_Element->m_Element.m_Type != MFT_ELEMENT_FILE){
		m_Last_Error = VFS_ERROR_NOT_A_FILE;
		return INVALID_UID;
	}
	return pEntry->m_pThis_Element->m_Element.m_UID;
}

size_64 VFS::VFSGetDirectoryUID(const wchar_t* pPath){
	CEntry* pEntry;
	CriticalSectionClass::LockClass lock(&m_Mount_Lock);
	pEntry = Locate_Entry(pPath, NULL, 0);
	if (!pEntry){
		m_Last_Error = VFS_CANNOT_FIND_ENTRY;
		return INVALID_UID;
	}
	if (pEntry->m_pThis_Element->m_Element.m_Type != MFT_ELEMENT_DIRECTORY){
		m_Last_Error = VFS_ERROR_NOT_A_DIRECTORY;
		return INVALID_UID;
	}
	return pEntry->m_pThis_Element->m_Element.m_UID;
}

size_64 VFS::VFSGetEntryUID(const wchar_t* pPath){
	CEntry* pEntry;
	CriticalSectionClass::LockClass lock(&m_Mount_Lock);
	pEntry = Locate_Entry(pPath, NULL, 0);
	if (!pEntry){
		m_Last_Error = VFS_CANNOT_FIND_ENTRY;
		return INVALID_UID;
	}
	return pEntry->m_pThis_Element->m_Element.m_UID;
}

size_64 VFS::VFSGetFiledataUID(ptr pFileData){
	if (!m_Is_Open) return INVALID_UID;
	return ((CEntry*)pFileData)->m_pThis_Element->m_Element.m_UID;
}


bool VFS::VFSDeleteFile(const wchar_t *pFile){
	CEntry* pEntry;
	CriticalSectionClass::LockClass lock(&m_Mount_Lock);
	pEntry = Locate_File(NULL, pFile, false, NULL);
	if (!pEntry){
		m_Last_Error = VFS_CANNOT_FIND_FILE;
		return false;
	}

	if (pEntry->Delete(true, this) == VFS_DELETE_SUCCESS){
		delete pEntry;
		return true;
	}
	m_Last_Error = VFS_DELETION_FAILED;
	return false;
}

bool VFS::VFSDeleteDirectory(const wchar_t *pDir){
	CEntry* pEntry;
	CriticalSectionClass::LockClass lock(&m_Mount_Lock);
	pEntry = Locate_Directory(NULL, pDir, NULL, 0, false, NULL);
	if (!pEntry){
		m_Last_Error = VFS_CANNOT_FIND_DIRECTORY;
		return false;
	}

	if (pEntry->Delete(true, this) == VFS_DELETE_SUCCESS){
		delete pEntry;
		return true;
	}
	m_Last_Error = VFS_DELETION_FAILED;
	return false;
}

int VFS::VFSGetSettings(){
	int res;
	res = 0;
#ifdef VFS_ENABLE_NO_ENUM
	res |= VFS_FLAG_NO_ENUM;
#endif //VFS_ENABLE_NO_ENUM

	return res;
}

size_32 VFS::VFSGetClusterSize(){
	return m_Header.m_Cluster_Size;
}

void VFS::DumpFTInfo(){
#ifdef _DEBUG
	int i;
	PINTERNAL_MFT_ELEMENT pElem;
	LogFormated(L"*** MFT ELEMENTS ***\n");
	pElem = m_Elements.Get_Head();
	i = 0;
	while (pElem){
#ifdef VFS_ENABLE_AES
	LogFormated(L"%10d: UID: %I64d Parent UID: %I64d Type: %s (%d) Dirty: %d Size: %-16I64u First cluster: %.16I64X This offset: %.16I64X Name: \"%s\" Encrypted: %s Key: %d\n",
#else //VFS_ENABLE_AES
	LogFormated(L"%10d: UID: %I64d Parent UID: %I64d Type: %s (%d) Dirty: %d Size: %-16I64u First cluster: %.16I64X This offset: %.16I64X Name: \"%s\"\n",
#endif //VFS_ENABLE_AES
		
			i++,
			pElem->m_Element.m_UID, pElem->m_Element.m_Parent_UID,
			Get_Type_Name2p(pElem->m_Element.m_Type), pElem->m_Element.m_Type,
			pElem->m_Element.m_Dirty,
			pElem->m_Element.m_File_Size,
			pElem->m_Element.m_First_Cluster,
			pElem->m_Element.m_This_Element,
			pElem->m_Element.m_Name
#ifdef VFS_ENABLE_AES
			,
			pElem->m_Element.m_Encrypted ? "yes" : "no",
			pElem->m_Element.m_KeyIndex
#endif //VFS_ENABLE_AES
			);
		pElem = pElem->m_pNext;
	}
	LogFormated(L"*** End of dump  ***\n");


#endif //_DEBUG
}

const wchar_t* VFS::TranslateErrorCode(int error){
	return TranslateVFSErrorCode(error);
}

bool VFS::VFSGetMD5(const wchar_t* pFile, unsigned char *pMD5, bool Store){
	POPEN_FILE _pFile;
	VFSFileHandle hFile;
	size_64 size;
	size_32 r;
	CriticalSectionClass::LockClass lock(&m_Mount_Lock);
	MD5_CTX ctx;
	unsigned char lBuf[1024];
	
	hFile = VFSCreateFile(pFile, FILE_OPEN_EXISTING | FILE_OPEN_READ);
	if (hFile == VFS_INVALID_FILE) return false;
	_pFile = FileHandleToOpenFile(hFile);

	MD5Init(&ctx, 0);
	size = VFSSize(hFile);

	while (size && (r = VFSRead(hFile, lBuf, sizeof(lBuf), NULL))){
		MD5Update(&ctx, lBuf, (unsigned int)r);
		size -= r;
	}

	MD5Final(&ctx);
	memcpy(pMD5, ctx.digest, 16);

	if (Store && !m_Read_Only && _pFile->m_Mode == VFS_FILE_MODE_ARCHIVE && _pFile->m_pMFT_Element){
		memcpy(_pFile->m_pMFT_Element->m_Element.m_MD5, pMD5, 16);
		_pFile->m_pMFT_Element->m_Element.m_GotMD5 = 1;
	}
	VFSCloseFile(hFile);
	return true;
}

bool VFS::VFSGetFileDataMD5(ptr pFileData, unsigned char *pMD5, bool Store){
	POPEN_FILE _pFile;
	VFSFileHandle hFile;
	size_64 size;
	size_32 r;
	CriticalSectionClass::LockClass lock(&m_Mount_Lock);
	MD5_CTX ctx;
	unsigned char lBuf[1024];
	
	hFile = VFSCreateFiledataFile(pFileData, FILE_OPEN_EXISTING | FILE_OPEN_READ);
	if (hFile == VFS_INVALID_FILE) return false;
	_pFile = FileHandleToOpenFile(hFile);

	MD5Init(&ctx, 0);
	size = VFSSize(hFile);

	while (size && (r = VFSRead(hFile, lBuf, sizeof(lBuf), NULL))){
		MD5Update(&ctx, lBuf, (unsigned int)r);
		size -= r;
	}

	MD5Final(&ctx);
	memcpy(pMD5, ctx.digest, 16);

	if (Store && !m_Read_Only && _pFile->m_Mode == VFS_FILE_MODE_ARCHIVE && _pFile->m_pMFT_Element){
		memcpy(_pFile->m_pMFT_Element->m_Element.m_MD5, pMD5, 16);
		_pFile->m_pMFT_Element->m_Element.m_GotMD5 = 1;
	}
	VFSCloseFile(hFile);
	return true;
}

bool VFS::VFSQueryMD5(const wchar_t* pFile, unsigned char *pMD5){
	CEntry *pEntry;
	CriticalSectionClass::LockClass lock(&m_Mount_Lock);

	pEntry = Locate_File(NULL, pFile, false, NULL);
	if (!pEntry || pEntry->m_pThis_Element->m_Element.m_Type != MFT_ELEMENT_FILE || !pEntry->m_pThis_Element->m_Element.m_GotMD5) return false;
	memcpy(pMD5, pEntry->m_pThis_Element->m_Element.m_MD5, 16);
	return true;
}

bool VFS::VFSQueryFileDataMD5(ptr pFileData, unsigned char *pMD5){
	CEntry *pEntry;
	CriticalSectionClass::LockClass lock(&m_Mount_Lock);

	pEntry = (CEntry*)pFileData;
	if (!pEntry || pEntry->m_pThis_Element->m_Element.m_Type != MFT_ELEMENT_FILE || !pEntry->m_pThis_Element->m_Element.m_GotMD5) return false;
	memcpy(pMD5, pEntry->m_pThis_Element->m_Element.m_MD5, 16);
	return true;
}


IStream* VFS::VFSCreateIStreamFile(const wchar_t* pName, UINT32 Mode){
	VFSFileHandle hFile;

	hFile = VFSCreateFile(pName, Mode);
	if (hFile == VFS_INVALID_FILE) return NULL;
	return new CVFSIStream(this, hFile);
}

IStream* VFS::VFSCreateIStreamFiledataFile(ptr pFiledata, UINT32 Mode){
	VFSFileHandle hFile;

	if ((Mode & FILE_MODE_ALL) != FILE_OPEN_EXISTING){
		m_Last_Error = VFS_INVALID_OPEN_MODE;
		return NULL;
	}
	hFile = VFSCreateFiledataFile(pFiledata, Mode);
	if (hFile == VFS_INVALID_FILE) return NULL;
	return new CVFSIStream(this, hFile);
}

bool VFS::VFSFileDataFileExists(ptr pFileData, const wchar_t* pFile){
	return (Locate_File((CEntry*)pFileData, pFile, false, NULL)) ? true : false;
}

bool VFS::VFSFileDataPathExists(ptr pFileData, const wchar_t* pPath){
	return Locate_Directory((CEntry*)pFileData, pPath, NULL, 0, false, NULL) ? true : false;
}

bool VFS::VFSRenameFile(const wchar_t* pFileName, const wchar_t* pNewName){
	if (m_Read_Only) return false;
	CEntry* pEntry;
	pEntry = Locate_File(NULL, pFileName, false, NULL);
	if (!pEntry){
		m_Last_Error = VFS_CANNOT_FIND_FILE;
		return false;
	}
	
	return pEntry->Rename(this, pNewName);
}

bool VFS::VFSRenameFolder(const wchar_t* pPathName, const wchar_t* pNewName){
	if (m_Read_Only) return false;
	CEntry* pEntry;
	pEntry = Locate_Directory(NULL, pPathName, NULL, 0, false, NULL);
	if (!pEntry){
		m_Last_Error = VFS_CANNOT_FIND_FILE;
		return false;
	}
	
	return pEntry->Rename(this, pNewName);
}

bool VFS::VFSRenameFiledataFile(ptr pFileData, const wchar_t* pNewName){
	if (m_Read_Only) return false;
	return ((CEntry*)pFileData)->Rename(this, pNewName);
}

bool VFS::VFSRenameFiledataFolder(ptr pFileData, const wchar_t* pNewName){
	if (m_Read_Only) return false;
	return ((CEntry*)pFileData)->Rename(this, pNewName);
}

ptr VFS::VFSCreateDirectory(ptr pFiledata, const wchar_t* pBaseName, wchar_t *pNewName, size_32 NewNameSize){
	wchar_t lBuf[NAME_LENGTH*2+1];
	int len;
	bool created;
	int i;

	ptr pRet;

	if (m_Read_Only) return NULL;

	if (StrLenW(pBaseName) > NAME_LENGTH) return NULL;

	StrCpyW(lBuf, pBaseName);

	created = false;

	i = 1;

	while ((pRet = Locate_Directory((CEntry*)pFiledata, lBuf, NULL, 0, true, &created)) && !created){
		len = _snwprintf(lBuf, (sizeof(lBuf)/sizeof(wchar_t))-1, L"%s (%d)", pBaseName, i++);
		if (len > NAME_LENGTH || len > (int)NewNameSize) break;
	}

	if (created){
		StrCpyW(pNewName, lBuf);
		return pRet;
	}
	return NULL;
}


bool VFS::VFSRegisterForProtocol(const wchar_t* pDomain){
	VFSUnregisterForProtocol();
	if (!(m_pDomain = StrDupW(pDomain))) return false;
	QueryCoreInterface()->RegisterForProtocol(this);
	return true;
}

void VFS::VFSUnregisterForProtocol(){
	if (!m_pDomain) return;
	QueryCoreInterface()->UnregisterForProtocol(this);
	g_pFreeer(m_pDomain);
	m_pDomain = NULL;
}

const wchar_t* VFS::VFSGetDomain(){
	return m_pDomain;
}

ptr VFS::VFSFileDataFromFilehandle(VFSFileHandle hFile){
	POPEN_FILE pFile = FileHandleToOpenFile(hFile);
	CriticalSectionClass::LockClass lock(&m_Mount_Lock);
	if (!m_Is_Open) return NULL;
	if (!m_Open_Files.Is_In_List(pFile)) return NULL;
	return pFile->m_pFileData;
}

bool VFS::VFSMoveFiledataEntry(ptr pEntry, ptr pNewDir){
	if (!pEntry) return false;
	if (!pNewDir) pNewDir = &m_Root_Dir;
	return ((CEntry*)pEntry)->Reattach((CEntry*)pNewDir);
}

bool VFS::VFSMoveEntry(const wchar_t *pPath, ptr pNewDir){
	CEntry* pEntry;
	pEntry = Find_Entry(NULL, pPath);
	if (!pEntry) return false;
	if (!pNewDir) pNewDir = &m_Root_Dir;
	return pEntry->Reattach((CEntry*)pNewDir);
}
