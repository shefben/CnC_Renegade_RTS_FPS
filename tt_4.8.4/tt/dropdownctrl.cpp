#include "General.h"
#include "dropdownctrl.h"
#include "stylemgr.h"
#include "dialogbase.h"
#include "mousemgr.h"
#include "comboboxctrl.h"
#include "dialogmgr.h"
RENEGADE_FUNCTION
int DropDownCtrlClass::Add_String(const WCHAR *string)
AT1(0x004EFF60);
void DropDownCtrlClass::Set_Item_Data(int index,uint32 data)
{
	if (index >= 0)
	{
		if (index < EntryList.Count())
		{
			EntryList[index].user_data = data;
		}
	}
}
uint32 DropDownCtrlClass::Get_Item_Data(int index)
{
	if (index >= 0)
	{
		if (index < EntryList.Count())
		{
			return EntryList[index].user_data;
		}
	}
	return 0;
}
void DropDownCtrlClass::Reset_Content()
{
	EntryList.Delete_All();
	Set_Curr_Sel(0);
	Set_Dirty(true);
}
DropDownCtrlClass::DropDownCtrlClass() : ComboBox(0), CellSize(0,0), FullRect(0,0,0,0), CurrSel(-1), ScrollPos(0), CountPerPage(0), DisplayScrollBar(0)
{
	StyleMgrClass::Assign_Font(&TextRenderer,StyleMgrClass::FONT_LISTS);
	ScrollBarCtrl.Set_Wants_Focus(false);
	ScrollBarCtrl.Set_Small_BMP_Mode(true);
	ScrollBarCtrl.Set_Advise_Sink(this);
}
void DropDownCtrlClass::Create_Control_Renderer()
{
	ControlRenderer.Reset();
	ControlRenderer.Enable_Texturing(false);
	ControlRenderer.Set_Coordinate_Range(Render2DClass::Get_Screen_Resolution());
	ControlRenderer.Add_Quad(FullRect,0xEC000000);
	ControlRenderer.Add_Rect(Rect,1.0,StyleMgrClass::Get_Line_Color(),StyleMgrClass::Get_Bk_Color());
}
void DropDownCtrlClass::Create_Text_Renderer()
{
	HilightRenderer.Reset();
	HilightRenderer.Set_Coordinate_Range(Render2DClass::Get_Screen_Resolution());
	StyleMgrClass::Configure_Hilighter(&HilightRenderer);
	TextRenderer.Reset();
	int pos = ScrollPos;
	float top = ClientRect.Top;
	for (int i = pos;i < EntryList.Count();i++)
	{
		Vector2 v = TextRenderer.Get_Text_Extents(EntryList[i].text);
		RectClass r;
		r.Left = ClientRect.Left;
		r.Top = top;
		r.Right = ClientRect.Left + CellSize.X;
		r.Bottom = top + CellSize.Y;
		if (r.Bottom <= ClientRect.Bottom)
		{
			StyleMgrClass::Render_Text(EntryList[i].text,&TextRenderer,r,true);
			if (i == CurrSel)
			{
				StyleMgrClass::Render_Hilight(&HilightRenderer,r);
			}
		}
		top += CellSize.Y;
	}
}
void DropDownCtrlClass::Render(void)
{
	if (IsDirty)
	{
		Create_Control_Renderer();
		Create_Text_Renderer();
	}
	ControlRenderer.Render();
	TextRenderer.Render();
	HilightRenderer.Render();
	IsDirty = false;
}
void DropDownCtrlClass::On_VScroll(ScrollBarCtrlClass *scrollbar, int ctrl_id, int new_position)
{
	ScrollPos = new_position;
	Set_Dirty(true);
}
void DropDownCtrlClass::On_LButton_Down(const Vector2 &mouse_pos)
{
	Set_Curr_Sel(Entry_From_Pos(mouse_pos));
}
void DropDownCtrlClass::On_LButton_Up(const Vector2 &mouse_pos)
{
	Set_Curr_Sel(Entry_From_Pos(mouse_pos));
	if (ComboBox)
	{
		ComboBox->On_Drop_Down_End(CurrSel);
	}
}
void DropDownCtrlClass::On_Mouse_Move(const Vector2 &mouse_pos)
{
	if (DialogMgrClass::Is_Button_Down (VK_LBUTTON))
	{
		Set_Curr_Sel(Entry_From_Pos(mouse_pos));
	}
}
void DropDownCtrlClass::On_Set_Cursor(const Vector2 &mouse_pos)
{
	if ((mouse_pos.X > Rect.Left) && (mouse_pos.X < Rect.Right) && (mouse_pos.Y > Rect.Top) && (mouse_pos.Y < Rect.Bottom))
	{
		MouseMgrClass::Set_Cursor(MouseMgrClass::CURSOR_ACTION);
	}
}
void DropDownCtrlClass::On_Kill_Focus(DialogControlClass *focus)
{
	if (ComboBox)
	{
		ComboBox->On_Drop_Down_End(CurrSel);
	}
}
bool DropDownCtrlClass::On_Key_Down(uint32 key_id, uint32 key_data)
{
	switch (key_id)
	{
	case VK_HOME:
		Set_Curr_Sel(0);
		return 1;
	case VK_END:
		Set_Curr_Sel(EntryList.Count() - 1);
		return 1;
	case VK_LEFT:
	case VK_UP:
		Set_Curr_Sel(EntryList.Count() - 1);
		return 1;
	case VK_RIGHT:
	case VK_DOWN:
		Set_Curr_Sel(EntryList.Count() + 1);
		return 1;
	default:
		return 0;
	}
}
void DropDownCtrlClass::On_Create(void)
{
}
void DropDownCtrlClass::On_Add_To_Dialog(void)
{
	if (DisplayScrollBar)
	{
		Parent->Add_Control(&ScrollBarCtrl);
	}
	TextColor.X = 0.34999999f;
	TextColor.Y = 1.0;
	TextColor.Z = 0.34999999f;
}
void DropDownCtrlClass::On_Remove_From_Dialog(void)
{
	if (DisplayScrollBar)
	{
		Parent->Remove_Control(&ScrollBarCtrl);
	}
}
RENEGADE_FUNCTION
void DropDownCtrlClass::Update_Client_Rect(void)
AT1(0x004EF810);
DropDownCtrlClass::~DropDownCtrlClass()
{
}
int DropDownCtrlClass::Find_String (const WCHAR* string)
{
	if (string)
	{
		for (int i = 0;i < EntryList.Count();i++)
		{
			if (!_wcsicmp(EntryList[i].text,string))
			{
				return i;
			}
		}
	}
	return -1;
}
void DropDownCtrlClass::Delete_String(int index)
{
	if ((index >= 0) && (index < EntryList.Count()))
	{
		EntryList.Delete(index);
		Set_Curr_Sel(min(CurrSel,EntryList.Count()-1));
		Set_Dirty(true);
	}
}
int DropDownCtrlClass::Find_Closest_String (const WCHAR* string)
{
	if ((string) && (wcslen(string)))
	{
		for (int i = 0;i < EntryList.Count();i++)
		{
			int v = _wcsicmp(EntryList[i].text,string);
			if (!v || v < 0)
			{
				return i;
			}
		}
	}
	return -1;
}
void DropDownCtrlClass::Set_Curr_Sel(int index)
{
	if (index >= -1)
	{
		if (index < EntryList.Count())
		{
			if (index != CurrSel)
			{
				CurrSel = index;
				Update_Scroll_Pos();
				Set_Dirty(true);
			}
		}
	}
}
void DropDownCtrlClass::Update_Scroll_Pos()
{
	if (CurrSel != -1)
	{
		if (CurrSel >= ScrollPos)
		{
			if (CurrSel >= CountPerPage + ScrollPos)
			{
				ScrollPos = ((CurrSel - CountPerPage + 1 <= 0) - 1) & (CurrSel - CountPerPage + 1);
				Set_Dirty(true);
			}
		}
		else
		{
			ScrollPos = CurrSel;
			Set_Dirty(true);
		}
		ScrollBarCtrl.Set_Pos(ScrollPos,false);
	}
}
bool DropDownCtrlClass::Get_String(int index, WideStringClass &string) const
{
	if (index >= 0)
	{
		if (index < EntryList.Count())
		{
			string = EntryList[index].text;
			return true;
		}
	}
	return false;
}
const WCHAR *DropDownCtrlClass::Get_String(int index) const
{
	if ((index >= 0) && (index < EntryList.Count()))
	{
		return EntryList[index].text;
	}
	return 0;
}
RENEGADE_FUNCTION
int DropDownCtrlClass::Entry_From_Pos(const Vector2 &mouse_pos)
AT1(0x004F03F0);
