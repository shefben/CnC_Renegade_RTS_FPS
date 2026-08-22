#ifndef TT_INCLUDE__SOLDIERFACTORYGAMEOBJ_H
#define TT_INCLUDE__SOLDIERFACTORYGAMEOBJ_H

#include "BuildingGameObj.h"

class SoldierFactoryGameObjDef;
class SoldierFactoryGameObj : public BuildingGameObj
{

public:
   SoldierFactoryGameObj();
   virtual ~SoldierFactoryGameObj();
   virtual const PersistFactoryClass& Get_Factory  () const;
   virtual bool              Save                (ChunkSaveClass& chunkSaver);
   virtual bool              Load                (ChunkLoadClass& chunkLoader);
   virtual void              Init                ();
   virtual SoldierFactoryGameObj* As_SoldierFactoryGameObj () {return this;}
   virtual void              CnC_Initialize      (BaseControllerClass* base);
   virtual void On_Destroyed             ();
   void Init(const SoldierFactoryGameObjDef &);
};


#endif
