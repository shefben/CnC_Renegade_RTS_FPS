#include "general.h"
#include "SmartGameObj.h"
#include "CClientControl.h"
#include "SoldierGameObj.h"
#include "cNetwork.h"
#include "cRemoteHost.h"
#include "CombatManager.h"
#include "MoveablePhysClass.h"
#include "LogicalListenerClass.h"
#include "SaveLoadSystemClass.h"
#include "TimeManager.h"
#include "Random2Class.h"
#include "GameObjManager.h"
#include "RenderObjClass.h"
#include "cSinglePlayerData.h"
#include "cconnection.h"
#include "WeaponViewClass.h"
#include "WeaponBagClass.h"
#include "WeaponClass.h"
#include "PlayerDataClass.h"
#include "OffenseObjectClass.h"
class CameraClass;
extern float ServerVersion;

REF_DEF2(FirstPersonOffsetTweak, Vector3, 0x0085CB10, 0x0085BCF8);

void Reset_First_Person_Offset_Tweak()
{
	FirstPersonOffsetTweak.X = 0;
	FirstPersonOffsetTweak.Y = 0;
	FirstPersonOffsetTweak.Z = 0;
}

void SmartGameObj::Alloc_Stealth_Effect()
{
	if (!stealthEffect)
	{
		stealthEffect = new StealthEffectClass();
		stealthEffect->Set_Fade_Distance(Get_Stealth_Fade_Distance());
	}
}

RENEGADE_FUNCTION
bool SmartGameObj::Is_Controlled_By_Me()
AT2(0x0069EED0,0x0069E770);
void SmartGameObj::Export_Creation(BitStreamClass& stream)
{
	PhysicalGameObj::Export_Creation(stream);
	stream.Add(clientId);
}
void SmartGameObj::Import_Creation(BitStreamClass& stream)
{
	PhysicalGameObj::Import_Creation(stream);
	int clientid;
	stream.Get(clientid);
	Set_Control_Owner(clientid);
	if (clientid == CombatManager::Get_My_Id())
	{
		ActionParamsStruct params;
		action.Follow_Input(params);
		CombatManager::Set_The_Star(this->As_SoldierGameObj(),true);
	}
}
void SmartGameObj::Export_Frequent(BitStreamClass& stream)
{
	ArmedGameObj::Export_Frequent(stream);
	if (cNetwork::Get_Server_Rhost(cNetwork::lastUpdatedClientId)->getVersion() >= 4.0f)
	{
		stream.Add(stealthed);
	}
	Export_Control_Sc(stream);
}


void SmartGameObj::Import_Frequent(BitStreamClass& stream)
{
	ArmedGameObj::Import_Frequent(stream);

	if (cNetwork::Get_Client_Rhost()->getVersion() >= 4.0f)
	{
		bool newStealth;
		stream.Get(newStealth);
		if(newStealth != stealthed)
		{
			Enable_Stealth(newStealth);
		}
	}
	
	if (!Is_Controlled_By_Me())
	{
		Import_Control_Sc(stream);
	}
	else
	{
		stream.Flush();
	}
}

void SmartGameObj::Apply_Control()
{
   bool weaponChanged = false;

   if (this->control.Get_Boolean (ControlClass::BooleanNextWeapon))
   {
      Reset_First_Person_Offset_Tweak();

      this->Get_Weapon_Bag()->Select_Next();
      weaponChanged = true;
   }

   if (this->control.Get_Boolean (ControlClass::BooleanPrevWeapon))
   {
      Reset_First_Person_Offset_Tweak();

      this->Get_Weapon_Bag()->Select_Prev();
      weaponChanged = true;
   }

   for (sint32 number = -1; number <= 9; ++number)
   {
      if (!this->control.Get_Boolean ((ControlClass::BooleanControl)(number + 11)))
         continue;

      Reset_First_Person_Offset_Tweak();

      this->Get_Weapon_Bag()->Select_Key_Number (number);
      weaponChanged = true;
   }

   WeaponClass* weapon = this->Get_Weapon_Bag()->Get_Selected_Weapon();

   if (weaponChanged)
   {
      SoldierGameObj* soldier = this->As_SoldierGameObj();
      if (soldier)
         if (soldier->Is_Sniping() && (!weapon || !weapon->Get_Definition()->CanSnipe))
            soldier->Set_Is_Sniping();
   }

   this->physController.velocity.X   = this->control.analog[0];
   this->physController.velocity.Y   = this->control.analog[1];
   this->physController.velocity.Z   = this->control.analog[2];
   this->physController.turnVelocity = this->control.analog[3];

   if (weapon)
   {
      if (this->controlEnabled)
      {
		 if (!this->DeletePending)
         {
            weapon->Set_Primary_Triggered (this->control.Get_Boolean (ControlClass::BooleanPrimaryFire));
            weapon->Set_Secondary_Triggered (this->control.Get_Boolean (ControlClass::BooleanSecondaryFire));

            if (this->control.Get_Boolean (ControlClass::BooleanDetonateC4))
               weapon->Next_C4_Detonation_Mode();

            if (this->control.Get_Boolean (ControlClass::BooleanReload))
               weapon->Force_Reload();

            if (this->control.Get_Boolean (ControlClass::BooleanPrimaryFire) || this->control.Get_Boolean (ControlClass::BooleanSecondaryFire))
               this->stealthDisableTime = 5.0f;
         }
      } else
      {
         weapon->Set_Primary_Triggered (false);
         weapon->Set_Secondary_Triggered (false);
      }
   }
}

