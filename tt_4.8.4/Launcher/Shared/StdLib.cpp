#include <windows.h>
#include "StdLib.h"
#include <stdio.h>
#include "Memory.h"
#include <stdlib.h>
/*
String functions
*/

int StrLen(const char *pStr){
	register int size = 0;
	while (*(pStr++))
		size++;
	return size;
}

char *StrCat2(char *pStr, const char *pAppend, BOOL ret_cur){
	register char *_pStr = pStr;
	while (*(pStr++))
		;
	pStr--;
	while (*(pStr++) = *(pAppend++))
		;
	return ((ret_cur == TRUE) ? pStr-1 : _pStr);
}

char *StrnCat2(char *pStr, const char *pAppend, int destsize, BOOL ret_cur){
	register char *_pStr = pStr;
	while (*(pStr++))
		destsize--;
	pStr--;
	while ((*(pStr++) = *(pAppend++)) && --destsize)
		;
	*(pStr--) = 0;
	return ((ret_cur == TRUE) ? pStr : _pStr);
}

char *StrCat(char *pStr, const char *pAppend){
	register char *_pStr = pStr;
	while (*(pStr++))
		;
	pStr--;
	while (*(pStr++) = *(pAppend++))
		;
	return _pStr;
}

char *StrnCat(char *pStr, const char *pAppend, int destsize){
	register char *_pStr = pStr;
	while (*(pStr++))
		destsize--;
	pStr--;
	while ((*(pStr++) = *(pAppend++)) && --destsize)
		;
	*(pStr--) = 0;
	return _pStr;
}

const char *StrChr(const char *pStr, char Chr){
	while (*pStr){
		if (*pStr == Chr) return pStr;
		pStr++;
	}
	return NULL;
}

const char *StrrChr(const char *pStr, char Chr){
	register const char *pResult = NULL;
	while (*pStr){
		if (*pStr == Chr) pResult = pStr;
		pStr++;
	}
	return pResult;
}

const char *StrStr(const char *pStr, const char *pFind){
	register const char *pLastStart;

	while (*pStr){
		pLastStart = pStr;

		while (*pStr == *pFind){
			pStr++;
			pFind++;
		}

		if (*pFind == 0)
			return pLastStart;

		pFind -= (pStr-pLastStart);
		pStr = pLastStart+1;

	}

	return NULL;
}

const char *BinaryStr(const void *pBin, const char *pFind, size_32 size){
	register const char *pLastStart;
	register const char *pStr;
	pStr = (const char*)pBin;
	while (size){
		pLastStart = pStr;

		while (*pStr == *pFind){
			pStr++;
			pFind++;
		}


		if (*pFind == 0)
			return pLastStart;

		pFind -= (pStr-pLastStart);

		pStr = pLastStart+1;
		size--;

	}

	return NULL;
}

const char *StrCpy(char *pDest, const char *pSrc){
	const char *pStr = pDest;

	while ((*(pDest++) = *(pSrc++)))
		;
	//*pDest = 0;
	return pStr;
}

const char *StrnCpy(char *pDest, const char *pSrc, int destsize){
	const char *pStr = pDest;

	while ((*(pDest++) = *(pSrc++)) && --destsize)
		;
	*pDest = 0;
	return pStr;
}
const char *StrnnCpy(char *pDest, const char *pSrc, int destsize, int srcsize){
	const char *pStr = pDest;

	while ((*(pDest++) = *(pSrc++)) && --destsize && --srcsize)
		;
	*pDest = 0;
	return pStr;
}

char *StrDup(const char *pSrc){
	register size_t len;
	register char *pResult;
	if (!pSrc || !pSrc[0]) return NULL;
	len = StrLen(pSrc);
	pResult = (char*)g_pAllocator(len+1);
	if (!pResult) return NULL;
	StrCpy(pResult, pSrc);
	return pResult;	
}

char *StrnDup(const char *pStr, int len){
	register char *pRes = (char*)pStr;
	register int _len = 0;
	while (*(pRes++) && _len < len) _len++;
	len = (_len < len) ? _len : len;
	if (pStr[len-1] != 0) len++;
	pRes = (char*)g_pAllocator(_len = len);
	while (--_len) *(pRes++) = *(pStr++);
	*pRes = 0;
	return pRes-(len-1);
}

/*
File path related stuff
*/

char *Strip_File_Path(char *_filename){
	register char *pStr = _filename;
	while (*_filename){
		if (*_filename == '\\') pStr = _filename+1;
		else if (*_filename == '/') pStr = _filename+1;
		_filename++;
	}
	return pStr ? ( (*pStr) ? pStr : NULL) : NULL;
}

