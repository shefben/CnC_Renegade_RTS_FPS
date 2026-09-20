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
 *                     $Archive:: /Commando/Code/Combat/building.h                            $*
 *                                                                                             *
 *                      $Author:: Patrick                                                     $*
 *                                                                                             *
 *                     $Modtime:: 1/07/02 3:52p                                               $*
 *                                                                                             *
 *                    $Revision:: 42                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef	BUILDING_H
#define	BUILDING_H

#include "always.h"
#include "damageablegameobj.h"
#include "combatphysobserver.h"
#include "buildingaggregate.h"
#include "lightphys.h"
#include "playertype.h"
#include "aabox.h"
#include "matrix3d.h"
#include "wwstring.h"

/*
** Forward declarations
*/
class AudibleSoundClass;
class BaseControllerClass;
class BuildingMonitorClass;
class PowerPlantGameObj;
class SoldierFactoryGameObj;
class VehicleFactoryGameObj;
class AirStripGameObj;
class WarFactoryGameObj;
class RefineryGameObj;
class ComCenterGameObj;
class RepairBayGameObj;
class ConstructionYardGameObj;
class SuperweaponGameObj;
class RayCollisionTestClass;
class CompositeAttachmentClass;
class StaticPhysClass;
class RenderObjClass;
struct BuildingAssemblyDefinition;


/**
** BuildingConstants
** Convienent namespace declaration for the constants used with buildings
*/
namespace BuildingConstants
{
	typedef enum
	{
		TYPE_NONE					= -1,
		TYPE_POWER_PLANT,
		TYPE_SOLDIER_FACTORY,
		TYPE_VEHICLE_FACTORY,
		TYPE_REFINERY,
		TYPE_COM_CENTER,
		TYPE_REPAIR_BAY,
		TYPE_SHRINE,
		TYPE_HELIPAD,
		TYPE_CONYARD,
		TYPE_BASE_DEFENSE,
		TYPE_COUNT
	} BuildingType;

	typedef enum
	{
		LEGACY_TEAM_GDI						= 0,
		LEGACY_TEAM_NOD,
	} LegacyBuildingTeam;

	enum
	{
		BASE_COUNT = 2,
	};
}

/**
** Building Game Obj Def
** This class is an editable definition for a building.  It contains all constant data needed to initialize
** a BuildingGameObj.
*/
class BuildingGameObjDef : public DamageableGameObjDef
{
public:

	BuildingGameObjDef( void );

	virtual uint32								Get_Class_ID (void) const override;
	virtual PersistClass *						Create( void ) const override;
	virtual bool								Save( ChunkSaveClass &csave ) override;
	virtual bool								Load( ChunkLoadClass &cload ) override;
	virtual const PersistFactoryClass &	Get_Factory( void ) const override;

	DECLARE_EDITABLE( BuildingGameObjDef, DamageableGameObjDef );

	void										Set_Type (BuildingConstants::BuildingType type) { Type = type; }
	BuildingConstants::BuildingType				Get_Type (void) const	{ return Type; }

	const StringClass &							Get_Mesh_Prefix (void) const	{ return MeshPrefix; }

	//
	//	Roadmap Section 40 -- the key into data/building_descriptors/*.json
	//	(BuildingDescriptorManager) this preset's instances should register against with
	//	TeamPowerGridService.  Empty by default: a preset that names none stays exactly as
	//	powered as it always was, since TeamPowerGridService treats an unresolved name as a
	//	neutral no-op member (see teampowergridservice.cpp).
	//
	const StringClass &							Get_Descriptor_Name (void) const	{ return DescriptorName; }

	//
	//	Q-017 -- the reverse link.  A stock or project-owned preset loaded with no authoring
	//	tool ever touching it has no way to set DescriptorName through the editable-parameter
	//	system, so BuildingDescriptorManager::Resolve_Game_Object_Presets sets it directly here
	//	once it has matched a descriptor's game_object_preset back to this definition.
	//
	void											Set_Descriptor_Name (const char *name)	{ DescriptorName = name; }

	int Get_Damage_Report(int team) const;
	int Get_Destroy_Report(int team) const;

	//
	//	When set, this building is left out of the team battlefield
	//	information screen -- it exists on the map but the team is not told
	//	about its health or even that it is there.
	//
	bool Get_Hide_Team_Battlefield_Information (void) const	{ return HideTeamBattlefieldInformation; }

protected:

