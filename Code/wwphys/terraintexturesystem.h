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
 *                     $Archive:: /Commando/Code/WWPhys/terraintexturesystem.h                $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	What the ground is made of, decided rather than painted.                                    *
 *                                                                                             *
 *	Roadmap Section 18 asks for deterministic material selection from height, slope, curvature, *
 *	biome, moisture, water distance and the road, river, city, Tiberium, cliff and manual       *
 *	override masks, in layers -- base, detail, macro variation, cliff, wet shoreline, road,     *
 *	biome, scorch -- with a runtime build API so that a seed-based generator can dress a map    *
 *	with no hand-painted full-map texture.  Its acceptance is that a generated heightfield can  *
 *	obtain coherent materials entirely from runtime data and masks.                             *
 *                                                                                             *
 *	This is the deciding half only.  The drawing half already exists and is stock Westwood:     *
 *	a RenegadeTerrainPatchClass carries a list of material passes, each with a texture and a    *
 *	weight per vertex, and composites them in order.  The level editor filled those weights by  *
 *	hand with a brush.  Nothing here replaces that machinery; what it replaces is the brush.    *
 *                                                                                             *
 *	Deterministic means what it says.  There is no random number generator anywhere below this  *
 *	line: variation comes from a hash of the grid position and a seed, so a server and every    *
 *	client compute the same ground without anyone sending it, and reloading a level gives the   *
 *	map it gave last time.                                                                      *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef TERRAINTEXTURESYSTEM_H
#define TERRAINTEXTURESYSTEM_H

#include "always.h"
#include "terrainmask.h"
#include "vector.h"
#include "wwstring.h"

class RenegadeTerrainPatchClass;
class TerrainMaterialClass;


/*
**	The layer kinds Section 18 names.
**
**	The kind is not what decides where a layer goes -- the rule does that -- but it says what
**	the layer is for, which is what lets a caller ask "what is the ground here" and get an
**	answer more useful than a texture index.
*/
enum TerrainLayerKind
{
	TERRAIN_LAYER_BASE			= 0,		// the ground under everything; always opaque
	TERRAIN_LAYER_DETAIL,					// close-range break-up
	TERRAIN_LAYER_MACRO,						// large scale variation, so a plain is not one colour
	TERRAIN_LAYER_CLIFF,						// steep ground
	TERRAIN_LAYER_SHORELINE,				// wet ground near water
	TERRAIN_LAYER_ROAD,
	TERRAIN_LAYER_BIOME,
	TERRAIN_LAYER_SCORCH,					// damage, if it is kept
	TERRAIN_LAYER_KIND_COUNT
};


/*
**	The masks.  Everything a rule can ask about that is not the shape of the ground.
*/
enum TerrainMaskType
{
	TERRAIN_MASK_NONE				= -1,

	TERRAIN_MASK_ROAD				= 0,
	TERRAIN_MASK_RIVER,
	TERRAIN_MASK_CITY,
	TERRAIN_MASK_TIBERIUM,
	TERRAIN_MASK_CLIFF,
	TERRAIN_MASK_MOISTURE,
	TERRAIN_MASK_BIOME,
	TERRAIN_MASK_WATER_DISTANCE,
	TERRAIN_MASK_OVERRIDE,
	TERRAIN_MASK_COUNT
};


/*
**	When a layer applies, and how strongly.
**
**	Every test is a window with a soft edge: full strength inside the range, fading to nothing
**	across the feather outside it.  A hard range would put a visible line across the map wherever
**	a threshold was crossed, which is the thing that makes generated terrain look generated.
**
**	A test with min above max is off, which is what the default is, so a rule that names nothing
**	fires everywhere at full strength -- the right default for a base layer.
*/
class TerrainLayerRuleClass
{
public:
	TerrainLayerRuleClass(void);

	//	World height, in metres.
	float		MinHeight;
	float		MaxHeight;
	float		HeightFeather;

	//	Slope, in radians, measured from the vertex normal.
	float		MinSlope;
	float		MaxSlope;
	float		SlopeFeather;

	//	Curvature, in height per square metre; positive is convex.
	float		MinCurvature;
	float		MaxCurvature;
	float		CurvatureFeather;

	//	One mask, and the window of its values this layer wants.
	int		MaskType;
	float		MinMask;
	float		MaxMask;
	float		MaskFeather;

	//	Deterministic variation.  Amount is how much of the weight the hash is allowed to move;
	//	frequency is in grid cells per lattice step, so a smaller number is a broader pattern.
	float		NoiseAmount;
	float		NoiseFrequency;
	int		NoiseSeed;

