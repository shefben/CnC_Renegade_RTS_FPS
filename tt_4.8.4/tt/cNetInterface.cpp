#include "General.h"
#include "cNetInterface.h"



#include "engine_string.h"



REF_DEF2(cNetInterface::mSidePreference, int, 0x007F932C, 0x007F83AC);
REF_DEF2(cNetInterface::Nickname, WideStringClass, 0x0081F360, 0x0081E548);



RENEGADE_FUNCTION
WideStringClass cNetInterface::Get_Nickname()
AT2(0x00451710, 0x00451790);



RENEGADE_FUNCTION
void cNetInterface::Set_Nickname(WideStringClass&)
AT2(0x00451900, 0x00451980);



RENEGADE_FUNCTION
void cNetInterface::Set_Random_Nickname()
AT2(0x00451A80, 0x00451B00);
