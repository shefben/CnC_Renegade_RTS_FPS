#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "fileclass.h"
#include "chunkclasses.h"
#include "widestringclass.h"
#include "VectorAndListTemplateClasses.h"
#include "SaveLoadSystem.h"
#include "Translation.h"

int g_Categories = 0;
int g_Category_Containers = 0;
int g_Objects = 0;

char *stristr(char *str, char *substr){
	char *orgstr = str;
	while (*str){
		if (strnicmp(str, substr, strlen(substr)) == 0)
			return str;		
		str++;
	}
	return NULL;
}

wchar_t *wcsistr(wchar_t *str, wchar_t *substr){
	if (!*str)
		return NULL;
	while (*str){
		if (wcsnicmp(str, substr, wcslen(substr)) == 0)
			return str;
		str++;
	}
	return NULL;
}


void Sub_Load_(ChunkLoadClass &load, PersistClass *cls){
	while (load.Open_Chunk()){
		if (load.Cur_Chunk_ID() == 0x100101)
			cls->Load(load);
		load.Close_Chunk();
	}
}

void Sub_Save_(ChunkSaveClass &save, PersistClass *cls){
	save.Begin_Chunk(0x100100);
	save.Write(&cls, 4);
	save.End_Chunk();
	
	save.Begin_Chunk(0x100101);
	cls->Save(save);
	save.End_Chunk();
}

TDBCategoryClass::TDBCategoryClass(){
	m_ID = 0;
	g_Categories++;
}

TDBCategoryClass::TDBCategoryClass(TDBCategoryClass const &src){
	g_Categories++;
	*this = src;
}

TDBCategoryClass::~TDBCategoryClass(){
	g_Categories--;
}

void *TDBCategoryClass::Get_Factory(){
	return NULL;
}

bool TDBCategoryClass::Save(ChunkSaveClass &save){
	save.Begin_Chunk(0x11221023);
	save.End_Chunk();
	save.Begin_Chunk(0x11221022);
	Save_Variables(save);
	save.End_Chunk();	
	return true;
}

bool TDBCategoryClass::Load(ChunkLoadClass &load){
	while (load.Open_Chunk()){
		if (load.Cur_Chunk_ID() == 0x11221022)
			Load_Variables(load);
		load.Close_Chunk();
	}
	return true;
}

TDBCategoryClass &TDBCategoryClass::operator = (const TDBCategoryClass &src){
	m_ID = src.m_ID;
	m_Name = src.m_Name;	
	return *this;
}

void TDBCategoryClass::Save_Variables(ChunkSaveClass &save){
	save.Begin_Micro_Chunk(1);
	save.Write(&m_ID, 4);
	save.End_Micro_Chunk();
	save.Begin_Micro_Chunk(2);
	save.Write(m_Name(), (int)strlen(m_Name())+1);
	save.End_Micro_Chunk();
}

void TDBCategoryClass::Load_Variables(ChunkLoadClass &load){
	while (load.Open_Mircro_Chunk()){
		if (load.Current_Micro_Chunk_ID() == 1)
			load.Read(&m_ID, 4);
		else if (load.Current_Micro_Chunk_ID() == 2){
			m_Name.Uninitialised_Grow(load.Current_Micro_Chunk_Length());
			load.Read(m_Name.m_string, load.Current_Micro_Chunk_Length());
		}
		load.Close_Mircro_Chunk();
	}
}

TDBCategoryClass *TDBCategoryClass::Clone(){
	return new TDBCategoryClass(*this);
}

char *TDBCategoryClass::Get_Name(){
	return m_Name();
}

void TDBCategoryClass::Set_Name(char const *name){
	m_Name.SetString(name);
}

unsigned long TDBCategoryClass::Get_ID(){
	return m_ID;
}

void TDBCategoryClass::Set_ID(unsigned long ID){
	m_ID = ID;
}

/*-- TDBObject --*/

TDBObjClass::TDBObjClass(){
	m_ID = 0;
	m_Sound_ID = -1;
	m_Category_ID = 0;
	g_Objects++;
}

TDBObjClass::TDBObjClass(TDBObjClass &src){
	g_Objects++;
	*this = src;
}

