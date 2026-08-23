/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 OpenW3D contributors.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "always.h"
#include "ttsettings.h"
#include "ini.h"
#include "ffactory.h"
#include "wwfile.h"
#include "wwdebug.h"

#include <stdio.h>
#include <string.h>
#include <algorithm>


////////////////////////////////////////////////////////////////
//	Static member initialization -- these are TT's own defaults.
//	tt.ini gameplay, engine2.cpp:606-640.
////////////////////////////////////////////////////////////////
bool			TTSettingsClass::ContinueReloadOnVehicleExit	= true;
bool			TTSettingsClass::DisableCostMultiplier			= false;
float			TTSettingsClass::BuildTimeDelay					= 2.0F;
bool			TTSettingsClass::VehicleOwnershipDisable		= false;
bool			TTSettingsClass::VehicleBuildingDisable		= false;
bool			TTSettingsClass::DisableVehicleFlipKill		= false;
bool			TTSettingsClass::Unsquishable						= false;
int			TTSettingsClass::UnsquishableArmor				= 0;
int			TTSettingsClass::UnsquishableArmor2				= 0;
int			TTSettingsClass::UnsquishableArmor3				= 0;
int			TTSettingsClass::UnsquishableArmor4				= 0;
bool			TTSettingsClass::NeutralVehiclePointsFix		= true;
float			TTSettingsClass::DrawDistance						= 300.0F;
int			TTSettingsClass::ScriptsLastTeamTime			= -1;
bool			TTSettingsClass::NukeWeatherDisable				= false;
bool			TTSettingsClass::IonWeatherDisable				= false;
StringClass	TTSettingsClass::MapPrefix							= "C&C_";

float			TTSettingsClass::NodHouseColor[3]				= { 1.0F, 0.0F, 0.0F };
float			TTSettingsClass::GDIHouseColor[3]				= { 1.0F, 204.0F / 255.0F, 0.0F };
float			TTSettingsClass::PrivateMessageColor[3]		= { 0.0F, 0.0F, 1.0F };
float			TTSettingsClass::PublicMessageColor[3]			= { 1.0F, 1.0F, 1.0F };

////////////////////////////////////////////////////////////////
//	tt.ini HUD and purchase terminal, hud.cpp:ReadHUDIniBits
////////////////////////////////////////////////////////////////
bool			TTSettingsClass::UseExtraPTPages					= false;
bool			TTSettingsClass::NewUnpurchasableLogic			= false;
float			TTSettingsClass::RefillLimit						= 0.0F;
bool			TTSettingsClass::AlternateSelectEnabled		= false;
bool			TTSettingsClass::SidebarSoundsEnabled			= false;
bool			TTSettingsClass::DisableKillMessages			= false;

StringClass	TTSettingsClass::SidebarRefillSound;
StringClass	TTSettingsClass::SidebarInfantrySound;
StringClass	TTSettingsClass::SidebarVehicleSound;

StringClass	TTSettingsClass::GDIUpArrowTexture;
StringClass	TTSettingsClass::GDIDownArrowTexture;
StringClass	TTSettingsClass::GDIBackgroundTexture1;
StringClass	TTSettingsClass::GDIBackgroundTexture2;
StringClass	TTSettingsClass::NODUpArrowTexture;
StringClass	TTSettingsClass::NODDownArrowTexture;
StringClass	TTSettingsClass::NODBackgroundTexture1;
StringClass	TTSettingsClass::NODBackgroundTexture2;

StringClass	TTSettingsClass::GDIAlternateSelectTexture[4];
StringClass	TTSettingsClass::NODAlternateSelectTexture[4];

StringClass	TTSettingsClass::ModRegistryKey;
StringClass	TTSettingsClass::VersionRegistryKey					= "Software\\Westwood\\Renegade";
StringClass	TTSettingsClass::WOLUrlRegistryKey					= "WOLSettings\\URL";

////////////////////////////////////////////////////////////////
//	hud.ini UI, hud.cpp:ReadHUDIniBits2
////////////////////////////////////////////////////////////////
bool			TTSettingsClass::ScrollingRadarMap				= false;
bool			TTSettingsClass::HidePlayerList					= false;
bool			TTSettingsClass::HideBottomText					= false;
int			TTSettingsClass::LodBudgetDialogValue1			= 5000;
int			TTSettingsClass::LodBudgetDialogValue2			= 10000;

