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
 *                     $Archive:: /Commando/Code/wwphys/worldshadowmanager.h                  $*
 *                                                                                             *
 *	Roadmap Section 24, Zero Hour / SAGE feature 8 -- projected / cached shadows.                *
 *                                                                                             *
 *	One shadow implementation.  Everything that used to hold a piece of the shadow system --     *
 *	the settings on the physics scene, the two texture managers hidden as file statics in        *
 *	pscene_projectors.cpp, and the per-object policy inside DynamicShadowManagerClass -- lives   *
 *	here, and each of those is now a caller rather than an owner.                                *
 *                                                                                             *
 *	The TT-facing names on PhysicsSceneClass still work and still mean what they meant; they     *
 *	forward.  The world systems that arrived after them -- terrain, roads, bridges, foliage,     *
 *	water, and whatever comes next -- register a caster here and get the same shadow the         *
 *	soldier gets, from the same code, rather than growing a second shadow system of their own.   *
 *                                                                                             *
 *	The other half of Section 24 is the word "cached".  Stock Renegade re-rendered every         *
 *	visible shadow into a shared render target every single frame, because the render targets    *
 *	were handed out from a ring that reset per frame and nothing could hold one long enough to   *
 *	be worth keeping.  Here a projector owns its render target for as long as it keeps being     *
 *	drawn, and its texture is regenerated only when the thing it is a picture of changed: the    *
 *	light moved, the object turned, the model changed, or the pose advanced.                     *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef WORLDSHADOWMANAGER_H
#define WORLDSHADOWMANAGER_H

#include "always.h"
#include "bittype.h"
#include "quat.h"
#include "vector.h"
#include "vector3.h"

class DynTexProjectClass;
class PhysClass;
class RenderObjClass;
class TexProjectClass;
class TextureClass;


/*
**	How many objects that are not MovePhysClass or DynamicAnimPhysClass may cast a shadow at
**	once.  A world system registers its object here; the table does not grow with the content.
*/
enum { WORLD_SHADOW_MAX_REGISTERED_CASTERS = 64 };


/*
**	WorldShadowManager
**	Static, because there is one world and the roadmap asks for one implementation.
*/
class WorldShadowManager
{
public:

	/*
	**	The shadow modes, in the order TT and stock Renegade number them.  PhysicsSceneClass
	**	keeps its own ShadowEnum for source compatibility with everything that spells it that
	**	way; the values are the same and this is where they are decided.
	*/
	enum ShadowModeEnum
	{
		SHADOW_MODE_NONE = 0,			// no shadows at all
		SHADOW_MODE_BLOBS,				// projected blob shadows
		SHADOW_MODE_BLOBS_PLUS,			// blobs, with the main character getting a rendered shadow
		SHADOW_MODE_HARDWARE,			// render-to-texture shadows
		SHADOW_MODE_COUNT,
	};

	static void			Init(void);
	static void			Shutdown(void);

	/*
	**	Give up every device resource -- render targets and cached static shadow textures --
	**	without forgetting the settings.  Called when the device is lost and when a world is
	**	unloaded.
	*/
	static void			Release_Resources(void);

	/*
	**	Settings.  One owner; PhysicsSceneClass forwards to these and so does anything else
	**	that wants to ask.
	*/
	static void			Set_Mode(int mode);
	static int			Get_Mode(void);
	static void			Set_Attenuation(float atten_start,float atten_end);
	static void			Get_Attenuation(float * set_start,float * set_end);
	static void			Set_Normal_Intensity(float intensity);
	static float		Get_Normal_Intensity(void);

	/*
	**	Resolutions.  TT names the static and the dynamic one separately, which is the right
	**	split: the static one is a one-off cost paid when a level loads, the dynamic one is
	**	paid every time a shadow is regenerated.
	*/
	static void			Set_Dynamic_Resolution(unsigned int res);
	static unsigned int	Get_Dynamic_Resolution(void);
	static void			Set_Static_Resolution(unsigned int res);
	static unsigned int	Get_Static_Resolution(void);

	/*
	**	How many render targets exist.  This is the bound: shadows beyond this many are not
	**	drawn rather than drawn cheaply, because a shadow with nowhere to render is not a
	**	shadow.
	*/
	static void			Set_Max_Simultaneous_Shadows(unsigned int count);
	static unsigned int	Get_Max_Simultaneous_Shadows(void);

	/*
	**	The one per-object shadow policy.  DynamicShadowManagerClass calls this, and so does
	**	every registered caster, so a soldier and a bridge section get their shadow decided by
	**	the same code.  The shadow pointer is created and released through the caller's own
	**	pointer so the TT-facing class keeps owning what it always owned.
	*/
	static void			Update_Object_Shadow(PhysClass & obj,DynTexProjectClass ** shadow,
														float near_z,float far_z,
														bool force_blob_box,const Vector3 & blob_box_scale);
	static void			Release_Object_Shadow(DynTexProjectClass ** shadow);

	/*
	**	Casters that are not MovePhysClass or DynamicAnimPhysClass.  Those two embed a
	**	DynamicShadowManagerClass and always have; everything else in the world had no way to
	**	cast a shadow at all until this.
	*/
	static bool			Register_Caster(PhysClass * obj,float near_z = -1.0f,float far_z = -1.0f);
	static bool			Unregister_Caster(PhysClass * obj);
	static bool			Is_Registered_Caster(const PhysClass * obj);
	static int			Get_Registered_Caster_Count(void);

	/*
	**	Drives the registered casters.  The embedded managers are driven by their own object's
	**	post-timestep, which is where they have always been driven from.
	*/
	static void			Timestep(float dt);

