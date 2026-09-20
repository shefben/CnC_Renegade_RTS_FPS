/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/***********************************************************************************************
 ***                            Confidential - Westwood Studios                              ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/combat.cpp                            $*
 *                                                                                             *
 *                      $Author:: Bhayes                                                      $*
 *                                                                                             *
 *                     $Modtime:: 2/17/02 10:25a                                              $*
 *                                                                                             *
 *                    $Revision:: 265                                                        $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "combat.h"

#include "purchaseavailability.h"
#include "gameeventbus.h"
#include "ccamera.h"
#include "scopemgr.h"
#include "gameobjmanager.h"
#include "input.h"
#include "weaponmanager.h"
#include "wwprofile.h"
#include "scriptman.h"
#include "pscene.h"
#include "damage.h"
#include "ww3d.h"
#include "assets.h"
#include "timemgr.h"
#include "soldier.h"
#include "SoundScene.h"
#include "weapons.h"
#include "vehicle.h"
#include "bones.h"
#include "humanstate.h"
#include "surfaceeffects.h"
#include "debug.h"
#include "backgroundmgr.h"
#include "postprocess.h"
#include "cover.h"
#include "spawn.h"
#include "vehiclephys.h"
#include "building.h"
#include "objectives.h"
#include "conversationmgr.h"
#include "bullet.h"
#include "debrissystem.h"
#include "statusoverlaysystem.h"
#include "worlddebugdisplay.h"
#include "commandermodecontroller.h"
#include "commandersidebar.h"
#include "commanderplacement.h"
#include "buildingconstruction.h"
#include "runtimebuildingfactory.h"
#include "commandercommandpalette.h"
#include "commandercommandpaletterenderer.h"
#include "commanderguardai.h"
#include "commanderselection.h"
#include "commanderselectionrenderer.h"
#include "commanderwaypoint.h"
#include "commanderwaypointrenderer.h"
#include "commanderproduction.h"
#include "definitionmgr.h"
#include "gameobjobserver.h"
#include "physicalgameobj.h"
#include "commanderghostrenderer.h"
#include "commanderstate.h"
#include "commandersidebarrenderer.h"
#include "constructionrenderer.h"
#include "dazzle.h"
#include "messagewindow.h"
#include "hudinfo.h"
#include "WeatherMgr.h"
#include "thread.h"
#include "savegame.h"
#include "buildingdescriptormanager.h"
#include "projectbuildingdefs.h"
#include "assetdep.h"
#include "saveloadstatus.h"
#include <stdio.h>
#include "SoundEnvironment.h"
#include "weaponview.h"
#include "hud.h"
#include "mapmgr.h"
#include "gametype.h"
#include "staticnetworkobject.h"
#include "diaglog.h"
#include "combatdazzle.h"
#include "ffactorylist.h"
#include "definitionfactorymgr.h"
#include "definitionfactory.h"
#include "globalsettings.h"
#include "dx8wrapper.h"
#include "Except.h"
#include "cheatmgr.h"
#include "systeminfolog.h"
#include "assetstatus.h"
#include "wwmemlog.h"
#include "unitcoordinationzonemgr.h"
#include "FastAllocator.h"
#include "screenfademanager.h"
#include "animatedsoundmgr.h"
#include "render2dsentence.h"
#include "stylemgr.h"
#include "translatedb.h"
#include "string_ids.h"


const int DEFAULT_MAX_SHADOWS = 4;


/*
**
*/
CCameraClass				*		CombatManager::MainCamera			= nullptr;
SimpleSceneClass			*		CombatManager::BackgroundScene	= nullptr;
SoundEnvironmentClass	*		CombatManager::SoundEnvironment	= nullptr;
DazzleLayerClass			*		CombatManager::DazzleLayer			= nullptr;
GameObjReference					CombatManager::TheStar;
bool									CombatManager::IsStarDeterminingTarget = true;
//bool									CombatManager::IAmServer			= true;
//bool									CombatManager::IAmClient			= true;
bool									CombatManager::IAmServer			= false;
bool									CombatManager::IAmClient			= false;
bool									CombatManager::IAmOnlyClient		= false;
bool									CombatManager::IAmOnlyServer		= false;
int									CombatManager::MyId					= 0;
int									CombatManager::SyncTime				= 0;
CombatNetworkHandlerClass *	CombatManager::NetworkHandler		= nullptr;
CombatMiscHandlerClass *		CombatManager::MiscHandler			= nullptr;
MessageWindowClass *				CombatManager::MessageWindow		= nullptr;
bool									CombatManager::EnableSkeletonSliderDemo = false;
int									CombatManager::DifficultyLevel		= 1;
bool									CombatManager::AutoTransitions		= true;
int									CombatManager::StarDamageDirection	= 0;
bool									CombatManager::AreObserversActive	= true;
bool									CombatManager::FirstPerson				= true;
bool									CombatManager::FirstPersonDefault 		= true;
bool									CombatManager::IsFirstLoad				= false;
int									CombatManager::StarKillerID			= 0;
bool									CombatManager::IsGamePaused			= false;
bool									CombatManager::IsLevelInitialized	= false;
StringClass							CombatManager::StartScript;
StringClass							CombatManager::RespawnScript;
bool									CombatManager::AutosaveRequested		= false;
DWORD									CombatManager::LastRoundTripPingMs	= 0;
DWORD									CombatManager::AvgRoundTripPingMs	= 0;
bool									CombatManager::FriendlyFirePermitted = false;
bool									CombatManager::BeaconPlacementEndsGame = false;
bool									CombatManager::HitReticleEnabled	 = true;
bool									CombatManager::IsGameplayPermitted = false;

int									CombatManager::CombatMode = CombatManager::COMBAT_MODE_NONE;
int									CombatManager::ReloadCount = 0;
StringClass							CombatManager::LastLSDName;
int									CombatManager::LoadProgress;
bool									CombatManager::MultiplayRenderingAllowed = true;

static	PhysicsSceneClass	*	GameScene	= nullptr;

/*
**
*/
void	CombatManager::Init( bool render_available )
{
//	Debug_Say(("CombatManager::Init\n"));

	IsGameplayPermitted=false;

	//
	//	Up first: ScriptManager::Init below registers the built-in scripts,
	//	and a script may subscribe to an event as it registers.
	//
	GameEventBus::Init();

	ConversationMgrClass::Initialize ();

	MessageWindow = new MessageWindowClass;
	MessageWindow->Initialize ();

	ScriptManager::Init();

	BonesManager::Init();

	ArmorWarheadManager::Init();

	CCameraClass::Init();

	ScopeMgrClass::Init();

	SurfaceEffectsManager::Init();

	ObjectiveManager::Init();

	CombatSoundManager::Init();

	// create THE camera
	MainCamera = new CCameraClass();

	// create the dazzle layer
	if (render_available) {
		DazzleLayer = new DazzleLayerClass;
		DazzleRenderObjClass::Set_Current_Dazzle_Layer(DazzleLayer);
	} else {
		DazzleRenderObjClass::Set_Current_Dazzle_Layer(nullptr);
	}

	// Pass the main camera onto the 3D audio library as the
	// listener's position.
	SoundSceneClass *sound_scene = WWAudioClass::Get_Instance ()->Get_Sound_Scene ();
	if (sound_scene != nullptr) {
		sound_scene->Attach_Listener_To_Obj (MainCamera);
	}

	HUDClass::Init(render_available);
	ScreenFadeManager::Init();
	FirstPerson	= FirstPersonDefault;
}

/*
**
*/
void	CombatManager::Shutdown( void )
{
//	Debug_Say(("CombatManager::Shutdown\n"));
	GameEventBus::Shutdown();

	ScreenFadeManager::Shutdown();
	HUDClass::Shutdown();

	if ( GameScene != nullptr ) {
		// Debug_Say(( "Releasing the PScene with %d Refs\n", GameScene->Num_Refs() ));
		GameScene->Release_Ref();
		GameScene = nullptr;
	}

	//
	// Reset the audio library
	//
	if (WWAudioClass::Get_Instance () != nullptr) {
		SoundSceneClass *sound_scene = WWAudioClass::Get_Instance ()->Get_Sound_Scene ();
		if (sound_scene != nullptr) {
			sound_scene->Attach_Listener_To_Obj (nullptr);
		}
	}

	if (DazzleLayer != nullptr) {
		delete DazzleLayer;
		DazzleLayer = nullptr;
		DazzleRenderObjClass::Set_Current_Dazzle_Layer(nullptr);
	}

	MainCamera->Release_Ref();
	MainCamera = nullptr;

	CombatSoundManager::Shutdown();

	ObjectiveManager::Shutdown();

	SurfaceEffectsManager::Shutdown();

	ScopeMgrClass::Shutdown();

	CCameraClass::Shutdown();

	ArmorWarheadManager::Shutdown();

	BonesManager::Shutdown();

	ScriptManager::Shutdown();

	if (MessageWindow != nullptr) {
		MessageWindow->Shutdown ();
		delete MessageWindow;
		MessageWindow = nullptr;
	}

	ConversationMgrClass::Shutdown ();

//	Debug_Say(( "Combat Shutdown %d refs\n", RefCountClass::Total_Refs() ));
}

//
//	The sink Scene_Init uses: every group written straight through to the game
//	scene, which is where the matrix has to end up.
//
class	PhysicsSceneCollisionGroupSink : public CollisionGroupSinkClass
{
public:
	void	Enable_All( int group ) override
		{ COMBAT_SCENE->Enable_All_Collision_Detections( group ); }
	void	Disable_All( int group ) override
		{ COMBAT_SCENE->Disable_All_Collision_Detections( group ); }
	void	Enable( int group0, int group1 ) override
		{ COMBAT_SCENE->Enable_Collision_Detection( group0, group1 ); }
	void	Disable( int group0, int group1 ) override
		{ COMBAT_SCENE->Disable_Collision_Detection( group0, group1 ); }
};


