#include <stdio.h>
#include "fileclass.h"
#include "memory.h"

FileClass::~FileClass(){};


inline unsigned long Get_File_Pointer(HANDLE hFile){
	return SetFilePointer( 
    hFile, // must have GENERIC_READ and/or GENERIC_WRITE 
    0,     // do not move pointer 
    NULL,  // hFile is not large enough to need this pointer 
    FILE_CURRENT);  // provides offset from current position 
}

RawFileClass::RawFileClass(){
	f = INVALID_HANDLE_VALUE;
	fname = NULL;	
	fmode = 0;
}
RawFileClass::~RawFileClass(){
	if (fname)
		free(fname);
	if (f != INVALID_HANDLE_VALUE)
		CloseHandle(f);
}
char *RawFileClass::File_Name(){
	return fname;
}
void RawFileClass::Set_Name(char *filename){
	if (fname){
		free(fname);
		fname = NULL;
	}
	if (filename){
		fname = strdup(filename);
		strcpy(fname, filename);
	}
}
BOOL RawFileClass::Create(){
	if (fname){
		if (f && (fmode == 2 ||fmode == 3)){				
			Seek(0, SEEK_SET);
			return TRUE;
		}		
		if (fmode == 0){
			Open_(2);
			return TRUE;
		}
	}
	return FALSE;
}
BOOL RawFileClass::Destroy(){
	if (fname)
		return !_unlink(fname);
	return false;
}
BOOL RawFileClass::Is_Available(int mode){
	Open_(mode);
	BOOL val = FALSE;
	if (f != INVALID_HANDLE_VALUE)
		val = true;
	Close();
	return val;
}
BOOL RawFileClass::Is_Open(){
	return f != INVALID_HANDLE_VALUE;
}
void RawFileClass::Open(char *file_name, int mode){
	Set_Name(file_name);
	Open_(mode);
}
void RawFileClass::Open_(int mode){
	if (fname){
		if (f != INVALID_HANDLE_VALUE)
			CloseHandle(f);
		if (mode == 2 || mode == 3)
			f = CreateFile(fname,
				GENERIC_WRITE,
				0,
				NULL,         
				CREATE_ALWAYS,        
				FILE_ATTRIBUTE_NORMAL, 
				NULL);             

		if (mode == 1)
			f = CreateFile(fname,	
				GENERIC_READ,          
				FILE_SHARE_READ,      
				NULL,
				OPEN_EXISTING,         
				FILE_ATTRIBUTE_NORMAL, 
				NULL); 
		if (f != INVALID_HANDLE_VALUE){
			fmode = mode;
		}
		else
			fmode = 0;
	}
}
unsigned long RawFileClass::Read(void *buffer, unsigned long r_size){
	unsigned long i;
	if (f){
		ReadFile(f, buffer, r_size, &i, NULL);
		//if (i == 0){
		//	if (feof(f)){
		//		return 0;
		//	} else {
		//		//error handling here
		//	}
		//}
		return i;
	}
	return 0;
}
unsigned long RawFileClass::Seek(unsigned long offset, int origin){
	if (f != INVALID_HANDLE_VALUE)
		return SetFilePointer(f, offset, NULL, origin);
	return 0;
}
long RawFileClass::Tell(){
	if (f != INVALID_HANDLE_VALUE)
		return Get_File_Pointer(f);
	return 0;
}
unsigned long RawFileClass::Size(){
	if (f != INVALID_HANDLE_VALUE){
		return GetFileSize(f, NULL);
	}
	return 0;
}
unsigned long RawFileClass::Write(void *buffer, unsigned long size){
	unsigned long i;
	if (f != INVALID_HANDLE_VALUE){
		WriteFile(f, buffer, size, &i, NULL);
		return i;
	}
	return 0;
}
void RawFileClass::Close(){
	if (f != INVALID_HANDLE_VALUE){
		CloseHandle(f);
		f = INVALID_HANDLE_VALUE;
	}
}
unsigned long RawFileClass::Get_Date_Time(){
	return 0;
}
void RawFileClass::Set_Date_Time(unsigned long date_time){
}
void RawFileClass::Error(int i, int j, char *error){
}
HANDLE RawFileClass::Get_File_Handle(){
	return f;
}
void RawFileClass::Bias(int i, int j){
}

/*--Memory file class--*/

MemoryFileClass::MemoryFileClass(){
	m_Ptr = NULL;
	m_Size = m_Offset = 0;
}

MemoryFileClass::~MemoryFileClass(){
}

char *MemoryFileClass::File_Name(){
	return NULL;
}

void MemoryFileClass::Set_Name(char *filename){
}

BOOL MemoryFileClass::Create(){
	return FALSE;
}

BOOL MemoryFileClass::Destroy(){
	return FALSE;
}

BOOL MemoryFileClass::Is_Available(int mode){
	return FALSE;
}

BOOL MemoryFileClass::Is_Open(){
	return m_Ptr ? TRUE : FALSE;
}

void MemoryFileClass::Open(char *file_name, int mode){

}

void MemoryFileClass::Open_(int mode){
}

unsigned long MemoryFileClass::Read(void *buffer, unsigned long r_size){
	unsigned long i;
	if (m_Ptr){
		i = m_Offset+(int)r_size < m_Size ? r_size : m_Size-m_Offset;
		memcpy(buffer, m_Ptr+m_Offset, i);
		m_Offset+=i;
		return i;
	}
	return 0;
}

unsigned long MemoryFileClass::Seek(unsigned long offset, int origin){
	int new_offset = 0, temp;
	if (m_Ptr){
		switch (origin){
			case FILE_BEGIN:
				new_offset = 0;
				break;
			case FILE_END:
				new_offset = m_Size-offset;
				break;
			case FILE_CURRENT:
				new_offset = m_Offset+offset;
				break;
			default:
				break;
		}
		if (new_offset >=0 && new_offset < m_Size){
			temp = new_offset-m_Offset;
			m_Offset = new_offset;
			return temp;
		}
	}
	return 0;
}

long MemoryFileClass::Tell(){
	if (m_Ptr) return m_Offset;
	return 0;
}

unsigned long MemoryFileClass::Size(){
	if (m_Ptr){
		return m_Size;
	}
	return 0;
}

unsigned long MemoryFileClass::Write(void *buffer, unsigned long size){
	return 0;
}

void MemoryFileClass::Close(){
}

unsigned long MemoryFileClass::Get_Date_Time(){
	return 0;
}

void MemoryFileClass::Set_Date_Time(unsigned long date_time){
}

void MemoryFileClass::Error(int i, int j, char *error){
}

HANDLE MemoryFileClass::Get_File_Handle(){
	return (HANDLE)m_Ptr;
}

void MemoryFileClass::Bias(int i, int j){
}

void MemoryFileClass::Set_Handle(unsigned char *ptr, int size){
	m_Ptr = ptr;
	m_Size = size;
}
