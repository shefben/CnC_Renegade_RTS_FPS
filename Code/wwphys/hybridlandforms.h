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
 *                     $Archive:: /Commando/Code/wwphys/hybridlandforms.h                     $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Hybrid world generator rewrite, item I6 -- runtime Phase 5, the vector landforms             *
 * (Procedural_World_Generator_Rewrite_Plan_FINAL_AUDITED.md Section 8 Phase 5, Section 10).    *
 * Geography becomes explicit vectors here: hills, ridges, explicit cliff runs and canyon/      *
 * ravine systems, each one an accepted record with its own compact-support field.  No          *
 * heightfield is written -- Phase 7 owns the terrain -- and no crossing window is created,     *
 * because water is not fixed until Phase 6.                                                   *
 *                                                                                             *
 * What this phase reads is already frozen: the Phase-2 identity and Section-6.10 intents, the  *
 * Phase-3 roles and the Phase-4 coarse matrices.  It re-derives no reserve of its own: the     *
 * two exclusions the plan names come straight out of Phase 4's layers -- BASE_PROTECTED        *
 * (BASE_CORE / BASE_EXPANSION) and Section 6.9's NO_IMMUTABLE_BARRIER -- which is also why     *
 * the matrices are taken mutable: LandformMask and RoughnessClass are the two Section-2.2      *
 * layers Phase 4 deliberately leaves clear for this phase to own, along with the HILL /        *
 * RIDGE / PLATEAU / CLIFF_BAND / CANYON_BAND / RAVINE_BAND flag bits.                          *
 *                                                                                             *
 * The provisional height every later phase needs -- Phase 6's Priority-Flood surface and       *
 * Phase 7's `landform` term -- is Hybrid_Landform_Height, so the sum of the accepted fields    *
 * has exactly one owner and no phase re-implements the Section-10.3 profile or the             *
 * Section-10.4 plateau flattening.                                                            *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef HYBRIDLANDFORMS_H
#define HYBRIDLANDFORMS_H

#include "always.h"
#include "vector.h"
#include "vector2.h"
#include "wwstring.h"
#include "hybridplanrecords.h"
#include "hybridplanning.h"
#include "hybridmapidentity.h"
#include "hybridstrategicgraph.h"
#include "hybridcoarsematrix.h"


/*
**	The Section 8 Phase 5 landform kinds.  The ordinal is part of every deterministic draw key for
**	a feature of that kind, so never reorder.  A plateau is not a kind: Section 10.4's plateau is a
**	conversion of an accepted hill or ridge and stays that feature's own record.
*/
enum HybridLandformKind
{
	HYBRID_LANDFORM_HILL = 0,
	HYBRID_LANDFORM_RIDGE,
	HYBRID_LANDFORM_CLIFF,
	HYBRID_LANDFORM_CANYON,
	HYBRID_LANDFORM_RAVINE,
	HYBRID_LANDFORM_KIND_COUNT
};

const char *	Hybrid_Landform_Kind_Name(HybridLandformKind kind);

/*
**	Is this kind an immutable barrier?  Section 8 Phase 5's rejection rules and Section 6.9's
**	NO_IMMUTABLE_BARRIER layer apply to cliff, canyon and ravine walls; a hill or a ridge is
**	ordinary relief and may stand inside an OPEN_MANEUVER disc.
*/
bool	Hybrid_Landform_Is_Immutable(HybridLandformKind kind);

//	Is this kind a line feature -- four controls plus a centripetal Catmull-Rom centerline?
bool	Hybrid_Landform_Is_Line(HybridLandformKind kind);


