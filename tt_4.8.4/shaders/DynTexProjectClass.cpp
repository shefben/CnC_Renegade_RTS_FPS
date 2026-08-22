#include "General.h"
#include "DynTexProjectClass.h"
DynTexProjectClass::DynTexProjectClass(PhysClass * shadow_generator) : 
	ProjectionObject(shadow_generator),
	LightSourceID(0xFFFFFFFF),
	LightVector(0.0f,0.0f,-1.0f)
{
	Set_Flag(VOLATILE,true);
}

DynTexProjectClass::~DynTexProjectClass(void)
{
}

void DynTexProjectClass::Pre_Render_Update(const Matrix3D & camera)
{
	TexProjectClass::Pre_Render_Update(camera);
	if (Get_Flag(TEXTURE_DIRTY))
	{
		Compute_Texture(ProjectionObject);
	}
}

void DynTexProjectClass::Set_Projection_Object(PhysClass * obj)
{
	ProjectionObject = obj;
}

PhysClass * DynTexProjectClass::Peek_Projection_Object(void) const
{
	return ProjectionObject;
}

void DynTexProjectClass::Set_Light_Source_ID(uint32 id)
{
	LightSourceID = id;
}

uint32 DynTexProjectClass::Get_Light_Source_ID(void)
{
	return LightSourceID;
}

void DynTexProjectClass::Set_Light_Vector(const Vector3 & vector)
{
	LightVector = vector;
}

void DynTexProjectClass::Get_Light_Vector(Vector3 * set_vector)
{
	*set_vector = LightVector;
}

void DynTexProjectClass::Enable_Perspective(bool onoff)
{
	Set_Flag(PERSPECTIVE,onoff);
}

bool DynTexProjectClass::Is_Perspective_Enabled(void)
{
	return Get_Flag(PERSPECTIVE);
}

void DynTexProjectClass::Set_Texture_Dirty(bool onoff)
{
	Set_Flag(TEXTURE_DIRTY,onoff);
}

bool DynTexProjectClass::Is_Texture_Dirty(void)
{
	return Get_Flag(TEXTURE_DIRTY);
}

void DynTexProjectClass::Update_Projection(const AABoxClass & objbox,const Matrix3D & objtm,float znear,float zfar)
{
	if (Get_Flag(PERSPECTIVE))
	{
		TexProjectClass::Compute_Perspective_Projection(objbox,objtm,LightVector,znear,zfar);
	}
	else
	{
		TexProjectClass::Compute_Ortho_Projection(objbox,objtm,LightVector,znear,zfar);
	}
}
