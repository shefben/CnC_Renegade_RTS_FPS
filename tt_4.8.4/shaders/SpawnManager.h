#ifndef TT_INCLUDE__SPAWNMANAGER_H
#define TT_INCLUDE__SPAWNMANAGER_H
#include "SpawnerClass.h"

class SoldierGameObj;
class SpawnManager
{

public:

   static REF_DECL2(AutoSpawnTimer, float);
   static REF_DECL2(SpawnerList, DynamicVectorClass<SpawnerClass*>);


   static void             Add_Spawner                            (SpawnerClass* spawner);
   static void             Display_Unused_Spawners                ();
   static Matrix3D         Get_Ctf_Spawn_Location                 (sint32 team);
   static Matrix3D         Get_Multiplayer_Spawn_Location         (sint32 team, SoldierGameObj* soldier);
   static Matrix3D         Get_Primary_Spawn_Location             ();
   static SpawnerClass*    Get_Primary_Spawner                    ();
   static bool             Load                                   (ChunkLoadClass& chunkLoader);
   static void             Remove_All_Spawners                    ();
   static void             Remove_Spawner                         (SpawnerClass* spawner);
   static bool             Save                                   (ChunkSaveClass& chunkSaver);
   static void             Spawner_Enable                         (sint32, bool);
   static bool             Spawner_Exists                         (sint32);
   static PhysicalGameObj* Spawner_Trigger                        (sint32);
   static void             Toggle_Allow_Killing_Hibernating_Spawn ();
   static void             Update                                 ();

};


#endif
