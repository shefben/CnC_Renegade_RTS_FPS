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
 *                     $Archive:: /Commando/Code/Combat/proceduralworldmanager.cpp            $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "proceduralworldmanager.h"
#include "bridgedamageservice.h"

#include "baseselectionplanner.h"
#include "biomedecorationgenerator.h"
#include "boundarycliffgenerator.h"
#include "bridgeplanner.h"
#include "castres.h"
#include "combat.h"
#include "combatspacevalidator.h"
#include "generatedworldintegrator.h"
#include "hydrologygenerator.h"
#include "watersystem.h"
#include "interiorcliffgenerator.h"
#include "lineseg.h"
#include "logisticsgraphservice.h"
#include "macroterraingenerator.h"
#include "mapplangenerator.h"
#include "expansionregionregistry.h"
#include "foundationgrading.h"
#include "generatedworldplacementrules.h"
#include "minimapbaselayer.h"
#include "navmeshgenerator.h"
#include "physcoltest.h"
#include "playertype.h"
#include "roadgraphgenerator.h"
#include "roadsystem.h"
#include "proceduralassetregistry.h"
#include "settlementgenerator.h"
#include "strategicplanner.h"
#include "proceduralbaselayout.h"
#include "tiberiumfieldgenerator.h"
#include "tunnelplanner.h"
#include "worldmetrics.h"
#include "transportationnetworkvalidator.h"
#include "proceduralboundsbuilder.h"
#include "phys.h"
#include "pscene.h"
#include "worldboundsservice.h"
#include "worldseedservice.h"
#include "worldshroudmanager.h"
#include "worldspatialindex.h"
#include "worldterrainsystem.h"
#include "wwdebug.h"
#include "wwmath.h"

#include <time.h>


const char * const ProceduralWorldManager::DEFAULT_PROFILE_PATH = "data/procedural_world/default.json";
const char * const ProceduralWorldManager::NATIVE_PROFILE_PATH = "data/procedural_world/native.json";
const char * const ProceduralWorldManager::NATIVE_MAP_NAME = "ow_procedural.mix";

bool	ProceduralWorldManager::Requested		= false;
bool	ProceduralWorldManager::RequestedExact	= false;
bool	ProceduralWorldManager::RequestedNative	= false;
bool	ProceduralWorldManager::WorldActive	= false;

unsigned int	ProceduralWorldManager::RequestedSeed	= 0;
StringClass		ProceduralWorldManager::RequestedProfilePath;

ProceduralWorldProfile		ProceduralWorldManager::ActiveProfile;
WorldGenerationManifest		ProceduralWorldManager::Manifest;
HybridMetricCache				ProceduralWorldManager::HybridMetrics;
bool								ProceduralWorldManager::HybridMetricsValid	= false;
HybridMapIdentity				ProceduralWorldManager::HybridIdentity;
HybridArchetypeIntentSet	ProceduralWorldManager::HybridIntents;
bool								ProceduralWorldManager::HybridIdentityValid	= false;
HybridStrategicGraph		ProceduralWorldManager::HybridGraph;
bool								ProceduralWorldManager::HybridGraphValid	= false;
HybridCoarseMatrices		ProceduralWorldManager::HybridCoarse;
bool								ProceduralWorldManager::HybridCoarseValid	= false;
HybridLandformSet			ProceduralWorldManager::HybridLandforms;
bool								ProceduralWorldManager::HybridLandformsValid	= false;
HybridHydrologySet			ProceduralWorldManager::HybridHydrology;
bool								ProceduralWorldManager::HybridHydrologyValid	= false;
HybridMacroTerrainSet		ProceduralWorldManager::HybridMacro;
bool								ProceduralWorldManager::HybridMacroValid		= false;
HybridAnchorSet				ProceduralWorldManager::HybridAnchors;
bool								ProceduralWorldManager::HybridAnchorsValid	= false;
HybridTopology				ProceduralWorldManager::HybridTopologyRecord;
bool								ProceduralWorldManager::HybridTopologyValid	= false;
HybridRouteSet				ProceduralWorldManager::HybridRoutes;
HybridRouteParams			ProceduralWorldManager::HybridRouteInputs;
HybridInteractionSet		ProceduralWorldManager::HybridInteractions;
bool							ProceduralWorldManager::HybridInteractionsValid		= false;
HybridPreflightInputs	ProceduralWorldManager::HybridInputs;
bool								ProceduralWorldManager::HybridRoutesValid	= false;
HybridScratchArena			ProceduralWorldManager::HybridArena;
HybridWorkCounters			ProceduralWorldManager::HybridWork;


/*
**	How far apart cliff stations are placed around the ring, and how many stations one chunk
**	carries.  These are generator tuning rather than world description -- Section 5.5's profile
**	list does not name either -- so they live here beside the generator that consumes them
**	instead of widening the schema with numbers no profile author has an opinion about.
**
**	Twelve units is close enough that a ring reads as rock rather than as a fence of panels at
**	the sizes a stock map runs to; sixteen stations a chunk keeps any single cliff mesh to a
**	couple of hundred triangles, which is the bounded-mesh property Phase 3 requires.
*/
static const float CLIFF_STATION_SPACING		= 12.0f;
static const int	 CLIFF_STATIONS_PER_CHUNK	= 16;

/*
**	How far the top of the cliff leans back from its foot, per unit of height.  Escarpments
**	default to a dead vertical wall; a slight batter keeps the face clear of whatever terrain
**	stands behind it and reads as rock rather than as a flat.
*/
static const float CLIFF_BATTER = 0.08f;

/*
**	The smallest level extent worth fitting a world to.  A level that reports something smaller
**	than this has effectively reported nothing -- an empty physics scene, or one holding a
**	single object -- and the profile's authored dimensions are used instead.
*/
static const float MINIMUM_FITTED_EXTENT = 50.0f;


/*
**	Build_Fitted_Terrain's own tuning.  How far above the level's own measured ceiling and below
**	its own measured floor the sampling ray reaches -- generous on purpose, so a tall interior or
**	ground that dips below the level's reported floor is still inside the cast rather than reading
**	as open sky.
*/
static const float FITTED_TERRAIN_RAY_MARGIN = 500.0f;


void ProceduralWorldManager::Init(void)
{
	Clear_Request ();
	Destroy_World ();
}


void ProceduralWorldManager::Shutdown(void)
{
	Destroy_World ();
	Clear_Request ();
	HybridMetricsValid = false;
}


bool ProceduralWorldManager::Is_Native_Map(const char * map_name)
{
	return (map_name != nullptr) && (::stricmp (map_name,NATIVE_MAP_NAME) == 0);
}


void ProceduralWorldManager::Clear_Request(void)
{
	Requested				= false;
	RequestedExact			= false;
	RequestedNative			= false;
	RequestedSeed			= 0;
	RequestedProfilePath	= "";
}


bool ProceduralWorldManager::Request_World_Exact(unsigned int seed,const char * profile_path,StringClass * error_out)
{
	if (seed == 0) {
		if (error_out != nullptr) *error_out = "an exact world request needs the seed the server used";
		return false;
	}
	if (!Request_World (seed,profile_path,error_out)) {
		return false;
	}
	RequestedExact = true;
	return true;
}


bool ProceduralWorldManager::Request_World(unsigned int seed,const char * profile_path,StringClass * error_out)
{
	const char * path = ((profile_path != nullptr) && (profile_path[0] != 0))
							  ? profile_path : DEFAULT_PROFILE_PATH;

	/*
	**	The profile is loaded and validated now, at the moment of the request, and then thrown
	**	away -- Generate_World loads it again when the level is up.  That looks wasteful and is
	**	not: the point is to refuse a bad request while the player is still looking at a menu,
	**	rather than after a level load, and re-reading later means a profile edited between the
	**	click and the load is still picked up.
	*/
	ProceduralWorldProfile probe;
	if (!ProceduralWorldProfileManager::Load_Profile_From_File (path,&probe,error_out)) {
		return false;
	}
	if (!ProceduralWorldProfileManager::Validate_Profile (probe,error_out)) {
		return false;
	}
	if (probe.Uses_Hybrid_Generator ()) {
		HybridPreflightResult preflight;
		if (!Hybrid_Preflight_Request_Check (probe,false,&preflight)) {
			if (error_out != nullptr) {
				error_out->Format ("hybrid preflight: %s (%s)",Hybrid_Preflight_Failure_Name (preflight.Failure),preflight.Detail.Peek_Buffer ());
			}
			return false;
		}
	}

	/*
	**	A seed of zero means "choose one".  Time is the only source of variety available here,
	**	and the value it produces is immediately fixed, logged and carried into the manifest --
	**	so this is a seed chosen once, not a world that is different every frame.  Everything
	**	downstream sees an ordinary explicit seed and has no idea where it came from.
	*/
	unsigned int chosen = seed;
	if (chosen == 0) {
		chosen = WorldSeedService::Normalize_Seed ((unsigned int)::time (nullptr));
	}

	Requested				= true;
	RequestedExact			= false;
	RequestedNative			= !probe.World.FitToLevel;
	RequestedSeed			= chosen;
	RequestedProfilePath	= path;

	WWDEBUG_SAY (("ProceduralWorldManager: world requested -- seed %u, profile '%s'\r\n",
					  chosen,path));
	return true;
}


/***********************************************************************************************
 * Fit_Profile_To_Level -- size the world to the level it is being laid over                     *
 *                                                                                             *
 *	The right size for a ring enclosing a hand-authored map is a measurable property of that     *
 *	map, so it is measured rather than guessed.  PhysicsSceneClass::Get_Level_Extents is the     *
 *	existing answer to "how big is this level"; no second extent is computed here.               *
 *                                                                                             *
 *	The world is made LARGER than the level, not fitted to it.  Every band the boundary defines  *
 *	-- approach, cliff, hard barrier -- is measured inward from the boundary polygon, so a ring  *
 *	drawn on the level's own outline would put its cliffs on top of the outermost ground a       *
 *	player can already walk on.  Growing outward by the full inset instead leaves the entire     *
 *	authored level inside PLAYABLE_INTERIOR and stands the cliffs on the empty ground beyond it. *
 *                                                                                             *
 *	The inset is two cliff-band widths plus the hard barrier because Phase 2 derives the         *
 *	approach band as one more cliff-band width; if that derivation ever becomes a real profile   *
 *	field, this expression is the second place that has to learn about it.                       *
 *=============================================================================================*/
bool ProceduralWorldManager::Fit_Profile_To_Level(ProceduralWorldProfile * profile)
{
	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance ();
	if (scene == nullptr) {
		return false;
	}

	Vector3 min_corner (0.0f,0.0f,0.0f);
	Vector3 max_corner (0.0f,0.0f,0.0f);
	scene->Get_Level_Extents (min_corner,max_corner);

	float level_width  = max_corner.X - min_corner.X;
	float level_height = max_corner.Y - min_corner.Y;

	if ((level_width < MINIMUM_FITTED_EXTENT) || (level_height < MINIMUM_FITTED_EXTENT) ||
		 !WWMath::Is_Valid_Float (level_width) || !WWMath::Is_Valid_Float (level_height)) {
		WWDEBUG_SAY (("ProceduralWorldManager: level reports no usable extent; "
						  "using the profile's own dimensions\r\n"));
		return false;
	}

	float inset = (profile->Boundary.CliffBandWidth * 2.0f) + profile->Boundary.HardBarrierOffset;

	profile->World.Width		= level_width  + (inset * 2.0f);
	profile->World.Height	= level_height + (inset * 2.0f);
	profile->World.CenterX	= (min_corner.X + max_corner.X) * 0.5f;
	profile->World.CenterY	= (min_corner.Y + max_corner.Y) * 0.5f;

	/*
	**	The cliffs stand on the level's floor, not on an abstract sea level, or a ring fitted to
	**	a map whose ground sits well above or below zero would float or be buried.
	*/
	profile->World.SeaLevel = min_corner.Z;

	WWDEBUG_SAY (("ProceduralWorldManager: fitted to level -- %.1f x %.1f about (%.1f,%.1f), floor %.1f\r\n",
					  profile->World.Width,profile->World.Height,
					  profile->World.CenterX,profile->World.CenterY,profile->World.SeaLevel));
	return true;
}


bool ProceduralWorldManager::Build_Cliff_Ring(const ProceduralWorldProfile & profile,StringClass * error_out)
{
	/*
	**	The ring is built from the same polygon the boundary service was just configured with,
	**	rebuilt from the profile rather than read back out of the service.  Both are pure
	**	functions of the profile, so they cannot differ -- and asking the builder keeps this
	**	function usable with no boundary installed, which is what lets the self check drive it.
	*/
	DynamicVectorClass<Vector2> polygon;
	if (!ProceduralBoundsBuilder::Build_Boundary_Polygon (profile,&polygon,error_out)) {
		return false;
	}

	BoundaryCliffParams params;
	params.Seed							= WorldSeedService::Derive_Subsystem_Seed (
													profile.Seed,WorldSeedService::SEED_DOMAIN_BOUNDARY_CLIFFS);
	params.CliffMinHeight			= profile.Boundary.CliffMinHeight;
	params.CliffMaxHeight			= profile.Boundary.CliffMaxHeight;
	params.CliffBandWidth			= profile.Boundary.CliffBandWidth;
	params.ToeWidth					= profile.Boundary.ToeWidth;
	params.RimRoughness				= profile.Boundary.RimRoughness;
	params.FaceRoughness				= profile.Boundary.FaceRoughness;
	params.CollisionSimplification= profile.Boundary.CollisionSimplification;
	params.BaseHeight					= profile.World.SeaLevel;
	params.StationSpacing			= CLIFF_STATION_SPACING;
	params.StationsPerChunk			= CLIFF_STATIONS_PER_CHUNK;
	params.Batter						= CLIFF_BATTER;
	params.LayerIndex					= -1;		// the first cliff-kind terrain layer

	return BoundaryCliffGenerator::Build_Ring (polygon,params,error_out);
}


/***********************************************************************************************
 * ProceduralWorldManager::Build_Map_Plan -- D-003 Phase 3B                                    *
 *=============================================================================================*/
/***********************************************************************************************
 * Build_Hybrid_Map_Identity -- rewrite item I3, runtime Phase 2                                 *
 *                                                                                             *
 * The hybrid generator's first runtime phase: it decides what kind of map this attempt's seed  *
 * is and freezes the Section-6.10 intents later phases consume instead of re-reading the       *
 * archetype name.  Everything it reads is already fixed -- the validated profile and the       *
 * preflight metric cache -- so it queries no live registry, and it touches no terrain.         *
 *=============================================================================================*/
bool ProceduralWorldManager::Build_Hybrid_Map_Identity(const ProceduralWorldProfile & profile,bool * built_out,
																		  StringClass * error_out)
{
	if (built_out != nullptr) *built_out = false;
	HybridIdentityValid	= false;
	HybridIdentity			= HybridMapIdentity ();
	HybridIntents.Reset ();

	//	The legacy generator has no Phase 2 at all: correctly skipped, not a failure.
	if (!profile.Uses_Hybrid_Generator ()) {
		return true;
	}

	//	Preflight owns every constant and every availability fact Phase 2 reads, and Generate_World
	//	runs it before the first attempt.  Reaching here without it is a programming error rather
	//	than a content problem, so it fails loudly instead of measuring anything itself.
	if (!HybridMetricsValid) {
		if (error_out != nullptr) *error_out = "hybrid map identity requested before preflight";
		return false;
	}

	HybridMapIdentityParams params;
	params.MapPlanSeed	= WorldSeedService::Derive_Subsystem_Seed (profile.Seed,WorldSeedService::SEED_DOMAIN_MAP_PLAN);
	params.HydrologySeed	= WorldSeedService::Derive_Subsystem_Seed (profile.Seed,WorldSeedService::SEED_DOMAIN_HYDROLOGY);
	params.BiomeSeed		= WorldSeedService::Derive_Subsystem_Seed (profile.Seed,WorldSeedService::SEED_DOMAIN_BIOMES);
	params.WorldCenterX	= profile.World.CenterX;
	params.WorldCenterY	= profile.World.CenterY;
	params.Constants		= HybridMetrics.Constants;

	/*
	**	Validate_Profile has already accepted every spelling below, so a parse failure here would
	**	mean the validator and the plan's own name tables disagree.  Falling back to the profile
	**	default keeps that disagreement from silently becoming a different map.
	*/
	if (!Hybrid_Symmetry_From_Name (profile.Plan.Symmetry.Peek_Buffer (),&params.Symmetry)) {
		params.Symmetry = HYBRID_SYMMETRY_ROTATIONAL;
	}
	if (!Hybrid_Archetype_From_Name (profile.Plan.Archetype.Peek_Buffer (),&params.ArchetypeSelector)) {
		params.ArchetypeSelector = HYBRID_ARCHETYPE_AUTO;
	}
	if (!Hybrid_Biome_Mode_From_Name (profile.Plan.BiomeMode.Peek_Buffer (),&params.BiomeModeSelector)) {
		params.BiomeModeSelector = HYBRID_BIOME_MODE_AUTO;
	}
	if (!Hybrid_Biome_From_Name (profile.Plan.Biome.Peek_Buffer (),&params.BiomeSelector)) {
		params.BiomeSelector = HYBRID_BIOME_AUTO;
	}
	if (!Hybrid_Biome_From_Name (profile.Plan.SecondaryBiome.Peek_Buffer (),&params.SecondaryBiomeSelector)) {
		params.SecondaryBiomeSelector = HYBRID_BIOME_AUTO;
	}

	params.MixedBiomeProbability	= profile.Plan.MixedBiomeProbability;
	params.IslandChance				= profile.Plan.IslandChance;
	params.RouteRedundancy			= profile.Plan.RouteRedundancy;
	params.FlankLanes					= profile.Plan.FlankLanes;
	params.CoastlineEnabled			= profile.Water.Coastline;
	for (int b = 0; b < HYBRID_BIOME_COUNT; b++) {
		params.BiomeAvailable[b] = HybridMetrics.BiomeAvailable[b];
	}

	HybridMapIdentityResult result;
	if (!Hybrid_Resolve_Map_Identity (params,&HybridIdentity,&HybridIntents,&result)) {
		if (error_out != nullptr) {
			error_out->Format ("hybrid map identity: %s (%s)",Hybrid_Map_Identity_Failure_Name (result.Failure),
									 result.Detail.Peek_Buffer ());
		}
		HybridIdentity = HybridMapIdentity ();
		HybridIntents.Reset ();
		return false;
	}

	HybridIdentityValid = true;
	if (built_out != nullptr) *built_out = true;
	return true;
}


