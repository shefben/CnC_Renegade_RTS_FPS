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
 *                     $Archive:: /Commando/Code/WWPhys/heightfield.h                         $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	The height data itself, and nothing else.                                                   *
 *                                                                                             *
 *	Roadmap Section 17 is emphatic on one point: rendered terrain and terrain collision must    *
 *	derive from the same source data, and roads, bridges, water, placement queries, foliage     *
 *	and anything generated later must all refer to the same canonical world coordinates.        *
 *	This class is that source.  It is a regular grid of heights with an origin and a cell       *
 *	size, it knows how it is triangulated, and every answer anything else gives about the       *
 *	shape of the ground comes from here.                                                        *
 *                                                                                             *
 *	The triangulation is part of the contract, not an implementation detail.  A cell is two     *
 *	triangles split along the diagonal from its low corner to its high corner, and the height   *
 *	at a point is the height of the triangle that contains it -- planar, not bilinear.  A       *
 *	bilinear surface is not made of triangles, so a bilinear sample and a triangle-mesh         *
 *	collide differently, and a soldier would stand a few centimetres off the ground he is       *
 *	drawn on.  Section 17's rule is kept by both sides asking this class.                       *
 *                                                                                             *
 *	Heights come from memory.  There is no file format here and no loader, because Section 17   *
 *	requires that the terrain service accept height data from a generated source, and the       *
 *	simplest way to be sure it can is to have no other kind.                                    *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef HEIGHTFIELD_H
#define HEIGHTFIELD_H

#include "always.h"
#include "aabox.h"
#include "vector3.h"

class LineSegClass;


/*
**	A patch is the unit of rebuild.
**
**	Renderers and collision meshes are not rebuilt a vertex at a time, so the field tracks
**	which square blocks of cells have changed since anything last looked.  The min and max
**	height of a patch is kept alongside, because it is what makes a ray able to skip a patch
**	without touching a triangle in it, and what a far-terrain representation samples.
*/
class TerrainPatchClass
{
public:
	TerrainPatchClass(void) : MinHeight(0.0f), MaxHeight(0.0f), Dirty(true) { }

	float		MinHeight;
	float		MaxHeight;
	bool		Dirty;

	bool		operator == (const TerrainPatchClass & that) const
	{
		return (MinHeight == that.MinHeight) && (MaxHeight == that.MaxHeight) && (Dirty == that.Dirty);
	}
	bool		operator != (const TerrainPatchClass & that) const	{ return !(*this == that); }
};


class HeightfieldClass
{
public:

	enum {
		//	Cells to a patch side.  Sixteen cells is 289 vertices, which is a sensible amount of
		//	geometry to throw away and rebuild when a road is graded through it.
		PATCH_CELLS		= 16,
	};

	HeightfieldClass(void);
	~HeightfieldClass(void);

	/*
	**	Creation.  Vertex counts, not cell counts: a field with two vertices a side has one
	**	cell.  Both must be at least two, and the cell size must be positive.
	*/
	bool					Create(int vertex_count_x,int vertex_count_y,float cell_size,const Vector3 & origin);
	void					Destroy(void);
	bool					Is_Valid(void) const						{ return (Heights != nullptr); }

	/*
	**	Shape.
	*/
	int					Get_Vertex_Count_X(void) const		{ return VertexCountX; }
	int					Get_Vertex_Count_Y(void) const		{ return VertexCountY; }
	int					Get_Cell_Count_X(void) const			{ return VertexCountX - 1; }
	int					Get_Cell_Count_Y(void) const			{ return VertexCountY - 1; }
	float					Get_Cell_Size(void) const				{ return CellSize; }
	const Vector3 &	Get_Origin(void) const					{ return Origin; }
	const AABoxClass &Get_Bounds(void) const					{ return Bounds; }

