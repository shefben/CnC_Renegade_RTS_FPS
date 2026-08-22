#include "general.h"

#include "CombatManager.h"
#include "VehicleFactoryGameObj.h"
#include "SpawnerClass.h"
#include "SpawnManager.h"
#include "RefineryGameObj.h"
#include "BaseControllerClass.h"
#include "engine_obj.h"

extern bool VehicleBuildingDisable;
extern bool BuildingHarvester[2];
extern bool CurrentlyBuildingTeam[2];
void SetCurrentlyBuilding(bool building,int team);
void BaseControllerClass::Request_Harvester(int harvesterDefinitionId)
{
	if(CombatManager::I_Am_Server())
	{
		BuildingGameObj* obj1 = (BuildingGameObj*)this->Find_Building(2);
		if(obj1 != 0)
		{
			VehicleFactoryGameObj* vehicleFactory = obj1->As_VehicleFactoryGameObj();
			bool create;
			if (!VehicleBuildingDisable)
			{
				create = vehicleFactory->Is_Busy();
			}
			else
			{
				create = CurrentlyBuildingTeam[Get_Object_Type(vehicleFactory)];
			}
			if(!create && !vehicleFactory->Is_Destroyed())
			{
				vehicleFactory->Request_Vehicle(harvesterDefinitionId, (float)8.0 * (this->OperationTimeFactor), NULL);
				if (VehicleBuildingDisable)
				{
					if (harvesterDefinitionId)
					{
						SetCurrentlyBuilding(true,Get_Object_Type(vehicleFactory));
						BuildingHarvester[Get_Object_Type(vehicleFactory)] = true;
					}
				}
			}
		}
		else
		{
			BuildingGameObj* obj2 = (BuildingGameObj*)this->Find_Building(3);
			if(obj2 != 0)
			{
				RefineryGameObj* refinery = obj2->As_RefineryGameObj();
				DynamicVectorClass<SpawnerClass *> spawns = SpawnManager::SpawnerList;
				for(int i = 0; i < spawns.Count(); i++)
				{
					assert(i <= spawns.Length());
					if(spawns[i] != 0 && spawns[i]->Can_Spawn_Object(harvesterDefinitionId))
					{
						PhysicalGameObj* obj3 = spawns[i]->Spawn_Object(harvesterDefinitionId);
						if(obj3 != 0)
						{
							VehicleGameObj* newHarv = obj3->As_VehicleGameObj();
							if(newHarv != 0)
							{
								refinery->Set_Harvester_Vehicle(newHarv);
								break;
							}
						}
					}
				}
			}
		}
	}
	return;
}

RENEGADE_FUNCTION
void BaseControllerClass::On_Beacon_Armed(BeaconGameObj *)
AT2(0x006EF0F0,0x006EE6B0);

RENEGADE_FUNCTION
void BaseControllerClass::On_Beacon_Disarmed(BeaconGameObj *)
AT2(0x006EF160,0x006EE720);
