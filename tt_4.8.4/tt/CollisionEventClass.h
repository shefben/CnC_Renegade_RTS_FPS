#ifndef TT_INCLUDE__COLLISIONEVENTCLASS_H
#define TT_INCLUDE__COLLISIONEVENTCLASS_H
#include "CastResultStruct.h"
class PhysClass;
class RenderObjClass;
class CollisionEventClass
{

public:

   PhysClass*        physics;    // 0
   CastResultStruct* castResult; // 4
   RenderObjClass*   bone;       // 8

};


#endif
