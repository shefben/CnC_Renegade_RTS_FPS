#include "../Shared/memory.h"
#include "STB.h"
#include "../VFS/API/VFSInterface.h"
#include "../VFS/API/VFSErrorcodes.h"
#include "../Shared/Hash.h"
#include <stdio.h>
#include "../Shared/StdLib.h"
/*
CStrVAR functions
*/

CStrVAR::CStrVAR(){
	m_String_Size = m_Temp_Size = 0;
	m_pString = m_pTemp_String = NULL;
}

CStrVAR::CStrVAR(short size, short){
	m_String_Size = m_Temp_Size = 0;
	m_pString = m_pTemp_String = NULL;
	Alloc(size, 0);
}

CStrVAR::~CStrVAR(){
	Del();
}

char *CStrVAR::Alloc(short size, short){
	Del();
	if (size){
		m_String_Size = size;
		m_pString = (char*)g_pAllocator(size);
	}
	return m_pString;
}

short CStrVAR::BuffLength(){
	return m_String_Size;
}

void CStrVAR::Del(){
	if (m_pString) {
		g_pFreeer(m_pString);
		m_pString = NULL;
	}

	m_String_Size = 0;

	if (m_pTemp_String) {
		g_pFreeer(m_pTemp_String);
		m_pTemp_String = NULL;
	}

	m_Temp_Size = 0;
}

const char *CStrVAR::GetTokenFirst(const char *pDelim){
	short len = (short)StrLen(m_pString);
	if (len >= m_Temp_Size){
		if (m_pTemp_String) g_pFreeer(m_pTemp_String);
		m_pTemp_String = (char*)g_pAllocator((size_t)(m_Temp_Size = len+1));
	}
	StrCpy(m_pTemp_String, m_pString);
	return strtok(m_pTemp_String, pDelim);
}

const char *CStrVAR::GetTokenNext(const char *pDelim){
	return strtok(NULL, pDelim);
}

const char *CStrVAR::Printf(const char *pFmt, ...){
	va_list va;
	if (!m_pString) return NULL;
	va_start(va, pFmt);
	_vsnprintf(m_pString, (size_t)m_String_Size, pFmt, va);
	va_end(va);
	return m_pString;
}

const char *CStrVAR::ReadString(VFSFileHandle hFile, bool ignore_whitespace){
	return "";
}

void CStrVAR::Set(const char *pString){
	short len;
	Del();
	if (pString){
		len = StrLen(pString);
		m_pString = (char*)g_pAllocator((size_t)len+1);
		StrCpy(m_pString, pString);
		m_String_Size = len;
	}
}

char *CStrVAR::Get(){
	return m_pString;
}

bool CStrVAR::SaveString(file hFile){
	return false;
}

HASHKEY CStrVAR::GetHASH(){
	return 0;//GetStrHash(m_pString, FALSE);
}

/*
classSTB functions
*/

classSTB::classSTB(IVFS *pVFS){
	m_pRow_Captions = NULL;
	m_pRow_Offsets = NULL;
	m_Row_Count = m_Col_Count = 0;
	m_File = VFS_INVALID_FILE;
	m_pVFS = pVFS;
}

classSTB::~classSTB(){
	if (VFS_VALID_FILE(m_File))
		m_pVFS->VFSCloseFile(m_File);
	
	if (m_pRow_Captions) delete [] m_pRow_Captions;
	if (m_pRow_Offsets) free(m_pRow_Offsets);
}

bool classSTB::Open(const wchar_t *pFilename, int embedded_offset){
	m_File = m_pVFS->VFSCreateFile(pFilename, FILE_OPEN_READ | FILE_OPEN_EXISTING);
	
	char type;
	int temp;
	short s;
	type = 0;
	if (m_File == VFS_INVALID_FILE) return false;

	if (embedded_offset < 0) m_Embedded_Offset = 0;
	else m_Embedded_Offset = embedded_offset+4;

	m_pVFS->VFSSeek(m_File, m_Embedded_Offset, SEEK_SET);

	m_pVFS->VFSRead(m_File, &temp, 4, NULL);
	if (((char*)&temp)[0] == 'S' && ((char*)&temp)[1] == 'T' && ((char*)&temp)[2] == 'B'){
		type = ((char*)&temp)[3]-'0';
	m_pVFS->VFSRead(m_File, &temp, 4, NULL);
	}
	m_pVFS->VFSRead(m_File, &m_Row_Count, 4, NULL);
	m_pVFS->VFSRead(m_File, &m_Col_Count, 4, NULL);
	m_Embedded_Offset += temp;
	m_pVFS->VFSSeek(m_File, 4, SEEK_CUR);

	/*
	Do type specific stuff
	*/

	if (type == 0)
		m_pVFS->VFSSeek(m_File, 4, SEEK_CUR);
	else if (type == 1)
		m_pVFS->VFSSeek(m_File, m_Col_Count*2+2, SEEK_CUR);

	/*
	Skip col descriptions
	*/

	for (int i = 0; i < m_Col_Count; i++){
		m_pVFS->VFSRead(m_File, &s, 2, NULL);
		m_pVFS->VFSSeek(m_File, (ssize_64)s, SEEK_CUR);
	}

	/*
	Read row descriptions
	*/

	m_pRow_Captions = new CStrVAR[m_Row_Count]();

	m_pVFS->VFSRead(m_File, &s, 2, NULL);
	m_pVFS->VFSSeek(m_File, (ssize_64)s, SEEK_CUR);

	for (int i = 0; i < m_Row_Count-1; i++){
		m_pVFS->VFSRead(m_File, &s, 2, NULL);
		m_pRow_Captions[i].Alloc(s+1, 0);
		m_pVFS->VFSRead(m_File, (void*)m_pRow_Captions[i].Get(), s, NULL);
		m_pRow_Captions[i].Get()[s] = 0;
		//printf("> \"%S\"\n", m_pRow_Captions[i].Get());
	}

	m_Row_Count--;
	m_Col_Count--;


	m_pVFS->VFSSeek(m_File, m_Embedded_Offset, SEEK_SET);

	m_pRow_Offsets = (unsigned long*)g_pAllocator(temp = (sizeof(unsigned long)*m_Row_Count));
	memset(m_pRow_Offsets, 0, temp);
	m_pRow_Offsets[0] = (unsigned long)m_pVFS->VFSTell(m_File);

	return true;
}

