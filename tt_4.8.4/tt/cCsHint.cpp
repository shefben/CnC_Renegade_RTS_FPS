#include "General.h"
#include "cCsHint.h"

#include "cNetwork.h"
#include "NetworkObjectMgrClass.h"
#include "cConnection.h"
#include "NetworkObjectClassId.h"
#include "SimpleNetworkObjectFactoryClass.h"



cCsHint::cCsHint()
{
	senderId = -1;
	objectId = -1;
	PacketType = 54;
}



void cCsHint::Init(int _objectId)
{
	senderId = cNetwork::Get_My_Id();
	objectId = _objectId;
	Set_Network_ID(NetworkObjectMgrClass::Get_New_Client_ID());
	Set_Object_Dirty_Bit(SERVER_HOST_ID, DB_CREATION, true);
	Set_Delete_Pending();
}



void cCsHint::Act()
{
	NetworkObjectClass* object = NetworkObjectMgrClass::Find_Object(objectId);
	if (object)
		object->Increment_Client_Hint_Count(senderId);
}



void cCsHint::Export_Creation(BitStreamClass& stream)
{
	cNetEvent::Export_Creation(stream);

	stream.Add(senderId);
	stream.Add(objectId);
}



void cCsHint::Import_Creation(BitStreamClass& stream)
{
	cNetEvent::Import_Creation(stream);

	stream.Get(senderId);
	stream.Get(objectId);

	Act();
}



SimpleNetworkObjectFactoryClass<cCsHint, NET_cCsHint> cCsHintFactory;
