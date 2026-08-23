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
 *                     $Archive:: /Commando/Code/Combat/scriptcommands.h                      $*
 *                                                                                             *
 *                      $Author:: Patrick                                                     $*
 *                                                                                             *
 *                     $Modtime:: 1/09/02 12:09p                                              $*
 *                                                                                             *
 *                    $Revision:: 211                                                         $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef	SCRIPTCOMMANDS_H
#define	SCRIPTCOMMANDS_H

#ifndef	VECTOR3_H
	#include "vector3.h"
	#include "wwstring.h"
#include "widestring.h"
#include "matrix3d.h"
#endif

#ifndef	COMBATSOUND_H
	#include "combatsound.h"
#endif

#ifndef	ACTIONPARAMS_H
	#include "actionparams.h"
#endif


/*
** Types
*/
class		ScriptableGameObj;
typedef	ScriptableGameObj	GameObject;
class		AudibleSoundClass;
typedef	AudibleSoundClass	Sound2D;
class		Sound3DClass;
typedef	Sound3DClass		Sound3D;
class		Matrix3D;
class		ScriptClass;
class		ScriptSaver;
class		ScriptLoader;


/*
** Script Commands
*/


/*
** MISC Script ENUMS
*/
enum {
		OBJECTIVE_TYPE_PRIMARY 				= 1,
		OBJECTIVE_TYPE_SECONDARY,
		OBJECTIVE_TYPE_TERTIARY,

		OBJECTIVE_STATUS_PENDING			= 0,
		OBJECTIVE_STATUS_ACCOMPLISHED,
		OBJECTIVE_STATUS_FAILED,
		OBJECTIVE_STATUS_HIDDEN,

		RADAR_BLIP_SHAPE_NONE				= 0,
		RADAR_BLIP_SHAPE_HUMAN,
		RADAR_BLIP_SHAPE_VEHICLE,
		RADAR_BLIP_SHAPE_STATIONARY,
		RADAR_BLIP_SHAPE_OBJECTIVE,

		RADAR_BLIP_COLOR_NOD					= 0,
		RADAR_BLIP_COLOR_GDI,
		RADAR_BLIP_COLOR_NEUTRAL,
		RADAR_BLIP_COLOR_MUTANT,
		RADAR_BLIP_COLOR_RENEGADE,
		RADAR_BLIP_COLOR_PRIMARY_OBJECTIVE,
		RADAR_BLIP_COLOR_SECONDARY_OBJECTIVE,
		RADAR_BLIP_COLOR_TERTIARY_OBJECTIVE,

		SCRIPT_PLAYERTYPE_SPECTATOR				= -4,		// -4
		SCRIPT_PLAYERTYPE_MUTANT,							// -3
		SCRIPT_PLAYERTYPE_NEUTRAL,							// -2
		SCRIPT_PLAYERTYPE_RENEGADE,							// -1
		SCRIPT_PLAYERTYPE_NOD,								//  0
		SCRIPT_PLAYERTYPE_GDI,								//  1
};

/*
** Script Commands List
*/

/*
**	The script-facing engine interface.
**
**	Built-in scripts call these directly.  They were once reached through a
**	table of function pointers handed across a DLL boundary; the scripts are
**	compiled into the engine now, so the table is gone and there is one
**	implementation of each of these, in scriptcommands.cpp.
**
**	The namespace is not decoration.  These names -- Get_Position, Find_Object,
**	Get_ID -- are too generic to sit at global scope in translation units that
**	also define hundreds of script classes.
*/
namespace ScriptEngine
{

	// Debug messages
	void Debug_Message ( const char *format, ... );

	// Action Commands
	void Action_Reset ( GameObject * obj, float priority );
	void Action_Goto ( GameObject * obj, const ActionParamsStruct & params );
	void Action_Attack ( GameObject * obj, const ActionParamsStruct & params );
	void Action_Play_Animation ( GameObject * obj, const ActionParamsStruct & params );
	void Action_Enter_Exit ( GameObject * obj, const ActionParamsStruct & params );
	void Action_Face_Location ( GameObject * obj, const ActionParamsStruct & params );
	void Action_Dock ( GameObject * obj, const ActionParamsStruct & params );
	void Action_Follow_Input ( GameObject * obj, const ActionParamsStruct & params );

	void Modify_Action ( GameObject * obj, int action_id, const ActionParamsStruct & params, bool modify_move = true, bool modify_attack = true );

	// Action information queries
	int Get_Action_ID ( GameObject * obj );
	bool Get_Action_Params ( GameObject * obj, ActionParamsStruct & params );
	bool Is_Performing_Pathfind_Action ( GameObject * obj );

