#include "General.h"
#include "editctrl.h"
#include "dialogmgr.h"
#include "stylemgr.h"
#include "mousemgr.h"

RENEGADE_FUNCTION
bool EditCtrlClass::Old_On_Key_Down(uint32 key_id, uint32 key_data)
AT1(0x004F6690);
RENEGADE_FUNCTION
void EditCtrlClass::Insert_String(const wchar_t *string)
AT1(0x004F6F40);
extern REF_DECL1(_Hwnd_0, HWND);
bool EditCtrlClass::On_Key_Down(uint32 key_id, uint32 key_data)
{
	if (((DialogMgrClass::Get_VKey_State(VK_CONTROL) & VKEY_PRESSED) == VKEY_PRESSED) && ((DialogMgrClass::Get_VKey_State(VK_SHIFT) & VKEY_PRESSED) != VKEY_PRESSED) && ((DialogMgrClass::Get_VKey_State(VK_MENU) & VKEY_PRESSED) != VKEY_PRESSED))
	{
		switch(key_id)
		{
		case 'X':
			if (HilightEndPos-HilightStartPos)
			{
				if (OpenClipboard(_Hwnd_0))
				{
					if (EmptyClipboard())
					{
						const wchar_t *w = Get_Text();
						HANDLE w2 = GlobalAlloc(GMEM_MOVEABLE,(HilightEndPos-HilightStartPos+1)*2);
						void *w2x = GlobalLock(w2);
						memcpy(w2x,&w[HilightStartPos],(HilightEndPos-HilightStartPos)*2);
						((wchar_t *)w2x)[HilightEndPos-HilightStartPos] = 0;
						GlobalUnlock(w2);
						SetClipboardData(CF_UNICODETEXT,w2);
					}
					CloseClipboard();
				}
			}
			Old_On_Key_Down(VK_DELETE,0x01530001);
			break;
		case 'C':
			if (HilightEndPos-HilightStartPos)
			{
				if (OpenClipboard(_Hwnd_0))
				{
					if (EmptyClipboard())
					{
						const wchar_t *w = Get_Text();
						HANDLE w2 = GlobalAlloc(GMEM_MOVEABLE,(HilightEndPos-HilightStartPos+1)*2);
						void *w2x = GlobalLock(w2);
						memcpy(w2x,&w[HilightStartPos],(HilightEndPos-HilightStartPos)*2);
						((wchar_t *)w2x)[HilightEndPos-HilightStartPos] = 0;
						GlobalUnlock(w2);
						SetClipboardData(CF_UNICODETEXT,w2);
					}
					CloseClipboard();
				}
			}
			return true;
			break;
		case 'V':
			if (IsClipboardFormatAvailable(CF_UNICODETEXT))
			{
				if (OpenClipboard(_Hwnd_0))
				{
					HANDLE h = GetClipboardData(CF_UNICODETEXT);
					wchar_t *w = (wchar_t *)GlobalLock(h);
					Insert_String(w);
					GlobalUnlock(h);
					CloseClipboard();
				}
			}
			return true;
			break;
		}
		return Old_On_Key_Down(key_id,key_data);
	}
	return Old_On_Key_Down(key_id,key_data);
}

void EditCtrlClass::Set_Caret_Pos(int new_pos)
{
	HilightStartPos = -1;
	if (CaretPos != new_pos)
	{
		CaretPos = new_pos;
		IsCaretDisplayed = true;
		LastCaretBlink = DialogMgrClass::Get_Time();
		Update_Scroll_Pos();
		Set_Dirty(true);
	}
}

int EditCtrlClass::Get_Caret_Pos() const
{
	if (mIME && mInComposition)
	{
		return CaretPos + mIME->GetCompositionCursorPos();
	}
	else
	{
		return CaretPos;
	}
}


