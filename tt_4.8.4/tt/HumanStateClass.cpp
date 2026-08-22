#include "General.h"
#include "HumanStateClass.h"
#include "WW3DAssetManager.h"
#include "HumanAnimControlClass.h"
#include "HAnimClass.h"
#include "cGameType.h"
#include "HumanAnimOverrideDef.h"
#include "HumanPhysClass.h"
#include "TimeManager.h"
#include "DefinitionMgrClass.h"
#include "HumanLoiterGlobalSettingsDef.h"
#include "CombatMaterialEffectManager.h"
#include "SurfaceEffectsManager.h"
#include "GlobalSettingsDef.h"
#include "scripts.h"
#include "SoldierGameObj.h"
#include "EncoderList.h"
#include "bitpackids.h"
#include "OffenseObjectClass.h"
#include "WeaponClass.h"

const char* LegAnimNames[30] = { "A0", "A1", "A2", "A3", "A4", "A5", "A6",
								"B1", "B2", "B3", "B4",
								"C0", "C1", "C2", "C3", "C4", "C5", "C6",
								"J0", "J1", "J2", "J3", "J4",
								"Z0", "Z1", "Z2", "Z3", "Z4", "Z5", "Z6"};

const char* _weapon_style_names[10] = { "A0", "A0", "C2", "D2", "E2", "F2", "A0", "A0", "B0", "A0"};

const char* _dive_anims[8] = {"S_A_HUMAN.H_A_SLD1_01",
							  "S_A_HUMAN.H_A_SLD1_02",
							  "S_A_HUMAN.H_A_SLD2_01",
							  "S_A_HUMAN.H_A_SLD2_02",
							  "S_A_HUMAN.H_A_SLD3_01",
							  "S_A_HUMAN.H_A_SLD3_02",
							  "S_A_HUMAN.H_A_SLD4_01",
							  "S_A_HUMAN.H_A_SLD4_02"};

const char* _WoundAnims[13] = {"S_A_HUMAN.H_A_811A",
							   "S_A_HUMAN.H_A_812A",
							   "S_A_HUMAN.H_A_821A",
							   "S_A_HUMAN.H_A_822A",
							   "S_A_HUMAN.H_A_831A",
							   "S_A_HUMAN.H_A_832A",
							   "S_A_HUMAN.H_A_841A",
							   "S_A_HUMAN.H_A_842A",
							   "S_A_HUMAN.H_A_851A",
							   "S_A_HUMAN.H_A_852A",
							   "S_A_HUMAN.H_A_861A",
							   "S_A_HUMAN.H_A_862A",
							   "S_A_HUMAN.H_A_871A"};

const char* _DeathAnims[18] = {"S_A_HUMAN.H_A_622A",
							   "S_A_HUMAN.H_A_635A",
							   "S_A_HUMAN.H_A_622A",
							   "S_A_HUMAN.H_A_632A",
							   "S_A_HUMAN.H_A_623A",
							   "S_A_HUMAN.H_A_634A",
							   "S_A_HUMAN.H_A_624A",
							   "S_A_HUMAN.H_A_633A",
							   "S_A_HUMAN.H_A_623A",
							   "S_A_HUMAN.H_A_634A",
							   "S_A_HUMAN.H_A_624A",
							   "S_A_HUMAN.H_A_633A",
							   "S_A_HUMAN.H_A_612A",
							   "S_A_HUMAN.H_A_FLMB",
							   "S_A_HUMAN.H_A_FLMB",
							   "S_A_HUMAN.H_A_FLMB",
							   "S_A_HUMAN.H_A_FLMB",
							   "S_A_HUMAN.H_A_FLMB"};


void HumanStateClass::Set_Precision()
{
	cEncoderList::Set_Precision(BITPACK_HUMAN_STATE, 0, (int) HIGHEST_HUMAN_STATE);
	cEncoderList::Set_Precision(BITPACK_HUMAN_SUB_STATE, 0, (int) HIGHEST_HUMAN_SUB_STATE);
}

void	HumanStateClass::Force_Animation( const char * anim_name, bool blend )
{
	float blend_time = blend ? 0.2f : 0;
	AnimControl->Set_Animation( anim_name, blend_time );
	AnimControl->Update( 0 );
}

void HumanStateClass::Set_State(HumanStateType newState, int newSubState)
{
	if((State == DEATH || State == DESTROY) && newState != DESTROY)
	{
		return;
	}

	if(State != DEATH && newState == DEATH)
	{
		StateLocked = false;
	}

	if(StateLocked && cGameType::GameType == 3)
	{
		StateLocked = false;
	}

	if(newState == State && SubState == newSubState)
	{
		return;
	}

	State = newState;
	SubState = newSubState;
	StateTimer = 0;
	
	if (State != LADDER
		&& State != IN_VEHICLE 
		&& State != TRANSITION
		&& State != TRANSITION_COMPLETE
		&& State != DEBUG_FLY)
	{
		HumanPhys->Enable_User_Control(false);
	}
	else
	{
		HumanPhys->Enable_User_Control(true);
	}

	if (State == IN_VEHICLE)
	{
		HumanPhys->Set_Collision_Group(UNCOLLIDEABLE_GROUP);
		HumanPhys->Set_Immovable(true);
	}
	else if (State == TRANSITION || State == TRANSITION_COMPLETE)
	{
		HumanPhys->Set_Collision_Group(BULLET_ONLY_COLLISION_GROUP);
		HumanPhys->Set_Immovable(true);
	}
	else if(State == DESTROY || State == DEATH)
	{
		HumanPhys->Set_Collision_Group(TERRAIN_ONLY_COLLISION_GROUP);
		HumanPhys->Set_Immovable(true);
	}
	else if (State == LADDER)
	{
		HumanPhys->Set_Collision_Group(SOLDIER_GHOST_COLLISION_GROUP);
		HumanPhys->Set_Immovable(false);
	}
	else
	{
		TT_ASSERT(HumanPhys);
		TT_ASSERT(HumanPhys->Get_Observer());
		TT_ASSERT(((CombatPhysObserverClass*)HumanPhys->Get_Observer())->As_DamageableGameObj());
		TT_ASSERT(((CombatPhysObserverClass*)HumanPhys->Get_Observer())->As_DamageableGameObj()->As_SoldierGameObj());

		SoldierGameObj* soldier = ((CombatPhysObserverClass*)HumanPhys->Get_Observer())->As_DamageableGameObj()->As_SoldierGameObj();
		if (HumanPhys->Get_Collision_Group() != SOLDIER_GHOST_COLLISION_GROUP || soldier->Is_Safe_To_Disable_Ghost_Collision())
			HumanPhys->Set_Collision_Group(SOLDIER_COLLISION_GROUP);
		HumanPhys->Set_Immovable(false);
	}

	Update_Animation();
}

