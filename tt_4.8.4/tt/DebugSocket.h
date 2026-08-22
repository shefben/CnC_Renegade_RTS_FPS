#ifndef TT_INCLUDE__DEBUGSOCKET_H
#define TT_INCLUDE__DEBUGSOCKET_H



#include "Socket.h"
#include "SList.h"



class DebugSocket :
	public Socket
{

	struct DelayedPacket
	{
		int receiveTime;
		byte* data;
		int dataSize;
		Address address;
	};

	float incomingPacketLoss;
	float incomingPacketDuplication;
	int incomingPacketLatency;
	SList<DelayedPacket> incomingPacketLatencyBuffer;
	int incomingBandwidthLimit;
	int incomingBandwidthCredit;
	int lastIncomingPacketTime;

public:

	DebugSocket();
	virtual ~DebugSocket();

	virtual int receiveFrom(byte* data, int maxDataSize, Address& address, uint flags = 0);

};



#endif