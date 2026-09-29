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
 *                                                                                             *
 *                 Project Name : OpenW3D                                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/WWPhys/macroterraingenerator.cpp             $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "macroterraingenerator.h"
#include "hybridmacroterrain.h"

#include "mapplangenerator.h"

#include "heightfield.h"
#include "phys.h"
#include "pscene.h"
#include "terrainmask.h"
#include "terraintexturesystem.h"
#include "vector2.h"
#include "vector3.h"
#include "worldboundsservice.h"
#include "worldterrainsystem.h"
#include "wwdebug.h"
#include "wwmath.h"
#include "wwstring.h"


bool				MacroTerrainGenerator::Generated			= false;
unsigned int	MacroTerrainGenerator::TerrainChecksum	= 0;
int				MacroTerrainGenerator::LastVertexCountX	= 0;
int				MacroTerrainGenerator::LastVertexCountY	= 0;


MacroTerrainParams::MacroTerrainParams(void) :
	Seed (1u),
	WorldWidth (4096.0f), WorldHeight (4096.0f), ChunkSize (128.0f),
	CenterX (0.0f), CenterY (0.0f), SeaLevel (0.0f),
	MinElevation (-30.0f), MaxElevation (160.0f), LandElevationBias (0.0f),
	MaxBuildableSlopeRadians (0.4886f),		// ~28 degrees
	FeatureScale (900.0f),
	MacroRelief (0.6f), HillDensity (0.45f), MountainDensity (0.18f), ValleyDensity (0.30f),
	RidgeSharpness (0.55f), PlateauChance (0.15f), BoundaryApproachFlattening (0.6f),
	UsePlan (false), PlanDetailWeight (0.35f),
	UseHybrid (false), HybridParams (nullptr), HybridMatrices (nullptr), HybridLandforms (nullptr),
	HybridHydrology (nullptr), HybridSurfaces (nullptr)
{
}


//	Bounded, same spirit as Section 3C.1's retry ceilings -- generation-tuning rather than
//	world description, so it lives beside the generator that consumes it rather than widening
//	the schema with a number no profile author has an opinion about (the same reasoning
//	ProceduralWorldManager's own CLIFF_STATION_SPACING/CLIFF_STATIONS_PER_CHUNK already use).
static const int SLOPE_RELAXATION_PASSES = 16;


void MacroTerrainGenerator::Init(void)
{
	Generated			= false;
	TerrainChecksum	= 0;
	LastVertexCountX	= 0;
	LastVertexCountY	= 0;
}


void MacroTerrainGenerator::Shutdown(void)
{
	Init ();
}


static void
Clear(void)
{
	MacroTerrainGenerator::Init ();
}


/***********************************************************************************************
**	Noise.  Single-octave value noise is TerrainMaskClass::Hash_Value plus a smoothstep-
**	interpolated lattice -- the exact idiom terraintexturesystem.cpp's own rule evaluation
**	already uses, reused here rather than duplicated as a new hash: only the fractal-summing
**	loop around it is new.
***********************************************************************************************/
static float
Value_Noise(float wx,float wy,float frequency,int seed)
{
	float fx = wx * frequency;
	float fy = wy * frequency;

	int lx = WWMath::Float_To_Int_Floor (fx);
	int ly = WWMath::Float_To_Int_Floor (fy);
	float u = fx - (float)lx;
	float v = fy - (float)ly;
	u = u * u * (3.0f - 2.0f * u);
	v = v * v * (3.0f - 2.0f * v);

	float h00 = TerrainMaskClass::Hash_Value (lx,  ly,  seed);
	float h10 = TerrainMaskClass::Hash_Value (lx+1,ly,  seed);
	float h01 = TerrainMaskClass::Hash_Value (lx,  ly+1,seed);
	float h11 = TerrainMaskClass::Hash_Value (lx+1,ly+1,seed);

	float lower = h00 + (h10 - h00) * u;
	float upper = h01 + (h11 - h01) * u;
	return lower + (upper - lower) * v;		// 0..1
}


