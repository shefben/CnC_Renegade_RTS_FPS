#include "General.h"
#include "DebugSocket.h"



#include "engine_diagnostics.h"
#include "SysTimeClass.h"
#include "Config.h"



DebugSocket::DebugSocket()
{
	incomingPacketLoss = config(CONFIG_NET_PACKET_LOSS);
	incomingPacketDuplication = config(CONFIG_NET_PACKET_DUPLICATION);
	incomingPacketLatency = config(CONFIG_NET_PACKET_LATENCY);
	incomingBandwidthLimit = config(CONFIG_NET_MAX_BANDWIDTH);
	incomingBandwidthCredit = 0;
	lastIncomingPacketTime = TIMEGETTIME();
}



DebugSocket::~DebugSocket()
{
	for (SLNode<DelayedPacket>* node = incomingPacketLatencyBuffer.Head(); node; node = node->Next())
	{
		DelayedPacket* packet = node->Data();
		delete[] packet->data;
		delete packet;
	}
}



int DebugSocket::receiveFrom(byte* data, int maxDataSize, Address& address, uint flags)
{
	int currentTime = TIMEGETTIME();
	int newDataSize = Socket::receiveFrom(data, maxDataSize, address, flags);
	
	if (newDataSize <= 0)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK)
			return 0;
	}
	else
	{
		if (rand() >= RAND_MAX * incomingPacketLoss)
		{
			do
			{
				DelayedPacket* newPacket = new DelayedPacket;
				newPacket->receiveTime = currentTime + incomingPacketLatency;
				newPacket->data = new byte[newDataSize];
				memcpy(newPacket->data, data, newDataSize);
				newPacket->dataSize = newDataSize;
				newPacket->address = address;

				bool added = false;
				for (SLNode<DelayedPacket>* node = incomingPacketLatencyBuffer.Head(); node; node = node->Next())
				{
					if (node->Data()->receiveTime > newPacket->receiveTime)
					{
						incomingPacketLatencyBuffer.insertBefore(newPacket, *node);
						added = true;
						break;
					}
				}
				if (!added)
					incomingPacketLatencyBuffer.Add_Tail(newPacket);

			} while (rand() < RAND_MAX * incomingPacketDuplication);
		}
	}


	SLNode<DelayedPacket>* headNode = incomingPacketLatencyBuffer.Head();
	if (headNode)
	{
		DelayedPacket* packet = headNode->Data();
		if (packet->receiveTime <= currentTime)
		{
			int dataSize = packet->dataSize;

			incomingBandwidthCredit += (currentTime - lastIncomingPacketTime) * incomingBandwidthLimit / 1000;
			lastIncomingPacketTime = currentTime;

			if (incomingBandwidthCredit > incomingBandwidthLimit)
				incomingBandwidthCredit = incomingBandwidthLimit;

			if (rand() < RAND_MAX * (1. - incomingBandwidthCredit / (.1 * incomingBandwidthLimit)))
			{
				delete[] packet->data;
				delete packet;
				incomingPacketLatencyBuffer.Remove_Head();
			}
			else
			{
				incomingBandwidthCredit -= (dataSize + 34) * 8; // 48 for UDP/IPv4 header size.

				memcpy(data, packet->data, dataSize);
				address = packet->address;

				delete[] packet->data;
				delete packet;
				incomingPacketLatencyBuffer.Remove_Head();

				return dataSize;
			}
		}
	}

	WSASetLastError(WSAEWOULDBLOCK);
	return 0;
}
