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
 *                 Project Name : WWPhys                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwphys/worldsurfacemarkmanager.h             $*
 *                                                                                             *
 *	Roadmap Section 35, Zero Hour / SAGE feature 22 -- the surface smudge / decal manager.       *
 *                                                                                             *
 *	One bounded surface-mark service, not a scorch database and a smudge database and a          *
 *	construction-mark database.  Everything that wants to leave a mark on the world asks here,   *
 *	and here decides how it gets drawn:                                                          *
 *                                                                                             *
 *	  - a mark that lies on ground the engine can conform to becomes four vertices in a mesh     *
 *	    shared with every other mark of its kind, so a thousand scorches cost as many draw       *
 *	    calls as one does;                                                                       *
 *	  - a mark that does not -- a bullet hole on a wall, a mark across a staircase -- is         *
 *	    handed to the geometry-clipping decal projector, which is the only thing that can put    *
 *	    a mark on arbitrary authored W3D geometry and have it follow the surface.                *
 *                                                                                             *
 *	Both are entries in the same fixed pool, so there is exactly one place in the engine that    *
 *	knows how many marks the world is holding and exactly one policy that decides which one      *
 *	goes when it is full.                                                                        *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef WORLDSURFACEMARKMANAGER_H
#define WORLDSURFACEMARKMANAGER_H

#include "always.h"
#include "bittype.h"
#include "matrix3d.h"
#include "surfacemarktype.h"
#include "vector.h"
#include "vector3.h"

class DecorationPhysClass;
class DynamicMeshClass;
class PhysClass;
class PhysicsSceneClass;


class WorldSurfaceMarkManager
{
public:

	/*
	**	Lifetime.  Init allocates the pool once and never again; it is the only allocation this
	**	service makes that is proportional to how many marks the world can hold.
	*/
	static void			Init(void);
	static void			Shutdown(void);

	/*
	**	The kinds roadmap Section 35 names, one definition each.  None of them names a texture
	**	yet, so none of them draws anything -- see docs/assets/SurfaceMarks.md.
	*/
	static void			Define_Default_Marks(void);

	/*
	**	Definitions.  A definition is a kind of mark: one texture, one look, one set of timings.
	**	Content names its own decal textures, so a group can also be interned on demand from a
	**	texture name that nothing declared in advance.
	*/
	static int			Define_Definition(const SurfaceMarkDefinitionClass & def);
	static int			Get_Definition_Count(void);
	static const SurfaceMarkDefinitionClass &	Peek_Definition(int index);
	static SurfaceMarkDefinitionClass *			Find_Definition(const char * name);
	static int			Find_Definition_Index(const char * name);
	static int			Find_Definition_By_Type(int type);
	static int			Find_Or_Define_Texture_Group(const char * texture_name,int type,float radius);

	/*
	**	Placement.  Create_Mark is the one entry point the game uses: it interns the group,
	**	decides which backend can draw the mark where it is, and places it.  Add_Mark is the
	**	batched path on its own, for callers that already know the ground is under them.
	*/
	static uint32		Create_Mark(	const Matrix3D &	tm,
												const char *		texture_name,
												float					radius,
												int					type,
												bool					apply_to_translucent_meshes = false,
												PhysClass *			only_this_obj = nullptr);

	static uint32		Add_Mark(int definition_index,const Vector3 & center,
										const Vector3 & normal,float radius);

	static bool			Remove_Mark(uint32 handle);
	static bool			Is_Mark_Alive(uint32 handle);
	static const SurfaceMarkClass *	Peek_Mark(uint32 handle);

	/*
	**	The clock, and the world going away.
	*/
	static void			Timestep(float dt);
	static void			Clear_Marks(void);

	/*
	**	Geometry.  One mesh per definition, made once at the largest that definition can ever
	**	need and refilled in place from then on.
	*/
	static bool			Build_Geometry(void);
	static void			Destroy_Geometry(void);
	static bool			Has_Geometry(int definition_index);

	/*
	**	Marks further than this from the camera are left out of the buffer.  The whole group
	**	still culls as one object through the spatial index; this is the finer cut.  Zero or
	**	less means draw them all.
	*/
	static void			Set_Draw_Distance(float distance);
	static float		Get_Draw_Distance(void);

	/*
	**	Accounting.  Get_Mark_Count against Get_Object_Count is the acceptance in two numbers,
	**	and Get_Pool_Size never moving is the other half.
	*/
	static int			Get_Pool_Size(void);
	static int			Get_Mark_Count(void);
	static int			Get_Batched_Mark_Count(void);
	static int			Get_Projected_Mark_Count(void);
	static int			Get_Projected_Budget(void);
	static int			Get_Group_Mark_Count(int definition_index);
	static int			Get_Object_Count(void);
	static int			Get_Poly_Count(void);
	static int			Get_Missing_Texture_Count(void);
	static int			Get_Eviction_Count(void);
	static int			Get_Group_Refusal_Count(void);

	/*
	**	Where the ground is, for a caller that has no contact of its own.  Composed from the
	**	answers that already exist rather than worked out again.
	*/
	static bool			Conform_Point(float x,float y,float hint_z,float * height_out);

private:

	static int			Allocate_Slot(int definition_index);
	static void			Release_Slot(int slot);
	static bool			Drape_Mark(SurfaceMarkClass & mark,const Vector3 & center,
											const Vector3 & normal,const Vector3 & tangent,float radius);

	static bool			Build_Definition_Geometry(int definition_index);
	static void			Destroy_Definition_Geometry(int definition_index);
	static void			Forget_Geometry(void);
	static float		Mark_Alpha(const SurfaceMarkClass & mark,
											const SurfaceMarkDefinitionClass & def);

	static DynamicVectorClass<SurfaceMarkDefinitionClass>	Definitions;

	static SurfaceMarkClass *		Pool;
	static int							PoolSize;
	static uint32						NextSerial;

	static DynamicVectorClass<DecorationPhysClass *>	Objects;
	static DynamicVectorClass<DynamicMeshClass *>		Meshes;

	static float						DrawDistance;
	static int							MissingTextures;
	static int							Evictions;
	static int							GroupRefusals;

	static PhysicsSceneClass *		BuiltScene;
};


#endif //WORLDSURFACEMARKMANAGER_H
