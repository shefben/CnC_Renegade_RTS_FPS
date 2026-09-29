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
 *                 Project Name : OpenW3D                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwphys/hybridcoarsematrix.h                  $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Hybrid world generator rewrite, item I5 -- runtime Phase 4, the coarse semantic matrices    *
 * (Procedural_World_Generator_Rewrite_Plan_FINAL_AUDITED.md Section 8 Phase 4, Section 9).    *
 * Phase 4 turns the frozen Phase-2 identity and Phase-3 roles into broad spatial intent at    *
 * spacing `S`: what is playable, what is buildable, which macro archetype band owns a region, *
 * which discs and corridors are reserved for a gameplay role, and which biome owns each land  *
 * cell.                                                                                      *
 *                                                                                             *
 * The matrix decides intent only (Section 9.2).  No road, river, cliff or building geometry   *
 * comes from a cell edge; the coarse corridor is the bounded region a later phase searches     *
 * inside.  No crossing window exists yet either: `CROSSING_WINDOW` and `RAMP_PASS_WINDOW`      *
 * reservations are inserted only after Phase 5's landforms and Phase 6's water are fixed, and  *
 * they reuse the reservation comparator declared here rather than a second copy of it.         *
 *                                                                                             *
 * Ownership: `HybridStrategicGraph` stays the one owner of every role position, which is why   *
 * this phase takes it mutable.  A hard-reservation conflict is resolved exactly as the plan     *
 * says -- by moving the lower-priority role through Section 6.1's bounded legality repair --    *
 * and the moved position is written back into the graph instead of being shadowed here.         *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef HYBRIDCOARSEMATRIX_H
#define HYBRIDCOARSEMATRIX_H

#include "always.h"
#include "vector.h"
#include "vector2.h"
#include "wwstring.h"
#include "hybridplanrecords.h"
#include "hybridplanning.h"
#include "hybridmapidentity.h"
#include "hybridstrategicgraph.h"


/*
**	Section 8 Phase 4's reservation priority order, highest first, exactly as the plan lists it.
**	The ordinal *is* the priority, so never reorder.
**
**	`OPEN_MANEUVER` is in the list but is not exclusive with roads or resources: its rank applies
**	only to immutable landform/water conflicts, which Phases 5 and 6 read from the
**	`NO_IMMUTABLE_BARRIER` layer.  A service/trail corridor has no rank of its own in the plan's
**	list because it is never a hard reservation either; it ranks with optional route corridors.
*/
enum HybridReservationPriority
{
	HYBRID_RESERVE_BASE_PROTECTED = 0,
	HYBRID_RESERVE_HEAVY_CORRIDOR,				// required heavy route-family corridor
	HYBRID_RESERVE_HOME_TIBERIUM,
	HYBRID_RESERVE_CROSSING_WINDOW,				// required window, inserted after Phases 5 and 6
	HYBRID_RESERVE_FIELD_TIBERIUM,				// contested / remote-rich
	HYBRID_RESERVE_OPEN_MANEUVER,
	HYBRID_RESERVE_OPTIONAL_CORRIDOR,			// optional route corridor, and service/trail intent
	HYBRID_RESERVE_OPTIONAL_ISLAND,
	HYBRID_RESERVE_PRIORITY_COUNT
};

const char *	Hybrid_Reservation_Priority_Name(HybridReservationPriority priority);


/*
**	One reservation.  Geometry is a capsule: `A == B` is the disc case, which is what every role
**	reserve uses, and `A != B` is a corridor.  `Exclusive` marks a reservation that no other
**	exclusive reservation may overlap -- role reserve discs -- as opposed to layered intent such as
**	a corridor, which is expected to run through the very discs it serves.
*/
struct HybridReservation
{
	HybridReservation(void) :
		StableId(0),Priority(HYBRID_RESERVE_BASE_PROTECTED),A(0.0f,0.0f),B(0.0f,0.0f),Radius(0.0f),
		SiteId(0),EdgeId(0),IntentStableId(0),RouteFamily(-1),Exclusive(false) { }

	//	DynamicVectorClass<T> (wwlib/vector.h) requires T to support == and !=.  StableId is the key.
	bool	operator == (const HybridReservation & that) const	{ return StableId == that.StableId; }
	bool	operator != (const HybridReservation & that) const	{ return !(*this == that); }

	bool	Is_Disc(void) const	{ return (A.X == B.X) && (A.Y == B.Y); }

	unsigned int					StableId;			// creation order, 1-based
	HybridReservationPriority	Priority;
	Vector2							A;						// world; capsule start, or the disc centre
	Vector2							B;						// world; capsule end, == A for a disc
	float								Radius;				// disc radius, or corridor half-width
	unsigned int					SiteId;				// owning HybridStrategicSite::StableId, 0 when none
	unsigned int					EdgeId;				// owning HybridStrategicEdge::StableId, 0 when none
	unsigned int					IntentStableId;	// owning HybridArchetypeIntent::StableId, 0 when none
	int								RouteFamily;
	bool								Exclusive;
};


/*
**	The Phase-4 priority comparator, reused unchanged when crossing-window reservations are inserted
**	after water and landform planning.  True when `a` outranks `b`; equal priorities break by
**	creation order, so the earlier record keeps its ground and the later one is the one that moves.
*/
bool	Hybrid_Reservation_Outranks(const HybridReservation & a,const HybridReservation & b);

//	Pure geometry: do the two capsules overlap at all?
bool	Hybrid_Reservations_Overlap(const HybridReservation & a,const HybridReservation & b);