char *Strip_Path_Parents(char *_path){
	register char *pStr = _path;
	register BOOL had_slash = FALSE;
	while (*_path){
		switch (*_path){
			case '\\':
			case '/':
				had_slash = TRUE;
				break;
			default:
				if (!had_slash) break;
				pStr = _path;
				had_slash = FALSE;
				break;
		}
		_path++;
	}
	return pStr;
}

void Fix_Slashes(char *pPath){
	register BOOL hadchar;
	register char *pDest;
	hadchar = FALSE;
	pDest = pPath;

	while (*pPath){
		if (*pPath == '\\' || *pPath == '/'){
			if (hadchar){
				*(pDest++) = '/';
				hadchar = FALSE;
			}			
		} else {
			*(pDest++) = *pPath;
			hadchar = TRUE;
		}
		pPath++;
	}
	*pDest = 0;
}

/*
It's assumed that it doesn't start with any slashes
*/

int Get_Path_Token_Length(const char *pToken){
	register int len = 0;
	while ((*pToken != '\\') && (*pToken) && (*pToken != '/')){
		len++;
		pToken++;
	}
	return len;
}

char *Remove_Slashes(char *pStr){
	register char *pDest = pStr, *pRes = pStr;
	while (*pStr){
		if (*pStr != '\\' && *pStr != '/') *(pDest++) = *pStr;
		pStr++;
	}
	*pDest = 0;
	return pRes;
}

int Get_Path_Token_Count(const char *pPath){
	register int result = 0;
	register BOOL had_char = FALSE;
	for (;;){
		switch (*pPath){
			case 0:
			case '/':
			case '\\':
				if (!had_char) break;
				result++;
				had_char = FALSE;
				break;
			default:
				had_char = TRUE;
		}
		if (*pPath == 0) break;
		pPath++;
	}
	return result;
}

char *Get_Path_Token(const char *pPath, int *pLength, int token, BOOL copy_token){
	register int len = 0;
	register int count = 0;
	register BOOL had_char = FALSE, had_slash = TRUE;
	const char *pPtr = pPath;
	for (;;){
		switch (*pPath){
			case 0:
			case '/':
			case '\\':
				had_slash = TRUE;
				if (!had_char) break;
				if (count++ == token){
					*pLength = len;
					if (!copy_token) return (char*)pPtr;
					return StrnDup(pPtr, len);
				}
				had_char = FALSE;
				break;
			default:
				had_char = TRUE;
				len++;
				if (!had_slash) break;
				had_slash = FALSE;
				pPtr = pPath;
				len = 1;
		}
		if (*pPath == 0) break;
		pPath++;
	}
	return NULL; //Should never reach this anyways, but makes a compiler warning without it
}

/*
Unicode file stuff
*/


/*wchar_t *Strip_File_PathW(wchar_t *_filename){
	register wchar_t *pStr = _filename;
	while (*_filename){
		if (*_filename == '\\') pStr = _filename+1;
		else if (*_filename == '/') pStr = _filename+1;
		_filename++;
	}
	return pStr ? ( (*pStr) ? pStr : NULL) : NULL;
}

wchar_t *Strip_Path_ParentsW(wchar_t *_path){
	register wchar_t *pStr = _path;
	register BOOL had_slash = FALSE;
	while (*_path){
		switch (*_path){
			case '\\':
			case '/':
				had_slash = TRUE;
				break;
			default:
				if (!had_slash) break;
				pStr = _path;
				had_slash = FALSE;
				break;
		}
		_path++;
	}
	return pStr;
}

void Fix_SlashesW(wchar_t *pPath){
	register BOOL hadchar;
	register wchar_t *pDest;
	hadchar = FALSE;
	pDest = pPath;

	while (*pPath){
		if (*pPath == '\\' || *pPath == '/'){
			if (hadchar){
				*(pDest++) = '/';
				hadchar = FALSE;
			}			
		} else {
			*(pDest++) = *pPath;
			hadchar = TRUE;
		}
		pPath++;
	}
	*pDest = 0;
}


int Get_Path_Token_LengthW(const wchar_t *pToken){
	register int len = 0;
	while ((*pToken != '\\') && (*pToken) && (*pToken != '/')){
		len++;
		pToken++;
	}
	return len;
}

wchar_t *Remove_SlashesW(wchar_t *pStr){
	register wchar_t *pDest = pStr, *pRes = pStr;
	while (*pStr){
		if (*pStr != '\\' && *pStr != '/') *(pDest++) = *pStr;
		pStr++;
	}
	*pDest = 0;
	return pRes;
}

int Get_Path_Token_CountW(const wchar_t *pPath){
	register int result = 0;
	register BOOL had_char = FALSE;
	for (;;){
		switch (*pPath){
			case 0:
			case '/':
			case '\\':
				if (!had_char) break;
				result++;
				had_char = FALSE;
				break;
			default:
				had_char = TRUE;
		}
		if (*pPath == 0) break;
		pPath++;
	}
	return result;
}

char *Get_Path_TokenW(const wchar_t *pPath, int *pLength, int token, BOOL copy_token){
	register int len = 0;
	register int count = 0;
	register BOOL had_char = FALSE, had_slash = TRUE;
	const wchar_t *pPtr = pPath;
	for (;;){
		switch (*pPath){
			case 0:
			case '/':
			case '\\':
				had_slash = TRUE;
				if (!had_char) break;
				if (count++ == token){
					*pLength = len;
					if (!copy_token) return (wchar_t*)pPtr;
					return StrnDup(pPtr, len);
				}
				had_char = FALSE;
				break;
			default:
				had_char = TRUE;
				len++;
				if (!had_slash) break;
				had_slash = FALSE;
				pPtr = pPath;
				len = 1;
		}
		if (*pPath == 0) break;
		pPath++;
	}
	return NULL; //Should never reach this anyways, but makes a compiler warning without it
}
*/

