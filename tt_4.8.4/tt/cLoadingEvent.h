#ifndef TT_INCLUDE__CLOADINGEVENT_H
#define TT_INCLUDE__CLOADINGEVENT_H



#include "cNetEvent.h"
#include "NetworkObjectClassId.h"



class cLoadingEvent :
	public cNetEvent
{

	int playerId;
	bool isLoading;

public:

	virtual uint Get_Network_Class_ID() const { return NET_cLoadingEvent; }
	virtual void Import_Creation(BitStreamClass& package);
	virtual void Export_Creation(BitStreamClass& package);
	virtual void Act();
	
	cLoadingEvent();
	void Init(bool _isLoading);

};



#endif
