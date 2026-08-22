#include "General.h"
#include "tabctrl.h"
#include "stylemgr.h"
#include "mousemgr.h"
#include "childdialog.h"
#include "dialogmgr.h"
#include "dialogbase.h"
RectClass BarUVs(1,1,26,255);
RectClass SelUVs(28,1,254,69);
RectClass UnfocusUVs(28,236,42,250);
RectClass FocusUVs(43,236,57,250);
RectClass BarTopUVs(1,1,26,79);
RectClass BarBottomUVs(1,169,26,255);
RectClass BarTileUVs(1,80,26,168);
TabCtrlClass::TabCtrlClass() : CurrTabIndex(0), NextBlinkTime(0), IsBubbleDisplayed(false), SelectorPos(0), ScaleX(0), ScaleY(0), SelRect(0,0,0,0)
{
	ScaleX = StyleMgrClass::Get_X_Scale();
	ScaleY = StyleMgrClass::Get_Y_Scale();

	StyleMgrClass::Assign_Font(&TextRenderer,StyleMgrClass::FONT_HEADER);
	StyleMgrClass::Assign_Font(&GlowRenderer,StyleMgrClass::FONT_HEADER);
	StyleMgrClass::Assign_Font(&HilightRenderer,StyleMgrClass::FONT_BIG_HEADER);
	StyleMgrClass::Assign_Font(&HilightGlowRenderer,StyleMgrClass::FONT_BIG_HEADER);
	StyleMgrClass::Configure_Renderer(&ControlRenderer);
	ControlRenderer.Set_Texture("IF_MENUPARTS9.TGA");
}
TabCtrlClass::~TabCtrlClass()
{
	Free_Tabs();
}
RENEGADE_FUNCTION
void TabCtrlClass::Update_Selector()
AT1(0x004FD3F0);
void TabCtrlClass::Update_Bubble()
{
	if (HasFocus)
	{
		if (DialogMgrClass::Get_Time() > NextBlinkTime)
		{
			IsBubbleDisplayed = !IsBubbleDisplayed;
			NextBlinkTime = DialogMgrClass::Get_Time() + 500;
			Set_Dirty(true);
		}
	}
	else
	{
		IsBubbleDisplayed = false;
	}
}
RENEGADE_FUNCTION
void TabCtrlClass::Create_Text_Renderer()
AT1(0x004FC6A0);
RENEGADE_FUNCTION
void TabCtrlClass::Create_Control_Renderer()
AT1(0x004FC920);
void TabCtrlClass::Render(void)
{
	Update_Selector();
	Update_Bubble();
	if (IsDirty)
	{
		Create_Control_Renderer();
		Create_Text_Renderer();
	}
	GlowRenderer.Render();
	HilightGlowRenderer.Render();
	TextRenderer.Render();
	HilightRenderer.Render();
	ControlRenderer.Render();
	IsDirty = false;
}
void TabCtrlClass::On_LButton_Down(const Vector2 &mouse_pos)
{
	if (mouse_pos.X < ((245 * ScaleX) + ClientRect.Left))
	{
		Set_Curr_Tab(Tab_From_Pos(mouse_pos));
	}
}
RENEGADE_FUNCTION
int TabCtrlClass::Tab_From_Pos(const Vector2 &mouse_pos)
AT1(0x004FD270);
void TabCtrlClass::On_LButton_Up(const Vector2 &mouse_pos)
{
}
void TabCtrlClass::On_Mouse_Move(const Vector2 &mouse_pos)
{
}
void TabCtrlClass::On_Mouse_Wheel(int direction)
{
	if (direction)
	{
		Set_Curr_Tab(CurrTabIndex + 1);
	}
	else
	{
		Set_Curr_Tab(CurrTabIndex - 1);
	}
}
void TabCtrlClass::On_Set_Cursor(const Vector2 &mouse_pos)
{
	if (((ScaleX * 245) + ClientRect.Left) > mouse_pos.X)
	{
		MouseMgrClass::Set_Cursor(MouseMgrClass::CURSOR_ACTION);
	}
	else
	{
		MouseMgrClass::Set_Cursor(MouseMgrClass::CURSOR_ARROW);
	}
}
void TabCtrlClass::On_Set_Focus(void)
{
	Set_Dirty(false);
	HasFocus = true;
}
void TabCtrlClass::On_Kill_Focus(DialogControlClass *focus)
{
	Set_Dirty(true);
	HasFocus = false;
}
bool TabCtrlClass::On_Key_Down(uint32 key_id, uint32 key_data)
{
	switch (key_id)
	{
	case VK_LEFT:
	case VK_UP:
		Set_Curr_Tab(CurrTabIndex - 1);
		return true;
	case VK_RIGHT:
	case VK_DOWN:
		Set_Curr_Tab(CurrTabIndex + 1);
		return true;
	case VK_HOME:
		Set_Curr_Tab(0);
		return true;
	case VK_END:
		Set_Curr_Tab(TabList.Count() - 1);
		return true;
	default:
		return false;
	}
}
void TabCtrlClass::On_Create(void)
{
}
void TabCtrlClass::Update_Client_Rect(void)
{
	ClientRect = Rect;
	Set_Dirty(true);
	for (int i = 0;i < TabList.Count();i++)
	{
		RectClass r;
		r.Left = (float)(int)((245 * ScaleX) + ClientRect.Left);
		r.Top = (float)(int)(ClientRect.Top);
		r.Right = (float)(int)(ClientRect.Right);
		r.Bottom = (float)(int)(ClientRect.Bottom);
		TabList[i]->Set_Rect(r);
	}
}
RENEGADE_FUNCTION
void TabCtrlClass::Free_Tabs()
AT1(0x004FD730);
void TabCtrlClass::Set_Curr_Tab(int index)
{
	if (index > TabList.Count() - 1)
	{
		index = TabList.Count() - 1;
	}
	if (index < 0)
	{
		index = 0;
	}
	if (index != CurrTabIndex)
	{
		TabList[CurrTabIndex]->Show(false);
		TabList[index]->Show(true);
		CurrTabIndex = index;
		Set_Dirty(true);
	}
}
void TabCtrlClass::Add_Tab(ChildDialogClass *dialog)
{
	if (dialog)
	{
		dialog->Show(!TabList.Count());
		dialog->Start_Dialog();
		Parent->Add_Child_Dialog(dialog);
		RectClass r;
		r.Left = (float)(int)((245 * ScaleX) + ClientRect.Left);
		r.Top = (float)(int)(ClientRect.Top);
		r.Right = (float)(int)(ClientRect.Right);
		r.Bottom = (float)(int)(ClientRect.Bottom);
		dialog->Set_Rect(r);
		dialog->Add_Ref();
		TabList.Add(dialog);
	}
}
RENEGADE_FUNCTION
void TabCtrlClass::Remove_Tab(int index)
AT1(0x004FD690);
RENEGADE_FUNCTION
bool TabCtrlClass::Apply_Changes_On_Tabs(void)
AT1(0x004FD820);
RENEGADE_FUNCTION
bool TabCtrlClass::Discard_Changes_On_Tabs(void)
AT1(0x004FD850);
RENEGADE_FUNCTION
void TabCtrlClass::Reload_Tabs(void)
AT1(0x004FD880);
