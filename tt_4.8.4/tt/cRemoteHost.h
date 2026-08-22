#ifndef TT_INCLUDE__CREMOTEHOST_H
#define TT_INCLUDE__CREMOTEHOST_H



#include "cNetStats.h"
#include "cPacket.h"
#include "engine_common.h"
#include "engine_vector.h"
#include "SList.h"
#include "cNetUtil.h"



class cConnection;



enum
{
	RELIABLE_SEND_LIST,
	RELIABLE_RCV_LIST,
	UNRELIABLE_SEND_LIST,
	UNRELIABLE_RCV_LIST
};



class cRemoteHost
{

	friend cConnection;

public:

	enum ResourceManagerState
	{
		RESOURCE_MANAGER_STATE_NEGOTIATING,
		RESOURCE_MANAGER_STATE_ACQUIRING_PACKAGES,
		RESOURCE_MANAGER_STATE_INGAME,
		RESOURCE_MANAGER_STATE_COUNT,
	};
	
	static REF_DECL2(cRemoteHost::AllowExtraModemBandwidthThrottling, UNK);
	static REF_DECL2(cRemoteHost::PriorityUpdateRate, int);

private:

	cNetStats      stats;                       // 0000  0000
	double         thresholdPriority;           // 0280  0278
	double         thresholdPriorityIncrement;  // 0288  0280
	int            lastReliablePacketId;        // 0290  0288
	int            lastUnreliablePacketId;      // 0294  028C
	uint16         resendTimeout;               // 0298  0290
	int            pingCount;                   // 029C  0294
	int            totalPingTime;               // 02A0  0298
	int            ping;                        // 02A4  029C
	int            minPing;                     // 02A8  02A0
	int            maxPing;                     // 02AC  02A4
	Address        address;                     // 02B0  02A8
	int            reliablePacketSendId;        // 02C0  02B8
	int            unreliablePacketSendId;      // 02C4  02BC
	int            reliablePacketRcvId;         // 02C8  02C0
	int            unreliablePacketRcvId;       // 02CC  02C4
	SList<cPacket> packetLists[4];              // 02D0  02C8
	int            listMax[4];                  // 0300  02F8
	int            listProcessingTime[4];       // 0310  0308
	uint32         lastKeepaliveTime;           // 0320  0318
	bool           mustEvict;                   // 0324  031C
	BOOL           isFlowControlEnabled;        // 0328  0320
	int            lastServiceCount;            // 032C  0324
	int            lastContactTime;             // 0330  0328
	int            targetBandwidth;             // 0334  032C
	int            maxBandwidth;                // 0338  0330
	int            id;                          // 033C  0334
	float          bandwidthMultiplier;         // 0340  0338
	float          averageObjectPriority;       // 0344  033C
	bool           isLoading;                   // 0348  0340
	bool           expectPacketFlood;           // 0349  0341
	uint32         floodTimer;                  // 034C  0348
	uint32         loadEndTime;                 // 0350  0348
	uint32         resendCount;                 // 0354  034C
	uint32         creationTime;                // 0358  0350
	uint           priorityUpdateTimer;         // 035C  0354
	uint32         extendedAveragePingTime;     // 0360  0358
	uint           extendedAveragePingCount;    // 0364  035C
	uint32         lastAveragePingTime;         // 0368  0360
	bool           isOutgoingFlooded;           // 036C  0364
	int            totalResentPacketsInQueue;   // 0370  0368
	uint32         nextOutgoingFloodActionTime; // 0374  036C
	int            outgoingFloodCount;          // 0378  0370
	sint16         unk037C;                     // 037C  ----

	// Added by TT

	float version;
	char serial[33];

	ResourceManagerState resourceManagerState;

public:

	cRemoteHost();
	~cRemoteHost();

	void Init_Stats();
	void Adjust_Resend_Timeout();
	void Add_Packet(cPacket& packet, uint8 listType);
	void Remove_Packet(int packetId, uint8 listType);
	void Adjust_Flow_If_Necessary(float sampleTime);
	void Toggle_Flow_Control();

	int Get_Last_Service_Count() { return lastServiceCount; }
	void Set_Last_Service_Count(int _lastServiceCount);

	void Compute_List_Max(int listType);
	int Get_List_Max(int listType);

	void Set_List_Processing_Time(int listType, int processingTime);
	int Get_List_Processing_Time(int listType);

	void Set_Flood(bool _expectPacketFlood);
	
	const Address& Get_Address() const { return address; }
	const cNetStats& Get_Stats() const { return stats; }
	cNetStats& Get_Stats() { return stats; }

