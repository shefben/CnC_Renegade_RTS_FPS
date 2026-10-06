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
 *                     $Archive:: /Commando/Code/wwphys/hybridrealization.h                   $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Hybrid world generator rewrite, item I13 -- runtime Phase 12, the part that is arithmetic.       *
 *                                                                                               *
 * Phase 12 consumes the frozen Phase-10 splines and Phase-11 compiled interactions and turns them  *
 * into roads on the ground.  It does not reroute, move a crossing, change a method or reopen       *
 * Phases 9 to 11; a failure here fails the attempt.  Two things about it are pure computation over   *
 * frozen records and live here, where a self-check can prove them exactly:                        *
 *                                                                                               *
 *   - Section 12.1's deterministic vertical-profile solver: sample a road interval at              *
 *     `min(W/2, A/3)`, give every sample its `RoadVerticalInterval` (or the fixed elevation a       *
 *     compiled crossing or junction endpoint requires), propagate the class grade forward and       *
 *     backward, walk the clamp, quantize, revalidate.  The output is the centerline the road will   *
 *     be built to, or a named refusal;                                                             *
 *   - Section 12.2's cross-section: at each solved sample, the roadbed is flat at the solved         *
 *     height, the shoulder is at the roadbed edge, and the ground blends from the shoulder edge to    *
 *     untouched terrain with a smoothstep over exactly `outer_blend = max(W, A/2)`.                *
 *                                                                                               *
 * Phase 12's plan lives here too: every road in routing order, graded once with its structures and  *
 * junction surfaces and carrying its actual paved edges, the terrain deformation over a height grid,  *
 * and every compiled bridge as its BridgeAssembly.  The runtime owners (WorldTerrainSystem,          *
 * RoadSystem, BridgeSystem) are driven from that plan by the procedural world manager.              *
 *                                                                                               *
 *---------------------------------------------------------------------------------------------*/

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef HYBRIDREALIZATION_H
#define HYBRIDREALIZATION_H

#include "always.h"
#include "vector.h"
#include "vector2.h"
#include "wwstring.h"
#include "hybridplanning.h"
#include "hybridroutes.h"
#include "hybridinteractions.h"


/*
**	Why a profile could not be solved.  Every one is a hard attempt failure: the road was accepted by
**	Phase 10 and its crossings compiled by Phase 11, so a profile that cannot exist means an earlier
**	phase promised something this one cannot build, and that is reported rather than patched.
*/
enum HybridProfileFailure
{
	HYBRID_PROFILE_OK = 0,
	HYBRID_PROFILE_INVALID_INPUT,				// too few samples, no height, or a degenerate spacing
	HYBRID_PROFILE_FORWARD_EMPTY,				// forward propagation emptied an interval
	HYBRID_PROFILE_BACKWARD_EMPTY,			// backward propagation emptied an interval
	HYBRID_PROFILE_CLAMP_EMPTY,				// the clamp walk found no legal height at a sample
	HYBRID_PROFILE_GRADE_AFTER_QUANTIZE,	// quantization pushed an adjacent grade past the class maximum
	HYBRID_PROFILE_FAILURE_COUNT
};

const char *	Hybrid_Profile_Failure_Name(HybridProfileFailure failure);


/*
**	One sample of a road interval the solver reads: where it is, the terrain there before this road
**	touches it, and -- when a compiled crossing or junction fixes the height here -- that height.
*/
struct HybridProfileSample
{
	HybridProfileSample(void) :
		Position(0.0f,0.0f),Station(0.0f),TerrainZ(0.0f),Fixed(false),FixedZ(0.0f),Unbounded(false),MaxGrade(0.0f) { }

	//	DynamicVectorClass<T> (wwlib/vector.h) requires T to support == and !=.
	bool	operator == (const HybridProfileSample & that) const
	{
		return (Station == that.Station) && (Position == that.Position) && (TerrainZ == that.TerrainZ) &&
				 (Fixed == that.Fixed) && (FixedZ == that.FixedZ) && (Unbounded == that.Unbounded) && (MaxGrade == that.MaxGrade);
	}
	bool	operator != (const HybridProfileSample & that) const	{ return !(*this == that); }

	Vector2	Position;
	float		Station;			// along-road arc length from the interval start
	float		TerrainZ;		// Phase-7 / current terrain height before deformation
	bool		Fixed;			// a compiled deck or junction elevation pins this sample
	float		FixedZ;

	/*
	**	A sample on a structure that carries its own surface -- a canyon or ravine deck -- is not on the
	**	ground, so no cut or fill interval applies there; only the grade does.  Its `TerrainZ` is then the line
	**	the structure keeps to rather than the ground under it, so the clamp walk holds the deck on the line
	**	between its two ends instead of pulling it down toward a canyon floor.
	*/
	bool		Unbounded;

	/*
	**	Q-028: the grade the step from the previous sample to this one may take, in radians, where the native terrain grid
	**	would realize the class maximum steeper than the class allows (Hybrid_Apply_Grade_Budget); 0 is the class maximum.
	*/
	float		MaxGrade;
};


