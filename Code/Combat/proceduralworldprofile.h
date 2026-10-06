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
 *                     $Archive:: /Commando/Code/Combat/proceduralworldprofile.h              $*
 *                                                                                             *
 *	V2 roadmap Section 34 Phase 1 -- the world profile schema.  See Section 4 (suggested        *
 *	schema), Section 4A (field semantics/limits contract), and Section 17A.1 (fairness          *
 *	threshold fields) for the design this mirrors.                                              *
 *                                                                                             *
 *	This is the immutable identity/configuration contract every later v2 generator phase        *
 *	consumes -- one JSON-loaded value object, the same shape `BuildingModelDescriptor`          *
 *	(buildingdescriptor.h) already established for the v1 building layer: pure data here,       *
 *	loading/validation in `ProceduralWorldProfileManager` (this file), hashing/manifest work     *
 *	in `worldgenerationmanifest.h`.  Every field documents units/meaning/default per Section     *
 *	4A; the full per-field table lives in docs/procedural_world/WorldProfileSchema.md.           *
 *                                                                                             *
 *	Nothing in this pass generates terrain, roads, or anything else -- later phases (2 onward)  *
 *	are the actual generators.  This phase only has to make a profile creatable, valid,         *
 *	hashable, and reproducible.  Fields later phases have not been implemented yet (fairness    *
 *	thresholds in particular) are still parsed and validated now, per Phase 1's own "gameplay    *
 *	validation thresholds" requirement, so those phases do not have to touch the schema again.   *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef PROCEDURALWORLDPROFILE_H
#define PROCEDURALWORLDPROFILE_H

#include "always.h"
#include "wwstring.h"
#include "vector.h"
#include "hybridplanrecords.h"


/*
**	World-scale dimensions.  Width/Height/ChunkSize/SeaLevel are all in the same world-unit
**	system Section 3A.1's `GameplayScaleProfile` measures actors in -- this schema does not
**	define its own unit system.
*/
struct ProceduralWorldDimensions
{
	ProceduralWorldDimensions(void) : Width(4096.0f),Height(4096.0f),ChunkSize(128.0f),SeaLevel(0.0f),
												 FitToLevel(false),CenterX(0.0f),CenterY(0.0f) { }
	float	Width;			// world units, > 0
	float	Height;			// world units, > 0
	float	ChunkSize;		// world units, > 0, must divide Width/Height for a whole chunk grid
	float	SeaLevel;		// world units, the water-plane height every hydrology/terrain height is relative to

	//	Schema 2.  When true, a procedural world laid over an existing hand-authored level takes
	//	Width/Height/SeaLevel from that level's own measured extents
	//	(PhysicsSceneClass::Get_Level_Extents) instead of from the fields above, which then act
	//	only as the fallback for a level that reports no usable extent.  This exists because the
	//	right size for a ring enclosing a stock Renegade map is a measurable property of that
	//	map, not a number a profile author should have to know; the values actually used are
	//	recorded in the generation manifest, so the world stays reproducible either way.
	bool	FitToLevel;

	//	Schema 2.  Where the world's centre sits, in world units.  The origin -- the default --
	//	is what a world generated from nothing uses.  A world laid over a hand-authored level
	//	needs the level's own centre instead, because a stock Renegade map is not built around
	//	the origin and a ring centred there would enclose the wrong ground.  FitToLevel fills
	//	this in from the level's measured extents.
	float	CenterX;
	float	CenterY;
};


/*
**	Section 1.4-1.5 -- the boundary cliff ring and the hard barrier just past it.  See Section
**	5 (Phase 2's own design section) for how these become real geometry; this is only the
**	profile-level sizing.
*/
struct ProceduralWorldBoundarySettings
{
	ProceduralWorldBoundarySettings(void) : Shape("irregular_ring"),CliffBandWidth(180.0f),
														  CliffMinHeight(90.0f),CliffMaxHeight(180.0f),
														  HardBarrierOffset(20.0f),AirCeiling(260.0f),
														  RimRoughness(0.35f),FaceRoughness(0.45f),
														  ToeWidth(14.0f),CollisionSimplification(2) { }
	StringClass	Shape;					// generator style tag; Phase 2 defines the legal set, not this schema
	float			CliffBandWidth;		// world units, > 0, must fit inside the world without consuming the playable interior
	float			CliffMinHeight;		// world units, > 0
	float			CliffMaxHeight;		// world units, >= CliffMinHeight
	float			HardBarrierOffset;	// world units past the cliff band where actors/geometry are hard-rejected
	float			AirCeiling;				// world units above sea level; must clear CliffMaxHeight (Section 1.5's aircraft rule)

