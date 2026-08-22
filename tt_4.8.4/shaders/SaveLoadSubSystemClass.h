#ifndef TT_INCLUDE__SAVELOADSUBSYSTEMCLASS_H
#define TT_INCLUDE__SAVELOADSUBSYSTEMCLASS_H

#include "PostLoadableClass.h"
#include "scripts.h"
#include "engine_vector.h"
#include "engine_io.h"

class SaveLoadSubSystemClass : public PostLoadableClass
{

public:

   sint32 s8;
   SaveLoadSubSystemClass();
   ~SaveLoadSubSystemClass();
   virtual uint32      Chunk_ID      () const = 0;
   virtual bool        Contains_Data () const;
   virtual bool        Save          (ChunkSaveClass& oSave) = 0;
   virtual bool        Load          (ChunkLoadClass& oLoad) = 0;
   virtual const char* Name          () const = 0;

}; // size: 12


#endif