void EditCtrlClass::Set_Sel(int start, int end)
{
	HilightAnchorPos = start;
	HilightStartPos = start;
	HilightEndPos = end;
	Set_Dirty(true);
}
void EditCtrlClass::Get_Sel(int& start, int& end) const
{
	start = HilightStartPos;
	end = HilightEndPos;
}
EditCtrlClass::EditCtrlClass() : IsCaretDisplayed(false), CaretBlinkDelay(500), LastCaretBlink(0), CaretPos(0), ScrollPos(0), NumChars(0), TextLimit(65535), HilightAnchorPos(-1), HilightStartPos(-1), HilightEndPos(-1), WasButtonPressedOnMe(false), mInComposition(false)
{
	StyleMgrClass::Assign_Font(&TextRenderer,StyleMgrClass::FONT_CONTROLS);
	StyleMgrClass::Configure_Renderer(&ControlRenderer);
	StyleMgrClass::Configure_Renderer(&CaretRenderer);
	StyleMgrClass::Configure_Renderer(&HilightRenderer);
	mIME = DialogMgrClass::Get_IME();
}
void EditCtrlClass::Create_Control_Renderers()
{
	ControlRenderer.Reset();
	ControlRenderer.Enable_Texturing(false);
	int linecol = StyleMgrClass::Get_Line_Color();
	int bkcol = StyleMgrClass::Get_Bk_Color();
	if (!IsEnabled)
	{
		linecol = StyleMgrClass::Get_Disabled_Line_Color();
		bkcol = StyleMgrClass::Get_Disabled_Bk_Color();
	}
	ControlRenderer.Add_Rect(Rect,1.0,linecol,bkcol);
}
RENEGADE_FUNCTION
void EditCtrlClass::Create_Caret_Renderer()
AT1(0x004F61C0);
RENEGADE_FUNCTION
void EditCtrlClass::Create_Text_Renderers()
AT1(0x004F5720);
void EditCtrlClass::Render(void)
{
	if (IsDirty)
	{
		Create_Control_Renderers();
		Create_Text_Renderers();
		Create_Caret_Renderer();
	}
	ControlRenderer.Render();
	TextRenderer.Render();
	HilightRenderer.Render();
	Update_Caret();
	if (HasFocus)
	{
		if (IsCaretDisplayed)
		{
			CaretRenderer.Render();
		}
	}
	IsDirty = false;
}
void EditCtrlClass::Update_Caret()
{
	if ((DialogMgrClass::Get_Time() - LastCaretBlink) > CaretBlinkDelay)
	{
		LastCaretBlink = DialogMgrClass::Get_Time();
		IsCaretDisplayed = !IsCaretDisplayed;
	}
}
RENEGADE_FUNCTION
void EditCtrlClass::Set_Text(const WCHAR *title)
AT1(0x004F7730);
void EditCtrlClass::On_LButton_Down(const Vector2 &mouse_pos)
{
	if (!mInComposition)
	{
		Set_Capture();
		if (HasFocus)
		{
			Set_Caret_Pos(Character_From_Pos(mouse_pos));
		}
		HilightAnchorPos = CaretPos;
		HilightStartPos = -1;
		HilightEndPos = -1;
		Set_Dirty(true);
		WasButtonPressedOnMe = true;
	}
}
void EditCtrlClass::On_LButton_Up(const Vector2 &mouse_pos)
{
	if (!mInComposition)
	{
		Release_Capture();
		WasButtonPressedOnMe = false;
	}
}
void EditCtrlClass::On_Mouse_Move(const Vector2 &mouse_pos)
{
	if (!mInComposition)
	{
		if (HasFocus)
		{
			if (WasButtonPressedOnMe)
			{
				Set_Caret_Pos(Character_From_Pos(mouse_pos));
			}
			int ap = HilightAnchorPos;
			int cp = CaretPos;
			if (HilightAnchorPos >= cp)
			{
				ap = cp;
			}
			HilightStartPos = ap;
			if (HilightAnchorPos > cp)
			{
				cp = HilightAnchorPos;
			}
			HilightEndPos = cp;
			Set_Dirty(true);
		}
	}
}
void EditCtrlClass::On_Set_Cursor(const Vector2 &mouse_pos)
{
	if ((mouse_pos.X > ClientRect.Left) && (mouse_pos.X < ClientRect.Right) && (mouse_pos.Y > ClientRect.Top) && (mouse_pos.Y < ClientRect.Bottom))
	{
		MouseMgrClass::Set_Cursor(MouseMgrClass::CURSOR_TEXT);
	}
}
RENEGADE_FUNCTION
void EditCtrlClass::On_Set_Focus(void)
AT1(0x004F64C0);
RENEGADE_FUNCTION
void EditCtrlClass::On_Kill_Focus(DialogControlClass *focus)
AT1(0x004F65C0);
RENEGADE_FUNCTION
void EditCtrlClass::On_Unicode_Char(WCHAR unicode)
AT1(0x004F6CB0);
void EditCtrlClass::On_Create(void)
{
}
RENEGADE_FUNCTION
void EditCtrlClass::Update_Client_Rect(void)
AT1(0x004F5BF0);
RENEGADE_FUNCTION
void EditCtrlClass::HandleNotification(IME::CompositionEvent&)
AT1(0x004F7BB0);
RENEGADE_FUNCTION
void EditCtrlClass::HandleNotification(IME::CandidateEvent&)
AT1(0x004F7D70);
EditCtrlClass::~EditCtrlClass()
{
	if (mIME)
	{
		mIME->Release_Ref();
	}
}
int EditCtrlClass::Get_Text_Length() const
{
	return NumChars;
}
void EditCtrlClass::Set_Text_Limit(int numChars)
{
	if (numChars <= 0)
	{
		TextLimit = -1;
	}
	else
	{
		TextLimit = numChars;
	}
}
int EditCtrlClass::Get_Text_Limit() const
{
	return TextLimit;
}
int EditCtrlClass::Get_Int()
{
	return _wtoi(Title);
}
void EditCtrlClass::Set_Int(int value)
{
	WideStringClass str;
	str.Format(L"%d",value);
	Set_Text(str);
}
RENEGADE_FUNCTION
int EditCtrlClass::Character_From_Pos(const Vector2 &mouse_pos)
AT1(0x004F6300);
RENEGADE_FUNCTION
void EditCtrlClass::Update_Scroll_Pos()
AT1(0x004F74D0);
