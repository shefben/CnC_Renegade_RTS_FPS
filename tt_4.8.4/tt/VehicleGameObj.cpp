#include "General.h"
#include "VehicleGameObj.h"

#include "bitstream.h"
#include "VehicleGameObjDef.h"
#include "Quaternion.h"
#include "VehiclePhysClass.h"
#include "CombatManager.h"
#include "SoldierGameObj.h"
#include "RenderObjClass.h"
#include "TimeManager.h"
#include "GameObjManager.h"
#include "cGameType.h"
#include "AudibleSoundClass.h"
#include "DefinitionMgrClass.h"
#include "engine2.h"
#include "encyclopedia.h"
#include "OffenseObjectClass.h"
#include "PlayerDataClass.h"
#include "engine_math.h"
#include "WeaponClass.h"
#include "WeaponBagClass.h"
#include "cNetwork.h"
#include "cRemoteHost.h"

REF_DEF2(VehicleGameObj::DefaultDriverIsGunner, bool, 0x00856E58, 0x00856040);



RENEGADE_FUNCTION
void VehicleGameObj::Aquire_Turret_Bones()
   AT2(0x00679140,0x006789E0);

RENEGADE_FUNCTION
void VehicleGameObj::Release_Turret_Bones()
   AT2(0x006792F0,0x00678B90);

RENEGADE_FUNCTION
void VehicleGameObj::Init_Wheel_Effects()
   AT2(0x0067AC30,0x0067A4D0);

RENEGADE_FUNCTION
void VehicleGameObj::Shutdown_Wheel_Effects()
   AT2(0x0067ACB0,0x0067A550);

void	Set_Subobject_Visibility (RenderObjClass *model, int bone_index, bool show)
{
	//
	//	Loop over all the subobjects that are attached to this bone
	//
	int count = model->Get_Num_Sub_Objects_On_Bone( bone_index );
	for (int index = 0; index < count; index ++) {
		RenderObjClass *sub_obj = model->Get_Sub_Object_On_Bone(index, bone_index);
		if (sub_obj != NULL) {

			//
			//	Show or hide this subobject
			//
			sub_obj->Set_Hidden (!show);
			sub_obj->Release_Ref();
		}
	}

	return ;
}

void	VehicleGameObj::Apply_Damage( const OffenseObjectClass & damager, float scale, int alternate_skin )
{
	float starting_health = Defense.Get_Health();

	//
	//	Let the base class actually handle the call
	//
	SmartGameObj::Apply_Damage( damager, scale, alternate_skin );

	// Stats
	if ( starting_health > 0 && Defense.Get_Health() <= 0 ) {
		if ( damager.Get_Owner() && damager.Get_Owner()->As_SoldierGameObj() != NULL ) {
			if ( damager.Get_Owner()->As_SoldierGameObj()->Get_Player_Data() != NULL ) {
				damager.Get_Owner()->As_SoldierGameObj()->Get_Player_Data()->VehiclesDestroyed++;
			}
		}
	}

	//
	//	Now make sure the model is in a state to match its damage level
	//
	Update_Damage_Meshes();
}

