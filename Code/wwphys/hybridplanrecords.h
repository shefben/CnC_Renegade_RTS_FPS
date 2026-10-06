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
 *                     $Archive:: /Commando/Code/wwphys/hybridplanrecords.h                   $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Hybrid world generator rewrite, item I1 -- the data records every later hybrid phase        *
 * reads and writes (Procedural_World_Generator_Rewrite_Plan_FINAL_AUDITED.md Sections 2, 4,   *
 * 6 and 7).  No generation happens here; these are the vocabulary and the containers:         *
 *                                                                                             *
 *   Representation A  strategic graph       -- HybridStrategicSite / HybridStrategicEdge      *
 *   Representation B  semantic matrices     -- HybridSemanticMatrix (SoA layers + bit flags)  *
 *   Representation C  vector features       -- HybridVectorFeature (StableId-keyed)           *
 *   Section 2.4       interaction records   -- HybridInteractionRecord                        *
 *   Section 2.5       refinement patches    -- HybridRefinementPatch + spacing rule           *
 *   Section 4.1-4.3   biome selectors and legal mixed pairs                                   *
 *   Section 6         archetypes in stable order                                              *
 *   Section 7         road classes and their cross-section table (multiples of W)             *
 *                                                                                             *
 * Old D-003 MapPlanGenerator records coexist with these until the plan's item 22 removes      *
 * them; nothing here reads or replaces them.                                                  *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef HYBRIDPLANRECORDS_H
#define HYBRIDPLANRECORDS_H

#include "always.h"
#include "vector.h"
#include "vector2.h"
#include "vector3.h"


//	Section 16.3's per-attempt arena, which owns the semantic matrices.  Declared, not included:
//	hybridplanning.h includes this file.
class HybridScratchArena;


/*
**	Section 2.1 -- strategic graph node roles and edge intents.
*/
enum HybridSiteRole
{
	HYBRID_SITE_GDI_BASE = 0,
	HYBRID_SITE_NOD_BASE,
	HYBRID_SITE_FRONT_CENTER,
	HYBRID_SITE_HOME_TIBERIUM_GDI,
	HYBRID_SITE_HOME_TIBERIUM_NOD,
	HYBRID_SITE_CONTESTED_TIBERIUM,
	HYBRID_SITE_REMOTE_TIBERIUM,
	HYBRID_SITE_ROUTE_WAYPOINT,
	HYBRID_SITE_ROAD_JUNCTION,
	HYBRID_SITE_CROSSING_OBLIGATION,
	HYBRID_SITE_ROLE_COUNT
};


enum HybridEdgeIntent
{
	HYBRID_EDGE_MAIN_ROUTE = 0,
	HYBRID_EDGE_FLANK_ROUTE,
	HYBRID_EDGE_RESOURCE_ROUTE,
	HYBRID_EDGE_OPTIONAL_CROSS_LINK,
	HYBRID_EDGE_INFANTRY_SHORTCUT,
	HYBRID_EDGE_INTENT_COUNT
};


/*
**	Section 2.2 -- semantic bit flags, one unsigned int per cell.
*/
enum HybridSemanticFlag
{
	HYBRID_FLAG_LAND								= 1 << 0,
	HYBRID_FLAG_WATER_BASIN						= 1 << 1,
	HYBRID_FLAG_COAST								= 1 << 2,
	HYBRID_FLAG_RIDGE								= 1 << 3,
	HYBRID_FLAG_PLATEAU							= 1 << 4,
	HYBRID_FLAG_HILL								= 1 << 5,
	HYBRID_FLAG_CLIFF_BAND						= 1 << 6,
	HYBRID_FLAG_CANYON_BAND						= 1 << 7,
	HYBRID_FLAG_RAVINE_BAND						= 1 << 8,
	HYBRID_FLAG_BASE_CORE						= 1 << 9,
	HYBRID_FLAG_BASE_EXPANSION					= 1 << 10,
	HYBRID_FLAG_TIBERIUM_RESERVE				= 1 << 11,
	HYBRID_FLAG_MAJOR_TRANSPORT_CORRIDOR	= 1 << 12,
	HYBRID_FLAG_SECONDARY_TRANSPORT_CORRIDOR	= 1 << 13,
	HYBRID_FLAG_CROSSING_WINDOW				= 1 << 14,
	HYBRID_FLAG_RAMP_PASS_WINDOW				= 1 << 15,
	HYBRID_FLAG_NO_DECORATION					= 1 << 16,

