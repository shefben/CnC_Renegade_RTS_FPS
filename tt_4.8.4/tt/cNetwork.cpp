#include "General.h"
#include "cNetwork.h"


#include "string_ids.h"
#include "resource.h"

#include "dialogbase.h"
#include "dialogmgr.h"
#include "engine_game.h"
#include "TranslateDBClass.h"
#include "engine_diagnostics.h"

#include "NetworkObjectFactoryMgrClass.h"
#include "NetworkObjectFactoryClass.h"
#include "cAppPacketStats.h"
#include "cMsgStatList.h"
#include "CombatManager.h"
#include "cGameSpyAdmin.h"
#include "cSinglePlayerData.h"
#include "GameModeManager.h"
#include "cBandwidth.h"
#include "GameModeClass.h"
#include "cUserOptions.h"
#include "cNetInterface.h"
#include "cClientPingManager.h"
#include "cConnection.h"
#include "cClientGoodbyeEvent.h"
#include "cClientControl.h"
#include "CClientFps.h"
#include "NetworkObjectMgrClass.h"
#include "LanGameModeClass.h"
#include "cLanChat.h"
#include "WolGameModeClass.h"
#include "DlgMPConnectionRefused.h"
#include "DlgMsgBox.h"
#include "cMsgStatListGroup.h"
#include "cGameType.h"
#include "SlaveMasterClass.h"
#include "ServerSettingsClass.h"
#include "cTeam.h"
#include "cSbboManager.h"
#include "cPlayerManager.h"
#include "cRemoteHost.h"
#include "cTeamManager.h"
#include "cGameOptionsEvent.h"
#include "SysTimeClass.h"
#include "MessageWindowClass.h"
#include "PacketManagerClass.h"
#include "cServerFps.h"
#include "ConsoleModeClass.h"
#include "CombatNetworkReceiver.h"
#include "CCDKeyAuth.h"
#include "cGameDataUpdateEvent.h"
#include "SmartGameObj.h"
#include "NetworkDiagnostics.h"
#include "engine_ttdef.h"
#include "TimeManager.h"
#include "DlgCnCWinScreen.h"
#include "GameInitMgrClass.h"
#include "CombatGameModeClass.h"
#include "cClientHintManager.h"
#include "DlgMPTeamSelect.h"
#include "cChangeTeamEvent.h"
#include "VisTableClass.h"
#include "SoldierGameObj.h"
#include "PhysicsSceneClass.h"
#include "WWAudioClass.h"
#include "PacketManagerClass.h"
#include "CombatNetworkReceiverInstanceClass.h"
#include "cGod.h"
#include "GameObjManager.h"
#include "cPriority.h"
#include "ScopeProfiler.h"
#include "GameCombatNetworkHandlerClass.h"
#include "ResourceMgr/NetworkManager.h"
#include "ResourceMgr/DownloadManager.h"
#include "Iterator.h"
#include "FileHashEvent.h"
#include "NetworkDiagnostics.h"
#include "DebugManager.h"
#include "HookRegistrant.h"
extern bool Connected;
REF_DEF2(cNetwork::ExeKey, uint32, 0x00820D78, 0x0081FF60);
REF_DEF2(cNetwork::ExeCRC, sint32, 0x00820D7C, 0x0081FF64);
REF_DEF2(cNetwork::StringsCRC, sint32, 0x00820D80, 0x0081FF68);
REF_DEF2(cNetwork::NetworkReceiver, CombatNetworkReceiverInstanceClass*, 0x00820D84, 0x0081FF6C);
REF_DEF2(cNetwork::Fps, uint32, 0x00820D88, 0x0081FF70);
REF_DEF2(cNetwork::ThinkCount, uint32, 0x00820D8C, 0x0081FF74);
REF_DEF2(cNetwork::PClientStatList, cMsgStatList*, 0x00820D90, 0x0081FF78);
REF_DEF2(cNetwork::PServerStatListGroup, cMsgStatListGroup*, 0x00820D94, 0x0081FF7C);
REF_DEF2(cNetwork::PClientConnection, cConnection*, 0x00820D98, 0x0081FF80);
REF_DEF2(cNetwork::PServerConnection, cConnection*, 0x00820D9C, 0x0081FF84);
REF_DEF2(cNetwork::Receiver, CombatNetworkReceiver*, 0x00820DA0, 0x0081FF88);
REF_DEF2(cNetwork::HaveDoneTeamChangeDialog, bool, 0x00820DA8, 0x0081FF8C);
REF_DEF2(cNetwork::HaveDoneMotdDialog, bool, 0x00820DA9, 0x0081FF90);
REF_DEF2(cNetwork::VisTable, VisTableClass*, 0x00820DAC, 0x0081FF94);
REF_DEF2(cNetwork::LastServerConnectionStateBad, bool, 0x00820DB0, 0x0081FF98);
REF_DEF2(cNetwork::NetHandler, GameCombatNetworkHandlerClass*, 0x00820BDC, 0x0081FDC4);
REF_DEF2(cNetwork::g_is_loading, bool, 0x0081DED0, 0x0081D0B4);


int cNetwork::lastUpdatedClientId;
uint cNetwork::worstFps;



JumpHookRegistrant(0x004569A0, 0x00456AB0, cNetwork::Init_Client);

void cNetwork::Init_Client(Port localPort)
{
	TT_ASSERT(The_Game());

	if (PClientConnection)
		Cleanup_Client();

	PClientStatList = new cMsgStatList;
	PClientStatList->Init(1);
	PClientStatList->Set_Name(0, "message");

	PClientConnection = new cConnection;

	CombatManager::Set_I_Am_Client(true);

	PClientConnection->Install_Accept_Handler(Accept_Handler);
	PClientConnection->Install_Refusal_Handler(Refusal_Handler);
	PClientConnection->Install_Client_Broken_Connection_Handler(Client_Broken_Connection_Handler);
	PClientConnection->Install_Client_Packet_Handler(Client_Packet_Handler);

	if (cGameSpyAdmin::Is_Gamespy_Game())
		The_Game()->Password = cGameSpyAdmin::Get_Password_Attempt();

	int bandwidthLimit;
	if (IS_SOLOPLAY ||
		(GameModeManager::Find("LAN")->Is_Active() && !cGameSpyAdmin::Is_Gamespy_Game()))
	{
		bandwidthLimit = cBandwidth::Get_Bandwidth_Bps_From_Type(BANDWIDTH_LANT1);

		HaveDoneTeamChangeDialog = false;
	}
	else
	{
		bandwidthLimit = cBandwidth::Get_Bandwidth_Bps_From_Type(cUserOptions::Get_Bandwidth_Type());

		HaveDoneTeamChangeDialog = GameModeManager::Find("WOL")->Is_Active();
	}

	PClientConnection->Set_Bandwidth_Budget_Out(bandwidthLimit);
	PClientConnection->Enable_Flow_Control(!cSinglePlayerData::Is_Single_Player());

	PClientConnection->Init_As_Client(The_Game()->IP, The_Game()->Port, localPort);

	cPacket packet;
	packet.Add_Wide_Terminated_String(cNetInterface::Get_Nickname());
	packet.Add_Wide_Terminated_String(The_Game()->Password, true);
	packet.Add(ExeKey);
	packet.Add(bandwidthLimit);
	packet.Add(TT_VERSION);
	StringClass serial(true);
	CCDKeyAuth::GetSerialHash(serial);
	packet.Add_Terminated_String(serial);
	PClientConnection->Connect_Cs(packet);

	HaveDoneMotdDialog = false;

	if (I_Am_Only_Client())
		cAppPacketStats::Reset();
	
	LastServerConnectionStateBad = false;

	cClientPingManager::Init();
}



void cNetwork::Cleanup_Client()
{
	Connected = false;
	if (I_Am_Client())
	{
		if (PClientConnection->Is_Established())
		{
			cClientGoodbyeEvent* clientGoodbyeEvent = new cClientGoodbyeEvent;
			clientGoodbyeEvent->Init();
			Flush();
		}

		delete PClientConnection;
		PClientConnection = NULL;
	}

	CombatManager::Set_I_Am_Client(false);

	delete PClientStatList;
	PClientStatList = NULL;

	delete PClientControl;
	PClientControl = NULL;

	delete PClientFps;
	PClientFps = NULL;
}


