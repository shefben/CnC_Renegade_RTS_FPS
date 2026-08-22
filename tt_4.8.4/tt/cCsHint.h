#ifndef TT_INCLUDE__CCSHINT_H
#define TT_INCLUDE__CCSHINT_H



#include "cNetEvent.h"
#include "NetworkObjectClassId.h"



class BitStreamClass;



class cCsHint :
	public cNetEvent
{

	int senderId; // 06B4
	int objectId; // 06B8

public:

	virtual ~cCsHint() {}
	virtual unsigned int Get_Network_Class_ID () const { return NET_cCsHint; }
	virtual void Import_Creation(BitStreamClass& stream);
	virtual void Export_Creation(BitStreamClass& stream);
	virtual void Act();

	cCsHint();
	void Init(int networkId);

};



#endif