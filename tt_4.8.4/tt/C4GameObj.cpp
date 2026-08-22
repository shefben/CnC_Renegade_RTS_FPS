#include "General.h"
#include "C4GameObj.h"
#include "C4GameObjDef.h"
#include "engine_weap.h"
#include "WeaponMgr.h"
#include "bitstream.h"
#include "ProjectileClass.h"
#include "TimeManager.h"
#include "SoldierGameObj.h"
#include "cNetwork.h"
#include "RenderObjClass.h"
#include "DefinitionMgrClass.h"
#include "GameObjManager.h"
#include "PhysicsSceneClass.h"
#include "StaticPhysClass.h"
#include "CombatManager.h"
#include "RefCountedReferencerClass.h"
#include "cGameType.h"
#include "WWAudioClass.h"
#include "CollisionEventClass.h"
#include "BuildingGameObj.h"
#include "PersistFactoryClass.h"
#include "SurfaceEffectsManager.h"
#include "ExplosionManager.h"
#include "cGameType.h"

int MineLimit = 30;
void C4GameObj::Maintain_C4_Limit(int playertype)
{
	if (CombatManager::I_Am_Server() && !IS_MISSION)
	{
		C4GameObj *OldestC4 = 0;
		int count = 0;
		for (SLNode<BaseGameObj>* objectNode = (SLNode<BaseGameObj>*)GameObjManager::GameObjList.Head(); objectNode; objectNode = objectNode->Next())
		{
			PhysicalGameObj* object = objectNode->Data()->As_PhysicalGameObj();
			if (object)
			{
				C4GameObj *c4 = object->As_C4GameObj();
				if ((c4) && (c4->Get_Player_Type() == playertype))
				{
					if (c4->AmmoDef)
					{
						int at = c4->AmmoDef->AmmoType;
						if (at != 2)
						{
							count++;
							if ((!OldestC4) || (c4->SleepTime > OldestC4->SleepTime))
							{
								OldestC4 = c4;
							}
						}
					}
				}
			}
		}
		if ((count > MineLimit) && (OldestC4))
		{
			OldestC4->Defuse();
		}
	}
}

void C4GameObj::Init_C4(AmmoDefinitionClass const *ammoPreset, SoldierGameObj* owner, uint32 detonationMode, const Matrix3D& muzzle)
{
	AmmoDef = ammoPreset;
	if (!ammoPreset->Model.Is_Empty())
	{
		Peek_Physical_Object()->Set_Model_By_Name(ammoPreset->Model);
	}
	Owner = owner;
	DetonationMode = detonationMode;
	Set_Transform(muzzle);
	attached = false;
	IsAttachedToMCT = false;
	attachedToDynamic = false;
	Peek_Physical_Object()->Flags &= 0xFFFFFFF0;
	Player = 0;
	if (owner)
	{
		Set_Player_Type(owner->Get_Player_Type());
		Player = owner->Get_Player_Data();
		if (CombatManager::I_Am_Server())
		{
			if (cGameType::GameType != 1)
			{
				Maintain_C4_Limit(Get_Player_Type());
			}
		}
	}
	int at = AmmoDef->AmmoType;
	if (1 != at)
	{
		float triggertime = AmmoDef->C4TriggerTime1;
		if (detonationMode == 2)
		{
			triggertime = AmmoDef->C4TriggerTime2;
		}
		if (detonationMode == 3)
		{
			triggertime = AmmoDef->C4TriggerTime3;
		}
		TriggerTime = triggertime;
	}
	if (AmmoDef->C4TimingSound1ID != 0)
	{
		RefCountedReferencerClass *rcrc = new RefCountedReferencerClass();
		ReferencerClass *rc = rcrc;
		(*rc) = Owner.Get_Ptr();
		WWAudioClass::_theInstance->Create_Instant_Sound(AmmoDef->C4TimingSound1ID,Get_Transform(),rcrc,0,2);
		rcrc->Release_Ref();
	}
	ProjectileClass *p = Peek_Physical_Object()->As_ProjectileClass();
	if (p)
	{
		Vector3 velocity = muzzle.getXAxis() * Get_Definition().ThrowVelocity;
		p->Set_Velocity(velocity);
	}
	Start_Observers();
	Set_Object_Dirty_Bit(DB_RARE, true);
}