void	CombatManager::Scene_Init( void )
{
	// Game scene is where the main action occurs!
	GameScene = new PhysicsSceneClass;
	GameScene->Set_Ambient_Light(Vector3(0.55f,0.55f,0.55f));
	GameScene->Set_Ambient_Light(Vector3(1,1,1));
	GameScene->Set_Fog_Color(Vector3(0.6f,0.6f,0.6f)); //Vector3(80.0f/255.0f,130.0f/255.0f,180.0f/255.0f));

	PhysicsSceneCollisionGroupSink	sink;
	Define_Collision_Groups( sink );
}


//
//	Define_Collision_Groups
//
//	The collision matrix, described once.  Scene_Init writes it into the scene;
//	the hard-gate self check writes it into a table of its own and reads it
//	back, which is the only way to look at the matrix without a physics scene
//	and the device it needs behind it.
//
void	CombatManager::Define_Collision_Groups( CollisionGroupSinkClass & sink )
{
	//
	//	Order matters and is the whole trick: every Enable_All first, then every
	//	Disable_All, then the individual pairs.  A narrow group's Disable_All is
	//	what takes it back out of the permissive groups it was just added to, so
	//	only the pairs named below survive.
	//

	//
	//	Permissive groups -- these collide with everything unless a narrow group
	//	opts out of them further down.
	//
	sink.Enable_All( DEFAULT_COLLISION_GROUP );
	sink.Enable_All( BULLET_COLLISION_GROUP );
	sink.Enable_All( TERRAIN_COLLISION_GROUP );
	sink.Enable_All( PhysicsSceneClass::COLLISION_GROUP_WORLD );
	sink.Enable_All( SOLDIER_GHOST_COLLISION_GROUP );
	sink.Enable_All( SOLDIER_COLLISION_GROUP );
	sink.Enable_All( C4_COLLISION_GROUP );
	sink.Enable_All( NAVAL_UNIT_COLLISION_GROUP );
	sink.Enable_All( BEACHING_UNIT_COLLISION_GROUP );
	sink.Enable_All( HOVER_UNIT_COLLISION_GROUP );
	sink.Enable_All( AMPHIBIOUS_UNIT_COLLISION_GROUP );
	sink.Enable_All( UNDERGROUND_TRANSITION_COLLISION_GROUP );
	sink.Enable_All( PLAYER_BUILDING_COLLISION_GROUP );
	sink.Enable_All( PLAYER_BUILDING_GHOST_COLLISION_GROUP );

	//
	//	Narrow groups -- nothing collides with these until it is named below.
	//
	sink.Disable_All( UNCOLLIDEABLE_GROUP );
	sink.Disable_All( TERRAIN_ONLY_COLLISION_GROUP );
	sink.Disable_All( TERRAIN_AND_BULLET_COLLISION_GROUP );
	sink.Disable_All( BULLET_ONLY_COLLISION_GROUP );
	sink.Disable_All( UNDERGROUND_COLLISION_GROUP );
	sink.Disable_All( SOLDIER_ONLY_COLLISION_GROUP );
	sink.Disable_All( SOLDIER_BULLET_COLLISION_GROUP );
	sink.Disable_All( WATER_SURFACE_COLLISION_GROUP );
	sink.Disable_All( WATER_EDGE_COLLISION_GROUP );
	sink.Disable_All( WATER_EDGE_ALT_COLLISION_GROUP );
	sink.Disable_All( BEACH_EDGE_COLLISION_GROUP );
	sink.Disable_All( AMPHIBIOUS_UNIT_FLOOR_COLLISION_GROUP );
	sink.Disable_All( DEFAULT_AND_SOLDIER_ONLY_COLLISION_GROUP );
	sink.Disable_All( TRAIN_COLLISION_GROUP );
	sink.Disable_All( TRAIN_TRACK_COLLISION_GROUP );

	//
	//	Stock pairs
	//
	sink.Enable( TERRAIN_ONLY_COLLISION_GROUP, TERRAIN_COLLISION_GROUP );
	sink.Enable( TERRAIN_AND_BULLET_COLLISION_GROUP, TERRAIN_COLLISION_GROUP );
	sink.Enable( TERRAIN_AND_BULLET_COLLISION_GROUP, BULLET_COLLISION_GROUP );
	sink.Disable( BULLET_COLLISION_GROUP, BULLET_COLLISION_GROUP );
	sink.Enable( BULLET_ONLY_COLLISION_GROUP, BULLET_COLLISION_GROUP );
	sink.Disable( PhysicsSceneClass::COLLISION_GROUP_WORLD,PhysicsSceneClass::COLLISION_GROUP_WORLD );
	sink.Disable( SOLDIER_GHOST_COLLISION_GROUP, SOLDIER_COLLISION_GROUP );
	sink.Disable( SOLDIER_GHOST_COLLISION_GROUP, SOLDIER_GHOST_COLLISION_GROUP );

	//
	//	A ghosted soldier passes through the DEFAULT group as well.  Ghosting
	//	is entered inside a ladder or elevator coordination zone and while
	//	squeezing past a team mate, and in both cases the point is that the
	//	soldier is not to be stopped by whatever he is sharing the space with.
	//	Leaving DEFAULT solid meant a vehicle or a placed dynamic object could
	//	still pin him at the foot of a ladder, which is the 4.8.4 ladder fix
	//	and the behavior every server has run since.
	//
	sink.Disable( SOLDIER_GHOST_COLLISION_GROUP, DEFAULT_COLLISION_GROUP );

	//
	//	C4 is DEFAULT plus ghosted soldiers -- a proximity mine has to notice a
	//	soldier who is walking through his own team -- and minus the two edge
	//	volumes, which exist to steer vehicles and would detonate it.
	//
	sink.Enable( C4_COLLISION_GROUP, SOLDIER_GHOST_COLLISION_GROUP );
	sink.Disable( C4_COLLISION_GROUP, UNDERGROUND_TRANSITION_COLLISION_GROUP );

	//
	//	An underground unit is out of the world: terrain holds it up, other
	//	underground units block it, and the transition volume is how it surfaces.
	//
	sink.Enable( UNDERGROUND_COLLISION_GROUP, TERRAIN_COLLISION_GROUP );
	sink.Enable( UNDERGROUND_COLLISION_GROUP, UNDERGROUND_COLLISION_GROUP );
	sink.Enable( UNDERGROUND_COLLISION_GROUP, UNDERGROUND_TRANSITION_COLLISION_GROUP );

	//
	//	Volumes that only infantry may be stopped by
	//
	sink.Enable( SOLDIER_ONLY_COLLISION_GROUP, SOLDIER_COLLISION_GROUP );
	sink.Enable( SOLDIER_ONLY_COLLISION_GROUP, SOLDIER_GHOST_COLLISION_GROUP );

	sink.Enable( SOLDIER_BULLET_COLLISION_GROUP, SOLDIER_COLLISION_GROUP );
	sink.Enable( SOLDIER_BULLET_COLLISION_GROUP, SOLDIER_GHOST_COLLISION_GROUP );
	sink.Enable( SOLDIER_BULLET_COLLISION_GROUP, BULLET_COLLISION_GROUP );
	sink.Enable( SOLDIER_BULLET_COLLISION_GROUP, C4_COLLISION_GROUP );

	//
	//	The water surface is solid to anything that travels on it and to bullets,
	//	and transparent to everything else.
	//
	sink.Enable( WATER_SURFACE_COLLISION_GROUP, NAVAL_UNIT_COLLISION_GROUP );
	sink.Enable( WATER_SURFACE_COLLISION_GROUP, BEACHING_UNIT_COLLISION_GROUP );
	sink.Enable( WATER_SURFACE_COLLISION_GROUP, HOVER_UNIT_COLLISION_GROUP );
	sink.Enable( WATER_SURFACE_COLLISION_GROUP, AMPHIBIOUS_UNIT_COLLISION_GROUP );
	sink.Enable( WATER_SURFACE_COLLISION_GROUP, BULLET_COLLISION_GROUP );

	//
	//	Shoreline volumes.  WATER_EDGE fences in the things that must not leave
	//	the water; the ALT variant fences in naval units alone, and BEACH_EDGE
	//	fences in the units allowed to run aground.
	//
	sink.Enable( WATER_EDGE_COLLISION_GROUP, DEFAULT_COLLISION_GROUP );
	sink.Enable( WATER_EDGE_COLLISION_GROUP, SOLDIER_COLLISION_GROUP );
	sink.Enable( WATER_EDGE_COLLISION_GROUP, NAVAL_UNIT_COLLISION_GROUP );
	sink.Enable( WATER_EDGE_ALT_COLLISION_GROUP, NAVAL_UNIT_COLLISION_GROUP );
	sink.Enable( BEACH_EDGE_COLLISION_GROUP, BEACHING_UNIT_COLLISION_GROUP );

	//
	//	The amphibious floor is the lake bed an amphibious unit drives along and
	//	nothing else can see.
	//
	sink.Enable( AMPHIBIOUS_UNIT_FLOOR_COLLISION_GROUP, AMPHIBIOUS_UNIT_COLLISION_GROUP );

	sink.Enable( DEFAULT_AND_SOLDIER_ONLY_COLLISION_GROUP, DEFAULT_COLLISION_GROUP );
	sink.Enable( DEFAULT_AND_SOLDIER_ONLY_COLLISION_GROUP, SOLDIER_COLLISION_GROUP );
	sink.Enable( DEFAULT_AND_SOLDIER_ONLY_COLLISION_GROUP, SOLDIER_GHOST_COLLISION_GROUP );

	//
	//	A player-built building is solid like terrain, except to the ghost group
	//	a building being placed uses, which has to be able to sit inside it while
	//	the placement is still provisional.
	//
	sink.Disable( PLAYER_BUILDING_COLLISION_GROUP, PLAYER_BUILDING_GHOST_COLLISION_GROUP );

	//
	//	A train is held by its track, not by the ground, so it ignores terrain
	//	entirely.  It still has to hit the things a moving vehicle must hit.
	//
	sink.Enable( TRAIN_COLLISION_GROUP, TRAIN_TRACK_COLLISION_GROUP );
	sink.Enable( TRAIN_COLLISION_GROUP, DEFAULT_COLLISION_GROUP );
	sink.Enable( TRAIN_COLLISION_GROUP, SOLDIER_COLLISION_GROUP );
	sink.Enable( TRAIN_COLLISION_GROUP, SOLDIER_GHOST_COLLISION_GROUP );
	sink.Enable( TRAIN_COLLISION_GROUP, BULLET_COLLISION_GROUP );
	sink.Enable( TRAIN_COLLISION_GROUP, C4_COLLISION_GROUP );
	sink.Enable( TRAIN_COLLISION_GROUP, NAVAL_UNIT_COLLISION_GROUP );
	sink.Enable( TRAIN_COLLISION_GROUP, BEACHING_UNIT_COLLISION_GROUP );
	sink.Enable( TRAIN_COLLISION_GROUP, HOVER_UNIT_COLLISION_GROUP );
	sink.Enable( TRAIN_COLLISION_GROUP, AMPHIBIOUS_UNIT_COLLISION_GROUP );

	//
	//	Track pieces line up end to end, so they have to see each other.
	//
	sink.Enable( TRAIN_TRACK_COLLISION_GROUP, TRAIN_TRACK_COLLISION_GROUP );
}


