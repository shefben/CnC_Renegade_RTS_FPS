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
 *                     $Archive:: /Commando/Code/wwphys/hybridhydrology.cpp                   $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Runtime Phase 6, the vector hydrology.  See hybridhydrology.h for what this phase owns and   *
 * why; the plan's twenty-one steps appear below in its own order, each one named where it       *
 * starts, so the sequence can be read against Section 8 Phase 6 line by line.                  *
 *                                                                                             *
 * The drainage solve is deliberately the plan's exact one -- Priority-Flood from the exterior,  *
 * then D8 with a fixed tie order over the *filled* surface -- because that combination has no   *
 * flat-area ambiguity for an implementer to resolve: every equal-height drain parent already    *
 * has a lower flood rank, so the flow field cannot contain a cycle and no flat solver is        *
 * chosen here.                                                                                 *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#include "hybridhydrology.h"

#include "wwdebug.h"
#include "wwmath.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>


static inline float Max2(float a,float b)	{ return (a > b) ? a : b; }
static inline float Min2(float a,float b)	{ return (a < b) ? a : b; }
static inline float Clamp2(float v,float lo,float hi)	{ return (v < lo) ? lo : ((v > hi) ? hi : v); }

//	The one smoothstep this phase uses; Section 8 Phase 6/7 name it for every blend band.
static inline float Smooth(float t)
{
	const float s = Clamp2(t,0.0f,1.0f);
	return s * s * (3.0f - (2.0f * s));
}


/*
**	Section 8 Phase 6's own constants, each used exactly once below.  Multipliers of W are lane
**	widths, multipliers of D are normalized map extent, multipliers of A are analysis spacings.
*/
static const float	SOURCE_SEPARATION_D			= 0.18f;		// step 11 D8 source separation
static const float	THRESHOLD_REDUCTION			= 0.80f;		// step 11 threshold multiplier
static const int		THRESHOLD_REDUCTION_LIMIT	= 3;			// step 11 at most three reductions
static const float	WIDTH_ACCUMULATION_SPAN		= 3.0f;		// Section 11 width ramp, 3*threshold
static const float	GUIDED_SINK_MULTIPLIER		= 4.0f;		// step 10 synthetic accumulation at the sink
static const float	LAKE_SHRINK_FRACTION			= 0.10f;		// step 14 guide polygon shrink per retry
static const float	SIMPLIFY_AREA_TOLERANCE		= 0.05f;		// step 13 simplification area budget
static const int		GUIDE_POLYGON_RADIALS		= 8;			// steps 14/17 eight-radial polygons
static const float	ISLAND_RADIUS_MIN				= 0.85f;		// step 17 radius multipliers
static const float	ISLAND_RADIUS_MAX				= 1.15f;
static const float	ISLAND_SHORE_CLEARANCE_W	= 6.0f;		// step 17 distance from the host shoreline
static const float	ISLAND_BLEND_W					= 2.0f;		// step 17 shoreline blend band
static const float	ISLAND_HOST_AREA_FACTOR		= 4.0f;		// step 17 minimum host water area factor
static const float	ISLAND_HOST_AREA_W			= 16.0f;		// step 17 (16W)^2 term
static const float	ISLAND_HOST_AREA_FRACTION	= 0.01f;		// step 17 0.01 * world area term
static const float	LAKE_AREA_A						= 4.0f;		// Section 5 true lake, 4*A*A
static const float	LAKE_AREA_FRACTION			= 0.002f;	// Section 5 true lake, 0.002 * world area
static const float	BASE_BARRIER_TIERS			= 4.0f;		// step 2 +4*tier_step on BASE_PROTECTED
static const float	APPROACH_LENGTH_W				= 8.0f;		// step 18 8W approach on both sides
static const float	WINDOW_MERGE_W					= 4.0f;		// step 19 merge distance
static const float	OBLIGATION_PREFERENCE_S		= 2.0f;		// step 18 within 2S of a crossing guide
static const float	RIVER_BANK_BLEND_W			= 2.0f;		// Phase 7 bank blend, max(river_width,2W)


/*
**	Step 4's neighbor tie order, `E,NE,N,NW,W,SW,S,SE`.  Priority-Flood discovers neighbors in the
**	same order for the same reason: the order is part of the deterministic record, not a detail.
*/
static const int	NEIGHBOR_DX[8] = {  1, 1, 0,-1,-1,-1, 0, 1 };
static const int	NEIGHBOR_DY[8] = {  0, 1, 1, 1, 0,-1,-1,-1 };


/***********************************************************************************************
**	Names, Section-5 quotas and the Section-11 formulas.
***********************************************************************************************/

const char * Hybrid_Water_Body_Kind_Name(HybridWaterBodyKind kind)
{
	switch (kind) {
		case HYBRID_WATER_BODY_LAKE:		return "lake";
		case HYBRID_WATER_BODY_COAST:		return "coast";
		case HYBRID_WATER_BODY_ISLAND:	return "island";
		default:									break;
	}
	return "unknown";
}


const char * Hybrid_Window_Kind_Name(HybridWindowKind kind)
{
	switch (kind) {
		case HYBRID_WINDOW_CROSSING:		return "crossing_window";
		case HYBRID_WINDOW_RAMP_PASS:		return "ramp_pass_window";
		default:									break;
	}
	return "unknown";
}


const char * Hybrid_Hydrology_Failure_Name(HybridHydrologyFailure failure)
{
	switch (failure) {
		case HYBRID_HYDROLOGY_OK:						return "ok";
		case HYBRID_HYDROLOGY_INVALID_INPUT:		return "invalid_input";
		case HYBRID_HYDROLOGY_SCRATCH_EXHAUSTED:	return "scratch_exhausted";
		case HYBRID_HYDROLOGY_RIVER_QUOTA:			return "river_quota";
		case HYBRID_HYDROLOGY_LAKE_QUOTA:			return "lake_quota";
		case HYBRID_HYDROLOGY_COAST_UNPLACEABLE:	return "coast_unplaceable";
		case HYBRID_HYDROLOGY_FLOW_NOT_ACYCLIC:	return "flow_not_acyclic";
		case HYBRID_HYDROLOGY_WATER_IN_RESERVE:	return "water_in_reserve";
		case HYBRID_HYDROLOGY_CORRIDOR_BLOCKED:	return "corridor_blocked";
		case HYBRID_HYDROLOGY_NO_CROSSING_WINDOW:	return "no_crossing_window";
		case HYBRID_HYDROLOGY_RECORD_INCOMPLETE:	return "record_incomplete";
		default:												break;
	}
	return "unknown";
}


static int Size_Pick(HybridSizeClass size_class,int small,int medium,int large)
{
	switch (size_class) {
		case HYBRID_SIZE_SMALL:		return small;
		case HYBRID_SIZE_MEDIUM:	return medium;
		case HYBRID_SIZE_LARGE:		return large;
		default:							break;
	}
	return medium;
}


int Hybrid_Required_River_Count(HybridSizeClass size_class)
{
	return Size_Pick(size_class,1,1,2);
}


int Hybrid_Required_Lake_Count(HybridSizeClass size_class)
{
	return Size_Pick(size_class,1,1,2);
}


float Hybrid_True_Lake_Min_Area(float a,float world_area)
{
	return Max2(LAKE_AREA_A * a * a,LAKE_AREA_FRACTION * world_area);
}


float Hybrid_Island_Min_Area(float w,float world_area)
{
	const float side = ISLAND_HOST_AREA_W * w;
	return Max2(side * side,ISLAND_HOST_AREA_FRACTION * world_area);
}


float Hybrid_River_Threshold(float river_accumulation_chunks,float chunk_size,float a)
{
	if ((a <= 0.0f) || (chunk_size <= 0.0f)) return 1.0f;
	const float cells_per_chunk = chunk_size / a;
	const float raw = Max2(0.0f,river_accumulation_chunks) * cells_per_chunk * cells_per_chunk;
	return Max2(1.0f,Hybrid_Round_Half_Up(raw));
}


float Hybrid_River_Width(float accumulation,float threshold,float stream_width,float river_width)
{
	if (threshold <= 0.0f) return stream_width;
	const float t = Clamp2((accumulation - threshold) / (WIDTH_ACCUMULATION_SPAN * threshold),0.0f,1.0f);
	return stream_width + ((river_width - stream_width) * t);
}


/***********************************************************************************************
**	HybridHydrologySet and HybridHydrologyParams.
***********************************************************************************************/

void HybridHydrologySet::Reset(void)
{
	Rivers.Delete_All();
	Bodies.Delete_All();
	Windows.Delete_All();
	Hash.Reset();

	RequiredRivers			= 0;
	RequiredLakes			= 0;
	RiverThreshold			= 0.0f;
	ThresholdReductions	= 0;
	StandingWaterArea		= 0.0f;
	StandingWaterFraction= 0.0f;
	WaterFractionTarget	= 0.0f;
	IslandSlots				= 0;
	IslandsRejected		= 0;
	OptionalLakes			= 0;
	StationsConsidered	= 0;
	for (int refusal = 0; refusal < HYBRID_LOCAL_PASS_FAILURE_COUNT; ++refusal) PassRefusals[refusal] = 0;
	StationsMerged			= 0;
	LocalRetries			= 0;
	Checksum					= 0;
}


const HybridRiverSystem * HybridHydrologySet::Find_River(unsigned int stable_id) const
{
	for (int i = 0; i < Rivers.Count(); ++i) {
		if (Rivers[i].StableId == stable_id) return &Rivers[i];
	}
	return nullptr;
}


const HybridWaterBody * HybridHydrologySet::Find_Body(unsigned int stable_id) const
{
	for (int i = 0; i < Bodies.Count(); ++i) {
		if (Bodies[i].StableId == stable_id) return &Bodies[i];
	}
	return nullptr;
}


const HybridCrossingWindow * HybridHydrologySet::Find_Window(unsigned int stable_id) const
{
	for (int i = 0; i < Windows.Count(); ++i) {
		if (Windows[i].StableId == stable_id) return &Windows[i];
	}
	return nullptr;
}


int HybridHydrologySet::Count_Of_Body_Kind(HybridWaterBodyKind kind) const
{
	int count = 0;
	for (int i = 0; i < Bodies.Count(); ++i) {
		if (Bodies[i].Kind == kind) count++;
	}
	return count;
}


int HybridHydrologySet::Count_Of_Window_Kind(HybridWindowKind kind) const
{
	int count = 0;
	for (int i = 0; i < Windows.Count(); ++i) {
		if (Windows[i].Kind == kind) count++;
	}
	return count;
}


int HybridHydrologySet::Count_Of_True_Lakes(float minimum_area) const
{
	int count = 0;
	for (int i = 0; i < Bodies.Count(); ++i) {
		if (Bodies[i].Kind != HYBRID_WATER_BODY_LAKE) continue;
		if (Bodies[i].Area + 0.0005f < minimum_area) continue;
		count++;
	}
	return count;
}


HybridHydrologyParams::HybridHydrologyParams(void) :
	HydrologySeed(0),
	WorldMinX(0.0f),WorldMinY(0.0f),WorldWidth(0.0f),WorldHeight(0.0f),
	SeaLevel(0.0f),MinElevation(-30.0f),MaxElevation(90.0f),TierStep(18.0f),TierCount(3),MaxCutDepth(6.0f),MaxFillHeight(6.0f),
	ChunkSize(30.0f),
	Rivers(true),Lakes(true),Coastline(true),
	RiverWidth(6.0f),StreamWidth(2.5f),RiverAccumulationChunks(6.0f),MaxRiverCount(4),
	WaterFraction(0.08f),
	RiverBedDepth(2.5f),LakeBedDepth(3.0f),
	MaxLocalRetriesPerStage(4),MaxAlternateCandidatesPerFeature(8),
	MinVehicleLaneClearance(1.0f),
	HeavyVehicleWidth(0.0f),HeavyVehicleHeight(0.0f),HarvesterWidth(0.0f),
	BridgeMaxSpanLength(0.0f),BridgeRequiredWidth(0.0f),BridgeApproachLength(0.0f),
	BridgeWaterClearanceMargin(0.0f),
	TunnelsAvailable(false),TunnelRequiredWidth(0.0f),TunnelRequiredHeight(0.0f),
	AllowFords(false),FordsAvailable(false),FordMaxWaterDepthLight(0.0f),FordMaxWaterDepthHarvester(0.0f),
	Arena(nullptr),Counters(nullptr)
{
	memset(&Constants,0,sizeof(Constants));
}


/***********************************************************************************************
**	Shared polygon/polyline geometry.  Every ring in this file is a closed simple ring stored
**	without a repeated final point, and every area is the shoelace area of that ring, so one
**	definition of "the polygon's area" serves the quota, the simplification budget and the
**	water-fraction accounting.
***********************************************************************************************/

static float Ring_Area(const DynamicVectorClass<Vector2> & ring)
{
	const int count = ring.Count();
	if (count < 3) return 0.0f;

	float twice = 0.0f;
	for (int i = 0; i < count; ++i) {
		const Vector2 & a = ring[i];
		const Vector2 & b = ring[(i + 1) % count];
		twice += (a.X * b.Y) - (b.X * a.Y);
	}
	return (float)fabs((double)twice) * 0.5f;
}


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


static float Distance_To_Segment(const Vector2 & point,const Vector2 & a,const Vector2 & b,float * t_out)
{
	const Vector2 axis = b - a;
	const float length_sq = (axis.X * axis.X) + (axis.Y * axis.Y);
	float t = 0.0f;
	if (length_sq > 0.0f) {
		t = (((point.X - a.X) * axis.X) + ((point.Y - a.Y) * axis.Y)) / length_sq;
		t = Clamp2(t,0.0f,1.0f);
	}
	if (t_out != nullptr) *t_out = t;
	const Vector2 nearest(a.X + (axis.X * t),a.Y + (axis.Y * t));
	const float dx = point.X - nearest.X;
	const float dy = point.Y - nearest.Y;
	return (float)sqrt((double)((dx * dx) + (dy * dy)));
}


static float Distance_To_Ring(const DynamicVectorClass<Vector2> & ring,const Vector2 & point)
{
	const int count = ring.Count();
	if (count == 0) return 0.0f;
	if (count == 1) return (point - ring[0]).Length();

	float best = 3.4e38f;
	for (int i = 0; i < count; ++i) {
		const float distance = Distance_To_Segment(point,ring[i],ring[(i + 1) % count],nullptr);
		best = Min2(best,distance);
	}
	return best;
}


/*
**	Distance from `point` to an open polyline, with the along-polyline arc length of the nearest
**	point when the caller wants it.  `stations` may be null, in which case the arc length is
**	measured from the polyline itself.
*/
static float Distance_To_Polyline(const DynamicVectorClass<Vector2> & points,const Vector2 & point,
											float * station_out,int * index_out)
{
	const int count = points.Count();
	if (count == 0) {
		if (station_out != nullptr) *station_out = 0.0f;
		if (index_out != nullptr) *index_out = -1;
		return 3.4e38f;
	}
	if (count == 1) {
		if (station_out != nullptr) *station_out = 0.0f;
		if (index_out != nullptr) *index_out = 0;
		return (point - points[0]).Length();
	}

	float best			= 3.4e38f;
	float best_station= 0.0f;
	int	best_index	= 0;
	float travelled	= 0.0f;

	for (int i = 0; i < (count - 1); ++i) {
		float t = 0.0f;
		const float distance = Distance_To_Segment(point,points[i],points[i + 1],&t);
		const float segment = (points[i + 1] - points[i]).Length();
		if (distance < best) {
			best			= distance;
			best_station= travelled + (segment * t);
			best_index	= (t <= 0.5f) ? i : (i + 1);
		}
		travelled += segment;
	}

	if (station_out != nullptr) *station_out = best_station;
	if (index_out != nullptr) *index_out = best_index;
	return best;
}


/*
**	Douglas-Peucker over an open point range, appending every retained point except the last.
*/
static void Simplify_Open_Range(const DynamicVectorClass<Vector2> & points,int first,int last,
											float tolerance,DynamicVectorClass<Vector2> * out)
{
	if (last <= first) return;

	float worst		= -1.0f;
	int	worst_at	= -1;
	for (int i = first + 1; i < last; ++i) {
		const float distance = Distance_To_Segment(points[i],points[first],points[last],nullptr);
		if (distance > worst) {
			worst		= distance;
			worst_at	= i;
		}
	}

	if ((worst_at < 0) || (worst <= tolerance)) {
		out->Add(points[first]);
		return;
	}
	Simplify_Open_Range(points,first,worst_at,tolerance,out);
	Simplify_Open_Range(points,worst_at,last,tolerance,out);
}


/*
**	Step 13's ring simplification at `A/2`.  A closed ring is split at its two most distant vertices
**	so the result does not depend on which vertex happened to be stored first, then each half is
**	simplified as an open range.  The caller applies the 5% area budget.
*/
static void Simplify_Ring(const DynamicVectorClass<Vector2> & ring,float tolerance,
									DynamicVectorClass<Vector2> * out)
{
	out->Delete_All();
	const int count = ring.Count();
	if (count < 4) {
		for (int i = 0; i < count; ++i) out->Add(ring[i]);
		return;
	}

	int	split_a	= 0;
	int	split_b	= 0;
	float best	= -1.0f;
	for (int i = 0; i < count; ++i) {
		for (int j = i + 1; j < count; ++j) {
			const float distance = (ring[j] - ring[i]).Length();
			if (distance > best) {
				best		= distance;
				split_a	= i;
				split_b	= j;
			}
		}
	}

	DynamicVectorClass<Vector2> half;
	for (int i = split_a; i <= split_b; ++i) half.Add(ring[i]);
	Simplify_Open_Range(half,0,half.Count() - 1,tolerance,out);
	out->Add(ring[split_b]);

	half.Delete_All();
	for (int i = split_b; i < (count + split_a); ++i) half.Add(ring[i % count]);
	half.Add(ring[split_a]);
	DynamicVectorClass<Vector2> second;
	Simplify_Open_Range(half,0,half.Count() - 1,tolerance,&second);
	for (int i = 1; i < second.Count(); ++i) out->Add(second[i]);
}


/*
**	Clip a convex ring against the half plane `dot(normal,p) <= offset`.  Step 15's coast polygon is
**	the world rectangle clipped this way, which keeps the sea a convex ring inside the world bounds
**	instead of an unbounded strategic-space quad.
*/
static void Clip_Convex_Ring(const DynamicVectorClass<Vector2> & ring,const Vector2 & normal,float offset,
										DynamicVectorClass<Vector2> * out)
{
	out->Delete_All();
	const int count = ring.Count();
	if (count < 3) return;

	for (int i = 0; i < count; ++i) {

		const Vector2 & a = ring[i];
		const Vector2 & b = ring[(i + 1) % count];
		const float da = ((normal.X * a.X) + (normal.Y * a.Y)) - offset;
		const float db = ((normal.X * b.X) + (normal.Y * b.Y)) - offset;

		if (da <= 0.0f) out->Add(a);
		if (((da < 0.0f) && (db > 0.0f)) || ((da > 0.0f) && (db < 0.0f))) {
			const float t = da / (da - db);
			out->Add(Vector2(Hybrid_Quantize_Position(a.X + ((b.X - a.X) * t)),
								  Hybrid_Quantize_Position(a.Y + ((b.Y - a.Y) * t))));
		}
	}
}