	/*
	**	Section 2.2's list is "flags include", not a closed set.  These three are what the coarse
	**	matrix cannot express with the listed bits:
	**
	**	  PLACEABLE				`WorldBoundsService::Can_Place_In_Region` answered yes for this cell,
	**								which is a narrower question than LAND -- the perimeter cliff band is
	**								playable but never buildable.
	**	  BASIN					the Section-8 Phase-4 macro "basin band", which `plateau_basin` needs and
	**								which is not WATER_BASIN: a dry basin floor is not water intent.
	**	  NO_IMMUTABLE_BARRIER	the dedicated Section-6.9 layer: Phase 5 may not put a cliff/canyon/
	**								ravine wall through these cells and Phase 6 may not put lake/coast
	**								water through them.
	*/
	HYBRID_FLAG_PLACEABLE						= 1 << 17,
	HYBRID_FLAG_BASIN								= 1 << 18,
	HYBRID_FLAG_NO_IMMUTABLE_BARRIER			= 1 << 19,
};


/*
**	Section 2.3 -- vector feature types.
*/
enum HybridVectorType
{
	HYBRID_VECTOR_RIVER_SPLINE = 0,
	HYBRID_VECTOR_ROAD_SPLINE,
	HYBRID_VECTOR_HIGHWAY_SPLINE,
	HYBRID_VECTOR_CANYON_SPLINE,
	HYBRID_VECTOR_RAVINE_SPLINE,
	HYBRID_VECTOR_CLIFF_SPLINE,
	HYBRID_VECTOR_RIDGE_SPLINE,
	HYBRID_VECTOR_LAKE_POLYGON,
	HYBRID_VECTOR_COAST_POLYGON,
	HYBRID_VECTOR_TIBERIUM_POLYGON,
	HYBRID_VECTOR_BASE_RESERVE_POLYGON,
	HYBRID_VECTOR_CROSSING_WINDOW,
	HYBRID_VECTOR_TYPE_COUNT
};


/*
**	Section 2.4 -- interaction kinds and the method that resolved one.
*/
enum HybridInteractionType
{
	HYBRID_INTERACTION_ROAD_X_RIVER = 0,
	HYBRID_INTERACTION_ROAD_X_CANYON,
	HYBRID_INTERACTION_ROAD_X_CLIFF,
	HYBRID_INTERACTION_ROAD_X_ROAD,
	HYBRID_INTERACTION_RIVER_X_BASIN,
	HYBRID_INTERACTION_ROAD_X_BASE,
	HYBRID_INTERACTION_ROAD_X_TIBERIUM,
	HYBRID_INTERACTION_TYPE_COUNT
};


enum HybridInteractionResolution
{
	HYBRID_RESOLUTION_UNRESOLVED = 0,
	HYBRID_RESOLUTION_BRIDGE,
	HYBRID_RESOLUTION_FORD,
	HYBRID_RESOLUTION_PASS_WINDOW,
	HYBRID_RESOLUTION_RAMP,
	HYBRID_RESOLUTION_AT_GRADE_JUNCTION,
	HYBRID_RESOLUTION_OVERPASS,
	HYBRID_RESOLUTION_REROUTE,
};


