#include "general.h"
#include "PhysicsSceneClass.h"
#include "LightEnvironmentClass.h"
#include "RenderInfoClass.h"
#include "CameraClass.h"
#include "StaticPhysClass.h"
#include "TexProjectClass.h"
#include "PhysTexProjectClass.h"
#include "engine_3dre.h"
#include "DX8Wrapper.h"
#include "WW3D.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "StaticAnimPhysDefClass.h"
#include "StaticAnimPhysClass.h"
#include "Quaternion.h"
#include "LightClass.h"
#include "projector.h"
#include "Quaternion.h"
#include "MaterialPassClass.h"
#include "CullSystemClass.h"
#include "AABTreeCullSystemClass.h"
#include "GridCullSystemClass.h"
#include "SimpleEffectClass.h"
#include "StaticAABTreeCullClass.h"
#include "CastResultStruct.h"
#include "physcoltest.h"
#include "PhysGridCullClass.h"
#include "CameraShakeSystemClass.h"
#include "DynamicPhysClass.h"
#include "PhysDecalSysClass.h"

int Find_POT(int num);
const int		SHADOW_CLIP_FAR							= 500;
const int		STATIC_PROJECTOR_RESOLUTION			= 256;
const float		STATIC_SHADOW_INTENSITY					= 0.6f;
const float		MIN_STATIC_SHADOW_COS_HALF_THETA		= cos(DEG_TO_RADF(10.0f)/2.0f);
const int		DEFAULT_MAX_DYNAMIC_SHADOWS			= 6;
const int		DEFAULT_DYNAMIC_SHADOW_RESOLUTION	= 256;
class StaticShadowTexMgrClass
{
public:
	StaticShadowTexMgrClass(void);
	virtual ~StaticShadowTexMgrClass(void);
	void						Reset(void);
	TextureClass *			Peek_Shadow_Texture(uint32 obj_type_id,const Quaternion & orientation);
	void						Add_Shadow_Texture(uint32 obj_type_id,const Quaternion & orientation,TextureClass * tex);
	void						Remove_Shadow_Texture(TextureClass * tex);
private:
	class	 ShadowTexClass
	{
	public:
		ShadowTexClass(void);
		ShadowTexClass(uint32 obj_type_id,const Quaternion & orientation,TextureClass * tex);
		ShadowTexClass(const ShadowTexClass & that);
		~ShadowTexClass(void);
		const ShadowTexClass & operator = (const ShadowTexClass &);
		bool					operator == (const ShadowTexClass &)						{ return false; }
		bool					operator != (const ShadowTexClass & that)					{ return true; }
		uint32				ObjectTypeID;
		Quaternion			ObjectOrientation;
		TextureClass *		Texture;
	};
	DynamicVectorClass<ShadowTexClass> ShadowTextures;
};
REF_DEF1(_StaticShadowTexMgr, StaticShadowTexMgrClass, 0x00855480);
class DynamicShadowTexMgrClass
{
public:
	DynamicShadowTexMgrClass(void);
	virtual ~DynamicShadowTexMgrClass(void);
	void Set_Max_Simultaneous_Shadows(unsigned int max);
	unsigned int Get_Max_Simultaneous_Shadows(void);
	void Set_Shadow_Resolution(unsigned int size);
	unsigned int Get_Shadow_Resolution(void);
	void Per_Frame_Reset(void);
	void Assign_Render_Target_Texture(TexProjectClass * tex_proj);
private:
	TextureClass *Allocate_Render_Target_Texture(void);
	unsigned int CurShadow;
	unsigned int TextureResolution;
	SimpleVecClass<TextureClass *> ShadowTextures;
};
REF_DEF1(_DynamicShadowTexMgr, DynamicShadowTexMgrClass, 0x008554A0);
static TextureClass* Create_Projector_Render_Target(unsigned w,unsigned h)
{
	WW3DFormat format=WW3D_FORMAT_UNKNOWN;
	if (ShaderCaps::SupportRenderToTextureFormat[WW3D_FORMAT_R3G3B2]) format=WW3D_FORMAT_R3G3B2;
	else if (ShaderCaps::SupportRenderToTextureFormat[WW3D_FORMAT_R5G6B5]) format=WW3D_FORMAT_R5G6B5;
	else if (ShaderCaps::SupportRenderToTextureFormat[WW3D_FORMAT_A4R4G4B4]) format=WW3D_FORMAT_A4R4G4B4;
	else if (ShaderCaps::SupportRenderToTextureFormat[WW3D_FORMAT_X1R5G5B5]) format=WW3D_FORMAT_X1R5G5B5;
	TextureClass* texture = DX8Wrapper::Create_Render_Target(w,h,format);
	if (texture) return texture;
	if (format!=WW3D_FORMAT_UNKNOWN)
	{
		format=WW3D_FORMAT_UNKNOWN;
		return DX8Wrapper::Create_Render_Target(w,h,format);
	}
	return NULL;
}

