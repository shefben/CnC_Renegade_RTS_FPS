#include "General.h"
#include "DoorPhysClass.h"
#include "EncoderList.h"
#include "bitpackids.h"

void DoorPhysClass::Set_Precision()
{
	cEncoderList::Set_Precision(BITPACK_DOOR_STATE,0.0f,5.0f,1.0f);
}