void SendStoredEvents();
void cNetwork::Accept_Handler()
{
	TT_ASSERT(I_Am_Client());

	CombatManager::Set_My_Id(Get_My_Id());
	NetworkObjectMgrClass::Init_New_Client_ID(Get_My_Id());

	if (!I_Am_Server())
	{
		TT_ASSERT(!PClientControl);
		PClientControl = new CClientControl;
		PClientControl->Init();

		TT_ASSERT(!PClientFps);
		PClientFps = new CClientFps;
		PClientFps->Init();

		if (GameModeManager::Find("LAN")->Is_Active())
			if (!DialogMgrClass::Find_Dialog(IDD_MULTIPLAY_CONNECTING)) // HACK: Accept_Actions (which we do not own) creates this dialog, but that should not happen twice.
				PLC->Accept_Actions();

		WolGameModeClass* wolGameMode = (WolGameModeClass*)GameModeManager::Find("WOL");
		if (wolGameMode->Is_Active())
			wolGameMode->Accept_Actions();
	}
	Connected = true;
	SendStoredEvents();
}



void cNetwork::Refusal_Handler(REFUSAL_CODE refusalCode)
{
	TT_ASSERT(I_Am_Client());
	TT_ASSERT(refusalCode >= 0 && refusalCode < REFUSAL_CODE_COUNT && refusalCode != NO_REFUSAL);

	// Note that we can not destroy the connection in this callback!
	DialogBaseClass* dialog = DialogMgrClass::Find_Dialog(IDD_MULTIPLAY_CONNECTING);

	if (dialog)
	{
		// Sending 1 as the parameter tells the dialog that it is being closed
		// as a result of a refusal from the server.
		dialog->On_Command(IDCANCEL, 0, 1);
	}

	if (GameModeManager::Find("LAN")->Is_Active())
		PLC->Refusal_Actions();

	else
	{
		WolGameModeClass* wolGameMode = (WolGameModeClass*)GameModeManager::Find("WOL");

		if (wolGameMode && wolGameMode->Is_Active())
			wolGameMode->Refusal_Actions();
	}


#pragma warning(suppress: 4127) // conditional expression is constant
	TT_ASSERT(REFUSAL_CODE_COUNT == 6); // If the refusal codes were changed, the following table will no longer work properly.

	static const uint refusalMessages[] =
	{
		// NO_REFUSAL
		IDS_MP_CONNECTION_REFUSED_GAME_FULL, // REFUSAL_SERVER_FULL
		IDS_MP_PASSWORD_WRONG, // REFUSAL_INVALID_PASSWORD
		IDS_MENU_VERSION_MISMATCH, // REFUSAL_INVALID_VERSION
		IDS_MP_NICKNAME_IN_USE, // REFUSAL_INVALID_PLAYER
		IDS_MP_CONNECTION_REFUSED_BY_APPLICATION  // REFUSAL_CONNECTION_REFUSED
	};

	const wchar_t* refusalMessage = TRANSLATE(refusalMessages[refusalCode - 1]);

	if (cGameSpyAdmin::Is_Gamespy_Game())
		DlgMPConnectionRefused::DoDialog(refusalMessage, false);
	else
		DlgMsgBox::DoDialog(TRANSLATE(IDS_MENU_SERVER_MESSAGE_TITLE), refusalMessage);
}



// void cNetwork::Compute_Exe_Key(void)



void cNetwork::Onetime_Init()
{
	Compute_Exe_Key();
	Receiver = NetworkReceiver = new CombatNetworkReceiverInstanceClass();
	CombatManager::Set_Combat_Network_Handler(NetHandler);
}



RENEGADE_FUNCTION
void cNetwork::Onetime_Shutdown()
AT2(0x00457740, 0x004571F0);
/*
void cNetwork::Onetime_Shutdown()
{
	delete NetworkReceiver;
	REF_PTR_RELEASE(cNetwork::VisTable);
	
	cGameChannelList::Remove_All();
	cAppPacketStats::Dump_Diagnostics();
}
*/



void cNetwork::Init_Server()
{
	TT_ASSERT(!PServerConnection);
	TT_ASSERT(The_Game());

	NetworkObjectClass::Set_Is_Server(true);

	PServerStatListGroup = new cMsgStatListGroup;
	PServerStatListGroup->Init(The_Game()->MaxPlayers, 1);

	PServerConnection = new cConnection;

	CombatManager::Set_I_Am_Server(true);

	PServerConnection->Install_Server_Broken_Connection_Handler(Server_Broken_Connection_Handler);
	PServerConnection->Install_Eviction_Handler(Eviction_Handler);
	PServerConnection->Install_Conn_Handler(Connection_Handler);
	PServerConnection->Install_Application_Acceptance_Handler(Application_Acceptance_Handler);
	PServerConnection->Install_Server_Packet_Handler(Server_Packet_Handler);

	if (IS_SOLOPLAY ||
		(GameModeManager::Find("LAN")->Is_Active() && !cGameSpyAdmin::Is_Gamespy_Game()))
	{
		uint bandwidthLimit = cBandwidth::Get_Bandwidth_Bps_From_Type(BANDWIDTH_LANT1);
		PServerConnection->Set_Bandwidth_Budget_Out(bandwidthLimit);
	}
	else
	{
		TT_ASSERT(cUserOptions::BandwidthBps > 0);
		uint bandwidthLimit = cBandwidth::Get_Bandwidth_Bps_From_Type((BANDWIDTH_TYPE_ENUM)cUserOptions::Get_Bandwidth_Type());

		// Distribute bandwidth over slaves
		if (The_Game()->IsDedicated && !SlaveMaster.Am_I_Slave() &&
			cUserOptions::Get_Bandwidth_Type() == BANDWIDTH_AUTO &&
			(ServerSettingsClass::Get_Master_Bandwidth() == 0 || ServerSettingsClass::Get_Master_Bandwidth() == -1))
		{
			int slaveCount = SlaveMaster.Get_Num_Enabled_Slaves();
			if (slaveCount)
				bandwidthLimit = bandwidthLimit / (slaveCount+1);
		}

		PServerConnection->Set_Bandwidth_Budget_Out(bandwidthLimit);
	}

	PServerConnection->Set_Max_Acceptable_Packetloss_Pc(10.);
	PServerConnection->Enable_Flow_Control(IS_MULTIPLAY);

	PServerConnection->Init_As_Server(
		The_Game()->Port,
		The_Game()->MaxPlayers,
		The_Game()->IsDedicated,
		ntohl(The_Game()->IP));

	for (int teamId = 0; teamId < MAX_TEAMS; teamId++)
	{
		cTeam* team = new cTeam;
		team->Init(teamId);
	}

	cSbboManager::Reset();
	cAppPacketStats::Reset();
}



void cNetwork::Cleanup_Server()
{
	delete PServerConnection;
	PServerConnection = NULL;

	CombatManager::Set_I_Am_Server(false);

	delete PServerStatListGroup;
	PServerStatListGroup = NULL;

	NetworkObjectClass::Set_Is_Server(false);
}



// bool cNetwork::Save(ChunkSaveClass & csave)
// bool cNetwork::Load(ChunkLoadClass &cload)