/*
**	The solved profile: one height per sample, quantized to `Q`, every adjacent grade inside the class
**	maximum, and the cut and fill each sample asks of the ground for the record.
*/
struct HybridSolvedProfile
{
	HybridSolvedProfile(void) : SteepestGrade(0.0f),DeepestCut(0.0f),HighestFill(0.0f) { }

	void	Reset(void)	{ Z.Delete_All(); SteepestGrade = 0.0f; DeepestCut = 0.0f; HighestFill = 0.0f; }

	DynamicVectorClass<float>	Z;					// solved height per sample
	float								SteepestGrade;	// radians, on the quantized profile
	float								DeepestCut;		// max(terrain - z)
	float								HighestFill;	// max(z - terrain)
};


/*
**	Section 12.1.  `samples` are in station order with `Station` strictly increasing; the first and last
**	are the interval's exact endpoints.  `max_cut_depth` / `max_fill_height` are the profile's grading
**	limits and `max_grade_radians` the class maximum; a sample's own `MaxGrade`, when set, bounds the step into it
**	instead (Q-028), in all four steps.  Deterministic: the same samples always give the same profile.
*/
bool	Hybrid_Solve_Vertical_Profile(const DynamicVectorClass<HybridProfileSample> & samples,
												  float max_cut_depth,float max_fill_height,float max_grade_radians,
												  float quantum,HybridSolvedProfile * out,HybridProfileFailure * failure_out);


/*
**	Section 12.2's cross-section at one sample, as a function of signed lateral offset from the
**	centerline: the roadbed is level at `solved_z` out to `half_roadbed`, and from there the ground blends
**	to `terrain_z_at_offset` with a smoothstep over exactly `outer_blend`.  Beyond `half_roadbed +
**	outer_blend` the terrain is untouched.  `Hybrid_Road_Cross_Section_Half_Width` is that outer reach.
*/
float	Hybrid_Road_Cross_Section_Height(float lateral,float solved_z,float terrain_z_at_offset,
													 float half_roadbed,float outer_blend);
float	Hybrid_Road_Cross_Section_Half_Width(HybridRoadClass road_class,float w,float a);


/*
**	The sample spacing Section 12.1 prescribes, `min(W/2, A/3)`, and the interval sampler that builds a
**	station-ordered sample list from a route's point range with exact endpoints.  `first_point` and
**	`last_point` are indices into `route.Points`; the height callback is the current terrain.
*/
float	Hybrid_Profile_Sample_Spacing(const HybridConstants & constants);

bool	Hybrid_Sample_Route_Interval(const HybridRoute & route,int first_point,int last_point,float spacing,
												 HybridRouteHeightSampleFunc sample,void * context,
												 DynamicVectorClass<HybridProfileSample> * out);

/*
**	The same sampler over any station-ordered polyline and any station range on it: the exact position at
**	`from_station`, every whole multiple of `spacing` after it, and the exact position at `to_station`.  A
**	compiled crossing's structure ends -- a bridge abutment, an overpass deck end -- fall between polyline points,
**	and the road has to be sampled up to exactly there.  Sample stations are measured from `from_station`.
*/
bool	Hybrid_Sample_Polyline_Range(const DynamicVectorClass<Vector2> & points,
												 const DynamicVectorClass<float> & stations,float from_station,
												 float to_station,float spacing,HybridRouteHeightSampleFunc sample,
												 void * context,DynamicVectorClass<HybridProfileSample> * out);

//	The point at `station` on a station-ordered polyline, clamped to its two ends.
Vector2	Hybrid_Polyline_Point_At(const DynamicVectorClass<Vector2> & points,
											 const DynamicVectorClass<float> & stations,float station);


/*
**	A structure on a road, as Section 12.1 sees it: where it starts and stops along the road, and how it holds
**	the profile there.  A LEVEL structure -- a river deck, an overpass deck -- holds every sample on it at one
**	elevation.  A CARRIED one -- a canyon or ravine deck -- spans between the road's own
**	elevations at its two ends and holds its interior to the grade alone on the line between them; its ends are
**	pinned once a compiled record has fixed them, and solved with the road while they have not.
*/
enum HybridProfileStructureMode
{
	HYBRID_PROFILE_STRUCTURE_LEVEL = 0,
	HYBRID_PROFILE_STRUCTURE_CARRIED,
};

struct HybridProfileStructure
{
	HybridProfileStructure(void) :
		From(0.0f),To(0.0f),Mode(HYBRID_PROFILE_STRUCTURE_LEVEL),EndsFixed(false),StartZ(0.0f),EndZ(0.0f) { }

	//	DynamicVectorClass<T> requires == and !=.
	bool	operator == (const HybridProfileStructure & that) const
	{
		return (From == that.From) && (To == that.To) && (Mode == that.Mode) && (EndsFixed == that.EndsFixed) &&
				 (StartZ == that.StartZ) && (EndZ == that.EndZ);
	}
	bool	operator != (const HybridProfileStructure & that) const	{ return !(*this == that); }

