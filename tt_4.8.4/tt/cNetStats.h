#ifndef TT_INCLUDE__CNETSTATS_H
#define TT_INCLUDE__CNETSTATS_H



enum
{
	STAT_PktSent,
	STAT_PktRcv,
	STAT_MsgSent,
	STAT_MsgRcv,
	STAT_AppByteSent,
	STAT_AppByteRcv,
	STAT_BitsSent,
	STAT_BitsRcv,
	STAT_UPktSent,
	STAT_UPktRcv,
	STAT_UByteSent,
	STAT_UByteRcv,
	STAT_RPktSent,
	STAT_RPktRcv,
	STAT_RByteSent,
	STAT_RByteRcv,
	STAT_AckCountSent,
	STAT_AckCountRcv,
	STAT_48,
	STAT_ResendCount,
	STAT_50,
	STAT_54,
	STAT_DiscardCount,
	STAT_UPktRcv2,
	STAT_ServiceCount,
	
	STAT_COUNT
};



class cNetStats
{

	static double outgoingPacketLoss; // Added by TT, this might be a memb er variable, or it does not exist at all.

public:
	
	uint StatSample[STAT_COUNT];
	uint unk0064[STAT_COUNT];
	uint StatSnapshot[STAT_COUNT];
	uint unk012C[STAT_COUNT];
	uint unk0190[STAT_COUNT];
	uint unk01F4[STAT_COUNT];
	
	int unk0258; // 0258; some time
	int lastUnreliablePacketId; // 025C
	int freezePacketId; // 0260
	uint unreliableCount; // 0264
	int sampleStartTime; // 0268
	int unk026C; // 026C
	int unk0270; // 0270
	int remoteServiceCount; // 0274
	int unk0278; // 0278
	int unk027C; // 027C


    cNetStats();

	void Init_Net_Stats           ();
	void Set_Pc_Packetloss_Sent   (double);
	void Set_Remote_Service_Count (sint32);
	bool Update_If_Sample_Done(int currentTime, bool force = false);
	float Get_Pc_Packetloss_Received() const;
	double Get_Pc_Packetloss_Sent() const;
	
	uint32 Get_Sample_Start_Time() { return sampleStartTime; }
	int Get_Freeze_Packet_Id() { return freezePacketId; }

}; // 0280  0278


#endif
