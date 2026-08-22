#ifndef TT_INCLUDE__CSCTEXTOBJ_H
#define TT_INCLUDE__CSCTEXTOBJ_H



#include "cNetEvent.h"
#include "NetworkObjectClassId.h"

#include "engine_string.h"
#include "engine_ttdef.h"



class cScTextObj :
	public cNetEvent
{

public:

	int senderId; // 06B4
	int receiverId; // 06B8
	TextMessageEnum type; // 06BC
	WideStringClass message; // 06C0
	bool popup; // 06C4


public:

	cScTextObj();
	void Init(const WideStringClass& message, TextMessageEnum type, bool popup, int senderId, int receiverId);
	void Set_Dirty_Bit_For_Team(DIRTY_BIT dirtyBits, int teamId);

	virtual uint Get_Network_Class_ID() const { return NET_cScTextObj; }
	virtual void Import_Creation(BitStreamClass& stream);
	virtual void Export_Creation(BitStreamClass& stream);
	virtual void Act();

}; // 06C8

#endif
