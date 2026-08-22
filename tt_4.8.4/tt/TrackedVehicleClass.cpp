#include "General.h"
#include "TrackedVehicleClass.h"

#include "OctBoxClass.h"
#include "TrackedVehicleDefClass.h"
#include "mapper.h"
#include "MeshClass.h"

const Vector3	SPRING_COLOR(1.0f,0.5f,0.0f);
const Vector3	SUSPENSION_FORCE_COLOR(1.0f,0.0f,0.0f);

/*TrackedVehicleClass::TrackedVehicleClass(void) :
	LeftTrackMovement(0),
	RightTrackMovement(0),
	LeftTrackLastPosition(0,0,0),
	RightTrackLastPosition(0,0,0)
{
}

void TrackedVehicleClass::Init(const TrackedVehicleDefClass & def) 
{
	VehiclePhysClass::Init(def);	
}

TrackedVehicleClass::~TrackedVehicleClass(void)
{
}*/

void TrackedVehicleClass::Render(RenderInfoClass & rinfo)
{
	const Matrix3D & tm = Get_Transform();
	float extent_y = ContactBox->innerBox.Extent.Y;
	float extent_z = ContactBox->innerBox.Extent.Z;
	Vector3 left_track_position = tm * Vector3(0,extent_y,-extent_z);
	Vector3 right_track_position = tm * Vector3(0,-extent_y,-extent_z);
	Vector3 forward;
	tm.Get_X_Vector(&forward);
	Vector3 move;
	Vector3::Subtract(left_track_position,LeftTrackLastPosition,&move);										
	LeftTrackMovement = Vector3::Dot_Product(move,forward);
	Vector3::Subtract(right_track_position,RightTrackLastPosition,&move);
	RightTrackMovement = Vector3::Dot_Product(move,forward);
	LeftTrackLastPosition = left_track_position;
	RightTrackLastPosition = right_track_position;
	const TrackedVehicleDefClass * def = Get_TrackedVehicleDef();
	for (int i=0; i<TrackMappers.Count(); i++) {
		float movement = 0.0f;
		if (TrackMappers[i].TrackType == LEFT_TRACK) {
			movement = LeftTrackMovement;
		} else {
			movement = RightTrackMovement;
		}
		TrackMappers[i].Mapper->Set_UV_Offset_Delta(
			Vector2(def->TrackUScaleFactor * movement,def->TrackVScaleFactor * movement) );
	}
	VehiclePhysClass::Render(rinfo);
}

/*void TrackedVehicleClass::On_Post_Load (void)
{
	VehiclePhysClass::On_Post_Load();
}*/

#ifdef HOVER_TEST

#include "PhysControllerClass.h"
#include "PhysicsSceneClass.h"
#include "physcoltest.h"
#include "PhysicsConstants.h"
#include "Line3DClass.h"
#include "TimeManager.h"

void TrackedVehicleClass::Stabilizer_Compute_Force_And_Torque(const Matrix3D& transform, Vector3* force, Vector3* torque)
{
	Vector3 velocity;
	Compute_Point_Velocity(transform.getPosition(), &velocity);
	LineSegClass ray(transform.getPosition(), transform.getPosition() + Vector3(0, 0, -4.f));
	
	CastResultStruct castResult;
	PhysRayCollisionTestClass collisionTest(ray, &castResult, 2);
	collisionTest.physicsGroup = 3;
	
	flags += 0x10000000;
	PhysicsSceneClass::Get_Instance()->Cast_Ray(collisionTest, false);
	flags -= 0x10000000;

#define sqr(a) ((a)*(a))
	float mass = Get_Definition()->mass / 4.f;
	float multiplier = ((1.f - (castResult.Fraction)) / (1.f - (2.f/4.f)));
	Vector3 stabilizerForce = -multiplier * mass * Get_Definition()->gravScale * PhysicsConstants::GravityAcceleration.Length() * -transform.Get_Z_Vector();
	//stabilizerForce -= (Vector3::Dot_Product(velocity, transform.Get_Z_Vector()) * mass) * transform.Get_Z_Vector() * (float)TimeManager::FrameTicks() / 10.;

	applyForce(transform.getPosition(), stabilizerForce, *force, *torque);
}



