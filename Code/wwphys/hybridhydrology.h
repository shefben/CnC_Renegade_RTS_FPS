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
 *                     $Archive:: /Commando/Code/wwphys/hybridhydrology.h                     $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Hybrid world generator rewrite, item I7 -- runtime Phase 6, the vector hydrology             *
 * (Procedural_World_Generator_Rewrite_Plan_FINAL_AUDITED.md Section 8 Phase 6, Section 11).    *
 * Water becomes explicit vectors here: river centerlines with their per-sample widths, lake     *
 * and coast polygons, the optional decorative islands, and -- once every water vector is       *
 * fixed -- every crossing and ramp/pass window the later road phases are allowed to use.       *
 *                                                                                             *
 * No heightfield is written: Phase 7 owns the terrain and consumes this phase's carve through   *
 * Hybrid_Water_Carve_Depth, so the carve profile has exactly one owner.  No road exists yet     *
 * either; a window is a *permission*, not a bridge, and Phases 9 to 12 decide which windows     *
 * are used and what physical object satisfies them.                                            *
 *                                                                                             *
 * Everything this phase reads is already frozen: the Phase-2 identity and Section-6.10          *
 * intents, the Phase-3 roles/corridor obligations, the Phase-4 matrices and reservations, and   *
 * the Phase-5 landforms.  The provisional drainage surface is semantic tier + Phase-5           *
 * landforms through `Hybrid_Landform_Height` and nothing else, which is why Phase 6 never       *
 * re-derives a landform profile: drainage that disagreed with Phase 5's geometry would put      *
 * rivers on the wrong side of a ridge.                                                         *
 *                                                                                             *
 * The matrices are taken mutable for exactly two writes the plan names: the accepted water      *
 * cells (`WATER_BASIN` / `COAST` intent and the `LAND` clear for an island) and each accepted   *
 * window's `CROSSING_WINDOW` / `RAMP_PASS_WINDOW` reservation plus its `NO_DECORATION`          *
 * approach stamp.  The window reservations reuse Phase 4's own comparator rather than a second  *
 * copy of it, which is the reason Phase 4 left `HYBRID_RESERVE_CROSSING_WINDOW` in its          *
 * priority list with nothing to insert yet.                                                    *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef HYBRIDHYDROLOGY_H
#define HYBRIDHYDROLOGY_H

#include "always.h"
#include "vector.h"
#include "vector2.h"
#include "wwstring.h"
#include "hybridplanrecords.h"
#include "hybridplanning.h"
#include "hybridmapidentity.h"
#include "hybridstrategicgraph.h"
#include "hybridcoarsematrix.h"
#include "hybridlandforms.h"
#include "hybridlocalpass.h"


/*
**	The standing-water/island polygon kinds.  A river is not one of these -- it is an open
**	centerline with a width, which is a different record -- and the ordinal is part of every
**	deterministic draw key, so never reorder.
*/
enum HybridWaterBodyKind
{
	HYBRID_WATER_BODY_LAKE = 0,		// inland standing water, from a Priority-Flood basin or a required guide
	HYBRID_WATER_BODY_COAST,			// the exterior sea, only where the archetype requires one
	HYBRID_WATER_BODY_ISLAND,			// decorative dry land inside a host water body
	HYBRID_WATER_BODY_KIND_COUNT
};

const char *	Hybrid_Water_Body_Kind_Name(HybridWaterBodyKind kind);


/*
**	Section 8 Phase 6 step 18's window kinds.  A river or a canyon/ravine gets a `CROSSING_WINDOW`;
**	an explicit cliff or ridge gets a `RAMP_PASS_WINDOW`.  Lakes and coast get neither: they are not
**	bridgeable road obstacles in this generator version (step 20).
*/
enum HybridWindowKind
{
	HYBRID_WINDOW_CROSSING = 0,
	HYBRID_WINDOW_RAMP_PASS,
	HYBRID_WINDOW_KIND_COUNT
};

const char *	Hybrid_Window_Kind_Name(HybridWindowKind kind);


