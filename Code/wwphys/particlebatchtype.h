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
 *                     $Archive:: /Commando/Code/wwphys/particlebatchtype.h                   $*
 *                                                                                             *
 *	Roadmap Section 26, Zero Hour / SAGE feature 10 -- the three shapes a batched particle can   *
 *	be drawn as, the five budget classes that decide which particles the world can afford, and   *
 *	the shape of a single particle in the one shared pool.                                      *
 *                                                                                             *
 *	The budget classes are the whole point.  A particle system without them answers "how many    *
 *	particles may exist" with "as many as were asked for", which is how a firefight in a         *
 *	courtyard costs more than the firefight decided.  With them the question has an answer that  *
 *	does not depend on how busy the frame is: the pool is a fixed size, every class has a share   *
 *	of it it can always have, and what is left over is headroom anybody may use until somebody   *
 *	more important wants it.                                                                    *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef PARTICLEBATCHTYPE_H
#define PARTICLEBATCHTYPE_H

#include "always.h"
#include "bittype.h"
#include "vector3.h"
#include "wwstring.h"


/*
**	How many particles the world may hold at once, across every effect and every budget class.
**	This is the only allocation in the system that is proportional to how many particles can
**	exist, it happens once, and it is the number every budget below is a share of.
*/
enum { PARTICLE_BATCH_POOL_SIZE = 4096 };

/*
**	How many particles may share one definition, and therefore one set of vertex buffers.  The
**	buffers for a group are made once at this size and refilled in place from then on, so this
**	caps the geometry a single texture can ever be asked to hold rather than capping the effect.
*/
enum { PARTICLE_BATCH_MAX_PER_GROUP = 1024 };

/*
**	How many distinct groups may exist at once.  Content names its own particle textures, so
**	groups are interned as they are asked for rather than declared up front, and this is what
**	stops a level full of differently textured effects from interning a buffer set each.
*/
enum { PARTICLE_BATCH_MAX_GROUPS = 24 };

/*
**	How many points one submission can carry.  PointGroupClass fills a dynamic vertex buffer of
**	1200 vertices at a time, so a quad group gets three hundred particles per submission and a
**	triangle group four hundred.  That is the real unit the acceptance is counted in: the cost
**	of a frame grows with the particles the camera can see divided by three hundred, and not
**	with the number of effects that produced them.
*/
enum { PARTICLE_BATCH_POINTS_PER_SUBMISSION_QUAD = 300 };
enum { PARTICLE_BATCH_POINTS_PER_SUBMISSION_TRI  = 400 };


/*
**	The shapes roadmap Section 26 names.  Each one is a different engine batcher underneath --
**	the first two are a PointGroupClass, the third a LineGroupClass -- and they are listed as
**	kinds here because the choice belongs to whoever defines the effect and to nobody else.
*/
enum ParticleBatchKindType
{
	PARTICLE_BATCH_SPRITE	= 0,		// a camera-facing textured quad that can be spun
	PARTICLE_BATCH_POINT,				// a camera-facing triangle: cheaper, no angle, no frames
	PARTICLE_BATCH_STREAK,				// a line from where the particle is to where it just was

	PARTICLE_BATCH_KIND_COUNT
};


/*
**	The budget classes roadmap Section 26 names, most important first.  The order is the policy:
**	when the pool is full, the lowest class with anything to spare loses a particle so a higher
**	one can have it, and a class that is inside its own reservation cannot be taken from at all.
**
**	CRITICAL_GAMEPLAY is for anything a player has to see to play correctly -- a beacon's plume,
**	a gas cloud, the smoke that says a building is about to fall.  AMBIENT is for anything the
**	world would look slightly nicer with and nobody would miss.  The two are deliberately far
**	apart, because the whole value of a budget class is what it is allowed to evict.
*/
enum ParticleBudgetClassType
{
	PARTICLE_BUDGET_CRITICAL_GAMEPLAY	= 0,
	PARTICLE_BUDGET_COMBAT_NEAR,
	PARTICLE_BUDGET_COMBAT_FAR,
	PARTICLE_BUDGET_ENVIRONMENT,
	PARTICLE_BUDGET_AMBIENT,

