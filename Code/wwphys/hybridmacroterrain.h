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
 *                     $Archive:: /Commando/Code/wwphys/hybridmacroterrain.h                  $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Hybrid world generator rewrite, item I8 -- runtime Phase 7, macro terrain realization        *
 * (Procedural_World_Generator_Rewrite_Plan_FINAL_AUDITED.md Section 8 Phase 7, Section 10.1).  *
 *                                                                                             *
 * This is the *formula* owner, not a second heightfield.  `WorldTerrainSystem` stays the one    *
 * heightfield and `MacroTerrainGenerator` stays the one thing that writes it: that generator    *
 * asks `Hybrid_Macro_Sample_Height` for each vertex when a request carries hybrid records, so   *
 * the plan's height expression, its detail-protection rules and its river longitudinal walk     *
 * have exactly one implementation and the native noise-geography channels are replaced rather   *
 * than joined by a second terrain path.  A plan-disabled (authored/legacy) request never        *
 * reaches anything here.                                                                       *
 *                                                                                             *
 * The one piece of state Phase 7 owns is the river surface: step 5's six-step walk fixes each   *
 * accepted river's final water surface and bed from the Phase-6 records, and everything that     *
 * follows -- the carve, the gate, and later the bridge deck clearances -- reads those frozen     *
 * samples instead of recomputing a surface of its own.                                          *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef HYBRIDMACROTERRAIN_H
#define HYBRIDMACROTERRAIN_H

#include "always.h"
#include "vector.h"
#include "vector2.h"
#include "wwstring.h"
#include "hybridplanning.h"
#include "hybridcoarsematrix.h"
#include "hybridlandforms.h"
#include "hybridhydrology.h"


/*
**	Section 8 Phase 7's exact detail-noise primitive.  Hash-grid value noise: the lattice corner
**	value is `2*Hash01(terrain,"detail_noise",octave,ix,iy)-1`, the per-axis weight is the plan's
**	own smoothstep, and the result is the bilinear blend of the four corners.  One primitive, so no
**	phase ever chooses among the tree's unrelated legacy noise functions.
*/
float	Hybrid_Value_Noise_2D(unsigned int terrain_seed,const Vector2 & point,float length,int octave);


/*
**	Phase 7's own record for one accepted river: the final water surface and bed height at every
**	Phase-6 centerline sample, after step 5's six-step longitudinal walk.
**
**	`Surface` is non-increasing from source to sink by construction and equals the sink's water level
**	at a bound mouth; `Bed` is `Surface - WaterDefinitionClass::BedDepth`, clamped only by the global
**	terrain minimum, and a bed that clamping would push above its own surface fails the phase.
*/
struct HybridRiverSurface
{
	HybridRiverSurface(void) : RiverId(0),SinkBodyId(0),MouthSurface(0.0f) { }

	//	DynamicVectorClass<T> (wwlib/vector.h) requires T to support == and !=.  RiverId is the key.
	bool	operator == (const HybridRiverSurface & that) const	{ return RiverId == that.RiverId; }
	bool	operator != (const HybridRiverSurface & that) const	{ return !(*this == that); }

	unsigned int	RiverId;					// HybridRiverSystem::StableId
	unsigned int	SinkBodyId;				// the lake/coast the mouth is bound to, 0 for an exterior sink
	float				MouthSurface;

	DynamicVectorClass<float>	Surface;
	DynamicVectorClass<float>	Bed;
};


/*
**	Runtime Phase 7's output.  The heightfield itself belongs to `WorldTerrainSystem`; this record is
**	the frozen river surfaces plus what the gate and the manifest need to describe the realization.
*/
struct HybridMacroTerrainSet
{
	HybridMacroTerrainSet(void)	{ Reset (); }

	void	Reset(void);

	const HybridRiverSurface *	Find_River(unsigned int river_id) const;

	//	The final water surface/bed at one river sample, or false when that river is not realized.
	bool	River_Sample(unsigned int river_id,int index,float * surface_out,float * bed_out) const;

	DynamicVectorClass<HybridRiverSurface>	Rivers;

	int			VertexCountX;				// as the heightfield was written
	int			VertexCountY;
	float			CellSize;
	float			MinHeight;					// realized extremes, absolute world units
	float			MaxHeight;
	int			ProtectedVertices;		// vertices excluded from ordinary-slope relaxation
	int			SlopeViolations;			// what the enforcement pass could not reduce, outside profiles
	int			WaterBedSupportVertices;	// D-037: vertices lowered as bounded shoreline raster support
	int			WaterBedSupportUnresolved;	// wet samples the bounded support could not bring under water
	unsigned int	Checksum;				// Phase 7 gate value
};


