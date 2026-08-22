#include "General.h"
#include "CombatNetworkReceiverInstanceClass.h"

#include "cGameType.h"
#include "SysTimeClass.h"
#include "cUserOptions.h"
#include "cRemoteHost.h"
#include "cPlayerManager.h"
#include "Vector3.h"
#include "SoldierGameObj.h"
#include "cNetwork.h"
#include "engine_diagnostics.h"
#include "cServerFps.h"
#include "Iterator.h"
#include "cPlayer.h"
#include "GameObjManager.h"


bool CombatNetworkReceiverInstanceClass::Server_Update_Dynamic_Objects(bool isUrgent)
{
	if (!IS_MULTIPLAY)
		return false;
	
	int currentTime = TIMEGETTIME();

	cRemoteHost::PriorityUpdateRate = cUserOptions::NetUpdateRate;
	for (SLNode<cPlayer>* playerNode = PlayerList.Head(); playerNode; playerNode = playerNode->Next())
	{
		cPlayer* player = playerNode->Data();
		if (player->PlayerId >= 0 && player->IsActive && player->IsInGame) // Used to check for FPS and isUrgent player->PlayerId >= 0 && player->IsActive && ((IsIngame && fps good) || isUrgent)
		{
			Vector3 position;
			
			SoldierGameObj* soldierObject = GameObjManager::Find_Soldier_Of_Client_ID(player->PlayerId);
			if (soldierObject)
			{
				soldierObject->Get_Position(&position);
				position.Z += 1.5f;
			}
			else
				position = Vector3(0, 0, -10000.f);
			
			cNetwork::Tell_Client_About_Dynamic_Objects(player->PlayerId, position);
			player->Set_Last_Update_Time_Ms(currentTime);
		}
	}
	
	return true;
}



void CombatNetworkReceiverInstanceClass::Server_Send_Delete_Notifications()
{
	if (IS_MULTIPLAY)
		for (Iterator<SList<cPlayer>> player(PlayerList); player; ++player)
			if (player->PlayerId >= 0 && player->IsInGame && player->IsActive)
				cNetwork::Tell_Client_About_Delete_Notifications(player->PlayerId);
}



bool CombatNetworkReceiverInstanceClass::Client_Update_Dynamic_Objects(bool isUrgent)
{
	cNetwork::Tell_Server_About_Delete_Notifications();

	int updateRate = cUserOptions::NetUpdateRate;

//	int serverFps = cServerFps::Get_Instance()->Get_Fps();
//	if (serverFps > 0 && serverFps < cUserOptions::NetUpdateRate)
//		updateRate = serverFps;

	static int lastUpdateTime;
	int currentTime = TIMEGETTIME();
	if (isUrgent || currentTime - lastUpdateTime >= 1000.f / updateRate)
	{
		lastUpdateTime = currentTime;
		cNetwork::Tell_Server_About_Dynamic_Objects();
	}

	return true;
}



//void CombatNetworkReceiverInstanceClass::Print(const char*, ...)
//void CombatNetworkReceiverInstanceClass::Print(const Vector3&, const char*, ...)
