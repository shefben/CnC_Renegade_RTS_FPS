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
 *                     $Archive:: /Commando/Code/Combat/airstripgameobj.cpp                                                                                                                                      $Author:: Patrick                                                     $*
 *                                                                                             *
 *                     $Modtime:: 2/24/02 2:06p                                               $*
 *                                                                                             *
 *                    $Revision:: 19                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "airstripgameobj.h"
#include "basecontroller.h"
#include "vehicle.h"
#include "wwhack.h"
#include "simpledefinitionfactory.h"
#include "persistfactory.h"
#include "definitionmgr.h"
#include "combatchunkid.h"
#include "debug.h"
#include "scriptzone.h"
#include "wwprofile.h"
#include "basecontroller.h"
#include "combatchunkid.h"
#include "objlibrary.h"
#include "cinematicgameobj.h"
#include "combat.h"
#include "gameobjmanager.h"
#include "phys.h"


////////////////////////////////////////////////////////////////
//	Hacks
////////////////////////////////////////////////////////////////
DECLARE_FORCE_LINK (AirStrip)

////////////////////////////////////////////////////////////////
//	Editable and persist factories
////////////////////////////////////////////////////////////////
SimplePersistFactoryClass	<AirStripGameObjDef,	CHUNKID_GAME_OBJECT_DEF_AIRSTRIP>					_AirStripGameObjDefPersistFactory;
SimplePersistFactoryClass	<AirStripGameObj,		CHUNKID_GAME_OBJECT_AIRSTRIP>							_AirStripGameObjPersistFactory;
DECLARE_DEFINITION_FACTORY (AirStripGameObjDef,	CLASSID_GAME_OBJECT_DEF_AIRSTRIP, "Airstrip")	_AirStripGameObjDefDefFactory;


////////////////////////////////////////////////////////////////
//	Constants
////////////////////////////////////////////////////////////////
static const float UNITIALIZED_TIMER	= -100.0F;
static const float VEHICLE_LOCK_TIME	= 30.0f;

////////////////////////////////////////////////////////////////
//	Save/Load constants
////////////////////////////////////////////////////////////////
enum
{
	CHUNKID_DEF_PARENT										=	0x02200638,
	CHUNKID_DEF_VARIABLES,

	MICROCHUNKID_DEF_CINEMATIC_DEFID						= 1,
	MICROCHUNKID_DEF_CINEMATIC_LENGTH_TO_DROPOFF,
	MICROCHUNKID_DEF_CINEMATIC_SLOT_INDEX,
	MICROCHUNKID_DEF_DISPLAY_VEHICLE_TIME,
	MICROCHUNKID_DEF_LANDING_POSITION_DEFID,
	MICROCHUNKID_DEF_LANDING_POSITION_DISTANCE,
	MICROCHUNKID_DEF_DELIVERED_COLLISION_GROUP,
	MICROCHUNKID_DEF_DEFAULT_ENGINE_ENABLE
};

enum
{
	CHUNKID_PARENT								=	0x0219043,
	CHUNKID_VARIABLES,

	MICROCHUNKID_UNUSED						= 1,
};


