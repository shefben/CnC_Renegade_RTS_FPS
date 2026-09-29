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
 *                     $Archive:: /Commando/Code/wwphys/hybridanchors.h                       $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Hybrid world generator rewrite, item I9 -- runtime Phase 8, validate and freeze the base and  *
 * Tiberium anchors (Procedural_World_Generator_Rewrite_Plan_FINAL_AUDITED.md Section 8 Phase 8, *
 * Section 0.5, Section 0.10).                                                                  *
 *                                                                                             *
 * This phase draws nothing and moves nothing.  Phase 3 chose every role centre and Phase 4      *
 * reserved it before geology and hydrology existed; Phase 8's whole job is to decide whether     *
 * the ground Phase 7 actually realized still satisfies the hard requirements, and to fail the    *
 * attempt when it does not.  Moving a centre here would move it into terrain nothing reserved.  *
 *                                                                                             *
 * What it freezes is the record later phases treat as authoritative: each base's centre and     *
 * `BASE_PROTECTED` radius exactly as Phase 4 set them, and the sorted list of viable heavy exit *
 * sectors Phase 9 assigns entrances from.  The provisional fairness numbers it computes are the  *
 * Section-0.10 ones only -- `PlanningDistance` over the frozen strategic graph -- because routed *
 * road distance does not exist until Phase 18 and Phase 19 owns the authoritative gate.         *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef HYBRIDANCHORS_H
#define HYBRIDANCHORS_H

#include "always.h"
#include "vector.h"
#include "vector2.h"
#include "wwstring.h"
#include "hybridplanning.h"
#include "hybridmapidentity.h"
#include "hybridstrategicgraph.h"
#include "hybridcoarsematrix.h"
#include "hybridlandforms.h"
#include "hybridhydrology.h"


//	Section 8 Phase 8: sixteen equal sectors, ordinal 0 along strategic `+u`, counter-clockwise.
#define HYBRID_EXIT_SECTOR_COUNT		16


/*
**	One validated and frozen base anchor.
*/
struct HybridBaseAnchor
{
	HybridBaseAnchor(void) :
		SiteId(0),Team(-1),Center(0.0f,0.0f),ProtectedRadius(0.0f),CoreRadius(0.0f),
		ExpansionArea(0.0f),MaxCoreSlopeRadians(0.0f),ViableSectorMask(0) { }

	//	DynamicVectorClass<T> (wwlib/vector.h) requires T to support == and !=.  SiteId is the key.
	bool	operator == (const HybridBaseAnchor & that) const	{ return SiteId == that.SiteId; }
	bool	operator != (const HybridBaseAnchor & that) const	{ return !(*this == that); }

	bool	Sector_Is_Viable(int ordinal) const
	{
		return ((ordinal >= 0) && (ordinal < HYBRID_EXIT_SECTOR_COUNT) &&
				  ((ViableSectorMask & (1u << ordinal)) != 0));
	}

	unsigned int	SiteId;					// HybridStrategicSite::StableId of the base
	int				Team;						// 0 GDI, 1 Nod
	Vector2			Center;					// frozen: exactly the Phase-3 centre Phase 4 reserved
	float				ProtectedRadius;		// frozen: R_BASE_PROTECTED, exactly Phase 4's reservation radius
	float				CoreRadius;				// R_BASE, the starting-building proxy area this phase checks slope over

	float				ExpansionArea;			// measured clear buildable area inside the protected disc
	float				MaxCoreSlopeRadians;	// the steepest realized slope over the proxy area

	unsigned int	ViableSectorMask;		// bit i set when exit sector ordinal i is heavy-viable

	//	The frozen sorted viable-sector ordinals Phase 9 assigns base entrances from.
	DynamicVectorClass<int>	ViableSectors;
};


/*
**	One validated resource anchor.  Phase 13 may shrink the eventual crystal core inside this already
**	protected maximum reserve; Phase 8 never moves the centre.
*/
struct HybridResourceAnchor
{
	HybridResourceAnchor(void) :
		SiteId(0),Role(HYBRID_SITE_REMOTE_TIBERIUM),Team(-1),Center(0.0f,0.0f),ReserveRadius(0.0f),
		ApproachEdgeId(0),ApproachCount(0) { }

	bool	operator == (const HybridResourceAnchor & that) const	{ return SiteId == that.SiteId; }
	bool	operator != (const HybridResourceAnchor & that) const	{ return !(*this == that); }

	unsigned int	SiteId;
	HybridSiteRole	Role;
	int				Team;						// 0 GDI, 1 Nod, -1 neutral
	Vector2			Center;					// frozen
	float				ReserveRadius;			// resource.field_radius_max, the protected maximum reserve

	unsigned int	ApproachEdgeId;			// the first Phase-3 route corridor that reaches the reserve edge
	int				ApproachCount;			// how many of its Phase-3 corridors can reach it at all
};


/*
**	Runtime Phase 8's output.  `Frozen` says the records in here are the authority later phases read
**	for a base centre, its protected radius and its viable exit sectors.
*/
struct HybridAnchorSet
{
	HybridAnchorSet(void)	{ Reset (); }

	void	Reset(void);

	const HybridBaseAnchor *		Find_Base(unsigned int site_id) const;
	const HybridBaseAnchor *		Base_Of_Team(int team) const;
	const HybridResourceAnchor *	Find_Resource(unsigned int site_id) const;
	int									Count_Of_Role(HybridSiteRole role) const;

	DynamicVectorClass<HybridBaseAnchor>		Bases;
	DynamicVectorClass<HybridResourceAnchor>	Resources;

