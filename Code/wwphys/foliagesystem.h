/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : OpenW3D                                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/WWPhys/foliagesystem.h                       $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	A forest that is drawn as a few dozen things rather than a few thousand.                    *
 *                                                                                             *
 *	Roadmap Section 21's acceptance is that large forests render with substantially fewer       *
 *	submissions than individual RenderObj instances, and the shape it asks for is cells,        *
 *	instances, visibility, distance LOD, material batches and optional collision proxies.       *
 *	Those are the five things this does, in that order:                                         *
 *                                                                                             *
 *	  cells           the world is cut into a grid.  A cell is the unit of culling and the      *
 *	                  unit of rebuilding, and it is why a batch can be large without a bridge   *
 *	                  or a tree at the far end of the map being in it.                          *
 *	  instances       a plant is a type, a position, a yaw and a scale.  Nothing per instance   *
 *	                  is allocated and nothing per instance is submitted.                       *
 *	  batches         within a cell, everything sharing a material and a level of detail is     *
 *	                  merged into one mesh, which is one thing to draw.  That is the whole      *
 *	                  acceptance: Get_Batch_Count against Get_Instance_Count.                   *
 *	  visibility      cells are culled by distance and by a cone about the view direction,      *
 *	                  and a culled cell takes all of its batches with it.                       *
 *	  distance LOD    a batch is either the merged model geometry, the merged impostor          *
 *	                  quads, or nothing.  Both meshes are built once and hidden, rather than    *
 *	                  the buffer being refilled as the camera moves.                            *
 *	  proxies         collision is a separate, merged, invisible mesh of posts, built only      *
 *	                  for the instances whose type blocks something.  A tree's leaves are not   *
 *	                  a thing to walk into, so the drawn mesh never collides.                   *
 *                                                                                             *
 *	The donor packs every tree in the world into one vertex buffer with a hard cap of four      *
 *	thousand trees, and refills that buffer as visibility changes.  Section 21 says not to      *
 *	preserve inefficient donor structure where something better fits, and this is where: cells  *
 *	give the same batching with no global cap, no per-frame refill, and culling that survives   *
 *	the camera turning round.                                                                   *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef FOLIAGESYSTEM_H
#define FOLIAGESYSTEM_H

#include "always.h"
#include "foliagetype.h"
#include "vector.h"
#include "vector3.h"
#include "wwstring.h"

class StaticPhysClass;


/*
**	One square of the grid, and the instances standing in it.
**
**	The instances are held as a range into one flat index array rather than as a vector per
**	cell, because there are a great many cells and a vector each is a great many allocations
**	for a list that never changes after the grid is built.
*/
class FoliageCellClass
{
public:

	FoliageCellClass(void) :
		X(0), Y(0), Min(0.0f,0.0f,0.0f), Max(0.0f,0.0f,0.0f),
		First(0), Count(0), Distance(0.0f), Visible(false) { }

	bool	operator == (const FoliageCellClass & that) const
	{
		return (X == that.X) && (Y == that.Y) && (First == that.First) && (Count == that.Count);
	}
	bool	operator != (const FoliageCellClass & that) const	{ return !(*this == that); }

	Vector3	Get_Center(void) const		{ return (Min + Max) * 0.5f; }
	float		Get_Radius(void) const;

	int			X;
	int			Y;
	Vector3		Min;
	Vector3		Max;

	int			First;
	int			Count;

	float			Distance;
	bool			Visible;
};


/*
**	One thing to draw: everything in one cell that shares a material and a level of detail.
**
**	NearDistance and CullDistance are the earliest near distance and the latest cull distance
**	of the types that contributed, so a batch keeps its detail until the last thing in it says
**	to drop it and disappears only when everything in it has.  A batch is drawn or not; nothing
**	inside one is decided separately, which is the price of merging and the reason cells are
**	small.
*/
class FoliageBatchClass
{
public:

	FoliageBatchClass(void) :
		Cell(-1), Lod(FOLIAGE_LOD_NEAR), First(0), Count(0),
		NearDistance(0.0f), CullDistance(0.0f), PolyCount(0),
		Phys(nullptr), Visible(false) { }

	bool	operator == (const FoliageBatchClass & that) const
	{
		return (Cell == that.Cell) && (Lod == that.Lod) && (Material == that.Material) &&
				 (First == that.First) && (Count == that.Count);
	}
	bool	operator != (const FoliageBatchClass & that) const	{ return !(*this == that); }

	int				Cell;
	int				Lod;
	StringClass		Material;

	int				First;
	int				Count;

	float				NearDistance;
	float				CullDistance;

	int				PolyCount;

	StaticPhysClass *	Phys;
	bool					Visible;
};


/*
**	The collision half.  One invisible merged mesh of posts per cell per set of collision bits,
**	built only where something in that cell blocks something.
*/
class FoliageProxyClass
{
public:

	FoliageProxyClass(void) :
		Cell(-1), Blocking(FOLIAGE_BLOCKS_NOTHING), Count(0), Phys(nullptr) { }

	bool	operator == (const FoliageProxyClass & that) const
	{
		return (Cell == that.Cell) && (Blocking == that.Blocking) && (Count == that.Count);
	}
	bool	operator != (const FoliageProxyClass & that) const	{ return !(*this == that); }

