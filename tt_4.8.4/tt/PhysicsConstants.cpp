#include "General.h"
#include "PhysicsConstants.h"



REF_DEF2(ContactFrictionCoefficient, ContactFrictionCoefficientType, 0x00855958, 0x00854B40);
REF_DEF2(ContactDragCoefficient, ContactDragCoefficientType, 0x00855AEC, 0x00854CD4);

REF_DEF2(PhysicsConstants::SurfaceTypeOverride, int, 0x0080F38C, 0x0080E564);
REF_DEF2(PhysicsConstants::OverrideDrag, float, 0x00855C70, 0x00854E58);
REF_DEF2(PhysicsConstants::OverrideFriction, float, 0x00855C74, 0x00854E5C);



const float PhysicsConstants::AngularDamping = .05f;
const Vector3 PhysicsConstants::GravityAcceleration(0, 0, -9.8f);
const float PhysicsConstants::DefaultContactFriction = .5f;
const float PhysicsConstants::DefaultContactDrag = 0.f;
