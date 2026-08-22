#include "General.h"
#include "CGameSpyQnR.h"



REF_DEF2(GameSpyQnR, CGameSpyQnR, 0x0082FD40, 0x0082EF28);



RENEGADE_FUNCTION
UNK CGameSpyQnR::LaunchArcade()
AT2(0x004E2E10, 0x004E26B0);



RENEGADE_FUNCTION
UNK CGameSpyQnR::Shutdown()
AT2(0x004E3110, 0x004E29B0);



RENEGADE_FUNCTION
UNK CGameSpyQnR::TrackUsage()
AT2(0x004E3150, 0x004E29F0);



RENEGADE_FUNCTION
void CGameSpyQnR::Init()
AT2(0x004E3210, 0x004E2AB0);



RENEGADE_FUNCTION
UNK CGameSpyQnR::DoGameStuff()
AT2(0x004E3320, 0x004E2BC0);



RENEGADE_FUNCTION
UNK CGameSpyQnR::Think()
AT2(0x004E3330, 0x004E2BD0);



RENEGADE_FUNCTION
UNK CGameSpyQnR::basic_callback(char*, int)
AT2(0x004E3470, 0x004E2D10);



RENEGADE_FUNCTION
UNK CGameSpyQnR::info_callback(char*, int)
AT2(0x004E34F0, 0x004E2D90);



RENEGADE_FUNCTION
UNK CGameSpyQnR::rules_callback(char*, int)
AT2(0x004E3890, 0x004E3130);



RENEGADE_FUNCTION
UNK CGameSpyQnR::Parse_HeartBeat_List(const char*)
AT2(0x004E3AB0, 0x004E3350);



RENEGADE_FUNCTION
UNK CGameSpyQnR::Append_InfoKey_Pair(char*, int, const char*, const char*)
AT2(0x004E3C10, 0x004E34B0);



RENEGADE_FUNCTION
UNK CGameSpyQnR::Append_InfoKey_Pair(char*, int, const char*, const WideStringClass&)
AT2(0x004E3CE0, 0x004E3580);



RENEGADE_FUNCTION
UNK CGameSpyQnR::Append_InfoKey_Pair(char*, int, const char*, const StringClass&)
AT2(0x004E3D70, 0x004E3610);



RENEGADE_FUNCTION
UNK CGameSpyQnR::players_callback(char*, int)
AT2(0x004E3D90, 0x004E3630);