static void Ring_Bounds(const DynamicVectorClass<Vector2> & ring,Vector2 * min_out,Vector2 * max_out)
{
	if (ring.Count() == 0) {
		min_out->Set(0.0f,0.0f);
		max_out->Set(0.0f,0.0f);
		return;
	}
	*min_out = ring[0];
	*max_out = ring[0];
	for (int i = 1; i < ring.Count(); ++i) {
		min_out->X = Min2(min_out->X,ring[i].X);
		min_out->Y = Min2(min_out->Y,ring[i].Y);
		max_out->X = Max2(max_out->X,ring[i].X);
		max_out->Y = Max2(max_out->Y,ring[i].Y);
	}
}


static Vector2 Ring_Centroid(const DynamicVectorClass<Vector2> & ring)
{
	const int count = ring.Count();
	if (count == 0) return Vector2(0.0f,0.0f);

	Vector2 sum(0.0f,0.0f);
	for (int i = 0; i < count; ++i) sum += ring[i];
	return Vector2(sum.X / (float)count,sum.Y / (float)count);
}


/***********************************************************************************************
**	The public evaluators Phase 7 and every later validator read instead of re-deriving.
***********************************************************************************************/

/*
**	Step 1's provisional surface: `tier_height + landform`, with no detail noise, no macro bias and
**	no `+4*tier_step` base barrier -- the barrier is a drainage-analysis device, never a height.
*/
float Hybrid_Provisional_Height(const HybridHydrologyParams & params,const HybridCoarseMatrices & matrices,
											const HybridLandformSet & landforms,const Vector2 & world)
{
	const HybridSemanticMatrix & matrix = matrices.Matrix;
	const int middle = (params.TierCount > 0) ? ((params.TierCount - 1) / 2) : 0;

	int cx = 0, cy = 0;
	if (!matrix.World_To_Cell(world,&cx,&cy)) {
		//	Outside the matrix: clamp to the nearest cell, which is the boundary band's own tier.
		const float cell = matrix.Get_Cell_Size();
		if (cell <= 0.0f) return params.SeaLevel;
		cx = (int)((world.X - matrix.Get_Origin_X()) / cell);
		cy = (int)((world.Y - matrix.Get_Origin_Y()) / cell);
		if (cx < 0) cx = 0;
		if (cy < 0) cy = 0;
		if (cx > (matrix.Get_Count_X() - 1)) cx = matrix.Get_Count_X() - 1;
		if (cy > (matrix.Get_Count_Y() - 1)) cy = matrix.Get_Count_Y() - 1;
	}

	const int tier = (int)matrix.BaseElevationTier[matrix.Index(cx,cy)];
	const float tier_height = params.SeaLevel + ((float)(tier - middle) * params.TierStep);
	return tier_height + Hybrid_Landform_Height(landforms,world,params.Constants.A);
}


bool Hybrid_Point_In_Standing_Water(const HybridHydrologySet & set,const Vector2 & world,
												  unsigned int * body_out,float * surface_out)
{
	if (body_out != nullptr) *body_out = 0;
	if (surface_out != nullptr) *surface_out = 0.0f;

	//	An island is dry land inside its host, so it answers first: step 17's subtraction.
	for (int i = 0; i < set.Bodies.Count(); ++i) {
		const HybridWaterBody & body = set.Bodies[i];
		if (body.Kind != HYBRID_WATER_BODY_ISLAND) continue;
		if ((world.X < body.BoundsMin.X) || (world.X > body.BoundsMax.X) ||
			 (world.Y < body.BoundsMin.Y) || (world.Y > body.BoundsMax.Y)) continue;
		if (Point_In_Ring(body.Points,world)) return false;
	}

	for (int i = 0; i < set.Bodies.Count(); ++i) {
		const HybridWaterBody & body = set.Bodies[i];
		if (body.Kind == HYBRID_WATER_BODY_ISLAND) continue;
		if ((world.X < body.BoundsMin.X) || (world.X > body.BoundsMax.X) ||
			 (world.Y < body.BoundsMin.Y) || (world.Y > body.BoundsMax.Y)) continue;
		if (!Point_In_Ring(body.Points,world)) continue;
		if (body_out != nullptr) *body_out = body.StableId;
		if (surface_out != nullptr) *surface_out = body.SurfaceHeight;
		return true;
	}
	return false;
}


bool Hybrid_Point_In_Water(const HybridHydrologySet & set,const Vector2 & world)
{
	if (Hybrid_Point_In_Standing_Water(set,world,nullptr,nullptr)) return true;

	for (int i = 0; i < set.Rivers.Count(); ++i) {

		const HybridRiverSystem & river = set.Rivers[i];
		if ((world.X < river.BoundsMin.X) || (world.X > river.BoundsMax.X) ||
			 (world.Y < river.BoundsMin.Y) || (world.Y > river.BoundsMax.Y)) continue;

		int index = -1;
		const float distance = Distance_To_Polyline(river.Points,world,nullptr,&index);
		if (distance <= (0.5f * river.Width_At_Index(index))) return true;
	}
	return false;
}


float Hybrid_Island_Height_Bias(const HybridHydrologySet & set,const Vector2 & world)
{
	float bias = 0.0f;

	for (int i = 0; i < set.Bodies.Count(); ++i) {

		const HybridWaterBody & body = set.Bodies[i];
		if (body.Kind != HYBRID_WATER_BODY_ISLAND) continue;
		if (body.HeightBias == 0.0f) continue;

		const float blend = Max2(body.ShoreBlend,0.0f);
		if ((world.X < (body.BoundsMin.X - blend)) || (world.X > (body.BoundsMax.X + blend)) ||
			 (world.Y < (body.BoundsMin.Y - blend)) || (world.Y > (body.BoundsMax.Y + blend))) continue;

		if (Point_In_Ring(body.Points,world)) {
			bias = Max2(bias,body.HeightBias);
			continue;
		}
		if (blend <= 0.0f) continue;

		const float distance = Distance_To_Ring(body.Points,world);
		if (distance >= blend) continue;
		bias = Max2(bias,body.HeightBias * (1.0f - Smooth(distance / blend)));
	}
	return bias;
}


float Hybrid_Water_Carve_Depth(const HybridHydrologySet & set,const Vector2 & world,float lane_width,
											float river_width)
{
	//	Inside an island nothing is carved at all -- step 17 suppresses the host's carve there.
	for (int i = 0; i < set.Bodies.Count(); ++i) {
		const HybridWaterBody & body = set.Bodies[i];
		if (body.Kind != HYBRID_WATER_BODY_ISLAND) continue;
		if ((world.X < body.BoundsMin.X) || (world.X > body.BoundsMax.X) ||
			 (world.Y < body.BoundsMin.Y) || (world.Y > body.BoundsMax.Y)) continue;
		if (Point_In_Ring(body.Points,world)) return 0.0f;
	}

	float carve = 0.0f;

	for (int i = 0; i < set.Bodies.Count(); ++i) {
		const HybridWaterBody & body = set.Bodies[i];
		if (body.Kind == HYBRID_WATER_BODY_ISLAND) continue;
		if (body.BedDepth <= 0.0f) continue;
		if ((world.X < body.BoundsMin.X) || (world.X > body.BoundsMax.X) ||
			 (world.Y < body.BoundsMin.Y) || (world.Y > body.BoundsMax.Y)) continue;
		if (Point_In_Ring(body.Points,world)) carve = Max2(carve,body.BedDepth);
	}

	const float bank = Max2(river_width,RIVER_BANK_BLEND_W * lane_width);

	for (int i = 0; i < set.Rivers.Count(); ++i) {

		const HybridRiverSystem & river = set.Rivers[i];
		if (river.BedDepth <= 0.0f) continue;
		if ((world.X < (river.BoundsMin.X - bank)) || (world.X > (river.BoundsMax.X + bank)) ||
			 (world.Y < (river.BoundsMin.Y - bank)) || (world.Y > (river.BoundsMax.Y + bank))) continue;

		int index = -1;
		const float distance = Distance_To_Polyline(river.Points,world,nullptr,&index);
		const float half = 0.5f * river.Width_At_Index(index);

		if (distance <= half) {
			carve = Max2(carve,river.BedDepth);
			continue;
		}
		if (distance >= (half + bank)) continue;
		carve = Max2(carve,river.BedDepth * (1.0f - Smooth((distance - half) / bank)));
	}

	return carve;
}


unsigned int Hybrid_Hydrology_Checksum(const HybridHydrologySet & set,unsigned int seed)
{
	HybridHashStream stream(seed);

	stream.Add_Int(set.RequiredRivers);
	stream.Add_Int(set.RequiredLakes);
	stream.Add_Scalar(set.RiverThreshold);
	stream.Add_Int(set.ThresholdReductions);
	stream.Add_Int(set.Rivers.Count());
	stream.Add_Int(set.Bodies.Count());
	stream.Add_Int(set.Windows.Count());

	for (int i = 0; i < set.Rivers.Count(); ++i) {

		const HybridRiverSystem & river = set.Rivers[i];
		stream.Add_Uint(river.StableId);
		stream.Add_Uint(river.IntentStableId);
		stream.Add_Bool(river.Guided);
		stream.Add_Bool(river.Smoothed);
		stream.Add_Int(river.SelectionRank);
		stream.Add_Uint(river.SinkBodyId);
		stream.Add_Scalar(river.MaxAccumulation);
		stream.Add_Length(river.Length);
		stream.Add_Length(river.BedDepth);
		stream.Add_Int(river.Points.Count());
		for (int p = 0; p < river.Points.Count(); ++p) {
			stream.Add_Length(river.Points[p].X);
			stream.Add_Length(river.Points[p].Y);
			stream.Add_Length(river.Widths[p]);
			stream.Add_Length(river.Surfaces[p]);
		}
	}

	for (int i = 0; i < set.Bodies.Count(); ++i) {

		const HybridWaterBody & body = set.Bodies[i];
		stream.Add_Uint(body.StableId);
		stream.Add_Uint((unsigned int)body.Kind);
		stream.Add_Uint(body.IntentStableId);
		stream.Add_Uint(body.HostBodyId);
		stream.Add_Bool(body.Required);
		stream.Add_Bool(body.FromGuidePolygon);
		stream.Add_Bool(body.Simplified);
		stream.Add_Length(body.SurfaceHeight);
		stream.Add_Length(body.BedDepth);
		stream.Add_Length(body.HeightBias);
		stream.Add_Length(body.ShoreBlend);
		stream.Add_Int(body.Points.Count());
		for (int p = 0; p < body.Points.Count(); ++p) {
			stream.Add_Length(body.Points[p].X);
			stream.Add_Length(body.Points[p].Y);
		}
	}

	for (int i = 0; i < set.Windows.Count(); ++i) {

		const HybridCrossingWindow & window = set.Windows[i];
		stream.Add_Uint(window.StableId);
		stream.Add_Uint((unsigned int)window.Kind);
		stream.Add_Uint((unsigned int)window.Obstruction);
		stream.Add_Uint(window.ObstructionRecordId);
		stream.Add_Uint(window.Methods);
		stream.Add_Bool(window.Preferred);
		stream.Add_Int(window.MergedCount);
		stream.Add_Length(window.Station);
		stream.Add_Length(window.Span);
		stream.Add_Length(window.ApproachLength);
		stream.Add_Length(window.LowElevation);
		stream.Add_Length(window.HighElevation);
		stream.Add_Length(window.Center.X);
		stream.Add_Length(window.Center.Y);
		stream.Add_Length(window.ApproachA.X);
		stream.Add_Length(window.ApproachA.Y);
		stream.Add_Length(window.ApproachB.X);
		stream.Add_Length(window.ApproachB.Y);

		//	The frozen local pass: Phase 11 consumes it, so it is part of what the world is.
		stream.Add_Int((int)window.Pass.Kind);
		stream.Add_Int(window.Pass.Orientation);
		stream.Add_Int(window.Pass.ControlCount);
		stream.Add_Length(window.Pass.TangentRun);
		for (int i = 0; i < window.Pass.ControlCount; ++i) {
			stream.Add_Length(window.Pass.Controls[i].X);
			stream.Add_Length(window.Pass.Controls[i].Y);
		}
		stream.Add_Bool(window.Pass.HasFloor);
		stream.Add_Length(window.Pass.FloorEntry.X);
		stream.Add_Length(window.Pass.FloorEntry.Y);
		stream.Add_Length(window.Pass.FloorExit.X);
		stream.Add_Length(window.Pass.FloorExit.Y);
	}

	return stream.Get();
}


/***********************************************************************************************
**	The builder: the analysis grid, the drainage solve and the basin records.
***********************************************************************************************/

/*
**	Step 5's processing order: descending `(filled_height,flood_rank)`.  The key is the quantized
**	filled height as an integer, so the order never depends on a float comparison, and the cell
**	index closes the order so it is total.
*/
struct HydroOrderEntry
{
	int	Key;
	int	Rank;
	int	Cell;
};


static int Order_Compare(const void * a,const void * b)
{
	const HydroOrderEntry * left = (const HydroOrderEntry *)a;
	const HydroOrderEntry * right = (const HydroOrderEntry *)b;
	if (left->Key > right->Key) return -1;
	if (left->Key < right->Key) return 1;
	if (left->Rank > right->Rank) return -1;
	if (left->Rank < right->Rank) return 1;
	if (left->Cell < right->Cell) return -1;
	if (left->Cell > right->Cell) return 1;
	return 0;
}


/*
**	One retained Priority-Flood depression: the pre-fill basin labels step 3 keeps, with the closed
**	ring step 13 judges and the spill level its water would stand at.
*/
struct HydroBasin
{
	HydroBasin(void) :
		Label(0),Cells(0),LowCell(-1),Area(0.0f),RingArea(0.0f),SpillHeight(0.0f),FloorHeight(0.0f),
		Simple(true),HasHoles(false),Center(0.0f,0.0f),BoundsMin(0.0f,0.0f),BoundsMax(0.0f,0.0f) { }

	bool	operator == (const HydroBasin & that) const	{ return Label == that.Label; }
	bool	operator != (const HydroBasin & that) const	{ return !(*this == that); }

	int		Label;
	int		Cells;
	int		LowCell;				// the lowest-index cell, which is where the outline walk starts
	float		Area;					// cell area, Cells * A * A
	float		RingArea;			// shoelace area of Ring
	float		SpillHeight;		// the filled level this depression drains over
	float		FloorHeight;		// lowest provisional height inside it
	bool		Simple;				// the outline closed without touching itself
	bool		HasHoles;			// dry islands inside the depression, which the ring folds in
	Vector2	Center;
	Vector2	BoundsMin;
	Vector2	BoundsMax;

	DynamicVectorClass<Vector2>	Ring;
};


/*
**	One lattice edge of a basin outline, in corner-index space.  Cell (x,y) owns corners (x,y) to
**	(x+1,y+1), so a corner index runs 0..CountX inclusive.
*/
struct HydroOutlineEdge
{
	int	X0;
	int	Y0;
	int	X1;
	int	Y1;
	bool	Used;

	bool	operator == (const HydroOutlineEdge & that) const
	{
		return (X0 == that.X0) && (Y0 == that.Y0) && (X1 == that.X1) && (Y1 == that.Y1);
	}
	bool	operator != (const HydroOutlineEdge & that) const	{ return !(*this == that); }
};


struct HydrologyBuilder
{
	const HybridHydrologyParams *		P;
	const HybridMapIdentity *			Identity;
	const HybridArchetypeIntentSet *	Intents;
	const HybridStrategicGraph *		Graph;
	const HybridLandformSet *			Landforms;
	HybridCoarseMatrices *				Matrices;
	HybridHydrologySet *					Out;

	//	The analysis grid, at spacing A over the world rectangle.
	int										CountX;
	int										CountY;
	int										CellCount;
	float										A;
	float										OriginX;
	float										OriginY;

	//	The drainage arrays, carved out of one block.
	float *									Height;			// provisional surface, with the base barrier applied
	float *									Filled;
	int *										Key;				// quantized Filled, the priority-queue key
	int *										Parent;			// drain_parent, -1 for an exterior seed
	int *										Rank;				// flood_rank
	int *										Basin;			// pre-fill depression label, -1 when none
	int *										Dest;				// D8 destination cell, -1 for a sink
	float *									Accum;
	unsigned char *						Channel;
	unsigned char *						Visited;
	int *										Heap;
	int										HeapCount;
	HydroOrderEntry *						Order;
	int *										Group;			// union-find over channel cells
	int *										Corner0;			// outline chaining: first edge starting at a corner
	int *										Corner1;			// ...and the second, which is a pinch
	DynamicVectorClass<int>				Block;			// owner when there is no attempt arena

	DynamicVectorClass<HydroBasin>	Basins;

	unsigned int							NextStableId;
	float										WorldArea;
	float										TrueLakeMinArea;
	float										Threshold;

	//	Why the last candidate was refused, so an exhausted stage says which rule did it.
	const char *							LastReject;

	int	Cell_X(int cell) const					{ return cell % CountX; }
	int	Cell_Y(int cell) const					{ return cell / CountX; }
	int	Cell_Index(int x,int y) const			{ return (y * CountX) + x; }
	bool	Valid_Cell(int x,int y) const			{ return (x >= 0) && (y >= 0) && (x < CountX) && (y < CountY); }

	Vector2	Cell_Center(int cell) const
	{
		return Vector2(OriginX + (((float)Cell_X(cell) + 0.5f) * A),
							OriginY + (((float)Cell_Y(cell) + 0.5f) * A));
	}

	bool	Is_Exterior(int cell) const
	{
		const int x = Cell_X(cell);
		const int y = Cell_Y(cell);
		return (x == 0) || (y == 0) || (x == (CountX - 1)) || (y == (CountY - 1));
	}

	//	The coarse cell a world point falls in, clamped, which is how every semantic query is made.
	bool	Coarse_Cell(const Vector2 & world,int * cx,int * cy) const
	{
		return Matrices->Matrix.World_To_Cell(world,cx,cy);
	}
};


//	Defined with the water-legality rules below; the river selection needs it first.
static bool Point_In_Tiberium_Reserve(const HydrologyBuilder & b,const Vector2 & world);


static bool Fail(HybridHydrologyResult * result,HybridHydrologyFailure failure,const char * detail)
{
	if (result != nullptr) {
		result->Failure	= failure;
		result->Detail		= (detail != nullptr) ? detail : "";
	}
	return false;
}


static void Note_Cells(const HydrologyBuilder & b,int cells)
{
	if ((b.P->Counters != nullptr) && (cells > 0)) {
		b.P->Counters->Add(HYBRID_WORK_ANALYSIS_CELLS,cells);
	}
}


