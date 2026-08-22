#include "General.h"
#include "cScTextObj.h"

#include "engine_game.h"
#include "engine_tdb.h"
#include "TranslateDBClass.h"
#include "string_ids.h"
#include "WWAudioClass.h"
#include "Matrix3d.h"
#include "cNetwork.h"
#include "DlgMsgBox.h"
#include "MessageWindowClass.h"
#include "bitstream.h"
#include "cPlayerManager.h"
#include "CombatManager.h"
#include "ConsoleModeClass.h"
#include "SoldierGameObj.h"
#include "cConnection.h"
#include "cPlayerManager.h"
#include "SimpleNetworkObjectFactoryClass.h"
#include "MessageWindowClass.h"



void Do_Recieve_Data_Sc(cScTextObj *Message);

REF_DEF2(OptionFlags, unsigned int, 0x007F8E2C, 0x007F7EAC);
extern REF_DECL2(PrivateMessageColor, Vector3);
extern REF_DECL2(PublicMessageColor, Vector3);



cScTextObj::cScTextObj()
{
	senderId = -1;
	receiverId = -1;
	type = PublicMessage;
	popup = false;

	PacketType = 11;
}



void cScTextObj::Init(const WideStringClass& _message, TextMessageEnum _type, bool _popup, int _senderId, int _receiverId)
{
	message = _message;
	type = _type;
	senderId = _senderId;
	receiverId = _receiverId;
	popup = _popup;

	if (type == TeamMessage)
		Set_Dirty_Bit_For_Team(DB_CREATION, cPlayerManager::Find_Player(senderId)->PlayerType);
	else if (type == PrivateMessage)
	{
		if (receiverId >= 0)
			Set_Object_Dirty_Bit(receiverId, DB_CREATION, true);
	}
	else if (type == PublicMessage)
		Set_Object_Dirty_Bit(DB_CREATION, true);
	
	if (senderId != -1)
		Set_Object_Dirty_Bit(senderId, DB_CREATION, true);
	
	if ((cNetwork::I_Am_Client() && Is_Client_Dirty(cNetwork::Get_My_Id())) || senderId == -1 || receiverId == -1)
	{
		bool b = true;
		if (type == TeamMessage)
		{
			b = false;
			cPlayer* p = cPlayerManager::Find_Player(senderId);
			if (p && p->Is_Team_Player() && CombatManager::Get_The_Star())
			{
				if (p->PlayerType == CombatManager::Get_The_Star()->Get_Player_Type())
					b = true;
			}
			else
			{
				if (cNetwork::I_Am_Only_Server() && ConsoleBox.Is_Exclusive() && The_Game() && !The_Game()->IsClanGame)
					b = true;
			}
		}

		if (b)
			Act();
	}

	Set_Delete_Pending();
}



void cScTextObj::Set_Dirty_Bit_For_Team(DIRTY_BIT dirtyBit, int teamId)
{
	TT_ASSERT(cNetwork::I_Am_Server());

	for (SLNode<cPlayer>* playerNode = cPlayerManager::Get_Player_Object_List().Head(); playerNode; playerNode = playerNode->Next())
	{
		cPlayer* player = playerNode->Data();

		if (player->IsActive && player->PlayerType == teamId)
			Set_Object_Dirty_Bit(player->PlayerId, dirtyBit, true);
	}
}



void cScTextObj::Import_Creation(BitStreamClass& stream)
{
	cNetEvent::Import_Creation(stream);

	stream.Get((char&)type);
	stream.Get(senderId);
	stream.Get(receiverId);
	stream.Get(popup);
	stream.Get_Wide_Terminated_String(message.Get_Buffer(512), 512, false);

	Act();

	Do_Recieve_Data_Sc(this);
}



void cScTextObj::Export_Creation(BitStreamClass& stream)
{
	cNetEvent::Export_Creation(stream);

	stream.Add((char&)type);
	stream.Add(senderId);
	stream.Add(receiverId);
	stream.Add(popup);
	stream.Add_Wide_Terminated_String(message);
}



void cScTextObj::Act()
{
	bool wide = message.Is_ANSI();
	if (wide && (OptionFlags & 0x10) ||
		!wide && (OptionFlags & 8))
	{
		WideStringClass w;
		WideStringClass w2;
		Vector3 messageColor;
		if (senderId == -1)
		{
			w = Get_Wide_Translated_String(IDS_MP_HOST);
			messageColor = PublicMessageColor;
		}
		else
		{
			cPlayer* senderPlayer = cPlayerManager::Find_Player(senderId);
			if (senderPlayer)
			{
				if (senderPlayer->PlayerName)
					w = senderPlayer->PlayerName;
				
				messageColor = senderPlayer->Get_Color();
			}
		}

		if (receiverId == -1)
			w2 = Get_Wide_Translated_String(IDS_MP_HOST);
		else
		{
			cPlayer* receiverPlayer = cPlayerManager::Find_Player(receiverId);
			if (receiverPlayer)
				w2 = receiverPlayer->PlayerName;
		}

		if (!w.Is_Empty() && !w2.Is_Empty())
		{
			StringClass sound;
			switch (type)
			{
			case PublicMessage:
				sound = "Public_Message";
				messageColor = PublicMessageColor;
				break;
			case TeamMessage:
				sound = "Team_Message";
				break;
			case PrivateMessage:
				sound = "Private_Message";
				messageColor = PrivateMessageColor;
				break;
			}

			WWAudioClass::_theInstance->Create_Instant_Sound(sound, Matrix3D::Identity, NULL, 0, 2);

			WideStringClass w3;
			if (type == PrivateMessage)
				w3.Format(L"%s (%s %s): ", w, TRANSLATE(IDS_MP_TO), w2);
			else
				w3.Format(L"%s: ", w);
			
			if (popup && cNetwork::I_Am_Client() &&
				(!cNetwork::I_Am_Server() || (type == PrivateMessage && cNetwork::Get_My_Id() != receiverId)))
				DlgMsgBox::DoDialog(TRANSLATE(IDS_MENU_ADMIN_MESSAGE), message);
			else
			{
				WideStringClass w4;
				w4.Format(L"%s\n", message);
				w3 += w4;
				if (MessageWindow)
					MessageWindow->Add_Message(w3, messageColor, 0, 0);

				if (wide)
				{
					if (type == PublicMessage)
						ConsoleBox.Add_Message(&w3, &messageColor, false);
					else if (type == TeamMessage)
					{
						if (cNetwork::I_Am_Only_Server() && ConsoleBox.Is_Exclusive() && The_Game() && !The_Game()->IsClanGame)
						{
							StringClass s2;
							s2.Copy_Wide(w3);

							StringClass logMessage = "[Team] ";
							logMessage += s2;
							ConsoleBox.Log_To_Disk(logMessage);
						}
					}
				}
			}
		}
	}
}



SimpleNetworkObjectFactoryClass<cScTextObj, NET_cScTextObj> cScTextObjFactory;
