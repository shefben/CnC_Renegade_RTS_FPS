#include "General.h"
#include "checkboxctrl.h"
#include "stylemgr.h"
#include "dialogbase.h"
#include "mousemgr.h"
void CheckBoxCtrlClass::Set_Check(bool onoff)
{
	Internal_Set_Check(onoff,false);
}
void CheckBoxCtrlClass::Internal_Set_Check(bool onoff,bool notify)
{
	IsChecked = onoff;
	Set_Dirty(true);
	if (notify)
	{
		Parent->On_Command(ID,0,onoff);
	}
}
CheckBoxCtrlClass::CheckBoxCtrlClass() : IsChecked(false), ButtonRect(0,0,0,0), TextRect(0,0,0,0), MaxRect(0,0,0,0)
{
	StyleMgrClass::Assign_Font(&TextRenderer,StyleMgrClass::FONT_CONTROLS);
	StyleMgrClass::Configure_Renderer(&ControlRenderer);
}
void CheckBoxCtrlClass::Render(void)
{
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
void CheckBoxCtrlClass::On_LButton_Down(const Vector2 &mouse_pos)
{
	Internal_Set_Check(!IsChecked,true);
}
void CheckBoxCtrlClass::On_LButton_Up(const Vector2 &mouse_pos)
{
}
void CheckBoxCtrlClass::On_Set_Cursor(const Vector2 &mouse_pos)
{
	if ((mouse_pos.X > Rect.Left) && (mouse_pos.X < Rect.Right) && (mouse_pos.Y > Rect.Top) && (mouse_pos.Y < Rect.Bottom) && (mouse_pos.X < TextRect.Right))
	{
		MouseMgrClass::Set_Cursor(MouseMgrClass::CURSOR_ACTION);
	}
	else
	{
		MouseMgrClass::Set_Cursor(MouseMgrClass::CURSOR_ARROW);
	}
}
void CheckBoxCtrlClass::On_Set_Focus(void)
{
	Set_Dirty(true);
	HasFocus = true;
}
void CheckBoxCtrlClass::On_Kill_Focus(DialogControlClass *focus)
{
	Set_Dirty(true);
	HasFocus = false;
}
bool CheckBoxCtrlClass::On_Key_Down(uint32 key_id, uint32 key_data)
{
	if (key_id == VK_SPACE)
	{
		Internal_Set_Check(!IsChecked,true);
		return 1;
	}
	return 0;
}
void CheckBoxCtrlClass::On_Create(void)
{
}
RENEGADE_FUNCTION
void CheckBoxCtrlClass::Update_Client_Rect(void)
AT1(0x00509890);
CheckBoxCtrlClass::~CheckBoxCtrlClass()
{
}
void CheckBoxCtrlClass::Create_Control_Renderers()
{
	ControlRenderer.Reset();
	ControlRenderer.Enable_Texturing(false);
	int LineColor = StyleMgrClass::Get_Line_Color();
	int BkColor = StyleMgrClass::Get_Bk_Color();
	if (!IsEnabled)
	{
		LineColor = StyleMgrClass::Get_Disabled_Line_Color();
		BkColor = StyleMgrClass::Get_Disabled_Bk_Color();
	}
	ControlRenderer.Add_Rect(ButtonRect,1.0,LineColor,BkColor);
	if (IsChecked)
	{
		int TextColor = StyleMgrClass::Get_Text_Color();
		int TextShadowColor = StyleMgrClass::Get_Text_Shadow_Color();
		if (!IsEnabled)
		{
			TextColor = StyleMgrClass::Get_Disabled_Text_Color();
			TextShadowColor = StyleMgrClass::Get_Disabled_Text_Shadow_Color();
		}
		RectClass OutlineRect = ButtonRect;
		OutlineRect.Left += 5;
		OutlineRect.Top += 5;
		OutlineRect.Right -= 5;
		OutlineRect.Bottom -= 5;
		Vector2 linepos1;
		Vector2 linepos2;
		linepos2.X = OutlineRect.Right - 2;
		linepos2.Y = OutlineRect.Bottom + 1;
		linepos1.X = OutlineRect.Left - 1;
		linepos1.Y = OutlineRect.Top + 1;
		ControlRenderer.Add_Line(linepos1,linepos2,2.0,TextShadowColor);
		linepos2.X = OutlineRect.Right - 2;
		linepos2.Y = OutlineRect.Top + 1;
		linepos1.X = OutlineRect.Left - 1;
		linepos1.Y = OutlineRect.Bottom + 1;
		ControlRenderer.Add_Line(linepos1,linepos2,2.0,TextShadowColor);
		linepos2.X = OutlineRect.Right - 1;
		linepos2.Y = OutlineRect.Bottom;
		linepos1.X = OutlineRect.Left;
		linepos1.Y = OutlineRect.Top;
		ControlRenderer.Add_Line(linepos1,linepos2,2.0,TextColor);
		linepos2.X = OutlineRect.Right - 1;
		linepos2.Y = OutlineRect.Top;
		linepos1.X = OutlineRect.Left;
		linepos1.Y = OutlineRect.Bottom;
		ControlRenderer.Add_Line(linepos1,linepos2,2.0,TextColor);
	}
}
void CheckBoxCtrlClass::Create_Text_Renderers()
{
	HilightRenderer.Reset();
	HilightRenderer.Set_Coordinate_Range(Render2DClass::Get_Screen_Resolution());
	StyleMgrClass::Configure_Hilighter(&HilightRenderer);
	Vector2 v = TextRenderer.Get_Text_Extents(Title);
	float f = ((((StyleMgrClass::Get_X_Scale() * 8.0f) + v.X) + TextRect.Left));
	TextRect.Right = f;
	if (TextRect.Right >= MaxRect.Right)
	{
		f = MaxRect.Right;
	}
	TextRect.Right = f;
	Rect.Right = f;
	TextRenderer.Reset();
	StyleMgrClass::Render_Text(Title,&TextRenderer,TextRect,true,true,StyleMgrClass::LEFT_JUSTIFY,IsEnabled,true);
	if (HasFocus)
	{
		StyleMgrClass::Render_Hilight(&HilightRenderer,TextRect);
	}
}
