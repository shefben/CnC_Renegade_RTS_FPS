#ifndef _HTMLWRITE_H_
#define _HTMLWRITE_H_
#include <windows.h>
#include "../Shared/StdLib.h"
#include "../Shared/Memory.h"
#include "../Shared/MemoryStream.h"
#include "../Shared/Lists.h"


typedef struct _HTMLAttribute : public AutoPoolClass<_HTMLAttribute, 16> {
	wchar_t*	m_pName;
	wchar_t*	m_pString;
} HTMLAttribute, *lpHTMLAttribute;

typedef struct _HTMLEntity {
	wchar_t			m_Char;
	const wchar_t*	m_pName;
	const char*		m_pEntitized;
} HTMLEntity, *lpHTMLEntity;

class CHTMLWriter {
private:
	IStream*				m_pStream;
	SList<HTMLAttribute>	m_Attributes;
	SList<wchar_t>			m_Tags;
	wchar_t*				m_pBufferW;
	char*					m_pBufferA;

public:
	CHTMLWriter(IStream* pStream);
	~CHTMLWriter();
	bool AddAttribute(const wchar_t* pName, const wchar_t* pString);
	bool OpenTag(const wchar_t* pTag);
	bool WriteClosedTag(const wchar_t* pTag);
	bool CloseTag();
	bool CloseAllTags();
	
	bool WriteRaw(const wchar_t* pString);
	bool WriteRawFmt(const wchar_t *pFmt, ...);
	bool WriteString(const wchar_t* pString);
	bool WriteStringFmt(const wchar_t *pFmt, ...);
	bool Finalize();

};

#endif //!_HTMLWRITE_H_
