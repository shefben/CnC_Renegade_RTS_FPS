#include "general.h"
#include "bitstream.h"
#include "PhysClass.h"
#include "MoveablePhysClass.h"
#include "RenderObjClass.h"
#include "AnimControlClass.h"
#include "VehicleGameObj.h"
#include "GameObjManager.h"
#include "SoldierGameObj.h"
#include "SimpleAnimControlClass.h"
#include "TimeManager.h"
#include "CombatManager.h"
#include "PhysicalGameObj.h"
#include "WeaponBagClass.h"
#include "WeaponClass.h"
#include "OffenseObjectClass.h"

void Do_Soldier_Update(SoldierGameObj *obj);



void PhysicalGameObj::Reset_Hibernating()
{
	if (Is_Hibernating())
		End_Hibernation();

	HibernationTime = min(HibernationTime + TimeManager::FrameSeconds * 2.f, 30.f);
}



RENEGADE_FUNCTION
void PhysicalGameObj::Hide_Muzzle_Flashes(bool hide)
   AT2(0x006A6B90,0x006A6430);

RENEGADE_FUNCTION
float PhysicalGameObj::Get_Facing() const
   AT2(0x006A5FE0,0x006A5880);

void PhysicalGameObj::Export_Creation(BitStreamClass& stream)
{
	ScriptableGameObj::Export_Creation(stream);
	Vector3 position;
	Get_Position(&position);
	float facing = Get_Facing();
	stream.Add(position.X,BITPACK_WORLD_POSITION_X);
	stream.Add(position.Y,BITPACK_WORLD_POSITION_Y);
	stream.Add(position.Z,BITPACK_WORLD_POSITION_Z);
	stream.Add(facing);
}

void PhysicalGameObj::Import_Creation(BitStreamClass& stream)
{
	ScriptableGameObj::Import_Creation(stream);
	Vector3 position;
	float facing = 0;
	stream.Get(position.X,BITPACK_WORLD_POSITION_X);
	stream.Get(position.Y,BITPACK_WORLD_POSITION_Y);
	stream.Get(position.Z,BITPACK_WORLD_POSITION_Z);
	stream.Get(facing);

	Matrix3D transform = Matrix3D::getZRotationMatrix(facing);
	transform.setPosition(position);
	Set_Transform(transform);
}

void PhysicalGameObj::Export_Frequent(BitStreamClass& stream)
{
	DamageableGameObj::Export_Frequent(stream);

	stream.Add(Ref && Ref.Get_Ptr()->As_PhysicalGameObj()->Physics->Peek_Model());
}



void PhysicalGameObj::Import_Frequent(BitStreamClass& stream)
{
	DamageableGameObj::Import_Frequent(stream);
	bool b;
	stream.Get(b);
	if (b)
	{
		MoveablePhysClass* moveable = Physics->As_MoveablePhysClass();
		if (moveable)
			moveable->Set_Velocity(Vector3());
	}
}



void PhysicalGameObj::Export_Rare
   (BitStreamClass& stream)
{
   DamageableGameObj::Export_Rare (stream);
   stream.Add_Terminated_String (this->Physics->Peek_Model()->Get_Name());
   if (this->Animation)
   {
      stream.Add_Terminated_String (this->Animation->Get_Animation_Name());
      stream.Add((uint32)this->Animation->Get_Current_Frame());
      stream.Add((uint32)this->Animation->Get_Target_Frame());
      stream.Add(this->Animation->Get_Mode());
   }
   else
   {
      stream.Add_Terminated_String ("");
      stream.Add(0);
      stream.Add(0);
      stream.Add(3);
   }
   ScriptableGameObj* attachObject = this->Ref;
   uint32 AttachObj = attachObject ? attachObject->Get_Network_ID() : 0;
   sint32 AttachBoneIndex     = this->AttachBoneIndex;
   stream.Add(AttachObj);
   stream.Add(AttachBoneIndex);
   stream.Add(this->Get_Player_Type());
   stream.Add(this->HudPokableIndicatorEnabled);

   VehicleGameObj* vehicle = this->As_VehicleGameObj();
   if (vehicle)
      if (this->Peek_Physical_Object()->Peek_Model())
	      stream.Add(this->Peek_Physical_Object()->Peek_Model()->Is_Hidden() ? true : false);
}