	// Physical control
	void Set_Position ( GameObject * obj, const Vector3 & position );
	Vector3 Get_Position ( GameObject * obj );
	Vector3 Get_Bone_Position ( GameObject * obj, const char * bone_name );
	float Get_Facing ( GameObject * obj );
	void Set_Facing ( GameObject * obj, float degrees );

	// Collision Control
	void Disable_All_Collisions ( GameObject * obj );
	void Disable_Physical_Collisions ( GameObject * obj );
	void Enable_Collisions ( GameObject * obj );

	// Object Management
	void Destroy_Object ( GameObject * obj );
	GameObject * Find_Object ( int obj_id );
	GameObject * Find_Nearest_Preset ( const Vector3 & position, const char * preset_name );
	GameObject * Create_Object ( const char * type_name, const Vector3 & position );
	GameObject * Create_Object_At_Bone ( GameObject * host_obj, const char * new_obj_type_name, const char * bone_name );

	//
	//	Build a vehicle at the vehicle factory of a side, the way a player
	//	buying one does: it is generated over `delay` seconds and then driven
	//	out of the factory door.  False when that side has no factory, or has
	//	one that is destroyed, disabled or already building something.
	//
	//	`owner`, if given, is the player it is delivered to; a vehicle with no
	//	owner belongs to the side alone.
	//
	bool Create_Vehicle ( const char * preset_name, float delay, GameObject * owner, int player_type );
	int Get_ID ( GameObject * obj );
	int Get_Preset_ID ( GameObject * obj );
	const char * Get_Preset_Name ( GameObject * obj );
	void Attach_Script (GameObject* object, const char* scriptName, const char* scriptParams);
	void Add_To_Dirty_Cull_List (GameObject* object);

	// Timers
	void Start_Timer ( GameObject * obj, ScriptClass * script, float duration, int timer_id );
	void Stop_Timer ( GameObject * obj, ScriptClass * script, int timer_id );
	bool Has_Timer ( GameObject * obj, ScriptClass * script, int timer_id );

	// Weapons
	void Trigger_Weapon ( GameObject * obj, bool trigger, const Vector3 & target, bool primary = true );
	void Select_Weapon ( GameObject * obj, const char * weapon_name );

	// Custom Script
	void Send_Custom_Event ( GameObject * from, GameObject * to, int type = 0, intptr_t param = 0, float delay = 0 );
	void Send_Damaged_Event ( GameObject * object, GameObject * damager );

	// Random Numbers
	float Get_Random ( float min, float max );
	int Get_Random_Int ( int min, int max );  // Get a random number between min and max-1, INCLUSIVE

	//	Random Selection
	GameObject * Find_Random_Simple_Object ( const char *preset_name );

	// Object Display
	void Set_Model ( GameObject * obj, const char * model_name );
	void Set_Animation ( GameObject * obj, const char * anim_name, bool looping, const char * sub_obj_name = nullptr, float start_frame = 0.0F, float end_frame = -1.0F, bool is_blended = false );
	void Set_Animation_Frame ( GameObject * obj, const char * anim_name, int frame );

	// Sounds
	// Note: Each sound creation function returns the ID of the new sound (0 on error)
	int Create_Sound ( const char * sound_preset_name, const Vector3 & position, GameObject * creator );
	int Create_2D_Sound ( const char * sound_preset_name );
	int Create_2D_WAV_Sound ( const char * wav_filename );
	int Create_3D_WAV_Sound_At_Bone ( const char * wav_filename, GameObject * obj, const char * bone_name );
	int Create_3D_Sound_At_Bone ( const char * sound_preset_name, GameObject * obj, const char * bone_name );
	int Create_Logical_Sound ( GameObject * creator, int type, const Vector3 & position, float radius );
	void Start_Sound ( int sound_id );
	void Stop_Sound ( int sound_id, bool destroy_sound = true );
	void Monitor_Sound ( GameObject * game_obj, int sound_id );
	void Set_Background_Music ( const char * wav_filename );
	void Fade_Background_Music ( const char * wav_filename, int fade_out_time, int fade_in_time );
	void Stop_Background_Music ( void );

