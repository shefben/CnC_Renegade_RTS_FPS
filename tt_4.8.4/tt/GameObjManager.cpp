#include "General.h"
#include "scripts.h"
#include "GameObjManager.h"

#include "cNetwork.h"
#include "SmartGameObj.h"
#include "engine_diagnostics.h"
#include "Iterator.h"


REF_DEF2(GameObjManager::CinematicFreezeActive, bool, 0x00857004, 0x008561EC);



// 0x00680210, 0x0067FAB0, 0x, 0x0810D878
PhysicalGameObj* GameObjManager::Find_PhysicalGameObj
   (uint32 networkId)
{
   if (!networkId)
      return 0;


   for (SLNode<GameObject>* node = GameObjManager::GameObjList.Head(); node; node = node->Next())
   {
	  BaseGameObj* object = (BaseGameObj *)node->Data();
      if (object->Get_Network_ID() != networkId)
         continue;

      return object->As_PhysicalGameObj();
   }

   return 0;
}



ScriptableGameObj* GameObjManager::Find_ScriptableGameObj
   (uint32 networkId)
{
   if (!networkId)
      return 0;

   for (SLNode<GameObject>* node = GameObjManager::GameObjList.Head(); node; node = node->Next())
   {
	  BaseGameObj* object = (BaseGameObj *)node->Data();
      if (object->Get_Network_ID() != networkId)
         continue;

      return object->As_ScriptableGameObj();
   }

   return 0;
}



RENEGADE_FUNCTION
void GameObjManager::Init()
AT2(0x0067FB90, 0x0067F430);



RENEGADE_FUNCTION
void GameObjManager::Shutdown()
AT2(0x0067FBF0, 0x0067F490);



RENEGADE_FUNCTION
bool GameObjManager::Save(ChunkSaveClass&)
AT2(0x0067FC50, 0x0067F4F0);



RENEGADE_FUNCTION
bool GameObjManager::Load(ChunkLoadClass&)
AT2(0x0067FD40, 0x0067F5E0);



RENEGADE_FUNCTION
void GameObjManager::Add(BaseGameObj*)
AT2(0x0067FE50, 0x0067F6F0);



RENEGADE_FUNCTION
void GameObjManager::Init_All()
AT2(0x0067FF40, 0x0067F7E0);



RENEGADE_FUNCTION
void GameObjManager::Destroy_All()
AT2(0x0067FF50, 0x0067F7F0);



void GameObjManager::Generate_Control()
{
	for (SLNode<SmartGameObj>* objectNode = (SLNode<SmartGameObj>*)SmartGameObjList.Head(); objectNode; objectNode = objectNode->Next())
	{
		SmartGameObj* object = objectNode->Data();
		if (!CinematicFreezeActive || !object->Is_Cinematic_Freeze_Enabled())
		{
			if (!object->Is_Hibernating())
				object->Generate_Control();
			else
				TT_ASSERT(object->Get_Control_Owner() != cNetwork::Get_My_Id()); // StealthEye: The client should always generate control for his own player object afaik.
		}
	}
}




RENEGADE_FUNCTION
void GameObjManager::Think()
AT2(0x00680000, 0x0067F8A0);



RENEGADE_FUNCTION
void GameObjManager::Post_Think()
AT2(0x006800D0, 0x0067F970);



SoldierGameObj* GameObjManager::Find_Soldier_Of_Client_ID(int clientId)
{
	for (Iterator<SList<SmartGameObj>> object((SList<SmartGameObj>&)SmartGameObjList); object; ++object)
	{
		SoldierGameObj* soldierObject = object->As_SoldierGameObj();
		if (soldierObject && !object->Is_Delete_Pending() && object->Get_Control_Owner() == clientId)
			return soldierObject;
	}
	
	return NULL;
}



RENEGADE_FUNCTION
SoldierGameObj* GameObjManager::Find_Different_Player_Soldier(int)
AT2(0x00680170, 0x0067FA10);



RENEGADE_FUNCTION
SoldierGameObj* GameObjManager::Find_Soldier_Of_Player_Type(int)
AT2(0x006801C0, 0x0067FA60);



RENEGADE_FUNCTION
VehicleGameObj* GameObjManager::Find_Vehicle_Occupied_By(SoldierGameObj*)
AT2(0x00680270, 0x0067FB10);



RENEGADE_FUNCTION
void GameObjManager::Init_Buildings()
AT2(0x006802E0, 0x0067FB80);



RENEGADE_FUNCTION
void GameObjManager::Update_Building_Collection_Spheres()
AT2(0x00680310, 0x0067FBB0);



RENEGADE_FUNCTION
void GameObjManager::Debug_Set_All_Building_States(float, bool)
AT2(0x00680420, 0x0067FCC0);



RENEGADE_FUNCTION
bool GameObjManager::Is_In_Environment_Zone(Vector3&)
AT2(0x00680470, 0x0067FD10);
