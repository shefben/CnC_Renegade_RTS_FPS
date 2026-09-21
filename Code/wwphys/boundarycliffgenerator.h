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
 *                     $Archive:: /Commando/Code/wwphys/boundarycliffgenerator.h              $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	V2 roadmap Section 34 Phase 3.  The cliff ring that gives Phase 2's invisible hard boundary  *
 *	a physical explanation: Sections 5.3 to 5.5.                                                *
 *                                                                                             *
 *	What this is NOT.  Phase 3's design text names five components -- BoundaryCliffGenerator,   *
 *	CliffSpline, CliffSegmentMeshBuilder, CliffCollisionBuilder, CliffMaterialProjector -- and   *
 *	says to "create or adapt something equivalent".  Four of those five already exist in this    *
 *	tree under other names, so this file is one generator and nothing else:                      *
 *                                                                                             *
 *	  cliff spline           -> EscarpmentClass's polyline of EscarpmentPointClass stations      *
 *	  segment mesh builder   -> EscarpmentSystem::Build_Escarpment_Geometry, itself on           *
 *	                            DynamicMeshBuilderClass, the shared runtime-mesh helper the      *
 *	                            road, bridge, foliage and water systems all already use          *
 *	  collision builder      -> the StaticPhysClass that same function adds to the physics       *
 *	                            scene, with the cliff collision bits escarpments already carry   *
 *	  material projector     -> the TERRAIN_LAYER_CLIFF terrain layer and                        *
 *	                            MATERIAL_PROGRAM_TERRAIN, which escarpments already resolve      *
 *                                                                                             *
 *	Writing a second cliff mesh path beside the one already here would have been the parallel    *
 *	implementation the project's own rules forbid, and would have produced two answers to        *
 *	"is there a wall at x,y".                                                                    *
 *                                                                                             *
 *	Why the ring is many escarpments and not one.  Phase 3 refuses to proceed if "the ring       *
 *	depends on one monolithic unbounded mesh", and EscarpmentSystem builds exactly one mesh and  *
 *	one physics object per escarpment.  A perimeter handed over as a single escarpment would     *
 *	therefore have been precisely that monolith -- one draw call and one collider the size of    *
 *	the world, culled all-or-nothing.  So the ring is cut into bounded chunks, each its own      *
 *	escarpment, and the seam between two chunks is not "close enough": chunk k's last station    *
 *	is the *same computed station* as chunk k+1's first, taken from one canonical station array  *
 *	that is built once.  Two chunks cannot drift apart because neither one owns the number.      *
 *                                                                                             *
 *	Which way the face points.  An escarpment drops to the right of its direction of travel.     *
 *	WorldBoundsService normalizes its polygon to clockwise, and the interior of a clockwise       *
 *	polygon lies to the right of every edge -- so walking the ring in polygon order drops the     *
 *	face inward, putting the high ground outboard and the playable interior at the foot of the    *
 *	cliff looking up at it.  Walking it the other way would build the map as a mesa to fall off   *
 *	rather than a basin to be contained by, so the direction is load-bearing, not incidental.     *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef BOUNDARYCLIFFGENERATOR_H
#define BOUNDARYCLIFFGENERATOR_H

#ifndef ALWAYS_H
#include "always.h"
#endif

#ifndef VECTOR2_H
#include "vector2.h"
#endif

#ifndef VECTOR3_H
#include "vector3.h"
#endif

#ifndef VECTOR_H
#include "vector.h"
#endif

#ifndef WWSTRING_H
#include "wwstring.h"
#endif


/*
**	Everything the ring needs, as plain numbers.
**
**	Deliberately not the Phase 1 ProceduralWorldProfile: that type lives in Combat, this file
**	lives in wwphys, and wwphys does not link Combat.  This is the same layer edge Phase 2
**	crossed with WorldBoundsService::Configure, and it is crossed the same way -- Combat fills
**	a struct of numbers from the profile and hands it down.
*/
struct BoundaryCliffParams
{
	BoundaryCliffParams(void) :
		Seed (1),
		CliffMinHeight (60.0f),
		CliffMaxHeight (110.0f),
		CliffBandWidth (120.0f),
		ToeWidth (12.0f),
		RimRoughness (0.35f),
		FaceRoughness (0.45f),
		CollisionSimplification (2),
		BaseHeight (0.0f),
		StationSpacing (12.0f),
		StationsPerChunk (16),
		Batter (0.08f),
		LayerIndex (-1)
		{ }

	//	Already derived for the cliff domain by the caller (WorldSeedService); this class does
	//	no seed derivation of its own, so there is exactly one place that knows how domains work.
	unsigned int	Seed;

	float	CliffMinHeight;			// world units, > 0
	float	CliffMaxHeight;			// world units, >= CliffMinHeight
	float	CliffBandWidth;			// world units, > 0 -- how far inward the ring's own band reaches
	float	ToeWidth;					// world units, >= 0, < CliffBandWidth
	float	RimRoughness;				// 0..1, how much the rim wanders between min and max height
	float	FaceRoughness;				// 0..1, how much the face inflects between toe and rim
	int	CollisionSimplification;// >= 1 -- reserved; see the note on Get_Face_Row_Count below

