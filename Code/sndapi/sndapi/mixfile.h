#ifndef _MIXFILE_H_
#define _MIXFILE_H_

#include <windows.h>
#include <conio.h>
#include "FileClass.h"
#include "WideStringClass.h"
#include "VectorAndListTemplateClasses.h"

#include "CRC32.H"

#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |   \
                ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))

#define MIX4CC MAKEFOURCC('M', 'I', 'X', '1')

class MixFileClass;


typedef struct {
	unsigned long m_FOURCC;
	unsigned long m_FileHeaderOffset;
	unsigned long m_FileNamesOffset;
	unsigned long m_Zero; //Pad?
} MIXHeader;

typedef struct {
	unsigned long m_CRC32;
	unsigned long m_FileOffset;
	unsigned long m_FileSize;
} MIXFileHeader;

__inline bool operator != (const MIXFileHeader &m0, const MIXFileHeader &m1){
	return !(m0.m_CRC32 == m1.m_CRC32 && m0.m_FileOffset == m1.m_FileOffset && m0.m_FileSize == m1.m_FileSize);
}

__inline bool operator == (const MIXFileHeader &m0, const MIXFileHeader &m1){
	return (m0.m_CRC32 == m1.m_CRC32 && m0.m_FileOffset == m1.m_FileOffset && m0.m_FileSize == m1.m_FileSize);
}

typedef struct {
    int m_ID;
	unsigned long m_Size;
	unsigned long m_FilePos;
	unsigned long m_RawOffset;
} MIXFile;

class cMIXFileRead {
private:
	MIXHeader m_Header;
	DynamicVectorClass<MIXFileHeader> m_FileHeaders;
	DynamicVectorClass<StringClass*> m_FileNames;
	DynamicVectorClass<MixFileClass *> m_OpenFiles;
	RawFileClass m_MIXFile;
	int m_CurrentMIXFileID;
	int m_FileCount;
	int m_StrCount;
public:
	cMIXFileRead(char *MIXFilename);
	~cMIXFileRead();
	void FreeData();
	MixFileClass *OpenMIXFile(char *filename);
	unsigned long ReadMIXFile(MIXFile *file, void *buffer, unsigned long count);
	unsigned long TellMIXFile(MIXFile *file);
	unsigned long SeekMIXFile(MIXFile *file, unsigned long sval, int seektype);
	void CloseMIXFile(MixFileClass *file);
	int FileCount();
	char *GetFileName(int idx);
	bool Open();
};

class cMIXFileCreate {
private:
	DynamicVectorClass<MIXFileHeader> m_FileHeaders;
	DynamicVectorClass<StringClass*> m_FileNames;
	DynamicVectorClass<StringClass*> m_Paths;

	RawFileClass m_MIXFile;
	MIXHeader m_Header;
	int m_CurrentMIXFileID;
	int m_FileCount;
	int m_StrCount;
	unsigned long CurrentOffset;
public:
	cMIXFileCreate();
	~cMIXFileCreate();
	bool Open(char *fname);
	void Close();
	inline void UpdateOffset();
	void AddFile(char *Path, char *FileName);
	void WriteMIX();
	void FreeData();
};

class MixFileClass : public FileClass {
	friend class cMIXFileRead;
public:
	MixFileClass();
	virtual ~MixFileClass();
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
	MIXFile m_File;
	cMIXFileRead *m_MIX;
};


#endif //_MIXFILE_H_