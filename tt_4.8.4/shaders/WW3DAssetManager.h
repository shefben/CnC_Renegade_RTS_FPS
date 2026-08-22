#ifndef TT_INCLUDE__WW3DASSETMANAGER_H
#define TT_INCLUDE__WW3DASSETMANAGER_H

#include "scripts.h"
#include "engine_common.h"
#include "engine_vector.h"
#include "engine_threading.h"
#include "engine_string.h"
#include "engine_3dre.h"
#include "texture.h"
#include "HashTemplateClass.h"
#include "slist.h"

class RenderObjClass;
class RenderObjIterator;
class HAnimClass;
class PrototypeLoaderClass;
class Font3DInstanceClass;
class TextureClass;
class PrototypeClass;
class HashTableClass;
class TextureFileCache;
class MetalMapManagerClass;
class AssetIterator;


class PrototypeLoaderClass
{
public:
	virtual int Chunk_Type() = 0;
	virtual PrototypeClass* Load_W3D(ChunkLoadClass &chunk_load) = 0;
};

class HTreeManagerClass {
public:
	int NumTrees; // 0000
	HTreeClass* TreePtr[4096]; // 0004
	HashTemplateClass<StringClass,HTreeClass *> HashTable; // 4004
};
class HAnimManagerClass {
public:
	HashTableClass* AnimPtrTable;
	HashTableClass* MissingAnimTable;
};
class WW3DAssetManager
{
private:
   enum {
      PROTOLOADERS_VECTOR_SIZE = 0x20,
      PROTOLOADERS_GROWTH_RATE = 0x10,
      PROTOTYPES_VECTOR_SIZE = 0x100,
      PROTOTYPES_GROWTH_RATE = 0x20,
   };
   enum {
      PROTOTYPE_HASH_TABLE_SIZE = 0x1000,
      PROTOTYPE_HASH_BITS = 0xc,
      PROTOTYPE_HASH_MASK = 0xfff,
   };
   DynamicVectorClass<PrototypeLoaderClass *> PrototypeLoaders; //4
   DynamicVectorClass<PrototypeClass *> Prototypes; //1C
   PrototypeClass** PrototypeHashTable; //34
   HTreeManagerClass HTreeManager; //38
   HAnimManagerClass HAnimManager; //404C
   TextureFileCache* TextureCache; //4054
   SList<Font3DDataClass> Font3DDatas; //4058
   SimpleDynVecClass<FontCharsClass *> FontCharsList; //4064
   bool WW3D_Load_On_Demand; //4074
public:
   bool Activate_Fog_On_Load; //4075
   MetalMapManagerClass* MetalManager; //4078
   HashTemplateClass<StringClass,TextureClass *> TextureHash; //407C
   SHADERS_API static REF_DECL2(TheInstance, WW3DAssetManager*);
            WW3DAssetManager();
   virtual ~WW3DAssetManager();//0004

   virtual bool            Load_3D_Assets2             (FileClass&);//0008
   virtual bool SHADERS_API Load_3D_Assets             (const char*);//000C
   virtual void            Free_Assets                 ();//0010
   virtual void            Release_Unused_Assets       ();//0014
   virtual RenderObjClass* Create_Render_Obj           (const char*);//0018
   virtual bool            Render_Obj_Exists           (const char*);//001C
   virtual RenderObjIterator*Create_Render_Obj_Iterator();//0020
   virtual void            Release_Render_Obj_Iterator (RenderObjIterator*);//0024
   virtual AssetIterator*  Create_HAnim_Iterator       ();//0028
   virtual HAnimClass*     Get_HAnim                   (const char* animationName);//002C
   virtual bool            Add_Anim                    (HAnimClass*);
   virtual TextureClass   *Get_Texture                 (const char*, TextureClass::MipCountType, WW3DFormat, bool);
   virtual void            Release_All_Textures        ();
   virtual void            Release_Unused_Textures     ();
   virtual void            Release_Texture             (TextureClass*);
   virtual void            Load_Procedural_Textures    ();
   virtual MetalMapManagerClass*Peek_Metal_Map_Manager ();
   virtual Font3DInstanceClass *Get_Font3DInstance     (const char*);
   virtual FontCharsClass* Get_FontChars               (const char*, int, bool);
   virtual AssetIterator*  Create_HTree_Iterator       ();
   virtual HTreeClass*     Get_HTree                   (const char*);
   virtual void            Register_Prototype_Loader   (PrototypeLoaderClass*);
   virtual AssetIterator*  Create_Font3DData_Iterator  ();
   virtual void            Add_Font3DData              (Font3DDataClass*);
   virtual void            Remove_Font3DData           (Font3DDataClass*);
   virtual Font3DDataClass*Get_Font3DData              (const char*);
   virtual void            Release_All_Font3DDatas     ();
   virtual void            Release_Unused_Font3DDatas  ();
   virtual void            Release_All_FontChars       ();
   TextureClass           *Real_Get_Texture                 (const char*, TextureClass::MipCountType, WW3DFormat, bool);
   bool Get_Activate_Fog_On_Load()
   {
	   return Activate_Fog_On_Load;
   }
};


#endif
