/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat																		  *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwui/stylemgr.h           $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 1/26/02 1:21p                                              $*
 *                                                                                             *
 *                    $Revision:: 16                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef __STYLE_MGR_H
#define __STYLE_MGR_H

#include "engine_string.h"
#include "engine_vector.h"


////////////////////////////////////////////////////////////////
//	Forward declarations
////////////////////////////////////////////////////////////////
class TextureClass;
class Render2DClass;
class Render2DSentenceClass;
class FontCharsClass;
class RectClass;


////////////////////////////////////////////////////////////////
//
//	StyleMgrClass
//
////////////////////////////////////////////////////////////////

class StyleMgrClass
{
public:

	////////////////////////////////////////////////////////////////
	//	Public constants
	////////////////////////////////////////////////////////////////

	typedef enum
	{
		FONT_TITLE			= 0,
		FONT_LG_CONTROLS,
		FONT_CONTROLS,
		FONT_LISTS,
		FONT_TOOLTIPS,
		FONT_MENU,
		FONT_SM_MENU,
		FONT_HEADER,
		FONT_BIG_HEADER,
		FONT_CREDITS,
		FONT_CREDITS_BOLD,
		FONT_INGAME_TXT,
		FONT_INGAME_BIG_TXT,
		FONT_INGAME_SUBTITLE_TXT,
		FONT_INGAME_HEADER_TXT,
		FONT_MAX
	} FONT_STYLE;

	typedef enum
	{
		LEFT_JUSTIFY		= 0,
		RIGHT_JUSTIFY,
		CENTER_JUSTIFY

	} JUSTIFICATION;


	typedef enum
	{
		EVENT_MOUSE_CLICK		= 0,
		EVENT_MOUSE_OVER,
		EVENT_MENU_BACK,
		EVENT_POPUP,
		EVENT_AUDIO_MAX

	} EVENT_AUDIO;


	////////////////////////////////////////////////////////////////
	//	Public methods
	////////////////////////////////////////////////////////////////

	//
	//	Initialization
	//
	static void				Initialize (void);
	static void				Initialize_From_INI (const char *filename);
	static void				Shutdown (void);

	//
	//	Font methods
	//
	static FontCharsClass *			Get_Font (FONT_STYLE style);
	static FontCharsClass *			Peek_Font (FONT_STYLE style)	{ return Fonts[style]; }
	static void							Assign_Font (Render2DSentenceClass *renderer, FONT_STYLE style);

	//
	//	Sound methods
	//
	static void							Play_Sound (EVENT_AUDIO event);

	//
	//	Configuration methods
	//
	static void				Configure_Renderer (Render2DClass *renderer);

	//
	//	Scale support
	//
	static float			Get_X_Scale (void)	{ return ScaleX; }
	static float			Get_Y_Scale (void)	{ return ScaleY; }

	//
	//	Color methods
	//
	static uint32			Get_Text_Color (void)						{ return TextColor; }
	static uint32			Get_Text_Shadow_Color (void)				{ return TextShadowColor; }
	static uint32			Get_Disabled_Text_Color (void)			{ return DisabledTextColor; }
	static uint32			Get_Disabled_Text_Shadow_Color (void)	{ return DisabledTextShadowColor; }
	static uint32			Get_Line_Color (void)						{ return LineColor; }
	static uint32			Get_Bk_Color (void)							{ return BkColor; }
	static uint32			Get_Disabled_Line_Color (void)			{ return DisabledLineColor; }
	static uint32			Get_Disabled_Bk_Color (void)				{ return DisabledBkColor; }
	static uint32			Get_Tab_Text_Color (void)					{ return TabTextColor; }
	static uint32			Get_Tab_Glow_Color (void)					{ return TabGlowColor; }

	//
	//	Backdrop support
	//
	static void				Render_Backdrop (Render2DClass *renderer, const RectClass &rect);
	
	//
	//	Text support
	//
	static void				Render_Text (const WCHAR *text, Render2DSentenceClass *renderer, uint32 text_color, uint32 shadow_color, const RectClass &rect, bool do_shadow = false, bool do_clip = true, JUSTIFICATION justify = LEFT_JUSTIFY, bool is_vcentered = true);
	static void				Render_Text (const WCHAR *text, Render2DSentenceClass *renderer, const RectClass &rect, bool do_shadow = false, bool do_clip = true, JUSTIFICATION justify = LEFT_JUSTIFY, bool is_enabled = true, bool is_vcentered = true);
	static void				Render_Title_Text (const WCHAR *text, Render2DSentenceClass *renderer, const RectClass &rect);
	static void				Render_Wrapped_Text (const WCHAR *text, Render2DSentenceClass *renderer, uint32 text_color, uint32 shadow_color, const RectClass &rect, bool do_shadow = false, bool do_vcenter = false);
	static void				Render_Wrapped_Text (const WCHAR *text, Render2DSentenceClass *renderer, const RectClass &rect, bool do_shadow = false, bool do_vcenter = false, bool is_enabled = true);	
	static void				Render_Wrapped_Text_Ex (const WCHAR *text, Render2DSentenceClass *renderer, const RectClass &rect, bool do_shadow = false, bool do_vcenter = false, bool is_enabled = true, JUSTIFICATION justify = LEFT_JUSTIFY);
	static void				Render_Wrapped_Text_Ex (const WCHAR *text, Render2DSentenceClass *renderer, uint32 text_color, uint32 shadow_color, const RectClass &rect, bool do_shadow = false, bool do_vcenter = false, JUSTIFICATION justify = LEFT_JUSTIFY);

	//
	//	Hilight support
	//
	static void				Configure_Hilighter (Render2DClass *renderer);
	static void				Render_Hilight (Render2DClass *renderer, const RectClass &rect);

	//
	//	Text "glow" support
	//
	static void				Render_Glow (const WCHAR *text, Render2DSentenceClass *renderer, const RectClass &rect, int radius_x, int radius_y, int color, JUSTIFICATION justify = LEFT_JUSTIFY);

	//making public so that the hud.ini code can set them.
	static REF_DECL1(TitleColor, uint32);
	static REF_DECL1(TitleHilightColor, uint32);
	static REF_DECL1(TitleShadowColor, uint32);
	static REF_DECL1(TextColor, uint32);
	static REF_DECL1(TextShadowColor, uint32);
	static REF_DECL1(LineColor, uint32);
	static REF_DECL1(BkColor, uint32);
	static REF_DECL1(DisabledTextColor, uint32);
	static REF_DECL1(DisabledTextShadowColor, uint32);
	static REF_DECL1(DisabledLineColor, uint32);
	static REF_DECL1(DisabledBkColor, uint32);
	static REF_DECL1(HilightColor, uint32);
	static REF_DECL1(TabTextColor, uint32);
	static REF_DECL1(TabGlowColor, uint32);
private:

	////////////////////////////////////////////////////////////////
	//	Private member data
	////////////////////////////////////////////////////////////////	
	static REF_ARR_DECL1(Fonts, FontCharsClass *, 15);
	static REF_DECL1(ScaleX, float);
	static REF_DECL1(ScaleY, float);
	static DynamicVectorClass<StringClass>	FontFileList;
	static StringClass				EventAudioList[EVENT_AUDIO_MAX];
};

#endif //__STYLE_MGR_H

