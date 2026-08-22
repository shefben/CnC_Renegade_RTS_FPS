#include "General.h"
#include "GameInitMgrClass.h"

#include "cGameType.h"
#include "SysTimeClass.h"
#include "WWAudioClass.h"
#include "GameModeManager.h"
#include "GameModeClass.h"
#include "engine_game.h"
#include "ModPackageMgrClass.h"
#include "DataSafe.h"



REF_DEF2(GameInitMgrClass::IsClientRequired, bool, 0x0081C65C, 0x0081B83C);
REF_DEF2(GameInitMgrClass::IsServerRequired, bool, 0x0081C65D, 0x0081B83D);
REF_DEF2(GameInitMgrClass::RestoreSFX, bool, 0x0081C65E, 0x0081B83E);
REF_DEF2(GameInitMgrClass::RestoreMusic, bool, 0x0081C65F, 0x0081B83F);
REF_DEF2(GameInitMgrClass::NeedsGameExit, bool, 0x0081C660, 0x0081B840);
REF_DEF2(GameInitMgrClass::NeedsGameExitAll, bool, 0x0081C661, 0x0081B841);



RENEGADE_FUNCTION
bool GameInitMgrClass::Is_Game_In_Progress()
AT2(0x00404660, 0x00404660);



RENEGADE_FUNCTION
void GameInitMgrClass::Start_Game(const char*, int, uint32)
AT2(0x00404690, 0x00404690);
/*
void GameInitMgrClass::Start_Game(const char* mapName, int a2, uint32 a3)
{
	if (IS_SOLOPLAY)
	{
		for (int startTime = TIMEGETTIME(); TIMEGETTIME() - startTime < 1500; )
			WWAudioClass::_theInstance->On_Frame_Update(0);
		
		WWAudioClass::_theInstance->Flush_Playlist();
		
		for (int startTime = TIMEGETTIME(); TIMEGETTIME() - startTime < 250; )
			WWAudioClass::_theInstance->On_Frame_Update(0);
	}
	
	if (GameModeManager::Find("Combat")->Is_Active())
	{
		GameInitMgrClass::End_Game();
		GameModeManager::Safely_Deactivate();
	}
	
	The_Game()->Set_Map_Name(mapName);
	ModPackageMgrClass::Load_Current_Mod();
	_reload_game_configuration_files();
	GenericDataSafeClass::Reset();
	PacketManager.Reset_Stats();
	GameInitMgrClass::Start_Client_Server();
	GameModeManager::Find("Menu")->Deactivate();
	GameModeManager::Find("Combat")->Activate();
	GameModeManager::Find("Combat")->Load_Level();
	if (GameInitMgrClass::Mode == 3)
		LanGameModeClass::Get_Lan_Interface()->Go_To_Location(11);
	else if (GameInitMgrClass::Mode == 4)
		WOL STUFF?;
	
	PhysicsSceneClass::TheScene->Release_Projector_Resources();
	TheDX8MeshRenderer->Invalidate();
	GameModeManager::Hide_Render_Frames(2);
	GameInitMgrClass::Transmit_Player_Data(a2, a3);
	AutoRestartClass::Set_Restart_Flag(AutoRestart, The_Game()->IsAutoRestart);
	GameSideServerControlClass::Init();
}
*/



RENEGADE_FUNCTION
void GameInitMgrClass::End_Game()
AT2(0x00404A80, 0x00404A80);



RENEGADE_FUNCTION
void GameInitMgrClass::Continue_Game()
AT2(0x00404D80, 0x00404D80);



RENEGADE_FUNCTION
void GameInitMgrClass::Display_End_Game_Menu()
AT2(0x00404E50, 0x00404E50);



RENEGADE_FUNCTION
UNK GameInitMgrClass::Transmit_Player_Data(int, uint32)
AT2(0x00404EA0, 0x00404EA0);



RENEGADE_FUNCTION
UNK GameInitMgrClass::Start_Client_Server()
AT2(0x00404F00, 0x00404F00);



RENEGADE_FUNCTION
UNK GameInitMgrClass::End_Client_Server()
AT2(0x004050D0, 0x004050D0);



RENEGADE_FUNCTION
UNK GameInitMgrClass::Initialize_SP()
AT2(0x004050F0, 0x004050F0);



RENEGADE_FUNCTION
UNK GameInitMgrClass::Shutdown_SP()
AT2(0x00405230, 0x00405230);



RENEGADE_FUNCTION
UNK GameInitMgrClass::Initialize_Skirmish()
AT2(0x00405240, 0x00405240);



RENEGADE_FUNCTION
UNK GameInitMgrClass::Shutdown_Skirmish()
AT2(0x00405380, 0x00405380);



RENEGADE_FUNCTION
UNK GameInitMgrClass::Initialize_LAN()
AT2(0x00405390, 0x00405390);



RENEGADE_FUNCTION
UNK GameInitMgrClass::Shutdown_LAN()
AT2(0x00405480, 0x00405480);



RENEGADE_FUNCTION
UNK GameInitMgrClass::Initialize_WOL()
AT2(0x004054A0, 0x004054A0);



RENEGADE_FUNCTION
UNK GameInitMgrClass::Shutdown_WOL()
AT2(0x00405590, 0x00405590);



RENEGADE_FUNCTION
UNK GameInitMgrClass::Shutdown()
AT2(0x004055B0, 0x004055B0);



RENEGADE_FUNCTION
UNK GameInitMgrClass::Think()
AT2(0x00405660, 0x00405660);