char *Format_Size(int size, char *buf, int bufsize){
	/*register int i;
	for (i = 0; i < 4; i++){
		if (size > (0x40000000 >> (i * 10))) break;
	}
	switch (i){
		case 0:
			_snprintf(buf, bufsize, "%.3f GB", (float)size/(float)(1024*1024*1024));
			break;
		case 1:
			_snprintf(buf, bufsize, "%.3f MB", (float)size/(float)(1024*1024));
			break;
		case 2:
			_snprintf(buf, bufsize, "%.3f KB", (float)size/(float)(1024));
			break;
		case 3:
			_snprintf(buf, bufsize, "%d B", size);
			break;
	}*/
	buf[0] = 0;
	return buf;
}

/*
Unicode version
*/


int StrLenW(const wchar_t *pStr){
	register int size = 0;
	while (*(pStr++))
		size++;
	return size;
}

wchar_t *StrCat2W(wchar_t *pStr, const wchar_t *pAppend, BOOL ret_cur){
	register wchar_t *_pStr = pStr;
	while (*(pStr++))
		;
	pStr--;
	while (*(pStr++) = *(pAppend++))
		;
	return ((ret_cur == TRUE) ? pStr-1 : _pStr);
}

wchar_t *StrnCat2W(wchar_t *pStr, const wchar_t *pAppend, int destsize, BOOL ret_cur){
	register wchar_t *_pStr = pStr;
	while (*(pStr++))
		destsize--;
	pStr--;
	while ((*(pStr++) = *(pAppend++)) && --destsize)
		;
	*(pStr--) = 0;
	return ((ret_cur == TRUE) ? pStr : _pStr);
}

wchar_t *StrCatW(wchar_t *pStr, const wchar_t *pAppend){
	register wchar_t *_pStr = pStr;
	while (*(pStr++))
		;
	pStr--;
	while (*(pStr++) = *(pAppend++))
		;
	return _pStr;
}

wchar_t *StrnCatW(wchar_t *pStr, const wchar_t *pAppend, int destsize){
	register wchar_t *_pStr = pStr;
	while (*(pStr++))
		destsize--;
	pStr--;
	while ((*(pStr++) = *(pAppend++)) && --destsize)
		;
	*(pStr--) = 0;
	return _pStr;
}

const wchar_t *StrChrW(const wchar_t *pStr, wchar_t Chr){
	while (*pStr){
		if (*pStr == Chr) return pStr;
		pStr++;
	}
	return NULL;
}

const wchar_t *StrrChrW(const wchar_t *pStr, wchar_t Chr){
	register const wchar_t *pResult = NULL;
	while (*pStr){
		if (*pStr == Chr) pResult = pStr;
		pStr++;
	}
	return pResult;
}

const wchar_t *StrStrW(const wchar_t *pStr, const wchar_t *pFind){
	register const wchar_t *pLastStart;

	while (*pStr){
		pLastStart = pStr;

		while (*pStr == *pFind){
			pStr++;
			pFind++;
		}

		if (*pFind == 0)
			return pLastStart;

		pFind -= (pStr-pLastStart);
		pStr = pLastStart+1;

	}

	return NULL;
}

