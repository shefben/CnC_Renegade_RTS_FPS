#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "WideStringClass.h"
#include "VectorAndListTemplateclasses.h"
#include "sndpreset.h"
#include "Chunkclasses.h"
#include "Fileclass.h"
#include "mmsystem.h"
#include "mixfile.h"
#include "resource.h"

HINSTANCE g_hInstance;
HKEY g_hKey;
HANDLE g_hEvent = NULL;
HANDLE g_hThread;
unsigned long g_uThread;


/*-- Show the last error in a message box --*/

void MessageError(){
	char lpMsgBuf[512];
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_ENGLISH_US),
		(LPTSTR)lpMsgBuf,
		512, NULL );
	MessageBox(0, lpMsgBuf, "ERROR", MB_ICONERROR);	
}

/*-- Show the last error as debugstring --*/

void LogError(){
	char lpMsgBuf[512];
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_ENGLISH_US),
		(LPTSTR)lpMsgBuf,
		512, NULL );
	strcat(lpMsgBuf, "\n");
	OutputDebugStr(lpMsgBuf);	
}

/*-- Log formated --*/

void LogFormated(char *fmt, ...){
	char lBuf[512];
	va_list va;
	va_start(va, fmt);
	_vsnprintf(lBuf, 512, fmt, va);
	va_end(va);
	OutputDebugStr(lBuf);	
}

/*-- Example filter: "String table\0*.tdb\0\0" --*/

bool __OpenFile(char *buf, int bufsize, int *FileOffset, int *ExtensionOffset, char *filter, char *title = NULL){
	char lBuf[MAX_PATH] = "";
	OPENFILENAME of;

	memset(&of, 0, sizeof(OPENFILENAME));

	of.lStructSize = sizeof(OPENFILENAME);
	of.hwndOwner = NULL;
	of.hInstance = NULL;
	of.lpstrFilter = filter;
	of.lpstrCustomFilter = NULL;
	of.nMaxCustFilter = 0;
	of.nFilterIndex = 0;
	of.lpstrFile = lBuf;
	of.nMaxFile = MAX_PATH;
	of.lpstrFileTitle = NULL;
	of.nMaxFileTitle = 0;
	of.lpstrInitialDir = NULL;
	of.lpstrTitle = title;
	of.Flags = OFN_FILEMUSTEXIST; /*--*/
	of.nFileOffset = 0;
	of.nFileExtension = 0;
	of.lpstrDefExt = NULL;
	of.lCustData = NULL;
	of.lpfnHook = NULL;
	of.lpTemplateName = NULL;	

	if (!GetOpenFileName(&of)) return false;
	_snprintf(buf, bufsize, "%s", of.lpstrFile);
	if (FileOffset)
		*FileOffset = of.nFileOffset;
	if (ExtensionOffset)
		*ExtensionOffset = of.nFileExtension;
	return true;
}

bool __SaveFile(char *buf, int bufsize, int *FileOffset, int *ExtensionOffset, char *filter, char *defext, char *title = NULL, char *defname = ""){
	char lBuf[MAX_PATH] = "";
	strcpy(lBuf, defname);
	OPENFILENAME of;

	memset(&of, 0, sizeof(OPENFILENAME));

	of.lStructSize = sizeof(OPENFILENAME);
	of.hwndOwner = NULL;
	of.hInstance = NULL;
	of.lpstrFilter  = filter;
	of.lpstrCustomFilter = NULL;
	of.nMaxCustFilter = 0;
	of.nFilterIndex = 0;
	of.lpstrFile = lBuf;
	of.nMaxFile = MAX_PATH;
	of.lpstrFileTitle = NULL;
	of.nMaxFileTitle = 0;
	of.lpstrInitialDir = NULL;
	of.lpstrTitle = title;
	of.Flags = OFN_OVERWRITEPROMPT; /*--*/
	of.nFileOffset = 0;
	of.nFileExtension = 0;
	of.lpstrDefExt = defext;
	of.lCustData = NULL;
	of.lpfnHook = NULL;
	of.lpTemplateName = NULL;	
	if (!GetSaveFileName(&of)) return false;
	_snprintf(buf, bufsize, "%s", of.lpstrFile);
	if (FileOffset)
		*FileOffset = of.nFileOffset;
	if (ExtensionOffset)
		*ExtensionOffset = of.nFileExtension;
	return true;
}