void SmartGameObj::Generate_Control()
{
   ActionClass::_acting = true;

   if (CombatManager::I_Am_Server())
   {
      if (this->clientId == -99999 || !this->Is_Human_Controlled())
         this->action.Act();
   }

   if (cNetwork::PClientConnection && this->clientId == (signed)cNetwork::PClientConnection->Get_Local_Id())
   {
      this->action.Act();
      if (PClientControl)
         PClientControl->Set_Update_Flag (!this->DeletePending ? (sint32)this->NetworkID : -1);
   }
   
   ActionClass::_acting = false;
}

void SmartGameObj::Export_State_Cs(BitStreamClass &BitStream)
{
	ArmedGameObj::Export_State_Cs(BitStream);
}

void SmartGameObj::Import_State_Cs(BitStreamClass& stream)
{
	ArmedGameObj::Import_State_Cs(stream);
}

void SmartGameObj::Think()
{
	if (controlEnabled)
	{
		Apply_Control();
	}
	else
	{
		physController.velocity.X = 0;
		physController.velocity.Y = 0;
		physController.velocity.Z = 0;
		physController.turnVelocity = 0;
		WeaponClass* weapon = this->Get_Weapon();
		if (weapon)
		{
			weapon->Set_Primary_Triggered(false);
			weapon->Set_Secondary_Triggered(false);
		}
	}
	enemySeenDisableTime = enemySeenDisableTime - TimeManager::FrameSeconds;
	if (enemySeenDisableTime < 0)
	{
		enemySeenDisableTime += .5f + (FreeRandom() % 4096) * (.5f / 4096.f);

		if (enemySeenEnabled)
		{
			for (SLNode<SmartGameObj>* node = (SLNode<SmartGameObj>*)GameObjManager::SmartGameObjList.Head(); node; node = node->Next())
			{
				SmartGameObj* object = node->Data();
				if (Is_Enemy(object) && object->Is_Visible() && Is_Obj_Visible(object))
				{
					if (object != CombatManager::Get_The_Star() && object->Peek_Physical_Object())
					{
						RenderObjClass* model = object->Peek_Physical_Object()->Peek_Model();
						if (model && model->Is_Hidden())
							continue;
					}
					
					for (int i = 0; i < Observers.Count(); i++)
						Observers[i]->Enemy_Seen(this, object);
				}
			}
		}
	}

	if (stealthTime > 0)
		stealthTime -= TimeManager::FrameSeconds;
	
	if (stealthDisableTime > 0)
		stealthDisableTime -= TimeManager::FrameSeconds;
	
	if ((stealthed || stealthTime > 0) && stealthDisableTime <= 0)
	{
		Alloc_Stealth_Effect();
		stealthEffect->Enable_Stealth(true);
		if (CombatManager::Get_The_Star())
			stealthEffect->Set_Friendly(Is_Teammate(CombatManager::Get_The_Star()));
		stealthEffect->Set_Broken(Defense.Get_Health() / Defense.Get_Health_Max() <= .25f);
		Peek_Physical_Object()->Add_Effect_To_Me(stealthEffect);
	}
	else if (stealthEffect)
		stealthEffect->Enable_Stealth(false);
	
	ScriptableGameObj::Think();
}

RENEGADE_FUNCTION
bool SmartGameObj::Is_Obj_Visible
   (PhysicalGameObj* object)
   AT2(0x0069F580,0x0069EE20);

void SmartGameObj::Enable_Stealth(bool stealth)
{
	stealthed = stealth;
	if (stealth)
	{
		Alloc_Stealth_Effect();
	}
	else
	{
		if (stealthEffect)
		{
			if (!cSinglePlayerData::Is_Single_Player())
			{
				Peek_Physical_Object()->Remove_Effect_From_Me(stealthEffect);
				if (this == CombatManager::Get_The_Star())
				{
					WeaponViewClass::HandsPhysObj->Remove_Effect_From_Me(stealthEffect);
				}
				REF_PTR_RELEASE(stealthEffect);
			}
		}
	}
}



