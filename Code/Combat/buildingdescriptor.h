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
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/buildingdescriptor.h                  $*
 *                                                                                             *
 *	Roadmap Section 39, Phase 33 -- building model semantic JSON + live building definitions.    *
 *                                                                                             *
 *	The W3D stays the geometry/hierarchy source; JSON supplies gameplay semantics on top of      *
 *	it, never mesh data.  This is the scoped-down honest version of the roadmap's own seven-     *
 *	step load pipeline: the full ask -- eagerly enumerate every subobject in a model's           *
 *	hierarchy, capture every node's transform and bounds, and validate every named node exists   *
 *	-- is a much larger integration with the W3D asset pipeline than this pass has time to       *
 *	build well.  What is built for real is the part Phase 34 (the power grid) and Phase 38       *
 *	(procedural construction) actually depend on: a validated descriptor whose power,             *
 *	capability and construction-visual values are queryable, and whose semantic anchors resolve  *
 *	*by bone name* against the real model on demand -- "never replace model-node semantics with  *
 *	hard-coded mapper world coordinates" is honoured by resolving late, through the model's own   *
 *	`HTree`, the same principle Section 29's `CompositeAttachmentClass` already established      *
 *	("the bone name is the transform, not a second implementation of what HTree already does").   *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef BUILDINGDESCRIPTOR_H
#define BUILDINGDESCRIPTOR_H

#include "always.h"
#include "matrix3d.h"
#include "vector.h"
#include "wwstring.h"

class RenderObjClass;


struct BuildingPowerDefinition
{
	BuildingPowerDefinition(void) : RequiresPower(true),Generation(0.0f),Consumption(0.0f),Priority(0) { }
	bool	RequiresPower;
	float	Generation;
	float	Consumption;
	int		Priority;			// deterministic load-shedding priority; higher stays powered first
};

struct BuildingCommunicationsCapability
{
	BuildingCommunicationsCapability(void) : Enabled(false),VisionRadius(0.0f),RadarRadius(0.0f),
															LastKnownRetentionSeconds(10.0f),RequiresBuildingOnline(true) { }
	bool	Enabled;
	float	VisionRadius;
	float	RadarRadius;
	float	LastKnownRetentionSeconds;
	bool	RequiresBuildingOnline;
};

struct BuildingSiloCapability
{
	BuildingSiloCapability(void) : Enabled(false),WalletCapacityBonusPerPlayer(0.0f),RequiresBuildingOnline(true) { }
	bool	Enabled;
	float	WalletCapacityBonusPerPlayer;
	bool	RequiresBuildingOnline;
};

struct BuildingConstructionVisualDefinition
{
	BuildingConstructionVisualDefinition(void) : DurationSeconds(15.0f),BlockWidth(2.0f),BlockDepth(2.0f),
																 RowHeight(1.5f),SourceRevealStart(0.75f),BlockFadeStart(0.9f),
																 MaxBlocks(2048) { }
	float	DurationSeconds;
	float	BlockWidth;
	float	BlockDepth;
	float	RowHeight;
	float	SourceRevealStart;
	float	BlockFadeStart;
	int		MaxBlocks;
};

struct BuildingFootprintDefinition
{
	BuildingFootprintDefinition(void) : Width(0.0f),Depth(0.0f) { }
	float	Width;
	float	Depth;
};

/*
**	Section 42's Commander sidebar needs a card's cost, icon, name and availability, and
**	Section 44 needs a build time.  None of that lives anywhere in this codebase today:
**	stock Renegade has no buildable-structure concept at all, so there is no canonical
**	owner to merge with, and Section 39's own rule -- "do not hard-code Power Plant output
**	or building draw in C++" -- applies just as much to a building's price as to its power.
**	So it is one more JSON block, read exactly like the others.
**
**	`Team` is a PLAYERTYPE_* value, or -1 for "any faction may build this".  `IconTexture`
**	names an existing Purchase Terminal image wherever one exists (Section 42, "reuse
**	existing Renegade Purchase Terminal character/vehicle images whenever available"); a
**	descriptor that names none gets a card with no icon, never a missing-texture crash.
*/
struct BuildingBuildDefinition
{
	BuildingBuildDefinition(void) : Buildable(false),Cost(0),BuildTimeSeconds(0.0f),TechLevel(0),
												Team(-1),DisplayName(""),IconTexture("") { }
	bool			Buildable;
	int			Cost;
	float			BuildTimeSeconds;		// <= 0 -> fall back to ConstructionVisual.DurationSeconds
	int			TechLevel;
	int			Team;						// PLAYERTYPE_*; -1 means any
	StringClass	DisplayName;
	StringClass	IconTexture;
};

