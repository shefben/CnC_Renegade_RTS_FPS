#ifndef TT_INCLUDE__CWINEVENT_H
#define TT_INCLUDE__CWINEVENT_H

#include "cNetEvent.h"
#include "NetworkObjectClassId.h"



class cWinEvent :
	public cNetEvent
{

	int hostedGameNumber; // 06B4
	int winnerId; // 06B8
	int clientId; // 06BC
	bool isMapCycleOver; // 06C0

public:

	cWinEvent();
	void Init(int, int, bool);

	virtual uint Get_Network_Class_ID() const { return NET_cWinEvent; }
	virtual void Import_Creation(BitStreamClass& stream);
	virtual void Export_Creation(BitStreamClass& stream);
	virtual void Act();

};


#endif