/***********************************************************************************************
 * Build_Hybrid_Strategic_Graph -- rewrite item I4, runtime Phase 3                            *
 *                                                                                             *
 * Gameplay roles before geography.  Every input is already frozen -- the Phase-2 MapIdentity,   *
 * the Section-6.10 intent set, the preflight constants and the validated profile -- and the     *
 * placeable boundary Configure_World_Bounds established for this attempt, so nothing here       *
 * measures content or touches terrain.  A role that has no legal position after Section 6.1's   *
 * bounded repair fails the attempt; Phase 8 revalidates these centers later and may not move    *
 * them.                                                                                        *
 *=============================================================================================*/
bool ProceduralWorldManager::Build_Hybrid_Strategic_Graph(const ProceduralWorldProfile & profile,bool * built_out,
																			 StringClass * error_out)
{
	if (built_out != nullptr) *built_out = false;
	HybridGraphValid = false;
	HybridGraph.Reset ();

	//	The legacy generator has no Phase 3: correctly skipped, not a failure.
	if (!profile.Uses_Hybrid_Generator ()) {
		return true;
	}

	//	Phase 2 owns every record Phase 3 reads, and it runs first in Generate_World_Attempt.
	if (!HybridIdentityValid) {
		if (error_out != nullptr) *error_out = "hybrid strategic graph requested before Phase 2";
		return false;
	}

	HybridStrategicGraphParams params;
	params.MapPlanSeed		= WorldSeedService::Derive_Subsystem_Seed (profile.Seed,WorldSeedService::SEED_DOMAIN_MAP_PLAN);
	params.Constants			= HybridMetrics.Constants;
	params.BoundaryMargin	= profile.Plan.BoundaryMarginCells * profile.World.ChunkSize;
	params.FieldRadiusMax	= profile.Resource.FieldRadiusMax;
	params.RiverWidth			= profile.Water.RiverWidth;

	HybridStrategicGraphResult result;
	if (!Hybrid_Build_Strategic_Graph (params,HybridIdentity,HybridIntents,&HybridGraph,&result)) {
		if (error_out != nullptr) {
			error_out->Format ("hybrid strategic graph: %s (%s)",Hybrid_Strategic_Graph_Failure_Name (result.Failure),
									 result.Detail.Peek_Buffer ());
		}
		HybridGraph.Reset ();
		return false;
	}

	HybridGraphValid = true;
	if (built_out != nullptr) *built_out = true;
	return true;
}


/***********************************************************************************************
 * Build_Hybrid_Coarse_Matrices -- rewrite item I5, runtime Phase 4                            *
 *                                                                                             *
 * Broad spatial intent from records that are already frozen.  The Section-16.3 arena is         *
 * reserved here because Phase 4 is the first phase that needs scratch at all, and its budget is *
 * the size-class budget preflight already computed; every later phase reuses the same block.    *
 * A Phase 4 failure is an ordinary seeded attempt failure: the caller retries with a new seed.   *
 *                                                                                             *
 * Phase 4 may move a lower-priority role through Section 6.1 repair, so it is handed the        *
 * strategic graph mutable -- the graph stays the one owner of every role position.               *
 *=============================================================================================*/
bool ProceduralWorldManager::Build_Hybrid_Coarse_Matrices(const ProceduralWorldProfile & profile,bool * built_out,
																			  StringClass * error_out)
{
	if (built_out != nullptr) *built_out = false;
	HybridCoarseValid = false;
	HybridCoarse.Reset ();

	//	The legacy generator has no Phase 4: correctly skipped, not a failure.
	if (!profile.Uses_Hybrid_Generator ()) {
		return true;
	}

	//	Phase 3 owns every role position Phase 4 reserves, and it runs first in Generate_World_Attempt.
	if (!HybridGraphValid) {
		if (error_out != nullptr) *error_out = "hybrid coarse matrices requested before Phase 3";
		return false;
	}

	/*
	**	One arena per attempt (Section 16.3), sized by the size-class budget preflight computed.  An
	**	existing block big enough is reused; Reset hands Phase 4 an empty arena without returning the
	**	block to the allocator.
	*/
	const int capacity = HybridMetrics.Budgets.ScratchArenaBytes;
	if (HybridArena.Get_Capacity () < capacity) {
		HybridArena.Release ();
		if (!HybridArena.Reserve (capacity)) {
			if (error_out != nullptr) {
				error_out->Format ("hybrid scratch arena: %d byte reservation failed",capacity);
			}
			return false;
		}
	}
	HybridArena.Reset ();
	HybridWork.Reset ();

	HybridCoarseMatrixParams params;
	params.MapPlanSeed	= WorldSeedService::Derive_Subsystem_Seed (profile.Seed,WorldSeedService::SEED_DOMAIN_MAP_PLAN);
	params.BiomeSeed		= WorldSeedService::Derive_Subsystem_Seed (profile.Seed,WorldSeedService::SEED_DOMAIN_BIOMES);
	params.Constants		= HybridMetrics.Constants;
	params.WorldMinX		= profile.World.CenterX - (0.5f * profile.World.Width);
	params.WorldMinY		= profile.World.CenterY - (0.5f * profile.World.Height);
	params.WorldWidth		= profile.World.Width;
	params.WorldHeight	= profile.World.Height;
	params.BoundaryMargin	= profile.Plan.BoundaryMarginCells * profile.World.ChunkSize;
	params.FieldRadiusMin	= profile.Resource.FieldRadiusMin;
	params.FieldRadiusMax	= profile.Resource.FieldRadiusMax;
	params.RiverWidth		= profile.Water.RiverWidth;
	params.TierCount		= profile.Plan.TierCount;
	params.Arena			= &HybridArena;
	params.Counters		= &HybridWork;

	HybridCoarseMatrixResult result;
	if (!Hybrid_Build_Coarse_Matrices (params,HybridIdentity,HybridIntents,&HybridGraph,&HybridCoarse,&result)) {
		if (error_out != nullptr) {
			error_out->Format ("hybrid coarse matrices: %s (%s)",Hybrid_Coarse_Matrix_Failure_Name (result.Failure),
										 result.Detail.Peek_Buffer ());
		}
		HybridCoarse.Reset ();
		return false;
	}

	HybridCoarseValid = true;
	if (built_out != nullptr) *built_out = true;
	return true;
}


/***********************************************************************************************
 * Build_Hybrid_Landforms -- rewrite item I6, runtime Phase 5                                   *
 *                                                                                             *
 * Vector geography before any heightfield.  Phase 4's matrices are handed over mutable because  *
 * `LandformMask` and `RoughnessClass` are Phase 5's own layers: Phase 4 deliberately leaves     *
 * both clear, and the two exclusions this phase honours -- `BASE_PROTECTED` and Section 6.9's   *
 * `NO_IMMUTABLE_BARRIER` -- are read out of those same matrices rather than re-derived here.    *
 *                                                                                             *
 * The arena and the work counters are the ones Phase 4 already reserved for this attempt, so    *
 * the candidate arrays share that block instead of reserving a second one.  A Phase 5 failure   *
 * is an ordinary seeded attempt failure: the caller retries with a new seed.                    *
 *=============================================================================================*/
bool ProceduralWorldManager::Build_Hybrid_Landforms(const ProceduralWorldProfile & profile,bool * built_out,
																	 StringClass * error_out)
{
	if (built_out != nullptr) *built_out = false;
	HybridLandformsValid = false;
	HybridLandforms.Reset ();

	//	The legacy generator has no Phase 5: correctly skipped, not a failure.
	if (!profile.Uses_Hybrid_Generator ()) {
		return true;
	}

	//	Phase 4 owns the layers Phase 5 writes into, and it runs first in Generate_World_Attempt.
	if (!HybridCoarseValid) {
		if (error_out != nullptr) *error_out = "hybrid landforms requested before Phase 4";
		return false;
	}

	HybridLandformParams params;
	params.TerrainSeed	= WorldSeedService::Derive_Subsystem_Seed (profile.Seed,WorldSeedService::SEED_DOMAIN_TERRAIN);
	params.Constants		= HybridMetrics.Constants;
	params.WorldMinX		= profile.World.CenterX - (0.5f * profile.World.Width);
	params.WorldMinY		= profile.World.CenterY - (0.5f * profile.World.Height);
	params.WorldWidth		= profile.World.Width;
	params.WorldHeight	= profile.World.Height;
	params.SeaLevel		= profile.World.SeaLevel;
	params.MinElevation	= profile.Terrain.MinElevation;
	params.MaxElevation	= profile.Terrain.MaxElevation;
	params.TierStep		= profile.Plan.TierStep;
	params.TierCount		= profile.Plan.TierCount;
	params.MacroRelief	= profile.Terrain.MacroRelief;
	params.HillDensity	= profile.Terrain.HillDensity;
	params.MountainDensity		= profile.Terrain.MountainDensity;
	params.InteriorCliffDensity	= profile.Terrain.InteriorCliffDensity;
	params.ValleyDensity			= profile.Terrain.ValleyDensity;
	//	Section 5's canyon/ravine bridge quota is a floor on the world, so Phase 5 owns satisfying it.
	params.CrossedCanyonSystemsRequired = Hybrid_Canyon_Bridge_Quota (HybridMetrics.Constants.SizeClass);
	params.PlateauChance			= profile.Terrain.PlateauChance;
	params.MaxAlternateCandidatesPerFeature	= profile.Retry.MaxAlternateCandidatesPerFeature;
	params.MinVehicleLaneClearance				= profile.Fairness.MinVehicleLaneClearance;
	params.Arena			= &HybridArena;
	params.Counters		= &HybridWork;

	HybridLandformResult result;
	if (!Hybrid_Build_Landforms (params,HybridIdentity,HybridIntents,HybridGraph,&HybridCoarse,
											  &HybridLandforms,&result)) {
		if (error_out != nullptr) {
			error_out->Format ("hybrid landforms: %s (%s)",Hybrid_Landform_Failure_Name (result.Failure),
										 result.Detail.Peek_Buffer ());
		}
		HybridLandforms.Reset ();
		return false;
	}

	HybridLandformsValid = true;
	if (built_out != nullptr) *built_out = true;
	return true;
}


/***********************************************************************************************
 * Build_Hybrid_Hydrology -- rewrite item I7, runtime Phase 6                                   *
 *                                                                                             *
 * Water as vectors, before any heightfield exists.  The provisional drainage surface is the     *
 * semantic tier plus the Phase-5 landform sum and nothing else, so drainage agrees with the     *
 * geography Phase 5 already fixed; the matrices are handed over mutable because the accepted    *
 * water intent, the crossing-window reservations and their `NO_DECORATION` approach stamps are  *
 * this phase's own writes.                                                                     *
 *                                                                                             *
 * `WaterDefinitionClass::BedDepth` is the only carve-depth source the plan allows, so the two   *
 * depths Phase 6 can select are read out of the canonical `WaterSystem` definitions here rather *
 * than being invented in the generator.  A Phase 6 failure is an ordinary seeded attempt        *
 * failure: the caller retries with a new seed.                                                 *
 *=============================================================================================*/
bool ProceduralWorldManager::Build_Hybrid_Hydrology(const ProceduralWorldProfile & profile,bool * built_out,
																	 StringClass * error_out)
{
	if (built_out != nullptr) *built_out = false;
	HybridHydrologyValid = false;
	HybridHydrology.Reset ();

	//	The legacy generator has no Phase 6: correctly skipped, not a failure.
	if (!profile.Uses_Hybrid_Generator ()) {
		return true;
	}

	//	Phase 5 owns the landforms this phase drains over, and it runs first in Generate_World_Attempt.
	if (!HybridLandformsValid) {
		if (error_out != nullptr) *error_out = "hybrid hydrology requested before Phase 5";
		return false;
	}

	HybridHydrologyParams params;
	params.HydrologySeed	= WorldSeedService::Derive_Subsystem_Seed (profile.Seed,WorldSeedService::SEED_DOMAIN_HYDROLOGY);
	params.Constants		= HybridMetrics.Constants;
	params.WorldMinX		= profile.World.CenterX - (0.5f * profile.World.Width);
	params.WorldMinY		= profile.World.CenterY - (0.5f * profile.World.Height);
	params.WorldWidth		= profile.World.Width;
	params.WorldHeight	= profile.World.Height;
	params.SeaLevel		= profile.World.SeaLevel;
	params.MinElevation	= profile.Terrain.MinElevation;
	params.MaxElevation	= profile.Terrain.MaxElevation;
	params.TierStep		= profile.Plan.TierStep;
	//	Step 18's pass candidates are planned road geometry, so they need the grading allowance.
	params.MaxCutDepth	= profile.Grading.MaxCutDepth;
	params.MaxFillHeight	= profile.Grading.MaxFillHeight;
	params.TierCount		= profile.Plan.TierCount;
	params.ChunkSize		= profile.World.ChunkSize;

	params.Rivers			= profile.Water.Rivers;
	params.Lakes			= profile.Water.Lakes;
	params.Coastline		= profile.Water.Coastline;
	params.RiverWidth		= profile.Water.RiverWidth;
	params.StreamWidth	= profile.Water.StreamWidth;
	params.RiverAccumulationChunks	= profile.Water.RiverAccumulationChunks;
	params.MaxRiverCount				= profile.Water.MaxRiverCount;
	params.WaterFraction				= profile.Plan.WaterFraction;

	/*
	**	The canonical carve depths.  Define_Default_Water is add-or-replace by name, so calling it is
	**	always safe and guarantees the two definitions exist before they are read; a definition a mod
	**	has replaced keeps its own depth, which is the point of reading them here.
	*/
	WaterSystem::Define_Default_Water ();
	const WaterDefinitionClass * river_water = WaterSystem::Find_Definition ("ow_water_river");
	const WaterDefinitionClass * lake_water = WaterSystem::Find_Definition ("ow_water_lake");
	params.RiverBedDepth	= (river_water != nullptr) ? river_water->Get_Bed_Depth () : 0.0f;
	params.LakeBedDepth	= (lake_water != nullptr) ? lake_water->Get_Bed_Depth () : 0.0f;

	params.MaxLocalRetriesPerStage			= profile.Retry.MaxLocalRetriesPerStage;
	params.MaxAlternateCandidatesPerFeature	= profile.Retry.MaxAlternateCandidatesPerFeature;
	params.MinVehicleLaneClearance			= profile.Fairness.MinVehicleLaneClearance;

	params.HeavyVehicleWidth	= HybridMetrics.HeavyVehicleWidth;
	params.HeavyVehicleHeight	= HybridMetrics.HeavyVehicleHeight;
	params.HarvesterWidth		= HybridMetrics.HarvesterWidth;

	/*
	**	Step 18's bridge capability is asked of the class that actually crosses: Section 9.5's heavy
	**	route class carries the mandatory crossings, and a class no registered bridge covers reports a
	**	zero clear width, which makes `bridge_capable` false instead of guessing a kind.
	*/
	const HybridRoadClass crossing_class = HybridMetrics.HeavyRouteClass;
	params.BridgeMaxSpanLength	= HybridMetrics.BridgeMaxSpanLength;
	params.BridgeRequiredWidth	= (HybridMetrics.BridgeForClass[crossing_class] >= 0)
										  ? HybridMetrics.BridgeRequiredWidth[crossing_class] : 0.0f;
	params.BridgeApproachLength		= HybridMetrics.BridgeApproachLength;
	params.BridgeWaterClearanceMargin= HybridMetrics.BridgeWaterClearanceMargin;

	params.TunnelsAvailable		= HybridMetrics.TunnelsAvailable && profile.Features.Tunnels;
	params.TunnelRequiredWidth	= HybridMetrics.TunnelRequiredWidth;
	params.TunnelRequiredHeight= HybridMetrics.TunnelRequiredHeight;

	/*
	**	Fords: the profile switch, preflight's answer about the runtime capability, and that
	**	capability's depth limits.  No fordability query exists yet (`FORD_CAPABILITY_UNAVAILABLE`),
	**	so the depths stay zero and no window carries a ford flag until one ships.
	*/
	params.AllowFords						= profile.Plan.AllowFords;
	params.FordsAvailable				= HybridMetrics.FordsAvailable;
	params.FordMaxWaterDepthLight		= 0.0f;
	params.FordMaxWaterDepthHarvester= 0.0f;

	params.Arena		= &HybridArena;
	params.Counters	= &HybridWork;

	HybridHydrologyResult result;
	if (!Hybrid_Build_Hydrology (params,HybridIdentity,HybridIntents,HybridGraph,HybridLandforms,
											 &HybridCoarse,&HybridHydrology,&result)) {
		if (error_out != nullptr) {
			error_out->Format ("hybrid hydrology: %s (%s)",Hybrid_Hydrology_Failure_Name (result.Failure),
										 result.Detail.Peek_Buffer ());
		}
		HybridHydrology.Reset ();
		return false;
	}

	HybridHydrologyValid = true;
	if (built_out != nullptr) *built_out = true;
	return true;
}