//	Multi-octave sum, normalized so the result stays in [-1,1] regardless of octave count.
//	`seed` is offset per octave by a fixed, documented constant rather than reseeding a stateful
//	generator -- there is no state here to carry between octaves, only more lattice samples.
static float
Fractal_Noise(float wx,float wy,float base_frequency,int octaves,int seed)
{
	float freq = base_frequency;
	float amp  = 1.0f;
	float sum  = 0.0f;
	float norm = 0.0f;

	for (int o = 0; o < octaves; o++) {
		sum  += amp * ((Value_Noise (wx,wy,freq,seed + (o * 101)) * 2.0f) - 1.0f);
		norm += amp;
		freq *= 2.0f;
		amp  *= 0.5f;
	}

	return (norm > 0.0f) ? (sum / norm) : 0.0f;
}


/*
**	One elevation sample, in world units relative to sea level.  Four independent noise
**	channels -- macro masses, ridges, fine hills, broad valleys -- each its own fractal sum at
**	its own frequency and its own seed offset, so a change to one profile weight cannot make
**	another channel's pattern shift with it.  `PlateauChance` optionally terraces the result
**	into bands, which is Phase 4 step 5's "plateaus/mesas where profile permits" -- a heightfield
**	approximation, not Phase 5's vertical-faced mesa (Section 8.1's own job, once interior cliffs
**	exist to build one).
*/
static float
Sample_Elevation(const MacroTerrainParams & p,float wx,float wy)
{
	float base_freq = 1.0f / WWMath::Max (p.FeatureScale,1.0f);
	int seed = (int)p.Seed;

	float macro      = Fractal_Noise (wx,wy,base_freq,4,seed);
	float ridge_raw  = Fractal_Noise (wx,wy,base_freq * 1.7f,4,seed + 1013);
	float ridge      = 1.0f - WWMath::Fabs (ridge_raw);			// 0..1, sharp along the zero crossing
	ridge = ridge * WWMath::Lerp (ridge,ridge * ridge,p.RidgeSharpness);
	float hill       = Fractal_Noise (wx,wy,base_freq * 3.1f,3,seed + 2027);
	float valley_raw = Fractal_Noise (wx,wy,base_freq * 0.6f,3,seed + 3041);
	//	The zero contour of broad valley noise forms a continuous, winding
	//	ravine. The old absolute-value term only made broad shallow bowls and
	//	left no recognisable channel for rivers or canyon walls to follow.
	float ravine = WWMath::Clamp (1.0f - WWMath::Fabs (valley_raw) / 0.16f,0.0f,1.0f);
	ravine *= ravine;

	float combined = (macro * p.MacroRelief)
						+ (((ridge * 2.0f) - 1.0f) * p.MountainDensity)
						+ (hill * p.HillDensity * 0.5f)
						- ((WWMath::Fabs (valley_raw) * 0.45f + ravine * 0.55f) * p.ValleyDensity);
	combined = WWMath::Clamp (combined,-1.0f,1.0f);

	if (p.PlateauChance > 0.0f) {
		float mask = Value_Noise (wx,wy,base_freq * 0.5f,seed + 4051);		// 0..1
		if (mask < p.PlateauChance) {
			const float TERRACE_BANDS = 5.0f;
			float normalized = (combined + 1.0f) * 0.5f;
			float terraced = (WWMath::Floor (normalized * TERRACE_BANDS) / TERRACE_BANDS * 2.0f) - 1.0f;

			//	Blended rather than snapped outright, so a mesa reads as a run of flats meeting a
			//	slope rather than as a single discontinuity the relaxation pass would immediately
			//	have to fight.
			float blend = WWMath::Clamp (1.0f - (mask / WWMath::Max (p.PlateauChance,0.001f)),0.0f,1.0f);
			combined = WWMath::Lerp (combined,terraced,blend);
		}
	}

	float relief = (combined >= 0.0f) ? (combined * p.MaxElevation) : (combined * -p.MinElevation);
	return relief + p.LandElevationBias;
}


