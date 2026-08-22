#ifndef TT_INCLUDE__DOORNETWORKOBJECTCLASS_H
#define TT_INCLUDE__DOORNETWORKOBJECTCLASS_H
#include "StaticNetworkObjectClass.h"
class DoorNetworkObjectClass : public StaticNetworkObjectClass {
	int State; //6CC
public:
	~DoorNetworkObjectClass();
	void Import_Rare(BitStreamClass &stream);
	void Export_Rare(BitStreamClass &stream);
	void Network_Think();
	void Get_Description(StringClass &Description);
	void Initialize(StaticAnimPhysClass *phys);
	DoorNetworkObjectClass * As_Door_Network_Object_Class(void) {return this;}
};
#endif
