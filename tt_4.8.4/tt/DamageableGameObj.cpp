#include "general.h"

#include "scripts.h"
#include "DamageableGameObj.h"
#include "OffenseObjectClass.h"
#include "cNetwork.h"
#include "cRemoteHost.h"
void DamageableGameObj::Export_Occasional(BitStreamClass &BitStream)
{
	Defense.Export(BitStream);
}

void DamageableGameObj::Set_Player_Type(int type)
{
	PlayerType = type;
	Set_Object_Dirty_Bit(DB_RARE, true);
}



void DamageableGameObj::Import_Rare(BitStreamClass& stream)
{
	if (cNetwork::Get_Client_Rhost()->getVersion() >= 4.0f)
	{
		float temp;
		stream.Get(temp);
		Defense.Set_Health_Max(temp);
		stream.Get(temp);
		Defense.Set_Shield_Strength_Max(temp);
	}
}



void DamageableGameObj::Export_Rare(BitStreamClass& stream)
{
	if (cNetwork::Get_Server_Rhost(cNetwork::lastUpdatedClientId)->getVersion() >= 4.0f)
	{
		stream.Add(Defense.Get_Health_Max());
		stream.Add(Defense.Get_Shield_Strength_Max());
	}
}



void DamageableGameObj::Import_Occasional
   (BitStreamClass& stream)
{
   float previousHealth = this->Get_Defense_Object()->Get_Health();
   float previousShield = this->Get_Defense_Object()->Get_Shield_Strength();
   this->Defense.Import (stream);
   float health = this->Get_Defense_Object()->Get_Health();
   float shield = this->Get_Defense_Object()->Get_Shield_Strength();

   if ((health <= 0 && previousHealth  > 0)
    || (health  > 0 && previousHealth <= 0)
    || ((sint32(health) >> 2) != (sint32(previousHealth) >> 2))
    || ((sint32(shield) >> 2) != (sint32(previousShield) >> 2)))
   {
      float damage = (previousHealth - health) + (previousShield - shield);
      for (sint32 u = 0; u < this->Observers.Count(); ++u)
         this->Observers[u]->Damaged ((GameObject *)this, 0, damage);
      
      if (health <= 0)
      {
         for (sint32 u = 0; u < this->Observers.Count(); ++u)
            this->Observers[u]->Killed ((GameObject *)this, 0);
      
         OffenseObjectClass offense;
         this->Completely_Damaged (offense);
      }
   }
}

RENEGADE_FUNCTION
void DamageableGameObj::Apply_Damage( const OffenseObjectClass & damager, float scale, int alternate_skin )
AT2(0x006D9520,0x006D8DC0);