	StringClass									MeshPrefix;
	StringClass									DescriptorName;
	ArmorType									MCTSkin;
	BuildingConstants::BuildingType				Type;

	int GDIDamageReportID;
	int NodDamageReportID;
	int GDIDestroyReportID;
	int NodDestroyReportID;
	bool HideTeamBattlefieldInformation;

	friend	class								BuildingGameObj;
};

/*
** Building Game Obj
** This class encapsulates the basic building functionality.
**
** At initialization, the following things need to happen.
** - Collects all of the meshes that are part of the building using proximity, prefix matching
**   and the "house" naming convention (# indicates an interior mesh, ^ indicates an exterior mesh).
** - Collect all of the BuildingAggregates that are part of the building using proximity and
**   prefix matching.
** - Collect all light sources associated with the building using proximity and prefix matching.
** - Install itself as an observer into each StaticPhysClass (meshes and aggregates) that is part
**   of the building.
**
** During gameplay, the building code does the following things:
** - Responds to all bullet collisions and explosions on any of its meshes/aggregates by
**   applying damage to the building.
** - When destroyed, applies the alternate materials on its exterior meshes
** - When the power goes out, applies the alternate materials on its interior meshes
** - When the state changes (power on/off, health 100,75,50,25,0) switches the behaviors of
**   all aggregates.
**
*/
class BuildingGameObj : public DamageableGameObj, public CombatPhysObserverClass
{
public:
	//	Constructor and Destructor
	BuildingGameObj( void );
	virtual	~BuildingGameObj( void );

	// Definitions
	virtual	void	Init( void ) override;
	void	Init( const BuildingGameObjDef & definition );
	const BuildingGameObjDef & Get_Definition( void ) const ;

	// Save / Load
	virtual	bool	Save( ChunkSaveClass & csave ) override;
	virtual	bool	Load( ChunkLoadClass & cload ) override;
	virtual	const	PersistFactoryClass & Get_Factory( void ) const override;

	// Type identification
	virtual	BuildingGameObj	*		As_BuildingGameObj( void ) override			{ return this; }
	virtual	DamageableGameObj	*		As_DamageableGameObj( void ) override		{ return this; }
	virtual	PowerPlantGameObj *		As_PowerPlantGameObj (void)		{ return nullptr; }
	virtual	SoldierFactoryGameObj *	As_SoldierFactoryGameObj (void)	{ return nullptr; }
	virtual	VehicleFactoryGameObj *	As_VehicleFactoryGameObj (void)	{ return nullptr; }
	virtual	ConstructionYardGameObj *	As_ConstructionYardGameObj (void)	{ return nullptr; }
	virtual	SuperweaponGameObj *		As_SuperweaponGameObj (void)		{ return nullptr; }
	virtual	AirStripGameObj *			As_AirStripGameObj (void)			{ return nullptr; }
	virtual	WarFactoryGameObj *		As_WarFactoryGameObj (void)		{ return nullptr; }
	virtual	RefineryGameObj *			As_RefineryGameObj (void)			{ return nullptr; }
	virtual	ComCenterGameObj *		As_ComCenterGameObj (void)			{ return nullptr; }
	virtual	RepairBayGameObj *		As_RepairBayGameObj (void)			{ return nullptr; }

	/****/

	/*
	** Position interface, the position of the building controller influences which meshes
	** and other components are automatically assigned to this building.  When there are two
	** BuildingGameObj's that can accept a particular component, the closest one wins.
	*/
	void											Get_Position(Vector3 * pos) const override		{ *pos = Position; }
	void											Set_Position(const Vector3 & pos)		{ Position = pos; CollectionSphere.Center = pos; }

	/*
	** Damage interface, designed to work similarly to a game object
	*/
	virtual	void								Apply_Damage( const OffenseObjectClass & damager,
																	  float scale = 1.0f,
																	  int alternate_skin = -1 ) override;
	void											Apply_Damage_Building(	const OffenseObjectClass & offense,
																				StaticPhysClass * component );
	void											Apply_Damage_Building(	const OffenseObjectClass & offense,
																				bool mct_damage );
	void											Set_Normalized_Health (float health);

	/*
	** Power interface, turn the power on and off
	*/
	void											Enable_Power(bool onoff);
	bool											Is_Power_Enabled(void) const			{ return IsPowerOn; }

	/*
	** Building announcment support
	*/
	void											Play_Announcement( int text_id, bool broadcast );
	void											Stop_Current_Announcement( void );