//	Defined with Build_Macro_Terrain below, which is the other phase that reads realized ground.
static bool Sample_Terrain_Height(const Vector2 & world,float * height_out,void * context);


/***********************************************************************************************
 * Build_Hybrid_Anchors -- rewrite item I9, runtime Phase 8                                     *
 *                                                                                             *
 * The first phase that reads realized ground rather than planning records, and the only one     *
 * whose entire job is to say no.  Phase 3 chose every centre and Phase 4 reserved it before      *
 * geology and hydrology existed, so a hard requirement the realized world does not meet is an    *
 * attempt failure -- moving a centre here would move it into terrain nothing reserved.           *
 *                                                                                             *
 * `V_REF` is the existing fairness reference speed exactly as `BaseSelectionPlanner` uses it:    *
 * world-unit distances divided by that number, with no unit conversion invented, which is what   *
 * Section 0.10 means by "use those values unchanged".                                            *
 *=============================================================================================*/
bool ProceduralWorldManager::Build_Hybrid_Anchors(const ProceduralWorldProfile & profile,bool * built_out,
																	StringClass * error_out)
{
	if (built_out != nullptr) *built_out = false;
	HybridAnchorsValid = false;
	HybridAnchors.Reset ();

	//	The legacy generator has no Phase 8: correctly skipped, not a failure.
	if (!profile.Uses_Hybrid_Generator ()) {
		return true;
	}

	//	Phase 7 owns the ground Phase 8 validates; a hybrid attempt without it is not a Phase-8 question.
	if (!HybridMacroValid) {
		return true;
	}
	if (!HybridHydrologyValid || !HybridLandformsValid || !HybridCoarseValid || !HybridGraphValid) {
		if (error_out != nullptr) *error_out = "hybrid anchors requested before Phases 3 to 6";
		return false;
	}

	HybridAnchorParams params;
	params.Constants		= HybridMetrics.Constants;
	params.SeaLevel		= profile.World.SeaLevel;
	params.MaxBuildableSlopeRadians	= (float)DEG_TO_RAD (profile.Terrain.MaxBuildableSlopeDegrees);
	params.FieldRadiusMax				= profile.Resource.FieldRadiusMax;
	params.ReferenceVehicleSpeed		= HybridMetrics.ReferenceVehicleSpeed;
	params.MaxInitialResourceTravelTimeDifferenceSeconds =
													profile.Fairness.MaxInitialResourceTravelTimeDifferenceSeconds;
	params.MaxBaseToFrontTravelTimeDifferenceSeconds =
													profile.Fairness.MaxBaseToFrontTravelTimeDifferenceSeconds;
	params.MinBaseExpansionArea		= profile.Fairness.MinBaseExpansionArea;
	params.MinVehicleApproachCount	= profile.Fairness.MinVehicleApproachCount;
	params.MinVehicleLaneClearance	= profile.Fairness.MinVehicleLaneClearance;
	params.HeavyVehicleWidth			= HybridMetrics.HeavyVehicleWidth;

	HybridAnchorResult result;
	if (!Hybrid_Validate_Anchors (params,HybridIdentity,HybridGraph,HybridCoarse,HybridLandforms,
											HybridHydrology,Sample_Terrain_Height,nullptr,&HybridAnchors,&result)) {
		if (error_out != nullptr) {
			error_out->Format ("hybrid anchors: %s (%s)",Hybrid_Anchor_Failure_Name (result.Failure),
									 result.Detail.Peek_Buffer ());
		}
		HybridAnchors.Reset ();
		return false;
	}

	HybridAnchorsValid = true;
	if (built_out != nullptr) *built_out = true;
	return true;
}


/***********************************************************************************************
 * Build_Hybrid_Topology -- rewrite item I10, runtime Phase 9                                   *
 *                                                                                             *
 * Which roads exist, what class each one is, and which frozen points every one of them must      *
 * pass through.  No terrain path is decided here: Phase 10 routes each edge and may not invent a  *
 * crossing this record does not carry.  The matrices are taken mutable for the one write the      *
 * plan asks for -- each entrance corridor reserved against unrelated Tiberium and decoration.     *
 *                                                                                             *
 * This replaces `RoadGraphGenerator`'s minimum spanning tree on the hybrid path; the                *
 * authored/legacy path still builds its own topology exactly as before.                           *
 *=============================================================================================*/
bool ProceduralWorldManager::Build_Hybrid_Topology(const ProceduralWorldProfile & profile,bool * built_out,
																	 StringClass * error_out)
{
	if (built_out != nullptr) *built_out = false;
	HybridTopologyValid = false;
	HybridTopologyRecord.Reset ();

	//	The legacy generator has no Phase 9: correctly skipped, not a failure.
	if (!profile.Uses_Hybrid_Generator ()) {
		return true;
	}

	//	Phase 8 freezes the centres and exit sectors this phase builds from.
	if (!HybridAnchorsValid) {
		return true;
	}

	HybridTopologyParams params;
	params.Constants					= HybridMetrics.Constants;
	params.RoadDensity				= profile.Plan.RoadDensity;
	params.FieldRadiusMax			= profile.Resource.FieldRadiusMax;
	params.MinVehicleLaneClearance= profile.Fairness.MinVehicleLaneClearance;
	params.HarvesterLength			= HybridMetrics.HarvesterLength;
	params.MaxRoutedEdges			= HybridMetrics.Budgets.MaxRoutedEdges;
	params.TunnelsAvailable			= HybridMetrics.TunnelsAvailable && profile.Features.Tunnels;

	HybridTopologyResult result;
	if (!Hybrid_Build_Topology (params,HybridIdentity,HybridGraph,HybridLandforms,HybridHydrology,
											HybridAnchors,&HybridCoarse,Sample_Terrain_Height,nullptr,
											&HybridTopologyRecord,&result)) {
		if (error_out != nullptr) {
			error_out->Format ("hybrid topology: %s (%s)",Hybrid_Topology_Failure_Name (result.Failure),
									 result.Detail.Peek_Buffer ());
		}
		HybridTopologyRecord.Reset ();
		return false;
	}

	HybridTopologyValid = true;
	if (built_out != nullptr) *built_out = true;
	return true;
}


/***********************************************************************************************
 * Build_Hybrid_Routes -- rewrite item I11, runtime Phase 10                                     *
 *                                                                                             *
 * Where each road Phase 9 decided on actually runs.  The search is bounded in every direction    *
 * the plan bounds it: three corridor envelopes per edge, `8N` A* expansions per route and         *
 * `R*8N` per attempt from the Section-16.1 budgets, and at most                                  *
 * `retry.max_local_retries_per_stage` spline control insertions per envelope.  A mandatory edge    *
 * with no legal spline inside those bounds fails the attempt; an optional cross-link is dropped.  *
 *=============================================================================================*/
bool ProceduralWorldManager::Build_Hybrid_Routes(const ProceduralWorldProfile & profile,bool * built_out,
																 StringClass * error_out)
{
	if (built_out != nullptr) *built_out = false;
	HybridRoutesValid = false;
	HybridRoutes.Reset ();

	//	The legacy generator has no Phase 10: correctly skipped, not a failure.
	if (!profile.Uses_Hybrid_Generator ()) {
		return true;
	}
	if (!HybridTopologyValid) {
		return true;
	}

	HybridRouteParams params;
	params.Constants		= HybridMetrics.Constants;
	params.WorldMinX		= profile.World.CenterX - (0.5f * profile.World.Width);
	params.WorldMinY		= profile.World.CenterY - (0.5f * profile.World.Height);
	params.WorldWidth		= profile.World.Width;
	params.WorldHeight	= profile.World.Height;
	params.MaxCutDepth	= profile.Grading.MaxCutDepth;
	params.MaxFillHeight	= profile.Grading.MaxFillHeight;
	params.RiverWidth		= profile.Water.RiverWidth;
	params.FieldRadiusMax= profile.Resource.FieldRadiusMax;
	params.MaxLocalRetriesPerStage		= profile.Retry.MaxLocalRetriesPerStage;
	params.AStarExpansionsPerRoute		= HybridMetrics.Budgets.AStarExpansionsPerRoute;
	params.AStarExpansionsPerAttempt	= HybridMetrics.Budgets.AStarExpansionsPerAttempt;
	params.Arena			= &HybridArena;
	params.Counters		= &HybridWork;

	HybridRouteResult result;
	if (!Hybrid_Build_Routes (params,HybridIdentity,HybridCoarse,HybridLandforms,HybridHydrology,
										HybridAnchors,HybridTopologyRecord,Sample_Terrain_Height,nullptr,
										&HybridRoutes,&result)) {
		if (error_out != nullptr) {
			error_out->Format ("hybrid routes: %s (%s)",Hybrid_Route_Failure_Name (result.Failure),
									 result.Detail.Peek_Buffer ());
		}
		HybridRoutes.Reset ();
		return false;
	}

	//	Kept for Phase 11's reroute, which re-runs Phase 10 with one edge's envelope floor raised.
	HybridRouteInputs = params;

	HybridRoutesValid = true;
	if (built_out != nullptr) *built_out = true;
	return true;
}



/***********************************************************************************************
 * Build_Hybrid_Interactions -- rewrite item I12, runtime Phase 11                                *
 *                                                                                             *
 * What each crossing actually is.  Phase 10 left every one of them as an unresolved structural    *
 * interval; this decides bridge, ford, surface pass, road cut, ramp pass, tunnel, at-grade         *
 * junction or overpass, in the fixed method order the plan sets out for each obstruction kind.     *
 * Nothing is realized: no RoadSystem, BridgeSystem or TunnelSystem object is created and no        *
 * terrain is touched, which is Phase 12's work.                                                   *
 *                                                                                             *
 * Every metric the solver needs comes from the frozen preflight cache -- the bridge registry and   *
 * `BridgePlannerParams` through `HybridMetrics`, the vehicle envelopes through the same -- so this  *
 * phase measures no content of its own and invents no constant.                                    *
 *=============================================================================================*/
bool ProceduralWorldManager::Build_Hybrid_Interactions(const ProceduralWorldProfile & profile,
																		 bool * built_out,StringClass * error_out)
{
	if (built_out != nullptr) *built_out = false;
	HybridInteractionsValid = false;
	HybridInteractions.Reset ();

	//	The legacy generator has no Phase 11: correctly skipped, not a failure.
	if (!profile.Uses_Hybrid_Generator ()) {
		return true;
	}
	if (!HybridRoutesValid) {
		return true;
	}

	HybridInteractionParams params;
	params.Constants		= HybridMetrics.Constants;
	params.WorldMinX		= profile.World.CenterX - (0.5f * profile.World.Width);
	params.WorldMinY		= profile.World.CenterY - (0.5f * profile.World.Height);
	params.WorldWidth		= profile.World.Width;
	params.WorldHeight	= profile.World.Height;
	params.MaxCutDepth	= profile.Grading.MaxCutDepth;
	params.MaxFillHeight	= profile.Grading.MaxFillHeight;
	params.RiverWidth		= profile.Water.RiverWidth;

	params.AllowFords					= profile.Plan.AllowFords;
	params.FordCapabilityAvailable	= HybridMetrics.FordsAvailable;
	params.TunnelsEnabled			= profile.Features.Tunnels;
	params.TunnelAvailable			= HybridMetrics.TunnelsAvailable;

	params.WorldUnitsPerMeter		= HybridMetrics.Constants.WorldUnitsPerMeter;
	params.HeavyVehicleWidth		= HybridMetrics.HeavyVehicleWidth;
	params.HeavyVehicleHeight		= HybridMetrics.HeavyVehicleHeight;
	params.HarvesterWidth			= HybridMetrics.HarvesterWidth;
	params.LightVehicleWidth		= HybridMetrics.HumveeWidth;
	params.MinVehicleLaneClearance= profile.Fairness.MinVehicleLaneClearance;
	params.TierStep					= profile.Plan.TierStep;

	params.BridgeWaterClearanceMargin = HybridMetrics.BridgeWaterClearanceMargin;
	for (int i = 0; i < HYBRID_ROAD_CLASS_COUNT; ++i) {
		params.BridgeForClass[i]		= HybridMetrics.BridgeForClass[i];
		params.BridgeRequiredWidth[i]	= HybridMetrics.BridgeRequiredWidth[i];
	}

	params.MaxLocalRetriesPerStage	= profile.Retry.MaxLocalRetriesPerStage;

	/*
	**	The registered bridge kinds, flattened for the wwphys layer.
	**
	**	Each kind's maximum supported span goes through `Hybrid_Resolve_Bridge_Max_Span`, which is the plan's
	**	own resolution order: the bridge definition's value first, `BridgePlannerParams::MaxSpanLength`
	**	second.  `BridgeDefinitionClass` exposes deck width, deck thickness, required clearance and support
	**	spacing but no maximum span of its own, so today every kind resolves to the planner's value -- which
	**	is a resolved metric, not a missing one.  Kinds that share it have equal span capability and are
	**	separated by the remaining suitability tests and then the lower registry id.  Preflight refuses the
	**	request when neither source supplies a span at all.
	*/
	const DynamicVectorClass<HybridBridgeInput> & bridges = HybridInputs.Bridges;
	for (int i = 0; i < bridges.Count (); ++i) {

		HybridBridgeKind kind;
		kind.StableId					= i;
		kind.ClearWidth				= bridges[i].Width;
		kind.StructuralDepth			= bridges[i].DeckThickness;
		kind.RequiredClearance		= bridges[i].RequiredClearance;
		kind.MaxSpan					= Hybrid_Resolve_Bridge_Max_Span (0.0f,
																					HybridMetrics.BridgeMaxSpanLength);
		kind.RequiredApproachLength= HybridMetrics.BridgeApproachLength;
		kind.StartSeat					= bridges[i].StartSeat;
		kind.EndSeat					= bridges[i].EndSeat;
		params.BridgeKinds.Add (kind);
	}

	const DynamicVectorClass<HybridTunnelModuleInput> & modules = HybridInputs.TunnelModules;
	for (int i = 0; i < modules.Count (); ++i) {

		HybridTunnelModule one;
		one.StableId			= i;
		one.Length				= modules[i].Length;
		one.ClearWidth			= modules[i].ClearanceWidth;
		one.ClearHeight		= modules[i].ClearanceHeight;
		one.MaxGradeRadians	= modules[i].MaxGradeRadians;
		one.PortalCompatible	= modules[i].PortalCompatible;
		params.TunnelModules.Add (one);
	}

	params.Arena	= &HybridArena;
	params.Counters= &HybridWork;

	/*
	**	Phase 11 through its own owner, which is also where the plan's next-envelope reroute lives: a crossing
	**	no method can compile may spend one more Phase-10 envelope, and the accepted state is always one
	**	Phase-10 run paired with one Phase-11 run over it.
	*/
	HybridRouteResult route_result;
	HybridInteractionResult result;
	int reroutes = 0;
	if (!Hybrid_Solve_Interactions_With_Reroute (HybridRouteInputs,params,HybridIdentity,HybridCoarse,
															 HybridLandforms,HybridHydrology,HybridAnchors,
															 HybridTopologyRecord,Sample_Terrain_Height,nullptr,
															 &HybridRoutes,&route_result,&HybridInteractions,&result,
															 &reroutes)) {
		if (error_out != nullptr) {
			if (route_result.Failure != HYBRID_ROUTE_OK) {
				error_out->Format ("hybrid routes (reroute): %s (%s)",
										 Hybrid_Route_Failure_Name (route_result.Failure),
										 route_result.Detail.Peek_Buffer ());
			} else {
				error_out->Format ("hybrid interactions: %s (%s)",
										 Hybrid_Interaction_Failure_Name (result.Failure),
										 result.Detail.Peek_Buffer ());
			}
		}
		HybridInteractions.Reset ();
		return false;
	}
	if (reroutes > 0) {
		WWDEBUG_SAY (("Hybrid Phase 11: %d reroute(s) spent on wider Phase-10 envelopes\r\n",reroutes));
	}

	HybridInteractionsValid = true;
	if (built_out != nullptr) *built_out = true;
	return true;
}


