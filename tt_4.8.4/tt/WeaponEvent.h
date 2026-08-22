#ifndef TT_INCLUDE_WEAPONEVENT_H
#define TT_INCLUDE_WEAPONEVENT_H
#include "cNetEvent.h"
#include "bitstream.h"
#include "SimpleNetworkObjectFactoryClass.h"
#include "NetworkObjectClassId.h"
#include "NetworkObjectMgrClass.h"
#include "WeaponClass.h"
class WeaponEvent:
	public cNetEvent
{
private:
	int Clip;
	int Inventory;
	int OwnerId;
	int Index;
public:
	WeaponEvent();
	void Init(int clip,int inventory,WeaponClass *weap);
	virtual ~WeaponEvent();
	virtual unsigned int Get_Network_Class_ID() const;
	virtual void Import_Creation(BitStreamClass &oStream);
	virtual void Export_Creation(BitStreamClass &oStream);
	virtual void Act();
};

#endif