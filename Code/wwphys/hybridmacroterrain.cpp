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
 *                     $Archive:: /Commando/Code/wwphys/hybridmacroterrain.cpp                $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Runtime Phase 7's height expression, its detail-protection rules, its river longitudinal      *
 * walk and its gate.  See hybridmacroterrain.h for why the formula lives here while the         *
 * heightfield stays `WorldTerrainSystem`'s and the write stays `MacroTerrainGenerator`'s.       *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#include "hybridmacroterrain.h"

#include "wwdebug.h"
#include "wwmath.h"

#include <math.h>
#include <string.h>


static inline float Max2(float a,float b)	{ return (a > b) ? a : b; }
static inline float Min2(float a,float b)	{ return (a < b) ? a : b; }
static inline float Clamp2(float v,float lo,float hi)	{ return (v < lo) ? lo : ((v > hi) ? hi : v); }

static inline float Smooth(float t)
{
	const float s = Clamp2(t,0.0f,1.0f);
	return s * s * (3.0f - (2.0f * s));
}


/*
**	Section 8 Phase 7's own constants, each used exactly once below.
*/
static const float	DETAIL_TIER_FRACTION		= 0.35f;		// detail_raw = detail_weight * tier_step * 0.35 * ...
static const float	DETAIL_OCTAVE0_WEIGHT	= 0.70f;
static const float	DETAIL_OCTAVE1_WEIGHT	= 0.30f;
static const float	DETAIL_OCTAVE0_S			= 8.0f;		// max(feature_scale, 8S)
static const float	DETAIL_OCTAVE1_S			= 4.0f;		// max(feature_scale/3, 4S)
static const float	DETAIL_OCTAVE1_DIVISOR	= 3.0f;
static const float	TIBERIUM_PROTECTION		= 0.25f;		// 0.25 inside a provisional Tiberium reserve
static const float	SHORE_BLEND_W				= 2.0f;		// smoothstep across 2W outward from a shoreline
static const float	BANK_BLEND_W				= 2.0f;		// bank blend is max(river_width, 2W)

/*
**	The plan writes the detail-noise draw as `Hash01(terrain,"detail_noise",octave,ix,iy)`: four keys,
**	where the shared Section-0.3 helper takes three.  The octave folds into the named key with the
**	standard golden-ratio odd constant, which separates the two octaves this phase uses without
**	touching the lattice keys, so `ix` and `iy` still key the hash exactly as the plan spells them.
*/
static const unsigned int	NOISE_OCTAVE_STRIDE = 0x9E3779B1u;


const char * Hybrid_Macro_Terrain_Failure_Name(HybridMacroTerrainFailure failure)
{
	switch (failure) {
		case HYBRID_MACRO_OK:						return "ok";
		case HYBRID_MACRO_INVALID_INPUT:			return "invalid_input";
		case HYBRID_MACRO_BED_ABOVE_SURFACE:	return "bed_above_surface";
		case HYBRID_MACRO_RIVER_NOT_MONOTONIC:	return "river_not_monotonic";
		case HYBRID_MACRO_ELEVATION_LIMIT:		return "elevation_limit";
		case HYBRID_MACRO_FEATURE_LOST:			return "feature_lost";
		default:											break;
	}
	return "unknown";
}


HybridMacroTerrainParams::HybridMacroTerrainParams(void) :
	TerrainSeed(0),
	SeaLevel(0.0f),MinElevation(-30.0f),MaxElevation(90.0f),TierStep(18.0f),TierCount(3),
	DetailWeight(0.35f),FeatureScale(240.0f),RiverWidth(6.0f),MaxBuildableSlopeRadians(0.6f)
{
	memset(&Constants,0,sizeof(Constants));
}


void HybridMacroTerrainSet::Reset(void)
{
	Rivers.Delete_All();
	VertexCountX		= 0;
	VertexCountY		= 0;
	CellSize				= 0.0f;
	MinHeight			= 0.0f;
	MaxHeight			= 0.0f;
	ProtectedVertices	= 0;
	SlopeViolations		= 0;
	WaterBedSupportVertices		= 0;
	WaterBedSupportUnresolved	= 0;
	Checksum				= 0;
}


const HybridRiverSurface * HybridMacroTerrainSet::Find_River(unsigned int river_id) const
{
	for (int i = 0; i < Rivers.Count(); ++i) {
		if (Rivers[i].RiverId == river_id) return &Rivers[i];
	}
	return nullptr;
}


bool HybridMacroTerrainSet::River_Sample(unsigned int river_id,int index,float * surface_out,float * bed_out) const
{
	const HybridRiverSurface * river = Find_River(river_id);
	if (river == nullptr) return false;
	if ((index < 0) || (index >= river->Surface.Count())) return false;
	if (surface_out != nullptr)	*surface_out = river->Surface[index];
	if (bed_out != nullptr)			*bed_out = river->Bed[index];
	return true;
}


/***********************************************************************************************
**	Section 8 Phase 7's detail-noise primitive.
***********************************************************************************************/

float Hybrid_Value_Noise_2D(unsigned int terrain_seed,const Vector2 & point,float length,int octave)
{
	if (length <= 0.0f) return 0.0f;

	const float qx = point.X / length;
	const float qy = point.Y / length;
	const int ix = (int)::floor((double)qx);
	const int iy = (int)::floor((double)qy);
	const float fx = qx - (float)ix;
	const float fy = qy - (float)iy;
	const float ux = fx * fx * (3.0f - (2.0f * fx));
	const float uy = fy * fy * (3.0f - (2.0f * fy));

	const unsigned int key = Hybrid_Key("detail_noise") + ((unsigned int)octave * NOISE_OCTAVE_STRIDE);

	const float c00 = (2.0f * Hybrid_Hash01(terrain_seed,key,(unsigned int)ix,(unsigned int)iy)) - 1.0f;
	const float c10 = (2.0f * Hybrid_Hash01(terrain_seed,key,(unsigned int)(ix + 1),(unsigned int)iy)) - 1.0f;
	const float c01 = (2.0f * Hybrid_Hash01(terrain_seed,key,(unsigned int)ix,(unsigned int)(iy + 1))) - 1.0f;
	const float c11 = (2.0f * Hybrid_Hash01(terrain_seed,key,(unsigned int)(ix + 1),(unsigned int)(iy + 1))) - 1.0f;

	const float lower = c00 + ((c10 - c00) * ux);
	const float upper = c01 + ((c11 - c01) * ux);
	return lower + ((upper - lower) * uy);
}


/***********************************************************************************************
**	Shared geometry over the frozen Phase-6 records.  Rings are closed and stored without a
**	repeated final point, exactly as Phase 6 wrote them.
***********************************************************************************************/

