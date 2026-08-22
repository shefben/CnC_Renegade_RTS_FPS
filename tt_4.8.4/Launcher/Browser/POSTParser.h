#ifndef _POSTPARSER_H_
#define _POSTPARSER_H_

#include <windows.h>
#include "../Shared/StdLib.h"
#include "../Shared/Memory.h"
#include "../Shared/Lists.h"
#include "BrowserAPI.h"


#define VALUE_SIZE_LIMIT	(1024*1024*4) //Max 4 MB in mem

#define PARSE_CACHE_SIZE	(1024*1024)
#define HEADER_CACHE_SIZE	(32*1024)

#define HexDigitToNum(chr) \
	((chr) >= '0' && (chr) <= '9' ? (chr)-'0' : \
	((chr) >= 'a' && (chr) <= 'f' ? (chr)-'a'+10 : \
	((chr) >= 'A' && (chr) <= 'F' ? (chr)-'A'+10 : 0 \
	)))

typedef struct _PostValue : public AutoPoolClass<_PostValue, 32> {
	wchar_t*	m_pName;
	int			m_Type;
	size_32		m_Size;
	union {
		unsigned char*	m_pString;
		wchar_t*		m_pWString;
		void*			m_pBinary;
		IStream*		m_pStream;
	};
} PostValue, *lpPostValue;

class CPOSTValues : public IPOSTValues {
private:
	size_32				m_Size;
	unsigned char*		m_pData;
	int					m_Type;
	SList<PostValue>	m_Values;
public:
	CPOSTValues();
	virtual ~CPOSTValues();

	virtual int GetValueCount();
	virtual const wchar_t* GetValueName(int index);
	virtual int GetValueType(int index);
	virtual size_32 GetValueSize(int index);
	virtual const void* GetValueBinary(int index);
	virtual const wchar_t* GetValueString(int index);
	virtual IStream* GetStream(int index);

	virtual int GetValueTypeByName(const wchar_t* pName);
	virtual size_32 GetValueSizeByName(const wchar_t* pName);
	virtual const void* GetValueBinaryByName(const wchar_t* pName);
	virtual const wchar_t* GetValueStringByName(const wchar_t* pName);
	virtual IStream* GetStreamByName(const wchar_t* pName);

public:
	void Parse(void* pPOSTData, size_32 size, bool multipart, const wchar_t* pBoundary);
	void Parse(IStream *pStream, bool multipart, const wchar_t* pBoundary);
	void ParseURLEncoded(char* pPOSTData, size_32 size, IStream* pStream);
	void FreeData();
	void ParseFromBindInfo(IInternetBindInfo* pBindInfo);
private:
	lpPostValue GetValueByName(const wchar_t*pName);
	int GetIndexByName(const wchar_t*pName);
	void ParseHeader(char *pHeaderCache, char** ppNewBoundary, char** ppName, char** ppFileName, char** ppContentType, char** ppTransferEncoding);
	wchar_t* ParseText(char *pSrc, size_32 size);
	void ParseStreamText(IStream* pStreamIn, IStream* pStreamOut, size_32 size);
	void ProcessBlock(char* pName, char* pFileName, char* pContentType, char* pTransferEncoding, size_32 begin, size_32 end, size_32 size, IStream *pStream);
};




#endif //!_POSTPARSER_H_
