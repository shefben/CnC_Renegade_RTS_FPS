#ifndef TT_INCLUDE__SAVELOADSYSTEMCLASS_H
#define TT_INCLUDE__SAVELOADSYSTEMCLASS_H
#include "PointerRemapClass.h"
#include "SaveLoadSubSystemClass.h"


class SaveLoadSystemClass
{

public:

   static REF_DECL2(PointerRemapper, PointerRemapClass);

   static SaveLoadSubSystemClass* Find_Sub_System                   (uint32 id);
   static bool                    Load                              (ChunkLoadClass& chunkLoader, bool);
   static void                    Post_Load_Processing              (void(*)());
   static void                    Register_Pointer                  (void*, void*);
   static void                    Register_Post_Load_Callback       (PostLoadableClass*);
   static void                    Request_Pointer_Remap             (void**);
   static void                    Register_Sub_System               (SaveLoadSubSystemClass *);
   static void                    Unregister_Sub_System             (SaveLoadSubSystemClass *);
};


#endif
