/*	Renegade tt.dll
	HUD and HUD.INI related code
	Copyright 2009 Jonathan Wilson

	This file is part of the Renegade tt.dll.
	CONFIDENTIAL: DO NOT USE OR DISTRIBUTE WITHOUT PERMISSION
*/
#include "general.h"

#include "tt.h"
#include "shaders.h"
#include "render2d.h"
#include "stylemgr.h"
#include "CombatManager.h"
#include "CCameraClass.h"
#include "FileHashEvent.h"
#include "engine_math.h"
bool HideBottomText = false;
bool HidePlayerList = false;
bool RenderInfoTexture = false;
Render2DClass *InfoTextureRender2DClass = 0;
SimpleDynVecClass<Scope *> ScopeList;
bool UseExtraPTPages = false;
bool NewUnpurchasableLogic = false;
unsigned int dialogtextcolor = 0xFFFFFF24;
unsigned int dialogglowcolor = 0xFF0E0000;
unsigned int merchandisetext = 0xFFFFFFFF;
extern unsigned int MenuHiliteColor;
extern unsigned int MenuStaticGlowColor;
extern unsigned int MenuActiveGlowColor;
extern unsigned int MenuPushedBaseGlowColor;
extern unsigned int MenuPushedHighlightGlowColor;
bool ListColumnColorEnabled = false;
float ListColumnColorRed = 1;
float ListColumnColorGreen = 1;
float ListColumnColorBlue = 1;
char *GDIUpArrowTexture;
char *GDIDownArrowTexture;
char *GDIBackgroundTexture1;
char *GDIBackgroundTexture2;
char *NODUpArrowTexture;
char *NODDownArrowTexture;
char *NODBackgroundTexture1;
char *NODBackgroundTexture2;
float refilllimit = 0;
bool AlternateSelectEnabled = false;
char *GDIAlternateSelectTexture1;
char *GDIAlternateSelectTexture2;
char *GDIAlternateSelectTexture3;
char *GDIAlternateSelectTexture4;
char *NODAlternateSelectTexture1;
char *NODAlternateSelectTexture2;
char *NODAlternateSelectTexture3;
char *NODAlternateSelectTexture4;
char *modreg = 0;
bool SidebarSoundsEnabled = false;
char *SidebarRefillSound;
char *SidebarInfantrySound;
char *SidebarVehicleSound;
char *VersionRegistryString = 0;
char *WOLUrlRegistryString = 0;
bool ScrollingMap = false;
bool DisableKillMessages = false;
int lodbudgetval1 = 5000;
int lodbudgetval2 = 10000;

void ReadScopeConfig()
{
	FileClass *f = Get_Data_File("scopes.cfg");
	if ((f) && (f->Is_Available()))
	{
		unsigned int size = f->Size();
		char *data = new char[size];
		f->Open(1);
		f->Read(data,size);
		CheckLoad(data,size,"scopes.cfg",HashScopesCfg);
		delete[] data;
		f->Close();
		Close_Data_File(f);
	}
	int handle = Commands->Text_File_Open("scopes.cfg");
	if (!handle)
	{
		return;
	}
	char data[100];
	while (Commands->Text_File_Get_String(handle,data,100))
	{
		Scope *s = new Scope;
		char *a = strtok(data,"=");
		char *b = strtok(NULL,"=");
		char *c = strtok(NULL,"=");
		char *e = strtok(NULL,"=");
		s->WeaponPreset = newstr(a);
		e[strlen(e)-2] = 0;
		s->ScopeTexture = newstr(b);
		s->MinZoom = (float)atof(c);
		s->MaxZoom = (float)atof(e);
		ScopeList.Add(s);
	}
	Commands->Text_File_Close(handle);
}

void FreeScopeConfig()
{
	int x = ScopeList.Count();
	for (int i = 0;i < x;i++)
	{
		delete[] ScopeList[i]->WeaponPreset;
		delete[] ScopeList[i]->ScopeTexture;
		delete ScopeList[i];
	}
}

void Cleanup_HUD()
{
	CCameraClass::Shutdown();
	Cleanup_HUD2();
	SAFE_DELETE(InfoTextureRender2DClass)
	SAFE_DELETE_ARRAY(GDIUpArrowTexture);
	SAFE_DELETE_ARRAY(GDIDownArrowTexture);
	SAFE_DELETE_ARRAY(GDIBackgroundTexture1);
	SAFE_DELETE_ARRAY(GDIBackgroundTexture2);
	SAFE_DELETE_ARRAY(NODUpArrowTexture);
	SAFE_DELETE_ARRAY(NODDownArrowTexture);
	SAFE_DELETE_ARRAY(NODBackgroundTexture1);
	SAFE_DELETE_ARRAY(NODBackgroundTexture2);
	SAFE_DELETE_ARRAY(GDIAlternateSelectTexture1);
	SAFE_DELETE_ARRAY(GDIAlternateSelectTexture2);
	SAFE_DELETE_ARRAY(GDIAlternateSelectTexture3);
	SAFE_DELETE_ARRAY(GDIAlternateSelectTexture4);
	SAFE_DELETE_ARRAY(NODAlternateSelectTexture1);
	SAFE_DELETE_ARRAY(NODAlternateSelectTexture2);
	SAFE_DELETE_ARRAY(NODAlternateSelectTexture3);
	SAFE_DELETE_ARRAY(NODAlternateSelectTexture4);
	SAFE_DELETE_ARRAY(SidebarRefillSound);
	SAFE_DELETE_ARRAY(SidebarInfantrySound);
	SAFE_DELETE_ARRAY(SidebarVehicleSound);
	SAFE_DELETE_ARRAY(VersionRegistryString);
	SAFE_DELETE_ARRAY(WOLUrlRegistryString);
	SAFE_DELETE_ARRAY(modreg);
}