static void Note_Samples(const HydrologyBuilder & b,int samples)
{
	if ((b.P->Counters != nullptr) && (samples > 0)) {
		b.P->Counters->Add(HYBRID_WORK_REFINEMENT_SAMPLES,samples);
	}
}


static void Note_Peak(const HydrologyBuilder & b)
{
	if (b.P->Counters != nullptr) {
		b.P->Counters->Note_Peak(HYBRID_WORK_PEAK_SCRATCH_BYTES,
										 (b.P->Arena != nullptr) ? b.P->Arena->Get_High_Water() : 0);
	}
}


//	Section 0.3's named draw for this phase: the hydrology domain plus the plan's own key spelling.
static float Hydro_Hash(const HydrologyBuilder & b,const char * name,unsigned int k0,unsigned int k1 = 0)
{
	return Hybrid_Hash01(b.P->HydrologySeed,Hybrid_Key(name),k0,k1);
}


/*
**	One block for every per-cell array.  The arena path is Section 16.3's; without an arena the
**	block comes from one heap vector, which is the same layout under a different owner.
*/
static bool Allocate_Arrays(HydrologyBuilder & b,HybridHydrologyResult * result)
{
	const int cells		= b.CellCount;
	const int corners		= (b.CountX + 1) * (b.CountY + 1);
	const int float_words= cells;			// one int-sized word per float entry

	//	Word counts, so the block is int-aligned by construction.
	const int words =
		(float_words * 3) +					// Height, Filled, Accum
		(cells * 6) +							// Key, Parent, Rank, Basin, Dest, Heap
		(cells * 1) +							// Group
		(((cells * 2) + 3) / 4) +			// Channel, Visited, one byte each, rounded to words
		(cells * (int)(sizeof(HydroOrderEntry) / sizeof(int))) +
		(corners * 2);							// Corner0, Corner1

	int * base = nullptr;
	if (b.P->Arena != nullptr) {
		base = (int *)b.P->Arena->Allocate(words * (int)sizeof(int),16);
		if (base == nullptr) {
			return Fail(result,HYBRID_HYDROLOGY_SCRATCH_EXHAUSTED,"drainage arrays do not fit the arena");
		}
	} else {
		b.Block.Resize(words);
		while (b.Block.Count() < words) b.Block.Add(0);
		if (b.Block.Count() < words) {
			return Fail(result,HYBRID_HYDROLOGY_SCRATCH_EXHAUSTED,"drainage arrays could not be allocated");
		}
		base = &b.Block[0];
	}

	int * cursor = base;
	b.Height	= (float *)cursor;	cursor += cells;
	b.Filled	= (float *)cursor;	cursor += cells;
	b.Accum	= (float *)cursor;	cursor += cells;
	b.Key		= cursor;				cursor += cells;
	b.Parent	= cursor;				cursor += cells;
	b.Rank	= cursor;				cursor += cells;
	b.Basin	= cursor;				cursor += cells;
	b.Dest	= cursor;				cursor += cells;
	b.Heap	= cursor;				cursor += cells;
	b.Group	= cursor;				cursor += cells;
	b.Channel= (unsigned char *)cursor;
	b.Visited= b.Channel + cells;
	cursor += ((cells * 2) + 3) / 4;
	b.Order	= (HydroOrderEntry *)cursor;
	cursor += cells * (int)(sizeof(HydroOrderEntry) / sizeof(int));
	b.Corner0 = cursor;				cursor += corners;
	b.Corner1 = cursor;				cursor += corners;

	memset(b.Channel,0,(size_t)(cells * 2));
	Note_Peak(b);
	return true;
}


/*
**	Steps 1 and 2: the provisional surface, plus the artificial `+4*tier_step` barrier over every
**	`BASE_PROTECTED` analysis cell so drainage can never route through a base reserve.
*/
static void Build_Provisional_Surface(HydrologyBuilder & b)
{
	const float barrier = BASE_BARRIER_TIERS * b.P->TierStep;

	for (int cell = 0; cell < b.CellCount; ++cell) {

		const Vector2 centre = b.Cell_Center(cell);
		float height = Hybrid_Provisional_Height(*b.P,*b.Matrices,*b.Landforms,centre);

		int cx = 0, cy = 0;
		if (b.Coarse_Cell(centre,&cx,&cy) && b.Matrices->Is_Base_Protected(cx,cy)) {
			height += barrier;
		}

		b.Height[cell]		= Hybrid_Quantize_Position(height);
		b.Filled[cell]		= b.Height[cell];
		b.Key[cell]			= 0;
		b.Parent[cell]		= -1;
		b.Rank[cell]		= -1;
		b.Basin[cell]		= -1;
		b.Dest[cell]		= -1;
		b.Accum[cell]		= 1.0f;
		b.Visited[cell]	= 0;
		b.Channel[cell]	= 0;
	}
	Note_Cells(b,b.CellCount);
}


/***********************************************************************************************
**	Step 3: deterministic Priority-Flood from every exterior analysis cell.
***********************************************************************************************/

static bool Heap_Less(const HydrologyBuilder & b,int left,int right)
{
	if (b.Key[left] != b.Key[right]) return b.Key[left] < b.Key[right];
	return left < right;						// the plan's `(filled_height,row_major_index)` key
}


static void Heap_Push(HydrologyBuilder & b,int cell)
{
	int index = b.HeapCount++;
	b.Heap[index] = cell;
	while (index > 0) {
		const int parent = (index - 1) / 2;
		if (!Heap_Less(b,b.Heap[index],b.Heap[parent])) break;
		const int swap = b.Heap[parent];
		b.Heap[parent]	= b.Heap[index];
		b.Heap[index]	= swap;
		index = parent;
	}
}


static int Heap_Pop(HydrologyBuilder & b)
{
	if (b.HeapCount <= 0) return -1;

	const int top = b.Heap[0];
	b.HeapCount--;
	if (b.HeapCount > 0) {
		b.Heap[0] = b.Heap[b.HeapCount];
		int index = 0;
		for (;;) {
			const int left		= (2 * index) + 1;
			const int right	= left + 1;
			int best				= index;
			if ((left < b.HeapCount) && Heap_Less(b,b.Heap[left],b.Heap[best]))	best = left;
			if ((right < b.HeapCount) && Heap_Less(b,b.Heap[right],b.Heap[best]))	best = right;
			if (best == index) break;
			const int swap = b.Heap[best];
			b.Heap[best]	= b.Heap[index];
			b.Heap[index]	= swap;
			index = best;
		}
	}
	return top;
}


static void Priority_Flood(HydrologyBuilder & b)
{
	b.HeapCount = 0;
	int rank = 0;

	for (int cell = 0; cell < b.CellCount; ++cell) {
		if (!b.Is_Exterior(cell)) continue;
		b.Filled[cell]		= b.Height[cell];
		b.Key[cell]			= Hybrid_Quantize_To_Int(b.Filled[cell],HYBRID_QUANTUM_POSITION);
		b.Parent[cell]		= -1;
		b.Rank[cell]		= rank++;
		b.Visited[cell]	= 1;
		Heap_Push(b,cell);
	}

	while (b.HeapCount > 0) {

		const int cell = Heap_Pop(b);
		if (cell < 0) break;
		const int x = b.Cell_X(cell);
		const int y = b.Cell_Y(cell);

		for (int n = 0; n < 8; ++n) {

			const int nx = x + NEIGHBOR_DX[n];
			const int ny = y + NEIGHBOR_DY[n];
			if (!b.Valid_Cell(nx,ny)) continue;

			const int neighbor = b.Cell_Index(nx,ny);
			if (b.Visited[neighbor] != 0) continue;

			b.Filled[neighbor]	= Max2(b.Height[neighbor],b.Filled[cell]);
			b.Key[neighbor]		= Hybrid_Quantize_To_Int(b.Filled[neighbor],HYBRID_QUANTUM_POSITION);
			b.Parent[neighbor]	= cell;
			b.Rank[neighbor]		= rank++;
			b.Visited[neighbor]	= 1;
			Heap_Push(b,neighbor);
		}
	}
	Note_Cells(b,b.CellCount);
}


/***********************************************************************************************
**	Step 3's retained basin labels, and the closed ring step 13 judges.
***********************************************************************************************/

/*
**	Trace the outer boundary of one labelled basin as a closed lattice ring.  The walk starts at the
**	basin's lowest-index cell -- which has no southern neighbour in the basin, so its bottom edge is
**	on the outer boundary -- and chains directed edges that keep the basin on their left.  A corner
**	that two unused edges still start from is a pinch: the outline would touch itself there, so the
**	basin is not a legal simple polygon and says so.
*/
static void Trace_Basin_Ring(HydrologyBuilder & b,HydroBasin * basin)
{
	const int label		= basin->Label;
	const int corner_w	= b.CountX + 1;
	DynamicVectorClass<HydroOutlineEdge> edges;

	for (int cell = 0; cell < b.CellCount; ++cell) {

		if (b.Basin[cell] != label) continue;
		const int x = b.Cell_X(cell);
		const int y = b.Cell_Y(cell);

		const bool south	= b.Valid_Cell(x,y - 1) && (b.Basin[b.Cell_Index(x,y - 1)] == label);
		const bool east	= b.Valid_Cell(x + 1,y) && (b.Basin[b.Cell_Index(x + 1,y)] == label);
		const bool north	= b.Valid_Cell(x,y + 1) && (b.Basin[b.Cell_Index(x,y + 1)] == label);
		const bool west	= b.Valid_Cell(x - 1,y) && (b.Basin[b.Cell_Index(x - 1,y)] == label);

		HydroOutlineEdge edge;
		edge.Used = false;
		if (!south)	{ edge.X0 = x;			edge.Y0 = y;		edge.X1 = x + 1;	edge.Y1 = y;		edges.Add(edge); }
		if (!east)	{ edge.X0 = x + 1;	edge.Y0 = y;		edge.X1 = x + 1;	edge.Y1 = y + 1;	edges.Add(edge); }
		if (!north)	{ edge.X0 = x + 1;	edge.Y0 = y + 1;	edge.X1 = x;		edge.Y1 = y + 1;	edges.Add(edge); }
		if (!west)	{ edge.X0 = x;			edge.Y0 = y + 1;	edge.X1 = x;		edge.Y1 = y;		edges.Add(edge); }
	}

	basin->Ring.Delete_All();
	basin->Simple	= true;
	basin->HasHoles= false;
	if (edges.Count() < 4) {
		basin->Simple = false;
		return;
	}

	//	Corner lookup, cleared only over the corners this basin actually uses.
	for (int i = 0; i < edges.Count(); ++i) {
		const int slot = (edges[i].Y0 * corner_w) + edges[i].X0;
		b.Corner0[slot] = -1;
		b.Corner1[slot] = -1;
	}
	for (int i = 0; i < edges.Count(); ++i) {
		const int slot = (edges[i].Y0 * corner_w) + edges[i].X0;
		if (b.Corner0[slot] < 0)			b.Corner0[slot] = i;
		else if (b.Corner1[slot] < 0)		b.Corner1[slot] = i;
		else										basin->Simple = false;
	}
	if (!basin->Simple) return;

	//	The start: the lowest-index cell's bottom edge, which is the first edge added for it.
	int start = -1;
	for (int i = 0; i < edges.Count(); ++i) {
		if ((edges[i].Y0 == edges[i].Y1) && (edges[i].X1 > edges[i].X0)) {
			const int cell_x = edges[i].X0;
			const int cell_y = edges[i].Y0;
			if (b.Valid_Cell(cell_x,cell_y) && (b.Basin[b.Cell_Index(cell_x,cell_y)] == label)) {
				start = i;
				break;
			}
		}
	}
	if (start < 0) {
		basin->Simple = false;
		return;
	}

	int current = start;
	int guard	= edges.Count() + 1;
	while (guard-- > 0) {

		edges[current].Used = true;
		basin->Ring.Add(Vector2(Hybrid_Quantize_Position(b.OriginX + ((float)edges[current].X0 * b.A)),
										Hybrid_Quantize_Position(b.OriginY + ((float)edges[current].Y0 * b.A))));

		const int slot	= (edges[current].Y1 * corner_w) + edges[current].X1;
		int next			= -1;
		const int first= b.Corner0[slot];
		const int second= b.Corner1[slot];

		if ((first >= 0) && !edges[first].Used)			next = first;
		else if ((second >= 0) && !edges[second].Used)	next = second;

		//	A corner both of whose edges are still unused would let the ring touch itself.
		if ((first >= 0) && (second >= 0) && !edges[first].Used && !edges[second].Used) {
			basin->Simple = false;
			return;
		}
		if (next < 0) break;					// back at the start corner: the ring is closed
		current = next;
	}

	for (int i = 0; i < edges.Count(); ++i) {
		if (!edges[i].Used) basin->HasHoles = true;
	}

	/*
	**	Collinear lattice corners carry no information, so the stored ring drops them.  This is not
	**	step 13's `A/2` simplification -- that one is applied to this ring later and is allowed to
	**	move it -- it only removes points that are exactly on an existing edge.
	*/
	DynamicVectorClass<Vector2> tight;
	const int count = basin->Ring.Count();
	for (int i = 0; i < count; ++i) {
		const Vector2 & previous	= basin->Ring[(i + count - 1) % count];
		const Vector2 & here			= basin->Ring[i];
		const Vector2 & following	= basin->Ring[(i + 1) % count];
		const float cross = ((here.X - previous.X) * (following.Y - here.Y)) -
								  ((here.Y - previous.Y) * (following.X - here.X));
		if (cross != 0.0f) tight.Add(here);
	}
	if (tight.Count() >= 4) {
		basin->Ring.Delete_All();
		for (int i = 0; i < tight.Count(); ++i) basin->Ring.Add(tight[i]);
	}

	basin->RingArea = Ring_Area(basin->Ring);
	Ring_Bounds(basin->Ring,&basin->BoundsMin,&basin->BoundsMax);
	if (basin->Ring.Count() < 4) basin->Simple = false;
}


/*
**	Step 3's "retain the pre-fill depression/basin labels": every four-connected run of cells the
**	fill had to raise is one depression.  Four-connectivity is deliberate -- two blobs that touch
**	only at a corner are two basins, and one polygon could not describe them both.
*/
static void Label_Basins(HydrologyBuilder & b)
{
	const float epsilon = 0.5f * HYBRID_QUANTUM_POSITION;
	b.Basins.Delete_All();

	DynamicVectorClass<int> queue;

	for (int seed = 0; seed < b.CellCount; ++seed) {

		if (b.Basin[seed] >= 0) continue;
		if (b.Filled[seed] <= (b.Height[seed] + epsilon)) continue;

		HydroBasin basin;
		basin.Label		= b.Basins.Count();
		basin.LowCell	= seed;
		basin.FloorHeight = b.Height[seed];
		basin.SpillHeight = b.Filled[seed];

		queue.Delete_All();
		queue.Add(seed);
		b.Basin[seed] = basin.Label;

		Vector2 sum(0.0f,0.0f);
		int head = 0;
		while (head < queue.Count()) {

			const int cell = queue[head++];
			basin.Cells++;
			sum += b.Cell_Center(cell);
			basin.FloorHeight = Min2(basin.FloorHeight,b.Height[cell]);
			basin.SpillHeight = Max2(basin.SpillHeight,b.Filled[cell]);

			const int x = b.Cell_X(cell);
			const int y = b.Cell_Y(cell);
			for (int n = 0; n < 8; n += 2) {			// E, N, W, S -- four-connected

				const int nx = x + NEIGHBOR_DX[n];
				const int ny = y + NEIGHBOR_DY[n];
				if (!b.Valid_Cell(nx,ny)) continue;

				const int neighbor = b.Cell_Index(nx,ny);
				if (b.Basin[neighbor] >= 0) continue;
				if (b.Filled[neighbor] <= (b.Height[neighbor] + epsilon)) continue;
				b.Basin[neighbor] = basin.Label;
				queue.Add(neighbor);
			}
		}

		basin.Area = (float)basin.Cells * b.A * b.A;
		basin.Center.Set(Hybrid_Quantize_Position(sum.X / (float)basin.Cells),
							  Hybrid_Quantize_Position(sum.Y / (float)basin.Cells));
		b.Basins.Add(basin);
	}

	for (int i = 0; i < b.Basins.Count(); ++i) {
		Trace_Basin_Ring(b,&b.Basins[i]);
	}
	Note_Cells(b,b.CellCount);
}


/***********************************************************************************************
**	Steps 4 and 5: D8 over the filled surface, then accumulation.
***********************************************************************************************/

static void Assign_D8(HydrologyBuilder & b)
{
	for (int cell = 0; cell < b.CellCount; ++cell) {

		const int x = b.Cell_X(cell);
		const int y = b.Cell_Y(cell);

		int	best			= -1;
		float best_height	= b.Filled[cell];

		for (int n = 0; n < 8; ++n) {

			const int nx = x + NEIGHBOR_DX[n];
			const int ny = y + NEIGHBOR_DY[n];
			if (!b.Valid_Cell(nx,ny)) continue;

			const int neighbor = b.Cell_Index(nx,ny);
			if (b.Filled[neighbor] >= b.Filled[cell]) continue;		// strictly lower only
			if ((best < 0) || (b.Filled[neighbor] < best_height)) {
				best			= neighbor;
				best_height	= b.Filled[neighbor];
			}
		}

		//	No strictly lower neighbour: the drain parent, which always has a lower flood rank.
		b.Dest[cell] = (best >= 0) ? best : b.Parent[cell];
	}
	Note_Cells(b,b.CellCount);
}


static void Accumulate_Flow(HydrologyBuilder & b)
{
	for (int cell = 0; cell < b.CellCount; ++cell) {
		b.Accum[cell]			= 1.0f;
		b.Order[cell].Key		= b.Key[cell];
		b.Order[cell].Rank	= b.Rank[cell];
		b.Order[cell].Cell	= cell;
	}
	::qsort(b.Order,(size_t)b.CellCount,sizeof(HydroOrderEntry),Order_Compare);

	for (int i = 0; i < b.CellCount; ++i) {
		const int cell = b.Order[i].Cell;
		const int destination = b.Dest[cell];
		if (destination >= 0) b.Accum[destination] += b.Accum[cell];
	}
	Note_Cells(b,b.CellCount);
}


/***********************************************************************************************
**	Steps 6 to 12: channels, D8 systems, the guided river, selection and the vector centerlines.
***********************************************************************************************/

/*
**	One distinct D8 source-to-sink system: the plan's step-9 record after shared downstream chains
**	have been merged.  Members of one system share a union-find root, which is exactly "merged by
**	analysis-cell id"; the centerline is the chain of the system's primary headwater.
*/
struct HydroSystem
{
	HydroSystem(void) : Root(-1),SourceCell(-1),SinkCell(-1),MaxAccumulation(0.0f),ChainLength(0.0f) { }

