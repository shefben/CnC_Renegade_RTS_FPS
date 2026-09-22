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
 *                     $Archive:: /Commando/Code/Combat/proceduralassetregistry.h             $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	Roadmap Section 28A -- procedural asset metadata and connector descriptors.  "Generators      *
 *	must not independently reverse-engineer the same W3D geometry through bounding-box guesses."   *
 *                                                                                             *
 *	Reuse, not a parallel descriptor format.  Section 39's `BuildingModelDescriptor`                *
 *	(buildingdescriptor.h) already owns a real footprint, real placement slope limit and real       *
 *	bone-resolved anchors for every building this project curates, loaded through                   *
 *	`BuildingDescriptorManager` from `data/building_descriptors/`; `WorldMetricsClass::                *
 *	Get_Building_Footprint` already turns that footprint into the circumscribing placement             *
 *	radius a generator needs.  This file names which descriptor answers for a given procedural         *
 *	asset id and adds exactly the fields Section 28A.4 asks for that Section 39 has no reason to         *
 *	carry -- settlement eligibility, neighbour spacing, cover classification -- rather than a             *
 *	second copy of width/depth/slope/anchors.                                                              *
 *                                                                                             *
 *	What is actually in the registry.  Real, offline-verified data only -- the same standard        *
 *	Section 34 Phase 8's Humvee-derived road width and this file's own bone-name dump both already      *
 *	hold themselves to.  Every W3D file this project currently ships under `data/building_models/`        *
 *	was opened (`Code/ww3d2/w3d_file.h`'s own `W3D_CHUNK_HIERARCHY`/`W3D_CHUNK_PIVOTS` layout, read           *
 *	offline the same way the Humvee's `WORLDBOX` was) and carries no bone named for an entrance, a          *
 *	door, or a pedestrian approach -- Section 28A.2's own connector list -- on any of the three               *
 *	real building kinds this tree currently has geometry for (a GDI Communications Center, a GDI               *
 *	Power Plant, a Nod Power Plant; every other `data/building_descriptors/` entry names a part                *
 *	list `data/building_models/` has no files for yet).  So `Resolve_Entrance_Anchor` below                     *
 *	honestly returns false for all three -- the same missing-anchor refusal                                     *
 *	`BuildingModelDescriptor::Resolve_Anchor_World_Transform` already uses -- rather than a                      *
 *	street-facing orientation this project cannot actually verify.  See                                          *
 *	`docs/procedural_world/Settlements.md` for what this means for Phase 13's own step 5.                         *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef PROCEDURALASSETREGISTRY_H
#define PROCEDURALASSETREGISTRY_H

#ifndef ALWAYS_H
#include "always.h"
#endif

#include "vector.h"
#include "wwstring.h"


//	Section 28A.1's own "category", named for what this tree's own real content actually
//	distinguishes today; POI/PROP are named for Phase 13 step 9's own vocabulary even though no
//	real asset fills either one yet (see the class comment).
enum ProceduralAssetCategory
{
	ASSET_CATEGORY_BUILDING = 0,
	ASSET_CATEGORY_POI,
	ASSET_CATEGORY_PROP,
	ASSET_CATEGORY_COUNT
};

//	Section 17A.3, named exactly as that section lists them.
enum AssetCoverClassificationType
{
	ASSET_COVER_VISUAL_ONLY = 0,
	ASSET_COVER_CONCEALMENT_ONLY,
	ASSET_COVER_PARTIAL_INFANTRY,
	ASSET_COVER_FULL_INFANTRY,
	ASSET_COVER_VEHICLE,
	ASSET_COVER_TYPE_COUNT
};


/*
**	One procedurally-placeable asset.  `DescriptorName` is a `BuildingDescriptorManager`
**	definition name for `ASSET_CATEGORY_BUILDING`; the base 28A.1/28A.2 fields (footprint,
**	placement slope, anchors) are never copied here -- they are resolved on demand through that
**	one owner, via `Resolve_Placement_Radius` below.
*/
class ProceduralAssetDescriptor
{
public:
	ProceduralAssetDescriptor(void);

	bool	operator == (const ProceduralAssetDescriptor & that) const;
	bool	operator != (const ProceduralAssetDescriptor & that) const	{ return !(*this == that); }

	StringClass					Id;					// stable id, Section 3B.5 -- equals DescriptorName for a building
	StringClass					DescriptorName;	// a BuildingDescriptorManager definition name
	ProceduralAssetCategory	Category;
	int								Faction;				// a PLAYERTYPE value; PLAYERTYPE_NEUTRAL for faction-agnostic

	AssetCoverClassificationType	Cover;
	bool									NavObstacle;				// true -- blocks ground pathing where it stands
	bool									AllowDuplicateAdjacent;	// Section 28A.4's own field, named identically
	float									MinSetback;					// world units from the street edge
	float									MinNeighborSpacing;		// world units, centre to centre, between two instances

	/*
	**	Section 28A.1's own placement radius/height, resolved through WorldMetricsClass (which
	**	itself resolves through BuildingDescriptorManager) rather than measured a second time
	**	here.  Returns false when the named descriptor is not loaded or carries no footprint --
	**	the honest "cannot place this, do not guess a size" answer.
	*/
	bool	Resolve_Placement_Radius(float * radius_out,float * height_out) const;

	/*
	**	Section 28A.2's own entrance connector.  Returns false for every asset this registry
	**	currently carries -- see the file's own header comment for the real, verified reason
	**	(no entrance/door bone exists on any of the three real models this tree has geometry
	**	for) -- never a fabricated bone name or a guessed world-space offset.
	*/
	bool	Has_Entrance_Anchor(void) const	{ return false; }
};


class ProceduralAssetRegistry
{
public:

	//	Loads data/building_descriptors/ through BuildingDescriptorManager (idempotent -- a
	//	caller that already loaded it separately is not double-charged) and registers this
	//	file's own real, offline-verified entries.  Safe to call with no game content mounted;
	//	entries whose own descriptor fails to load are skipped, not fabricated.
	static void		Init(void);
	static void		Shutdown(void);

	static bool		Has_Registry(void)		{ return Built; }

	static int		Get_Asset_Count(void)	{ return Assets.Count(); }
	static bool		Get_Asset(int index,ProceduralAssetDescriptor * out);
	static bool		Get_Asset_By_Id(const StringClass & id,ProceduralAssetDescriptor * out);

	/*
	**	Every registered asset of a category, eligible for a faction (PLAYERTYPE_NEUTRAL asks
	**	for faction-agnostic assets only; a real faction also includes PLAYERTYPE_NEUTRAL
	**	entries).  Appends indices into this registry's own list; not cleared first.
	*/
	static int		Find_Assets(ProceduralAssetCategory category,int faction,
											 DynamicVectorClass<int> & indices_out);

private:

	static bool		Built;
	static DynamicVectorClass<ProceduralAssetDescriptor>	Assets;
};

#endif	// PROCEDURALASSETREGISTRY_H