/*
**	Section 2.5 -- refinement patch regions.  Spacing per region is Hybrid_Patch_Spacing.
*/
enum HybridPatchRegion
{
	HYBRID_PATCH_ROAD_CORRIDOR = 0,
	HYBRID_PATCH_BRIDGE_APPROACH,
	HYBRID_PATCH_ROAD_INTERSECTION,
	HYBRID_PATCH_RIVER_BANK,
	HYBRID_PATCH_LAKE_SHORE,
	HYBRID_PATCH_CLIFF_EDGE,
	HYBRID_PATCH_CANYON_RAVINE_WALL,
	HYBRID_PATCH_BASE_CORE_EXIT,
	HYBRID_PATCH_TIBERIUM_HARVEST_APPROACH,
	HYBRID_PATCH_REGION_COUNT
};


/*
**	Section 6 -- archetypes in their stable order.  The ordinal is part of the deterministic
**	selection domain; never reorder.
*/
enum HybridArchetype
{
	HYBRID_ARCHETYPE_RIVER_VALLEY = 0,
	HYBRID_ARCHETYPE_TWIN_LAKES,
	HYBRID_ARCHETYPE_CANYON_CROSSROADS,
	HYBRID_ARCHETYPE_HIGHLAND_PASS,
	HYBRID_ARCHETYPE_COASTAL_PENINSULA,
	HYBRID_ARCHETYPE_PLATEAU_BASIN,
	HYBRID_ARCHETYPE_RAVINE_NETWORK,
	HYBRID_ARCHETYPE_OPEN_FRONTIER,
	HYBRID_ARCHETYPE_COUNT,
	HYBRID_ARCHETYPE_AUTO = HYBRID_ARCHETYPE_COUNT,		// profile selector only
};


/*
**	Sections 4.1-4.3 -- biome selectors.
*/
enum HybridBiomeMode
{
	HYBRID_BIOME_MODE_AUTO = 0,
	HYBRID_BIOME_MODE_SINGLE,
	HYBRID_BIOME_MODE_MIXED,
	HYBRID_BIOME_MODE_COUNT
};


enum HybridBiome
{
	HYBRID_BIOME_TEMPERATE = 0,
	HYBRID_BIOME_DESERT,
	HYBRID_BIOME_SNOW,
	HYBRID_BIOME_TIBERIUM_WASTELAND,
	HYBRID_BIOME_COUNT,
	HYBRID_BIOME_AUTO = HYBRID_BIOME_COUNT,				// profile selector only
};


/*
**	Section 7 -- road classes and physical road tiers.  `HIGHWAY_RAMP` (Section 12.6) is appended after the
**	five strategic classes, so their ordinals, every class-indexed array and the Phase-6 `approach_classes`
**	bits keep their values; a ramp is never A*-routed, never a family's class and never binds a window.
*/
enum HybridRoadClass
{
	HYBRID_ROAD_HIGHWAY_4 = 0,
	HYBRID_ROAD_PRIMARY_2,
	HYBRID_ROAD_SECONDARY_2,
	HYBRID_ROAD_SERVICE_DIRT_2,
	HYBRID_ROAD_TRAIL_1,
	HYBRID_ROAD_HIGHWAY_RAMP,
	HYBRID_ROAD_CLASS_COUNT
};

//	The classes a road is routed, searched and bridged as: every class but the constructed HIGHWAY_RAMP.
inline bool Hybrid_Road_Class_Is_Routed(int road_class)
{
	return (road_class >= HYBRID_ROAD_HIGHWAY_4) && (road_class <= HYBRID_ROAD_TRAIL_1);
}


//	Section 12.6: an access ramp's logical role.  Ordinals are part of the checksum: never reorder.
enum HybridRampRole
{
	HYBRID_RAMP_ROLE_NONE = 0,
	HYBRID_RAMP_ROLE_ON,						// ON_RAMP: logical travel enters HIGHWAY_4
	HYBRID_RAMP_ROLE_OFF,					// OFF_RAMP: logical travel leaves HIGHWAY_4
	HYBRID_RAMP_ROLE_COUNT
};