typedef enum {
	LEG_STYLE_STAND,					// A0
	LEG_STYLE_RUN_FORWARD,				// A1
	LEG_STYLE_RUN_BACKWARD,				// A2
	LEG_STYLE_RUN_LEFT,					// A3
	LEG_STYLE_RUN_RIGHT,				// A4
	LEG_STYLE_TURN_LEFT,				// A5
	LEG_STYLE_TURN_RIGHT,				// A6
	LEG_STYLE_WALK_FORWARD,				// B1
	LEG_STYLE_WALK_BACKWARD,			// B2
	LEG_STYLE_WALK_LEFT,				// B3
	LEG_STYLE_WALK_RIGHT,				// B4
	LEG_STYLE_CROUCH,					// C0
	LEG_STYLE_CROUCH_MOVE_FORWARD,		// C1
	LEG_STYLE_CROUCH_MOVE_BACKWARD,		// C2
	LEG_STYLE_CROUCH_MOVE_LEFT,			// C3
	LEG_STYLE_CROUCH_MOVE_RIGHT,		// C4
	LEG_STYLE_CROUCH_TURN_LEFT,			// C3
	LEG_STYLE_CROUCH_TURN_RIGHT,		// C4
	LEG_STYLE_JUMP_UP,					// J0
	LEG_STYLE_JUMP_FORWARD,				// J1
	LEG_STYLE_JUMP_BACKWARD,			// J2
	LEG_STYLE_JUMP_LEFT,				// J3
	LEG_STYLE_JUMP_RIGHT,				// J4
	LEG_STYLE_FLY_STATIONARY,			// Z0
	LEG_STYLE_FLY_FORWARD,				// Z1
	LEG_STYLE_FLY_BACKWARD,				// Z2
	LEG_STYLE_FLY_LEFT,					// Z3
	LEG_STYLE_FLY_RIGHT,				// Z4
	LEG_STYLE_FLY_UP,					// Z5
	LEG_STYLE_FLY_DOWN,					// Z6
} HUMAN_ANIM_LEG_STYLE;