static bool Point_In_Ring(const DynamicVectorClass<Vector2> & ring,const Vector2 & point)
{
	const int count = ring.Count();
	if (count < 3) return false;

	bool inside = false;
	for (int i = 0, j = count - 1; i < count; j = i++) {
		const Vector2 & a = ring[i];
		const Vector2 & b = ring[j];
		if (((a.Y > point.Y) != (b.Y > point.Y)) &&
			 (point.X < (((b.X - a.X) * (point.Y - a.Y) / (b.Y - a.Y)) + a.X))) {
			inside = !inside;
		}
	}
	return inside;
}


static float Distance_To_Segment(const Vector2 & point,const Vector2 & a,const Vector2 & b)
{
	const Vector2 axis = b - a;
	const float length_sq = (axis.X * axis.X) + (axis.Y * axis.Y);
	float t = 0.0f;
	if (length_sq > 0.0f) {
		t = (((point.X - a.X) * axis.X) + ((point.Y - a.Y) * axis.Y)) / length_sq;
		t = Clamp2(t,0.0f,1.0f);
	}
	const float dx = point.X - (a.X + (axis.X * t));
	const float dy = point.Y - (a.Y + (axis.Y * t));
	return (float)sqrt((double)((dx * dx) + (dy * dy)));
}


static float Distance_To_Ring(const DynamicVectorClass<Vector2> & ring,const Vector2 & point)
{
	const int count = ring.Count();
	if (count == 0) return 0.0f;
	if (count == 1) return (point - ring[0]).Length();

	float best = 3.4e38f;
	for (int i = 0; i < count; ++i) {
		best = Min2(best,Distance_To_Segment(point,ring[i],ring[(i + 1) % count]));
	}
	return best;
}


//	The nearest centerline sample of a river, and the distance to it.
static float Nearest_River_Sample(const HybridRiverSystem & river,const Vector2 & point,int * index_out)
{
	const int count = river.Points.Count();
	if (index_out != nullptr) *index_out = -1;
	if (count == 0) return 3.4e38f;
	if (count == 1) {
		if (index_out != nullptr) *index_out = 0;
		return (point - river.Points[0]).Length();
	}

	float best	= 3.4e38f;
	int	at		= 0;
	for (int i = 0; i < (count - 1); ++i) {
		const float distance = Distance_To_Segment(point,river.Points[i],river.Points[i + 1]);
		if (distance >= best) continue;
		best	= distance;
		at		= ((point - river.Points[i]).Length() <= (point - river.Points[i + 1]).Length()) ? i : (i + 1);
	}
	const float last = (point - river.Points[count - 1]).Length();
	if (last < best) {
		best	= last;
		at		= count - 1;
	}
	if (index_out != nullptr) *index_out = at;
	return best;
}


static int Middle_Tier(int tier_count)
{
	return (tier_count > 0) ? ((tier_count - 1) / 2) : 0;
}


//	The coarse cell a world point falls in, clamped to the matrix, which is how Phase 4's layers
//	are read outside the matrix rectangle (the boundary band's own values).
static int Coarse_Index(const HybridSemanticMatrix & matrix,const Vector2 & world)
{
	if (matrix.Get_Cell_Count() <= 0) return -1;

	int cx = 0, cy = 0;
	if (!matrix.World_To_Cell(world,&cx,&cy)) {
		const float cell = matrix.Get_Cell_Size();
		if (cell <= 0.0f) return -1;
		cx = (int)((world.X - matrix.Get_Origin_X()) / cell);
		cy = (int)((world.Y - matrix.Get_Origin_Y()) / cell);
		if (cx < 0) cx = 0;
		if (cy < 0) cy = 0;
		if (cx > (matrix.Get_Count_X() - 1)) cx = matrix.Get_Count_X() - 1;
		if (cy > (matrix.Get_Count_Y() - 1)) cy = matrix.Get_Count_Y() - 1;
	}
	return matrix.Index(cx,cy);
}


/***********************************************************************************************
**	Step 5: the river longitudinal walk.
***********************************************************************************************/

static bool Fail(HybridMacroTerrainResult * result,HybridMacroTerrainFailure failure,const char * detail)
{
	if (result != nullptr) {
		result->Failure	= failure;
		result->Detail		= (detail != nullptr) ? detail : "";
	}
	return false;
}


bool Hybrid_Resolve_River_Surfaces(const HybridMacroTerrainParams & params,const HybridCoarseMatrices & matrices,
													const HybridLandformSet & landforms,const HybridHydrologySet & hydrology,
													HybridMacroTerrainSet * out,HybridMacroTerrainResult * result)
{
	if (result != nullptr) {
		result->Failure	= HYBRID_MACRO_OK;
		result->Detail		= "";
	}
	if (out == nullptr) {
		return Fail(result,HYBRID_MACRO_INVALID_INPUT,"no output record");
	}
	(void)matrices;
	(void)landforms;

	out->Reset();
	const float floor_height = params.SeaLevel + params.MinElevation;
	StringClass detail;

	for (int i = 0; i < hydrology.Rivers.Count(); ++i) {

		const HybridRiverSystem & river = hydrology.Rivers[i];
		const int count = river.Points.Count();
		if (count <= 0) continue;

		HybridRiverSurface record;
		record.RiverId		= river.StableId;
		record.SinkBodyId	= river.SinkBodyId;

		/*
		**	1 -- the provisional surface sample.  Phase 6 already stored exactly what the plan asks for
		**	per river kind: the filled drainage height for a D8 river, the provisional terrain height for
		**	a guided one.  Both are quantized to `Q` here, which is where the plan puts the quantization.
		*/
		for (int point = 0; point < count; ++point) {
			record.Surface.Add(Hybrid_Quantize_Position(river.Surfaces[point]));
		}

		//	2 -- a mouth bound to a lake or the sea equals that sink's water level.
		if (river.SinkBodyId != 0) {
			const HybridWaterBody * sink = hydrology.Find_Body(river.SinkBodyId);
			if (sink != nullptr) {
				record.Surface[count - 1] = Hybrid_Quantize_Position(sink->SurfaceHeight);
			}
		}
		record.MouthSurface = record.Surface[count - 1];

		//	3 -- walk downstream: never rise.
		for (int point = 1; point < count; ++point) {
			record.Surface[point] = Min2(record.Surface[point],record.Surface[point - 1]);
		}

		/*
		**	The mouth is *fixed*, so the downstream walk may not move it: a sink level above the walk's
		**	own answer is restored here and step 4 then raises the reach above it, which is exactly what
		**	the plan means by walking upstream "from the fixed mouth".
		*/
		if (river.SinkBodyId != 0) {
			record.Surface[count - 1] = record.MouthSurface;
		}

		//	4 -- walk upstream from the fixed mouth, which restores the sink level without rising downstream.
		for (int point = count - 2; point >= 0; --point) {
			record.Surface[point] = Max2(record.Surface[point],record.Surface[point + 1]);
		}

		//	5 -- the bed target, clamped only by the global terrain minimum.
		for (int point = 0; point < count; ++point) {

			const float bed = Hybrid_Quantize_Position(Max2(record.Surface[point] - river.BedDepth,floor_height));
			if (bed > record.Surface[point]) {
				detail.Format("river %u sample %d: bed %.3f is above its surface %.3f after the %.3f floor",
									river.StableId,point,bed,record.Surface[point],floor_height);
				out->Reset();
				return Fail(result,HYBRID_MACRO_BED_ABOVE_SURFACE,detail.Peek_Buffer());
			}
			record.Bed.Add(bed);
		}

		out->Rivers.Add(record);
	}
	return true;
}