	/*
	**	Section 0.10 provisional fairness, over `PlanningDistance` on the frozen strategic graph.
	**	Times are seconds at `V_REF`; a negative distance means "not connected on the graph".
	*/
	float			HomeFieldDistance[2];			// 0 GDI, 1 Nod -- base to its own HOME field
	float			HomeFieldTime[2];
	float			FrontDistance[2];					// base to its nearest CONTESTED field
	float			FrontTime[2];
	float			BaseToBaseDistance;
	float			HomeTimeDifference;
	float			FrontTimeDifference;

	bool			Frozen;
	unsigned int	Checksum;						// Phase 8 gate value
};


/*
**	Phase 8's inputs beyond the frozen Phase 2-7 records.  Plain values only; there is no seed here
**	because Phase 8 makes no draw of any kind.
*/
struct HybridAnchorParams
{
	HybridAnchorParams(void);

	HybridConstants	Constants;

	float					SeaLevel;
	float					MaxBuildableSlopeRadians;	// terrain.max_buildable_slope_degrees, in radians
	float					FieldRadiusMax;				// resource.field_radius_max

	//	Section 0.10's fairness contract: the thresholds Phase 8 is allowed to apply, and V_REF.
	float					ReferenceVehicleSpeed;		// V_REF, world units per second
	float					MaxInitialResourceTravelTimeDifferenceSeconds;
	float					MaxBaseToFrontTravelTimeDifferenceSeconds;
	float					MinBaseExpansionArea;
	int					MinVehicleApproachCount;
	float					MinVehicleLaneClearance;

	//	Step "possible heavy exit sectors": the swept rectangle's width comes from the heavy envelope.
	float					HeavyVehicleWidth;
};


/*
**	Phase 8 failures.  Each is a named attempt failure: the manager retries with a new seed, because
**	nothing in this phase may move a centre to rescue one.
*/
enum HybridAnchorFailure
{
	HYBRID_ANCHOR_OK = 0,
	HYBRID_ANCHOR_INVALID_INPUT,				// no graph/matrix, no sampler, or degenerate constants
	HYBRID_ANCHOR_MISSING_ROLE,				// a Section-5 mandatory role is not in the graph
	HYBRID_ANCHOR_BASE_NOT_PLACEABLE,		// the protected disc is not entirely placeable land
	HYBRID_ANCHOR_BASE_HARD_OVERLAP,			// water, cliff, canyon or ravine reaches the protected disc
	HYBRID_ANCHOR_BASE_TOO_STEEP,				// the starting-building proxy area is past the buildable slope
	HYBRID_ANCHOR_BASE_NO_EXPANSION,			// clear expansion area below fairness.min_base_expansion_area
	HYBRID_ANCHOR_BASE_NO_EXITS,				// fewer viable heavy exit sectors than the fairness minimum
	HYBRID_ANCHOR_RESOURCE_ILLEGAL,			// a field reserve is wet, unplaceable, or inside a wall/base
	HYBRID_ANCHOR_RESOURCE_NO_APPROACH,		// no Phase-3 corridor reaches the reserve edge
	HYBRID_ANCHOR_FAIRNESS_HOME,				// provisional home-field travel-time difference
	HYBRID_ANCHOR_FAIRNESS_FRONT,				// provisional base-to-front travel-time difference
	HYBRID_ANCHOR_BASE_TO_BASE_ZERO,			// the strategic base-to-base planning distance is zero
	HYBRID_ANCHOR_FAILURE_COUNT
};

const char *	Hybrid_Anchor_Failure_Name(HybridAnchorFailure failure);

struct HybridAnchorResult
{
	HybridAnchorResult(void) : Failure(HYBRID_ANCHOR_OK) { }

	HybridAnchorFailure	Failure;
	StringClass				Detail;
};


/*
**	Runtime Phase 8.  Validates the frozen Phase-3 centres against the realized Phase-7 ground through
**	`sample`, freezes each base's centre, protected radius and sorted viable exit sectors, computes the
**	Section-0.10 provisional fairness numbers and checks the phase gate.  Returns false with
**	result->Failure set on the first failure, leaving `out` empty.
**
**	`sample` reads the realized heightfield at a world point and returns false outside it, exactly as
**	Phase 7's gate sampler does, so this phase never assumes a particular heightfield owner.
*/
typedef bool (*HybridAnchorHeightSampleFunc)(const Vector2 & world,float * height_out,void * context);

bool	Hybrid_Validate_Anchors(const HybridAnchorParams & params,const HybridMapIdentity & identity,
											const HybridStrategicGraph & graph,const HybridCoarseMatrices & matrices,
											const HybridLandformSet & landforms,const HybridHydrologySet & hydrology,
											HybridAnchorHeightSampleFunc sample,void * context,
											HybridAnchorSet * out,HybridAnchorResult * result);

/*
**	The world direction of exit-sector ordinal `ordinal`: ordinal 0 along strategic `+u`, increasing
**	counter-clockwise by 22.5 degrees.  Exposed because Phase 9 assigns entrances from the same
**	ordinals this phase froze and must not re-derive the convention.
*/
Vector2	Hybrid_Exit_Sector_Direction(const HybridStrategicFrame & frame,int ordinal);

/*
**	The Phase 8 gate value: a quantized digest of every frozen anchor, its sector list and the
**	provisional fairness numbers.  Hybrid_Validate_Anchors stores it in out->Checksum.
*/
unsigned int	Hybrid_Anchor_Checksum(const HybridAnchorSet & set,unsigned int seed = 0);


#endif // HYBRIDANCHORS_H
