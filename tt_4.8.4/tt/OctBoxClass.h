#ifndef TT_INCLUDE__OCTBOXCLASS_H
#define TT_INCLUDE__OCTBOXCLASS_H



#include "CastResultStruct.h"
#include "Matrix3.h"
#include "scripts.h"
#include "engine_vector.h"
#include "AABoxClass.h"
#include "OBBoxClass.h"


class RigidBodyClass;
class PhysClass;



class OctBoxClass
{

public:

   RigidBodyClass& body; // 0000
   Matrix3 o4; // 0004
   AABoxClass innerBox; // 0028
   float   f64; // 0040
   float   f68; // 0044
   float   f72; // 0048
   Vector3 o76; // 004C
   Vector3 o88; // 0058
   OBBoxClass       box; // 0064
   int              contactCount; // 00A0
   CastResultStruct contacts[16];// 00A4
   PhysClass*       contactPhysics[16]; // 0324


   OctBoxClass ();
   OctBoxClass (const OctBoxClass& object);

   void                    Compute_Contacts       (bool);
   void                    Compute_Octant_Contact (uint32, bool);
   const CastResultStruct& Get_Contact            (int);
   bool                    Is_Intersecting        (MultiListClass<PhysClass>*, bool, bool);
   int                     Get_Contact_Count      () {return contactCount;}

   OctBoxClass& operator = (const OctBoxClass& object);

}; // 868

#endif
