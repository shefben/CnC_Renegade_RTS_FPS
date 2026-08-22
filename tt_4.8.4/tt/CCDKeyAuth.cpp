#include "General.h"
#include "CCDKeyAuth.h"
#include "RegistryClass.h"


RENEGADE_FUNCTION
void CCDKeyAuth::DisconnectUser(int clientId)
AT2(0x004E2910, 0x004E21B0);

void CCDKeyAuth::GetSerialHash(StringClass &serial)
{
	RegistryClass registry(Build_Registry_Location_String((char *)0x007F5274,0,""),true);
	registry.Get_String("Serial",serial,"0000000000000000000000");
}
