#include "General.h"
#include "stylemgr.h"
#include "render2d.h"
#include "WWAudioClass.h"
#include "WW3DAssetManager.h"

REF_DEF1(StyleMgrClass::TitleColor, uint32, 0x007FCF84);
REF_DEF1(StyleMgrClass::TitleHilightColor, uint32, 0x007FCF88);
REF_DEF1(StyleMgrClass::TitleShadowColor, uint32, 0x007FCF8C);
REF_DEF1(StyleMgrClass::TextColor, uint32, 0x007FCF90);
REF_DEF1(StyleMgrClass::TextShadowColor, uint32, 0x007FCF94);
REF_DEF1(StyleMgrClass::LineColor, uint32, 0x007FCF98);
REF_DEF1(StyleMgrClass::BkColor, uint32, 0x007FCF9C);
REF_DEF1(StyleMgrClass::DisabledTextColor, uint32, 0x007FCFA0);
REF_DEF1(StyleMgrClass::DisabledTextShadowColor, uint32, 0x007FCFA4);
REF_DEF1(StyleMgrClass::DisabledLineColor, uint32, 0x007FCFA8);
REF_DEF1(StyleMgrClass::DisabledBkColor, uint32, 0x007FCFAC);
REF_DEF1(StyleMgrClass::HilightColor, uint32, 0x007FCFB0);
REF_DEF1(StyleMgrClass::TabTextColor, uint32, 0x007FCFB4);
REF_DEF1(StyleMgrClass::TabGlowColor, uint32, 0x007FCFB8);
REF_DEF1(StyleMgrClass::ScaleX, float, 0x007FCFBC);
REF_DEF1(StyleMgrClass::ScaleY, float, 0x007FCFC0);
REF_ARR_DEF1(StyleMgrClass::Fonts, FontCharsClass *, 15, 0x0082FE20);
DynamicVectorClass<StringClass> StyleMgrClass::FontFileList;
StringClass StyleMgrClass::EventAudioList[StyleMgrClass::EVENT_AUDIO_MAX];
void StyleMgrClass::Play_Sound(EVENT_AUDIO event)
{
	if (WWAudioClass::_theInstance && !EventAudioList[event].Is_Empty())
	{
		StringClass str = EventAudioList[event];
		StringClass sname = strtok(str.Peek_Buffer(),",");
		float volume = atoi(strtok(NULL,",")) * 0.0099999998f;
		WWAudioClass::_theInstance->Simple_Play_2D_Sound_Effect(sname,1.0,volume);
	}
}

RENEGADE_FUNCTION
void StyleMgrClass::Render_Text(const WCHAR *text, Render2DSentenceClass *renderer, uint32 text_color, uint32 shadow_color, const RectClass &rect, bool do_shadow, bool do_clip, JUSTIFICATION justify, bool is_vcentered)
AT1(0x004E8990);

void StyleMgrClass::Render_Text(const WCHAR *text, Render2DSentenceClass *renderer, const RectClass &rect, bool do_shadow, bool do_clip, JUSTIFICATION justify, bool is_enabled, bool is_vcentered)
{
	uint32 TextColor = StyleMgrClass::TextColor;
	uint32 ShadowColor = StyleMgrClass::TextShadowColor;
	if (!is_enabled)
	{
		TextColor = StyleMgrClass::DisabledTextColor;
		ShadowColor = StyleMgrClass::DisabledTextShadowColor;
	}
	StyleMgrClass::Render_Text(text,renderer,TextColor,ShadowColor,rect,do_shadow,do_clip,justify,is_vcentered);
}
void StyleMgrClass::Configure_Renderer(Render2DClass *renderer)
{
	renderer->Set_Coordinate_Range(Render2DClass::Get_Screen_Resolution());
}

void StyleMgrClass::Shutdown()
{
	for (int i = 0;i < FONT_MAX;i++)
	{
		REF_PTR_RELEASE(Fonts[i]);
	}
	for (int i = 0;i < FontFileList.Count();i++)
	{
		RemoveFontResource(FontFileList[i]);
	}
}

