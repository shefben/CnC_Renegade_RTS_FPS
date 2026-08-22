#ifndef _BASETYPES_H_
#define _BASETYPES_H_

#include <windows.h>

typedef UINT64	size_64;
typedef UINT32	size_32;
typedef UINT16	size_16;
typedef INT64	ssize_64;
typedef INT32	ssize_32;
typedef INT16	ssize_16;
typedef void*	ptr;

#define SET_BIT(num, bit) ((num) |= (bit))
#define UNSET_BIT(num, bit) ((num) &= ~(bit))
#define TOGGLE_BIT(num, bit) ((num) ^= ((bit))
#define GET_BIT(num, bit) ((num) & (bit) ? 1 : 0)

#endif //_BASETYPES_H_
