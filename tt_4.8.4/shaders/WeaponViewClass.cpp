#include "General.h"
#include "WeaponViewClass.h"



RENEGADE_FUNCTION
void WeaponViewClass::Think()
AT2(0x0070E460, 0x0070DA20);

RENEGADE_FUNCTION
Vector3 WeaponViewClass::Get_Muzzle_Pos()
AT2(0x0070F500, 0x0070EAC0);

REF_DEF2(WeaponViewClass::WeaponViewEnabled,bool,0x0085E574,0x0085D74C);
REF_DEF2(WeaponViewClass::HandsPhysObj,DecorationPhysClass*,0x0085E5E0,0x0085D7B8);
