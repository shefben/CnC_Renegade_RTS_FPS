#include "General.h"
#include "cConnection.h"



#include "cRemoteHost.h"
#include "SysTimeClass.h"
#include "cSinglePlayerData.h"
#include "WOLNATInterfaceClass.h"
#include "BandwidthBalancerClass.h"
#include "PacketManagerClass.h"
#include "cMsgStatList.h"
#include "cNetwork.h"
#include "DebugSocket.h"
#include "Config.h"
#include "ResourceMgr/NetworkManager.h"



REF_DEF2(cConnection::IsFlowControlEnabled, bool, 0x0080EB8C, 0x0080DD64);



cConnection::cConnection() :
	remoteHostCount(0),
	refusalSendId(0),
	refusalReceiveId(-1),
	minRemoteHostId(-1),
	maxRemoteHostId(-2),
	isInitialized(false),
	isServer(false),
	isDedicated(false),
	simulatedPacketLossPerRandmax(0),
	simulatedPacketDuplicationPerRandmax(0),
	maxAcceptablePacketLoss(0),
	minSimulatedPacketLatency(0),
	maxSimulatedPacketLatency(0),
	bandwidthLimit(0),
	serviceCount(0),
	frameStartTime(TIMEGETTIME()),
	isDestroyed(false),
	remoteHosts(NULL),
	acceptHandler(NULL),
	refusalHandler(NULL),
	serverBrokenConnectionHandler(NULL),
	clientBrokenConnectionHandler(NULL),
	evictionHandler(NULL),
	connectionHandler(NULL),
	applicationAcceptanceHandler(NULL),
	serverPacketHandler(NULL),
	clientPacketHandler(NULL),
	isBad(false),
	extraTimeoutTime(0),
	extraTimeoutTimeStarted(0),
	canProcess(true)
{
	if (!cSinglePlayerData::Is_Single_Player())
	{
		if (config(CONFIG_NET_SIMULATION))
			socket = new DebugSocket();
		else
			socket = new Socket();

		socket->setBufferSizes(10000);
	}

	msgStatList = new cMsgStatList;
	msgStatList->Init(cPacket::TypeCount);
	for (int i = 0; i < cPacket::TypeCount; i++)
		msgStatList->Set_Name(i, cPacket::getTypeName(i));
}



cConnection::~cConnection()
{
	if (!cSinglePlayerData::Is_Single_Player())
		delete socket;

	for (int remoteHostId = minRemoteHostId; remoteHostId <= maxRemoteHostId; remoteHostId++)
		if (remoteHosts[remoteHostId])
			Destroy_Connection(remoteHostId);

	delete[] remoteHosts;
	delete msgStatList;
}



void cConnection::Init_As_Client(Ip serverIp, Port serverPort, Port _localPort)
{
	Address serverAddress = { AF_INET, ntohs(serverPort), (IN_ADDR&)serverIp };
	Init_As_Client(serverAddress, _localPort);
}



void cConnection::Init_As_Server(Port _localPort, uint maxRemoteHostCount, bool _isDedicated, Ip localIp)
{
	minRemoteHostId = 1;
	maxRemoteHostId = maxRemoteHostCount;

	remoteHosts = new cRemoteHost*[maxRemoteHostId + 1];
	for (int remoteHostId = minRemoteHostId; remoteHostId <= maxRemoteHostId; ++remoteHostId)
		remoteHosts[remoteHostId] = NULL;

	Init_Stats();

	isServer = true;
	isDedicated = _isDedicated;
	localHostId = SERVER_HOST_ID;

	if (cSinglePlayerData::Is_Single_Player())
		cConnection::IsFlowControlEnabled = false;
	
	else
	{
		for (uint i = 0; i < 50 && _localPort != 0; i++)
			if (Bind(_localPort++, localIp))
				break;
		
		WOLNATInterface.Set_Server(true);
	}

	isInitialized = true;
}



void cConnection::Connect_Cs(cPacket& packet)
{
	packet.Set_Type(cPacket::TypeConnect);
	packet.Set_Id(remoteHosts[SERVER_HOST_ID]->reliablePacketSendId++);
	remoteHosts[SERVER_HOST_ID]->Add_Packet(packet, RELIABLE_SEND_LIST);
}



void cConnection::Send_Packet_To_Individual(cPacket& packet, int remoteHostId, uint8 flags)
{
	TT_ASSERT(isInitialized);
	TT_ASSERT(packet.Get_Compressed_Size_Bytes() > 0);
	TT_ASSERT(Get_Remote_Host(remoteHostId));
	TT_ASSERT
	(
		flags == SEND_RELIABLE ||
		flags == SEND_UNRELIABLE ||
		flags == (SEND_UNRELIABLE | SEND_MULTI)
	);

	packet.Set_Num_Sends(flags & SEND_MULTI ? cNetUtil::MULTI_SENDS : 1);
	packet.Set_Sender_Id(localHostId);
	packet.Set_Type((uint8)(flags & SEND_RELIABLE ? cPacket::TypeReliable : cPacket::TypeUnreliable));

	Set_R_And_U_Packet_Id(packet, remoteHostId, packet.Get_Type());

	remoteHosts[remoteHostId]->statSample(STAT_MsgSent) += packet.Get_Num_Sends();
	
	R_And_U_Send(packet, remoteHostId);
}



bool cConnection::Is_Established() const
{
	TT_ASSERT(this); // TODO: When trying to host a game when the port is already in use, this will be NULL. Fix this.
	return isServer || (localHostId != INVALID_HOST_ID && remoteHosts[SERVER_HOST_ID]);
}