//
//	Get_Collision_Group_Name
//
//	Friendly name for a collision group.  Debug output and the tools show this
//	rather than a bare number.
//
const char *	Get_Collision_Group_Name( Collision_Group_Type group )
{
	switch ( group ) {
		case DEFAULT_COLLISION_GROUP:						return "Default";
		case UNCOLLIDEABLE_GROUP:							return "Uncollideable";
		case TERRAIN_ONLY_COLLISION_GROUP:				return "Terrain-only";
		case BULLET_COLLISION_GROUP:						return "Bullet";
		case TERRAIN_AND_BULLET_COLLISION_GROUP:		return "Terrain and Bullet";
		case BULLET_ONLY_COLLISION_GROUP:				return "Bullet-only";
		case SOLDIER_COLLISION_GROUP:						return "Soldier";
		case SOLDIER_GHOST_COLLISION_GROUP:				return "Soldier Ghost";
		case C4_COLLISION_GROUP:							return "C4";
		case UNDERGROUND_COLLISION_GROUP:				return "Underground";
		case SOLDIER_ONLY_COLLISION_GROUP:				return "Soldier-only";
		case SOLDIER_BULLET_COLLISION_GROUP:			return "Soldier and Bullet";
		case TERRAIN_COLLISION_GROUP:						return "Terrain";
		case WATER_SURFACE_COLLISION_GROUP:				return "Water Surface";
		case WATER_EDGE_COLLISION_GROUP:					return "Water Edge";
		case WATER_EDGE_ALT_COLLISION_GROUP:			return "Water Edge (Naval)";
		case BEACH_EDGE_COLLISION_GROUP:					return "Beach Edge";
		case NAVAL_UNIT_COLLISION_GROUP:					return "Naval Unit";
		case BEACHING_UNIT_COLLISION_GROUP:				return "Beaching Unit";
		case HOVER_UNIT_COLLISION_GROUP:					return "Hover Unit";
		case AMPHIBIOUS_UNIT_COLLISION_GROUP:			return "Amphibious Unit";
		case AMPHIBIOUS_UNIT_FLOOR_COLLISION_GROUP:	return "Amphibious Unit Floor";
		case UNDERGROUND_TRANSITION_COLLISION_GROUP:	return "Underground Transition";
		case DEFAULT_AND_SOLDIER_ONLY_COLLISION_GROUP:	return "Default and Soldier-only";
		case PLAYER_BUILDING_COLLISION_GROUP:			return "Player Building";
		case PLAYER_BUILDING_GHOST_COLLISION_GROUP:	return "Player Building Ghost";
		case TRAIN_COLLISION_GROUP:						return "Train";
		case TRAIN_TRACK_COLLISION_GROUP:				return "Train Track";
		default:													return "Unknown";
	}
}

/*
** Note:  The editor calls these per level functions only once per program execution.
** If you have code that requires a init/shutdown/reset call with each level load,
** notify Pat so he can add those calls manually
**
*/
//
//	The level currently being loaded, unloaded or played.  Declared here rather
//	than beside the load thread below because the level lifecycle events report
//	it, and those are raised from further up this file.
//
StringClass	_load_map_name;

void	CombatManager::Pre_Load_Level( bool render_available )
{
	//
	//	Raised before anything of the new level exists and before any client
	//	has been told about it, which is the last point at which a subscriber
	//	can still change what the clients will be sent.
	//
	GameEventBus::Raise_Pre_Load_Level( _load_map_name );

	//	Every purchase entry is on offer again in a new level.
	PurchaseAvailabilityClass::Reset();
	CommanderStateClass::Reset();

	MultiplayRenderingAllowed = true;
	if ( !IS_MISSION && !I_Am_Server() ) {
		MultiplayRenderingAllowed = false;
	}

	HUDClass::Enable( true );
	HUDClass::Reset();

	IsGamePaused = false;

	GameObjObserverManager::Reset();

	CoverManager::Init();

	GameObjManager::Init();

	BulletManager::Init();
	DebrisManager::Init();
	CommanderModeController::Init();

	//	Roadmap Sections 42-46, Phases 36-40.  Each is pure state with no device dependency, so
	//	all four come up here with the rest of the combat services and are torn down together
	//	below -- a dedicated server initialises exactly the same set (Section 47).
	CommanderSidebarModel::Init();
	CommanderPlacementValidator::Init();
	BuildingConstructionService::Init();
	CommanderProductionService::Init();
	CommanderGuardAI::Init();
	CommanderSelection::Init();
	CommanderCommandPalette::Init();
	CommanderWaypointSystem::Init();

	CommanderSidebarRenderer::Init( render_available );
	CommanderGhostRenderer::Init( render_available );
	BuildingConstructionRenderer::Init( render_available );
	CommanderSelectionRenderer::Init( render_available );
	CommanderCommandPaletteRenderer::Init( render_available );
	CommanderWaypointRenderer::Init( render_available );

	cEncoderList::Clear_Entries();

	cPacket::Init_Encoder();

	BackgroundScene = NEW_REF (SimpleSceneClass, ());

	SoundEnvironment = NEW_REF (SoundEnvironmentClass, ());

	BackgroundMgrClass::Init (BackgroundScene, SoundEnvironment, render_available);

	WeatherMgrClass::Init (SoundEnvironment);

	WeaponViewClass::Init();

	SmartGameObj::Set_Global_Sight_Range_Scale( 1.0f );
//	SoundSystem::Set_Global_Listener_Scale( 1.0f );

	//
	//	Map initialization (temporary -- until its more data driven)
	//
	/*MapMgrClass::Set_Map_Texture ("L1-MAP.TGA");
	MapMgrClass::Set_Map_Center (Vector2 (308.0F, 414.0F));
	MapMgrClass::Set_Map_Scale (Vector2 (0.53F, 0.54F));
	MapMgrClass::Cloud_All_Cells ();*/

	ScreenFadeManager::Enable_Letterbox( 0, 0 );
	ScreenFadeManager::Set_Screen_Overlay_Opacity( 0, 0 );

	HUDInfo::Set_HUD_Help_Text( U_CHAR("") );	// Clear text
}

bool	_preload_assets;

static class LoadThreadClass : public ThreadClass
{
public:
	LoadThreadClass(const char *thread_name = "Game loader thread") : ThreadClass(thread_name) {}

