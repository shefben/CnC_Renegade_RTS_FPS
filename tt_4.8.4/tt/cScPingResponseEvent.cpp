#include "General.h"
#include "cScPingResponseEvent.h"

#include "SimpleNetworkObjectFactoryClass.h"
#include "cClientPingManager.h"
#include "cNetwork.h"



cScPingResponseEvent::cScPingResponseEvent()
{
	pingId = -1;
	PacketType = 25;
}



void cScPingResponseEvent::Init(int clientId, int _pingId)
{
	pingId = _pingId;
	Set_Object_Dirty_Bit(clientId, DB_CREATION, true);

	Set_Delete_Pending();
}



void cScPingResponseEvent::Import_Creation(BitStreamClass& stream)
{
	cNetEvent::Import_Creation(stream);

	stream.Get(pingId);

	Act();
}



void cScPingResponseEvent::Export_Creation(BitStreamClass& stream)
{
	cNetEvent::Export_Creation(stream);

	stream.Add(pingId);
}



void cScPingResponseEvent::Act()
{
	cClientPingManager::Response_Received(pingId);
}



SimpleNetworkObjectFactoryClass<cScPingResponseEvent, NET_cScPingResponseEvent> cScPingResponseEventFactory;
