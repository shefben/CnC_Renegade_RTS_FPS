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
 *                     $Archive:: /Commando/Code/wwphys/worldshadowmanager.cpp                $*
 *                                                                                             *
 *	Roadmap Section 24 -- projected / cached shadows.  See worldshadowmanager.h for why this     *
 *	exists and docs/zerohour/WorldShadowManager.md for what was merged into it.                  *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "worldshadowmanager.h"

#include "camera.h"
#include "dx8caps.h"
#include "dx8wrapper.h"
#include "dyntexproject.h"
#include "light.h"
#include "colmathaabox.h"
#include "matpass.h"
#include "phys.h"
#include "physresourcemgr.h"
#include "pot.h"
#include "projector.h"
#include "refcount.h"
#include "pscene.h"
#include "rendobj.h"
#include "texture.h"
#include "wwdebug.h"
#include "wwmath.h"
#include "wwmemlog.h"


/*
**	The bounds.  Every one of these is a refusal that gets counted rather than a number that
**	quietly grows with the content.
*/
const unsigned int	WORLD_SHADOW_MIN_RESOLUTION		= 16;
const unsigned int	WORLD_SHADOW_MAX_RESOLUTION		= 256;
const unsigned int	WORLD_SHADOW_DEFAULT_RESOLUTION	= 256;
const unsigned int	WORLD_SHADOW_MAX_SLOTS				= 32;
const unsigned int	WORLD_SHADOW_DEFAULT_SLOTS			= 6;

/*
**	How much a thing has to change before the picture of it is worth taking again.  The
**	quaternion dot is the cosine of half the angle between two orientations, so 0.9999 is a
**	little under two degrees of turn.
*/
const float	WORLD_SHADOW_ORIENTATION_DOT	= 0.9999f;
const float	WORLD_SHADOW_LIGHT_EPSILON2	= 0.000001f;

/*
**	Two static objects of the same kind facing within ten degrees of each other share a shadow
**	texture.  Stock Renegade picked that number and it is the reason a row of trees is one
**	texture rather than twenty.
*/
const float	WORLD_SHADOW_STATIC_COS_HALF_THETA = WWMath::Cos(DEG_TO_RADF(10.0f) / 2.0f);


int			WorldShadowManager::ShadowMode			= WorldShadowManager::SHADOW_MODE_NONE;
float			WorldShadowManager::AttenStart			= 25.0f;
float			WorldShadowManager::AttenEnd			= 40.0f;
float			WorldShadowManager::NormalIntensity	= 0.45f;
unsigned int	WorldShadowManager::DynamicResolution	= WORLD_SHADOW_DEFAULT_RESOLUTION;
unsigned int	WorldShadowManager::StaticResolution	= WORLD_SHADOW_DEFAULT_RESOLUTION;

DynamicVectorClass<WorldShadowManager::ShadowSlotClass>			WorldShadowManager::Slots;
DynamicVectorClass<WorldShadowManager::StaticShadowTexClass>	WorldShadowManager::StaticTextures;
DynamicVectorClass<WorldShadowManager::RegisteredCasterClass>	WorldShadowManager::Casters;

uint32		WorldShadowManager::Generation		= 1;
int			WorldShadowManager::FrameStamp		= 0;
int			WorldShadowManager::TextureRenders	= 0;
int			WorldShadowManager::CacheHits			= 0;
int			WorldShadowManager::Evictions			= 0;
int			WorldShadowManager::MissingTargets	= 0;
int			WorldShadowManager::CasterRefusals	= 0;
bool			WorldShadowManager::Initialized		= false;


/***********************************************************************************************
**	ShadowStampClass -- what a cached shadow texture is a picture of
***********************************************************************************************/

WorldShadowManager::ShadowStampClass::ShadowStampClass(void) :
	LightVector(0.0f,0.0f,-1.0f),
	Orientation(1.0f,0.0f,0.0f,0.0f),
	Model(nullptr),
	AnimFrame(0.0f),
	Generation(0),
	Perspective(false)
{
}


