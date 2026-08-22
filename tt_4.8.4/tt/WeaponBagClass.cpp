#include "general.h"

#include "WeaponClass.h"
#include "WeaponBagClass.h"
#include "bitstream.h"
#include "WeaponManager.h"
void WeaponBagClass::Select_Weapon_ID
   (uint32 presetId)
{
   for (sint32 u = 1; u < this->weapons.Count(); ++u)
      if (this->weapons[u]->Get_Definition()->Get_ID() == presetId)
      {
         this->Select_Index (u);
         break;
      }
}

WeaponClass* WeaponBagClass::Find_Weapon
   (WeaponDefinitionClass *preset)
{

   for (sint32 u = 1; u < this->weapons.Count(); ++u)
   {
      WeaponClass* weapon = this->weapons[u];
      if (weapon->Get_Definition() == preset)
         return weapon;
   }

   return 0;
}



void WeaponBagClass::Select_Weapon_Name
   (const char* weaponName)
{
   if ((!weaponName) || (!weaponName[0]))
   {
	   this->Deselect();
	   return;
   }
   for (sint32 u = 1; u < this->weapons.Count(); ++u)
   {
      if (!_stricmp (this->weapons[u]->Get_Definition()->Get_Name(), weaponName))
      {
         this->Select_Index (u);
         break;
      }
   }
}

WeaponBagClass::WeaponBagClass
   (ArmedGameObj* owner)
   : owner(owner), weaponIndex(0), Changed(true), HUDChanged(true)
{
   this->Mark_Owner_Dirty();

   WeaponClass* weapon = 0;
   this->weapons.Add (weapon);
}

void WeaponBagClass::Select_Weapon
   (WeaponClass* weapon)
{
   for (sint32 u = 0; u < this->weapons.Count(); ++u)
      if (this->weapons[u] == weapon)
      {
         this->Select_Index (u);
         break;
      }
}

// 0x006F3930, 0x006F2EF0, 0x0818EABC, 0x08177DCA
void WeaponBagClass::Remove_Weapon
   (sint32 weaponIndex)
{
   if (!weaponIndex || weaponIndex >= this->weapons.Count())
      return;

   uint32 newWeaponIndex = 0;

   if (weaponIndex < this->weaponIndex) // the selected weapon comes after the removed weapon
   {
      newWeaponIndex = this->weaponIndex - 1; // the index of the selected weapon will be decreased when removing the weapon
      this->Select_Index (0);
   }
   else if (weaponIndex == this->weaponIndex)  // the selected weapon is the one to be removed
   {
      if (weaponIndex == this->weapons.Count() - 1) // the weapon to be removed is the last one
      {
         if (this->weapons.Count() > 2) // there are more than one other weapons
            newWeaponIndex = 1; // select weapon 1
         // select no weapon otherwise
      }
      else
         newWeaponIndex = this->weaponIndex; // the index if the current weapon will be the index of the next weapon after removal
      
      this->Select_Index (0);
   }


   WeaponClass* weapon = this->weapons[weaponIndex];
   this->weapons.Delete(weaponIndex);

   if (newWeaponIndex)
      this->Select_Index (newWeaponIndex);

   delete weapon;
}

void WeaponBagClass::Deselect()
{
   this->Select_Index (0);
}

WeaponBagClass::~WeaponBagClass()
{
   this->Clear_Weapons();
}


void WeaponBagClass::Import_Weapon_List
   (BitStreamClass& stream)
{
	DynamicVectorClass<uint32> presetIds;
	DynamicVectorClass<sint32> ammoAmounts;

	uint32 count;
	stream.Get(count);
	for (uint32 u = 0; u < count; ++u)
	{
		uint32 presetId;
		stream.Get(presetId);
		sint32 ammoAmmount;
		stream.Get(ammoAmmount);
		presetIds.Add (presetId);
		ammoAmounts.Add (ammoAmmount);
	}

	for (uint32 u = this->weapons.Count() - 1; u > 0; --u)
	{
		WeaponClass* weapon = this->weapons[u];

		uint32 presetId = weapon->Get_Definition()->Get_ID();
		int position = presetIds.ID(presetId);

		if (position == -1)
		{
			this->Remove_Weapon(u);
			continue;
		}

		weapon->Set_Total_Rounds(ammoAmounts[position]);

		presetIds.Delete(position);
		ammoAmounts.Delete(position);
	}

	for (sint32 u = 0; u < presetIds.Count(); ++u)
	{
		WeaponDefinitionClass* preset = WeaponManager::Find_Weapon_Definition (presetIds[u]);
		if (!preset)
			continue;

		this->Add_Weapon(preset, ammoAmounts[u], true);
		if (!this->weaponIndex)
			this->Select_Index (1);
	}
}

void WeaponBagClass::Export_Weapon_List
	(BitStreamClass& stream)
{
   stream.Add(this->weapons.Count() - 1);

   for (int i = 1; i < this->weapons.Count(); ++i)
   {
      WeaponClass* weapon = this->weapons[i];

	  // FIX: Send ammo load for weapons with infinite ammo to make these weapons start loaded.
      int ammo = weapon->Get_Total_Rounds();
      if (ammo < 0)
         ammo = weapon->Get_Clip_Rounds();

      stream.Add(weapon->Get_Definition()->Get_ID());
      stream.Add(ammo);
   }
}

void WeaponBagClass::Mark_Owner_Dirty()
{
   if (this->owner)
      ((ArmedGameObj *)this->owner)->Set_Object_Dirty_Bit(DB_OCCASIONAL,true);
}