	bool	operator == (const HydroSystem & that) const	{ return Root == that.Root; }
	bool	operator != (const HydroSystem & that) const	{ return !(*this == that); }

	int		Root;
	int		SourceCell;
	int		SinkCell;
	float		MaxAccumulation;
	float		ChainLength;			// quantized world length of the primary chain

	DynamicVectorClass<int>	Chain;	// source to sink, inclusive
};


static int Group_Find(HydrologyBuilder & b,int cell)
{
	int root = cell;
	while (b.Group[root] != root) root = b.Group[root];
	while (b.Group[cell] != root) {
		const int next = b.Group[cell];
		b.Group[cell] = root;
		cell = next;
	}
	return root;
}


static void Group_Union(HydrologyBuilder & b,int a,int c)
{
	const int root_a = Group_Find(b,a);
	const int root_c = Group_Find(b,c);
	if (root_a == root_c) return;
	//	The lower cell index always wins, so the root never depends on visit order.
	if (root_a < root_c)	b.Group[root_c] = root_a;
	else						b.Group[root_a] = root_c;
}


//	Trace one headwater through D8 destinations to its sink, and measure the chain.
static float Trace_Chain(HydrologyBuilder & b,int headwater,DynamicVectorClass<int> * chain)
{
	chain->Delete_All();
	float length	= 0.0f;
	int	cell		= headwater;
	int	guard		= b.CellCount + 1;

	chain->Add(cell);
	while ((guard-- > 0) && (b.Dest[cell] >= 0)) {
		const int next = b.Dest[cell];
		length += (b.Cell_Center(next) - b.Cell_Center(cell)).Length();
		chain->Add(next);
		cell = next;
	}
	return Hybrid_Quantize_Position(length);
}


/*
**	Steps 7 to 9 at the current threshold.  Rebuilt from scratch on every step-11 threshold
**	reduction, which is what the plan asks for: the channel set, the headwaters, the merged systems
**	and their ranking all move with the threshold.
*/
static void Build_Systems(HydrologyBuilder & b,DynamicVectorClass<HydroSystem> * systems)
{
	systems->Delete_All();

	//	Step 7: channel cells.
	for (int cell = 0; cell < b.CellCount; ++cell) {
		b.Channel[cell] = (b.Accum[cell] >= b.Threshold) ? 1 : 0;
		b.Group[cell]	= cell;
	}

	//	Step 9's merge: a channel cell and its channel destination are one system.
	for (int cell = 0; cell < b.CellCount; ++cell) {
		if (b.Channel[cell] == 0) continue;
		const int destination = b.Dest[cell];
		if ((destination >= 0) && (b.Channel[destination] != 0)) Group_Union(b,cell,destination);
	}

	DynamicVectorClass<int> chain;

	//	Step 8: a headwater is a channel cell nothing upstream flows into.
	for (int cell = 0; cell < b.CellCount; ++cell) {

		if (b.Channel[cell] == 0) continue;

		const int x = b.Cell_X(cell);
		const int y = b.Cell_Y(cell);
		bool upstream = false;
		for (int n = 0; (n < 8) && !upstream; ++n) {
			const int nx = x + NEIGHBOR_DX[n];
			const int ny = y + NEIGHBOR_DY[n];
			if (!b.Valid_Cell(nx,ny)) continue;
			const int neighbor = b.Cell_Index(nx,ny);
			if (b.Channel[neighbor] == 0) continue;
			if (b.Dest[neighbor] == cell) upstream = true;
		}
		if (upstream) continue;

		const float length	= Trace_Chain(b,cell,&chain);
		const int root			= Group_Find(b,cell);
		const int sink			= chain[chain.Count() - 1];

		int existing = -1;
		for (int i = 0; i < systems->Count(); ++i) {
			if ((*systems)[i].Root == root) { existing = i; break; }
		}

		if (existing < 0) {
			HydroSystem system;
			system.Root					= root;
			system.SourceCell			= cell;
			system.SinkCell			= sink;
			system.ChainLength		= length;
			system.MaxAccumulation	= b.Accum[sink];
			for (int i = 0; i < chain.Count(); ++i) system.Chain.Add(chain[i]);
			systems->Add(system);
			continue;
		}

		HydroSystem & system = (*systems)[existing];
		system.MaxAccumulation = Max2(system.MaxAccumulation,b.Accum[sink]);

		/*
		**	The system's own centerline is its primary headwater's chain: the longest chain, then the
		**	one carrying more water at its source, then the lower analysis-cell id.  Every other
		**	headwater in the system is a tributary, which Section 5 does not count separately.
		*/
		bool replace = false;
		if (length > system.ChainLength)															replace = true;
		else if (length == system.ChainLength) {
			if (b.Accum[cell] > b.Accum[system.SourceCell])									replace = true;
			else if ((b.Accum[cell] == b.Accum[system.SourceCell]) && (cell < system.SourceCell))	replace = true;
		}
		if (!replace) continue;

		system.SourceCell	= cell;
		system.SinkCell	= sink;
		system.ChainLength= length;
		system.Chain.Delete_All();
		for (int i = 0; i < chain.Count(); ++i) system.Chain.Add(chain[i]);
	}
	Note_Cells(b,b.CellCount);
}


/*
**	Step 9's ranking: descending maximum accumulation, then descending quantized chain length, then
**	the lower analysis-cell id, which is this record's StableId order before any id is assigned.
*/
static void Rank_Systems(DynamicVectorClass<HydroSystem> * systems)
{
	const int count = systems->Count();
	for (int i = 1; i < count; ++i) {
		for (int j = i; j > 0; --j) {

			const HydroSystem & left	= (*systems)[j - 1];
			const HydroSystem & right	= (*systems)[j];
			bool swap = false;

			if (right.MaxAccumulation > left.MaxAccumulation)						swap = true;
			else if (right.MaxAccumulation == left.MaxAccumulation) {
				if (right.ChainLength > left.ChainLength)								swap = true;
				else if ((right.ChainLength == left.ChainLength) &&
							(right.SourceCell < left.SourceCell))						swap = true;
			}
			if (!swap) break;

			HydroSystem hold = (*systems)[j - 1];
			(*systems)[j - 1] = (*systems)[j];
			(*systems)[j] = hold;
		}
	}
}


//	Step 16's legality rule for a mouth that reaches the exterior.
static bool Sink_Is_Legal(HydrologyBuilder & b,int cell)
{
	const Vector2 centre = b.Cell_Center(cell);
	int cx = 0, cy = 0;
	if (!b.Coarse_Cell(centre,&cx,&cy)) return false;
	if (b.Matrices->Is_Base_Protected(cx,cy)) return false;
	return true;
}


/*
**	Step 12: the vector centerline of one D8-derived river.  Direction-change cells and the two
**	endpoints are retained, one interior three-point smoothing pass is applied, and the centripetal
**	Catmull-Rom result is accepted only when every `A/2` sample stays within `A` of the original D8
**	polyline and its projection along that polyline never goes backwards.  Otherwise the unsmoothed
**	D8 polyline is the centerline, exactly as the plan requires.
*/
static void Build_River_Centerline(HydrologyBuilder & b,const HydroSystem & system,HybridRiverSystem * river)
{
	DynamicVectorClass<Vector2> raw;
	for (int i = 0; i < system.Chain.Count(); ++i) raw.Add(b.Cell_Center(system.Chain[i]));

	river->Points.Delete_All();
	river->Stations.Delete_All();
	river->Smoothed = false;

	if (raw.Count() < 2) {
		for (int i = 0; i < raw.Count(); ++i) {
			river->Points.Add(raw[i]);
			river->Stations.Add(0.0f);
		}
		return;
	}

	//	Endpoints plus every direction change.
	DynamicVectorClass<Vector2> controls;
	controls.Add(raw[0]);
	for (int i = 1; i < (raw.Count() - 1); ++i) {
		const int previous	= system.Chain[i - 1];
		const int here			= system.Chain[i];
		const int following	= system.Chain[i + 1];
		const int dx0 = b.Cell_X(here) - b.Cell_X(previous);
		const int dy0 = b.Cell_Y(here) - b.Cell_Y(previous);
		const int dx1 = b.Cell_X(following) - b.Cell_X(here);
		const int dy1 = b.Cell_Y(following) - b.Cell_Y(here);
		if ((dx0 != dx1) || (dy0 != dy1)) controls.Add(raw[i]);
	}
	controls.Add(raw[raw.Count() - 1]);

	//	Exactly one interior three-point smoothing pass.
	DynamicVectorClass<Vector2> smoothed;
	for (int i = 0; i < controls.Count(); ++i) {
		if ((i == 0) || (i == (controls.Count() - 1))) {
			smoothed.Add(controls[i]);
			continue;
		}
		const Vector2 & previous	= controls[i - 1];
		const Vector2 & here			= controls[i];
		const Vector2 & following	= controls[i + 1];
		smoothed.Add(Vector2(Hybrid_Quantize_Position((0.25f * previous.X) + (0.50f * here.X) + (0.25f * following.X)),
									Hybrid_Quantize_Position((0.25f * previous.Y) + (0.50f * here.Y) + (0.25f * following.Y))));
	}

	DynamicVectorClass<Vector2> samples;
	DynamicVectorClass<float> stations;
	bool accepted = false;

	if ((smoothed.Count() >= 2) &&
		 Hybrid_Sample_Centripetal_Catmull_Rom(&smoothed[0],smoothed.Count(),0.5f * b.A,&samples,&stations)) {

		accepted = true;
		float previous_projection = -1.0f;
		for (int i = 0; (i < samples.Count()) && accepted; ++i) {
			float projection = 0.0f;
			const float distance = Distance_To_Polyline(raw,samples[i],&projection,nullptr);
			if (distance > b.A)							accepted = false;
			if (projection < previous_projection)	accepted = false;
			previous_projection = projection;
		}
		Note_Samples(b,samples.Count());
	}

	if (accepted) {
		river->Smoothed = true;
		for (int i = 0; i < samples.Count(); ++i) {
			river->Points.Add(Vector2(Hybrid_Quantize_Position(samples[i].X),
											  Hybrid_Quantize_Position(samples[i].Y)));
			river->Stations.Add(Hybrid_Quantize_Position(stations[i]));
		}
		return;
	}

	float travelled = 0.0f;
	for (int i = 0; i < raw.Count(); ++i) {
		if (i > 0) travelled += (raw[i] - raw[i - 1]).Length();
		river->Points.Add(raw[i]);
		river->Stations.Add(Hybrid_Quantize_Position(travelled));
	}
}


//	Section 11's width, the filled-drainage surface and the bounds, per accepted river sample.
static void Finish_River_Record(HydrologyBuilder & b,const HydroSystem * system,HybridRiverSystem * river)
{
	river->Accumulation.Delete_All();
	river->Widths.Delete_All();
	river->Surfaces.Delete_All();

	const int count = river->Points.Count();
	float half_max = 0.0f;

	for (int i = 0; i < count; ++i) {

		float accumulation	= 0.0f;
		float surface			= 0.0f;

		if (system != nullptr) {

			//	The nearest chain cell owns this sample's drainage and its filled surface.
			int	nearest	= system->Chain[0];
			float best		= 3.4e38f;
			for (int c = 0; c < system->Chain.Count(); ++c) {
				const float distance = (b.Cell_Center(system->Chain[c]) - river->Points[i]).Length();
				if (distance < best) {
					best		= distance;
					nearest	= system->Chain[c];
				}
			}
			accumulation	= b.Accum[nearest];
			surface			= b.Filled[nearest];

		} else {

			/*
			**	Step 10's synthetic accumulation for a guided river: linear from the threshold at the
			**	source to four times the threshold at the sink, so the ordinary width formula applies
			**	unchanged.  Its surface is the provisional terrain height, which is what Phase 7 step 1
			**	asks for on a guided river.
			*/
			const float t = (count > 1) ? ((float)i / (float)(count - 1)) : 0.0f;
			accumulation	= b.Threshold * (1.0f + ((GUIDED_SINK_MULTIPLIER - 1.0f) * t));
			surface			= Hybrid_Provisional_Height(*b.P,*b.Matrices,*b.Landforms,river->Points[i]);
		}

		const float width = Hybrid_Quantize_Position(Hybrid_River_Width(accumulation,b.Threshold,
																							 b.P->StreamWidth,b.P->RiverWidth));
		river->Accumulation.Add(accumulation);
		river->Widths.Add(width);
		river->Surfaces.Add(Hybrid_Quantize_Position(surface));
		half_max = Max2(half_max,0.5f * width);
	}

	river->MaxAccumulation	= (count > 0) ? river->Accumulation[count - 1] : 0.0f;
	river->Length				= (river->Stations.Count() > 0) ? river->Stations[river->Stations.Count() - 1] : 0.0f;
	river->BedDepth			= b.P->RiverBedDepth;
	river->Source				= (count > 0) ? river->Points[0] : Vector2(0.0f,0.0f);
	river->Mouth				= (count > 0) ? river->Points[count - 1] : Vector2(0.0f,0.0f);

	Vector2 minimum(0.0f,0.0f);
	Vector2 maximum(0.0f,0.0f);
	if (count > 0) {
		minimum = river->Points[0];
		maximum = river->Points[0];
		for (int i = 1; i < count; ++i) {
			minimum.X = Min2(minimum.X,river->Points[i].X);
			minimum.Y = Min2(minimum.Y,river->Points[i].Y);
			maximum.X = Max2(maximum.X,river->Points[i].X);
			maximum.Y = Max2(maximum.Y,river->Points[i].Y);
		}
	}
	river->BoundsMin.Set(minimum.X - half_max,minimum.Y - half_max);
	river->BoundsMax.Set(maximum.X + half_max,maximum.Y + half_max);
}


/*
**	Step 10: the frozen Section-6.10 guided river, inserted as selected system 0.
**
**	Direction: a coastal guide always flows toward the sea, which is the side its `VSign` names; a
**	non-coastal guide flows from the higher provisional endpoint to the lower one, tie broken from
**	the negative strategic `v` endpoint toward the positive.
*/
static bool Insert_Guided_River(HydrologyBuilder & b)
{
	const HybridArchetypeIntent * intent = b.Intents->Find_First(HYBRID_INTENT_GUIDED_RIVER);
	bool coastal = false;
	if (intent == nullptr) {
		intent = b.Intents->Find_First(HYBRID_INTENT_COASTAL_GUIDED_RIVER);
		coastal = (intent != nullptr);
	}
	if ((intent == nullptr) || (intent->Points.Count() < 2)) return false;

	HybridRiverSystem river;
	river.StableId			= b.NextStableId++;
	river.IntentStableId	= intent->StableId;
	river.Guided			= true;
	river.SelectionRank	= 0;

	const Vector2 first	= intent->Points[0];
	const Vector2 last	= intent->Points[intent->Points.Count() - 1];
	bool reverse			= false;

	if (coastal) {
		const float sign		= (float)((intent->VSign >= 0) ? 1 : -1);
		const float first_v	= b.Identity->Frame.To_Strategic(first).Y * sign;
		const float last_v	= b.Identity->Frame.To_Strategic(last).Y * sign;
		reverse = (first_v > last_v);								// the sea is the +sign side
	} else {
		const float first_h	= Hybrid_Provisional_Height(*b.P,*b.Matrices,*b.Landforms,first);
		const float last_h	= Hybrid_Provisional_Height(*b.P,*b.Matrices,*b.Landforms,last);
		if (first_h < last_h)			reverse = true;
		else if (first_h == last_h)	reverse = (b.Identity->Frame.To_Strategic(first).Y >
															b.Identity->Frame.To_Strategic(last).Y);
	}

	float travelled = 0.0f;
	for (int i = 0; i < intent->Points.Count(); ++i) {
		const int source = reverse ? (intent->Points.Count() - 1 - i) : i;
		const Vector2 point = intent->Points[source];
		if (i > 0) travelled += (point - river.Points[i - 1]).Length();
		river.Points.Add(point);
		river.Stations.Add(Hybrid_Quantize_Position(travelled));
	}

	Finish_River_Record(b,nullptr,&river);
	b.Out->Rivers.Add(river);
	return true;
}


/*
**	Step 11: select the remaining D8 systems under the `>=0.18D` source-separation rule, reducing the
**	threshold by exactly 0.80 at most three times when too few qualify.  A system whose mouth is not
**	a legal exterior sink is never selectable, which is step 16's rule applied where it can still be
**	satisfied by choosing a different system.
*/
static bool Select_Rivers(HydrologyBuilder & b,HybridHydrologyResult * result)
{
	const int required = b.Out->RequiredRivers;
	const int guided = b.Out->Rivers.Count();
	int remaining = required - guided;
	if (remaining <= 0) {
		b.Out->RiverThreshold = b.Threshold;
		return true;
	}

	const float separation = SOURCE_SEPARATION_D * b.P->Constants.D;
	DynamicVectorClass<HydroSystem> systems;
	StringClass detail;

	for (int reduction = 0; reduction <= THRESHOLD_REDUCTION_LIMIT; ++reduction) {

		if (reduction > 0) {
			b.Threshold = Max2(1.0f,Hybrid_Round_Half_Up(b.Threshold * THRESHOLD_REDUCTION));
			b.Out->ThresholdReductions = reduction;
		}

		Build_Systems(b,&systems);
		Rank_Systems(&systems);

		//	The guided river's own source counts against the separation rule.
		DynamicVectorClass<Vector2> sources;
		for (int i = 0; i < guided; ++i) sources.Add(b.Out->Rivers[i].Source);

		DynamicVectorClass<int> chosen;
		for (int i = 0; (i < systems.Count()) && (chosen.Count() < remaining); ++i) {

			const HydroSystem & system = systems[i];
			if (system.Chain.Count() < 2) continue;
			if (!Sink_Is_Legal(b,system.SinkCell)) continue;

			//	A channel through a Tiberium hard reserve is a world Phase 8 has to refuse, so it is not a
			//	selectable system: the ranking simply moves to the next one.
			bool through_reserve = false;
			for (int c = 0; (c < system.Chain.Count()) && !through_reserve; ++c) {
				if (Point_In_Tiberium_Reserve(b,b.Cell_Center(system.Chain[c]))) through_reserve = true;
			}
			if (through_reserve) continue;

			const Vector2 source = b.Cell_Center(system.SourceCell);
			bool separated = true;
			for (int s = 0; (s < sources.Count()) && separated; ++s) {
				if ((source - sources[s]).Length() < separation) separated = false;
			}
			if (!separated) continue;

			sources.Add(source);
			chosen.Add(i);
		}

		if (chosen.Count() < remaining) continue;

		for (int i = 0; i < chosen.Count(); ++i) {

			const HydroSystem & system = systems[chosen[i]];
			HybridRiverSystem river;
			river.StableId		= b.NextStableId++;
			river.SelectionRank= guided + i;
			river.SourceCell	= system.SourceCell;
			river.SinkCell		= system.SinkCell;

			Build_River_Centerline(b,system,&river);
			Finish_River_Record(b,&system,&river);
			b.Out->Rivers.Add(river);
		}

		b.Out->RiverThreshold = b.Threshold;
		return true;
	}

	detail.Format("%d of %d river systems after %d threshold reductions (threshold %.3f)",
					  b.Out->Rivers.Count(),required,THRESHOLD_REDUCTION_LIMIT,b.Threshold);
	return Fail(result,HYBRID_HYDROLOGY_RIVER_QUOTA,detail.Peek_Buffer());
}


