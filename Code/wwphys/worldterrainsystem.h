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
class RenegadeTerrainPatchClass;
class StaticPhysClass;


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


/*
**	Roadmap Section 34 Phase 4 steps 13-16 -- the chunk registry.
**
**	"Chunk" is not a second unit alongside the patch this class already tracks; it is this one
**	under Phase 4's name for it.  A generated world's `ProceduralWorldDimensions::ChunkSize` is
**	chosen so that one heightfield patch (PATCH_CELLS cells) covers exactly one chunk, so the
**	seam guarantee Phase 4 step 15 asks for -- chunk seams use shared height samples -- is the
**	same guarantee the render/collision patch grid already has, not a second one to prove.
**
**	Buildable is a coarse slope-only signal, not Section 18C's canonical
**	`EvaluateBuildability` query -- that contract also needs road/corridor/tunnel/bridge/
**	resource exclusions that do not exist yet.  This field exists so a chunk can be ruled out
**	cheaply and early; it must never be read as the final word on whether something may be
**	built there once 18C's real query exists.
*/
class TerrainChunkInfoClass
{
public:
	TerrainChunkInfoClass(void) :
		MinHeight(0.0f), MaxHeight(0.0f),
		MinSlopeRadians(0.0f), MaxSlopeRadians(0.0f),
		Buildable(false), CliffBand(false),
		Revision(0), Checksum(0) { }

	float				MinHeight;
	float				MaxHeight;
	float				MinSlopeRadians;
	float				MaxSlopeRadians;
	bool				Buildable;		// slope-only, coarse -- see class comment
	bool				CliffBand;		// any sampled point in the chunk classified cliff band or past it
	unsigned int	Revision;		// bumped every time this entry is (re)computed; 0 means never
	unsigned int	Checksum;		// Section 3B.4-quantized CRC over the fields above
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
	/*
	**	What the ground is made of at a point.  The answer is an index into the terrain texture
	**	system's layer list -- Section 18 is what decides it, from height, slope, curvature and
	**	the masks -- and the surface type is the stock Renegade one that footsteps and decals
	**	already read.  Both return false when there is no terrain, and when the layer table has
	**	nothing to say, which is a level whose ground nobody has dressed yet.
	*/
	static bool			Get_Material(float x,float y,int * layer_index_out);
	static bool			Get_Surface_Type(float x,float y,int * surface_type_out);
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
	**	Which representation draws which ground, this frame.
	**
	**	The decision is per far block rather than per patch, and that is the whole trick: if a
	**	block and its own near patches could disagree, the two surfaces would be drawn over the
	**	same ground and fight.  One decision per block means every square metre of the field is
	**	drawn by exactly one representation, always.
	**
	**	Called once per frame from PhysicsSceneClass::Pre_Render_Processing, which is where the
	**	camera is.  With no far layer built it shows every near patch and returns, so a level that
	**	never asked for the far layer is unaffected.
	*/
	static void			Update_Terrain_LOD(const Vector3 & viewer_position);
	static int			Get_Visible_Near_Patch_Count(void);
	static int			Get_Visible_Far_Block_Count(void);

	/*
	**	Collision: the heightfield's triangles, as geometry the physics scene can see.
	**
	**	One RenegadeTerrainPatchClass per heightfield patch, each wrapped in a StaticPhysClass
	**	and handed to the static culling system.  The engine's own terrain render object is used
	**	rather than a new mesh type because it splits a cell along the same diagonal this
	**	heightfield does, so what a soldier walks into is the surface the sampling functions
	**	describe.  Section 17's requirement that rendered terrain and terrain collision derive
	**	from the same source data is kept by there being one triangulation and one grid.
	**
	**	Build_Collision creates whatever is missing and refills whatever is dirty, so calling it
	**	again after shaping the ground rebuilds only the patches that moved.  Update_Collision is
	**	the same sweep without the creation, for a caller that shapes terrain every frame and
	**	does not want to accidentally build a world.
	*/
	static bool			Build_Collision(void);
	static void			Update_Collision(void);
	static void			Destroy_Collision(void);
	static bool			Has_Collision(void);
	static int			Get_Collision_Patch_Count(void);
	static StaticPhysClass *	Peek_Collision_Patch(int px,int py);
	static RenegadeTerrainPatchClass *	Peek_Patch_Model(int px,int py);

	/*
	**	The geometry of one patch, on its own.  Create_Patch_Model hands back a render object
	**	holding a patch's triangles with one reference on it; Fill_Patch_Model refills one that
	**	already exists.  Both are public because the collision builder is not the only thing
	**	that will want a patch's mesh -- a renderer wants the same fill, and the checks build a
	**	patch with no physics scene to put it in.
	*/
	static RenegadeTerrainPatchClass *	Create_Patch_Model(int px,int py);
	static bool			Fill_Patch_Model(RenegadeTerrainPatchClass * model,int px,int py);

	/*
	**	The far/background layer.  Roadmap Section 34.
	**
	**	Not a second terrain database.  A far block is a decimated view of the one heightfield
	**	this class already owns: every FAR_STRIDE-th grid point, over FAR_BLOCK_PATCHES near
	**	patches each way, as one render object with no collision at all.  Everything that asks
	**	the ground a question still asks the near representation, which is Section 34's
	**	requirement that one terrain query/collision representation stay authoritative -- kept
	**	here by the far layer being physically unable to answer, not by convention.
	**
	**	Seams.  Far blocks meet on sampled grid lines and share those vertices exactly, so the
	**	far surface is continuous across the whole field.  Where it meets near terrain the two
	**	agree only at the sampled points, and between them the far chord can ride above the true
	**	ground; so the whole far layer is lowered by FarTerrainDrop, the largest amount by which
	**	the decimated surface anywhere exceeds the real one.  That makes far <= near everywhere,
	**	which is the invariant that matters: a boundary can show a small step down, never a hole
	**	through to the sky, and never far ground poking through near ground.
	*/
	enum {
		FAR_BLOCK_PATCHES	= 4,		// near patches per far block, each way
		FAR_STRIDE			= 4,		// grid vertices per far vertex; must divide PATCH_CELLS
	};

