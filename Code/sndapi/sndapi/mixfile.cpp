#include <windows.h>
#include <conio.h>
#include "FileClass.h"
#include "WideStringClass.h"
#include "mixfile.h"


char *Strip_Path(char *fname){
	char *temp;
	if ((temp = strrchr(fname, '\\')) || (temp = strchr(fname, '//'))) return temp+1;
	return fname;
}

cMIXFileRead::cMIXFileRead(char *MIXFilename){
	m_MIXFile.Open(MIXFilename, fo_read);
}
cMIXFileRead::~cMIXFileRead(){
	m_MIXFile.Close();
	FreeData();
}
void cMIXFileRead::FreeData(){
	for (int i = 0; i < m_FileNames.m_nItemCount; i++)
		delete(m_FileNames[i]);
	m_FileNames.Clear();
	m_FileHeaders.Clear();
	for (int i = 0; i < m_OpenFiles.m_nItemCount; i++)
		delete m_OpenFiles[i];
	m_OpenFiles.Clear();
}
MixFileClass *cMIXFileRead::OpenMIXFile(char *filename){
	MixFileClass *tempfile;
	int idx = -1;
	for (int i = 0; i < m_FileNames.m_nItemCount; i++){
		if (stricmp(Strip_Path(filename), (*m_FileNames[i])()) != 0)
			continue;
		idx = i;
		break;
	}
	if (idx == -1) return NULL;
	tempfile = new MixFileClass();
	if (tempfile){
		tempfile->m_File.m_ID = idx; //m_CurrentMIXFileID++;
		tempfile->m_File.m_RawOffset = m_FileHeaders[idx].m_FileOffset;
		tempfile->m_File.m_Size = m_FileHeaders[idx].m_FileSize;
		tempfile->m_File.m_FilePos = 0;
		tempfile->m_MIX = this;
		return tempfile;
	}
	return NULL;
}
unsigned long cMIXFileRead::ReadMIXFile(MIXFile *file, void *buffer, unsigned long count){
	unsigned long rval;
	m_MIXFile.Seek(file->m_RawOffset+file->m_FilePos, SEEK_SET);
	rval = m_MIXFile.Read(buffer, (count < file->m_Size-file->m_FilePos ? count : file->m_Size-file->m_FilePos));
	file->m_FilePos += rval;
	return rval;
}
unsigned long cMIXFileRead::TellMIXFile(MIXFile *file){
	return file->m_FilePos;
}
unsigned long cMIXFileRead::SeekMIXFile(MIXFile *file, unsigned long sval, int seektype){
	switch (seektype){
	case SEEK_SET:
		file->m_FilePos = (sval < file->m_Size ? sval : file->m_Size);
		break;
	case SEEK_CUR:
		file->m_FilePos = (sval + file->m_FilePos < file->m_Size ? file->m_FilePos + sval : file->m_Size);
		break;
	case SEEK_END:
		file->m_FilePos = (file->m_Size - sval < 0 ? 0 : file->m_Size - sval);
		break;					
		}
	return file->m_FilePos;
}
void cMIXFileRead::CloseMIXFile(MixFileClass *file){
	m_OpenFiles.Delete(m_OpenFiles.ID(file));
	delete file;
}
int cMIXFileRead::FileCount(){
	return m_FileNames.m_nItemCount;
}
char *cMIXFileRead::GetFileName(int idx){
	return (*m_FileNames[idx])();
}
bool cMIXFileRead::Open(){
	MIXFileHeader fh;
	StringClass *str;
	unsigned char slen;
	m_MIXFile.Read(&m_Header, sizeof(MIXHeader));
	if (m_Header.m_FOURCC != MIX4CC)
		return false;
	m_MIXFile.Seek(m_Header.m_FileHeaderOffset, SEEK_SET);
	m_MIXFile.Read(&m_FileCount, 4);
	for (int i = 0; i < m_FileCount; i++){
		m_MIXFile.Read(&fh, sizeof(MIXFileHeader));
		m_FileHeaders.Add(fh);
	}
	m_MIXFile.Seek(m_Header.m_FileNamesOffset, SEEK_SET);
	m_MIXFile.Read(&m_StrCount, 4);
	for (int i = 0; i < m_StrCount; i++){
		m_MIXFile.Read(&slen, 1);
		str = new StringClass();
		if (!str){
			m_MIXFile.Seek(slen, FILE_CURRENT);
			continue;
		}
		str->Uninitialised_Grow(slen);
		m_MIXFile.Read((*str)(), slen);				
		m_FileNames.Add(str);
		
	}
	return true;
}


