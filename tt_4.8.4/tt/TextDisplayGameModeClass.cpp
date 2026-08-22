#include "general.h"

#include "TextDisplayGameModeClass.h"

REF_DEF2(TextDisplayGameModeClass::Instance, TextDisplayGameModeClass*, 0x0081DF54, 0x0081D13C);

RENEGADE_FUNCTION
void TextDisplayGameModeClass::Print_System(const char* format, ...)
AT2(0x0042CB90,0x0042CCC0);



void TextDisplayGameModeClass::Flush()
{
	while (!o80.Is_Empty())
		delete o80.Remove_Head();
}
