#ifndef _BASE_H_
#define _BASE_H_

#define SHARED_CALL		__fastcall
#define SHARED_CDECL	__cdecl
#define SHARED_STDCALL	__stdcall

#ifndef _DEBUG
#ifdef _M_X64
#define SHARED_INLINE	
#else //_M_X64
#define SHARED_INLINE	//__forceinline
#endif //_M_X64
#else //_DEBUG
#define SHARED_INLINE
#endif //_DEBUG

#define REMOVE_FLAG(type, flags) (~((type)(flags)))

#define PTR_DIFF(x, y)	((x) - (y))

/*#define INRANGE(c, Min, Max) \
	(((c) - (Min)) <= ((Max) - (Min)))*/

#define INRANGE(c, Min, Max) \
	((c) >= (Min) && (c) <= (Max))

#define ISHEX(c) ( \
	INRANGE((c), '0', '9') ||  \
	INRANGE((c), 'a', 'f') ||  \
	INRANGE((c), 'A', 'F') \
	)

#define ISDIGIT(c) (INRANGE((c), '0', '9'))

/*#define ISHEX(c) ( \
	((c) >= '0' && (c) <= '9') || \
	((c) >= 'a' && (c) <= 'f') || \
	((c) >= 'A' && (c) <= 'F') \
	)

#define ISDIGIT(c) ((c) >= '0' && (c) <= '9')//*/

#endif //_BASE_H_