/*
**	Step 18's method flags, computed from the frozen preflight metrics at window creation time and
**	never re-derived later.  Phase 11 chooses among the methods a window already permits; it may not
**	invent one the window does not carry.
*/
enum HybridWindowMethodFlag
{
	HYBRID_WINDOW_METHOD_BRIDGE			= 1 << 0,
	HYBRID_WINDOW_METHOD_PASS				= 1 << 1,	// a PRIMARY_2 swept path across the local floor/ramp
	HYBRID_WINDOW_METHOD_TUNNEL			= 1 << 2,
	HYBRID_WINDOW_METHOD_FORD_LIGHT		= 1 << 3,
	HYBRID_WINDOW_METHOD_FORD_HARVESTER	= 1 << 4,
	HYBRID_WINDOW_METHOD_CUT				= 1 << 5,	// cliff/ridge only: the cut Phase 11 tries first
};


/*
**	One accepted river system.
**
**	`Points` is the vector centerline sampled at `A/2` and quantized; `Stations` is each point's
**	along-centerline arc length; `Accumulation`, `Widths` and `Surfaces` are per point, so Phase 7's
**	longitudinal walk and Phase 11's span queries read one record and compute nothing.
**
**	`Surfaces` is exactly what Phase 7 step 1 asks for: the Phase-6 filled-drainage height at that
**	sample for a D8-derived river, and the Phase-6 provisional terrain height for a guided river,
**	which has no D8 chain of its own.
*/
struct HybridRiverSystem
{
	HybridRiverSystem(void) :
		StableId(0),IntentStableId(0),Guided(false),Smoothed(false),SourceCell(-1),SinkCell(-1),
		SinkBodyId(0),SelectionRank(0),MaxAccumulation(0.0f),Length(0.0f),BedDepth(0.0f),
		Source(0.0f,0.0f),Mouth(0.0f,0.0f),BoundsMin(0.0f,0.0f),BoundsMax(0.0f,0.0f) { }

	//	DynamicVectorClass<T> (wwlib/vector.h) requires T to support == and !=.  StableId is the key.
	bool	operator == (const HybridRiverSystem & that) const	{ return StableId == that.StableId; }
	bool	operator != (const HybridRiverSystem & that) const	{ return !(*this == that); }

	unsigned int	StableId;				// hydrology record id, 1-based across rivers/bodies/windows
	unsigned int	IntentStableId;		// the GUIDED_RIVER / COASTAL_GUIDED_RIVER intent, 0 when D8-derived
	bool				Guided;
	bool				Smoothed;				// step 12's smoothing pass was accepted
	int				SourceCell;				// analysis-grid row-major index, -1 for a guided river
	int				SinkCell;
	unsigned int	SinkBodyId;				// the lake/coast body the mouth binds to, 0 = legal exterior sink
	int				SelectionRank;			// 0 is the guided river when one exists (step 10)

	float				MaxAccumulation;		// analysis cells of drainage at the sink
	float				Length;					// centerline arc length
	float				BedDepth;				// WaterDefinitionClass::BedDepth of the carving water type

	Vector2			Source;
	Vector2			Mouth;
	Vector2			BoundsMin;				// centerline inflated by the local half width
	Vector2			BoundsMax;

	DynamicVectorClass<Vector2>	Points;
	DynamicVectorClass<float>		Stations;
	DynamicVectorClass<float>		Accumulation;
	DynamicVectorClass<float>		Widths;
	DynamicVectorClass<float>		Surfaces;

	float	Width_At_Index(int index) const	{ return ((index >= 0) && (index < Widths.Count ())) ? Widths[index] : 0.0f; }
};


/*
**	One accepted lake, coast or island polygon.  `Points` is a closed simple ring in world space,
**	quantized, with no repeated final point.
**
**	An island is stored as its own record rather than as a hole in its host's ring: the plan's
**	"subtract it from the host water polygon" is realized by every water query in this file
**	treating an island polygon as land, which is the same result without a polygon boolean and
**	without a second representation of the host's outline for Phase 7 to disagree with.
*/
struct HybridWaterBody
{
	HybridWaterBody(void) :
		StableId(0),Kind(HYBRID_WATER_BODY_LAKE),IntentStableId(0),HostBodyId(0),BasinLabel(-1),
		Required(false),FromGuidePolygon(false),Simplified(false),SurfaceHeight(0.0f),Area(0.0f),
		BedDepth(0.0f),HeightBias(0.0f),ShoreBlend(0.0f),Retries(0),
		Center(0.0f,0.0f),BoundsMin(0.0f,0.0f),BoundsMax(0.0f,0.0f) { }

