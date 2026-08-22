#include <windows.h>
#include "../Shared/StdLib.h"
#include "../Shared/Memory.h"
#include "../Shared/MemoryStream.h"
#include <stdio.h>
#include <stdlib.h>
#include "HTMLWrite.h"
#include "MultiCache.h"

#define INTERNAL_BUFFER_SIZEW	(64*1024)
#define INTERNAL_BUFFER_SIZEA	(256*1024)

//Surrogate ranges
#define WCH_UTF16_HIGH_FIRST    WCHAR(0xd800)
#define WCH_UTF16_HIGH_LAST     WCHAR(0xdbff)
#define WCH_UTF16_LOW_FIRST     WCHAR(0xdc00)
#define WCH_UTF16_LOW_LAST      WCHAR(0xdfff)


HTMLEntity	g_HTMLEntities[] = {
	{160,  L"nbsp",     "&nbsp;"},
	{161,  L"iexcl",    "&iexcl;"},
	{162,  L"cent",     "&cent;"},
	{163,  L"pound",    "&pound;"},
	{164,  L"curren",   "&curren;"},
	{165,  L"yen",      "&yen;"},
	{166,  L"brvbar",   "&brvbar;"},
	{167,  L"sect",     "&sect;"},
	{168,  L"uml",      "&uml;"},
	{169,  L"copy",     "&copy;"},
	{170,  L"ordf",     "&ordf;"},
	{171,  L"laquo",    "&laquo;"},
	{172,  L"not",      "&not;"},
	{173,  L"shy",      "&shy;"},
	{174,  L"reg",      "&reg;"},
	{175,  L"macr",     "&macr;"},
	{176,  L"deg",      "&deg;"},
	{177,  L"plusmn",   "&plusmn;"},
	{178,  L"sup2",     "&sup2;"},
	{179,  L"sup3",     "&sup3;"},
	{180,  L"acute",    "&acute;"},
	{181,  L"micro",    "&micro;"},
	{182,  L"para",     "&para;"},
	{183,  L"middot",   "&middot;"},
	{184,  L"cedil",    "&cedil;"},
	{185,  L"sup1",     "&sup1;"},
	{186,  L"ordm",     "&ordm;"},
	{187,  L"raquo",    "&raquo;"},
	{188,  L"frac14",   "&frac14;"},
	{189,  L"frac12",   "&frac12;"},
	{190,  L"frac34",   "&frac34;"},
	{191,  L"iquest",   "&iquest;"},
	{192,  L"Agrave",   "&Agrave;"},
	{193,  L"Aacute",   "&Aacute;"},
	{194,  L"Acirc",    "&Acirc;"},
	{195,  L"Atilde",   "&Atilde;"},
	{196,  L"Auml",     "&Auml;"},
	{197,  L"Aring",    "&Aring;"},
	{198,  L"AElig",    "&AElig;"},
	{199,  L"Ccedil",   "&Ccedil;"},
	{200,  L"Egrave",   "&Egrave;"},
	{201,  L"Eacute",   "&Eacute;"},
	{202,  L"Ecirc",    "&Ecirc;"},
	{203,  L"Euml",     "&Euml;"},
	{204,  L"Igrave",   "&Igrave;"},
	{205,  L"Iacute",   "&Iacute;"},
	{206,  L"Icirc",    "&Icirc;"},
	{207,  L"Iuml",     "&Iuml;"},
	{208,  L"ETH",      "&ETH;"},
	{209,  L"Ntilde",   "&Ntilde;"},
	{210,  L"Ograve",   "&Ograve;"},
	{211,  L"Oacute",   "&Oacute;"},
	{212,  L"Ocirc",    "&Ocirc;"},
	{213,  L"Otilde",   "&Otilde;"},
	{214,  L"Ouml",     "&Ouml;"},
	{215,  L"times",    "&times;"},
	{216,  L"Oslash",   "&Oslash;"},
	{217,  L"Ugrave",   "&Ugrave;"},
	{218,  L"Uacute",   "&Uacute;"},
	{219,  L"Ucirc",    "&Ucirc;"},
	{220,  L"Uuml",     "&Uuml;"},
	{221,  L"Yacute",   "&Yacute;"},
	{222,  L"THORN",    "&THORN;"},
	{223,  L"szlig",    "&szlig;"},
	{224,  L"agrave",   "&agrave;"},
	{225,  L"aacute",   "&aacute;"},
	{226,  L"acirc",    "&acirc;"},
	{227,  L"atilde",   "&atilde;"},
	{228,  L"auml",     "&auml;"},
	{229,  L"aring",    "&aring;"},
	{230,  L"aelig",    "&aelig;"},
	{231,  L"ccedil",   "&ccedil;"},
	{232,  L"egrave",   "&egrave;"},
	{233,  L"eacute",   "&eacute;"},
	{234,  L"ecirc",    "&ecirc;"},
	{235,  L"euml",     "&euml;"},
	{236,  L"igrave",   "&igrave;"},
	{237,  L"iacute",   "&iacute;"},
	{238,  L"icirc",    "&icirc;"},
	{239,  L"iuml",     "&iuml;"},
	{240,  L"eth",      "&eth;"},
	{241,  L"ntilde",   "&ntilde;"},
	{242,  L"ograve",   "&ograve;"},
	{243,  L"oacute",   "&oacute;"},
	{244,  L"ocirc",    "&ocirc;"},
	{245,  L"otilde",   "&otilde;"},
	{246,  L"ouml",     "&ouml;"},
	{247,  L"divide",   "&divide;"},
	{248,  L"oslash",   "&oslash;"},
	{249,  L"ugrave",   "&ugrave;"},
	{250,  L"uacute",   "&uacute;"},
	{251,  L"ucirc",    "&ucirc;"},
	{252,  L"uuml",     "&uuml;"},
	{253,  L"yacute",   "&yacute;"},
	{254,  L"thorn",    "&thorn;"},
	{255,  L"yuml",     "&yuml;"},
	{338,  L"OElig",    "&OElig;"},
	{339,  L"oelig",    "&oelig;"},
	{34,   L"quot",     "&quot;"},
	{352,  L"Scaron",   "&Scaron;"},
	{353,  L"scaron",   "&scaron;"},
	{376,  L"Yuml",     "&Yuml;"},
	{38,   L"amp",      "&amp;"},
	{402,  L"fnof",     "&fnof;"},
	{60,   L"lt",       "&lt;"},
	{62,   L"gt",       "&gt;"},
	{710,  L"circ",     "&circ;"},
	{732,  L"tilde",    "&tilde;"},
	{8194, L"ensp",     "&ensp;"},
	{8195, L"emsp",     "&emsp;"},
	{8201, L"thinsp",   "&thinsp;"},
	{8204, L"zwnj",     "&zwnj;"},
	{8205, L"zwj",      "&zwj;"},
	{8206, L"lrm",      "&lrm;"},
	{8207, L"rlm",      "&rlm;"},
	{8211, L"ndash",    "&ndash;"},
	{8212, L"mdash",    "&mdash;"},
	{8216, L"lsquo",    "&lsquo;"},
	{8217, L"rsquo",    "&rsquo;"},
	{8218, L"sbquo",    "&sbquo;"},
	{8220, L"ldquo",    "&ldquo;"},
	{8221, L"rdquo",    "&rdquo;"},
	{8222, L"bdquo",    "&bdquo;"},
	{8224, L"dagger",   "&dagger;"},
	{8225, L"Dagger",   "&Dagger;"},
	{8226, L"bull",     "&bull;"},
	{8230, L"hellip",   "&hellip;"},
	{8240, L"permil",   "&permil;"},
	{8242, L"prime",    "&prime;"},
	{8243, L"Prime",    "&Prime;"},
	{8249, L"lsaquo",   "&lsaquo;"},
	{8250, L"rsaquo",   "&rsaquo;"},
	{8254, L"oline",    "&oline;"},
	{8260, L"frasl",    "&frasl;"},
	{8364, L"euro",     "&euro;"},
	{8465, L"image",    "&image;"},
	{8472, L"weierp",   "&weierp;"},
	{8476, L"real",     "&real;"},
	{8482, L"trade",    "&trade;"},
	{8501, L"alefsym",  "&alefsym;"},
	{8592, L"larr",     "&larr;"},
	{8593, L"uarr",     "&uarr;"},
	{8594, L"rarr",     "&rarr;"},
	{8595, L"darr",     "&darr;"},
	{8596, L"harr",     "&harr;"},
	{8629, L"crarr",    "&crarr;"},
	{8656, L"lArr",     "&lArr;"},
	{8657, L"uArr",     "&uArr;"},
	{8658, L"rArr",     "&rArr;"},
	{8659, L"dArr",     "&dArr;"},
	{8660, L"hArr",     "&hArr;"},
	{8704, L"forall",   "&forall;"},
	{8706, L"part",     "&part;"},
	{8707, L"exist",    "&exist;"},
	{8709, L"empty",    "&empty;"},
	{8711, L"nabla",    "&nabla;"},
	{8712, L"isin",     "&isin;"},
	{8713, L"notin",    "&notin;"},
	{8715, L"ni",       "&ni;"},
	{8719, L"prod",     "&prod;"},
	{8721, L"sum",      "&sum;"},
	{8722, L"minus",    "&minus;"},
	{8727, L"lowast",   "&lowast;"},
	{8730, L"radic",    "&radic;"},
	{8733, L"prop",     "&prop;"},
	{8734, L"infin",    "&infin;"},
	{8736, L"ang",      "&ang;"},
	{8743, L"and",      "&and;"},
	{8744, L"or",       "&or;"},
	{8745, L"cap",      "&cap;"},
	{8746, L"cup",      "&cup;"},
	{8747, L"int",      "&int;"},
	{8756, L"there4",   "&there4;"},
	{8764, L"sim",      "&sim;"},
	{8773, L"cong",     "&cong;"},
	{8776, L"asymp",    "&asymp;"},
	{8800, L"ne",       "&ne;"},
	{8801, L"equiv",    "&equiv;"},
	{8804, L"le",       "&le;"},
	{8805, L"ge",       "&ge;"},
	{8834, L"sub",      "&sub;"},
	{8835, L"sup",      "&sup;"},
	{8836, L"nsub",     "&nsub;"},
	{8838, L"sube",     "&sube;"},
	{8839, L"supe",     "&supe;"},
	{8853, L"oplus",    "&oplus;"},
	{8855, L"otimes",   "&otimes;"},
	{8869, L"perp",     "&perp;"},
	{8901, L"sdot",     "&sdot;"},
	{8968, L"lceil",    "&lceil;"},
	{8969, L"rceil",    "&rceil;"},
	{8970, L"lfloor",   "&lfloor;"},
	{8971, L"rfloor",   "&rfloor;"},
	{9001, L"lang",     "&lang;"},
	{9002, L"rang",     "&rang;"},
	{913,  L"Alpha",    "&Alpha;"},
	{914,  L"Beta",     "&Beta;"},
	{915,  L"Gamma",    "&Gamma;"},
	{916,  L"Delta",    "&Delta;"},
	{917,  L"Epsilon",  "&Epsilon;"},
	{918,  L"Zeta",     "&Zeta;"},
	{919,  L"Eta",      "&Eta;"},
	{920,  L"Theta",    "&Theta;"},
	{921,  L"Iota",     "&Iota;"},
	{922,  L"Kappa",    "&Kappa;"},
	{923,  L"Lambda",   "&Lambda;"},
	{924,  L"Mu",       "&Mu;"},
	{925,  L"Nu",       "&Nu;"},
	{926,  L"Xi",       "&Xi;"},
	{927,  L"Omicron",  "&Omicron;"},
	{928,  L"Pi",       "&Pi;"},
	{929,  L"Rho",      "&Rho;"},
	{931,  L"Sigma",    "&Sigma;"},
	{932,  L"Tau",      "&Tau;"},
	{933,  L"Upsilon",  "&Upsilon;"},
	{934,  L"Phi",      "&Phi;"},
	{935,  L"Chi",      "&Chi;"},
	{936,  L"Psi",      "&Psi;"},
	{937,  L"Omega",    "&Omega;"},
	{945,  L"alpha",    "&alpha;"},
	{946,  L"beta",     "&beta;"},
	{947,  L"gamma",    "&gamma;"},
	{948,  L"delta",    "&delta;"},
	{949,  L"epsilon",  "&epsilon;"},
	{950,  L"zeta",     "&zeta;"},
	{951,  L"eta",      "&eta;"},
	{952,  L"theta",    "&theta;"},
	{953,  L"iota",     "&iota;"},
	{954,  L"kappa",    "&kappa;"},
	{955,  L"lambda",   "&lambda;"},
	{956,  L"mu",       "&mu;"},
	{957,  L"nu",       "&nu;"},
	{958,  L"xi",       "&xi;"},
	{959,  L"omicron",  "&omicron;"},
	{960,  L"pi",       "&pi;"},
	{961,  L"rho",      "&rho;"},
	{962,  L"sigmaf",   "&sigmaf;"},
	{963,  L"sigma",    "&sigma;"},
	{964,  L"tau",      "&tau;"},
	{965,  L"upsilon",  "&upsilon;"},
	{966,  L"phi",      "&phi;"},
	{967,  L"chi",      "&chi;"},
	{9674, L"loz",      "&loz;"},
	{968,  L"psi",      "&psi;"},
	{969,  L"omega",    "&omega;"},
	{977,  L"thetasym", "&thetasym;"},
	{978,  L"upsih",    "&upsih;"},
	{982,  L"piv",      "&piv;"},
	{9824, L"spades",   "&spades;"},
	{9827, L"clubs",    "&clubs;"},
	{9829, L"hearts",   "&hearts;"},
	{9830, L"diams",    "&diams;"},
	{0,    NULL,        NULL},
};

