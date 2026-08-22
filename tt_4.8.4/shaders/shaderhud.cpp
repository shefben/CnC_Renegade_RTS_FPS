/*	Renegade Scripts.dll
	Code related to the custom HUD as placed inside shaders.dll
	Copyright 2009 Jonathan Wilson

	This file is part of the Renegade scripts.dll
	The Renegade scripts.dll is free software; you can redistribute it and/or modify it under
	the terms of the GNU General Public License as published by the Free
	Software Foundation; either version 2, or (at your option) any later
	version. See the file COPYING for more details.
	In addition, an exemption is given to allow Run Time Dynamic Linking of this code with any closed source module that does not contain code covered by this licence.
	Only the source code to the module(s) containing the licenced code has to be released.
*/
#include "general.h"

#include "RadarMarkerClass.h"
#include "shaderstatemanager.h"
#include "Objective.h"
#include "ObjectiveManager.h"
#include "RadarManager.h"
#include "SoldierGameObj.h"
#include "CombatManager.h"
#include "DX8Wrapper.h"
#include "SystemSettingEntryStaticLODBudget.h"
#include "SystemSettingEntryDynamicLODBudget.h"
#include "SystemSettings.h"
#include "RegistryClass.h"
#include "wwmath.h"
#include "CCameraClass.h"
#include "PhysicsSceneClass.h"
#include "PhysDecalSysClass.h"
#include "engine_game.h"
#include "engine_obj2.h"
#include "engine_tdb.h"
#include "engine_weap.h"
#include "WeaponBagClass.h"
#include "WeaponClass.h"
#include "WeaponMgr.h"
#include "slist.h"
#include "GameObjManager.h"

struct Color
{
	unsigned int ColorValue;
	float Value;
};

struct Quad
{
	unsigned int Color;
	RectClass Position;
	RectClass UV;
};

struct Texture
{
	char *TextureName;
	Render2DClass *TextureRender2DClass;
	unsigned int QuadCount;
	Quad *Quads;
};

