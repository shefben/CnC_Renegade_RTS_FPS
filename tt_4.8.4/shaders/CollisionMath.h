#ifndef TT_INCLUDE__COLLISIONMATH_H
#define TT_INCLUDE__COLLISIONMATH_H



class Vector3;
class TriClass;
class FrustumClass;
class LineSegClass;
class SphereClass;
class AABoxClass;
class OBBoxClass;
class PlaneClass;
class AAPlaneClass;
class CastResultStruct;



class SHADERS_API CollisionMath
{

	static const float COINCIDENCE_EPSILON;

public:

	enum OverlapType
	{
		POS = 0x1,
		NEG = 0x2,
		ON = 0x4,
		BOTH = 0x8,
		OUTSIDE = 0x1,
		INSIDE = 0x2,
		OVERLAPPED = 0x8,
		FRONT = 0x1,
		BACK = 0x2,
	};


	static OverlapType Overlap_Test(const AABoxClass&, const AABoxClass&);
	static OverlapType Overlap_Test(const FrustumClass&, const AABoxClass&, int&);
	static OverlapType Overlap_Test(const PlaneClass&, const AABoxClass&);
	static bool Intersection_Test(const AABoxClass&, const AABoxClass&);
	static OverlapType Overlap_Test(const AABoxClass&, const SphereClass&);
	static OverlapType Overlap_Test(const AABoxClass&, const LineSegClass&);
	static OverlapType Overlap_Test(const AABoxClass&, const TriClass&);
	static bool Collide(const AABoxClass&, const Vector3&, const PlaneClass&, CastResultStruct*);
	static bool Collide(const AABoxClass&, const Vector3&, const AABoxClass&, CastResultStruct*);
	static bool Collide(const AABoxClass&, const Vector3&, const TriClass&, CastResultStruct*);
	static bool Intersection_Test(const AABoxClass&, const TriClass&);
	static OverlapType Overlap_Test(const FrustumClass&, const Vector3&);
	static OverlapType Overlap_Test(const FrustumClass&, const TriClass&);
	static OverlapType Overlap_Test(const FrustumClass&, const SphereClass&);
	static OverlapType Overlap_Test(const FrustumClass&, const AABoxClass&);
	static OverlapType Overlap_Test(const FrustumClass&, const OBBoxClass&);
	static OverlapType Overlap_Test(const FrustumClass&, const OBBoxClass&, int&);
	static bool Collide(const LineSegClass&, const AAPlaneClass&, CastResultStruct*);
	static bool Collide(const LineSegClass&, const PlaneClass&, CastResultStruct*);
	static bool Collide(const LineSegClass&, const TriClass&, CastResultStruct*);
	static bool Collide(const LineSegClass&, const SphereClass&, CastResultStruct*);
	static bool Collide(const LineSegClass&, const AABoxClass&, CastResultStruct*);
	static bool Collide(const LineSegClass&, const OBBoxClass&, CastResultStruct*);
	static bool Intersection_Test(const OBBoxClass&, const OBBoxClass&);
	static bool Intersection_Test(const OBBoxClass&, const AABoxClass&);
	static bool Intersection_Test(const AABoxClass&, const OBBoxClass&);
	static bool Collide(const OBBoxClass&, const Vector3&, const OBBoxClass&, const Vector3&, CastResultStruct*);
	static bool Collide(const OBBoxClass&, const Vector3&, const AABoxClass&, const Vector3&, CastResultStruct*);
	static bool Collide(const AABoxClass&, const Vector3&, const OBBoxClass&, const Vector3&, CastResultStruct*);
	static OverlapType Overlap_Test(const OBBoxClass&, const Vector3&);
	static OverlapType Overlap_Test(const OBBoxClass&, const LineSegClass&);
	static OverlapType Overlap_Test(const OBBoxClass&, const TriClass&);
	static OverlapType Overlap_Test(const AABoxClass&, const OBBoxClass&);
	static OverlapType Overlap_Test(const OBBoxClass&, const AABoxClass&);
	static OverlapType Overlap_Test(const OBBoxClass&, const OBBoxClass&);
	static bool Collide(const OBBoxClass&, const Vector3&, const PlaneClass&, CastResultStruct*);
	static bool Collide(const OBBoxClass&, const Vector3&, const TriClass&, const Vector3&, CastResultStruct*);
	static bool Intersection_Test(const OBBoxClass&, const TriClass&);
	static OverlapType Overlap_Test(const AAPlaneClass&, const Vector3&);
	static OverlapType Overlap_Test(const AAPlaneClass&, const LineSegClass&);
	static OverlapType Overlap_Test(const AAPlaneClass&, const TriClass&);
	static OverlapType Overlap_Test(const AAPlaneClass&, const SphereClass&);
	static OverlapType Overlap_Test(const AAPlaneClass&, const AABoxClass&);
	static OverlapType Overlap_Test(const AAPlaneClass&, const OBBoxClass&);
	static OverlapType Overlap_Test(const PlaneClass&, const LineSegClass&);
	static OverlapType Overlap_Test(const PlaneClass&, const TriClass&);
	static OverlapType Overlap_Test(const PlaneClass&, const SphereClass&);
	static OverlapType Overlap_Test(const PlaneClass&, const OBBoxClass&);

};



#endif
