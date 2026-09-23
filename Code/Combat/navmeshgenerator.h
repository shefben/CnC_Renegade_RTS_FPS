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
 *                     $Archive:: /Commando/Code/Combat/navmeshgenerator.h                    $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	V2 roadmap Section 34 Phase 16 steps 3-8 -- infantry/ground-vehicle/air navigation.           *
 *                                                                                             *
 *	The one real consultable AI movement system.  `PathfindClass` (wwphys/Pathfind.h) is not a     *
 *	stub: `Add_Sector`/`Add_Portal` are real public database-construction methods, its singleton     *
 *	is what `action.cpp` (Combat's own AI movement code) actually queries -- Does_Pathfind_Data_       *
 *	Exist, Find_Waypath -- to decide how a unit moves. What does not exist anywhere in this tree         *
 *	is an automatic sector-from-geometry BUILDER: `PathfindSectorBuilderClass`                             *
 *	(wwphys/PathfindSectorBuilder.cpp) looks like one, but it is not in wwphys/CMakeLists.txt (so           *
 *	not part of any built target), calls PathfindClass methods (Add_Body_Box, Get_Raw_Data,                    *
 *	Reset_Generated_Data) that do not exist on the PathfindClass actually checked in, and is                    *
 *	wired to LevelEdit's own interactive asset manager/message-box UI -- confirmed dead, unbuilt                 *
 *	Westwood source, not a reusable reference implementation.  So this file builds real sectors                   *
 *	and portals itself and registers them through the one real, live API.                                          *
 *                                                                                             *
 *	One sector graph, not three.  Steps 3/4 ask for separate infantry/ground-vehicle navigation,      *
 *	but `PathfindSectorClass` carries no locomotion-class tag and `PathfindClass` is one global       *
 *	singleton -- there is no way to register "walkable for infantry, not for a tank" as two              *
 *	different answers about the same point without inventing a parallel sector format, which           *
 *	Section 28A's own reuse discipline already forbids for asset descriptors and this file holds          *
 *	to for the same reason.  `WorldMetricsClass::Get_Class_Metrics`'s own documented answer to             *
 *	exactly this shape of question is the most restrictive member across every locomotion class            *
 *	in play -- "a corridor has to admit the worst case, not the average one" -- so that is what             *
 *	this generator builds: one real sector graph, sized by the tightest MaxClimbableSlope across             *
 *	infantry, wheeled and tracked (whichever classes have a loaded definition), which is walkable            *
 *	by all of them simultaneously.  Ground-vehicle navigation is a real requirement on this one                *
 *	graph, not a second graph.  Naval is skipped outright -- no canonical naval locomotor exists                 *
 *	anywhere in this tree (grepped), and the roadmap's own step 6 wording ("only where canonical                  *
 *	support exists") allows exactly that.                                                                          *
 *                                                                                             *
 *	Flight is not a sector graph.  Step 5 names it "air navigation/flight volume", and this            *
 *	tree's own AI has no flight-pathing consultation of PathfindClass -- flight is real player           *
 *	control, not AI sector traversal.  So this file's own air output is exactly what the roadmap          *
 *	names: a real bounded volume (WorldBoundsService::Get_Air_Ceiling as the real ceiling already          *
 *	established by Phase 2, the tallest real chunk height plus a clearance margin as the floor),            *
 *	not a fabricated air sector graph nothing would ever query.                                              *
 *                                                                                             *
 *	Grid, not a physics floodfill.  The real historical algorithm (the dead builder above)                    *
 *	tumbles a physics body through the world one simulated step at a time.  This project's own                  *
 *	terrain generators already resolved the equivalent question once, deterministically, into the               *
 *	chunk registry `WorldTerrainSystem::Get_Chunk_Info` exposes (CliffBand/InteriorCliff/                        *
 *	MaxSlopeRadians) -- re-deriving it through a physics simulation would be the exact kind of                    *
 *	second, drifting copy this project's own reuse discipline forbids.  One sector per walkable                   *
 *	chunk, at the chunk registry's own resolution, is coarser than a per-cell physics floodfill                    *
 *	would be; that is a disclosed granularity choice, not a hidden one -- see                                      *
 *	docs/procedural_world/FinalWorldIntegration.md.                                                                *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef NAVMESHGENERATOR_H
#define NAVMESHGENERATOR_H

#ifndef ALWAYS_H
#include "always.h"
#endif

#include "vector.h"
#include "vector3.h"
#include "wwstring.h"


struct NavMeshParams
{
	NavMeshParams(void);

	//	>= 0.  A structure/bridge-abutment clearance margin added to every real footprint radius
	//	this generator excludes a sector for -- the same "reserved region" idea Phase 11/12 already
	//	apply to their own candidates, sized small since the chunk grid itself is already coarse.
	float	StructureClearanceMargin;

	//	>= 0.  How far (world units) a bridge's own road-edge endpoint may search for the nearest
	//	walkable sector to link an off-mesh portal to.  Beyond this, that bridge's own portal is
	//	honestly not built rather than linked to an unrelated, far-away sector.
	float	BridgeLinkSearchRadius;

	//	>= 0.  Added above the tallest real chunk height to make the flight volume's own floor
	//	(step 5) -- real clearance, not zero, the same idea Get_Overhead_Clearance already applies
	//	to a single unit.
	float	FlightFloorClearanceMargin;
};


class NavMeshGenerator
{
public:

	static void		Init(void);
	static void		Shutdown(void);

	/*
	**	Requires WorldTerrainSystem::Has_Chunk_Registry() and a loaded infantry definition (so
	**	WorldMetricsClass::Get_Class_Metrics(LOCOMOTION_INFANTRY,...) succeeds -- with none loaded
	**	there is no real envelope to size a sector by, and this refuses rather than guess one).
	**	Resets and rebuilds PathfindClass::Get_Instance()'s own sector/portal database -- safe to
	**	call again across a bounded seed retry, the same re-buildable contract every generator in
	**	this chain already has.
	*/
	static bool		Build(const NavMeshParams & params,StringClass * error_out);

	static void		Clear(void);
	static bool		Has_Navigation(void)	{ return Built; }

	static int		Get_Sector_Count(void)			{ return SectorCount; }
	static int		Get_Portal_Count(void)			{ return PortalCount; }
	static int		Get_Bridge_Portal_Count(void)	{ return BridgePortalCount; }

	//	True when at least one of LOCOMOTION_WHEELED/LOCOMOTION_TRACKED had a loaded definition,
	//	so the one sector graph's own slope cap is genuinely a ground-vehicle answer and not
	//	silently infantry-only (see this file's own header comment on why there is one graph).
	static bool		Is_Ground_Vehicle_Navigable(void)	{ return GroundVehicleIncluded; }

	static float	Get_Max_Climbable_Slope_Radians(void)	{ return MaxClimbableSlopeRadians; }

	static bool		Has_Flight_Volume(void)	{ return HasFlightVolume; }
	static bool		Get_Flight_Volume(float * min_altitude_out,float * max_altitude_out);

	static unsigned int	Get_Checksum(void)	{ return Checksum; }

	//	Step 8.  0 until the first successful Build; 1 after it.  Real and meaningful today as
	//	"this world's nav was built once"; Phase 18's own monotonic world revision is the real
	//	generalisation of this idea once runtime mutation exists to bump it further.
	static unsigned int	Get_Revision(void)	{ return Revision; }

	/*
	**	Which terrain chunks this generator judged walkable, and how the grid is shaped.
	**
	**	Exposed because Phase 17's chokepoint analysis needs exactly the walkability judgement
	**	the sector graph was built from.  Re-deriving that predicate at the call site would be
	**	two answers to one question -- "can a unit stand here" -- that could drift apart the
	**	first time either side's slope, water or structure-clearance rule changed.
	**
	**	Distinct from Get_Sector_Count: a chunk is navigable whether or not a live PathfindClass
	**	existed to register a sector for it (see navmeshgenerator.cpp on `has_pathfinder`).  All
	**	three answer for the last successful Build; zero/false after Clear.
	*/
	static int		Get_Chunk_Count_X(void)	{ return ChunkCountX; }
	static int		Get_Chunk_Count_Y(void)	{ return ChunkCountY; }
	static bool		Is_Chunk_Navigable(int px,int py);

private:

	static bool		Built;
	static int			SectorCount;
	static int			PortalCount;
	static int			BridgePortalCount;
	static bool			GroundVehicleIncluded;
	static float		MaxClimbableSlopeRadians;
	static bool			HasFlightVolume;
	static float		FlightMinAltitude;
	static float		FlightMaxAltitude;
	static unsigned int	Checksum;
	static unsigned int	Revision;

	static int			ChunkCountX;
	static int			ChunkCountY;
	static DynamicVectorClass<int>	ChunkNavigable;		// px + py*ChunkCountX, 0 or 1
};

#endif	// NAVMESHGENERATOR_H
