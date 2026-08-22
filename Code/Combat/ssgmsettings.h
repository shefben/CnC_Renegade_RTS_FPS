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
 *	SSGMSettingsClass -- what a server operator asked for in ssgm.ini.
 *
 *	The 4.8.4 package's server-side game manager keeps its configuration in `ssgm.ini`: which
 *	buildings are in play, whether purchases are free, what a character spawns holding, what a
 *	wrecked vehicle leaves behind.  Every option here carries SSGM's own name and SSGM's own
 *	default, so a server with no ssgm.ini behaves exactly as SSGM does with no ssgm.ini.
 *
 *	Two scopes, one value.  SSGM reads `[General]` at startup and the map's own section at every
 *	level load, and a handful of options may be answered differently per map.  Rather than keep
 *	a global and a map copy of each and make every reader remember which to look at, the global
 *	value is the fallback and what is published here is always the answer for the level now
 *	loaded.
 *
 *	This lives in Combat because that is where the objects the settings govern live -- buildings,
 *	powerups, vehicles, soldiers.  The server management built on top of it is in Commando.
 *
 **********************************************************************************************/

#ifndef	SSGMSETTINGS_H
#define	SSGMSETTINGS_H

#ifndef	ALWAYS_H
	#include "always.h"
#endif

#include "wwstring.h"
#include "vector.h"
#include "hashtemplate.h"

class INIClass;


/*
**	What the sky is doing, if a map asked for anything.
*/
enum SSGMWeatherEnum
{
	SSGM_WEATHER_NONE	= 0,
	SSGM_WEATHER_SNOW,
	SSGM_WEATHER_ASH,
	SSGM_WEATHER_RAIN,
};


class	SSGMSettingsClass
{
public:

	////////////////////////////////////////////////////////////////
	//	Public methods
	////////////////////////////////////////////////////////////////

	//
	//	Read ssgm.ini through the current file factory.  Safe to call when the
	//	file does not exist: every option keeps its SSGM default.  Calling it
	//	again re-reads the file, which is how a live edit is picked up.
	//
	static void			Load (void);

	//
	//	Apply the named level's own section over the global values.  Call this
	//	after Load, once per level load; the level name is taken without its
	//	path or extension, lower-cased, exactly as SSGM keys its sections.
	//
	static void			Load_Map (const char *map_name);

	static void			Shutdown (void);

	static bool			Is_Loaded (void)					{ return IsLoaded; }

	//
	//	The modification time of the file the last Load read, or zero when
	//	there was no file.  SSGM re-reads its configuration when this changes.
	//
	static unsigned	Get_File_Time (void)				{ return FileTime; }
	static bool			File_Has_Changed (void);

	////////////////////////////////////////////////////////////////
	//	Buildings out of play
	//
	//	A disabled building is present but takes no part: it neither works nor
	//	can be destroyed for points.
	////////////////////////////////////////////////////////////////

	static bool			InvincibleBuildings;
	static bool			DisableBaseDefenses;
	static bool			DisablePowerPlants;
	static bool			DisableRefineries;
	static bool			DisableSoldierFactories;
	static bool			DisableVehicleFactories;
	static bool			DisableRepairPads;
	static bool			DisableCommCenters;
	static bool			DisableConstructionYards;
	static bool			DisableShrines;
	static bool			DisableHelipads;
	static bool			DisableSpecials;
	static bool			DisableTechCenters;
	static bool			DisableNavalFactories;

	////////////////////////////////////////////////////////////////
	//	Gameplay
	////////////////////////////////////////////////////////////////

	static bool			DisableBeacons;
	static bool			FreePurchases;
	static bool			DisableExtraWeapons;
	static bool			BuildingDeathPages;
	static bool			VehicleOwnership;
	static bool			VehicleLockIcons;
	static bool			SBHCanPickupDropWeapons;
	static bool			CharactersDropDNA;
	static bool			ShowExtraMessages;
	static bool			PlayPowerupSounds;
	static bool			PlayQuakeSounds;
	static float		PowerupExpireTime;
	static float		WreckDestroySelfTime;
	static int			RefillLimit;

	//
	//	Everyone is put on this team regardless of what they chose.  -1 leaves
	//	team choice alone, which is the default.  A map may answer differently.
	//
	static int			ForceTeam;

	//
	//	What a player spawns as.  Empty means the game's own default soldier
	//	for that team.
	//
	static StringClass	GDISpawnCharacter;
	static StringClass	NodSpawnCharacter;

	//
	//	Weather for the level now loaded, from the map section or, failing
	//	that, from [General].
	//
	static int			WeatherType;

	////////////////////////////////////////////////////////////////
	//	Logging
	////////////////////////////////////////////////////////////////

	static bool			EnableGamelog;
	static bool			ExtraKillMessages;

	//
	//	TCP port the game log listens on, or zero for no log socket.  Read
	//	once at startup; changing it needs a restart, as SSGM's does.
	//
	static int			LogPort;

	////////////////////////////////////////////////////////////////
	//	Tables
	////////////////////////////////////////////////////////////////

	//
	//	A disabled preset cannot be created, bought or granted.  Both the
	//	global list and the map's own list are consulted.
	//
	static bool			Is_Preset_Disabled (int definition_id);

	//
	//	Extra weapons a character is given when it is created, and the weapons
	//	it leaves on the ground when it dies.  Null when the preset is not
	//	listed, which is the normal case.
	//
	static const DynamicVectorClass<StringClass> *	Get_Weapon_Grants (const char *preset);
	static const DynamicVectorClass<StringClass> *	Get_Weapon_Drops (const char *preset);

	//
	//	What a destroyed vehicle of this preset leaves behind, and the name to
	//	print for a preset instead of its own translated name.  Both answer
	//	false and leave the string alone when nothing was configured.
	//
	static bool			Get_Vehicle_Wreckage (const char *preset, StringClass &wreckage);
	static bool			Get_Translation_Override (const char *preset, StringClass &name);

private:

	////////////////////////////////////////////////////////////////
	//	Private methods
	////////////////////////////////////////////////////////////////

	static bool			Load_File (const char *filename, INIClass &ini, unsigned *file_time);
	static void			Load_General (const INIClass &ini);
	static void			Load_Tables (const INIClass &ini);
	static void			Free_Tables (void);

	static int			Get_Weather (const INIClass &ini, const char *section, int fallback);
	static void			Get_Preset_List (const INIClass &ini, const char *section,
								DynamicVectorClass<int> &list);
	static void			Get_Name_List (const INIClass &ini, const char *section,
								HashTemplateClass<StringClass, DynamicVectorClass<StringClass> *> &table);

	////////////////////////////////////////////////////////////////
	//	Private member data
	////////////////////////////////////////////////////////////////

	static bool			IsLoaded;
	static unsigned	FileTime;

	//
	//	The [General] answers for the options a map may override, kept so a
	//	map that says nothing falls back to them rather than to the previous
	//	map's answer.
	//
	static int			GlobalForceTeam;
	static int			GlobalWeatherType;
	static bool			GlobalExtraKillMessages;

	static DynamicVectorClass<int>	DisabledPresets;
	static DynamicVectorClass<int>	MapDisabledPresets;

	static HashTemplateClass<StringClass, DynamicVectorClass<StringClass> *>	WeaponGrants;
	static HashTemplateClass<StringClass, DynamicVectorClass<StringClass> *>	WeaponDrops;
	static HashTemplateClass<StringClass, StringClass>								VehicleWreckages;
	static HashTemplateClass<StringClass, StringClass>								TranslationOverrides;
};

#endif	// SSGMSETTINGS_H
