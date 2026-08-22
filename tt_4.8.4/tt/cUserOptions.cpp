#include "General.h"
#include "cUserOptions.h"



REF_DEF2(cUserOptions::ShowNamesOnSoldier, bool, 0x0082D760, 0x00000000);
REF_DEF2(cUserOptions::BandwidthBps, int, 0x0082CAC8, 0x0082BCB0);
REF_DEF2(cUserOptions::ResultsLogNumber, cRegistryInt, 0x0082E918, 0x0082DB00);
REF_DEF2(cUserOptions::NetUpdateRate, int, 0x0082D370, 0x0082C558);
REF_DEF2(cUserOptions::ClientHintFactor, float, 0x0082C8D0, 0x0082BAB8);



RENEGADE_FUNCTION
bool cUserOptions::Parse_Command_Line(const char*)
AT2(0x0046F8D0, 0x0046F040);



RENEGADE_FUNCTION
void cUserOptions::Set_Server_INI_File(char*)
AT2(0x0046FFD0, 0x0046F740);



RENEGADE_FUNCTION
void cUserOptions::Set_Bandwidth_Type(BANDWIDTH_TYPE_ENUM)
AT2(0x004700A0, 0x0046F810);



RENEGADE_FUNCTION
BANDWIDTH_TYPE_ENUM cUserOptions::Get_Bandwidth_Type()
AT2(0x00470100, 0x0046F870);



RENEGADE_FUNCTION
void cUserOptions::Set_Bandwidth_Bps(int)
AT2(0x00470120, 0x0046F890);



RENEGADE_FUNCTION
void cUserOptions::Reread()
AT2(0x00470150, 0x0046F8C0);