/***********************************************************************************************
**	The height expression.
***********************************************************************************************/

float Hybrid_Detail_Protection_Factor(const HybridMacroTerrainParams & params,const HybridCoarseMatrices & matrices,
													 const HybridHydrologySet & hydrology,const Vector2 & world)
{
	const HybridSemanticMatrix & matrix = matrices.Matrix;
	const float w = params.Constants.W;
	const float bank = Max2(params.RiverWidth,BANK_BLEND_W * w);
	const float shore = SHORE_BLEND_W * w;
	float factor = 1.0f;

	//	The two semantic reserves.  A base *core* is zero; the expansion ring is not named here.
	const int index = Coarse_Index(matrix,world);
	if (index >= 0) {
		if ((matrix.Flags[index] & HYBRID_FLAG_BASE_CORE) != 0)					factor = Min2(factor,0.0f);
		if ((matrix.Flags[index] & HYBRID_FLAG_TIBERIUM_RESERVE) != 0)		factor = Min2(factor,TIBERIUM_PROTECTION);
	}

	for (int i = 0; i < hydrology.Bodies.Count(); ++i) {

		const HybridWaterBody & body = hydrology.Bodies[i];
		if (body.Kind == HYBRID_WATER_BODY_ISLAND) continue;
		if ((world.X < (body.BoundsMin.X - shore)) || (world.X > (body.BoundsMax.X + shore)) ||
			 (world.Y < (body.BoundsMin.Y - shore)) || (world.Y > (body.BoundsMax.Y + shore))) continue;

		if (Point_In_Ring(body.Points,world)) {
			factor = Min2(factor,0.0f);
			continue;
		}
		const float distance = Distance_To_Ring(body.Points,world);
		if (distance >= shore) continue;
		factor = Min2(factor,Smooth(distance / shore));
	}

	for (int i = 0; i < hydrology.Rivers.Count(); ++i) {

		const HybridRiverSystem & river = hydrology.Rivers[i];
		if ((world.X < (river.BoundsMin.X - bank)) || (world.X > (river.BoundsMax.X + bank)) ||
			 (world.Y < (river.BoundsMin.Y - bank)) || (world.Y > (river.BoundsMax.Y + bank))) continue;

		int index_at = -1;
		const float distance = Nearest_River_Sample(river,world,&index_at);
		const float half = 0.5f * river.Width_At_Index(index_at);

		if (distance <= half) {
			factor = Min2(factor,0.0f);
			continue;
		}
		if (distance >= (half + bank)) continue;
		factor = Min2(factor,Smooth((distance - half) / bank));
	}

	//	An island is dry land: it keeps whatever protection the rules above did not take away, so no
	//	extra rule is needed here -- its own polygon is excluded from the water tests above.
	return Clamp2(factor,0.0f,1.0f);
}


float Hybrid_Macro_Water_Carve(const HybridMacroTerrainParams & params,const HybridHydrologySet & hydrology,
											const HybridMacroTerrainSet & surfaces,const Vector2 & world,float base_height)
{
	const float w = params.Constants.W;
	const float bank = Max2(params.RiverWidth,BANK_BLEND_W * w);

	//	Inside an island nothing is carved at all: step 17 suppressed its host's carve there.
	for (int i = 0; i < hydrology.Bodies.Count(); ++i) {
		const HybridWaterBody & body = hydrology.Bodies[i];
		if (body.Kind != HYBRID_WATER_BODY_ISLAND) continue;
		if ((world.X < body.BoundsMin.X) || (world.X > body.BoundsMax.X) ||
			 (world.Y < body.BoundsMin.Y) || (world.Y > body.BoundsMax.Y)) continue;
		if (Point_In_Ring(body.Points,world)) return 0.0f;
	}

	float carve = 0.0f;

	for (int i = 0; i < hydrology.Bodies.Count(); ++i) {

		const HybridWaterBody & body = hydrology.Bodies[i];
		if (body.Kind == HYBRID_WATER_BODY_ISLAND) continue;
		if ((world.X < body.BoundsMin.X) || (world.X > body.BoundsMax.X) ||
			 (world.Y < body.BoundsMin.Y) || (world.Y > body.BoundsMax.Y)) continue;
		if (!Point_In_Ring(body.Points,world)) continue;

		/*
		**	The bed a standing body stands on is its own surface less its `BedDepth`.  Ocean and coast
		**	carry a zero depth (`WaterSystem::Define_Default_Water`), so the sea floor is cut exactly to
		**	sea level -- the plan's "coast does not reshape the ground" with the one cut that still puts
		**	the shoreline where the polygon says it is.
		*/
		carve = Max2(carve,Max2(0.0f,base_height - (body.SurfaceHeight - body.BedDepth)));
	}

	for (int i = 0; i < hydrology.Rivers.Count(); ++i) {

		const HybridRiverSystem & river = hydrology.Rivers[i];
		if ((world.X < (river.BoundsMin.X - bank)) || (world.X > (river.BoundsMax.X + bank)) ||
			 (world.Y < (river.BoundsMin.Y - bank)) || (world.Y > (river.BoundsMax.Y + bank))) continue;

		int index_at = -1;
		const float distance = Nearest_River_Sample(river,world,&index_at);
		const float half = 0.5f * river.Width_At_Index(index_at);
		if (distance >= (half + bank)) continue;

		float bed = 0.0f;
		if (!surfaces.River_Sample(river.StableId,index_at,nullptr,&bed)) continue;

		const float full = Max2(0.0f,base_height - bed);
		if (distance <= half) {
			carve = Max2(carve,full);
			continue;
		}
		carve = Max2(carve,full * (1.0f - Smooth((distance - half) / bank)));
	}

	return carve;
}


