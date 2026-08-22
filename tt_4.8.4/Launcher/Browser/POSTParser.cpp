#include <windows.h>
#include "../Shared/StdLib.h"
#include "../Shared/Memory.h"
#include "../Shared/Lists.h"
#include "BrowserAPI.h"
#include "POSTParser.h"
#include "../Shared/FileStream.h"


void* GetPOSTData(BINDINFO *pBindInfo, size_32 *pSize, IStream** pStream){
	UINT size;
	void *pData;
	void *postdata;
	if (pBindInfo->dwBindVerb != BINDVERB_POST){
		if (pStream )*pStream = NULL;
		return NULL;
	}
	if (pBindInfo->stgmedData.tymed == TYMED_ISTREAM){
		if (pStream) *pStream = pBindInfo->stgmedData.pstm;
		return NULL;
	}
	if (pBindInfo->stgmedData.tymed != TYMED_HGLOBAL) return NULL;

	size = pBindInfo->cbstgmedData;

	if (!size) return NULL;

	if (!(pData = GlobalLock(pBindInfo->stgmedData.hGlobal))) return NULL;

	if ((postdata = g_pAllocator(size+1))){
		memcpy(postdata, pData, size);
		((char*)postdata)[size] = 0;
	}
	GlobalUnlock(pBindInfo->stgmedData.hGlobal);

	if (pSize) *pSize = size;

	return postdata;
}

bool IsURLEncoded(IInternetBindInfo* pBindInfo){
	LPOLESTR pType;
	ULONG fetched;
	bool res;

	pType = NULL;
	if (!pBindInfo) return false;

	pBindInfo->AddRef();
	if (pBindInfo->GetBindString(BINDSTRING_POST_DATA_MIME, &pType, 1, &fetched) != S_OK || !fetched || !pType){
		pBindInfo->Release();
		return false;
	}
	pBindInfo->Release();
	res = ((wcsnicmp(pType, L"application/x-www-form-urlencoded", sizeof("application/x-www-form-urlencoded")-1) == 0) || !(*pType)) ? true : false;	
	CoTaskMemFree(pType);
	return res;
}

bool IsBindInfoMultiPart(IInternetBindInfo* pBindInfo, wchar_t *pBoundary, int BufSize){
	LPOLESTR pType, temp, temp2;
	ULONG fetched;
	bool res;

	pType = NULL;
	if (!pBindInfo) return false;

	pBindInfo->AddRef();
	if (pBindInfo->GetBindString(BINDSTRING_POST_DATA_MIME, &pType, 1, &fetched) != S_OK || !fetched || !pType){
		pBindInfo->Release();
		return false;
	}
	pBindInfo->Release();
	temp = pType;

	res = (wcsnicmp(pType, L"multipart/form-data", sizeof("multipart/form-data")-1) == 0) ? true : false;

	if (pBoundary){
		while (temp){
			temp2 = (LPOLESTR)StrChrW(temp, ';');

			while (*temp == ' ')
				temp++;

			if (temp2){
				*temp2 = 0;
				temp2++;
			}

			if (wcsnicmp(temp, L"boundary=", sizeof("boundary=")-1) == 0){
				StrnCpyW(pBoundary, temp+sizeof("boundary=")-1, BufSize);
				/*if (temp = (LPOLESTR)StrChrW(temp, ' '))
					*temp = 0;//*/
			}
			temp = temp2;
		}
	}



	CoTaskMemFree(pType);

	return res;
		
}

int ProcessPOSTField(wchar_t* pField){
	wchar_t *pCur;
	int offset;
	offset = 0;
	int cur;

	cur = 0;
	pCur = pField;
	while (*pField){
		switch (*pField){
			case '=':
				offset = cur;
				*pCur = 0;
				pField++;
				break;
			case '%':
				*pCur = (HexDigitToNum(*(pField+1)) << 4) | HexDigitToNum(*(pField+2));
				
				pField += 3;
				break;
			case '+':
				*pCur = ' ';
				pField++;
				break;
			default:
				*pCur = *pField;
				pField++;
		}
		*pCur++;
		cur++;
	}
	*pCur = 0;
	return offset;
}