bool WorldShadowManager::ShadowStampClass::Matches(const ShadowStampClass & that) const
{
	if (Generation != that.Generation) {
		return false;
	}
	if (Model != that.Model) {
		return false;
	}

	/*
	**	A perspective projection depends on where the object is, not only which way it faces,
	**	so it is never considered unchanged.  Only local lights project that way and they are
	**	rare; the sun is orthographic and that is the case worth caching.
	*/
	if (Perspective || that.Perspective) {
		return false;
	}

	if (AnimFrame != that.AnimFrame) {
		return false;
	}
	if ((LightVector - that.LightVector).Length2() > WORLD_SHADOW_LIGHT_EPSILON2) {
		return false;
	}

	//	q and -q are the same rotation, so the sign of the dot does not matter.
	float dot = Orientation.X * that.Orientation.X +
					Orientation.Y * that.Orientation.Y +
					Orientation.Z * that.Orientation.Z +
					Orientation.W * that.Orientation.W;

	return (WWMath::Fabs(dot) >= WORLD_SHADOW_ORIENTATION_DOT);
}


/***********************************************************************************************
**	ShadowSlotClass -- one render target and whoever is holding it
***********************************************************************************************/

WorldShadowManager::ShadowSlotClass::ShadowSlotClass(void) :
	RenderTarget(nullptr),
	Holder(nullptr),
	HasCommitted(false),
	LastFrame(-1)
{
}


void WorldShadowManager::ShadowSlotClass::Reset(void)
{
	Holder = nullptr;
	Committed = ShadowStampClass();
	Pending = ShadowStampClass();
	HasCommitted = false;
	LastFrame = -1;
}


/***********************************************************************************************
**	StaticShadowTexClass -- a static shadow texture shared by every object of its kind
***********************************************************************************************/

WorldShadowManager::StaticShadowTexClass::StaticShadowTexClass(void) :
	ObjectTypeID(0),
	ObjectOrientation(1.0f,0.0f,0.0f,0.0f),
	Texture(nullptr)
{
}


WorldShadowManager::StaticShadowTexClass::StaticShadowTexClass
(
	uint32					type_id,
	const Quaternion &	orientation,
	TextureClass *			tex
) :
	ObjectTypeID(type_id),
	ObjectOrientation(orientation),
	Texture(tex)
{
	if (Texture != nullptr) {
		Texture->Add_Ref();
	}
}


WorldShadowManager::StaticShadowTexClass::StaticShadowTexClass(const StaticShadowTexClass & that) :
	ObjectTypeID(that.ObjectTypeID),
	ObjectOrientation(that.ObjectOrientation),
	Texture(that.Texture)
{
	if (Texture != nullptr) {
		Texture->Add_Ref();
	}
}


const WorldShadowManager::StaticShadowTexClass &
WorldShadowManager::StaticShadowTexClass::operator = (const StaticShadowTexClass & that)
{
	if (this != &that) {
		ObjectTypeID = that.ObjectTypeID;
		ObjectOrientation = that.ObjectOrientation;
		REF_PTR_SET(Texture,that.Texture);
	}
	return *this;
}


WorldShadowManager::StaticShadowTexClass::~StaticShadowTexClass(void)
{
	REF_PTR_RELEASE(Texture);
}


/***********************************************************************************************
**	RegisteredCasterClass
***********************************************************************************************/

WorldShadowManager::RegisteredCasterClass::RegisteredCasterClass(void) :
	Object(nullptr),
	Shadow(nullptr),
	NearZ(-1.0f),
	FarZ(-1.0f)
{
}


/***********************************************************************************************
**	Lifetime
***********************************************************************************************/

void WorldShadowManager::Init(void)
{
	WWMEMLOG(MEM_GAMEDATA);

	Shutdown();

	ShadowMode			= SHADOW_MODE_NONE;
	AttenStart			= 25.0f;
	AttenEnd				= 40.0f;
	NormalIntensity	= 0.45f;
	DynamicResolution	= WORLD_SHADOW_DEFAULT_RESOLUTION;
	StaticResolution	= WORLD_SHADOW_DEFAULT_RESOLUTION;

	Generation			= 1;
	FrameStamp			= 0;
	TextureRenders		= 0;
	CacheHits			= 0;
	Evictions			= 0;
	MissingTargets		= 0;
	CasterRefusals		= 0;

	//	The slots exist from the start; the render targets inside them are made on demand,
	//	because at this point there may well not be a device to make one with.
	Size_Slot_Table(WORLD_SHADOW_DEFAULT_SLOTS);

	Initialized = true;
}


