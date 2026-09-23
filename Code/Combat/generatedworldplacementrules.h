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
 *                     $Archive:: /Commando/Code/Combat/generatedworldplacementrules.h        $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	V2 roadmap Section 34 Phase 17 steps 1-2 -- generated-world placement exclusions.            *
 *                                                                                             *
 *	Step 1 is "feed generated terrain/world bounds into the existing v1 building placement        *
 *	validator", and the honest answer to it is that most of it was already done: Phase 2 wired     *
 *	`WorldBoundsService::Can_Place_In_Region` and Phase 4 wired                                     *
 *	`WorldTerrainSystem::Query_Placement_Surface` into `CommanderPlacementValidator::Validate`        *
 *	when each of those owners first existed.  What did not exist then is everything Phases 5-16       *
 *	went on to generate -- water bodies, interior cliffs, bridges, a road graph and a sector          *
 *	graph -- and none of it is visible to a validator that only asks the ground how steep it is.        *
 *	This file is step 2: one question, asked of the real generated-world owners, answered with           *
 *	the reason rather than a bool.                                                                        *
 *                                                                                             *
 *	Why a separate file and not more code inside CommanderPlacementValidator.  That validator is        *
 *	Section 43's shared client/server decision and is deliberately v1-only: it runs unchanged on       *
 *	every hand-authored Renegade map, where none of the systems below is configured.  Keeping the     *
 *	generated-world half here means the v1 validator gains one call rather than nine includes, and   *
 *	means this file can say "no generated world -- nothing to exclude" in one place (directive      *
 *	1.10, legacy maps remain supported) instead of nine times.                                    *
 *                                                                                             *
 *	Nothing here invents a threshold.  Every distance is read from the real geometry that owns       *
 *	it -- a bridge's own width, a road spline's own width, the chunk registry's own cliff flags,       *
 *	the footprint radius the caller passed -- because a placement rule with a number in it that         *
 *	no generator agreed to is a rule that will disagree with the world the first time a profile           *
 *	changes.                                                                                                *
 *                                                                                             *
 *	The tunnel exclusion is TunnelSystem::Overlaps_Footprint (V2 Phase 10): the mouths, the opened      *
 *	approaches and the covered bore of every accepted tunnel, measured against the real geometry.       *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef GENERATEDWORLDPLACEMENTRULES_H
#define GENERATEDWORLDPLACEMENTRULES_H

#ifndef ALWAYS_H
#include "always.h"
#endif

#include "vector.h"
#include "vector3.h"
#include "wwstring.h"


/*
**	Phase 17 step 2's own exclusion list, one value each, in the order that step names them.
**	The order is also the evaluation order, which matters: a point inside the boundary cliff
**	band is reported as that rather than as whichever later rule would also have caught it, so
**	a rejection reason stays the most structural true thing about the position.
*/
enum GeneratedWorldExclusionType
{
	GENERATED_WORLD_EXCLUSION_NONE = 0,
	GENERATED_WORLD_EXCLUSION_BOUNDARY_CLIFF_BAND,
	GENERATED_WORLD_EXCLUSION_WATER,
	GENERATED_WORLD_EXCLUSION_IMMUTABLE_CLIFF,
	GENERATED_WORLD_EXCLUSION_TUNNEL,
	GENERATED_WORLD_EXCLUSION_BRIDGE,
	GENERATED_WORLD_EXCLUSION_RESOURCE_ROUTE,
	GENERATED_WORLD_EXCLUSION_NAV_CHOKEPOINT,

	GENERATED_WORLD_EXCLUSION_COUNT
};

const char *	Generated_World_Exclusion_Name(GeneratedWorldExclusionType exclusion);


class GeneratedWorldPlacementRules
{
public:

	static void		Init(void);
	static void		Shutdown(void);

	/*
	**	The one test.  Returns GENERATED_WORLD_EXCLUSION_NONE when nothing in the generated
	**	world forbids building a footprint of this radius here -- which is every position on
	**	every hand-authored map, where none of the owners consulted below reports any data.
	**
	**	Pure: reads the generated-world owners and writes nothing.  That is what lets the
	**	Commander's ghost call it on every cursor move and the server call it again on arrival
	**	and get the same answer, which is the whole reason Section 43's validator is shaped the
	**	way it is.
	**
	**	`radius` is the footprint's own sample radius -- the same number
	**	`CommanderPlacementValidator` already passes `WorldBoundsService` and
	**	`WorldTerrainSystem::Query_Placement_Surface` -- so a building whose centre clears a
	**	feature but whose corner does not is still refused.
	*/
	static GeneratedWorldExclusionType	Classify(const Vector3 & position,float radius);