/*
**	Bounded local relaxation, roadmap step 9.  Deterministic and order-fixed: every pass walks
**	the grid row-major and only ever compares a vertex to its right and down neighbours, so the
**	same heights always produce the same corrections regardless of what called this.  `locked`
**	vertices (cliff band and beyond) are never read as a source or written as a destination --
**	the immutable face stays immutable by never being a term in this loop's arithmetic, not by a
**	check that could be defeated by a future edit to it.
**
**	Not a hard guarantee for every profile: a small, bounded number of passes reduces slope
**	toward the limit rather than proving it everywhere for an arbitrarily aggressive profile.
**	Check_Ordinary_Slope_Limit reports what is actually left rather than asserting there is
**	nothing, which is the honest version of this contract.
*/
static void
Relax_Slopes(float * heights,const bool * locked,int vx,int vy,float cell_size,float max_slope_radians)
{
	float clamped_slope = WWMath::Clamp (max_slope_radians,0.0f,1.5f);		// tan() diverges past ~85.9 degrees
	// A terrain triangle can climb in both grid axes at once.  Limiting each
	// edge to tan(limit) independently allowed the combined gradient to reach
	// sqrt(2) times the buildable slope, leaving whole seed variants unusable.
	float max_delta = cell_size * WWMath::Tan (clamped_slope) * 0.70710678f;

	for (int pass = 0; pass < SLOPE_RELAXATION_PASSES; pass++) {
		for (int iy = 0; iy < vy; iy++) {
			for (int ix = 0; ix < vx; ix++) {

				int idx = (iy * vx) + ix;
				if (locked[idx]) {
					continue;
				}

				if ((ix + 1) < vx) {
					int ridx = idx + 1;
					if (!locked[ridx]) {
						float d = heights[ridx] - heights[idx];
						if (WWMath::Fabs (d) > max_delta) {
							float excess = (WWMath::Fabs (d) - max_delta) * 0.5f;
							float sign = (d > 0.0f) ? 1.0f : -1.0f;
							heights[ridx] -= sign * excess;
							heights[idx]  += sign * excess;
						}
					}
				}

				if ((iy + 1) < vy) {
					int didx = idx + vx;
					if (!locked[didx]) {
						float d = heights[didx] - heights[idx];
						if (WWMath::Fabs (d) > max_delta) {
							float excess = (WWMath::Fabs (d) - max_delta) * 0.5f;
							float sign = (d > 0.0f) ? 1.0f : -1.0f;
							heights[didx] -= sign * excess;
							heights[idx]  += sign * excess;
						}
					}
				}
			}
		}
	}
}


