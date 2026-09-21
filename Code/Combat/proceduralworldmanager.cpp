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
#include "interiorcliffgenerator.h"
#include "macroterraingenerator.h"
#include "proceduralboundsbuilder.h"
#include "phys.h"
#include "pscene.h"
#include "worldboundsservice.h"
#include "worldseedservice.h"
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

	ActiveProfile = profile;

	/*
	**	The manifest records the profile as actually used, fit-to-level included, so the world
	**	can be reproduced from it.  Recording the profile as authored would make a manifest that
	**	describes a different world than the one it came from.
	*/
	DynamicVectorClass<StringClass> content_identifiers;
	Manifest = WorldGenerationManifestService::Create_Empty_World_Manifest (profile,content_identifiers);
	Manifest.CliffRingChecksum			= BoundaryCliffGenerator::Get_Ring_Checksum ();
	Manifest.TerrainChecksum			= terrain_built ? MacroTerrainGenerator::Get_Terrain_Checksum () : 0;
	Manifest.InteriorCliffChecksum	= interior_cliffs_built ? InteriorCliffGenerator::Get_Checksum () : 0;

	WorldActive = true;

	StringClass reproduction = WorldGenerationManifestService::Format_Reproduction_String (Manifest);
	WWDEBUG_SAY (("ProceduralWorldManager: world generated -- %s\r\n",reproduction.Peek_Buffer ()));
	WWDEBUG_SAY (("ProceduralWorldManager: cliff ring %s -- %d stations in %d chunks, checksum 0x%08X\r\n",
					  cliffs ? "built" : "NOT BUILT",
					  BoundaryCliffGenerator::Get_Station_Count (),
					  BoundaryCliffGenerator::Get_Chunk_Count (),
					  BoundaryCliffGenerator::Get_Ring_Checksum ()));
	WWDEBUG_SAY (("ProceduralWorldManager: macro terrain %s -- %s\r\n",
					  terrain_built ? "built" : "skipped (fit_to_level)",
					  terrain_built ? "" : "the level's own ground stands"));
	WWDEBUG_SAY (("ProceduralWorldManager: interior cliffs %s -- %d feature(s), checksum 0x%08X\r\n",
					  interior_cliffs_built ? "built" : (terrain_built ? "NOT BUILT" : "skipped (no terrain)"),
					  InteriorCliffGenerator::Get_Feature_Count (),
					  InteriorCliffGenerator::Get_Checksum ()));

	return true;
}


void ProceduralWorldManager::Destroy_World(void)
{
	//	Interior cliffs before the boundary ring -- both only ever remove their own escarpment
	//	IDs from the shared collection (see each generator's own Clear), so the order between
	//	them does not matter; it is written this way only because interior cliffs depend on the
	//	terrain the ring does not, so tearing down in roughly reverse build order reads clearly.
	InteriorCliffGenerator::Clear ();
	BoundaryCliffGenerator::Clear_Ring ();
	WorldBoundsService::Clear ();

	//	A `fit_to_level` world never had a heightfield in the first place (Build_Macro_Terrain
	//	skipped it), so this is a no-op then -- WorldTerrainSystem::Destroy_Terrain is always
	//	safe to call with no terrain.  A generated world's terrain goes with everything else a
	//	procedural world owns.
	WorldTerrainSystem::Destroy_Terrain ();
	MacroTerrainGenerator::Init ();

	ActiveProfile	= ProceduralWorldProfile ();
	Manifest			= WorldGenerationManifest ();
	WorldActive		= false;
}