	/*
	**	Every exclusion except the navigation one.
	**
	**	The seven others are all "a feature is here"; the chokepoint rule is the odd one out --
	**	it is about what the walkable world would look like afterwards, and it is the only one
	**	whose cost is a graph search rather than a handful of point queries.
	**
	**	Foundation grading is the caller that wants exactly this half.  Flattening ground does
	**	not remove it from the walkable world, so a grading apron cannot create a chokepoint; a
	**	river, a cliff face or a bridge abutment under that apron still forbids it.
	*/
	static GeneratedWorldExclusionType	Classify_Static_Features(const Vector3 & position,float radius);

	/*
	**	Every static exclusion that applies here at once, as `1 << GENERATED_WORLD_EXCLUSION_*`
	**	bits, rather than only the first one in priority order.
	**
	**	A placement wants the single most structural reason and stops at it.  Phase 18's runtime
	**	operations need the whole set, because which features forbid an operation depends on the
	**	operation: carving a channel is *supposed* to move water, and reclaiming shallow water is
	**	supposed to fill it, while neither may touch a bridge abutment.  Asking "is anything here"
	**	and then asking again without the part that was allowed is not a question this can answer;
	**	one mask is.
	**
	**	`Classify_Static_Features` is the lowest set bit of this, which is what keeps the two from
	**	ever disagreeing about what is present.
	*/
	static unsigned int	Get_Static_Feature_Mask(const Vector3 & position,float radius);

	//	True when any generated-world data exists to exclude anything at all.  False on a
	//	hand-authored level, where Classify is a constant GENERATED_WORLD_EXCLUSION_NONE.
	static bool		Has_Generated_World(void);

	/*
	**	Chokepoint metadata.  The articulation points of the navigable-chunk graph
	**	`NavMeshGenerator` built: the chunks whose loss on its own splits the walkable world in
	**	two.  Built once after navigation, cached, and exposed as real data because the
	**	Commander and the AI both want to know where a map's necks are, not only whether one
	**	particular placement sits on one.
	**
	**	Requires NavMeshGenerator::Has_Navigation().  Returns true with zero chokepoints for a
	**	world that genuinely has none, which is a correct and common outcome.
	*/
	static bool		Build_Chokepoints(StringClass * error_out);
	static void		Clear_Chokepoints(void);
	static bool		Has_Chokepoints(void)		{ return ChokepointsBuilt; }
	static int		Get_Chokepoint_Count(void)	{ return ChokepointCount; }
	static bool		Is_Chunk_Chokepoint(int px,int py);

	static unsigned int	Get_Chokepoint_Checksum(void)	{ return ChokepointChecksum; }

	/*
	**	The articulation-point analysis on its own, over a caller-supplied navigable grid.
	**
	**	Public because the graph theory and the world are separable, and proving the algorithm
	**	on a grid whose answer is known by construction -- two rooms joined by a one-chunk neck
	**	-- is a real test of the real code, where proving it on generated terrain only shows
	**	that some number came back.  `Build_Chokepoints` calls exactly this.
	**
	**	`navigable` is count_x*count_y, row-major, non-zero where a unit can stand.
	**	`out_articulation` is resized to match and set non-zero at each articulation point.
	*/
	static bool		Find_Articulation_Points(const DynamicVectorClass<int> & navigable,
														int count_x,int count_y,
														DynamicVectorClass<int> * out_articulation);

	/*
	**	Would removing every navigable chunk this footprint covers disconnect the walkable
	**	world?  The exact test, which the cached articulation set cannot replace: covering an
	**	articulation point *and* everything it was separating leaves a connected remainder, and
	**	covering two chunks that are individually harmless can still cut the world in half.
	**
	**	This is what Classify uses, so the ghost's colour and the server's verdict are the same
	**	answer to the same question rather than a cheap approximation and an exact one that can
	**	disagree at the boundary.  False -- "does not disconnect" -- whenever there is no
	**	navigation built to disconnect.
	*/
	static bool		Would_Disconnect_Navigation(const Vector3 & position,float radius);

private:

	//	The chunk the world point falls in, from the chunk registry's own patch bounds.  False
	//	when there is no registry, or the point is off the field.
	static bool		World_To_Chunk(float x,float y,int * px_out,int * py_out);

	static bool			ChokepointsBuilt;
	static int			ChokepointCount;
	static int			ChokepointCountX;
	static int			ChokepointCountY;
	static unsigned int	ChokepointChecksum;

	static DynamicVectorClass<int>	Chokepoint;		// px + py*ChokepointCountX, 0 or 1
};

#endif	// GENERATEDWORLDPLACEMENTRULES_H