/***********************************************************************************************
**	Steps 13 to 17: lakes, the water-fraction basins, the coast, the mouths and the islands.
***********************************************************************************************/

/*
**	Is `world` inside a Tiberium hard reserve?
**
**	Phase 4 reserves every Tiberium role exclusively at `field_radius_max`, and Phase 8 requires a
**	field's *full* reserve to be dry, so water that stands in one is a world Phase 8 must refuse.  The
**	reserve is measured on its own disc rather than on the rasterized coarse flag for the same reason
**	Phase 5 measures it that way: a cell is `S` wide and flagged from its centre, so the flag and the
**	geometry disagree by up to half a cell -- and the phase that fails over that disagreement is a later
**	one, over ground this phase could simply have avoided.
*/
static bool Point_In_Tiberium_Reserve(const HydrologyBuilder & b,const Vector2 & world)
{
	for (int s = 0; s < b.Graph->Sites.Count(); ++s) {

		const HybridStrategicSite & site = b.Graph->Sites[s];
		if (site.Radius <= 0.0f) continue;
		if ((site.Role != HYBRID_SITE_HOME_TIBERIUM_GDI) && (site.Role != HYBRID_SITE_HOME_TIBERIUM_NOD) &&
			 (site.Role != HYBRID_SITE_CONTESTED_TIBERIUM) && (site.Role != HYBRID_SITE_REMOTE_TIBERIUM)) continue;
		if ((world - site.Position).Length() <= site.Radius) return true;
	}
	return false;
}


/*
**	Is `world` on a frozen strategic waypoint the heavy routes have to pass *through*?
**
**	Step 13's legal-basin rule says a basin is legal only when "every heavy-route corridor it intersects
**	retains a connected dry bypass ... inside MaxCorridorHalfWidth(PRIMARY_2) around that heavy
**	corridor".  A corridor's own frozen waypoints are where that connectivity is anchored: Phase 9 makes
**	each of them a topology node and Phase 10 routes a leg that starts or ends exactly there, so standing
**	water on one leaves no dry bypass through it at all -- the widest dry lane somewhere else in the
**	corridor cannot be used, because the route is not free to miss the waypoint.  Lakes and coast get no
**	crossing windows either (step 20), so such a waypoint is simply unreachable.
**
**	The clearance has to be enough for a road to *arrive and leave*, not merely to stand there.  Phase 10
**	routes on the analysis grid, so a waypoint whose neighbouring analysis cells are all wet is enclosed
**	even though the waypoint itself is dry: measured on the failing seed, the lake ringed a frozen family
**	waypoint at one cell's distance and left it unreachable from every direction.  The clearance is
**	therefore one analysis spacing plus the roadbed half width, never less than the roadbed half width
**	plus the lane clearance.
*/
static bool Ring_Clears_Route_Waypoints(const HydrologyBuilder & b,const DynamicVectorClass<Vector2> & ring);


static bool Point_On_Route_Waypoint(const HydrologyBuilder & b,const Vector2 & world)
{
	const float half_roadbed = 0.5f * Hybrid_Roadbed_Width(HYBRID_ROAD_PRIMARY_2,b.P->Constants.W);
	const float clearance = Max2(half_roadbed + b.P->MinVehicleLaneClearance,
										  b.P->Constants.A + half_roadbed);

	for (int s = 0; s < b.Graph->Sites.Count(); ++s) {

		const HybridStrategicSite & site = b.Graph->Sites[s];
		if ((site.Role != HYBRID_SITE_ROUTE_WAYPOINT) && (site.Role != HYBRID_SITE_FRONT_CENTER) &&
			 (site.Role != HYBRID_SITE_ROAD_JUNCTION) && (site.Role != HYBRID_SITE_CROSSING_OBLIGATION)) continue;
		if ((world - site.Position).Length() <= clearance) return true;
	}
	return false;
}


//	Is any part of this ring inside a hard water exclusion -- BASE_PROTECTED, OPEN_MANEUVER, a field or
//	a frozen route waypoint?
static bool Ring_Clears_Water_Reserves(HydrologyBuilder & b,const DynamicVectorClass<Vector2> & ring)
{
	const HybridSemanticMatrix & matrix = b.Matrices->Matrix;
	Vector2 minimum(0.0f,0.0f);
	Vector2 maximum(0.0f,0.0f);
	Ring_Bounds(ring,&minimum,&maximum);

	if (!Ring_Clears_Route_Waypoints(b,ring)) return false;

	int samples = 0;
	for (float y = minimum.Y; y <= maximum.Y; y += b.A) {
		for (float x = minimum.X; x <= maximum.X; x += b.A) {

			const Vector2 point(x,y);
			if (!Point_In_Ring(ring,point)) continue;
			samples++;

			if (Point_In_Tiberium_Reserve(b,point))										return false;

			int cx = 0, cy = 0;
			if (!b.Coarse_Cell(point,&cx,&cy)) continue;
			if (b.Matrices->Is_Base_Protected(cx,cy))									return false;
			if (matrix.Has_Flag(cx,cy,HYBRID_FLAG_NO_IMMUTABLE_BARRIER))			return false;
		}
	}
	Note_Samples(b,samples);
	return true;
}


/*
**	The waypoint clearance, measured against the polygon itself rather than against its interior samples:
**	water that merely *surrounds* a frozen waypoint never contains it, and that is exactly the case that
**	encloses it on the analysis grid.
*/
static bool Ring_Clears_Route_Waypoints(const HydrologyBuilder & b,const DynamicVectorClass<Vector2> & ring)
{
	const float half_roadbed = 0.5f * Hybrid_Roadbed_Width(HYBRID_ROAD_PRIMARY_2,b.P->Constants.W);
	const float clearance = Max2(half_roadbed + b.P->MinVehicleLaneClearance,
										  b.P->Constants.A + half_roadbed);

	for (int s = 0; s < b.Graph->Sites.Count(); ++s) {

		const HybridStrategicSite & site = b.Graph->Sites[s];
		if ((site.Role != HYBRID_SITE_ROUTE_WAYPOINT) && (site.Role != HYBRID_SITE_FRONT_CENTER) &&
			 (site.Role != HYBRID_SITE_ROAD_JUNCTION) && (site.Role != HYBRID_SITE_CROSSING_OBLIGATION)) continue;

		if (Point_In_Ring(ring,site.Position))						return false;
		if (Distance_To_Ring(ring,site.Position) <= clearance)	return false;
	}
	return true;
}


/*
**	Steps 13/15's dry-bypass rule.  Every required heavy corridor must keep a connected dry crossing
**	width of `heavy_vehicle_width + 2*min_vehicle_lane_clearance` somewhere inside
**	`MaxCorridorHalfWidth(PRIMARY_2)` of its axis, counting the accepted standing water plus the
**	candidate ring.  Rivers are not counted: a river is a crossable obstruction with windows, while a
**	lake or a coast is not bridgeable in this generator version at all.
*/
static bool Corridor_Bypass_Survives(HydrologyBuilder & b,const DynamicVectorClass<Vector2> * candidate)
{
	const HybridConstants & k = b.P->Constants;
	const float required	= b.P->HeavyVehicleWidth + (2.0f * b.P->MinVehicleLaneClearance);
	const float limit		= Hybrid_Max_Corridor_Half_Width(HYBRID_ROAD_PRIMARY_2,k.S,k.D);
	const float step		= Max2(0.5f * k.W,k.Q);
	if (required <= 0.0f) return true;

	for (int r = 0; r < b.Matrices->Reservations.Count(); ++r) {

		const HybridReservation & reservation = b.Matrices->Reservations[r];
		if (reservation.Priority != HYBRID_RESERVE_HEAVY_CORRIDOR) continue;

		Vector2 axis = reservation.B - reservation.A;
		const float axis_length = axis.Length();
		if (axis_length <= 0.0f) continue;
		axis *= (1.0f / axis_length);
		const Vector2 normal(-axis.Y,axis.X);
		const float half = limit;

		for (float along = 0.0f; along <= axis_length; along += b.A) {

			const Vector2 centre = reservation.A + (axis * along);
			float best	= 0.0f;
			float run	= 0.0f;

			for (float lateral = -half; lateral <= half; lateral += step) {

				const Vector2 point = centre + (normal * lateral);
				bool wet = Hybrid_Point_In_Standing_Water(*b.Out,point,nullptr,nullptr);
				if (!wet && (candidate != nullptr)) wet = Point_In_Ring(*candidate,point);

				if (wet) {
					run = 0.0f;
				} else {
					run	= run + step;
					best	= Max2(best,run);
				}
			}
			if (best < required) return false;
		}
	}
	return true;
}


//	The lowest provisional shoreline height around a ring, which clamps the water level by `Q`.
static float Ring_Min_Shore_Height(HydrologyBuilder & b,const DynamicVectorClass<Vector2> & ring)
{
	float lowest = 3.4e38f;
	for (int i = 0; i < ring.Count(); ++i) {
		lowest = Min2(lowest,Hybrid_Provisional_Height(*b.P,*b.Matrices,*b.Landforms,ring[i]));
	}
	Note_Samples(b,ring.Count());
	return (lowest < 3.4e38f) ? lowest : b.P->SeaLevel;
}


/*
**	Step 13's `A/2` simplification with its 5% area budget.  A simplification that moves the area
**	further than that is discarded and the original ring stands.
*/
static void Apply_Ring_Simplification(HydrologyBuilder & b,HybridWaterBody * body)
{
	if (body->Points.Count() < 5) return;

	const float before = Ring_Area(body->Points);
	if (before <= 0.0f) return;

	DynamicVectorClass<Vector2> simplified;
	Simplify_Ring(body->Points,0.5f * b.A,&simplified);
	if (simplified.Count() < 4) return;

	const float after = Ring_Area(simplified);
	if (after <= 0.0f) return;
	if ((float)fabs((double)(after - before)) > (SIMPLIFY_AREA_TOLERANCE * before)) return;

	body->Points.Delete_All();
	for (int i = 0; i < simplified.Count(); ++i) body->Points.Add(simplified[i]);
	body->Simplified	= true;
	body->Area			= after;
}


static void Finish_Body_Record(HydrologyBuilder & b,HybridWaterBody * body)
{
	body->Area = Ring_Area(body->Points);
	Ring_Bounds(body->Points,&body->BoundsMin,&body->BoundsMax);
	body->Center = Ring_Centroid(body->Points);
	Note_Samples(b,body->Points.Count());
}


/*
**	One candidate lake from a retained basin: the legality rules of step 13, then the ring, its
**	simplification and its water level.  False leaves nothing behind.
*/
static bool Try_Basin_Lake(HydrologyBuilder & b,const HydroBasin & basin,bool required,HybridWaterBody * out)
{
	if (!basin.Simple) 									{ b.LastReject = "basin outline is not simple"; return false; }
	if (basin.Ring.Count() < 4)						{ b.LastReject = "basin outline is degenerate"; return false; }
	if (basin.RingArea < b.TrueLakeMinArea)			{ b.LastReject = "basin is smaller than a true lake"; return false; }
	if (!Ring_Clears_Water_Reserves(b,basin.Ring))	{ b.LastReject = "basin reaches a no-water reserve"; return false; }
	if (!Corridor_Bypass_Survives(b,&basin.Ring))	{ b.LastReject = "basin starves a heavy corridor"; return false; }

	HybridWaterBody body;
	body.Kind			= HYBRID_WATER_BODY_LAKE;
	body.BasinLabel	= basin.Label;
	body.Required		= required;
	body.BedDepth		= b.P->LakeBedDepth;
	for (int i = 0; i < basin.Ring.Count(); ++i) body.Points.Add(basin.Ring[i]);

	Finish_Body_Record(b,&body);
	Apply_Ring_Simplification(b,&body);
	Finish_Body_Record(b,&body);

	//	Water elevation: the spill height, never above the lowest shoreline point.
	const float shore = Ring_Min_Shore_Height(b,body.Points);
	body.SurfaceHeight = Hybrid_Quantize_Position(Min2(basin.SpillHeight,shore - b.P->Constants.Q));

	*out = body;
	return true;
}


/*
**	Step 14's required fallback: a deterministic eight-radial carved polygon at the archetype guide's
**	own radius, shrunk by exactly 10% per local retry and never below the Section-5 qualified area.
*/
static bool Try_Guide_Lake(HydrologyBuilder & b,const HybridArchetypeIntent & intent,HybridWaterBody * out)
{
	const Vector2 centre = b.Identity->Frame.To_World(intent.Guide);
	const int retries = (b.P->MaxLocalRetriesPerStage > 0) ? b.P->MaxLocalRetriesPerStage : 0;

	float multipliers[GUIDE_POLYGON_RADIALS];
	float shape = 0.0f;
	for (int i = 0; i < GUIDE_POLYGON_RADIALS; ++i) {
		multipliers[i] = Hybrid_Range(ISLAND_RADIUS_MIN,ISLAND_RADIUS_MAX,
												Hydro_Hash(b,"lake_radial",intent.StableId,(unsigned int)i),
												HYBRID_QUANTUM_NORMALIZED);
	}
	for (int i = 0; i < GUIDE_POLYGON_RADIALS; ++i) {
		shape += multipliers[i] * multipliers[(i + 1) % GUIDE_POLYGON_RADIALS];
	}
	const float unit_area = 0.5f * (float)sin(2.0 * 3.14159265358979323846 / (double)GUIDE_POLYGON_RADIALS) * shape;
	if (unit_area <= 0.0f) { b.LastReject = "degenerate lake guide polygon"; return false; }

	const float min_radius = (float)sqrt((double)(b.TrueLakeMinArea / unit_area));
	float radius = Max2(intent.Radius,min_radius);

	for (int retry = 0; retry <= retries; ++retry) {

		if (retry > 0) {
			radius *= (1.0f - LAKE_SHRINK_FRACTION);
			b.Out->LocalRetries++;
			if (radius < min_radius) {
				b.LastReject = "lake guide shrank below the Section-5 qualified area";
				return false;
			}
		}

		HybridWaterBody body;
		body.Kind					= HYBRID_WATER_BODY_LAKE;
		body.Required				= true;
		body.IntentStableId		= intent.StableId;
		body.FromGuidePolygon	= true;
		body.BedDepth				= b.P->LakeBedDepth;
		body.Retries				= retry;

		for (int i = 0; i < GUIDE_POLYGON_RADIALS; ++i) {
			const float angle = (2.0f * 3.14159265358979323846f * (float)i) / (float)GUIDE_POLYGON_RADIALS;
			const float r = radius * multipliers[i];
			body.Points.Add(Vector2(Hybrid_Quantize_Position(centre.X + (r * (float)cos((double)angle))),
											Hybrid_Quantize_Position(centre.Y + (r * (float)sin((double)angle)))));
		}
		Finish_Body_Record(b,&body);

		if (body.Area < b.TrueLakeMinArea)					{ b.LastReject = "lake guide is below the qualified area"; continue; }
		if (!Ring_Clears_Water_Reserves(b,body.Points))	{ b.LastReject = "lake guide reaches a no-water reserve"; continue; }
		if (!Corridor_Bypass_Survives(b,&body.Points))	{ b.LastReject = "lake guide starves a heavy corridor"; continue; }

		const float shore = Ring_Min_Shore_Height(b,body.Points);
		body.SurfaceHeight = Hybrid_Quantize_Position(shore - b.P->Constants.Q);
		*out = body;
		return true;
	}
	return false;
}


/*
**	Steps 13 and 14: the required lakes, from the ranked legal basins first and from the archetype
**	guides only when the ranking is short.
*/
static bool Build_Lakes(HydrologyBuilder & b,HybridHydrologyResult * result)
{
	if (!b.P->Lakes) return true;

	//	Rank legal basins by descending quantized area then basin label, which is their creation order.
	DynamicVectorClass<int> ranked;
	for (int i = 0; i < b.Basins.Count(); ++i) {
		if (!b.Basins[i].Simple) continue;
		if (b.Basins[i].RingArea < b.TrueLakeMinArea) continue;
		ranked.Add(i);
	}
	for (int i = 1; i < ranked.Count(); ++i) {
		for (int j = i; j > 0; --j) {
			const HydroBasin & left		= b.Basins[ranked[j - 1]];
			const HydroBasin & right	= b.Basins[ranked[j]];
			const float left_area		= Hybrid_Quantize_Position(left.RingArea);
			const float right_area		= Hybrid_Quantize_Position(right.RingArea);
			bool swap = false;
			if (right_area > left_area)											swap = true;
			else if ((right_area == left_area) && (right.Label < left.Label))	swap = true;
			if (!swap) break;
			const int hold = ranked[j - 1];
			ranked[j - 1]	= ranked[j];
			ranked[j]		= hold;
		}
	}

	const int required = b.Out->RequiredLakes;
	int accepted = 0;

	for (int i = 0; (i < ranked.Count()) && (accepted < required); ++i) {
		HybridWaterBody body;
		if (!Try_Basin_Lake(b,b.Basins[ranked[i]],true,&body)) continue;
		body.StableId = b.NextStableId++;
		b.Out->Bodies.Add(body);
		accepted++;
	}

	//	Step 14's guides, in intent order, for whatever the ranking could not supply.
	for (int i = 0; (i < b.Intents->Intents.Count()) && (accepted < required); ++i) {

		const HybridArchetypeIntent & intent = b.Intents->Intents[i];
		if ((intent.Kind != HYBRID_INTENT_LAKE_GUIDE) && (intent.Kind != HYBRID_INTENT_BASIN_GUIDE)) continue;

		HybridWaterBody body;
		if (!Try_Guide_Lake(b,intent,&body)) continue;
		body.StableId = b.NextStableId++;
		b.Out->Bodies.Add(body);
		accepted++;
	}

	if (accepted < required) {
		StringClass detail;
		detail.Format("%d of %d required lakes (%d legal basins, last refusal: %s)",accepted,required,
						  ranked.Count(),b.LastReject);
		return Fail(result,HYBRID_HYDROLOGY_LAKE_QUOTA,detail.Peek_Buffer());
	}

	/*
	**	Section 11's `water_fraction`: optional basins in descending (area, label) order, stopping as
	**	soon as the next one would move the standing-water fraction further from the target than
	**	stopping does.  A required lake is never distorted to hit the fraction.
	*/
	float area = 0.0f;
	for (int i = 0; i < b.Out->Bodies.Count(); ++i) {
		if (b.Out->Bodies[i].Kind == HYBRID_WATER_BODY_LAKE) area += b.Out->Bodies[i].Area;
	}

	const float target = Clamp2(b.P->WaterFraction,0.0f,1.0f) * b.WorldArea;

	for (int i = 0; i < ranked.Count(); ++i) {

		const HydroBasin & basin = b.Basins[ranked[i]];
		bool taken = false;
		for (int body = 0; body < b.Out->Bodies.Count(); ++body) {
			if (b.Out->Bodies[body].BasinLabel == basin.Label) taken = true;
		}
		if (taken) continue;

		const float before	= (float)fabs((double)(area - target));
		const float after		= (float)fabs((double)((area + basin.RingArea) - target));
		if (after >= before) break;

		HybridWaterBody candidate;
		if (!Try_Basin_Lake(b,basin,false,&candidate)) continue;
		candidate.StableId = b.NextStableId++;
		b.Out->Bodies.Add(candidate);
		area += candidate.Area;
		b.Out->OptionalLakes++;
	}

	return true;
}


