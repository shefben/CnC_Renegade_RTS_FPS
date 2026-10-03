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
 *                     $Archive:: /Commando/Code/wwphys/hybridplanning.h                      $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Hybrid world generator rewrite, item I2 -- the shared deterministic helpers every hybrid    *
 * phase uses (Procedural_World_Generator_Rewrite_Plan_FINAL_AUDITED.md Sections 0.3, 0.5,     *
 * 0.6, 2.3, 7.1 and 16):                                                                      *
 *                                                                                             *
 *   Section 0.3   Hash01 / PickIndex / PickSign / Range / RoundHalfUp and quantization        *
 *   Section 0.5   the canonical constants H C W A S Q D R_BASE R_EXPAND R_BASE_PROTECTED       *
 *   Section 0.6   size class and the hard world-size limits                                   *
 *   Section 7.1   road width, corridor, vertical-interval and grade-compatibility helpers     *
 *   Section 2.3   ProceduralPlanningSpatialHash (B_HASH buckets, StableId-sorted results)     *
 *   Section 16    per-attempt work counters, budgets and the scratch arena                    *
 *                                                                                             *
 * Everything here is pure math over values the caller passes in: no profile, no metric       *
 * lookup, no global state.  The Combat-side preflight (hybridpreflight.h) resolves the       *
 * runtime metrics and feeds them through Hybrid_Compute_Constants.                            *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef HYBRIDPLANNING_H
#define HYBRIDPLANNING_H

#include "always.h"
#include "vector.h"
#include "hybridplanrecords.h"


/*
**	Section 0.3 -- quantization steps.  Positions and lengths snap to Q world units, angles to
**	0.001 radians, normalized [0,1] values to 1e-6.  Checksums take the quantized integers, never
**	raw float bits.
*/
#define HYBRID_QUANTUM_POSITION				0.001f
#define HYBRID_QUANTUM_ANGLE					0.001f
#define HYBRID_QUANTUM_NORMALIZED			0.000001f


/*
**	Section 0.3 helpers.  Hybrid_Hash01 is the plan's Hash01: the repo CRC over the three keys,
**	seeded with a subsystem seed from WorldSeedService::Derive_Subsystem_Seed, reduced to its top
**	24 bits so the float is exact and always < 1.
*/
float			Hybrid_Round_Half_Up(float value);
int			Hybrid_Quantize_To_Int(float value,float quantum);
float			Hybrid_Quantize(float value,float quantum);
inline float	Hybrid_Quantize_Position(float value)		{ return Hybrid_Quantize(value,HYBRID_QUANTUM_POSITION); }
inline float	Hybrid_Quantize_Angle(float value)			{ return Hybrid_Quantize(value,HYBRID_QUANTUM_ANGLE); }
inline float	Hybrid_Quantize_Normalized(float value)	{ return Hybrid_Quantize(value,HYBRID_QUANTUM_NORMALIZED); }

/*
**	The plan writes its draws as `Hash01(domain,"name",...)`.  Hybrid_Hash01 takes integer keys, so
**	a named key becomes one through this: the repository string CRC, which is stable across hosts and
**	builds.  Use it for every named key rather than a hand-assigned number, so a key's identity stays
**	the plan's own spelling.
*/
unsigned int	Hybrid_Key(const char * name);

float			Hybrid_Hash01(unsigned int domain_seed,unsigned int k0 = 0,unsigned int k1 = 0,unsigned int k2 = 0);
int			Hybrid_Pick_Index(int count,float h);
int			Hybrid_Pick_Sign(float h);
float			Hybrid_Range(float a,float b,float h,float quantum = HYBRID_QUANTUM_POSITION);

/*
**	Section 8 Phases 5, 6 and 10 -- the one centripetal Catmull-Rom sampler for every plan curve.
**	The curve interpolates every control (the two ends are reflected into phantom controls), knots
**	are centripetal, and the output polyline is resampled at `spacing` by arc length with both
**	endpoints included and every point quantized to `Q`.  `out_stations` receives the along-curve
**	arc length of each point when it is not null.  False means degenerate input and no output.
*/
/*
**	The most controls one curve may carry, phantom ends included: the size of the sampler's own knot
**	vector.  Phase 5's line features use four; a Phase-10 road spline uses one per direction change of its
**	A* chain, which on a long route is dozens.
*/
#define HYBRID_MAX_SPLINE_KNOTS			512

bool			Hybrid_Sample_Centripetal_Catmull_Rom(const Vector2 * controls,int control_count,float spacing,
																	DynamicVectorClass<Vector2> * out_points,
																	DynamicVectorClass<float> * out_stations);


