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
 *                     $Archive:: /Commando/Code/Combat/scriptcommands.cpp                    $*
 *                                                                                             *
 *                      $Author:: Bhayes                                                      $*
 *                                                                                             *
 *                     $Modtime:: 2/16/02 8:37p                                               $*
 *                                                                                             *
 *                    $Revision:: 285                                                         $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "scriptcommands.h"
#include "debug.h"
#include "combat.h"
#include "smartgameobj.h"
#include "damage.h"
#include "gameobjmanager.h"
#include "crandom.h"
#include "scriptman.h"
#include "scriptevents.h"
#include "objlibrary.h"
#include "phys.h"
#include "ccamera.h"
#include "Sound3D.h"
#include "combat.h"
#include "radar.h"
#include "explosion.h"
#include "powerup.h"
#include "weaponmanager.h"
#include "soldier.h"
#include "weapons.h"
#include "pscene.h"
#include "staticanimphys.h"
#include "hanim.h"
#include "objectives.h"
#include "spawn.h"
#include "assets.h"
#include "widestring.h"
#include "translatedb.h"
#include "damageablegameobj.h"
#include "damagecontext.h"
#include "networkobject.h"
#include "weaponbag.h"
#include "weapons.h"
#include "vehicle.h"
#include "combatchunkid.h"
#include "LogicalSound.h"
#include "soldierobserver.h"
#include "cinematicgameobj.h"
#include "dynamicanimphys.h"
#include <stdio.h>
#include "conversationmgr.h"
#include "activeconversation.h"
#include "orator.h"
#include "gameobjobserver.h"
#include "gameinfo.h"
#include "playerroster.h"
#include "ini.h"
#include "ffactory.h"
#include "wwfile.h"
#include "obbox.h"
#include "scriptzone.h"
#include "damageablegameobj.h"
#include "purchasesettings.h"
#include "teampurchasesettings.h"
#include "playertype.h"
#include "animcontrol.h"
#include "playerdata.h"
#include "building.h"
#include "basecontroller.h"
#include "vehiclefactorygameobj.h"
#include "colors.h"
#include "animcontrol.h"
#include "beacongameobj.h"
#include "c4.h"
#include "movephys.h"
#include "translatedb.h"
#include "translateobj.h"
#include "gameeventbus.h"
#include "string_ids.h"
#include "vehicle.h"
#include "scscriptcommandevent.h"
#include "weaponbag.h"
#include "scriptzone.h"
#include "hud.h"
#include "backgroundmgr.h"
#include "WeatherMgr.h"
#include "mapmgr.h"
#include "heightdb.h"
#include "weaponbag.h"
#include "ffactory.h"
#include "playerterminal.h"
#include "hudinfo.h"
#include "globalsettings.h"
#include "screenfademanager.h"


//
//	Toggled by the scripttrace console command, so it is engine state rather
//	than part of the script-facing interface and stays outside the namespace.
//
bool		ScriptTrace	= false;


