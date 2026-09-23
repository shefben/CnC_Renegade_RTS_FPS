/*
**	Command & Conquer Renegade(tm)
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
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/tunnelplanner.h                       $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	V2 roadmap Section 34 Phase 10 -- tunnels, caves, underpasses and portal integration.        *
 *	Sections 12 and 12A.                                                                        *
 *                                                                                             *
 *	Consume, resolve, realize through the one tunnel system -- the same shape BridgePlanner has.   *
 *	Candidates come from what earlier phases already decided, never re-derived: a road crossing    *
 *	BridgePlanner could not resolve (its own fallback stage 3, "alternate crossing type", which      *
 *	this phase is), and every STRATEGIC_NODE_TUNNEL_CANDIDATE an interior cliff produced that no      *
 *	road passes through (an infantry flank or cave shortcut).  TunnelSystem owns the geometry,         *
 *	the portal cuts and the interior bounds; this file owns only the decision.                           *
 *                                                                                             *
 *	Every number that sizes a bore comes in through TunnelPlannerParams, filled by the caller from     *
 *	WorldMetricsClass and the world profile.  A class the caller could not size (no metrics            *
 *	resolved) has an invalid clearance, and its candidates are refused with that reason rather than      *
 *	built to a guessed size.                                                                               *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef TUNNELPLANNER_H
#define TUNNELPLANNER_H

#ifndef ALWAYS_H
#include "always.h"
#endif

#include "vector.h"
#include "vector3.h"
#include "wwstring.h"
#include "tunnelsystem.h"


//	Section 34 Phase 10 step 1's five candidate sources, named as that step lists them.
enum TunnelCandidateSourceType
{
	TUNNEL_SOURCE_CLIFF_BYPASS = 0,			// a TUNNEL_CANDIDATE a road passes through
	TUNNEL_SOURCE_UNRESOLVED_ROAD_EDGE,		// a crossing BridgePlanner left unresolved
	TUNNEL_SOURCE_INFANTRY_FLANK,				// a TUNNEL_CANDIDATE no road passes through
	TUNNEL_SOURCE_SETTLEMENT_UNDERPASS,
	TUNNEL_SOURCE_STRATEGIC_ALTERNATE,
	TUNNEL_SOURCE_COUNT
};

const char *	Tunnel_Source_Name(TunnelCandidateSourceType source);


enum TunnelFailureReasonType
{
	TUNNEL_FAILURE_NONE = 0,
	TUNNEL_FAILURE_NO_CLEARANCE_DATA,	// the class's clearance could not be sized from real metrics
	TUNNEL_FAILURE_NO_OPEN_GROUND,		// no portal ground found on one side within the bore length
	TUNNEL_FAILURE_TOO_LONG,
	TUNNEL_FAILURE_GRADE_TOO_HIGH,
	TUNNEL_FAILURE_NO_OVERBURDEN,			// nothing above the bore to tunnel through
	TUNNEL_FAILURE_NOT_CONTINUOUS,		// ground drops away under the bore between the portals
	TUNNEL_FAILURE_BOUNDARY,				// step 2 / Section 12.4
	TUNNEL_FAILURE_BASE_ZONE,				// step 2 -- a protected base zone
	TUNNEL_FAILURE_MAJOR_WATER,			// step 2 -- water at a portal or an approach
	TUNNEL_FAILURE_COLLISION,				// another tunnel's bore
	TUNNEL_FAILURE_TYPE_COUNT
};

const char *	Tunnel_Failure_Reason_Name(TunnelFailureReasonType reason);


class TunnelCandidate
{
public:
	TunnelCandidate(void);

	bool	operator == (const TunnelCandidate & that) const;
	bool	operator != (const TunnelCandidate & that) const	{ return !(*this == that); }

	StringClass						Id;				// the strategic node / crossing id it came from
	TunnelCandidateSourceType	Source;
	TunnelClassType				Class;
	Vector3							Position;		// on the barrier
	Vector3							Axis;				// plan direction through the barrier, unit
	int								RoadId;			// -1 for a non-road candidate
	StringClass						RoadEdgeId;		// empty for a non-road candidate
	float								RoadWidth;		// 0 for a non-road candidate
};


class ResolvedTunnel
{
public:
	ResolvedTunnel(void);

	bool	operator == (const ResolvedTunnel & that) const;
	bool	operator != (const ResolvedTunnel & that) const	{ return !(*this == that); }

	StringClass						Id;				// stable id, Section 3B.5
	int								TunnelId;		// the TunnelSystem id this resolution owns
	StringClass						CandidateId;
	StringClass						RoadEdgeId;		// non-empty when step 14 resolved a road edge
	TunnelClassType				Class;
	TunnelCandidateSourceType	Source;
	bool								RoadSplit;		// the carried road was cut at both portals
	Vector3							PortalA;
	Vector3							PortalB;

	//	Section 12.5 -- what navigation represents explicitly.  Filled by NavMeshGenerator:
	//	the navigable chunk slot outside each mouth, and the interior's own registered
	//	PathfindClass sector (-1 without a live pathfinder).  NavLinked means both mouths
	//	reached walkable ground and the class is eligible for the sector graph as sized.
	bool								NavLinked;
	int								EntranceChunk;
	int								InteriorSector;
	int								ExitChunk;
};


class TunnelResolutionRecord
{
public:
	TunnelResolutionRecord(void);

	bool	operator == (const TunnelResolutionRecord & that) const;
	bool	operator != (const TunnelResolutionRecord & that) const	{ return !(*this == that); }

	StringClass						CandidateId;
	TunnelCandidateSourceType	Source;
	bool								Resolved;
	int								AttemptsMade;
	float								Displacement;
	TunnelFailureReasonType		FinalFailureReason;
};


struct TunnelPlannerParams
{
	TunnelPlannerParams(void);

	unsigned int	Seed;

	//	Section 12A.1, one per class, sized by the caller from WorldMetricsClass and the profile.
	//	UNDERPASS inherits its carried road's cross-section at resolve time.
	TunnelClearanceClass	ClassClearance[TUNNEL_CLASS_COUNT];

	float	MaxBoreLength;						// longest bore; same order as the longest bridge span
	float	MinCover;							// ground this far above the roof counts as cover
	float	WallThickness;
	float	BaseProtectedRadius;				// the base-selection footprint radius, same formula
	int	MaxRepositionAttempts;			// bounded, deterministic sideways retries per candidate
	float	RepositionStep;					// world units per retry
};


class TunnelPlanner
{
public:

	static void			Init(void);
	static void			Shutdown(void);

	/*
	**	Gathers candidates (step 1), resolves each (steps 2-6, 13-14, 16) and realizes the
	**	accepted ones through TunnelSystem (layout, portal cuts, geometry).  Requires the chunk
	**	registry; returns true with zero tunnels when nothing asks for one.
	*/
	static bool			Generate(const TunnelPlannerParams & params,StringClass * error_out);
	static void			Clear(void);

	//	Step 1, exposed so it is separately checkable.
	static int			Gather_Candidates(const TunnelPlannerParams & params,DynamicVectorClass<TunnelCandidate> & out);

	/*
	**	Steps 2-6 and 13 for one candidate, without realizing anything: fills `tunnel_out` with a
	**	legal bore or returns false with the reason.  Pure over terrain, bounds, water and the
	**	tunnels already in TunnelSystem.
	*/
	static bool			Plan_Bore(const TunnelCandidate & candidate,const TunnelPlannerParams & params,
										 TunnelClass * tunnel_out,TunnelFailureReasonType * reason_out);

	/*
	**	Plan_Bore with bounded sideways repositioning, then realize: TunnelSystem::Add_Tunnel and,
	**	for a road candidate, the road split (step 14).  Records a diagnostic either way.
	*/
	static bool			Resolve_Candidate(const TunnelCandidate & candidate,const TunnelPlannerParams & params);

	static bool			Has_Plan(void)				{ return Built; }
	static int			Get_Tunnel_Count(void)	{ return Tunnels.Count(); }
	static bool			Get_Tunnel(int index,ResolvedTunnel * out);
	static bool			Set_Nav_Links(int index,int entrance,int interior,int exit);
	static int			Get_Diagnostic_Count(void)	{ return Diagnostics.Count(); }
	static bool			Get_Diagnostic(int index,TunnelResolutionRecord * out);
	static bool			Is_Candidate_Resolved(const StringClass & candidate_id);
	static unsigned int	Get_Checksum(void)		{ return Checksum; }

	//	Step 16 and the gate: every accepted bore inside the world, every road tunnel's carried
	//	road split and bound at both portals.
	static int			Check_No_Boundary_Breach(StringClass * report);
	static int			Check_Road_Tunnels_Bound(StringClass * report);

private:

	static bool			Built;
	static unsigned int	Checksum;
	static DynamicVectorClass<ResolvedTunnel>				Tunnels;
	static DynamicVectorClass<TunnelResolutionRecord>	Diagnostics;
};

#endif	// TUNNELPLANNER_H