lpHTMLEntity FindEntity(wchar_t ch){
	int i;
	for (i = 0; g_HTMLEntities[i].m_pName; i++)
		if (g_HTMLEntities[i].m_Char == ch) return &g_HTMLEntities[i];
	return NULL;
}

SizedPoolClass<256, 32>	g_HTMLStrPool;
CMultiCache				g_HTMLMultiCache;

wchar_t *HTMLStrPoolStrDupW(const wchar_t *pStr){
	int len;
	wchar_t *_pStr;
	len = StrLenW(pStr);

	if (len > 128-1) return NULL;

	_pStr = (wchar_t*)g_HTMLStrPool.Allocate_Object_Memory();

	StrCpyW(_pStr, pStr);
	
	return _pStr;
}

void HTMLStrPoolFree(wchar_t* pStr){
	g_HTMLStrPool.Free_Object_Memory(pStr);
}


//int 

bool IsValidAttributeName(const wchar_t* pName){
	while (*pName){

		if (*pName < 32 || *pName > 127) return false;

		switch (*pName){

			case ' ':
			case '<':
			case '>':
			case '&':
			case '/':
			case '\"':
			case '?':
				return false;
			default:
				break;
		}

		pName++;
	}
	return true;
}

CHTMLWriter::CHTMLWriter(IStream* pStream){
	(m_pStream = pStream)->AddRef();
	m_pBufferW = (wchar_t*)g_pAllocator(INTERNAL_BUFFER_SIZEW*sizeof(wchar_t));
	m_pBufferA = (char*)g_pAllocator(INTERNAL_BUFFER_SIZEA);
}

