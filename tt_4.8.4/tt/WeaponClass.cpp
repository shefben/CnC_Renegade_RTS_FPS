#include "general.h"

#include "WeaponClass.h"
#include "WeaponMgr.h"
#include "WeaponBagClass.h"
#include "render2d.h"
#include "RenderObjClass.h"
#include "tt.h"
#include "AudibleSoundClass.h"
#include "WeaponManager.h"
#include "CombatManager.h"
#include "SoldierGameObj.h"
#include "TimeManager.h"
#include "RefCountedReferencerClass.h"
#include "WWAudioClass.h"
#include "Random2Class.h"
#include "ObjectLibraryManager.h"
#include "FileHashEvent.h"
#include "WeaponEvent.h"
#include "cPlayer.h"
#include "C4GameObj.h"
#include "BeaconGameObj.h"
#include "ParticleEmitterClass.h"
#include "PhysicsSceneClass.h"
#include "WeaponViewClass.h"
#include "WW3DAssetManager.h"
#include "DefinitionMgrClass.h"
#include "TimedDecorationPhysClass.h"
#include "RigidBodyClass.h"
#include "ProjectileClass.h"
#include "CCameraClass.h"
#include "PhysColTest.h"
float zoomDivisor = 0.8f;

float defaultMinZoom;
float defaultMaxZoom;

float& MinZoom = *(float*)0x007E42E4;
float& MaxZoom = *(float*)0x007E42E0;

Render2DClass *ScopeRender2DClass = 0;
Render2DClass *OriginalScopeRender2DClass = 0;

Render2DClass **SniperHud = (Render2DClass **)0x0085F7F8;

bool customScope = false;

void WeaponClass::Select()
{
	Set_State(STATE_START_SWITCH);
	if(Get_Definition()->CanSnipe)
	{
		if(!Exe && Owner.Get_Ptr() == CombatManager::Get_The_Star())
		{
			for(int i = 0; i < ScopeList.Count(); i ++)
			{
				if(_stricmp(ScopeList[i]->WeaponPreset, Get_Name()) == 0)
				{
					DWORD oldprotect;
	
					VirtualProtect((void*)0x007E42E4, 4, PAGE_READWRITE, &oldprotect);
					VirtualProtect((void*)0x007E42E0, 4, PAGE_READWRITE, &oldprotect);
				
					defaultMaxZoom = MaxZoom;
					defaultMinZoom = MinZoom;

					ScopeRender2DClass = CreateRender2DClass();
					char *Name = newstr(ScopeList[i]->ScopeTexture);
					FileClass *f = Get_Data_File(Name);
					if ((!f) || (!f->Is_Available()))
					{
						int len = strlen(Name);
						Name[len - 3] = 'd';
						Name[len - 2] = 'd';
						Name[len - 1] = 's';
						f = Get_Data_File(Name);
					}
					if ((f) && (f->Is_Available()))
					{
						unsigned int size = f->Size();
						char *data = new char[size];
						f->Open(1);
						f->Read(data,size);
						CheckGlobal(data,size,Name,HashCustomScope);
						delete[] data;
						f->Close();
						Close_Data_File(f);
					}
					ScopeRender2DClass->Set_Texture(Name);
					delete[] Name;
					RectClass uv = RectClass(0,0,1,1);
					ScopeRender2DClass->Add_Quad(Render2DClass::Get_Screen_Resolution(),uv,0xFFFFFFFF);
					MinZoom = zoomDivisor/ScopeList[i]->MinZoom;
					MaxZoom = zoomDivisor/ScopeList[i]->MaxZoom;
					OriginalScopeRender2DClass = *SniperHud;
					(*SniperHud) = ScopeRender2DClass;

					customScope = true;

					break;
				}
			}
		}
	}
}

