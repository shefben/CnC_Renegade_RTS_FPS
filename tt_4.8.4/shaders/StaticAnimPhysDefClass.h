#ifndef TT_INCLUDE__STATICANIMPHYSDEFCLASS_H
#define TT_INCLUDE__STATICANIMPHYSDEFCLASS_H

#include "StaticPhysDefClass.h"
#include "ProjectorManagerClass.h"
class AnimCollisionManagerDefClass
{
	sint32 collisionMode;
	sint32 animationMode;
	StringClass animationName;
};

class StaticAnimPhysDefClass : public StaticPhysDefClass
{

public:

   bool   isCosmetic; // 40
   AnimCollisionManagerDefClass collisionDef; // 44
   ProjectorManagerDefClass projectorDef; // 56
   bool   shadowDynamicObjects; // 96
   bool   shadowIsAdditive; // 97
   bool   shadowIgnoresZRotation; // 98
   float  shadowNearZ; // 100
   float  shadowFarZ; // 104
   float  shadowIntensity; // 108
   bool   doesCollideInPathfind; // 112

   virtual ~StaticAnimPhysDefClass();
   virtual const PersistFactoryClass& Get_Factory                  () const;
   virtual bool                       Save                         (ChunkSaveClass& oSave);
   virtual bool                       Load                         (ChunkLoadClass& oLoad);
   virtual uint32                     Get_Class_ID                 () const;
   virtual PersistClass* Create          () const;
   virtual const char *               Get_Type_Name                ();
   virtual bool                       Is_Type                      (const char *);

}; // RH7: 116


#endif
