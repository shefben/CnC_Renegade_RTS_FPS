#include "General.h"
#include "WW3DAssetManager.h"
#include "buttonctrl.h"
#include "stylemgr.h"
#include "mousemgr.h"
#include "dialogbase.h"
#include "dialogmgr.h"
void ButtonCtrlClass::Set_Bitmap(const char *texture1,const char *texture2)
{
	TextureClass *tex1 = WW3DAssetManager::TheInstance->Get_Texture(texture1,TextureClass::MIP_LEVELS_1,WW3D_FORMAT_UNKNOWN,true);
	ButtonRenderers[0].Set_Texture(tex1);
	if (texture2 && *texture2)
	{
		TextureClass *tex2 = WW3DAssetManager::TheInstance->Get_Texture(texture2,TextureClass::MIP_LEVELS_1,WW3D_FORMAT_UNKNOWN,true);
		ButtonRenderers[1].Set_Texture(tex2);
		if (tex2)
		{
			tex2->Release_Ref();
		}
	}
	else
	{
		ButtonRenderers[1].Set_Texture(tex1);
	}
	if (tex1)
	{
		tex1->Release_Ref();
	}
	Style |= 0x80;
}
ButtonCtrlClass::ButtonCtrlClass() : WasButtonPressedOnMe(false), IsMouseOverMe(false), PulseTime(0)
{
	StyleMgrClass::Assign_Font(&TextRenderer,StyleMgrClass::FONT_CONTROLS);
	StyleMgrClass::Assign_Font(&GlowRenderer,StyleMgrClass::FONT_CONTROLS);
	StyleMgrClass::Configure_Renderer(&ButtonRenderers[0]);
	StyleMgrClass::Configure_Renderer(&ButtonRenderers[1]);
}
void ButtonCtrlClass::On_LButton_Down(const Vector2 &mouse_pos)
{
	Set_Capture();
	WasButtonPressedOnMe = true;
	StyleMgrClass::Play_Sound(StyleMgrClass::EVENT_MOUSE_CLICK);
}
void ButtonCtrlClass::On_LButton_Up(const Vector2 &mouse_pos)
{
	if ((mouse_pos.X > Rect.Left) && (mouse_pos.X < Rect.Right) && (mouse_pos.Y > Rect.Top) && (mouse_pos.Y < Rect.Bottom))
	{
		IsMouseOverMe = true;
	}
	else
	{
		IsMouseOverMe = false;
	}
	if (WasButtonPressedOnMe)
	{
		if (IsMouseOverMe)
		{
			Parent->On_Command(ID,0,0);
		}
	}
	WasButtonPressedOnMe = false;
}
void ButtonCtrlClass::On_Mouse_Move(const Vector2 &mouse_pos)
{
	if ((mouse_pos.X > Rect.Left) && (mouse_pos.X < Rect.Right) && (mouse_pos.Y > Rect.Top) && (mouse_pos.Y < Rect.Bottom))
	{
		IsMouseOverMe = true;
	}
	else
	{
		IsMouseOverMe = false;
	}
}
void ButtonCtrlClass::On_Set_Cursor(const Vector2 &mouse_pos)
{
	if ((mouse_pos.X > ClientRect.Left) && (mouse_pos.X < ClientRect.Right) && (mouse_pos.Y > ClientRect.Top) && (mouse_pos.Y < ClientRect.Bottom))
	{
		MouseMgrClass::Set_Cursor(MouseMgrClass::CURSOR_ACTION);
	}
}
void ButtonCtrlClass::On_Create(void)
{
	GlowRenderer.Build_Sentence(Title);
}
void ButtonCtrlClass::On_Kill_Focus(DialogControlClass *focus)
{
	WasButtonPressedOnMe = false;
	IsMouseOverMe = false;
	HasFocus = false;
}
bool ButtonCtrlClass::On_Key_Down(uint32 key_id, uint32 key_data)
{
	if ((key_id == VK_RETURN) || (key_id == VK_SPACE))
	{
		Parent->On_Command(ID,0,0);
		StyleMgrClass::Play_Sound(StyleMgrClass::EVENT_MOUSE_CLICK);
		return 1;
	}
	return 0;
}
void ButtonCtrlClass::On_Frame_Update(void)
{
	Vector3 mouse_pos = DialogMgrClass::Get_Mouse_Pos();
	if ((mouse_pos.X > ClientRect.Left) && (mouse_pos.X < ClientRect.Right) && (mouse_pos.Y > ClientRect.Top) && (mouse_pos.Y < ClientRect.Bottom))
	{
		Update_Pulse(true);
	}
	else
	{
		Update_Pulse(false);
	}
}
void ButtonCtrlClass::Create_Text_Renderers(void)
{
	TextRenderer.Reset();
	GlowRenderer.Reset_Polys();
	StyleMgrClass::Render_Text(Title,&TextRenderer,Rect,true,true,StyleMgrClass::CENTER_JUSTIFY,IsEnabled,true);
	StyleMgrClass::Render_Glow(Title,&GlowRenderer,Rect,8,8,StyleMgrClass::Get_Tab_Glow_Color(),StyleMgrClass::CENTER_JUSTIFY);
}
RENEGADE_FUNCTION
void ButtonCtrlClass::Create_Component_Button(void)
AT1(0x004FA0A0);
void ButtonCtrlClass::Create_Bitmap_Button(void)
{
	int color = 0xFFFFFFFF;
	if (!IsEnabled)
	{
		color = 0xFF606060;
	}
	ButtonRenderers[UP].Reset();
	ButtonRenderers[DOWN].Reset();
	ButtonRenderers[UP].Add_Quad(Rect,color);
	ButtonRenderers[DOWN].Add_Quad(Rect,color);
}
void ButtonCtrlClass::Render(void)
{
	if (Is_Dirty())
	{
		Create_Text_Renderers();
		if (Style & BS_BITMAP)
		{
			Create_Bitmap_Button();
		}
		else
		{
			Create_Component_Button();
		}
	}
	if (WasButtonPressedOnMe)
	{
		if (IsMouseOverMe)
		{
			ButtonRenderers[DOWN].Render();
		}
	}
	ButtonRenderers[UP].Render();
	if (!(Style & BS_BITMAP))
	{
		if (HasFocus)
		{
			GlowRenderer.Render();
		}
		TextRenderer.Render();
	}
	IsDirty = false;
}
void ButtonCtrlClass::Update_Pulse(bool is_mouse_over)
{
	int pt = PulseTime;
	if (pt != 1000 || is_mouse_over)
	{
		int color;
		if (IsEnabled)
		{
			if (is_mouse_over)
			{
				int newpt = pt - DialogMgrClass::Get_Frame_Time();
				PulseTime = newpt;
				if (newpt < 0)
				{
					PulseTime = newpt + 1000;
				}
				int col = (int)(((cos((PulseTime * 0.001) * 6.2831855) * 0.2) + 0.80000001) * 255);
				color = RGB(col,col,col) | 0xFF000000;
			}
			else
			{
				color = 0xFFFFFFFF;
				PulseTime = 1000;
			}
		}
		else
		{
			color = 0xFF606060;
		}
		ButtonRenderers[UP].Force_Color(color);
		ButtonRenderers[DOWN].Force_Color(color);
	}
}