bool ProceduralWorldManager::Build_Map_Plan(const ProceduralWorldProfile & profile,bool * built_out,StringClass * error_out)
{
	if (built_out != nullptr) *built_out = false;
	MapPlanGenerator::Clear ();

	if (!profile.Plan.Enabled) {
		return true;
	}
	if (profile.World.FitToLevel) {
		WWDEBUG_SAY (("ProceduralWorldManager: map plan skipped -- fit_to_level world\r\n"));
		return true;
	}

	MapPlanParams params;
	params.Seed					= WorldSeedService::Derive_Subsystem_Seed (profile.Seed,WorldSeedService::SEED_DOMAIN_MAP_PLAN);
	params.WorldWidth			= profile.World.Width;
	params.WorldHeight		= profile.World.Height;
	params.ChunkSize			= profile.World.ChunkSize;
	params.CenterX				= profile.World.CenterX;
	params.CenterY				= profile.World.CenterY;
	if (profile.Plan.Symmetry == "none")				params.Symmetry = MAP_PLAN_SYMMETRY_NONE;
	else if (profile.Plan.Symmetry == "mirror_x")	params.Symmetry = MAP_PLAN_SYMMETRY_MIRROR_X;
	else														params.Symmetry = MAP_PLAN_SYMMETRY_ROTATIONAL;
	params.TierCount			= profile.Plan.TierCount;
	params.TierStep			= profile.Plan.TierStep;
	//	Rewrite Section 0.7: lane_width_cells is inert on the hybrid path; the corridor half-width
	//	stays the MapPlanParams default until the hybrid corridor phases replace this planner.
	if (!profile.Uses_Hybrid_Generator ()) {
		params.LaneWidthCells	= profile.Plan.LaneWidthCells;
	}
	params.BaseRadiusCells	= profile.Plan.BaseRadiusCells;
	params.FlankLaneCount	= profile.Plan.FlankLanes;
	params.WaterFraction		= profile.Plan.WaterFraction;
	params.BoundaryMarginCells	= profile.Plan.BoundaryMarginCells;

	if (!MapPlanGenerator::Generate (params,error_out)) {
		return false;
	}

	if (built_out != nullptr) *built_out = true;
	return true;
}


/***********************************************************************************************
 * Build_Macro_Terrain -- roadmap Section 34 Phase 4                                             *
 *                                                                                             *
 *	A world fitted to a hand-authored level already has ground: the level's own W3D geometry,     *
 *	the one a player has always stood on there.  WorldTerrainSystem's heightfield is one           *
 *	rectangular grid with no holes, so generating terrain anywhere in a fitted world -- even a      *
 *	flat plane nobody would notice -- would stand a second, collidable ground under or through       *
 *	the level's real one.  That is exactly Phase 4's own "procedural terrain has a separate           *
 *	collision truth" do-not-proceed condition, so this function refuses to run for one, and does       *
 *	not treat refusing as a failure: it is the correct answer for every world this integration          *
 *	can currently produce, since the only caller of Request_World (the main menu's procedural           *
 *	button) always requests a `fit_to_level` profile.  This is the resolution to the design                *
 *	question Phase 3's own planning note carried forward: coexist by never touching the level's             *
 *	ground, and only paint ground for a world that does not have any yet.                                   *
 *=============================================================================================*/
/*
**	Runtime Phase 7's gate reads the heightfield that was actually written, through the canonical
**	owner, so the gate never assumes which generator produced it.
*/
static bool Sample_Terrain_Height(const Vector2 & world,float * height_out,void * context)
{
	(void)context;
	return WorldTerrainSystem::Sample_Height (world.X,world.Y,height_out);
}


bool ProceduralWorldManager::Build_Macro_Terrain(const ProceduralWorldProfile & profile,
																  bool * built_out,StringClass * error_out)
{
	if (built_out != nullptr) *built_out = false;

	if (profile.World.FitToLevel) {
		//	No second collidable ground -- see Build_Fitted_Terrain's own comment -- but Phases
		//	5-9 gate on WorldTerrainSystem::Has_Chunk_Registry, not on whether MacroTerrainGenerator
		//	itself ran, so sampling the level into a chunk registry is what actually unblocks them.
		if (!Build_Fitted_Terrain (profile,error_out)) {
			return false;
		}
		if (built_out != nullptr) *built_out = true;
		return true;
	}

	MacroTerrainParams params;
	params.Seed							= WorldSeedService::Derive_Subsystem_Seed (
													profile.Seed,WorldSeedService::SEED_DOMAIN_TERRAIN);
	params.WorldWidth					= profile.World.Width;
	params.WorldHeight					= profile.World.Height;
	params.ChunkSize						= profile.World.ChunkSize;
	params.CenterX							= profile.World.CenterX;
	params.CenterY							= profile.World.CenterY;
	params.SeaLevel						= profile.World.SeaLevel;
	params.MinElevation					= profile.Terrain.MinElevation;
	params.MaxElevation					= profile.Terrain.MaxElevation;
	params.LandElevationBias			= profile.Terrain.LandElevationBias;
	params.MaxBuildableSlopeRadians	= (float)DEG_TO_RAD (profile.Terrain.MaxBuildableSlopeDegrees);
	params.FeatureScale					= profile.Terrain.FeatureScale;
	params.MacroRelief					= profile.Terrain.MacroRelief;
	params.HillDensity					= profile.Terrain.HillDensity;
	params.MountainDensity				= profile.Terrain.MountainDensity;
	params.ValleyDensity					= profile.Terrain.ValleyDensity;
	params.RidgeSharpness				= profile.Terrain.RidgeSharpness;
	params.PlateauChance					= profile.Terrain.PlateauChance;
	params.BoundaryApproachFlattening	= profile.Terrain.BoundaryApproachFlattening;
	params.UsePlan							= MapPlanGenerator::Has_Plan ();
	params.PlanDetailWeight				= profile.Plan.DetailWeight;

	/*
	**	Runtime Phase 7 (rewrite item I8).  A hybrid request replaces the noise-geography channels
	**	entirely: the height at every vertex becomes the plan's own expression over the frozen Phase
	**	4-6 records.  Step 5's river longitudinal walk runs first, because the carve reads the water
	**	surfaces it fixes, and the phase gate runs after the heightfield exists.  An authored/legacy
	**	request leaves every one of these fields alone and takes the path it always took.
	*/
	HybridMacroTerrainParams hybrid_params;
	HybridMacroTerrainResult hybrid_result;
	HybridMacroValid = false;
	HybridMacro.Reset ();

	const bool use_hybrid = profile.Uses_Hybrid_Generator () && HybridHydrologyValid;
	if (use_hybrid) {

		hybrid_params.TerrainSeed		= params.Seed;
		hybrid_params.Constants			= HybridMetrics.Constants;
		hybrid_params.SeaLevel			= profile.World.SeaLevel;
		hybrid_params.MinElevation		= profile.Terrain.MinElevation;
		hybrid_params.MaxElevation		= profile.Terrain.MaxElevation;
		hybrid_params.TierStep			= profile.Plan.TierStep;
		hybrid_params.TierCount			= profile.Plan.TierCount;
		hybrid_params.DetailWeight		= profile.Plan.DetailWeight;
		hybrid_params.FeatureScale		= profile.Terrain.FeatureScale;
		hybrid_params.RiverWidth		= profile.Water.RiverWidth;
		hybrid_params.MaxBuildableSlopeRadians = params.MaxBuildableSlopeRadians;

		if (!Hybrid_Resolve_River_Surfaces (hybrid_params,HybridCoarse,HybridLandforms,HybridHydrology,
														&HybridMacro,&hybrid_result)) {
			if (error_out != nullptr) {
				error_out->Format ("hybrid macro terrain: %s (%s)",
										 Hybrid_Macro_Terrain_Failure_Name (hybrid_result.Failure),
										 hybrid_result.Detail.Peek_Buffer ());
			}
			HybridMacro.Reset ();
			return false;
		}

		//	The plan's Phase 7 owns the geography, so the noise-geography path is switched off, not mixed.
		params.UsePlan				= false;
		params.UseHybrid			= true;
		params.HybridParams		= &hybrid_params;
		params.HybridMatrices	= &HybridCoarse;
		params.HybridLandforms	= &HybridLandforms;
		params.HybridHydrology	= &HybridHydrology;
		params.HybridSurfaces	= &HybridMacro;
	}

	if (!MacroTerrainGenerator::Generate (params,error_out)) {
		if (use_hybrid) HybridMacro.Reset ();
		return false;
	}

	if (use_hybrid) {
		if (!Hybrid_Check_Macro_Terrain_Gate (hybrid_params,HybridCoarse,HybridLandforms,HybridHydrology,
															&HybridMacro,Sample_Terrain_Height,nullptr,&hybrid_result)) {
			if (error_out != nullptr) {
				error_out->Format ("hybrid macro terrain gate: %s (%s)",
										 Hybrid_Macro_Terrain_Failure_Name (hybrid_result.Failure),
										 hybrid_result.Detail.Peek_Buffer ());
			}
			HybridMacro.Reset ();
			return false;
		}
		HybridMacroValid = true;
		WWDEBUG_SAY (("Hybrid Phase 7: %d x %d vertices at %.2f, heights %.2f..%.2f, %d protected, "
						  "%d river surfaces, checksum 0x%08X\r\n",
						  HybridMacro.VertexCountX,HybridMacro.VertexCountY,HybridMacro.CellSize,
						  HybridMacro.MinHeight,HybridMacro.MaxHeight,HybridMacro.ProtectedVertices,
						  HybridMacro.Rivers.Count (),HybridMacro.Checksum));
	}

	if (built_out != nullptr) *built_out = true;
	return true;
}


/***********************************************************************************************
 * Build_Fitted_Terrain -- populate the chunk registry for a `fit_to_level` world                *
 *                                                                                             *
 *	Macro terrain never runs for a fitted world -- the level's own W3D geometry stays the only    *
 *	collidable ground there is, per Build_Macro_Terrain's and macroterraingenerator.h's own        *
 *	comments.  But Phases 5-9 never actually read MacroTerrainGenerator; every one of them gates    *
 *	on WorldTerrainSystem::Has_Chunk_Registry, and the registry itself is built purely from          *
 *	HeightfieldClass's own height/slope data (WorldTerrainSystem::Build_Chunk_Registry_Entry reads    *
 *	Heightfield->Get_Patch, nothing else -- no collision patch is ever consulted).  So a heightfield   *
 *	can exist purely as bookkeeping: created, filled with the level's own real sampled heights,          *
 *	registered into a chunk registry, with Build_Collision and Build_Far_Terrain_Representation           *
 *	never called on it.  Nothing about the level's actual collidable ground changes.  This is what           *
 *	closes the fitted-world gap: the one caller that exists (the "Procedural Skirmish" menu button)          *
 *	always requests `fit_to_level`, so this is what makes Phases 5-9 run on a real request instead of          *
 *	only ever behind a synthetic self check.                                                                    *
 *                                                                                             *
 *	Sampling.  One straight-down ray per heightfield vertex, cast through WorldSpatialIndex -- the      *
 *	one canonical spatial query owner (Section 16), the same entry point CommanderModeController's       *
 *	own ground pick already uses -- from above the level's own measured ceiling to below its own          *
 *	measured floor.  The topmost hit is the ground a player standing at that point would actually be       *
 *	on.  A vertex with no hit at all is off the level's own footprint -- Fit_Profile_To_Level always        *
 *	makes the fitted world larger than the level by the boundary inset -- and is written flat at sea         *
 *	level, the same value a cliff-band vertex gets, below.                                                     *
 *                                                                                             *
 *	The immutable cliff band stays immutable the same way MacroTerrainGenerator keeps it that way: any   *
 *	vertex WorldBoundsService classifies at or past WORLD_ZONE_CLIFF_BAND is written flat at sea level     *
 *	and never sampled, so MacroTerrainGenerator::Check_Cliff_Band_Untouched -- which reads only              *
 *	WorldTerrainSystem's own data, never which generator built it -- proves this path the same way it        *
 *	proves the generated-terrain one.                                                                          *
 *=============================================================================================*/
bool ProceduralWorldManager::Build_Fitted_Terrain(const ProceduralWorldProfile & profile,StringClass * error_out)
{
	#define FITFAIL(msg) { if (error_out) *error_out = msg; WorldTerrainSystem::Destroy_Terrain (); return false; }

	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance ();
	if (scene == nullptr)								FITFAIL ("no physics scene to sample the level from");
	if (profile.World.ChunkSize <= 0.0f)			FITFAIL ("chunk size must be positive");
	if (!WWMath::Is_Valid_Float (profile.World.Width) || (profile.World.Width <= 0.0f))
																			FITFAIL ("world width must be positive");
	if (!WWMath::Is_Valid_Float (profile.World.Height) || (profile.World.Height <= 0.0f))
																			FITFAIL ("world height must be positive");

	Vector3 level_min (0.0f,0.0f,0.0f);
	Vector3 level_max (0.0f,0.0f,0.0f);
	scene->Get_Level_Extents (level_min,level_max);

	float cast_top		= level_max.Z + FITTED_TERRAIN_RAY_MARGIN;
	float cast_bottom	= level_min.Z - FITTED_TERRAIN_RAY_MARGIN;

	float cell_size = profile.World.ChunkSize / (float)HeightfieldClass::PATCH_CELLS;

	int vx = (int)WWMath::Ceil (profile.World.Width  / cell_size) + 1;
	int vy = (int)WWMath::Ceil (profile.World.Height / cell_size) + 1;
	if (vx < 2) vx = 2;
	if (vy < 2) vy = 2;

	Vector3 origin (profile.World.CenterX - ((float)(vx - 1) * cell_size * 0.5f),
						 profile.World.CenterY - ((float)(vy - 1) * cell_size * 0.5f),
						 profile.World.SeaLevel);

	if (!WorldTerrainSystem::Create_Terrain (vx,vy,cell_size,origin)) {
		FITFAIL ("WorldTerrainSystem refused to create the fitted terrain");
	}

	bool configured = WorldBoundsService::Is_Configured ();

	float * heights = new float[vx * vy];
	int sampled_count = 0;
	int flat_count		= 0;

	for (int iy = 0; iy < vy; iy++) {
		for (int ix = 0; ix < vx; ix++) {

			int idx = (iy * vx) + ix;
			float wx = origin.X + ((float)ix * cell_size);
			float wy = origin.Y + ((float)iy * cell_size);

			WorldBoundsZoneType zone = configured
				? WorldBoundsService::Classify_Point (Vector2 (wx,wy))
				: WORLD_ZONE_PLAYABLE_INTERIOR;

			if (zone >= WORLD_ZONE_CLIFF_BAND) {
				//	The immutable band's own foot -- see this function's own comment.
				heights[idx] = 0.0f;
				flat_count++;
				continue;
			}

			LineSegClass ray (Vector3 (wx,wy,cast_top),Vector3 (wx,wy,cast_bottom));
			CastResultStruct result;
			//	Terrain only: the fit samples the level's ground, not the props, characters and
			//	decorations standing on it (the stock drop-to-ground probe does the same).
			PhysRayCollisionTestClass raytest (ray,&result,TERRAIN_ONLY_COLLISION_GROUP,COLLISION_TYPE_PHYSICAL);

			if (WorldSpatialIndex::Query_Ray (raytest)) {
				float world_z = cast_top + ((cast_bottom - cast_top) * result.Fraction);
				heights[idx] = world_z - origin.Z;
				sampled_count++;
			} else {
				//	No level geometry under this vertex -- outside the level's own footprint, in
				//	the fitted world's own boundary inset.  Flat at sea level, same as the cliff
				//	band, is correct here for the same reason: there is no real ground to record.
				heights[idx] = 0.0f;
				flat_count++;
			}
		}
	}

	bool set_ok = WorldTerrainSystem::Set_Heights (heights,vx * vy);
	delete [] heights;

	if (!set_ok) {
		FITFAIL ("WorldTerrainSystem refused the sampled heights");
	}

	//	Device-less from here down, same as MacroTerrainGenerator's own Generate: the chunk
	//	registry needs only the heightfield just filled and WorldBoundsService, no collision, no
	//	scene render state.  Build_Collision/Build_Far_Terrain_Representation are deliberately
	//	never called -- that is exactly the second-collidable-ground failure this function exists
	//	to avoid.
	WorldTerrainSystem::Build_Chunk_Registry ((float)DEG_TO_RAD (profile.Terrain.MaxBuildableSlopeDegrees));

	WWDEBUG_SAY (("ProceduralWorldManager: fitted terrain sampled -- %d x %d vertices (%d sampled, %d flat), "
					  "%d x %d chunks, checksum 0x%08X\r\n",
					  vx,vy,sampled_count,flat_count,
					  WorldTerrainSystem::Get_Chunk_Count_X (),WorldTerrainSystem::Get_Chunk_Count_Y (),
					  WorldTerrainSystem::Get_World_Chunk_Checksum ()));

	return true;

	#undef FITFAIL
}


/***********************************************************************************************
 * Build_Interior_Cliffs -- roadmap Section 34 Phase 5                                           *
 *                                                                                             *
 *	Candidates come from the chunk registry Build_Macro_Terrain just built, so this simply has     *
 *	nothing to read when that step was skipped or failed -- the caller passes terrain_built        *
 *	rather than this function re-deriving the same `fit_to_level` check a second place.             *
 *=============================================================================================*/
bool ProceduralWorldManager::Build_Interior_Cliffs(const ProceduralWorldProfile & profile,StringClass * error_out)
{
	InteriorCliffParams params;
	params.Seed = WorldSeedService::Derive_Subsystem_Seed (
							profile.Seed,WorldSeedService::SEED_DOMAIN_INTERIOR_CLIFFS);
	params.MaxBuildableSlopeRadians	= (float)DEG_TO_RAD (profile.Terrain.MaxBuildableSlopeDegrees);
	params.Density							= profile.Terrain.InteriorCliffDensity;
	params.LayerIndex						= -1;		// the first cliff-kind terrain layer

	return InteriorCliffGenerator::Generate (params,error_out);
}


/***********************************************************************************************
 * Build_Hydrology -- roadmap Section 34 Phase 6                                                 *
 *=============================================================================================*/