/*
**	Phase 5's own encoding of the two Section-2.2 layers Phase 4 leaves clear.
**
**	`LandformMask` is composable, one byte per coarse cell: a cell may be both a ridge flank and a
**	canyon wall.  `RoughnessClass` is a single value per cell and the *maximum* wins, because the
**	roughest thing standing in a cell is what decoration, material and tactical passes must respect.
*/
enum HybridLandformMaskBit
{
	HYBRID_LANDFORM_MASK_HILL			= 1 << 0,
	HYBRID_LANDFORM_MASK_RIDGE			= 1 << 1,
	HYBRID_LANDFORM_MASK_PLATEAU_TOP	= 1 << 2,
	HYBRID_LANDFORM_MASK_CLIFF_FACE	= 1 << 3,
	HYBRID_LANDFORM_MASK_CANYON_FLOOR	= 1 << 4,
	HYBRID_LANDFORM_MASK_CANYON_WALL	= 1 << 5,
	HYBRID_LANDFORM_MASK_RAVINE_FLOOR	= 1 << 6,
	HYBRID_LANDFORM_MASK_RAVINE_WALL	= 1 << 7,
};

enum HybridRoughnessClass
{
	HYBRID_ROUGHNESS_FLAT = 0,			// no landform field reaches this cell
	HYBRID_ROUGHNESS_ROLLING,			// hill/ridge flank, plateau top, canyon or ravine floor
	HYBRID_ROUGHNESS_STEEP,				// ridge core, canyon or ravine wall
	HYBRID_ROUGHNESS_BROKEN,			// explicit cliff face
	HYBRID_ROUGHNESS_CLASS_COUNT
};

const char *	Hybrid_Roughness_Class_Name(HybridRoughnessClass roughness);


/*
**	One accepted landform.  Every scalar was drawn exactly once from its Section-8 Phase-5 range
**	with a key of (terrain domain, kind, StableId, parameter name); a retry moved the feature and
**	never redrew any of this.
**
**	Geometry is the same for all five kinds: Center plus, for a line feature, the four local
**	controls rotated into world space and the centripetal Catmull-Rom centerline sampled at A/2.
**	A hill has no centerline; its field is the compact-support ellipse described by RadiusX/RadiusY
**	and Orientation.
*/
struct HybridLandform
{
	HybridLandform(void) :
		StableId(0),Kind(HYBRID_LANDFORM_HILL),SystemId(0),ParentStableId(0),IntentStableId(0),
		Required(false),IsPlateau(false),UpperSideSign(1),Center(0.0f,0.0f),
		BoundsMin(0.0f,0.0f),BoundsMax(0.0f,0.0f),
		RadiusX(0.0f),RadiusY(0.0f),Orientation(0.0f),Length(0.0f),
		Amplitude(0.0f),Depth(0.0f),HeightDifference(0.0f),
		InfluenceHalfWidth(0.0f),FloorHalfWidth(0.0f),RimHalfWidth(0.0f),
		EdgeWidth(0.0f),TalusWidth(0.0f),
		SaddlePresent(false),SaddleStation(0.0f),SaddleFlatHalfLength(0.0f),SaddleRampLength(0.0f),
		CandidateAttempts(0)
	{
		Controls[0].Set (0.0f,0.0f);
		Controls[1].Set (0.0f,0.0f);
		Controls[2].Set (0.0f,0.0f);
		Controls[3].Set (0.0f,0.0f);
	}

	//	DynamicVectorClass<T> (wwlib/vector.h) requires T to support == and !=.  StableId is the key.
	bool	operator == (const HybridLandform & that) const	{ return StableId == that.StableId; }
	bool	operator != (const HybridLandform & that) const	{ return !(*this == that); }

	unsigned int			StableId;			// creation order, 1-based; keys every draw this feature owns
	HybridLandformKind	Kind;

	/*
	**	Section 8 Phase 5: `ravine target` counts independent canyon/ravine *systems*.  Every member
	**	of one system shares SystemId; a Section-6.8 branch also records its parent.  Hills, ridges
	**	and cliffs are never systems and leave both at 0.
	*/
	unsigned int			SystemId;
	unsigned int			ParentStableId;