	PARTICLE_BUDGET_CLASS_COUNT
};


const char *	Particle_Batch_Kind_Name(int kind);
const char *	Particle_Budget_Class_Name(int budget_class);

/*
**	The share of the pool a class can always have, and the most it may ever hold.  The
**	reservations are what make a budget class a promise rather than a preference, so they must
**	add up to no more than the pool -- which is checked where they are installed, not here.
*/
int				Particle_Budget_Class_Default_Reserve(int budget_class);
int				Particle_Budget_Class_Default_Cap(int budget_class);

/*
**	How many particles of this kind one draw submission carries.  A streak group is drawn whole,
**	so it answers with its own cap.
*/
int				Particle_Batch_Kind_Points_Per_Submission(int kind);


/**
** ParticleBatchDefinitionClass
**
** A kind of particle: one texture, one shape, one budget class, one set of curves.  Every
** particle that shares a definition shares a set of buffers and is drawn with it, which is the
** whole of the "shared buffers/pools" requirement and most of the acceptance -- submissions are
** counted in definitions and in three hundreds, never in effects.
**
** The curves are deliberately two-ended rather than keyframed.  A keyframed particle is what
** the authored W3D emitters already are and they keep their own implementation; what this
** service is for is the effects the engine itself throws, which want a start, an end and no
** asset pipeline in between.
*/
class ParticleBatchDefinitionClass
{
public:

	ParticleBatchDefinitionClass(void);

	bool						operator == (const ParticleBatchDefinitionClass & that) const;
	bool						operator != (const ParticleBatchDefinitionClass & that) const { return !(*this == that); }

	const char *			Get_Name(void) const						{ return Name.Peek_Buffer(); }
	void						Set_Name(const char * name)			{ Name = (name != nullptr) ? name : ""; }

	const char *			Get_Texture(void) const					{ return Texture.Peek_Buffer(); }
	void						Set_Texture(const char * texture)	{ Texture = (texture != nullptr) ? texture : ""; }
	bool						Names_A_Texture(void) const			{ return (Texture.Get_Length() > 0); }

	int						Get_Kind(void) const						{ return Kind; }
	void						Set_Kind(int kind);

	int						Get_Budget_Class(void) const			{ return BudgetClass; }
	void						Set_Budget_Class(int budget_class);

	float						Get_Lifetime(void) const				{ return Lifetime; }
	void						Set_Lifetime(float seconds);

	float						Get_Start_Size(void) const				{ return StartSize; }
	void						Set_Start_Size(float size)				{ StartSize = (size > 0.0f) ? size : 0.0f; }

	float						Get_End_Size(void) const				{ return EndSize; }
	void						Set_End_Size(float size)				{ EndSize = (size > 0.0f) ? size : 0.0f; }

	const Vector3 &		Get_Start_Color(void) const				{ return StartColor; }
	void						Set_Start_Color(const Vector3 & c)	{ StartColor = c; }

	const Vector3 &		Get_End_Color(void) const				{ return EndColor; }
	void						Set_End_Color(const Vector3 & c)		{ EndColor = c; }

	float						Get_Start_Alpha(void) const			{ return StartAlpha; }
	void						Set_Start_Alpha(float alpha);

	float						Get_End_Alpha(void) const				{ return EndAlpha; }
	void						Set_End_Alpha(float alpha);

	float						Get_Gravity(void) const					{ return Gravity; }
	void						Set_Gravity(float accel)				{ Gravity = accel; }

	float						Get_Drag(void) const						{ return Drag; }
	void						Set_Drag(float per_second);

	float						Get_Spin_Rate(void) const				{ return SpinRate; }
	void						Set_Spin_Rate(float radians_per_second)	{ SpinRate = radians_per_second; }

