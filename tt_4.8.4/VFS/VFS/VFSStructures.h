#ifndef _VFSSTRUCTURES_H_
#define _VFSSTRUCTURES_H_

#include "../Shared/BaseTypes.h"
#include "../Shared/Lists.h"
#include "API/VFSBase.h"

//#define VFS_ENABLE_NO_ENUM

#define VFS_SETTING_NO_ENUM			(1)
#define VFS_ENABLE_AES
#define VFS_ENABLE_FILE_CACHE
#define VFS_CACHE_ALIGN				(5*1024*1024) //5MB
#define VFS_MAX_CACHE_SIZE			(20*1024*1024) //20MB
#define VFS_NO_USERVERSION_CHECK

#define OpenFileToFileHandle(pFile) ((VFSFileHandle)(pFile))
#define OpenFileToFileHandle(pFile) ((VFSFileHandle)(pFile))
#define FileHandleToOpenFile(hFile) ((POPEN_FILE)(hFile))

#ifdef VFS_ENABLE_AES
#define VFS_OPEN_AES_FLAG		((FILE_OPEN_AES) | (FILE_OPEN_WRITE))
#endif //VFS_ENABLE_AES

#define VFS_FILEFLAG_ENCRYPTED	(1)

#define VFS_FILE_MODE_FLAT		(0)
#define VFS_FILE_MODE_ARCHIVE	(1)
#define VFS_FILE_MODE_MEMORY	(2)

#define VFS_POOL_SIZES	(512) //Align for memory pools

#define VFS_FOUR_CC		('\0SFV') //VFS\0
//#define VFS_VERSION		('101V') //V101 //Moved to API/VFSVersion.h

#define FILE_TABLE_NAME	(L"$FT$")
#define DIR_TABLE_NAME	(L"$DT$")

#define HEAD_CLUSTER	((UINT64)(-1))
#define TAIL_CLUSTER	((UINT64)(-2))
#define NO_CLUSTERS		((UINT64)(-3))

#define NULL_CLUSTER	((UINT64)(-4))

#define CLUSTER_SIZE	(0x4000) //16kb, obsolete, clustersize is per VFS now
#define CLUSTER_ALIGN	(2) //Allocate clusters in that count

#define NAME_LENGTH		(255) //Maximum name length (excluding zero termination)
#define NAME_TYPE		wchar_t //Type used for names

#define ROOT_UID		((size_64)-1)
#define INVALID_UID		((size_64)-2)

#define VFS_PATH_CACHE_SIZE				(0x8000) //32k
#define VFS_CACHE_SIZE					(0x4000) //16k

//#define CHILD_HASH_LIST_SIZE			(0x2000) //8k


#define MFT_ELEMENT_FLAG_DIRTY			(1)
#define MFT_ELEMENT_FLAG_UNUSED			(1 << 1)
#define MFT_ELEMENT_FLAG_FILE			(2 << 1)
#define MFT_ELEMENT_FLAG_DIRECTORY		(3 << 1)

#define MFT_ELEMENT_FLAG_WRITE_LOCKED	(1 << 3)
#define MFT_ELEMENT_FLAG_READ_LOCKED	(1 << 4)
//#define MFT_ELEMENT_FLAG_ENCRYPTED		(1 << 5)

#define MFT_ELEMENT_NOT_DIRTY			(0)
#define MFT_ELEMENT_DIRTY				(1)

#define MFT_ELEMENT_UNUSED				(1)
#define MFT_ELEMENT_FILE				(2)
#define MFT_ELEMENT_DIRECTORY			(3)

#define MFT_ELEMENT_NOT_LOCKED			(0)
#define MFT_ELEMENT_LOCKED				(1)

#define MFT_ELEMENT_ENUM				(0)
#define MFT_ELEMENT_NO_ENUM				(1)
#define MFT_ELEMENT_NO_ENUM_ALL			(2)

#define MFT_ELEMENT_ENCRYPTED			(1)

#define MFT_TYPE_FLAGS					(MFT_ELEMENT_FLAG_UNUSED | MFT_ELEMENT_FLAG_FILE | MFT_ELEMENT_FLAG_DIRECTORY)

#define MFT_FLAGS_SET_TYPE(flags, type)	(((flags) & (~(UINT32)(MFT_TYPE_FLAGS))) | ((type) & MFT_TYPE_FLAGS))

#define MFT_ELEMENT_CLEAR_FLAGS ( \
	MFT_ELEMENT_FLAG_WRITE_LOCKED | \
	MFT_ELEMENT_FLAG_READ_LOCKED | \
	MFT_ELEMENT_FLAG_DIRTY \
	)