	/*
	** Utility functions for the building mesh naming convention.
	** Exterior meshes contain a ^ as the first character following their prefix.
	** Interior meshes contain a # as the first character following their prefix.
	*/
	bool											Name_Prefix_Matches_This_Building(const char * name);
	static bool									Is_Interior_Mesh_Name(const char * name);
	static bool									Is_Exterior_Mesh_Name(const char * name);
	const char *								Get_Name_Prefix (void) const { return Get_Definition().MeshPrefix; }

	/*
	** Building component support
	*/
	void											Collect_Building_Components (void);
	void											Get_Collection_Sphere (SphereClass *sphere) const	{ *sphere = CollectionSphere; }
	void											Set_Collection_Sphere (const SphereClass &sphere)	{ CollectionSphere = sphere; }


	/*
	** CnC mode suport
	*/
	virtual void								CnC_Initialize (BaseControllerClass *base);
	virtual void								On_Destroyed (void);
	virtual void								On_Damaged (void);
	virtual void								On_Revived (void);
	bool											Is_Destroyed (void) const { return IsDestroyed; }
	void											Set_Is_Destroyed (bool onoff);

	/*
	**	Roadmap Section 52A, Phase 50.  The other half of lightphys.h's own "the building code
	**	uses the Group ID..." comment: any LightPhysClass whose Group ID was authored (or set)
	**	to equal this building's own Get_ID() follows this building's power/destroyed state.
	**	Called from Enable_Power, On_Destroyed and On_Revived -- every place IsPowerOn or
	**	IsDestroyed actually changes -- never on a timer.
	*/
	void											Update_Group_Lighting (void);

	//
	//	Spy-disabled buildings are marked by gameplay scripts; the flag is
	//	replicated and persisted, but the engine imposes no behaviour of its
	//	own on it.
	//
	bool											Is_Spy_Disabled (void) const			{ return IsSpyDisabled; }
	void											Set_Is_Spy_Disabled (bool onoff)		{ IsSpyDisabled = onoff; }

	/*
	** Network support
	*/
   virtual	void								Import_Rare (BitStreamClass &packet) override;
   virtual	void								Export_Rare (BitStreamClass &packet) override;
	virtual	void								Export_Creation (BitStreamClass &packet) override;
	virtual	void								Import_Creation (BitStreamClass &packet) override;
	static void									Set_Precision (void);
	virtual bool								Get_World_Position (Vector3 &pos) const	override { pos = Position; return true; }
	//virtual float								Compute_Object_Priority (int client_id, const Vector3 &client_pos);
	//virtual	void								Get_Extended_Information( StringClass & description );
	virtual	void								Get_Description( StringClass & description ) override;
	virtual bool								Is_Tagged(void) override									{ return false; }


	/*
	**
	*/
	bool	Is_GDI( void )						{ return Get_Player_Type() == PLAYERTYPE_GDI; }
	bool	Is_Nod( void )						{ return Get_Player_Type() == PLAYERTYPE_NOD; }

	static void	Set_Can_Repair_Buildings(bool flag)		{CanRepairBuildings = flag;}
	static bool	Get_Can_Repair_Buildings(void)			{return CanRepairBuildings;}

	//
	//	Informational
	//
	void											Find_Closest_Poly (const Vector3 &pos, float *distance2);
	float											Find_Closest_Poly (const Vector3 &pos)	{ float distance2 = 0; Find_Closest_Poly (pos, &distance2); return distance2; }
	void											Find_Closest_Poly (const Vector3 &pos, float *distance2, Vector3 *poly_pos);

	//
	//	Proximity and ray queries against the building's collected geometry.
	//	Building_In_Range answers 0 for no, 1 for yes and 2 for the MCT being in
	//	range; Is_In_Range_Coarse is the cheap bounding-box rejection it opens
	//	with, and is worth calling directly before any per-polygon work.
	//
	int											Building_In_Range (const Vector3 &point, float range);
	bool											Is_In_Range_Coarse (const Vector3 &point, float range_sq) const;
	bool											Cast_Ray (RayCollisionTestClass &raytest);
	BuildingAggregateClass *				Find_MCT (void);

	//
	//	Bounding box enclosing every collected component.  A building is an
	//	aggregate of static meshes with no model of its own, so this is what
	//	callers use in place of Peek_Model()->Get_Bounding_Box().
	//
	const AABoxClass &						Get_Bounds (void) const					{ return BoundingBox; }

