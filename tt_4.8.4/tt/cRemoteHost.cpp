#include "General.h"
#include "cRemoteHost.h"

#include "cNetUtil.h"
#include "SysTimeClass.h"
#include "cConnection.h"
#include "PacketManagerClass.h"
#include "engine_math.h"
#include "DebugManager.h"



REF_DEF2(cRemoteHost::PriorityUpdateRate, int, 0x0080EC78, 0x0080DE50);



cRemoteHost::cRemoteHost()
{
	lastReliablePacketId = -2;
	lastUnreliablePacketId = -2;
	resendTimeout = (uint16)cNetUtil::DefaultResendTimeoutMs;
	ping = 0;
	maxPing = 0;
	reliablePacketSendId = 0;
	unreliablePacketSendId = 0;
	reliablePacketRcvId = 0;
	unreliablePacketRcvId = 0;

	uint currentTime = SysTimeClass::SystemTime.Get();

	lastKeepaliveTime = currentTime;
	mustEvict = false;
	isFlowControlEnabled = cConnection::Is_Flow_Control_Enabled();
	lastServiceCount = 0;
	lastContactTime = 0;
	targetBandwidth = 0;
	maxBandwidth = 0;
	bandwidthMultiplier = 1.f;
	averageObjectPriority = 0.5f;
	isLoading = false;
	expectPacketFlood = false;
	loadEndTime = 0;
	resendCount = 0;
	creationTime = currentTime;
	priorityUpdateTimer = currentTime - 500;
	extendedAveragePingTime = 0;
	extendedAveragePingCount = 0;
	lastAveragePingTime = 0;
	isOutgoingFlooded = false;
	totalResentPacketsInQueue = 0;
	nextOutgoingFloodActionTime = 0;
	outgoingFloodCount = 0;

	memset(&address, 0, sizeof(address));
	thresholdPriority = isFlowControlEnabled ? 0.5f : 0.f;
	thresholdPriorityIncrement = 0.01;

	Init_Stats();

	version = 0;
	serial[0] = '\0'; // TODO: Fetch serial later on.
}



cRemoteHost::~cRemoteHost()
{
	for (uint i = 0; i < 4; ++i)
		for (SLNode<cPacket>* node = packetLists[i].Head(); node; node = node->Next())
			delete node->Data();
}



void cRemoteHost::Init_Stats()
{
	stats.Init_Net_Stats();
	Adjust_Resend_Timeout();

	pingCount = 0;
	totalPingTime = 0;
	ping = 0;
	minPing = 1000000;
	maxPing = 0;
}



void cRemoteHost::Adjust_Resend_Timeout()
{
	if (pingCount > 0)
	{
		if (maxPing && ping)
		{
			sint16 newResendTimeout = min((sint16)(3 * ping), (sint16)(maxPing * 1.3f));
			newResendTimeout = clamp<sint16>(newResendTimeout, 333, 3000);

			if (newResendTimeout < resendTimeout)
				resendTimeout = newResendTimeout;
			else
				resendTimeout = (resendTimeout + newResendTimeout) / 2;

			extendedAveragePingTime += totalPingTime;
			extendedAveragePingCount += pingCount;
			lastAveragePingTime = ping;
		}
		
		pingCount = 0;
		totalPingTime = 0;
		ping = 0;
		minPing = 0xFFFF;
		maxPing = 0;
	}
}



void cRemoteHost::Add_Packet(cPacket& packet, uint8 listType)
{
	TT_ASSERT(packet.Get_Id() >= 0);
	TT_ASSERT(listType >= 0 && listType < 4);

	cPacket* newPacket = new cPacket(packet);

	switch (listType)
	{
	case RELIABLE_SEND_LIST:
	
		if (lastReliablePacketId != -2)
			TT_ASSERT(packet.Get_Id() == lastReliablePacketId + 1);
		
		lastReliablePacketId = packet.Get_Id();
		
		packetLists[listType].Add_Tail(newPacket);
		
		break;

	case UNRELIABLE_SEND_LIST:

		if (lastUnreliablePacketId != -2)
			TT_ASSERT(packet.Get_Id() == lastUnreliablePacketId + 1);
		
		lastUnreliablePacketId = packet.Get_Id();
		
		packetLists[listType].Add_Tail(newPacket);

		break;
		
	case RELIABLE_RCV_LIST:
	case UNRELIABLE_RCV_LIST:

		for (SLNode<cPacket>* packetNode = packetLists[listType].Head(); packetNode; packetNode = packetNode->Next())
		{
			cPacket* listPacket = packetNode->Data();
			if (listPacket->Get_Id() > packet.Get_Id())
			{
				packetLists[listType].insertBefore(newPacket, *packetNode);
				return;
			}
		}

		packetLists[listType].Add_Tail(newPacket);

		break;
	}
}



