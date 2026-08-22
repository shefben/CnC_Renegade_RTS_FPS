#include "General.h"
#include "tooltip.h"
#include "stylemgr.h"

RENEGADE_FUNCTION
void ToolTipClass::Set_Position(const Vector2& pos)
AT1(0x0050B800);

RENEGADE_FUNCTION
void ToolTipClass::Set_Text(wchar_t const *)
AT1(0x0050B820);

void ToolTipClass::Render()
{
	BackgroundRenderer.Render();
	TextRenderer.Render();
}

ToolTipClass::~ToolTipClass()
{
}

ToolTipClass::ToolTipClass() : TextRenderer(), BackgroundRenderer(), BkColor(1.0f,0.95599997f,0.73299998f)
{
	StyleMgrClass::Assign_Font(&TextRenderer,StyleMgrClass::FONT_TOOLTIPS);
}
