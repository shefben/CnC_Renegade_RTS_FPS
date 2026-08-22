#include "General.h"
#include "Quaternion.h"

#include "Vector2.h"
#include "Vector3.h"
#include "Matrix3.h"
#include "Matrix3D.h"
#include "Matrix4.h"
#include "wwmath.h"


const Quaternion Quaternion::IDENTITY(0.f, 0.f, 0.f, 1.f);



Quaternion::Quaternion(float angle, const Vector3& axis)
{
	float cosAngle = cos(angle / 2.f);
	float sinAngle = sin(angle / 2.f);

	t = cosAngle;
	i = axis.X * sinAngle;
	j = axis.Y * sinAngle;
	k = axis.Z * sinAngle;
}



void Quaternion::Normalize()
{
   float squareLength = (this->X * this->X) + (this->Y * this->Y)
                        + (this->Z * this->Z) + (this->W * this->W);
   if (!squareLength)
      return;

   float div = 1.0f / ::sqrtf (squareLength);

   this->X *= div;
   this->Y *= div;
   this->Z *= div;
   this->W *= div;
}



Matrix3	Quaternion::Build_Matrix3()
{
	return Matrix3
	(
		1.f - 2.f * (j * j + k * k),
		      2.f * (i * j - k * t),
		      2.f * (k * i + j * t),

		      2.f * (i * j + k * t),
		1.f - 2.f * (k * k + i * i),
		      2.f * (j * k - i * t),

		      2.f * (k * i - j * t),
		      2.f * (j * k + i * t),
		1.f - 2.f * (j * j + i * i)
	);
}



Matrix3D Quaternion::Build_Matrix3D()
{
	return Matrix3D(Build_Matrix3(), Vector3(0, 0, 0));
}



Matrix4 Quaternion::Build_Matrix4()
{
	return Matrix4(Build_Matrix3D());
}



Vector3 Quaternion::getTrackballAxis(const Vector2& screenPosition)
{
	float sqrXyLength = screenPosition.Length2();
	if (sqrXyLength > 1.f)
	{
		float invXyLength = 1.f / sqrt(sqrXyLength);
		return Vector3(screenPosition.X * invXyLength, screenPosition.Y * invXyLength, 0);
	}
	return Vector3(screenPosition.X, screenPosition.Y, sqrt(1 - sqrXyLength));
}



Quaternion Quaternion::Trackball(float x0, float y0, float x1, float y1, float size)
{
	float invSize = 1.f / size;
	Vector3 a = getTrackballAxis(Vector2(x0 * invSize, y0 * invSize));
	Vector3 b = getTrackballAxis(Vector2(x1 * invSize, y1 * invSize));

	Vector3 axis = Vector3::Cross_Product(a, b);
	if (axis.Length2() == 0)
		return IDENTITY;

	axis.Normalize();
	float cosAngle = a * b;
	
	float angle;
	if (cosAngle < -1.f)
		angle = WWMATH_PI;
	else if (cosAngle > 1.f)
		angle = 0;
	else
		angle = -acos(cosAngle);

	return Quaternion(angle, axis);
}

static int _nxt[3] = { 1 , 2 , 0 };
Quaternion Build_Quaternion(const Matrix3D & mat)
{
	float tr,s;
	int i,j,k;
	Quaternion q;
	tr = mat[0][0] + mat[1][1] + mat[2][2];
	if (tr > 0.0f)
	{
		s = sqrt(tr + 1.0f);
		q[3] = s * 0.5f;
		s = 0.5f / s;
		q[0] = (mat[2][1] - mat[1][2]) * s;
		q[1] = (mat[0][2] - mat[2][0]) * s;
		q[2] = (mat[1][0] - mat[0][1]) * s;
	}
	else
	{
		i=0;
		if (mat[1][1] > mat[0][0])
		{
			i = 1;
		}
		if (mat[2][2] > mat[i][i])
		{
			i = 2;
		}
		j = _nxt[i];
		k = _nxt[j];
		s = sqrt((mat[i][i] - (mat[j][j] + mat[k][k])) + 1.0f);
		q[i] = s * 0.5f;
		if (s != 0.0f)
		{
			s = 0.5f / s;
		}
		q[3] = ( mat[k][j] - mat[j][k] ) * s;
		q[j] =	( mat[j][i] + mat[i][j] ) * s;    
		q[k] =	( mat[k][i] + mat[i][k] ) * s;
	}
	return q;
}