void cRemoteHost::Remove_Packet(int packetId, uint8 listType)
{
	TT_ASSERT(packetId >= 0);
	TT_ASSERT(listType >= 0 && listType < 4);

	for (SLNode<cPacket>* packetNode = packetLists[listType].Head(); packetNode; packetNode = packetNode->Next())
	{
		cPacket* packet = packetNode->Data();

		if (packetId == packet->Get_Id())
		{
			if (listType == RELIABLE_SEND_LIST)
			{
				// Packets that require a resend shouldn't count towards ping time calculations. It may be being removed because
				// the ACK to the first send just came in and if we just resent it then the ping time will look really low so we get
				// biased towards a low resend timeout value on connections of variable quality. ST - 12/7/2001 12:48PM
				if (packet->Get_Resend_Count() == 0 || pingCount == 0)
				{
					uint32 currentTime = TIMEGETTIME();
					int pingTime = currentTime - packet->Get_Send_Time();

					if (packet->Get_Resend_Count() != 0 && pingCount == 0)
					{
						// If we are not getting any timing info at all then we need to do something. Use the first send time. It's going
						// to make it big but that should cut down on the resends and let us get better timing info.
						if (pingCount == 0)
							pingTime = TIMEGETTIME() - packet->Get_First_Send_Time();
					}

					totalPingTime += pingTime;
					pingCount++;
					
					TT_ASSERT(pingCount > 0);
					ping = cMathUtil::Round(totalPingTime / pingCount);
					
					if (pingTime < minPing)
						minPing = pingTime;
					
					if (pingTime > maxPing)
						maxPing = pingTime;
				}
			}

			packetLists[listType].Remove(packet);
			delete packet;
			break;
		}
	}
}



void cRemoteHost::Set_Flood(bool _expectPacketFlood)
{
	expectPacketFlood = _expectPacketFlood;
	if (expectPacketFlood)
		floodTimer = TIMEGETTIME();
}



void cRemoteHost::Adjust_Flow_If_Necessary(float sampleTime)
{
	static bool AllowExtraModemBandwidthThrottling = true; // Actually cRemoteHost::AllowExtraModemBandwidthThrottling

	if (!isFlowControlEnabled)
		return;

	const char* type = "<unknown>";

	float oldBandwidthMultiplier = bandwidthMultiplier;

	float actual = (float)PacketManager().Get_Compressed_Bandwidth_Out(address);
	float desired = (float)targetBandwidth;

	if (expectPacketFlood)
	{
		bandwidthMultiplier = 0.5f;
		if (TIMEGETTIME() - floodTimer > 8000)
			expectPacketFlood = false;
		type = "expect flood";
	}
	else
	{
		// If we are sending way more than we know the remote host can receive then we need to send much less.
		if (actual > desired)
		{
			bandwidthMultiplier *= (desired / actual) * .85f;
			type = "sending too much; throttling down";
		}
		else
		{
			// Another case we have to trap is a low bandwidth connection that has been accidentally flooded by having several
			// frames where we send more stuff than usual. It could also be a remote host that has incorrectly reported his
			// downstream bandwidth and can't receive as much as we think we can send to him. Or it could simply be some temporary
			// condition at either end of the connection or anywhere in between. If this happens we have to cut way back on sends
			// so that the connection can catch up.
			if (AllowExtraModemBandwidthThrottling)
			{
				if (isOutgoingFlooded)
				{
					if (Is_Outgoing_Flooded())
						Dam_The_Flood();
					else
						isOutgoingFlooded = false;
				}
				else
				{
					if (Is_Outgoing_Flooded())
					{
						isOutgoingFlooded = true;

						outgoingFloodCount++;
						nextOutgoingFloodActionTime = 0;
						Dam_The_Flood();
					}
				}
			}

			if (bandwidthMultiplier < 20.f)
			{
				if (actual < desired * .7f)
				{
					bandwidthMultiplier += lerp(.5f, 0.f, actual / desired);
					if (bandwidthMultiplier > 20.f)
						bandwidthMultiplier = 20.f;
					type = "not aproaching limit; throttle up fast";
				}
				else if (actual < desired * .95f)
				{
					bandwidthMultiplier += lerp(.1f, 0.f, actual / desired);
					if (bandwidthMultiplier > 20.f)
						bandwidthMultiplier = 20.f;
					type = "aproaching limit; throttle up slowly";
				}
				else
					type = "perfect; no action";
			}
		}
	}
	
	debugOutput("clientId: %d;\toldMultiplier: %f;\tmultiplier: %f;\tactual: %.0f;\tdesired: %.0f;\ttype: %s", id, oldBandwidthMultiplier, bandwidthMultiplier, actual, desired, type);
}



