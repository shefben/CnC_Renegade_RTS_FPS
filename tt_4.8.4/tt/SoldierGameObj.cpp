#include "general.h"
#include "SoldierGameObj.h"
#include "VehicleGameObj.h"
#include "CollisionEventClass.h"
#include "MoveablePhysClass.h"
#include "VehicleGameObjDef.h"
#include "cNetwork.h"
#include "cRemoteHost.h"
#include "Phys3Class.h"
#include "CombatManager.h"
#include "ArmorWarheadManager.h"
#include "PhysicsSceneClass.h"
#include "CollisionMath.h"
#include "HumanPhysClass.h"
#include "SoldierGameObjDef.h"
#include "SoldierObserverClass.h"
#include "WWMath.h"
#include "TimeManager.h"
#include "UnitCoordinationZoneMgr.h"
#include "DefinitionMgrClass.h"
#include "ExplosionManager.h"
#include "DynamicSpeechAnimClass.h"
#include "SurfaceEffectsManager.h"
#include "LineSegClass.h"
#include "physcoltest.h"
#include "CombatMaterialEffectManager.h"
#include "TransitionInstanceClass.h"
#include "Random2Class.h"
#include "CCameraClass.h"
#include "WWAudioClass.h"
#include "AudibleSoundClass.h"
#include "cGameType.h"
#include "ObjectLibraryManager.h"
#include "GlobalSettingsDef.h"
#include "encyclopedia.h"
#include "RefCountedReferencerClass.h"
#include "TransitionManager.h"
#include "HUDInfo.h"
#include "input.h"
#include "PlayerTerminalClass.h"
#include "BonesManager.h"
#include "AnimControlClass.h"
#include "OffenseObjectClass.h"
#include "PlayerDataClass.h"
#include "WeaponBagClass.h"
#include "WeaponClass.h"
#include "SimpleGameObj.h"
#include "SimpleGameObjDef.h"
#include "WW3DAssetManager.h"
RenderObjClass *Create_Render_Obj_From_Filename(const char *filename);
void SoldierGameObj::Set_Emot_Icon(const char* model_name, float duration)
{
	if ( EmotIconModel != NULL )
	{
		if ( EmotIconModel->Is_In_Scene() )
		{
			EmotIconModel->Remove ();
		}
		REF_PTR_RELEASE( EmotIconModel );
	}
	if ( model_name != NULL )
	{
		EmotIconModel = ::Create_Render_Obj_From_Filename( model_name );
		if ( EmotIconModel != NULL )
		{
			const char *name = EmotIconModel->Get_Name ();
			StringClass anim_name;
			anim_name.Format ("%s.%s", name, name);
			HAnimClass *anim = WW3DAssetManager::TheInstance->Get_HAnim( anim_name );
			if ( anim != NULL )
			{
				EmotIconModel->Set_Animation( anim, 0.0F, RenderObjClass::ANIM_MODE_LOOP );
				REF_PTR_RELEASE( anim );
			}
			Matrix3D tm = Get_Transform();
			tm.Set_Translation (tm.Get_Translation() + Vector3 (0, 0, 2.0));
			EmotIconModel->Set_Transform( tm );
			COMBAT_SCENE->Add_Render_Object( EmotIconModel );
			EmotIconTimer = duration;
		}
	}
}

RENEGADE_FUNCTION
void SoldierGameObj::Adjust_Skeleton(float height, float width)
AT2(0x006CE980,0x006CE220);

RENEGADE_FUNCTION
void SoldierGameObj::Update_Back_Gun()
AT2(0x006CC5E0,0x006C6BC0);

RENEGADE_FUNCTION
void SoldierGameObj::Prepare_Speech_Framework()
AT2(0x006C7320,0x006CBE80);

void SoldierGameObj::Enter_Vehicle( VehicleGameObj * vehicle, const char * anim_name )
{
	Vehicle = vehicle;
	HumanState.Set_State( HumanStateClass::IN_VEHICLE );
	AnimationName = anim_name;
	HumanState.Force_Animation( anim_name, false );
}

void SoldierGameObj::Set_Model(const char *model_name)
{
	Peek_Physical_Object()->Set_Model_By_Name(model_name);
	HumanState.Set_Anim_Control( (HumanAnimControlClass *)Get_Anim_Control() );
}

void SoldierGameObj::Set_AI_State(SoldierAIState aistate)
{
	this->AIState = aistate;
}

Vector3 SoldierGameObj::Get_Bullseye_Position()
{
	if (Vehicle)
		return Vehicle->Get_Bullseye_Position();

	RenderObjClass* model = Peek_Physical_Object()->Peek_Model();

	// Allow custom target bone.
	int boneIndex = model->Get_Bone_Index("target");
	if (boneIndex > 0)
		return model->Get_Bone_Transform(boneIndex).getPosition();
	
	// Shoot at chest (use "C HEAD" if the AI should aim for the head)
	boneIndex = model->Get_Bone_Index("C SPINE1");
	if (boneIndex > 0)
		return model->Get_Bone_Transform(boneIndex).getPosition();

	return model->Get_Position();
}
unsigned int Get_Shield_Type_Number(GameObject *obj);
extern unsigned int UnsquishableArmor;
extern unsigned int UnsquishableArmor2;
extern unsigned int UnsquishableArmor3;
extern unsigned int UnsquishableArmor4;
extern bool Unsquishable;
CollisionReactionType SoldierGameObj::Collision_Occurred(const CollisionEventClass& o1)
{
	PhysClass* oPhys = o1.physics;
	if (!oPhys)
		return COLLISION_REACTION_DEFAULT;

	CombatPhysObserverClass* oObserver = (CombatPhysObserverClass *)oPhys->Get_Observer();
	if (!oObserver)
		return COLLISION_REACTION_DEFAULT;

	PhysicalGameObj* oPhysical = oObserver->As_PhysicalGameObj();
	if (!oPhysical)
		return COLLISION_REACTION_DEFAULT;

	MoveablePhysClass* oMoveable = oPhys->As_MoveablePhysClass();
	if (!oMoveable)
		return COLLISION_REACTION_DEFAULT;

	VehicleGameObj* Vehicle = oPhysical->As_VehicleGameObj();
	if (!Vehicle)
		return COLLISION_REACTION_DEFAULT;
	
	Vector3 oVelocity, oPos1, oPos2;

	oMoveable->Get_Velocity (&oVelocity);
	this->Get_Position(&oPos1);
	oPhysical->Get_Position(&oPos2);

	float fVelocity = oVelocity.Length();

	if (fVelocity < Vehicle->Get_Squish_Velocity())
		return COLLISION_REACTION_DEFAULT;

	if (Unsquishable)
	{
		unsigned int armor = Get_Shield_Type_Number((GameObject *)this);
		if (armor == UnsquishableArmor || armor == UnsquishableArmor2 || armor == UnsquishableArmor3 || armor == UnsquishableArmor4)
			return COLLISION_REACTION_DEFAULT;
	}

	Vector3 oDiff = oPos1 - oPos2;
	if (this->HumanState.Get_State() == HumanStateClass::DEATH || oVelocity.X * oDiff.X + oVelocity.Y * oDiff.Y + oVelocity.Z * oDiff.Z < 0)
		return COLLISION_REACTION_DEFAULT;

	SoldierGameObj* oDriver = Vehicle->Get_Driver();
	ArmedGameObj* oDamager = oDriver;
	if (!oDamager)
		oDamager = Vehicle;

	if (!this->Is_Enemy(oDamager->As_DamageableGameObj()))
		return COLLISION_REACTION_DEFAULT;


	OffenseObjectClass offense(10000, 1, oDamager);
	Apply_Damage_Extended(offense, 10000.f, oVelocity, 0);

	if (oDriver && oDriver->playerData)
		++oDriver->playerData->KillsFromVehicle;

	return COLLISION_REACTION_DEFAULT;
}



