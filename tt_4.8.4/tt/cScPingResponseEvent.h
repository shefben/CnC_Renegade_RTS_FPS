#ifndef TT_INCLUDE__CSCPINGRESPONSEEVENT_H
#define TT_INCLUDE__CSCPINGRESPONSEEVENT_H



#include "cNetEvent.h"
#include "NetworkObjectClassId.h"



class cScPingResponseEvent :
	public cNetEvent
{

	int pingId; // 06B4

public:

	cScPingResponseEvent();
	void Init(int clientId, int _pingId);
	
	virtual uint Get_Network_Class_ID() const { return NET_cScPingResponseEvent; }
	virtual void Import_Creation(BitStreamClass& stream);
	virtual void Export_Creation(BitStreamClass& stream);
	virtual void Act();

};



#endif