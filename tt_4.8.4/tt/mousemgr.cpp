#include "General.h"
#include "mousemgr.h"

RENEGADE_FUNCTION
void MouseMgrClass::Set_Cursor(CURSOR_TYPE)
AT1(0x004F9400);

RENEGADE_FUNCTION
void MouseMgrClass::Shutdown()
AT1(0x004F9310);

RENEGADE_FUNCTION
void MouseMgrClass::Show_Cursor(bool)
AT1(0x004F9360);

RENEGADE_FUNCTION
void MouseMgrClass::Render()
AT1(0x004F94F0);