CHTMLWriter::~CHTMLWriter(){
	if (m_pBufferW) g_pFreeer(m_pBufferW);
	if (m_pBufferA) g_pFreeer(m_pBufferA);
	m_pStream->Release();
}

bool CHTMLWriter::AddAttribute(const wchar_t* pName, const wchar_t* pString){
	lpHTMLAttribute pAttr;
	if (!IsValidAttributeName(pName)) return false;
	if (!(pAttr = new HTMLAttribute)) return false;

	pAttr->m_pName = HTMLStrPoolStrDupW(pName);
	pAttr->m_pString = HTMLStrPoolStrDupW(pString);

	m_Attributes.Add_Tail(pAttr);

	return true;
}

bool CHTMLWriter::OpenTag(const wchar_t* pTag){
	GenericSLNode<HTMLAttribute>* pNode;
	if (!IsValidAttributeName(pTag)) return false;

	pNode = m_Attributes.Get_Head();
	if (pNode){
		WriteRawFmt(L"<%s", pTag);
		while (pNode){
			WriteRawFmt(L" %s=\"", pNode->m_pData->m_pName);
			WriteString(pNode->m_pData->m_pString);
			WriteRaw(L"\"");

			HTMLStrPoolFree(pNode->m_pData->m_pName);
			HTMLStrPoolFree(pNode->m_pData->m_pString);

			delete pNode->m_pData;

			pNode = pNode->m_pNext;
		}
		m_Attributes.Remove_All();

		WriteRaw(L" >");
	} else
		WriteRawFmt(L"<%s>", pTag);

	m_Tags.Add_Head(HTMLStrPoolStrDupW(pTag));

	return true;
}