bool MacroTerrainGenerator::Generate(const MacroTerrainParams & params,StringClass * error_out)
{
	#define FAIL(msg) { if (error_out) *error_out = msg; Clear(); WorldTerrainSystem::Destroy_Terrain(); return false; }

	Clear ();

	if (!WWMath::Is_Valid_Float (params.WorldWidth) || (params.WorldWidth <= 0.0f))
																				FAIL ("world width must be positive");
	if (!WWMath::Is_Valid_Float (params.WorldHeight) || (params.WorldHeight <= 0.0f))
																				FAIL ("world height must be positive");
	if (!WWMath::Is_Valid_Float (params.ChunkSize) || (params.ChunkSize <= 0.0f))
																				FAIL ("chunk size must be positive");
	if (params.MaxElevation < params.MinElevation)					FAIL ("max elevation below min elevation");
	if (!WWMath::Is_Valid_Float (params.LandElevationBias))	FAIL ("land elevation bias is not finite");
	if (!WWMath::Is_Valid_Float (params.FeatureScale) || (params.FeatureScale <= 0.0f))
																				FAIL ("feature scale must be positive");
	if ((params.MaxBuildableSlopeRadians <= 0.0f) || (params.MaxBuildableSlopeRadians >= 1.5f))
																				FAIL ("max buildable slope must be between 0 and ~86 degrees");
	if (params.UsePlan && !MapPlanGenerator::Has_Plan ())		FAIL ("use_plan set but MapPlanGenerator holds no plan");
	if (params.UseHybrid && params.UsePlan)						FAIL ("hybrid Phase 7 and the D-003 plan path are exclusive");
	if (params.UseHybrid && ((params.HybridParams == nullptr) || (params.HybridMatrices == nullptr)
		|| (params.HybridLandforms == nullptr) || (params.HybridHydrology == nullptr)
		|| (params.HybridSurfaces == nullptr)))					FAIL ("use_hybrid set without the Phase 4-6 records");
	if (params.UsePlan && ((params.PlanDetailWeight < 0.0f) || (params.PlanDetailWeight > 1.0f)))
																				FAIL ("plan detail weight must be 0..1");

	float cell_size = params.ChunkSize / (float)HeightfieldClass::PATCH_CELLS;

	int vx = (int)WWMath::Ceil (params.WorldWidth / cell_size) + 1;
	int vy = (int)WWMath::Ceil (params.WorldHeight / cell_size) + 1;
	if (vx < 2) vx = 2;
	if (vy < 2) vy = 2;

	Vector3 origin (params.CenterX - ((float)(vx - 1) * cell_size * 0.5f),
						 params.CenterY - ((float)(vy - 1) * cell_size * 0.5f),
						 params.SeaLevel);

	if (!WorldTerrainSystem::Create_Terrain (vx,vy,cell_size,origin)) {
		FAIL ("WorldTerrainSystem refused to create terrain");
	}

	bool configured = WorldBoundsService::Is_Configured ();
	float approach_falloff = WWMath::Max (params.FeatureScale,1.0f);

	float * heights = new float[vx * vy];
	bool  * locked  = new bool[vx * vy];

	for (int iy = 0; iy < vy; iy++) {
		for (int ix = 0; ix < vx; ix++) {

			int idx = (iy * vx) + ix;
			float wx = origin.X + ((float)ix * cell_size);
			float wy = origin.Y + ((float)iy * cell_size);

			WorldBoundsZoneType zone = configured
				? WorldBoundsService::Classify_Point (Vector2 (wx,wy))
				: WORLD_ZONE_PLAYABLE_INTERIOR;

			float h;
			if (zone >= WORLD_ZONE_CLIFF_BAND) {

				//	The immutable face's own foot.  Flat, at sea level, and never touched again --
				//	this vertex is locked below, so neither generation nor relaxation can move it.
				h = 0.0f;
				locked[idx] = true;

			} else if (params.UseHybrid) {

				/*
				**	Runtime Phase 7 (item I8).  The whole height expression -- semantic tier, macro bias,
				**	the Phase-5 landform sum, the Phase-6 water carve and the protected detail noise -- is
				**	`Hybrid_Macro_Sample_Height`, which returns an absolute height while this grid stores
				**	relief relative to sea level.  Nothing else in this loop applies to a hybrid world:
				**	the boundary-approach flattening is the noise path's own seam rule, and Phase 4 has
				**	already put the hybrid boundary band's tier into the matrix.
				*/
				h = Hybrid_Macro_Sample_Height (*params.HybridParams,*params.HybridMatrices,
														  *params.HybridLandforms,*params.HybridHydrology,
														  *params.HybridSurfaces,Vector2 (wx,wy)) - params.SeaLevel;

				/*
				**	The ordinary-slope enforcement below runs only outside the explicit steep and water
				**	profiles the plan names, so a vertical-faced cliff, a canyon wall and a river bed keep
				**	the shape Phases 5 and 6 fixed.  A protected vertex is still written; it is only never
				**	a term in the relaxation loop's arithmetic.
				*/
				if (Hybrid_Macro_Is_Protected_Profile (*params.HybridParams,*params.HybridLandforms,
																	*params.HybridHydrology,Vector2 (wx,wy))) {
					locked[idx] = true;
					params.HybridSurfaces->ProtectedVertices ++;
				} else {
					locked[idx] = false;
				}

			} else {

				h = Sample_Elevation (params,wx,wy);

				if (params.UsePlan) {
					//	D-003 Phase 3B.  The plan's tier height is the structure; the noise relief
					//	is kept only as in-tier detail.  A RAMP cell keeps its blended tier slope and
					//	drops the detail so a lane crossing a tier stays under the buildable cap; a
					//	CLIFF_SEAM cell keeps its sharp step so the interior cliff generator sees a
					//	chunk still steep after relaxation, which is exactly the candidate it wants.
					float tier_h = MapPlanGenerator::Sample_Tier_Height (wx,wy);
					MapPlanCellKind kind = MapPlanGenerator::Get_Kind_At (wx,wy);
					float detail = (kind == MAP_PLAN_CELL_RAMP) ? 0.0f : params.PlanDetailWeight;
					if ((kind == MAP_PLAN_CELL_BASE) || (kind == MAP_PLAN_CELL_FIELD)) detail *= 0.25f;
					h = tier_h + (h - params.LandElevationBias) * detail + params.LandElevationBias;
				}

				if ((zone == WORLD_ZONE_BOUNDARY_APPROACH) && configured) {
					//	Pulled toward the flat band the closer the approach zone gets to it, so
					//	the two meet without a seam a slope check would flag -- step 6's own
					//	requirement, kept without ever writing to a cliff-band vertex to do it.
					float inward = WorldBoundsService::Get_Inward_Distance (Vector2 (wx,wy));
					float t = WWMath::Clamp (inward / approach_falloff,0.0f,1.0f);
					float flattened = WWMath::Lerp (0.0f,h,t);
					h = WWMath::Lerp (h,flattened,params.BoundaryApproachFlattening);
				}

				locked[idx] = false;
			}

			h = WWMath::Clamp (h,params.MinElevation,params.MaxElevation);
			heights[idx] = h;
		}
	}

	Relax_Slopes (heights,locked,vx,vy,cell_size,params.MaxBuildableSlopeRadians);

	//	Relaxation can only ever move an unlocked vertex, but the clamp is re-applied afterward
	//	in case a correction pushed one back past the elevation ceiling/floor at the very edge of
	//	the legal range -- cheap insurance against a fault a profile at the edge of its own limits
	//	could otherwise produce.
	for (int idx = 0; idx < (vx * vy); idx++) {
		if (!locked[idx]) {
			heights[idx] = WWMath::Clamp (heights[idx],params.MinElevation,params.MaxElevation);
		}
	}

	if (params.UseHybrid) {
		//	Phase 7's own record of what was realized, for its gate, its checksum and the manifest.
		float lowest = heights[0];
		float highest = heights[0];
		for (int idx = 0; idx < (vx * vy); idx++) {
			if (heights[idx] < lowest) lowest = heights[idx];
			if (heights[idx] > highest) highest = heights[idx];
		}
		params.HybridSurfaces->VertexCountX	= vx;
		params.HybridSurfaces->VertexCountY	= vy;
		params.HybridSurfaces->CellSize		= cell_size;
		params.HybridSurfaces->MinHeight		= lowest + params.SeaLevel;
		params.HybridSurfaces->MaxHeight		= highest + params.SeaLevel;
	}

	bool set_ok = WorldTerrainSystem::Set_Heights (heights,vx * vy);
	delete [] heights;
	delete [] locked;

	if (!set_ok) {
		FAIL ("WorldTerrainSystem refused the generated heights");
	}

	//	Device-less from here down: the chunk registry needs no scene.
	WorldTerrainSystem::Build_Chunk_Registry (params.MaxBuildableSlopeRadians);
	if (!WorldTerrainSystem::Has_Chunk_Registry ()) {
		FAIL ("generated terrain has no chunk registry");
	}

	//	A native world has no level or editor command to define terrain layers.  Patch
	//	rendering draws one pass per registered layer, so an empty registry leaves
	//	the generated ground invisible even though its collision mesh exists.
	if (TerrainTextureSystem::Get_Layer_Count() == 0) {
		TerrainTextureSystem::Define_Default_Layers();
	}
	if (!TerrainTextureSystem::Create_Masks ()) {
		FAIL ("generated terrain texture masks could not be created");
	}

	//	Both calls are safe without a physics scene, so device-less generators still
	//	get heightfield data. Once a live scene exists, both representations are
	//	required before the world can be accepted.
	const bool collision_built = WorldTerrainSystem::Build_Collision ();
	const bool far_built = WorldTerrainSystem::Build_Far_Terrain_Representation ();
	//	Device-less self-checks may generate only data.  In a live scene, accepting a
	//	heightfield without its collision or rendered far layer produces a broken map.
	if (PhysicsSceneClass::Get_Instance () != nullptr) {
		if (!collision_built) FAIL ("generated terrain collision could not be built");
		if (!far_built) FAIL ("generated far terrain could not be built");
	}

	Generated			= true;
	TerrainChecksum	= WorldTerrainSystem::Get_World_Chunk_Checksum ();
	LastVertexCountX	= vx;
	LastVertexCountY	= vy;

	WWDEBUG_SAY (("MacroTerrainGenerator: terrain generated -- %d x %d vertices, %d x %d chunks, checksum 0x%08X\r\n",
					  vx,vy,WorldTerrainSystem::Get_Chunk_Count_X (),WorldTerrainSystem::Get_Chunk_Count_Y (),TerrainChecksum));

	return true;

	#undef FAIL
}


