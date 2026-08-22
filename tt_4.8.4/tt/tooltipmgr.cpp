#include "General.h"
#include "tooltipmgr.h"

RENEGADE_FUNCTION
void ToolTipMgrClass::Render()
AT1(0x0050B160);

RENEGADE_FUNCTION
void ToolTipMgrClass::Reset()
AT1(0x0050B180);

RENEGADE_FUNCTION
void ToolTipMgrClass::Update(Vector2 const &)
AT1(0x0050B190);

RENEGADE_FUNCTION
void ToolTipMgrClass::Shutdown()
AT1(0x0050B130);
