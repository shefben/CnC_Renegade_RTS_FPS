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
 *                     $Archive:: /Commando/Code/wwphys/worldlightmanager.h                   $*
 *                                                                                             *
 *	Roadmap Section 25, Zero Hour / SAGE feature 11 -- dynamic-light filtering.                  *
 *                                                                                             *
 *	The goal in the roadmap is one sentence: stop passing all dynamic lights to all renderable   *
 *	content.  Static lights were never the problem -- they have lived in an AABTree since        *
 *	Renegade shipped and an object collects only the ones whose volume reaches it.  Dynamic      *
 *	lights were the problem, and the shape of the problem was that there were none: the scene    *
 *	declared Add_Dynamic_Light and never defined it, Collect_Lights took a dynamic_lights flag   *
 *	and ignored it under a "TODO: Dynamic lights!!", and the one place that could have grown a   *
 *	whole-world light scan had not grown it yet.                                                 *
 *                                                                                             *
 *	This is the owner.  A dynamic light lives in a grid keyed on its attenuation volume, so a    *
 *	query costs the cells it touches and the lights in them, not the number of lights in the     *
 *	world.  An object that no dynamic light reaches pays one box test and is handed exactly the  *
 *	cached static lighting environment it would have been handed before -- the common case has   *
 *	no new cost at all, which is the only way a per-object light query is affordable.            *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef WORLDLIGHTMANAGER_H
#define WORLDLIGHTMANAGER_H

#include "always.h"
#include "aabox.h"
#include "bittype.h"
#include "gridcull.h"
#include "physlist.h"
#include "vector.h"
#include "vector3.h"

class LightEnvironmentClass;
class LightPhysClass;
class PhysClass;


/*
**	How many dynamic lights may exist at once.  Muzzle flashes, explosions, tracer glow and the
**	Commander's placement feedback all want one for a fraction of a second; the table is sized
**	for a firefight, not for the content.
*/
enum { WORLD_LIGHT_MAX_DYNAMIC_LIGHTS = 64 };

/*
**	How many lights may be applied to a single object.  LightEnvironmentClass keeps the four
**	strongest as directional lights and folds the rest into ambient, so this is the number of
**	lights whose ambient contribution is worth summing, not a visual limit.
*/
enum { WORLD_LIGHT_MAX_LIGHTS_PER_OBJECT = 8 };


/*
**	WorldLightManager
**	Static, because there is one world.  The physics scene forwards to this; it does not keep a
**	second copy of anything here.
*/
class WorldLightManager
{
public:

	static void			Init(void);
	static void			Shutdown(void);

	/*
	**	Drop every dynamic light without forgetting the settings.  Called when a world is
	**	unloaded: a light belongs to the world it was lit in.
	*/
	static void			Release_Resources(void);

	/*
	**	The grid the dynamic lights live in.  Partitioning is lazy -- the first light asks the
	**	physics scene for the level extents -- so nothing has to remember to call this.  It is
	**	public because the level editor and the self check both want to say where the world is
	**	without having a scene.
	*/
	static void			Re_Partition(const Vector3 & world_min,const Vector3 & world_max);
	static bool			Is_Partitioned(void);

	/*
	**	Bounds.  Every one of these refuses rather than grows, and every refusal is counted.
	*/
	static void			Set_Max_Dynamic_Lights(unsigned int count);
	static unsigned int	Get_Max_Dynamic_Lights(void);
	static void			Set_Max_Lit_Objects(unsigned int count);
	static unsigned int	Get_Max_Lit_Objects(void);
	static float		Get_Max_Light_Radius(void);

	/*
	**	Making a dynamic light.  A caller that just wants a flash of coloured light somewhere
	**	uses Create_Point_Light and never sees a LightPhysClass; a caller with its own light
	**	object -- a weapon effect that animates one, say -- hands it over with Add_Dynamic_Light
	**	and keeps its own reference.
	**
	**	lifetime <= 0 means the light stays until it is removed.  A positive lifetime fades the
	**	intensity to nothing over that many seconds and then removes the light, which is what
	**	every muzzle flash in the game is going to want.
	*/
	static LightPhysClass *	Create_Point_Light(const Vector3 & position,const Vector3 & color,
															float inner_radius,float outer_radius,
															float intensity,float lifetime);

