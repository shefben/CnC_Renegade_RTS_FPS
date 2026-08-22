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
 *                     $Archive:: /Commando/Code/Combat/combat.h                             $*
 *                                                                                             *
 *                      $Author:: Bhayes                                                      $*
 *                                                                                             *
 *                     $Modtime:: 2/17/02 10:27a                                              $*
 *                                                                                             *
 *                    $Revision:: 130                                                        $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef	COMBAT_H
#define	COMBAT_H

#ifndef	ALWAYS_H
	#include "always.h"
#endif

#ifndef	GAMEOBJREF_H
	#include "gameobjref.h"
#endif

#ifndef	VECTOR3_H
	#include "vector3.h"
#endif

#ifndef	WWSTRING_H
	#include "wwstring.h"
#endif


/*
**
*/
class PhysicsSceneClass;
class CameraClass;
class CCameraClass;
class	SmartGameObj;
class	ArmedGameObj;
class	PhysicalGameObj;
class	SimpleSceneClass;
class BaseGameObj;
class VehicleGameObj;
class SoldierGameObj;
class DazzleLayerClass;
class MessageWindowClass;
class SoundEnvironmentClass;


/*
**
*/
#define	COMBAT_CAMERA				   CombatManager::Get_Camera()
#define	COMBAT_STAR						CombatManager::Get_The_Star()
#define	COMBAT_SCENE					CombatManager::Get_Scene()
#define	COMBAT_DAZZLE_LAYER			CombatManager::Get_Dazzle_Layer()

/*
** Collision Groups
*/
typedef enum {
	DEFAULT_COLLISION_GROUP			= 0,		// collides with everything
	UNCOLLIDEABLE_GROUP,							// collides with nothing
	TERRAIN_ONLY_COLLISION_GROUP,				// collides only with terrain
	BULLET_COLLISION_GROUP,						// collides with everything but itself
	TERRAIN_AND_BULLET_COLLISION_GROUP,		// collides with terrain and bullets
	BULLET_ONLY_COLLISION_GROUP,				// collides only with bullets
	SOLDIER_COLLISION_GROUP,					// collides with everything (but only soldiers use it)
	SOLDIER_GHOST_COLLISION_GROUP,			// collides with everything but soldiers

	//
	//	Groups 8-14.  Terrain owns 15 and cannot move.
	//
	C4_COLLISION_GROUP,							// like DEFAULT, but also hits SOLDIER_GHOST, and never WATER_EDGE or UNDERGROUND_TRANSITION
	UNDERGROUND_COLLISION_GROUP,				// collides with TERRAIN, itself and UNDERGROUND_TRANSITION
	SOLDIER_ONLY_COLLISION_GROUP,				// collides only with SOLDIER and SOLDIER_GHOST
	SOLDIER_BULLET_COLLISION_GROUP,			// collides with C4, BULLET, SOLDIER and SOLDIER_GHOST

	TERRAIN_COLLISION_GROUP			= 15,		// Terrain must be 15

	//
	//	Groups 16-31.  Water, amphibious and rail behavior: each of these is
	//	deliberately narrow, so the permissive groups above are disabled against
	//	them and the pairs that matter are turned back on one at a time.
	//
	WATER_SURFACE_COLLISION_GROUP,			// collides with NAVAL_UNIT, BEACHING_UNIT, HOVER_UNIT, AMPHIBIOUS_UNIT and BULLET
	WATER_EDGE_COLLISION_GROUP,				// collides with DEFAULT, SOLDIER and NAVAL_UNIT
	WATER_EDGE_ALT_COLLISION_GROUP,			// collides only with NAVAL_UNIT
	BEACH_EDGE_COLLISION_GROUP,				// collides only with BEACHING_UNIT
	NAVAL_UNIT_COLLISION_GROUP,				// like DEFAULT, but also hits WATER_SURFACE and WATER_EDGE
	BEACHING_UNIT_COLLISION_GROUP,			// like DEFAULT, but also hits WATER_SURFACE and BEACH_EDGE
	HOVER_UNIT_COLLISION_GROUP,				// like DEFAULT, but also hits WATER_SURFACE
	AMPHIBIOUS_UNIT_COLLISION_GROUP,			// like DEFAULT, but also hits AMPHIBIOUS_UNIT_FLOOR
	AMPHIBIOUS_UNIT_FLOOR_COLLISION_GROUP,	// collides only with AMPHIBIOUS_UNIT
	UNDERGROUND_TRANSITION_COLLISION_GROUP,// like DEFAULT, but also hits UNDERGROUND, and never C4
	DEFAULT_AND_SOLDIER_ONLY_COLLISION_GROUP,	// collides only with DEFAULT, SOLDIER and SOLDIER_GHOST
	PLAYER_BUILDING_COLLISION_GROUP,			// part TERRAIN, part DEFAULT, but never PLAYER_BUILDING_GHOST
	PLAYER_BUILDING_GHOST_COLLISION_GROUP,	// like DEFAULT, but never PLAYER_BUILDING
	TRAIN_COLLISION_GROUP,						// collides with TRAIN_TRACK, DEFAULT, SOLDIER, BULLET, C4 and the water-borne unit groups
	TRAIN_TRACK_COLLISION_GROUP,				// collides with itself and TRAIN

	COLLISION_GROUP_COUNT						// not a group
} Collision_Group_Type;

