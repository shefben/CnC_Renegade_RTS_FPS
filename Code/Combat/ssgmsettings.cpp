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

#include "ssgmsettings.h"

#include "ini.h"
#include "wwfile.h"
#include "ffactory.h"
#include "definitionmgr.h"
#include "definition.h"
#include "wwdebug.h"

#include <string.h>


////////////////////////////////////////////////////////////////
//	Static member initialization
//
//	Every default here is SSGM's own, so a server with no ssgm.ini
//	behaves as SSGM does with no ssgm.ini.
////////////////////////////////////////////////////////////////
bool			SSGMSettingsClass::IsLoaded						= false;
unsigned		SSGMSettingsClass::FileTime						= 0;

bool			SSGMSettingsClass::InvincibleBuildings			= false;
bool			SSGMSettingsClass::DisableBaseDefenses			= false;
bool			SSGMSettingsClass::DisablePowerPlants			= false;
bool			SSGMSettingsClass::DisableRefineries			= false;
bool			SSGMSettingsClass::DisableSoldierFactories	= false;
bool			SSGMSettingsClass::DisableVehicleFactories	= false;
bool			SSGMSettingsClass::DisableRepairPads			= false;
bool			SSGMSettingsClass::DisableCommCenters			= false;
bool			SSGMSettingsClass::DisableConstructionYards	= false;
bool			SSGMSettingsClass::DisableShrines				= false;
bool			SSGMSettingsClass::DisableHelipads				= false;
bool			SSGMSettingsClass::DisableSpecials				= false;
bool			SSGMSettingsClass::DisableTechCenters			= false;
bool			SSGMSettingsClass::DisableNavalFactories		= false;

bool			SSGMSettingsClass::DisableBeacons				= false;
bool			SSGMSettingsClass::FreePurchases					= false;
bool			SSGMSettingsClass::DisableExtraWeapons			= false;
bool			SSGMSettingsClass::BuildingDeathPages			= false;
bool			SSGMSettingsClass::VehicleOwnership				= true;
bool			SSGMSettingsClass::VehicleLockIcons				= true;
bool			SSGMSettingsClass::SBHCanPickupDropWeapons	= true;
bool			SSGMSettingsClass::CharactersDropDNA			= true;
bool			SSGMSettingsClass::ShowExtraMessages			= true;
bool			SSGMSettingsClass::PlayPowerupSounds			= false;
bool			SSGMSettingsClass::PlayQuakeSounds				= false;
float			SSGMSettingsClass::PowerupExpireTime			= 10.0F;
float			SSGMSettingsClass::WreckDestroySelfTime		= 60.0F;
int			SSGMSettingsClass::RefillLimit					= 0;
int			SSGMSettingsClass::ForceTeam						= -1;
int			SSGMSettingsClass::WeatherType					= SSGM_WEATHER_NONE;

StringClass	SSGMSettingsClass::GDISpawnCharacter;
StringClass	SSGMSettingsClass::NodSpawnCharacter;

bool			SSGMSettingsClass::EnableGamelog					= false;
bool			SSGMSettingsClass::ExtraKillMessages			= false;
int			SSGMSettingsClass::LogPort							= 0;

int			SSGMSettingsClass::GlobalForceTeam				= -1;
int			SSGMSettingsClass::GlobalWeatherType			= SSGM_WEATHER_NONE;
bool			SSGMSettingsClass::GlobalExtraKillMessages	= false;

DynamicVectorClass<int>		SSGMSettingsClass::DisabledPresets;
DynamicVectorClass<int>		SSGMSettingsClass::MapDisabledPresets;

HashTemplateClass<StringClass, DynamicVectorClass<StringClass> *>	SSGMSettingsClass::WeaponGrants;
HashTemplateClass<StringClass, DynamicVectorClass<StringClass> *>	SSGMSettingsClass::WeaponDrops;
HashTemplateClass<StringClass, StringClass>								SSGMSettingsClass::VehicleWreckages;
HashTemplateClass<StringClass, StringClass>								SSGMSettingsClass::TranslationOverrides;


