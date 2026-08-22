#ifndef TT_INCLUDE__DEFINITIONMGRCLASS_H
#define TT_INCLUDE__DEFINITIONMGRCLASS_H

#include "scripts.h"
#include "engine_common.h"
#include "engine_threading.h"
#include "engine_string.h"
#include "engine_vector.h"
#include "PostLoadableClass.h"
class DefinitionClass;
class ChunkSaveClass;
class ChunkLoadClass;
class DefinitionMgrClass : public PostLoadableClass
{

public:

   static REF_DECL2(DefinitionCount, uint32);
   static REF_DECL2(SortedDefinitionArray, DefinitionClass**);

   static DefinitionClass* Find_Definition              (uint32 uID, bool);
   static DefinitionClass* Find_Named_Definition        (const char* cPresetName, bool);
   static DefinitionClass* Find_Typed_Definition        (const char* cName, uint32 uClassID, bool);
   static sint32                 fnCompareDefinitionsCallback (const void*, const void*);
   static void                   Prepare_Definition_Array     ();

   static DefinitionClass* findPreset (uint32 presetId);
   static DefinitionClass* findPreset (uint32 presetId, uint32 classId);


            DefinitionMgrClass();
   virtual ~DefinitionMgrClass();

   virtual uint32      Chunk_ID      ();
   virtual bool        Contains_Data ();
   virtual bool        Save          (ChunkSaveClass& chunkSaver);
   virtual bool        Load          (ChunkLoadClass& chunkLoader);
   virtual const char* Name          ();
   static void Free_Definitions();
};


#endif
