#ifndef _VFSADVANCED_H_
#define _VFSADVANCED_H_
#include <windows.h>

#define VFS_FLAG_NO_ENUM	(1) //If no enum was enabled during archieve creation
#define VFS_FLAG_ENCRYPTED	(1) //Encrypted file

#define VFS_OPTION_AESKEY	(0)

#ifndef VFS_INTERNAL
#define MFT_ELEMENT_ENUM		(0)
#define MFT_ELEMENT_NO_ENUM		(1)
#define MFT_ELEMENT_NO_ENUM_ALL	(2)

#define VFS_SETTING_NO_ENUM		(1)
#endif //VFS_INTERNAL

typedef union _VFSFLAGS {
	struct {
		UINT32					m_Dirty			: 1;
		UINT32					m_Type			: 2;
		UINT32					m_Write_Locked	: 1;
		UINT32					m_Read_Locked	: 1;
		UINT32					m_Locked		: 1;
		UINT32					m_No_Enum		: 2;
		UINT32					m_Encrypted		: 1;
		UINT32					m_KeyIndex		: 7;
		UINT32					m_Reserved		: 32-16;
	};
	UINT32					m_Flags;
} VFSFLAGS, *PVFSFLAGS;


#endif //_VFSADVANCED_H_