enum HybridPhysicalRoadTier
{
	HYBRID_ROAD_TIER_HIGHWAY = 0,
	HYBRID_ROAD_TIER_PAVED,
	HYBRID_ROAD_TIER_DIRT,
};


/*
**	One row of the Section 7 cross-section table.  Widths and the turn radius are multiples of
**	the lane width W (Section 0.5); resolve them with the canonical W, never a local literal.
*/
struct HybridRoadClassSpec
{
	HybridRoadClass			Class;
	int							Lanes;
	float							ShoulderW;				// the inner shoulder: left of the logical travel direction
	float							MedianW;
	float							MaxGradeDegrees;
	float							MaxCrossSlopeDegrees;
	float							MinTurnRadiusW;
	HybridPhysicalRoadTier	PhysicalTier;
	float							OuterShoulderW;		// the outer shoulder; equal to ShoulderW except HIGHWAY_RAMP

	float	Roadbed_Width_W(void) const		{ return (float)Lanes + ShoulderW + OuterShoulderW + MedianW; }
};


/*
**	Name tables and legality rules.  Name lookups accept exactly the plan's spellings; "auto"
**	parses to the *_AUTO selector where one exists.
*/
const char *						Hybrid_Archetype_Name(HybridArchetype archetype);
bool									Hybrid_Archetype_From_Name(const char * name,HybridArchetype * out);
const char *						Hybrid_Biome_Name(HybridBiome biome);
bool									Hybrid_Biome_From_Name(const char * name,HybridBiome * out);
const char *						Hybrid_Biome_Mode_Name(HybridBiomeMode mode);
bool									Hybrid_Biome_Mode_From_Name(const char * name,HybridBiomeMode * out);
const char *						Hybrid_Road_Class_Name(HybridRoadClass road_class);
const HybridRoadClassSpec &	Hybrid_Road_Class_Spec(HybridRoadClass road_class);

//	Section 4.3 -- unordered legal mixed pairs.  Identical biomes and snow + desert are illegal.
bool									Hybrid_Is_Legal_Biome_Pair(HybridBiome a,HybridBiome b);

//	Section 4 -- route_family_count = clamp(max(route_redundancy, 1 + flank_lanes), 2, 3).
int									Hybrid_Route_Family_Count(int route_redundancy,int flank_lanes);

//	Section 2.5 -- sample spacing for a refinement patch region given W and the ordinary grid
//	spacing A.  Ridge/hill detail (A/2) is ordinary-grid detail, not a patch region.
float									Hybrid_Patch_Spacing(HybridPatchRegion region,float lane_width,float grid_spacing);


/*
**	Representation A -- strategic graph records.  StableId is assigned by the owning phase from
**	its seed domain and sort order; it never depends on container addresses.
*/
struct HybridStrategicSite
{
	HybridStrategicSite(void) : StableId(0),Role(HYBRID_SITE_ROUTE_WAYPOINT),Team(-1),Position(0.0f,0.0f),Radius(0.0f),RouteFamily(-1) { }

	//	DynamicVectorClass<T> (wwlib/vector.h) requires T to support == and !=.  StableId is the key.
	bool	operator == (const HybridStrategicSite & that) const	{ return StableId == that.StableId; }
	bool	operator != (const HybridStrategicSite & that) const	{ return !(*this == that); }

	unsigned int		StableId;
	HybridSiteRole		Role;
	int					Team;				// 0 GDI, 1 Nod, -1 neutral
	Vector2				Position;		// world
	float					Radius;			// world units; 0 for point sites
	int					RouteFamily;	// heavy route-family waypoints: 0..route_family_count-1, else -1
};


struct HybridStrategicEdge
{
	HybridStrategicEdge(void) : StableId(0),Intent(HYBRID_EDGE_MAIN_ROUTE),SiteA(0),SiteB(0),RouteFamily(-1),
										RoadClass(HYBRID_ROAD_CLASS_COUNT) { }