float Hybrid_Macro_Sample_Height(const HybridMacroTerrainParams & params,const HybridCoarseMatrices & matrices,
											 const HybridLandformSet & landforms,const HybridHydrologySet & hydrology,
											 const HybridMacroTerrainSet & surfaces,const Vector2 & world)
{
	const HybridSemanticMatrix & matrix = matrices.Matrix;
	const int index = Coarse_Index(matrix,world);
	const int middle = Middle_Tier(params.TierCount);

	const int tier = (index >= 0) ? (int)matrix.BaseElevationTier[index] : middle;
	const float tier_height = params.SeaLevel + ((float)(tier - middle) * params.TierStep);

	//	`macro_height_bias` is the Phase-4 layer plus Phase 6's island bias, already shoreline blended.
	float bias = (index >= 0) ? matrix.MacroHeightBias[index] : 0.0f;
	bias += Hybrid_Island_Height_Bias(hydrology,world);

	const float landform = Hybrid_Landform_Height(landforms,world,params.Constants.A);
	const float base = tier_height + bias + landform;

	const float carve = Hybrid_Macro_Water_Carve(params,hydrology,surfaces,world,base);

	const float length0 = Max2(params.FeatureScale,DETAIL_OCTAVE0_S * params.Constants.S);
	const float length1 = Max2(params.FeatureScale / DETAIL_OCTAVE1_DIVISOR,DETAIL_OCTAVE1_S * params.Constants.S);
	const float n0 = Hybrid_Value_Noise_2D(params.TerrainSeed,world,length0,0);
	const float n1 = Hybrid_Value_Noise_2D(params.TerrainSeed,world,length1,1);
	const float detail_raw = params.DetailWeight * params.TierStep * DETAIL_TIER_FRACTION *
									 ((DETAIL_OCTAVE0_WEIGHT * n0) + (DETAIL_OCTAVE1_WEIGHT * n1));
	const float detail = detail_raw * Hybrid_Detail_Protection_Factor(params,matrices,hydrology,world);

	const float height = base - carve + detail;
	return Clamp2(height,params.SeaLevel + params.MinElevation,params.SeaLevel + params.MaxElevation);
}


bool Hybrid_Macro_Is_Protected_Profile(const HybridMacroTerrainParams & params,const HybridLandformSet & landforms,
														const HybridHydrologySet & hydrology,const Vector2 & world)
{
	const float w = params.Constants.W;
	const float bank = Max2(params.RiverWidth,BANK_BLEND_W * w);
	const float shore = SHORE_BLEND_W * w;

	//	An explicit cliff, canyon or ravine wall: the vertical geometry ordinary slope must not flatten.
	for (int i = 0; i < landforms.Landforms.Count(); ++i) {
		const HybridLandform & landform = landforms.Landforms[i];
		if (!Hybrid_Landform_Is_Immutable(landform.Kind)) continue;
		if ((world.X < landform.BoundsMin.X) || (world.X > landform.BoundsMax.X) ||
			 (world.Y < landform.BoundsMin.Y) || (world.Y > landform.BoundsMax.Y)) continue;
		if (Hybrid_Landform_Contribution(landform,world,nullptr) != 0.0f) return true;
	}

	for (int i = 0; i < hydrology.Bodies.Count(); ++i) {
		const HybridWaterBody & body = hydrology.Bodies[i];
		if (body.Kind == HYBRID_WATER_BODY_ISLAND) continue;
		if ((world.X < (body.BoundsMin.X - shore)) || (world.X > (body.BoundsMax.X + shore)) ||
			 (world.Y < (body.BoundsMin.Y - shore)) || (world.Y > (body.BoundsMax.Y + shore))) continue;
		if (Point_In_Ring(body.Points,world)) return true;
		if (Distance_To_Ring(body.Points,world) < shore) return true;
	}

	for (int i = 0; i < hydrology.Rivers.Count(); ++i) {
		const HybridRiverSystem & river = hydrology.Rivers[i];
		if ((world.X < (river.BoundsMin.X - bank)) || (world.X > (river.BoundsMax.X + bank)) ||
			 (world.Y < (river.BoundsMin.Y - bank)) || (world.Y > (river.BoundsMax.Y + bank))) continue;
		int index_at = -1;
		const float distance = Nearest_River_Sample(river,world,&index_at);
		if (distance <= ((0.5f * river.Width_At_Index(index_at)) + bank)) return true;
	}

	return false;
}


/***********************************************************************************************
**	D-037 (Q-029): bounded water-bed raster support.
***********************************************************************************************/

bool Hybrid_Water_Bed_Ceiling(const HybridMacroTerrainParams & params,const HybridHydrologySet & hydrology,
											const HybridMacroTerrainSet & surfaces,const Vector2 & world,float * ceiling_out)
{
	//	An island is dry land standing in water: nothing supports a bed there, exactly as the carve says.
	for (int i = 0; i < hydrology.Bodies.Count(); ++i) {
		const HybridWaterBody & body = hydrology.Bodies[i];
		if (body.Kind != HYBRID_WATER_BODY_ISLAND) continue;
		if ((world.X < body.BoundsMin.X) || (world.X > body.BoundsMax.X) ||
			 (world.Y < body.BoundsMin.Y) || (world.Y > body.BoundsMax.Y)) continue;
		if (Point_In_Ring(body.Points,world)) return false;
	}

	bool wet = false;
	float ceiling = 0.0f;

	for (int i = 0; i < hydrology.Bodies.Count(); ++i) {
		const HybridWaterBody & body = hydrology.Bodies[i];
		if (body.Kind == HYBRID_WATER_BODY_ISLAND) continue;
		if ((world.X < body.BoundsMin.X) || (world.X > body.BoundsMax.X) ||
			 (world.Y < body.BoundsMin.Y) || (world.Y > body.BoundsMax.Y)) continue;
		if (!Point_In_Ring(body.Points,world)) continue;
		ceiling = wet ? Min2(ceiling,body.SurfaceHeight) : body.SurfaceHeight;
		wet = true;
	}

	//	A river supports a bed only inside its own channel.  The bank blend is a carve profile, not
	//	water: ground under a bank is dry and carries no ceiling of its own.
	for (int i = 0; i < hydrology.Rivers.Count(); ++i) {

		const HybridRiverSystem & river = hydrology.Rivers[i];
		if ((world.X < river.BoundsMin.X) || (world.X > river.BoundsMax.X) ||
			 (world.Y < river.BoundsMin.Y) || (world.Y > river.BoundsMax.Y)) continue;

		int index_at = -1;
		const float distance = Nearest_River_Sample(river,world,&index_at);
		if (distance > (0.5f * river.Width_At_Index(index_at))) continue;

		float surface = 0.0f;
		if (!surfaces.River_Sample(river.StableId,index_at,&surface,nullptr)) continue;
		ceiling = wet ? Min2(ceiling,surface) : surface;
		wet = true;
	}

	if (!wet) return false;
	if (ceiling_out != nullptr) *ceiling_out = ceiling;
	return true;
}


