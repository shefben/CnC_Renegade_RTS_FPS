#include "General.h"
#include "multilinetextctrl.h"
#include "stylemgr.h"
#include "mousemgr.h"
#include "dialogbase.h"
MultiLineTextCtrlClass::MultiLineTextCtrlClass() : IsInitialized(false), ScrollPos(0), IsScrollBarDisplayed(false), RowCount(0), RowsPerPage(0)
{
	UINT scroll;
	StyleMgrClass::Assign_Font(&TextRenderer,StyleMgrClass::FONT_CONTROLS);
	StyleMgrClass::Configure_Renderer(&ControlRenderer);
	TextRenderer.Set_Texture_Size_Hint(256);
	ScrollBarCtrl.Set_Wants_Focus(0);
	ScrollBarCtrl.Set_Advise_Sink(this);
	ScrollBarCtrl.Set_Is_Embedded(true);
	SystemParametersInfo(SPI_GETWHEELSCROLLLINES,0,&scroll,0);
	MouseWheelIncrement = scroll * 0.0083333338f;
}
MultiLineTextCtrlClass::~MultiLineTextCtrlClass()
{
	ScrollBarCtrl.Set_Advise_Sink(0);
	if (Parent)
	{
		Parent->Remove_Control(&ScrollBarCtrl);
	}
}
void MultiLineTextCtrlClass::Create_Control_Renderer()
{
	ControlRenderer.Reset();
	ControlRenderer.Enable_Texturing(false);
	int color = StyleMgrClass::Get_Line_Color();
	if (!IsEnabled)
	{
		color = StyleMgrClass::Get_Disabled_Line_Color();
	}
	ControlRenderer.Add_Outline(Rect,1.0,color);
}
RENEGADE_FUNCTION
void MultiLineTextCtrlClass::Create_Text_Renderer()
AT1(0x005071B0);
void MultiLineTextCtrlClass::Render(void)
{
	if (IsDirty)
	{
		Update_Scroll_Bar_Visibility();
		Create_Control_Renderer();
		Create_Text_Renderer();
	}
	ControlRenderer.Render();
	TextRenderer.Render();
	IsDirty = false;
}
RENEGADE_FUNCTION
void MultiLineTextCtrlClass::Update_Scroll_Bar_Visibility()
AT1(0x00507650);
void MultiLineTextCtrlClass::Set_Text(const WCHAR *title)
{
	Title = title;
	Set_Dirty();
	ScrollPos = 0;
	RowCount = 0;
}
void MultiLineTextCtrlClass::On_VScroll(ScrollBarCtrlClass *scrollbar, int ctrl_id, int new_position)
{
	Set_Scroll_Pos(new_position);
}
RENEGADE_FUNCTION
void MultiLineTextCtrlClass::Update_Client_Rect(void)
AT1(0x00507500);
bool MultiLineTextCtrlClass::On_Key_Down(uint32 key_id, uint32 key_data)
{
	switch (key_id)
	{
	case VK_UP:
		Set_Scroll_Pos(ScrollPos - 1);
		break;
	case VK_DOWN:
		Set_Scroll_Pos(ScrollPos + 1);
		break;
	case VK_PRIOR:
		Set_Scroll_Pos(ScrollPos - RowsPerPage);
		break;
	case VK_NEXT:
		Set_Scroll_Pos(ScrollPos + RowsPerPage);
		break;
	case VK_HOME:
		Set_Scroll_Pos(0);
		break;
	case VK_END:
		Set_Scroll_Pos(RowCount);
		break;
	}
	return 1;
}
void MultiLineTextCtrlClass::On_Mouse_Wheel(int direction)
{
	int delta = (int)(direction * MouseWheelIncrement);
	int newpos = ScrollPos + delta;
	if (ScrollPos != newpos)
	{
		int pages = RowCount - RowsPerPage;
		if (newpos > pages)
		{
			newpos = pages;
		}
		if (newpos < 0)
		{
			newpos = 0;
		}
		ScrollPos = newpos;
		ScrollBarCtrl.Set_Pos(newpos,0);
		Set_Dirty(true);
	}
}
void MultiLineTextCtrlClass::Set_Scroll_Pos(int new_position)
{
	if (ScrollPos != new_position)
	{
		int delta = RowCount - RowsPerPage;
		ScrollPos = new_position;
		if (new_position < delta)
		{
			ScrollPos = delta;
		}
		int sp = ((delta <= 0) - 1) & delta;
		ScrollPos = sp;
		ScrollBarCtrl.Set_Pos(sp,false);
		Set_Dirty(true);
	}
}
