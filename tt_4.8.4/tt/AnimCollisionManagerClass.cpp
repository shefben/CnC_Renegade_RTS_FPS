#include "general.h"

#include "AnimCollisionManagerClass.h"



RENEGADE_FUNCTION
void AnimCollisionManagerClass::Set_Animation_Mode
   (AnimModeType type)
   AT2(0x00625210,0x00624AB0);



RENEGADE_FUNCTION
void AnimCollisionManagerClass::Set_Target_Frame
   (float frame)
   AT2(0x006252F0,0x00624B90);



RENEGADE_FUNCTION
void AnimCollisionManagerClass::Set_Target_Frame_End()
   AT2(0x00625310,0x00624BB0);

RENEGADE_FUNCTION
void AnimCollisionManagerClass::Set_Animation
	(const char* animation)
	AT2(0x00625230,0x00624AD0);

RENEGADE_FUNCTION
void AnimCollisionManagerClass::Set_Current_Frame
	(float frame)
	AT2(0x006253D0,0x00624C70);