void StyleMgrClass::Configure_Hilighter(Render2DClass *renderer)
{
	renderer->Enable_Alpha(false);
	renderer->Enable_Texturing(false);
	renderer->Enable_Additive(true);
}
RENEGADE_FUNCTION
void StyleMgrClass::Render_Glow (const WCHAR *text, Render2DSentenceClass *renderer, const RectClass &rect, int radius_x, int radius_y, int color, JUSTIFICATION justify)
AT1(0x004E8EE0);
void StyleMgrClass::Render_Hilight (Render2DClass *renderer, const RectClass &rect)
{
	renderer->Add_Quad(rect,StyleMgrClass::HilightColor);
}
void StyleMgrClass::Render_Wrapped_Text_Ex (const WCHAR *text, Render2DSentenceClass *renderer, const RectClass &rect, bool do_shadow, bool do_vcenter, bool is_enabled, JUSTIFICATION justify)
{
	uint32 TextColor = StyleMgrClass::TextColor;
	uint32 ShadowColor = StyleMgrClass::TextShadowColor;
	if (!is_enabled)
	{
		TextColor = StyleMgrClass::DisabledTextColor;
		ShadowColor = StyleMgrClass::DisabledTextShadowColor;
	}
	Render_Wrapped_Text_Ex(text,renderer,TextColor,ShadowColor,rect,do_shadow,do_vcenter,justify);
}
void StyleMgrClass::Render_Wrapped_Text_Ex (const WCHAR *text, Render2DSentenceClass *renderer, uint32 text_color, uint32 shadow_color, const RectClass &rect, bool do_shadow, bool do_vcenter, JUSTIFICATION justify)
{
	renderer->Set_Wrapping_Width(rect.Width());
	renderer->Set_Clipping_Rect(rect);
	int row_count;
	Vector2 v = renderer->Get_Formatted_Text_Extents(text,&row_count);
	int char_height = renderer->Peek_Font()->Get_Char_Height();
	RectClass newrect(rect);
	if (do_vcenter)
	{
		newrect.Top = ((rect.Top + rect.Bottom) * 0.5f) - (row_count * char_height * 0.5f);
	}
	const WCHAR *row = renderer->Find_Row_Start(text,0);
	if (row)
	{
		const WCHAR *row2 = 0;
		do
		{
			row2 = renderer->Find_Row_Start(row,1);
			WideStringClass str;
			if (row2)
			{
				int len = (((char *)row2) - ((char *)row)) >> 1;
				wchar_t *buf = str.Get_Buffer(len + 1);
				memcpy(buf,row,((char *)row2) - ((char *)row));
				buf[len] = 0;
			}
			else if (row)
			{
				str = row;
			}
			StyleMgrClass::Render_Text(str,renderer,text_color,shadow_color,newrect,do_shadow,false,justify,false);
			row = row2;
			newrect.Top += char_height;
		} while (row2);
	}
}

void StyleMgrClass::Assign_Font(Render2DSentenceClass *renderer,FONT_STYLE style)
{
	renderer->Set_Font(Fonts[style]);
}

void StyleMgrClass::Initialize_From_INI(const char *filename)
{
	char *FontNames[FONT_MAX] = {"FONT_TITLE","FONT_LG_CONTROLS","FONT_CONTROLS","FONT_LISTS","FONT_TOOLTIPS","FONT_MENU","FONT_SM_MENU","FONT_HEADER","FONT_BIG_HEADER","FONT_CREDITS","FONT_CREDITS_BOLD","FONT_INGAME_TXT","FONT_INGAME_BIG_TXT","FONT_INGAME_SUBTITLE_TXT","FONT_INGAME_HEADER_TXT"};
	Shutdown();

	const RectClass &screen_rect = Render2DClass::Get_Screen_Resolution();

	float screen_ar_width = 0;
	float screen_ar_height = 0;

	if (screen_rect.Width() / screen_rect.Height() <= 4.0f / 3.0f)
	{
		// smaller than 4:3, use width as primary
		screen_ar_width = screen_rect.Width();
		screen_ar_height = screen_ar_width * 3.0f / 4.0f;
	}
	else
	{
		// larger than 4:3, use height as primary
		screen_ar_height = screen_rect.Height();
		screen_ar_width =  screen_ar_height * 4.0f / 3.0f;
	}

	ScaleX = screen_ar_width / 800.0f;
	ScaleY = screen_ar_height / 600.0f;

	INIClass *ini = Get_INI(filename);
	int count = ini->Entry_Count("Font File List");
	for (int i = 0;i < count;i++)
	{
		StringClass fontname;
		ini->Get_String(fontname,"Font File List",ini->Get_Entry("Font File List",i),0);
		AddFontResource(fontname);
		FontFileList.Add(fontname);
	}
	for (int i = 0; i < FONT_MAX; i++)
	{
		StringClass font;
		ini->Get_String(font, "Font Names", FontNames[i]);
		StringClass fname = strtok(font.Peek_Buffer(), ",");
		int ptsize = (int)(atoi(strtok(NULL, ",")) * ScaleY);
		bool bold = atoi(strtok(NULL, ","));
		if (ptsize < 8)
			ptsize = 8;
		
		if (ptsize < 10 && ScaleY < 1)
			bold = false;
		
		Fonts[i] = WW3DAssetManager::TheInstance->Get_FontChars(fname,ptsize,bold);
	}
	ini->Get_String(EventAudioList[0],"Audio","AUDIO_CLICK");
	ini->Get_String(EventAudioList[1],"Audio","AUDIO_MOUSEOVER");
	ini->Get_String(EventAudioList[2],"Audio","AUDIO_BACK");
	ini->Get_String(EventAudioList[3],"Audio","AUDIO_POPUP");
	Release_INI(ini);
}
