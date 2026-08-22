#ifndef TT_INCLUDE__HUMANLOITERGLOBALSETTINGSDEF_H
#define TT_INCLUDE__HUMANLOITERGLOBALSETTINGSDEF_H
#include "Definition.h"
class HumanLoiterGlobalSettingsDef : public DefinitionClass {
public:
	float activationDelay;
	float loiterFrequency;
	DynamicVectorClass<StringClass> loiters;
	HumanLoiterGlobalSettingsDef();
	virtual ~HumanLoiterGlobalSettingsDef();
	virtual uint32 Get_Class_ID();
	virtual bool Load(ChunkLoadClass *load);
	virtual bool Save(ChunkSaveClass *save);
	virtual PersistClass *Create() const;
	virtual PersistFactoryClass *Get_Factory();
 
	float Get_Activation_Delay();
 
	static HumanLoiterGlobalSettingsDef *Get_Default_Loiters();
	static HumanLoiterGlobalSettingsDef *Get_Weaponless_Loiters();
	static HumanLoiterGlobalSettingsDef *Get_Weapon_Loiters();
	const char *Pick_Animation();
};

#endif
