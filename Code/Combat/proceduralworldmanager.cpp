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

#include "baseselectionplanner.h"
#include "biomedecorationgenerator.h"
#include "boundarycliffgenerator.h"
#include "bridgeplanner.h"
#include "castres.h"
#include "combatspacevalidator.h"
#include "generatedworldintegrator.h"
#include "hydrologygenerator.h"
#include "interiorcliffgenerator.h"
#include "lineseg.h"
#include "logisticsgraphservice.h"
#include "macroterraingenerator.h"
#include "expansionregionregistry.h"
#include "foundationgrading.h"
#include "generatedworldplacementrules.h"
#include "minimapbaselayer.h"
#include "navmeshgenerator.h"
#include "physcoltest.h"
#include "playertype.h"
#include "roadgraphgenerator.h"
#include "proceduralassetregistry.h"
#include "settlementgenerator.h"
#include "strategicplanner.h"
#include "tiberiumfieldgenerator.h"
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

bool	ProceduralWorldManager::Requested		= false;
bool	ProceduralWorldManager::WorldActive	= false;

unsigned int	ProceduralWorldManager::RequestedSeed	= 0;
StringClass		ProceduralWorldManager::RequestedProfilePath;

ProceduralWorldProfile		ProceduralWorldManager::ActiveProfile;
WorldGenerationManifest		ProceduralWorldManager::Manifest;


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
**	Phase 5's own tuning, not exposed on the profile -- InteriorCliffDensity (the one field
**	Phase 1 already reserved for this phase) is the caller-facing knob; these are the fixed
**	shape decisions BoundaryCliffParams' own CLIFF_BATTER etc. are for the boundary ring.
*/
static const float INTERIOR_CLIFF_SLOPE_THRESHOLD_FRACTION	= 0.92f;
static const int	  INTERIOR_CLIFF_MAX_RUN_LENGTH				= 10;
static const float INTERIOR_CLIFF_BATTER							= 0.1f;
static const float INTERIOR_CLIFF_TOE_WIDTH						= 6.0f;

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
}


