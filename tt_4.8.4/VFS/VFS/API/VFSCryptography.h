#ifndef _VFSCRYPTOGRAPHY_H_
#define _VFSCRYPTOGRAPHY_H_

#define VFS_OPTION_AES_KEYCB	(0)

#define FILE_OPEN_AES			(1 << 6)

#define FILE_OPTION_AES_KEY		(1)

//128 bit AES

typedef bool (VFS_CALL *pfGetAESKey)(int KeyIndex, ptr pKey, ptr pIV);

#endif //_VFSCRYPTOGRAPHY_H_