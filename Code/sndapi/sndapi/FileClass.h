#ifndef _FILECLASS_H_
#define _FILECLASS_H_
#include <windows.h>

enum { fo_read = 1, fo_create, fo_write };

class FileClass {
public:
	/* 0x00 */ virtual ~FileClass();
	/* 0x04 */ virtual char *File_Name() = 0;
	/* 0x08 */ virtual void Set_Name(char *filename) = 0;
	/* 0x0C */ virtual BOOL Create() = 0;
	/* 0x10 */ virtual BOOL Destroy() = 0;
	/* 0x14 */ virtual BOOL Is_Available(int mode) = 0;
	/* 0x18 */ virtual BOOL Is_Open() = 0;
	/* 0x1C */ virtual void Open(char *file_name, int mode) = 0;
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



class RawFileClass : public FileClass {
public:
	RawFileClass();
	virtual ~RawFileClass();
	virtual char *File_Name();
	virtual void Set_Name(char *filename);
	virtual BOOL Create();
	virtual BOOL Destroy();
	virtual BOOL Is_Available(int mode);
	virtual BOOL Is_Open();
	virtual void Open(char *file_name, int mode);
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
	char *fname;
	int fmode;
};

class MemoryFileClass : public FileClass {
public:
	MemoryFileClass();
	virtual ~MemoryFileClass();
	virtual char *File_Name();
	virtual void Set_Name(char *filename);
	virtual BOOL Create();
	virtual BOOL Destroy();
	virtual BOOL Is_Available(int mode);
	virtual BOOL Is_Open();
	virtual void Open(char *file_name, int mode);
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
	void Set_Handle(unsigned char *ptr, int size);
private:
	unsigned char *m_Ptr;
	int m_Size;
	int m_Offset;
};

#endif //_FILECLASS_H_