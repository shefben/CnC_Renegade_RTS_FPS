/*
**	OpenW3D
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
 *                 Project Name : OpenW3D                                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/hybridpreflight.h                     $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Hybrid world generator rewrite, item I2 -- the immutable metric-cache preflight              *
 * (Procedural_World_Generator_Rewrite_Plan_FINAL_AUDITED.md Sections 0.3, 0.6, 0.8, 4.2, 5    *
 * and 16.4).                                                                                  *
 *                                                                                             *
 * Preflight runs once per request, before any seeded work.  It resolves every runtime metric  *
 * the hybrid phases read -- Humvee / heavy-vehicle / Harvester collision widths, bridge         *
 * capability, starting-building footprints, world units per meter, the fairness               *
 * reference speeds, the Harvester load, biome material slots -- into one HybridMetricCache,    *
 * and rejects the request with a named failure when something required is missing.  Nothing  *
 * after preflight may query a live registry again, and nothing here draws from a seed.        *
 *                                                                                             *
 * The runtime lookups live in HybridPreflightInputs::Gather_From_Runtime; Hybrid_Run_Preflight *
 * itself is pure over (profile, inputs), so the self-checks drive it with hand-filled inputs. *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef HYBRIDPREFLIGHT_H
#define HYBRIDPREFLIGHT_H

#include "always.h"
#include "vector.h"
#include "wwstring.h"
#include "hybridplanning.h"
#include "hybridplanrecords.h"

class ProceduralWorldProfile;


/*
**	Preflight failure codes.  The names the plan spells out are used verbatim; the rest follow the
**	same pattern for the metrics Section 0.8 lists without naming a code.
*/
enum HybridPreflightFailure
{
	HYBRID_PREFLIGHT_OK = 0,
	HYBRID_PREFLIGHT_REQUIRED_FEATURE_DISABLED,				// HYBRID_REQUIRED_FEATURE_DISABLED
	HYBRID_PREFLIGHT_WORLD_SIZE_LIMIT_EXCEEDED,				// WORLD_SIZE_LIMIT_EXCEEDED
	HYBRID_PREFLIGHT_MISSING_HUMVEE_COLLISION_METRIC,		// MISSING_HUMVEE_COLLISION_METRIC
	HYBRID_PREFLIGHT_MISSING_HEAVY_VEHICLE_COLLISION_METRIC,	// MISSING_HEAVY_VEHICLE_COLLISION_METRIC
	HYBRID_PREFLIGHT_MISSING_HARVESTER_COLLISION_METRIC,	// MISSING_HARVESTER_COLLISION_METRIC
	HYBRID_PREFLIGHT_MISSING_WORLD_UNITS_PER_METER,			// MISSING_WORLD_UNITS_PER_METER
	HYBRID_PREFLIGHT_NO_HEAVY_CAPABLE_BRIDGE,					// NO_HEAVY_CAPABLE_BRIDGE
	HYBRID_PREFLIGHT_FORD_CAPABILITY_UNAVAILABLE,			// FORD_CAPABILITY_UNAVAILABLE
	HYBRID_PREFLIGHT_MISSING_STARTING_BUILDING_FOOTPRINT,	// MISSING_STARTING_BUILDING_FOOTPRINT
	HYBRID_PREFLIGHT_MISSING_FAIRNESS_REFERENCE_SPEED,		// MISSING_FAIRNESS_REFERENCE_SPEED
	HYBRID_PREFLIGHT_MISSING_HARVESTER_LOAD_AMOUNT,			// MISSING_HARVESTER_LOAD_AMOUNT
	HYBRID_PREFLIGHT_UNSUPPORTED_BIOME_ASSET_SET,			// UNSUPPORTED_BIOME_ASSET_SET
	HYBRID_PREFLIGHT_FAILURE_COUNT
};

const char *	Hybrid_Preflight_Failure_Name(HybridPreflightFailure failure);


/*
**	Section 4.2 semantic material slots.  Every biome needs a ground slot (its own surface) and the
**	shared exposed-rock slot.
*/
enum HybridMaterialSlot
{
	HYBRID_SLOT_GROUND = 0,			// grass_or_soil / sand_or_dry_soil / snow_ground / barren_or_cracked_ground
	HYBRID_SLOT_EXPOSED_ROCK,		// exposed_rock
	HYBRID_SLOT_COUNT
};

const char *	Hybrid_Material_Slot_Name(HybridBiome biome,HybridMaterialSlot slot);


