#ifndef TT_INCLUDE_WEAPONMANAGER_H
#define TT_INCLUDE_WEAPONMANAGER_H


class WeaponManager
{

public:

   static AmmoDefinitionClass*   Find_Ammo_Definition   (const char* presetName);
   static AmmoDefinitionClass*   Find_Ammo_Definition   (uint32 presetId);
   static WeaponDefinitionClass* Find_Weapon_Definition (const char* presetName);
   static WeaponDefinitionClass* Find_Weapon_Definition (uint32 presetId);

};


#endif
