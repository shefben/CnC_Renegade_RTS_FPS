#include "General.h"
#include "dialogcontrol.h"
#include "dialogmgr.h"
#include "mousemgr.h"
#include "stylemgr.h"
DialogControlClass::DialogControlClass() : Rect(0,0,0,0), ClientRect(0,0,0,0), Style(0), ID(0), Parent(0), HasFocus(false), IsDirty(true), WantsFocus(true), IsEnabled(true), IsVisible(true), IsEmbedded(false), TextColor(0,0,0), IsTextColorOverridden(false), AdviseSink(0)
{
	TextColor.X = ((float)(((StyleMgrClass::Get_Text_Color()) >> 0x10) & 0xFF)) / 255;
	TextColor.Y = ((float)(((StyleMgrClass::Get_Text_Color()) >> 8) & 0xFF)) / 255;
	TextColor.Z = ((float)((StyleMgrClass::Get_Text_Color()) & 0xFF)) / 255;
}

DialogControlClass::~DialogControlClass()
{
	if (this == DialogMgrClass::Get_Focus())
	{
		DialogMgrClass::Set_Focus(0);
	}
}

void DialogControlClass::Set_Window_Pos(class Vector2 const &v)
{
	float height = Rect.Bottom - Rect.Top;
	float width = Rect.Right - Rect.Left;
	Rect.Left = (float)(int)v.X;
	Rect.Top = (float)(int)v.Y;
	Rect.Bottom = (float)(int)(Rect.Top + height);
	Rect.Right = (float)(int)(Rect.Left + width);
	Update_Client_Rect();
}

void DialogControlClass::Center_Mouse()
{
}

void DialogControlClass::On_Set_Cursor(Vector2 const &v)
{
	MouseMgrClass::Set_Cursor(MouseMgrClass::CURSOR_ARROW);
}

void DialogControlClass::Enable(bool enable)
{
	if (IsEnabled != enable)
	{
		IsEnabled = enable;
		Set_Dirty(true);
		if ((!IsEnabled) && (DialogMgrClass::Get_Focus() == this))
		{
			DialogMgrClass::Set_Focus(0);
		}
	}
}

void DialogControlClass::Release_Capture()
{
	DialogMgrClass::Release_Capture();
}
void DialogControlClass::Set_Focus()
{
	DialogMgrClass::Set_Focus(this);
}
void DialogControlClass::Set_Capture()
{
	DialogMgrClass::Set_Capture(this);
}