void HumanStateClass::Update_Animation()
{
	
	if(AnimControl->Get_Skeleton() == 'V')
	{
		StateLocked = true;
		return;
	}
	
	if(StateLocked == true)
	{
		return;
	}
	
	switch(State)
	{
		case UPRIGHT:
		case AIRBORNE:
		case DEBUG_FLY:
		{
			int	leg_style = LEG_STYLE_STAND;
			int hold_style = WeaponHoldStyle;
			if ( State == AIRBORNE ) 
			{

				leg_style = LEG_STYLE_JUMP_UP;
				if ( SubState & SUB_STATE_LEFT )
					leg_style = LEG_STYLE_JUMP_LEFT;
				if ( SubState & SUB_STATE_RIGHT )
					leg_style = LEG_STYLE_JUMP_RIGHT;
				if ( SubState & SUB_STATE_FORWARD )
					leg_style = LEG_STYLE_JUMP_FORWARD;
				if ( SubState & SUB_STATE_BACKWARD )
					leg_style = LEG_STYLE_JUMP_BACKWARD;

			}
			else if ( State == DEBUG_FLY )
			{
				leg_style = LEG_STYLE_FLY_STATIONARY;

				if ( SubState & SUB_STATE_LEFT ) 	
					leg_style = LEG_STYLE_FLY_LEFT;
				if ( SubState & SUB_STATE_RIGHT )
					leg_style = LEG_STYLE_FLY_RIGHT;
				if ( SubState & SUB_STATE_FORWARD ) 
					leg_style = LEG_STYLE_FLY_FORWARD;
				if ( SubState & SUB_STATE_BACKWARD )
					leg_style = LEG_STYLE_FLY_BACKWARD;
				if ( SubState & SUB_STATE_UP )
					leg_style = LEG_STYLE_FLY_UP;
				if ( SubState & SUB_STATE_DOWN )
					leg_style = LEG_STYLE_FLY_DOWN;
			}
			else
			{
				if ( SubState & SUB_STATE_TURN_LEFT )
					leg_style = LEG_STYLE_TURN_LEFT;
				if ( SubState & SUB_STATE_TURN_RIGHT )
					leg_style = LEG_STYLE_TURN_RIGHT;
				if ( SubState & SUB_STATE_LEFT ) 	
					leg_style = LEG_STYLE_RUN_LEFT;
				if ( SubState & SUB_STATE_RIGHT )
					leg_style = LEG_STYLE_RUN_RIGHT;
				if ( SubState & SUB_STATE_FORWARD ) 
					leg_style = LEG_STYLE_RUN_FORWARD;
				if ( SubState & SUB_STATE_BACKWARD )
					leg_style = LEG_STYLE_RUN_BACKWARD;

				
				if ( Get_State_Flag( CROUCHED_FLAG ) ) 
				{
					// Tend to hold at chest when crouched
					if (( hold_style == WEAPON_HOLD_STYLE_HANDS_DOWN ) ||
						( hold_style == WEAPON_HOLD_STYLE_C4 ) ||
						( hold_style == WEAPON_HOLD_STYLE_BEACON ) ) 
					{

						hold_style = WEAPON_HOLD_STYLE_AT_CHEST;
					}

					leg_style += LEG_STYLE_CROUCH - LEG_STYLE_STAND;
				} 
				else if ( SubState & SUB_STATE_SLOW ) 
				{	
					if ( ( leg_style >= LEG_STYLE_RUN_FORWARD ) && 
						( leg_style <= LEG_STYLE_RUN_RIGHT ) ) 
					{
						leg_style += LEG_STYLE_WALK_FORWARD - LEG_STYLE_RUN_FORWARD;
					}
				}
			}
			if (hold_style > 9)
			{
				hold_style = 9;
			}
			const char * leg_anim_name = LegAnimNames[leg_style];
			const char * torso_anim_name = _weapon_style_names[hold_style];

			float blend_time = 0.2f;
			if ( NoAnimBlend ) 
			{
				blend_time = 0;
				NoAnimBlend = false;
			}
	
			if ( torso_anim_name[1] == '2' ) 
			{
				// Lets try aiming
				StringClass	anim1_name(0,true);
				StringClass	anim2_name(0,true);
				StringClass	anim3_name(0,true);
				anim1_name.Format( "S_A_HUMAN.H_A_%c1%s", 'A' + hold_style, leg_anim_name );
				anim2_name.Format( "S_A_HUMAN.H_A_%c2%s", 'A' + hold_style, leg_anim_name );
				anim3_name.Format( "S_A_HUMAN.H_A_%c3%s", 'A' + hold_style, leg_anim_name );

	
				// See if we have the tilting data
				HAnimClass * anim = WW3DAssetManager::TheInstance->Get_HAnim( anim3_name );
				if(anim != 0)
				{
					anim->Release_Ref();
					float tilt_blend = clamp(AimingTilt/1.134464f, -1.f, 1.f);
				
					float frame = AnimControl->Get_Frame();
					
					if(tilt_blend < 0)
						AnimControl->Set_Animation(anim1_name, anim2_name, 1.0f+tilt_blend, blend_time);
					else
						AnimControl->Set_Animation(anim3_name, anim2_name, 1.0f-tilt_blend, blend_time);
					
					AnimControl->Set_Mode(ANIM_MODE_LOOP, frame);
					return;
				}
			}
			
			StringClass	anim_name(0,true);
				
			anim_name.Format( "S_A_HUMAN.H_A_%s%s", torso_anim_name, leg_anim_name );
	
			if(HumanAnimOverride != 0)
			{
				if ( hold_style == WEAPON_HOLD_STYLE_EMPTY_HANDS ) 
				{
					if ( leg_style == LEG_STYLE_RUN_FORWARD ) 
					{
						anim_name = HumanAnimOverride->runEmptyHands;
					}
					if ( leg_style == LEG_STYLE_WALK_FORWARD ) 
					{
						anim_name = HumanAnimOverride->walkEmptyHands;
					}
				}
				if ( hold_style == WEAPON_HOLD_STYLE_AT_CHEST ) 
				{
					if ( leg_style == LEG_STYLE_RUN_FORWARD ) 
					{
						anim_name = HumanAnimOverride->runAtChest;
					}
					if ( leg_style == LEG_STYLE_WALK_FORWARD ) 
					{
						anim_name = HumanAnimOverride->walkAtChest;
					}
				}
				if ( hold_style == WEAPON_HOLD_STYLE_AT_HIP ) 
				{
					if ( leg_style == LEG_STYLE_RUN_FORWARD ) 
					{
						anim_name = HumanAnimOverride->runAtHip;
					}
					if ( leg_style == LEG_STYLE_WALK_FORWARD ) 
					{
						anim_name = HumanAnimOverride->walkAtHip;
					}
				}
			}
			if (State != DEBUG_FLY || WW3DAssetManager::TheInstance->Get_HAnim(anim_name) != NULL)
			{
				AnimControl->Set_Animation(anim_name, blend_time, 0);
				AnimControl->Set_Mode(ANIM_MODE_LOOP, -1);
			}
		}
		break;
			
		case DIVE:
		{
			srand((unsigned int)time(NULL));
			int index = (cGameType::GameType == 3 ? 0 : rand() & 1);
			if((SubState & 2) != 0)
				index += 2;

			if((SubState & 0x10) != 0)
				index += 4;

			if((SubState & 0x20) != 0)
				index += 6;

			AnimControl->Set_Animation(_dive_anims[index], 0.2f, 0.0);
			AnimControl->Set_Mode(ANIM_MODE_ONCE, -1);
			StateLocked = true;
		}
		break;
			
		case LAND:
		{
			int dir = 0;
			if ( SubState & SUB_STATE_LEFT )
				dir = 3;
			if ( SubState & SUB_STATE_RIGHT )
				dir = 4;
			if ( SubState & SUB_STATE_FORWARD )
				dir = 1;
			if ( SubState & SUB_STATE_BACKWARD )
				dir = 2;

			StringClass anim_name(0,true);
			anim_name.Format( "S_A_HUMAN.H_A_A0L%d", dir );
			AnimControl->Set_Animation( anim_name, 0.2f , 0);
			AnimControl->Set_Mode(ANIM_MODE_ONCE, -1);
		}
		break;
			
		case WOUNDED:
			AnimControl->Set_Animation(_WoundAnims[SubState], 0.2f, 0);
			AnimControl->Set_Mode(ANIM_MODE_ONCE, -1);
			break;
			
		case DEATH:
			AnimControl->Set_Animation(_DeathAnims[SubState], 0.2f, 0);
			AnimControl->Set_Mode(ANIM_MODE_ONCE, -1);
			StateLocked = true;
			break;
			
		case LADDER:
		{
			char* anim_name = "S_A_HUMAN.H_A_412A";
			if((SubState & SUB_STATE_UP) != 0)
				anim_name = "S_A_HUMAN.H_A_422A";

			if((SubState & SUB_STATE_DOWN) != 0)
				anim_name = "S_A_HUMAN.H_A_432A";

			AnimControl->Set_Animation(anim_name, 0.2f, 0);
			AnimControl->Set_Mode(ANIM_MODE_LOOP, -1);
		}
		break;
			
		case ANIMATION:
		case LOITER:
		case DESTROY:
		case IN_VEHICLE:
		case TRANSITION:
			break;

		/*case DEBUG_FLY:
			{
				StringClass var_48(0,true);
					
				var_48.Format("S_A_HUMAN.H_A_%sa0", _weapon_style_names[WeaponHoldStyle]);
	
				AnimControl->Set_Animation(AnimControl->Get_Animation_Name(), var_48, 0.5, 0.8);
				AnimControl->Set_Mode(ANIM_MODE_LOOP, -1);
			}
			break;*/
			
		case ON_FIRE:
		case ON_CNC_FIRE:
			AnimControl->Set_Animation("S_A_HUMAN.H_A_FLMA", 0.2f, 0);
			AnimControl->Set_Mode(ANIM_MODE_LOOP, -1);
			break;
			
		case ON_CHEM:
		case ON_CNC_CHEM:
			AnimControl->Set_Animation("S_A_HUMAN.h_a_6x01", 0.2f, 0);
			AnimControl->Set_Mode(ANIM_MODE_LOOP, -1);
			break;
			
		case ON_ELECTRIC:
			AnimControl->Set_Animation("S_A_HUMAN.h_a_6x05", 0.2f, 0);
			AnimControl->Set_Mode(ANIM_MODE_LOOP, -1);
			break;
			
		default:
			AnimControl->Set_Animation((char*)0);
			break;
	}		
}