void classSTB::Close(){
	if (VFS_VALID_FILE(m_File))
		m_pVFS->VFSCloseFile(m_File);
	
	if (m_pRow_Captions) delete [] m_pRow_Captions;
	if (m_pRow_Offsets) g_pFreeer(m_pRow_Offsets);

	m_pRow_Captions = NULL;
	m_pRow_Offsets = NULL;
	m_Row_Count = m_Col_Count = 0;
	m_File = VFS_INVALID_FILE;
}

int classSTB::GetColumnCount(){
	return m_Col_Count;
}

int classSTB::GetRowCount(){
	return m_Row_Count;
}

HASHKEY classSTB::GetHASH(int row){
	if (row >= 0 && row < m_Row_Count)
		return m_pRow_Captions[row].GetHASH();
	return (HASHKEY)0;
}

int classSTB::GetInteger(){
	int col;
	unsigned short s;
	if (m_pVFS->VFSEndOfFile(m_File)) return 0;
	if (m_Current_Column >= m_Col_Count){
		if (m_Current_Row >= m_Row_Count) return 0;
		m_Current_Row++;
		m_Current_Column = 0;
	}
	col = m_Current_Column;
	m_Current_Column++;
	if (col == 0)
		m_pRow_Offsets[m_Current_Row] = (unsigned long)m_pVFS->VFSTell(m_File);
	m_pVFS->VFSRead(m_File, &s, 2, NULL);
	if (s == 0 || s >= 260) return 0;
	m_pVFS->VFSRead(m_File, m_Buf, s, NULL);
	m_Buf[s] = 0;
	return atoi(m_Buf);
}

const char *classSTB::GetString(){
	int col;
	unsigned short s;
	if (m_pVFS->VFSEndOfFile(m_File)) return 0;
	if (m_Current_Column >= m_Col_Count){
		if (m_Current_Row >= m_Row_Count) return NULL;
		m_Current_Row++;
		m_Current_Column = 0;
	}
	col = m_Current_Column;
	m_Current_Column++;
	if (col == 0)
		m_pRow_Offsets[m_Current_Row] = (unsigned long)m_pVFS->VFSTell(m_File);
	m_pVFS->VFSRead(m_File, &s, 2, NULL);
	if (!s) return NULL;
	if (s > 0 && s < sizeof(m_Buf)){
		m_pVFS->VFSRead(m_File, m_Buf, 2, NULL);
		m_Buf[s] = 0;
	} else {
		m_pVFS->VFSRead(m_File, m_Buf, sizeof(m_Buf)-1, NULL);
		m_Buf[sizeof(m_Buf)-1] = 0;
		m_pVFS->VFSSeek(m_File, s-sizeof(m_Buf), SEEK_CUR);
	}
	return m_Buf;
}

bool classSTB::SetIndexPosition(int col, int row){
	int temp;
	short s;
	if (row >= m_Row_Count || col >= m_Col_Count)
		return false; //Boundaries exceeded
	if (row == m_Current_Row && col == m_Current_Column)
		return true; //Field already queried

	if (m_pRow_Offsets[row] == 0){
		temp = row-1;
		if (m_pRow_Offsets[temp] == 0)
			while (m_pRow_Offsets[temp] == 0)
				temp--;

		m_pVFS->VFSSeek(m_File, m_pRow_Offsets[temp], SEEK_SET);

		for (int _r = temp; _r <= row; _r++){
			m_pRow_Offsets[_r] = (unsigned long)m_pVFS->VFSTell(m_File);
			for (int _c = 0; _c < m_Col_Count; _c++){
				m_pVFS->VFSRead(m_File, &s, 2, NULL);
				m_pVFS->VFSSeek(m_File, s, SEEK_CUR);
			}
		}
	}
	m_pVFS->VFSSeek(m_File, m_pRow_Offsets[row], SEEK_SET);
	temp = col;
	while (temp){
		m_pVFS->VFSRead(m_File, &s, 2, NULL);
		m_pVFS->VFSSeek(m_File, s, SEEK_CUR);
		temp--;
	}
	m_Current_Row = row;
	m_Current_Column = col;
	return true;
}