/*
**	Step 15: exterior coast water, only where the archetype requires it.  The polygon is the world
**	rectangle clipped to the sea side of the guide's strategic `v` boundary, and a required coast that
**	starves a heavy corridor is moved outward by exactly `S` per local retry.
*/
static bool Build_Coast(HydrologyBuilder & b,HybridHydrologyResult * result)
{
	const HybridArchetypeIntent * intent = b.Intents->Find_First(HYBRID_INTENT_COAST_GUIDE);
	if ((intent == nullptr) || !b.P->Coastline) return true;

	const HybridStrategicFrame & frame = b.Identity->Frame;
	const float sign	= (float)((intent->VSign >= 0) ? 1 : -1);
	const float scale	= (frame.Scale > 0.0f) ? frame.Scale : 1.0f;
	const int retries	= (b.P->MaxLocalRetriesPerStage > 0) ? b.P->MaxLocalRetriesPerStage : 0;

	DynamicVectorClass<Vector2> rectangle;
	rectangle.Add(Vector2(b.P->WorldMinX,b.P->WorldMinY));
	rectangle.Add(Vector2(b.P->WorldMinX + b.P->WorldWidth,b.P->WorldMinY));
	rectangle.Add(Vector2(b.P->WorldMinX + b.P->WorldWidth,b.P->WorldMinY + b.P->WorldHeight));
	rectangle.Add(Vector2(b.P->WorldMinX,b.P->WorldMinY + b.P->WorldHeight));

	float boundary = intent->Guide.Y;

	for (int retry = 0; retry <= retries; ++retry) {

		if (retry > 0) {
			boundary += sign * (b.P->Constants.S / scale);
			b.Out->LocalRetries++;
		}

		//	Keep the half plane `sign * v >= sign * boundary`, which is the sea side.
		const Vector2 normal((-sign / scale) * frame.AxisV.X,(-sign / scale) * frame.AxisV.Y);
		const float offset = (-sign * boundary) + ((normal.X * frame.Center.X) + (normal.Y * frame.Center.Y));

		HybridWaterBody body;
		body.Kind				= HYBRID_WATER_BODY_COAST;
		body.Required			= true;
		body.IntentStableId	= intent->StableId;
		body.SurfaceHeight	= Hybrid_Quantize_Position(b.P->SeaLevel);
		body.BedDepth			= 0.0f;					// ocean and coast carve nothing (Section 11)
		body.Retries			= retry;

		DynamicVectorClass<Vector2> clipped;
		Clip_Convex_Ring(rectangle,normal,offset,&clipped);
		for (int i = 0; i < clipped.Count(); ++i) body.Points.Add(clipped[i]);
		Finish_Body_Record(b,&body);

		if (body.Points.Count() < 3)							{ b.LastReject = "coast clipped away"; continue; }
		if (!Ring_Clears_Water_Reserves(b,body.Points))	{ b.LastReject = "coast reaches a no-water reserve"; continue; }
		if (!Corridor_Bypass_Survives(b,&body.Points))	{ b.LastReject = "coast starves a heavy corridor"; continue; }

		body.StableId = b.NextStableId++;
		b.Out->Bodies.Add(body);
		return true;
	}

	StringClass detail;
	detail.Format("required coast survived no local retry (%s)",b.LastReject);
	return Fail(result,HYBRID_HYDROLOGY_COAST_UNPLACEABLE,detail.Peek_Buffer());
}


/*
**	Step 16: bind each river mouth to the sink it reaches.  A river that enters a selected lake or the
**	sea ends at that shoreline -- the sample that first falls inside the polygon -- so the channel
**	never runs through standing water; a river that reaches none ends at its legal exterior D8 sink,
**	which selection already required it to have.
*/
static void Bind_River_Mouths(HydrologyBuilder & b)
{
	for (int i = 0; i < b.Out->Rivers.Count(); ++i) {

		HybridRiverSystem & river = b.Out->Rivers[i];
		int shore = -1;
		unsigned int body_id = 0;

		for (int point = river.Points.Count() - 1; point >= 0; --point) {
			unsigned int found = 0;
			if (!Hybrid_Point_In_Standing_Water(*b.Out,river.Points[point],&found,nullptr)) {
				shore = point;
				break;
			}
			body_id = found;
		}

		if ((body_id == 0) || (shore < 0) || (shore == (river.Points.Count() - 1))) continue;

		//	Truncate to the shoreline sample, which becomes the mouth.
		while (river.Points.Count() > (shore + 1)) {
			const int last = river.Points.Count() - 1;
			river.Points.Delete(last);
			river.Stations.Delete(last);
			river.Widths.Delete(last);
			river.Accumulation.Delete(last);
			river.Surfaces.Delete(last);
		}

		river.SinkBodyId	= body_id;
		river.Mouth			= river.Points[river.Points.Count() - 1];
		river.Length		= river.Stations[river.Stations.Count() - 1];
		river.MaxAccumulation = river.Accumulation[river.Accumulation.Count() - 1];

		/*
		**	The mouth sample sits on the shoreline, so Phase 7 step 2's "final mouth sample equals the
		**	sink water level" is already true of the record: force it here so no later phase has to.
		*/
		const HybridWaterBody * sink = b.Out->Find_Body(body_id);
		if (sink != nullptr) {
			river.Surfaces[river.Surfaces.Count() - 1] = sink->SurfaceHeight;
		}
	}
}


/*
**	Step 17: the optional decorative islands.  Rejection of a slot is never a generation failure, so
**	every refusal only counts and moves on.
*/
static void Build_Islands(HydrologyBuilder & b)
{
	const float w = b.P->Constants.W;
	b.Out->IslandSlots = b.Identity->Island_Slot_Count_Accepted();
	if (b.Out->IslandSlots <= 0) return;

	const float island_area		= Hybrid_Island_Min_Area(w,b.WorldArea);
	const float host_minimum	= ISLAND_HOST_AREA_FACTOR * Max2(ISLAND_HOST_AREA_W * w * ISLAND_HOST_AREA_W * w,
																				ISLAND_HOST_AREA_FRACTION * b.WorldArea);
	const float clearance		= ISLAND_SHORE_CLEARANCE_W * w;

	for (int slot = 0; slot < b.Identity->IslandSlotCount; ++slot) {

		if (!b.Identity->Island_Slot_Accepted(slot)) continue;

		//	Host: the exterior sea when there is one, otherwise the largest qualifying true lake.
		int host = -1;
		for (int i = 0; i < b.Out->Bodies.Count(); ++i) {
			if (b.Out->Bodies[i].Kind != HYBRID_WATER_BODY_COAST) continue;
			host = i;
			break;
		}
		if (host < 0) {
			float best = 0.0f;
			for (int i = 0; i < b.Out->Bodies.Count(); ++i) {
				const HybridWaterBody & body = b.Out->Bodies[i];
				if (body.Kind != HYBRID_WATER_BODY_LAKE) continue;
				if (body.Area < host_minimum) continue;
				if (body.Area <= best) continue;
				best	= body.Area;
				host	= i;
			}
		}
		if (host < 0) { b.Out->IslandsRejected++; continue; }

		const HybridWaterBody & host_body = b.Out->Bodies[host];

		//	Radius multipliers and the radius that makes the polygon exactly the qualified area.
		float multipliers[GUIDE_POLYGON_RADIALS];
		float shape = 0.0f;
		for (int i = 0; i < GUIDE_POLYGON_RADIALS; ++i) {
			multipliers[i] = Hybrid_Range(ISLAND_RADIUS_MIN,ISLAND_RADIUS_MAX,
													Hydro_Hash(b,"island_radial",(unsigned int)slot,(unsigned int)i),
													HYBRID_QUANTUM_NORMALIZED);
		}
		for (int i = 0; i < GUIDE_POLYGON_RADIALS; ++i) {
			shape += multipliers[i] * multipliers[(i + 1) % GUIDE_POLYGON_RADIALS];
		}
		const float unit_area = 0.5f * (float)sin(2.0 * 3.14159265358979323846 / (double)GUIDE_POLYGON_RADIALS) * shape;
		if (unit_area <= 0.0f) { b.Out->IslandsRejected++; continue; }
		const float radius = (float)sqrt((double)(island_area / unit_area));

		/*
		**	The first stable interior candidate at least `6W` from the host shoreline.  Candidates are
		**	the host's own interior analysis points in keyed order, so the choice is a draw and not a
		**	scan artifact.
		*/
		DynamicVectorClass<Vector2> candidates;
		DynamicVectorClass<float> keys;
		for (float y = host_body.BoundsMin.Y; y <= host_body.BoundsMax.Y; y += b.A) {
			for (float x = host_body.BoundsMin.X; x <= host_body.BoundsMax.X; x += b.A) {
				const Vector2 point(Hybrid_Quantize_Position(x),Hybrid_Quantize_Position(y));
				if (!Point_In_Ring(host_body.Points,point)) continue;
				if (Distance_To_Ring(host_body.Points,point) < (clearance + radius)) continue;
				candidates.Add(point);
				keys.Add(Hydro_Hash(b,"island_site",(unsigned int)slot,(unsigned int)candidates.Count()));
			}
		}
		Note_Samples(b,candidates.Count());
		if (candidates.Count() == 0) { b.Out->IslandsRejected++; continue; }

		int best_at = 0;
		for (int i = 1; i < candidates.Count(); ++i) {
			if (keys[i] < keys[best_at]) best_at = i;
		}
		const Vector2 centre = candidates[best_at];

		HybridWaterBody island;
		island.Kind			= HYBRID_WATER_BODY_ISLAND;
		island.HostBodyId	= host_body.StableId;
		island.SurfaceHeight= host_body.SurfaceHeight;
		island.BedDepth	= 0.0f;
		island.ShoreBlend	= ISLAND_BLEND_W * w;

		for (int i = 0; i < GUIDE_POLYGON_RADIALS; ++i) {
			const float angle = (2.0f * 3.14159265358979323846f * (float)i) / (float)GUIDE_POLYGON_RADIALS;
			const float r = radius * multipliers[i];
			island.Points.Add(Vector2(Hybrid_Quantize_Position(centre.X + (r * (float)cos((double)angle))),
											  Hybrid_Quantize_Position(centre.Y + (r * (float)sin((double)angle)))));
		}
		Finish_Body_Record(b,&island);

		bool inside = (island.Area + 0.0005f >= island_area);
		for (int i = 0; inside && (i < island.Points.Count()); ++i) {
			if (!Point_In_Ring(host_body.Points,island.Points[i])) inside = false;
		}
		if (!inside) { b.Out->IslandsRejected++; continue; }

		/*
		**	Phase 7's island bias: enough lift to stand the island's centre above its host's surface by
		**	half a tier, and never less than half a tier of lift at all.
		*/
		const float land = Hybrid_Provisional_Height(*b.P,*b.Matrices,*b.Landforms,centre);
		island.HeightBias = Hybrid_Quantize_Position(Max2(0.5f * b.P->TierStep,
																		  island.SurfaceHeight + (0.5f * b.P->TierStep) - land));

		island.StableId = b.NextStableId++;
		b.Out->Bodies.Add(island);
	}
}


/***********************************************************************************************
**	Steps 18 to 21: the crossing/pass windows, the semantic stamps and the planning spatial hash.
***********************************************************************************************/

/*
**	One obstruction to sample for windows: a selected river, or an explicit Phase-5 cliff, canyon or
**	ravine.  Ridges are not in the plan's step-18 list and get no windows; lakes and coast are
**	excluded by step 20.
*/
struct HydroObstruction
{
	HybridObstructionKind	Kind;
	HybridWindowKind			Window;
	unsigned int				RecordId;
	const HybridRiverSystem *	River;		// null for a landform
	const HybridLandform *		Landform;	// null for a river
};


static float Max_Grade_Radians(HybridRoadClass road_class)
{
	return Hybrid_Road_Class_Spec(road_class).MaxGradeDegrees * (3.14159265358979323846f / 180.0f);
}


static float Max_Cross_Slope_Radians(HybridRoadClass road_class)
{
	return Hybrid_Road_Class_Spec(road_class).MaxCrossSlopeDegrees * (3.14159265358979323846f / 180.0f);
}


//	The obstruction's local half width at a sample, which is what an approach has to start beyond.
static float Obstruction_Half_Span(const HydroObstruction & obstruction,int index)
{
	if (obstruction.River != nullptr) return 0.5f * obstruction.River->Width_At_Index(index);

	const HybridLandform & landform = *obstruction.Landform;
	switch (landform.Kind) {
		case HYBRID_LANDFORM_CANYON:
		case HYBRID_LANDFORM_RAVINE:	return landform.RimHalfWidth;
		case HYBRID_LANDFORM_CLIFF:	return (0.5f * landform.EdgeWidth) + landform.TalusWidth;
		default:								break;
	}
	return landform.Influence_Half_Width();
}


/*
**	Is this approach sample legal?  Step 18's approach rule: continuous non-water, non-hard-wall
**	ground that never enters `BASE_PROTECTED` or a Tiberium hard reserve.  The obstruction being
**	crossed is exempt from the wall test -- that is what the window is for -- and so is the crossed
**	river's own channel, which the approach starts outside of anyway.
*/
static bool Approach_Sample_Is_Legal(HydrologyBuilder & b,const HydroObstruction & obstruction,
												  const Vector2 & point)
{
	const HybridSemanticMatrix & matrix = b.Matrices->Matrix;

	int cx = 0, cy = 0;
	if (!b.Coarse_Cell(point,&cx,&cy))												return false;
	if (b.Matrices->Is_Base_Protected(cx,cy))										return false;
	if (matrix.Has_Flag(cx,cy,HYBRID_FLAG_TIBERIUM_RESERVE))					return false;
	if (Hybrid_Point_In_Standing_Water(*b.Out,point,nullptr,nullptr))		return false;

	//	Another river's channel is water, so an approach may not run along it.
	for (int i = 0; i < b.Out->Rivers.Count(); ++i) {
		const HybridRiverSystem & river = b.Out->Rivers[i];
		if (river.StableId == obstruction.RecordId) continue;
		int index = -1;
		const float distance = Distance_To_Polyline(river.Points,point,nullptr,&index);
		if (distance <= (0.5f * river.Width_At_Index(index))) return false;
	}

	//	Another immutable wall is a hard obstruction, so it is not an approach either.
	for (int i = 0; i < b.Landforms->Landforms.Count(); ++i) {
		const HybridLandform & landform = b.Landforms->Landforms[i];
		if (!Hybrid_Landform_Is_Immutable(landform.Kind)) continue;
		if (landform.StableId == obstruction.RecordId) continue;
		if ((point.X < landform.BoundsMin.X) || (point.X > landform.BoundsMax.X) ||
			 (point.Y < landform.BoundsMin.Y) || (point.Y > landform.BoundsMax.Y)) continue;
		if (Hybrid_Landform_Contribution(landform,point,nullptr) != 0.0f) return false;
	}
	return true;
}


//	Does a `PRIMARY_2` swept path across the obstruction obey grade and cross slope (`pass_capable`)?
/*
**	The provisional height field, as the local-pass builder wants it.  Phase 6 has no realized terrain yet,
**	so this is the same provisional profile every other Phase-6 rule reads.
*/
struct ProvisionalHeightContext
{
	HydrologyBuilder *	B;
};


static bool Provisional_Height_Sample(const Vector2 & world,float * height_out,void * context)
{
	ProvisionalHeightContext * ground = (ProvisionalHeightContext *)context;
	if ((ground == nullptr) || (height_out == nullptr)) return false;
	*height_out = Hybrid_Provisional_Height(*ground->B->P,*ground->B->Matrices,*ground->B->Landforms,world);
	return true;
}


static HybridLocalPassParams Local_Pass_Params(const HydrologyBuilder & b)
{
	HybridLocalPassParams params;
	params.Constants					= b.P->Constants;
	params.MaxCutDepth				= b.P->MaxCutDepth;
	params.MaxFillHeight				= b.P->MaxFillHeight;
	params.SweptVehicleWidth		= Max2(b.P->HeavyVehicleWidth,b.P->HarvesterWidth);
	params.MinVehicleLaneClearance= b.P->MinVehicleLaneClearance;
	params.PassClass					= HYBRID_ROAD_PRIMARY_2;		// step 18 proves a pass for PRIMARY_2
	return params;
}


static bool Pass_Path_Is_Legal(HydrologyBuilder & b,const Vector2 & from,const Vector2 & to)
{
	const HybridConstants & k = b.P->Constants;
	const float step		= Min2(k.W,0.5f * k.A);
	const float grade		= (float)tan((double)Max_Grade_Radians(HYBRID_ROAD_PRIMARY_2));
	const float cross		= (float)tan((double)Max_Cross_Slope_Radians(HYBRID_ROAD_PRIMARY_2));
	const float half		= 0.5f * Hybrid_Roadbed_Width(HYBRID_ROAD_PRIMARY_2,k.W);

	Vector2 axis = to - from;
	const float length = axis.Length();
	if ((length <= 0.0f) || (step <= 0.0f)) return false;
	axis *= (1.0f / length);
	const Vector2 normal(-axis.Y,axis.X);

	float previous	= 0.0f;
	bool	first		= true;
	int	samples	= 0;

	for (float along = 0.0f; along <= length; along += step) {

		const Vector2 point = from + (axis * along);
		const float height = Hybrid_Provisional_Height(*b.P,*b.Matrices,*b.Landforms,point);
		samples++;

		if (!first && ((float)fabs((double)(height - previous)) > (grade * step))) {
			Note_Samples(b,samples);
			return false;
		}
		previous	= height;
		first		= false;

		const float left	= Hybrid_Provisional_Height(*b.P,*b.Matrices,*b.Landforms,point + (normal * half));
		const float right	= Hybrid_Provisional_Height(*b.P,*b.Matrices,*b.Landforms,point - (normal * half));
		samples += 2;
		if ((float)fabs((double)(left - right)) > (cross * 2.0f * half)) {
			Note_Samples(b,samples);
			return false;
		}
	}
	Note_Samples(b,samples);
	return true;
}