//
//	Everything below is the script-facing engine interface declared in
//	scriptcommands.h.  See that header for why it is namespaced.
//
namespace ScriptEngine
{



#define	SCRIPT_TRACE(x)	if (::ScriptTrace) {Debug_Say(x);}

#define	SCRIPT_PTR_CHECK( x )				if ( x == nullptr ) { Debug_Say(( "nullptr Script Ptr at %s line %d\n", __FILE__, __LINE__ )); return;	}
#define	SCRIPT_PTR_CHECK_RET( x, ret )	if ( x == nullptr ) { Debug_Say(( "nullptr Script Ptr at %s line %d\n", __FILE__, __LINE__ )); return ret;	}

char	DamageModelName[ 30 ];	// HACK
bool	DamageModelDirection;		// HACK

#define	SCRIPT_PRIORITY	2

/*
** Debug messages
*/
void	Debug_Message( [[maybe_unused]] const char *format, ... )
{
#ifdef WWDEBUG
	SCRIPT_PTR_CHECK( format );
	va_list arg_list;
	va_start (arg_list, format);
	StringClass string(true);
	string.Format_Args( format, arg_list );
	va_end (arg_list);
	Debug_Script(( (const char *)string ));
#endif
}

void Modify_Action( GameObject * obj, int action_id, const ActionParamsStruct & params, bool modify_move, bool modify_attack )
{
	SCRIPT_PTR_CHECK( obj );
	SCRIPT_TRACE((	"ST>Modify_Action( %d, %d )\n", obj->Get_ID(), action_id ));

	SmartGameObj * smart = obj->As_SmartGameObj();
	if (	smart != nullptr && smart->Get_Action() != nullptr &&
			smart->Get_Action()->Get_Parameters().ActionID == action_id )
	{
		smart->Get_Action()->Modify( params, modify_move, modify_attack );
	}

	return ;
}


int	Get_Action_ID( GameObject * obj )
{
	SCRIPT_PTR_CHECK_RET( obj, 0 );
	SCRIPT_TRACE((	"ST>Get_Action_ID( %d )\n", obj->Get_ID() ));

	int retval = 0;

	//
	//	Simply return the ID of the current action
	//
	SmartGameObj * smart = obj->As_SmartGameObj();
	if (	smart != nullptr && smart->Get_Action() != nullptr ) {
		retval = smart->Get_Action()->Get_Parameters().ActionID;
	}

	return retval;
}

bool	Get_Action_Params( GameObject * obj, ActionParamsStruct & params )
{
	SCRIPT_PTR_CHECK_RET( obj, false );
	SCRIPT_TRACE((	"ST>Get_Action_Params( %d )\n", obj->Get_ID() ));

	bool retval = false;

	//
	//	Simply copy the parameters to the provided structure
	//
	SmartGameObj * smart = obj->As_SmartGameObj();
	if (	smart != nullptr && smart->Get_Action() != nullptr ) {
		params					= smart->Get_Action()->Get_Parameters();
		params.MoveObject		= smart->Get_Action()->Get_Parameters().MoveObjectRef;
		params.LookObject		= smart->Get_Action()->Get_Parameters().LookObjectRef;
		params.AttackObject	= smart->Get_Action()->Get_Parameters().AttackObjectRef;
		retval = true;
	}

	return retval;
}

bool	Is_Performing_Pathfind_Action( GameObject * obj )
{
	SCRIPT_PTR_CHECK_RET( obj, false );
	SCRIPT_TRACE((	"ST>Is_Performing_Pathfind_Action( %d )\n", obj->Get_ID() ));

	bool retval = false;

	//
	//	Simply ask the action if its "busy", this means its doing something that
	// can't be interrupted
	//
	SmartGameObj * smart = obj->As_SmartGameObj();
	if (	smart != nullptr && smart->Get_Action() != nullptr ) {
		smart->Get_Action()->Is_Busy();
	}

	return retval;
}


void Action_Reset( GameObject * obj, float priority )
{
	SCRIPT_PTR_CHECK( obj );
	SCRIPT_TRACE((	"ST>Action_Reset( %d, %f )\n", obj->Get_ID(), priority ));

	SmartGameObj * smart = obj->As_SmartGameObj();
	if ( smart ) {
		smart->Get_Action()->Reset( priority );
	}
}

void Action_Goto( GameObject * obj, const ActionParamsStruct & params )
{
	SCRIPT_PTR_CHECK( obj );
	SCRIPT_TRACE((	"ST>Action_Goto( %d )\n", obj->Get_ID() ));

	SmartGameObj * smart = obj->As_SmartGameObj();
	if ( smart ) {
		smart->Get_Action()->Goto( params );
	}
}

void Action_Attack( GameObject * obj, const ActionParamsStruct & params )
{
	SCRIPT_PTR_CHECK( obj );
	SCRIPT_TRACE((	"ST>Action_Attack( %d )\n", obj->Get_ID() ));

	SmartGameObj * smart = obj->As_SmartGameObj();
	if ( smart ) {
		smart->Get_Action()->Attack( params );
	}
}

void Action_Play_Animation( GameObject * obj, const ActionParamsStruct & params )
{
	SCRIPT_PTR_CHECK( obj );
	SCRIPT_TRACE((	"ST>Action_Play_Animation( %d )\n", obj->Get_ID() ));

	SmartGameObj * smart = obj->As_SmartGameObj();
	if ( smart ) {
		smart->Get_Action()->Play_Animation( params );
	}
}

void Action_Enter_Exit( GameObject * obj, const ActionParamsStruct & params )
{
	SCRIPT_PTR_CHECK( obj );
	SCRIPT_TRACE((	"ST>Action_Enter_Exit( %d )\n", obj->Get_ID() ));

	SmartGameObj * smart = obj->As_SmartGameObj();
	if ( smart ) {
		smart->Get_Action()->Enter_Exit( params );
	}
}


void Action_Face_Location( GameObject * obj, const ActionParamsStruct & params )
{
	SCRIPT_PTR_CHECK( obj );
	SCRIPT_TRACE((	"ST>Action_Face_Location( %d )\n", obj->Get_ID() ));

	SmartGameObj * smart = obj->As_SmartGameObj();
	if ( smart ) {
		smart->Get_Action()->Face_Location( params );
	}
}


void Action_Dock( GameObject * obj, const ActionParamsStruct & params )
{
	SCRIPT_PTR_CHECK( obj );
	SCRIPT_TRACE((	"ST>Action_Dock( %d )\n", obj->Get_ID() ));

	SmartGameObj * smart = obj->As_SmartGameObj();
	if ( smart ) {
		smart->Get_Action()->Dock_Vehicle( params );
	}
}


void Action_Follow_Input( GameObject * obj, const ActionParamsStruct & params )
{
	SCRIPT_PTR_CHECK( obj );
	SCRIPT_TRACE((	"ST>Action_Follow_Input( %d )\n", obj->Get_ID() ));

	SmartGameObj * smart = obj->As_SmartGameObj();
	if ( smart ) {
		smart->Get_Action()->Follow_Input( params );
	}
}



/*
** Physical control
*/
void	Set_Position( GameObject * obj, const Vector3 & position )
{
	SCRIPT_PTR_CHECK( obj );

	PhysicalGameObj * pgobj = obj->As_PhysicalGameObj();
	if ( pgobj == nullptr ) {
		Debug_Say(( "Not a PhysicalGameObj at %s %d\n", __FILE__, __LINE__ ));
		return;
	}

	SCRIPT_TRACE((	"ST>Set_Position( %d, (%f ,%f,%f) )\n", pgobj->Get_ID(), position[0], position[1], position[2] ));
	pgobj->Peek_Physical_Object()->Set_Position( position );
}

Vector3 Get_Position( GameObject * obj )
{
	SCRIPT_PTR_CHECK_RET( obj, Vector3(0,0,0) );

	Vector3 pos;
	obj->Get_Position(&pos);
	return pos;
}

Vector3 Get_Bone_Position( GameObject * obj, const char * bone_name )
{
	SCRIPT_PTR_CHECK_RET( obj, Vector3(0,0,0) );

	PhysicalGameObj * pgobj = obj->As_PhysicalGameObj();
	if ( pgobj == nullptr ) {
		Debug_Say(( "Not a PhysicalGameObj at %s %d\n", __FILE__, __LINE__ ));
		return Vector3(0,0,0);
	}

	SCRIPT_PTR_CHECK_RET( pgobj->Peek_Model(), Vector3(0,0,0) );
	Matrix3D tm = pgobj->Peek_Model()->Get_Bone_Transform( bone_name );
	return tm.Get_Translation();
}

float Get_Facing( GameObject * obj )
{
	SCRIPT_PTR_CHECK_RET( obj, 0 );

	PhysicalGameObj * pgobj = obj->As_PhysicalGameObj();
	if ( pgobj == nullptr ) {
		Debug_Say(( "Not a PhysicalGameObj at %s %d\n", __FILE__, __LINE__ ));
		return 0;
	}

	return RAD_TO_DEGF( pgobj->Get_Facing() );
}

void	Set_Facing( GameObject * obj, float degrees )
{
	SCRIPT_PTR_CHECK( obj );

	PhysicalGameObj * pgobj = obj->As_PhysicalGameObj();
	if ( pgobj == nullptr ) {
		Debug_Say(( "Not a PhysicalGameObj at %s %d\n", __FILE__, __LINE__ ));
		return;
	}


	SCRIPT_TRACE((	"ST>Set_Facing( %d, %f )\n", pgobj->Get_ID(), degrees ));
	Vector3 pos;
	pgobj->Get_Position(&pos);

	Matrix3D tm(1);
	tm.Translate(pos);
	tm.Rotate_Z( DEG_TO_RADF( degrees ) );

	pgobj->Set_Transform(tm);
}


/*
** Collision Control
*/
void	Disable_All_Collisions( GameObject * obj )
{
	SCRIPT_PTR_CHECK( obj );

	PhysicalGameObj * pgobj = obj->As_PhysicalGameObj();
	if ( pgobj == nullptr ) {
		Debug_Say(( "Not a PhysicalGameObj at %s %d\n", __FILE__, __LINE__ ));
		return;
	}

	SCRIPT_TRACE((	"ST>Disable_All_Collisions( %d )\n", pgobj->Get_ID() ));
	pgobj->Set_Collision_Group( UNCOLLIDEABLE_GROUP );
}

void	Disable_Physical_Collisions( GameObject * obj )
{
	SCRIPT_PTR_CHECK( obj );

	PhysicalGameObj * pgobj = obj->As_PhysicalGameObj();
	if ( pgobj == nullptr ) {
		Debug_Say(( "Not a PhysicalGameObj at %s %d\n", __FILE__, __LINE__ ));
		return;
	}

	SCRIPT_TRACE((	"ST>Disable_Physical_Collisions( %d )\n", pgobj->Get_ID() ));
	pgobj->Set_Collision_Group( BULLET_ONLY_COLLISION_GROUP );
}

void	Enable_Collisions( GameObject * obj )
{
	SCRIPT_PTR_CHECK( obj );

	PhysicalGameObj * pgobj = obj->As_PhysicalGameObj();
	if ( pgobj == nullptr ) {
		Debug_Say(( "Not a PhysicalGameObj at %s %d\n", __FILE__, __LINE__ ));
		return;
	}

	SCRIPT_TRACE((	"ST>Enable_Collisions( %d )\n", pgobj->Get_ID() ));
	if ( pgobj->As_SoldierGameObj () != nullptr ) {
		pgobj->Set_Collision_Group( SOLDIER_COLLISION_GROUP );
	} else {
		pgobj->Set_Collision_Group( DEFAULT_COLLISION_GROUP );
	}
}

/*
** Object Management
*/
void	Destroy_Object( GameObject * obj )
{
	SCRIPT_PTR_CHECK( obj );
	SCRIPT_TRACE((	"ST>Destroy_Object( %d )\n", obj->Get_ID() ));
	obj->Set_Delete_Pending();
}


GameObject * Find_Object( int obj_id )
{
	return GameObjManager::Find_ScriptableGameObj( obj_id );
}

int	Get_ID( GameObject * obj )
{
	SCRIPT_PTR_CHECK_RET( obj, 0 );
	return obj->Get_ID();
}

int	Get_Preset_ID( GameObject * obj )
{
	SCRIPT_PTR_CHECK_RET( obj, 0 );
	return obj->Get_Definition ().Get_ID ();
}

const char * Get_Preset_Name( GameObject * obj )
{
	SCRIPT_PTR_CHECK_RET( obj, "" );
	return obj->Get_Definition ().Get_Name ();
}




/******************************************************************************
*
* NAME
*     Create_Object
*
* DESCRIPTION
*     Create an object with the specified transform.
*
* INPUTS
*     const char* name
*     const Matrix3D& tm
*
* RESULTS
*     GameObject*
*
******************************************************************************/

GameObject* Create_Object(const char* name, const Matrix3D& tm)
{
	assert(name != nullptr);
	GameObject* object = ObjectLibraryManager::Create_Object(name);

	if (object != nullptr) {
		WWASSERT( object->As_PhysicalGameObj() );
		object->As_PhysicalGameObj()->Set_Transform(tm);
		object->Start_Observers();
	}

	return object;
}


GameObject* Create_Object(const char* type_name, const Vector3 & position)
{
	SCRIPT_PTR_CHECK_RET(type_name, nullptr);
	SCRIPT_TRACE((	"ST>Create_Object( %s (%f,%f,%f) )\n",
		type_name, position[0], position[1], position[2] ));

#if(0) // Denzil
	GameObject* obj = ObjectLibraryManager::Create_Object(type_name);

	if (obj != nullptr) {
		Matrix3D tm(1);
		tm.Set_Translation(position);
		obj->Set_Transform(tm);
	}

	return obj;
#else
	Matrix3D tm(true);
	tm.Set_Translation(position);
	return Create_Object(type_name, tm);
#endif
}


GameObject* Create_Object_At_Bone(GameObject* host_obj, const char* new_obj_type_name, const char* bone_name)
{
	SCRIPT_PTR_CHECK_RET( host_obj, nullptr );
	SCRIPT_PTR_CHECK_RET( new_obj_type_name, nullptr );
	SCRIPT_PTR_CHECK_RET( bone_name, nullptr );

	PhysicalGameObj * pgobj = host_obj->As_PhysicalGameObj();
	if ( pgobj == nullptr ) {
		Debug_Say(( "Not a PhysicalGameObj at %s %d\n", __FILE__, __LINE__ ));
		return nullptr;
	}

	SCRIPT_TRACE((	"ST>Create_Object_At_Bone( %d %s %s )\n", host_obj->Get_ID(), new_obj_type_name, bone_name ));

#if(0) // Denzil
	GameObject* obj = ObjectLibraryManager::Create_Object(new_obj_type_name);

	if (obj != nullptr) {
		obj->Set_Transform(host_obj->Peek_Model()->Get_Bone_Transform(bone_name));
	}

	return obj;
#else

	Matrix3D tm = pgobj->Peek_Model()->Get_Bone_Transform(bone_name);
	return Create_Object(new_obj_type_name, tm);
#endif
}


bool Create_Vehicle(const char* preset_name, float delay, GameObject* owner, int player_type)
{
	SCRIPT_PTR_CHECK_RET( preset_name, false );

	SCRIPT_TRACE((	"ST>Create_Vehicle( %s %f %d )\n", preset_name, delay, player_type ));

	//
	//	Only the server builds vehicles.  A client asking would produce one
	//	the rest of the game does not know about.
	//
	if ( CombatManager::I_Am_Server() == false ) {
		return false;
	}

	int definition_id = Get_Definition_ID( preset_name );
	if ( definition_id == 0 ) {
		Debug_Say(( "Create_Vehicle: no preset named %s\n", preset_name ));
		return false;
	}

	BaseControllerClass * base = BaseControllerClass::Find_Base( player_type );
	if ( base == nullptr ) {
		return false;
	}

	BuildingGameObj * building = base->Find_Building( BuildingConstants::TYPE_VEHICLE_FACTORY );
	if ( building == nullptr ) {
		return false;
	}

	VehicleFactoryGameObj * factory = building->As_VehicleFactoryGameObj();
	if ( factory == nullptr || factory->Is_Available() == false ) {
		return false;
	}

	SoldierGameObj * player = nullptr;
	if ( owner != nullptr ) {
		PhysicalGameObj * physical = owner->As_PhysicalGameObj();
		if ( physical != nullptr ) {
			player = physical->As_SoldierGameObj();
		}
	}

	return factory->Request_Vehicle( definition_id, delay, player );
}


// Attach a script to and object.
void Attach_Script(GameObject* object, const char* scriptName, const char* scriptParams)
{
	SCRIPT_PTR_CHECK( object );
	SCRIPT_PTR_CHECK( scriptName );
	// Create a script instance
	assert(scriptName != nullptr);
	ScriptClass* script = ScriptManager::Create_Script(scriptName);

	if (script != nullptr) {

		// Set the scripts parameters
		assert(scriptParams != nullptr);
		script->Set_Parameters_String(scriptParams);

		// Add the script to the object
		assert(object != nullptr);
		object->Add_Observer(script);

	} else {

		WWDEBUG_SAY(("Unable to create script: %s\r\n",scriptName));
	}
}

void Add_To_Dirty_Cull_List(GameObject* obj)
{
	SCRIPT_PTR_CHECK( obj );
	PhysicalGameObj *physobj = obj->As_PhysicalGameObj();
	if ( physobj != nullptr ) {
		COMBAT_SCENE->Add_To_Dirty_Cull_List( physobj->Peek_Physical_Object() );
	}
}


/*
** Timers
*/
void	Start_Timer( GameObject * obj, ScriptClass * script, float duration, int timer_id )
{
	SCRIPT_PTR_CHECK( obj );
	SCRIPT_PTR_CHECK( script );
	SCRIPT_TRACE((	"ST>Start_Timer( %d, %d, %f, %d )\n", obj->Get_ID(), script->Get_ID(), duration, timer_id ));
	obj->Start_Observer_Timer( script->Get_ID(), duration, timer_id );
}

void	Stop_Timer( GameObject * obj, ScriptClass * script, int timer_id )
{
	SCRIPT_PTR_CHECK( obj );
	SCRIPT_PTR_CHECK( script );
	SCRIPT_TRACE((	"ST>Stop_Timer( %d, %d, %d )\n", obj->Get_ID(), script->Get_ID(), timer_id ));
	obj->Stop_Observer_Timer( script->Get_ID(), timer_id );
}

bool	Has_Timer( GameObject * obj, ScriptClass * script, int timer_id )
{
	SCRIPT_PTR_CHECK_RET( obj, false );
	SCRIPT_PTR_CHECK_RET( script, false );
	return obj->Has_Observer_Timer( script->Get_ID(), timer_id );
}



/*
** Weapons
*/
void	Trigger_Weapon( GameObject * obj, bool trigger, const Vector3 & target, bool primary_fire )
{
	SCRIPT_PTR_CHECK( obj );
	SCRIPT_TRACE((	"ST>Trigger_Weapon( %d, %d, (%f %f %f ) %d\n", obj->Get_ID(), trigger, target.X, target.Y, target.Z, primary_fire ));
	PhysicalGameObj *phyobj = obj->As_PhysicalGameObj();
	if ( phyobj != nullptr ) {
		ArmedGameObj *armed = phyobj->As_ArmedGameObj();
		if (armed) {
			WeaponClass * weapon = armed->Get_Weapon();

			if ( weapon != nullptr ) {
				armed->Set_Targeting( target );
				if ( primary_fire ) {
					weapon->Set_Primary_Triggered( trigger );
				} else {
					weapon->Set_Secondary_Triggered( trigger );
				}
			} else {
				Debug_Say(( "This object does not have this weapon\n" ));
			}
		} else {
			Debug_Say(( "This object cannot fire weapons\n" ));
		}
	} else {
		Debug_Say(( "This object cannot fire weapons\n" ));
	}
}

void	Select_Weapon( GameObject * obj, const char * weapon_name )
{
	SCRIPT_PTR_CHECK( obj );
	SCRIPT_TRACE((	"ST>Select_Weapon( %d, %s\n", obj->Get_ID(), weapon_name ));
	PhysicalGameObj *phyobj = obj->As_PhysicalGameObj();
	if ( phyobj != nullptr ) {
		ArmedGameObj *armed = phyobj->As_ArmedGameObj();
		if (armed) {
			armed->Get_Weapon_Bag()->Select_Weapon_Name( weapon_name );
		} else {
			Debug_Say(( "This object cannot fire weapons\n" ));
		}
	} else {
		Debug_Say(( "This object cannot fire weapons\n" ));
	}
}


/*
**
*/
void	Send_Custom_Event( GameObject * from, GameObject * to, int type, intptr_t param, float delay )
{
#if(0) // Denzil 4/4/00 - From not required for most messages
	SCRIPT_PTR_CHECK( from );
	SCRIPT_PTR_CHECK( to );  // ?? This may be okay to be null
	SCRIPT_TRACE((	"ST>Send_Custom_Event( %d, %d %d, %d )\n", from->Get_ID(), to->Get_ID(), type, param ));
#else
	SCRIPT_PTR_CHECK( to );
	SCRIPT_TRACE(("ST>Send_Custom_Event( %d, %d %d, %d )\n",
		((from != nullptr) ? from->Get_ID() : 0), to->Get_ID(), type, param));
#endif

	WWASSERT( type < CUSTOM_EVENT_SYSTEM_FIRST );

	if ( to ) {
		if ( delay <= 0 ) {
			const GameObjObserverList & observer_list = to->Get_Observers();
			for( int index = 0; index < observer_list.Count(); index++ ) {
				observer_list[ index ]->Custom( to, type, param, from );
			}
		} else {
			to->Start_Custom_Timer( from, delay, type, param );
		}
	}
}

void	Send_Damaged_Event( GameObject * object, GameObject * damager )
{
	SCRIPT_PTR_CHECK( object );
	SCRIPT_TRACE(("ST>Send_Damaged_Event( %d, %d )\n",
		object->Get_ID(), ((damager != nullptr) ? damager->Get_ID() : 0) ));

	if ( object ) {
		const GameObjObserverList & observer_list = object->Get_Observers();
		for( int index = 0; index < observer_list.Count(); index++ ) {
			observer_list[ index ]->Damaged( object, damager, 0 );
		}
	}
}


/*
**
*/
float	Get_Random( float min, float max )
{
	return FreeRandom.Get_Float( min, max );
}


int	Get_Random_Int( int min, int max )
{
	return FreeRandom.Get_Int( min, max );
}


/*
**	Find_Random_Simple_Object
*/
GameObject *Find_Random_Simple_Object ( const char *preset_name )
{
	SCRIPT_TRACE((	"ST>Find_Random_Simple_Object( %s )\n", preset_name ));

	GameObject *retval = nullptr;
	DynamicVectorClass<SimpleGameObj *> obj_list;

	//
	//	Build a list of all the simple game objects that match the criteria
	//
	SLNode<BaseGameObj> *node = nullptr;
	for (node = GameObjManager::Get_Game_Obj_List()->Head(); node; node = node->Next()) {
		WWASSERT(node->Data() != nullptr);

		//
		//	Is this a simple game obj?
		//
		PhysicalGameObj *physical_obj = node->Data()->As_PhysicalGameObj();
		if ( physical_obj != nullptr ) {
			SimpleGameObj *simple_obj = physical_obj->As_SimpleGameObj();
			if ( simple_obj != nullptr ) {

				//
				//	Is this one of the objects we can choose from?
				//
				if ( ::stricmp ( simple_obj->Get_Definition ().Get_Name (), preset_name ) == 0 ) {
					obj_list.Add (simple_obj);
				}
			}
		}
	}

	//
	//	Index randomly into the object list
	//
	int count = obj_list.Count ();
	if ( count > 0 ) {
		int random_index = FreeRandom.Get_Int( count );
		retval = obj_list[random_index];
	}

	return retval;
}


/*
** Object Display
*/
void	Set_Model( GameObject * obj, const char * model_name )
{
	SCRIPT_PTR_CHECK( obj );

	PhysicalGameObj * pgobj = obj->As_PhysicalGameObj();
	if ( pgobj == nullptr ) {
		Debug_Say(( "Not a PhysicalGameObj at %s %d\n", __FILE__, __LINE__ ));
		return;
	}

	if ( obj->As_PhysicalGameObj() && obj->As_PhysicalGameObj()->As_SimpleGameObj() ) {
		if ( obj->As_PhysicalGameObj()->As_SimpleGameObj()->Get_Definition().Get_Is_Editor_Object() ) {
			Debug_Say(( "Can't Set_Model on a IsEditorObject %s %s\n", obj->As_PhysicalGameObj()->As_SimpleGameObj()->Get_Definition().Get_Name(), model_name ));
//			WWASSERT(0);
		}
	}

//	Matrix3D tm = pgobj->Get_Transform();
	SCRIPT_TRACE((	"ST>Set_Model( %d, %s, %d )\n", obj->Get_ID(), model_name == nullptr ? "nullptr" : model_name ));
	if (pgobj->As_SoldierGameObj() ) {
		// For soldiers, call Set_Model, so the anim control will be updated
		pgobj->As_SoldierGameObj()->Set_Model( model_name );
	} else {
		pgobj->Peek_Physical_Object()->Set_Model_By_Name( model_name );
		if ( pgobj->Get_Anim_Control() != nullptr ) {
			pgobj->Get_Anim_Control()->Set_Model( pgobj->Peek_Model() );
		}
	}

	pgobj->Set_Object_Dirty_Bit( NetworkObjectClass::BIT_RARE, true);

	pgobj->Hide_Muzzle_Flashes();				// Cinematics need this
	return ;
}

void	Set_Animation( GameObject * obj, const char * anim_name, bool looping, const char * sub_obj_name, float start_frame, float end_frame, bool is_blended )
{
	SCRIPT_PTR_CHECK( obj );
	SCRIPT_TRACE((	"ST>Set_Animation( %d, %s, %d )\n", obj->Get_ID(), anim_name == nullptr ? "nullptr" : anim_name, looping ));

	PhysicalGameObj * pgobj = obj->As_PhysicalGameObj();
	if ( pgobj == nullptr ) {
		Debug_Say(( "Not a PhysicalGameObj at %s %d\n", __FILE__, __LINE__ ));
		return;
	}

	// check that this object is not in Action_Play_Animation
	if ( pgobj->As_SmartGameObj() != nullptr ) {
		if ( pgobj->As_SmartGameObj()->Get_Action()->Is_Animating() ) {
			Debug_Say(( "Can't Set_Animation when Action_Play_Animationing\n" ));
			WWASSERT( 0 );
		}
	}

	if ( ( sub_obj_name != nullptr ) && ( *sub_obj_name != 0 ) ) {

		RenderObjClass * model = pgobj->Peek_Model();
		RenderObjClass * sub = model->Get_Sub_Object_By_Name( sub_obj_name );

		if ( sub != nullptr ) {
			HAnimClass * anim = WW3DAssetManager::Get_Instance()->Get_HAnim( anim_name );
			if ( anim != nullptr ) {
				sub->Set_Animation( anim, start_frame, looping ? RenderObjClass::ANIM_MODE_LOOP : RenderObjClass::ANIM_MODE_ONCE );
				anim->Release_Ref();
			}
			sub->Release_Ref();
		}

	} else {
		CinematicGameObj * cinobj = pgobj->As_CinematicGameObj();
		if (cinobj != nullptr) {

			// CinematicGameObj's require you to use the physics animation interface
			DynamicAnimPhysClass * dynanim = cinobj->Peek_Physical_Object()->As_DynamicAnimPhysClass();
			if (dynanim != nullptr) {

				AnimCollisionManagerClass & anim_mgr = dynanim->Get_Animation_Manager();

				if (anim_name != nullptr) {
					anim_mgr.Set_Animation(anim_name);
				}
				anim_mgr.Set_Current_Frame( start_frame );

				if (looping) {
					anim_mgr.Set_Animation_Mode(AnimCollisionManagerClass::ANIMATE_LOOP);
				} else {
					anim_mgr.Set_Animation_Mode(AnimCollisionManagerClass::ANIMATE_TARGET);
					anim_mgr.Set_Target_Frame_End();
				}

				//
				//	Specify the target frame (if necessary)
				//
				if ( end_frame >= 0 ) {
					anim_mgr.Set_Target_Frame( end_frame );
				}

				//
				//	"Dirty" the object for networking
				//
				cinobj->Set_Object_Dirty_Bit( NetworkObjectClass::BIT_RARE, true );

			} else {
				WWDEBUG_SAY(("Error! cinematic game obj %s not using DynamicAnimPhys.\r\n"));
			}

		} else {

			//
			//	Soldier's can blend their animation, so if the game object is
			// a soldier and the blend flag is set, then blend the animation.
			//
			SoldierGameObj *soldier = pgobj->As_SoldierGameObj();
			if ( soldier != nullptr && is_blended ) {
				soldier->Set_Blended_Animation( anim_name, looping, start_frame );
			} else {
				pgobj->Set_Animation( anim_name, looping, start_frame );
			}

			//
			//	If the anim_name is the same as the current anim, then the frame_offset isn't set, so
			// force it.
			//
			AnimControlClass *anim_control = pgobj->Get_Anim_Control();
			if (anim_control != nullptr) {
				anim_control->Set_Mode( looping ? ANIM_MODE_LOOP : ANIM_MODE_ONCE, start_frame );

				//
				//	Specify the target frame (if necessary)
				//
				if ( end_frame >= 0 ) {
					anim_control->Set_Mode( ANIM_MODE_TARGET, start_frame );
					anim_control->Set_Target_Frame( end_frame );
				}
			}
		}
	}
}


void	Set_Animation_Frame( GameObject * obj, const char * anim_name, int frame )
{
	SCRIPT_PTR_CHECK( obj );
	SCRIPT_TRACE((	"ST>Set_Animation_Frame( %d, %s, %d )\n", obj->Get_ID(), anim_name == nullptr ? "nullptr" : anim_name, frame ));

	PhysicalGameObj * pgobj = obj->As_PhysicalGameObj();
	if ( pgobj == nullptr ) {
		Debug_Say(( "Not a PhysicalGameObj at %s %d\n", __FILE__, __LINE__ ));
		return;
	}

	// check that this object is not in Action_Play_Animation
	if ( pgobj->As_SmartGameObj() != nullptr ) {
		if ( pgobj->As_SmartGameObj()->Get_Action()->Is_Animating() ) {
			Debug_Say(( "Can't Set_Animation when Action_Play_Animationing\n" ));
			WWASSERT( 0 );
		}
	}

	pgobj->Set_Animation_Frame ( anim_name, frame );
}


/*
**
*/
int Create_Sound(const char* sound_name, const Vector3& position, GameObject* creator)
{
	SCRIPT_TRACE(("ST>Create_Sound( %s, (%f,%f,%f) )\n", sound_name, position[0], position[1], position[2]));

	Matrix3D matrix(true);
	matrix.Set_Translation(position);

	WWAudioClass* audio = WWAudioClass::Get_Instance();
	assert(audio != nullptr);

	RefCountedGameObjReference *owner_ref = new RefCountedGameObjReference;
	owner_ref->Set_Ptr( creator );
	int sound_id = audio->Create_Instant_Sound(sound_name, matrix, owner_ref);
	REF_PTR_RELEASE( owner_ref );

	return sound_id;
}

int Create_2D_Sound( const char * sound_preset_name )
{
	WWAudioClass* audio = WWAudioClass::Get_Instance();
	assert(audio != nullptr);

	//
	//	Create the sound object from its preset
	//
	int sound_id = 0;
	AudibleSoundClass *sound = audio->Create_Sound( sound_preset_name );
	if ( sound != nullptr ) {

		sound_id = sound->Get_ID ();
		sound->Play();
		sound->Release_Ref();
	}

	return sound_id;
}

int Create_2D_WAV_Sound( const char * wav_filename )
{
	WWAudioClass* audio = WWAudioClass::Get_Instance();
	assert(audio != nullptr);

	AudibleSoundClass * sound = audio->Create_Sound_Effect(wav_filename);

	int sound_id = 0;
	if ( sound != nullptr ) {
		WWASSERT( sound->Get_Loop_Count() != 0 );
		sound_id = sound->Get_ID ();
		sound->Play();
		sound->Release_Ref();
	}

	return sound_id;
}


int Create_3D_Sound_At_Bone( const char * sound_preset_name, GameObject * obj, const char * bone_name )
{
	WWAudioClass* audio = WWAudioClass::Get_Instance();
	assert(audio != nullptr);

	PhysicalGameObj * pgobj = obj->As_PhysicalGameObj();
	if ( pgobj == nullptr ) {
		Debug_Say(( "Not a PhysicalGameObj at %s %d\n", __FILE__, __LINE__ ));
		return 0;
	}

	//
	//	Create the sound object from its preset
	//
	int sound_id = 0;
	AudibleSoundClass *sound = audio->Create_Sound( sound_preset_name );
	if ( sound != nullptr ) {

		//
		//	Attach the sound to the bone
		//
		RenderObjClass * model = pgobj->Peek_Model();
		int bone_index = model->Get_Bone_Index( bone_name );
		sound->Attach_To_Object( model, bone_index );

		//
		//	Add the sound to the world and release our hold on it
		//
		sound_id = sound->Get_ID ();
		sound->Add_To_Scene();
		REF_PTR_RELEASE( sound );
	}

	return sound_id;
}


int Create_3D_WAV_Sound_At_Bone( const char * wav_filename, GameObject * obj, const char * bone_name )
{
	WWAudioClass* audio = WWAudioClass::Get_Instance();
	assert(audio != nullptr);

	PhysicalGameObj * pgobj = obj->As_PhysicalGameObj();
	if ( pgobj == nullptr ) {
		Debug_Say(( "Not a PhysicalGameObj at %s %d\n", __FILE__, __LINE__ ));
		return 0;
	}

	int sound_id = 0;
	Sound3DClass *	sound = audio->Create_3D_Sound( wav_filename );
	if ( sound ) {

		sound_id = sound->Get_ID ();
		sound->Set_Type ( AudibleSoundClass::TYPE_SOUND_EFFECT );
		sound->Set_Priority ( 1 );
		sound->Set_Volume ( 1 );
		sound->Set_Loop_Count ( 1 );
		sound->Set_DropOff_Radius ( 30 );
		sound->Set_Definition ( nullptr );
		sound->Set_Max_Vol_Radius ( 30 );
		RenderObjClass * model = pgobj->Peek_Model();
		int bone_index = model->Get_Bone_Index( bone_name );
		sound->Attach_To_Object( model, bone_index );

		sound->Add_To_Scene();
		sound->Release_Ref();
	}

	return sound_id;
}

int Create_Logical_Sound( GameObject * creator, int type, const Vector3 & position, float radius )
{
	SCRIPT_TRACE(("ST>Create_Sound( %d, (%f,%f,%f) )\n", type, position[0], position[1], position[2]));

	int sound_id = 0;
	WWAudioClass* audio = WWAudioClass::Get_Instance();
	if ( audio != nullptr ) {
		LogicalSoundClass *	sound = audio->Create_Logical_Sound();
		if ( sound != nullptr ) {

			RefCountedGameObjReference *owner_ref = new RefCountedGameObjReference;
			owner_ref->Set_Ptr( creator );
			sound->Set_User_Data( owner_ref, 0 );
			REF_PTR_RELEASE( owner_ref );

			sound_id = sound->Get_ID ();
			sound->Set_Type_Mask( type );
			sound->Set_Notify_Delay( 0 );
			sound->Set_Single_Shot( true );
			sound->Set_DropOff_Radius( radius );
			sound->Set_Position( position );
			sound->Add_To_Scene ();
			sound->Release_Ref();
		}
	}

	return sound_id;
}

void Monitor_Sound( GameObject * game_obj, int sound_id )
{
	SCRIPT_TRACE(("ST>Monitor_Sound( %d, %d )\n", game_obj->Get_ID(), sound_id));

	WWAudioClass* audio = WWAudioClass::Get_Instance();
	if ( audio != nullptr ) {

		//
		//	Lookup the sound object by its ID
		//
		SoundSceneObjClass *sound_obj = audio->Find_Sound_Object (sound_id);
		if ( sound_obj != nullptr ) {

			//
			//	Register the game object as a callback for the sound object
			//
			sound_obj->Register_Callback( AudioCallbackClass::EVENT_SOUND_ENDED, game_obj );
		}
	}

	return ;
}


void Fade_Background_Music( const char * wav_filename, int fade_out_time, int fade_in_time )
{
	SCRIPT_TRACE(("ST>Fade_Background_Music( %s, %d, %d )\n", wav_filename, fade_out_time, fade_in_time));

	WWAudioClass* audio = WWAudioClass::Get_Instance();
	if ( audio != nullptr ) {

		//
		//	Pass the background music onto the sound library
		//
		audio->Fade_Background_Music( wav_filename, fade_out_time, fade_in_time );
	}

	return ;
}


void Set_Background_Music( const char * wav_filename )
{
	SCRIPT_TRACE(("ST>Set_Background_Music( %s )\n", wav_filename));

	WWAudioClass* audio = WWAudioClass::Get_Instance();
	if ( audio != nullptr ) {

		//
		//	Pass the background music onto the sound library
		//
		audio->Set_Background_Music( wav_filename );
	}

	return ;
}

void Stop_Background_Music( void )
{
	SCRIPT_TRACE(("ST>Stop_Background_Music( )\n"));

	WWAudioClass* audio = WWAudioClass::Get_Instance();
	if ( audio != nullptr ) {

		//
		//	Simply stop the background music
		//
		audio->Set_Background_Music( nullptr );
	}

	return ;
}



void Start_Sound( int sound_id )
{
	SCRIPT_TRACE(("ST>Stop_Sound( %d )\n", sound_id));

	WWAudioClass* audio = WWAudioClass::Get_Instance();
	if ( audio != nullptr ) {

		//
		//	Lookup the sound object by its ID
		//
		SoundSceneObjClass *sound_obj = audio->Find_Sound_Object( sound_id );
		if ( sound_obj != nullptr) {

			//
			//	Add the sound to the scene (if necessary)
			//
			if ( sound_obj->Is_In_Scene() == false ) {
				sound_obj->Add_To_Scene();
			}

			//
			//	Start playing the sound
			//
			AudibleSoundClass *audible_sound = 	sound_obj->As_AudibleSoundClass();
			if (audible_sound != nullptr) {
				audible_sound->Play();
			}
		}
	}

	return ;
}


void Stop_Sound( int sound_id, bool destroy_sound )
{
	SCRIPT_TRACE(("ST>Stop_Sound( %d )\n", sound_id));

	WWAudioClass* audio = WWAudioClass::Get_Instance();
	if ( audio != nullptr ) {

		//
		//	Lookup the sound object by its ID
		//
		SoundSceneObjClass *sound_obj = audio->Find_Sound_Object( sound_id );
		if ( sound_obj != nullptr ) {

			//
			//	Stop playing the sound
			//
			AudibleSoundClass *audible_sound = 	sound_obj->As_AudibleSoundClass();
			if (audible_sound != nullptr) {
				audible_sound->Stop();
			}

			//
			//	Remove the sound from the world (this will destroy the sound)
			//
			if ( destroy_sound ) {
				sound_obj->Remove_From_Scene();
			}
		}
	}

	return ;
}


/*
**
*/
float	Get_Health( GameObject * obj )
{
	SCRIPT_PTR_CHECK_RET( obj, 0 );

	DamageableGameObj * dgobj = obj->As_DamageableGameObj();
	if ( dgobj == nullptr ) {
		Debug_Say(( "Not a DamageableGameObj at %s %d\n", __FILE__, __LINE__ ));
		return 0;
	}

	return dgobj->Get_Defense_Object()->Get_Health();
}

float	Get_Max_Health( GameObject * obj )
{
	SCRIPT_PTR_CHECK_RET( obj, 0 );

	DamageableGameObj * dgobj = obj->As_DamageableGameObj();
	if ( dgobj == nullptr ) {
		Debug_Say(( "Not a DamageableGameObj at %s %d\n", __FILE__, __LINE__ ));
		return 0;
	}

	return dgobj->Get_Defense_Object()->Get_Health_Max();
}

void	Set_Health( GameObject * obj, float health )
{
	SCRIPT_PTR_CHECK( obj );

	DamageableGameObj * dgobj = obj->As_DamageableGameObj();
	if ( dgobj == nullptr ) {
		Debug_Say(( "Not a DamageableGameObj at %s %d\n", __FILE__, __LINE__ ));
		return;
	}

	dgobj->Get_Defense_Object()->Set_Health( health );
}

float	Get_Shield_Strength( GameObject * obj )
{
	SCRIPT_PTR_CHECK_RET( obj, 0 );

	DamageableGameObj * dgobj = obj->As_DamageableGameObj();
	if ( dgobj == nullptr ) {
		Debug_Say(( "Not a DamageableGameObj at %s %d\n", __FILE__, __LINE__ ));
		return 0;
	}

	return dgobj->Get_Defense_Object()->Get_Shield_Strength();
}

float	Get_Max_Shield_Strength( GameObject * obj )
{
	SCRIPT_PTR_CHECK_RET( obj, 0 );

	DamageableGameObj * dgobj = obj->As_DamageableGameObj();
	if ( dgobj == nullptr ) {
		Debug_Say(( "Not a DamageableGameObj at %s %d\n", __FILE__, __LINE__ ));
		return 0;
	}

	return dgobj->Get_Defense_Object()->Get_Shield_Strength_Max();
}

void	Set_Shield_Strength( GameObject * obj, float strength )
{
	SCRIPT_PTR_CHECK( obj );

	DamageableGameObj * dgobj = obj->As_DamageableGameObj();
	if ( dgobj == nullptr ) {
		Debug_Say(( "Not a DamageableGameObj at %s %d\n", __FILE__, __LINE__ ));
		return;
	}

	dgobj->Get_Defense_Object()->Set_Shield_Strength( strength );
}

void	Set_Shield_Type( GameObject * obj, const char * name )
{
	SCRIPT_PTR_CHECK( obj );

	DamageableGameObj * dgobj = obj->As_DamageableGameObj();
	if ( dgobj == nullptr ) {
		Debug_Say(( "Not a DamageableGameObj at %s %d\n", __FILE__, __LINE__ ));
		return;
	}

	dgobj->Get_Defense_Object()->Set_Shield_Type( ArmorWarheadManager::Get_Armor_Type( name ) );
}


//
//	The vehicle a soldier is driving, if any.  A base defence that fires at
//	the man rather than the tank he is sitting in is aiming at the wrong
//	thing, and that is what this is for.
//
//
//	The closest object of a given preset.  A cinematic control file names
//	the scenery it wants to drive by preset, because the level designer who
//	wrote the file has no object id to give it.
//
GameObject *	Find_Nearest_Preset( const Vector3 & position, const char * preset_name )
{
	if ( preset_name == nullptr ) {
		return nullptr;
	}

	GameObject *	nearest = nullptr;
	float			nearest_dist2 = FLT_MAX;

	SLNode<BaseGameObj> * node = GameObjManager::Get_Game_Obj_List()->Head();
	for ( ; node != nullptr; node = node->Next() ) {
		BaseGameObj * base = node->Data();
		GameObject * obj = ( base != nullptr ) ? base->As_ScriptableGameObj() : nullptr;
		if ( obj == nullptr ) {
			continue;
		}

		if ( ::stricmp( Get_Preset_Name( obj ), preset_name ) != 0 ) {
			continue;
		}

		Vector3 obj_position;
		obj->Get_Position( &obj_position );

		float dist2 = ( obj_position - position ).Length2();
		if ( dist2 < nearest_dist2 ) {
			nearest = obj;
			nearest_dist2 = dist2;
		}
	}

	return nearest;
}

GameObject *	Get_Vehicle( GameObject * obj )
{
	SCRIPT_PTR_CHECK_RET( obj, nullptr );

	SoldierGameObj * soldier = obj->As_SoldierGameObj();
	if ( soldier == nullptr ) {
		return nullptr;
	}

	return soldier->Get_Vehicle();
}

//
//	Whether this is a team's harvester.  Automated defences leave it alone:
//	shooting the harvester is a player's decision, not a turret's.
//
bool	Is_Harvester( GameObject * obj )
{
	if ( obj == nullptr ) {
		return false;
	}

	for ( int team = 0; team < 2; team++ ) {
		BaseControllerClass * base = BaseControllerClass::Find_Base( team );
		if ( base != nullptr && base->Get_Harvester_Vehicle() == obj ) {
			return true;
		}
	}

	return false;
}

//
//	Which team owns this.  Asked of a DamageableGameObj rather than a
//	PhysicalGameObj, because a building controller is the former and not
//	the latter -- asking a building which team it belonged to answered Nod
//	and logged a complaint, which is why anything driven by a building had
//	its team written into the script instead.
//
int	Get_Player_Type( GameObject * obj )
{
	SCRIPT_PTR_CHECK_RET( obj, 0 );

	DamageableGameObj * dgobj = obj->As_DamageableGameObj();
	if ( dgobj == nullptr ) {
		Debug_Say(( "Not a DamageableGameObj at %s %d\n", __FILE__, __LINE__ ));
		return 0;
	}

	return	dgobj->Get_Player_Type();
}

void	Set_Player_Type( GameObject * obj, int type )
{
	SCRIPT_PTR_CHECK( obj );

	DamageableGameObj * dgobj = obj->As_DamageableGameObj();
	if ( dgobj == nullptr ) {
		Debug_Say(( "Not a DamageableGameObj at %s %d\n", __FILE__, __LINE__ ));
	} else {
		dgobj->Set_Player_Type( type );
	}

}


void	Update_Network_Object( GameObject * obj )
{
	SCRIPT_PTR_CHECK( obj );
	SCRIPT_TRACE((	"ST>Update_Network_Object( %d )\n", obj->Get_ID() ));

	//
	//	BIT_RARE carries the occasional and frequent bits with it, so this is
	//	everything the object has to say rather than only its slowest state.
	//
	obj->Set_Object_Dirty_Bit( NetworkObjectClass::BIT_RARE, true );
}


/*
**
*/
float	Get_Distance( const Vector3 & p1, const Vector3 & p2 )
{
	Vector3 v = p1 - p2;
	return v.Length();
}


/*
**
*/
void	Set_Camera_Host( GameObject * obj )
{
	//SCRIPT_PTR_CHECK( obj );
	SCRIPT_TRACE((	"ST>Set_Camera_Host( %p )\n", obj ));
	if ( obj == nullptr ) {
		COMBAT_CAMERA->Set_Host_Model( nullptr );
	} else {
		PhysicalGameObj * pgobj = obj->As_PhysicalGameObj();
		if ( pgobj == nullptr ) {
			Debug_Say(( "Not a PhysicalGameObj at %s %d\n", __FILE__, __LINE__ ));
			return;
		}

		COMBAT_CAMERA->Set_Host_Model( pgobj->Peek_Model() );
	}
}

void	Force_Camera_Look( const Vector3 & target )
{
	//SCRIPT_PTR_CHECK( obj );
	SCRIPT_TRACE((	"ST>Force_Camera_Look( %f %f %f )\n", target.X, target.Y, target.Z ));
	if ( COMBAT_CAMERA ) {
		COMBAT_CAMERA->Force_Look( target );
	}
}


/*
**
**	Find_Closest_Soldier
**
*/
GameObject * Find_Closest_Soldier( const Vector3 & pos, float min_dist, float max_dist, bool only_human )
{
	AABoxClass box (pos, Vector3 (max_dist / 2, max_dist / 2, max_dist / 2));

	//
	//	Collect all the dynamic objects in this box
	//
	NonRefPhysListClass obj_list;
	PhysicsSceneClass::Get_Instance ()->Collect_Objects (box, false, true, &obj_list);

	float closest_dist		= max_dist;
	GameObject *closest_obj	= nullptr;

	//
	//	Loop over all the collected objects
	//
	NonRefPhysListIterator it (&obj_list);
	for (it.First(); !it.Is_Done(); it.Next()) {
		PhysClass *phys_obj = it.Peek_Obj ();
		PhysicalGameObj *game_obj = nullptr;

		if ( phys_obj->Get_Observer() != nullptr ) {
			game_obj = ((CombatPhysObserverClass *)phys_obj->Get_Observer())->As_PhysicalGameObj();
		}

		if (game_obj != nullptr && game_obj->As_PhysicalGameObj() != nullptr ) {

			Vector3 obj_pos;
			game_obj->As_PhysicalGameObj()->Get_Position (&obj_pos);

			//
			//	Is this object the closest one we found yet?
			//
			float len = (obj_pos - pos).Length ();
			if (len >= min_dist && len <= closest_dist) {

				//
				//	If only_human, see if this is a human. Duh.
				//
				if (!only_human ||
					(game_obj->As_SmartGameObj() != nullptr &&
					 game_obj->As_SmartGameObj()->Is_Human_Controlled())) {

					closest_dist	= len;
					closest_obj		= game_obj;
				}
			}
		}
	}

	return closest_obj;
}


/*
**
*/
GameObject * Get_The_Star( void )
{
	return (GameObject*)COMBAT_STAR;
}

GameObject * Get_A_Star( const Vector3 & pos )
{
	SoldierGameObj * nearest_human_player = nullptr;
	Vector3 n_c_pos = Vector3( 1000000,1000000,1000000 );
	n_c_pos += pos;

	SLNode<SmartGameObj> *objnode;
	for (	objnode = GameObjManager::Get_Smart_Game_Obj_List()->Head(); objnode; objnode = objnode->Next() ) {
		SoldierGameObj * soldier = objnode->Data()->As_SoldierGameObj();
		if ( soldier && soldier->Is_Human_Controlled() ) {
			Vector3 c_pos;
			soldier->Get_Position( &c_pos );
			c_pos -= pos;
			if ( c_pos.Length2() < n_c_pos.Length2() ) {
				nearest_human_player = soldier;
				n_c_pos = c_pos;
			}
		}
	}
	return nearest_human_player;
}

bool Is_A_Star( GameObject * obj )
{
	SCRIPT_PTR_CHECK_RET( obj, false );
	if ( obj->As_SmartGameObj() ) {
		SoldierGameObj * soldier = obj->As_SmartGameObj()->As_SoldierGameObj();
		return soldier && soldier->Is_Human_Controlled();
	}
	return false;
}

/*
** HACK
*/
const char * Get_Damage_Bone_Name( void )
{
	return DamageModelName;
}
bool Get_Damage_Bone_Direction( void )
{
	return DamageModelDirection;
}


/*
**
*/
void	Control_Enable( GameObject * obj, bool enable )
{
	SCRIPT_PTR_CHECK( obj );
	SCRIPT_TRACE((	"ST>Control_Enable( %d, %d )\n", obj->Get_ID(), enable ));
	SmartGameObj *smart = obj->As_SmartGameObj();
	if (smart) {
		smart->Control_Enable( enable );
	} else {
		Debug_Say(( "This object can't Control_Enable\n" ));
	}
}


/*
**
*/
bool	Is_Object_Visible( GameObject * looker, GameObject * obj )
{
	SCRIPT_PTR_CHECK_RET( looker, false );
	SCRIPT_PTR_CHECK_RET( obj, false );
	SCRIPT_TRACE((	"ST>Is_Object_Visible( %d, %d )\n", looker->Get_ID(), obj->Get_ID() ));
	SmartGameObj *smart = looker->As_SmartGameObj();
	if (smart) {

		PhysicalGameObj * pgobj = obj->As_PhysicalGameObj();
		if ( pgobj == nullptr ) {
			Debug_Say(( "Not a PhysicalGameObj at %s %d\n", __FILE__, __LINE__ ));
			return false;
		}

		return smart->Is_Obj_Visible( pgobj );
	} else {
		Debug_Say(( "This object can't Is_Object_Visible\n" ));
	}
	return false;
}


void	Enable_Enemy_Seen( GameObject * obj, bool enable )
{
	SCRIPT_PTR_CHECK( obj );
	SCRIPT_TRACE((	"ST>Enable_Enemy_Seen( %d, %d )\n", obj->Get_ID(), enable ));
	SmartGameObj *smart = obj->As_SmartGameObj();
	if (smart) {
		smart->Set_Enemy_Seen_Enabled( enable );
	} else {
		Debug_Say(( "Only Smart Objs can Enable_Enemy_Seen\n" ));
	}
}

/*
**
*/

Vector3 _DisplayColor( 1,1,1 );

void	Set_Display_Color( unsigned char red, unsigned char green, unsigned char blue )
{
	_DisplayColor = Vector3( (float)red/255.0f, (float)green/255.0f, (float)blue/255.0f );
}

void	Display_Text( int string_id )
{
	TDBObjClass *	 obj = TranslateDBClass::Find_Object( string_id );

	if ( obj ) {
		DebugManager::Display_Text( obj->Get_String(), _DisplayColor );

		int sound_def_id = (int)obj->Get_Sound_ID ();
		if (sound_def_id > 0) {
			//	Create the sound object
			AudibleSoundClass * sound = WWAudioClass::Get_Instance ()->Create_Sound (sound_def_id);
			if (sound != nullptr) {
				sound->Add_To_Scene();
				sound->Release_Ref();
			}
		}
	}
}

void	Display_Float( float value, const char * format )
{
	WideStringClass wide_format;
	wide_format.Convert_From( format );
	WideStringClass string;
	string.Format( wide_format, value );
	DebugManager::Display_Text( string, _DisplayColor );
}

void	Display_Int( int value, const char * format )
{
	WideStringClass wide_format;
	wide_format.Convert_From( format );
	WideStringClass string;
	string.Format( wide_format, value );
	DebugManager::Display_Text( string, _DisplayColor );
}

/*
**
*/
void	Save_Data( ScriptSaver & saver, int id, int size, void * data )
{
	SCRIPT_PTR_CHECK( data );
	saver.CSave.Begin_Micro_Chunk(id);
	saver.CSave.Write(data,size);
	saver.CSave.End_Micro_Chunk();
	WWASSERT((unsigned)size <= 250);		// Make sure we don't save too much
}


void Save_Pointer(ScriptSaver& saver, int /* id */, void* pointer)
{
	SCRIPT_PTR_CHECK(pointer);
    const uint32_t pointer_id = SaveLoadSystemClass::Serialize_Pointer(pointer);
	Save_Data(saver, pointer_id, sizeof(uint32_t), pointer);
}


bool	Load_Begin( ScriptLoader & loader, int * id )
{
	SCRIPT_PTR_CHECK_RET( id, false );
	if ( !loader.CLoad.Open_Micro_Chunk()) {
		return false;
	}
	*id = loader.CLoad.Cur_Micro_Chunk_ID();
	return true;
}

void	Load_Data( ScriptLoader & loader, [[maybe_unused]] int size, void * data )
{
	SCRIPT_PTR_CHECK( data );
	unsigned int chunkSize = loader.CLoad.Cur_Micro_Chunk_Length();
	WWASSERT((unsigned)size >= chunkSize);
	loader.CLoad.Read(data, chunkSize);
	WWASSERT((unsigned)size <= 250);		// Make sure we don't save too much
}


void Load_Pointer(ScriptLoader& loader, void** pointer)
{
	SCRIPT_PTR_CHECK(pointer);
	Load_Data(loader, sizeof(void*), pointer);
	REQUEST_POINTER_REMAP(pointer);
}


void	Load_End( ScriptLoader & loader )
{
	loader.CLoad.Close_Micro_Chunk();
}

void Begin_Chunk(ScriptSaver& saver, unsigned int chunkID)
{
	saver.CSave.Begin_Chunk(chunkID);
}


void End_Chunk(ScriptSaver& saver)
{
	saver.CSave.End_Chunk();
}


bool Open_Chunk(ScriptLoader& loader, unsigned int* chunkID)
{
	SCRIPT_PTR_CHECK_RET(chunkID, false);

	if (loader.CLoad.Open_Chunk()) {
		*chunkID = loader.CLoad.Cur_Chunk_ID();
		return true;
	}

	return false;
}


void Close_Chunk(ScriptLoader& loader)
{
	loader.CLoad.Close_Chunk();
}


//
//	Clear_Map_Cell
//
void
Clear_Map_Cell( int cell_x, int cell_y )
{
	SCRIPT_TRACE((	"ST>Clear_Map_Cell( %d, %d)\n", cell_x, cell_y ));
	MapMgrClass::Clear_Cloud_Cell( cell_x, cell_y );
	HUDClass::Add_Map_Reveal();
	return ;
}


//
//	Clear_Map_Cell_By_Pos
//
void
Clear_Map_Cell_By_Pos( const Vector3 &world_space_pos )
{
	SCRIPT_TRACE((	"ST>Clear_Map_Cell_By_Pos( %f, %f, %f)\n", world_space_pos.X, world_space_pos.Y, world_space_pos.Z ));
	MapMgrClass::Clear_Cloud_Cell( world_space_pos );
	HUDClass::Add_Map_Reveal();
	return ;
}


//
//	Clear_Map_Region_By_Pos
//
void
Clear_Map_Region_By_Pos( const Vector3 &world_space_pos, int pixel_radius )
{
	SCRIPT_TRACE((	"ST>Clear_Map_Region_By_Pos( %f, %f, %f, %d)\n", world_space_pos.X, world_space_pos.Y, world_space_pos.Z, pixel_radius ));
	MapMgrClass::Clear_Cloud_Cells( world_space_pos, pixel_radius );
	HUDClass::Add_Map_Reveal();
	return ;
}


//
//	Clear_Map_Cell_By_Pixel_Pos
//
void
Clear_Map_Cell_By_Pixel_Pos( int pixel_pos_x, int pixel_pos_y )
{
	SCRIPT_TRACE((	"ST>Clear_Map_Cell_By_Pixel_Pos( %d, %d)\n", pixel_pos_x, pixel_pos_y ));
	MapMgrClass::Clear_Cloud_Cell_By_Pixel( pixel_pos_x, pixel_pos_y );
	HUDClass::Add_Map_Reveal();
	return ;
}


//
//	Show_Player_Map_Marker
//
void
Show_Player_Map_Marker( bool onoff )
{
	SCRIPT_TRACE((	"ST>Show_Player_Map_Marker()\n" ));
	MapMgrClass::Show_Player_Marker( onoff );
	return ;
}


//
//	Reveal_Map
//
void
Reveal_Map( void )
{
	SCRIPT_TRACE((	"ST>Reveal_Map()\n" ));
	MapMgrClass::Clear_All_Cloud_Cells();
	HUDClass::Add_Map_Reveal();
	return ;
}

//
//	Shroud_Map
//
void
Shroud_Map( void )
{
	SCRIPT_TRACE((	"ST>Shroud_Map()\n" ));
	MapMgrClass::Cloud_All_Cells();
	return ;
}

//
//	Get_Safe_Flight_Height
//
float
Get_Safe_Flight_Height (float x_pos, float y_pos)
{
	SCRIPT_TRACE((	"ST>Get_Safe_Flight_Height( %.2f, %.2f )\n", x_pos, y_pos ));
	return HeightDBClass::Get_Height ( Vector3 ( x_pos, y_pos, 0 ) );
}


/*
**
*/
void Clear_Radar_Markers( void )
{
	SCRIPT_TRACE((	"ST>Clear_Radar_Markers()\n" ));
	RadarManager::Clear_Markers();
}

void	Clear_Radar_Marker( int id )
{
	SCRIPT_TRACE((	"ST>Clear_Radar_Marker( %d )\n", id ));
	RadarManager::Clear_Marker( id );
}

#if 0
void	Add_Radar_Marker( int id, const Vector3& position, const Vector3& color, bool flash )
{
	SCRIPT_TRACE((	"ST>Add_Radar_Marker( %d, (%f,%f,%f), (%f,%f,%f), %d)\n",
			id, position.X, position.Y, position.Z, color.X, color.Y, color.Z, flash ));
/*	RadarMarkerClass marker;
	marker.ID = id;
	marker.Position = position;
	marker.Color = color;
	marker.Flash = flash;
	RadarManager::Add_Marker( marker );*/
}
#else
void	Add_Radar_Marker( int id, const Vector3& position, int shape_type, int color_type )
{
	SCRIPT_TRACE((	"ST>Add_Radar_Marker( %d, (%f,%f,%f), %d, %d)\n",
			id, position.X, position.Y, position.Z, shape_type, color_type ));

	RadarMarkerClass marker;
	marker.ID = id;
	marker.Position = position;
	marker.Type = shape_type;
	marker.Color = color_type;
	RadarManager::Add_Marker( marker );
}
#endif

#if 0
void	Add_Obj_Radar_Marker( int id, GameObject * obj, Vector3 color, bool flash )
{
	SCRIPT_PTR_CHECK( obj );
	SCRIPT_TRACE((	"ST>Add_Radar_Marker( %d, %d, (%f,%f,%f), %d)\n",
			id, obj->Get_ID(), color.X, color.Y, color.Z, flash ));
	SCRIPT_PTR_CHECK( obj );
/*	RadarMarkerClass marker;
	marker.ID = id;*/

	PhysicalGameObj * pgobj = obj->As_PhysicalGameObj();
	if ( pgobj == nullptr ) {
		Debug_Say(( "Not a PhysicalGameObj at %s %d\n", __FILE__, __LINE__ ));
		return;
	}

/*	marker.Set_Game_Object( pgobj );
	marker.Color = color;
	marker.Flash = flash;
	RadarManager::Add_Marker( marker );*/
}
#endif

void	Set_Obj_Radar_Blip_Shape( GameObject * obj, int shape_type )
{
	SCRIPT_PTR_CHECK( obj );
	SCRIPT_TRACE((	"ST>Set_Obj_Radar_Blip_Shape( %d, %d )\n", obj->Get_ID(), shape_type ));

	PhysicalGameObj * pgobj = obj->As_PhysicalGameObj();
	if ( pgobj == nullptr ) {
		Debug_Say(( "Not a PhysicalGameObj at %s %d\n", __FILE__, __LINE__ ));
		return;
	}
	if ( shape_type < 0 ) {
		pgobj->Reset_Radar_Blip_Shape_Type();
	} else {
		pgobj->Set_Radar_Blip_Shape_Type( shape_type );
	}
}

void	Set_Obj_Radar_Blip_Color( GameObject * obj, int color_type )
{
	SCRIPT_PTR_CHECK( obj );
	SCRIPT_TRACE((	"ST>Set_Obj_Radar_Blip_Color( %d, %d )\n", obj->Get_ID(), color_type ));

	PhysicalGameObj * pgobj = obj->As_PhysicalGameObj();
	if ( pgobj == nullptr ) {
		Debug_Say(( "Not a PhysicalGameObj at %s %d\n", __FILE__, __LINE__ ));
		return;
	}
	if ( color_type < 0 ) {
		pgobj->Reset_Radar_Blip_Color_Type();
	} else {
		pgobj->Set_Radar_Blip_Color_Type( color_type );
	}
}

void	Enable_Radar( bool enable )
{
	SCRIPT_TRACE((	"ST>Enable_Radar( %d )\n", enable ));
	RadarManager::Set_Hidden( !enable );
}


/*
**
*/
void	Create_Explosion( const char * explosion_def_name, const Vector3 & pos, GameObject * creator )
{
	SCRIPT_PTR_CHECK( explosion_def_name );
	SCRIPT_TRACE((	"ST>Create_Explosion( %s, (%f,%f,%f), %d )\n",
		explosion_def_name, pos.X, pos.Y, pos.Z, (creator != nullptr ) ? creator->Get_ID() : 0 ));

	ExplosionDefinitionClass * explosion_def = (ExplosionDefinitionClass *)DefinitionMgrClass::Find_Typed_Definition( explosion_def_name, CLASSID_DEF_EXPLOSION );
	if ( explosion_def != nullptr )
	{
		int explosion_id = explosion_def->Get_ID();
		SmartGameObj * smart = nullptr;
		if ( creator ) {
			smart = creator->As_SmartGameObj();
		}
		ExplosionManager::Create_Explosion_At( explosion_id, pos, smart );
	}
}

void	Create_Explosion_At_Bone( const char * explosion_def_name, GameObject * object, const char * bone_name, GameObject * creator )
{
	SCRIPT_PTR_CHECK( explosion_def_name );
	SCRIPT_PTR_CHECK( object );
	SCRIPT_PTR_CHECK( bone_name );
	SCRIPT_TRACE((	"ST>Create_Explosion_At_Bone( %s, %d, %s, %d )\n",
		explosion_def_name,
		(object != nullptr) ? object->Get_ID() : 0 ,
		bone_name,
		(creator != nullptr ) ? creator->Get_ID() : 0 ));

	ExplosionDefinitionClass * explosion_def = (ExplosionDefinitionClass *)DefinitionMgrClass::Find_Typed_Definition( explosion_def_name, CLASSID_DEF_EXPLOSION );
	if ( explosion_def != nullptr )
	{
		int explosion_id = explosion_def->Get_ID();
		SmartGameObj * smart = nullptr;
		if ( creator ) {
			smart = creator->As_SmartGameObj();
		}

		PhysicalGameObj * pgobj = object->As_PhysicalGameObj();
		if ( pgobj == nullptr ) {
			Debug_Say(( "Not a PhysicalGameObj at %s %d\n", __FILE__, __LINE__ ));
			return;
		}

		Matrix3D tm = pgobj->Peek_Model()->Get_Bone_Transform( bone_name );

		ExplosionManager::Create_Explosion_At( explosion_id, tm, smart );
	}

}



/*
**
*/
void	Enable_HUD( bool enable )
{
	SCRIPT_TRACE((	"ST>Enable_HUD( %d )\n", enable ));
	HUDClass::Enable( enable );
}


void	Mission_Complete( bool success )
{
	SCRIPT_TRACE((	"ST>Mission_Complete( %d )\n", success ));
   CombatManager::Mission_Complete( success );
}


void	Give_PowerUp( GameObject * obj, const char * preset_name, bool display_on_hud )
{
	SCRIPT_PTR_CHECK( obj );
	SCRIPT_PTR_CHECK( preset_name );
	SCRIPT_TRACE((	"ST>Give_PowerUp( %d, %s )\n", obj->Get_ID(), preset_name ));
	SmartGameObj *smart = obj->As_SmartGameObj();
	if (smart) {

		PowerUpGameObjDef *	def = (PowerUpGameObjDef*)DefinitionMgrClass::Find_Typed_Definition( preset_name, CLASSID_GAME_OBJECT_DEF_POWERUP );
		if ( def != nullptr ) {
			def->Grant( smart, nullptr, display_on_hud );
		} else {
			Debug_Say(( "Powerup Definition %s not found\n", preset_name ));
		}

	} else {
		Debug_Say(( "Only Samrt objs can get powerups\n" ));
	}
}


bool Innate_Soldier_Enable( GameObject * obj, int bits, bool state )
{
	SCRIPT_PTR_CHECK_RET(obj, false);
	SCRIPT_TRACE((	"ST>Innate_Soldier_Enable( %d, %d, %d )\n", obj->Get_ID(), bits, state ));

	PhysicalGameObj * pgobj = obj->As_PhysicalGameObj();
	if ( pgobj == nullptr ) {
//		Cinematics call this for non-soldiers
//		Debug_Say(( "Not a PhysicalGameObj at %s %d\n", __FILE__, __LINE__ ));
		return false;
	}

	SoldierGameObj * soldier = pgobj->As_SoldierGameObj();
	if ( soldier == nullptr ) {
//		Cinematics call this for non-soldiers
//		Debug_Say(( "Object is not a soldier!\n" ));
	}
	SCRIPT_PTR_CHECK_RET(soldier, false);
	bool old_state = soldier->Is_Innate_Enabled( bits );
	if ( state ) {
		soldier->Innate_Enable( bits );
	} else {
		soldier->Innate_Disable( bits );
	}
	return old_state;
}


bool	Innate_Soldier_Enable_Enemy_Seen( GameObject * obj, bool state )
{
	return Innate_Soldier_Enable( obj, SOLDIER_INNATE_EVENT_ENEMY_SEEN, state );
}


bool	Innate_Soldier_Enable_Gunshot_Heard( GameObject * obj, bool state )
{
	return Innate_Soldier_Enable( obj, SOLDIER_INNATE_EVENT_GUNSHOT_HEARD, state );
}


bool	Innate_Soldier_Enable_Footsteps_Heard( GameObject * obj, bool state )
{
	return Innate_Soldier_Enable( obj, SOLDIER_INNATE_EVENT_FOOTSTEP_HEARD, state );
}


bool	Innate_Soldier_Enable_Bullet_Heard( GameObject * obj, bool state )
{
	return Innate_Soldier_Enable( obj, SOLDIER_INNATE_EVENT_BULLET_HEARD, state );
}


bool	Innate_Soldier_Enable_Actions( GameObject * obj, bool state )
{
	return Innate_Soldier_Enable( obj, SOLDIER_INNATE_ACTIONS, state );
}


// Disable all innate abilities
void Innate_Disable(GameObject* object)
{
	Innate_Soldier_Enable(object, 0xFFFFFFFF, false);
}


// Enable all innate abilities
void Innate_Enable(GameObject* object)
{
	Innate_Soldier_Enable(object, 0xFFFFFFFF, true);
}


void	Set_Innate_Soldier_Home_Location( GameObject * obj, const Vector3& home_pos, float home_radius )
{
	SCRIPT_PTR_CHECK( obj );
	SCRIPT_TRACE((	"ST>Set_Innate_Soldier_Home_Location( %d, (%f %f %f), %f )\n", obj->Get_ID(), home_pos.X, home_pos.Y, home_pos.Z, home_radius ));

	SmartGameObj * smart = obj->As_SmartGameObj();
	if ( smart != nullptr ) {
		SoldierGameObj * soldier = smart->As_SoldierGameObj();
		if ( soldier != nullptr ) {
			SoldierObserverClass * innate = soldier->Get_Innate_Controller();
			if ( innate ) {
				innate->Set_Home_Location( home_pos, home_radius );
			}
		}
	}

}

void	Set_Innate_Aggressiveness( GameObject * obj, float aggressiveness )
{
	SCRIPT_PTR_CHECK( obj );
	SCRIPT_TRACE((	"ST>Set_Innate_Aggressiveness( %d, %f )\n", obj->Get_ID(), aggressiveness ));

	bool set = false;
	SmartGameObj * smart = obj->As_SmartGameObj();
	if ( smart != nullptr ) {
		SoldierGameObj * soldier = smart->As_SoldierGameObj();
		if ( soldier != nullptr ) {
			SoldierObserverClass * innate = soldier->Get_Innate_Controller();
			if ( innate ) {
				innate->Set_Aggressiveness( aggressiveness );
				set = true;
			}
		}
	}
	if ( !set ) {
		Debug_Say(( "Can only Set_Innate_Aggressiveness on a soldier\n" ));
	}
}

void	Set_Innate_Take_Cover_Probability( GameObject * obj, float probability )
{
	SCRIPT_PTR_CHECK( obj );
	SCRIPT_TRACE((	"ST>Set_Innate_Take_Cover_Probability( %d, %f )\n", obj->Get_ID(), probability ));

	bool set = false;
	SmartGameObj * smart = obj->As_SmartGameObj();
	if ( smart != nullptr ) {
		SoldierGameObj * soldier = smart->As_SoldierGameObj();
		if ( soldier != nullptr ) {
			SoldierObserverClass * innate = soldier->Get_Innate_Controller();
			if ( innate ) {
				innate->Set_Take_Cover_Probability( probability );
				set = true;
			}
		}
	}
	if ( !set ) {
		Debug_Say(( "Can only Set_Innate_Take_Cover_Probability on a soldier\n" ));
	}
}

void	Set_Innate_Is_Stationary( GameObject * obj, bool stationary )
{
	SCRIPT_PTR_CHECK( obj );
	SCRIPT_TRACE((	"ST>Set_Innate_Take_Cover_Probability( %d, %d )\n", obj->Get_ID(), stationary ));

	bool set = false;
	SmartGameObj * smart = obj->As_SmartGameObj();
	if ( smart != nullptr ) {
		SoldierGameObj * soldier = smart->As_SoldierGameObj();
		if ( soldier != nullptr ) {
			SoldierObserverClass * innate = soldier->Get_Innate_Controller();
			if ( innate ) {
				innate->Set_Is_Stationary( stationary );
				set = true;
			}
		}
	}
	if ( !set ) {
		Debug_Say(( "Can only Set_Innate_Is_Stationary on a soldier\n" ));
	}
}


void	Innate_Force_State_Bullet_Heard( GameObject * obj, const Vector3 & pos )
{
	SCRIPT_PTR_CHECK( obj );
	SCRIPT_TRACE((	"ST>Innate_Force_State_Bullet_Heard( %d, ( %f, %f, %f, )  )\n", obj->Get_ID(), pos.X, pos.Y, pos.Z ));

	bool set = false;
	SmartGameObj * smart = obj->As_SmartGameObj();
	if ( smart != nullptr ) {
		SoldierGameObj * soldier = smart->As_SoldierGameObj();
		if ( soldier != nullptr ) {
			SoldierObserverClass * innate = soldier->Get_Innate_Controller();
			if ( innate ) {
				innate->Set_State( soldier, SoldierObserverClass::SOLDIER_AI_BULLET_HEARD, pos);
				set = true;
			}
		}
	}
	if ( !set ) {
		Debug_Say(( "Can only Innate_Force_State_Bullet_Heard on a soldier with innate\n" ));
	}
}

void	Innate_Force_State_Footsteps_Heard( GameObject * obj, const Vector3 & pos )
{
	SCRIPT_PTR_CHECK( obj );
	SCRIPT_TRACE((	"ST>Innate_Force_State_Footsteps_Heard( %d, ( %f, %f, %f, )  )\n", obj->Get_ID(), pos.X, pos.Y, pos.Z ));

	bool set = false;
	SmartGameObj * smart = obj->As_SmartGameObj();
	if ( smart != nullptr ) {
		SoldierGameObj * soldier = smart->As_SoldierGameObj();
		if ( soldier != nullptr ) {
			SoldierObserverClass * innate = soldier->Get_Innate_Controller();
			if ( innate ) {
				innate->Set_State( soldier, SoldierObserverClass::SOLDIER_AI_FOOTSTEPS_HEARD, pos);
				set = true;
			}
		}
	}
	if ( !set ) {
		Debug_Say(( "Can only Innate_Force_State_Footsteps_Heard on a soldier with innate\n" ));
	}
}

void	Innate_Force_State_Gunshots_Heard( GameObject * obj, const Vector3 & pos )
{
	SCRIPT_PTR_CHECK( obj );
	SCRIPT_TRACE((	"ST>Innate_Force_State_Gunshot_Heard( %d, ( %f, %f, %f, )  )\n", obj->Get_ID(), pos.X, pos.Y, pos.Z ));

	bool set = false;
	SmartGameObj * smart = obj->As_SmartGameObj();
	if ( smart != nullptr ) {
		SoldierGameObj * soldier = smart->As_SoldierGameObj();
		if ( soldier != nullptr ) {
			SoldierObserverClass * innate = soldier->Get_Innate_Controller();
			if ( innate ) {
				innate->Set_State( soldier, SoldierObserverClass::SOLDIER_AI_GUNSHOT_HEARD, pos);
				set = true;
			}
		}
	}
	if ( !set ) {
		Debug_Say(( "Can only Innate_Force_State_Gunshot_Heard on a soldier with innate\n" ));
	}
}

void	Innate_Force_State_Enemy_Seen( GameObject * obj, GameObject * enemy )
{
	SCRIPT_PTR_CHECK( obj );
	SCRIPT_PTR_CHECK( enemy );
	SCRIPT_TRACE((	"ST>Innate_Force_State_Enemy_Seen( %d, %d )\n", obj->Get_ID(), enemy->Get_ID() ));

	bool set = false;
	SmartGameObj * smart = obj->As_SmartGameObj();
	if ( smart != nullptr ) {
		SoldierGameObj * soldier = smart->As_SoldierGameObj();
		if ( soldier != nullptr ) {
			SoldierObserverClass * innate = soldier->Get_Innate_Controller();
			if ( innate ) {
				innate->Set_State( soldier, SoldierObserverClass::SOLDIER_AI_ENEMY_SEEN, Vector3(0,0,0), enemy );
				set = true;
			}
		}
	}
	if ( !set ) {
		Debug_Say(( "Can only Innate_Force_State_Enemy_Seen on a soldier with innate\n" ));
	}
}


/*
**
*/

void	Static_Anim_Phys_Goto_Frame( int obj_id, float frame, const char * anim_name )
{
	SCRIPT_TRACE((	"ST>Static_Anim_Phys_Goto_Frame( %d, %d )\n", obj_id, frame ));

	StaticPhysClass * spc = COMBAT_SCENE->Find_Static_Object( obj_id );
	if ( spc != nullptr ) {
		StaticAnimPhysClass * sapc = spc->As_StaticAnimPhysClass();
		if ( sapc  ) {
			if ( anim_name != nullptr ) {
				sapc->Get_Animation_Manager().Set_Animation( anim_name );
			}
			sapc->Get_Animation_Manager().Set_Animation_Mode( AnimCollisionManagerClass::ANIMATE_TARGET );
			sapc->Get_Animation_Manager().Set_Target_Frame( frame );
		}
	}
}


void	Static_Anim_Phys_Goto_Last_Frame( int obj_id, const char * anim_name )
{
	SCRIPT_TRACE((	"ST>Static_Anim_Phys_Goto_Last_Frame( %d )\n", obj_id ));

	StaticPhysClass * spc = COMBAT_SCENE->Find_Static_Object( obj_id );
	if ( spc != nullptr ) {
		StaticAnimPhysClass * sapc = spc->As_StaticAnimPhysClass();
		if ( sapc  ) {
			if ( anim_name != nullptr ) {
				sapc->Get_Animation_Manager().Set_Animation( anim_name );
			}
			sapc->Get_Animation_Manager().Set_Animation_Mode( AnimCollisionManagerClass::ANIMATE_TARGET );
			sapc->Get_Animation_Manager().Set_Target_Frame( float(sapc->Get_Animation_Manager().Peek_Animation()->Get_Num_Frames() - 1) );
		}
	}
}

unsigned int Get_Sync_Time( void )
{
	return CombatManager::Get_Sync_Time();
}

/*
** Objectives
*/
void	Add_Objective( int id, int type,  int status, int short_description_id, const char * description_sound_filename, int long_description_id )
{
	SCRIPT_TRACE((	"ST>Add_Objective( %d, %d, %d, %d %d )\n", id, type, status, short_description_id, long_description_id ));
	ObjectiveManager::Add_Objective( id, type, status, short_description_id, long_description_id, description_sound_filename );
}

void	Remove_Objective( int id )
{
	SCRIPT_TRACE((	"ST>Remove_Objective( %d )\n", id ));
	ObjectiveManager::Remove_Objective( id );
}

void	Set_Objective_Status( int id, int status )
{
	SCRIPT_TRACE((	"ST>Set_Objective_Status( %d, %d )\n", id, status ));
	ObjectiveManager::Set_Objective_Status( id, status );
}

void	Change_Objective_Type( int id, int type )
{
	SCRIPT_TRACE((	"ST>Change_Objective_Type( %d, %d )\n", id, type ));
	ObjectiveManager::Change_Objective_Type( id, type );
}

void	Set_Objective_Radar_Blip( int id, const Vector3 & position )
{
	SCRIPT_TRACE((	"ST>Set_Objective_Radar_Blip( %d, %f %f %f )\n", id, position.X, position.Y, position.Z ));
	ObjectiveManager::Set_Objective_Radar_Blip( id, position );
}

void	Set_Objective_Radar_Blip_Object( int id, ScriptableGameObj * unit )
{
	SCRIPT_PTR_CHECK( unit );
	SCRIPT_TRACE((	"ST>Set_Objective_Radar_Blip_Object( %d, %d )\n", id, unit->Get_ID() ));
	PhysicalGameObj * pobj = nullptr;
	if ( unit ) {
		pobj = unit->As_PhysicalGameObj();
	}
	ObjectiveManager::Set_Objective_Radar_Blip( id, pobj );
}

void	Set_Objective_HUD_Info( int id, float priority, const char * texture_name, int message_id )
{
	SCRIPT_PTR_CHECK( texture_name );
	SCRIPT_TRACE((	"ST>Set_Objective_HUD_Info( %d, %f, %s, %d )\n", id, priority, texture_name, message_id ));
	ObjectiveManager::Set_Objective_HUD_Info( id, priority, texture_name, message_id );
}

void	Set_Objective_HUD_Info_Position( int id, float priority, const char * texture_name, int message_id, const Vector3 & position )
{
	SCRIPT_PTR_CHECK( texture_name );
	SCRIPT_TRACE((	"ST>Set_Objective_HUD_Info_Position( %d, %f, %s, %d )\n", id, priority, texture_name, message_id ));
	ObjectiveManager::Set_Objective_HUD_Info( id, priority, texture_name, message_id, position );
}

/*
**
*/
void	Shake_Camera( const Vector3 & pos, float radius, float intensity, float duration )
{
	SCRIPT_TRACE((	"ST>Shake_Camera( (%f, %f, %f), %f, %f, %f\n",pos.X, pos.Y, pos.Z, radius, intensity, duration ));
	if ( COMBAT_SCENE ) {
		COMBAT_SCENE->Add_Camera_Shake( pos, radius, duration, intensity );
	}
}


void	Enable_Spawner( int id, bool enable )
{
	SCRIPT_TRACE((	"ST>Enable_Spawned( %d, %d )\n",id, enable ));
	SpawnManager::Spawner_Enable( id, enable );
}

GameObject * Trigger_Spawner( int id )
{
	SCRIPT_TRACE((	"ST>Trigger_Spawned( %d )\n", id ));
	return SpawnManager::Spawner_Trigger( id );
}

void	Enable_Engine( GameObject* object, bool onoff )
{
	SCRIPT_PTR_CHECK( object );
	SCRIPT_TRACE((	"ST>Enable_Engine( %d, %d )\n", object, onoff ));

	PhysicalGameObj *physical_obj = object->As_PhysicalGameObj ();
	if ( physical_obj != nullptr ) {
		VehicleGameObj *vehicle = physical_obj->As_VehicleGameObj ();
		if ( vehicle != nullptr ) {
			vehicle->Enable_Engine( onoff );
		}
	}
}


/*
**
*/
int	Get_Difficulty_Level( void )
{
	return CombatManager::Get_Difficulty_Level();
}

void	Grant_Key( GameObject* object, int key, bool grant )
{
	SCRIPT_PTR_CHECK( object );
	SCRIPT_TRACE((	"ST>Grant_Key( %d, %d )\n", object->Get_ID(), key, grant ));

	SoldierGameObj * soldier = nullptr;
	if ( object->As_SmartGameObj() != nullptr ) {
		soldier = object->As_SmartGameObj()->As_SoldierGameObj();
	}

	if ( soldier == nullptr ) {
		Debug_Say(( "Object is not a soldier!\n" ));
	} else {
		if ( grant ) {
			soldier->Give_Key( key );
		} else {
			soldier->Remove_Key( key );
		}
	}
}

bool	Has_Key( GameObject* object, int key )
{
	SoldierGameObj * soldier = nullptr;
	if ( object->As_SmartGameObj() != nullptr ) {
		soldier = object->As_SmartGameObj()->As_SoldierGameObj();
	}

	return soldier && soldier->Has_Key( key );
}

void	Enable_Hibernation( GameObject * object, bool enable )
{
	SCRIPT_PTR_CHECK( object );
	SCRIPT_TRACE((	"ST>Enable_Hibernation( %d, %d )\n", object->Get_ID(), enable ));

	if ( object->As_PhysicalGameObj() ) {
		object->As_PhysicalGameObj()->Enable_Hibernation( enable );
	}
}

void	Attach_To_Object_Bone( GameObject * object, GameObject * host_object, const char * bone_name )
{
	SCRIPT_PTR_CHECK( object );
	if ( host_object == nullptr ) {
		SCRIPT_TRACE((	"ST>Attach_To_Object_Bone( %d, nullptr )\n", object->Get_ID() ));
	} else {
		SCRIPT_TRACE((	"ST>Attach_To_Object_Bone( %d, %d, %s )\n", object->Get_ID(), host_object->Get_ID(), bone_name ? "nullptr" : bone_name  ));
	}

	if ( object->As_PhysicalGameObj() ) {
		if ( host_object == nullptr ) {
			object->As_PhysicalGameObj()->Attach_To_Object_Bone( nullptr, nullptr );
		} else {
			WWASSERT( host_object->As_PhysicalGameObj() ) ;
			object->As_PhysicalGameObj()->Attach_To_Object_Bone( host_object->As_PhysicalGameObj(), bone_name );
		}
	} else {
		Debug_Say(( "Can only Attach_To_Object_Bone for PhysicalObjects\n" ));
	}
}

int	Create_Conversation( const char *conversation_name, int priority, float max_dist, bool is_interruptable )
{
	int active_conversation_id = -1;
	SCRIPT_TRACE((	"ST>Create_Conversation( %s )\n", conversation_name ));

	//
	//	Try to find the requested conversation
	//
	ConversationClass *conversation = ConversationMgrClass::Find_Conversation( conversation_name );
	if ( conversation != nullptr ) {

		//
		//	Create a new run-time conversation object
		//
		ActiveConversationClass *active_conversation = ConversationMgrClass::Create_New_Conversation( conversation );
		if (active_conversation != nullptr) {

			if (priority > 0) {
				active_conversation->Set_Priority (priority);
			}

			if (max_dist > 0) {
				active_conversation->Set_Max_Dist (max_dist);
			}

			active_conversation->Set_Is_Interruptable (is_interruptable);

			//
			//	Return the ID of the conversation to the caller
			//
			active_conversation_id = active_conversation->Get_ID ();
			REF_PTR_RELEASE( active_conversation );
		}

		REF_PTR_RELEASE( conversation );
	}

	return active_conversation_id;
}

void	Join_Conversation_Facing( GameObject * object, int active_conversation_id, int obj_id_to_face )
{
	SCRIPT_PTR_CHECK( object );
	SCRIPT_TRACE((	"ST>Join_Conversation_Facing( %d, %d, %d )\n", object->Get_ID (), active_conversation_id, obj_id_to_face ));

	//
	//	Dig out the soldier pointer (if we have one)
	//
	PhysicalGameObj *game_obj = nullptr;
	if ( object != nullptr ) {
		game_obj = object->As_PhysicalGameObj();
	}

	//
	//	Lookup the run-time conversation object
	//
	ActiveConversationClass *active_conversation = nullptr;
	active_conversation = ConversationMgrClass::Find_Active_Conversation( active_conversation_id );
	if (active_conversation != nullptr) {

		//
		//	Add this object to the conversation
		//
		OratorClass *orator = active_conversation->Add_Orator( game_obj );

		//
		//	Set the flags
		//
		orator->Set_Flag( OratorClass::FLAG_DONT_MOVE,			true );
		orator->Set_Flag( OratorClass::FLAG_DONT_TURN_HEAD,	false );
		orator->Set_Flag( OratorClass::FLAG_DONT_FACE,			false );
		orator->Set_Look_At_Obj( obj_id_to_face );

		REF_PTR_RELEASE( active_conversation );
	}

	return ;
}


void	Join_Conversation( GameObject * object, int active_conversation_id, bool allow_move, bool allow_head_turn, bool allow_face )
{
	int obj_id = 0;
	if ( object != nullptr ) {
		obj_id = object->Get_ID();
	}

	SCRIPT_TRACE((	"ST>Join_Conversation( %d, %d, %d, %d )\n", obj_id, active_conversation_id, allow_move, allow_head_turn ));

	//
	//	Dig out the soldier pointer (if we have one)
	//
	PhysicalGameObj *game_obj = nullptr;
	if ( object != nullptr ) {
		game_obj = object->As_PhysicalGameObj();
	}

	//
	//	Lookup the run-time conversation object
	//
	ActiveConversationClass *active_conversation = nullptr;
	active_conversation = ConversationMgrClass::Find_Active_Conversation( active_conversation_id);
	if (active_conversation != nullptr) {

		//
		//	Add this object to the conversation
		//
		OratorClass *orator = active_conversation->Add_Orator( game_obj );

		//
		//	Set the flags
		//
		orator->Set_Flag( OratorClass::FLAG_DONT_MOVE,			!allow_move );
		orator->Set_Flag( OratorClass::FLAG_DONT_TURN_HEAD,	!allow_head_turn );
		orator->Set_Flag( OratorClass::FLAG_DONT_FACE,			!allow_face );

		REF_PTR_RELEASE( active_conversation );
	}

	return ;
}


void	Stop_All_Conversations( void )
{
	SCRIPT_TRACE((	"ST>Stop_All_Conversations( )\n" ));

	//
	//	Simply ask the conversation manager to end all playing conversations
	//
	ConversationMgrClass::Reset_Active_Conversations();
	return ;
}


void	Stop_Conversation( int active_conversation_id )
{
	SCRIPT_TRACE((	"ST>Stop_Conversation( %d )\n", active_conversation_id ));

	//
	//	Lookup the run-time conversation object
	//
	ActiveConversationClass *active_conversation = nullptr;
	active_conversation = ConversationMgrClass::Find_Active_Conversation( active_conversation_id);
	if (active_conversation != nullptr) {

		//
		//	Stop the conversation
		//
		active_conversation->Stop_Conversation (ACTION_COMPLETE_CONVERSATION_INTERRUPTED);
	}

	return ;
}


void	Start_Conversation( int active_conversation_id, int action_id )
{
	SCRIPT_TRACE((	"ST>Start_Conversation( %d, %d )\n", active_conversation_id, action_id ));

	//
	//	Lookup the run-time conversation object
	//
	ActiveConversationClass *active_conversation = nullptr;
	active_conversation = ConversationMgrClass::Find_Active_Conversation( active_conversation_id);
	if (active_conversation != nullptr) {

		//
		//	Start the conversation
		//
		active_conversation->Set_Action_ID( action_id );
		active_conversation->Start_Conversation();
		REF_PTR_RELEASE( active_conversation );
	}

	return ;
}

void	Monitor_Conversation( GameObject * object, int active_conversation_id )
{
	SCRIPT_PTR_CHECK( object );
	SCRIPT_TRACE((	"ST>Monitor_Conversation( %d, %d )\n", object->Get_ID (), active_conversation_id ));

	//
	//	Lookup the run-time conversation object
	//
	ActiveConversationClass *active_conversation = nullptr;
	active_conversation = ConversationMgrClass::Find_Active_Conversation( active_conversation_id);
	if (active_conversation != nullptr) {

		//
		//	Start the conversation
		//
		active_conversation->Register_Monitor( object );
		REF_PTR_RELEASE( active_conversation );
	}

	return ;
}


void	Start_Random_Conversation( GameObject * object )
{
	SCRIPT_PTR_CHECK( object );
	SCRIPT_TRACE((	"ST>Start_Random_Conversation( %d )\n", object->Get_ID() ));

	//
	//	Dig the physical game obj out of the game object pointer
	//
	PhysicalGameObj *game_obj = nullptr;
	if ( object != nullptr ) {
		game_obj = object->As_PhysicalGameObj();
	}

	//
	//	Start any conversation
	//
	if ( game_obj != nullptr ) {
		ConversationMgrClass::Start_Conversation( game_obj );
	}

	return ;
}


/*
**
*/
void	Lock_Soldier_Facing( GameObject * object, GameObject * object_to_face, bool turn_body )
{
	SCRIPT_PTR_CHECK( object );
	SCRIPT_TRACE((	"ST>Lock_Soldier_Facing( %d, %d )\n", object->Get_ID(), turn_body ));

	//
	//	Is this a physical game object?
	//
	PhysicalGameObj *phys_game_obj = object->As_PhysicalGameObj();
	if (phys_game_obj != nullptr) {

		//
		//	Is this a soldier game object?
		//
		SoldierGameObj *soldier = phys_game_obj->As_SoldierGameObj();
		if ( soldier != nullptr ) {
			if ( object_to_face != nullptr ) {
				soldier->Lock_Facing( object_to_face->As_PhysicalGameObj(), turn_body );
			} else {
				soldier->Lock_Facing( nullptr, false );
			}
		}
	}

	return ;
}


/*
**
*/
void	Unlock_Soldier_Facing( GameObject * object )
{
	SCRIPT_PTR_CHECK( object );
	SCRIPT_TRACE((	"ST>Unlock_Soldier_Facing( %d )\n", object->Get_ID() ));

	//
	//	Is this a physical game object?
	//
	PhysicalGameObj *phys_game_obj = object->As_PhysicalGameObj();
	if (phys_game_obj != nullptr) {

		//
		//	Is this a soldier game object?
		//
		SoldierGameObj *soldier = phys_game_obj->As_SoldierGameObj();
		if ( soldier != nullptr ) {
			soldier->Lock_Facing( nullptr, false );
		}
	}

	return ;
}


/*
**
*/
void	Apply_Damage( GameObject * object, float amount, const char * warhead_name, GameObject * damager )
{
	SCRIPT_PTR_CHECK( object );
	SCRIPT_TRACE((	"ST>Apply_Damage( %d, %f, %s )\n", object->Get_ID(), amount, warhead_name ));

	DamageableGameObj * damgo = object->As_DamageableGameObj();
	if ( damgo ) {
		int warhead = ArmorWarheadManager::Get_Warhead_Type( warhead_name );
		SmartGameObj * smart = nullptr;
		if ( damager ) {
			smart = damager->As_SmartGameObj();
		}
		OffenseObjectClass offense( amount, warhead, smart );
		if ( damgo->As_SmartGameObj() ) {
			damgo->As_SmartGameObj()->Apply_Damage_Extended( offense );

			//
			//	Damage meshes normally follow a hit arriving over the wire.  A
			//	script doing the damage server-side skips that, so the vehicle
			//	stays looking undamaged on every machine until something else
			//	hits it.
			//
			VehicleGameObj *vehicle = damgo->As_SmartGameObj()->As_VehicleGameObj();
			if ( vehicle != nullptr ) {
				vehicle->Damage_Meshes_Update();
			}
		} else {
			damgo->Apply_Damage( offense );
		}
	} else {
		Debug_Say(( "Can only Apply_Damage to a DamageableGameObj\n" ));
	}
}

unsigned int	Get_Damage_Warhead( void )
{
	return DamageContextClass::Get_Warhead();
}

GameObject *	Get_Explosion_Object( void )
{
	return DamageContextClass::Get_Explosion_Object();
}

unsigned int	Get_Warhead_Type( const char * warhead_name )
{
	return ArmorWarheadManager::Get_Warhead_Type( warhead_name );
}

void	Set_Loiters_Allowed( GameObject * object, bool allowed )
{
	SCRIPT_PTR_CHECK( object );
	SCRIPT_TRACE((	"ST>Set_Loiters_Allowed( %d, %d )\n", object->Get_ID(), allowed ));

	PhysicalGameObj * physgo = object->As_PhysicalGameObj();
	SoldierGameObj * soldier = nullptr;
	if ( physgo ) {
		soldier = physgo->As_SoldierGameObj();
	}
	if ( soldier ) {
		soldier->Set_Loiters_Allowed( allowed );
	} else {
		Debug_Say(( "Can only Set_Loiters_Allowed to a SoldierGameObj\n" ));
	}

}

void	Set_Is_Visible( GameObject * object, bool visible )
{
	SCRIPT_PTR_CHECK( object );
	SCRIPT_TRACE((	"ST>Set_Is_Visible( %d, %d )\n", object->Get_ID(), visible ));

	PhysicalGameObj * physgo = object->As_PhysicalGameObj();
	SoldierGameObj * soldier = nullptr;
	if ( physgo ) {
		soldier = physgo->As_SoldierGameObj();
	}
	if ( soldier ) {
		soldier->Set_Is_Visible( visible );
	} else {
		Debug_Say(( "Can only Set_Is_Visible on a SoldierGameObj\n" ));
	}
}


void	Set_Is_Rendered( GameObject * object, bool rendered )
{
	SCRIPT_PTR_CHECK( object );
	SCRIPT_TRACE((	"ST>Set_Is_Rendered( %d, %d )\n", object->Get_ID(), rendered ));

	PhysicalGameObj * physgo = object->As_PhysicalGameObj();
	if ( physgo ) {
		physgo->Peek_Model()->Set_Hidden( !rendered );
		physgo->Set_Object_Dirty_Bit (NetworkObjectClass::BIT_RARE, true);
	} else {
		Debug_Say(( "Can only Set_Is_Rendered on a PhysicalGameObj\n" ));
	}
}


/*
**
*/
float	Get_Points( GameObject * object )
{
	if ( object && object->As_SmartGameObj() && object->As_SmartGameObj()->Get_Player_Data() ) {
		return object->As_SmartGameObj()->Get_Player_Data()->Get_Score();
	}
	Debug_Say(( "Cannot Get Points for object\n" ));
	return 0;
}

void	Give_Points( GameObject * object, float points, bool entire_team )
{
	SCRIPT_PTR_CHECK( object );
	SCRIPT_TRACE((	"ST>Give_Points( %d, %f, %d )\n", object->Get_ID(), points, entire_team ));

	SmartGameObj * smart = object->As_SmartGameObj();
	if ( smart ) {
		if ( entire_team ) {
			SLNode<SoldierGameObj> *objnode;
			for (	objnode = GameObjManager::Get_Star_Game_Obj_List()->Head(); objnode; objnode = objnode->Next()) {
				SoldierGameObj * obj = objnode->Data();
				if ( obj && obj->Get_Player_Data() && obj->Is_Teammate( smart ) ) {
					obj->Get_Player_Data()->Increment_Score( points );
				}
			}
		} else {
			if ( smart->Get_Player_Data() ) {
				smart->Get_Player_Data()->Increment_Score( points );
			}
		}
	} else {
		Debug_Say(( "Cannot Give Points to object\n" ));
	}
}

/*
**
*/
float	Get_Money( GameObject * object )
{
	if ( object && object->As_SmartGameObj() && object->As_SmartGameObj()->Get_Player_Data() ) {
		return object->As_SmartGameObj()->Get_Player_Data()->Get_Money();
	}
	Debug_Say(( "Cannot Get Money for object\n" ));
	return 0;
}

void	Give_Money( GameObject * object, float money, bool entire_team )
{
	SCRIPT_PTR_CHECK( object );
	SCRIPT_TRACE((	"ST>Give_Money( %d, %f, %d )\n", object->Get_ID(), money, entire_team ));

	SmartGameObj * smart = object->As_SmartGameObj();
	if ( smart ) {
		if ( entire_team ) {
			SLNode<SoldierGameObj> *objnode;
			for (	objnode = GameObjManager::Get_Star_Game_Obj_List()->Head(); objnode; objnode = objnode->Next()) {
				SoldierGameObj * obj = objnode->Data();
				if ( obj && obj->Get_Player_Data() && obj->Is_Teammate( smart ) ) {
					obj->Get_Player_Data()->Increment_Money( money );
				}
			}
		} else {
			if ( smart->Get_Player_Data() ) {
				smart->Get_Player_Data()->Increment_Money( money );
			}
		}
	} else {
		Debug_Say(( "Cannot Give Money to object\n" ));
	}
}

/*
**
*/
bool	Get_Building_Power( GameObject * object )
{
	SCRIPT_PTR_CHECK_RET( object, false );
	BuildingGameObj *building = object->As_BuildingGameObj();
	if ( building ) {
		return ( building->Is_Power_Enabled() );
	}
	Debug_Say(( "Object is not a Building\n" ));
	return false;
}

void	Set_Building_Power( GameObject * object, bool onoff )
{
	SCRIPT_PTR_CHECK( object );
	SCRIPT_TRACE((	"ST>Set_Building_Power( %d, %d )\n", object->Get_ID(), onoff ));

	BuildingGameObj *building = object->As_BuildingGameObj();
	if ( building ) {
		building->Enable_Power( onoff );
	} else {
		Debug_Say(( "Object is not a Building\n" ));
	}
}


void	Play_Building_Announcement( GameObject * object, int text_id )
{
	SCRIPT_PTR_CHECK( object );
	SCRIPT_TRACE((	"ST>Play_Building_Announcement( %d, %d )\n", object->Get_ID(), text_id ));

	BuildingGameObj *building = object->As_BuildingGameObj();
	if ( building ) {
		building->Play_Announcement( text_id, false );
	} else {
		Debug_Say(( "Object is not a Building\n" ));
	}
}


GameObject * Find_Nearest_Building_To_Pos( const Vector3 & position, const char * mesh_prefix )
{
	SCRIPT_PTR_CHECK_RET( mesh_prefix, nullptr );
	SCRIPT_TRACE((	"ST>Find_Nearest_Building_To_Pos( (%f,%f,%f), %s )\n", position.X, position.Y, position.Z, mesh_prefix ));

	float best_dist2						= 99999.0F;
	BuildingGameObj *best_building	= nullptr;

	//
	//	Loop over all the buildings in the level
	//
	SLNode<BuildingGameObj> *obj_node = nullptr;
	for ( obj_node = GameObjManager::Get_Building_Game_Obj_List()->Head(); obj_node; obj_node = obj_node->Next() ) {
		BuildingGameObj *building = obj_node->Data ()->As_BuildingGameObj ();
		if (building != nullptr) {

			//
			//	Is this the type of building we're looking for?
			//
			const char *name_prefix = building->Get_Name_Prefix();
			if (::stricmp( mesh_prefix, name_prefix ) == 0) {

				//
				//	Is this the closest building we've found yet?
				//
				Vector3 building_pos;
				building->Get_Position (&building_pos);
				float dist2 = (building_pos - position).Length2 ();
				if (dist2 < best_dist2) {
					best_building	= building;
					best_dist2		= dist2;
				}
			}
		}
	}

	return best_building;
}


GameObject * Find_Nearest_Building( GameObject * object, const char * mesh_prefix )
{
	SCRIPT_PTR_CHECK_RET( object, nullptr );
	SCRIPT_PTR_CHECK_RET( mesh_prefix, nullptr );
	SCRIPT_TRACE((	"ST>Find_Nearest_Building( %d, %s )\n", object->Get_ID(), mesh_prefix ));

	//
	//	Get the position of the game object
	//
	Vector3 position;
	object->Get_Position (&position);

	//
	//	Return the building that's nearest to this position
	//
	return Find_Nearest_Building_To_Pos( position, mesh_prefix );
}


int	Team_Members_In_Zone( GameObject * object, int player_type )
{
	SCRIPT_PTR_CHECK_RET( object, 0 );

	ScriptZoneGameObj *script_zone = object->As_ScriptZoneGameObj();
	if ( script_zone ) {
		return script_zone->Count_Team_Members_Inside( player_type );
	} else {
		Debug_Say(( "Object is not a ScriptZone\n" ));
	}

	return 0;
}

void Set_Clouds (float cloudcover, float cloudgloominess, float ramptime)
{
	if (!BackgroundMgrClass::Set_Clouds (cloudcover, cloudgloominess, ramptime)) {
		Debug_Say (("Cannot set clouds\n"));
	}
}

void Set_Lightning (float intensity, float startdistance, float enddistance, float heading, float distribution, float ramptime)
{
	if (!BackgroundMgrClass::Set_Lightning (intensity, startdistance, enddistance, heading, distribution, ramptime)) {
		Debug_Say (("Cannot set lightning\n"));
	}
}

void Set_War_Blitz (float intensity, float startdistance, float enddistance, float heading, float distribution, float ramptime)
{
	if (!BackgroundMgrClass::Set_War_Blitz (intensity, startdistance, enddistance, heading, distribution, ramptime)) {
		Debug_Say (("Cannot set war blitz\n"));
	}
}

void Set_Wind (float heading, float speed, float variability, float ramptime)
{
	if (!WeatherMgrClass::Set_Wind (heading, speed, variability, ramptime)) {
		Debug_Say (("Cannot set wind\n"));
	}
}

void Set_Rain (float density, float ramptime, bool /* prime */)
{
	if (!WeatherMgrClass::Set_Precipitation (WeatherMgrClass::PRECIPITATION_RAIN, density, ramptime)) {
		Debug_Say (("Cannot set rain\n"));
	}
}

void Set_Snow (float density, float ramptime, bool /* prime */)
{
	if (!WeatherMgrClass::Set_Precipitation (WeatherMgrClass::PRECIPITATION_SNOW, density, ramptime)) {
		Debug_Say (("Cannot set snow\n"));
	}
}

void Set_Ash (float density, float ramptime, bool /* prime */)
{
	if (!WeatherMgrClass::Set_Precipitation (WeatherMgrClass::PRECIPITATION_ASH, density, ramptime)) {
		Debug_Say (("Cannot set ash\n"));
	}
}

void Set_Fog_Enable (bool enabled)
{
	WeatherMgrClass::Set_Fog_Enable (enabled);
}

void Set_Fog_Range (float startdistance, float enddistance, float ramptime)
{
	if (!WeatherMgrClass::Set_Fog_Range (startdistance, enddistance, ramptime)) {
		Debug_Say (("Cannot set fog range\n"));
	}
}

void Enable_Stealth(GameObject * object, bool onoff)
{
	SCRIPT_PTR_CHECK( object );
	SCRIPT_TRACE((	"ST>Enable_Stealth( %d, %d )\n", object->Get_ID(), (onoff ? 1 : 0)));

	SmartGameObj * smartobj = object->As_SmartGameObj();
	if (smartobj != nullptr) {
		smartobj->Enable_Stealth(onoff);
	}
}

void	Cinematic_Sniper_Control(bool enabled, float zoom)
{
	SCRIPT_TRACE((	"ST>Sniper_Control( %d, %f )\n", enabled, zoom ));

	if ( COMBAT_CAMERA ) {
		COMBAT_CAMERA->Cinematic_Sniper_Control( enabled, zoom );
	}
}

/*
**
*/
void *	Text_File_Open( const char * filename )
{
	FileClass * file = _TheFileFactory->Get_File( filename );
	if ( file ) {
		file->Open();
		if ( !file->Is_Available() ) {
			_TheFileFactory->Return_File( file );
			file = nullptr;
		}
	}
	return (void *)file;
}

bool	Text_File_Get_String( void *handle, char * buffer, int size )
{
	FileClass * file = (FileClass *)handle;
	char ch[4];
	char *b = buffer;
	while ( file->Read( &ch[0], 1 ) == 1 ) {
		if ( size > 0 ) {
			*b++ = ch[0];
			size--;
		}
		if ( ch[0] == '\n' ) {
			break;
		}
	}
	*b = 0;
	return (buffer[0] != 0);
}

void	Text_File_Close( void *handle )
{
	FileClass * file = (FileClass *)handle;
	if ( file != nullptr ) {
		file->Close();
		_TheFileFactory->Return_File( file );
	}
}


/*
**
*/
void	Enable_Vehicle_Transitions( GameObject * object, bool enable )
{
	SCRIPT_PTR_CHECK( object );
	SCRIPT_TRACE((	"ST>Enable_Vehicle_Transitions( %d, %d )\n", object->Get_ID(), enable ));

	PhysicalGameObj * physgo = object->As_PhysicalGameObj();
	VehicleGameObj * vehicle = nullptr;
	if ( physgo ) {
		vehicle = physgo->As_VehicleGameObj();
	}
	if ( vehicle ) {
		vehicle->Script_Enable_Transitions( enable );
	} else {
		Debug_Say(( "Can only Enable_Vehicle_Transitions on a Vehicle\n" ));
	}
}


/*
**
*/
void	Display_GDI_Player_Terminal (void)
{
	SCRIPT_TRACE((	"ST>Display_GDI_Player_Terminal( )\n" ));

	PlayerTerminalClass::Get_Instance ()->Display_Terminal (COMBAT_STAR, PlayerTerminalClass::TYPE_GDI);
	return ;
}


/*
**
*/
void	Display_NOD_Player_Terminal (void)
{
	SCRIPT_TRACE((	"ST>Display_NOD_Player_Terminal( )\n" ));

	PlayerTerminalClass::Get_Instance ()->Display_Terminal (COMBAT_STAR, PlayerTerminalClass::TYPE_NOD);
	return ;
}


/*
**
*/
void	Display_Mutant_Player_Terminal (void)
{
	SCRIPT_TRACE((	"ST>Display_Mutant_Player_Terminal( )\n" ));

	PlayerTerminalClass::Get_Instance ()->Display_Terminal (COMBAT_STAR, PlayerTerminalClass::TYPE_MUTANT);
	return ;
}


/*
**
*/
bool	Reveal_Encyclopedia_Character( int object_id )
{
	SCRIPT_TRACE((	"ST>Reveal_Encyclopedia_Character( %d )\n", object_id ));
	return EncyclopediaMgrClass::Reveal_Object( EncyclopediaMgrClass::TYPE_CHARACTER, object_id );
}


/*
**
*/
bool	Reveal_Encyclopedia_Weapon( int object_id )
{
	SCRIPT_TRACE((	"ST>Reveal_Encyclopedia_Weapon( %d )\n", object_id ));
	return EncyclopediaMgrClass::Reveal_Object( EncyclopediaMgrClass::TYPE_WEAPON, object_id );
}


/*
**
*/
bool	Reveal_Encyclopedia_Vehicle( int object_id )
{
	SCRIPT_TRACE((	"ST>Reveal_Encyclopedia_Vehicle( %d )\n", object_id ));
	return EncyclopediaMgrClass::Reveal_Object( EncyclopediaMgrClass::TYPE_VEHICLE, object_id );
}


/*
**
*/
bool	Reveal_Encyclopedia_Building( int object_id )
{
	SCRIPT_TRACE((	"ST>Reveal_Encyclopedia_Building( %d )\n", object_id ));
	return EncyclopediaMgrClass::Reveal_Object( EncyclopediaMgrClass::TYPE_BUILDING, object_id );
}


/*
**
*/
void	Display_Encyclopedia_Event_UI( void )
{
	SCRIPT_TRACE((	"ST>Display_Encyclopedia_Event_UI( )\n" ));
	EncyclopediaMgrClass::Display_Event_UI();

	HUDClass::Add_Data_Link();
	return ;
}


/*
**
*/
void	Scale_AI_Awareness( float sight_scale, [[maybe_unused]] float hearing_scale )
{
	SCRIPT_TRACE((	"ST>Scale_AI_Awareness ( %f %f )\n", sight_scale, hearing_scale ));
	SmartGameObj::Set_Global_Sight_Range_Scale( sight_scale );
//	SoundSystem::Set_Global_Listener_Scale( hearing_scale );
}


/*
**
*/
void	Enable_Cinematic_Freeze( GameObject * object, bool enable )
{
	SCRIPT_PTR_CHECK( object );
	SCRIPT_TRACE((	"ST>Enable_Cinematic_Freeze( %d, %d )\n", object->Get_ID(), enable ));

	object->Enable_Cinematic_Freeze( enable );
}

void	Expire_Powerup( GameObject * object )
{
	SCRIPT_PTR_CHECK( object );
	SCRIPT_TRACE((	"ST>Expire_Powerup( %d )\n", object->Get_ID() ));

	PowerUpGameObj * powerup = nullptr;
	if ( object->As_PhysicalGameObj() != nullptr ) {
		powerup = object->As_PhysicalGameObj()->As_PowerUpGameObj();
	}
	if ( powerup ) {
		powerup->Expire();
	}
}

void	Set_HUD_Help_Text( int string_id, const Vector3 &color )
{
	SCRIPT_TRACE((	"ST>Set_HUD_Help_Text( %d )\n", string_id ));

	if (string_id == 0) {

		//
		//	Clear the help text
		//
		HUDInfo::Set_HUD_Help_Text( U_CHAR("") );
	} else {

		//
		//	Set the help text
		//
		HUDInfo::Set_HUD_Help_Text( TRANSLATE( string_id ), color );

		//
		//	Lookup the sound to play
		//
		int sound_id = GlobalSettingsDef::Get_Global_Settings ()->Get_HUD_Help_Text_Sound_ID ();
		if (sound_id != 0) {

			//
			//	Play the sound
			//
			WWAudioClass* audio = WWAudioClass::Get_Instance();
			assert(audio != nullptr);
			audio->Create_Instant_Sound( sound_id, Matrix3D(1) );
		}
	}

	return ;
}

/*
**
*/
void	Enable_HUD_Pokable_Indicator( GameObject * object, bool enable )
{
	SCRIPT_PTR_CHECK( object );
	SCRIPT_TRACE((	"ST>Enable_HUD_Pokable_Indicator( %d, %d )\n", object->Get_ID(), enable ));
	if ( object->As_PhysicalGameObj() != nullptr ) {
		object->As_PhysicalGameObj()->Enable_HUD_Pokable_Indicator( enable );
	} else {
		Debug_Say(( "Can only Enable_HUD_Pokable_Indicator on PhysicalGameObjs\n" ));
	}
}

void	Enable_Innate_Conversations( GameObject * object, bool enable )
{
	SCRIPT_PTR_CHECK( object );
	SCRIPT_TRACE((	"ST>Enable_Innate_Conversations( %d, %d )\n", object->Get_ID(), enable ));
	if ( object->As_PhysicalGameObj() != nullptr ) {
		object->As_PhysicalGameObj()->Enable_Innate_Conversations( enable );
	} else {
		Debug_Say(( "Can only Enable_Innate_Conversations on PhysicalGameObjs\n" ));
	}
}

void	Display_Health_Bar( GameObject * object, bool display )
{
	SCRIPT_PTR_CHECK( object );
	SCRIPT_TRACE((	"ST>Display_Health_Bar( %d, %d )\n", object->Get_ID(), display ));
	if ( object->As_DamageableGameObj() != nullptr ) {
		object->As_DamageableGameObj()->Set_Is_Health_Bar_Displayed( display );
	} else {
		Debug_Say(( "Can only Display_Health_Bar on DamageableGameObjs\n" ));
	}
}

void	Enable_Shadow( GameObject * object, bool enable )
{
	SCRIPT_PTR_CHECK( object );
	SCRIPT_TRACE(( "ST>Enable_Shadow( %d, %d )\n", object->Get_ID(), enable));
	if ( object->As_PhysicalGameObj() != nullptr) {
		object->As_PhysicalGameObj()->Peek_Physical_Object()->Enable_Shadow_Generation(enable);
	} else {
		Debug_Say(( "Can only call Enable_Shadow on PhysicalGameObjs\n"));
	}
}

void	Clear_Weapons( GameObject * object )
{
	SCRIPT_PTR_CHECK( object );
	SCRIPT_TRACE(( "ST>Clear_Weapons( %d )\n", object->Get_ID() ));
	if ( object->As_SmartGameObj() != nullptr) {
		object->As_SmartGameObj()->Get_Weapon_Bag()->Clear_Weapons();
	} else {
		Debug_Say(( "Can only call Clear_Weapons on SmartGameObjs\n"));
	}
}

void	Set_Num_Tertiary_Objectives( int count )
{
	SCRIPT_TRACE(( "ST>Set_Num_Tertiary_Objectives( %d )\n", count ));
	ObjectiveManager::Set_Num_Specified_Tertiary_Objectives( count );
}


void	Enable_Letterbox( bool onoff, float seconds )
{
	SCRIPT_TRACE(( "ST>Enable_Letterbox( %d, %f)\n", onoff,seconds ));
	ScreenFadeManager::Enable_Letterbox(onoff,seconds);
}

void	Set_Screen_Fade_Color( float r, float g, float b, float seconds )
{
	SCRIPT_TRACE(( "ST>Set_Screen_Fade_Color( %f, %f, %f, %f)\n", r,g,b,seconds ));
	ScreenFadeManager::Set_Screen_Overlay_Color(r,g,b,seconds);
}

void	Set_Screen_Fade_Opacity( float opacity, float seconds )
{
	SCRIPT_TRACE(( "ST>Set_Screen_Fade_Opacity( %f, %f)\n", opacity,seconds ));
	ScreenFadeManager::Set_Screen_Overlay_Opacity(opacity,seconds);
}


/*
**	Per-client commands.
**
**	A script runs on the server.  Anything that draws, plays, or fades belongs
**	to one machine, so these build the S->C event and let the addressee's own
**	client run the ordinary command.  There is still exactly one implementation
**	of each effect -- the local one above -- which is the point.
*/

static int	Client_Id_Of( GameObject * player )
{
	if ( player == nullptr ) {
		return -1;
	}

	SmartGameObj * smart_obj = player->As_SmartGameObj();
	if ( smart_obj == nullptr ) {
		return -1;
	}

	return smart_obj->Get_Control_Owner();
}

void	Send_Message( int red, int green, int blue, const char * message )
{
	SCRIPT_PTR_CHECK( message );
	SCRIPT_TRACE(( "ST>Send_Message( %d, %d, %d, %s )\n", red, green, blue, message ));

	cScScriptCommandEvent * event = new cScScriptCommandEvent;
	event->Set_Text( message );
	event->Set_Color( Vector3( red / 255.0f, green / 255.0f, blue / 255.0f ) );
	event->Init( SCRIPT_CLIENT_CMD_SEND_MESSAGE, -1 );
}

void	Send_Message_Player( GameObject * player, int red, int green, int blue, const char * message )
{
	SCRIPT_PTR_CHECK( message );
	SCRIPT_TRACE(( "ST>Send_Message_Player( %d, %d, %d, %s )\n", red, green, blue, message ));

	int client_id = Client_Id_Of( player );
	if ( client_id == -1 ) {
		return ;
	}

	cScScriptCommandEvent * event = new cScScriptCommandEvent;
	event->Set_Text( message );
	event->Set_Color( Vector3( red / 255.0f, green / 255.0f, blue / 255.0f ) );
	event->Init( SCRIPT_CLIENT_CMD_SEND_MESSAGE, client_id );
}

void	Send_Message_Team( int team, int red, int green, int blue, const char * message )
{
	SCRIPT_PTR_CHECK( message );
	SCRIPT_TRACE(( "ST>Send_Message_Team( %d, %s )\n", team, message ));

	cScScriptCommandEvent * event = new cScScriptCommandEvent;
	event->Set_Text( message );
	event->Set_Color( Vector3( red / 255.0f, green / 255.0f, blue / 255.0f ) );
	event->Init_For_Team( SCRIPT_CLIENT_CMD_SEND_MESSAGE, team );
}

void	Send_Message_With_Team_Color( int team, const char * message )
{
	SCRIPT_PTR_CHECK( message );
	SCRIPT_TRACE(( "ST>Send_Message_With_Team_Color( %d, %s )\n", team, message ));

	cScScriptCommandEvent * event = new cScScriptCommandEvent;
	event->Set_Text( message );
	event->Set_Color( ::Get_Color_For_Team( team ) );
	event->Init( SCRIPT_CLIENT_CMD_SEND_MESSAGE, -1 );
}

void	Create_Sound_Player( GameObject * player, const char * sound_preset_name, const Vector3 & position )
{
	SCRIPT_PTR_CHECK( sound_preset_name );

	int client_id = Client_Id_Of( player );
	if ( client_id == -1 ) {
		return ;
	}

	cScScriptCommandEvent * event = new cScScriptCommandEvent;
	event->Set_Text( sound_preset_name );
	event->Set_Position( position );
	event->Init( SCRIPT_CLIENT_CMD_CREATE_SOUND, client_id );
}

void	Create_Sound_Team( int team, const char * sound_preset_name, const Vector3 & position )
{
	SCRIPT_PTR_CHECK( sound_preset_name );

	cScScriptCommandEvent * event = new cScScriptCommandEvent;
	event->Set_Text( sound_preset_name );
	event->Set_Position( position );
	event->Init_For_Team( SCRIPT_CLIENT_CMD_CREATE_SOUND, team );
}

void	Create_2D_Sound_Player( GameObject * player, const char * sound_preset_name )
{
	SCRIPT_PTR_CHECK( sound_preset_name );

	int client_id = Client_Id_Of( player );
	if ( client_id == -1 ) {
		return ;
	}

	cScScriptCommandEvent * event = new cScScriptCommandEvent;
	event->Set_Text( sound_preset_name );
	event->Init( SCRIPT_CLIENT_CMD_CREATE_2D_SOUND, client_id );
}

void	Create_2D_Sound_Team( int team, const char * sound_preset_name )
{
	SCRIPT_PTR_CHECK( sound_preset_name );

	cScScriptCommandEvent * event = new cScScriptCommandEvent;
	event->Set_Text( sound_preset_name );
	event->Init_For_Team( SCRIPT_CLIENT_CMD_CREATE_2D_SOUND, team );
}

void	Create_2D_WAV_Sound_Player( GameObject * player, const char * wav_filename )
{
	SCRIPT_PTR_CHECK( wav_filename );

	int client_id = Client_Id_Of( player );
	if ( client_id == -1 ) {
		return ;
	}

	cScScriptCommandEvent * event = new cScScriptCommandEvent;
	event->Set_Text( wav_filename );
	event->Init( SCRIPT_CLIENT_CMD_CREATE_2D_WAV_SOUND, client_id );
}

void	Create_2D_WAV_Sound_Team( int team, const char * wav_filename )
{
	SCRIPT_PTR_CHECK( wav_filename );

	cScScriptCommandEvent * event = new cScScriptCommandEvent;
	event->Set_Text( wav_filename );
	event->Init_For_Team( SCRIPT_CLIENT_CMD_CREATE_2D_WAV_SOUND, team );
}

void	Set_Background_Music_Player( GameObject * player, const char * wav_filename )
{
	SCRIPT_PTR_CHECK( wav_filename );

	int client_id = Client_Id_Of( player );
	if ( client_id == -1 ) {
		return ;
	}

	cScScriptCommandEvent * event = new cScScriptCommandEvent;
	event->Set_Text( wav_filename );
	event->Init( SCRIPT_CLIENT_CMD_SET_BACKGROUND_MUSIC, client_id );
}

void	Fade_Background_Music_Player( GameObject * player, const char * wav_filename, int fade_out_time, int fade_in_time )
{
	SCRIPT_PTR_CHECK( wav_filename );

	int client_id = Client_Id_Of( player );
	if ( client_id == -1 ) {
		return ;
	}

	cScScriptCommandEvent * event = new cScScriptCommandEvent;
	event->Set_Text( wav_filename );
	event->Set_Int_Params( fade_out_time, fade_in_time );
	event->Init( SCRIPT_CLIENT_CMD_FADE_BACKGROUND_MUSIC, client_id );
}

void	Stop_Background_Music_Player( GameObject * player )
{
	int client_id = Client_Id_Of( player );
	if ( client_id == -1 ) {
		return ;
	}

	cScScriptCommandEvent * event = new cScScriptCommandEvent;
	event->Init( SCRIPT_CLIENT_CMD_STOP_BACKGROUND_MUSIC, client_id );
}

void	Set_HUD_Help_Text_Player( GameObject * player, int string_id, const Vector3 & color )
{
	int client_id = Client_Id_Of( player );
	if ( client_id == -1 ) {
		return ;
	}

	cScScriptCommandEvent * event = new cScScriptCommandEvent;
	event->Set_Int_Params( string_id );
	event->Set_Color( color );
	event->Init( SCRIPT_CLIENT_CMD_SET_HUD_HELP_TEXT, client_id );
}

void	Set_Screen_Fade_Color_Player( GameObject * player, float r, float g, float b, float seconds )
{
	int client_id = Client_Id_Of( player );
	if ( client_id == -1 ) {
		return ;
	}

	cScScriptCommandEvent * event = new cScScriptCommandEvent;
	event->Set_Color( Vector3( r, g, b ) );
	event->Set_Float_Params( seconds );
	event->Init( SCRIPT_CLIENT_CMD_SET_SCREEN_FADE_COLOR, client_id );
}

void	Set_Screen_Fade_Opacity_Player( GameObject * player, float opacity, float seconds )
{
	int client_id = Client_Id_Of( player );
	if ( client_id == -1 ) {
		return ;
	}

	cScScriptCommandEvent * event = new cScScriptCommandEvent;
	event->Set_Float_Params( opacity, seconds );
	event->Init( SCRIPT_CLIENT_CMD_SET_SCREEN_FADE_OPACITY, client_id );
}

void	Set_Occupants_Fade( GameObject * vehicle_obj, float r, float g, float b, float opacity )
{
	PhysicalGameObj * physical = ( vehicle_obj != nullptr ) ? vehicle_obj->As_PhysicalGameObj() : nullptr;
	VehicleGameObj * vehicle = ( physical != nullptr ) ? physical->As_VehicleGameObj() : nullptr;
	if ( vehicle == nullptr ) {
		return ;
	}

	//
	//	Both halves at once and both instant.  The colour and the opacity are
	//	two commands to the client, and fading one without the other leaves a
	//	seat looking through the last colour it was given.
	//
	int seat_count = vehicle->Get_Definition().Get_Seat_Count();
	for ( int seat = 0; seat < seat_count; seat ++ ) {

		SoldierGameObj * occupant = vehicle->Get_Occupant( seat );
		if ( occupant == nullptr ) {
			continue;
		}

		Set_Screen_Fade_Color_Player( occupant, r, g, b, 0 );
		Set_Screen_Fade_Opacity_Player( occupant, opacity, 0 );
	}

	return ;
}


void	Force_Camera_Look_Player( GameObject * player, const Vector3 & target )
{
	int client_id = Client_Id_Of( player );
	if ( client_id == -1 ) {
		return ;
	}

	cScScriptCommandEvent * event = new cScScriptCommandEvent;
	event->Set_Position( target );
	event->Init( SCRIPT_CLIENT_CMD_FORCE_CAMERA_LOOK, client_id );
}

void	Enable_Radar_Player( GameObject * player, bool enable )
{
	int client_id = Client_Id_Of( player );
	if ( client_id == -1 ) {
		return ;
	}

	cScScriptCommandEvent * event = new cScScriptCommandEvent;
	event->Set_Int_Params( enable ? 1 : 0 );
	event->Init( SCRIPT_CLIENT_CMD_ENABLE_RADAR, client_id );
}

void	Display_GDI_Player_Terminal_Player( GameObject * player )
{
	int client_id = Client_Id_Of( player );
	if ( client_id == -1 ) {
		return ;
	}

	cScScriptCommandEvent * event = new cScScriptCommandEvent;
	event->Init( SCRIPT_CLIENT_CMD_DISPLAY_GDI_TERMINAL, client_id );
}

void	Display_NOD_Player_Terminal_Player( GameObject * player )
{
	int client_id = Client_Id_Of( player );
	if ( client_id == -1 ) {
		return ;
	}

	cScScriptCommandEvent * event = new cScScriptCommandEvent;
	event->Init( SCRIPT_CLIENT_CMD_DISPLAY_NOD_TERMINAL, client_id );
}

void	Kill_All_Buildings_By_Team( int team )
{
	SCRIPT_TRACE(( "ST>Kill_All_Buildings_By_Team( %d )\n", team ));

	BaseControllerClass * base = BaseControllerClass::Find_Base( team );
	if ( base != nullptr ) {
		base->Set_Base_Destroyed( true );
	}
}

/*
**	One implementation of a refill, here rather than in the purchase terminal,
**	because a script can want to grant one without a terminal being involved.
**	VendorClass::Grant_Supplies calls this.
*/
void	Grant_Refill( GameObject * player )
{
	SCRIPT_PTR_CHECK( player );

	PhysicalGameObj * physical_obj = player->As_PhysicalGameObj();
	if ( physical_obj == nullptr ) {
		return ;
	}

	SoldierGameObj * soldier = physical_obj->As_SoldierGameObj();
	if ( soldier == nullptr ) {
		return ;
	}

	WeaponBagClass * weapon_bag = soldier->Get_Weapon_Bag();
	for ( int weapon_index = 0; weapon_index < weapon_bag->Get_Count(); weapon_index ++ ) {
		WeaponClass * weapon = weapon_bag->Peek_Weapon( weapon_index );
		if ( weapon != nullptr && weapon->Get_Definition()->CanReceiveGenericCnCAmmo ) {
			weapon->Set_Inventory_Rounds( weapon->Get_Definition()->MaxInventoryRounds );
			weapon->Set_Clip_Rounds( weapon->Get_Definition()->ClipSize );
		}
	}

	DefenseObjectClass * defense_obj = soldier->Get_Defense_Object();
	defense_obj->Set_Health( defense_obj->Get_Health_Max() );
	defense_obj->Set_Shield_Strength( defense_obj->Get_Shield_Strength_Max() );
}


/*
**	Two per-preset character flags.  Both answer false for anything that is not
**	a soldier, which is what every caller wants: a vehicle is not a spy, and
**	asking whether a building can be squished is not a question.
*/
bool	Is_Spy( GameObject * obj )
{
	if ( obj == nullptr ) {
		return false;
	}

	PhysicalGameObj * physical_obj = obj->As_PhysicalGameObj();
	SoldierGameObj * soldier = ( physical_obj != nullptr ) ? physical_obj->As_SoldierGameObj() : nullptr;

	return ( soldier != nullptr ) && soldier->Get_Definition().Is_Spy();
}


bool	Is_Unsquishable( GameObject * obj )
{
	if ( obj == nullptr ) {
		return false;
	}

	PhysicalGameObj * physical_obj = obj->As_PhysicalGameObj();
	SoldierGameObj * soldier = ( physical_obj != nullptr ) ? physical_obj->As_SoldierGameObj() : nullptr;

	return ( soldier != nullptr ) && !soldier->Is_Squishable();
}


/*
**	The portable half of the 4.8.4 script API.
**
**	These are the questions the 4.8.4 library asks that the stock catalog never
**	needed.  None of them are TT-specific: they are ordinary things to want to
**	know about a vehicle, a base, or a script, and they are written here in the
**	canonical owner rather than in a library beside the engine.
**
**	Where a team is taken, 2 means "either side", which is how the library's
**	own scripts are authored.
*/

static bool	Team_Matches( GameObject * obj, int team )
{
	if ( team == 2 ) {
		return true;
	}

	return Get_Player_Type( obj ) == team;
}

static VehicleGameObj *	As_Vehicle( GameObject * obj )
{
	if ( obj == nullptr ) {
		return nullptr;
	}

	PhysicalGameObj * physical_obj = obj->As_PhysicalGameObj();
	if ( physical_obj == nullptr ) {
		return nullptr;
	}

	return physical_obj->As_VehicleGameObj();
}

static SoldierGameObj *	As_Soldier( GameObject * obj )
{
	if ( obj == nullptr ) {
		return nullptr;
	}

	PhysicalGameObj * physical_obj = obj->As_PhysicalGameObj();
	if ( physical_obj == nullptr ) {
		return nullptr;
	}

	return physical_obj->As_SoldierGameObj();
}

static MoveablePhysClass *	As_Moveable( GameObject * obj )
{
	if ( obj == nullptr ) {
		return nullptr;
	}

	PhysicalGameObj * physical_obj = obj->As_PhysicalGameObj();
	if ( physical_obj == nullptr ) {
		return nullptr;
	}

	PhysClass * phys_obj = physical_obj->Peek_Physical_Object();
	if ( phys_obj == nullptr ) {
		return nullptr;
	}

	return phys_obj->As_MoveablePhysClass();
}

//
//	Finding things
//

GameObject * Find_Object_By_Preset( int team, const char * preset_name )
{
	SCRIPT_PTR_CHECK_RET( preset_name, nullptr );

	for ( SLNode<BaseGameObj> * node = GameObjManager::Get_Game_Obj_List()->Head();
			node != nullptr; node = node->Next() ) {

		ScriptableGameObj * obj = node->Data()->As_ScriptableGameObj();
		if ( obj == nullptr ) {
			continue;
		}

		if ( ::stricmp( obj->Get_Definition().Get_Name(), preset_name ) == 0 && Team_Matches( obj, team ) ) {
			return obj;
		}
	}

	return nullptr;
}

GameObject * Find_Closest_Building( const Vector3 & position )
{
	GameObject *	closest			= nullptr;
	float				closest_dist2	= FLT_MAX;

	for ( SLNode<BuildingGameObj> * node = GameObjManager::Get_Building_Game_Obj_List()->Head();
			node != nullptr; node = node->Next() ) {

		BuildingGameObj * building = node->Data();
		float dist2 = ( Get_Position( building ) - position ).Length2();
		if ( dist2 < closest_dist2 ) {
			closest_dist2	= dist2;
			closest			= building;
		}
	}

	return closest;
}

GameObject * Find_Smart_Object_By_Team( int team )
{
	for ( SLNode<SmartGameObj> * node = GameObjManager::Get_Smart_Game_Obj_List()->Head();
			node != nullptr; node = node->Next() ) {

		SmartGameObj * obj = node->Data();
		if ( Team_Matches( obj, team ) ) {
			return obj;
		}
	}

	return nullptr;
}

GameObject * Find_Object_With_Script( const char * script_name )
{
	SCRIPT_PTR_CHECK_RET( script_name, nullptr );

	for ( SLNode<BaseGameObj> * node = GameObjManager::Get_Game_Obj_List()->Head();
			node != nullptr; node = node->Next() ) {

		ScriptableGameObj * obj = node->Data()->As_ScriptableGameObj();
		if ( obj != nullptr && Is_Script_Attached( obj, script_name ) ) {
			return obj;
		}
	}

	return nullptr;
}

int Get_Object_Count( int team, const char * preset_name )
{
	SCRIPT_PTR_CHECK_RET( preset_name, 0 );

	int count = 0;

	for ( SLNode<SmartGameObj> * node = GameObjManager::Get_Smart_Game_Obj_List()->Head();
			node != nullptr; node = node->Next() ) {

		SmartGameObj * obj = node->Data();
		if ( Team_Matches( obj, team ) &&
			  ::stricmp( obj->Get_Definition().Get_Name(), preset_name ) == 0 ) {
			count ++;
		}
	}

	return count;
}

//
//	`allow_empty` decides whether a vehicle with nobody in it counts.
//
bool Is_Unit_In_Range( const char * preset_name, float range, const Vector3 & position, int team, bool allow_empty )
{
	SCRIPT_PTR_CHECK_RET( preset_name, false );

	for ( SLNode<SmartGameObj> * node = GameObjManager::Get_Smart_Game_Obj_List()->Head();
			node != nullptr; node = node->Next() ) {

		SmartGameObj * obj = node->Data();
		if ( !Team_Matches( obj, team ) ||
			  ::stricmp( obj->Get_Definition().Get_Name(), preset_name ) != 0 ) {
			continue;
		}

		Vector3 offset = Get_Position( obj ) - position;
		offset.Z = 0.0f;
		if ( offset.Length2() > ( range * range ) ) {
			continue;
		}

		VehicleGameObj * vehicle = obj->As_VehicleGameObj();
		if ( vehicle != nullptr && !allow_empty && vehicle->Get_Occupant_Count() == 0 ) {
			continue;
		}

		return true;
	}

	return false;
}

//
//	Movement state
//

Vector3 Get_Velocity( GameObject * obj )
{
	Vector3 velocity( 0.0f, 0.0f, 0.0f );

	MoveablePhysClass * moveable = As_Moveable( obj );
	if ( moveable != nullptr ) {
		moveable->Get_Velocity( &velocity );
	}

	return velocity;
}

void Set_Velocity( GameObject * obj, const Vector3 & velocity )
{
	MoveablePhysClass * moveable = As_Moveable( obj );
	if ( moveable != nullptr ) {
		moveable->Set_Velocity( velocity );
	}
}

void Set_Transform( GameObject * obj, const Matrix3D & transform )
{
	MoveablePhysClass * moveable = As_Moveable( obj );
	if ( moveable != nullptr ) {
		moveable->Set_Transform( transform );
	}
}

float Get_Mass( GameObject * obj )
{
	MoveablePhysClass * moveable = As_Moveable( obj );
	if ( moveable == nullptr ) {
		return 0.0f;
	}

	return moveable->Get_Mass();
}

//
//	Model and animation state
//

const char * Get_Model( GameObject * obj )
{
	SCRIPT_PTR_CHECK_RET( obj, "DUMMY" );

	PhysicalGameObj * physical_obj = obj->As_PhysicalGameObj();
	if ( physical_obj == nullptr ) {
		return "DUMMY";
	}

	PhysClass * phys_obj = physical_obj->Peek_Physical_Object();
	if ( phys_obj == nullptr || phys_obj->Peek_Model() == nullptr ) {
		return "DUMMY";
	}

	return phys_obj->Peek_Model()->Get_Name();
}

//
//	A cinematic object drives its own animation directly and has no control to
//	ask, which is why it is excluded rather than allowed to return garbage.
//
static AnimControlClass *	Peek_Anim_Control( GameObject * obj )
{
	if ( obj == nullptr ) {
		return nullptr;
	}

	PhysicalGameObj * physical_obj = obj->As_PhysicalGameObj();
	if ( physical_obj == nullptr || physical_obj->As_CinematicGameObj() != nullptr ) {
		return nullptr;
	}

	return physical_obj->Get_Anim_Control();
}

float Get_Animation_Frame( GameObject * obj )
{
	AnimControlClass * control = Peek_Anim_Control( obj );
	if ( control == nullptr ) {
		return 0.0f;
	}

	return control->Get_Current_Frame();
}

float Get_Animation_Target_Frame( GameObject * obj )
{
	AnimControlClass * control = Peek_Anim_Control( obj );
	if ( control == nullptr ) {
		return 0.0f;
	}

	return control->Get_Target_Frame();
}

//
//	Powerups
//

void Set_Powerup_Always_Allow_Grant( GameObject * obj, bool allow )
{
	SCRIPT_PTR_CHECK( obj );

	PhysicalGameObj * physical_obj = obj->As_PhysicalGameObj();
	if ( physical_obj == nullptr ) {
		return ;
	}

	PowerUpGameObj * powerup = physical_obj->As_PowerUpGameObj();
	if ( powerup == nullptr ) {
		return ;
	}

	( (PowerUpGameObjDef &)powerup->Get_Definition() ).Set_Always_Allow_Grant( allow );
}

const char * Get_Powerup_Weapon( const char * powerup_preset_name )
{
	SCRIPT_PTR_CHECK_RET( powerup_preset_name, "None" );

	PowerUpGameObjDef * powerup_def =
		(PowerUpGameObjDef *)DefinitionMgrClass::Find_Named_Definition( powerup_preset_name );
	if ( powerup_def == nullptr || powerup_def->Get_Grant_Weapon_ID() == 0 ) {
		return "None";
	}

	DefinitionClass * weapon_def =
		DefinitionMgrClass::Find_Definition( powerup_def->Get_Grant_Weapon_ID(), true );
	if ( weapon_def == nullptr ) {
		return "None";
	}

	return weapon_def->Get_Name();
}

//
//	Vehicles and their occupants
//

GameObject * Get_Vehicle_Driver( GameObject * obj )
{
	VehicleGameObj * vehicle = As_Vehicle( obj );
	if ( vehicle == nullptr ) {
		return nullptr;
	}

	return vehicle->Get_Driver();
}

GameObject * Get_Vehicle_Gunner( GameObject * obj )
{
	VehicleGameObj * vehicle = As_Vehicle( obj );
	if ( vehicle == nullptr ) {
		return nullptr;
	}

	return vehicle->Get_Actual_Gunner();
}

GameObject * Get_Vehicle_Occupant( GameObject * obj, int seat )
{
	VehicleGameObj * vehicle = As_Vehicle( obj );
	if ( vehicle == nullptr ) {
		return nullptr;
	}

	if ( seat < 0 || seat >= vehicle->Get_Definition().Get_Seat_Count() ) {
		return nullptr;
	}

	return vehicle->Get_Occupant( seat );
}

int Get_Vehicle_Occupant_Count( GameObject * obj )
{
	VehicleGameObj * vehicle = As_Vehicle( obj );
	if ( vehicle == nullptr ) {
		return 0;
	}

	return vehicle->Get_Occupant_Count();
}

int Get_Vehicle_Seat_Count( GameObject * obj )
{
	VehicleGameObj * vehicle = As_Vehicle( obj );
	if ( vehicle == nullptr ) {
		return 0;
	}

	return vehicle->Get_Definition().Get_Seat_Count();
}

int Get_Occupant_Seat( GameObject * vehicle_obj, GameObject * occupant )
{
	VehicleGameObj *	vehicle	= As_Vehicle( vehicle_obj );
	SoldierGameObj *	soldier	= As_Soldier( occupant );

	if ( vehicle == nullptr || soldier == nullptr ) {
		return -1;
	}

	return vehicle->Find_Seat( soldier );
}

//
//	Answers for the vehicle the object is, or the one it is riding in.
//
int Get_Vehicle_Mode( GameObject * obj )
{
	VehicleGameObj * vehicle = As_Vehicle( obj );
	if ( vehicle == nullptr ) {
		vehicle = As_Vehicle( Get_Vehicle( obj ) );
	}

	if ( vehicle == nullptr ) {
		return 0;
	}

	return (int)vehicle->Get_Definition().Get_Type();
}

bool Is_VTOL( GameObject * obj )
{
	return ( Get_Vehicle_Mode( obj ) == VEHICLE_TYPE_FLYING ) || Get_Fly_Mode( obj );
}

//
//	Action id 39 is the enter/exit action; priority 100 outranks anything a
//	soldier decides for itself, which is the point of forcing it.
//
static void	Order_Exit( SoldierGameObj * occupant )
{
	if ( occupant == nullptr ) {
		return ;
	}

	ActionParamsStruct params;
	params.Set_Basic( 0, 100, 39 );
	Action_Enter_Exit( occupant, params );
}

void Force_Occupants_Exit( GameObject * obj )
{
	VehicleGameObj * vehicle = As_Vehicle( obj );
	if ( vehicle == nullptr ) {
		return ;
	}

	int seat_count = vehicle->Get_Definition().Get_Seat_Count();
	for ( int seat = 0; seat < seat_count; seat ++ ) {
		Order_Exit( vehicle->Get_Occupant( seat ) );
	}
}

void Force_Occupant_Exit( GameObject * obj, int seat )
{
	VehicleGameObj * vehicle = As_Vehicle( obj );
	if ( vehicle == nullptr ) {
		return ;
	}

	if ( seat < 0 || seat >= vehicle->Get_Definition().Get_Seat_Count() ) {
		return ;
	}

	Order_Exit( vehicle->Get_Occupant( seat ) );
}

//
//	Turns out everyone who is *not* on the given team, which is what a captured
//	vehicle wants.
//
void Force_Occupants_Exit_Team( GameObject * obj, int team )
{
	VehicleGameObj * vehicle = As_Vehicle( obj );
	if ( vehicle == nullptr ) {
		return ;
	}

	int seat_count = vehicle->Get_Definition().Get_Seat_Count();
	for ( int seat = 0; seat < seat_count; seat ++ ) {

		SoldierGameObj * occupant = vehicle->Get_Occupant( seat );
		if ( occupant != nullptr && Get_Player_Type( occupant ) != team ) {
			Order_Exit( occupant );
		}
	}
}

void Soldier_Transition_Vehicle( GameObject * obj )
{
	Order_Exit( As_Soldier( obj ) );
}

//
//	Soldier state
//

bool Get_Fly_Mode( GameObject * obj )
{
	SoldierGameObj * soldier = As_Soldier( obj );
	if ( soldier == nullptr ) {
		return false;
	}

	return soldier->Get_Fly_Mode();
}

void Toggle_Fly_Mode( GameObject * obj )
{
	SoldierGameObj * soldier = As_Soldier( obj );
	if ( soldier == nullptr ) {
		return ;
	}

	soldier->Toggle_Fly_Mode();
	Enable_Collisions( soldier );
}

bool Is_Stealth( GameObject * obj )
{
	if ( obj == nullptr ) {
		return false;
	}

	SmartGameObj * smart_obj = obj->As_SmartGameObj();
	if ( smart_obj == nullptr ) {
		return false;
	}

	return smart_obj->Is_Stealthed();
}

bool Is_Stealth_Enabled( GameObject * obj )
{
	if ( obj == nullptr ) {
		return false;
	}

	SmartGameObj * smart_obj = obj->As_SmartGameObj();
	if ( smart_obj == nullptr ) {
		return false;
	}

	return smart_obj->Is_Stealth_Enabled();
}

//
//	Swap a soldier's character without respawning them.  Refused while they are
//	in a vehicle, where the seat would still be holding the old body.
//
bool Change_Character( GameObject * obj, const char * preset_name )
{
	SCRIPT_PTR_CHECK_RET( preset_name, false );

	SoldierGameObj * soldier = As_Soldier( obj );
	if ( soldier == nullptr || Get_Vehicle( obj ) != nullptr ) {
		return false;
	}

	DefinitionClass * def = DefinitionMgrClass::Find_Named_Definition( preset_name );
	if ( def == nullptr || def->Get_Class_ID() != CLASSID_GAME_OBJECT_DEF_SOLDIER ) {
		return false;
	}

	soldier->Re_Init( *(SoldierGameObjDef *)def );
	soldier->Post_Re_Init();
	return true;
}

//
//	The player behind a soldier, if there is one
//

int Get_Player_ID( GameObject * obj )
{
	SoldierGameObj * soldier = As_Soldier( obj );
	if ( soldier == nullptr || soldier->Get_Player_Data() == nullptr ) {
		return -1;
	}

	return soldier->Get_Player_Data()->Get_Player_Id();
}

//
//	A bot has a tag rather than a player record; a soldier with neither is
//	"None" so a caller always has something to print.
//
const unichar_t * Get_Wide_Player_Name( GameObject * obj )
{
	SoldierGameObj * soldier = As_Soldier( obj );
	if ( soldier == nullptr ) {
		return U_CHAR("None");
	}

	if ( !soldier->Get_Bot_Tag().Is_Empty() ) {
		return soldier->Get_Bot_Tag().Peek_Buffer();
	}

	if ( soldier->Get_Player_Data() != nullptr ) {
		const unichar_t * name = soldier->Get_Player_Data()->Get_Player_Name();
		if ( name != nullptr ) {
			return name;
		}
	}

	return U_CHAR("None");
}

//
//	The roster answers by ID.  A player who has just disconnected still has an
//	ID in flight in somebody's script, so all three of these answer a harmless
//	nothing rather than assuming the player is still there.
//

const unichar_t * Get_Player_Name_By_ID( int player_id )
{
	PlayerRosterInterfaceClass * roster = PlayerRosterClass::Peek_Interface();
	if ( roster == nullptr ) {
		return U_CHAR("None");
	}

	const unichar_t * name = roster->Get_Player_Name( player_id );
	return ( name != nullptr ) ? name : U_CHAR("None");
}


GameObject * Find_Object_By_Player_ID( int player_id )
{
	return GameObjManager::Find_Soldier_Of_Client_ID( player_id );
}


int Get_Player_Type_By_ID( int player_id )
{
	PlayerRosterInterfaceClass * roster = PlayerRosterClass::Peek_Interface();
	if ( roster == nullptr ) {
		return PLAYERTYPE_NEUTRAL;
	}

	return roster->Get_Player_Type( player_id );
}


//
//	Moves the player's record to the other side.  Destroying the body is how a
//	change takes effect immediately rather than at their next death.
//
void Change_Team( GameObject * obj, int team, bool destroy_object )
{
	SoldierGameObj * soldier = As_Soldier( obj );
	if ( soldier == nullptr || soldier->Get_Player_Data() == nullptr ) {
		return ;
	}

	soldier->Get_Player_Data()->Set_Player_Type( team );

	if ( destroy_object ) {
		Destroy_Object( obj );
	}
}

//
//	Bases and buildings
//

GameObject * Find_Building_By_Type( int team, int type )
{
	BaseControllerClass * base = BaseControllerClass::Find_Base( team );
	if ( base == nullptr ) {
		return nullptr;
	}

	return base->Find_Building( (BuildingConstants::BuildingType)type );
}

GameObject * Find_Construction_Yard( int team )
{
	return Find_Building_By_Type( team, BuildingConstants::TYPE_CONYARD );
}

GameObject * Find_Com_Center( int team )
{
	return Find_Building_By_Type( team, BuildingConstants::TYPE_COM_CENTER );
}

bool Is_Building_Dead( GameObject * obj )
{
	if ( obj == nullptr ) {
		return false;
	}

	DamageableGameObj * dgobj = obj->As_DamageableGameObj();
	if ( dgobj == nullptr ) {
		return false;
	}

	BuildingGameObj * building = dgobj->As_BuildingGameObj();
	if ( building == nullptr ) {
		return false;
	}

	return building->Is_Destroyed();
}

bool Is_Base_Powered( int team )
{
	BaseControllerClass * base = BaseControllerClass::Find_Base( team );
	if ( base == nullptr ) {
		return false;
	}

	return base->Is_Base_Powered();
}

void Power_Base( int team, bool powered )
{
	BaseControllerClass * base = BaseControllerClass::Find_Base( team );
	if ( base != nullptr ) {
		base->Power_Base( powered );
	}
}

bool Is_Radar_Enabled( int team )
{
	BaseControllerClass * base = BaseControllerClass::Find_Base( team );
	if ( base == nullptr ) {
		return false;
	}

	return base->Is_Radar_Enabled();
}

void Enable_Base_Radar( int team, bool enable )
{
	BaseControllerClass * base = BaseControllerClass::Find_Base( team );
	if ( base != nullptr ) {
		base->Enable_Radar( enable );
	}
}

void Set_Can_Generate_Soldiers( int team, bool can_generate )
{
	BaseControllerClass * base = BaseControllerClass::Find_Base( team );
	if ( base != nullptr ) {
		base->Set_Can_Generate_Soldiers( can_generate );
	}
}

//
//	Weapons
//

static WeaponBagClass *	Peek_Weapon_Bag( GameObject * obj )
{
	if ( obj == nullptr ) {
		return nullptr;
	}

	PhysicalGameObj * physical_obj = obj->As_PhysicalGameObj();
	if ( physical_obj == nullptr ) {
		return nullptr;
	}

	ArmedGameObj * armed_obj = physical_obj->As_ArmedGameObj();
	if ( armed_obj == nullptr ) {
		return nullptr;
	}

	return armed_obj->Get_Weapon_Bag();
}

const char * Get_Weapon( GameObject * obj, int position )
{
	WeaponBagClass * bag = Peek_Weapon_Bag( obj );
	if ( bag == nullptr || position <= 0 || position >= bag->Get_Count() ) {
		return "None";
	}

	WeaponClass * weapon = bag->Peek_Weapon( position );
	if ( weapon == nullptr ) {
		return "None";
	}

	return weapon->Get_Name();
}

const char * Get_Current_Weapon( GameObject * obj )
{
	WeaponBagClass * bag = Peek_Weapon_Bag( obj );
	if ( bag == nullptr ) {
		return "None";
	}

	return Get_Weapon( obj, bag->Get_Index() );
}

bool Has_Weapon( GameObject * obj, const char * weapon_name )
{
	SCRIPT_PTR_CHECK_RET( weapon_name, false );

	WeaponBagClass * bag = Peek_Weapon_Bag( obj );
	if ( bag == nullptr ) {
		return false;
	}

	for ( int index = 0; index < bag->Get_Count(); index ++ ) {
		WeaponClass * weapon = bag->Peek_Weapon( index );
		if ( weapon != nullptr && ::stricmp( weapon->Get_Name(), weapon_name ) == 0 ) {
			return true;
		}
	}

	return false;
}

static C4GameObj *	As_C4( GameObject * obj )
{
	if ( obj == nullptr ) {
		return nullptr;
	}

	PhysicalGameObj * physical_obj = obj->As_PhysicalGameObj();
	if ( physical_obj == nullptr ) {
		return nullptr;
	}

	return physical_obj->As_C4GameObj();
}

int Get_C4_Mode( GameObject * obj )
{
	C4GameObj * c4 = As_C4( obj );
	if ( c4 == nullptr || c4->Peek_Ammo_Definition() == nullptr ) {
		return 0;
	}

	return c4->Peek_Ammo_Definition()->AmmoType;
}

//
//	Who planted it.  The owner is restored first: it is stored by id and is not
//	resolved until somebody asks.
//
GameObject * Get_C4_Planter( GameObject * obj )
{
	C4GameObj * c4 = As_C4( obj );
	if ( c4 == nullptr ) {
		return nullptr;
	}

	c4->Restore_Owner();
	return c4->Get_Owner();
}

GameObject * Get_C4_Attached( GameObject * obj )
{
	C4GameObj * c4 = As_C4( obj );
	if ( c4 == nullptr ) {
		return nullptr;
	}

	return c4->Get_Stuck_Object();
}

GameObject * Get_Beacon_Planter( GameObject * obj )
{
	if ( obj == nullptr ) {
		return nullptr;
	}

	PhysicalGameObj * physical_obj = obj->As_PhysicalGameObj();
	if ( physical_obj == nullptr ) {
		return nullptr;
	}

	BeaconGameObj * beacon = physical_obj->As_BeaconGameObj();
	if ( beacon == nullptr ) {
		return nullptr;
	}

	return beacon->Get_Owner();
}

//
//	Definitions
//

int Get_Definition_ID( const char * preset_name )
{
	SCRIPT_PTR_CHECK_RET( preset_name, 0 );

	DefinitionClass * def = DefinitionMgrClass::Find_Named_Definition( preset_name );
	if ( def == nullptr ) {
		return 0;
	}

	return def->Get_ID();
}

void Repair_All_Buildings_By_Team( int team, int except_id, float health )
{
	for (	SLNode<BuildingGameObj> * node = GameObjManager::Get_Building_Game_Obj_List()->Head();
			node != nullptr;
			node = node->Next() ) {

		BuildingGameObj * building = node->Data();
		if ( building == nullptr || !Team_Matches( building, team ) ) {
			continue;
		}

		if ( Get_ID( building ) == except_id ) {
			continue;
		}

		//
		//	A building on nothing stays on nothing: this repairs damage, it
		//	does not rebuild.
		//
		float health_now = Get_Health( building );
		if ( health_now > 0 ) {
			Set_Health( building, health_now + health );
		}
	}

	return ;
}


void Repair_All_Buildings_By_Team_Radius( int team, int center_id, float health, float radius )
{
	GameObject * center = Find_Object( center_id );
	if ( center == nullptr ) {
		return ;
	}

	Vector3 position = Get_Position( center );
	float radius_squared = radius * radius;

	for (	SLNode<BuildingGameObj> * node = GameObjManager::Get_Building_Game_Obj_List()->Head();
			node != nullptr;
			node = node->Next() ) {

		BuildingGameObj * building = node->Data();
		if ( building == nullptr || !Team_Matches( building, team ) ) {
			continue;
		}

		if ( Get_ID( building ) == center_id ) {
			continue;
		}

		float health_now = Get_Health( building );
		if ( health_now <= 0 ) {
			continue;
		}

		if ( ( Get_Position( building ) - position ).Length2() <= radius_squared ) {
			Set_Health( building, health_now + health );
		}
	}

	return ;
}


//
//	A base defence is a vehicle that never moves, so it repairs itself when
//	asked rather than being repaired from here.  This is the asking.
//
void Repair_All_Static_Vehicles_By_Team( int team, int message )
{
	for (	SLNode<SmartGameObj> * node = GameObjManager::Get_Smart_Game_Obj_List()->Head();
			node != nullptr;
			node = node->Next() ) {

		SmartGameObj * smart_obj = node->Data();
		VehicleGameObj * vehicle = ( smart_obj != nullptr ) ? smart_obj->As_VehicleGameObj() : nullptr;

		if ( vehicle == nullptr || !Team_Matches( vehicle, team ) ) {
			continue;
		}

		PhysClass * physical_obj = vehicle->Peek_Physical_Object();
		if ( physical_obj != nullptr && physical_obj->As_DecorationPhysClass() != nullptr ) {
			Send_Custom_Event( vehicle, vehicle, message, 0, 0 );
		}
	}

	return ;
}


void Repair_All_Turrets_By_Team( int team, float health )
{
	for (	SLNode<SmartGameObj> * node = GameObjManager::Get_Smart_Game_Obj_List()->Head();
			node != nullptr;
			node = node->Next() ) {

		SmartGameObj * smart_obj = node->Data();
		VehicleGameObj * vehicle = ( smart_obj != nullptr ) ? smart_obj->As_VehicleGameObj() : nullptr;

		if ( vehicle == nullptr || !vehicle->Is_Turret() || !Team_Matches( vehicle, team ) ) {
			continue;
		}

		float health_now = Get_Health( vehicle );
		if ( health_now > 0 ) {
			Set_Health( vehicle, health_now + health );
		}

		float shield_now = Get_Shield_Strength( vehicle );
		if ( shield_now > 0 ) {
			Set_Shield_Strength( vehicle, shield_now + health );
		}
	}

	return ;
}


void Damage_Occupants( GameObject * obj, float amount, const char * warhead_name )
{
	VehicleGameObj * vehicle = ( obj != nullptr ) ? obj->As_VehicleGameObj() : nullptr;
	if ( vehicle == nullptr ) {
		return ;
	}

	int seats = vehicle->Get_Definition().Get_Seat_Count();
	for ( int seat = 0; seat < seats; seat ++ ) {

		SoldierGameObj * occupant = vehicle->Get_Occupant( seat );
		if ( occupant != nullptr ) {
			Apply_Damage( occupant, amount, warhead_name, nullptr );
		}
	}

	return ;
}


void Kill_Occupants( GameObject * obj )
{
	Damage_Occupants( obj, 99999.0f, "Death" );
	return ;
}


void Damage_All_Objects_Area_By_Team
(
	float					amount,
	const char *		warhead_name,
	const Vector3 &	position,
	float					radius,
	GameObject *		damager,
	bool					soldiers,
	bool					vehicles,
	int					team
)
{
	float radius_squared = radius * radius;

	//
	//	Collected first, because applying damage can destroy an object and
	//	take it out of the list this is walking.
	//
	DynamicVectorClass<GameObject *> targets;

	for (	SLNode<SmartGameObj> * node = GameObjManager::Get_Smart_Game_Obj_List()->Head();
			node != nullptr;
			node = node->Next() ) {

		SmartGameObj * smart_obj = node->Data();
		if ( smart_obj == nullptr || !Team_Matches( smart_obj, team ) ) {
			continue;
		}

		bool wanted = ( soldiers && smart_obj->As_SoldierGameObj() != nullptr )
				|| ( vehicles && smart_obj->As_VehicleGameObj() != nullptr );

		if ( !wanted ) {
			continue;
		}

		if ( ( Get_Position( smart_obj ) - position ).Length2() <= radius_squared ) {
			targets.Add( smart_obj );
		}
	}

	for ( int index = 0; index < targets.Count(); index ++ ) {
		Apply_Damage( targets[index], amount, warhead_name, damager );
	}

	return ;
}


//
//	The three ranged-damage calls differ only in how far the damage falls off
//	with distance, so they share one walk.
//
enum RangedFalloffEnum
{
	RANGED_FLAT,				// the same damage everywhere inside the radius
	RANGED_SCALED,				// full damage at the centre, none at the edge
	RANGED_PERCENTAGE			// a fraction of the target's own maximum health
};

static void	Ranged_Damage_To_Buildings_Team_Internal
(
	int						team,
	float						amount,
	const char *			warhead_name,
	const Vector3 &		position,
	float						radius,
	GameObject *			damager,
	RangedFalloffEnum		falloff
)
{
	if ( radius <= 0 ) {
		return ;
	}

	DynamicVectorClass<GameObject *> targets;
	DynamicVectorClass<float> amounts;

	for (	SLNode<BuildingGameObj> * node = GameObjManager::Get_Building_Game_Obj_List()->Head();
			node != nullptr;
			node = node->Next() ) {

		BuildingGameObj * building = node->Data();
		if ( building == nullptr || !Team_Matches( building, team ) ) {
			continue;
		}

		if ( Get_Health( building ) <= 0 ) {
			continue;
		}

		float distance = ( Get_Position( building ) - position ).Length();
		if ( distance > radius ) {
			continue;
		}

		targets.Add( building );

		switch ( falloff )
		{
			case RANGED_SCALED:
				amounts.Add( amount - distance * ( amount / radius ) );
				break;

			case RANGED_PERCENTAGE:
				amounts.Add( Get_Max_Health( building ) * amount );
				break;

			default:
				amounts.Add( amount );
				break;
		}
	}

	for ( int index = 0; index < targets.Count(); index ++ ) {
		Apply_Damage( targets[index], amounts[index], warhead_name, damager );
	}

	return ;
}


void Ranged_Damage_To_Buildings_Team( int team, float amount, const char * warhead_name,
		const Vector3 & position, float radius, GameObject * damager )
{
	Ranged_Damage_To_Buildings_Team_Internal( team, amount, warhead_name, position,
			radius, damager, RANGED_FLAT );
	return ;
}


void Ranged_Scale_Damage_To_Buildings_Team( int team, float amount, const char * warhead_name,
		const Vector3 & position, float radius, GameObject * damager )
{
	Ranged_Damage_To_Buildings_Team_Internal( team, amount, warhead_name, position,
			radius, damager, RANGED_SCALED );
	return ;
}


void Ranged_Percentage_Damage_To_Buildings_Team( int team, float fraction, const char * warhead_name,
		const Vector3 & position, float radius, GameObject * damager )
{
	Ranged_Damage_To_Buildings_Team_Internal( team, fraction, warhead_name, position,
			radius, damager, RANGED_PERCENTAGE );
	return ;
}


const char * Get_Skin( GameObject * obj )
{
	DamageableGameObj * damageable = ( obj != nullptr ) ? obj->As_DamageableGameObj() : nullptr;
	if ( damageable == nullptr ) {
		return "";
	}

	return ArmorWarheadManager::Get_Armor_Name( damageable->Get_Defense_Object()->Get_Skin() );
}


void Create_Effect_All_Stealthed_Objects_Area
(
	const Vector3 &	position,
	float					radius,
	const char *		effect_preset,
	const Vector3 &	offset,
	int					team
)
{
	//
	//	Measured on the ground, so height does not put a target out of range.
	//
	Vector3 center = position;
	center.Z = 0;

	float radius_squared = radius * radius;

	for (	SLNode<SmartGameObj> * node = GameObjManager::Get_Smart_Game_Obj_List()->Head();
			node != nullptr;
			node = node->Next() ) {

		SmartGameObj * smart_obj = node->Data();
		if ( smart_obj == nullptr || !Team_Matches( smart_obj, team ) ) {
			continue;
		}

		Vector3 where = Get_Position( smart_obj );
		Vector3 flat = where;
		flat.Z = 0;

		if ( ( flat - center ).Length2() > radius_squared ) {
			continue;
		}

		VehicleGameObj * vehicle = smart_obj->As_VehicleGameObj();
		bool hiding = Is_Stealth( smart_obj )
				|| ( vehicle != nullptr && vehicle->Is_Underground() );

		if ( hiding ) {
			Create_Object( effect_preset, where + offset );
		}
	}

	return ;
}


void Get_Translated_String( int string_id, WideStringClass & text )
{
	text = L"";

	const unichar_t * translated = TranslateDBClass::Get_String( string_id );
	if ( translated != nullptr ) {
		text = translated;
	}

	return ;
}


void Get_Team_Name( int team, WideStringClass & name )
{
	//
	//	The two playing teams are named in the translation database; anything
	//	else is neutral, which is not a team anyone plays and so is not there.
	//
	if ( team == 0 ) {
		Get_Translated_String( IDS_MP_TEAMNAME_MISSIONS_TEAM_0, name );
	} else if ( team == 1 ) {
		Get_Translated_String( IDS_MP_TEAMNAME_MISSIONS_TEAM_1, name );
	} else {
		name = L"Neutral";
	}

	return ;
}


void Get_Current_Translated_Weapon( GameObject * obj, WideStringClass & name )
{
	name = L"";

	WeaponBagClass * bag = Peek_Weapon_Bag( obj );
	if ( bag == nullptr ) {
		return ;
	}

	int index = bag->Get_Index();
	if ( index <= 0 || index >= bag->Get_Count() ) {
		return ;
	}

	WeaponClass * weapon = bag->Peek_Weapon( index );
	if ( weapon == nullptr || weapon->Get_Definition() == nullptr ) {
		return ;
	}

	Get_Translated_String( weapon->Get_Definition()->IconNameID, name );
	return ;
}


void Console_Input( const char * text )
{
	if ( text == nullptr ) {
		return ;
	}

	GameEventBus::Raise_Console_Input( text );
	return ;
}


void Console_Output( const char * format, ... )
{
	SCRIPT_PTR_CHECK( format );

	va_list arg_list;
	va_start( arg_list, format );

	StringClass text;
	text.Format_Args( format, arg_list );

	va_end( arg_list );

	GameEventBus::Raise_Console_Print( text.Peek_Buffer() );
	return ;
}


void Grant_Weapon( GameObject * obj, const char * weapon_name, bool grant,
		int rounds, bool in_clips )
{
	if ( !grant ) {
		Remove_Weapon( obj, weapon_name );
		return ;
	}

	WeaponBagClass * bag = Peek_Weapon_Bag( obj );
	if ( bag == nullptr || weapon_name == nullptr ) {
		return ;
	}

	const WeaponDefinitionClass * def = WeaponManager::Find_Weapon_Definition( weapon_name );
	if ( def == nullptr ) {
		Debug_Say(( "Weapon Definition %s not found\n", weapon_name ));
		return ;
	}

	//
	//	A count in clips is a count in rounds once the clip size is known.
	//	Negative counts are the engine's "as much as it holds" and are passed
	//	through untouched.
	//
	if ( in_clips && rounds > 0 ) {
		rounds *= (int)def->ClipSize;
	}

	bag->Add_Weapon( def, rounds, true );
	return ;
}


bool Grant_Weapon_Definition( GameObject * obj, int definition_id, bool select )
{
	if ( obj == nullptr || definition_id == 0 ) {
		return false;
	}

	const WeaponDefinitionClass * def = WeaponManager::Find_Weapon_Definition( definition_id );
	if ( def == nullptr || def->Get_Name() == nullptr ) {
		return false;
	}

	Grant_Weapon( obj, def->Get_Name(), true, -1, true );

	if ( select ) {
		Select_Weapon( obj, def->Get_Name() );
	}

	return true;
}


//
//	The match that is running
//
//	Commando owns all of this; see gameinfo.h for why the questions are asked
//	through an interface instead of reached for directly.
//

float Get_Time_Remaining_Seconds( void )
{
	GameInfoInterfaceClass * game = GameInfoClass::Peek_Interface();
	return ( game != nullptr ) ? game->Get_Time_Remaining_Seconds() : 0.0f;
}

void Set_Time_Remaining_Seconds( float seconds )
{
	GameInfoInterfaceClass * game = GameInfoClass::Peek_Interface();
	if ( game != nullptr ) {
		game->Set_Time_Remaining_Seconds( seconds );
	}
}

int Get_Time_Limit_Minutes( void )
{
	GameInfoInterfaceClass * game = GameInfoClass::Peek_Interface();
	return ( game != nullptr ) ? game->Get_Time_Limit_Minutes() : 0;
}

void Set_Time_Limit_Minutes( int minutes )
{
	GameInfoInterfaceClass * game = GameInfoClass::Peek_Interface();
	if ( game != nullptr ) {
		game->Set_Time_Limit_Minutes( minutes );
	}
}

unsigned int Get_Game_Duration_Seconds( void )
{
	GameInfoInterfaceClass * game = GameInfoClass::Peek_Interface();
	return ( game != nullptr ) ? game->Get_Duration_Seconds() : 0;
}

int Get_Game_Win_Type( void )
{
	GameInfoInterfaceClass * game = GameInfoClass::Peek_Interface();
	return ( game != nullptr ) ? game->Get_Win_Type() : 0;
}

int Get_Game_Winner_ID( void )
{
	GameInfoInterfaceClass * game = GameInfoClass::Peek_Interface();
	return ( game != nullptr ) ? game->Get_Winner_ID() : -1;
}

int Get_Game_Max_Players( void )
{
	GameInfoInterfaceClass * game = GameInfoClass::Peek_Interface();
	return ( game != nullptr ) ? game->Get_Max_Players() : 0;
}

const char * Get_Map_Name( void )
{
	GameInfoInterfaceClass * game = GameInfoClass::Peek_Interface();
	const char * name = ( game != nullptr ) ? game->Get_Map_Name() : nullptr;
	return ( name != nullptr ) ? name : "";
}

const unichar_t * Get_Game_Title( void )
{
	GameInfoInterfaceClass * game = GameInfoClass::Peek_Interface();
	const unichar_t * title = ( game != nullptr ) ? game->Get_Title() : nullptr;
	return ( title != nullptr ) ? title : U_CHAR("");
}

float Get_Team_Score( int player_type )
{
	GameInfoInterfaceClass * game = GameInfoClass::Peek_Interface();
	return ( game != nullptr ) ? game->Get_Team_Score( player_type ) : 0.0f;
}

int Get_Team_Credits( int player_type )
{
	GameInfoInterfaceClass * game = GameInfoClass::Peek_Interface();
	return ( game != nullptr ) ? game->Get_Team_Credits( player_type ) : 0;
}


//
//	Turn the radar on or off for one whole side.  Player type 2 is neither
//	side and means everybody, which is the convention the 4.8.4 library used.
//
void Enable_Radar_Team( int player_type, bool onoff )
{
	SLNode<SoldierGameObj> * node = GameObjManager::Get_Star_Game_Obj_List()->Head();
	while ( node != nullptr ) {

		SoldierGameObj * soldier = node->Data();
		node = node->Next();

		if ( soldier == nullptr ) {
			continue;
		}

		if ( player_type == 2 || soldier->Get_Player_Type() == player_type ) {
			Enable_Radar_Player( soldier, onoff );
		}
	}
}


void Remove_Weapon( GameObject * obj, const char * weapon_name )
{
	WeaponBagClass * bag = Peek_Weapon_Bag( obj );
	if ( bag == nullptr || weapon_name == nullptr ) {
		return ;
	}

	for ( int index = 0; index < bag->Get_Count(); index ++ ) {

		WeaponClass * weapon = bag->Peek_Weapon( index );
		if ( weapon == nullptr || weapon->Get_Name() == nullptr ) {
			continue;
		}

		if ( ::_stricmp( weapon->Get_Name(), weapon_name ) == 0 ) {
			bag->Remove_Weapon( index );
			return ;
		}
	}

	return ;
}


void Set_Skin( GameObject * obj, const char * armor_name )
{
	DamageableGameObj * damageable = ( obj != nullptr ) ? obj->As_DamageableGameObj() : nullptr;
	if ( damageable == nullptr ) {
		return ;
	}

	damageable->Get_Defense_Object()->Set_Skin( ArmorWarheadManager::Get_Armor_Type( armor_name ) );
	return ;
}


void Set_Max_Health( GameObject * obj, float health )
{
	DamageableGameObj * damageable = ( obj != nullptr ) ? obj->As_DamageableGameObj() : nullptr;
	if ( damageable == nullptr ) {
		return ;
	}

	damageable->Get_Defense_Object()->Set_Health_Max( health );
	return ;
}


void Set_Max_Shield_Strength( GameObject * obj, float strength )
{
	DamageableGameObj * damageable = ( obj != nullptr ) ? obj->As_DamageableGameObj() : nullptr;
	if ( damageable == nullptr ) {
		return ;
	}

	damageable->Get_Defense_Object()->Set_Shield_Strength_Max( strength );
	return ;
}


void Set_Damage_Points( GameObject * obj, float points )
{
	DamageableGameObj * damageable = ( obj != nullptr ) ? obj->As_DamageableGameObj() : nullptr;
	if ( damageable == nullptr ) {
		return ;
	}

	damageable->Get_Defense_Object()->Set_Damage_Points( points );
	return ;
}


void Set_Death_Points( GameObject * obj, float points )
{
	DamageableGameObj * damageable = ( obj != nullptr ) ? obj->As_DamageableGameObj() : nullptr;
	if ( damageable == nullptr ) {
		return ;
	}

	damageable->Get_Defense_Object()->Set_Death_Points( points );
	return ;
}


void Damage_All_Objects_Area
(
	float					amount,
	const char *		warhead_name,
	const Vector3 &	position,
	float					radius,
	int					team,
	GameObject *		damager
)
{
	float radius_squared = radius * radius;

	//
	//	Collected first, because applying damage can destroy an object and
	//	take it out of the list this is walking.
	//
	DynamicVectorClass<GameObject *> targets;

	for (	SLNode<BaseGameObj> * node = GameObjManager::Get_Game_Obj_List()->Head();
			node != nullptr;
			node = node->Next() ) {

		ScriptableGameObj * obj = ( node->Data() != nullptr ) ? node->Data()->As_ScriptableGameObj() : nullptr;
		PhysicalGameObj * physical = ( obj != nullptr ) ? obj->As_PhysicalGameObj() : nullptr;

		if ( physical == nullptr ) {
			continue;
		}

		if ( team == 0 || team == 1 ) {
			if ( physical->Get_Player_Type() != team ) {
				continue;
			}
		}

		Vector3 delta = Get_Position( obj ) - position;
		if ( delta.Length2() > radius_squared ) {
			continue;
		}

		targets.Add( obj );
	}

	for ( int index = 0; index < targets.Count(); index ++ ) {
		Apply_Damage( targets[index], amount, warhead_name, damager );
	}

	return ;
}


const char * Get_Definition_Name( int definition_id )
{
	if ( definition_id == 0 ) {
		return "none";
	}

	DefinitionClass * def = DefinitionMgrClass::Find_Definition( definition_id, true );
	if ( def == nullptr ) {
		return "none";
	}

	return def->Get_Name();
}


//
//	The translated name a definition carries, if it carries one.  Only a
//	damageable thing has one -- a building, a vehicle, a soldier -- which is
//	also the only kind of thing anybody wants a name for.
//
static bool	Translated_Name_Of( DefinitionClass * def, WideStringClass & name )
{
	DamageableGameObjDef * damageable = dynamic_cast<DamageableGameObjDef *>( def );
	if ( damageable == nullptr ) {
		return false;
	}

	int name_id = damageable->Get_Translated_Name_ID();
	if ( name_id == 0 ) {
		return false;
	}

	const unichar_t * translated = TranslateDBClass::Get_String( name_id );
	if ( translated == nullptr || translated[0] == 0 ) {
		return false;
	}

	name = translated;
	return true;
}


void Get_Translated_Definition_Name( int definition_id, WideStringClass & name )
{
	name = L"";

	DefinitionClass * def = DefinitionMgrClass::Find_Definition( definition_id, true );
	if ( def == nullptr ) {
		return ;
	}

	if ( !Translated_Name_Of( def, name ) ) {
		name = def->Get_Name();
	}

	return ;
}


void Get_Translated_Preset_Name( GameObject * obj, WideStringClass & name )
{
	name = L"";

	if ( obj == nullptr ) {
		return ;
	}

	Get_Translated_Definition_Name( (int)obj->Get_Definition().Get_ID(), name );
	return ;
}

bool Is_Valid_Preset_ID( int definition_id )
{
	return DefinitionMgrClass::Find_Definition( definition_id, true ) != nullptr;
}

//
//	Translated strings
//

bool Is_Valid_String_ID( int string_id )
{
	return TranslateDBClass::Find_Object( (uint32)string_id ) != nullptr;
}

int Get_String_Sound_ID( int string_id )
{
	TDBObjClass * entry = TranslateDBClass::Find_Object( (uint32)string_id );
	if ( entry == nullptr ) {
		return 0;
	}

	return (int)entry->Get_Sound_ID();
}

//
//	Scripts on objects
//

ScriptClass * Find_Script_On_Object( GameObject * obj, const char * script_name )
{
	if ( obj == nullptr || script_name == nullptr ) {
		return nullptr;
	}

	const GameObjObserverList & observers = obj->Get_Observers();
	for ( int index = 0; index < observers.Count(); index ++ ) {

		GameObjObserverClass * observer = observers[ index ];
		if ( observer != nullptr && ::stricmp( observer->Get_Name(), script_name ) == 0 ) {
			return (ScriptClass *)observer;
		}
	}

	return nullptr;
}

bool Is_Script_Attached( GameObject * obj, const char * script_name )
{
	return Find_Script_On_Object( obj, script_name ) != nullptr;
}

void Attach_Script_Once( GameObject * obj, const char * script_name, const char * params )
{
	SCRIPT_PTR_CHECK( obj );

	if ( !Is_Script_Attached( obj, script_name ) ) {
		Attach_Script( obj, script_name, params );
	}
}

void Attach_Script_Once_V( GameObject * obj, const char * script_name, const char * format, ... )
{
	SCRIPT_PTR_CHECK( obj );
	SCRIPT_PTR_CHECK( format );

	//
	//	Tested before the list is built rather than after: a script already
	//	attached is the common case, and there is no reason to format for it.
	//
	if ( Is_Script_Attached( obj, script_name ) ) {
		return ;
	}

	va_list arg_list;
	va_start( arg_list, format );

	StringClass params;
	params.Format_Args( format, arg_list );

	va_end( arg_list );

	Attach_Script( obj, script_name, params );
	return ;
}


void Attach_Script_Occupants( GameObject * obj, const char * script_name, const char * params )
{
	VehicleGameObj * vehicle = As_Vehicle( obj );
	if ( vehicle == nullptr ) {
		return ;
	}

	int seat_count = vehicle->Get_Definition().Get_Seat_Count();
	for ( int seat = 0; seat < seat_count; seat ++ ) {

		SoldierGameObj * occupant = vehicle->Get_Occupant( seat );
		if ( occupant != nullptr ) {
			Attach_Script( occupant, script_name, params );
		}
	}
}

void Attach_Script_Preset( const char * script_name, const char * params, const char * preset_name, int team, bool once )
{
	SCRIPT_PTR_CHECK( preset_name );

	for ( SLNode<BaseGameObj> * node = GameObjManager::Get_Game_Obj_List()->Head();
			node != nullptr; node = node->Next() ) {

		ScriptableGameObj * obj = node->Data()->As_ScriptableGameObj();
		if ( obj == nullptr ) {
			continue;
		}

		if ( ::stricmp( obj->Get_Definition().Get_Name(), preset_name ) != 0 || !Team_Matches( obj, team ) ) {
			continue;
		}

		if ( once ) {
			Attach_Script_Once( obj, script_name, params );
		} else {
			Attach_Script( obj, script_name, params );
		}
	}
}

void Attach_Script_Type( const char * script_name, const char * params, unsigned long class_id, int team, bool once )
{
	for ( SLNode<BaseGameObj> * node = GameObjManager::Get_Game_Obj_List()->Head();
			node != nullptr; node = node->Next() ) {

		ScriptableGameObj * obj = node->Data()->As_ScriptableGameObj();
		if ( obj == nullptr ) {
			continue;
		}

		if ( obj->Get_Definition().Get_Class_ID() != class_id || !Team_Matches( obj, team ) ) {
			continue;
		}

		if ( once ) {
			Attach_Script_Once( obj, script_name, params );
		} else {
			Attach_Script( obj, script_name, params );
		}
	}
}

void Attach_Script_Building( const char * script_name, const char * params, int team )
{
	for ( SLNode<BuildingGameObj> * node = GameObjManager::Get_Building_Game_Obj_List()->Head();
			node != nullptr; node = node->Next() ) {

		BuildingGameObj * building = node->Data();
		if ( Team_Matches( building, team ) ) {
			Attach_Script( building, script_name, params );
		}
	}
}

void Attach_Script_Player_Once( const char * script_name, const char * params, int team )
{
	for ( SLNode<SoldierGameObj> * node = GameObjManager::Get_Star_Game_Obj_List()->Head();
			node != nullptr; node = node->Next() ) {

		SoldierGameObj * soldier = node->Data();
		if ( Team_Matches( soldier, team ) ) {
			Attach_Script_Once( soldier, script_name, params );
		}
	}
}

//
//	A script asked to remove itself is destroyed rather than detached, so its
//	own Destroy_Script runs and it lets go of whatever it was holding.
//
void Remove_Script( GameObject * obj, const char * script_name )
{
	SCRIPT_PTR_CHECK( obj );
	SCRIPT_PTR_CHECK( script_name );

	const GameObjObserverList & observers = obj->Get_Observers();
	for ( int index = observers.Count() - 1; index >= 0; index -- ) {

		GameObjObserverClass * observer = observers[ index ];
		if ( observer != nullptr && ::stricmp( observer->Get_Name(), script_name ) == 0 ) {
			ScriptManager::Request_Destroy_Script( (ScriptClass *)observer );
		}
	}
}

void Remove_All_Scripts( GameObject * obj )
{
	SCRIPT_PTR_CHECK( obj );

	const GameObjObserverList & observers = obj->Get_Observers();
	for ( int index = observers.Count() - 1; index >= 0; index -- ) {

		GameObjObserverClass * observer = observers[ index ];
		if ( observer != nullptr && ( (ScriptClass *)observer )->Owner() != nullptr ) {
			ScriptManager::Request_Destroy_Script( (ScriptClass *)observer );
		}
	}
}

void Remove_Script_Preset( const char * script_name, const char * preset_name, int team )
{
	SCRIPT_PTR_CHECK( preset_name );

	for ( SLNode<BaseGameObj> * node = GameObjManager::Get_Game_Obj_List()->Head();
			node != nullptr; node = node->Next() ) {

		ScriptableGameObj * obj = node->Data()->As_ScriptableGameObj();
		if ( obj == nullptr ) {
			continue;
		}

		if ( ::stricmp( obj->Get_Definition().Get_Name(), preset_name ) == 0 && Team_Matches( obj, team ) ) {
			Remove_Script( obj, script_name );
		}
	}
}

void Remove_Script_Type( const char * script_name, unsigned long class_id, int team )
{
	for ( SLNode<BaseGameObj> * node = GameObjManager::Get_Game_Obj_List()->Head();
			node != nullptr; node = node->Next() ) {

		ScriptableGameObj * obj = node->Data()->As_ScriptableGameObj();
		if ( obj == nullptr ) {
			continue;
		}

		if ( obj->Get_Definition().Get_Class_ID() == class_id && Team_Matches( obj, team ) ) {
			Remove_Script( obj, script_name );
		}
	}
}

//
//	Broadcasting a custom
//

void Send_Custom_All_Objects( int type, GameObject * sender, int team )
{
	SCRIPT_PTR_CHECK( sender );

	for ( SLNode<BaseGameObj> * node = GameObjManager::Get_Game_Obj_List()->Head();
			node != nullptr; node = node->Next() ) {

		ScriptableGameObj * obj = node->Data()->As_ScriptableGameObj();
		if ( obj != nullptr && Team_Matches( obj, team ) ) {
			Send_Custom_Event( sender, obj, type, 0, 0.0f );
		}
	}
}

void Send_Custom_All_Objects_Area( int type, const Vector3 & position, float distance, GameObject * sender, int team )
{
	SCRIPT_PTR_CHECK( sender );

	for ( SLNode<BaseGameObj> * node = GameObjManager::Get_Game_Obj_List()->Head();
			node != nullptr; node = node->Next() ) {

		ScriptableGameObj * obj = node->Data()->As_ScriptableGameObj();
		if ( obj == nullptr || !Team_Matches( obj, team ) ) {
			continue;
		}

		Vector3 offset = Get_Position( obj ) - position;
		offset.Z = 0.0f;
		if ( offset.Length2() <= ( distance * distance ) ) {
			Send_Custom_Event( sender, obj, type, 0, 0.0f );
		}
	}
}

void Send_Custom_To_Preset( GameObject * sender, const char * preset_name, int type, int param, float delay )
{
	SCRIPT_PTR_CHECK( sender );
	SCRIPT_PTR_CHECK( preset_name );

	for ( SLNode<BaseGameObj> * node = GameObjManager::Get_Game_Obj_List()->Head();
			node != nullptr; node = node->Next() ) {

		ScriptableGameObj * obj = node->Data()->As_ScriptableGameObj();
		if ( obj != nullptr && ::stricmp( obj->Get_Definition().Get_Name(), preset_name ) == 0 ) {
			Send_Custom_Event( sender, obj, type, param, delay );
		}
	}
}

//
//	Effects over a set of objects
//

void Create_Effect_All_Of_Preset( const char * effect_preset_name, const char * preset_name, float z_adjust, bool z_absolute )
{
	SCRIPT_PTR_CHECK( effect_preset_name );
	SCRIPT_PTR_CHECK( preset_name );

	for ( SLNode<SmartGameObj> * node = GameObjManager::Get_Smart_Game_Obj_List()->Head();
			node != nullptr; node = node->Next() ) {

		SmartGameObj * obj = node->Data();
		if ( ::stricmp( obj->Get_Definition().Get_Name(), preset_name ) != 0 ) {
			continue;
		}

		Vector3 position = Get_Position( obj );
		if ( z_absolute ) {
			position.Z = z_adjust;
		} else {
			position.Z += z_adjust;
		}

		GameObject * effect = Create_Object( effect_preset_name, position );
		if ( effect != nullptr ) {
			Set_Facing( effect, Get_Facing( obj ) );
		}
	}
}

//
//	Time
//

void Seconds_To_Hms( float seconds, int & out_hours, int & out_minutes, int & out_seconds )
{
	int total = (int)seconds;

	out_hours	= total / 3600;
	out_minutes	= ( total % 3600 ) / 60;
	out_seconds	= total % 60;
}



/*
**	Reading an ini file that is not one of the engine's own.  A script may ship
**	its own configuration -- a table of sounds, a table of kill messages -- and
**	wants it read through the same mix-file lookup everything else uses rather
**	than through the operating system.  The caller owns what comes back and
**	hands it to Release_INI.
*/
INIClass * Get_INI( const char * filename )
{
	SCRIPT_PTR_CHECK_RET( filename, nullptr );

	if ( _TheFileFactory == nullptr ) {
		return nullptr;
	}

	FileClass * file = _TheFileFactory->Get_File( filename );
	if ( file == nullptr ) {
		return nullptr;
	}

	INIClass * ini = nullptr;
	if ( file->Is_Available() ) {
		ini = new INIClass( *file );
	}

	_TheFileFactory->Return_File( file );
	return ini;
}


void Release_INI( INIClass * ini )
{
	delete ini;
}


/*
**	A team's colour, asked about an object rather than about the team.  The
**	4.8.4 shape returned it through three unsigned out-parameters; here it is
**	one vector, which is what the engine keeps and what every caller wanted.
*/
Vector3 Get_Object_Color( GameObject * obj )
{
	if ( obj == nullptr ) {
		return Vector3( 1.0f, 1.0f, 1.0f );
	}

	DamageableGameObj * damageable = obj->As_DamageableGameObj();
	if ( damageable == nullptr ) {
		return Vector3( 1.0f, 1.0f, 1.0f );
	}

	return damageable->Get_Team_Color();
}


/*
**	Zones made at runtime.  A script zone's bounding box is not part of its
**	preset -- a level editor places it -- so a zone created from a preset has
**	to be given one before anything is allowed to observe it entering.
*/
void Set_Zone_Box( GameObject * obj, const OBBoxClass & box )
{
	if ( obj == nullptr ) {
		return ;
	}

	ScriptZoneGameObj * zone = obj->As_ScriptZoneGameObj();
	if ( zone == nullptr ) {
		return ;
	}

	zone->Set_Bounding_Box( const_cast<OBBoxClass &>( box ) );
}


GameObject * Create_Zone( const char * preset_name, const OBBoxClass & box )
{
	SCRIPT_PTR_CHECK_RET( preset_name, nullptr );

	GameObject * zone = Create_Object( preset_name, box.Center );
	if ( zone == nullptr ) {
		return nullptr;
	}

	Set_Zone_Box( zone, box );
	zone->Start_Observers();
	return zone;
}


/*
**	What a purchase terminal charges one side for a preset.  Zero means the
**	preset is not sold there, which is also what a free unit answers -- the
**	4.8.4 shape had the same ambiguity and every caller treats both the same
**	way, as "no price to divide by".
**
**	The beacon is asked about first because it is the one entry a team's
**	settings carry directly rather than on one of the pages.
*/
//
//	The page geometry is the purchase settings' own, but those constants are
//	protected, so the two that matter are spelled out here.  A page holds ten
//	entries and each entry may carry three alternate skins.
//
static const int	PURCHASE_ENTRIES		= 10;
static const int	PURCHASE_ALTERNATES	= 3;

int Get_Team_Cost( int definition_id, int player_type )
{
	if ( definition_id == 0 ) {
		return 0;
	}

	TeamPurchaseSettingsDefClass::TEAM team = ( player_type == PLAYERTYPE_NOD )
			? TeamPurchaseSettingsDefClass::TEAM_NOD
			: TeamPurchaseSettingsDefClass::TEAM_GDI;

	TeamPurchaseSettingsDefClass * team_settings = TeamPurchaseSettingsDefClass::Get_Definition( team );
	if ( team_settings != nullptr && team_settings->Get_Beacon_Definition() == definition_id ) {
		return team_settings->Get_Beacon_Cost();
	}

	PurchaseSettingsDefClass::TEAM page_team = ( player_type == PLAYERTYPE_NOD )
			? PurchaseSettingsDefClass::TEAM_NOD
			: PurchaseSettingsDefClass::TEAM_GDI;

	for ( int type = 0; type < PurchaseSettingsDefClass::TYPE_COUNT; type ++ ) {

		PurchaseSettingsDefClass * page = PurchaseSettingsDefClass::Find_Definition(
				(PurchaseSettingsDefClass::TYPE)type, page_team );
		if ( page == nullptr ) {
			continue;
		}

		for ( int index = 0; index < PURCHASE_ENTRIES; index ++ ) {

			bool is_match = ( page->Get_Definition( index ) == definition_id );
			for ( int alt = 0; !is_match && alt < PURCHASE_ALTERNATES; alt ++ ) {
				is_match = ( page->Get_Alt_Definition( index, alt ) == definition_id );
			}

			if ( is_match ) {
				return page->Get_Cost( index );
			}
		}
	}

	return 0;
}


int Get_Team_Cost( const char * preset_name, int player_type )
{
	SCRIPT_PTR_CHECK_RET( preset_name, 0 );
	return Get_Team_Cost( Get_Definition_ID( preset_name ), player_type );
}


int Get_Cost( int definition_id )
{
	int cost = Get_Team_Cost( definition_id, PLAYERTYPE_NOD );
	if ( cost == 0 ) {
		cost = Get_Team_Cost( definition_id, PLAYERTYPE_GDI );
	}

	return cost;
}


int Get_Cost( const char * preset_name )
{
	SCRIPT_PTR_CHECK_RET( preset_name, 0 );
	return Get_Cost( Get_Definition_ID( preset_name ) );
}


}	// namespace ScriptEngine