	// Object Properties
	float Get_Health ( GameObject * obj );
	float Get_Max_Health ( GameObject * obj );
	void Set_Health ( GameObject * obj, float health );
	float Get_Shield_Strength ( GameObject * obj );
	float Get_Max_Shield_Strength ( GameObject * obj );
	void Set_Shield_Strength ( GameObject * obj, float strength );
	void Set_Shield_Type ( GameObject * obj, const char * name );
	int Get_Player_Type ( GameObject * obj );
	GameObject * Get_Vehicle ( GameObject * obj );
	bool Is_Harvester ( GameObject * obj );
	void Set_Player_Type ( GameObject * obj, int type );

	//
	//	Say everything this object has to say to every client again.  A script
	//	that changes state the object does not itself mark as changed -- its
	//	side, its model, what it is standing on -- leaves the clients showing
	//	the old thing until something else makes the object talk.  This makes
	//	it talk.
	//
	void Update_Network_Object ( GameObject * obj );

	// Math
	float Get_Distance ( const Vector3 & p1, const Vector3 & p2 );

	// Set Camera Host
	void Set_Camera_Host ( GameObject * obj );
	void Force_Camera_Look ( const Vector3 & target );

	// Get the Star
	GameObject * Get_The_Star ( void );
	GameObject * Get_A_Star ( const Vector3 & pos );
	GameObject * Find_Closest_Soldier ( const Vector3 & pos, float min_dist, float max_dist, bool only_human = true );
	bool Is_A_Star ( GameObject * obj );

	// Object Control
	void Control_Enable ( GameObject * obj, bool enable );

	// Hack
	const char * Get_Damage_Bone_Name ( void );
	bool Get_Damage_Bone_Direction ( void ); // true means shot in the back

	// Visibility
	bool Is_Object_Visible ( GameObject * looker, GameObject * obj );
	void Enable_Enemy_Seen ( GameObject * obj, bool enable = true );

	// Display Text
	void Set_Display_Color ( unsigned char red = 255, unsigned char green = 255, unsigned char blue = 255 );
	void Display_Text ( int string_id );
	void Display_Float ( float value, const char * format = "%f" );
	void Display_Int ( int value, const char * format = "%d" );

	// SaveLoad
	void Save_Data ( ScriptSaver & saver, int id, int size, void * data );
	void Save_Pointer ( ScriptSaver & saver, int id, void * pointer );
	bool Load_Begin ( ScriptLoader & loader, int * id );
	void Load_Data ( ScriptLoader & loader, int size, void * data );
	void Load_Pointer ( ScriptLoader & loader, void ** pointer );
	void Load_End ( ScriptLoader & loader );

	void Begin_Chunk (ScriptSaver& saver, unsigned int chunkID);
	void End_Chunk (ScriptSaver& saver);
	bool Open_Chunk (ScriptLoader& loader, unsigned int* chunkID);
	void Close_Chunk (ScriptLoader& loader);

	// Radar Effects
	void Clear_Radar_Markers ( void );
	void Clear_Radar_Marker ( int id );
	void Add_Radar_Marker ( int id, const Vector3& position, int shape_type, int color_type );
	void Set_Obj_Radar_Blip_Shape ( GameObject * obj, int shape_type );	// Set to -1 to reset default
	void Set_Obj_Radar_Blip_Color ( GameObject * obj, int color_type );	// Set to -1 to reset default
	void Enable_Radar ( bool enable );

	//
	//	Map support
	//
	void Clear_Map_Cell ( int cell_x, int cell_y );
	void Clear_Map_Cell_By_Pos ( const Vector3 &world_space_pos );
	void Clear_Map_Cell_By_Pixel_Pos ( int pixel_pos_x, int pixel_pos_y );
	void Clear_Map_Region_By_Pos ( const Vector3 &world_space_pos, int pixel_radius );
	void Reveal_Map ( void );
	void Shroud_Map ( void );
	void Show_Player_Map_Marker ( bool onoff );

	//
	//	Height DB access
	//
	float Get_Safe_Flight_Height ( float x_pos, float y_pos );

	// Explosions
	void Create_Explosion ( const char * explosion_def_name, const Vector3 & pos, GameObject * creator = nullptr );
	void Create_Explosion_At_Bone ( const char * explosion_def_name, GameObject * object, const char * bone_name, GameObject * creator = nullptr );

	// HUD
	void Enable_HUD ( bool enable );
	void Mission_Complete ( bool success );

	void Give_PowerUp ( GameObject * obj, const char * preset_name, bool display_on_hud = false );

	// Administration
	void Innate_Disable (GameObject* object);
	void Innate_Enable (GameObject* object);