////////////////////////////////////////////////////////////////
//
//	AirStripGameObjDef
//
////////////////////////////////////////////////////////////////
AirStripGameObjDef::AirStripGameObjDef (void)	:
	CinematicLengthToDropOff (0),
	CinematicLengthToVehicleDisplay (0),
	CinematicDefID (0),
	CinematicSlotIndex (0),
	LandingPositionDefID (0),
	LandingPositionDistance (0),
	DeliveredCollisionGroup (-1),
	DefaultEngineEnable (true)
{
	//
	//	Editable support
	//
	EDITABLE_PARAM (AirStripGameObjDef, ParameterClass::TYPE_FLOAT,		CinematicLengthToDropOff);
	EDITABLE_PARAM (AirStripGameObjDef, ParameterClass::TYPE_INT,			CinematicSlotIndex);
	EDITABLE_PARAM (AirStripGameObjDef, ParameterClass::TYPE_FLOAT,		CinematicLengthToVehicleDisplay);
	EDITABLE_PARAM (AirStripGameObjDef, ParameterClass::TYPE_FLOAT,		LandingPositionDistance);
	EDITABLE_PARAM (AirStripGameObjDef, ParameterClass::TYPE_BOOL,		DefaultEngineEnable);

	#ifdef PARAM_EDITING_ON
		GenericDefParameterClass *param = new GenericDefParameterClass (&CinematicDefID);
		param->Set_Class_ID (CLASSID_GAME_OBJECT_DEF_SIMPLE);
		param->Set_Name ("Drop-Off Cinematic");
		GENERIC_EDITABLE_PARAM (AirStripGameObjDef, param)

		GenericDefParameterClass *pad_param = new GenericDefParameterClass (&LandingPositionDefID);
		pad_param->Set_Class_ID (CLASSID_GAME_OBJECT_DEF_SIMPLE);
		pad_param->Set_Name ("Landing Position");
		GENERIC_EDITABLE_PARAM (AirStripGameObjDef, pad_param)

		//
		//	The designer picks the group by name rather than by number, so
		//	the list has to come from the same table the scene is built from.
		//
		EnumParameterClass *group_param = new EnumParameterClass (&DeliveredCollisionGroup);
		group_param->Set_Name ("Delivered Collision Group");
		group_param->Add_Value ("Leave Unchanged", -1);
		for (int group = 0; group < COLLISION_GROUP_COUNT; group ++) {
			group_param->Add_Value (::Get_Collision_Group_Name ((Collision_Group_Type)group), group);
		}
		GENERIC_EDITABLE_PARAM (AirStripGameObjDef, group_param)
	#endif //PARAM_EDITING_ON

	return ;
}