//
//	The one place the file is named.
//
static const char * const	SSGM_INI_FILENAME	= "ssgm.ini";


////////////////////////////////////////////////////////////////
//
//	Load_File
//
////////////////////////////////////////////////////////////////
bool
SSGMSettingsClass::Load_File (const char *filename, INIClass &ini, unsigned *file_time)
{
	if (file_time != nullptr) {
		(*file_time) = 0;
	}

	if (_TheFileFactory == nullptr) {
		return false;
	}

	FileClass *file = _TheFileFactory->Get_File (filename);
	if (file == nullptr) {
		return false;
	}

	bool retval = false;
	if (file->Is_Available ()) {

		//
		//	The modification time is what tells a running server that the
		//	operator has edited the file, so it is taken from the same open
		//	that reads it rather than from a second look at the disk.
		//
		if (file_time != nullptr) {
			file->Open (FileClass::READ);
			(*file_time) = file->Get_Date_Time ();
			file->Close ();
		}

		ini.Load (*file);
		retval = true;
	}

	_TheFileFactory->Return_File (file);
	return retval;
}


////////////////////////////////////////////////////////////////
//
//	File_Has_Changed
//
////////////////////////////////////////////////////////////////
bool
SSGMSettingsClass::File_Has_Changed (void)
{
	if (_TheFileFactory == nullptr) {
		return false;
	}

	FileClass *file = _TheFileFactory->Get_File (SSGM_INI_FILENAME);
	if (file == nullptr) {
		return false;
	}

	unsigned time = 0;
	if (file->Is_Available ()) {
		file->Open (FileClass::READ);
		time = file->Get_Date_Time ();
		file->Close ();
	}

	_TheFileFactory->Return_File (file);
	return (time != FileTime);
}


////////////////////////////////////////////////////////////////
//
//	Get_Weather
//
////////////////////////////////////////////////////////////////
int
SSGMSettingsClass::Get_Weather (const INIClass &ini, const char *section, int fallback)
{
	StringClass weather;
	ini.Get_String (weather, section, "WeatherType", "None");

	if (weather.Compare_No_Case ("Snow") == 0) {
		return SSGM_WEATHER_SNOW;
	}

	if (weather.Compare_No_Case ("Ash") == 0) {
		return SSGM_WEATHER_ASH;
	}

	if (weather.Compare_No_Case ("Rain") == 0) {
		return SSGM_WEATHER_RAIN;
	}

	//
	//	"None" is also what a section that says nothing reads as, and SSGM
	//	treats an unanswered map as "whatever the server said", not as clear
	//	skies.  That is why this takes a fallback rather than returning none.
	//
	return fallback;
}