void SmartGameObj::Set_Player_Data(PlayerDataClass* _playerData)
{
	if (playerData)
		playerData->Owner = NULL;

	playerData = _playerData;

	if (playerData)
		playerData->Owner = this;
}



void SmartGameObj::Post_Think()
{
	ArmedGameObj::Post_Think();
	if (!Is_Delete_Pending())
		control.booleanA = 0;
}

void SmartGameObj::Init
   (const SmartGameObjDef& oDefinition)
{
   ArmedGameObj::Init(oDefinition);
   SmartGameObj::Copy_Settings(oDefinition);
}

void SmartGameObj::Copy_Settings
   (const SmartGameObjDef& oDefinition)
{
	if (Peek_Physical_Object()->As_MoveablePhysClass())
	{
		Peek_Physical_Object()->As_MoveablePhysClass()->Set_Controller(&physController);
	}
	Register_Listener();
	if (oDefinition.stealthed)
	{
		Enable_Stealth(true);
	}
}

RENEGADE_FUNCTION
void SmartGameObj::Register_Listener()
AT2(0x0069FA20,0x0069F2C0);

void SmartGameObj::Re_Init
   (const SmartGameObjDef& oDefinition)
{
	ArmedGameObj::Re_Init(oDefinition);
	if (listener)
	{
		listener->Remove_From_Scene();
	}
	if (stealthEffect)
	{
		stealthEffect->Release_Ref();
		stealthEffect = 0;
		stealthed = false;
		stealthTime = 0;
		stealthDisableTime = 0;
	}
	Copy_Settings(oDefinition);
}

bool SmartGameObj::Load(ChunkLoadClass &oLoad)
{
	uint32 owner = 0;
	void *pointer;
	while (oLoad.Open_Chunk())
	{
		switch (oLoad.Cur_Chunk_ID())
		{
		case 0x364C9F09:
			PhysicalGameObj::Load(oLoad);
			break;
		case 0x364C9F0F:
			ArmedGameObj::Load(oLoad);
			break;
		case 0x364C9F0A:
			pointer = 0;
			while (oLoad.Open_Micro_Chunk())
			{
				switch (oLoad.Cur_Micro_Chunk_ID())
				{
				case 1:
					oLoad.Read(&controlEnabled,1);
					break;
				case 4:
					oLoad.Read(&owner,4);
					break;
				case 8:
					oLoad.Read(&pointer,4);
					break;
				case 9:
					oLoad.Read(&enemySeenEnabled,1);
					break;
				case 0xB:
					oLoad.Read(&enemySeenDisableTime,4);
					break;
				case 0xC:
					oLoad.Read(&playerData,4);
					break;
				case 0xD:
					oLoad.Read(&stealthed,1);
					break;
				case 0xE:
					oLoad.Read(&stealthTime,4);
					break;
				case 0xF:
					oLoad.Read(&stealthDisableTime,4);
					break;
				}
				oLoad.Close_Micro_Chunk();
			}
			if (pointer)
			{
				SaveLoadSystemClass::Register_Pointer(pointer,&physController);
			}
			break;
		case 0x364C9F0B:
			control.Load(oLoad);
			break;
		case 0x364C9F0C:
			physController.Load(oLoad);
			break;
		case 0x364C9F0D:
			action.Load(oLoad);
			break;
		case 0x364C9F11:
			Alloc_Stealth_Effect();
			stealthEffect->Load(oLoad);
			break;
		}
		oLoad.Close_Chunk();
	}
	if (playerData)
	{
		SaveLoadSystemClass::Request_Pointer_Remap((void **)&playerData);
	}
	Set_Control_Owner(owner);
	SaveLoadSystemClass::Register_Post_Load_Callback(this);
	return true;
}



void SmartGameObj::Begin_Hibernation()
{
	PhysicalGameObj::Begin_Hibernation();
	physController.Reset();

	if (listener)
		listener->Remove_From_Scene();

	action.Begin_Hibernation();
}




void SmartGameObj::End_Hibernation()
{
	PhysicalGameObj::End_Hibernation();
	
	if (listener)
		listener->Add_To_Scene(true);
	
	action.End_Hibernation();
}

void SmartGameObj::Apply_Damage( const OffenseObjectClass & damager, float scale, int alternate_skin )
{
	if ((scale * damager.Get_Damage()) > 0)
	{
		if (stealthEffect)
		{
			stealthEffect->Damage_Occured();
		}
	}
	PhysicalGameObj::Apply_Damage(damager,scale,alternate_skin);
}


const SmartGameObjDef & SmartGameObj::Get_Definition( void ) const
{
	return (const SmartGameObjDef &)*definition;
}