void C4GameObj::Import_Rare(BitStreamClass& stream)
{
	PhysicalGameObj::Import_Rare(stream);
	int ammo;
	stream.Get(ammo);
	if (ammo)
	{
		AmmoDef = (AmmoDefinitionClass *)DefinitionMgrClass::Find_Definition(ammo,true);
		Peek_Physical_Object()->Set_Model_By_Name(AmmoDef->Model);
	}
	int ownerid;
	stream.Get(ownerid);
	SmartGameObj *obj;
	if (ownerid)
	{
		obj = GameObjManager::Find_SmartGameObj(ownerid);
	}
	else
	{
		obj = 0;
	}
	Owner = obj;
	Vector3 position, velocity;
	stream.Get(velocity.X,BITPACK_VEHICLE_VELOCITY);
	stream.Get(velocity.Y,BITPACK_VEHICLE_VELOCITY);
	stream.Get(velocity.Z,BITPACK_VEHICLE_VELOCITY);
	ProjectileClass* projectile = this->Peek_Physical_Object()->As_ProjectileClass();
	if (projectile)
	{
		projectile->Set_Velocity(velocity);
	}
	stream.Get(this->attached);
	if (this->attached)
	{
		Peek_Physical_Object()->Flags = Peek_Physical_Object()->Flags & 0x7F00 | 0x0800;
		stream.Get(position.X,BITPACK_WORLD_POSITION_X);
		stream.Get(position.Y,BITPACK_WORLD_POSITION_Y);
		stream.Get(position.Z,BITPACK_WORLD_POSITION_Z);
		if (projectile)
		{
			Matrix3D m = projectile->Get_Transform();
			if (((m[0].W - position.X) * (m[0].W - position.X)) + ((m[1].W - position.Y) * (m[1].W - position.Y)) + ((m[2].W - position.Z) * (m[2].W - position.Z)) > 0.25)
			{
				m[0].W = position.X;
				m[1].W = position.Y;
				m[2].W = position.Z;
				projectile->Set_Transform(m);
			}
		}
		stream.Get(this->IsAttachedToMCT);
		stream.Get(this->attachedToDynamic);
		int attachid;
		stream.Get(attachid);
		attachObject = GameObjManager::Find_SmartGameObj(attachid);
		if (this->attachedToDynamic)
		{
			stream.Get(this->AttachLocation.X,BITPACK_VEHICLE_VELOCITY);
			stream.Get(this->AttachLocation.Y,BITPACK_VEHICLE_VELOCITY);
			stream.Get(this->AttachLocation.Z,BITPACK_VEHICLE_VELOCITY);
			stream.Get(this->AttachBoneIndex);
		}
		bool phys;
		stream.Get(phys);
		if (phys)
		{
			int physid;
			stream.Get(physid);
			StaticPhysClass *physobj = PhysicsSceneClass::Get_Instance()->Get_Static_Object_By_ID(physid);
			if ((phys) && (physobj->As_StaticAnimPhysClass()))
			{
				REF_PTR_SET(attachPhysics,physobj);
			}
			REF_PTR_RELEASE(physobj);
		}
	}
}

void C4GameObj::Export_Rare(BitStreamClass& stream)
{
	PhysicalGameObj::Export_Rare (stream);

	stream.Add(this->AmmoDef ? this->AmmoDef->Get_ID() : 0);
	stream.Add(((ScriptableGameObj *)this->Owner) ? ((ScriptableGameObj *)this->Owner)->Get_Network_ID() : 0);

	Vector3 position, velocity;

	ProjectileClass* projectile = this->Peek_Physical_Object()->As_ProjectileClass();
	if (projectile)
	{
		projectile->Get_Position (&position);
		projectile->Get_Velocity (&velocity);
	}

	stream.Add(velocity.X,BITPACK_VEHICLE_VELOCITY);
	stream.Add(velocity.Y,BITPACK_VEHICLE_VELOCITY);
	stream.Add(velocity.Z,BITPACK_VEHICLE_VELOCITY);

	stream.Add(this->attached);
	if (this->attached)
	{
		// TODO: Will disabling compression on these fix the C4 in PT bug?
		stream.Add(position.X,BITPACK_WORLD_POSITION_X);
		stream.Add(position.Y,BITPACK_WORLD_POSITION_Y);
		stream.Add(position.Z,BITPACK_WORLD_POSITION_Z);

		stream.Add(this->IsAttachedToMCT);
		stream.Add(this->attachedToDynamic);

		// FIX: don't use 0 if not attached, because it would attach the object to Arc Effects, which
		// have network ID 0 on clients
		stream.Add(((ScriptableGameObj *)attachObject) ? ((ScriptableGameObj*)attachObject)->Get_Network_ID() : 0xFFFFFFFF);
		if (this->attachedToDynamic)
		{
			stream.Add(this->AttachLocation.X,BITPACK_VEHICLE_VELOCITY);
			stream.Add(this->AttachLocation.Y,BITPACK_VEHICLE_VELOCITY);
			stream.Add(this->AttachLocation.Z,BITPACK_VEHICLE_VELOCITY);

			stream.Add(this->AttachBoneIndex);
		}

      stream.Add(this->attachPhysics != 0);
      if (this->attachPhysics)
         stream.Add(this->attachPhysics->Get_ID());
   }
}



