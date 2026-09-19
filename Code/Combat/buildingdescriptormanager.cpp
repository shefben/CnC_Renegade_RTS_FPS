/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 Electronic Arts Inc.
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
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/buildingdescriptormanager.cpp         $*
 *                                                                                             *
 *	Roadmap Section 39, Phase 33 -- building model semantic JSON + live building definitions.    *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "buildingdescriptormanager.h"

#include "rawfile.h"

#include <cmath>

#include "nlohmann/json.hpp"

using nlohmann::json;


struct BuildingDescriptorRecord
{
	BuildingDescriptorRecord(void) : SourcePath("") { }
	bool	operator == (const BuildingDescriptorRecord &) const	{ return false; }
	bool	operator != (const BuildingDescriptorRecord &) const	{ return true; }

	BuildingModelDescriptor	Descriptor;
	StringClass					SourcePath;		// empty for a descriptor loaded from text, never on disk
};

static DynamicVectorClass<BuildingDescriptorRecord>	_Records;
static int		_ReloadFailureCount = 0;
static StringClass	_LastError ("");


static int	Find_Record(const char * definition_name)
{
	if (definition_name == nullptr) return -1;
	for (int i = 0; i < _Records.Count (); i++) {
		if (_Records [i].Descriptor.DefinitionName.Compare_No_Case (definition_name) == 0) {
			return i;
		}
	}
	return -1;
}


/***********************************************************************************************
 * Validate_Descriptor -- Section 39's named invalid cases, as far as this scope covers them     *
 *=============================================================================================*/
static bool	Validate_Descriptor(const BuildingModelDescriptor & d,StringClass * error_out)
{
	#define FAIL(msg) { if (error_out) *error_out = msg; return false; }

	if (d.DefinitionName.Get_Length () == 0)				FAIL ("empty definition_name");
	if (d.ModelName.Get_Length () == 0)						FAIL ("empty model_name");
	if (d.SchemaVersion != 1)										FAIL ("unsupported schema_version");
	if (d.Footprint.Width <= 0.0f || d.Footprint.Depth <= 0.0f)	FAIL ("empty footprint");
	if (!std::isfinite (d.Footprint.Width) || !std::isfinite (d.Footprint.Depth))
																					FAIL ("non-finite footprint");

	if (!std::isfinite (d.Power.Generation) || d.Power.Generation < 0.0f)
																					FAIL ("negative/non-finite power.generation");
	if (!std::isfinite (d.Power.Consumption) || d.Power.Consumption < 0.0f)
																					FAIL ("negative/non-finite power.consumption");
	if (d.Power.Priority < 0)										FAIL ("invalid power.priority");

	if (d.Communications.Enabled) {
		if (!std::isfinite (d.Communications.VisionRadius) || d.Communications.VisionRadius < 0.0f)
																				FAIL ("negative/non-finite communications vision_radius");
		if (!std::isfinite (d.Communications.RadarRadius) || d.Communications.RadarRadius < 0.0f)
																				FAIL ("negative/non-finite communications radar_radius");
		if (!std::isfinite (d.Communications.LastKnownRetentionSeconds) || d.Communications.LastKnownRetentionSeconds < 0.0f)
																				FAIL ("negative/non-finite communications last_known_retention_seconds");
	}

	if (d.TiberiumSilo.Enabled) {
		if (!std::isfinite (d.TiberiumSilo.WalletCapacityBonusPerPlayer) || d.TiberiumSilo.WalletCapacityBonusPerPlayer < 0.0f)
																				FAIL ("negative wallet_capacity_bonus_per_player");
	}

	if (d.ConstructionVisual.DurationSeconds <= 0.0f)		FAIL ("invalid construction_visual.duration_seconds");
	if (d.ConstructionVisual.BlockWidth <= 0.0f || d.ConstructionVisual.BlockDepth <= 0.0f)
																					FAIL ("invalid construction_visual block dimensions");
	if (d.ConstructionVisual.RowHeight <= 0.0f)					FAIL ("invalid construction_visual.row_height");
	if (d.ConstructionVisual.MaxBlocks <= 0)						FAIL ("invalid construction_visual.max_blocks");
	if (d.ConstructionVisual.SourceRevealStart < 0.0f || d.ConstructionVisual.SourceRevealStart > 1.0f)
																					FAIL ("invalid construction_visual.source_reveal_start");
	if (d.ConstructionVisual.BlockFadeStart < 0.0f || d.ConstructionVisual.BlockFadeStart > 1.0f)
																					FAIL ("invalid construction_visual.block_fade_start");

	//	"incompatible capability/building role" -- the one concrete case Section 39 names that
	//	this scope can check without a role concept of its own: a Power Plant and a
	//	communications-capable building are mutually exclusive roles in every donor definition.
	if (!d.Power.RequiresPower && d.Communications.Enabled) {
		FAIL ("a generator (requires_power=false) cannot also be a communications building");
	}

	#undef FAIL
	return true;
}


