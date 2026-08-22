#include "General.h"
#include "GameHintEvent.h"
#include "dlghint.h"
#include "dlgimghint.h"
#include "WWAudioClass.h"
#include "scripts.h"
#include "cNetwork.h"
#include "cConnection.h"

GameHintEvent::GameHintEvent()
{
	playerId = 0;
	eventId = 0;
	soundPreset = 0;
	titleId = 0;
	textId = 0;
	textId2 = 0;
	textId3 = 0;
	textureValid = false;
	textureName = 0;
}

GameHintEvent::~GameHintEvent()
{
	if (textureName)
	{
		delete[] textureName;
	}
	if (soundPreset)
	{
		delete[] soundPreset;
	}
}

unsigned int GameHintEvent::Get_Network_Class_ID() const
{
	return NET_GameHintEvent;
}

void GameHintEvent::Init(int PlayerID,int EventID,const char *Sound,int TitleID,int TextID,int TextID2,int TextID3)
{
	cNetEvent::Init();
	playerId = PlayerID;
	eventId = EventID;
	soundPreset = newstr(Sound);
	titleId = TitleID;
	textId = TextID;
	textId2 = TextID2;
	textId3 = TextID3;
	textureValid = false;
	if (cNetwork::PClientConnection && PlayerID == cNetwork::PClientConnection->Get_Local_Id())
		Act();
	else
		Set_Object_Dirty_Bit(PlayerID, DB_CREATION,true);
}

void GameHintEvent::Init(int PlayerID,int EventID,const char *Sound,int TitleID,int TextID,int TextID2,int TextID3,const char *TextureName)
{
	playerId = PlayerID;
	eventId = EventID;
	soundPreset = newstr(Sound);
	titleId = TitleID;
	textId = TextID;
	textId2 = TextID2;
	textId3 = TextID3;
	textureValid = true;
	textureName = newstr(TextureName);
	if (cNetwork::PClientConnection && PlayerID == cNetwork::PClientConnection->Get_Local_Id())
		Act();
	else
		Set_Object_Dirty_Bit(PlayerID, DB_CREATION,true);
}

void GameHintEvent::Import_Creation(BitStreamClass &oStream)
{
	cNetEvent::Import_Creation(oStream);
	oStream.Get(eventId);
	soundPreset = new char[261];
	oStream.Get_Terminated_String(soundPreset, 261);
	oStream.Get(titleId);
	oStream.Get(textId);
	oStream.Get(textId2);
	oStream.Get(textId3);
	oStream.Get(textureValid);
	if (textureValid)
	{
		textureName = new char[261];
		oStream.Get_Terminated_String(textureName, 261);
	}
	else
	{
		textureName = 0;
	}
	Act();
}

void GameHintEvent::Export_Creation(BitStreamClass &oStream)
{
	cNetEvent::Export_Creation(oStream);
	oStream.Add(eventId);
	soundPreset = new char[261];
	oStream.Add_Terminated_String(soundPreset);
	oStream.Add(titleId);
	oStream.Add(textId);
	oStream.Add(textId2);
	oStream.Add(textId3);
	oStream.Add(textureValid);
	if (textureValid)
	{
		oStream.Add_Terminated_String(textureName);
	}
	Set_Delete_Pending();
}

SimpleDynVecClass<int> Hints;

void ReadHintFile()
{
	int handle = Commands->Text_File_Open("hints.cfg");
	if (!handle)
	{
		return;
	}
	char data[100];
	while (Commands->Text_File_Get_String(handle,data,100))
	{
		Hints.Add(atoi(data));
	}
	Commands->Text_File_Close(handle);
}

extern bool HintsEnabled;
int Create_2D_Sound(const char *Sound);
void GameHintEvent::Act()
{
	if (HintsEnabled)
	{
		for (int i = 0;i < Hints.Count();i++)
		{
			if (Hints[i] == eventId)
			{
				return;
			}
		}
		Create_2D_Sound(soundPreset);
		if (textureValid)
		{
			DlgImgHint::DoDialog(titleId,textId,textId2,textId3,textureName);
		}
		else
		{
			DlgHint::DoDialog(titleId,textId,textId2,textId3);
		}
		Hints.Add(eventId);
		FILE *f = fopen("data\\hints.cfg","wt");
		if (f)
		{
			for (int i = 0;i < Hints.Count();i++)
			{
				fprintf(f,"%d\n",Hints[i]);
			}
			fclose(f);
		}
	}
}

SimpleNetworkObjectFactoryClass<GameHintEvent, NET_GameHintEvent> gameHintEventFactory;