	//	A final multiplier, and whether this layer pushes the others out of the way.
	float		Strength;
	bool		Exclusive;

	//	Helpers, so a caller does not have to remember which three fields make a window.
	void		Set_Height_Range(float min_height,float max_height,float feather = 0.0f);
	void		Set_Slope_Range(float min_slope,float max_slope,float feather = 0.0f);
	void		Set_Curvature_Range(float min_curvature,float max_curvature,float feather = 0.0f);
	void		Set_Mask_Range(int mask_type,float min_value,float max_value,float feather = 0.0f);
	void		Set_Noise(float amount,float frequency,int seed);
};


class TerrainLayerClass
{
public:
	TerrainLayerClass(void);
	~TerrainLayerClass(void);

	StringClass						Name;
	int								Kind;
	TerrainLayerRuleClass		Rule;
	TerrainMaterialClass *		Material;

	bool	operator == (const TerrainLayerClass & that) const	{ return (this == &that); }
	bool	operator != (const TerrainLayerClass & that) const	{ return (this != &that); }

private:
	// Not implemented.  A layer owns a reference to its material.
	TerrainLayerClass(const TerrainLayerClass & that);
	TerrainLayerClass & operator = (const TerrainLayerClass & that);
};


class TerrainTextureSystem
{
public:

	enum {
		//	Eight layer kinds, eight layers.  The stock patch will grow its pass list to whatever
		//	it is given; the limit is here so that a rule table with a mistake in it cannot turn
		//	one patch into a hundred draw calls.
		MAX_LAYERS		= 8,
	};

	static void			Init(void);
	static void			Shutdown(void);

	/*
	**	Masks.  Created to match the terrain service's heightfield, so that a rule can ask about
	**	height and about the road at the same grid point.  Every mask starts at zero, which means
	**	a caller that writes none of them still gets an answer -- the shape-only one.
	*/
	static bool			Create_Masks(void);
	static void			Destroy_Masks(void);
	static bool			Has_Masks(void);
	static TerrainMaskClass *	Peek_Mask(int mask_type);

	/*
	**	Water distance is derived rather than written: it is how far each point is from the
	**	nearest place the river mask fires.  Call it after the rivers are drawn and before the
	**	shoreline layer is asked about.
	*/
	static bool			Update_Water_Distance(float threshold = 0.5f,float max_distance = 64.0f);

	/*
	**	Layers, in draw order: the first is the ground, the rest go over it.  Passing no texture
	**	name defines the rule without content, which is what a check with no asset manager and a
	**	generator that has not chosen its art yet both want.
	*/
	static int			Define_Layer(const char * name,int kind,const TerrainLayerRuleClass & rule,
											 const char * texture_name = nullptr,
											 float meters_per_tile = 10.0f,int surface_type = 0);
	static void			Clear_Layers(void);
	static int			Get_Layer_Count(void);
	static TerrainLayerClass *	Peek_Layer(int index);
	static int			Find_Layer(const char * name);

	/*
	**	A ready-made table: grass, rock on the steep parts, a cliff face, a wet shoreline, and a
	**	road wherever the road mask says.  It exists so that a generated heightfield has coherent
	**	materials without anyone writing a rule table first, which is Section 18's acceptance
	**	condition stated as a function.
	*/
	static void			Define_Default_Layers(void);

	/*
	**	Evaluation.  Weights are per layer, in layer order, each between zero and one; the base
	**	layer is always one because something has to be underneath.  The return value is the
	**	number written.
	*/
	static int			Compute_Weights(int ix,int iy,float * weights_out,int max_weights);
	static int			Get_Dominant_Layer(int ix,int iy);
	static bool			Get_Dominant_Layer_At(float x,float y,int * layer_out);
	static bool			Get_Surface_Type_At(float x,float y,int * surface_type_out);

	/*
	**	Building.  Fills a terrain patch's material passes from the rules, in the same form the
	**	level editor's brush filled them in: the first layer with any influence on a quad draws
	**	it opaque, and each later one draws over it with its share of the weight.
	*/
	static bool			Build_Patch_Materials(RenegadeTerrainPatchClass * model,int px,int py);
	static bool			Build_All_Patch_Materials(void);

private:

	static float		Evaluate_Window(float value,float min_value,float max_value,float feather);
	static float		Evaluate_Rule(const TerrainLayerRuleClass & rule,int ix,int iy);

	static DynamicVectorClass<TerrainLayerClass *>	Layers;
	static TerrainMaskClass *								Masks;
	static bool													MasksCreated;
};

#endif	// TERRAINTEXTURESYSTEM_H
