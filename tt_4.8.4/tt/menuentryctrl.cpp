#include "General.h"
#include "menuentryctrl.h"
#include "ww3d.h"
#include "stylemgr.h"
#include "mousemgr.h"
#include "dialogbase.h"
#include "dialogmgr.h"
#include "engine_math.h"
unsigned int MenuHiliteColor = 0xFF000000;

unsigned int MenuStaticGlowColor =			GetColorUInt(255,  9,  0, 0); 
unsigned int MenuActiveGlowColor =			GetColorUInt(255, 16,  0, 0);
unsigned int MenuPushedBaseGlowColor =		GetColorUInt(255, 48,  0, 0);
unsigned int MenuPushedHighlightGlowColor =	GetColorUInt(255, 48, 29, 0);

MenuEntryCtrlClass::MenuEntryCtrlClass() : CurrRadiusX(5), WasButtonPressedOnMe(false), IsMouseOverMe(false), CurrState(0), CurrRadiusY(5), StartTime(0), EndTime(0)
{
	CurrColor = MenuStaticGlowColor;
	StyleMgrClass::Assign_Font(&TextRenderer,StyleMgrClass::FONT_MENU);
	StyleMgrClass::Assign_Font(&GlowRenderer,StyleMgrClass::FONT_MENU);
}

void MenuEntryCtrlClass::Create_Glow(int radiusx,int radiusy,int color)
{
	GlowRenderer.Reset_Polys();
	StyleMgrClass::JUSTIFICATION justify = StyleMgrClass::CENTER_JUSTIFY;
	if ((Style & 0xF00) == BS_LEFT)
	{
		justify = StyleMgrClass::LEFT_JUSTIFY;
	}
	StyleMgrClass::Render_Glow(Title,&GlowRenderer,Rect,radiusx,radiusy,color,justify);
}

void MenuEntryCtrlClass::Create_Text_Renderer()
{
	TextRenderer.Reset_Polys();
	GlowRenderer.Reset_Polys();
	Vector2 ret = TextRenderer.Get_Text_Extents(Title);
	int X = (int)((((Rect.Right - Rect.Left) * 0.5) + Rect.Left) - (ret.X * 0.5));
	int Y = (int)((((Rect.Bottom - Rect.Top) * 0.5) + Rect.Top) - (ret.Y * 0.5));
	if ((Style & 0xF00) == BS_LEFT)
	{
		X = (int)(Rect.Left + 1);
	}
	if (CurrState == UP)
	{
		Create_Glow(CurrRadiusX,CurrRadiusY,MenuStaticGlowColor);
		Vector2 v = Vector2((float)X,(float)Y);
		TextRenderer.Set_Location(v);
		TextRenderer.Draw_Sentence(StyleMgrClass::Get_Text_Color());
	}
	else if (CurrState == DOWN)
	{
		Create_Glow(CurrRadiusX,CurrRadiusY,CurrColor);
		Vector2 v = Vector2((float)(X+1),(float)(Y+1));
		TextRenderer.Set_Location(v);
		TextRenderer.Draw_Sentence(0xFF000000);
		v = Vector2((float)X,(float)Y);
		TextRenderer.Set_Location(v);
		TextRenderer.Draw_Sentence(0xFF000000);
	}
	else if (CurrState == HILIGHT)
	{
		Create_Glow(CurrRadiusX,CurrRadiusY,CurrColor);
		Vector2 v = Vector2((float)(X-1),(float)(Y-1));
		TextRenderer.Set_Location(v);
		TextRenderer.Draw_Sentence(StyleMgrClass::Get_Text_Color());
		v = Vector2((float)X,(float)Y);
		TextRenderer.Set_Location(v);
		TextRenderer.Draw_Sentence(MenuHiliteColor);
	}
}

void MenuEntryCtrlClass::Render(void)
{
	if (Is_Dirty())
	{
		Create_Text_Renderer();
	}
	GlowRenderer.Render();
	TextRenderer.Render();
	Set_Dirty(false);
}

void MenuEntryCtrlClass::Center_Mouse(void)
{
	Vector2 ret = TextRenderer.Get_Text_Extents(Title);
	int X;
	if ((Style & 0xF00) == BS_LEFT)
	{
		X = (int)(((ret.X * 0.5) + Rect.Left));
	}
	else
	{
		X = (int)(((((Rect.Right - Rect.Left) * 0.5) + Rect.Left) - (ret.X * 0.5)));
	}
	int Y = (int)(((Rect.Bottom - Rect.Top) * 0.5) + Rect.Top);
	Vector3 mousepos = DialogMgrClass::Get_Mouse_Pos();
	mousepos.X = (float)X;
	mousepos.Y = (float)Y;
	DialogMgrClass::Set_Mouse_Pos(mousepos);
}

