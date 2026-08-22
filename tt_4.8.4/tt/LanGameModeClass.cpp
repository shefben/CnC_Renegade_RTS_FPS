#include "General.h"
#include "LanGameModeClass.h"



REF_DEF2(LanGameModeClass::PLanChat, cLanChat*, 0x0081DED4, 0x0081D0B8);



RENEGADE_FUNCTION
void LanGameModeClass::Init()
AT2(0x00429920, 0x00429AA0);



RENEGADE_FUNCTION
void LanGameModeClass::Shutdown()
AT2(0x00429950, 0x00429AD0);



RENEGADE_FUNCTION
void LanGameModeClass::Think()
AT2(0x00429980, 0x00429B00);



cLanChat* LanGameModeClass::Get_Lan_Interface()
{
	return LanGameModeClass::PLanChat;
}