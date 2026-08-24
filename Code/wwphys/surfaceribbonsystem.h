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
 *                     $Archive:: /Commando/Code/WWPhys/surfaceribbonsystem.h                 $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	Marks on the ground that cost a fixed amount however long the game runs.                    *
 *                                                                                             *
 *	Roadmap Section 23 accepts this when "vehicles can leave stable marks without unbounded     *
 *	allocations or per-mark heavyweight objects."  Both halves of that are structural, so both  *
 *	are answered by structure rather than by care:                                              *
 *                                                                                             *
 *	  no unbounded allocation    every ribbon in the world comes out of one pool, allocated     *
 *	                             once at Init and never grown.  Every edge lives in a fixed     *
 *	                             array inside its ribbon.  Laying a mark writes into memory     *
 *	                             that already exists; when the pool is empty Bind returns -1     *
 *	                             and says so, instead of making more.  Get_Pool_Size never      *
 *	                             changes, and that is the acceptance measured.                  *
 *	  no per-mark heavy object   a mark is a RibbonEdgeClass -- two corners, a texture          *
 *	                             coordinate and an age.  It is not a RenderObj, not a PhysObj,  *
 *	                             not refcounted, not in the scene, not networked, not saved.    *
 *	                             What is in the scene is one mesh per kind of mark, so every    *
 *	                             tank track on the map is one thing to draw no matter how many  *
 *	                             tanks are driving.  Get_Edge_Count against Get_Object_Count    *
 *	                             is the ratio that shows it.                                    *
 *                                                                                             *
 *	The remaining Section 23 requirements fall out of the same shape.  Fade and lifetime are    *
 *	an age on each edge and a curve on the definition.  Pooling is Bind/Unbind.  Teleport reset *
 *	is a distance test on the emitter, so it needs no cooperation from the thing that moved.    *
 *	Conform is the caller handing us the point its own physics already found -- a wheel knows   *
 *	where it touched, and that point is on terrain, road or bridge deck without this system     *
 *	knowing which -- with Conform_Point for callers that have no contact of their own.          *
 *                                                                                             *
 *	Nothing here is networked, and that is the "network derivation from motion" requirement met *
 *	rather than skipped: every machine runs the same vehicle physics and so lays the same       *
 *	marks from the same contacts.  Sending them would be sending a function of state that has   *
 *	already been sent.                                                                          *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef SURFACERIBBONSYSTEM_H
#define SURFACERIBBONSYSTEM_H

#include "always.h"
#include "ribbontype.h"
#include "vector.h"
#include "vector3.h"

class DynamicMeshClass;
class DecorationPhysClass;
class PhysicsSceneClass;


/*
**	One laid ribbon: a pooled slot, a ring of edges, and the anchor the next edge is measured
**	from.
**
**	A ribbon outlives the thing that made it.  Unbind marks it unbound, and it keeps fading and
**	shrinking from the tail until it is empty, at which point the slot goes back to the pool.
**	That is why an emitter can be destroyed mid-run without its tracks blinking out.
*/
class SurfaceRibbonClass
{
public:

	SurfaceRibbonClass(void)	{ Reset(); }

	void			Reset(void);

	bool			Is_Free(void) const					{ return (!Bound && (Count == 0)); }
	int			Get_Edge_Count(void) const			{ return Count; }
	int			Get_Definition(void) const			{ return Definition; }
	int			Get_Owner(void) const				{ return Owner; }
	bool			Is_Bound(void) const					{ return Bound; }

	//	Edge 0 is the oldest still-living edge; Get_Edge_Count()-1 is the newest.
	const RibbonEdgeClass &	Peek_Edge(int index) const	{ return Edges[(First + index) % SURFACE_RIBBON_MAX_EDGES]; }

	int			Definition;
	int			Owner;
	bool			Bound;

	//	The ring.  First is the oldest edge, Count is how many are live.
	RibbonEdgeClass	Edges[SURFACE_RIBBON_MAX_EDGES];
	int			First;
	int			Count;

	//	Where the last edge was laid, and how far along the ribbon that was.
	Vector3		Anchor;
	bool			HaveAnchor;
	float			Distance;
};


/*
**	The service.  Static, like every other world system here, because there is one set of marks
**	in one world and passing a pointer to it through every vehicle would be ceremony.
*/
class SurfaceRibbonSystem
{
public:

	static void		Init(void);
	static void		Shutdown(void);

