#ifndef _PLANEREFLECTIONCAMERA_H_
#define _PLANEREFLECTIONCAMERA_H_

#include "UserClipPlaneCamera.h"

class PlaneReflectionCamera: public UserClipPlaneCamera
{
protected:
	PlaneClass			ReflectionPlane;
	Matrix3D			ReflectionMatrix;

	mutable Matrix3D	ReflectedTransform;
public:
	PlaneReflectionCamera(): UserClipPlaneCamera(), ReflectionPlane(0, 0, 1, 0), ReflectionMatrix(true)
	{
		/* nothing */
	}

	PlaneReflectionCamera(const PlaneReflectionCamera& that)
	{
		*this = that;
	}

	PlaneReflectionCamera& operator = (const PlaneReflectionCamera& that)
	{
		if (this == &that) return *this;
		UserClipPlaneCamera::operator = (that);
		ReflectionPlane = that.ReflectionPlane;
		ReflectionMatrix = that.ReflectionMatrix;
		return *this;
	}

	PlaneReflectionCamera& operator = (const CameraClass& that)
	{
		if (this == &that) return *this;
		CameraClass::operator = (that);
		Set_Transform(that.Get_Transform());
		return *this;
	}

	RenderObjClass* Clone() const
	{
		return new PlaneReflectionCamera(*this);
	}

	int Class_ID() const
	{
		return RenderObjClass::CLASSID_PLANEREFLECTIONCAMERA;
	}

	TT_INLINE void Set_Reflection_Plane(const PlaneClass& plane)
	{
		ReflectionPlane = plane;
		ReflectionMatrix = Matrix3D::Reflect_Plane(plane);
		UserClipPlaneCamera::Set_User_Clip_Plane(plane);
		FrustumValid = false;
	}
	
	virtual void Update_Frustum() const
	{
		if (FrustumValid) return;

		ReflectedTransform = ReflectionMatrix * Transform;
		ReflectedTransform.Get_Inverse(CameraInvTransform);
		
		Vector2 vpmin, vpmax;
		float znear, zfar;
		Get_View_Plane(vpmin, vpmax);
		Get_Clip_Planes(znear, zfar);

		Frustum.Init(ReflectedTransform, vpmin, vpmax, -znear, -zfar);
		ViewSpaceFrustum.Init(Matrix3D(true), vpmin, vpmax, -znear, -zfar);

		float e = Get_Focal_Length();
		float ar = Get_Aspect_Ratio();
		float l_x = e / sqrtf(e * e + 1);
		float l_z = -(1.0f / sqrtf(e * e + 1));
		float b_y = e / sqrtf(e * e + ar * ar);
		float b_z = -(ar / sqrtf(e * e + ar * ar));

		Frustum.Planes[0] = CameraInvTransform.Transform_Plane(PlaneClass(Vector3(0, 0, -1),		-znear));	// near
		Frustum.Planes[1] = CameraInvTransform.Transform_Plane(PlaneClass(Vector3(0, b_y, b_z),		0));		// bottom
		Frustum.Planes[2] = CameraInvTransform.Transform_Plane(PlaneClass(Vector3(-l_x, 0, l_z),	0));		// rig
		Frustum.Planes[3] = CameraInvTransform.Transform_Plane(PlaneClass(Vector3(0, -b_y, b_z),	0));		// top
		Frustum.Planes[4] = CameraInvTransform.Transform_Plane(PlaneClass(Vector3(l_x, 0, l_z),		0));		// left
		Frustum.Planes[5] = CameraInvTransform.Transform_Plane(PlaneClass(Vector3(0, 0, 1),			zfar));		// far

		// Why this works I don't know. It does though.
		for (int i = 0; i < 6; ++i)
		{
			Frustum.Planes[i].N = -Frustum.Planes[i].N;
		};

		memset(&Frustum.Corners, 0, sizeof(Frustum.Corners));
		memset(&ViewSpaceFrustum.Corners, 0, sizeof(Frustum.Corners));

		NearClipBBox.Center = ReflectedTransform * Vector3(0,0, -znear);
		NearClipBBox.Extent.X = (vpmax.X - vpmin.X) * znear * 0.5f;
		NearClipBBox.Extent.Y = (vpmax.Y - vpmin.Y) * znear * 0.5f;
		NearClipBBox.Extent.Z = 0.01f;
		NearClipBBox.Basis.Set(ReflectedTransform);

		Update_Projection_Matrix();

		FrustumValid = true;
	};

};

#endif
