#include "general.h"

#include "OctBoxClass.h"
#include "LineSegClass.h"
#include "physcoltest.h"
#include "PhysicsSceneClass.h"
#include "RigidBodyClass.h"
#include "TexProjectClass.h"


Vector3 OBBoxClass::getWorldEdge
   (uint edgeNumber)
{
   static const Vector3 _octantOffsets[] =
   {
      Vector3 ( 1.0f,  1.0f,  1.0f),
      Vector3 (-1.0f,  1.0f,  1.0f),
      Vector3 (-1.0f, -1.0f,  1.0f),
      Vector3 ( 1.0f, -1.0f,  1.0f),
      Vector3 ( 1.0f,  1.0f, -1.0f),
      Vector3 (-1.0f,  1.0f, -1.0f),
      Vector3 (-1.0f, -1.0f, -1.0f),
      Vector3 ( 1.0f, -1.0f, -1.0f),
   };
   Vector3 v = _octantOffsets[edgeNumber];
   v.Scale(this->Extent);
   return this->Center + this->Basis * v;
}

RENEGADE_FUNCTION
void OctBoxClass::Compute_Contacts
   (bool)
   AT2(0x0066CF60,0x0066C800);



void OctBoxClass::Compute_Octant_Contact
   (uint32 edgeNumber, bool arg2)
{
   static const Vector3 _octantOffsets[] =
   {
      Vector3 ( 1.0f,  1.0f,  1.0f),
      Vector3 (-1.0f,  1.0f,  1.0f),
      Vector3 (-1.0f, -1.0f,  1.0f),
      Vector3 ( 1.0f, -1.0f,  1.0f),
      Vector3 ( 1.0f,  1.0f, -1.0f),
      Vector3 (-1.0f,  1.0f, -1.0f),
      Vector3 (-1.0f, -1.0f, -1.0f),
      Vector3 ( 1.0f, -1.0f, -1.0f),
   };

   const Vector3& offsets = _octantOffsets[edgeNumber];


   Vector3 edgePosition  = this->box.getWorldEdge (edgeNumber);
   Vector3 edgeDirection = this->box.Basis * (offsets * this->f64);


   CastResultStruct result1;
   // LineSegClass line (edgePosition, edgePosition + this->box.rotation.applyTo (edgeDirection));
   LineSegClass line (edgePosition, edgePosition + edgeDirection);
   PhysRayCollisionTestClass test1 (line, &result1, 0x00000022);

   result1.ComputeContactPoint = true;
   test1.CollisionGroup = this->body.Get_Collision_Group();

   PhysicsSceneClass::Get_Instance()->Cast_Ray (test1, true);


   CastResultStruct result2;
   Vector3 v = offsets;
   v.Scale(this->o76);
   OBBoxClass box (this->box.Center + this->box.Basis * v, this->o88,this->box.Basis);
   PhysOBBoxCollisionTestClass test2 (box, edgeDirection, &result2, 0x00000022);
   
   result2.ComputeContactPoint = true;
   test2.CollisionGroup = this->body.Get_Collision_Group();

   PhysicsSceneClass::Get_Instance()->Cast_OBBox (test2, true);

   if (test2.CollidedPhysObj && test2.CollidedPhysObj->As_RigidBodyClass())
      test2.CollidedPhysObj->Flags &= ~0x00000080;

   bool use1 = (!result1.StartBad && result1.Fraction <= 1.0f && result1.Normal.Length2() > 0);
   bool use2 = (!result2.StartBad && result2.Fraction <= 1.0f && result2.Normal.Length2() > 0);

   if (use1 && use2 && result1.Fraction - 0.25f < result2.Fraction)
      use2 = false;

   if (use2)
   {
      this->contacts[this->contactCount]       = result2;
      this->contactPhysics[this->contactCount] = test2.CollidedPhysObj;

      ++this->contactCount;
   }

   if (use1)
   {
      this->contacts[this->contactCount]       = result1;
      this->contactPhysics[this->contactCount] = test1.CollidedPhysObj;

      ++this->contactCount;
   }
}



const CastResultStruct& OctBoxClass::Get_Contact
   (int index)
{
   static const CastResultStruct _noResult;

   if ((index >= this->contactCount) || (index >= 16))
      return _noResult;

   return this->contacts[index];
}



RENEGADE_FUNCTION
bool OctBoxClass::Is_Intersecting
   (MultiListClass<PhysClass>*, bool, bool)
   AT2(0x0066CCF0,0x0066C590);