	void Thread_Function() override {

		CombatManager::Set_Load_Progress( 0 );

		WWMEMLOG(MEM_GAMEDATA);

		WWLOG_PREPARE_TIME_AND_MEMORY("Game loader thread");

		#ifndef PARAM_EDITING_ON
			// Tell the datasafe to expect calls from this thread now.
			GenericDataSafeClass::Set_Preferred_Thread(ThreadClass::Get_Current_Thread_ID());
		#endif // PARAM_EDITING_ON

		CombatManager::Inc_Load_Progress();

		//	Reload the definition databases (to support level-specific temp ddb's)
		INIT_STATUS("Free definition databases");
		DefinitionMgrClass::Free_Definitions();
		WWLOG_INTERMEDIATE("Free definitions");

		INIT_STATUS("Load definition databases");
		SaveGameManager::Load_Definitions();
		WWLOG_INTERMEDIATE("Load definitions");

		//
		//	Q-017.  Free_Definitions just above wiped every game object definition, including
		//	the two project-owned building presets (Tiberium Silo, Advanced Communications
		//	Center) that have no stock Objects.DDB entry -- re-add them every level load, not
		//	just the first, then re-link every buildable descriptor's game_object_preset back to
		//	the real definition it names (BuildingGameObjDef::DescriptorName), which is how
		//	Initialize_Building's TeamPowerGridService::RegisterBuilding call finds the right
		//	descriptor for a building created from a preset rather than a runtime-descriptor-
		//	carrying instance.  The descriptor directory itself only needs loading once: unlike
		//	DefinitionMgrClass, BuildingDescriptorManager's registry is not touched by
		//	Free_Definitions.
		//
		{
			static bool descriptors_loaded = false;
			if (!descriptors_loaded) {
				descriptors_loaded = true;
				BuildingDescriptorManager::Init ();
				BuildingDescriptorManager::Load_Directory ("data/building_descriptors",nullptr,nullptr);
			}
		}
		ProjectBuildingDefs::Register ();
		BuildingDescriptorManager::Resolve_Game_Object_Presets (nullptr);

		CombatManager::Inc_Load_Progress();
		//
		//	Make sure the animated-sound system is setup.  This needs
		// to be done after the definition databases are loaded...
		//
		AnimatedSoundMgrClass::Initialize ();

		StringClass filename_to_load( _load_map_name, true );
		StringClass lsd_filename( _load_map_name,true );

		// Prep the level loading
		INIT_STATUS("Pre Load level");

		PlayerInfoLog::Set_Current_Map_Name(_load_map_name);
		SaveGameManager::Pre_Load_Game(_load_map_name, filename_to_load, lsd_filename);
		CombatManager::Set_Last_LSD_Name( lsd_filename );
		WWLOG_INTERMEDIATE("Preload game");

		CombatManager::Inc_Load_Progress();

		//
		//	Preload the assets
		//
		if ( _preload_assets ) {
			INIT_STATUS("Preload assets");
			AssetDependencyManager::Load_Always_Assets();
			WWLOG_INTERMEDIATE("Preload always assets");

			CombatManager::Inc_Load_Progress();

			AssetDependencyManager::Load_Level_Assets( lsd_filename );
			WWLOG_INTERMEDIATE("Preload level assets");
		} else {
			CombatManager::Inc_Load_Progress();
		}

		CombatManager::Inc_Load_Progress();

		// Now load the level
		INIT_STATUS("Load level");
		SaveGameManager::Load_Game( filename_to_load );
		WWLOG_INTERMEDIATE("Load game");

		CombatManager::Inc_Load_Progress();

	}
} thread;

void	CombatManager::Load_Level_Threaded( const char * map_name, bool preload_assets )
{
	//
	//	Turn off music and sound effects while loading
	//
	IsLevelInitialized = false;
	WWAudioClass::Get_Instance ()->Enable_New_Sounds (false);

	_preload_assets = preload_assets;
	_load_map_name = map_name;

	WWASSERT(!thread.Is_Running());
	thread.Stop();
	thread.Execute();
}

bool	CombatManager::Is_Load_Level_Complete( void )
{
	ThreadClass::Sleep_Ms(50);
	return (thread.Is_Running() == false);
}

bool	CombatManager::Is_Loading_Level( void )
{
	return (thread.Is_Running());
}

/*
** Note:  The editor calls these per level functions only once per program execution.
** If you have code that requires a init/shutdown/reset call with each level load,
** notify Pat so he can add those calls manually
**
*/
void	CombatManager::Post_Load_Level( void )
{
	// Dont reset SyncTime.  Level from the editor should have it at 0, and levels from
	// the game need it set to continue cinematics
//	SyncTime = 0;
	CombatMode = COMBAT_MODE_NONE;
	Input::Flush();
	HitReticleEnabled	 = true;		// Default for on when you start each level

	Clear_Star_Damage_Direction();

#if 0
	// Re-init all objects loaded from editor
	if ( CombatManager::Is_First_Load() ) {
		Debug_Say(( "Re-Initing all game objects\n" ));
		GameObjManager::Init_All();
	}
#endif

	//
	//	Build network wrappers for every static anim object in the level
	//
	StaticNetworkObjectClass::Generate_Static_Network_Objects ();


	// Debug code to create a definition
#if 0
	DefinitionFactoryClass * def_factory = (DefinitionFactoryClass *)DefinitionFactoryMgrClass::Find_Factory( CLASSID_GLOBAL_SETTINGS_DEF_HUMAN_ANIM_OVERRIDE );
	if ( def_factory != nullptr ) {
		HumanAnimOverrideDef * def = (HumanAnimOverrideDef *)def_factory->Create();
		if ( def != nullptr ) {
			def->Set_Name( "HAO Test" );
			def->Set_ID (DefinitionMgrClass::Get_New_ID (def->Get_Class_ID ()));
			def->WalkEmptyHands = "H_A_432A";
			def->RunEmptyHands = "H_A_J43B";
			DefinitionMgrClass::Register_Definition( def );
		}
	}
#endif

	if (IsLevelInitialized == false && TheStar != nullptr) {
		IsLevelInitialized = true;

		//
		//	Turn music and sound effects back on...
		//
		WWAudioClass::Get_Instance ()->Enable_New_Sounds (true);
	}

	//
	//	Generate the unit coordination zones
	//
	UnitCoordinationZoneMgr::Build_Zones ();

	//
	//	The level is complete and playable from here.
	//
	GameEventBus::Raise_Level_Loaded( _load_map_name );
	return ;
}

/*
** Note:  The editor calls these per level functions only once per program execution.
** If you have code that requires a init/shutdown/reset call with each level load,
** notify Pat so he can add those calls manually
**
*/

void	CombatManager::Unload_Level( void )
{
	//
	//	Raised while the level is still standing; a subscriber that holds
	//	object IDs has to drop them before the teardown below runs.
	//
	GameEventBus::Raise_Level_Unloaded( _load_map_name );

	// Display please wait screen.....
	Render2DSentenceClass backdropText;
	backdropText.Set_Texture_Size_Hint( 256 );
	FontCharsClass *font	= StyleMgrClass::Peek_Font( StyleMgrClass::FONT_INGAME_BIG_TXT );
	if (font != nullptr) {
		backdropText.Set_Font( font );
		WideStringClass wide_str = TRANSLATE( IDS_MP_LOADING );
		backdropText.Build_Sentence( wide_str );
		Vector2 extents = backdropText.Get_Text_Extents( wide_str );
		float x = (Render2DClass::Get_Screen_Resolution().Right - extents.X) / 2.0f;
		float y = (Render2DClass::Get_Screen_Resolution().Bottom - extents.Y) / 2.0f;
		backdropText.Set_Location( Vector2( WWMath::Trunc(x), WWMath::Trunc(y) ) );
		backdropText.Draw_Sentence( 0xFF00FF00 );	// Green
		WW3D::Begin_Render( true, true, Vector3(0.0f,0.0f,0.0f), nullptr);
		backdropText.Render();
		WW3D::End_Render();
	}

	WWLOG_PREPARE_TIME_AND_MEMORY("Unload level");

	WWDEBUG_SAY(("CombatManager::Unload_Level\n"));
	SystemInfoLog::Set_State_Exiting();
	WWLOG_INTERMEDIATE("SystemInfoLog::Set_State_Exiting()");

	// Don't log load-on-demands after the game is over
	AssetStatusClass::Peek_Instance()->Enable_Load_On_Demand_Reporting(false);
	WWLOG_INTERMEDIATE("AssetStatusClass::Peek_Instance()->Enable_Load_On_Demand_Reporting(false)");

	//
	//	Free the static anim phys object network wrappers
	//
	StaticNetworkObjectClass::Free_Static_Network_Objects ();
	WWLOG_INTERMEDIATE("StaticNetworkObjectClass::Free_Static_Network_Objects ()");

	WeaponViewClass::Shutdown();
	WWLOG_INTERMEDIATE("WeaponViewClass::Shutdown()");

	GameObjManager::Shutdown();
	WWLOG_INTERMEDIATE("GameObjManager::Shutdown()");

	CoverManager::Shutdown();
	WWLOG_INTERMEDIATE("CoverManager::Shutdown()");

	BulletManager::Shutdown();
	WWLOG_INTERMEDIATE("BulletManager::Shutdown()");

	DebrisManager::Shutdown();
	WWLOG_INTERMEDIATE("DebrisManager::Shutdown()");

	StatusOverlaySystem::Shutdown();
	WWLOG_INTERMEDIATE("StatusOverlaySystem::Shutdown()");

	WorldDebugDisplay::Shutdown();
	WWLOG_INTERMEDIATE("WorldDebugDisplay::Shutdown()");

	CommanderModeController::Shutdown();
	WWLOG_INTERMEDIATE("CommanderModeController::Shutdown()");

	ObjectiveManager::Reset();
	WWLOG_INTERMEDIATE("ObjectiveManager::Reset()");

	WeatherMgrClass::Shutdown();
	WWLOG_INTERMEDIATE("WeatherMgrClass::Shutdown()");

	BackgroundMgrClass::Shutdown();
	WWLOG_INTERMEDIATE("BackgroundMgrClass::Shutdown()");

	REF_PTR_RELEASE (SoundEnvironment);
	WWLOG_INTERMEDIATE("REF_PTR_RELEASE (SoundEnvironment)");

	REF_PTR_RELEASE (BackgroundScene);
	WWLOG_INTERMEDIATE("REF_PTR_RELEASE (BackgroundScene)");

	WW3DAssetManager::Get_Instance()->Free_Assets();	// Free all assets
	WWLOG_INTERMEDIATE("WW3DAssetManager::Get_Instance()->Free_Assets()");

	WWLOG_INTERMEDIATE("Delete factories");

	//
	//	Kill the background music and flush the cache
	//
	if (WWAudioClass::Get_Instance () != nullptr) {
		WWAudioClass::Get_Instance ()->Set_Background_Music (nullptr);
		WWAudioClass::Get_Instance ()->Flush_Cache ();
	}
	WWLOG_INTERMEDIATE("Flush audio cache");

	SystemInfoLog::Reset_State();
	WWLOG_INTERMEDIATE("SystemInfoLog::Reset_State()");

	if (MessageWindow != nullptr) {
		MessageWindow->Clear();
		MessageWindow->Clear_Log();
	}

	//
	//	Shutdown the animated-sound system
	//
	AnimatedSoundMgrClass::Shutdown();
	WWLOG_INTERMEDIATE("AnimatedSoundMgrClass::Shutdown");
	return ;
}