const wchar_t *BinaryStrW(const void *pBin, const wchar_t *pFind, size_32 size){
	register const wchar_t *pLastStart;
	register const wchar_t *pStr;
	pStr = (const wchar_t*)pBin;
	while (size > 1){
		pLastStart = pStr;

		while (*pStr == *pFind){
			pStr++;
			pFind++;
		}

		if (*pFind == 0)
			return pLastStart;

		pFind -= (pStr-pLastStart);
		pStr = (wchar_t*)(((char*)pLastStart)+1);
		
		size--;

	}

	return NULL;
}

const wchar_t *StrCpyW(wchar_t *pDest, const wchar_t *pSrc){
	const wchar_t *pStr = pDest;

	while ((*(pDest++) = *(pSrc++)))
		;
	//*pDest = 0;
	return pStr;
}

const wchar_t *StrnCpyW(wchar_t *pDest, const wchar_t *pSrc, int destsize){
	const wchar_t *pStr = pDest;

	while ((*(pDest++) = *(pSrc++)) && --destsize)
		;
	*pDest = 0;
	return pStr;
}
const wchar_t *StrnnCpyW(wchar_t *pDest, const wchar_t *pSrc, int destsize, int srcsize){
	const wchar_t *pStr = pDest;

	while ((*(pDest++) = *(pSrc++)) && --destsize && --srcsize)
		;
	*pDest = 0;
	return pStr;
}

wchar_t *StrDupW(const wchar_t *pSrc){
	register size_t len;
	register wchar_t *pResult;
	if (!pSrc/* || !pSrc[0]*/) return NULL;
	len = StrLenW(pSrc);
	pResult = (wchar_t*)g_pAllocator((len+1)*sizeof(wchar_t));
	if (!pResult) return NULL;
	StrCpyW(pResult, pSrc);
	return pResult;	
}

wchar_t *StrnDupW(const wchar_t *pStr, int len){
	register wchar_t *pRes = (wchar_t*)pStr;
	register int _len = 0;
	while (*(pRes++) && _len < len) _len++;
	len = (_len < len) ? _len : len;
	if (pStr[len-1] != 0) len++;
	pRes = (wchar_t*)g_pAllocator((_len = len)*sizeof(wchar_t));
	while (--_len) *(pRes++) = *(pStr++);
	*pRes = 0;
	return pRes-(len-1);
}

wchar_t *Strip_File_PathW(wchar_t *_filename){
	register wchar_t *pStr = _filename;
	while (*_filename){
		if (*_filename == '\\') pStr = _filename+1;
		else if (*_filename == '/') pStr = _filename+1;
		_filename++;
	}
	return pStr ? ( (*pStr) ? pStr : NULL) : NULL;
}

wchar_t *Strip_Path_ParentsW(wchar_t *_path){
	register wchar_t *pStr = _path;
	register BOOL had_slash = FALSE;
	while (*_path){
		switch (*_path){
			case '\\':
			case '/':
				had_slash = TRUE;
				break;
			default:
				if (!had_slash) break;
				pStr = _path;
				had_slash = FALSE;
				break;
		}
		_path++;
	}
	return pStr;
}

/*
It's assumed that it doesn't start with any slashes
*/

int Get_Path_Token_LengthW(const wchar_t *pToken){
	register int len = 0;
	while ((*pToken != '\\') && (*pToken) && (*pToken != '/')){
		len++;
		pToken++;
	}
	return len;
}

wchar_t *Remove_SlashesW(wchar_t *pStr){
	register wchar_t *pDest = pStr, *pRes = pStr;
	while (*pStr){
		if (*pStr != '\\' && *pStr != '/') *(pDest++) = *pStr;
		pStr++;
	}
	*pDest = 0;
	return pRes;
}

int Get_Path_Token_CountW(const wchar_t *pPath){
	register int result = 0;
	register BOOL had_char = FALSE;
	for (;;){
		switch (*pPath){
			case 0:
			case '/':
			case '\\':
				if (!had_char) break;
				result++;
				had_char = FALSE;
				break;
			default:
				had_char = TRUE;
		}
		if (*pPath == 0) break;
		pPath++;
	}
	return result;
}

void Fix_SlashesW(wchar_t *pPath){
	register BOOL hadchar;
	register wchar_t *pDest;
	hadchar = FALSE;
	pDest = pPath;

	while (*pPath){
		if (*pPath == '\\' || *pPath == '/'){
			if (hadchar){
				*(pDest++) = '/';
				hadchar = FALSE;
			}			
		} else {
			*(pDest++) = *pPath;
			hadchar = TRUE;
		}
		pPath++;
	}
	*pDest = 0;
}