/*
**	Phase 7's inputs beyond the frozen Phase 4-6 records.  Plain values only, so the evaluator is
**	reproducible from (params, matrices, landforms, hydrology, surfaces).
*/
struct HybridMacroTerrainParams
{
	HybridMacroTerrainParams(void);

	unsigned int		TerrainSeed;			// SEED_DOMAIN_TERRAIN, already derived
	HybridConstants	Constants;

	float					SeaLevel;
	float					MinElevation;			// relative to SeaLevel, exactly as the profile stores it
	float					MaxElevation;
	float					TierStep;
	int					TierCount;

	float					DetailWeight;			// plan.detail_weight
	float					FeatureScale;			// terrain.feature_scale
	float					RiverWidth;				// water.river_width -- the bank blend term
	float					MaxBuildableSlopeRadians;
};


/*
**	Phase 7 failures.  Each is a named attempt failure: the manager retries with a new seed.
*/
enum HybridMacroTerrainFailure
{
	HYBRID_MACRO_OK = 0,
	HYBRID_MACRO_INVALID_INPUT,				// degenerate constants, or no matrix
	HYBRID_MACRO_BED_ABOVE_SURFACE,			// step 5: the elevation floor lifts a bed above its own water
	HYBRID_MACRO_RIVER_NOT_MONOTONIC,		// gate: a river surface rises downstream
	HYBRID_MACRO_ELEVATION_LIMIT,				// gate: a realized height is outside the elevation envelope
	HYBRID_MACRO_FEATURE_LOST,					// gate: a Phase-5/6 feature is no longer represented
	HYBRID_MACRO_FAILURE_COUNT
};

const char *	Hybrid_Macro_Terrain_Failure_Name(HybridMacroTerrainFailure failure);

struct HybridMacroTerrainResult
{
	HybridMacroTerrainResult(void) : Failure(HYBRID_MACRO_OK) { }

	HybridMacroTerrainFailure	Failure;
	StringClass						Detail;
};


/*
**	Step 5's six-step river longitudinal walk, which must run before any height is evaluated: the
**	provisional surface per sample, the mouth pinned to its sink level, the downstream `min` walk,
**	the upstream `max` walk back from the fixed mouth, and the bed target with its single global
**	clamp.  Fills `out->Rivers`; returns false with `result->Failure` set when a clamp would put a
**	bed above its own water surface.
*/
bool	Hybrid_Resolve_River_Surfaces(const HybridMacroTerrainParams & params,const HybridCoarseMatrices & matrices,
													const HybridLandformSet & landforms,const HybridHydrologySet & hydrology,
													HybridMacroTerrainSet * out,HybridMacroTerrainResult * result);

/*
**	Section 8 Phase 7's `detail_protection_factor(p)`, exactly as the plan lists it, taking the
**	minimum where rules overlap: 0 in a base-core reserve, 0.25 in a provisional Tiberium reserve,
**	0 inside lake/sea water, 0 on a river bed out to half the local width, a smoothstep across each
**	river bank blend `max(river_width,2W)`, a smoothstep across `2W` outward from a lake/sea
**	shoreline, and 1 elsewhere.
*/
float	Hybrid_Detail_Protection_Factor(const HybridMacroTerrainParams & params,const HybridCoarseMatrices & matrices,
													 const HybridHydrologySet & hydrology,const Vector2 & world);

/*
**	How far Phase 7 cuts the ground down at `world` to realize the accepted water, in world units.
**
**	This is the realization of the plan's `water_carve` term and it is deliberately not Phase 6's
**	`Hybrid_Water_Carve_Depth`: inside a channel the ground is taken to the frozen *bed height* from
**	step 5's walk rather than a fixed depth below whatever the land happened to be, which is what
**	makes a river bed follow its own water surface downhill.  Phase 6's query stays what it is -- how
**	deep the water is -- for the callers that ask that question.
*/
float	Hybrid_Macro_Water_Carve(const HybridMacroTerrainParams & params,const HybridHydrologySet & hydrology,
											const HybridMacroTerrainSet & surfaces,const Vector2 & world,float base_height);

/*
**	The Section 8 Phase 7 height expression at `world`, in absolute world units, clamped to
**	`sea_level + min_elevation .. sea_level + max_elevation`:
**
**	  tier_height + macro_height_bias + landform - water_carve + detail * detail_protection_factor
**
**	`macro_height_bias` is the Phase-4 layer plus Phase 6's accepted `island_macro_height_bias`
**	blended across its exact `2W` shoreline band.
*/
float	Hybrid_Macro_Sample_Height(const HybridMacroTerrainParams & params,const HybridCoarseMatrices & matrices,
											 const HybridLandformSet & landforms,const HybridHydrologySet & hydrology,
											 const HybridMacroTerrainSet & surfaces,const Vector2 & world);