void DynamicShadowTexMgrClass::Set_Max_Simultaneous_Shadows(unsigned int max)
{
	int curlen = ShadowTextures.Length();
	int i;
	for (i=max;i<curlen; i++)
	{
		REF_PTR_RELEASE(ShadowTextures[i]);
	}
	ShadowTextures.Resize(max);
	if (curlen>ShadowTextures.Length()) curlen=ShadowTextures.Length();
	for (i=0; i<curlen; i++)
	{
		if (!ShadowTextures[i])
		{
			ShadowTextures[i] = Allocate_Render_Target_Texture();
		}
	}
	for (; i<ShadowTextures.Length(); i++)
	{
		ShadowTextures[i] = Allocate_Render_Target_Texture();
	}
}

unsigned int DynamicShadowTexMgrClass::Get_Max_Simultaneous_Shadows(void)
{
	return ShadowTextures.Length();
}

void DynamicShadowTexMgrClass::Set_Shadow_Resolution(unsigned int res)
{
	unsigned int oksize = Find_POT(res);
	if (oksize > 256)
	{
		oksize = 256;
	}
	if (oksize < 16)
	{
		oksize = 16;
	}
	if (oksize != TextureResolution)
	{
		int i;
		for (i=0; i<ShadowTextures.Length(); i++) 
		{
			REF_PTR_RELEASE(ShadowTextures[i]);
		}
		TextureResolution = oksize;
		for (i=0; i<ShadowTextures.Length(); i++)
		{
			ShadowTextures[i] = Allocate_Render_Target_Texture();
		}
	}
}

unsigned int DynamicShadowTexMgrClass::Get_Shadow_Resolution(void)
{
	return TextureResolution;
}

void DynamicShadowTexMgrClass::Per_Frame_Reset(void)
{
	CurShadow = 0;
}

void DynamicShadowTexMgrClass::Assign_Render_Target_Texture(TexProjectClass * tex_proj)
{
	if (CurShadow < (unsigned int)ShadowTextures.Length())
	{
		if (ShadowTextures[CurShadow] == NULL)
		{
			ShadowTextures[CurShadow] = Allocate_Render_Target_Texture();
		}
		tex_proj->Set_Render_Target(ShadowTextures[CurShadow]);
		CurShadow++;
	}
}

TextureClass * DynamicShadowTexMgrClass::Allocate_Render_Target_Texture(void)
{
	TextureClass * texture=Create_Projector_Render_Target(DX8Wrapper::ShadowWidth,DX8Wrapper::ShadowHeight);
	if (texture != NULL)
	{
		texture->UAddressMode = TextureClass::TEXTURE_ADDRESS_CLAMP;
		texture->VAddressMode = TextureClass::TEXTURE_ADDRESS_CLAMP;
	}
	return texture;
}

TextureClass * StaticShadowTexMgrClass::Peek_Shadow_Texture(uint32 obj_type_id,const Quaternion &orientation)
{
	for (int i=0; i<ShadowTextures.Count(); i++)
	{
		Quaternion deltaq = ShadowTextures[i].ObjectOrientation * Inverse(orientation);
		if ((ShadowTextures[i].ObjectTypeID == obj_type_id) && (deltaq.W > MIN_STATIC_SHADOW_COS_HALF_THETA))
		{
			return ShadowTextures[i].Texture;
		}
	}
	return NULL;
}

void StaticShadowTexMgrClass::Add_Shadow_Texture(uint32 obj_type_id,const Quaternion &orientation,TextureClass *tex)
{
	ShadowTexClass record(obj_type_id,orientation,tex);
	ShadowTextures.Add(record);
}