void WorldShadowManager::Shutdown(void)
{
	//	Every registered caster is dropped with its shadow; anything still holding one is
	//	going away with the world it belonged to.
	for (int i = 0; i < Casters.Count(); i ++) {
		Release_Object_Shadow(&Casters[i].Shadow);
	}
	Casters.Delete_All();

	Release_Resources();

	Slots.Delete_All();
	StaticTextures.Delete_All();

	Initialized = false;
}


void WorldShadowManager::Release_Resources(void)
{
	for (int i = 0; i < Slots.Count(); i ++) {
		if (Slots[i].Holder != nullptr) {
			Slots[i].Holder->Set_Render_Target(nullptr);
		}
		REF_PTR_RELEASE(Slots[i].RenderTarget);
		Slots[i].Reset();
	}

	Reset_Static_Shadow_Textures();
}


/***********************************************************************************************
**	Settings
***********************************************************************************************/

void WorldShadowManager::Set_Mode(int mode)
{
	if ((mode < 0) || (mode >= SHADOW_MODE_COUNT)) {
		return;
	}
	if (mode == ShadowMode) {
		return;
	}

	ShadowMode = mode;

	/*
	**	The mode decides how many rendered shadows may exist at once.  Stock Renegade made the
	**	same decision in the same place; it is kept because it is the one knob a player turns.
	*/
	switch (ShadowMode) {
		default:
		case SHADOW_MODE_NONE:
		case SHADOW_MODE_BLOBS:
			Set_Max_Simultaneous_Shadows(0);
			break;
		case SHADOW_MODE_BLOBS_PLUS:
			Set_Max_Simultaneous_Shadows(1);
			break;
		case SHADOW_MODE_HARDWARE:
			Set_Max_Simultaneous_Shadows(4);
			break;
	}

	//	Everything cached was cached for a different mode.
	Invalidate_All();
}


int WorldShadowManager::Get_Mode(void)
{
	return ShadowMode;
}


void WorldShadowManager::Set_Attenuation(float atten_start,float atten_end)
{
	if (atten_start < 0.0f) {
		atten_start = 0.0f;
	}
	if (atten_end < atten_start) {
		atten_end = atten_start;
	}
	AttenStart = atten_start;
	AttenEnd = atten_end;
}


void WorldShadowManager::Get_Attenuation(float * set_start,float * set_end)
{
	if (set_start != nullptr) {
		*set_start = AttenStart;
	}
	if (set_end != nullptr) {
		*set_end = AttenEnd;
	}
}


void WorldShadowManager::Set_Normal_Intensity(float intensity)
{
	if (intensity < 0.0f) {
		intensity = 0.0f;
	}
	if (intensity > 1.0f) {
		intensity = 1.0f;
	}
	NormalIntensity = intensity;
}


float WorldShadowManager::Get_Normal_Intensity(void)
{
	return NormalIntensity;
}


static unsigned int Clamp_Resolution(unsigned int res)
{
	unsigned int oksize = ::Find_POT(res);
	if (oksize > WORLD_SHADOW_MAX_RESOLUTION) {
		oksize = WORLD_SHADOW_MAX_RESOLUTION;
	}
	if (oksize < WORLD_SHADOW_MIN_RESOLUTION) {
		oksize = WORLD_SHADOW_MIN_RESOLUTION;
	}
	return oksize;
}


void WorldShadowManager::Set_Dynamic_Resolution(unsigned int res)
{
	unsigned int oksize = Clamp_Resolution(res);
	if (oksize == DynamicResolution) {
		return;
	}

	DynamicResolution = oksize;

	//	Every render target is the wrong size now.  Drop them; they are remade on demand at
	//	the new size, and every cached picture in them is invalid by definition.
	for (int i = 0; i < Slots.Count(); i ++) {
		if (Slots[i].Holder != nullptr) {
			Slots[i].Holder->Set_Render_Target(nullptr);
		}
		REF_PTR_RELEASE(Slots[i].RenderTarget);
		Slots[i].Reset();
	}
}


unsigned int WorldShadowManager::Get_Dynamic_Resolution(void)
{
	return DynamicResolution;
}


void WorldShadowManager::Set_Static_Resolution(unsigned int res)
{
	unsigned int oksize = Clamp_Resolution(res);
	if (oksize == StaticResolution) {
		return;
	}

	StaticResolution = oksize;

	//	The cached static shadow textures were rendered at the old size.
	Reset_Static_Shadow_Textures();
}