void HumanStateClass::Set_Sub_State(int newSubState)
{
	if(Is_Sub_State_Adjustable() && newSubState != SubState)
	{
		SubState = newSubState;
		Update_Animation();
	}
}

bool HumanStateClass::Is_Sub_State_Adjustable()
{
	return State == UPRIGHT || State == LADDER || State == DEBUG_FLY;
}

bool HumanStateClass::Is_State_Interruptable()
{
	return State == UPRIGHT || State == LAND || State == ANIMATION || State == WOUNDED || State == LOITER;
}

void HumanStateClass::Reset_Loiter_Delay()
{
	srand((unsigned int)time(NULL));
	LoiterDelay = rand() / (RAND_MAX/6.f) - 3.0f;
}


void HumanStateClass::Update_State()
{
	StateTimer += TimeManager::FrameSeconds;
	
	if(AnimControl)
	{
		if(AnimControl->Get_Skeleton() == 'V')
		{
			LoitersAllowed = false;
		}
	}
	if(State == UPRIGHT && LoitersAllowed)
	{
		if(SubState != 0)
		{
			Reset_Loiter_Delay();
		}

		LoiterDelay += TimeManager::FrameSeconds;

		HumanLoiterGlobalSettingsDef* loiter = HumanLoiterCollection;

		if(!HumanLoiterCollection)
		{
			loiter = HumanLoiterGlobalSettingsDef::Get_Default_Loiters();
		}
		if(loiter)
		{
			if(LoiterDelay > loiter->activationDelay)
			{
				Set_State(LOITER, SubState);

				StringClass anim(loiter->Pick_Animation(), true);
				if ( !anim.Is_Empty() ) {
					StringClass new_anim(true);
					Strip_Path_From_Filename( new_anim, anim );
					// remove the .W3D
					if ( new_anim.Get_Length() >= 5 ) {
						new_anim.Erase( new_anim.Get_Length()-4, 4 );
					}
					AnimControl->Set_Animation( new_anim, 0.2f, 0 );
					AnimControl->Set_Mode( ANIM_MODE_ONCE , -1 );
					AnimControl->Update( 0 );	// update 
				}
			}
		}
	}
	else
	{
		Reset_Loiter_Delay();
	}

	if(AnimControl->Is_Complete())
	{
		if((StateLocked && State != LOCKED_ANIMATION) || State == DEATH)
		{
			StateLocked = false;
			if(State != DIVE && State != ANIMATION)
			{
				if(State == DEATH)
				{
					Set_State(DESTROY, 0);
					TransitionEffectClass* deathEffect = CombatMaterialEffectManager::Get_Death_Effect();
					HumanPhys->Add_Effect_To_Me(deathEffect);
					
					deathEffect->Release_Ref();
				}
				else if(State == TRANSITION)
				{
					Set_State(TRANSITION_COMPLETE, 0);
				}
			}
			else
			{
				Set_State(UPRIGHT, 0);
			}
		}
		else if(State == LOITER)
		{
			Set_State(UPRIGHT, 0);
		}
		else if(State == LAND)
		{
			Set_State(UPRIGHT, SubState);
		}
		else if(State == WOUNDED)
		{
			Set_State(UPRIGHT, 0);
		}
	}
	
	if(!HumanPhys->Is_In_Contact() && State == UPRIGHT)
	{
		Begin_Jump();
	}
	else if(HumanPhys->Is_In_Contact() && State == AIRBORNE)
	{
		Complete_Jump();
	}

	if(WeaponHoldTimer > 0 && !Get_State_Flag(CROUCHED_FLAG))
	{
		Reset_Loiter_Delay();
		WeaponHoldTimer -= TimeManager::FrameSeconds;
		if(WeaponHoldTimer <= 0)
		{
			if(WeaponHoldStyle == WEAPON_HOLD_STYLE_HANDGUN || WeaponHoldStyle == WEAPON_HOLD_STYLE_C4 || WeaponHoldStyle == WEAPON_HOLD_STYLE_BEACON)
			{
				WeaponHoldStyle = WEAPON_HOLD_STYLE_HANDS_DOWN;
			}
			else
			{
				WeaponHoldStyle = WEAPON_HOLD_STYLE_AT_CHEST;
			}
			Update_Animation();
		}
	}
}