void cConnection::Service_Read()
{
	TT_ASSERT(isInitialized);

	combinedStats.StatSample[STAT_ServiceCount]++;

	frameStartTime = TIMEGETTIME();

	if (extraTimeoutTime && (frameStartTime - extraTimeoutTimeStarted) > (extraTimeoutTime * 2))
	{
		extraTimeoutTime = 0;
		extraTimeoutTimeStarted = 0;
	}

	while (Receive_Packet());

	if (canProcess)
	{
		for (int remoteHostId = minRemoteHostId; remoteHostId <= maxRemoteHostId; remoteHostId++)
		{
			cRemoteHost* remoteHost = remoteHosts[remoteHostId];
			if (remoteHost)
			{
				//remoteHosts[remoteHostId]->Compute_List_Max(RELIABLE_RCV_LIST); // Not needed?

				for (SLNode<cPacket>* node = remoteHost->Get_Packet_List(RELIABLE_RCV_LIST).Head(); node;)
				{
					cPacket* packet = node->Data();
					TT_ASSERT(packet);

					int pendingPackets = packet->Get_Id() - remoteHost->Get_Reliable_Packet_Rcv_Id();

					if (pendingPackets < 0)
						// Discard duplicate packet
						combinedStats.StatSample[STAT_DiscardCount]++;
					
					else if (pendingPackets > 0)
						// Previous packet was not yet received; wait for it to process the remaining packets
						break;

					else
					{
						// If the remote host was removed (ie. the client left the game for whatever reason), stop receiving.
						if (Demultiplex_R_Or_U_Packet(*packet, remoteHostId))
							break;

						remoteHost->Set_Reliable_Packet_Rcv_Id(packet->Get_Id() + 1);
					}

					node = node->Next();
					remoteHost->Get_Packet_List(RELIABLE_RCV_LIST).Remove_Head();
					delete packet;
				}
			}
		}

		for (int remoteHostId = minRemoteHostId; remoteHostId <= maxRemoteHostId; remoteHostId++)
		{
			cRemoteHost* remoteHost = remoteHosts[remoteHostId];
			if (remoteHost)
			{
				//remoteHost>Compute_List_Max(UNRELIABLE_RCV_LIST);
				//int listProcessingStart = TIMEGETTIME();
				
				for (SLNode<cPacket>* node = remoteHost->Get_Packet_List(UNRELIABLE_RCV_LIST).Head(); node; node = node->Next())
				{
					cPacket* packet = node->Data();
					TT_ASSERT(packet);

					if (packet->Get_Id() < remoteHost->Get_Unreliable_Packet_Rcv_Id())
					{
						// Discard duplicate packet or skipped packet
						combinedStats.StatSample[STAT_DiscardCount]++;
					}
					else
					{
						// If the remote host was removed (ie. the client left the game for whatever reason), stop receiving.
						if (Demultiplex_R_Or_U_Packet(*packet, remoteHostId))
							break;

						remoteHost->Set_Unreliable_Packet_Rcv_Id(packet->Get_Id() + 1);
					}

					delete packet;
				}

				remoteHost->Get_Packet_List(UNRELIABLE_RCV_LIST).Remove_All();

				//if (remoteHost)
				//	remoteHost->Set_List_Processing_Time(UNRELIABLE_RCV_LIST, TIMEGETTIME() - list_processing_start);
			}
		}

		if (isServer)
		{
			for (int remoteHostId = minRemoteHostId; remoteHostId <= maxRemoteHostId; remoteHostId++)
			{
				cRemoteHost* remoteHost = remoteHosts[remoteHostId];
				if (remoteHost && remoteHost->Must_Evict())
				{
					remoteHost->Set_Must_Evict(false);
					TT_ASSERT(evictionHandler);
					evictionHandler(remoteHostId);
					TT_UNREACHABLE;
				}
			}
		}
	}
}



