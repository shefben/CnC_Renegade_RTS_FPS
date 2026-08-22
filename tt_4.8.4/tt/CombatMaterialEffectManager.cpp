#include "General.h"
#include "CombatMaterialEffectManager.h"
#include "WW3DAssetManager.h"

TransitionEffectClass *CombatMaterialEffectManager::Get_Spawn_Effect()
{
	TransitionEffectClass *effect = new TransitionEffectClass();
	effect->Set_Parameter(1);
	effect->Set_Target_Parameter(0);
	effect->Set_Transition_Time(2);
	effect->Enable_Remove_On_Complete(true);
	effect->Set_Max_Intensity(0.25);
	TextureClass *texture = WW3DAssetManager::TheInstance->Get_Texture("REN_spawn.tga",TextureClass::MIP_LEVELS_1,WW3D_FORMAT_UNKNOWN,true);
	effect->Set_Texture(texture);
	texture->Release_Ref();
	return effect;
}

TransitionEffectClass *CombatMaterialEffectManager::Get_Death_Effect()
{
	TransitionEffectClass *effect = new TransitionEffectClass();
	effect->Set_Parameter(0);
	effect->Set_Target_Parameter(1);
	effect->Set_Start_Delay(1.5);
	effect->Set_Transition_Time(0.5);
	effect->Set_Max_Intensity(0.5);
	effect->Set_Max_UV_Velocity(Vector2(3.75,-6.0));
	TextureClass *texture = WW3DAssetManager::TheInstance->Get_Texture("REN_death.tga",TextureClass::MIP_LEVELS_1,WW3D_FORMAT_UNKNOWN,true);
	effect->Set_Texture(texture);
	texture->Release_Ref();
	return effect;
}

TransitionEffectClass *CombatMaterialEffectManager::Get_Health_Effect()
{
	TransitionEffectClass *effect = new TransitionEffectClass();
	effect->Set_Parameter(0);
	effect->Set_Target_Parameter(0.49000001f);
	effect->Set_Transition_Time(1);
	effect->Set_Max_Intensity(0.5);
	effect->Set_Min_UV_Velocity(Vector2(0,-3.0));
	effect->Set_Max_UV_Velocity(Vector2(0,-3.0));
	TextureClass *texture = WW3DAssetManager::TheInstance->Get_Texture("REN_repair.tga",TextureClass::MIP_LEVELS_1,WW3D_FORMAT_UNKNOWN,true);
	effect->Set_Texture(texture);
	texture->Release_Ref();
	return effect;
}

TransitionEffectClass *CombatMaterialEffectManager::Get_Electrocution_Effect()
{
	TransitionEffectClass *effect = new TransitionEffectClass();
	effect->Set_Parameter(0);
	effect->Set_Target_Parameter(0.49000001f);
	effect->Set_Transition_Time(1);
	effect->Set_Max_Intensity(0.5);
	effect->Set_Min_UV_Velocity(Vector2(0,-3.0));
	effect->Set_Max_UV_Velocity(Vector2(0,-3.0));
	TextureClass *texture = WW3DAssetManager::TheInstance->Get_Texture("REN_shock.tga",TextureClass::MIP_LEVELS_1,WW3D_FORMAT_UNKNOWN,true);
	effect->Set_Texture(texture);
	texture->Release_Ref();
	return effect;
}