void	VehicleGameObj::Update_Damage_Meshes( void )
{
	//
	//	Calculate what percent health we have after this damage
	//
	SCRIPTS_DamageMeshesUpdate = false;
	DefenseObjectClass *defense_object = &Defense;
	if ( defense_object != NULL ) {
		float ending_health = defense_object->Get_Health() / defense_object->Get_Health_Max();

		//
		//	Make sure we have a render object to lookup bones on
		//
		RenderObjClass *model = Peek_Physical_Object()->Peek_Model();
		if (model != NULL) {
			bool show_damage25 = false;
			bool show_damage50 = false;
			bool show_damage75 = false;

			//
			//	Determine what the visibility state of these bones should be
			//
			if (ending_health < 0.25F) {
				show_damage25 = true;
				show_damage50 = true;
				show_damage75 = true;
			} else if (ending_health < 0.5F) {
				show_damage25 = true;
				show_damage50 = true;
			} else if (ending_health < 0.75F) {
				show_damage25 = true;
			}

			static const char *	DAMAGE25_BONE_NAME		= "DAMAGE25";
			static const char *	DAMAGE50_BONE_NAME		= "DAMAGE50";
			static const char *	DAMAGE75_BONE_NAME		= "DAMAGE75";
			static int				DAMAGE25_BONE_NAME_LEN	= ::strlen( DAMAGE25_BONE_NAME );
			static int				DAMAGE50_BONE_NAME_LEN	= ::strlen( DAMAGE50_BONE_NAME );
			static int				DAMAGE75_BONE_NAME_LEN	= ::strlen( DAMAGE75_BONE_NAME );

			//
			//	Loop over all the bones in the model, showing and hiding any that represent
			// damage levels
			//
			int bone_count = model->Get_Num_Bones();
			for (	int index = 0; index < bone_count; index ++) {
				const char *bone_name = model->Get_Bone_Name( index );

				if ( _strnicmp( bone_name, DAMAGE25_BONE_NAME, DAMAGE25_BONE_NAME_LEN ) == 0 ) {

					//
					//	Show/hide the subobjects associated with 25% damage
					//
					::Set_Subobject_Visibility( model, index, show_damage25 );
				} else if ( _strnicmp( bone_name, DAMAGE50_BONE_NAME, DAMAGE50_BONE_NAME_LEN ) == 0 ) {

					//
					//	Show/hide the subobjects associated with 50% damage
					//
					::Set_Subobject_Visibility( model, index, show_damage50 );
				} else if ( _strnicmp( bone_name, DAMAGE75_BONE_NAME, DAMAGE75_BONE_NAME_LEN ) == 0 ) {

					//
					//	Show/hide the subobjects associated with 75% damage
					//
					::Set_Subobject_Visibility( model, index, show_damage75 );
				}
			}
		}
	}

	return ;
}

void VehicleGameObj::Think( void )
{
	Apply_Control();
	Update_Transitions();
	if (cGameType::GameType == 3)
	{
		Set_Targeting(TargetVector, true);
	}
	SmartGameObj::Think();
	Update_Sound_Effects();
	Update_Wheel_Effects();
	if (LockTimer > 0.0f) {
		LockTimer -= TimeManager::FrameSeconds;
	} else {
		LockOwner = NULL;
	}
	if (stealthEffect != NULL) {
		if ((Get_Occupant_Count() == 0 ) && ( !IS_MISSION )) {
			stealthEffect->Enable_Stealth(false);
		}
	}
}

void VehicleGameObj::Update_Turret(float weapon_turn, float weapon_tilt)
{
	if ( TurretBone != 0 )
	{
		Matrix3D	facing(1);
		facing.Rotate_Z( weapon_turn );
		if ( BarrelBone == 0 )
		{
			facing.Rotate_Y( -weapon_tilt );
		}
		Peek_Physical_Object()->Peek_Model()->Control_Bone( TurretBone, facing, false);
	}
	if ( BarrelBone != 0 )
	{
		Matrix3D	facing(1);
		facing.Rotate_Y( -weapon_tilt );
		Peek_Physical_Object()->Peek_Model()->Control_Bone( BarrelBone, facing, false);
	}
}



