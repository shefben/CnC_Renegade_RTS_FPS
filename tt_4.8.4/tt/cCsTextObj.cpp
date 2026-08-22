#include "General.h"
#include "cCsTextObj.h"

#include "cScTextObj.h"
#include "GameModeManager.h"
#include "GameModeClass.h"
#include "bitstream.h"
#include "NetworkObjectMgrClass.h"
#include "cNetwork.h"
#include "NetworkObjectClassId.h"
#include "SimpleNetworkObjectFactoryClass.h"
#include "cConnection.h"



bool Do_Recieve_Data_Cs(cCsTextObj* Message2);



cCsTextObj::cCsTextObj()
{
	senderId = -1;
	type = PublicMessage;
	receiverId = -1;
	PacketType = 36;
}



void cCsTextObj::Act()
{
	if (Do_Recieve_Data_Cs(this))
		if (GameModeManager::Find("Combat")->Is_Active())
			(new cScTextObj())->Init(message, type, false, senderId, receiverId);
}



void cCsTextObj::Export_Creation(BitStreamClass& stream)
{
	cNetEvent::Export_Creation(stream);

	stream.Add(senderId);
	stream.Add((char&)type);
	stream.Add_Wide_Terminated_String(message);
	stream.Add(receiverId);
}



void cCsTextObj::Import_Creation(BitStreamClass& stream)
{
	cNetEvent::Import_Creation(stream);

	stream.Get(senderId);
	stream.Get((char&)type);
	stream.Get_Wide_Terminated_String(message.Get_Buffer(512), 512, false);
	stream.Get(receiverId);

	Act();
}



void cCsTextObj::Init(const WideStringClass& _message, TextMessageEnum _type, int _senderId, int _receiverId)
{
	message = _message;
	type = _type;
	senderId = _senderId;
	receiverId = _receiverId;

	Set_Network_ID(NetworkObjectMgrClass::Get_New_Client_ID());
	if (cNetwork::I_Am_Server())
		Act();
	else
		Set_Object_Dirty_Bit(SERVER_HOST_ID, DB_CREATION, true);

	Set_Delete_Pending();
}



SimpleNetworkObjectFactoryClass<cCsTextObj, NET_cCsTextObj> cCsTextObjFactory;