/*
**	Which contextual sidebar mode clicking this building switches to (Section 42), and
**	which production queue Section 45 binds to it.  Deliberately one enumerated kind rather
**	than three booleans: "Airstrip configured as ground factory" is a real case in the
**	roadmap's own list, and it is the descriptor that decides, not the model name.
*/
enum BuildingProductionKindType
{
	BUILDING_PRODUCTION_NONE = 0,
	BUILDING_PRODUCTION_INFANTRY,
	BUILDING_PRODUCTION_GROUND_VEHICLE,
	BUILDING_PRODUCTION_AIR_VEHICLE,
};

struct BuildingProductionDefinition
{
	BuildingProductionDefinition(void) : Kind(BUILDING_PRODUCTION_NONE),QueueCapacity(5),
													 RequiresPower(true) { }
	BuildingProductionKindType	Kind;
	int								QueueCapacity;
	bool								RequiresPower;
};

/*
**	Section 43's placement checks that are per-definition rather than per-world: the slope a
**	foundation tolerates, and how much clear ground a production exit needs in front of it.
*/
struct BuildingPlacementRules
{
	BuildingPlacementRules(void) : MaxSlopeDegrees(15.0f),ExitClearance(0.0f),
											 RequiresFoundationSupport(true) { }
	float	MaxSlopeDegrees;
	float	ExitClearance;
	bool	RequiresFoundationSupport;
};

/*
**	Roadmap Section 44 / Q-016 -- damage visuals for a *runtime-constructed* building.
**
**	A map-authored building expresses damage by swapping alternate materials across the
**	scattered statics `Collect_Building_Components` claimed (see building.cpp,
**	`Enable_Alternate_Materials`).  A Commander-constructed building has no such scatter: it is
**	one loaded model, so its damage states are whole models named by data, not a material flag.
**	That is the whole reason this table exists rather than the legacy path being reused.
**
**	`States` is the living table, sorted descending by MinHealthFraction, and its last entry is
**	always 0.0 so a living building always resolves to something.  `MinHealthFraction` is the
**	bottom of that state's band: a state holds while health is at or above it and no earlier
**	state's band applies.  `DestroyedModel` is separate because "destroyed" is not a health
**	threshold -- a building sitting at exactly zero health that has not yet been declared
**	destroyed is still the last living state, and a building a script kills outright at full
**	health is rubble regardless of what the table says.
*/
struct BuildingDamageStateEntry
{
	BuildingDamageStateEntry(void) : ModelName(""),MinHealthFraction(0.0f) { }
	bool	operator == (const BuildingDamageStateEntry &) const	{ return false; }
	bool	operator != (const BuildingDamageStateEntry &) const	{ return true; }

	StringClass	ModelName;
	float			MinHealthFraction;	// this state holds while normalized health >= this
};

struct BuildingDamageVisualDefinition
{
	BuildingDamageVisualDefinition(void) : DestroyedModel("") { }

	DynamicVectorClass<BuildingDamageStateEntry>	States;
	StringClass												DestroyedModel;		// empty -> keep the last living model

	//	-1 when the table is empty.  Never returns an index for a destroyed building; that case
	//	is DESTROYED_STATE_INDEX and is asked for explicitly.
	int				Find_State_Index(float normalized_health) const;

	const char *	Get_State_Model(int index) const;
};

//	The sentinel BuildingGameObj stores in place of a state index once the building is destroyed.
enum { BUILDING_DAMAGE_STATE_DESTROYED = -1 };


struct BuildingAnchorEntry
{
	BuildingAnchorEntry(void) : Role(""),BoneName("") { }
	bool	operator == (const BuildingAnchorEntry &) const	{ return false; }
	bool	operator != (const BuildingAnchorEntry &) const	{ return true; }