/*
**	Checksum accumulator for identity hashes and stage checksums.  Floats go in as 64-bit quantized
**	integers (world units at 0.001, angles at 0.001 rad, every other scalar at 1e-6) and every value
**	is fed as explicit little-endian bytes, so the result never depends on float bit patterns or host
**	byte order.
*/
class HybridHashStream
{
public:
	HybridHashStream(unsigned int seed = 0) : Crc(seed)	{ }

	void				Add_Uint(unsigned int value);
	void				Add_Int(int value)							{ Add_Uint((unsigned int)value); }
	void				Add_Bool(bool value)							{ Add_Uint(value ? 1u : 0u); }
	void				Add_Quantized(float value,float quantum);
	void				Add_Length(float value)						{ Add_Quantized(value,HYBRID_QUANTUM_POSITION); }
	void				Add_Angle(float value)						{ Add_Quantized(value,HYBRID_QUANTUM_ANGLE); }
	void				Add_Scalar(float value)						{ Add_Quantized(value,HYBRID_QUANTUM_NORMALIZED); }
	void				Add_String(const char * value);

	unsigned int	Get(void) const								{ return Crc; }

private:
	void				Add_Bytes(const unsigned char * bytes,int count);

	unsigned int	Crc;
};


/*
**	Section 0.6 -- size class by D = min(width,height), and the hard limits.
*/
enum HybridSizeClass
{
	HYBRID_SIZE_SMALL = 0,				// D < 900
	HYBRID_SIZE_MEDIUM,					// 900 <= D < 1800
	HYBRID_SIZE_LARGE,					// 1800 <= D <= 2400
	HYBRID_SIZE_CLASS_COUNT
};

#define HYBRID_MAX_WORLD_EXTENT				2400.0f
#define HYBRID_MAX_WORLD_AREA					5760000.0f
#define HYBRID_SIZE_MEDIUM_MIN_D				900.0f
#define HYBRID_SIZE_LARGE_MIN_D				1800.0f

const char *	Hybrid_Size_Class_Name(HybridSizeClass size_class);
HybridSizeClass	Hybrid_Size_Class_For(float min_dimension);
bool				Hybrid_World_Size_Within_Limits(float width,float height);


/*
**	Section 0.5 -- the canonical constants, computed once per request from the resolved Humvee
**	width, world units per meter, chunk size, world size and base settings.
*/
struct HybridConstants
{
	float					WorldUnitsPerMeter;
	float					H;							// Humvee lateral collision width
	float					C;							// max(0.35 * WUPM, 0.15 * H)
	float					W;							// max(H + 2C, 1.30 * H)
	float					A;							// analysis-grid spacing
	float					S;							// coarse semantic-matrix spacing
	float					Q;							// geometry quantization
	float					D;							// min(width,height)
	float					RBase;					// base_radius_cells * chunk_size
	float					RExpand;					// sqrt(min_base_expansion_area / pi)
	float					RBaseProtected;		// max(RBase,RExpand)
	float					BHash;					// max(4A, 2S), planning spatial hash bucket
	HybridSizeClass	SizeClass;
};

void	Hybrid_Compute_Constants(float humvee_width,float world_units_per_meter,float chunk_size,
										float world_width,float world_height,float base_radius_cells,
										float min_base_expansion_area,HybridConstants * out);


/*
**	Section 7.1 -- road helpers.  Widths are world units; the class table is Section 7's.
*/
struct HybridVerticalInterval
{
	float	Low;
	float	High;
};

float							Hybrid_Roadbed_Width(HybridRoadClass road_class,float w);
float							Hybrid_Carriageway_Width(HybridRoadClass road_class,float w);
float							Hybrid_Preferred_Corridor_Half_Width(HybridRoadClass road_class,float s,float d);
float							Hybrid_Max_Corridor_Half_Width(HybridRoadClass road_class,float s,float d);
HybridVerticalInterval	Hybrid_Road_Vertical_Interval(float z,float max_cut_depth,float max_fill_height);
float							Hybrid_Road_Outer_Blend_Width(float w,float a);
float							Hybrid_Road_Deformation_Half_Width(HybridRoadClass road_class,float w,float a);
float							Hybrid_Interval_Gap(const HybridVerticalInterval & i0,const HybridVerticalInterval & i1);
bool							Hybrid_Grade_Compatible(const HybridVerticalInterval & i0,const HybridVerticalInterval & i1,
																float horizontal_distance,float max_grade_radians);

/*
**	A road's roadbed edges relative to its reference spline, inner (left of travel, negative) and outer.  For
**	every class but HIGHWAY_RAMP the reference is the roadbed centre, so the two are equal and opposite; a
**	ramp's reference is its travel lane (D-022), so its roadbed spans -0.70W..+0.80W and its derived midpoint
**	lies RoadbedMidpointOffset = +0.05W outward -- a derived value, never a routing centerline.
*/
float							Hybrid_Road_Inner_Edge_Offset(HybridRoadClass road_class,float w);
float							Hybrid_Road_Outer_Edge_Offset(HybridRoadClass road_class,float w);
float							Hybrid_Road_Roadbed_Midpoint_Offset(HybridRoadClass road_class,float w);


