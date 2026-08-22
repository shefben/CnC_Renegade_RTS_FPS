#ifndef TT_INCLUDE__COMBATMANAGER_H
#define TT_INCLUDE__COMBATMANAGER_H
#include "engine_string.h"
#include "ReferencerClass.h"
#include "VehicleGameObj.h"

#define	COMBAT_CAMERA CombatManager::Get_Camera()
#define	COMBAT_STAR CombatManager::Get_The_Star()
#define	COMBAT_SCENE CombatManager::Get_Scene()
#define	COMBAT_DAZZLE_LAYER CombatManager::Get_Dazzle_Layer()


class	CombatNetworkHandlerClass {
public:
   virtual	bool	Can_Damage(ArmedGameObj * p_armed_damager, PhysicalGameObj * p_phys_victim) = 0;
   virtual	float	Get_Damage_Factor(ArmedGameObj * p_armed_damager, PhysicalGameObj * p_phys_victim) = 0;
	virtual	void	On_Soldier_Kill(SoldierGameObj * p_soldier, SoldierGameObj * p_victim)						= 0;
	virtual	void	On_Soldier_Death(SoldierGameObj * p_soldier)						= 0;
	virtual	bool	Is_Gameplay_Permitted(void)											= 0;
};

class	CombatMiscHandlerClass {
public:
	virtual	void	Mission_Complete( bool success )												= 0;
	virtual	void	Star_Killed( void )																= 0;
};

class SimpleSceneClass;
class SoldierGameObj;
class CCameraClass;
class ReferencerClass;
class GameCombatNetworkHandlerClass;
class ArmedGameObj;
class PhysicalGameObj;
class ChunkSaveClass;
class ChunkLoadClass;
class Vector3;
class MessageWindowClass;
class SoundEnvironmentClass;
class PhysicsSceneClass;
class SceneClass;
class DazzleLayerClass;
class CombatDazzleClass;
class CombatMiscHandlerClass;