bool VehicleGameObj::Set_Targeting(const Vector3& target_pos, bool do_tilt)
{
	bool	ready = true;
	ArmedGameObj::Set_Targeting(target_pos, true);
	float relative_turn = 0;
	float relative_tilt = 0;
	if ( TurretBone != 0 )
	{
		Matrix3D	turret_base = Peek_Physical_Object()->Peek_Model()->Get_Bone_Transform( TurretBone );
		Vector3	turret_space_target;
		Matrix3D::Inverse_Transform_Vector( turret_base, target_pos, &turret_space_target );

		relative_turn = WWMath::Atan2( turret_space_target.Y, turret_space_target.X );
		if ( BarrelOffset )
		{
			turret_space_target.Z = 0;
			float barrel_offset_angle = WWMath::Atan2( BarrelOffset, turret_space_target.Length() );
			relative_turn -= barrel_offset_angle;
		}
	}
	if ( WWMath::Fabs( relative_turn ) < DEG_TO_RAD( 80 ) )
	{
		if ( BarrelBone != 0 )
		{
			Matrix3D	barrel_base = Peek_Physical_Object()->Peek_Model()->Get_Bone_Transform( BarrelBone );
			Vector3	barrel_space_target;
			Matrix3D::Inverse_Transform_Vector( barrel_base, target_pos, &barrel_space_target );
			
			float dist = barrel_space_target.Length();
			if ( dist )
			{
				relative_tilt = WWMath::Fast_Asin( barrel_space_target.Z / dist );
			}
		}
	}
	float max_move;
	max_move = Get_Definition().WeaponTurnRate * TimeManager::FrameSeconds;
	if ( Get_Definition().WeaponTurnRate < DEG_TO_RAD(1000) )
	{
		TurretTurn += WWMath::Clamp( relative_turn, -max_move, max_move );
		if ( WWMath::Fabs( relative_turn ) > WWMath::Fabs( max_move ) )
		{
			ready = false;
		}
	}
	else
	{
		TurretTurn += relative_turn;
	}
	TurretTurn = WWMath::Clamp( TurretTurn, Get_Definition().WeaponTurnMin, Get_Definition().WeaponTurnMax );
	max_move = Get_Definition().WeaponTiltRate * TimeManager::FrameSeconds;
	if ( do_tilt )
	{
		if ( Get_Definition().WeaponTiltRate < DEG_TO_RAD(1000) )
		{
			BarrelTilt += WWMath::Clamp( relative_tilt, -max_move, max_move );
			if ( WWMath::Fabs( relative_tilt ) > WWMath::Fabs( max_move ) )
			{
				ready = false;
			}
		}
		else
		{
			BarrelTilt += relative_tilt;
		}
	}
	BarrelTilt = WWMath::Clamp( BarrelTilt, Get_Definition().WeaponTiltMin, Get_Definition().WeaponTiltMax );
	Update_Turret( TurretTurn, BarrelTilt );
	if ( Get_Definition().WeaponTurnRate > DEG_TO_RAD(1000) &&
		( WWMath::Fabs(relative_turn) >= DEG_TO_RAD( 2 ) || WWMath::Fabs(relative_tilt) > DEG_TO_RAD( 2 ) ) )
	{
		static int calls = 0;
		if ( calls < 3 )
		{
			calls++;
			Set_Targeting( target_pos, do_tilt );
			calls--;
		}
		ready = true;
	}
	return ready;
}



void VehicleGameObj::Export_Creation(BitStreamClass& bitStream)
{
	SmartGameObj::Export_Creation(bitStream);
	if (LockOwner.Get_Ptr())
	{
		bitStream.Add(LockOwner.Get_Ptr()->Get_Network_ID());
		bitStream.Add(LockTimer,BITPACK_VEHICLE_LOCK_TIMER);
	}
	else
	{
		bitStream.Add(0);
	}
}



void VehicleGameObj::Import_Creation(BitStreamClass& bitStream)
{
	SmartGameObj::Import_Creation(bitStream);
	int lockid;
	bitStream.Get(lockid);
	if (lockid)
	{
		LockOwner = GameObjManager::Find_PhysicalGameObj(lockid);
		bitStream.Get(LockTimer,BITPACK_VEHICLE_LOCK_TIMER);
	}
}



