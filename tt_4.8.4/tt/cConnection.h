#ifndef TT_INCLUDE__CCONNECTION_H
#define TT_INCLUDE__CCONNECTION_H



#include "cNetUtil.h"
#include "cNetStats.h"
#include "SList.h"
#include "Socket.h"



class cRemoteHost;
class cMsgStatList;
class cPacket;
template<class T> class Iterator;



enum REFUSAL_CODE
{
	NO_REFUSAL,
	REFUSAL_SERVER_FULL,
	REFUSAL_INVALID_PASSWORD,
	REFUSAL_INVALID_VERSION,
	REFUSAL_INVALID_PLAYER,
	REFUSAL_CONNECTION_REFUSED,

	REFUSAL_CODE_COUNT
};



// Send flags
static const uint8 SEND_RELIABLE   = 0x01; // you must specify this or SEND_UNRELIABLE
static const uint8 SEND_UNRELIABLE = 0x02; // you must specify this or SEND_RELIABLE
static const uint8 SEND_MULTI      = 0x04; // For SEND_UNRELIABLE only. Default is single send.



static const int INVALID_HOST_ID = -1;
static const int SERVER_HOST_ID = 0;
static const int CLIENTSERVER_HOST_ID = 1; // This is the client host id for the server if the game is nondedicated.



class cConnection
{

	friend Iterator<cConnection>;

private:

	typedef void (*Accept_Handler)();
	typedef void (*Refusal_Handler)(REFUSAL_CODE eCode);
	typedef void (*Server_Broken_Connection_Handler)(int remoteHostId);
	typedef void (*Client_Broken_Connection_Handler)();
	typedef void (*Connection_Handler)(int remoteHostId);
	typedef void (*Eviction_Handler)(int remoteHostId);
	typedef REFUSAL_CODE (*Application_Acceptance_Handler)(cPacket& packet);
	typedef void (*Server_Packet_Handler)(cPacket& packet, int remoteHostId);
	typedef void (*Client_Packet_Handler)(cPacket& packet);

	static REF_DECL2(IsFlowControlEnabled, bool);

	int localHostId; // 0 0
	uint16 localPort; // 4 4
	double maxAcceptablePacketLoss;// 8 8
	cNetStats combinedStats; // 16 16
	cNetStats averagedStats; // 656 648
	int frameStartTime; // 1296 1280
	bool isServer; // 1300 1284
	bool isDedicated; // 1301 1285
	bool isInitialized; // 1302 1286
	SOCKET __socket; // 1304 1288; Unused
	uint16 simulatedPacketLossPerRandmax;// 1308 1292
	uint16 simulatedPacketDuplicationPerRandmax;// 1310 1294
	int minSimulatedPacketLatency; // 1312 1296
	int maxSimulatedPacketLatency; // 1316 1300
	int refusalSendId; // 1320 1304
	int refusalReceiveId; // 1324 1308
	uint bandwidthLimit; // 1328 1312
	SList<cPacket> packets; // 1332 1316
	int serviceCount; // 1344 1328
	bool isBad; // 1348 1332
	cRemoteHost** remoteHosts; // 1352 1336
	int minRemoteHostId; // 1356 1340
	int maxRemoteHostId; // 1360 1344
	uint remoteHostCount; // 1364 1348
	bool isDestroyed; // 1368 1352
	cMsgStatList* msgStatList; // 1372 1356
	uint32 extraTimeoutTime; // 1376 1360
	uint32 extraTimeoutTimeStarted; // 1380 1364
	bool canProcess; // 1384 1368
	Accept_Handler acceptHandler; // 1388 1372
	Refusal_Handler refusalHandler; // 1392 1376
	Server_Broken_Connection_Handler serverBrokenConnectionHandler; // 1396 1380
	Client_Broken_Connection_Handler clientBrokenConnectionHandler; // 1400 1384
	Eviction_Handler evictionHandler; // 1404 1388
	Connection_Handler connectionHandler; // 1408 1392
	Application_Acceptance_Handler applicationAcceptanceHandler; // 1412 1396
	Server_Packet_Handler serverPacketHandler; // 1416 1400
	Client_Packet_Handler clientPacketHandler; // 1420 1404
	Socket* socket;

public:
	
	static bool Is_Flow_Control_Enabled() { return IsFlowControlEnabled; }
	static uint Get_Total_Compressed_Bytes_Sent();
	static uint Get_Total_Uncompressed_Bytes_Sent();

	cConnection();
	~cConnection();
	
