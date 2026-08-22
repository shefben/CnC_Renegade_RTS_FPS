#ifndef TT_INCLUDE__BULLETCLASS_H
#define TT_INCLUDE__BULLETCLASS_H

#include "BulletDataClass.h"
#include "engine_vector.h"
#include "NetworkObjectClass.h"
#include "ScritpableGameObj.h"
#include "PostLoadableClass.h"
class ProjectileClass;
class BulletClass : public CombatPhysObserverClass, public MultiListObjectClass, public PostLoadableClass
{

public:

   BulletDataClass  data;       // 20
   ProjectileClass* projectile; // 80
   Vector3          bullet_target;
   ReferencerClass  target_obj;
   sint32           time;
   Vector3          o116;
   sint32           ModelCRC;


public:

   virtual CollisionReactionType Collision_Occurred (const CollisionEventClass& collisionEvent);
   virtual int Object_Expired     (PhysClass* physics);

   void Init          (const BulletDataClass&, float, const Vector3&, DamageableGameObj*);
   void Shutdown      ();
   void Think         ();

}; // 132


#endif