/*
**	Is `world` inside an explicit steep or water profile -- a cliff/canyon/ravine wall, a river bed or
**	bank, or a lake/sea polygon and its shoreline band?  The ordinary-slope enforcement pass runs
**	only outside these, which is the plan's rule and the reason a vertical-faced cliff survives
**	relaxation at all.
*/
bool	Hybrid_Macro_Is_Protected_Profile(const HybridMacroTerrainParams & params,const HybridLandformSet & landforms,
														const HybridHydrologySet & hydrology,const Vector2 & world);

/*
**	D-037 (Q-029).  The water-bed ceiling the realized ground must stay at or below at `world`, in
**	absolute world units: the lowest water surface covering it -- a non-island body's `SurfaceHeight`,
**	and the frozen Phase-7 surface at the nearest sample of every river whose own channel covers it.
**	False when `world` is dry, an island interior included, because no water stands there to support.
**
**	This is exactly the invariant the gate measures ("no lake or river bed above its own water
**	surface"), expressed once so the gate and the raster-support pass below ask the same question of
**	the same frozen record rather than keeping two opinions of where the bed may stand.
*/
bool	Hybrid_Water_Bed_Ceiling(const HybridMacroTerrainParams & params,const HybridHydrologySet & hydrology,
											const HybridMacroTerrainSet & surfaces,const Vector2 & world,float * ceiling_out);

/*
**	D-037 (Q-029): bounded water-bed raster support.
**
**	The Phase-7 carve is a pointwise expression and stops exactly at the frozen Phase-6 water polygon,
**	but the native grid realizes it as triangles, and one 3.75 m triangle can straddle that boundary.
**	Interpolation across such a cell then lifts the realized ground above the water surface at wet
**	points that the continuous expression had put below it -- the water is correct, its discrete
**	realization is not.
**
**	This pass corrects the realization and nothing else.  For every native cell whose footprint meets
**	the frozen water (a body ring, an island ring or a river channel) it evaluates the interpolation
**	`HeightfieldClass::Sample_Cell` will perform, at the vertices of that cell's wet region, and
**	lowers the least amount on the vertices that interpolation actually draws from -- a vertex outside
**	the polygon included, which stays dry land and never becomes part of the water body.  A vertex is
**	never taken below the ceiling it has to satisfy, never below the elevation floor, never because it
**	merely sits near water, and never in a cell with no wet region at all.  No water polygon, water
**	level, shoreline, topology or tolerance is touched.
**
**	`heights` is the grid the generator is about to write, relative to `params.SeaLevel`, row major,
**	`vx * vy` vertices at `cell_size` spacing from `origin`.  `immutable` marks vertices no generator
**	authority may move (the boundary cliff band's locked foot); it may be null.  The return value is
**	the number of vertices lowered, and `unresolved_out` receives the number of wet samples the
**	bounded correction could not satisfy -- those stay a legitimate hard gate failure.
*/
int	Hybrid_Apply_Water_Bed_Raster_Support(const HybridMacroTerrainParams & params,
															const HybridHydrologySet & hydrology,
															const HybridMacroTerrainSet & surfaces,
															float * heights,const bool * immutable,int vx,int vy,
															float cell_size,const Vector2 & origin,int * unresolved_out);

/*
**	The Phase 7 gate, run after the heightfield exists: the elevation envelope, every river surface
**	non-increasing source to sink, no lake or river bed above its own water surface, and every
**	Phase-5/6 feature still represented in the realized ground.  `sample` reads the written
**	heightfield at a world point (false when the point is outside it), so the gate never assumes a
**	particular heightfield owner.
*/
typedef bool (*HybridMacroHeightSampleFunc)(const Vector2 & world,float * height_out,void * context);

bool	Hybrid_Check_Macro_Terrain_Gate(const HybridMacroTerrainParams & params,const HybridCoarseMatrices & matrices,
													const HybridLandformSet & landforms,const HybridHydrologySet & hydrology,
													HybridMacroTerrainSet * surfaces,HybridMacroHeightSampleFunc sample,
													void * context,HybridMacroTerrainResult * result);

/*
**	The Phase 7 gate value: a quantized digest of the realized extents and every frozen river
**	surface/bed sample.  The heightfield's own checksum stays `WorldTerrainSystem`'s.
*/
unsigned int	Hybrid_Macro_Terrain_Checksum(const HybridMacroTerrainSet & set,unsigned int seed = 0);


#endif // HYBRIDMACROTERRAIN_H
