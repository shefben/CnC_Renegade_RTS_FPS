#ifndef TT_INCLUDE_GAMEHINTEVENT_H
#define TT_INCLUDE_GAMEHINTEVENT_H
#include "cNetEvent.h"
#include "bitstream.h"
#include "SimpleNetworkObjectFactoryClass.h"
#include "NetworkObjectClassId.h"
#include "NetworkObjectMgrClass.h"

class GameHintEvent:
	public cNetEvent
{
public:
	int playerId;
	int eventId;
	char *soundPreset;
	int titleId;
	int textId;
	int textId2;
	int textId3;
	bool textureValid;
	char *textureName;
	GameHintEvent();
	void Init(int PlayerID,int EventID,const char *Sound,int TitleID,int TextID,int TextID2,int TextID3);
	void Init(int PlayerID,int EventID,const char *Sound,int TitleID,int TextID,int TextID2,int TextID3,const char *TextureName);
	virtual ~GameHintEvent();
	virtual unsigned int Get_Network_Class_ID() const;
	virtual void Import_Creation(BitStreamClass &oStream);
	virtual void Export_Creation(BitStreamClass &oStream);
	virtual void Act();
};

#endif