unsigned int WorldShadowManager::Get_Static_Resolution(void)
{
	return StaticResolution;
}


void WorldShadowManager::Set_Max_Simultaneous_Shadows(unsigned int count)
{
	if (count > WORLD_SHADOW_MAX_SLOTS) {
		count = WORLD_SHADOW_MAX_SLOTS;
	}
	Size_Slot_Table(count);
}


unsigned int WorldShadowManager::Get_Max_Simultaneous_Shadows(void)
{
	return (unsigned int)Slots.Count();
}


void WorldShadowManager::Size_Slot_Table(unsigned int count)
{
	//	Shrinking: whoever is holding a slot that is about to stop existing loses it first.
	for (int i = (int)count; i < Slots.Count(); i ++) {
		if (Slots[i].Holder != nullptr) {
			Slots[i].Holder->Set_Render_Target(nullptr);
		}
		REF_PTR_RELEASE(Slots[i].RenderTarget);
		Slots[i].Reset();
	}

	if ((int)count < Slots.Count()) {
		Slots.Set_Active((int)count);
	} else {
		while (Slots.Count() < (int)count) {
			Slots.Add(ShadowSlotClass());
		}
	}
}


/***********************************************************************************************
**	Render targets, held across frames
***********************************************************************************************/

TextureClass * WorldShadowManager::Create_Render_Target(unsigned int width,unsigned int height)
{
	if (!DX8Wrapper::Is_Initted() || DX8Wrapper::Is_Device_Lost()) {
		return nullptr;
	}

	/*
	**	Shadows are a single channel of darkness, so the smallest format the card will render
	**	into is the right one.  Falling all the way through to UNKNOWN means "whatever the
	**	display is using", which always works and costs the most.
	*/
	WW3DFormat format = WW3D_FORMAT_UNKNOWN;
	if (DX8Wrapper::Get_Current_Caps()->Support_Render_To_Texture_Format(WW3D_FORMAT_R3G3B2)) {
		format = WW3D_FORMAT_R3G3B2;
	} else if (DX8Wrapper::Get_Current_Caps()->Support_Render_To_Texture_Format(WW3D_FORMAT_R5G6B5)) {
		format = WW3D_FORMAT_R5G6B5;
	} else if (DX8Wrapper::Get_Current_Caps()->Support_Render_To_Texture_Format(WW3D_FORMAT_A4R4G4B4)) {
		format = WW3D_FORMAT_A4R4G4B4;
	} else if (DX8Wrapper::Get_Current_Caps()->Support_Render_To_Texture_Format(WW3D_FORMAT_X1R5G5B5)) {
		format = WW3D_FORMAT_X1R5G5B5;
	}

	TextureClass * texture = DX8Wrapper::Create_Render_Target((int)width,(int)height,format);
	if (texture != nullptr) {
		return texture;
	}

	if (format != WW3D_FORMAT_UNKNOWN) {
		return DX8Wrapper::Create_Render_Target((int)width,(int)height,WW3D_FORMAT_UNKNOWN);
	}
	return nullptr;
}


int WorldShadowManager::Find_Slot(const TexProjectClass * projector)
{
	if (projector == nullptr) {
		return -1;
	}
	for (int i = 0; i < Slots.Count(); i ++) {
		if (Slots[i].Holder == projector) {
			return i;
		}
	}
	return -1;
}


bool WorldShadowManager::Holds_Render_Target(const TexProjectClass * projector)
{
	int index = Find_Slot(projector);
	return ((index >= 0) && (Slots[index].RenderTarget != nullptr));
}