void *Play_Sound_Filename(char *filename){
	FileClass *f = SoundDB::Get_Sound_File(filename);
	void *mem;
	int size;
	if (!f){
		LogFormated("Cannot open file: %s\n", filename);
		return NULL;
	}
	size = f->Size();
	if (!(mem = malloc(size))){
		LogFormated("Unable to allocate %d bytes of memory (file: %s)\n", size, filename);
		return NULL;
	}
	f->Read(mem, f->Size());
	sndPlaySound((char*)mem, SND_ASYNC | SND_MEMORY);
	return mem;
}

void *Play_Sound_File(FileClass *f){
	void *mem;
	if (!f) return NULL;
	if (!(mem = malloc(f->Size()))) return NULL;
	f->Read(mem, f->Size());
	sndPlaySound((char*)mem, SND_ASYNC | SND_MEMORY);
	return mem;
}

unsigned long __stdcall SoundThread(void *arg){
	MSG msg;
	void *cur_alloc = NULL;
	memset(&msg, 0, sizeof(MSG));
	FileClass *f = NULL;
	while (msg.message != THREAD_EXIT_MSG){
		if (GetMessage(&msg, NULL, 0U, 0U)) {
			sndPlaySound(NULL, 0);
			if (cur_alloc){
				free(cur_alloc);
				cur_alloc = NULL;
			}
			switch (msg.message) {
			case THREAD_PLAY_FILENAME:
				cur_alloc = Play_Sound_Filename((char*)msg.lParam);
				SetEvent(g_hEvent);
				break;
			case THREAD_PLAY_FILE:
				f = (FileClass*)msg.lParam;
				cur_alloc = Play_Sound_File(f);
				SetEvent(g_hEvent);
				break;
			case THREAD_PLAY_STOP:
				SetEvent(g_hEvent);
				break;
			}
		}
	}
	return NULL;
}


void SoundObject::ODB_Subload(ChunkLoadClass &load){
	while (load.Open_Chunk()){
		switch (load.Cur_Chunk_ID()){
			case 0x100:
				while (load.Open_Mircro_Chunk()){
					if (load.Current_Micro_Chunk_ID() == 11){
						m_Filename.Uninitialised_Grow(load.Current_Micro_Chunk_Length());
						load.Read(m_Filename(), load.Current_Micro_Chunk_Length());
					} else if (load.Current_Micro_Chunk_ID() == 10)
						load.Read(&m_Is_3D_Sound, 1);				
					
					load.Close_Mircro_Chunk();
				}
				break;
			case 0x200:
				while (load.Open_Chunk()){
					if (load.Cur_Chunk_ID() == 0x100){
						while (load.Open_Mircro_Chunk()){
							if (load.Current_Micro_Chunk_ID() == 1)
								load.Read(&m_Preset_ID, 4);
							else if (load.Current_Micro_Chunk_ID() == 3){
								m_Preset_Name.Uninitialised_Grow(load.Current_Micro_Chunk_Length());
								load.Read(m_Preset_Name(), load.Current_Micro_Chunk_Length());
							}
							load.Close_Mircro_Chunk();
						}
					}
					load.Close_Chunk();
				}
				break;
			default:
				break;
		}
		load.Close_Chunk();
	}
}

bool Dlg_Load_Options(HWND hDlg, HKEY key){
	HWND hList = GetDlgItem(hDlg, IDC_MOUNTS);
	char lBuf[8192*2];
	unsigned char lBuf2[MAX_PATH];	
	int i = 0;
	unsigned long size = 8192*2, type, dsize=MAX_PATH;
	HKEY hKey;

	if (RegOpenKeyEx(key, "sndapi\\mounts\\", 0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS) goto __end;

	while (RegEnumValue(hKey, i++, lBuf, &size, 0, &type, lBuf2, &dsize) == ERROR_SUCCESS){
		dsize = MAX_PATH;
		size = 8192*2;
		if (type == REG_SZ){
			//printf ("%s - ", lBuf2);
			SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)lBuf2);
		}

	}

	RegCloseKey(hKey);

	if (RegOpenKeyEx(key, "sndapi\\", 0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS) return true;
	dsize=MAX_PATH;

	if (RegQueryValueEx(hKey, "database", NULL, &type, lBuf2, &dsize) == ERROR_SUCCESS)
		if (type == REG_SZ)	
			SetDlgItemText(hDlg, IDC_DB, (char*)lBuf2);
	
	RegCloseKey(hKey);

__end:
	return true;
}

