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
 *                     $Archive:: /Commando/Code/wwphys/worldparticlebatchmanager.h           $*
 *                                                                                             *
 *	Roadmap Section 26, Zero Hour / SAGE feature 10 -- particle batching.                        *
 *                                                                                             *
 *	One pool of particles for the whole world and one set of vertex buffers per texture, instead *
 *	of a render object, a buffer and a draw call for every effect that happens to be going on.   *
 *	Stock Renegade pays per emitter: a ParticleEmitterClass makes a ParticleBufferClass, the     *
 *	buffer allocates a dozen arrays sized by the asset, and the scene draws it on its own.  Two  *
 *	hundred impacts in a courtyard is two hundred of those.  Here two hundred impacts that share *
 *	a texture are two thousand particles in one group, and a group is submitted once per three   *
 *	hundred particles the camera can see.                                                       *
 *                                                                                             *
 *	What this service is *not* is a second implementation of the authored emitters.  A W3D       *
 *	emitter is content -- keyframed colour, opacity, size, rotation, frames, line properties --  *
 *	and ParticleBufferClass remains its one owner, because the asset describes that behaviour    *
 *	and Renegade content must keep loading unmodified.  This is for the particles the engine     *
 *	itself throws, where the caller has a position, a velocity and an opinion about how much     *
 *	the effect is worth, and wants no asset pipeline in between.                                *
 *                                                                                             *
 *	The budget classes are what make the cost of a frame a decision rather than an outcome.      *
 *	Every particle is spawned into one, each class has a share of the pool it can always have,   *
 *	and what is left is headroom: when the pool is full the lowest class with anything above its *
 *	reservation loses its oldest particle so a more important one can exist.  A class inside its *
 *	reservation is never taken from, and a class at its own cap recycles its own oldest rather   *
 *	than refusing -- an emitter at its ceiling should keep looking alive, just not get bigger.   *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef WORLDPARTICLEBATCHMANAGER_H
#define WORLDPARTICLEBATCHMANAGER_H

#include "always.h"
#include "aabox.h"
#include "bittype.h"
#include "particlebatchtype.h"
#include "vector.h"
#include "vector3.h"

class ParticleBatchGroupClass;
class PhysicsSceneClass;
class RenderInfoClass;


class WorldParticleBatchManager
{
public:

	/*
	**	Lifetime.  Init allocates the pool once and never again; it is the only allocation this
	**	service makes that is proportional to how many particles the world can hold.
	*/
	static void			Init(void);
	static void			Shutdown(void);

	/*
	**	The world going away.  Particles belong to the world they were thrown in, and the buffers
	**	belong to the scene they were added to, so both go and the pool stays.
	*/
	static void			Release_Resources(void);

	/*
	**	One definition per shape and per budget class, so that code which wants "a spark" can ask
	**	for one without knowing an asset name.  None of them names a texture yet, so none of them
	**	draws anything today -- see docs/assets/ParticleSprites.md.
	*/
	static void			Define_Default_Batches(void);

	/*
	**	Definitions.  A definition is a kind of particle: one texture, one shape, one budget
	**	class, one set of curves.  Content names its own particle textures, so a group can also be
	**	interned on demand from a texture name that nothing declared in advance.
	*/
	static int			Define_Definition(const ParticleBatchDefinitionClass & def);
	static int			Get_Definition_Count(void);
	static const ParticleBatchDefinitionClass &	Peek_Definition(int index);
	static ParticleBatchDefinitionClass *			Find_Definition(const char * name);
	static int			Find_Definition_Index(const char * name);
	static int			Find_Or_Define_Texture_Group(const char * texture_name,int kind,
																	int budget_class,float size,float lifetime);

	/*
	**	Point a definition at a texture it did not have.  The buffers are kept -- they hold no
	**	texture -- but the group's render object does, so it is rebuilt on the next fill.
	*/
	static bool			Set_Definition_Texture(int index,const char * texture_name);

	/*
	**	Emission.  One particle, or a burst thrown outward from a point.  Both answer whether the
	**	budget let them happen, because a caller that wants to know is entitled to know and one
	**	that does not can ignore it.
	*/
	static bool			Emit(int definition_index,const Vector3 & position,const Vector3 & velocity,
										const Vector3 & tint = Vector3(1.0f,1.0f,1.0f),float size_scale = 1.0f);

	static int			Emit_Burst(int definition_index,const Vector3 & center,int count,
											float speed,float spread_radius = 0.0f,
											const Vector3 & tint = Vector3(1.0f,1.0f,1.0f),
											float size_scale = 1.0f);

	static void			Clear_Particles(void);
	static void			Clear_Group(int definition_index);

	/*
	**	The clock.  Moves every particle, ages it, retires it, then refills the buffers that draw
	**	the survivors.  One pass over one array, whatever the world is doing.
	*/
	static void			Timestep(float dt);