void cNetwork::Update_Fps()
{
	static uint lastUpdateTime = 0;
	static int frameCount = 0;
	
	uint currentTime = TIMEGETTIME();
	uint timeInterval = currentTime - lastUpdateTime;
	frameCount++;

	bool updateFps = timeInterval > 500;

	if (updateFps)
	{
		Fps = (int)(frameCount * 1000 / (float)timeInterval + 0.5f);

		lastUpdateTime = currentTime;
		frameCount = 0;

		if (I_Am_Server())
		{
			TT_ASSERT(cServerFps::Get_Instance());
			cServerFps::Get_Instance()->Set_Fps(Fps);
		}

		if (I_Am_Client() && PClientFps)
			PClientFps->Set_Fps(Fps);
	}

	// For the worst FPS counter, we measure the worst frame time over 500 ms.
	// Four 500 ms periods are measured to make sure the worst frame time is
	// shown about 2 seconds.
	// This method ensures a immidiate response to a frame drop and shows this
	// number for 1.5 to 2 seconds.

	static uint periodIndex;
	static uint periodMaxFrameTime[4] = {0};
	uint frameTime = TimeManager::FrameTicks;

	if (updateFps)
	{
		periodIndex = (periodIndex + 1) % 4;
		periodMaxFrameTime[periodIndex] = frameTime;
	}
	else if (frameTime > periodMaxFrameTime[periodIndex])
		periodMaxFrameTime[periodIndex] = frameTime;

	uint maxFrameTime = max(max(periodMaxFrameTime[0], periodMaxFrameTime[1]), max(periodMaxFrameTime[2], periodMaxFrameTime[3]));
	if (maxFrameTime != 0.f)
		worstFps = 1000 / maxFrameTime;
	else
		worstFps = 999;
}



void cNetwork::Connection_Status_Change_Feedback()
{
	TT_ASSERT(I_Am_Client());

	if (!CombatManager::Get_Message_Window())
		return;

	static int connectionLostNotificationTime = 0;
	static int connectionRestoredNotificationTime = 0;

	int currentTime = TIMEGETTIME();
	if (PClientConnection->Is_Bad_Connection())
	{
		if (!connectionLostNotificationTime || currentTime - connectionLostNotificationTime >= 0)
		{
			// The next connection lost message should be repeated after 4 seconds.
			connectionLostNotificationTime = currentTime + 6000;
			CombatManager::Get_Message_Window()->Add_Message(WideStringClass(TRANSLATE(IDS_MENU_CONNECTION_INTERRUPTED)));
		}

		// The connection restored notification may only be sent if it has been working correctly for at least 2 seconds.
		connectionRestoredNotificationTime = currentTime + 1000;
	}
	else
	{
		// If it is time to send a connection restored message, send it and reset the connetion status.
		if (connectionRestoredNotificationTime && currentTime - connectionRestoredNotificationTime >= 0)
		{
			connectionLostNotificationTime = 0;
			connectionRestoredNotificationTime = 0;
			CombatManager::Get_Message_Window()->Add_Message(TRANSLATE(IDS_MENU_CONNECTION_RESTORED), Vector3(0, .9f, .2f));
		}
	}
}



void cNetwork::Update()
{
	// Assert that we do not accidentally call this function recursively.
	static int recursionLevel = 0;
	TT_ASSERT(++recursionLevel == 1);
	
	ThinkCount++;

	Update_Fps();

	bool shouldFlushPackets = false;
	if (I_Am_Server())
	{
		if (I_Am_Client())
		{
			PClientConnection->Service_Send();
			Connection_Status_Change_Feedback();
		}

		PServerConnection->Service_Read();

		if (!g_is_loading)
		{
			Shared_Client_And_Server_Think();

			if (I_Am_Client())
				Client_Think();

			shouldFlushPackets = Server_Think();
			
			PServerConnection->Service_Send();

			if (I_Am_Client())
				PClientConnection->Service_Read();
		}
	}
	else if (I_Am_Client())
	{
		PClientConnection->Service_Read();

		if (!g_is_loading)
		{
			Shared_Client_And_Server_Think();
			shouldFlushPackets = Client_Think();
		}

		if (PClientConnection)
		{
			PClientConnection->Service_Send();
			Connection_Status_Change_Feedback();
		}
	}

	NetworkObjectMgrClass::Delete_Pending();

	if (shouldFlushPackets)
		PacketManager().Flush(true);

	--recursionLevel;
}



void cNetwork::Client_Send_Packet(cPacket& packet, int mode)
{
	TT_ASSERT(I_Am_Client());
	TT_ASSERT(cNetwork::PClientConnection->Is_Established())

	PClientConnection->Send_Packet_To_Individual(packet, 0, (uint8)mode);
}



void cNetwork::Server_Send_Packet(cPacket& packet, int mode, int recipient)
{
	TT_ASSERT(I_Am_Server());
	TT_ASSERT(PServerConnection->Is_Established());

	if (recipient != ALL)
		PServerConnection->Send_Packet_To_Individual(packet, recipient, (uint8)mode);
	else
	{
		// We cannot just send to all rhosts because that includes anyone
		// browsing the server settings, whereas here we wish to send to the
		// ingame players.
		//PServerConnection->Send_Packet_To_All(*packet, mode);

		
		for (SLNode<cPlayer>* playerNode = cPlayerManager::Get_Player_Object_List().Head(); playerNode; playerNode = playerNode->Next())
		{
			cPlayer* player = playerNode->Data();
			TT_ASSERT(playerNode);

			if (player->Get_Is_Active() &&
				player->Is_Human() &&
				player->Get_Is_In_Game())
			{
				PServerConnection->Send_Packet_To_Individual(packet, player->Get_Id(), (uint8)mode);
			}
		}
	}
}



void cNetwork::Server_Send_Packet_To_All_Connected(cPacket & packet, int mode)
{
	TT_ASSERT(I_Am_Server());
	TT_ASSERT(PServerConnection->Is_Established());

	for (int remoteHostId = PServerConnection->Get_Min_RHost(); remoteHostId <= PServerConnection->Get_Max_RHost(); remoteHostId++)
		if (Get_Server_Rhost(remoteHostId))
			PServerConnection->Send_Packet_To_Individual(packet, remoteHostId, (uint8)mode);
}



// LPCSTR cNetwork::Get_Client_Enumeration_String(void)



cRemoteHost* cNetwork::Get_Server_Rhost(int remoteHostId)
{
	TT_ASSERT(I_Am_Server());
	return PServerConnection->Get_Remote_Host(remoteHostId);
}



cRemoteHost* cNetwork::Get_Client_Rhost()
{
	TT_ASSERT(I_Am_Client());
	return PClientConnection->Get_Remote_Host(SERVER_HOST_ID);
}



double cNetwork::Get_Server_Rhost_Threshold_Priority(int clientId)
{
	TT_ASSERT(Get_Server_Rhost(clientId));
	return Get_Server_Rhost(clientId)->Get_Threshold_Priority();
}



double cNetwork::Get_Client_Rhost_Threshold_Priority()
{
	TT_ASSERT(Get_Client_Rhost());
	return Get_Client_Rhost()->Get_Threshold_Priority();
}



int cNetwork::Get_My_Id()
{
	TT_ASSERT(I_Am_Client());
	return PClientConnection->Get_Local_Id();
}



// LPCSTR cNetwork::Get_Client_String(int recipient)



void cNetwork::Server_Broken_Connection_Handler(int clientId)
{
	WideStringClass message;
	message.Format(L"%s %d\n", TRANSLATE(IDS_MP_CONNECTION_TO_CLIENT_BROKEN), clientId);
	CombatManager::Get_Message_Window()->Add_Message(message);

	Vector3 color(1.f, 1.f, 0.f);
	ConsoleBox.Add_Message(&message, &color, false);
	WWAudioClass::Get_Instance()->Create_Instant_Sound("Broken_Connection", Matrix3D::Identity, NULL, 0, 2);

	cNetwork::Cleanup_After_Client(clientId);
}



void cNetwork::Client_Broken_Connection_Handler()
{
	TT_ASSERT(I_Am_Client());

	if (PClientConnection->Have_Id())
		DlgMsgBox::DoDialog(L"", TRANSLATE(IDS_MP_CONNECTION_TO_SERVER_BROKEN));

	extern bool& g_client_quit;
	g_client_quit = true;
}



// void cNetwork::Process_Eviction_Sc(cPacket & packet)
// void cNetwork::Eviction_Handler(int evicted_rhost_id)



bool cNetwork::I_Am_God()
{
	TT_UNREACHABLE;
}



