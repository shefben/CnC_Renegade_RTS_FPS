/***********************************************************************************************
 ***                            Confidential - Westwood Studios                              ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/constructionyardgameobj.cpp           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#include "constructionyardgameobj.h"
#include "basecontroller.h"
#include "combat.h"
#include "combatchunkid.h"
#include "debug.h"
#include "definitionmgr.h"
#include "gameobjmanager.h"
#include "persistfactory.h"
#include "simpledefinitionfactory.h"
#include "vehicle.h"
#include "wwhack.h"
#include "wwprofile.h"
#include "timemgr.h"


////////////////////////////////////////////////////////////////
//	Hacks
////////////////////////////////////////////////////////////////
DECLARE_FORCE_LINK (ConstructionYard)

////////////////////////////////////////////////////////////////
//	Editable and persist factories
////////////////////////////////////////////////////////////////
SimplePersistFactoryClass	<ConstructionYardGameObjDef,	CHUNKID_GAME_OBJECT_DEF_CONSTRUCTION_YARD>								_ConstructionYardGameObjDefPersistFactory;
SimplePersistFactoryClass	<ConstructionYardGameObj,		CHUNKID_GAME_OBJECT_CONSTRUCTION_YARD>									_ConstructionYardGameObjPersistFactory;
DECLARE_DEFINITION_FACTORY (ConstructionYardGameObjDef,	CLASSID_GAME_OBJECT_DEF_CONSTRUCTION_YARD, "Construction Yard")	_ConstructionYardGameObjDefDefFactory (false);

////////////////////////////////////////////////////////////////
//	Save/Load constants
////////////////////////////////////////////////////////////////
enum
{
	CHUNKID_DEF_PARENT						=	0x02240900,
	CHUNKID_DEF_VARIABLES,

	MICROCHUNKID_DEF_REPAIR_RATE			= 1,
	MICROCHUNKID_DEF_VEHICLE_REPAIR_RATE,
	MICROCHUNKID_DEF_REPAIR_AMOUNT,
	MICROCHUNKID_DEF_LOW_POWER_REPAIR_AMOUNT,
	MICROCHUNKID_DEF_REPAIR_WARHEAD,
};

enum
{
	CHUNKID_PARENT								=	0x02240902,
	CHUNKID_VARIABLES,

	MICROCHUNKID_REPAIR_TIMER				= 1,
	MICROCHUNKID_VEHICLE_REPAIR_TIMER,
	MICROCHUNKID_IS_SPY,
};


////////////////////////////////////////////////////////////////
//
//	ConstructionYardGameObjDef
//
////////////////////////////////////////////////////////////////
ConstructionYardGameObjDef::ConstructionYardGameObjDef (void)	:
	RepairRate (5.0F),
	VehicleRepairRate (5.0F),
	RepairAmount (25.0F),
	LowPowerRepairAmount (5.0F),
	RepairWarhead (0)
{
	EDITABLE_PARAM (ConstructionYardGameObjDef, ParameterClass::TYPE_FLOAT, RepairRate);
	EDITABLE_PARAM (ConstructionYardGameObjDef, ParameterClass::TYPE_FLOAT, VehicleRepairRate);
	EDITABLE_PARAM (ConstructionYardGameObjDef, ParameterClass::TYPE_FLOAT, RepairAmount);
	EDITABLE_PARAM (ConstructionYardGameObjDef, ParameterClass::TYPE_FLOAT, LowPowerRepairAmount);

#ifdef PARAM_EDITING_ON
	EnumParameterClass *param = new EnumParameterClass (&RepairWarhead);
	param->Set_Name ("Repair Warhead");
	for (int index = 0; index < ArmorWarheadManager::Get_Num_Warhead_Types (); index ++) {
		param->Add_Value (ArmorWarheadManager::Get_Warhead_Name (index), index);
	}
	GENERIC_EDITABLE_PARAM (ConstructionYardGameObjDef, param)
#endif

	return ;
}

ConstructionYardGameObjDef::~ConstructionYardGameObjDef (void)
{
	return ;
}

uint32
ConstructionYardGameObjDef::Get_Class_ID (void) const
{
	return CLASSID_GAME_OBJECT_DEF_CONSTRUCTION_YARD;
}

PersistClass *
ConstructionYardGameObjDef::Create (void) const
{
	ConstructionYardGameObj *building = new ConstructionYardGameObj;
	building->Init (*this);

	return building;
}

bool
ConstructionYardGameObjDef::Save (ChunkSaveClass &csave)
{
	csave.Begin_Chunk (CHUNKID_DEF_PARENT);
		BuildingGameObjDef::Save (csave);
	csave.End_Chunk ();

	csave.Begin_Chunk (CHUNKID_DEF_VARIABLES);
		WRITE_MICRO_CHUNK (csave, MICROCHUNKID_DEF_REPAIR_RATE, RepairRate);
		WRITE_MICRO_CHUNK (csave, MICROCHUNKID_DEF_VEHICLE_REPAIR_RATE, VehicleRepairRate);
		WRITE_MICRO_CHUNK (csave, MICROCHUNKID_DEF_REPAIR_AMOUNT, RepairAmount);
		WRITE_MICRO_CHUNK (csave, MICROCHUNKID_DEF_LOW_POWER_REPAIR_AMOUNT, LowPowerRepairAmount);
		WRITE_MICRO_CHUNK (csave, MICROCHUNKID_DEF_REPAIR_WARHEAD, RepairWarhead);
	csave.End_Chunk ();

	return true;
}

bool
ConstructionYardGameObjDef::Load (ChunkLoadClass &cload)
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
				Debug_Say (("Unrecognized Construction Yard Def chunkID\n"));
				break;
		}

		cload.Close_Chunk ();
	}

	return true;
}

void
ConstructionYardGameObjDef::Load_Variables (ChunkLoadClass &cload)
{
	while (cload.Open_Micro_Chunk ()) {
		switch (cload.Cur_Micro_Chunk_ID ()) {

			READ_MICRO_CHUNK (cload, MICROCHUNKID_DEF_REPAIR_RATE, RepairRate);
			READ_MICRO_CHUNK (cload, MICROCHUNKID_DEF_VEHICLE_REPAIR_RATE, VehicleRepairRate);
			READ_MICRO_CHUNK (cload, MICROCHUNKID_DEF_REPAIR_AMOUNT, RepairAmount);
			READ_MICRO_CHUNK (cload, MICROCHUNKID_DEF_LOW_POWER_REPAIR_AMOUNT, LowPowerRepairAmount);
			READ_MICRO_CHUNK (cload, MICROCHUNKID_DEF_REPAIR_WARHEAD, RepairWarhead);
		}

		cload.Close_Micro_Chunk ();
	}

	return ;
}

const PersistFactoryClass &
ConstructionYardGameObjDef::Get_Factory (void) const
{
	return _ConstructionYardGameObjDefPersistFactory;
}


////////////////////////////////////////////////////////////////
//
//	ConstructionYardGameObj
//
////////////////////////////////////////////////////////////////
ConstructionYardGameObj::ConstructionYardGameObj (void)	:
	RepairTimer (0),
	VehicleRepairTimer (0),
	IsSpy (false)
{
	return ;
}

ConstructionYardGameObj::~ConstructionYardGameObj (void)
{
	return ;
}

const PersistFactoryClass &
ConstructionYardGameObj::Get_Factory (void) const
{
	return _ConstructionYardGameObjPersistFactory;
}

void
ConstructionYardGameObj::Init (void)
{
	Init (Get_Definition ());
	return ;
}

void
ConstructionYardGameObj::Init (const ConstructionYardGameObjDef &definition)
{
	BuildingGameObj::Init (definition);

	RepairTimer				= definition.RepairRate;
	VehicleRepairTimer	= definition.VehicleRepairRate;

	return ;
}

const ConstructionYardGameObjDef &
ConstructionYardGameObj::Get_Definition (void) const
{
	return (const ConstructionYardGameObjDef &)BaseGameObj::Get_Definition ();
}

bool
ConstructionYardGameObj::Save (ChunkSaveClass &csave)
{
	csave.Begin_Chunk (CHUNKID_PARENT);
		BuildingGameObj::Save (csave);
	csave.End_Chunk ();

	csave.Begin_Chunk (CHUNKID_VARIABLES);
		WRITE_MICRO_CHUNK (csave, MICROCHUNKID_REPAIR_TIMER, RepairTimer);
		WRITE_MICRO_CHUNK (csave, MICROCHUNKID_VEHICLE_REPAIR_TIMER, VehicleRepairTimer);
		WRITE_MICRO_CHUNK (csave, MICROCHUNKID_IS_SPY, IsSpy);
	csave.End_Chunk ();

	return true;
}

bool
ConstructionYardGameObj::Load (ChunkLoadClass &cload)
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
				Debug_Say (("Unrecognized Construction Yard chunkID\n"));
				break;
		}

		cload.Close_Chunk ();
	}

	return true;
}

void
ConstructionYardGameObj::Load_Variables (ChunkLoadClass &cload)
{
	while (cload.Open_Micro_Chunk ()) {
		switch (cload.Cur_Micro_Chunk_ID ()) {

			READ_MICRO_CHUNK (cload, MICROCHUNKID_REPAIR_TIMER, RepairTimer);
			READ_MICRO_CHUNK (cload, MICROCHUNKID_VEHICLE_REPAIR_TIMER, VehicleRepairTimer);
			READ_MICRO_CHUNK (cload, MICROCHUNKID_IS_SPY, IsSpy);
		}

		cload.Close_Micro_Chunk ();
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Think
//
//	The repairs are a server-side change to health, which replicates on its
//	own, so a client has nothing to do here.
//
////////////////////////////////////////////////////////////////
void
ConstructionYardGameObj::Think (void)
{
	WWPROFILE ("ConstructionYard Think");

	BuildingGameObj::Think ();

	if (CombatManager::I_Am_Server () == false) {
		return ;
	}

	//
	//	A destroyed yard repairs nothing, and neither does one with a spy
	//	standing in it.
	//
	if (Is_Destroyed () || IsSpy) {
		return ;
	}

	//
	//	Losing base power slows the yard down rather than stopping it.
	//
	float amount = Get_Definition ().RepairAmount;
	if (BaseController != nullptr && BaseController->Is_Base_Powered () == false) {
		amount = Get_Definition ().LowPowerRepairAmount;
	}

	float frame_time = TimeManager::Get_Frame_Seconds ();

	if (Get_Definition ().RepairRate > 0) {
		RepairTimer -= frame_time;
		if (RepairTimer <= 0) {
			RepairTimer = Get_Definition ().RepairRate;
			Repair_Buildings (amount);
		}
	}

	if (Get_Definition ().VehicleRepairRate > 0) {
		VehicleRepairTimer -= frame_time;
		if (VehicleRepairTimer <= 0) {
			VehicleRepairTimer = Get_Definition ().VehicleRepairRate;
			Repair_Vehicles (amount);
		}
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Repair_Buildings
//
//	Every building the base controller knows about except this one -- a yard
//	that healed itself would be unkillable while it still had a base.
//
////////////////////////////////////////////////////////////////
void
ConstructionYardGameObj::Repair_Buildings (float amount)
{
	if (BaseController == nullptr) {
		return ;
	}

	//
	//	Negative damage is a repair, which is how the warhead the definition
	//	names gets a say in what each building actually gets back.
	//
	OffenseObjectClass offense (-amount, (WarheadType)Get_Definition ().RepairWarhead, nullptr);

	for (int index = 0; index < BaseController->Get_Building_Count (); index ++) {

		BuildingGameObj *building = BaseController->Peek_Building (index);

		if (building == nullptr || building == this || building->Is_Destroyed ()) {
			continue;
		}

		DefenseObjectClass *defense = building->Get_Defense_Object ();
		if (defense != nullptr && defense->Get_Health () < defense->Get_Health_Max ()) {
			building->Apply_Damage (offense);
		}
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Repair_Vehicles
//
////////////////////////////////////////////////////////////////
void
ConstructionYardGameObj::Repair_Vehicles (float amount)
{
	int my_team = Get_Player_Type ();

	OffenseObjectClass offense (-amount, (WarheadType)Get_Definition ().RepairWarhead, nullptr);

	for (	SLNode<BaseGameObj> *node = GameObjManager::Get_Game_Obj_List ()->Head ();
			node != nullptr;
			node = node->Next ())
	{
		BaseGameObj *obj = node->Data ();
		if (obj == nullptr) {
			continue;
		}

		PhysicalGameObj *phys_obj = obj->As_PhysicalGameObj ();
		if (phys_obj == nullptr) {
			continue;
		}

		VehicleGameObj *vehicle = phys_obj->As_VehicleGameObj ();
		if (vehicle == nullptr || vehicle->Get_Player_Type () != my_team) {
			continue;
		}

		DefenseObjectClass *defense = vehicle->Get_Defense_Object ();
		if (defense != nullptr && defense->Get_Health () < defense->Get_Health_Max ()) {
			vehicle->Apply_Damage (offense);
		}
	}

	return ;
}