int WorldShadowManager::Allocate_Slot(TexProjectClass * projector)
{
	if (projector == nullptr) {
		return -1;
	}

	//	Already holding one.  This is the case the whole design exists for.
	int index = Find_Slot(projector);
	if (index >= 0) {
		Slots[index].LastFrame = FrameStamp;
		return index;
	}

	//	A slot nobody is using.
	for (int i = 0; i < Slots.Count(); i ++) {
		if (Slots[i].Holder == nullptr) {
			Slots[i].Reset();
			Slots[i].Holder = projector;
			Slots[i].LastFrame = FrameStamp;
			return i;
		}
	}

	/*
	**	Everything is held.  Take from whoever did not ask this frame, oldest first -- a
	**	shadow that has stopped being drawn has stopped needing a picture.
	*/
	int victim = -1;
	int oldest = FrameStamp;
	for (int i = 0; i < Slots.Count(); i ++) {
		if ((Slots[i].LastFrame != FrameStamp) && (Slots[i].LastFrame <= oldest)) {
			oldest = Slots[i].LastFrame;
			victim = i;
		}
	}

	/*
	**	Everyone asked this frame.  Then the one that loses is the one furthest from the
	**	camera, which is the same rule the projector pass uses to decide which shadows are
	**	worth drawing at all.
	*/
	if (victim < 0) {
		Vector3 camera_pos(0.0f,0.0f,0.0f);
		PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
		if (scene != nullptr) {
			camera_pos = scene->Get_Last_Camera_Position();
		}

		float farthest = (projector->Get_Bounding_Volume().Center - camera_pos).Length2();
		for (int i = 0; i < Slots.Count(); i ++) {
			if (Slots[i].Holder == nullptr) {
				continue;
			}
			float dist2 = (Slots[i].Holder->Get_Bounding_Volume().Center - camera_pos).Length2();
			if (dist2 > farthest) {
				farthest = dist2;
				victim = i;
			}
		}

		//	The requester is further away than everything already holding a slot.  It waits.
		if (victim < 0) {
			return -1;
		}
	}

	Release_Slot(victim);
	Slots[victim].Holder = projector;
	Slots[victim].LastFrame = FrameStamp;
	Evictions ++;
	return victim;
}


void WorldShadowManager::Release_Slot(int index)
{
	if ((index < 0) || (index >= Slots.Count())) {
		return;
	}

	if (Slots[index].Holder != nullptr) {
		Slots[index].Holder->Set_Render_Target(nullptr);
	}

	//	The render target itself stays; it is the expensive part and the next holder wants
	//	exactly the same thing.
	TextureClass * target = Slots[index].RenderTarget;
	Slots[index].Reset();
	Slots[index].RenderTarget = target;
}


bool WorldShadowManager::Acquire_Render_Target(TexProjectClass * projector)
{
	int index = Allocate_Slot(projector);
	if (index < 0) {
		return false;
	}

	ShadowSlotClass & slot = Slots[index];

	if (slot.RenderTarget == nullptr) {
		slot.RenderTarget = Create_Render_Target(DynamicResolution,DynamicResolution);
		if (slot.RenderTarget == nullptr) {
			//	No device, or the device would not give us one.  Counted, not asserted: this
			//	is the normal state on a dedicated server.
			MissingTargets ++;
			slot.Holder = nullptr;
			return false;
		}
		SET_REF_OWNER(slot.RenderTarget);
		slot.RenderTarget->Set_U_Addr_Mode(TextureClass::TEXTURE_ADDRESS_CLAMP);
		slot.RenderTarget->Set_V_Addr_Mode(TextureClass::TEXTURE_ADDRESS_CLAMP);
		slot.HasCommitted = false;
	}

	//	Set_Render_Target also makes it the projector's texture, which is what gets sampled.
	if (projector->Peek_Render_Target() != slot.RenderTarget) {
		projector->Set_Render_Target(slot.RenderTarget);
	}

	return true;
}


void WorldShadowManager::Release_Render_Target(TexProjectClass * projector)
{
	int index = Find_Slot(projector);
	if (index >= 0) {
		Release_Slot(index);
	}
}


void WorldShadowManager::Note_Texture_Rendered(TexProjectClass * projector)
{
	int index = Find_Slot(projector);
	if (index < 0) {
		return;
	}

	/*
	**	The projector clears its own dirty flag when it actually renders.  If it is still
	**	dirty then nothing was drawn -- no receiver was found, or there was no render target
	**	-- and the picture in the slot is still the old one, so the stamp must not move.
	*/
	if (projector->Needs_Render_Target()) {
		return;
	}

	Slots[index].Committed = Slots[index].Pending;
	Slots[index].HasCommitted = true;
	TextureRenders ++;
}


void WorldShadowManager::Begin_Frame(void)
{
	FrameStamp ++;
	TextureRenders = 0;
	CacheHits = 0;
}


/***********************************************************************************************
**	Invalidation
***********************************************************************************************/