	//	The five kinds Section 23 names.  None of them names a texture; see the asset list.
	static void		Define_Default_Ribbons(void);

	/*
	**	Definitions
	*/
	static int											Define_Definition(const SurfaceRibbonDefinitionClass & def);
	static int											Get_Definition_Count(void);
	static const SurfaceRibbonDefinitionClass &	Peek_Definition(int index);
	static SurfaceRibbonDefinitionClass *			Find_Definition(const char * name);
	static int											Find_Definition_Index(const char * name);
	static int											Find_Definition_By_Category(int category);

	/*
	**	Emitters.  A handle is an index into the pool; it stays valid until Unbind, after which
	**	the ribbon finishes fading on its own and the slot is reused.
	*/
	static int		Bind(int definition_index,int owner);
	static void		Unbind(int handle);
	static bool		Is_Bound(int handle);
	static int		Find_Bound(int owner);

	//	Read a pooled ribbon.  For the checks and for debug drawing; nothing that lays marks
	//	needs it, and nothing may modify what it returns.
	static const SurfaceRibbonClass *	Peek_Ribbon(int handle);

	//	Lay the next mark.  Position is a point on the ground -- normally a wheel contact, which
	//	is already conformed to whatever it touched.  Forward is the direction of travel, normal
	//	is the ground normal, surface_type is what the ground is made of.  Returns true when an
	//	edge was actually added, which is only when the emitter has moved far enough.
	static bool		Add_Point(int handle,const Vector3 & position,const Vector3 & forward,
									const Vector3 & normal,int surface_type);

	//	End the current run without ending the ribbon.  The laid marks stay and fade; the next
	//	Add_Point starts a fresh, unconnected strip.  This is the teleport reset, and Add_Point
	//	calls it for itself when the emitter jumps further than the definition allows.
	static void		Break(int handle);

	//	Find the ground under a point for callers that have no contact of their own -- a scorch
	//	trail, a dragged body.  Uses the physics scene when there is one and the terrain service
	//	when there is not, so it answers the same way on a dedicated server.
	static bool		Conform_Point(float x,float y,float hint_z,float * height_out);

	/*
	**	Per frame.  Timestep ages every edge, drops the dead ones, frees emptied ribbons and
	**	rebuilds the meshes.  PhysicsSceneClass::Update calls it; nothing else needs to.
	*/
	static void		Timestep(float dt);

	//	Drop every mark in the world without disturbing who is laying them.  A level change does
	//	this for itself; the console command exists so a look can be started from clean ground.
	static void		Clear_Marks(void);

	/*
	**	Geometry.  Built by Timestep; these are here for the console and the checks.
	*/
	static bool		Build_Geometry(void);
	static void		Destroy_Geometry(void);
	static bool		Has_Geometry(int definition_index);

	/*
	**	Counts.  Get_Edge_Count against Get_Object_Count is Section 23's acceptance, and
	**	Get_Pool_Size never moving is the other half of it.
	*/
	static int		Get_Pool_Size(void);
	static int		Get_Active_Ribbon_Count(void);
	static int		Get_Edge_Count(void);
	static int		Get_Object_Count(void);
	static int		Get_Poly_Count(void);
	static int		Get_Missing_Texture_Count(void);
	static int		Get_Bind_Failure_Count(void);

private:

	static bool		Build_Definition_Geometry(int definition_index);
	static void		Destroy_Definition_Geometry(int definition_index);

	//	Let go of geometry belonging to a scene that no longer exists.  Unlike Destroy_Geometry
	//	this does not ask the current scene to remove anything, because the objects were never
	//	in it -- the scene that held them released them when it went.
	static void		Forget_Geometry(void);
	static float	Edge_Alpha(const SurfaceRibbonClass & ribbon,int index,
									 const SurfaceRibbonDefinitionClass & def);

	static DynamicVectorClass<SurfaceRibbonDefinitionClass>	Definitions;
	static SurfaceRibbonClass *									Pool;
	static int															PoolSize;

	//	One drawn object per definition, made only when that kind has something to show.
	static DynamicVectorClass<DecorationPhysClass *>			Objects;
	static DynamicVectorClass<DynamicMeshClass *>				Meshes;

	static int		MissingTextures;
	static int		BindFailures;

	//	The scene the current geometry was built into.  A level change makes a new one, and the
	//	meshes have to be made again rather than pointing into the world that just ended.
	static PhysicsSceneClass *	BuiltScene;
};

#endif // SURFACERIBBONSYSTEM_H