cPlayer* cNetwork::Get_My_Player_Object()
{
	if (!I_Am_Client())
		return NULL;

	return cPlayerManager::Find_Player(Get_My_Id());
}



int cNetwork::Get_My_Team_Number()
{
	cPlayer* player = Get_My_Player_Object();
	TT_ASSERT(player);

	return player->PlayerType;
}



Vector3 cNetwork::Get_My_Color()
{
	cPlayer* player = Get_My_Player_Object();
	TT_ASSERT(player);

	return player->Get_Color();
}



BOOL cNetwork::Show_Welcome_Message(const WideStringClass& playerName)
{
	return !IS_MISSION && playerName == cNetInterface::Get_Nickname();
}



// Never used; approximately priority = .02/distance
float cNetwork::Get_Distance_Priority(const Vector3& position1, const Vector3& position2)
{
	float distance = (position2 - position1).Length();

	TT_ASSERT(The_Game());
	float maxDistance = The_Game()->MaxWorldDistance;
	TT_ASSERT(maxDistance > 0);

	float range1 = maxDistance / 25.0f;
	float range2 = maxDistance / 5.0f;
	float range3 = maxDistance + 1;

	float priority;

	if (distance < range1)
		priority = (float) ((range1 - distance) / range1 * 0.499 + 0.50);
	else if (distance < range2)
		priority = (float) ((range2 - distance) / (range2 - range1) * 0.40 + 0.10);
	else if (distance < range3)
		priority = (float) ((range3 - distance) / (range3 - range2) * 0.10 + 0.00);
	else
		priority = 0;

	TT_ASSERT(priority >= 0 && priority <= 1);

	return priority;

}


/*
void cNetwork::Shell_Command(const char* command)
{
	TT_ASSERT(command);

	HINSTANCE hinst = ShellExecute(NULL, NULL, command, NULL, "", SW_SHOW);
	TT_ASSERT(hinst > 32);
}
*/



REFUSAL_CODE cNetwork::Application_Acceptance_Handler(cPacket& packet)
{
	WideStringClass clientName(NULL, true);
	packet.Get_Wide_Terminated_String(clientName.Get_Buffer(256), 256, true);
	if (!isPlayerNameAllowed(clientName))
	{
		Console_Output("Player with invalid nickname blocked, player IP was %s\n", inet_ntoa(packet.getAddress().sin_addr));
		return REFUSAL_INVALID_PLAYER;
	}

	WideStringClass clientPassword(NULL, true);
	packet.Get_Wide_Terminated_String(clientPassword.Get_Buffer(256), 256, true);


	cGameData* gameData = The_Game();
	TT_ASSERT(gameData);

	if (gameData->IsPassworded && clientPassword.Compare(gameData->Password) != 0)
		return REFUSAL_INVALID_PASSWORD;

	uint32 clientExeKey;
	packet.Get(clientExeKey);
	if (clientExeKey != ExeKey)
		return REFUSAL_INVALID_VERSION;

	if (gameData->CurrentPlayers >= gameData->MaxPlayers)
		return REFUSAL_SERVER_FULL;
	
	return NO_REFUSAL;
}



void cNetwork::Connection_Handler(int clientId)
{
	TT_ASSERT(clientId > 0);
	TT_ASSERT(I_Am_Server());
	TT_ASSERT(Receiver);

	// Don't send if we are the host of a nondedicated game. (This is the case when the local client id is not yet set)
	if (!(I_Am_Client() && PClientConnection->Get_Local_Id() == INVALID_HOST_ID))
	{
		TT_ASSERT(!I_Am_Client() || clientId != cNetwork::Get_My_Id());
		
		for (Iterator<SList<cTeam>> teamIterator(cTeamManager::Get_Team_Object_List()); teamIterator; ++teamIterator)
			Send_Object_Update(teamIterator, clientId);

		cGameOptionsEvent* gameOptionsEvent = new cGameOptionsEvent();
		gameOptionsEvent->Init(clientId);
		Send_Object_Update(gameOptionsEvent, clientId);
	}
}



// void cNetwork::Set_Desired_Frame_Sleep_Ms(int b)
// void cNetwork::Set_Simulated_Packet_Loss_Pc(int b)
// void cNetwork::Set_Simulated_Packet_Duplication_Pc(int b)
// void cNetwork::Set_Simulated_Latency_Range_Ms(int lower, int upper)
// void cNetwork::Set_Spam_Count(int spam_count)
// void cNetwork::Get_Simulated_Latency_Range_Ms(int & lower, int & upper)
// void cNetwork::SwitchTeam(int newTeam)



void cNetwork::Enable_Waiting_Players()
{
	if (cNetwork::PServerConnection)
	{
		for (SLNode<cPlayer>* playerNode = cPlayerManager::Get_Player_Object_List().Head(); playerNode; playerNode = playerNode->Next())
		{
			cPlayer* player = playerNode->Data();
			if (player->Is_Human() && player->Get_Is_Waiting_For_Intermission())
			{
				player->Set_Is_In_Game(true);
				player->Set_Is_Waiting_For_Intermission(false);
				cNetwork::PServerConnection->Set_Rhost_Is_In_Game(player->Get_Id(), true);
				(new cGameDataUpdateEvent())->Init(player->Get_Id());
			}
		}
	}
}



uint cNetwork::Send_Object_Update(NetworkObjectClass* object, int clientId)
{
	TT_ASSERT(object);
	TT_ASSERT(clientId > 0 ? PServerConnection : PClientConnection);
	TT_ASSERT(clientId > 0 ? PServerConnection->Is_Established() : PClientConnection->Is_Established()); // TODO: Triggers when the client fails to connect. Is this a problem?

	uint result = 0;

	// Only update if the object is to be created, or has been created before for this client.
	// This avoids duplicate transmimssions.
	if (object->Get_Object_Dirty_Bit(clientId, DB_CREATION) || !object->Get_Object_Dirty_Bit(clientId, DB_NOT_CREATED))
	{
		lastUpdatedClientId = clientId;
		
		cPacket packet;
		packet.Add(object->Get_Network_ID());
		packet.Add(object->Get_Object_Dirty_Bits(clientId));
		packet.Add(object->Is_Delete_Pending());
		uint firstWritePosition = packet.Get_Bit_Write_Position();
		uint8 packetType = SEND_UNRELIABLE;
		
		if (object->Is_Delete_Pending())
			packetType = SEND_RELIABLE;
		
		if (object->Get_Object_Dirty_Bit(clientId, DB_CREATION))
		{
			uint previousWritePosition = packet.Get_Bit_Write_Position();
			packet.Add(object->Get_Network_Class_ID());
			NetworkObjectFactoryClass* factory = NetworkObjectFactoryMgrClass::Find_Factory(object->Get_Network_Class_ID());
			TT_ASSERT(factory);
			
			factory->Prep_Packet(object, packet);
			object->Export_Creation(packet);
			cAppPacketStats::Increment_Bits_Sent_Tier(object->Get_App_Packet_Type(), PACKET_TIER_CREATION, packet.Get_Bit_Write_Position() - previousWritePosition);
			packetType = SEND_RELIABLE;
		}
		if (object->Get_Object_Dirty_Bit(clientId, DB_RARE))
		{
			uint previousWritePosition = packet.Get_Bit_Write_Position();
			object->Export_Rare(packet);
			cAppPacketStats::Increment_Bits_Sent_Tier(object->Get_App_Packet_Type(), PACKET_TIER_RARE, packet.Get_Bit_Write_Position() - previousWritePosition);
			packetType = SEND_RELIABLE;
		}
		if (object->Get_Object_Dirty_Bit(clientId, DB_OCCASIONAL))
		{
			uint previousWritePosition = packet.Get_Bit_Write_Position();
			object->Export_Occasional(packet);
			cAppPacketStats::Increment_Bits_Sent_Tier(object->Get_App_Packet_Type(), PACKET_TIER_OCCASIONAL, packet.Get_Bit_Write_Position() - previousWritePosition);
			packetType = SEND_RELIABLE;
		}
		if (object->Get_Object_Dirty_Bit(clientId, DB_FREQUENT))
		{
			uint previousWritePosition = packet.Get_Bit_Write_Position();
			object->Export_Frequent(packet);
			cAppPacketStats::Increment_Bits_Sent_Tier(object->Get_App_Packet_Type(), PACKET_TIER_FREQUENT, packet.Get_Bit_Write_Position() - previousWritePosition);
		}

		if (object->isUnreliable()) // Unreliable override
			packetType = SEND_UNRELIABLE;
		
		uint lastWritePosition = packet.Get_Bit_Write_Position();
		
		if (object->Is_Delete_Pending() || lastWritePosition > firstWritePosition)
		{
	#if NET_DIAG
			networkDiagnostics.onObjectUpdateSent(*object, clientId);
	#endif

			result = lastWritePosition;
			if (clientId <= 0)
				cNetwork::Client_Send_Packet(packet, packetType);
			else
				cNetwork::Server_Send_Packet(packet, packetType, clientId);
		}
		
		cAppPacketStats::Increment_Packets_Sent(object->Get_App_Packet_Type());
		cAppPacketStats::Increment_Bits_Sent(object->Get_App_Packet_Type(), lastWritePosition - firstWritePosition);

		// Reset all dirty bits and set the created flag if applicable.
		object->Set_Object_Dirty_Bits(clientId, object->Is_Delete_Pending() ? DB_NOT_CREATED : 0);
	}
	
	return result;
}