void HumanStateClass::Init(HumanPhysClass *phys)
{
	HumanPhys = phys;
	phys->Add_Ref();
}

void HumanStateClass::Set_Anim_Control(HumanAnimControlClass *control)
{
	AnimControl = control;
	AnimControl->Set_Model(HumanPhys->Peek_Model());
}

void HumanStateClass::Set_Human_Anim_Override(int id)
{
	HumanAnimOverride = (HumanAnimOverrideDef *)DefinitionMgrClass::Find_Definition(id,true);
}

void HumanStateClass::Set_Human_Loiter_Collection(int id)
{
	HumanLoiterCollection = (HumanLoiterGlobalSettingsDef *)DefinitionMgrClass::Find_Definition(id,true);
}

void HumanStateClass::Begin_Jump()
{
	JumpTM = HumanPhys->Get_Transform();

	Vector3 velocity;
	HumanPhys->Get_Velocity(&velocity);
	velocity = HumanPhys->Get_Transform().Inverse_Rotate_Vector(velocity);

	int newSubState = 0;

	if(velocity.X > 0.2f)
	{
		newSubState |= 1;
	}
	else if(velocity.X < -0.2f)
	{
		newSubState |= 2;
	}

	if(velocity.Y > 0.2f)
	{
		newSubState |= 0x10;
	}
	else if(velocity.Y < -0.2f)
	{
		newSubState |= 0x20;
	}
	
	Set_State(AIRBORNE, newSubState);

	if (Exe == 0)
	{
		SurfaceEffectsManager::Apply_Effect(HumanPhys->Get_Contact_Surface_Type(),5, HumanPhys->Get_Transform(), 0, 0, true, true);
	}
}

void HumanStateClass::Complete_Jump()
{
	Vector3 startPos(JumpTM[0].W, JumpTM[1].W, JumpTM[2].W);
	Vector3 endPos(HumanPhys->Get_Transform()[0].W, HumanPhys->Get_Transform()[1].W, HumanPhys->Get_Transform()[2].W);

	Vector3 movement = startPos - endPos;

	if (movement.Z > GlobalSettingsDef::GlobalSettings->FallingDamageMinDist)
	{
		float fallDamageFactor = (movement.Z - GlobalSettingsDef::GlobalSettings->FallingDamageMinDist)/(GlobalSettingsDef::GlobalSettings->FallingDamageMaxDist-GlobalSettingsDef::GlobalSettings->FallingDamageMinDist);
		fallDamageFactor = clamp(fallDamageFactor, 0.0f, 1.0f);

		if(HumanPhys->o308.groundObject)
		{
			if(HumanPhys->o308.groundObject->As_ElevatorPhysClass())
			{
				fallDamageFactor = 0.0;
			}
		}

		if(HumanPhys->Get_Observer())
		{
			PhysicalGameObj* physicalObj = ((CombatPhysObserverClass *)(HumanPhys->Get_Observer()))->As_PhysicalGameObj();
			float maxHealth = physicalObj->Get_Defense_Object()->Get_Health_Max();

			for(int i = 0; i < physicalObj->Get_Observers().Count(); i ++)
			{
				physicalObj->Get_Observers()[i]->Custom(physicalObj, CUSTOM_EVENT_FALLING_DAMAGE, (int)(maxHealth * fallDamageFactor), 0);
			}

			OffenseObjectClass damage(maxHealth * fallDamageFactor, GlobalSettingsDef::GlobalSettings->FallingDamageWarhead, 0);
			
			physicalObj->Apply_Damage_Extended(damage, fallDamageFactor, movement, 0);
		}
	}
	
	Vector3 velocity;

	HumanPhys->Get_Velocity(&velocity);

	velocity = HumanPhys->Get_Transform().Inverse_Rotate_Vector(velocity);

	int newSubState = 0;

	if(velocity.X > 0.2f)
	{
		newSubState |= 1;
	}
	else if(velocity.X < -0.2f)
	{
		newSubState |= 2;
	}

	if(velocity.Y > 0.2f)
	{
		newSubState |= 0x10;
	}
	else if(velocity.Y < -0.2f)
	{
		newSubState |= 0x20;
	}

	Set_State(LAND, newSubState);

	Matrix3D var_7C = HumanPhys->Get_Transform();

	if (Exe == 0)
	{
		SurfaceEffectsManager::Apply_Effect(HumanPhys->Get_Contact_Surface_Type(), 6, var_7C, 0, 0, true, true);
	}
}

