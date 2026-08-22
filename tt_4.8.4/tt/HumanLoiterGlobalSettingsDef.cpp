#include "General.h"
#include "HumanLoiterGlobalSettingsDef.h"

RENEGADE_FUNCTION
HumanLoiterGlobalSettingsDef* HumanLoiterGlobalSettingsDef::Get_Default_Loiters()
AT2(0x006F5BF0, 0x006F51B0);



RENEGADE_FUNCTION
HumanLoiterGlobalSettingsDef* HumanLoiterGlobalSettingsDef::Get_Weapon_Loiters()
AT2(0x006F5C20, 0x006F51E0);



RENEGADE_FUNCTION
HumanLoiterGlobalSettingsDef* HumanLoiterGlobalSettingsDef::Get_Weaponless_Loiters()
AT2(0x006F5C50, 0x006F5210);

RENEGADE_FUNCTION
const char* HumanLoiterGlobalSettingsDef::Pick_Animation()
AT2(0x006F5C80, 0x006F5240);