void SoldierGameObj::Export_Occasional(BitStreamClass &oStream)
{
	DamageableGameObj::Export_Occasional(oStream);
	WeaponBag->Export_Weapon_List(oStream);

	if (cNetwork::Get_Server_Rhost(cNetwork::lastUpdatedClientId)->getVersion() >= 4.0f)
	{
		oStream.Add(HumanState.Get_State_Flag(HumanStateClass::SNIPING_FLAG));
	}
}



void SoldierGameObj::Import_Occasional(BitStreamClass &oStream)
{
	DamageableGameObj::Import_Occasional(oStream);
	WeaponBag->Import_Weapon_List(oStream);
	if (cNetwork::Get_Client_Rhost()->getVersion() >= 4.0f)
	{
		bool sniping = oStream.Get(sniping);
		if (sniping != HumanState.Get_State_Flag(HumanStateClass::SNIPING_FLAG) && !Is_Controlled_By_Me())
		{
			this->HumanState.Toggle_State_Flag(HumanStateClass::SNIPING_FLAG);
		}
	}
}



void SoldierGameObj::Export_Frequent(BitStreamClass& stream)
{
	stream.Add(HumanState.Get_State() == HumanStateClass::IN_VEHICLE);
	if (HumanState.Get_State() != HumanStateClass::IN_VEHICLE)
	{
		WeaponClass* weapon = ArmedGameObj::Get_Weapon();
		stream.Add(weapon != NULL);
		if (weapon)
		{
			stream.Add(weapon->Get_Definition()->Get_ID());
			stream.Add(weapon->Get_Total_Rounds());
		}
		
		Vector3 position;
		Get_Position(&position);
		if (cNetwork::Get_Server_Rhost(cNetwork::lastUpdatedClientId)->getVersion() >= 4.0f)
		{
			stream.Add(position.X);
			stream.Add(position.Y);
			stream.Add(position.Z);
		}
		else
		{
			stream.Add(position.X,BITPACK_WORLD_POSITION_X);
			stream.Add(position.Y,BITPACK_WORLD_POSITION_Y);
			stream.Add(position.Z,BITPACK_WORLD_POSITION_Z);
		}
		
		stream.Add((int)HumanState.Get_State(),BITPACK_HUMAN_STATE);
		stream.Add((int)HumanState.Get_Sub_State(),BITPACK_HUMAN_SUB_STATE);
		
		if (HumanState.Get_State() == HumanStateClass::AIRBORNE)
		{
			Vector3 velocity;
			Get_Velocity(velocity);
			stream.Add(velocity.X);
			stream.Add(velocity.Y);
			stream.Add(velocity.Z);
		}
		
		if (HumanState.Get_State() == HumanStateClass::ANIMATION ||
			HumanState.Get_State() == HumanStateClass::IN_VEHICLE ||
			HumanState.Get_State() == HumanStateClass::TRANSITION)
			stream.Add_Terminated_String(AnimationName);
		
		stream.Add(SpecialDamageMode != 0);
		if (SpecialDamageMode)
			stream.Add(SpecialDamageMode);
	}
	
	SmartGameObj::Export_Frequent(stream);
	
	// BACKCOMPAT: old wall lag fix
	if (cNetwork::Get_Server_Rhost(cNetwork::lastUpdatedClientId)->getVersion() < 4.0f)
	{
		Vector3 position;
		Get_Position(&position);
		stream.Add(position.X);
		stream.Add(position.Y);
		stream.Add(position.Z);
	}
}



void SoldierGameObj::Import_Frequent(BitStreamClass& stream)
{
	bool b;
	stream.Get(b);
	if (!b)
	{
		WeaponClass* weapon = Get_Weapon();
		stream.Get(b);
		if (b)
		{
			uint weaponDefinitionId = stream.Get(weaponDefinitionId);
			int weaponRounds = stream.Get(weaponRounds);
			weaponRounds;
			
			if (!weapon || weapon->Get_Definition()->Get_ID() != weaponDefinitionId)
			{
				WeaponBag->Select_Weapon_ID(weaponDefinitionId);
				weapon = Get_Weapon();
			}
			
			//if (weapon && !weapon->Is_Triggered())
				//weapon->Set_Total_Rounds(weaponRounds);
		}
		else
			if (weapon)
				WeaponBag->Deselect();
		
		Vector3 position;
		
		if (cNetwork::Get_Client_Rhost()->getVersion() >= 4.0f)
		{
			stream.Get(position.X);
			stream.Get(position.Y);
			stream.Get(position.Z);
		}
		else
		{
			stream.Get(position.X,BITPACK_WORLD_POSITION_X);
			stream.Get(position.Y,BITPACK_WORLD_POSITION_Y);
			stream.Get(position.Z,BITPACK_WORLD_POSITION_Z);
		}

		Interpret_Sc_Position_Data(position);
		
		HumanStateClass::HumanStateType newHumanState = (HumanStateClass::HumanStateType)stream.Get((uint32&)newHumanState,BITPACK_HUMAN_STATE);
		int newHumanSubState = stream.Get(newHumanSubState,BITPACK_HUMAN_SUB_STATE);
		
		Vector3 velocity;
		if (newHumanState == HumanStateClass::AIRBORNE)
		{
			stream.Get(velocity.X);
			stream.Get(velocity.Y);
			stream.Get(velocity.Z);
		}
		
		if (HumanState.Is_Locked())
		{
			stream.Flush();
			return;
		}
		
		char newAnimationName[80] = "";
		if (newHumanState == HumanStateClass::ANIMATION ||
			newHumanState == HumanStateClass::IN_VEHICLE ||
			newHumanState == HumanStateClass::TRANSITION)
			stream.Get_Terminated_String(newAnimationName, 80, false);
		
		Interpret_Sc_State_Data(newHumanState, newHumanSubState, newAnimationName, velocity, position);
		stream.Get(b);
		if (b)
		{
			ArmorWarheadManager::SpecialDamageType newSpecialDamageType = (ArmorWarheadManager::SpecialDamageType)stream.Get((sint32&)newSpecialDamageType);
			if (newSpecialDamageType != (int)SpecialDamageMode)
				Set_Special_Damage_Mode(newSpecialDamageType, NULL);
		}
		
		if (HumanState.Get_State() == HumanStateClass::DIVE)
			return;
	}
	
	SmartGameObj::Import_Frequent(stream);
	
	// BACKCOMPAT: old wall lag fix
	if (!stream.Is_Flushed())
	{
		Vector3 position;
		stream.Get(position.X);
		stream.Get(position.Y);
		stream.Get(position.Z);
		Interpret_Sc_Position_Data(position);
	}
}



void SoldierGameObj::Export_State_Cs(BitStreamClass &oStream)
{
	oStream.Add(HumanState.Get_State_Flag(HumanStateClass::SNIPING_FLAG));
	
	if (cNetwork::Get_Client_Rhost()->getVersion() < 4.0f)
		oStream.Add(false);

	SmartGameObj::Export_State_Cs(oStream);
}



void SoldierGameObj::Import_State_Cs(BitStreamClass& stream)
{
	bool sniping = stream.Get(sniping);
	if ((this->WeaponBag->Get_Selected_Weapon()) && (this->WeaponBag->Get_Selected_Weapon()->Get_Definition()->CanSnipe))
	{
		if (sniping != this->HumanState.Get_State_Flag(HumanStateClass::SNIPING_FLAG))
		{
			this->HumanState.Toggle_State_Flag(HumanStateClass::SNIPING_FLAG);
			Set_Object_Dirty_Bit(DB_OCCASIONAL, true);
		}
	}
	bool b;
	uint32 u;
	if (cNetwork::Get_Server_Rhost(((cPacket&)stream).Get_Sender_Id())->getVersion() < 4.0f)
		if (stream.Get(b))
			stream.Get(u);

	SmartGameObj::Import_State_Cs (stream);
}