/*
**	The cells whose footprint can hold a shoreline: every cell a water ring segment or a river
**	segment's own channel passes through.  A cell entirely inside water needs no correction (every
**	corner is already carved to or below the ceiling, so no convex combination of them can exceed it)
**	and a cell entirely outside it has no wet region to support, which is where the bound in D-037
**	comes from -- the footprint, never a distance.  The walk below is per water segment rather than
**	per cell, so the cost follows the length of the shoreline instead of the area of the world.
*/
struct HybridSupportCellRange
{
	int	X0;
	int	X1;
	int	Y0;
	int	Y1;
};

static bool Support_Cell_Range(int cells_x,int cells_y,const Vector2 & origin,float cell_size,
											const Vector2 & lo,const Vector2 & hi,HybridSupportCellRange * out)
{
	//	One cell of slack, so a segment lying exactly on a cell boundary takes in the cell either side
	//	of it.  Visiting a cell with no wet region in it costs a few tests and can change nothing.
	out->X0 = (int)WWMath::Floor((lo.X - origin.X) / cell_size) - 1;
	out->X1 = (int)WWMath::Floor((hi.X - origin.X) / cell_size) + 1;
	out->Y0 = (int)WWMath::Floor((lo.Y - origin.Y) / cell_size) - 1;
	out->Y1 = (int)WWMath::Floor((hi.Y - origin.Y) / cell_size) + 1;

	if (out->X0 < 0) out->X0 = 0;
	if (out->Y0 < 0) out->Y0 = 0;
	if (out->X1 > (cells_x - 1)) out->X1 = cells_x - 1;
	if (out->Y1 > (cells_y - 1)) out->Y1 = cells_y - 1;

	return (out->X0 <= out->X1) && (out->Y0 <= out->Y1);
}


//	Where two segments meet, if they do.  The wet region of a cell is a polygon whose vertices are
//	cell corners, water-ring vertices inside the cell, and exactly these crossings; a linear
//	interpolant over that region takes its maximum at one of them.
static bool Segment_Crossing(const Vector2 & a,const Vector2 & b,const Vector2 & c,const Vector2 & d,
										Vector2 * out)
{
	const Vector2 r = b - a;
	const Vector2 s = d - c;
	const float denominator = (r.X * s.Y) - (r.Y * s.X);
	if ((float)fabs((double)denominator) <= 1.0e-9f) return false;

	const Vector2 delta = c - a;
	const float t = ((delta.X * s.Y) - (delta.Y * s.X)) / denominator;
	const float u = ((delta.X * r.Y) - (delta.Y * r.X)) / denominator;
	if ((t < 0.0f) || (t > 1.0f) || (u < 0.0f) || (u > 1.0f)) return false;

	if (out != nullptr) *out = a + (r * t);
	return true;
}


static int Add_Cell_Crossings(Vector2 * candidates,const Vector2 & a,const Vector2 & b,
											 const Vector2 & corner0,float cell_size)
{
	const Vector2 corner1(corner0.X + cell_size,corner0.Y);
	const Vector2 corner2(corner0.X + cell_size,corner0.Y + cell_size);
	const Vector2 corner3(corner0.X,corner0.Y + cell_size);

	//	The four edges and the cell's own diagonal, which is the triangle split the sampler uses.
	const Vector2 edges[5][2] = {
		{ corner0,corner1 },
		{ corner1,corner2 },
		{ corner2,corner3 },
		{ corner3,corner0 },
		{ corner0,corner2 }
	};

	int count = 0;
	for (int edge = 0; edge < 5; ++edge) {
		Vector2 crossing(0.0f,0.0f);
		if (!Segment_Crossing(a,b,edges[edge][0],edges[edge][1],&crossing)) continue;
		candidates[count ++] = crossing;
	}
	return count;
}


/*
**	The interpolation `HeightfieldClass::Sample_Cell` performs, as weights on the four cell corners,
**	so the correction below reasons about the same arithmetic the runtime sampler will.  Corner order
**	is the sampler's: (cx,cy), (cx+1,cy), (cx+1,cy+1), (cx,cy+1).
*/
static void Cell_Interpolation_Weights(float u,float v,float * weights)
{
	u = Clamp2(u,0.0f,1.0f);
	v = Clamp2(v,0.0f,1.0f);

	weights[0] = 0.0f;
	weights[1] = 0.0f;
	weights[2] = 0.0f;
	weights[3] = 0.0f;

	if (u >= v) {
		//	Below the diagonal: h = v0 + (v1 - v0) u + (v2 - v1) v.
		weights[0] = 1.0f - u;
		weights[1] = u - v;
		weights[2] = v;
	} else {
		//	Above it: h = v0 + (v2 - v3) u + (v3 - v0) v.
		weights[0] = 1.0f - v;
		weights[2] = u;
		weights[3] = v - u;
	}
}


