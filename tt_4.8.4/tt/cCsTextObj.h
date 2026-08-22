#ifndef TT_INCLUDE__CCSTEXTOBJ_H
#define TT_INCLUDE__CCSTEXTOBJ_H



#include "cNetEvent.h"
#include "engine_string.h"
#include "engine_ttdef.h"
#include "NetworkObjectClassId.h"



class cCsTextObj :
	public cNetEvent
{

public:

	int senderId; // 006B4
	TextMessageEnum type; // 06B8
	WideStringClass message; // 06BC
	int receiverId; // 06C0


public:

	cCsTextObj();
	void Init (const WideStringClass& message, TextMessageEnum type, sint32 senderId, sint32 receiverId);

	virtual unsigned int Get_Network_Class_ID() const { return NET_cCsTextObj; }
	virtual void Import_Creation(BitStreamClass& stream);
	virtual void Export_Creation(BitStreamClass& stream);
	virtual void Act();

}; // 06C4



#endif
