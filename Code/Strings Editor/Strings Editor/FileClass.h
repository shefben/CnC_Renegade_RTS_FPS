#ifndef _FILECLASS_H_
#define _FILECLASS_H_
#pragma once
#include <windows.h>
class FileClass {
public:
	/* 0x00 */ virtual ~FileClass();
	/* 0x04 */ virtual wchar_t *File_Name() = 0;
	/* 0x08 */ virtual void Set_Name(wchar_t *filename) = 0;
	/* 0x0C */ virtual BOOL Create() = 0;
	/* 0x10 */ virtual BOOL Destroy() = 0;
	/* 0x14 */ virtual BOOL Is_Available(int mode) = 0;
	/* 0x18 */ virtual BOOL Is_Open() = 0;
	/* 0x1C */ virtual void Open(wchar_t *file_name, int mode) = 0;
	/* 0x20 */ virtual void Open_(int mode) = 0;
	/* 0x24 */ virtual unsigned long Read(void *buffer, unsigned long size) = 0;
	/* 0x28 */ virtual unsigned long Seek(unsigned long offset, int origin) = 0;
	/* 0x2C */ virtual long Tell() = 0;
	/* 0x30 */ virtual unsigned long Size() = 0;
	/* 0x34 */ virtual unsigned long Write(void *buffer, unsigned long size) = 0;
	/* 0x38 */ virtual void Close() = 0;
	/* 0x3C */ virtual unsigned long Get_Date_Time() = 0;
	/* 0x40 */ virtual void Set_Date_Time(unsigned long date_time) = 0;
	/* 0x44 */ virtual void Error(int i, int j, char *error) = 0;
	/* 0x48 */ virtual HANDLE Get_File_Handle() = 0;
	/* 0x4C */ virtual void Bias(int i, int j) = 0;
};

enum { fo_read, fo_write, fo_create };


class RawFileClass : public FileClass {
public:
	RawFileClass();
	virtual ~RawFileClass();
	virtual wchar_t *File_Name();
	virtual void Set_Name(wchar_t *filename);
	virtual BOOL Create();
	virtual BOOL Destroy();
	virtual BOOL Is_Available(int mode);
	virtual BOOL Is_Open();
	virtual void Open(wchar_t *file_name, int mode);
	virtual void Open_(int mode);
	virtual unsigned long Read(void *buffer, unsigned long r_size);
	virtual unsigned long Seek(unsigned long offset, int origin);
	virtual long Tell();
	virtual unsigned long Size();
	virtual unsigned long Write(void *buffer, unsigned long size);
	virtual void Close();
	virtual unsigned long Get_Date_Time();
	virtual void Set_Date_Time(unsigned long date_time);
	virtual void Error(int i, int j, char *error);
	virtual HANDLE Get_File_Handle();
	virtual void Bias(int i, int j);
private:
	HANDLE f;
	wchar_t *fname;
	int fmode;
};

#endif //_FILECLASS_H_