	/*
	**	Render targets, held across frames by the projector using them.  Acquire returns false
	**	when there is no target to be had, which is the honest answer on a dedicated server and
	**	on hardware that would not give us one.
	*/
	static bool			Acquire_Render_Target(TexProjectClass * projector);
	static void			Release_Render_Target(TexProjectClass * projector);
	static bool			Holds_Render_Target(const TexProjectClass * projector);
	static void			Note_Texture_Rendered(TexProjectClass * projector);

	/*
	**	Invalidation.  Anything that changes what a shadow would look like says so here rather
	**	than reaching into a projector.
	*/
	static void			Invalidate_All(void);
	static void			Invalidate_Object(const PhysClass * obj);

	/*
	**	Shared static shadow resources.  Static shadows are keyed by what kind of object cast
	**	them and which way it was facing, so a hundred identical trees planted at the same
	**	angle share one texture.
	*/
	static TextureClass *	Peek_Static_Shadow_Texture(uint32 type_id,const Quaternion & orientation);
	static void				Add_Static_Shadow_Texture(uint32 type_id,const Quaternion & orientation,
																	TextureClass * tex);
	static void				Remove_Static_Shadow_Texture(TextureClass * tex);
	static void				Reset_Static_Shadow_Textures(void);
	static int				Get_Static_Shadow_Texture_Count(void);

	/*
	**	One render target of the shape shadows want.  Both halves of the system allocate
	**	through this, so there is one answer to "what format can this card render into".
	*/
	static TextureClass *	Create_Render_Target(unsigned int width,unsigned int height);

	/*
	**	Accounting.  Renders against cache hits is the whole point of the section stated in two
	**	numbers, and the counters reset each frame so they read as a rate.
	*/
	static void			Begin_Frame(void);
	static int			Get_Render_Target_Count(void);
	static int			Get_Held_Slot_Count(void);
	static int			Get_Texture_Render_Count(void);
	static int			Get_Cache_Hit_Count(void);
	static int			Get_Eviction_Count(void);
	static int			Get_Missing_Target_Count(void);
	static int			Get_Caster_Refusal_Count(void);
	static uint32		Get_Generation(void);

private:

	/*
	**	What a shadow texture is a picture of.  If none of this changed, the picture did not
	**	change either and there is nothing to render.
	*/
	class ShadowStampClass
	{
	public:
		ShadowStampClass(void);
		bool				Matches(const ShadowStampClass & that) const;

		Vector3			LightVector;
		Quaternion		Orientation;
		RenderObjClass *	Model;			// identity only, never dereferenced
		float				AnimFrame;
		uint32			Generation;
		bool				Perspective;
	};

	/*
	**	One render target and whoever is currently holding it.
	*/
	class ShadowSlotClass
	{
	public:
		ShadowSlotClass(void);
		void				Reset(void);

		//	Stubs so this can live in a DynamicVectorClass; slots are addressed by index and
		//	never searched for by value.
		bool	operator == (const ShadowSlotClass &) const	{ return false; }
		bool	operator != (const ShadowSlotClass &) const	{ return true; }

		TextureClass *		RenderTarget;
		TexProjectClass *	Holder;			// not ref-counted, released before the holder dies
		ShadowStampClass	Committed;
		ShadowStampClass	Pending;
		bool					HasCommitted;
		int					LastFrame;
	};

	/*
	**	A static shadow texture and the object kind and orientation it was rendered for.
	*/
	class StaticShadowTexClass
	{
	public:
		StaticShadowTexClass(void);
		StaticShadowTexClass(uint32 type_id,const Quaternion & orientation,TextureClass * tex);
		StaticShadowTexClass(const StaticShadowTexClass & that);
		~StaticShadowTexClass(void);
		const StaticShadowTexClass & operator = (const StaticShadowTexClass & that);

		bool	operator == (const StaticShadowTexClass &) const	{ return false; }
		bool	operator != (const StaticShadowTexClass &) const	{ return true; }

		uint32				ObjectTypeID;
		Quaternion			ObjectOrientation;
		TextureClass *		Texture;
	};

	/*
	**	An object that asked to cast a shadow without being one of the two classes that embed
	**	a shadow manager.
	*/
	class RegisteredCasterClass
	{
	public:
		RegisteredCasterClass(void);

		bool	operator == (const RegisteredCasterClass &) const	{ return false; }
		bool	operator != (const RegisteredCasterClass &) const	{ return true; }

		PhysClass *				Object;			// not ref-counted; unregistered before it dies
		DynTexProjectClass *	Shadow;
		float						NearZ;
		float						FarZ;
	};

	static int			Find_Slot(const TexProjectClass * projector);
	static int			Allocate_Slot(TexProjectClass * projector);
	static void			Release_Slot(int index);
	static void			Size_Slot_Table(unsigned int count);
	static void			Build_Stamp(const PhysClass & obj,const Vector3 & light_vector,
											bool perspective,ShadowStampClass * stamp);
	static bool			Texture_Is_Stale(TexProjectClass * projector,const ShadowStampClass & stamp);

	static int			ShadowMode;
	static float		AttenStart;
	static float		AttenEnd;
	static float		NormalIntensity;
	static unsigned int	DynamicResolution;
	static unsigned int	StaticResolution;

	static DynamicVectorClass<ShadowSlotClass>			Slots;
	static DynamicVectorClass<StaticShadowTexClass>	StaticTextures;
	static DynamicVectorClass<RegisteredCasterClass>	Casters;

	static uint32		Generation;
	static int			FrameStamp;
	static int			TextureRenders;
	static int			CacheHits;
	static int			Evictions;
	static int			MissingTargets;
	static int			CasterRefusals;
	static bool			Initialized;
};


#endif //WORLDSHADOWMANAGER_H