void VehicleGameObj::Export_Frequent(BitStreamClass& bitStream)
{
	Apply_Control();

	WeaponClass* weapon = Get_Weapon();
	bitStream.Add(weapon ? weapon->Get_Total_Rounds() : 0);

	uint type = Get_Definition().Get_Type();
	if (type >= VEHICLE_TYPE_CAR && type <= VEHICLE_TYPE_FLYING)
	{
		VehiclePhysClass* phys = Peek_Vehicle_Phys();
		if (phys)
		{
			Vector3 position;
			Quaternion orientation;
			Vector3 velocity;
			Vector3 angularVelocity;

			phys->Get_Position(&position);
			phys->Get_Orientation(&orientation);
			phys->Get_Velocity(&velocity);
			phys->Get_Angular_Velocity(&angularVelocity);

			bitStream.Add(phys->Is_Engine_Enabled());
			bitStream.Add(position.X, BITPACK_WORLD_POSITION_X);
			bitStream.Add(position.Y, BITPACK_WORLD_POSITION_Y);
			bitStream.Add(position.Z, BITPACK_WORLD_POSITION_Z);
			bitStream.Add(orientation.X, BITPACK_VEHICLE_QUATERNION);
			bitStream.Add(orientation.Y, BITPACK_VEHICLE_QUATERNION);
			bitStream.Add(orientation.Z, BITPACK_VEHICLE_QUATERNION);
			bitStream.Add(orientation.W, BITPACK_VEHICLE_QUATERNION);
			bitStream.Add(velocity.X, BITPACK_VEHICLE_VELOCITY);
			bitStream.Add(velocity.Y, BITPACK_VEHICLE_VELOCITY);
			bitStream.Add(velocity.Z, BITPACK_VEHICLE_VELOCITY);
			bitStream.Add(angularVelocity.X, BITPACK_VEHICLE_ANGULAR_VELOCITY);
			bitStream.Add(angularVelocity.Y, BITPACK_VEHICLE_ANGULAR_VELOCITY);
			bitStream.Add(angularVelocity.Z, BITPACK_VEHICLE_ANGULAR_VELOCITY);
		}
	}

	bitStream.Add(DriverIsGunner);
	if (cNetwork::Get_Server_Rhost(cNetwork::lastUpdatedClientId)->getVersion() >= 4.0f)
	{
		bitStream.Add(SCRIPTS_DamageMeshesUpdate);
	}

	SmartGameObj::Export_Frequent(bitStream);
}



void VehicleGameObj::Import_Frequent(BitStreamClass& bitStream)
{
	uint weaponRounds;
	bitStream.Get(weaponRounds);
	
	//WeaponClass* weapon = Get_Weapon();
	//if (weapon)
		//weapon->Set_Total_Rounds(weaponRounds);
	
	uint type = Get_Definition().Get_Type();
	if (type >= VEHICLE_TYPE_CAR && type <= VEHICLE_TYPE_FLYING)
	{
		VehiclePhysClass* phys = Peek_Vehicle_Phys();
		if (phys)
		{
			bool engineEnabled;
			Vector3 position;
			Quaternion orientation;
			Vector3 velocity;
			Vector3 angularVelocity;
			
			bitStream.Get(engineEnabled);
			bitStream.Get(position.X, BITPACK_WORLD_POSITION_X);
			bitStream.Get(position.Y, BITPACK_WORLD_POSITION_Y);
			bitStream.Get(position.Z, BITPACK_WORLD_POSITION_Z);
			bitStream.Get(orientation.X, BITPACK_VEHICLE_QUATERNION);
			bitStream.Get(orientation.Y, BITPACK_VEHICLE_QUATERNION);
			bitStream.Get(orientation.Z, BITPACK_VEHICLE_QUATERNION);
			bitStream.Get(orientation.W, BITPACK_VEHICLE_QUATERNION);
			bitStream.Get(velocity.X, BITPACK_VEHICLE_VELOCITY);
			bitStream.Get(velocity.Y, BITPACK_VEHICLE_VELOCITY);
			bitStream.Get(velocity.Z, BITPACK_VEHICLE_VELOCITY);
			bitStream.Get(angularVelocity.X, BITPACK_VEHICLE_ANGULAR_VELOCITY);
			bitStream.Get(angularVelocity.Y, BITPACK_VEHICLE_ANGULAR_VELOCITY);
			bitStream.Get(angularVelocity.Z, BITPACK_VEHICLE_ANGULAR_VELOCITY);
			
			orientation.Normalize();
			
			if (CombatManager::Get_The_Star() && CombatManager::Get_The_Star()->Get_Vehicle() == this)
				phys->RigidBodyClass::Network_Latency_State_Update(position, orientation, velocity, angularVelocity);
			else
				phys->RigidBodyClass::Network_Interpolate_State_Update(position, orientation, velocity, angularVelocity, .1f);
			
			phys->Enable_Engine(engineEnabled);
		}
	}
	
	bitStream.Get(DriverIsGunner);
	if (cNetwork::Get_Client_Rhost()->getVersion() >= 4.0f)
	{
		bitStream.Get(SCRIPTS_DamageMeshesUpdate);
		if (SCRIPTS_DamageMeshesUpdate)
		{
			Update_Damage_Meshes();
		}
	}
	
	SmartGameObj::Import_Frequent(bitStream);
}



