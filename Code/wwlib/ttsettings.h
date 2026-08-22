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

/***********************************************************************************************
 *
 *	TTSettingsClass -- the TT 4.8.4 server configuration, natively.
 *
 *	TT ships most of its behaviour as configuration rather than as unconditional code: `tt.ini`
 *	and `hud.ini` are read once at startup and their values decide what the runtime does.  This
 *	is that surface, with TT's own option names and TT's own defaults, so a tree with no ini
 *	files present behaves exactly as TT does with no ini files present.
 *
 *	This lives in wwlib because its consumers span every layer above: wwphys reads the vehicle
 *	flip kill, Combat reads the gameplay options, wwui reads the styling colours and Commando
 *	reads the purchase terminal and HUD options.
 *
 *	See docs/tt484/TTSettings.md for the inventory, the provenance of each default, and the
 *	relationship to roadmap directive 0.4.
 *
 **********************************************************************************************/

#ifndef __TTSETTINGS_H
#define __TTSETTINGS_H

#include "always.h"
#include "wwstring.h"

class INIClass;


class TTSettingsClass
{
public:

	////////////////////////////////////////////////////////////////
	//	Public methods
	////////////////////////////////////////////////////////////////

	//
	//	Read tt.ini and hud.ini through the current file factory.  Safe to call
	//	when neither exists: every option keeps its TT default.  Calling it
	//	again re-reads both files.
	//
	static void			Load (void);

	static bool			Is_Loaded (void)	{ return IsLoaded; }

	////////////////////////////////////////////////////////////////
	//	tt.ini [General] -- gameplay
	////////////////////////////////////////////////////////////////
	static bool			ContinueReloadOnVehicleExit;
	static bool			DisableCostMultiplier;
	static float		BuildTimeDelay;
	static bool			VehicleOwnershipDisable;
	static bool			VehicleBuildingDisable;
	static bool			DisableVehicleFlipKill;
	static bool			Unsquishable;
	static int			UnsquishableArmor;
	static int			UnsquishableArmor2;
	static int			UnsquishableArmor3;
	static int			UnsquishableArmor4;
	static bool			NeutralVehiclePointsFix;
	static float		DrawDistance;
	static int			ScriptsLastTeamTime;
	static bool			NukeWeatherDisable;
	static bool			IonWeatherDisable;
	static StringClass	MapPrefix;

	//
	//	Team and chat colours, 0..1 per channel as the renderer wants them
	//
	static float		NodHouseColor[3];
	static float		GDIHouseColor[3];
	static float		PrivateMessageColor[3];
	static float		PublicMessageColor[3];

	////////////////////////////////////////////////////////////////
	//	tt.ini [General] -- HUD and purchase terminal
	//
	//	TT reads these through ReadHUDIniBits, whose parameter is named
	//	`hudini` but which engine2.cpp:646 calls with `ttini`.  They are tt.ini
	//	options despite the name.
	////////////////////////////////////////////////////////////////
	static bool			UseExtraPTPages;
	static bool			NewUnpurchasableLogic;
	static float		RefillLimit;
	static bool			AlternateSelectEnabled;
	static bool			SidebarSoundsEnabled;
	static bool			DisableKillMessages;

	static StringClass	SidebarRefillSound;
	static StringClass	SidebarInfantrySound;
	static StringClass	SidebarVehicleSound;

	static StringClass	GDIUpArrowTexture;
	static StringClass	GDIDownArrowTexture;
	static StringClass	GDIBackgroundTexture1;
	static StringClass	GDIBackgroundTexture2;
	static StringClass	NODUpArrowTexture;
	static StringClass	NODDownArrowTexture;
	static StringClass	NODBackgroundTexture1;
	static StringClass	NODBackgroundTexture2;

	static StringClass	GDIAlternateSelectTexture[4];
	static StringClass	NODAlternateSelectTexture[4];

	static StringClass	ModRegistryKey;
	static StringClass	VersionRegistryKey;
	static StringClass	WOLUrlRegistryKey;

	////////////////////////////////////////////////////////////////
	//	hud.ini [General] -- UI
	////////////////////////////////////////////////////////////////
	static bool			ScrollingRadarMap;
	static bool			HidePlayerList;
	static bool			HideBottomText;
	static int			LodBudgetDialogValue1;
	static int			LodBudgetDialogValue2;

	static bool			ListColumnColorEnabled;
	static float		ListColumnColor[3];

	//
	//	Styling colours, packed 0xAARRGGBB.  TT accepts either `<name>Hex` as a
	//	hex string or `<name>Alpha`/`Red`/`Green`/`Blue` as four integers.
	//
	static unsigned int	TitleColor;
	static unsigned int	TitleHilightColor;
	static unsigned int	TitleShadowColor;
	static unsigned int	TextColor;
	static unsigned int	TextShadowColor;
	static unsigned int	LineColor;
	static unsigned int	BkColor;
	static unsigned int	DisabledTextColor;
	static unsigned int	DisabledTextShadowColor;
	static unsigned int	DisabledLineColor;
	static unsigned int	DisabledBkColor;
	static unsigned int	HilightColor;
	static unsigned int	TabTextColor;
	static unsigned int	TabGlowColor;

	static unsigned int	DialogTextTitleColor;
	static unsigned int	DialogTextTitleGlowColor;
	static unsigned int	MenuHilightColor;
	static unsigned int	MerchandiseTextColor;
	static unsigned int	MenuStaticGlowColor;
	static unsigned int	MenuActiveGlowColor;
	static unsigned int	MenuPushedBaseGlowColor;
	static unsigned int	MenuPushedHighlightGlowColor;

	//
	//	Pack an ARGB quad the way the styling options store them
	//
	static unsigned int	Make_Color (int a, int r, int g, int b)
	{
		return	((unsigned int)(a & 0xFF) << 24) |
					((unsigned int)(r & 0xFF) << 16) |
					((unsigned int)(g & 0xFF) << 8) |
					 (unsigned int)(b & 0xFF);
	}

private:

	////////////////////////////////////////////////////////////////
	//	Private methods
	////////////////////////////////////////////////////////////////
	static bool			Load_File (const char *filename, INIClass &ini);
	static void			Load_Gameplay (const INIClass &ini);
	static void			Load_Terminal (const INIClass &ini);
	static void			Load_UI (const INIClass &ini);

	static void			Get_Channels (const INIClass &ini, const char *prefix,
										int def_r, int def_g, int def_b, float *out);
	static unsigned int	Get_Color (const INIClass &ini, const char *entry, unsigned int defvalue);
	static StringClass	Get_Text (const INIClass &ini, const char *entry, const char *defvalue = "");

	////////////////////////////////////////////////////////////////
	//	Private member data
	////////////////////////////////////////////////////////////////
	static bool			IsLoaded;
};


#endif //__TTSETTINGS_H