void C4GameObj::Think()
{
	SimpleGameObj::Think();
	DynamicPhysClass* C4PhysDynamic = this->Peek_Physical_Object()->As_DynamicPhysClass();
	C4PhysDynamic->Update_Visibility_Status();

	if (!cNetwork::PServerConnection || !this->AmmoDef)
		return;

	this->SleepTime += TimeManager::FrameSeconds;

	this->Restore_Owner();
	int at = AmmoDef->AmmoType;
	switch (at)
	{
		case C4_REMOTE:
		{
			ScriptableGameObj* owner = this->Owner;
			if (owner)
			{
				SoldierGameObj* soldier = owner->As_SoldierGameObj();
				if (soldier && soldier->Detonate_C4())
				{
					this->Detonate();
				}
			}
			else
			{
				this->Defuse();
			}
		}
		break;

		case C4_TIMED:
		{
			this->TriggerTime -= TimeManager::FrameSeconds;
			if (this->TriggerTime <= 0)
			this->Detonate();
		}
		break;

		case C4_PROXIMITY:
		{
			this->TriggerTime -= TimeManager::FrameSeconds;
			if (this->TriggerTime <= 0)
			{
				this->TriggerTime += 0.25f;
				Vector3 position = this->Get_Transform().getPosition();
				float range = 0;
				if (this->DetonationMode == 1)
				{
					range = this->AmmoDef->C4TriggerRange1;
				}
				if (this->DetonationMode == 2)
				{
					range = this->AmmoDef->C4TriggerRange2;
				}
				if (this->DetonationMode == 3)
				{
					range = this->AmmoDef->C4TriggerRange3;
				}
				for (SLNode<GameObject> *node = GameObjManager::SmartGameObjList.Head(); node; node = node->Next())
				{
					SmartGameObj* object = (SmartGameObj *)node->Data();
					if (!this->Is_Enemy(object))
						continue;

					SoldierGameObj* soldier = object->As_SoldierGameObj();
					if (soldier && (soldier->Is_Dead() || soldier->Is_Destroyed()))
						continue;

					if (Vector3::Distance(position,object->Get_Transform().getPosition()) <= range)
					{
						this->Detonate();
						break;
					}
				}
			}
		}
		break;
	}
}


void C4GameObj::Post_Think()
{
	PhysicalGameObj::Post_Think();
	
	if (attached)
	{
		if (attachObject)
		{
			PhysicalGameObj* attachedAsPhys = attachObject->As_PhysicalGameObj();
			if (attachedAsPhys)
			{
				RenderObjClass* attachedModel = attachedAsPhys->Peek_Physical_Object()->Peek_Model();
				const Matrix3D& boneTransform = attachedModel ? attachedModel->Get_Bone_Transform(AttachBoneIndex) : attachedAsPhys->Get_Transform();
				
				Set_Position(boneTransform.applyTo(AttachLocation));
				
				RenderObjClass* model = Peek_Physical_Object()->Peek_Model();
				SoldierGameObj* attachedAsSoldier = attachedAsPhys->As_SoldierGameObj();
				if (model && attachedAsSoldier)
					model->Set_Hidden(attachedAsSoldier->Get_Vehicle() != NULL);
			}
		}
		else if (attachPhysics)
		{
			const Matrix3D& boneTransform = attachPhysics->Peek_Model()->Get_Bone_Transform(AttachBoneIndex);

			PhysicalGameObj::Set_Position(boneTransform.applyTo(AttachLocation));
		}
		else if (attachedToDynamic)
		{
			Set_Delete_Pending();
		}
	}
}