void VehicleGameObj::Set_Precision()
{
	cEncoderList::Set_Precision(BITPACK_VEHICLE_VELOCITY,
		-90.0f, 90.0f, 0.01f); // 200 mph = approx. 90 m/s
	cEncoderList::Set_Precision(BITPACK_VEHICLE_ANGULAR_VELOCITY,
		-20.0f, 20.0f, 0.01f); // to approx 6 PI
	cEncoderList::Set_Precision(BITPACK_VEHICLE_QUATERNION,
		-1.0f, 1.0f, 0.0005f); // -1 to 1
	cEncoderList::Set_Precision(BITPACK_VEHICLE_LOCK_TIMER,
		0.0f, 16.0f, 0.25f);
}

/*
** Occupants ( drivers, gunners, passengers )
*/
void	VehicleGameObj::Add_Occupant( SoldierGameObj * occupant )
{
	// Add the the lowest empty seat
	for ( int i = 0; i < Get_Definition().NumSeats; i++ ) {
		if ( SeatOccupants[i] == NULL ) {
			Add_Occupant( occupant, i );
			break;
		}
	}
}

void	VehicleGameObj::Add_Occupant( SoldierGameObj * occupant, int seat_id )
{
	TT_ASSERT( occupant );

	if ( SeatOccupants[seat_id] != NULL ) {
		return;
	}

	if ( seat_id == 0 ) {
		DriverIsGunner = DefaultDriverIsGunner;
	}

    if(OccupiedSeats == 0 && !vehicleExitReloadContinued)
    {
        Get_Weapon_Bag()->Select_Weapon_ID(Get_Definition().WeaponDefID);
    }

	SeatOccupants[seat_id] = occupant;
	OccupiedSeats++;
    Set_Object_Dirty_Bit( DB_RARE, true );

	const char * anim_name = NULL;
    if ( Get_Definition().Get_Type() == VEHICLE_TYPE_BIKE){
		anim_name = "S_A_HUMAN.H_A_V20A";
	} else {
		anim_name = "S_A_HUMAN.H_A_V10A";
	}
	occupant->Enter_Vehicle( this, anim_name );

	// possibily delete transitions
	Create_And_Destroy_Transitions();

	Enable_Engine(true);

	if ( Get_Definition().OccupantsVisible == false ) {
		occupant->Peek_Physical_Object()->Peek_Model()->Set_Hidden( true );
	}

	// Notify the observers that someone entered
	const SimpleDynVecClass<GameObjObserverClass*> & observer_list = Get_Observers();
	for( int index = 0; index < observer_list.Count(); index++ ) {
		observer_list[ index ]->Custom( this, CUSTOM_EVENT_VEHICLE_ENTERED, seat_id, occupant );
	}

	//
	//	Reveal this vehicle to the player if he was the one to enter the vehicle
	//
	if ( occupant == COMBAT_STAR ) {
		EncyclopediaMgrClass::Reveal_Object( this );
	}

	//
	// Reset any action the vehicle was performing
	//
	action.Reset(1);

	//
	// Unlock the vehicle!
	//
    LockTimer = 0.0f;
	LockOwner = NULL;
}

