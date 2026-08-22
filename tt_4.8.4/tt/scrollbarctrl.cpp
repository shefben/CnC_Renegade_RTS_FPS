#include "General.h"
#include "scrollbarctrl.h"
#include "stylemgr.h"
#include "mousemgr.h"
#include "dialogbase.h"
#include "engine_math.h"
void ScrollBarCtrlClass::Set_Pos(int pos, bool send_notify)
{
	// The position may be out of bounds for the up/down buttons. Clamp it.
	pos = clamp(pos, MinPos, MaxPos);
	if (pos != CurrPos)
	{
		CurrPos = pos;
		Update_Thumb_Rect();
		Set_Dirty(true);
		if (send_notify)
			ADVISE_NOTIFY(On_VScroll(this, ID, CurrPos));
	}
}
void ScrollBarCtrlClass::Set_Range(int range_min, int range_max)
{
	MinPos = range_min;
	MaxPos = max(range_min,range_max);
	Update_Thumb_Rect();
	Set_Dirty(true);
}
ScrollBarCtrlClass::ScrollBarCtrlClass() : WasButtonPressedOnMe(false), MinPos(0), MaxPos(100), CurrPos(0), PageSize(10), CurrState(0), ThumbRect(0,0,0,0), TrackRect(0,0,0,0), TopButtonRect(0,0,0,0), BottomButtonRect(0,0,0,0), MouseClickPos(0,0), MouseClickSliderPos(0), ThumbWidth(0), ThumbHeight(0), IsSmallBMPMode(false), IsDragging(false), WasTopButtonPressed(false), WasBottomButtonPressed(false), Width(10), ScaleX(1), ScaleY(1)
{
	StyleMgrClass::Configure_Renderer(&ButtonRenderer);
	StyleMgrClass::Configure_Renderer(&ControlRenderers[0]);
	StyleMgrClass::Configure_Renderer(&ControlRenderers[1]);
	ButtonRenderer.Set_Texture("IF_MENUPARTS9.TGA");
	RectClass ScreenRes = Render2DClass::Get_Screen_Resolution();
	ScaleX = (ScreenRes.Right - ScreenRes.Left) * 0.00125f;
	ScaleY = (ScreenRes.Right - ScreenRes.Left) * 0.0016666667f;
	Width *= ScaleX;
}
void ScrollBarCtrlClass::Render(void)
{
	if (WasTopButtonPressed)
	{
		Set_Pos(CurrPos - 1,true);
	}
	else if (WasBottomButtonPressed)
	{
		Set_Pos(CurrPos + 1,true);
	}
	if (IsDirty)
	{
		Create_Control_Renderers();
		Create_Button_Renderer();
	}
	ControlRenderers[CurrState].Render();
	ButtonRenderer.Render();
	IsDirty = false;
}
RENEGADE_FUNCTION
void ScrollBarCtrlClass::On_LButton_Down(const Vector2 &mouse_pos)
AT1(0x00506950);
void ScrollBarCtrlClass::On_LButton_Up(const Vector2 &mouse_pos)
{
	Release_Capture();
	WasButtonPressedOnMe = false;
	IsDragging = false;
	WasTopButtonPressed = false;
	WasBottomButtonPressed = false;
	Set_Dirty(true);
}
void ScrollBarCtrlClass::On_Mouse_Move(const Vector2 &mouse_pos)
{
	if (IsDragging)
	{
		float f = ((((mouse_pos.Y - MouseClickPos.Y) + MouseClickSliderPos) - TrackRect.Top) / (TrackRect.Bottom - TrackRect.Top));
		Set_Pos(((int)f * (MaxPos - MinPos)),true);
	}
}
void ScrollBarCtrlClass::On_Set_Cursor(const Vector2 &mouse_pos)
{
	MouseMgrClass::Set_Cursor(MouseMgrClass::CURSOR_ACTION);
}
void ScrollBarCtrlClass::On_Set_Focus(void)
{
	Set_Dirty(true);
	HasFocus = true;
}
void ScrollBarCtrlClass::On_Kill_Focus(DialogControlClass *focus)
{
	Set_Dirty(true);
	HasFocus = false;
}
bool ScrollBarCtrlClass::On_Key_Down(uint32 key_id, uint32 key_data)
{
	switch (key_id)
	{
	case VK_LEFT:
	case VK_UP:
		Set_Pos(CurrPos - 1,true);
		return true;
	case VK_RIGHT:
	case VK_DOWN:
		Set_Pos(CurrPos + 1,true);
		return true;
	case VK_HOME:
		Set_Pos(MinPos,true);
		return true;
	case VK_END:
		Set_Pos(MaxPos,true);
		return true;
	case VK_PRIOR:
		Scroll_Page(-1,true);
		return true;
	case VK_NEXT:
		Scroll_Page(1,true);
		return true;
	default:
		return false;
	}
}
void ScrollBarCtrlClass::On_Create(void)
{
}
RENEGADE_FUNCTION
void ScrollBarCtrlClass::Update_Client_Rect(void)
AT1(0x00506670);
ScrollBarCtrlClass::~ScrollBarCtrlClass()
{
}
RENEGADE_FUNCTION
void ScrollBarCtrlClass::Update_Thumb_Rect(void)
AT1(0x00506EF0);
void ScrollBarCtrlClass::Scroll_Page(int direction, bool send_notify)
{
	Set_Pos(CurrPos + direction * PageSize,send_notify);
	if (send_notify)
	{
		ADVISE_NOTIFY(On_VScroll_Page(this,ID,direction));
	}
}
int ScrollBarCtrlClass::Slider_Pos_From_Mouse_Pos(const Vector2 &mouse_pos)
{
	if (mouse_pos.Y < TrackRect.Top)
	{
		return 0;
	}
	if (mouse_pos.Y > TrackRect.Bottom)
	{
		return MaxPos;
	}
	float f = ((mouse_pos.Y - TrackRect.Top) / (TrackRect.Bottom - TrackRect.Top));
	return ((int)f * (MaxPos - MinPos)) + MinPos;
}
RENEGADE_FUNCTION
void ScrollBarCtrlClass::Create_Control_Renderers(void)
AT1(0x00506530);
RENEGADE_FUNCTION
void ScrollBarCtrlClass::Create_Button_Renderer(void)
AT1(0x005062D0);