bool Dlg_Save_Options(HWND hDlg, HKEY key){
	HWND hList = GetDlgItem(hDlg, IDC_MOUNTS);
	char lBuf[8192*2], *temp;
	int i = 0, cItems;
	unsigned long size = 8192*2, type, dsize=MAX_PATH;
	HKEY hKey;

	if (RegCreateKeyEx(key, "sndapi\\mounts\\", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL) != ERROR_SUCCESS) goto __end;

	while (RegEnumValue(hKey, i, lBuf, &size, 0, &type, NULL, NULL) == ERROR_SUCCESS){
		size = 8192*2;
		if (type == REG_SZ){
			RegDeleteValue(hKey, lBuf);
		} else
			i++;
	}
	cItems = (int)SendMessage(hList, LB_GETCOUNT, 0, 0);
	for (i = 0; i < cItems; i++){
		temp = (char*)malloc(size = ((int)SendMessage(hList, LB_GETTEXTLEN, i, 0)+1));
		if (temp){
			SendMessage(hList, LB_GETTEXT, i, (LPARAM)temp);
			_snprintf((char*)lBuf, MAX_PATH, "%d", i);
			RegSetValueEx(hKey, lBuf, 0, REG_SZ, (unsigned char*)temp, size);
			free(temp);
		}
	}
	RegCloseKey(hKey);
	if (RegCreateKeyEx(key, "sndapi\\", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL) != ERROR_SUCCESS) goto __end;
	
	GetDlgItemText(hDlg, IDC_DB, lBuf, MAX_PATH);
	RegSetValueEx(hKey, "database", 0, REG_SZ, (unsigned char*)lBuf, (unsigned long)strlen(lBuf)+1);
	RegCloseKey(hKey);


__end:
	return true;
}

INT_PTR CALLBACK OptionsDlg(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam){
	HWND hList = GetDlgItem(hDlg, IDC_MOUNTS);
	int res;
	int cnt, tmp;
	char lBuf[MAX_PATH];
	switch (Msg){
		case WM_INITDIALOG:
			g_hKey = (HKEY)lParam;
			Dlg_Load_Options(hDlg, g_hKey);
			return TRUE;
		case WM_COMMAND:
			switch (LOWORD(wParam)){
		case IDC_OK:
			Dlg_Save_Options(hDlg, g_hKey);
			EndDialog(hDlg, 1);
			break;
		case IDC_CANCEL:			
			EndDialog(hDlg, 0);
			break;
		case IDC_ADD:
			if (__OpenFile(lBuf, MAX_PATH, NULL, NULL, "Mixfile\0*.mix;always.dat;always2.dat;always.dbs\0\0")){
				SendMessage(GetDlgItem(hDlg, IDC_MOUNTS), LB_ADDSTRING, 0, (LPARAM)lBuf);
			}
			break;
		case IDC_BROWSE:
			if (__OpenFile(lBuf, MAX_PATH, NULL, NULL, "Sound database\0sounds.sdb\0\0")){
				SetDlgItemText(hDlg, IDC_DB, lBuf);
			}
			break;

		case IDC_REMOVE:
			res = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);
			if (res != LB_ERR){
				SendMessage(hList, LB_DELETESTRING, res, 0);
				cnt = (int)SendMessage(hList, LB_GETCOUNT, 0, 0);
				tmp = cnt > res ? res : res-1;
				SendMessage(hList, LB_SETCURSEL, tmp > 0 ? tmp: 0, 0);
			}
			break;
			}

			return TRUE;
			break;
		case WM_CLOSE:
			EndDialog(hDlg, 0);
			return TRUE;
			break;
		default:
			break;
	}
	return FALSE;
}



SoundObject::SoundObject(){
	m_Preset_ID = 0;
}

void SoundObject::Load(ChunkLoadClass &load){
	while (load.Open_Mircro_Chunk()){
		switch (load.Current_Micro_Chunk_ID()){
			case 1:
				load.Read(&m_Preset_ID, 4);
				break;
			case 2:
				m_Preset_Name.Uninitialised_Grow(load.Current_Micro_Chunk_Length());
				load.Read(m_Preset_Name(), load.Current_Micro_Chunk_Length());
				break;
			case 3:
				m_Filename.Uninitialised_Grow(load.Current_Micro_Chunk_Length());
				load.Read(m_Filename(), load.Current_Micro_Chunk_Length());
				break;
			case 4:
				load.Read(&m_Is_3D_Sound, 1);
				break;
			default:
				break;
		}
		load.Close_Mircro_Chunk();
	}
}