/*
**	Section 12.6.6's interchange constants, derived once from the Section-7 table (never retyped), in world
**	units.  Angles are radians.  Q-026 / D-022 values included.
*/
struct HybridInterchangeConstants
{
	float	W;
	float	MergeLength;					// max(12W, 2 R_HIGHWAY_4)    = 16W
	float	DivergeLength;					// max(10W, 1.5 R_HIGHWAY_4)  = 12W
	float	TaperLength;					// 6W
	float	MergeFullLength;				// MergeLength - TaperLength  = 10W
	float	DivergeFullLength;			// DivergeLength - TaperLength =  6W
	float	GoreReserveMargin;			// 2W, also the gore shoulder transition
	float	InterchangeGoreSpacing;		// 24W
	int	GoreCandidates;				// K_GORE = 32
	float	ThetaC;							// 15 degrees
	float	Psi;								// 45 degrees
	float	RampRadius;						// R_r = 5W
	float	ArmLength;						// l_arm = max(4W, R_r) = 5W
	float	T15;								// R_r tan(7.5 deg)
	float	T30;								// R_r tan(15 deg)
	float	DSep;								// 0.5 RoadbedWidth(HIGHWAY_4) + 0.5 RoadbedWidth(HIGHWAY_RAMP) = 3.575W
	float	DJoin;							// 0.5 median + 1.5W = 1.875W (outer through lane centre)
	float	DAux;								// DJoin + W = 2.875W (auxiliary lane centre; the gore offset)
	float	HHwy;								// 0.5 median + 2W = 2.375W
	float	EHwy;								// 0.5 RoadbedWidth(HIGHWAY_4) = 2.825W
	float	HAux;								// HHwy + W = 3.375W
	float	EAux;								// EHwy + W = 3.825W
	float	HighwayShoulder;				// HIGHWAY_4's shoulder, 0.45W: the gore's outer shoulder
	float	HighwayJunctionReserve;		// max(4W, R_HIGHWAY_4) = 8W
};

void	Hybrid_Interchange_Constants(float w,HybridInterchangeConstants * out);


/*
**	Section 12.6.7, one gore candidate `k`: l_conv, d_Lmin, d_L (`d_lov` > 0 for a grade-separated DIAMOND_ACCESS
**	crossing, else 0), G and S_I.  All RampLaneCenterline positions in the side frame.
*/
struct HybridRampCandidate
{
	int	K;
	float	ConvergenceLength;			// l_conv(k)
	float	LocalOffsetMin;				// d_Lmin(k)
	float	LocalOffset;					// d_L(k)
	float	GoreOffset;						// G(k)
	float	StraightHalfExtent;			// S_I(k)
};

void	Hybrid_Ramp_Candidate(const HybridInterchangeConstants & c,int k,float d_lov,HybridRampCandidate * out);

/*
**	The ramp's control polygon in travel order, side frame (12.6.7): an ON_RAMP is [L_s, V1, V2, p_g], an
**	OFF_RAMP its mirror [p_g', V2', V1', L_s].  `role` is a HybridRampRole.
*/
void	Hybrid_Ramp_Control_Polygon(const HybridInterchangeConstants & c,const HybridRampCandidate & candidate,int role,
											Vector2 polygon[4]);

/*
**	The Q-026 merge/diverge cross section at `along`, the distance into the ramp's highway-parallel interval in
**	chain order (an ON_RAMP's interval starts at its gore, an OFF_RAMP's ends at it): the auxiliary-lane centre
**	`d`, the widened ramp-side carriageway and roadbed half widths `h = d + 0.5W` and `E = d + 0.95W`, whether
**	the station is in the full-width lane, and the taper parameter `u` (0 in the full lane).
*/
struct HybridAuxiliaryLaneSection
{
	bool	Full;
	float	U;
	float	D;
	float	H;
	float	E;
};

void	Hybrid_Auxiliary_Lane_Section(const HybridInterchangeConstants & c,int role,float along,
											HybridAuxiliaryLaneSection * out);

/*
**	D-022's gore shoulder transition on the independent ramp, `from_gore` along it from the gore (0..2W):
**	the inner and outer shoulders, which reach 0 and the highway's 0.45W at the gore.  The reference spline
**	does not move.  The same for an ON_RAMP (approaching its gore) and an OFF_RAMP (leaving it).
*/
void	Hybrid_Ramp_Gore_Shoulders(const HybridInterchangeConstants & c,float from_gore,float * inner_out,float * outer_out);