	// Innate Soldier AI Enable/Disable (returns old value)
	bool Innate_Soldier_Enable_Enemy_Seen ( GameObject * obj, bool state );
	bool Innate_Soldier_Enable_Gunshot_Heard ( GameObject * obj, bool state );
	bool Innate_Soldier_Enable_Footsteps_Heard ( GameObject * obj, bool state );
	bool Innate_Soldier_Enable_Bullet_Heard ( GameObject * obj, bool state );
	bool Innate_Soldier_Enable_Actions ( GameObject * obj, bool state );
	void Set_Innate_Soldier_Home_Location ( GameObject * obj, const Vector3& home_pos, float home_radius = 999999);
	void Set_Innate_Aggressiveness ( GameObject * obj, float aggressiveness );
	void Set_Innate_Take_Cover_Probability ( GameObject * obj, float probability );
	void Set_Innate_Is_Stationary ( GameObject * obj, bool stationary );

	void Innate_Force_State_Bullet_Heard ( GameObject * obj, const Vector3 & pos );
	void Innate_Force_State_Footsteps_Heard ( GameObject * obj, const Vector3 & pos );
	void Innate_Force_State_Gunshots_Heard ( GameObject * obj, const Vector3 & pos );
	void Innate_Force_State_Enemy_Seen ( GameObject * obj, GameObject * enemy );

	// Control of StaticAnimPhys
	void Static_Anim_Phys_Goto_Frame ( int obj_id, float frame, const char * anim_name = nullptr );
	void Static_Anim_Phys_Goto_Last_Frame ( int obj_id, const char * anim_name = nullptr );

	// Timing
	unsigned int Get_Sync_Time ( void );

	// Objectives
	void Add_Objective ( int id, int type, int status, int short_description_id, const char* description_sound_filename = nullptr, int long_description_id = 0 );
	void Remove_Objective ( int id );
	void Set_Objective_Status ( int id, int status );
	void Change_Objective_Type ( int id, int type );
	void Set_Objective_Radar_Blip ( int id, const Vector3 & position );
	void Set_Objective_Radar_Blip_Object ( int id, ScriptableGameObj * unit );
	void Set_Objective_HUD_Info ( int id, float priority, const char * texture_name, int message_id );
	void Set_Objective_HUD_Info_Position ( int id, float priority, const char* texture_name, int message_id, const Vector3 & position );

	// Camaera Shakes
	void Shake_Camera ( const Vector3 & pos, float radius = 25, float intensity = 0.25f, float duration = 1.5f );

	// Spawners
	void Enable_Spawner ( int id, bool enable );
	GameObject * Trigger_Spawner ( int id );

	// Vehicles
	void Enable_Engine ( GameObject* object, bool onoff );

	// Difficulty Level
	int Get_Difficulty_Level ( void );

	// Keys
	void Grant_Key ( GameObject* object, int key, bool grant = true );
	bool Has_Key ( GameObject* object, int key );

	// Hibernation
	void Enable_Hibernation ( GameObject * object, bool enable );

	void Attach_To_Object_Bone ( GameObject * object, GameObject * host_object, const char * bone_name );

	// Conversation
	int Create_Conversation ( const char *conversation_name, int priority = 0, float max_dist = 0, bool is_interruptable = true );
	void Join_Conversation ( GameObject * object, int active_conversation_id, bool allow_move = true, bool allow_head_turn = true, bool allow_face = true );
	void Join_Conversation_Facing ( GameObject * object, int active_conversation_id, int obj_id_to_face );
	void Start_Conversation ( int active_conversation_id, int action_id = 0 );
	void Monitor_Conversation ( GameObject * object, int active_conversation_id );
	void Start_Random_Conversation ( GameObject * object );
	void Stop_Conversation ( int active_conversation_id );
	void Stop_All_Conversations ( void );

	// Locked facing support
	void Lock_Soldier_Facing ( GameObject * object, GameObject * object_to_face, bool turn_body );
	void Unlock_Soldier_Facing ( GameObject * object );

	// Apply Damage
	void Apply_Damage ( GameObject * object, float amount, const char * warhead_name, GameObject * damager = nullptr );

	//
	//	What is hitting this object, asked from inside its Damaged, Killed or
	//	Destroyed handler.  Those are handed the victim, who is responsible and
	//	how much health went, which does not distinguish a repair gun from a
	//	tank shell.
	//
	//	Get_Damage_Warhead is the warhead of the damage being applied; compare
	//	it against a Get_Warhead_Type of the name you care about.  It reads
	//	(unsigned int)-1 outside any damage.
	//
	//	Get_Explosion_Object is the mine, C4 or beacon whose blast is being
	//	resolved, and null when the damage came from anything else.  It is the
	//	device rather than the player who placed it; the player is the damager
	//	the handler already has.
	//
	unsigned int Get_Damage_Warhead ( void );
	GameObject * Get_Explosion_Object ( void );