RENEGADE_FUNCTION
void SoldierGameObj::Set_Special_Damage_Mode(ArmorWarheadManager::SpecialDamageType, ArmedGameObj*)
AT2(0x006CF9F0, 0x006CF290);



void SoldierGameObj::Interpret_Sc_Position_Data(const Vector3& _newPosition)
{
	Vector3 newPosition = _newPosition;
	Vector3 newVelocity(0, 0, 0);
	
	if (HumanState.Get_State() != HumanStateClass::TRANSITION && Is_In_Elevator())
	{
		Vector3 oldPosition;
		Get_Position(&oldPosition);
		
		newPosition.Z = oldPosition.Z;
	}
	
	if (this == CombatManager::Get_The_Star()) // Used to check for _UseLatencyInterpret, which is always true.
		Peek_Human_Phys()->Network_Latency_State_Update(newPosition, newVelocity);
	else
		Peek_Human_Phys()->Network_State_Update(newPosition, newVelocity);
}



void SoldierGameObj::Interpret_Sc_State_Data(
	HumanStateClass::HumanStateType state, int sub_state, 
	LPCSTR trans_name, const Vector3 & velocity, const Vector3 & sc_position)
{
	if ( ( Get_State() != state ) || (	Get_Sub_State() != sub_state ) ){
		if ( ( Get_State() == HumanStateClass::TRANSITION ) &&
			  ( state == HumanStateClass::TRANSITION_COMPLETE ) ) {
		} else if ( ( Get_State() == HumanStateClass::UPRIGHT ) &&
						( state == HumanStateClass::LAND ) ) {	
		} else {
			if (Is_Controlled_By_Me() &&
				 ((( Get_State() == HumanStateClass::UPRIGHT ) || ( Get_State() == HumanStateClass::AIRBORNE ) || ( Get_State() == HumanStateClass::LAND )) &&
				  (( state       == HumanStateClass::UPRIGHT ) || ( state       == HumanStateClass::AIRBORNE ) || ( state       == HumanStateClass::LAND )) )) {
			} else {
				if (state == HumanStateClass::TRANSITION) {
					Start_Transition_Animation( trans_name, NULL );
					AnimationName = trans_name;
				} else if (state == HumanStateClass::ANIMATION) {
					AnimationName = trans_name;
					HumanState.Start_Scripted_Animation( AnimationName, true, false );
				} else if (state == HumanStateClass::IN_VEHICLE) {
					HumanState.Force_Animation( trans_name, false );
					AnimationName = trans_name;
				}
				if (HumanState.Get_State() != HumanStateClass::IN_VEHICLE)
				{
					HumanState.Set_State( state, sub_state );
				}
			}
		}
	}
		
	if (( state == HumanStateClass::AIRBORNE ) && ( this != CombatManager::Get_The_Star() ) ) {
		Peek_Human_Phys()->Set_In_Contact( false );
		Peek_Human_Phys()->Set_Velocity( velocity );
		Peek_Human_Phys()->Set_Position( sc_position );
	}
}


bool SoldierGameObj::Is_In_Elevator( void )
{
	HumanPhysClass	* p_human_phys = Peek_Human_Phys();

	return		
		p_human_phys != NULL && 
		p_human_phys->Peek_Carrier_Object() != NULL && 
		p_human_phys->Peek_Carrier_Object()->As_ElevatorPhysClass() != NULL;
}



void SoldierGameObj::Exit_Vehicle()
{
	Vehicle = NULL;
	HumanState.Set_State(HumanStateClass::UPRIGHT, 0);
}



bool SoldierGameObj::Is_Safe_To_Disable_Ghost_Collision() const
{
	const PhysClass *p = Peek_Physical_Object();
	const AABoxClass& collisionBox = ((PhysClass *)p)->Peek_Model()->Get_Bounding_Box();
	
	MultiListClass<PhysClass> collidees;
	PhysicsSceneClass::Get_Instance()->Collect_Objects(collisionBox, false, true, &collidees);
	
	for (MultiListIterator<PhysClass> collideeIterator(&collidees); collideeIterator; ++collideeIterator)
	{
		PhysClass* collideePhys = collideeIterator.Get_Obj();
		if (collideePhys->Get_Observer())
		{
			PhysicalGameObj* physicalCollidee = ((CombatPhysObserverClass *)(collideePhys->Get_Observer()))->As_PhysicalGameObj();
			if (physicalCollidee &&
				physicalCollidee != this)
			{
				SoldierGameObj* soldierCollidee = physicalCollidee->As_SoldierGameObj();
				if (!soldierCollidee ||
					physicalCollidee->As_SoldierGameObj()->HumanState.Get_State() != HumanStateClass::DESTROY)
				{
					if (CollisionMath::Overlap_Test(collisionBox, collideePhys->Peek_Model()->Get_Bounding_Box()) != CollisionMath::OUTSIDE)
						return false;
				}
			}
		}
	}
	
	return true;
}

void SoldierGameObj::Copy_Settings(const SoldierGameObjDef& oDefinition)
{
	HumanState.Init(Peek_Human_Phys());
	HumanState.Set_Anim_Control((HumanAnimControlClass *)Get_Anim_Control());
	if (Get_Definition().HumanAnimOverrideDefID)
	{
		HumanState.Set_Human_Anim_Override(Get_Definition().HumanAnimOverrideDefID);
	}
	else
	{
		HumanState.Set_Human_Anim_Override(0);
	}
	if (Get_Definition().HumanLoiterCollectionDefID)
		HumanState.Set_Human_Loiter_Collection(Get_Definition().HumanLoiterCollectionDefID);
	else
		HumanState.Set_Human_Loiter_Collection(0);
	
	Adjust_Skeleton(oDefinition.SkeletonHeight, oDefinition.SkeletonWidth);
	RenderObjClass *robj = Peek_Human_Phys()->Peek_Model();
	if (robj)
	{
		robj->Set_Sub_Objects_Match_LOD(true);
	}
	if (!InnateObserver)
	{
		if (Get_Definition().UseInnateBehavior)
		{
			if (!Is_Controlled_By_Me())
			{
				InnateObserver = new SoldierObserverClass();
				Insert_Observer(InnateObserver);
			}
		}
	}
	for (unsigned int i = 0;i < 20;i++)
	{
		const DialogueClass *d = &DialogList[i];
		d = &oDefinition.DialogList[i];
	}
	Peek_Physical_Object()->Flags = Peek_Physical_Object()->Flags & 0xFFFFFFF0 | 6;
	Prepare_Speech_Framework();
}