//temporary, remove when we add the complete WWMath class
inline float Fabs(float val)
{
	int value=*(int*)&val;
	value&=0x7fffffff;
	return *(float*)&value;
}


void HumanStateClass::Post_Think( void )
{
	// Update sub_state per movement
	// do it for upright, land, ladder, airborne,
	if ( Is_Sub_State_Adjustable() || Is_State_Interruptable() ) {

		// Update the SubState
		int new_sub_state = 0;

		// Get our current move vector
		Vector3	move_vector;
		move_vector = HumanPhys->animationMove;
		if ( TimeManager::FrameSeconds > 0 ) {
			move_vector /= TimeManager::FrameSeconds;
		}

		move_vector = HumanPhys->Get_Transform().Inverse_Rotate_Vector( move_vector );

		// When walking running diagonally, use forward/backward legs.
		// Unless you are crouched, then use straffe legs
		float direction_ratio = 0.75f;
		if ( Get_State_Flag( CROUCHED_FLAG ) ) {		
			direction_ratio = 2;
		}

		// Convert to SubMode
		if (Fabs( move_vector[0] ) > direction_ratio * Fabs( move_vector[1] ) ) {
				  if ( move_vector[0] > 0.2f ) 	new_sub_state |= SUB_STATE_FORWARD;
			else if ( move_vector[0] < -0.2f )	new_sub_state |= SUB_STATE_BACKWARD;
		} else {
				  if ( move_vector[1] > 0.2f ) 	new_sub_state |= SUB_STATE_LEFT;
			else if ( move_vector[1] < -0.2f )	new_sub_state |= SUB_STATE_RIGHT;
		}

		if ( new_sub_state == 0 ) {
				  if ( move_vector[2] > 0.2f ) 	new_sub_state |= SUB_STATE_UP;
			else if ( move_vector[2] < -0.2f )	new_sub_state |= SUB_STATE_DOWN;
		}

		if ( new_sub_state != 0 ) {
			if ( move_vector.Length() < 3.21f ) new_sub_state |= SUB_STATE_SLOW;
			//Debug_Say(( "%f %f\n", move_vector.Length(), WALKING_THRESHHOLD ));
		}																		

#if 0	// No turn anims!!!
		// Get our current turn vector
		if ( turnVelocity > 0 )				new_sub_state |= SUB_STATE_TURN_LEFT;
		else if ( turnVelocity < 0 ) 		new_sub_state |= SUB_STATE_TURN_RIGHT;
		turnVelocity = 0;
#endif

		// Get him out of WOUNDED, LAND, LOITER states if moving or shooting
		if ( Is_State_Interruptable() && State != UPRIGHT ) {
//			if ( new_sub_state != 0 || WeaponState > WeaponClass::STATE_READY ) {

			if ( new_sub_state != 0 || WeaponFired ) {
				if ( State == LAND && SubState == new_sub_state ) {
					// don't interrupt lands for the same direction
				} else {
//					Debug_Say(( "Interrupt State %s\n", Get_State_Name() ));
					Set_State( UPRIGHT , SubState );
				}
			}
		}
		
		if ( Is_Sub_State_Adjustable() ) {

			if ( new_sub_state != SubState ) {
				Set_Sub_State( (HumanSubStateType)new_sub_state );
			}


#if 0		// Disable all leg twisting

// ===================================================================			// LEG TWIST!!!!!

			// Don't leg twist for crouched
			if ( state == UPRIGHT && !Get_State_Flag( CROUCHED_FLAG ) ) {

				float	legs_rotation = 0;

				// Compare the facing to the motion, set leg_racing to the difference
				if ( move_vector.Length() > 0 ) {
					float move_direction = atan2( -move_vector.Y, move_vector.X );

					if ( new_sub_state & SUB_STATE_FORWARD ) {
						legs_rotation = -move_direction;
					} else if ( new_sub_state & SUB_STATE_BACKWARD ) {
						legs_rotation = -move_direction + DEG_TO_RADF( 180.f );
					} else if ( new_sub_state & SUB_STATE_LEFT ) {
						legs_rotation = -move_direction + DEG_TO_RADF( 270.f );
					} else if ( new_sub_state & SUB_STATE_RIGHT ) {
						legs_rotation = -move_direction + DEG_TO_RADF( 90.f );
					}

					legs_rotation = wrap( legs_rotation, DEG_TO_RADF( -180.f ), DEG_TO_RADF( 180.f ) );
//					legs_rotation = WWMath::Clamp( legs_rotation, DEG_TO_RADF( -45 ), DEG_TO_RADF( 45 ) );
					legs_rotation = clamp( legs_rotation, DEG_TO_RADF( -30.f ), DEG_TO_RADF( 30.f ) );

					if ( Fabs( legs_rotation ) < DEG_TO_RAD( 25 ) ) {
						legs_rotation = 0;
					}
				}

				// Move LegRotation toward leg_rotation
				float rot_diff = legs_rotation - legRotation;
				float max_mov = DEG_TO_RADF( 180.f ) * TimeManager::FrameSeconds;
//				float max_mov = DEG_TO_RAD( 90 ) * TimeManager::Get_Frame_Seconds();
				rot_diff = clamp( rot_diff, -max_mov, max_mov );
				legRotation += rot_diff;
				legs_rotation = legRotation;

				// I'm making this staic for now, because all human
				// skeletons have the bone at the same index
				static int  root_bone = -1;
				if ( root_bone == -1 ) {			// Get root bone index
					root_bone = HumanPhys->Peek_Model()->Get_Bone_Index( "c spine" );
				}

				static int  torso_bone = -1;
				if ( torso_bone == -1 ) {			// Get torso bone index
					torso_bone = HumanPhys->Peek_Model()->Get_Bone_Index( "c spine1" );
				}

				if ( legs_rotation != 0 ) {

					TT_ASSERT( root_bone != -1 );
					TT_ASSERT( torso_bone != -1 );

					if ( !HumanPhys->Peek_Model()->Is_Bone_Captured( root_bone ) ) {
						HumanPhys->Peek_Model()->Capture_Bone( root_bone );
					}
					if ( !HumanPhys->Peek_Model()->Is_Bone_Captured( torso_bone ) ) {
						HumanPhys->Peek_Model()->Capture_Bone( torso_bone );
					}

					Matrix3D	root_adjust(1);				// adjust it
					root_adjust.Rotate_X( legs_rotation );
					HumanPhys->Peek_Model()->Control_Bone( root_bone, root_adjust , false );

					Matrix3D	legs_adjust(1);				// adjust it
					legs_adjust.Rotate_X( -legs_rotation );
					HumanPhys->Peek_Model()->Control_Bone( torso_bone, legs_adjust , false);
				} else {	// no adjustment, release
	 				if ( HumanPhys->Peek_Model()->Is_Bone_Captured( root_bone ) ) {
						HumanPhys->Peek_Model()->Release_Bone( root_bone );
					}
	 				if ( HumanPhys->Peek_Model()->Is_Bone_Captured( torso_bone ) ) {
						HumanPhys->Peek_Model()->Release_Bone( torso_bone );
					}
				}
			}
#endif
		}

		// Scale animation speed
		float ideal_speed = 0;
		if ( !(new_sub_state & SUB_STATE_SLOW) ) {
			if ( new_sub_state & SUB_STATE_FORWARD )	ideal_speed = 5.5f;
			if ( new_sub_state & SUB_STATE_BACKWARD )	ideal_speed = 4.5f;
			if ( new_sub_state & SUB_STATE_LEFT )		ideal_speed = 4.5f;
			if ( new_sub_state & SUB_STATE_RIGHT )		ideal_speed = 5.5f;
		} else {
			if ( new_sub_state & SUB_STATE_FORWARD )	ideal_speed = 1.6f;
			if ( new_sub_state & SUB_STATE_BACKWARD )	ideal_speed = 1.5f;
			if ( new_sub_state & SUB_STATE_LEFT )		ideal_speed = 1.5f;
			if ( new_sub_state & SUB_STATE_RIGHT )		ideal_speed = 1.6f;
		}

		if ( State == LADDER ) {
			if ( new_sub_state & SUB_STATE_UP )			ideal_speed = 0.15f;
			if ( new_sub_state & SUB_STATE_DOWN )		ideal_speed = 0.15f;
		}

		// Turning is at speed 1
		bool turning = ( new_sub_state & (SUB_STATE_TURN_LEFT | SUB_STATE_TURN_RIGHT ) && 
						  !( new_sub_state & (SUB_STATE_FORWARD | SUB_STATE_BACKWARD )));

		if ( !turning && ideal_speed != 0 ) {
			// Get Anim_Speed_Scale
			Vector3	vel;
			vel = HumanPhys->animationMove;
			if ( TimeManager::FrameSeconds > 0 ) {
				vel /= TimeManager::FrameSeconds;
			}
			float speed = clamp( vel.Length() / ideal_speed, 0.33f, 3.0f );
			AnimControl->Set_Anim_Speed_Scale( speed );
		} else {
			AnimControl->Set_Anim_Speed_Scale( 1 );
		}

		HumanPhys->animationMove.Set(0.0f, 0.0f, 0.0f);

	}
}

