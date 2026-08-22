#include "General.h"
#include "HUDClass.h"
#include "engine_3d.h"
#include "WeaponClass.h"
#include "WeaponBagClass.h"
#include "cPlayer.h"
#include "render2d.h"
#include "CombatManager.h"
#include "CCameraClass.h"
#include "HUDInfo.h"
#include "ReferencerClass.h"
#include "SoldierGameObj.h"
#include "HUDGlobalSettingsDef.h"
#include "RadarManager.h"
#include "sniperhud.h"
#include "TimeManager.h"
#include "wwmath.h"
#include "BuildingGameObj.h"
#include "TranslateDBClass.h"
#include "string_ids.h"
#include "VehicleGameObj.h"
#include "texture.h"
#include "WW3DAssetManager.h"
#include "ObjectiveManager.h"
#include "objective.h"
#include "WeaponManager.h"
#include "cGameType.h"
extern bool HideHealthText;
extern bool HideBulletText;
extern bool HideClipText;
extern bool HideShieldText;
extern bool HideWeaponText;
extern bool HideWeaponImage;
extern bool HideWeaponBox;
extern bool HideInfoBox;
extern bool HidePowerupIcons;
extern bool HideCenter;
extern unsigned int WeaponChartColor;
extern unsigned int WeaponChartEmptyColor;
static void Generate_WChar_Text_From_Number(WCHAR* text,int digits,int min_digits,int value)
{
	text[digits]=0;
	while (digits)
	{
		digits--;
		text[digits]=value%10+'0';
		value/=10;
		min_digits--;
		if (value==0 && (min_digits == 0)) break;
	}
	if ((min_digits==0) && digits)
	{
		int i=0;
		while (WCHAR c=text[digits++])
		{
			text[i++]=c;
		}
		text[i]=0;
	}
}
Vector2	INFO_UV_SCALE( 1.0f/256.0f, 1.0f/256.0f );
Vector2 INFO_OFFSET( 7, -179 );
Vector2 FRAME_1_UV_UL( 96, 105 );
Vector2 FRAME_1_UV_LR( 214, 255 );
Vector2 FRAME_1_OFFSET( -3, -1 );
Vector2 FRAME_2_UV_UL( 215, 125 );
Vector2 FRAME_2_UV_LR( 255, 192 );
Vector2 FRAME_2_OFFSET( 114, 57 );
Vector2 FRAME_3_UV_UL( 218, 192 );
Vector2 FRAME_3_UV_LR( 255, 201 );
Vector2 FRAME_3_OFFSET( 154, 115 );
Vector2 FRAME_4_UV_UL( 216, 200 );
Vector2 FRAME_4_UV_LR( 255, 255 );
Vector2 FRAME_4_OFFSET( 191, 115 );
Vector2 FRAME_5_UV_UL( 80, 203 );
Vector2 FRAME_5_UV_LR( 100, 258 );
Vector2 FRAME_5_OFFSET( 230, 116 );
Vector2 FRAME_6_UV_UL( 216, 101 );
Vector2 FRAME_6_UV_LR( 240, 125 );
Vector2 FRAME_6_OFFSET( 74, 149 );
Vector2 HEALTH_BACK_UV_UL( 183, 241 );
Vector2 HEALTH_BACK_UV_LR( 186, 248 );
Vector2 HEALTH_BACK_UL( 98, 122 );
Vector2 HEALTH_BACK_LR( 224, 168 );
Vector2 GRADIENT_BLACK_UV_UL( 3, 135 );
Vector2 GRADIENT_BLACK_UV_LR( 44, 144 );
Vector2 HEALTH_TEXT_BACK_UL( 77, 124 );
Vector2 HEALTH_TEXT_BACK_LR( 163, 150 );
Vector2 HEALTH_UV_UL( 94, 52 );
Vector2 HEALTH_UV_LR( 249, 100 );
Vector2 HEALTH_OFFSET( 73, 121 );
Vector2 SHIELD_UV_UL( 66, 97 );
Vector2 SHIELD_UV_LR( 96, 132 );
Vector2 SHIELD_OFFSET( 211, 140 );
Vector2 KEY_1_UV_UL( 30, 180 );
Vector2 KEY_1_UV_LR( 57, 197 );
Vector2 KEY_1_OFFSET( 32, 134 );
Vector2 KEY_2_UV_UL( 0, 181 );
Vector2 KEY_2_UV_LR( 30, 197 );
Vector2 KEY_2_OFFSET( 41, 140 );
Vector2 KEY_3_UV_UL( 69, 133 );
Vector2 KEY_3_UV_LR( 97, 149 );
Vector2 KEY_3_OFFSET( 50, 148 );
Vector2 HEALTH_CROSS_1_UV_UL( 33, 199 );
Vector2 HEALTH_CROSS_1_UV_LR( 63, 226 );
Vector2 HEALTH_CROSS_1_OFFSET( 77, 124 );
Vector2 HEALTH_CROSS_2_UV_UL( 33, 228 );
Vector2 HEALTH_CROSS_2_UV_LR( 63, 258 );
Vector2 HEALTH_CROSS_2_OFFSET( 77, 124 );
Vector2 TIME_BACK_UL( 150, 97 );
Vector2 TIME_BACK_LR( 213, 116 );
Vector2 TIME_CLOCK_UV_UL( 46, 125 );
Vector2 TIME_CLOCK_UV_LR( 66, 148 );
Vector2 TIME_CLOCK_OFFSET( 186, 94 );
Vector2 WEAPON_BOX_UV_UL( 0, 0 );
Vector2 WEAPON_BOX_UV_LR( 95, 95 );
Vector2 WEAPON_BOX_OFFSET( 195, -53 );
Vector2 POWER_OUT_ICON_UV_UL( 0, 95 );
Vector2 POWER_OUT_ICON_UV_LR( 30, 129 );
Vector2 POWER_OUT_ICON_OFFSET( 143, 41 );
Vector2 REPAIR_ICON_UV_UL( 31, 97 );
Vector2 REPAIR_ICON_UV_LR( 61, 126 );
Vector2 REPAIR_ICON_OFFSET( 181, 43 );
Vector2 GDI_ICON_UV_UL( 14, 222 );
Vector2 GDI_ICON_UV_LR( 34, 238 );
Vector2 GDI_ICON_OFFSET( 217, 77 );
Vector2 NOD_ICON_UV_UL( 14, 205 );
Vector2 NOD_ICON_UV_LR( 34, 221 );
Vector2 NOD_ICON_OFFSET( 238, 76 );
Vector2 NEUTRAL_ICON_UV_UL( 14, 238 );
Vector2 NEUTRAL_ICON_UV_LR( 33, 257 );
Vector2 NEUTRAL_ICON_OFFSET( 260, 77 );
Vector2 BULLET_ICON_UV_UL( 2, 211 );
Vector2 BULLET_ICON_UV_LR( 13, 255 );
Vector2 BULLET_ICON_OFFSET( -20, -30 );
Vector2 RADAR_CENTER_OFFSET( 55, 78 );
Vector2 DAMAGE_1_UV_UL( 65, 184 );
Vector2 DAMAGE_1_UV_LR( 78, 255 );
Vector2 DAMAGE_2_UV_UL( 200, 3 );
Vector2 DAMAGE_2_UV_LR( 248, 51 );
Vector2 HORIZ_DAMAGE_SIZE( 81, 14 );
Vector2 VERT_DAMAGE_SIZE( 15, 78 );
Vector2 DIAG_DAMAGE_SIZE( 53, 59 );
Vector2 HV_DAMAGE_OFFSET( 170, 168 );
Vector2 DIAG_DAMAGE_OFFSET( 119, 117 );
Vector2 POWERUP_BOX_UV_UL( 50, 1 );
Vector2 POWERUP_BOX_UV_LR( 127, 52 );
Vector2 RADAR_RINGS_UV_UL( 95, 0 );
Vector2 RADAR_RINGS_UV_LR( 197, 53 );
Vector2 RADAR_RINGS_L_OFFSET( -51, -50 );
Vector2 RADAR_RINGS_R_OFFSET( 0, -50 );
Vector2 RADAR_STAR_UV_UL( 241, 103 );
Vector2 RADAR_STAR_UV_LR( 249, 111 );
Vector2 RADAR_STAR_OFFSET( 280, 50 );
Vector2 RADAR_SQUARE_UV_UL( 247, 85 );
Vector2 RADAR_SQUARE_UV_LR( 255, 93 );
Vector2 RADAR_SQUARE_OFFSET( 290, 50 );
Vector2 RADAR_TRIANGLE_UV_UL( 247, 93 );
Vector2 RADAR_TRIANGLE_UV_LR( 255, 101 );
Vector2 RADAR_TRIANGLE_OFFSET( 300, 50 );
Vector2 RADAR_CIRCLE_UV_UL( 247, 77 );
Vector2 RADAR_CIRCLE_UV_LR( 255, 85 );
Vector2 RADAR_CIRCLE_OFFSET( 310, 50 );
Vector2 RADAR_BRACKET_UV_UL( 241, 114 );
Vector2 RADAR_BRACKET_UV_LR( 249, 122 );
Vector2 RADAR_BRACKET_OFFSET( 320, 50 );
Vector2 RADAR_SWEEP_UV_UL( 80, 182 );
Vector2 RADAR_SWEEP_UV_LR( 95, 191 );
Vector2 RADAR_SWEEP_OFFSET( 330, 50 );
Vector2 TARGET_HEALTH_L_UV_UL( 0, 165 );
Vector2 TARGET_HEALTH_L_UV_LR( 20, 181 );
Vector2 TARGET_HEALTH_R_UV_UL( 20, 165 );
Vector2 TARGET_HEALTH_R_UV_LR( 96, 173 );
Vector2 TARGET_HEALTH_OFFSET( 123, 5 );
Vector2 TARGET_NAME_UV_UL( 1, 149 );
Vector2 TARGET_NAME_UV_LR( 91, 164 );
Vector2 TARGET_NAME_OFFSET( 125, 24 );
Vector2 TARGET_ENTERABLE_UV_UL( 45, 209 );
Vector2 TARGET_ENTERABLE_UV_LR( 51, 215 );
Vector2 TARGET_ENTERABLE_SIZE( 32, 32 );
float	  TARGET_ENTERABLE_BOUNCE	 = 4;
#define	RETICLE_WIDTH	(64.0f/640.0f)
#define	RETICLE_HEIGHT	(64.0f/480.0f)
#define		LARGE_FONT		"FONT12x16.TGA"
#define		MEDIUM_FONT		"FONT12x16.TGA"
#define		SMALL_FONT		"FONT6x8.TGA"
enum
{
	HUD_HELP_TEXT_DISPLAYING = 0,
	HUD_HELP_TEXT_FADING,
	HUD_HELP_TEXT_DONE
};
#define		HUD_MAIN_TEXTURE		"HUD_MAIN.TGA"
#define		HUD_CHATPBOX_TEXTURE	"HUD_CHATPBOX.TGA"
#define		HUD_WEAPONS_TEXTURE	"hud_TibRVoltR.tga"
Render2DClass *			InfoRenderer;
Render2DClass *			CenterRenderer;
Render2DTextClass *			CenterTextRenderer;
Render2DSentenceClass * HUDHelpTextRenderer;
Vector2						HUDHelpTextExtents (0, 0);
float							HUDHelpTextTimer = 0;
int							HUDHelpTextState = HUD_HELP_TEXT_DISPLAYING;
unsigned long COLOR( float alpha, unsigned long color = 0x00FFFFFF )
{
	alpha = WWMath::Clamp( alpha, 0, 1 ) * 255.0f;
	color &= 0x00FFFFFF;
	color |= ((int)alpha) << 24;
	return color;
}
unsigned long Get_Health_Color( float percent )
{
	Vector3	color = HUDGlobalSettingsDef::Get_Instance()->Get_Health_High_Color();
	if ( percent <= 0.5f )
	{
		color = HUDGlobalSettingsDef::Get_Instance()->Get_Health_Med_Color();
	}
	if ( percent <= 0.25f )
	{
		color = HUDGlobalSettingsDef::Get_Instance()->Get_Health_Low_Color();
	}
	return color.Convert_To_ARGB();
}
struct PowerupIconStruct
{
	PowerupIconStruct( void ) : Renderer( NULL ), Number( 0 )	{}
	~PowerupIconStruct( void )
	{
		if ( Renderer != NULL )
		{
			delete Renderer;
			Renderer = NULL;
		}
	}
	WideStringClass	Name;
	int				Number;
	Render2DClass * Renderer;
	RectClass		UV;
	RectClass		IconBox;
	float			Timer;
};
static	DynamicVectorClass<PowerupIconStruct*>	RightPowerupIconList;
static	DynamicVectorClass<PowerupIconStruct*>	LeftPowerupIconList;
Render2DClass * PowerupBoxRenderer;
Render2DSentenceClass * PowerupTextRenderer;
#define		MAX_ICONS		5
#define		POWERUP_TIME	6
static	void	Powerup_Init( void )
{
	PowerupBoxRenderer = new Render2DClass();
	PowerupBoxRenderer->Set_Texture( HUD_CHATPBOX_TEXTURE );
	PowerupBoxRenderer->Set_Coordinate_Range( Render2DClass::Get_Screen_Resolution() );
	PowerupBoxRenderer->Enable_Additive( true );
	FontCharsClass * font = Fonts[11];
	PowerupTextRenderer = new Render2DSentenceClass();
	PowerupTextRenderer->Set_Font( font );
}
static	void	Powerup_Shutdown( void )
{
	delete PowerupBoxRenderer;
	PowerupBoxRenderer = NULL;
	delete PowerupTextRenderer;
	PowerupTextRenderer = NULL;
	int i;
	for ( i = 0; i < LeftPowerupIconList.Count(); i++ )
	{
		delete LeftPowerupIconList[i];
	}
	LeftPowerupIconList.Delete_All();
	for ( i = 0; i < RightPowerupIconList.Count(); i++ )
	{
		delete RightPowerupIconList[i];
	}
	RightPowerupIconList.Delete_All();
}
static	void	Powerup_Add( const WCHAR * name, int number, const char * texture_name, const RectClass & uv, const Vector2 & offset, bool right_list = true )
{
	PowerupIconStruct * data = new PowerupIconStruct();
	data->Renderer = new Render2DClass();
	StringClass new_name(true);
	Strip_Path_From_Filename( new_name, texture_name );
	data->Renderer->Set_Texture_Init( new_name );
	data->Renderer->Set_Coordinate_Range( Render2DClass::Get_Screen_Resolution() );
	float size = (float)data->Renderer->Peek_Texture()->Width;
	data->UV = uv;
	if ( size > 0 )
	{
		data->UV.Scale( Vector2( 1/size, 1/size ) );
	}
	data->IconBox = uv;
	data->IconBox += offset + Vector2( 0, -40.0f ) - data->IconBox.Upper_Left();
	data->Name = name;
	data->Number = number;
 	data->Timer = POWERUP_TIME;
	if ( right_list )
	{
		RightPowerupIconList.Add( data );
	}
	else
	{
		LeftPowerupIconList.Add( data );
	}
}
void 	Powerup_Reset( void )
{
	while ( LeftPowerupIconList.Count() )
	{
		delete LeftPowerupIconList[0];
		LeftPowerupIconList.Delete(0);
	}
	while ( RightPowerupIconList.Count() )
	{
		delete RightPowerupIconList[0];
		RightPowerupIconList.Delete(0);
	}
}
#define		POWERUP_BOX_BASE		POWERUP_BOX_WIDTH + 6, 112
#define		POWERUP_BOX_WIDTH		80
#define		POWERUP_BOX_HEIGHT		55
#define		POWERUP_BOX_SPACING		(POWERUP_BOX_HEIGHT + 10)
#define		POWERUP_BOX_UV			127,0,207,55
static	void	Powerup_Update( void )
{
	PowerupBoxRenderer->Reset();
	PowerupTextRenderer->Reset();
	RectClass box_uv( POWERUP_BOX_UV_UL, POWERUP_BOX_UV_LR );
	box_uv.Scale( Vector2( 1.0f/128.0f, 1.0f/128.0f ) );
#define	ANIMATE_TIME	1.0f
	static float LeftAnimateTimer = 0;
	static float RightAnimateTimer = 0;
	if ( LeftPowerupIconList.Count() > 0 && LeftPowerupIconList[0]->Timer < 0 )
	{
		LeftAnimateTimer += TimeManager::FrameSeconds;
		if ( LeftAnimateTimer > ANIMATE_TIME )
		{
			LeftAnimateTimer = 0;
			if ( LeftPowerupIconList.Count() )
			{
				delete LeftPowerupIconList[0];
				LeftPowerupIconList.Delete(0);
			}
		}
	}
	else
	{
		LeftAnimateTimer = 0;
	}
	if ( RightPowerupIconList.Count() > 0 && RightPowerupIconList[0]->Timer < 0 )
	{
		RightAnimateTimer += TimeManager::FrameSeconds;
		if ( RightAnimateTimer > ANIMATE_TIME )
		{
			RightAnimateTimer = 0;
			if ( RightPowerupIconList.Count() )
			{
				delete RightPowerupIconList[0];
				RightPowerupIconList.Delete(0);
			}
		}
	}
	else
	{
		RightAnimateTimer = 0;
	}
 	RectClass box( POWERUP_BOX_UV_UL, POWERUP_BOX_UV_LR );
	Vector2 start = Render2DClass::Get_Screen_Resolution().Lower_Right() - Vector2( POWERUP_BOX_BASE );
	box += start - box.Lower_Left();
	box -= Vector2( box.Left - 6, +75 );
	int i;
	for ( i = 0; i < MAX_ICONS && i < LeftPowerupIconList.Count(); i++ )
	{
		LeftPowerupIconList[i]->Timer -= TimeManager::FrameSeconds;
		RectClass draw_box = box;
		int green = 0xFF00FF00;
		int white = 0xFFFFFFFF;
		if ( i == 0 && LeftAnimateTimer > 0.0f )
		{
			green = COLOR( WWMath::Clamp( 1.0f - (LeftAnimateTimer/ANIMATE_TIME), 0, 1 ), green ) ;
			white = COLOR( WWMath::Clamp( 1.0f - (LeftAnimateTimer/ANIMATE_TIME), 0, 1 ), white ) ;
		}
		PowerupTextRenderer->Build_Sentence( LeftPowerupIconList[i]->Name );
		PowerupTextRenderer->Set_Location( Vector2( draw_box.Left + 1, draw_box.Top + POWERUP_BOX_HEIGHT - 15 ) );
		PowerupTextRenderer->Draw_Sentence( white );
		RectClass	icon_box = LeftPowerupIconList[i]->IconBox;
		icon_box += draw_box.Upper_Left();
		LeftPowerupIconList[i]->Renderer->Reset();
		LeftPowerupIconList[i]->Renderer->Add_Quad( icon_box, LeftPowerupIconList[i]->UV, white );
		if ( i == 0 && LeftAnimateTimer > ANIMATE_TIME * 0.5f )
		{
			box += Vector2( 0, ((2*LeftAnimateTimer/ANIMATE_TIME)-1) * POWERUP_BOX_SPACING );
		}
		box -= Vector2( 0, POWERUP_BOX_SPACING );
	}
 	box = RectClass( POWERUP_BOX_UV_UL, POWERUP_BOX_UV_LR );
	start = Render2DClass::Get_Screen_Resolution().Lower_Right() - Vector2( POWERUP_BOX_BASE );
	box += start - box.Lower_Left();
	for ( i = 0; i < MAX_ICONS && i < RightPowerupIconList.Count(); i++ )
	{
		RightPowerupIconList[i]->Timer -= TimeManager::FrameSeconds;
		RectClass draw_box = box;
		int green = 0xFF00FF00;
		int white = 0xFFFFFFFF;
		if ( i == 0 && RightAnimateTimer > 0.0f )
		{
			green = COLOR( WWMath::Clamp( 1.0f - (RightAnimateTimer/ANIMATE_TIME), 0, 1 ), green ) ;
			white = COLOR( WWMath::Clamp( 1.0f - (RightAnimateTimer/ANIMATE_TIME), 0, 1 ), white ) ;
		}
		PowerupTextRenderer->Build_Sentence( RightPowerupIconList[i]->Name );
		float left_edge = draw_box.Left + 1;
		Vector2 extents = PowerupTextRenderer->Get_Text_Extents( RightPowerupIconList[i]->Name );
		if ( left_edge + extents.X + 1> Render2DClass::Get_Screen_Resolution().Right )
		{
			left_edge = Render2DClass::Get_Screen_Resolution().Right - extents.X - 1;
		}
		PowerupTextRenderer->Set_Location( Vector2( left_edge, draw_box.Top + POWERUP_BOX_HEIGHT - 15 ) );
		PowerupTextRenderer->Draw_Sentence( white );
		if ( RightPowerupIconList[i]->Number != 0 )
		{
			WideStringClass num(0,true);
			num.Format( L"%d", RightPowerupIconList[i]->Number );
			PowerupTextRenderer->Build_Sentence( num );
			PowerupTextRenderer->Set_Location( Vector2( draw_box.Right - 12, draw_box.Top + 1 ) );
			PowerupTextRenderer->Draw_Sentence( white );
		}
		RectClass	icon_box = RightPowerupIconList[i]->IconBox;
		icon_box += draw_box.Upper_Left();
		RightPowerupIconList[i]->Renderer->Reset();
		RightPowerupIconList[i]->Renderer->Add_Quad( icon_box, RightPowerupIconList[i]->UV, green );
		if ( i == 0 && RightAnimateTimer > ANIMATE_TIME * 0.5f )
		{
			box += Vector2( 0, ((2*RightAnimateTimer/ANIMATE_TIME)-1) * POWERUP_BOX_SPACING );
		}
		box -= Vector2( 0, POWERUP_BOX_SPACING );
	}
}
static	void	Powerup_Render( void )
{
	if (!HidePowerupIcons)
	{
		int i;
		for ( i = 0; i < MAX_ICONS && i < LeftPowerupIconList.Count(); i++ )
		{
			LeftPowerupIconList[i]->Renderer->Render();
		}
		for ( i = 0; i < MAX_ICONS && i < RightPowerupIconList.Count(); i++ )
		{
			RightPowerupIconList[i]->Renderer->Render();
		}
		PowerupTextRenderer->Render();
	}
}
Render2DClass * WeaponBoxRenderer;
Render2DClass * WeaponImageRenderer;
Render2DTextClass * WeaponClipCountRenderer;
Render2DTextClass * WeaponTotalCountRenderer;
Render2DSentenceClass * WeaponNameRenderer;
Vector2			WeaponBase;
WeaponClass *	_LastHUDWeapon = NULL;
int				_LastVehicleSeat = -1;
#define		WEAPON_OFFSET			100, 110
#define		SNIPER_UV				90,0,255,53
#define		SNIPER_OFFSET			-80,-4
static void HUD_Help_Text_Init( void )
{
	FontCharsClass *font = Fonts[12];
	HUDHelpTextRenderer = new Render2DSentenceClass;
	HUDHelpTextRenderer->Set_Font( font );
	HUDHelpTextExtents.Set (0, 0);
	HUDHelpTextTimer = 0;
	HUDHelpTextState = HUD_HELP_TEXT_DISPLAYING;
	return ;
}
static	void	HUD_Help_Text_Render( void )
{
	const float HUD_HELP_TEXT_FADE_TIME = 2.0F;
	const float HUD_HELP_TEXT_DISPLAY_TIME = 2.0F;
	const WideStringClass &string = HUDInfo::Get_HUD_Help_Text();
	bool is_empty = string.Is_Empty();
	if (HUDInfo::Is_HUD_Help_Text_Dirty())
	{
		HUDHelpTextState = HUD_HELP_TEXT_DISPLAYING;
		HUDInfo::Set_Is_HUD_Help_Text_Dirty( false );
		HUDHelpTextRenderer->Reset();
		if (is_empty == false)
		{
			HUDHelpTextRenderer->Build_Sentence( string );
			HUDHelpTextTimer = HUD_HELP_TEXT_DISPLAY_TIME;
		}
		HUDHelpTextExtents = HUDHelpTextRenderer->Get_Text_Extents (string);
	}
	if ( is_empty == false )
	{
		HUDHelpTextRenderer->Reset_Polys();
		Vector2 reticle_offset = COMBAT_CAMERA->Get_Camera_Target_2D_Offset();
		const RectClass &screen_rect = Render2DClass::Get_Screen_Resolution();
		float x_pos = screen_rect.Center ().X - (HUDHelpTextExtents.X * 0.5F);
		float y_pos = screen_rect.Center ().Y - (HUDHelpTextExtents.Y);
		y_pos -= (RETICLE_HEIGHT * screen_rect.Height () * 0.25F);
		HUDHelpTextRenderer->Set_Location( Vector2( x_pos, y_pos ) );
		HUDHelpTextRenderer->Draw_Sentence( RGB_TO_INT32 (0, 0, 0) );
		HUDHelpTextRenderer->Set_Location( Vector2( x_pos-1, y_pos-1 ) );
		HUDHelpTextRenderer->Draw_Sentence( VRGB_TO_INT32 (HUDInfo::Get_HUD_Help_Text_Color ()) );
		if (HUDHelpTextState == HUD_HELP_TEXT_FADING)
		{
			float percent = WWMath::Clamp ((HUDHelpTextTimer / HUD_HELP_TEXT_FADE_TIME), 0.0F, 1.0F);
			HUDHelpTextRenderer->Force_Alpha (percent * 0.5F);
		}
		else
		{
			HUDHelpTextRenderer->Force_Alpha (0.5F);
		}
		HUDHelpTextRenderer->Render();
		HUDHelpTextTimer -= TimeManager::FrameSeconds;
		if (HUDHelpTextTimer <= 0)
		{
			HUDHelpTextState ++;
			if (HUDHelpTextState >= HUD_HELP_TEXT_DONE)
			{
				HUDInfo::Set_HUD_Help_Text( L"" );
				HUDHelpTextRenderer->Reset();
			}
			else if (HUDHelpTextState == HUD_HELP_TEXT_FADING)
			{
				HUDHelpTextTimer = HUD_HELP_TEXT_FADE_TIME;
			}
		}
	}
}
static	void	HUD_Help_Text_Shutdown( void )
{
	delete HUDHelpTextRenderer;
	HUDHelpTextRenderer = NULL;
	return ;
}
static	void	Weapon_Init( void )
{
	WeaponBoxRenderer = new Render2DClass();
	WeaponBoxRenderer->Set_Texture( HUD_MAIN_TEXTURE );
	WeaponBoxRenderer->Set_Coordinate_Range( Render2DClass::Get_Screen_Resolution() );
	RectClass box_uv( WEAPON_BOX_UV_UL, WEAPON_BOX_UV_LR );
	RectClass draw_box = box_uv;
	box_uv.Scale( Vector2( 1.0f/256.0f, 1.0f/256.0f ) );
	draw_box += Render2DClass::Get_Screen_Resolution().Lower_Right() - Vector2( WEAPON_OFFSET ) - draw_box.Upper_Left();
	WeaponBoxRenderer->Add_Quad( draw_box, box_uv );
	WeaponBase = draw_box.Upper_Left();
	WeaponImageRenderer = new Render2DClass();
	WeaponImageRenderer->Set_Coordinate_Range( Render2DClass::Get_Screen_Resolution() );
	FontCharsClass *chars_font = Fonts[11];
	WeaponNameRenderer = new Render2DSentenceClass();
	WeaponNameRenderer->Set_Font( chars_font );
	Font3DInstanceClass * font = WW3DAssetManager::TheInstance->Get_Font3DInstance( LARGE_FONT );
	WeaponClipCountRenderer = new Render2DTextClass( font );
	WeaponClipCountRenderer->Set_Coordinate_Range( Render2DClass::Get_Screen_Resolution() );
	font->Release_Ref();
	font = WW3DAssetManager::TheInstance->Get_Font3DInstance( LARGE_FONT );
	CenterTextRenderer = new Render2DTextClass( font );
	CenterTextRenderer->Set_Coordinate_Range( Render2DClass::Get_Screen_Resolution() );
	font->Release_Ref();
	font = WW3DAssetManager::TheInstance->Get_Font3DInstance( SMALL_FONT );
	WeaponTotalCountRenderer = new Render2DTextClass( font );
	WeaponTotalCountRenderer->Set_Coordinate_Range( Render2DClass::Get_Screen_Resolution() );
	font->Release_Ref();
	_LastHUDWeapon = NULL;
	_LastVehicleSeat = -1;
}
static	void	Weapon_Shutdown( void )
{
	delete WeaponBoxRenderer;
	WeaponBoxRenderer = NULL;
	delete WeaponImageRenderer;
	WeaponImageRenderer = NULL;
	delete WeaponNameRenderer;
	WeaponNameRenderer = NULL;
	delete WeaponClipCountRenderer;
	WeaponClipCountRenderer = NULL;
	delete WeaponTotalCountRenderer;
	WeaponTotalCountRenderer = NULL;
}
#define		CLIP_ROUNDS_OFFSET		15,27
#define		TOTAL_ROUNDS_OFFSET		65,34
#define		WEAPON_NAME_OFFSET		-10, 95
float	LastClipCount = 0;
float	CenterClipCountTimer = 0;
const float	CENTER_CLIP_COUNT_TIME	= 2.0f;
enum
{
	SEAT_DRIVER = 0,
	SEAT_GUNNER,
	SEAT_PASENGER,
};
const char * _Seat_Textures[3] =
{
	"hud_driverseat.tga",
	"hud_gunseat.tga",
	"hud_passseat.tga",
};
static	void	Weapon_Reset( void ) 
{
	_LastHUDWeapon = (WeaponClass *)0xFFFFFFFF;
	_LastVehicleSeat = -1;
}
static	void	Weapon_Update( void ) 
{
	WeaponClass * weapon = NULL;
	if ( CombatManager::Get_The_Star())
	{
		weapon = CombatManager::Get_The_Star()->Get_Weapon();
		if ( CombatManager::Get_The_Star()->Get_Vehicle() )
		{
			weapon = CombatManager::Get_The_Star()->Get_Vehicle()->Get_Weapon();
		}
	}
	WeaponClipCountRenderer->Reset();
	WeaponTotalCountRenderer->Reset();
	if ( weapon != NULL )
	{
		WCHAR tmp_text[5];
		if ( weapon->Get_Clip_Rounds() == -1 )
		{
			tmp_text[0]='9';
			tmp_text[1]='9';
			tmp_text[2]='9';
			tmp_text[3]=0;
		}
		else
		{
			Generate_WChar_Text_From_Number(tmp_text,3,3,weapon->Get_Clip_Rounds());
		}
		WeaponClipCountRenderer->Set_Location( WeaponBase + Vector2( CLIP_ROUNDS_OFFSET ) );
		WeaponClipCountRenderer->Draw_Text( tmp_text );
		if ( LastClipCount != weapon->Get_Clip_Rounds() )
		{
			LastClipCount = (float)weapon->Get_Clip_Rounds();
			CenterClipCountTimer = CENTER_CLIP_COUNT_TIME;
		}
		if ( CenterClipCountTimer > 0 )
		{
			Vector2	center_clip_count_offset = Render2DClass::Get_Screen_Resolution().Center();
			center_clip_count_offset.X *= 1.5;
			float fade = WWMath::Clamp( CenterClipCountTimer, 0, 1 );
			RectClass uv;
			uv.Set( BULLET_ICON_UV_UL, BULLET_ICON_UV_LR );
			RectClass draw = uv;
			uv.Scale( INFO_UV_SCALE );
			draw += center_clip_count_offset + BULLET_ICON_OFFSET - draw.Upper_Left();
			CenterRenderer->Add_Quad( draw, uv, COLOR( fade ) );
			CenterTextRenderer->Set_Location( draw.Upper_Right() + Vector2( 4, 15 ) );
			CenterTextRenderer->Draw_Text( tmp_text, COLOR( fade ) );
			CenterClipCountTimer -= TimeManager::FrameSeconds;
		}
		if ( weapon->Get_Total_Rounds() == -1 )
		{
			tmp_text[0]='9';
			tmp_text[1]='9';
			tmp_text[2]='9';
			tmp_text[3]=0;
		}
		else
		{
			Generate_WChar_Text_From_Number(tmp_text,3,3,weapon->Get_Total_Rounds() - weapon->Get_Clip_Rounds());
		}
		WeaponTotalCountRenderer->Set_Location( WeaponBase + Vector2( TOTAL_ROUNDS_OFFSET ) );
		WeaponTotalCountRenderer->Draw_Text( tmp_text );
	}
	if ( CombatManager::Get_The_Star()->Get_Vehicle() )
	{
		int seat = SEAT_PASENGER;
		if ( CombatManager::Get_The_Star()->Get_Vehicle()->Get_Driver() == CombatManager::Get_The_Star() )
		{
			seat = SEAT_DRIVER;
		}
		else if ( CombatManager::Get_The_Star()->Get_Vehicle()->Get_Gunner() == CombatManager::Get_The_Star())
		{
			seat = SEAT_GUNNER;
		}
		if ( _LastVehicleSeat != seat )
		{
			_LastVehicleSeat = seat;
			_LastHUDWeapon = (WeaponClass *)0xFFFFFFFF;
			WeaponImageRenderer->Reset();
			StringClass filename = _Seat_Textures[seat];
			WeaponImageRenderer->Set_Texture( filename );
			Vector2		offset( 16, 34 );
			RectClass icon_box( 0,0,64,64 );
			icon_box += WeaponBase + offset - icon_box.Upper_Left();
			WeaponImageRenderer->Add_Quad( icon_box );
			WeaponNameRenderer->Reset();
			WideStringClass name(CombatManager::Get_The_Star()->Get_Vehicle()->Get_Vehicle_Name(),true);
			WeaponNameRenderer->Build_Sentence( name );
			Vector2 text_size = WeaponNameRenderer->Get_Text_Extents( name );
			WeaponNameRenderer->Set_Location( Render2DClass::Get_Screen_Resolution().Lower_Right() - text_size );
			WeaponNameRenderer->Draw_Sentence();
		}
	}
	else if ( _LastHUDWeapon != weapon )
	{
		_LastHUDWeapon = weapon;
		_LastVehicleSeat = -1;
		WeaponImageRenderer->Reset();
		WeaponNameRenderer->Reset();
		if ( weapon != NULL )
		{
			StringClass filename( HUD_WEAPONS_TEXTURE, true );
			RectClass	uv( 0,64,128,128 );
			Vector2		offset( -24, 38  );
			const WeaponDefinitionClass * def = weapon->Get_Definition();
			if ( !def->IconTextureName.Is_Empty() )
			{
				Strip_Path_From_Filename( filename, def->IconTextureName );
				uv = def->IconTextureUV;
				offset = def->IconOffset;
 			}
			RectClass icon_box = uv;
			icon_box += WeaponBase + offset - icon_box.Upper_Left();
			{
				StringClass new_name(true);
				Strip_Path_From_Filename( new_name, filename );
				WeaponImageRenderer->Set_Texture_Init( new_name );
			}
			float size = (float)WeaponImageRenderer->Peek_Texture()->Width;
			if ( size > 0 )
			{
				uv.Scale( Vector2( 1/size, 1/size ) );
			}
			int color = 0xFF00FF00;
			WeaponImageRenderer->Add_Quad( icon_box, uv, color );
			WideStringClass name(0,true);
			name = TranslateDBClass::Get_String( def->IconNameID );
			WeaponNameRenderer->Build_Sentence( name );
			Vector2 text_size = WeaponNameRenderer->Get_Text_Extents( name ) + Vector2( 1, 0 );
			WeaponNameRenderer->Set_Location( Render2DClass::Get_Screen_Resolution().Lower_Right() - text_size );
			WeaponNameRenderer->Draw_Sentence();
		}
	}
}
void Weapon_Render()
{
	if (!HideWeaponBox)
	{
		WeaponBoxRenderer->Render();
	}
	if (!HideWeaponImage)
	{
		WeaponImageRenderer->Render();
	}
	if (!HideWeaponText)
	{
		WeaponNameRenderer->Render();
	}
	if (!HideClipText)
	{
		WeaponClipCountRenderer->Render();
	}
	if (!HideBulletText)
	{
		WeaponTotalCountRenderer->Render();
	}
}
Render2DClass * WeaponChartBoxRenderer;
static	DynamicVectorClass<Render2DClass *>	WeaponChartIcons;
Render2DSentenceClass * WeaponChartKeynameRenderer;
float	WeaponChartTimer;
#define	WEAPON_CHART_TIME	3.0f
static	void	Weapon_Chart_Init( void )
{
	WeaponChartBoxRenderer = new Render2DClass();
	WeaponChartBoxRenderer->Enable_Texturing( false );
	WeaponChartBoxRenderer->Set_Coordinate_Range( Render2DClass::Get_Screen_Resolution() );
	FontCharsClass *chars_font = Fonts[11];
	WeaponChartKeynameRenderer = new Render2DSentenceClass();
	WeaponChartKeynameRenderer->Set_Font( chars_font );
}
static	void	Weapon_Chart_Shutdown( void )
{
	delete WeaponChartBoxRenderer;
	WeaponChartBoxRenderer = NULL;
	for ( int i = 0; i < WeaponChartIcons.Count(); i++)
	{
		delete WeaponChartIcons[i];
		WeaponChartIcons[i] = NULL;
	}
	WeaponChartIcons.Delete_All();
	delete WeaponChartKeynameRenderer;
	WeaponChartKeynameRenderer = NULL;
}
Vector2	WeaponChartBase( 0.11f, 0.05f );
Vector2	WeaponChartSpacing( 0.075f, 0.05f );
float		WeaponChartIconScale = 0.45f / 640.0f;
static	void	Clear_Weapon_Chart_Icons( void )
{
	if ( WeaponChartIcons.Count() != 0 )
	{
		int i;
		for ( i = 0; i < WeaponChartIcons.Count(); i++)
		{
			delete WeaponChartIcons[i];
			WeaponChartIcons[i] = NULL;
		}
		WeaponChartIcons.Delete_All();
		WeaponChartKeynameRenderer->Reset();
	}
}