	bool	operator == (const HybridStrategicEdge & that) const	{ return StableId == that.StableId; }
	bool	operator != (const HybridStrategicEdge & that) const	{ return !(*this == that); }

	unsigned int		StableId;
	HybridEdgeIntent	Intent;
	unsigned int		SiteA;			// HybridStrategicSite::StableId
	unsigned int		SiteB;
	int					RouteFamily;	// 0..route_family_count-1 for base-to-base routes, else -1

	/*
	**	Set only where the plan fixes the class at intent time -- Phase 3's `SERVICE_DIRT_2` branch
	**	to a non-home field.  HYBRID_ROAD_CLASS_COUNT means "Phase 9 assigns it", which is the
	**	normal case: Section 9.5 owns heavy/highway class assignment.
	*/
	HybridRoadClass	RoadClass;
};


/*
**	Representation B -- semantic matrices, structure-of-arrays at the plan grid resolution.
**	Every layer has CountX * CountY entries, row-major `(y,x)`.
**
**	Section 16.3 puts these layers in the per-attempt scratch arena, so Resize takes one and carves
**	every layer out of a single aligned block.  With no arena -- a self-check, or an offline tool --
**	the matrix owns one equivalent heap block instead, which is the same layout with a different
**	owner.  The block is never grown: a Resize that does not fit the arena fails, and the caller
**	reports it as the budget failure it is rather than silently paging.
*/
class HybridSemanticMatrix
{
public:
	HybridSemanticMatrix(void);
	~HybridSemanticMatrix(void);

	bool	Resize(int count_x,int count_y,float cell_size,float origin_x,float origin_y,HybridScratchArena * arena = nullptr);
	void	Clear(void);

	int	Get_Count_X(void) const								{ return CountX; }
	int	Get_Count_Y(void) const								{ return CountY; }
	int	Get_Cell_Count(void) const							{ return CountX * CountY; }
	int	Get_Layer_Bytes(void) const						{ return BlockBytes; }
	float	Get_Cell_Size(void) const							{ return CellSize; }
	float	Get_Origin_X(void) const							{ return OriginX; }
	float	Get_Origin_Y(void) const							{ return OriginY; }
	bool	Is_Valid_Cell(int cx,int cy) const				{ return cx >= 0 && cy >= 0 && cx < CountX && cy < CountY; }
	int	Index(int cx,int cy) const							{ return cy * CountX + cx; }

	//	The centre of a cell, which is the point every Phase-4 rasterization rule is measured at.
	Vector2	Cell_Center(int cx,int cy) const
	{
		return Vector2 (OriginX + (((float)cx + 0.5f) * CellSize),OriginY + (((float)cy + 0.5f) * CellSize));
	}

	//	The cell a world point falls in.  False when the point is outside the matrix.
	bool	World_To_Cell(const Vector2 & world,int * cx,int * cy) const;

	bool	Has_Flag(int cx,int cy,unsigned int flag) const	{ return (Flags[Index (cx,cy)] & flag) != 0; }
	void	Set_Flag(int cx,int cy,unsigned int flag)		{ Flags[Index (cx,cy)] |= flag; }
	void	Clear_Flag(int cx,int cy,unsigned int flag)		{ Flags[Index (cx,cy)] &= ~flag; }

	//	Section 2.2 layers, each CountX * CountY entries into the shared block.
	signed char *		BaseElevationTier;
	float *				MacroHeightBias;
	unsigned char *	LandWaterIntent;		// 0 land, 1 water
	unsigned char *	LandformMask;			// Phase 5 owns this; Phase 4 leaves it clear
	unsigned char *	ReservationMask;		// winning HybridReservationPriority + 1, 0 when unreserved
	float *				TransportInfluence;	// 0..1
	float *				ResourceInfluence;	// 0..1
	unsigned char *	BiomeId;					// HybridBiome
	unsigned char *	RoughnessClass;		// Phase 5 owns this; Phase 4 leaves it clear
	unsigned char *	NoDecorationMask;
	unsigned int *		Flags;					// HybridSemanticFlag bits

private:
	//	One owner per block, so the matrix is deliberately not copyable.
	HybridSemanticMatrix(const HybridSemanticMatrix & that);
	HybridSemanticMatrix & operator = (const HybridSemanticMatrix & that);

