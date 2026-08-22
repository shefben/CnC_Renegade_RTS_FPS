#include "General.h"
#include "CombatGameModeClass.h"

#include "cNetwork.h"
#include "GameModeManager.h"
#include "CombatManager.h"
#include "CCameraClass.h"
#include "MultiHUDClass.h"
#include "cPlayerManager.h"
#include "cTeamManager.h"
#include "RadioCommandDisplayClass.h"
#include "ConsoleModeClass.h"
#include "LoadingScreenClass.h"
#include "LevelManager.h"
#include "WW3D.h"
#include "AssetStatusClass.h"
#include "cGameType.h"
#include "cConnection.h"
#include "cNetwork.h"
#include "SystemSettings.h"
#include "NetworkObjectMgrClass.h"
#include "textureloader.h"
#include "SaveLoadSystemClass.h"
#include "ControlClass.h"
#include "GameObjManager.h"
#include "PhysClass.h"
#include "VehicleGameObj.h"
#include "RadarManager.h"
#include "BuildingGameObj.h"
#include "DefenseObjectClass.h"
#include "HumanStateClass.h"
#include "DoorPhysClass.h"
#include "ElevatorPhysClass.h"
#include "CGameSpyQnR.h"
#include "HashMixFileFactoryClass.h"
#include "tt.h"
#include "engine_game.h"
#include "string_ids.h"
#include "render2d.h"
#include "FileFactoryListClass.h"
#include "SimpleFileFactoryClass.h"
#include "cSbboManager.h"
#include "CombatManager.h"
#include "MultiHUDClass.h"
#include "CampaignManager.h"
#include "ConsoleGameModeClass.h"
#include "cLoadingEvent.h"
#include "SaveGameManager.h"
#include "GameInitMgrClass.h"
#include "cGameSpyAdmin.h"
#include "CommandoSaveLoadClass.h"
#include "WWMeasureItClass.h"
#include "ResourceMgr/PackageManager.h"
#include "SoldierGameObj.h"
#include "TranslateDBClass.h"


extern Render2DClass *InfoTextureRender2DClass;
extern bool RenderInfoTexture;

class CombatGameMiscHandlerClass : public CombatMiscHandlerClass {
	virtual	void	Mission_Complete( bool success )												= 0;
	virtual	void	Star_Killed( void )																= 0;
};

REF_DEF2(g_b_core_restart, bool, 0x0081C610, 0x0081B7F0);
REF_DEF2(g_client_quit, bool, 0x0081C618, 0x0081B7F8);
REF_DEF2(GameMiscHandler, CombatGameMiscHandlerClass, 0x0081C614, 0x0081B7F4);
REF_DEF2(PendingCampaignContinue, bool, 0x0081C624, 0x0081B804);
REF_DEF2(ForceGodPending, bool, 0x007F5044, 0x007F4044);



RENEGADE_FUNCTION
void Windows_Message_Handler()
AT2(0x005DB280, 0x005DAB20);



RENEGADE_FUNCTION
void Stop_Main_Loop(int exitCode)
AT2(0x0043B8A0, 0x0043B940);



RENEGADE_FUNCTION
void CombatGameModeClass::Combat_Keyboard()
AT2(0x00401040, 0x00401040);



RENEGADE_FUNCTION
void CombatGameModeClass::Init()
AT2(0x004015D0, 0x004015D0);



RENEGADE_FUNCTION
void CombatGameModeClass::Shutdown()
AT2(0x00401600, 0x00401600);


