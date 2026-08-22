#include "general.h"

#include "scripts.h"
#include "engine_common.h"
#include "engine_vector.h"
#include "engine_threading.h"
#include "engine_string.h"
#include "WeaponMgr.h"
#include "DefinitionMgrClass.h"
#include "WeaponManager.h"



AmmoDefinitionClass* WeaponManager::Find_Ammo_Definition
   (const char* name)
{
   DefinitionClass* definition = DefinitionMgrClass::Find_Typed_Definition (name, 45058, true);
   if (!definition)
      return 0;

   return static_cast<AmmoDefinitionClass*> (definition);
}



AmmoDefinitionClass* WeaponManager::Find_Ammo_Definition
   (uint32 id)
{
   DefinitionClass* definition = DefinitionMgrClass::Find_Definition (id, true);
   if (!definition)
      return 0;

   return static_cast<AmmoDefinitionClass*> (definition);
}



WeaponDefinitionClass* WeaponManager::Find_Weapon_Definition
   (const char* presetName)
{
   DefinitionClass* preset = DefinitionMgrClass::Find_Typed_Definition (presetName, 45057, true);
   if (!preset)
      return 0;

   return static_cast<WeaponDefinitionClass*> (preset);
}



WeaponDefinitionClass* WeaponManager::Find_Weapon_Definition
   (uint32 presetId)
{
   DefinitionClass* preset = DefinitionMgrClass::findPreset (presetId, 45057);
   if (!preset)
      return 0;

   return static_cast<WeaponDefinitionClass*> (preset);
}
