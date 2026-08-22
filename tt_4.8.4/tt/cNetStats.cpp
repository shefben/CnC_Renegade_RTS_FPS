#include "General.h"
#include "cNetStats.h"



#include "cNetUtil.h"



double cNetStats::outgoingPacketLoss = 0;



cNetStats::cNetStats() :
	lastUnreliablePacketId(-1),
	freezePacketId(-1)
{
	Init_Net_Stats();
}



RENEGADE_FUNCTION
void cNetStats::Init_Net_Stats()
AT2(0x006209F0, 0x00620290);



void cNetStats::Set_Remote_Service_Count(sint32 _remoteServiceCount)
{
	remoteServiceCount = _remoteServiceCount;
}




void cNetStats::Set_Pc_Packetloss_Sent(double _outgoingPacketLoss)
{
	outgoingPacketLoss = _outgoingPacketLoss; // Not done in original code.
}



bool cNetStats::Update_If_Sample_Done(int currentTime, bool force)
{
	if (force || currentTime - unk0258 > cNetUtil::NETSTATS_SAMPLE_TIME_MS)
	{
		static int update_count = 0;
		update_count++;
		
		float sampleScale = 1000.f / max(1, currentTime - sampleStartTime);
		
		for (int i = 0; i < STAT_COUNT; i++)
		{
			unk0190[i] += StatSample[i];
			unk01F4[i] = (int)(unk0190[i] * sampleScale);
			StatSnapshot[i] = StatSample[i];
			
			if (update_count % 4 == 0)
			{
				unk012C[i] = StatSample[i] + unk0064[i];
				unk0064[i] = 0;
			}
			else
				unk0064[i] += StatSample[i];
			
			StatSample[i] = 0;
		}
		
		unk0258 = currentTime;
		return true;
	}
	
	return false;
}



float cNetStats::Get_Pc_Packetloss_Received() const
{
	int receivedPacketCount = lastUnreliablePacketId - freezePacketId;
	if (receivedPacketCount > 0)
		return (1 - (float)unreliableCount / receivedPacketCount) * 100;

	return 0;
}



double cNetStats::Get_Pc_Packetloss_Sent() const
{
	return outgoingPacketLoss;
}