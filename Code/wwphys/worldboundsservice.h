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
 *                 Project Name : WWPhys                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwphys/worldboundsservice.h                  $*
 *                                                                                             *
 *	V2 roadmap Section 34 Phase 2 -- the one authoritative finite world boundary.  Design:       *
 *	Sections 1.3-1.6 (one finite map, one absolute boundary; the skybox is not the boundary)     *
 *	and Sections 5.1/5.2/5.6-5.10 (boundary topology, protected zones, physics enforcement,      *
 *	air boundary, Commander-camera boundary, feature interaction, debugging).                    *
 *                                                                                             *
 *	WHY THIS LIVES IN WWPHYS.  Every v1 world-layer owner this project already has --            *
 *	`WorldTerrainSystem`, `RoadSystem`, `BridgeSystem`, `WaterSystem`, `FoliageSystem` -- lives   *
 *	in wwphys, and Phase 2's step 14 ("integrate actor boundary checks at the canonical           *
 *	movement/authority layer") lands in `MoveablePhysClass::Post_Timestep_Process`                *
 *	(`Code/wwphys/movephys.cpp`), which is below Combat and cannot include it.  Putting the        *
 *	boundary anywhere else would have forced either a layering inversion or a second boundary      *
 *	representation on the physics side -- and "different subsystems maintain different boundary    *
 *	polygons" is the first thing Phase 2's own "do not proceed if" list forbids.                   *
 *                                                                                             *
 *	WHAT THIS DOES NOT OWN.  Generation.  This service receives a finished polygon; building one   *
 *	from a `ProceduralWorldProfile` and its `BoundarySeed` is Combat's job                          *
 *	(`Code/Combat/proceduralboundsbuilder.h`), because the profile/seed services from Phase 1 live  *
 *	there.  One owner for the boundary *data and queries* (here), one for the *decision* of what     *
 *	that boundary is (there), which is the same split `CommanderModeController::Set_World_Bounds`    *
 *	already used before v2 existed.                                                                 *
 *                                                                                             *
 *	LEGACY MAPS.  Directive 1.10 is not optional: a hand-authored Renegade map must keep working    *
 *	untouched.  So this service is INACTIVE until something calls `Configure`, and every query       *
 *	answers "unbounded/legal" while inactive -- `Is_Configured()` false means infantry, vehicles,    *
 *	aircraft, the Commander camera and building placement all behave exactly as they did before      *
 *	v2, because every one of their new checks short-circuits.  Nothing in the stock game calls        *
 *	`Configure`.                                                                                     *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef WORLDBOUNDSSERVICE_H
#define WORLDBOUNDSSERVICE_H

#include "always.h"
#include "vector.h"
#include "vector2.h"
#include "vector3.h"
#include "wwstring.h"


/*
**	Section 5.2's protected zones, ordered from the inside out.  The numeric order is
**	deliberate and relied on: a "deeper than X" test is a `>=` comparison, so adding a zone
**	later means inserting it in spatial order, not appending it.
*/
enum WorldBoundsZoneType
{
	WORLD_ZONE_PLAYABLE_INTERIOR = 0,	// ordinary terrain/features
	WORLD_ZONE_BOUNDARY_APPROACH,			// terrain naturally rises/changes toward the perimeter
	WORLD_ZONE_CLIFF_BAND,					// tall generated perimeter cliffs; no roads/buildings/tunnels/resources
	WORLD_ZONE_HARD_BOUNDARY,				// authoritative non-crossable barrier behind/within the cliff geometry
	WORLD_ZONE_OUTSIDE_WORLD,				// never valid gameplay space

	WORLD_ZONE_COUNT
};


class WorldBoundsService
{
	public:

		/*
		**	How finely `Build_Ring_Polygon`-style callers are expected to tessellate the ring for
		**	*query* purposes.  Phase 3's cliff meshes will tessellate their own geometry far
		**	finer; what matters is that both follow the same underlying ring, which is why the
		**	polygon is handed in rather than re-derived per consumer.  Not a hard limit --
		**	`Configure` accepts any vertex count that passes validation.
		*/
		enum { SUGGESTED_RING_SEGMENTS = 64 };

		//	Section 5.1: a boundary must not self-intersect, and a degenerate ring is not a
		//	world.  Both are enforced by Configure; these are the thresholds it uses.
		static const float	MINIMUM_VERTEX_SPACING;
		static const float	MINIMUM_INTERIOR_AREA;

		static void		Init(void);
		static void		Shutdown(void);

		/*
		**	Installs the authoritative boundary.  `vertices` is a closed ring given once (the
		**	closing edge from last back to first is implied, never duplicated); winding may be
		**	either way on input and is normalized to clockwise internally, so a caller cannot
		**	silently install a backwards world.
		**
		**	`cliff_band_width` and `hard_barrier_offset` come straight from the world profile's
		**	own boundary block.  The BOUNDARY_APPROACH band has no profile field of its own --
		**	nothing in Section 5 asks for one -- so it is derived as one more cliff-band width
		**	inward, documented in docs/procedural_world/WorldBoundsAndBoundaryCliffs.md and
		**	trivially replaced by a real field later if a profile ever wants to tune it
		**	separately.
		**
		**	Returns false and changes nothing when validation fails (Section 34 Phase 2 step 3:
		**	closed, non-self-intersecting, minimum vertex spacing, minimum usable interior
		**	area).  `error_out`, when given, receives an actionable reason.
		*/
		static bool		Configure(const DynamicVectorClass<Vector2> & vertices,
										float cliff_band_width,float hard_barrier_offset,
										float air_ceiling,StringClass * error_out);