	unsigned char *	Block;
	int					BlockBytes;
	bool					OwnsBlock;			// false when the block came from the attempt arena
	int					CountX;
	int					CountY;
	float					CellSize;
	float					OriginX;
	float					OriginY;
};


/*
**	Representation C -- vector features.  Geometry is control points (splines) or a closed ring
**	(polygons); Width/Depth are the feature's cross-section parameters in world units.
*/
struct HybridVectorFeature
{
	HybridVectorFeature(void) :
		StableId(0),Type(HYBRID_VECTOR_ROAD_SPLINE),OwnerSiteId(0),SourceEdgeId(0),
		BoundsMin(0.0f,0.0f),BoundsMax(0.0f,0.0f),Priority(0),Required(false),
		Width(0.0f),Depth(0.0f),RoadClass(HYBRID_ROAD_CLASS_COUNT) { }

	bool	operator == (const HybridVectorFeature & that) const	{ return StableId == that.StableId; }
	bool	operator != (const HybridVectorFeature & that) const	{ return !(*this == that); }

	unsigned int						StableId;
	HybridVectorType					Type;
	unsigned int						OwnerSiteId;		// semantic owner, HybridStrategicSite::StableId or 0
	unsigned int						SourceEdgeId;		// HybridStrategicEdge::StableId or 0
	Vector2								BoundsMin;
	Vector2								BoundsMax;
	int									Priority;
	bool									Required;
	float									Width;
	float									Depth;
	HybridRoadClass					RoadClass;			// road/highway splines only, else CLASS_COUNT
	DynamicVectorClass<Vector3>	Points;
};


/*
**	Section 2.4 -- one detected interaction between two vector features.
*/
struct HybridInteractionRecord
{
	HybridInteractionRecord(void) :
		StableId(0),Type(HYBRID_INTERACTION_ROAD_X_ROAD),FeatureA(0),FeatureB(0),
		Position(0.0f,0.0f),StationA0(0.0f),StationA1(0.0f),Resolution(HYBRID_RESOLUTION_UNRESOLVED) { }

	bool	operator == (const HybridInteractionRecord & that) const	{ return StableId == that.StableId; }
	bool	operator != (const HybridInteractionRecord & that) const	{ return !(*this == that); }

	unsigned int						StableId;
	HybridInteractionType			Type;
	unsigned int						FeatureA;			// HybridVectorFeature::StableId (the road for ROAD_X_*)
	unsigned int						FeatureB;
	Vector2								Position;			// world, crossing centre
	float									StationA0;			// arc-length interval along FeatureA
	float									StationA1;
	HybridInteractionResolution	Resolution;
};


/*
**	Section 2.5 -- a temporary refinement patch.  Released once its deformation is committed.
*/
struct HybridRefinementPatch
{
	HybridRefinementPatch(void) :
		StableId(0),Region(HYBRID_PATCH_ROAD_CORRIDOR),OwnerFeatureId(0),
		BoundsMin(0.0f,0.0f),BoundsMax(0.0f,0.0f),Spacing(0.0f) { }

	bool	operator == (const HybridRefinementPatch & that) const	{ return StableId == that.StableId; }
	bool	operator != (const HybridRefinementPatch & that) const	{ return !(*this == that); }

	unsigned int		StableId;
	HybridPatchRegion	Region;
	unsigned int		OwnerFeatureId;	// HybridVectorFeature::StableId
	Vector2				BoundsMin;
	Vector2				BoundsMax;
	float					Spacing;				// Hybrid_Patch_Spacing
};


#endif // HYBRIDPLANRECORDS_H
