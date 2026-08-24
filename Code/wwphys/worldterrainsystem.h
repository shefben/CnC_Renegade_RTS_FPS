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
 *                     $Archive:: /Commando/Code/WWPhys/worldterrainsystem.h                  $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	One terrain service, not several terrain engines.                                           *
 *                                                                                             *
 *	Roadmap Section 17 takes the Zero Hour donor's terrain responsibilities -- BaseHeightMap    *
 *	for ownership and lifetime, HeightMap and FlatHeightMap as rendering strategies beneath     *
 *	it, TerrainVisual and TerrainLogic as the split between drawing and asking -- and says      *
 *	plainly not to recreate them as several independent engines.  The canonical OpenW3D form    *
 *	is one WorldTerrainSystem that owns the terrain data and answers questions about it, with   *
 *	near, detail and far rendering as components below it rather than as rivals beside it.      *
 *                                                                                             *
 *	So this class owns a HeightfieldClass and nothing else owns one.  Collision, placement,     *
 *	roads, bridges, water, foliage and anything generated later ask here, which is how          *
 *	Section 17's requirement that they all refer to the same canonical world coordinates and    *
 *	the same source data is kept: there is only one copy to refer to.                           *
 *                                                                                             *
 *	Terrain is additive.  A level with no heightfield is every Renegade level that exists, and  *
 *	Has_Terrain is false for all of them: the queries return false, callers fall through to     *
 *	the W3D geometry they already used, and nothing about a stock level changes.  Section 17's  *
 *	world model is heightfield terrain *and* arbitrary static W3D meshes, not instead of.       *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef WORLDTERRAINSYSTEM_H
#define WORLDTERRAINSYSTEM_H

#include "always.h"
#include "heightfield.h"
#include "vector.h"
#include "vector3.h"

class FrustumClass;
class LineSegClass;


/*
**	What the ground is made of at a point.
**
**	Section 18 is the terrain texture system, and it is what will decide these from height,
**	slope, moisture, masks and the rest.  Until it lands there is one answer, and the enum
**	exists so that callers written now ask the question in the form Section 18 will answer.
*/
enum TerrainMaterialType
{
	TERRAIN_MATERIAL_DEFAULT		= 0,
	TERRAIN_MATERIAL_COUNT
};


/*
**	The answer to "can something stand here".
**
**	A single sample is not enough to place a building on: the ground under a footprint can be
**	flat where the middle was measured and a cliff at one corner.  So the query measures the
**	whole disc and reports the range, and the caller decides what it can live with.
*/
class TerrainPlacementInfoClass
{
public:
	TerrainPlacementInfoClass(void) :
		AverageHeight(0.0f), MinHeight(0.0f), MaxHeight(0.0f),
		MaxSlope(0.0f), AverageNormal(0.0f,0.0f,1.0f), IsOnTerrain(false) { }

	float			AverageHeight;
	float			MinHeight;
	float			MaxHeight;
	float			MaxSlope;			// radians
	Vector3		AverageNormal;
	bool			IsOnTerrain;		// false when any of the disc was off the field
};


class WorldTerrainSystem
{
public:

	static void			Init(void);
	static void			Shutdown(void);

	/*
	**	Creating terrain.  Heights come from memory, always: Section 17 requires the service to
	**	accept a generated source, and there is no other kind here to fall back on.
	*/
	static bool			Create_Terrain(int vertex_count_x,int vertex_count_y,float cell_size,const Vector3 & origin);
	static bool			Set_Heights(const float * heights,int count);
	static void			Destroy_Terrain(void);
	static bool			Has_Terrain(void);

	static HeightfieldClass *	Peek_Heightfield(void);

	/*
	**	Queries.  Every one returns false when there is no terrain or the point is off it, and
	**	writes nothing in that case.
	*/
	static bool			Sample_Height(float x,float y,float * height_out);
	static bool			Sample_Normal(float x,float y,Vector3 * normal_out);
	static bool			Sample_Slope(float x,float y,float * slope_radians_out);
	static bool			Ray_Intersect_Terrain(const LineSegClass & ray,float * fraction_out,Vector3 * normal_out);
	static bool			Get_Material(float x,float y,TerrainMaterialType * material_out);
	static bool			Get_Bounds(AABoxClass * bounds_out);

	/*
	**	Patches: the unit a renderer or a collision mesh is rebuilt in.
	*/
	static bool			Get_Patch(int px,int py,TerrainPatchClass * patch_out);
	static bool			Get_Patch_Bounds(int px,int py,AABoxClass * bounds_out);
	static void			Invalidate_Patch(int px,int py);
	static int			Get_Patch_Count_X(void);
	static int			Get_Patch_Count_Y(void);

	/*
	**	Shaping the ground.  ModifyHeightRegion is the primitive; a road grade and a river cut
	**	are polylines walked with it, which is why they are here rather than in the road and
	**	water systems -- the ground has one owner, and they ask it to change.
	*/
	static void			Modify_Height_Region(const Vector3 & center,float radius,float delta);
	static void			Apply_Road_Grade(const Vector3 * points,int count,float width,float blend);
	static void			Apply_River_Cut(const Vector3 * points,int count,float width,float depth);

	/*
	**	Is this footprint flat enough and on the ground?
	*/
	static bool			Query_Placement_Surface(const Vector3 & center,float radius,TerrainPlacementInfoClass * info_out);

	/*
	**	Rendering support.  Which patches a frustum can see, and how much detail one wants at a
	**	distance.  The renderer that consumes these arrives with the terrain pipelines; the
	**	selection is here because it is a question about terrain data, not about a device.
	*/
	enum {
		LOD_NEAR				= 0,
		LOD_MEDIUM			= 1,
		LOD_FAR				= 2,
		LOD_COUNT			= 3,
	};

	static int			Select_Render_LOD(int px,int py,const Vector3 & viewer_position);
	static void			Get_Visible_Terrain_Patches(const FrustumClass & frustum,DynamicVectorClass<int> & patch_indices);

	/*
	**	Collision, and the far/background layer.  Both are declared because Section 17 names
	**	them and callers should be written against the final shape; both refuse for now and say
	**	why once.  Collision needs runtime mesh building against WWPhys, and the far layer is
	**	Section 34's own phase.
	*/
	static bool			Build_Collision(void);
	static bool			Build_Far_Terrain_Representation(void);
	static void			Invalidate_Far_Terrain_Region(const AABoxClass & region);

private:

	static HeightfieldClass *	Heightfield;
	static bool						ReportedNoCollision;
	static bool						ReportedNoFarTerrain;
};

#endif	// WORLDTERRAINSYSTEM_H