bool ProceduralWorldManager::Build_Hydrology(const ProceduralWorldProfile & profile,StringClass * error_out)
{
	HydrologyParams params;
	params.Seed							= WorldSeedService::Derive_Subsystem_Seed (
													profile.Seed,WorldSeedService::SEED_DOMAIN_HYDROLOGY);
	params.GenerateRivers				= profile.Water.Rivers;
	params.GenerateLakes					= profile.Water.Lakes;
	params.GeneratePonds					= profile.Water.Ponds;
	params.RiverWidth					= profile.Water.RiverWidth;
	params.StreamWidth					= profile.Water.StreamWidth;
	params.RiverAccumulationChunks	= profile.Water.RiverAccumulationChunks;
	params.MaxRiverCount					= profile.Water.MaxRiverCount;
	//	water.coastline has been parsed since schema 1 and is consumed from generator 7 on.
	params.GenerateCoastline			= profile.Water.Coastline;
	params.SeaLevel						= profile.World.SeaLevel;

	return HydrologyGenerator::Generate (params,error_out);
}


/***********************************************************************************************
 * Build_Strategic_Plan -- roadmap Section 34 Phase 7                                            *
 *=============================================================================================*/
bool ProceduralWorldManager::Build_Strategic_Plan(const ProceduralWorldProfile & profile,StringClass * error_out)
{
	StrategicPlannerParams params;
	params.Seed							= WorldSeedService::Derive_Subsystem_Seed (
													profile.Seed,WorldSeedService::SEED_DOMAIN_BASES);
	params.MinApproachCount			= profile.Fairness.MinVehicleApproachCount;

	return StrategicPlanner::Generate (params,error_out);
}


/***********************************************************************************************
 * Build_Road_Graph -- roadmap Section 34 Phase 8                                                *
 *=============================================================================================*/
bool ProceduralWorldManager::Build_Road_Graph(const ProceduralWorldProfile & profile,StringClass * error_out)
{
	RoadGraphParams params;
	params.Seed							= WorldSeedService::Derive_Subsystem_Seed (
													profile.Seed,WorldSeedService::SEED_DOMAIN_ROADS);
	params.MaxBuildableSlopeRadians	= (float)DEG_TO_RAD (profile.Terrain.MaxBuildableSlopeDegrees);

	params.RequireBridgeCrossing = !profile.World.FitToLevel;
	return RoadGraphGenerator::Generate (params,error_out);
}


/***********************************************************************************************
 * Build_Bridge_Plan -- roadmap Section 34 Phase 9                                               *
 *=============================================================================================*/
bool ProceduralWorldManager::Build_Bridge_Plan(const ProceduralWorldProfile & profile,StringClass * error_out)
{
	BridgePlannerParams params;
	params.Seed							= WorldSeedService::Derive_Subsystem_Seed (
													profile.Seed,WorldSeedService::SEED_DOMAIN_BRIDGES);

	params.MaxApproachGradeRadians = (float)DEG_TO_RAD (profile.Terrain.MaxBuildableSlopeDegrees);
	return BridgePlanner::Generate (params,error_out);
}


/***********************************************************************************************
 * Build_Tunnel_Plan -- roadmap Section 34 Phase 10                                              *
 *                                                                                             *
 *	Section 12A.1's class envelopes, sized from WorldMetricsClass (the loaded definitions' own    *
 *	measured bodies) and the profile's own lane clearance -- nothing is guessed.  A class whose     *
 *	locomotion metrics did not resolve is left invalid, and TunnelPlanner refuses its candidates    *
 *	with "no_clearance_data" rather than building a bore to an invented size.                        *
 *=============================================================================================*/
bool ProceduralWorldManager::Build_Tunnel_Plan(const ProceduralWorldProfile & profile,StringClass * error_out)
{
	if (!profile.Features.Tunnels) {
		return true;		// an explicit opt-out is not a failure
	}

	TunnelPlannerParams params;
	params.Seed							= WorldSeedService::Derive_Subsystem_Seed (
													profile.Seed,WorldSeedService::SEED_DOMAIN_TUNNELS);
	params.BaseProtectedRadius		= WWMath::Sqrt (profile.Fairness.MinBaseExpansionArea / WWMATH_PI);
	params.MaxRepositionAttempts	= profile.Retry.MaxAlternateCandidatesPerFeature;

	float max_buildable = (float)DEG_TO_RAD (profile.Terrain.MaxBuildableSlopeDegrees);
	float lane = profile.Fairness.MinVehicleLaneClearance;

	WorldMetricsStruct infantry;
	bool has_infantry = WorldMetricsClass::Get_Class_Metrics (LOCOMOTION_INFANTRY,&infantry);

	WorldMetricsStruct wheeled, tracked;
	bool has_wheeled = WorldMetricsClass::Get_Class_Metrics (LOCOMOTION_WHEELED,&wheeled);
	bool has_tracked = WorldMetricsClass::Get_Class_Metrics (LOCOMOTION_TRACKED,&tracked);

	if (has_infantry) {
		//	Two abreast is the passing/fighting clearance 12A.1 asks for; a body's own radius
		//	either side is the lateral safety, its own step the vertical one.
		TunnelClearanceClass c;
		c.InternalWidth		= 2.0f * infantry.CorridorWidth;
		c.InternalHeight		= infantry.OverheadClearance;
		c.LateralSafety		= infantry.CollisionRadius;
		c.VerticalSafety		= infantry.StepHeight;
		c.MaxGradeRadians		= WWMath::Min (infantry.MaxClimbableSlope,max_buildable);
		c.MinPortalStraight	= 2.0f * infantry.CorridorWidth;
		params.ClassClearance[TUNNEL_CLASS_INFANTRY_ONLY]	= c;
		params.ClassClearance[TUNNEL_CLASS_CAVE_SHORTCUT]	= c;
	}

	if (has_wheeled || has_tracked) {
		//	The widest and tallest loaded ground vehicle, the lane clearance the profile states,
		//	and the gentlest of their climb limits.
		TunnelClearanceClass c;
		float grade = max_buildable;
		if (has_wheeled) {
			c.InternalWidth	= WWMath::Max (c.InternalWidth,wheeled.CorridorWidth);
			c.InternalHeight	= WWMath::Max (c.InternalHeight,wheeled.OverheadClearance);
			grade					= WWMath::Min (grade,wheeled.MaxClimbableSlope);
			if (wheeled.HasTurningRadius) c.MinBendRadius = WWMath::Max (c.MinBendRadius,wheeled.TurningRadius);
		}
		if (has_tracked) {
			c.InternalWidth	= WWMath::Max (c.InternalWidth,tracked.CorridorWidth);
			c.InternalHeight	= WWMath::Max (c.InternalHeight,tracked.OverheadClearance);
			grade					= WWMath::Min (grade,tracked.MaxClimbableSlope);
			if (tracked.HasTurningRadius) c.MinBendRadius = WWMath::Max (c.MinBendRadius,tracked.TurningRadius);
		}
		c.LateralSafety		= lane;
		c.VerticalSafety		= lane;
		c.MaxGradeRadians		= grade;
		c.MinPortalStraight	= WWMath::Max (c.MinBendRadius,c.InternalWidth);
		params.ClassClearance[TUNNEL_CLASS_VEHICLE]		= c;
		params.ClassClearance[TUNNEL_CLASS_UNDERPASS]	= c;

		if (has_infantry) {
			//	The vehicle envelope plus an explicit infantry lane beside it.
			TunnelClearanceClass m = c;
			m.InternalWidth		= c.InternalWidth + infantry.CorridorWidth;
			m.InternalHeight		= WWMath::Max (c.InternalHeight,infantry.OverheadClearance);
			m.MaxGradeRadians		= WWMath::Min (c.MaxGradeRadians,infantry.MaxClimbableSlope);
			params.ClassClearance[TUNNEL_CLASS_MIXED] = m;
		}
	}

	if (!has_infantry || (!has_wheeled && !has_tracked)) {
		WWDEBUG_SAY (("ProceduralWorldManager: tunnel clearance -- infantry metrics %s, vehicle metrics %s; "
						  "classes without metrics are refused, not guessed\r\n",
						  has_infantry ? "resolved" : "MISSING",(has_wheeled || has_tracked) ? "resolved" : "MISSING"));
	}

	return TunnelPlanner::Generate (params,error_out);
}


/***********************************************************************************************
 * Build_Base_Selection -- roadmap Section 34 Phase 11                                           *
 *=============================================================================================*/
bool ProceduralWorldManager::Build_Base_Selection(const ProceduralWorldProfile & profile,StringClass * error_out)
{
	BaseSelectionParams params;
	//	Same domain Phase 7's own candidate generation already answers to -- Section 4/Phase 1
	//	step 6 closed the seed-domain list at twelve entries, "bases" among them, covering both
	//	the candidates and the selection made from them rather than splitting one concern in two.
	params.Seed											= WorldSeedService::Derive_Subsystem_Seed (
																profile.Seed,WorldSeedService::SEED_DOMAIN_BASES);
	params.MaxInitialResourceTravelTimeDifferenceSeconds	= profile.Fairness.MaxInitialResourceTravelTimeDifferenceSeconds;
	params.MaxBaseToBaseTravelTimeDifferenceSeconds			= profile.Fairness.MaxBaseToBaseTravelTimeDifferenceSeconds;
	params.MinBaseToBaseTravelTimeSeconds					= profile.Fairness.MinBaseToBaseTravelTimeSeconds;
	params.MinBaseExpansionArea									= profile.Fairness.MinBaseExpansionArea;
	params.AllowSingleBridgeDependency							= profile.Fairness.AllowSingleBridgeDependency;
	params.MaxRepairAttemptsPerPair								= profile.Retry.MaxLocalRetriesPerStage;

	return BaseSelectionPlanner::Generate (params,error_out);
}


/***********************************************************************************************
 * Build_Tiberium_Fields -- roadmap Section 34 Phase 12                                          *
 *=============================================================================================*/
bool ProceduralWorldManager::Build_Tiberium_Fields(const ProceduralWorldProfile & profile,StringClass * error_out)
{
	if (!profile.Features.TiberiumFields) {
		return true;		// Directive-1.10-flavoured: an explicit opt-out is not a failure
	}

	TiberiumFieldParams params;
	params.Seed							= WorldSeedService::Derive_Subsystem_Seed (
													profile.Seed,WorldSeedService::SEED_DOMAIN_TIBERIUM);
	params.FieldRadiusMin				= profile.Resource.FieldRadiusMin;
	params.FieldRadiusMax				= profile.Resource.FieldRadiusMax;
	params.InitialYieldMin				= profile.Resource.InitialYieldMin;
	params.InitialYieldMax				= profile.Resource.InitialYieldMax;
	params.CrystalDensity				= profile.Resource.CrystalDensity;
	params.HarvestAnchorCount			= profile.Resource.HarvestAnchorCount;
	params.HarvesterLoadAmount		= profile.Resource.HarvesterLoadAmount;
	params.GrowthEnabled				= profile.Resource.GrowthEnabled;
	params.GrowthRatePerSecond		= profile.Resource.GrowthRatePerSecond;
	params.MaxRepairAttemptsPerField	= profile.Retry.MaxLocalRetriesPerStage;
	params.MaxBuildableSlopeRadians	= (float)DEG_TO_RAD (profile.Terrain.MaxBuildableSlopeDegrees);

	return TiberiumFieldGenerator::Generate (params,error_out);
}


/***********************************************************************************************
 * Build_Settlements -- roadmap Section 34 Phase 13                                              *
 *=============================================================================================*/
bool ProceduralWorldManager::Build_Settlements(const ProceduralWorldProfile & profile,StringClass * error_out)
{
	//	Rewrite Section 0.7: settlement/city controls are inert on the hybrid path.
	if (profile.Uses_Hybrid_Generator ()) {
		return true;
	}

	if (!profile.Features.Settlements) {
		return true;		// Directive-1.10-flavoured: an explicit opt-out is not a failure
	}

	ProceduralAssetRegistry::Init ();

	SettlementParams params;
	params.Seed									= WorldSeedService::Derive_Subsystem_Seed (
														profile.Seed,WorldSeedService::SEED_DOMAIN_SETTLEMENTS);
	params.MaxBuildableSlopeRadians		= (float)DEG_TO_RAD (profile.Terrain.MaxBuildableSlopeDegrees);

	return SettlementGenerator::Generate (params,error_out);
}


/***********************************************************************************************
 * Build_Biome_Decoration -- roadmap Section 34 Phase 14                                         *
 *=============================================================================================*/
bool ProceduralWorldManager::Build_Biome_Decoration(const ProceduralWorldProfile & profile,StringClass * error_out)
{
	if (!profile.Features.Forests) {
		return true;		// Directive-1.10-flavoured: an explicit opt-out is not a failure
	}

	BiomeDecorationParams params;
	params.Seed							= WorldSeedService::Derive_Subsystem_Seed (
													profile.Seed,WorldSeedService::SEED_DOMAIN_BIOMES);
	params.MaxBuildableSlopeRadians	= (float)DEG_TO_RAD (profile.Terrain.MaxBuildableSlopeDegrees);

	return BiomeDecorationGenerator::Generate (params,error_out);
}


/***********************************************************************************************
 * Validate_Combat_Space -- roadmap Section 34 Phase 15                                          *
 *=============================================================================================*/
bool ProceduralWorldManager::Validate_Combat_Space(const ProceduralWorldProfile & profile)
{
	CombatSpaceValidationParams params;
	params.Seed												= profile.Seed;
	params.MaxMandatoryExposedInfantryRunTimeSeconds	= profile.Fairness.MaxMandatoryExposedInfantryRunTimeSeconds;
	params.MinVehicleLaneClearance							= profile.Fairness.MinVehicleLaneClearance;

	return CombatSpaceValidator::Validate (params,nullptr);
}


/***********************************************************************************************
 * Build_World_Integration -- roadmap Section 34 Phase 16 steps 1-2                              *
 *=============================================================================================*/
bool ProceduralWorldManager::Build_World_Integration(const ProceduralWorldProfile & profile,StringClass * error_out)
{
	(void)profile;
	return GeneratedWorldIntegrator::Build (error_out);
}


/***********************************************************************************************
 * Build_Navigation -- roadmap Section 34 Phase 16 steps 3-8                                     *
 *=============================================================================================*/
bool ProceduralWorldManager::Build_Navigation(const ProceduralWorldProfile & profile,StringClass * error_out)
{
	(void)profile;
	NavMeshParams params;
	return NavMeshGenerator::Build (params,error_out);
}


/***********************************************************************************************
 * Build_Minimap_Layer -- roadmap Section 34 Phase 16 step 12                                    *
 *=============================================================================================*/
bool ProceduralWorldManager::Build_Minimap_Layer(const ProceduralWorldProfile & profile,StringClass * error_out)
{
	(void)profile;
	return MinimapBaseLayer::Build (error_out);
}


/***********************************************************************************************
 * Build_Shroud_Coverage -- roadmap Section 34 Phase 16 step 13                                  *
 *=============================================================================================*/
bool ProceduralWorldManager::Build_Shroud_Coverage(const ProceduralWorldProfile & profile,StringClass * error_out)
{
	(void)profile;
	return GeneratedWorldIntegrator::Partition_Shroud_Over_World (error_out);
}


/***********************************************************************************************
 * Arm_Expansion_Rules -- roadmap Section 34 Phase 17 steps 1-2, 5 and 10-13                      *
 *                                                                                               *
 * The Commander's expansion rules do not generate anything; they decide what the Commander may   *
 * do to the world for the rest of the match.  Arming them is three things: the profile's own     *
 * grading limits, the chokepoint set derived from the navigation just built, and an empty        *
 * expansion registry.                                                                            *
 *                                                                                               *
 * Returns false when the chokepoint set could not be derived -- which is exactly the case where  *
 * navigation did not build -- and that is reported rather than hidden: a world whose chokepoints *
 * are unknown is a world where one exclusion is silently not enforced, and a log line saying so  *
 * is worth more than a true this function did not earn.                                          *
 *=============================================================================================*/
bool ProceduralWorldManager::Arm_Expansion_Rules(const ProceduralWorldProfile & profile)
{
	FoundationGradingLimits limits;
	limits.MaxCutDepth				= profile.Grading.MaxCutDepth;
	limits.MaxFillHeight				= profile.Grading.MaxFillHeight;
	limits.MaxSlopeChangeRadians	= profile.Grading.MaxSlopeChangeRadians;
	limits.MaxAffectedArea			= profile.Grading.MaxAffectedArea;
	FoundationGradingService::Set_Limits (limits);

	ExpansionRegionRegistry::Clear ();

	StringClass chokepoint_error;
	if (!GeneratedWorldPlacementRules::Build_Chokepoints (&chokepoint_error)) {
		WWDEBUG_SAY (("ProceduralWorldManager: chokepoint analysis failed (%s); the nav-chokepoint "
						  "placement exclusion has no data to enforce\r\n",
						  chokepoint_error.Peek_Buffer ()));
		return false;
	}

	return true;
}


/***********************************************************************************************
 * Validate_Transportation_Network -- the gap report's item 6                                    *
 *=============================================================================================*/
bool ProceduralWorldManager::Validate_Transportation_Network(const ProceduralWorldProfile & profile)
{
	TransportationValidationParams params;
	params.MaxBuildableSlopeRadians	= (float)DEG_TO_RAD (profile.Terrain.MaxBuildableSlopeDegrees);

	return TransportationNetworkValidator::Validate (params,nullptr);
}