bool SoldierGameObj::Is_Permitted_To_Enter_Vehicle(void)
{
	if (!CombatManager::Is_Gameplay_Permitted()) {
		return false;
	}
	return true;
}
static float	_shake_delay = 0;
static float	_cry_delay = 0;
const float EMOT_ICON_HEIGHT = 2.0F;
#define CORPSE_PERSIST_TIME 2.0f
typedef enum
{
	SURFACE_TYPE_LIGHT_METAL = 0,
	SURFACE_TYPE_HEAVY_METAL,
	SURFACE_TYPE_WATER,
	SURFACE_TYPE_SAND,
	SURFACE_TYPE_DIRT,
	SURFACE_TYPE_MUD,
	SURFACE_TYPE_GRASS,
	SURFACE_TYPE_WOOD,
	SURFACE_TYPE_CONCRETE,
	SURFACE_TYPE_FLESH,
	SURFACE_TYPE_ROCK,
	SURFACE_TYPE_SNOW,
	SURFACE_TYPE_ICE,
	SURFACE_TYPE_DEFAULT,
	SURFACE_TYPE_GLASS,
	SURFACE_TYPE_CLOTH,
	SURFACE_TYPE_TIBERIUM_FIELD,
	SURFACE_TYPE_FOLIAGE_PERMEABLE,
	SURFACE_TYPE_GLASS_PERMEABLE,
	SURFACE_TYPE_ICE_PERMEABLE,
	SURFACE_TYPE_CLOTH_PERMEABLE,
	SURFACE_TYPE_ELECTRICAL,
	SURFACE_TYPE_FLAMMABLE,
	SURFACE_TYPE_STEAM,
	SURFACE_TYPE_ELECTRICAL_PERMEABLE,
	SURFACE_TYPE_FLAMMABLE_PERMEABLE,
	SURFACE_TYPE_STEAM_PERMEABLE,
	SURFACE_TYPE_WATER_PERMEABLE,
	SURFACE_TYPE_TIBERIUM_WATER,
	SURFACE_TYPE_TIBERIUM_WATER_PERMEABLE,
	SURFACE_TYPE_UNDERWATER_DIRT,
	SURFACE_TYPE_UNDERWATER_TIBERIUM_DIRT,

	SURFACE_TYPE_MAX
} W3D_SURFACE_TYPES;



void SoldierGameObj::Think()
{
	if (this == CombatManager::Get_The_Star())
	{
		_shake_delay -= TimeManager::FrameSeconds;
		_cry_delay -= TimeManager::FrameSeconds;
	}

	if (Peek_Physical_Object()->Get_Collision_Group() == SOLDIER_GHOST_COLLISION_GROUP && Is_Safe_To_Disable_Ghost_Collision())
			Peek_Physical_Object()->Set_Collision_Group(SOLDIER_COLLISION_GROUP);

	PlayerDataClass* playerData = Get_Player_Data();
	if (playerData)
	{
		playerData->Stats_Add_Game_Time(TimeManager::FrameSeconds);
		playerData->Stats_Set_Final_Health(Defense.Get_Health());
		if (Get_Vehicle())
			playerData->Stats_Add_Vehicle_Time(TimeManager::FrameSeconds);
	}

	Update_Locked_Facing();
	Handle_Legs();
	SmartGameObj::Think();

	if (CombatManager::I_Am_Server())
	{
		if (Get_Weapon() &&
			Get_Weapon()->Get_Style() == WEAPON_HOLD_STYLE_C4 &&
			Get_Weapon()->Is_Firing() &&
			Get_State() == HumanStateClass::UPRIGHT)
		{
			AnimationName = "s_a_human.h_a_j12c";
			HumanState.Start_Scripted_Animation(AnimationName, true, false);
		}
	}

	if (Get_State() != HumanStateClass::IN_VEHICLE)
	{
		HumanState.Update_Weapon(Get_Weapon(), WeaponBag->Is_Changed());

		if (WeaponBag->Is_Changed())
		{
			WeaponChanged = true;
			WeaponBag->Reset_Changed();
		}

		HumanState.Update_State(); 

		if (Get_Weapon() && WeaponRenderModel)
		{
			if (Get_Weapon()->Get_Style() == WEAPON_HOLD_STYLE_C4 ||
				Get_Weapon()->Get_Style() == WEAPON_HOLD_STYLE_BEACON)
				WeaponRenderModel->Set_Hidden(!Get_Weapon()->Is_Loaded());
			else
				WeaponRenderModel->Set_Hidden(false);
		}

	}

	if (SpecialDamageMode != ArmorWarheadManager::SPECIAL_DAMAGE_TYPE_NONE &&
		SpecialDamageTimer > 0)
	{
		float previous_timer = SpecialDamageTimer;

		SpecialDamageTimer -= TimeManager::FrameSeconds;

		if ( (int)SpecialDamageTimer != (int)previous_timer ) {
			StringClass explosion_name = ArmorWarheadManager::Get_Special_Damage_Explosion( SpecialDamageMode );
			if ( !explosion_name.Is_Empty() ) {
				ExplosionDefinitionClass * def = (ExplosionDefinitionClass *)DefinitionMgrClass::Find_Typed_Definition( explosion_name, 0xB003, true );
				if ( def != NULL) {
					Vector3 pos = Get_Bullseye_Position();
					ExplosionManager::Create_Explosion_At( def->Get_ID(), pos, NULL, Vector3(0,0,1), NULL );
				}
			}
		}

		if ( Defense.Get_Health() > 0 && 
				(Allow_Special_Damage_State_Lock () || Is_Human_Controlled() ) ) {
			unsigned int warhead = ArmorWarheadManager::Get_Special_Damage_Warhead( SpecialDamageMode );
			float damage = ArmorWarheadManager::Get_Special_Damage_Scale( SpecialDamageMode ) * 
									TimeManager::FrameSeconds;
			OffenseObjectClass off( damage, warhead, (ArmedGameObj*)SpecialDamageDamager.Get_Ptr()	);
			Apply_Damage_Extended( off, 1 );
		}

		if ( HumanState.Get_State() != HumanStateClass::DEATH ) {

			if ( Defense.Get_Health() <= 0 ) {
				HumanState.Set_State( HumanStateClass::DEATH, 
					SpecialDamageMode - ArmorWarheadManager::SPECIAL_DAMAGE_TYPE_FIRE + HumanStateClass::OUCH_FIRE );
			}

			if ( SpecialDamageTimer <= 0 ) {
				Set_Special_Damage_Mode( ArmorWarheadManager::SPECIAL_DAMAGE_TYPE_NONE );
			}
		}
	}

	if (Get_State() == HumanStateClass::DESTROY) {
		if ( HumanState.Get_State_Timer() > CORPSE_PERSIST_TIME ) {
			if ( Is_Human_Controlled() ) {	
			}

			if (cNetwork::I_Am_Server())
				Set_Delete_Pending();
		}
	}

	if ( !CombatManager::I_Am_Only_Client() || Is_Controlled_By_Me()) {
		if ( Get_State() == HumanStateClass::TRANSITION_COMPLETE ) {
			if ( TransitionCompletionData != NULL ) {
				TransitionInstanceClass::End( this, TransitionCompletionData );
				TransitionCompletionData = NULL;
			} else {
				HumanState.Set_State( HumanStateClass::UPRIGHT );
			}
		}
	}

		EmotIconTimer -= TimeManager::FrameSeconds;
		if ( EmotIconTimer <= 0 ) {

			if ( EmotIconModel != NULL && EmotIconModel->Is_In_Scene() ) {
				EmotIconModel->Remove();
				REF_PTR_RELEASE( EmotIconModel );
			}

		} else if ( EmotIconModel != NULL  ) {

			Matrix3D tm = Get_Transform();
			tm.Set_Translation (tm.Get_Translation() + Vector3 (0, 0, EMOT_ICON_HEIGHT));
			EmotIconModel->Set_Transform( tm );
		}

	GenerateIdleFacialAnimTimer -= TimeManager::FrameSeconds;
	if ( GenerateIdleFacialAnimTimer <= 0 ) {
		
		if ( HeadModel != NULL ) {
			if ( ( HeadModel->Peek_Animation() == NULL ) || 
				  (HeadModel->Peek_Animation() == SpeechAnim ) ) {
				SpeechAnim->Generate_Idle_Animation( 20, 0.5F );
				HeadModel->Set_Animation( SpeechAnim, 0, RenderObjClass::ANIM_MODE_LOOP );
			}
		}

		GenerateIdleFacialAnimTimer = 100.0F;
	}


		if ( Peek_Human_Phys() ) {
			SurfaceEffectsManager::Apply_Damage( Peek_Human_Phys()->Get_Contact_Surface_Type(), this );
		}

		bool in_water = false;
		Vector3 p0 = Get_Transform().Get_Translation();
		Vector3 p1 = Get_Bullseye_Position();

		int hit_type = 2;
		if ( Is_Crouched() ) {
			hit_type = 4;
		} else if ( HumanState.Get_Sub_State() & HumanStateClass::SUB_STATE_SLOW ) {
			hit_type = 3;
		}

		Vector3 vel;
		Peek_Human_Phys()->Get_Velocity(&vel);

		if ((Peek_Human_Phys()->Get_Contact_Surface_Type() == SURFACE_TYPE_UNDERWATER_DIRT) &&
			 (vel.Length2() > 0.1f))
		{
			
			CastResultStruct res;
			LineSegClass ray(p0,p1);
			PhysRayCollisionTestClass raytest(ray,&res,BULLET_COLLISION_GROUP,4);
			
			Peek_Human_Phys()->Inc_Ignore_Counter();
			CombatManager::Get_Scene()->Cast_Ray(raytest);
			Peek_Human_Phys()->Dec_Ignore_Counter();

			if ((res.Fraction < 1.0f) && (res.SurfaceType == SURFACE_TYPE_WATER_PERMEABLE)) {

				Vector3 point;
				ray.Compute_Point(res.Fraction, &point);

				SurfaceEffectsManager::Update_Persistant_Emitter(	WaterWake,
																					res.SurfaceType, 
																					hit_type, 
																					Matrix3D(point));
				in_water = true;
			}
		}
		
		if (!in_water)
				SurfaceEffectsManager::Update_Persistant_Emitter(	WaterWake,
																					SURFACE_TYPE_DEFAULT, 
																					hit_type, 
																					Matrix3D(p0));


	if (Get_Player_Data() && Get_Player_Data()->PunishTimer > 0)
		Get_Player_Data()->PunishTimer += TimeManager::FrameSeconds;
}