	bool	operator == (const HybridWaterBody & that) const	{ return StableId == that.StableId; }
	bool	operator != (const HybridWaterBody & that) const	{ return !(*this == that); }

	unsigned int			StableId;
	HybridWaterBodyKind	Kind;
	unsigned int			IntentStableId;	// LAKE_GUIDE / BASIN_GUIDE / COAST_GUIDE intent, 0 when none
	unsigned int			HostBodyId;			// island only: the water body it stands in
	int						BasinLabel;			// the Priority-Flood basin it came from, -1 when none
	bool						Required;			// a Section-5 quota lake, or the archetype's own coast
	bool						FromGuidePolygon;	// step 14/15's carved guide polygon rather than a real basin
	bool						Simplified;			// the A/2 simplification was accepted

	float						SurfaceHeight;		// water elevation; an island carries its host's
	float						Area;					// polygon area, world units squared
	float						BedDepth;			// WaterDefinitionClass::BedDepth, 0 for ocean/coast and islands
	float						HeightBias;			// island only: Phase 7's `island_macro_height_bias`
	float						ShoreBlend;			// island only: the 2W shoreline blend band
	int						Retries;				// local retries this body consumed

	Vector2					Center;
	Vector2					BoundsMin;
	Vector2					BoundsMax;

	DynamicVectorClass<Vector2>	Points;
};


/*
**	The run-shortfall histogram's buckets.  Fixed bounds, so two runs of the same world report the same
**	distribution and a median can be read off without keeping every sample.
*/
#define HYBRID_PASS_RATIO_BUCKETS 8

int				Hybrid_Pass_Ratio_Bucket(float ratio);
const char *	Hybrid_Pass_Ratio_Bucket_Name(int bucket);


/*
**	One accepted crossing/pass window.  The obstruction is named by its own record: a river's
**	hydrology StableId, or a Phase-5 landform's StableId.
**
**	`Tangent` runs along the obstruction; the crossing direction is its normal, and `ApproachA` /
**	`ApproachB` are the two ends of the reserved straight approach -- the exact geometry Phase 11
**	sweeps and Phase 9 reserves against decoration.
*/
struct HybridCrossingWindow
{
	HybridCrossingWindow(void) :
		StableId(0),Kind(HYBRID_WINDOW_CROSSING),Obstruction(HYBRID_OBSTRUCTION_NONE),
		ObstructionRecordId(0),Methods(0),Preferred(false),MergedCount(1),Station(0.0f),Span(0.0f),
		ApproachLength(0.0f),LowElevation(0.0f),HighElevation(0.0f),Center(0.0f,0.0f),
		Tangent(1.0f,0.0f),ApproachA(0.0f,0.0f),ApproachB(0.0f,0.0f),
		BoundsMin(0.0f,0.0f),BoundsMax(0.0f,0.0f) { }

	bool	operator == (const HybridCrossingWindow & that) const	{ return StableId == that.StableId; }
	bool	operator != (const HybridCrossingWindow & that) const	{ return !(*this == that); }

	bool	Has_Method(unsigned int flag) const	{ return (Methods & flag) != 0; }

	unsigned int				StableId;
	HybridWindowKind			Kind;
	HybridObstructionKind	Obstruction;
	unsigned int				ObstructionRecordId;	// river StableId, or HybridLandform::StableId
	unsigned int				Methods;					// HybridWindowMethodFlag bits
	bool							Preferred;				// on a required heavy corridor, or within 2S of its guide
	int							MergedCount;			// stations folded into this window by step 19

	float							Station;					// along-obstruction arc length of the station
	float							Span;						// obstruction width/clear span at the station
	float							ApproachLength;		// 8W, or the registered bridge approach when longer
	float							LowElevation;			// provisional endpoint elevations, for the grade rule
	float							HighElevation;

	Vector2						Center;
	Vector2						Tangent;
	Vector2						ApproachA;
	Vector2						ApproachB;
	Vector2						BoundsMin;
	Vector2						BoundsMax;

	/*
	**	The local pass this station proved, when it proved one.  Phase 6 freezes the geometry rather than only
	**	the verdict so Phase 11 evaluates the *same* pass: a flag alone would leave Phase 11 to invent another
	**	path, and two phases inventing their own geometry for one crossing is how they come to disagree.
	**	Meaningful only when `Has_Method(HYBRID_WINDOW_METHOD_PASS)`.
	*/
	HybridLocalPassGeometry	Pass;
};