	void Init_As_Client(Ip remoteIp, Port remotePort, Port _localPort = 0);
	void Init_As_Server(Port _localPort, uint maxRemoteHostCount, bool _isDedicated, Ip localIp = 0);
	void Connect_Cs(cPacket& packet);
	void Send_Packet_To_Individual(cPacket& packet, int remoteHostId, uint8 flags);
	bool Have_Id() const { return localHostId != INVALID_HOST_ID; }
	bool Is_Established() const;
	void Service_Read();
	void Service_Send(bool isUrgent = false);
	uint Get_Bandwidth_Budget_Out() const { return bandwidthLimit; }
	void Set_Bandwidth_Budget_Out(uint _bandwidthLimit);
	void Destroy_Connection(int remoteHostId);
	void Init_Stats();
	double Get_Threshold_Priority(int remoteHostId);
	void Set_Packet_Loss(double simulatedPacketLoss);
	void Set_Packet_Duplication(double simulatedPacketDuplication);
	void Set_Packet_Latency_Range(int _minSimulatedPacketLatency, int _maxSimulatedPacketLatency);
	void Set_Max_Acceptable_Packetloss_Pc(double _maxAcceptablePacketLoss);
	void Enable_Flow_Control(bool isEnabled) { IsFlowControlEnabled = isEnabled; }
	SList<cPacket>& Get_Packet_List() { return packets; }
	void Clear_Resend_Counts();
	int Get_Min_RHost() const { return minRemoteHostId; }
	int Get_Max_RHost() const { return maxRemoteHostId; }
	int Get_Num_RHosts() const { return remoteHostCount; }
	cRemoteHost* Get_Remote_Host(int remoteHostId) const;
	bool Is_Destroy() { return isDestroyed; }
	int Get_Local_Id() const {return localHostId;}
	double Get_Max_Acceptable_Packetloss_Pc() const { return maxAcceptablePacketLoss; }
	cNetStats& Get_Combined_Stats() { return combinedStats; }
	cNetStats& Get_Averaged_Stats() { return averagedStats; }
	cMsgStatList* Get_Stat_List() { return msgStatList; }
	bool Is_Bad_Connection() { return isBad; }
	void Set_Rhost_Is_In_Game(int remoteHostId, bool isInGame);
	void Set_Rhost_Expect_Packet_Flood(int remoteHostId, bool expectPacketFlood);
	void Allow_Extra_Timeout_For_Loading();
	void Allow_Packet_Processing(bool _canProcess) { canProcess = _canProcess; }
	void Send_Packet_To_Address(cPacket& packet, const Address& address);

	void Install_Accept_Handler(Accept_Handler handler);
	void Install_Refusal_Handler(Refusal_Handler handler);
	void Install_Server_Broken_Connection_Handler(Server_Broken_Connection_Handler handler);
	void Install_Client_Broken_Connection_Handler(Client_Broken_Connection_Handler handler);
	void Install_Eviction_Handler(Eviction_Handler handler);
	void Install_Conn_Handler(Connection_Handler handler);
	void Install_Application_Acceptance_Handler(Application_Acceptance_Handler handler);
	void Install_Server_Packet_Handler(Server_Packet_Handler handler);
	void Install_Client_Packet_Handler(Client_Packet_Handler handler);
	
	// The following are only for hooking purposes
	void _Init_As_Client_1(const Address& remoteAddress, Port _localPort) { Init_As_Client(remoteAddress, _localPort); }
	void _Init_As_Client_2(Ip remoteIp, Port remotePort, Port _localPort = 0) { Init_As_Client(remoteIp, remotePort, _localPort); }
	int _Send_Wrapper_1(cPacket& packet, const Address& address) { return Send_Wrapper(packet, address); }
	int _Send_Wrapper_2(cPacket& packet, int remoteHostId) { return Send_Wrapper(packet, remoteHostId); }
	SOCKET Get_Socket() { return __socket; }

	void sendPacket(cPacket& packet, cRemoteHost& destinationHost, bool reliable);
	void sendPacket(cPacket& packet, int destinationHostId, bool reliable);
	void setMaxRemoteHostCount(int maxRemoteHostCount);

private:

	cConnection(const cConnection& that); // Disallow copying
	cConnection& operator=(const cConnection& that); // Disallow copying

	void Init_As_Client(const Address& remoteAddress, Port _localPort = 0);
	bool Demultiplex_R_Or_U_Packet(cPacket& packet, int remoteHostId);
	void Send_Accept_Sc(int remoteHostId);
	bool Bind(Port port, Ip ip = 0);
	bool Receive_Packet();
	int Low_Level_Send_Wrapper(cPacket& packet, const Address& address);
	int Send_Wrapper(cPacket& packet, const Address& address);
	int Send_Wrapper(cPacket& packet, int remoteHostId);
	int Low_Level_Receive_Wrapper(cPacket& packet);
	int Receive_Wrapper(cPacket& packet);
	void Set_R_And_U_Packet_Id(cPacket& packet, int remoteHostId, uint8 flags);
	void R_And_U_Send(cPacket& packet, int remoteHostId);
	void Send_Ack(const Address& address, int packetId);
	void Send_Refusal_Sc(const Address& address, REFUSAL_CODE refusalCode);
	void Process_Connection_Request(cPacket& packet);
	void Send_Keepalives();
	TT_DEPRECATED("Use cPacket::getTypeName instead.") static const char* Type_Translation(int type);
	bool Sender_Id_Tests(const cPacket& packet);
	static uint16 Calculate_Packet_Bits(uint16 app_bytes);
	int Single_Player_sendto(cPacket& packet);
	int Single_Player_recvfrom(byte* data);
	int Address_To_Rhostid(const Address& address);
	bool Is_Time_To_Resend_Packet_To_Remote_Host(const cPacket& packet, const cRemoteHost& remoteHost) const;
	bool Is_Packet_Too_Old(const cPacket& packet, const cRemoteHost& remoteHost) const;
	int getRealRemoteHostCount() const;
	bool isRemoteHostIdValid(int remoteHostId) const;

}; // 1424  1408



#endif
