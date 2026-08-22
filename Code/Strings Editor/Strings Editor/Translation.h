#ifndef _TDBOBJ_H_
#define _TDBOBJ_H_
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "fileclass.h"
#include "chunkclasses.h"
#include "widestringclass.h"
#include "VectorAndListTemplateClasses.h"
#include "SaveLoadSystem.h"
#include "LList.h"

extern int g_Categories;
extern int g_Category_Containers;
extern int g_Objects;

void Sub_Load_(ChunkLoadClass &load, PersistClass *cls);
void Sub_Save_(ChunkSaveClass &save, PersistClass *cls);
char *stristr(char *str, char *substr);
wchar_t *wcsistr(wchar_t *str, wchar_t *substr);

class TDBCategoryClass : public PersistClass{
private:
	StringClass m_Name;
	unsigned long m_ID;
public:
	TDBCategoryClass();
	TDBCategoryClass(TDBCategoryClass const &src);

	virtual ~TDBCategoryClass();
	virtual void *Get_Factory();
	virtual bool Save(ChunkSaveClass &save);
	virtual bool Load(ChunkLoadClass &load);

	TDBCategoryClass &operator = (const TDBCategoryClass &src);
	void Save_Variables(ChunkSaveClass &save);
	void Load_Variables(ChunkLoadClass &load);
	TDBCategoryClass *Clone();
	char *Get_Name();
	void Set_Name(char const *name);
	unsigned long Get_ID();
	void Set_ID(unsigned long ID);
};

class TDBObjClass : public PersistClass{
private:
	StringClass m_English_String;	//0x08
	StringClass m_ID_Description;	//0x0C
	unsigned long m_ID;				//0x10
	unsigned long m_Sound_ID;		//0x14
	int m_Category_ID;				//0x18
	StringClass m_Animation_Name;	//0x1C
	DynamicVectorClass<WideStringClass>m_Strings; //0x20
public:
	TDBObjClass();
	TDBObjClass(TDBObjClass &src);

	/*-- Virtual crap --*/
	virtual ~TDBObjClass(); //0x08
	//post load, 0x0C
	virtual void *Get_Factory(); //10
	virtual bool Save(ChunkSaveClass &save); //0x14
	virtual bool Load(ChunkLoadClass &load); //0x18
	virtual void *As_StringTwiddlerClass(); //0x1C
	virtual TDBObjClass *Clone(); //0x20
	virtual wchar_t *Get_String(unsigned long idx); //0x24
	virtual wchar_t *Get_String(); //0x28
	virtual char *Get_English_String(); //0x2C
	virtual unsigned long Get_ID(); //0x30
	virtual char *Get_ID_Desc(); //0x34
	virtual unsigned long Get_Sound_ID(); //0x38
	virtual char *Get_Animation_Name(); //0x3C
	virtual int Get_Category_ID(); //0x40
	virtual void Set_String(unsigned long idx, const wchar_t *string); //0x44
	virtual void Set_English_String(const char *string); //0x48
	virtual void Set_ID(unsigned long ID); //0x4C
	virtual void Set_ID_Desc(char const *desc); //0x50
	virtual void Set_Animation_Name(char const *anim); //0x54
	virtual void Set_Sound_ID(unsigned long ID); //0x58
	virtual void Set_Category_ID(unsigned long ID); //0x5C
	virtual bool Contains_Translation(unsigned long idx); //0x60

	/*-- Other stuff --*/
	TDBObjClass &operator = (const TDBObjClass &as);
	void Save_Variables(ChunkSaveClass &save);
	void Load_Variables(ChunkLoadClass &load);
};

class CatergoryContainer{
public:
	CatergoryContainer();
	~CatergoryContainer();
	DynamicVectorClass<TDBObjClass*>m_Objects;
	TDBCategoryClass *m_This;
	__inline void Free(){ m_Objects.Clear(); };
	void Build(TDBCategoryClass *cat);
	void Kill_Category();
};

class TDataBase{
public:
	static bool m_Is_Single_Language_Export;
	static int m_Language_ID;
	static int m_Version;
	static DynamicVectorClass<TDBObjClass*>m_Objects;
	static DynamicVectorClass<TDBCategoryClass*>m_Categories;
	static DynamicVectorClass<unsigned long>m_Hashes;
	static DynamicVectorClass<CatergoryContainer*> m_Containers;

	static bool Load(ChunkLoadClass &load);
	static bool Save(ChunkSaveClass &save);

	static bool Load_Sub(ChunkLoadClass &load);
	static bool Save_Sub(ChunkSaveClass &save);

	static bool Load_Variables(ChunkLoadClass &load);
	static bool Save_Variables(ChunkSaveClass &save);

	static bool Load_Objects(ChunkLoadClass &load);
	static bool Save_Objects(ChunkSaveClass &save);

	static bool Load_Categories(ChunkLoadClass &load);
	static bool Save_Categories(ChunkSaveClass &save);

	static void Free_All();
	static void Build();
	static void Kill_Category(int ID);
};


#endif //_TDBOBJ_H_