	float								From;				// stations along the road, From < To
	float								To;
	HybridProfileStructureMode	Mode;
	bool								EndsFixed;		// CARRIED only: StartZ / EndZ are compiled pins
	float								StartZ;			// LEVEL: the deck elevation; CARRIED: the pinned ends
	float								EndZ;
};

/*
**	Section 12.1 over one stretch `[from_station, to_station]` of a station-ordered road centerline, with every
**	structure on it holding its own surface.  The stretch is sampled at `spacing` with its two ends and every
**	structure end inside it as exact samples of their own, then solved once.  This is the one vertical-profile
**	path on the hybrid startup path: Phase 11 asks it whether a crossing can be graded, and Phase 12 realizes
**	the road it returns.  `start_z_out` / `end_z_out` receive each structure's solved end elevations, or its
**	pins, in the order of `structures`; a structure outside the stretch reports its pins unchanged.  `budget`
**	(optional, Q-028) is the native terrain grid the road will be realized on: every sample gets its realization-aware
**	grade (Hybrid_Apply_Grade_Budget) before the one solve.
*/
struct HybridGradeBudget;

bool	Hybrid_Solve_Road_Stretch(const DynamicVectorClass<Vector2> & points,const DynamicVectorClass<float> & stations,
										  float from_station,float to_station,
										  const DynamicVectorClass<HybridProfileStructure> & structures,float spacing,
										  HybridRouteHeightSampleFunc sample,void * context,float max_cut_depth,
										  float max_fill_height,float max_grade_radians,float quantum,
										  DynamicVectorClass<HybridProfileSample> * samples_out,
										  HybridSolvedProfile * solved_out,DynamicVectorClass<float> * start_z_out,
										  DynamicVectorClass<float> * end_z_out,HybridProfileFailure * failure_out,
										  const HybridGradeBudget * budget = nullptr);


/***********************************************************************************************
**	Section 12.2 / 12.2a -- the realization plan: every road Phase 12 builds, in routing order, graded once by
**	Section 12.1 and carrying the cross section the terrain is deformed to; and every compiled bridge or overpass
**	deck as the assembly Phase 12 builds it from.  Pure computation over the frozen Phase-9..11 records: the
**	runtime owners (WorldTerrainSystem, RoadSystem, BridgeSystem) are driven from it, never the
**	other way round.
***********************************************************************************************/

//	One solved centerline sample of a realized road.
struct HybridRealizedSample
{
	HybridRealizedSample(void) :
		Position(0.0f,0.0f),Station(0.0f),Z(0.0f),Left(0.0f),Right(0.0f),CarriageLeft(0.0f),CarriageRight(0.0f),Structure(false) { }

	bool	operator == (const HybridRealizedSample & that) const	{ return (Station == that.Station) && (Position == that.Position); }
	bool	operator != (const HybridRealizedSample & that) const	{ return !(*this == that); }

	Vector2	Position;
	float		Station;
	float		Z;					// the Section-12.1 solution
	float		Left;				// the roadbed edge, shoulder included, left of the road's travel direction
	float		Right;			// ...and right of it
	float		CarriageLeft;	// the paved carriageway's edge, left and right: the roadbed less its shoulder
	float		CarriageRight;
	bool		Structure;		// strictly inside a compiled deck (its ends are on the road): not the road's ground
};


/*
**	One physical road (Q-025): a heavy family's whole trunk, an interchange ramp or crossing edge, or any other
**	edge on its own.  `Left`/`Right` are the actual paved edges: a symmetric class's half roadbed; a ramp's
**	RampLaneCenterline edges with D-022's gore transition and D-024's `E`; a HIGHWAY_4's ramp side widened to the
**	compiled merge/diverge `E(x)` over each parallel interval (12.6.11).
*/
struct HybridRealizedRoad
{
	HybridRealizedRoad(void) :
		StableId(0),EdgeId(0),RoadClass(HYBRID_ROAD_CLASS_COUNT),Tier(HYBRID_ROAD_TIER_PAVED),Capabilities(0),Group(0),
		CarriagewayWidth(0.0f),ShoulderWidth(0.0f) { }

	bool	operator == (const HybridRealizedRoad & that) const	{ return StableId == that.StableId; }
	bool	operator != (const HybridRealizedRoad & that) const	{ return !(*this == that); }

	unsigned int							StableId;			// realization order, 1-based
	unsigned int							EdgeId;				// the logical edge that names it (a trunk's first)
	DynamicVectorClass<unsigned int>	Edges;				// every logical edge it carries, in travel order
	HybridRoadClass						RoadClass;
	HybridPhysicalRoadTier				Tier;					// Section 7's physical tier
	unsigned int							Capabilities;
	int										Group;				// Hybrid_Route_Priority_Rank: 0 trunks, 1 group 1b, then the rest
	float										CarriagewayWidth;	// lanes * W + median, the width RoadSystem is handed
	float										ShoulderWidth;		// the symmetric shoulder RoadSystem is handed (12.6.13 item 1)
	DynamicVectorClass<HybridRealizedSample>	Samples;
};


