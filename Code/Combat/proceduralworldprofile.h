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
														 MinElevation(-30.0f),MaxElevation(160.0f),
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


struct ProceduralWorldFeatureFlags
{
	ProceduralWorldFeatureFlags(void) : Forests(true),Settlements(true),Tunnels(true),
													 Highways(true),TiberiumFields(true) { }
	bool	Forests;
	bool	Settlements;
	bool	Tunnels;
	bool	Highways;
	bool	TiberiumFields;
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
		enum { CURRENT_SCHEMA_VERSION = 4, CURRENT_GENERATOR_VERSION = 4 };

		ProceduralWorldProfile(void);

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
		ProceduralWorldFeatureFlags			Features;
		ProceduralWorldRetryLimits				Retry;
		ProceduralWorldFairnessThresholds	Fairness;
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