//
//	Friendly name for a collision group, for debug output and the tools.  Keep
//	this in step with the enum above.
//
const char *	Get_Collision_Group_Name( Collision_Group_Type group );

/*
**
*/
class	CombatNetworkHandlerClass {
public:
   virtual	bool	Can_Damage(ArmedGameObj * p_armed_damager, PhysicalGameObj * p_phys_victim) = 0;
   virtual	float	Get_Damage_Factor(ArmedGameObj * p_armed_damager, PhysicalGameObj * p_phys_victim) = 0;
	virtual	void	On_Soldier_Kill(SoldierGameObj * p_soldier, SoldierGameObj * p_victim)						= 0;
	virtual	void	On_Soldier_Death(SoldierGameObj * p_soldier)						= 0;
	virtual	bool	Is_Gameplay_Permitted(void)											= 0;
};


/*
**
*/
class	CombatMiscHandlerClass {
public:
	virtual	void	Mission_Complete( bool success )												= 0;
	virtual	void	Star_Killed( void )																= 0;
};

/*
**
*/
class	CombatManager {

public:
	// Init and Shutdown should be called once per program execution
	static	void	Init( bool render_available = true );
	static	void 	Shutdown( void );

	// Scene_Init is called by the game, but not the editor
	static	void	Scene_Init( void );

	// Level Loading functions get called for each level loaded.
	static	void	Pre_Load_Level( bool render_available = true );
	static	void	Load_Level_Threaded( const char * map_name, bool preload_assets );
	static	bool	Is_Load_Level_Complete( void );
	static	bool	Is_Loading_Level( void );

	static	void	Post_Load_Level( void );

	static	void	Unload_Level( void );

	// Main Loop functions
	static	void 	Generate_Control( void );
	static	void 	Think( void );
	static	void 	Render( void );

	static	void 	Handle_Input( void );

	// Save/Load
	static	bool	Save( ChunkSaveClass &csave );
	static	bool	Load( ChunkLoadClass &cload );

	// Client/Server Settings
	static	void	Set_I_Am_Server( bool yes )		{ IAmServer = yes; }
	static	void	Set_I_Am_Client( bool yes )		{ IAmClient = yes; }
	static	bool	I_Am_Server( void )					{ return IAmServer; }
	static	bool	I_Am_Client( void )					{ return IAmClient; }
	static	bool	I_Am_Only_Client( void )			{ return IAmClient && !IAmServer; }
	static	bool	I_Am_Only_Server( void )			{ return IAmServer && !IAmClient; }
   static	bool	I_Am_Client_Server(void)			{ return IAmClient && IAmServer;}

	static	void	Set_Friendly_Fire_Permitted( bool yes )	{ FriendlyFirePermitted = yes; }
	static	bool	Is_Friendly_Fire_Permitted( void )			{ return FriendlyFirePermitted; }

