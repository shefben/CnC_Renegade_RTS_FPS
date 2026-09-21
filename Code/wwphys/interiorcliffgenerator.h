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
 *                     $Archive:: /Commando/Code/wwphys/interiorcliffgenerator.h              $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	V2 roadmap Section 34 Phase 5.  Interior cliffs, ravines and canyon walls -- the vertical    *
 *	relief a heightfield alone cannot hold, placed inside the playable interior rather than      *
 *	around its edge.  Section 8.                                                                *
 *                                                                                             *
 *	Reuse, not a second cliff renderer.  Phase 3's BoundaryCliffGenerator turned out to be a     *
 *	thin planner over EscarpmentClass/EscarpmentSystem (escarpment.h), which already is the      *
 *	general "polyline with an upper and lower bench" cliff-face primitive Phase 5 step 4 asks    *
 *	for -- an open, non-ring polyline builds through it exactly as a closed boundary ring does.  *
 *	This generator is the same shape: a planner that emits EscarpmentClass chunks and hands them *
 *	to the one shared system, tracking only the IDs it created so Clear removes exactly its own  *
 *	work and leaves the boundary ring, and any hand-placed escarpment, standing.  The two        *
 *	generators' owned-ID lists partition the same EscarpmentSystem collection; neither ever      *
 *	iterates or clears the whole collection.                                                     *
 *                                                                                             *
 *	Where a candidate comes from.  Phase 4's chunk registry already carries, per chunk, the       *
 *	post-relaxation slope extremes MacroTerrainGenerator's own bounded relaxation pass pushed      *
 *	every ordinary chunk down toward the profile's buildable-slope cap.  A chunk still sitting     *
 *	at that cap after relaxation is exactly the terrain that "requested slope/relief exceeding     *
 *	ordinary heightfield limits" (Phase 5 step 1's own phrase) and was capped rather than given     *
 *	the vertical face it wanted.  So candidates are read from data the terrain generator already   *
 *	computed, not synthesized from a second noise field that could disagree with it.               *
 *                                                                                             *
 *	Terrain conforms cliffs; cliffs do not conform terrain.  Escarpment.h's own                    *
 *	Conform_To_Terrain resamples every escarpment currently registered, which would also            *
 *	resample the boundary ring's authored rim/toe against the (deliberately flat) cliff-band        *
 *	terrain and collapse it.  This generator never calls it; benches are sampled directly, only      *
 *	for the stations this generator itself is adding, through WorldTerrainSystem::Sample_Height      *
 *	one inset either side of the local path direction -- the same idea Conform_To_Terrain uses,       *
 *	scoped so it can never touch a station it did not create.                                         *
 *                                                                                             *
 *	What this does NOT attempt.  Ridge-line extraction from a slope field is a real image-           *
 *	processing problem (non-maximum suppression along a gradient, watershed segmentation) and         *
 *	this is not that: a candidate run is a deterministic connected walk of steep, unclaimed            *
 *	chunks, ordered by nearest-neighbour continuation, not a mathematically optimal crest line.         *
 *	It reads as a wall because it is real, collision-backed geometry conformed to real ground on         *
 *	both sides, not because the walk is claimed to be the best one through the terrain.                   *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef INTERIORCLIFFGENERATOR_H
#define INTERIORCLIFFGENERATOR_H

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


/*
**	Section 8.1's six interior cliff kinds, minus COASTAL_CLIFF.  This generator runs before
**	Phase 6 hydrology exists to say where a coast is, so it can never assign that kind -- left
**	out of the enum entirely rather than a value this file promises never to pick, which is a
**	stronger guarantee than a runtime check could make.  A future Phase 6 pass may retag a
**	feature that turns out to border generated water; that retagging is Phase 6's job, not
**	this file's, and would read this enum's own comment before adding a member here.
*/
enum InteriorCliffType
{
	INTERIOR_CLIFF_ESCARPMENT = 0,
	INTERIOR_CLIFF_CANYON_WALL,
	INTERIOR_CLIFF_MOUNTAIN_FACE,
	INTERIOR_CLIFF_RAVINE_WALL,
	INTERIOR_CLIFF_QUARRY_STYLE,
	INTERIOR_CLIFF_TYPE_COUNT
};

//	Section 8.3's five traversability kinds, named exactly as that section lists them.
enum InteriorCliffTraversabilityType
{
	INTERIOR_CLIFF_NON_TRAVERSABLE = 0,
	INTERIOR_CLIFF_INFANTRY_RAMP_NEARBY,
	INTERIOR_CLIFF_VEHICLE_RAMP_NEARBY,
	INTERIOR_CLIFF_TUNNEL_BYPASS,
	INTERIOR_CLIFF_BRIDGE_CROSSING,
	INTERIOR_CLIFF_TRAVERSABILITY_COUNT
};


/*
**	Everything the generator needs, as plain numbers -- the same layer edge BoundaryCliffParams
**	and MacroTerrainParams already cross (wwphys does not link Combat; Combat fills this from
**	the profile and hands it down).
*/
struct InteriorCliffParams
{
	InteriorCliffParams(void);

	//	Already derived for the interior-cliff domain by the caller (WorldSeedService).
	unsigned int	Seed;

