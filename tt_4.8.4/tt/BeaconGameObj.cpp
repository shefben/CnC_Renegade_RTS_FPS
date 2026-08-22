#include "General.h"
#include "BeaconGameObj.h"
#include "BeaconGameObjDef.h"
#include "SoldierGameObj.h"
#include "AudibleSoundClass.h"
#include "TimeManager.h"
#include "CombatManager.h"
#include "HUDInfo.h"
#include "BackgroundMgrClass.h"
#include "WeatherMgrClass.h"
#include "CollisionMath.h"
#include "ObjectLibraryManager.h"
#include "WWAudioClass.h"
#include "BitStream.h"
#include "BaseControllerClass.h"
#include "Engine_Math.h"
#include "GameObjManager.h"
#include "WeaponMgr.h"
#include "Iterator.h"
#include "ExplosionManager.h"
#include "DefinitionMgrClass.h"
#include "BuildingGameObj.h"
#include "OffenseObjectClass.h"
bool NukeWeatherDisable = false;
bool IonWeatherDisable = false;
RENEGADE_FUNCTION
const PersistFactoryClass& BeaconGameObj::Get_Factory() const
AT2(0x00709600, 0x00708BC0);



RENEGADE_FUNCTION
void BeaconGameObj::Init()
AT2(0x00709610, 0x00708BD0);



RENEGADE_FUNCTION
void BeaconGameObj::Init(const BeaconGameObjDef&)
AT2(0x00709640, 0x00708C00);



void BeaconGameObj::Init_Beacon(const WeaponDefinitionClass *weapon, SoldierGameObj *soldier, const Vector3 &pos)
{
	WeaponDef = weapon;
	Owner = soldier;
	Set_Position(pos);
	if (soldier)
	{
		Player = soldier->Get_Player_Data();
	}
	if (Owner->As_SmartGameObj())
	{
		this->Set_Player_Type(Owner->As_SmartGameObj()->Get_Player_Type());
	}
}

RENEGADE_FUNCTION
bool BeaconGameObj::Save(ChunkSaveClass&)
AT2(0x00709700, 0x00708CC0);



RENEGADE_FUNCTION
bool BeaconGameObj::Load(ChunkLoadClass&)
AT2(0x00709850, 0x00708E10);



RENEGADE_FUNCTION
bool BeaconGameObj::Load_Variables(ChunkLoadClass&)
AT2(0x00709960, 0x00708F20);



RENEGADE_FUNCTION
void BeaconGameObj::Think()
AT2(0x007099F0, 0x00708FB0);



RENEGADE_FUNCTION
void BeaconGameObj::Get_Information(StringClass&)
AT2(0x00709B90, 0x00709150);



void BeaconGameObj::Start_Cinematic(int number)
{
	PhysicalGameObj *obj = (PhysicalGameObj *)ObjectLibraryManager::Create_Object(number);
	if (obj)
	{
		obj->Start_Observers();
		Vector3 v;
		Get_Position(&v);
		obj->Set_Position(v);
		CinematicObj = obj;
	}
}

void BeaconGameObj::Stop_Armed_Sound()
{
	if (ArmedSound)
	{
		ArmedSound->Remove_From_Scene();
		REF_PTR_RELEASE(ArmedSound);
	}
}

