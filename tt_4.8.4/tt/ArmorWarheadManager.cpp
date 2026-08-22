#include "General.h"
#include "ArmorWarheadManager.h"



RENEGADE_FUNCTION
void ArmorWarheadManager::Init()
AT2(0x00687040, 0x006868E0);



RENEGADE_FUNCTION
void ArmorWarheadManager::Shutdown()
AT2(0x00687570, 0x00686E10);



RENEGADE_FUNCTION
UNK ArmorWarheadManager::Get_Num_Armor_Types()
AT2(0x00687730, 0x00686FD0);



RENEGADE_FUNCTION
UNK ArmorWarheadManager::Get_Num_Warhead_Types()
AT2(0x00687740, 0x00686FE0);

RENEGADE_FUNCTION
uint ArmorWarheadManager::Get_Warhead_Type(const char*)
AT2(0x00687790, 0x00687030);

RENEGADE_FUNCTION
UNK ArmorWarheadManager::Get_Warhead_Name(uint)
AT2(0x006877E0, 0x00687080);



RENEGADE_FUNCTION
float ArmorWarheadManager::Get_Damage_Multiplier(uint, uint)
AT2(0x006877F0, 0x00687090);



RENEGADE_FUNCTION
float ArmorWarheadManager::Get_Shield_Absorbsion(uint, uint)
AT2(0x00687AE0, 0x00687380);



RENEGADE_FUNCTION
bool ArmorWarheadManager::Is_Armor_Soft(uint)
AT2(0x00687DD0, 0x00687670);



RENEGADE_FUNCTION
int ArmorWarheadManager::Get_Armor_Save_ID(uint)
AT2(0x00687DE0, 0x00687680);



RENEGADE_FUNCTION
int ArmorWarheadManager::Find_Armor_Save_ID(int)
AT2(0x00687DF0, 0x00687690);



RENEGADE_FUNCTION
int ArmorWarheadManager::Get_Warhead_Save_ID(uint)
AT2(0x00687E20, 0x006876C0);



RENEGADE_FUNCTION
int ArmorWarheadManager::Find_Warhead_Save_ID(int)
AT2(0x00687E30, 0x006876D0);



RENEGADE_FUNCTION
ArmorWarheadManager::SpecialDamageType ArmorWarheadManager::Get_Special_Damage_Type(uint)
AT2(0x00687E60, 0x00687700);



RENEGADE_FUNCTION
float ArmorWarheadManager::Get_Special_Damage_Probability(uint)
AT2(0x00687E70, 0x00687710);



RENEGADE_FUNCTION
unsigned int ArmorWarheadManager::Get_Special_Damage_Warhead(SpecialDamageType)
AT2(0x00687E80, 0x00687720);



RENEGADE_FUNCTION
UNK ArmorWarheadManager::Get_Special_Damage_Duration(SpecialDamageType)
AT2(0x00687E90, 0x00687730);



RENEGADE_FUNCTION
float ArmorWarheadManager::Get_Special_Damage_Scale(SpecialDamageType)
AT2(0x00687EA0, 0x00687740);



RENEGADE_FUNCTION
const char *ArmorWarheadManager::Get_Special_Damage_Explosion(SpecialDamageType)
AT2(0x00687EB0, 0x00687750);



RENEGADE_FUNCTION
float ArmorWarheadManager::Get_Visceroid_Probability(uint)
AT2(0x00687EC0, 0x00687760);



RENEGADE_FUNCTION
bool ArmorWarheadManager::Is_Skin_Impervious(ArmorWarheadManager::SpecialDamageType, uint)
AT2(0x00687ED0, 0x00687770);