/*
**	A hard conflict: both reservations are exclusive, they overlap, and neither owns the other.  A
**	corridor running through the base or field it serves is layered intent, not a conflict, and
**	Section 2.4 already has `ROAD_X_TIBERIUM` / `ROAD_X_BASE` interaction records for resolving what
**	that means on the ground.
*/
bool	Hybrid_Reservations_Conflict(const HybridReservation & a,const HybridReservation & b);


/*
**	Runtime Phase 4's output.  The matrix is the Section-2.2 record; the reservation list is what
**	later phases query and extend.
*/
struct HybridCoarseMatrices
{
	HybridCoarseMatrices(void) { Reset (); }

	void	Reset(void);

	const HybridReservation *	Find_Reservation(unsigned int stable_id) const;
	int								Count_Of_Priority(HybridReservationPriority priority) const;

	//	Is this cell inside a BASE_PROTECTED disc -- core or expansion ring?
	bool								Is_Base_Protected(int cx,int cy) const;

	HybridSemanticMatrix								Matrix;
	DynamicVectorClass<HybridReservation>		Reservations;

	//	Section 4.3's divider as Phase 4 resolved it: the shift iterations it needed, or the straight
	//	`u = 0` fallback.  Empty/absent on a single-biome map.
	HybridBiomeDivider	Divider;
	int						DividerShifts;				// 0..6; -1 when the straight u=0 fallback was used
	float						PrimaryLandFraction;		// of playable land cells
	float						SecondaryLandFraction;

	int						PlayableCells;
	int						PlaceableCells;
	int						RoleMoves;					// Section 6.1 repairs this phase applied
	unsigned int			Checksum;					// Phase 4 gate value
};


/*
**	Phase 4's inputs beyond the frozen identity/intent/graph records.  Plain values only, so the
**	builder is reproducible from (params, identity, intents, graph) plus the configured placeable
**	boundary.
*/
struct HybridCoarseMatrixParams
{
	HybridCoarseMatrixParams(void);

	unsigned int		MapPlanSeed;			// SEED_DOMAIN_MAP_PLAN, already derived
	unsigned int		BiomeSeed;				// SEED_DOMAIN_BIOMES, already derived
	HybridConstants	Constants;

	//	World rectangle, in world units.  The matrix origin is the rectangle's minimum corner.
	float					WorldMinX;
	float					WorldMinY;
	float					WorldWidth;
	float					WorldHeight;

	float					BoundaryMargin;		// plan.boundary_margin_cells * world.chunk_size
	float					FieldRadiusMin;		// resource.field_radius_min -- the Tiberium field core
	float					FieldRadiusMax;		// resource.field_radius_max -- every Tiberium role's reserve
	float					RiverWidth;				// water.river_width
	int					TierCount;				// plan.tier_count; middle tier is floor((tier_count-1)/2)

	//	Section 16.3 arena and Section 16.1 counters.  Optional: with no arena the matrix owns its
	//	own block, which is what a self-check or an offline tool gets.
	HybridScratchArena *		Arena;
	HybridWorkCounters *		Counters;
};


/*
**	Phase 4 failures.  Each is a named attempt failure: the manager retries with a new seed.
*/
enum HybridCoarseMatrixFailure
{
	HYBRID_COARSE_OK = 0,
	HYBRID_COARSE_INVALID_INPUT,						// no graph, no spacing, or a degenerate world rectangle
	HYBRID_COARSE_SCRATCH_EXHAUSTED,					// the arena cannot hold the Section-2.2 layers
	HYBRID_COARSE_OPEN_MANEUVER_UNPLACEABLE,		// Section 6.9: a required disc fails the 95% rule after repair
	HYBRID_COARSE_UNRESOLVED_CONFLICT,				// a hard-reservation conflict survived Section 6.1 repair
	HYBRID_COARSE_BASE_ON_HARD_INTENT,				// gate: a BASE_PROTECTED disc overlaps water/cliff intent
	HYBRID_COARSE_ROLE_OUTSIDE_RESERVATION,		// gate: a required role centre is not in its reserved region
	HYBRID_COARSE_FAILURE_COUNT
};

const char *	Hybrid_Coarse_Matrix_Failure_Name(HybridCoarseMatrixFailure failure);

struct HybridCoarseMatrixResult
{
	HybridCoarseMatrixResult(void) : Failure(HYBRID_COARSE_OK) { }

	HybridCoarseMatrixFailure	Failure;
	StringClass						Detail;
};


/*
**	Runtime Phase 4.  Builds the reservation set from the Phase-3 roles, resolves hard-reservation
**	conflicts by moving the lower-priority role (writing the repaired position back into `graph`),
**	then populates the Section-2.2 layers in the plan's exact ten-step order and checks the phase
**	gate.  Returns false with result->Failure set on the first failure.
*/
bool	Hybrid_Build_Coarse_Matrices(const HybridCoarseMatrixParams & params,const HybridMapIdentity & identity,
												const HybridArchetypeIntentSet & intents,HybridStrategicGraph * graph,
												HybridCoarseMatrices * out,HybridCoarseMatrixResult * result);

/*
**	The Phase 4 gate value: a quantized digest of the matrix dimensions, every layer in row-major
**	order, every reservation and the resolved biome divider.  Hybrid_Build_Coarse_Matrices stores it
**	in out->Checksum.
*/
unsigned int	Hybrid_Coarse_Matrix_Checksum(const HybridCoarseMatrices & matrices,unsigned int seed = 0);


#endif // HYBRIDCOARSEMATRIX_H