/*
**	A compiled bridge or overpass deck as Phase 12 builds it: CompiledBridge (the Phase-11 record) -> assembly ->
**	ordered pieces.  Phase 11 chose the logical bridge family and its structural requirements; Phase 12 owns the
**	physical assembly.  Navigation, route independence and gameplay see one assembly as one logical crossing.
**	This version builds every assembly from exactly one WHOLE piece -- the existing single bridge/deck
**	realization -- and the other kinds are reserved for a later modular bridge family.  Piece ids and transforms
**	are ordered and part of the stage checksum, and every piece seam is a swept-validation sample.
**	Ordinals are part of the checksum: never reorder.
*/
enum HybridBridgePieceKind
{
	HYBRID_BRIDGE_PIECE_WHOLE = 0,					// the existing one-object bridge or deck realization
	HYBRID_BRIDGE_PIECE_START_ABUTMENT,
	HYBRID_BRIDGE_PIECE_END_ABUTMENT,
	HYBRID_BRIDGE_PIECE_CENTER,
	HYBRID_BRIDGE_PIECE_SHORT_CENTER,
	HYBRID_BRIDGE_PIECE_SUPPORT,						// a pier
	HYBRID_BRIDGE_PIECE_DAMAGED_TRAVERSABLE,
	HYBRID_BRIDGE_PIECE_BROKEN,
	HYBRID_BRIDGE_PIECE_COUNT
};

const char *	Hybrid_Bridge_Piece_Kind_Name(HybridBridgePieceKind kind);

struct HybridBridgePiece
{
	HybridBridgePiece(void) :
		PieceId(0),Kind(HYBRID_BRIDGE_PIECE_WHOLE),Start(0.0f,0.0f),End(0.0f,0.0f),StartZ(0.0f),EndZ(0.0f),Width(0.0f) { }

	bool	operator == (const HybridBridgePiece & that) const	{ return PieceId == that.PieceId; }
	bool	operator != (const HybridBridgePiece & that) const	{ return !(*this == that); }

	unsigned int				PieceId;				// 1-based, in deck order from Start
	HybridBridgePieceKind	Kind;
	Vector2						Start;				// the piece's transform: its two deck ends and their surface heights
	Vector2						End;
	float							StartZ;
	float							EndZ;
	float							Width;
};

struct HybridBridgeAssembly
{
	HybridBridgeAssembly(void) :
		StableId(0),InteractionId(0),Method(HYBRID_METHOD_BRIDGE),BridgeKind(-1),EdgeId(0),Start(0.0f,0.0f),End(0.0f,0.0f),
		StartZ(0.0f),EndZ(0.0f),Width(0.0f) { }

	bool	operator == (const HybridBridgeAssembly & that) const	{ return StableId == that.StableId; }
	bool	operator != (const HybridBridgeAssembly & that) const	{ return !(*this == that); }

	unsigned int				StableId;			// in the Phase-11 records' order, 1-based
	unsigned int				InteractionId;		// the compiled bridge or overpass it builds
	HybridInteractionMethod	Method;				// BRIDGE or OVERPASS
	int							BridgeKind;			// the logical family Phase 11 chose
	unsigned int				EdgeId;				// the road it carries
	Vector2						Start;				// the whole deck, end to end
	Vector2						End;
	float							StartZ;
	float							EndZ;
	float							Width;				// Section 12.4's clear width
	DynamicVectorClass<HybridBridgePiece>	Pieces;
};


/*
**	A frozen structural point (D-026): the authoritative drivable surface there -- the terrain off a deck, the deck on
**	one -- must stand at `Z` within 2Q, never relaxed by the terrain cell size.  Ordinary roadbed between pins is held
**	to the coarser realization tolerance.  The kinds are in hybridinteractions.h, beside the landing records.
*/
struct HybridRealizedPin
{
	HybridRealizedPin(void) :
		Kind(HYBRID_PIN_JUNCTION),Position(0.0f,0.0f),Z(0.0f),RoadIndex(-1),AssemblyId(0),InteractionId(0) { }

	bool	operator == (const HybridRealizedPin & that) const	{ return (Kind == that.Kind) && (Position == that.Position); }
	bool	operator != (const HybridRealizedPin & that) const	{ return !(*this == that); }

	HybridRealizedPinKind	Kind;
	Vector2						Position;
	float							Z;
	int							RoadIndex;			// the earliest road whose surface it is; a deck end's carried road
	unsigned int				AssemblyId;			// a deck end's BridgeAssembly, 0 otherwise
	unsigned int				InteractionId;		// the compiled record that froze it, 0 for a deck end
};


struct HybridRealizationPlan
{
	HybridRealizationPlan(void)	{ Reset (); }

	void	Reset(void);

	//	The physical road carrying a logical edge, or null.
	const HybridRealizedRoad *	Find_Road(unsigned int edge_id) const;
	const HybridBridgeAssembly *	Find_Bridge(unsigned int stable_id) const;