void WeaponBagClass::Select_Index
   (sint32 index)
{
   if (index == this->weaponIndex)
      return;

   if (index >= this->weapons.Count())
   {
      return;
   }

   WeaponClass* weapon = this->weapons[this->weaponIndex];
   if (weapon)
      weapon->Deselect();

   this->weaponIndex = index;
   this->Changed = true;
   this->HUDChanged = true;

   weapon = this->weapons[index];
   if (weapon)
      weapon->Select();

   this->Mark_Owner_Dirty();
}

// 0x006F3CA0, 0x006F3260, 0x, 0x081780DA
void WeaponBagClass::Select_Next()
{
   for (sint32 u = this->weaponIndex + 1; u != this->weaponIndex; ++u)
   {
      if (u >= this->weapons.Count())
      {
         if (!this->weaponIndex)
            return;

         u = 0;
      }

      WeaponClass* weapon = this->weapons[u];
	  if (!weapon)
		continue;
	  int cs = weapon->Get_Definition()->ClipSize;
	  if (!weapon->Does_Weapon_Exist() || (!weapon->Get_Total_Rounds() && cs))
		continue;

      this->Select_Index (u);
      break;
   }
}



// 0x006F3D30, 0x006F32F0, 0x, 0x08178146
void WeaponBagClass::Select_Prev()
{
   for (sint32 i = this->weaponIndex - 1; i != (sint32)this->weaponIndex; --i)
   {
      if (i < 0)
      {
         i = this->weapons.Count() - 1;

         if (i == (sint32)this->weaponIndex)
            return;
      }

      WeaponClass* weapon = this->weapons[i];
	  if (!weapon)
		  continue;
	  int cs = weapon->Get_Definition()->ClipSize;
	  if (!weapon->Does_Weapon_Exist() || (!weapon->Get_Total_Rounds() && cs))
         continue;

      this->Select_Index (i);
      break;
   }
}

void WeaponBagClass::Clear_Weapons()
{
   this->Select_Index (0);

   for (uint32 u = this->weapons.Count() - 1; u > 0; --u)
   {
      WeaponClass* weapon = this->weapons[u];
      this->weapons.Delete(u);

      delete weapon;
   }

   this->Mark_Owner_Dirty();
}

// 0x006F3A40, 0x006F3000, 0x0818EB98, 0x08177EA4
WeaponClass* WeaponBagClass::Add_Weapon
   (WeaponDefinitionClass *preset, sint32 ammo, bool owned)
{
   this->Mark_Owner_Dirty();

   WeaponClass* weapon = this->Find_Weapon (preset);
   if (weapon)
   {
      if (ammo < 0)
         weapon->Set_Total_Rounds (-1);
      else
         weapon->Add_Rounds(ammo);

      weapon->Set_Weapon_Exists (owned);

      return weapon;
   }

   weapon = new WeaponClass(preset);
   int cs = preset->ClipSize;
   if ((weapon->Get_Inventory_Rounds() < cs) && (weapon->Get_Inventory_Rounds() != -1))
   {
      weapon->Set_Clip_Rounds(weapon->Get_Inventory_Rounds());
   }
   else
   {
      weapon->Set_Clip_Rounds(preset->ClipSize);
   }
   weapon->Add_Rounds(ammo);
   weapon->Set_Weapon_Exists(owned);
   weapon->Set_Owner(this->owner);

   float keyNumber = weapon->Get_Definition()->KeyNumber;

   sint32 index = 1;
   for (; index < this->weapons.Count(); ++index)
      if (this->weapons[index]->Get_Definition()->KeyNumber > keyNumber)
         break;

   if (this->weaponIndex >= index)
      ++this->weaponIndex;

   this->weapons.Insert (index, weapon);
   this->Changed = true;
   this->HUDChanged = true;
   
   return weapon;
}


WeaponClass* WeaponBagClass::Add_Weapon
   (const char* weaponName, sint32 ammo, bool owned)
{
   WeaponDefinitionClass* preset = WeaponManager::Find_Weapon_Definition (weaponName);
   if (!preset)
      return 0;

   return this->Add_Weapon (preset, ammo, owned);
}

WeaponClass* WeaponBagClass::Add_Weapon
   (uint32 weaponId, sint32 ammo, bool owned)
{
   WeaponDefinitionClass* preset = WeaponManager::Find_Weapon_Definition (weaponId);
   if (!preset)
      return 0;

   return this->Add_Weapon (preset, ammo, owned);
}



bool WeaponBagClass::Is_Changed()
{
   return this->Changed;
}



bool WeaponBagClass::Is_Ammo_Full
   (uint32 weaponId)
{
   WeaponDefinitionClass* preset = WeaponManager::Find_Weapon_Definition (weaponId);
   if (!preset)
      return false;

   WeaponClass* weapon = this->Find_Weapon (preset);
   if (!weapon)
      return false;

   return weapon->Is_Ammo_Maxed();
}



bool WeaponBagClass::Is_Weapon_Owned
   (uint32 weaponId)
{
   WeaponDefinitionClass* preset = WeaponManager::Find_Weapon_Definition (weaponId);
   if (!preset)
      return false;

   WeaponClass* weapon = this->Find_Weapon (preset);
   if (!weapon)
      return false;

   return weapon->Does_Weapon_Exist();
}



RENEGADE_FUNCTION
bool WeaponBagClass::Move_Contents
   (WeaponBagClass *sourceBag)
   AT2(0x006F4370,0x006F3930);



RENEGADE_FUNCTION
void WeaponBagClass::Select_Key_Number
   (uint32 keyNumber)
   AT2(0x006F3DC0,0x006F3380);

void WeaponBagClass::Reset_Changed
   ()
{
   this->Changed = false;
}