extern HashMixFileFactoryClass* _mapFileFactory;
extern REF_DECL2(RenegadeBaseFileFactory, SimpleFileFactoryClass);
extern REF_DECL2(RenegadeFileFactory, FileFactoryListClass);
void CombatGameModeClass::Load_Level()
{
	if (stristr(The_Game()->MapName,".mix"))
	{
		_mapFileFactory = new HashMixFileFactoryClass(The_Game()->MapName,RenegadeBaseFileFactory);
		RenegadeFileFactory.Add_FileFactory(_mapFileFactory,The_Game()->MapName);
	}
	ConsoleBox.Print("Loading level %s\n", The_Game()->MapName);
	
	CombatManager::Set_Load_Progress(0);
	LoadingScreenClass loadingScreen;
	loadingScreen.Render(true);
	Load_Registry_Keys();
	
	//SaveLoadStatus::Set_Status_Text("Release current level", 0);
	
	LevelManager::Release_Level();
	if (!ConsoleBox.Is_Exclusive())
	{
		WW3D::_Invalidate_Textures();
		AssetStatusClass::Instance.unk0001 = false;
	}
	
	PendingCampaignContinue = false;
	cNetwork::g_is_loading = true;
	
	if (cNetwork::PClientConnection)
		cNetwork::PClientConnection->Allow_Packet_Processing(false);
	if (cNetwork::PServerConnection)
		cNetwork::PServerConnection->Allow_Packet_Processing(false);
	
	if (IS_MULTIPLAY && cNetwork::PClientConnection)
		cNetwork::PClientConnection->Allow_Extra_Timeout_For_Loading();
	
	CombatManager::Pre_Load_Level(!ConsoleBox.Is_Exclusive());
	
	
	//SaveLoadStatus::Set_Status_Text("Apply system settings", 0);
	
	SystemSettings::Apply_All();
	CombatManager::Set_Combat_Misc_Handler(&GameMiscHandler);
	
	//DiagLogClass::Log_Timed("LOAD", "%s", The_Game()->MapName);
	NetworkObjectMgrClass::_IsLevelLoading = true;
	TextureLoader::Suspend_Texture_Load();
	CombatManager::Load_Level_Threaded(The_Game()->MapName, 1);
	
	while (!CombatManager::Is_Load_Level_Complete())
	{
		loadingScreen.Render(true);
		Windows_Message_Handler();
		
		if (IS_MULTIPLAY)
			cNetwork::Update();
		
		ThreadClass::Sleep_Ms(50);
	}
	
	GenericDataSafeClass::Set_Preferred_Thread(GetCurrentThreadId());
	TextureLoader::Continue_Texture_Load();
	
	if (IS_MULTIPLAY)
		cNetwork::Update();
	
	
	//SaveLoadStatus::Set_Status_Text("Post_Load_Processing", 0);
	loadingScreen.Render(true);
	Windows_Message_Handler();
	
	SaveLoadSystemClass::Post_Load_Processing(IS_MULTIPLAY ? cNetwork::Update : NULL);
	NetworkObjectMgrClass::_IsLevelLoading = false;
	
	
	//SaveLoadStatus::Set_Status_Text("Post_Load_Level", 0);
	loadingScreen.Render(true);
	Windows_Message_Handler();
	
	if (IS_MULTIPLAY)
		cNetwork::Update();
		
	CombatManager::Post_Load_Level();
	
	
	//SaveLoadStatus::Set_Status_Text("Post_Load_Id_Uniqueness_Check", 0);
	loadingScreen.Render(true);
	Windows_Message_Handler();
	
	if (IS_MULTIPLAY)
		cNetwork::Update();
	
	Post_Load_Id_Uniqueness_Check();
	
	
	//SaveLoadStatus::Set_Status_Text("Post_Load_Dynamic_Object_Filtering", 0);
	loadingScreen.Render(true);
	Windows_Message_Handler();
	
	if (IS_MULTIPLAY)
		cNetwork::Update();
		
	Post_Load_Dynamic_Object_Filtering();
	
	
	//SaveLoadStatus::Set_Status_Text("Spawn_Point_Validation", 0);
	loadingScreen.Render(true);
	Windows_Message_Handler();
	
	if (IS_MULTIPLAY)
		cNetwork::Update();
	
	Spawn_Point_Validation();
	
	
	//SaveLoadStatus::Set_Status_Text("Compute world size", 0);

	Compute_World_Size();
	ControlClass::Set_Precision();
	HumanStateClass::Set_Precision();
	VehicleGameObj::Set_Precision();
	DoorPhysClass::Set_Precision();
	ElevatorPhysClass::Set_Precision();
	DefenseObjectClass::Set_Precision();
	BuildingGameObj::Set_Precision();
	
	if (!cNetwork::PClientConnection && cNetwork::PServerConnection)
		GameModeManager::BackgroundColor = Vector3(0, 0, .4f);
	
	
	//SaveLoadStatus::Set_Status_Text("Registry keys", 0);
	
	Load_Registry_Keys();
	Save_Registry_Keys();
	
	
	//SaveLoadStatus::Set_Status_Text("Init radar class", 0);
	
	loadingScreen.Render(true);
	Windows_Message_Handler();
	
	if (IS_MULTIPLAY)
		cNetwork::Update();
	
	RadarManager::Init();
	RadarManager::Set_Radar_Mode(The_Game()->RadarMode);
	The_Game()->Reset_Game(true);
	
	
	//SaveLoadStatus::Set_Status_Text("Init_Buildings", 0);
	
	loadingScreen.Render(true);
	Windows_Message_Handler();
	
	if (IS_MULTIPLAY)
		cNetwork::Update();

	GameObjManager::Init_Buildings();
	
	
	//SaveLoadStatus::Set_Status_Text("Init_Textures", 0);
	
	loadingScreen.Render(true);
	Windows_Message_Handler();
	
	if (IS_MULTIPLAY)
		cNetwork::Update();
	
	TextureLoader::Update(IS_MULTIPLAY ? cNetwork::Update : NULL);
	
	The_Game()->On_Game_Begin();
	
	ForceGodPending = false;
	
	if (IS_MULTIPLAY)
		cNetwork::Update();
	
	AssetStatusClass::Instance.unk0001 = true;
	cNetwork::g_is_loading = false;
	ConsoleBox.Print("Load 100%% complete\n");
	ConsoleBox.Print("Level loaded OK\n");
	GameSpyQnR.Init();
	
	if (cNetwork::PClientConnection)
		cNetwork::PClientConnection->Allow_Packet_Processing(true);
	if (cNetwork::PServerConnection)
		cNetwork::PServerConnection->Allow_Packet_Processing(true);

	DoLoadLevel2();
	
	/*
	if ( !ConsoleBox[284] )
	{
		v25 = FastAllocatorGeneral::Get_Allocator();
		v6 = v25;
		v8 = *(_DWORD *)(v25 + 4100);
		v7 = 0;
		do
		{
			CriticalSectionClass::LockClass::LockClass(&v34, v6 + 8 * v7 + 3072);
			v8 += *(_DWORD *)(v6 + 24 * v7 + 8);
			CriticalSectionClass::LockClass::_LockClass(&v34);
			++v7;
		}
		while ( v7 <= 127 );
		v35 = v8;
		v26 = FastAllocatorGeneral::Get_Allocator();
		v9 = v26;
		v11 = *(_DWORD *)(v26 + 4100);
		v10 = 0;
		do
		{
			CriticalSectionClass::LockClass::LockClass(&v36, v9 + 8 * v10 + 3072);
			v11 += *(_DWORD *)(v9 + 24 * v10 + 12);
			CriticalSectionClass::LockClass::_LockClass(&v36);
			++v10;
		}
		while ( v10 <= 127 );
		
		v37 = v11;
		v13 = *(_DWORD *)(FastAllocatorGeneral::Get_Allocator() + 4108);
		allocator = FastAllocatorGeneral::Get_Allocator();
		n = allocator->4104;
		
		for (uint i = 0; i < 128; i++)
		{
			CriticalSectionClass::LockClass lock(allocator->3072[i@8]);
			n += allocator->16[i@24];
		}
		
		v39 = (int)StringClass::m_EmptyString;
		StringClass::Get_String(&v39, 0, 1, v5);
		*(_BYTE *)v39 = StringClass::m_NullChar;
		v40 = 274877907i64 * v13 / 0x3E8;
		StringClass::Format((int)&v39, "\nMalloc count: %d\nFree count: %d\nFF Heap: %d.%3.3d.%3.3d (%d Mb)\nFF Use: %d.%3.3d.%3.3d (%d Mb)\nActual Use: %d.%3.3d.%3.3d (%d Mb)\nFF Count: %d\n", WW3D::LastFrameMemoryAllocations[0]);
		StringClass::Free_String(&v39);
	}
	*/
}