void MenuEntryCtrlClass::On_LButton_Down(const Vector2 &mouse_pos)
{
	Set_Capture();
	WasButtonPressedOnMe = true;
	if ((mouse_pos.X > Rect.Left) && (mouse_pos.X < Rect.Right) && (mouse_pos.Y > Rect.Top) && (mouse_pos.Y < Rect.Bottom))
	{
		IsMouseOverMe = true;
		Set_State(DOWN);
	}
	else
	{
		IsMouseOverMe = false;
	}
}

void MenuEntryCtrlClass::On_LButton_Up(const Vector2 &mouse_pos)
{
	Release_Capture();
	WasButtonPressedOnMe = false;
	if ((mouse_pos.X > Rect.Left) && (mouse_pos.X < Rect.Right) && (mouse_pos.Y > Rect.Top) && (mouse_pos.Y < Rect.Bottom))
	{
		IsMouseOverMe = true;
	}
	else
	{
		IsMouseOverMe = false;
	}
	if (CurrState != DOWN)
	{
		if (Has_Focus())
		{
			Set_State(HILIGHT);
		}
		else
		{
			Set_State(UP);
		}
	}
}

void MenuEntryCtrlClass::Set_State(int new_state)
{
	if (CurrState != new_state)
	{
		CurrState = new_state;
		Set_Dirty(true);
	}
	switch (CurrState)
	{
	case UP:
		CurrRadiusX = 5;
		CurrRadiusY = 5;
		break;
	case DOWN:
		StartTime = DialogMgrClass::Get_Time();
		EndTime = DialogMgrClass::Get_Time() + 300;
		CurrColor = MenuActiveGlowColor;
		StyleMgrClass::Play_Sound(StyleMgrClass::EVENT_MOUSE_CLICK);
		break;
	case HILIGHT:
		StartTime = DialogMgrClass::Get_Time();
		EndTime = DialogMgrClass::Get_Time() + 1000;
		CurrColor = MenuActiveGlowColor;
		break;
	}
}

void MenuEntryCtrlClass::Update_State()
{
	int currentTime = DialogMgrClass::Get_Time();

	switch (CurrState)
	{

	case DOWN:

		if (currentTime < EndTime)
		{
			CurrRadiusX = (int)((currentTime - StartTime) / (float)(EndTime - StartTime) * 155.f + 5.f);
			CurrRadiusY = (int)((currentTime - StartTime) / (float)(EndTime - StartTime) * 25.f + 5.f);
			

			Vector3 startColor(LOBYTE(HIWORD(MenuPushedBaseGlowColor)),
								HIBYTE(LOWORD(MenuPushedBaseGlowColor)),
								LOBYTE(LOWORD(MenuPushedBaseGlowColor)));
			Vector3 highlightColor(LOBYTE(HIWORD(MenuPushedHighlightGlowColor)),
								HIBYTE(LOWORD(MenuPushedHighlightGlowColor)),
								LOBYTE(LOWORD(MenuPushedHighlightGlowColor)));
			//float intensity = MaxHilightRedValue * 3.f;
			//Vector3 startColor(intensity, 0, 0);
			//Vector3 highlightColor(intensity, intensity * .6f, 0);
			Vector3 endColor(0, 0, 0);
			Vector3 colorVector;
			
			int highlightTime = (StartTime + EndTime) / 2;

			if (currentTime < highlightTime)
				Vector3::Lerp(startColor, highlightColor, (currentTime - StartTime) / (float)(highlightTime - StartTime), &colorVector);
			else
				Vector3::Lerp(highlightColor, endColor, (currentTime - highlightTime) / (float)(EndTime - highlightTime), &colorVector);

			CurrColor = GetColorUInt(255, (int)colorVector.X, (int)colorVector.Y, (int)colorVector.Z);
			
			Set_Dirty(true);
		}
		else
		{
			if (CurrRadiusX != 160)
			{
				CurrRadiusX = 160;
				CurrRadiusY = 60;
				CurrColor = GetColorUInt(255, 0, 0, 0);
			}
			
			On_Pushed();
			Set_State(HasFocus ? HILIGHT : UP);
		}

		break;
	
	case HILIGHT:

		if (currentTime >= EndTime)
		{
			CurrRadiusX = 5;
			CurrRadiusY = 5;
			CurrColor = MenuActiveGlowColor;
			StartTime = currentTime;
			EndTime = currentTime + 1000;
		}
		else
		{
			int radius = (int)lerp(5.f, 60.f, (currentTime - StartTime) / (float)(EndTime - StartTime));
			CurrRadiusX = radius;
			CurrRadiusY = radius;
			
			int pulseTime = EndTime - 500;
			if (currentTime > pulseTime)
			{
				Vector3 startColor(LOBYTE(HIWORD(MenuActiveGlowColor)),
					HIBYTE(LOWORD(MenuActiveGlowColor)),
					LOBYTE(LOWORD(MenuActiveGlowColor)));
				Vector3 endColor(0, 0, 0);
				Vector3 colorVector;

				Vector3::Lerp(startColor, endColor, (currentTime - pulseTime) / (float)(EndTime - pulseTime), &colorVector);

				CurrColor = GetColorUInt(255, (int)colorVector.X, (int)colorVector.Y, (int)colorVector.Z);
			}
			
			Set_Dirty(true);
		}

		break;

	}
}

