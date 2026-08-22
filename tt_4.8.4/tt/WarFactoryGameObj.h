#ifndef TT_INCLUDE__WARFACTORYGAMEOBJ_H
#define TT_INCLUDE__WARFACTORYGAMEOBJ_H

#include "VehicleFactoryGameObj.h"

class WarFactoryGameObjDef;
class WarFactoryGameObj : public VehicleFactoryGameObj
{
private:
   sint32 staticObjectID;
   float timer;
public:
   WarFactoryGameObj();
   virtual ~WarFactoryGameObj();
   virtual const PersistFactoryClass& Get_Factory() const;
   virtual bool              Save                (ChunkSaveClass& chunkSaver);
   virtual bool              Load                (ChunkLoadClass& chunkLoader);
   virtual void              Init                ();
   virtual void              Think               ();
   virtual WarFactoryGameObj* As_WarFactoryGameObj() {return this;}
   virtual void              CnC_Initialize      (BaseControllerClass* base);
   virtual void Begin_Generation();
   void Play_Creation_Animation(bool bPlay);
   const WarFactoryGameObjDef & Get_Definition( void ) const ;
};


#endif
