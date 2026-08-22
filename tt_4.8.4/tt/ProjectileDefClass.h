#ifndef TT_INCLUDE__PROJECTILEDEFCLASS_H
#define TT_INCLUDE__PROJECTILEDEFCLASS_H

#include "MoveablePhysDefClass.h"


class ProjectileDefClass : public MoveablePhysDefClass
{
public:
   bool collidesOnMove;
   uint32 orientationMode;
   Vector3 tumbleAxis;
   float tumbleRate;
   float lifetime;
   uint32 bounceCount;
   virtual ~ProjectileDefClass();
   virtual const PersistFactoryClass& Get_Factory                  () const;
   virtual bool                       Save                         (ChunkSaveClass& oSave);
   virtual bool                       Load                         (ChunkLoadClass& oLoad);
   virtual uint32                     Get_Class_ID                 () const;
   virtual PersistClass* Create          () const;
   virtual const char *               Get_Type_Name                ();
   virtual bool                       Is_Type                      (const char *);

}; // 36


#endif
