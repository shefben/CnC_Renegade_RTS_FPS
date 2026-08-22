#ifndef TT_INCLUDE__HUDGLOBALSETTINGSDEF_H
#define TT_INCLUDE__HUDGLOBALSETTINGSDEF_H



#include "Vector3.h"
#include "Definition.h"

class ChunkSaveClass;
class ChunkLoadClass;
class PersistFactoryClass;
class PersistClass;

class HUDGlobalSettingsDef : public DefinitionClass
{
	
private:

	static REF_DECL1(Instance, HUDGlobalSettingsDef*);

public:
	Vector3 NodColor;
	Vector3 GDIColor;
	Vector3 NeutralColor;
	Vector3 MutantColor;
	Vector3 RenegadeColor;
	Vector3 PrimaryObjectiveColor;
	Vector3 SecondaryObjectiveColor;
	Vector3 TertiaryObjectiveColor;
	Vector3 HealthHighColor;
	Vector3 HealthMedColor;
	Vector3 HealthLowColor;
	Vector3 EnemyColor;
	Vector3 FriendlyColor;
	Vector3 NoRelationColor;
	HUDGlobalSettingsDef();
	~HUDGlobalSettingsDef();
	virtual uint32 Get_Class_ID() const;
	virtual const PersistFactoryClass &Get_Factory             () const;
	virtual PersistClass* Create() const;
	bool Save(ChunkSaveClass&);
	bool Load(ChunkLoadClass&);
	static HUDGlobalSettingsDef* Get_Instance() { return Instance; }
	Vector3 Get_Nod_Color() {return NodColor;}
	Vector3 Get_GDI_Color() {return GDIColor;}
	Vector3 Get_Neutral_Color() {return NeutralColor;}
	Vector3 Get_Mutant_Color() {return MutantColor;}
	Vector3 Get_Renegade_Color() {return RenegadeColor;}
	Vector3 Get_Primary_Objective_Color() {return PrimaryObjectiveColor;}
	Vector3 Get_Secondary_Objective_Color() {return SecondaryObjectiveColor;}
	Vector3 Get_Tertiary_Objective_Color() {return TertiaryObjectiveColor;}
	Vector3 Get_Health_High_Color() {return HealthHighColor;}
	Vector3 Get_Health_Med_Color() {return HealthMedColor;}
	Vector3 Get_Health_Low_Color() {return HealthLowColor;}
	Vector3 Get_Enemy_Color() {return EnemyColor;}
	Vector3 Get_Friendly_Color() {return FriendlyColor;}
	Vector3 Get_No_Relation_Color() {return NoRelationColor;}

}; // 0438



#endif