void cConnection::Service_Send(bool isUrgent)
{
	TT_ASSERT(isInitialized);

	serviceCount++;

	int realRemoteHostCount = remoteHostCount;
	
	// Don't take into account the server's remotehost
	if (isServer && !isDedicated && remoteHosts[CLIENTSERVER_HOST_ID])
	{
		realRemoteHostCount--;
		remoteHosts[CLIENTSERVER_HOST_ID]->Set_Target_Bps(10000000);
	}

	if (realRemoteHostCount > 0)
	{
		if (isServer && BandwidthBalancer.IsEnabled)
			BandwidthBalancer.Adjust(this, isDedicated);
		else
		{
			int maxRemoteHostBps = bandwidthLimit / realRemoteHostCount;

			for (int remoteHostId = minRemoteHostId; remoteHostId <= maxRemoteHostId; remoteHostId++)
			{
				if (remoteHosts[remoteHostId])
				{
					int remoteHostBps = min(maxRemoteHostBps, remoteHosts[remoteHostId]->Get_Maximum_Bps());
					remoteHosts[remoteHostId]->Set_Target_Bps(remoteHostBps);
				}
			}
		}
	}
	
	isBad = false;
	
	for (int remoteHostId = minRemoteHostId; remoteHostId <= maxRemoteHostId; remoteHostId++)
	{
		cRemoteHost* remoteHost = remoteHosts[remoteHostId];

		if (remoteHost)
		{
			int resentPackets = 0;

			for (SLNode<cPacket>* node = remoteHost->Get_Packet_List(RELIABLE_SEND_LIST).Head(); node;)
			{
				cPacket* packet = node->Data();
				TT_ASSERT(packet);

				node = node->Next();

				if (packet->Get_Resend_Count() > 1)
				{
					resentPackets++;
					if (frameStartTime - packet->Get_First_Send_Time() > 5000)
						isBad = true;
				}

				if (Is_Time_To_Resend_Packet_To_Remote_Host(*packet, *remoteHost))
				{
					remoteHost->statSample(STAT_RPktSent)++;
					remoteHost->statSample(STAT_RByteSent) += packet->Get_Compressed_Size_Bytes();

					Send_Packet_To_Address(*packet, remoteHost->Get_Address());

					packet->Set_Send_Time();
					packet->Increment_Resend_Count();

					if (packet->Get_Resend_Count() > 0)
					{
						remoteHost->statSample(STAT_ResendCount)++;
						remoteHost->Increment_Resends();
					}
					
					if (Is_Packet_Too_Old(*packet, *remoteHost) && canProcess)
					{
						// Connection broken
						Destroy_Connection(remoteHostId);

						if (isServer)
						{
							TT_ASSERT(serverBrokenConnectionHandler);
							serverBrokenConnectionHandler(remoteHostId);
						}
						else
						{
							TT_ASSERT(clientBrokenConnectionHandler);
							clientBrokenConnectionHandler();
						}
						
						break;
					}
				}
			}

			remoteHost->Set_Total_Resent_Packets_In_Queue(resentPackets);
		}
	}

	for (int remoteHostId = minRemoteHostId; remoteHostId <= maxRemoteHostId; remoteHostId++)
	{
		cRemoteHost* remoteHost = remoteHosts[remoteHostId];

		if (remoteHost)
		{
			for (SLNode<cPacket>* node = remoteHost->Get_Packet_List(UNRELIABLE_SEND_LIST).Head(); node; node = node->Next())
			{
	            cPacket* packet = node->Data();
	            TT_ASSERT(packet);

				remoteHost->statSample(STAT_UPktSent)++;
				remoteHost->statSample(STAT_UByteSent) += packet->Get_Compressed_Size_Bytes();

				Send_Packet_To_Address(*packet, remoteHost->Get_Address());
				
				delete packet;
			}
			
			remoteHost->Get_Packet_List(UNRELIABLE_SEND_LIST).Remove_All();
		}
	}
	
	Send_Keepalives();
	
	int sampleTime = frameStartTime - combinedStats.Get_Sample_Start_Time();
	if (isServer && remoteHostCount > 0 && sampleTime > cNetUtil::NETSTATS_SAMPLE_TIME_MS)
	{
		for (int statistic = 0; statistic < STAT_COUNT; statistic++)
		{
			for (int remoteHostId = minRemoteHostId; remoteHostId <= maxRemoteHostId; remoteHostId++)
				if (remoteHosts[remoteHostId])
					combinedStats.StatSample[statistic] += remoteHosts[remoteHostId]->Get_Stats().StatSnapshot[statistic];

			averagedStats.StatSample[statistic] = cMathUtil::Round(combinedStats.StatSample[statistic] / (float)remoteHostCount);
		}

		averagedStats.Update_If_Sample_Done(frameStartTime);
	}
	
	combinedStats.Update_If_Sample_Done(frameStartTime);

	// Add one for the server client.
	for (int remoteHostId = (!cNetwork::I_Am_Server() || isDedicated) ? minRemoteHostId : minRemoteHostId + 1; remoteHostId <= maxRemoteHostId; remoteHostId++)
	{
		cRemoteHost* remoteHost = remoteHosts[remoteHostId];
		if (remoteHost)
		{
			if (remoteHost->Get_Stats().Update_If_Sample_Done(frameStartTime, false))
			{
				remoteHost->Adjust_Resend_Timeout();
				remoteHost->Adjust_Flow_If_Necessary((float)sampleTime);
			}
		}
	}
	
	PacketManager().Flush(isUrgent);
}



void cConnection::Set_Bandwidth_Budget_Out(uint _bandwidthLimit)
{
	bandwidthLimit = _bandwidthLimit;

	// If we only have very limited bandwidth available then it's silly to have huge send buffers since that can mask
	// problems with outgoing packets until it's too late to recover.
	if (socket && !cSinglePlayerData::Is_Single_Player())
		// Make the buffers big enough for 3 seconds of data.
		socket->setBufferSizes(min(bandwidthLimit / 8 * 3, 250000));
}



void cConnection::Destroy_Connection(int remoteHostId)
{
	TT_ASSERT(isInitialized);
	TT_ASSERT(Get_Remote_Host(remoteHostId));

	// The above assert was added in TT. This line was in the original code, we will keep it just to be sure.
	if (remoteHosts[remoteHostId])
	{
		delete remoteHosts[remoteHostId];
		remoteHosts[remoteHostId] = NULL;
		remoteHostCount--;

		TT_ASSERT(remoteHostCount >= 0);
	}
}



void cConnection::Init_Stats()
{
	combinedStats.Init_Net_Stats();
	averagedStats.Init_Net_Stats();

	for (int remoteHostId = minRemoteHostId; remoteHostId <= maxRemoteHostId; ++remoteHostId)
		if (remoteHosts[remoteHostId])
			remoteHosts[remoteHostId]->Init_Stats();

	frameStartTime = TIMEGETTIME();
}



double cConnection::Get_Threshold_Priority(int remoteHostId)
{
	TT_ASSERT(Get_Remote_Host(remoteHostId));
	return remoteHosts[remoteHostId]->Get_Threshold_Priority();
}



void cConnection::Set_Packet_Loss(double simulatedPacketLoss)
{
	TT_ASSERT(simulatedPacketLoss >= 0 && simulatedPacketLoss <= 100);
	simulatedPacketLossPerRandmax = (uint16)cMathUtil::Round(simulatedPacketLoss / 100 * RAND_MAX);
}



void cConnection::Set_Packet_Duplication(double simulatedPacketDuplication)
{
	TT_ASSERT(simulatedPacketDuplication >= 0 && simulatedPacketDuplication <= 100);
	simulatedPacketDuplicationPerRandmax = (uint16)cMathUtil::Round(simulatedPacketDuplication / 100 * RAND_MAX);
}