wchar_t *Get_Path_TokenW(const wchar_t *pPath, int *pLength, int token, BOOL copy_token){
	register int len = 0;
	register int count = 0;
	register BOOL had_char = FALSE, had_slash = TRUE;
	const wchar_t *pPtr = pPath;
	for (;;){
		switch (*pPath){
			case 0:
			case '/':
			case '\\':
				had_slash = TRUE;
				if (!had_char) break;
				if (count++ == token){
					*pLength = len;
					if (!copy_token) return (wchar_t*)pPtr;
					return StrnDupW(pPtr, len);
				}
				had_char = FALSE;
				break;
			default:
				had_char = TRUE;
				len++;
				if (!had_slash) break;
				had_slash = FALSE;
				pPtr = pPath;
				len = 1;
		}
		if (*pPath == 0) break;
		pPath++;
	}
	return NULL; //Should never reach this anyways, but makes a compiler warning without it
}

/*
File functions
*/


file fopen_(const char *_filename, const char *_mode){
	register unsigned long acc = 0, disp = 0, share = 0;
	file hFile;
	BOOL is_append = FALSE;
	if (*_mode && *(_mode+1) == '+'){
		switch (*_mode){
			case 'r':
				acc = GENERIC_READ | GENERIC_WRITE; disp = OPEN_EXISTING;
				break;
			case 'w':
				acc = GENERIC_READ | GENERIC_WRITE; disp = CREATE_ALWAYS;
				break;
			case 'a':
				acc = GENERIC_READ | GENERIC_WRITE; disp = OPEN_ALWAYS;
				is_append = TRUE;
				break;
			default:
				return INVALID_FILE;
		}
	} else if (*_mode){
		switch (*_mode){
			case 'r':
				acc = GENERIC_READ; disp = OPEN_EXISTING; share = FILE_SHARE_READ;
				break;
			case 'w':
				acc = GENERIC_WRITE; disp = CREATE_ALWAYS;
				break;
			case 'a':
				acc = GENERIC_WRITE; disp = OPEN_ALWAYS;
				is_append = TRUE;
				break;
			default:
				return INVALID_FILE;
		}
	} else return INVALID_FILE;
	if ((hFile = CreateFileA(_filename, acc, share, NULL, disp, 0, NULL)) == INVALID_FILE) return INVALID_FILE;
	if (is_append) SetFilePointer(hFile, 0, NULL, SEEK_END);
	return hFile;
}



file fopen_W(const wchar_t *_filename, const wchar_t *_mode){
	register unsigned long acc = 0, disp = 0;
	file hFile;
	BOOL is_append = FALSE;
	if (*_mode && *(_mode+1) == '+'){
		switch (*_mode){
			case 'r':
				acc = GENERIC_READ | GENERIC_WRITE; disp = OPEN_EXISTING;
				break;
			case 'w':
				acc = GENERIC_READ | GENERIC_WRITE; disp = CREATE_ALWAYS;
				break;
			case 'a':
				acc = GENERIC_READ | GENERIC_WRITE; disp = OPEN_ALWAYS;
				is_append = TRUE;
				break;
			default:
				return INVALID_FILE;
		}
	} else if (*_mode){
		switch (*_mode){
			case 'r':
				acc = GENERIC_READ; disp = OPEN_EXISTING;
				break;
			case 'w':
				acc = GENERIC_WRITE; disp = CREATE_ALWAYS;
				break;
			case 'a':
				acc = GENERIC_WRITE; disp = OPEN_ALWAYS;
				is_append = TRUE;
				break;
			default:
				return INVALID_FILE;
		}
	} else return INVALID_FILE;
	if ((hFile = CreateFileW(_filename, acc, 0, NULL, disp, 0, NULL)) == INVALID_FILE) return INVALID_FILE;
	if (is_append) SetFilePointer(hFile, 0, NULL, SEEK_END);
	return hFile;
}

void fclose_(file _file){
	CloseHandle(_file);
}

void fseteof_(file _file){
	SetEndOfFile(_file);
}

size_32 fread_(void *_buf, size_32 _ecount, file _file){
	DWORD r;
	ReadFile(_file, _buf, (DWORD)_ecount, &r, NULL);
	return (int)r;
}

size_32 fwrite_(void *_buf, size_32 _ecount, file _file){
	DWORD w;
	WriteFile(_file, _buf, (DWORD)_ecount, &w, NULL);
	return (size_32)w;
}


