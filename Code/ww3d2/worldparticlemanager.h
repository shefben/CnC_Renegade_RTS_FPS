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
 *                 Project Name : WW3D                                                         *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/ww3d2/worldparticlemanager.h                 $*
 *                                                                                             *
 *	Roadmap Section 26, Zero Hour / SAGE feature 10 -- particle batching.                        *
 *                                                                                             *
 *	What the stock engine does is one draw per emitter.  ParticleBufferClass::Render_Particles   *
 *	ends in PointGroupClass::Render, and that function sets a material, a shader and a texture,  *
 *	locks a dynamic vertex buffer, fills it and draws.  Every one of those is per emitter, so a  *
 *	firefight with sixty explosions in it pays sixty state blocks, sixty vertex-buffer locks     *
 *	and sixty draw calls to put up a few thousand quads that nearly all share one texture.       *
 *	That is the cost the roadmap's acceptance names, and it is the cost this owner removes.      *
 *                                                                                             *
 *	The simulation does not move.  ParticleBufferClass still ages particles, interpolates its    *
 *	keyframes and decides what is alive, because that is content behaviour and a second          *
 *	implementation of it would be a second way for stock content to look wrong.  What moves is   *
 *	the submission: a buffer hands its live particles here instead of drawing them, they are     *
 *	gathered into one of three fixed pools, and at flush each distinct combination of texture,   *
 *	shader and point shape is drawn once.  Draw calls stop counting emitters and start counting  *
 *	materials.                                                                                   *
 *                                                                                             *
 *	The pools are fixed and allocated once, which is the other half of the acceptance.  Stock    *
 *	Renegade allocates roughly twenty arrays per emitter at construction and frees them at       *
 *	death, so a firefight is also an allocator workload; the pools here are sized at startup,    *
 *	never grow, and are the budget rather than merely holding it.                                *
 *                                                                                             *
 *	Appearance is preserved exactly, not approximately.  The staging arrays always carry size,   *
 *	orientation and frame per particle even for a buffer that had none, which is bit-identical   *
 *	to the constant-offset loops in PointGroupClass::Update_Arrays because those loops index     *
 *	the same tables with the same values.  The one property that is not free is the diffuse      *
 *	array, because its absence changes a shader bit -- an untextured-white emitter draws with    *
 *	GRADIENT_DISABLE -- so whether the gradient is disabled is part of the batch key and two     *
 *	submissions that disagree about it are never merged.                                         *
 *                                                                                             *
 *	Turn it off with Set_Enabled(false) and every buffer draws itself exactly as it does today.  *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef WORLDPARTICLEMANAGER_H
#define WORLDPARTICLEMANAGER_H

#include "always.h"
#include "shader.h"
#include "vector3.h"
#include "vector4.h"

class RenderInfoClass;
class TextureClass;


/*
**	The three shared pools Section 26 asks for.  They are separate pools and not one pool with a
**	mode field because the three draw through different renderers and want different per-particle
**	data: a streak needs a second position and a second colour that a sprite has no use for.
**
**	SPRITE is W3D_EMITTER_RENDER_MODE_QUAD_PARTICLES -- a textured, orientable quad, which is
**	what almost all stock Renegade content uses.  POINT is the TRI_PARTICLES mode, three vertices
**	instead of four.  STREAK is the LINEGRP_TETRA and LINEGRP_PRISM modes, where each particle is
**	drawn as a solid between where it is and where it was.
**
**	The fourth stock render mode, W3D_EMITTER_RENDER_MODE_LINE, has no pool and is not batched.
**	It is a single continuous polyline threaded through one emitter's particles in order, so two
**	emitters' particles cannot share one of them: concatenating them would draw a line joining
**	two unrelated effects.  It stays one draw per emitter, deliberately.
*/
enum ParticlePoolType
{
	PARTICLE_POOL_SPRITE = 0,
	PARTICLE_POOL_POINT,
	PARTICLE_POOL_STREAK,

	PARTICLE_POOL_COUNT
};


