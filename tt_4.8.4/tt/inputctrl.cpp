#include "General.h"
#include "inputctrl.h"
#include "stylemgr.h"
#include "mousemgr.h"
#include "dialogbase.h"
#include "dialogmgr.h"
void InputCtrlClass::Set_Key_Assignment(int game_key_id, const WideStringClass &key_name)
{
	KeyAssignment = game_key_id;
	if (key_name)
	{
		Title = key_name;
	}
	Set_Dirty(true);
}
InputCtrlClass::InputCtrlClass() : KeyAssignment(0), MouseIgnoreTime(0), UserData(0), PendingKeyID(-1)
{
	StyleMgrClass::Assign_Font(&TextRenderer,StyleMgrClass::FONT_CONTROLS);
	StyleMgrClass::Configure_Renderer(&ControlRenderer);
}
void InputCtrlClass::Render(void)
{
	if (PendingKeyID != -1)
	{
		if (PendingKeyID >= 0)
		{
			if (PendingKeyID <= 256)
			{
				On_New_Key(PendingKeyID);
			}
			PendingKeyID = -1;
		}
	}
	if (IsDirty)
	{
		Create_Control_Renderers();
		Create_Text_Renderers();
	}
	ControlRenderer.Render();
	TextRenderer.Render();
	HilightRenderer.Render();
	IsDirty = false;
}
void InputCtrlClass::On_LButton_Down(const Vector2 &mouse_pos)
{
	if (HasFocus)
	{
		if (DialogMgrClass::Get_Time() > MouseIgnoreTime)
		{
			On_New_Key(VK_LBUTTON);
		}
	}
}
void InputCtrlClass::On_RButton_Down(const Vector2 &mouse_pos)
{
	if (HasFocus)
	{
		On_New_Key(VK_RBUTTON);
	}
}
void InputCtrlClass::On_MButton_Down(const Vector2 &mouse_pos)
{
	if (HasFocus)
	{
		On_New_Key(VK_MBUTTON);
	}
}
void InputCtrlClass::On_LButton_Up(const Vector2 &mouse_pos)
{
}
void InputCtrlClass::On_Set_Cursor(const Vector2 &mouse_pos)
{
	MouseMgrClass::Set_Cursor(MouseMgrClass::CURSOR_ACTION);
}
void InputCtrlClass::On_Set_Focus(void)
{
	MouseIgnoreTime = DialogMgrClass::Get_Time() + 500;
	Set_Dirty(true);
	HasFocus = true;
}
void InputCtrlClass::On_Kill_Focus(DialogControlClass *focus)
{
	Set_Dirty(true);
	HasFocus = false;
}
bool InputCtrlClass::On_Key_Down(uint32 key_id, uint32 key_data)
{
	PendingKeyID = key_id;
	return false;
}
void InputCtrlClass::On_Create(void)
{
	Title = L"";
}
void InputCtrlClass::On_Mouse_Wheel(int direction)
{
	if (HasFocus)
	{
		if (direction < 0)
		{
			On_New_Key(VK_MOUSEWHEEL_UP);
		}
		else
		{
			On_New_Key(VK_MOUSEWHEEL_DOWN);
		}
	}
}
void InputCtrlClass::Update_Client_Rect(void)
{
	ClientRect = Rect;
	ClientRect.Left -= 1;
	ClientRect.Top -= 1;
	ClientRect.Right += 1;
	ClientRect.Bottom += 1;
	Set_Dirty(true);
}
InputCtrlClass::~InputCtrlClass()
{
}
void InputCtrlClass::On_New_Key(int vkey_id)
{
	if (Parent)
	{
		int game_key_id = 0;
		WideStringClass key_name;
		if (Parent->On_InputCtrl_Get_Key_Info(this,ID,vkey_id,key_name,&game_key_id))
		{
			Set_Key_Assignment(game_key_id,key_name);
		}
	}
}
void InputCtrlClass::Create_Control_Renderers()
{
	ControlRenderer.Reset();
	ControlRenderer.Enable_Texturing(false);
	int linecol = StyleMgrClass::Get_Line_Color();
	int bkcol = StyleMgrClass::Get_Bk_Color();
	if (!IsEnabled)
	{
		linecol = StyleMgrClass::Get_Disabled_Line_Color();
		bkcol = StyleMgrClass::Get_Disabled_Bk_Color();
	}
	ControlRenderer.Add_Rect(Rect,1.0,linecol,bkcol);
}
void InputCtrlClass::Create_Text_Renderers()
{
	HilightRenderer.Reset();
	HilightRenderer.Set_Coordinate_Range(Render2DClass::Get_Screen_Resolution());
	StyleMgrClass::Configure_Hilighter(&HilightRenderer);
	TextRenderer.Reset();
	StyleMgrClass::Render_Text(Title,&TextRenderer,ClientRect,true,true,StyleMgrClass::CENTER_JUSTIFY,IsEnabled,true);
	if (HasFocus)
	{
		StyleMgrClass::Render_Hilight(&HilightRenderer,ClientRect);
	}
}