class CombatManager
{

public:
	SHADERS_API static void Init(bool render_available);
	static void Shutdown();
	static void Scene_Init();
	static void Pre_Load_Level(bool);
	static bool Load_Level_Threaded(char const*, bool);
	static bool Is_Load_Level_Complete();
	static bool Is_Loading_Level();
	static void Post_Load_Level();
	static void Unload_Level();
	static void Generate_Control();
	static void SHADERS_API Think();
	static void SHADERS_API Render();
	static void Handle_Input();
	static bool Save(ChunkSaveClass&);
	static bool Load(ChunkLoadClass&);
	static void Set_I_Am_Server(bool yes) { IAmServer = yes; }
	static void Set_I_Am_Client(bool yes) { IAmClient = yes; }
	static bool I_Am_Server() { return IAmServer; }
	static bool I_Am_Client() { return IAmClient; }
	static bool I_Am_Only_Client() { return IAmClient && !IAmServer; }
	static bool I_Am_Only_Server() { return IAmServer && !IAmClient; }
	static bool I_Am_Client_Server() { return IAmClient && IAmServer;}
	static void Set_Friendly_Fire_Permitted( bool yes ) { FriendlyFirePermitted = yes; }
	static bool Is_Friendly_Fire_Permitted( void ) { return FriendlyFirePermitted; }
	static void Set_Beacon_Placement_Ends_Game( bool yes ){ BeaconPlacementEndsGame = yes; }
	static bool Does_Beacon_Placement_Ends_Game( void )	{ return BeaconPlacementEndsGame; }
	static void Set_My_Id( int id ) { MyId = id; }
	static int Get_My_Id( void ) { return MyId; }
	static void Set_Last_Round_Trip_Ping_Ms( DWORD ping ) { LastRoundTripPingMs = ping; }
	static DWORD Get_Last_Round_Trip_Ping_Ms( void ) { return LastRoundTripPingMs; }
	static void Set_Avg_Round_Trip_Ping_Ms( DWORD ping ) { AvgRoundTripPingMs = ping; }
	static DWORD Get_Avg_Round_Trip_Ping_Ms( void ) { return AvgRoundTripPingMs; }
	static void Set_Combat_Network_Handler( CombatNetworkHandlerClass * handler )	{ NetworkHandler = handler; }
	static bool Can_Damage(ArmedGameObj*, PhysicalGameObj*);
	static float Get_Damage_Factor(ArmedGameObj*, PhysicalGameObj*);
	static void On_Soldier_Kill(SoldierGameObj *attacker, SoldierGameObj *victim);
	static void On_Soldier_Death(SoldierGameObj*);
	static bool Is_Gameplay_Permitted();
	static void Set_Combat_Misc_Handler( CombatMiscHandlerClass * handler ) { MiscHandler = handler; }
	static void Mission_Complete(bool);
	static void Star_Killed();
	static void Set_The_Star(SoldierGameObj*, bool);
	static SoldierGameObj *Get_The_Star( void ) { return (SoldierGameObj *)TheStar.Get_Ptr(); }
	static void Update_Star();
	static void Update_Star_Targeting();
	static bool Is_Star_Determining_Target(void) {return IsStarDeterminingTarget;}
	static void Set_Is_Star_Determining_Target(bool flag) {IsStarDeterminingTarget = flag;}
	static PhysicsSceneClass* Get_Scene();
	static SimpleSceneClass* Get_Background_Scene( void ) { return BackgroundScene; }
	static CCameraClass* Get_Camera( void ) { return MainCamera; }
	static SoundEnvironmentClass* Get_Sound_Environment( void ) { return SoundEnvironment;	}
	static void Set_Camera_Profile(char const*);
	static	void	Set_Camera_Vehicle( VehicleGameObj * obj, int seat = 0 );
	bool Is_In_Camera_Frustrum(Vector3&);
	static bool Are_Observers_Active( void ) { return AreObserversActive; }
	static void Set_Observers_Active( bool onoff ) { AreObserversActive = onoff; }
	static bool Is_First_Load( void ) { return IsFirstLoad; }
	static void Set_First_Load( bool onoff ) { IsFirstLoad = onoff; }
	static void Do_Skeleton_Slider_Demo();
	static void Toggle_Skeleton_Slider_Demo( void ) { EnableSkeletonSliderDemo = !EnableSkeletonSliderDemo; }
	static bool Is_Skeleton_Slider_Demo_Enabled( void ) { return EnableSkeletonSliderDemo; }
	static DazzleLayerClass *Get_Dazzle_Layer(void) { return DazzleLayer; }
	static void Set_First_Person( bool on_off ) { FirstPerson = on_off; }
	static bool Is_First_Person( void ) { return FirstPerson; }
	static void Set_First_Person_Default( bool on_off ) { FirstPersonDefault = on_off; }
	static bool Get_First_Person_Default( void ) { return FirstPersonDefault; }
	static void Set_Difficulty_Level( int level ) { DifficultyLevel = level; }
	static int Get_Difficulty_Level( void ) { return DifficultyLevel; }
	static bool Are_Transitions_Automatic( void ) { return AutoTransitions; }
	static void Set_Transitions_Automatic( bool state ) { AutoTransitions = state; }
	static MessageWindowClass *Get_Message_Window (void) { return MessageWindow; }
	static void Show_Star_Damage_Direction( int direction ) { StarDamageDirection |= 1 << (direction&7); }
	static int Get_Star_Damage_Direction( void ) { return StarDamageDirection; }
	static void Clear_Star_Damage_Direction( void ) { StarDamageDirection = 0; }
	static int Get_Sync_Time( void ) { return SyncTime; }
	static bool Is_Game_Paused( void ) { return IsGamePaused; }
	static void Register_Star_Killer(ArmedGameObj*);
	static const char *Get_Start_Script( void ) { return StartScript; }
	static const char *Get_Respawn_Script( void ) { return RespawnScript; }
	static void Set_Start_Script( const char * string ) { StartScript = string; }
	static void Set_Respawn_Script( const char * string ) { RespawnScript = string; }
	static void Request_Autosave( bool state = true ) { AutosaveRequested = state; }
	static bool Is_Autosave_Requested( void ) { return AutosaveRequested; }
	static int Get_Reload_Count( void ) { return ReloadCount; }
	static bool Is_Hit_Reticle_Enabled( void ) { return HitReticleEnabled; }
	static void Toggle_Hit_Reticle_Enabled( void ) { HitReticleEnabled = !HitReticleEnabled; }
	static const char * Get_Last_LSD_Name( void ) { return LastLSDName; };
	static void Set_Last_LSD_Name( const char * name ) { LastLSDName = name; };
	static int Get_Load_Progress( void ) { return LoadProgress; };
	static void Inc_Load_Progress( void ) { LoadProgress++; };
	static void Set_Load_Progress( int set ) { LoadProgress = set; };
private:
	static const ReferencerClass& getTheStarReference() { return TheStar; }
	static void postSceneInit();
	static REF_DECL2(IAmServer, bool);
	static REF_DECL2(IAmClient, bool);
	static REF_DECL2(IAmOnlyClient, bool);
	static REF_DECL2(IAmOnlyServer, bool);
	static REF_DECL2(MyId, int);
	static REF_DECL2(SyncTime, uint);
	static REF_DECL2(IsGamePaused, bool);
	static REF_DECL2(IsLevelInitialized, bool);
	static REF_DECL2(FriendlyFirePermitted, bool);
	static REF_DECL2(BeaconPlacementEndsGame, bool);
	static REF_DECL2(FirstPerson, bool);
	static REF_DECL2(FirstPersonDefault, bool);
	static REF_DECL2(MainCamera, CCameraClass*);
	static REF_DECL2(BackgroundScene, SimpleSceneClass*);
	static REF_DECL2(SoundEnvironment, SoundEnvironmentClass*);
	static REF_DECL2(DazzleLayer, DazzleLayerClass*);
	static REF_DECL2(MessageWindow, MessageWindowClass*);
	static REF_DECL2(TheStar, ReferencerClass);
	static REF_DECL2(IsStarDeterminingTarget, bool);
	static REF_DECL2(IsFirstLoad, bool);
	static REF_DECL2(AreObserversActive, bool);
	static REF_DECL2(EnableSkeletonSliderDemo, bool);
	static REF_DECL2(DifficultyLevel, int);
	static REF_DECL2(AutoTransitions, bool);
	static REF_DECL2(StarDamageDirection, int);
	static REF_DECL2(StarKillerID, int);
	static REF_DECL2(NetworkHandler, CombatNetworkHandlerClass*);
	static REF_DECL2(MiscHandler, CombatMiscHandlerClass*);
	static REF_DECL2(StartScript, StringClass);
	static REF_DECL2(RespawnScript, StringClass);
	static REF_DECL2(ReloadCount, int);
	static REF_DECL2(HitReticleEnabled, bool);
	static REF_DECL2(IsGameplayPermitted, bool);
	typedef enum {
		COMBAT_MODE_NONE,
		COMBAT_MODE_FIRST_PERSON,
		COMBAT_MODE_THIRD_PERSON,
		COMBAT_MODE_SNIPING,
		COMBAT_MODE_IN_VEHICLE,
		COMBAT_MODE_ON_LADDER,
		COMBAT_MODE_DIEING,
		COMBAT_MODE_CORPSE,
		COMBAT_MODE_SNAP_SHOT,
	};
	static REF_DECL2(CombatMode, int);
	static REF_DECL2(AutosaveRequested, bool);
	static void Set_Combat_Mode(int);
	static void Update_Combat_Mode();
	static REF_DECL2(LastRoundTripPingMs, int);
	static REF_DECL2(AvgRoundTripPingMs, int);
	static REF_DECL2(LastLSDName, StringClass);
	static REF_DECL2(LoadProgress, int);
	static REF_DECL2(MultiplayRenderingAllowed, bool);
	static REF_DECL2(_TheCombatDazzleHandler, CombatDazzleClass);
};


#endif
