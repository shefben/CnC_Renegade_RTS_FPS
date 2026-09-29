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
 *                     $Archive:: /Commando/Code/Combat/building.cpp                          $*
 *                                                                                             *
 *                      $Author:: Greg_h                                                      $*
 *                                                                                             *
 *                     $Modtime:: 6/20/02 3:09p                                               $*
 *                                                                                             *
 *                    $Revision:: 68                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "building.h"
#include "simpledefinitionfactory.h"
#include "persistfactory.h"
#include "combatchunkid.h"
#include "compositeattachment.h"
#include "debrissystem.h"
#include "rtstelemetryservice.h"
#include "teampowergridservice.h"
#include "commanderproduction.h"
#include "worldshadowmanager.h"
#include "debug.h"
#include "wwphysids.h"
#include "meshmdl.h"
#include "gameobjmanager.h"
#include "light.h"
#include "worldlightmanager.h"
#include "WWAudio.h"
#include "AudibleSound.h"
#include "Sound3D.h"
#include "translateobj.h"
#include "translatedb.h"
#include "combat.h"
#include "messagewindow.h"
#include "buildingmonitor.h"
#include "basecontroller.h"
#include "bitpackids.h"
#include "soldier.h"
#include "playerdata.h"
#include "encyclopediamgr.h"
#include "apppackettypes.h"
#include "wwprofile.h"
#include "coltest.h"
#include "buildingconstruction.h"
#include "buildingdescriptormanager.h"
#include "runtimedoorservice.h"
#include "pscene.h"
#include "rendobj.h"
#include "staticphys.h"
#include "assetmgr.h"
#include "wwmath.h"
#include <algorithm>


/////////////////////////////////////////////////////////////////////////////
//	Namespaces
/////////////////////////////////////////////////////////////////////////////
using namespace BuildingConstants;

//	Forward declaration: defined with the rest of the runtime-construction path further down
//	this file (Q-016), but Export_Rare/Import_Rare need it for the component-assembly door
//	state block and are defined earlier in the file than that section.
static const BuildingModelDescriptor * Peek_Runtime_Descriptor (const StringClass &name);


/////////////////////////////////////////////////////////////////////////////
//
//	Constants
//
/////////////////////////////////////////////////////////////////////////////
static const char *	BULDING_TYPE_NAMES[TYPE_COUNT + 1] =
{
	"<None>",
	"Power Plant",
	"Soldier Factory",
	"Vehicle Factory",
	"Refinery",
	"Com Center",
	"Repair Bay",
	"Shrine",
	"Helipad",
	"Conyard",
	"Base Defense",
};


static const char *	BULDING_TEAM_NAMES[BASE_COUNT] =
{
	"GDI",
	"NOD"
};

//
// Class statics
//
bool	BuildingGameObj::CanRepairBuildings	= true;


/************************************************************************************************
**
** BuildingGameObjDef
**
************************************************************************************************/
SimplePersistFactoryClass<BuildingGameObjDef, CHUNKID_GAME_OBJECT_DEF_BUILDING>	_BuildingGameObjDefPersistFactory;

DECLARE_DEFINITION_FACTORY(BuildingGameObjDef, CLASSID_GAME_OBJECT_DEF_BUILDING, "<Generic Building>") _BuildingGameObjDefDefFactory;


/////////////////////////////////////////////////////////////////////////////
//
//	BuildingGameObjDef
//
/////////////////////////////////////////////////////////////////////////////
BuildingGameObjDef::BuildingGameObjDef (void)	:
	MCTSkin(0),
	Type(TYPE_NONE),
	GDIDamageReportID(0),
	NodDamageReportID(0),
	GDIDestroyReportID(0),
	NodDestroyReportID(0),
	HideTeamBattlefieldInformation(false)
{
	EDITABLE_PARAM( BuildingGameObjDef, ParameterClass::TYPE_STRING,	MeshPrefix );
	EDITABLE_PARAM( BuildingGameObjDef, ParameterClass::TYPE_STRING,	DescriptorName );
	EDITABLE_PARAM( BuildingGameObjDef, ParameterClass::TYPE_BOOL,	HideTeamBattlefieldInformation );

#ifdef PARAM_EDITING_ON
	int skin_type_counter;
	EnumParameterClass * mct_skin_param = new EnumParameterClass( (int*)&MCTSkin );
	mct_skin_param->Set_Name("MCTSkin");
	for ( skin_type_counter = 0; skin_type_counter < ArmorWarheadManager::Get_Num_Armor_Types(); skin_type_counter++ ) {
		mct_skin_param->Add_Value(ArmorWarheadManager::Get_Armor_Name(skin_type_counter), skin_type_counter);
	}
	GENERIC_EDITABLE_PARAM(BuildingGameObjDef,mct_skin_param);
#endif

//
//	Configure the building type parameter
//
#ifdef PARAM_EDITING_ON
	EnumParameterClass *building_type_param = new EnumParameterClass( (int*)&Type );
	building_type_param->Set_Name("Building Type");
	for ( int index = TYPE_NONE; index < TYPE_COUNT; index++ ) {
		building_type_param->Add_Value(BULDING_TYPE_NAMES[index+1], index);
	}
	GENERIC_EDITABLE_PARAM(BuildingGameObjDef,building_type_param);
#endif

	EDITABLE_PARAM(BuildingGameObjDef, ParameterClass::TYPE_STRINGSDB_ID, GDIDamageReportID);
	EDITABLE_PARAM(BuildingGameObjDef, ParameterClass::TYPE_STRINGSDB_ID, NodDamageReportID);
	EDITABLE_PARAM(BuildingGameObjDef, ParameterClass::TYPE_STRINGSDB_ID, GDIDestroyReportID);
	EDITABLE_PARAM(BuildingGameObjDef, ParameterClass::TYPE_STRINGSDB_ID, NodDestroyReportID);
}


/////////////////////////////////////////////////////////////////////////////
//
//	Get_Class_ID
//
/////////////////////////////////////////////////////////////////////////////
uint32
BuildingGameObjDef::Get_Class_ID (void) const
{
	return CLASSID_GAME_OBJECT_DEF_BUILDING;
}


/////////////////////////////////////////////////////////////////////////////
//
//	Create
//
/////////////////////////////////////////////////////////////////////////////
PersistClass *
BuildingGameObjDef::Create (void) const
{
	BuildingGameObj * obj = new BuildingGameObj;
	obj->Init( *this );
	return obj;
}


/////////////////////////////////////////////////////////////////////////////
//	Save/Load constants
/////////////////////////////////////////////////////////////////////////////
enum	{
	CHUNKID_DEF_PARENT							=	207011030,
	CHUNKID_DEF_VARIABLES,

	MICROCHUNKID_DEF_MESHPREFIX				= 1,
	MICROCHUNKID_DEF_MCTSKIN,
	MICROCHUNKID_DEF_BUILDING_TYPE,
	LEGACY_MICROCHUNKID_DEF_BUILDING_TEAM,
	MICROCHUNKID_DEF_GDI_DAMAGE_REPORT_ID,
	MICROCHUNKID_DEF_NOD_DAMAGE_REPORT_ID,
	MICROCHUNKID_DEF_GDI_DESTROY_REPORT_ID,
	MICROCHUNKID_DEF_NOD_DESTROY_REPORT_ID,
	MICROCHUNKID_DEF_HIDE_TEAM_BATTLEFIELD_INFORMATION,
	MICROCHUNKID_DEF_DESCRIPTOR_NAME,
};


/////////////////////////////////////////////////////////////////////////////
//
//	Save
//
/////////////////////////////////////////////////////////////////////////////
bool
BuildingGameObjDef::Save (ChunkSaveClass &csave)
{
	csave.Begin_Chunk( CHUNKID_DEF_PARENT );
		DamageableGameObjDef::Save( csave );
	csave.End_Chunk();

	csave.Begin_Chunk( CHUNKID_DEF_VARIABLES );
		WRITE_MICRO_CHUNK_WWSTRING( csave, MICROCHUNKID_DEF_MESHPREFIX, MeshPrefix );
		WRITE_MICRO_CHUNK( csave, MICROCHUNKID_DEF_MCTSKIN, MCTSkin );
		WRITE_MICRO_CHUNK( csave, MICROCHUNKID_DEF_BUILDING_TYPE, Type );
		WRITE_MICRO_CHUNK( csave, MICROCHUNKID_DEF_GDI_DAMAGE_REPORT_ID, GDIDamageReportID );
		WRITE_MICRO_CHUNK( csave, MICROCHUNKID_DEF_NOD_DAMAGE_REPORT_ID, NodDamageReportID );
		WRITE_MICRO_CHUNK( csave, MICROCHUNKID_DEF_GDI_DESTROY_REPORT_ID, GDIDestroyReportID );
		WRITE_MICRO_CHUNK( csave, MICROCHUNKID_DEF_NOD_DESTROY_REPORT_ID, NodDestroyReportID );
		WRITE_MICRO_CHUNK( csave, MICROCHUNKID_DEF_HIDE_TEAM_BATTLEFIELD_INFORMATION, HideTeamBattlefieldInformation );
		WRITE_MICRO_CHUNK_WWSTRING( csave, MICROCHUNKID_DEF_DESCRIPTOR_NAME, DescriptorName );
	csave.End_Chunk();

	return true;
}


