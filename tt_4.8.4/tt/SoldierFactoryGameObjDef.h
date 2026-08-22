#ifndef TT_INCLUDE__SOLDIERFACTORYGAMEOBJDEF_H
#define TT_INCLUDE__SOLDIERFACTORYGAMEOBJDEF_H

#include "BuildingGameObjDef.h"
#include "SoldierFactoryGameObj.h"


class SoldierFactoryGameObjDef : public BuildingGameObjDef
{
public:
   SoldierFactoryGameObjDef();
   virtual ~SoldierFactoryGameObjDef();
   virtual const PersistFactoryClass& Get_Factory  () const;
   virtual bool                       Save         (ChunkSaveClass& chunkSaver);
   virtual bool                       Load         (ChunkLoadClass& chunkLoader);
   virtual uint32                     Get_Class_ID () const;
   virtual SoldierFactoryGameObj*     Create       ();
};

#endif