void Do_Death_Sound(GameObject *obj,int soundid,Matrix3D const &pos,RefCountClass *ref,unsigned long unk1,int unk2);
void	SoldierGameObj::Apply_Damage_Extended( const OffenseObjectClass & damager, float scale,
			const	Vector3 & direction, const char * collision_box_name )
{
	if ( Defense.Is_Repair( damager ) ) {
		SmartGameObj::Apply_Damage_Extended( damager, scale, direction, collision_box_name );

		if ( HealingEffect == NULL ) {
			HealingEffect = CombatMaterialEffectManager::Get_Health_Effect();
			if ( HealingEffect != NULL ) {
				Peek_Human_Phys()->Add_Effect_To_Me( HealingEffect );
			}
		}

		if ( HealingEffect != NULL ) {
			HealingEffect->Set_Target_Parameter(0.495f);
		}

		return;
	}

	if (Is_In_Vehicle()) {
		return;
	}

	bool ambushed = false;
	if ( ( AIState == AI_STATE_IDLE ) || ( AIState == AI_STATE_SECONDARY_IDLE ) ) {
		if ( damager.Get_Owner() != NULL && !Is_Human_Controlled() ) {
			scale *= 1;
			ambushed = true;
		}
	}

	bool anim_ok = true;

	Reset_Hibernating();

	int ouch_type = HumanState.Get_Ouch_Type( direction, collision_box_name );

	PlayerDataClass * damager_data = NULL;
	if ( damager.Get_Owner() && damager.Get_Owner()->As_SoldierGameObj() ) {
		damager_data = damager.Get_Owner()->As_SoldierGameObj()->playerData;
	}

	if (direction.Length2() > WWMATH_EPSILON) {
		switch( ouch_type ) {
			case HumanStateClass::HEAD_FROM_BEHIND:
			case HumanStateClass::HEAD_FROM_FRONT:
				if ( playerData != NULL ) {
					playerData->HeadHit++;
				}
				if ( damager_data != NULL ) {
					damager_data->HeadShots++;
				}
				break;

			case HumanStateClass::TORSO_FROM_BEHIND:
			case HumanStateClass::TORSO_FROM_FRONT:
				if ( playerData != NULL ) {
					playerData->TorsoHit++;
				}
				if ( damager_data != NULL ) {
					damager_data->TorsoShots++;
				}
				break;

			case HumanStateClass::LEFT_ARM_FROM_BEHIND:
			case HumanStateClass::LEFT_ARM_FROM_FRONT:
			case HumanStateClass::RIGHT_ARM_FROM_BEHIND:
			case HumanStateClass::RIGHT_ARM_FROM_FRONT:
				if ( playerData != NULL ) {
					playerData->ArmHit++;
				}
				if ( damager_data != NULL ) {
					damager_data->ArmShots++;
				}
				break;

			case HumanStateClass::LEFT_LEG_FROM_BEHIND:
			case HumanStateClass::LEFT_LEG_FROM_FRONT:
			case HumanStateClass::RIGHT_LEG_FROM_BEHIND:
			case HumanStateClass::RIGHT_LEG_FROM_FRONT:
				if ( playerData != NULL ) {
					playerData->LegHit++;
				}
				if ( damager_data != NULL ) {
					damager_data->LegShots++;
				}
				break;

			case HumanStateClass::GROIN:
				if ( playerData != NULL ) {
					playerData->CrotchHit++;
				}
				if ( damager_data != NULL ) {
					damager_data->CrotchShots++;
				}
				break;
		}
	}

	int dialogue_id = -1;
	ArmedGameObj *damage_owner = damager.Get_Owner();
	if( damage_owner != NULL ) {
		
		if ( Is_Enemy( damage_owner ) ) {
			dialogue_id = 0;
		} else {
			dialogue_id = 1;
		}		
	}

	float health_before = Defense.Get_Health();
	float armor_before = Defense.Get_Shield_Strength();

	if ( collision_box_name != NULL ) {
		const char * start = ::strchr( collision_box_name, '.' );
		if ( start != NULL ) {
			start++;
			float bone_scale = BonesManager::Get_Bone_Damage_Scale( start );
			scale *= bone_scale;
		}
	}

	if ( this == CombatManager::Get_The_Star() ) {
		if ( Defense.Would_Damage( damager ) && !damager.ForceServerDamage ) {
			if ( direction.Length() == 0 ) {
				for ( int i = 0; i < 8; i++ ) {
					CombatManager::Show_Star_Damage_Direction( i );
				}
			} else {
				Vector3 relative_direction = Get_Transform().Inverse_Rotate_Vector( direction );
				float angle = ::WWMath::Atan2 ( relative_direction.Y, -relative_direction.X);
				int dir = (int)(8.0f * angle / DEG_TO_RAD( 360 ) + 8.5f);
				CombatManager::Show_Star_Damage_Direction( dir & 7 );
			}
		}
	}


	SmartGameObj::Apply_Damage_Extended( damager, scale, direction, collision_box_name );
	float health = Defense.Get_Health();

	if (health == health_before && Defense.Get_Shield_Strength() == armor_before) {

		if ( health > 0 || 
				HumanState.Get_State() == HumanStateClass::DEATH || 
				HumanState.Get_State() == HumanStateClass::DESTROY ) {
			return;
		} else {
		}
	}


	int warhead = damager.Get_Warhead();
	ArmorWarheadManager::SpecialDamageType special_damage = ArmorWarheadManager::Get_Special_Damage_Type( warhead );
	if ( special_damage != 0 ) {
		float probability = ArmorWarheadManager::Get_Special_Damage_Probability( warhead );
		if (health <= 0) {
			probability = 1;
		}
		if ( FreeRandom.Get_Float() < probability ) {
			int skin = Get_Defense_Object()->Get_Skin();
			ArmorWarheadManager::SpecialDamageType special_damage2 = ArmorWarheadManager::Get_Special_Damage_Type( warhead );
			if ( !ArmorWarheadManager::Is_Skin_Impervious( special_damage2, skin ) ) {
				Set_Special_Damage_Mode( special_damage2, damager.Get_Owner() );
			}
		}
	}

	if ( this == CombatManager::Get_The_Star() ) {
		if ( 	_shake_delay < 0 ) {
			_shake_delay = 0.6f;

			Vector3	pos = COMBAT_CAMERA->Get_Transform().Get_Translation();
			CombatManager::Get_Scene()->Add_Camera_Shake(	pos,	1, 0.5f, 0.05f );
		}

		anim_ok = false;
		if ( 	_cry_delay < 0 ) {
			_cry_delay = 2;
			anim_ok = true;

			AudibleSoundClass * sound = WWAudioClass::Get_Instance()->Create_Sound( "Take_Damage_Sound",0,0,2);
			if ( sound ) {
				sound->Play(true);
				sound->Release_Ref();
			}
		}

	}


	if ( health <= 0 ) {

		if ( cGameType::GameType == 1 ) {
			int warhead2 = damager.Get_Warhead();
			float visceroid_probability = ArmorWarheadManager::Get_Visceroid_Probability( warhead2 );
			if ( !Is_Human_Controlled() && visceroid_probability != 0 ) {
				if ( FreeRandom.Get_Float() < visceroid_probability ) {

					#define	VISCEROID_NAME			"Visceroid"
					if ( _stricmp( Get_Definition().Get_Name(), VISCEROID_NAME ) != 0 ) {
						PhysicalGameObj * vis = (PhysicalGameObj *)ObjectLibraryManager::Create_Object( VISCEROID_NAME );
						if ( vis != NULL ) {
							vis->Set_Transform( Get_Transform() );
							vis->Start_Observers ();
							Set_Delete_Pending();
						}
					}
				}
			}
		}

		Enable_Hibernation( false );

		SoldierGameObj * damager_owner = NULL;
		if ( damager.Get_Owner() && damager.Get_Owner()->As_SoldierGameObj() ) {
			damager_owner = damager.Get_Owner()->As_SoldierGameObj();
		}
		if ( damager_owner && damager_owner->Get_Player_Data() ) {

			if ( Is_Teammate( damager_owner ) ) {
				damager_owner->Get_Player_Data()->AlliesKilled++;
			}

			if ( Is_Enemy( damager_owner ) ) {
				damager_owner->Get_Player_Data()->EnemiesKilled++;
			}

			if ( damager_owner->Get_Vehicle() ) {
				damager_owner->Get_Player_Data()->KillsFromVehicle++;
			}
		}

		if ( HumanState.Get_State() != HumanStateClass::DEATH ) {

			if ( this == CombatManager::Get_The_Star() ) {
				CombatManager::Register_Star_Killer( damager.Get_Owner() );
			}

			HumanState.Set_State( HumanStateClass::DEATH, ouch_type );

			Stop_Current_Speech ();

			int death_sound_id = Get_Definition ().DeathSoundPresetID;
			if (death_sound_id == 0) {

				GlobalSettingsDef *global_settings = GlobalSettingsDef::GlobalSettings;
				if (global_settings != NULL) {
					death_sound_id = global_settings->DeathSoundID;
				}				
			}

			if ( damager.Get_Owner () == CombatManager::Get_The_Star() ) {
				EncyclopediaMgrClass::Reveal_Object( this );
			}

			if (death_sound_id != 0) {
				Matrix3D tm = Get_Transform();
				RefCountedReferencerClass *owner_ref = new RefCountedReferencerClass(this);
				WWAudioClass::Get_Instance()->Create_Instant_Sound( death_sound_id, tm, owner_ref,0,2);
				Do_Death_Sound(this,death_sound_id, tm, owner_ref, 0, 2);
				REF_PTR_RELEASE(owner_ref);
			}


			if ( Get_Definition().KilledExplosion != 0 ) {
				Vector3 pos;
				Get_Position(&pos);
				ExplosionManager::Create_Explosion_At( Get_Definition().KilledExplosion, pos, damager.Get_Owner(), Vector3(0,0,1), NULL );
			}

			if (	damage_owner != NULL &&
					damage_owner != this &&
					damage_owner->As_SoldierGameObj () != NULL )
			{
				SoldierGameObj *our_killer = damage_owner->As_SoldierGameObj  ();
				
				if ( Is_Enemy( damage_owner ) ) {			
					our_killer->Say_Dialogue( 5 );
				} else {
					our_killer->Say_Dialogue( 4 );
				}		
			}

		}

	} else {
		if ( Get_State() == HumanStateClass::UPRIGHT && anim_ok ) {
			HumanState.Set_State( HumanStateClass::WOUNDED, ouch_type );
		}
	}

	if( CurrentSpeech == NULL && dialogue_id >= 0 ) {
		Say_Dialogue( dialogue_id );
	}
}