void cNetwork::Broadcast_Object_Update(NetworkObjectClass& object)
{
	for (Iterator<SList<cPlayer>> player(PlayerList); player; ++player)
		cNetwork::Send_Object_Update(&object, player->PlayerId);
}



void cNetwork::Server_Kill_Connection(int playerId)
{
	PServerConnection->Destroy_Connection(playerId);
}



void cNetwork::Cleanup_After_Client(int clientId)
{
	Remove_Player(clientId);
	Delete_Player_Objects(clientId);
	NetworkObjectMgrClass::Delete_Client_Objects(clientId);
	Receiver->Server_Send_Delete_Notifications();
	NetworkObjectMgrClass::Restore_Dirty_Bits(clientId);
	CCDKeyAuth::DisconnectUser(clientId);
}



bool cNetwork::isPlayerNameAllowed(const WideStringClass& playerName)
{
	// Check the length
	const int playerNameLength = playerName.Get_Length();
	if (playerNameLength <= 0 || playerNameLength > 35)
		return false;
	
	// Check whether the player is using the server name
	if (!cNetwork::I_Am_Client_Server() && playerName.Compare(cNetInterface::Get_Nickname()) == 0)
		return false;

	// Check whether the name contains any invalid characters (spaces at the begin and end and double spaces are not allowed)
	bool isPreviousCharacterWhitespace = true; // Setting this to true makes sure there is no space at the beginning of the name.

	for (int i = 0; i < playerNameLength; i++)
	{
		wchar_t character = playerName[i];

		if (character == ' ')
		{
			if (isPreviousCharacterWhitespace)
				return false;

			isPreviousCharacterWhitespace = true;
		}
		else if (character >= ' ' && character <= '~')
			isPreviousCharacterWhitespace = false;
		else
			return false;
	}

	if (isPreviousCharacterWhitespace) // This only happens when the name ends with a space.
		return false;

	// Check whether the player already is ingame.
	if (cPlayerManager::Find_Player(playerName))
		return false;

	return true;
}



RENEGADE_FUNCTION
void cNetwork::Compute_Exe_Key()
AT2(0x00457450, 0x00456F00);



RENEGADE_FUNCTION
bool cNetwork::Save(ChunkSaveClass&)
AT2(0x00457A50, 0x00457500);



RENEGADE_FUNCTION
bool cNetwork::Load(ChunkLoadClass&)
AT2(0x00457A80, 0x00457530);



RENEGADE_FUNCTION
void cNetwork::Get_Client_Enumeration_String()
AT2(0x004580D0, 0x00457B60);



RENEGADE_FUNCTION
const char* cNetwork::Get_Client_String(int)
AT2(0x00458270, 0x00457D00);



RENEGADE_FUNCTION
void cNetwork::Process_Eviction_Sc(cPacket&)
AT2(0x004585A0, 0x00458030);



RENEGADE_FUNCTION
void cNetwork::Eviction_Handler(int)
AT2(0x00458650, 0x004580E0);



RENEGADE_FUNCTION
void cNetwork::Shell_Command(const char*)
AT2(0x00458B20, 0x004585B0);



RENEGADE_FUNCTION
void cNetwork::Set_Desired_Frame_Sleep_Ms(int)
AT2(0x00458D80, 0x00458810);



RENEGADE_FUNCTION
void cNetwork::Set_Simulated_Packet_Loss_Pc(int)
AT2(0x00458D90, 0x00458820);



RENEGADE_FUNCTION
void cNetwork::Set_Simulated_Packet_Duplication_Pc(int)
AT2(0x00458DD0, 0x00458860);



RENEGADE_FUNCTION
void cNetwork::Set_Simulated_Latency_Range_Ms(int, int)
AT2(0x00458E10, 0x004588A0);



RENEGADE_FUNCTION
void cNetwork::Set_Spam_Count(int)
AT2(0x00458E40, 0x004588D0);



RENEGADE_FUNCTION
void cNetwork::Get_Simulated_Latency_Range_Ms(int&, int&)
AT2(0x00458E50, 0x004588E0);



void cNetwork::Flush()
{
	if (Receiver)
	{
		if (I_Am_Server())
			Receiver->Server_Update_Dynamic_Objects(true);
		
		if (I_Am_Client())
			Receiver->Client_Update_Dynamic_Objects(true);
	}

	if (I_Am_Server())
		PServerConnection->Service_Send(true);

	if (I_Am_Client())
		PClientConnection->Service_Send(true);

	PacketManager().Flush(true);
}



RENEGADE_FUNCTION
void cNetwork::SwitchTeam(int)
AT2(0x00458EC0, 0x00458950);
/*
{
	cPlayer* player = cPlayerManager::Find_Player(Get_My_Id());
	if (player && teamId != player->PlayerType)
		(new cChangeTeamEvent)->Init();
}
*/