void WeaponClass::Set_State(WeaponStateType new_state)
{
	#define	SWITCH_TIME		1.0f
	#define	RELOAD_TIME		Definition->ReloadTime
	#define	READY_TIME		25.0f
	State = new_state;
	if ( State == STATE_START_SWITCH )	StateTimer = SWITCH_TIME/2.0f;
	if ( State == STATE_END_SWITCH )		StateTimer = SWITCH_TIME/2.0f;
	if ( State == STATE_RELOAD )			StateTimer = (float) RELOAD_TIME;
	if ( State == STATE_READY )			StateTimer = READY_TIME;
	if ( State == STATE_FIRE_PRIMARY )	StateTimer = 1.0f / PrimaryAmmoDefinition->RateOfFire;
	if ( State == STATE_FIRE_SECONDARY )StateTimer = 1.0f / SecondaryAmmoDefinition->RateOfFire;
	if ( State == STATE_CHARGE )			StateTimer = PrimaryAmmoDefinition->ChargeTime;
}

void WeaponClass::Deselect()
{
	Do_Continuous_Effects(false);
	IsPrimaryTriggered = false;
	IsSecondaryTriggered = false;
	LastFrameIsPrimaryTriggered = false;
	LastFrameIsSecondaryTriggered = false;
	if(customScope && !Exe && Owner.Get_Ptr() == CombatManager::Get_The_Star())
	{
		MaxZoom = defaultMaxZoom;
		MinZoom = defaultMinZoom;
		(*SniperHud) = OriginalScopeRender2DClass;
	}
}

RenderObjClass *Create_Render_Obj_From_Filename(const char *filename)
{
	StringClass str;
	Strip_Path_From_Filename(str,filename);
	str.Erase(str.Get_Length() - 4,4);
	return WW3DAssetManager::TheInstance->Create_Render_Obj(str);
}

void WeaponClass::Do_Continuous_Effects(bool enable)
{
	if (!enable)
	{
		for (int i = 0;i < ContinuousEmitters.Length();i++)
		{
			if ( ContinuousEmitters[i] != NULL )
			{
				if ( ContinuousEmitters[i]->Peek_Scene() != NULL )
				{
					PhysicsSceneClass::Get_Instance()->Remove_Render_Object( ContinuousEmitters[i] );
				}
				ContinuousEmitters[i]->Stop();
				ContinuousEmitters[i]->Release_Ref();
				ContinuousEmitters[i] = NULL;
			}
		}
		if ( ContinuousSound != NULL )
		{
			ContinuousSound->Stop(true);
			ContinuousSound->Remove_From_Scene();
			ContinuousSound->Release_Ref();
			ContinuousSound = NULL;
		}
	}
	if ( enable )
	{
		const AmmoDefinitionClass *ammo_def = PrimaryAmmoDefinition;
		if (IsPrimaryTriggered == false && IsSecondaryTriggered)
		{
			ammo_def = SecondaryAmmoDefinition;
		}
		int i;
		if ( ContinuousEmitters.Length() == 0 )
		{
			int num_muzzles = 1;
			if ( Get_Muzzle( 0 ) != Get_Muzzle( 1 ) )
			{
				num_muzzles = 2;
			}
			ContinuousEmitters.Resize( num_muzzles );
			for ( int i = 0; i < ContinuousEmitters.Length(); i++ )
			{
				ContinuousEmitters[i] = NULL;
			}
		}
		for ( i = 0; i < ContinuousEmitters.Length(); i++ )
		{
			if ( ContinuousEmitters[i] == NULL && !ammo_def->ContinuousEmitterName.Is_Empty() )
			{
				RenderObjClass * renderobj = Create_Render_Obj_From_Filename( ammo_def->ContinuousEmitterName );
				if ( renderobj )
				{
					ContinuousEmitters[i] = (ParticleEmitterClass *)renderobj;
					ContinuousEmitters[i]->Set_Velocity_Inheritance_Factor( 1 );
				}
				if ( ContinuousEmitters[i] )
				{
					ContinuousEmitters[i]->Start();
					PhysicsSceneClass::Get_Instance()->Add_Render_Object( ContinuousEmitters[i] );
				}
			}
		}
		if ( ContinuousSound == NULL && ammo_def->ContinuousSoundDefID != 0 )
		{
			ContinuousSound = WWAudioClass::Get_Instance()->Create_Continuous_Sound( ammo_def->ContinuousSoundDefID,0,0,2);
			if ( ContinuousSound != NULL )
			{
				ContinuousSound->Add_To_Scene( true );
			}
		}
		if ( (Get_Owner() == COMBAT_STAR) && CombatManager::Is_First_Person() )
		{
			Vector3	fp_muzzle_pos = WeaponViewClass::Get_Muzzle_Pos();
			Matrix3D muzzle;
			muzzle.Obj_Look_At( fp_muzzle_pos, Get_Owner()->Get_Targeting_Pos(), 0 );
			for ( i = 0; i < ContinuousEmitters.Length(); i++ )
			{
				if ( ContinuousEmitters[i] != NULL )
				{
					ContinuousEmitters[i]->Set_Transform( muzzle );
				}
			}
			if ( ContinuousSound != NULL )
			{
				ContinuousSound->Set_Transform( muzzle );
			}
		}
		else
		{
			for ( i = 0; i < ContinuousEmitters.Length(); i++ )
			{
				if ( ContinuousEmitters[i] != NULL )
				{
					ContinuousEmitters[i]->Set_Transform( Get_Muzzle( i ) );
				}
			}
			if ( ContinuousSound != NULL )
			{
				ContinuousSound->Set_Transform( Get_Muzzle() );
			}
		}
	}
}

