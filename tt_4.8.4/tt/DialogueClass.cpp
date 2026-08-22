#include "General.h"
#include "DialogueClass.h"

RENEGADE_FUNCTION
void DialogueClass::Free_Options()
   AT2(0x00767300,0x00766BA0);

RENEGADE_FUNCTION
const DialogueClass& DialogueClass::operator=(const DialogueClass& string)
   AT2(0x00766F70,0x00766810);