/*
**	Budget classes, in the order Section 26 gives them, which is also the order of decreasing
**	claim on a full pool.
**
**	A budget class answers one question: when more particles want to exist than the pool can
**	hold, whose disappear?  CRITICAL_GAMEPLAY is served first, out of a reserve no other class
**	can reach -- an effect a player reads as information rather than decoration, a beacon's
**	plume or a placement confirmation.  COMBAT_NEAR and COMBAT_FAR are the same effects at different
**	distances and are not set by anyone: a submission that names no class is resolved to one of
**	them by how far the effect is from the camera, so the same explosion is near when it is in
**	the player's face and far when it is across the map.  ENVIRONMENT is weather and Section 36
**	owns it.  AMBIENT is decoration -- steam, drips, smoke from scenery -- and is thinned first.
*/
enum ParticleBudgetType
{
	PARTICLE_BUDGET_CRITICAL_GAMEPLAY = 0,
	PARTICLE_BUDGET_COMBAT_NEAR,
	PARTICLE_BUDGET_COMBAT_FAR,
	PARTICLE_BUDGET_ENVIRONMENT,
	PARTICLE_BUDGET_AMBIENT,

	PARTICLE_BUDGET_COUNT,

	/*
	**	Not a budget class.  It is what a buffer carries until somebody says otherwise, and it
	**	resolves to COMBAT_NEAR or COMBAT_FAR at submission time.  Stock content can never say
	**	anything else, because a .w3d emitter has no field for it and requiring a re-export is
	**	forbidden.
	*/
	PARTICLE_BUDGET_AUTO = -1
};


/*
**	One emitter's worth of particles, offered to a pool.
**
**	Every array may be null, in which case the matching Default is used for every particle; that
**	is how a ParticleBufferClass whose property is constant presents itself, and it is the common
**	case.  APT is the active point table the buffer already builds for its own level of detail --
**	null means the first Count entries of every array are live, which is what a full, undecimated
**	buffer looks like.
**
**	Center and Budget together answer which budget class pays for this.  A Budget of
**	PARTICLE_BUDGET_AUTO is resolved from Center; anything else is taken as given.
*/
struct ParticleSubmissionStruct
{
	TextureClass *			Texture;
	ShaderClass				Shader;
	int						PointMode;				// PointGroupClass::PointModeEnum; sprite and point pools
	int						LineMode;				// LineGroupClass::LineModeType; streak pool
	unsigned char			FrameRowColumnLog2;

	const Vector3 *			Position;				// required
	const Vector3 *			TailPosition;			// streak pool only, required there
	const Vector4 *			Diffuse;
	const Vector4 *			TailDiffuse;			// streak pool only
	const float *			Size;
	const unsigned char *	Orientation;			// sprite and point pools only
	const unsigned char *	Frame;					// sprite and point pools only
	const float *			UCoord;					// streak pool only

	Vector4					DefaultDiffuse;
	Vector4					DefaultTailDiffuse;
	float					DefaultSize;
	unsigned char			DefaultOrientation;
	unsigned char			DefaultFrame;
	float					DefaultUCoord;

	const unsigned int *	APT;
	int						Count;

	Vector3					Center;
	int						Budget;

	ParticleSubmissionStruct(void);
};


/*
**	WorldParticleManager
**	Static, because there is one world and the pools are the world's, not an emitter's.
*/
class WorldParticleManager
{
public:

	/*
	**	Sizing the pools and building the staging arrays.  Neither touches a device, so both are
	**	valid on a dedicated server and in a self check -- the renderers a flush eventually needs
	**	are built on first use, not here, because building one calls
	**	VertexMaterialClass::Get_Preset and that dereferences a null until a client has a device.
	*/
	static void				Init(void);
	static void				Shutdown(void);

	/*
	**	Release what a device owns without forgetting the sizes or the statistics.  Called when a
	**	device is lost or reset; the next flush rebuilds.
	*/
	static void				Release_Resources(void);

	/*
	**	Off means every particle buffer draws itself the way it always has.  This is the switch
	**	that makes "disabled is the stock image" checkable rather than asserted.
	*/
	static void				Set_Enabled(bool onoff);
	static bool				Is_Enabled(void);

	/*
	**	How many particles a pool may draw in one flush.  This is the budget: there is no separate
	**	number, because a budget that is not the size of the thing it governs is a number that can
	**	disagree with it.  Setting a capacity reallocates the staging arrays, so it is a startup
	**	or console operation and not something to do per frame.
	*/
	static void				Set_Pool_Capacity(ParticlePoolType pool,int capacity);
	static int				Get_Pool_Capacity(ParticlePoolType pool);

