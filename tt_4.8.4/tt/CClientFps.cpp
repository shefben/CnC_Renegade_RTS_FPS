#include "General.h"
#include "CClientFps.h"



#include "cNetwork.h"
#include "NetworkObjectMgrClass.h"
#include "cConnection.h"
#include "cPlayerManager.h"
#include "cPlayer.h"


REF_DEF2(PClientFps, CClientFps*, 0x0082FCFC, 0x0082EEE4);



CClientFps::CClientFps()
{
	clientId = -1;
	fps = 0;
	PacketType = 49;
}



void CClientFps::Init()
{
	clientId = cNetwork::Get_My_Id();
	Set_Network_ID(NetworkObjectMgrClass::Get_New_Client_ID());
	Set_Object_Dirty_Bit(SERVER_HOST_ID, DB_CREATION, true);
}



void CClientFps::Set_Fps(int _fps)
{
	fps = (uint8)_fps;
	Set_Object_Dirty_Bit(SERVER_HOST_ID, DB_FREQUENT, 1);
}



void CClientFps::Act()
{
	cPlayer* player = cPlayerManager::Find_Player(clientId);
	TT_ASSERT(player);

	if (player)
		player->Set_Fps(fps);
}



void CClientFps::Export_Creation(BitStreamClass& bitStream)
{
	bitStream.Add(clientId);
}



void CClientFps::Import_Creation(BitStreamClass& bitStream)
{
	bitStream.Get(clientId);
}



void CClientFps::Export_Frequent(BitStreamClass& bitStream)
{
	bitStream.Add(fps);
}



void CClientFps::Import_Frequent(BitStreamClass& bitStream)
{
	bitStream.Get(fps);
	Act();
}



void CClientFps::Delete()
{
	delete this;
}