void VehicleGameObj::Remove_Occupant(SoldierGameObj* occupant)
{
	int seat = Find_Seat(occupant);
	if (seat >= 0)
	{
		if (seat == 0)
			Enable_Engine(false);
		
		if (occupant == Get_Actual_Gunner())
		{
			if (Get_Weapon())
			{
				Get_Weapon()->Set_Did_Fire(false);
				Get_Weapon()->Set_Primary_Triggered(false);
				Get_Weapon()->Set_Secondary_Triggered(false);              
			}
		}
		
		SeatOccupants[seat]->Exit_Vehicle();
		
		if (!Get_Definition().OccupantsVisible && SeatOccupants[seat]->Peek_Physical_Object() && SeatOccupants[seat]->Peek_Physical_Object()->Peek_Model())
			SeatOccupants[seat]->Peek_Physical_Object()->Peek_Model()->Set_Hidden(false);
		
		SeatOccupants[seat] = NULL;
		
		--OccupiedSeats;
		
		if (cGameType::GameType != 1 && OccupiedSeats == 0)
			Set_Player_Type(-2);

        if (OccupiedSeats == 0 && !vehicleExitReloadContinued)
            Get_Weapon_Bag()->Deselect();
		
		Set_Object_Dirty_Bit(DB_RARE, true);
		Create_And_Destroy_Transitions();
		
		for (int observerIndex = 0; observerIndex < Observers.Count(); observerIndex++)
			Observers[observerIndex]->Custom(this, 1000000029, seat, occupant);
	}
}



void VehicleGameObj::Enable_Engine(bool value)
{
	if (Peek_Vehicle_Phys())
      Peek_Vehicle_Phys()->Enable_Engine(value);
}



int	VehicleGameObj::Find_Seat( SoldierGameObj * occupant )
{
	for ( int i = 0; i < SeatOccupants.Length(); i++ )
	{
		if ( SeatOccupants[i] == occupant )
		{
			return i;
		}
	}
	return -1;
}



RENEGADE_FUNCTION
void VehicleGameObj::Create_And_Destroy_Transitions()
AT2(0x00678E70, 0x00678710);



void VehicleGameObj::Post_Think()
{
	RenderObjClass* model = Peek_Physical_Object()->Peek_Model();
	
	for (int seatIndex = 0; seatIndex < SeatOccupants.Length(); ++seatIndex)
	{
		if (SeatOccupants[seatIndex] &&
			SeatOccupants[seatIndex]->Get_State() != HumanStateClass::TRANSITION)
		{
			char seatBoneName[8];
			sprintf(seatBoneName, "SEAT%d", seatIndex);
			int seatBoneIndex = model->Get_Bone_Index(seatBoneName);
			if (seatBoneIndex != -1)
				SeatOccupants[seatIndex]->Set_Transform(model->Get_Bone_Transform(seatBoneIndex));
		}
	}
	
	SmartGameObj::Post_Think();
	const VehicleGameObjDef &def = Get_Definition();
	if (Get_Weapon() && !def.Fire0Anim.Is_Empty())
	{
		Get_Weapon()->Reset_Anim_Update();
		int state = Get_Weapon()->Get_Anim_State();
		if (state == WEAPON_ANIM_NOT_FIRING)
		{
			Set_Animation( NULL, false, 0.f);
		}
		else if (state == WEAPON_ANIM_FIRING_0)
		{
			Set_Animation( def.Fire0Anim, false, 0.f);
		}
		else if (state == WEAPON_ANIM_FIRING_1)
		{
			Set_Animation( !def.Fire1Anim.Is_Empty() ? def.Fire1Anim : def.Fire0Anim, false, 0.f);
		}
	}
}