	static	void	Set_Beacon_Placement_Ends_Game( bool yes ){ BeaconPlacementEndsGame = yes; }
	static	bool	Does_Beacon_Placement_Ends_Game( void )	{ return BeaconPlacementEndsGame; }

	static	void	Set_My_Id( int id )					{ MyId = id; }
	static	int	Get_My_Id( void )						{ return MyId; }

	// Latency support
	static	void	Set_Last_Round_Trip_Ping_Ms( DWORD ping )		{ LastRoundTripPingMs = ping; }
	static	DWORD	Get_Last_Round_Trip_Ping_Ms( void )				{ return LastRoundTripPingMs; }
	static	void	Set_Avg_Round_Trip_Ping_Ms( DWORD ping )		{ AvgRoundTripPingMs = ping; }
	static	DWORD	Get_Avg_Round_Trip_Ping_Ms( void )				{ return AvgRoundTripPingMs; }

	// Network Handler Functions
	static	void	Set_Combat_Network_Handler( CombatNetworkHandlerClass * handler )	{ NetworkHandler = handler; }

	static	bool	Can_Damage(ArmedGameObj * p_armed_damager, PhysicalGameObj * p_phys_victim);
	static	float	Get_Damage_Factor(ArmedGameObj * p_armed_damager, PhysicalGameObj * p_phys_victim);
	static	void	On_Soldier_Kill(SoldierGameObj * p_soldier, SoldierGameObj * p_victim);
	static	void	On_Soldier_Death(SoldierGameObj * p_soldier);
	static	bool	Is_Gameplay_Permitted(void);

	// Misc Handler
	static	void	Set_Combat_Misc_Handler( CombatMiscHandlerClass * handler )	{ MiscHandler = handler; }
	static	void	Mission_Complete( bool success );
	static	void	Star_Killed( void );

	// The Star
	static	void 	Set_The_Star( SoldierGameObj *target, bool is_star_determining_target = true );
	static	SoldierGameObj * Get_The_Star( void )		{ return (SoldierGameObj *)TheStar.Get_Ptr(); }
	static	void	Update_Star( void );
	static	void	Update_Star_Targeting( void );

	static	bool	Is_Star_Determining_Target(void)					{return IsStarDeterminingTarget;}
	static	void	Set_Is_Star_Determining_Target(bool flag)		{IsStarDeterminingTarget = flag;}

	// The Scene
	static	PhysicsSceneClass			*Get_Scene( void );
	static	SimpleSceneClass			*Get_Background_Scene( void )		{ return BackgroundScene; }
	static	CCameraClass				*Get_Camera( void )					{ return MainCamera; }
	static   SoundEnvironmentClass	*Get_Sound_Environment( void )	{ return SoundEnvironment;	}

	static	void	Set_Camera_Profile( const char * profile_name );
	static	void	Set_Camera_Vehicle( VehicleGameObj * obj, int seat = 0 );

	static	bool	Is_In_Camera_Frustrum(Vector3 & position);

	static	bool	Are_Observers_Active( void )			{ return AreObserversActive; }
	static	void	Set_Observers_Active( bool onoff )	{ AreObserversActive = onoff; }
	static	bool	Is_First_Load( void )					{ return IsFirstLoad; }
	static	void	Set_First_Load( bool onoff )			{ IsFirstLoad = onoff; }

	static	void	Do_Skeleton_Slider_Demo( void );
	static	void	Toggle_Skeleton_Slider_Demo( void )			{ EnableSkeletonSliderDemo = !EnableSkeletonSliderDemo; }
	static	bool	Is_Skeleton_Slider_Demo_Enabled( void )	{ return EnableSkeletonSliderDemo; }

	static	DazzleLayerClass * Get_Dazzle_Layer(void)			{ return DazzleLayer; }

	// First Person
	static	void	Set_First_Person( bool on_off )						{ FirstPerson = on_off; }
	static	bool	Is_First_Person( void )								{ return FirstPerson; }
	static	void	Set_First_Person_Default( bool on_off )				{ FirstPersonDefault = on_off; }
	static	bool	Get_First_Person_Default( void )					{ return FirstPersonDefault; }