	StringClass	Role;			// one of Section 39's semantic role names, e.g. "mct_anchor"
	StringClass	BoneName;	// the W3D node/bone this role resolves to on ModelName
};


/*
**	D-001 / V2 roadmap Sections 16A.3 and 28A -- building semantics as sidecar data.
**
**	A point in the building's own frame, with a facing, named by what it is for: where the
**	harvester docks, where vehicles leave the factory, which way the building faces the street.
**	Bone anchors (above) need a bone in the hierarchy; most of these have none, and re-exporting
**	retail W3Ds to add one is exactly what the project may not do.  So they are data beside the
**	model, transformed through the placed building's transform.  Every authored value names its
**	source in the descriptor's _comment_semantic_anchors.  A role a descriptor leaves out has no
**	answer, never a guessed one.
**
**	Roles: entrance, vehicle_exit, infantry_exit, street_side, door, pct, mct, harvester_dock.
**	An unknown role is a load-time refusal.  The destroyed model is not an anchor: it is
**	DamageVisual.DestroyedModel.
*/
struct BuildingSemanticAnchor
{
	BuildingSemanticAnchor(void) : Role(""),LocalPosition(0.0f,0.0f,0.0f),LocalYawDegrees(0.0f) { }
	bool	operator == (const BuildingSemanticAnchor &) const	{ return false; }
	bool	operator != (const BuildingSemanticAnchor &) const	{ return true; }

	StringClass	Role;
	Vector3		LocalPosition;		// in the building's own frame
	float			LocalYawDegrees;	// facing about local Z, from local +X
};

bool	Semantic_Anchor_Role_Is_Known(const char * role);


/*
**	Roadmap Section 21, Phase 20 -- optional functional subcomponents.
**
**	Exactly Section 21.1's own ten example role names, spelled as an enum so a typo in JSON is
**	a load-time refusal rather than a role that silently never matches anything.  Not every role
**	has a defined effect (21.2 gives examples for the first six; tiberium_storage has none) and
**	not every role that does has a real building to attach it to in this tree yet -- see
**	docs/procedural_world/BuildingFunctionalSubcomponents.md for exactly which of the ten are
**	wired this phase and why the rest are not: Phase 20's own step 5, "do not implement an
**	effect when the model lacks a coherent semantic part", applies as much to "no buildable
**	descriptor of that category exists at all" as it does to "no bone represents it".
*/
enum BuildingSubcomponentRoleType
{
	SUBCOMPONENT_POWER_GENERATOR_MODULE = 0,
	SUBCOMPONENT_COMMUNICATIONS_ARRAY,
	SUBCOMPONENT_RADAR_DISH,
	SUBCOMPONENT_REFINERY_PROCESSOR,
	SUBCOMPONENT_TIBERIUM_STORAGE,
	SUBCOMPONENT_FACTORY_PRODUCTION_MODULE,
	SUBCOMPONENT_FACTORY_EXIT_MECHANISM,
	SUBCOMPONENT_REPAIR_MODULE,
	SUBCOMPONENT_DEFENSE_TARGETING_MODULE,
	SUBCOMPONENT_DEFENSE_WEAPON_MODULE,
	SUBCOMPONENT_ROLE_COUNT
};

//	Canonical string <-> enum, shared by the JSON parser and anything that reports a role by
//	name (the debug status command).  Returns nullptr / false rather than guessing on an unknown
//	name -- the same refusal Production.Kind's own "unknown production.kind" already uses.
const char *	Subcomponent_Role_Name(BuildingSubcomponentRoleType role);
bool				Subcomponent_Role_From_Name(const char * name,BuildingSubcomponentRoleType * out_role);