void	SoldierGameObj::Stop_Current_Speech( void )
{
	if ( CurrentSpeech != NULL ) {
		GenerateIdleFacialAnimTimer = 0;
		CurrentSpeech->Stop(true);
		CurrentSpeech->Remove_From_Scene();
		CurrentSpeech->Release_Ref();
		CurrentSpeech = NULL;
	}
}

RENEGADE_FUNCTION
void SoldierGameObj::Say_Dialogue( int dialog_id )
AT2(0x006CCDF0,0x006CC690);

void	SoldierGameObj::Update_Locked_Facing( void )
{
	if ( FacingObject != NULL ) {
		Vector3 pos;
		FacingObject.Get_Ptr ()->Get_Position( &pos );
		if ( FacingObject.Get_Ptr ()->As_PhysicalGameObj ()->As_SoldierGameObj () != NULL ) {
			const float SOLDIER_HEIGHT = 1.7F;
			pos.Z += SOLDIER_HEIGHT;
		}
		Look_At( pos, 100.0F );
		if ( FacingAllowBodyTurn ) {
			Internal_Set_Targeting( pos, false );
		}
	}
}

bool	SoldierGameObj::Internal_Set_Targeting( const Vector3 & target_pos, bool do_tilt )
{
	SmartGameObj::Set_Targeting( target_pos,true);
	if ( ( Get_State() == HumanStateClass::DEATH ) ||
		  ( Get_State() == HumanStateClass::DESTROY ) ||
		  ( Get_State() == HumanStateClass::TRANSITION ) ||
		  ( Get_State() == HumanStateClass::LADDER ) ) {
		return false;
	}
	if ( Get_State() == HumanStateClass::IN_VEHICLE )
	{
		if ( Vehicle != NULL )
		{
			if ( Vehicle->Get_Driver_Is_Gunner() )
			{
				if ( Vehicle->Get_Driver() == this )
				{
					Vehicle->Set_Targeting( target_pos );
				}
			}
			else
			{
				if ( Vehicle->Get_Driver() == this && Vehicle->Get_Gunner() == NULL )
				{
					Vehicle->Set_Targeting( target_pos );
				}
				else if ( Vehicle->Get_Gunner() == this )
				{
					Vehicle->Set_Targeting( target_pos );
				}
			}
		}
		return false;
	}
	Vector3 muzzle_pos;
	Get_Position( &muzzle_pos );
	muzzle_pos.Z += Get_Weapon_Height();
	Vector3	rel_target_pos = target_pos - muzzle_pos;
	float dist = rel_target_pos.Length();
	float	tilt = 0;
	if ( dist && do_tilt )
	{
		tilt = WWMath::Fast_Asin( rel_target_pos.Z / dist );
	}
	bool	is_complete = true;
	float cur_facing = Peek_Human_Phys()->Get_Heading();
	float facing = WWMath::Atan2( rel_target_pos.Y, rel_target_pos.X );
	float facing_dif = facing - cur_facing;
	if ( WWMath::Fabs(facing_dif) > 0.001f )
	{
		facing_dif = WWMath::Wrap( facing_dif, DEG_TO_RADF( -180.0f ), DEG_TO_RADF( 180.0f ) );
		float change = facing_dif;
		if ( !Is_Human_Controlled() )
		{
			float max_change = Get_Turn_Rate() * TimeManager::FrameSeconds;
			if ( WWMath::Fabs( change ) < DEG_TO_RAD( 20 ) )
			{
				max_change *= 0.3f;
			}
			change = WWMath::Clamp( facing_dif, -max_change, max_change );
		}
		if ( !Is_Human_Controlled() )
		{
  			HumanState.Set_Turn_Velocity( change );
		}
		is_complete = (change == facing_dif);
		facing = cur_facing + change;
		facing = WWMath::Wrap( facing, DEG_TO_RADF( -180.0f ), DEG_TO_RADF( 180.0f ) );
		Peek_Human_Phys()->Set_Heading( facing );
	}
	if ( Is_Human_Controlled() && Get_State() != HumanStateClass::IN_VEHICLE )
	{
#define		TILT_DOWN_SPEED		4.0f
		float direction = -1;
		if ( Get_Weapon() && Get_Weapon()->Is_Reloading() )
		{
			direction = 1;
		}
		ReloadingTilt += direction * TimeManager::FrameSeconds * TILT_DOWN_SPEED;
		ReloadingTilt = WWMath::Clamp( ReloadingTilt, 0, 1 );
		if ( ReloadingTilt > 0 )
		{
			tilt = WWMath::Lerp(	(float)tilt, DEG_TO_RADF( -90 ), ReloadingTilt );
		}
	}
	HumanState.Update_Aiming( tilt, 0 );
	return is_complete;
}



