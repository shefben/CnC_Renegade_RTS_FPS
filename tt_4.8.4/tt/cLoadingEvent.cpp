#include "General.h"
#include "cLoadingEvent.h"

#include "cPlayerManager.h"
#include "NetworkObjectMgrClass.h"
#include "cNetwork.h"
#include "cConnection.h"
#include "cGameDataUpdateEvent.h"
#include "SimpleNetworkObjectFactoryClass.h"
#include "cPlayer.h"


cLoadingEvent::cLoadingEvent()
{
	PacketType = 44;
	playerId = 0;
	isLoading = false;
}



void cLoadingEvent::Init(bool _isLoading)
{
	TT_ASSERT(cNetwork::I_Am_Client());

	Set_Network_ID(NetworkObjectMgrClass::Get_New_Client_ID());
	
	playerId = cNetwork::Get_My_Id();
	isLoading = _isLoading;
	
	if (cNetwork::PServerConnection)
		Act();
	else
		Set_Object_Dirty_Bit(SERVER_HOST_ID, DB_CREATION, true);

	Set_Delete_Pending();
}



void cLoadingEvent::Act()
{
	TT_ASSERT(cNetwork::I_Am_Server());

	Set_Delete_Pending();

	cPlayer* player = cPlayerManager::Find_Player(playerId);
	if (player)
	{
		player->Set_Is_In_Game(!isLoading);
		
		if (!isLoading)
		{
			cNetwork::PServerConnection->Set_Rhost_Expect_Packet_Flood(playerId, true);
			cNetwork::PServerConnection->Set_Rhost_Is_In_Game(playerId, true);
			
			(new cGameDataUpdateEvent())->Init(playerId);
		}
	}
}



void cLoadingEvent::Export_Creation(BitStreamClass& packet)
{
	cNetEvent::Export_Creation(packet);

	packet.Add(playerId);
	packet.Add(isLoading);
}



void cLoadingEvent::Import_Creation(BitStreamClass& packet)
{
	cNetEvent::Import_Creation(packet);

	packet.Get(playerId);
	packet.Get(isLoading);

	Act();
}



SimpleNetworkObjectFactoryClass<cLoadingEvent, NET_cLoadingEvent> cLoadingEventFactory;
