#include "General.h"
#include "RefineryGameObj.h"

#include "RefineryGameObjDef.h"
#include "CombatManager.h"
#include "TimeManager.h"
#include "HarvesterClass.h"
#include "engine_game.h"


void RefineryGameObj::Think()
{
	Manage_Money_Trickle_Sound();

	if (CombatManager::I_Am_Server() && !destroyed && Defense.Get_Health() > 0 && The_Game()->Is_Gameplay_Permitted())
	{
		if (!harvester)
		{
			Set_Is_Harvester_Docked(false);
			Play_Unloading_Animation(false);
			base->Request_Harvester(Get_Definition().harvesterId);
		}
		
		if (harvesterDocked)
		{
			float elapsedTime = TimeManager::FrameSeconds / base->Get_Operation_Time_Factor();
			unloadTime -= elapsedTime;

			float remainingMoney = (unloadTime / Get_Definition().unloadTime) * Get_Definition().moneyPerUnload;
			if (remainingMoney < 0)
				remainingMoney = 0;
			float moneyToDistribute = moneyPerUnload - remainingMoney;

			base->Distribute_Funds_To_Each_Teammate(moneyToDistribute);
			moneyPerUnload -= moneyToDistribute;

			if (unloadTime <= 0)
			{
				harvester->Go_Harvest();
				Set_Is_Harvester_Docked(false);
				Play_Unloading_Animation(false);
			}
		}
		else
		{
			timeToNextMoney -= TimeManager::FrameSeconds;
			if (timeToNextMoney <= 0)
			{
				timeToNextMoney += 1.f;
				base->Distribute_Funds_To_Each_Teammate(Get_Definition().moneyPerSecond / base->Get_Operation_Time_Factor());
			}
		}

		if (harvester)
			harvester->Think();
	}

	BuildingGameObj::Think();
}



RENEGADE_FUNCTION
void RefineryGameObj::Manage_Money_Trickle_Sound()
AT2(0x007436D0, 0x00742F70);



RENEGADE_FUNCTION
void RefineryGameObj::Play_Unloading_Animation(bool)
AT2(0x007437A0, 0x00743040);



void RefineryGameObj::On_Harvester_Docked()
{
	const RefineryGameObjDef& definition = Get_Definition();

	unloadTime = definition.unloadTime;
	moneyPerUnload = definition.moneyPerUnload;
	moneyPerSecond = moneyPerUnload / unloadTime;

	Set_Is_Harvester_Docked(true);
}



void RefineryGameObj::Set_Is_Harvester_Docked
   (bool harvesterDocked)
{
   if (harvesterDocked == this->harvesterDocked)
      return;

   this->harvesterDocked = harvesterDocked;
   this->Set_Object_Dirty_Bit(DB_RARE, true);
}



RENEGADE_FUNCTION
void RefineryGameObj::Set_Harvester_Vehicle(VehicleGameObj*)
AT2(0x007438C0, 0x00743160);