CollisionReactionType C4GameObj::Collision_Occurred(const CollisionEventClass& event)
{
	if (attached)
		return COLLISION_REACTION_2;
	
	PhysicalGameObj* physicalCollidee = NULL;
	BuildingGameObj* buildingCollidee = NULL;

	PhysClass& collideePhys = *event.physics;
	CombatPhysObserverClass* collideeObserver = (CombatPhysObserverClass*)collideePhys.Get_Observer();

	if (collideeObserver)
	{
		physicalCollidee = collideeObserver->As_PhysicalGameObj();
		buildingCollidee = collideeObserver->As_BuildingGameObj();
	}
	
	Restore_Owner();
	
	if (!physicalCollidee || !Owner)
	{
		if (buildingCollidee)
		{
			attached = true;
			attachObject = buildingCollidee;
			IsAttachedToMCT = collideePhys.Get_Factory().Chunk_ID() == 0x20A03 && ((BuildingAggregateClass&)collideePhys).Is_MCT();
		}
		else
		{
			if (physicalCollidee || event.physics->As_ProjectileClass())
				return COLLISION_REACTION_2;
			
			if (collideePhys.As_StaticAnimPhysClass())
			{
				collideePhys.Add_Ref();
				REF_PTR_RELEASE(attachPhysics);
				attachPhysics = &collideePhys;
				AttachBoneIndex = 0;
				RenderObjClass* collideeModel = collideePhys.Peek_Model();
				if (collideeModel)
				{
					Vector3 position;
					AttachBoneIndex = collideeModel->Get_Sub_Object_Bone_Index(event.bone);
					const Matrix3D& boneTransform = collideeModel->Get_Bone_Transform(AttachBoneIndex);
					Get_Position(&position);
					AttachLocation = boneTransform.Inverse_Rotate_Vector(position - boneTransform.getPosition());
				}
			}
			
			if (event.castResult && SurfaceEffectsManager::Is_Surface_Permeable(event.castResult->SurfaceType))
				return COLLISION_REACTION_2;
		}
	}
	else
	{
		VehicleGameObj* vehicleCollidee = physicalCollidee->As_VehicleGameObj();
		
		if (vehicleCollidee && vehicleCollidee == Owner->As_SoldierGameObj()->Get_Vehicle())
			return COLLISION_REACTION_2;
		
		if (physicalCollidee == Owner)
			return COLLISION_REACTION_2;
		
		attached = true;
		attachedToDynamic = true;
		attachObject = physicalCollidee;
		AttachBoneIndex = 0;
		RenderObjClass* collideeModel = collideePhys.Peek_Model();
		if (collideeModel)
		{
			Vector3 position;
			AttachBoneIndex = collideeModel->Get_Sub_Object_Bone_Index(event.bone);
			Get_Position(&position);
			const Matrix3D& boneTransform = collideeModel->Get_Bone_Transform(AttachBoneIndex);
			AttachLocation = boneTransform.Inverse_Rotate_Vector(position - boneTransform.getPosition());
		}
	}
	
	Peek_Physical_Object()->Enable_User_Control(true);
	
	if (CombatManager::I_Am_Server())
		Set_Object_Dirty_Bit(DB_RARE, true);
	
	return COLLISION_REACTION_1;
}

extern GameObject *ExplosionObj;
//RENEGADE_FUNCTION
void C4GameObj::Detonate()
//AT2(0x0070C8D0,0x0070BE90);
{
	if (CombatManager::I_Am_Server())
	{
		Restore_Owner();
		if (AmmoDef && AmmoDef->ExplosionDefID)
		{
			int damager_id = 0;
			ScriptableGameObj *obj = Owner;
			if (obj)
			{
				damager_id = obj->getNetworkId();
			}
			DamageableGameObj *damage = 0;
			if ((attached) && (attachedToDynamic))
			{
				damage = (DamageableGameObj *)attachObject.Get_Ptr();
			}
			ExplosionObj = this;
			ExplosionManager::Server_Explode(AmmoDef->ExplosionDefID,Get_Transform().Get_Translation(),damager_id,damage);
			ExplosionObj = 0;
		}
		if (attachObject.Get_Ptr())
		{
			BuildingGameObj *building = attachObject.Get_Ptr()->As_BuildingGameObj();
			if (building)
			{
				ExplosionObj = this;
				ExplosionManager::Explosion_Damage_Building(AmmoDef->ExplosionDefID,building,IsAttachedToMCT,(ArmedGameObj *)Owner.Get_Ptr());
				ExplosionObj = 0;
			}
		}
	}
	Set_Delete_Pending();
}

const C4GameObjDef & C4GameObj::Get_Definition( void ) const
{
	return (const C4GameObjDef &)*definition;
}