/*
**	Runtime Phase 6's output.  `Hash` is the Section-2.3 planning spatial hash over every accepted
**	water vector and window AABB, in coordinates relative to the world rectangle's minimum corner,
**	which is the same convention Phase 5 used; Phases 7 and 9-12 query it instead of scanning.
**
**	Every record shares one StableId space, assigned in creation order, so a hash query answers with
**	ids that are unique across rivers, bodies and windows.
*/
struct HybridHydrologySet
{
	HybridHydrologySet(void)	{ Reset (); }

	void	Reset(void);

	const HybridRiverSystem *		Find_River(unsigned int stable_id) const;
	const HybridWaterBody *			Find_Body(unsigned int stable_id) const;
	const HybridCrossingWindow *	Find_Window(unsigned int stable_id) const;

	int	Count_Of_Body_Kind(HybridWaterBodyKind kind) const;
	int	Count_Of_Window_Kind(HybridWindowKind kind) const;

	//	The Section-5 true-lake count: required or not, a lake that met the qualified area.
	int	Count_Of_True_Lakes(float minimum_area) const;

	DynamicVectorClass<HybridRiverSystem>		Rivers;
	DynamicVectorClass<HybridWaterBody>			Bodies;
	DynamicVectorClass<HybridCrossingWindow>	Windows;
	ProceduralPlanningSpatialHash					Hash;

	//	Section 5 quotas this attempt had to satisfy, kept for the gate and the log.
	int			RequiredRivers;
	int			RequiredLakes;

	float			RiverThreshold;			// the accepted threshold, after any step-11 reduction
	int			ThresholdReductions;		// 0..3
	float			StandingWaterArea;		// inland standing water, excluding rivers and exterior sea
	float			StandingWaterFraction;
	float			WaterFractionTarget;

	int			IslandSlots;				// accepted Section-5 slots this map had
	int			IslandsRejected;			// slots that found no polygon; never a failure
	int			OptionalLakes;				// basins added toward water_fraction
	int			StationsConsidered;

	//	Why step 18 refused a local pass at a station, by reason, for the census.
	int			PassRefusals[HYBRID_LOCAL_PASS_FAILURE_COUNT];

	/*
	**	How far short the refusals fell.  A count alone cannot tell "the supported parameter space cannot
	**	build a pass" from "passes fit and something later rejects them", and those are different defects, so
	**	the along-obstruction run every station asked for is kept as a distribution of
	**	`required_tangent_run / available_tangent_run`.  Diagnosis, not world state: none of it is hashed.
	*/
	int			PassStationsMeasured;
	int			PassRunFits;						// stations whose run fits, before any later validation
	float			PassRatioMin;
	float			PassRatioMax;
	int			PassRatioBuckets[HYBRID_PASS_RATIO_BUCKETS];

	/*
	**	And what the compiled profiles did, which the run distribution cannot say: the steepest grade any
	**	refused candidate's own profile reached, and which leg of the candidate it was on.
	*/
	float			PassProfileGradeMax;
	int			PassProfileLegRefusals[HYBRID_LOCAL_PASS_MAX_CONTROLS];
	int			StationsMerged;
	int			LocalRetries;

	unsigned int	Checksum;				// Phase 6 gate value
};


/*
**	Phase 6's inputs beyond the frozen identity/intent/graph/matrix/landform records.  Plain values
**	only, so the builder is reproducible from (params, identity, intents, graph, matrices,
**	landforms).
*/
struct HybridHydrologyParams
{
	HybridHydrologyParams(void);

	unsigned int		HydrologySeed;			// SEED_DOMAIN_HYDROLOGY, already derived
	HybridConstants	Constants;

	//	World rectangle, in world units; the analysis grid origin is the rectangle's minimum corner.
	float					WorldMinX;
	float					WorldMinY;
	float					WorldWidth;
	float					WorldHeight;

	//	Section 8 Phase 6/7 elevation frame.  Min/MaxElevation are relative to SeaLevel, exactly as
	//	the profile stores them.
	float					SeaLevel;
	float					MinElevation;
	float					MaxElevation;
	float					TierStep;

	/*
	**	Section 7.1's vertical-interval inputs.  Step 18 needs them because a pass candidate is planned road
	**	geometry: its profile has to be realizable inside `RoadVerticalInterval`, or the window would promise
	**	Phase 11 a pass that Phase 12 could not build.
	*/
	float					MaxCutDepth;
	float					MaxFillHeight;


