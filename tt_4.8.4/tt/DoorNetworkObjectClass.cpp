#include "General.h"
#include "DoorNetworkObjectClass.h"
#include "DoorPhysClass.h"
#include "bitstream.h"
void DoorNetworkObjectClass::Import_Rare(BitStreamClass &stream)
{
	stream.Get(State,BITPACK_DOOR_STATE);
	if (phys && phys->As_DoorPhysClass())
	{
		phys->As_DoorPhysClass()->Set_State(State);
	}
}
void DoorNetworkObjectClass::Export_Rare(BitStreamClass &stream)
{
	stream.Add(State,BITPACK_DOOR_STATE);
}
