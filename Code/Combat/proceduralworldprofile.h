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
	ProceduralWorldDimensions(void) : Width(4096.0f),Height(4096.0f),ChunkSize(128.0f),SeaLevel(0.0f) { }
	float	Width;			// world units, > 0
	float	Height;			// world units, > 0
	float	ChunkSize;		// world units, > 0, must divide Width/Height for a whole chunk grid
	float	SeaLevel;		// world units, the water-plane height every hydrology/terrain height is relative to
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
														  HardBarrierOffset(20.0f),AirCeiling(260.0f) { }
	StringClass	Shape;					// generator style tag; Phase 2 defines the legal set, not this schema
	float			CliffBandWidth;		// world units, > 0, must fit inside the world without consuming the playable interior
	float			CliffMinHeight;		// world units, > 0
	float			CliffMaxHeight;		// world units, >= CliffMinHeight
	float			HardBarrierOffset;	// world units past the cliff band where actors/geometry are hard-rejected
	float			AirCeiling;				// world units above sea level; must clear CliffMaxHeight (Section 1.5's aircraft rule)
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
														 InteriorCliffDensity(0.16f),ValleyDensity(0.30f) { }
	float	MacroRelief;				// normalized weight, 0..1
	float	HillDensity;				// normalized weight, 0..1
	float	MountainDensity;			// normalized weight, 0..1
	float	InteriorCliffDensity;	// normalized weight, 0..1
	float	ValleyDensity;				// normalized weight, 0..1
};


struct ProceduralWorldWaterSettings
{
	ProceduralWorldWaterSettings(void) : Rivers(true),Lakes(true),Ponds(true),Coastline(true) { }
	bool	Rivers;
	bool	Lakes;
	bool	Ponds;
	bool	Coastline;
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
		enum { CURRENT_SCHEMA_VERSION = 1, CURRENT_GENERATOR_VERSION = 1 };

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