StaticShadowTexMgrClass::ShadowTexClass::ShadowTexClass(void) :
	ObjectTypeID(0),
	ObjectOrientation(1),
	Texture(NULL)
{
}

StaticShadowTexMgrClass::ShadowTexClass::ShadowTexClass(uint32 obj_type_id,const Quaternion & orientation,TextureClass * tex) :
	ObjectTypeID(obj_type_id),
	ObjectOrientation(orientation),
	Texture(NULL)
{
	REF_PTR_SET(Texture,tex);
}

	StaticShadowTexMgrClass::ShadowTexClass::ShadowTexClass(const ShadowTexClass & that) :
	ObjectTypeID(0),
	ObjectOrientation(1),
	Texture(NULL)
{
	*this = that;
}

const StaticShadowTexMgrClass::ShadowTexClass &
StaticShadowTexMgrClass::ShadowTexClass::operator = (const ShadowTexClass & that)
{
	ObjectTypeID = that.ObjectTypeID;
	ObjectOrientation = that.ObjectOrientation;
	REF_PTR_SET(Texture,that.Texture);
	return *this;
}

StaticShadowTexMgrClass::ShadowTexClass::~ShadowTexClass(void)
{
	REF_PTR_RELEASE(Texture);
}

void PhysicsSceneClass::Set_Shadow_Mode(ShadowEnum shadow_mode)
{
	if (((int)shadow_mode >= 0) && ((int)shadow_mode < SHADOW_MODE_COUNT)) {
		if (ShadowMode!=shadow_mode) {
			ShadowMode = shadow_mode;
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
		}
	}
}

void PhysicsSceneClass::Set_Max_Simultaneous_Shadows(unsigned int count)
{
	_DynamicShadowTexMgr.Set_Max_Simultaneous_Shadows(count);
}

void PhysicsSceneClass::Apply_Projectors
(
	const CameraClass &	camera
)
{
	Vector3 view_pos;
	Vector3 view_dir;
	camera.Get_Transform().Get_Translation(&view_pos);
	camera.Get_Transform().Get_Z_Vector(&view_dir);
	view_dir = -view_dir;
	if (StaticProjectorsEnabled)
	{
		StaticProjectorCullingSystem->Reset_Collection();
		StaticProjectorCullingSystem->Collect_Objects(camera.Get_Frustum());
		TexProjectClass * static_projector = (TexProjectClass *)StaticProjectorCullingSystem->Get_First_Collected_Object_Internal();
		while (static_projector != NULL)
		{
			if (!static_projector->Is_Intensity_Zero())
			{
				float attenuation = 1.0f;
				if (static_projector->Is_Attenuation_Enabled())
				{
					attenuation = Compute_Projector_Attenuation(static_projector,view_pos,view_dir);
					static_projector->Set_Attenuation(attenuation);
				}
				if (attenuation > 0.0f)
				{
					Apply_Projector_To_Objects(static_projector,camera);
				}
			}
			static_projector = (TexProjectClass *)StaticProjectorCullingSystem->Get_Next_Collected_Object_Internal(static_projector);
		}
	}
	MultiListClass<TexProjectClass> rt_projector_list;
	unsigned int count = 0;
	if (DynamicProjectorsEnabled)
	{
		DynamicProjectorCullingSystem->Reset_Collection();
		DynamicProjectorCullingSystem->Collect_Objects(camera.Get_Frustum());
		TexProjectClass * dynamic_projector = (TexProjectClass *)DynamicProjectorCullingSystem->Get_First_Collected_Object_Internal();
		while (dynamic_projector != NULL)
		{
			if (!dynamic_projector->Is_Intensity_Zero())
			{
				float attenuation = 1.0f;
				if (dynamic_projector->Is_Attenuation_Enabled())
				{
					attenuation = Compute_Projector_Attenuation(dynamic_projector,view_pos,view_dir);
					dynamic_projector->Set_Attenuation(attenuation);
				}
				if (attenuation > 0.0f)
				{
					if (dynamic_projector->Needs_Render_Target())
					{
						rt_projector_list.Add(dynamic_projector);
						count++;
					}
					else
					{
						Apply_Projector_To_Objects(dynamic_projector,camera);
					}
				}
			}
			dynamic_projector = (TexProjectClass *)DynamicProjectorCullingSystem->Get_Next_Collected_Object_Internal(dynamic_projector);
		}
	}
	Vector3 cam_pos;
	camera.Get_Transform().Get_Translation(&cam_pos);
	MultiListIterator<TexProjectClass> it(&rt_projector_list);
	while (count > _DynamicShadowTexMgr.Get_Max_Simultaneous_Shadows())
	{
		it.First();
		TexProjectClass * farthest_shadow = it.Peek_Obj();
		float farthest_dist = (farthest_shadow->Get_Bounding_Volume().Center - cam_pos).Length2();
		it.Next();
 		while (!it.Is_Done())
		{
			float dist = (it.Peek_Obj()->Get_Bounding_Volume().Center - cam_pos).Length2();
			if (dist > farthest_dist)
			{
				farthest_dist = dist;
				farthest_shadow = it.Peek_Obj();
			}
			it.Next();
		}
		rt_projector_list.Remove(farthest_shadow);
		farthest_shadow->Set_Render_Target(NULL);
		count--;
	}
	_DynamicShadowTexMgr.Per_Frame_Reset();
	it.First();
	while (!it.Is_Done())
	{
		TexProjectClass * projector = it.Peek_Obj();
		_DynamicShadowTexMgr.Assign_Render_Target_Texture(projector);
		if (projector->Peek_Render_Target() != NULL)
		{
			Apply_Projector_To_Objects(projector,camera);
		}
		it.Next();
	}
	/*
	// The next line of code is VERY WRONG. DX8Wrapper::Set_Render_Target behavior has changed
	// Setting the render target to NULL moves up the RT stack, so when there's already an active
	// RT, it jumps to one above proper RT causing a stack unbalance (and very unexpected results)
	// DX8Wrapper::Set_Render_Target((IDirect3DSurface9 *)NULL,false);
	*/
}