	int					TierCount;
	float					ChunkSize;				// world.chunk_size, for the accumulation conversion

	//	Section 5 water toggles and the Section 9.2 channel metrics.
	bool					Rivers;
	bool					Lakes;
	bool					Coastline;
	float					RiverWidth;
	float					StreamWidth;
	float					RiverAccumulationChunks;
	int					MaxRiverCount;
	float					WaterFraction;

	/*
	**	`WaterDefinitionClass::BedDepth` is the only carve-depth source the plan allows, so the two
	**	depths this phase can select come in as values rather than being looked up here.  A selected
	**	system is a river by Section 5's own definition -- a stream is a tributary this phase never
	**	selects -- and ocean/coast carve nothing at all, so no stream or coast depth exists.
	*/
	float					RiverBedDepth;
	float					LakeBedDepth;

	//	Section 17's bounded retry limits.
	int					MaxLocalRetriesPerStage;
	int					MaxAlternateCandidatesPerFeature;

	//	Section 0.10 / 12.2 corridor clearance.
	float					MinVehicleLaneClearance;

	//	Step 18's preflight metrics.  Every one is a frozen `HybridMetricCache` field.
	float					HeavyVehicleWidth;
	float					HeavyVehicleHeight;
	float					HarvesterWidth;
	float					BridgeMaxSpanLength;			// 0 = no bridge kind is registered at all
	float					BridgeRequiredWidth;			// Section 12.4 clear width for the heavy route class
	float					BridgeApproachLength;		// 0 = none registered; the 8W rule then stands alone
	float					BridgeWaterClearanceMargin;
	bool					TunnelsAvailable;
	float					TunnelRequiredWidth;
	float					TunnelRequiredHeight;

	/*
	**	Fords.  `AllowFords` is the profile switch and `FordsAvailable` is preflight's answer about
	**	the runtime capability, which is false today (`FORD_CAPABILITY_UNAVAILABLE`): no fordability
	**	query exists to approve a swept crossing.  The two depths are that query's capability limits,
	**	so a window carries a ford flag only when the water it crosses is shallower than the limit; 0
	**	means the capability does not exist, which is what the manager passes until one ships.
	*/
	bool					AllowFords;
	bool					FordsAvailable;
	float					FordMaxWaterDepthLight;
	float					FordMaxWaterDepthHarvester;

	//	Section 16.3 arena and Section 16.1 counters.  Optional: with no arena the drainage arrays
	//	come from the ordinary heap, which is what a self-check or an offline tool gets.
	HybridScratchArena *		Arena;
	HybridWorkCounters *		Counters;
};


/*
**	Phase 6 failures.  Each is a named attempt failure: the manager retries with a new seed.
*/
enum HybridHydrologyFailure
{
	HYBRID_HYDROLOGY_OK = 0,
	HYBRID_HYDROLOGY_INVALID_INPUT,				// no matrix, no spacing, or a degenerate world rectangle
	HYBRID_HYDROLOGY_SCRATCH_EXHAUSTED,			// the arena cannot hold the drainage arrays
	HYBRID_HYDROLOGY_RIVER_QUOTA,					// step 11: too few systems after three threshold reductions
	HYBRID_HYDROLOGY_LAKE_QUOTA,					// step 14: a required basin survived no local retry
	HYBRID_HYDROLOGY_COAST_UNPLACEABLE,			// step 15: the required coast blocks a heavy corridor
	HYBRID_HYDROLOGY_FLOW_NOT_ACYCLIC,			// gate: a selected river's chain is not source-to-sink
	HYBRID_HYDROLOGY_WATER_IN_RESERVE,			// gate: BASE_PROTECTED or OPEN_MANEUVER no-water violation
	HYBRID_HYDROLOGY_CORRIDOR_BLOCKED,			// gate: a lake/coast leaves no dry heavy bypass
	HYBRID_HYDROLOGY_NO_CROSSING_WINDOW,		// gate: an obstructed required corridor has no window
	HYBRID_HYDROLOGY_RECORD_INCOMPLETE,			// gate: a required feature has no stable vector record
	HYBRID_HYDROLOGY_FAILURE_COUNT
};

const char *	Hybrid_Hydrology_Failure_Name(HybridHydrologyFailure failure);