/*
RENEGADE_FUNCTION
void cNetwork::Tell_Client_About_Dynamic_Objects(int clientId, const Vector3& clientPosition)
AT2(0x00460750, 0x00460250);
*/
void cNetwork::Tell_Client_About_Dynamic_Objects(int clientId, const Vector3& clientPosition)
{
	const uint16 MINIMAL_UPDATE_PERIOD = 140;//140;
	const uint16 MAXIMAL_UPDATE_PERIOD = 5000;//5000;

	TT_ASSERT(cNetwork::I_Am_Server());
	TT_ASSERT(clientId >= 0);
	
	if (cNetwork::I_Am_Client() && clientId == cNetwork::Get_My_Id())
		return;
	
	cRemoteHost* clientHost = cNetwork::Get_Server_Rhost(clientId);
	if (!clientHost)
		return;
	
	uint currentTime = TIMEGETTIME();

	static uint lastUpdateTime[128] = {0};
	if (lastUpdateTime[clientId] == 0)
		lastUpdateTime[clientId] = currentTime;
	int timeStep = currentTime - lastUpdateTime[clientId];
	lastUpdateTime[clientId] = currentTime;
	
	bool updatePriorities = clientHost->priorityUpdateTimerUpdate(currentTime);

	uint64 bandwidth = clientHost->Get_Target_Bps();

	if (!clientHost->Get_Flood())
		bandwidth = (uint64)(bandwidth * Get_Server_Rhost(clientId)->Get_Bandwidth_Multiplier());

	uint64 availableBits = bandwidth * timeStep / 1000;
	uint64 availableImportantBits = availableBits * 3 / 4;
	
	float min_vis_distance = 15.0f;
	if (bandwidth > 60000)
		min_vis_distance = 50.0f;
	
	uint64 importantBitsSent = 0;
	uint64 unimportantBitsSent = 0;
	uint importantUpdateCount = 0;
	uint unimportantUpdateCount = 0;
	
	VisTableClass* playerVisTable = NULL;
	if (updatePriorities)
		playerVisTable = COMBAT_SCENE->Get_Vis_Table(clientPosition);
	
	static SimpleDynVecClass<NetworkObjectClass*> pendingObjectList(500);
	pendingObjectList.Delete_All(false);
	
	SoldierGameObj* clientSoldier = GameObjManager::Find_Soldier_Of_Client_ID(clientId);
	
	float prioritySum = 0.f;
	
	int count = NetworkObjectMgrClass::Get_Object_Count();
	for (int index = 0; index < count; ++index)
	{
		NetworkObjectClass* object = NetworkObjectMgrClass::Get_Object(index);

		const byte IMPORTANT_DIRTY_BIT_MASK = (DB_CREATION | DB_RARE | DB_OCCASIONAL) & (~DB_FREQUENT);
		if (object->Get_Object_Dirty_Bits(clientId) & IMPORTANT_DIRTY_BIT_MASK)
		{
			if (importantBitsSent < availableImportantBits)
			{
				importantBitsSent += Send_Object_Update(object, clientId);
				++importantUpdateCount;
				object->Set_Last_Update_Time(clientId, currentTime);
			}
		}
		
		if (object->Get_Object_Dirty_Bits(clientId) & DB_FREQUENT)
		{
			float priority = 0.f;
			if (object->Get_App_Packet_Type() == 19) // 19 = APPPACKETTYPE_SERVERFPS
				priority = .05f;
			else
			{
				priority = object->Get_Cached_Priority_2(clientId);
				if (object == clientSoldier)
				{
					if (clientSoldier->Is_In_Vehicle())
						priority = .1f;
					else
						priority = .8f;
				}
				else if (object->Get_Client_Hint_Count(clientId) > 0)
				{
					priority = 1.f;
					object->Reset_Client_Hint_Count(clientId);
				}
				else if (updatePriorities)
				{
					int objectVisId = object->Get_Vis_ID();
					if (objectVisId == -1 || !playerVisTable || playerVisTable->Get_Bit(objectVisId))
						priority = cPriority::Compute_Object_Priority_2(clientId, clientPosition, object, false, clientSoldier);
					else
					{
						float distance = cPriority::Get_Object_Distance_2(clientPosition, object);
						if (distance <= min_vis_distance)
							priority = .2f;
						else if (bandwidth > 100000 && distance < 150.0f)
							priority = .01f;
						else
							priority = 0.f;
					}
				}
			}

			if (priority > .009f)
			{
				prioritySum += priority;
				pendingObjectList.Add(object);
			}
			
			object->Set_Cached_Priority_2(clientId, priority);
		}
	}
	
	uint64 availableUnimportantBits = availableBits - unimportantBitsSent;

	uint64 unscaledUnimportantBandwidth = 0;
	if (availableUnimportantBits > 0)
	{
		if (pendingObjectList.Count() == 0)
			Get_Server_Rhost(clientId)->Set_Average_Priority(0.f);
		else
		{
			Get_Server_Rhost(clientId)->Set_Average_Priority(prioritySum / pendingObjectList.Count());
			
			for (int i = 0; i < pendingObjectList.Count(); ++i)
			{
				NetworkObjectClass* object = pendingObjectList[i];
				float objectPriority = object->Get_Cached_Priority_2(clientId);
				
				uint16 objectUpdatePeriod = lerp(MAXIMAL_UPDATE_PERIOD, MINIMAL_UPDATE_PERIOD, objectPriority); // Reverse linear
				object->setUpdatePeriod(clientId, objectUpdatePeriod);

				
				uint8 frequentUpdateSize = object->Get_Frequent_Update_Export_Size();
				if (frequentUpdateSize == 0)
				{
					cPacket packet;
					int bits_before = packet.Get_Bit_Write_Position();
					packet.Add(object->Get_Network_ID());
					packet.Add(object->Get_Object_Dirty_Bits(clientId));
					packet.Add(object->Is_Delete_Pending());
					int bits_now = packet.Get_Bit_Write_Position();
					object->Export_Frequent(packet);
					int bits_after = packet.Get_Bit_Write_Position();
					
					if (bits_now < bits_after)
						frequentUpdateSize = (uint8)(bits_after - bits_before);
					else
						frequentUpdateSize = 0xFF;
					
					object->Set_Frequent_Update_Export_Size(frequentUpdateSize);
				}

				if (frequentUpdateSize != 0xFF)
					unscaledUnimportantBandwidth += frequentUpdateSize * 1000 / objectUpdatePeriod;
			}
			
			float objectUpdatePeriodFactor = (float)unscaledUnimportantBandwidth * timeStep / 1000 / availableUnimportantBits;
			for (int i = 0; i < pendingObjectList.Count(); ++i)
			{
				if (unimportantBitsSent >= availableUnimportantBits)
					break;

				NetworkObjectClass* object = pendingObjectList[i];
				uint16 objectUpdatePeriod = object->getUpdatePeriod(clientId);
				objectUpdatePeriod = (uint16)(objectUpdatePeriod * objectUpdatePeriodFactor);
				object->setUpdatePeriod(clientId, objectUpdatePeriod);

				if (currentTime - object->Get_Last_Update_Time(clientId) > objectUpdatePeriod)
				{
					unimportantBitsSent += Send_Object_Update(object, clientId);
					++unimportantUpdateCount;
					object->Set_Last_Update_Time(clientId, currentTime);
				}
			}
		}
	}
#if NET_DIAG
	FILE* file = fopen("net.log", "at");
	static bool writeHeader = true;
	if (file)
	{
		if (writeHeader)
		{
			fprintf(file, "time\ttimeStep\tclientId\tbandwidth\ttargetBandwidth\timportantBitsSent\tunimportantBitsSent\timporatantUpdateCount\tunimportantUpdateCount\n");
			writeHeader = false;
		}

		fprintf
		(
			file,
			"%d\t%d\t%d\t%I64d\t%d\t%I64d\t%I64d\t%d\t%d\n",
			(uint32)DebugTimer::getMicroTime(),
			timeStep,
			clientId,
			bandwidth,
			clientHost->getTargetBandwidth(),
			importantBitsSent,
			unimportantBitsSent,
			importantUpdateCount,
			unimportantUpdateCount
		);

		fclose(file);
	}
#endif
	
	REF_PTR_RELEASE(playerVisTable);
}



void cNetwork::Tell_Server_About_Dynamic_Objects()
{
	TT_ASSERT(cNetwork::I_Am_Client());

	int objectCount = NetworkObjectMgrClass::Get_Object_Count();
	for (int objectIndex = 0; objectIndex < objectCount; ++objectIndex)
	{
		NetworkObjectClass* object = NetworkObjectMgrClass::Get_Object(objectIndex);
		if (object->Is_Client_Dirty(SERVER_HOST_ID))
			Send_Object_Update(object, SERVER_HOST_ID);
	}
}



void cNetwork::Tell_Client_About_Delete_Notifications(int clientId)
{
	TT_ASSERT(clientId > 0);
	TT_ASSERT(cNetwork::I_Am_Server());

	if (!Get_Server_Rhost(clientId))
		return;

	if (cNetwork::I_Am_Client() && clientId == cNetwork::Get_My_Id())
		return;

	int objectCount = NetworkObjectMgrClass::Get_Object_Count();
	for (int objectIndex = 0; objectIndex < objectCount; ++objectIndex)
	{
		NetworkObjectClass* object = NetworkObjectMgrClass::Get_Object(objectIndex);
		TT_ASSERT(object);

		if (object->Is_Delete_Pending())
			Send_Object_Update(object, clientId);
	}
}