/*
**	Raw runtime facts preflight consumes.  Gather_From_Runtime fills these from WorldMetricsClass,
**	BridgeSystem, BridgePlannerParams, TerrainTextureSystem and FoliageSystem; tests
**	fill them by hand.  List order is registry order, which is each entry's StableId.
*/
struct HybridVehicleMetricInput
{
	HybridVehicleMetricInput(void) : Measured(false),Length(0.0f),Width(0.0f),Height(0.0f),DefinitionID(0) { }

	bool				Measured;					// WORLDBOX resolved; never a render-bound fallback
	float				Length;						// full collision extents, world units
	float				Width;
	float				Height;
	int				DefinitionID;				// provenance: the physics definition measured

	/*
	**	Diagnostics only (D-034), never hashed or compared: the definition measured or tried, its model string as the
	**	definition stores it, and the render object that names (PhysDefClass::Render_Obj_Name), which is what the
	**	asset manager was asked for.  A failed metric names all three.
	*/
	StringClass		Source;
	StringClass		ModelName;
	StringClass		ModelKey;

	bool operator == (const HybridVehicleMetricInput & that) const
	{
		return (Measured == that.Measured) && (Length == that.Length) && (Width == that.Width) &&
				 (Height == that.Height) && (DefinitionID == that.DefinitionID);
	}
	bool operator != (const HybridVehicleMetricInput & that) const	{ return !(*this == that); }
};

struct HybridBridgeInput
{
	HybridBridgeInput(void) :
		Width(0.0f),DeckThickness(0.0f),RequiredClearance(0.0f),SupportSpacing(0.0f),SupportClearance(0.0f),
		StartSeat(0.0f),EndSeat(0.0f) { }

	StringClass		Name;
	float				Width;						// clear deck width, world units
	float				DeckThickness;				// structural depth
	float				RequiredClearance;
	float				SupportSpacing;
	float				SupportClearance;

	/*
	**	The abutment seat at each end: the definition's own start and end cap section lengths, which
	**	BridgeSystem::Layout_Bridge lays from each endpoint inward.  The canonical abutment is that cap, and it
	**	has to stand on the bank, so a bridge reaches this far past the obstruction at each end.
	*/
	float				StartSeat;
	float				EndSeat;
	StringClass		StructuralModels;			// every colliding or traversable model, '|' separated

	bool operator == (const HybridBridgeInput & that) const
	{
		return (Name == that.Name) && (Width == that.Width) && (DeckThickness == that.DeckThickness) &&
				 (RequiredClearance == that.RequiredClearance) && (SupportSpacing == that.SupportSpacing) &&
				 (SupportClearance == that.SupportClearance) && (StartSeat == that.StartSeat) &&
				 (EndSeat == that.EndSeat) && (StructuralModels == that.StructuralModels);
	}
	bool operator != (const HybridBridgeInput & that) const	{ return !(*this == that); }
};

struct HybridBuildingFootprintInput
{
	StringClass		Name;
	int				Team;							// 0 = GDI, 1 = Nod
	bool				Found;
	float				Radius;
	float				Height;

	bool operator == (const HybridBuildingFootprintInput & that) const
	{
		return (Name == that.Name) && (Team == that.Team) && (Found == that.Found) &&
				 (Radius == that.Radius) && (Height == that.Height);
	}
	bool operator != (const HybridBuildingFootprintInput & that) const	{ return !(*this == that); }
};

struct HybridTerrainLayerInput
{
	StringClass		Name;
	int				Kind;							// TerrainLayerKind
	int				SurfaceType;				// SURFACE_TYPE_*

	bool operator == (const HybridTerrainLayerInput & that) const
	{
		return (Name == that.Name) && (Kind == that.Kind) && (SurfaceType == that.SurfaceType);
	}
	bool operator != (const HybridTerrainLayerInput & that) const	{ return !(*this == that); }
};

struct HybridPreflightInputs
{
	HybridPreflightInputs(void);

	void		Gather_From_Runtime(const ProceduralWorldProfile & profile);

	float		WorldUnitsPerMeter;
	HybridVehicleMetricInput	Humvee;
	HybridVehicleMetricInput	HeavyVehicle;			// WorldMetricsClass's heavy-ground (tracked) aggregate
	HybridVehicleMetricInput	Harvesters[2];			// each team's starting refinery's Harvester, 0 = GDI, 1 = Nod
	float		ReferenceVehicleSpeed;		// m/s
	float		ReferenceInfantrySpeed;		// m/s
	bool		FordCapabilityAvailable;	// no ford subsystem exists yet
	float		BridgeMaxSpanLength;
	float		BridgeWaterClearanceMargin;
	float		BridgeApproachLength;		// 0 = none registered; preflight falls back to 8W
	bool		DecorationTypesAvailable;
	bool		LampTypeAvailable;