	//	Section 5.5's remaining named profile controls, added in schema version 2 when Phase 3
	//	built the cliff geometry that consumes them.  Roughness values are normalized 0..1
	//	amplitude weights, exactly like Section 18B's density weights: 0 means "perfectly smooth,
	//	the generator applies no variation at all", 1 means "the generator's own maximum
	//	variation for that feature".  The mapping from weight to world units is the cliff
	//	generator's concern, not this schema's.
	float			RimRoughness;				// normalized weight, 0..1 -- how much the rim height wanders between min/max
	float			FaceRoughness;				// normalized weight, 0..1 -- how much the face inflects between rim and toe
	float			ToeWidth;					// world units, >= 0 -- the scree/blend skirt at the cliff base, inside the band
	int			CollisionSimplification;// >= 1 -- render stations per collision station; 1 means collision matches render exactly
};


/*
**	Section 18B's MACRO/MESO density knobs.  These are normalized weights (0..1), not area
**	fractions or counts -- Section 4A requires density semantics to be stated explicitly rather
**	than left implementation-defined, so: 0 means "generator may omit the feature entirely",
**	1 means "generator's own maximum density for this feature", and the mapping from weight to
**	actual feature count/area is entirely the terrain generator's (Phase 3/4's) concern, not
**	this schema's.
*/
struct ProceduralWorldTerrainSettings
{
	ProceduralWorldTerrainSettings(void) : MacroRelief(0.6f),HillDensity(0.45f),MountainDensity(0.18f),
														 InteriorCliffDensity(0.16f),ValleyDensity(0.30f),
												 MinElevation(-30.0f),MaxElevation(160.0f),LandElevationBias(0.0f),
														 MaxBuildableSlopeDegrees(28.0f),FeatureScale(900.0f),
														 RidgeSharpness(0.55f),PlateauChance(0.15f),
														 BoundaryApproachFlattening(0.6f) { }
	float	MacroRelief;				// normalized weight, 0..1
	float	HillDensity;				// normalized weight, 0..1
	float	MountainDensity;			// normalized weight, 0..1
	float	InteriorCliffDensity;	// normalized weight, 0..1 -- Phase 5's own concern; parsed and
												//	hashed since Phase 1, not consumed by Phase 4's heightfield
												//	generator (Section 8.1's vertical-faced replacement does
												//	not exist yet)
	float	ValleyDensity;				// normalized weight, 0..1

	//	Schema 3, added when Phase 4 built the macro terrain generator these five densities
	//	above were parsed-but-unconsumed placeholders for.
	float	MinElevation;				// world units relative to world.sea_level, may be negative -- the relief floor
	float	MaxElevation;				// world units relative to world.sea_level, must clear boundary.cliff_min_height -- the relief ceiling
	float	LandElevationBias;		// shifts ordinary terrain above sea level without moving water or the cliff foot
	float	MaxBuildableSlopeDegrees;	// degrees, (0,90) -- both the generation-time ordinary-slope cap
												//	and the chunk registry's buildability threshold.  One number
												//	serves both deliberately: two separate definitions of "too
												//	steep" that could silently disagree is worse than one that
												//	is slightly conservative for either use.
	float	FeatureScale;				// world units, > 0 -- the approximate wavelength of a macro landform
	float	RidgeSharpness;			// normalized weight, 0..1
	float	PlateauChance;				// normalized weight, 0..1 -- 0 means the generator never terraces
												//	a rise into a mesa; see docs/procedural_world/MacroTerrainGeneration.md
												//	for why this is a heightfield approximation, not Phase 5's
												//	vertical-faced mesa
	float	BoundaryApproachFlattening;	// normalized weight, 0..1 -- how strongly relief is pulled toward
												//	the flat cliff band as the approach zone nears it
};


/*
**	Section 9, Section 18A.  Rivers/Lakes/Ponds/Coastline gate whether that category generates
**	at all; the four numeric fields below are Schema 4's addition when Phase 6 built the
**	generator these were reserved for.  Section 18A asks for full width/depth *ranges* per
**	category; this schema carries single representative widths instead (a river's own width
**	still grows with its accumulated drainage as it runs downhill -- see
**	docs/procedural_world/Hydrology.md -- so a fixed number is a floor/ceiling pair's practical
**	midpoint, not a claim that every river is a constant width). Depth is not a profile field at
**	all: WaterDefinitionClass::BedDepth, already set per category by
**	WaterSystem::Define_Default_Water, is the single source of truth for carve depth, and a
**	second depth number on this schema could only ever disagree with it.
*/
struct ProceduralWorldWaterSettings
{
	ProceduralWorldWaterSettings(void) : Rivers(true),Lakes(true),Ponds(true),Coastline(true),
														RiverWidth(6.0f),StreamWidth(2.5f),
														RiverAccumulationChunks(6.0f),MaxRiverCount(4) { }
	bool	Rivers;
	bool	Lakes;
	bool	Ponds;
	bool	Coastline;

