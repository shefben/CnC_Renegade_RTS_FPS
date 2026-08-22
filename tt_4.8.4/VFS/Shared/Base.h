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

#define GetPtr(pvar) (*(void**)(pvar))
#define SetPtr(pvar, _ptr) (*((void**)pvar) = _ptr)

#define REMOVE_FLAG(type, flags) (~((type)(flags)))

#define va_next(va) ((va)+sizeof(void*))
#define va_seek(va, count) ((va)+(sizeof(void*)*(count)))
#endif //_BASE_H_