int ParsePOSTData(wchar_t *pSrc, int size, LPPOSTDATA results, wchar_t**FieldArray){
	int count, i, offset;

	count = TokenizeW(pSrc, FieldArray, size, L"&", NULL, FALSE, FALSE, FALSE, TRUE);

	for (i = 0; i < count; i++){
		offset = ProcessPOSTField(FieldArray[i]);
		results[i].m_pName = FieldArray[i];
		results[i].m_pValue = FieldArray[i]+offset+1;
		results[i].m_Size = -1;
	}

	return count;
}

IInternetSession* GetInternetSession(){
	IInternetSession* s_pInternetSession = NULL;
	if (!s_pInternetSession)
		if (CoInternetGetSession(0, &s_pInternetSession, 0) != S_OK)
			s_pInternetSession = NULL;

	return s_pInternetSession;
}

size_32 BindStreamSeek(IStream* pStream, size_32 offset){
	char lBuf[1024];
	ULARGE_INTEGER off;
	LARGE_INTEGER i;
	ULONG r;
	i.QuadPart = 0;
	pStream->Seek(i, STREAM_SEEK_SET, &off);

	while (pStream->Read(lBuf, min(offset, 1024), &r) == S_OK && r && offset)
		offset -= r;

	return offset;
}

const wchar_t UnicodeControlMapping[32] = {
    0x20ac, // 0x80
    0x0081, // 0x81
    0x201a, // 0x82
    0x0192, // 0x83
    0x201e, // 0x84
    0x2026, // 0x85
    0x2020, // 0x86
    0x2021, // 0x87
    0x02c6, // 0x88
    0x2030, // 0x89
    0x0160, // 0x8a
    0x2039, // 0x8b
    0x0152, // 0x8c <min>
    0x008d, // 0x8d
    0x017d, // 0x8e
    0x008f, // 0x8f
    0x0090, // 0x90
    0x2018, // 0x91
    0x2019, // 0x92
    0x201c, // 0x93
    0x201d, // 0x94
    0x2022, // 0x95
    0x2013, // 0x96
    0x2014, // 0x97
    0x02dc, // 0x98
    0x2122, // 0x99 <max>
    0x0161, // 0x9a
    0x203a, // 0x9b
    0x0153, // 0x9c
    0x009d, // 0x9d
    0x017e, // 0x9e
    0x0178  // 0x9f
};


#if 0

size_32 DecodeToStream(char* pSrc, size_32 size, IStream* pStream){
	LARGE_INTEGER l;
	ULARGE_INTEGER _size;
	DWORD xch;
	wchar_t ch;

	l.QuadPart = 0;
	pStream->Seek(l, STREAM_SEEK_CUR, &_size);
	size_32 sizeneeded;
	char* pCur, *pStr, *pEnd;
	char lBuf[512];
	StrCpy(lBuf, "0x");

	pStr = pCur = pSrc;

	sizeneeded = 0;
	while (*pStr){
		pCur = pStr;
		if (*pCur == '&'){
			if (*(pCur+1) == '#'){
				if (*(pCur+2) == 'x' || *(pCur+2) == 'X'){
					pEnd = pCur += 3;

					while (ISHEX(*pEnd))
						pEnd++;
					
					if (!*pEnd || *pEnd != ';' || PTR_DIFF(pEnd, pCur) <= 1) goto __text;



				} else{
					pEnd = pCur += 2;
					while (ISDIGIT(*pEnd))
						pEnd++;
					
					if (!*pEnd || *pEnd != ';' || PTR_DIFF(pEnd, pCur) <= 1) goto __text;
				}

			}

		}
__text:
		pStr++;
	}

	return NULL;

	return _size.LowPart;
}

#endif //0

CPOSTValues::CPOSTValues(){
	m_pData = NULL;
	m_Size = 0;
}

CPOSTValues::~CPOSTValues(){
	FreeData();
}


int CPOSTValues::GetValueCount(){
	return m_Values.Get_Count();

}

const wchar_t* CPOSTValues::GetValueName(int index){
	GenericSLNode<PostValue> *pNode;

	pNode = m_Values.Get_Head();
	while (pNode){
		if (index == 0)
			return pNode->m_pData->m_pName;
		index--;
		pNode = pNode->m_pNext;
	}
	return NULL;
}

int CPOSTValues::GetValueType(int index){
	GenericSLNode<PostValue> *pNode;

	pNode = m_Values.Get_Head();
	while (pNode){
		if (index == 0)
			return pNode->m_pData->m_Type;
		index--;
		pNode = pNode->m_pNext;
	}
	return POST_VALUE_TYPE_FAIL;
}

