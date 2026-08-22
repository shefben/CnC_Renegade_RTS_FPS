#include "general.h"

#include "SpawnManager.h"
#include "MoveablePhysClass.h"
#include "SoldierGameObj.h"
#include "SpawnerDefClass.h"
REF_DEF2(SpawnManager::AutoSpawnTimer, float, 0x00811EDC, 0x008110B4); // = 2.0f
REF_DEF2(SpawnManager::SpawnerList, DynamicVectorClass<SpawnerClass*>, 0x008572C0, 0x008564A8);

Matrix3D SpawnManager::Get_Multiplayer_Spawn_Location
   (sint32 team, SoldierGameObj* soldier)
{
   if (team == -2)
      team = -1;

   SimpleDynVecClass<SpawnerClass*> availableSpawns;
   SimpleDynVecClass<SpawnerClass*> freeSpawns;

   MoveablePhysClass* phys = soldier->Peek_Physical_Object()->As_MoveablePhysClass();

   for (sint32 u = 0; u < SpawnerList.Count(); ++u)
   {
      SpawnerClass* spawn = SpawnerList[u];
      const SpawnerDefClass* definition = spawn->Get_Definition();

      if (!definition->primary && definition->multiplayerSoldier && definition->Get_Player_Type() == team)
      {
         availableSpawns.Add(spawn);
         if (phys && phys->Can_Teleport(spawn->Get_TM(), true, 0))
            freeSpawns.Add(spawn);
      }
   }

	SpawnerClass* spawn = NULL;
	if (freeSpawns.Count() != 0)
	{
		spawn = freeSpawns[rand() % freeSpawns.Count()];
		if (!spawn)
			spawn = availableSpawns[rand() % availableSpawns.Count()];
	}
	return (spawn ? spawn->Get_TM() : Matrix3D());
}

const SpawnerDefClass* SpawnerClass::Get_Definition() const
{
   return this->definition;
}

Matrix3D SpawnerClass::Get_TM() const
{
   return this->transform;
}

sint32 SpawnerDefClass::Get_Player_Type() const
{
   return this->team;
}



RENEGADE_FUNCTION
Matrix3D SpawnManager::Get_Primary_Spawn_Location()
AT2(0x006A24B0, 0x006A1D50);



RENEGADE_FUNCTION
SpawnerClass* SpawnManager::Get_Primary_Spawner()
AT2(0x006A2A70, 0x006A2310);
