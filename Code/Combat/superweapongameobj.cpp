/***********************************************************************************************
 ***                            Confidential - Westwood Studios                              ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/superweapongameobj.cpp                $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#include "superweapongameobj.h"
#include "basecontroller.h"
#include "combat.h"
#include "combatchunkid.h"
#include "debug.h"
#include "definitionmgr.h"
#include "persistfactory.h"
#include "phys.h"
#include "pscene.h"
#include "rendobj.h"
#include "simpledefinitionfactory.h"
#include "staticanimphys.h"
#include "timemgr.h"
#include "wwhack.h"
#include "wwprofile.h"


////////////////////////////////////////////////////////////////
//	Hacks
////////////////////////////////////////////////////////////////
DECLARE_FORCE_LINK (Superweapon)

////////////////////////////////////////////////////////////////
//	Editable and persist factories
////////////////////////////////////////////////////////////////
SimplePersistFactoryClass	<SuperweaponGameObjDef,	CHUNKID_GAME_OBJECT_DEF_SUPERWEAPON>						_SuperweaponGameObjDefPersistFactory;
SimplePersistFactoryClass	<SuperweaponGameObj,		CHUNKID_GAME_OBJECT_SUPERWEAPON>								_SuperweaponGameObjPersistFactory;
DECLARE_DEFINITION_FACTORY (SuperweaponGameObjDef,	CLASSID_GAME_OBJECT_DEF_SUPERWEAPON, "Superweapon")	_SuperweaponGameObjDefDefFactory;

////////////////////////////////////////////////////////////////
//	Constants
////////////////////////////////////////////////////////////////

//	Marker in the model name of the static anim phys that plays the launch.
static const char * const LAUNCH_ANIMATION_MARKER = "WEP#LAUNCH";

//	How long a launch runs before the building goes quiet again.
static const float LAUNCH_DURATION = 10.0F;

////////////////////////////////////////////////////////////////
//	Save/Load constants
////////////////////////////////////////////////////////////////
enum
{
	CHUNKID_DEF_PARENT						=	0x02240904,
	CHUNKID_DEF_VARIABLES,

	MICROCHUNKID_DEF_UNUSED					= 1,
};

enum
{
	CHUNKID_PARENT								=	0x02240906,
	CHUNKID_VARIABLES,

	MICROCHUNKID_LAUNCH_TIMER				= 1,
	MICROCHUNKID_LAUNCHING,
};


////////////////////////////////////////////////////////////////
//
//	SuperweaponGameObjDef
//
////////////////////////////////////////////////////////////////
SuperweaponGameObjDef::SuperweaponGameObjDef (void)
{
	return ;
}

SuperweaponGameObjDef::~SuperweaponGameObjDef (void)
{
	return ;
}

uint32
SuperweaponGameObjDef::Get_Class_ID (void) const
{
	return CLASSID_GAME_OBJECT_DEF_SUPERWEAPON;
}

PersistClass *
SuperweaponGameObjDef::Create (void) const
{
	SuperweaponGameObj *building = new SuperweaponGameObj;
	building->Init (*this);

	return building;
}

bool
SuperweaponGameObjDef::Save (ChunkSaveClass &csave)
{
	csave.Begin_Chunk (CHUNKID_DEF_PARENT);
		BuildingGameObjDef::Save (csave);
	csave.End_Chunk ();

	csave.Begin_Chunk (CHUNKID_DEF_VARIABLES);
	csave.End_Chunk ();

	return true;
}

bool
SuperweaponGameObjDef::Load (ChunkLoadClass &cload)
{
	while (cload.Open_Chunk ()) {
		switch (cload.Cur_Chunk_ID ()) {

			case CHUNKID_DEF_PARENT:
				BuildingGameObjDef::Load (cload);
				break;

			case CHUNKID_DEF_VARIABLES:
				Load_Variables (cload);
				break;

			default:
				Debug_Say (("Unrecognized Superweapon Def chunkID\n"));
				break;
		}

		cload.Close_Chunk ();
	}

	return true;
}

void
SuperweaponGameObjDef::Load_Variables (ChunkLoadClass &cload)
{
	while (cload.Open_Micro_Chunk ()) {
		cload.Close_Micro_Chunk ();
	}

	return ;
}

const PersistFactoryClass &
SuperweaponGameObjDef::Get_Factory (void) const
{
	return _SuperweaponGameObjDefPersistFactory;
}


////////////////////////////////////////////////////////////////
//
//	SuperweaponGameObj
//
////////////////////////////////////////////////////////////////
SuperweaponGameObj::SuperweaponGameObj (void)	:
	LaunchAnimationID (0),
	LaunchTimer (0),
	Launching (false)
{
	return ;
}

SuperweaponGameObj::~SuperweaponGameObj (void)
{
	return ;
}

const PersistFactoryClass &
SuperweaponGameObj::Get_Factory (void) const
{
	return _SuperweaponGameObjPersistFactory;
}

void
SuperweaponGameObj::Init (void)
{
	Init (Get_Definition ());
	return ;
}

void
SuperweaponGameObj::Init (const SuperweaponGameObjDef &definition)
{
	BuildingGameObj::Init (definition);
	return ;
}

const SuperweaponGameObjDef &
SuperweaponGameObj::Get_Definition (void) const
{
	return (const SuperweaponGameObjDef &)BaseGameObj::Get_Definition ();
}

bool
SuperweaponGameObj::Save (ChunkSaveClass &csave)
{
	csave.Begin_Chunk (CHUNKID_PARENT);
		BuildingGameObj::Save (csave);
	csave.End_Chunk ();

	csave.Begin_Chunk (CHUNKID_VARIABLES);
		WRITE_MICRO_CHUNK (csave, MICROCHUNKID_LAUNCH_TIMER, LaunchTimer);
		WRITE_MICRO_CHUNK (csave, MICROCHUNKID_LAUNCHING, Launching);
	csave.End_Chunk ();

	return true;
}

bool
SuperweaponGameObj::Load (ChunkLoadClass &cload)
{
	while (cload.Open_Chunk ()) {
		switch (cload.Cur_Chunk_ID ()) {

			case CHUNKID_PARENT:
				BuildingGameObj::Load (cload);
				break;

			case CHUNKID_VARIABLES:
				Load_Variables (cload);
				break;

			default:
				Debug_Say (("Unrecognized Superweapon chunkID\n"));
				break;
		}

		cload.Close_Chunk ();
	}

	return true;
}

void
SuperweaponGameObj::Load_Variables (ChunkLoadClass &cload)
{
	while (cload.Open_Micro_Chunk ()) {
		switch (cload.Cur_Micro_Chunk_ID ()) {

			READ_MICRO_CHUNK (cload, MICROCHUNKID_LAUNCH_TIMER, LaunchTimer);
			READ_MICRO_CHUNK (cload, MICROCHUNKID_LAUNCHING, Launching);
		}

		cload.Close_Micro_Chunk ();
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	CnC_Initialize
//
//	Find the launch animation, the same way the war factory finds its
//	construction animation: the nearest static anim phys whose model name
//	carries the marker.  A level that has no such object simply never plays
//	one.
//
////////////////////////////////////////////////////////////////
void
SuperweaponGameObj::CnC_Initialize (BaseControllerClass *base)
{
	BuildingGameObj::CnC_Initialize (base);

	Vector3 pos;
	Get_Position (&pos);

	float closest2 = 99999.0F;

	RefPhysListIterator iterator = PhysicsSceneClass::Get_Instance ()->Get_Static_Object_Iterator ();
	for (iterator.First (); !iterator.Is_Done (); iterator.Next ()) {

		StaticAnimPhysClass *anim_phys_obj = iterator.Peek_Obj ()->As_StaticAnimPhysClass ();
		if (anim_phys_obj == nullptr || anim_phys_obj->Peek_Model () == nullptr) {
			continue;
		}

		StringClass name = anim_phys_obj->Peek_Model ()->Get_Name ();
		name.To_Upper ();
		if (::strstr (name, LAUNCH_ANIMATION_MARKER) == nullptr) {
			continue;
		}

		Vector3 anim_pos;
		anim_phys_obj->Get_Position (&anim_pos);

		float dist2 = (anim_pos - pos).Length2 ();
		if (dist2 < closest2) {
			closest2				= dist2;
			LaunchAnimationID	= anim_phys_obj->Get_ID ();
		}
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Launch
//
////////////////////////////////////////////////////////////////
void
SuperweaponGameObj::Launch (void)
{
	//
	//	A destroyed weapon cannot fire, and a launch already running is not
	//	restarted -- the animation would jump back to the beginning.
	//
	if (Is_Destroyed () || Launching) {
		return ;
	}

	Launching	= true;
	LaunchTimer	= LAUNCH_DURATION;

	Play_Launch_Animation (true);

	Set_Object_Dirty_Bit (NetworkObjectClass::BIT_OCCASIONAL, true);

	return ;
}


////////////////////////////////////////////////////////////////
//
//	On_Destroyed
//
////////////////////////////////////////////////////////////////
void
SuperweaponGameObj::On_Destroyed (void)
{
	BuildingGameObj::On_Destroyed ();

	//
	//	Whatever was in the tube is not coming out.
	//
	if (Launching) {
		Launching	= false;
		LaunchTimer	= 0;
		Play_Launch_Animation (false);
		Set_Object_Dirty_Bit (NetworkObjectClass::BIT_OCCASIONAL, true);
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Think
//
////////////////////////////////////////////////////////////////
void
SuperweaponGameObj::Think (void)
{
	WWPROFILE ("Superweapon Think");

	BuildingGameObj::Think ();

	if (Launching == false) {
		return ;
	}

	LaunchTimer -= TimeManager::Get_Frame_Seconds ();
	if (LaunchTimer > 0) {
		return ;
	}

	//
	//	The server owns when the launch ends; a client is told.
	//
	if (CombatManager::I_Am_Server ()) {
		Launching	= false;
		LaunchTimer	= 0;
		Play_Launch_Animation (false);
		Set_Object_Dirty_Bit (NetworkObjectClass::BIT_OCCASIONAL, true);
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Play_Launch_Animation
//
////////////////////////////////////////////////////////////////
void
SuperweaponGameObj::Play_Launch_Animation (bool onoff)
{
	StaticPhysClass *static_phys_obj = PhysicsSceneClass::Get_Instance ()->Find_Static_Object (LaunchAnimationID);
	if (static_phys_obj == nullptr) {
		return ;
	}

	StaticAnimPhysClass *anim_phys_obj = static_phys_obj->As_StaticAnimPhysClass ();
	if (anim_phys_obj == nullptr) {
		return ;
	}

	AnimCollisionManagerClass &anim_mgr = anim_phys_obj->Get_Animation_Manager ();
	anim_mgr.Set_Animation_Mode (AnimCollisionManagerClass::ANIMATE_TARGET);

	if (onoff) {
		anim_mgr.Set_Target_Frame_End ();
	} else {
		anim_mgr.Set_Target_Frame (0);
	}

	static_phys_obj->Enable_Is_State_Dirty (true);

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Export_Occasional
//
////////////////////////////////////////////////////////////////
void
SuperweaponGameObj::Export_Occasional (BitStreamClass &packet)
{
	BuildingGameObj::Export_Occasional (packet);

	packet.Add (Launching);

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Import_Occasional
//
////////////////////////////////////////////////////////////////
void
SuperweaponGameObj::Import_Occasional (BitStreamClass &packet)
{
	BuildingGameObj::Import_Occasional (packet);

	bool launching = false;
	packet.Get (launching);

	//
	//	Only act on a change.  The animation manager is told to run to the end
	//	or back to the start, so telling it the same thing every update would be
	//	harmless but pointless -- and a client that joins mid-launch still gets
	//	the animation started here.
	//
	if (launching != Launching) {
		Launching	= launching;
		LaunchTimer	= launching ? LAUNCH_DURATION : 0;
		Play_Launch_Animation (launching);
	}

	return ;
}