	int					Cell;
	unsigned				Blocking;
	int					Count;
	StaticPhysClass *	Phys;
};


class FoliageSystem
{
public:

	static void			Init(void);
	static void			Shutdown(void);

	//	The kinds of thing that can stand in a field.
	static bool			Define_Type(const FoliageTypeClass & type);
	static int			Get_Type_Count(void);
	static FoliageTypeClass *	Peek_Type(int index);
	static FoliageTypeClass *	Find_Type(const char * name);
	static int			Find_Type_Index(const char * name);
	static void			Define_Default_Foliage(void);

	//	The things standing in it.  Add everything first: adding invalidates the grid, because
	//	the grid is where the instances are and it is built from all of them at once.
	static int			Add_Instance(const FoliageInstanceClass & instance);
	static int			Add_Instance(const char * type,const Vector3 & position,
											float yaw = 0.0f,float scale = 1.0f);
	//	Deterministic: the same seed puts the same forest on the server and on every client,
	//	which is why the scatter is here and not in whatever calls it.
	static int			Scatter(const char * type,const Vector3 & center,float radius,
										int count,unsigned seed);
	static int			Get_Instance_Count(void);
	static int			Get_Live_Instance_Count(void);
	static const FoliageInstanceClass *	Peek_Instance(int index);
	static void			Clear_Instances(void);

	//	The grid.
	static bool			Build_Cells(float cell_size = 32.0f);
	static bool			Are_Cells_Built(void);
	static int			Get_Cell_Count(void);
	static const FoliageCellClass *	Peek_Cell(int index);
	static float		Get_Cell_Size(void);
	static int			Find_Cell(float x,float y);
	static int			Get_Cell_Instance(int cell,int n);

	//	The batches.  This is the acceptance: how many things the forest is to draw.
	static bool			Build_Batches(void);
	static bool			Are_Batches_Built(void);
	static int			Get_Batch_Count(void);
	static const FoliageBatchClass *	Peek_Batch(int index);
	static int			Get_Batch_Instance(int batch,int n);

	static int			Get_Proxy_Count(void);
	static const FoliageProxyClass *	Peek_Proxy(int index);

	//	Visibility and distance LOD.  Pass a cosine of -1 for a half angle to cull by distance
	//	alone, which is what a system with no camera to ask should do.
	static void			Update_Visibility(const Vector3 & eye,const Vector3 & forward,
													float half_angle_cos = -1.0f,float max_distance = 0.0f);
	static int			Get_Visible_Cell_Count(void);
	static int			Get_Visible_Batch_Count(void);
	static int			Get_Visible_Instance_Count(void);

	//	Geometry.  Needs a physics scene; without one the batches are still planned and still
	//	counted, which is what makes the acceptance answerable without a device.
	static bool			Build_Geometry(void);
	static void			Destroy_Geometry(void);
	static bool			Has_Geometry(void);
	static int			Get_Object_Count(void);
	static int			Get_Missing_Model_Count(void);
	static int			Get_Multi_Material_Model_Count(void);
	static int			Get_Oversized_Batch_Count(void);

	//	Destructible instances.  Returns true when the instance died on this call.
	static bool			Apply_Damage(int instance,float amount);
	static bool			Destroy_Instance(int instance);
	static int			Get_Destroyed_Count(void);

	//	Queries.
	static int			Find_Instances_Near(const Vector3 & position,float radius,
														DynamicVectorClass<int> & found);
	static int			Find_Nearest_Instance(const Vector3 & position,float radius);
	static bool			Is_Blocked(const Vector3 & position,unsigned blocking_bits);

private:

	static void			Clear_Cells(void);
	static void			Clear_Batches(void);
	static bool			Build_Proxies(void);
	static bool			Build_Batch_Geometry(FoliageBatchClass & batch);
	static bool			Build_Proxy_Geometry(FoliageProxyClass & proxy);
	static void			Destroy_Batch_Geometry(FoliageBatchClass & batch);
	static void			Destroy_Proxy_Geometry(FoliageProxyClass & proxy);
	static void			Rebuild_Cell_Geometry(int cell);
	static void			Apply_Visibility(void);

	static DynamicVectorClass<FoliageTypeClass>		Types;
	static DynamicVectorClass<FoliageInstanceClass>	Instances;
	static DynamicVectorClass<FoliageCellClass>		Cells;
	static DynamicVectorClass<FoliageBatchClass>		Batches;
	static DynamicVectorClass<FoliageProxyClass>		Proxies;

	static DynamicVectorClass<int>	CellInstances;
	static DynamicVectorClass<int>	BatchInstances;
	static DynamicVectorClass<int>	Grid;

	static float		CellSize;
	static float		GridOriginX;
	static float		GridOriginY;
	static int			GridWidth;
	static int			GridHeight;

	static bool			CellsBuilt;
	static bool			BatchesBuilt;
	static bool			GeometryBuilt;

	static int			VisibleCells;
	static int			VisibleBatches;
	static int			VisibleInstances;

	static int			MissingModels;
	static int			MultiMaterialModels;
	static int			OversizedBatches;
	static int			Destroyed;
};

#endif // FOLIAGESYSTEM_H