bool CHTMLWriter::WriteClosedTag(const wchar_t* pTag){
	GenericSLNode<HTMLAttribute>* pNode;
	if (!IsValidAttributeName(pTag)) return false;

	pNode = m_Attributes.Get_Head();
	if (pNode){
		WriteRawFmt(L"<%s", pTag);
		while (pNode){
			WriteRawFmt(L" %s=\"", pNode->m_pData->m_pName);
			WriteString(pNode->m_pData->m_pString);
			WriteRaw(L"\"");

			HTMLStrPoolFree(pNode->m_pData->m_pName);
			HTMLStrPoolFree(pNode->m_pData->m_pString);

			delete pNode->m_pData;

			pNode = pNode->m_pNext;
		}
		m_Attributes.Remove_All();

		WriteRaw(L" />");
	} else
		WriteRawFmt(L"<%s/>", pTag);

	return true;
}

bool CHTMLWriter::CloseTag(){
	GenericSLNode<wchar_t>* pNode;
	if (!(pNode = m_Tags.Get_Head())) return false;
	WriteRawFmt(L"</%s>", pNode->m_pData);

	HTMLStrPoolFree(pNode->m_pData);
	m_Tags.Remove(pNode->m_pData);
	return true;
}

bool CHTMLWriter::CloseAllTags(){
	GenericSLNode<wchar_t>* pNode;
	while(!(pNode = m_Tags.Get_Head())) return false;

	while (pNode){
		WriteRawFmt(L"</%s>", pNode->m_pData);
		HTMLStrPoolFree(pNode->m_pData);
		pNode = pNode->m_pNext;
	}

	m_Tags.Remove_All();
	return true;
}

