#include "General.h"
#include "cLanChat.h"

#include "cGameSpyAdmin.h"
#include "SysTimeClass.h"
#include "cNetwork.h"
#include "GameModeManager.h"
#include "GameModeClass.h"
#include "cNetInterface.h"
#include "engine_game.h"
#include "LanGameModeClass.h"



RENEGADE_FUNCTION
cLanChat::cLanChat()
AT2(0x0047A4B0, 0x00479C40);



RENEGADE_FUNCTION
cLanChat::~cLanChat()
AT2(0x0047A530, 0x00479CC0);



RENEGADE_FUNCTION
bool cLanChat::Load_Lan_Registry_Keys()
AT2(0x0047A550, 0x00479CE0);



RENEGADE_FUNCTION
bool cLanChat::Save_Lan_Registry_Keys()
AT2(0x0047A650, 0x00479DE0);



RENEGADE_FUNCTION
void cLanChat::Init_Lan_Protocol_And_Socket()
AT2(0x0047A720, 0x00479EB0);



RENEGADE_FUNCTION
void cLanChat::Accept_Actions()
AT2(0x0047A760, 0x00479EF0);
/*
{
	DlgMPConnecting::Create(cNetInterface::Get_Side_Preference(), 0);
}
*/



void cLanChat::Refusal_Actions()
{
}



void cLanChat::Send_Position_Broadcast()
{
	if (!cGameSpyAdmin::Is_Gamespy_Game())
	{
		uint currentTime = TIMEGETTIME();
		if (currentTime - unk0014 > 1000)
		{
			unk0014 = currentTime;
			++unk0018;
			bool isHostingGame = cNetwork::I_Am_Server() && GameModeManager::Find("Combat")->Is_Active();

			cPacket packet;
			packet.Add((uint8)0);
			packet.Add_Wide_Terminated_String(cNetInterface::Get_Nickname());
			packet.Add(unk0018);
			packet.Add(isHostingGame);
			if (isHostingGame)
			{
				packet.Add(The_Game()->Get_Game_Type());
				The_Game()->Export_Tier_1_Data(packet);
			}

			cNetUtil::Broadcast(socket, 3373, packet);
		}
	}
}



RENEGADE_FUNCTION
void cLanChat::Process_Position_Broadcast(cPacket& packet)
AT2(0x0047A930, 0x0047A0C0);
/*
{
	WideStringClass localNickname = cNetInterface::Get_Nickname();
	WideStringClass remoteNickname;
	int v11;
	
	packet.Get_Wide_Terminated_String(remoteNickname.Get_Buffer(256), 256);
	packet.Get(&v11);
	packet.Get(&isHostingGame);
	if (localNickname && _wcsicmp(remoteNickname, localNickname) == 0 && v11 > unk0018)
	{
		packet.Flush();
		sub_493D40(); // Show IDD_MP_CHANGE_LAN_NICKNAME dialog.
	}
	else
	{
		if (isHostingGame)
		{
			int gameType;
			packet.Get(&gameType);
			cGameData* gameData = cGameData::Create_Game_Of_Type(gameType);
			gameData->Import_Tier_1_Data(packet);
			if (gameType->Does_Map_Exist())
			{
				RefPtr<WWOnline::ChannelData> v15;
				cGameChannelList::Add_Channel(gameData, &v15);
				delete v15.4; // Part of inlined dtor?
				v15.4 = 0; // Part of inlined dtor?
			}
			else
				delete gameData;
		}
		else
			cGameChannelList::Remove_Channel(&remoteNickname);
	}
}
*/



RENEGADE_FUNCTION
void cLanChat::Lan_Packet_Handler(cPacket&)
AT2(0x0047AAA0, 0x0047A230);



RENEGADE_FUNCTION
void cLanChat::Go_To_Location(ChatLocationEnum)
AT2(0x0047AB20, 0x0047A2B0);



static void External_Lan_Packet_Handler(cPacket& packet)
{
	LanGameModeClass::Get_Lan_Interface()->Lan_Packet_Handler(packet);
}



void cLanChat::Think()
{
	if (!cGameSpyAdmin::Is_Gamespy_Game() && unk001C != 7)
	{
		Send_Position_Broadcast();
		cNetUtil::Lan_Servicing(socket, &External_Lan_Packet_Handler);
	}
}
