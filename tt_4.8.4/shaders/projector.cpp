#include "General.h"
#include "Projector.h"
ProjectorClass::ProjectorClass(void) :
	Transform(1),
	Projection(1),
	LocalBoundingVolume(Vector3(0,0,0),Vector3(1,1,1)),
	WorldBoundingVolume(Vector3(0,0,0),Vector3(1,1,1),Matrix3(1))
{
	Mapper = new MatrixMapperClass();
}

ProjectorClass::~ProjectorClass(void)
{
	REF_PTR_RELEASE(Mapper);
}

void ProjectorClass::Set_Transform(const Matrix3D & tm)
{
	Transform = tm;
	Update_WS_Bounding_Volume();
}

const Matrix3D & ProjectorClass::Get_Transform(void) const
{
	return Transform;
}

void ProjectorClass::Set_Perspective_Projection(float hfov,float vfov,float znear,float zfar)
{
	Mapper->Set_Type(MatrixMapperClass::PERSPECTIVE_PROJECTION);
	Projection.Init_Perspective_OGL(hfov,vfov,0.1f,zfar); // TODO: Investigate/port to D3D
	float tan_hfov2 = tan(hfov) * 0.5f;
	float tan_vfov2 = tan(vfov) * 0.5f;
	LocalBoundingVolume.Center.Set(0.0f,0.0f,-(zfar+znear)*0.5f);
	LocalBoundingVolume.Extent.X = zfar * tan_hfov2;
	LocalBoundingVolume.Extent.Y = zfar * tan_vfov2;
	LocalBoundingVolume.Extent.Z = (zfar-znear)*0.5f;
	Update_WS_Bounding_Volume();
}

void ProjectorClass::Compute_Texture_Coordinate(const Vector3 & point,Vector3 * set_stq)
{
	Mapper->Compute_Texture_Coordinate(point,set_stq);
}

void ProjectorClass::Set_Ortho_Projection(float xmin,float xmax,float ymin,float ymax,float znear,float zfar)
{
	Mapper->Set_Type(MatrixMapperClass::ORTHO_PROJECTION);
	Projection.Init_Ortho_OGL(xmin,xmax,ymin,ymax,0.1f,zfar); // TODO: Investigate/port to D3D
	LocalBoundingVolume.Center.Set((xmax+xmin)*0.5f, (ymax+ymin)*0.5f, -(zfar+znear)*0.5f);
	LocalBoundingVolume.Extent.Set((xmax-xmin)*0.5f, (ymax-ymin)*0.5f, (zfar-znear)*0.5f);
	Update_WS_Bounding_Volume();
}

RENEGADE_FUNCTION
void ProjectorClass::Update_WS_Bounding_Volume()
AT1(0x005D34E0); //still an AT1 because we dont have OBBoxClass stuff