/*
**	One cell, against one water segment that passes through it.  The candidate points are the vertices
**	of the cell's wet region -- its corners, the segment's own vertex when it falls inside, the
**	segment's crossings of the cell's edges and of its diagonal -- plus a fixed interior lattice,
**	because a river channel's boundary is an offset curve rather than a polygon.  A linear interpolant
**	takes its maximum over a convex region at a vertex of that region, which is what makes this set the
**	right one to measure rather than a dense sweep of the cell.
**
**	`boundary_ceiling` is the water surface on the wet side of this segment.  The candidates the
**	segment itself produces lie *on* the water boundary, where the pointwise test answers "dry" because
**	a ring excludes its own edge; they belong to the closure of the wet region all the same, and they
**	are where the interpolant takes its maximum, so they are measured against that surface.  A cell
**	corner or lattice point is only ever measured when it is strictly wet.
*/
static void Correct_Support_Cell(const HybridMacroTerrainParams & params,const HybridHydrologySet & hydrology,
											  const HybridMacroTerrainSet & surfaces,float * heights,
											  const bool * immutable,bool * lowered,int vx,float cell_size,
											  const Vector2 & origin,int cx,int cy,const Vector2 & seg_a,
											  const Vector2 & seg_b,bool has_boundary_ceiling,
											  float boundary_ceiling,float channel_half_width,int * unresolved)
{
	const float floor_rel = params.MinElevation;
	const float epsilon = Max2(HYBRID_QUANTUM_POSITION * 0.5f,0.001f);

	const Vector2 corner0(origin.X + ((float)cx * cell_size),origin.Y + ((float)cy * cell_size));
	const Vector2 corner2(corner0.X + cell_size,corner0.Y + cell_size);

	Vector2 candidates[32];
	bool on_boundary[32];
	int count = 0;

	candidates[count] = corner0;								on_boundary[count ++] = false;
	candidates[count] = Vector2(corner2.X,corner0.Y);	on_boundary[count ++] = false;
	candidates[count] = corner2;								on_boundary[count ++] = false;
	candidates[count] = Vector2(corner0.X,corner2.Y);	on_boundary[count ++] = false;

	for (int ly = 1; ly <= 3; ++ly) {
		for (int lx = 1; lx <= 3; ++lx) {
			candidates[count] = Vector2(corner0.X + (cell_size * 0.25f * (float)lx),
												 corner0.Y + (cell_size * 0.25f * (float)ly));
			on_boundary[count ++] = false;
		}
	}

	const int boundary_first = count;

	if ((seg_a.X >= corner0.X) && (seg_a.X <= corner2.X) && (seg_a.Y >= corner0.Y) && (seg_a.Y <= corner2.Y)) {
		candidates[count ++] = seg_a;
	}
	count += Add_Cell_Crossings(&candidates[count],seg_a,seg_b,corner0,cell_size);

	/*
	**	A channel's lateral boundary is an offset curve with no vertices of its own, so the two offsets
	**	of this segment's own end stand in for it where they fall inside the cell.
	*/
	if (channel_half_width > 0.0f) {
		Vector2 along = seg_b - seg_a;
		if (along.Length() > 0.0f) {
			along.Normalize();
			const Vector2 across(-along.Y,along.X);
			for (int side = -1; side <= 1; side += 2) {
				const Vector2 edge = seg_a + (across * (channel_half_width * (float)side));
				if ((edge.X < corner0.X) || (edge.X > corner2.X) ||
					 (edge.Y < corner0.Y) || (edge.Y > corner2.Y)) continue;
				candidates[count ++] = edge;
			}
		}
	}

	for (int i = boundary_first; i < count; ++i) {
		on_boundary[i] = true;
	}

	const int corner_index[4] = {
		(cy * vx) + cx,
		(cy * vx) + cx + 1,
		((cy + 1) * vx) + cx + 1,
		((cy + 1) * vx) + cx
	};

	for (int i = 0; i < count; ++i) {

		const Vector2 & point = candidates[i];

		float ceiling_abs = 0.0f;
		bool wet = Hybrid_Water_Bed_Ceiling(params,hydrology,surfaces,point,&ceiling_abs);
		if (on_boundary[i] && has_boundary_ceiling) {
			ceiling_abs = wet ? Min2(ceiling_abs,boundary_ceiling) : boundary_ceiling;
			wet = true;
		}
		if (!wet) continue;

		const float ceiling_rel = ceiling_abs - params.SeaLevel;
		const float target = Max2(ceiling_rel,floor_rel);

		float weights[4];
		Cell_Interpolation_Weights((point.X - corner0.X) / cell_size,(point.Y - corner0.Y) / cell_size,weights);

		float realized = 0.0f;
		for (int corner = 0; corner < 4; ++corner) {
			realized += weights[corner] * heights[corner_index[corner]];
		}

		float excess = realized - ceiling_rel;
		if (excess <= epsilon) continue;

		/*
		**	Lower the least, on the vertices this interpolation actually draws from.  The heaviest weight
		**	first, because that is the smallest total correction that removes the excess, and never past
		**	the ceiling the wet point itself has to satisfy -- beyond that a drop would be flattening the
		**	shoreline rather than supporting the water.
		*/
		for (int attempt = 0; (attempt < 4) && (excess > epsilon); ++attempt) {

			int chosen = -1;
			for (int corner = 0; corner < 4; ++corner) {
				if (weights[corner] <= epsilon) continue;
				if ((immutable != nullptr) && immutable[corner_index[corner]]) continue;
				if (heights[corner_index[corner]] <= (target + epsilon)) continue;
				if (chosen < 0) {
					chosen = corner;
					continue;
				}
				if (weights[corner] > weights[chosen]) chosen = corner;
			}
			if (chosen < 0) break;

			const int index = corner_index[chosen];
			const float drop = Min2(excess / weights[chosen],heights[index] - target);

			heights[index] -= drop;
			excess -= drop * weights[chosen];
			lowered[index] = true;
		}

		//	A remaining excess is a real conflict with a frozen invariant -- the elevation floor, or a
		//	cliff-band vertex -- and stays the gate's hard failure rather than a silent pass.
		if (excess > epsilon) (*unresolved) ++;
	}
}