void SoundObject::Save(ChunkSaveClass &save){
	save.Begin_Micro_Chunk(1);
	save.Write(&m_Preset_ID, 4);
	save.End_Micro_Chunk();

	save.Begin_Micro_Chunk(2);
	save.Write(m_Preset_Name(), (int)strlen(m_Preset_Name())+1);
	save.End_Micro_Chunk();

	save.Begin_Micro_Chunk(3);
	save.Write(m_Filename(), (int)strlen(m_Filename())+1);
	save.End_Micro_Chunk();

	save.Begin_Micro_Chunk(4);
	save.Write(&m_Is_3D_Sound, 1);
	save.End_Micro_Chunk();
}

void SoundObject::Load_From_ODB(ChunkLoadClass &load){
	while (load.Open_Chunk()){
		if (load.Cur_Chunk_ID() == 0x00100101){
			ODB_Subload(load);
		}
		load.Close_Chunk();
	}
}

unsigned long SoundObject::Get_ID(){
	return m_Preset_ID;
}

char *SoundObject::Get_Filename(){
	return m_Filename();
}

char *SoundObject::Get_Presetname(){
	return m_Preset_Name();
}

bool SoundObject::Is_3D_Sound(){
	return m_Is_3D_Sound;
}


void SoundObject::Set_ID(unsigned long ID){
	m_Preset_ID = ID;
}

void SoundObject::Set_Filename(char *name){
	m_Filename.SetString(name);
}

void SoundObject::Set_Prestname(char *name){
	m_Preset_Name.SetString(name);
}



DynamicVectorClass<SoundObject*> SoundDB::m_Objects(0,0);
DynamicVectorClass<cMIXFileRead*> SoundDB::m_Mounts(0,0);

int SoundDB::Get_Version(){
	return SNDAPI_VERSION;
}

bool SoundDB::Load_From_ODB(char *fname){
	SoundObject *obj = NULL;
	RawFileClass f;
	ChunkLoadClass load(&f);
	Clear_Objects();
	f.Open(fname, 1);
	if (!f.Is_Open()) return false; 

	while (load.Open_Chunk()){
		if (load.Cur_Chunk_ID() == 0x101){
			while (load.Open_Chunk()){
				if (load.Cur_Chunk_ID() == 0x101){
					while (load.Open_Chunk()){
						if (load.Cur_Chunk_ID() == 0x00030000){
							if (obj = new SoundObject){
								obj->Load_From_ODB(load);
								m_Objects.Add(obj);
							}
						}
						load.Close_Chunk();
					}
				}
				load.Close_Chunk();
			}
		}
		load.Close_Chunk();
	}
	f.Close();
	return m_Objects.m_nItemCount ? true : false;
}

bool SoundDB::Load_From_SoundDB(char *fname){
	Clear_Objects();
	RawFileClass f;
	SoundObject *obj;
	ChunkLoadClass load(&f);
	f.Open(fname, fo_read);
	if (!f.Is_Open()) return false;
	while (load.Open_Chunk()){
		if (load.Cur_Chunk_ID() == SND_CID){
			obj = new SoundObject();
			if (obj){
				obj->Load(load);
				SoundDB::m_Objects.Add(obj);
			}
		}
		load.Close_Chunk();
	}
	f.Close();
	return m_Objects.m_nItemCount ? true : false;
}

bool SoundDB::Save_To_SoundDB(char *fname){	
	int cCount;
	RawFileClass f;
	ChunkSaveClass save(&f);
	f.Open(fname, 2);
	if (!f.Is_Open()) return false;
	cCount = m_Objects.m_nItemCount;
	for (int i = 0; i < cCount; i++){
		save.Begin_Chunk(SND_CID);
		m_Objects[i]->Save(save);
		save.End_Chunk();
	}
	f.Close();
	return true;
}

void SoundDB::Clear_Mounts(){
	int cItems = SoundDB::m_Mounts.m_nItemCount;
	for (int i = 0; i < cItems; i++){
		delete SoundDB::m_Mounts[i];
	}
	SoundDB::m_Mounts.Clear();
}

