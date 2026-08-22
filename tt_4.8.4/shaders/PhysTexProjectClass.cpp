#include "general.h"
#include "PhysTexProjectClass.h"
#include "PhysClass.h"
#include "PhysicsSceneClass.h"
#include "LightEnvironmentClass.h"
#include "RenderInfoClass.h"
PhysTexProjectClass::PhysTexProjectClass(void) 
{
}

PhysTexProjectClass::~PhysTexProjectClass(void)
{
}

bool PhysTexProjectClass::Compute_Perspective_Projection
(
	PhysClass * obj,
	const Vector3 & lightpos,
	float nearz,
	float farz
)
{
	RenderObjClass * model = obj->Peek_Model();
	if (model == NULL)
	{
		return false;
	}
	AABoxClass box;
	model->Get_Obj_Space_Bounding_Box(box);
	const Matrix3D & tm = model->Get_Transform();
	return TexProjectClass::Compute_Perspective_Projection(box,tm,lightpos,nearz,farz);
}

bool PhysTexProjectClass::Compute_Ortho_Projection
(
	PhysClass * obj,
	const Vector3 & lightdir,
	float nearz,
	float farz
)
{
	RenderObjClass * model = obj->Peek_Model();
	if (model == NULL)
	{
		return false;
	}
	AABoxClass box;
	model->Get_Obj_Space_Bounding_Box(box);
	const Matrix3D & tm = model->Get_Transform();
	return TexProjectClass::Compute_Ortho_Projection(box,tm,lightdir,nearz,farz);
}

bool PhysTexProjectClass::Compute_Texture(PhysClass * obj,bool additive_projection)
{
	RenderObjClass * model = obj->Peek_Model();
	if (model == NULL)
	{
		return false;
	}
	int tex_size = Get_Texture_Size();
	SpecialRenderInfoClass * context = PhysicsSceneClass::Get_Instance()->Get_Shadow_Render_Context(tex_size,tex_size);
	if (context == NULL)
	{
		return false;
	}
	bool pushed_mtls = false;
	LightEnvironmentClass lenv;
	if (!additive_projection)
	{
		Vector3 v = Vector3(0,0,0);
		lenv.Reset(v,v);
		MaterialPassClass * shadow_mtl = PhysicsSceneClass::Get_Instance()->Get_Shadow_Material_Pass();
		if (shadow_mtl)
		{
			pushed_mtls = true;
			context->Push_Material_Pass(shadow_mtl);
			context->Push_Override_Flags((RenderInfoClass::RINFO_OVERRIDE_FLAGS)(RenderInfoClass::RINFO_OVERRIDE_ADDITIONAL_PASSES_ONLY | RenderInfoClass::RINFO_OVERRIDE_SHADOW_RENDERING));
			REF_PTR_RELEASE(shadow_mtl);
		}
	}
	else
	{
		Vector3 v = Vector3(0,0,0);
		Vector3 v2 = Vector3(1,1,1);
		lenv.Reset(v,v2);
	}
	context->light_environment = &lenv;
	bool success = TexProjectClass::Compute_Texture(model,context);
	if (pushed_mtls)
	{
		context->Pop_Override_Flags();
		context->Pop_Material_Pass();
	}
	return success;
}