	static bool			Add_Dynamic_Light(LightPhysClass * light,float lifetime = -1.0f);
	static bool			Remove_Dynamic_Light(LightPhysClass * light);
	static bool			Is_Dynamic_Light(const LightPhysClass * light);
	static void			Remove_All_Dynamic_Lights(void);
	static int			Get_Dynamic_Light_Count(void);

	/*
	**	Moving a light.  Set_Transform already re-buckets it in the grid, so this only exists to
	**	keep the union bound honest without every caller having to know there is one.
	*/
	static void			Move_Dynamic_Light(LightPhysClass * light,const Vector3 & position);

	/*
	**	Fades and expires the lights with a lifetime.  Driven from PhysicsSceneClass::Update.
	*/
	static void			Timestep(float dt);

	/*
	**	The queries.  These are what Section 25 is about: the returned set is the lights whose
	**	attenuation volume actually reaches the argument, and the work done to find them scales
	**	with that set and the cells around it.
	*/
	static int			Collect_Dynamic_Lights(const Vector3 & point,NonRefPhysListClass * list);
	static int			Collect_Dynamic_Lights(const AABoxClass & bounds,NonRefPhysListClass * list);

	/*
	**	Composition.  Given an object and the cached static lighting environment it would have
	**	been rendered with, return the environment to actually render it with.  When no dynamic
	**	light reaches the object -- which is nearly every object nearly every frame -- the same
	**	pointer comes back and nothing was copied.
	**
	**	The returned pointer stays valid for the rest of the frame, because meshes on the static
	**	sort list keep it until the list is flushed.  Begin_Frame is what makes that true.
	*/
	static void			Begin_Frame(void);
	static LightEnvironmentClass *	Apply_Dynamic_Lights(PhysClass * obj,
																			LightEnvironmentClass * static_env);

	/*
	**	Accounting.  The acceptance is a claim about how cost scales, so the numbers that would
	**	show it failing are the ones kept: how many lights are in the world against how many were
	**	looked at, per query.
	*/
	static bool			Has_Dynamic_Lights(void);
	static void			Get_Light_Bounds(AABoxClass * set_bounds);
	static int			Get_Query_Count(void);
	static int			Get_Lights_Examined(void);
	static int			Get_Lights_Applied(void);
	static int			Get_Lit_Object_Count(void);
	static int			Get_Trivial_Reject_Count(void);
	static int			Get_Overflow_Count(void);
	static int			Get_Refusal_Count(void);
	static int			Get_Clamped_Radius_Count(void);

private:

	/*
	**	One dynamic light and how long it has left.
	*/
	class DynamicLightClass
	{
	public:
		DynamicLightClass(void);

		//	Stubs so this can live in a DynamicVectorClass; records are addressed by index and
		//	searched for by light pointer, never by value.
		bool	operator == (const DynamicLightClass &) const	{ return false; }
		bool	operator != (const DynamicLightClass &) const	{ return true; }

		LightPhysClass *	Light;			// ref-counted, this table owns one reference
		float					Life;				// seconds left, or a negative number for forever
		float					MaxLife;
		float					BaseIntensity;
	};

	static int			Find_Light(const LightPhysClass * light);
	static void			Ensure_Partitioned(void);
	static void			Recompute_Bounds(void);
	static void			Size_Environment_Pool(unsigned int count);
	static int			Gather(const Vector3 & point,LightPhysClass ** set,int max_set);

	static bool			Initialized;
	static bool			Partitioned;
	static unsigned int	MaxDynamicLights;
	static unsigned int	MaxLitObjects;

	/*
	**	The grid is the whole answer to the acceptance line.  It is a separate culling system
	**	from the one the dynamic objects live in, because a light query that had to walk every
	**	dynamic object and ask each one whether it happened to be a light would scale with the
	**	content instead of with the lights.
	*/
	static TypedGridCullSystemClass<LightPhysClass> *	Grid;
	static DynamicVectorClass<DynamicLightClass>			Lights;

	static AABoxClass	LightBounds;			// union of every dynamic light volume
	static bool			BoundsDirty;

	static int			QueryCount;
	static int			LightsExamined;
	static int			LightsApplied;
	static int			LitObjectCount;
	static int			TrivialRejects;
	static int			Overflows;
	static int			Refusals;
	static int			ClampedRadii;

	static int			EnvironmentCount;
	static int			EnvironmentNext;
	static LightEnvironmentClass *	Environments;
};


#endif //WORLDLIGHTMANAGER_H
