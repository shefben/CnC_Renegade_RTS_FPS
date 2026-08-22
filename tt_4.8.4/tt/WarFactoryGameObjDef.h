#ifndef TT_INCLUDE__WARFACTORYGAMEOBJDEF_H
#define TT_INCLUDE__WARFACTORYGAMEOBJDEF_H

#include "VehicleFactoryGameObjDef.h"
#include "WarFactoryGameObj.h"


class WarFactoryGameObjDef : public VehicleFactoryGameObjDef
{
public:
            WarFactoryGameObjDef();
   virtual ~WarFactoryGameObjDef();
   virtual const PersistFactoryClass& Get_Factory  () const;
   virtual bool                       Save         (ChunkSaveClass& chunkSaver);
   virtual bool                       Load         (ChunkLoadClass& chunkLoader);
   virtual uint32                     Get_Class_ID () const;
   virtual WarFactoryGameObj*         Create       ();
   void Load_Variables (ChunkLoadClass& chunkLoader);
}; // 168

#endif