float PhysicsSceneClass::Compute_Projector_Attenuation(TexProjectClass * dynamic_projector,const Vector3 & view_pos,const Vector3 & view_dir)
{
	Vector3 r;
	Vector3::Subtract(dynamic_projector->Get_Bounding_Volume().Center,view_pos,&r);
	float dist = Vector3::Dot_Product(r,view_dir);
	if (dist > ShadowAttenEnd) {
		return 0.0f;
	}
	if (dist < ShadowAttenStart) {
		return 1.0f;
	}
	return 1.0f - (dist - ShadowAttenStart) / (ShadowAttenEnd - ShadowAttenStart);
}

void PhysicsSceneClass::Apply_Projector_To_Objects
(
	TexProjectClass * tex_proj,
	const CameraClass &	camera
)
{
	bool projector_update_needed = false;
	SimpleEffectClass *effect = new SimpleEffectClass(tex_proj->Peek_Material_Pass());
	effect->autoRemove = true;
	if (tex_proj->Is_Affect_Static_Objects_Enabled())
	{
		StaticCullingSystem->Reset_Collection();
		StaticCullingSystem->Collect_Objects(tex_proj->Get_Bounding_Volume());
		StaticPhysClass * static_obj = (StaticPhysClass *)StaticCullingSystem->Get_First_Collected_Object_Internal();
		while (static_obj)
		{
			if (VisibleStaticObjectList.Contains(static_obj) || VisibleWSMeshList.Is_In_List(static_obj))
			{
				CastResultStruct castResult;
				PhysOBBoxCollisionTestClass collisionTest(tex_proj->Get_Bounding_Volume(), Vector3(0, 0, 0), &castResult, 1);
				collisionTest.CollidedPhysObj = NULL;
				collisionTest.CollisionGroup = 15;
				collisionTest.CheckStaticObjs = true;
				collisionTest.CheckDynamicObjs = true;
				static_obj->Cast_OBBox(collisionTest);
				
				if (castResult.StartBad)
				{
					static_obj->Add_Effect_To_Me(effect);
					projector_update_needed = true;
				}
			}
			static_obj = (StaticPhysClass *)StaticCullingSystem->Get_Next_Collected_Object_Internal(static_obj);
		}
	}
	if (tex_proj->Is_Affect_Dynamic_Objects_Enabled())
	{
		DynamicCullingSystem->Reset_Collection();
		DynamicCullingSystem->Collect_Objects(tex_proj->Get_Bounding_Volume());
		PhysClass * dyn_obj = (PhysClass *)DynamicCullingSystem->Get_First_Collected_Object_Internal();
		while (dyn_obj)
		{
			if ((VisibleDynamicObjectList.Contains(dyn_obj)) && (dyn_obj != tex_proj->Get_Projection_Object_ID()))
			{
				dyn_obj->Add_Effect_To_Me(effect);
				projector_update_needed = true;
			}
			dyn_obj = (PhysClass *)DynamicCullingSystem->Get_Next_Collected_Object_Internal(dyn_obj);
		}
	}
	if (projector_update_needed)
	{
		tex_proj->Pre_Render_Update(camera.Get_Transform());
	}
	REF_PTR_RELEASE(effect);
}