RENEGADE_FUNCTION
void CombatGameModeClass::Core_Shutdown()
AT2(0x00402E50, 0x00402E50);



RENEGADE_FUNCTION
void CombatGameModeClass::Post_Load_Id_Uniqueness_Check()
AT2(0x00402EC0, 0x00402EC0);



RENEGADE_FUNCTION
void CombatGameModeClass::Post_Load_Dynamic_Object_Filtering()
AT2(0x00402F70, 0x00402F70);



RENEGADE_FUNCTION
void CombatGameModeClass::Compute_World_Size()
AT2(0x00403020, 0x00403020);



RENEGADE_FUNCTION
void CombatGameModeClass::Spawn_Point_Validation()
AT2(0x00403140, 0x00403140);



RENEGADE_FUNCTION
void CombatGameModeClass::Core_Restart()
AT2(0x00403160, 0x00403160);



RENEGADE_FUNCTION
bool CombatGameModeClass::Load_Registry_Keys()
AT2(0x00403470, 0x00403470);



RENEGADE_FUNCTION
bool CombatGameModeClass::Save_Registry_Keys()
AT2(0x00403540, 0x00403540);


#include "engine_ttdef.h"
extern LoadLevelHook Think_Hook;

void CombatGameModeClass::Think()
{
	if (Think_Hook)
	{
		Think_Hook();
	}
	if (Is_Active())
	{
		Combat_Keyboard();
		
		if (Is_Active())
		{
			float networkUpdateTime = 0.f;
			float combatThinkTime = 0.f;
			
			CombatManager::Generate_Control();
			{
				WWMeasureItClass measure(&networkUpdateTime);
				cNetwork::Update();
			}
			{
				WWMeasureItClass measure(&combatThinkTime);
				CombatManager::Think();
			}
			
			if (cNetwork::PServerConnection)
			{
				cSbboManager::Increment_Accum_Time_S_Net_Update(networkUpdateTime);
				cSbboManager::Increment_Accum_Time_S_Combat_Think(combatThinkTime);
				cSbboManager::Think();
			}
			
			SoldierGameObj* theStar = CombatManager::Get_The_Star();
			if (theStar)
			{
				ConsoleGameModeClass::Get_Instance()->unk0158 = theStar->Get_Facing();
				theStar->Get_Position(&ConsoleGameModeClass::Get_Instance()->unk014C);
			}
			
			MultiHUDClass::Think();
			cPlayerManager::Think();
			cTeamManager::Think();
			
			if (PendingCampaignContinue)
			{
				PendingCampaignContinue = false;
				CampaignManager::Continue(true);
			}
			
			if (g_b_core_restart)
			{
				g_b_core_restart = false;

				cPlayer* player = cNetwork::Get_My_Player_Object();
				if (player)
				{
					//player->Set_Is_In_Game(false);
					(new cLoadingEvent())->Init(true);
					Core_Restart();
					//player->Set_Is_In_Game(true);
					(new cLoadingEvent())->Init(false);
				}
				else
					Core_Restart();

				if (!IS_MISSION)
				{
					if (cNetwork::I_Am_Server())
						cNetwork::Enable_Waiting_Players();

					if (cNetwork::I_Am_Client())
						MultiHUDClass::Init();
				}
			}
			
			if (CombatManager::Is_Autosave_Requested())
			{
				CombatManager::Request_Autosave(false);
				SaveGameManager::Description = TRANSLATE(IDS_SAVE_AUTOSAVE);
				SaveGameManager::Save_Game("save\\autosave.sav", CommandoSaveLoad);
			}
			
			if (g_client_quit)
			{
				g_client_quit = false;
				Suspend();
				GameInitMgrClass::End_Game();
				
				if (cGameSpyAdmin::IsLaunchedFromGamespy)
					Stop_Main_Loop(0);
				else
					GameInitMgrClass::Display_End_Game_Menu();
			}
		}
	}
}



void CombatGameModeClass::Render()
{
	if (Is_Active())
	{
		if (cNetwork::PClientConnection)
		{
			GameModeClass* menuGameMode = GameModeManager::Find("Menu");
			if (COMBAT_CAMERA && COMBAT_CAMERA->Is_Valid() && (!menuGameMode || !menuGameMode->Is_Active()))
				CombatManager::Render();

			if (RenderInfoTexture)
				InfoTextureRender2DClass->Render();
		}
		
		MultiHUDClass::Render();
		//cBandwidthGraph::Render();
		cPlayerManager::Render();
		cTeamManager::Render();
		The_Game()->Render();
		RadioCommandDisplayClass::Render();
	}
}



RENEGADE_FUNCTION
void CombatGameModeClass::Toggle_Multi_Hud()
AT2(0x00403950, 0x00403950);



RENEGADE_FUNCTION
void CombatGameModeClass::Resume()
AT2(0x004039A0, 0x004039A0);



RENEGADE_FUNCTION
void CombatGameModeClass::Suspend()
AT2(0x004039D0, 0x004039D0);



RENEGADE_FUNCTION
void CombatGameModeClass::Quick_Save()
AT2(0x00403A00, 0x00403A00);
