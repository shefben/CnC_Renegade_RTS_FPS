#include "General.h"
#include "cGameOptionsEvent.h"

#include "resource.h"
#include "string_ids.h"
#include "popupdialog.h"
#include "dialogmgr.h"
#include "bitstream.h"
#include "engine_game.h"
#include "TranslateDBClass.h"
#include "cGameType.h"
#include "ModPackageMgrClass.h"
#include "DlgMPConnectionRefused.h"
#include "DlgMPConnecting.h"
#include "SimpleNetworkObjectFactoryClass.h"
#include "cNetwork.h"
#include "cRemoteHost.h"



unsigned long CRC_Stringi(char  const*, unsigned long = 0); // TODO: Move



cGameOptionsEvent::cGameOptionsEvent()
{
	PacketType = 8;
	hostedGameNumber = -1;
}



cGameOptionsEvent::~cGameOptionsEvent()
{
}



void cGameOptionsEvent::Init(int clientId)
{
	hostedGameNumber = cGameData::HostedGameNumber;

	if (clientId == -1)
		Set_Object_Dirty_Bit(DB_CREATION, true);
	else
		Set_Object_Dirty_Bit(clientId, DB_CREATION, true);
	
	Set_Delete_Pending();
}



void cGameOptionsEvent::Act()
{
	cGameData::HostedGameNumber = hostedGameNumber;

	if (IS_MULTIPLAY && cNetwork::Get_Client_Rhost()->getVersion() < 4.0f)
	{
		DlgMPConnecting* dialog = (DlgMPConnecting*)DialogMgrClass::Find_Dialog(IDD_MULTIPLAY_CONNECTING);
		if (dialog)
			dialog->setGameData(The_Game());
	}
}



void cGameOptionsEvent::Export_Creation(BitStreamClass& stream)
{
	cNetEvent::Export_Creation(stream);

	cGameData* theGame = The_Game();
	theGame->Export_Tier_1_Data((cPacket&)stream);
	theGame->Export_Tier_2_Data((cPacket&)stream);
	stream.Add(theGame->TimeRemaining_Seconds);
	stream.Add(hostedGameNumber);
	stream.Add(CRC_Stringi(theGame->ModName));
	stream.Add(CRC_Stringi(theGame->MapName));
}



void cGameOptionsEvent::Import_Creation(BitStreamClass& stream)
{
	cNetEvent::Import_Creation(stream);

	uint32 modCrc;
	uint32 mapCrc;

	cGameData* theGame = The_Game();
	TT_ASSERT(theGame);

	theGame->Import_Tier_1_Data((cPacket&)stream);
	theGame->Import_Tier_2_Data((cPacket&)stream);
	stream.Get(theGame->TimeRemaining_Seconds);
	stream.Get(hostedGameNumber);
	stream.Get(modCrc);
	stream.Get(mapCrc);

	StringClass modName;
	StringClass mapName;
	ModPackageMgrClass::Get_Mod_Map_Name_From_CRC(modCrc, mapCrc, &modName, &mapName);
	theGame->ModName = modName;
	theGame->MapName = mapName;

	bool success = true;

	if (IS_MULTIPLAY)
	{
		if (cNetwork::Get_Client_Rhost()->getVersion() < 4.0f && !theGame->Is_Map_Valid(false))
		{
			DlgMPConnecting* connectingDialog = (DlgMPConnecting*)DialogMgrClass::Find_Dialog(IDD_MULTIPLAY_CONNECTING);
			if (connectingDialog)
			{
				connectingDialog->handleRefusal();
				success = false;
			}

			WideStringClass message;
			message.Format(L"%s  %s", TRANSLATE(IDS_MP_CONNECTION_REFUSED_BY_APPLICATION), TRANSLATE(IDS_MENU_MISSING_MAP));
			DlgMPConnectionRefused::DoDialog(message, false);
		}
	}

	if (success)
		Act();
}



SimpleNetworkObjectFactoryClass<cGameOptionsEvent, NET_cGameOptionsEvent> cGameOptionsEventFactory;