void PhysicsSceneClass::Render_Object(RenderInfoClass & context,PhysClass * obj)
{
	if ((obj->Peek_Model() == NULL) || (obj->Is_Rendering_Disabled()))
	{
		return;
	}
	bool do_lighting = ((obj->Is_Pre_Lit() == false) &&  (obj->Peek_Model()->Is_Not_Hidden_At_All()));
	if (do_lighting)
	{
		LightEnvironmentClass & light_env = *(obj->Get_Static_Lighting_Environment());
		light_env.Pre_Render_Update(context.Camera.Get_Transform());
		context.light_environment = &light_env;
	}
	else
	{
		static LightEnvironmentClass _emptylightenvironment;
		Vector3 v = Vector3(0,0,0);
		Vector3 v2 = Vector3(1,1,1);
		_emptylightenvironment.Reset(v,v2);
		context.light_environment = &_emptylightenvironment;
	}
	{
		obj->Render(context);
	}
	if (do_lighting)
	{
		context.light_environment = NULL;
	}
}

void PhysicsSceneClass::Render_Objects(
	RenderInfoClass& rinfo,
	RefMultiListClass<PhysClass> * static_ws_list,
	RefMultiListClass<PhysClass> * static_list,
	RefMultiListClass<PhysClass> * dyn_list)
{
	RefMultiListIterator<PhysClass> it(static_ws_list);

	if (WW3D::MeshDrawMode!=WW3D::MESH_DRAW_MODE_NONE)
	{
		for (it.First(static_ws_list); !it.Is_Done(); it.Next())
		{
			Render_Object(rinfo,it.Peek_Obj());
		}
		for (it.First(static_list); !it.Is_Done(); it.Next())
		{
			Render_Object(rinfo,it.Peek_Obj());
		}
	}
	for (it.First(dyn_list); !it.Is_Done(); it.Next())
	{
		Render_Object(rinfo,it.Peek_Obj());
	}
}

float PhysicsSceneClass::Get_Shadow_Normal_Intensity(void)
{
	return ShadowNormalIntensity;
}

void StaticShadowTexMgrClass::Reset(void)
{
	ShadowTextures.Delete_All();
}

void PhysicsSceneClass::Customized_Render(RenderInfoClass & rinfo)
{
	Render_Objects(rinfo,&VisibleWSMeshList,&VisibleStaticObjectList,&VisibleDynamicObjectList);
}

RENEGADE_FUNCTION
void StaticAnimPhysClass::Set_Shadow(TexProjectClass *)
AT1(0x0064FE50);

void PhysicsSceneClass::Get_Sun_Light_Vector(Vector3 *v)
{
	SunLight->Validate_Transform();
	v->X = SunLight->Get_Transform()[0][3] + SunLight->Get_Transform()[0][2];
	v->Y = SunLight->Get_Transform()[1][3] + SunLight->Get_Transform()[1][2];
	v->Z = SunLight->Get_Transform()[2][3] + SunLight->Get_Transform()[2][2];
}

