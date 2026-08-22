#ifndef TT_INCLUDE__STATICNETWORKOBJECTCLASS_H
#define TT_INCLUDE__STATICNETWORKOBJECTCLASS_H
#include "NetworkObjectClass.h"
class StaticAnimPhysClass;
class DoorNetworkObjectClass;
class ElevatorNetworkObjectClass;
class DSAPONetworkObjectClass;
class StaticNetworkObjectClass : public NetworkObjectClass
{
protected:
	StaticAnimPhysClass *phys; //6B4
	int AnimationMode; //6B8
	float LoopStart; //6BC
	float LoopEnd; //6C0
	float CurrentFrame; //6C4
	float TargetFrame; //6C8
public:
	~StaticNetworkObjectClass();
	void Import_Rare(BitStreamClass &stream);
	void Export_Rare(BitStreamClass &stream);
	void Network_Think();
	void Delete();
	int Get_Vis_ID();
	bool Get_World_Position();
	virtual void Initialize(StaticAnimPhysClass *phys);
	virtual DoorNetworkObjectClass * As_Door_Network_Object_Class(void) {return 0;};
	virtual ElevatorNetworkObjectClass * As_Elevator_Network_Object_Class(void) {return 0;}
	virtual DSAPONetworkObjectClass * As_DSAPO_Network_Object_Class(void) {return 0;}
};
#endif
