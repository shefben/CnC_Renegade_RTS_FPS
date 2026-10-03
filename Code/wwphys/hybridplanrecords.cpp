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
 ***                            Confidential - Westwood Studios                              ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : OpenW3D                                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwphys/hybridplanrecords.cpp                 $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Hybrid rewrite item I1 -- name tables, the Section 7 road-class table and the small pure    *
 * rules (legal biome pairs, route family count, patch spacing) the records header declares.   *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#include "hybridplanrecords.h"
#include "hybridplanning.h"
#include "wwmath.h"

#include <string.h>


namespace
{
	const char * const ARCHETYPE_NAMES [HYBRID_ARCHETYPE_COUNT] =
	{
		"river_valley",
		"twin_lakes",
		"canyon_crossroads",
		"highland_pass",
		"coastal_peninsula",
		"plateau_basin",
		"ravine_network",
		"open_frontier",
	};

	const char * const BIOME_NAMES [HYBRID_BIOME_COUNT] =
	{
		"temperate",
		"desert",
		"snow",
		"tiberium_wasteland",
	};

	const char * const BIOME_MODE_NAMES [HYBRID_BIOME_MODE_COUNT] =
	{
		"auto",
		"single",
		"mixed",
	};

	const char * const ROAD_CLASS_NAMES [HYBRID_ROAD_CLASS_COUNT] =
	{
		"HIGHWAY_4",
		"PRIMARY_2",
		"SECONDARY_2",
		"SERVICE_DIRT_2",
		"TRAIL_1",
		"HIGHWAY_RAMP",
	};

	//	Section 7 cross-section table.  Lane width is W for every class.
	const HybridRoadClassSpec ROAD_CLASS_SPECS [HYBRID_ROAD_CLASS_COUNT] =
	{
		//	class							lanes	inner		median	grade	x-slope	turn	tier								outer
		{	HYBRID_ROAD_HIGHWAY_4,			4,		0.45f,	0.75f,	8.0f,		6.0f,		8.0f,	HYBRID_ROAD_TIER_HIGHWAY,	0.45f	},
		{	HYBRID_ROAD_PRIMARY_2,			2,		0.30f,	0.0f,		12.0f,	8.0f,		5.0f,	HYBRID_ROAD_TIER_PAVED,		0.30f	},
		{	HYBRID_ROAD_SECONDARY_2,		2,		0.20f,	0.0f,		14.0f,	10.0f,	4.0f,	HYBRID_ROAD_TIER_PAVED,		0.20f	},
		{	HYBRID_ROAD_SERVICE_DIRT_2,	2,		0.20f,	0.0f,		18.0f,	12.0f,	3.0f,	HYBRID_ROAD_TIER_DIRT,		0.20f	},
		{	HYBRID_ROAD_TRAIL_1,				1,		0.10f,	0.0f,		22.0f,	14.0f,	2.0f,	HYBRID_ROAD_TIER_DIRT,		0.10f	},
		//	Section 12.6.6: one lane, 0.20W inner and 0.30W outer shoulder (1.50W), the normal paved tier and speed hint.
		{	HYBRID_ROAD_HIGHWAY_RAMP,		1,		0.20f,	0.0f,		12.0f,	8.0f,		5.0f,	HYBRID_ROAD_TIER_PAVED,		0.30f	},
	};

	int Find_Name(const char * const * table,int count,const char * name)
	{
		if (name == nullptr) {
			return -1;
		}
		for (int index = 0; index < count; index ++) {
			if (::strcmp (table[index],name) == 0) {
				return index;
			}
		}
		return -1;
	}

	float Min_Float(float a,float b)
	{
		return (a < b) ? a : b;
	}
}


const char * Hybrid_Archetype_Name(HybridArchetype archetype)
{
	if (archetype == HYBRID_ARCHETYPE_AUTO) return "auto";
	if (archetype < 0 || archetype >= HYBRID_ARCHETYPE_COUNT) return "";
	return ARCHETYPE_NAMES[archetype];
}


bool Hybrid_Archetype_From_Name(const char * name,HybridArchetype * out)
{
	if (name != nullptr && ::strcmp (name,"auto") == 0) {
		if (out != nullptr) *out = HYBRID_ARCHETYPE_AUTO;
		return true;
	}
	int index = Find_Name (ARCHETYPE_NAMES,HYBRID_ARCHETYPE_COUNT,name);
	if (index < 0) return false;
	if (out != nullptr) *out = (HybridArchetype)index;
	return true;
}


