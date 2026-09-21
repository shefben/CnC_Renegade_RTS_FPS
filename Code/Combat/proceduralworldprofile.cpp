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
 *                     $Archive:: /Commando/Code/Combat/proceduralworldprofile.cpp            $*
 *                                                                                             *
 *	V2 roadmap Section 34 Phase 1.  JSON shape/parsing mirrors buildingdescriptormanager.cpp    *
 *	exactly -- same nlohmann::json usage, same RawFileClass file-read idiom, same FAIL-macro     *
 *	validation style -- because that is the canonical v1 JSON/config loader this phase's own    *
 *	"locate in source, reuse it" instruction points at.                                          *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "proceduralworldprofile.h"

#include "rawfile.h"

#include <cmath>

#include "nlohmann/json.hpp"

using nlohmann::json;


ProceduralWorldProfile::ProceduralWorldProfile(void) :
	SchemaVersion (CURRENT_SCHEMA_VERSION),
	GeneratorVersion (CURRENT_GENERATOR_VERSION),
	Seed (1),
	Name ("")
{
}


static bool
Read_File_To_String(const char * file_path,StringClass * out)
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


bool ProceduralWorldProfileManager::Load_Profile_From_Text(const char * json_text,
																				 ProceduralWorldProfile * out_profile,
																				 StringClass * error_out)
{
	if ((json_text == nullptr) || (out_profile == nullptr)) {
		if (error_out != nullptr) *error_out = "null argument";
		return false;
	}

	json root;
	try {
		root = json::parse (json_text);

		ProceduralWorldProfile p;
		p.SchemaVersion		= root.value ("schema_version",(unsigned int)ProceduralWorldProfile::CURRENT_SCHEMA_VERSION);
		p.GeneratorVersion	= root.value ("generator_version",(unsigned int)ProceduralWorldProfile::CURRENT_GENERATOR_VERSION);
		p.Seed					= root.value ("seed",1u);
		p.Name					= root.value ("name",std::string ("")).c_str ();

		if (root.contains ("world") && root ["world"].is_object ()) {
			auto & w = root ["world"];
			p.World.Width			= w.value ("width",p.World.Width);
			p.World.Height			= w.value ("height",p.World.Height);
			p.World.ChunkSize	= w.value ("chunk_size",p.World.ChunkSize);
			p.World.SeaLevel		= w.value ("sea_level",p.World.SeaLevel);
		}

		if (root.contains ("boundary") && root ["boundary"].is_object ()) {
			auto & b = root ["boundary"];
			p.Boundary.Shape					= b.value ("shape",std::string (p.Boundary.Shape.Peek_Buffer ())).c_str ();
			p.Boundary.CliffBandWidth		= b.value ("cliff_band_width",p.Boundary.CliffBandWidth);
			p.Boundary.CliffMinHeight		= b.value ("cliff_min_height",p.Boundary.CliffMinHeight);
			p.Boundary.CliffMaxHeight		= b.value ("cliff_max_height",p.Boundary.CliffMaxHeight);
			p.Boundary.HardBarrierOffset	= b.value ("hard_barrier_offset",p.Boundary.HardBarrierOffset);
			p.Boundary.AirCeiling			= b.value ("air_ceiling",p.Boundary.AirCeiling);
		}

		if (root.contains ("terrain") && root ["terrain"].is_object ()) {
			auto & t = root ["terrain"];
			p.Terrain.MacroRelief				= t.value ("macro_relief",p.Terrain.MacroRelief);
			p.Terrain.HillDensity				= t.value ("hill_density",p.Terrain.HillDensity);
			p.Terrain.MountainDensity			= t.value ("mountain_density",p.Terrain.MountainDensity);
			p.Terrain.InteriorCliffDensity	= t.value ("interior_cliff_density",p.Terrain.InteriorCliffDensity);
			p.Terrain.ValleyDensity			= t.value ("valley_density",p.Terrain.ValleyDensity);
		}

		if (root.contains ("water") && root ["water"].is_object ()) {
			auto & w = root ["water"];
			p.Water.Rivers		= w.value ("rivers",p.Water.Rivers);
			p.Water.Lakes		= w.value ("lakes",p.Water.Lakes);
			p.Water.Ponds		= w.value ("ponds",p.Water.Ponds);
			p.Water.Coastline	= w.value ("coastline",p.Water.Coastline);
		}

		if (root.contains ("world_features") && root ["world_features"].is_object ()) {
			auto & f = root ["world_features"];
			p.Features.Forests			= f.value ("forests",p.Features.Forests);
			p.Features.Settlements		= f.value ("settlements",p.Features.Settlements);
			p.Features.Tunnels			= f.value ("tunnels",p.Features.Tunnels);
			p.Features.Highways			= f.value ("highways",p.Features.Highways);
			p.Features.TiberiumFields	= f.value ("tiberium_fields",p.Features.TiberiumFields);
		}

		if (root.contains ("retry") && root ["retry"].is_object ()) {
			auto & r = root ["retry"];
			p.Retry.MaxLocalRetriesPerStage				= r.value ("max_local_retries_per_stage",p.Retry.MaxLocalRetriesPerStage);
			p.Retry.MaxAlternateCandidatesPerFeature	= r.value ("max_alternate_candidates_per_feature",p.Retry.MaxAlternateCandidatesPerFeature);
		}

		if (root.contains ("fairness") && root ["fairness"].is_object ()) {
			auto & fa = root ["fairness"];
			p.Fairness.MaxInitialResourceTravelTimeDifferenceSeconds
				= fa.value ("max_initial_resource_travel_time_difference_seconds",p.Fairness.MaxInitialResourceTravelTimeDifferenceSeconds);
			p.Fairness.MaxProjectedInitialIncomeDifferencePercent
				= fa.value ("max_projected_initial_income_difference_percent",p.Fairness.MaxProjectedInitialIncomeDifferencePercent);
			p.Fairness.MaxBaseToFrontTravelTimeDifferenceSeconds
				= fa.value ("max_base_to_front_travel_time_difference_seconds",p.Fairness.MaxBaseToFrontTravelTimeDifferenceSeconds);
			p.Fairness.MaxBaseToBaseTravelTimeDifferenceSeconds
				= fa.value ("max_base_to_base_travel_time_difference_seconds",p.Fairness.MaxBaseToBaseTravelTimeDifferenceSeconds);
			p.Fairness.MinVehicleApproachCount			= fa.value ("min_vehicle_approach_count",p.Fairness.MinVehicleApproachCount);
			p.Fairness.MinInfantryApproachCount			= fa.value ("min_infantry_approach_count",p.Fairness.MinInfantryApproachCount);
			p.Fairness.MinBaseExpansionArea				= fa.value ("min_base_expansion_area",p.Fairness.MinBaseExpansionArea);
			p.Fairness.AllowSingleBridgeDependency		= fa.value ("allow_single_bridge_dependency",p.Fairness.AllowSingleBridgeDependency);
			p.Fairness.MaxMandatoryExposedInfantryRunDistance
				= fa.value ("max_mandatory_exposed_infantry_run_distance",p.Fairness.MaxMandatoryExposedInfantryRunDistance);
			p.Fairness.MaxMandatoryExposedInfantryRunTimeSeconds
				= fa.value ("max_mandatory_exposed_infantry_run_time_seconds",p.Fairness.MaxMandatoryExposedInfantryRunTimeSeconds);
			p.Fairness.PreferredCoverInterval			= fa.value ("preferred_cover_interval",p.Fairness.PreferredCoverInterval);
			p.Fairness.MinVehicleLaneClearance			= fa.value ("min_vehicle_lane_clearance",p.Fairness.MinVehicleLaneClearance);
		}

		*out_profile = p;

	} catch (json::exception & e) {
		if (error_out != nullptr) *error_out = e.what ();
		return false;
	}

	return true;
}


