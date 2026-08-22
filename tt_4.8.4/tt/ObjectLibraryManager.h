#ifndef TT_INCLUDE__OBJECTLIBRARYMANAGER_H
#define TT_INCLUDE__OBJECTLIBRARYMANAGER_H
#include "Persist.h"

class ObjectLibraryManager
{

public:

   static PersistClass* Create_Object (const char* cPresetName);
   static PersistClass* Create_Object (uint32 uDefinitionID);

};


#endif
