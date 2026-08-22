#ifndef TT_INCLUDE__CNETWORK_H
#define TT_INCLUDE__CNETWORK_H



#include "cNetUtil.h"
#include "cPacket.h"



enum REFUSAL_CODE;
class cConnection;
class cMsgStatList;
class cMsgStatListGroup;
class CombatNetworkReceiver;
class CombatNetworkReceiverInstanceClass;
class cRemoteHost;
class cPlayer;
class NetworkObjectClass;
class ChunkLoadClass;
class ChunkSaveClass;
class VisTableClass;
class GameCombatNetworkHandlerClass;



#define MAX_TEXTMESSAGE_LENGTH 200
#define ALL -1



class cNetwork
{

public:

	static REF_DECL2(BandwidthBarLength, sint32);
	static REF_DECL2(BandwidthScaler, sint32);
	static char* ClientEnumerationString(); // [300]
	static char* ClientString(); // [300]
	static sint8* Command(); // [224]
	static REF_DECL2(ExeCRC, sint32);
	static REF_DECL2(ExeKey, uint32);
	static REF_DECL2(Fps, uint32);
	static uint32 worstFps;
	static REF_DECL2(GraphingY, float); // [8]
	static REF_DECL2(HaveDoneMotdDialog, bool);
	static REF_DECL2(HaveDoneTeamChangeDialog, bool);
	static REF_DECL2(LastServerConnectionStateBad, bool);
	static sint8* MessageToSend(); // [200]
	static REF_DECL2(NetHandler, GameCombatNetworkHandlerClass*);
	static REF_DECL2(NetworkReceiver, CombatNetworkReceiverInstanceClass*);
	static REF_DECL2(PClientConnection, cConnection*);
	static REF_DECL2(PClientStatList, cMsgStatList*);
	static REF_DECL2(PServerConnection, cConnection*);
	static REF_DECL2(PServerStatListGroup, cMsgStatListGroup*);
	static REF_DECL2(Receiver, CombatNetworkReceiver*);
	static REF_DECL2(SensibleUpdate, bool);
	static REF_DECL2(StringsCRC, sint32);
	static REF_DECL2(ThinkCount, uint32);
	static REF_DECL2(VisTable, VisTableClass*);
	static REF_DECL2(g_is_loading, bool);

	static int lastUpdatedClientId;

	static void Init_Client(Port localPort);
	static void Accept_Handler();
	static REFUSAL_CODE Application_Acceptance_Handler(cPacket& oPacket);
	static void Cleanup_After_Client(int clientId);
	static void Cleanup_Client();
	static void Cleanup_Server();
	static void Client_Broken_Connection_Handler();
	static void Client_Packet_Handler(cPacket& oPacket);
	static void Client_Send_Packet(cPacket& oPacket, sint32);
	static bool Client_Think();
	static void Compute_Exe_Key();
	static void Connection_Handler(int clientId);
	static void Connection_Status_Change_Feedback();
	static void Delete_Player_Objects(sint32 clientId);
	static void Enable_Waiting_Players();
	static void End_Game_Test();
	static void Eviction_Handler(int clientId);
	static void Flush();
	static void Get_Client_Enumeration_String();
	static cRemoteHost* Get_Client_Rhost();
	static double Get_Client_Rhost_Threshold_Priority();
	static const char* Get_Client_String(int clientId);
	static float Get_Distance_Priority(const Vector3&, const Vector3&);
	static Vector3 Get_My_Color();
	static int Get_My_Id();
	static cPlayer* Get_My_Player_Object();
	static sint32 Get_My_Team_Number();
	static cRemoteHost* Get_Server_Rhost(int clientId);
	static double Get_Server_Rhost_Threshold_Priority(int clientId);
	static void Get_Simulated_Latency_Range_Ms(int&, int&);
	static void Hibernation_Think();
	static void Hook_Compute_Exe_Key();
	static bool I_Am_God();
	static void Init_Server();
	static void Intermission_Over_Processing();
	static bool Load(ChunkLoadClass& oLoad);
	static void Onetime_Init();
	static void Onetime_Shutdown();
	static VisTableClass* Peek_Temp_Vis_Table();
	static void Process_Eviction_Sc(cPacket& oPacket);
	static void Refusal_Handler(REFUSAL_CODE eCode);
	static void Remove_Player(int clientId);
	static bool Save(ChunkSaveClass& oSave);
	static uint Send_Object_Update(NetworkObjectClass* object, int clientId);
	static void Broadcast_Object_Update(NetworkObjectClass& object);
	static void Server_Broken_Connection_Handler(int clientId);
	static void Server_Kill_Connection(int clientId);
	static void Server_Packet_Handler(cPacket& oPacket, int clientId);
	static void Server_Send_Packet(cPacket& oPacket, sint32, int clientId);
	static void Server_Send_Packet_To_All_Connected(cPacket& oPacket, sint32);
	static bool Server_Think();
	static void Set_Desired_Frame_Sleep_Ms(int);
	static void Set_Simulated_Latency_Range_Ms(sint32, sint32);
	static void Set_Simulated_Packet_Duplication_Pc(sint32);
	static void Set_Simulated_Packet_Loss_Pc(sint32);
	static void Set_Spam_Count(sint32);
	static void Shared_Client_And_Server_Think();
	static void Shell_Command(const char* c1);
	static BOOL Show_Welcome_Message(const WideStringClass& playerName);
	static void SwitchTeam(int clientId);
	static void Tell_Client_About_Delete_Notifications(int clientId);
	static void Tell_Server_About_Delete_Notifications();
	static void Tell_Client_About_Dynamic_Objects(int clientId, const Vector3& clientPosition);
	static void Tell_Server_About_Dynamic_Objects();
	static void Test_For_Team_Defaulting(cPlayer* oPlayer);
	static void Update();
	static void Update_Fps();


	static int Get_Fps() { return Fps; }
	static int Get_Think_Count() { return ThinkCount; }
	static bool I_Am_Client() { return PClientConnection != NULL; }
	static bool I_Am_Server() { return PServerConnection != NULL; }
	static bool I_Am_Only_Client() { return PClientConnection && !PServerConnection; }
	static bool I_Am_Only_Server() { return !PClientConnection && PServerConnection; }
	static bool I_Am_Client_Server() { return PClientConnection && PServerConnection; }
	static void Set_Receiver(CombatNetworkReceiver* receiver) { Receiver = receiver; }
	static int Get_Exe_Key() { return ExeKey; }
	static int Get_Exe_CRC() { return ExeCRC; }
	static int Get_Strings_CRC() { return StringsCRC; }
	static void Set_Graphing_Y(float y) { GraphingY = y; }

	static bool isPlayerNameAllowed(const WideStringClass& playerName);

	// In cNetwork.ttinit.cpp
	static uint32 Get_Data_Files_CRC();
	static uint32 computeDataFilesCrc();

};


#endif