void TrackedVehicleClass::Compute_Force_And_Torque(Vector3* force, Vector3* torque)
{
	if (engineEnabled)
	{
		if (controller)
		{
			Vector3 move(controller->Get_Move_Forward(), controller->Get_Move_Left(), controller->Get_Move_Up());
			Vector3 moveForce;
			Matrix3D::Rotate_Vector(Get_Transform(), move, &moveForce);
			moveForce *= Get_Definition()->maxEngineTorque; //10.f * Get_Definition()->mass * Get_Definition()->gravScale;
			applyForce(Get_Transform() * Vector3(3.f, 0.f, .1f), moveForce, *force, *torque);
		}
		
		Matrix3D transform = Get_Transform();
		Stabilizer_Compute_Force_And_Torque(transform * Matrix3D(Vector3( box->innerBox.Extent.X,  box->innerBox.Extent.Y, 0)), force, torque);
		Stabilizer_Compute_Force_And_Torque(transform * Matrix3D(Vector3(-box->innerBox.Extent.X,  box->innerBox.Extent.Y, 0)), force, torque);
		Stabilizer_Compute_Force_And_Torque(transform * Matrix3D(Vector3( box->innerBox.Extent.X, -box->innerBox.Extent.Y, 0)), force, torque);
		Stabilizer_Compute_Force_And_Torque(transform * Matrix3D(Vector3(-box->innerBox.Extent.X, -box->innerBox.Extent.Y, 0)), force, torque);
	}

	Vector3 position;
	Get_Position(position);

	static Line3DClass* lineRenderers[10] = {0};

	for (int i = 0; i < 10; ++i)
		if (lineRenderers[i])
			PhysicsSceneClass::Get_Instance()->Remove_Render_Object(lineRenderers[i]);
	
	lineRenderers[0] = new Line3DClass(position + Vector3(0, 0, 3), position + Vector3(0, 0, 3) + *force * .00005f, .02f, 1.f, 0.f, 0.f, 1.f);
	lineRenderers[1] = new Line3DClass(position + Vector3(0, 0, 3), position + Vector3(0, 0, 3) + *torque * .00005f, .02f, 0.f, 1.f, 0.f, 1.f);

	RigidBodyClass::Compute_Force_And_Torque(force, torque);

	lineRenderers[3] = new Line3DClass(position + Vector3(0, .1, 3), position + Vector3(0, .1, 3) + *force * .00005f, .02f, .5f, 0.f, 0.f, 1.f);
	lineRenderers[4] = new Line3DClass(position + Vector3(0, .1, 3), position + Vector3(0, .1, 3) + *torque * .00005f, .02f, 0.f, .5f, 0.f, 1.f);
	
	Vector3 angularVelocity;
	Get_Angular_Velocity(angularVelocity);
	if (angularVelocity.Length() != 0)
	{
		Vector3 angularVelocityDirection = angularVelocity;
		angularVelocityDirection.Normalize();
		float factor = Vector3::Dot_Product(angularVelocityDirection, inertia * angularVelocityDirection) / Get_Definition()->mass;
		Vector3 dragtorque = angularVelocity * (angularVelocity.Length() * Get_Definition()->aerodynamicDrag * factor * factor);
		lineRenderers[5] = new Line3DClass(position + Vector3(0, .2, 3), position + Vector3(0, .2, 3) + dragtorque * .00005f, .02f, 0.f, 1.f, 1.f, 1.f);
	}
	void DebugOutputString(const char* format, ...);
	DebugOutputString("av: %f %f %f:  %f\n", angularVelocity.X, angularVelocity.Y, angularVelocity.Z, angularVelocity.Length());

	for (int i = 0; i < 10; ++i)
		if (lineRenderers[i])
		{
			PhysicsSceneClass::Get_Instance()->Add_Render_Object(lineRenderers[i]);
			lineRenderers[i]->Release_Ref();
		}

	flags &= ~FRICTION_DISABLED;
}
#endif
