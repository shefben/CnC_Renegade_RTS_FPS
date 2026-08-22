#include "General.h"
#include "CombatManager.h"
#include "GameCombatNetworkHandlerClass.h"
#include "WWAudioClass.h"

#include "PhysicsSceneClass.h"
#include "TexProjectClass.h"
#include "SoldierGameObj.h"
#include "CCameraClass.h"
#include "MessageWindowClass.h"
#include "ArmorWarheadManager.h"
#include "ObjectiveManager.h"
#include "DazzleLayerClass.h"
#include "DazzleRenderObjClass.h"
#include "HUDClass.h"
#include "ScreenFadeManager.h"
#include "PhysClass.h"
#include "GameObjManager.h"

REF_DEF2(CombatManager::DifficultyLevel, sint32, 0x0080F458, 0x0080E630);
REF_DEF2(CombatManager::FriendlyFirePermitted, bool, 0x00855EFC, 0x008550E4);
REF_DEF2(CombatManager::MainCamera, CCameraClass*, 0x00855EB8, 0x008550A0);
REF_DEF2(CombatManager::FirstPerson, bool, 0x0080F45E, 0x00000000);
REF_DEF2(CombatManager::FirstPersonDefault, bool, 0x0080F45F, 0x00000000);
REF_DEF2(CombatManager::MessageWindow, MessageWindowClass*, 0x00855EDC, 0x008550C4);
REF_DEF2(CombatManager::BackgroundScene, SimpleSceneClass*, 0x00855EBC, 0x008550A4);
REF_DEF2(CombatManager::SoundEnvironment, SoundEnvironmentClass*, 0x00855EC0, 0x008550A8);
REF_DEF2(CombatManager::IsGameplayPermitted, bool, 0x00855EFE, 0x008550E6);
REF_DEF2(CombatManager::NetworkHandler, CombatNetworkHandlerClass*, 0x00855ED4, 0x008550BC);
REF_DEF2(CombatManager::SyncTime, uint, 0x00855ED0, 0x008550B8);
REF_DEF2(CombatManager::DazzleLayer, DazzleLayerClass*, 0x00855EC4, 0x008550AC);
REF_DEF2(CombatManager::MultiplayRenderingAllowed, bool, 0x0080F461, 0x0080E639);
REF_DEF2(CombatManager::TheStar, ReferencerClass, 0x00855E40, 0x00855028);
REF_DEF2(CombatManager::_TheCombatDazzleHandler, CombatDazzleClass, 0x0085F330, 0x0085E508);
REF_DEF2(CombatManager::HitReticleEnabled, bool, 0x0080F460, 0x0080E638);
REF_DEF2(CombatManager::BeaconPlacementEndsGame, bool, 0x00855EFD, 0x008550E5);

REF_DEF2(CombatManager::IAmClient, bool, 0x00855EC9, 0x008550B1);
REF_DEF2(CombatManager::IAmServer, bool, 0x00855EC8, 0x008550B0);
REF_DEF2(CombatManager::MyId, int, 0x00855ECC, 0x008550B4);
REF_DEF2(CombatManager::LastLSDName, StringClass, 0x00855EA8, 0x00855090);
REF_DEF2(CombatManager::LoadProgress, int, 0x00855EB4, 0x0085509C);
REF_DEF2(CombatManager::MiscHandler, CombatMiscHandlerClass*, 0x00855ED8, 0x008550C0);
REF_DEF2(CombatManager::StarDamageDirection, int, 0x00855EE4, 0x008550CC);
REF_DEF2(CombatManager::AutosaveRequested, bool, 0x00855EF2, 0x008550DA);

RENEGADE_FUNCTION
void CombatManager::Shutdown()
AT2(0x00670170, 0x0066FA10);



RENEGADE_FUNCTION
void CombatManager::Scene_Init()
AT2(0x00670260, 0x0066FB00);



void CombatManager::postSceneInit()
{
	PhysicsSceneClass::Get_Instance()->Disable_Collision_Detection(SOLDIER_GHOST_COLLISION_GROUP, DEFAULT_COLLISION_GROUP);
}