cMIXFileCreate::cMIXFileCreate(){
	m_Header.m_FileHeaderOffset = 16;
	m_Header.m_FOURCC = MIX4CC;
	UpdateOffset();
	m_Header.m_Zero = 0;
	CurrentOffset = 16;
}
cMIXFileCreate::~cMIXFileCreate(){
	Close();
	FreeData();
}
bool cMIXFileCreate::Open(char *fname){		
	m_MIXFile.Open(fname, fo_create);
	return (m_MIXFile.Is_Open() ? true : false);
}
void cMIXFileCreate::Close(){
	if (m_MIXFile.Is_Open())
		m_MIXFile.Close();
}
void cMIXFileCreate::UpdateOffset(){		
	m_Header.m_FileNamesOffset = m_Header.m_FileHeaderOffset+(sizeof(MIXFileHeader)*m_FileHeaders.m_nItemCount)+4;
}
void cMIXFileCreate::AddFile(char *Path, char *FileName){
	char Dir[MAX_PATH];
	RawFileClass test;
	_snprintf(Dir, MAX_PATH, "%s\\%s", Path, FileName);		
	test.Open(Dir, fo_read);
	if (test.Is_Open()){
		MIXFileHeader h;
		StringClass *p = new StringClass;
		StringClass *f = new StringClass;
		p->SetString(Dir);
		f->SetString(FileName);
		_snprintf(Dir, MAX_PATH, "%s", FileName);
		h.m_CRC32 = CRC_STRING(_strupr(Dir));
		h.m_FileSize = test.Size();
		h.m_FileOffset = CurrentOffset;
		CurrentOffset += h.m_FileSize;
		m_Header.m_FileHeaderOffset += h.m_FileSize;
		m_Paths.Add(p);
		m_FileNames.Add(f);
		m_FileHeaders.Add(h);
		UpdateOffset();
	}
}
void cMIXFileCreate::WriteMIX(){
	unsigned char *Buffer = (unsigned char*)malloc(8192);
	unsigned long initoffset = sizeof(MIXHeader)+4+(sizeof(MIXFileHeader)*m_FileHeaders.m_nItemCount);
	unsigned long temp;
	unsigned char c;
	m_Header.m_FOURCC = MAKEFOURCC('M', 'I', 'X', '1');
	m_MIXFile.Seek(0, SEEK_SET);
	m_MIXFile.Write(&m_Header, sizeof(MIXHeader));

	temp = m_FileHeaders.m_nItemCount;
	for (int i = 0; i < m_FileHeaders.m_nItemCount; i++){
		m_MIXFile.Seek(m_FileHeaders[i].m_FileOffset, SEEK_SET);
		RawFileClass in;
		in.Open(m_Paths[i]->GetString(), fo_read);
		if (in.Is_Open()){
			int j = 0;
			while ((j = in.Read(Buffer, 8192)))
				m_MIXFile.Write(Buffer, j);
			in.Close();
		}			
	}

	m_MIXFile.Write(&temp, 4);
	for (int i = 0; i < m_FileHeaders.m_nItemCount; i++){
		m_MIXFile.Write(&m_FileHeaders[i], sizeof(MIXFileHeader));
	}

	m_MIXFile.Write(&temp, 4);
	for (int i = 0; i < m_FileNames.m_nItemCount; i++){
		c = (unsigned char)strlen(m_FileNames[i]->GetString());
		c++;
		m_MIXFile.Write(&c, 1);
		m_MIXFile.Write(m_FileNames[i]->GetString(), c);
	}

	free(Buffer);
}
void cMIXFileCreate::FreeData(){
	while (m_FileNames.m_nItemCount){
		delete m_FileNames[0];
		m_FileNames.Delete(0);
	}
	while (m_Paths.m_nItemCount){
		delete m_Paths[0];
		m_Paths.Delete(0);
	}
}

MixFileClass::MixFileClass(){
	m_MIX = NULL;
}
MixFileClass::~MixFileClass(){
	//Close();
}
char *MixFileClass::File_Name(){
	return m_MIX->GetFileName(m_File.m_ID);
}
void MixFileClass::Set_Name(char *filename){
}
BOOL MixFileClass::Create(){
	return FALSE;
}
BOOL MixFileClass::Destroy(){
	return FALSE;
}
BOOL MixFileClass::Is_Available(int mode){
	return FALSE;
}
BOOL MixFileClass::Is_Open(){
	return FALSE;
}
void MixFileClass::Open(char *file_name, int mode){
}
void MixFileClass::Open_(int mode){
}
unsigned long MixFileClass::Read(void *buffer, unsigned long r_size){
	return m_MIX->ReadMIXFile(&m_File, buffer, r_size);
}
unsigned long MixFileClass::Seek(unsigned long offset, int origin){
	m_MIX->SeekMIXFile(&m_File, offset, origin);
	return 0;
}
long MixFileClass::Tell(){
	return m_MIX->TellMIXFile(&m_File);
}
unsigned long MixFileClass::Size(){
	return m_File.m_Size;
}
unsigned long MixFileClass::Write(void *buffer, unsigned long size){
	return 0;
}
void MixFileClass::Close(){
	m_MIX->CloseMIXFile(this);
}
unsigned long MixFileClass::Get_Date_Time(){
	return 0;
}
void MixFileClass::Set_Date_Time(unsigned long date_time){
}
void MixFileClass::Error(int i, int j, char *error){
}
HANDLE MixFileClass::Get_File_Handle(){
	return (HANDLE)&m_File;
}
void MixFileClass::Bias(int i, int j){
}