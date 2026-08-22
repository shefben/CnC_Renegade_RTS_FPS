#ifndef TT_INCLUDE__CGAMEOPTIONSEVENT_H
#define TT_INCLUDE__CGAMEOPTIONSEVENT_H



#include "cNetEvent.h"
#include "NetworkObjectClassId.h"



class cGameOptionsEvent :
	public cNetEvent
{

private:

	int hostedGameNumber; // 06B4

public:

	cGameOptionsEvent();
	~cGameOptionsEvent();
	void Init(int clientId);
	virtual void Act();
	virtual void Export_Creation(BitStreamClass& bitStream);
	virtual void Import_Creation(BitStreamClass& bitStream);
	virtual unsigned int Get_Network_Class_ID() const { return NET_cGameOptionsEvent; }

};



#endif