	float	RiverWidth;					// world units, > 0 -- width a channel reaches at full accumulation
	float	StreamWidth;				// world units, > 0, <= RiverWidth -- width at a channel's own source
	float	RiverAccumulationChunks;	// > 0 -- chunks of drainage a channel needs before it counts as a
												//	river rather than a stream/tributary (Section 9.2)
	int	MaxRiverCount;				// > 0 -- bounded count of distinct channel systems generated
};


/*
**	D-003 Phase 3B.  Schema 11's addition when the map plan generator was built.  A 2D layout
**	plan built before any 3D terrain: base sites, Tiberium fields, lanes, integer height tiers,
**	planned water and per-cell passability.  Absent block or `enabled:false` skips the phase
**	and the pipeline behaves as it did under schema 10 (noise-first terrain).  A fit_to_level
**	world always skips it.  Field ranges are validated by Validate.
*/
struct ProceduralWorldPlanSettings
{
	ProceduralWorldPlanSettings(void) : Enabled(false),Symmetry("rotational"),
														TierCount(3),TierStep(18.0f),DetailWeight(0.35f),
														LaneWidthCells(1),BaseRadiusCells(3),FlankLanes(1),
														WaterFraction(0.08f),BoundaryMarginCells(2.0f),
														Archetype("auto"),BiomeMode("auto"),Biome("auto"),
														SecondaryBiome("auto"),MixedBiomeProbability(0.125f),
														RoadDensity(0.65f),IslandChance(0.20f),RouteRedundancy(2),
														AllowFords(false),LegacyLaneWidthCellsPresent(false) { }
	bool			Enabled;
	StringClass	Symmetry;				// "none" | "rotational" | "mirror_x"
	int			TierCount;				// 2..5 -- integer height tiers
	float			TierStep;				// world units per tier, > 0
	float			DetailWeight;			// 0..1 -- share of the noise relief kept inside a tier
	int			LaneWidthCells;		// legacy (schema <= 11) -- inert on the hybrid path (Section 4)
	int			BaseRadiusCells;		// >= 1 -- flat base disc radius in plan cells
	int			FlankLanes;				// 0..2 -- flank lanes besides the centre lane
	float			WaterFraction;			// hybrid 0.03..0.20 -- share of the world planned as water
	float			BoundaryMarginCells;	// >= 0 -- cells kept between a site disc and the world edge

	//	Schema 12 -- hybrid rewrite Section 4 (item I1).  Names are the plan's spellings.
	StringClass	Archetype;				// "auto" | a Section 6 archetype
	StringClass	BiomeMode;				// "auto" | "single" | "mixed"
	StringClass	Biome;					// "auto" | temperate | desert | snow | tiberium_wasteland
	StringClass	SecondaryBiome;		// same set; only legal Section 4.3 pairs
	float			MixedBiomeProbability;	// 0..1 -- auto mode's chance of a mixed map
	float			RoadDensity;			// 0..1
	float			IslandChance;			// 0..1
	int			RouteRedundancy;		// 2..3
	bool			AllowFords;				// true fails preflight until a ford capability exists

	//	Set by the parser when the profile carries lane_width_cells, so the hybrid path can say
	//	once that it ignores it (Section 0.7).  Not a profile field.
	bool			LegacyLaneWidthCellsPresent;

	int			Route_Family_Count(void) const	{ return Hybrid_Route_Family_Count (RouteRedundancy,FlankLanes); }
};


/*
**	Section 15, Section 17A.6.  Schema 5's addition when Phase 12 built the field generator
**	these were reserved for -- Section 17A.6 requires field/profile data to "define or derive"
**	exactly this list.  No canonical v1 data source exists for a Tiberium field's own game-scale
**	yield or crystal density (unlike, say, the Humvee's own W3D-measured width), so these are
**	fixed, documented, tunable defaults rather than a guess dressed up as a measurement.
*/
struct ProceduralWorldResourceSettings
{
	ProceduralWorldResourceSettings(void) :
		FieldRadiusMin(20.0f),FieldRadiusMax(60.0f),
		InitialYieldMin(1500.0f),InitialYieldMax(4000.0f),
		CrystalDensity(0.15f),HarvestAnchorCount(3),
		HarvesterLoadAmount(600.0f),
		GrowthEnabled(false),GrowthRatePerSecond(0.0f)
		{ }

