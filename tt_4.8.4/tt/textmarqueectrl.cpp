#include "General.h"
#include "textmarqueectrl.h"
#include "stylemgr.h"
#include "mousemgr.h"
#include "dialogbase.h"
TextMarqueeCtrlClass::TextMarqueeCtrlClass() : ScrollRate(2), ScrollPos(0), PixelHeight(0)
{
	StyleMgrClass::Assign_Font(&TextRenderers[0],StyleMgrClass::FONT_CREDITS);
	StyleMgrClass::Assign_Font(&TextRenderers[1],StyleMgrClass::FONT_CREDITS_BOLD);
	StyleMgrClass::Configure_Renderer(&ControlRenderer);
	TextRenderers[0].Set_Texture_Size_Hint(256);
	TextRenderers[1].Set_Texture_Size_Hint(256);
}
TextMarqueeCtrlClass::~TextMarqueeCtrlClass()
{
}
void TextMarqueeCtrlClass::Create_Control_Renderer()
{
	ControlRenderer.Reset();
	ControlRenderer.Enable_Texturing(false);
	int linecol = StyleMgrClass::Get_Line_Color();
	if (!IsEnabled)
	{
		linecol = StyleMgrClass::Get_Disabled_Line_Color();
	}
	if (!(Style & WS_DISABLED))
	{
		ControlRenderer.Add_Outline(Rect,1.0,linecol);
	}
}
RENEGADE_FUNCTION
void TextMarqueeCtrlClass::Create_Text_Renderer()
AT1(0x00507E90);
void TextMarqueeCtrlClass::Render(void)
{
	if (IsDirty)
	{
		Create_Control_Renderer();
		Create_Text_Renderer();
	}
	ControlRenderer.Render();
	TextRenderers[0].Render();
	TextRenderers[1].Render();
	IsDirty = false;
}
RENEGADE_FUNCTION
void TextMarqueeCtrlClass::Set_Text(const WCHAR *title)
AT1(0x00508170);
RENEGADE_FUNCTION
void TextMarqueeCtrlClass::Update_Client_Rect(void)
AT1(0x005080B0);
RENEGADE_FUNCTION
void TextMarqueeCtrlClass::On_Frame_Update(void)
AT1(0x00508740);
