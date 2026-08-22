#include "General.h"
#include "GameData.h"

#include "dialogmgr.h"
#include "engine2.h"
#include "string_ids.h"
#include "GameModeManager.h"
#include "WOLGameModeClass.h"
#include "DlgCnCWinScreen.h"
#include "cTeamManager.h"
#include "cPlayerManager.h"
#include "SysTimeClass.h"
#include "cWinEvent.h"
#include "cUserOptions.h"
#include "ConsoleModeClass.h"
#include "TextDisplayGameModeClass.h"
#include "cGameType.h"
#include "WWAudioClass.h"
#include "cNetwork.h"
#include "cGameDataUpdateEvent.h"
#include "cScTextObj.h"
#include "cTeam.h"
#include "BuildingGameObj.h"
#include "VehicleGameObj.h"
#include "CombatManager.h"
#include "SpawnManager.h"
#include "SpawnerDefClass.h"
#include "Iterator.h"
#include "ResourceMgr/NetworkManager.h"
#include "TranslateDBClass.h"


REF_DEF2(cGameData::IsManualExit, bool, 0x0082F125, 0x0082E30D);



void cGameData::Begin_Intermission()
{
	if (GameModeManager::Find("WOL")->Is_Active())
		((WolGameModeClass *)GameModeManager::Find("WOL"))->End_Game();
	
	IsIntermission = true;
	IntermissionTimeLeft = (float)IntermissionTime_Seconds;
	if (!ConsoleBox.Is_Exclusive())
	{
		DlgCnCWinScreen *dlg = new DlgCnCWinScreen();
		dlg->Start_Dialog();
		dlg->Release_Ref();
	}
}


void DoGameOver2();
void cGameData::Game_Over_Processing()
{
	TT_ASSERT(cNetwork::I_Am_Server());

	WinnerID = cTeamManager::Get_Leaders_Id();
	if (Has_Config_File_Changed())
	{
		cGameData* oTempGame = cGameData::Create_Game_Of_Type(2);
		if (oTempGame)
		{
			oTempGame->INI = INI;
			oTempGame->Load_From_Server_Config();
			WideStringClass oTempString;
			if (oTempGame->Is_Valid_Settings (oTempString, true))
			{
				ConsoleBox.Print("Game settings changed - reloading settings\n");
				Load_From_Server_Config();
			}
			delete oTempGame;
		}
	}
	Rotate_Map();
	cPlayerManager::Increment_Player_Times();
	uint32 uTime = TIMEGETTIME();
	Set_Game_Duration_S((uTime - GameStartTime) / 1000);

	WideStringClass mvp = cPlayerManager::Determine_Mvp_Name();
	if (!mvp.Is_Empty() && mvp.Compare_No_Case(MVPName))
		Increment_Mvp_Count();
	else
	{
		Set_Mvp_Count(1);
		Set_Mvp_Name(mvp);
	}

	serverNetworkManager.onGameEnd();

	(new cWinEvent)->Init(WinnerID, -99999, false);
	cPlayerManager::Compute_Ladder_Points(WinnerID);
	cTeamManager::Log_Team_List();
	cPlayerManager::Log_Player_List();
	sint32 sLogNumber = cUserOptions::ResultsLogNumber.Data;
	if (++sLogNumber > 100)
		sLogNumber = 1;
	
	cUserOptions::ResultsLogNumber.Set(sLogNumber);
	Begin_Intermission();
	DoGameOver2();
}
RENEGADE_FUNCTION
bool cGameData::Has_Config_File_Changed()
AT2(0x004751B0,0x00474880);
RENEGADE_FUNCTION
cGameData* cGameData::Create_Game_Of_Type(int type)
AT2(0x00475710,0x00474DE0);



void cGameData::Rotate_Map()
{
	++MapNumber;

	if (MapNumber >= 100 || MapList[MapNumber].Is_Empty())
	{
		MapNumber = 0;
		MapCycleOver = !DoMapsLoop;
	}
	else
		MapCycleOver = false;
	
	MapName = MapList[MapNumber];
}



void cGameData::Set_Game_Duration_S(uint32 gameDuration)
{
	GameDuration_Seconds = gameDuration;
}
void cGameData::Increment_Mvp_Count()
{
   MVPCount++;
}
void cGameData::Set_Mvp_Count(uint32 mvpCount)
{
   MVPCount = mvpCount;
}
void cGameData::Set_Mvp_Name(WideStringClass mvpName)
{
   MVPName = mvpName;
}
extern bool RenderHud;
extern bool HideBottomText;
REF_DEF1(cGameData::PTextRenderer,Render2DSentenceClass *,0x0082F11C);



void cGameData::Render()
{
	Show_Game_Settings_Limits();
	if (RenderHud && PTextRenderer && !HideBottomText)
	{
		PTextRenderer->Render();
	}
}