WeaponClass::WeaponClass( const WeaponDefinitionClass *def ) :
	Definition( NULL ),
	PrimaryAmmoDefinition( NULL ),
	SecondaryAmmoDefinition( NULL ),
	Model( NULL ),
	State( STATE_IDLE ),
	StateTimer( 0.0f ),
	UpdateModel( WEAPON_MODEL_UPDATE_IS_NEEDED ),
	NextAnimState( WEAPON_ANIM_NOT_FIRING ),
	CurrentAnimState( WEAPON_ANIM_NOT_FIRING ),
	LastFrameIsPrimaryTriggered( false ),
	LastFrameIsSecondaryTriggered( false ),
	IsPrimaryTriggered( false ),
	IsSecondaryTriggered( false ),
	TotalRoundsFired( 0 ),
	ClipRounds( 0 ),
	InventoryRounds( 0 ),
	BurstDelayTimer( 0 ),
	BurstCount( 0 ),
	BulletBumpTime( 0 ),
   DidFire( false ),
	ContinuousEmitters( NULL ),
	ContinuousSound( NULL ),
	C4DetonationMode( 1 ),
	Target( 0, 0, 0 ),
	FiringSound( NULL ),
	FiringSoundDefID( 0 ),
	WeaponExists( true ),
	SafetySet( false ),
	LockTriggers( false ),
	EmptySoundTimer( 0 )
{
	if ( def != NULL )
	{
		Init( def );
	}
}

WeaponClass::~WeaponClass()
{
	if (FiringSound != NULL)
	{
		FiringSound->Remove_From_Scene();
		FiringSoundDefID = 0;
	}
	REF_PTR_RELEASE(Model);
	REF_PTR_RELEASE(FiringSound);
	Do_Continuous_Effects(false);
}

void WeaponClass::Init(const WeaponDefinitionClass *weapon_def)
{
	Definition = weapon_def;
	PrimaryAmmoDefinition = WeaponManager::Find_Ammo_Definition( Definition->PrimaryAmmoDefID );
	SecondaryAmmoDefinition = WeaponManager::Find_Ammo_Definition( Definition->SecondaryAmmoDefID );

	int cs = weapon_def->ClipSize;
	int mir = weapon_def->MaxInventoryRounds;
   if (0 > cs)
      Set_Clip_Rounds(-1);
   if (0 > mir)
      Set_Inventory_Rounds(-1);
}

void WeaponClass::Set_Primary_Triggered
   (bool triggered)
{
	if ( !LockTriggers )
	{
		IsPrimaryTriggered = triggered;
	}
}

void WeaponClass::Set_Secondary_Triggered
   (bool triggered)
{
	if ( !LockTriggers )
	{
		IsSecondaryTriggered = triggered;
	}
}

void WeaponClass::Next_C4_Detonation_Mode()
{
#define	LAST_C4_DETONATION_MODE	3
	if (++C4DetonationMode > LAST_C4_DETONATION_MODE)
	{
		C4DetonationMode = 1;
	}
}