	//	Look a warhead up by the name a level or a .ini writes.
	unsigned int Get_Warhead_Type ( const char * warhead_name );

	// Soldier
	void Set_Loiters_Allowed ( GameObject * object, bool allowed );

	void Set_Is_Visible ( GameObject * object, bool visible );
	void Set_Is_Rendered ( GameObject * object, bool rendered );

	// Points
	float Get_Points ( GameObject * object );
	void Give_Points ( GameObject * object, float points, bool entire_team );

	// Money (points and money were separated 09/06/01)
	float Get_Money ( GameObject * object );
	void Give_Money ( GameObject * object, float money, bool entire_team );

	// Buildings
	bool Get_Building_Power ( GameObject * object );
	void Set_Building_Power ( GameObject * object, bool onoff );
	void Play_Building_Announcement ( GameObject * object, int text_id );
	GameObject * Find_Nearest_Building_To_Pos ( const Vector3 & position, const char * mesh_prefix );
	GameObject * Find_Nearest_Building ( GameObject * object, const char * mesh_prefix );

	// Zones
	int Team_Members_In_Zone ( GameObject * object, int player_type );

	// Background
	void Set_Clouds (float cloudcover, float cloudgloominess, float ramptime);
	void Set_Lightning (float intensity, float startdistance, float enddistance, float heading, float distribution, float ramptime);
	void Set_War_Blitz (float intensity, float startdistance, float enddistance, float heading, float distribution, float ramptime);

	// Weather
	void Set_Wind (float heading, float speed, float variability, float ramptime);
	void Set_Rain (float density, float ramptime, bool prime);
	void Set_Snow (float density, float ramptime, bool prime);
	void Set_Ash (float density, float ramptime, bool prime);
	void Set_Fog_Enable (bool enabled);
	void Set_Fog_Range (float startdistance, float enddistance, float ramptime);

	// Stealth control
	void Enable_Stealth (GameObject * object, bool onoff);

	// Sniper control
	void Cinematic_Sniper_Control (bool enabled, float zoom);

	// File Access
	void * Text_File_Open ( const char * filename );
	bool Text_File_Get_String ( void *handle, char * buffer, int size );
	void Text_File_Close ( void *handle );

	// Vehicle Transitions
	void Enable_Vehicle_Transitions ( GameObject * object, bool enable );

	// Player terminal support
	void Display_GDI_Player_Terminal ();
	void Display_NOD_Player_Terminal ();
	void Display_Mutant_Player_Terminal ();

	// Encyclopedia support
	bool Reveal_Encyclopedia_Character ( int object_id );
	bool Reveal_Encyclopedia_Weapon ( int object_id );
	bool Reveal_Encyclopedia_Vehicle ( int object_id );
	bool Reveal_Encyclopedia_Building ( int object_id );
	void Display_Encyclopedia_Event_UI ( void );

	void Scale_AI_Awareness ( float sight_scale, float hearing_scale );

	// Cinematic Freeze
	void Enable_Cinematic_Freeze ( GameObject * object, bool enable );

	void Expire_Powerup ( GameObject * object );

	// Hud stuff
	void Set_HUD_Help_Text ( int string_id, const Vector3 &color );
	void Enable_HUD_Pokable_Indicator ( GameObject * object, bool enable );

	void Enable_Innate_Conversations ( GameObject * object, bool enable );

	void Display_Health_Bar ( GameObject * object, bool display );

	// Shadow control.  In certain cases we need to manually disable shadow casting
	// on an object.  Cinematics with too many characters are an example of this.
	void Enable_Shadow ( GameObject * object, bool enable );

	void Clear_Weapons ( GameObject * object );

	void Set_Num_Tertiary_Objectives ( int count );

	// Letterbox and screen fading controls
	void Enable_Letterbox ( bool onoff, float seconds );
	void Set_Screen_Fade_Color ( float r, float g, float b, float seconds );
	void Set_Screen_Fade_Opacity ( float opacity, float seconds );

	// Commands whose effect belongs to one client rather than to the world.
	// A script runs on the server, so these ask the addressee's machine to run
	// the matching command above; on a dedicated server the server itself sees
	// nothing, which is right.  A team of -1 means every connected client.
	// `player` is any object a client is controlling -- normally the soldier.
	// A sound started on someone else's machine has no id here, so unlike their
	// local counterparts these return nothing.

