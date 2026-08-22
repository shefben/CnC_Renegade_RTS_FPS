#include "General.h"
#include "AirStripGameObj.h"



#include "CombatManager.h"
#include "VehicleGameObj.h"
#include "RenderObjClass.h"
#include "ObjectLibraryManager.h"
#include "AirStripGameObjDef.h"
#include "jfwcine.h"


extern bool VehicleOwnershipDisable;
void AirStripGameObj::Start_Cinematic()
{
	if (CombatManager::I_Am_Server())
	{
		VehicleGameObj* vehicle = Create_Vehicle();
		if (vehicle)
		{
			RenderObjClass* model = vehicle->Peek_Physical_Object()->Peek_Model();
			if (model)
			{
				model->Set_Hidden(true);
				vehicle->Set_Object_Dirty_Bit(DB_RARE, true);
			}

			if (oRequester)
			{
				if (VehicleOwnershipDisable)
				{
					Commands->Send_Custom_Event(oRequester,vehicle,CUSTOM_EVENT_VEHICLE_OWNER,Commands->Get_ID(oRequester),0);
				}
				else
				{
					vehicle->Lock_Vehicle(oRequester,30);
				}
				Commands->Send_Custom_Event(oRequester, vehicle, CUSTOM_EVENT_VEHICLE_OWNER, oRequester.Get_Ptr()->Get_Network_ID(), 0);
			}
			
			dropCinematic = (PhysicalGameObj*)ObjectLibraryManager::Create_Object(Get_Definition().dropCinematicId);
			if (dropCinematic)
			{
				dropCinematic->Start_Observers();
				dropCinematic->Set_Transform(creationTransform);
				
				for (int i = 0; i < dropCinematic->Get_Observers().Count(); i++)
				{
					GameObjObserverClass* observer = dropCinematic->Get_Observers()[i];
					if (_stricmp(observer->Get_Name(), "Test_Cinematic") == 0)
					{
						observer->Custom(this, 10000 + Get_Definition().dropCinematicVehicleSlot, vehicle->Get_Network_ID(), this);
						break;
					}
				}
			}
		}

		isCinematicPlaying = true;
		lengthToVehicleDisplay = Get_Definition().lengthToVehicleDisplay;
	}
}

const AirStripGameObjDef & AirStripGameObj::Get_Definition( void ) const
{
	return (const AirStripGameObjDef &)*definition;
}
