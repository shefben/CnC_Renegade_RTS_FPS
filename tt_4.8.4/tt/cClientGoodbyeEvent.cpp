#include "General.h"
#include "cClientGoodbyeEvent.h"



#include "cConnection.h"
#include "cNetwork.h"
#include "NetworkObjectMgrClass.h"
#include "SimpleNetworkObjectFactoryClass.h"
#include "NetworkObjectClassId.h"



cClientGoodbyeEvent::cClientGoodbyeEvent()
{
	senderId = INVALID_HOST_ID;
	PacketType = 42;
}



cClientGoodbyeEvent::~cClientGoodbyeEvent()
{
}



void cClientGoodbyeEvent::Init()
{
	senderId = cNetwork::Get_My_Id();
	Set_Network_ID(NetworkObjectMgrClass::Get_New_Client_ID());
	if (cNetwork::I_Am_Server())
		Act();
	else
		Set_Object_Dirty_Bit(SERVER_HOST_ID, DB_CREATION, true);
	
	Set_Delete_Pending();
}



void cClientGoodbyeEvent::Act()
{
	cNetwork::Server_Kill_Connection(senderId);
	cNetwork::Cleanup_After_Client(senderId);
}



void cClientGoodbyeEvent::Import_Creation(BitStreamClass& stream)
{
	cNetEvent::Import_Creation(stream);

	stream.Get(senderId);

	Act();
}



void cClientGoodbyeEvent::Export_Creation(BitStreamClass& bitStream)
{
	cNetEvent::Export_Creation(bitStream);

	bitStream.Add(senderId);
}



SimpleNetworkObjectFactoryClass<cClientGoodbyeEvent, NET_cClientGoodbyeEvent> cClientGoodbyeEventFactory;
