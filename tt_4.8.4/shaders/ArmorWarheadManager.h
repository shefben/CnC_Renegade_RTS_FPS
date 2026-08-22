#ifndef TT_INCLUDE__ARMORWARHEADMANAGER_H
#define TT_INCLUDE__ARMORWARHEADMANAGER_H
#include "engine_string.h"

class ArmorWarheadManager
{

public:

	enum SpecialDamageType
	{
		SPECIAL_DAMAGE_TYPE_NONE,
		SPECIAL_DAMAGE_TYPE_FIRE,
		SPECIAL_DAMAGE_TYPE_CHEM,
		SPECIAL_DAMAGE_TYPE_ELECTRIC,
		SPECIAL_DAMAGE_TYPE_CNC_FIRE,
		SPECIAL_DAMAGE_TYPE_CNC_CHEM,
		SPECIAL_DAMAGE_TYPE_SUPER_FIRE,
		NUM_SPECIAL_DAMAGE_TYPES,
	};


	static REF_DECL2(Multipliers, UNK);
	static REF_DECL2(Absorbsion, UNK);


	static void Init();
	static void Shutdown();
	static uint Get_Num_Armor_Types();
	static uint Get_Num_Warhead_Types();
	static uint Get_Armor_Type(const char*);
	static uint Get_Warhead_Type(const char*);
	static const char *Get_Armor_Name(uint);
	static UNK Get_Warhead_Name(uint);
	static float Get_Damage_Multiplier(uint, uint);
	static float Get_Shield_Absorbsion(uint, uint);
	bool Is_Armor_Soft(uint);
	static int Get_Armor_Save_ID(uint);
	static int Find_Armor_Save_ID(int);
	static int Get_Warhead_Save_ID(uint);
	static int Find_Warhead_Save_ID(int);
	static SpecialDamageType Get_Special_Damage_Type(uint);
	static float Get_Special_Damage_Probability(uint);
	static unsigned int Get_Special_Damage_Warhead(SpecialDamageType);
	static UNK Get_Special_Damage_Duration(SpecialDamageType);
	static float Get_Special_Damage_Scale(SpecialDamageType);
	static const char *Get_Special_Damage_Explosion(SpecialDamageType);
	static float Get_Visceroid_Probability(uint);
	static bool Is_Skin_Impervious(SpecialDamageType, uint);

};


#endif
