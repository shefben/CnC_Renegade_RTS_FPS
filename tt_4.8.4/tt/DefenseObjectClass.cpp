#include "general.h"
#include "DamageableGameObj.h"
#include "tt.h"
#include "VehicleGameObj.h"
#include "SmartGameObj.h"
#include "cPlayerManager.h"
#include "CombatManager.h"
#include "ArmorWarheadManager.h"
#include "cNetwork.h"
#include "cRemoteHost.h"
#include "CombatManager.h"
#include "cGameType.h"
#include "SoldierGameObj.h"
#include "cConnection.h"
#include "wwmath.h"
#include "OffenseObjectClass.h"
#include "PlayerDataClass.h"

bool NeutralVechiclePointsFix = true;
float DefenseObjectClass::Apply_Damage
   (const OffenseObjectClass& offense, float scale, sint32 skin)
{
	if (CombatManager::I_Am_Server())
	{
		SmartGameObj* damager = offense.Get_Owner() ? offense.Get_Owner()->As_SmartGameObj() : NULL;

		if (!offense.EnableClientDamage || offense.ForceServerDamage || !canTrustClientDamage(damager))
			return Do_Damage(offense, scale, skin);
	}
	else if (cNetwork::Get_Client_Rhost()->getVersion() < 4.0f)
	{
		// BACKCOMPAT
		SmartGameObj* damager = offense.Get_Owner() ? offense.Get_Owner()->As_SmartGameObj() : NULL;
		
		int id = -1;
		if (!Exe)
		{
			id = cNetwork::PClientConnection->Get_Local_Id();
		}
		if (damager && offense.EnableClientDamage && !offense.ForceServerDamage && damager->Get_Control_Owner() == id && canTrustClientDamage(damager))
		{
			// TODO: These are to find the bug that caused StealthEye to be banned by BIATCH for doing 100000000 damage.
			// If this assert triggers, please directly try to figure out what caused it since it rarely occurs!
			TT_ASSERT(scale < 10);
			TT_ASSERT(offense.Get_Damage() < 10000);
			Request_Damage(offense, scale);
		}
	}
	
	return Health;
}



bool DefenseObjectClass::canTrustClientDamage
   (SmartGameObj* damager) const
{
	int id = -1;
	if (!Exe)
	{
		id = cNetwork::PClientConnection->Get_Local_Id();
	}
	if (cGameType::GameType == 3 && // Multiplayer
		Owner.Get_Ptr() &&
		damager &&
		damager->Get_Control_Owner() > 0 &&
		!(CombatManager::I_Am_Server() && damager->Get_Control_Owner() == id))
	{
		DamageableGameObj* damagee = Owner.Get_Ptr()->As_DamageableGameObj();

		if (damagee && !damagee->As_BuildingGameObj())
			return true;
	}

	return false;
}

