#include "General.h"
#include "dialogtext.h"
#include "stylemgr.h"
extern unsigned int dialogtextcolor;
extern unsigned int dialogglowcolor;
DialogTextClass::DialogTextClass() : IsTitle(false)
{
	Set_Wants_Focus(false);
	StyleMgrClass::Assign_Font(&TextRenderer,StyleMgrClass::FONT_CONTROLS);
	StyleMgrClass::Configure_Renderer(&ControlRenderer);
}
void DialogTextClass::Render(void)
{
	if (Style & WS_VISIBLE)
	{
		if (IsDirty)
		{
			Create_Text_Renderer();
		}
		if (IsTitle)
		{
			GlowRenderer.Render();
		}
		TextRenderer.Render();
		ControlRenderer.Render();
		IsDirty = false;
	}
}
void DialogTextClass::On_Create(void)
{
	if (Title.Get_Length() >= 2)
	{
		if (Title[0] == '%')
		{
			if (Title[1] == 't')
			{
				WideStringClass temp(&Title.Peek_Buffer()[2]);
				Title = temp;
				IsTitle = true;
				StyleMgrClass::Assign_Font(&TextRenderer,StyleMgrClass::FONT_TITLE);
				StyleMgrClass::Assign_Font(&GlowRenderer,StyleMgrClass::FONT_TITLE);
				GlowRenderer.Build_Sentence(Title);
			}
			else if (Title[1] == 'h')
			{
				StyleMgrClass::Assign_Font(&TextRenderer,StyleMgrClass::FONT_LG_CONTROLS);
				WideStringClass temp(&Title.Peek_Buffer()[2]);
				Title = temp;
			}
			else if (Title[1] == 's')
			{
				StyleMgrClass::Assign_Font(&TextRenderer,StyleMgrClass::FONT_TOOLTIPS);
				WideStringClass temp(&Title.Peek_Buffer()[2]);
				Title = temp;
			}
		}
	}
}
void DialogTextClass::Create_Text_Renderer()
{
	TextRenderer.Reset();
	ControlRenderer.Reset();
	int style = Style & SS_TYPEMASK;
	if (style == SS_ETCHEDHORZ)
	{
		int color = StyleMgrClass::Get_Line_Color();
		if (!IsEnabled)
		{
			color = StyleMgrClass::Get_Disabled_Line_Color();
		}
		Vector2 v;
		v.X = Rect.Right;
		v.Y = Rect.Top;
		Vector2 v2;
		v2.X = Rect.Left;
		v2.Y = Rect.Top;
		ControlRenderer.Add_Line(v,v2,1.0,color);
	}
	else if (style == SS_BLACKFRAME)
	{
		int color = StyleMgrClass::Get_Line_Color();
		if (!IsEnabled)
		{
			color = StyleMgrClass::Get_Disabled_Line_Color();
		}
		ControlRenderer.Add_Outline(Rect,1.0,color);
	}
	else
	{
		StyleMgrClass::JUSTIFICATION justify = StyleMgrClass::LEFT_JUSTIFY;
		int style2 = style & 0xF;
		if (style2 == SS_RIGHT)
		{
			justify = StyleMgrClass::RIGHT_JUSTIFY;
		}
		else if (style2 == SS_CENTER)
		{
			justify = StyleMgrClass::CENTER_JUSTIFY;
		}
		bool center = (Style & SS_CENTERIMAGE) == SS_CENTERIMAGE;
		if (IsTitle)
		{
			GlowRenderer.Reset_Polys();
			StyleMgrClass::Render_Glow(Title,&GlowRenderer,Rect,5,5,dialogglowcolor,StyleMgrClass::CENTER_JUSTIFY);
			StyleMgrClass::Render_Text(Title,&TextRenderer,dialogtextcolor,0,Rect,false,false,StyleMgrClass::CENTER_JUSTIFY,true);
		}
		else
		{
			if (style == SS_LEFTNOWORDWRAP)
			{
				if (IsTextColorOverridden)
				{
					StyleMgrClass::Render_Text(Title,&TextRenderer,RGB(TextColor.Z * 255,TextColor.Y * 255,TextColor.X * 255)|0xFF000000,0xFF000000,Rect,true,true,justify,center);
				}
				else
				{
					StyleMgrClass::Render_Text(Title,&TextRenderer,Rect,true,true,justify,IsEnabled,center);
				}
			}
			else
			{
				if (IsTextColorOverridden)
				{
					StyleMgrClass::Render_Wrapped_Text_Ex(Title,&TextRenderer,RGB(TextColor.Z * 255,TextColor.Y * 255,TextColor.X * 255)|0xFF000000,0xFF000000,Rect,true,center,justify);
				}
				else
				{
					StyleMgrClass::Render_Wrapped_Text_Ex(Title,&TextRenderer,Rect,true,center,IsEnabled,justify);
				}
			}
		}
	}
}