#ifndef TT_INCLUDE__CCHANGETEAMEVENT_H
#define TT_INCLUDE__CCHANGETEAMEVENT_H



#include "NetworkObjectClassId.h"



class BitStreamClass;



class cChangeTeamEvent
{

public:

	cChangeTeamEvent();
	void Init();

	virtual ~cChangeTeamEvent();
	virtual uint Get_Network_Class_ID() const { return NET_cChangeTeamEvent; }
	virtual void Import_Creation(BitStreamClass& bitStream);
	virtual void Export_Creation(BitStreamClass& bitStream);
	virtual void Act();

	static bool Is_Change_Team_Possible();

};



#endif