//	Step 18's `preferred` rule: on a required heavy corridor, or within `2S` of a crossing guide.
static bool Window_Is_Preferred(HydrologyBuilder & b,const Vector2 & centre)
{
	const HybridConstants & k = b.P->Constants;

	for (int r = 0; r < b.Matrices->Reservations.Count(); ++r) {
		const HybridReservation & reservation = b.Matrices->Reservations[r];
		if (reservation.Priority != HYBRID_RESERVE_HEAVY_CORRIDOR) continue;
		if (Distance_To_Segment(centre,reservation.A,reservation.B,nullptr) <= reservation.Radius) return true;
	}

	for (int o = 0; o < b.Graph->Obligations.Count(); ++o) {
		const HybridCrossingObligation & obligation = b.Graph->Obligations[o];
		const Vector2 guide = b.Identity->Frame.To_World(obligation.Guide);
		if ((centre - guide).Length() <= (OBLIGATION_PREFERENCE_S * k.S)) return true;
	}
	return false;
}


/*
**	Can a router actually step off this station onto the grid, on both banks?
**
**	Step 18's approach rule is written in world units -- `8W` of continuous non-water, non-hard-wall
**	approach on both sides -- and a station can satisfy it along its own axis while every *analysis cell*
**	around it is refused, because the dry strip is narrower than the grid the router moves on.  Phase 10
**	then has a frozen waypoint it cannot enter or leave, and the whole route family fails.  Measured on
**	the failing seed that was the case for two windows a lake ran alongside.
**
**	So the station also has to have, on each bank, at least one legal eight-neighbour cell of its own
**	analysis cell.  That is the smallest statement of "a road can use this window" in the terms Phase 10
**	actually works in.
*/
static bool Window_Has_Grid_Access(HydrologyBuilder & b,const HydroObstruction & obstruction,
											  const Vector2 & centre,const Vector2 & normal)
{
	int cx = 0, cy = 0;
	{
		const float fx = (centre.X - b.OriginX) / b.A;
		const float fy = (centre.Y - b.OriginY) / b.A;
		cx = (int)::floor((double)fx);
		cy = (int)::floor((double)fy);
	}
	if (!b.Valid_Cell(cx,cy)) return false;

	bool positive = false;
	bool negative = false;

	for (int n = 0; n < 8; ++n) {

		const int nx = cx + NEIGHBOR_DX[n];
		const int ny = cy + NEIGHBOR_DY[n];
		if (!b.Valid_Cell(nx,ny)) continue;

		const Vector2 point = b.Cell_Center(b.Cell_Index(nx,ny));
		if (!Approach_Sample_Is_Legal(b,obstruction,point)) continue;

		const Vector2 offset = point - centre;
		const float side = (offset.X * normal.X) + (offset.Y * normal.Y);
		if (side > 0.0f)			positive = true;
		else if (side < 0.0f)	negative = true;
	}
	return positive && negative;
}


/*
**	Step 18 for one obstruction: sample its centerline every `A` and keep the stations whose two
**	approaches are legal, whose endpoints can be connected within `PRIMARY_2` grade, and which at
**	least one method can actually serve.
*/
static void Sample_Obstruction_Windows(HydrologyBuilder & b,const HydroObstruction & obstruction,
													  const DynamicVectorClass<Vector2> & points,
													  const DynamicVectorClass<float> & stations,
													  DynamicVectorClass<HybridCrossingWindow> * out)
{
	const HybridConstants & k = b.P->Constants;
	const float approach	= Max2(APPROACH_LENGTH_W * k.W,b.P->BridgeApproachLength);
	const float step		= 0.5f * k.A;
	const float grade		= (float)tan((double)Max_Grade_Radians(HYBRID_ROAD_PRIMARY_2));
	const float length	= (stations.Count() > 0) ? stations[stations.Count() - 1] : 0.0f;
	if ((points.Count() < 2) || (length <= 0.0f)) return;

	float next_station = 0.0f;

	for (int index = 0; index < points.Count(); ++index) {

		if (stations[index] < next_station) continue;
		next_station = stations[index] + k.A;
		b.Out->StationsConsidered++;
		if (b.P->Counters != nullptr) b.P->Counters->Add(HYBRID_WORK_CROSSING_STATIONS,1);

		//	Local frame: the tangent along the obstruction, and the crossing normal.
		const int previous	= (index > 0) ? (index - 1) : index;
		const int following	= (index < (points.Count() - 1)) ? (index + 1) : index;
		Vector2 tangent = points[following] - points[previous];
		const float tangent_length = tangent.Length();
		if (tangent_length <= 0.0f) continue;
		tangent *= (1.0f / tangent_length);
		const Vector2 normal(-tangent.Y,tangent.X);

		const Vector2 centre	= points[index];
		const float half		= Obstruction_Half_Span(obstruction,index);
		const Vector2 side_a	= centre + (normal * half);
		const Vector2 side_b	= centre - (normal * half);
		const Vector2 end_a	= centre + (normal * (half + approach));
		const Vector2 end_b	= centre - (normal * (half + approach));

		/*
		**	Both approaches must be continuously legal over their whole `8W` run -- and over the *width* the
		**	road needs, not only along the axis.  A window whose approach is a dry line through a lake is a
		**	window Phase 10 cannot actually enter: it routes on the analysis grid, so a cell centre half a
		**	roadbed off the axis has to be legal too.  Measured on the failing seed, a crossing obligation
		**	on such a window left its frozen waypoint enclosed by water on every side.
		*/
		const float lateral_half = 0.5f * Hybrid_Roadbed_Width(HYBRID_ROAD_PRIMARY_2,k.W);
		bool legal = true;
		for (float out_of = step; (out_of <= approach) && legal; out_of += step) {
			for (int lateral_index = -1; (lateral_index <= 1) && legal; ++lateral_index) {

				const Vector2 offset = tangent * ((float)lateral_index * lateral_half);
				if (!Approach_Sample_Is_Legal(b,obstruction,side_a + (normal * out_of) + offset))	legal = false;
				if (!Approach_Sample_Is_Legal(b,obstruction,side_b - (normal * out_of) + offset))	legal = false;
			}
		}
		if (!legal) continue;
		if (!Window_Has_Grid_Access(b,obstruction,centre,normal)) continue;

		const float height_a = Hybrid_Provisional_Height(*b.P,*b.Matrices,*b.Landforms,end_a);
		const float height_b = Hybrid_Provisional_Height(*b.P,*b.Matrices,*b.Landforms,end_b);
		const float span_distance = (end_a - end_b).Length();
		if (span_distance <= 0.0f) continue;
		if ((float)fabs((double)(height_a - height_b)) > (grade * span_distance)) continue;

		const float span = 2.0f * half;
		unsigned int methods = 0;
		HybridLocalPassGeometry pass_geometry;

		//	`bridge_capable`: a registered kind covers the span and clear width, and its approach fits.
		if ((b.P->BridgeMaxSpanLength >= span) && (b.P->BridgeRequiredWidth > 0.0f)) {
			int cx = 0, cy = 0;
			bool reserve = b.Coarse_Cell(end_a,&cx,&cy) && b.Matrices->Matrix.Has_Flag(cx,cy,HYBRID_FLAG_PLACEABLE);
			if (reserve) {
				reserve = b.Coarse_Cell(end_b,&cx,&cy) && b.Matrices->Matrix.Has_Flag(cx,cy,HYBRID_FLAG_PLACEABLE);
			}
			if (reserve) methods |= HYBRID_WINDOW_METHOD_BRIDGE;
		}

		if (obstruction.Kind == HYBRID_OBSTRUCTION_RIVER) {

			/*
			**	Fords.  `AllowFords` is the profile switch; `FordsAvailable` is preflight's answer about
			**	the runtime capability, which is false until a fordability query exists, and the two
			**	depths are that query's limits.  The water this crossing would drive through is the
			**	river's own bed depth.
			*/
			if (b.P->AllowFords && b.P->FordsAvailable) {
				const float depth = obstruction.River->BedDepth;
				if ((b.P->FordMaxWaterDepthLight > 0.0f) && (depth <= b.P->FordMaxWaterDepthLight)) {
					methods |= HYBRID_WINDOW_METHOD_FORD_LIGHT;
				}
				if ((b.P->FordMaxWaterDepthHarvester > 0.0f) && (depth <= b.P->FordMaxWaterDepthHarvester)) {
					methods |= HYBRID_WINDOW_METHOD_FORD_HARVESTER;
				}
			}

		} else {

			/*
			**	`pass_capable`: the plan's "sampled `PRIMARY_2` swept path across the local floor/ramp".  That is
			**	not the straight line between banks -- a wall too steep to climb straight down is routinely easy
			**	to climb at an angle, and testing the straight line made every pass window on every seed
			**	incapable.  The shared local-pass builder constructs the diagonal the class grade actually
			**	requires, judges it as road, and the geometry it accepts is frozen onto the record for Phase 11
			**	to revalidate rather than reinvent.
			*/
			{
				const HybridLandform * landform = obstruction.Landform;
				const bool cliff = (obstruction.Kind == HYBRID_OBSTRUCTION_CLIFF);
				const HybridLocalPassKind pass_kind = cliff ? HYBRID_LOCAL_PASS_RAMP
																	  : HYBRID_LOCAL_PASS_FLOOR;
				const float floor_half = ((landform != nullptr) && !cliff) ? landform->FloorHalfWidth : 0.0f;
				const float rim_half = (landform != nullptr) ? Max2(landform->RimHalfWidth,half) : half;

				ProvisionalHeightContext ground;
				ground.B = &b;

				HybridLocalPassFailure why = HYBRID_LOCAL_PASS_OK;
				/*
				**	How much along-obstruction run this station may spend: half the feature's own length, so a
				**	descent and an ascent both fit on it.  A ramp cannot lean further along a feature than the
				**	feature is long, and that -- not an invented multiple of the lane width -- is the bound.
				*/
				const float available_run = (landform != nullptr) ? (0.5f * landform->Length)
																			 : (12.0f * k.W);

				if (Hybrid_Build_Local_Pass(Local_Pass_Params(b),pass_kind,centre,tangent,normal,half,
													 floor_half,rim_half,approach,available_run,
													 Provisional_Height_Sample,&ground,&pass_geometry,&why)) {
					methods |= HYBRID_WINDOW_METHOD_PASS;
				} else if ((why > HYBRID_LOCAL_PASS_OK) && (why < HYBRID_LOCAL_PASS_FAILURE_COUNT)) {
					b.Out->PassRefusals[why]++;
				}
			}

			if (b.P->TunnelsAvailable && (b.P->TunnelRequiredWidth >= b.P->HeavyVehicleWidth) &&
				 (b.P->TunnelRequiredHeight >= b.P->HeavyVehicleHeight)) {
				methods |= HYBRID_WINDOW_METHOD_TUNNEL;
			}

			/*
			**	A cliff station that survived the approach and grade rules is exactly the cut Phase 11
			**	tries first, so the cut metadata the plan asks for is that fact plus the two endpoint
			**	elevations already stored on the record.
			*/
			if (obstruction.Kind == HYBRID_OBSTRUCTION_CLIFF) methods |= HYBRID_WINDOW_METHOD_CUT;
		}

		//	A river station survives only when a bridge or a ford can serve it (step 18).
		if (obstruction.Kind == HYBRID_OBSTRUCTION_RIVER) {
			const unsigned int usable = HYBRID_WINDOW_METHOD_BRIDGE | HYBRID_WINDOW_METHOD_FORD_LIGHT |
												 HYBRID_WINDOW_METHOD_FORD_HARVESTER;
			if ((methods & usable) == 0) continue;
		} else if (methods == 0) {
			continue;
		}

		HybridCrossingWindow window;
		window.Kind					= obstruction.Window;
		window.Obstruction		= obstruction.Kind;
		window.ObstructionRecordId = obstruction.RecordId;
		window.Methods				= methods;
		if ((methods & HYBRID_WINDOW_METHOD_PASS) != 0) window.Pass = pass_geometry;
		window.Station				= stations[index];
		window.Span					= Hybrid_Quantize_Position(span);
		window.ApproachLength	= Hybrid_Quantize_Position(approach);
		window.LowElevation		= Hybrid_Quantize_Position(Min2(height_a,height_b));
		window.HighElevation		= Hybrid_Quantize_Position(Max2(height_a,height_b));
		window.Center.Set(Hybrid_Quantize_Position(centre.X),Hybrid_Quantize_Position(centre.Y));
		window.Tangent.Set(Hybrid_Quantize_Normalized(tangent.X),Hybrid_Quantize_Normalized(tangent.Y));
		window.ApproachA.Set(Hybrid_Quantize_Position(end_a.X),Hybrid_Quantize_Position(end_a.Y));
		window.ApproachB.Set(Hybrid_Quantize_Position(end_b.X),Hybrid_Quantize_Position(end_b.Y));
		window.Preferred			= Window_Is_Preferred(b,window.Center);
		window.BoundsMin.Set(Min2(window.ApproachA.X,window.ApproachB.X),
									Min2(window.ApproachA.Y,window.ApproachB.Y));
		window.BoundsMax.Set(Max2(window.ApproachA.X,window.ApproachB.X),
									Max2(window.ApproachA.Y,window.ApproachB.Y));
		out->Add(window);
	}
}


/*
**	Steps 18 and 19: every window in one pass over the fixed water and the explicit landforms, then
**	the `<4W` merge that keeps `preferred` over plain and the lower record otherwise.
*/
static void Build_Windows(HydrologyBuilder & b)
{
	DynamicVectorClass<HybridCrossingWindow> candidates;

	for (int i = 0; i < b.Out->Rivers.Count(); ++i) {
		const HybridRiverSystem & river = b.Out->Rivers[i];
		HydroObstruction obstruction;
		obstruction.Kind		= HYBRID_OBSTRUCTION_RIVER;
		obstruction.Window	= HYBRID_WINDOW_CROSSING;
		obstruction.RecordId	= river.StableId;
		obstruction.River		= &river;
		obstruction.Landform	= nullptr;
		Sample_Obstruction_Windows(b,obstruction,river.Points,river.Stations,&candidates);
	}

	for (int i = 0; i < b.Landforms->Landforms.Count(); ++i) {

		const HybridLandform & landform = b.Landforms->Landforms[i];
		HydroObstruction obstruction;
		obstruction.River		= nullptr;
		obstruction.Landform	= &landform;
		obstruction.RecordId	= landform.StableId;

		switch (landform.Kind) {
			case HYBRID_LANDFORM_CANYON:
				obstruction.Kind		= HYBRID_OBSTRUCTION_CANYON;
				obstruction.Window	= HYBRID_WINDOW_CROSSING;
				break;
			case HYBRID_LANDFORM_RAVINE:
				obstruction.Kind		= HYBRID_OBSTRUCTION_RAVINE;
				obstruction.Window	= HYBRID_WINDOW_CROSSING;
				break;
			case HYBRID_LANDFORM_CLIFF:
				obstruction.Kind		= HYBRID_OBSTRUCTION_CLIFF;
				obstruction.Window	= HYBRID_WINDOW_RAMP_PASS;
				break;
			default:
				continue;					// hills and ridges are not step-18 obstructions
		}
		Sample_Obstruction_Windows(b,obstruction,landform.Points,landform.Stations,&candidates);
	}

	//	Step 19's merge, per obstruction, in station order -- which is already the creation order.
	const float merge = WINDOW_MERGE_W * b.P->Constants.W;

	for (int i = 0; i < candidates.Count(); ++i) {

		HybridCrossingWindow keep = candidates[i];
		int folded = 1;

		while ((i + 1) < candidates.Count()) {

			const HybridCrossingWindow & next = candidates[i + 1];
			if (next.ObstructionRecordId != keep.ObstructionRecordId) break;
			if (next.Obstruction != keep.Obstruction) break;
			if ((next.Station - keep.Station) >= merge) break;

			//	`preferred` beats plain; otherwise the earlier record keeps its ground.
			if (next.Preferred && !keep.Preferred) {
				const int hold_count = folded;
				keep = next;
				folded = hold_count;
			}
			keep.Methods |= next.Methods;
			folded++;
			i++;
			b.Out->StationsMerged++;
		}

		keep.MergedCount	= folded;
		keep.StableId		= b.NextStableId++;
		b.Out->Windows.Add(keep);
	}
}


/*
**	Step 19's semantic stamp and step 21's spatial hash.  The accepted water becomes water intent,
**	each island clears it again, every window's approach reserve becomes `NO_DECORATION` with its own
**	window flag, and a window on a required heavy corridor also becomes a Phase-4
**	`HYBRID_RESERVE_CROSSING_WINDOW` reservation -- the priority Phase 4 left in its list with nothing
**	to insert yet.
*/
static void Stamp_And_Insert(HydrologyBuilder & b)
{
	HybridSemanticMatrix & matrix = b.Matrices->Matrix;
	const HybridConstants & k = b.P->Constants;

	for (int i = 0; i < b.Out->Bodies.Count(); ++i) {

		const HybridWaterBody & body = b.Out->Bodies[i];

		for (int cy = 0; cy < matrix.Get_Count_Y(); ++cy) {
			for (int cx = 0; cx < matrix.Get_Count_X(); ++cx) {

				const Vector2 centre = matrix.Cell_Center(cx,cy);
				if ((centre.X < body.BoundsMin.X) || (centre.X > body.BoundsMax.X) ||
					 (centre.Y < body.BoundsMin.Y) || (centre.Y > body.BoundsMax.Y)) continue;
				if (!Point_In_Ring(body.Points,centre)) continue;

				const int index = matrix.Index(cx,cy);
				if (body.Kind == HYBRID_WATER_BODY_ISLAND) {
					matrix.LandWaterIntent[index] = 0;
					matrix.Flags[index] |= HYBRID_FLAG_LAND;
					matrix.Flags[index] &= ~HYBRID_FLAG_WATER_BASIN;
					continue;
				}
				matrix.LandWaterIntent[index] = 1;
				if (body.Kind == HYBRID_WATER_BODY_LAKE)	matrix.Flags[index] |= HYBRID_FLAG_WATER_BASIN;
				else													matrix.Flags[index] |= HYBRID_FLAG_COAST;
			}
		}
		Note_Cells(b,matrix.Get_Cell_Count());

		b.Out->Hash.Insert(body.StableId,body.BoundsMin.X - b.P->WorldMinX,body.BoundsMin.Y - b.P->WorldMinY,
								 body.BoundsMax.X - b.P->WorldMinX,body.BoundsMax.Y - b.P->WorldMinY);
	}

	//	A river channel is water intent, but it is not a basin: it is an obstruction with windows.
	for (int i = 0; i < b.Out->Rivers.Count(); ++i) {

		const HybridRiverSystem & river = b.Out->Rivers[i];
		for (int point = 0; point < river.Points.Count(); ++point) {
			int cx = 0, cy = 0;
			if (!b.Coarse_Cell(river.Points[point],&cx,&cy)) continue;
			matrix.LandWaterIntent[matrix.Index(cx,cy)] = 1;
		}

		b.Out->Hash.Insert(river.StableId,river.BoundsMin.X - b.P->WorldMinX,river.BoundsMin.Y - b.P->WorldMinY,
								 river.BoundsMax.X - b.P->WorldMinX,river.BoundsMax.Y - b.P->WorldMinY);
	}

	const float half_roadbed = 0.5f * Hybrid_Roadbed_Width(HYBRID_ROAD_PRIMARY_2,k.W);

	for (int i = 0; i < b.Out->Windows.Count(); ++i) {

		const HybridCrossingWindow & window = b.Out->Windows[i];
		const unsigned int flag = (window.Kind == HYBRID_WINDOW_CROSSING) ? HYBRID_FLAG_CROSSING_WINDOW
																								: HYBRID_FLAG_RAMP_PASS_WINDOW;

		for (int cy = 0; cy < matrix.Get_Count_Y(); ++cy) {
			for (int cx = 0; cx < matrix.Get_Count_X(); ++cx) {

				const Vector2 centre = matrix.Cell_Center(cx,cy);
				if (Distance_To_Segment(centre,window.ApproachA,window.ApproachB,nullptr) >
					 (half_roadbed + (0.5f * matrix.Get_Cell_Size()))) continue;

				const int index = matrix.Index(cx,cy);
				matrix.NoDecorationMask[index]	= 1;
				matrix.Flags[index]					|= (HYBRID_FLAG_NO_DECORATION | flag);
			}
		}
		Note_Cells(b,matrix.Get_Cell_Count());

		if (window.Preferred) {
			HybridReservation reservation;
			reservation.StableId	= (unsigned int)(b.Matrices->Reservations.Count() + 1);
			reservation.Priority	= HYBRID_RESERVE_CROSSING_WINDOW;
			reservation.A			= window.ApproachA;
			reservation.B			= window.ApproachB;
			reservation.Radius	= half_roadbed;
			reservation.Exclusive= false;
			b.Matrices->Reservations.Add(reservation);
		}

		b.Out->Hash.Insert(window.StableId,window.BoundsMin.X - b.P->WorldMinX,
								 window.BoundsMin.Y - b.P->WorldMinY,window.BoundsMax.X - b.P->WorldMinX,
								 window.BoundsMax.Y - b.P->WorldMinY);
	}
}


