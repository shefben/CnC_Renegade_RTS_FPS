#include "General.h"
#include "listctrl.h"
#include "stylemgr.h"
#include "mousemgr.h"
#include "dialogbase.h"
RENEGADE_FUNCTION
void ListCtrlClass::Delete_All_Entries()
AT1(0x004F3A20);
RENEGADE_FUNCTION
void ListCtrlClass::Add_Column(wchar_t const *title, float f, Vector3 const &v)
AT1(0x004F2B90);
RENEGADE_FUNCTION
int ListCtrlClass::Insert_Entry(int entry, wchar_t const *string)
AT1(0x004F3160);
bool ListCtrlClass::Set_Entry_Color(int index, int col_index, const Vector3 &color)
{
	if ((col_index < 0) || (col_index >= ColList.Count()))
	{
		return 0;
	}
	ColList[col_index]->Set_Entry_Color(index,color);
	Set_Dirty(true);
	return true;
}
void ListCtrlClass::Scroll_To_End()
{
	LastPageTopEntryIndex = Find_Last_Page_Top_Entry();
	if (LastPageTopEntryIndex != ScrollBarCtrl.Get_Max_Pos())
		ScrollBarCtrl.Set_Range(0, LastPageTopEntryIndex);
	ScrollPos = LastPageTopEntryIndex;
	ScrollBarCtrl.Set_Pos(ScrollPos, false);
	Set_Dirty(true);
}
RENEGADE_FUNCTION
bool ListCtrlClass::Delete_Entry(int index)
AT1(0x004F2F30);
void ListCtrlClass::Set_Curr_Sel(int index)
{
	if (index == -1)
	{
		Select_All(false);
		CurrSel = -1;
	}
	else
	{
		Set_Sel(index,false);
	}
}
bool ListCtrlClass::Set_Entry_Text(int index, int col_index, const WCHAR *text)
{
	if ((col_index < 0) || (col_index >= ColList.Count()))
	{
		return 0;
	}
	ColList[col_index]->Set_Entry_Text(index,text);
	Update_Row_Height(index);
	LastPageTopEntryIndex = Find_Last_Page_Top_Entry();
	Set_Dirty(true);
	return true;
}
bool ListCtrlClass::Set_Entry_Data(int index, int col_index, uint32 user_data)
{
	ColList[col_index]->Set_Entry_Data(index,user_data);
	return true;
}
uint32 ListCtrlClass::Get_Entry_Data(int index, int col_index)
{
	if ((col_index < 0) || (col_index >= ColList.Count()) || (index < 0) || (index >= RowInfoList.Count()))
	{
		return 0;
	}
	return ColList[col_index]->Get_Entry_Data(index);
}
const wchar_t* ListCtrlClass::Get_Entry_Text(int index, int col_index)
{
	return ColList[col_index]->Get_Entry_Text(index);
}
RENEGADE_FUNCTION
void ListCtrlClass::Update_Scroll_Bar_Visibility()
AT1(0x004F1CA0);

void ListCtrlClass::Sort(LISTCTRL_SORT_CALLBACK sort_callback, uint32 user_param)
{
	if (RowInfoList.Count() > 1)
	{
		Quick_Sort(0,RowInfoList.Count() - 1,sort_callback,user_param);
		Set_Dirty(true);
	}
}