const char * Hybrid_Biome_Name(HybridBiome biome)
{
	if (biome == HYBRID_BIOME_AUTO) return "auto";
	if (biome < 0 || biome >= HYBRID_BIOME_COUNT) return "";
	return BIOME_NAMES[biome];
}


bool Hybrid_Biome_From_Name(const char * name,HybridBiome * out)
{
	if (name != nullptr && ::strcmp (name,"auto") == 0) {
		if (out != nullptr) *out = HYBRID_BIOME_AUTO;
		return true;
	}
	int index = Find_Name (BIOME_NAMES,HYBRID_BIOME_COUNT,name);
	if (index < 0) return false;
	if (out != nullptr) *out = (HybridBiome)index;
	return true;
}


const char * Hybrid_Biome_Mode_Name(HybridBiomeMode mode)
{
	if (mode < 0 || mode >= HYBRID_BIOME_MODE_COUNT) return "";
	return BIOME_MODE_NAMES[mode];
}


bool Hybrid_Biome_Mode_From_Name(const char * name,HybridBiomeMode * out)
{
	int index = Find_Name (BIOME_MODE_NAMES,HYBRID_BIOME_MODE_COUNT,name);
	if (index < 0) return false;
	if (out != nullptr) *out = (HybridBiomeMode)index;
	return true;
}


const char * Hybrid_Road_Class_Name(HybridRoadClass road_class)
{
	if (road_class < 0 || road_class >= HYBRID_ROAD_CLASS_COUNT) return "";
	return ROAD_CLASS_NAMES[road_class];
}


const HybridRoadClassSpec & Hybrid_Road_Class_Spec(HybridRoadClass road_class)
{
	if (road_class < 0 || road_class >= HYBRID_ROAD_CLASS_COUNT) {
		return ROAD_CLASS_SPECS[HYBRID_ROAD_TRAIL_1];
	}
	return ROAD_CLASS_SPECS[road_class];
}


/***********************************************************************************************
 * Hybrid_Is_Legal_Biome_Pair -- Section 4.3                                                   *
 *                                                                                             *
 * Legal unordered pairs: temperate+snow, temperate+desert, temperate+tiberium_wasteland,      *
 * desert+tiberium_wasteland, snow+tiberium_wasteland.  Snow+desert is rejected even when both *
 * are named explicitly; a biome never pairs with itself.                                      *
 *=============================================================================================*/
bool Hybrid_Is_Legal_Biome_Pair(HybridBiome a,HybridBiome b)
{
	if (a < 0 || a >= HYBRID_BIOME_COUNT || b < 0 || b >= HYBRID_BIOME_COUNT) return false;
	if (a == b) return false;
	if ((a == HYBRID_BIOME_SNOW && b == HYBRID_BIOME_DESERT) || (a == HYBRID_BIOME_DESERT && b == HYBRID_BIOME_SNOW)) {
		return false;
	}
	return true;
}


int Hybrid_Route_Family_Count(int route_redundancy,int flank_lanes)
{
	int count = route_redundancy;
	if (1 + flank_lanes > count) count = 1 + flank_lanes;
	if (count < 2) count = 2;
	if (count > 3) count = 3;
	return count;
}


/***********************************************************************************************
 * Hybrid_Patch_Spacing -- Section 2.5                                                         *
 *                                                                                             *
 * The table names spacing for shore, road corridor, bridge/tunnel/overpass approach and base  *
 * entrance/production corridor.  Intersections use the road corridor row; Tiberium harvest    *
 * approaches are production corridors; cliff edges and canyon/ravine walls use the ridge/hill *
 * row (A/2), the finest steep-landform spacing the table gives.                               *
 *=============================================================================================*/
float Hybrid_Patch_Spacing(HybridPatchRegion region,float lane_width,float grid_spacing)
{
	const float w = lane_width;
	const float a = grid_spacing;

	switch (region) {
		case HYBRID_PATCH_RIVER_BANK:
		case HYBRID_PATCH_LAKE_SHORE:
			return Min_Float (w,a / 2.0f);

		case HYBRID_PATCH_ROAD_CORRIDOR:
		case HYBRID_PATCH_ROAD_INTERSECTION:
			return Min_Float (w / 2.0f,a / 3.0f);

		case HYBRID_PATCH_BRIDGE_APPROACH:
			return Min_Float (w / 3.0f,a / 4.0f);

		case HYBRID_PATCH_BASE_CORE_EXIT:
		case HYBRID_PATCH_TIBERIUM_HARVEST_APPROACH:
			return Min_Float (w / 2.0f,a / 3.0f);

		case HYBRID_PATCH_CLIFF_EDGE:
		case HYBRID_PATCH_CANYON_RAVINE_WALL:
			return a / 2.0f;

		default:
			return a;
	}
}