struct HybridHydrologyResult
{
	HybridHydrologyResult(void) : Failure(HYBRID_HYDROLOGY_OK) { }

	HybridHydrologyFailure	Failure;
	StringClass					Detail;
};


/*
**	Section 5's per-size-class water quotas, so the gate, the builder and any later validator share
**	one owner: river systems 1 / 1 / 2 and true lakes 1 / 1 / 2.
*/
int		Hybrid_Required_River_Count(HybridSizeClass size_class);
int		Hybrid_Required_Lake_Count(HybridSizeClass size_class);

//	Section 5 qualifications: true lake area >= max(4*A*A, 0.002 * world area), island dry area
//	>= max((16W)^2, 0.01 * world area).
float		Hybrid_True_Lake_Min_Area(float a,float world_area);
float		Hybrid_Island_Min_Area(float w,float world_area);

//	Step 6: `river_threshold = max(1, RoundHalfUp(river_accumulation_chunks * (chunk_size/A)^2))`.
float		Hybrid_River_Threshold(float river_accumulation_chunks,float chunk_size,float a);

//	Section 11's exact width: lerp(stream_width, river_width, clamp((accum-threshold)/(3*threshold),0,1)).
float		Hybrid_River_Width(float accumulation,float threshold,float stream_width,float river_width);


/*
**	Runtime Phase 6.  Runs the plan's exact twenty-one-step sequence over the frozen Phase 2-5
**	records, writing the accepted water intent, the window reservations and their `NO_DECORATION`
**	approach stamps into `matrices`, and checks the phase gate.  Returns false with result->Failure
**	set on the first failure, leaving `out` empty.
*/
bool	Hybrid_Build_Hydrology(const HybridHydrologyParams & params,const HybridMapIdentity & identity,
										const HybridArchetypeIntentSet & intents,const HybridStrategicGraph & graph,
										const HybridLandformSet & landforms,HybridCoarseMatrices * matrices,
										HybridHydrologySet * out,HybridHydrologyResult * result);

/*
**	The provisional Phase-6 drainage surface: semantic tier plus the Phase-5 landform sum, with no
**	Phase-7 detail noise and no `+4*tier_step` base barrier, which is an analysis-only device.
**	Exposed because Phase 7's guided-river surface samples and any later validator must use exactly
**	this expression rather than a second copy of it.
*/
float	Hybrid_Provisional_Height(const HybridHydrologyParams & params,const HybridCoarseMatrices & matrices,
											const HybridLandformSet & landforms,const Vector2 & world);

/*
**	The Section-8 Phase-7 `water_carve` term at `world`: how far the ground is cut down by the
**	accepted water, in world units, positive downward.  A point inside a lake/coast polygon carves
**	to that body's bed; a point within half a river's local width carves to the river bed and blends
**	out across the bank width `max(river_width, 2W)`; a point inside an island polygon carves
**	nothing at all.
*/
float	Hybrid_Water_Carve_Depth(const HybridHydrologySet & set,const Vector2 & world,float lane_width,
											float river_width);

/*
**	Is `world` inside accepted standing water (lake or coast), with islands honoured as land?  The
**	body's id and surface height come back when the caller wants them.
*/
bool	Hybrid_Point_In_Standing_Water(const HybridHydrologySet & set,const Vector2 & world,
													unsigned int * body_out,float * surface_out);

/*
**	Is `world` wet at all -- inside accepted standing water, or inside a river's own channel out to half
**	its local width?  Islands are land, as everywhere else in this file.  One owner, because Phases 8, 9
**	and 15 all ask exactly this question of the frozen record.
*/
bool	Hybrid_Point_In_Water(const HybridHydrologySet & set,const Vector2 & world);


/*
**	Phase 7's island term: `island_macro_height_bias` blended from full value inside the island to
**	zero across its exact `2W` shoreline band.  Zero everywhere else.
*/
float	Hybrid_Island_Height_Bias(const HybridHydrologySet & set,const Vector2 & world);

/*
**	The Phase 6 gate value: a quantized digest of every accepted river, body and window in StableId
**	order.  Hybrid_Build_Hydrology stores it in out->Checksum.
*/
unsigned int	Hybrid_Hydrology_Checksum(const HybridHydrologySet & set,unsigned int seed = 0);


#endif // HYBRIDHYDROLOGY_H