	static bool			Build_Far_Terrain_Representation(void);
	static void			Update_Far_Terrain(void);
	static void			Destroy_Far_Terrain(void);

	/*
	**	Has_Far_Terrain is about the block table, which a field gets when it is created, so that
	**	an edit made before anything was drawn is still recorded.  The blocks themselves need a
	**	physics scene to live in; Get_Far_Block_Built_Count is how many exist.
	*/
	static bool			Has_Far_Terrain(void);
	static int			Get_Far_Block_Built_Count(void);

	/*
	**	Editing.  A system that changes the ground -- a road grade, a river cut, the Commander's
	**	construction -- says which box it changed and the blocks over that box are refilled on
	**	the next Update_Far_Terrain.  Nothing reloads and nothing else is touched, which is
	**	Section 34's runtime/generated-terrain requirement.
	*/
	static void			Invalidate_Far_Terrain_Region(const AABoxClass & region);
	static void			Invalidate_Far_Block(int bx,int by);
	static bool			Is_Far_Block_Dirty(int bx,int by);

	static int			Get_Far_Block_Count_X(void);
	static int			Get_Far_Block_Count_Y(void);
	static bool			Get_Far_Block_Bounds(int bx,int by,AABoxClass * bounds_out);
	static StaticPhysClass *	Peek_Far_Block(int bx,int by);
	static RenegadeTerrainPatchClass *	Peek_Far_Block_Model(int bx,int by);
	static RenegadeTerrainPatchClass *	Create_Far_Block_Model(int bx,int by);

	/*
	**	What the layer cost and what it bought, as numbers a check can compare.
	*/
	static float		Get_Far_Terrain_Drop(void);
	static int			Get_Far_Vertex_Count(void);
	static int			Get_Near_Vertex_Count(void);
	static int			Get_Far_Refill_Count(void);

	/*
	**	The chunk registry.  Roadmap Section 34 Phase 4 steps 13-16.  See TerrainChunkInfoClass.
	**
	**	Device-less: every input is the heightfield's own patch data plus WorldBoundsService,
	**	neither of which needs a physics scene, so the registry can be built and checked before
	**	anything is rendered or collided with -- the same Plan/Realize split Phase 3's cliff
	**	ring uses, except here the "plan" is the whole thing a generator needs.
	**
	**	Build_Chunk_Registry allocates the table if it is missing or the wrong shape (mirrors
	**	Build_Collision's "different shape, start over") and (re)computes every entry that is
	**	new or whose patch is dirty; an entry nobody has touched since the last call is left
	**	alone.  `max_buildable_slope_radians` is remembered, so a caller that only wants to
	**	pick up edits made since the last generation calls it again with the same value.
	*/
	static bool			Build_Chunk_Registry(float max_buildable_slope_radians);
	static void			Destroy_Chunk_Registry(void);
	static bool			Has_Chunk_Registry(void);
	static int			Get_Chunk_Count_X(void);
	static int			Get_Chunk_Count_Y(void);
	static bool			Get_Chunk_Info(int px,int py,TerrainChunkInfoClass * info_out);
	static unsigned int	Get_Chunk_Checksum(int px,int py);

	//	A CRC over every chunk's own checksum, in row-major order -- the one number Phase 4
	//	step 17's manifest checksum and a device-less self check both mean by "the terrain".
	static unsigned int	Get_World_Chunk_Checksum(void);

private:

	static bool			Build_Collision_Patch(int px,int py);
	static bool			Build_Chunk_Registry_Entry(int px,int py);

	static void			Create_Far_Block_Table(void);
	static bool			Fill_Far_Block_Model(RenegadeTerrainPatchClass * model,int bx,int by);
	static bool			Build_Far_Block(int bx,int by);
	static void			Far_Block_Point_Counts(int bx,int by,int * points_x_out,int * points_y_out);
	static float		Compute_Far_Terrain_Drop(void);
	static void			Fold_Dirty_Patches_Into_Far_Blocks(void);

	static HeightfieldClass *	Heightfield;
	static bool						ReportedNoCollision;
	static bool						ReportedNoFarTerrain;

	static StaticPhysClass **	CollisionPatches;
	static int						CollisionPatchCountX;
	static int						CollisionPatchCountY;

	static StaticPhysClass **	FarBlocks;
	static bool *					FarBlockDirty;
	static int						FarBlockCountX;
	static int						FarBlockCountY;
	static float					FarTerrainDrop;

	//	Roadmap Section 52A, Phase 47: the far/near handover dissolve.  Parallel to FarBlocks,
	//	same lifetime, same indexing.  FarBlockLastNear is last frame's decision, so a change
	//	is detected without needing a getter off RenegadeTerrainPatchClass; FarBlockOverlapTimer
	//	counts down the window during which both layers stay drawn together.
	static bool *					FarBlockLastNear;
	static float *					FarBlockOverlapTimer;
	static int						FarRefillCount;
	static int						VisibleNearPatchCount;
	static int						VisibleFarBlockCount;

	static TerrainChunkInfoClass *	ChunkRegistry;
	static int							ChunkRegistryCountX;
	static int							ChunkRegistryCountY;
	static float						ChunkMaxBuildableSlopeRadians;
};

#endif	// WORLDTERRAINSYSTEM_H