////////////////////////////////////////////////////////////////
//
//	Get_Preset_List
//
////////////////////////////////////////////////////////////////
void
SSGMSettingsClass::Get_Preset_List
(
	const INIClass						&ini,
	const char							*section,
	DynamicVectorClass<int>			&list
)
{
	list.Delete_All ();

	int count = ini.Entry_Count (section);
	for (int index = 0; index < count; index ++) {

		const char *entry = ini.Get_Entry (section, index);
		if (entry == nullptr) {
			continue;
		}

		DefinitionClass *definition = DefinitionMgrClass::Find_Named_Definition (entry);
		if (definition != nullptr) {
			list.Add ((int)definition->Get_ID ());
		} else {
			WWDEBUG_SAY (("SSGM: [%s] names preset '%s', which does not exist\n", section, entry));
		}
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Get_Name_List
//
////////////////////////////////////////////////////////////////
void
SSGMSettingsClass::Get_Name_List
(
	const INIClass																	&ini,
	const char																		*section,
	HashTemplateClass<StringClass, DynamicVectorClass<StringClass> *>	&table
)
{
	int count = ini.Entry_Count (section);
	for (int index = 0; index < count; index ++) {

		const char *entry = ini.Get_Entry (section, index);
		if (entry == nullptr) {
			continue;
		}

		StringClass value;
		ini.Get_String (value, section, entry);

		DynamicVectorClass<StringClass> *names = new DynamicVectorClass<StringClass>;

		//
		//	A comma-separated list, trimmed of the spaces an operator is
		//	likely to have typed around the commas.
		//
		for (char *token = ::strtok (value.Peek_Buffer (), ","); token != nullptr;
				token = ::strtok (nullptr, ",")) {

			while ((*token) == ' ' || (*token) == '\t') {
				token ++;
			}

			StringClass name = token;
			name.Trim ();

			if (!name.Is_Empty ()) {
				names->Add (name);
			}
		}

		if (names->Count () > 0) {
			table.Insert (entry, names);
		} else {
			delete names;
		}
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Load_General
//
////////////////////////////////////////////////////////////////
void
SSGMSettingsClass::Load_General (const INIClass &ini)
{
	static const char * const	SECTION	= "General";

	InvincibleBuildings		= ini.Get_Bool (SECTION, "InvincibleBuildings", false);
	DisableBaseDefenses		= ini.Get_Bool (SECTION, "DisableBaseDefenses", false);
	DisablePowerPlants		= ini.Get_Bool (SECTION, "DisablePowerPlants", false);
	DisableRefineries			= ini.Get_Bool (SECTION, "DisableRefineries", false);
	DisableSoldierFactories	= ini.Get_Bool (SECTION, "DisableSoldierFactories", false);
	DisableVehicleFactories	= ini.Get_Bool (SECTION, "DisableVehicleFactories", false);
	DisableRepairPads			= ini.Get_Bool (SECTION, "DisableRepairPads", false);
	DisableCommCenters		= ini.Get_Bool (SECTION, "DisableCommCenters", false);
	DisableConstructionYards= ini.Get_Bool (SECTION, "DisableConstructionYards", false);
	DisableShrines				= ini.Get_Bool (SECTION, "DisableShrines", false);
	DisableHelipads			= ini.Get_Bool (SECTION, "DisableHelipads", false);
	DisableSpecials			= ini.Get_Bool (SECTION, "DisableSpecials", false);
	DisableTechCenters		= ini.Get_Bool (SECTION, "DisableTechCenters", false);
	DisableNavalFactories	= ini.Get_Bool (SECTION, "DisableNavalFactories", false);

	DisableBeacons				= ini.Get_Bool (SECTION, "DisableBeacons", false);
	FreePurchases				= ini.Get_Bool (SECTION, "FreePurchases", false);
	DisableExtraWeapons		= ini.Get_Bool (SECTION, "DisableExtraWeapons", false);
	BuildingDeathPages		= ini.Get_Bool (SECTION, "BuildingDeathPages", false);
	VehicleOwnership			= ini.Get_Bool (SECTION, "VehicleOwnership", true);
	VehicleLockIcons			= ini.Get_Bool (SECTION, "VehicleLockIcons", true);
	SBHCanPickupDropWeapons	= ini.Get_Bool (SECTION, "SBHCanPickupDropWeapons", true);
	CharactersDropDNA			= ini.Get_Bool (SECTION, "CharactersDropDNA", true);
	ShowExtraMessages			= ini.Get_Bool (SECTION, "ShowExtraMessages", true);
	PlayPowerupSounds			= ini.Get_Bool (SECTION, "PlayPowerupSounds", false);
	PlayQuakeSounds			= ini.Get_Bool (SECTION, "PlayQuakeSounds", false);

	PowerupExpireTime			= ini.Get_Float (SECTION, "PowerupExpireTime", 10.0F);
	WreckDestroySelfTime		= ini.Get_Float (SECTION, "WreckDestroySelfTime", 60.0F);
	RefillLimit					= ini.Get_Int (SECTION, "RefillLimit", 0);

	EnableGamelog				= ini.Get_Bool (SECTION, "EnableGamelog", false);
	LogPort						= ini.Get_Int (SECTION, "Port", 0);

	//
	//	Empty means "the game's own soldier for that team".  SSGM writes the
	//	name it found at map load time into its own default, which cannot be
	//	done here because the settings are read before a level exists; the
	//	empty answer is resolved by whoever spawns the player.
	//
	ini.Get_String (GDISpawnCharacter, SECTION, "GDISpawnChar", "");
	ini.Get_String (NodSpawnCharacter, SECTION, "NodSpawnChar", "");

	//
	//	The three a map may answer differently.  These are the fallbacks; the
	//	published values are set by Load_Map.
	//
	GlobalForceTeam			= ini.Get_Int (SECTION, "ForceTeam", -1);
	GlobalWeatherType			= Get_Weather (ini, SECTION, SSGM_WEATHER_NONE);
	GlobalExtraKillMessages	= ini.Get_Bool (SECTION, "ExtraKillMessages", false);

	ForceTeam					= GlobalForceTeam;
	WeatherType					= GlobalWeatherType;
	ExtraKillMessages			= GlobalExtraKillMessages;

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Load_Tables
//
////////////////////////////////////////////////////////////////
void
SSGMSettingsClass::Load_Tables (const INIClass &ini)
{
	Get_Name_List (ini, "WeaponGrant", WeaponGrants);
	Get_Name_List (ini, "WeaponDrop", WeaponDrops);

	int count = ini.Entry_Count ("VehicleWreckage");
	for (int index = 0; index < count; index ++) {
		const char *entry = ini.Get_Entry ("VehicleWreckage", index);
		if (entry != nullptr) {
			StringClass wreckage;
			ini.Get_String (wreckage, "VehicleWreckage", entry);
			VehicleWreckages.Insert (entry, wreckage);
		}
	}

	count = ini.Entry_Count ("Translation_Overrides");
	for (int index = 0; index < count; index ++) {
		const char *entry = ini.Get_Entry ("Translation_Overrides", index);
		if (entry != nullptr) {
			StringClass name;
			ini.Get_String (name, "Translation_Overrides", entry);
			TranslationOverrides.Insert (entry, name);
		}
	}

	//
	//	The globally disabled presets.  A map may disable more; it may not
	//	re-enable any of these.
	//
	Get_Preset_List (ini, "PresetDisable", DisabledPresets);

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Free_Tables
//
////////////////////////////////////////////////////////////////
void
SSGMSettingsClass::Free_Tables (void)
{
	{
		HashTemplateIterator<StringClass, DynamicVectorClass<StringClass> *> it (WeaponGrants);
		for (it.First (); !it.Is_Done (); it.Next ()) {
			delete it.Peek_Value ();
		}
		WeaponGrants.Remove_All ();
	}

	{
		HashTemplateIterator<StringClass, DynamicVectorClass<StringClass> *> it (WeaponDrops);
		for (it.First (); !it.Is_Done (); it.Next ()) {
			delete it.Peek_Value ();
		}
		WeaponDrops.Remove_All ();
	}

	VehicleWreckages.Remove_All ();
	TranslationOverrides.Remove_All ();

	DisabledPresets.Delete_All ();
	MapDisabledPresets.Delete_All ();

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Load
//
////////////////////////////////////////////////////////////////
void
SSGMSettingsClass::Load (void)
{
	Free_Tables ();

	INIClass ini;
	if (Load_File (SSGM_INI_FILENAME, ini, &FileTime)) {
		Load_General (ini);
		Load_Tables (ini);
	}

	IsLoaded = true;

	WWDEBUG_SAY (("SSGMSettingsClass::Load - %d disabled presets, gamelog %s, log port %d\n",
		DisabledPresets.Count (), EnableGamelog ? "on" : "off", LogPort));

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Load_Map
//
////////////////////////////////////////////////////////////////
void
SSGMSettingsClass::Load_Map (const char *map_name)
{
	MapDisabledPresets.Delete_All ();

	//
	//	Start from what the server said, so a map that answers nothing does
	//	not inherit the previous map's answers.
	//
	ForceTeam			= GlobalForceTeam;
	WeatherType			= GlobalWeatherType;
	ExtraKillMessages	= GlobalExtraKillMessages;

	if (map_name == nullptr || (*map_name) == 0) {
		return ;
	}

	INIClass ini;
	if (!Load_File (SSGM_INI_FILENAME, ini, nullptr)) {
		return ;
	}

	//
	//	SSGM keys a map's section by the lower-cased map name.
	//
	StringClass section = map_name;
	::_strlwr (section.Peek_Buffer ());

	ForceTeam			= ini.Get_Int (section, "ForceTeam", GlobalForceTeam);
	WeatherType			= Get_Weather (ini, section, GlobalWeatherType);
	ExtraKillMessages	= ini.Get_Bool (section, "ExtraKillMessages", GlobalExtraKillMessages);

	StringClass preset_section = section;
	preset_section += "_PresetDisable";
	Get_Preset_List (ini, preset_section, MapDisabledPresets);

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Shutdown
//
////////////////////////////////////////////////////////////////
void
SSGMSettingsClass::Shutdown (void)
{
	Free_Tables ();
	IsLoaded = false;
	FileTime = 0;
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Is_Preset_Disabled
//
////////////////////////////////////////////////////////////////
bool
SSGMSettingsClass::Is_Preset_Disabled (int definition_id)
{
	for (int index = 0; index < DisabledPresets.Count (); index ++) {
		if (DisabledPresets[index] == definition_id) {
			return true;
		}
	}

	for (int index = 0; index < MapDisabledPresets.Count (); index ++) {
		if (MapDisabledPresets[index] == definition_id) {
			return true;
		}
	}

	return false;
}


////////////////////////////////////////////////////////////////
//
//	Get_Weapon_Grants
//
////////////////////////////////////////////////////////////////
const DynamicVectorClass<StringClass> *
SSGMSettingsClass::Get_Weapon_Grants (const char *preset)
{
	if (preset == nullptr) {
		return nullptr;
	}

	DynamicVectorClass<StringClass> *names = nullptr;
	WeaponGrants.Get (StringClass (preset), names);
	return names;
}


////////////////////////////////////////////////////////////////
//
//	Get_Weapon_Drops
//
////////////////////////////////////////////////////////////////
const DynamicVectorClass<StringClass> *
SSGMSettingsClass::Get_Weapon_Drops (const char *preset)
{
	if (preset == nullptr) {
		return nullptr;
	}

	DynamicVectorClass<StringClass> *names = nullptr;
	WeaponDrops.Get (StringClass (preset), names);
	return names;
}


////////////////////////////////////////////////////////////////
//
//	Get_Vehicle_Wreckage
//
////////////////////////////////////////////////////////////////
bool
SSGMSettingsClass::Get_Vehicle_Wreckage (const char *preset, StringClass &wreckage)
{
	if (preset == nullptr) {
		return false;
	}

	StringClass value;
	if (!VehicleWreckages.Get (StringClass (preset), value) || value.Is_Empty ()) {
		return false;
	}

	wreckage = value;
	return true;
}


////////////////////////////////////////////////////////////////
//
//	Get_Translation_Override
//
////////////////////////////////////////////////////////////////
bool
SSGMSettingsClass::Get_Translation_Override (const char *preset, StringClass &name)
{
	if (preset == nullptr) {
		return false;
	}

	StringClass value;
	if (!TranslationOverrides.Get (StringClass (preset), value) || value.Is_Empty ()) {
		return false;
	}

	name = value;
	return true;
}