bool CHTMLWriter::WriteRaw(const wchar_t* pString){
	char badchar;
	BOOL mapfailed;
	char *pCur;
	wchar_t *pCurW;
	int len, i;
	DWORD n;
	UINT needed, count;
	ULONG w;

	len = StrLenW(pString);


	badchar = '?';

	needed = WideCharToMultiByte(GetACP(), 0, pString, -1, NULL, 0, NULL, &mapfailed);

	if (needed > INTERNAL_BUFFER_SIZEA) return false;

	if (!mapfailed){
		count = WideCharToMultiByte(GetACP(), 0, pString, -1, m_pBufferA, INTERNAL_BUFFER_SIZEA, &badchar, NULL);
		if (count < INTERNAL_BUFFER_SIZEA)
			count--;
		m_pStream->Write(m_pBufferA, count, &w);
	} else {
		pCur = m_pBufferA;
		for (pCurW = (wchar_t*)pString; pCurW < pString+len; pCurW++){
			//Surrogate pair
			if (INRANGE(*pCurW, WCH_UTF16_HIGH_FIRST, WCH_UTF16_HIGH_LAST)){
				n = (((DWORD)*(pCurW++))-WCH_UTF16_HIGH_FIRST) << 10;
				n |= ((DWORD)*(pCurW))-WCH_UTF16_LOW_FIRST;
__entity:
				pCur = m_pBufferA;
				*(pCur++) = '&';
				*(pCur++) = '#';
				_ultoa(n, pCur, 10);
				pCur += i = StrLen(pCur);
				*(pCur++) = ';';
				m_pStream->Write(m_pBufferA, i+3, &w);
			} else {
				count = WideCharToMultiByte(GetACP(), 0, pCurW, 1, m_pBufferA, INTERNAL_BUFFER_SIZEA, NULL, &mapfailed);
				if (mapfailed)
					goto __entity;
				m_pStream->Write(m_pBufferA, count, &w);
			}
		}
	}

	return true;
}