/*
**	The gate this phase exists for -- "functional damage exists without a second building
**	health/state system" -- is why this entry carries no health of its own.  `FullFunctionHealthFraction`
**	and `DisabledHealthFraction` are two points on the *building's own* normalized health, read
**	from the one place that number already lives (DamageableGameObj::Get_Health/Get_Health_Max,
**	the same field DamageVisual's table already reads); the function this part contributes scales
**	linearly between them and is clamped 0..1 outside that range.  A role with no entry in a
**	descriptor's Subcomponents list is simply not tracked -- full function always, exactly the
**	behavior every descriptor had before this list existed, which is what "remains optional per
**	descriptor" means in practice.
**
**	`AnchorBoneName` is resolved the same way BuildingAnchorEntry already is (Get_Bone_Index
**	against every currently-loaded runtime part, first match wins) and degrades the same way on a
**	miss -- not found is "no visual anchor for this part", never a crash and never a reason the
**	numeric effect stops applying.  Empty means the descriptor names no anchor at all, which is a
**	legitimate authored choice, not an error: the capability still degrades with health, there is
**	simply nothing to point a future visual at yet.
*/
struct BuildingSubcomponentDefinition
{
	BuildingSubcomponentDefinition(void) : Role(SUBCOMPONENT_POWER_GENERATOR_MODULE),AnchorBoneName(""),
														 FullFunctionHealthFraction(0.75f),DisabledHealthFraction(0.0f) { }
	bool	operator == (const BuildingSubcomponentDefinition &) const	{ return false; }
	bool	operator != (const BuildingSubcomponentDefinition &) const	{ return true; }

	BuildingSubcomponentRoleType	Role;
	StringClass							AnchorBoneName;
	float									FullFunctionHealthFraction;	// >= this building health fraction -> 1.0
	float									DisabledHealthFraction;		// <= this building health fraction -> 0.0
};


/*
**	Q-017 follow-up -- component assembly.
**
**	Every Commander-constructed building so far has been one `.w3d` in, one `StaticPhysClass`
**	out.  Real Renegade buildings are not built that way: a map-authored building is a shell
**	(exterior/interior statics) plus several separately-placed building aggregates, exactly what
**	`Collect_Building_Components` claims by proximity.  When a real, hand-curated asset manifest
**	is available for a building (see docs/zerohour/BuildingComponentAssembly.md), this table lets
**	a runtime-constructed instance be put together the same way: several parts, each its own
**	physics object, instead of one merged mesh standing in for the whole building.
**
**	Deliberately NOT a `BuildingAggregateClass`/`DoorPhysClass` per part.  Both of those need
**	authored data this project does not have for any of these parts -- per-state animation frame
**	ranges (`BuildingAggregateDefClass::Frame0/Frame1`) and door trigger zones/sounds
**	(`DoorPhysDefClass`) respectively -- and guessing either would be exactly the mistake Q-017's
**	own instructions forbade for preset names, applied to animation/trigger data instead.  So
**	every part is plain static geometry: correct positioning and collision, no per-part
**	animation.  `Kind` is informational plus routing (Code/Combat/building.cpp files each part
**	into InteriorMeshes or ExteriorMeshes by it), not a promise of aggregate/door behavior.
*/
struct BuildingAssemblyPart
{
	BuildingAssemblyPart(void) : Id(""),Kind(""),ModelName(""),Position(0.0f,0.0f,0.0f),
											RotationDegrees(0.0f,0.0f,0.0f),LoadByDefault(true) { }
	bool	operator == (const BuildingAssemblyPart &) const	{ return false; }
	bool	operator != (const BuildingAssemblyPart &) const	{ return true; }

	StringClass	Id;					// the manifest's own part id, for diagnostics only
	StringClass	Kind;					// "exterior" | "interior" | "aggregate" | "door" -- see routing above
	StringClass	ModelName;
	Vector3		Position;			// building-local offset; every part authored so far is (0,0,0)
	Vector3		RotationDegrees;	// building-local rotation; every part authored so far is (0,0,0)
	bool			LoadByDefault;		// false = a night variant or authoring-only source; never loaded
};

struct BuildingAssemblyDefinition
{
	//	Empty (the default, and true for every descriptor except the ones with a real curated
	//	manifest) means "no assembly" -- the single-mesh runtime path is exactly what it always
	//	was, unchanged.
	DynamicVectorClass<BuildingAssemblyPart>	Parts;
};


class BuildingModelDescriptor
{
	public:

		BuildingModelDescriptor(void);

		StringClass		DefinitionName;
		StringClass		ModelName;
		unsigned int	SchemaVersion;