/*
**
*/
PhysicsSceneClass	*	CombatManager::Get_Scene( void )
{
	return PhysicsSceneClass::Get_Instance();
}

/*
** Note: Generate_Control and Think are separated because network update needs
** to happen in-between them.
*/

/*
**
*/
void 	CombatManager::Generate_Control( void )
{
	WWPROFILE( "Generate Control" );
	// First, generate input control for all necessary game objects and
	// possibly send those control to the server
	GameObjManager::Generate_Control();
}

/*
**
*/
void 	CombatManager::Handle_Input()
{
	if ( Input::Get_State( INPUT_FUNCTION_FIRST_PERSON_TOGGLE ) ) {
		Set_First_Person( !Is_First_Person() );

		if ( COMBAT_STAR ) {
			Vector3 pos;
			COMBAT_STAR->Get_Position( &pos );
			DIAG_LOG(( "SWPE", "%1.2f; %1.2f; %1.2f; %s ", pos.X, pos.Y, pos.Z, Is_First_Person() ? "First" : "Third" ));
		}
	}
}


/*
**	Roadmap Section 45's population accounting.  CommanderProductionService::Set_Population
**	needs a matching decrement on death and had nothing to call it from.  A minimal
**	GameObjObserverClass rather than a second registry: the engine already calls Killed() at
**	the moment of death and Detach() reliably when a torn-down object's observer list is
**	cleared.  Destroyed() is declared on the same interface and is never called anywhere in
**	this tree (grepped) -- Detach is the hook every self-freeing observer here actually uses,
**	see HarvesterClass::Detach's own comment for why deletion goes through the manager rather
**	than happening in Detach itself (it runs from inside the walk over the observer list).
*/
class CommanderPopulationObserverClass : public GameObjObserverClass
{
	public:
		CommanderPopulationObserverClass (int team,BuildingProductionKindType kind) :
			Team (team),Kind (kind),Counted (false) { }

		const char *	Get_Name (void) override	{ return "CommanderPopulationObserverClass"; }

		void	Attach (GameObject *) override	{ }
		void	Detach (GameObject *) override		{ GameObjObserverManager::Delete_Register (this); }

		void	Killed (GameObject *,GameObject *) override
		{
			if (Counted) { return; }
			Counted = true;
			int current = CommanderProductionService::Get_Population (Team,Kind);
			CommanderProductionService::Set_Population (Team,Kind,(current > 0) ? (current - 1) : 0);
		}

		void	Created (GameObject *) override							{ }
		void	Destroyed (GameObject *) override						{ }

		//	Feeds Section 37's guard/retaliate memory -- CommanderGuardAI::Report_Damage is a
		//	no-op for a unit that is not currently guarding, so this costs nothing for every
		//	Commander-produced unit that is not.  amount can be negative (repair rides the same
		//	Damaged() call with negative damage, per damageablegameobj.cpp), which is not an
		//	attack and is excluded.
		void	Damaged (GameObject * obj,GameObject * damager,float amount) override
		{
			if ((amount <= 0.0f) || (damager == nullptr) || (obj == nullptr)) { return; }
			Vector3 attacker_position;
			damager->Get_Position (&attacker_position);
			CommanderGuardAI::Report_Damage (obj->Get_ID (),damager->Get_ID (),attacker_position,
														 CommanderStateClass::Get_Estimated_Server_Time ());
		}

		void	Custom (GameObject *,int,intptr_t,GameObject *) override	{ }
		void	Sound_Heard (GameObject *,const CombatSound &) override		{ }
		void	Enemy_Seen (GameObject *,GameObject *) override				{ }
		void	Action_Complete (GameObject *,int,ActionCompleteReason) override	{ }
		void	Timer_Expired (GameObject *,int) override						{ }
		void	Animation_Complete (GameObject *,const char *) override		{ }
		void	Poked (GameObject *,GameObject *) override						{ }
		void	Entered (GameObject *,GameObject *) override					{ }
		void	Exited (GameObject *,GameObject *) override						{ }

	private:
		int									Team;
		BuildingProductionKindType	Kind;
		bool									Counted;
};


/*
**	Roadmap Section 45, Phase 39 -- the completion consumer CommanderProductionService's own
**	header says belongs to "the caller that knows how to build a SoldierGameObj or a
**	VehicleGameObj".  That caller owns no validation, no timing and no queue -- all of that
**	already happened -- it does exactly two things: create the canonical Renegade object
**	Section 45 requires ("normal AI SoldierGameObj paths... normal canonical VehicleGameObj"),
**	and place it somewhere that is not inside the facility's own collision.
**
**	Deliberately *not* routed through VehicleFactoryGameObj::Request_Vehicle: that call starts
**	the factory's own IsBusy timer and its own GeneratingVehicleID, which is the machinery a
**	human's PT purchase drives, and Section 45 keeps Commander production as its own
**	independent queue on purpose ("multiple facilities may have independent queues").  Feeding
**	an already-elapsed Commander item through a second timer would either double the wait or
**	collide with a human purchase in progress at the same building.  So this creates the unit
**	the same way SpawnerClass::Create_Spawned_Object does -- DefinitionMgrClass::Find_Definition
**	plus Create() -- which is the one production primitive every path in this codebase already
**	shares, Commander or human.
**
**	Placement has no semantic anchor to read: Resolve_Anchor_World_Transform needs a single
**	RenderObjClass model, and a level-placed factory is BuildingGameObj's legacy aggregate of
**	separately-placed statics with no model of its own (see building.h) -- see
**	BuildingGameObj::Get_Commander_Exit_Point, the one formula both this and the ExitClear
**	check in cCommanderProductionEvent::Act share.
*/
static void	Drain_Completed_Commander_Production (void)
{
	int count = CommanderProductionService::Get_Completed_Count ();
	for (int i = 0; i < count; i++) {

		CommanderProductionItem item;
		if (!CommanderProductionService::Get_Completed_Item (i,&item)) {
			continue;
		}

		//	PhysicalGameObj multiply-inherits As_BuildingGameObj from both DamageableGameObj and
		//	CombatPhysObserverClass, which is ambiguous called directly on a PhysicalGameObj*.
		//	Narrowing the pointer itself to the single DamageableGameObj base first (a plain
		//	upcast, not a virtual call) is what the rest of this codebase does to resolve it.
		PhysicalGameObj * facility_obj = GameObjManager::Find_PhysicalGameObj (item.FacilityId);
		DamageableGameObj * facility_damageable = (facility_obj != nullptr) ? (DamageableGameObj *)facility_obj : nullptr;
		BuildingGameObj * facility = (facility_damageable != nullptr) ? facility_damageable->As_BuildingGameObj () : nullptr;
		if (facility == nullptr) {
			//	The facility died between completion and this drain (On_Facility_Destroyed
			//	should already have cancelled it, but a destroyed-this-frame race is cheap to
			//	guard against here too) -- nothing to deliver into.
			continue;
		}

		DefinitionClass * def = DefinitionMgrClass::Find_Definition ((uint32)item.DefinitionId);
		PhysicalGameObjDef * physical_def = (def != nullptr) ? (PhysicalGameObjDef *)def : nullptr;
		if (physical_def == nullptr) {
			continue;
		}

		//	Same point Request_Production's own ExitClear check tested before this item was ever
		//	accepted into the queue -- one formula (BuildingGameObj::Get_Commander_Exit_Point),
		//	not a second copy of it here.
		Vector3 spawn_pos;
		facility->Get_Commander_Exit_Point (&spawn_pos,nullptr);

		PhysicalGameObj * unit = (PhysicalGameObj *)physical_def->Create ();
		if (unit == nullptr) {
			continue;
		}

		unit->Set_Transform (Matrix3D (spawn_pos));

		//	Counted the moment it exists, uncounted the moment CommanderPopulationObserverClass
		//	sees it die -- see that class's own comment for why Killed()/Detach() rather than
		//	Destroyed(), which nothing in this tree ever calls.
		unit->Add_Observer (new CommanderPopulationObserverClass (item.Team,item.Kind));
		CommanderProductionService::Set_Population (item.Team,item.Kind,
			CommanderProductionService::Get_Population (item.Team,item.Kind) + 1);

		unit->Start_Observers ();
	}

	CommanderProductionService::Clear_Completed ();
}