	// ------------------------------------------------------------------
	//	Questions and actions the 4.8.4 script library needs and the stock
	//	catalog never asked for.  A team of 2 means "either side" wherever one
	//	is taken, matching how the library's own scripts are authored.
	// ------------------------------------------------------------------

	// Finding things
	GameObject * Find_Object_By_Preset ( int team, const char * preset_name );
	GameObject * Find_Closest_Building ( const Vector3 & position );
	GameObject * Find_Smart_Object_By_Team ( int team );
	GameObject * Find_Object_With_Script ( const char * script_name );
	int Get_Object_Count ( int team, const char * preset_name );
	bool Is_Unit_In_Range ( const char * preset_name, float range, const Vector3 & position, int team, bool allow_empty );

	// Movement state
	Vector3 Get_Velocity ( GameObject * obj );
	void Set_Velocity ( GameObject * obj, const Vector3 & velocity );
	void Set_Transform ( GameObject * obj, const Matrix3D & transform );
	float Get_Mass ( GameObject * obj );

	// Model and animation state
	const char * Get_Model ( GameObject * obj );
	float Get_Animation_Frame ( GameObject * obj );
	float Get_Animation_Target_Frame ( GameObject * obj );

	// Powerups
	void Set_Powerup_Always_Allow_Grant ( GameObject * obj, bool allow );
	const char * Get_Powerup_Weapon ( const char * powerup_preset_name );

	// Vehicles and their occupants
	GameObject * Get_Vehicle_Driver ( GameObject * obj );
	GameObject * Get_Vehicle_Gunner ( GameObject * obj );
	GameObject * Get_Vehicle_Occupant ( GameObject * obj, int seat );
	int Get_Vehicle_Occupant_Count ( GameObject * obj );
	int Get_Vehicle_Seat_Count ( GameObject * obj );
	int Get_Occupant_Seat ( GameObject * vehicle, GameObject * occupant );
	int Get_Vehicle_Mode ( GameObject * obj );
	bool Is_VTOL ( GameObject * obj );
	void Force_Occupants_Exit ( GameObject * obj );
	void Force_Occupant_Exit ( GameObject * obj, int seat );
	void Force_Occupants_Exit_Team ( GameObject * obj, int team );
	void Soldier_Transition_Vehicle ( GameObject * obj );

	// Soldier state
	bool Get_Fly_Mode ( GameObject * obj );
	void Toggle_Fly_Mode ( GameObject * obj );
	bool Is_Stealth ( GameObject * obj );
	bool Is_Stealth_Enabled ( GameObject * obj );
	bool Change_Character ( GameObject * obj, const char * preset_name );

	// The player behind a soldier, if there is one
	int Get_Player_ID ( GameObject * obj );
	const unichar_t * Get_Wide_Player_Name ( GameObject * obj );
	void Change_Team ( GameObject * obj, int team, bool destroy_object );

	// Bases and buildings
	GameObject * Find_Building_By_Type ( int team, int type );
	GameObject * Find_Construction_Yard ( int team );
	GameObject * Find_Com_Center ( int team );
	bool Is_Building_Dead ( GameObject * obj );
	bool Is_Base_Powered ( int team );
	void Power_Base ( int team, bool powered );
	bool Is_Radar_Enabled ( int team );
	void Enable_Base_Radar ( int team, bool enable );
	void Set_Can_Generate_Soldiers ( int team, bool can_generate );

	// Weapons
	const char * Get_Weapon ( GameObject * obj, int position );
	const char * Get_Current_Weapon ( GameObject * obj );
	bool Has_Weapon ( GameObject * obj, const char * weapon_name );
	int Get_C4_Mode ( GameObject * obj );
	GameObject * Get_C4_Planter ( GameObject * obj );
	GameObject * Get_C4_Attached ( GameObject * obj );
	GameObject * Get_Beacon_Planter ( GameObject * obj );

	// Object state a server rule needs to change
	void Remove_Weapon ( GameObject * obj, const char * weapon_name );

	//
	//	A weapon by its own definition name rather than through a powerup that
	//	grants it, which is what a script wants when the weapon it is handing
	//	out has no powerup -- an emplacement's gun, say.  Grant false takes it
	//	away again.  Rounds of -1 fills the clip and leaves the reserve
	//	unlimited; a positive count is rounds, or clips of rounds when
	//	`in_clips` is set.
	//
	void Grant_Weapon ( GameObject * obj, const char * weapon_name, bool grant,
			int rounds, bool in_clips );

