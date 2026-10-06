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
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/hybridroadrealizer.h                  $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Hybrid world generator rewrite, item I13 -- runtime Phase 12 on the canonical owners.          *
 *                                                                                               *
 * On the native hybrid path this is the one runtime realization owner of roads, bridge and       *
 * overpass decks (each built from its BridgeAssembly), junction surfaces and accepted road         *
 * cut/fill.  It drives WorldTerrainSystem, RoadSystem and BridgeSystem from the frozen Phase-12     *
 * plan (Hybrid_Plan_Realization) exactly once per attempt, keeps the reverse-order cleanup list,    *
 * verifies the runtime objects against the plan, and publishes read-only migration adapters into   *
 * the legacy planners the downstream systems still read.  The native hybrid generator builds no     *
 * tunnels (D-032): none is planned, compiled or realized.                                          *
 *                                                                                               *
 *---------------------------------------------------------------------------------------------*/

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef HYBRIDROADREALIZER_H
#define HYBRIDROADREALIZER_H

#include "always.h"
#include "vector.h"
#include "wwstring.h"
#include "hybridrealization.h"
#include "hybridanchors.h"


//	One reference vehicle's measured collision envelope (preflight's WORLDBOX), full extents in world units.
struct HybridSweptEnvelope
{
	HybridSweptEnvelope(void) : Length(0.0f),Width(0.0f),Height(0.0f) { }

	float	Length;
	float	Width;
	float	Height;
};


//	Everything Phase 12's runtime half reads: the frozen records and the plan built from them.
struct HybridRoadRealizerInputs
{
	HybridRoadRealizerInputs(void) :
		Plan(nullptr),Topology(nullptr),Routes(nullptr),Interactions(nullptr),Anchors(nullptr),Hydrology(nullptr),
		MinVehicleLaneClearance(0.0f)
	{
		memset(&Constants,0,sizeof(Constants));
	}

	const HybridRealizationPlan *		Plan;
	const HybridTopology *				Topology;
	const HybridRouteSet *				Routes;
	const HybridInteractionSet *		Interactions;
	const HybridAnchorSet *				Anchors;
	const HybridHydrologySet *			Hydrology;
	HybridConstants						Constants;
	DynamicVectorClass<StringClass>	BridgeDefinitions;	// the BridgeSystem definition of each Phase-11 bridge kind

	//	Section 12.3's swept envelopes: light on every road, heavy on heavy edges, Harvester on economy edges.
	HybridSweptEnvelope					Light;
	HybridSweptEnvelope					Heavy;
	HybridSweptEnvelope					Harvester;
	float										MinVehicleLaneClearance;	// Section 12.4's clear-width margin
};


//	One RoadSystem road Phase 12 created: a stretch of one planned road between its decks.
struct HybridRealizedPiece
{
	HybridRealizedPiece(void) : RoadIndex(-1),FirstSample(0),LastSample(0),FromStation(0.0f),ToStation(0.0f),RoadId(-1) { }

	bool	operator == (const HybridRealizedPiece & that) const	{ return RoadId == that.RoadId; }
	bool	operator != (const HybridRealizedPiece & that) const	{ return !(*this == that); }

	int		RoadIndex;			// into the plan's Roads
	int		FirstSample;		// the plan samples it was built through, inclusive
	int		LastSample;
	float		FromStation;
	float		ToStation;
	int		RoadId;				// RoadSystem
};


class HybridRoadRealizer
{
public:

	/*
	**	Section 12.2 steps 1-9 on the runtime owners, once: the terrain deformation committed through
	**	WorldTerrainSystem, one RoadSystem road per planned road stretch between decks (its edge profile carrying the
	**	asymmetric/effective edges), one BridgeSystem bridge per BridgeAssembly (its single WHOLE piece), the ends
	**	bound to each other and a ramp's gore end attached to its highway, then the network and the bridge layout
	**	built.  Every created id goes on the cleanup list.  False -- with nothing left behind -- on any failure.
	*/
	static bool			Realize(const HybridRoadRealizerInputs & in,StringClass * error_out);

	/*
	**	Section 12.3's record half: every planned stretch and assembly realized exactly once and nothing else added;
	**	every realized road point on its planned centerline at its solved height; every bridge at its assembly's
	**	ends, width and kind, bound at both decked ends; and no RoadSystem-discovered junction inside an active
	**	interchange's parallel interval or gore reserve.
	*/
	static bool			Verify(const HybridRoadRealizerInputs & in,StringClass * error_out);

	/*
	**	Section 12.3's physical half, against the runtime objects only:
	**	- the canonical swept/collision query at `min(W/2, A/3)` along every road (light), every heavy-capable road
	**	  (heavy) and every economy road (Harvester), ramps, parallel intervals, local junctions, decks and overpasses
	**	  included: the envelope's footprint on paved runtime surface, standing on the solved surface, under no deck
	**	  closer than its height plus Section 12.3's safety, and clear of every pier;
	**	- every frozen structural pin (plan.Pins) on its authoritative drivable surface -- the terrain off a deck, the deck
	**	  on one, and the road attached there -- within 2Q, never relaxed by the terrain cell (D-026);
	**	- every BRIDGE_END_SUPPORT claim inside its assembly's end support footprint;
	**	- every realized road sample against its solved grade and cross slope;
	**	- every BridgeAssembly against its compiled span, clear width, deck elevation, attachment at both ends and
	**	  vertical clearance;
	**	- every active interchange by the Section-12.6.13 ramp list.
	**	A discrepancy fails the attempt; nothing is repaired after realization.
	*/
	static bool			Verify_Physical(const HybridRoadRealizerInputs & in,StringClass * error_out);
	static int			Get_Swept_Samples(void)							{ return SweptSamples; }

	//	The terrain Phase 12 graded as structural support under bridge ends (D-026 BRIDGE_END_SUPPORT), by assembly.
	static int			Get_Support_Claim_Count(void)					{ return Claims.Count(); }
	static const HybridTerrainClaim &	Get_Support_Claim(int index)	{ return Claims[index]; }

	//	The migration adapters: nodes, road graph edges, river/canyon bridges and the frozen base pair.
	static bool			Publish_Adapters(const HybridRoadRealizerInputs & in,StringClass * error_out);

	//	The cleanup list in reverse order: bridges, then roads.  Nobody else's roads or bridges are touched.
	static void			Clear(void);

	static int			Get_Realize_Count(void)							{ return RealizeCount; }
	static int			Get_Piece_Count(void)							{ return Pieces.Count(); }
	static const HybridRealizedPiece &	Get_Piece(int index)		{ return Pieces[index]; }
	static int			Get_Bridge_Count(void)							{ return BridgeIds.Count(); }
	static int			Get_Bridge_Id(int index)						{ return BridgeIds[index]; }
	static float		Get_Terrain_Changed(void)						{ return (float)TerrainChanged; }

private:

	static int										RealizeCount;
	static int										TerrainChanged;
	static int										BaselineRoads;
	static int										BaselineBridges;
	static int										SweptSamples;
	static DynamicVectorClass<HybridRealizedPiece>	Pieces;
	static DynamicVectorClass<int>			BridgeIds;			// one per plan assembly, in order
	static DynamicVectorClass<HybridTerrainClaim>	Claims;
};


#endif	// HYBRIDROADREALIZER_H