void InitInfoTexture()
{
	InfoTextureRender2DClass = CreateRender2DClass();
	RectClass uv = RectClass(0,0,1,1);
	InfoTextureRender2DClass->Add_Quad(Render2DClass::Get_Screen_Resolution(),uv,0xFFFFFFFF);
}

void ClearInfoTexture()
{
	RenderInfoTexture = false;
}

void SetInfoTexture(const char *texture)
{
	RenderInfoTexture = true;
	InfoTextureRender2DClass->Set_Texture(texture);
}

void ReadHUDIniBits(INIClass *hudini)
{
	GDIUpArrowTexture = new char[260];
	GDIDownArrowTexture = new char[260];
	GDIBackgroundTexture1 = new char[260];
	GDIBackgroundTexture2 = new char[260];
	NODUpArrowTexture = new char[260];
	NODDownArrowTexture = new char[260];
	NODBackgroundTexture1 = new char[260];
	NODBackgroundTexture2 = new char[260];
	UseExtraPTPages = hudini->Get_Bool("General","UseExtraPTPages",false);
	NewUnpurchasableLogic = hudini->Get_Bool("General","NewUnpurchaseableLogic",false);
	hudini->Get_String("General","GDIUpArrowTexture","",GDIUpArrowTexture,260-1);
	hudini->Get_String("General","GDIDownArrowTexture","",GDIDownArrowTexture,260-1);
	hudini->Get_String("General","GDIBackgroundTexture1","",GDIBackgroundTexture1,260-1);
	hudini->Get_String("General","GDIBackgroundTexture2","",GDIBackgroundTexture2,260-1);
	hudini->Get_String("General","NODUpArrowTexture","",NODUpArrowTexture,260-1);
	hudini->Get_String("General","NODDownArrowTexture","",NODDownArrowTexture,260-1);
	hudini->Get_String("General","NODBackgroundTexture1","",NODBackgroundTexture1,260-1);
	hudini->Get_String("General","NODBackgroundTexture2","",NODBackgroundTexture2,260-1);
	refilllimit = hudini->Get_Float("General","RefillLimit",0);
	AlternateSelectEnabled = hudini->Get_Bool("General","AlternateSelectEnabled",false);
	GDIAlternateSelectTexture1 = new char[260];
	GDIAlternateSelectTexture2 = new char[260];
	GDIAlternateSelectTexture3 = new char[260];
	GDIAlternateSelectTexture4 = new char[260];
	hudini->Get_String("General","GDIAlternateSelectTexture1","",GDIAlternateSelectTexture1,260-1);
	hudini->Get_String("General","GDIAlternateSelectTexture2","",GDIAlternateSelectTexture2,260-1);
	hudini->Get_String("General","GDIAlternateSelectTexture3","",GDIAlternateSelectTexture3,260-1);
	hudini->Get_String("General","GDIAlternateSelectTexture4","",GDIAlternateSelectTexture4,260-1);
	NODAlternateSelectTexture1 = new char[260];
	NODAlternateSelectTexture2 = new char[260];
	NODAlternateSelectTexture3 = new char[260];
	NODAlternateSelectTexture4 = new char[260];
	hudini->Get_String("General","NODAlternateSelectTexture1","",NODAlternateSelectTexture1,260-1);
	hudini->Get_String("General","NODAlternateSelectTexture2","",NODAlternateSelectTexture2,260-1);
	hudini->Get_String("General","NODAlternateSelectTexture3","",NODAlternateSelectTexture3,260-1);
	hudini->Get_String("General","NODAlternateSelectTexture4","",NODAlternateSelectTexture4,260-1);
	modreg = new char[50];
	hudini->Get_String("General","ModReg","",modreg,50-1);
	SidebarSoundsEnabled = hudini->Get_Bool("General","SidebarSoundsEnabled",false);
	if (SidebarSoundsEnabled)
	{
		SidebarRefillSound = new char[50];
		hudini->Get_String("General","SidebarRefillSound","",SidebarRefillSound,50-1);
		SidebarInfantrySound = new char[50];
		hudini->Get_String("General","SidebarInfantrySound","",SidebarInfantrySound,50-1);
		SidebarVehicleSound = new char[50];
		hudini->Get_String("General","SidebarVehicleSound","",SidebarVehicleSound,50-1);
	}
	VersionRegistryString = new char[50];
	hudini->Get_String("General","VersionReg","Software\\Westwood\\Renegade",VersionRegistryString,50-1);
	WOLUrlRegistryString = new char[50];
	hudini->Get_String("General","WOLUrlReg","WOLSettings\\URL",WOLUrlRegistryString,50-1);
	DisableKillMessages = hudini->Get_Bool("General","DisableKillMessages",false);
}

