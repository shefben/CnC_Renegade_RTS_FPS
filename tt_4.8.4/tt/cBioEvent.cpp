#include "General.h"
#include "cBioEvent.h"

#include "cNetwork.h"
#include "cNetInterface.h"
#include "NetworkObjectMgrClass.h"
#include "cGameSpyAdmin.h"
#include "cGod.h"
#include "cGameType.h"
#include "cRemoteHost.h"
#include "cConnection.h"
#include "cGameDataUpdateEvent.h"
#include "ConsoleModeClass.h"
#include "cWinEvent.h"
#include "SimpleNetworkObjectFactoryClass.h"
#include "Iterator.h"
#include "engine_player.h"
#include "engine_game.h"
void Do_Version_Send();



cBioEvent::cBioEvent()
{
	PacketType = 43;

	ClientID = 0;
	TeamID = -1;
	ClanID = 0;
}



void cBioEvent::Init(int teamid, int clanid)
{
	cNetEvent::Init();
	Set_Network_ID(NetworkObjectMgrClass::Get_New_Client_ID());

	Do_Version_Send();

	ClientID = cNetwork::Get_My_Id();
	Nickname = cNetInterface::Get_Nickname();
	strcpy(MapName, The_Game()->MapName);
	TeamID = teamid;
	ClanID = clanid;

	if (cNetwork::I_Am_Server())
		Act();
	else
		Set_Object_Dirty_Bit(SERVER_HOST_ID, DB_CREATION, true);
}



void cBioEvent::Act()
{
	// TODO: Use Acceptance_Handler name here.

	if (!cNetwork::isPlayerNameAllowed(Nickname))
	{
		Console_Output("Player with invalid nickname blocked, player IP was %s\n", inet_ntoa(cNetwork::Get_Server_Rhost(ClientID)->Get_Address().sin_addr));
		cNetwork::Server_Kill_Connection(ClientID);
		return;
	}
	
	if (cGameSpyAdmin::Is_Nickname_Collision(Nickname))
	{
		WideStringClass str;
		int count = 1;
		do
		{
			str.Format(L"%s(%d)",Nickname,count);
			count++;
		} while (cGameSpyAdmin::Is_Nickname_Collision(str));
		Nickname = str;
	}

	for (Iterator<SList<cPlayer>> player(PlayerList); player; ++player)
		cNetwork::Send_Object_Update(player, ClientID);

	cPlayer* player = cGod::Create_Player(ClientID,Nickname,TeamID,ClanID,false);
	if (IS_MULTIPLAY)
	{
		player->Set_Ip_Address(cNetwork::Get_Server_Rhost(ClientID)->Get_Address().sin_addr.S_un.S_addr);
		if (cNetwork::PServerConnection)
			cNetwork::PServerConnection->Set_Rhost_Expect_Packet_Flood(player->PlayerId,true);
		
		(new cGameDataUpdateEvent())->Init(player->PlayerId);
		ConsoleBox.Print_Maybe("Player %S joined the game\n", Nickname);
		if (The_Game()->IsIntermission)
		{
			if (_stricmp(MapName, The_Game()->MapName) != 0)
				(new cWinEvent())->Init(The_Game()->WinnerID, ClientID, The_Game()->MapCycleOver);
			else
			{
				player->Set_Is_In_Game(false);
				player->Set_Is_Waiting_For_Intermission(true);
			}
		}
	}
}



void cBioEvent::Import_Creation(BitStreamClass& stream)
{
	cNetEvent::Import_Creation(stream);
	
	stream.Get(ClientID);
	stream.Get_Wide_Terminated_String(Nickname.Get_Buffer(256), 256, true);
	stream.Get(TeamID);
	stream.Get(ClanID);
	stream.Get_Terminated_String(MapName, 256, false);

	Act();
}



void cBioEvent::Export_Creation(BitStreamClass& stream)
{
	cNetEvent::Export_Creation(stream);

	stream.Add(ClientID);
	stream.Add_Wide_Terminated_String(Nickname);
	stream.Add(TeamID);
	stream.Add(ClanID);
	stream.Add_Terminated_String(MapName);
}



SimpleNetworkObjectFactoryClass<cBioEvent, NET_cBioEvent> cBioEventFactory;
