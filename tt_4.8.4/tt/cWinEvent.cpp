#include "General.h"
#include "cWinEvent.h"

#include "string_ids.h"
#include "cNetwork.h"
#include "cGameType.h"
#include "GameModeManager.h"
#include "GameModeClass.h"
#include "WWAudioClass.h"
#include "cTeamManager.h"
#include "cPlayerManager.h"
#include "ModPackageMgrClass.h"
#include "SimpleNetworkObjectFactoryClass.h"
#include "Engine_Game.h"
#include "TranslateDBClass.h"

unsigned long CRC_Stringi(char  const*, unsigned long = 0); // TODO: Move



cWinEvent::cWinEvent() :
	hostedGameNumber(-1),
	winnerId(-99999),
	clientId(-99999),
	isMapCycleOver(false)
{
	PacketType = 13;
}



void cWinEvent::Init(int _winnerId, int _clientId, bool _isMapCycleOver)
{
	cNetEvent::Init();
	
	winnerId = _winnerId;
	clientId = _clientId;
	isMapCycleOver = _isMapCycleOver;
	hostedGameNumber = cGameData::HostedGameNumber;

	if (clientId > 0)
		Set_Object_Dirty_Bit(clientId, DB_CREATION, true);
	else
		Set_Object_Dirty_Bit(DB_CREATION, true);

	if (cNetwork::I_Am_Client() && (clientId <= 0 || clientId == cNetwork::Get_My_Id()))
		Act();
}



void cWinEvent::Import_Creation(BitStreamClass& stream)
{
	cNetEvent::Import_Creation(stream);
	
	int winType = 0;
	int gameDuration = 0;
	WideStringClass mvpName(0x100, true);
	int mvpCount = 0;
	uint32 modCrc = 0;
	uint32 mapCrc = 0;
	
	stream.Get(winnerId);
	stream.Get(clientId);
	stream.Get(hostedGameNumber);
	stream.Get(isMapCycleOver);
	stream.Get(winType);
	stream.Get(gameDuration);
	stream.Get_Wide_Terminated_String(mvpName.Peek_Buffer(), 0x100);
	stream.Get(mvpCount);
	stream.Get(modCrc);
	stream.Get(mapCrc);
	
	StringClass modName(0, true);
	StringClass mapName(0, true);
	ModPackageMgrClass::Get_Mod_Map_Name_From_CRC(modCrc, mapCrc, &modName, &mapName);
	
	cGameData::HostedGameNumber = hostedGameNumber + 1;
	The_Game()->WinnerID = winnerId;
	The_Game()->WinType = winType;
	The_Game()->GameDuration_Seconds = gameDuration;
	The_Game()->MVPName = mvpName;
	The_Game()->MVPCount = mvpCount;
	The_Game()->ModName = modName;
	The_Game()->MapName = mapName;
	
	The_Game()->Begin_Intermission();
	
	Act();
}



void cWinEvent::Export_Creation(BitStreamClass& stream)
{
	cNetEvent::Export_Creation(stream);

	stream.Add(winnerId);
	stream.Add(clientId);
	stream.Add(hostedGameNumber);
	stream.Add(isMapCycleOver);
	stream.Add(The_Game()->WinType);
	stream.Add(The_Game()->GameDuration_Seconds);
	stream.Add_Wide_Terminated_String(The_Game()->MVPName);
	stream.Add(The_Game()->MVPCount);
	stream.Add(CRC_Stringi(The_Game()->ModName));
	stream.Add(CRC_Stringi(The_Game()->MapName));
}



void cWinEvent::Act()
{
	if (!IS_MISSION)
	{
		if (GameModeManager::Find("Combat")->Is_Active())
		{
			TT_ASSERT(The_Game());

			WWAudioClass::_theInstance->Create_Instant_Sound("Game_Over", Matrix3D::Identity, NULL, 0, 2);
			
			WideStringClass winText;
			winText.Format(L"%s %s %s", The_Game()->Get_Team_Word(), cTeamManager::Get_Team_Name(winnerId), TRANSLATE(IDS_MP_WIN_FORMATTING));
			cGameData::Set_Win_Text(winText);
			
			The_Game()->MapCycleOver = isMapCycleOver;
		}
	}
}



SimpleNetworkObjectFactoryClass<cWinEvent, NET_cWinEvent> cWinEventFactory;