float DefenseObjectClass::Do_Damage(const OffenseObjectClass& offense, float scale, int alternate_skin)
{
	SmartGameObj* smart = offense.Get_Owner() ? offense.Get_Owner()->As_SmartGameObj() : NULL;


	// Keep track of who is attempting to damage who. This is used as a server
	// heuristic used in network priority calculations. It doesn't matter whether
	// or not damage is actually done.

	if (Get_Owner() && offense.Get_Owner())
	{
		offense.Get_Owner()->Set_Last_Object_Id_I_Damaged(Get_Owner()->Get_Network_ID());
		Get_Owner()->Set_Last_Object_Id_I_Got_Damaged_By(offense.Get_Owner()->Get_Network_ID());
	}


	// Determine and scale total damage by global scales (not scaled for health/shield, hence 'unscaled')
	float unscaledDamage = offense.Get_Damage() * scale;
	
	if (IS_MISSION && offense.Get_Owner() == CombatManager::Get_The_Star())
	{
		switch (CombatManager::Get_Difficulty_Level())
		{
			case 0: unscaledDamage *= 2.f; break;
			case 1: unscaledDamage *= 4.f / 3.f; break;
			case 2: unscaledDamage *= 1.f; break;
		}
	}
	
	
	// Determine health and shield damage scales.
	float healthDamageScale;
	float shieldDamageScale;
	
	if (alternate_skin != -1)
	{
		// Determine health and shield damage scale for alternate skin.
		healthDamageScale = ArmorWarheadManager::Get_Damage_Multiplier(alternate_skin, offense.Get_Warhead());
		shieldDamageScale = 0;
	}
	else
	{
		// Determine health and shield damage scale for our normal skin.
		healthDamageScale = ArmorWarheadManager::Get_Damage_Multiplier(Skin, offense.Get_Warhead());
		shieldDamageScale = ArmorWarheadManager::Get_Damage_Multiplier(ShieldType, offense.Get_Warhead());
	}
	
	// Determine scaled health and shield damage
	float scaledHealthDamage = 0;
	float scaledShieldDamage = 0;
	
	bool isRepair = (unscaledDamage * healthDamageScale < 0 || unscaledDamage * shieldDamageScale < 0);
	if (isRepair)
	{
		// Check whether health is not full
		if (Health < HealthMax && healthDamageScale != 0)
		{
			// Apply to health
			scaledHealthDamage = WWMath::Clamp(unscaledDamage * healthDamageScale, Health - HealthMax, 0);
			scaledShieldDamage = 0;
			Health = Health - scaledHealthDamage;
		}
		else
		{
			// Apply to shield
			scaledHealthDamage = 0;
			scaledShieldDamage = WWMath::Clamp(unscaledDamage * shieldDamageScale, ShieldStrength - ShieldStrengthMax, 0);
			ShieldStrength = ShieldStrength - scaledShieldDamage;
		}
	}
	else
	{
		if (smart && Get_Owner() && smart != Get_Owner() && smart->Is_Teammate(Get_Owner()))
			if (!CombatManager::Is_Friendly_Fire_Permitted())
				return Health;
		
		float unscaledShieldDamage = 0.f;
		
		// if we have a shield, redirect a fraction of our damage
		// If alternate skin (MCT) ignore shield damage
		if (ShieldStrength > 0.f && alternate_skin == -1)
		{
			unscaledShieldDamage = unscaledDamage * ArmorWarheadManager::Get_Shield_Absorbsion(ShieldType, offense.Get_Warhead());
			scaledShieldDamage = unscaledShieldDamage * shieldDamageScale;
			if (ShieldStrength < scaledShieldDamage)
			{
				scaledShieldDamage = ShieldStrength;
				unscaledShieldDamage = scaledShieldDamage / shieldDamageScale;
			}
			
			ShieldStrength = WWMath::Clamp(ShieldStrength - scaledShieldDamage, 0, ShieldStrengthMax);
		}
		
		float unscaledHealthDamage = unscaledDamage - unscaledShieldDamage;
		scaledHealthDamage = unscaledHealthDamage * healthDamageScale;
		
		if (Health < scaledHealthDamage)
			scaledHealthDamage = Health;
		
		Health = Health - scaledHealthDamage;
		
		if (Health < 1.f && !CanObjectDie)
			Health = 1.f;
	}
	
	TT_ASSERT(Health >= 0.0f); // These asserts should show that the below clamp is not necessary; if one of these is hit, contact StealthEye.
	TT_ASSERT(Health <= HealthMax);
	//Health = WWMath::Clamp(Health, 0, HealthMax);
	
	if (scaledHealthDamage != 0 || scaledShieldDamage != 0)
		Mark_Owner_Dirty();
	
	if (scaledHealthDamage > 0 && Health <= 0.0f && smart && smart->As_SoldierGameObj() && Get_Owner() && Get_Owner()->As_SoldierGameObj())
		CombatManager::On_Soldier_Kill(smart->As_SoldierGameObj(), Get_Owner()->As_SoldierGameObj());
	
	// Apply Points for damage/death
	if (smart && smart->Get_Player_Data() && offense.Get_Owner() != Get_Owner())
	{
		// Compute points multiplier
		float pointsMultiplier = 1.f;
		if (Get_Owner())
		{
			if (Get_Owner()->As_VehicleGameObj())
			{
				if (Get_Owner()->As_VehicleGameObj()->Scripts_Is_Teammate(smart))
					// Negative points for teammates
					pointsMultiplier = -pointsMultiplier;
				else if (!Get_Owner()->As_VehicleGameObj()->Scripts_Is_Enemy(smart))
					// No points for neutrals
					pointsMultiplier = 0.f;
			}
			else
			{
				if (smart->Is_Teammate(Get_Owner()))
					// Negative points for teammates
					pointsMultiplier = -pointsMultiplier;
				else if (!smart->Is_Enemy(Get_Owner()))
					// No points for neutrals
					pointsMultiplier = 0.f;
			}
		}
		
		// Compute damage point scale
		float damagePointScale = DamagePoints;
		
		if (isRepair)
			// Half points for repair
			damagePointScale *= .5f;
		
		// Apply damage points
		smart->Get_Player_Data()->Apply_Damage_Points(pointsMultiplier * (scaledHealthDamage + scaledShieldDamage) * damagePointScale, Get_Owner());
		
		// Apply death points
		if (Health <= 0.0f && scaledHealthDamage > 0)
			smart->Get_Player_Data()->Apply_Death_Points(DeathPoints * pointsMultiplier, Get_Owner());
	}
	
	return Health;
}