/*
**
*/
struct BoneToOuchType {
	const char * bone_name;
	int ouch_type;
};

BoneToOuchType	_BoneToOuchTypeList[] = {
	{ "K_HEAD", HumanStateClass::HEAD_FROM_BEHIND },
	{ "K_NECK", HumanStateClass::HEAD_FROM_BEHIND },
	{ "K_CHEST", HumanStateClass::HEAD_FROM_BEHIND },
	{ "K_ABDOMEN", HumanStateClass::TORSO_FROM_BEHIND },
	{ "K_PELVIS", HumanStateClass::GROIN },
	{ "K_L THIGH", HumanStateClass::LEFT_LEG_FROM_BEHIND },
	{ "K_L CALF", HumanStateClass::LEFT_LEG_FROM_BEHIND },
	{ "K_L FOOT", HumanStateClass::LEFT_LEG_FROM_BEHIND },
	{ "K_L HAND", HumanStateClass::LEFT_ARM_FROM_BEHIND },
	{ "K_L FOREARM", HumanStateClass::LEFT_ARM_FROM_BEHIND },
	{ "K_L UPPERARM", HumanStateClass::LEFT_ARM_FROM_BEHIND },
	{ "K_R THIGH", HumanStateClass::RIGHT_LEG_FROM_BEHIND },
	{ "K_R CALF", HumanStateClass::RIGHT_LEG_FROM_BEHIND },
	{ "K_R FOOT", HumanStateClass::RIGHT_LEG_FROM_BEHIND },
	{ "K_R HAND", HumanStateClass::RIGHT_ARM_FROM_BEHIND },
	{ "K_R FOREARM", HumanStateClass::RIGHT_ARM_FROM_BEHIND },
	{ "K_R UPPERARM", HumanStateClass::RIGHT_ARM_FROM_BEHIND },
};
#define	BONE_LIST_COUNT	( sizeof(_BoneToOuchTypeList) / sizeof(_BoneToOuchTypeList[0]) )


