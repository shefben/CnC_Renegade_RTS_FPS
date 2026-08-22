#ifndef TT_INCLUDE__SPAWNERDEFCLASS_H
#define TT_INCLUDE__SPAWNERDEFCLASS_H

#include "engine_vector.h"
#include "Definition.h"
#include "engine_string.h"
#include "SpawnerClass.h"

class SpawnerDefClass : public DefinitionClass
{

private:

   DynamicVectorClass<uint32>      definitions;          // 28
   sint32                          team;                 // 52
   sint32                          maxSpawnCount;        // 56
public:
   float                           spawnDelay;           // 60
   float                           spawnDelayVariation;  // 64
   bool                            primary;              // 68
   bool                            multiplayerSoldier;   // 69
   bool                            gotoSpawn;            // 70
   float                           gotoSpawnPriority;    // 72
   bool                            teleportFirstSpawn;   // 76
   sint32                          spawnEffectId;        // 80
   float                           postVisualDelay;      // 84
   bool                            defaultDisabled;      // 88
   bool                            killHibernatingSpawn; // 89
   bool                            applySpawnEffect;     // 90
   bool                            multiplayerWeapon;    // 91
   DynamicVectorClass<StringClass> scriptNames;          // 92
   DynamicVectorClass<StringClass> scriptParameters;     // 116


public:

   SpawnerDefClass();

   virtual const PersistFactoryClass&       Get_Factory  () const;
   virtual bool                       Save         (ChunkSaveClass& chunkSaver);
   virtual bool                       Load         (ChunkLoadClass& chunkLoader);
   virtual uint32                     Get_Class_ID () const;
   virtual SpawnerClass*              Create       ();

   sint32                            Get_Player_Type              () const;
   const DynamicVectorClass<uint32>& Get_Spawn_Definition_ID_List () const { return definitions; }
   bool                              Is_Multiplay_Weapon_Spawner  () const;

   bool   getDefaultDisabled () const;
   sint32 getMaxSpawnCount   () const;

}; // size: 140


#endif