	float	BaseHeight;					// world units -- the height the cliff foot stands on (profile sea level)

	//	How finely the polygon is resampled.  A boundary polygon has a handful of vertices; a
	//	cliff needs stations far closer together than that or the "ring" is a coarse fence.
	float	StationSpacing;			// world units between stations, > 0

	//	How many stations one chunk carries.  This is the knob that keeps any single mesh
	//	bounded no matter how big the world gets.
	int	StationsPerChunk;			// >= 2

	float	Batter;						// metres of set-back per metre of height, passed straight to the escarpment
	int	LayerIndex;					// terrain layer that dresses the face; -1 means "the first cliff layer"
};


class BoundaryCliffGenerator
{
public:

	static void			Init(void);
	static void			Shutdown(void);

	/*
	**	Build the ring from a closed boundary polygon.  The polygon is the one
	**	WorldBoundsService is already configured with -- passed in rather than read back out,
	**	so this stays a pure function of its arguments and the self check can drive it with no
	**	boundary installed at all.
	**
	**	Returns false and builds nothing if the parameters or the polygon are unusable.  There
	**	is no partial ring: half a cliff ring is worse than none, because the gap is exactly
	**	the traversal leak Phase 3 exists to prevent, and it would be invisible from inside.
	*/
	static bool			Build_Ring(const DynamicVectorClass<Vector2> & boundary_polygon,
											const BoundaryCliffParams & params,StringClass * error_out);

	/*
	**	The half of Build_Ring that decides what the ring IS: the canonical station array, the
	**	chunk division, and the escarpments carrying them.  Deterministic, and it touches no
	**	render object and no physics scene, so it runs in a device-less process.
	**
	**	It is public because that is what makes the ring checkable.  Every property Phase 3
	**	actually cares about -- same seed means same ring, chunk seams share stations, no
	**	station is short enough to walk up, no face is inverted -- is a property of the plan,
	**	not of the triangles, and a check that cannot stand up a physics scene can still hold
	**	the plan to all four.
	*/
	static bool			Plan_Ring(const DynamicVectorClass<Vector2> & boundary_polygon,
										  const BoundaryCliffParams & params,StringClass * error_out);

	/*
	**	The other half: turn the planned ring into meshes and colliders.  Needs a physics
	**	scene.  Separate from Plan_Ring only so the plan can be checked; normal callers want
	**	Build_Ring, which is the two in order.
	*/
	static bool			Realize_Ring(StringClass * error_out);

	static bool			Is_Planned(void)				{ return StationCount > 0; }

	/*
	**	Drop the ring.  Only removes escarpments this generator created, so a map that also
	**	has hand-placed escarpments keeps them.
	*/
	static void			Clear_Ring(void);

	static bool			Has_Ring(void)					{ return RingBuilt; }
	static int			Get_Chunk_Count(void)		{ return ChunkCount; }
	static int			Get_Station_Count(void)		{ return StationCount; }

	/*
	**	Phase 3 step 17.  A checksum over the canonical station array -- position, both bench
	**	heights and inflection of every station, quantized -- so "the same seed produced the
	**	same ring" is something that can be shown rather than assumed.  Zero with no ring.
	*/
	static unsigned int	Get_Ring_Checksum(void)	{ return RingChecksum; }

	/*
	**	Phase 3 step 15.  The seam diagnostics, run over the built ring.  Each returns a count
	**	of faults found and appends a human-readable line per fault to `report` when given one.
	**
	**	Render/collision gaps are the same question asked once, because the ring's render mesh
	**	and its collider are the same geometry -- an escarpment's StaticPhysClass collides with
	**	the very mesh it draws, so a render gap IS a collision gap and there is no way for the
	**	two to disagree.  That is a property worth having rather than a corner cut.
	*/
	static int			Check_Seam_Continuity(StringClass * report);
	static int			Check_Inverted_Triangles(StringClass * report);
	static int			Check_Traversable_Ramps(float max_climbable_height,StringClass * report);

	/*
	**	Everything the debug layer needs to draw the ring without reaching into escarpments.
	*/
	static int			Get_Ring_Station(int index,Vector3 * toe_out,Vector3 * rim_out);

private:

	static bool			Resample_Polygon(const DynamicVectorClass<Vector2> & polygon,float spacing,
												  DynamicVectorClass<Vector2> * out_stations);

	static bool			RingBuilt;
	static int			ChunkCount;
	static int			StationCount;
	static unsigned int	RingChecksum;

	//	IDs of the escarpments this generator owns, so Clear_Ring can drop exactly those.
	static DynamicVectorClass<int>	OwnedEscarpmentIds;

	//	The canonical stations, kept after the build so the diagnostics and the debug layer
	//	ask the same array the geometry was built from rather than re-deriving it and possibly
	//	disagreeing with it.
	static DynamicVectorClass<Vector3>	StationToe;
	static DynamicVectorClass<Vector3>	StationRim;
};

#endif	// BOUNDARYCLIFFGENERATOR_H
