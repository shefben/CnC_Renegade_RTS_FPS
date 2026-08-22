#include "General.h"
#include "shortcutbarctrl.h"
#include "stylemgr.h"
#include "mousemgr.h"
#include "dialogbase.h"
ShortcutBarCtrlClass::ShortcutBarCtrlClass() : FullRect(0,0,0,0), IsHiddenState(true), CurrSel(-1), BarOpacity(1), PulseDirection(-1)
{
	StyleMgrClass::Assign_Font(&TextRenderer,StyleMgrClass::FONT_CONTROLS);
	StyleMgrClass::Assign_Font(&GlowRenderer,StyleMgrClass::FONT_HEADER);
	StyleMgrClass::Assign_Font(&HilightRenderer,StyleMgrClass::FONT_HEADER);
	StyleMgrClass::Configure_Renderer(&ControlRenderer);
	StyleMgrClass::Configure_Renderer(&TexturedControlRenderer);
	TexturedControlRenderer.Set_Texture("if_bar.tga");
}
ShortcutBarCtrlClass::~ShortcutBarCtrlClass()
{
}
void ShortcutBarCtrlClass::Render(void)
{
	if (IsDirty)
	{
		Create_Control_Renderers();
		Create_Text_Renderers();
	}
	ControlRenderer.Render();
	TexturedControlRenderer.Render();
	GlowRenderer.Render();
	TextRenderer.Render();
	HilightRenderer.Render();
	IsDirty = false;
}
void ShortcutBarCtrlClass::Create_Control_Renderers()
{
	ControlRenderer.Reset();
	TexturedControlRenderer.Reset();
	ControlRenderer.Enable_Texturing(false);
	if (IsHiddenState)
	{
		Create_Hidden_State_Renderers();
	}
	else
	{
		Create_Displayed_State_Renderers();
	}
}
void ShortcutBarCtrlClass::Create_Hidden_State_Renderers()
{
	Render_Strip(Rect,0);
}
RENEGADE_FUNCTION
void ShortcutBarCtrlClass::Create_Displayed_State_Renderers()
AT1(0x005020A0);
RENEGADE_FUNCTION
void ShortcutBarCtrlClass::Render_Strip(const RectClass &screen_rect, bool flip_uvs)
AT1(0x00501E90);
RENEGADE_FUNCTION
void ShortcutBarCtrlClass::Create_Text_Renderers()
AT1(0x00501C90);
const RectClass &ShortcutBarCtrlClass::Get_Window_Rect(void) const
{
	if (!IsHiddenState)
	{
		return FullRect;
	}
	else
	{
		return Rect;
	}
}
void ShortcutBarCtrlClass::On_LButton_Down(const Vector2 &mouse_pos)
{
	int entry = Entry_From_Pos(mouse_pos);
	if (entry != -1)
	{
		Set_Curr_Sel(entry,false);
		Send_Command();
	}
}
void ShortcutBarCtrlClass::On_LButton_Up(const Vector2 &mouse_pos)
{
}
RENEGADE_FUNCTION
void ShortcutBarCtrlClass::On_Mouse_Move(const Vector2 &mouse_pos)
AT1(0x00502400);
void ShortcutBarCtrlClass::On_Set_Cursor(const Vector2 &mouse_pos)
{
	MouseMgrClass::Set_Cursor(MouseMgrClass::CURSOR_ACTION);
}
void ShortcutBarCtrlClass::On_Set_Focus(void)
{
	HasFocus = true;
}
void ShortcutBarCtrlClass::On_Kill_Focus(DialogControlClass *focus)
{
	Set_Hidden_State(true);
}
bool ShortcutBarCtrlClass::On_Key_Down(uint32 key_id, uint32 key_data)
{
	switch (key_id)
	{
	case VK_RETURN:
	case VK_SPACE:
		Send_Command();
		return true;
	case VK_LEFT:
	case VK_UP:
		Set_Curr_Sel(CurrSel - 1,true);
		return true;
	case VK_RIGHT:
	case VK_DOWN:
		Set_Curr_Sel(CurrSel + 1,true);
		return true;
	default:
		return false;
	}
}
void ShortcutBarCtrlClass::On_Mouse_Wheel(int direction)
{
	if (!IsHiddenState)
	{
		if (direction <= 0)
		{
			Set_Curr_Sel(CurrSel - 1,true);
		}
		else
		{
			Set_Curr_Sel(CurrSel + 1,true);
		}
	}
}
void ShortcutBarCtrlClass::On_Create(void)
{
}
void ShortcutBarCtrlClass::On_Frame_Update(void)
{
	Update_Pulse();
}
RENEGADE_FUNCTION
void ShortcutBarCtrlClass::Update_Client_Rect(void)
AT1(0x00502180);
RENEGADE_FUNCTION
void ShortcutBarCtrlClass::Set_Curr_Sel(int index, bool snap_mouse)
AT1(0x00502B90);
RENEGADE_FUNCTION
void ShortcutBarCtrlClass::Add_Button(int ctrl_id, const WCHAR *text)
AT1(0x005027C0);
RENEGADE_FUNCTION
void ShortcutBarCtrlClass::Remove_Button(int ctrl_id)
AT1(0x005028F0);
void ShortcutBarCtrlClass::Send_Command()
{
	if (CurrSel >= 0)
	{
		if (CurrSel < ShortcutList.Count())
		{
			Parent->On_Command(ShortcutList[CurrSel].ctrl_id,0,0);
		}
	}
}
RENEGADE_FUNCTION
int ShortcutBarCtrlClass::Entry_From_Pos(const Vector2 &mouse_pos)
AT1(0x00502A70);
void ShortcutBarCtrlClass::Set_Hidden_State(bool onoff)
{
	if (IsHiddenState != onoff)
	{
		IsHiddenState = onoff;
		Set_Dirty(true);
		if (IsHiddenState)
		{
			Release_Capture();
		}
		else
		{
			Set_Focus();
			Set_Capture();
		}
	}
}
RENEGADE_FUNCTION
void ShortcutBarCtrlClass::Update_Pulse()
AT1(0x00502D50);