void ListCtrlClass::Set_Sort_Designator(int col_index, SORT_TYPE type)
{
	SortColumn = col_index;
	SortType = type;
	Set_Dirty(true);
}
ListCtrlClass::ListCtrlClass() : RowBorderHeight(0), ScrollPos(0), CurrSel(-1), CurrState(0), LastPageTopEntryIndex(0), IsScrollBarDisplayed(false), IsSelectionAllowed(true), IsNoSelectionAllowed(false), IsMultipleSelection(false), SortColumn(0), SortType(SORT_NONE), PulsePercent(1), PulseDirection(1), MinRowHeight(0), TextRect(0,0,0,0), HeaderRect(0,0,0,0)
{
	StyleMgrClass::Configure_Renderer(&ControlRenderer);
	StyleMgrClass::Configure_Renderer(&UnderlineRenderer);
	StyleMgrClass::Configure_Renderer(&HilightRenderer);
	StyleMgrClass::Configure_Hilighter(&HilightRenderer);
	StyleMgrClass::Assign_Font(&HeaderRenderer,StyleMgrClass::FONT_HEADER);
	StyleMgrClass::Assign_Font(&TextRenderer,StyleMgrClass::FONT_LISTS);
	TextRenderer.Set_Texture_Size_Hint(256);
	ScrollBarCtrl.Set_Wants_Focus(false);
	ScrollBarCtrl.Set_Advise_Sink(this);
	ScrollBarCtrl.Set_Is_Embedded(true);
}
void ListCtrlClass::Render(void)
{
	if (IsDirty)
	{
		Update_Scroll_Bar_Visibility();
		Create_Text_Renderers();
	}
	if (IsScrollBarDisplayed)
	{
		Parent->Add_Control(&ScrollBarCtrl);
	}
	else
	{
		Parent->Remove_Control(&ScrollBarCtrl);
	}
	if (HasFocus || IsDirty)
	{
		Create_Control_Renderer();
	}
	IconMgr.Render_Icons();
	TextRenderer.Render();
	HilightRenderer.Render();
	UnderlineRenderer.Render();
	HeaderRenderer.Render();
	ControlRenderer.Render();
	IsDirty = false;
}
void ListCtrlClass::On_VScroll(ScrollBarCtrlClass *scrollbar, int ctrl_id, int new_position)
{
	if (ScrollPos != new_position)
	{
		ScrollPos = new_position;
		Set_Dirty(true);
	}
}
void ListCtrlClass::On_VScroll_Page(ScrollBarCtrlClass *scrollbar, int ctrl_id, int direction)
{
	Scroll_Page(direction);
}
void ListCtrlClass::On_LButton_Down(const Vector2 &mouse_pos)
{
	if (mouse_pos.Y < HeaderRect.Bottom)
	{
		int col = Col_From_Pos(mouse_pos);
		if (col != -1)
		{
			ADVISE_NOTIFY(On_ListCtrl_Column_Click(this,ID,col));
		}
	}
	else
	{
		int entry = Entry_From_Pos(mouse_pos);
		if (entry == -1)
		{
			if (IsNoSelectionAllowed)
			{
				Set_Sel(-1,true);
			}
		}
		else
		{
			Set_Sel(entry,true);
		}
	}
}
void ListCtrlClass::On_LButton_DblClk(const Vector2 &mouse_pos)
{
	int entry = Entry_From_Pos(mouse_pos);
	if (entry == CurrSel)
	{
		ADVISE_NOTIFY(On_ListCtrl_DblClk(this,ID,entry));
	}
}
void ListCtrlClass::On_LButton_Up(const Vector2 &mouse_pos)
{
}
void ListCtrlClass::On_Mouse_Move(const Vector2 &mouse_pos)
{
	int entry = Entry_From_Pos(mouse_pos);
	ADVISE_NOTIFY(On_ListCtrl_Mouse_Over(this,ID,entry));
}
void ListCtrlClass::On_Mouse_Wheel(int direction)
{
	if (direction >= 0)
	{
		if (ScrollPos < LastPageTopEntryIndex)
		{
			ScrollPos++;
			ScrollBarCtrl.Set_Pos(ScrollPos);
		}
	}
	else
	{
		if (ScrollPos > 0)
		{
			ScrollPos--;
			ScrollBarCtrl.Set_Pos(ScrollPos);
		}
	}
	Set_Dirty(true);
}
void ListCtrlClass::On_Set_Cursor(const Vector2 &mouse_pos)
{
	if (IsSelectionAllowed)
	{
		MouseMgrClass::Set_Cursor(MouseMgrClass::CURSOR_ACTION);
	}
}
void ListCtrlClass::On_Set_Focus(void)
{
	Set_Dirty(true);
	HasFocus = true;
}
void ListCtrlClass::On_Kill_Focus(DialogControlClass *focus)
{
	Set_Dirty(true);
	HasFocus = false;
}
bool ListCtrlClass::On_Key_Down(uint32 key_id, uint32 key_data)
{
	switch (key_id)
	{
	case VK_LEFT:
	case VK_UP:
		Set_Sel(CurrSel - 1,true);
		return true;
	case VK_RIGHT:
	case VK_DOWN:
		Set_Sel(CurrSel + 1,true);
		return true;
	case VK_PRIOR:
		if (CurrSel == ScrollPos)
		{
			Scroll_Page(-1);
			return true;
		}
		else
		{
			Set_Sel(ScrollPos,true);
			return true;
		}
	case VK_NEXT:
		{
			if(ScrollPos > RowInfoList.Count())
			{
				if(CurrSel == RowInfoList.Count())
				{
					Scroll_Page(1);
					return true;
				}
				else
				{
					Set_Sel(RowInfoList.Count(),true);
					return true;
				}
			}
			else
			{
				float height = TextRect.Top;
				int rc = RowInfoList.Count();
				int i;
				for(i = ScrollPos;i < rc;i++)
				{
					height += RowInfoList[i]->Get_Height();
					if (height >= TextRect.Bottom)
					{
						rc = i - 1;
						break;
					}
				}
				if (CurrSel == rc)
				{
					Scroll_Page(1);
					return true;
				}
				else
				{
					Set_Sel(RowInfoList.Count(),true);
					return true;
				}
			}
		}
	case VK_HOME:
		Set_Sel(0,true);
		return true;
	case VK_END:
		Set_Sel(RowInfoList.Count() - 1,true);
		return true;
	case VK_RETURN:
	case VK_SPACE:
		ADVISE_NOTIFY(On_ListCtrl_DblClk(this,ID,CurrSel));
		return true;
	default:
		return false;
	}

}
void ListCtrlClass::On_Create(void)
{
}
void ListCtrlClass::On_Destroy(void)
{
	Delete_All_Entries();
	Delete_All_Columns();
}
RENEGADE_FUNCTION
void ListCtrlClass::Update_Client_Rect(void)
AT1(0x004F1A80);
ListCtrlClass::~ListCtrlClass()
{
	Delete_All_Entries();
	if (Parent)
	{
		Parent->Remove_Control(&ScrollBarCtrl);
	}
}
void ListCtrlClass::Set_Tabstop(float stop)
{
	TextRenderer.Set_Tabstop(stop);
}
RENEGADE_FUNCTION
void ListCtrlClass::Auto_Size_Columns (float col_spacing)
AT1(0x004F2960);
RENEGADE_FUNCTION
void ListCtrlClass::Auto_Size_Columns_Include_Contents (float col_spacing)
AT1(0x004F2860);
void ListCtrlClass::Set_Column_Color (int col_index, const Vector3 &color)
{
	if (col_index >= 0 && col_index < ColList.Count())
	{
		ColList[col_index]->Set_Color(color);
	}
}
RENEGADE_FUNCTION
bool ListCtrlClass::Remove_Column (int col_index)
AT1(0x004F2CF0);
RENEGADE_FUNCTION
void ListCtrlClass::Delete_All_Columns (void)
AT1(0x004F2E00);
int ListCtrlClass::Get_Column_Count(void) const
{
	return ColList.Count();
}
RENEGADE_FUNCTION
int ListCtrlClass::Find_Entry(int col_index, const WCHAR* text)
AT1(0x004F3160);
RENEGADE_FUNCTION
bool ListCtrlClass::Set_Entry_Int (int index, int col_index, int value)
AT1(0x004F3860);
bool ListCtrlClass::Select_Entry (int index, bool onoff)
{
	if (index < 0 || index >= RowInfoList.Count())
	{
		return false;
	}
	RowInfoList[index]->Select(onoff);
	Set_Dirty(true);
	return true;
}
bool ListCtrlClass::Is_Entry_Selected (int index)
{
	if (index < 0 || index >= RowInfoList.Count())
	{
		return false;
	}
	Set_Dirty(true);
	return RowInfoList[index]->Is_Selected();
}
int ListCtrlClass::Get_Entry_Count (void) const
{
	return RowInfoList.Count();
}
void ListCtrlClass::Set_Min_Row_Height (int height)
{
	MinRowHeight = (float)height;
	for (int i = 0;i < RowInfoList.Count();i++)
	{
		Update_Row_Height(i);
	}
}
RENEGADE_FUNCTION
void ListCtrlClass::Add_Icon (int index, int col_index, const char *texture_name)
AT1(0x004F29D0);
RENEGADE_FUNCTION
void ListCtrlClass::Reset_Icons (int index, int col_index)
AT1(0x004F2B40);
void ListCtrlClass::Set_Icon_Size (float width, float height)
{
	IconMgr.Set_Icon_Width(width);
	IconMgr.Set_Icon_Height(height);
}
int ListCtrlClass::Get_First_Selected (void) const
{
	for (int i = 0;i < Get_Entry_Count();i++)
	{
		if (RowInfoList[i]->Is_Selected())
		{
			return i;
		}
	}
	return -1;
}
int ListCtrlClass::Get_Next_Selected (int index) const
{
	for (int i = index + 1;i < Get_Entry_Count();i++)
	{
		if (RowInfoList[i]->Is_Selected())
		{
			return i;
		}
	}
	return -1;
}
void ListCtrlClass::Select_All (bool onoff)
{
	for (int index = 0;index < Get_Entry_Count();index++)
	{
		RowInfoList[index]->Select(onoff);
	}
}
bool ListCtrlClass::Toggle_Entry_Selection (int index)
{
	if (index < 0 || index >= RowInfoList.Count())
	{
		return false;
	}
	RowInfoList[index]->Select(!RowInfoList[index]->Is_Selected());
	Set_Dirty(true);
	return true;
}
void ListCtrlClass::Sort_Alphabetically (int col_index, SORT_TYPE type)
{
	Sort(Default_Sort_Callback,type+col_index);
}
void ListColumnClass::Free_Data()
{
	Delete_All_Entries();
}
void ListColumnClass::Reset_Contents()
{
	Free_Data();
}
RENEGADE_FUNCTION
int ListColumnClass::Insert_Entry(int index, const WCHAR *entry_name)
AT1(0x004F4390);
RENEGADE_FUNCTION
bool ListColumnClass::Delete_Entry(int index)
AT1(0x004F4500);
RENEGADE_FUNCTION
void ListColumnClass::Delete_All_Entries()
AT1(0x004F45C0);
RENEGADE_FUNCTION
void ListColumnClass::Move_Entry(int old_index, int new_index)
AT1(0x004F4660);
RENEGADE_FUNCTION
void ListColumnClass::Swap_Entries(int index1, int index2)
AT1(0x004F4700);
RENEGADE_FUNCTION
int ListCtrlClass::Default_Sort_Callback(ListCtrlClass *list_ctrl, int item_index1, int item_index2, uint32 user_param)
AT1(0x004F1A10);
int ListCtrlClass::Find_Last_Page_Top_Entry()
{
	float lineStartHeight = TextRect.Bottom;
	for (int rowIndex = RowInfoList.Count() - 1; rowIndex >= 0; --rowIndex)
	{
		lineStartHeight -= RowInfoList[rowIndex]->Get_Height();
		if (lineStartHeight <= TextRect.Top)
			return rowIndex + 1;
	}
	return 0;
}
void ListCtrlClass::Set_Sel(int new_sel, bool notify)
{
	if (IsSelectionAllowed)
	{
		if (!IsMultipleSelection)
		{
			Select_Entry(CurrSel,false);
		}
		int oldsel = CurrSel;
		if ((new_sel == -1) && IsNoSelectionAllowed)
		{
			CurrSel = new_sel;
		}
		else
		{
			int count = Get_Entry_Count();
			CurrSel = max(new_sel,0);
			CurrSel = min(CurrSel,count - 1);
			if (IsMultipleSelection)
			{
				Toggle_Entry_Selection(CurrSel);
			}
			else
			{
				Select_Entry(CurrSel,true);
			}
		}
		if (notify)
		{
			if (oldsel != CurrSel)
			{
				ADVISE_NOTIFY(On_ListCtrl_Sel_Change(this,ID,oldsel,CurrSel));
			}
		}
		Set_Dirty(true);
		Update_Scroll_Pos();
	}
}
RENEGADE_FUNCTION
void ListCtrlClass::Update_Row_Height(int row_index)
AT1(0x004F3550);
RENEGADE_FUNCTION
void ListCtrlClass::Quick_Sort(int start_index, int end_index, LISTCTRL_SORT_CALLBACK sort_callback, uint32 user_param)
AT1(0x004F2350);
RENEGADE_FUNCTION
void ListCtrlClass::Create_Control_Renderer()
AT1(0x004F1080);
RENEGADE_FUNCTION
void ListCtrlClass::Create_Text_Renderers()
AT1(0x004F1270);
RENEGADE_FUNCTION
void ListCtrlClass::Scroll_Page(int direction)
AT1(0x004F4100);
RENEGADE_FUNCTION
int ListCtrlClass::Entry_From_Pos(const Vector2 &mouse_pos)
AT1(0x004F4020);
RENEGADE_FUNCTION
int ListCtrlClass::Col_From_Pos(const Vector2 &mouse_pos)
AT1(0x004F3F50);
RENEGADE_FUNCTION
void ListCtrlClass::Update_Scroll_Pos()
AT1(0x004F3DE0);