size_32 fseek_(file _file, LONG _offset, INT32 _origin){
	return ((SetFilePointer(_file, _offset, NULL, _origin) == INVALID_SET_FILE_POINTER) ? 1 : 0);
}

size_32 ftell_(file _file){
	register long pos;
	pos = SetFilePointer(_file, 0, NULL, SEEK_CUR);
	return (pos == INVALID_SET_FILE_POINTER) ? 0 : (size_32)pos;
}

size_32 fsize_(file _file){
	return GetFileSize(_file, NULL);
}

size_32 filecopy_(file _in, file _out, void *_cache, size_32 _cachesize, size_32 _size){
	register size_32 r, total;

	total = 0;
	while ((r = fread_(_cache, min(_cachesize, _size), _in))){
		if (r != fwrite_(_cache, r, _out)) break;
		_size -= r;
		total += r;
	}
	return total;
}

size_64 fseek_64(file _file, size_64 _offset, INT32 _origin){
	LARGE_INTEGER res;
	res.QuadPart = _offset;
	return (SetFilePointerEx(_file, res, &res, _origin) == TRUE) ? (size_64)res.QuadPart : (size_64)-1;
}

size_64 fsize_64(file _file){
	LARGE_INTEGER res;
	return (GetFileSizeEx(_file, &res) == TRUE) ? (size_64)res.QuadPart : (size_64)(-1);
}

size_64 ftell_64(file _file){
	LARGE_INTEGER res;
	res.QuadPart = 0;
	return (SetFilePointerEx(_file, res, &res, FILE_CURRENT) == TRUE) ? (size_64)res.QuadPart : (size_64)-1;
}

/*
Everything below this is taken from the CRT code coming with VC
Comments and some #ifdefs/etc were stripped
*/

void * __cdecl MemMove (void *dst, const void *src, size_t count){
	register void * ret = dst;

	if (dst <= src || (char *)dst >= ((char *)src + count)) {
		while (count--) {
			*(char *)dst = *(char *)src;
			dst = (char *)dst + 1;
			src = (char *)src + 1;
		}
	}
	else {
		dst = (char *)dst + count - 1;
		src = (char *)src + count - 1;
		while (count--) {
			*(char *)dst = *(char *)src;
			dst = (char *)dst - 1;
			src = (char *)src - 1;
		}
	}
	return ret;
}


int TokenizeA(char *pSrc, char **dest_array, int max_count, const char *extra_ws, const char *extra_quotation, BOOL extended_whitespace, BOOL translate_slashes, BOOL leave_quotation, BOOL NoDefaultWhiteSpace){
	char *pTemp, *pPtr, *pCur, *pQuote;
	register int token = 0, count = 0;
	int quotation_index = -1;
	register BOOL In_Quotes = FALSE;
	memset(dest_array, 0, sizeof(char*)*max_count);

	dest_array[0] = pTemp = pPtr = pCur = pSrc;

	while (1){
		switch (*pSrc){
			case 0:
				*pTemp = 0;
				if (count)
					dest_array[token++] = pCur;
				goto __exit_loop;
				break;
			case '\\':
				if (!translate_slashes) goto __process_char;
				pSrc++;
				switch (*(pSrc)){
					case '\"':
						count++; pSrc++;
						*(pTemp++) = '\"';
						continue;
					case '\'':
						count++; pSrc++;
						*(pTemp++) = '\'';
						continue;
					case 't':
						count++; pSrc++;
						*(pTemp++) = '\t';
						continue;
					case 'r':
						count++; pSrc++;
						*(pTemp++) = '\r';
						continue;
					case 'n':
						count++; pSrc++;
						*(pTemp++) = '\n';
						continue;
					case '\\':
						pPtr++;
					default:
						count++;
						*(pTemp++) = '\\';
						continue;
				}
				break;
			case '\"':
				if (quotation_index != -1) goto __process_char;
				if (!In_Quotes){
					In_Quotes = TRUE;
				} else
					In_Quotes = FALSE;
				if (leave_quotation)
					*(pTemp++) = *pSrc;
				pSrc++;
				continue;
			case '\r':
			case '\n':
				if (!extended_whitespace) goto __process_char;
			case '\t':
			case ' ':
				if (NoDefaultWhiteSpace) goto __process_char;
__process_ws:
				if (!In_Quotes) {
					pSrc++;
					if (count){
						*pTemp = 0;
						dest_array[token++] = pCur;
						/*
						Exit if max token count is reached
						*/
						if (token == max_count) goto __exit_loop;
						//pCur = pTemp = pSrc;
						*pTemp++;
						pCur = pTemp;
						count = 0;
					}
					continue;
				}
				count++;
				*(pTemp++) = *(pSrc++);
				continue;
__add_to_list:
				if (count){
					*pTemp = 0;
					dest_array[token++] = pCur;
					/*
					Exit if max token count is reached
					*/
					if (token == max_count) goto __exit_loop;
					//pCur = pTemp = pSrc;
					*pTemp++;
					pCur = pTemp;
					count = 0;
				}
				continue;

			default:
__process_char:
				if (extra_quotation && (pQuote = (char*)StrChr(extra_quotation, *pSrc))){
					if (!In_Quotes){
						quotation_index = PtrToLong(pQuote)-PtrToLong(extra_quotation)+1;
						In_Quotes = TRUE;
						*(pSrc++) = 0;
						goto __add_to_list;
					} else if (*(extra_quotation+quotation_index) == *pQuote){
						quotation_index = -1;
						In_Quotes = FALSE;
						*(pSrc++) = 0;
						goto __add_to_list;
					}
					continue;
				}
				if (extra_ws && StrChr(extra_ws, *pSrc)) goto __process_ws;
				count++;
				*(pTemp++) = *(pSrc++);
				continue;
				break;
		}
	}
__exit_loop:
	return token;
}