	/*
	**	The share of a pool a budget class is guaranteed, as a fraction.  A class may always
	**	exceed its share while the pool still has room -- the shares only decide who keeps what
	**	when it does not -- so in a quiet scene, where everything fits, the shares have no effect
	**	at all and the image is the stock image.  CRITICAL_GAMEPLAY's share is not settable,
	**	because a guaranteed minimum that can be tuned to nothing is not a guarantee.
	**
	**	Contention is measured rather than predicted: a flush records what each class asked for,
	**	and that decides the caps the next accumulation runs under.  A submission arriving on its
	**	own is indistinguishable from the first of five hundred, so any rule that decides at
	**	submission time has to assume the worst and ration a pool that is nearly empty.
	*/
	static void				Set_Budget_Share(ParticleBudgetType budget,float share);
	static float			Get_Budget_Share(ParticleBudgetType budget);

	/*
	**	Where the frame being drawn is being drawn from.  A submission that names no budget class
	**	is resolved against this, so the renderer keeps it current; nothing else needs to call it.
	*/
	static void				Set_Camera_Position(const Vector3 & position);

	/*
	**	Where an effect stops being near.  In world units from the camera.
	*/
	static void				Set_Combat_Near_Distance(float distance);
	static float			Get_Combat_Near_Distance(void);

	/*
	**	Offer a pool one emitter's live particles.
	**
	**	False means the manager is not taking submissions -- it is disabled, or it has not been
	**	initialised -- and the caller must draw itself.  True means the submission was taken, and
	**	the caller must not draw: taken does not mean every particle was kept, because a pool that
	**	is full thins the submission, and a caller that drew the remainder itself would defeat the
	**	budget and cost the draw call the budget exists to avoid.
	**
	**	Thinning takes every n'th particle rather than cutting the tail, so an emitter over budget
	**	loses density evenly instead of losing one end of itself.
	*/
	static bool				Submit(ParticlePoolType pool,const ParticleSubmissionStruct & submission);

	/*
	**	Draw everything offered since the last flush, one submission per distinct batch.
	**
	**	Called once per static sort level while those lists are being rendered, and once more at
	**	the end of the frame for the particles that never went on a sort list.  Per sort level is
	**	what keeps the draw order right: a batch is drawn at the end of the level its members were
	**	drawn in, and merging is confined to one level, so nothing moves in front of or behind
	**	anything it was not already in front of or behind.
	**
	**	With no device this does every part of the work except the drawing, and counts it, which
	**	is what lets the arithmetic be checked without a screen.
	*/
	static void				Flush(RenderInfoClass & rinfo);

	/*
	**	What the last Reset_Statistics to now looked like.  Submitted counts the particles
	**	offered, Drawn the ones that reached a batch, Thinned the difference the budget took.
	**	Batches counts draw submissions issued and Submissions counts the emitters behind them,
	**	so Submissions divided by Batches is the ratio the acceptance is about.
	*/
	static void				Reset_Statistics(void);
	static unsigned int		Get_Submitted_Particles(ParticlePoolType pool);
	static unsigned int		Get_Drawn_Particles(ParticlePoolType pool);
	static unsigned int		Get_Thinned_Particles(ParticlePoolType pool);
	static unsigned int		Get_Batch_Count(ParticlePoolType pool);
	static unsigned int		Get_Submission_Count(ParticlePoolType pool);
	static unsigned int		Get_Refused_Submission_Count(ParticlePoolType pool);
	static unsigned int		Get_Budget_Particles(ParticlePoolType pool,ParticleBudgetType budget);

	/*
	**	How much of a pool is spoken for right now, before the next flush empties it.  The self
	**	check reads this between submissions; nothing in the renderer needs it.
	*/
	static int				Get_Pending_Particles(ParticlePoolType pool);
	static int				Get_Pending_Batches(ParticlePoolType pool);

	/*
	**	Names, for the console and the self check.
	*/
	static const char *		Get_Pool_Name(ParticlePoolType pool);
	static const char *		Get_Budget_Name(ParticleBudgetType budget);
};

#endif // WORLDPARTICLEMANAGER_H