void cGameData::On_Game_Begin()
{
	GetSystemTime(&GameStartTime2);
	FrameCount = 0;
	GameStartTime = TIMEGETTIME();
	GameDuration_Seconds = 0;
	
	//cCsDamageEvent::AreClientsTrusted = UseLagReduction;
	BuildingGameObj::Set_Can_Repair_Buildings(CanRepairBuildings);
	VehicleGameObj::Set_Default_Driver_Is_Gunner(DriverIsAlwaysGunner);
	CombatManager::Set_Friendly_Fire_Permitted(IsFriendlyFirePermitted);
	CombatManager::Set_Beacon_Placement_Ends_Game(false);
	
	if (cNetwork::I_Am_Server())
	{
		Filter_Spawners();
		
		if (!IS_MISSION)
		{
			if (IsClanGame || IsTeamChangingAllowed)
			{
				if (Is_Skirmish() || Is_Cnc() && rand() & 1)
					Swap_Team_Sides();
			}
			else if (!RemixTeams)
			{
				if (Is_Skirmish() || Is_Cnc() && rand() & 1)
					Swap_Team_Sides();
				
				Rebalance_Team_Sides();
			}
			else
			{
				Remix_Team_Sides();
				Rebalance_Team_Sides();
			}
		}
	}
	
	WolGameModeClass* wolGameMode = (WolGameModeClass*)GameModeManager::Find("WOL");
	if (wolGameMode->Is_Active())
		wolGameMode->Start_Game(this);
}



void cGameData::On_Game_End()
{
	WolGameModeClass* wolGameMode = (WolGameModeClass*)GameModeManager::Find("WOL");
	if (wolGameMode->Is_Active())
		wolGameMode->End_Game();
}



void cGameData::Filter_Spawners()
{
	if (!SpawnWeapons)
		for (int i = 0; i < SpawnManager::SpawnerList.Count(); ++i)
			if (SpawnManager::SpawnerList[i]->Get_Definition()->multiplayerWeapon)
				SpawnManager::SpawnerList[i]->Enable(false);
}



void cGameData::Reset_Game(bool skipPlayerReset)
{
	WinnerID = -1;
	IsIntermission = false;

	if (GameModeManager::Find("Combat")->Is_Active())
	{
		GameModeClass* menuGameMode = GameModeManager::Find("Menu");
		if (menuGameMode->Is_Active())
			menuGameMode->Deactivate();
		else if (Exe == 0)
			DialogMgrClass::Flush_Dialogs();
	}

	if (TextDisplayGameModeClass::Instance)
		TextDisplayGameModeClass::Instance->Flush();

	if (!IS_MISSION)
		WWAudioClass::_theInstance->Create_Instant_Sound("Game_Start", Matrix3D::Identity, NULL, 0, 2);
	
	if (!IS_MISSION || !skipPlayerReset)
		cPlayerManager::Reset_Players();

	cTeamManager::Reset_Teams();
	
	if (TimeLimit_Minutes > 0 )
	{
		Reset_Time_Remaining_Seconds();
		if (cNetwork::I_Am_Server())
			(new cGameDataUpdateEvent())->Init(-1);
	}

	if (cNetwork::I_Am_Server())
		++HostedGameNumber;
}



void cGameData::Swap_Team_Sides()
{
	for (Iterator<SList<cPlayer>> player(PlayerList); player; ++player)
	{
		int pt = player->PlayerType;
		player->Set_Player_Type(1 - pt);
	}
	
	(new cScTextObj)->Init(TRANSLATE(IDS_MP_TEAMS_SWAPPED), PublicMessage, false, -1, -1);
}



void cGameData::Remix_Team_Sides()
{
	if (!IsClanGame)
	{
		for (Iterator<SList<cPlayer>> player(PlayerList); player; ++player)
			player->Set_Player_Type(rand() & 1);
		
		(new cScTextObj())->Init(TRANSLATE(IDS_MP_TEAMS_REMIXED), PublicMessage, false, -1, -1);
	}
}



void cGameData::Rebalance_Team_Sides()
{
	if (!IsClanGame)
	{
		int team0Size = cTeamManager::Find_Team(0)->Tally_Size();
		int team1Size = cTeamManager::Find_Team(1)->Tally_Size();
		
		int changeCount = (team0Size - team1Size) / 2;
		
		if (changeCount != 0)
		{
			if (changeCount > 0)
				while (changeCount--)
					cPlayerManager::Find_Team_Player(0)->Set_Player_Type(1);
			else
				while (changeCount++)
					cPlayerManager::Find_Team_Player(1)->Set_Player_Type(0);
			
			(new cScTextObj)->Init(TRANSLATE(IDS_MP_TEAMS_REBALANCED), PublicMessage, false, -1, -1);
		}
	}
}