void ReadHUDIniBits2(INIClass *hudini)
{
	ScrollingMap = hudini->Get_Bool("General","ScrollingRadarMap",false);

	StyleMgrClass::TitleColor =					hudini->Get_Color_UInt("General", "TitleColor",	GetColorUInt(255, 255, 255, 255));
	StyleMgrClass::TitleHilightColor =			hudini->Get_Color_UInt("General", "TitleHilightColor", GetColorUInt(255, 255, 255, 0));
	StyleMgrClass::TitleShadowColor =			hudini->Get_Color_UInt("General", "TitleShadowColor", GetColorUInt(255, 0, 0, 0));
	StyleMgrClass::TextColor =					hudini->Get_Color_UInt("General", "TextColor", GetColorUInt(255, 255, 213, 40));
	StyleMgrClass::TextShadowColor =			hudini->Get_Color_UInt("General", "TextShadowColor", GetColorUInt(200, 0, 0, 0));
	StyleMgrClass::LineColor =					hudini->Get_Color_UInt("General", "LineColor", GetColorUInt(255, 255, 174, 40));
	StyleMgrClass::BkColor =					hudini->Get_Color_UInt("General", "BkColor", GetColorUInt(40, 255, 174, 40));
	StyleMgrClass::DisabledTextColor =			hudini->Get_Color_UInt("General", "DisabledTextColor", GetColorUInt(140, 255, 213, 40));
	StyleMgrClass::DisabledTextShadowColor =	hudini->Get_Color_UInt("General", "DisabledTextShadowColor", GetColorUInt(96, 0, 0, 0));
	StyleMgrClass::DisabledLineColor =			hudini->Get_Color_UInt("General", "DisabledLineColor", GetColorUInt(128, 230, 160, 35));
	StyleMgrClass::DisabledBkColor =			hudini->Get_Color_UInt("General", "DisabledBkColor", GetColorUInt(30, 255, 174, 40));
	StyleMgrClass::HilightColor =				hudini->Get_Color_UInt("General", "HilightColorBkColor", GetColorUInt(255, 70, 70, 70));
	StyleMgrClass::TabTextColor =				hudini->Get_Color_UInt("General", "TabTextColor", GetColorUInt(255, 255, 255, 255));
	StyleMgrClass::TabGlowColor =				hudini->Get_Color_UInt("General", "TabGlowColor", GetColorUInt(255, 16, 10, 0));

	dialogtextcolor = hudini->Get_Color_UInt("General", "DialogTextTitleColor", GetColorUInt(255, 255, 255, 36));
	dialogglowcolor = hudini->Get_Color_UInt("General", "DialogTextTitleGlowColor", GetColorUInt(255, 14, 0, 0));
	MenuHiliteColor = hudini->Get_Color_UInt("General", "MenuHiliteColor", GetColorUInt(255, 0, 0, 0));
	merchandisetext = hudini->Get_Color_UInt("General", "MerchandiseTextColor", GetColorUInt(255, 255, 255, 255));

	MenuStaticGlowColor =			hudini->Get_Color_UInt("General", "MenuStaticGlowColor", GetColorUInt(255,  9,  0, 0));
	MenuActiveGlowColor =			hudini->Get_Color_UInt("General", "MenuActiveGlowColor", GetColorUInt(255, 16,  0, 0));
	MenuPushedBaseGlowColor =		hudini->Get_Color_UInt("General", "MenuPushedBaseGlowColor", GetColorUInt(255, 48,  0, 0));
	MenuPushedHighlightGlowColor =	hudini->Get_Color_UInt("General", "MenuPushedHighlightGlowColor", GetColorUInt(255, 48, 29, 0));

	ListColumnColorEnabled = hudini->Get_Bool("General","ListColumnColorEnabled",false);
	if (ListColumnColorEnabled)
	{
		ListColumnColorRed = (float)(hudini->Get_Int("General","ListColumnColorRed",255) / 255.0);
		ListColumnColorGreen = (float)(hudini->Get_Int("General","ListColumnColorGreen",255) / 255.0);
		ListColumnColorBlue = (float)(hudini->Get_Int("General","ListColumnColorBlue",255) / 255.0);
	}
	HidePlayerList = hudini->Get_Bool("General","HidePlayerList",false);
	HideBottomText = hudini->Get_Bool("General","HideBottomText",false);
	lodbudgetval1 = hudini->Get_Int("General","LodBudgetDialogValue1",5000);
	lodbudgetval2 = hudini->Get_Int("General","LodBudgetDialogValue2",10000);
	lodbudgetval1 = max(lodbudgetval1,65535);
	lodbudgetval2 = max(lodbudgetval2,65535);
}

bool RenderHud = true;