int TokenizeW(wchar_t *pSrc, wchar_t **dest_array, int max_count, const wchar_t *extra_ws, const wchar_t *extra_quotation, BOOL extended_whitespace, BOOL translate_slashes, BOOL leave_quotation, BOOL NoDefaultWhiteSpace){
	wchar_t *pTemp, *pPtr, *pCur, *pQuote;
	register int token = 0, count = 0;
	int quotation_index = -1;
	register BOOL In_Quotes = FALSE;
	memset(dest_array, 0, sizeof(wchar_t*)*max_count);

	dest_array[0] = pTemp = pPtr = pCur = pSrc;

	while (1){
		switch (*pSrc){
			case 0:
				*pTemp = 0;
				if (count)
					dest_array[token++] = pCur;
				goto __exit_loop;
				break;
			case '\\':
				if (!translate_slashes) goto __process_char;
				pSrc++;
				switch (*(pSrc)){
					case '\"':
						count++; pSrc++;
						*(pTemp++) = '\"';
						continue;
					case '\'':
						count++; pSrc++;
						*(pTemp++) = '\'';
						continue;
					case 't':
						count++; pSrc++;
						*(pTemp++) = '\t';
						continue;
					case 'r':
						count++; pSrc++;
						*(pTemp++) = '\r';
						continue;
					case 'n':
						count++; pSrc++;
						*(pTemp++) = '\n';
						continue;
					case '\\':
						pPtr++;
					default:
						count++;
						*(pTemp++) = '\\';
						continue;
				}
				break;
			case '\"':
				if (quotation_index != -1) goto __process_char;
				if (!In_Quotes){
					In_Quotes = TRUE;
				} else
					In_Quotes = FALSE;
				if (leave_quotation)
					*(pTemp++) = *pSrc;
				pSrc++;
				continue;
			case '\r':
			case '\n':
				if (!extended_whitespace) goto __process_char;
			case '\t':
			case ' ':
				if (NoDefaultWhiteSpace) goto __process_char;
__process_ws:
				if (!In_Quotes) {
					pSrc++;
					if (count){
						*pTemp = 0;
						dest_array[token++] = pCur;
						/*
						Exit if max token count is reached
						*/
						if (token == max_count) goto __exit_loop;
						//pCur = pTemp = pSrc;
						*pTemp++;
						pCur = pTemp;
						count = 0;
					}
					continue;
				}
				count++;
				*(pTemp++) = *(pSrc++);
				continue;
__add_to_list:
				if (count){
					*pTemp = 0;
					dest_array[token++] = pCur;
					/*
					Exit if max token count is reached
					*/
					if (token == max_count) goto __exit_loop;
					//pCur = pTemp = pSrc;
					*pTemp++;
					pCur = pTemp;
					count = 0;
				}
				continue;

			default:
__process_char:
				if (extra_quotation && (pQuote = (wchar_t*)StrChrW(extra_quotation, *pSrc))){
					if (!In_Quotes){
						quotation_index = PtrToLong(pQuote)-PtrToLong(extra_quotation)+1;
						In_Quotes = TRUE;
						*(pSrc++) = 0;
						goto __add_to_list;
					} else if (*(extra_quotation+quotation_index) == *pQuote){
						quotation_index = -1;
						In_Quotes = FALSE;
						*(pSrc++) = 0;
						goto __add_to_list;
					}
					continue;
				}
				if (extra_ws && StrChrW(extra_ws, *pSrc)) goto __process_ws;
				count++;
				*(pTemp++) = *(pSrc++);
				continue;
				break;
		}
	}
__exit_loop:
	return token;
}

