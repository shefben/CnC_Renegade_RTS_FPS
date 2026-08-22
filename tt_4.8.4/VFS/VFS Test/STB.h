#ifndef _STB_H_
#define _STB_H_

#include "../Shared/Hash.h"
#include "../VFS/API/VFSInterface.h"
#include "../VFS/API/VFSErrorcodes.h"

#define	IS_DIGIT(c)						((c >= '0' && c <= '9') ? 1 : 0)

class CStrVAR{
private:
	char *m_pTemp_String;
	short m_Temp_Size;
	char *m_pString;
	short m_String_Size;
public:
	CStrVAR();
	CStrVAR(short size, short);
	~CStrVAR();
	char *Alloc(short size, short);
	short BuffLength();
	void Del();
	char *Get();
	const char *GetTokenFirst(const char *pDelim);
	const char *GetTokenNext( const char *pDelim);
	const char *Printf(const char *pFmt, ...);
	const char *ReadString(file hFile, bool ignore_whitespace);
	void Set(const char *pString);
	bool SaveString(file hFile);
	HASHKEY GetHASH();
};

class classSTB {
private:	
	VFSFileHandle	m_File;				//0
	int				m_Embedded_Offset;	//4
	unsigned long*	m_pRow_Offsets;		//8
	int				m_Current_Row;		//c
	int				m_Current_Column;	//10
	CStrVAR*		m_pRow_Captions;	//14
	char			m_Buf[0x1000];		//18
	int				m_Row_Count;		//1018
	int				m_Col_Count;		//101C
	IVFS*			m_pVFS;
public:
	classSTB(IVFS* pVFS);
	~classSTB();
	bool Open(const wchar_t *pFilename, int embedded_offset);
	void Close();
	int GetColumnCount();
	int GetRowCount();
	HASHKEY GetHASH(int row);
	int GetInteger();
	const char *GetString();
	bool SetIndexPosition(int col, int row);
};


#endif //_STB_H_