TDBObjClass::~TDBObjClass(){
	g_Objects--;
}

void *TDBObjClass::Get_Factory(){
	return NULL;
}

bool TDBObjClass::Save(ChunkSaveClass &save){
	register int temp;
	save.Begin_Chunk(0x6141109);
	save.End_Chunk();

	save.Begin_Chunk(0x6141108);
	Save_Variables(save);
	save.End_Chunk();

	save.Begin_Chunk(0x614110A);
	save.Write(m_English_String(), (int)strlen(m_English_String())*1);
	save.End_Chunk();

	if (TDataBase::m_Is_Single_Language_Export){
		temp = TDataBase::m_Language_ID;
		if (temp >= 0 && temp < m_Strings.m_nItemCount){
			save.Begin_Chunk(0x614110B);
			save.Write(m_Strings[temp](), (int)wcslen(m_Strings[temp]())*2+2);
			save.End_Chunk();
		}
	} else {
		for (int i = 0; i < m_Strings.m_nItemCount; i++){
			save.Begin_Chunk(0x614110B);
			save.Write(m_Strings[i](), (int)wcslen(m_Strings[i]())*2+2);
			save.End_Chunk();
		}
	}
	return true;
}

bool TDBObjClass::Load(ChunkLoadClass &load){

	WideStringClass str;
	while (load.Open_Chunk()){
		switch (load.Cur_Chunk_ID()){
			case 0x6141108:
				Load_Variables(load);
				break;
			case 0x614110A:
				m_English_String.Uninitialised_Grow(load.Cur_Chunk_Length());
				load.Read(m_English_String(), load.Cur_Chunk_Length());
				break;
			case 0x614110B:
				str.Uninitialised_Grow(load.Cur_Chunk_Length() >> 1);
				load.Read(str(), load.Cur_Chunk_Length());
				str.Convert_From_2Byte();
				m_Strings.Add(str);
				break;
			default:
				break;
		}
		load.Close_Chunk();
	}
	return true;
}

void *TDBObjClass::As_StringTwiddlerClass(){
	return NULL;
}

TDBObjClass *TDBObjClass::Clone(){
	return NULL;
}

wchar_t *TDBObjClass::Get_String(unsigned long idx){
	if (idx >= 0 && idx < (unsigned long)m_Strings.m_nItemCount)
		return m_Strings[idx]();
	return L"";
}

wchar_t *TDBObjClass::Get_String(){
	return Get_String(0);
}

char *TDBObjClass::Get_English_String(){
	return m_English_String();
}

unsigned long TDBObjClass::Get_ID(){
	return m_ID;
}

char *TDBObjClass::Get_ID_Desc(){
	return m_ID_Description();
}

unsigned long TDBObjClass::Get_Sound_ID(){
	return m_Sound_ID;
}

char *TDBObjClass::Get_Animation_Name(){
	return m_Animation_Name();
}

int TDBObjClass::Get_Category_ID(){
	return m_Category_ID;
}

void TDBObjClass::Set_String(unsigned long idx, const wchar_t *string){
	WideStringClass wstr(0, false);
	if (!string || idx < 0) return;
	while ((int)idx >= m_Strings.m_nItemCount)
		m_Strings.Add(wstr);
	m_Strings[idx].SetString((wchar_t*)string);

}

void TDBObjClass::Set_English_String(const char *string){
	m_English_String.SetString(string);
}

void TDBObjClass::Set_ID(unsigned long ID){
	m_ID = ID;
}

void TDBObjClass::Set_ID_Desc(char const *desc){
	m_ID_Description.SetString(desc);
}

void TDBObjClass::Set_Animation_Name(char const *anim){
	m_Animation_Name.SetString(anim);
}

void TDBObjClass::Set_Sound_ID(unsigned long ID){
	m_Sound_ID = ID;
}

void TDBObjClass::Set_Category_ID(unsigned long ID){
	m_Category_ID = ID;
}

bool TDBObjClass::Contains_Translation(unsigned long idx){
	if (idx < 0 || idx >= (unsigned long)m_Strings.m_nItemCount) return false;
	return wcslen(m_Strings[idx]()) ? true : false;
}