size_32 CPOSTValues::GetValueSize(int index){
	GenericSLNode<PostValue> *pNode;

	pNode = m_Values.Get_Head();
	while (pNode){
		if (index == 0)
			return pNode->m_pData->m_Size;
		index--;
		pNode = pNode->m_pNext;
	}
	return 0;
}

const void* CPOSTValues::GetValueBinary(int index){
	GenericSLNode<PostValue> *pNode;

	pNode = m_Values.Get_Head();
	while (pNode){
		if (index == 0)
			return (pNode->m_pData->m_Type != POST_VALUE_TYPE_STREAM) ? pNode->m_pData->m_pBinary : NULL;
		index--;
		pNode = pNode->m_pNext;
	}
	return NULL;
}

const wchar_t* CPOSTValues::GetValueString(int index){
	GenericSLNode<PostValue> *pNode;

	pNode = m_Values.Get_Head();
	while (pNode){
		if (index == 0)
			return (pNode->m_pData->m_Type != POST_VALUE_TYPE_STREAM) ? pNode->m_pData->m_pWString : NULL;
		index--;
		pNode = pNode->m_pNext;
	}
	return NULL;
}

IStream* CPOSTValues::GetStream(int index){
	GenericSLNode<PostValue> *pNode;

	pNode = m_Values.Get_Head();
	while (pNode){
		if (index == 0)
			return (pNode->m_pData->m_Type == POST_VALUE_TYPE_STREAM) ? pNode->m_pData->m_pStream : NULL;
		index--;
		pNode = pNode->m_pNext;
	}
	return NULL;
}



int CPOSTValues::GetValueTypeByName(const wchar_t* pName){
	return GetValueType(GetIndexByName(pName));
}

size_32 CPOSTValues::GetValueSizeByName(const wchar_t* pName){
	return GetValueSize(GetIndexByName(pName));
}

const void* CPOSTValues::GetValueBinaryByName(const wchar_t* pName){
	return GetValueBinary(GetIndexByName(pName));
}

const wchar_t* CPOSTValues::GetValueStringByName(const wchar_t* pName){
	return GetValueString(GetIndexByName(pName));
}

IStream* CPOSTValues::GetStreamByName(const wchar_t* pName){
	return GetStream(GetIndexByName(pName));
}


void CPOSTValues::Parse(void* pPOSTData, size_32 size, bool multipart, const wchar_t* pBoundary){
}

#define IsBoundary(p, boundary, blen) \
	(*(p) == '-' && *((p)+1) == '-' && \
	(strnicmp((char*)(p)+2, (boundary), blen) == 0) && \
	*(p+blen+2) == '\r' && *((p)+1+blen+2) == '\n')

#define IsBoundaryEnd(p, boundary, blen) (*(p) == '-' && *((p)+1) == '-' && \
	(strnicmp((char*)(p)+2, (boundary), blen) == 0) && \
	*(p+blen+2) == '-' && *((p)+1+blen+2) == '-')