	//
	//	Give an object the weapon with this definition id, and optionally make
	//	it the one in its hands.  False if the id names no weapon: a level that
	//	leaves the parameter at zero is asking for whatever the preset already
	//	carries.
	//
	bool Grant_Weapon_Definition ( GameObject * obj, int definition_id, bool select );

	//
	//	The match that is running.  A level asks these; there is no match in the
	//	editor, so each of them answers with a harmless nothing there.
	//
	float Get_Time_Remaining_Seconds ( void );
	void Set_Time_Remaining_Seconds ( float seconds );
	int Get_Time_Limit_Minutes ( void );
	void Set_Time_Limit_Minutes ( int minutes );
	unsigned int Get_Game_Duration_Seconds ( void );
	int Get_Game_Win_Type ( void );
	int Get_Game_Winner_ID ( void );
	int Get_Game_Max_Players ( void );
	const char * Get_Map_Name ( void );
	const unichar_t * Get_Game_Title ( void );
	float Get_Team_Score ( int player_type );
	int Get_Team_Credits ( int player_type );

	//
	//	Turn the radar on or off for one whole side at once.
	//
	void Enable_Radar_Team ( int player_type, bool onoff );
	void Set_Skin ( GameObject * obj, const char * armor_name );
	void Set_Max_Health ( GameObject * obj, float health );
	void Set_Max_Shield_Strength ( GameObject * obj, float strength );
	void Set_Damage_Points ( GameObject * obj, float points );
	void Set_Death_Points ( GameObject * obj, float points );
	
	//
	//	Hurt everything within radius of a point.  A team of 0 or 1 hurts
	//	only that team; anything else hurts everyone.
	//
	void Damage_All_Objects_Area ( float amount, const char * warhead_name,
			const Vector3 & position, float radius, int team, GameObject * damager );
	
	//
	//	Whole categories of thing repaired or damaged at once, which is how a
	//	superweapon and a repair beacon work.  Throughout this group Nod is 0,
	//	GDI is 1, and 2 means both.
	//
	void Repair_All_Buildings_By_Team ( int team, int except_id, float health );
	void Repair_All_Buildings_By_Team_Radius ( int team, int center_id, float health,
			float radius );
	void Repair_All_Static_Vehicles_By_Team ( int team, int message );
	void Repair_All_Turrets_By_Team ( int team, float health );
	
	void Damage_Occupants ( GameObject * obj, float amount, const char * warhead_name );
	void Kill_Occupants ( GameObject * obj );
	
	void Damage_All_Objects_Area_By_Team ( float amount, const char * warhead_name,
			const Vector3 & position, float radius, GameObject * damager,
			bool soldiers, bool vehicles, int team );
	
	void Ranged_Damage_To_Buildings_Team ( int team, float amount,
			const char * warhead_name, const Vector3 & position, float radius,
			GameObject * damager );
	void Ranged_Scale_Damage_To_Buildings_Team ( int team, float amount,
			const char * warhead_name, const Vector3 & position, float radius,
			GameObject * damager );
	void Ranged_Percentage_Damage_To_Buildings_Team ( int team, float fraction,
			const char * warhead_name, const Vector3 & position, float radius,
			GameObject * damager );
	
	//
	//	The armor an object is wearing.  The name is the 4.8.4 library's; it
	//	reads what Set_Skin writes.
	//
	const char * Get_Skin ( GameObject * obj );
	
	//
	//	An indicator object beside everything in the area that is hiding --
	//	stealthed, or a vehicle that has gone underground.
	//
	void Create_Effect_All_Stealthed_Objects_Area ( const Vector3 & position,
			float radius, const char * effect_preset, const Vector3 & offset,
			int team );
	
	//
	//	The two halves of the console seam.  Input is a line to run as though
	//	it had been typed; output is a line to print.  Both go out on the
	//	event bus, because the console itself lives above this library.
	//
	void Console_Input ( const char * text );
	void Console_Output ( const char * format, ... );
	
	// Definitions
	int Get_Definition_ID ( const char * preset_name );
	const char * Get_Definition_Name ( int definition_id );
	bool Is_Valid_Preset_ID ( int definition_id );

	// Translated strings
	//
	//	The name to show a player for an object, or for a preset: the
	//	translated name where the definition has one, and the preset's own
	//	name where it does not.  Out-parameters rather than a returned
	//	buffer, so there is nothing for the caller to free.
	//
	void Get_Translated_Preset_Name ( GameObject * obj, WideStringClass & name );
	void Get_Translated_Definition_Name ( int definition_id, WideStringClass & name );
	