void BeaconGameObj::Set_State(int state)
{
	if (State != state)
	{
		Restore_Owner();
		if (CombatManager::I_Am_Server())
		{
			Set_Object_Dirty_Bit(DB_RARE,true);
		}
		bool nuke = Get_Definition().IsNuke != 0;
		switch (state)
		{
		case STATE_0:
			Stop_Armed_Sound();
			Display_Message(Get_Definition().ArmingInterruptedTextID);
			break;
		case STATE_DEPLOYING:
			ArmTime = Get_Definition().ArmTime;
			Display_Message(Get_Definition().ArmingTextID);
			Start_Owner_Animation();
			break;
		case STATE_DEPLOYED:
			{
				if (!PreDetonateCinematicActive)
				{
					Stop_Owner_Animation();
					DetonateTime = Get_Definition().DetonateTime;
					BroadcastToAllTime = Get_Definition().BroadcastToAllTime;
					ArmedSound = WWAudioClass::Get_Instance()->Create_Continuous_Sound(Get_Definition().ArmedSoundDefID,0,0,2);
					if (ArmedSound)
					{
						ArmedSound->Set_Transform(Get_Transform());
						ArmedSound->Add_To_Scene(true);
					}
					if (CombatManager::I_Am_Server())
					{
						if (nuke)
						{
							if (!NukeWeatherDisable)
							{
								BackgroundMgrClass::Override_Sky_Tint(0.80000001f,DetonateTime * 0.5f);
								WeatherMgrClass::Override_Wind(0,3.0,1.0,DetonateTime * 0.5f);
							}
						}
						else
						{
							if (!IonWeatherDisable)
							{
								BackgroundMgrClass::Override_Clouds(1.0,1.0,DetonateTime * 0.5f);
								BackgroundMgrClass::Override_Lightning(0.80000001f,0.2f,0.80000001f,0,1.0,DetonateTime * 0.5f);
								WeatherMgrClass::Override_Precipitation(WeatherMgrClass::PRECIPITATION_RAIN,2.0,DetonateTime * 0.5f);
							}
						}
					}
					BaseControllerClass *base = BaseControllerClass::Find_Base(Get_Player_Type());
					if (base)
					{
						base->On_Beacon_Armed(this);
					}
					float f = Get_Definition().PreDetonateCinematicDelay;
					if (f < 0.001f)
					{
						f = 0.001f;
					}
					PreDetonateCinematicDelay = f;
					PreDetonateCinematicActive = true;
				}
			}
			break;
		case STATE_DISARMED:
			{
				Display_Message(Get_Definition().DisarmedTextID);
				Stop_Armed_Sound();
				Stop_Owner_Animation();
				if (CombatManager::I_Am_Server())
				{
					if (nuke)
					{
						if (!NukeWeatherDisable)
						{
							BackgroundMgrClass::Restore_Sky_Tint(5);
							WeatherMgrClass::Restore_Wind(5);
						}
					}
					else
					{
						if (!IonWeatherDisable)
						{
							BackgroundMgrClass::Restore_Clouds(5);
							BackgroundMgrClass::Restore_Lightning(5);
							WeatherMgrClass::Restore_Precipitation(5);
						}
					}
				}
				BaseControllerClass *base = BaseControllerClass::Find_Base(Get_Player_Type());
				if (base)
				{
					base->On_Beacon_Disarmed(this);
				}
				if (CinematicObj)
				{
					CinematicObj->Set_Delete_Pending();
					CinematicObj = 0;
				}
				Set_Delete_Pending();
			}
			break;
		case STATE_DETONATED:
			{
				Stop_Armed_Sound();
				PreDetonateCinematicActive = 0;
				ArmTime = Get_Definition().PostDetonateTime;
				if (CombatManager::I_Am_Server())
				{
					Start_Cinematic(Get_Definition().PostDetonateCinematicObj);
					if (nuke)
					{
						if (!NukeWeatherDisable)
						{
							WeatherMgrClass::Override_Precipitation(WeatherMgrClass::PRECIPITATION_ASH,0.30000001f,0);
						}
					}
				}
				if (Get_Definition().ExplosionObj)
				{
					Create_Explosion();
				}
				RenderObjClass *robj = Peek_Physical_Object()->Peek_Model();
				if (robj)
				{
					robj->Set_Hidden(true);
				}
			}
			break;
		}
		State = state;
	}
}

void BeaconGameObj::Update_State()
{
	ArmTime -= TimeManager::FrameSeconds;
	
	if (PreDetonateCinematicActive)
	{
		DetonateTime -= TimeManager::FrameSeconds;
		
		if (ArmedSound)
			ArmedSound->Set_Pitch_Factor(lerp(6.f, 1.f, DetonateTime / Get_Definition().DetonateTime));
		
		if (DetonateTime <= 0)
			Set_State(STATE_DETONATED);
		
		if (PreDetonateCinematicDelay != 0)
		{
			PreDetonateCinematicDelay -= TimeManager::FrameSeconds;
			
			if (PreDetonateCinematicDelay <= 0)
			{
				PreDetonateCinematicDelay = 0;
				
				if (CombatManager::I_Am_Server())
					Start_Cinematic(Get_Definition().PreDetonateCinematicObj);
			}
		}
	}
	
	switch (State)
	{
	
	case STATE_0:
	case STATE_DISARMED:
		break;
		
	case STATE_DEPLOYING:
		if (Owner == COMBAT_STAR)
			HUDInfo::ActionStatusValue = lerp(1.f, 0.f, ArmTime / Get_Definition().ArmTime);
		
		if (ArmTime <= 0)
			Set_State(STATE_DEPLOYED);
		
		break;
		
	case STATE_DEPLOYED:
		if (BroadcastToAllTime != 0)
		{
			BroadcastToAllTime -= TimeManager::FrameSeconds;
			
			if (BroadcastToAllTime <= 0)
			{
				BroadcastToAllTime = 0;
				
				BaseControllerClass* baseController = BaseControllerClass::Find_Base(Get_Player_Type());
				if (baseController)
					baseController->On_Beacon_Warning(this);
			}
		}
		break;
		
	case STATE_DETONATED:
		if (ArmTime <= 0)
		{
			if (CombatManager::I_Am_Server())
			{
				if (CombatManager::Does_Beacon_Placement_Ends_Game() && Is_In_Enemy_Base())
				{
					BaseControllerClass* enemyBase = Get_Enemy_Base();
					if (enemyBase)
					{
						enemyBase->Destroy_Base();
						enemyBase->Set_Beacon_Destroyed_Base(true);
					}
				}
				
				if (Get_Definition().IsNuke)
				{
					if (!NukeWeatherDisable)
					{
						BackgroundMgrClass::Restore_Sky_Tint(5.f);
						WeatherMgrClass::Restore_Wind(5.f);
						WeatherMgrClass::Restore_Precipitation(5.f);
					}
				}
				else
				{
					if (!IonWeatherDisable)
					{
						BackgroundMgrClass::Restore_Clouds(5.f);
						BackgroundMgrClass::Restore_Lightning(5.f);
						WeatherMgrClass::Restore_Precipitation(5.f);
					}
				}
				
			}
			
			Set_Delete_Pending();
		}
		break;
	}
}