/*
**
*/
void 	CombatManager::Think()
{
	SyncTime += (int)((TimeManager::Get_Frame_Seconds() * 1000.0f) + 0.5f);

	WWPROFILENAMED( "CombatManager Think", top );

	IsGameplayPermitted=NetworkHandler->Is_Gameplay_Permitted();

{	WWPROFILE( "Input" );
	Handle_Input();
}
//	GroupControllerManager::Update();

	//
	//	Display debug boxes for the coordinate zones as necessary
	//
	if ( SoldierGameObj::Is_Ghost_Collision_Debug_Display_Enabled() ) {
		UnitCoordinationZoneMgr::Display_Debug_Boxes();
	}

{	WWPROFILE( "Bullets" );
	BulletManager::Update();
}

{	WWPROFILE( "Debris" );
	DebrisManager::Update();
}

{	WWPROFILE( "StatusOverlays" );
	StatusOverlaySystem::Update();
}

{	WWPROFILE( "WorldDebugDisplay" );
	WorldDebugDisplay::Update();
}

	ObjectiveManager::Update( TimeManager::Get_Frame_Seconds() );

	// Now, Process all objects logically
	ConversationMgrClass::Think();
{	WWPROFILE( "Game Obj Think" );
	GameObjManager::Think();

	// Now, Process all objects physically
}{	WWPROFILE( "Scene" );
  	COMBAT_SCENE->Update( TimeManager::Get_Frame_Seconds(), 0 );

}{	WWPROFILE( "Star" );
	Update_Star();

	// In normal mode, the camera must think before Post_Think, since the
	// camera update calls Set_Targeting on the star, which must feed Update_Animation
}{	WWPROFILE( "Camera 1" );
	//	Roadmap Section 41: while Commander Mode is active, CommanderModeController owns the
	//	camera and CCameraClass::Update is not called at all -- not touched, not degraded, just
	//	skipped for the duration.  star_is_alive here is the one fact CommanderModeController
	//	needs and does not look up itself; SoldierGameObj::Get_Defense_Object matches the
	//	dead-soldier check RadarManager/TacticalIntelligenceModel already use.
	if ( CommanderModeController::Is_Active() ) {
		bool star_is_alive = (COMBAT_STAR != nullptr) &&
									 (COMBAT_STAR->Get_Defense_Object() != nullptr) &&
									 (COMBAT_STAR->Get_Defense_Object()->Get_Health() > 0.0f);
		CommanderModeController::Update( TimeManager::Get_Frame_Seconds(),star_is_alive );
	} else if ( !MainCamera->Is_Using_Host_Model() ) {
		MainCamera->Update();
	}

	// Now, Post Process all objects logically
}{	WWPROFILE( "Post Think" );
	GameObjManager::Post_Think();
}
	// In host_model mode, the camera must think after Post_Think, so the host model has
	// a chance to determine where the camera should be
	if ( !CommanderModeController::Is_Active() && MainCamera->Is_Using_Host_Model() ) {
		MainCamera->Update();
	}

	// The targeting comes from the update weapons in the post_think
	Update_Star_Targeting();

	Do_Skeleton_Slider_Demo();

{	WWPROFILE( "Message Window" );
	MessageWindow->On_Frame_Update();
}

	SpawnManager::Update();

	/*
	**	The Commander/RTS layer, Sections 42-45.  Two halves, kept apart on purpose.
	**
	**	The authoritative half advances only on the server, and only on server time: construction
	**	completion and production timing are gameplay, and Section 44 is explicit that "server
	**	time/state owns completion, never client frame rate".  A client's own clock never reaches
	**	either service.
	**
	**	The presentation half runs wherever there is a scene to draw into, including on a listen
	**	server, and reads the services without writing to them.  A dedicated server reaches the
	**	first block and not the second, which is Section 47's rule about never initialising client
	**	rendering resources, satisfied by never calling into them.
	*/
	if ( CombatManager::I_Am_Server() ) {
		CommanderProductionService::Update( TimeManager::Get_Frame_Seconds() );
		Drain_Completed_Commander_Production();

		//	Roadmap Section 37 -- server-authoritative, like every other Commander AI decision;
		//	a client never sees a guarding unit do anything but move and fight, the same as any
		//	other AI it does not control.
		CommanderGuardAI::Update( CommanderStateClass::Get_Estimated_Server_Time(),
											TimeManager::Get_Frame_Seconds() );

		//	Roadmap Section 45.  A Commander who quit, timed out, crashed or switched team has to
		//	give the job up, or the team is locked out of its own production for the rest of the
		//	match with nothing able to elect a replacement.
		CommanderProductionService::Update_Commander_Vacancy( TimeManager::Get_Frame_Seconds() );
	}

	/*
	**	Construction advances on *both* sides, against the same clock.  On the server that clock
	**	is its own; on a client it is the server's, as last replicated and advanced locally since
	**	(CommanderStateClass::Get_Estimated_Server_Time).  That is what makes a client's build-up
	**	animation land on the same stage the server is at without a single block crossing the
	**	wire -- Section 44's "clients deterministically reconstruct blocks", extended to the
	**	timeline as well as the layout.
	**
	**	Section 44's completion activation runs between the Update and the Clear below.  It runs
	**	on both sides, not only the server: a client's own copy of the building is hidden by the
	**	same rule the server hid it by, so if only the server revealed it the building would
	**	finish building and stay invisible on every other machine.  Everything inside the
	**	activation that is a *decision* rather than a reveal is guarded server-side within
	**	RuntimeBuildingFactory itself.
	*/
	BuildingConstructionService::Update( CommanderStateClass::Get_Estimated_Server_Time() );

	{
		int completed_count = BuildingConstructionService::Get_Completed_Count();
		for ( int index = 0; index < completed_count; index ++ ) {
			RuntimeBuildingFactory::Activate_Completed_Building(
				BuildingConstructionService::Get_Completed_Building( index ) );
		}
	}

	BuildingConstructionService::Clear_Completed();

	if ( CombatManager::I_Am_Server() ) {
		//	Notices when anything the client mirror carries has moved, so no call site has to
		//	remember to announce a change it caused.
		CommanderStateClass::Server_Update();
	}

	//	Each of the three no-ops when it was initialised without a device, so a dedicated
	//	server runs these three calls and allocates nothing.
	CommanderGhostRenderer::Update();
	BuildingConstructionRenderer::Update();
	CommanderSidebarRenderer::Update();
	CommanderSelectionRenderer::Update();
	CommanderCommandPaletteRenderer::Update();

	//	Waypoint markers age on both sides, so a client whose expiry never arrived still drops the
	//	marker and a dedicated server's list does not grow for the whole match.
	CommanderWaypointSystem::Update( TimeManager::Get_Frame_Seconds() );
	CommanderWaypointRenderer::Update( TimeManager::Get_Frame_Seconds() );

{	WWPROFILE( "Sound Environment" );

	if ( SoundEnvironment != nullptr ) {
		SoundEnvironment->Update (COMBAT_SCENE, MainCamera);
	}
}

{	WWPROFILE( "Background" );

	BackgroundMgrClass::Update (COMBAT_SCENE, MainCamera);
}

{  WWPROFILE( "Weather" );

	WeatherMgrClass::Update (COMBAT_SCENE, MainCamera);
}

	HUDClass::Think();
	WeaponViewClass::Think();
	ScreenFadeManager::Think();

	//
	//	Last, so a per-frame subscriber sees a settled world rather than one
	//	halfway through its update.
	//
	GameEventBus::Raise_Think();
}

/*
**
*/
void CombatManager::Render()
{
	if ( COMBAT_STAR != nullptr ) {
		MultiplayRenderingAllowed = true;
	}

	if ( MultiplayRenderingAllowed ) {
		SystemInfoLog::Record_Frame();

		{
			WWPROFILE( "Camera Shakes" );
			COMBAT_SCENE->Apply_Camera_Shakes (*MainCamera);
		}

		DazzleRenderObjClass::Install_Dazzle_Visibility_Handler(&_TheCombatDazzleHandler);

		//	Roadmap Section 52A, Phase 57.  Every dazzle that renders this frame marks itself
		//	as a glare source as it goes; this opens the list they mark into.  A no-op while
		//	bloom is off, which is the default.
		PostProcessChainClass::Begin_Glare();

		{
			WWPROFILE( "Combat Render BG" );

			WW3D::Render (BackgroundScene, MainCamera);
		}

		{
			WWPROFILE( "Combat Render FG" );
			WW3D::Render(COMBAT_SCENE, MainCamera);
		}

		{
			WWPROFILE( "DazzleRenderer" );
			DazzleLayerClass * dlayer = COMBAT_DAZZLE_LAYER;
			if (dlayer != nullptr) {
				dlayer->Render(COMBAT_CAMERA);
			}
		}

		DazzleRenderObjClass::Install_Dazzle_Visibility_Handler(nullptr);

		//	Roadmap Section 52A, Phase 57.  After the dazzles have drawn, so the sources are
		//	this frame's, and before the HUD, so the interface does not bloom.
		{
			WWPROFILE( "Bloom" );
			PostProcessChainClass::Render_Bloom();
		}

		//	Roadmap Section 52A, Phase 55.  Off by default; see BackgroundMgrClass::Render_Sun_Shafts.
		BackgroundMgrClass::Render_Sun_Shafts();

		HUDClass::Render();

		ScreenFadeManager::Render();
	}
}

/*
**
*/
enum	{
	CHUNKID_THE_STAR							=	916991712,
	CHUNKID_FIRST_LOAD,
	CHUNKID_NOT_FIRST_LOAD,				// Delete this one, ( and change default to false )
	CHUNKID_VARIABLES,
	CHUNKID_CCAMERA,

	MICROCHUNKID_FIRST_LOAD					=	1,
	MICROCHUNKID_DIFFICULTY_LEVEL			=	2,
	MICROCHUNKID_SYNC_TIME					=	3,
	MICROCHUNKID_START_SCRIPT,
	MICROCHUNKID_RESPAWN_SCRIPT,
	MICROCHUNKID_RELOAD_COUNT,
	MICROCHUNKID_CHEAT_HISTORY,
	MICROCHUNKID_FIRST_PERSON,
};