bool MenuEntryCtrlClass::On_Key_Down(uint32 key_id, uint32 key_data)
{
	switch (key_id)
	{
	case VK_RETURN:
		Set_State(DOWN);
		break;
	case VK_UP:
		{
			DialogControlClass *d = Parent->Find_Next_Group_Control(this,-1);
			if (d)
			{
				d->Set_Focus();
			}
		}
		break;
	case VK_DOWN:
		{
			DialogControlClass *d = Parent->Find_Next_Group_Control(this,1);
			if (d)
			{
				d->Set_Focus();
			}
		}
		break;
	}
	return true;
}

void MenuEntryCtrlClass::On_Mouse_Move(const Vector2 &mouse_pos)
{
	Set_Focus();
	if ((mouse_pos.X > Rect.Left) && (mouse_pos.X < Rect.Right) && (mouse_pos.Y > Rect.Top) && (mouse_pos.Y < Rect.Bottom))
	{
		IsMouseOverMe = true;
	}
	else
	{
		IsMouseOverMe = false;
	}
}

void MenuEntryCtrlClass::On_Set_Cursor(const Vector2 &mouse_pos)
{
	if ((mouse_pos.X > Rect.Left) && (mouse_pos.X < Rect.Right) && (mouse_pos.Y > Rect.Top) && (mouse_pos.Y < Rect.Bottom))
	{
		MouseMgrClass::Set_Cursor(MouseMgrClass::CURSOR_ACTION);
	}
	else
	{
		MouseMgrClass::Set_Cursor(MouseMgrClass::CURSOR_ARROW);
	}
}
void MenuEntryCtrlClass::On_Create(void)
{
	if ((Style & 0xF) == BS_OWNERDRAW)
	{
		StyleMgrClass::Assign_Font(&TextRenderer,StyleMgrClass::FONT_SM_MENU);
		StyleMgrClass::Assign_Font(&GlowRenderer,StyleMgrClass::FONT_SM_MENU);
	}
	TextRenderer.Build_Sentence(Title);
	GlowRenderer.Build_Sentence(Title);
	Vector2 ret = TextRenderer.Get_Text_Extents(Title);
	Rect = MaxRect;
	if ((Style & 0xF00) != BS_LEFT)
	{
		int X = (int)((((Rect.Right - Rect.Left) * 0.5) + Rect.Left) - (ret.X * 0.5));
		Rect.Left = (float)X;
	}
	Vector2 extents = TextRenderer.Get_Text_Extents(L"W");
	Rect.Right = extents.X + Rect.Left + ret.X;
}
void MenuEntryCtrlClass::On_Set_Focus(void)
{
	if (!WasButtonPressedOnMe)
	{
		Set_State(HILIGHT);
		StyleMgrClass::Play_Sound(StyleMgrClass::EVENT_MOUSE_OVER);
	}
	HasFocus = true;
}
void MenuEntryCtrlClass::On_Kill_Focus(DialogControlClass *)
{
	if (CurrState != DOWN)
	{
		Set_State(UP);
	}
	WasButtonPressedOnMe = false;
	HasFocus = false;
}
bool MenuEntryCtrlClass::On_Key_Up(uint32 key_id)
{
	return false;
}
void MenuEntryCtrlClass::On_Mouse_Wheel(int direction)
{
	if (HasFocus)
	{
		DialogControlClass *d = Parent->Find_Next_Group_Control(this,direction);
		if (d)
		{
			d->Set_Focus();
			d->Center_Mouse();
		}
	}
}
void MenuEntryCtrlClass::On_Frame_Update(void)
{
	Update_State();
}
void MenuEntryCtrlClass::Update_Client_Rect(void)
{
	MaxRect = Rect;
	Set_Dirty(true);
}
MenuEntryCtrlClass::~MenuEntryCtrlClass()
{
}



void MenuEntryCtrlClass::On_Pushed()
{
	Parent->On_Command(ID, 1, 0);
}