BaseControllerClass* BeaconGameObj::Get_Enemy_Base()
{
	return BaseControllerClass::Find_Base(Get_Player_Type() == 0 ? 1 : 0);
}



bool BeaconGameObj::Is_In_Enemy_Base()
{
	BaseControllerClass* enemyBase = Get_Enemy_Base();
	if (enemyBase)
	{
		Vector3 position;
		Get_Position(&position);
		return CollisionMath::Overlap_Test(enemyBase->Get_Beacon_Zone(), position) != CollisionMath::OUTSIDE;
	}
	return false;
}



void BeaconGameObj::Stop_Current_Message_Sound()
{
	if (CurrentMessageSound)
	{
		CurrentMessageSound->Remove_From_Scene();
		REF_PTR_RELEASE(CurrentMessageSound);
	}
}



RENEGADE_FUNCTION
void BeaconGameObj::Display_Message(int)
AT2(0x0070A570, 0x00709B30);



void BeaconGameObj::Begin_Arming()
{
	Set_State(STATE_DEPLOYING);
}

void BeaconGameObj::Start_Owner_Animation()
{
	Restore_Owner();
	if (Get_Owner())
	{
		Get_Owner()->Set_Animation(Get_Definition().ArmingAnimationName);
	}
	if (Owner == COMBAT_STAR)
	{
		HUDInfo::DisplayActionBar = true;
	}
}


RENEGADE_FUNCTION
void BeaconGameObj::Stop_Owner_Animation()
AT2(0x0070A850, 0x00709E10);



RENEGADE_FUNCTION
bool BeaconGameObj::Was_Owner_Interrupted()
AT2(0x0070A950, 0x00709F10);



void BeaconGameObj::Completely_Damaged(const OffenseObjectClass&)
{
	if (!Is_Delete_Pending() && State != STATE_DETONATED)
	{
		Set_State(STATE_DISARMED);
		Set_Delete_Pending();
	}
}


extern GameObject *ExplosionObj;
void BeaconGameObj::Create_Explosion()
{
	if (CombatManager::I_Am_Server())
	{
		Restore_Owner();
	}
	Vector3 v;
	Get_Position(&v);
	if (Get_Owner())
	{
		Vector3 v2 = Vector3(0,0,-1);
		ExplosionObj = this;
		ExplosionManager::Create_Explosion_At(Get_Definition().ExplosionObj,Get_Transform(),Get_Owner(),v2,0);
		ExplosionObj = 0;
		if (CombatManager::I_Am_Server())
		{
			ExplosionDefinitionClass *explosion = (ExplosionDefinitionClass *)DefinitionMgrClass::Find_Definition(Get_Definition().ExplosionObj,true);
			float damageradius = explosion->DamageRadius;
			float damageradiussquared = damageradius * damageradius;
			for (Iterator<SList<BuildingGameObj>> object((SList<BuildingGameObj>&)GameObjManager::BuildingGameObjList); object; ++object)
			{
				float poly = 0;
				object->Find_Closest_Poly(v,&poly);
				if (poly < damageradiussquared)
				{
					OffenseObjectClass offense;
					float dist = WWMath::Sqrt(poly);
					offense.ForceServerDamage = false;
					offense.EnableClientDamage = false;
					offense.Set_Warhead(explosion->Warhead);
					offense.Set_Damage((1 - WWMath::Clamp(dist / damageradius)) * explosion->DamageStrength);
					offense.Set_Owner(Get_Owner());
					ExplosionObj = this;
					object->Apply_Damage_Building(offense,true);
					ExplosionObj = 0;
				}
			}
		}
	}
}

void BeaconGameObj::Export_Rare(BitStreamClass& stream)
{
	Restore_Owner();
	PhysicalGameObj::Export_Rare(stream);
	stream.Add(State);
	int owner = 0;
	if (Get_Owner())
	{
		owner = Get_Owner()->Get_Network_ID();
	}
	stream.Add(owner);
}

void BeaconGameObj::Import_Rare(BitStreamClass& stream)
{
	PhysicalGameObj::Import_Rare(stream);
	int state;
	stream.Get(state);
	int owner;
	stream.Get(owner);
	if (owner)
	{
		Owner = GameObjManager::Find_SmartGameObj(owner);
	}
	Set_State(state);
}

RENEGADE_FUNCTION
void BeaconGameObj::Restore_Owner()
AT2(0x0070AEE0, 0x0070A4A0);

bool BeaconGameObj::Can_Place_Here(const Vector3& position)
{
	return true;
}

const BeaconGameObjDef & BeaconGameObj::Get_Definition( void ) const
{
	return (const BeaconGameObjDef &)*definition;
}