void cNetwork::Tell_Server_About_Delete_Notifications()
{
	TT_ASSERT(cNetwork::I_Am_Client());

	if (!Get_Client_Rhost())
		return;

	if (cNetwork::I_Am_Server())
		return;

	int objectCount = NetworkObjectMgrClass::Get_Object_Count();
	for (int objectIndex = 0; objectIndex < objectCount; ++objectIndex)
	{
		NetworkObjectClass* object = NetworkObjectMgrClass::Get_Object(objectIndex);
		TT_ASSERT(object);

		if (object->Is_Delete_Pending())
			Send_Object_Update(object, SERVER_HOST_ID);
	}
}



void cNetwork::Intermission_Over_Processing()
{
	TT_ASSERT(cNetwork::I_Am_Server());

	DlgCnCWinScreen::Close_Dialog();

	for (SLNode<cPlayer>* playerNode = cPlayerManager::Get_Player_Object_List().Head(); playerNode; playerNode = playerNode->Next())
	{
		cPlayer* player = playerNode->Data();
		Delete_Player_Objects(player->Get_Id());

		if (player->Is_Human())
		{
			player->Set_Is_In_Game(false);
			cNetwork::PServerConnection->Set_Rhost_Is_In_Game(player->Get_Id(), false);
		}
	}

	if (cGameData::Is_Manual_Exit())
		GameInitMgrClass::Set_Needs_Game_Exit_All(true);
	else
	{
		// If we have to quit out now then don't do a core restart.
		if (!The_Game() || The_Game()->MapCycleOver)
			GameInitMgrClass::Set_Needs_Game_Exit(true);
		else
		{
			WolGameModeClass* wolGameMode = (WolGameModeClass*)GameModeManager::Find("WOL");
			g_b_core_restart = !(wolGameMode->Is_Active() && wolGameMode->Post_Game_Check());
		}
	}

	TT_ASSERT(The_Game());
	The_Game()->IsIntermission = false;
}



void cNetwork::End_Game_Test()
{
	TT_ASSERT(cNetwork::I_Am_Server());

	if (IS_MISSION || !GameModeManager::Find("Combat")->Is_Active())
		return;

	TT_ASSERT(The_Game());
	if (The_Game()->IsIntermission)
	{
		if (The_Game()->IntermissionTimeLeft <= 0)
			Intermission_Over_Processing();
	}
	else
	{
		bool isGameOver = The_Game()->Is_Game_Over();
		bool sort = isGameOver;

		if (!sort)
		{
			static int LastSortedSecond = 0;
			int seconds = cMathUtil::Round(TimeManager::SystemTicks() / 1000.f);
			if (seconds != LastSortedSecond)
			{
				LastSortedSecond = seconds;
				sort = true;
			}
		}

		if (sort)
		{
			cTeamManager::Sort_Teams();
			cPlayerManager::Sort_Players(true);
		}

		if (isGameOver)
			The_Game()->Game_Over_Processing();
	}
}



void cNetwork::Shared_Client_And_Server_Think()
{
	TT_ASSERT(I_Am_Client() || I_Am_Server());
	TT_ASSERT(The_Game());

	The_Game()->Think();
	Hibernation_Think();
}



bool cNetwork::Client_Think()
{
	bool result = false;

	if (PClientConnection->Is_Destroy())
	{
		Cleanup_Client();
		return result;
	}

	if (!PClientConnection->Have_Id())
		return result;

	TT_ASSERT(The_Game());
	if (I_Am_Only_Client() && The_Game()->IsIntermission && The_Game()->IntermissionTimeLeft <= 0)
	{
		DlgCnCWinScreen::Close_Dialog();

		if (The_Game()->MapCycleOver || !The_Game()->Is_Map_Valid(NULL))
			GameInitMgrClass::Set_Needs_Game_Exit(true);
		else
			g_b_core_restart = true;
	}

	cClientPingManager::Think();
	cClientHintManager::Think();

	TT_ASSERT(Receiver);
	result = Receiver->Client_Update_Dynamic_Objects(false);

	if (!HaveDoneTeamChangeDialog && DlgMsgBox::Get_Current_Count() == 0 && GameModeManager::Find("Combat")->Is_Active())
	{
		HaveDoneTeamChangeDialog = true;
		if (cChangeTeamEvent::Is_Change_Team_Possible())
			DlgMPTeamSelect::DoDialog(*The_Game());
	}

	if (!HaveDoneMotdDialog && DlgMsgBox::Get_Current_Count() == 0 && GameModeManager::Find("Combat")->Is_Active())
	{
		HaveDoneMotdDialog = true;
		
		if (!I_Am_Server() && The_Game() && wcslen(The_Game()->Motd) > 0)
			DlgMsgBox::DoDialog(TRANSLATE(IDS_MENU_MOTD), The_Game()->Motd);
	}

	downloadManager.think();
	clientNetworkManager.think();

	return result;
}



RENEGADE_FUNCTION
VisTableClass* cNetwork::Peek_Temp_Vis_Table()
AT2(0x00461E60, 0x00461790);
/*
{
	if (!VisTable || VisTable->Get_Bit_Count() != COMBAT_SCENE->Get_Vis_Table_Size())
	{
		REF_PTR_RELEASE(VisTable);
		VisTable = new VisTableClass(COMBAT_SCENE->Get_Vis_Table_Size(), 0);
	}
	else
		VisTable->Reset_All();

	return VisTable;
}
*/



void cNetwork::Hibernation_Think()
{
	if (COMBAT_SCENE)
	{
		// The server can't let any player go into hibernation or updates will cease to be sent for that player.
		if (I_Am_Server() && !IS_MISSION)
		{
			for (SLNode<BaseGameObj>* objectNode = (SLNode<BaseGameObj>*)GameObjManager::GameObjList.Head(); objectNode; objectNode = objectNode->Next())
			{
				PhysicalGameObj* object = objectNode->Data()->As_PhysicalGameObj();
				if (object)
					object->Reset_Hibernating();
			}
		}
		else
		{
			if (COMBAT_STAR)
			{
				VisTableClass* visTable = Peek_Temp_Vis_Table();
				TT_ASSERT(visTable);
				
				// Build vis for all player objects.
				/*
				for (SLNode<SmartGameObj>* objectNode = (SLNode<SmartGameObj>*)GameObjManager::SmartGameObjList.Head(); objectNode; objectNode = objectNode->Next())
				{
					SmartGameObj* object = objectNode->Data();
					TT_ASSERT(object);
					
					SoldierGameObj* soldierObject = object->As_SoldierGameObj();
					
					if (soldierObject && soldierObject->Has_Player())
					{
						Vector3 soldierHeadPosition;
						soldierObject->Get_Position(&soldierHeadPosition);
						soldierHeadPosition.Z += 2; // Start near the player's head
						
						VisTableClass* soldierVisTable = COMBAT_SCENE->Get_Vis_Table(soldierHeadPosition);
						if (!soldierVisTable)
						{
							visTable = NULL;
							break;
						}
						else
						{
							visTable->Merge(*soldierVisTable);
							REF_PTR_RELEASE(soldierVisTable);
						}
					}
				}
				*/

				// Build vis for star
				SmartGameObj* object = COMBAT_STAR;
				TT_ASSERT(object);
				
				SoldierGameObj* soldierObject = object->As_SoldierGameObj();
				
				if (soldierObject && soldierObject->Has_Player())
				{
					Vector3 soldierHeadPosition;
					soldierObject->Get_Position(&soldierHeadPosition);
					soldierHeadPosition.Z += 2; // Start near the player's head
					
					VisTableClass* soldierVisTable = COMBAT_SCENE->Get_Vis_Table(soldierHeadPosition);
					if (!soldierVisTable)
						visTable = NULL;
					else
					{
						visTable->Merge(*soldierVisTable);
						REF_PTR_RELEASE(soldierVisTable);
					}
				}
				
				Vector3	starPosition;
				COMBAT_STAR->Get_Position(&starPosition);
				
				// Reset hibernating on anything visible
				for (SLNode<BaseGameObj>* objectNode = (SLNode<BaseGameObj>*)GameObjManager::GameObjList.Head(); objectNode; objectNode = objectNode->Next())
				{
					PhysicalGameObj* object2 = objectNode->Data()->As_PhysicalGameObj();
					if (object2)
					{
						PhysClass* objectPhys = object2->Peek_Physical_Object();
						if (objectPhys)
						{
							if (!visTable || visTable->Get_Bit(objectPhys->Get_Vis_Object_ID()))
							{
								Vector3 objectPosition;
								object2->Get_Position(&objectPosition);
								if ((objectPosition - starPosition).Length2() < 300*300)
									object2->Reset_Hibernating();
							}
						}
					}
				}
			}
		}
	}
}



