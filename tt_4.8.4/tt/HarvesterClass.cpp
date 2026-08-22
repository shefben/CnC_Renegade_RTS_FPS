#include "general.h"

#include "HarvesterClass.h"
#include "TimeManager.h"
#include "VehicleGameObj.h"
#include "PhysClass.h"
#include "RenderObjClass.h"
#include "HTreeClass.h"
#include "RefineryGameObj.h"

void HarvesterClass::Harvest_Tiberium()
{
   if (!this->harvester)
      return;

   if (!this->harvesting)
   {
      this->harvesting  = true;
      this->harvestTime = 15.0f;
   }

   this->state               = StateHarvesting;
   this->harvestRelocateTime = (((::rand() & 0xFFF) / 4095.0f) * 4.0f) + 3.0f;

   const char* h = this->harvester->Peek_Physical_Object()->Peek_Model()->Get_HTree()->Get_Name();
   this->harvestAnimation.Format("%s.%s",h,h);
   this->Play_Harvest_Animation(true);
}



void HarvesterClass::Play_Harvest_Animation
   (bool play)
{
   if (play)
      this->harvester->Set_Animation (this->harvestAnimation, true, 0);
   else
      this->harvester->Set_Animation (0, true, 0);
}

void HarvesterClass::Action_Complete
   (GameObject *object,int action_id,ActionCompleteReason complete_reason)
{
   if (complete_reason)
      return;

   if (action_id == 1001)
      if (this->state == StateGoingToUnload)
      {this->Unload_Tiberium();
         return;
      }

   if (action_id == 1000)
      if (this->state == StateGoingToHarvest)
      {
         this->Harvest_Tiberium();
         return;
      }
}

void HarvesterClass::Unload_Tiberium()
{
   if (!this->harvester)
      return;
   if (this->state == StateUnloading)
      return;
   this->state = StateUnloading;
   this->refinery->On_Harvester_Docked();
}

void HarvesterClass::Think()
{
   if (this->harvesting)
   {
      this->harvestTime -= TimeManager::FrameSeconds;
      if (this->harvestTime <= 0)
      {
         this->Go_Unload_Tiberium();
         return;
      }
   }

   switch (this->state)
   {
   case NoState:
      if (this->harvester)
         this->Go_Harvest();
      break;

   case StateHarvesting:
      this->harvestRelocateTime -= TimeManager::FrameSeconds;
      if (this->harvestRelocateTime <= 0)
         this->Go_Harvest();
      break;

   default:
      ;
   }
}

void HarvesterClass::Go_Unload_Tiberium()
{
   if (!this->harvester)
      return;

   if (this->state == StateGoingToUnload)
      return;
   
   this->state = StateGoingToUnload;


   this->Play_Harvest_Animation (false);

   this->harvesting = false;


   ActionParamsStruct a;
   a.Priority     = 99;
   a.ActionID     = 1001;
   a.ObserverID   = this->Get_ID();
   a.DockLocation = this->dockLocation;
   a.DockEntrance = this->dockEntrance;

   this->harvester->Get_Action().Dock_Vehicle (a);
}

RENEGADE_FUNCTION
void HarvesterClass::Go_Harvest()
   AT2(0x00769C30,0x007694D0);

void HarvesterClass::Detach(GameObject *object)
{
	if (this->refinery)
	{
		this->refinery->Set_Harvester(0);
	}
	this->harvester = 0;
	delete this;
}