	/*
	**	Section 0.6's oversize escape: only an explicit developer/offline caller sets this.  Runtime
	**	gathering never does, so a hosted or joined match always gets WORLD_SIZE_LIMIT_EXCEEDED.
	*/
	bool		AllowOversizeDevelopment;

	DynamicVectorClass<HybridBridgeInput>					Bridges;
	DynamicVectorClass<HybridBuildingFootprintInput>	Buildings;
	DynamicVectorClass<HybridTerrainLayerInput>			TerrainLayers;
};


/*
**	The immutable metric cache every hybrid phase reads after preflight.
*/
struct HybridMetricCache
{
	HybridConstants		Constants;
	HybridWorkBudgets		Budgets;

	float		HumveeWidth;
	float		HumveeHeight;
	float		HeavyVehicleWidth;
	float		HeavyVehicleHeight;
	float		HarvesterWidth;								// the wider of the two teams' Harvesters
	float		HarvesterLength;								// ...and the longer one: Section 8 Phase 9's turning disc
	float		HarvesterHeight;
	int		HumveeDefinitionID;							// provenance of the measurements above
	int		HeavyVehicleDefinitionID;
	int		HarvesterDefinitionID[2];
	float		ReferenceVehicleSpeed;
	float		ReferenceInfantrySpeed;
	float		HarvesterLoadAmount;
	bool		FordsAvailable;
	bool		DecorationAvailable;
	bool		LampsAvailable;

	HybridRoadClass	HeavyRouteClass;							// Section 9.5: HIGHWAY_4 when highways are quota'd, else PRIMARY_2
	int		BridgeForClass[HYBRID_ROAD_CLASS_COUNT];			// index into the input bridges, -1 = none carries it
	float		BridgeRequiredWidth[HYBRID_ROAD_CLASS_COUNT];	// Section 12.4 clear-width requirement
	float		BridgeMaxSpanLength;
	float		BridgeWaterClearanceMargin;
	float		BridgeApproachLength;

	/*
	**	Section 12.6's planning metrics, per class: the largest required approach length, and the largest structural
	**	depth, among registered bridge/overpass kinds able to carry the class (0 when none can).  Approach length
	**	comes from BridgePlannerParams (every kind shares it), depth from each definition's deck thickness.
	*/
	float		BridgeApproachMax[HYBRID_ROAD_CLASS_COUNT];
	float		OverpassDepthMax[HYBRID_ROAD_CLASS_COUNT];

	int		BiomeSlotLayer[HYBRID_BIOME_COUNT][HYBRID_SLOT_COUNT];	// terrain layer index, -1 = unresolved
	bool		BiomeAvailable[HYBRID_BIOME_COUNT];

	unsigned int	ContentRevision;							// Hybrid_Structural_Content_Revision of the inputs
	unsigned int	PlanningInputHash;
};

struct HybridPreflightResult
{
	HybridPreflightFailure	Failure;
	StringClass					Detail;
	HybridMetricCache			Metrics;
};


/*
**	The content-independent part of preflight -- Section 5's required features and Section 0.6's
**	size limits -- for refusing a request while the player is still at the menu, before any
**	content is measured or any seeded stage runs.  Hybrid_Run_Preflight repeats it first.
*/
bool				Hybrid_Preflight_Request_Check(const ProceduralWorldProfile & profile,bool allow_oversize,
															 HybridPreflightResult * result);

/*
**	Runs every Section 0.8 check in the plan's order and fills the metric cache.  Returns false
**	with result->Failure set on the first failing check.
*/
bool				Hybrid_Run_Preflight(const ProceduralWorldProfile & profile,const HybridPreflightInputs & inputs,
											HybridPreflightResult * result);

/*
**	The structural content identity: a digest of every collision-relevant record preflight froze
**	-- vehicle collision boxes and their definitions, bridge geometry and structural
**	models, starting-building footprints, terrain layers and their surface types.  Render-only
**	facts (decoration, lamps, bridge dressing, textures) are left out so a cosmetic content
**	change does not invalidate a plan.
*/
unsigned int	Hybrid_Structural_Content_Revision(const HybridPreflightInputs & inputs);

/*
**	Section 0.3 HybridPlanningInputHash: every authoritative input and nothing render-only or
**	legacy.  Hybrid_Run_Preflight stores it in the cache; exposed for the self-checks.
*/
unsigned int	Hybrid_Planning_Input_Hash(const ProceduralWorldProfile & profile,const HybridPreflightInputs & inputs,
													const HybridMetricCache & metrics);


#endif