/***********************************************************************************************
 * Parse_And_Validate -- JSON text in, a validated descriptor out                                *
 *=============================================================================================*/
static bool	Parse_And_Validate(const char * definition_name,const char * json_text,
										  BuildingModelDescriptor * out,StringClass * error_out)
{
	if (json_text == nullptr) {
		if (error_out) *error_out = "no JSON text supplied";
		return false;
	}

	json root;
	try {
		root = json::parse (json_text);

		BuildingModelDescriptor d;
		d.DefinitionName	= definition_name;		// registry identity, not whatever the file says
		d.ModelName			= root.value ("model_name",std::string ("")).c_str ();
		d.SchemaVersion	= root.value ("schema_version",1u);

		if (root.contains ("footprint") && root ["footprint"].is_object ()) {
			d.Footprint.Width = root ["footprint"].value ("width",0.0f);
			d.Footprint.Depth = root ["footprint"].value ("depth",0.0f);
		}

		if (root.contains ("power") && root ["power"].is_object ()) {
			auto & p = root ["power"];
			d.Power.RequiresPower	= p.value ("requires_power",true);
			d.Power.Generation		= p.value ("generation",0.0f);
			d.Power.Consumption		= p.value ("consumption",0.0f);
			d.Power.Priority			= p.value ("priority",0);
		}

		if (root.contains ("capabilities") && root ["capabilities"].is_object ()) {
			auto & c = root ["capabilities"];

			if (c.contains ("communications") && c ["communications"].is_object ()) {
				auto & comm = c ["communications"];
				d.Communications.Enabled						= comm.value ("enabled",false);
				d.Communications.VisionRadius					= comm.value ("vision_radius",0.0f);
				d.Communications.RadarRadius					= comm.value ("radar_radius",0.0f);
				d.Communications.LastKnownRetentionSeconds	= comm.value ("last_known_retention_seconds",10.0f);
				d.Communications.RequiresBuildingOnline		= comm.value ("requires_building_online",true);
			}
			if (c.contains ("tiberium_silo") && c ["tiberium_silo"].is_object ()) {
				auto & silo = c ["tiberium_silo"];
				d.TiberiumSilo.Enabled								= silo.value ("enabled",false);
				d.TiberiumSilo.WalletCapacityBonusPerPlayer	= silo.value ("wallet_capacity_bonus_per_player",0.0f);
				d.TiberiumSilo.RequiresBuildingOnline			= silo.value ("requires_building_online",true);
			}
		}

		if (root.contains ("construction_visual") && root ["construction_visual"].is_object ()) {
			auto & cv = root ["construction_visual"];
			d.ConstructionVisual.DurationSeconds		= cv.value ("duration_seconds",15.0f);
			d.ConstructionVisual.BlockWidth				= cv.value ("block_width",2.0f);
			d.ConstructionVisual.BlockDepth				= cv.value ("block_depth",2.0f);
			d.ConstructionVisual.RowHeight				= cv.value ("row_height",1.5f);
			d.ConstructionVisual.SourceRevealStart	= cv.value ("source_reveal_start",0.75f);
			d.ConstructionVisual.BlockFadeStart		= cv.value ("block_fade_start",0.9f);
			d.ConstructionVisual.MaxBlocks				= cv.value ("max_blocks",2048);
		}

		if (root.contains ("anchors") && root ["anchors"].is_object ()) {
			for (auto & element : root ["anchors"].items ()) {
				if (!element.value ().is_string ()) continue;
				BuildingAnchorEntry entry;
				entry.Role		= element.key ().c_str ();
				entry.BoneName	= element.value ().get<std::string> ().c_str ();
				d.Anchors.Add (entry);
			}
		}

		StringClass validation_error ("");
		if (!Validate_Descriptor (d,&validation_error)) {
			if (error_out) *error_out = validation_error;
			return false;
		}

		*out = d;
		return true;

	} catch (const std::exception & e) {
		if (error_out) *error_out = e.what ();
		return false;
	}
}


static bool	Read_File_To_String(const char * file_path,StringClass * out)
{
	RawFileClass file (file_path);
	if (!file.Open (FileClass::READ)) {
		return false;
	}
	int size = file.Size ();
	if (size <= 0) {
		file.Close ();
		return false;
	}

	char * buffer = new char [size + 1];
	int read = file.Read (buffer,size);
	buffer [read] = '\0';
	file.Close ();

	*out = buffer;
	delete [] buffer;
	return true;
}


