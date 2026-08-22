#include "General.h"
#include "comboboxctrl.h"
#include "stylemgr.h"
#include "dialogbase.h"
#include "mousemgr.h"
#include "dialogmgr.h"
int ComboBoxCtrlClass::Select_String(const WCHAR *string)
{
	int i = DropDownCtrl.Find_String(string);
	if (i >= 0)
	{
		Set_Sel(i,false);
	}
	return i;
}
void ComboBoxCtrlClass::Delete_String(int index)
{
	if (index >= 0)
	{
		if (index < DropDownCtrl.Get_Count())
		{
			DropDownCtrl.Delete_String(index);
			CurrSel = -2;
			Set_Sel(DropDownCtrl.Get_Curr_Sel(),false);
		}
	}
}
RENEGADE_FUNCTION
void ComboBoxCtrlClass::Set_Sel(int index,bool notify)
AT1(0x004EF140);
void ComboBoxCtrlClass::Set_Curr_Sel(int index)
{
	Set_Sel(index,false);
}
ComboBoxCtrlClass::ComboBoxCtrlClass() : IsDropDownDisplayed(false), ButtonRect(0,0,0,0), TextRect(0,0,0,0), FullRect(0,0,0,0), DropDownSize(0,0), WasButtonPressedOnMe(false), IsInitialized(false), LastDropDownDisplayChange(0), CurrSel(-1)
{
	StyleMgrClass::Assign_Font(&TextRenderer,StyleMgrClass::FONT_LISTS);
	StyleMgrClass::Configure_Renderer(&ControlRenderer);
	StyleMgrClass::Configure_Renderer(&HilightRenderer);
	DropDownCtrl.Set_Combo_Box(this);
	DropDownCtrl.Set_Is_Embedded(true);
	EditControl.Set_Is_Embedded(true);
}
void ComboBoxCtrlClass::On_LButton_Down(const Vector2 &mouse_pos)
{
	if (HasFocus)
	{
		WasButtonPressedOnMe = true;
	}
	Display_Drop_Down(IsDropDownDisplayed == 0);
}
void ComboBoxCtrlClass::On_LButton_Up(const Vector2 &mouse_pos)
{
	WasButtonPressedOnMe = false;
}
void ComboBoxCtrlClass::On_Mouse_Move(const Vector2 &mouse_pos)
{
}
void ComboBoxCtrlClass::On_Set_Cursor(const Vector2 &mouse_pos)
{
	if ((mouse_pos.X > Rect.Left) && (mouse_pos.X < Rect.Right) && (mouse_pos.Y > Rect.Top) && (mouse_pos.Y < Rect.Bottom))
	{
		MouseMgrClass::Set_Cursor(MouseMgrClass::CURSOR_ACTION);
	}
}
void ComboBoxCtrlClass::On_Set_Focus(void)
{
	Set_Dirty(true);
	HasFocus = true;
}
void ComboBoxCtrlClass::On_Kill_Focus(DialogControlClass *focus)
{
	Set_Dirty(true);
	HasFocus = false;
}
void ComboBoxCtrlClass::On_Mouse_Wheel(int direction)
{
	int sel;
	if (direction >= 0)
	{
		sel = DropDownCtrl.Get_Curr_Sel() + 1;
	}
	else
	{
		sel = DropDownCtrl.Get_Curr_Sel() - 1;
		if (sel < 0)
		{
			sel = 0;
		}
	}
	Set_Sel(sel,true);
}
bool ComboBoxCtrlClass::On_Key_Down(uint32 key_id, uint32 key_data)
{
	switch (key_id)
	{
	case VK_END:
		Set_Sel(DropDownCtrl.Get_Count(),true);
		break;
	case VK_HOME:
		Set_Sel(0,true);
		break;
	case VK_LEFT:
	case VK_UP:
		{
			int sel = DropDownCtrl.Get_Curr_Sel();
			sel--;
			if (sel < 0)
			{
				sel = 0;
			}
			Set_Sel(sel,true);
		}
		break;
	case VK_RIGHT:
	case VK_DOWN:
		Set_Sel(DropDownCtrl.Get_Curr_Sel()+1,true);
		break;
	}
	Set_Dirty(true);
	return false;
}
void ComboBoxCtrlClass::On_Create(void)
{
	if ((Style & 0xF) == 2)
	{
		Parent->Add_Control(&EditControl);
		Set_Wants_Focus(false);
		EditControl.Set_Advise_Sink(this);
	}
}
void ComboBoxCtrlClass::On_Destroy(void)
{
	Parent->Remove_Control(&DropDownCtrl);
	Parent->Remove_Control(&EditControl);
}
RENEGADE_FUNCTION
void ComboBoxCtrlClass::Update_Client_Rect(void)
AT1(0x004EEA50);
void ComboBoxCtrlClass::On_Drop_Down_End(int curr_sel)
{
	Set_Sel(curr_sel,true);
	Display_Drop_Down(false);
	Set_Focus();
}
void ComboBoxCtrlClass::On_EditCtrl_Change(EditCtrlClass* edit_ctrl, int ctrl_id)
{
	if (edit_ctrl == &EditControl)
	{
		ADVISE_NOTIFY(On_ComboBoxCtrl_Edit_Change(this,ID));
	}
}
bool ComboBoxCtrlClass::On_EditCtrl_Key_Down(EditCtrlClass* edit_ctrl, uint32 key_id, uint32 key_data)
{
	if (edit_ctrl == &EditControl)
	{
		if (key_id == VK_UP)
		{
			Set_Sel(max(CurrSel - 1,0),true);
			return true;
		}
		if (key_id == VK_DOWN)
		{
			Set_Sel(CurrSel + 1,true);
			return true;
		}
	}
	return false;
}
const WCHAR *ComboBoxCtrlClass::Get_Text(void) const
{
	if (Style & CBS_DROPDOWN)
	{
		return DropDownCtrl.Get_String(CurrSel);
	}
	else
	{
		return EditControl.Get_Text();
	}
}
RENEGADE_FUNCTION
void ComboBoxCtrlClass::Set_Text(const WCHAR *title)
AT1(0x004EF280);
RENEGADE_FUNCTION
void ComboBoxCtrlClass::Set_Window_Pos(const Vector2 &pos)
AT1(0x004EE9B0);
void ComboBoxCtrlClass::Render(void)
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
	WasButtonPressedOnMe = DialogMgrClass::Is_Button_Down(1) & WasButtonPressedOnMe;
}
void ComboBoxCtrlClass::Set_Style(DWORD style)
{
	Style = style;
	if (!(Style & 0x80))
	{
		EditControl.Set_Style(0);
	}
	else
	{
		EditControl.Set_Style(0x400);
	}
}
ComboBoxCtrlClass::~ComboBoxCtrlClass()
{
}
RENEGADE_FUNCTION
void ComboBoxCtrlClass::Display_Drop_Down(bool onoff)
AT1(0x004EEF20);
RENEGADE_FUNCTION
void ComboBoxCtrlClass::Create_Control_Renderers()
AT1(0x004EE6F0);
void ComboBoxCtrlClass::Create_Text_Renderers()
{
	if ((Style & 0xF) != CBS_DROPDOWN)
	{
		HilightRenderer.Reset();
		HilightRenderer.Set_Coordinate_Range(Render2DClass::Get_Screen_Resolution());
		StyleMgrClass::Configure_Hilighter(&HilightRenderer);
		WideStringClass str;
		DropDownCtrl.Get_String(CurrSel,str);
		TextRenderer.Reset();
		StyleMgrClass::Render_Text(str,&TextRenderer,TextRect,true,true,StyleMgrClass::LEFT_JUSTIFY,IsEnabled,true);
		if (HasFocus)
		{
			if (IsEnabled)
			{
				StyleMgrClass::Render_Hilight(&HilightRenderer,TextRect);
			}
		}
	}
}