/***********************************************************************************************
 * HybridSemanticMatrix                                                                        *
 *=============================================================================================*/
static int Align_Up_16(int bytes)
{
	return (bytes + 15) & ~15;
}


HybridSemanticMatrix::HybridSemanticMatrix(void) :
	BaseElevationTier(nullptr),MacroHeightBias(nullptr),LandWaterIntent(nullptr),LandformMask(nullptr),
	ReservationMask(nullptr),TransportInfluence(nullptr),ResourceInfluence(nullptr),BiomeId(nullptr),
	RoughnessClass(nullptr),NoDecorationMask(nullptr),Flags(nullptr),
	Block(nullptr),BlockBytes(0),OwnsBlock(false),CountX(0),CountY(0),CellSize(0.0f),OriginX(0.0f),OriginY(0.0f)
{
}


HybridSemanticMatrix::~HybridSemanticMatrix(void)
{
	Clear ();
}


/*
**	One aligned block with every layer carved out of it in declaration order.  A zeroed block is
**	already the documented default for every layer, HYBRID_BIOME_TEMPERATE included, because that
**	biome is ordinal 0.  An arena that cannot fit the block fails the Resize; nothing grows.
*/
bool HybridSemanticMatrix::Resize(int count_x,int count_y,float cell_size,float origin_x,float origin_y,
											 HybridScratchArena * arena)
{
	Clear ();
	if ((count_x <= 0) || (count_y <= 0)) {
		return false;
	}

	const int count		= count_x * count_y;
	const int narrow		= Align_Up_16 (count * (int)sizeof (unsigned char));
	const int wide			= Align_Up_16 (count * (int)sizeof (float));
	const int bytes		= (7 * narrow) + (4 * wide);

	if (arena != nullptr) {
		Block			= (unsigned char *)arena->Allocate (bytes,16);
		OwnsBlock	= false;
	} else {
		Block			= new unsigned char [bytes];
		OwnsBlock	= true;
	}
	if (Block == nullptr) {
		OwnsBlock = false;
		return false;
	}

	::memset (Block,0,(size_t)bytes);
	BlockBytes = bytes;

	unsigned char * cursor = Block;
	BaseElevationTier	= (signed char *)cursor;			cursor += narrow;
	LandWaterIntent	= cursor;								cursor += narrow;
	LandformMask		= cursor;								cursor += narrow;
	ReservationMask	= cursor;								cursor += narrow;
	BiomeId				= cursor;								cursor += narrow;
	RoughnessClass		= cursor;								cursor += narrow;
	NoDecorationMask	= cursor;								cursor += narrow;
	MacroHeightBias	= (float *)cursor;					cursor += wide;
	TransportInfluence	= (float *)cursor;				cursor += wide;
	ResourceInfluence	= (float *)cursor;					cursor += wide;
	Flags					= (unsigned int *)cursor;

	CountX	= count_x;
	CountY	= count_y;
	CellSize	= cell_size;
	OriginX	= origin_x;
	OriginY	= origin_y;
	return true;
}


bool HybridSemanticMatrix::World_To_Cell(const Vector2 & world,int * cx,int * cy) const
{
	if ((CountX <= 0) || (CountY <= 0) || (CellSize <= 0.0f)) {
		return false;
	}

	const int x = (int)WWMath::Floor ((world.X - OriginX) / CellSize);
	const int y = (int)WWMath::Floor ((world.Y - OriginY) / CellSize);
	if (!Is_Valid_Cell (x,y)) {
		return false;
	}

	if (cx != nullptr) *cx = x;
	if (cy != nullptr) *cy = y;
	return true;
}


void HybridSemanticMatrix::Clear(void)
{
	if (OwnsBlock && (Block != nullptr)) {
		delete [] Block;
	}

	Block					= nullptr;
	BlockBytes			= 0;
	OwnsBlock			= false;
	BaseElevationTier	= nullptr;
	MacroHeightBias	= nullptr;
	LandWaterIntent	= nullptr;
	LandformMask		= nullptr;
	ReservationMask	= nullptr;
	TransportInfluence	= nullptr;
	ResourceInfluence	= nullptr;
	BiomeId				= nullptr;
	RoughnessClass		= nullptr;
	NoDecorationMask	= nullptr;
	Flags					= nullptr;

	CountX	= 0;
	CountY	= 0;
	CellSize	= 0.0f;
	OriginX	= 0.0f;
	OriginY	= 0.0f;
}
