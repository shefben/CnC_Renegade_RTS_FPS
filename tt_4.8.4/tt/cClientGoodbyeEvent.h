#ifndef TT_INCLUDE__CCLIENTGOODBYEEVENT_H
#define TT_INCLUDE__CCLIENTGOODBYEEVENT_H



#include "cNetEvent.h"
#include "NetworkObjectClassId.h"



class cClientGoodbyeEvent :
	public cNetEvent
{

public:

	int senderId; // 06B4


	cClientGoodbyeEvent();
	~cClientGoodbyeEvent();
	virtual void Init();
	virtual void Act();
	virtual void Export_Creation(BitStreamClass&);
	virtual void Import_Creation(BitStreamClass&);
	virtual unsigned int Get_Network_Class_ID() const { return NET_cClientGoodbyeEvent; }


};



#endif