void WeaponClass::Force_Reload()
{
	if ( Is_Reload_OK() && State <= STATE_READY )
	{
		Set_State( STATE_RELOAD );
		if ( Definition->ReloadSoundDefID != 0 )
		{
			Matrix3D	muzzle = Get_Muzzle();
			RefCountedReferencerClass *owner_ref = new RefCountedReferencerClass;
			owner_ref->Set_Ptr( Get_Owner() );
			AudibleSoundClass * sound = WWAudioClass::Get_Instance()->Create_Sound( Definition->ReloadSoundDefID, owner_ref,0,2);
			if ( sound != NULL )
			{
				sound->Set_Transform( muzzle );
				sound->Attach_To_Object(Model, -1);
				sound->Add_To_Scene( true );
				sound->Release_Ref();
			}
			REF_PTR_RELEASE( owner_ref );
		}
	}
}

void WeaponClass::Set_Model( RenderObjClass *model )
{
	if (Model != NULL)
	{
		Model->Release_Ref();
	}
	Model = model;
	if (Model != NULL)
	{
		Model->Add_Ref();
		Init_Muzzle_Flash(Model);
	}
}



void WeaponClass::Set_Total_Rounds(int num)
{
	if (num == -1)
	{
		InventoryRounds = -1;
	}
	else if (InventoryRounds == -1)
	{
		ClipRounds = num;
	}
	else
	{
		num -= (int)InventoryRounds;
		ClipRounds = num;
		if (ClipRounds < 0)
		{
			InventoryRounds = InventoryRounds + ClipRounds;
			ClipRounds = 0;
		}
		if (ClipRounds > Definition->ClipSize)
		{
			InventoryRounds = InventoryRounds + (ClipRounds - Definition->ClipSize);
			ClipRounds = ClipRounds - (ClipRounds - Definition->ClipSize);
		}
		if (InventoryRounds > (int)Definition->MaxInventoryRounds)
		{
			sint32 mir = Definition->MaxInventoryRounds;
			InventoryRounds = mir;
		}
	}
}

void WeaponClass::Update()
{
	ArmedGameObj*   owner   = Get_Owner();
	SoldierGameObj* soldier = owner ? owner->As_SoldierGameObj() : 0;
	if (soldier && soldier->Get_State() == HumanStateClass::IN_VEHICLE)
	{
		IsPrimaryTriggered = false;
		IsSecondaryTriggered = false;
	}
	if (Get_Style() == WEAPON_HOLD_STYLE_C4 ||
		Get_Style() == WEAPON_HOLD_STYLE_BEACON ||
		Get_Can_Snipe() )
	{
		IsSecondaryTriggered = false;
	}
	if ( (int)PrimaryAmmoDefinition->BurstMax == 0 )
	{
		BurstCount = -1;
	}
	else
	{
		BurstDelayTimer = BurstDelayTimer - TimeManager::FrameSeconds;
		if ((!IsPrimaryTriggered && !IsSecondaryTriggered) || (BurstDelayTimer <= 0.0f))
		{
			BurstDelayTimer = PrimaryAmmoDefinition->BurstDelayTime;
			sint32 bm = PrimaryAmmoDefinition->BurstMax;
			BurstCount = bm;
		}
	}
	if (TimeManager::FrameSeconds != 0)
	{
		DidFire = false;
	}
	Update_State(TimeManager::FrameSeconds);
	if ((Get_Owner() == CombatManager::Get_The_Star()) && CombatManager::Is_First_Person())
	{
		Update_Muzzle_Flash(false, false);
	}
	else
	{
		Update_Muzzle_Flash( DidFire && (TotalRoundsFired & 1), DidFire && (~TotalRoundsFired & 1) );
	}
	if ( DidFire )
	{
		Do_Firing_Effects();
		NextAnimState = (TotalRoundsFired & 1) ? WEAPON_ANIM_FIRING_0 : WEAPON_ANIM_FIRING_1;
	}
	else
	{
		NextAnimState = WEAPON_ANIM_NOT_FIRING;
	}
	if ((IsPrimaryTriggered == false && LastFrameIsPrimaryTriggered == true) ||
		(IsSecondaryTriggered == false && LastFrameIsSecondaryTriggered == true) )
	{
		Do_Continuous_Effects(false);
	}
	else
	{
		bool emitters_on = IsPrimaryTriggered || IsSecondaryTriggered;
		if ( State == STATE_START_SWITCH ||  State == STATE_END_SWITCH || 
			State == STATE_RELOAD || Get_Clip_Rounds() == 0 )
		{
			emitters_on = false;
		}
		Do_Continuous_Effects(emitters_on);
	}
	LastFrameIsPrimaryTriggered	= IsPrimaryTriggered;
	LastFrameIsSecondaryTriggered	= IsSecondaryTriggered;
}



