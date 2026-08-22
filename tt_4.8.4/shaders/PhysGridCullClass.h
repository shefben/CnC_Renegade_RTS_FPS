#ifndef TT_INCLUDE__PHYSGRIDCULLCLASS_H
#define TT_INCLUDE__PHYSGRIDCULLCLASS_H



#include "CullSystemClass.h"
class ChunkLoadClass;
class ChunkSaveClass;
class PhysClass;

class PhysGridCullClass :
	public CullSystemClass
{

public:
	virtual void Re_Partition(Vector3  const&, Vector3  const&, float);
	virtual void Load(ChunkLoadClass &);
	virtual void Save(ChunkSaveClass &);
	virtual int Get_Object_Count();
	virtual void Add_Object(PhysClass*);
	virtual void Remove_Object(PhysClass*);
};



#endif