	/*
	**	Heights by grid index.  Out of range reads return the edge value rather than asserting,
	**	because clamping the border is what every caller that walks off the edge wants.
	*/
	float					Get_Height(int ix,int iy) const;
	void					Set_Height(int ix,int iy,float height);

	/*
	**	Heights in bulk, from wherever they were generated.  The array is row major, x fastest,
	**	and must hold exactly Get_Vertex_Count_X() * Get_Vertex_Count_Y() values.
	*/
	bool					Set_Heights(const float * heights,int count);

	/*
	**	Raise or lower a circular region, with a smooth falloff to nothing at the radius.  This
	**	is the shape a generator, a road grade and a river cut are all built out of.
	*/
	void					Modify_Height_Region(const Vector3 & center,float radius,float delta);

	/*
	**	Flatten a circular region towards a target height.  Blend is 1 for "exactly this
	**	height", less for a partial pull, and the same falloff applies outward from the radius.
	*/
	void					Blend_Height_Region(const Vector3 & center,float radius,float target_height,float blend);

	/*
	**	Sampling.  World x and y in, ground in out.  False when the point is outside the field,
	**	in which case nothing is written -- a caller over the void needs to know it is over the
	**	void, not to be handed the nearest edge and told it is standing on it.
	*/
	bool					Sample_Height(float x,float y,float * height_out) const;
	bool					Sample_Normal(float x,float y,Vector3 * normal_out) const;
	bool					Sample_Slope(float x,float y,float * slope_radians_out) const;

	/*
	**	Cast a segment at the ground.  Walks the cells the segment crosses in order and stops
	**	at the first triangle it hits, so the fraction returned is the nearest one.
	*/
	bool					Cast_Ray(const LineSegClass & ray,float * fraction_out,Vector3 * normal_out) const;

	/*
	**	Patches.
	*/
	int					Get_Patch_Count_X(void) const			{ return PatchCountX; }
	int					Get_Patch_Count_Y(void) const			{ return PatchCountY; }
	bool					Get_Patch(int px,int py,TerrainPatchClass * patch_out) const;
	bool					Get_Patch_Bounds(int px,int py,AABoxClass * bounds_out) const;
	void					Invalidate_Patch(int px,int py);
	void					Invalidate_All_Patches(void);
	void					Validate_Patch(int px,int py);
	bool					Is_Patch_Dirty(int px,int py) const;

	/*
	**	The corner vertices of a cell, and the two triangles it is made of.  Collision mesh
	**	building and renderers use these so that there is one triangulation in the program.
	*/
	bool					Get_Cell_Triangles(int cx,int cy,Vector3 * verts_out) const;

	/*
	**	The shading normal at a grid vertex.  Sample_Normal answers for a point inside a
	**	triangle and gives that triangle's face normal; a vertex is where several triangles
	**	meet and none of their normals is the one a renderer wants, so this takes the height
	**	gradient across the vertex's neighbours instead.
	**
	**	Neighbours are clamped at the border, which means the patches either side of a seam
	**	compute the same normal for the vertex they share -- lighting that disagreed across a
	**	seam would draw the seam.
	*/
	Vector3				Compute_Vertex_Normal(int ix,int iy) const;

private:

	void					Update_Bounds(void);
	void					Update_Patch_Extents(int px,int py);
	void					Invalidate_Patches_Touching(int ix,int iy);
	bool					Cell_From_World(float x,float y,int * cx_out,int * cy_out) const;
	bool					Sample_Cell(int cx,int cy,float x,float y,float * height_out,Vector3 * normal_out) const;

	float *				Heights;
	int					VertexCountX;
	int					VertexCountY;
	float					CellSize;
	Vector3				Origin;
	AABoxClass			Bounds;

	TerrainPatchClass *	Patches;
	int					PatchCountX;
	int					PatchCountY;

	// Not implemented.
	HeightfieldClass(const HeightfieldClass & that);
	HeightfieldClass & operator = (const HeightfieldClass & that);
};

#endif	// HEIGHTFIELD_H