		//	Drops the boundary entirely -- every query returns to its "unbounded" answer.  This
		//	is what a legacy/hand-authored map leaves the service in, and what unloading a
		//	procedural world must restore.
		static void		Clear(void);

		static bool		Is_Configured(void);

		/*
		**	Section 5.6's authoritative queries.  All three answer permissively when the service
		**	is not configured: contains-anything, classify-everything-as-interior, and
		**	closest-valid-point returns the point it was given.
		*/
		static bool						Contains_Horizontal_Point(const Vector2 & point);
		static bool						Contains_Gameplay_Point(const Vector3 & point);
		static WorldBoundsZoneType	Classify_Point(const Vector2 & point);

		/*
		**	The nearest point an actor is allowed to occupy.  For a point already legal this is
		**	the point itself.  For an escaped one it is pulled back inside past the hard
		**	boundary, so the returned point is legal by construction rather than merely closer.
		*/
		static Vector3	Closest_Valid_Point(const Vector3 & point);

		/*
		**	Section 5.9's feature rules.  `Segment_Crosses_Hard_Boundary` is what a road,
		**	highway, tunnel, bridge or river planner asks before committing a span -- "does this
		**	line leave the world" -- rather than each planner sampling points at its own
		**	interval and hoping.  Conservative: samples the segment densely enough that no
		**	crossing shorter than the hard band can slip between samples.
		*/
		static bool		Segment_Crosses_Hard_Boundary(const Vector2 & start,const Vector2 & end);

		/*
		**	Construction/placement legality.  Stricter than `Contains_Horizontal_Point`: the
		**	cliff band is physically inside the world but nothing may be BUILT there (Section
		**	5.9 forbids a Commander building, settlement, base or Tiberium field in the cliff
		**	band).  `radius` is the footprint's own extent, so a building whose centre is legal
		**	but whose corner reaches the cliff band is refused.
		*/
		static bool		Can_Place_In_Region(const Vector2 & centre,float radius);

		/*
		**	Section 5.7/5.8.  The air volume is the same horizontal polygon plus a ceiling --
		**	deliberately not an independent volume, since "aircraft uses unrelated bounds" is on
		**	Phase 2's own "do not proceed if" list.  The Commander camera clamp is the same
		**	horizontal test: the camera may look from anywhere an aircraft could legally be, and
		**	no further (Section 5.8: it must not fly outside the cliff ring and view the terrain
		**	from behind the world).
		*/
		static bool		Is_Inside_Air_Volume(const Vector3 & point);
		static float	Get_Air_Ceiling(void);
		static Vector3	Clamp_Commander_Camera(const Vector3 & focus);

		/*
		**	Server-side escape recovery (Section 5.6's fallback protection).  Enforcement is
		**	server-authoritative -- a client never corrects its own position, it receives the
		**	corrected one -- so the movement hook only recovers when this is set.  wwphys cannot
		**	ask `CombatManager::I_Am_Server` (that is Combat, a layer above), so Combat sets this
		**	once at startup; the default of false means an unconfigured or client-side build
		**	enforces nothing.
		*/
		static void		Set_Server_Authoritative(bool is_authoritative);
		static bool		Is_Server_Authoritative(void);

		/*
		**	Called by `MoveablePhysClass::Post_Timestep_Process` for every moving actor once its
		**	timestep has been applied.  Returns true when it moved the actor back inside, having
		**	logged the violation with the actor's identity, its prior transform and the attempted
		**	one (Phase 2 step 17).  `last_safe` is the caller's own remembered good position --
		**	kept on the actor rather than in a map here so the common case (still legal, just
		**	update it) costs no lookup.
		*/
		static bool		Recover_Out_Of_Bounds_Actor(const Vector3 & attempted,const Vector3 & last_safe,
																	bool has_last_safe,const char * actor_description,
																	Vector3 * out_recovered);

		//	Violation counter, for the self check and for `world_bounds` diagnostics.  Never
		//	resets on its own.
		static int		Get_Violation_Count(void);

		/*
		**	Section 5.10's debug drawing.  Returns the boundary ring for a given zone as a
		**	closed polyline so a debug layer can draw every band without this service depending
		**	on any renderer.  `zone` selects which band's inner edge is wanted; the outermost
		**	(OUTSIDE_WORLD) edge is the configured polygon itself.  Returns false when not
		**	configured or when the requested band collapsed to nothing.
		*/
		static bool		Get_Zone_Ring(WorldBoundsZoneType zone,DynamicVectorClass<Vector2> * out_vertices);

		//	Distance from `point` inward to the nearest boundary edge; negative outside the
		//	world.  The one primitive every classification above is derived from, exposed
		//	because Phase 3's cliff generator needs exactly this to place its rim and toe.
		static float	Get_Inward_Distance(const Vector2 & point);
};

#endif // WORLDBOUNDSSERVICE_H