	unsigned int			IntentStableId;	// HybridArchetypeIntent::StableId, 0 for a generic feature
	bool						Required;			// an archetype principal feature, which is never discarded
	bool						IsPlateau;			// Section 10.4 conversion of this hill/ridge
	int						UpperSideSign;		// cliff only: PickSign(Hash01(terrain,"cliff_upper_side",id))

	Vector2					Center;
	Vector2					BoundsMin;			// full influence/protected polygon AABB
	Vector2					BoundsMax;

	//	Hill ellipse.
	float						RadiusX;				// drawn hill radius
	float						RadiusY;				// RadiusX * aspect ratio
	float						Orientation;		// radians; hill ellipse axis, or the line feature's axis

	//	Line features.
	float						Length;				// L
	Vector2					Controls[4];		// world, at local -0.50L, -0.1666667L, +0.1666667L, +0.50L

	//	Vertical parameters.  Exactly one of these is non-zero per kind.
	float						Amplitude;			// hill/ridge
	float						Depth;				// canyon/ravine, positive magnitude of a downward field
	float						HeightDifference;	// cliff

	//	Cross-section parameters.
	float						InfluenceHalfWidth;	// ridge
	float						FloorHalfWidth;		// canyon/ravine
	float						RimHalfWidth;			// canyon/ravine
	float						EdgeWidth;				// cliff, Section 10.2
	float						TalusWidth;				// cliff rock/talus transition, Section 10.2

	/*
	**	Section 6.10's mandatory SURFACE_SADDLE, on the `highland_pass` principal ridge only.
	**	SaddleStation is the along-centerline arc length of the saddle point; the amplitude
	**	multiplier is 0.15 inside SaddleFlatHalfLength, lerps to 1 across SaddleRampLength and is 1
	**	beyond.  The flat half-length starts at 4W and is widened symmetrically until the footprint
	**	fits a PRIMARY_2 roadbed plus 2 * fairness.min_vehicle_lane_clearance.
	*/
	bool						SaddlePresent;
	float						SaddleStation;
	float						SaddleFlatHalfLength;
	float						SaddleRampLength;

	int						CandidateAttempts;	// candidate cells / stations consumed before acceptance

	DynamicVectorClass<Vector2>	Points;		// centerline sampled at A/2, quantized; empty for a hill

	//	Along-centerline arc length of Points[i], and the total.  Empty for a hill.
	DynamicVectorClass<float>		Stations;

	float	Centerline_Length(void) const	{ return (Stations.Count () > 0) ? Stations[Stations.Count () - 1] : 0.0f; }

	//	The outer half-width of this feature's full influence polygon, measured from its centerline.
	float	Influence_Half_Width(void) const;
};


/*
**	Runtime Phase 5's output.  `Hash` is the Section-2.3 ProceduralPlanningSpatialHash over the
**	accepted landform AABBs, in coordinates relative to the world rectangle's minimum corner: the
**	candidate overlap test queries it rather than scanning every accepted feature, which is Section
**	16.2's rule, and Phases 6 and 7 inherit the same hash.  Hybrid_Landform_Height needs no hash --
**	it rejects on each feature's own AABB, and a map has a bounded number of landforms.
*/
struct HybridLandformSet
{
	HybridLandformSet(void)	{ Reset (); }

	void	Reset(void);

	const HybridLandform *	Find(unsigned int stable_id) const;
	int							Count_Of_Kind(HybridLandformKind kind) const;

	//	Independent canyon/ravine systems -- the Section-5 quota unit, not the branch count.
	int							Count_Of_Systems(void) const;

	DynamicVectorClass<HybridLandform>	Landforms;
	ProceduralPlanningSpatialHash			Hash;

	//	Section 8 Phase 5 target counts this attempt had to satisfy, kept for the gate and the log.
	int			TargetHills;
	int			TargetRidges;
	int			TargetCliffs;
	int			TargetSystems;

