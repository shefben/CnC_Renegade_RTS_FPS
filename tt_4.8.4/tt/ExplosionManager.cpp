#include "General.h"
#include "DamageableGameObj.h"
#include "WeaponMgr.h"
#include "ExplosionManager.h"
#include "DefinitionMgrClass.h"
#include "CastResultStruct.h"
#include "physcoltest.h"
#include "PhysClass.h"
#include "PhysicsSceneClass.h"
#include "PhysDefClass.h"
#include "HTreeClass.h"
#include "WW3DAssetManager.h"
#include "HAnimClass.h"
#include "cNetwork.h"
#include "RefCountedReferencerClass.h"
#include "WWAudioClass.h"
#include "TexProjectClass.h"
#include "wwmath.h"
#include "OffenseObjectClass.h"
#include "GameObjManager.h"
#include "ArmedGameObj.h"

void ExplosionManager::Create_Explosion_At
   (uint32 explosionId, const Vector3& position, ArmedGameObj* damager, const Vector3& direction, DamageableGameObj* fullDamageTarget)
{
   Matrix3D transform (Matrix3D::Identity);
   transform.setPosition (position);

   Create_Explosion_At (explosionId, transform, damager, direction, fullDamageTarget);
}



void ExplosionManager::Create_Explosion_At
   (uint32 explosionId, const Matrix3D& transform, ArmedGameObj* damager, const Vector3& direction, DamageableGameObj* fullDamageTarget)
{
   ExplosionDefinitionClass* preset = static_cast<ExplosionDefinitionClass*> (DefinitionMgrClass::Find_Definition (explosionId, true));
   if (!preset)
      return;

   Vector3 position = transform.getPosition();

   if (cNetwork::I_Am_Server())
   {
      float range = preset->DamageRadius;
      if (range > 0)
      {
         float damage = preset->DamageStrength;

         OffenseObjectClass offense(damage, preset->Warhead, damager);

		 for (SLNode<GameObject>* node = GameObjManager::GameObjList.Head(); node; node = node->Next())
         {
            BaseGameObj      *object   = (BaseGameObj *)node->Data();
            PhysicalGameObj*  physical = object->As_PhysicalGameObj();

            if (object == fullDamageTarget)
            {
               if (physical)
                  physical->Apply_Damage_Extended (offense, 1.0f, Vector3(0, 0, 0), 0);
               else
                  fullDamageTarget->Apply_Damage (offense, 1.0f, -1);

               continue;
            }

			if (!physical || !physical->Peek_Physical_Object() || !physical->Takes_Explosion_Damage())
               continue;
               
            Vector3 objectPosition  = physical->Get_Bullseye_Position();
            Vector3 objectDirection = objectPosition - position;
            float   distance        = objectDirection.Length();

            if (distance > range)
               continue;

            float damageMultiplier = 1.0f;

            if (preset->DamageIsScaled)
               damageMultiplier = 1.0f - (distance / range);

            CastResultStruct result;
            PhysRayCollisionTestClass test (LineSegClass (objectPosition, position), &result, 0x00000004);

            test.CollisionGroup = physical->Peek_Physical_Object()->Get_Collision_Group();
            test.CheckDynamicObjs = false;

            PhysicsSceneClass::Get_Instance()->Cast_Ray (test, false);

            if (result.Fraction < 0.9998f)
               damageMultiplier *= 0.25f;

            physical->Apply_Damage_Extended (offense, damageMultiplier, objectDirection, 0);
         }
      }
   }

   if (preset->PhysicsID)
   {
      PhysDefClass* physicsPreset = (PhysDefClass* )(DefinitionMgrClass::Find_Definition (preset->PhysicsID, true));
      if (physicsPreset)
      {
         PhysClass* physics = (PhysClass *)physicsPreset->Create();

		 RenderObjClass* model = physics->Peek_Model();
         if (model)
         {
            physics->Set_Transform (transform);

            if (preset->AnimatedExplosion)
            {
               HTreeClass* hTree = model->Get_HTree();
               if (hTree)
               {
                  char animationName[512];
				  _snprintf(animationName, sizeof(animationName), "%s.%s", hTree->Get_Name(), hTree->Get_Name());

                  HAnimClass* animation = WW3DAssetManager::TheInstance->Get_HAnim (animationName);
                  if (animation)
                  {
                     model->Set_Animation (animation, 0, 2);
                     animation->Release_Ref();
                  }
               }
            }

            PhysicsSceneClass::Get_Instance()->Add_Dynamic_Object (physics);
         }

         physics->Release_Ref();
      }
   }

   if (cNetwork::PClientConnection)
   {
      if (preset->SoundID)
      {
         RefCountedReferencerClass* referencer = new RefCountedReferencerClass(damager);

         WWAudioClass::_theInstance->Create_Instant_Sound(preset->SoundID, transform, referencer, 0, 2);

         referencer->Release_Ref();
      }

	  if (preset->DecalFilename && preset->DecalFilename[0])
      {
         Matrix3D decalTransform;
         decalTransform.Look_At (position, position + direction, Commands->Get_Random(0,WWMATH_PI * 2.0f));

         StringClass decalFile;
		 decalFile = StringClass(248,true);
         Strip_Path_From_Filename(decalFile, preset->DecalFilename);

         PhysicsSceneClass::Get_Instance()->Create_Decal (decalTransform, decalFile, preset->DecalSize, false, false, 0);
      }

      if (preset->CameraShakeIntensity > 0)
         PhysicsSceneClass::Get_Instance()->Add_Camera_Shake (position, preset->CameraShakeRadius, preset->CameraShakeDuration, preset->CameraShakeIntensity);
   }
}

RENEGADE_FUNCTION
void ExplosionManager::Explosion_Damage_Building
   (sint32, BuildingGameObj*, bool bStuckOnTerminal, ArmedGameObj*)
   AT2(0x00721250,0x00720810);

RENEGADE_FUNCTION
void ExplosionManager::Server_Explode
   (sint32, const Vector3& position, sint32, DamageableGameObj*)
   AT2(0x007212E0,0x007208A0);