	float	FieldRadiusMin;			// world units, > 0 -- Section 17A.6's own "field area/radius range"
	float	FieldRadiusMax;			// world units, >= FieldRadiusMin
	float	InitialYieldMin;			// credit-equivalent value, >= 0 -- Section 17A.6's own "initial yield range"
	float	InitialYieldMax;			// >= InitialYieldMin
	float	CrystalDensity;			// 0..1 -- Section 17A.6's own "crystal density/harvest density"
	int	HarvestAnchorCount;		// >= 1 -- Section 17A.6's own "harvest anchor count"
	float	HarvesterLoadAmount;		// >= 0 -- Section 17A.6's own "Harvester load amount"

	//	Step 11: "optional regeneration/growth only if profile enables it".  Off by default --
	//	no generator this schema has ever shipped relies on a field growing back, so a profile
	//	that never mentions this stays exactly as it always has.
	bool		GrowthEnabled;
	float		GrowthRatePerSecond;		// >= 0, meaningful only when GrowthEnabled
};


struct ProceduralWorldFeatureFlags
{
	ProceduralWorldFeatureFlags(void) : Forests(true),Settlements(true),Tunnels(true),
													 Highways(true),TiberiumFields(true),
													 LegacySettlementsPresent(false) { }
	bool	Forests;
	bool	Settlements;			// legacy -- inert on the hybrid path (Section 0.7)
	bool	Tunnels;				// legacy path only -- the native hybrid generator builds no tunnels (D-032)
	bool	Highways;
	bool	TiberiumFields;

	//	Set by the parser when world_features.settlements is present, so the hybrid path can say
	//	once that it ignores it.  Not a profile field.
	bool	LegacySettlementsPresent;
};


/*
**	Section 3C.1's bounded-retry requirement, as one profile-level ceiling every generator
**	stage's own local retry loop is bounded by.  A stage may use fewer retries than this; none
**	may use more.
*/
struct ProceduralWorldRetryLimits
{
	ProceduralWorldRetryLimits(void) : MaxLocalRetriesPerStage(8),MaxAlternateCandidatesPerFeature(6) { }
	int	MaxLocalRetriesPerStage;				// > 0 -- Section 3C.1 step 1, bounded local geometric adjustment
	int	MaxAlternateCandidatesPerFeature;	// > 0 -- Section 3C.1 step 2, bounded alternate candidate attempts
};


/*
**	Section 34 Phase 17 step 5's four grading limits, named exactly as that step lists them.
**
**	Every one of them is a ceiling on what the Commander may do to ground that a generator
**	already produced and that Phase 15's combat-space validation already accepted, which is why
**	they belong to the profile rather than to a building descriptor: a descriptor can say how
**	steep a foundation tolerates, but only the world can say how much of itself may be moved.
**
**	MaxSlopeChangeRadians defaults to the same 0.5 rad buildable-slope cap every terrain-reading
**	generator in this chain already shares, so a grading operation can never leave ground steeper
**	than ground a generator would have been willing to make.
*/
struct ProceduralWorldGradingSettings
{
	ProceduralWorldGradingSettings(void) :
		MaxCutDepth(6.0f),MaxFillHeight(6.0f),
		MaxSlopeChangeRadians(0.5f),MaxAffectedArea(4096.0f) { }

	float	MaxCutDepth;				// world units, > 0
	float	MaxFillHeight;				// world units, > 0
	float	MaxSlopeChangeRadians;	// > 0
	float	MaxAffectedArea;			// world units^2, > 0
};


/*
**	V2 roadmap Section 34 Phase 21 step 1's own "logistics profile toggles".  Section 22.1's own
**	default philosophy -- "keep core player purchases simple... default gameplay must remain
**	viable without mandatory micromanagement" -- is why every field here defaults to leaving
**	logistics observable but inert: `Enabled` builds the graph (routes/blockage status a
**	Commander UI or an AI can read), `ConvoyEnabled` is the separate, further-opt-in step that
**	lets anything actually issue a convoy order, and `ConstructionSupportRadius` at 0 means
**	Section 22.2's "construction support radius" grants no bonus at all until a profile author
**	explicitly sets one.
*/
struct ProceduralWorldLogisticsSettings
{
	ProceduralWorldLogisticsSettings(void) :
		Enabled(false),ConvoyEnabled(false),ConstructionSupportRadius(0.0f),EnemyThreatRadius(0.0f) { }

	bool		Enabled;
	bool		ConvoyEnabled;
	float		ConstructionSupportRadius;	// world units, >= 0; 0 disables the bonus (Section 22.2 step 5)

