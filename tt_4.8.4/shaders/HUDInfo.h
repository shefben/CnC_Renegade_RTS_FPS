#ifndef TT_INCLUDE__HUDINFO_H
#define TT_INCLUDE__HUDINFO_H


#include "engine_string.h"
#include "ReferencerClass.h"
#include "ScriptableGameObj.h"
class DamageableGameObj;
class ReferencerClass;
class Vector3;



class HUDInfo
{

public:

	static REF_DECL1(DisplayActionBar, bool);
	static REF_DECL1(ActionStatusValue, float);
	static REF_DECL1(IsHUDHelpTextDirty, bool);
	static REF_DECL1(IsMCT, bool);
	static REF_DECL1(WeaponTargetObject, ReferencerClass);
	static REF_DECL1(WeaponTargetPosition, Vector3);
	static REF_DECL1(InfoObject, ReferencerClass);
	static REF_DECL1(InfoObjectTimer, float);
	static REF_DECL1(HUDHelpText, WideStringClass);
	static REF_DECL1(HUDHelpTextColor, Vector3);

	static void Set_Info_Object(DamageableGameObj*, bool);
	static void Update_Info_Object();
	static void Set_Weapon_Target_Object(DamageableGameObj*);
	static DamageableGameObj *Get_Info_Object()
	{
		if (InfoObject.Get_Ptr())
		{
			return InfoObject.Get_Ptr()->As_DamageableGameObj();
		}
		else
		{
			return 0;
		}
	}
	static bool Is_HUD_Help_Text_Dirty() {return IsHUDHelpTextDirty;}
	static void Set_Is_HUD_Help_Text_Dirty(bool set) {IsHUDHelpTextDirty = set;}
	static const WideStringClass &Get_HUD_Help_Text() {return HUDHelpText;}
	static Vector3 Get_HUD_Help_Text_Color() {return HUDHelpTextColor;}
	static void Set_HUD_Help_Text(wchar_t *wct) {HUDHelpText = wct;}
	static bool Display_Action_Status_Bar() {return DisplayActionBar;}
	static float Get_Action_Status_Value() {return ActionStatusValue;}
	static DamageableGameObj *Get_Weapon_Target_Object() {if (WeaponTargetObject.Get_Ptr()) { return WeaponTargetObject.Get_Ptr()->As_DamageableGameObj(); } return 0; }
	static Vector3 Get_Weapon_Target_Position() {return WeaponTargetPosition;}
};

#endif