void SoldierGameObj::Enable_Ghost_Collision(bool value)
{
	TT_UNREACHABLE;
}



RENEGADE_FUNCTION
void	SoldierGameObj::Handle_Legs( void )
AT2(0x006CA1A0,0x006C9A40);

void	SoldierGameObj::Start_Transition_Animation( const char * anim_name, TransitionCompletionDataStruct *data )
{
	TransitionCompletionData = data;
	AnimationName = anim_name;
	HumanState.Start_Transition_Animation( anim_name, false );
}

void Poke_Do(GameObject *poked,GameObject *poker);
void SoldierGameObj::Apply_Control( void )
{
	if ( !CombatManager::Is_Gameplay_Permitted() ) {
		Clear_Control();
		physController.Reset();
		return;
	}
	if ( Get_State() == HumanStateClass::IN_VEHICLE ) {
		if ( Get_Vehicle() && Get_Vehicle()->Get_Driver() == this ) {
			if ( control.Get_Boolean( ControlClass::BooleanToggleDriverIsGunner ) ) {
				Get_Vehicle()->Toggle_Driver_Is_Gunner();
			}
		}		

		if (	CombatManager::I_Am_Server() )
		{
			if ( control.Get_Boolean( ControlClass::BooleanUse ) ) {
				TransitionManager::Check( this, true );
			}
		}
		return;
	}

	if ( ( Get_State() == HumanStateClass::TRANSITION ) || 
		  ( Get_State() == HumanStateClass::ANIMATION ) ||
		  ( Get_State() == HumanStateClass::DESTROY ) ||
		  ( Get_State() == HumanStateClass::DEATH ) ||
		  ( Get_State() == HumanStateClass::ON_FIRE ) ||
		  ( Get_State() == HumanStateClass::ON_CHEM ) ||
		  ( Get_State() == HumanStateClass::ON_CNC_FIRE ) ||
		  ( Get_State() == HumanStateClass::ON_CNC_CHEM ) ||
		  ( Get_State() == HumanStateClass::ON_ELECTRIC ) ) {

		if ( Get_State() == HumanStateClass::DESTROY && Is_Human_Controlled() ) {
			if ( control.Get_Boolean( ControlClass::BooleanPrimaryFire ) ) {
				HumanState.Set_State_Timer( 1000 );
			}
		}

		Clear_Control();
	}

	if ( Get_State() == HumanStateClass::DIVE ) {

		Clear_Control();

		SmartGameObj::Apply_Control();

		Vector3	forced_move(0,0,0);
		int sub_state = HumanState.Get_Sub_State();
		if ( sub_state & HumanStateClass::SUB_STATE_FORWARD )	forced_move = Vector3( 0.5f,0,0 );
		if ( sub_state & HumanStateClass::SUB_STATE_BACKWARD )	forced_move = Vector3( -0.5f,0,0 );
		if ( sub_state & HumanStateClass::SUB_STATE_LEFT )		forced_move = Vector3( 0,0.5f,0 );
		if ( sub_state & HumanStateClass::SUB_STATE_RIGHT )		forced_move = Vector3( 0,-0.5f,0 );

		physController.Set_Move_Forward(	forced_move.X );
		physController.Set_Move_Left(		forced_move.Y );
		physController.Set_Move_Up(			forced_move.Z );
		return;
	}

#define	CLIMB_SCALE			0.3f

	if ( Get_State() == HumanStateClass::LADDER ) {

		float up_down = CLIMB_SCALE * control.analog[0];

		if ( control.analog[0] == 0 ) {
			LadderUpMask = false;
			LadderDownMask = false;
		}

		if ( LadderUpMask ) {
			up_down = min( up_down, 0 );
		}
		if ( LadderDownMask ) {
			up_down = max( up_down, 0 );
		}

		control.analog[2] = up_down;


		control.analog[1] = 0;
		control.analog[3] = 0;
		control.Set_Boolean( ControlClass::BooleanJump, 0 );
	} else {
		LadderUpMask = false;
		LadderDownMask = false;


		if ( !InFlyMode ) {
			control.analog[2] = 0;
		}
	}

	if ( HumanState.Is_Sub_State_Adjustable() ) {

		bool new_state = control.Get_Boolean( ControlClass::BooleanCrouch );
		bool old_state = HumanState.Get_State_Flag( HumanStateClass::CROUCHED_FLAG );

		if ( new_state != old_state ) {
	
			HumanState.Toggle_State_Flag( HumanStateClass::CROUCHED_FLAG );
		}
	}

	DetonateC4 = false;
	if ( Get_Weapon() && control.Get_Boolean( ControlClass::BooleanDetonateC4 ) ) {
		if ( Get_Weapon()->Get_Style() == WEAPON_HOLD_STYLE_C4 ) {
			DetonateC4 = true;
		}

		if ( Get_Weapon()->Get_Can_Snipe() ) {
			if ( HumanState.Is_Sub_State_Adjustable() ) {
				HumanState.Toggle_State_Flag( HumanStateClass::SNIPING_FLAG );
			}
		}
	}


	if ( Is_Sniping() || Is_On_Ladder() ) {
		control.Set_Boolean( ControlClass::Boolean5, 0 );
		control.Set_Boolean( ControlClass::Boolean6, 0 );
		control.Set_Boolean( ControlClass::Boolean7, 0 );
		control.Set_Boolean( ControlClass::Boolean8, 0 );
	}

	bool dove = false;

	if ( Get_State() == HumanStateClass::UPRIGHT ) {
		if ( control.Get_Boolean( ControlClass::Boolean5 ) ) {
			HumanState.Set_State( HumanStateClass::DIVE, HumanStateClass::SUB_STATE_FORWARD );
			dove = true;
		}

		if ( control.Get_Boolean( ControlClass::Boolean6 ) ) {
			HumanState.Set_State( HumanStateClass::DIVE, HumanStateClass::SUB_STATE_BACKWARD );
			dove = true;
		}

		if ( control.Get_Boolean( ControlClass::Boolean7 ) ) {
			HumanState.Set_State( HumanStateClass::DIVE, HumanStateClass::SUB_STATE_LEFT );
			dove = true;
		}

		if ( control.Get_Boolean( ControlClass::Boolean8 ) ) {
			HumanState.Set_State( HumanStateClass::DIVE, HumanStateClass::SUB_STATE_RIGHT );
			dove = true;
		}

	}

	float amount = control.analog[3];
	if ( CombatManager::Get_The_Star() == this ) {
		amount = 0;
	}
	if ( amount != 0 ) {
		float heading = Peek_Human_Phys()->Get_Heading();
		heading += amount * Get_Turn_Rate() * TimeManager::FrameSeconds;
		heading = WWMath::Wrap( heading, DEG_TO_RADF( -180.0f ), DEG_TO_RADF( 180.0f ) );
		Peek_Human_Phys()->Set_Heading(heading);
		control.analog[3] = 0.0f;
		if ( !Is_Human_Controlled() ) {
  			HumanState.Set_Turn_Velocity( amount );
		}
	}

	SmartGameObj::Apply_Control();

	if ( control.Get_Boolean( ControlClass::BooleanJump ) ) {
		if ( Get_State() != HumanStateClass::AIRBORNE ) {
			physController.Set_Move_Up( Get_Definition().JumpVelocity );
		}

	}
	if ( Is_Sniping() ) {
		physController.Set_Move_Forward( WWMath::Clamp( physController.Get_Move_Forward(), -0.25f, 0.25f ) );	
		physController.Set_Move_Left( WWMath::Clamp( physController.Get_Move_Left(), -0.25f, 0.25f ) );	
	}

	if ( Get_State() == HumanStateClass::LADDER ) {
		physController.Set_Move_Forward( 0 );	
	}

	if ( ( Get_State() == HumanStateClass::UPRIGHT ) && Is_Crouched() ) {
		float crouch_speed = GlobalSettingsDef::GlobalSettings->SoldierCrouchSpeed;
		physController.Set_Move_Forward( physController.Get_Move_Forward() * crouch_speed );	
		physController.Set_Move_Left( physController.Get_Move_Left() * crouch_speed );	
	} else if ( control.Get_Boolean( ControlClass::BooleanWalk ) ) {
		float walk_speed = GlobalSettingsDef::GlobalSettings->SoldierWalkSpeed;
		physController.Set_Move_Forward( physController.Get_Move_Forward() * walk_speed );	
		physController.Set_Move_Left( physController.Get_Move_Left() * walk_speed );	
	}

	if ( this == CombatManager::Get_The_Star() ) {
		static bool was_walking = false;
		if ( control.Get_Boolean( ControlClass::BooleanWalk )  != was_walking ) {
			was_walking = control.Get_Boolean( ControlClass::BooleanWalk );
		}

	}

	if (	CombatManager::I_Am_Server() ||
			(Is_Controlled_By_Me() && TransitionCompletionData == NULL) )
	{
		bool action_triggered = control.Get_Boolean( ControlClass::BooleanUse );

		bool transition_triggered = false;
		if ( CombatManager::I_Am_Server() ) {
			transition_triggered = TransitionManager::Check( this, action_triggered );
		}

		if ( !transition_triggered ) {
			if ( action_triggered ) {

				DamageableGameObj *damageable_target = HUDInfo::Get_Info_Object();
				if ( damageable_target != NULL ) {
					PhysicalGameObj *physical_target = damageable_target->As_PhysicalGameObj();
					if ( physical_target != NULL ) {
						Vector3 target_pos;
						physical_target->Get_Position( &target_pos );
						Vector3 my_pos;
						Get_Position( &my_pos );

						if ( (target_pos - my_pos).Length() <= 2 ) {
							if (CombatManager::I_Am_Server() || cGameType::GameType == 1) {
								const SimpleDynVecClass<GameObjObserverClass *> & observer_list = physical_target->Get_Observers();
								for( int index = 0; index < observer_list.Count(); index++ ) {
									observer_list[ index ]->Poked( physical_target, this );
								}
							}
							else
							{
								Poke_Do(physical_target,this);
							}

							if ( CombatManager::Get_The_Star() == this ) {

								EncyclopediaMgrClass::Reveal_Object( damageable_target );
								
								if ( physical_target->As_SimpleGameObj () != NULL ) {
									
									PlayerTerminalClass::TYPE type = PlayerTerminalClass::TYPE_NONE;
									type = (PlayerTerminalClass::TYPE)(physical_target->As_SimpleGameObj())->Get_Definition().Get_Player_Terminal_Type();
									if (type != PlayerTerminalClass::TYPE_NONE) {

										PlayerTerminalClass::Get_Instance()->Display_Terminal( this, type );
									}
								}
							}
						}
					}
				}
			}
		} else {
			physController.Reset();
		}
	}

	if ( IS_SOLOPLAY && Is_Human_Controlled() && Input::Get_State( INPUT_FUNCTION_DEBUG_RAPID_MOVE ) ) {
		if ( control.Get_Boolean( ControlClass::BooleanJump ) ) {
			physController.Set_Move_Up( Get_Definition().JumpVelocity );
		}
	}

}

