#include "General.h"
#include "cGameDataUpdateEvent.h"

#include "bitstream.h"
#include "engine_game.h"
#include "SimpleNetworkObjectFactoryClass.h"
#include "SList.h"
#include "cPlayerManager.h"
#include "cNetwork.h"



cGameDataUpdateEvent::cGameDataUpdateEvent()
{
	PacketType = 24;

	timeRemaining = 0;
}



cGameDataUpdateEvent::~cGameDataUpdateEvent()
{
}



void cGameDataUpdateEvent::Init(int clientId)
{
	timeRemaining = (int)The_Game()->TimeRemaining_Seconds;
	hostedGameNumber = cGameData::HostedGameNumber;
	if (clientId == -1)
		Set_Object_Dirty_Bit(DB_CREATION, true);
	else
		Set_Object_Dirty_Bit(clientId, DB_CREATION, true);
	
	Set_Delete_Pending();
}



void cGameDataUpdateEvent::Act()
{
	if (The_Game() && timeRemaining > 0)
		The_Game()->TimeRemaining_Seconds = (float)timeRemaining;
	
	cGameData::HostedGameNumber = hostedGameNumber;
}



void cGameDataUpdateEvent::Export_Creation(BitStreamClass& stream)
{
	cNetEvent::Export_Creation(stream);

	stream.Add(timeRemaining);
	stream.Add(hostedGameNumber);
}



void cGameDataUpdateEvent::Import_Creation(BitStreamClass& stream)
{
	cNetEvent::Import_Creation(stream);

	stream.Get(timeRemaining);
	stream.Get(hostedGameNumber);
	Act();
	Set_Delete_Pending();
}



SimpleNetworkObjectFactoryClass<cGameDataUpdateEvent, NET_cGameDataUpdateEvent> cGameDataUpdateEventFactory;