typedef struct CLUSTER_HEADER {
	size_64		m_Previous_Cluster;
	size_64		m_Next_Cluster;
	size_64		m_This_Cluster;
	size_64		m_Last_Cluster;		//Only valid in the first cluster of a cluster chain
	size_64		m_This_Data;
	size_64		m_Cluster_Count;	//Only valid in the first cluster of a cluster chain
	size_32		m_Checksum;
	size_32		m_Dirty;
} *PCLUSTER_HEADER;

typedef struct _ClusterHandle : AutoPoolClass<_ClusterHandle, 32>{
	PCLUSTER_HEADER	m_pChain;
} *ClusterHandle;


typedef struct CLUSTER {
	union {
		UINT8	m_Data[1];
		struct {
			struct CLUSTER_HEADER	m_Header;
			UINT8					m_Payload[1];
		};
	};
} *PCLUSTER;

typedef struct VFS_HEADER {
	size_32		m_Four_CC;
	size_32		m_Version;
	size_32		m_User_Version;

	size_32		m_Cluster_Size;
	size_32		m_Element_Count;

	size_64		m_First_Unused;

	size_64		m_Cluster_Count;
	wchar_t		m_Cluster_Name[256];
} *PVFSHEADER;

typedef struct MFT_ELEMENT {
	size_64					m_First_Cluster; //0
	size_64					m_File_Size; //8
	size_64					m_This_Element; //10
	UINT32					m_Ref_Count; //18
	union { //1C
		struct {
			UINT32					m_Dirty			: 1;
			UINT32					m_Type			: 2;
			UINT32					m_Write_Locked	: 1;
			UINT32					m_Read_Locked	: 1;
			UINT32					m_Locked		: 1;
			UINT32					m_No_Enum		: 2;
			UINT32					m_Encrypted		: 1;
			UINT32					m_KeyIndex		: 7;
			UINT32					m_GotMD5		: 1;
			UINT32					m_Reserved		: 32-17;
		};
		UINT32					m_Flags;
	};
	size_64					m_Parent_UID; //20
	size_64					m_UID; //28

	size_64					m_Checksum; //30
	unsigned char			m_Data[8]; //38
	unsigned char			m_MD5[16]; //40

	NAME_TYPE				m_Name[NAME_LENGTH+1]; //Name length + zero termination //50
} *PMFT_ELEMENT;

typedef struct INTERNAL_MFT_ELEMENT : public AutoPoolClass<INTERNAL_MFT_ELEMENT, VFS_POOL_SIZES>{
	MFT_ELEMENT							m_Element;
	PCLUSTER_HEADER						m_pCluster_Chain;
	INTERNAL_MFT_ELEMENT*				m_pNext;
	INTERNAL_MFT_ELEMENT*				m_pNext2;
	
	HashNode<MFT_ELEMENT*, HASHKEY>*	m_pThis_Node;
} *PINTERNAL_MFT_ELEMENT;

typedef struct DIRECTORY_ELEMENT {
	size_64		m_Parent_UID;
	size_64		m_UID;
	size_32		m_Flags;
	size_64		m_This_Offset;
	NAME_TYPE	m_Name[NAME_LENGTH+1]; //Name length + zero termination
} *PDIRECTORY_ELEMENT;

#define FILE_FLAG_READ	(1)
#define FILE_FLAG_WRITE	(1 << 1)

class OPEN_FILE : public AutoPoolClass<OPEN_FILE, 16>{
public:
	PINTERNAL_MFT_ELEMENT	m_pMFT_Element;
	ssize_64				m_Offset;
	size_64					m_Size;
	file					m_hFlat;
	void*					m_pCache;
	size_64					m_UID;
	VFSFileHandle			m_Handle;
	ptr						m_pFileData;

	OPEN_FILE(){
		m_pMFT_Element = NULL;
		m_Offset = 0;
		m_Size = 0;
		m_hFlat = INVALID_FILE;
		m_pCache = NULL;
		m_UID = INVALID_UID;
		m_Handle = 0;
		m_pFileData = NULL;
	}

	//size_32			m_Flags;
	struct {
		size_32			m_Write_Access	: 1;
		size_32			m_Read_Access	: 1;
		size_32			m_Mode			: 2;
		size_32			m_Reserved		: 32-4;
	};
};
typedef OPEN_FILE* POPEN_FILE;

struct VFSPROTOCOLNODE {
	VFSPROTOCOLNODE*	m_pNext;
	VFSPROTOCOLNODE*	m_pPrev;
	IVFS*				m_pThisVFS;
	const wchar_t*		m_pName;
};

#endif //_VFSSTRUCTURES_H_