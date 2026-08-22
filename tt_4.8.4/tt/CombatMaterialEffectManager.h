#ifndef TT_INCLUDE_COMBATMATERIALEFFECTMANAGER_H
#define TT_INCLUDE_COMBATMATERIALEFFECTMANAGER_H
#include "TransitionEffectClass.h"
class CombatMaterialEffectManager
{
public:
	static TransitionEffectClass *Get_Spawn_Effect();
	static TransitionEffectClass *Get_Death_Effect();
	static TransitionEffectClass *Get_Health_Effect();
	static TransitionEffectClass *Get_Electrocution_Effect();
};

#endif