	DynamicVectorClass<HybridRealizedRoad>		Roads;			// routing order
	DynamicVectorClass<HybridBridgeAssembly>	Bridges;
	DynamicVectorClass<HybridRealizedPin>		Pins;				// every frozen structural point, in record order
	DynamicVectorClass<HybridVerticalConstraint>	Constraints;	// D-029: the fixed heights every road was graded with
	unsigned int										Checksum;
};


struct HybridRealizationParams
{
	HybridRealizationParams(void);

	HybridConstants	Constants;
	float					MaxCutDepth;
	float					MaxFillHeight;
	float					LightVehicleWidth;			// preflight metric cache
	float					HeavyVehicleWidth;
	float					HarvesterWidth;
	float					MinVehicleLaneClearance;
	float					TerrainCellSize;				// the height grid Phase 12 deforms; 0 when there is none yet
	float					TerrainOriginX;				// ...and its origin (Q-028's grade budget is taken on its triangles)
	float					TerrainOriginY;
	float					LightVehicleLength;			// the envelopes Section 12.3 sweeps, for Q-028's grade budget
	float					HeavyVehicleLength;
	float					HarvesterLength;
};

enum HybridRealizationFailure
{
	HYBRID_REALIZATION_OK = 0,
	HYBRID_REALIZATION_INVALID_INPUT,
	HYBRID_REALIZATION_ROAD,				// a routed road cannot be realized from its compiled crossings
	HYBRID_REALIZATION_PROFILE,			// Section 12.1 refuses a road with its structures and junction surfaces
	HYBRID_REALIZATION_FAILURE_COUNT
};

const char *	Hybrid_Realization_Failure_Name(HybridRealizationFailure failure);

/*
**	D-029: why one road's whole profile is infeasible, from its fixed-height constraints.
**	- OVERLAP: two constraints hold one stretch at different heights.
**	- GRADE: two constraints `Separation` apart differ by `Dz`, past the class grade.
**	- CUT_FILL: a constraint's height cannot be graded to the road's cut/fill interval at `StationB`.
**	- GROUND: no constraint takes part; the ground alone cannot be graded.
**	- OTHER: the solver failed later (clamp walk, quantization).
*/
enum HybridConstraintConflictKind
{
	HYBRID_CONFLICT_NONE = 0,
	HYBRID_CONFLICT_OVERLAP,
	HYBRID_CONFLICT_GRADE,
	HYBRID_CONFLICT_CUT_FILL,
	HYBRID_CONFLICT_GROUND,
	HYBRID_CONFLICT_OTHER,
};

struct HybridConstraintConflict
{
	HybridConstraintConflict(void) :
		Kind(HYBRID_CONFLICT_NONE),RoadEdgeId(0),HasA(false),HasB(false),StationA(0.0f),StationB(0.0f),ZA(0.0f),ZB(0.0f),
		Separation(0.0f),Dz(0.0f),RequiredGrade(0.0f),AllowedGrade(0.0f),TerrainZ(0.0f),RequiredCutFill(0.0f),
		AllowedCutFill(0.0f) { }

	HybridConstraintConflictKind	Kind;
	unsigned int						RoadEdgeId;
	bool									HasA;					// the earlier (or already held) side is a constraint
	bool									HasB;					// the later side is a constraint
	HybridVerticalConstraint		A;
	HybridVerticalConstraint		B;
	float									StationA;
	float									StationB;
	float									ZA;
	float									ZB;
	float									Separation;
	float									Dz;
	float									RequiredGrade;		// what the transition between the two sides needs
	float									AllowedGrade;		// the class maximum
	float									TerrainZ;			// CUT_FILL / GROUND: the ground where the road cannot reach
	float									RequiredCutFill;	// CUT_FILL: the cut (negative) or fill (positive) it would need
	float									AllowedCutFill;	// ...and the limit on that side
};

struct HybridRealizationResult
{
	HybridRealizationResult(void) : Failure(HYBRID_REALIZATION_OK),EdgeId(0),Profile(HYBRID_PROFILE_OK) { }

	HybridRealizationFailure	Failure;
	StringClass						Detail;
	unsigned int					EdgeId;
	HybridProfileFailure			Profile;
	HybridConstraintConflict	Conflict;				// D-029: set when the failure is a road's fixed heights
};