void cConnection::Set_Packet_Latency_Range(int _minSimulatedPacketLatency, int _maxSimulatedPacketLatency)
{
	TT_ASSERT(_minSimulatedPacketLatency >= 0);
	TT_ASSERT(_maxSimulatedPacketLatency >= 0);
	TT_ASSERT(_minSimulatedPacketLatency <= _maxSimulatedPacketLatency);

	minSimulatedPacketLatency = _minSimulatedPacketLatency;
	maxSimulatedPacketLatency = _maxSimulatedPacketLatency;
}



void cConnection::Set_Max_Acceptable_Packetloss_Pc(double _maxAcceptablePacketLoss)
{
	TT_ASSERT(_maxAcceptablePacketLoss >= 0 && _maxAcceptablePacketLoss <= 100);
	maxAcceptablePacketLoss = _maxAcceptablePacketLoss;
}




void cConnection::Clear_Resend_Counts()
{
	for (int remoteHostId = minRemoteHostId; remoteHostId <= maxRemoteHostId; remoteHostId++)
	{
		cRemoteHost* remoteHost = remoteHosts[remoteHostId];
		if (remoteHost)
		{
			for (SLNode<cPacket>* node = remoteHost->Get_Packet_List(RELIABLE_SEND_LIST).Head(); node; node = node->Next())
			{
				cPacket* packet = node->Data();
				TT_ASSERT(packet);

				if (packet->Get_Resend_Count() > 0)
					packet->Clear_Resend_Count();
			}
		}
	}
}



cRemoteHost* cConnection::Get_Remote_Host(int remoteHostId) const
{
	if (remoteHostId >= minRemoteHostId && remoteHostId <= maxRemoteHostId)
		return remoteHosts[remoteHostId];
	else
		return NULL;
}



void cConnection::Set_Rhost_Is_In_Game(int remoteHostId, bool isInGame)
{
	TT_ASSERT(isServer);
	//TT_ASSERT(Get_Remote_Host(remoteHostId)); // TODO: Prevent this assert from triggering

	cRemoteHost* remoteHost = remoteHosts[remoteHostId];
	if (remoteHost)
		remoteHost->Set_Is_Loading(!isInGame);
}



void cConnection::Set_Rhost_Expect_Packet_Flood(int remoteHostId, bool expectPacketFlood)
{
	TT_ASSERT(isServer);
	TT_ASSERT(isRemoteHostIdValid(remoteHostId));

	cRemoteHost* remoteHost = remoteHosts[remoteHostId];
	if (remoteHost)
		remoteHosts[remoteHostId]->Set_Flood(expectPacketFlood);
}



void cConnection::Allow_Extra_Timeout_For_Loading()
{
	extraTimeoutTime = cNetUtil::SERVER_CONNECTION_LOSS_TIMEOUT_LOADING_ALLOWANCE;
	extraTimeoutTimeStarted = TIMEGETTIME();
}



void cConnection::Send_Packet_To_Address(cPacket& packet, const Address& address)
{
	TT_ASSERT(isInitialized);

	int remoteHostId = Address_To_Rhostid(address);
	cRemoteHost* remoteHost = remoteHostId != INVALID_HOST_ID ? remoteHosts[remoteHostId] : NULL;

	for (int i = 0; i < packet.Get_Num_Sends(); i++)
	{
		if (remoteHost)
			Send_Wrapper(packet, remoteHostId);
		else
			Send_Wrapper(packet, address);

		if (remoteHost)
		{
			remoteHost->statSample(STAT_PktSent)++;
			remoteHost->statSample(STAT_AppByteSent) += packet.Get_Compressed_Size_Bytes();
			//remoteHost->statSample(STAT_HdrByteSent) += cNetUtil::Get_Header_Bytes();
			remoteHost->statSample(STAT_BitsSent) += Calculate_Packet_Bits((uint16)packet.Get_Compressed_Size_Bytes());
		}
	}
}



void cConnection::Install_Accept_Handler(Accept_Handler handler)
{
	TT_ASSERT(handler);
	acceptHandler = handler;
}



void cConnection::Install_Refusal_Handler(Refusal_Handler handler)
{
	TT_ASSERT(handler);
	refusalHandler = handler;
}



void cConnection::Install_Server_Broken_Connection_Handler(Server_Broken_Connection_Handler handler)
{
	TT_ASSERT(handler);
	serverBrokenConnectionHandler = handler;
}



void cConnection::Install_Client_Broken_Connection_Handler(Client_Broken_Connection_Handler handler)
{
	TT_ASSERT(handler);
	clientBrokenConnectionHandler = handler;
}



void cConnection::Install_Eviction_Handler(Eviction_Handler handler)
{
	TT_ASSERT(handler);
	evictionHandler = handler;
}



void cConnection::Install_Conn_Handler(Connection_Handler handler)
{
	TT_ASSERT(handler);
	connectionHandler = handler;
}



void cConnection::Install_Application_Acceptance_Handler(Application_Acceptance_Handler handler)
{
	TT_ASSERT(handler);
	applicationAcceptanceHandler = handler;
}



void cConnection::Install_Server_Packet_Handler(Server_Packet_Handler handler)
{
	TT_ASSERT(handler);
	serverPacketHandler = handler;
}



void cConnection::Install_Client_Packet_Handler(Client_Packet_Handler handler)
{
	TT_ASSERT(handler);
	clientPacketHandler = handler;
}



/*==============================================================================
  Private methods
------------------------------------------------------------------------------*/



void cConnection::Init_As_Client(const Address& remoteAddress, Port _localPort)
{
	TT_ASSERT(remoteHostCount == 0);

	localHostId = INVALID_HOST_ID;
	isServer = false;
	isDedicated = false;
	
	minRemoteHostId = SERVER_HOST_ID;
	maxRemoteHostId = SERVER_HOST_ID;

	remoteHosts = new cRemoteHost*[1];
	remoteHosts[SERVER_HOST_ID] = new cRemoteHost();
	remoteHosts[SERVER_HOST_ID]->id = SERVER_HOST_ID;
	++remoteHostCount;

	if (cNetwork::I_Am_Server())
		remoteHosts[SERVER_HOST_ID]->setIsFlowControlEnabled(false);

	if (!cSinglePlayerData::Is_Single_Player())
	{
		Bind(_localPort);
		remoteHosts[SERVER_HOST_ID]->address = remoteAddress;
	}
	
	Init_Stats();

	isInitialized = true;
}