void SoundDB::Clear_Objects(){
	int cItems = SoundDB::m_Objects.m_nItemCount;
	
	for (int i = 0; i < cItems; i++){
		delete SoundDB::m_Objects[i];
	}
	SoundDB::m_Objects.Clear();
}

ISoundObject *SoundDB::Find_Sound(unsigned long sound){
	int cItems = SoundDB::m_Objects.m_nItemCount;	
	for (int i = 0; i < cItems; i++){
		if (SoundDB::m_Objects[i]->Get_ID() == sound){
			return SoundDB::m_Objects[i];
		}
	}
	return NULL;
}

ISoundObject *SoundDB::Get_Sound(int idx){
	if (idx < 0 || SoundDB::m_Objects.m_nItemCount <= idx) return NULL;
	else return SoundDB::m_Objects[idx];
}

bool SoundDB::Mount_MIX(char *filename){
	cMIXFileRead *mix;
	mix = new cMIXFileRead(filename);
	
	if (!mix) return false;
	if (!mix->Open()){
		delete mix;
		return false;
	}
	
	if (SoundDB::m_Mounts.Add(mix))
		LogFormated("Mounted %s\n", filename);
	return true;
}

FileClass *SoundDB::Get_Sound_File(char *filename){
	MixFileClass *f = NULL;
	int cItems = SoundDB::m_Mounts.m_nItemCount;
	if (!filename) return NULL;
	for (int i = 0; i < cItems; i++){		
		if (f = SoundDB::m_Mounts[i]->OpenMIXFile(filename)) break;
	}
	return f;
}

void SoundDB::Play_Sound(char *filename){
	PostThreadMessage(g_uThread, THREAD_PLAY_FILENAME, NULL, (LPARAM)filename);
	WaitForSingleObject(g_hEvent, INFINITE);
}

void SoundDB::Play_Sound_Class(FileClass *file){
	if (!file) return;
	PostThreadMessage(g_uThread, THREAD_PLAY_FILE, NULL, (LPARAM)file);
	WaitForSingleObject(g_hEvent, INFINITE);
}

void SoundDB::Play_Sound_Object(ISoundObject *obj){
	Play_Sound(obj->Get_Filename());
}

void SoundDB::Play_Sound_PresetID(unsigned long ID){
	ISoundObject *o = SoundDB::Find_Sound(ID);
	if (!o) return;
	SoundDB::Play_Sound(o->Get_Filename());
}

void SoundDB::Stop_Sound(){
	PostThreadMessage(g_uThread, THREAD_PLAY_STOP, NULL, NULL);
	WaitForSingleObject(g_hEvent, INFINITE);
}

bool SoundDB::Do_Options(HKEY base_key){
	return DialogBoxParam(g_hInstance, (LPCSTR)IDD_OPTIONS, NULL, OptionsDlg, (LPARAM)base_key) == 1 ? true : false;
}

bool SoundDB::Do_Convert(){	
	SoundObject *obj = NULL;
	RawFileClass f;
	int cCount;
	ChunkLoadClass load(&f);
	ChunkSaveClass save(&f);
	DynamicVectorClass<SoundObject*>lObjs;

	char lBufI[MAX_PATH];
	char lBufO[MAX_PATH];

	if (!__OpenFile(lBufI, MAX_PATH, NULL, NULL, "preset database\0objects.ddb\0\0", "Select the input preset database file")) return false;
	if (!__SaveFile(lBufO, MAX_PATH, NULL, NULL, "Sound database\0*.sdb\0\0", "sdb", "Select the output sound database file", "sounds.sdb")) return false;

	f.Open(lBufI, 1);
	if (!f.Is_Open()) return false; 

	while (load.Open_Chunk()){
		if (load.Cur_Chunk_ID() == 0x101){
			while (load.Open_Chunk()){
				if (load.Cur_Chunk_ID() == 0x101){
					while (load.Open_Chunk()){
						if (load.Cur_Chunk_ID() == 0x00030000){
							if (obj = new SoundObject){
								obj->Load_From_ODB(load);
								lObjs.Add(obj);
							}
						}
						load.Close_Chunk();
					}
				}
				load.Close_Chunk();
			}
		}
		load.Close_Chunk();
	}
	f.Close();


	f.Open(lBufO, 2);
	if (!f.Is_Open()){
		cCount = lObjs.m_nItemCount;
		for (int i = 0; i < cCount; i++)
			delete lObjs[i];
		lObjs.Clear();
		return false;
	}
	cCount = lObjs.m_nItemCount;
	for (int i = 0; i < cCount; i++){
		save.Begin_Chunk(SND_CID);
		lObjs[i]->Save(save);
		delete lObjs[i];
		save.End_Chunk();
	}
	f.Close();
	lObjs.Clear();

	return true;
}