RENEGADE_FUNCTION
int Get_Primary_Key_For_Function(int) //only necessary because shaders cant call ttinit
AT1(0x006763C0);
RENEGADE_FUNCTION
void Get_Translated_Key_Name(int keycode,WideStringClass &Name)  //only necessary because shaders cant call ttinit
AT1(0x00677040);
static	void	Build_Weapon_Chart_Icons( void )
{
	WeaponBagClass * weapon_bag = CombatManager::Get_The_Star()->Get_Weapon_Bag();
	Clear_Weapon_Chart_Icons();
	float screen_scale = Render2DClass::Get_Screen_Resolution().Width();
	Vector2	pos = WeaponChartBase * screen_scale;
	for ( int column = 1; column <= 10; column++ )
	{
		int key = Get_Primary_Key_For_Function(35 + (column%10) );
		WideStringClass name(0,true);
		Get_Translated_Key_Name(key, name);
		WeaponChartKeynameRenderer->Build_Sentence( name );
		Vector2 text_size = WeaponChartKeynameRenderer->Get_Text_Extents( name );
		Vector2 text_offset = pos - text_size/2 - Vector2( 0, WeaponChartSpacing.Y * screen_scale * 0.75f );
		text_offset.X = (float)(int)text_offset.X;
		text_offset.Y = (float)(int)text_offset.Y;
		WeaponChartKeynameRenderer->Set_Location( text_offset );
		WeaponChartKeynameRenderer->Draw_Sentence();
		for ( int i = 0; i < weapon_bag->Get_Count(); i++ )
		{
			WeaponClass *	weapon = weapon_bag->Peek_Weapon( i );
			if ( weapon != NULL )
			{
				if ( (int)weapon->Get_Key_Number() != (column%10) )
				{
					continue;
				}
				StringClass filename( HUD_WEAPONS_TEXTURE, true );
				RectClass	uv( 0,64,128,128 );
				Vector2		offset( -40, 40 );
				const WeaponDefinitionClass * def = weapon->Get_Definition();
				if ( !def->IconTextureName.Is_Empty() )
				{
					Strip_Path_From_Filename( filename, def->IconTextureName );
					uv = def->IconTextureUV;
					offset = def->IconOffset;
				}
				RectClass icon_box = uv;
				icon_box += pos - icon_box.Center();
				icon_box.Scale_Relative_Center( WeaponChartIconScale * screen_scale );
				pos.Y += WeaponChartSpacing.Y * screen_scale;
				Render2DClass * renderer = new Render2DClass();
				WeaponChartIcons.Add( renderer );
				renderer->Set_Coordinate_Range( Render2DClass::Get_Screen_Resolution() );
				StringClass new_name(true);
				Strip_Path_From_Filename( new_name, filename );
				renderer->Set_Texture_Init( new_name );
				if ( renderer->Peek_Texture() != NULL )
				{
					float size = (float)renderer->Peek_Texture()->Width;
					if ( size > 0 )
					{
						uv.Scale( Vector2( 1/size, 1/size ) );
					}
				}
				renderer->Add_Quad( icon_box, uv );
			}
		}
		pos.Y = WeaponChartBase.Y * screen_scale;
		pos.X += WeaponChartSpacing.X * screen_scale;
	}
}
bool	ForceChartRebuild = false;
void	HUDClass::Force_Weapon_Chart_Update( void )
{
	ForceChartRebuild = true;
}
void	HUDClass::Force_Weapon_Chart_Display( void )
{
	WeaponChartTimer = WEAPON_CHART_TIME;
}
static	void	Weapon_Chart_Update( void )
{
	if ( CombatManager::Get_The_Star() == NULL || CombatManager::Get_The_Star()->Get_Weapon_Bag() == NULL || CombatManager::Get_The_Star()->Get_Vehicle() != NULL )
	{
		Clear_Weapon_Chart_Icons();
		return;
	}
	int i;
	WeaponBagClass * weapon_bag = CombatManager::Get_The_Star()->Get_Weapon_Bag();
	if ( weapon_bag->HUD_Is_Changed() )
	{
		weapon_bag->HUD_Reset_Changed();
		WeaponChartTimer = WEAPON_CHART_TIME;
	}
	WeaponChartTimer -= TimeManager::FrameSeconds;
	if ( WeaponChartTimer <= 0 )
	{
		return;
	}
	if ( ForceChartRebuild || (weapon_bag->Get_Count()-1 != WeaponChartIcons.Count()) )
	{
		ForceChartRebuild = false;
		Build_Weapon_Chart_Icons();
	}
	float alpha = WWMath::Clamp( WeaponChartTimer, 0, 1 );
	WeaponChartBoxRenderer->Reset();
	int index = 0;
	for ( int column = 1; column <= 10; column++ )
	{
		for ( i = 0; i < weapon_bag->Get_Count(); i++ )
		{
			WeaponClass *	weapon = weapon_bag->Peek_Weapon( i );
			if ( weapon != NULL )
			{
				if ( (int)weapon->Get_Key_Number() != (column%10) )
				{
					continue;
				}
				long color = WeaponChartColor;
				if ( weapon_bag->Get_Index() == i )
				{
					color = WeaponChartColor;
				}
				if ( weapon->Get_Total_Rounds() == 0 )
				{
					color = WeaponChartEmptyColor;
					if ( weapon_bag->Get_Index() == i )
					{
						color = WeaponChartEmptyColor;
					}
				}
				if ( weapon_bag->Get_Index() == i )
				{
					color = COLOR( alpha, color );
				}
				else
				{
					color = COLOR( alpha * 0.5f, color );
				}
				WeaponChartIcons[index]->Force_Color( color );
				index++;
			}
		}
	}
	WeaponChartKeynameRenderer->Force_Alpha( alpha );
}
static	void	Weapon_Chart_Render( void )
{
	if ( WeaponChartTimer <= 0 )
	{
		return;
	}
	WeaponChartBoxRenderer->Render();
	for ( int i = 0; i < WeaponChartIcons.Count(); i++)
	{
		WeaponChartIcons[i]->Render();
	}
	WeaponChartKeynameRenderer->Render();
}
Render2DClass * DamageRenderer;
const int NUM_DAMAGE_INDICATORS	= 8;
float	DamageIndicatorIntensity[ NUM_DAMAGE_INDICATORS ];
bool	DamageIndicatorIntensityChanging;
bool	DamageIndicatorOrientation;
static	void	Damage_Reset( void ) 
{
	for ( int i = 0; i < NUM_DAMAGE_INDICATORS; i++ )
	{
		DamageIndicatorIntensity[ i ] = 0;
	}
	DamageIndicatorIntensityChanging = true;
	CombatManager::Clear_Star_Damage_Direction();
}
static	void	Damage_Init( void ) 
{
	DamageRenderer = new Render2DClass();
	DamageRenderer->Set_Texture( HUD_MAIN_TEXTURE );
	DamageRenderer->Set_Coordinate_Range( Render2DClass::Get_Screen_Resolution() );
	DamageRenderer->Enable_Additive( true );
	Damage_Reset();
}
static	void	Damage_Shutdown( void )
{
	delete DamageRenderer;
	DamageRenderer = NULL;
}
static	void	Damage_Add_Indicator( int index, float start_x, float start_y, float end_x, float end_y )
{
	Vector2	vert[4];
	vert[0] = Vector2( start_x, start_y );
	vert[1] = Vector2( start_x, end_y );
	vert[2] = Vector2( end_x, start_y );
	vert[3] = Vector2( end_x, end_y );
	if ( !CombatManager::Is_First_Person() )
	{
		for ( int i = 0; i < 4; i++ )
		{
			vert[i].X *= WWMath::Fabs(1 + vert[i].Y);
			vert[i].Y /= 2;
			vert[i].Y += 0.25f;
		}
	}
	for ( int i = 0; i < 4; i++ )
	{
		vert[i].X	= (vert[i].X + 0.5f) * Render2DClass::Get_Screen_Resolution().Width();
		vert[i].Y	= (vert[i].Y + 0.5f) * Render2DClass::Get_Screen_Resolution().Height();
	}
	RectClass uv( DAMAGE_1_UV_UL, DAMAGE_1_UV_LR );
	if ( index & 1 )
	{
		uv = RectClass( DAMAGE_2_UV_UL, DAMAGE_2_UV_LR );
	}
	uv.Scale( INFO_UV_SCALE );
	unsigned long color_bits = (int)(DamageIndicatorIntensity[index] * 255) & 0x000000FF;
  	unsigned long color = color_bits | color_bits<<8 | color_bits<<16;
	switch (index)
	{
		case 3:
		case 4:	DamageRenderer->Add_Quad(	vert[1], vert[3], vert[0], vert[2], uv, color );
					break;
		case 5:
		case 6:	DamageRenderer->Add_Quad(	vert[0], vert[1], vert[2], vert[3], uv, color );
					break;
		case 7:
		case 0:	DamageRenderer->Add_Quad(	vert[2], vert[0], vert[3], vert[1], uv, color );
					break;
		case 1:
		case 2:	DamageRenderer->Add_Quad(	vert[3], vert[2], vert[1], vert[0], uv, color );
					break;
	}
}
REF_DEF1(DamageIndicatorsEnabled, bool, 0x008100B0);
static	void	Damage_Update( void )
{
	DamageRenderer->Reset();
	if (DamageIndicatorsEnabled == false)
	{
		return ;
	}
	int new_damage = CombatManager::Get_Star_Damage_Direction();
	if ( new_damage != 0 )
	{
		DamageIndicatorIntensityChanging = true;
	}
	if ( !DamageIndicatorIntensityChanging &&
		DamageIndicatorOrientation == CombatManager::Is_First_Person() )
	{
		return;
	}
	DamageIndicatorOrientation = CombatManager::Is_First_Person();
	DamageIndicatorIntensityChanging = false;
	CombatManager::Clear_Star_Damage_Direction();
	for ( int i = 0; i < NUM_DAMAGE_INDICATORS; i++ )
	{
		if ( new_damage & (1<<i) )
		{
			DamageIndicatorIntensity[ i ] = 1;
			DamageIndicatorIntensityChanging = true;
		}
		else
		{
			if ( DamageIndicatorIntensity[ i ] > 0 )
			{
				DamageIndicatorIntensity[ i ] -= TimeManager::FrameSeconds;
				DamageIndicatorIntensity[ i ] = WWMath::Clamp( DamageIndicatorIntensity[ i ], 0, 1 );
				DamageIndicatorIntensityChanging = true;
			}
		}
	}
	float HORIZ_WIDTH		=	HORIZ_DAMAGE_SIZE.U/640.0f;
	float HORIZ_HEIGHT		=	HORIZ_DAMAGE_SIZE.V/480.0f;
	float VERT_WIDTH		=	VERT_DAMAGE_SIZE.U/640.0f;
	float VERT_HEIGHT		=	VERT_DAMAGE_SIZE.V/480.0f;
	float OFFSET_X			=	HV_DAMAGE_OFFSET.U/640.0f;
	float OFFSET_Y			=	HV_DAMAGE_OFFSET.V/480.0f;
	float DIAG_WIDTH		=	DIAG_DAMAGE_SIZE.U/640.0f;
	float DIAG_HEIGHT		=	DIAG_DAMAGE_SIZE.V/480.0f;
	float DIAG_OFFSET_X		=	DIAG_DAMAGE_OFFSET.U/640.0f;
	float DIAG_OFFSET_Y		=	DIAG_DAMAGE_OFFSET.V/480.0f;
	Damage_Add_Indicator( 0,	-HORIZ_WIDTH/2,					-OFFSET_Y-HORIZ_HEIGHT/2,		HORIZ_WIDTH/2,					-OFFSET_Y+HORIZ_HEIGHT/2		);
	Damage_Add_Indicator( 2,	OFFSET_X-VERT_WIDTH/2,			-VERT_HEIGHT/2,					OFFSET_X+VERT_WIDTH/2,			VERT_HEIGHT/2					);
	Damage_Add_Indicator( 4,	-HORIZ_WIDTH/2,					OFFSET_Y-HORIZ_HEIGHT/2,		HORIZ_WIDTH/2,					OFFSET_Y+HORIZ_HEIGHT/2			);
	Damage_Add_Indicator( 6,	-OFFSET_X-VERT_WIDTH/2,			-VERT_HEIGHT/2,					-OFFSET_X+VERT_WIDTH/2,			VERT_HEIGHT/2					);
	Damage_Add_Indicator( 1,	DIAG_OFFSET_X-DIAG_WIDTH/2,		-DIAG_OFFSET_Y-DIAG_HEIGHT/2,	DIAG_OFFSET_X+DIAG_WIDTH/2,		-DIAG_OFFSET_Y+DIAG_HEIGHT/2	);
	Damage_Add_Indicator( 3,	DIAG_OFFSET_X-DIAG_WIDTH/2,		DIAG_OFFSET_Y-DIAG_HEIGHT/2,	DIAG_OFFSET_X+DIAG_WIDTH/2,		DIAG_OFFSET_Y+DIAG_HEIGHT/2 	);
	Damage_Add_Indicator( 5,	-DIAG_OFFSET_X-DIAG_WIDTH/2,	DIAG_OFFSET_Y-DIAG_HEIGHT/2,	-DIAG_OFFSET_X+DIAG_WIDTH/2,	DIAG_OFFSET_Y+DIAG_HEIGHT/2		);
	Damage_Add_Indicator( 7,	-DIAG_OFFSET_X-DIAG_WIDTH/2,	-DIAG_OFFSET_Y-DIAG_HEIGHT/2,	-DIAG_OFFSET_X+DIAG_WIDTH/2,	-DIAG_OFFSET_Y+DIAG_HEIGHT/2	);
}
void	HUDClass::Damage_Render( void )
{
	DamageRenderer->Render();
}
Render2DClass * TargetRenderer;
Render2DClass * TargetBoxRenderer;
Render2DSentenceClass * TargetNameRenderer;
Render2DSentenceClass * InfoDebugRenderer;
WideStringClass TargetNameString = WideStringClass(true);
Vector2 TargetNameLocation;
static	void	Target_Init( void )
{
	TargetRenderer = new Render2DClass();
	TargetRenderer->Set_Texture( HUD_MAIN_TEXTURE );
	TargetRenderer->Set_Coordinate_Range( Render2DClass::Get_Screen_Resolution() );
	TargetBoxRenderer = new Render2DClass();
	TargetBoxRenderer->Enable_Texturing( false );
	TargetBoxRenderer->Set_Coordinate_Range( Render2DClass::Get_Screen_Resolution() );
	FontCharsClass *chars_font = Fonts[11];
	TargetNameRenderer = new Render2DSentenceClass();
	TargetNameRenderer->Set_Font( chars_font );
	InfoDebugRenderer = new Render2DSentenceClass();
	InfoDebugRenderer->Set_Font( chars_font );
}
static	void	Target_Shutdown( void )
{
	delete TargetRenderer;
	TargetRenderer = NULL;
	delete TargetBoxRenderer;
	TargetBoxRenderer = NULL;
	delete TargetNameRenderer;
	TargetNameRenderer = NULL;
	TargetNameString=L"";
	delete InfoDebugRenderer;
	InfoDebugRenderer = NULL;
}
bool	InfoDebug = false;
static RectClass Get_Target_Box( PhysicalGameObj * obj );
static void	Target_Box_Edge( const Vector2 & a, const Vector2 & b, unsigned int color );
bool SoldierGameObj::Is_Permitted_To_Enter_Vehicle()
{
	return CombatManager::Is_Gameplay_Permitted();
}
static	void	Target_Update( void )
{
	TargetRenderer->Reset();
	TargetBoxRenderer->Reset();
	InfoDebugRenderer->Reset();
	HUDInfo::Update_Info_Object();
	static float box_zoom_size = 0;
	DamageableGameObj * d_obj = HUDInfo::Get_Info_Object();
	if ( d_obj != NULL )
	{
		PhysicalGameObj * p_obj = d_obj->As_PhysicalGameObj();
		BuildingGameObj * building = d_obj->As_BuildingGameObj();
		DamageableGameObj * obj = d_obj;
		if ( obj && CombatManager::Get_The_Star() )
		{
			box_zoom_size += TimeManager::FrameSeconds * 4;
			box_zoom_size = WWMath::Clamp( box_zoom_size, 0, 1 );
			int color = HUDGlobalSettingsDef::Get_Instance()->Get_No_Relation_Color().Convert_To_ARGB();
			if ( p_obj != NULL )
			{
				if ( CombatManager::Get_The_Star()->Is_Teammate(p_obj) )
				{
					color = HUDGlobalSettingsDef::Get_Instance()->Get_Friendly_Color().Convert_To_ARGB();
				}
				else if ( CombatManager::Get_The_Star()->Is_Enemy(p_obj) )
				{
					color = HUDGlobalSettingsDef::Get_Instance()->Get_Enemy_Color().Convert_To_ARGB();
				}
			}
			if ( building != NULL )
			{
				if ( CombatManager::Get_The_Star()->Is_Teammate(building) )
				{
					color = HUDGlobalSettingsDef::Get_Instance()->Get_Friendly_Color().Convert_To_ARGB();
				}
				else if ( CombatManager::Get_The_Star()->Is_Enemy(building) )
				{
					color = HUDGlobalSettingsDef::Get_Instance()->Get_Enemy_Color().Convert_To_ARGB();
				}
			}
			RectClass box = Render2DClass::Get_Screen_Resolution();
			if ( obj->As_PhysicalGameObj() )
			{
				box = Get_Target_Box( obj->As_PhysicalGameObj() );
			}
			else
			{
				box.Scale_Relative_Center( 0.3f );
				Vector2 new_center = (COMBAT_CAMERA->Get_Camera_Target_2D_Offset() * 0.5f );
				new_center.Y *= -1.0f;
				new_center += Vector2( 0.5f, 0.5f );
				new_center.X *= Render2DClass::Get_Screen_Resolution().Right;
				new_center.Y *= Render2DClass::Get_Screen_Resolution().Bottom;
				box += new_center - box.Center();
			}
			if ( box_zoom_size < 1 )
			{
				box.Scale_Relative_Center( 1 + ((1-box_zoom_size) * 0.3f) );
			}
			box.Snap_To_Units( Vector2( 1, 1 ) );
			RectClass res = Render2DClass::Get_Screen_Resolution();
			if ( box.Top < 0 )
			{
				box.Top = 0;
			}
			if ( box.Left < 0 )
			{
				box.Left = 0;
			}
			if ( box.Right > res.Right-1 )
			{
				box.Right = res.Right-1;
			}
			if ( box.Bottom > res.Bottom - 26 )
			{
				box.Bottom = res.Bottom - 26;
			}
			Target_Box_Edge( box.Upper_Left(), box.Upper_Right(), color );
			Target_Box_Edge( box.Upper_Left(), box.Lower_Left(), color );
			Target_Box_Edge( box.Lower_Right(), box.Upper_Right(), color );
			Target_Box_Edge( box.Lower_Right(), box.Lower_Left(), color );
			RectClass	uv( TARGET_HEALTH_R_UV_UL, TARGET_HEALTH_R_UV_LR );
			RectClass	draw = uv;
			RectClass	draw2;
			bool draw_health = obj->Is_Health_Bar_Displayed();
			if ( draw_health )
			{
				float health_percent = 0;
				const DefenseObjectClass	* def = obj->Get_Defense_Object();
				if ( def )
				{
					float health_max = def->Get_Health_Max();
					float shield_max = def->Get_Shield_Strength_Max();
					float max = health_max + shield_max;
					float health = def->Get_Health();
					float shield = def->Get_Shield_Strength();
					if ( health_max )
					{
						shield *= (health/health_max);
					}
					float total = health + shield;
					if ( max != 0 )
					{
						health_percent = total / max;
						health_percent = WWMath::Clamp( health_percent, 0, 1 );
					}
				}
				int health_color = Get_Health_Color( health_percent );
				static float power_flash = 0;
				power_flash += TimeManager::FrameSeconds;
				power_flash = WWMath::Wrap( power_flash, 0, 2 );
				RectClass uv3( POWER_OUT_ICON_UV_UL, POWER_OUT_ICON_UV_LR );
				RectClass draw3 = uv3;
				uv3.Scale( INFO_UV_SCALE );
				RectClass	black( HEALTH_BACK_UV_UL, HEALTH_BACK_UV_LR );
				black.Scale( INFO_UV_SCALE );
				uv.Scale( INFO_UV_SCALE );
				draw += box.Lower_Left() - draw.Upper_Left() + Vector2( 0, 18 );
				draw += Vector2( (box.Width() - draw.Width())/2 + 2, 0 );
				draw3 += draw.Center() - draw3.Center();
				draw.Inflate( Vector2( 1, 1 ) );
				TargetRenderer->Add_Quad( draw, black );
				draw.Inflate( Vector2( -1, -1 ) );
				draw.Right = draw.Left + draw.Width() * health_percent;
				uv.Right = uv.Left + uv.Width() * health_percent;
				TargetRenderer->Add_Quad( draw, uv, health_color );
				if ( power_flash < 1 )
				{
				}
				uv.Set( TARGET_HEALTH_L_UV_UL, TARGET_HEALTH_L_UV_LR );
				draw2 = uv;
			}
			uv.Set( TARGET_NAME_UV_UL, TARGET_NAME_UV_LR );
			draw = uv;
			uv.Scale( INFO_UV_SCALE );
			draw += box.Lower_Left() - draw.Upper_Left() + Vector2( 0, 1 );
			draw += Vector2( (box.Width() - draw.Width())/2 + 10, 0 );
			draw.Snap_To_Units( Vector2( 1, 1 ) );
			TargetRenderer->Add_Quad( draw, uv, color );
			if (obj->Get_Definition().Get_Translated_Name_ID() != 0 )
			{
				TDBObjClass* translate_obj = TranslateDBClass::Find_Object(obj->Get_Definition().Get_Translated_Name_ID());
				if (translate_obj)
				{
					WideStringClass translate_string = translate_obj->Get_String();
					if (building && HUDInfo::IsMCT)
						translate_string = TRANSLATE(IDS_Enc_Struct_Nod_MCT_Name);

					Vector2 string_loc = draw.Upper_Left() + Vector2(3.f, 1.f);

					bool redraw = false;
					if (translate_string != TargetNameString)
					{
						TargetNameString = translate_string;
						TargetNameRenderer->Reset();
						TargetNameRenderer->Build_Sentence(TargetNameString);
						redraw = true;
					}

					if (string_loc != TargetNameLocation)
					{
						TargetNameLocation = string_loc;
						TargetNameRenderer->Set_Location(TargetNameLocation);
						TargetNameRenderer->Set_Base_Location(TargetNameLocation);
						redraw = true;
					}

					if (redraw)
						TargetNameRenderer->Draw_Sentence(color);
				}
				else
				{
					TargetNameRenderer->Reset();
					TargetNameString=L"";
					TargetNameLocation = Vector2(0.0f,0.0f);
				}
			}
			else
			{
				TargetNameRenderer->Reset();
				TargetNameString=L"";
				TargetNameLocation = Vector2(0.0f,0.0f);
			}
			int team = 1;
			if ( p_obj != NULL )
			{
				team = p_obj->Get_Player_Type();
				SoldierGameObj *s_obj = p_obj->As_SoldierGameObj();
				if (s_obj != NULL)
				{
					if (s_obj->Is_Human_Controlled())
					{
						if (s_obj->Get_Player_Data())
						{
							team = ((cPlayer *)s_obj->Get_Player_Data())->PlayerType;
						}

					}
				}
			}
			if ( building != NULL )
			{
				if ( building->Get_Player_Type() == 1 )
				{
					team = 1;
				}
				else if ( building->Get_Player_Type() == 0 )
				{
					team = 0;
				}
				else
				{
					team = -2;
				}
			}
			if ( team == 0 )
			{
				uv.Set( NOD_ICON_UV_UL, NOD_ICON_UV_LR );
			}
			else if ( team == 1 )
			{
				uv.Set( GDI_ICON_UV_UL, GDI_ICON_UV_LR );
			}
			else
			{
				uv.Set( NEUTRAL_ICON_UV_UL, NEUTRAL_ICON_UV_LR );
			}
			draw2 = uv;
			uv.Scale( INFO_UV_SCALE );
			draw2 += draw.Upper_Left() - draw2.Upper_Right();
			TargetRenderer->Add_Quad( draw2, uv );
			bool draw_chevrons = false;
			if ( p_obj != NULL && p_obj->Is_HUD_Pokable_Indicator_Enabled() )
			{
				draw_chevrons = true;
			}
			if ( obj->As_SmartGameObj() != NULL &&
				  obj->As_SmartGameObj()->As_VehicleGameObj() != NULL &&
				  CombatManager::Get_The_Star()->Is_Permitted_To_Enter_Vehicle() &&
				  obj->As_SmartGameObj()->As_VehicleGameObj()->Is_Entry_Permitted( CombatManager::Get_The_Star() ) )
			{
				draw_chevrons = true;
			}
			if ( draw_chevrons )
			{
				RectClass enterable_box( Vector2(0,0), TARGET_ENTERABLE_SIZE );
				enterable_box += Vector2( box.Center().X - enterable_box.Center().X ,
								box.Top - enterable_box.Bottom );
				static float enterable_bounce = 0;
				enterable_bounce += TimeManager::FrameSeconds * 5;
				enterable_bounce = WWMath::Wrap( enterable_bounce, 0, DEG_TO_RADF( 360 ) );
				enterable_box += Vector2( 0, TARGET_ENTERABLE_BOUNCE * (WWMath::Sin( enterable_bounce ) - 1) );
				uv.Set( TARGET_ENTERABLE_UV_UL, TARGET_ENTERABLE_UV_LR );
				uv.Scale( INFO_UV_SCALE );
				color = Get_Health_Color( 1 );
				enterable_box -= Vector2( 0, enterable_box.Height() * 0.6f );
				TargetRenderer->Add_Tri(	enterable_box.Upper_Right(), enterable_box.Upper_Left(), enterable_box.Center(),
					uv.Upper_Right(), uv.Upper_Left(), uv.Center(), color );
				enterable_box += Vector2( 0, enterable_box.Height() * 0.6f );
				TargetRenderer->Add_Tri(	enterable_box.Upper_Right(), enterable_box.Upper_Left(), enterable_box.Center(),
					uv.Upper_Right(), uv.Upper_Left(), uv.Center(), color );
				enterable_box += Vector2( 0, enterable_box.Height() * 0.6f );
				TargetRenderer->Add_Tri(	enterable_box.Upper_Right(), enterable_box.Upper_Left(), enterable_box.Center(),
					uv.Upper_Right(), uv.Upper_Left(), uv.Center(), color );
			}
		}
	}
	else
	{
		TargetNameRenderer->Reset();
		TargetNameString=L"";
		TargetNameLocation = Vector2(0.0f,0.0f);
		box_zoom_size = 0;
		HUDInfo::InfoObject = 0;
	}
}
static	void	Target_Render( void )
{
	TargetRenderer->Render();
	TargetBoxRenderer->Render();
	TargetNameRenderer->Render();
	InfoDebugRenderer->Render();
}
static RectClass Get_Target_Box( PhysicalGameObj * obj )
{
	PhysClass * po = obj->Peek_Physical_Object();
	Vector2	top(0,0);
	Vector2	bottom(0,0);
	if ( po != NULL )
	{
		AABoxClass obj_box;
		po->Get_Shadow_Blob_Box( &obj_box );
		Matrix3D obj_tm = po->Get_Transform();
		Matrix3D boxview_tm;
		Matrix3D boxview_inv_tm;
		boxview_tm.Look_At(COMBAT_CAMERA->Get_Transform().Get_Translation(),obj_tm * obj_box.Center,0.0f);
		boxview_tm.Get_Orthogonal_Inverse(boxview_inv_tm);
		Matrix3D obj_to_boxview_tm = boxview_inv_tm * obj_tm;
		AABoxClass boxview_box;
		obj_to_boxview_tm.Transform_Center_Extent_AABox( obj_box.Center, obj_box.Extent, &boxview_box.Center, &boxview_box.Extent );
		Matrix3D camera_tm = COMBAT_CAMERA->Get_Transform();
		Matrix3D camera_inv_tm;
		camera_tm.Get_Orthogonal_Inverse(camera_inv_tm);
		Matrix3D boxview_to_camera_tm = camera_inv_tm * boxview_tm;
		AABoxClass camera_box;
		boxview_to_camera_tm.Transform_Center_Extent_AABox( boxview_box.Center, boxview_box.Extent, &camera_box.Center, &camera_box.Extent);
		camera_box.Extent.Z = 0.0f;
		Vector3 ctop = camera_box.Center - camera_box.Extent;
		Vector3 cbottom = camera_box.Center + camera_box.Extent;
		Vector3	temp;
		COMBAT_CAMERA->Project_Camera_Space_Point( temp, ctop );
		top.X = temp.X;
		top.Y = temp.Y;
		COMBAT_CAMERA->Project_Camera_Space_Point( temp, cbottom );
		bottom.X = temp.X;
		bottom.Y = temp.Y;
	}
	RectClass	screen = Render2DClass::Get_Screen_Resolution();
	top.X = top.X * 0.5f + 0.5f;
	top.Y = top.Y * -0.5f + 0.5f;
	bottom.X = bottom.X * 0.5f + 0.5f;
	bottom.Y = bottom.Y * -0.5f + 0.5f;
	float temp = top.Y;
	top.Y = bottom.Y;
	bottom.Y = temp;
	static RectClass info_box;
	info_box.Set( top.X * screen.Right, top.Y * screen.Bottom, bottom.X * screen.Right, bottom.Y * screen.Bottom );
	return info_box;
}
static void	Target_Box_Edge( const Vector2 & a, const Vector2 & b, unsigned int color )
{
	float percent = 0.2f;
	Vector2 a_ = b - a;
	a_ *= percent;
	a_ += a;
	TargetBoxRenderer->Add_Line( a, a_, 2, color );
	Vector2 b_ = a - b;
	b_ *= percent;
	b_ += b;
	TargetBoxRenderer->Add_Line( b, b_, 2, color );
}
DynamicVectorClass<Render2DClass *>	ObjectivePogRenderers;
Render2DClass * ObjectiveArrowRenderer;
Render2DSentenceClass * ObjectiveTextRenderer;
int	CurrentObjectiveIndex = 0;
void * CurrentObjective = NULL;
static int CachedObjectiveIndex=-1;
int CachedRange=0;
#define	OBJECTIVE_ARROW_TEXTURE		"HUD_obje_arrow.TGA"
static	void	Objective_Init( void )
{
	ObjectiveArrowRenderer = new Render2DClass();
	ObjectiveArrowRenderer->Set_Texture( OBJECTIVE_ARROW_TEXTURE );
	ObjectiveArrowRenderer->Set_Coordinate_Range( Render2DClass::Get_Screen_Resolution() );
	FontCharsClass *font = Fonts[11];
	ObjectiveTextRenderer = new Render2DSentenceClass();
	ObjectiveTextRenderer->Set_Font( font );
	CurrentObjectiveIndex=0;
	CurrentObjective=NULL;
	CachedObjectiveIndex=-1;
	CachedRange=0;
}
static	void	Objective_Release_Pogs( void )
{
	for ( int i = 0; i < ObjectivePogRenderers.Count(); i++ )
	{
		delete ObjectivePogRenderers[i];
		ObjectivePogRenderers[i] = NULL;
	}
	ObjectivePogRenderers.Delete_All();
}
static	void	Objective_Shutdown( void )
{
	Objective_Release_Pogs();
	delete ObjectiveArrowRenderer;
	ObjectiveArrowRenderer = NULL;
	delete ObjectiveTextRenderer;
	ObjectiveTextRenderer = NULL;
}
typedef enum
{
	INPUT_FUNCTION_MOVE_FORWARD,
	INPUT_FUNCTION_MOVE_BACKWARD,
	INPUT_FUNCTION_MOVE_LEFT,
	INPUT_FUNCTION_MOVE_RIGHT,
	INPUT_FUNCTION_MOVE_UP,
	INPUT_FUNCTION_MOVE_DOWN,
	INPUT_FUNCTION_WALK_MODE,
	INPUT_FUNCTION_TURN_LEFT,
	INPUT_FUNCTION_TURN_RIGHT,
	INPUT_FUNCTION_VEHICLE_TURN_LEFT,
	INPUT_FUNCTION_VEHICLE_TURN_RIGHT,
	INPUT_FUNCTION_VEHICLE_TOGGLE_GUNNER,
	INPUT_FUNCTION_WEAPON_UP,
	INPUT_FUNCTION_WEAPON_DOWN,
	INPUT_FUNCTION_WEAPON_LEFT,
	INPUT_FUNCTION_WEAPON_RIGHT,
	INPUT_FUNCTION_WEAPON_RESET,
	INPUT_FUNCTION_ZOOM_IN,
	INPUT_FUNCTION_ZOOM_OUT,
	INPUT_FUNCTION_ACTION,
	INPUT_FUNCTION_JUMP,
	INPUT_FUNCTION_CROUCH,
	INPUT_FUNCTION_DIVE_FORWARD,
	INPUT_FUNCTION_DIVE_BACKWARD,
	INPUT_FUNCTION_DIVE_LEFT,
	INPUT_FUNCTION_DIVE_RIGHT,
	INPUT_FUNCTION_TURN_AROUND,
	INPUT_FUNCTION_DROP_FLAG,
	INPUT_FUNCTION_NEXT_WEAPON,
	INPUT_FUNCTION_PREV_WEAPON,
	INPUT_FUNCTION_FIRE_WEAPON_PRIMARY,
	INPUT_FUNCTION_FIRE_WEAPON_SECONDARY,
	INPUT_FUNCTION_USE_WEAPON,
	INPUT_FUNCTION_RELOAD_WEAPON,
	INPUT_FUNCTION_SELECT_NO_WEAPON,
	INPUT_FUNCTION_SELECT_WEAPON_0,
	INPUT_FUNCTION_SELECT_WEAPON_1,
	INPUT_FUNCTION_SELECT_WEAPON_2,
	INPUT_FUNCTION_SELECT_WEAPON_3,
	INPUT_FUNCTION_SELECT_WEAPON_4,
	INPUT_FUNCTION_SELECT_WEAPON_5,
	INPUT_FUNCTION_SELECT_WEAPON_6,
	INPUT_FUNCTION_SELECT_WEAPON_7,
	INPUT_FUNCTION_SELECT_WEAPON_8,
	INPUT_FUNCTION_SELECT_WEAPON_9,
	INPUT_FUNCTION_CYCLE_POG,
	INPUT_FUNCTION_PANIC,
	INPUT_FUNCTION_CURSOR_TARGETING,
	INPUT_FUNCTION_FIRST_PERSON_TOGGLE,
	INPUT_FUNCTION_SUICIDE,
	INPUT_FUNCTION_VERBOSE_HELP,
	INPUT_FUNCTION_BEGIN_PUBLIC_MESSAGE,
	INPUT_FUNCTION_BEGIN_TEAM_MESSAGE,
	INPUT_FUNCTION_BEGIN_PRIVATE_MESSAGE,
	INPUT_FUNCTION_BEGIN_CONSOLE,
	INPUT_FUNCTION_HELP_SCREEN,
	INPUT_FUNCTION_EVA_OBJECTIVES_SCREEN,
	INPUT_FUNCTION_EVA_MAP_SCREEN,
	INPUT_FUNCTION_CAMERA_HEADING_LEFT,
	INPUT_FUNCTION_CAMERA_HEADING_RIGHT,
	INPUT_FUNCTION_CAMERA_TRANSTILT_INC,
	INPUT_FUNCTION_CAMERA_TRANSTILT_DEC,
	INPUT_FUNCTION_CAMERA_VIEWTILT_INC,
	INPUT_FUNCTION_CAMERA_VIEWTILT_DEC,
	INPUT_FUNCTION_CAMERA_DIST_INC,
	INPUT_FUNCTION_CAMERA_DIST_DEC,
	INPUT_FUNCTION_CAMERA_FOV_INC,
	INPUT_FUNCTION_CAMERA_FOV_DEC,
	INPUT_FUNCTION_CAMERA_HEIGHT_INC,
	INPUT_FUNCTION_CAMERA_HEIGHT_DEC,
	INPUT_FUNCTION_MENU_SERVERQUICKSTART,
   INPUT_FUNCTION_MENU_CLIENTQUICKSTART,
	INPUT_FUNCTION_MAKE_SCREEN_SHOT,
	INPUT_FUNCTION_TOGGLE_MOVIE_CAPTURE,
	INPUT_FUNCTION_DEBUG_SINGLE_STEP,
	INPUT_FUNCTION_DEBUG_SINGLE_STEP_STEP,
	INPUT_FUNCTION_DEBUG_RAPID_MOVE,
	INPUT_FUNCTION_DEBUG_GENERIC0,
	INPUT_FUNCTION_DEBUG_GENERIC1,
	INPUT_FUNCTION_DEBUG_FAR_CLIP_IN,
	INPUT_FUNCTION_DEBUG_FAR_CLIP_OUT,
   INPUT_FUNCTION_QUICK_FULL_EXIT,
	INPUT_FUNCTION_VIS_UPDATE,
	INPUT_FUNCTION_TOGGLE_SNAP_SHOT_MODE,
	INPUT_FUNCTION_SNAP_SHOT_ADVANCE,
	INPUT_FUNCTION_CNC,
	INPUT_FUNCTION_QUICKSAVE,
	INPUT_FUNCTION_MENU_TOGGLE,
	INPUT_FUNCTION_EVA_MISSION_OBJECTIVES_TOGGLE,
	INPUT_FUNCTION_RADIO_CMD_01,
	INPUT_FUNCTION_RADIO_CMD_02,
	INPUT_FUNCTION_RADIO_CMD_03,
	INPUT_FUNCTION_RADIO_CMD_04,
	INPUT_FUNCTION_RADIO_CMD_05,
	INPUT_FUNCTION_RADIO_CMD_06,
	INPUT_FUNCTION_RADIO_CMD_07,
	INPUT_FUNCTION_RADIO_CMD_08,
	INPUT_FUNCTION_RADIO_CMD_09,
	INPUT_FUNCTION_RADIO_CMD_10,
	INPUT_FUNCTION_RADIO_CMD_11,
	INPUT_FUNCTION_RADIO_CMD_12,
	INPUT_FUNCTION_RADIO_CMD_13,
	INPUT_FUNCTION_RADIO_CMD_14,
	INPUT_FUNCTION_RADIO_CMD_15,
	INPUT_FUNCTION_RADIO_CMD_16,
	INPUT_FUNCTION_RADIO_CMD_17,
	INPUT_FUNCTION_RADIO_CMD_18,
	INPUT_FUNCTION_RADIO_CMD_19,
	INPUT_FUNCTION_RADIO_CMD_20,
	INPUT_FUNCTION_RADIO_CMD_21,
	INPUT_FUNCTION_RADIO_CMD_22,
	INPUT_FUNCTION_RADIO_CMD_23,
	INPUT_FUNCTION_RADIO_CMD_24,
	INPUT_FUNCTION_RADIO_CMD_25,
	INPUT_FUNCTION_RADIO_CMD_26,
	INPUT_FUNCTION_RADIO_CMD_27,
	INPUT_FUNCTION_RADIO_CMD_28,
	INPUT_FUNCTION_RADIO_CMD_29,
	INPUT_FUNCTION_RADIO_CMD_30,
	INPUT_FUNCTION_TEAM_INFO_TOGGLE,
	INPUT_FUNCTION_BATTLE_INFO_TOGGLE,
	INPUT_FUNCTION_SERVER_INFO_TOGGLE,
	INPUT_FUNCTION_PROFILE_ENTER_CHILD0,
	INPUT_FUNCTION_PROFILE_ENTER_CHILD1,
	INPUT_FUNCTION_PROFILE_ENTER_CHILD2,
	INPUT_FUNCTION_PROFILE_ENTER_CHILD3,
	INPUT_FUNCTION_PROFILE_ENTER_CHILD4,
	INPUT_FUNCTION_PROFILE_ENTER_CHILD5,
	INPUT_FUNCTION_PROFILE_ENTER_CHILD6,
	INPUT_FUNCTION_PROFILE_ENTER_CHILD7,
	INPUT_FUNCTION_PROFILE_ENTER_CHILD8,
	INPUT_FUNCTION_PROFILE_ENTER_CHILD9,
	INPUT_FUNCTION_PROFILE_ENTER_PARENT,
	INPUT_FUNCTION_PROFILE_RESET,
	INPUT_FUNCTION_COUNT
} InputFunction;
REF_DEF1(MenuMode, bool, 0x00856E26);
REF_DEF1(ConsoleMode, bool, 0x00856E25);
REF_ARR_DEF1(FunctionValue, float, INPUT_FUNCTION_COUNT, 0x008569C0);
static	bool	Peek_State( InputFunction function )	{ return (FunctionValue[ function ] > 0.5f); }
static	bool	Get_State( InputFunction function )		{ return (MenuMode || ConsoleMode) ? false : Peek_State (function); }
#define	POG_FLY_TIME	2.0f
static	void	Objective_Update( void )
{
	RectClass	pog_box( 0, 0, 64, 64 );
	pog_box += Render2DClass::Get_Screen_Resolution().Upper_Right() - pog_box.Upper_Right();
	pog_box += Vector2( -16, 8 );
#define	POG_SPACING		Vector2( 10, 0 )
	bool rebuild = false;
	int objective_count = ObjectiveManager::Get_Num_HUD_Objectives();
	if ( objective_count > 0 )
	{
		if ( CurrentObjective != ObjectiveManager::ObjectiveList[CurrentObjectiveIndex])
		{
			CurrentObjectiveIndex = 0;
			rebuild = true;
		}
		if ( (cGameType::GameType == 1) && Get_State( INPUT_FUNCTION_CYCLE_POG ) )
		{
			rebuild = true;
			CurrentObjectiveIndex++;
			if ( CurrentObjectiveIndex >= objective_count )
			{
				CurrentObjectiveIndex = 0;
			}
		}
 		CurrentObjective = ObjectiveManager::ObjectiveList[CurrentObjectiveIndex];
	}
	else
	{
		if (CurrentObjectiveIndex!=0 || CurrentObjective!=NULL)
		{
			rebuild=true;
		}
		CurrentObjectiveIndex = 0;
		CurrentObjective = NULL;
	}
	if ( rebuild || ObjectiveManager::HUDUpdate )
	{
		Objective_Release_Pogs();
		ObjectiveTextRenderer->Reset();
		CachedRange=0;
		CachedObjectiveIndex=-1;
		int count = objective_count;
		pog_box += POG_SPACING * (float)count;
		bool dont_clear = false;
		for ( int i = count - 1; i >= 0; i-- )
		{
			int index = (i + CurrentObjectiveIndex) % objective_count;
			pog_box -= POG_SPACING;
			Render2DClass * renderer = new Render2DClass();
			if ( renderer )
			{
				renderer->Set_Texture( ObjectiveManager::Get_HUD_Objectives_Pog_Texture_Name( index ) );
				renderer->Set_Coordinate_Range( Render2DClass::Get_Screen_Resolution() );
				float	age = ObjectiveManager::Get_HUD_Objectives_Age( index );
				if ( age < POG_FLY_TIME )
				{
					Vector2 offset = Render2DClass::Get_Screen_Resolution().Center() - Render2DClass::Get_Screen_Resolution().Upper_Right();
					offset.X *= 0.75f;
					float fly = WWMath::Clamp( (2.0f-(2.0f*age/POG_FLY_TIME)), 0, 1 );
					offset *= fly;
					pog_box += offset;
					renderer->Add_Quad( pog_box );
					pog_box -= offset;
					dont_clear = true;
					Render2DClass * renderer2 = new Render2DClass();
					if ( renderer2 )
					{
						renderer2->Set_Texture( "HUD_STAR.TGA" );
						renderer2->Set_Coordinate_Range( Render2DClass::Get_Screen_Resolution() );
						RectClass	star_box( -32, -32, 32, 32 );
						star_box.Scale( fly );
						Vector2	star_fly_start = Render2DClass::Get_Screen_Resolution().Center();
						Vector2	star_fly_end = Render2DClass::Get_Screen_Resolution().Lower_Left();
						star_fly_start.X *= 0.85f;
						star_fly_start.Y *= 1.175f;
						star_fly_end.X += 40.0f;
						star_fly_end.Y *= 0.8f;
						star_box += star_fly_end;
						Vector2	offset2 = star_fly_start - star_fly_end;
						offset2 *= fly;
						star_box += offset2;
						Vector3 color3( 0,1,0 );
						if ( ObjectiveManager::ObjectiveList[index] != NULL )
						{
							color3 = ObjectiveManager::ObjectiveList[index]->Type_To_Color();
						}
						unsigned int color = color3.Convert_To_ARGB();
						renderer2->Add_Quad( star_box, color  );
						ObjectivePogRenderers.Add( renderer );
					}
				}
				else
				{
					renderer->Add_Quad( pog_box );
				}
				ObjectivePogRenderers.Add( renderer );
			}
		}
		if ( !dont_clear )
		{
			ObjectiveManager::HUDUpdate = false;
		}
	}
	ObjectiveArrowRenderer->Reset();
	float range = 0;
	int count = objective_count;
	if ( count > 0 )
	{
		float angle = 0;
		Vector3	objective_pos = ObjectiveManager::Get_HUD_Objectives_Location( CurrentObjectiveIndex );
		if ( CombatManager::Get_The_Star())
		{
			Vector3 rel_pos;
			Matrix3D::Inverse_Transform_Vector( CombatManager::Get_The_Star()->Get_Transform(), objective_pos, &rel_pos );
			angle = ::atan2( rel_pos.Y, rel_pos.X );
			range = rel_pos.Length();
		}
		Vector2 arrow_vertex;
		arrow_vertex.X = WWMath::Fast_Sin( (float)(angle + DEG_TO_RAD( 180 + 45 )) );
		arrow_vertex.Y = WWMath::Fast_Cos( (float)(angle + DEG_TO_RAD( 180 + 45 )) );
		Vector2 verts[4];
		verts[0] = Vector2( arrow_vertex.X, arrow_vertex.Y );
		verts[1] = Vector2( arrow_vertex.Y, -arrow_vertex.X );
		verts[2] = Vector2( -arrow_vertex.Y, arrow_vertex.X );
		verts[3] = Vector2( -arrow_vertex.X, -arrow_vertex.Y );
		Vector2 offset;
		offset.Y = WWMath::Fast_Sin( (float)(-angle + DEG_TO_RAD( -90 )) );
		offset.X = WWMath::Fast_Cos( (float)(-angle + DEG_TO_RAD( -90 )) );
		offset *= 35;
		offset += pog_box.Center();
		const float SIN_45=0.70710678118654752440084436210485f;
		for ( int i = 0; i < 4; i++ )
		{
			verts[i] *= 0.5f * 16 / SIN_45;
			verts[i] += offset;
		}
		ObjectiveArrowRenderer->Add_Quad( verts[0], verts[1], verts[2], verts[3] );
	}
	int irange=(int)(range);
	irange=(irange/10)*10;
	if (CachedObjectiveIndex!=CurrentObjectiveIndex || irange!=CachedRange)
	{
		ObjectiveTextRenderer->Reset();
		if (objective_count>0)
		{
			CachedRange=irange;
			CachedObjectiveIndex=CurrentObjectiveIndex;
			Vector2 position = pog_box.Lower_Left();
			position += Vector2( 0, -15 );
			WideStringClass str(ObjectiveManager::Get_HUD_Objectives_Message( CurrentObjectiveIndex ),true);
			ObjectiveTextRenderer->Build_Sentence( str );
			Vector2 text_size = ObjectiveTextRenderer->Get_Text_Extents( str );
			position.X = (float)(int)(pog_box.Center().X - (text_size.X/2));
			ObjectiveTextRenderer->Set_Location( position );
			ObjectiveTextRenderer->Draw_Sentence();
			position = pog_box.Lower_Left();
			str.Format( TRANSLATE(IDS_HUD_RANGE), irange );
			ObjectiveTextRenderer->Build_Sentence( str );
			text_size = ObjectiveTextRenderer->Get_Text_Extents( str );
			position.X = (float)(int)(pog_box.Center().X - (text_size.X/2));
			ObjectiveTextRenderer->Set_Location( position );
			ObjectiveTextRenderer->Draw_Sentence();
		}
		else
		{
			CachedRange=0;
			CachedObjectiveIndex=-1;
		}
	}
}
static	void	Objective_Render( void )
{
	for ( int i = 0; i < ObjectivePogRenderers.Count(); i++ )
	{
		ObjectivePogRenderers[i]->Render();
	}
	ObjectiveArrowRenderer->Render();
	ObjectiveTextRenderer->Render();
}
Render2DTextClass * InfoHealthCountRenderer;
Render2DTextClass * InfoShieldCountRenderer;
Vector2	InfoBase(0,0);
float	LastHealth = 0;
float	CenterHealthTimer = 0;
const float	CENTER_HEALTH_TIME	= 2.0f;
static	void	Info_Init( void )
{
	InfoRenderer = new Render2DClass();
	InfoRenderer->Set_Texture( HUD_MAIN_TEXTURE );
	InfoRenderer->Set_Coordinate_Range( Render2DClass::Get_Screen_Resolution() );
	CenterRenderer = new Render2DClass();
	CenterRenderer->Set_Texture( HUD_MAIN_TEXTURE );
	CenterRenderer->Set_Coordinate_Range( Render2DClass::Get_Screen_Resolution() );
	Font3DInstanceClass * font = WW3DAssetManager::TheInstance->Get_Font3DInstance( LARGE_FONT );
	InfoHealthCountRenderer = new Render2DTextClass( font );
	InfoHealthCountRenderer->Set_Coordinate_Range( Render2DClass::Get_Screen_Resolution() );
	font->Release_Ref();
	font = WW3DAssetManager::TheInstance->Get_Font3DInstance( SMALL_FONT );
	InfoShieldCountRenderer = new Render2DTextClass( font );
	InfoShieldCountRenderer->Set_Coordinate_Range( Render2DClass::Get_Screen_Resolution() );
	font->Release_Ref();
	InfoBase = Render2DClass::Get_Screen_Resolution().Lower_Left() + Vector2( INFO_OFFSET );
}
static	void	Info_Shutdown( void )
{
	delete InfoRenderer;
	InfoRenderer = NULL;
	delete InfoHealthCountRenderer;
	InfoHealthCountRenderer = NULL;
	delete InfoShieldCountRenderer;
	InfoShieldCountRenderer = NULL;
	delete CenterRenderer;
	CenterRenderer = NULL;
}
static	void	Info_Update_Health_Shield( void )
{
	float health = 0;
	float health_percent = 0;
	float shield = 0;
	float shield_percent = 0;
	if (CombatManager::Get_The_Star())
	{
		const DefenseObjectClass	* def = CombatManager::Get_The_Star()->Get_Defense_Object();
		if ( CombatManager::Get_The_Star()->Get_Vehicle() )
		{
			def = CombatManager::Get_The_Star()->Get_Vehicle()->Get_Defense_Object();
		}
		if ( def && def->Get_Health_Max() )
		{
			health = def->Get_Health();
			health_percent = WWMath::Clamp( def->Get_Health() / def->Get_Health_Max(), 0, 1 );
		}
		if ( def && def->Get_Shield_Strength_Max() )
		{
			shield = def->Get_Shield_Strength();
			shield_percent = WWMath::Clamp( def->Get_Shield_Strength() / def->Get_Shield_Strength_Max(), 0, 1 );
		}
	}
	RectClass uv;
	RectClass draw;
	uv.Set( HEALTH_UV_UL, HEALTH_UV_LR );
	draw = uv;
	uv.Scale( INFO_UV_SCALE );
	draw += InfoBase + HEALTH_OFFSET - draw.Upper_Left();
	static float _last_health_percent = 0;
	float diff = health_percent - _last_health_percent;
	float max_change = TimeManager::FrameSeconds;
	_last_health_percent += WWMath::Clamp( diff, -max_change, max_change );
	uv.Right = uv.Left + uv.Width() * _last_health_percent;
	draw.Right = draw.Left + draw.Width() * _last_health_percent;
	float color_percent = MAX( _last_health_percent, health_percent );
	InfoRenderer->Add_Quad( draw, uv, Get_Health_Color( color_percent ) );
	uv.Set( GRADIENT_BLACK_UV_UL, GRADIENT_BLACK_UV_LR );
	uv.Scale( INFO_UV_SCALE );
	draw.Set( HEALTH_TEXT_BACK_UL, HEALTH_TEXT_BACK_LR );
	draw += InfoBase;
	InfoRenderer->Add_Quad( draw, uv );
	static float flash = 0;
	flash += TimeManager::FrameSeconds * 4;
	if ( flash > 2 )
	{
		flash -= 2;
	}
	if ( health_percent > 0.25f )
	{
		flash = 0;
	}
	float intensity = flash;
	if ( flash > 1 )
	{
		intensity = 2 - flash;
	}
	int health_color = Get_Health_Color( color_percent );
	uv.Set( HEALTH_CROSS_1_UV_UL, HEALTH_CROSS_1_UV_LR );
	draw = uv;
	uv.Scale( INFO_UV_SCALE );
	draw += InfoBase + HEALTH_CROSS_1_OFFSET - draw.Upper_Left();
	InfoRenderer->Add_Quad( draw, uv, COLOR( intensity, health_color ) );
	RectClass	uv2;
	uv2.Set( HEALTH_CROSS_2_UV_UL, HEALTH_CROSS_2_UV_LR );
	uv2.Scale( INFO_UV_SCALE );
	InfoRenderer->Add_Quad( draw, uv2, COLOR( 1-intensity, health_color ) );
	InfoHealthCountRenderer->Reset();
	if ( health < 1 && health > 0 )
	{
		health = 1;
	}
	long lhealth=WWMath::Float_To_Long(health);
	WCHAR tmp_text[5];
	Generate_WChar_Text_From_Number(tmp_text,4,3,lhealth);
	InfoHealthCountRenderer->Set_Location( draw.Upper_Right() + Vector2( 4,4) );
	InfoHealthCountRenderer->Draw_Text( tmp_text, health_color );
	if ( health != LastHealth || health_percent <= 0.25f )
	{
		LastHealth = health;
		CenterHealthTimer = CENTER_HEALTH_TIME;
	}
	if ( CenterHealthTimer > 0 )
	{
		Vector2	health_center_offset = Render2DClass::Get_Screen_Resolution().Center();
		health_center_offset.X *= 0.5;
		health_center_offset.Y -= draw.Height() / 2;
		health_center_offset -= HEALTH_CROSS_1_OFFSET;
		float fade = WWMath::Clamp( CenterHealthTimer, 0, 1 );
	 	uv.Set( GRADIENT_BLACK_UV_UL, GRADIENT_BLACK_UV_LR );
		uv.Scale( INFO_UV_SCALE );
		draw.Set( HEALTH_TEXT_BACK_UL, HEALTH_TEXT_BACK_LR );
		draw += health_center_offset;
		CenterRenderer->Add_Quad( draw, uv, COLOR( fade ) );
		uv.Set( HEALTH_CROSS_1_UV_UL, HEALTH_CROSS_1_UV_LR );
		draw = uv;
		uv.Scale( INFO_UV_SCALE );
		draw += health_center_offset + HEALTH_CROSS_1_OFFSET - draw.Upper_Left();
		CenterRenderer->Add_Quad( draw, uv, COLOR( fade * intensity, health_color ) );
		CenterRenderer->Add_Quad( draw, uv2, COLOR( fade * (1-intensity), health_color ) );
		CenterTextRenderer->Set_Location( draw.Upper_Right() + Vector2( 4,4) );
		CenterTextRenderer->Draw_Text( tmp_text, COLOR( fade, health_color ) );
		CenterHealthTimer -= TimeManager::FrameSeconds;
	}
	static float _last_shield_percent = 0;
	diff = shield_percent - _last_shield_percent;
	max_change = TimeManager::FrameSeconds;
	_last_shield_percent += WWMath::Clamp( diff, -max_change, max_change );
	shield_percent = _last_shield_percent;
	uv.Right = uv.Left + uv.Width() * shield_percent;
	draw.Right = draw.Left + draw.Width() * shield_percent;
	if ( shield_percent > 0 )
	{
		#define	TOTAL_SHIELD_MOVEMENT		80
		for ( float percent = 0; percent < shield_percent; percent += 0.1f )
		{
			uv.Set( SHIELD_UV_UL, SHIELD_UV_LR );
			draw = uv;
			uv.Scale( INFO_UV_SCALE );
			draw += InfoBase + SHIELD_OFFSET - draw.Upper_Left();
			draw += Vector2( (float)((int)(-percent * TOTAL_SHIELD_MOVEMENT)), 0 );
			InfoRenderer->Add_Quad( draw, uv );
		}
		uv.Set( SHIELD_UV_UL, SHIELD_UV_LR );
		draw = uv;
		uv.Scale( INFO_UV_SCALE );
		draw += InfoBase + SHIELD_OFFSET - draw.Upper_Left();
		draw += Vector2( (float)((int)(-shield_percent * TOTAL_SHIELD_MOVEMENT)), 0 );
		InfoRenderer->Add_Quad( draw, uv );
		InfoShieldCountRenderer->Reset();
		long lshield=WWMath::Float_To_Long(shield);
		WCHAR tmp_text2[5];
		Generate_WChar_Text_From_Number(tmp_text2,4,3,lshield);
		InfoShieldCountRenderer->Set_Location( draw.Upper_Left() + Vector2( 4,4) );
		InfoShieldCountRenderer->Draw_Text( tmp_text2 );
	}
	else
	{
		InfoShieldCountRenderer->Reset();
	}
}
void Info_Update()
{
	InfoRenderer->Reset();
	CenterRenderer->Reset();
	CenterTextRenderer->Reset();
	RectClass uv;
	uv.Set( FRAME_1_UV_UL, FRAME_1_UV_LR );
	RectClass draw = uv;
	uv.Scale( INFO_UV_SCALE );
	draw += InfoBase + FRAME_1_OFFSET - draw.Upper_Left();
	InfoRenderer->Add_Quad( draw, uv );
	uv.Set( FRAME_2_UV_UL, FRAME_2_UV_LR );
	draw = uv;
	uv.Scale( INFO_UV_SCALE );
	draw += InfoBase + FRAME_2_OFFSET - draw.Upper_Left();
	InfoRenderer->Add_Quad( draw, uv );
	uv.Set( FRAME_3_UV_UL, FRAME_3_UV_LR );
	draw = uv;
	uv.Scale( INFO_UV_SCALE );
	draw += InfoBase + FRAME_3_OFFSET - draw.Upper_Left();
	InfoRenderer->Add_Quad( draw, uv );
	uv.Set( FRAME_4_UV_UL, FRAME_4_UV_LR );
	draw = uv;
	uv.Scale( INFO_UV_SCALE );
	draw += InfoBase + FRAME_4_OFFSET - draw.Upper_Left();
	InfoRenderer->Add_Quad( draw, uv );
	uv.Set( FRAME_5_UV_UL, FRAME_5_UV_LR );
	draw = uv;
	uv.Scale( INFO_UV_SCALE );
	draw += InfoBase + FRAME_5_OFFSET - draw.Upper_Left();
	InfoRenderer->Add_Quad( draw, uv );
	uv.Set( FRAME_6_UV_UL, FRAME_6_UV_LR );
	draw = uv;
	uv.Scale( INFO_UV_SCALE );
	draw += InfoBase + FRAME_6_OFFSET - draw.Upper_Left();
	InfoRenderer->Add_Quad( draw, uv );
	uv.Set( HEALTH_BACK_UV_UL, HEALTH_BACK_UV_LR );
	uv.Scale( INFO_UV_SCALE );
	draw.Set( HEALTH_BACK_UL, HEALTH_BACK_LR );
	draw += InfoBase;
	InfoRenderer->Add_Quad( draw, uv );
	Info_Update_Health_Shield();
	if (CombatManager::Get_The_Star())
	{
		if ( CombatManager::Get_The_Star()->Has_Key( 1 ) )
		{
			uv.Set( KEY_1_UV_UL, KEY_1_UV_LR );
			draw = uv;
			uv.Scale( INFO_UV_SCALE );
			draw += InfoBase + KEY_1_OFFSET - draw.Upper_Left();
			InfoRenderer->Add_Quad( draw, uv );
		}
		if ( CombatManager::Get_The_Star()->Has_Key( 2 ) )
		{
			uv.Set( KEY_2_UV_UL, KEY_2_UV_LR );
			draw = uv;
			uv.Scale( INFO_UV_SCALE );
			draw += InfoBase + KEY_2_OFFSET - draw.Upper_Left();
			InfoRenderer->Add_Quad( draw, uv );
		}
		if ( CombatManager::Get_The_Star()->Has_Key( 3 ) )
		{
			uv.Set( KEY_3_UV_UL, KEY_3_UV_LR );
			draw = uv;
			uv.Scale( INFO_UV_SCALE );
			draw += InfoBase + KEY_3_OFFSET - draw.Upper_Left();
			InfoRenderer->Add_Quad( draw, uv );
		}
	}
}
void Info_Render()
{
	if (!HideInfoBox)
	{
		InfoRenderer->Render();
	}
	if (!HideCenter)
	{
		CenterRenderer->Render();
		CenterTextRenderer->Render();
	}
	if (!HideHealthText)
	{
		InfoHealthCountRenderer->Render();
	}
	if (!HideShieldText)
	{
		InfoShieldCountRenderer->Render();
	}
}
static bool		_HUDEnabled = true;
static bool		_HUDInited = false;
typedef enum
{
	RETICLE	= 0,
	RETICLE_HIT,
	ACTION_STATUSBAR_RENDERER,
	DEMO_HUD_IMAGE,
	DEMO_HUD_IMAGE2,
	NUM_RENDER_IMAGES,
};
Render2DClass		*	RenderImages[ NUM_RENDER_IMAGES ];
static bool	Is_HUD_Displayed( void );
void 	HUDClass::Init(bool render_available)
{
	for( int i = 0; i < NUM_RENDER_IMAGES; i++ )
	{
		RenderImages[i] = NULL;
	}
	RenderImages[RETICLE] = new Render2DClass();
	RenderImages[RETICLE]->Set_Texture( "HD_reticle.tga" );
	RenderImages[RETICLE]->Set_Hidden( true );
	RenderImages[RETICLE_HIT] = new Render2DClass();
	RenderImages[RETICLE_HIT]->Set_Texture( "HD_reticle_hit.tga" );
	RenderImages[RETICLE_HIT]->Set_Hidden( true );
	RenderImages[ACTION_STATUSBAR_RENDERER] = new Render2DClass();
	RenderImages[ACTION_STATUSBAR_RENDERER]->Set_Coordinate_Range( Render2DClass::Get_Screen_Resolution() );
	if (render_available)
	{
		_HUDEnabled = true;
		SniperHUDClass::Init();
		Powerup_Init();
		Weapon_Init();
		Weapon_Chart_Init();
		Info_Init();
		Damage_Init();
		Target_Init();
		Objective_Init();
		HUD_Help_Text_Init();
		_HUDInited = true;
	}
}
void 	HUDClass::Shutdown()
{
	if (_HUDInited)
	{
		Objective_Shutdown();
		Target_Shutdown();
		Damage_Shutdown();
		Info_Shutdown();
		Weapon_Chart_Shutdown();
		Weapon_Shutdown();
		Powerup_Shutdown();
		HUD_Help_Text_Shutdown();
		SniperHUDClass::Shutdown();
		for( int i = 0; i < NUM_RENDER_IMAGES; i++ )
		{
			if ( RenderImages[i] )
			{
				delete RenderImages[i];
				RenderImages[i] = NULL;
			}
		}
		_HUDInited = false;
	}
}
void 	HUDClass::Reset( void )
{
	Powerup_Reset();
	Damage_Reset();
	Weapon_Reset();
}
void UpdateHUD2();
void HUDClass::Render()
{
	if (COMBAT_CAMERA && COMBAT_CAMERA->Draw_Sniper())
		SniperHUDClass::Render();
	
	if (Is_HUD_Displayed())
	{
		Powerup_Render();
		Weapon_Render();
		Weapon_Chart_Render();
		Info_Render();
		Damage_Render();
		Target_Render();
		HUD_Help_Text_Render();
		Objective_Render();
		RadarManager::Render();
		for( int i = 0; i < NUM_RENDER_IMAGES; i++ )
		{
			if ( RenderImages[i] )
			{
				RenderImages[i]->Render();
			}
		}
		UpdateHUD2();
	}
}
bool Is_HUD_Displayed()
{
	if (_HUDEnabled && CombatManager::Get_The_Star())
	{
		HumanStateClass::HumanStateType starstate = CombatManager::Get_The_Star()->Get_State();
		if (starstate != HumanStateClass::DEATH && starstate != HumanStateClass::DESTROY)
		{
			return true;
		}
	}
	return false;
}
void 	HUDClass::Think()
{
	if ( COMBAT_CAMERA && COMBAT_CAMERA->Draw_Sniper() )
	{
		SniperHUDClass::Update();
	}
	if ( !Is_HUD_Displayed() )
	{
		return;
	}
	if ( HUDInfo::Display_Action_Status_Bar() )
	{
		RenderImages[ACTION_STATUSBAR_RENDERER]->Set_Hidden( false );
		RenderImages[ACTION_STATUSBAR_RENDERER]->Reset();
		RenderImages[ACTION_STATUSBAR_RENDERER]->Enable_Texturing( false );
		const RectClass &screen_rect = Render2DClass::Get_Screen_Resolution();
		RectClass status_bar_rect (0.4F, 0.95F, 0.6F, 0.98F);
		status_bar_rect.Left		= (float)int(status_bar_rect.Left * screen_rect.Width());
		status_bar_rect.Right	= (float)int(status_bar_rect.Right * screen_rect.Width());
		status_bar_rect.Top		= (float)int(status_bar_rect.Top * screen_rect.Height());
		status_bar_rect.Bottom	= (float)int(status_bar_rect.Bottom  * screen_rect.Height());
		RenderImages[ACTION_STATUSBAR_RENDERER]->Add_Line( status_bar_rect.Upper_Left (), status_bar_rect.Upper_Right (), 1, 0xFFFFFFFF );
		RenderImages[ACTION_STATUSBAR_RENDERER]->Add_Line( status_bar_rect.Upper_Right (), status_bar_rect.Lower_Right (), 1, 0xFFFFFFFF );
		RenderImages[ACTION_STATUSBAR_RENDERER]->Add_Line( status_bar_rect.Lower_Right (), status_bar_rect.Lower_Left (), 1, 0xFFFFFFFF );
		RenderImages[ACTION_STATUSBAR_RENDERER]->Add_Line( status_bar_rect.Lower_Left (), status_bar_rect.Upper_Left (), 1, 0xFFFFFFFF );
		status_bar_rect.Right -= 1.0F;
		status_bar_rect.Bottom -= 1.0F;
		float width					= status_bar_rect.Width() * HUDInfo::Get_Action_Status_Value();
		status_bar_rect.Right	= status_bar_rect.Left + int(width);
		DWORD color					= VRGB_TO_INT32( Vector3( 1.0F, 0.25F, 0 ) );
		RenderImages[ACTION_STATUSBAR_RENDERER]->Add_Quad( status_bar_rect, color );
	}
	else
	{
		RenderImages[ACTION_STATUSBAR_RENDERER]->Set_Hidden( true );
	}
	SmartGameObj * star = CombatManager::Get_The_Star();
	if ( CombatManager::Get_The_Star()->Get_Vehicle() )
	{
		star = CombatManager::Get_The_Star()->Get_Vehicle();
	}
	Info_Update();
	Powerup_Update();
	Weapon_Update();
	Weapon_Chart_Update();
	Damage_Update();
	Target_Update();
	Objective_Update();
	Matrix3D tm;
	star->Get_Transform().Get_Inverse( tm );
	if ( star->As_VehicleGameObj() && COMBAT_CAMERA && !COMBAT_CAMERA->Is_2D_Targeting() )
	{
		Matrix3D gun;
		gun.Obj_Look_At( star->Get_Transform().Get_Translation(), star->Get_Targeting_Pos(), 0 );
		gun.Get_Inverse( tm );
	}
	PhysicalGameObj * obj = NULL;
	if ( HUDInfo::Get_Info_Object() )
	{
		obj = HUDInfo::Get_Info_Object()->As_PhysicalGameObj();
	}
	RadarManager::Set_Bracket_Object( obj );
	Vector2	radar_center = InfoBase + RADAR_CENTER_OFFSET;
	RadarManager::Update( tm, radar_center );
	unsigned long reticle_color = HUDGlobalSettingsDef::Get_Instance()->Get_No_Relation_Color().Convert_To_ARGB();
	if ( HUDInfo::Get_Weapon_Target_Object() != NULL )
	{
		reticle_color = HUDGlobalSettingsDef::Get_Instance()->Get_Friendly_Color().Convert_To_ARGB();
		PhysicalGameObj * pgo = HUDInfo::Get_Weapon_Target_Object()->As_PhysicalGameObj();
		if ( pgo && pgo->Is_Enemy( star ) )
		{
			reticle_color = HUDGlobalSettingsDef::Get_Instance()->Get_Enemy_Color().Convert_To_ARGB();
		}
	}
	WeaponClass * weapon = star->Get_Weapon();
	if ( weapon )
	{
		if ( weapon->Is_Reloading() || weapon->Is_Switching() || !weapon->Is_Loaded() )
		{
			reticle_color = 0xFFFFFF00;
		}
	}
	Vector2 reticle_offset = COMBAT_CAMERA->Get_Camera_Target_2D_Offset();
	RenderImages[RETICLE]->Reset();
	RenderImages[RETICLE]->Add_Quad( RectClass( reticle_offset.X - RETICLE_WIDTH/2, reticle_offset.Y - RETICLE_HEIGHT/2, reticle_offset.X + RETICLE_WIDTH/2, reticle_offset.Y + RETICLE_HEIGHT/2 ), reticle_color);
	if ( CombatManager::Is_Gameplay_Permitted() )
	{
		RenderImages[RETICLE]->Set_Hidden( false );
	}
	else
	{
		RenderImages[RETICLE]->Set_Hidden( true );
	}
	if ( CombatManager::Is_Gameplay_Permitted() && (weapon != NULL) && CombatManager::Is_Hit_Reticle_Enabled() )
	{
		Vector3 pos3d = HUDInfo::Get_Weapon_Target_Position();
		Vector3 reticle_hit_offset;
		COMBAT_CAMERA->Project( reticle_hit_offset, pos3d );
		RenderImages[RETICLE_HIT]->Reset();
		RenderImages[RETICLE_HIT]->Add_Quad( RectClass( reticle_hit_offset.X - RETICLE_WIDTH/2, reticle_hit_offset.Y - RETICLE_HEIGHT/2, reticle_hit_offset.X + RETICLE_WIDTH/2, reticle_hit_offset.Y + RETICLE_HEIGHT/2 ), reticle_color);
		RenderImages[RETICLE_HIT]->Set_Hidden( false );
	}
	else
	{
		RenderImages[RETICLE_HIT]->Set_Hidden( true );
	}
}
void	HUDClass::Toggle_Hide_Points( void )
{
}
void	HUDClass::Display_Points( float points )
{
}
bool	HUDClass::Is_Enabled( void )
{
	return _HUDEnabled;
}
void	HUDClass::Enable( bool enable )
{
	_HUDEnabled = enable;
}
#define	SNIPER_AMMO_UV		3, 139,79,191
#define	SNIPER_AMMO_OFFSET	-10,-4
#define	NUKE_UV				193,187,244,246
#define	NUKE_OFFSET			10,-4
#define	ION_UV				101,196,164,246
#define	ION_OFFSET			10,-4
void	HUDClass::Add_Powerup_Weapon( int id, int rounds )
{
	const WeaponDefinitionClass	* def = WeaponManager::Find_Weapon_Definition( id );
	if ( def )
	{
		if ( !def->IconTextureName.Is_Empty() )
		{
			Powerup_Add( TranslateDBClass::Get_String( def->IconNameID ), rounds,
				def->IconTextureName, def->IconTextureUV, def->IconOffset );
		}
	}
}
void	HUDClass::Add_Powerup_Ammo( int id, int rounds )
{
	Add_Powerup_Weapon( id, rounds );
}
void	HUDClass::Add_Shield_Grant( float strength )
{
	const char * texture_name = "hud_armor3.tga";
	if ( strength > 75 )
	{
		texture_name = "hud_armor1.tga";
	} else if ( strength > 30 )
	{
		texture_name = "hud_armor2.tga";
	}
	Powerup_Add( TranslateDBClass::Get_String(IDS_Power_up_Armor_00),  (int)strength, texture_name, RectClass( 0,0,64,64 ),	Vector2( 10,40 ) );
}
void	HUDClass::Add_Health_Grant( float amount )
{
	const char * texture_name = "hud_health1.tga";
	if ( amount > 75 )
	{
		texture_name = "hud_health3.tga";
	} else if ( amount > 30 )
	{
		texture_name = "hud_health2.tga";
	}
	Powerup_Add( TranslateDBClass::Get_String(IDS_Power_up_Health_00),  (int)amount, texture_name, RectClass( 0,0,64,64 ),	Vector2( 10,40 ) );
}
void	HUDClass::Add_Shield_Upgrade_Grant( float strength )
{
	Powerup_Add( TranslateDBClass::Get_String(IDS_Power_up_Armor_Upgrade),  (int)strength, "hud_armedal.tga", RectClass( 0,0,64,64 ),	Vector2( 10,40 ), false );
}
void	HUDClass::Add_Health_Upgrade_Grant( float amount )
{
	Powerup_Add( TranslateDBClass::Get_String(IDS_Power_up_Health_Upgrade),  (int)amount, "hud_hemedal.tga", RectClass( 0,0,64,64 ),	Vector2( 10,40 ), false );
}
void	HUDClass::Add_Key_Grant( int key )
{
	const char * texture_name = "hud_keycard_green.tga";
	if ( key == 3 )
	{
		texture_name = "hud_keycard_red.tga";
	} else if ( key == 2 )
	{
		texture_name = "hud_keycard_yellow.tga";
	}
	Powerup_Add( TranslateDBClass::Get_String(IDS_Power_up_SecurityCard),  0, texture_name, RectClass( 0,0,64,64 ),	Vector2( 10,40 ), false );
}
void	HUDClass::Add_Objective( int type )
{
	if ( type == 1 )
	{
		Powerup_Add( TranslateDBClass::Get_String(IDS_Enc_Obj_Priority_0_Primary),  0, "p_eva1.tga", RectClass( 0,0,64,64 ),	Vector2( 10,40 ), false );
	} else if ( type == 2 )
	{
		Powerup_Add( TranslateDBClass::Get_String(IDS_Enc_Obj_Priority_0_Secondary),  0, "p_eva2.tga", RectClass( 0,0,64,64 ),	Vector2( 10,40 ), false );
	}
}
void	HUDClass::Add_Data_Link( void )
{
	int cur = (int)(TimeManager::TotalSeconds * 2.0f);
	static int last = 0;
	if ( cur == last )
	{
		return;
	}
	last = cur;
	Powerup_Add( TranslateDBClass::Get_String(IDS_Power_up_DataDisc_01),  0, "hud_cd_rom.tga", RectClass( 0,0,64,64 ),	Vector2( 10,40 ), false );
}
void	HUDClass::Add_Map_Reveal( void )
{
	Add_Data_Link();
}
enum
{
	CHUNKID_VARIABLES			=	1117011622,
	CHUNKID_MARKER_ENTRY,
	MICROCHUNKID_ENABLED		=	1,
};
bool	HUDClass::Save( ChunkSaveClass &csave )
{
	csave.Begin_Chunk( CHUNKID_VARIABLES );
	csave.Begin_Micro_Chunk(MICROCHUNKID_ENABLED);
	csave.Write(&_HUDEnabled,sizeof(_HUDEnabled));
	csave.End_Micro_Chunk();
	csave.End_Chunk();
	return true;
}
bool	HUDClass::Load( ChunkLoadClass &cload )
{
	while (cload.Open_Chunk())
	{
		switch(cload.Cur_Chunk_ID())
		{
			case CHUNKID_VARIABLES:
				while (cload.Open_Micro_Chunk())
				{
					switch(cload.Cur_Micro_Chunk_ID())
					{
						case MICROCHUNKID_ENABLED:
						cload.Read(&_HUDEnabled,sizeof(_HUDEnabled));
						default:
							break;
					}
					cload.Close_Micro_Chunk();
				}
				break;
			default:
				break;
		}
		cload.Close_Chunk();
	}
	return true;
}

RENEGADE_FUNCTION
void ScriptableGameObj::Start_Observers()
AT2(0x006B6C40,0x006B64E0);
