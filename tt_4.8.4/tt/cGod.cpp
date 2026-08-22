#include "general.h"
#include "cGod.h"


#include "cPlayer.h"
#include "engine_game.h"
#include "bitstream.h"

#include "GameModeClass.h"
#include "cPlayerManager.h"
#include "GameModeManager.h"
#include "tt.h"
#include "SysTimeClass.h"
#include "cNetwork.h"
#include "WolGameModeClass.h"
#include "cGameType.h"
#include "SpawnerClass.h"
#include "SpawnManager.h"
#include "DefinitionMgrClass.h"
#include "SpawnerDefClass.h"
#include "ObjectLibraryManager.h"
#include "CombatManager.h"
#include "SoldierGameObj.h"
#include "SoldierObserverClass.h"



class NetworkObjectClass;
char *GDISoldierName;
char *NodSoldierName;



RENEGADE_FUNCTION
void cGod::Think()
AT2(0x00405820, 0x00405820);
/*
{
	if (!The_Game()->IsIntermission)
	{
		if (!PlayerList.Is_Empty())
		{
			if (State == 0)
				State = IS_MISSION ? 3 : 1;

			if (State == 3)
			{
				SoldierGameObj* soldierObject = Create_Commando(PlayerList.Head()->Data());
				script = ScriptManager::Create_Script(CombatManager::StartScript);
				if (script)
					soldierObject->Add_Observer(script);
				State = 4;
			}

			if (State == 4)
			{
				if (!PlayerList.Is_Empty())
				{
					cPlayer* player = PlayerList->Head().Data();
					if (player)
					{
						SoldierGameObj* soldier = GameObjManager::Find_Soldier_Of_Client_ID(player->PlayerId);
						if (soldier && !player->Owner)
							soldier->Set_Player_Data(player);
					}
				}
			}

			if (State == 1)
			{
				for (SLNode<cPlayer>* playerNode = PlayerList.Head(); playerNode; playerNode = playerNode->Next())
				{
					cPlayer* player = playerNode->Data();
					if (player->IsInGame && player->IsActive && !GameObjManager::Find_Soldier_Of_Client_ID(player->PlayerId))
						Create_Commando(player);
				}
			}
		}
	}
}
*/



cPlayer* cGod::Create_Player(int clientId, const WideStringClass& name, int preferredTeamId, uint32 wolClanId, bool)
{
	cPlayer* player = 0;

	player = cPlayerManager::Find_Player(name);
	if (player)
		cNetwork::Delete_Player_Objects(player->PlayerId);
	else
		player = cPlayerManager::Find_Inactive_Player (name);

	bool newPlayer = !player;

	if (newPlayer)
	{
		player = new cPlayer;
		player->Init();
		player->Set_Name(name);
		player->Set_Player_Type(The_Game()->Choose_Player_Type(player, preferredTeamId, 0));
	}

	player->Set_Id(clientId);
	player->Set_Wol_ClanID(wolClanId);
	player->Set_Is_In_Game(true);
	player->Set_Is_Waiting_For_Intermission(false);
	player->Reset_Join_Time();
	player->Set_Is_Active(true);

	if (newPlayer)
	{
		WolGameModeClass* wolGameMode = (WolGameModeClass*)GameModeManager::Find("WOL");
		if (wolGameMode->Is_Active())
			wolGameMode->Init_WOL_Player(player);
	}

	DoPlayerJoin2(player);

	cNetwork::Send_Object_Update(player, player->PlayerId);
	return player;
}



SoldierGameObj* cGod::Create_Commando(int playerId, int playerType)
{
	StringClass commandoDefinitionName = "Commando";
	if (cGameType::GameType == 1)
	{
		SpawnerClass* spawner = SpawnManager::Get_Primary_Spawner();
		if (spawner)
		{
			DefinitionClass* definition = DefinitionMgrClass::Find_Definition(spawner->Get_Definition()->Get_Spawn_Definition_ID_List()[0], true);
			if (definition)
				commandoDefinitionName = definition->Get_Name();
		}
	}
	else if (The_Game()->Is_Cnc() || The_Game()->Is_Skirmish())
	{
		if (playerType)
			commandoDefinitionName = GDISoldierName;
		else
			commandoDefinitionName = NodSoldierName;
	}

	SoldierGameObj* commando = (SoldierGameObj*)ObjectLibraryManager::Create_Object(commandoDefinitionName);
	if (cGameType::GameType != 3)
	{
		float healthShieldStrength;
		if (CombatManager::Get_Difficulty_Level() == 0)
			healthShieldStrength = 200.f;
		else if (CombatManager::Get_Difficulty_Level() == 2)
			healthShieldStrength = 75.f;
		else
			healthShieldStrength = 100.f;
		
		commando->Get_Defense_Object()->Set_Health_Max(healthShieldStrength);
		commando->Get_Defense_Object()->Set_Health(healthShieldStrength);
		commando->Get_Defense_Object()->Set_Shield_Strength_Max(healthShieldStrength);
		commando->Get_Defense_Object()->Set_Shield_Strength(healthShieldStrength);
	}
	
	if (cGameType::GameType != 1 || playerType != 1)
		commando->Set_Transform(SpawnManager::Get_Multiplayer_Spawn_Location(playerType, commando));
	else
		commando->Set_Transform(SpawnManager::Get_Primary_Spawn_Location());
	
	commando->Set_Control_Owner(playerId);
	commando->Set_Player_Data(cPlayerManager::Find_Player(playerId));
	
	if (The_Game()->Remember_Inventory() &&cGameType::GameType == 1)
		Restore_Inventory(commando);
	
	commando->Set_Player_Type(playerType);
	if (!commando->Is_Human_Controlled())
	{
		commando->Set_Innate_Observer(new SoldierObserverClass());
		commando->Add_Observer(commando->Get_Innate_Observer());
	}
	
	commando->Start_Observers();
	The_Game()->Soldier_Added(commando);
	if (cNetwork::I_Am_Client() && playerId == cNetwork::Get_My_Id())
	{
		ActionParamsStruct actionParams;
		commando->Get_Action().ActionClass::Follow_Input(actionParams);
		CombatManager::Set_The_Star(commando, true);
	}

	return commando;
}



RENEGADE_FUNCTION
void cGod::Restore_Inventory(SoldierGameObj*)
AT2(0x00407840, 0x00407840);
