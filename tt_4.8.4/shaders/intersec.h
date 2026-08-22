#ifndef TT_INCLUDE_INTERSEC_H
#define TT_INCLUDE_INTERSEC_H

typedef unsigned short POLYGONINDEX;
class IntersectionResultClass 
{
public:
	RenderObjClass *IntersectedRenderObject;
	POLYGONINDEX IntersectedPolygon;
	Matrix3D ModelMatrix;
	Vector3 ModelLocation;
	Vector3 Intersection;
	float Range;
	float Alpha, Beta;
	bool Intersects;
	enum INTERSECTION_TYPE
	{
		NONE = 0,
		GENERIC, 
		POLYGON
	} IntersectionType;
};
class IntersectionClass
{
public:
	enum {
		MAX_POLY_INTERSECTION_COUNT = 1000,
		MAX_HIERARCHY_NODE_COUNT = 256			
	};
	Vector3 *RayLocation;
	Vector3 *RayDirection;
	Vector3 *IntersectionNormal;
	float ScreenX, ScreenY;
	bool InterpolateNormal;
	bool ConvexTest;
	float MaxDistance;
	IntersectionResultClass Result;
	virtual ~IntersectionClass() {}
	TT_INLINE bool Intersect_Sphere_Quick(SphereClass &Sphere, IntersectionResultClass *FinalResult) 
	{
		Vector3 sphere_vector(Sphere.Center - *RayLocation);
		FinalResult->Alpha = Vector3::Dot_Product(sphere_vector, *RayDirection);
		FinalResult->Beta = Sphere.Radius * Sphere.Radius - (Vector3::Dot_Product(sphere_vector, sphere_vector) - FinalResult->Alpha * FinalResult->Alpha);
		if(FinalResult->Beta < 0.0f)
		{
			return FinalResult->Intersects = false;
		}
		return FinalResult->Intersects = true;
	}
	TT_INLINE bool Intersect_Sphere(SphereClass &Sphere, IntersectionResultClass *FinalResult) 
	{
		if(!Intersect_Sphere_Quick(Sphere, FinalResult))
		{
			return false;
		}
		float d = sqrtf(FinalResult->Beta);
		FinalResult->Range = FinalResult->Alpha - d;
		if(FinalResult->Range > MaxDistance)
		{
			return false;
		}
		FinalResult->Intersection = *RayLocation +  FinalResult->Range * (*RayDirection);
		if(IntersectionNormal != 0)
		{
			(*IntersectionNormal) = FinalResult->Intersection - Sphere.Center;	
		}
		return true;
	}
};
#endif
