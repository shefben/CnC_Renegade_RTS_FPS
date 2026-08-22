#include "General.h"
#include "CollisionMath.h"

#include "Vector3.h"
#include "AABoxClass.h"
#include "CameraClass.h"



const float CollisionMath::COINCIDENCE_EPSILON = .000001f;



RENEGADE_FUNCTION
CollisionMath::OverlapType CollisionMath::Overlap_Test(FrustumClass  const&,AABoxClass  const&)
AT1(0x005FEB10);

RENEGADE_FUNCTION
CollisionMath::OverlapType CollisionMath::Overlap_Test(FrustumClass  const&,SphereClass  const&)
AT1(0x005FEAC0);

CollisionMath::OverlapType CollisionMath::Overlap_Test(const AABoxClass& box1, const AABoxClass& box2)
{
	// Returns INSIDE only if box 2 is inside box 1

	Vector3 centerOffset = (box2.Center - box1.Center).abs();

	if (centerOffset.X > box1.Extent.X + box2.Extent.X ||
	    centerOffset.Y > box1.Extent.Y + box2.Extent.Y ||
	    centerOffset.Z > box1.Extent.Z + box2.Extent.Z)
		return OUTSIDE;

	else if (centerOffset.X <= box1.Extent.X - box2.Extent.X &&
	         centerOffset.Y <= box1.Extent.Y - box2.Extent.Y &&
	         centerOffset.Z <= box1.Extent.Z - box2.Extent.Z)
		return INSIDE;

	else
		return OVERLAPPED;
}



CollisionMath::OverlapType CollisionMath::Overlap_Test(const FrustumClass& frustum, const Vector3& vector)
{
	for (uint planeIndex = 0; planeIndex < 6; ++planeIndex)
	{
		const PlaneClass& plane = frustum.Planes[planeIndex];
		float planeDistance = vector * plane.N - plane.D;
		
		if (planeDistance > COINCIDENCE_EPSILON)
			return OUTSIDE;
		else if (planeDistance >= -COINCIDENCE_EPSILON)
			return OVERLAPPED;
	}
	
	return INSIDE;
}



CollisionMath::OverlapType CollisionMath::Overlap_Test(const OBBoxClass& box, const Vector3& position)
{
	Vector3 worldOffset = position - box.Center;
	Vector3 objectOffset = box.Basis * worldOffset;
	
	if (fabs(objectOffset.X) <= box.Extent.X &&
		fabs(objectOffset.Y) <= box.Extent.Y &&
		fabs(objectOffset.Z) <= box.Extent.Z)
		return INSIDE;
	
	return OUTSIDE;
}
