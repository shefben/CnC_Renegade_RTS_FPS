#ifndef TT_INCLUDE__BUILDINGAGGREGATEDEFCLASS_H
#define TT_INCLUDE__BUILDINGAGGREGATEDEFCLASS_H

#include "BuildingAggregateClass.h"
#include "StaticAnimPhysDefClass.h"


class BuildingAggregateDefClass : public StaticAnimPhysDefClass
{

public:

   static BuildingAggregateDefClass* repairBayAnimation;


private:

   uint32 mode;     // 116
   bool   terminal; // 120
   sint32 frame0[10]; //124
   sint32 frame1[10]; //164
   bool   animationEnabled[10]; //204


public:
   ~BuildingAggregateDefClass();
   virtual const PersistFactoryClass& Get_Factory                  () const;
   virtual bool                       Save                         (ChunkSaveClass& oSave);
   virtual bool                       Load                         (ChunkLoadClass& oLoad);
   virtual uint32                     Get_Class_ID                 () const;
   virtual PersistClass* Create          () const;
   virtual const char *               Get_Type_Name                ();
   virtual bool                       Is_Type                      (const char *);
   void Load_State_Animation_Data (ChunkLoadClass& chunkLoader, uint32 stateIndex);
   bool isTerminal() const;

}; // RH7 216, RH8 214


#endif