	//	Schema 9 (D-001, Section 22 enemy obstruction): a known enemy contact within this many world
	//	units of a route closes it to that team's convoys.  0, the default, is off -- the same
	//	inert-unless-asked default as the rest of this block.
	float		EnemyThreatRadius;
};


/*
**	V2 roadmap Section 34 Phase 22 step 1's own "explicit world/game-profile toggle... default is
**	OFF unless user later changes policy".  Section 23's opening line -- "Do not alter default
**	Renegade vehicle handling unless enabled by game profile" -- is why every field defaults to
**	leaving VehicleGameObj::Apply_Control's live control path byte-for-byte what it always was:
**	`Enabled` is the only field that path reads before doing anything else, and a legacy/stock
**	map never loads a profile at all (ProceduralWorldManager::Get_Active_Profile stays default-
**	constructed), so it is provably unaffected -- a stronger guarantee than gating through
**	ssgm.ini's own donor-parity SSGMSettingsClass would give, since that file is read for every
**	map, procedural or not.  `AllowImmobilize` is step 5's own further, explicit opt-in: a human
**	driver's basic drivability is never removed by health alone unless a profile author asks for
**	that specifically.
*/
/*
**	V2 roadmap Section 16A.2 -- which structures a generated base starts with, declared by the
**	profile rather than inferred from a faction name.  Each list names building descriptors
**	(data/building_descriptors file stems) in placement order; the first is the base's anchor
**	and goes on the selected site itself.  Empty lists place nothing, which is also what every
**	world fitted to a hand-authored level gets: that level brings its own base, and base
**	selection never runs over it.  Schema 9.
**
**	Everything else Section 16A.8 lists -- credits, wallet cap, starting units, the Commander
**	role, the shroud policy, AI state -- stays the game mode's, exactly as on a stock map, until
**	a profile field is added for it.  Nothing here overrides them.
*/
struct ProceduralWorldBaseSettings
{
	ProceduralWorldBaseSettings(void) : SpawnCandidateCount(12),SpawnZoneRadius(30.0f) { }

	DynamicVectorClass<StringClass>	GdiInitialBuildings;
	DynamicVectorClass<StringClass>	NodInitialBuildings;
	int										SpawnCandidateCount;		// Section 16A.6's spawn pool size, per faction
	float										SpawnZoneRadius;			// Section 16A.5's fallback zone, world units
};


/*
**	Schema 9 (D-001 P23): environment weather a generated world asks for.  Densities are the
**	weather renderer's own unit (particles per square world unit, the same the rain/snow/ash
**	console commands take).  Both 0 by default: no world is dusty or Tiberium-hazed unless its
**	profile says so.  The Tiberium haze is also only drawn near a generated field.
*/
struct ProceduralWorldWeatherSettings
{
	ProceduralWorldWeatherSettings(void) : DustDensity(0.0f),TiberiumDensity(0.0f) { }

	float	DustDensity;			// scaled down by wetness: rain settles dust
	float	TiberiumDensity;		// full inside a field, fading to nothing one radius beyond it
};


struct ProceduralWorldVehicleSubsystemSettings
{
	ProceduralWorldVehicleSubsystemSettings(void) :
		Enabled(false),AllowImmobilize(false),
		EngineDamageStartFraction(0.66f),MobilityDamageStartFraction(0.5f),
		WeaponDamageStartFraction(0.66f),
		MinEngineFactor(0.25f),MinMobilityFactor(0.35f),MinWeaponFactor(0.3f),
		ImmobilizeFraction(0.05f),WeaponDisableFraction(0.0f),
		RetreatToRepairFraction(0.3f),
		SensorDamageStartFraction(0.66f),MinSensorFactor(0.3f) { }

	bool	Enabled;						// master toggle; OFF (default) leaves canonical vehicle behavior untouched
	bool	AllowImmobilize;				// Section 23 step 5's explicit sub-toggle; OFF means health alone never forces a stop

	//	Each is the 0..1 health fraction *below which* that subsystem begins to degrade, so 0
	//	switches that one subsystem's degradation off entirely (nothing is below zero health)
	//	while leaving the others alone -- a per-subsystem off switch, distinct from `Enabled`,
	//	which turns the whole feature off.  Proven every way in terrain_vehiclesubsystems.
	float	EngineDamageStartFraction;	// engine: acceleration and sustained propulsion
	float	MobilityDamageStartFraction;	// track/wheel: turning
	float	WeaponDamageStartFraction;		// turret/weapon module: rate of fire

