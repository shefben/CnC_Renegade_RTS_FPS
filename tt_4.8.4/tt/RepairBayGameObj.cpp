#include "general.h"

#include "DX8PolygonRendererClass.h"
#include "RepairBayGameObj.h"
#include "multilist.h"
#include "RenderObjClass.h"
#include "BuildingAggregateDefClass.h"
#include "PhysicsSceneClass.h"
#include "ObjectLibraryManager.h"
#include "TimeManager.h"
#include "MeshClass.h"
#include "HAnimClass.h"
#include "TexProjectClass.h"
#include "bitstream.h"
#include "ScriptZoneGameObj.h"
#include "engine_obj2.h"
#include "engine_player.h"
#include "PhysicalGameObj.h"
const char* RepairBayGameObj::BoneNames[6] =
   { "BONE02", "BONE03", "BONE04", "BONE06", "BONE07", "BONE08" };

void RepairBayGameObj::Export_Creation(BitStreamClass &stream)
{
	BuildingGameObj::Export_Creation(stream);
	stream.Add(repairZone.Center.X,BITPACK_WORLD_POSITION_X);
	stream.Add(repairZone.Center.Y,BITPACK_WORLD_POSITION_Y);
	stream.Add(repairZone.Center.Z,BITPACK_WORLD_POSITION_Z);
	stream.Add(repairZone.Extent.X,BITPACK_WORLD_POSITION_X);
	stream.Add(repairZone.Extent.Y,BITPACK_WORLD_POSITION_Y);
	stream.Add(repairZone.Extent.Z,BITPACK_WORLD_POSITION_Z);
	stream.Add(repairZone.Basis.Get_Z_Rotation());
}

void RepairBayGameObj::Import_Creation(BitStreamClass &stream)
{
	BuildingGameObj::Import_Creation(stream);
	stream.Get(repairZone.Center.X,BITPACK_WORLD_POSITION_X);
	stream.Get(repairZone.Center.Y,BITPACK_WORLD_POSITION_Y);
	stream.Get(repairZone.Center.Z,BITPACK_WORLD_POSITION_Z);
	stream.Get(repairZone.Extent.X,BITPACK_WORLD_POSITION_X);
	stream.Get(repairZone.Extent.Y,BITPACK_WORLD_POSITION_Y);
	stream.Get(repairZone.Extent.Z,BITPACK_WORLD_POSITION_Z);
	float rotate = 0;
	stream.Get(rotate);
	repairZone.Basis.Rotate_Z(rotate);
}

void RepairBayGameObj::CnC_Initialize
   (BaseControllerClass* base)
{
   BuildingGameObj::CnC_Initialize (base);

   Vector3 position;
   this->Get_Position (&position);

   ScriptZoneGameObj* zone = ScriptZoneGameObj::Find_Closest_Zone (position, ZT_VEHICLE_REPAIR);
   if (zone)
   {
	   this->repairZone = zone->Get_Bounding_Box();
	   if (!zone->Get_Observers().Count())
	   {
         zone->Set_Delete_Pending();
	   }
   }

   bool multiplayerAggregate = false;
   Matrix3D multiplayerAggregateTransform;

   for (RefMultiListIterator<BuildingAggregateClass> iter (&this->aggregates); !iter.Is_Done(); iter.Next())
   {
      BuildingAggregateClass* aggregate = iter.Get_Obj();
	  if (aggregate)
	  {
         aggregate->Release_Ref();
	  }
      if (!aggregate)
         continue;
      if (!aggregate->Peek_Model())
         continue;

      const char *name = aggregate->Peek_Model()->Get_Name();
      if (!_stricmp(name,"MNREP_AG_1") || !_stricmp(name,"MGREP_AG_1"))
      {
         multiplayerAggregate = true;
         multiplayerAggregateTransform = aggregate->Get_Transform();
         break;
      }
      if (!stristr(name,"REP^NOD_FX"))
	  {
         continue;
	  }
      this->repairEffect = aggregate;
      break;
   }
   if (multiplayerAggregate && BuildingAggregateDefClass::repairBayAnimation)
   {
      BuildingAggregateClass* aggregate = (BuildingAggregateClass *)BuildingAggregateDefClass::repairBayAnimation->Create();
      this->Add_Aggregate (aggregate);
      aggregate->Set_Transform (multiplayerAggregateTransform);
      this->repairEffect = aggregate;
      aggregate->Release_Ref();
   }
   Vector3 temp;
   float minDistance = 100000.0f;

   RefMultiListIterator<PhysClass> iterator = PhysicsSceneClass::Get_Instance()->Get_Static_Object_Iterator();
   for (iterator.First(); !iterator.Is_Done(); iterator.Next())
   {
      PhysClass* phys = iterator.Get_Obj();
	  if (phys)
	  {
         phys->Release_Ref();
	  }
      if (!phys)
         continue;
      if (!phys->As_StaticAnimPhysClass())
         continue;

	  if (phys->Get_Definition()->Get_ID() != Get_Definition().staticAnimId)
         continue;

      phys->Get_Position(&temp);

	  float distance = Vector3::Distance(position,temp);
      if (distance >= minDistance)
         continue;

      minDistance = distance;
      this->physId = phys->Get_ID();

      break;
   }
}