HumanPhysClass	*SoldierGameObj::Peek_Human_Phys( void ) const 
{ 
	return ((SoldierGameObj *)this)->Peek_Physical_Object()->As_HumanPhysClass(); 
}

const SoldierGameObjDef & SoldierGameObj::Get_Definition( void ) const
{
	return (const SoldierGameObjDef &)*definition;
}

//------------------------------------------------------------------------------------
void	SoldierGameObj::Set_Blended_Animation( const char *animation_name, bool looping, float frame_offset, bool play_backwards )
{
	if (HumanState.Get_State() == HumanStateClass::DEATH || HumanState.Get_State() == HumanStateClass::DESTROY)
		return;
	
	if ( animation_name == NULL ) {
		HumanState.Stop_Scripted_Animation();
		return;
	}

	// Humans ignore the start_frame parameter for now

	AnimationName = animation_name;
	HumanState.Start_Scripted_Animation( animation_name, true, looping );
	Get_Anim_Control()->Set_Mode( looping ? ANIM_MODE_LOOP : ANIM_MODE_ONCE, frame_offset );

	if ( play_backwards ) {
		HAnimClass *anim = Get_Anim_Control()->Peek_Animation();
		if ( anim != NULL ) {
			int frame_count = anim->Get_Num_Frames();
			Get_Anim_Control()->Set_Mode( ANIM_MODE_TARGET, (float)frame_count - 1 );
			Get_Anim_Control()->Set_Target_Frame( 0 );
		}
	}

	return ;
}


//------------------------------------------------------------------------------------
void	SoldierGameObj::Set_Animation( const char *animation_name, bool looping, float start_frame )
{
	if (HumanState.Get_State() == HumanStateClass::DEATH || HumanState.Get_State() == HumanStateClass::DESTROY)
		return;
	
	if ( animation_name == NULL ) {
//		Debug_Say(( "Stoping Scripted Human Animation\n" ));
		HumanState.Stop_Scripted_Animation();
// FIX		AnimControl->UnLock_Animation();
		return;
	}

	// Humans ignore the start_frame parameter for now

//	Debug_Say(( "Starting Scripted Human Animation %s\n", animation_name ));
	AnimationName = animation_name;
	HumanState.Start_Scripted_Animation( animation_name, false, looping );
	Get_Anim_Control()->Set_Mode( looping ? ANIM_MODE_LOOP : ANIM_MODE_ONCE, start_frame );
}

void SoldierGameObj::Apply_Damage( const OffenseObjectClass & damager, float scale, int alternate_skin )
{
	if (!Is_In_Vehicle())
	{
		SmartGameObj::Apply_Damage( damager, scale, alternate_skin );
	}
}

float SoldierGameObj::Get_Weapon_Height( void )
{
	float height = 1.62f;
	if ( Is_Crouched() )
	{
		height -= 0.56f;
	}
	return height;
}
