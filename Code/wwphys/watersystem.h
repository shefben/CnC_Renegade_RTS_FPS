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
 *                     $Archive:: /Commando/Code/WWPhys/watersystem.h                         $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	Roadmap Section 22's acceptance is that water works visually and physically without a       *
 *	competing collision system.  Renegade already has one: a soldier's feet report              *
 *	SURFACE_TYPE_UNDERWATER_DIRT when they are on a lake bed, and a ray cast up from there       *
 *	looking for SURFACE_TYPE_WATER_PERMEABLE is how the existing wake and splash code finds      *
 *	the surface -- see soldier.cpp and surfaceeffects.cpp, both untouched by this.  A second,    *
 *	area-based water system that answered the same question its own way would be exactly the    *
 *	competing system the acceptance rules out.                                                  *
 *                                                                                             *
 *	So this does not build one.  The mesh a water area's geometry becomes is tagged             *
 *	SURFACE_TYPE_WATER_PERMEABLE, poly by poly, the same way any other piece of authored         *
 *	geometry would be, and given COLLISION_TYPE_PROJECTILE so rays and bullets find it and       *
 *	nothing else does -- a soldier and a vehicle pass through it and land on the real floor      *
 *	underneath, exactly as roadsystem.h explains a road surface must not be a second floor a     *
 *	few centimetres above the first.  Every one of Renegade's water reactions -- wake, splash,   *
 *	surface damage -- therefore already works over this geometry with no new code at all.        *
 *                                                                                             *
 *	What is new is the authoring and the rendering: a body of water is placed as a shape         *
 *	(watertype.h), the ground beneath a bounded one is carved to it, the terrain's existing       *
 *	river mask is stamped so the shoreline blends the way Section 18 already knows how to        *
 *	blend one, and a mesh is built if there is a texture to put on it.  A headless query -- is    *
 *	this point wet, how deep, which way is it flowing -- answers from the authored shape         *
 *	directly, the same way FoliageSystem::Is_Blocked answers without a scene.                     *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef WATERSYSTEM_H
#define WATERSYSTEM_H

#include "always.h"
#include "vector.h"
#include "vector3.h"
#include "watertype.h"


class WaterSystem
{
public:

	static void			Init(void);
	static void			Shutdown(void);

	/*
	**	Kinds.  None of the defaults names any texture; see docs/assets.
	*/
	static void			Define_Default_Water(void);
	static int			Define_Definition(const WaterDefinitionClass & def);	// add or replace by name
	static int			Get_Definition_Count(void)		{ return Definitions.Count(); }
	static const WaterDefinitionClass &	Peek_Definition(int index);
	static WaterDefinitionClass *			Find_Definition(const char * name);
	static int			Find_Definition_Index(const char * name);

	/*
	**	Placed areas.
	*/
	static int			Add_Area(const WaterAreaClass & area);		// returns its id
	static void			Clear_Areas(void);
	static int			Get_Area_Count(void)				{ return Areas.Count(); }
	static const WaterAreaClass &	Peek_Area(int index);
	static WaterAreaClass *			Find_Area(const char * name);
	static int			Find_Area_Index(const char * name);

	/*
	**	Shaping the ground and telling the terrain texture system where the shoreline is.  Both
	**	go through WorldTerrainSystem's and TerrainTextureSystem's own existing public entry
	**	points -- "the ground has one owner", per worldterrainsystem.h, and this asks it rather
	**	than reaching around it.
	*/
	static bool			Shape_Terrain(int index);
	static int			Shape_All_Terrain(void);			// returns how many areas were shaped
	static bool			Stamp_Mask(int index);
	static int			Stamp_All_Masks(void);				// also refreshes the water distance field

	/*
	**	The surface mesh.  Nothing is built for an area whose definition names no texture -- a
	**	channel with no water drawn over it is a dry channel, which is what it looks like, and
	**	every physical answer below still works from the authored shape either way.
	*/
	static bool			Build_Geometry(int index);
	static int			Build_All_Geometry(void);
	static void			Destroy_Geometry(int index);
	static void			Destroy_All_Geometry(void);
	static bool			Has_Geometry(int index);
	static int			Get_Object_Count(void);
	static int			Get_Missing_Texture_Count(void)	{ return MissingTextures; }

	/*
	**	Headless queries, answered from the authored shapes rather than the physics scene.
	*/
	static int			Find_Area_At(float x,float y);		// -1 if nothing is there
	static bool			Is_Point_In_Water(float x,float y)	{ return Find_Area_At(x,y) >= 0; }
	static bool			Get_Water_Height(float x,float y,float * height_out);
	static bool			Get_Water_Depth(float x,float y,float ground_z,float * depth_out);
	static bool			Get_Flow(float x,float y,Vector3 * direction_out,float * speed_out);
	static bool			Is_Navigable(float x,float y,float ground_z,float min_depth);

private:

	static bool			Build_Area_Geometry(WaterAreaClass & area,const WaterDefinitionClass & def);
	static void			Destroy_Area_Geometry(WaterAreaClass & area);

	static DynamicVectorClass<WaterDefinitionClass>	Definitions;
	static DynamicVectorClass<WaterAreaClass>			Areas;
	static int			NextID;
	static int			MissingTextures;
};

#endif	// WATERSYSTEM_H