void WorldShadowManager::Invalidate_All(void)
{
	Generation ++;
	if (Generation == 0) {
		Generation = 1;
	}
}


void WorldShadowManager::Invalidate_Object(const PhysClass * obj)
{
	if (obj == nullptr) {
		return;
	}

	for (int i = 0; i < Slots.Count(); i ++) {
		if (Slots[i].Holder == nullptr) {
			continue;
		}
		if (Slots[i].Holder->Get_Projection_Object_ID() == (void *)obj) {
			Slots[i].HasCommitted = false;
		}
	}
}


void WorldShadowManager::Build_Stamp
(
	const PhysClass &		obj,
	const Vector3 &		light_vector,
	bool						perspective,
	ShadowStampClass *	stamp
)
{
	WWASSERT(stamp != nullptr);

	stamp->LightVector = light_vector;
	stamp->Orientation = Build_Quaternion(obj.Get_Transform());
	stamp->Model = ((PhysClass &)obj).Peek_Model();
	stamp->AnimFrame = (stamp->Model != nullptr) ? stamp->Model->Get_Animation_Frame() : 0.0f;
	stamp->Generation = Generation;
	stamp->Perspective = perspective;
}


bool WorldShadowManager::Texture_Is_Stale(TexProjectClass * projector,const ShadowStampClass & stamp)
{
	/*
	**	Claim a slot now rather than at render time, so that the stamp describing what the
	**	next picture would be is recorded before anything renders.  Allocation here costs
	**	nothing on the device: the render target inside the slot is made later, on the frame
	**	the shadow is actually drawn.
	*/
	int index = Find_Slot(projector);
	if (index < 0) {
		index = Allocate_Slot(projector);
	}
	if (index < 0) {
		//	Every slot belongs to a shadow nearer the camera than this one.  It is stale by
		//	definition and will stay that way until one frees up.
		return true;
	}

	ShadowSlotClass & slot = Slots[index];
	slot.Pending = stamp;
	slot.LastFrame = FrameStamp;

	if ((slot.RenderTarget == nullptr) || (!slot.HasCommitted)) {
		return true;
	}

	if (slot.Committed.Matches(stamp)) {
		CacheHits ++;
		return false;
	}
	return true;
}


/***********************************************************************************************
**	The one per-object shadow policy
***********************************************************************************************/

