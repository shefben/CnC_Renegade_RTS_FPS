#include "general.h"

#include "RigidBodyClass.h"
#include "RigidBodyDefClass.h"
#include "PhysicsConstants.h"
#include "OctBoxClass.h"
#include "Quaternion.h"
#include "StateVectorClass.h"



int RigidBodyClass::Compute_Derivatives
(
	float						t,
	StateVectorClass *	test_state,
	StateVectorClass *	dydt,
	int						index
)
{
	if (test_state) {
		Set_State(*test_state,index);
	}

	// time derivitive of position
	(*dydt)[index++] = Velocity[0];
	(*dydt)[index++] = Velocity[1];
	(*dydt)[index++] = Velocity[2];

	// time derivitive of orientation
	Quaternion avel(AngularVelocity.X,AngularVelocity.Y,AngularVelocity.Z,0.0f);
	Quaternion qdot = 0.5 * avel * State.Orientation;
	(*dydt)[index++] = qdot[0];
	(*dydt)[index++] = qdot[1];
	(*dydt)[index++] = qdot[2];
	(*dydt)[index++] = qdot[3];

	// time derivitives of momentum and angular momentum (a.k.a. force and torque)
	Vector3 force(0,0,0);
	Vector3 torque(0,0,0);
	Compute_Force_And_Torque(&force,&torque);
	
	(*dydt)[index++] = force[0];
	(*dydt)[index++] = force[1];
	(*dydt)[index++] = force[2];

	(*dydt)[index++] = torque[0];
	(*dydt)[index++] = torque[1];
	(*dydt)[index++] = torque[2];
	
	return index;
}

void RigidBodyClass::Compute_Point_Velocity(const Vector3& pointOffset, Vector3* result)
{
	*result = Velocity + Vector3::Cross_Product(AngularVelocity, pointOffset - State.Position);
}



void RigidBodyClass::applyForce(const Vector3& position, const Vector3& force, Vector3& totalForce, Vector3& totalTorque)
{
	Vector3 arm = position - Get_Transform().getPosition();
	totalForce += force;
	totalTorque += Vector3::Cross_Product(arm, force);
}


void RigidBodyClass::Compute_Force_And_Torque(Vector3* force, Vector3* torque)
{
	TT_UNIMPLEMENTED;
	// angular velocity calculation is wrong, friction calculation is possibly wrong.
	// Renegade seems to have been based on a SIGGRAPH course, see these:
	// http://www-2.cs.cmu.edu/~baraff/sigcourse/
	// http://www-2.cs.cmu.edu/~baraff/sigcourse/notesd1.pdf
	// http://www-2.cs.cmu.edu/~baraff/sigcourse/notesd2.pdf
	TT_UNIMPLEMENTED;
#if 0
	const RigidBodyDefClass& definition = *Get_RigidBodyDef();
	
	// Apply gravity
	*force += PhysicsConstants::GravityAcceleration * (GravScale * Mass);
	
	// Apply aerodynamic drag
	*force -= Velocity * (Velocity.Length() * definition.AerodynamicDragCoefficient);
	
	// Apply angular aerodynamic drag
	// *torque -= angularVelocity * (angularVelocity.Length() * PhysicsConstants::AngularDamping);
	
	if (definition.CollisionDisabled)
		return;
	
	// add collision forces
	ContactBox->Compute_Contacts(false);
	
	for (uint32 contactIndex = 0; contactIndex < ContactBox->contactCount; ++contactIndex)
	{
		const CastResultStruct& contact = ContactBox->Get_Contact(contactIndex);
		
		Vector3 contactArm = contact.ContactPoint - State.Position;
		
		Vector3 contactVelocity;
		Compute_Point_Velocity(contact.ContactPoint, &contactVelocity);
		
		float perpendicularVelocityLength = Vector3::Dot_Product(contactVelocity, contact.Normal);
		
		Phys3Class* physics = ContactBox->contactPhysics[contactIndex]->As_Phys3Class();
		if (!physics || !Push_Phys3_Object_Away(physics, contact))
		{
			float normalForceLength = (1.0f - contact.Fraction) * ContactBox->f68 - perpendicularVelocityLength * ContactBox->f72;
			if (normalForceLength > 0)
			{
				Vector3 normalForce = contact.Normal * normalForceLength;
				
				applyForce(contact.ContactPoint, normalForce, *force, *torque);
				
				if (!(Flags & FRICTION_DISABLED))
				{
					Vector3 parallelVelocity = contactVelocity - contact.Normal * perpendicularVelocityLength;
					float parallelVelocitySquaredLength = parallelVelocity.Length2();
					
					if (parallelVelocitySquaredLength >= 0.00000001f)
					{
						float parallelVelocityLength = sqrt(parallelVelocitySquaredLength);
						Vector3 parallelVelocityDirection = parallelVelocity / parallelVelocityLength;
						
						float frictionCoefficient = PhysicsConstants::Get_Contact_Friction_Coefficient(0, contact.SurfaceType);
						float frictionForceLength = frictionCoefficient * normalForceLength;
						frictionForceLength = min(frictionForceLength, parallelVelocityLength * definition.Mass * 30 / ContactBox->contactCount);
						Vector3 frictionForce = parallelVelocityDirection * -frictionForceLength;

						applyForce(contact.ContactPoint, frictionForce, *force, *torque);
					}
				}
			}
		}
	}
#endif
}



RENEGADE_FUNCTION
bool RigidBodyClass::Push_Phys3_Object_Away
   (Phys3Class*, const CastResultStruct&)
   AT2(0x00657190,0x00656A30);



RENEGADE_FUNCTION
void RigidBodyClass::Network_Interpolate_State_Update(const Vector3& position, const Quaternion& orientation, const Vector3& velocity, const Vector3& angularVelocity, float)
AT2(0x00652F40, 0x006527E0);



RENEGADE_FUNCTION
void RigidBodyClass::Network_Latency_State_Update(const Vector3& position, const Quaternion& orientation, const Vector3& velocity, const Vector3& angularVelocity)
AT2(0x00653160, 0x00652A00);