void RepairBayGameObj::Emit_Welding_Arc
   (RenderObjClass* targetModel)
{
   if (!this->repairEffect)
      return;

   if (!this->arcEffects[0])
   {
      for (uint32 u = 0; u < 4; ++u)
      {
         PersistClass* object = ObjectLibraryManager::Create_Object ("Arc Effect");
         if (!object)
		 {
            break;
		 }
		 PhysicalGameObj* physical = ((PhysicalGameObj *)object)->As_PhysicalGameObj();
         if (!physical)
         {
            break;
         }
         this->arcEffects[u]    = physical;
         this->arcDisplayTime[u]= 0;
		 if (physical->Peek_Physical_Object() && physical->Peek_Physical_Object()->Peek_Model())
		 {
            physical->Peek_Physical_Object()->Peek_Model()->Set_Hidden(1);
		 }
      }

      if (!this->arcEffects[0])
         return;

      PhysicalGameObj* firstArcEffect = this->arcEffects[0];
      RenderObjClass* model = firstArcEffect->Peek_Physical_Object() ? firstArcEffect->Peek_Physical_Object()->Peek_Model() : 0;

      if (model)
      {
         for (uint32 u = 0; u < 6; ++u)
            this->boneTransforms[u] = model->Get_Bone_Transform (BoneNames[u]);

         this->lastBoneTransform = model->Get_Bone_Transform (model->Get_Bone_Index ("BONE_END"));
      }
   }

   char buffer[15];
   targetModel->Validate_Transform();

   for (uint32 arcNumber = 0; arcNumber < 4; ++arcNumber)
   {
      if (this->arcDisplayTime[arcNumber] > 0)
         continue;

      if (::rand() % 4)
         continue;

	  sprintf(buffer,"REP^NODRIM_FX%u",::rand() % 8 + 1);
      Vector3 sourcePosition = this->repairEffect->Peek_Model()->Get_Bone_Transform(buffer).getPosition();
      Vector3 targetPosition;
      MeshClass* mesh = ::Find_Random_Mesh(targetModel);
      if (mesh)
      {
         MeshModelClass* meshModel = mesh->Get_Model();
         if (meshModel)
         {
            if (meshModel->Vertex->Get_Count())
            {
               mesh->Validate_Transform();

			   uint32 vertexIndex = ::rand() % meshModel->Vertex->Get_Count();
			   targetPosition = mesh->Get_Transform().applyTo (meshModel->Vertex->Get_Array()[vertexIndex]);
            }

            meshModel->Release_Ref();
         }
      }

      PhysicalGameObj* arc      = this->arcEffects[arcNumber];
      RenderObjClass*  arcModel = arc->Peek_Physical_Object()->Peek_Model();
      Matrix3D         arcTransform;

      arcTransform.Obj_Look_At (sourcePosition, targetPosition);
      arcModel->Set_Transform (arcTransform);

      Vector3 difference = targetPosition - sourcePosition;

      for (uint32 boneNumber = 0; boneNumber < 6; ++boneNumber)
      {
         Vector3 originalPosition = this->boneTransforms[boneNumber].getPosition();
         Vector3 scaledDifference = difference * ::fabs (originalPosition.X / this->lastBoneTransform.getPosition().X);

         originalPosition = arcTransform.applyTo (originalPosition);

         Matrix3D boneTransform (Matrix3D::Identity);
         boneTransform.setPosition (sourcePosition + scaledDifference - originalPosition);

         sint32 boneIndex = arcModel->Get_Bone_Index (BoneNames[boneNumber]);
         arcModel->Capture_Bone (boneIndex);
         arcModel->Control_Bone (boneIndex, boneTransform, true);
      }

      Vector3 lastBonePosition = this->lastBoneTransform.getPosition();
      Vector3 transformedLastBonePosition = arcTransform.applyTo (lastBonePosition);

      lastBonePosition = targetPosition - transformedLastBonePosition;

      Matrix3D transform (Matrix3D::Identity);
      transform.setPosition (lastBonePosition);

      sint32 boneIndex = arcModel->Get_Bone_Index ("BONE_END");
      arcModel->Capture_Bone (boneIndex);
      arcModel->Control_Bone (boneIndex, transform, true);

      HAnimClass* animation = arcModel->Peek_Animation();
      if (animation)
      {
         float random = (float)(::rand() % 100) / 100.0f;
         arcModel->Set_Animation (animation, 0, 1);
         this->arcDisplayTime[arcNumber] = animation->Get_Total_Time() - 0.5f + random;
      }
      else
         this->arcDisplayTime[arcNumber] = 3.0f;

      arcModel->Set_Hidden (0);
   }
}



