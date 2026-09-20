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

#include "building.h"
#include "definition.h"
#include "definitionclassids.h"
#include "definitionmgr.h"
#include "rawfile.h"
#include "win.h"
#include "wwdebug.h"

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

	if (d.Build.Buildable) {
		if (d.Build.Cost < 0)										FAIL ("negative build.cost");
		if (!std::isfinite (d.Build.BuildTimeSeconds) || d.Build.BuildTimeSeconds < 0.0f)
																					FAIL ("negative/non-finite build.build_time_seconds");
		if (d.Build.TechLevel < 0)									FAIL ("negative build.tech_level");
	}

	if (d.Production.Kind != BUILDING_PRODUCTION_NONE) {
		if (d.Production.QueueCapacity <= 0)					FAIL ("invalid production.queue_capacity");
	}

	if (!std::isfinite (d.Placement.MaxSlopeDegrees) ||
		 d.Placement.MaxSlopeDegrees < 0.0f || d.Placement.MaxSlopeDegrees > 90.0f)
																					FAIL ("invalid placement_rules.max_slope_degrees");
	if (!std::isfinite (d.Placement.ExitClearance) || d.Placement.ExitClearance < 0.0f)
																					FAIL ("negative/non-finite placement_rules.exit_clearance");

	/*
	**	Q-016's damage-visual table.  Validated as a table rather than as three names because the
	**	schema allows extra health-threshold states between intact and damaged, and the selection
	**	scan in Find_State_Index is only correct if the table really is sorted, really is free of
	**	duplicate thresholds, and really does reach 0.0.  Checking that here means the runtime
	**	never has to.
	*/
	if (d.DamageVisual.States.Count () == 0)			FAIL ("empty damage_visual state table");
	{
		float previous = 2.0f;
		for (int i = 0; i < d.DamageVisual.States.Count (); i++) {
			const BuildingDamageStateEntry & entry = d.DamageVisual.States [i];
			if (entry.ModelName.Get_Length () == 0)	FAIL ("damage_visual state with no model");
			if (!std::isfinite (entry.MinHealthFraction) ||
				 entry.MinHealthFraction < 0.0f || entry.MinHealthFraction > 1.0f)
																		FAIL ("damage_visual health_fraction outside 0..1");
			if (entry.MinHealthFraction >= previous)	FAIL ("damage_visual thresholds not strictly descending");
			previous = entry.MinHealthFraction;
		}
		if (d.DamageVisual.States [d.DamageVisual.States.Count () - 1].MinHealthFraction != 0.0f) {
			FAIL ("damage_visual table does not reach health_fraction 0");
		}
	}

	/*
	**	Only a buildable descriptor needs a preset: that is the one case where something has to
	**	create the canonical BuildingGameObj at runtime.  Every map-authored descriptor names no
	**	preset because the level already placed the object.
	*/
	if (d.Build.Buildable && (d.GameObjectPreset.Get_Length () == 0)) {
		FAIL ("build.buildable requires game_object_preset");
	}

	/*
	**	Q-017 -- when real building definitions are loaded (the running game, or a tool run
	**	against real data), a named preset must actually resolve to one, and it must really be
	**	a building.  Skipped, not failed, when nothing of CLASSID_BUILDINGS is registered at
	**	all: an editor session or most self-check runs have no Objects.DDB in reach, and cannot
	**	tell "wrong name" from "nothing to check against" -- the same missing-data rule that
	**	already governs every other check in this function applies here too.
	*/
	if ((d.GameObjectPreset.Get_Length () > 0) &&
		 (DefinitionMgrClass::Get_First (CLASSID_BUILDINGS,DefinitionMgrClass::ID_SUPERCLASS) != nullptr))
	{
		DefinitionClass *preset = DefinitionMgrClass::Find_Named_Definition (d.GameObjectPreset.Peek_Buffer (),true);
		if (preset == nullptr) {
			FAIL ("game_object_preset does not resolve to any loaded definition");
		}
		if (SuperClassID_From_ClassID (preset->Get_Class_ID ()) != CLASSID_BUILDINGS) {
			FAIL ("game_object_preset resolves to a definition that is not a building");
		}
	}

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

		if (root.contains ("build") && root ["build"].is_object ()) {
			auto & b = root ["build"];
			d.Build.Buildable			= b.value ("buildable",false);
			d.Build.Cost				= b.value ("cost",0);
			d.Build.BuildTimeSeconds	= b.value ("build_time_seconds",0.0f);
			d.Build.TechLevel			= b.value ("tech_level",0);
			d.Build.Team				= b.value ("team",-1);
			d.Build.DisplayName		= b.value ("display_name",std::string ("")).c_str ();
			d.Build.IconTexture		= b.value ("icon_texture",std::string ("")).c_str ();
		}

		if (root.contains ("production") && root ["production"].is_object ()) {
			auto & p = root ["production"];
			std::string kind = p.value ("kind",std::string ("none"));
			if (kind == "infantry")					d.Production.Kind = BUILDING_PRODUCTION_INFANTRY;
			else if (kind == "ground_vehicle")	d.Production.Kind = BUILDING_PRODUCTION_GROUND_VEHICLE;
			else if (kind == "air_vehicle")		d.Production.Kind = BUILDING_PRODUCTION_AIR_VEHICLE;
			else if (kind == "none")				d.Production.Kind = BUILDING_PRODUCTION_NONE;
			else {
				if (error_out) *error_out = "unknown production.kind";
				return false;
			}
			d.Production.QueueCapacity	= p.value ("queue_capacity",5);
			d.Production.RequiresPower	= p.value ("requires_power",true);
		}

		if (root.contains ("placement_rules") && root ["placement_rules"].is_object ()) {
			auto & pr = root ["placement_rules"];
			d.Placement.MaxSlopeDegrees				= pr.value ("max_slope_degrees",15.0f);
			d.Placement.ExitClearance					= pr.value ("exit_clearance",0.0f);
			d.Placement.RequiresFoundationSupport	= pr.value ("requires_foundation_support",true);
		}

		d.GameObjectPreset = root.value ("game_object_preset",std::string ("")).c_str ();

		/*
		**	damage_visual -- Q-016.
		**
		**	A declared fraction is a *transition*: the health you fall below to enter the state it
		**	names.  So "damaged_health_fraction: 0.5 with damaged_model X" means X takes over below
		**	half health, and an extra state declaring 0.8 means that state takes over below 80%.
		**	The intact model is whatever is worn above the highest transition, which is why it has
		**	no fraction of its own to declare and cannot be accidentally outranked by an extra
		**	state -- a table where the pristine model showed at medium health and a battered one at
		**	full health would be the obvious way to get this wrong.
		**
		**	The stored table turns those transitions into the bottom of each state's band, so every
		**	reader does one descending scan and never has to know any of the above.
		*/
		{
			float damaged_fraction = 0.5f;
			StringClass intact_model	= d.ModelName;
			StringClass damaged_model ("");

			//	Transitions, before sorting: (fraction you fall below, the model you fall into).
			DynamicVectorClass<BuildingDamageStateEntry>	transitions;

			if (root.contains ("damage_visual") && root ["damage_visual"].is_object ()) {
				auto & dv = root ["damage_visual"];

				damaged_fraction	= dv.value ("damaged_health_fraction",0.5f);
				intact_model		= dv.value ("intact_model",std::string (d.ModelName.Peek_Buffer ())).c_str ();
				damaged_model		= dv.value ("damaged_model",std::string ("")).c_str ();
				d.DamageVisual.DestroyedModel = dv.value ("destroyed_model",std::string ("")).c_str ();

				if (dv.contains ("extra_states") && dv ["extra_states"].is_array ()) {
					for (auto & element : dv ["extra_states"]) {
						if (!element.is_object ()) continue;
						BuildingDamageStateEntry entry;
						entry.ModelName			= element.value ("model",std::string ("")).c_str ();
						entry.MinHealthFraction	= element.value ("health_fraction",0.0f);
						transitions.Add (entry);
					}
				}
			}

			if (intact_model.Get_Length () == 0) {
				intact_model = d.ModelName;
			}

			if (damaged_model.Get_Length () > 0) {
				BuildingDamageStateEntry damaged;
				damaged.ModelName			= damaged_model;
				damaged.MinHealthFraction	= damaged_fraction;
				transitions.Add (damaged);
			}

			//	Descending by the health each transition happens at.  Insertion sort: the table is
			//	three or four entries long and this runs once per descriptor load.
			for (int i = 1; i < transitions.Count (); i++) {
				BuildingDamageStateEntry key = transitions [i];
				int j = i - 1;
				while ((j >= 0) && (transitions [j].MinHealthFraction < key.MinHealthFraction)) {
					transitions [j + 1] = transitions [j];
					j--;
				}
				transitions [j + 1] = key;
			}

			/*
			**	Transitions to bands.  The intact model's band starts at the first transition; each
			**	transition's model holds from the next transition down; the last one reaches zero.
			**	Built this way the table is descending and terminates at 0.0 by construction, and
			**	Validate_Descriptor then re-checks both because a malformed file can still produce
			**	duplicate transitions that this arithmetic cannot repair.
			*/
			BuildingDamageStateEntry intact;
			intact.ModelName			= intact_model;
			intact.MinHealthFraction	= (transitions.Count () > 0) ? transitions [0].MinHealthFraction : 0.0f;
			d.DamageVisual.States.Add (intact);

			for (int i = 0; i < transitions.Count (); i++) {
				BuildingDamageStateEntry state;
				state.ModelName			= transitions [i].ModelName;
				state.MinHealthFraction	= (i + 1 < transitions.Count ())
												  ? transitions [i + 1].MinHealthFraction : 0.0f;
				d.DamageVisual.States.Add (state);
			}
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

const BuildingModelDescriptor * BuildingDescriptorManager::Get_Descriptor_By_Index(int index)
{
	if (index < 0 || index >= _Records.Count ()) return nullptr;
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


/***********************************************************************************************
 * Q-015 -- validation without loading                                                           *
 *                                                                                             *
 *	The editor-facing entry points.  Both run `Parse_And_Validate`, which is the same function  *
 *	`Load_Descriptor_From_Text` runs, and then discard the descriptor instead of registering    *
 *	it.  That is the whole trick: there is exactly one parser and exactly one validator in this *
 *	file, and "validate" is "load, minus the commit".  A rule added to Validate_Descriptor is   *
 *	enforced by the editor the same day, with nothing to keep in step.                           *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

//	The definition name a load of this path would have used: the file's stem.  Validating under
//	the same name a load would use is what stops a name-dependent rule passing here and failing
//	there.
static void	Definition_Name_From_Path(const char * file_path,StringClass * out)
{
	*out = "";
	if ((file_path == nullptr) || (file_path [0] == 0)) {
		return;
	}

	const char * start = file_path;
	for (const char * scan = file_path; *scan != 0; scan++) {
		if ((*scan == '\\') || (*scan == '/')) {
			start = scan + 1;
		}
	}

	StringClass name (start,true);

	int length = name.Get_Length ();
	for (int i = length - 1; i >= 0; i--) {
		if (name [i] == '.') {
			name.Erase (i,length - i);
			break;
		}
	}

	*out = name;
}


bool BuildingDescriptorManager::Validate_Text(const char * definition_name,const char * json_text,
															 StringClass * error_out)
{
	BuildingModelDescriptor scratch;
	StringClass error ("");

	//	A definition name is required by the validator (an unnamed descriptor is invalid), and a
	//	caller validating loose text may not have one, so a placeholder stands in.  It never
	//	reaches the registry, because nothing here registers anything.
	const char * name = ((definition_name != nullptr) && (definition_name [0] != 0))
								? definition_name : "unnamed";

	bool ok = Parse_And_Validate (name,json_text,&scratch,&error);
	if (error_out != nullptr) {
		*error_out = error;
	}
	return ok;
}


bool BuildingDescriptorManager::Validate_File(const char * file_path,StringClass * error_out)
{
	if ((file_path == nullptr) || (file_path [0] == 0)) {
		if (error_out != nullptr) *error_out = "no file path";
		return false;
	}

	StringClass text ("");
	if (!Read_File_To_String (file_path,&text)) {
		if (error_out != nullptr) *error_out = "could not read the file";
		return false;
	}

	StringClass name ("");
	Definition_Name_From_Path (file_path,&name);

	return Validate_Text (name.Peek_Buffer (),text.Peek_Buffer (),error_out);
}


int BuildingDescriptorManager::Validate_Directory(const char * directory_path,
																  StringClass * report_out,int * checked_out)
{
	if (checked_out != nullptr) {
		*checked_out = 0;
	}
	if ((directory_path == nullptr) || (directory_path [0] == 0)) {
		return -1;
	}

	/*
	**	Enumerated through the platform rather than through SimpleFileFactoryClass, because the
	**	factory only finds files inside the game's own search path and a content author's
	**	descriptor folder is exactly not there.  This is the same call MixFileCreator's own
	**	directory walk uses.
	*/
	StringClass base (directory_path,true);
	int length = base.Get_Length ();
	if ((length > 0) && (base [length - 1] != '\\') && (base [length - 1] != '/')) {
		base += "\\";
	}

	StringClass pattern (base,true);
	pattern += "*.json";

	WIN32_FIND_DATAA find_data = { 0 };
	HANDLE handle = ::FindFirstFileA (pattern.Peek_Buffer (),&find_data);
	if (handle == INVALID_HANDLE_VALUE) {
		return -1;
	}

	int failures = 0;
	int checked = 0;

	do {
		if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
			continue;
		}

		StringClass full_path (base,true);
		full_path += find_data.cFileName;

		StringClass error ("");
		bool ok = Validate_File (full_path.Peek_Buffer (),&error);

		checked++;
		if (!ok) {
			failures++;
		}

		if (report_out != nullptr) {
			StringClass line ("");
			if (ok) {
				line.Format ("%s: OK\r\n",find_data.cFileName);
			} else {
				line.Format ("%s: FAILED -- %s\r\n",find_data.cFileName,error.Peek_Buffer ());
			}
			*report_out += line;
		}

	} while (::FindNextFileA (handle,&find_data));

	::FindClose (handle);

	if (checked_out != nullptr) {
		*checked_out = checked;
	}
	return failures;
}