	//	Where each subsystem's own efficiency bottoms out at zero health.  A floor rather than
	//	zero is Section 23 step 5 in numeric form: degradation alone never removes a capability
	//	outright -- only the two explicit cut-off rules below do that.
	float	MinEngineFactor;				// 0..1 engine factor floor at zero health
	float	MinMobilityFactor;			// 0..1 mobility factor floor at zero health
	float	MinWeaponFactor;				// 0..1 rate-of-fire factor floor at zero health

	//	The two capability cut-offs, each opt-in and each inert by default.  ImmobilizeFraction is
	//	consulted only when AllowImmobilize is true; WeaponDisableFraction follows this file's own
	//	"0 turns this rule off" convention, since nothing is below zero health -- so by default a
	//	damaged weapon fires ever more slowly rather than ever falling silent.
	float	ImmobilizeFraction;			// 0..1, requires AllowImmobilize: health at/below which movement is zeroed
	float	WeaponDisableFraction;			// 0..1, 0 turns this rule off: health at/below which the weapon stops entirely

	//	Section 23.3/Phase 22 step 8's "AI RETREAT_TO_REPAIR response": the health fraction at or
	//	below which a vehicle with no human driver walks itself to the nearest same-team repair
	//	bay, via the same ScriptEngine::Action_Goto every other order in this tree already uses.
	//	A human driver's own choices are never overridden.
	float	RetreatToRepairFraction;

	//	Schema 9 (D-001, Section 23 sensor module): an AI-driven vehicle's sight range shrinks with
	//	its health on the same curve as the weapon module, with the same defaults.  Never a human's:
	//	a player-driven vehicle's sensor factor is always 1.
	float	SensorDamageStartFraction;
	float	MinSensorFactor;				// 0..1 sight-range factor floor at zero health
};


/*
**	Section 17A.1's required profile thresholds, named exactly as that section lists them.
**	Parsed and validated starting this phase even though no generator/analyzer consumes them
**	yet (Phase 15/17A's own job) -- Phase 1's purpose is the whole schema contract, and
**	re-touching this file from a later phase to add fields it should already have found here
**	would be exactly the kind of rework Section 34.0's phase discipline exists to avoid.
*/
struct ProceduralWorldFairnessThresholds
{
	ProceduralWorldFairnessThresholds(void) :
		MaxInitialResourceTravelTimeDifferenceSeconds(15.0f),
		MaxProjectedInitialIncomeDifferencePercent(10.0f),
		MaxBaseToFrontTravelTimeDifferenceSeconds(20.0f),
		MaxBaseToBaseTravelTimeDifferenceSeconds(20.0f),
		MinBaseToBaseTravelTimeSeconds(0.0f),
		MinVehicleApproachCount(2),
		MinInfantryApproachCount(2),
		MinBaseExpansionArea(4096.0f),
		AllowSingleBridgeDependency(false),
		MaxMandatoryExposedInfantryRunDistance(60.0f),
		MaxMandatoryExposedInfantryRunTimeSeconds(12.0f),
		PreferredCoverInterval(25.0f),
		MinVehicleLaneClearance(1.0f)
		{ }

	float	MaxInitialResourceTravelTimeDifferenceSeconds;	// >= 0
	float	MaxProjectedInitialIncomeDifferencePercent;		// >= 0, percent (0..100 typical)
	float	MaxBaseToFrontTravelTimeDifferenceSeconds;			// >= 0
	float	MaxBaseToBaseTravelTimeDifferenceSeconds;			// >= 0
	float	MinBaseToBaseTravelTimeSeconds;				// >= 0, minimum route time between opposing bases
	int		MinVehicleApproachCount;								// >= 1 -- Section 10A.7's normal target is 2
	int		MinInfantryApproachCount;								// >= 1
	float	MinBaseExpansionArea;									// world units^2, > 0
	bool		AllowSingleBridgeDependency;							// profile opt-in; default false per Section 17A.1
	float	MaxMandatoryExposedInfantryRunDistance;			// world units, >= 0
	float	MaxMandatoryExposedInfantryRunTimeSeconds;		// seconds, >= 0
	float	PreferredCoverInterval;								// world units, > 0
	float	MinVehicleLaneClearance;								// world units, >= 0 -- Section 3A.2's per-profile floor
};


class ProceduralWorldProfile
{
	public:

