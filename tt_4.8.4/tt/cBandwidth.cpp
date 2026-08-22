#include "General.h"
#include "cBandwidth.h"



RENEGADE_FUNCTION
uint cBandwidth::Get_Bandwidth_Bps_From_Type(BANDWIDTH_TYPE_ENUM)
AT2(0x00455CA0, 0x00455DB0);



RENEGADE_FUNCTION
const wchar_t* cBandwidth::Get_Bandwidth_String_From_Type(BANDWIDTH_TYPE_ENUM)
AT2(0x00455D10, 0x00455E20);



RENEGADE_FUNCTION
BANDWIDTH_TYPE_ENUM cBandwidth::Get_Bandwidth_Type_From_String(const char*)
AT2(0x00455F20, 0x00456030);
