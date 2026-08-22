#include "General.h"
#include "IMECandidateCtrl.h"
#include "stylemgr.h"
#include "dialogbase.h"
#include "mousemgr.h"
IMECandidateCtrl::IMECandidateCtrl() : mFullRect(0,0,0,0), mCellSize(0,0), mCurrSel(-1), mScrollPos(0), mCellsPerPage(0), mCandidate(0)
{
	StyleMgrClass::Assign_Font(&mTextRenderer,StyleMgrClass::FONT_LISTS);
	Set_Wants_Focus(false);
	mScrollBarCtrl.Set_Wants_Focus(false);
	mScrollBarCtrl.Set_Small_BMP_Mode(true);
	mScrollBarCtrl.Set_Advise_Sink(this);
}
RENEGADE_FUNCTION
void IMECandidateCtrl::On_VScroll(ScrollBarCtrlClass*, int, int)
AT1(0x005111A0);
void IMECandidateCtrl::Render(void)
{
	if (IsDirty)
	{
		CreateControlRenderer();
		CreateTextRenderer();
	}
	mControlRenderer.Render();
	mTextRenderer.Render();
	mHilightRenderer.Render();
	IsDirty = false;
}
void IMECandidateCtrl::CreateControlRenderer()
{
	mControlRenderer.Reset();
	mControlRenderer.Enable_Texturing(false);
	mControlRenderer.Set_Coordinate_Range(Render2DClass::Get_Screen_Resolution());
	mControlRenderer.Add_Quad(mFullRect,0xAA000000);
	int linecol = StyleMgrClass::Get_Line_Color();
	int bkcol = StyleMgrClass::Get_Bk_Color();
	if (!IsEnabled)
	{
		linecol = StyleMgrClass::Get_Disabled_Line_Color();
		bkcol = StyleMgrClass::Get_Disabled_Bk_Color();
	}
	mControlRenderer.Add_Rect(Rect,1.0,linecol,bkcol);
}
RENEGADE_FUNCTION
void IMECandidateCtrl::CreateTextRenderer()
AT1(0x00510A00);
RENEGADE_FUNCTION
void IMECandidateCtrl::Update_Client_Rect(void)
AT1(0x00510D90);
void IMECandidateCtrl::On_Set_Cursor(const Vector2& mouse_pos)
{
	if ((mouse_pos.X > ClientRect.Left) && (mouse_pos.X < ClientRect.Right) && (mouse_pos.Y > ClientRect.Top) && (mouse_pos.Y < ClientRect.Bottom))
	{
		MouseMgrClass::Set_Cursor(MouseMgrClass::CURSOR_ACTION);
	}
}
RENEGADE_FUNCTION
void IMECandidateCtrl::On_LButton_Down(const Vector2& mousePos)
AT1(0x00511010);
RENEGADE_FUNCTION
void IMECandidateCtrl::On_LButton_Up(const Vector2& mousePos)
AT1(0x005110C0);
void IMECandidateCtrl::On_Add_To_Dialog(void)
{
	Parent->Add_Control(&mScrollBarCtrl);
}
void IMECandidateCtrl::On_Remove_From_Dialog(void)
{
	Parent->Remove_Control(&mScrollBarCtrl);
}
IMECandidateCtrl::~IMECandidateCtrl()
{
}