/*
**	D-029: the canonical vertical constraints, compiled once.  The roads are solved in routing order (trunks, group 1b,
**	the rest; Hybrid_Route_Priority_Rank then StableId), each graded whole once by Section 12.1 with every fixed height
**	on it together:
**	- its compiled decks, and its D-027 landings;
**	- at every compiled at-grade or topology junction it shares with a road solved before it, that earlier road's own
**	  solved surface over the junction's footprint (D-028);
**	- an interchange local junction at z_L, a ramp's gore at the highway's solved surface, an overpass's lower road at
**	  its compiled LowerSurfaceZ.
**	Two constraints holding one stretch at different heights are a conflict, never a choice.  `constraints_out`
**	receives every record in compile order; `plan_out` (optional) the plan built from them.  On failure
**	`result->Conflict` names the conflicting constraints.  Phase 11 runs this; Phase 12 never does.
*/
bool	Hybrid_Compile_Vertical_Constraints(const HybridRealizationParams & params,const HybridTopology & topology,
													const HybridRouteSet & routes,const HybridInteractionSet & interactions,
													HybridRouteHeightSampleFunc sample,void * context,
													DynamicVectorClass<HybridVerticalConstraint> * constraints_out,
													HybridRealizationPlan * plan_out,HybridRealizationResult * result);

/*
**	Phase 12's plan: every road graded once by Section 12.1 with exactly the constraints Phase 11 compiled
**	(`interactions.Constraints`, consumed unchanged and kept on the plan), never a junction height of its own.  A set
**	Phase 11 did not finish is refused.  A failure fails the attempt (Section 12).
*/
bool	Hybrid_Plan_Realization(const HybridRealizationParams & params,const HybridTopology & topology,
										const HybridRouteSet & routes,const HybridInteractionSet & interactions,
										HybridRouteHeightSampleFunc sample,void * context,HybridRealizationPlan * out,
										HybridRealizationResult * result);

unsigned int	Hybrid_Realization_Checksum(const HybridRealizationPlan & plan,unsigned int seed = 0);


/*
**	Section 12.2's terrain deformation over a height grid (vertex (ix, iy) at origin + spacing * (ix, iy), row-major
**	in x), road by road in plan order:
**	1-2. the roadbed and shoulder, out to the sample's actual paved edges either side, at the solved height;
**	3. a smoothstep from the paved edge to the terrain over exactly `outer_blend = max(W, A/2)`;
**	4. nothing under a deck, and no water;
**	5. never an earlier road's roadbed or shoulder -- the earlier road owns that surface, a junction's shared patch
**	   included, so a later road's blend (a ramp's at its gore) never modifies it.  Q-028: inside a compiled junction's
**	   raster support (Hybrid_Junction_Raster_Support) the earlier road's band beyond its paved edge is the junction's,
**	   graded by the junction's later road, and no third road writes any of the support.
**	Steps 6-9 belong to the runtime owners.
*/
struct HybridHeightGrid
{
	float	OriginX;
	float	OriginY;
	float	Spacing;
	int	CountX;
	int	CountY;
};

struct HybridDeformationStats
{
	HybridDeformationStats(void) : Changed(0),Protected(0),Water(0),Structure(0),Pins(0),PinPasses(0),PinResidual(0.0f) { }

	int	Changed;			// vertex writes
	int	Protected;		// left alone: an earlier road's roadbed
	int	Water;
	int	Structure;		// under a deck
	int	Pins;				// structural pins realized
	int	PinPasses;		// correction passes they took
	float	PinResidual;	// the largest pin residual left, grid heights
};

/*
**	Terrain a structure owns rather than a road (D-026).  BRIDGE_END_SUPPORT is a water vertex graded as structural
**	support under a bridge end -- never ordinary road deformation into water.
*/
enum HybridTerrainClaimKind
{
	HYBRID_TERRAIN_BRIDGE_END_SUPPORT = 0,
};

struct HybridTerrainClaim
{
	HybridTerrainClaim(void) : Kind(HYBRID_TERRAIN_BRIDGE_END_SUPPORT),Vertex(-1),AssemblyId(0) { }

	bool	operator == (const HybridTerrainClaim & that) const	{ return Vertex == that.Vertex; }
	bool	operator != (const HybridTerrainClaim & that) const	{ return !(*this == that); }

	HybridTerrainClaimKind	Kind;
	int						Vertex;				// row-major index into the height grid
	unsigned int			AssemblyId;			// the BridgeAssembly that owns it
};

/*
**	The end support footprint of a deck end (D-026): the terrain cells under the rectangle from the deck end `end`
**	(0 Start, 1 End) 1.5 grid cells along the deck, and across it on each side the wider of the deck's half width and
**	the attached road's actual paved roadbed there (shoulder, asymmetric and widened edges included; never its outer
**	blend), since the abutment carries the whole road -- `left`/`right` looking into the deck.  A vertex belongs when it
**	is no more than 1.5 cells along the deck from that end, not before it, and one of its four cells overlaps the
**	rectangle.
*/
void	Hybrid_Bridge_End_Support_Extents(const HybridRealizationPlan & plan,const HybridBridgeAssembly & assembly,int end,
												 float * left_out,float * right_out);
bool	Hybrid_Bridge_End_Support_Contains(const HybridBridgeAssembly & assembly,int end,float left,float right,float cell,
													  const Vector2 & vertex);

