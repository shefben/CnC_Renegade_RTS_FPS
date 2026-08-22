#include "General.h"
#include "HUDInfo.h"
#include "CombatManager.h"
#include "SoldierGameObj.h"

REF_DEF1(HUDInfo::DisplayActionBar, bool, 0x00857348);
REF_DEF1(HUDInfo::ActionStatusValue, float, 0x0085734C);
REF_DEF1(HUDInfo::IsHUDHelpTextDirty, bool, 0x00811FBC);
REF_DEF1(HUDInfo::IsMCT, bool, 0x00857350);
REF_DEF1(HUDInfo::WeaponTargetObject, ReferencerClass, 0x00857338);
REF_DEF1(HUDInfo::WeaponTargetPosition, Vector3, 0x00857328);
REF_DEF1(HUDInfo::InfoObject, ReferencerClass, 0x00857308);
REF_DEF1(HUDInfo::InfoObjectTimer, float, 0x00857324);
REF_DEF1(HUDInfo::HUDHelpText, WideStringClass, 0x00857334);
REF_DEF1(HUDInfo::HUDHelpTextColor, Vector3, 0x00857318);

RENEGADE_FUNCTION
void HUDInfo::Update_Info_Object()
AT1(0x006A80B0);

RENEGADE_FUNCTION
void HUDInfo::Set_Info_Object(DamageableGameObj *, bool)
AT1(0x006A8030);

RENEGADE_FUNCTION
void HUDInfo::Set_Weapon_Target_Object(DamageableGameObj *)
AT1(0x006A81C0);
