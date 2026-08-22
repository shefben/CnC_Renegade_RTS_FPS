#ifndef _HASH_H_
#define _HASH_H_

#include "BaseTypes.h"
typedef UINT32 HASHKEY;

HASHKEY Get_Str_Hash(const char *pStr, BOOL ins);
HASHKEY Get_Str_HashW(const wchar_t *pStr, BOOL ins);
HASHKEY Get_Hash(const unsigned char *pStr, int len, HASHKEY initial_val, BOOL post_process);

#endif //_HASH_H_
