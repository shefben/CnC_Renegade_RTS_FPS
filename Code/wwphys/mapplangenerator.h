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
 *                     $Archive:: /Commando/Code/wwphys/mapplangenerator.h                   $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	D-003.  Phase 3B, the 2D map plan that runs before any 3D terrain exists.                    *
 *                                                                                             *
 *	C&C battlefields are laid out in plan view first: RA2's random map generator places start   *
 *	positions, ore and integer height tiers and only then dresses tier edges with cliff tiles   *
 *	and ramps; Renegade's multiplayer maps put two bases at opposite ends, a Tiberium field       *
 *	between them and two or three lanes across.  This generator produces that plan as a stack    *
 *	of 2D layers on a chunk-resolution grid so every later phase realizes intent instead of      *
 *	reacting to noise:                                                                            *
 *                                                                                             *
 *	  L0  layout graph  -- GDI/Nod base sites, centre and flank Tiberium fields, lanes, chokes,   *
 *	                       symmetry mode.  Exposed as Sites.                                      *
 *	  L1  height tiers  -- integer tier per cell (0 = lowest).  Bases sit on a flat mid tier,    *
 *	                       lanes keep tier continuity, the centre field sits low.                 *
 *	  L2  water plan    -- lowest-tier cells marked water where nothing gameplay-critical lives.  *
 *	  L3  passability   -- open / base / lane / field / cliff seam / ramp / water per cell.       *
 *                                                                                             *
 *	One owner.  The plan is data only; MacroTerrainGenerator realizes L1 into the heightfield,    *
 *	the existing cliff, hydrology and strategic owners read the result through their own          *
 *	inputs.  Nothing here touches WorldTerrainSystem, EscarpmentSystem or WaterSystem.            *
 *                                                                                             *
 *	Deterministic.  Every choice is an integer hash of (cell, seed) or a seeded walk in a fixed   *
 *	order, so a server and a client with the same seed and profile hold the same plan without    *
 *	exchanging it.  The checksum folds every layer so the manifest can prove that.                *
 *                                                                                             *
 *	Grid addressing is by world position, not chunk index: WorldTerrainSystem's chunk grid is     *
 *	derived from vertex counts and can differ by one from ceil(width / chunk), so callers sample  *
 *	with world coordinates and the generator maps those onto its own cells.                        *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef MAPPLANGENERATOR_H
#define MAPPLANGENERATOR_H

#include "always.h"
#include "vector.h"
#include "vector2.h"
#include "wwstring.h"


enum MapPlanSymmetry
{
	MAP_PLAN_SYMMETRY_NONE = 0,			// second base placed by the same rules, no mirroring
	MAP_PLAN_SYMMETRY_ROTATIONAL,			// 180 degrees about the world centre (RA2 default)
	MAP_PLAN_SYMMETRY_MIRROR_X,			// mirrored across the vertical axis through the centre
};


enum MapPlanCellKind
{
	MAP_PLAN_CELL_OPEN = 0,
	MAP_PLAN_CELL_BASE,						// inside a base site's flat disc
	MAP_PLAN_CELL_LANE,						// inside a lane corridor
	MAP_PLAN_CELL_FIELD,						// inside a Tiberium field disc
	MAP_PLAN_CELL_CLIFF_SEAM,				// tier differs from a 4-neighbour and no ramp/lane crosses
	MAP_PLAN_CELL_RAMP,						// lane crosses a tier change here; terrain must stay walkable
	MAP_PLAN_CELL_WATER,						// planned water body
};


enum MapPlanSiteKind
{
	MAP_PLAN_SITE_BASE_GDI = 0,
	MAP_PLAN_SITE_BASE_NOD,
	MAP_PLAN_SITE_FIELD_CENTER,
	MAP_PLAN_SITE_FIELD_FLANK,
	MAP_PLAN_SITE_CHOKE,
};


struct MapPlanSite
{
	MapPlanSite(void) : Kind(MAP_PLAN_SITE_BASE_GDI),CellX(0),CellY(0),Position(0.0f,0.0f),Radius(0.0f) { }

	//	DynamicVectorClass<T> (wwlib/vector.h) requires T to support == and !=.  Field-wise.
	bool	operator == (const MapPlanSite & that) const
	{
		return (Kind == that.Kind) && (CellX == that.CellX) && (CellY == that.CellY) && (Radius == that.Radius);
	}
	bool	operator != (const MapPlanSite & that) const	{ return !(*this == that); }
	MapPlanSiteKind	Kind;
	int					CellX;
	int					CellY;
	Vector2				Position;		// world, cell centre
	float					Radius;			// world units
};