unsigned int WeaponChartColor = 0xFF00;
unsigned int WeaponChartEmptyColor = 0xFFFF00;
bool HideWeaponBox = false;
bool HideInfoBox = false;
bool HidePowerupIcons = false;
bool HideHealthText = false;
bool HideShieldText = false;
bool HideCompass = false;
bool HideWeaponImage = false;
bool HideWeaponText = false;
bool HideClipText = false;
bool HideBulletText = false;
bool HideRadar = false;
bool HideCenter = false;
SimpleDynVecClass<unsigned int> Colors;
SimpleDynVecClass<Color *> HealthColors;
SimpleDynVecClass<Color *> ShieldColors;
SimpleDynVecClass<Color *> HealthBarColors;
SimpleDynVecClass<Color *> ShieldBarColors;
SimpleDynVecClass<Color *> HealthIconColors;
SimpleDynVecClass<Color *> ShieldIconColors;
unsigned int HealthColorCount;
unsigned int ShieldColorCount;
Render2DTextClass *HealthRender2DClass = 0;
Render2DTextClass *ShieldRender2DClass = 0;
bool RenderHealthText = false;
bool RenderShieldText = false;
unsigned int HealthColor = 0;
unsigned int ShieldColor = 0;
Vector2 HealthPosition = Vector2(0,0);
Vector2 ShieldPosition = Vector2(0,0);
Vector2 HealthOffset = Vector2(0,0);
Vector2 ShieldOffset = Vector2(0,0);
Render2DTextClass *BulletRender2DClass = 0;
bool RenderBulletText = false;
unsigned int BulletColor = 0;
Vector2 BulletPosition = Vector2(0,0);
Render2DTextClass *ClipRender2DClass = 0;
bool RenderClipText = false;
unsigned int ClipColor = 0;
Vector2 ClipPosition = Vector2(0,0);
Vector2 WeaponPosition = Vector2(0,0);
bool RenderWeaponText = false;
bool RenderWeaponText2 = false;
Render2DSentenceClass *WeaponNameRenderer2 = 0;
unsigned int WeaponColor = 0;
Render2DClass *WeaponImageRender2DClass = 0;
Vector2 WeaponImagePosition = Vector2(0,0);
bool RenderWeaponImage = false;
bool RenderWeaponImage2 = false;
unsigned int WeaponImageColor = 0;
unsigned int WeaponImageVehicleColor = 0;
unsigned int TextureCount = 0;
SimpleDynVecClass<Texture *> HudTextures;
char *HealthBarTexture = 0;
Render2DClass *HealthBarRender2DClass = 0;
RectClass HealthBarUV = RectClass(0,0,0,0);
Vector2 HealthBarEmptyUV = Vector2(0,0);
Vector2 HealthBarPosition = Vector2(0,0);
unsigned int HealthBarColorCount = 0;
unsigned int HealthBarColor = 0;
unsigned int HealthBarEmptyColor = 0;
unsigned int HealthBarStyle = 0;
bool RenderHealthBar = false;
unsigned int HealthBarDirection = 0;
float HealthBarOffset = 0;
float HealthBarLength = 0;
bool RenderHealthBarEmpty = false;
char *ShieldBarTexture = 0;
Render2DClass *ShieldBarRender2DClass = 0;
RectClass ShieldBarUV = RectClass(0,0,0,0);
Vector2 ShieldBarEmptyUV = Vector2(0,0);
Vector2 ShieldBarPosition = Vector2(0,0);
unsigned int ShieldBarColorCount = 0;
unsigned int ShieldBarColor = 0;
unsigned int ShieldBarEmptyColor = 0;
unsigned int ShieldBarStyle = 0;
bool RenderShieldBar = false;
unsigned int ShieldBarDirection = 0;
float ShieldBarOffset = 0;
float ShieldBarLength = 0;
bool RenderShieldBarEmpty = false;
bool BulletEnabled = false;
bool ClipEnabled = false;
bool HealthBarEnabled = false;
bool HealthEnabled = false;
bool ShieldBarEnabled = false;
bool ShieldEnabled = false;
bool WeaponEnabled = false;
bool WeaponImageEnabled = false;
bool HealthIconEnabled = false;
bool ShieldIconEnabled = false;
Vector2 HealthIconPosition = Vector2(0,0);
Vector2 ShieldIconPosition = Vector2(0,0);
RectClass HealthIconUV = RectClass(0,0,0,0);
RectClass ShieldIconUV = RectClass(0,0,0,0);
unsigned long HealthIconColor = 0;
unsigned long ShieldIconColor = 0;
unsigned int HealthIconColorCount = 0;
unsigned int ShieldIconColorCount = 0;
char *ShieldIconTexture = 0;
char *HealthIconTexture = 0;
Render2DClass *HealthIconRender2DClass = 0;
Render2DClass *ShieldIconRender2DClass = 0;
bool RenderHealthIcon = false;
bool RenderShieldIcon = false;
char *BlipTexture;
Vector2 BlipUV[6] = {Vector2(0,0),Vector2(0,0),Vector2(0,0),Vector2(0,0),Vector2(0,0),Vector2(0,0)};
unsigned long BlipColor[8] = {0,0,0,0,0,0,0,0};
Vector2 RadarPosition = Vector2(0,0);
Vector2 BackgroundUV = Vector2(0,0);
char *BackgroundTexture = 0;
unsigned long BackgroundColor = 0;
Render2DClass *BackgroundRender2DClass = 0;
Render2DClass *BlipRender2DClass = 0;
float RadarSize = 0;
float RadarWorldSize = 0;
bool RenderRadar = false;
bool RadarEnabled = false;
bool DrawStar = false;
bool CompassEnabled = false;
Vector2 CompassPosition = Vector2(0,0);
bool RenderCompass = false;
Render2DSentenceClass *CompassRenderer = 0;
unsigned int CompassColor = 0;
bool CreditsEnabled = false;
Vector2 CreditsPosition = Vector2(0,0);
bool RenderCredits = false;
Render2DSentenceClass *CreditsRenderer = 0;
unsigned int CreditsColor = 0;
unsigned int CreditsStringID = 0;
bool TimeEnabled = false;
Vector2 TimePosition = Vector2(0,0);
bool RenderTime = false;
Render2DSentenceClass *TimeRenderer = 0;
unsigned int TimeColor = 0;
unsigned int TimeStringID = 0;
bool ScrollingMap = false;
float MapScale = 0;
char *MapTexture;
Vector2 MapOffset;
extern float CenterClipCountTimer;
void ReadMapINI()
{
	char *c = newstr(The_Game()->MapName);
	char *c2 = strchr(c,'.');
	if (c2)
	{
		c2[1] = 'i';
		c2[2] = 'n';
		c2[3] = 'i';
	}
	INIClass *hudini = Get_INI(c);
	delete[] c;
	if (!hudini)
	{
		return;
	}
	if (ScrollingMap)
	{
		MapScale = hudini->Get_Float("General","ScrollingMapScale",0);
		hudini->Get_String("General","ScrollingMapTexture","",MapTexture,260-1);
		MapOffset.X = hudini->Get_Float("General","ScrollingMapOffsetX",0);
		MapOffset.Y = hudini->Get_Float("General","ScrollingMapOffsetY",0);
		RadarWorldSize = hudini->Get_Float("General","RadarWorldSize",0);
	}
	Release_INI(hudini);
}
void __declspec(dllexport) ReadHUDBits(INIClass *hudini)
{
	unsigned int color = RGB(255,255,255)+0xFF000000;
	Colors.Add(color);
	unsigned int colors = hudini->Get_Int("General","ColorCount",0);
	for (unsigned int i = 0;i < colors;i++)
	{
		char section[20];
		sprintf(section,"Color%d",i+1);
		unsigned int Red = hudini->Get_Int(section,"Red",255);
		unsigned int Green = hudini->Get_Int(section,"Green",255);
		unsigned int Blue = hudini->Get_Int(section,"Blue",255);
		color = RGB(Blue,Green,Red)+0xFF000000;
		Colors.Add(color);
	}

	HealthEnabled = hudini->Get_Bool("General","HealthEnabled",false);
	if (HealthEnabled)
	{
		HideHealthText = true;
		bool HealthVisible = hudini->Get_Bool("General","HealthVisible",true);
		float HealthXPos = hudini->Get_Float("General","HealthXPos",118);
		float HealthYPos = hudini->Get_Float("General","HealthYPos",717);
		float HealthXOffset = hudini->Get_Float("General","HealthXOffset",0);
		float HealthYOffset = hudini->Get_Float("General","HealthYOffset",0);
		if (HealthXPos < 0)
		{
			HealthXPos += Render2DClass::Get_Screen_Resolution().Right;
		}
		if (HealthYPos < 0)
		{
			HealthYPos += Render2DClass::Get_Screen_Resolution().Bottom;
		}
		char HealthFont[100];
		hudini->Get_String("General","HealthFont","FONT12x16.TGA",HealthFont,sizeof(HealthFont));
		unsigned int HealthCol = hudini->Get_Int("General","HealthColor",0);
		HealthColorCount = hudini->Get_Int("General","HealthColorCount",0);
		for (unsigned int i = 0;i < HealthColorCount;i++)
		{
			char section[20];
			sprintf(section,"HealthColor%d",i);
			unsigned int index = hudini->Get_Int(section,"Color",0);
			float value = hudini->Get_Float(section,"Value",0);
			Color *c = new Color;
			c->ColorValue = Colors[index];
			c->Value = value;
			HealthColors.Add(c);
		}
		HealthPosition = Vector2(HealthXPos,HealthYPos);
		HealthOffset = Vector2(HealthXOffset,HealthYOffset);
		RenderHealthText = HealthVisible;
		HealthColor = Colors[HealthCol];
		HealthRender2DClass = CreateRender2DTextClass(HealthFont);
	}

	ShieldEnabled = hudini->Get_Bool("General","ShieldEnabled",false);
	if (ShieldEnabled)
	{
		HideShieldText = true;
		char ShieldFont[100];
		hudini->Get_String("General","ShieldFont","FONT6x8.TGA",ShieldFont,sizeof(ShieldFont));
		float ShieldXPos = hudini->Get_Float("General","ShieldXPos",221);
		float ShieldYPos = hudini->Get_Float("General","ShieldYPos",733);
		float ShieldXOffset = hudini->Get_Float("General","ShieldXOffset",0);
		float ShieldYOffset = hudini->Get_Float("General","ShieldYOffset",0);
		if (ShieldXPos < 0)
		{
			ShieldXPos += Render2DClass::Get_Screen_Resolution().Right;
		}
		if (ShieldYPos < 0)
		{
			ShieldYPos += Render2DClass::Get_Screen_Resolution().Bottom;
		}
		bool ShieldVisible = hudini->Get_Bool("General","ShieldVisible",true);
		unsigned int ShieldCol = hudini->Get_Int("General","ShieldColor",0);
		ShieldColorCount = hudini->Get_Int("General","ShieldColorCount",0);
		for (unsigned int i = 0;i < ShieldColorCount;i++)
		{
			char section[20];
			sprintf(section,"ShieldColor%d",i);
			unsigned int index = hudini->Get_Int(section,"Color",0);
			float value = hudini->Get_Float(section,"Value",0);
			Color *c = new Color;
			c->ColorValue = Colors[index];
			c->Value = value;
			ShieldColors.Add(c);
		}
		ShieldPosition = Vector2(ShieldXPos,ShieldYPos);
		ShieldOffset = Vector2(ShieldXOffset,ShieldYOffset);
		RenderShieldText = ShieldVisible;
		ShieldColor = Colors[ShieldCol];
		ShieldRender2DClass = CreateRender2DTextClass(ShieldFont);
	}

	BulletEnabled = hudini->Get_Bool("General","BulletEnabled",false);
	if (BulletEnabled)
	{
		HideBulletText = true;
		float BulletXPos = hudini->Get_Float("General","BulletXPos",100);
		float BulletYPos = hudini->Get_Float("General","BulletYPos",100);
		if (BulletXPos < 0)
		{
			BulletXPos += Render2DClass::Get_Screen_Resolution().Right;
		}
		if (BulletYPos < 0)
		{
			BulletYPos += Render2DClass::Get_Screen_Resolution().Bottom;
		}
		bool BulletVisible = hudini->Get_Bool("General","BulletVisible",true);
		char BulletFont[100];
		hudini->Get_String("General","BulletFont","FONT12x16.TGA",BulletFont,sizeof(BulletFont));
		unsigned int BulletCol = hudini->Get_Int("General","BulletColor",0);
		BulletPosition = Vector2(BulletXPos,BulletYPos);
		RenderBulletText = BulletVisible;
		BulletColor = Colors[BulletCol];
		BulletRender2DClass = CreateRender2DTextClass(BulletFont);
	}

	ClipEnabled = hudini->Get_Bool("General","ClipEnabled",false);
	if (ClipEnabled)
	{
		HideClipText = true;
		float ClipXPos = hudini->Get_Float("General","ClipXPos",200);
		float ClipYPos = hudini->Get_Float("General","ClipYPos",200);
		if (ClipXPos < 0)
		{
			ClipXPos += Render2DClass::Get_Screen_Resolution().Right;
		}
		if (ClipYPos < 0)
		{
			ClipYPos += Render2DClass::Get_Screen_Resolution().Bottom;
		}
		bool ClipVisible = hudini->Get_Bool("General","ClipVisible",true);
		char ClipFont[100];
		hudini->Get_String("General","ClipFont","FONT12x16.TGA",ClipFont,sizeof(ClipFont));
		unsigned int ClipCol = hudini->Get_Int("General","ClipColor",0);
		ClipPosition = Vector2(ClipXPos,ClipYPos);
		RenderClipText = ClipVisible;
		ClipColor = Colors[ClipCol];
		ClipRender2DClass = CreateRender2DTextClass(ClipFont);
	}

	WeaponEnabled = hudini->Get_Bool("General","WeaponEnabled",false);
	if (WeaponEnabled)
	{
		HideWeaponText = true;
		float WeaponXPos = hudini->Get_Float("General","WeaponXPos",300);
		float WeaponYPos = hudini->Get_Float("General","WeaponYPos",300);
		if (WeaponXPos < 0)
		{
			WeaponXPos += Render2DClass::Get_Screen_Resolution().Right;
		}
		if (WeaponYPos < 0)
		{
			WeaponYPos += Render2DClass::Get_Screen_Resolution().Bottom;
		}
		RenderWeaponText = hudini->Get_Bool("General","WeaponVisible",true);
		RenderWeaponText2 = hudini->Get_Bool("General","WeaponVisibleNonVehicle",true);
		unsigned int WeaponFont = hudini->Get_Int("General","WeaponFont",11);
		WeaponPosition = Vector2(WeaponXPos,WeaponYPos);
		unsigned int WeaponCol = hudini->Get_Int("General","WeaponColor",0);
		WeaponColor = Colors[WeaponCol];
		WeaponNameRenderer2 = CreateRender2DSentenceClass(WeaponFont);
	}

	WeaponImageEnabled = hudini->Get_Bool("General","WeaponImageEnabled",false);
	if (WeaponImageEnabled)
	{
		HideWeaponImage = true;
		float WeaponImageXPos = hudini->Get_Float("General","WeaponImageXPos",300);
		float WeaponImageYPos = hudini->Get_Float("General","WeaponImageYPos",300);
		if (WeaponImageXPos < 0)
		{
			WeaponImageXPos += Render2DClass::Get_Screen_Resolution().Right;
		}
		if (WeaponImageYPos < 0)
		{
			WeaponImageYPos += Render2DClass::Get_Screen_Resolution().Bottom;
		}
		RenderWeaponImage = hudini->Get_Bool("General","WeaponImageVisible",true);
		RenderWeaponImage2 = hudini->Get_Bool("General","WeaponImageVisibleNonVehicle",true);
		WeaponImagePosition = Vector2(WeaponImageXPos,WeaponImageYPos);
		unsigned int WeaponImageCol = hudini->Get_Int("General","WeaponImageColor",0);
		WeaponImageColor = Colors[WeaponImageCol];
		unsigned int WeaponImageVehicleCol = hudini->Get_Int("General","WeaponImageVehicleColor",0);
		WeaponImageVehicleColor = Colors[WeaponImageVehicleCol];
		WeaponImageRender2DClass = CreateRender2DClass();
	}

	TextureCount = hudini->Get_Int("General","TextureCount",0);
	for (unsigned int i = 0;i < TextureCount;i++)
	{
		char section[20];
		sprintf(section,"Texture%d",i);
		Texture *t = new Texture;
		t->TextureName = new char[260];
		hudini->Get_String(section,"TextureName","",t->TextureName,260-1);
		t->TextureRender2DClass = CreateRender2DClass();
		t->TextureRender2DClass->Set_Texture(t->TextureName);
		t->QuadCount = hudini->Get_Int(section,"QuadCount",0);
		t->Quads = new Quad[t->QuadCount];
		if (t->TextureRender2DClass->Peek_Texture()->Initialized == false)
		{
			t->TextureRender2DClass->Peek_Texture()->Init();
		}
		unsigned int sz = t->TextureRender2DClass->Peek_Texture()->Width;
		float size = (float)(1.0 / sz);
		for (unsigned int j = 0;j < t->QuadCount;j++)
		{
			char entry[50];
			sprintf(entry,"Quad%dColor",j);
			t->Quads[j].Color = Colors[hudini->Get_Int(section,entry,0)];
			sprintf(entry,"Quad%dXPos",j);
			float QuadXPos = hudini->Get_Float(section,entry,0);
			sprintf(entry,"Quad%dYPos",j);
			float QuadYPos = hudini->Get_Float(section,entry,0);
			sprintf(entry,"Quad%dXCenter",j);
			bool QuadXCenter = hudini->Get_Bool(section,entry,false);
			sprintf(entry,"Quad%dYCenter",j);
			bool QuadYCenter = hudini->Get_Bool(section,entry,false);
			if (QuadXCenter)
			{
				QuadXPos = (Render2DClass::Get_Screen_Resolution().Right / 2) + QuadXPos;
			}
			else
			{
				if (QuadXPos < 0)
				{
					QuadXPos += Render2DClass::Get_Screen_Resolution().Right;
				}
			}
			if (QuadYCenter)
			{
				QuadYPos = (Render2DClass::Get_Screen_Resolution().Bottom / 2) + QuadYPos;
			}
			else
			{
				if (QuadYPos < 0)
				{
					QuadYPos += Render2DClass::Get_Screen_Resolution().Bottom;
				}
			}
			sprintf(entry,"Quad%dTop",j);
			float QuadTop = hudini->Get_Float(section,entry,0);
			sprintf(entry,"Quad%dLeft",j);
			float QuadLeft = hudini->Get_Float(section,entry,0);
			sprintf(entry,"Quad%dBottom",j);
			float QuadBottom = hudini->Get_Float(section,entry,0);
			sprintf(entry,"Quad%dRight",j);
			float QuadRight = hudini->Get_Float(section,entry,0);
			float QuadXSize = QuadRight - QuadLeft;
			float QuadYSize = QuadBottom - QuadTop;
			t->Quads[j].Position.Top = QuadYPos;
			t->Quads[j].Position.Left = QuadXPos;
			t->Quads[j].Position.Bottom = QuadYPos + QuadYSize;
			t->Quads[j].Position.Right = QuadXPos + QuadXSize;
			t->Quads[j].UV.Top = QuadTop * size;
			t->Quads[j].UV.Left = QuadLeft * size;
			t->Quads[j].UV.Bottom = QuadBottom * size;
			t->Quads[j].UV.Right = QuadRight * size;
		}
		HudTextures.Add(t);
	}

	HealthBarEnabled = hudini->Get_Bool("General","HealthBarEnabled",false);
	if (HealthBarEnabled)
	{
		unsigned int HealthBarCol = hudini->Get_Int("General","HealthBarColor",0);
		unsigned int HealthBarEmptyCol = hudini->Get_Int("General","HealthBarEmptyColor",0);
		HealthBarColorCount = hudini->Get_Int("General","HealthBarColorCount",0);
		for (unsigned int i = 0;i < HealthBarColorCount;i++)
		{
			char section[20];
			sprintf(section,"HealthBarColor%d",i);
			unsigned int index = hudini->Get_Int(section,"Color",0);
			float value = hudini->Get_Float(section,"Value",0);
			Color *c = new Color;
			c->ColorValue = Colors[index];
			c->Value = value;
			HealthBarColors.Add(c);
		}
		HealthBarColor = Colors[HealthBarCol];
		HealthBarEmptyColor = Colors[HealthBarEmptyCol];
		HealthBarRender2DClass = CreateRender2DClass();
		HealthBarTexture = new char[260];
		hudini->Get_String("General","HealthBarTexture","",HealthBarTexture,260-1);
		float HealthBarXPos = hudini->Get_Float("General","HealthBarXPos",300);
		float HealthBarYPos = hudini->Get_Float("General","HealthBarYPos",300);
		if (HealthBarXPos < 0)
		{
			HealthBarXPos += Render2DClass::Get_Screen_Resolution().Right;
		}
		if (HealthBarYPos < 0)
		{
			HealthBarYPos += Render2DClass::Get_Screen_Resolution().Bottom;
		}
		HealthBarPosition = Vector2(HealthBarXPos,HealthBarYPos);
		float HealthBarTop = hudini->Get_Float("General","HealthBarTop",0);
		float HealthBarBottom = hudini->Get_Float("General","HealthBarBottom",0);
		float HealthBarLeft = hudini->Get_Float("General","HealthBarLeft",0);
		float HealthBarRight = hudini->Get_Float("General","HealthBarRight",0);
		HealthBarUV = RectClass(HealthBarLeft,HealthBarTop,HealthBarRight,HealthBarBottom);
		float HealthBarEmptyTop = hudini->Get_Float("General","HealthBarEmptyTop",HealthBarTop);
		float HealthBarEmptyLeft = hudini->Get_Float("General","HealthBarEmptyLeft",HealthBarLeft);
		HealthBarEmptyUV = Vector2(HealthBarEmptyLeft,HealthBarEmptyTop);
		RenderHealthBar = hudini->Get_Bool("General","HealthBarVisible",true);
		HealthBarStyle = hudini->Get_Int("General","HealthBarStyle",0);
		HealthBarDirection = hudini->Get_Int("General","HealthBarDirection",1);
		HealthBarOffset = hudini->Get_Float("General","HealthBarOffset",10);
		if (!HealthBarOffset)
		{
			HealthBarOffset++;
		}
		HealthBarLength = hudini->Get_Float("General","HealthBarLength",500);
		RenderHealthBarEmpty = hudini->Get_Bool("General","HealthBarEmptyVisible",true);
	}

	ShieldBarEnabled = hudini->Get_Bool("General","ShieldBarEnabled",false);
	if (ShieldBarEnabled)
	{
		unsigned int ShieldBarCol = hudini->Get_Int("General","ShieldBarColor",0);
		unsigned int ShieldBarEmptyCol = hudini->Get_Int("General","ShieldBarEmptyColor",0);
		ShieldBarColorCount = hudini->Get_Int("General","ShieldBarColorCount",0);
		for (unsigned int i = 0;i < ShieldBarColorCount;i++)
		{
			char section[20];
			sprintf(section,"ShieldBarColor%d",i);
			unsigned int index = hudini->Get_Int(section,"Color",0);
			float value = hudini->Get_Float(section,"Value",0);
			Color *c = new Color;
			c->ColorValue = Colors[index];
			c->Value = value;
			ShieldBarColors.Add(c);
		}
		ShieldBarColor = Colors[ShieldBarCol];
		ShieldBarEmptyColor = Colors[ShieldBarEmptyCol];
		ShieldBarRender2DClass = CreateRender2DClass();
		ShieldBarTexture = new char[260];
		hudini->Get_String("General","ShieldBarTexture","",ShieldBarTexture,260-1);
		float ShieldBarXPos = hudini->Get_Float("General","ShieldBarXPos",300);
		float ShieldBarYPos = hudini->Get_Float("General","ShieldBarYPos",300);
		if (ShieldBarXPos < 0)
		{
			ShieldBarXPos += Render2DClass::Get_Screen_Resolution().Right;
		}
		if (ShieldBarYPos < 0)
		{
			ShieldBarYPos += Render2DClass::Get_Screen_Resolution().Bottom;
		}
		ShieldBarPosition = Vector2(ShieldBarXPos,ShieldBarYPos);
		float ShieldBarTop = hudini->Get_Float("General","ShieldBarTop",0);
		float ShieldBarBottom = hudini->Get_Float("General","ShieldBarBottom",0);
		float ShieldBarLeft = hudini->Get_Float("General","ShieldBarLeft",0);
		float ShieldBarRight = hudini->Get_Float("General","ShieldBarRight",0);
		ShieldBarUV = RectClass(ShieldBarLeft,ShieldBarTop,ShieldBarRight,ShieldBarBottom);
		float ShieldBarEmptyTop = hudini->Get_Float("General","ShieldBarEmptyTop",ShieldBarTop);
		float ShieldBarEmptyLeft = hudini->Get_Float("General","ShieldBarEmptyLeft",ShieldBarLeft);
		ShieldBarEmptyUV = Vector2(ShieldBarEmptyLeft,ShieldBarEmptyTop);
		RenderShieldBar = hudini->Get_Bool("General","ShieldBarVisible",true);
		ShieldBarStyle = hudini->Get_Int("General","ShieldBarStyle",0);
		ShieldBarDirection = hudini->Get_Int("General","ShieldBarDirection",1);
		ShieldBarOffset = hudini->Get_Float("General","ShieldBarOffset",10);
		if (!ShieldBarOffset)
		{
			ShieldBarOffset++;
		}
		ShieldBarLength = hudini->Get_Float("General","ShieldBarLength",500);
		RenderShieldBarEmpty = hudini->Get_Bool("General","ShieldBarEmptyVisible",true);
	}

	CompassEnabled = hudini->Get_Bool("General","EnableCompass",false);
	if (CompassEnabled)
	{
		HideCompass = true;
		float CompassXPos = hudini->Get_Float("General","CompassXPos",300);
		float CompassYPos = hudini->Get_Float("General","CompassYPos",300);
		if (CompassXPos < 0)
		{
			CompassXPos += Render2DClass::Get_Screen_Resolution().Right;
		}
		if (CompassYPos < 0)
		{
			CompassYPos += Render2DClass::Get_Screen_Resolution().Bottom;
		}
		RenderCompass = hudini->Get_Bool("General","CompassVisible",true);
		unsigned int CompassFont = hudini->Get_Int("General","CompassFont",11);
		CompassPosition = Vector2(CompassXPos,CompassYPos);
		unsigned int CompassCol = hudini->Get_Int("General","CompassColor",0);
		CompassColor = Colors[CompassCol];
		CompassRenderer = CreateRender2DSentenceClass(CompassFont);
	}

	CreditsEnabled = hudini->Get_Bool("General","EnableCredits",false);
	if (CreditsEnabled)
	{
		RenderCredits = true;
		float CreditsXPos = hudini->Get_Float("General","CreditsXPos",300);
		float CreditsYPos = hudini->Get_Float("General","CreditsYPos",300);
		if (CreditsXPos < 0)
		{
			CreditsXPos += Render2DClass::Get_Screen_Resolution().Right;
		}
		if (CreditsYPos < 0)
		{
			CreditsYPos += Render2DClass::Get_Screen_Resolution().Bottom;
		}
		unsigned int CreditsFont = hudini->Get_Int("General","CreditsFont",11);
		CreditsPosition = Vector2(CreditsXPos,CreditsYPos);
		unsigned int CreditsCol = hudini->Get_Int("General","CreditsColor",0);
		CreditsColor = Colors[CreditsCol];
		CreditsRenderer = CreateRender2DSentenceClass(CreditsFont);
		CreditsStringID = hudini->Get_Int("General","CreditsStringID",0);
	}

	TimeEnabled = hudini->Get_Bool("General","EnableTime",false);
	if (TimeEnabled)
	{
		RenderTime = true;
		float TimeXPos = hudini->Get_Float("General","TimeXPos",300);
		float TimeYPos = hudini->Get_Float("General","TimeYPos",300);
		if (TimeXPos < 0)
		{
			TimeXPos += Render2DClass::Get_Screen_Resolution().Right;
		}
		if (TimeYPos < 0)
		{
			TimeYPos += Render2DClass::Get_Screen_Resolution().Bottom;
		}
		unsigned int TimeFont = hudini->Get_Int("General","TimeFont",11);
		TimePosition = Vector2(TimeXPos,TimeYPos);
		unsigned int TimeCol = hudini->Get_Int("General","TimeColor",0);
		TimeColor = Colors[TimeCol];
		TimeRenderer = CreateRender2DSentenceClass(TimeFont);
		TimeStringID = hudini->Get_Int("General","TimeStringID",0);
	}

	RadarEnabled = hudini->Get_Bool("General","EnableRadar",false);
	if (RadarEnabled)
	{
		HideRadar = true;
		DrawStar = hudini->Get_Bool("General","DrawStar",false);
		RenderRadar = hudini->Get_Bool("General","RadarVisible",true);
		RadarSize = hudini->Get_Float("General","RadarSize",0);
		RadarWorldSize = hudini->Get_Float("General","RadarWorldSize",0);
		unsigned long BackgroundCol = hudini->Get_Int("General","BackgroundColor",0);
		BackgroundColor = Colors[BackgroundCol];
		BackgroundTexture = new char[260];
		hudini->Get_String("General","BackgroundTexture","",BackgroundTexture,260-1);
		BlipTexture = new char[260];
		hudini->Get_String("General","BlipTexture","",BlipTexture,260-1);
		BackgroundRender2DClass = CreateRender2DClass();
		BlipRender2DClass = CreateRender2DClass();
		float BackgroundTop = hudini->Get_Float("General","BackgroundTop",0);
		float BackgroundLeft = hudini->Get_Float("General","BackgroundLeft",0);
		BackgroundUV = Vector2(BackgroundLeft,BackgroundTop);
		float RadarX = hudini->Get_Float("General","RadarX",0);
		float RadarY = hudini->Get_Float("General","RadarY",0);
		if (RadarX < 0)
		{
			RadarX += Render2DClass::Get_Screen_Resolution().Right;
		}
		if (RadarY < 0)
		{
			RadarY += Render2DClass::Get_Screen_Resolution().Bottom;
		}
		RadarPosition = Vector2(RadarX,RadarY);
		BlipUV[0] = Vector2(0,0);
		for (unsigned int i = 1; i < 6;i++)
		{
			char entry[50];
			sprintf(entry,"RadarBlip%dTop",i);
			float BlipTop = hudini->Get_Float("General",entry,0);
			sprintf(entry,"RadarBlip%dLeft",i);
			float BlipLeft = hudini->Get_Float("General",entry,0);
			BlipUV[i] = Vector2(BlipLeft,BlipTop);
		}
		for (unsigned int i = 0;i < 8;i++)
		{
			char entry[50];
			sprintf(entry,"RadarBlipColor%d",i);
			unsigned int BlipCol = hudini->Get_Int("General",entry,0);
			BlipColor[i] = Colors[BlipCol];
		}
		ScrollingMap = hudini->Get_Bool("General","ScrollingRadarMap",false);
		if (ScrollingMap)
		{
			MapScale = hudini->Get_Float("General","ScrollingMapScale",0);
			hudini->Get_String("General","ScrollingMapTexture","",MapTexture,260-1);
			MapOffset.X = hudini->Get_Float("General","ScrollingMapOffsetX",0);
			MapOffset.Y = hudini->Get_Float("General","ScrollingMapOffsetY",0);
		}
	}

	HealthIconEnabled = hudini->Get_Bool("General","HealthIconEnabled",false);
	if (HealthIconEnabled)
	{
		unsigned int HealthIconCol = hudini->Get_Int("General","HealthIconColor",0);
		HealthIconColorCount = hudini->Get_Int("General","HealthIconColorCount",0);
		for (unsigned int i = 0;i < HealthIconColorCount;i++)
		{
			char section[20];
			sprintf(section,"HealthIconColor%d",i);
			unsigned int index = hudini->Get_Int(section,"Color",0);
			float value = hudini->Get_Float(section,"Value",0);
			Color *c = new Color;
			c->ColorValue = Colors[index];
			c->Value = value;
			HealthIconColors.Add(c);
		}
		HealthIconColor = Colors[HealthIconCol];
		HealthIconRender2DClass = CreateRender2DClass();
		HealthIconTexture = new char[260];
		hudini->Get_String("General","HealthIconTexture","",HealthIconTexture,260-1);
		float HealthIconXPos = hudini->Get_Float("General","HealthIconXPos",300);
		float HealthIconYPos = hudini->Get_Float("General","HealthIconYPos",300);
		if (HealthIconXPos < 0)
		{
			HealthIconXPos += Render2DClass::Get_Screen_Resolution().Right;
		}
		if (HealthIconYPos < 0)
		{
			HealthIconYPos += Render2DClass::Get_Screen_Resolution().Bottom;
		}
		HealthIconPosition = Vector2(HealthIconXPos,HealthIconYPos);
		float HealthIconTop = hudini->Get_Float("General","HealthIconTop",0);
		float HealthIconBottom = hudini->Get_Float("General","HealthIconBottom",0);
		float HealthIconLeft = hudini->Get_Float("General","HealthIconLeft",0);
		float HealthIconRight = hudini->Get_Float("General","HealthIconRight",0);
		HealthIconUV = RectClass(HealthIconLeft,HealthIconTop,HealthIconRight,HealthIconBottom);
		RenderHealthIcon = true;
	}

	ShieldIconEnabled = hudini->Get_Bool("General","ShieldIconEnabled",false);
	if (ShieldIconEnabled)
	{
		unsigned int ShieldIconCol = hudini->Get_Int("General","ShieldIconColor",0);
		ShieldIconColorCount = hudini->Get_Int("General","ShieldIconColorCount",0);
		for (unsigned int i = 0;i < ShieldIconColorCount;i++)
		{
			char section[20];
			sprintf(section,"ShieldIconColor%d",i);
			unsigned int index = hudini->Get_Int(section,"Color",0);
			float value = hudini->Get_Float(section,"Value",0);
			Color *c = new Color;
			c->ColorValue = Colors[index];
			c->Value = value;
			ShieldIconColors.Add(c);
		}
		ShieldIconColor = Colors[ShieldIconCol];
		ShieldIconRender2DClass = CreateRender2DClass();
		ShieldIconTexture = new char[260];
		hudini->Get_String("General","ShieldIconTexture","",ShieldIconTexture,260-1);
		float ShieldIconXPos = hudini->Get_Float("General","ShieldIconXPos",300);
		float ShieldIconYPos = hudini->Get_Float("General","ShieldIconYPos",300);
		if (ShieldIconXPos < 0)
		{
			ShieldIconXPos += Render2DClass::Get_Screen_Resolution().Right;
		}
		if (ShieldIconYPos < 0)
		{
			ShieldIconYPos += Render2DClass::Get_Screen_Resolution().Bottom;
		}
		ShieldIconPosition = Vector2(ShieldIconXPos,ShieldIconYPos);
		float ShieldIconTop = hudini->Get_Float("General","ShieldIconTop",0);
		float ShieldIconBottom = hudini->Get_Float("General","ShieldIconBottom",0);
		float ShieldIconLeft = hudini->Get_Float("General","ShieldIconLeft",0);
		float ShieldIconRight = hudini->Get_Float("General","ShieldIconRight",0);
		ShieldIconUV = RectClass(ShieldIconLeft,ShieldIconTop,ShieldIconRight,ShieldIconBottom);
		RenderShieldIcon = true;
	}
	HideWeaponBox = hudini->Get_Bool("General","HideWeaponBox",false);
	HideInfoBox = hudini->Get_Bool("General","HideInfoBox",false);
	HideCenter = hudini->Get_Bool("General","HideCenter",false);
	HidePowerupIcons = hudini->Get_Bool("General","HidePowerupIcons",false);
	WeaponChartColor = hudini->Get_Color_UInt("General", "WeaponChartColor", GetColorUInt(0, 0, 255, 0));
	WeaponChartEmptyColor = hudini->Get_Color_UInt("General", "WeaponChartEmptyColor", GetColorUInt(0, 255, 255, 0));
	PhysDecalSysClass::decalpoolsize = hudini->Get_Int("General","DecalLimit",50);
	PhysicsSceneClass::Get_Instance()->Release_Decal_Resources();
	PhysicsSceneClass::Get_Instance()->Allocate_Decal_Resources();
	int MaxStaticLODBudget = hudini->Get_Int("General","MaxStaticLODBudget",10000);
	int MaxDynamicLODBudget = hudini->Get_Int("General","MaxDynamicLODBudget",10000);
	MaxStaticLODBudget = max(MaxStaticLODBudget,65535);
	MaxDynamicLODBudget = max(MaxDynamicLODBudget,65535);
	RegistryClass registry(Build_Registry_Location_String((char *)0x007F5274,0,"System Settings"),true);
	SystemSettingEntryDynamicLODBudget *dynlod = new SystemSettingEntryDynamicLODBudget();
	dynlod->Set_Range(100,MaxDynamicLODBudget);
	dynlod->Set_Step_Size(100);
	dynlod->Set_Value(10000);
	dynlod->Registry_Load(registry);
	delete dynlod;
	SystemSettingEntryStaticLODBudget *statlod = new SystemSettingEntryStaticLODBudget();
	statlod->Set_Range(100,MaxStaticLODBudget);
	statlod->Set_Step_Size(100);
	statlod->Set_Value(3000);
	statlod->Registry_Load(registry);
	delete statlod;
}