void BuildingDescriptorManager::Init(void)
{
	_Records.Delete_All ();
	_ReloadFailureCount = 0;
	_LastError = "";
}

void BuildingDescriptorManager::Shutdown(void)
{
	_Records.Delete_All ();
}


bool BuildingDescriptorManager::Load_Descriptor_From_Text(const char * definition_name,const char * json_text,
																				  StringClass * error_out)
{
	if (definition_name == nullptr || definition_name [0] == '\0') {
		if (error_out) *error_out = "empty definition name";
		return false;
	}

	BuildingModelDescriptor descriptor;
	StringClass error ("");
	if (!Parse_And_Validate (definition_name,json_text,&descriptor,&error)) {
		_LastError = error;
		if (error_out) *error_out = error;
		return false;
	}

	int index = Find_Record (definition_name);
	if (index < 0) {
		BuildingDescriptorRecord record;
		record.Descriptor	= descriptor;
		record.SourcePath	= "";
		_Records.Add (record);
	} else {
		_Records [index].Descriptor = descriptor;
		//	SourcePath is left as it was -- loading new text over a file-backed descriptor does
		//	not change where a later Reload_Descriptor re-reads from.
	}
	return true;
}

bool BuildingDescriptorManager::Load_Descriptor_From_File(const char * definition_name,const char * file_path,
																				  StringClass * error_out)
{
	StringClass text ("");
	if (!Read_File_To_String (file_path,&text)) {
		if (error_out) *error_out = "could not read file";
		_LastError = "could not read file";
		return false;
	}
	if (!Load_Descriptor_From_Text (definition_name,text.Peek_Buffer (),error_out)) {
		return false;
	}

	int index = Find_Record (definition_name);
	if (index >= 0) {
		_Records [index].SourcePath = file_path;
	}
	return true;
}


bool BuildingDescriptorManager::Reload_Descriptor(const char * definition_name,StringClass * error_out)
{
	int index = Find_Record (definition_name);
	if (index < 0) {
		if (error_out) *error_out = "no such descriptor is currently loaded";
		return false;
	}
	if (_Records [index].SourcePath.Get_Length () == 0) {
		if (error_out) *error_out = "this descriptor was loaded from text, not a file, and has nothing to reload from";
		return false;
	}

	StringClass text ("");
	if (!Read_File_To_String (_Records [index].SourcePath.Peek_Buffer (),&text)) {
		_ReloadFailureCount++;
		_LastError = "could not read file";
		if (error_out) *error_out = _LastError;
		return false;			// last known-good Descriptor is untouched
	}

	BuildingModelDescriptor candidate;
	StringClass error ("");
	if (!Parse_And_Validate (definition_name,text.Peek_Buffer (),&candidate,&error)) {
		_ReloadFailureCount++;
		_LastError = error;
		if (error_out) *error_out = error;
		return false;			// last known-good Descriptor is untouched
	}

	//	Atomic publish: one assignment, nothing observes a half-updated descriptor.
	_Records [index].Descriptor = candidate;
	return true;
}

int BuildingDescriptorManager::Reload_All_Descriptors(void)
{
	int succeeded = 0;
	for (int i = 0; i < _Records.Count (); i++) {
		if (_Records [i].SourcePath.Get_Length () == 0) continue;
		StringClass error ("");
		if (Reload_Descriptor (_Records [i].Descriptor.DefinitionName.Peek_Buffer (),&error)) {
			succeeded++;
		}
	}
	return succeeded;
}


const BuildingModelDescriptor * BuildingDescriptorManager::Get_Descriptor(const char * definition_name)
{
	int index = Find_Record (definition_name);
	if (index < 0) return nullptr;
	return &_Records [index].Descriptor;
}

int BuildingDescriptorManager::Get_Descriptor_Count(void)
{
	return _Records.Count ();
}

bool BuildingDescriptorManager::Is_Loaded(const char * definition_name)
{
	return Find_Record (definition_name) >= 0;
}

int BuildingDescriptorManager::Get_Reload_Failure_Count(void)
{
	return _ReloadFailureCount;
}

const char * BuildingDescriptorManager::Get_Last_Error(void)
{
	return _LastError.Peek_Buffer ();
}

void BuildingDescriptorManager::Unload_Descriptor(const char * definition_name)
{
	int index = Find_Record (definition_name);
	if (index >= 0) {
		_Records.Delete (index);
	}
}

void BuildingDescriptorManager::Unload_All(void)
{
	_Records.Delete_All ();
}