void CPOSTValues::Parse(IStream *pStream, bool multipart, const wchar_t* pBoundary){
	char Boundary[516];
	int blen;

	char *pNewBoundary, *pName, *pFileName, *pEncoding, *pContentType;

	unsigned char* pCache, *pHeaderCache, *pEnd;
	size_32 soffset;
	STATSTG stat;
	size_32 end, begin;
	LARGE_INTEGER move, zmove;
	ULONG r;
	ULONG u;
	ULONG left;

	size_32 size;

	soffset = 0;
	pCache = NULL;
	pHeaderCache = NULL;
	pCache = (unsigned char*)g_pAllocator(PARSE_CACHE_SIZE);
	pHeaderCache = (unsigned char*)g_pAllocator(HEADER_CACHE_SIZE);
	if (!pCache || !pHeaderCache) goto __exit;


	blen = WideCharToMultiByte(CP_UTF8, 0, pBoundary, -1, Boundary+4, sizeof(Boundary)-4, NULL, NULL)-1;
	memcpy(Boundary, "\r\n--", 4);
	//STREAM_SEEK_CUR

	pStream->Stat(&stat, STATFLAG_NONAME);

	if (pStream->Read(pCache, blen+4, &r) != S_OK || (int)r < blen+4) goto __exit;
	soffset += r;
	
	if (!IsBoundary(pCache, Boundary+4, blen)) goto __exit;

	move.QuadPart = zmove.QuadPart = 0;

	begin = soffset;
	if (pStream->Read(pCache, PARSE_CACHE_SIZE-blen, &r) == S_OK && r){

		soffset += r;
		left = r;

__continue:
		u = 0;
		while (u < r-3){
			if (pCache[u] == '\r' && pCache[u+1] == '\n' && pCache[u+2] == '\r' && pCache[u+3] == '\n'){
				break;
			}
			u++;
		}

		if (u == r-3) goto __exit;
		memcpy(pHeaderCache, pCache, u+2);
		pHeaderCache[u] = 0;

		ParseHeader((char*)pHeaderCache, &pNewBoundary, &pName, &pFileName, &pContentType, &pEncoding);

		if (pNewBoundary){
			StrCpy(Boundary+4, pNewBoundary);
			blen = StrLen(pNewBoundary);
		}

		//Search for boundary

		memmove(pCache, pCache+u+4, r-u-4);

		left -= (u+4);

		end = (begin += u+4);
		//begin = end = soffset+u+4;
		while (!(pEnd = (unsigned char*)BinaryStr(pCache, Boundary, left))){
			memmove(pCache, pCache+r-u-4, blen);
			left -= blen;
			if (pStream->Read(pCache+blen, PARSE_CACHE_SIZE-blen-4, &r) != S_OK || !r)
				break;
			soffset += r;
			end += r;
			left = r+blen;
		}

		end += (size_32)(pEnd-pCache);

		size = end-begin;
		memmove(pCache, pEnd+blen+4, left-((pEnd-pCache)+blen+4));
		left -= (ULONG)(pEnd-pCache)+blen+4;

		BindStreamSeek(pStream, begin);
		ProcessBlock(pName, pFileName, pContentType, pEncoding, begin, end, size, pStream);
		BindStreamSeek(pStream, soffset);
		begin += (size_32)(pEnd-pCache)+blen+4+2;

		if (pCache[0] == '-' && pCache[1] == '-'){
			memmove(pCache, pCache+4, left-4);
			left -= 4;

			left = 0;
		} else {
			memmove(pCache, pCache+2, left-2);
			left -= 2;

			if (left == 0){
				if (pStream->Read(pCache, PARSE_CACHE_SIZE-blen-4, &r) == S_OK && r)
					left = r;

				soffset += r;
			}
		}

		


		//Do stuff here

		if (left) goto __continue;
		
	}

__exit:
	if (pCache)
		g_pFreeer(pCache);
	if (pHeaderCache)
		g_pFreeer(pHeaderCache);

}

wchar_t* DeEntitize(char* pBuf, int size){
	wchar_t*pRes, *pCur, *pStr, *pTemp, *pEnd;
	DWORD chr;
	pRes = Multibyte_To_Unicode_Allocated(pBuf, size);
	if (!pRes) return false;

	pStr = pCur = pRes;
	while (*pStr){
		switch (*pStr){
			case '&':
				if (*(++pStr) != '#'){
					*(pCur++) = *(pStr-1);
					continue;
				}
				pStr++;
				if (*pStr == 'x' || *pStr == 'X'){
					pStr++;
					pTemp = pStr;
					while (*pTemp && *pTemp != ';' && ISHEX(*pTemp))
						pTemp++;

					if (*pTemp != ';' || PTR_DIFF(pTemp, pStr) < 1){
						pStr -= 3;
						goto __default;
					}
					chr = wcstol(pStr, &pEnd, 16);	

				} else {
					pTemp = pStr;
					while (*pTemp && *pTemp != ';' && ISDIGIT(*pTemp))
						pTemp++;

					if (*pTemp != ';' || PTR_DIFF(pTemp, pStr) < 1){
						pStr -= 2;
						goto __default;
					}
					chr = wcstol(pStr, &pEnd, 10);					
				}

				if (chr < 0x10000){					
					*(pCur++) = (wchar_t)chr;
				} else {
					*(pCur++) = (wchar_t)(0xd800 + (chr >> 10));
					*(pCur++) = (wchar_t)(0xdc00 + (chr & 0x3ff));
				}
				pStr += PTR_DIFF(pTemp, pStr)+1;
				continue;

			default:
__default:
				*(pCur++) = *(pStr++);
				if (size != -1) size--;
		}
	}
	*pCur = 0;
	pStr = StrDupW(pRes);
	g_pFreeer(pRes);
	return pStr;
}