void UpdateHUD2()
{
	for (unsigned int i = 0;i < TextureCount;i++)
	{
		HudTextures[i]->TextureRender2DClass->Reset();
		HudTextures[i]->TextureRender2DClass->Set_Texture(HudTextures[i]->TextureName);
		for (unsigned int j = 0;j < HudTextures[i]->QuadCount;j++)
		{
			HudTextures[i]->TextureRender2DClass->Add_Quad(HudTextures[i]->Quads[j].Position,HudTextures[i]->Quads[j].UV,HudTextures[i]->Quads[j].Color);
		}
		HudTextures[i]->TextureRender2DClass->Render();
	}
	GameObject *obj = Get_Vehicle_Return((GameObject *)(CombatManager::Get_The_Star()));
	GameObject *soldier = (GameObject *)CombatManager::Get_The_Star();
	if (RenderHealthBar)
	{
		HealthBarRender2DClass->Reset();
		HealthBarRender2DClass->Set_Texture(HealthBarTexture);
		float health = Commands->Get_Health(obj);
		float maxhealth = Commands->Get_Max_Health(obj);
		float healthpercent = (health / (maxhealth / 100));
		unsigned int color = 0;
		color = HealthBarColor;
		for (unsigned int i = 0;i < HealthBarColorCount;i++)
		{
			if (healthpercent <= HealthBarColors[i]->Value)
			{
				color = HealthBarColors[i]->ColorValue;
				break;
			}
		}
		if (HealthBarRender2DClass->Peek_Texture()->Initialized == false)
		{
			HealthBarRender2DClass->Peek_Texture()->Init();
		}
		unsigned int sz = HealthBarRender2DClass->Peek_Texture()->Width;
		float size = (float)(1.0 / sz);
		float xsize = HealthBarUV.Right-HealthBarUV.Left;
		float ysize = HealthBarUV.Bottom-HealthBarUV.Top;
		if (!HealthBarStyle)
		{
			float fullxsize = xsize * (healthpercent / 100);
			RectClass UV,Position,EmptyUV,EmptyPosition;
			if (healthpercent > 0)
			{
				UV = RectClass(HealthBarUV.Left*size,HealthBarUV.Top*size,(HealthBarUV.Left+fullxsize)*size,HealthBarUV.Bottom*size);
				Position = RectClass(HealthBarPosition.X,HealthBarPosition.Y,HealthBarPosition.X+fullxsize,HealthBarPosition.Y+ysize);
				HealthBarRender2DClass->Add_Quad(Position,UV,color);
			}
			if (healthpercent < 100)
			{
				if (RenderHealthBarEmpty)
				{
					EmptyUV = RectClass((HealthBarEmptyUV.X+fullxsize)*size,HealthBarEmptyUV.Y*size,(HealthBarEmptyUV.X+xsize)*size,(HealthBarEmptyUV.Y+ysize)*size);
					EmptyPosition = RectClass(HealthBarPosition.X+fullxsize,HealthBarPosition.Y,HealthBarPosition.X+xsize,HealthBarPosition.Y+ysize);
					HealthBarRender2DClass->Add_Quad(EmptyPosition,EmptyUV,HealthBarEmptyColor);
				}
			}
		}
		else
		{
			RectClass UV;
			RectClass Position;
			UV = RectClass(HealthBarUV.Left*size,HealthBarUV.Top*size,HealthBarUV.Right*size,HealthBarUV.Bottom*size);
			Position.Top = HealthBarPosition.Y;
			Position.Bottom = Position.Top+ysize;
			if (!HealthBarDirection)
			{
				float length = HealthBarLength * (healthpercent / 100);
				Position.Left = HealthBarPosition.X;
				Position.Right = Position.Left + xsize;
				do
				{
					HealthBarRender2DClass->Add_Quad(Position,UV,color);
					Position.Left += HealthBarOffset;
					Position.Right = Position.Left + xsize;
				} while (Position.Right < HealthBarPosition.X + length);
				Position.Left = (HealthBarPosition.X) + length;
				Position.Left -= xsize;
				Position.Right = Position.Left + xsize;
				if (Position.Left > HealthBarPosition.X)
				{
					HealthBarRender2DClass->Add_Quad(Position,UV,color);
				}
			}
			else if (health > 0)
			{
				float length = HealthBarLength - (HealthBarLength * (healthpercent / 100));
				Position.Right = HealthBarPosition.X + HealthBarLength;
				Position.Left = Position.Right - xsize;
				if (Position.Left < HealthBarPosition.X + length - xsize)
				{
					Position.Left = HealthBarPosition.X + length - xsize;
					Position.Right = Position.Left + xsize;
				}
				do
				{
					HealthBarRender2DClass->Add_Quad(Position,UV,color);
					Position.Right -= HealthBarOffset;
					Position.Left = Position.Right - xsize;
				} while (Position.Left > HealthBarPosition.X + length - xsize);
				Position.Right = HealthBarPosition.X + length;
				Position.Left = Position.Right + xsize;
				if (Position.Right < HealthBarPosition.X + HealthBarLength)
				{
					HealthBarRender2DClass->Add_Quad(Position,UV,color);
				}
			}
		}
		HealthBarRender2DClass->Render();
	}
	if (RenderShieldBar)
	{
		ShieldBarRender2DClass->Reset();
		ShieldBarRender2DClass->Set_Texture(ShieldBarTexture);
		float shield = Commands->Get_Shield_Strength(obj);
		float maxshield = Commands->Get_Max_Shield_Strength(obj);
		float shieldpercent = (shield / (maxshield / 100));
		unsigned int color = 0;
		color = ShieldBarColor;
		for (unsigned int i = 0;i < ShieldBarColorCount;i++)
		{
			if (shieldpercent <= ShieldBarColors[i]->Value)
			{
				color = ShieldBarColors[i]->ColorValue;
				break;
			}
		}
		if (ShieldBarRender2DClass->Peek_Texture()->Initialized == false)
		{
			ShieldBarRender2DClass->Peek_Texture()->Init();
		}
		unsigned int sz = ShieldBarRender2DClass->Peek_Texture()->Width;
		float size = (float)(1.0 / sz);
		float xsize = ShieldBarUV.Right-ShieldBarUV.Left;
		float ysize = ShieldBarUV.Bottom-ShieldBarUV.Top;
		if (!ShieldBarStyle)
		{
			float fullxsize = xsize * (shieldpercent / 100);
			RectClass UV,Position,EmptyUV,EmptyPosition;
			if (shieldpercent > 0)
			{
				UV = RectClass(ShieldBarUV.Left*size,ShieldBarUV.Top*size,(ShieldBarUV.Left+fullxsize)*size,ShieldBarUV.Bottom*size);
				Position = RectClass(ShieldBarPosition.X,ShieldBarPosition.Y,ShieldBarPosition.X+fullxsize,ShieldBarPosition.Y+ysize);
				ShieldBarRender2DClass->Add_Quad(Position,UV,color);
			}
			if (shieldpercent < 100)
			{
				if (RenderShieldBarEmpty)
				{
					EmptyUV = RectClass((ShieldBarEmptyUV.X+fullxsize)*size,ShieldBarEmptyUV.Y*size,(ShieldBarEmptyUV.X+xsize)*size,(ShieldBarEmptyUV.Y+ysize)*size);
					EmptyPosition = RectClass(ShieldBarPosition.X+fullxsize,ShieldBarPosition.Y,ShieldBarPosition.X+xsize,ShieldBarPosition.Y+ysize);
					ShieldBarRender2DClass->Add_Quad(EmptyPosition,EmptyUV,ShieldBarEmptyColor);
				}
			}
		}
		else if (shield > 0)
		{
			RectClass UV;
			RectClass Position;
			UV = RectClass(ShieldBarUV.Left*size,ShieldBarUV.Top*size,ShieldBarUV.Right*size,ShieldBarUV.Bottom*size);
			Position.Top = ShieldBarPosition.Y;
			Position.Bottom = Position.Top+ysize;
			if (!ShieldBarDirection)
			{
				float length = ShieldBarLength * (shieldpercent / 100);
				Position.Left = ShieldBarPosition.X;
				Position.Right = Position.Left + xsize;
				do
				{
					ShieldBarRender2DClass->Add_Quad(Position,UV,color);
					Position.Left += ShieldBarOffset;
					Position.Right = Position.Left + xsize;
				} while (Position.Right < ShieldBarPosition.X + length);
				Position.Left = (ShieldBarPosition.X) + length;
				Position.Left -= xsize;
				Position.Right = Position.Left + xsize;
				if (Position.Left > ShieldBarPosition.X)
				{
					ShieldBarRender2DClass->Add_Quad(Position,UV,color);
				}
			}
			else
			{
				float length = ShieldBarLength - (ShieldBarLength * (shieldpercent / 100));
				Position.Right = ShieldBarPosition.X + ShieldBarLength;
				Position.Left = Position.Right - xsize;
				if (Position.Left < ShieldBarPosition.X + length - xsize)
				{
					Position.Left = ShieldBarPosition.X + length - xsize;
					Position.Right = Position.Left + xsize;
				}
				do
				{
					ShieldBarRender2DClass->Add_Quad(Position,UV,color);
					Position.Right -= ShieldBarOffset;
					Position.Left = Position.Right - xsize;
				} while (Position.Left > ShieldBarPosition.X + length - xsize);
				Position.Right = ShieldBarPosition.X + length;
				Position.Left = Position.Right + xsize;
				if (Position.Right < ShieldBarPosition.X + ShieldBarLength)
				{
					ShieldBarRender2DClass->Add_Quad(Position,UV,color);
				}
			}
		}
		ShieldBarRender2DClass->Render();
	}
	if (RenderHealthIcon)
	{
		HealthIconRender2DClass->Reset();
		HealthIconRender2DClass->Set_Texture(HealthIconTexture);
		float health = Commands->Get_Health(obj);
		float maxhealth = Commands->Get_Max_Health(obj);
		float healthpercent = (health / (maxhealth / 100));
		unsigned int color = 0;
		color = HealthIconColor;
		for (unsigned int i = 0;i < HealthIconColorCount;i++)
		{
			if (healthpercent <= HealthIconColors[i]->Value)
			{
				color = HealthIconColors[i]->ColorValue;
				break;
			}
		}
		if (HealthIconRender2DClass->Peek_Texture()->Initialized == false)
		{
			HealthIconRender2DClass->Peek_Texture()->Init();
		}
		unsigned int sz = HealthIconRender2DClass->Peek_Texture()->Width;
		float size = (float)(1.0 / sz);
		float xsize = HealthIconUV.Right-HealthIconUV.Left;
		float ysize = HealthIconUV.Bottom-HealthIconUV.Top;
		RectClass UV;
		RectClass Position;
		UV = RectClass(HealthIconUV.Left*size,HealthIconUV.Top*size,HealthIconUV.Right*size,HealthIconUV.Bottom*size);
		Position.Top = HealthIconPosition.Y;
		Position.Bottom = Position.Top+ysize;
		Position.Left = HealthIconPosition.X;
		Position.Right = Position.Left+xsize;
		HealthIconRender2DClass->Add_Quad(Position,UV,color);
		HealthIconRender2DClass->Render();
	}
	if (RenderShieldIcon)
	{
		ShieldIconRender2DClass->Reset();
		ShieldIconRender2DClass->Set_Texture(ShieldIconTexture);
		float shield = Commands->Get_Shield_Strength(obj);
		float maxshield = Commands->Get_Max_Shield_Strength(obj);
		float shieldpercent = (shield / (maxshield / 100));
		unsigned int color = 0;
		color = ShieldIconColor;
		for (unsigned int i = 0;i < ShieldIconColorCount;i++)
		{
			if (shieldpercent <= ShieldIconColors[i]->Value)
			{
				color = ShieldIconColors[i]->ColorValue;
				break;
			}
		}
		if (ShieldIconRender2DClass->Peek_Texture()->Initialized == false)
		{
			ShieldIconRender2DClass->Peek_Texture()->Init();
		}
		unsigned int sz = ShieldIconRender2DClass->Peek_Texture()->Width;
		float size = (float)(1.0 / sz);
		float xsize = ShieldIconUV.Right-ShieldIconUV.Left;
		float ysize = ShieldIconUV.Bottom-ShieldIconUV.Top;
		RectClass UV;
		RectClass Position;
		UV = RectClass(ShieldIconUV.Left*size,ShieldIconUV.Top*size,ShieldIconUV.Right*size,ShieldIconUV.Bottom*size);
		Position.Top = ShieldIconPosition.Y;
		Position.Bottom = Position.Top+ysize;
		Position.Left = ShieldIconPosition.X;
		Position.Right = Position.Left+xsize;
		ShieldIconRender2DClass->Add_Quad(Position,UV,color);
		ShieldIconRender2DClass->Render();
	}
	if ((RenderWeaponText) || ((RenderWeaponText2) && (!Get_Vehicle(soldier))))
	{
		GameObject *o = Get_Vehicle(soldier);
		const wchar_t *name;
		if (o)
		{
			name = Get_Wide_Vehicle_Name(o);
		}
		else
		{
			name = Get_Current_Wide_Translated_Weapon(soldier);
		}
		Render2DSentenceDrawSentence(WeaponNameRenderer2,name,&WeaponPosition,WeaponColor);
	}
	if (RenderCompass)
	{
		const wchar_t *str = Get_Wide_Translated_String(12640+RadarManager::CurrentCompassRendererIndex);
		Vector2 v = CompassPosition;
		Vector2 sz = CompassRenderer->Get_Text_Extents(str);
		v.X -= (sz.X / 2);
		v.X = floor(v.X);
		Render2DSentenceDrawSentence(CompassRenderer,str,&v,CompassColor);
	}
	if (RenderCredits)
	{
		const wchar_t *str = Get_Wide_Translated_String(CreditsStringID);
		wchar_t str2[100];
		swprintf(str2,100,str,(unsigned int)Commands->Get_Money(soldier));
		Render2DSentenceDrawSentence(CreditsRenderer,str2,&CreditsPosition,CreditsColor);
	}
	if (RenderTime)
	{
		if (The_Game()->TimeLimit_Minutes)
		{
			float secs = The_Game()->TimeRemaining_Seconds;
			const wchar_t *str = Get_Wide_Translated_String(TimeStringID);
			int hours,minutes,seconds;
			Seconds_To_Hms(secs,hours,minutes,seconds);
			wchar_t str2[100];
			wchar_t str3[100];
			swprintf(str3,100,L"%02d:%02d:%02d",hours,minutes,seconds);
			swprintf(str2,100,str,str3);
			Render2DSentenceDrawSentence(TimeRenderer,str2,&TimePosition,TimeColor);
		}
	}
	if (RenderHealthText)
	{
		HealthRender2DClass->Reset();
		RectClass *r = (RectClass *)((char *)HealthRender2DClass+0x5B8);
		float health = Commands->Get_Health(obj);
		float maxhealth = Commands->Get_Max_Health(obj);
		float healthpercent = (health / (maxhealth / 100));
		r->Right = HealthPosition.X + (healthpercent * HealthOffset.X);
		r->Left = HealthPosition.X + (healthpercent * HealthOffset.X);
		r->Top = HealthPosition.Y + (healthpercent * HealthOffset.Y);
		r->Bottom = HealthPosition.Y + (healthpercent * HealthOffset.Y);
		char text[10];
		unsigned int h = WWMath::Float_To_Long(health);
		sprintf(text,"%d",h);
		unsigned int color = 0;
		if (!HealthColorCount)
		{
			color = HealthColor;
		}
		else
		{
			for (unsigned int i = 0;i < HealthColorCount;i++)
			{
				if (healthpercent < HealthColors[i]->Value)
				{
					color = HealthColors[i]->ColorValue;
					break;
				}
			}
		}
		HealthRender2DClass->Draw_Text(text,color);
		HealthRender2DClass->Render();
	}
	if (RenderBulletText)
	{
		BulletRender2DClass->Reset();
		RectClass *r = (RectClass *)((char *)BulletRender2DClass+0x5B8);
		unsigned int bullets = Get_Current_Bullets(obj);
		if (bullets == -1)
		{
			bullets = 999;
		}
		r->Right = BulletPosition.X;
		r->Left = BulletPosition.X;
		r->Top = BulletPosition.Y;
		r->Bottom = BulletPosition.Y;
		char text[10];
		sprintf(text,"%d",bullets);
		BulletRender2DClass->Draw_Text(text,BulletColor);
		BulletRender2DClass->Render();
	}
	if (RenderClipText)
	{
		ClipRender2DClass->Reset();
		RectClass *r = (RectClass *)((char *)ClipRender2DClass+0x5B8);
		unsigned int Clips = Get_Current_Clip_Bullets(obj);
		if (Clips == -1)
		{
			Clips = 999;
		}
		r->Right = ClipPosition.X;
		r->Left = ClipPosition.X;
		r->Top = ClipPosition.Y;
		r->Bottom = ClipPosition.Y;
		char text[10];
		sprintf(text,"%d",Clips);
		ClipRender2DClass->Draw_Text(text,ClipColor);
		ClipRender2DClass->Render();
	}
	if (RenderShieldText)
	{
		ShieldRender2DClass->Reset();
		RectClass *r = (RectClass *)((char *)ShieldRender2DClass+0x5B8);
		float shield = Commands->Get_Shield_Strength(obj);
		float maxshield = Commands->Get_Max_Shield_Strength(obj);
		float shieldpercent = (shield / (maxshield / 100));
		r->Right = ShieldPosition.X + (shieldpercent * ShieldOffset.X);
		r->Left = ShieldPosition.X + (shieldpercent * ShieldOffset.X);
		r->Top = ShieldPosition.Y + (shieldpercent * ShieldOffset.Y);
		r->Bottom = ShieldPosition.Y + (shieldpercent * ShieldOffset.Y);
		char text[10];
		unsigned int s = WWMath::Float_To_Long(shield);
		sprintf(text,"%d",s);
		unsigned int color = 0;
		if (!ShieldColorCount)
		{
			color = ShieldColor;
		}
		else
		{
			for (unsigned int i = 0;i < ShieldColorCount;i++)
			{
				if (shieldpercent < ShieldColors[i]->Value)
				{
					color = ShieldColors[i]->ColorValue;
					break;
				}
			}
		}
		ShieldRender2DClass->Draw_Text(text,color);
		ShieldRender2DClass->Render();
	}
	if ((RenderWeaponImage) || ((RenderWeaponImage2) && (!Get_Vehicle(soldier))))
	{
		WeaponImageRender2DClass->Reset();
		GameObject *o = Get_Vehicle(soldier);
		if (o)
		{
			const char *texture;
			if (Get_Vehicle_Driver(o) == soldier)
			{
				texture = "hud_driverseat.tga";
			}
			else if (Get_Vehicle_Gunner_Pos(o) == soldier)
			{
				texture = "hud_gunseat.tga";
			}
			else
			{
				texture = "hud_passseat.tga";
			}
			RectClass r;
			r.Top = WeaponImagePosition.Y + 16;
			r.Left = WeaponImagePosition.X + 34;
			r.Bottom = WeaponImagePosition.Y + 64 + 16;
			r.Right = WeaponImagePosition.X + 64 + 34;
			WeaponImageRender2DClass->Set_Texture(texture);
			WeaponImageRender2DClass->Add_Quad(r,WeaponImageVehicleColor);
		}
		else
		{
			SoldierGameObj *sobj = (SoldierGameObj *)soldier;
			WeaponBagClass *w = sobj->Get_Weapon_Bag();
			if ((w->Get_Index()) && (w->Get_Index() < w->Get_Count()))
			{
				WeaponClass *wc = w->Peek_Weapon(w->Get_Index());
				const WeaponDefinitionClass *d = wc->Get_Definition();
				const char *h = d->IconTextureName;
				const char *texture = strrchr(h,'\\');
				if (!texture)
				{
					texture = h-1;
				}
				texture++;
				RectClass r = d->IconTextureUV;
				Vector2 v = d->IconOffset;
				WeaponImageRender2DClass->Set_Texture(texture);
				if (WeaponImageRender2DClass->Peek_Texture()->Initialized == false)
				{
					WeaponImageRender2DClass->Peek_Texture()->Init();
				}
				unsigned int sz = WeaponImageRender2DClass->Peek_Texture()->Width;
				float size = (float)(1.0 / sz);
				RectClass uv = RectClass(r.Left*size,r.Top*size,r.Right*size,r.Bottom*size);
				RectClass position = RectClass(WeaponImagePosition.X+v.X,WeaponImagePosition.Y+v.Y,WeaponImagePosition.X+v.X,WeaponImagePosition.Y+v.Y);
				position.Bottom += r.Bottom-r.Top;
				position.Right += r.Right-r.Left;
				WeaponImageRender2DClass->Add_Quad(position,uv,WeaponImageColor);
			}
		}
		WeaponImageRender2DClass->Render();
	}
	if (RenderRadar)
	{
		Matrix3D RadarTM;
		PhysicalGameObj *physobj = (PhysicalGameObj *)obj;
		physobj->Get_Transform().Get_Orthogonal_Inverse(RadarTM);
		VehicleGameObj *vehobj = physobj->As_VehicleGameObj();
		if (vehobj && COMBAT_CAMERA && !COMBAT_CAMERA->Is_2D_Targeting())
		{
			Matrix3D gun;
			gun.Obj_Look_At(vehobj->Get_Transform().Get_Translation(),vehobj->Get_Targeting_Pos(),0);
			gun.Get_Inverse(RadarTM);
		}
		RadarTM.rotateZ(-WWMATH_PI/2);
		BackgroundRender2DClass->Reset();
		BlipRender2DClass->Reset();
		BlipRender2DClass->Set_Texture(BlipTexture);
		if (BlipRender2DClass->Peek_Texture()->Initialized == false)
		{
			BlipRender2DClass->Peek_Texture()->Init();
		}
		unsigned int bsz = BlipRender2DClass->Peek_Texture()->Width;
		float blipsize = (float)(1.0 / bsz);
		Vector3 sv = Commands->Get_Position(obj);
		sv.Z = 0;
		float facing = Commands->Get_Facing(obj);
		facing += 180;
		facing += 90;
		if (facing > 360)
		{
			facing -= 360;
		}
		float facing2 = 360 - facing;
		facing = D3DXToRadian(facing);
		facing2 = D3DXToRadian(facing2);
		if (!ScrollingMap)
		{
			BackgroundRender2DClass->Set_Texture(BackgroundTexture);
			RectClass position = RectClass(RadarPosition.X,RadarPosition.Y,RadarPosition.X+RadarSize,RadarPosition.Y+RadarSize);
			if (BackgroundRender2DClass->Peek_Texture()->Initialized == false)
			{
				BackgroundRender2DClass->Peek_Texture()->Init();
			}
			unsigned int sz = BackgroundRender2DClass->Peek_Texture()->Width;
			float size = (float)(1.0 / sz);
			RectClass uv = RectClass(BackgroundUV.X*size,BackgroundUV.Y*size,(BackgroundUV.X+RadarSize)*size,(BackgroundUV.Y+RadarSize)*size);
			BackgroundRender2DClass->Add_Quad(position,uv,BackgroundColor);
			BackgroundRender2DClass->Render();
		}
		else
		{
			BackgroundRender2DClass->Set_Texture(MapTexture);
			RectClass position = RectClass(RadarPosition.X,RadarPosition.Y,RadarPosition.X+RadarSize,RadarPosition.Y+RadarSize);
			if (BackgroundRender2DClass->Peek_Texture()->Initialized == false)
			{
				BackgroundRender2DClass->Peek_Texture()->Init();
			}
			unsigned int sz = BackgroundRender2DClass->Peek_Texture()->Width;
			float size = (float)(1.0 / sz);
			Vector2 pos = Vector2(((sv.X+MapOffset.X)*MapScale)+(sz/2),((sv.Y+MapOffset.Y)*(-MapScale))+(sz/2));
			RectClass uv = RectClass((pos.X-(RadarSize/2))*size,(pos.Y-(RadarSize/2))*size,(pos.X+(RadarSize/2))*size,(pos.Y+(RadarSize/2))*size);
			BackgroundRender2DClass->Add_Quad(position,uv,BackgroundColor);
			Matrix4 m1,m2;
			D3DXVECTOR2 v2 = D3DXVECTOR2(pos.X*size,pos.Y*size);
			DWORD ttf = 0;
			DWORD tci = 0;
			DWORD wrap = 0;
			DX8Wrapper::Get_Transform(D3DTS_TEXTURE0,m1);
			StateManager::GetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,&ttf);
			StateManager::GetTextureStageState(0,D3DTSS_TEXCOORDINDEX,&tci);
			StateManager::GetRenderState(D3DRS_WRAP0,&wrap);
			StateManager::SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_COUNT2);
			StateManager::SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,D3DTSS_TCI_PASSTHRU);
			StateManager::SetRenderState(D3DRS_WRAP0,0);
			D3DXMatrixTransformation2D((D3DXMATRIX*)&m2,NULL,0,NULL,(D3DXVECTOR2 *)&v2,facing2,NULL);
			m2[2].X = m2[3].X;
			m2[2].Y = m2[3].Y;
			DX8Wrapper::Set_Transform(D3DTS_TEXTURE0,m2);
			BackgroundRender2DClass->Render();
			StateManager::SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,ttf);
			StateManager::SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,tci);
			DX8Wrapper::Set_Transform(D3DTS_TEXTURE0,m1);
			StateManager::SetRenderState(D3DRS_WRAP0,wrap);
		}
		float radarsize = RadarSize / 2;
		float radarscale = radarsize / RadarWorldSize;
		Vector3 objpos = Commands->Get_Position(obj);
		int objTeam = Commands->Get_Player_Type(obj);
		if ((RadarManager::RadarIntensity) && (BaseControllerClass::Find_Base(objTeam)) && (BaseControllerClass::Find_Base(objTeam)->Is_Radar_Enabled()))
		{
			for (SLNode<GameObject>* iter = GameObjManager::GameObjList.Head(); iter; iter = iter->Next())
			{
				PhysicalGameObj *o = iter->Data()->As_PhysicalGameObj();
				if (!o) continue;
				if ((o == obj) && (DrawStar == false)) continue;
				if (!o->Peek_Physical_Object()) continue;
				if (o->As_SoldierGameObj() && o->Get_Defense_Object()->Get_Health() <= 0.0f) continue;
				if (!The_Game()->RadarMode) continue;
				if (The_Game()->RadarMode != 2)
				{	
					if (objTeam != o->Get_Player_Type()) continue;
					if (objTeam > 1) continue;
				}
				if (o->As_SmartGameObj() && o->As_SmartGameObj()->Is_Stealthed())
				{
					if (objTeam != o->Get_Player_Type()) continue;
				}

				unsigned int type = o->Get_Radar_Blip_Shape_Type();
				unsigned int color = o->Get_Radar_Blip_Color_Type();
				if (type)
				{
					Vector3 pos,relpos;
					o->Get_Position(&pos);
					relpos = RadarTM.applyTo(pos);
					bool sb = false;
					if (abs(objpos.Z - pos.Z) > 3.0f)
					{
						sb = true;
					}
					relpos.Z = 0;
					relpos *= radarscale;
					if (relpos.Length() >= radarsize)
					{
						if (type == 4)
						{
							relpos.Normalize();
							relpos *= radarsize;
						}
					}
					if (relpos.Length() <= radarsize)
					{
						relpos.X = 1.f - relpos.X;
						relpos.Y = 2.f + relpos.Y;
						relpos.X += RadarPosition.X + radarsize;
						relpos.Y += RadarPosition.Y + radarsize;
						float br = 4;
						if (sb)
						{
							br *= (2.0f/3.0f);
						}
						RectClass uv = RectClass(BlipUV[type].X*blipsize,BlipUV[type].Y*blipsize,(BlipUV[type].X+8)*blipsize,(BlipUV[type].Y+8)*blipsize);
						RectClass screen = RectClass(relpos.X-br,relpos.Y-br,relpos.X+br,relpos.Y+br);
						screen.Top = (float)(int)screen.Top;
						screen.Left = (float)(int)screen.Left;
						screen.Bottom = (float)(int)screen.Bottom;
						screen.Right = (float)(int)screen.Right;
						BlipRender2DClass->Add_Quad(screen,uv,BlipColor[color]);
						if (o == RadarManager::BracketObj)
						{
							RectClass uv2 = RectClass(BlipUV[5].X*blipsize,BlipUV[5].Y*blipsize,(BlipUV[5].X+(br*2))*blipsize,(BlipUV[5].Y+(br*2))*blipsize);
							BlipRender2DClass->Add_Quad(screen,uv2,BlipColor[5]);
						}
					}
				}
			}
			for (int i = 0; i < ObjectiveManager::ObjectiveList.Count(); i++)
			{
				Objective* objective = ObjectiveManager::ObjectiveList[i];
				if (objective->showOnRadar &&
					objective->status == Objective::Pending)
				{
					unsigned int type = 4;
					unsigned int color = objective->Radar_Blip_Color_Type();
					if (type)
					{
						Vector3 pos,relpos;
						pos = objective->location;
						relpos = RadarTM.applyTo(pos);
						bool sb = false;
						if (abs(objpos.Z - pos.Z) > 3.0f)
						{
							sb = true;
						}
						relpos.Z = 0;
						relpos *= radarscale;
						if (relpos.Length() < radarsize)
						{
							relpos.X = 1.f - relpos.X;
							relpos.Y = 2.f + relpos.Y;
							relpos.X += RadarPosition.X + radarsize;
							relpos.Y += RadarPosition.Y + radarsize;
							float br = 4;
							if (sb)
							{
								br *= (2.0f/3.0f);
							}
							RectClass uv = RectClass(BlipUV[type].X*blipsize,BlipUV[type].Y*blipsize,(BlipUV[type].X+(br*2))*blipsize,(BlipUV[type].Y+(br*2))*blipsize);
							RectClass screen = RectClass(relpos.X-br,relpos.Y-br,relpos.X+br,relpos.Y+br);
							screen.Top = (float)(int)screen.Top;
							screen.Left = (float)(int)screen.Left;
							screen.Bottom = (float)(int)screen.Bottom;
							screen.Right = (float)(int)screen.Right;
							BlipRender2DClass->Add_Quad(screen,uv,BlipColor[color]);
						}
					}
				}
			}
			for (int i = 0; i < RadarManager::Markers.Count(); i++)
			{
				RadarMarkerClass* marker = &RadarManager::Markers[i];
				unsigned int type = marker->radarBlipShape;
				unsigned int color = marker->radarBlipColorType;
				if (type)
				{
					Vector3 pos,relpos;
					pos = marker->location;
					relpos = RadarTM.applyTo(pos);
					bool sb = false;
					if (abs(objpos.Z - pos.Z) > 3.0f)
					{
						sb = true;
					}
					relpos.Z = 0;
					relpos *= radarscale;
					if (relpos.Length() < radarsize)
					{
						relpos.X = 1.f - relpos.X;
						relpos.Y = 2.f + relpos.Y;
						relpos.X += RadarPosition.X + radarsize;
						relpos.Y += RadarPosition.Y + radarsize;
						float br = 4;
						if (sb)
						{
							br *= (2.0f/3.0f);
						}
						RectClass uv = RectClass(BlipUV[type].X*blipsize,BlipUV[type].Y*blipsize,(BlipUV[type].X+(br*2))*blipsize,(BlipUV[type].Y+(br*2))*blipsize);
						RectClass screen = RectClass(relpos.X-br,relpos.Y-br,relpos.X+br,relpos.Y+br);
						screen.Top = (float)(int)screen.Top;
						screen.Left = (float)(int)screen.Left;
						screen.Bottom = (float)(int)screen.Bottom;
						screen.Right = (float)(int)screen.Right;
						BlipRender2DClass->Add_Quad(screen,uv,BlipColor[color]);
					}
				}
			}
			BlipRender2DClass->Render();
		}
	}
}