void LogFormated(const wchar_t *fmt, ...){
	wchar_t lBuf[512];
	va_list va;
	va_start(va, fmt);
	_vsnwprintf(lBuf, 512, fmt, va);
	va_end(va);
	OutputDebugStringW(lBuf);
}

void LogError(){
	wchar_t lpMsgBuf[512];
	unsigned long error = GetLastError();
	FormatMessageW(
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		error,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_ENGLISH_US),
		(LPWSTR)lpMsgBuf,
		512, NULL );	
	LogFormated(L"ERROR %d: %s", error, lpMsgBuf);
}

void LogIID(const IID* pIID){
	LogFormated(L"%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
		pIID->Data1, pIID->Data2, pIID->Data3,
		pIID->Data4[0], pIID->Data4[1], pIID->Data4[2], pIID->Data4[3],
		pIID->Data4[4], pIID->Data4[5], pIID->Data4[6], pIID->Data4[7]
		);
}


char *Unicode_To_Multibyte(const wchar_t *unicode, int len){
	static char s_Multibyte[8192];
	int count = WideCharToMultiByte(CP_UTF8, 0, unicode, len, s_Multibyte, sizeof(s_Multibyte), NULL, NULL);
	if (!count)
		s_Multibyte[0] = 0;

	if (len > 0) count++;;
	s_Multibyte[count-1] = 0;

	return s_Multibyte;
}

wchar_t *Multibyte_To_Unicode(const char *multibyte, int len){
	static wchar_t s_Unicode[8192];
	int count = MultiByteToWideChar(CP_UTF8, 0, multibyte, len, s_Unicode, sizeof(s_Unicode)/sizeof(wchar_t));
	if (!count)
		s_Unicode[0] = 0;

	if (len > 0) count++;
	s_Unicode[count-1] = 0;
	
	return s_Unicode;
}


char *Unicode_To_Multibyte_Allocated(const wchar_t *unicode, int len){
	int count = WideCharToMultiByte(CP_UTF8, 0, unicode, -1, NULL, 0, NULL, NULL);
	char *pBuf;
	if (!count) return NULL;
	if (len > 0) count++;
	if (!(pBuf = (char*)g_pAllocator(count))) return NULL;
	if (!WideCharToMultiByte(CP_UTF8, 0, unicode, -1, (char*)pBuf, count, NULL, NULL)){
		g_pFreeer(pBuf);
		return NULL;
	}
	if (len > 0) pBuf[count-1] = 0;
	return pBuf;
}

wchar_t *Multibyte_To_Unicode_Allocated(const char *multibyte, int len){
	int count = MultiByteToWideChar(CP_UTF8, 0, multibyte, len, NULL, 0);
	wchar_t *pBuf;
	if (!count) return NULL;
	if (len > 0) count++;
	if (!(pBuf = (wchar_t*)g_pAllocator(count*sizeof(wchar_t)))) return NULL;
	if (!MultiByteToWideChar(CP_UTF8, 0, multibyte, len, pBuf, count)){
		g_pFreeer(pBuf);
		return NULL;
	}
	if (len > 0) pBuf[count-1] = 0;
	return pBuf;
}

bool MakeDirFromFilenameW(wchar_t* pBuf, const wchar_t *pBaseDir, const wchar_t *pFilename){
	int count, i;
	wchar_t *pTemp;
	int l;

	//Assume full path in pBuf if this is NULL
	if (pBaseDir){
		StrCpyW(pBuf, pBaseDir);
		StrCatW(pBuf, L"\\");
		StrCatW(pBuf, pFilename);
	}

	count = Get_Path_Token_CountW(pBuf);

	if (count < 1 || count > 128) return false;
	if (count == 1) return true;

	for (i = 0; i < count-1; i++){
		pTemp = Get_Path_TokenW(pBuf, &l, i, FALSE);
		*(pTemp+l) = 0;
		if (!(CreateDirectoryW(pBuf, NULL)  || GetLastError() == ERROR_ALREADY_EXISTS)) return false;

		*(pTemp+l) = '\\';
	}
	return true;
}

void InitializeStub(void *pReserved){
	void* s_pReserved;
	s_pReserved = pReserved;
}
