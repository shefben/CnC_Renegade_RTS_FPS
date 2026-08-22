#ifndef __PERSISTFACTORYCLASS_H__
#define __PERSISTFACTORYCLASS_H__
#include "scripts.h"
#include "engine_common.h"
#include "engine_vector.h"
#include "engine_io.h"
#include "engine_threading.h"
#include "engine_string.h"
class PersistClass;
class PersistFactoryClass
{

public:

           PersistFactoryClass();
  virtual ~PersistFactoryClass();

  virtual uint32        Chunk_ID () const = 0;
  virtual PersistClass* Load     (ChunkLoadClass& xLoad) const = 0;
  virtual void          Save     (ChunkSaveClass& xSave, PersistClass*) const = 0;

};


#endif