/***********************************************************************************************
**	The Phase 6 gate.
***********************************************************************************************/

//	Is this point inside the obstruction itself -- a river's channel, or an immutable wall's field?
static bool Point_Inside_Obstruction(const HybridRiverSystem * river,const HybridLandform * landform,
													  const Vector2 & point)
{
	if (river != nullptr) {
		int index = -1;
		const float distance = Distance_To_Polyline(river->Points,point,nullptr,&index);
		return distance <= (0.5f * river->Width_At_Index(index));
	}
	if (landform == nullptr) return false;
	if ((point.X < landform->BoundsMin.X) || (point.X > landform->BoundsMax.X) ||
		 (point.Y < landform->BoundsMin.Y) || (point.Y > landform->BoundsMax.Y)) return false;
	return Hybrid_Landform_Contribution(*landform,point,nullptr) != 0.0f;
}


/*
**	Does this obstruction actually *obstruct* a required heavy corridor?  A barrier that only clips
**	the edge of the corridor band leaves the road a dry way past it inside
**	`MaxCorridorHalfWidth(PRIMARY_2)`, and the plan's gate is about the corridors a window is the
**	only way through: the ones where some station along the corridor has no contiguous
**	`heavy_vehicle_width + 2*min_vehicle_lane_clearance` run clear of this obstruction.
*/
static bool Obstruction_Blocks_Corridor(HydrologyBuilder & b,const HybridReservation & reservation,
														  const HybridRiverSystem * river,const HybridLandform * landform)
{
	const HybridConstants & k = b.P->Constants;
	const float required	= b.P->HeavyVehicleWidth + (2.0f * b.P->MinVehicleLaneClearance);
	const float half		= Hybrid_Max_Corridor_Half_Width(HYBRID_ROAD_PRIMARY_2,k.S,k.D);
	const float step		= Max2(0.5f * k.W,k.Q);
	if (required <= 0.0f) return false;

	Vector2 axis = reservation.B - reservation.A;
	const float axis_length = axis.Length();
	if (axis_length <= 0.0f) return false;
	axis *= (1.0f / axis_length);
	const Vector2 normal(-axis.Y,axis.X);

	for (float along = 0.0f; along <= axis_length; along += k.A) {

		const Vector2 centre = reservation.A + (axis * along);
		float best	= 0.0f;
		float run	= 0.0f;

		for (float lateral = -half; lateral <= half; lateral += step) {
			const Vector2 point = centre + (normal * lateral);
			if (Point_Inside_Obstruction(river,landform,point)) {
				run = 0.0f;
			} else {
				run	= run + step;
				best	= Max2(best,run);
			}
		}
		if (best < required) return true;
	}
	return false;
}


static bool Check_Gate(HydrologyBuilder & b,HybridHydrologyResult * result)
{
	const HybridSemanticMatrix & matrix = b.Matrices->Matrix;
	StringClass detail;

	//	1 -- Section 5 river and true-lake quotas.
	if (b.Out->Rivers.Count() < b.Out->RequiredRivers) {
		detail.Format("%d of %d required river systems",b.Out->Rivers.Count(),b.Out->RequiredRivers);
		return Fail(result,HYBRID_HYDROLOGY_RIVER_QUOTA,detail.Peek_Buffer());
	}
	if (b.P->Lakes) {
		const int lakes = b.Out->Count_Of_True_Lakes(b.TrueLakeMinArea);
		if (lakes < b.Out->RequiredLakes) {
			detail.Format("%d of %d required true lakes",lakes,b.Out->RequiredLakes);
			return Fail(result,HYBRID_HYDROLOGY_LAKE_QUOTA,detail.Peek_Buffer());
		}
	}

	//	2 -- every selected river is a continuous, acyclic source-to-sink vector.
	for (int i = 0; i < b.Out->Rivers.Count(); ++i) {

		const HybridRiverSystem & river = b.Out->Rivers[i];
		if ((river.Points.Count() < 2) || (river.Widths.Count() != river.Points.Count()) ||
			 (river.Surfaces.Count() != river.Points.Count()) ||
			 (river.Stations.Count() != river.Points.Count())) {
			detail.Format("river %u has %d points, %d widths, %d surfaces, %d stations",river.StableId,
							  river.Points.Count(),river.Widths.Count(),river.Surfaces.Count(),river.Stations.Count());
			return Fail(result,HYBRID_HYDROLOGY_RECORD_INCOMPLETE,detail.Peek_Buffer());
		}
		for (int point = 1; point < river.Points.Count(); ++point) {
			if (river.Stations[point] > river.Stations[point - 1]) continue;
			detail.Format("river %u station %d does not advance (%.3f after %.3f)",river.StableId,point,
							  river.Stations[point],river.Stations[point - 1]);
			return Fail(result,HYBRID_HYDROLOGY_FLOW_NOT_ACYCLIC,detail.Peek_Buffer());
		}
	}

	for (int i = 0; i < b.Out->Bodies.Count(); ++i) {
		const HybridWaterBody & body = b.Out->Bodies[i];
		if (body.Points.Count() >= 3) continue;
		detail.Format("%s %u has %d ring points",Hybrid_Water_Body_Kind_Name(body.Kind),body.StableId,
						  body.Points.Count());
		return Fail(result,HYBRID_HYDROLOGY_RECORD_INCOMPLETE,detail.Peek_Buffer());
	}

	//	3 -- no water intent inside BASE_PROTECTED or an OPEN_MANEUVER no-water reserve.
	for (int i = 0; i < b.Out->Bodies.Count(); ++i) {

		const HybridWaterBody & body = b.Out->Bodies[i];
		if (body.Kind == HYBRID_WATER_BODY_ISLAND) continue;

		for (float y = body.BoundsMin.Y; y <= body.BoundsMax.Y; y += b.A) {
			for (float x = body.BoundsMin.X; x <= body.BoundsMax.X; x += b.A) {

				const Vector2 point(x,y);
				if (!Point_In_Ring(body.Points,point)) continue;
				int cx = 0, cy = 0;
				if (!b.Coarse_Cell(point,&cx,&cy)) continue;
				if (b.Matrices->Is_Base_Protected(cx,cy)) {
					detail.Format("%s %u reaches BASE_PROTECTED cell %d,%d",Hybrid_Water_Body_Kind_Name(body.Kind),
									  body.StableId,cx,cy);
					return Fail(result,HYBRID_HYDROLOGY_WATER_IN_RESERVE,detail.Peek_Buffer());
				}
				if (matrix.Has_Flag(cx,cy,HYBRID_FLAG_NO_IMMUTABLE_BARRIER)) {
					detail.Format("%s %u reaches OPEN_MANEUVER cell %d,%d",Hybrid_Water_Body_Kind_Name(body.Kind),
									  body.StableId,cx,cy);
					return Fail(result,HYBRID_HYDROLOGY_WATER_IN_RESERVE,detail.Peek_Buffer());
				}
			}
		}
	}

	for (int i = 0; i < b.Out->Rivers.Count(); ++i) {

		const HybridRiverSystem & river = b.Out->Rivers[i];
		for (int point = 0; point < river.Points.Count(); ++point) {

			int cx = 0, cy = 0;
			if (!b.Coarse_Cell(river.Points[point],&cx,&cy)) continue;
			if (b.Matrices->Is_Base_Protected(cx,cy)) {
				detail.Format("river %u reaches BASE_PROTECTED cell %d,%d",river.StableId,cx,cy);
				return Fail(result,HYBRID_HYDROLOGY_WATER_IN_RESERVE,detail.Peek_Buffer());
			}
			if (matrix.Has_Flag(cx,cy,HYBRID_FLAG_NO_IMMUTABLE_BARRIER)) {
				detail.Format("river %u reaches OPEN_MANEUVER cell %d,%d",river.StableId,cx,cy);
				return Fail(result,HYBRID_HYDROLOGY_WATER_IN_RESERVE,detail.Peek_Buffer());
			}
		}
	}

	//	4 -- no lake or coast leaves a required heavy corridor without a dry bypass.
	if (!Corridor_Bypass_Survives(b,nullptr)) {
		return Fail(result,HYBRID_HYDROLOGY_CORRIDOR_BLOCKED,
						"standing water leaves a required heavy corridor no dry bypass");
	}

	//	5 -- every obstructed required corridor has at least one accepted window on that obstruction.
	for (int r = 0; r < b.Matrices->Reservations.Count(); ++r) {

		const HybridReservation & reservation = b.Matrices->Reservations[r];
		if (reservation.Priority != HYBRID_RESERVE_HEAVY_CORRIDOR) continue;

		for (int i = 0; i < b.Out->Rivers.Count(); ++i) {

			const HybridRiverSystem & river = b.Out->Rivers[i];
			if (!Obstruction_Blocks_Corridor(b,reservation,&river,nullptr)) continue;

			bool served = false;
			for (int window = 0; (window < b.Out->Windows.Count()) && !served; ++window) {
				if (b.Out->Windows[window].ObstructionRecordId != river.StableId) continue;
				if (Distance_To_Segment(b.Out->Windows[window].Center,reservation.A,reservation.B,nullptr) >
					 (reservation.Radius + b.P->RiverWidth)) continue;
				served = true;
			}
			if (served) continue;

			detail.Format("river %u blocks heavy corridor %u with no window",river.StableId,reservation.StableId);
			return Fail(result,HYBRID_HYDROLOGY_NO_CROSSING_WINDOW,detail.Peek_Buffer());
		}

		for (int i = 0; i < b.Landforms->Landforms.Count(); ++i) {

			const HybridLandform & landform = b.Landforms->Landforms[i];
			if (!Hybrid_Landform_Is_Immutable(landform.Kind)) continue;
			if (!Obstruction_Blocks_Corridor(b,reservation,nullptr,&landform)) continue;

			bool served = false;
			for (int window = 0; (window < b.Out->Windows.Count()) && !served; ++window) {
				if (b.Out->Windows[window].ObstructionRecordId != landform.StableId) continue;
				if (Distance_To_Segment(b.Out->Windows[window].Center,reservation.A,reservation.B,nullptr) >
					 (reservation.Radius + landform.Influence_Half_Width())) continue;
				served = true;
			}
			if (served) continue;

			detail.Format("%s %u blocks heavy corridor %u with no window",
							  Hybrid_Landform_Kind_Name(landform.Kind),landform.StableId,reservation.StableId);
			return Fail(result,HYBRID_HYDROLOGY_NO_CROSSING_WINDOW,detail.Peek_Buffer());
		}
	}

	return true;
}


/***********************************************************************************************
 * Hybrid_Build_Hydrology -- runtime Phase 6                                                    *
 *=============================================================================================*/
bool Hybrid_Build_Hydrology(const HybridHydrologyParams & params,const HybridMapIdentity & identity,
										const HybridArchetypeIntentSet & intents,const HybridStrategicGraph & graph,
										const HybridLandformSet & landforms,HybridCoarseMatrices * matrices,
										HybridHydrologySet * out,HybridHydrologyResult * result)
{
	if (result != nullptr) {
		result->Failure	= HYBRID_HYDROLOGY_OK;
		result->Detail		= "";
	}
	if ((matrices == nullptr) || (out == nullptr)) {
		return Fail(result,HYBRID_HYDROLOGY_INVALID_INPUT,"no matrices or no output record");
	}

	out->Reset();

	if ((matrices->Matrix.Get_Cell_Count() <= 0) || (params.Constants.A <= 0.0f) ||
		 (params.Constants.S <= 0.0f) || (params.Constants.W <= 0.0f) || (params.Constants.D <= 0.0f) ||
		 (params.WorldWidth <= 0.0f) || (params.WorldHeight <= 0.0f) || (params.TierStep <= 0.0f)) {
		return Fail(result,HYBRID_HYDROLOGY_INVALID_INPUT,"degenerate constants, world rectangle or matrix");
	}
	if (params.RiverWidth <= 0.0f) {
		return Fail(result,HYBRID_HYDROLOGY_INVALID_INPUT,"water.river_width is not positive");
	}

	HydrologyBuilder b;
	b.P				= &params;
	b.Identity		= &identity;
	b.Intents		= &intents;
	b.Graph			= &graph;
	b.Landforms		= &landforms;
	b.Matrices		= matrices;
	b.Out				= out;
	b.A				= params.Constants.A;
	b.OriginX		= params.WorldMinX;
	b.OriginY		= params.WorldMinY;
	b.Height			= nullptr;
	b.HeapCount		= 0;
	b.NextStableId	= 1;
	b.WorldArea		= params.WorldWidth * params.WorldHeight;
	b.TrueLakeMinArea = Hybrid_True_Lake_Min_Area(b.A,b.WorldArea);
	b.Threshold		= Hybrid_River_Threshold(params.RiverAccumulationChunks,params.ChunkSize,b.A);
	b.LastReject	= "none";

	//	Dimensions are ceil(extent/A), computed as an exact integer step so no rounding mode enters.
	b.CountX = (int)(params.WorldWidth / b.A);
	b.CountY = (int)(params.WorldHeight / b.A);
	if (((float)b.CountX * b.A) < params.WorldWidth) b.CountX++;
	if (((float)b.CountY * b.A) < params.WorldHeight) b.CountY++;
	if ((b.CountX < 3) || (b.CountY < 3)) {
		return Fail(result,HYBRID_HYDROLOGY_INVALID_INPUT,"analysis grid is smaller than three cells");
	}
	b.CellCount = b.CountX * b.CountY;

	out->RequiredRivers		= params.Rivers ? Hybrid_Required_River_Count(identity.SizeClass) : 0;
	out->RequiredLakes		= params.Lakes ? Hybrid_Required_Lake_Count(identity.SizeClass) : 0;
	out->WaterFractionTarget= params.WaterFraction;
	out->Hash.Init(params.WorldWidth,params.WorldHeight,params.Constants.BHash);

	if (!Allocate_Arrays(b,result)) { out->Reset(); return false; }

	//	1-5: the provisional surface, the fill, the basins, D8 and accumulation.
	Build_Provisional_Surface(b);
	Priority_Flood(b);
	Label_Basins(b);
	Assign_D8(b);
	Accumulate_Flow(b);

	//	6-12: the channel threshold, the guided river, the selected systems and their centerlines.
	if (params.Rivers) {
		Insert_Guided_River(b);
		if (!Select_Rivers(b,result)) { out->Reset(); return false; }
	} else {
		out->RiverThreshold = b.Threshold;
	}

	//	13-17: the lakes, the water-fraction basins, the coast, the mouths and the islands.
	if (!Build_Lakes(b,result))	{ out->Reset(); return false; }
	if (!Build_Coast(b,result))	{ out->Reset(); return false; }
	Bind_River_Mouths(b);
	Build_Islands(b);

	float standing = 0.0f;
	for (int i = 0; i < out->Bodies.Count(); ++i) {
		if (out->Bodies[i].Kind != HYBRID_WATER_BODY_LAKE) continue;
		standing += out->Bodies[i].Area;
	}
	for (int i = 0; i < out->Bodies.Count(); ++i) {
		if (out->Bodies[i].Kind != HYBRID_WATER_BODY_ISLAND) continue;
		const HybridWaterBody * host = out->Find_Body(out->Bodies[i].HostBodyId);
		if ((host != nullptr) && (host->Kind == HYBRID_WATER_BODY_LAKE)) standing -= out->Bodies[i].Area;
	}
	out->StandingWaterArea		= Max2(0.0f,standing);
	out->StandingWaterFraction	= (b.WorldArea > 0.0f) ? (out->StandingWaterArea / b.WorldArea) : 0.0f;

	//	18-21: the windows, the semantic stamps and the planning spatial hash.
	Build_Windows(b);
	Stamp_And_Insert(b);

	if (!Check_Gate(b,result)) { out->Reset(); return false; }

	out->Checksum = Hybrid_Hydrology_Checksum(*out,identity.Checksum);
	Note_Peak(b);

	WWDEBUG_SAY(("Hybrid Phase 6: %d rivers (%d guided), %d lakes, %d coast, %d islands (%d rejected), "
					 "%d windows from %d stations (%d merged), threshold %.1f after %d reductions, "
					 "water %.3f of %.3f, checksum 0x%08X\r\n",
					 out->Rivers.Count(),(out->Rivers.Count() > 0) && out->Rivers[0].Guided ? 1 : 0,
					 out->Count_Of_Body_Kind(HYBRID_WATER_BODY_LAKE),
					 out->Count_Of_Body_Kind(HYBRID_WATER_BODY_COAST),
					 out->Count_Of_Body_Kind(HYBRID_WATER_BODY_ISLAND),out->IslandsRejected,
					 out->Windows.Count(),out->StationsConsidered,out->StationsMerged,
					 out->RiverThreshold,out->ThresholdReductions,out->StandingWaterFraction,
					 out->WaterFractionTarget,out->Checksum));

	return true;
}