/*
**	Section 27.5.  "Every procedural-world crash/validation failure should log a concise
**	reproduction string."  Built straight from `profile` rather than `Manifest` -- a failed
**	attempt may never have reached the point where `Manifest` itself gets committed, but the
**	profile that attempt actually used to derive its own seed/hash is always in hand right here.
**	`ContentManifestHash` is always 0 today (see worldgenerationmanifest.h's own comment: no
**	generator/asset registry exists yet), the same honest value `Create_Empty_World_Manifest`
**	would compute for it.
*/
static StringClass
Format_Attempt_Reproduction_String(const ProceduralWorldProfile & profile)
{
	WorldGenerationManifest temp;
	temp.GeneratorVersion	= profile.GeneratorVersion;
	temp.Seed					= profile.Seed;
	temp.ProfileHash			= WorldGenerationManifestService::Compute_Profile_Hash (profile);
	temp.ContentManifestHash	= 0;
	if (ProceduralWorldManager::Has_Hybrid_Metrics ()) {
		temp.HybridPlanningInputHash	= ProceduralWorldManager::Get_Hybrid_Metrics ().PlanningInputHash;
		temp.HybridContentRevision		= ProceduralWorldManager::Get_Hybrid_Metrics ().ContentRevision;
	}
	return WorldGenerationManifestService::Format_Reproduction_String (temp);
}


bool ProceduralWorldManager::Generate_World(StringClass * error_out)
{
	//	Directive 1.10.  No request, no procedural anything: this is every hand-authored map.
	if (!Requested) {
		return true;
	}

	//	Gap report item 5's own world-level retry policy.  Attempt 0 always uses the requested
	//	seed exactly -- a player who never needed a retry sees the same world this profile has
	//	always produced for that seed -- and only a genuine failure (required connectivity
	//	broken, or a generation attempt failing outright) ever asks for a different one,
	//	deterministically derived from the same base rather than reaching for the clock.  Bounded
	//	by MAX_WORLD_GENERATION_ATTEMPTS; never loops looking for a connected layout that may not
	//	exist for this profile at all.
	unsigned int base_seed = WorldSeedService::Normalize_Seed (RequestedSeed);
	bool any_attempt_completed = false;
	StringClass last_error;

	//	The metric cache belongs to this request: whatever an earlier request measured says
	//	nothing about this profile's content requirements.
	HybridMetricsValid = false;

	//	A client rebuilding its server's world gets that world's one attempt, never a retry.
	int attempts = RequestedExact ? 1 : MAX_WORLD_GENERATION_ATTEMPTS;

	for (int attempt = 0; attempt < attempts; attempt++) {
		//	A retry must not inherit geometry, reservations or an active manifest from
		//	the preceding candidate.
		if (attempt > 0) {
			Destroy_World ();
			any_attempt_completed = false;
		}

		ProceduralWorldProfile profile;
		if (!ProceduralWorldProfileManager::Load_Profile_From_File (RequestedProfilePath.Peek_Buffer (),
																					  &profile,error_out)) {
			return false;			// the profile itself never loads -- no seed changes that
		}

		profile.Seed = WorldSeedService::Normalize_Seed (base_seed + (unsigned int)attempt);

		//	Rewrite Section 0.7: one concise message per request when the hybrid path meets legacy fields.
		if ((attempt == 0) && profile.Uses_Hybrid_Generator ()
		 && (profile.Plan.LegacyLaneWidthCellsPresent || profile.Features.LegacySettlementsPresent)) {
			WWDEBUG_SAY (("ProceduralWorld: deprecated %s%s%s ignored by the hybrid generator\r\n",
				profile.Plan.LegacyLaneWidthCellsPresent ? "plan.lane_width_cells" : "",
				(profile.Plan.LegacyLaneWidthCellsPresent && profile.Features.LegacySettlementsPresent) ? " and " : "",
				profile.Features.LegacySettlementsPresent ? "world_features.settlements" : ""));
		}

		if (profile.World.FitToLevel) {
			Fit_Profile_To_Level (&profile);		// falls back to the profile's own dimensions
		}

		StringClass validate_error;
		if (!ProceduralWorldProfileManager::Validate_Profile (profile,&validate_error)) {
			if (error_out != nullptr) *error_out = validate_error;
			return false;			// a profile that fails validation fails it on every seed alike
		}

		/*
		**	Hybrid preflight measures content, not the seed, so a failure here fails every attempt
		**	alike -- and it must finish before any seeded stage runs.  Section 17's attempt
		**	teardown keeps the immutable content metrics, so the cache is built by the first
		**	attempt that needs it and reused by the retries: nothing preflight reads changes
		**	with the effective seed.
		*/
		if (!profile.Uses_Hybrid_Generator ()) {
			HybridMetricsValid = false;
		} else if (!HybridMetricsValid) {
			HybridPreflightInputs preflight_inputs;
			preflight_inputs.Gather_From_Runtime (profile);
			HybridPreflightResult preflight;
			if (!Hybrid_Run_Preflight (profile,preflight_inputs,&preflight)) {
				if (error_out != nullptr) {
					error_out->Format ("hybrid preflight: %s (%s)",Hybrid_Preflight_Failure_Name (preflight.Failure),preflight.Detail.Peek_Buffer ());
				}
				return false;
			}
			HybridMetrics = preflight.Metrics;
			HybridInputs = preflight_inputs;
			HybridMetricsValid = true;
			WWDEBUG_SAY (("ProceduralWorld: hybrid preflight passed, content revision %08X, planning input hash %08X\n",
								preflight.Metrics.ContentRevision,preflight.Metrics.PlanningInputHash));
		}

		StringClass attempt_error;
		bool required_connectivity_ok = false;
		if (!Generate_World_Attempt (profile,&attempt_error,&required_connectivity_ok)) {
			last_error = attempt_error;
			WWDEBUG_SAY (("ProceduralWorldManager: attempt %d (seed %u) failed outright (%s)%s -- %s\r\n",
							  attempt,profile.Seed,attempt_error.Peek_Buffer (),
							  (attempt + 1 < MAX_WORLD_GENERATION_ATTEMPTS) ? "; retrying" : "; no attempts left",
						 Format_Attempt_Reproduction_String (profile).Peek_Buffer ()));
			Destroy_World ();
			continue;
		}
		any_attempt_completed = true;

		if (required_connectivity_ok || RequestedExact) {
			if (attempt > 0) {
				WWDEBUG_SAY (("ProceduralWorldManager: required connectivity achieved on retry attempt %d (seed %u)\r\n",
								  attempt,profile.Seed));
			}
			return true;
		}

		WWDEBUG_SAY (("ProceduralWorldManager: attempt %d (seed %u) left a required route disconnected%s -- %s\r\n",
						  attempt,profile.Seed,
						  (attempt + 1 < MAX_WORLD_GENERATION_ATTEMPTS) ? "; retrying with a new seed" : "; no attempts left",
						  Format_Attempt_Reproduction_String (profile).Peek_Buffer ()));
	}

	//	Every bounded attempt either failed outright or left required connectivity broken.  The
	//	world left active is whichever attempt last completed -- Generate_World_Attempt commits
	//	ActiveProfile/Manifest/WorldActive itself, for the attempt it just ran -- never silently
	//	presented as fully connected: the log above already said so for every attempt, plainly,
	//	and TransportationNetworkValidator's own PASS/FAIL line says so again for the one that
	//	stands.
	if (!any_attempt_completed) {
		if (error_out != nullptr) *error_out = last_error;
		return false;
	}

	WWDEBUG_SAY (("ProceduralWorldManager: exhausted %d attempt(s) without achieving required connectivity; "
					  "the last attempt's world stands, not fully connected -- %s\r\n",MAX_WORLD_GENERATION_ATTEMPTS,
					  WorldGenerationManifestService::Format_Reproduction_String (Manifest).Peek_Buffer ()));
	return true;
}


