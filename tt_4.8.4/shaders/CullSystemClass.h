#ifndef TT_INCLUDE_CULLSYSTEMCLASS_H
#define TT_INCLUDE_CULLSYSTEMCLASS_H



#include "CullableClass.h"



class FrustumClass;
class OBBoxClass;
class AABoxClass;
class Vector3;



class CullSystemClass
{

private:

	CullableClass* CollectionHead;

public:

	virtual ~CullSystemClass();
	virtual void Collect_Objects(const Vector3&) = 0;
	virtual void Collect_Objects(const AABoxClass&) = 0;
	virtual void Collect_Objects(const OBBoxClass&) = 0;
	virtual void Collect_Objects(const FrustumClass&) = 0;
	virtual void Update_Culling(CullableClass*) = 0;

	void Reset_Collection()
	{
		CollectionHead = 0;
	}

	CullableClass *Get_First_Collected_Object_Internal()
	{
		return CollectionHead;
	}

	CullableClass *Get_Next_Collected_Object_Internal(CullableClass *cullable)
	{
		return cullable->Get_Next_Collected();
	}

};



#endif