bool cConnection::Demultiplex_R_Or_U_Packet(cPacket& packet, int remoteHostId)
{
	TT_ASSERT(Get_Remote_Host(remoteHostId));
	TT_ASSERT((uint8)packet.Get_Type() < cPacket::TypeCount);

	switch (packet.Get_Type())
	{

	case cPacket::TypeReliable:
	case cPacket::TypeUnreliable:

		if (isServer)
		{
			TT_ASSERT(serverPacketHandler);
			serverPacketHandler(packet, remoteHostId);
		}
		else
		{
			TT_ASSERT(clientPacketHandler);
			clientPacketHandler(packet);
		}
		break;

	case cPacket::TypeResourceManager:
		networkManager.onPacketReceived(packet);
		break;

	}

	return !remoteHosts[remoteHostId];
}



void cConnection::Send_Accept_Sc(int hostId)
{
	cPacket packet;
	packet.Add(hostId);
	packet.Add(TT_VERSION);
	packet.Set_Type(cPacket::TypeAccept);
	packet.Set_Id(remoteHosts[hostId]->reliablePacketSendId++);
	packet.Set_Sender_Id(localHostId);
	remoteHosts[hostId]->Add_Packet(packet, RELIABLE_SEND_LIST);
}



bool cConnection::Bind(Port _localPort, Ip localIp)
{
	if (socket->bind(localIp, _localPort))
		return false;

	localPort = _localPort;
	return true;
}



bool cConnection::Receive_Packet()
{
	cPacket packet;
	
	int receiveResult = Receive_Wrapper(packet);
	
	if (!receiveResult)
		return false;
	
	if (cSinglePlayerData::Is_Single_Player() && receiveResult == SOCKET_ERROR)
		return false;
	
	if (packet.Get_Type() == cPacket::TypeFirewallProbe)
	{
		WOLNATInterface.Intercept_Game_Packet(packet);
		return true;
	}
	
	uint packetBits = Calculate_Packet_Bits((uint16)receiveResult);
	int remoteHostId = Address_To_Rhostid(packet.getAddress());
	
	if (remoteHostId != INVALID_HOST_ID)
	{
		cRemoteHost* remoteHost = remoteHosts[remoteHostId];
		
		++remoteHost->statSample(STAT_PktRcv);
		remoteHost->statSample(STAT_AppByteRcv) += receiveResult;
		remoteHost->statSample(STAT_BitsRcv) += packetBits;
		remoteHost->setLastContactTime(TIMEGETTIME());
	}
	
	int packetSenderId = packet.Get_Sender_Id();
	cRemoteHost* packetSenderHost = NULL;
	if (packetSenderId != INVALID_HOST_ID)
	{
		packetSenderHost = remoteHosts[packetSenderId];
		if (!packetSenderHost)
			return true;
	}

	int packetId = packet.Get_Id();
	
	switch (packet.Get_Type())
	{
	
	case cPacket::TypeUnreliable:
		{
			if (localHostId == INVALID_HOST_ID)
				break;
			
			if (!Sender_Id_Tests(packet))
				break;
			
			if (packetId < packetSenderHost->unreliablePacketRcvId)
				break;
			
			cNetStats& packetSenderStats = packetSenderHost->Get_Stats();
			++packetSenderStats.StatSample[STAT_MsgRcv];
			++packetSenderStats.StatSample[STAT_UPktRcv];
			packetSenderStats.StatSample[STAT_UByteRcv] += receiveResult;
			
			if ((signed)packetId > packetSenderHost->Get_Stats().Get_Freeze_Packet_Id())
			{
				++packetSenderStats.StatSample[STAT_UPktRcv2];
				++packetSenderStats.unreliableCount;
			}
			
			if ((signed)packetId > packetSenderStats.lastUnreliablePacketId)
				packetSenderStats.lastUnreliablePacketId = packetId;
			
			packetSenderHost->Add_Packet(packet, UNRELIABLE_RCV_LIST);
			
			break;
		}

	case cPacket::TypeReliable:
		{
			if (localHostId == INVALID_HOST_ID)
				break;
			
			if (!Sender_Id_Tests(packet))
				break;
			
			Send_Ack(packet.getAddress(), packet.Get_Id());
			
			cNetStats& packetSenderStats = packetSenderHost->Get_Stats();
			packetSenderStats.StatSample[STAT_MsgRcv]++;
			packetSenderStats.StatSample[STAT_RPktRcv]++;
			packetSenderStats.StatSample[STAT_RByteRcv] += receiveResult;
			
			packetSenderHost->Add_Packet(packet, RELIABLE_RCV_LIST);
			
			break;
		}
		
	case cPacket::TypeAck:
		{
			if (!Sender_Id_Tests(packet))
				break;
			
			cNetStats& packetSenderStats = packetSenderHost->Get_Stats();
			packetSenderStats.StatSample[STAT_AckCountRcv]++;
			
			packetSenderHost->Remove_Packet(packetId, RELIABLE_SEND_LIST);
			
			break;
		}

	case cPacket::TypeKeepalive:
		
		if (!Sender_Id_Tests(packet))
			break;
		
		if (localHostId == INVALID_HOST_ID)
			break;
		
		Send_Ack(packet.getAddress(), packetId);
		
		float packetLossPercentage;
		int remoteServiceRate;
		packet.Get(packetLossPercentage);
		packet.Get(remoteServiceRate);

		packetSenderHost->Get_Stats().Set_Pc_Packetloss_Sent(packetLossPercentage);
		packetSenderHost->Get_Stats().Set_Remote_Service_Count(remoteServiceRate);
		packetSenderHost->Add_Packet(packet, RELIABLE_RCV_LIST);
		
		break;
		
	case cPacket::TypeConnect:
	
		Send_Ack(packet.getAddress(), packet.Get_Id());
		Process_Connection_Request(packet);
		
		break;
		
	case cPacket::TypeAccept:

		if (localHostId != INVALID_HOST_ID)
		{
			cNetStats& packetSenderStats = packetSenderHost->Get_Stats();
			packetSenderStats.StatSample[STAT_DiscardCount]++;
			Send_Ack(packet.getAddress(), packet.Get_Id());
			break;
		}
		
		packet.Get(localHostId);
		packet.Get(packetSenderHost->version);
		
		Send_Ack(packet.getAddress(), packet.Get_Id());
		
		acceptHandler();
		clientNetworkManager.onAcceptationReceived();
		
		packetSenderHost->Add_Packet(packet, RELIABLE_RCV_LIST);
		
		break;
		
	case cPacket::TypeRefusal:
		
		if (localHostId != INVALID_HOST_ID)
		{
			cNetStats& packetSenderStats = packetSenderHost->Get_Stats();
			packetSenderStats.StatSample[STAT_DiscardCount]++;
			break;
		}
		
		if (packet.Get_Id() <= refusalReceiveId)
			break;
		
		refusalReceiveId = packet.Get_Id();

		uint refusalCode;
		packet.Get(refusalCode);
		refusalHandler((REFUSAL_CODE)refusalCode);

		isDestroyed = true;
		
		break;

	case cPacket::TypeResourceManager:
		{
			if (localHostId == INVALID_HOST_ID)
				break;
			
			if (!Sender_Id_Tests(packet))
				break;
			
			Send_Ack(packet.getAddress(), packet.Get_Id());
			packetSenderHost->Add_Packet(packet, RELIABLE_RCV_LIST);

			break;
		}

	}
	
	return true;
}