		/*
		**	The `BuildingGameObjDef` preset a runtime-constructed instance of this descriptor is
		**	created from -- how a Commander-built Power Plant becomes a real `PowerPlantGameObj`
		**	with the capabilities its class carries, rather than a bare controller.  Empty means
		**	this descriptor describes a building nobody constructs at runtime (every map-authored
		**	building), which is why it is only required when `build.buildable` is set.
		*/
		StringClass		GameObjectPreset;

		BuildingFootprintDefinition				Footprint;
		BuildingPowerDefinition						Power;
		BuildingCommunicationsCapability			Communications;
		BuildingSiloCapability						TiberiumSilo;
		BuildingConstructionVisualDefinition	ConstructionVisual;
		BuildingBuildDefinition						Build;
		BuildingProductionDefinition				Production;
		BuildingPlacementRules						Placement;
		BuildingDamageVisualDefinition			DamageVisual;
		BuildingAssemblyDefinition					Assembly;

		DynamicVectorClass<BuildingAnchorEntry>	Anchors;
		DynamicVectorClass<BuildingSemanticAnchor>	SemanticAnchors;

		//	Phase 20.  Empty for every descriptor but the handful this phase actually authors --
		//	see BuildingSubcomponentDefinition above for why empty is a real, supported, zero-
		//	behavior-change state rather than a placeholder.
		DynamicVectorClass<BuildingSubcomponentDefinition>	Subcomponents;

		//	The authoritative construction duration: the build block's own value when it sets
		//	one, otherwise the construction visual's.  One accessor so that no caller has to
		//	remember which of the two blocks wins.
		float	Get_Build_Time_Seconds(void) const;

		const char *	Find_Anchor_Bone(const char * role_name) const;

		//	Sidecar semantic anchors.  Resolve composes the anchor's local frame with the placed
		//	building's transform; both return false for a role this descriptor does not author.
		bool				Find_Semantic_Anchor(const char * role_name,BuildingSemanticAnchor * out) const;
		bool				Resolve_Semantic_Anchor(const char * role_name,const Matrix3D & building_world_transform,
															Matrix3D * out_transform) const;

		//	Phase 20.  -1.0f means this descriptor tracks no subcomponent for that role -- the
		//	caller's own signal to leave its capability value unmodified.  Otherwise 0..1, linearly
		//	interpolated between the declared entry's two health thresholds and clamped; see
		//	BuildingSubcomponentDefinition for why building health is the only input.
		float				Get_Subcomponent_Function_Fraction(BuildingSubcomponentRoleType role,
																				 float building_health_fraction) const;
		bool				Has_Subcomponent(BuildingSubcomponentRoleType role) const;
		const char *	Find_Subcomponent_Anchor_Bone(BuildingSubcomponentRoleType role) const;

		//	The model a runtime instance should be wearing right now.  Falls back to ModelName
		//	whenever the table cannot answer, so an absent damage_visual block is a building that
		//	simply never changes model -- the project's standard missing-art degradation.
		const char *	Get_Damage_Model(float normalized_health,bool destroyed) const;

		/*
		**	worldTransform = buildingWorldTransform * nodeModelTransform -- Section 39's own
		**	formula, minus the optional JSON offset this pass does not implement (no descriptor
		**	loaded this pass declares one).  Returns false, and leaves *out_transform
		**	untouched, when the role has no anchor entry or the bone does not exist on any of
		**	the instantiated parts -- the same missing-anchor refusal
		**	CompositeAttachmentClass::Attach uses for a missing bone.
		**
		**	Takes every currently-loaded part's render object, not one model: a single-mesh
		**	runtime building passes a one-element list, and a component-assembly building
		**	(BuildingModelDescriptor::Assembly) passes every part it built, tried in the order
		**	they were built.  The real asset drop's own manifest notes which parts carry the
		**	real MCT/PT bone proxies (interior and most aggregates, per
		**	docs/zerohour/BuildingComponentAssembly.md); trying every part in build order finds
		**	them without this function having to know the convention itself.
		*/
		bool	Resolve_Anchor_World_Transform(const DynamicVectorClass<RenderObjClass *> & part_models,
															 const Matrix3D & building_world_transform,
															 const char * role_name,Matrix3D * out_transform) const;
};

#endif // BUILDINGDESCRIPTOR_H