bool CHTMLWriter::WriteRawFmt(const wchar_t *pFmt, ...){
	va_list va;
	va_start(va, pFmt);
	_vsnwprintf(m_pBufferW, INTERNAL_BUFFER_SIZEW, pFmt, va);
	va_end(va);
	return WriteRaw(m_pBufferW);
}

bool CHTMLWriter::WriteString(const wchar_t* pString){
	BOOL mapfailed;
	char *pCur;
	wchar_t *pCurW;
	int len, i;
	DWORD n;
	UINT count;
	ULONG w;
	lpHTMLEntity pEntity;

	len = StrLenW(pString);



	pCur = m_pBufferA;
	for (pCurW = (wchar_t*)pString; pCurW < pString+len; pCurW++){
		//Surrogate pair
		
		if (INRANGE(*pCurW, WCH_UTF16_HIGH_FIRST, WCH_UTF16_HIGH_LAST)){
			n = (((DWORD)*(pCurW++))-WCH_UTF16_HIGH_FIRST) << 10;
			n |= ((DWORD)*(pCurW))-WCH_UTF16_LOW_FIRST;
__entity:
			pCur = m_pBufferA;
			*(pCur++) = '&';
			*(pCur++) = '#';
			_ultoa(n, pCur, 10);
			pCur += i = StrLen(pCur);
			*(pCur++) = ';';
			m_pStream->Write(m_pBufferA, i+3, &w);
		} if ((pEntity = FindEntity(*pCurW))){
			m_pStream->Write(pEntity->m_pEntitized, StrLen(pEntity->m_pEntitized), &w);

		}else {
			count = WideCharToMultiByte(GetACP(), 0, pCurW, 1, m_pBufferA, INTERNAL_BUFFER_SIZEA, NULL, &mapfailed);
			if (mapfailed){
				n = *pCurW;
				goto __entity;
			}
			m_pStream->Write(m_pBufferA, count, &w);
		}
	}
	return false;
}

bool CHTMLWriter::WriteStringFmt(const wchar_t *pFmt, ...){
	va_list va;
	va_start(va, pFmt);
	_vsnwprintf(m_pBufferW, INTERNAL_BUFFER_SIZEW, pFmt, va);
	va_end(va);
	return WriteString(m_pBufferW);
}

bool CHTMLWriter::Finalize(){
	char c;
	ULONG w;
	LARGE_INTEGER off;
	ULARGE_INTEGER newoff;
	c = 0;
	off.QuadPart = 0;
	m_pStream->Write(&c, 1, &w);
	m_pStream->Seek(off, STREAM_SEEK_SET, &newoff);

	return true;
}