void __declspec(dllexport) ReadRuntimeINI(const char *ini)
{
	INIClass *hudini = Get_INI(ini);
	if (!hudini)
	{	
		return;
	}
	if (HealthEnabled)
	{
		unsigned int HealthCol = hudini->Get_Int("General","HealthColor",0);
		HealthColorCount = hudini->Get_Int("General","HealthColorCount",0);
		for (unsigned int i = 0;i < HealthColorCount;i++)
		{
			char section[20];
			sprintf(section,"HealthColor%d",i);
			unsigned int index = hudini->Get_Int(section,"Color",0);
			HealthColors[i]->ColorValue = Colors[index];
		}
		HealthColor = Colors[HealthCol];
	}

	if (ShieldEnabled)
	{
		unsigned int ShieldCol = hudini->Get_Int("General","ShieldColor",0);
		ShieldColorCount = hudini->Get_Int("General","ShieldColorCount",0);
		for (unsigned int i = 0;i < ShieldColorCount;i++)
		{
			char section[20];
			sprintf(section,"ShieldColor%d",i);
			unsigned int index = hudini->Get_Int(section,"Color",0);
			ShieldColors[i]->ColorValue = Colors[index];
		}
		ShieldColor = Colors[ShieldCol];
	}

	if (BulletEnabled)
	{
		unsigned int BulletCol = hudini->Get_Int("General","BulletColor",0);
		BulletColor = Colors[BulletCol];
	}

	if (ClipEnabled)
	{
		unsigned int ClipCol = hudini->Get_Int("General","ClipColor",0);
		ClipColor = Colors[ClipCol];
	}

	if (WeaponEnabled)
	{
		unsigned int WeaponCol = hudini->Get_Int("General","WeaponColor",0);
		WeaponColor = Colors[WeaponCol];
	}

	if (CompassEnabled)
	{
		unsigned int CompassCol = hudini->Get_Int("General","CompassColor",0);
		CompassColor = Colors[CompassCol];
	}

	if (CreditsEnabled)
	{
		unsigned int CreditsCol = hudini->Get_Int("General","CreditsColor",0);
		CreditsColor = Colors[CreditsCol];
	}

	if (TimeEnabled)
	{
		unsigned int TimeCol = hudini->Get_Int("General","TimeColor",0);
		TimeColor = Colors[TimeCol];
	}

	if (WeaponImageEnabled)
	{
		unsigned int WeaponImageCol = hudini->Get_Int("General","WeaponImageColor",0);
		WeaponImageColor = Colors[WeaponImageCol];
		unsigned int WeaponImageVehicleCol = hudini->Get_Int("General","WeaponImageVehicleColor",0);
		WeaponImageVehicleColor = Colors[WeaponImageVehicleCol];
	}

	for (unsigned int i = 0;i < TextureCount;i++)
	{
		char section[20];
		sprintf(section,"Texture%d",i);
		hudini->Get_String(section,"TextureName","",HudTextures[i]->TextureName,260-1);
		for (unsigned int j = 0;j < HudTextures[i]->QuadCount;j++)
		{
			char entry[50];
			sprintf(entry,"Quad%dColor",j);
			HudTextures[i]->Quads[j].Color = Colors[hudini->Get_Int(section,entry,0)];
		}
	}

	if (HealthBarEnabled)
	{
		unsigned int HealthBarCol = hudini->Get_Int("General","HealthBarColor",0);
		unsigned int HealthBarEmptyCol = hudini->Get_Int("General","HealthBarEmptyColor",0);
		for (unsigned int i = 0;i < HealthBarColorCount;i++)
		{
			char section[20];
			sprintf(section,"HealthBarColor%d",i);
			unsigned int index = hudini->Get_Int(section,"Color",0);
			HealthBarColors[i]->ColorValue = Colors[index];
		}
		HealthBarColor = Colors[HealthBarCol];
		HealthBarEmptyColor = Colors[HealthBarEmptyCol];
		hudini->Get_String("General","HealthBarTexture","",HealthBarTexture,260-1);
	}

	if (ShieldBarEnabled)
	{
		unsigned int ShieldBarCol = hudini->Get_Int("General","ShieldBarColor",0);
		unsigned int ShieldBarEmptyCol = hudini->Get_Int("General","ShieldBarEmptyColor",0);
		for (unsigned int i = 0;i < ShieldBarColorCount;i++)
		{
			char section[20];
			sprintf(section,"ShieldBarColor%d",i);
			unsigned int index = hudini->Get_Int(section,"Color",0);
			ShieldBarColors[i]->ColorValue = Colors[index];
		}
		ShieldBarColor = Colors[ShieldBarCol];
		ShieldBarEmptyColor = Colors[ShieldBarEmptyCol];
		hudini->Get_String("General","ShieldBarTexture","",ShieldBarTexture,260-1);
	}

	if (HealthIconEnabled)
	{
		unsigned int HealthIconCol = hudini->Get_Int("General","HealthIconColor",0);
		for (unsigned int i = 0;i < HealthIconColorCount;i++)
		{
			char section[20];
			sprintf(section,"HealthIconColor%d",i);
			unsigned int index = hudini->Get_Int(section,"Color",0);
			HealthIconColors[i]->ColorValue = Colors[index];
		}
		HealthIconColor = Colors[HealthIconCol];
		hudini->Get_String("General","HealthIconTexture","",HealthIconTexture,260-1);
	}

	if (ShieldIconEnabled)
	{
		unsigned int ShieldIconCol = hudini->Get_Int("General","ShieldIconColor",0);
		for (unsigned int i = 0;i < ShieldIconColorCount;i++)
		{
			char section[20];
			sprintf(section,"ShieldIconColor%d",i);
			unsigned int index = hudini->Get_Int(section,"Color",0);
			ShieldIconColors[i]->ColorValue = Colors[index];
		}
		ShieldIconColor = Colors[ShieldIconCol];
		hudini->Get_String("General","ShieldIconTexture","",ShieldIconTexture,260-1);
	}

	if (RadarEnabled)
	{
		unsigned long BackgroundCol = hudini->Get_Int("General","BackgroundColor",0);
		BackgroundColor = Colors[BackgroundCol];
		hudini->Get_String("General","BackgroundTexture","",BackgroundTexture,260-1);
		hudini->Get_String("General","BlipTexture","",BlipTexture,260-1);
		for (unsigned int i = 0;i < 8;i++)
		{
			char entry[50];
			sprintf(entry,"RadarBlipColor%d",i);
			unsigned int BlipCol = hudini->Get_Int("General",entry,0);
			BlipColor[i] = Colors[BlipCol];
		}
	}
	Release_INI(hudini);
}