////////////////////////////////////////////////////////////////
//
//	~AirStripGameObjDef
//
////////////////////////////////////////////////////////////////
AirStripGameObjDef::~AirStripGameObjDef (void)
{
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Get_Class_ID
//
////////////////////////////////////////////////////////////////
uint32
AirStripGameObjDef::Get_Class_ID (void) const
{
	return CLASSID_GAME_OBJECT_DEF_AIRSTRIP;
}


////////////////////////////////////////////////////////////////
//
//	Create
//
////////////////////////////////////////////////////////////////
PersistClass *
AirStripGameObjDef::Create (void) const
{
	AirStripGameObj *building = new AirStripGameObj;
	building->Init (*this);

	return building;
}


////////////////////////////////////////////////////////////////
//
//	Create
//
////////////////////////////////////////////////////////////////
bool
AirStripGameObjDef::Save (ChunkSaveClass &csave)
{
	csave.Begin_Chunk (CHUNKID_DEF_PARENT);
		VehicleFactoryGameObjDef::Save (csave);
	csave.End_Chunk ();

	csave.Begin_Chunk (CHUNKID_DEF_VARIABLES);

		WRITE_MICRO_CHUNK (csave, MICROCHUNKID_DEF_CINEMATIC_LENGTH_TO_DROPOFF,	CinematicLengthToDropOff);
		WRITE_MICRO_CHUNK (csave, MICROCHUNKID_DEF_CINEMATIC_DEFID,					CinematicDefID);
		WRITE_MICRO_CHUNK (csave, MICROCHUNKID_DEF_CINEMATIC_SLOT_INDEX,			CinematicSlotIndex);
		WRITE_MICRO_CHUNK (csave, MICROCHUNKID_DEF_DISPLAY_VEHICLE_TIME,			CinematicLengthToVehicleDisplay);
		WRITE_MICRO_CHUNK (csave, MICROCHUNKID_DEF_LANDING_POSITION_DEFID,		LandingPositionDefID);
		WRITE_MICRO_CHUNK (csave, MICROCHUNKID_DEF_LANDING_POSITION_DISTANCE,	LandingPositionDistance);
		WRITE_MICRO_CHUNK (csave, MICROCHUNKID_DEF_DELIVERED_COLLISION_GROUP,	DeliveredCollisionGroup);
		WRITE_MICRO_CHUNK (csave, MICROCHUNKID_DEF_DEFAULT_ENGINE_ENABLE,		DefaultEngineEnable);

	csave.End_Chunk ();

	return true;
}


////////////////////////////////////////////////////////////////
//
//	Load
//
////////////////////////////////////////////////////////////////
bool
AirStripGameObjDef::Load (ChunkLoadClass &cload)
{
	while (cload.Open_Chunk ())
	{
		switch (cload.Cur_Chunk_ID ())
		{
			case CHUNKID_DEF_PARENT:
				VehicleFactoryGameObjDef::Load (cload);
				break;

			case CHUNKID_DEF_VARIABLES:
				Load_Variables (cload);
				break;

			default:
				Debug_Say (("Unrecognized AirStrip Def chunkID\n"));
				break;

		}
		cload.Close_Chunk ();
	}

	return true;
}


////////////////////////////////////////////////////////////////
//
//	Load_Variables
//
////////////////////////////////////////////////////////////////
void
AirStripGameObjDef::Load_Variables (ChunkLoadClass &cload)
{
	while (cload.Open_Micro_Chunk ()) {

		switch (cload.Cur_Micro_Chunk_ID ())
		{
			READ_MICRO_CHUNK (cload, MICROCHUNKID_DEF_CINEMATIC_LENGTH_TO_DROPOFF,	CinematicLengthToDropOff);
			READ_MICRO_CHUNK (cload, MICROCHUNKID_DEF_CINEMATIC_DEFID,					CinematicDefID);
			READ_MICRO_CHUNK (cload, MICROCHUNKID_DEF_CINEMATIC_SLOT_INDEX,			CinematicSlotIndex);
			READ_MICRO_CHUNK (cload, MICROCHUNKID_DEF_DISPLAY_VEHICLE_TIME,			CinematicLengthToVehicleDisplay);
			READ_MICRO_CHUNK (cload, MICROCHUNKID_DEF_LANDING_POSITION_DEFID,		LandingPositionDefID);
			READ_MICRO_CHUNK (cload, MICROCHUNKID_DEF_LANDING_POSITION_DISTANCE,	LandingPositionDistance);
			READ_MICRO_CHUNK (cload, MICROCHUNKID_DEF_DELIVERED_COLLISION_GROUP,	DeliveredCollisionGroup);
			READ_MICRO_CHUNK (cload, MICROCHUNKID_DEF_DEFAULT_ENGINE_ENABLE,		DefaultEngineEnable);

			default:
				Debug_Say (("Unrecognized AirStrip Def Variable chunkID\n"));
				break;
		}

		cload.Close_Micro_Chunk();
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Get_Factory
//
////////////////////////////////////////////////////////////////
const PersistFactoryClass &
AirStripGameObjDef::Get_Factory (void) const
{
	return _AirStripGameObjDefPersistFactory;
}


////////////////////////////////////////////////////////////////
//
//	AirStripGameObj
//
////////////////////////////////////////////////////////////////
AirStripGameObj::AirStripGameObj (void)	:
	CinematicStartTimer (0),
	ClearDropoffZoneTimer (UNITIALIZED_TIMER),
	DisplayVehicleTimer (UNITIALIZED_TIMER),
	IsCinematicStarted (false),
	CinematicObject (nullptr),
	CurrentLandingPosition (-1),
	DeliveryTM (1)
{
	return ;
}


////////////////////////////////////////////////////////////////
//
//	~AirStripGameObj
//
////////////////////////////////////////////////////////////////
AirStripGameObj::~AirStripGameObj (void)
{
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Get_Factory
//
////////////////////////////////////////////////////////////////
const PersistFactoryClass &
AirStripGameObj::Get_Factory (void) const
{
	return _AirStripGameObjPersistFactory;
}


////////////////////////////////////////////////////////////////
//
//	Init
//
////////////////////////////////////////////////////////////////
void AirStripGameObj::Init( void )
{
	Init( Get_Definition() );
}

////////////////////////////////////////////////////////////////
//
//	Init
//
////////////////////////////////////////////////////////////////
void
AirStripGameObj::Init (const AirStripGameObjDef &definition)
{
	VehicleFactoryGameObj::Init (definition);
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Get_Definition
//
////////////////////////////////////////////////////////////////
const AirStripGameObjDef &
AirStripGameObj::Get_Definition (void) const
{
	return (const AirStripGameObjDef &)BaseGameObj::Get_Definition ();
}


////////////////////////////////////////////////////////////////
//
//	Save
//
////////////////////////////////////////////////////////////////
bool
AirStripGameObj::Save (ChunkSaveClass &csave)
{
	csave.Begin_Chunk (CHUNKID_PARENT);
		VehicleFactoryGameObj::Save (csave);
	csave.End_Chunk ();

	csave.Begin_Chunk (CHUNKID_VARIABLES);
	csave.End_Chunk ();
	return true;
}


////////////////////////////////////////////////////////////////
//
//	Load
//
////////////////////////////////////////////////////////////////
bool
AirStripGameObj::Load (ChunkLoadClass &cload)
{
	while (cload.Open_Chunk ()) {
		switch (cload.Cur_Chunk_ID ()) {

			case CHUNKID_PARENT:
				VehicleFactoryGameObj::Load (cload);
				break;

			case CHUNKID_VARIABLES:
				Load_Variables (cload);
				break;

			default:
				Debug_Say (("Unrecognized AirStrip chunkID\n"));
				break;
		}

		cload.Close_Chunk();
	}

	return true;
}


////////////////////////////////////////////////////////////////
//
//	Load_Variables
//
////////////////////////////////////////////////////////////////
void
AirStripGameObj::Load_Variables (ChunkLoadClass &cload)
{
	while (cload.Open_Micro_Chunk ()) {

		/*switch (cload.Cur_Micro_Chunk_ID ())
		{
			default:
				Debug_Say (("Unrecognized AirStrip Variable chunkID\n"));
				break;
		}*/

		cload.Close_Micro_Chunk();
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	CnC_Initialize
//
////////////////////////////////////////////////////////////////
void
AirStripGameObj::CnC_Initialize (BaseControllerClass *base)
{
	VehicleFactoryGameObj::CnC_Initialize (base);

	//
	//	The pads are level geometry, so they are found once, here, rather
	//	than searched for on every delivery.
	//
	Collect_Landing_Positions ();
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Collect_Landing_Positions
//
////////////////////////////////////////////////////////////////
void
AirStripGameObj::Collect_Landing_Positions (void)
{
	LandingPositionIDs.Delete_All ();
	CurrentLandingPosition = -1;

	int pad_def_id = Get_Definition ().LandingPositionDefID;
	if (pad_def_id == 0) {
		return ;
	}

	Vector3 pos;
	Get_Position (&pos);

	float max_dist = Get_Definition ().LandingPositionDistance;
	float max_dist2 = max_dist * max_dist;

	//
	//	Several buildings can share a pad preset, so distance from this
	//	building is what decides which pads are ours.  A distance of zero
	//	means the level did not care to divide them up.
	//
	SLNode<BaseGameObj> *node = nullptr;
	for (node = GameObjManager::Get_Game_Obj_List ()->Head (); node != nullptr; node = node->Next ()) {

		PhysicalGameObj *pad = node->Data ()->As_PhysicalGameObj ();
		if (pad == nullptr || pad->Get_Definition ().Get_ID () != (uint32)pad_def_id) {
			continue;
		}

		Vector3 pad_pos;
		pad->Get_Position (&pad_pos);

		if (max_dist <= 0.0F || (pad_pos - pos).Length2 () <= max_dist2) {
			LandingPositionIDs.Add (pad->Get_ID ());
		}
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Choose_Landing_Position
//
////////////////////////////////////////////////////////////////
void
AirStripGameObj::Choose_Landing_Position (void)
{
	//
	//	No pads is the stock arrangement: one building, one drop point.
	//
	DeliveryTM = CreationTM;

	int pad_count = LandingPositionIDs.Count ();
	if (pad_count == 0) {
		return ;
	}

	//
	//	Take the pads in turn.  A pad that has been removed from the level
	//	since we collected it is skipped rather than waited for.
	//
	for (int attempt = 0; attempt < pad_count; attempt ++) {

		CurrentLandingPosition = (CurrentLandingPosition + 1) % pad_count;

		PhysicalGameObj *pad = GameObjManager::Find_PhysicalGameObj (LandingPositionIDs[CurrentLandingPosition]);
		if (pad != nullptr) {
			DeliveryTM = pad->Get_Transform ();
			break;
		}
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Think
//
////////////////////////////////////////////////////////////////
void
AirStripGameObj::Think (void)
{
	WWPROFILE ("AirStrip Think");

	//
	//	Begin the cinematic if necessary
	//
	if (IsDestroyed == false && GeneratingVehicleID != 0) {

		//
		//	Start the cinematic (if necessary)
		//
		if (IsCinematicStarted == false) {
			CinematicStartTimer -= TimeManager::Get_Frame_Seconds ();
			if (CinematicStartTimer <= 0) {
				Start_Cinematic ();
				ClearDropoffZoneTimer = 0.8f * Get_Definition().CinematicLengthToDropOff;
			}
		}
	}

	if ((GeneratingVehicleID != 0) && (IsCinematicStarted)) {

		//
		//	Check to see if its time to display the object or not
		//
		if (DisplayVehicleTimer > UNITIALIZED_TIMER) {
			DisplayVehicleTimer -= TimeManager::Get_Frame_Seconds ();
			if (DisplayVehicleTimer < 0) {

				//
				//	Display the vehicle
				//
				if (Vehicle != nullptr) {
					PhysicalGameObj *physical_obj = Vehicle.Get_Ptr ()->As_PhysicalGameObj ();
					physical_obj->Peek_Model ()->Set_Hidden (false);
					physical_obj->Set_Object_Dirty_Bit (NetworkObjectClass::BIT_RARE, true);

				}
				DisplayVehicleTimer = UNITIALIZED_TIMER;

				// Tell the vehicle to drive to one of the delivery points
				Deliver_Vehicle();
			}
		}

		//
		// Check if we need to clear the dropoff zone
		//
		if (ClearDropoffZoneTimer > UNITIALIZED_TIMER) {
			ClearDropoffZoneTimer -= TimeManager::Get_Frame_Seconds ();
			if (ClearDropoffZoneTimer < 0) {
				Destroy_Blocking_Objects ();
				ClearDropoffZoneTimer = UNITIALIZED_TIMER;
			}
		}
	}

	VehicleFactoryGameObj::Think ();
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Begin_Generation
//
////////////////////////////////////////////////////////////////
void
AirStripGameObj::Begin_Generation (void)
{
	CinematicStartTimer	= GenerationTime - Get_Definition ().CinematicLengthToDropOff;
	IsCinematicStarted	= false;

	//
	//	The cinematic, the drop and the pad clearing all read DeliveryTM, so
	//	the pad is chosen once here and not revisited mid-delivery.
	//
	Choose_Landing_Position ();
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Start_Cinematic
//
////////////////////////////////////////////////////////////////
void
AirStripGameObj::Start_Cinematic (void)
{
	if (CombatManager::I_Am_Server () == false) {
		return ;
	}

	//
	//	Create the vehicle
	//
	VehicleGameObj *vehicle = Create_Vehicle ();
	if (vehicle != nullptr) {

		//
		//	Hide the vehicle until later
		//
		if (vehicle->Peek_Model () != nullptr) {
			vehicle->Peek_Model ()->Set_Hidden (true);
			vehicle->Set_Object_Dirty_Bit (NetworkObjectClass::BIT_RARE, true);
		}

		//
		// Lock the vehicle
		//
		if (Purchaser != nullptr) {
			vehicle->Lock_Vehicle(Purchaser,VEHICLE_LOCK_TIME);
		}

		//
		//	An aircraft on its way down is not where it will end up, so a
		//	level can put it in a group that ignores whatever the pad sits
		//	on for the duration of the drop.
		//
		int collision_group = Get_Definition ().DeliveredCollisionGroup;
		if (collision_group >= 0 && vehicle->Peek_Physical_Object () != nullptr) {
			vehicle->Peek_Physical_Object ()->Set_Collision_Group ((unsigned char)collision_group);
		}

		//
		//	An aircraft delivered with its engine off has to be started before
		//	it can be flown off the pad.
		//
		vehicle->Enable_Engine (Get_Definition ().DefaultEngineEnable);

		//
		//	Create the cinematic controller
		//
		CinematicObject = ObjectLibraryManager::Create_Object (Get_Definition ().CinematicDefID);
		if (CinematicObject != nullptr) {
			CinematicObject->Start_Observers ();

			//
			//	Position the cinematic controller in the world
			//
			CinematicObject->Set_Transform (DeliveryTM);

			//
			//	Try to find the script parser so we can communicate with it
			//
			const GameObjObserverList &script_list = CinematicObject->Get_Observers ();
			for (int index = 0; index < script_list.Count (); index ++) {
				GameObjObserverClass *script = script_list[index];
				if (::stricmp (script->Get_Name (), "Test_Cinematic") == 0) {

					//
					//	Attach the vehicle to the given slot in the cinematic
					//
					int slot = 10000 + Get_Definition ().CinematicSlotIndex;
					script->Custom (this, slot, vehicle->Get_ID (), this);
					break;
				}
			}
		}
	}

	IsCinematicStarted = true;
	DisplayVehicleTimer = Get_Definition ().CinematicLengthToVehicleDisplay;
	return ;
}

