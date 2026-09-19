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

struct BuildingAnchorEntry
{
	BuildingAnchorEntry(void) : Role(""),BoneName("") { }
	bool	operator == (const BuildingAnchorEntry &) const	{ return false; }
	bool	operator != (const BuildingAnchorEntry &) const	{ return true; }

	StringClass	Role;			// one of Section 39's semantic role names, e.g. "mct_anchor"
	StringClass	BoneName;	// the W3D node/bone this role resolves to on ModelName
};


class BuildingModelDescriptor
{
	public:

		BuildingModelDescriptor(void);

		StringClass		DefinitionName;
		StringClass		ModelName;
		unsigned int	SchemaVersion;

		BuildingFootprintDefinition				Footprint;
		BuildingPowerDefinition						Power;
		BuildingCommunicationsCapability			Communications;
		BuildingSiloCapability						TiberiumSilo;
		BuildingConstructionVisualDefinition	ConstructionVisual;
		BuildingBuildDefinition						Build;
		BuildingProductionDefinition				Production;
		BuildingPlacementRules						Placement;

		DynamicVectorClass<BuildingAnchorEntry>	Anchors;

		//	The authoritative construction duration: the build block's own value when it sets
		//	one, otherwise the construction visual's.  One accessor so that no caller has to
		//	remember which of the two blocks wins.
		float	Get_Build_Time_Seconds(void) const;

		const char *	Find_Anchor_Bone(const char * role_name) const;

		/*
		**	worldTransform = buildingWorldTransform * nodeModelTransform -- Section 39's own
		**	formula, minus the optional JSON offset this pass does not implement (no descriptor
		**	loaded this pass declares one).  Returns false, and leaves *out_transform
		**	untouched, when the role has no anchor entry or the bone does not exist on the
		**	instantiated model -- the same missing-anchor refusal
		**	CompositeAttachmentClass::Attach uses for a missing bone.
		*/
		bool	Resolve_Anchor_World_Transform(RenderObjClass * building_model,const Matrix3D & building_world_transform,
															 const char * role_name,Matrix3D * out_transform) const;
};

#endif // BUILDINGDESCRIPTOR_H