void cRemoteHost::Toggle_Flow_Control()
{
   isFlowControlEnabled = !isFlowControlEnabled;

   thresholdPriority = isFlowControlEnabled ? .5 : 0;
}



void cRemoteHost::Set_Last_Service_Count(int _lastServiceCount)
{
	TT_ASSERT(_lastServiceCount >= 0);
	lastServiceCount = _lastServiceCount;
}



void cRemoteHost::Compute_List_Max(int listType)
{
	TT_ASSERT(listType >= 0 && listType < 4);
	listMax[listType] = packetLists[listType].Get_Count();
}



int cRemoteHost::Get_List_Max(int listType)
{
	TT_ASSERT(listType >= 0 && listType < 4);
	return listMax[listType];
}



void cRemoteHost::Set_List_Processing_Time(int listType, int processingTime)
{
	TT_ASSERT(listType >= 0 && listType < 4);
	listProcessingTime[listType] = processingTime;
}



int cRemoteHost::Get_List_Processing_Time(int listType)
{
	TT_ASSERT(listType >= 0 && listType < 4);
	return listProcessingTime[listType];
}



void cRemoteHost::Set_Is_Loading(bool _isLoading)
{
	if (isLoading && !_isLoading)
		loadEndTime = TIMEGETTIME();
	
	isLoading = _isLoading;
}



bool cRemoteHost::Was_Recently_Loading(int time) const
{
	if (isLoading)
		return true;

	if (!time)
		time = TIMEGETTIME();

	return time - loadEndTime < 8000;
}



bool cRemoteHost::Is_Outgoing_Flooded()
{
	// A flood while loading is acceptable
	if (Was_Recently_Loading())
		return false;

	// 1. Look for a spike in the ping times. If we are sending more than the remote host can receive then ping times will
	// quickly rise as each packet backs up at the receive end and thus gets ack'd later than usual.
	if (extendedAveragePingCount)
	{
		// average ping time over the life of the connection to use as a base line.
		uint extendedAverage = extendedAveragePingTime / extendedAveragePingCount;

		if (lastAveragePingTime > 1500u || lastAveragePingTime > 500u && lastAveragePingTime > extendedAverage * 3u)
		{
			if (TIMEGETTIME() - lastContactTime < 500)
			{
				if (!isOutgoingFlooded)
				{
					debugOutput("Detected abnormal or high ping times - assuming outbound connection to rhost %d is flooded", id);
					debugOutput("Normal average ping time = %d, last average ping time = %d", extendedAverage, lastAveragePingTime);
				}
				return true;
			}
		}
	}

	// 2. An excessive number of packets in the out queue that are older than the average ping time. Since acks aren't
	// coming back we resend more and so exacerbate the problem.

	if (totalResentPacketsInQueue > 15)
	{
		int reliableSendCount = packetLists[RELIABLE_SEND_LIST].Get_Count();

		// Let's say that if more than 90% of the packets in the queue have been resent then there is a problem.
		if (totalResentPacketsInQueue > reliableSendCount * .9)
		{
			// More resends than we expect. If we are still receiving stuff from this host then chances are we are
			// flooding him.
			if (TIMEGETTIME() - lastContactTime < 500)
			{
				debugOutput("Detected abnormally high number of resends - assuming outbound connection to rhost %d is flooded", id);
				debugOutput("Total packets in queue = %d, queue packets resent = %d", reliableSendCount, totalResentPacketsInQueue);
				return true;
			}
		}
	}

	return false;
}




void cRemoteHost::Dam_The_Flood()
{
	TT_ASSERT(isOutgoingFlooded);

	debugOutput("called");

	// Try something else every now and then until things improve.
	if (TIMEGETTIME() > nextOutgoingFloodActionTime)
	{
		// Try reducing BandwidthMultiplier. This will be a temporary change and will be allowed to revert once the connection
		// recovers.
		if (bandwidthMultiplier > .5f || nextOutgoingFloodActionTime == 0)
		{
			bandwidthMultiplier = bandwidthMultiplier / 2.0f;
			debugOutput("Flood: Reduced bandwidth multiplier to %f.", bandwidthMultiplier);

			// Give this a half second to take effect.
			if (bandwidthMultiplier <= .5f)
				// Reducing MaximumBps is a last resort. Give it a little more time to recover.
				nextOutgoingFloodActionTime = TIMEGETTIME() + 2000;
			else
				nextOutgoingFloodActionTime = TIMEGETTIME() + 600;

		}
		else
		{
			maxBandwidth /= 2;

			debugOutput("Flood: Reduced bandwidth to %d.", maxBandwidth);
			
			// Give this a few seconds to take effect before stepping down again.
			nextOutgoingFloodActionTime = TIMEGETTIME() + 5000;
		}
	}
}