bool cNetwork::Server_Think()
{
	TT_ASSERT(I_Am_Server());
	TT_ASSERT(Receiver);

	if (GameModeManager::Find("Combat")->Is_Active())
		cGod::Think();

	End_Game_Test();

	if (I_Am_Server())
	{
		Receiver->Server_Send_Delete_Notifications();
		return Receiver->Server_Update_Dynamic_Objects(false);
	}
	else 
		return false;
}



void cNetwork::Delete_Player_Objects(int clientId)
{
	TT_ASSERT(I_Am_Server());

	for (SLNode<SmartGameObj>* objectNode = (SLNode<SmartGameObj>*)GameObjManager::SmartGameObjList.Head(); objectNode; objectNode = objectNode->Next())
	{
		SmartGameObj* object = objectNode->Data();
		if (object->Get_Control_Owner() == clientId)
			object->Set_Delete_Pending();
	}
}



void cNetwork::Remove_Player(int clientId)
{
	TT_ASSERT(I_Am_Server());

	cPlayer* player = cPlayerManager::Find_Player(clientId);
	if (player)
	{
		extern PlayerLeave Player_Leave_Hook;
		if (Player_Leave_Hook)
			Player_Leave_Hook(player->PlayerId);
		
		player->Increment_Total_Time();
		player->Set_Is_Active(false);

		ConsoleBox.Print_Maybe("Player %S left the game\n", player->PlayerName);
	}
}



void cNetwork::Test_For_Team_Defaulting(cPlayer* player)
{
	TT_UNREACHABLE;
}



static NetworkObjectClass* Create_Network_Object(cPacket& packet, int objectClassId, int objectId)
{
	TT_ASSERT(objectId > 0);

	NetworkObjectFactoryClass* objectFactory = NetworkObjectFactoryMgrClass::Find_Factory(objectClassId);
	TT_ASSERT(objectFactory);

	if (objectFactory)
	{
		NetworkObjectClass* object = objectFactory->Create(packet);
		object->Set_Network_ID(objectId);
		return object;
	}

	return NULL;
}



void cNetwork::Server_Packet_Handler(cPacket & packet, int rhost_id)
{
	TT_ASSERT(I_Am_Server());
	TT_ASSERT(Receiver);
	TT_ASSERT(PServerConnection->Get_Remote_Host(rhost_id));

	if (!PServerConnection->Get_Remote_Host(rhost_id))
	{
		DebugOutputString("cNetwork::Server_Packet_Handler: flushing packet from invalid rhost_id %d\n", rhost_id);
		return;
	}

	int objectId;
	byte dirtyBits;
	bool isDeletePending;

	packet.Get(objectId);
	packet.Get(dirtyBits);
	packet.Get(isDeletePending);

	NetworkObjectClass* object = NetworkObjectMgrClass::Find_Object(objectId);

	if ((dirtyBits & DB_CREATION) == DB_CREATION)
	{
		int objectClassId;
		packet.Get(objectClassId);
		
		if (!object)
		{
			object = Create_Network_Object(packet, objectClassId, objectId);
			;//DebugOutputString("Created object %d of type %-20s\n", objectId, cAppPacketStats::Interpret_Type(object->Get_App_Packet_Type()));
		}
		else
			;//DebugOutputString("Creating object %d of type %-20s, but it already exists\n", objectId, cAppPacketStats::Interpret_Type(object->Get_App_Packet_Type()));
	}

	if (object)
	{
#if NET_DIAG
		networkDiagnostics.onObjectUpdateReceived(*object, rhost_id);
#endif

		if (isDeletePending)
		{
			;//DebugOutputString("Deleted object %d of type %-20s\n", objectId, cAppPacketStats::Interpret_Type(object->Get_App_Packet_Type()));
			object->Set_Delete_Pending();
		}

		if ((dirtyBits & DB_CREATION) == DB_CREATION)
			object->Import_Creation(packet);

		if ((dirtyBits & DB_RARE) == DB_RARE)
			object->Import_Rare(packet);

		if ((dirtyBits & DB_OCCASIONAL) == DB_OCCASIONAL)
			object->Import_Occasional(packet);

		if ((dirtyBits & DB_FREQUENT) == DB_FREQUENT)
			object->Import_Frequent(packet);

		object->Set_Object_Dirty_Bits(SERVER_HOST_ID, DB_NOT_CREATED); // To prevent deletions being sent back to the original sender.

		object->Increment_Import_State_Count();
	}
	else
		;//DebugOutputString("Received update for non-existent object %d (%02X, %d).\n", objectId, dirtyBits, (int)isDeletePending);
}



void cNetwork::Client_Packet_Handler(cPacket& packet)
{
	TT_ASSERT(I_Am_Client());
	TT_ASSERT(Receiver);

	if (cNetwork::I_Am_Server())
		return;

	TT_ASSERT(!g_is_loading); // StealthEye: why is this called when loading? It could explain some odd behaviour if some packets are randomly dropped...
	if (g_is_loading)
	{
		DebugOutputString("Client flushing packet during loading\n");
		return;
	}


	int objectId;
	byte dirtyBits;
	bool isDeletePending;
	
	packet.Get(objectId);
	packet.Get(dirtyBits);
	packet.Get(isDeletePending);

	NetworkObjectClass* object = NetworkObjectMgrClass::Find_Object(objectId);

	if ((dirtyBits & DB_CREATION) == DB_CREATION)
	{
		int objectClassId;
		packet.Get(objectClassId);
		
		if (!object)
		{
			object = Create_Network_Object(packet, objectClassId, objectId);
			;//DebugOutputString("Created object %d of type %-20s\n", objectId, cAppPacketStats::Interpret_Type(object->Get_App_Packet_Type()));
		}
		else
			;//DebugOutputString("Creating object %d of type %-20s, but it already exists\n", objectId, cAppPacketStats::Interpret_Type(object->Get_App_Packet_Type()));
	}
	
	if (object)
	{
#if NET_DIAG
		networkDiagnostics.onObjectUpdateReceived(*object, SERVER_HOST_ID);
#endif

		if (isDeletePending)
		{
			;//DebugOutputString("Deleted object %d of type %-20s\n", objectId, cAppPacketStats::Interpret_Type(object->Get_App_Packet_Type()));
			object->Set_Delete_Pending();
		}

		if ((dirtyBits & DB_CREATION) == DB_CREATION)
		{
			object->Import_Creation(packet);

			if (object->Get_Network_Class_ID() == NET_BaseGameObj)
			{
				SmartGameObj* smartGameObj = ((BaseGameObj*)object)->As_SmartGameObj();
				if (smartGameObj)
					smartGameObj->Set_Player_Data(cPlayerManager::Find_Player(smartGameObj->Get_Control_Owner()));
			}
		}

		if ((dirtyBits & DB_RARE) == DB_RARE)
			object->Import_Rare(packet);

		if ((dirtyBits & DB_OCCASIONAL) == DB_OCCASIONAL)
			object->Import_Occasional(packet);

		if ((dirtyBits & DB_FREQUENT) == DB_FREQUENT)
			object->Import_Frequent(packet);

		object->Set_Object_Dirty_Bits(SERVER_HOST_ID, DB_NOT_CREATED); // To prevent deletions being sent back to the original sender.

		object->Increment_Import_State_Count();
		object->Set_Last_Clientside_Update_Time(TIMEGETTIME());
	}
	else
		;//DebugOutputString("Received update for non-existent object %d (%02X, %d).\n", objectId, dirtyBits, (int)isDeletePending);
}
