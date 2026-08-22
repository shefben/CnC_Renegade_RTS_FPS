#include "General.h"
#include "ElevatorPhysClass.h"
#include "EncoderList.h"
#include "bitpackids.h"

void ElevatorPhysClass::Set_Precision()
{
	cEncoderList::Set_Precision( BITPACK_ELEVATOR_STATE,					0, (int)ElevatorPhysClass::STATE_MAX );
	cEncoderList::Set_Precision( BITPACK_ELEVATOR_TOP_DOOR_STATE,		0, (int)ElevatorPhysClass::DOOR_STATE_MAX );
	cEncoderList::Set_Precision( BITPACK_ELEVATOR_BOTTOM_DOOR_STATE,	0, (int)ElevatorPhysClass::DOOR_STATE_MAX );
}