struct MapPlanParams
{
	MapPlanParams(void);

	unsigned int		Seed;
	float					WorldWidth;
	float					WorldHeight;
	float					ChunkSize;
	float					CenterX;
	float					CenterY;

	MapPlanSymmetry	Symmetry;
	int					TierCount;				// >= 2 -- integer height tiers
	float					TierStep;				// world units per tier, > 0
	int					LaneWidthCells;		// >= 1 -- lane corridor half-width in cells is (LaneWidthCells)
	int					BaseRadiusCells;		// >= 1 -- flat base disc radius in cells
	int					FlankLaneCount;		// 0..2 -- lanes besides the centre lane
	float					WaterFraction;			// 0..1 -- share of lowest-tier open cells marked water
	float					BoundaryMarginCells;	// >= 0 -- cells kept clear of the world edge for sites
};


class MapPlanGenerator
{
public:

	static void				Init(void);
	static void				Shutdown(void);

	//	Builds the plan.  false with a reason on invalid params; the previous plan is cleared
	//	either way.
	static bool				Generate(const MapPlanParams & params,StringClass * error_out);
	static void				Clear(void);

	static bool				Has_Plan(void)					{ return Built; }
	static unsigned int	Get_Checksum(void)			{ return Checksum; }

	static int				Get_Cell_Count_X(void)		{ return CountX; }
	static int				Get_Cell_Count_Y(void)		{ return CountY; }
	static float			Get_Cell_Size(void)			{ return CellSize; }
	static float			Get_Tier_Step(void)			{ return TierStep; }
	static int				Get_Tier_Count(void)			{ return TierCount; }

	//	Cell queries by index.  false outside the grid.
	static bool				Get_Cell(int cx,int cy,int * tier_out,MapPlanCellKind * kind_out);

	//	World-position queries.  Outside the grid these clamp to the nearest cell so terrain
	//	sampling along the world edge never reads garbage.
	static bool				World_To_Cell(float wx,float wy,int * cx_out,int * cy_out);
	static int				Get_Tier_At(float wx,float wy);
	static MapPlanCellKind	Get_Kind_At(float wx,float wy);

	//	Realization helper for MacroTerrainGenerator.  Returns the tier height (tier * step)
	//	bilinearly blended across cell centres so a tier edge is a short slope rather than a
	//	vertical wall the relaxation pass would then smear.  CLIFF_SEAM cells are excluded from
	//	the blend on purpose so a seam stays steep enough for the interior cliff generator to
	//	pick it up; RAMP cells blend so a lane stays walkable.
	static float			Sample_Tier_Height(float wx,float wy);

	//	Site queries.
	static int				Get_Site_Count(void)			{ return Sites.Count(); }
	static bool				Get_Site(int index,MapPlanSite * out);
	static int				Get_Site_Count_By_Kind(MapPlanSiteKind kind);

	//	Strategic scoring helpers: 1.0 inside a base site disc / a Tiberium field site disc,
	//	0.0 elsewhere.
	static float			Get_Base_Site_Weight(float wx,float wy);
	static float			Get_Field_Site_Weight(float wx,float wy);

private:

	static void				Place_Sites(const MapPlanParams & params);
	static void				Paint_Lanes(const MapPlanParams & params);
	static void				Assign_Tiers(const MapPlanParams & params);
	static void				Mark_Seams_And_Ramps(void);
	static void				Plan_Water(const MapPlanParams & params);
	static void				Compute_Checksum(void);

	static void				Paint_Disc(int cx,int cy,int radius,MapPlanCellKind kind,bool overwrite);
	static void				Paint_Corridor(int x0,int y0,int x1,int y1,int half_width,unsigned int seed,int wobble);
	static void				Mirror_Cell(const MapPlanParams & params,int cx,int cy,int * mx_out,int * my_out);
	static bool				Cell_In_Grid(int cx,int cy)	{ return (cx >= 0) && (cy >= 0) && (cx < CountX) && (cy < CountY); }

	static bool									Built;
	static unsigned int						Checksum;
	static int									CountX;
	static int									CountY;
	static float								CellSize;
	static float								OriginX;			// world X of cell (0,0)'s min corner
	static float								OriginY;
	static float								TierStep;
	static int									TierCount;
	static DynamicVectorClass<signed char>		Tiers;			// CountX * CountY, row-major
	static DynamicVectorClass<unsigned char>	Kinds;			// MapPlanCellKind per cell
	static DynamicVectorClass<MapPlanSite>		Sites;
};


#endif // MAPPLANGENERATOR_H