void WeaponClass::Update_State(float pending_time)
{
	LockTriggers = false;
	while (pending_time > 0.0f)
	{
		bool trigger_ok = true;
		if ( Get_Style() == WEAPON_HOLD_STYLE_BEACON && 
			Get_Owner() && Get_Owner()->As_SoldierGameObj() && 
			!Get_Owner()->As_SoldierGameObj()->Is_Upright() )
		{
			trigger_ok = false;
		}
		if ( (State == STATE_READY) || (State == STATE_IDLE) )
		{
			if ( (IsPrimaryTriggered || IsSecondaryTriggered) && (BurstCount != 0) && CombatManager::Is_Gameplay_Permitted() && trigger_ok )
			{
				if ( SafetySet )
				{
					LockTriggers = true;
				}
				else
				{
					if ( Is_Loaded() )
					{
						Set_State( STATE_CHARGE );
					}
					else
					{
						if (Definition->EmptySoundDefID != 0)
						{
							EmptySoundTimer -= TimeManager::FrameSeconds;
							if (EmptySoundTimer <= 0)
							{
								EmptySoundTimer = 0.3f;
								Matrix3D	muzzle = Get_Muzzle();
								RefCountedReferencerClass *owner_ref = new RefCountedReferencerClass;
								owner_ref->Set_Ptr( Get_Owner() );
								AudibleSoundClass * sound = WWAudioClass::Get_Instance()->Create_Sound( Definition->EmptySoundDefID, owner_ref,0,2);
								if ( sound != NULL )
								{
									sound->Set_Transform( muzzle );
									sound->Attach_To_Object(Model,-1);
									sound->Add_To_Scene( true );
									sound->Release_Ref();
								}
								REF_PTR_RELEASE( owner_ref );
							}
						}
					}
				}
			}
			if ( Is_Reload_Needed() )
			{
				Force_Reload();
			}
		}
		float	useable_time = min( StateTimer, pending_time );
		StateTimer -= useable_time;
		pending_time -= useable_time;
		if ( StateTimer <= 0 )
		{
			switch ( State )
			{
				case STATE_IDLE:
					StateTimer = 0;
					pending_time = 0;
					break;
				case STATE_READY:
					Set_State( STATE_IDLE );
					break;
				case STATE_CHARGE:
					if ( IsPrimaryTriggered || !IsSecondaryTriggered )
					{
						Set_State( STATE_FIRE_PRIMARY );
					}
					else
					{
						Set_State( STATE_FIRE_SECONDARY );
					}
					if ( IsPrimaryTriggered  || IsSecondaryTriggered )
					{
						if ( Is_Muzzle_Clear() )
						{
							Do_Fire( IsPrimaryTriggered );
							DidFire = true;
						}
					}
					break;
				case STATE_FIRE_PRIMARY:
				case STATE_FIRE_SECONDARY:
					Set_State( STATE_READY );
					break;
				case STATE_RELOAD:
					Do_Reload();
					Set_State( STATE_READY );
					break;
				case STATE_START_SWITCH:
					if ( UpdateModel == WEAPON_MODEL_UPDATE_WILL_BE_NEEDED )
					{
						UpdateModel = WEAPON_MODEL_UPDATE_IS_NEEDED;
					}
					Set_State( STATE_END_SWITCH );
					break;
				case STATE_END_SWITCH:
					Set_State( STATE_IDLE );
					break;
			}
		}
	}
}

RENEGADE_FUNCTION
void WeaponClass::Do_Firing_Effects()
AT2(0x00700020,0x006FF5E0);

