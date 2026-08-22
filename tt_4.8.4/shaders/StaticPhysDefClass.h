#ifndef TT_INCLUDE__STATICPHYSDEFCLASS_H
#define TT_INCLUDE__STATICPHYSDEFCLASS_H

#include "PhysDefClass.h"


class StaticPhysDefClass : public PhysDefClass
{

private:

   bool isNonOccluder;
public:
   virtual ~StaticPhysDefClass();
   virtual const PersistFactoryClass& Get_Factory                  () const;
   virtual bool                       Save                         (ChunkSaveClass& oSave);
   virtual bool                       Load                         (ChunkLoadClass& oLoad);
   virtual uint32                     Get_Class_ID                 () const;
   virtual PersistClass* Create          () const;
   virtual const char *               Get_Type_Name                ();
   virtual bool                       Is_Type                      (const char *);

}; // RH7: 40


#endif