/*
**	The grid's surface at `point`, interpolated exactly as HeightfieldClass::Sample_Cell does: two triangles per cell,
**	split on the diagonal from the low corner to the high one.  `vertices_out` / `weights_out` (3 each, optional)
**	receive the triangle's vertex indices and their weights.
*/
bool	Hybrid_Grid_Height_At(const HybridHeightGrid & grid,const float * heights,const Vector2 & point,float * height_out,
									int * vertices_out = nullptr,float * weights_out = nullptr);

/*
**	Steps 1-5 as above, then every structural pin (plan.Pins) realized exactly: the least change to the vertices of the
**	triangle under each pin that puts the surface there at the pin's height, iterated over pins sharing vertices.  Water
**	is never changed but a deck end's own support footprint, which `claims` (optional) records as BRIDGE_END_SUPPORT.
*/
bool	Hybrid_Deform_Terrain(const HybridRealizationPlan & plan,const HybridConstants & k,const HybridHydrologySet * hydrology,
									const HybridHeightGrid & grid,float * heights,HybridDeformationStats * stats,
									DynamicVectorClass<HybridTerrainClaim> * claims = nullptr);


/*
**	D-026's realization limits, shared by Phase 11's pin judgement and Phase 12's verification so the two can never
**	disagree.  Ordinary roadbed may stand off its solved surface by the slope-limited surface over one cell diagonal;
**	a vehicle's grade over `base` may reach the class maximum plus Section 12.1 step 4's angular quantum and 2Q over the
**	base; its cross slope over `base` the class maximum and 2Q over the base.
*/
float	Hybrid_Realized_Surface_Tolerance(const HybridConstants & k,float cell,HybridRoadClass road_class);
float	Hybrid_Realized_Grade_Limit(HybridRoadClass road_class,float base,float q);
float	Hybrid_Realized_Cross_Limit(HybridRoadClass road_class,float base,float q);

//	The reference vehicles' collision envelopes Section 12.3 sweeps: light, heavy, Harvester (full extents).
enum
{
	HYBRID_ENVELOPE_LIGHT = 0,
	HYBRID_ENVELOPE_HEAVY,
	HYBRID_ENVELOPE_HARVESTER,
	HYBRID_ENVELOPE_COUNT
};

struct HybridPinEnvelopes
{
	HybridPinEnvelopes(void)	{ for (int i = 0; i < HYBRID_ENVELOPE_COUNT; ++i) { Length[i] = 0.0f; Width[i] = 0.0f; } }

	float	Length[HYBRID_ENVELOPE_COUNT];
	float	Width[HYBRID_ENVELOPE_COUNT];
};

//	Does a road with these capabilities carry envelope `e` (Section 12.3 sweeps it only with what it must carry)?
bool	Hybrid_Envelope_Swept(unsigned int capabilities,int e);


/*
**	Q-028 (option b): the realization-aware grade budget.  Section 12.3 judges a road's grade on the native terrain grid,
**	where each roadbed vertex holds the solved profile at its nearest centerline foot (12.2 step 1) and the surface between
**	vertices is the grid's two triangles per cell.  On a straight road that surface rises along the road exactly as fast as
**	the profile; on a curve the vertices' foot stations no longer vary linearly with position, and the surface between a
**	vehicle envelope's ends can rise faster than any step of the profile under it.
**
**	The budget is the exact amplification, taken from the native rasterizer's own geometry: for every envelope the road
**	must carry, at every sample Section 12.3 sweeps from, the envelope's two grade points as Pin_Sweep places them, the
**	grid triangle under each (Hybrid_Grid_Height_At), and each triangle vertex's nearest centerline foot (Deform_Roads).
**	The realized rise between the two points is then a fixed linear sum of the profile's steps, sum(b_k * dZ_k); with
**	every step |dZ_k| <= s_k * ds_k, the realized grade is at most sum(|b_k| * ds_k * s_k) / L.  Each step whose
**	envelope sums exceed the run takes the amplification A_k = max(1, sum(|b_k| * ds_k) / (L + Q / G)) of the worst
**	such envelope, and the solver grade atan(G / A_k) - Q_angle, G = tan(class + Q_angle), so that after the solver's
**	own quantization (each step within tan(grade + Q_angle)) the realized grade stays within D-026's unchanged
**	G + 2Q / L -- Q of the 2Q is left for rounding.  A straight road gives A = 1 exactly: its budget is the class.
**
**	A step with both samples fixed (a compiled hold: a deck, a landing, a junction's earlier surface), or inside a deck,
**	keeps the class maximum: its height is a compiled constraint realized by its owner, never this road's own grade.
**	An envelope whose grade points or triangle vertices stand over a deck (`Decks`) is the deck end's, D-027's.
**	So is one with a vertex whose nearest foot is another pass of the same road (beyond pi * (widest half + a cell
**	diagonal) of arc from the envelope, the farthest its own pass can put it): that height is the other pass's, never a
**	grade of this one, and no grade budget can change it.
*/
typedef void (*HybridRoadEdgesFunc)(const Vector2 & position,float station,const Vector2 & travel,float * left_out,
												float * right_out,void * context);

struct HybridGradeBudget
{
	HybridGradeBudget(void);