TDBObjClass &TDBObjClass::operator = (const TDBObjClass &as){
	return *this;
}

void TDBObjClass::Save_Variables(ChunkSaveClass &save){
	save.Begin_Micro_Chunk(1);
	save.Write(&m_ID, 4);
	save.End_Micro_Chunk();

	save.Begin_Micro_Chunk(5);
	save.Write(&m_Sound_ID, 4);
	save.End_Micro_Chunk();

	save.Begin_Micro_Chunk(7);
	save.Write(&m_Category_ID, 4);
	save.End_Micro_Chunk();

	save.Begin_Chunk(2);
	save.Write(m_ID_Description(), (int)strlen(m_ID_Description())+1);
	save.End_Micro_Chunk();

	save.Begin_Micro_Chunk(6);
	save.Write(m_Animation_Name(), (int)strlen(m_Animation_Name())+1);
	save.End_Micro_Chunk();


}

void TDBObjClass::Load_Variables(ChunkLoadClass &load){
	WideStringClass wstr;
	while (load.Open_Mircro_Chunk()){
		switch (load.Current_Micro_Chunk_ID()){
			case 1:
				load.Read(&m_ID, 4);
				break;
			case 2:
				m_ID_Description.Uninitialised_Grow(load.Current_Micro_Chunk_Length());
				load.Read(m_ID_Description(), load.Current_Micro_Chunk_Length());
				break;
			case 3:
				wstr.Uninitialised_Grow(load.Current_Micro_Chunk_Length()>>2);
				load.Read(wstr(), load.Current_Micro_Chunk_Length());
				m_Strings.Add(wstr);
				break;
			case 4:
				m_English_String.Uninitialised_Grow(load.Current_Micro_Chunk_Length());
				load.Read(m_English_String(), load.Current_Micro_Chunk_Length());
				break;
			case 5:
				load.Read(&m_Sound_ID, 4);
				break;
			case 6:
				m_Animation_Name.Uninitialised_Grow(load.Current_Micro_Chunk_Length());
				load.Read(m_Animation_Name(), load.Current_Micro_Chunk_Length());
				break;
			case 7:
				load.Read(&m_Category_ID, 4);
				break;
			default:
				break;
		}
		load.Close_Mircro_Chunk();
	}
}

void CatergoryContainer::Build(TDBCategoryClass *cat){
	Free();
	m_This = cat;
	for (int i = 0; i < TDataBase::m_Objects.m_nItemCount; i++)
		if (TDataBase::m_Objects[i]->Get_Category_ID() == m_This->Get_ID())
			m_Objects.Add(TDataBase::m_Objects[i]);
}
void CatergoryContainer::Kill_Category(){
	for (int i = 0; i < m_Objects.m_nItemCount; i++){
		m_Objects[i]->Set_Category_ID(0);
		TDataBase::m_Containers[0]->m_Objects.Add(m_Objects[i]);
	}
	int j = TDataBase::m_Categories.ID(m_This);
	if (j == -1) return;
	TDataBase::m_Categories.Delete(j);
	delete m_This;	
}


CatergoryContainer::CatergoryContainer(){
	g_Category_Containers++;
}

CatergoryContainer::~CatergoryContainer(){
	g_Category_Containers--;
}


bool TDataBase::m_Is_Single_Language_Export = true;
int TDataBase::m_Language_ID = 0;
int TDataBase::m_Version = 0;
DynamicVectorClass<TDBObjClass*> TDataBase::m_Objects;
DynamicVectorClass<TDBCategoryClass*> TDataBase::m_Categories;
DynamicVectorClass<unsigned long> TDataBase::m_Hashes;
DynamicVectorClass<CatergoryContainer*> TDataBase::m_Containers;


bool TDataBase::Load(ChunkLoadClass &load){
	while (load.Open_Chunk()){
		if (load.Cur_Chunk_ID() == 0x00090000)
			Load_Sub(load);
		load.Close_Chunk();
	}
	return true;
}
bool TDataBase::Save(ChunkSaveClass &save){
	save.Begin_Chunk(0x00090000);
	Save_Sub(save);
	save.End_Chunk();
	return true;
}

