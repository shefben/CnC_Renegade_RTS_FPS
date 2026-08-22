#ifndef TT_INCLUDE__CGAMEDATAUPDATEEVENT_H
#define TT_INCLUDE__CGAMEDATAUPDATEEVENT_H



#include "cNetEvent.h"
#include "NetworkObjectClassId.h"



class cGameDataUpdateEvent :
	public cNetEvent
{
public:
	int timeRemaining;
	int hostedGameNumber;

	virtual ~cGameDataUpdateEvent();
	virtual unsigned int Get_Network_Class_ID() const { return NET_cGameDataUpdateEvent; }
	virtual void   Act();
	virtual void   Export_Creation(BitStreamClass& stream);
	virtual void   Import_Creation(BitStreamClass& stream);
	cGameDataUpdateEvent();
	void Init(int clientid);
};



#endif