	/*
	**	The budgets, and the headroom.  Reserve is the share a class can always have, cap is the
	**	most it may ever hold; the reservations must add up to no more than the pool, which is
	**	why installing them is a function and not a table a caller can write into.
	*/
	static bool			Set_Budget(int budget_class,int reserve,int cap);
	static void			Reset_Budgets(void);
	static int			Get_Budget_Reserve(int budget_class);
	static int			Get_Budget_Cap(int budget_class);
	static int			Get_Class_Particle_Count(int budget_class);
	static int			Get_Class_Headroom(int budget_class);
	static int			Get_Reserved_Total(void);
	static int			Get_Headroom(void);

	/*
	**	Geometry.  One set of buffers per group, made once at the largest that group can ever
	**	need and refilled in place from then on, and one render object to put them on the screen.
	*/
	static bool			Build_Geometry(void);
	static void			Destroy_Geometry(void);
	static bool			Has_Geometry(int definition_index);
	static bool			Has_Object(int definition_index);

	/*
	**	Particles further than this from the camera are left out of the buffers.  The group still
	**	culls as one object through the spatial index; this is the finer cut.  Zero or less means
	**	draw them all.
	*/
	static void			Set_Draw_Distance(float distance);
	static float		Get_Draw_Distance(void);

	/*
	**	Accounting.  Get_Particle_Count against Get_Submission_Count is the acceptance in two
	**	numbers, and Get_Buffer_Allocation_Count not moving while the first one grows is the
	**	other half of it.
	*/
	static int			Get_Pool_Size(void);
	static int			Get_Particle_Count(void);
	static int			Get_Group_Particle_Count(int definition_index);
	static int			Get_Drawn_Particle_Count(void);
	static int			Get_Culled_Particle_Count(void);
	static int			Get_Submission_Count(void);
	static int			Get_Object_Count(void);
	static int			Get_Poly_Count(void);
	static int			Get_Buffer_Allocation_Count(void);
	static int			Get_Refusal_Count(void);
	static int			Get_Class_Refusal_Count(int budget_class);
	static int			Get_Shed_Count(void);
	static int			Get_Recycle_Count(void);
	static int			Get_Missing_Texture_Count(void);
	static int			Get_Group_Refusal_Count(void);
	static int			Get_Peak_Particle_Count(void);

	/*
	**	The render path, called by the group's own render object and by nothing else.
	*/
	static void			Render_Group(int definition_index,RenderInfoClass & rinfo);
	static bool			Get_Group_Bounds(int definition_index,AABoxClass * box);
	static int			Get_Group_Sort_Level(int definition_index);
	static int			Get_Group_Poly_Count(int definition_index);

private:

	static int			Allocate_Slot(int definition_index,int budget_class);
	static void			Release_Slot(int slot);
	static int			Find_Oldest_In_Class(int budget_class);
	static int			Find_Oldest_In_Group(int definition_index);
	static void			Link_Into_Class(int slot,int budget_class);
	static void			Unlink_From_Class(int slot,int budget_class);

	static bool			Fill_Group(int definition_index);
	static bool			Build_Group_Buffers(int definition_index);
	static void			Attach_Group_Object(int definition_index);
	static void			Destroy_Group_Geometry(int definition_index);
	static void			Forget_Geometry(void);

	static DynamicVectorClass<ParticleBatchDefinitionClass>	Definitions;
	static DynamicVectorClass<ParticleBatchGroupClass *>		Groups;

	static BatchParticleClass *	Pool;
	static int							PoolSize;
	static int							LiveCount;
	static int							PeakCount;

	/*
	**	The free slots, as a stack threaded through the pool itself, and the per-class lists of
	**	live ones in spawn order.  Age order and spawn order are the same thing here -- every
	**	live particle is aged by the same dt -- so the head of a class list is its oldest
	**	particle, which is the one the budget policy is entitled to take.
	*/
	static int							FreeHead;
	static int							ClassHead[PARTICLE_BUDGET_CLASS_COUNT];
	static int							ClassTail[PARTICLE_BUDGET_CLASS_COUNT];

	static int							ClassCount[PARTICLE_BUDGET_CLASS_COUNT];
	static int							ClassReserve[PARTICLE_BUDGET_CLASS_COUNT];
	static int							ClassCap[PARTICLE_BUDGET_CLASS_COUNT];
	static int							ClassRefusals[PARTICLE_BUDGET_CLASS_COUNT];

	static float						DrawDistance;
	static int							Submissions;
	static int							DrawnParticles;
	static int							CulledParticles;
	static int							BufferAllocations;
	static int							Sheds;
	static int							Recycles;
	static int							MissingTextures;
	static int							GroupRefusals;

	static PhysicsSceneClass *		BuiltScene;
};


#endif //WORLDPARTICLEBATCHMANAGER_H
