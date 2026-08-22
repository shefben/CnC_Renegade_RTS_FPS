#include "General.h"
#include "listiconmgr.h"
ListIconMgrClass::ListIconMgrClass() : IconWidth(16), IconHeight(16)
{
}
RENEGADE_FUNCTION
void ListIconMgrClass::Reset_Icons()
AT1(0x005103C0);
RENEGADE_FUNCTION
void ListIconMgrClass::Render_Icons()
AT1(0x00510500);