bool	CombatManager::Save( ChunkSaveClass &csave )
{
	csave.Begin_Chunk( CHUNKID_THE_STAR );
	TheStar.Save( csave );
	csave.End_Chunk();

	csave.Begin_Chunk( CHUNKID_VARIABLES );
		bool	first_load = !Are_Observers_Active();
		WRITE_MICRO_CHUNK( csave, MICROCHUNKID_FIRST_LOAD, first_load );
		if ( !first_load ) {
			WRITE_MICRO_CHUNK( csave, MICROCHUNKID_DIFFICULTY_LEVEL, DifficultyLevel );
			WRITE_MICRO_CHUNK( csave, MICROCHUNKID_FIRST_PERSON, FirstPerson );
		}
		WRITE_MICRO_CHUNK( csave, MICROCHUNKID_SYNC_TIME, SyncTime );
		WRITE_MICRO_CHUNK_WWSTRING( csave, MICROCHUNKID_START_SCRIPT, StartScript );
		WRITE_MICRO_CHUNK_WWSTRING( csave, MICROCHUNKID_RESPAWN_SCRIPT, RespawnScript );
		WRITE_MICRO_CHUNK( csave, MICROCHUNKID_RELOAD_COUNT, ReloadCount );

	#ifndef PARAM_EDITING_ON
		int cheat_history = CheatMgrClass::Get_Instance()->Get_History();
		WRITE_MICRO_CHUNK( csave, MICROCHUNKID_CHEAT_HISTORY, cheat_history );
	#endif //PARAM_EDITING_ON

	csave.End_Chunk();

	if ( COMBAT_CAMERA != nullptr ) {
		csave.Begin_Chunk( CHUNKID_CCAMERA );
			COMBAT_CAMERA->Save( csave );
		csave.End_Chunk();
	}

	return true;
}

bool	CombatManager::Load( ChunkLoadClass &cload )
{
	ReloadCount = 0;		// Legacy
	IsFirstLoad = true;		// Default
	int legacy_dificulty_level = DifficultyLevel;
	int cheat_history = 0;
	FirstPerson = FirstPersonDefault;

	while (cload.Open_Chunk()) {
		switch(cload.Cur_Chunk_ID()) {

			case CHUNKID_THE_STAR:
				TheStar.Load( cload );
				break;

			case CHUNKID_VARIABLES:
				while (cload.Open_Micro_Chunk()) {
					switch(cload.Cur_Micro_Chunk_ID()) {
						READ_MICRO_CHUNK( cload, MICROCHUNKID_FIRST_LOAD, IsFirstLoad );
						READ_MICRO_CHUNK( cload, MICROCHUNKID_DIFFICULTY_LEVEL, DifficultyLevel );
						READ_MICRO_CHUNK( cload, MICROCHUNKID_SYNC_TIME, SyncTime );
						READ_MICRO_CHUNK_WWSTRING( cload, MICROCHUNKID_START_SCRIPT, StartScript );
						READ_MICRO_CHUNK_WWSTRING( cload, MICROCHUNKID_RESPAWN_SCRIPT, RespawnScript );
						READ_MICRO_CHUNK( cload, MICROCHUNKID_RELOAD_COUNT, ReloadCount );
						READ_MICRO_CHUNK( cload, MICROCHUNKID_CHEAT_HISTORY, cheat_history );
						READ_MICRO_CHUNK( cload, MICROCHUNKID_FIRST_PERSON, FirstPerson );
						default:
							Debug_Say(( "Unrecognized CombatManager variable chunkID\n" ));
							break;

					}
					cload.Close_Micro_Chunk();
				}
				break;

			case CHUNKID_CCAMERA:
				if ( COMBAT_CAMERA != nullptr ) {
					COMBAT_CAMERA->Load( cload );
				}
				break;

			default:
				Debug_Say(( "Unrecognized CombatManager chunkID\n" ));
				break;

		}
		cload.Close_Chunk();
	}

	if ( IsFirstLoad ) {
		DifficultyLevel = legacy_dificulty_level;
		ReloadCount = 0;
		CheatMgrClass::Get_Instance()->Reset_History();
	} else {
		ReloadCount++;		// Count another load!
		CheatMgrClass::Get_Instance()->Set_History( cheat_history );
		CheatMgrClass::Get_Instance()->Update_History();
	}

	return true;
}

/*
**
*/
bool CombatManager::Can_Damage(ArmedGameObj * p_armed_damager, PhysicalGameObj * p_phys_victim)
{
	if (NetworkHandler != nullptr) {
		return NetworkHandler->Can_Damage(p_armed_damager, p_phys_victim);
	} else {
		return true;
	}
}

float CombatManager::Get_Damage_Factor(ArmedGameObj * p_armed_damager, PhysicalGameObj * p_phys_victim)
{
	if (NetworkHandler != nullptr) {
		return NetworkHandler->Get_Damage_Factor(p_armed_damager, p_phys_victim);
	} else {
		return 1.0f;
	}
}

void CombatManager::On_Soldier_Kill(SoldierGameObj * p_soldier, SoldierGameObj * p_victim)
{
	WWASSERT(p_soldier != nullptr);
	WWASSERT(p_victim != nullptr);

	if (NetworkHandler != nullptr) {
		 NetworkHandler->On_Soldier_Kill(p_soldier, p_victim);
	}
}

void CombatManager::On_Soldier_Death(SoldierGameObj * p_soldier)
{
	WWASSERT(p_soldier != nullptr);

	if (NetworkHandler != nullptr) {
		 NetworkHandler->On_Soldier_Death(p_soldier);
	}
}

bool CombatManager::Is_Gameplay_Permitted(void)
{
	if (NetworkHandler == nullptr) {
		return true;
	} else {
		return IsGameplayPermitted;
//		return NetworkHandler->Is_Gameplay_Permitted();
	}
}

/*
** Misc Handler
*/
void	CombatManager::Mission_Complete( bool success )
{
	if (MiscHandler != nullptr) {
		MiscHandler->Mission_Complete( success );
	}
}

void	CombatManager::Star_Killed( void )
{
	if (MiscHandler != nullptr) {
		MiscHandler->Star_Killed();
	}
}

/*
**
*/
void	CombatManager::Set_Camera_Profile( const char * profile_name )
{
	if ( profile_name == nullptr ) {
		COMBAT_CAMERA->Use_Default_Profile();
	} else {
		COMBAT_CAMERA->Use_Profile( profile_name );
	}
}

/*
**
*/
void 	CombatManager::Set_The_Star( SoldierGameObj *target, bool is_star_determining_target )
{
	if ( 	TheStar != target ) {
		HUDClass::Reset();	// Reset the HUD (clear damage and powerups)
	}

	TheStar = target;
	IsStarDeterminingTarget = is_star_determining_target;
	if ( target != nullptr ) {
		COMBAT_CAMERA->Force_Heading( target->Get_Facing() );
	}

	HUDClass::Force_Weapon_Chart_Update();
	WeaponViewClass::Reset();

	if (IsLevelInitialized == false) {
		IsLevelInitialized = true;

		//
		//	Turn music and sound effects back on...
		//
		WWAudioClass::Get_Instance ()->Enable_New_Sounds (true);
	}
}

/*
**
*/
void CombatManager::Update_Star( void )
{
	SoldierGameObj * star = COMBAT_STAR;
	if ( star == nullptr ) {
		return;
	}

	if ( star->Is_Destroyed() ) {
		Set_Combat_Mode( COMBAT_MODE_CORPSE );
	} else if ( star->Is_Dead() ) {
		Set_Combat_Mode( COMBAT_MODE_DIEING );
	} else if ( star->Get_Profile_Vehicle() != nullptr ) {
		Set_Combat_Mode( COMBAT_MODE_IN_VEHICLE );
	} else if ( star->Is_Sniping() ) {
		Set_Combat_Mode( COMBAT_MODE_SNIPING );
	} else if ( star->Use_Ladder_View() ) {
		Set_Combat_Mode( COMBAT_MODE_ON_LADDER );
	} else if ( Is_First_Person() ) {
		Set_Combat_Mode( COMBAT_MODE_FIRST_PERSON );
	} else {
		Set_Combat_Mode( COMBAT_MODE_THIRD_PERSON );
	}

	Update_Combat_Mode();

	//
	//	Update the map
	//
	Vector3 star_pos;
	star->Get_Position (&star_pos);
	MapMgrClass::Clear_Cloud_Cell (star_pos);
	return ;
}

void CombatManager::Update_Star_Targeting( void )
{
	WWPROFILENAMED( "Targeting", top );
	SmartGameObj * star = nullptr;

	if ( COMBAT_STAR != nullptr ) {
		SoldierGameObj * soldier = COMBAT_STAR;

	   WWASSERT(soldier != nullptr);
		star = soldier;
		if ( soldier->Get_Vehicle() ) {
			star = soldier->Get_Vehicle();
		}

		// Now, if the star has a weapon, gather the information needing to be displayed
		// such as where the bullets will hit, what we are targeting, distance, etc.
		HUDInfo::Set_Weapon_Target_Object( nullptr );
		WeaponClass * weapon = star->Get_Weapon();
		if ( weapon ) {
			WWPROFILE( "Display_Targeting" );
			weapon->Display_Targeting();
		}
	}
}

bool CombatManager::Is_In_Camera_Frustrum(Vector3 & position)
{
	WWASSERT(COMBAT_CAMERA != nullptr);
   Vector3 projected_mid_point;
	bool is_visible = (COMBAT_CAMERA->Project(projected_mid_point, position) == CameraClass::INSIDE_FRUSTUM);
	return is_visible;
}

void	CombatManager::Do_Skeleton_Slider_Demo( void )
{
	if (( EnableSkeletonSliderDemo == false ) ||
		 ( COMBAT_CAMERA->Is_2D_Targeting() == false )) {
		return;
	}

	if ( COMBAT_STAR != nullptr ) {
		Vector2 sliders = COMBAT_CAMERA->Get_Camera_Target_2D_Offset();
		// Slide is 0-1, height * widht should be -scale - scale
		sliders.X = (sliders.X - 0.5f) * 2;
		sliders.Y = (sliders.Y - 0.5f) * -2;
		float scale = 1;
		float height = sliders.Y * scale;
		float width = sliders.X * scale;
//		Debug_Say(( "Height %f, width %f\n", height, width ));
		COMBAT_STAR->Adjust_Skeleton( height, width );
	}
}


