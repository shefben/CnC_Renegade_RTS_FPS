#include "General.h"
#include "BuildingAggregateClass.h"



void BuildingAggregateClass::On_Post_Load()
{
	StaticAnimPhysClass::On_Post_Load();
	Set_Current_State(state, true);
}
