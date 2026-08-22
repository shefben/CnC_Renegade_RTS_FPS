#ifndef TT_INCLUDE_DYNAMICANIMPHYSCLASS_H
#define TT_INCLUDE_DYNAMICANIMPHYSCLASS_H
#include "DecorationPhysClass.h"
#include "AnimCollisionManagerClass.h"
#include "MoveablePhysClass.h"

#pragma warning(disable: 4355) // 'this' : used in base member initializer list
class DynamicAnimPhysClass : public DecorationPhysClass
{
public:
	AnimCollisionManagerClass animMgr;
	DynamicShadowManagerClass shadowMgr;
	DynamicAnimPhysClass() : shadowMgr(*this)
	{
	}
};

#endif