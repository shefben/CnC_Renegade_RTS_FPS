#include "General.h"
#include "BulletDataClass.h"

#include "DX8PolygonRendererClass.h"
#include "CollisionEventClass.h"
#include "PhysClass.h"
#include "ReferencerClass.h"
#include "SoldierGameObj.h"
#include "RenderObjClass.h"
#include "MeshClass.h"
#include "PhysicsSceneClass.h"
#include "ExplosionManager.h"
#include "BuildingGameObj.h"
#include "PersistFactoryClass.h"
#include "DamageableStaticPhysClass.h"
#include "SurfaceEffectsManager.h"
#include "NewDamageEvent.h"
#include "wwmath.h"
#include "WeaponMgr.h"
#include "OffenseObjectClass.h"
#include "SimpleGameObj.h"
#include "SimpleGameObjDef.h"
#include "C4GameObj.h"

uint BulletDataClass::Bullet_Collision_Occurred(const CollisionEventClass& collisionEvent)
{
	uint result = 0;
	
	OffenseObjectClass offense(ammoDefinition->Damage, ammoDefinition->Warhead, (ArmedGameObj *)owner.Get_Ptr());
	offense.EnableClientDamage = true;
	
	const Vector3& contactPoint = collisionEvent.castResult->ContactPoint;
	const Vector3& normal = collisionEvent.castResult->Normal;
	
	PhysicalGameObj* physicalObserver = NULL;
	BuildingGameObj* buildingObserver = NULL;
	CombatPhysObserverClass* observer = (CombatPhysObserverClass *)collisionEvent.physics->Get_Observer();
	if (observer)
	{
		physicalObserver = observer->As_PhysicalGameObj();
		buildingObserver = observer->As_BuildingGameObj();
	
		if (physicalObserver)
		{
			if (owner)
			{
				SimpleGameObj* simpleObserver = physicalObserver->As_SimpleGameObj();
				VehicleGameObj* vehicleObserver = physicalObserver->As_VehicleGameObj();
				SoldierGameObj* soldierOwner = owner->As_SoldierGameObj();
				
				if ((vehicleObserver && soldierOwner && vehicleObserver == soldierOwner->Get_Vehicle()) ||
					physicalObserver == owner ||
					(simpleObserver && simpleObserver->Is_Hidden_Object()))
					
					return 2;
			}
		}
	}
	
	bool v58 = false;
	if (collisionEvent.bone &&
		!collisionEvent.bone->Class_ID())
	{
		MeshClass* meshBone = (MeshClass*)collisionEvent.bone;
		if (meshBone->Get_W3D_Flags() & 0x10000000 &&
			meshBone->Is_Not_Hidden_At_All())
		{
			PhysicsSceneClass::Get_Instance()->Shatter_Mesh(meshBone, contactPoint, normal, velocity);
			v58 = true;
			result = 2;
		}
	}
	
	if (Vector3::Dot_Product(normal, velocity) > 0)
		return 2;
	
	if (!v58)
	{
		if (physicalObserver)
		{
			if (physicalObserver->Get_Network_ID() == lastColideeId)
				return 2;
			
			if (grenadeSafetyTime <= 0 ||
				owner ||
				physicalObserver->Is_Enemy((DamageableGameObj*)owner.Get_Ptr()))
			{
				lastColideeId = physicalObserver->Get_Network_ID();
				
				const char* boneName = collisionEvent.bone ? collisionEvent.bone->Get_Name() : NULL;
				NewDamageEvent::OnBulletCollisionOccurred(this, *physicalObserver, boneName);
				physicalObserver->Apply_Damage_Extended(offense, 1.f, velocity, boneName);
				if (physicalObserver->As_C4GameObj())
				{
					ScriptableGameObj* attachObject = physicalObserver->As_C4GameObj()->Get_Stuck_Object();
					
					if (attachObject && attachObject->As_PhysicalGameObj())
						attachObject->As_PhysicalGameObj()->Apply_Damage_Extended(offense, 1.f, velocity, NULL);
				}
				
				if (physicalObserver->Is_Soft() && ++softPierceCount >= ammoDefinition->SoftPierceLimit)
					return 0;
				
				if (ammoDefinition->ExplosionDefID)
					ExplosionManager::Create_Explosion_At(ammoDefinition->ExplosionDefID, contactPoint, (ArmedGameObj*)owner.Get_Ptr(), -normal, physicalObserver);
				
				b48 = true;
				result = 1;
			}
			else
				return 0;
		}
		else
		{
			if (grenadeSafetyTime > 0)
				return 0;
			
			if (buildingObserver)
				buildingObserver->Apply_Damage_Building(offense, collisionEvent.physics->As_StaticPhysClass());
			
			if (collisionEvent.physics->Get_Factory().Chunk_ID() == 0x20A02)
			{
				offense.Set_Owner(NULL);
				((DamageableStaticPhysClass*)collisionEvent.physics)->Apply_Damage_Static(offense);
			}
			
			if (!SurfaceEffectsManager::Does_Surface_Stop_Bullets(collisionEvent.castResult->SurfaceType))
				result = 2;
			
			else if (ammoDefinition->TerrainActivated)
			{
				if (ammoDefinition->ExplosionDefID)
				{
					ExplosionManager::Create_Explosion_At(ammoDefinition->ExplosionDefID, contactPoint, (ArmedGameObj*)owner.Get_Ptr(), -normal, buildingObserver);
					b49 = true;
				}
				
				b48 = true;
				result = 1;
			}
		}
	}
	
	if (Exe == 0)
	{
		Matrix3D matrix;
		matrix.Look_At(contactPoint, contactPoint - normal, rand() * (float)(2 * WWMATH_PI / RAND_MAX));
		
		SurfaceEffectsManager::Apply_Effect(collisionEvent.castResult->SurfaceType, ammoDefinition->HitterType, matrix, collisionEvent.physics, (PhysicalGameObj*)owner.Get_Ptr(), ammoDefinition->Damage >= 0.0f && !v58, 1);
	}
	
	return result;
}



/*
RENEGADE_FUNCTION
uint32 BulletDataClass::Bullet_Expired()
AT4(0x00717450, 0x00716A10, 0x00000000, 0x08190942);
*/