void SoundDB::Apply_Options(HKEY key){
	char lBuf[8192*2];
	unsigned char lBuf2[MAX_PATH];	
	int i = 0;
	unsigned long size = 8192*2, type, dsize=MAX_PATH;
	HKEY hKey;
	SoundDB::Clear_Objects();
	SoundDB::Clear_Mounts();

	if (RegOpenKeyEx(key, "sndapi\\mounts\\", 0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS) return;

	while (RegEnumValue(hKey, i++, lBuf, &size, 0, &type, lBuf2, &dsize) == ERROR_SUCCESS){
		dsize = MAX_PATH;
		size = 8192*2;
		if (type == REG_SZ){
			SoundDB::Mount_MIX((char*)lBuf2);
		}

	}
	RegCloseKey(hKey);

	if (RegOpenKeyEx(key, "sndapi\\", 0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS) return;
	dsize=MAX_PATH;

	if (RegQueryValueEx(hKey, "database", NULL, &type, lBuf2, &dsize) == ERROR_SUCCESS)
		if (type == REG_SZ)	
			SoundDB::Load_From_SoundDB((char*)lBuf2);
	
	RegCloseKey(hKey);

}

SoundDB::~SoundDB(){
	SoundDB::Clear_Objects();
	SoundDB::Clear_Mounts();
}

SoundDB g_SoundDB;

int CSoundDatabase::Get_Version(){
	return SoundDB::Get_Version();
}

ISoundObject *CSoundDatabase::Find_Sound(unsigned long preset_ID){
	return SoundDB::Find_Sound(preset_ID);
}

ISoundObject *CSoundDatabase::Get_Sound(int idx){
	return SoundDB::Get_Sound(idx);
}

void CSoundDatabase::Clear_Mounts(){
	SoundDB::Clear_Mounts();
}

void CSoundDatabase::Clear_Objects(){
	SoundDB::Clear_Objects();
}

bool CSoundDatabase::Save_To_SoundDB(char *filename){
	return SoundDB::Save_To_SoundDB(filename);
}

bool CSoundDatabase::Load_From_SoundDB(char *filename){
	return SoundDB::Load_From_SoundDB(filename);
}

bool CSoundDatabase::Load_From_ODB(char *filename){
	return SoundDB::Load_From_ODB(filename);
}

bool CSoundDatabase::Mount_MIX(char *filename){
	return SoundDB::Mount_MIX(filename);
}

void CSoundDatabase::Play_Sound(char *filename){
	SoundDB::Play_Sound(filename);
}

void CSoundDatabase::Play_Sound_Class(FileClass *file){
	return SoundDB::Play_Sound_Class(file);
}

void CSoundDatabase::Play_Sound_Object(ISoundObject *obj){
	SoundDB::Play_Sound_Object(obj);
}

void CSoundDatabase::Play_Sound_PresetID(unsigned long ID){
	SoundDB::Play_Sound_PresetID(ID);
}

void CSoundDatabase::Stop_Sound(){
	SoundDB::Stop_Sound();
}

bool CSoundDatabase::Do_Options(HKEY base_key){
	return SoundDB::Do_Options(base_key);
}

bool CSoundDatabase::Do_Convert(){
	return SoundDB::Do_Convert();
}

void CSoundDatabase::Apply_Options(HKEY base_key){
	 SoundDB::Apply_Options(base_key);
}

CSoundDatabase g_CSoundDB;

ISoundDatabase *Get_IF(){
	return (ISoundDatabase*)&g_CSoundDB;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved){
	switch (fdwReason){
		case DLL_PROCESS_ATTACH:
			g_hInstance = hinstDLL;
			g_hThread = CreateThread(NULL, 0, SoundThread, 0, NULL, &g_uThread);
			g_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
			break;
		case DLL_PROCESS_DETACH:
			TerminateThread(g_hThread, 0);
			CloseHandle(g_hThread);
			CloseHandle(g_hEvent);
			break;
		default:
			break;
	}
	return TRUE;
}
