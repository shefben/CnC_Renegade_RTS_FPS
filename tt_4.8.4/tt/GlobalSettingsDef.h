#ifndef TT_INCLUDE__GLOBALSETTINGSDEF_H
#define TT_INCLUDE__GLOBALSETTINGSDEF_H
#include "Definition.h"
class GlobalSettingsDef : 
	public DefinitionClass 
{
public:
	static REF_DECL2(GlobalSettings, GlobalSettingsDef*);

	int DeathSoundID;								//001C
	int EVAObjectivesSoundID;						//0020
	int HUDHelpTextSoundID;							//0024
	float MaxConversationDist;						//0028
	float MaxCombatConversationDist;				//002C
	float SoldierWalkSpeed;							//0030
	float SoldierCrouchSpeed;						//0034
	float FallingDamageMinDist;						//0038
	float FallingDamageMaxDist;						//003C
	StringClass PurchaseGDICharactersTexture;		//0040
	StringClass PurchaseGDIVehiclesTexture;			//0044
	StringClass PurchaseGDIEquipementTexture;		//0048
	StringClass PurchaseNODCharactersTexture;		//004C
	StringClass PurchaseNODVehiclesTexture;			//0050
	StringClass PurchaseNODEquipementTexture;		//0054
	StringClass PurchaseGDIMUTCharactersTexture;	//0058
	StringClass PurchaseGDIMUTVehiclesTexture;		//005C
	StringClass PurchaseGDIMUTEquipementTexture;	//0060
	StringClass PurchaseNODMUTCharactersTexture;	//0064
	StringClass PurchaseNODMUTVehiclesTexture;		//0068
	StringClass PurchaseNODMUTEquipementTexture;	//006C
	int EncyclopediaEventStringID;					//0070
	int FallingDamageWarhead;						//0074
	float StealthDistanceHuman;						//0078
	float StealthDistanceVehicle;					//007C
	float MPStealthDistanceHuman;					//0080
	float MPStealthDistanceVehicle;					//0084 

};//0088

#endif