	//
	//	Roadmap Section 45 -- where a Commander purchase at this facility appears, and the
	//	radius around that point that has to be clear before one more can.  A legacy
	//	map-placed building (this class) has no authored exit/spawn anchor -- that is Section
	//	44's semantic-node concept, and it needs a single runtime model this class's aggregate
	//	of separately-placed statics does not have (see Q-016 in awaiting_answers.md).  So this
	//	is an approximation from the one thing every building already has regardless: its own
	//	collected bounding box, stepped outside its footprint by its circumscribing radius plus
	//	a clearance margin.  One formula, shared by the caller that spawns a unit here
	//	(Code/Combat/combat.cpp) and the caller that asks first whether it is clear to
	//	(Code/Commando/commanderproductionevent.cpp) -- neither invents its own.
	//
	void											Get_Commander_Exit_Point (Vector3 * out_point,float * out_radius) const;

	//
	//	Roadmap Section 29 -- the same generalized attachment layer vehicles use
	//	(Code/Combat/compositeattachment.h), reached the way a building has to reach it.  A
	//	building has no model of its own, so there is no single host to attach to: the bone
	//	names a place on one of the collected components, and Attach_Visual_Module finds
	//	whichever aggregate or exterior mesh actually carries that bone.  This is what makes
	//	the layer consumable by the building semantic descriptor system, which addresses
	//	building geometry by W3D node name rather than by model.
	//
	/*
	**	Roadmap Section 44, Q-016 -- the second component collection path.
	**
	**	`Collect_Building_Components` above claims already-placed statics near this building by a
	**	naming convention.  That is how every map-authored building in this engine works and it is
	**	left exactly as it was.  A Commander-constructed building has no scatter to claim: it is a
	**	single `.w3d` named by its descriptor, loaded at runtime and put into the scene as one
	**	`StaticPhysClass`.  Everything the legacy path derives from a proximity scan -- bounds,
	**	the announcement sphere, which components carry damage, where the named anchors are -- this
	**	path derives from that model's own hierarchy instead.
	**
	**	The two paths never run for the same building: `Is_Runtime_Constructed` decides which one
	**	`Import_Creation` and the damage/state machinery take, and a runtime building's collection
	**	sphere is deliberately zero-radius so it can never claim a map static, nor be claimed.
	*/
	bool											Init_Runtime_Building (const char *descriptor_name,const Matrix3D &transform);
	bool											Is_Runtime_Constructed (void) const		{ return IsRuntimeConstructed; }
	const StringClass &						Get_Runtime_Descriptor_Name (void) const	{ return RuntimeDescriptorName; }
	const Matrix3D &							Get_Runtime_Transform (void) const		{ return RuntimeTransform; }
	RenderObjClass *							Peek_Runtime_Model (void) const;
	int											Get_Runtime_Damage_State (void) const	{ return RuntimeDamageState; }

	//	Section 44's "real model hidden ... real model revealed".  Hiding leaves the physics
	//	object in the scene, because the footprint is occupied from the moment construction
	//	starts -- only the drawing stops.
	void											Set_Runtime_Model_Hidden (bool hidden);
	bool											Is_Runtime_Model_Hidden (void) const	{ return RuntimeModelHidden; }

	//	A named semantic anchor (`mct_anchor`, `pct_anchor`, ...) resolved against the runtime
	//	model's own hierarchy.  Refuses every anchor once the building is destroyed, which is
	//	Q-016's "MCT/PCT ... disabled when the destroyed state is entered" expressed where the
	//	anchors are actually read rather than as a flag each caller has to remember.
	bool											Resolve_Runtime_Anchor (const char *role_name,Matrix3D *out_transform) const;

	//	RuntimeDoorService's one-liner: it owns the door open/close state (a component-assembly
	//	door part is server-authoritative timed state, the same shape BuildingConstructionService
	//	already established), but only a GameObj can dirty itself over the network, so the
	//	service calls back into this rather than reaching into a private bit of BuildingGameObj.
	void											Mark_Runtime_Door_State_Dirty (void);

	//	The one question every capability asks: does this building do its job right now?  False
	//	while destroyed, and false for a runtime building whose model has not been revealed yet.
	bool											Are_Capabilities_Enabled (void) const;

	bool											Attach_Visual_Module (const char *slot_name,const char *bone_name,const char *model_name);
	void											Detach_Visual_Module (const char *slot_name);
	void											Set_Visual_Module_Damaged (const char *slot_name,bool damaged);

protected:

	/*
	**	CnC mode stuff
	*/
	BuildingMonitorClass *					BuildingMonitor;
	BaseControllerClass *					BaseController;
	bool											IsDestroyed;
	bool											IsSpyDisabled;

//private:

	Vector3										Position;
	bool											IsPowerOn;
	int											CurrentState;						// derived from the health and IsPowerOn

	AudibleSoundClass *						CurrentAnnouncement;
	SphereClass									AnnouncementSphere;
	SphereClass									CollectionSphere;
	AABoxClass									BoundingBox;

	RefMultiListClass<StaticPhysClass>				InteriorMeshes;
	RefMultiListClass<StaticPhysClass>				ExteriorMeshes;
	RefMultiListClass<BuildingAggregateClass>		Aggregates;
	RefMultiListClass<LightPhysClass>				PowerOnLights;
	RefMultiListClass<LightPhysClass>				PowerOffLights;

	//	Null until the first Attach_Visual_Module call.  See compositeattachment.h.
	CompositeAttachmentClass *							AttachmentController;

	//	"no state chosen yet", distinct from BUILDING_DAMAGE_STATE_DESTROYED which is a real one.
	enum { RUNTIME_DAMAGE_STATE_UNSET = -2 };

	//	Runtime-constructed state.  All of it is inert for a map-authored building.
	bool											IsRuntimeConstructed;
	bool											RuntimeModelHidden;
	StringClass									RuntimeDescriptorName;
	StringClass									RuntimeModelName;			// single-mesh path: what is currently worn
	Matrix3D										RuntimeTransform;
	int											RuntimeDamageState;		// index into the descriptor's table

	//	Q-017 follow-up -- every scene object this runtime building currently owns, single-mesh
	//	path or component assembly alike: one entry for the former, one per loaded part for the
	//	latter.  ExteriorMeshes/InteriorMeshes (above) categorize the same objects by kind for
	//	every other consumer that already reads those lists (Update_Bounding_Box,
	//	Enable_Alternate_Materials); this is the ownership list Release_Runtime_Components tears
	//	down and Peek_Runtime_Model/Resolve_Runtime_Anchor read from.
	DynamicVectorClass<StaticPhysClass *>	RuntimeParts;

	//	Component-assembly buildings only (BuildingModelDescriptor::Assembly non-empty): whether
	//	the currently-built parts are the assembly or the single DamageVisual.DestroyedModel
	//	substitute -- see Update_Runtime_Damage_Model's assembly branch.
	bool											RuntimeAssemblyDestroyed;

	static bool									CanRepairBuildings;

	void											Initialize_Building(void);

	void											Reset_Components(void);
	void											Add_Mesh(StaticPhysClass * terrain);
	void											Update_Bounding_Box (void);
	void											Remove_Mesh(StaticPhysClass * terrain);
	void											Add_Aggregate(BuildingAggregateClass * aggregate);
	void											Remove_Aggregate(BuildingAggregateClass * aggregate);
	void											Add_Light(LightPhysClass * light);
	void											Find_Closest_Poly_For_Model (RenderObjClass *model, const Vector3 &pos, float *distance2, Vector3 *poly_pos = nullptr);

	//	The runtime path's halves: put the model in the world, take it back out, derive the
	//	structure the legacy path would have scanned for, and swap the model when the health
	//	threshold the descriptor named is crossed.
	bool											Build_Runtime_Components (const char *model_name);

	//	Q-017 follow-up -- the component-assembly sibling of Build_Runtime_Components above:
	//	one call per BuildingModelDescriptor::Assembly part instead of one call total.  Shares
	//	Release_Runtime_Components with the single-mesh path -- both populate the same
	//	RuntimeParts ownership list, so tearing either down is one function.
	bool											Build_Runtime_Assembly (const BuildingAssemblyDefinition &assembly);
	void											Release_Runtime_Components (void);
	void											Derive_Runtime_Structure (void);
	void											Update_Runtime_Damage_Model (bool force_update);
	void											On_Runtime_Destroyed_State_Entered (void);

	void											Update_State(bool force_update = false);
	void											Enable_Alternate_Materials(RefMultiListClass<StaticPhysClass> & models, bool onoff);
	void											Enable_Alternate_Materials(RenderObjClass * model,bool onoff);

	friend class GameObjManager;
};

#endif // BUILDING_H