	//
	//	A string out of the translation database by its id, the name of a
	//	team, and the name of the weapon an object is holding.  All three are
	//	out-parameters for the same reason as the pair above.
	//
	void Get_Translated_String ( int string_id, WideStringClass & text );
	void Get_Team_Name ( int team, WideStringClass & name );
	void Get_Current_Translated_Weapon ( GameObject * obj, WideStringClass & name );
	
	bool Is_Valid_String_ID ( int string_id );
	int Get_String_Sound_ID ( int string_id );

	// Scripts on objects
	bool Is_Script_Attached ( GameObject * obj, const char * script_name );
	ScriptClass * Find_Script_On_Object ( GameObject * obj, const char * script_name );
	void Attach_Script_Once ( GameObject * obj, const char * script_name, const char * params );

	//	The same, with the parameter list built printf-style.
	void Attach_Script_Once_V ( GameObject * obj, const char * script_name, const char * format, ... );
	void Attach_Script_Occupants ( GameObject * obj, const char * script_name, const char * params );
	void Attach_Script_Preset ( const char * script_name, const char * params, const char * preset_name, int team, bool once );
	void Attach_Script_Type ( const char * script_name, const char * params, unsigned long class_id, int team, bool once );
	void Attach_Script_Building ( const char * script_name, const char * params, int team );
	void Attach_Script_Player_Once ( const char * script_name, const char * params, int team );
	void Remove_Script ( GameObject * obj, const char * script_name );
	void Remove_All_Scripts ( GameObject * obj );
	void Remove_Script_Preset ( const char * script_name, const char * preset_name, int team );
	void Remove_Script_Type ( const char * script_name, unsigned long class_id, int team );

	// Broadcasting a custom
	void Send_Custom_All_Objects ( int type, GameObject * sender, int team );
	void Send_Custom_All_Objects_Area ( int type, const Vector3 & position, float distance, GameObject * sender, int team );
	void Send_Custom_To_Preset ( GameObject * sender, const char * preset_name, int type, int param, float delay );

	// Effects over a set of objects
	void Create_Effect_All_Of_Preset ( const char * effect_preset_name, const char * preset_name, float z_adjust, bool z_absolute );

	// Time
	void Seconds_To_Hms ( float seconds, int & out_hours, int & out_minutes, int & out_seconds );

	void Send_Message ( int red, int green, int blue, const char * message );
	void Send_Message_Player ( GameObject * player, int red, int green, int blue, const char * message );
	void Send_Message_Team ( int team, int red, int green, int blue, const char * message );
	void Send_Message_With_Team_Color ( int team, const char * message );

	void Create_Sound_Player ( GameObject * player, const char * sound_preset_name, const Vector3 & position );
	void Create_Sound_Team ( int team, const char * sound_preset_name, const Vector3 & position );
	void Create_2D_Sound_Player ( GameObject * player, const char * sound_preset_name );
	void Create_2D_Sound_Team ( int team, const char * sound_preset_name );
	void Create_2D_WAV_Sound_Player ( GameObject * player, const char * wav_filename );
	void Create_2D_WAV_Sound_Team ( int team, const char * wav_filename );

	void Set_Background_Music_Player ( GameObject * player, const char * wav_filename );
	void Fade_Background_Music_Player ( GameObject * player, const char * wav_filename, int fade_out_time, int fade_in_time );
	void Stop_Background_Music_Player ( GameObject * player );

	void Set_HUD_Help_Text_Player ( GameObject * player, int string_id, const Vector3 & color );
	void Set_Screen_Fade_Color_Player ( GameObject * player, float r, float g, float b, float seconds );
	void Set_Screen_Fade_Opacity_Player ( GameObject * player, float opacity, float seconds );

	//	Every occupant of a vehicle at once, which is what a script wants when
	//	the vehicle itself is what is doing something to their view.
	void Set_Occupants_Fade ( GameObject * vehicle, float r, float g, float b, float opacity );
	void Force_Camera_Look_Player ( GameObject * player, const Vector3 & target );
	void Enable_Radar_Player ( GameObject * player, bool enable );
	void Display_GDI_Player_Terminal_Player ( GameObject * player );
	void Display_NOD_Player_Terminal_Player ( GameObject * player );

	// Server-side team operations
	void Kill_All_Buildings_By_Team ( int team );

	// Restore a soldier's ammunition, health and armor -- what a vehicle
	// purchase terminal grants when it sells a refill.
	void Grant_Refill ( GameObject * player );

}	// namespace ScriptEngine

#endif	// SCRIPTCOMMANDS_H
