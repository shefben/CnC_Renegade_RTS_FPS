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
 *                     $Archive:: /Commando/Code/Combat/minimapbaselayer.h                    $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	V2 roadmap Section 34 Phase 16 step 12 -- minimap/tactical-map base representation.           *
 *                                                                                             *
 *	Real, currently unconsumed.  Grepping this tree for a minimap/tactical-map renderer or          *
 *	base-layer class turns up nothing -- there is no v1 consumer this file integrates with, only      *
 *	`WorldShroudManager` (live per-team coverage) and `TacticalIntelligenceModel` (live per-object     *
 *	contacts), neither of which owns a *static* map background.  So this is real output with no         *
 *	renderer to hand it to yet, the same honesty `CombatSpaceValidator`'s own repair-operation             *
 *	names already carry.                                                                                    *
 *                                                                                             *
 *	Reuse, not a second terrain summary.  The cell grid is `WorldTerrainSystem`'s own chunk                  *
 *	registry, read, not re-derived -- the same registry `NavMeshGenerator` and                               *
 *	`BiomeDecorationGenerator` already read.  Road/water/cliff classification per cell comes from               *
 *	the same real masks and queries every earlier phase already established                                    *
 *	(`TERRAIN_MASK_ROAD`, `WaterSystem::Is_Point_In_Water`, `TerrainChunkInfoClass::CliffBand`).                 *
 *	Markers name real, already-resolved objects (`BaseSelectionPlanner`, `TiberiumFieldGenerator`,               *
 *	`SettlementGenerator`, `BridgePlanner`) by the same stable ids their own owners already assign.               *
 *	Tunnel portals, named in step 12's own list, are not produced -- no tunnel system exists                      *
 *	anywhere in this tree (Phase 16's own recon; see docs/procedural_world/FinalWorldIntegration.md).             *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef MINIMAPBASELAYER_H
#define MINIMAPBASELAYER_H

#ifndef ALWAYS_H
#include "always.h"
#endif

#include "vector.h"
#include "vector3.h"
#include "wwstring.h"


enum MinimapMarkerType
{
	MINIMAP_MARKER_BASE = 0,
	MINIMAP_MARKER_SETTLEMENT,
	MINIMAP_MARKER_POI,
	MINIMAP_MARKER_TIBERIUM_FIELD,
	MINIMAP_MARKER_BRIDGE,
	MINIMAP_MARKER_TYPE_COUNT
};

const char *	Minimap_Marker_Type_Name(MinimapMarkerType type);


class MinimapMarker
{
public:
	MinimapMarker(void);

	bool	operator == (const MinimapMarker & that) const;
	bool	operator != (const MinimapMarker & that) const	{ return !(*this == that); }

	StringClass				Id;			// the owning generator's own stable id
	MinimapMarkerType	Type;
	Vector3					Position;
	float						Radius;		// world units; 0 when not meaningful for this marker type
};


//	One cell, at the chunk registry's own resolution -- see the header comment for why this is
//	not a second, finer terrain summary.
class MinimapCellClass
{
public:
	MinimapCellClass(void);

	bool	operator == (const MinimapCellClass &) const	{ return false; }
	bool	operator != (const MinimapCellClass &) const	{ return true; }

	float		AverageHeight;
	bool		IsWater;
	bool		IsCliffBand;
	bool		HasRoad;
};


class MinimapBaseLayer
{
public:

	static void		Init(void);
	static void		Shutdown(void);

	//	Requires WorldTerrainSystem::Has_Chunk_Registry().
	static bool		Build(StringClass * error_out);

	static void		Clear(void);
	static bool		Has_Layer(void)	{ return Built; }

	static int		Get_Cell_Count_X(void)	{ return CellCountX; }
	static int		Get_Cell_Count_Y(void)	{ return CellCountY; }
	static bool		Get_Cell(int px,int py,MinimapCellClass * out);

	static int		Get_Marker_Count(void)	{ return Markers.Count(); }
	static bool		Get_Marker(int index,MinimapMarker * out);

	static unsigned int	Get_Checksum(void)	{ return Checksum; }

private:

	static bool			Built;
	static int				CellCountX;
	static int				CellCountY;
	static unsigned int	Checksum;

	static DynamicVectorClass<MinimapCellClass>	Cells;
	static DynamicVectorClass<MinimapMarker>		Markers;
};

#endif	// MINIMAPBASELAYER_H