bool ProceduralWorldProfileManager::Load_Profile_From_File(const char * file_path,
																				 ProceduralWorldProfile * out_profile,
																				 StringClass * error_out)
{
	StringClass text ("");
	if (!Read_File_To_String (file_path,&text)) {
		if (error_out != nullptr) *error_out = "could not read file";
		return false;
	}
	return Load_Profile_From_Text (text.Peek_Buffer (),out_profile,error_out);
}


/***********************************************************************************************
 * Validate_Profile -- Phase 1 step 3's named checks, exactly                                    *
 *=============================================================================================*/
bool ProceduralWorldProfileManager::Validate_Profile(const ProceduralWorldProfile & p,StringClass * error_out)
{
	#define FAIL(msg) { if (error_out) *error_out = msg; return false; }
	#define NEED_FINITE(v,msg) { if (!std::isfinite (v)) FAIL (msg); }
	#define NEED_NONNEGATIVE(v,msg) { NEED_FINITE (v,msg); if ((v) < 0.0f) FAIL (msg); }

	if (p.SchemaVersion != ProceduralWorldProfile::CURRENT_SCHEMA_VERSION)		FAIL ("unsupported schema_version");

	//	Finite positive world dimensions.
	NEED_FINITE (p.World.Width,"non-finite world.width");
	NEED_FINITE (p.World.Height,"non-finite world.height");
	if (p.World.Width <= 0.0f)		FAIL ("world.width must be positive");
	if (p.World.Height <= 0.0f)	FAIL ("world.height must be positive");

	//	Valid chunk size.
	NEED_FINITE (p.World.ChunkSize,"non-finite world.chunk_size");
	if (p.World.ChunkSize <= 0.0f)							FAIL ("world.chunk_size must be positive");
	if (p.World.ChunkSize > p.World.Width)				FAIL ("world.chunk_size larger than world.width");
	if (p.World.ChunkSize > p.World.Height)				FAIL ("world.chunk_size larger than world.height");
	NEED_FINITE (p.World.SeaLevel,"non-finite world.sea_level");

	//	Cliff band smaller than the usable world -- two bands (one per side) must still leave a
	//	positive playable interior on the shorter axis.
	NEED_FINITE (p.Boundary.CliffBandWidth,"non-finite boundary.cliff_band_width");
	if (p.Boundary.CliffBandWidth <= 0.0f)					FAIL ("boundary.cliff_band_width must be positive");
	{
		float shorter_axis = (p.World.Width < p.World.Height) ? p.World.Width : p.World.Height;
		if ((p.Boundary.CliffBandWidth * 2.0f) >= shorter_axis)
																				FAIL ("boundary.cliff_band_width leaves no playable interior");
	}

	//	Valid min/max cliff heights.
	NEED_FINITE (p.Boundary.CliffMinHeight,"non-finite boundary.cliff_min_height");
	NEED_FINITE (p.Boundary.CliffMaxHeight,"non-finite boundary.cliff_max_height");
	if (p.Boundary.CliffMinHeight <= 0.0f)					FAIL ("boundary.cliff_min_height must be positive");
	if (p.Boundary.CliffMaxHeight < p.Boundary.CliffMinHeight)
																				FAIL ("boundary.cliff_max_height below cliff_min_height");

	NEED_NONNEGATIVE (p.Boundary.HardBarrierOffset,"negative/non-finite boundary.hard_barrier_offset");

	//	Air ceiling above the terrain/cliff limits (Section 1.5: aircraft stay inside the
	//	horizontal boundary and cannot simply fly over the cliffs).
	NEED_FINITE (p.Boundary.AirCeiling,"non-finite boundary.air_ceiling");
	if (p.Boundary.AirCeiling <= p.Boundary.CliffMaxHeight)
																				FAIL ("boundary.air_ceiling must clear cliff_max_height");

	//	Nonnegative densities.
	NEED_NONNEGATIVE (p.Terrain.MacroRelief,"negative/non-finite terrain.macro_relief");
	NEED_NONNEGATIVE (p.Terrain.HillDensity,"negative/non-finite terrain.hill_density");
	NEED_NONNEGATIVE (p.Terrain.MountainDensity,"negative/non-finite terrain.mountain_density");
	NEED_NONNEGATIVE (p.Terrain.InteriorCliffDensity,"negative/non-finite terrain.interior_cliff_density");
	NEED_NONNEGATIVE (p.Terrain.ValleyDensity,"negative/non-finite terrain.valley_density");

	//	Bounded retry counts.
	if (p.Retry.MaxLocalRetriesPerStage <= 0)				FAIL ("retry.max_local_retries_per_stage must be positive");
	if (p.Retry.MaxAlternateCandidatesPerFeature <= 0)	FAIL ("retry.max_alternate_candidates_per_feature must be positive");

	//	Fairness thresholds: finite, and nonnegative/positive per their documented sign.
	NEED_NONNEGATIVE (p.Fairness.MaxInitialResourceTravelTimeDifferenceSeconds,"negative/non-finite fairness.max_initial_resource_travel_time_difference_seconds");
	NEED_NONNEGATIVE (p.Fairness.MaxProjectedInitialIncomeDifferencePercent,"negative/non-finite fairness.max_projected_initial_income_difference_percent");
	NEED_NONNEGATIVE (p.Fairness.MaxBaseToFrontTravelTimeDifferenceSeconds,"negative/non-finite fairness.max_base_to_front_travel_time_difference_seconds");
	NEED_NONNEGATIVE (p.Fairness.MaxBaseToBaseTravelTimeDifferenceSeconds,"negative/non-finite fairness.max_base_to_base_travel_time_difference_seconds");
	if (p.Fairness.MinVehicleApproachCount < 1)			FAIL ("fairness.min_vehicle_approach_count must be at least 1");
	if (p.Fairness.MinInfantryApproachCount < 1)			FAIL ("fairness.min_infantry_approach_count must be at least 1");
	if (!std::isfinite (p.Fairness.MinBaseExpansionArea) || p.Fairness.MinBaseExpansionArea <= 0.0f)
																				FAIL ("fairness.min_base_expansion_area must be positive");
	NEED_NONNEGATIVE (p.Fairness.MaxMandatoryExposedInfantryRunDistance,"negative/non-finite fairness.max_mandatory_exposed_infantry_run_distance");
	NEED_NONNEGATIVE (p.Fairness.MaxMandatoryExposedInfantryRunTimeSeconds,"negative/non-finite fairness.max_mandatory_exposed_infantry_run_time_seconds");
	if (!std::isfinite (p.Fairness.PreferredCoverInterval) || p.Fairness.PreferredCoverInterval <= 0.0f)
																				FAIL ("fairness.preferred_cover_interval must be positive");
	NEED_NONNEGATIVE (p.Fairness.MinVehicleLaneClearance,"negative/non-finite fairness.min_vehicle_lane_clearance");

	#undef FAIL
	#undef NEED_FINITE
	#undef NEED_NONNEGATIVE

	return true;
}