Quaternion Build_Quaternion(const Matrix3D & mat);
void PhysicsSceneClass::Setup_Static_Directional_Shadow
(
	StaticAnimPhysClass & obj,
	const Vector3 & light_dir,
	TextureClass * render_target
)
{
	const StaticAnimPhysDefClass * def = obj.Get_Definition();
	if (def == NULL)
	{
		return;
	}
	int type_id = def->Get_ID();
	Quaternion obj_orientation(1);
	if (def->shadowIgnoresZRotation == false)
	{
		obj_orientation = Build_Quaternion(obj.Get_Transform());
	}
	PhysTexProjectClass * shadow_projector = new PhysTexProjectClass();
	shadow_projector->Set_Texture_Size(STATIC_PROJECTOR_RESOLUTION);
	shadow_projector->Set_Intensity(def->shadowIntensity,true);
	if (def->shadowIsAdditive)
	{
		shadow_projector->Init_Additive();
	}
	else
	{
		shadow_projector->Init_Multiplicative();
		shadow_projector->Set_Intensity(Get_Shadow_Normal_Intensity(),true);
	}
	shadow_projector->Compute_Ortho_Projection(&obj,light_dir,def->shadowNearZ,def->shadowFarZ);
	shadow_projector->Enable_Attenuation(true);
	shadow_projector->Enable_Depth_Gradient(false);
	shadow_projector->Peek_Material_Pass()->EnableOnTranslucentMeshes = false;
	TextureClass * existing_texture = _StaticShadowTexMgr.Peek_Shadow_Texture(type_id,obj_orientation);
	if (existing_texture != NULL)
	{
		shadow_projector->Set_Texture(existing_texture);
	}
	if (existing_texture == NULL)
	{
		shadow_projector->Set_Render_Target(render_target);
		shadow_projector->Compute_Texture(&obj,def->shadowIsAdditive);
		SurfaceClass * surf = render_target->Get_Surface_Level(0);
		SurfaceClass::SurfaceDescription desc;
		surf->Get_Description(desc);
		SurfaceClass * new_surf = new SurfaceClass(desc.Width,desc.Height,desc.Format);
		DX8Wrapper::D3DDevice->GetRenderTargetData(surf->Peek_D3D_Surface(), new_surf->Peek_D3D_Surface());
		TextureClass * new_texture = new TextureClass(new_surf,TextureClass::MIP_LEVELS_ALL);
		shadow_projector->Set_Render_Target(NULL);
		shadow_projector->Set_Texture(new_texture);
		REF_PTR_RELEASE(surf);
		REF_PTR_RELEASE(new_surf);
		REF_PTR_RELEASE(new_texture);
		_StaticShadowTexMgr.Add_Shadow_Texture(type_id,obj_orientation,shadow_projector->Peek_Texture());
	}
	obj.Set_Shadow(shadow_projector);
	REF_PTR_RELEASE(shadow_projector);
}

void PhysicsSceneClass::Generate_Static_Shadow_Projectors(void)
{
	if (!StaticProjectorsDirty) return;
	if (DX8Wrapper::IsDeviceLost || !DX8Wrapper::IsInitted) return;
	RefMultiListClass<PhysClass> shadow_gen_list;
	RefMultiListIterator<PhysClass> static_anim_iterator(&StaticAnimList);
	for (static_anim_iterator.First(); !static_anim_iterator.Is_Done(); static_anim_iterator.Next())
	{
		StaticAnimPhysClass * obj = (StaticAnimPhysClass *)static_anim_iterator.Peek_Obj();
		if (obj != NULL)
		{
			const StaticAnimPhysDefClass * def = obj->Get_Definition();
			if (def && def->shadowDynamicObjects)
			{
				obj->Set_Shadow(NULL);
				shadow_gen_list.Add(obj);
			}
		}
	}
	_StaticShadowTexMgr.Reset();
	TextureClass * render_target = Create_Projector_Render_Target(STATIC_PROJECTOR_RESOLUTION,STATIC_PROJECTOR_RESOLUTION);
	if (render_target != NULL)
	{
		RefMultiListIterator<PhysClass> shadow_gen_iterator(&shadow_gen_list);
		for (shadow_gen_iterator.First(); !shadow_gen_iterator.Is_Done(); shadow_gen_iterator.Next())
		{
			StaticAnimPhysClass * obj = (StaticAnimPhysClass *)shadow_gen_iterator.Peek_Obj();
			Vector3 sunvector;
			Get_Sun_Light_Vector(&sunvector);
			Setup_Static_Directional_Shadow(*obj,sunvector,render_target);
		}
		/*
		// The next line of code is VERY WRONG. See above comment.
		// DX8Wrapper::Set_Render_Target((IDirect3DSurface9 *)NULL,false);
		*/
		REF_PTR_RELEASE(render_target);
	}
	StaticProjectorsDirty=false;
}