int cConnection::Low_Level_Send_Wrapper(cPacket& packet, const Address& address)
{
	if (cSinglePlayerData::Is_Single_Player())
		return Single_Player_sendto(packet);

	else
	{
		bool took = PacketManager().Take_Packet((byte*)packet.Get_Data(), packet.Get_Compressed_Size_Bytes(), address.sin_addr.s_addr, address.sin_port, *socket);
		took;
		TT_ASSERT(took);

		PacketManager().Flush();
		return packet.Get_Compressed_Size_Bytes();
	}
}



int cConnection::Send_Wrapper(cPacket& packet, const Address& address)
{
	cPacket fullPacket;
	cPacket::Construct_Full_Packet(fullPacket, packet);

	uint8 packetType = packet.Get_Type();
	TT_ASSERT(packetType >= 0 && packetType < cPacket::TypeCount);
	msgStatList->Increment_Num_Msg_Sent(packetType);
	msgStatList->Increment_Num_Byte_Sent(packetType, fullPacket.Get_Compressed_Size_Bytes());

	return Low_Level_Send_Wrapper(fullPacket, address);
}



int cConnection::Send_Wrapper(cPacket& packet, int remoteHostId)
{
	TT_ASSERT(Get_Remote_Host(remoteHostId));
	return Send_Wrapper(packet, remoteHosts[remoteHostId]->Get_Address());
}



int cConnection::Low_Level_Receive_Wrapper(cPacket& packet)
{
	if (cSinglePlayerData::Is_Single_Player())
		return Single_Player_recvfrom((byte*)packet.Get_Data());
	
	else
	{
		// Try getting a packet from the packet manager. Keep trying for a number of times if we get a socket error. If we don't
		// do this and just fail then we fall out of the receive packet loop and no more packets are received this frame. 15 of
		// these a second and we don't get any packets in at all.

		for (int i = 0; i < 250; i++)
		{
			Address address;
			int bytes = PacketManager().Get_Packet(*socket, (byte*)packet.Get_Data(), packet.Get_Max_Size(), (uint32&)address.sin_addr.s_addr, address.sin_port);

			if (bytes >= 0)
			{
				packet.setAddress(address);
				return bytes;
			}

			// Connection was reset/packet was refused.

			// If we try to invoke the broken connection handler when loading a level it will access the player list
			// which will cause a DataSafe access from the wrong (main) thread with potentially catastrophic effects.
			if (isServer && canProcess)
			{
				int remoteHostId = Address_To_Rhostid(address);
				if (remoteHostId != INVALID_HOST_ID)
				{
					Destroy_Connection(remoteHostId);
					
					TT_ASSERT(serverBrokenConnectionHandler);
					serverBrokenConnectionHandler(remoteHostId);
				}
			}
		}
	}

	return 0;
}



int cConnection::Receive_Wrapper(cPacket& packet)
{
	cPacket appPacket;
	int byteLength = Low_Level_Receive_Wrapper(appPacket);

	if (byteLength > 0)
	{
		// We won't be able to read the header from the full packet unless we
		// set the bit length as least as high as the packet size.
		// The returned packet object will have the exact bit length set correctly.

		appPacket.Set_Bit_Length(byteLength * 8);
		cPacket::Construct_App_Packet(packet, appPacket);

		uint8 packetType = packet.Get_Type();
		TT_ASSERT(packetType >= 0 && packetType < cPacket::TypeCount);

		if (!(packetType >= 0 && packetType < cPacket::TypeCount))
			return 0;

		msgStatList->Increment_Num_Msg_Recd(packetType);
		msgStatList->Increment_Num_Byte_Recd(packetType, byteLength);
	}

	return byteLength;
}