	void Set_Is_Loading(bool _isLoading);
	bool Get_Is_Loading() const { return isLoading; }
	bool Was_Recently_Loading(const int currentTime = 0) const;

	int Get_Maximum_Bps() { return maxBandwidth; }
	void Set_Target_Bps(int _targetBandwidth) { targetBandwidth = _targetBandwidth; }
	SList<cPacket>& Get_Packet_List(uint listType) { return packetLists[listType]; }
	void Increment_Resends() { resendCount++; }
	void Set_Total_Resent_Packets_In_Queue(uint resentPackets) { totalResentPacketsInQueue = resentPackets; }
	uint32 Get_Last_Keepalive_Time_Ms() const { return lastKeepaliveTime; }
	void Set_Last_Keepalive_Time_Ms(uint _lastKeepaliveTime) { lastKeepaliveTime = _lastKeepaliveTime; };
	int Get_Resend_Timeout_Ms() const { return resendTimeout; }

	int Get_Reliable_Packet_Send_Id() { return reliablePacketSendId; }
	void Increment_Reliable_Packet_Send_Id() { reliablePacketSendId++; }
	int Get_Unreliable_Packet_Send_Id() { return unreliablePacketSendId; }
	void Increment_Unreliable_Packet_Send_Id() { unreliablePacketSendId++; }

	int Get_Reliable_Packet_Rcv_Id() const { return reliablePacketRcvId; }
	void Set_Reliable_Packet_Rcv_Id(int _reliablePacketRcvId) { reliablePacketRcvId = _reliablePacketRcvId; }
	int Get_Unreliable_Packet_Rcv_Id() const { return unreliablePacketRcvId; }
	void Set_Unreliable_Packet_Rcv_Id(int _unreliablePacketRcvId) { unreliablePacketRcvId = _unreliablePacketRcvId; }

	bool Must_Evict() const { return mustEvict; }
	void Set_Must_Evict(bool _mustEvict) { mustEvict = _mustEvict; }
	double Get_Threshold_Priority() const { return thresholdPriority; }

	float getVersion() const { return version; }
	float& getVersionRef() { return version; }
	void setVersion(float _version) { version = _version; }

	const char* getSerial() const { return serial; }

	int incrementReliablePacketSendId() { return reliablePacketSendId++; }

	int getId() const { return id; }
	void setId(int _id) { id = _id; }

	void setAddress(const Address& _address) { address = _address; }

	void setLastContactTime(uint32 _lastContactTime) { lastContactTime = _lastContactTime; }

	uint& statSample(int sampleType) { return stats.StatSample[sampleType]; }

	void setIsFlowControlEnabled(bool _isFlowControlEnabled) { isFlowControlEnabled = _isFlowControlEnabled; }

	int getTargetBandwidth() const { return targetBandwidth; }
	int getMaxBandwidth() const { return maxBandwidth; }
	void setMaxBandwidth(int bandwidth) { maxBandwidth = bandwidth; }
	float Get_Bandwidth_Multiplier() const { return bandwidthMultiplier; }

	int Get_Ping() { return ping; }
	
	TT_DEPRECATED("priorityUpdateCounter is no longer available") inline int Get_Priority_Update_Counter();// { return priorityUpdateCounter; }
	TT_DEPRECATED("priorityUpdateCounter is no longer available") inline void Increment_Priority_Count();// { if (++priorityUpdateCounter > PriorityUpdateRate) priorityUpdateCounter = 0; }
	int	Get_Target_Bps() const { return targetBandwidth; }
	bool Get_Flood() const { return expectPacketFlood; }
	void Set_Average_Priority(float _averagePriority) { averageObjectPriority = _averagePriority; }

	bool priorityUpdateTimerUpdate(uint currentTime)
	{
		if (currentTime - priorityUpdateTimer >= 500)
		{
			priorityUpdateTimer = currentTime;
			return true;
		}
		else
			return false;
	}

	ResourceManagerState getResourceManagerState() const { return resourceManagerState; }
	void setResourceManagerState(ResourceManagerState _resourceManagerState) { char buf[999]; sprintf(buf, __FUNCTION__ "(%d)\n", _resourceManagerState); OutputDebugString(buf); resourceManagerState = _resourceManagerState; }


private:

	cRemoteHost(const cRemoteHost&); // Disallow copying
	cRemoteHost& operator=(const cRemoteHost&); // Disallow copying

	void Dam_The_Flood();
	bool Is_Outgoing_Flooded();

}; // 0380  0374  (original, before TT modifications)


#endif