/////////////////////////////////////////////////////////////////////////////
//
//	Load
//
/////////////////////////////////////////////////////////////////////////////
bool
BuildingGameObjDef::Load (ChunkLoadClass &cload)
{
	int legacy_team = -1;

	while (cload.Open_Chunk()) {
		switch(cload.Cur_Chunk_ID()) {

			case CHUNKID_DEF_PARENT:
				DamageableGameObjDef::Load( cload );
				break;

			case CHUNKID_DEF_VARIABLES:
				while (cload.Open_Micro_Chunk()) {
					switch(cload.Cur_Micro_Chunk_ID()) {
						READ_MICRO_CHUNK_WWSTRING( cload, MICROCHUNKID_DEF_MESHPREFIX, MeshPrefix );
						READ_MICRO_CHUNK( cload, MICROCHUNKID_DEF_MCTSKIN, MCTSkin );
						READ_MICRO_CHUNK( cload, MICROCHUNKID_DEF_BUILDING_TYPE, Type );
						READ_MICRO_CHUNK( cload, LEGACY_MICROCHUNKID_DEF_BUILDING_TEAM, legacy_team );
						READ_MICRO_CHUNK( cload, MICROCHUNKID_DEF_GDI_DAMAGE_REPORT_ID, GDIDamageReportID );
						READ_MICRO_CHUNK( cload, MICROCHUNKID_DEF_NOD_DAMAGE_REPORT_ID, NodDamageReportID );
						READ_MICRO_CHUNK( cload, MICROCHUNKID_DEF_GDI_DESTROY_REPORT_ID, GDIDestroyReportID );
						READ_MICRO_CHUNK( cload, MICROCHUNKID_DEF_NOD_DESTROY_REPORT_ID, NodDestroyReportID );
						READ_MICRO_CHUNK( cload, MICROCHUNKID_DEF_HIDE_TEAM_BATTLEFIELD_INFORMATION, HideTeamBattlefieldInformation );
						READ_MICRO_CHUNK_WWSTRING( cload, MICROCHUNKID_DEF_DESCRIPTOR_NAME, DescriptorName );

						default:
							Debug_Say(("Unhandled Micro Chunk:%d File:%s Line:%d\r\n",cload.Cur_Micro_Chunk_ID(),__FILE__,__LINE__));
							break;

					}
					cload.Close_Micro_Chunk();
				}
				break;

			default:
				Debug_Say(("Unhandled Chunk:%d File:%s Line:%d\r\n",cload.Cur_Chunk_ID(),__FILE__,__LINE__));
				break;

		}
		cload.Close_Chunk();
	}

	if ( legacy_team != -1 ) {
		DefaultPlayerType = PLAYERTYPE_GDI;
		if ( legacy_team == BuildingConstants::LEGACY_TEAM_NOD ) {
			DefaultPlayerType = PLAYERTYPE_NOD;
		}
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
//
//	Get_Factory
//
/////////////////////////////////////////////////////////////////////////////
const PersistFactoryClass &
BuildingGameObjDef::Get_Factory (void) const
{
	return _BuildingGameObjDefPersistFactory;
}


int BuildingGameObjDef::Get_Damage_Report(int team) const
{
	if (PLAYERTYPE_GDI == team) {
		return GDIDamageReportID;
	} else if (PLAYERTYPE_NOD == team) {
		return NodDamageReportID;
	}

	return 0;
}


int BuildingGameObjDef::Get_Destroy_Report(int team) const
{
	if (PLAYERTYPE_GDI == team) {
		return GDIDestroyReportID;
	} else if (PLAYERTYPE_NOD == team) {
		return NodDestroyReportID;
	}

	return 0;
}



/************************************************************************************************
**
** BuildingGameObj
**
************************************************************************************************/
SimplePersistFactoryClass<BuildingGameObj, CHUNKID_GAME_OBJECT_BUILDING>	_BuildingGameObjPersistFactory;


/////////////////////////////////////////////////////////////////////////////
//
//	Get_Factory
//
/////////////////////////////////////////////////////////////////////////////
const PersistFactoryClass &
BuildingGameObj::Get_Factory (void) const
{
	return _BuildingGameObjPersistFactory;
}


/////////////////////////////////////////////////////////////////////////////
//
//	BuildingGameObj
//
/////////////////////////////////////////////////////////////////////////////
BuildingGameObj::BuildingGameObj (void)	:
	Position (0, 0, 0),
	IsPowerOn (true),
	CurrentState (-1),
	AnnouncementSphere (Vector3 (0, 0, 0), 1.0F),
	CollectionSphere (Vector3 (0, 0, 0), 50.0F),
	CurrentAnnouncement (nullptr),
	IsDestroyed (false),
	IsSpyDisabled (false),
	BoundingBox (Vector3 (0, 0, 0), Vector3 (0, 0, 0)),
	BuildingMonitor (nullptr),
	BaseController (nullptr),
	AttachmentController (nullptr),
	IsRuntimeConstructed (false),
	RuntimeModelHidden (false),
	RuntimeDescriptorName (""),
	RuntimeModelName (""),
	RuntimeTransform (1),
	RuntimeDamageState (RUNTIME_DAMAGE_STATE_UNSET),
	RuntimeAssemblyDestroyed (false),
	LoadedRuntimeDescriptorName (""),
	LoadedRuntimeTransform (1)
{
	GameObjManager::Add_Building (this);
	Update_State ();
	Set_App_Packet_Type(APPPACKETTYPE_BUILDING);
}


/////////////////////////////////////////////////////////////////////////////
//
//	~BuildingGameObj
//
/////////////////////////////////////////////////////////////////////////////
BuildingGameObj::~BuildingGameObj (void)
{
	//
	//	Roadmap Section 40 -- drop this building's TeamPowerGridService registration (if any;
	//	a no-op if it was never registered).  Unconditional on client and server alike: it is
	//	bookkeeping, not an Enable_Power decision.
	//
	TeamPowerGridService::UnregisterBuilding (Get_ID ());

	Stop_Current_Announcement();

	//
	//	Free the building monitor (if we have one)
	//
	if (BuildingMonitor != nullptr) {
		Remove_Observer (BuildingMonitor);
		delete BuildingMonitor;
		BuildingMonitor = nullptr;
	}

	//
	//	Before Reset_Components, which drops this building's hold on the very models the
	//	attachments are hanging off.
	//
	if (AttachmentController != nullptr) {
		delete AttachmentController;		// its destructor detaches whatever is still attached
		AttachmentController = nullptr;
	}

	//
	//	Before Reset_Components, which would clear this building's hold on the runtime phys
	//	object without ever taking it back out of the scene.
	//
	Release_Runtime_Components ();

	GameObjManager::Remove_Building( this );
	Reset_Components();
	return ;
}


/////////////////////////////////////////////////////////////////////////////
//
//	Init
//
/////////////////////////////////////////////////////////////////////////////
void BuildingGameObj::Init( void )
{
	Init( Get_Definition() );
}


/////////////////////////////////////////////////////////////////////////////
//
//	Init
//
/////////////////////////////////////////////////////////////////////////////
void
BuildingGameObj::Init (const BuildingGameObjDef &definition)
{
	DamageableGameObj::Init (definition);
	return ;
}


/////////////////////////////////////////////////////////////////////////////
//
//	Get_Definition
//
/////////////////////////////////////////////////////////////////////////////
const BuildingGameObjDef &
BuildingGameObj::Get_Definition (void) const
{
	return (const BuildingGameObjDef &)BaseGameObj::Get_Definition();
}


/////////////////////////////////////////////////////////////////////////////
//	Save/Load constants
/////////////////////////////////////////////////////////////////////////////
enum	{
	CHUNKID_PARENT							=	207011120,
	CHUNKID_VARIABLES,

	MICROCHUNKID_POSITION				=	1,
	MICROCHUNKID_ISPOWERON,
	MICROCHUNKID_COLLECTION_SPHERE,
	MICROCHUNKID_IS_SPY_DISABLED,
	MICROCHUNKID_RUNTIME_DESCRIPTOR,
	MICROCHUNKID_RUNTIME_TRANSFORM,
};


/////////////////////////////////////////////////////////////////////////////
//
//	Save
//
/////////////////////////////////////////////////////////////////////////////
bool
BuildingGameObj::Save (ChunkSaveClass &csave)
{
	csave.Begin_Chunk (CHUNKID_PARENT);
		DamageableGameObj::Save (csave);
	csave.End_Chunk ();

	csave.Begin_Chunk (CHUNKID_VARIABLES);
		WRITE_MICRO_CHUNK (csave, MICROCHUNKID_POSITION,				Position);
		WRITE_MICRO_CHUNK (csave, MICROCHUNKID_ISPOWERON,				IsPowerOn);
		WRITE_MICRO_CHUNK (csave, MICROCHUNKID_COLLECTION_SPHERE,	CollectionSphere);
		WRITE_MICRO_CHUNK (csave, MICROCHUNKID_IS_SPY_DISABLED,	IsSpyDisabled);

		//
		//	A runtime building (Commander-built, or placed by a generated world's base layout)
		//	is a descriptor and a transform; without them a load would bring it back as a map
		//	building with nothing to collect.  A map-authored building writes neither.
		//
		if (IsRuntimeConstructed) {
			WRITE_MICRO_CHUNK_WWSTRING (csave, MICROCHUNKID_RUNTIME_DESCRIPTOR,	RuntimeDescriptorName);
			WRITE_MICRO_CHUNK (csave, MICROCHUNKID_RUNTIME_TRANSFORM,			RuntimeTransform);
		}
	csave.End_Chunk ();

	return true;
}


/////////////////////////////////////////////////////////////////////////////
//
//	Load
//
/////////////////////////////////////////////////////////////////////////////
bool
BuildingGameObj::Load (ChunkLoadClass &cload)
{
	while (cload.Open_Chunk ()) {
		switch (cload.Cur_Chunk_ID ()) {

			case CHUNKID_PARENT:
				DamageableGameObj::Load (cload);
				break;

			case CHUNKID_VARIABLES:
				while (cload.Open_Micro_Chunk ()) {
					switch (cload.Cur_Micro_Chunk_ID ()) {

						READ_MICRO_CHUNK (cload, MICROCHUNKID_POSITION,				Position);
						READ_MICRO_CHUNK (cload, MICROCHUNKID_ISPOWERON,			IsPowerOn);
						READ_MICRO_CHUNK (cload, MICROCHUNKID_COLLECTION_SPHERE,	CollectionSphere);
						READ_MICRO_CHUNK (cload, MICROCHUNKID_IS_SPY_DISABLED,	IsSpyDisabled);
						READ_MICRO_CHUNK_WWSTRING (cload, MICROCHUNKID_RUNTIME_DESCRIPTOR,	LoadedRuntimeDescriptorName);
						READ_MICRO_CHUNK (cload, MICROCHUNKID_RUNTIME_TRANSFORM,			LoadedRuntimeTransform);

						default:
							Debug_Say(("Unhandled Micro Chunk:%d File:%s Line:%d\r\n",cload.Cur_Micro_Chunk_ID(),__FILE__,__LINE__));
							break;

					}
					cload.Close_Micro_Chunk();
				}
				break;

			default:
				Debug_Say(("Unhandled Chunk:%d File:%s Line:%d\r\n",cload.Cur_Chunk_ID(),__FILE__,__LINE__));
				break;

		}

		cload.Close_Chunk ();
	}

	// Fix player_type (legacy)
	Set_Player_Type( Get_Definition().DefaultPlayerType );

	//
	//	Hack to get legacy code to work...
	//
	if (CollectionSphere.Center.X == 0 && CollectionSphere.Center.Y == 0 && CollectionSphere.Center.Z == 0) {
		CollectionSphere.Center = Position;
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
//
//	Enable_Power
//
/////////////////////////////////////////////////////////////////////////////
void
BuildingGameObj::Enable_Power (bool onoff)
{
	if ( IsPowerOn != onoff ) {
		// Notify the observers
		const GameObjObserverList & observer_list = Get_Observers();
		for( int index = 0; index < observer_list.Count(); index++ ) {
			observer_list[ index ]->Custom( this, CUSTOM_EVENT_BUILDING_POWER_CHANGED, onoff, nullptr );
		}
	}

	IsPowerOn = onoff;

	Update_Group_Lighting ();
	Update_State ();

	//
	//	Mark the object as dirty so it's state will be mirrored on the client
	//
	if (CombatManager::I_Am_Server ()) {
		Set_Object_Dirty_Bit (NetworkObjectClass::BIT_RARE, true);
	}

	return ;
}


/////////////////////////////////////////////////////////////////////////////
//
//	Update_Group_Lighting -- roadmap Section 52A, Phase 50.
//
//	The other half of lightphys.h's own "the building code uses the Group ID..." comment.
//	Runs on both client and server -- Enable_Power/On_Destroyed/On_Revived all already do --
//	so a light stays correct locally on either side without needing a network field of its
//	own; IsPowerOn and IsDestroyed already arrive replicated through the usual building state.
//
/////////////////////////////////////////////////////////////////////////////
void
BuildingGameObj::Update_Group_Lighting (void)
{
	bool should_glow = IsPowerOn && !IsDestroyed;
	WorldLightManager::Set_Group_Curve ((uintptr_t)Get_ID (),
		should_glow ? LightClass::LIGHT_CURVE_STEADY : LightClass::LIGHT_CURVE_FAIL);
}

void BuildingGameObj::Apply_Damage( const OffenseObjectClass & damager, float scale, int alternate_skin )
{
	/*
	** If we're already completely damaged, just return
	*/
	if ( DefenseObject.Get_Health() <= 0 ) {
		return;
	}

	// if gameplay not permitted, skip
	if ( !CombatManager::Is_Gameplay_Permitted() ) {
		return;
	}
	//
	// TSS102501
	//
	if ( !CanRepairBuildings && DefenseObject.Is_Repair(damager) ) {
		return;
	}

	float old_health = Get_Defense_Object()->Get_Health();

	DamageableGameObj::Apply_Damage( damager, scale, alternate_skin );

	if ( old_health != Get_Defense_Object()->Get_Health() ) {
		Set_Object_Dirty_Bit( NetworkObjectClass::BIT_OCCASIONAL, true );
	}

//	WWDEBUG_SAY(("Applied damage to a building (prefix=%s) new health = %f\r\n",Get_Definition().MeshPrefix,DefenseObject.Get_Health()));

	// Stats
	if ( DefenseObject.Get_Health() <= 0 ) {
		if ( damager.Get_Owner() != nullptr &&
			  damager.Get_Owner()->As_SoldierGameObj() != nullptr &&
			  damager.Get_Owner()->As_SoldierGameObj()->Get_Player_Data() != nullptr ) {
			damager.Get_Owner()->As_SoldierGameObj()->Get_Player_Data()->Stats_Add_Building_Destroyed();
		}

		//
		//	Reveal this building to the encyclopedia
		//
		EncyclopediaMgrClass::Reveal_Object (this);
	}

	/*
	** If our health status changed states, switch all of our aggregates
	*/
	Update_State();
}

void BuildingGameObj::Apply_Damage_Building(const OffenseObjectClass & offense, StaticPhysClass * component )
{
	/*
	** If the MCT is being damaged, use an alternate skin type
	*/
	ArmorType skin = DefenseObject.Get_Skin();
	if (component->Get_Factory().Chunk_ID() == PHYSICS_CHUNKID_BUILDINGAGGREGATE) {
		if (((BuildingAggregateClass *)component)->Is_MCT()) {
			skin = Get_Definition().MCTSkin;
		}
	}

	/*
	** Apply the damage
	*/
	Apply_Damage( offense, 1.0, skin );
}

void  BuildingGameObj::Apply_Damage_Building(	const OffenseObjectClass & offense,	bool mct_damage )
{
	/*
	** If the MCT is being damaged, use an alternate skin type
	*/
	ArmorType skin = DefenseObject.Get_Skin();
	if ( mct_damage ) {
		skin = Get_Definition().MCTSkin;
	}

	/*
	** Apply the damage
	*/
	Apply_Damage( offense, 1.0, skin );
}

/////////////////////////////////////////////////////////////////////////////
//
//	Is_Interior_Mesh_Name
//
/////////////////////////////////////////////////////////////////////////////
bool
BuildingGameObj::Is_Interior_Mesh_Name (const char * name)
{
// (gth) Renegade day 120 patch, accept tiles as building "parts"
#if 0
	char * meshname = strchr(name,'.');
	if (meshname != nullptr) {
		return strchr(meshname,'#') != nullptr;
	} else {
		return false;
	}
#else
	return strchr(name,'#') != nullptr;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//
//	Is_Exterior_Mesh_Name
//
/////////////////////////////////////////////////////////////////////////////
bool
BuildingGameObj::Is_Exterior_Mesh_Name (const char * name)
{
// (gth) Renegade day 120 patch, accept tiles as building "parts"
#if 0
	char * meshname = strchr(name,'.');
	if (meshname != nullptr) {
		return strchr(meshname,'^') != nullptr;
	} else {
		return false;
	}
#else
	return strchr(name,'^') != nullptr;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//
//	Name_Prefix_Matches_This_Building
//
/////////////////////////////////////////////////////////////////////////////
bool
BuildingGameObj::Name_Prefix_Matches_This_Building (const char * name)
{
	bool retval = false;

	if (name != nullptr) {
		StringClass prefex(Get_Definition().MeshPrefix,true);
		const char * meshname = strchr(name,'.');
		if (meshname != nullptr) {
			meshname++;
			retval = (strnicmp(meshname,prefex,strlen(prefex)) == 0);
		} else {
			retval = (strnicmp(name,prefex,strlen(prefex)) == 0);
		}
	}

	return retval;
}


/////////////////////////////////////////////////////////////////////////////
//
//	Set_Normalized_Health
//
/////////////////////////////////////////////////////////////////////////////
void
BuildingGameObj::Set_Normalized_Health (float health)
{
	//
	//	Note:  We convert from normalized to absolute values
	//
	DefenseObject.Set_Health (health * DefenseObject.Get_Health_Max ());
	Update_State ();


	WWDEBUG_SAY(("Building State Set:\r\n"));
	WWDEBUG_SAY(("  prefix = %s mesh count = %d aggregate count = %d light count = %d\r\n",
		Get_Definition().MeshPrefix.Peek_Buffer(),
		InteriorMeshes.Count() + ExteriorMeshes.Count(),
		Aggregates.Count(),
		PowerOnLights.Count() + PowerOffLights.Count()));
}


/////////////////////////////////////////////////////////////////////////////
//
//	Update_State
//
/////////////////////////////////////////////////////////////////////////////
void
BuildingGameObj::Update_State (bool force_update)
{
	/*
	**	Q-016's model swap, asked every time rather than only when CurrentState changes: the
	**	descriptor's health thresholds are data and can be finer than BuildingStateClass's four
	**	coarse bands, so a swap gated on the coarse state would silently never happen.  The call
	**	is a no-op for a map-authored building and compares one integer for a runtime one.
	*/
	Update_Runtime_Damage_Model (force_update);

	/*
	**	Phase 20 -- a building whose descriptor tracks a grid-facing subcomponent
	**	(power_generator_module/communications_array/radar_dish) generates a genuinely different
	**	number every time its health does, so the grid has to be told on the same event that
	**	already tells it about destruction (On_Runtime_Destroyed_State_Entered, below).  Gated on
	**	actually having one of those three roles declared so the overwhelming majority of
	**	buildings -- every descriptor before this phase, and every one after it that does not opt
	**	in -- take no extra RecalculateTeam call on every hit, honoring the header's own "call on
	**	relevant state/config changes, never every frame".
	**
	**	The constructor calls Update_State before Init/Load has attached a definition, so the
	**	descriptor lookup must wait until one exists.
	*/
	if (CombatManager::I_Am_Server () && Has_Definition ()) {
		const StringClass & descriptor_name = Get_Definition ().Get_Descriptor_Name ();
		const BuildingModelDescriptor *descriptor = (descriptor_name.Get_Length () > 0)
			? BuildingDescriptorManager::Get_Descriptor (descriptor_name.Peek_Buffer ()) : nullptr;
		if ((descriptor != nullptr) &&
			 (descriptor->Has_Subcomponent (SUBCOMPONENT_POWER_GENERATOR_MODULE) ||
			  descriptor->Has_Subcomponent (SUBCOMPONENT_COMMUNICATIONS_ARRAY) ||
			  descriptor->Has_Subcomponent (SUBCOMPONENT_RADAR_DISH)))
		{
			TeamPowerGridService::RecalculateTeam (Get_Player_Type ());
		}
	}

	/*
	** Evaluate our current state
	*/
	float health_percentage = 100.0f * DefenseObject.Get_Health()/DefenseObject.Get_Health_Max();
	int health_state = BuildingStateClass::Percentage_To_Health_State(health_percentage);
	int new_state = BuildingStateClass::Compose_State(health_state,IsPowerOn);

	/*
	** If the state has changed, inform all of our components
	*/
	if ((new_state != CurrentState) || (force_update == true)) {

		int old_health_state = BuildingStateClass::Get_Health_State( CurrentState );

		if (health_state != old_health_state) {
			int event = CUSTOM_EVENT_BUILDING_DAMAGED;
			if (health_state < old_health_state) {
				event = CUSTOM_EVENT_BUILDING_REPAIRED;
			}

			const GameObjObserverList & observer_list = Get_Observers();
			for( int index = 0; index < observer_list.Count(); index++ ) {
				observer_list[ index ]->Custom( this, event, int(health_percentage), nullptr );
			}
		}

		CurrentState = new_state;

		//
		//	Mark the object as dirty so it's state will be mirrored on the client
		//
		if (CombatManager::I_Am_Server ()) {
			Set_Object_Dirty_Bit (NetworkObjectClass::BIT_RARE, true);
		}

		/*
		** Aggregates: inform all aggregates of the new state
		*/
		RefMultiListIterator<BuildingAggregateClass> aggregate_iterator(&Aggregates);
		for (aggregate_iterator.First(); !aggregate_iterator.Is_Done(); aggregate_iterator.Next()) {
			aggregate_iterator.Peek_Obj()->Set_Current_State(CurrentState,force_update);
		}

		/*
		**	Q-016: a runtime-constructed building expresses damage by swapping whole models named
		**	by its descriptor, so the legacy alternate-material swap is not merely unnecessary for
		**	it, it is wrong -- the loaded model has no second material set to switch to, and the
		**	call would quietly do nothing while looking like it had done something.  Map-authored
		**	buildings keep the legacy behaviour untouched, which is the whole point of the split.
		*/
		if (!IsRuntimeConstructed) {

			/*
			** Interior meshes: switch to alternate materials if either the power is out or
			** the building is at zero health
			*/
			Enable_Alternate_Materials(InteriorMeshes,((health_state == BuildingStateClass::HEALTH_0) || (!IsPowerOn)));


			/*
			** Exterior meshes: use alternate materials if the building is destroyed
			*/
			Enable_Alternate_Materials(ExteriorMeshes,(health_state == BuildingStateClass::HEALTH_0));
		}

		/*
		** Lights: enable the lights which match the current power status, invalidate all
		** lighting caches within the bounds of the lights that changed.
		*/
		bool disable = ((IsPowerOn == false) || (health_state == BuildingStateClass::HEALTH_0));
		disable &= (!PowerOffLights.Is_Empty());	// for lights, power is not disabled if there are not power-off lights

		AABoxClass light_bounds(Position,Vector3(1,1,1));

		RefMultiListIterator<LightPhysClass> light_iterator(&PowerOnLights);
		for (light_iterator.First(); !light_iterator.Is_Done(); light_iterator.Next()) {
			light_iterator.Peek_Obj()->Set_Disabled(disable);
			light_bounds.Add_Box(light_iterator.Peek_Obj()->Peek_Model()->Get_Bounding_Box());
		}

		disable = !disable;
		for (light_iterator.First(&PowerOffLights); !light_iterator.Is_Done(); light_iterator.Next()) {
			light_iterator.Peek_Obj()->Set_Disabled(disable);
			light_bounds.Add_Box(light_iterator.Peek_Obj()->Peek_Model()->Get_Bounding_Box());
		}

		PhysicsSceneClass::Get_Instance()->Invalidate_Lighting_Caches(light_bounds);
	}

	return ;
}


/////////////////////////////////////////////////////////////////////////////
//
//	Play_Announcement
//
/////////////////////////////////////////////////////////////////////////////
void
BuildingGameObj::Play_Announcement(int text_id, bool broadcast)
{
	Stop_Current_Announcement();

	// Lookup the translation object from the strings database
	TDBObjClass* translate_obj = TranslateDBClass::Find_Object(text_id);

	if (translate_obj) {
		const unichar_t* string = translate_obj->Get_String();
		int sound_def_id = (int)translate_obj->Get_Sound_ID();
		float duration = 2.0F;

		//	Play the sound effect
		bool display_text = true;

		if (sound_def_id > 0) {

			//	Create the sound object
			CurrentAnnouncement = WWAudioClass::Get_Instance()->Create_Sound(sound_def_id);

			if (CurrentAnnouncement) {
				duration = (CurrentAnnouncement->Get_Duration() / 1000.0F);

				if (broadcast == false) {
					// Tweak the radii of the sound so it can be heard throughout the building
					CurrentAnnouncement->Set_DropOff_Radius(AnnouncementSphere.Radius);
					Sound3DClass* sound3D = CurrentAnnouncement->As_Sound3DClass();

					if (sound3D) {
						sound3D->Set_Max_Vol_Radius(AnnouncementSphere.Radius * 0.9F);
					}

					//	Play the sound effect at the annoucement position for the building
					CurrentAnnouncement->Set_Transform(Matrix3D(AnnouncementSphere.Center));
					display_text = (CurrentAnnouncement->Is_Sound_Culled() == false);
				}

				CurrentAnnouncement->Add_To_Scene();
			}
		}

		//	Display the text on the screen
		if (display_text && string) {
			float message_duration = std::max(duration, 5.0F);
			CombatManager::Get_Message_Window()->Add_Message(string, Vector3(1, 1, 1), nullptr, message_duration);
		}
	}
}


/////////////////////////////////////////////////////////////////////////////
//
//	Stop_Current_Announcement
//
/////////////////////////////////////////////////////////////////////////////
void
BuildingGameObj::Stop_Current_Announcement (void)
{
	if (CurrentAnnouncement) {

		//	Stop the sound object and remove it from the world
		CurrentAnnouncement->Stop();
		CurrentAnnouncement->Remove_From_Scene();
		CurrentAnnouncement->Release_Ref();
		CurrentAnnouncement = nullptr;
	}
}


/////////////////////////////////////////////////////////////////////////////
//
//	Initialize_Building
//
/////////////////////////////////////////////////////////////////////////////
void
BuildingGameObj::Initialize_Building (void)
{
	RefMultiListIterator<StaticPhysClass> mesh_iterator (&InteriorMeshes);

	//
	//	Generate a bounding box that contains all the building interior meshes
	//
	AABoxClass bounding_box (Position, Vector3 (1, 1, 1));
	for (mesh_iterator.First (); !mesh_iterator.Is_Done (); mesh_iterator.Next ()) {
		bounding_box.Add_Box (mesh_iterator.Peek_Obj ()->Peek_Model ()->Get_Bounding_Box());
	}

	//
	//	Turn the bounding box into a sphere that we can use to play building
	// announcement sound effects in
	//
	//	Q-016: a runtime building has no InteriorMeshes to measure -- Derive_Runtime_Structure
	//	already computed this sphere from the loaded model's own hierarchy, so overwriting it
	//	here with the degenerate box above would silence its announcements entirely.
	if (!IsRuntimeConstructed) {
		AnnouncementSphere.Center	= bounding_box.Center;
		AnnouncementSphere.Radius	= std::max (bounding_box.Extent.X, bounding_box.Extent.Y);
		AnnouncementSphere.Radius	= std::max (AnnouncementSphere.Radius, bounding_box.Extent.Z);
	}

	//
	//	If we can find a base for our team, then add ourselves to it.
	//
	BaseControllerClass *base = BaseControllerClass::Find_Base ( Get_Player_Type() );
	if (base != nullptr) {
		base->Add_Building (this);
	}

	//
	//	Roadmap Section 40 -- register with the team power grid.  Safe and unconditional even
	//	though almost no preset names a descriptor yet: TeamPowerGridService treats an empty or
	//	unresolved DescriptorName as a neutral member that neither generates, consumes, nor
	//	gets shed (see teampowergridservice.cpp, Peek_Descriptor) -- "missing config never
	//	blocks engine work" applies here exactly as it does to missing art.  A preset that does
	//	name one starts participating in real deterministic allocation the moment it does,
	//	with no further code change.  Registration itself is just bookkeeping (Get_ID() is the
	//	stable per-building member id), so it happens on both client and server; only the
	//	Enable_Power-driving recalculation is server-authoritative, matching Power_Base's own
	//	existing rule.
	//
	TeamPowerGridService::RegisterBuilding (this,Get_Player_Type (),Get_Definition ().Get_Descriptor_Name (),Get_ID ());
	if (CombatManager::I_Am_Server ()) {
		TeamPowerGridService::RecalculateTeam (Get_Player_Type ());
	}

	return ;
}


/////////////////////////////////////////////////////////////////////////////
//
//	Reset_Components
//
/////////////////////////////////////////////////////////////////////////////
void
BuildingGameObj::Reset_Components (void)
{
	RefMultiListIterator<StaticPhysClass> mesh_iterator(&InteriorMeshes);

	for (mesh_iterator.First(); !mesh_iterator.Is_Done(); mesh_iterator.Next()) {
		WWASSERT(mesh_iterator.Peek_Obj()->Get_Observer() == this);
		mesh_iterator.Peek_Obj()->Set_Observer(nullptr);
	}
	for (mesh_iterator.First(&ExteriorMeshes); !mesh_iterator.Is_Done(); mesh_iterator.Next()) {
		WWASSERT(mesh_iterator.Peek_Obj()->Get_Observer() == this);
		mesh_iterator.Peek_Obj()->Set_Observer(nullptr);
	}

	RefMultiListIterator<BuildingAggregateClass> agg_iterator(&Aggregates);
	for (agg_iterator.First(); !agg_iterator.Is_Done(); agg_iterator.Next()) {
		WWASSERT(agg_iterator.Peek_Obj()->Get_Observer() == this);
		agg_iterator.Peek_Obj()->Set_Observer(nullptr);
	}

	InteriorMeshes.Reset_List();
	ExteriorMeshes.Reset_List();
	PowerOnLights.Reset_List();
	PowerOffLights.Reset_List();
	Aggregates.Reset_List();
}


/////////////////////////////////////////////////////////////////////////////
//
//	Add_Mesh
//
/////////////////////////////////////////////////////////////////////////////
void
BuildingGameObj::Add_Mesh (StaticPhysClass * terrain)
{
	WWASSERT(terrain != nullptr);
	if (Is_Interior_Mesh_Name(terrain->Peek_Model()->Get_Name())) {
		WWASSERT(terrain->Get_Observer() == nullptr);
		InteriorMeshes.Add(terrain);
		terrain->Set_Observer(this);
	} else if (Is_Exterior_Mesh_Name(terrain->Peek_Model()->Get_Name())) {
		WWASSERT(terrain->Get_Observer() == nullptr);
		ExteriorMeshes.Add(terrain);
		terrain->Set_Observer(this);
	}
}


/////////////////////////////////////////////////////////////////////////////
//
//	Remove_Mesh
//
/////////////////////////////////////////////////////////////////////////////
void
BuildingGameObj::Remove_Mesh (StaticPhysClass * terrain)
{
	WWASSERT(terrain != nullptr);
	if (Is_Interior_Mesh_Name(terrain->Peek_Model()->Get_Name())) {
		WWASSERT(terrain->Get_Observer() == this);
		terrain->Set_Observer(nullptr);
		InteriorMeshes.Remove(terrain);
	} else if (Is_Exterior_Mesh_Name(terrain->Peek_Model()->Get_Name())) {
		WWASSERT(terrain->Get_Observer() == this);
		terrain->Set_Observer(nullptr);
		ExteriorMeshes.Remove(terrain);
	}
}


/////////////////////////////////////////////////////////////////////////////
//
//	Add_Aggregate
//
/////////////////////////////////////////////////////////////////////////////
void
BuildingGameObj::Add_Aggregate (BuildingAggregateClass * aggregate)
{
	WWASSERT(aggregate != nullptr);
	Aggregates.Add(aggregate);
	WWASSERT(aggregate->Get_Observer() == nullptr);
	aggregate->Set_Observer(this);
}


/////////////////////////////////////////////////////////////////////////////
//
//	Remove_Aggregate
//
/////////////////////////////////////////////////////////////////////////////
void
BuildingGameObj::Remove_Aggregate (BuildingAggregateClass * aggregate)
{
	WWASSERT(aggregate != nullptr);
	WWASSERT(aggregate->Get_Observer() == this);
	aggregate->Set_Observer(nullptr);
	Aggregates.Remove(aggregate);
}


/////////////////////////////////////////////////////////////////////////////
//
//	Add_Light
//
/////////////////////////////////////////////////////////////////////////////
void
BuildingGameObj::Add_Light (LightPhysClass * light)
{
	WWASSERT(light != nullptr);
	if (light->Get_Group_ID() == 0) {
		PowerOnLights.Add(light);
	} else {
		PowerOffLights.Add(light);
	}
}


/////////////////////////////////////////////////////////////////////////////
//
//	Enable_Alternate_Materials
//
/////////////////////////////////////////////////////////////////////////////
void
BuildingGameObj::Enable_Alternate_Materials (RefMultiListClass<StaticPhysClass> & models, bool onoff)
{
	RefMultiListIterator<StaticPhysClass> it(&models);
	for (it.First(); !it.Is_Done(); it.Next()) {
		Enable_Alternate_Materials(it.Peek_Obj()->Peek_Model(),onoff);
	}
}


/////////////////////////////////////////////////////////////////////////////
//
//	Enable_Alternate_Materials
//
/////////////////////////////////////////////////////////////////////////////
void
BuildingGameObj::Enable_Alternate_Materials (RenderObjClass * model,bool onoff)
{
	if (model == nullptr) return;

	for (int i=0; i<model->Get_Num_Sub_Objects(); i++) {
		RenderObjClass * subobj = model->Get_Sub_Object(i);
		Enable_Alternate_Materials(subobj,onoff);
		REF_PTR_RELEASE(subobj);
	}

	if (model->Class_ID() == RenderObjClass::CLASSID_MESH) {
		MeshModelClass * mesh_model = ((MeshClass *)model)->Get_Model();
		mesh_model->Enable_Alternate_Material_Description(onoff);
		REF_PTR_RELEASE(mesh_model);
	}
}


/////////////////////////////////////////////////////////////////////////////
//
//	CnC_Initialize
//
/////////////////////////////////////////////////////////////////////////////
void
BuildingGameObj::CnC_Initialize (BaseControllerClass *base)
{
	BaseController	= base;

	//
	//	Create an observer that we can use to monitor the building's health
	//
	BuildingMonitor = new BuildingMonitorClass;
	BuildingMonitor->Set_Building (this);
	Add_Observer (BuildingMonitor);
	return ;
}


/////////////////////////////////////////////////////////////////////////////
//
//	On_Damaged
//
/////////////////////////////////////////////////////////////////////////////
void
BuildingGameObj::On_Damaged (void)
{
	//
	//	Notify the controller that the building has been damaged
	//
	if (BaseController != nullptr) {
		BaseController->On_Building_Damaged (this);
	}

	return ;
}


/////////////////////////////////////////////////////////////////////////////
//
//	On_Destroyed
//
/////////////////////////////////////////////////////////////////////////////
void
BuildingGameObj::On_Destroyed (void)
{
	IsDestroyed = true;

	Update_Group_Lighting ();

	//
	//	Scatter debris around the ruin -- roadmap Section 28.  Every client that sees this
	//	call spawns its own fragments; none of them are networked, and none are GameObjs.
	//
	DebrisManager::Spawn_Building_Debris (this);

	//
	//	Roadmap Section 38.  Server-only -- every client reaches On_Destroyed to spawn its own
	//	debris, but only the server's count is gameplay-authoritative.
	//
	if (CombatManager::I_Am_Server ()) {
		RTSTelemetryService::Record_Event (TELEMETRY_BUILDING_DESTROYED,Get_Player_Type (),1.0f);
	}

	//
	//	Notify the controller that the building has been destroyed
	//
	if (BaseController != nullptr) {
		BaseController->On_Building_Destroyed (this);
	}

	//
	//	Roadmap Section 40 -- server-only, matching Power_Base's own rule: only the server
	//	decides Enable_Power; clients receive it mirrored through BIT_RARE like every other
	//	building state.
	//
	if (CombatManager::I_Am_Server ()) {
		TeamPowerGridService::OnBuildingDestroyed (Get_ID ());

		//
		//	Roadmap Section 45: a dead factory's queue is cancelled and every item refunded in
		//	full to whoever paid for it, and nothing it had finished is delivered afterwards.
		//	Here because this is the one death transition every path reaches (damage and
		//	Set_Is_Destroyed alike); the runtime destroyed-model state is derived from the
		//	IsDestroyed this sets, so it never needs a second call.
		//
		CommanderProductionService::Resolve_Facility_Destroyed (Get_ID ());
	}

	//
	//	Mark the object as dirty so it's state will be mirrored on the client
	//
	if (CombatManager::I_Am_Server ()) {
		Set_Object_Dirty_Bit (NetworkObjectClass::BIT_RARE, true);
	}

	//
	//	Reveal this building to the encyclopedia
	//
	EncyclopediaMgrClass::Reveal_Object (this);
	return ;
}


/////////////////////////////////////////////////////////////////////////////
//
//	On_Revived
//
//	The inverse of On_Destroyed.  Nothing in stock Renegade brings a building
//	back, but Set_Is_Destroyed does, and without this the base controller would
//	stay convinced the base was lost and the model would keep its rubble state.
//
/////////////////////////////////////////////////////////////////////////////
void
BuildingGameObj::On_Revived (void)
{
	IsDestroyed = false;

	Update_Group_Lighting ();

	if (BaseController != nullptr) {
		BaseController->On_Building_Revived (this);
	}

	//
	//	Roadmap Section 40 -- server-only, matching Power_Base's own rule (see On_Destroyed).
	//
	if (CombatManager::I_Am_Server ()) {
		TeamPowerGridService::OnBuildingRevived (Get_ID ());
	}

	//
	//	A defence that tore its guns down when the building fell has to be told
	//	it may put them back up.
	//
	const GameObjObserverList &observer_list = Get_Observers ();
	for (int index = 0; index < observer_list.Count (); index ++) {
		observer_list[index]->Custom (this, CUSTOM_EVENT_BUILDING_REVIVED, 0, nullptr);
	}

	if (CombatManager::I_Am_Server ()) {
		Set_Object_Dirty_Bit (NetworkObjectClass::BIT_RARE, true);
	}

	//
	//	The current state was picked for a destroyed building, so it has to be
	//	recomputed now that the building is standing again.
	//
	Update_State (true);
	return ;
}


/////////////////////////////////////////////////////////////////////////////
//
//	Set_Is_Destroyed
//
//	Routed through the two notifications so a script flipping this flag gets
//	the same effects the damage system would have produced.
//
/////////////////////////////////////////////////////////////////////////////
void
BuildingGameObj::Set_Is_Destroyed (bool onoff)
{
	if (onoff == IsDestroyed) {
		return ;
	}

	if (onoff) {
		On_Destroyed ();
	} else {
		On_Revived ();
	}

	return ;
}


/////////////////////////////////////////////////////////////////////////////
//
//	Import_Rare
//
/////////////////////////////////////////////////////////////////////////////
void
BuildingGameObj::Import_Rare (BitStreamClass &packet)
{
	DamageableGameObj::Import_Rare (packet);

	//
	//	Read the state information from the server
	//
	bool is_destroyed	= IsDestroyed;
	bool is_power_on	= IsPowerOn;
	int current_state	= CurrentState;
	packet.Get (is_destroyed);
	packet.Get (is_power_on);
	packet.Get (current_state, BITPACK_BUILDING_STATE);

	//
	//	Is the power state of the building changing?
	//
	if (is_power_on != IsPowerOn) {
		Enable_Power (is_power_on);
	}

	//
	// Update our health if we are destroyed
	//
	if (is_destroyed) {
		Get_Defense_Object()->Set_Health(0.0f);
	}

	//
	//	Recalculate our state (if necessary)
	//
	if (current_state != CurrentState) {
		Update_State ();
	}

	//
	//	Become destroyed (if necessary)
	//
	if (is_destroyed && IsDestroyed == false) {
		On_Destroyed ();
	}

	//
	//	... and the other way, so a building revived on the server comes back
	//	here too.
	//
	if (is_destroyed == false && IsDestroyed) {
		On_Revived ();
	}

	packet.Get (IsSpyDisabled);

	/*
	**	Component-assembly door state.  Deterministic on both sides which (if any) of this
	**	descriptor's assembly parts are doors -- both machines loaded the same data/
	**	building_descriptors JSON -- so no count crosses the wire, only the two numbers per door
	**	that are genuinely dynamic (RuntimeDoorService::Compute_Fraction derives everything
	**	else). A map-authored building, or a runtime one with no door part, reads nothing here.
	*/
	if (IsRuntimeConstructed) {
		const BuildingModelDescriptor *descriptor = Peek_Runtime_Descriptor (RuntimeDescriptorName);
		if (descriptor != nullptr) {
			for (int i = 0; i < descriptor->Assembly.Parts.Count (); i++) {
				if (descriptor->Assembly.Parts [i].Kind.Compare_No_Case ("door") != 0) {
					continue;
				}
				bool direction_is_opening = false;
				float fraction_at_start = 0.0f;
				float start_time_seconds = 0.0f;
				packet.Get (direction_is_opening);
				packet.Get (fraction_at_start);
				packet.Get (start_time_seconds);
				RuntimeDoorService::Set_State (Get_ID (),i,direction_is_opening,fraction_at_start,start_time_seconds);
			}
		}
	}

	return ;
}


/////////////////////////////////////////////////////////////////////////////
//
//	Export_Rare
//
/////////////////////////////////////////////////////////////////////////////
void
BuildingGameObj::Export_Rare (BitStreamClass &packet)
{
	DamageableGameObj::Export_Rare (packet);

	//
	//	Transmit the state information
	//
	packet.Add (IsDestroyed);
	packet.Add (IsPowerOn);
	packet.Add (CurrentState, BITPACK_BUILDING_STATE);
	packet.Add (IsSpyDisabled);

	//	Mirrors Import_Rare's door block exactly -- same descriptor, same order, no count.
	if (IsRuntimeConstructed) {
		const BuildingModelDescriptor *descriptor = Peek_Runtime_Descriptor (RuntimeDescriptorName);
		if (descriptor != nullptr) {
			for (int i = 0; i < descriptor->Assembly.Parts.Count (); i++) {
				if (descriptor->Assembly.Parts [i].Kind.Compare_No_Case ("door") != 0) {
					continue;
				}
				bool direction_is_opening = false;
				float fraction_at_start = 0.0f;
				float start_time_seconds = 0.0f;
				RuntimeDoorService::Get_State (Get_ID (),i,&direction_is_opening,&fraction_at_start,&start_time_seconds);
				packet.Add (direction_is_opening);
				packet.Add (fraction_at_start);
				packet.Add (start_time_seconds);
			}
		}
	}

	return ;
}


/////////////////////////////////////////////////////////////////////////////
//
//	Set_Precision
//
/////////////////////////////////////////////////////////////////////////////
void
BuildingGameObj::Set_Precision (void)
{
	cEncoderList::Set_Precision (BITPACK_BUILDING_STATE, -1, (int)BuildingStateClass::STATE_COUNT);
	return ;
}


/////////////////////////////////////////////////////////////////////////////
//
//	Collect_Building_Components
//
/////////////////////////////////////////////////////////////////////////////
void
BuildingGameObj::Collect_Building_Components (void)
{
	/*
	**	Q-016: the legacy proximity/prefix scan and the runtime hierarchy path are alternatives,
	**	never stages of one another.  Running this for a runtime building would tear down the
	**	model it is holding (Reset_Components) and then claim whatever map statics happened to sit
	**	near where the Commander put it -- so it refuses outright rather than being made to cope.
	*/
	if (IsRuntimeConstructed) {
		return ;
	}

	//
	//	A runtime building coming back from a save takes the runtime path it was built by, not
	//	the proximity scan, which would claim whatever map statics sat where it stands.
	//
	if (LoadedRuntimeDescriptorName.Get_Length () > 0) {
		StringClass descriptor_name = LoadedRuntimeDescriptorName;
		LoadedRuntimeDescriptorName = "";
		if (Init_Runtime_Building (descriptor_name.Peek_Buffer (),LoadedRuntimeTransform)) {
			return ;
		}
	}

	Reset_Components ();

	//
	// Iterate through all static objects in the level, assigning the building components to
	// the appropriate building controller
	//
	RefPhysListIterator staticobj_iterator = PhysicsSceneClass::Get_Instance()->Get_Static_Object_Iterator();
	for (staticobj_iterator.First(); !staticobj_iterator.Is_Done(); staticobj_iterator.Next()) {

		StaticPhysClass * obj = staticobj_iterator.Peek_Obj()->As_StaticPhysClass ();
		WWASSERT (obj != nullptr);
		WWASSERT (obj->Peek_Model() != nullptr);

		const char *obj_name = obj->Peek_Model()->Get_Name();

		if (obj->Get_Factory().Chunk_ID() == PHYSICS_CHUNKID_BUILDINGAGGREGATE) {

			//
			//	Does this aggregate match the prefix that this building is expecting?
			//
			if (Name_Prefix_Matches_This_Building (obj_name)) {

				//
				//	Calculate how far the object is from the center of this building's
				// collection sphere
				//
				Vector3 obj_pos = obj->Peek_Model()->Get_Bounding_Box().Center;
				float dist2 = (CollectionSphere.Center - obj_pos).Length2 ();

				//
				//	Is this object within the collection sphere?
				//
				if (dist2 <= (CollectionSphere.Radius * CollectionSphere.Radius)) {

					// Check for an existing building owner
					CombatPhysObserverClass * old_observer = (CombatPhysObserverClass *)obj->Get_Observer();
					BuildingGameObj * existing_building = nullptr;
					if ( old_observer != nullptr ) {
						existing_building = old_observer->As_BuildingGameObj();
					}
					if ( existing_building != nullptr ) {
						// Another building already has this
						float existing_dist2 = (existing_building->CollectionSphere.Center - obj_pos).Length2 ();

						if ( dist2 < existing_dist2 ) {
							// But we are closer, so remove from the existing
							existing_building->Remove_Aggregate ((BuildingAggregateClass *)obj);
							// and Take him
							Add_Aggregate ((BuildingAggregateClass *)obj);
						}

					} else {
						// We are the first/only
						Add_Aggregate ((BuildingAggregateClass *)obj);
					}

				}
			}

		} else if (Is_Interior_Mesh_Name (obj_name) || Is_Exterior_Mesh_Name (obj_name)) {

			//
			//	Does this mesh match the prefix that this building is expecting?
			//
			if (Name_Prefix_Matches_This_Building (obj_name)) {

				//
				//	Calculate how far the object is from the center of this building's
				// collection sphere
				//
				Vector3 obj_pos = obj->Peek_Model()->Get_Bounding_Box().Center;
				float dist2 = (CollectionSphere.Center - obj_pos).Length2 ();

				//
				//	Is this object within the collection sphere?
				//
				if (dist2 <= (CollectionSphere.Radius * CollectionSphere.Radius)) {

					// Check for an existing building owner
					CombatPhysObserverClass * old_observer = (CombatPhysObserverClass *)obj->Get_Observer();
					BuildingGameObj * existing_building = nullptr;
					if ( old_observer != nullptr ) {
						existing_building = old_observer->As_BuildingGameObj();
					}
					if ( existing_building != nullptr ) {
						// Another building already has this
						float existing_dist2 = (existing_building->CollectionSphere.Center - obj_pos).Length2 ();

						if ( dist2 < existing_dist2 ) {
							// But we are closer, so remove from the existing
							existing_building->Remove_Mesh (obj);
							// and Take him
							Add_Mesh (obj);
						}

					} else {
						// We are the first/only
						Add_Mesh (obj);
					}

				}
			}
		}
	}

	//
	// Iterate through all static lights in the level, assigning each building light to
	// the appropriate building controller
	//
	RefPhysListIterator staticlight_iterator = PhysicsSceneClass::Get_Instance ()->Get_Static_Light_Iterator ();
	for (staticlight_iterator.First (); !staticlight_iterator.Is_Done (); staticlight_iterator.Next ()) {

		LightPhysClass * light = staticlight_iterator.Peek_Obj()->As_LightPhysClass();
		WWASSERT(light != nullptr);
		WWASSERT(light->Peek_Model() != nullptr);

		//
		//	Does this light match the prefix that this building is expecting?
		//
		if (Name_Prefix_Matches_This_Building (light->Get_Name ())) {

			//
			//	Calculate how far the light is from the center of this building's
			// collection sphere
			//
			Vector3 light_pos;
			light->Get_Position (&light_pos);
			float dist2 = (CollectionSphere.Center - light_pos).Length2 ();

			//
			//	Is this light within the collection sphere?
			//
			if (dist2 <= (CollectionSphere.Radius * CollectionSphere.Radius)) {
				Add_Light (light);
			}
		}
	}

	Update_Bounding_Box ();

	//
	//	Update the building's state
	//
	Initialize_Building ();
	Update_State (true);
	return ;
}


/////////////////////////////////////////////////////////////////////////////
//
//	Update_Bounding_Box
//
//	Union of every collected component's bounding box.  Falls back to a
//	degenerate box at the building's position when nothing was collected, so
//	Get_Bounds never hands back uninitialized memory.
//
/////////////////////////////////////////////////////////////////////////////
void
BuildingGameObj::Update_Bounding_Box (void)
{
	MinMaxAABoxClass bounds;
	bounds.Init_Empty ();
	bool found_any = false;

	RefMultiListIterator<BuildingAggregateClass> aggregate_it (&Aggregates);
	for (aggregate_it.First (); !aggregate_it.Is_Done (); aggregate_it.Next ()) {
		RenderObjClass *model = aggregate_it.Peek_Obj ()->Peek_Model ();
		if (model != nullptr) {
			bounds.Add_Box (model->Get_Bounding_Box ());
			found_any = true;
		}
	}

	RefMultiListClass<StaticPhysClass> *mesh_lists[2] = { &InteriorMeshes, &ExteriorMeshes };
	for (int index = 0; index < 2; index ++) {
		RefMultiListIterator<StaticPhysClass> mesh_it (mesh_lists[index]);
		for (mesh_it.First (); !mesh_it.Is_Done (); mesh_it.Next ()) {
			RenderObjClass *model = mesh_it.Peek_Obj ()->Peek_Model ();
			if (model != nullptr) {
				bounds.Add_Box (model->Get_Bounding_Box ());
				found_any = true;
			}
		}
	}

	if (found_any) {
		BoundingBox.Init (bounds);
	} else {
		BoundingBox.Center = Position;
		BoundingBox.Extent.Set (0, 0, 0);
	}

	return ;
}


/////////////////////////////////////////////////////////////////////////////
//
//	Export_Creation
//
/////////////////////////////////////////////////////////////////////////////
void
BuildingGameObj::Export_Creation (BitStreamClass &packet)
{
	DamageableGameObj::Export_Creation (packet);

	//
	//	Send the object's position
	//
	Vector3 position (0, 0, 0);
	Get_Position (&position);
	packet.Add (position.X, BITPACK_WORLD_POSITION_X);
	packet.Add (position.Y, BITPACK_WORLD_POSITION_Y);
	packet.Add (position.Z, BITPACK_WORLD_POSITION_Z);

	//
	//	Send the collection sphere's definition
	//
	packet.Add (CollectionSphere.Center.X,		BITPACK_WORLD_POSITION_X);
	packet.Add (CollectionSphere.Center.Y,		BITPACK_WORLD_POSITION_Y);
	packet.Add (CollectionSphere.Center.Z,		BITPACK_WORLD_POSITION_Z);
	packet.Add (CollectionSphere.Radius,		BITPACK_BUILDING_RADIUS);

	/*
	**	Q-016: which of the two collection paths the receiving client must take, and everything
	**	that path needs.  A map-authored building sends one bit and nothing else, exactly as it
	**	always did.  A runtime building sends its descriptor name and its full transform, because
	**	a client cannot derive either: the descriptor chooses the model and the damage table, and
	**	the transform is a placement the client never saw validated.  The whole 4x3 goes on the
	**	wire rather than a yaw, because Build_Placement_Transform may tilt a building to its
	**	ground slope and a re-derived yaw-only transform would sink one corner into the terrain.
	*/
	packet.Add (IsRuntimeConstructed);
	if (IsRuntimeConstructed) {
		packet.Add_Terminated_String (RuntimeDescriptorName.Peek_Buffer (),true);
		for (int row = 0; row < 3; row ++) {
			for (int col = 0; col < 4; col ++) {
				packet.Add (RuntimeTransform [row][col]);
			}
		}
	}
	return ;
}


/////////////////////////////////////////////////////////////////////////////
//
//	Import_Creation
//
/////////////////////////////////////////////////////////////////////////////
void
BuildingGameObj::Import_Creation (BitStreamClass &packet)
{
	DamageableGameObj::Import_Creation (packet);

	//
	//	Read the object's position
	//
	Vector3 position (0, 0, 0);
	packet.Get (position.X, BITPACK_WORLD_POSITION_X);
	packet.Get (position.Y, BITPACK_WORLD_POSITION_Y);
	packet.Get (position.Z, BITPACK_WORLD_POSITION_Z);
	Set_Position (position);

	//
	//	Read the collection sphere's definition
	//
	packet.Get (CollectionSphere.Center.X,		BITPACK_WORLD_POSITION_X);
	packet.Get (CollectionSphere.Center.Y,		BITPACK_WORLD_POSITION_Y);
	packet.Get (CollectionSphere.Center.Z,		BITPACK_WORLD_POSITION_Z);
	packet.Get (CollectionSphere.Radius,		BITPACK_BUILDING_RADIUS);

	bool is_runtime = false;
	packet.Get (is_runtime);

	if (is_runtime) {

		char descriptor_name [256] = { 0 };
		packet.Get_Terminated_String (descriptor_name,sizeof (descriptor_name),true);

		Matrix3D transform (1);
		for (int row = 0; row < 3; row ++) {
			for (int col = 0; col < 4; col ++) {
				packet.Get (transform [row][col]);
			}
		}

		//
		//	The runtime collection path, in place of the proximity scan below.  It loads the
		//	model, derives the structure from it and leaves the building ready for Update_State.
		//
		Init_Runtime_Building (descriptor_name,transform);
		return ;
	}

	//
	//	Now initialize the object
	//
	Collect_Building_Components ();
	Update_State ();
	Initialize_Building ();
	return ;
}


/*
////////////////////////////////////////////////////////////////////////////
//
//	Compute_Object_Priority
//
/////////////////////////////////////////////////////////////////////////////
float
BuildingGameObj::Compute_Object_Priority (int client_id, const Vector3 &client_pos)
{
	float priority = 1.0F;

	if (Get_Object_Dirty_Bit (client_id, BIT_RARE) == false) {
		priority = DamageableGameObj::Compute_Object_Priority (client_id, client_pos);
	}

	//
	// Priority value is cached, update the cached value.
	//
	Set_Cached_Priority(priority);

	return priority;
}
*/


////////////////////////////////////////////////////////////////////////////
//
//	Get_Description
//
/////////////////////////////////////////////////////////////////////////////
void
//BuildingGameObj::Get_Extended_Information(StringClass & description)
BuildingGameObj::Get_Description(StringClass & description)
{
	//
	// Construct a diagnostic string
	//

	StringClass line(0,true);

   line.Format("ID:    %d\n", Get_ID());
	description += line;

   line.Format("Name:  %s\n", Get_Definition().Get_Name());
	description += line;

   line.Format("Team:  %d\n", Get_Player_Type());
	description += line;

	Vector3 position;
	Get_Position(&position);
	line.Format("POS:   %-5.2f, %-5.2f, %-5.2f\n", position.X, position.Y, position.Z);
	description += line;

	if (Get_Defense_Object() != nullptr) {
		line.Format("HLTH:  %-5.2f\n", Get_Defense_Object()->Get_Health());
		description += line;
	}

	line.Format("HIB:   %d\n", Is_Hibernating());
	description += line;

	line.Format("Destr: %d\n", IsDestroyed);
	description += line;

	line.Format("Power: %d\n", IsPowerOn);
	description += line;

	line.Format("ISC:   %d\n", Get_Import_State_Count());
	description += line;
}


////////////////////////////////////////////////////////////////////////////
//
//	Find_Closest_Poly_For_Model
//
/////////////////////////////////////////////////////////////////////////////
void
BuildingGameObj::Find_Closest_Poly_For_Model
(
	RenderObjClass *	model,
	const Vector3 &	pos,
	float *				distance2,
	Vector3 *			poly_pos
)
{
	if (model == nullptr) {
		return ;
	}

	//
	//	Convert the point from world space to object space
	//
	Vector3 obj_space_point;
	Matrix3D::Inverse_Transform_Vector (model->Get_Transform (), pos, &obj_space_point);
	int index;
	for (index = 0; index < model->Get_Num_Sub_Objects (); index++) {
		RenderObjClass *sub_obj = model->Get_Sub_Object (index);

		//
		//	Recurse into this sub-object
		//
		if (sub_obj != nullptr) {
			Find_Closest_Poly_For_Model (sub_obj, pos, distance2, poly_pos);
		}

		REF_PTR_RELEASE (sub_obj);
	}

	//
	//	Is this object a mesh?  If so, then do a distance check against all its polygons
	//
	if (model->Class_ID () == RenderObjClass::CLASSID_MESH) {
		MeshModelClass *mesh_model = ((MeshClass *)model)->Peek_Model ();
		if (mesh_model != nullptr) {

			const TriIndex *tri_array	= mesh_model->Get_Polygon_Array ();
			Vector3 *vert_array			= mesh_model->Get_Vertex_Array ();

			//
			//	Check each polygon to see which is the closest
			//
			int poly_count = mesh_model->Get_Polygon_Count ();
			for (index = 0; index < poly_count; index ++) {

				int vert1 = tri_array[index][0];
				int vert2 = tri_array[index][1];
				int vert3 = tri_array[index][2];

				//
				//	Compute the distance from the center of this polygon to the
				//
				//
				Vector3 poly_center	= (vert_array[vert1] + vert_array[vert2] + vert_array[vert3]) * 0.33F;
				Vector3 delta			= (poly_center - obj_space_point);
				float dist2				= delta.Length2 ();
				if (dist2 < (*distance2)) {
					(*distance2) = dist2;

					//
					//	The comparison is done in the model's own space; the
					//	caller wants the answer in world space.
					//
					if (poly_pos != nullptr) {
						Matrix3D::Transform_Vector (model->Get_Transform (), poly_center, poly_pos);
					}
				}
			}
		}
	}

	return ;
}


////////////////////////////////////////////////////////////////////////////
//
//	Find_MCT
//
/////////////////////////////////////////////////////////////////////////////
BuildingAggregateClass *
BuildingGameObj::Find_MCT (void)
{
	RefMultiListIterator<BuildingAggregateClass> iterator (&Aggregates);

	for (iterator.First (); !iterator.Is_Done (); iterator.Next ()) {
		if (iterator.Peek_Obj ()->Is_MCT ()) {
			return iterator.Peek_Obj ();
		}
	}

	return nullptr;
}


////////////////////////////////////////////////////////////////////////////
//
//	Get_Commander_Exit_Point -- see the comment in building.h
//
////////////////////////////////////////////////////////////////////////////
void
BuildingGameObj::Get_Commander_Exit_Point (Vector3 * out_point,float * out_radius) const
{
	const float MIN_STANDOFF		= 1.5f;		// floor on the footprint-derived standoff distance
	const float EXIT_CLEARANCE	= 3.0f;		// margin added past the footprint's own edge
	const float CHECK_RADIUS		= 2.5f;		// how wide a clear space has to be at the point itself

	float standoff = (BoundingBox.Extent.X + BoundingBox.Extent.Y) * 0.5f + EXIT_CLEARANCE;
	if (standoff < MIN_STANDOFF) {
		standoff = MIN_STANDOFF;
	}

	Vector3 position (0.0f,0.0f,0.0f);
	Get_Position (&position);

	/*
	**	Q-016: a runtime-constructed building does have an orientation, and may name an
	**	`exit_anchor` bone on its own model.  Asked in that order -- the anchor is the artist's
	**	answer and beats anything derived -- and only then does this fall back to the fixed
	**	direction a legacy aggregate building has no choice but to use.
	*/
	if (IsRuntimeConstructed) {

		Matrix3D anchor (1);
		if (Resolve_Runtime_Anchor ("exit_anchor",&anchor)) {
			if (out_point != nullptr) {
				*out_point = anchor.Get_Translation ();
			}
			if (out_radius != nullptr) {
				*out_radius = CHECK_RADIUS;
			}
			return ;
		}

		if (out_point != nullptr) {
			*out_point = position + RuntimeTransform.Get_X_Vector () * standoff;
		}
		if (out_radius != nullptr) {
			*out_radius = CHECK_RADIUS;
		}
		return ;
	}

	//	A fixed direction, not a derived facing: this legacy aggregate building has no single
	//	orientation to read one from (Position is a point, not a transform).
	if (out_point != nullptr) {
		*out_point = position + Vector3 (standoff,0.0f,0.0f);
	}
	if (out_radius != nullptr) {
		*out_radius = CHECK_RADIUS;
	}
}


////////////////////////////////////////////////////////////////////////////
//
//	Attach_Visual_Module / Detach_Visual_Module / Set_Visual_Module_Damaged
//
//	Roadmap Section 29 -- see the comment in building.h.  A building is a set of collected
//	components, so the bone name alone decides the host: whichever aggregate or exterior mesh
//	actually carries it wins.  Get_Bone_Index answers 0 both for "root" and for "no such bone",
//	the same convention VehicleGameObj::Aquire_Turret_Bones already reads it by, so a module
//	cannot be attached to a component's root -- which is the right refusal anyway, since a
//	module with no named place on the geometry has no business being positioned by this.
//
/////////////////////////////////////////////////////////////////////////////
bool
BuildingGameObj::Attach_Visual_Module (const char *slot_name,const char *bone_name,const char *model_name)
{
	RenderObjClass *host = nullptr;

	RefMultiListIterator<BuildingAggregateClass> agg_it (&Aggregates);
	for (agg_it.First (); !agg_it.Is_Done () && (host == nullptr); agg_it.Next ()) {
		RenderObjClass *model = agg_it.Peek_Obj ()->Peek_Model ();
		if ((model != nullptr) && (model->Get_Bone_Index (bone_name) != 0)) {
			host = model;
		}
	}

	RefMultiListIterator<StaticPhysClass> mesh_it (&ExteriorMeshes);
	for (mesh_it.First (); !mesh_it.Is_Done () && (host == nullptr); mesh_it.Next ()) {
		RenderObjClass *model = mesh_it.Peek_Obj ()->Peek_Model ();
		if ((model != nullptr) && (model->Get_Bone_Index (bone_name) != 0)) {
			host = model;
		}
	}

	if (host == nullptr) {
		WWDEBUG_SAY (("BuildingGameObj::Attach_Visual_Module() : no component carries bone '%s'\n",
			bone_name));
		return false;
	}

	if (AttachmentController == nullptr) {
		AttachmentController = new CompositeAttachmentClass;
	}
	return AttachmentController->Attach (host, slot_name, bone_name, model_name);
}

void
BuildingGameObj::Detach_Visual_Module (const char *slot_name)
{
	if (AttachmentController != nullptr) {
		AttachmentController->Detach (slot_name);
	}
}

void
BuildingGameObj::Set_Visual_Module_Damaged (const char *slot_name,bool damaged)
{
	if (AttachmentController != nullptr) {
		AttachmentController->Set_Slot_Hidden (slot_name, damaged);
	}
}


////////////////////////////////////////////////////////////////////////////
//
//	Find_Closest_Poly
//
/////////////////////////////////////////////////////////////////////////////
void
BuildingGameObj::Find_Closest_Poly (const Vector3 &pos, float *distance2)
{
	Find_Closest_Poly (pos, distance2, nullptr);
	return ;
}


////////////////////////////////////////////////////////////////////////////
//
//	Find_Closest_Poly
//
//	As above, but also reports where the closest polygon is.  poly_pos is left
//	alone when the building has no geometry, so callers should seed it.
//
/////////////////////////////////////////////////////////////////////////////
void
BuildingGameObj::Find_Closest_Poly (const Vector3 &pos, float *distance2, Vector3 *poly_pos)
{
	(*distance2) = 9999.0F;

	RefMultiListIterator<StaticPhysClass> int_iterator (&InteriorMeshes);
	RefMultiListIterator<StaticPhysClass> ext_iterator (&ExteriorMeshes);

	//
	//	Check all the interior meshes first
	//
	for (int_iterator.First (); !int_iterator.Is_Done (); int_iterator.Next ()) {
		Find_Closest_Poly_For_Model (int_iterator.Peek_Obj ()->Peek_Model (), pos, distance2, poly_pos);
	}

	//
	//	Now, check all the exterior meshes
	//
	for (ext_iterator.First (); !ext_iterator.Is_Done (); ext_iterator.Next ()) {
		Find_Closest_Poly_For_Model (ext_iterator.Peek_Obj ()->Peek_Model (), pos, distance2, poly_pos);
	}

	return ;
}


////////////////////////////////////////////////////////////////////////////
//
//	Is_In_Range_Coarse
//
//	Squared distance from the point to the building's bounding box, which is
//	zero on every axis where the point lies inside the slab.  This is the cheap
//	rejection that keeps Building_In_Range off the per-polygon path.
//
/////////////////////////////////////////////////////////////////////////////
bool
BuildingGameObj::Is_In_Range_Coarse (const Vector3 &point, float range_sq) const
{
	Vector3 delta (0, 0, 0);

	for (int axis = 0; axis < 3; axis ++) {
		float offset = point[axis] - BoundingBox.Center[axis];
		float extent = BoundingBox.Extent[axis];

		if (offset > extent) {
			delta[axis] = offset - extent;
		} else if (offset < -extent) {
			delta[axis] = offset + extent;
		}
	}

	return delta.Length2 () <= range_sq;
}


////////////////////////////////////////////////////////////////////////////
//
//	Building_In_Range
//
//	0 for no, 1 for yes, 2 for the MCT being in range.  The MCT answer is the
//	more specific one, so it is tested first and wins.
//
/////////////////////////////////////////////////////////////////////////////
int
BuildingGameObj::Building_In_Range (const Vector3 &point, float range)
{
	float range_sq = range * range;

	if (Is_In_Range_Coarse (point, range_sq) == false) {
		return 0;
	}

	BuildingAggregateClass *mct = Find_MCT ();
	if (mct != nullptr) {
		float mct_distance2 = 9999.0F;
		Find_Closest_Poly_For_Model (mct->Peek_Model (), point, &mct_distance2);
		if (mct_distance2 <= range_sq) {
			return 2;
		}
	}

	float distance2 = 9999.0F;
	Find_Closest_Poly (point, &distance2);
	if (distance2 <= range_sq) {
		return 1;
	}

	return 0;
}


////////////////////////////////////////////////////////////////////////////
//
//	Cast_Ray
//
//	A building is an aggregate of static meshes with no model of its own, so
//	the ray has to be offered to each collected mesh in turn.  The result
//	structure shortens the ray as hits land, so the nearest one survives.
//
/////////////////////////////////////////////////////////////////////////////
bool
BuildingGameObj::Cast_Ray (RayCollisionTestClass &raytest)
{
	if (raytest.Cull (BoundingBox)) {
		return false;
	}

	bool hit = false;

	RefMultiListClass<StaticPhysClass> *mesh_lists[2] = { &InteriorMeshes, &ExteriorMeshes };
	for (int index = 0; index < 2; index ++) {

		RefMultiListIterator<StaticPhysClass> mesh_it (mesh_lists[index]);
		for (mesh_it.First (); !mesh_it.Is_Done (); mesh_it.Next ()) {

			RenderObjClass *model = mesh_it.Peek_Obj ()->Peek_Model ();
			if (model != nullptr && model->Cast_Ray (raytest)) {
				hit = true;
			}
		}
	}

	return hit;
}
/***********************************************************************************************
 ***                                                                                         ***
 ***	Roadmap Section 44 / Q-016 -- the runtime component collection path.                    ***
 ***                                                                                         ***
 ***	Everything below this line runs only for a Commander-constructed building.  Nothing      ***
 ***	above it changed shape for their sake: the map-authored proximity/prefix scan, the       ***
 ***	aggregate state machine and the alternate-material damage visuals are exactly what they  ***
 ***	were, and a building created by a level keeps taking that path start to finish.          ***
 ***                                                                                         ***
 ***********************************************************************************************/

static const BuildingModelDescriptor *
Peek_Runtime_Descriptor (const StringClass &name)
{
	if (name.Get_Length () == 0) {
		return nullptr;
	}
	return BuildingDescriptorManager::Get_Descriptor (name.Peek_Buffer ());
}

static void
Walk_Runtime_Hierarchy (RenderObjClass *obj, MinMaxAABoxClass *interior_bounds,
								int *interior_count, int *exterior_count)
{
	if (obj == nullptr) {
		return ;
	}

	const char *name = obj->Get_Name ();
	if (name != nullptr) {
		if (BuildingGameObj::Is_Interior_Mesh_Name (name)) {
			interior_bounds->Add_Box (obj->Get_Bounding_Box ());
			(*interior_count) ++;
		} else if (BuildingGameObj::Is_Exterior_Mesh_Name (name)) {
			(*exterior_count) ++;
		}
	}

	int count = obj->Get_Num_Sub_Objects ();
	for (int index = 0; index < count; index ++) {
		RenderObjClass *sub_object = obj->Get_Sub_Object (index);
		Walk_Runtime_Hierarchy (sub_object,interior_bounds,interior_count,exterior_count);
		REF_PTR_RELEASE (sub_object);
	}

	return ;
}

RenderObjClass *
BuildingGameObj::Peek_Runtime_Model (void) const
{
	if (RuntimeParts.Count () == 0) {
		return nullptr;
	}
	StaticPhysClass *phys = RuntimeParts [0];
	return (phys != nullptr) ? phys->Peek_Model () : nullptr;
}

//
//	Q-017 follow-up -- shared by Build_Runtime_Components and Build_Runtime_Assembly below: put
//	one already-loaded model into the scene as its own StaticPhysClass, at a given transform, and
//	record it in RuntimeParts.  Neither caller's own bookkeeping (RuntimeModelName, which of
//	Exterior/InteriorMeshes) belongs here -- only what every part construction has in common.
//
static StaticPhysClass *
Add_Runtime_Part (BuildingGameObj *building,PhysicsSceneClass *scene,RenderObjClass *model,
						 const Matrix3D &world_transform,bool hidden)
{
	StaticPhysClass *phys = new StaticPhysClass;
	phys->Set_Model (model);
	phys->Set_Transform (world_transform);
	model->Release_Ref ();

	scene->Add_Static_Object (phys);
	phys->Set_Observer (building);

	if (hidden && (phys->Peek_Model () != nullptr)) {
		phys->Peek_Model ()->Set_Hidden (true);
	}

	return phys;
}

bool
BuildingGameObj::Build_Runtime_Components (const char *model_name)
{
	if ((model_name == nullptr) || (model_name[0] == 0)) {
		return false;
	}

	PhysicsSceneClass *scene = PhysicsSceneClass::Get_Instance ();
	if (scene == nullptr) {
		return false;
	}

	WW3DAssetManager *assets = WW3DAssetManager::Get_Instance ();
	if (assets == nullptr) {
		return false;
	}

	RenderObjClass *model = assets->Create_Render_Obj (model_name);
	if (model == nullptr) {
		WWDEBUG_SAY (("BuildingGameObj: no model %s for runtime descriptor %s; the building "
						  "is real but will not draw\n",model_name,RuntimeDescriptorName.Peek_Buffer ()));
		return false;
	}

	StaticPhysClass *phys = Add_Runtime_Part (this,scene,model,RuntimeTransform,RuntimeModelHidden);

	RuntimeModelName = model_name;
	ExteriorMeshes.Add (phys);
	RuntimeParts.Add (phys);
	WorldShadowManager::Register_Caster (phys);		// P20, as the assembly path does for its shell

	return true;
}

bool
BuildingGameObj::Build_Runtime_Assembly (const BuildingAssemblyDefinition &assembly)
{
	PhysicsSceneClass *scene = PhysicsSceneClass::Get_Instance ();
	if (scene == nullptr) {
		return false;
	}

	WW3DAssetManager *assets = WW3DAssetManager::Get_Instance ();
	if (assets == nullptr) {
		return false;
	}

	bool any_loaded = false;

	for (int i = 0; i < assembly.Parts.Count (); i++) {
		const BuildingAssemblyPart &part = assembly.Parts [i];
		if (!part.LoadByDefault) {
			continue;			// a night variant or authoring-only source -- never loaded at runtime
		}
		if (part.ModelName.Get_Length () == 0) {
			continue;
		}

		RenderObjClass *model = assets->Create_Render_Obj (part.ModelName.Peek_Buffer ());
		if (model == nullptr) {
			//	One missing part is a missing-art gap for that part, not a reason to abandon the
			//	rest of the building -- the same "behave sanely when absent" rule
			//	docs/assets/README.md states, applied per part instead of per building.
			WWDEBUG_SAY (("BuildingGameObj: no model %s for assembly part '%s' of runtime descriptor "
							  "%s; that part will not draw\n",part.ModelName.Peek_Buffer (),
							  part.Id.Peek_Buffer (),RuntimeDescriptorName.Peek_Buffer ()));
			continue;
		}

		Matrix3D part_local (1);
		part_local.Rotate_X (DEG_TO_RADF (part.RotationDegrees.X));
		part_local.Rotate_Y (DEG_TO_RADF (part.RotationDegrees.Y));
		part_local.Rotate_Z (DEG_TO_RADF (part.RotationDegrees.Z));
		part_local.Translate (part.Position);

		//	Measured from the model while this scope still owns a reference to it, before
		//	Add_Runtime_Part hands ownership over to the StaticPhysClass it constructs -- see
		//	docs/zerohour/BuildingComponentAssembly.md for why this is a measured vertical
		//	retract rather than a guessed slide or a baked animation nothing in the real asset
		//	confirms exists.  Only a "door"-kind part gets this; every other kind is unaffected.
		bool is_door = (part.Kind.Compare_No_Case ("door") == 0);
		Matrix3D open_local (1);
		Vector3 trigger_position (0.0f,0.0f,0.0f);
		float trigger_radius = 0.0f;
		if (is_door) {
			AABoxClass obj_bounds;
			model->Get_Obj_Space_Bounding_Box (obj_bounds);

			float rise = 2.0f * obj_bounds.Extent.Z * RuntimeDoorService::RISE_HEIGHT_MARGIN;
			open_local = part_local;
			open_local.Set_Translation (part_local.Get_Translation () + Vector3 (0.0f,0.0f,rise));

			trigger_radius		= std::max (obj_bounds.Extent.X,obj_bounds.Extent.Y) +
										  RuntimeDoorService::TRIGGER_RADIUS_MARGIN;
			trigger_position	= (RuntimeTransform * part_local).Get_Translation ();
		}

		StaticPhysClass *phys = Add_Runtime_Part (this,scene,model,RuntimeTransform * part_local,RuntimeModelHidden);

		RuntimeParts.Add (phys);
		if (part.Kind.Compare_No_Case ("interior") == 0) {
			InteriorMeshes.Add (phys);
		} else {
			//	P20: a runtime-built shell has no baked lighting and no stock object shadow, so
			//	it casts through the world shadow manager -- cached, regenerated only when it
			//	changes.  Interiors are inside the shell and would only cost a projector.
			WorldShadowManager::Register_Caster (phys);
			//	"exterior", "aggregate", "door", or anything else the manifest names -- every one
			//	of them is exterior-collision geometry by default, same as the single-mesh path.
			ExteriorMeshes.Add (phys);
		}

		if (is_door) {
			RuntimeDoorService::Register (this,i,phys,part_local,open_local,trigger_position,trigger_radius);
		}

		any_loaded = true;
	}

	return any_loaded;
}

void
BuildingGameObj::Release_Runtime_Components (void)
{
	//	Unconditional and before anything else, cheap no-op when nothing is registered -- the
	//	same "always call, never remember" pattern TeamPowerGridService::UnregisterBuilding
	//	already uses from the destructor.  Must run before the StaticPhysClass objects below are
	//	released, so the service is never left holding a pointer to one that no longer exists.
	RuntimeDoorService::Unregister_Building (Get_ID ());

	if (RuntimeParts.Count () == 0) {
		RuntimeModelName = "";
		return ;
	}

	PhysicsSceneClass *scene = PhysicsSceneClass::Get_Instance ();

	for (int i = 0; i < RuntimeParts.Count (); i++) {
		StaticPhysClass *phys = RuntimeParts [i];
		if (phys == nullptr) {
			continue;
		}

		if (phys->Get_Observer () == this) {
			phys->Set_Observer (nullptr);
		}
		WorldShadowManager::Unregister_Caster (phys);		// a no-op for a part that never cast
		//	A part is a member of exactly one of these two lists; Remove on the other is a safe
		//	no-op (RefMultiListClass::Remove only releases a reference when it actually removed
		//	something), so trying both avoids this function having to remember which.
		ExteriorMeshes.Remove (phys);
		InteriorMeshes.Remove (phys);

		if (scene != nullptr) {
			scene->Remove_Object (phys);
		}
		phys->Release_Ref ();
	}

	RuntimeParts.Delete_All ();
	RuntimeModelName	= "";
	return ;
}

void
BuildingGameObj::Derive_Runtime_Structure (void)
{
	Update_Bounding_Box ();

	MinMaxAABoxClass interior_bounds;
	interior_bounds.Init_Empty ();
	int interior_count = 0;
	int exterior_count = 0;

	//	Every currently-loaded part, not just one -- a component-assembly building's interior
	//	proxy and its aggregates are each their own hierarchy, and the announcement sphere has to
	//	be sized from all of them the same way the single-mesh path always sized it from one.
	for (int i = 0; i < RuntimeParts.Count (); i++) {
		RenderObjClass *model = (RuntimeParts [i] != nullptr) ? RuntimeParts [i]->Peek_Model () : nullptr;
		Walk_Runtime_Hierarchy (model,&interior_bounds,&interior_count,&exterior_count);
	}

	AABoxClass source = BoundingBox;
	if (interior_count > 0) {
		source.Init (interior_bounds);
	}

	AnnouncementSphere.Center	= source.Center;
	AnnouncementSphere.Radius	= std::max (source.Extent.X,source.Extent.Y);
	AnnouncementSphere.Radius	= std::max (AnnouncementSphere.Radius,source.Extent.Z);
	if (AnnouncementSphere.Radius <= 0.0f) {
		AnnouncementSphere.Radius = 1.0f;
	}

	return ;
}

bool
BuildingGameObj::Init_Runtime_Building (const char *descriptor_name,const Matrix3D &transform)
{
	if ((descriptor_name == nullptr) || (descriptor_name[0] == 0)) {
		return false;
	}

	StringClass wanted (descriptor_name,true);
	const BuildingModelDescriptor *descriptor = Peek_Runtime_Descriptor (wanted);
	if (descriptor == nullptr) {
		WWDEBUG_SAY (("BuildingGameObj: no descriptor %s; refusing to create a runtime "
						  "building with no model, damage table or capabilities\n",descriptor_name));
		return false;
	}

	Release_Runtime_Components ();

	IsRuntimeConstructed		= true;
	RuntimeDescriptorName	= wanted;
	RuntimeTransform			= transform;
	RuntimeDamageState		= RUNTIME_DAMAGE_STATE_UNSET;
	RuntimeAssemblyDestroyed = false;

	Set_Position (transform.Get_Translation ());

	CollectionSphere.Center	= Position;
	CollectionSphere.Radius	= 0.0f;

	Update_Runtime_Damage_Model (true);

	//
	//	The same two calls Collect_Building_Components ends with, for the same reasons: the
	//	building joins its base and the power grid, and its state machine runs once from a known
	//	starting point.  Keeping the tail identical is what makes the two paths interchangeable
	//	from every caller's point of view.
	//
	Initialize_Building ();
	Update_State (true);

	return true;
}

void
BuildingGameObj::Update_Runtime_Damage_Model (bool force_update)
{
	if (!IsRuntimeConstructed) {
		return ;
	}

	const BuildingModelDescriptor *descriptor = Peek_Runtime_Descriptor (RuntimeDescriptorName);
	if (descriptor == nullptr) {
		return ;
	}

	/*
	**	Q-017 follow-up -- component-assembly buildings (BuildingModelDescriptor::Assembly
	**	non-empty) take a different, simpler path than the single-mesh health-band table below.
	**	Nothing in the real asset drop the assembly blocks were built from has a damaged-state
	**	variant for any individual part -- only night variants, a different axis entirely -- so
	**	there is no honest health-band swap to make.  An assembly building therefore has exactly
	**	two visual states: assembled (built once, kept for the building's life) and destroyed,
	**	which reuses the single-mesh DamageVisual.DestroyedModel path (Build_Runtime_Components)
	**	unchanged -- "destroyed" was already a single whole-building model, assembly or not.
	*/
	if (descriptor->Assembly.Parts.Count () > 0) {
		bool wants_destroyed = IsDestroyed;

		if (force_update || (wants_destroyed != RuntimeAssemblyDestroyed)) {
			bool entering_destroyed = wants_destroyed && !RuntimeAssemblyDestroyed;

			Release_Runtime_Components ();
			if (wants_destroyed) {
				if (descriptor->DamageVisual.DestroyedModel.Get_Length () > 0) {
					Build_Runtime_Components (descriptor->DamageVisual.DestroyedModel.Peek_Buffer ());
				}
			} else {
				Build_Runtime_Assembly (descriptor->Assembly);
			}
			Derive_Runtime_Structure ();

			RuntimeAssemblyDestroyed = wants_destroyed;

			if (entering_destroyed || (force_update && wants_destroyed)) {
				On_Runtime_Destroyed_State_Entered ();
			}
		}

		RuntimeDamageState = wants_destroyed ? (int)BUILDING_DAMAGE_STATE_DESTROYED : 0;
		return ;
	}

	float health_max = DefenseObject.Get_Health_Max ();
	float normalized = (health_max > 0.0f) ? (DefenseObject.Get_Health () / health_max) : 0.0f;

	int wanted_state = IsDestroyed ? (int)BUILDING_DAMAGE_STATE_DESTROYED
											 : descriptor->DamageVisual.Find_State_Index (normalized);

	if (!force_update && (wanted_state == RuntimeDamageState)) {
		return ;
	}

	bool entering_destroyed = (wanted_state == (int)BUILDING_DAMAGE_STATE_DESTROYED) &&
									  (RuntimeDamageState != (int)BUILDING_DAMAGE_STATE_DESTROYED);

	RuntimeDamageState = wanted_state;

	StringClass wanted_model (descriptor->Get_Damage_Model (normalized,IsDestroyed),true);

	if (force_update || (wanted_model.Compare_No_Case (RuntimeModelName) != 0)) {
		Release_Runtime_Components ();
		Build_Runtime_Components (wanted_model.Peek_Buffer ());
		Derive_Runtime_Structure ();
	}

	if (entering_destroyed || (force_update && IsDestroyed)) {
		On_Runtime_Destroyed_State_Entered ();
	}

	return ;
}

void
BuildingGameObj::On_Runtime_Destroyed_State_Entered (void)
{
	if (CombatManager::I_Am_Server ()) {
		TeamPowerGridService::OnBuildingDestroyed (Get_ID ());
	}

	Update_Group_Lighting ();
	return ;
}

void
BuildingGameObj::Mark_Runtime_Door_State_Dirty (void)
{
	Set_Object_Dirty_Bit (NetworkObjectClass::BIT_RARE, true);
	return ;
}

void
BuildingGameObj::Set_Runtime_Model_Hidden (bool hidden)
{
	RuntimeModelHidden = hidden;

	//	Every part, not just the first -- a component-assembly building under construction has
	//	to hide its whole shell, not just whichever part happens to be RuntimeParts[0].
	for (int i = 0; i < RuntimeParts.Count (); i++) {
		RenderObjClass *model = (RuntimeParts [i] != nullptr) ? RuntimeParts [i]->Peek_Model () : nullptr;
		if (model != nullptr) {
			model->Set_Hidden (hidden);
		}
	}

	return ;
}

bool
BuildingGameObj::Resolve_Runtime_Anchor (const char *role_name,Matrix3D *out_transform) const
{
	if (!IsRuntimeConstructed || (out_transform == nullptr)) {
		return false;
	}

	if (IsDestroyed) {
		return false;
	}

	const BuildingModelDescriptor *descriptor = Peek_Runtime_Descriptor (RuntimeDescriptorName);
	if (descriptor == nullptr) {
		return false;
	}

	//	Every currently-loaded part's model, in build order -- a single-mesh building passes a
	//	one-element list, a component-assembly building passes every part, and the resolver
	//	tries each in turn until one of them actually carries the named bone.
	DynamicVectorClass<RenderObjClass *> part_models;
	for (int i = 0; i < RuntimeParts.Count (); i++) {
		RenderObjClass *model = (RuntimeParts [i] != nullptr) ? RuntimeParts [i]->Peek_Model () : nullptr;
		if (model != nullptr) {
			part_models.Add (model);
		}
	}

	return descriptor->Resolve_Anchor_World_Transform (part_models,RuntimeTransform,
																		 role_name,out_transform);
}


/*
**	Roadmap Section 21, Phase 20 -- functional subcomponents.
**
**	Deliberately looked up by Get_Definition().Get_Descriptor_Name() rather than
**	RuntimeDescriptorName/Peek_Runtime_Descriptor: that pair is "inert for a map-authored
**	building" by design (see the header's own comment on the runtime-constructed state block),
**	but DescriptorName is the definition's own back-link (Resolve_Game_Object_Presets, Q-017) and
**	resolves for *any* instance of a described definition, map-placed or Commander-built alike --
**	the same resolution TeamPowerGridService::Peek_Descriptor already performs from the member's
**	own copy of this same name.  Health-driven degradation is not a runtime-construction-only
**	concept; only the anchor/bone status report below is, because only a runtime building's parts
**	live in RuntimeParts.
*/
static float
Effective_Subcomponent_Fraction (const BuildingModelDescriptor *descriptor,
											 BuildingSubcomponentRoleType role,float health_fraction)
{
	if (descriptor == nullptr) return 1.0f;
	float fraction = descriptor->Get_Subcomponent_Function_Fraction (role,health_fraction);
	return (fraction < 0.0f) ? 1.0f : fraction;		// not tracked -- unmodified, same as before Phase 20
}

float
BuildingGameObj::Get_Effective_Power_Generation (void) const
{
	const BuildingModelDescriptor *descriptor =
		BuildingDescriptorManager::Get_Descriptor (Get_Definition ().Get_Descriptor_Name ().Peek_Buffer ());
	if (descriptor == nullptr) return 0.0f;

	float health_max = DefenseObject.Get_Health_Max ();
	float normalized = (health_max > 0.0f) ? (DefenseObject.Get_Health () / health_max) : 0.0f;

	return descriptor->Power.Generation *
			 Effective_Subcomponent_Fraction (descriptor,SUBCOMPONENT_POWER_GENERATOR_MODULE,normalized);
}

float
BuildingGameObj::Get_Effective_Vision_Radius (void) const
{
	const BuildingModelDescriptor *descriptor =
		BuildingDescriptorManager::Get_Descriptor (Get_Definition ().Get_Descriptor_Name ().Peek_Buffer ());
	if (descriptor == nullptr) return 0.0f;

	float health_max = DefenseObject.Get_Health_Max ();
	float normalized = (health_max > 0.0f) ? (DefenseObject.Get_Health () / health_max) : 0.0f;

	return descriptor->Communications.VisionRadius *
			 Effective_Subcomponent_Fraction (descriptor,SUBCOMPONENT_COMMUNICATIONS_ARRAY,normalized);
}

float
BuildingGameObj::Get_Effective_Radar_Radius (void) const
{
	const BuildingModelDescriptor *descriptor =
		BuildingDescriptorManager::Get_Descriptor (Get_Definition ().Get_Descriptor_Name ().Peek_Buffer ());
	if (descriptor == nullptr) return 0.0f;

	float health_max = DefenseObject.Get_Health_Max ();
	float normalized = (health_max > 0.0f) ? (DefenseObject.Get_Health () / health_max) : 0.0f;

	//	"destroyed radar dish -> radar capability disabled while building may remain alive" --
	//	the linear fraction reaching 0 at the descriptor's own disabled_health_fraction already is
	//	that disable, expressed as a zero-radius sensor rather than a second boolean to keep in
	//	sync with it.
	return descriptor->Communications.RadarRadius *
			 Effective_Subcomponent_Fraction (descriptor,SUBCOMPONENT_RADAR_DISH,normalized);
}

/*
**	D-001 / V2 Phase 20: which descriptor describes this building's parts.  A runtime building
**	names its own; a map-authored one is described through its definition's DescriptorName, the
**	same name every other descriptor consumer (power, comms) already reads for it.
*/
static const BuildingModelDescriptor *
Peek_Subcomponent_Descriptor (bool runtime,const StringClass &runtime_name,const StringClass &definition_name)
{
	if (runtime) return Peek_Runtime_Descriptor (runtime_name);
	if (definition_name.Get_Length () == 0) return nullptr;
	return BuildingDescriptorManager::Get_Descriptor (definition_name.Peek_Buffer ());
}

static bool
Model_Has_Bone (StaticPhysClass *phys,const char *bone_name)
{
	RenderObjClass *model = (phys != nullptr) ? phys->Peek_Model () : nullptr;
	return (model != nullptr) && (model->Get_Bone_Index (bone_name) != 0);
}

int
BuildingGameObj::Get_Subcomponent_Count (void) const
{
	const BuildingModelDescriptor *descriptor =
		Peek_Subcomponent_Descriptor (IsRuntimeConstructed,RuntimeDescriptorName,Get_Definition ().Get_Descriptor_Name ());
	if (descriptor == nullptr) return 0;
	return descriptor->Subcomponents.Count ();
}

bool
BuildingGameObj::Get_Subcomponent_Status (int index,StringClass *role_name_out,
														 float *function_fraction_out,bool *anchor_resolves_out) const
{
	const BuildingModelDescriptor *descriptor =
		Peek_Subcomponent_Descriptor (IsRuntimeConstructed,RuntimeDescriptorName,Get_Definition ().Get_Descriptor_Name ());
	if (descriptor == nullptr) return false;
	if ((index < 0) || (index >= descriptor->Subcomponents.Count ())) return false;

	const BuildingSubcomponentDefinition & sub = descriptor->Subcomponents [index];

	if (role_name_out != nullptr) {
		const char *name = Subcomponent_Role_Name (sub.Role);
		*role_name_out = (name != nullptr) ? name : "(unknown)";
	}

	if (function_fraction_out != nullptr) {
		float health_max = DefenseObject.Get_Health_Max ();
		float normalized = (health_max > 0.0f) ? (DefenseObject.Get_Health () / health_max) : 0.0f;
		*function_fraction_out = Effective_Subcomponent_Fraction (descriptor,sub.Role,normalized);
	}

	if (anchor_resolves_out != nullptr) {
		/*
		**	Direct bone lookup, not Resolve_Runtime_Anchor: a subcomponent's AnchorBoneName is
		**	already the W3D bone name itself, not an Anchors-table role that indirects to one --
		**	the same Get_Bone_Index scan Resolve_Anchor_World_Transform runs, done here against
		**	the one name in hand instead of a role lookup that would never find it.
		*/
		bool resolved = false;
		const char *bone = sub.AnchorBoneName.Peek_Buffer ();
		if (sub.AnchorBoneName.Get_Length () > 0) {
			if (IsRuntimeConstructed) {
				for (int i = 0; (i < RuntimeParts.Count ()) && !resolved; i++) {
					resolved = Model_Has_Bone (RuntimeParts [i],bone);
				}
			} else {
				//	A map-authored building has no RuntimeParts: its hierarchy is whatever
				//	Collect_Building_Components claimed from the level -- interior and exterior
				//	statics and the aggregates -- so the bone is looked for there.
				RefMultiListIterator<StaticPhysClass> interior (const_cast<RefMultiListClass<StaticPhysClass> *> (&InteriorMeshes));
				for (interior.First (); !interior.Is_Done () && !resolved; interior.Next ()) {
					resolved = Model_Has_Bone (interior.Peek_Obj (),bone);
				}
				RefMultiListIterator<StaticPhysClass> exterior (const_cast<RefMultiListClass<StaticPhysClass> *> (&ExteriorMeshes));
				for (exterior.First (); !exterior.Is_Done () && !resolved; exterior.Next ()) {
					resolved = Model_Has_Bone (exterior.Peek_Obj (),bone);
				}
				RefMultiListIterator<BuildingAggregateClass> aggregates (const_cast<RefMultiListClass<BuildingAggregateClass> *> (&Aggregates));
				for (aggregates.First (); !aggregates.Is_Done () && !resolved; aggregates.Next ()) {
					resolved = Model_Has_Bone (aggregates.Peek_Obj (),bone);
				}
			}
		}
		*anchor_resolves_out = resolved;
	}

	return true;
}


bool
BuildingGameObj::Are_Capabilities_Enabled (void) const
{
	if (IsDestroyed) {
		return false;
	}

	if (BuildingConstructionService::Is_Under_Construction (Get_ID ())) {
		return false;
	}

	return true;
}