RENEGADE_FUNCTION
void CombatManager::Pre_Load_Level(bool)
AT2(0x00670400, 0x0066FCA0);



RENEGADE_FUNCTION
bool CombatManager::Load_Level_Threaded(const char*, bool)
AT2(0x006707A0, 0x00670040);



RENEGADE_FUNCTION
bool CombatManager::Is_Load_Level_Complete()
AT2(0x00670820, 0x006700C0);



RENEGADE_FUNCTION
bool CombatManager::Is_Loading_Level()
AT2(0x00670840, 0x006700E0);



RENEGADE_FUNCTION
void CombatManager::Post_Load_Level()
AT2(0x00670850, 0x006700F0);



RENEGADE_FUNCTION
void CombatManager::Unload_Level()
AT2(0x006708B0, 0x00670150);



PhysicsSceneClass* CombatManager::Get_Scene()
{
	return PhysicsSceneClass::Get_Instance();
}


#ifndef TTINIT_EXPORTS // Hack to avoid linker errors
void CombatManager::Generate_Control()
{
	GameObjManager::Generate_Control();
}
#endif



RENEGADE_FUNCTION
void CombatManager::Handle_Input()
AT2(0x00670B00, 0x006703A0);



RENEGADE_FUNCTION
bool CombatManager::Save(ChunkSaveClass&)
AT2(0x00670F10, 0x006707B0);



RENEGADE_FUNCTION
bool CombatManager::Load(ChunkLoadClass&)
AT2(0x006710F0, 0x00670990);



RENEGADE_FUNCTION
bool CombatManager::Can_Damage(ArmedGameObj*, PhysicalGameObj*)
AT2(0x006712D0, 0x00670B70);



RENEGADE_FUNCTION
float CombatManager::Get_Damage_Factor(ArmedGameObj*, PhysicalGameObj*)
AT2(0x006712F0, 0x00670B90);



void CombatManager::On_Soldier_Kill(SoldierGameObj *attacker, SoldierGameObj *victim)
{
	if (NetworkHandler)
		NetworkHandler->On_Soldier_Kill(attacker, victim);
}



RENEGADE_FUNCTION
void CombatManager::On_Soldier_Death(SoldierGameObj*)
AT2(0x00671340, 0x00670BE0);



bool CombatManager::Is_Gameplay_Permitted()
{
	if (NetworkHandler)
		return IsGameplayPermitted;
	else
		return true;
}



RENEGADE_FUNCTION
void CombatManager::Mission_Complete(bool)
AT2(0x00671380, 0x00670C20);



RENEGADE_FUNCTION
void CombatManager::Star_Killed()
AT2(0x006713A0, 0x00670C40);



RENEGADE_FUNCTION
void CombatManager::Set_Camera_Profile(const char*)
AT2(0x006713B0, 0x00670C50);



RENEGADE_FUNCTION
void CombatManager::Set_The_Star(SoldierGameObj*, bool)
AT2(0x006713D0, 0x00670C70);



RENEGADE_FUNCTION
void CombatManager::Update_Star()
AT2(0x00671440, 0x00670CE0);



RENEGADE_FUNCTION
void CombatManager::Update_Star_Targeting()
AT2(0x006715A0, 0x00670E40);



RENEGADE_FUNCTION
bool CombatManager::Is_In_Camera_Frustrum(Vector3&)
AT2(0x006715F0, 0x00670EC0);



RENEGADE_FUNCTION
void CombatManager::Do_Skeleton_Slider_Demo()
AT2(0x00671620, 0x00670F40);



RENEGADE_FUNCTION
void CombatManager::Set_Combat_Mode(int)
AT2(0x00671780, 0x00671020);



RENEGADE_FUNCTION
void CombatManager::Update_Combat_Mode()
AT2(0x006719B0, 0x00671250);



RENEGADE_FUNCTION
void CombatManager::Register_Star_Killer(ArmedGameObj*)
AT2(0x00671D40, 0x006715E0);