int MacroTerrainGenerator::Check_Height_Range(const MacroTerrainParams & params,StringClass * report)
{
	int failures = 0;
	int count_x = WorldTerrainSystem::Get_Chunk_Count_X ();
	int count_y = WorldTerrainSystem::Get_Chunk_Count_Y ();

	for (int py = 0; py < count_y; py++) {
		for (int px = 0; px < count_x; px++) {
			TerrainChunkInfoClass info;
			if (!WorldTerrainSystem::Get_Chunk_Info (px,py,&info)) {
				continue;
			}
			if ((info.MinHeight < params.MinElevation - 0.01f) || (info.MaxHeight > params.MaxElevation + 0.01f)) {
				failures++;
				if ((report != nullptr) && (failures == 1)) {
					report->Format ("chunk (%d,%d) height [%.2f,%.2f] outside profile range [%.2f,%.2f]",
										  px,py,info.MinHeight,info.MaxHeight,params.MinElevation,params.MaxElevation);
				}
			}
		}
	}
	return failures;
}


int MacroTerrainGenerator::Check_Ordinary_Slope_Limit(const MacroTerrainParams & params,StringClass * report)
{
	int failures = 0;
	int count_x = WorldTerrainSystem::Get_Chunk_Count_X ();
	int count_y = WorldTerrainSystem::Get_Chunk_Count_Y ();

	for (int py = 0; py < count_y; py++) {
		for (int px = 0; px < count_x; px++) {
			TerrainChunkInfoClass info;
			if (!WorldTerrainSystem::Get_Chunk_Info (px,py,&info)) {
				continue;
			}
			if (info.CliffBand) {
				continue;		// the immutable face is exempt by definition, not a violation
			}
			if (info.MaxSlopeRadians > params.MaxBuildableSlopeRadians + 0.01f) {
				failures++;
				if ((report != nullptr) && (failures == 1)) {
					report->Format ("chunk (%d,%d) slope %.3f rad exceeds ordinary limit %.3f rad",
										  px,py,info.MaxSlopeRadians,params.MaxBuildableSlopeRadians);
				}
			}
		}
	}
	return failures;
}


