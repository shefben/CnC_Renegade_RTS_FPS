#include "general.h"

#include "DefinitionMgrClass.h"
#include "PersistFactoryClass.h"
#include "Definition.h"
REF_DEF2(DefinitionMgrClass::DefinitionCount, uint32, 0x0083042C, 0x0082F614);
REF_DEF2(DefinitionMgrClass::SortedDefinitionArray, DefinitionClass**, 0x00830428, 0x0082F610);



RENEGADE_FUNCTION
DefinitionClass* DefinitionMgrClass::Find_Definition
   (uint32 uID, bool bFollowTwiddlers)
   AT2(0x00526760,0x00526000);



RENEGADE_FUNCTION
DefinitionClass* DefinitionMgrClass::Find_Named_Definition
   (const char* cPresetName, bool bFollowTwiddlers)
   AT2(0x00526860,0x00526100);



DefinitionClass* DefinitionMgrClass::findPreset
   (uint32 presetId, uint32 classId)
{
   DefinitionClass* preset = Find_Definition (presetId, true);
   if (!preset || preset->Get_Class_ID() != classId)
      return 0;

   return preset;
}



RENEGADE_FUNCTION
DefinitionClass* DefinitionMgrClass::Find_Typed_Definition
   (const char* cName, uint32 uClassID, bool bFollowTwiddlers)
   AT2(0x005268E0,0x00526180);



RENEGADE_FUNCTION
sint32 DefinitionMgrClass::fnCompareDefinitionsCallback
   (const void*, const void*)
   AT2(0x005275D0,0x00526E70);



RENEGADE_FUNCTION
void DefinitionMgrClass::Prepare_Definition_Array()
   AT2(0x00526F40,0x005267E0);







/*
const DefinitionClass* DefinitionMgrClass::Find_Definition
   (uint32 uID, bool bFollowTwiddlers)
{
   DefinitionClass* oDefinition = 0;

   for (uint32 u = 0; u < DefinitionMgrClass::DefinitionCount; i++) {
   DefinitionClass* xDefTemp = DefinitionMgrClass::SortedDefinitionArray[i];
   if (xDefTemp->Get_ID() == uID) {
     xDef = xDefTemp;
     break;
   }
   }

   if (!xDef || !bFollowTwiddlers || xDef->Get_Class_ID() != 0xE000)
   return xDef;

   return ((TwiddlerClass*)xDef)->Twiddle();
}



const DefinitionClass* DefinitionMgrClass::_Find_Typed_Definition
  (const char* cName, uint32 uClassID, bool bFollowTwiddlers)
{
  FX_ASSERT(cName);

  DefinitionClass* xDef = 0;

  for (uint32 i = 0; i < DefinitionMgrClass::DefinitionCount; i++) {
    DefinitionClass* xDefTemp = DefinitionMgrClass::SortedDefinitionArray[i];
    if (xDefTemp->Get_Class_ID() == uClassID && !stricmp (xDefTemp->Get_Name(), cName)) {
      xDef = xDefTemp;
      break;
    }
  }

  if (!xDef || !bFollowTwiddlers || xDef->Get_Class_ID() != 0xE000)
    return xDef;

  return ((TwiddlerClass*)xDef)->Twiddle();
}
*/
RENEGADE_FUNCTION
void DefinitionMgrClass::Free_Definitions()
AT2(0x00526DA0,0x00526640);