RENEGADE_FUNCTION
void VehicleGameObj::Update_Wheel_Effects()
AT2(0x0067AD20,0x0067A5C0);

void VehicleGameObj::Update_Sound_Effects(void)
{
	if (Peek_Vehicle_Phys() == NULL) {
		return;
	}

	switch (EngineSoundState)
	{

	case ENGINE_SOUND_STATE_STARTING:
		{
			// play our ENGINE_STARTING sound, if it is finished or not found, destroy and go to ENGINE_RUNNING
			if ((CachedEngineSound == NULL) || ((CachedEngineSound != NULL) && (CachedEngineSound->Get_State()==0))) {
				Change_Engine_Sound_State(ENGINE_SOUND_STATE_RUNNING);
			}
		}
		break;

	case ENGINE_SOUND_STATE_RUNNING:
		{
			if (CachedEngineSound != NULL) {
				Update_Engine_Sound_Pitch ();
			}
			if (Peek_Vehicle_Phys()->Is_Engine_Enabled() == false) {
				Change_Engine_Sound_State(ENGINE_SOUND_STATE_STOPPING);
			}
		}
		break;

	case ENGINE_SOUND_STATE_STOPPING:
		{
			// play the ENGINE_STOPPING sound, if it is finished or not found, destroy and go to ENGINE_OFF
			if ((CachedEngineSound == NULL) || ((CachedEngineSound != NULL) && (CachedEngineSound->Get_State()==0))) {
				Change_Engine_Sound_State(ENGINE_SOUND_STATE_OFF);
			}
		}
		break;
	case ENGINE_SOUND_STATE_OFF:
		{
			// if the engine is on, go to ENGINE_STARTING
			if (Peek_Vehicle_Phys()->Is_Engine_Enabled() == true) {
				Change_Engine_Sound_State(ENGINE_SOUND_STATE_STARTING);
			}
		}
		break;

	default:
		break;
	}

}

void VehicleGameObj::Change_Engine_Sound_State(int new_state)
{
	if (CachedEngineSound != NULL) {
		CachedEngineSound->Remove_From_Scene();
		CachedEngineSound->Release_Ref();
		CachedEngineSound = NULL;
	}

	EngineSoundState = new_state;

	DefinitionClass * sound_def = DefinitionMgrClass::Find_Definition(Get_Definition().EngineSound[new_state],true);
	if (sound_def != NULL) {
		CachedEngineSound = (AudibleSoundClass *)sound_def->Create();
	}

	if (CachedEngineSound != NULL) {
		CachedEngineSound->Add_To_Scene(true);
		CachedEngineSound->Attach_To_Object(Peek_Physical_Object()->Peek_Model(),-1);
	}

	return ;
}

RENEGADE_FUNCTION
void VehicleGameObj::Update_Engine_Sound_Pitch(void)
AT2(0x0067B0C0,0x0067A960);

RENEGADE_FUNCTION
void VehicleGameObj::Update_Transitions()
AT2(0x0067B800,0x0067B0A0);

bool VehicleGameObj::Scripts_Is_Teammate(const DamageableGameObj* oObject)
{
	int team = SCRIPTS_LastTeam;
	if (SCRIPTS_LastTeam == -2)
	{
		team = Get_Player_Type();
	}
	if ((this == oObject) || (Scripts_Is_Team_Player() && (team == oObject->Get_Player_Type())))
	{
		return true;
	}
	return false;
}

VehiclePhysClass	*VehicleGameObj::Peek_Vehicle_Phys( void )
{
	return Peek_Physical_Object()->As_VehiclePhysClass();
}
