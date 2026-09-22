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

#include "boundarycliffgenerator.h"
#include "bridgeplanner.h"
#include "castres.h"
#include "hydrologygenerator.h"
#include "interiorcliffgenerator.h"
#include "lineseg.h"
#include "macroterraingenerator.h"
#include "physcoltest.h"
#include "roadgraphgenerator.h"
#include "strategicplanner.h"
#include "transportationnetworkvalidator.h"
#include "proceduralboundsbuilder.h"
#include "phys.h"
#include "pscene.h"
#include "worldboundsservice.h"
#include "worldseedservice.h"
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

	ProceduralWorldProfile profile;
	if (!ProceduralWorldProfileManager::Load_Profile_From_File (RequestedProfilePath.Peek_Buffer (),
																				  &profile,error_out)) {
		return false;
	}

	//	The requested seed overrides whatever the profile file carries; the profile describes a
	//	kind of world, the request picks which one.
	profile.Seed = WorldSeedService::Normalize_Seed (RequestedSeed);

	if (profile.World.FitToLevel) {
		Fit_Profile_To_Level (&profile);		// falls back to the profile's own dimensions
	}

	if (!ProceduralWorldProfileManager::Validate_Profile (profile,error_out)) {
		return false;
	}

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

	//	Final network validation, item 6.  Runs regardless of which of the steps above actually
	//	built anything -- with nothing built it trivially passes (0 roads, 0 bridges, nothing to
	//	be broken), which is the correct answer for a hand-authored map with no procedural roads
	//	at all, not a special case to skip around.
	bool network_valid = Validate_Transportation_Network (profile);

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
	//	TransportationNetworkValidator::Validate already logged its own per-finding lines above
	//	this one; this is the single-line summary item 1's own per-pass status list asks for.
	WWDEBUG_SAY (("ProceduralWorldManager: transportation network validation %s\r\n",
					  network_valid ? "PASS" : "FAIL -- see TransportationNetworkValidator lines above"));

	return true;
}


void ProceduralWorldManager::Destroy_World(void)
{
	//	Reverse build order.  BridgePlanner, RoadGraphGenerator and StrategicPlanner each only
	//	remove their own work (bridges, roads, or nothing shared at all -- StrategicPlanner is
	//	pure data); hydrology and interior cliffs each only ever remove their own IDs from their
	//	own shared collection (see each generator's own Clear).
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
