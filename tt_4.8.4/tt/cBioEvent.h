#ifndef TT_INCLUDE__CBIOEVENT_H
#define TT_INCLUDE__CBIOEVENT_H



#include "cNetEvent.h"
#include "NetworkObjectClassId.h"
#include "engine_string.h"



class cBioEvent :
	public cNetEvent
{
	
	int ClientID; // 06B4
	WideStringClass Nickname; // 06B8
	char MapName[256]; // 06BC
	int TeamID; // 07BC
	unsigned long ClanID; // 07C0

public:

	virtual unsigned int Get_Network_Class_ID() const { return NET_cBioEvent; }
	virtual void Import_Creation(BitStreamClass& stream);
	virtual void Export_Creation(BitStreamClass& stream);
	virtual void Act();

	cBioEvent();
	void Init(int teamid,int clanid);

}; // 07C4



#endif