	// Difficulty
	static	void	Set_Difficulty_Level( int level )		{ DifficultyLevel = level; }
	static	int	Get_Difficulty_Level( void )				{ return DifficultyLevel; }

	static	bool	Are_Transitions_Automatic( void )			{ return AutoTransitions; }
	static	void	Set_Transitions_Automatic( bool state )	{ AutoTransitions = state; }

	// Message window
	static	MessageWindowClass *	Get_Message_Window (void) { return MessageWindow; }

	static	void	Show_Star_Damage_Direction( int direction )		{ StarDamageDirection |= 1 << (direction&7); }
	static	int	Get_Star_Damage_Direction( void )					{ return StarDamageDirection; }
	static	void	Clear_Star_Damage_Direction( void )					{ StarDamageDirection = 0; }

	static	int	Get_Sync_Time( void )									{ return SyncTime; }
	static	bool	Is_Game_Paused( void )									{ return IsGamePaused; }


	static	void	Register_Star_Killer( ArmedGameObj * killer );

	static	const char *	Get_Start_Script( void )					{ return StartScript; }
	static	const char *	Get_Respawn_Script( void )					{ return RespawnScript; }
	static	void			Set_Start_Script( const char * string )		{ StartScript = string; }
	static	void			Set_Respawn_Script( const char * string )	{ RespawnScript = string; }

	static	void			Request_Autosave( bool state = true )		{ AutosaveRequested = state; }
	static	bool			Is_Autosave_Requested( void )					{ return AutosaveRequested; }

	static	int				Get_Reload_Count( void )						{ return ReloadCount; }

	static	bool			Is_Hit_Reticle_Enabled( void )				{ return HitReticleEnabled; }
	static	void			Toggle_Hit_Reticle_Enabled( void )			{ HitReticleEnabled = !HitReticleEnabled; }

	static	const char * Get_Last_LSD_Name( void )						{ return LastLSDName; };
	static	void			 Set_Last_LSD_Name( const char * name )	{ LastLSDName = name; };

	static	int			 Get_Load_Progress( void )						{ return LoadProgress; };
	static	void			 Inc_Load_Progress( void )						{ LoadProgress++; };
	static	void			 Set_Load_Progress( int set )					{ LoadProgress = set; };

private:
	static	bool		IAmServer;
	static	bool		IAmClient;
	static	bool		IAmOnlyClient;
	static	bool		IAmOnlyServer;
	static	int		MyId;
	static	int		SyncTime;
	static	bool		IsGamePaused;
	static	bool		IsLevelInitialized;
	static	bool		FriendlyFirePermitted;
	static	bool		BeaconPlacementEndsGame;

	static	bool		FirstPerson;
	static	bool		FirstPersonDefault;

	static	CCameraClass				*	MainCamera;
	static	SimpleSceneClass			*	BackgroundScene;

	static   SoundEnvironmentClass	*  SoundEnvironment;

	static	DazzleLayerClass			*	DazzleLayer;
	static	MessageWindowClass		*	MessageWindow;

	static	GameObjReference				TheStar;

	static	bool		IsStarDeterminingTarget;
	static	bool		IsFirstLoad;
	static	bool		AreObserversActive;
	static	bool		EnableSkeletonSliderDemo;

	static	int		DifficultyLevel;
	static	bool		AutoTransitions;

	static	int		StarDamageDirection;
	static	int		StarKillerID;

	static	CombatNetworkHandlerClass *	NetworkHandler;
	static	CombatMiscHandlerClass *		MiscHandler;

	static	StringClass		StartScript;
	static	StringClass		RespawnScript;

	static	int				ReloadCount;
	static	bool			HitReticleEnabled;

	static	bool	IsGameplayPermitted;

	/*
	** Combat Mode Management
	*/
	enum {
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
	static	int	CombatMode;

	static	bool	AutosaveRequested;

	static	void	Set_Combat_Mode( int mode );
	static	void	Update_Combat_Mode( void );

	/*
	** Latency Support
	*/
	static DWORD	LastRoundTripPingMs;
	static DWORD	AvgRoundTripPingMs;

	static StringClass	LastLSDName;

	static	int	LoadProgress;

	static	bool	MultiplayRenderingAllowed;

};

#endif