	//	Fraction of MaxBuildableSlopeRadians a chunk's own post-relaxation MaxSlopeRadians must
	//	reach to be a cliff candidate at all.  1.0 means only chunks sitting exactly at the cap;
	//	values below 1.0 admit chunks the relaxation pass reduced toward the limit without quite
	//	reaching it, which is real too -- see macroterraingenerator.cpp's own note that
	//	relaxation "reduces toward the limit", not a hard guarantee.
	float	SlopeThresholdFraction;

	//	The profile's own buildable-slope cap, radians -- the same number Phase 4's chunk
	//	registry was built with.  Deliberately one shared number rather than a second
	//	independent "too steep" definition; see TerrainChunkInfoClass's own comment on why.
	float	MaxBuildableSlopeRadians;

	//	Normalized weight, 0..1 -- ProceduralWorldTerrainSettings::InteriorCliffDensity.  Gates
	//	which qualifying chunks actually start a cliff run; 0 admits none, 1 admits every
	//	qualifying chunk as a candidate seed.
	float	Density;

	//	Chunks.  A run shorter than 2 is discarded rather than built -- Phase 5's own
	//	do-not-proceed forbids "interior cliffs... implemented as extreme heightfield spikes",
	//	and a single isolated chunk is exactly that spike.
	int	MaxRunLength;

	float	Batter;			// metres of set-back per metre of height, passed straight to the escarpment
	float	ToeWidth;		// world units, >= 0 -- the scree/debris skirt Section 8.4 asks for
	int	LayerIndex;		// terrain layer that dresses the face; -1 means "the first cliff layer"
};


/*
**	One interior cliff, after it has been built.  Read-only outside the generator; Section 8.3
**	step 9's "identify nearby potential" is BypassPoint/HasBypassPoint -- a single representative
**	point, not a mesh or a placed object, because Phase 5 step 10 explicitly forbids building the
**	actual bridge/tunnel/ramp yet.  Later phases (8, 9, 10) are the ones that turn a candidate
**	into geometry.
*/
class InteriorCliffFeature
{
public:
	InteriorCliffFeature(void);

	//	DynamicVectorClass<T> (wwlib/vector.h) requires T to support == and != -- the same reason
	//	EscarpmentClass and EscarpmentPointClass each carry their own.  Field-wise, same as both.
	bool	operator == (const InteriorCliffFeature & that) const;
	bool	operator != (const InteriorCliffFeature & that) const	{ return !(*this == that); }

	int		EscarpmentId;			// the EscarpmentSystem id this feature owns
	InteriorCliffType					Type;
	InteriorCliffTraversabilityType	Traversability;
	float		Length;				// world units, along the polyline
	float		MaxFaceHeight;		// world units, the tallest station on this feature
	Vector3	BypassPoint;		// midpoint of the run; meaningful only when HasBypassPoint
	bool		HasBypassPoint;
};


class InteriorCliffGenerator
{
public:

	static void			Init(void);
	static void			Shutdown(void);

	/*
	**	Plan and realize in one call -- interior cliffs have no scene-dependent half worth
	**	splitting out on its own the way Phase 3's ring does, because EscarpmentSystem's
	**	Build_Geometry already declines cleanly with no physics scene (Has_Geometry stays
	**	false, nothing throws), so the device-less self check calls this directly and simply
	**	does not find geometry built.
	**
	**	Requires WorldTerrainSystem::Has_Chunk_Registry() -- interior cliffs are read from the
	**	chunk registry's own slope data, so there is nothing to plan without it.  Returns false
	**	and builds nothing on any input a caller should not have passed; a world simply too flat
	**	or too small to have any qualifying chunk still returns true with zero features, because
	**	that is a correct, unremarkable outcome, not an error.
	*/
	static bool			Generate(const InteriorCliffParams & params,StringClass * error_out);

	//	Drop only this generator's own escarpments and chunk-registry tags.  Mirrors
	//	BoundaryCliffGenerator::Clear_Ring exactly, including the same "read the collection back
	//	minus what is owned, then re-add" idiom for the shared EscarpmentSystem collection.
	static void			Clear(void);

	static bool			Has_Cliffs(void)				{ return Built; }
	static int			Get_Feature_Count(void)		{ return Features.Count(); }
	static bool			Get_Feature(int index,InteriorCliffFeature * out);

	//	A CRC over every feature's own type, traversability, length, tallest station and bypass
	//	point, quantized (Section 3B.4) -- the same "same seed, same checksum" claim Phase 3 and
	//	Phase 4 each make for their own output.  Zero with no cliffs built.
	static unsigned int	Get_Checksum(void)			{ return Checksum; }

	/*
	**	Section 34 Phase 5's own diagnostics, plus the two carried over from Phase 3/4's
	**	do-not-proceed list that a second generator touching the same terrain/escarpment
	**	collection could violate without either of them knowing.
	*/
	static int			Check_No_Cliff_Band_Overlap(StringClass * report);
	static int			Check_No_Single_Chunk_Spikes(StringClass * report);
	static int			Check_Bypass_Candidates_Present(StringClass * report);
	static int			Check_No_Inverted_Faces(StringClass * report);

private:

	static bool			Built;
	static unsigned int	Checksum;

	static DynamicVectorClass<int>						OwnedEscarpmentIds;
	static DynamicVectorClass<InteriorCliffFeature>	Features;
};

#endif	// INTERIORCLIFFGENERATOR_H
