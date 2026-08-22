#include "General.h"
#include "merchandisectrl.h"
#include "stylemgr.h"
#include "mousemgr.h"
extern unsigned int merchandisetext;
RENEGADE_FUNCTION
void MerchandiseCtrlClass::Set_Texture(const char *texture)
AT1(0x004FB6C0);
RENEGADE_FUNCTION
void MerchandiseCtrlClass::Add_Alternate_Texture(const char *texture)
AT1(0x004FB830);
MerchandiseCtrlClass::MerchandiseCtrlClass() : UserData(0), Cost(0), Count(0), CurrentTextureIndex(0)
{
	StyleMgrClass::Assign_Font(&NameTextRenderer,StyleMgrClass::FONT_TOOLTIPS);
	StyleMgrClass::Assign_Font(&CostTextRenderer,StyleMgrClass::FONT_HEADER);
	StyleMgrClass::Assign_Font(&CountTextRenderer,StyleMgrClass::FONT_TOOLTIPS);
	StyleMgrClass::Configure_Renderer(&ControlRenderer);
	StyleMgrClass::Configure_Renderer(&TextureRenderer);
	StyleMgrClass::Configure_Renderer(&ButtonRenderer);
	StyleMgrClass::Configure_Renderer(&HilightRenderer);
	StyleMgrClass::Configure_Hilighter(&HilightRenderer);
	TextureRenderer.Enable_Texturing(true);
	ButtonRenderer.Enable_Texturing(true);
	ButtonRenderer.Set_Texture("hud_cnc_Button.tga");
}
void MerchandiseCtrlClass::Render(void)
{
	if (IsDirty)
	{
		Create_Text_Renderer();
		Create_Control_Renderer();
		Create_Texture_Renderer();
	}
	TextureRenderer.Render();
	ControlRenderer.Render();
	NameTextRenderer.Render();
	CostTextRenderer.Render();
	CountTextRenderer.Render();
	ButtonRenderer.Render();
	HilightRenderer.Render();
}
RENEGADE_FUNCTION
void MerchandiseCtrlClass::On_LButton_Down(const Vector2 &mouse_pos)
AT1(0x004FB970);
RENEGADE_FUNCTION
void MerchandiseCtrlClass::On_LButton_DblClk(const Vector2 &mouse_pos)
AT1(0x004FBAD0);
void MerchandiseCtrlClass::On_Set_Cursor(const Vector2 &mouse_pos)
{
	MouseMgrClass::Set_Cursor(MouseMgrClass::CURSOR_ACTION);
}
RENEGADE_FUNCTION
void MerchandiseCtrlClass::Update_Client_Rect(void)
AT1(0x004FB200);
MerchandiseCtrlClass::~MerchandiseCtrlClass()
{
}
RENEGADE_FUNCTION
void MerchandiseCtrlClass::Create_Control_Renderer()
AT1(0x004FB3D0);
void MerchandiseCtrlClass::Create_Text_Renderer()
{
	WideStringClass cost;
	cost.Format(L"%d",Cost);
	NameTextRenderer.Reset();
	CostTextRenderer.Reset();
	CountTextRenderer.Reset();
	StyleMgrClass::Render_Text(Title,&NameTextRenderer,merchandisetext,0xFF000000,TextRect,true,true,StyleMgrClass::LEFT_JUSTIFY,true);
	StyleMgrClass::Render_Text(cost,&CostTextRenderer,merchandisetext,0xFF000000,CostRect,true,true,StyleMgrClass::LEFT_JUSTIFY,true);
	if (Count > 1)
	{
		WideStringClass count;
		count.Format(L"%d",Count);
		StyleMgrClass::Render_Text(count,&CountTextRenderer,merchandisetext,0xFF000000,CountRect,true,true,StyleMgrClass::CENTER_JUSTIFY,true);
	}
}
void MerchandiseCtrlClass::Create_Texture_Renderer()
{
	int color = 0xFFFFFFFF;
	if (!IsEnabled)
	{
		color = 0xFF606060;
	}
	TextureRenderer.Reset();
	TextureRenderer.Add_Quad(Rect,color);
}