/*
**
*/
void	Vehicle_Panic( VehicleGameObj * vehicle )
{
	VehiclePhysClass * rbody = vehicle->Peek_Vehicle_Phys();
	if (rbody != nullptr) {
#if 1
		// Try to put the vehicle up-right in its current position
		Matrix3D tm = rbody->Get_Transform();
		Vector3 position;
		Vector3 forward;

		tm.Get_Translation(&position);
		tm.Get_X_Vector(&forward);
		if (forward.Z > 0.707f) {
			tm.Get_Z_Vector(&forward);
		}

		forward.Z = 0.0f;
		position.Z += 3.0f;
		Matrix3D new_tm;
		new_tm.Obj_Look_At(position,position+forward,0.0f);

		rbody->Set_Transform(new_tm);
#endif
	}
}

/*
** Combat Mode Management
*/
void	CombatManager::Set_Combat_Mode( int mode )
{
	// I don't know how to handle loading a saved game in cinematics, so postpone
	// this mode change until the camera is not host controlled?
	if ( COMBAT_CAMERA && COMBAT_CAMERA->Is_Using_Host_Model() ) {
		return;
	}

	if ( mode != CombatMode ) {
		//Debug_Say(( "Switching CombatMode to %d\n", mode ));

		// When changing to or from ladder, lerp
		if ( ( CombatMode == COMBAT_MODE_ON_LADDER ) || ( mode == COMBAT_MODE_ON_LADDER ) ) {
			COMBAT_CAMERA->Set_Lerp_Time( 0.5f );
		}

		CombatMode = mode;

		// Pause game in single player corpse mode
//		IsGamePaused = (CombatMode == COMBAT_MODE_CORPSE) && IS_SOLOPLAY;

		SoldierGameObj * star = COMBAT_STAR;

		switch ( CombatMode ) {

			case COMBAT_MODE_NONE:
				break;

			case COMBAT_MODE_FIRST_PERSON:
				COMBAT_CAMERA->Set_Is_Star_Sniping( false );
				COMBAT_CAMERA->Use_Profile( "First_Person" );
				COMBAT_CAMERA->Set_Profile_Distance( 0 );
				star->Peek_Model()->Set_Hidden( true );
				WeaponViewClass::Enable( true );
				break;

			case COMBAT_MODE_ON_LADDER:
			case COMBAT_MODE_THIRD_PERSON:
				star->Peek_Model()->Set_Hidden( false );
				COMBAT_CAMERA->Set_Is_Star_Sniping( false );
				COMBAT_CAMERA->Use_Default_Profile();
				WeaponViewClass::Enable( false );
				break;

			case COMBAT_MODE_SNIPING:
				COMBAT_CAMERA->Set_Is_Star_Sniping( true );
				star->Peek_Model()->Set_Hidden( true );
				COMBAT_CAMERA->Use_Profile( "Sniper" );
				COMBAT_CAMERA->Set_Profile_Height( star->Get_Weapon_Height() );
				COMBAT_CAMERA->Set_Profile_Distance( -star->Get_Weapon_Length() );
				COMBAT_CAMERA->Enable_2D_Targeting( false );
				WeaponViewClass::Enable( false );
				break;

			case COMBAT_MODE_IN_VEHICLE:
			{
				VehicleGameObj * vehicle = star->Get_Profile_Vehicle();
				WWASSERT( vehicle );
	 			COMBAT_CAMERA->Set_Is_Star_Sniping( false );
				if ( vehicle->Get_Profile() != nullptr ) {
					COMBAT_CAMERA->Use_Profile( vehicle->Get_Profile() );
				} else {
					COMBAT_CAMERA->Use_Default_Profile();
				}
				WeaponViewClass::Enable( false );
			}
				break;

			case COMBAT_MODE_DIEING:
				star->Peek_Model()->Set_Hidden( false );
				COMBAT_CAMERA->Set_Is_Star_Sniping( false );
				COMBAT_CAMERA->Use_Profile( "Death" );
				COMBAT_CAMERA->Set_Lerp_Time( 0.5f );
				WeaponViewClass::Enable( false );
				break;

			case COMBAT_MODE_CORPSE:
				break;

			case COMBAT_MODE_SNAP_SHOT:
				break;

			default:
				Debug_Say(( "Bad CombatMode %d\n", CombatMode ));
				CombatMode = COMBAT_MODE_NONE;
				break;

		}
	}
}

void	CombatManager::Update_Combat_Mode( void )
{
	SoldierGameObj * star = COMBAT_STAR;
	Vector3	pos;
	star->Get_Position( &pos );
	COMBAT_CAMERA->Set_Anchor_Position( pos );

	//XXX
	//COMBAT_CAMERA->Enable_2D_Targeting(true);

	switch ( CombatMode ) {

		case COMBAT_MODE_NONE:
			break;

		case COMBAT_MODE_FIRST_PERSON:
			star->Peek_Model()->Set_Hidden( true );	// Putting it here, because vehicles are turning it back on
			COMBAT_CAMERA->Set_Profile_Height( star->Get_Weapon_Height() );	// This is every frame for crouching
//			COMBAT_CAMERA->Enable_2D_Targeting( Input::Get_State(INPUT_FUNCTION_CURSOR_TARGETING) );
			COMBAT_CAMERA->Enable_2D_Targeting( false );
			break;

		case COMBAT_MODE_ON_LADDER:
		case COMBAT_MODE_THIRD_PERSON:
			COMBAT_CAMERA->Enable_2D_Targeting( Input::Get_State(INPUT_FUNCTION_CURSOR_TARGETING ) );

			if ( star->Is_On_Ladder() ) {
				COMBAT_CAMERA->Force_Heading( star->Get_Facing() );
			}
			break;

		case COMBAT_MODE_SNIPING:
			COMBAT_CAMERA->Set_Profile_Height( star->Get_Weapon_Height() );	// This is every frame for crouching
			break;

		case COMBAT_MODE_IN_VEHICLE:
		{
			VehicleGameObj * vehicle = star->Get_Profile_Vehicle();
			WWASSERT( vehicle );

#ifdef WWDEBUG
			if ( Input::Get_State( INPUT_FUNCTION_PANIC ) ) {
				Vehicle_Panic( vehicle );
			}
#endif

			Vector3	vehicle_pos;
			vehicle->Get_Position( &vehicle_pos );
			COMBAT_CAMERA->Set_Anchor_Position( vehicle_pos );

//			bool target_2d = ( !vehicle->Use_2D_Aiming() ^ Input::Get_State(INPUT_FUNCTION_CURSOR_TARGETING) ^ VehicleGameObj::Is_Target_Steering() );

			bool target_2d = (Input::Get_State(INPUT_FUNCTION_CURSOR_TARGETING) ^ VehicleGameObj::Is_Target_Steering() );

			// This can be used to always for the V key down for vehicles
			if ( VehicleGameObj::Get_Camera_Locked_To_Turret() ) {
				target_2d = !target_2d;
			}

			if ( !vehicle->Use_2D_Aiming() ) {
				target_2d = true;
			}

			if ( vehicle->Get_Gunner() == COMBAT_STAR ) {
				target_2d = true;
			}

			if ( target_2d ) {
				COMBAT_CAMERA->Enable_2D_Targeting( false );
			} else {
				COMBAT_CAMERA->Enable_2D_Targeting( true );

	// Lateral velocity offset camera code
				float desired_heading = vehicle->Get_Facing();;
				VehiclePhysClass * vehiclephys = vehicle->Peek_Physical_Object()->As_VehiclePhysClass();
				if (vehiclephys != nullptr) {
					Vector3 vel,localvel;
					vehiclephys->Get_Velocity(&vel);

					Matrix3D invtm;
					vehiclephys->Get_Transform().Get_Orthogonal_Inverse(invtm);
					Matrix3D::Rotate_Vector(invtm,vel,&localvel);

	//				float lateralvel = localvel.Y;
					float heading_offset = WWMath::Atan2(localvel.Y,localvel.X);
					heading_offset = WWMath::Clamp(heading_offset,DEG_TO_RADF(-45.0f),DEG_TO_RADF(45.0f));

					float offset_scale = localvel.X / 5.0f;
					offset_scale = WWMath::Clamp(offset_scale,0.0f,1.0f);
					heading_offset *= offset_scale;

					desired_heading += heading_offset;
				}
				COMBAT_CAMERA->Force_Heading( desired_heading );
				COMBAT_CAMERA->Set_Tilt( 0 );
			}
		}
			break;

		case COMBAT_MODE_DIEING:

			// Make the camera look towards the star killer
			if ( StarKillerID != 0 ) {
				PhysicalGameObj * killer = GameObjManager::Find_PhysicalGameObj( StarKillerID );
				if ( killer ) {
					Vector3 star_pos;
					Vector3 killer_pos;
					COMBAT_STAR->Get_Position( &star_pos );
					killer->Get_Position( &killer_pos );

					Vector3 dif = killer_pos - star_pos;
					float heading = WWMath::Atan2(dif.Y,dif.X);
					COMBAT_CAMERA->Force_Heading( heading );
				}
			}

			break;

		case COMBAT_MODE_CORPSE:
			break;

		case COMBAT_MODE_SNAP_SHOT:
			break;

	}
}

void	CombatManager::Register_Star_Killer( ArmedGameObj * killer )
{
	if ( killer != nullptr ) {
		StarKillerID = killer->Get_ID();
	} else {
		StarKillerID = 0;
	}
}