/***********************************************************************************************
 * Load_Directory -- Q-017's production load path.  Same directory walk as Validate_Directory,  *
 * Load_Descriptor_From_File instead of Validate_File.                                           *
 *=============================================================================================*/
int BuildingDescriptorManager::Load_Directory(const char * directory_path,StringClass * report_out,
															  int * loaded_out)
{
	if (loaded_out != nullptr) {
		*loaded_out = 0;
	}
	if ((directory_path == nullptr) || (directory_path [0] == 0)) {
		return -1;
	}

	StringClass base (directory_path,true);
	int length = base.Get_Length ();
	if ((length > 0) && (base [length - 1] != '\\') && (base [length - 1] != '/')) {
		base += "\\";
	}

	StringClass pattern (base,true);
	pattern += "*.json";

	WIN32_FIND_DATAA find_data = { 0 };
	HANDLE handle = ::FindFirstFileA (pattern.Peek_Buffer (),&find_data);
	if (handle == INVALID_HANDLE_VALUE) {
		return -1;
	}

	int failures = 0;
	int loaded = 0;

	do {
		if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
			continue;
		}

		StringClass full_path (base,true);
		full_path += find_data.cFileName;

		StringClass name ("");
		Definition_Name_From_Path (full_path.Peek_Buffer (),&name);

		StringClass error ("");
		bool ok = Load_Descriptor_From_File (name.Peek_Buffer (),full_path.Peek_Buffer (),&error);

		loaded++;
		if (!ok) {
			failures++;
		}

		if (report_out != nullptr) {
			StringClass line ("");
			if (ok) {
				line.Format ("%s: OK\r\n",find_data.cFileName);
			} else {
				line.Format ("%s: FAILED -- %s\r\n",find_data.cFileName,error.Peek_Buffer ());
			}
			*report_out += line;
		}

	} while (::FindNextFileA (handle,&find_data));

	::FindClose (handle);

	if (loaded_out != nullptr) {
		*loaded_out = loaded;
	}
	return failures;
}


