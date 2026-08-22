#ifndef TT_INCLUDE_COLTEST_H
#define TT_INCLUDE_COLTEST_H
#include "coltype.h"
#include "castresultstruct.h"
#include "LineSegClass.h"
#include "AABoxClass.h"
#include "CollisionMath.h"
#include "OBBoxClass.h"
class RenderObjClass;
class TriClass;
class CollisionTestClass
{
public:
	CollisionTestClass(CastResultStruct * res,int collision_type);
	CollisionTestClass(const CollisionTestClass & that);
public:
	CastResultStruct *			Result;
	int								CollisionType; 
	RenderObjClass *				CollidedRenderObj;		
};

inline CollisionTestClass::CollisionTestClass(CastResultStruct * res,int collision_type) :
	Result(res),
	CollisionType(collision_type),
	CollidedRenderObj(NULL)
{
}

inline CollisionTestClass::CollisionTestClass(const CollisionTestClass & that) :
	Result(that.Result),
	CollisionType(that.CollisionType),
	CollidedRenderObj(that.CollidedRenderObj)
{
}

class RayCollisionTestClass : public CollisionTestClass
{
public:
	RayCollisionTestClass(const LineSegClass & ray,CastResultStruct * res,int collision_type = COLLISION_TYPE_0,bool ignore_translucent_meshes=false);
	//RayCollisionTestClass(const RayCollisionTestClass & raytest,const Matrix3D & tm);
	bool Cull(const Vector3 & min,const Vector3 & max);
	bool Cull(const AABoxClass & box);
	bool Cast_To_Triangle(const TriClass & tri);
public:
	LineSegClass Ray;
	bool IgnoreTranslucentMeshes;
private:
	RayCollisionTestClass(const RayCollisionTestClass &);
	RayCollisionTestClass & operator = (const RayCollisionTestClass &);
};

inline RayCollisionTestClass::RayCollisionTestClass(const LineSegClass & ray,CastResultStruct * res,int collision_type,bool ignore_translucent_meshes) :
	CollisionTestClass(res,collision_type),
	Ray(ray),
	IgnoreTranslucentMeshes(ignore_translucent_meshes)
{
}

/*inline RayCollisionTestClass::RayCollisionTestClass(const RayCollisionTestClass & raytest,const Matrix3D & tm) :
	CollisionTestClass(raytest),
	Ray(raytest.Ray,tm),
	IgnoreTranslucentMeshes(raytest.IgnoreTranslucentMeshes)
{
}*/

/*inline bool RayCollisionTestClass::Cull(const Vector3 & _min,const Vector3 & _max)
{ 
	return (CollisionMath::Overlap_Test(_min,_max,Ray) == CollisionMath::POS);
}*/

inline bool RayCollisionTestClass::Cull(const AABoxClass & box) 
{ 
	return (CollisionMath::Overlap_Test(box,Ray) == CollisionMath::POS);
}

inline bool RayCollisionTestClass::Cast_To_Triangle(const TriClass & tri) 
{
	return CollisionMath::Collide(Ray,tri,Result);
}

class AABoxCollisionTestClass : public CollisionTestClass
{
public:
	AABoxCollisionTestClass(const AABoxClass & aabox,const Vector3 & move,CastResultStruct * res,int collision_type = COLLISION_TYPE_0);
	AABoxCollisionTestClass(const AABoxCollisionTestClass & that);
	enum ROTATION_TYPE 
	{
		ROTATE_NONE = 0,
		ROTATE_Z90,
		ROTATE_Z180,
		ROTATE_Z270
	};
	bool							Cull(const Vector3 & min,const Vector3 & max);
	bool							Cull(const AABoxClass & box);
	bool							Cast_To_Triangle(const TriClass & tri);
	void							Translate(const Vector3 & translation);
	void							Rotate(ROTATION_TYPE rotation);
	void							Transform(const Matrix3D & tm);
public:
	AABoxClass					Box;
	Vector3						Move;
	Vector3						SweepMin;
	Vector3						SweepMax;
private:
	AABoxCollisionTestClass & operator = (const AABoxCollisionTestClass &);
};

inline void AABoxCollisionTestClass::Translate(const Vector3 & translation)
{
	Box.Center += translation;
	SweepMin += translation;
	SweepMax += translation;
}

inline bool AABoxCollisionTestClass::Cull(const Vector3 & min,const Vector3 & max) 
{
	if ((SweepMin.X > max.X) || (SweepMax.X < min.X))
	{
		return true;
	}
	if ((SweepMin.Y > max.Y) || (SweepMax.Y < min.Y))
	{
		return true;
	}
	if ((SweepMin.Z > max.Z) || (SweepMax.Z < min.Z))
	{
		return true;
	}
	return false;
}

inline bool AABoxCollisionTestClass::Cast_To_Triangle(const TriClass & tri) 
{
	return CollisionMath::Collide(Box,Move,tri,Result);
}

class OBBoxCollisionTestClass : public CollisionTestClass
{
public:
	OBBoxCollisionTestClass(const OBBoxClass & obbox,const Vector3 & move,CastResultStruct * res,int type = COLLISION_TYPE_0);
	OBBoxCollisionTestClass(const OBBoxCollisionTestClass & that);
	//OBBoxCollisionTestClass(const OBBoxCollisionTestClass & that,const Matrix3D & tm);
	OBBoxCollisionTestClass(const AABoxCollisionTestClass & that,const Matrix3D & tm);
	bool Cull(const Vector3 & min,const Vector3 & max);
	bool Cull(const AABoxClass & box);
	bool Cast_To_Triangle(const TriClass & tri);
public:
	OBBoxClass	 				Box;
	Vector3						Move;
	Vector3						SweepMin;
	Vector3						SweepMax;
private:
	OBBoxCollisionTestClass & operator = (const OBBoxCollisionTestClass &);
};

inline bool OBBoxCollisionTestClass::Cull(const Vector3 & min,const Vector3 & max) 
{
	if ((SweepMin.X > max.X) || (SweepMax.X < min.X))
	{
		return true;
	}
	if ((SweepMin.Y > max.Y) || (SweepMax.Y < min.Y))
	{
		return true;
	}
	if ((SweepMin.Z > max.Z) || (SweepMax.Z < min.Z))
	{
		return true;
	}
	return false;
}

inline bool OBBoxCollisionTestClass::Cast_To_Triangle(const TriClass & tri) 
{
	return CollisionMath::Collide(Box,Move,tri,Vector3(0,0,0),Result);
}

#endif