REF_DEF2(_TheEjectCasingObserver,PhysObserverClass,0x0085E27C,0x0085D454);
void WeaponClass::Make_Shell_Eject( const Matrix3D & tm )
{
	DefinitionClass * def = DefinitionMgrClass::Find_Definition( Definition->EjectPhysDefID, true);
	if ((def != NULL) && ((PhysDefClass *)def)->Is_Type( "ProjectileDef" ))
	{
		ProjectileClass * PhysicalObject = (ProjectileClass *)def->Create();
		PhysicalObject->Set_Transform( tm );
		PhysicalObject->Set_Observer(&_TheEjectCasingObserver);
		PhysicalObject->Set_Collision_Group( DEFAULT_COLLISION_GROUP );
		PhysicalObject->Set_Velocity( tm.Rotate_Vector( Vector3( 2,0,0 ) ) );
		COMBAT_SCENE->Add_Dynamic_Object( PhysicalObject );
		PhysicalObject->Release_Ref();
	}
}

const Matrix3D & WeaponClass::Get_Muzzle( int index )
{
	if ( Get_Owner() == COMBAT_STAR && CombatManager::Is_First_Person() )
	{
		static Matrix3D _muzzle;
		_muzzle.Obj_Look_At( COMBAT_CAMERA->Get_Transform().Get_Translation(), Get_Owner()->Get_Targeting_Pos(), 0 );
		return _muzzle;
	}
	return Get_Owner()->Get_Muzzle( index );
}


bool WeaponClass::Is_Muzzle_Clear()
{
	int primary = 1;
	int muzzle_index = Get_Total_Rounds_Fired() & 1 + ( primary ? 0 : 2 );
	Matrix3D muzzle = Get_Muzzle( muzzle_index );
	Vector3 start_pt;
	Vector3 end_pt;
	muzzle.Get_Translation( &end_pt );
	if ( Get_Owner() == NULL )
	{
		return true;
	}
	if ((Get_Owner() == COMBAT_STAR) && CombatManager::Is_First_Person())
	{
		return true;
	}
	if (Get_Owner()->As_SoldierGameObj() != NULL)
	{
		start_pt = Get_Owner()->Get_Bullseye_Position();
	}
	else
	{
		Vector3 owner_pos;
		Get_Owner()->Get_Position( &owner_pos );
		owner_pos -= end_pt;
		owner_pos.Z = 0.0f;
		float dist = owner_pos.Quick_Length();
		Vector3 offset;
		Matrix3D::Rotate_Vector(muzzle,Vector3(-dist,0.0f,0.0f),&offset);
		start_pt = end_pt + offset;
	}
	LineSegClass ray( start_pt, end_pt );
	CastResultStruct res;
	PhysRayCollisionTestClass raytest(ray,&res,BULLET_COLLISION_GROUP,COLLISION_TYPE_PROJECTILE);
	raytest.CheckDynamicObjs = false;
	{ 
		COMBAT_SCENE->Cast_Ray( raytest );
	}
	return raytest.Result->Fraction == 1.0f;
}

void	WeaponClass::Do_Fire( bool primary )
{
	if (	Get_Owner() && 
			Get_Owner()->As_SoldierGameObj() && 
			Get_Owner()->As_SoldierGameObj()->Get_Player_Data() ) {
		Get_Owner()->As_SoldierGameObj()->Get_Player_Data()->Stats_Add_Shot_Fired();

		Get_Owner()->As_SoldierGameObj()->Get_Player_Data()->Stats_Add_Weapon_Fired( 
			this->Get_Definition()->Get_ID() );

	}


	const AmmoDefinitionClass *ammo_def;
	if ( primary ) {
		ammo_def = PrimaryAmmoDefinition;
	} else {
		ammo_def = SecondaryAmmoDefinition;
	}


	if ( Get_Style() == WEAPON_HOLD_STYLE_BEACON ) {

		if ( Fire_Beacon( ammo_def ) ) {

			BurstCount = BurstCount - 1;
			Decrement_Rounds( ammo_def->SprayBulletCost );
			Set_Clip_Rounds_Client(ClipRounds);
		}

	} else {

		BurstCount = BurstCount - 1;
		Decrement_Rounds( ammo_def->SprayBulletCost );

		if ( Get_Style() == WEAPON_HOLD_STYLE_C4 ) {
			if ( CombatManager::I_Am_Server() ) {
				Fire_C4( ammo_def );
			}
		} else {
			Fire_Bullet( ammo_def, primary );
		}
	}

	return ;
}