RENEGADE_FUNCTION
bool RepairBayGameObj::Repair_Vehicle()
   AT2(0x00740F00,0x007407A0);

void RepairBayGameObj::Think()
{
   if (!this->destroyed)
   {
      this->repairWaitTime -= TimeManager::FrameSeconds;
      if (this->repairWaitTime < 0)
      {
         this->repairWaitTime = 1.0f;
         this->isRepairing = this->Repair_Vehicle();
      }

      if (this->isRepairing)
         this->Update_Repairing_Animations();
   }

   for (uint32 u = 0; u < 4; ++u)
   {
      if (this->arcDisplayTime[u] <= 0)
         continue;

      this->arcDisplayTime[u] -= TimeManager::FrameSeconds;
      if (this->arcDisplayTime[u] <= 0)
         this->arcEffects[u]->Peek_Physical_Object()->Peek_Model()->Set_Hidden (1);
      else
         this->arcEffects[u]->Peek_Physical_Object()->Set_Transform (this->arcEffects[u]->Peek_Physical_Object()->Get_Transform());
   }

   BuildingGameObj::Think();
}


void RepairBayGameObj::Update_Repairing_Animations()
{
   SimpleDynVecClass<PhysicalGameObj*> vehicles;

   for (sint32 u = 0; u < this->vehicles.Count(); ++u)
   {
      ScriptableGameObj* object = this->vehicles[u];
      if (!object)
         continue;

      PhysicalGameObj* vehicle = (PhysicalGameObj *)object->As_VehicleGameObj();
      if (!vehicle || !vehicle->Peek_Physical_Object() || !vehicle->Peek_Physical_Object()->Peek_Model() || !vehicle->Is_Damaged())
         continue;

      vehicles.Add (vehicle);
   }

   for (uint32 u = 0; u < 4; ++u)
   {
      if (!vehicles.Count())
         break;
	  unsigned int x = ::rand() % vehicles.Count();
      PhysicalGameObj* vehicle = vehicles[x];
      vehicles.Delete(x);
      this->Emit_Welding_Arc (vehicle->Peek_Physical_Object()->Peek_Model());
   }
}

const RepairBayGameObjDef & RepairBayGameObj::Get_Definition( void ) const
{
	return (const RepairBayGameObjDef &)*definition;
}