bool ProceduralWorldManager::Generate_World_Attempt(const ProceduralWorldProfile & profile,StringClass * error_out,
																	  bool * required_connectivity_ok_out)
{
	if (required_connectivity_ok_out != nullptr) *required_connectivity_ok_out = false;

	/*
	**	Boundary first, then cliffs.  The order is not cosmetic: the cliff ring's whole purpose
	**	is to explain a boundary that already exists and is already authoritative, and Phase 3's
	**	own do-not-proceed list forbids cliff geometry being the only enforcement.  If the cliff
	**	build fails below, the boundary stays configured and the world is still correctly closed
	**	-- invisible, but closed.
	*/
	if (!ProceduralBoundsBuilder::Configure_World_Bounds (profile,error_out)) {
		return false;
	}

	/*
	**	Runtime Phase 2 (rewrite item I3).  Identity before geography: every later hybrid phase
	**	reads these records, and a Phase 2 failure is a profile/content contradiction that no other
	**	seed rescues, so it is worth finding before any geometry exists.
	*/
	bool identity_built = false;
	StringClass identity_error;
	if (!Build_Hybrid_Map_Identity (profile,&identity_built,&identity_error)) {
		WWDEBUG_SAY (("ProceduralWorldManager: %s\r\n",identity_error.Peek_Buffer ()));
		if (error_out != nullptr) *error_out = identity_error;
		return false;
	}

	/*
	**	Runtime Phase 3 (rewrite item I4).  Roles before geography, and after the boundary, because
	**	every role reserve is tested against the placeable boundary Configure_World_Bounds just
	**	established.  A Phase 3 failure is an ordinary seeded attempt failure: the caller retries.
	*/
	bool graph_built = false;
	StringClass graph_error;
	if (!Build_Hybrid_Strategic_Graph (profile,&graph_built,&graph_error)) {
		WWDEBUG_SAY (("ProceduralWorldManager: %s\r\n",graph_error.Peek_Buffer ()));
		if (error_out != nullptr) *error_out = graph_error;
		return false;
	}

	/*
	**	Runtime Phase 4 (rewrite item I5).  Broad spatial intent before any geometry: the coarse
	**	semantic matrices and the reservation set, which is also where a lower-priority role is moved
	**	if two hard reservations collide.  An ordinary seeded attempt failure.
	*/
	bool coarse_built = false;
	StringClass coarse_error;
	if (!Build_Hybrid_Coarse_Matrices (profile,&coarse_built,&coarse_error)) {
		WWDEBUG_SAY (("ProceduralWorldManager: %s\r\n",coarse_error.Peek_Buffer ()));
		if (error_out != nullptr) *error_out = coarse_error;
		return false;
	}

	/*
	**	Runtime Phase 5 (rewrite item I6).  Vector landforms before any terrain height: the hills,
	**	ridges, explicit cliff runs and canyon/ravine systems, plus the two Section-2.2 layers Phase 4
	**	leaves for this phase.  An ordinary seeded attempt failure.
	*/
	bool landforms_built = false;
	StringClass landform_error;
	if (!Build_Hybrid_Landforms (profile,&landforms_built,&landform_error)) {
		WWDEBUG_SAY (("ProceduralWorldManager: %s\r\n",landform_error.Peek_Buffer ()));
		if (error_out != nullptr) *error_out = landform_error;
		return false;
	}

	/*
	**	Runtime Phase 6 (rewrite item I7).  Vector hydrology before any terrain height: the selected
	**	rivers with their widths and drainage surfaces, the lakes, the coast, the decorative islands
	**	and every crossing/ramp-pass window the road phases may use.  An ordinary seeded attempt
	**	failure.
	*/
	bool hybrid_hydrology_built = false;
	StringClass hybrid_hydrology_error;
	if (!Build_Hybrid_Hydrology (profile,&hybrid_hydrology_built,&hybrid_hydrology_error)) {
		WWDEBUG_SAY (("ProceduralWorldManager: %s\r\n",hybrid_hydrology_error.Peek_Buffer ()));
		if (error_out != nullptr) *error_out = hybrid_hydrology_error;
		return false;
	}

	StringClass cliff_error;
	bool cliffs = Build_Cliff_Ring (profile,&cliff_error);
	if (!cliffs) {
		WWDEBUG_SAY (("ProceduralWorldManager: cliff ring failed (%s); the boundary is still enforced\r\n",
						 cliff_error.Peek_Buffer ()));
		if (!profile.World.FitToLevel) {
			if (error_out != nullptr) *error_out = cliff_error;
			return false;
		}
	}

	//	Map plan, Phase 3B (D-003).  The 2D layout every later phase realizes.  Skipped -- not
	//	a failure -- when the profile leaves it disabled or the world is fitted to a level.
	bool plan_built = false;
	StringClass plan_error;
	if (!Build_Map_Plan (profile,&plan_built,&plan_error)) {
		WWDEBUG_SAY (("ProceduralWorldManager: map plan failed (%s)\r\n",plan_error.Peek_Buffer ()));
		if (error_out != nullptr) *error_out = plan_error;
		return false;
	}

	//	Macro terrain, Phase 4.  Runs after the boundary and the cliff ring for the same reason
	//	they run in that order: the terrain generator reads WorldBoundsService to keep the cliff
	//	band flat, so the boundary has to exist first.  Skipped entirely -- not a failure -- for
	//	a `fit_to_level` world; see Build_Macro_Terrain's own comment.
	bool terrain_built = false;
	StringClass terrain_error;
	if (!Build_Macro_Terrain (profile,&terrain_built,&terrain_error)) {
		WWDEBUG_SAY (("ProceduralWorldManager: required terrain failed (%s)\r\n",
						 terrain_error.Peek_Buffer ()));
		if (error_out != nullptr) *error_out = terrain_error;
		return false;
	}

	/*
	**	Runtime Phase 8 (rewrite item I9).  The frozen Phase-3 centres against the ground Phase 7 just
	**	realized: the protected discs, the starting-building slope, the expansion area, the sixteen exit
	**	sectors and the Section-0.10 provisional fairness numbers.  Nothing moves; a failure here is an
	**	ordinary seeded attempt failure.
	*/
	bool hybrid_anchors_built = false;
	StringClass hybrid_anchor_error;
	if (!Build_Hybrid_Anchors (profile,&hybrid_anchors_built,&hybrid_anchor_error)) {
		WWDEBUG_SAY (("ProceduralWorldManager: %s\r\n",hybrid_anchor_error.Peek_Buffer ()));
		if (error_out != nullptr) *error_out = hybrid_anchor_error;
		return false;
	}

	/*
	**	Runtime Phase 9 (rewrite item I10).  The transportation topology over the frozen anchors: base
	**	entrances, heavy route families with their bound crossing windows, home and field roads, the
	**	quota fills, the optional cross-links and the shared overpass obligations.
	*/
	bool hybrid_topology_built = false;
	StringClass hybrid_topology_error;
	if (!Build_Hybrid_Topology (profile,&hybrid_topology_built,&hybrid_topology_error)) {
		WWDEBUG_SAY (("ProceduralWorldManager: %s\r\n",hybrid_topology_error.Peek_Buffer ()));
		if (error_out != nullptr) *error_out = hybrid_topology_error;
		return false;
	}

	/*
	**	Runtime Phase 10 (rewrite item I11).  Where each of those roads actually runs: the bounded
	**	heading-aware A* through every frozen waypoint, and the spline each accepted cell chain becomes.
	*/
	bool hybrid_routes_built = false;
	StringClass hybrid_route_error;
	if (!Build_Hybrid_Routes (profile,&hybrid_routes_built,&hybrid_route_error)) {
		WWDEBUG_SAY (("ProceduralWorldManager: %s\r\n",hybrid_route_error.Peek_Buffer ()));
		if (error_out != nullptr) *error_out = hybrid_route_error;
		return false;
	}

	/*
	**	Runtime Phase 11 (rewrite item I12).  What each crossing Phase 10 left unresolved actually is:
	**	a bridge, a ford, a surface pass, a road cut, a ramp pass, a tunnel, an at-grade junction or an
	**	overpass, chosen in the fixed method order the plan sets out for each obstruction kind.  Nothing
	**	is realized here; Phase 12 consumes these records.
	*/
	bool hybrid_interactions_built = false;
	StringClass hybrid_interaction_error;
	if (!Build_Hybrid_Interactions (profile,&hybrid_interactions_built,&hybrid_interaction_error)) {
		WWDEBUG_SAY (("ProceduralWorldManager: %s\r\n",hybrid_interaction_error.Peek_Buffer ()));
		if (error_out != nullptr) *error_out = hybrid_interaction_error;
		return false;
	}

	//	Interior cliffs, Phase 5.  Reads the chunk registry Build_Macro_Terrain just built, so it
	//	only runs when that step actually produced one -- terrain_built already carries both the
	//	`fit_to_level` skip and any macro terrain failure, so there is exactly one place that
	//	decides whether a chunk registry exists this run.
	bool interior_cliffs_built = false;
	StringClass interior_cliff_error;
	if (terrain_built) {
		interior_cliffs_built = Build_Interior_Cliffs (profile,&interior_cliff_error);
		if (!interior_cliffs_built) {
			WWDEBUG_SAY (("ProceduralWorldManager: interior cliffs failed (%s); the terrain itself is unaffected\r\n",
							  interior_cliff_error.Peek_Buffer ()));
		}
	}
	if (!profile.World.FitToLevel && profile.Terrain.InteriorCliffDensity > 0.0f &&
		(!interior_cliffs_built || InteriorCliffGenerator::Get_Feature_Count () == 0)) {
		if (error_out != nullptr) *error_out = "native world has no generated interior cliffs";
		return false;
	}

	//	Hydrology, Phase 6.  Reads the same chunk registry interior cliffs does, so it shares the
	//	same gate; runs whether or not interior cliffs themselves succeeded, since a cliff feature
	//	failing does not mean the terrain the chunk registry describes is wrong.
	bool hydrology_built = false;
	StringClass hydrology_error;
	if (terrain_built) {
		hydrology_built = Build_Hydrology (profile,&hydrology_error);
		if (!hydrology_built) {
			WWDEBUG_SAY (("ProceduralWorldManager: hydrology failed (%s); the terrain itself is unaffected\r\n",
							  hydrology_error.Peek_Buffer ()));
		}
	}
	if (!profile.World.FitToLevel && profile.Water.Rivers) {
		bool has_river = false;
		if (hydrology_built) {
			for (int i = 0; i < HydrologyGenerator::Get_Feature_Count (); i++) {
				HydrologyFeature feature;
				if (HydrologyGenerator::Get_Feature (i,&feature) && feature.Type == HYDRO_FEATURE_RIVER) {
					has_river = true;
					break;
				}
			}
		}
		if (!has_river) {
			if (error_out != nullptr) *error_out = "native world has no generated rivers";
			return false;
		}
	}
	if (hydrology_built && PhysicsSceneClass::Get_Instance () != nullptr) {
		for (int i = 0; i < HydrologyGenerator::Get_Feature_Count (); i++) {
			HydrologyFeature feature;
			if (!HydrologyGenerator::Get_Feature (i,&feature)) continue;
			int area = WaterSystem::Find_Area_Index (feature.Name.Peek_Buffer ());
			if (area < 0 || !WaterSystem::Has_Geometry (area)) {
				if (error_out != nullptr) *error_out = "generated water surface geometry is missing";
				return false;
			}
		}
	}
	// Water carving changes slopes and buildability.  Strategic anchors must
	// read the finished wet terrain, not MacroTerrainGenerator's initial grid.
	if (terrain_built && !WorldTerrainSystem::Build_Chunk_Registry (
		(float)DEG_TO_RAD (profile.Terrain.MaxBuildableSlopeDegrees))) {
		if (error_out != nullptr) *error_out = "terrain registry refresh after hydrology failed";
		return false;
	}
	if (interior_cliffs_built && !InteriorCliffGenerator::Restore_Chunk_Claims ()) {
		if (error_out != nullptr) *error_out = "interior cliff exclusions lost after hydrology";
		return false;
	}

	//	Strategic plan, Phase 7.  Reads the same chunk registry; also reads whatever interior
	//	cliff/hydrology candidates exist, optionally, so it runs whether or not either of those
	//	stages themselves succeeded.
	bool strategic_plan_built = false;
	StringClass strategic_plan_error;
	if (terrain_built) {
		strategic_plan_built = Build_Strategic_Plan (profile,&strategic_plan_error);
		if (!strategic_plan_built) {
			WWDEBUG_SAY (("ProceduralWorldManager: strategic plan failed (%s); the terrain itself is unaffected\r\n",
							  strategic_plan_error.Peek_Buffer ()));
		}
	}

	//	Road graph, Phase 8.  Needs the strategic plan to connect, not merely the terrain.
	bool road_graph_built = false;
	StringClass road_graph_error;
	if (strategic_plan_built) {
		road_graph_built = Build_Road_Graph (profile,&road_graph_error);
		if (!road_graph_built) {
			WWDEBUG_SAY (("ProceduralWorldManager: road graph failed (%s); the strategic plan itself is unaffected\r\n",
							  road_graph_error.Peek_Buffer ()));
		}
	}
	if (road_graph_built && !WorldTerrainSystem::Build_Chunk_Registry (
		(float)DEG_TO_RAD (profile.Terrain.MaxBuildableSlopeDegrees))) {
		if (error_out != nullptr) *error_out = "terrain registry refresh after road grading failed";
		return false;
	}
	if (interior_cliffs_built && !InteriorCliffGenerator::Restore_Chunk_Claims ()) {
		if (error_out != nullptr) *error_out = "interior cliff exclusions lost after road grading";
		return false;
	}

	//	Bridge plan, Phase 9.  Needs the road graph -- every candidate comes from an edge's own
	//	unresolved-crossing list.
	bool bridge_plan_built = false;
	StringClass bridge_plan_error;
	if (road_graph_built) {
		bridge_plan_built = Build_Bridge_Plan (profile,&bridge_plan_error);
		if (!bridge_plan_built) {
			WWDEBUG_SAY (("ProceduralWorldManager: bridge plan failed (%s); the road graph itself is unaffected\r\n",
							  bridge_plan_error.Peek_Buffer ()));
		}
	}
	if (!profile.World.FitToLevel && (!bridge_plan_built || !BridgePlanner::Has_Usable_River_Crossing ())) {
		if (error_out != nullptr) *error_out = "generated world needs a usable river bridge with connected road approaches";
		return false;
	}
	if (bridge_plan_built && !WorldTerrainSystem::Build_Chunk_Registry (
		(float)DEG_TO_RAD (profile.Terrain.MaxBuildableSlopeDegrees))) {
		if (error_out != nullptr) *error_out = "terrain registry refresh after bridges failed";
		return false;
	}
	if (interior_cliffs_built && !InteriorCliffGenerator::Restore_Chunk_Claims ()) {
		if (error_out != nullptr) *error_out = "interior cliff exclusions lost after bridges";
		return false;
	}

	//	Tunnels, Phase 10.  After the bridge plan, whose unresolved crossings are one of its two
	//	candidate sources; the other (cliff bypasses) needs only the strategic plan and terrain.
	bool tunnel_plan_built = false;
	StringClass tunnel_plan_error;
	if (terrain_built) {
		tunnel_plan_built = Build_Tunnel_Plan (profile,&tunnel_plan_error);
		if (!tunnel_plan_built) {
			WWDEBUG_SAY (("ProceduralWorldManager: tunnel plan failed (%s); the bridge plan itself is unaffected\r\n",
							  tunnel_plan_error.Peek_Buffer ()));
		}
	}
	if (tunnel_plan_built && !WorldTerrainSystem::Build_Chunk_Registry (
		(float)DEG_TO_RAD (profile.Terrain.MaxBuildableSlopeDegrees))) {
		if (error_out != nullptr) *error_out = "terrain registry refresh after tunnels failed";
		return false;
	}
	if (interior_cliffs_built && !InteriorCliffGenerator::Restore_Chunk_Claims ()) {
		if (error_out != nullptr) *error_out = "interior cliff exclusions lost after tunnels";
		return false;
	}

	//	Base selection, Phase 11.  Needs the road graph and the bridge plan -- Phase 11's own
	//	purpose line: choose bases only once roads, bridges and terrain constraints are known.
	bool base_selection_built = false;
	StringClass base_selection_error;
	if (bridge_plan_built) {
		base_selection_built = Build_Base_Selection (profile,&base_selection_error);
		if (!base_selection_built) {
			WWDEBUG_SAY (("ProceduralWorldManager: base selection failed (%s); the bridge plan itself is unaffected\r\n",
							  base_selection_error.Peek_Buffer ()));
		}
	}
	if (!profile.World.FitToLevel &&
		(!base_selection_built || !BaseSelectionPlanner::Has_Selection ())) {
		if (error_out != nullptr) {
			*error_out = base_selection_error.Get_Length () > 0 ?
				base_selection_error.Peek_Buffer () : "native world has no valid base pair";
		}
		return false;
	}

	//	Tiberium fields, Phase 12.  Needs the base selection -- step 1's own "initial per-team
	//	field" needs a selected base to be initial for.
	bool tiberium_fields_built = false;
	StringClass tiberium_fields_error;
	if (base_selection_built) {
		tiberium_fields_built = Build_Tiberium_Fields (profile,&tiberium_fields_error);
		if (!tiberium_fields_built) {
			WWDEBUG_SAY (("ProceduralWorldManager: Tiberium fields failed (%s); the base selection itself is unaffected\r\n",
							  tiberium_fields_error.Peek_Buffer ()));
		}
	}

	//	Settlements, Phase 13.  Reads Phase 11/12's own reserved regions directly, so it shares
	//	the road-graph gate rather than needing a further one.
	bool settlements_built = false;
	StringClass settlements_error;
	if (tiberium_fields_built || base_selection_built) {
		settlements_built = Build_Settlements (profile,&settlements_error);
		if (!settlements_built) {
			WWDEBUG_SAY (("ProceduralWorldManager: settlements failed (%s); the base selection itself is unaffected\r\n",
							  settlements_error.Peek_Buffer ()));
		}
	}

	//	Biome decoration, Phase 14.  Needs only the chunk registry.
	bool biome_decoration_built = false;
	StringClass biome_decoration_error;
	if (terrain_built) {
		biome_decoration_built = Build_Biome_Decoration (profile,&biome_decoration_error);
		if (!biome_decoration_built) {
			WWDEBUG_SAY (("ProceduralWorldManager: biome decoration failed (%s); the terrain itself is unaffected\r\n",
							  biome_decoration_error.Peek_Buffer ()));
		}
	}

	//	Final network validation, item 6.  Runs regardless of which of the steps above actually
	//	built anything -- with nothing built it trivially passes (0 roads, 0 bridges, nothing to
	//	be broken), which is the correct answer for a hand-authored map with no procedural roads
	//	at all, not a special case to skip around.
	//	Hydrology carves beds, roads grade approaches, and tunnels cut holes after
	//	MacroTerrainGenerator built its first meshes. Commit those edits before
	//	validation and navigation use the finished ground. Fitted worlds retain the
	//	stock level's collision and never create a second terrain surface.
	if (!profile.World.FitToLevel) {
		if (!WorldTerrainSystem::Build_Collision ()) {
			if (error_out != nullptr) *error_out = "final generated terrain collision rebuild failed";
			return false;
		}
		if (!WorldTerrainSystem::Build_Far_Terrain_Representation ()) {
			if (error_out != nullptr) *error_out = "final generated far terrain rebuild failed";
			return false;
		}
	}
	//	The road planner builds splines and grades ground; drawing the finished
	//	carriageways is a separate RoadSystem step. Bridges and tunnels may have
	//	changed the network, so build its meshes after those stages.
	if (!profile.World.FitToLevel && RoadGraphGenerator::Get_Edge_Count () == 0) {
		if (error_out != nullptr) *error_out = "native world has no generated roads";
		return false;
	}
	if (RoadSystem::Get_Road_Count () > 0 && !RoadSystem::Build_Geometry ()) {
		if (error_out != nullptr) *error_out = "generated road geometry could not be built";
		return false;
	}
	bool network_valid = Validate_Transportation_Network (profile);

	//	Phase 15.  Runs after transportation validation over the same finished world; its own
	//	hard-failure verdict is step 12's own "reject... rather than silently accepting", folded
	//	into the retry trigger below rather than refusing the world directly -- see
	//	CombatSpaceValidator's own class comment for why a second repair pass this late is unsafe.
	bool combat_space_valid = Validate_Combat_Space (profile);

	//	Phase 16.  Final generated-world integration, over the same finished, validated world --
	//	never a reason to retry with a fresh seed (a collision/nav/minimap build failure is not a
	//	layout problem a different seed would fix), so these three run for their own side effects
	//	only and are not folded into required_connectivity_ok_out below.
	bool world_integration_built = false;
	StringClass world_integration_error;
	if (settlements_built) {
		world_integration_built = Build_World_Integration (profile,&world_integration_error);
		if (!world_integration_built) {
			WWDEBUG_SAY (("ProceduralWorldManager: world integration failed (%s); settlement structures stay data-only\r\n",
							  world_integration_error.Peek_Buffer ()));
		}
	}

	bool navigation_built = false;
	StringClass navigation_error;
	if (terrain_built) {
		navigation_built = Build_Navigation (profile,&navigation_error);
		if (!navigation_built) {
			WWDEBUG_SAY (("ProceduralWorldManager: navigation failed (%s); PathfindClass has no sectors for this world\r\n",
							  navigation_error.Peek_Buffer ()));
		}
	}

	bool minimap_built = false;
	StringClass minimap_error;
	if (terrain_built) {
		minimap_built = Build_Minimap_Layer (profile,&minimap_error);
		if (!minimap_built) {
			WWDEBUG_SAY (("ProceduralWorldManager: minimap base layer failed (%s)\r\n",
							  minimap_error.Peek_Buffer ()));
		}
	}

	bool shroud_coverage_built = false;
	StringClass shroud_coverage_error;
	if (terrain_built) {
		shroud_coverage_built = Build_Shroud_Coverage (profile,&shroud_coverage_error);
		if (!shroud_coverage_built) {
			WWDEBUG_SAY (("ProceduralWorldManager: shroud coverage failed (%s); the shroud grid still "
							  "covers whatever world it was last partitioned over\r\n",
							  shroud_coverage_error.Peek_Buffer ()));
		}
	}

	/*
	**	Phase 17 steps 1-2/5/10-13.  The Commander's own expansion rules, armed over the world
	**	that has just been accepted.
	**
	**	Nothing is built here in the sense the four blocks above build things: this arms the rules
	**	a placement will be judged by for the rest of the match.  The grading limits come from the
	**	profile (step 5), the chokepoint set is derived from the navigation that was just built
	**	(step 2's last exclusion), and the expansion registry starts empty because no Commander has
	**	built anything yet.
	**
	**	Not folded into required_connectivity_ok_out for the same reason the Phase 16 blocks are
	**	not: a world with no chokepoints, or one whose nav did not build, is not a world a
	**	different seed would fix.
	*/
	bool expansion_rules_armed = Arm_Expansion_Rules (profile);

	/*
	**	V2 roadmap Phase 21 (Section 22).  Section 22.1's own default is inert: with
	**	profile.Logistics.Enabled false (the schema default), nothing here runs and the graph
	**	stays empty, exactly the "no mandatory micromanagement" contract Section 22.1 states.
	**	When armed, the graph is built from data this pass has already produced for real -- the
	**	selected bases, the resolved Tiberium fields and the resolved POIs -- using
	**	RoadGraphGenerator's own real path search for route length and reachability rather than a
	**	second distance estimate.  REFINERY/FACTORY/REPAIR nodes are not added here: those come
	**	from live buildings, which do not exist until a match is actually played, so
	**	LogisticsGraphService::Rebuild_Building_Nodes is a live/console-triggered follow-up, not a
	**	generation-time step.
	*/
	bool logistics_built = false;
	if (profile.Logistics.Enabled) {
		StringClass logistics_error;
		logistics_built = LogisticsGraphService::Rebuild_Nodes (profile.Logistics.ConvoyEnabled,
																				profile.Logistics.ConstructionSupportRadius,
																				&logistics_error);
		if (logistics_built) {
			LogisticsGraphService::Set_Enemy_Threat_Radius (profile.Logistics.EnemyThreatRadius);
			logistics_built = LogisticsGraphService::Rebuild_Routes (&logistics_error);
		}
		if (!logistics_built) {
			WWDEBUG_SAY (("ProceduralWorldManager: logistics graph failed (%s); no Commander logistics "
							  "data exists for this world\r\n",logistics_error.Peek_Buffer ()));
		}
	}

	//	Gap report item 5's own retry trigger, distinct from network_valid above: a narrow
	//	shoulder or an unbound abutment is a real finding but not worth a whole world's own
	//	seed-bumped retry over, while a required base pair actually left unreachable -- or a
	//	Phase 15 hard combat-space failure -- is exactly what item 5/step 12 say must never be
	//	silently accepted.
	//	D-001 / Section 17A.1: base-to-front fairness, over the contested fields both bases can
	//	reach.  An unfair front is a layout a different seed can fix, so it joins the retry trigger;
	//	a world with no front is not checked, and says so.
	bool front_fair = true;
	if (tiberium_fields_built) {
		float front_difference = -1.0f;
		StringClass front_report;
		front_fair = (TiberiumFieldGenerator::Check_Front_Travel_Time_Within_Threshold (
							  profile.Fairness.MaxBaseToFrontTravelTimeDifferenceSeconds,&front_report,&front_difference) == 0);
		if (front_difference < 0.0f) {
			WWDEBUG_SAY (("ProceduralWorldManager: no contested field reachable from both bases; the base-to-front limit is not applied\r\n"));
		} else if (!front_fair) {
			WWDEBUG_SAY (("ProceduralWorldManager: %s",front_report.Peek_Buffer ()));
		}
	}

	if (required_connectivity_ok_out != nullptr) {
		*required_connectivity_ok_out = (!TransportationNetworkValidator::Did_Check_Required_Base_Routes () ||
													 TransportationNetworkValidator::Has_Required_Base_Routes ()) &&
													combat_space_valid && front_fair;
	}

	ActiveProfile = profile;

	/*
	**	The manifest records the profile as actually used, fit-to-level included, so the world
	**	can be reproduced from it.  Recording the profile as authored would make a manifest that
	**	describes a different world than the one it came from.
	*/
	DynamicVectorClass<StringClass> content_identifiers;
	Manifest = WorldGenerationManifestService::Create_Empty_World_Manifest (profile,content_identifiers);
	//	Hybrid plan Section 0.3: a hybrid world's manifest carries the planning input hash and the
	//	structural content revision preflight froze, on top of the legacy profile hash.
	if (HybridMetricsValid) {
		Manifest.HybridPlanningInputHash	= HybridMetrics.PlanningInputHash;
		Manifest.HybridContentRevision	= HybridMetrics.ContentRevision;
	}
	Manifest.CliffRingChecksum			= BoundaryCliffGenerator::Get_Ring_Checksum ();
	//	Read from WorldTerrainSystem directly rather than MacroTerrainGenerator's own cache -- the
	//	registry has exactly one owner regardless of which path built it (generated or fitted),
	//	and Get_Terrain_Checksum() only ever reflects the generated path.
	Manifest.TerrainChecksum			= terrain_built ? WorldTerrainSystem::Get_World_Chunk_Checksum () : 0;
	Manifest.InteriorCliffChecksum	= interior_cliffs_built ? InteriorCliffGenerator::Get_Checksum () : 0;
	Manifest.HydrologyChecksum		= hydrology_built ? HydrologyGenerator::Get_Checksum () : 0;
	Manifest.StrategicPlanChecksum	= strategic_plan_built ? StrategicPlanner::Get_Checksum () : 0;
	Manifest.RoadGraphChecksum		= road_graph_built ? RoadGraphGenerator::Get_Checksum () : 0;

	//	Unlike Phase 7's candidate ids, a road graph edge is a road this generator actually
	//	built through RoadSystem -- realized, not merely proposed -- so SelectedRoadGraphEdgeIds
	//	genuinely means "selected" here.
	if (road_graph_built) {
		for (int i = 0; i < RoadGraphGenerator::Get_Edge_Count (); i++) {
			RoadGraphEdge edge;
			if (RoadGraphGenerator::Get_Edge (i,&edge)) {
				Manifest.SelectedRoadGraphEdgeIds.Add (edge.Id);
			}
		}
	}

	//	Phase 9 -- a resolved bridge, like a road graph edge, is a real object BridgeSystem
	//	actually built, not a candidate, so it belongs in SelectedBridgeCandidateIds for real.
	Manifest.BridgePlanChecksum = bridge_plan_built ? BridgePlanner::Get_Checksum () : 0;
	if (bridge_plan_built) {
		for (int i = 0; i < BridgePlanner::Get_Bridge_Count (); i++) {
			ResolvedBridge bridge;
			if (BridgePlanner::Get_Bridge (i,&bridge)) {
				Manifest.SelectedBridgeCandidateIds.Add (bridge.Id);
			}
		}
	}

	//	Phase 10 -- the candidates a real tunnel was built for, the same "real object, not a
	//	candidate" standard the bridge list above holds to.
	if (tunnel_plan_built) {
		for (int i = 0; i < TunnelPlanner::Get_Tunnel_Count (); i++) {
			ResolvedTunnel tunnel;
			if (TunnelPlanner::Get_Tunnel (i,&tunnel)) {
				Manifest.SelectedTunnelCandidateIds.Add (tunnel.CandidateId);
			}
		}
	}

	//	Phase 16 step 8.  0 until navigation is built; the field's own pre-existing comment
	//	("0 until Phase 16 exists") is finally answered.
	Manifest.NavigationRevision = navigation_built ? NavMeshGenerator::Get_Revision () : 0;

	//	Phase 11 -- unlike Phase 7's own full candidate list, these two ids are an actual
	//	decision: the pair BaseSelectionPlanner chose, not every pair it considered.
	//	InitialBaseLayoutIds stays empty -- Section 16A's own building-transform contract is not
	//	this generator's job; see baseselectionplanner.h's own class comment.
	if (base_selection_built && BaseSelectionPlanner::Has_Selection ()) {
		SelectedBaseSite gdi_site, nod_site;
		if (BaseSelectionPlanner::Get_Selected_Site (PLAYERTYPE_GDI,&gdi_site)) {
			Manifest.SelectedBaseCandidateIds.Add (gdi_site.Id);
		}
		if (BaseSelectionPlanner::Get_Selected_Site (PLAYERTYPE_NOD,&nod_site)) {
			Manifest.SelectedBaseCandidateIds.Add (nod_site.Id);
		}
	}

	//	Phase 12 -- a resolved field, like a resolved bridge, is real: it has a real footprint and
	//	a real Tiberium-mask stamp, not merely a candidate StrategicPlanner already listed.
	if (tiberium_fields_built) {
		for (int i = 0; i < TiberiumFieldGenerator::Get_Field_Count (); i++) {
			ResolvedTiberiumField field;
			if (TiberiumFieldGenerator::Get_Field (i,&field)) {
				Manifest.SelectedResourceFieldIds.Add (field.Id);
			}
		}
	}

	//	Phase 13.
	if (settlements_built) {
		for (int i = 0; i < SettlementGenerator::Get_Settlement_Count (); i++) {
			ResolvedSettlement settlement;
			if (SettlementGenerator::Get_Settlement (i,&settlement)) {
				Manifest.SettlementLayoutIds.Add (settlement.Id);
			}
		}
		for (int i = 0; i < SettlementGenerator::Get_Poi_Count (); i++) {
			ResolvedPoi poi;
			if (SettlementGenerator::Get_Poi (i,&poi)) {
				Manifest.ImportantPoiIds.Add (poi.Id);
			}
		}
	}

	WorldActive = true;

	StringClass reproduction = WorldGenerationManifestService::Format_Reproduction_String (Manifest);
	WWDEBUG_SAY (("ProceduralWorldManager: world generated -- %s\r\n",reproduction.Peek_Buffer ()));
	WWDEBUG_SAY (("ProceduralWorldManager: cliff ring %s -- %d stations in %d chunks, checksum 0x%08X\r\n",
					  cliffs ? "built" : "NOT BUILT",
					  BoundaryCliffGenerator::Get_Station_Count (),
					  BoundaryCliffGenerator::Get_Chunk_Count (),
					  BoundaryCliffGenerator::Get_Ring_Checksum ()));
	WWDEBUG_SAY (("ProceduralWorldManager: macro terrain %s (%s) -- %s\r\n",
					  terrain_built ? "built" : "NOT BUILT",
					  profile.World.FitToLevel ? "fitted, sampled from the level" : "generated",
					  profile.World.FitToLevel ? "the level's own ground stands; chunk registry only"
														  : (terrain_built ? "" : "the world's own ground is unaffected")));
	WWDEBUG_SAY (("ProceduralWorldManager: interior cliffs %s -- %d feature(s), checksum 0x%08X\r\n",
					  interior_cliffs_built ? "built" : (terrain_built ? "NOT BUILT" : "skipped (no terrain)"),
					  InteriorCliffGenerator::Get_Feature_Count (),
					  InteriorCliffGenerator::Get_Checksum ()));
	WWDEBUG_SAY (("ProceduralWorldManager: hydrology %s -- %d feature(s), checksum 0x%08X\r\n",
					  hydrology_built ? "built" : (terrain_built ? "NOT BUILT" : "skipped (no terrain)"),
					  HydrologyGenerator::Get_Feature_Count (),
					  HydrologyGenerator::Get_Checksum ()));
	WWDEBUG_SAY (("ProceduralWorldManager: strategic plan %s -- %d node(s), checksum 0x%08X\r\n",
					  strategic_plan_built ? "built" : (terrain_built ? "NOT BUILT" : "skipped (no terrain)"),
					  StrategicPlanner::Get_Node_Count (),
					  StrategicPlanner::Get_Checksum ()));
	WWDEBUG_SAY (("ProceduralWorldManager: road graph %s -- %d edge(s), checksum 0x%08X\r\n",
					  road_graph_built ? "built" : (strategic_plan_built ? "NOT BUILT" : "skipped (no strategic plan)"),
					  RoadGraphGenerator::Get_Edge_Count (),
					  RoadGraphGenerator::Get_Checksum ()));
	WWDEBUG_SAY (("ProceduralWorldManager: bridge plan %s -- %d bridge(s), checksum 0x%08X\r\n",
					  bridge_plan_built ? "built" : (road_graph_built ? "NOT BUILT" : "skipped (no road graph)"),
					  BridgePlanner::Get_Bridge_Count (),
					  BridgePlanner::Get_Checksum ()));
	WWDEBUG_SAY (("ProceduralWorldManager: tunnel plan %s -- %d tunnel(s) of %d candidate(s), %d cell(s) opened, checksum 0x%08X\r\n",
					  tunnel_plan_built ? "built" : (terrain_built ? "NOT BUILT" : "skipped (no terrain)"),
					  TunnelPlanner::Get_Tunnel_Count (),
					  TunnelPlanner::Get_Diagnostic_Count (),
					  TunnelSystem::Get_Opened_Cell_Count (),
					  TunnelPlanner::Get_Checksum ()));
	WWDEBUG_SAY (("ProceduralWorldManager: base selection %s -- %d pair(s) rejected, %d repair(s), checksum 0x%08X\r\n",
					  (base_selection_built && BaseSelectionPlanner::Has_Selection ()) ? "built" :
						 (bridge_plan_built ? "NOT BUILT" : "skipped (no bridge plan)"),
					  BaseSelectionPlanner::Get_Rejected_Pair_Count (),
					  BaseSelectionPlanner::Get_Repair_Log_Count (),
					  BaseSelectionPlanner::Get_Checksum ()));
	WWDEBUG_SAY (("ProceduralWorldManager: Tiberium fields %s -- %d field(s), %d crystal(s), checksum 0x%08X\r\n",
					  tiberium_fields_built ? "built" : (base_selection_built ? "NOT BUILT" : "skipped (no base selection)"),
					  TiberiumFieldGenerator::Get_Field_Count (),
					  TiberiumFieldGenerator::Get_Crystal_Count (),
					  TiberiumFieldGenerator::Get_Checksum ()));
	WWDEBUG_SAY (("ProceduralWorldManager: settlements %s -- %d settlement(s), %d structure(s), %d POI(s), checksum 0x%08X\r\n",
					  settlements_built ? "built" : ((tiberium_fields_built || base_selection_built) ? "NOT BUILT" : "skipped"),
					  SettlementGenerator::Get_Settlement_Count (),
					  SettlementGenerator::Get_Structure_Count (),
					  SettlementGenerator::Get_Poi_Count (),
					  SettlementGenerator::Get_Checksum ()));
	WWDEBUG_SAY (("ProceduralWorldManager: biome decoration %s -- %d forest chunk(s), %d tree(s), %d rock(s), checksum 0x%08X\r\n",
					  biome_decoration_built ? "built" : (terrain_built ? "NOT BUILT" : "skipped (no terrain)"),
					  BiomeDecorationGenerator::Get_Forest_Chunk_Count (),
					  BiomeDecorationGenerator::Get_Tree_Count (),
					  BiomeDecorationGenerator::Get_Rock_Count (),
					  BiomeDecorationGenerator::Get_Checksum ()));
	//	TransportationNetworkValidator::Validate already logged its own per-finding lines above
	//	this one; this is the single-line summary item 1's own per-pass status list asks for.
	WWDEBUG_SAY (("ProceduralWorldManager: transportation network validation %s\r\n",
					  network_valid ? "PASS" : "FAIL -- see TransportationNetworkValidator lines above"));
	WWDEBUG_SAY (("ProceduralWorldManager: combat space validation %s -- %d hard failure(s), sightline P50=%.1f P90=%.1f\r\n",
					  combat_space_valid ? "PASS" : "FAIL -- see CombatSpaceValidator lines above",
					  CombatSpaceValidator::Get_Failure_Count (),
					  CombatSpaceValidator::Get_Metrics ().SightlineP50,
					  CombatSpaceValidator::Get_Metrics ().SightlineP90));
	WWDEBUG_SAY (("ProceduralWorldManager: world integration %s -- %d structure(s) collidable, %d missing model\r\n",
					  world_integration_built ? "built" : (settlements_built ? "NOT BUILT" : "skipped (no settlements)"),
					  GeneratedWorldIntegrator::Get_Collision_Count (),
					  GeneratedWorldIntegrator::Get_Missing_Model_Count ()));
	WWDEBUG_SAY (("ProceduralWorldManager: navigation %s -- %d sector(s), %d portal(s) (%d bridge), ground-vehicle=%s, revision %u, checksum 0x%08X\r\n",
					  navigation_built ? "built" : (terrain_built ? "NOT BUILT" : "skipped (no terrain)"),
					  NavMeshGenerator::Get_Sector_Count (),
					  NavMeshGenerator::Get_Portal_Count (),
					  NavMeshGenerator::Get_Bridge_Portal_Count (),
					  NavMeshGenerator::Is_Ground_Vehicle_Navigable () ? "yes" : "infantry-only",
					  NavMeshGenerator::Get_Revision (),
					  NavMeshGenerator::Get_Checksum ()));
	WWDEBUG_SAY (("ProceduralWorldManager: minimap base layer %s -- %d x %d cell(s), %d marker(s), checksum 0x%08X\r\n",
					  minimap_built ? "built" : (terrain_built ? "NOT BUILT" : "skipped (no terrain)"),
					  MinimapBaseLayer::Get_Cell_Count_X (),
					  MinimapBaseLayer::Get_Cell_Count_Y (),
					  MinimapBaseLayer::Get_Marker_Count (),
					  MinimapBaseLayer::Get_Checksum ()));
	{
		float	shroud_cell_size	= 0.0f;
		int	shroud_cols			= 0;
		int	shroud_rows			= 0;
		Vector3 shroud_min (0.0f,0.0f,0.0f);
		WorldShroudManager::Get_Grid_Geometry (&shroud_cell_size,&shroud_cols,&shroud_rows,&shroud_min);
		WWDEBUG_SAY (("ProceduralWorldManager: shroud coverage %s -- %d x %d cell(s) of %.1f over the generated world\r\n",
						  shroud_coverage_built ? "built" : (terrain_built ? "NOT BUILT" : "skipped (no terrain)"),
						  shroud_cols,shroud_rows,shroud_cell_size));
	}
	WWDEBUG_SAY (("ProceduralWorldManager: expansion rules %s -- %d chokepoint(s), grading limits "
					  "cut %.1f / fill %.1f / slope %.2frad / area %.0f, checksum 0x%08X\r\n",
					  expansion_rules_armed ? "armed" : "NOT ARMED",
					  GeneratedWorldPlacementRules::Get_Chokepoint_Count (),
					  FoundationGradingService::Get_Limits ().MaxCutDepth,
					  FoundationGradingService::Get_Limits ().MaxFillHeight,
					  FoundationGradingService::Get_Limits ().MaxSlopeChangeRadians,
					  FoundationGradingService::Get_Limits ().MaxAffectedArea,
					  GeneratedWorldPlacementRules::Get_Chokepoint_Checksum ()));
	WWDEBUG_SAY (("ProceduralWorldManager: logistics graph %s -- %d node(s), %d route(s), checksum 0x%08X\r\n",
					  profile.Logistics.Enabled ? (logistics_built ? "built" : "NOT BUILT") : "disabled",
					  LogisticsGraphService::Get_Node_Count (),
					  LogisticsGraphService::Get_Route_Count (),
					  LogisticsGraphService::Get_Checksum ()));

	return true;
}


