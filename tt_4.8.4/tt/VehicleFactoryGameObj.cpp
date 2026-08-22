#include "general.h"

#include "VehicleFactoryGameObj.h"
#include "SoldierGameObj.h"
#include "VehicleFactoryGameObjDef.h"
#include "TimeManager.h"
#include "VehicleGameObj.h"
#include "BaseControllerClass.h"
#include "engine_obj.h"
#include "CombatManager.h"
#include "PhysicsSceneClass.h"
#include "OffenseObjectClass.h"
RENEGADE_FUNCTION
void VehicleFactoryGameObj::Deliver_Vehicle()
   AT2(0x006EE3B0,0x006ED970);

extern bool VehicleBuildingDisable;
extern bool BuildingHarvester[2];
void SetCurrentlyBuilding(bool building,int team);
bool VehicleFactoryGameObj::Request_Vehicle
   (uint32 uDefinitionID, float fCreationTime, SoldierGameObj* oRequester)
{
   if (!uDefinitionID || this->creatingVehicle)
      return false;

   this->uVehicleDefinitionID = uDefinitionID;
   this->fCreationTime = 5.0f;
   if (!VehicleBuildingDisable)
   {
      this->creatingVehicle = true;
   }
   this->oRequester = oRequester;
   this->totalBuildingTime = Get_Definition().Get_Total_Building_Time();
   this->Begin_Generation();
   this->Set_Object_Dirty_Bit (DB_RARE,true);

   return false;
}




// CLI: 0x006EDFB0, SVR: 0x006ED570
void VehicleFactoryGameObj::Think()
{
   if (this->totalBuildingTime > -0.01f)
   {
      this->totalBuildingTime -= TimeManager::FrameSeconds;
      if (this->totalBuildingTime <= 0)
      {
         this->On_Generation_Complete();
         this->totalBuildingTime = -0.01f;
      }
   }
   
   ScriptableGameObj::Think();
}

void VehicleFactoryGameObj::On_Generation_Complete()
{
	VehicleGameObj *veh = (VehicleGameObj *)vehicle.Get_Ptr();
	if ((veh) && (veh->As_VehicleGameObj()))
	{
		veh->Get_Action().params.MoveLocation.X = 0;
		veh->Get_Action().params.MoveLocation.Y = 0;
		veh->Get_Action().params.MoveLocation.Z = 0;
		veh->Get_Action().params.MoveSpeed = 0;
		veh->Get_Action().params.MoveArrivedDistance = 1000;
		veh->Get_Action().params.MoveCrouched = false;
		base->On_Vehicle_Generated(veh);
	}
	uVehicleDefinitionID = 0;
	fCreationTime = 0;
	if (VehicleBuildingDisable)
	{
		if (BuildingHarvester[Get_Object_Type(this)])
		{
			BuildingHarvester[Get_Object_Type(this)] = false;
			SetCurrentlyBuilding(false,Get_Object_Type(this));
		}
	}
	else
	{
		creatingVehicle = 0;
	}
	Set_Object_Dirty_Bit(DB_RARE,true);
}



RENEGADE_FUNCTION
VehicleGameObj* VehicleFactoryGameObj::Create_Vehicle()
AT2(0x006EE6A0, 0x006EDC60);
/*
VehicleGameObj* VehicleFactoryGameObj::Create_Vehicle()
{
	if (CombatManager::I_Am_Server() && uVehicleDefinitionID)
	{
		VehicleGameObj* _vehicle = (VehicleGameObj*)ObjectLibraryManager::Create_Object(uVehicleDefinitionID);
		if (_vehicle)
		{
			if (_vehicle->As_VehicleGameObj())
			{
				PhysClass* vehiclePhysics = vehicle->Peek_Physics_Object();
				if (vehiclePhysics)
				{
					if (MapMgrClass::EnableVTOL || !vehiclePhysics->As_VTOLVehicleClass()))
					{
						_vehicle->Start_Observers();
						vehicle = _vehicle;
						return _vehicle;
					}
				}
			}
			
			_vehicle->Set_Delete_Pending();
		}
	}
	
	return NULL;
}
*/

class OBBoxIntersectionTestClass
{
public:
	unsigned int unk1;
	OBBoxClass Box;
	Vector3 unk2;
	Vector3 unk3;
};

class PhysOBBoxIntersectionTestClass : public OBBoxIntersectionTestClass
{
public:
	int CollisionGroup;
	bool unk4;
	bool unk5;
	GenericMultiListClass* MultiList;
};

void VehicleFactoryGameObj::Destroy_Blocking_Objects()
{
	if (CombatManager::I_Am_Server())
	{
		MultiListClass<PhysClass> list;
		COMBAT_SCENE->Collect_Objects(constructionZone,false,true,&list);
		for (MultiListIterator<PhysClass> iterator(&list); iterator; ++iterator)
		{
			if (iterator.Peek_Obj()->Get_Observer())
			{
				PhysicalGameObj *owner = ((CombatPhysObserverClass *)iterator.Get_Obj()->Get_Observer())->As_PhysicalGameObj();
				if (owner)
				{
					if (owner != vehicle)
					{
						ArmedGameObj *ArmedObject = owner->As_ArmedGameObj();
						if (ArmedObject)
						{
							PhysOBBoxIntersectionTestClass IntersectionTest;
							memset(&IntersectionTest, 0, sizeof(IntersectionTest));
							IntersectionTest.Box = constructionZone;
							if (ArmedObject->Peek_Physical_Object()->Intersection_Test(IntersectionTest))
							{
								OffenseObjectClass offense;
								offense.ForceServerDamage = false;
								offense.EnableClientDamage = false;
								offense.Set_Damage(10000);
								offense.Set_Warhead(Get_Definition().Get_Pad_Clearing_Warhead());
								offense.Set_Owner(0);
								Vector3 v = Vector3(0,0,0);
								owner->Apply_Damage_Extended(offense,1,v,0);
							}
						}
					}
				}
			}
		}
	}
}

const VehicleFactoryGameObjDef & VehicleFactoryGameObj::Get_Definition( void ) const
{
	return (const VehicleFactoryGameObjDef &)*definition;
}