/*
**	Per vertex, because that is the unit the invariant is kept in: generation locks and writes
**	flat exactly the vertices WorldBoundsService classifies at or past the cliff band.  A chunk's
**	own CliffBand flag is deliberately coarser -- set when any point of the chunk touches the band,
**	so nothing builds on or routes through a chunk half in it -- which means a straddling chunk
**	legitimately holds ordinary, unflattened vertices too.  Judging flatness per chunk asserted
**	an invariant no generator promises.
*/
int MacroTerrainGenerator::Check_Cliff_Band_Untouched(StringClass * report)
{
	int failures = 0;
	HeightfieldClass * field = WorldTerrainSystem::Peek_Heightfield ();
	if ((field == nullptr) || !WorldBoundsService::Is_Configured ()) {
		return 0;
	}

	const Vector3 & origin = field->Get_Origin ();
	float cell = field->Get_Cell_Size ();
	for (int iy = 0; iy < field->Get_Vertex_Count_Y (); iy++) {
		for (int ix = 0; ix < field->Get_Vertex_Count_X (); ix++) {
			Vector2 p (origin.X + ((float)ix * cell),origin.Y + ((float)iy * cell));
			if (WorldBoundsService::Classify_Point (p) < WORLD_ZONE_CLIFF_BAND) {
				continue;
			}
			float h = origin.Z + field->Get_Height (ix,iy);
			if (WWMath::Fabs (h) > 0.01f) {
				failures++;
				if ((report != nullptr) && (failures == 1)) {
					report->Format ("cliff-band vertex (%d,%d) at (%.1f,%.1f) height %.2f is not flat at sea level",
										  ix,iy,p.X,p.Y,h);
				}
			}
		}
	}
	return failures;
}
