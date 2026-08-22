#ifndef TT_INCLUDE__CCSPINGREQUESTEVENT_H
#define TT_INCLUDE__CCSPINGREQUESTEVENT_H



#include "cNetEvent.h"
#include "NetworkObjectClassId.h"



class cCsPingRequestEvent :
	public cNetEvent
{

	int clientId; // 06B4
	int pingId; // 06B8

public:

	cCsPingRequestEvent();
	void Init(int _pingId);
	
	virtual uint Get_Network_Class_ID() const { return NET_cCsPingRequestEvent; }
	virtual void Import_Creation(BitStreamClass& stream);
	virtual void Export_Creation(BitStreamClass& stream);
	virtual void Act();

};



#endif