void cConnection::Set_R_And_U_Packet_Id(cPacket& packet, int remoteHostId, uint8 packetType)
{
	TT_ASSERT(Get_Remote_Host(remoteHostId));
	TT_ASSERT(packet.Get_Type() == cPacket::TypeReliable || packet.Get_Type() == cPacket::TypeUnreliable);

	if (packetType == cPacket::TypeReliable)
	{
		packet.Set_Id(remoteHosts[remoteHostId]->Get_Reliable_Packet_Send_Id());
		remoteHosts[remoteHostId]->Increment_Reliable_Packet_Send_Id();
	}
	else
	{
		packet.Set_Id(remoteHosts[remoteHostId]->Get_Unreliable_Packet_Send_Id());
		remoteHosts[remoteHostId]->Increment_Unreliable_Packet_Send_Id();
	}
}



void cConnection::R_And_U_Send(cPacket& packet, int remoteHostId)
{
	TT_ASSERT(Get_Remote_Host(remoteHostId));
	TT_ASSERT(packet.Get_Type() == cPacket::TypeReliable || packet.Get_Type() == cPacket::TypeUnreliable);

	remoteHosts[remoteHostId]->Add_Packet(packet, (uint8)(packet.Get_Type() == cPacket::TypeReliable ? RELIABLE_SEND_LIST : UNRELIABLE_SEND_LIST));
}



void cConnection::Send_Ack(const Address& address, int packetId)
{
	TT_ASSERT(isInitialized);
	TT_ASSERT(packetId >= 0);
	TT_ASSERT(localHostId != INVALID_HOST_ID);

	cPacket packet;
	packet.Set_Type(cPacket::TypeAck);
	packet.Set_Id(packetId);
	packet.Set_Sender_Id(localHostId);

	int remoteHostId = Address_To_Rhostid(address);
	if (remoteHostId != INVALID_HOST_ID)
	{
		remoteHosts[remoteHostId]->statSample(STAT_AckCountSent)++;
		remoteHosts[remoteHostId]->statSample(STAT_UPktSent)++;
		remoteHosts[remoteHostId]->statSample(STAT_UByteSent) += packet.Get_Compressed_Size_Bytes();
	}

	Send_Packet_To_Address(packet, address);
}



void cConnection::Send_Refusal_Sc(const Address& address, REFUSAL_CODE refusalCode)
{
	cPacket packet;
	packet.Add(refusalCode);
	packet.Set_Type(cPacket::TypeRefusal);
	packet.Set_Id(refusalSendId++);
	packet.Set_Sender_Id(localHostId);
	packet.Set_Num_Sends(cNetUtil::MULTI_SENDS);
	Send_Packet_To_Address(packet, address);
}



void cConnection::Process_Connection_Request(cPacket& packet)
{
	int remoteHostId = INVALID_HOST_ID;
	
	for (int i = minRemoteHostId; i <= maxRemoteHostId; ++i)
	{
		cRemoteHost* remoteHost = remoteHosts[i];
		if (remoteHost)
		{
			if (cSinglePlayerData::Is_Single_Player())
				continue;
			
			if (cNetUtil::Is_Same_Address(remoteHost->address, packet.getAddress()))
				return;
		}
		else if (remoteHostId == INVALID_HOST_ID)
			remoteHostId = i;
	}
	
	if (remoteHostId == INVALID_HOST_ID)
		return Send_Refusal_Sc(packet.getAddress(), REFUSAL_SERVER_FULL);
	
	REFUSAL_CODE refusalCode = applicationAcceptanceHandler(packet);
	if (refusalCode != 0)
		return Send_Refusal_Sc(packet.getAddress(), refusalCode);
	
	cRemoteHost* remoteHost = new cRemoteHost();
	remoteHosts[remoteHostId] = remoteHost;
	
	remoteHost->id = remoteHostId;
	remoteHost->address = packet.getAddress();
	packet.Get(remoteHost->maxBandwidth);
	packet.Get(remoteHost->version);
	packet.Get_Terminated_String(remoteHost->serial,33);
	
	++remoteHostCount;
	
	Send_Accept_Sc(remoteHostId);
	connectionHandler(remoteHostId);
	serverNetworkManager.onConnectionAccepted(*remoteHost);

	remoteHost->reliablePacketRcvId++;
}



void cConnection::Send_Keepalives()
{
	TT_ASSERT(isInitialized);

	if (localHostId != INVALID_HOST_ID)
	{
		for (int remoteHostId = minRemoteHostId; remoteHostId <= maxRemoteHostId; remoteHostId++)
		{
			cRemoteHost* remoteHost = remoteHosts[remoteHostId];
			if (remoteHost && frameStartTime - remoteHost->Get_Last_Keepalive_Time_Ms() > cNetUtil::KEEPALIVE_TIMEOUT_MS)
			{
				int serviceRate = (int)(1000 * (serviceCount - remoteHost->lastServiceCount) / (frameStartTime - remoteHost->Get_Last_Keepalive_Time_Ms()));
				remoteHost->lastServiceCount = serviceCount;
				remoteHost->Set_Last_Keepalive_Time_Ms(frameStartTime);

				int packetId = remoteHosts[remoteHostId]->Get_Reliable_Packet_Send_Id();
				remoteHosts[remoteHostId]->Increment_Reliable_Packet_Send_Id();

				cPacket packet;
				packet.Add(remoteHosts[remoteHostId]->Get_Stats().Get_Pc_Packetloss_Received());
				packet.Add(serviceRate);

				packet.Set_Type(cPacket::TypeKeepalive);
				packet.Set_Id(packetId);
				packet.Set_Sender_Id(localHostId);

				remoteHosts[remoteHostId]->Add_Packet(packet, RELIABLE_SEND_LIST);
			}
		}
	}
}



