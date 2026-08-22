#ifndef TT_INCLUDE__EXPLOSIONMANAGER_H
#define TT_INCLUDE__EXPLOSIONMANAGER_H
#include "Vector3.h"
#include "Matrix3D.h"
class BuildingGameObj;
class ArmedGameObj;
class ExplosionManager
{

public:

   static void Create_Explosion_At       (uint32 explosionId, const Vector3& position, ArmedGameObj* damager, const Vector3& direction, DamageableGameObj* fullDamageTarget);
   static void Create_Explosion_At       (uint32 explosionId, const Matrix3D& transform, ArmedGameObj* damager, const Vector3& direction, DamageableGameObj* fullDamageTarget);
   static void Explosion_Damage_Building (sint32, BuildingGameObj*, bool, ArmedGameObj*);
   static void Server_Explode            (sint32, const Vector3& position, sint32, DamageableGameObj*);
};

#endif
