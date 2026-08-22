#ifndef _USERCLIPPLANECAMERA_H_
#define _USERCLIPPLANECAMERA_H_

#include "CameraClass.h"

class UserClipPlaneCamera: public CameraClass
{
protected:
	bool		UserClipPlaneStatus;
	PlaneClass	UserClipPlane;

	mutable Matrix4 UnclippedProjectionTransform;
	mutable Matrix4 ClippedProjectionTransform;

public:
	UserClipPlaneCamera(): CameraClass(), UserClipPlaneStatus(false), UserClipPlane(0, 0, 1, 0), UnclippedProjectionTransform(true), ClippedProjectionTransform(true) 
	{
		/* nothing */
	}

	UserClipPlaneCamera(const UserClipPlaneCamera& that)
	{
		*this = that;
	}
	
	UserClipPlaneCamera& operator=(const UserClipPlaneCamera& that)
	{
		if (this == &that) return *this;
		CameraClass::operator = (that);
		UserClipPlane = that.UserClipPlane;
		UnclippedProjectionTransform = that.UnclippedProjectionTransform;
		ClippedProjectionTransform = that.ClippedProjectionTransform;
		return *this;
	}

	UserClipPlaneCamera& operator=(const CameraClass& that)
	{
		if (this == &that) return *this;
		CameraClass::operator = (that);
		Set_Transform(that.Get_Transform());
		return *this;
	}
	
	virtual RenderObjClass* Clone() const
	{
		return new UserClipPlaneCamera(*this);
	}

	virtual int Class_ID() const
	{
		return RenderObjClass::CLASSID_USERCLIPPLANECAMERA;
	}

	TT_INLINE void Set_User_Clip_Plane_Status(bool status)
	{
		if (status)
		{
			UserClipPlaneStatus = true;
			ProjectionTransform = ClippedProjectionTransform;
		}
		else
		{
			UserClipPlaneStatus = false;
			ProjectionTransform = UnclippedProjectionTransform;
		}
	}

	TT_INLINE void Set_User_Clip_Plane(const PlaneClass& plane)
	{
		UserClipPlane = plane;
		FrustumValid = false;
	}

	void Update_Projection_Matrix() const
	{
		if (Projection == PERSPECTIVE)
		{
			Vector2 vpmin, vpmax;
			float znear, zfar;
			Get_View_Plane(vpmin, vpmax);
			Get_Clip_Planes(znear, zfar);

			UnclippedProjectionTransform.Init_Perspective(vpmin.X * znear, vpmax.X * znear, vpmin.Y * znear, vpmax.Y * znear, znear, zfar);
			ClippedProjectionTransform = UnclippedProjectionTransform;

			PlaneClass _clip_plane = Transform.Transform_Plane(PlaneClass(UserClipPlane.N, -UserClipPlane.D));
			Vector4& clip_plane = *(Vector4*)&_clip_plane;
		
			Vector4 q;
			q.X = sgn(clip_plane.X) / UnclippedProjectionTransform[0][0];
			q.Y = sgn(clip_plane.Y) / UnclippedProjectionTransform[1][1];
			q.Z = 1.0f;
			q.W = (1.0f - UnclippedProjectionTransform[2][2]) / UnclippedProjectionTransform[2][3];

			Vector4 c = clip_plane * (1.0f / Vector4::Dot_Product(clip_plane, q));

			ClippedProjectionTransform[2][0] = c.X;
			ClippedProjectionTransform[2][1] = c.Y;
			ClippedProjectionTransform[2][2] = c.Z;
			ClippedProjectionTransform[2][3] = c.W;
		}
		else
		{
			TT_INTERRUPT; // Orthographic projection isn't supported right now. If you really want it to be, please buy a copy of Game Programming Gems 5 (ISBN 1584503521) for Saberhawk
		}

		if (UserClipPlaneStatus)
		{
			ProjectionTransform = ClippedProjectionTransform;
		}
		else
		{
			ProjectionTransform = UnclippedProjectionTransform;
		}
	}

	virtual void Update_Frustum() const
	{
		if (FrustumValid) return;

		Transform.Get_Inverse(CameraInvTransform);
		
		Vector2 vpmin, vpmax;
		float znear, zfar;
		Get_View_Plane(vpmin, vpmax);
		Get_Clip_Planes(znear, zfar);

		Frustum.Init(Transform, vpmin, vpmax, -znear, -zfar);
		ViewSpaceFrustum.Init(Matrix3D(true), vpmin, vpmax, -znear, -zfar);

		NearClipBBox.Center = Transform * Vector3(0,0, -znear);
		NearClipBBox.Extent.X = (vpmax.X - vpmin.X) * znear * 0.5f;
		NearClipBBox.Extent.Y = (vpmax.Y - vpmin.Y) * znear * 0.5f;
		NearClipBBox.Extent.Z = 0.01f;
		NearClipBBox.Basis.Set(Transform);

		Update_Projection_Matrix();

		FrustumValid = true;
	};
	
};

#endif