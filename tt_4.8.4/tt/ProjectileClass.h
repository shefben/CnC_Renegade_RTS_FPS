#ifndef TT_INCLUDE__PROJECTILECLASS_H
#define TT_INCLUDE__PROJECTILECLASS_H

#include "MoveablePhysClass.h"
#include "ProjectileDefClass.h"


class ProjectileClass :
	public MoveablePhysClass
{

private:

   Vector3 position;        // 00C0  00C0  00DC  00D8
   Vector3 velocity;        // ____  ____  ____  00E4
   bool    collidesOnMove;  // ____  ____  ____  00F0
   uint32  orientationMode; // ____  ____  ____  00F4
   Vector3 angularVelocity; // ____  ____  ____  00F8
   float   f264;            // ____  ____  ____  0104
   float   lifeTime;        // ____  ____  ____  0108
   sint32  bounceCount;     // ____  ____  ____  010C


public:

            ProjectileClass();
   virtual                            ~ProjectileClass             ();
   virtual void                       Timestep                     (float);
   virtual const Matrix3D&            Get_Transform                () const;
   virtual void                       Set_Transform                (const Matrix3D&);
   virtual bool                       Cast_Ray                     (PhysRayCollisionTestClass&);
   virtual ProjectileClass*           As_ProjectileClass           () {return this;}
   virtual bool                       Save                         (ChunkSaveClass& oSave);
   virtual bool                       Load                         (ChunkLoadClass& oLoad);
   virtual void					Get_Velocity(Vector3 * set_vel) const;
   virtual void                       Set_Velocity                 (const Vector3&);
   virtual const AABoxClass&          Get_Bounding_Box             () const;
   virtual void                       Get_Angular_Velocity         (Vector3& target) const;
   virtual void                       Set_Angular_Velocity         (const Vector3&);
   virtual void                       Set_Lifetime                 (float);
   virtual float                      Get_Lifetime                 ();
   virtual void                       Set_Bounce_Count             (uint32);
   virtual sint32                     Get_Bounce_Count             ();
   virtual void                       Apply_Impulse                (const Vector3&);
   virtual void                       Apply_Impulse                (const Vector3&,const Vector3&);
   virtual void                       Set_Collides_On_Move         (bool);
   virtual const PersistFactoryClass& Get_Factory                  () const;

}; // ____  ____  ____  0110

#endif