void WorldShadowManager::Update_Object_Shadow
(
	PhysClass &				obj,
	DynTexProjectClass **	shadow_ptr,
	float						near_z,
	float						far_z,
	bool						force_blob_box,
	const Vector3 &		blob_box_scale
)
{
	WWASSERT(shadow_ptr != nullptr);

	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
	if (scene == nullptr) {
		Release_Object_Shadow(shadow_ptr);
		return;
	}

	RenderObjClass * model = obj.Peek_Model();

	/*
	**	Which box the shadow is projected from.  A blob is a smear the size of the object's
	**	footprint; a real shadow is the object rendered from the light.
	*/
	bool use_blob = (ShadowMode == SHADOW_MODE_BLOBS) ||
						 ((ShadowMode == SHADOW_MODE_BLOBS_PLUS) && (!obj.Is_Force_Projection_Shadow_Enabled()));

	AABoxClass objbox;
	if (use_blob) {
		obj.Get_Shadow_Blob_Box(&objbox);
	} else if (force_blob_box) {
		obj.Get_Shadow_Blob_Box(&objbox);
		objbox.Extent.Scale(blob_box_scale);
	} else if (model != nullptr) {
		model->Get_Obj_Space_Bounding_Box(objbox);
	} else {
		objbox.Center.Set(0.0f,0.0f,0.0f);
		objbox.Extent.Set(0.0f,0.0f,0.0f);
	}

	/*
	**	Far enough away and the shadow is not worth having at all.  Checked before anything is
	**	allocated so a level full of distant vehicles costs nothing.
	*/
	Vector3 position;
	Matrix3D::Transform_Vector(obj.Get_Transform(),objbox.Center,&position);

	float shutoff = AttenEnd * 1.3f;
	float shadow_dist2 = (position - scene->Get_Last_Camera_Position()).Length2();

	if (	(obj.Do_Any_Effects_Suppress_Shadows()) ||
			(!obj.Is_Shadow_Generation_Enabled()) ||
			(ShadowMode == SHADOW_MODE_NONE) ||
			(shadow_dist2 > (shutoff * shutoff)) ||
			(model == nullptr) ||
			(model->Is_Hidden()) ||
			(objbox.Extent.Length2() < 0.1f) )
	{
		Release_Object_Shadow(shadow_ptr);
		return;
	}

	/*
	**	Find the light.  Only the sun casts dynamic shadows: local shadow casting was disabled
	**	in stock Renegade and nothing has asked for it back, so there is one light here and it
	**	is orthographic, which is exactly the case a cached picture is correct for.
	*/
	if (!obj.Is_In_The_Sun()) {
		//	Out of the sun.  Fade whatever is there rather than snapping it off.
		DynTexProjectClass * shadow = *shadow_ptr;
		if (shadow != nullptr) {
			shadow->Set_Intensity(0.0f);
			if (!shadow->Is_Intensity_Zero()) {
				shadow->Update_Projection(objbox,obj.Get_Transform(),near_z,far_z);
			} else {
				Release_Object_Shadow(shadow_ptr);
			}
		}
		return;
	}

	Vector3 sunlight;
	scene->Get_Sun_Light_Vector(&sunlight);

	/*
	**	Make the projector if this object does not have one yet.
	*/
	DynTexProjectClass * shadow = *shadow_ptr;
	if (shadow == nullptr) {
		shadow = NEW_REF(DynTexProjectClass,(&obj));
		shadow->Enable_Attenuation(true);
		shadow->Enable_Depth_Gradient(true);
		shadow->Enable_Affect_Dynamic_Objects(false);
		shadow->Set_Intensity(NormalIntensity,true);
		shadow->Peek_Material_Pass()->Enable_On_Translucent_Meshes(false);

		scene->Add_Dynamic_Texture_Projector(shadow);
		*shadow_ptr = shadow;
	}

	LightClass * sun = scene->Get_Sun_Light();
	shadow->Enable_Perspective(false);
	shadow->Set_Light_Source_ID((uintptr_t)sun);
	shadow->Set_Light_Vector(sunlight);
	if (sun != nullptr) {
		sun->Release_Ref();
	}

	//	The projection matrix is cheap and depends on where the object is, so it is always
	//	rebuilt.  The texture is the expensive half and is the thing being cached.
	shadow->Update_Projection(objbox,obj.Get_Transform(),near_z,far_z);
	shadow->Set_Intensity(NormalIntensity);

	if (use_blob) {
		/*
		**	A blob needs no render target and no rendering.  If this object was casting a real
		**	shadow a moment ago, it gives back the slot it was holding.
		*/
		Release_Render_Target(shadow);

		TextureClass * blob = PhysResourceMgrClass::Get_Shadow_Blob_Texture();
		if (blob != nullptr) {
			shadow->Set_Texture(blob);
			blob->Release_Ref();
		}
		shadow->Set_Texture_Dirty(false);
		return;
	}

	ShadowStampClass stamp;
	Build_Stamp(obj,sunlight,false,&stamp);
	shadow->Set_Texture_Dirty(Texture_Is_Stale(shadow,stamp));
}


void WorldShadowManager::Release_Object_Shadow(DynTexProjectClass ** shadow_ptr)
{
	if ((shadow_ptr == nullptr) || (*shadow_ptr == nullptr)) {
		return;
	}

	DynTexProjectClass * shadow = *shadow_ptr;
	Release_Render_Target(shadow);

	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
	if ((scene != nullptr) && (scene->Contains(shadow))) {
		scene->Remove_Dynamic_Texture_Projector(shadow);
	}

	shadow->Release_Ref();
	*shadow_ptr = nullptr;
}


/***********************************************************************************************
**	Casters that are not MovePhysClass or DynamicAnimPhysClass
***********************************************************************************************/

bool WorldShadowManager::Register_Caster(PhysClass * obj,float near_z,float far_z)
{
	if (obj == nullptr) {
		return false;
	}
	if (Is_Registered_Caster(obj)) {
		return true;
	}

	if (Casters.Count() >= WORLD_SHADOW_MAX_REGISTERED_CASTERS) {
		//	Refused, and said so.  A world system that wants more shadows than this is asking
		//	for something the render-target budget cannot pay for anyway.
		CasterRefusals ++;
		return false;
	}

	RegisteredCasterClass caster;
	caster.Object = obj;
	caster.Shadow = nullptr;
	caster.NearZ = near_z;
	caster.FarZ = far_z;
	Casters.Add(caster);

	obj->Enable_Shadow_Generation(true);
	return true;
}