	int			PlateauCount;
	int			SystemCount;
	int			Rejections;			// candidates refused by the Phase-5 rejection rules
	int			Moves;				// Section 6.1 repair offsets accepted for required features
	unsigned int	Checksum;		// Phase 5 gate value
};


/*
**	Phase 5's inputs beyond the frozen identity/intent/graph/matrix records.  Plain values only, so
**	the builder is reproducible from (params, identity, intents, graph, matrices) plus the
**	configured placeable boundary.
*/
struct HybridLandformParams
{
	HybridLandformParams(void);

	unsigned int		TerrainSeed;			// SEED_DOMAIN_TERRAIN, already derived
	HybridConstants	Constants;

	//	World rectangle, in world units; the matrix origin is the rectangle's minimum corner.
	float					WorldMinX;
	float					WorldMinY;
	float					WorldWidth;
	float					WorldHeight;

	//	Section 8 Phase 5 elevation envelope.  Min/MaxElevation are relative to SeaLevel, exactly as
	//	the profile stores them, and every amplitude/depth is clamped so the provisional height stays
	//	inside them.
	float					SeaLevel;
	float					MinElevation;
	float					MaxElevation;
	float					TierStep;
	int					TierCount;

	//	Section 4 terrain weights the Phase-5 target counts and ranges consume.
	float					MacroRelief;
	float					HillDensity;
	float					MountainDensity;
	float					InteriorCliffDensity;
	float					ValleyDensity;
	float					PlateauChance;

	/*
	**	Section 17's bounded local retry limit.  Phase 5's only local retry is the alternate candidate
	**	cell, so `retry.max_local_retries_per_stage` has nothing to bound here and is not taken.
	*/
	int					MaxAlternateCandidatesPerFeature;

	//	Section 6.10's surface-saddle footprint rule.
	float					MinVehicleLaneClearance;

	//	Section 16.3 arena and Section 16.1 counters.  Optional: with no arena the candidate arrays
	//	come from the ordinary heap, which is what a self-check or an offline tool gets.
	HybridScratchArena *		Arena;
	HybridWorkCounters *		Counters;
};


/*
**	Phase 5 failures.  Each is a named attempt failure: the manager retries with a new seed.
*/
enum HybridLandformFailure
{
	HYBRID_LANDFORM_OK = 0,
	HYBRID_LANDFORM_INVALID_INPUT,					// no matrix, no spacing, or a degenerate world rectangle
	HYBRID_LANDFORM_SCRATCH_EXHAUSTED,			// the arena cannot hold the candidate arrays
	HYBRID_LANDFORM_NO_CANDIDATE_CELL,			// no placeable cell outside the hard reserves exists at all
	HYBRID_LANDFORM_CANDIDATES_EXHAUSTED,		// a generic feature used max_alternate_candidates_per_feature
	HYBRID_LANDFORM_REQUIRED_UNPLACEABLE,		// an archetype principal feature survived no Section-6.1 offset
	HYBRID_LANDFORM_ELEVATION_LIMIT,				// clamping to min/max elevation collapses a required depth
	HYBRID_LANDFORM_COUNT_MISMATCH,				// gate: target hill/ridge/cliff/system counts
	HYBRID_LANDFORM_QUALIFICATION_FAILED,		// gate: a Section-5 minimum qualification
	HYBRID_LANDFORM_IMMUTABLE_IN_RESERVE,		// gate: BASE_PROTECTED or NO_IMMUTABLE_BARRIER violation
	HYBRID_LANDFORM_FAILURE_COUNT
};

const char *	Hybrid_Landform_Failure_Name(HybridLandformFailure failure);

struct HybridLandformResult
{
	HybridLandformResult(void) : Failure(HYBRID_LANDFORM_OK) { }

	HybridLandformFailure	Failure;
	StringClass					Detail;
};