bool			TTSettingsClass::ListColumnColorEnabled		= false;
float			TTSettingsClass::ListColumnColor[3]				= { 1.0F, 1.0F, 1.0F };

unsigned int TTSettingsClass::TitleColor						= TTSettingsClass::Make_Color (255, 255, 255, 255);
unsigned int TTSettingsClass::TitleHilightColor				= TTSettingsClass::Make_Color (255, 255, 255, 0);
unsigned int TTSettingsClass::TitleShadowColor				= TTSettingsClass::Make_Color (255, 0, 0, 0);
unsigned int TTSettingsClass::TextColor						= TTSettingsClass::Make_Color (255, 255, 213, 40);
unsigned int TTSettingsClass::TextShadowColor				= TTSettingsClass::Make_Color (200, 0, 0, 0);
unsigned int TTSettingsClass::LineColor						= TTSettingsClass::Make_Color (255, 255, 174, 40);
unsigned int TTSettingsClass::BkColor							= TTSettingsClass::Make_Color (40, 255, 174, 40);
unsigned int TTSettingsClass::DisabledTextColor				= TTSettingsClass::Make_Color (140, 255, 213, 40);
unsigned int TTSettingsClass::DisabledTextShadowColor		= TTSettingsClass::Make_Color (96, 0, 0, 0);
unsigned int TTSettingsClass::DisabledLineColor				= TTSettingsClass::Make_Color (128, 230, 160, 35);
unsigned int TTSettingsClass::DisabledBkColor				= TTSettingsClass::Make_Color (30, 255, 174, 40);
unsigned int TTSettingsClass::HilightColor					= TTSettingsClass::Make_Color (255, 70, 70, 70);
unsigned int TTSettingsClass::TabTextColor					= TTSettingsClass::Make_Color (255, 255, 255, 255);
unsigned int TTSettingsClass::TabGlowColor					= TTSettingsClass::Make_Color (255, 16, 10, 0);

unsigned int TTSettingsClass::DialogTextTitleColor			= TTSettingsClass::Make_Color (255, 255, 255, 36);
unsigned int TTSettingsClass::DialogTextTitleGlowColor	= TTSettingsClass::Make_Color (255, 14, 0, 0);
unsigned int TTSettingsClass::MenuHilightColor				= TTSettingsClass::Make_Color (255, 0, 0, 0);
unsigned int TTSettingsClass::MerchandiseTextColor			= TTSettingsClass::Make_Color (255, 255, 255, 255);
unsigned int TTSettingsClass::MenuStaticGlowColor			= TTSettingsClass::Make_Color (255, 9, 0, 0);
unsigned int TTSettingsClass::MenuActiveGlowColor			= TTSettingsClass::Make_Color (255, 16, 0, 0);
unsigned int TTSettingsClass::MenuPushedBaseGlowColor		= TTSettingsClass::Make_Color (255, 48, 0, 0);
unsigned int TTSettingsClass::MenuPushedHighlightGlowColor = TTSettingsClass::Make_Color (255, 48, 29, 0);

bool			TTSettingsClass::IsLoaded							= false;


static const char *SECTION = "General";


////////////////////////////////////////////////////////////////
//
//	Get_Text
//
////////////////////////////////////////////////////////////////
StringClass
TTSettingsClass::Get_Text (const INIClass &ini, const char *entry, const char *defvalue)
{
	StringClass value;
	ini.Get_String (value, SECTION, entry, defvalue);
	return value;
}


