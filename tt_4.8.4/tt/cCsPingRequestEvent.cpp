#include "General.h"
#include "cCsPingRequestEvent.h"

#include "SimpleNetworkObjectFactoryClass.h"
#include "cNetwork.h"
#include "NetworkObjectMgrClass.h"
#include "cConnection.h"
#include "cScPingResponseEvent.h"



cCsPingRequestEvent::cCsPingRequestEvent()
{
	clientId = -1;
	pingId = -1;
	PacketType = 50;
}



void cCsPingRequestEvent::Init(int _pingId)
{
	clientId = cNetwork::Get_My_Id();
	pingId = _pingId;
	Set_Network_ID(NetworkObjectMgrClass::Get_New_Client_ID());
	Set_Object_Dirty_Bit(SERVER_HOST_ID, DB_CREATION, true);

	Set_Delete_Pending();
}



void cCsPingRequestEvent::Import_Creation(BitStreamClass& stream)
{
	cNetEvent::Import_Creation(stream);

	stream.Get(clientId);
	stream.Get(pingId);

	Act();
}



void cCsPingRequestEvent::Export_Creation(BitStreamClass& stream)
{
	cNetEvent::Export_Creation(stream);

	stream.Add(clientId);
	stream.Add(pingId);
}



void cCsPingRequestEvent::Act()
{
	(new cScPingResponseEvent())->Init(clientId, pingId);
}



SimpleNetworkObjectFactoryClass<cCsPingRequestEvent, NET_cCsPingRequestEvent> cCsPingRequestEventFactory;