void CPOSTValues::ParseURLEncoded(char* pPOSTData, size_32 size, IStream* pStream){
	char* pCur, *pField, *pStr;
	int offset;
	int cur;
	lpPostValue pv;
	char c;
	c = 0;
	if (pStream){
	} else {
		pField = pPOSTData;
		while(true) {		
			if (*pPOSTData == '&' || !*pPOSTData){
				
				cur = 0;
				offset = -1;
				pStr = pCur = pField;
				c = *pPOSTData;
				*pPOSTData = 0;
				while (*pField){
					switch (*pField){
						case '=':
							offset = cur;
							*pCur = 0;
							pField++;
							break;
						case '%':
							*pCur = (HexDigitToNum(*(pField+1)) << 4) | HexDigitToNum(*(pField+2));
							
							pField += 3;
							break;
						case '+':
							*pCur = ' ';
							pField++;
							break;
						default:
							*pCur = *pField;
							pField++;
					}
					*pCur++;
					cur++;
				}
				*pCur = 0;
				if ((pv = new PostValue)){
					pv->m_pName = DeEntitize(pStr, -1);
					pv->m_pWString = DeEntitize(pStr+offset+1, -1);
					pv->m_Type = POST_VALUE_TYPE_STRING;
					pv->m_Size = -1;
					m_Values.Add_Tail(pv);
				}
				*pPOSTData = c;
				if (!*pPOSTData) break;
				pField = ++pPOSTData;
				
			}
			pPOSTData++;
		}
	}
}

void CPOSTValues::FreeData(){
}

void CPOSTValues::ParseFromBindInfo(IInternetBindInfo* pBindInfo){
	BINDINFO binfo;
	IStream* pStream;
	size_32 pdsize;
	void* pPOSTData;
	DWORD bindflags;

	wchar_t boundary[512];
	memset(&binfo, 0, sizeof(BINDINFO));
	binfo.cbSize = sizeof(BINDINFO);

	pStream = NULL;

	pBindInfo->AddRef();
	if (pBindInfo->GetBindInfo(&bindflags, &binfo) == S_OK){
		pPOSTData = GetPOSTData(&binfo, &pdsize, &pStream);


		if ((pPOSTData || pStream) && IsURLEncoded(pBindInfo)){
			ParseURLEncoded((char*)pPOSTData, pdsize, pStream);
			if (pPOSTData) g_pFreeer(pPOSTData);
		} else if ((pPOSTData || pStream) && IsBindInfoMultiPart(pBindInfo, boundary, sizeof(boundary)/sizeof(wchar_t))){
			pStream ? Parse(pStream, true, boundary) : Parse(pPOSTData, pdsize, true, boundary);
			if (pPOSTData) g_pFreeer(pPOSTData);
		}
	}
	pBindInfo->Release();
}

wchar_t* CPOSTValues::ParseText(char *pSrc, size_32 size){
	return DeEntitize(pSrc, size);
}

void CPOSTValues::ParseStreamText(IStream* pStreamIn, IStream* pStreamOut, size_32 size){
}

lpPostValue CPOSTValues::GetValueByName(const wchar_t*pName){
	GenericSLNode<PostValue> *pNode;

	pNode = m_Values.Get_Head();
	while (pNode){
		if (wcsicmp(pName, pNode->m_pData->m_pName) == 0) return pNode->m_pData;
		pNode = pNode->m_pNext;
	}
	return NULL;
}

int CPOSTValues::GetIndexByName(const wchar_t*pName){
	GenericSLNode<PostValue> *pNode;
	int i;

	pNode = m_Values.Get_Head();
	i = 0;
	while (pNode){
		if (wcsicmp(pName, pNode->m_pData->m_pName) == 0) return i;
		i++;
		pNode = pNode->m_pNext;
	}
	return -1;
}