void ProceduralWorldManager::Clear_Request(void)
{
	Requested				= false;
	RequestedSeed			= 0;
	RequestedProfilePath	= "";
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
	params.MaxBuildableSlopeRadians	= (float)DEG_TO_RAD (profile.Terrain.MaxBuildableSlopeDegrees);
	params.FeatureScale					= profile.Terrain.FeatureScale;
	params.MacroRelief					= profile.Terrain.MacroRelief;
	params.HillDensity					= profile.Terrain.HillDensity;
	params.MountainDensity				= profile.Terrain.MountainDensity;
	params.ValleyDensity					= profile.Terrain.ValleyDensity;
	params.RidgeSharpness				= profile.Terrain.RidgeSharpness;
	params.PlateauChance					= profile.Terrain.PlateauChance;
	params.BoundaryApproachFlattening	= profile.Terrain.BoundaryApproachFlattening;

	if (!MacroTerrainGenerator::Generate (params,error_out)) {
		return false;
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
			PhysRayCollisionTestClass raytest (ray,&result,0);

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
	params.SlopeThresholdFraction	= INTERIOR_CLIFF_SLOPE_THRESHOLD_FRACTION;
	params.MaxBuildableSlopeRadians	= (float)DEG_TO_RAD (profile.Terrain.MaxBuildableSlopeDegrees);
	params.Density							= profile.Terrain.InteriorCliffDensity;
	params.MaxRunLength					= INTERIOR_CLIFF_MAX_RUN_LENGTH;
	params.Batter							= INTERIOR_CLIFF_BATTER;
	params.ToeWidth						= INTERIOR_CLIFF_TOE_WIDTH;
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

	return BridgePlanner::Generate (params,error_out);
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

	for (int attempt = 0; attempt < MAX_WORLD_GENERATION_ATTEMPTS; attempt++) {

		ProceduralWorldProfile profile;
		if (!ProceduralWorldProfileManager::Load_Profile_From_File (RequestedProfilePath.Peek_Buffer (),
																					  &profile,error_out)) {
			return false;			// the profile itself never loads -- no seed changes that
		}

		profile.Seed = WorldSeedService::Normalize_Seed (base_seed + (unsigned int)attempt);

		if (profile.World.FitToLevel) {
			Fit_Profile_To_Level (&profile);		// falls back to the profile's own dimensions
		}

		StringClass validate_error;
		if (!ProceduralWorldProfileManager::Validate_Profile (profile,&validate_error)) {
			if (error_out != nullptr) *error_out = validate_error;
			return false;			// a profile that fails validation fails it on every seed alike
		}

		StringClass attempt_error;
		bool required_connectivity_ok = false;
		if (!Generate_World_Attempt (profile,&attempt_error,&required_connectivity_ok)) {
			last_error = attempt_error;
			WWDEBUG_SAY (("ProceduralWorldManager: attempt %d (seed %u) failed outright (%s)%s\r\n",
							  attempt,profile.Seed,attempt_error.Peek_Buffer (),
							  (attempt + 1 < MAX_WORLD_GENERATION_ATTEMPTS) ? "; retrying" : "; no attempts left"));
			continue;
		}
		any_attempt_completed = true;

		if (required_connectivity_ok) {
			if (attempt > 0) {
				WWDEBUG_SAY (("ProceduralWorldManager: required connectivity achieved on retry attempt %d (seed %u)\r\n",
								  attempt,profile.Seed));
			}
			return true;
		}

		WWDEBUG_SAY (("ProceduralWorldManager: attempt %d (seed %u) left a required route disconnected%s\r\n",
						  attempt,profile.Seed,
						  (attempt + 1 < MAX_WORLD_GENERATION_ATTEMPTS) ? "; retrying with a new seed" : "; no attempts left"));
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
					  "the last attempt's world stands, not fully connected\r\n",MAX_WORLD_GENERATION_ATTEMPTS));
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

	StringClass cliff_error;
	bool cliffs = Build_Cliff_Ring (profile,&cliff_error);
	if (!cliffs) {
		WWDEBUG_SAY (("ProceduralWorldManager: cliff ring failed (%s); the boundary is still enforced\r\n",
						  cliff_error.Peek_Buffer ()));
	}

	//	Macro terrain, Phase 4.  Runs after the boundary and the cliff ring for the same reason
	//	they run in that order: the terrain generator reads WorldBoundsService to keep the cliff
	//	band flat, so the boundary has to exist first.  Skipped entirely -- not a failure -- for
	//	a `fit_to_level` world; see Build_Macro_Terrain's own comment.
	bool terrain_built = false;
	StringClass terrain_error;
	if (!Build_Macro_Terrain (profile,&terrain_built,&terrain_error)) {
		WWDEBUG_SAY (("ProceduralWorldManager: macro terrain failed (%s); the world's own ground is unaffected\r\n",
						  terrain_error.Peek_Buffer ()));
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
	if (required_connectivity_ok_out != nullptr) {
		*required_connectivity_ok_out = (!TransportationNetworkValidator::Did_Check_Required_Base_Routes () ||
													 TransportationNetworkValidator::Has_Required_Base_Routes ()) &&
													combat_space_valid;
	}

	ActiveProfile = profile;

	/*
	**	The manifest records the profile as actually used, fit-to-level included, so the world
	**	can be reproduced from it.  Recording the profile as authored would make a manifest that
	**	describes a different world than the one it came from.
	*/
	DynamicVectorClass<StringClass> content_identifiers;
	Manifest = WorldGenerationManifestService::Create_Empty_World_Manifest (profile,content_identifiers);
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
	MinimapBaseLayer::Clear ();
	NavMeshGenerator::Clear ();
	GeneratedWorldIntegrator::Clear ();
	BiomeDecorationGenerator::Clear ();
	SettlementGenerator::Clear ();
	ProceduralAssetRegistry::Shutdown ();
	TiberiumFieldGenerator::Clear ();
	BaseSelectionPlanner::Clear ();
	BridgePlanner::Clear ();
	RoadGraphGenerator::Clear ();
	StrategicPlanner::Clear ();
	HydrologyGenerator::Clear ();
	InteriorCliffGenerator::Clear ();
	BoundaryCliffGenerator::Clear_Ring ();
	WorldBoundsService::Clear ();

	//	A `fit_to_level` world now has its own heightfield too -- bookkeeping only, sampled by
	//	Build_Fitted_Terrain, never collidable -- and it goes with everything else a procedural
	//	world owns, the same as a generated world's terrain.  Safe to call with no terrain at all
	//	(a world whose fitting itself failed), since Destroy_Terrain always is.
	WorldTerrainSystem::Destroy_Terrain ();
	MacroTerrainGenerator::Init ();

	ActiveProfile	= ProceduralWorldProfile ();
	Manifest			= WorldGenerationManifest ();
	WorldActive		= false;
}