		//	Section 3B.6 -- a persisted world's identity.  SchemaVersion is this struct's own
		//	layout/field-set version (bumped when a field is added/removed/renamed);
		//	GeneratorVersion identifies the *algorithm* that will consume it (bumped when Phase
		//	2 onward changes what a given seed+profile produces).  The two are independent: a
		//	schema can gain an optional field without the generator output changing, and a
		//	generator algorithm change does not by itself require a new schema field.
		//	Schema 2 added Section 5.5's rim/face roughness, toe width and collision
		//	simplification to the boundary block (Phase 3).  Generator 2 is the cliff ring
		//	itself: the same seed and the same schema-1 profile produce a world with cliffs in
		//	it now where before it produced only an invisible boundary, so the algorithm
		//	identity had to move with it.  A schema-1 profile still loads -- the four new fields
		//	take their documented defaults -- which is the point of versioning them separately.
		//
		//	Schema 3 added the terrain block's elevation range, buildable-slope cap, feature
		//	scale, ridge sharpness, plateau chance and boundary-approach flattening (Phase 4).
		//	Generator 3 is the macro terrain generator: a world with `world.fit_to_level` false
		//	now stands on a generated landform instead of the flat plane a heightfield-less
		//	world always was.  A schema-1 or schema-2 profile still loads; the six new fields
		//	take their documented defaults.  A `fit_to_level` world's own ground is unaffected
		//	either way -- see ProceduralWorldManager::Build_Macro_Terrain for why terrain
		//	generation is skipped there entirely.
		//
		//	Schema 4 added the water block's river/stream width, accumulation threshold and
		//	channel-count cap (Phase 6).  Generator 4 is the hydrology generator: a world with
		//	`world.fit_to_level` false may now carve rivers/streams/lakes/ponds into its own
		//	generated terrain where before it stood dry. A schema-1/2/3 profile still loads; the
		//	four new fields take their documented defaults.  Phase 5's interior cliffs did not
		//	move either number -- it added no profile field, reusing `terrain.
		//	interior_cliff_density`, reserved since Phase 1.
		//
		//	Schema 5 added the resource block's field radius range, initial yield range, crystal
		//	density, harvest anchor count, Harvester load amount and growth fields (Phase 12).
		//	Generator 5 is the Tiberium field generator: a world with `features.tiberium_fields`
		//	true and a strategic plan's own resource candidates may now carry real, harvestable
		//	fields where before a resource candidate was only a scored point. A schema-1/2/3/4
		//	profile still loads; the eight new fields take their documented defaults. Phases 7-11
		//	moved no schema number -- each reused fields Phase 1 had already reserved for it
		//	(Fairness, Retry) rather than needing a new one.
		//	Schema 6 added the grading block's four Phase 17 limits.  Generator 6 is the
		//	Commander's foundation grading itself: a world built by generator 5 was terrain no
		//	runtime actor could reshape, and one built by generator 6 is terrain the Commander may
		//	level within those limits, so two worlds from the same seed are no longer the same
		//	world once a match has been played in them.  A schema-1..5 profile still loads; the
		//	four new fields take their documented defaults.  Phase 16 moved neither number -- it
		//	added no profile field, reading everything it needed from the chunk registry.
		//	Phases 18-20 moved neither number either, for the same reason: journal/snapshot/
		//	subcomponent state all derive from data the schema already describes.
		//	Schema 7 added the logistics block's three Phase 21 toggles.  Generator stays at 6:
		//	`LogisticsGraphService` only reads terrain/road/building state generator 6 already
		//	produced, the same way Phase 16's world integration and Phase 18's operation journal
		//	do, so the same seed and the same schema-1..6 profile still produce byte-identical
		//	terrain -- a schema field can be added with no generator output change, exactly the
		//	independence this comment block has documented since schema 2.  A schema-1..6 profile
		//	still loads; the three new fields take their documented (inert-by-default) values.
		//	Schema 8 added the vehicle_subsystems block's nine Phase 22 fields.  Generator stays
		//	at 6 for the same reason schema 7 did not move it: VehicleGameObj reads its own
		//	already-generated DefenseObject health at control time, the same "derived read of
		//	data the schema already describes" every schema-only phase since 16 has been.  A
		//	schema-1..7 profile still loads; the nine new fields take their documented
		//	(canonical-behavior-preserving) defaults.
		//	Generator 7 (V2 close) adds tunnels (Phase 10: bores, portal cuts in the ground and
		//	cliff faces, road splits at tunnel mouths), the Section 9.4 coastline (water.coastline,
		//	parsed since schema 1, finally consumed) and Section 10.6 street lamps, so the same seed
		//	and profile now produce a different world than generator 6 did.  Schema stays 8 --
		//	every one of the three reads a field or switch the schema already carried.
		//
		//	Compatibility rules (V2 Phase 30; docs/procedural_world/NetworkingAndPersistence.md):
		//	a profile of any schema 1..CURRENT loads, every field it predates taking its documented
		//	default; a newer schema is refused.  generator_version, when a profile states one, must
		//	equal CURRENT_GENERATOR_VERSION -- no older generator is kept (one canonical
		//	implementation), so a profile naming another one names a world this build cannot
		//	reproduce, and is refused rather than silently generating a different one.
		// Native terrain relief and base-route fields extend schema 9; changed terrain
		// shaping and hydrology require a new deterministic generator identity.
		// Generator 9 reserves a native river bridge and hashes its placement.
		// Schema 11 adds the D-003 map plan block; generator 10 realizes terrain from it.
		// Schema 12 adds the hybrid rewrite's plan fields (archetype, biome selectors, road
		// density, island chance, route redundancy, allow_fords); generator 11 is the hybrid
		// generator identity and makes legacy lane_width_cells/settlements inert on that path.
		// Generator 12 widens Section 10.2's cliff talus range to 1W..5.25W (Q-021, D-016): the same
		// seed now draws different cliff widths, so every cliff, window and route downstream differs.
		// Generator 13 is the Q-022/Q-023 crossing ownership correction: Phase 9 freezes only structural
		// sequences a road of the edge's class can drive, Phase 10 enters every frozen crossing along its axis,
		// Phase 11 compiles attached crossings graded over the whole realized road, and Section 12.1 treats a
		// pinned height as its quantization cell -- the same seed now builds different topologies and roads.
		// Generator 14 is the Q-024/Q-025 stabilization: the overpass number is a target, each heavy family is one
		// continuous physical trunk, independent roads keep their roadbeds apart, entrance corridors stay out of
		// Tiberium reserves, and junctions compile at shared topology nodes.
		// Generator 15 makes an ordinary route guide a strategic anchor only: the search passes its cell, the
		// centerline is not bound to it, and Phase 9's pose connector treats it as the same droppable corner.
		// Generator 16 is the first identity with the finalized post-stabilization routing (D-021 and its revision)
		// and the HIGHWAY_RAMP records (Section 12.6, Q-026 auxiliary-lane profile, D-022).  The identity-15
		// development commits after 5384130c are not compatibility fixtures.
		enum { CURRENT_SCHEMA_VERSION = 12, CURRENT_GENERATOR_VERSION = 16 };