////////////////////////////////////////////////////////////////
//
//	Get_Channels
//
// TT stores its team and chat colours as three 0..255 integers named
// <prefix>Red / <prefix>Green / <prefix>Blue and divides by 255 on the way in.
//
////////////////////////////////////////////////////////////////
void
TTSettingsClass::Get_Channels (const INIClass &ini, const char *prefix,
										int def_r, int def_g, int def_b, float *out)
{
	static const char *SUFFIX[3] = { "Red", "Green", "Blue" };
	const int defaults[3] = { def_r, def_g, def_b };

	for (int index = 0; index < 3; index ++) {
		StringClass entry;
		entry.Format ("%s%s", prefix, SUFFIX[index]);
		out[index] = (float)ini.Get_Int (SECTION, entry, defaults[index]) / 255.0F;
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Get_Color
//
// TT accepts a styling colour either as <entry>Hex, a hex string, or as the
// four integers <entry>Alpha / Red / Green / Blue.  The hex form wins.
//
////////////////////////////////////////////////////////////////
unsigned int
TTSettingsClass::Get_Color (const INIClass &ini, const char *entry, unsigned int defvalue)
{
	StringClass name;
	name.Format ("%sHex", entry);

	char hex[32] = { 0 };
	ini.Get_String (SECTION, name, "", hex, sizeof (hex));
	if (hex[0] != 0) {
		unsigned int color = 0;
		if (::sscanf (hex, "%x", &color) == 1) {
			return color;
		}
	}

	static const char *SUFFIX[4] = { "Alpha", "Red", "Green", "Blue" };
	unsigned int channels[4];

	for (int index = 0; index < 4; index ++) {
		name.Format ("%s%s", entry, SUFFIX[index]);
		int shift = 24 - (index * 8);
		channels[index] = (unsigned int)ini.Get_Int (SECTION, name, (defvalue >> shift) & 0xFF);
	}

	return Make_Color (channels[0], channels[1], channels[2], channels[3]);
}


////////////////////////////////////////////////////////////////
//
//	Load_File
//
////////////////////////////////////////////////////////////////
bool
TTSettingsClass::Load_File (const char *filename, INIClass &ini)
{
	if (_TheFileFactory == nullptr) {
		return false;
	}

	FileClass *file = _TheFileFactory->Get_File (filename);
	if (file == nullptr) {
		return false;
	}

	bool retval = false;
	if (file->Is_Available ()) {
		ini.Load (*file);
		retval = true;
	}

	_TheFileFactory->Return_File (file);
	return retval;
}


////////////////////////////////////////////////////////////////
//
//	Load_Gameplay
//
////////////////////////////////////////////////////////////////
void
TTSettingsClass::Load_Gameplay (const INIClass &ini)
{
	ContinueReloadOnVehicleExit	= ini.Get_Bool (SECTION, "ContinueReloadOnVehicleExit", true);
	DisableCostMultiplier			= ini.Get_Bool (SECTION, "DisableCostMultiplier", false);
	BuildTimeDelay						= ini.Get_Float (SECTION, "BuildTimeDelay", 2.0F);
	VehicleOwnershipDisable			= ini.Get_Bool (SECTION, "VehicleOwnershipDisable", false);
	VehicleBuildingDisable			= ini.Get_Bool (SECTION, "VehicleBuildingDisable", false);
	DisableVehicleFlipKill			= ini.Get_Bool (SECTION, "DisableVehicleFlipKill", false);
	Unsquishable						= ini.Get_Bool (SECTION, "Unsquishable", false);
	NeutralVehiclePointsFix			= ini.Get_Bool (SECTION, "NeutralVechiclePointsFix", true);
	DrawDistance						= ini.Get_Float (SECTION, "DrawDistance", 300.0F);
	ScriptsLastTeamTime				= ini.Get_Int (SECTION, "ScriptsLastTeamTime", -1);
	NukeWeatherDisable				= ini.Get_Bool (SECTION, "NukeWeatherDisable", false);
	IonWeatherDisable					= ini.Get_Bool (SECTION, "IonWeatherDisable", false);
	MapPrefix							= Get_Text (ini, "MapPrefix", "C&C_");

	//
	//	TT only reads the armour exemptions when squish protection is on
	//
	if (Unsquishable) {
		UnsquishableArmor		= ini.Get_Int (SECTION, "UnsquishableArmor", 0);
		UnsquishableArmor2	= ini.Get_Int (SECTION, "UnsquishableArmor2", 0);
		UnsquishableArmor3	= ini.Get_Int (SECTION, "UnsquishableArmor3", 0);
		UnsquishableArmor4	= ini.Get_Int (SECTION, "UnsquishableArmor4", 0);
	}

	Get_Channels (ini, "NodHouse", 255, 0, 0, NodHouseColor);
	Get_Channels (ini, "GDIHouse", 255, 204, 0, GDIHouseColor);
	Get_Channels (ini, "PrivateMessage", 0, 0, 255, PrivateMessageColor);
	Get_Channels (ini, "PublicMessage", 255, 255, 255, PublicMessageColor);

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Load_Terminal
//
////////////////////////////////////////////////////////////////
void
TTSettingsClass::Load_Terminal (const INIClass &ini)
{
	UseExtraPTPages			= ini.Get_Bool (SECTION, "UseExtraPTPages", false);

	//
	//	TT's own spelling of the key, typo and all -- an ini written for TT has
	//	to keep working
	//
	NewUnpurchasableLogic	= ini.Get_Bool (SECTION, "NewUnpurchaseableLogic", false);

	RefillLimit					= ini.Get_Float (SECTION, "RefillLimit", 0.0F);
	AlternateSelectEnabled	= ini.Get_Bool (SECTION, "AlternateSelectEnabled", false);
	SidebarSoundsEnabled		= ini.Get_Bool (SECTION, "SidebarSoundsEnabled", false);
	DisableKillMessages		= ini.Get_Bool (SECTION, "DisableKillMessages", false);

	GDIUpArrowTexture			= Get_Text (ini, "GDIUpArrowTexture");
	GDIDownArrowTexture		= Get_Text (ini, "GDIDownArrowTexture");
	GDIBackgroundTexture1	= Get_Text (ini, "GDIBackgroundTexture1");
	GDIBackgroundTexture2	= Get_Text (ini, "GDIBackgroundTexture2");
	NODUpArrowTexture			= Get_Text (ini, "NODUpArrowTexture");
	NODDownArrowTexture		= Get_Text (ini, "NODDownArrowTexture");
	NODBackgroundTexture1	= Get_Text (ini, "NODBackgroundTexture1");
	NODBackgroundTexture2	= Get_Text (ini, "NODBackgroundTexture2");

	for (int index = 0; index < 4; index ++) {
		StringClass entry;

		entry.Format ("GDIAlternateSelectTexture%d", index + 1);
		GDIAlternateSelectTexture[index] = Get_Text (ini, entry);

		entry.Format ("NODAlternateSelectTexture%d", index + 1);
		NODAlternateSelectTexture[index] = Get_Text (ini, entry);
	}

	if (SidebarSoundsEnabled) {
		SidebarRefillSound	= Get_Text (ini, "SidebarRefillSound");
		SidebarInfantrySound	= Get_Text (ini, "SidebarInfantrySound");
		SidebarVehicleSound	= Get_Text (ini, "SidebarVehicleSound");
	}

	ModRegistryKey			= Get_Text (ini, "ModReg");
	VersionRegistryKey	= Get_Text (ini, "VersionReg", "Software\\Westwood\\Renegade");
	WOLUrlRegistryKey		= Get_Text (ini, "WOLUrlReg", "WOLSettings\\URL");

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Load_UI
//
////////////////////////////////////////////////////////////////
void
TTSettingsClass::Load_UI (const INIClass &ini)
{
	ScrollingRadarMap		= ini.Get_Bool (SECTION, "ScrollingRadarMap", false);
	HidePlayerList			= ini.Get_Bool (SECTION, "HidePlayerList", false);
	HideBottomText			= ini.Get_Bool (SECTION, "HideBottomText", false);

	ListColumnColorEnabled = ini.Get_Bool (SECTION, "ListColumnColorEnabled", false);
	if (ListColumnColorEnabled) {
		Get_Channels (ini, "ListColumnColor", 255, 255, 255, ListColumnColor);
	}

	//
	//	TT clamps these with max(value, 65535), which pins both to 65535 and
	//	leaves the two options with no effect at all (hud.cpp:257-258).  Keep
	//	the options, drop the typo: an upper bound is what was meant.
	//
	LodBudgetDialogValue1 = std::min (ini.Get_Int (SECTION, "LodBudgetDialogValue1", 5000), 65535);
	LodBudgetDialogValue2 = std::min (ini.Get_Int (SECTION, "LodBudgetDialogValue2", 10000), 65535);

	TitleColor						= Get_Color (ini, "TitleColor", Make_Color (255, 255, 255, 255));
	TitleHilightColor				= Get_Color (ini, "TitleHilightColor", Make_Color (255, 255, 255, 0));
	TitleShadowColor				= Get_Color (ini, "TitleShadowColor", Make_Color (255, 0, 0, 0));
	TextColor						= Get_Color (ini, "TextColor", Make_Color (255, 255, 213, 40));
	TextShadowColor				= Get_Color (ini, "TextShadowColor", Make_Color (200, 0, 0, 0));
	LineColor						= Get_Color (ini, "LineColor", Make_Color (255, 255, 174, 40));
	BkColor							= Get_Color (ini, "BkColor", Make_Color (40, 255, 174, 40));
	DisabledTextColor				= Get_Color (ini, "DisabledTextColor", Make_Color (140, 255, 213, 40));
	DisabledTextShadowColor		= Get_Color (ini, "DisabledTextShadowColor", Make_Color (96, 0, 0, 0));
	DisabledLineColor				= Get_Color (ini, "DisabledLineColor", Make_Color (128, 230, 160, 35));
	DisabledBkColor				= Get_Color (ini, "DisabledBkColor", Make_Color (30, 255, 174, 40));

	//
	//	TT reads the highlight colour from "HilightColorBkColor" -- keep the
	//	key, an existing hud.ini would stop working otherwise
	//
	HilightColor					= Get_Color (ini, "HilightColorBkColor", Make_Color (255, 70, 70, 70));
	TabTextColor					= Get_Color (ini, "TabTextColor", Make_Color (255, 255, 255, 255));
	TabGlowColor					= Get_Color (ini, "TabGlowColor", Make_Color (255, 16, 10, 0));

	DialogTextTitleColor			= Get_Color (ini, "DialogTextTitleColor", Make_Color (255, 255, 255, 36));
	DialogTextTitleGlowColor	= Get_Color (ini, "DialogTextTitleGlowColor", Make_Color (255, 14, 0, 0));
	MenuHilightColor				= Get_Color (ini, "MenuHiliteColor", Make_Color (255, 0, 0, 0));
	MerchandiseTextColor			= Get_Color (ini, "MerchandiseTextColor", Make_Color (255, 255, 255, 255));

	MenuStaticGlowColor			= Get_Color (ini, "MenuStaticGlowColor", Make_Color (255, 9, 0, 0));
	MenuActiveGlowColor			= Get_Color (ini, "MenuActiveGlowColor", Make_Color (255, 16, 0, 0));
	MenuPushedBaseGlowColor		= Get_Color (ini, "MenuPushedBaseGlowColor", Make_Color (255, 48, 0, 0));
	MenuPushedHighlightGlowColor = Get_Color (ini, "MenuPushedHighlightGlowColor", Make_Color (255, 48, 29, 0));

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Load
//
////////////////////////////////////////////////////////////////
bool
TTSettingsClass::Load_UI_File (const char *filename)
{
	if ((filename == nullptr) || (filename [0] == 0)) {
		return false;
	}

	INIClass ini;
	if (!Load_File (filename, ini)) {
		return false;
	}

	Load_UI (ini);
	return true;
}


void
TTSettingsClass::Load (void)
{
	//
	//	tt.ini carries the gameplay options and, despite the naming inside TT,
	//	the HUD and purchase-terminal options as well: ReadHUDIniBits takes a
	//	parameter called hudini but engine2.cpp:646 hands it ttini.
	//
	{
		INIClass ini;
		if (Load_File ("tt.ini", ini)) {
			Load_Gameplay (ini);
			Load_Terminal (ini);
		}
	}

	//
	//	hud.ini carries the UI options
	//
	{
		INIClass ini;
		if (Load_File ("hud.ini", ini)) {
			Load_UI (ini);
		}
	}

	IsLoaded = true;

	WWDEBUG_SAY (("TTSettingsClass::Load - flip kill %s, unpurchasable logic %s, cost multiplier %s\n",
		DisableVehicleFlipKill ? "disabled" : "enabled",
		NewUnpurchasableLogic ? "new" : "stock",
		DisableCostMultiplier ? "disabled" : "enabled"));

	return ;
}