void CPOSTValues::ParseHeader(char *pHeaderCache, char** ppNewBoundary, char** ppName, char** ppFileName, char** ppContentType, char** ppTransferEncoding){
	char *temp;
	char* Array[8];
	char* Array2[8];
	int c0, c1, i;
	if (ppNewBoundary) *ppNewBoundary = NULL;
	if (ppName) *ppName = NULL;
	if (ppFileName) *ppFileName = NULL;
	if (ppContentType) *ppContentType = NULL;
	if (ppTransferEncoding) *ppTransferEncoding = NULL;
	while (pHeaderCache && *pHeaderCache){
		temp = (char*)StrStr(pHeaderCache, "\r\n");
		if (temp){
			*temp = 0;
			temp += 2;
		}
		c0 = TokenizeA(pHeaderCache, Array, 8, ";", NULL, FALSE, FALSE, TRUE, TRUE);
		for (i = 0; i < c0; i++){
			c1 = TokenizeA(Array[i], Array2, 8, "=:", NULL, FALSE, FALSE, FALSE, FALSE);
			if (c1 < 2) continue;
			if (stricmp(Array2[0], "name") == 0){
				if (ppName) *ppName = Array2[1];
			} else if(stricmp(Array2[0], "filename") == 0){
				if (ppFileName) *ppFileName = Array2[1];
			} else if(stricmp(Array2[0], "boundary") == 0){
				if (ppNewBoundary) *ppNewBoundary = Array2[1];
			} else if(stricmp(Array2[0], "Content-Type") == 0){
				if (ppContentType) *ppContentType = Array2[1];
			} else if(stricmp(Array2[0], "Content-Transfer-Encoding") == 0){
				if (ppTransferEncoding) *ppTransferEncoding = Array2[1];
			}
		}
		pHeaderCache = temp;
	}
}

void CPOSTValues::ProcessBlock(char* pName, char* pFileName, char* pContentType, char* pTransferEncoding, size_32 begin, size_32 end, size_32 size, IStream *pStream){
	CFileStream *pFileStream;
	STATSTG stat;

	unsigned char lBuf[1024];
	ULONG r, o;
	LARGE_INTEGER l;
	l.QuadPart = 0;
	BindStreamSeek(pStream, begin);
	lpPostValue val;
	char* pBuf;
	//pStream->Read(lBuf, min(size, 1024), &r);

	val = new PostValue;

	pFileStream = NULL;

	if (!val) return;

	val->m_pName = DeEntitize(pName, -1);
	val->m_pBinary = NULL;
	val->m_Size = 0;

	if (size < VALUE_SIZE_LIMIT){
		o = 0;

		if (!pContentType || (pContentType && stricmp(pContentType, "text/plain") == 0)){
			val->m_Type = POST_VALUE_TYPE_STRING;
			pBuf = (char*)g_pAllocator(size+1);
			if (pBuf){
				pStream->Read(pBuf, size, &r);

				pBuf[size] = 0;
				val->m_pWString = ParseText(pBuf, size);
				g_pFreeer(pBuf);
			} else
				goto __clean;
		} else {
			val->m_Type = POST_VALUE_TYPE_BINARY;
			val->m_Size = size;
			if ((val->m_pBinary = g_pAllocator(size))){
				while (size){
					pStream->Read(lBuf, min(size, 1024), &r);
					size -= r;
					memcpy(((unsigned char*)val->m_pBinary)+o, lBuf, r);

					o += r;
				}

			}
		}
	} else {
		pFileStream = new CFileStream();
		pFileStream->AddRef();
		pFileStream->Open(NULL, true, false);
		if (!pContentType || (pContentType && stricmp(pContentType, "text/plain") == 0))
			ParseStreamText(pStream, pFileStream, size);
		else {
			while (size){
				pStream->Read(lBuf, min(size, 1024), &r);
				size -= r;
				pFileStream->Write(lBuf, r, &r);
			}
		}
		pFileStream->Seek(l, SEEK_SET, NULL);

		val->m_Type = POST_VALUE_TYPE_STREAM;
		val->m_pStream = pFileStream;
		pFileStream->Stat(&stat, STATFLAG_DEFAULT);
		
		val->m_Size = stat.cbSize.LowPart;

		pFileStream->Release();
	}
	m_Values.Add_Tail(val);
	return;
__clean:
	if (val){
		if (val->m_pName)
			g_pFreeer(val->m_pName);
		delete val;
	}

	if (pBuf) g_pFreeer(pBuf);
}