		ProceduralWorldProfile(void);

		//	The hybrid generator runs for a plan-enabled world that is not fit to a level.
		bool			Uses_Hybrid_Generator(void) const	{ return Plan.Enabled && !World.FitToLevel; }

		unsigned int	SchemaVersion;
		unsigned int	GeneratorVersion;

		//	Raw, as authored/requested.  WorldSeedService::Normalize_Seed is what subsystem seed
		//	derivation actually consumes -- see worldseedservice.h for why 0 is not used as-is.
		unsigned int	Seed;

		StringClass		Name;			// human-readable only; never part of the hash/manifest identity

		ProceduralWorldDimensions				World;
		ProceduralWorldBoundarySettings		Boundary;
		ProceduralWorldTerrainSettings		Terrain;
		ProceduralWorldWaterSettings			Water;
		ProceduralWorldPlanSettings			Plan;
		ProceduralWorldResourceSettings		Resource;
		ProceduralWorldFeatureFlags			Features;
		ProceduralWorldRetryLimits				Retry;
		ProceduralWorldFairnessThresholds	Fairness;
		ProceduralWorldGradingSettings		Grading;
		ProceduralWorldLogisticsSettings	Logistics;
		ProceduralWorldVehicleSubsystemSettings	VehicleSubsystems;
		ProceduralWorldBaseSettings			Base;
		ProceduralWorldWeatherSettings		Weather;
};


class ProceduralWorldProfileManager
{
	public:

		//	Parses json_text directly -- what the self check uses, with no file on disk.
		static bool		Load_Profile_From_Text(const char * json_text,ProceduralWorldProfile * out_profile,
																StringClass * error_out);

		//	Reads the file, then behaves exactly like Load_Profile_From_Text.
		static bool		Load_Profile_From_File(const char * file_path,ProceduralWorldProfile * out_profile,
															  StringClass * error_out);

		//	The strict validation Phase 1 step 3 requires: finite positive world dimensions,
		//	valid chunk size, cliff band smaller than the usable world, valid min/max cliff
		//	heights, air ceiling above the terrain/cliff limits, nonnegative densities, bounded
		//	retry counts, and every fairness threshold finite and within its documented sign.
		static bool		Validate_Profile(const ProceduralWorldProfile & profile,StringClass * error_out);
};

#endif // PROCEDURALWORLDPROFILE_H