void __declspec(dllexport) Cleanup_HUD2()
{
	for (unsigned int i = 0;i < HealthColorCount;i++)
	{
		SAFE_DELETE(HealthColors[i]);
	}
	for (unsigned int i = 0;i < ShieldColorCount;i++)
	{
		SAFE_DELETE(ShieldColors[i]);
	}
	for (unsigned int i = 0;i < HealthBarColorCount;i++)
	{
		SAFE_DELETE(HealthBarColors[i]);
	}
	for (unsigned int i = 0;i < ShieldBarColorCount;i++)
	{
		SAFE_DELETE(ShieldBarColors[i]);
	}
	for (unsigned int i = 0;i < HealthIconColorCount;i++)
	{
		SAFE_DELETE(HealthIconColors[i]);
	}
	for (unsigned int i = 0;i < ShieldIconColorCount;i++)
	{
		SAFE_DELETE(ShieldIconColors[i]);
	}
	for (unsigned int i = 0;i < TextureCount;i++)
	{
		SAFE_DELETE(HudTextures[i]->TextureRender2DClass);
		SAFE_DELETE_ARRAY(HudTextures[i]->TextureName);
		SAFE_DELETE_ARRAY(HudTextures[i]->Quads);
		SAFE_DELETE(HudTextures[i]);
	}
	SAFE_DELETE(HealthRender2DClass);
	SAFE_DELETE(ShieldRender2DClass);
	SAFE_DELETE(BulletRender2DClass);
	SAFE_DELETE(ClipRender2DClass);
	SAFE_DELETE(WeaponNameRenderer2);
	SAFE_DELETE(WeaponImageRender2DClass);
	SAFE_DELETE_ARRAY(HealthBarTexture);
	SAFE_DELETE(HealthBarRender2DClass);
	SAFE_DELETE_ARRAY(ShieldBarTexture);
	SAFE_DELETE(ShieldBarRender2DClass);
	SAFE_DELETE(CompassRenderer);
	SAFE_DELETE(CreditsRenderer);
	SAFE_DELETE(TimeRenderer);
	SAFE_DELETE_ARRAY(BackgroundTexture);
	SAFE_DELETE_ARRAY(BlipTexture);
	SAFE_DELETE(BackgroundRender2DClass);
	SAFE_DELETE(BlipRender2DClass);
	SAFE_DELETE(HealthIconRender2DClass);
	SAFE_DELETE_ARRAY(HealthIconTexture);
	SAFE_DELETE(ShieldIconRender2DClass);
	SAFE_DELETE_ARRAY(ShieldIconTexture);
}

void __declspec(dllexport) Update_Radar_Map(float scale,float offsetx,float offsety,const char *texture)
{
	MapScale = scale;
	MapOffset.X = offsetx;
	MapOffset.Y = offsety;
	strcpy(MapTexture,texture);
}

void __declspec(dllexport) Send_HUD_Number(int number)
{
}