void WeaponClass::Do_Reload()
{
	int added = (int)Definition->ClipSize - (int)ClipRounds;
	bool apply_cheat = false;
	if ( apply_cheat == false && InventoryRounds >= 0 )
	{
		if ( InventoryRounds < added )
		{
			added = InventoryRounds;
		}
		InventoryRounds = InventoryRounds - added;
	}
	ClipRounds = ClipRounds + added;
}

void WeaponClass::Set_Owner( ArmedGameObj *owner )
{
	Owner = owner;
}

void WeaponClass::Add_Rounds_Client(int num)
{
	Add_Rounds(num);
	(new WeaponEvent)->Init(ClipRounds,InventoryRounds,this);
}

void WeaponClass::Add_Rounds(int num)
{
	if ( ClipRounds == 0 )
	{
		if ( num < 0 )
		{
			sint32 csz = Definition->ClipSize;
			ClipRounds = csz;
		}
		else if ( InventoryRounds >= 0 )
		{
			int count = MIN(num, (int)Definition->ClipSize);
			num -= count;
			ClipRounds = count;
		}
	}
	if (num < 0)
	{
		InventoryRounds = -1;
	}
	else if (InventoryRounds >= 0)
	{
		InventoryRounds = InventoryRounds + num;
	}
	if ((InventoryRounds != -1) && (InventoryRounds > (int)Definition->MaxInventoryRounds))
	{
		sint32 mir = Definition->MaxInventoryRounds;
		InventoryRounds = mir;
	}
}

bool WeaponClass::Is_Ammo_Maxed( void )
{
	if (InventoryRounds == -1) return true;
	if (((int)Definition->MaxInventoryRounds == 0) && (ClipRounds == 0)) return false;
	return (InventoryRounds == (int)Definition->MaxInventoryRounds);
}

bool
WeaponClass::Is_Reload_OK( void )
{
	bool retval = (InventoryRounds != 0);
	return retval;
}

void	WeaponClass::Fire_C4( const AmmoDefinitionClass *ammo_def )
{
	#define		C4_OBJECT_NAME				"Tossed C4"
	C4GameObj *c4 = (C4GameObj *)ObjectLibraryManager::Create_Object( C4_OBJECT_NAME );
	if ( c4 ) {
		c4->Init_C4( ammo_def, Get_Owner()->As_SoldierGameObj(), C4DetonationMode, Get_Muzzle() );
	}
}

bool	WeaponClass::Fire_Beacon( const AmmoDefinitionClass *ammo_def )
{
	bool retval = false;
	if ( CombatManager::I_Am_Server() ) {
		BeaconGameObj *beacon = (BeaconGameObj *)ObjectLibraryManager::Create_Object( ammo_def->BeaconDefID );
		if ( beacon != NULL ) {
			Vector3 pos;
			Get_Owner()->Get_Position( &pos );
			beacon->Init_Beacon( Definition, Get_Owner()->As_SoldierGameObj(), pos );
			beacon->Start_Observers ();
			if ( beacon->Can_Place_Here ( pos ) ) {
				beacon->Begin_Arming();
				retval = true;
			} else {
				beacon->Set_Delete_Pending ();
			}
		}
	}
	return retval;
}

RENEGADE_FUNCTION
void WeaponClass::Fire_Bullet(const AmmoDefinitionClass *ammo_def, bool primary)
AT2(0x006FE840,0x006FDE00);

void	WeaponClass::Decrement_Rounds( int rounds )
{
	if ( ClipRounds != -1 ) {
		ClipRounds = ClipRounds - rounds;
		if ( ClipRounds <= 0 ) {
			ClipRounds = 0;
		}
	}
}

void WeaponClass::Set_Clip_Rounds(int rounds)
{
	ClipRounds = rounds;
}
void WeaponClass::Set_Inventory_Rounds( int rounds )
{
	InventoryRounds = rounds;
}
void WeaponClass::Set_Clip_Rounds_Client(int rounds)
{
	ClipRounds = rounds;
	(new WeaponEvent)->Init(ClipRounds,InventoryRounds,this);
}
void WeaponClass::Set_Inventory_Rounds_Client( int rounds )
{
	InventoryRounds = rounds;
	(new WeaponEvent)->Init(ClipRounds,InventoryRounds,this);
}
