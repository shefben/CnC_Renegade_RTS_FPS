#include "General.h"
#include "cClientHintManager.h"

#include "cNetwork.h"
#include "CombatManager.h"
#include "cUserOptions.h"
#include "SysTimeClass.h"
#include "SoldierGameObj.h"
#include "PhysicsSceneClass.h"
#include "NetworkObjectMgrClass.h"
#include "VisTableClass.h"
#include "cPriority.h"
#include "cCsHint.h"
#include "GameObjManager.h"
#pragma warning(disable:6255) // _alloca indicates failure by raising a stack overflow exception. Consider using _malloca instead



void cClientHintManager::Think()
{
	if (!cNetwork::I_Am_Server() && COMBAT_SCENE && cUserOptions::ClientHintFactor >= 1)
	{
		TT_ASSERT(GameObjManager::Find_Soldier_Of_Client_ID(cNetwork::Get_My_Id()) == COMBAT_STAR || (COMBAT_STAR && COMBAT_STAR->Is_Delete_Pending())); // It used to find the star by the client id (left part of the equality). StealthEye thinks this is the same as COMBAT_STAR, which is more efficient. TODO: Remove if never triggered.
		
		SoldierGameObj* soldier = COMBAT_STAR;
		if (soldier)
		{
			uint32 currentTime = TIMEGETTIME();
			static uint32 lastHintTime = currentTime - 1000;
			
			if (currentTime - lastHintTime >= 1000)
			{
				Vector3 soldierEyePosition;
				soldier->Get_Position(&soldierEyePosition);
				soldierEyePosition.Z += 1.5f;
				
				VisTableClass* visTable = COMBAT_SCENE->Get_Vis_Table(soldierEyePosition);
				
				int objectCount = NetworkObjectMgrClass::_ObjectList.Count();
				int tempObjectCount = 0;
				NetworkObjectClass** tempObjects = (NetworkObjectClass**)alloca(objectCount * sizeof(NetworkObjectClass*));
				
				for (int objectIndex = 0; objectIndex < objectCount; ++objectIndex)
				{
					NetworkObjectClass* object = NetworkObjectMgrClass::_ObjectList[objectIndex];
					
					if (object->Get_App_Packet_Type() == 2 || // APPPACKETTYPE_SOLDIER
						object->Get_App_Packet_Type() == 3) // APPPACKETTYPE_VEHICLE
					{
						if (object->Get_Clientside_Update_Frequency() != 0)
						{
							int visId = object->Get_Vis_ID();
							if (!visTable || visId == -1 || visTable->Get_Bit(visId))
							{
								float priority = cPriority::Compute_Object_Priority(cNetwork::Get_My_Id(), soldierEyePosition, object, 1, 0);
								object->Set_Cached_Priority(priority);
								
								tempObjects[tempObjectCount++] = object;
							}
						}
					}
				}
				
				if (visTable)
					visTable->Release_Ref();
				
				if (tempObjectCount > 1)
				{
					qsort(tempObjects, tempObjectCount, sizeof(NetworkObjectClass*), (int(*)(const void*, const void*))cClientHintManager::Priority_Compare);
					int maxUpdateRateRatioIndex = -1;
					int maxUpdateRateRatio = 0;
					for (int i = 1; i < tempObjectCount; ++i)
					{
						if (currentTime - tempObjects[i]->Get_Last_Clientside_Update_Time() > 1500)
						{
							int currentUpdateRate = tempObjects[i]->Get_Clientside_Update_Frequency();
							int previousUpdateRate = tempObjects[i-1]->Get_Clientside_Update_Frequency();
							
							if (previousUpdateRate != 0 && currentUpdateRate != 0 && previousUpdateRate < currentUpdateRate)
							{
								int updateRateRatio = 100 * currentUpdateRate / previousUpdateRate;
								if (updateRateRatio > maxUpdateRateRatio)
								{
									maxUpdateRateRatio = updateRateRatio;
									maxUpdateRateRatioIndex = i;
								}
							}
						}
					}
					
					if (maxUpdateRateRatioIndex != -1 && maxUpdateRateRatio > cUserOptions::ClientHintFactor * 10.f + 100.f)
					{
						(new cCsHint())->Init(tempObjects[maxUpdateRateRatioIndex]->Get_Network_ID());
						lastHintTime = currentTime;
					}
				}
			}
		}
	}
}



int cClientHintManager::Priority_Compare(const void** object1, const void** object2)
{
	float priority1 = ((NetworkObjectClass*)*object1)->Get_Cached_Priority();
	float priority2 = ((NetworkObjectClass*)*object2)->Get_Cached_Priority();
	
	if (priority1 == priority2)
		return 0;
	else if (priority1 >= priority1)
		return 1;
	else
		return -1;
}