bool TDataBase::Load_Sub(ChunkLoadClass &load){
	while (load.Open_Chunk()){
		switch (load.Cur_Chunk_ID()){
			case 0x07141200:
				Load_Variables(load);
				break;
			case 0x07141201:
				Load_Objects(load);
				break;
			case 0x07141202:
				Load_Categories(load);
				break;
			default:
				break;
		}
		load.Close_Chunk();
	}
	return true;
}
bool TDataBase::Save_Sub(ChunkSaveClass &save){
	save.Begin_Chunk(0x07141200);
	Save_Variables(save);
	save.End_Chunk();

	save.Begin_Chunk(0x07141201);
	Save_Objects(save);
	save.End_Chunk();

	save.Begin_Chunk(0x07141202);
	Save_Categories(save);
	save.End_Chunk();
	return true;
}

bool TDataBase::Load_Variables(ChunkLoadClass &load){
	while (load.Open_Mircro_Chunk()){
		switch (load.Current_Micro_Chunk_ID()){
			case 1:
				load.Read(&m_Version, 4);
				break;
			case 2:
				load.Read(&m_Language_ID, 4);
				break;
			default:
				break;
		}
		load.Close_Mircro_Chunk();
	}	
	return true;
}

bool TDataBase::Save_Variables(ChunkSaveClass &save){
	save.Begin_Micro_Chunk(1);
	save.Write(&m_Version, 4);
	save.End_Chunk();
	save.Begin_Micro_Chunk(2);
	save.Write(&m_Language_ID, 4);
	save.End_Chunk();
	return true;
}

bool TDataBase::Load_Objects(ChunkLoadClass &load){
	TDBObjClass *temp = NULL;
	while (load.Open_Chunk()){
		if (load.Cur_Chunk_ID() == 0x00090001){
			temp = new TDBObjClass();
			if (temp){
				Sub_Load_(load, temp);
				m_Objects.Add(temp);
			}
		}
		load.Close_Chunk();
	}
	return true;
}

bool TDataBase::Save_Objects(ChunkSaveClass &save){
	for (int i = 0; i < m_Objects.m_nItemCount; i++){
		save.Begin_Chunk(0x00090001);
		Sub_Save_(save, m_Objects[i]);
		save.End_Chunk();
	}
	return true;
}

bool TDataBase::Load_Categories(ChunkLoadClass &load){
	TDBCategoryClass *temp;
	while (load.Open_Chunk()){
		if (load.Cur_Chunk_ID() == 0x00090002){
			temp = new TDBCategoryClass();
			if (temp){
				Sub_Load_(load, temp);
				m_Categories.Add(temp);
			}
		}
		load.Close_Chunk();
	}
	return true;
}

bool TDataBase::Save_Categories(ChunkSaveClass &save){
	for (int i = 0; i < m_Categories.m_nItemCount; i++){
		save.Begin_Chunk(0x00090001);
		Sub_Save_(save, m_Categories[i]);
		save.End_Chunk();
	}
	return true;
}

void TDataBase::Free_All(){
	int i;
	for (i = 0; i < m_Objects.m_nItemCount; i++)
		delete m_Objects[i];
	
	for (i = 0; i < m_Categories.m_nItemCount; i++)
		delete m_Categories[i];
	
	for (i = 0; i < m_Containers.m_nItemCount; i++)
		delete m_Containers[i];

	m_Objects.Clear();
	m_Categories.Clear();
	m_Containers.Clear();
	m_Hashes.Clear();
}

void TDataBase::Build(){
	CatergoryContainer *cnt;
	for (int i = 0; i < m_Categories.m_nItemCount; i++){
		cnt = new CatergoryContainer();
		if (!cnt) continue;
		cnt->Build(m_Categories[i]);
		m_Containers.Add(cnt);
	}
}

void TDataBase::Kill_Category(int ID){
	int i;
	for (i = 0; i < m_Containers.m_nItemCount; i++){
		if (ID == m_Containers[i]->m_This->Get_ID()){
			m_Containers[i]->Kill_Category();
			delete m_Containers[i];
			m_Containers.Delete(i);
			Build();
			return;
		}
	}
}