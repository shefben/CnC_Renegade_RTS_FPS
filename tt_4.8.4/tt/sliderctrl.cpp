#include "General.h"
#include "sliderctrl.h"
#include "dialogbase.h"
#include "stylemgr.h"
#include "mousemgr.h"
void SliderCtrlClass::Set_Range(int minrange,int maxrange)
{
	MinPos = minrange;
	MaxPos = maxrange;
	Set_Pos(minrange,true);
}

void SliderCtrlClass::Set_Pos(int position,bool callposchanged)
{
	if (position < MinPos)
	{
		position = MinPos;
	}
	if (position > MaxPos)
	{
		position = MaxPos;
	}
	if (position != CurrPos)
	{
		CurrPos = position;
		Set_Dirty(true);
		if (callposchanged)
		{
			ADVISE_NOTIFY(On_SliderCtrl_Pos_Changed(this,ID,CurrPos));
		}
	}
}
SliderCtrlClass::SliderCtrlClass() : IsDragging(false), MinPos(0), MaxPos(100), CurrPos(0)
{
}
SliderCtrlClass::~SliderCtrlClass()
{
}
RENEGADE_FUNCTION
void SliderCtrlClass::Create_Control_Renderer()
AT1(0x004FBD30);
void SliderCtrlClass::Render(void)
{
	if (IsDirty)
	{
		Create_Control_Renderer();
	}
	ControlRenderer.Render();
}
void SliderCtrlClass::On_LButton_Down(const Vector2 &mouse_pos)
{
	Set_Capture();
	IsDragging = true;
	Set_Pos(Slider_Pos_From_Mouse_Pos(mouse_pos),true);
}
RENEGADE_FUNCTION
int SliderCtrlClass::Slider_Pos_From_Mouse_Pos(const Vector2 &mouse_pos)
AT1(0x004FC0F0);
void SliderCtrlClass::On_LButton_Up(const Vector2 &mouse_pos)
{
	Release_Capture();
	IsDragging = false;
}
void SliderCtrlClass::On_Mouse_Move(const Vector2 &mouse_pos)
{
	if (IsDragging)
	{
		Set_Pos(Slider_Pos_From_Mouse_Pos(mouse_pos),true);
	}
}
void SliderCtrlClass::On_Set_Cursor(const Vector2 &mouse_pos)
{
	MouseMgrClass::Set_Cursor(MouseMgrClass::CURSOR_ACTION);
}
void SliderCtrlClass::On_Set_Focus(void)
{
	Set_Dirty(true);
	HasFocus = true;
}
void SliderCtrlClass::On_Kill_Focus(DialogControlClass *focus)
{
	IsDragging = false;
	Set_Dirty(true);
	HasFocus = false;
}
bool SliderCtrlClass::On_Key_Down(uint32 key_id, uint32 key_data)
{
	switch (key_id)
	{
	case VK_LEFT:
	case VK_DOWN:
		Set_Pos(CurrPos - 1,true);
		return 1;
	case VK_RIGHT:
	case VK_UP:
		Set_Pos(CurrPos + 1,true);
		return 1;
	case VK_HOME:
		Set_Pos(MinPos,true);
		return 1;
	case VK_END:
		Set_Pos(MaxPos,true);
		return 1;
	}
	return 0;
}
void SliderCtrlClass::On_Create(void)
{
}
void SliderCtrlClass::Update_Client_Rect(void)
{
	ClientRect = Rect;
	Set_Dirty(true);
}