	float											CellSize;		// the native terrain grid; 0: none, the class maximum everywhere
	float											OriginX;
	float											OriginY;
	float											Spacing;			// Section 12.1's sample spacing, Pin_Sweep's travel direction
	float											Length[HYBRID_ENVELOPE_COUNT];	// the envelopes the road carries; 0 = not swept
	float											Width[HYBRID_ENVELOPE_COUNT];
	float											Left;				// the class's own paved edges, when `Edges` is null
	float											Right;
	HybridRoadEdgesFunc						Edges;			// the realized paved edges at a sample (Section 12.2)
	void *										EdgesContext;
	DynamicVectorClass<HybridProfileStructure>	Decks;		// deck spans, stations of the stretch's polyline
};

/*
**	Sets every sample's `MaxGrade` from `budget` over the samples' own polyline (positions and stations as sampled, so the
**	centerline the grid is deformed from).  Samples already marked Fixed / Unbounded keep the class maximum as above.
**	`amplification_out` (optional) receives A_k per sample (the step into it; 1 where the class applies).
*/
void	Hybrid_Apply_Grade_Budget(const HybridGradeBudget & budget,float max_grade_radians,float quantum,
										  DynamicVectorClass<HybridProfileSample> * samples,DynamicVectorClass<float> * amplification_out = nullptr);

//	The solver grade that keeps an amplification `a` within the class maximum: atan(tan(class + Q_angle) / a) - Q_angle.
float	Hybrid_Grade_Budget_Radians(float max_grade_radians,float amplification);


/*
**	Q-028: a compiled junction's raster support.  The junction's semantic shared surface is the later road's paved roadbed
**	over every station held at the earlier road's surface (its JUNCTION records).  The grid realizes that surface on every
**	triangle it reaches, and a triangle under it references vertices at most one cell away: a vertex belongs to the support
**	when one of its own triangles -- |x|, |y| <= cell and |x - y| <= cell about it, the low-to-high diagonal split -- meets
**	the surface.  Raster ownership only, never a height constraint (Section 12.2 step 5 below): inside the support the
**	earlier road keeps its own paved roadbed; the vertices its band claims beyond its paved edge are the junction's, graded
**	by the later road's own roadbed construction (its solved profile, the shared surface inside the hold and its own
**	profile past it), and no other road's deformation may write them.  Each vertex belongs to the first junction listing it.
*/
struct HybridJunctionSupport
{
	HybridJunctionSupport(void) : Vertex(-1),EarlierRoad(-1),LaterRoad(-1),InteractionId(0) { }
	bool	operator == (const HybridJunctionSupport & that) const	{ return Vertex == that.Vertex; }
	bool	operator != (const HybridJunctionSupport & that) const	{ return !(*this == that); }

	int				Vertex;				// row-major into the grid
	int				EarlierRoad;		// plan.Roads index whose surface the junction holds
	int				LaterRoad;			// ...and the road held on it
	unsigned int	InteractionId;
};

void	Hybrid_Junction_Raster_Support(const HybridRealizationPlan & plan,const HybridHeightGrid & grid,
												 DynamicVectorClass<HybridJunctionSupport> * out);

/*
**	D-027: can the native terrain grid realize `plan`?  Phase 11 runs Phase 12's own deformation and pin realization over
**	a patch of `grid` under every road (vertex heights read through `sample`; the patch's counts are ignored, its origin
**	and spacing are the field's) and judges the result exactly as Phase 12's Section 12.3 does:
**	- every structural pin's drivable surface within 2Q, and the ground its correction moved inside cut/fill;
**	- at every ground sample of every road, its realized surface -- centre and paved edges -- within the ordinary
**	  tolerance of the surface it stands on (its own, or the earliest road's where roadbeds overlap) and the class cross
**	  slope across it;
**	- the same samples swept with every envelope the road's capabilities require, placed as Section 12.3 places them,
**	  on paved surface or a deck, within the class grade and cross slope.
**	Each failure is one issue: a pin's when within L_pin + 2 cells of it (`Pin` its index, once per pin), else its road's
**	(`Pin` -1, once per road).  True when there is none.
*/
struct HybridRasterIssue
{
	HybridRasterIssue(void) : Pin(-1),Road(-1) { }
	bool	operator == (const HybridRasterIssue & that) const	{ return (Pin == that.Pin) && (Road == that.Road); }
	bool	operator != (const HybridRasterIssue & that) const	{ return !(*this == that); }

	int				Pin;			// plan.Pins index, -1 for an ordinary roadbed failure
	int				Road;			// plan.Roads index
	StringClass		Detail;
};

bool	Hybrid_Plan_Rasterization(const HybridRealizationPlan & plan,const HybridConstants & k,const HybridHydrologySet * hydrology,
										 const HybridHeightGrid & grid,HybridRouteHeightSampleFunc sample,void * context,float max_cut,
										 float max_fill,const HybridPinEnvelopes & envelopes,DynamicVectorClass<HybridRasterIssue> * issues);


#endif	// HYBRIDREALIZATION_H