/*
**	Section 2.3 -- ProceduralPlanningSpatialHash.  Compiler scratch, not a runtime spatial
**	subsystem.  Each feature goes into every bucket its quantized AABB touches; queries return the
**	distinct StableIds whose buckets overlap the query box, sorted ascending.  The bucket grid is
**	fixed at construction over [0,width) x [0,height); coordinates outside clamp to the edge.
*/
class ProceduralPlanningSpatialHash
{
public:
	ProceduralPlanningSpatialHash(void);
	~ProceduralPlanningSpatialHash(void);

	void		Init(float world_width,float world_height,float bucket_size);
	void		Reset(void);

	void		Insert(unsigned int stable_id,float min_x,float min_y,float max_x,float max_y);
	int		Query(float min_x,float min_y,float max_x,float max_y,DynamicVectorClass<unsigned int> * out_ids) const;

	float		Get_Bucket_Size(void) const		{ return BucketSize; }
	int		Get_Bucket_Count_X(void) const	{ return CountX; }
	int		Get_Bucket_Count_Y(void) const	{ return CountY; }
	int		Get_Entry_Count(void) const		{ return EntryCount; }

private:
	void		Bucket_Range(float min_x,float min_y,float max_x,float max_y,int * x0,int * y0,int * x1,int * y1) const;

	float										BucketSize;
	int										CountX;
	int										CountY;
	int										EntryCount;
	DynamicVectorClass<unsigned int> *	Buckets;
};


/*
**	Section 16.1 -- deterministic work counters, one set per attempt, and the budgets that bound
**	them.  A counter hitting its budget fails that stage deterministically; wall-clock time is
**	never an input.
*/
enum HybridWorkCounter
{
	HYBRID_WORK_SEMANTIC_CELLS = 0,
	HYBRID_WORK_ANALYSIS_CELLS,
	HYBRID_WORK_ASTAR_EXPANSIONS,
	HYBRID_WORK_ROUTED_EDGES,
	HYBRID_WORK_CROSSING_PROBLEMS,
	HYBRID_WORK_CROSSING_STATIONS,
	HYBRID_WORK_LOS_RAYS,
	HYBRID_WORK_REFINEMENT_SAMPLES,
	HYBRID_WORK_ATTEMPTS,
	HYBRID_WORK_PEAK_SCRATCH_BYTES,
	HYBRID_WORK_COUNTER_COUNT
};

struct HybridWorkBudgets
{
	int	MaxRoutedEdges;							// R: 18 / 32 / 52 by size class
	int	AStarExpansionsPerRoute;				// 8 * N, N = analysis grid cells
	int	AStarExpansionsPerAttempt;				// R * 8 * N
	int	CrossingStationsPerProblem;			// 64
	int	CrossingProblemsPerEdge;				// 12
	int	LosRays;										// 512 / 2048 / 4096 by size class
	int	LocalRepairCandidates;					// 3 * retry.max_alternate_candidates_per_feature
	int	AttemptCount;								// 3
	int	WorkerCount;								// 4
	int	ScratchArenaBytes;						// 128 / 256 / 512 MiB by size class
};

const char *	Hybrid_Work_Counter_Name(HybridWorkCounter counter);
void				Hybrid_Get_Work_Budgets(HybridSizeClass size_class,int analysis_grid_cells,
												int max_alternate_candidates_per_feature,HybridWorkBudgets * out);

class HybridWorkCounters
{
public:
	HybridWorkCounters(void)		{ Reset(); }

	void		Reset(void);
	void		Add(HybridWorkCounter counter,int amount = 1)	{ Counts[counter] += amount; }
	void		Note_Peak(HybridWorkCounter counter,int value)	{ if (value > Counts[counter]) Counts[counter] = value; }
	int		Get(HybridWorkCounter counter) const				{ return Counts[counter]; }
	bool		Exceeds(HybridWorkCounter counter,int budget) const	{ return Counts[counter] > budget; }

private:
	int		Counts[HYBRID_WORK_COUNTER_COUNT];
};


/*
**	Section 16.3 -- the per-attempt scratch arena.  One contiguous block reserved at the start of
**	an attempt; allocations bump a cursor and are released together by Reset.  An allocation past
**	the capacity fails (returns null) rather than growing, so the caller reports it as a budget
**	failure instead of paging.
*/
class HybridScratchArena
{
public:
	HybridScratchArena(void);
	~HybridScratchArena(void);

	bool		Reserve(int capacity_bytes);
	void		Release(void);
	void		Reset(void)							{ Used = 0; }
	void *	Allocate(int bytes,int alignment = 16);

	int		Get_Capacity(void) const		{ return Capacity; }
	int		Get_Used(void) const				{ return Used; }
	int		Get_High_Water(void) const		{ return HighWater; }

private:
	unsigned char *	Block;
	int					Capacity;
	int					Used;
	int					HighWater;
};


#endif