void ProceduralWorldManager::Destroy_World(void)
{
	//	Reverse build order.  BridgePlanner, RoadGraphGenerator and StrategicPlanner each only
	//	remove their own work (bridges, roads, or nothing shared at all -- StrategicPlanner is
	//	pure data); hydrology and interior cliffs each only ever remove their own IDs from their
	//	own shared collection (see each generator's own Clear).
	LogisticsGraphService::Clear ();
	BridgeDamageService::Reset ();			// span damage belongs to the bridges about to go
	ProceduralBaseLayoutService::Reset ();	// the level's own objects go with the level; this is bookkeeping
	MinimapBaseLayer::Clear ();
	NavMeshGenerator::Clear ();
	GeneratedWorldIntegrator::Clear ();
	BiomeDecorationGenerator::Clear ();
	SettlementGenerator::Clear ();
	ProceduralAssetRegistry::Shutdown ();
	TiberiumFieldGenerator::Clear ();
	BaseSelectionPlanner::Clear ();
	TunnelPlanner::Clear ();
	BridgePlanner::Clear ();
	RoadGraphGenerator::Clear ();
	StrategicPlanner::Clear ();
	HydrologyGenerator::Clear ();
	InteriorCliffGenerator::Clear ();
	MapPlanGenerator::Clear ();
	BoundaryCliffGenerator::Clear_Ring ();
	WorldBoundsService::Clear ();

	//	A `fit_to_level` world now has its own heightfield too -- bookkeeping only, sampled by
	//	Build_Fitted_Terrain, never collidable -- and it goes with everything else a procedural
	//	world owns, the same as a generated world's terrain.  Safe to call with no terrain at all
	//	(a world whose fitting itself failed), since Destroy_Terrain always is.
	WorldTerrainSystem::Destroy_Terrain ();
	MapPlanGenerator::Init ();
	MacroTerrainGenerator::Init ();

	//	Phase 2's and Phase 3's records belong to the attempt being torn down.  The immutable content
	//	metrics do not: Section 17's teardown keeps them, because nothing preflight reads a seed.
	HybridIdentityValid	= false;
	HybridIdentity			= HybridMapIdentity ();
	HybridIntents.Reset ();
	HybridGraphValid		= false;
	HybridGraph.Reset ();

	//	Phase 4's layers live in the attempt arena, so they die with it; the arena block itself is
	//	released here rather than being held across an idle client.
	HybridCoarseValid		= false;
	HybridCoarse.Reset ();
	HybridLandformsValid	= false;
	HybridLandforms.Reset ();
	HybridHydrologyValid	= false;
	HybridHydrology.Reset ();
	HybridMacroValid		= false;
	HybridMacro.Reset ();
	HybridAnchorsValid	= false;
	HybridAnchors.Reset ();
	HybridTopologyValid	= false;
	HybridTopologyRecord.Reset ();
	HybridRoutesValid		= false;
	HybridRoutes.Reset ();
	HybridArena.Release ();
	HybridWork.Reset ();

	ActiveProfile	= ProceduralWorldProfile ();
	Manifest			= WorldGenerationManifest ();
	WorldActive		= false;
}
