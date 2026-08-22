#include <stdio.h>
#include "fileclass.h"

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
wchar_t *RawFileClass::File_Name(){
	return fname;
}
void RawFileClass::Set_Name(wchar_t *filename){
	if (filename){
		fname = wcsdup(filename);
	}
}
BOOL RawFileClass::Create(){
	if (fname){
		if (f && (fmode == fo_create ||fmode == fo_write)){
			Seek(0, SEEK_SET);
			return TRUE;
		}		
		if (fmode == 0){
			Open_(fo_write);
			return TRUE;
		}
	}
	return FALSE;
}
BOOL RawFileClass::Destroy(){
	/*if (fname)
		return !_unlink(fname);	*/
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
void RawFileClass::Open(wchar_t *file_name, int mode){
	Set_Name(file_name);
	Open_(mode);
}
void RawFileClass::Open_(int mode){
	if (fname){
		if (f != INVALID_HANDLE_VALUE)
			CloseHandle(f);
		if (mode == fo_write || mode == fo_create)
			f = CreateFile(fname,
				GENERIC_WRITE,
				0,
				NULL,         
				CREATE_ALWAYS,        
				FILE_ATTRIBUTE_NORMAL, 
				NULL);             

		if (mode == fo_read)
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