bool cConnection::Sender_Id_Tests(const cPacket& packet)
{
	TT_ASSERT(isInitialized);

	int senderHostId = packet.Get_Sender_Id();

	if (!isServer && senderHostId != SERVER_HOST_ID)
	{
		combinedStats.StatSample[STAT_DiscardCount]++;
		return false;
	}

	cRemoteHost* senderHost = Get_Remote_Host(senderHostId);
	if (!senderHost)
	{
		combinedStats.StatSample[STAT_DiscardCount]++;
		return false;
	}

	if (!cSinglePlayerData::Is_Single_Player() && !cNetUtil::Is_Same_Address(senderHost->Get_Address(), packet.getAddress()))
	{
		combinedStats.StatSample[STAT_DiscardCount]++;
		return false;
	}

	return true;
}



uint16 cConnection::Calculate_Packet_Bits(uint16 bytes)
{
	return (uint16)(11.328125 * (bytes + 33));
}



int cConnection::Single_Player_sendto(cPacket& packet)
{
	TT_ASSERT(cSinglePlayerData::Is_Single_Player());

	SList<cPacket>* packetList = cSinglePlayerData::Get_Input_Packet_List(isServer ? CLIENT_LIST : SERVER_LIST);
	TT_ASSERT(packetList);

	packetList->Add_Tail(new cPacket(packet));

	return packet.Get_Compressed_Size_Bytes();
}



int cConnection::Single_Player_recvfrom(byte* data)
{
	TT_ASSERT(cSinglePlayerData::IsSinglePlayer);

	SList<cPacket>* packetList = cSinglePlayerData::Get_Input_Packet_List(isServer ? SERVER_LIST : CLIENT_LIST);
	TT_ASSERT(packetList);

	cPacket* packet = packetList->Remove_Head();
	if (!packet)
	{
		WSASetLastError(WSAEWOULDBLOCK);
		return SOCKET_ERROR;
	}

	memcpy(data, packet->Get_Data(), packet->Get_Max_Size());

	int packetSize = packet->Get_Compressed_Size_Bytes();
	delete packet;

	return packetSize;
}



int cConnection::Address_To_Rhostid(const Address& address)
{
	if (!cSinglePlayerData::Is_Single_Player())
		for (int remoteHostId = minRemoteHostId; remoteHostId <= maxRemoteHostId; remoteHostId++)
			if (remoteHosts[remoteHostId] && cNetUtil::Is_Same_Address(remoteHosts[remoteHostId]->Get_Address(), address))
				return remoteHostId;

	return INVALID_HOST_ID;
}



bool cConnection::Is_Time_To_Resend_Packet_To_Remote_Host(const cPacket& packet, const cRemoteHost& remoteHost) const
{
	TT_ASSERT(isInitialized);
	
	int lastSendTime = packet.Get_Send_Time();
	if (lastSendTime == cPacket::Get_Default_Send_Time())
		return true;

	int timeSinceLastSend = frameStartTime - packet.Get_Send_Time();
	if (timeSinceLastSend > 3000)
		return true;

	bool isLoading = isServer && remoteHost.Get_Is_Loading();
	float resendMultiplier = isLoading ? 2.f : .5f;
	int timeout = (int)(remoteHost.Get_Resend_Timeout_Ms() * (1.f + packet.Get_Resend_Count() * resendMultiplier));
	TT_ASSERT(timeout > 0);

	return timeSinceLastSend >= timeout;
}



bool cConnection::Is_Packet_Too_Old(const cPacket& packet, const cRemoteHost& remoteHost) const
{
	if (packet.Get_Send_Time() == cPacket::Get_Default_Send_Time())
		return false;

	if (frameStartTime <= packet.Get_First_Send_Time())
		return false;

	int timeout = 0;
	if (isServer)
	{
		timeout = cNetUtil::SERVER_CONNECTION_LOSS_TIMEOUT;

		if (remoteHost.Get_Is_Loading() || remoteHost.Was_Recently_Loading(frameStartTime) || getRealRemoteHostCount() == 0)
			timeout += cNetUtil::SERVER_CONNECTION_LOSS_TIMEOUT_LOADING_ALLOWANCE;
	}
	else
		timeout = cNetUtil::CLIENT_CONNECTION_LOSS_TIMEOUT + extraTimeoutTime;

	return frameStartTime - packet.Get_First_Send_Time() >= timeout;
}



int cConnection::getRealRemoteHostCount() const
{
	int result = Get_Num_RHosts();
	if (isServer && !isDedicated)
		result--;
	
	TT_ASSERT(result >= 0);
	return result;
}



bool cConnection::isRemoteHostIdValid(int remoteHostId) const
{
	return remoteHostId >= minRemoteHostId && remoteHostId <= maxRemoteHostId;
}



void cConnection::sendPacket(cPacket& packet, cRemoteHost& destinationHost, bool reliable)
{
	packet.Set_Id(destinationHost.reliablePacketSendId++);
	packet.Set_Sender_Id(localHostId);
	destinationHost.Add_Packet(packet, reliable ? (uint8)RELIABLE_SEND_LIST : (uint8)UNRELIABLE_SEND_LIST);
}



void cConnection::sendPacket(cPacket& packet, int destinationHostId, bool reliable)
{
	TT_ASSERT(remoteHosts[destinationHostId]);
	sendPacket(packet, *remoteHosts[destinationHostId], reliable);
}



void cConnection::setMaxRemoteHostCount(int maxRemoteHostCount)
{
	if (maxRemoteHostCount > maxRemoteHostId)
	{
		int _maxRemoteHostId = maxRemoteHostCount;

		cRemoteHost** _remoteHosts = new cRemoteHost*[_maxRemoteHostId + 1];
		memset(_remoteHosts, NULL, sizeof(cRemoteHost*) * (_maxRemoteHostId + 1));
		
		memcpy(_remoteHosts, remoteHosts, sizeof(cRemoteHost*) * (maxRemoteHostId + 1));
		delete[] remoteHosts;

		remoteHosts = _remoteHosts;
		maxRemoteHostId = _maxRemoteHostId;
	}
}