/*
**	Section 8 Phase 5's deterministic target counts.
**
**	  hills   = 3 / 6 / 10 + RoundHalfUp(hill_density * (2 / 4 / 6))
**	  ridges  = 1 / 2 / 3  + RoundHalfUp(mountain_density * (1 / 2 / 3))
**	  cliffs  = Section-5 quota (1 / 2 / 4) + RoundHalfUp(interior_cliff_density * (1 / 2 / 4))
**	  systems = Section-5 quota (1 / 1 / 2) + RoundHalfUp(valley_density * (0 / 1 / 2))
**
**	Archetype-required features count toward these totals.
*/
int	Hybrid_Hill_Target_Count(HybridSizeClass size_class,float hill_density);
int	Hybrid_Ridge_Target_Count(HybridSizeClass size_class,float mountain_density);
int	Hybrid_Cliff_Target_Count(HybridSizeClass size_class,float interior_cliff_density);
int	Hybrid_Ravine_System_Target_Count(HybridSizeClass size_class,float valley_density);

//	Section 6.8's secondary branch count for `ravine_network`: exactly 2 / 3 / 5.
int	Hybrid_Ravine_Branch_Count(HybridSizeClass size_class);

/*
**	Section 5 minimum qualifications, so the gate and any later validator share one owner.
*/
float	Hybrid_Cliff_Min_Length(float w,float d);						// max(8W, 0.12D)
float	Hybrid_Ravine_Min_Length(float w,float d);					// max(12W, 0.20D)
float	Hybrid_Ravine_Min_Depth(float tier_step);						// 0.75 * tier_step


/*
**	Runtime Phase 5.  Creates the landforms in the plan's exact order -- archetype principal
**	features, remaining canyon/ravine systems, explicit cliff runs, remaining ridges, hills, then
**	the optional plateau conversion -- writing LandformMask, RoughnessClass and the landform flag
**	bits into `matrices` as each feature is accepted, and checks the phase gate.  Returns false with
**	result->Failure set on the first failure.
*/
bool	Hybrid_Build_Landforms(const HybridLandformParams & params,const HybridMapIdentity & identity,
										const HybridArchetypeIntentSet & intents,const HybridStrategicGraph & graph,
										HybridCoarseMatrices * matrices,HybridLandformSet * out,
										HybridLandformResult * result);

/*
**	The Section-8 Phase-7 `landform` term: the sum of every accepted Phase-5 field at `world`.
**
**	Hills, ridges and cliffs sum.  Canyon and ravine members of one system combine by Section
**	10.3's rule instead -- the minimum (deepest) contribution followed by one A-wide smooth union
**	-- and the resulting per-system depths then sum with everything else.  Positive is up.
*/
float	Hybrid_Landform_Height(const HybridLandformSet & set,const Vector2 & world,float grid_spacing);

/*
**	One feature's own contribution at `world`, and its normalized influence profile in [0,1].
**	Exposed because the gate, the mask stamping and the height evaluator must all use exactly one
**	implementation of the Section-10.3 / 10.4 profiles.
*/
float	Hybrid_Landform_Contribution(const HybridLandform & landform,const Vector2 & world,float * profile_out);

/*
**	Is `world` inside any accepted *immutable* landform's field -- an explicit cliff, canyon or ravine
**	wall?  Every later phase asks this question (Phase 6's approaches, Phase 7's protected profiles,
**	Phase 8's anchors, Phase 9's corridors), so it has one owner here rather than a copy per phase.
*/
bool	Hybrid_Point_In_Immutable_Landform(const HybridLandformSet & set,const Vector2 & world);


/*
**	The Phase 5 gate value: a quantized digest of every accepted landform in StableId order.
**	Hybrid_Build_Landforms stores it in out->Checksum.
*/
unsigned int	Hybrid_Landform_Checksum(const HybridLandformSet & set,unsigned int seed = 0);


#endif // HYBRIDLANDFORMS_H