bool WorldShadowManager::Unregister_Caster(PhysClass * obj)
{
	for (int i = 0; i < Casters.Count(); i ++) {
		if (Casters[i].Object == obj) {
			Release_Object_Shadow(&Casters[i].Shadow);
			Casters.Delete(i);
			return true;
		}
	}
	return false;
}


bool WorldShadowManager::Is_Registered_Caster(const PhysClass * obj)
{
	for (int i = 0; i < Casters.Count(); i ++) {
		if (Casters[i].Object == obj) {
			return true;
		}
	}
	return false;
}


int WorldShadowManager::Get_Registered_Caster_Count(void)
{
	return Casters.Count();
}


void WorldShadowManager::Timestep(float /* dt */)
{
	if (!Initialized) {
		return;
	}

	static const Vector3 no_blob_scale(1.0f,1.0f,1.0f);

	for (int i = 0; i < Casters.Count(); i ++) {
		if (Casters[i].Object == nullptr) {
			continue;
		}
		Update_Object_Shadow(*Casters[i].Object,&Casters[i].Shadow,
									Casters[i].NearZ,Casters[i].FarZ,false,no_blob_scale);
	}
}


/***********************************************************************************************
**	Shared static shadow resources
***********************************************************************************************/

TextureClass * WorldShadowManager::Peek_Static_Shadow_Texture
(
	uint32					type_id,
	const Quaternion &	orientation
)
{
	/*
	**	Two objects of the same kind facing within a few degrees of each other cast shadows a
	**	player will never tell apart, so they share the picture.  This is the whole of "shared
	**	static shadow resources": a hundred identical trees are one texture.
	*/
	for (int i = 0; i < StaticTextures.Count(); i ++) {
		if (StaticTextures[i].ObjectTypeID != type_id) {
			continue;
		}

		Quaternion delta = StaticTextures[i].ObjectOrientation * Inverse(orientation);
		if (delta.W > WORLD_SHADOW_STATIC_COS_HALF_THETA) {
			return StaticTextures[i].Texture;
		}
	}
	return nullptr;
}


void WorldShadowManager::Add_Static_Shadow_Texture
(
	uint32					type_id,
	const Quaternion &	orientation,
	TextureClass *			tex
)
{
	if (tex == nullptr) {
		return;
	}
	StaticTextures.Add(StaticShadowTexClass(type_id,orientation,tex));
}


void WorldShadowManager::Remove_Static_Shadow_Texture(TextureClass * tex)
{
	for (int i = 0; i < StaticTextures.Count(); i ++) {
		if (StaticTextures[i].Texture == tex) {
			StaticTextures.Delete(i);
			return;
		}
	}
}


void WorldShadowManager::Reset_Static_Shadow_Textures(void)
{
	StaticTextures.Delete_All();
}


int WorldShadowManager::Get_Static_Shadow_Texture_Count(void)
{
	return StaticTextures.Count();
}


/***********************************************************************************************
**	Accounting
***********************************************************************************************/

int WorldShadowManager::Get_Render_Target_Count(void)
{
	int count = 0;
	for (int i = 0; i < Slots.Count(); i ++) {
		if (Slots[i].RenderTarget != nullptr) {
			count ++;
		}
	}
	return count;
}


int WorldShadowManager::Get_Held_Slot_Count(void)
{
	int count = 0;
	for (int i = 0; i < Slots.Count(); i ++) {
		if (Slots[i].Holder != nullptr) {
			count ++;
		}
	}
	return count;
}


int WorldShadowManager::Get_Texture_Render_Count(void)
{
	return TextureRenders;
}


int WorldShadowManager::Get_Cache_Hit_Count(void)
{
	return CacheHits;
}


int WorldShadowManager::Get_Eviction_Count(void)
{
	return Evictions;
}


int WorldShadowManager::Get_Missing_Target_Count(void)
{
	return MissingTargets;
}


int WorldShadowManager::Get_Caster_Refusal_Count(void)
{
	return CasterRefusals;
}


uint32 WorldShadowManager::Get_Generation(void)
{
	return Generation;
}