	//	How much of the particle's own velocity is drawn behind it, in seconds.  Streaks only.
	float						Get_Streak_Seconds(void) const		{ return StreakSeconds; }
	void						Set_Streak_Seconds(float seconds)	{ StreakSeconds = (seconds > 0.0f) ? seconds : 0.0f; }

	//	Additive is what fire, sparks and muzzle flashes are; alpha-blended is what smoke and
	//	dust are.  Nothing else about the shader is a definition's business.
	bool						Is_Additive(void) const					{ return Additive; }
	void						Set_Additive(bool onoff)				{ Additive = onoff; }

	//	A texture divided into a 2^n by 2^n grid of frames, the engine's own sprite-sheet
	//	convention.  Zero means the whole texture is the particle.
	uint8						Get_Frame_Grid_Log2(void) const		{ return FrameGridLog2; }
	void						Set_Frame_Grid_Log2(uint8 log2);

	int						Get_Max_Particles(void) const			{ return MaxParticles; }
	void						Set_Max_Particles(int count);

private:

	StringClass				Name;					// what code asks for it by
	StringClass				Texture;				// what draws it, empty until art exists
	int						Kind;					// one of ParticleBatchKindType
	int						BudgetClass;		// one of ParticleBudgetClassType
	float						Lifetime;			// seconds from spawn to gone; never zero
	float						StartSize;			// metres, at birth
	float						EndSize;				// metres, at death
	Vector3					StartColor;			// multiplied by the per-particle tint
	Vector3					EndColor;
	float						StartAlpha;
	float						EndAlpha;
	float						Gravity;				// metres per second squared, down
	float						Drag;					// fraction of velocity lost per second
	float						SpinRate;			// radians per second, sprites only
	float						StreakSeconds;		// seconds of velocity drawn behind it, streaks only
	bool						Additive;			// additive, or alpha blended
	uint8						FrameGridLog2;		// 2^n frames across the texture
	int						MaxParticles;		// cap for this group, up to PARTICLE_BATCH_MAX_PER_GROUP
};


/**
** BatchParticleClass
**
** One particle.  A position, a velocity, the four things the renderer reads, a clock, and the
** group and budget class it belongs to.  There is no allocation here, no reference count and no
** virtual function: a particle is an entry in an array that already exists, which is the half
** of the acceptance that is about allocations rather than draw calls.
**
** The budget class is copied in at spawn rather than read back through the definition, so that
** retuning a definition cannot move particles that already exist between budgets and leave the
** per-class counts disagreeing with the pool.
*/
class BatchParticleClass
{
public:

	BatchParticleClass(void)		{ Reset(); }

	void						Reset(void);

	bool						Is_Free(void) const			{ return !InUse; }

	bool						InUse;				// this slot holds a particle
	int						Definition;			// index into the manager's definition table
	int						BudgetClass;		// one of ParticleBudgetClassType, fixed at spawn
	Vector3					Position;			// world space
	Vector3					Velocity;			// metres per second
	Vector3					Tint;					// per-spawn multiplier on the definition's colour
	float						SizeScale;			// per-spawn multiplier on the definition's size
	Vector3					Color;				// what the renderer reads, this frame
	float						Alpha;
	float						Size;
	float						Angle;				// radians
	float						Age;					// seconds since it was spawned
	float						Lifetime;			// seconds it was given

	/*
	**	The manager's bookkeeping, and nobody else's.  A live particle is on the doubly linked
	**	list of its budget class, oldest first, which is what makes "shed the oldest particle of
	**	the least important class" a constant-time answer rather than a scan of the pool at
	**	exactly the moment the pool is busiest.  A free slot is on the singly linked free list
	**	through ListNext, so the pool needs no second array and no allocation to be recycled.
	*/
	int						ListNext;
	int						ListPrev;
};


#endif //PARTICLEBATCHTYPE_H
