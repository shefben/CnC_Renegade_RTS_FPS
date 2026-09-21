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
 *                                                                                             *
 *                 Project Name : OpenW3D                                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwphys/hydrologygenerator.h                  $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	V2 roadmap Section 34 Phase 6.  Rivers, streams, lakes and ponds carved from the terrain's    *
 *	own drainage rather than drawn as blue splines over hills.  Section 9.                        *
 *                                                                                             *
 *	One water renderer, used a second way.  `WaterSystem`/`WaterAreaClass` (Phase 17-A, complete   *
 *	before this roadmap started) already carve, mask and draw a body of water from a placed        *
 *	shape -- what it never had was anything computing that shape procedurally ("nothing plants      *
 *	anything," its own doc says). This generator is exactly that: it computes drainage over the      *
 *	chunk grid, turns high-accumulation runs into `WaterAreaClass` centre lines and closed basins     *
 *	into rings, and hands both to `WaterSystem::Add_Area`/`Shape_Terrain`/`Stamp_Mask`/`Build_          *
 *	Geometry` -- the same entry points the console command and self check already use. There is no       *
 *	second carving primitive and no second collision answer; `WorldTerrainSystem::Apply_River_Cut`         *
 *	(used internally by `WaterSystem::Shape_Terrain`) is untouched by this file.                            *
 *                                                                                             *
 *	Drainage at chunk resolution, not vertex resolution.  A full per-vertex D8 flow-accumulation      *
 *	pass is a real hydrology simulation this file does not attempt. Phase 4/5 already established      *
 *	the chunk grid as the unit generators reason about (one chunk = one heightfield patch), and         *
 *	this generator reuses it: one representative height per chunk, steepest-descent flow direction       *
 *	among four neighbours, and accumulation summed in one height-descending pass -- the standard          *
 *	sorted-list D8 algorithm, at chunk rather than vertex granularity. Coarser than a real hydrology        *
 *	tool, and honest about it.                                                                                *
 *                                                                                             *
 *	Section 5.9, kept structurally.  A channel is never allowed to reach a `CliffBand` chunk: the        *
 *	walk stops one chunk short and terminates into an enclosed terminal pond there (or, with ponds          *
 *	and lakes both disabled, simply stops carving) rather than ever handing a boundary-band point to        *
 *	`WaterSystem`.  "Water modifies boundary cliff band" and "rivers terminate as unexplained                *
 *	traversable holes" are both prevented the same way Phase 4/5 keep their own boundary conditions:          *
 *	by construction, not by a check a future edit could defeat.                                                *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef HYDROLOGYGENERATOR_H
#define HYDROLOGYGENERATOR_H

#ifndef ALWAYS_H
#include "always.h"
#endif

#ifndef VECTOR_H
#include "vector.h"
#endif

#ifndef VECTOR3_H
#include "vector3.h"
#endif

#ifndef WWSTRING_H
#include "wwstring.h"
#endif


//	Section 9.2/9.3's own categories, restricted to the four this generator ever produces.
//	Ocean/coast (Section 9.4) is not attempted this phase -- see this file's own header and
//	docs/procedural_world/Hydrology.md.
enum HydrologyFeatureType
{
	HYDRO_FEATURE_RIVER = 0,
	HYDRO_FEATURE_STREAM,
	HYDRO_FEATURE_LAKE,
	HYDRO_FEATURE_POND,
	HYDRO_FEATURE_TYPE_COUNT
};

//	Section 9.3/9.5's five shore kinds, one per feature (coarse -- the worst/least-crossable
//	classification found while sampling it), not per segment.  See Hydrology.md for why.
enum HydrologyShoreType
{
	HYDRO_SHORE_WALKABLE_BANK = 0,
	HYDRO_SHORE_STEEP_BANK,
	HYDRO_SHORE_CLIFF_SHORE,
	HYDRO_SHORE_VEHICLE_CROSSABLE,
	HYDRO_SHORE_NON_CROSSABLE,
	HYDRO_SHORE_TYPE_COUNT
};

//	Section 9.5's crossing kinds.  This generator only ever proposes FORD or BRIDGE for a linear
//	channel and ROUTE_AROUND for an enclosed body -- TUNNEL is Phase 10's own judgement call, not
//	something a hydrology pass can decide from width/depth alone.
enum HydrologyCrossingType
{
	HYDRO_CROSSING_FORD = 0,
	HYDRO_CROSSING_BRIDGE,
	HYDRO_CROSSING_TUNNEL,
	HYDRO_CROSSING_ROUTE_AROUND,
	HYDRO_CROSSING_TYPE_COUNT
};


class HydrologyCrossingCandidate
{
public:
	HydrologyCrossingCandidate(void);

	//	DynamicVectorClass<T> (wwlib/vector.h) requires T to support == and !=.
	bool	operator == (const HydrologyCrossingCandidate & that) const
	{
		return (Position == that.Position) && (Kind == that.Kind);
	}
	bool	operator != (const HydrologyCrossingCandidate & that) const	{ return !(*this == that); }

	Vector3						Position;
	HydrologyCrossingType	Kind;
};


/*
**	One placed body of water, after it has been built.  Read-only outside the generator.
*/
class HydrologyFeature
{
public:
	HydrologyFeature(void);

	bool	operator == (const HydrologyFeature & that) const;
	bool	operator != (const HydrologyFeature & that) const	{ return !(*this == that); }

	int							WaterAreaId;		// the WaterSystem::Add_Area id this feature owns
	StringClass					Name;					// the unique name it was placed under -- WaterSystem::Find_Area(Name)
																	//	is how a checker/consumer looks its WaterAreaClass back up, since
																	//	Add_Area's own id can drift from the area's array index over time
	HydrologyFeatureType		Type;
	HydrologyShoreType		Shore;
	bool							Enclosed;			// true for a lake/pond, including a river's own terminal pond
	float							Length;				// world units, 0 for an enclosed body

	DynamicVectorClass<HydrologyCrossingCandidate>	Crossings;
};


struct HydrologyParams
{
	HydrologyParams(void);

	unsigned int	Seed;

	bool	GenerateRivers;	// also gates streams/tributaries -- Section 9.2 treats them as one network
	bool	GenerateLakes;
	bool	GeneratePonds;

	float	RiverWidth;						// world units, > 0 -- width at full accumulation
	float	StreamWidth;					// world units, > 0, <= RiverWidth -- width at a channel's own source
	float	RiverAccumulationChunks;	// > 0 -- chunks of drainage before a channel counts as a river
	int	MaxRiverCount;					// > 0 -- bounded count of distinct channel systems
};


class HydrologyGenerator
{
public:

	static void			Init(void);
	static void			Shutdown(void);

	/*
	**	Requires WorldTerrainSystem::Has_Chunk_Registry() -- drainage is computed over the chunk
	**	registry's own height data, so there is nothing to compute without it.  Returns true with
	**	zero features for a world too flat or too small to drain anywhere, which is a correct,
	**	unremarkable outcome, not an error.
	*/
	static bool			Generate(const HydrologyParams & params,StringClass * error_out);

	//	Drop only this generator's own water areas, mirroring BoundaryCliffGenerator::Clear_Ring
	//	and InteriorCliffGenerator::Clear -- the same "read the collection back minus what is
	//	owned, then re-add" idiom, since WaterSystem::Clear_Areas has no per-owner selectivity of
	//	its own.
	static void			Clear(void);

	static bool			Has_Hydrology(void)			{ return Built; }
	static int			Get_Feature_Count(void)		{ return Features.Count(); }
	static bool			Get_Feature(int index,HydrologyFeature * out);

	//	A CRC over every feature's own type, shore classification, enclosed flag, length and
	//	crossing candidates, quantized (Section 3B.4).  Zero with no hydrology built.
	static unsigned int	Get_Checksum(void)			{ return Checksum; }

	static int			Check_No_Cliff_Band_Overlap(StringClass * report);
	static int			Check_River_Never_Climbs(StringClass * report);
	static int			Check_Lake_Within_Basin(StringClass * report);
	static int			Check_Crossing_Candidates_Present(StringClass * report);

private:

	//	One basin, built as a closed water area exactly matching the sink chunk's own footprint.
	//	Shared by the standalone basin pass and a river's own Section-5.9 terminal pond -- both
	//	go through one place that decides lake-vs-pond and one place that calls WaterSystem.
	static bool			Build_Basin_Feature(int px,int py,float accumulation,const HydrologyParams & params);

	static bool			Built;
	static unsigned int	Checksum;

	static DynamicVectorClass<int>					OwnedWaterAreaIds;
	static DynamicVectorClass<HydrologyFeature>	Features;
};

#endif	// HYDROLOGYGENERATOR_H
