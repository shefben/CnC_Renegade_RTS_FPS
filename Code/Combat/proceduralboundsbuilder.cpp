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
 *                     $Archive:: /Commando/Code/Combat/proceduralboundsbuilder.cpp           $*
 *                                                                                             *
 *	V2 roadmap Section 34 Phase 2 step 2.  See proceduralboundsbuilder.h.                        *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "proceduralboundsbuilder.h"

#include "random.h"
#include "worldboundsservice.h"
#include "worldseedservice.h"
#include "wwmath.h"


const char * const	ProceduralBoundsBuilder::SHAPE_IRREGULAR_RING	= "irregular_ring";
const char * const	ProceduralBoundsBuilder::SHAPE_ROUNDED_RING	= "rounded_ring";
const char * const	ProceduralBoundsBuilder::SHAPE_RECTANGLE		= "rectangle";


/*
**	How far a vertex of an irregular ring may be pulled in from the clean ellipse, as a
**	fraction of its own radius.  Chosen to read as a natural coastline rather than a circle
**	(Section 5.1 asks for an "irregular natural ring" and explicitly discourages a straight
**	square wall) while staying small enough that the ring is still obviously the shape the
**	profile's width/height asked for.
**
**	Jitter only ever SHRINKS a radius, never grows it, which is what keeps two guarantees for
**	free: the ring always fits inside the profile's declared world extent, and a star-shaped
**	ring built from strictly positive radii around one centre cannot self-intersect -- so
**	`WorldBoundsService::Configure`'s self-intersection check can never fail on anything this
**	builder produces, and a failure there would mean a real bug rather than unlucky noise.
*/
static const float	IRREGULAR_RING_JITTER		= 0.12f;
static const float	ROUNDED_RING_JITTER			= 0.0f;

//	Vertices per rectangle side, so a "rectangle" world still has enough edge samples for the
//	band-ring insets in Get_Zone_Ring to look like bands rather than four long chords.
static const int		RECTANGLE_VERTICES_PER_SIDE	= 8;


static void
Build_Ring(float half_width,float half_height,int segments,float jitter,
			  unsigned int seed,DynamicVectorClass<Vector2> * out_vertices)
{
	Random4Class rng (seed);

	for (int i = 0; i < segments; i++) {

		float angle = (2.0f * WWMATH_PI * (float)i) / (float)segments;

		/*
		**	One draw per vertex, consumed in index order.  Deterministic for a given seed, and
		**	the per-vertex value depends only on how many vertices came before it -- which is
		**	fixed by `segments`, not by anything another generator stage does.
		*/
		float scale = 1.0f - (jitter * rng.Get_Float ());

		out_vertices->Add (Vector2 (WWMath::Cos (angle) * half_width  * scale,
											 WWMath::Sin (angle) * half_height * scale));
	}
}


static void
Build_Rectangle(float half_width,float half_height,DynamicVectorClass<Vector2> * out_vertices)
{
	//	Walked as four sides rather than four corners so the edge sampling is even; the corner
	//	vertex of each side is emitted once, by that side, never duplicated by the next.
	const int PER_SIDE = RECTANGLE_VERTICES_PER_SIDE;

	for (int i = 0; i < PER_SIDE; i++) {
		float t = (float)i / (float)PER_SIDE;
		out_vertices->Add (Vector2 (-half_width + (2.0f * half_width * t),-half_height));
	}
	for (int i = 0; i < PER_SIDE; i++) {
		float t = (float)i / (float)PER_SIDE;
		out_vertices->Add (Vector2 (half_width,-half_height + (2.0f * half_height * t)));
	}
	for (int i = 0; i < PER_SIDE; i++) {
		float t = (float)i / (float)PER_SIDE;
		out_vertices->Add (Vector2 (half_width - (2.0f * half_width * t),half_height));
	}
	for (int i = 0; i < PER_SIDE; i++) {
		float t = (float)i / (float)PER_SIDE;
		out_vertices->Add (Vector2 (-half_width,half_height - (2.0f * half_height * t)));
	}
}


bool ProceduralBoundsBuilder::Build_Boundary_Polygon(const ProceduralWorldProfile & profile,
																		  DynamicVectorClass<Vector2> * out_vertices,
																		  StringClass * error_out)
{
	if (out_vertices == nullptr) {
		if (error_out != nullptr) *error_out = "null output vertex list";
		return false;
	}

	//	The profile has to be valid before its numbers mean anything -- a negative width would
	//	otherwise produce a mirrored ring rather than an error.
	if (!ProceduralWorldProfileManager::Validate_Profile (profile,error_out)) {
		return false;
	}

	out_vertices->Delete_All ();

	float half_width	= profile.World.Width  * 0.5f;
	float half_height	= profile.World.Height * 0.5f;

	unsigned int seed = WorldSeedService::Derive_Subsystem_Seed (profile.Seed,
																					 WorldSeedService::SEED_DOMAIN_BOUNDARY);

	const char * shape = profile.Boundary.Shape.Peek_Buffer ();

	if (::strcmp (shape,SHAPE_IRREGULAR_RING) == 0) {
		Build_Ring (half_width,half_height,WorldBoundsService::SUGGESTED_RING_SEGMENTS,
						IRREGULAR_RING_JITTER,seed,out_vertices);

	} else if (::strcmp (shape,SHAPE_ROUNDED_RING) == 0) {
		Build_Ring (half_width,half_height,WorldBoundsService::SUGGESTED_RING_SEGMENTS,
						ROUNDED_RING_JITTER,seed,out_vertices);

	} else if (::strcmp (shape,SHAPE_RECTANGLE) == 0) {
		Build_Rectangle (half_width,half_height,out_vertices);

	} else {
		if (error_out != nullptr) {
			StringClass message (128u,true);
			message.Format ("unknown boundary.shape '%s'",shape);
			*error_out = message;
		}
		out_vertices->Delete_All ();
		return false;
	}

	/*
	**	Every shape above is built about the origin, because the jitter and the rectangle walk
	**	both read more clearly that way.  The profile's centre is applied once, here, afterwards
	**	-- rather than threaded through three builders that would each have to remember it.  A
	**	world generated from nothing leaves the centre at the origin and this loop changes
	**	nothing; a world fitted over a hand-authored level moves the whole ring onto that
	**	level's own ground, which is not built around the origin.
	*/
	if ((profile.World.CenterX != 0.0f) || (profile.World.CenterY != 0.0f)) {
		Vector2 centre (profile.World.CenterX,profile.World.CenterY);
		for (int i = 0; i < out_vertices->Count (); i++) {
			(*out_vertices) [i] = (*out_vertices) [i] + centre;
		}
	}

	return true;
}


bool ProceduralBoundsBuilder::Configure_World_Bounds(const ProceduralWorldProfile & profile,
																		  StringClass * error_out)
{
	DynamicVectorClass<Vector2> vertices;
	if (!Build_Boundary_Polygon (profile,&vertices,error_out)) {
		return false;
	}

	return WorldBoundsService::Configure (vertices,
													  profile.Boundary.CliffBandWidth,
													  profile.Boundary.HardBarrierOffset,
													  profile.Boundary.AirCeiling,
													  error_out);
}
