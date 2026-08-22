#include "general.h"

#include "BuildingAggregateClass.h"
#include "BuildingAggregateDefClass.h"



uint32 BuildingAggregateClass::Get_Current_State() const
{
	return state;
}



bool BuildingAggregateClass::Is_MCT()
{
	return static_cast<const BuildingAggregateDefClass*>(Get_Definition())->isTerminal();
}



RENEGADE_FUNCTION
void BuildingAggregateClass::Set_Current_State(uint32 state, bool force)
AT2(0x007236B0, 0x00722C70);



bool BuildingAggregateDefClass::isTerminal() const
{
	return terminal;
}
