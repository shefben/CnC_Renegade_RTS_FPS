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
 *                     $Archive:: /Commando/Code/WWPhys/macroterraingenerator.h               $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	Roadmap Section 34 Phase 4 -- the bounded playable landform inside the immutable perimeter  *
 *	Phase 3 built.  Phase 4 asks for a "ProceduralHeightSource or equivalent that feeds the      *
 *	existing terrain system" -- this is it, and it is a generator and nothing else.  Near/far     *
 *	render, collision, LOD and patches are all already WorldTerrainSystem's own job (v1), so      *
 *	this class does exactly one thing that did not already exist: it fills a Heights[] array      *
 *	with a bounded, deterministic macro landform and hands it to WorldTerrainSystem through the   *
 *	API every other caller already uses -- Create_Terrain and Set_Heights, never a shortcut       *
 *	into HeightfieldClass directly.  That is what keeps Phase 4's first do-not-proceed condition  *
 *	("generated terrain bypasses WorldTerrainSystem") from ever being possible: there is only     *
 *	one way in.                                                                                   *
 *                                                                                             *
 *	Coexist, not replace.  A world fitted to a hand-authored level (`ProceduralWorldDimensions:: *
 *	FitToLevel`) already has ground: the level's own W3D geometry, which is what a player has     *
 *	always stood on there.  HeightfieldClass is one rectangular grid with no holes in it, so       *
 *	generating terrain anywhere in a fitted world -- even a flat plane nobody would notice --      *
 *	would stand a second, collidable ground under or through the level's real one, which is        *
 *	exactly Phase 4's second do-not-proceed condition ("procedural terrain has a separate           *
 *	collision truth").  So `Generate` is never called for a fitted world; `ProceduralWorldManager::* *
 *	Build_Macro_Terrain` is where that decision is made, and it makes it before this class is       *
 *	ever asked for anything.  This is the answer to the open question `WIP_features.md` carried      *
 *	from Phase 3's own planning note: macro terrain coexists with a hand-authored level by never      *
 *	touching it, and only ever paints ground for a world that does not have any yet.                  *
 *                                                                                             *
 *	The immutable cliff face is kept immutable the same structural way: every vertex this class     *
 *	writes is classified against `WorldBoundsService` first, and anything at or past               *
 *	WORLD_ZONE_CLIFF_BAND is written flat, at the world's own sea level, and never touched by         *
 *	the relief or the slope relaxation pass below.  A generator that "usually" left the band alone   *
 *	would still be the failure Phase 3 exists to prevent; this one is structurally unable to write   *
 *	anything else there.                                                                             *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef MACROTERRAINGENERATOR_H
#define MACROTERRAINGENERATOR_H

#include "always.h"
#include "wwstring.h"


/*
**	Plain generator-tuning numbers, filled in by the Combat-side profile translator
**	(`ProceduralWorldManager::Build_Macro_Terrain`) exactly the way `BoundaryCliffParams`
**	is filled in for the cliff ring -- wwphys never includes a Combat header, so the profile
**	crosses the layer boundary as numbers, not as a `ProceduralWorldProfile`.
*/
struct MacroTerrainParams
{
	MacroTerrainParams(void);

	unsigned int	Seed;

	//	World rectangle.  Matches `ProceduralWorldDimensions` post fit-to-level (which this
	//	generator is never actually handed a fitted profile for -- see the class comment -- but
	//	the fields are named the same for clarity).
	float	WorldWidth;
	float	WorldHeight;
	float	ChunkSize;			// world units per chunk; cell_size = ChunkSize / HeightfieldClass::PATCH_CELLS
	float	CenterX;
	float	CenterY;
	float	SeaLevel;			// the flat baseline; also what the cliff band and beyond are written at

	//	Relief.
	float	MinElevation;		// world units relative to SeaLevel, <= 0 typically -- the relief floor
	float	MaxElevation;		// world units relative to SeaLevel, > 0 -- the relief ceiling
	float	MaxBuildableSlopeRadians;	// both the generation-time ordinary-slope cap and the chunk
												//	registry's buildability threshold -- see the .cpp for why
												//	one number serves both rather than risking two definitions
												//	of "too steep" disagreeing with each other
	float	FeatureScale;		// world units, > 0 -- the approximate wavelength of a macro landform
	float	MacroRelief;		// normalized weight, 0..1
	float	HillDensity;		// normalized weight, 0..1
	float	MountainDensity;	// normalized weight, 0..1
	float	ValleyDensity;		// normalized weight, 0..1
	float	RidgeSharpness;	// normalized weight, 0..1
	float	PlateauChance;		// normalized weight, 0..1 -- 0 means never terrace a rise into a mesa
	float	BoundaryApproachFlattening;	// normalized weight, 0..1 -- how strongly relief is pulled
												//	toward the flat cliff band as the approach zone nears it

	//	Deliberately not consumed here: `InteriorCliffDensity`.  Phase 5 replaces an
	//	impossible-to-heightfield face with real cliff geometry (Section 8.1); until that
	//	exists, a density this class cannot act on would either be silently ignored (which is
	//	what happens today, honestly, by simply not being a field here) or would have to fake a
	//	steep hill and call it a cliff, which is worse.
};


class MacroTerrainGenerator
{
	public:

		static void		Init(void);
		static void		Shutdown(void);

		/*
		**	Generates a landform and hands it to WorldTerrainSystem: Create_Terrain, Set_Heights,
		**	Build_Chunk_Registry, then a best-effort Build_Collision/Build_Far_Terrain_Representation
		**	-- both already safe with no physics scene (WorldTerrainSystem's own contract), so this
		**	call is itself safe to make device-less, the same Plan/Realize shape Phase 3's cliff
		**	ring uses without needing a second pair of entry points to get it.
		**
		**	Returns false and leaves WorldTerrainSystem exactly as it was (Destroy_Terrain on any
		**	failure) on a bad parameter -- Phase 4 gets the same "refuse whole, not half" contract
		**	Phase 3's cliff ring has, for the same reason: a half-generated world is a hole in it.
		*/
		static bool		Generate(const MacroTerrainParams & params,StringClass * error_out);

		static bool				Has_Generated(void)				{ return Generated; }
		static unsigned int	Get_Terrain_Checksum(void)		{ return TerrainChecksum; }
		static int				Get_Vertex_Count_X(void)		{ return LastVertexCountX; }
		static int				Get_Vertex_Count_Y(void)		{ return LastVertexCountY; }

		/*
		**	Step 15's diagnostics, device-less -- they read WorldTerrainSystem's own heightfield
		**	and chunk registry, nothing else.  Each returns the number of violations found and
		**	writes a one-line reason for the first one into `report` when given; zero is a clean
		**	pass.  Deliberately not an assertion: a profile aggressive enough to defeat the bounded
		**	slope relaxation pass is a tuning problem the check makes visible, not a crash.
		*/
		static int		Check_Height_Range(const MacroTerrainParams & params,StringClass * report);
		static int		Check_Ordinary_Slope_Limit(const MacroTerrainParams & params,StringClass * report);
		static int		Check_Cliff_Band_Untouched(StringClass * report);

	private:

		static bool				Generated;
		static unsigned int	TerrainChecksum;
		static int				LastVertexCountX;
		static int				LastVertexCountY;
};

#endif	// MACROTERRAINGENERATOR_H