SpecialRenderInfoClass *
PhysicsSceneClass::Get_Shadow_Render_Context(int width,int height)
{
	if (ShadowRenderContext == NULL)
	{
		if (ShadowCamera == NULL)
		{
			ShadowCamera = new CameraClass();
			ShadowCamera->Set_Clip_Planes(0.2f,(float)SHADOW_CLIP_FAR);
			ShadowCamera->Set_View_Plane(DEG_TO_RAD(90.0f),DEG_TO_RAD(90.0f));
			ShadowCamera->Set_Viewport(Vector2(0,0),Vector2(1,1));
		}
		ShadowRenderContext = new SpecialRenderInfoClass(*ShadowCamera,1);
	}
	return ShadowRenderContext;
}

RENEGADE_FUNCTION
MaterialPassClass *PhysicsSceneClass::Get_Shadow_Material_Pass()
AT1(0x00627FB0);

RENEGADE_FUNCTION
void PhysicsSceneClass::Remove_Texture_Projector(TexProjectClass *)
AT1(0x00628370);

RENEGADE_FUNCTION
void PhysicsSceneClass::Add_Static_Texture_Projector(TexProjectClass *)
AT1(0x00628220);

RENEGADE_FUNCTION
void PhysicsSceneClass::Add_Dynamic_Texture_Projector(TexProjectClass *)
AT1(0x006282D0);

const float				MAX_TIMESTEP = 1.0f / 15.0f;
void PhysicsSceneClass::Update(float dt,int frameid)
{
	Generate_Static_Shadow_Projectors();
	FrameNum = frameid;
	if (dt == 0.0f)
	{
		return;
	}
	float remaining = dt;
	while (remaining > 0)
	{
		float step = min(remaining,MAX_TIMESTEP);
		RefMultiListIterator<PhysClass> it(&TimestepList);
		for (it.First(); !it.Is_Done(); it.Next())
		{
			PhysClass* phys_obj=it.Peek_Obj();
			if (phys_obj->Is_Object_Simulating())
			{
				if (!UpdateOnlyVisibleObjects || phys_obj->Get_Last_Visible_Frame()==CurrentFrameNumber || !phys_obj->As_VehiclePhysClass())
				{
					phys_obj->Timestep(step);
				}
			}
		}
		remaining -= step;
	}
	RefMultiListIterator<PhysClass> it(&TimestepList);
	for (it.First(); !it.Is_Done(); it.Next())
	{
		PhysClass* phys_obj=it.Peek_Obj();
		if (phys_obj->Is_Object_Simulating())
		{
			if (!UpdateOnlyVisibleObjects || phys_obj->Get_Last_Visible_Frame()==CurrentFrameNumber || !phys_obj->As_VehiclePhysClass())
			{
				phys_obj->Post_Timestep_Process();
			}
		}
	}
	CameraShakeSystem->Timestep(dt);
	MaterialEffectClass::Timestep_All_Effects(dt);
	Process_Release_List();
	CurrentFrameNumber++;
}

void PhysicsSceneClass::Add_Dynamic_Object(PhysClass * newobj)
{
	DynamicCullingSystem->Add_Object(newobj);
	Internal_Add_Dynamic_Object(newobj);
	DynamicPhysClass * dynobj = newobj->As_DynamicPhysClass();
	if (dynobj != NULL)
	{
		dynobj->Update_Visibility_Status();
	}
}

void PhysicsSceneClass::Internal_Add_Dynamic_Object(PhysClass * newobj)
{
	ObjList.Add(newobj);
	SceneClass::Add_Render_Object(newobj->Peek_Model());
	if (newobj->Needs_Timestep())
	{
		TimestepList.Add(newobj);
	}
}

RENEGADE_FUNCTION
void PhysicsSceneClass::Remove_Dynamic_Texture_Projector(TexProjectClass * projector)
AT1(0x00628310);

bool PhysicsSceneClass::Contains(TexProjectClass * projector)
{
	return (DynamicProjectorList.Is_In_List(projector) || StaticProjectorList.Is_In_List(projector));
}

RENEGADE_FUNCTION
void PhysicsSceneClass::Get_Level_Extents(Vector3 &min, Vector3 &max)
AT1(0x00623C80);

void PhysicsSceneClass::Allocate_Decal_Resources()
{
	DecalSystem = new PhysDecalSysClass(this);
}

void PhysicsSceneClass::Release_Decal_Resources()
{
	SAFE_DELETE(DecalSystem);
}