void DefenseObjectClass::Import(BitStreamClass& packet)
{
	bool is_health_zero = packet.Get(is_health_zero);
	int health				= packet.Get(health, BITPACK_HEALTH);
	int shield_strength	= packet.Get(shield_strength, BITPACK_SHIELD_STRENGTH);
	unsigned int shield_type;
	packet.Get(shield_type, BITPACK_SHIELD_TYPE);
	ShieldType			= shield_type;
	Health				= (float)health;
	ShieldStrength		= (float)shield_strength;
	if (is_health_zero) {
		Health = 0;
	} else {
		Health = WWMath::Max(Health, 0.01f);
	}
}

void DefenseObjectClass::Export(BitStreamClass &packet)
{
	int health				= cMathUtil::Round((double) (float)Health);
	int shield_strength	= cMathUtil::Round((double) (float)ShieldStrength);
	packet.Add((bool)(((float)Health) == 0));
	packet.Add(health,							BITPACK_HEALTH);
	packet.Add(shield_strength,				BITPACK_SHIELD_STRENGTH);
	packet.Add((unsigned long)ShieldType,	BITPACK_SHIELD_TYPE);
}

void DefenseObjectClass::Set_Health
   (float health)
{
	float old_health = Health;
	Health = WWMath::Clamp(health, 0, HealthMax);
	if ( old_health != (float)Health ) {
		Mark_Owner_Dirty();
	}
}

void DefenseObjectClass::Set_Shield_Strength
   (float str)
{
	float old = ShieldStrength;
	ShieldStrength = WWMath::Clamp(str, 0, ShieldStrengthMax);
	if ( old != (float)ShieldStrength ) {
		Mark_Owner_Dirty();
	}
}

// BACKCOMPAT
RENEGADE_FUNCTION
void DefenseObjectClass::Request_Damage(const OffenseObjectClass& offense, float scale)
AT2(0x0068A5E0, 0x00689E80);

void DefenseObjectClass::Set_Precision()
{
	cEncoderList::Set_Precision(BITPACK_HEALTH, 0.0f, (float) MAX_MAX_HEALTH);
	cEncoderList::Set_Precision(BITPACK_SHIELD_STRENGTH, 0.0f, (float) MAX_MAX_SHIELD_STRENGTH);
	cEncoderList::Set_Precision(BITPACK_SHIELD_TYPE, 0.0f,
		//ArmorWarheadManager::Get_Num_Armor_Types(), 1);
		(float)ArmorWarheadManager::Get_Num_Armor_Types());
}

bool DefenseObjectClass::Would_Damage( const OffenseObjectClass	& offense, float scale )
{
	float damage = offense.Get_Damage() * scale;
	float damage_scale = ArmorWarheadManager::Get_Damage_Multiplier( Skin, offense.Get_Warhead() );
	float shield_damage_scale = ArmorWarheadManager::Get_Damage_Multiplier( ShieldType, offense.Get_Warhead() );

	SmartGameObj * smart = NULL;
	if ( offense.Get_Owner() != NULL ) {
		smart = offense.Get_Owner()->As_SmartGameObj();
	}
	if (	smart && Get_Owner() && 
			smart->Is_Teammate( Get_Owner() ) && 
			(smart != Get_Owner() ) ) {
		// This is friendly fire!!
		if ( !CombatManager::Is_Friendly_Fire_Permitted() ) {
   			return false;
   		}
   	}

	if ( damage * damage_scale > 0 && (float)Health > 0 ) {
		return true;
	}

	if ( damage * shield_damage_scale > 0 && (float)ShieldStrength > 0 ) {
		return true;
	}

	return false;
}

bool DefenseObjectClass::Is_Repair( const OffenseObjectClass	& offense, float scale )
{
	float damage = offense.Get_Damage() * scale;
	float damage_scale = ArmorWarheadManager::Get_Damage_Multiplier( Skin, offense.Get_Warhead() );
	float shield_damage_scale = ArmorWarheadManager::Get_Damage_Multiplier( ShieldType, offense.Get_Warhead() );
	return ( (damage * damage_scale < 0) || (damage * shield_damage_scale < 0) );
}
