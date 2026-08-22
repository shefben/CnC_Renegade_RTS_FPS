#include "General.h"
#include "ProgressCtrl.h"
#include "stylemgr.h"
RENEGADE_FUNCTION
void ProgressCtrlClass::Set_Range(unsigned int minrange, unsigned int maxrange)
AT1(0x004FE660);
float ProgressCtrlClass::Calculate_Bar_Width(unsigned int position)
{
	return (((((Rect.Right - Rect.Left) - 6.0f) / (mMaxLimit - mMinLimit)) * position - mMinLimit));
}
void ProgressCtrlClass::Set_Position(unsigned int position)
{
	if (mMaxLimit < position)
	{
		position = mMaxLimit;
	}
	if (mMinLimit > position)
	{
		position = mMinLimit;
	}
	unsigned int oldpos = mPosition;
	mPosition = position;
	if (oldpos != mPosition)
	{
		mBarRect.Right = Calculate_Bar_Width(position) + mBarRect.Left;
	}
	Set_Dirty(true);
}
ProgressCtrlClass::ProgressCtrlClass() : mBarRect(0,0,0,0), mMinLimit(0), mMaxLimit(100), mPosition(0), mStep(10)
{
	StyleMgrClass::Configure_Renderer(&mControlRenderer);
}
ProgressCtrlClass::~ProgressCtrlClass()
{
}
void ProgressCtrlClass::Create_Control_Renderers()
{
	mControlRenderer.Reset();
	mControlRenderer.Enable_Texturing(false);
	int LineColor = StyleMgrClass::Get_Line_Color();
	int BkColor = StyleMgrClass::Get_Bk_Color();
	int TextColor = StyleMgrClass::Get_Text_Color();
	if (!IsEnabled)
	{
		LineColor = StyleMgrClass::Get_Disabled_Line_Color();
		BkColor = StyleMgrClass::Get_Disabled_Bk_Color();
		TextColor = StyleMgrClass::Get_Disabled_Text_Color();
	}
	mControlRenderer.Add_Rect(Rect,1.0,LineColor,BkColor);
	if (mPosition > mMinLimit)
	{
		mControlRenderer.Add_Quad(mBarRect,TextColor);
	}
}
void ProgressCtrlClass::Render(void)
{
	if (IsDirty)
	{
		Create_Control_Renderers();
	}
	mControlRenderer.Render();
	IsDirty = false;
}
void ProgressCtrlClass::Update_Client_Rect(void)
{
	mBarRect.Left = Rect.Left + 3.0f;
	mBarRect.Top = Rect.Top + 3.0f;
	mBarRect.Bottom = Rect.Bottom - 3.0f;
	mBarRect.Right = Calculate_Bar_Width(mPosition) + mBarRect.Left;
	Set_Dirty(true);
}
void ProgressCtrlClass::Get_Range(unsigned int& min, unsigned int& max)
{
	min = mMinLimit;
	max = mMaxLimit;
}
void ProgressCtrlClass::Delta_Position(int delta)
{
	Set_Position(mPosition + delta);
}
unsigned int ProgressCtrlClass::Get_Position(void) const
{
	return mPosition;
}
void ProgressCtrlClass::Set_Step(unsigned int step)
{
	unsigned int newstep;
	newstep = mMaxLimit - mMinLimit;
	if (newstep >= step)
	{
		newstep = step;
	}
	if (newstep < 1)
	{
		newstep = 1;
	}
	mStep = newstep;
}
RENEGADE_FUNCTION
void ProgressCtrlClass::Step_Position(void)
AT1(0x004FE8D0);