int Hybrid_Apply_Water_Bed_Raster_Support(const HybridMacroTerrainParams & params,
																	const HybridHydrologySet & hydrology,
																	const HybridMacroTerrainSet & surfaces,
																	float * heights,const bool * immutable,int vx,int vy,
																	float cell_size,const Vector2 & origin,int * unresolved_out)
{
	if (unresolved_out != nullptr) *unresolved_out = 0;
	if ((heights == nullptr) || (vx < 2) || (vy < 2) || (cell_size <= 0.0f)) return 0;
	if ((hydrology.Bodies.Count() == 0) && (hydrology.Rivers.Count() == 0)) return 0;

	const int cells_x = vx - 1;
	const int cells_y = vy - 1;

	bool * lowered = new bool[vx * vy];
	memset(lowered,0,sizeof(bool) * (size_t)(vx * vy));

	int unresolved = 0;
	HybridSupportCellRange range;

	//	Every water ring, islands included: a cell straddling an island's shore has a wet region too,
	//	on the host's side of it, so an island's own ring is measured against its host's water surface.
	for (int i = 0; i < hydrology.Bodies.Count(); ++i) {

		const HybridWaterBody & body = hydrology.Bodies[i];

		bool has_ceiling = (body.Kind != HYBRID_WATER_BODY_ISLAND);
		float ceiling = body.SurfaceHeight;
		if (!has_ceiling) {
			for (int host = 0; host < hydrology.Bodies.Count(); ++host) {
				if (hydrology.Bodies[host].StableId != body.HostBodyId) continue;
				if (hydrology.Bodies[host].Kind == HYBRID_WATER_BODY_ISLAND) continue;
				ceiling = hydrology.Bodies[host].SurfaceHeight;
				has_ceiling = true;
				break;
			}
		}

		const DynamicVectorClass<Vector2> & ring = body.Points;
		for (int point = 0; point < ring.Count(); ++point) {

			const Vector2 & a = ring[point];
			const Vector2 & b = ring[(point + 1) % ring.Count()];
			if (!Support_Cell_Range(cells_x,cells_y,origin,cell_size,
											Vector2(Min2(a.X,b.X),Min2(a.Y,b.Y)),
											Vector2(Max2(a.X,b.X),Max2(a.Y,b.Y)),&range)) continue;

			for (int cy = range.Y0; cy <= range.Y1; ++cy) {
				for (int cx = range.X0; cx <= range.X1; ++cx) {
					Correct_Support_Cell(params,hydrology,surfaces,heights,immutable,lowered,vx,cell_size,origin,
												cx,cy,a,b,has_ceiling,ceiling,0.0f,&unresolved);
				}
			}
		}
	}

	//	Every river channel.  A centerline is an open polyline, never a ring: the last point has no
	//	segment after it, and the channel is the centerline inflated by its own local half width.
	for (int i = 0; i < hydrology.Rivers.Count(); ++i) {

		const HybridRiverSystem & river = hydrology.Rivers[i];
		for (int point = 0; point < (river.Points.Count() - 1); ++point) {

			const Vector2 & a = river.Points[point];
			const Vector2 & b = river.Points[point + 1];
			const float half = 0.5f * Max2(river.Width_At_Index(point),river.Width_At_Index(point + 1));
			if (!Support_Cell_Range(cells_x,cells_y,origin,cell_size,
											Vector2(Min2(a.X,b.X) - half,Min2(a.Y,b.Y) - half),
											Vector2(Max2(a.X,b.X) + half,Max2(a.Y,b.Y) + half),&range)) continue;

			//	The lower of the segment's two frozen surfaces, because a crossing can fall anywhere
			//	along it and the walk leaves the surface non-increasing downstream.
			float surface_a = 0.0f;
			float surface_b = 0.0f;
			const bool has_ceiling = surfaces.River_Sample(river.StableId,point,&surface_a,nullptr)
										  && surfaces.River_Sample(river.StableId,point + 1,&surface_b,nullptr);
			const float ceiling = Min2(surface_a,surface_b);

			for (int cy = range.Y0; cy <= range.Y1; ++cy) {
				for (int cx = range.X0; cx <= range.X1; ++cx) {
					Correct_Support_Cell(params,hydrology,surfaces,heights,immutable,lowered,vx,cell_size,origin,
												cx,cy,a,b,has_ceiling,ceiling,half,&unresolved);
				}
			}
		}
	}

	int count = 0;
	for (int i = 0; i < (vx * vy); ++i) {
		if (lowered[i]) count ++;
	}

	delete [] lowered;

	if (unresolved_out != nullptr) *unresolved_out = unresolved;
	return count;
}


/***********************************************************************************************
**	The Phase 7 gate.
***********************************************************************************************/

