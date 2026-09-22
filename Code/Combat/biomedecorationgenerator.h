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
 *                     $Archive:: /Commando/Code/Combat/biomedecorationgenerator.h            $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	V2 roadmap Section 34 Phase 14 -- biomes, terrain materials, forests, rocks, and natural       *
 *	decoration.  Section 13, Section 10.6, Section 28.                                            *
 *                                                                                             *
 *	Real placement through the one existing system.  `FoliageSystem::Define_Default_Foliage`       *
 *	(already called by `FoliageSystem::Init`) already defines five real, working foliage types      *
 *	-- `ow_tree_conifer`, `ow_bush_scrub`, `ow_tree_dead`, `ow_grass_clump`, `ow_rock_boulder` --      *
 *	each with real blocking/destructible/LOD-distance/scale-range data; that file's own comment       *
 *	says plainly that none of them names a model yet, so a forest of them "plans exactly the same       *
 *	batches and draws nothing at all" until there is art.  This generator's job is deciding WHERE       *
 *	those five real types belong and placing real instances through `FoliageSystem::Add_Instance` --    *
 *	never `FoliageSystem::Scatter`, whose own uniform-disc placement checks no terrain legality at        *
 *	all (confirmed by reading its own implementation) -- then calling the system's own real                *
 *	`Build_Cells`/`Build_Batches` (step 13's own LOD/budget acceptance, already built).                     *
 *                                                                                             *
 *	Masks already real.  `TERRAIN_MASK_ROAD` is stamped by `RoadSystem`; `TERRAIN_MASK_TIBERIUM`          *
 *	by `TiberiumFieldGenerator` (Phase 12); `TERRAIN_MASK_WATER_DISTANCE` by `WaterSystem::                 *
 *	Update_Water_Distance`.  This generator reads all three rather than re-deriving any of them.            *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef BIOMEDECORATIONGENERATOR_H
#define BIOMEDECORATIONGENERATOR_H

#ifndef ALWAYS_H
#include "always.h"
#endif

#include "wwstring.h"


struct BiomeDecorationParams
{
	BiomeDecorationParams(void);

	unsigned int	Seed;

	//	The same shared cap every terrain-reading generator in this chain already uses.
	float	MaxBuildableSlopeRadians;

	//	No profile field carries Section 28's own forest/rock density yet -- the same class of
	//	disclosed, fixed-constant simplification `SettlementGenerator`'s own footprint radius
	//	already is.  0..1; 0 disables that category outright.
	float	ForestDensity;
	float	RockDensity;
};


class BiomeDecorationGenerator
{
public:

	static void			Init(void);
	static void			Shutdown(void);

	/*
	**	Requires WorldTerrainSystem::Has_Chunk_Registry().  Runs whether or not any of Phase 11/
	**	12/13's own generators produced anything -- their reserved regions are read when present
	**	and simply contribute nothing to exclude when absent, the same optional-dependency
	**	pattern StrategicPlanner's own Phase 5/6 candidate reuse already established.
	*/
	static bool			Generate(const BiomeDecorationParams & params,StringClass * error_out);

	//	FoliageSystem::Clear_Instances -- FoliageSystem carries no per-instance removal API (no
	//	"Remove_Instance", only a full wipe), and no other generator in this procedural chain
	//	ever adds an instance, so this generator is the shared store's only writer here; a full
	//	clear is the correct reset for a world regeneration, the same as every hand-authored
	//	map's own instances would be if one were ever loaded through the same procedural path.
	static void			Clear(void);
	static bool			Has_Decoration(void)	{ return Built; }

	static int			Get_Tree_Count(void)		{ return TreeCount; }
	static int			Get_Rock_Count(void)		{ return RockCount; }
	static int			Get_Forest_Chunk_Count(void)	{ return ForestChunkCount; }

	static unsigned int	Get_Checksum(void)		{ return Checksum; }

	static int			Check_No_Instance_In_Road(StringClass * report);
	static int			Check_No_Instance_Off_Terrain(StringClass * report);

private:

	static bool			Built;
	static unsigned int	Checksum;

	static int				TreeCount;
	static int				RockCount;
	static int				ForestChunkCount;
};

#endif	// BIOMEDECORATIONGENERATOR_H
