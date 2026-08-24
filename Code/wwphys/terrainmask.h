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
 *                     $Archive:: /Commando/Code/WWPhys/terrainmask.h                         $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	One grid of numbers over the ground, and what everything that is not shape is made of.      *
 *                                                                                             *
 *	Roadmap Section 18 selects terrain materials from height, slope, curvature, biome,          *
 *	moisture, water distance, and the road, river, city, Tiberium, cliff and manual override    *
 *	masks.  The first three are shape and come from the heightfield.  Every one of the others   *
 *	is the same thing wearing a different name: a number per grid point that something wrote    *
 *	there.  So there is one class for all of them.                                              *
 *                                                                                             *
 *	That is not a simplification for its own sake.  Section 18's acceptance is that a           *
 *	generated heightfield can obtain coherent materials entirely from runtime data and masks,   *
 *	and the surest way to be certain no hidden file is involved is for the only way to say      *
 *	anything about the ground to be writing it into one of these.                               *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef TERRAINMASK_H
#define TERRAINMASK_H

#include "always.h"
#include "vector3.h"


class TerrainMaskClass
{
public:

	TerrainMaskClass(void);
	~TerrainMaskClass(void);

	/*
	**	A mask is the same shape as the heightfield it describes: one value per grid vertex, in
	**	the same row-major order, so that a layer rule can ask about height and about the road
	**	at the same point without converting between two ideas of where a point is.
	*/
	bool					Create(int vertex_count_x,int vertex_count_y,float cell_size,const Vector3 & origin);
	void					Destroy(void);
	bool					Is_Valid(void) const					{ return (Values != nullptr); }

	int					Get_Vertex_Count_X(void) const		{ return VertexCountX; }
	int					Get_Vertex_Count_Y(void) const		{ return VertexCountY; }
	float					Get_Cell_Size(void) const				{ return CellSize; }
	const Vector3 &	Get_Origin(void) const					{ return Origin; }

	/*
	**	Values by grid index.  Out of range reads clamp to the border, the same as the
	**	heightfield, because a rule evaluated at the edge of the world wants the edge value and
	**	not an assertion.
	*/
	float					Get(int ix,int iy) const;
	void					Set(int ix,int iy,float value);
	void					Clear(float value = 0.0f);

	/*
	**	Values in bulk, from wherever they were generated.
	*/
	bool					Set_Values(const float * values,int count);

	/*
	**	Sampling in world coordinates, bilinear between grid points.  Unlike the heightfield
	**	this does interpolate across the whole cell rather than within a triangle: a mask is
	**	not geometry, nothing collides with it, and a road that faded in two different ways on
	**	either side of a diagonal would show the diagonal.
	*/
	bool					Sample(float x,float y,float * value_out) const;

	/*
	**	Writing into a mask.  A disc with a soft edge is the primitive; a polyline stamped along
	**	its length is how a road, a river or a city limit says where it is.
	*/
	void					Stamp_Disc(const Vector3 & center,float radius,float value,float feather = 0.0f);
	void					Stamp_Polyline(const Vector3 * points,int count,float width,float value,float feather = 0.0f);

	/*
	**	How far every point is from the nearest place where source is at or above threshold,
	**	in world units, saturating at max_distance.
	**
	**	This is what turns a river mask into the water-distance input Section 18 asks for, and
	**	it is a two-pass chamfer sweep rather than an exact transform: the error is a few per
	**	cent on diagonals, and nothing downstream of a shoreline blend can see that.
	*/
	bool					Build_Distance_Field(const TerrainMaskClass & source,float threshold,float max_distance);

	/*
	**	Deterministic variation.  Section 18 wants macro variation across a map without a hand
	**	painted texture, and a random number generator would give a different map every time the
	**	level loaded.  This is a hash of the grid position and a seed: no state, no order
	**	dependence, and the same answer on the server and on every client.
	*/
	static float		Hash_Value(int ix,int iy,int seed);
	void					Fill_With_Noise(int seed,float frequency,float low = 0.0f,float high = 1.0f);

private:

	float					Sample_Bilinear(float fx,float fy) const;

	float *				Values;
	int					VertexCountX;
	int					VertexCountY;
	float					CellSize;
	Vector3				Origin;

	// Not implemented.
	TerrainMaskClass(const TerrainMaskClass & that);
	TerrainMaskClass & operator = (const TerrainMaskClass & that);
};

#endif	// TERRAINMASK_H