bool Hybrid_Check_Macro_Terrain_Gate(const HybridMacroTerrainParams & params,const HybridCoarseMatrices & matrices,
													const HybridLandformSet & landforms,const HybridHydrologySet & hydrology,
													HybridMacroTerrainSet * surfaces,HybridMacroHeightSampleFunc sample,
													void * context,HybridMacroTerrainResult * result)
{
	if (result != nullptr) {
		result->Failure	= HYBRID_MACRO_OK;
		result->Detail		= "";
	}
	if ((surfaces == nullptr) || (sample == nullptr)) {
		return Fail(result,HYBRID_MACRO_INVALID_INPUT,"no surfaces or no height sampler");
	}

	const float quantum	= HYBRID_QUANTUM_POSITION;
	const float floor_h	= params.SeaLevel + params.MinElevation;
	const float ceiling	= params.SeaLevel + params.MaxElevation;
	const float tolerance= Max2(quantum,0.01f);
	StringClass detail;

	//	1 -- every river surface is non-increasing source to sink, and no bed is above its own water.
	for (int i = 0; i < surfaces->Rivers.Count(); ++i) {

		const HybridRiverSurface & river = surfaces->Rivers[i];
		for (int point = 1; point < river.Surface.Count(); ++point) {
			if (river.Surface[point] <= (river.Surface[point - 1] + quantum)) continue;
			detail.Format("river %u rises from %.3f to %.3f at sample %d",river.RiverId,
								river.Surface[point - 1],river.Surface[point],point);
			return Fail(result,HYBRID_MACRO_RIVER_NOT_MONOTONIC,detail.Peek_Buffer());
		}
		for (int point = 0; point < river.Bed.Count(); ++point) {
			if (river.Bed[point] <= (river.Surface[point] + quantum)) continue;
			detail.Format("river %u bed %.3f is above its surface %.3f at sample %d",river.RiverId,
								river.Bed[point],river.Surface[point],point);
			return Fail(result,HYBRID_MACRO_BED_ABOVE_SURFACE,detail.Peek_Buffer());
		}
	}

	/*
	**	2 -- the realized ground.  Every water record's own samples are checked against the elevation
	**	envelope and against their water surface, which is the plan's "no lake/river bed protrudes above
	**	its water surface" measured on the heightfield that was actually written.
	*/
	for (int i = 0; i < hydrology.Rivers.Count(); ++i) {

		const HybridRiverSystem & river = hydrology.Rivers[i];
		const HybridRiverSurface * fixed = surfaces->Find_River(river.StableId);
		if (fixed == nullptr) continue;

		for (int point = 0; point < river.Points.Count(); ++point) {

			float height = 0.0f;
			if (!sample(river.Points[point],&height,context)) continue;
			if ((height < (floor_h - tolerance)) || (height > (ceiling + tolerance))) {
				detail.Format("river %u sample %d realized %.3f outside %.3f..%.3f",river.StableId,point,height,
									floor_h,ceiling);
				return Fail(result,HYBRID_MACRO_ELEVATION_LIMIT,detail.Peek_Buffer());
			}
			if (height <= (fixed->Surface[point] + tolerance)) continue;
			detail.Format("river %u bed stands %.3f above its water surface at sample %d",river.StableId,
								height - fixed->Surface[point],point);
			return Fail(result,HYBRID_MACRO_BED_ABOVE_SURFACE,detail.Peek_Buffer());
		}
	}

	for (int i = 0; i < hydrology.Bodies.Count(); ++i) {

		const HybridWaterBody & body = hydrology.Bodies[i];
		if (body.Kind == HYBRID_WATER_BODY_ISLAND) continue;

		// A polygon centroid can lie outside a concave lake, or on a subtracted island.
		// Keep the existing probe when wet; otherwise use the most interior wet analysis sample.
		Vector2 probe = body.Center;
		unsigned int owner = 0;
		if (!Hybrid_Point_In_Standing_Water(hydrology,probe,&owner,nullptr) || (owner != body.StableId)) {
			float best = -1.0f;
			const float spacing = params.Constants.A;
			if (spacing <= 0.0f) return Fail(result,HYBRID_MACRO_INVALID_INPUT,"no water-probe analysis spacing");
			for (float y = body.BoundsMin.Y + (0.5f * spacing); y < body.BoundsMax.Y; y += spacing) {
				for (float x = body.BoundsMin.X + (0.5f * spacing); x < body.BoundsMax.X; x += spacing) {
					const Vector2 candidate(x,y);
					if (!Hybrid_Point_In_Standing_Water(hydrology,candidate,&owner,nullptr) || (owner != body.StableId)) continue;
					float clearance = Distance_To_Ring(body.Points,candidate);
					for (int island = 0; island < hydrology.Bodies.Count(); ++island) {
						const HybridWaterBody & hole = hydrology.Bodies[island];
						if ((hole.Kind == HYBRID_WATER_BODY_ISLAND) && (hole.HostBodyId == body.StableId)) {
							clearance = Min2(clearance,Distance_To_Ring(hole.Points,candidate));
						}
					}
					if (clearance <= best) continue;
					best = clearance;
					probe = candidate;
				}
			}
			if (best < 0.0f) return Fail(result,HYBRID_MACRO_INVALID_INPUT,"water body has no wet analysis probe");
		}

		float height = 0.0f;
		if (!sample(probe,&height,context)) continue;
		if ((height < (floor_h - tolerance)) || (height > (ceiling + tolerance))) {
			detail.Format("%s %u realized %.3f outside %.3f..%.3f",Hybrid_Water_Body_Kind_Name(body.Kind),
								body.StableId,height,floor_h,ceiling);
			return Fail(result,HYBRID_MACRO_ELEVATION_LIMIT,detail.Peek_Buffer());
		}
		if (height <= (body.SurfaceHeight + tolerance)) continue;
		//	The probe, the formula's own answer there and the floor, because the three together say
		//	whether the realization drifted from the expression or the expression hit the envelope.
		detail.Format("%s %u bed stands %.3f above its water surface at (%.3f,%.3f) -- formula %.3f, surface %.3f, floor %.3f",
							Hybrid_Water_Body_Kind_Name(body.Kind),body.StableId,height - body.SurfaceHeight,
							probe.X,probe.Y,
							Hybrid_Macro_Sample_Height(params,matrices,landforms,hydrology,*surfaces,probe),
							body.SurfaceHeight,floor_h);
		return Fail(result,HYBRID_MACRO_BED_ABOVE_SURFACE,detail.Peek_Buffer());
	}

	/*
	**	3 -- every Phase-5 feature is still represented.  The test is the sign of the realized height
	**	against the same point's tier-and-bias reference: a hill or ridge stands above it, a canyon or
	**	ravine floor below it, a cliff's upper shelf above its own toe.  A point the water carved is
	**	skipped -- water outranks relief by the plan's own formula, and the water record itself was
	**	already checked above.
	*/
	const HybridSemanticMatrix & matrix = matrices.Matrix;
	const int middle = Middle_Tier(params.TierCount);

	for (int i = 0; i < landforms.Landforms.Count(); ++i) {

		const HybridLandform & landform = landforms.Landforms[i];
		const Vector2 probe = (landform.Points.Count() > 0)
									 ? landform.Points[landform.Points.Count() / 2] : landform.Center;

		const float contribution = Hybrid_Landform_Contribution(landform,probe,nullptr);
		if ((float)fabs((double)contribution) <= Max2(quantum,0.05f)) continue;

		float height = 0.0f;
		if (!sample(probe,&height,context)) continue;

		const int index = Coarse_Index(matrix,probe);
		const int tier = (index >= 0) ? (int)matrix.BaseElevationTier[index] : middle;
		float reference = params.SeaLevel + ((float)(tier - middle) * params.TierStep);
		if (index >= 0) reference += matrix.MacroHeightBias[index];

		//	Water at the probe, or an elevation clamp at it, makes the sign test meaningless.
		const float base = reference + contribution;
		if (Hybrid_Macro_Water_Carve(params,hydrology,*surfaces,probe,base) > 0.0f) continue;
		if ((base <= floor_h) || (base >= ceiling)) continue;

		const float realized = height - reference;
		const bool represented = (contribution > 0.0f) ? (realized > 0.0f) : (realized < 0.0f);
		if (represented) continue;

		detail.Format("%s %u contributes %.3f but the ground is %.3f from its tier reference",
							Hybrid_Landform_Kind_Name(landform.Kind),landform.StableId,contribution,realized);
		return Fail(result,HYBRID_MACRO_FEATURE_LOST,detail.Peek_Buffer());
	}

	//	4 -- every accepted crossing window is still connectable within PRIMARY_2 grade on real ground.
	const float grade = (float)tan((double)(Hybrid_Road_Class_Spec(HYBRID_ROAD_PRIMARY_2).MaxGradeDegrees *
														  (3.14159265358979323846f / 180.0f)));
	for (int i = 0; i < hydrology.Windows.Count(); ++i) {

		const HybridCrossingWindow & window = hydrology.Windows[i];
		float height_a = 0.0f;
		float height_b = 0.0f;
		if (!sample(window.ApproachA,&height_a,context)) continue;
		if (!sample(window.ApproachB,&height_b,context)) continue;

		const float span = (window.ApproachA - window.ApproachB).Length();
		if (span <= 0.0f) continue;
		if ((float)fabs((double)(height_a - height_b)) <= ((grade * span) + tolerance)) continue;

		detail.Format("window %u approaches realized %.3f apart over %.3f, past PRIMARY_2 grade",
							window.StableId,(float)fabs((double)(height_a - height_b)),span);
		return Fail(result,HYBRID_MACRO_FEATURE_LOST,detail.Peek_Buffer());
	}

	surfaces->Checksum = Hybrid_Macro_Terrain_Checksum(*surfaces,0);
	return true;
}


unsigned int Hybrid_Macro_Terrain_Checksum(const HybridMacroTerrainSet & set,unsigned int seed)
{
	HybridHashStream stream(seed);

	stream.Add_Int(set.VertexCountX);
	stream.Add_Int(set.VertexCountY);
	stream.Add_Length(set.CellSize);
	stream.Add_Length(set.MinHeight);
	stream.Add_Length(set.MaxHeight);
	stream.Add_Int(set.ProtectedVertices);
	stream.Add_Int(set.Rivers.Count());

	for (int i = 0; i < set.Rivers.Count(); ++i) {

		const HybridRiverSurface & river = set.Rivers[i];
		stream.Add_Uint(river.RiverId);
		stream.Add_Uint(river.SinkBodyId);
		stream.Add_Length(river.MouthSurface);
		stream.Add_Int(river.Surface.Count());
		for (int point = 0; point < river.Surface.Count(); ++point) {
			stream.Add_Length(river.Surface[point]);
			stream.Add_Length(river.Bed[point]);
		}
	}

	return stream.Get();
}
