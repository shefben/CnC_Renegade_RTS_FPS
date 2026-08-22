#ifndef TT_INCLUDE__SIMPLEPERSISTFACTORYCLASS_H
#define TT_INCLUDE__SIMPLEPERSISTFACTORYCLASS_H

#include "PersistFactoryClass.h"


template <class T, uint32 I>
class SimplePersistFactoryClass : public PersistFactoryClass
{

public:

   // TODO: ctor

   virtual ~SimplePersistFactoryClass()
   {
   }


   virtual uint32 Chunk_ID () const
   {
      return I;
   }


   virtual PersistClass* Load
      (ChunkLoadClass& chunkLoader) const
   {
      return 0;
   }


   virtual void Save
      (ChunkSaveClass& chunkSaver, PersistClass*) const
   {
   }

};


#endif
