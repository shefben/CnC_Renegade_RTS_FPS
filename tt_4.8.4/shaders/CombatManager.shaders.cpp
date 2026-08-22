#include "General.h"
#include "CombatManager.h"

#include "PhysicsSceneClass.h"
#include "cMathUtil.h"
#include "TimeManager.h"
#include "GameCombatNetworkHandlerClass.h"
#include "ObjectiveManager.h"
#include "BulletManager.h"
#include "ConversationMgrClass.h"
#include "GameObjManager.h"
#include "CCameraClass.h"
#include "WW3D.h"
#include "HUDClass.h"
#include "MessageWindowClass.h"
#include "SpawnManager.h"
#include "SoundEnvironmentClass.h"
#include "BackgroundMgrClass.h"
#include "WeatherMgrClass.h"
#include "WeaponViewClass.h"
#include "ScreenFadeManager.h"
#include "CameraClass.h"
#include "DazzleLayerClass.h"
#include "DazzleRenderObjClass.h"
#include "CombatDazzleClass.h"
#include "TexProjectClass.h"
#include "SurfaceEffectsManager.h"
#include "WWAudioClass.h"
#include "ArmorWarheadManager.h"
#include "SimpleSceneClass.h"
#include "cGameType.h"
#include "ScriptManager.h"
#ifdef SHADERS_EXPORTS
#include "TextureController.h"
#include "shader.h"
#endif
REF_DEF2(CombatManager::IAmClient, bool, 0x00855EC9, 0x008550B1);
REF_DEF2(CombatManager::IAmServer, bool, 0x00855EC8, 0x008550B0);
REF_DEF2(CombatManager::IsStarDeterminingTarget, bool, 0x0080F454, 0x0080E62C);
RENEGADE_FUNCTION
void ArmorWarheadManager::Init()
AT1(0x006862E0);

class BonesManager {
public:
	static void Init();
};

RENEGADE_FUNCTION
void BonesManager::Init()
AT1(0x006AA1B0);

class CombatSoundManager {
public:
	static void Init();
};

RENEGADE_FUNCTION
void CombatSoundManager::Init()
AT1(0x007107F0);

RENEGADE_FUNCTION
void SurfaceEffectsManager::Init()
AT1(0x006A9080);

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
REF_DEF2(CombatManager::StarDamageDirection, int, 0x00855EE4, 0x008550CC);
REF_DEF2(CombatManager::AreObserversActive, bool, 0x0080F45D, 0x0080E635);

RENEGADE_FUNCTION
void CombatManager::Handle_Input()
AT2(0x00670B00, 0x006703A0);



RENEGADE_FUNCTION
void CombatManager::Update_Star()
AT2(0x00671440, 0x00670CE0);


RENEGADE_FUNCTION
void CombatManager::Update_Star_Targeting()
AT2(0x006715A0, 0x00670E40);



RENEGADE_FUNCTION
void CombatManager::Do_Skeleton_Slider_Demo()
AT2(0x00671620, 0x00670F40);


#ifdef SHADERS_EXPORTS
PhysicsSceneClass* CombatManager::Get_Scene()
{
	return PhysicsSceneClass::Get_Instance();
}

void CombatManager::Think()
{
	SyncTime += cMathUtil::Round(1000.f * TimeManager::FrameSeconds);
	IsGameplayPermitted = NetworkHandler->Is_Gameplay_Permitted();
	Handle_Input();
	
	BulletManager::Update();
	ObjectiveManager::Update(TimeManager::FrameSeconds);
	ConversationMgrClass::Think();
	GameObjManager::Think();
	Get_Scene()->Update(TimeManager::FrameSeconds, 0);
	Update_Star();
	if (!MainCamera->Is_Using_Host_Model())
		MainCamera->Update();

	GameObjManager::Post_Think();
	
	if (MainCamera->Is_Using_Host_Model())
		MainCamera->Update();
	
	Update_Star_Targeting();
	//Do_Skeleton_Slider_Demo();
	MessageWindow->On_Frame_Update();
	SpawnManager::Update();
	
	if (SoundEnvironment)
		SoundEnvironment->Update(Get_Scene(), MainCamera);
	
	if (!Exe)
	{
		HUDClass::Think(); //HUDClass shouldnt attempt to render on FDS
	}
	WeaponViewClass::Think();
	ScreenFadeManager::Think();
}

bool AllowFogOverride = false;
void CombatManager::Render()
{
	AllowFogOverride = true;
	if (TheStar)
		MultiplayRenderingAllowed = true;
	
	if (CombatManager::MultiplayRenderingAllowed)
	{
		// SystemInfoLog::Record_Frame();

		BackgroundMgrClass::Update(Get_Scene(), MainCamera);
		WeatherMgrClass::Update(Get_Scene(), MainCamera);
		
		Get_Scene()->Apply_Camera_Shakes(*MainCamera);
		DazzleRenderObjClass::Install_Dazzle_Visibility_Handler(&_TheCombatDazzleHandler);
		
		Vector3 vector = Vector3(0, 0, 0);
		WW3D::Render(BackgroundScene, MainCamera, false, false, vector);
		WW3D::Render(Get_Scene(), MainCamera, false, false, vector);
		
		if (DazzleLayer)
			DazzleLayer->Render(MainCamera);
		
		DazzleRenderObjClass::Install_Dazzle_Visibility_Handler(NULL);
		if (!Exe)
		{
			HUDClass::Render(); //HUDClass shouldnt attempt to render on FDS
		}
		ScreenFadeManager::Render();
	}
	AllowFogOverride = false;
}

bool CombatManager::Is_Gameplay_Permitted()
{
	if (NetworkHandler)
		return IsGameplayPermitted;
	else
		return true;
}

void CombatManager::Init(bool render_available)
{
	IsGameplayPermitted = false;
	MessageWindowClass* messageWindow = new MessageWindowClass();
	MessageWindow = messageWindow;
	messageWindow->Initialize();
	ScriptManager::Init();
	BonesManager::Init();
	ArmorWarheadManager::Init();
	CCameraClass::Init();
	SurfaceEffectsManager::Init();
	ObjectiveManager::Init();
	CombatSoundManager::Init();
	MainCamera = new CCameraClass();
	if (render_available)
	{
		DazzleLayerClass* dazzleLayer = new DazzleLayerClass();
		DazzleLayer = dazzleLayer;
		DazzleRenderObjClass::Set_Current_Dazzle_Layer(dazzleLayer);
	}
	else
		DazzleRenderObjClass::Set_Current_Dazzle_Layer(NULL);
	
	if (WWAudioClass::_theInstance->soundScene)
		WWAudioClass::_theInstance->soundScene->Attach_Listener_To_Obj(MainCamera, -1);
	
	HUDClass::Init(render_available);
	ScreenFadeManager::Init();
	FirstPerson = FirstPersonDefault;
}
#endif

RENEGADE_FUNCTION
void SpawnManager::Update()
AT2(0x006A2AC0, 0x006A2360);

RENEGADE_FUNCTION
void MessageWindowClass::On_Frame_Update()
AT2(0x006A7500, 0x006A6DA0);

RENEGADE_FUNCTION
void GameObjManager::Think()
AT2(0x00680000, 0x0067F8A0);

RENEGADE_FUNCTION
void GameObjManager::Post_Think()
AT2(0x006800D0, 0x0067F970);