int	HumanStateClass::Get_Ouch_Type( const	Vector3 & direction, const char * collision_box_name )
{
	// Initialize ouch_type to a default value
	int ouch_type = TORSO_FROM_FRONT;
	
	const char * base_name = NULL;
	if ( collision_box_name != NULL ) {
		base_name = ::strchr( collision_box_name, '.' );
	} else {
		return ouch_type;
	}

	if ( base_name != NULL ) {
		base_name++;
		for ( int i = 0; i < BONE_LIST_COUNT; i++ ) {
			if ( ::strcmp( _BoneToOuchTypeList[i].bone_name, base_name ) == 0 ) {
				ouch_type = _BoneToOuchTypeList[i].ouch_type;
			}
		}
	}

	if ( ouch_type != -1 ) {
		// Set direction
		Vector3 relative_direction = HumanPhys->Get_Transform().Inverse_Rotate_Vector( direction );
		if ( ouch_type != HumanStateClass::GROIN ) {		// but not for the groin shot
			if ( relative_direction.X < 0 ) {
				ouch_type += 1;
			}
		}
	} else {
		//Debug_Say(( "Bad human collision box name %s\n", collision_box_name ));
	}

	return ouch_type;
}

void	HumanStateClass::Update_Weapon( WeaponClass * weapon, bool new_weapon )
{
	Update_Recoil(weapon);
	int new_hold_style = WeaponHoldStyle;
	WeaponFired = false;
	if ( weapon ) {

		if ( weapon->Is_Reloading() ) {
			Raise_Weapon();
		}

		weapon->Set_Safety( WeaponHoldStyle > WEAPON_HOLD_STYLE_EMPTY_HANDS );

		if ( weapon->Get_Owner() && 
			  weapon->Get_Owner()->As_SoldierGameObj() && 
			  weapon->Get_Owner()->As_SoldierGameObj()->Is_Human_Controlled() ) {
			WeaponFired = weapon->Is_Firing();
		}


		if ( weapon->Is_Triggered() || new_weapon || Get_State_Flag( CROUCHED_FLAG ) ) {
			if ( weapon->Is_Safety_Set() ) {
				NoAnimBlend = true;
			}
			Raise_Weapon();
			new_hold_style = weapon->Get_Style();
		}

		if ( WeaponHoldTimer > 0 ) {
			if ( WeaponHoldStyle > WEAPON_HOLD_STYLE_EMPTY_HANDS ) {
				new_hold_style = weapon->Get_Style();
			}
		}
	} else {
		new_hold_style = WEAPON_HOLD_STYLE_EMPTY_HANDS;
	}
	if ( new_hold_style == WeaponHoldStyle ) {
		return;
	}
	WeaponHoldStyle = new_hold_style;
	if ( !StateLocked ) {
		Update_Animation();
	}
}

RENEGADE_FUNCTION
void HumanStateClass::Update_Recoil(WeaponClass * weapon)
AT2(0x0069AD00,0x0069A5A0);

void	HumanStateClass::Start_Scripted_Animation( const char * anim_name, bool blend, bool looping )
{
	if (( Get_State() == DEATH ) || ( Get_State() == DESTROY ) ) {
		return;
	}
	Set_State( ANIMATION );
	float blend_time = blend ? 0.2f : 0;
	AnimControl->Set_Animation( anim_name, blend_time );
	AnimControl->Set_Mode( looping ? ANIM_MODE_LOOP : ANIM_MODE_ONCE );
	AnimControl->Update( 0 );	// update 
	StateLocked = true;
}

void	HumanStateClass::Start_Transition_Animation( const char * anim_name, bool blend )
{
	if ( StateLocked ) {
		return;
	}
	if (( Get_State() == DEATH ) || ( Get_State() == DESTROY ) ) {
		return;
	}
	Set_State( TRANSITION );
	float blend_time = blend ? 0.2f : 0;
	AnimControl->Set_Animation( anim_name, blend_time );
	AnimControl->Set_Mode( ANIM_MODE_ONCE );
	AnimControl->Update( 0 );
	StateLocked = true;
}


void	HumanStateClass::Stop_Scripted_Animation( void )
{
	if ( Get_State() != ANIMATION && Get_State() != LOCKED_ANIMATION ) {
//		Debug_Say(( "Not in a Scripted Animation to stop\n" ));

		// Better clear the locked state.  This was keeping us in the place beacon mode
		StateLocked = false;
		return;
	}

	StateLocked = false;
	Set_State( UPRIGHT );
//	AnimControl->Lock_Animation();
}

void	HumanStateClass::Update_Aiming( float tilt, float turn )
{
	if ( (AimingTilt == tilt) && (AimingTurn == turn) )
	{
		return;
	}
	AimingTilt = tilt;
	AimingTurn = turn;
	if ( !StateLocked )
	{
		Update_Animation();
	}
}