void PhysicalGameObj::Import_Rare
   (BitStreamClass& stream)
{
   DamageableGameObj::Import_Rare (stream);


   char buffer[1024];

   stream.Get_Terminated_String (buffer, sizeof(buffer));
   if (!this->Physics->Peek_Model() || ::_stricmp (buffer, this->Physics->Peek_Model()->Get_Name()))
   {
	  VehicleGameObj *v = this->As_VehicleGameObj();
	  if (v)
	  {
		  v->Release_Turret_Bones();
		  v->Shutdown_Wheel_Effects();
	  }
      this->Physics->Set_Model_By_Name (buffer);
	  if (v)
	  {
		  v->Aquire_Turret_Bones();
	  }
	  if (this->Animation)
	  {
		  this->Animation->Set_Model(this->Physics->Peek_Model());
	  }
	  if (v)
	  {
		  v->Init_Muzzle_Bones();
		  v->Init_Wheel_Effects();
		  v->Update_Damage_Meshes();
		  WeaponBagClass *w = v->Get_Weapon_Bag();
		  if ((w->Get_Index()) && (w->Get_Index() < w->Get_Count()))
		  {
             w->Peek_Weapon(w->Get_Index())->Clear_Firing_Sound();
		  }
	  }
	  this->Hide_Muzzle_Flashes(true);
	  if (this->As_SoldierGameObj())
	  {
		  ((SoldierGameObj *)this)->Set_Model(buffer);
		  Do_Soldier_Update(this->As_SoldierGameObj());
	  }
   }


   uint32 animationFrame = 0;
   uint32 animationTargetFrame = 0;
   uint32 animationMode = 0;

   stream.Get_Terminated_String (buffer, sizeof(buffer));
   stream.Get(animationFrame);
   stream.Get(animationTargetFrame);
   stream.Get(animationMode);

   if (*buffer)
   {
      if (!this->Animation)
         this->Set_Animation (buffer, true, 0);

      if (this->Animation)
      {
         this->Animation->Set_Animation (buffer, 0, (float)animationFrame);
         this->Animation->Set_Target_Frame ((float)animationTargetFrame);

         if (animationMode == ANIM_MODE_STOP)
            this->Animation->Set_Mode (ANIM_MODE_STOP, float(animationFrame));
         else
            this->Animation->Set_Mode ((AnimMode)animationMode, -1);
      }
   }
   // stop animation by unsetting looping mode
   else if (this->Animation)
      this->Animation->Set_Mode ((AnimMode)0, -1);

   stream.Get(this->AttachObj);
   stream.Get(this->AttachBoneIndex);

   if (this->AttachObj)
   {
      PhysicalGameObj* object = GameObjManager::Find_PhysicalGameObj (this->AttachObj);

      this->Ref = object;

      if (object)
         this->AttachObj = 0;
      else
         this->Reset_Hibernating();
   }
   else
      this->Ref = 0;

   sint32 teamId = -1;
   stream.Get(teamId);

   this->Set_Player_Type (teamId);
   stream.Get(this->HudPokableIndicatorEnabled);
   if (this->As_VehicleGameObj())
   {
      bool hidden;
	  stream.Get(hidden);
	  int h = hidden ? 1 : 0;
	  if ((this->Peek_Physical_Object()->Peek_Model()) && (this->Peek_Physical_Object()->Peek_Model()->Is_Hidden() != h))
	  {
         this->Peek_Physical_Object()->Peek_Model()->Set_Hidden(h);
		 if (!h)
		 {
			this->As_VehicleGameObj()->Update_Damage_Meshes();
         }
	  }
   }
}

RENEGADE_FUNCTION
void Create_Animation_Name
   (StringClass& target, const char* animation, const char* model)
   AT2(0x006DB000,0x006DA8A0);

void PhysicalGameObj::Set_Animation
   (const char* animation, bool loop, float firstframe)
{
   if (!this->Animation)
   this->Animation = new SimpleAnimControlClass;

   if (!animation || !*animation)
      this->Animation->Set_Animation (static_cast<HAnimClass*> (0), 0, 0);
   else
   {
      StringClass name;
	  name = animation;

      if (!::strchr (animation, '.'))
      {
         StringClass newName;
         ::Create_Animation_Name (newName, animation, this->Physics->Peek_Model()->Get_Name());
         name = newName;
      }

      if (!*name)
         return;

      this->Animation->Set_Model (this->Physics->Peek_Model());
      this->Animation->Set_Animation (name, 0, firstframe);
      this->Animation->Set_Mode ((AnimMode)(loop ? 1 : 0), -1);
      this->Animation->Update (0);
   }

   this->Set_Object_Dirty_Bit(DB_RARE,true);
}

Vector3 PhysicalGameObj::Get_Bullseye_Position()
{
	RenderObjClass* model = Peek_Physical_Object()->Peek_Model();

	// Allow custom target bone.
	int boneIndex = model->Get_Bone_Index("target");
	if (boneIndex > 0)
		return model->Get_Bone_Transform(boneIndex).getPosition();
	
	// Fix the transport helicopter
	boneIndex = model->Get_Bone_Index("V_FUSELAGE");
	if (boneIndex > 0)
		return model->Get_Bone_Transform(boneIndex).getPosition();

	return model->Get_Position();
}

RENEGADE_FUNCTION
bool PhysicalGameObj::Load (ChunkLoadClass&)
AT2(0x006A50C0,0x006A4960);

void PhysicalGameObj::Apply_Damage_Extended( const OffenseObjectClass & damager, float scale,
			const	Vector3 & direction, const char * collision_box_name )
{
	Apply_Damage(damager,scale,-1);
}

void PhysicalGameObj::Apply_Damage( const OffenseObjectClass & damager, float scale, int alternate_skin )
{
	if (CombatManager::Can_Damage(damager.Get_Owner(),this))
	{
		DamageableGameObj::Apply_Damage(damager,scale,alternate_skin);
	}
}

const PhysicalGameObjDef & PhysicalGameObj::Get_Definition( void ) const
{
	return (const PhysicalGameObjDef &)*definition;
}

void PhysicalGameObj::Begin_Hibernation()
{
}

void PhysicalGameObj::End_Hibernation()
{
}

void PhysicalGameObj::Reset_Radar_Blip_Color_Type()
{
	int playertype = Get_Player_Type();
	switch (playertype)
	{
	case -3:
		RadarBlipColor = 3;
		break;
	case -1:
		RadarBlipColor = 4;
		break;
	case 0:
		RadarBlipColor = 0;
		break;
	case 1:
		RadarBlipColor = 1;
		break;
	default:
		RadarBlipColor = 2;
	}
}

void PhysicalGameObj::Set_Collision_Group(int group)
{
	Physics->Set_Collision_Group((Collision_Group_Type)group);
}