/***********************************************************************************************
 * Resolve_Game_Object_Presets                                                                   *
 *=============================================================================================*/
int BuildingDescriptorManager::Resolve_Game_Object_Presets(StringClass * report_out)
{
	int resolved = 0;

	for (int index = 0; index < Get_Descriptor_Count (); index ++) {
		const BuildingModelDescriptor *descriptor = Get_Descriptor_By_Index (index);
		if ((descriptor == nullptr) || (descriptor->GameObjectPreset.Get_Length () == 0)) {
			continue;			// nothing to resolve -- a map-authored descriptor, most likely
		}

		StringClass line ("");

		DefinitionClass *preset =
			DefinitionMgrClass::Find_Named_Definition (descriptor->GameObjectPreset.Peek_Buffer (),true);

		if (preset == nullptr) {
			WWDEBUG_SAY (("BuildingDescriptorManager: descriptor '%s' names preset '%s', which no loaded "
							  "definition provides\n",descriptor->DefinitionName.Peek_Buffer (),
							  descriptor->GameObjectPreset.Peek_Buffer ()));
			if (report_out != nullptr) {
				line.Format ("%s -> %s: FAILED -- no such definition\r\n",
								 descriptor->DefinitionName.Peek_Buffer (),descriptor->GameObjectPreset.Peek_Buffer ());
				*report_out += line;
			}
			continue;
		}

		if (SuperClassID_From_ClassID (preset->Get_Class_ID ()) != CLASSID_BUILDINGS) {
			WWDEBUG_SAY (("BuildingDescriptorManager: descriptor '%s' names preset '%s', which is not a "
							  "building definition (class 0x%08X)\n",descriptor->DefinitionName.Peek_Buffer (),
							  descriptor->GameObjectPreset.Peek_Buffer (),preset->Get_Class_ID ()));
			if (report_out != nullptr) {
				line.Format ("%s -> %s: FAILED -- not a building definition\r\n",
								 descriptor->DefinitionName.Peek_Buffer (),descriptor->GameObjectPreset.Peek_Buffer ());
				*report_out += line;
			}
			continue;
		}

		//	Safe: the superclass check above is exactly the guarantee every BuildingGameObjDef
		//	subclass shares (see RuntimeBuildingFactory::Create_Constructing_Building, the same
		//	test).
		((BuildingGameObjDef *)preset)->Set_Descriptor_Name (descriptor->DefinitionName.Peek_Buffer ());
		resolved++;

		if (report_out != nullptr) {
			line.Format ("%s -> %s: OK\r\n",
							 descriptor->DefinitionName.Peek_Buffer (),descriptor->GameObjectPreset.Peek_Buffer ());
			*report_out += line;
		}
	}

	return resolved;
}
