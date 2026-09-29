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
#include "wwmath.h"

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
			p.World.FitToLevel	= w.value ("fit_to_level",p.World.FitToLevel);	// schema 2
			p.World.CenterX		= w.value ("center_x",p.World.CenterX);
			p.World.CenterY		= w.value ("center_y",p.World.CenterY);
		}

		if (root.contains ("boundary") && root ["boundary"].is_object ()) {
			auto & b = root ["boundary"];
			p.Boundary.Shape					= b.value ("shape",std::string (p.Boundary.Shape.Peek_Buffer ())).c_str ();
			p.Boundary.CliffBandWidth		= b.value ("cliff_band_width",p.Boundary.CliffBandWidth);
			p.Boundary.CliffMinHeight		= b.value ("cliff_min_height",p.Boundary.CliffMinHeight);
			p.Boundary.CliffMaxHeight		= b.value ("cliff_max_height",p.Boundary.CliffMaxHeight);
			p.Boundary.HardBarrierOffset	= b.value ("hard_barrier_offset",p.Boundary.HardBarrierOffset);
			p.Boundary.AirCeiling			= b.value ("air_ceiling",p.Boundary.AirCeiling);

			//	Schema 2.  Absent in a schema-1 profile, which is why each default comes from
			//	the value already in p rather than from a literal repeated here.
			p.Boundary.RimRoughness				= b.value ("rim_roughness",p.Boundary.RimRoughness);
			p.Boundary.FaceRoughness			= b.value ("face_roughness",p.Boundary.FaceRoughness);
			p.Boundary.ToeWidth					= b.value ("toe_width",p.Boundary.ToeWidth);
			p.Boundary.CollisionSimplification	= b.value ("collision_simplification",p.Boundary.CollisionSimplification);
		}

		if (root.contains ("terrain") && root ["terrain"].is_object ()) {
			auto & t = root ["terrain"];
			p.Terrain.MacroRelief				= t.value ("macro_relief",p.Terrain.MacroRelief);
			p.Terrain.HillDensity				= t.value ("hill_density",p.Terrain.HillDensity);
			p.Terrain.MountainDensity			= t.value ("mountain_density",p.Terrain.MountainDensity);
			p.Terrain.InteriorCliffDensity	= t.value ("interior_cliff_density",p.Terrain.InteriorCliffDensity);
			p.Terrain.ValleyDensity			= t.value ("valley_density",p.Terrain.ValleyDensity);

			//	Schema 3.  Absent in a schema-1/2 profile, which is why each default comes from
			//	the value already in p rather than from a literal repeated here.
			p.Terrain.MinElevation					= t.value ("min_elevation",p.Terrain.MinElevation);
			p.Terrain.MaxElevation					= t.value ("max_elevation",p.Terrain.MaxElevation);
			p.Terrain.LandElevationBias			= t.value ("land_elevation_bias",p.Terrain.LandElevationBias);
			p.Terrain.MaxBuildableSlopeDegrees	= t.value ("max_buildable_slope_degrees",p.Terrain.MaxBuildableSlopeDegrees);
			p.Terrain.FeatureScale					= t.value ("feature_scale",p.Terrain.FeatureScale);
			p.Terrain.RidgeSharpness				= t.value ("ridge_sharpness",p.Terrain.RidgeSharpness);
			p.Terrain.PlateauChance					= t.value ("plateau_chance",p.Terrain.PlateauChance);
			p.Terrain.BoundaryApproachFlattening	= t.value ("boundary_approach_flattening",p.Terrain.BoundaryApproachFlattening);
		}

		if (root.contains ("water") && root ["water"].is_object ()) {
			auto & w = root ["water"];
			p.Water.Rivers		= w.value ("rivers",p.Water.Rivers);
			p.Water.Lakes		= w.value ("lakes",p.Water.Lakes);
			p.Water.Ponds		= w.value ("ponds",p.Water.Ponds);
			p.Water.Coastline	= w.value ("coastline",p.Water.Coastline);

			//	Schema 4.  Absent in a schema-1/2/3 profile, which is why each default comes from
			//	the value already in p rather than from a literal repeated here.
			p.Water.RiverWidth					= w.value ("river_width",p.Water.RiverWidth);
			p.Water.StreamWidth					= w.value ("stream_width",p.Water.StreamWidth);
			p.Water.RiverAccumulationChunks	= w.value ("river_accumulation_chunks",p.Water.RiverAccumulationChunks);
			p.Water.MaxRiverCount				= w.value ("max_river_count",p.Water.MaxRiverCount);
		}

		//	Schema 11 (D-003).  Absent in older profiles; defaults leave the phase disabled.
		if (root.contains ("plan") && root ["plan"].is_object ()) {
			auto & pl = root ["plan"];
			p.Plan.Enabled					= pl.value ("enabled",p.Plan.Enabled);
			if (pl.contains ("symmetry") && pl ["symmetry"].is_string ()) {
				p.Plan.Symmetry = pl ["symmetry"].get<std::string> ().c_str ();
			}
			p.Plan.TierCount				= pl.value ("tier_count",p.Plan.TierCount);
			p.Plan.TierStep				= pl.value ("tier_step",p.Plan.TierStep);
			p.Plan.DetailWeight			= pl.value ("detail_weight",p.Plan.DetailWeight);
			p.Plan.LaneWidthCells		= pl.value ("lane_width_cells",p.Plan.LaneWidthCells);
			p.Plan.LegacyLaneWidthCellsPresent = pl.contains ("lane_width_cells");
			p.Plan.BaseRadiusCells		= pl.value ("base_radius_cells",p.Plan.BaseRadiusCells);
			p.Plan.FlankLanes				= pl.value ("flank_lanes",p.Plan.FlankLanes);
			p.Plan.WaterFraction			= pl.value ("water_fraction",p.Plan.WaterFraction);
			p.Plan.BoundaryMarginCells	= pl.value ("boundary_margin_cells",p.Plan.BoundaryMarginCells);

			//	Schema 12 (hybrid rewrite Section 4).  Absent fields keep the Section 4 defaults.
			if (pl.contains ("archetype") && pl ["archetype"].is_string ()) {
				p.Plan.Archetype = pl ["archetype"].get<std::string> ().c_str ();
			}
			if (pl.contains ("biome_mode") && pl ["biome_mode"].is_string ()) {
				p.Plan.BiomeMode = pl ["biome_mode"].get<std::string> ().c_str ();
			}
			if (pl.contains ("biome") && pl ["biome"].is_string ()) {
				p.Plan.Biome = pl ["biome"].get<std::string> ().c_str ();
			}
			if (pl.contains ("secondary_biome") && pl ["secondary_biome"].is_string ()) {
				p.Plan.SecondaryBiome = pl ["secondary_biome"].get<std::string> ().c_str ();
			}
			p.Plan.MixedBiomeProbability	= pl.value ("mixed_biome_probability",p.Plan.MixedBiomeProbability);
			p.Plan.RoadDensity				= pl.value ("road_density",p.Plan.RoadDensity);
			p.Plan.IslandChance				= pl.value ("island_chance",p.Plan.IslandChance);
			p.Plan.RouteRedundancy			= pl.value ("route_redundancy",p.Plan.RouteRedundancy);
			p.Plan.AllowFords					= pl.value ("allow_fords",p.Plan.AllowFords);
		}

		if (root.contains ("resource") && root ["resource"].is_object ()) {
			auto & rs = root ["resource"];

			//	Schema 5.  Absent in a schema-1/2/3/4 profile, which is why each default comes
			//	from the value already in p rather than from a literal repeated here.
			p.Resource.FieldRadiusMin			= rs.value ("field_radius_min",p.Resource.FieldRadiusMin);
			p.Resource.FieldRadiusMax			= rs.value ("field_radius_max",p.Resource.FieldRadiusMax);
			p.Resource.InitialYieldMin			= rs.value ("initial_yield_min",p.Resource.InitialYieldMin);
			p.Resource.InitialYieldMax			= rs.value ("initial_yield_max",p.Resource.InitialYieldMax);
			p.Resource.CrystalDensity			= rs.value ("crystal_density",p.Resource.CrystalDensity);
			p.Resource.HarvestAnchorCount		= rs.value ("harvest_anchor_count",p.Resource.HarvestAnchorCount);
			p.Resource.HarvesterLoadAmount		= rs.value ("harvester_load_amount",p.Resource.HarvesterLoadAmount);
			p.Resource.GrowthEnabled			= rs.value ("growth_enabled",p.Resource.GrowthEnabled);
			p.Resource.GrowthRatePerSecond	= rs.value ("growth_rate_per_second",p.Resource.GrowthRatePerSecond);
		}

		if (root.contains ("world_features") && root ["world_features"].is_object ()) {
			auto & f = root ["world_features"];
			p.Features.Forests			= f.value ("forests",p.Features.Forests);
			p.Features.Settlements		= f.value ("settlements",p.Features.Settlements);
			p.Features.LegacySettlementsPresent = f.contains ("settlements");
			p.Features.Tunnels			= f.value ("tunnels",p.Features.Tunnels);
			p.Features.Highways			= f.value ("highways",p.Features.Highways);
			p.Features.TiberiumFields	= f.value ("tiberium_fields",p.Features.TiberiumFields);
		}

		if (root.contains ("retry") && root ["retry"].is_object ()) {
			auto & r = root ["retry"];
			p.Retry.MaxLocalRetriesPerStage				= r.value ("max_local_retries_per_stage",p.Retry.MaxLocalRetriesPerStage);
			p.Retry.MaxAlternateCandidatesPerFeature	= r.value ("max_alternate_candidates_per_feature",p.Retry.MaxAlternateCandidatesPerFeature);
		}

		if (root.contains ("grading") && root ["grading"].is_object ()) {
			auto & g = root ["grading"];

			//	Schema 6.  Absent in a schema-1..5 profile, which is why each default comes from
			//	the value already in p rather than from a literal repeated here.
			p.Grading.MaxCutDepth				= g.value ("max_cut_depth",p.Grading.MaxCutDepth);
			p.Grading.MaxFillHeight				= g.value ("max_fill_height",p.Grading.MaxFillHeight);
			p.Grading.MaxSlopeChangeRadians	= g.value ("max_slope_change_radians",p.Grading.MaxSlopeChangeRadians);
			p.Grading.MaxAffectedArea			= g.value ("max_affected_area",p.Grading.MaxAffectedArea);
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
			p.Fairness.MinBaseToBaseTravelTimeSeconds
				= fa.value ("min_base_to_base_travel_time_seconds",p.Fairness.MinBaseToBaseTravelTimeSeconds);
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

		if (root.contains ("logistics") && root ["logistics"].is_object ()) {
			auto & lg = root ["logistics"];

			//	Schema 7.  Absent in a schema-1..6 profile, which is why each default comes from
			//	the value already in p (Section 22.1's own inert-by-default philosophy) rather
			//	than from a literal repeated here.
			p.Logistics.Enabled						= lg.value ("enabled",p.Logistics.Enabled);
			p.Logistics.ConvoyEnabled				= lg.value ("convoy_enabled",p.Logistics.ConvoyEnabled);
			p.Logistics.ConstructionSupportRadius	= lg.value ("construction_support_radius",p.Logistics.ConstructionSupportRadius);
			p.Logistics.EnemyThreatRadius			= lg.value ("enemy_threat_radius",p.Logistics.EnemyThreatRadius);		// schema 9
		}

		if (root.contains ("weather") && root ["weather"].is_object ()) {
			auto & wt = root ["weather"];
			p.Weather.DustDensity		= wt.value ("dust_density",p.Weather.DustDensity);			// schema 9
			p.Weather.TiberiumDensity	= wt.value ("tiberium_density",p.Weather.TiberiumDensity);	// schema 9
		}

		if (root.contains ("base") && root ["base"].is_object ()) {
			auto & bs = root ["base"];

			//	Schema 9 (Section 16A.2).  Absent in a schema-1..8 profile: no initial buildings,
			//	which is what every world before this block had.
			const char * list_keys [2] = { "gdi_initial_buildings","nod_initial_buildings" };
			DynamicVectorClass<StringClass> * lists [2] = { &p.Base.GdiInitialBuildings,&p.Base.NodInitialBuildings };
			for (int k = 0; k < 2; k++) {
				if (bs.contains (list_keys [k]) && bs [list_keys [k]].is_array ()) {
					for (auto & entry : bs [list_keys [k]]) {
						if (entry.is_string ()) {
							lists [k]->Add (StringClass (entry.get<std::string> ().c_str ()));
						}
					}
				}
			}
			p.Base.SpawnCandidateCount	= bs.value ("spawn_candidate_count",p.Base.SpawnCandidateCount);
			p.Base.SpawnZoneRadius		= bs.value ("spawn_zone_radius",p.Base.SpawnZoneRadius);
		}

		if (root.contains ("vehicle_subsystems") && root ["vehicle_subsystems"].is_object ()) {
			auto & vs = root ["vehicle_subsystems"];

			//	Schema 8.  Absent in a schema-1..7 profile, which is why each default comes from
			//	the value already in p (Section 23's own "do not alter default Renegade vehicle
			//	handling unless enabled by game profile") rather than from a literal repeated here.
			p.VehicleSubsystems.Enabled							= vs.value ("enabled",p.VehicleSubsystems.Enabled);
			p.VehicleSubsystems.AllowImmobilize					= vs.value ("allow_immobilize",p.VehicleSubsystems.AllowImmobilize);
			p.VehicleSubsystems.EngineDamageStartFraction		= vs.value ("engine_damage_start_fraction",p.VehicleSubsystems.EngineDamageStartFraction);
			p.VehicleSubsystems.MobilityDamageStartFraction	= vs.value ("mobility_damage_start_fraction",p.VehicleSubsystems.MobilityDamageStartFraction);
			p.VehicleSubsystems.WeaponDamageStartFraction		= vs.value ("weapon_damage_start_fraction",p.VehicleSubsystems.WeaponDamageStartFraction);
			p.VehicleSubsystems.MinEngineFactor					= vs.value ("min_engine_factor",p.VehicleSubsystems.MinEngineFactor);
			p.VehicleSubsystems.MinMobilityFactor				= vs.value ("min_mobility_factor",p.VehicleSubsystems.MinMobilityFactor);
			p.VehicleSubsystems.MinWeaponFactor					= vs.value ("min_weapon_factor",p.VehicleSubsystems.MinWeaponFactor);
			p.VehicleSubsystems.SensorDamageStartFraction	= vs.value ("sensor_damage_start_fraction",p.VehicleSubsystems.SensorDamageStartFraction);	// schema 9
			p.VehicleSubsystems.MinSensorFactor				= vs.value ("min_sensor_factor",p.VehicleSubsystems.MinSensorFactor);	// schema 9
			p.VehicleSubsystems.ImmobilizeFraction				= vs.value ("immobilize_fraction",p.VehicleSubsystems.ImmobilizeFraction);
			p.VehicleSubsystems.WeaponDisableFraction			= vs.value ("weapon_disable_fraction",p.VehicleSubsystems.WeaponDisableFraction);
			p.VehicleSubsystems.RetreatToRepairFraction			= vs.value ("retreat_to_repair_fraction",p.VehicleSubsystems.RetreatToRepairFraction);
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

	//	See ProceduralWorldProfile's own compatibility rules: any schema up to this build's
	//	loads with defaults for what it predates; the generator must be this build's own.
	if ((p.SchemaVersion == 0) || (p.SchemaVersion > ProceduralWorldProfile::CURRENT_SCHEMA_VERSION))
		FAIL ("unsupported schema_version (newer than this build)");
	if (p.GeneratorVersion != ProceduralWorldProfile::CURRENT_GENERATOR_VERSION)
		FAIL ("generator_version is not this build's generator -- the world it names cannot be reproduced");

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
	NEED_FINITE (p.World.CenterX,"non-finite world.center_x");
	NEED_FINITE (p.World.CenterY,"non-finite world.center_y");

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

	//	Section 5.5's cliff shaping controls (schema 2).  The roughness weights are normalized,
	//	so unlike the density weights they have a real upper bound to enforce: a value above 1
	//	is not "more variation", it is a profile author misunderstanding the unit, and silently
	//	clamping it would hide that.
	NEED_NONNEGATIVE (p.Boundary.RimRoughness,"negative/non-finite boundary.rim_roughness");
	NEED_NONNEGATIVE (p.Boundary.FaceRoughness,"negative/non-finite boundary.face_roughness");
	if (p.Boundary.RimRoughness > 1.0f)						FAIL ("boundary.rim_roughness above 1");
	if (p.Boundary.FaceRoughness > 1.0f)					FAIL ("boundary.face_roughness above 1");

	//	The toe is a skirt at the base of the face and lives inside the cliff band, so a toe
	//	wider than the band would put scree in the playable interior -- which Section 5.9
	//	forbids just as firmly as it forbids building in the band.
	NEED_NONNEGATIVE (p.Boundary.ToeWidth,"negative/non-finite boundary.toe_width");
	if (p.Boundary.ToeWidth >= p.Boundary.CliffBandWidth)
																				FAIL ("boundary.toe_width must be narrower than cliff_band_width");

	if (p.Boundary.CollisionSimplification < 1)			FAIL ("boundary.collision_simplification must be at least 1");

	//	Nonnegative densities.
	NEED_NONNEGATIVE (p.Terrain.MacroRelief,"negative/non-finite terrain.macro_relief");
	NEED_NONNEGATIVE (p.Terrain.HillDensity,"negative/non-finite terrain.hill_density");
	NEED_NONNEGATIVE (p.Terrain.MountainDensity,"negative/non-finite terrain.mountain_density");
	NEED_NONNEGATIVE (p.Terrain.InteriorCliffDensity,"negative/non-finite terrain.interior_cliff_density");
	NEED_NONNEGATIVE (p.Terrain.ValleyDensity,"negative/non-finite terrain.valley_density");

	//	Schema 3's macro terrain fields (Phase 4).
	NEED_FINITE (p.Terrain.MinElevation,"non-finite terrain.min_elevation");
	NEED_FINITE (p.Terrain.MaxElevation,"non-finite terrain.max_elevation");
	NEED_FINITE (p.Terrain.LandElevationBias,"non-finite terrain.land_elevation_bias");
	if (p.Terrain.MaxElevation < p.Terrain.MinElevation)	FAIL ("terrain.max_elevation below min_elevation");

	//	Section 1.5's aircraft rule extends naturally to generated ground: nothing the terrain
	//	generator can produce may reach as high as where aircraft are allowed to fly.
	if (p.Terrain.MaxElevation >= p.Boundary.AirCeiling)	FAIL ("terrain.max_elevation must clear boundary.air_ceiling");

	NEED_FINITE (p.Terrain.MaxBuildableSlopeDegrees,"non-finite terrain.max_buildable_slope_degrees");
	if ((p.Terrain.MaxBuildableSlopeDegrees <= 0.0f) || (p.Terrain.MaxBuildableSlopeDegrees >= 90.0f))
																				FAIL ("terrain.max_buildable_slope_degrees must be between 0 and 90");

	NEED_FINITE (p.Terrain.FeatureScale,"non-finite terrain.feature_scale");
	if (p.Terrain.FeatureScale <= 0.0f)						FAIL ("terrain.feature_scale must be positive");

	//	Normalized weights, same rule as the boundary block's roughness fields: a value above 1
	//	is a profile author misunderstanding the unit, not "more variation" to silently clamp.
	NEED_NONNEGATIVE (p.Terrain.RidgeSharpness,"negative/non-finite terrain.ridge_sharpness");
	NEED_NONNEGATIVE (p.Terrain.PlateauChance,"negative/non-finite terrain.plateau_chance");
	NEED_NONNEGATIVE (p.Terrain.BoundaryApproachFlattening,"negative/non-finite terrain.boundary_approach_flattening");
	if (p.Terrain.RidgeSharpness > 1.0f)						FAIL ("terrain.ridge_sharpness above 1");
	if (p.Terrain.PlateauChance > 1.0f)						FAIL ("terrain.plateau_chance above 1");
	if (p.Terrain.BoundaryApproachFlattening > 1.0f)		FAIL ("terrain.boundary_approach_flattening above 1");

	//	Schema 4's water fields (Phase 6).
	NEED_FINITE (p.Water.RiverWidth,"non-finite water.river_width");
	NEED_FINITE (p.Water.StreamWidth,"non-finite water.stream_width");
	if (p.Water.RiverWidth <= 0.0f)							FAIL ("water.river_width must be positive");
	if (p.Water.StreamWidth <= 0.0f)						FAIL ("water.stream_width must be positive");
	if (p.Water.StreamWidth > p.Water.RiverWidth)			FAIL ("water.stream_width must not exceed water.river_width");
	NEED_FINITE (p.Water.RiverAccumulationChunks,"non-finite water.river_accumulation_chunks");
	if (p.Water.RiverAccumulationChunks <= 0.0f)			FAIL ("water.river_accumulation_chunks must be positive");
	if (p.Water.MaxRiverCount <= 0)							FAIL ("water.max_river_count must be positive");

	//	Schema 11 (D-003) plan block.  Ranges mirror MapPlanGenerator::Generate's own checks so a
	//	bad profile is rejected at load, not at the first world attempt.
	if (p.Plan.Enabled) {
		if ((p.Plan.Symmetry != "none") && (p.Plan.Symmetry != "rotational") && (p.Plan.Symmetry != "mirror_x"))
																		FAIL ("plan.symmetry must be none, rotational or mirror_x");
		if ((p.Plan.TierCount < 2) || (p.Plan.TierCount > 5))	FAIL ("plan.tier_count must be 2..5");
		NEED_FINITE (p.Plan.TierStep,"non-finite plan.tier_step");
		if (p.Plan.TierStep <= 0.0f)							FAIL ("plan.tier_step must be positive");
		NEED_FINITE (p.Plan.DetailWeight,"non-finite plan.detail_weight");
		if ((p.Plan.DetailWeight < 0.0f) || (p.Plan.DetailWeight > 1.0f))	FAIL ("plan.detail_weight must be 0..1");
		if (p.Plan.BaseRadiusCells < 1)						FAIL ("plan.base_radius_cells must be >= 1");
		if ((p.Plan.FlankLanes < 0) || (p.Plan.FlankLanes > 2))	FAIL ("plan.flank_lanes must be 0..2");
		NEED_FINITE (p.Plan.WaterFraction,"non-finite plan.water_fraction");
		NEED_NONNEGATIVE (p.Plan.BoundaryMarginCells,"negative/non-finite plan.boundary_margin_cells");

		//	lane_width_cells is legacy (Section 0.7) but the D-003 planner still reads it until the
		//	hybrid corridor phases replace that planner, so its range stays enforced.
		if (p.Plan.LaneWidthCells < 1)						FAIL ("plan.lane_width_cells must be >= 1");

		//	Section 4 -- the hybrid path's water range.  Only a fitted world keeps the legacy range;
		//	it skips the phase anyway.
		if (p.Uses_Hybrid_Generator ()) {
			if ((p.Plan.WaterFraction < 0.03f) || (p.Plan.WaterFraction > 0.20f))	FAIL ("plan.water_fraction must be 0.03..0.20");
		} else {
			if ((p.Plan.WaterFraction < 0.0f) || (p.Plan.WaterFraction > 1.0f))	FAIL ("plan.water_fraction must be 0..1");
		}

		HybridArchetype archetype;
		HybridBiomeMode biome_mode;
		HybridBiome biome;
		HybridBiome secondary_biome;
		if (!Hybrid_Archetype_From_Name (p.Plan.Archetype,&archetype))
																		FAIL ("plan.archetype must be auto or a Section 6 archetype");
		if (!Hybrid_Biome_Mode_From_Name (p.Plan.BiomeMode,&biome_mode))
																		FAIL ("plan.biome_mode must be auto, single or mixed");
		if (!Hybrid_Biome_From_Name (p.Plan.Biome,&biome))
																		FAIL ("plan.biome must be auto, temperate, desert, snow or tiberium_wasteland");
		if (!Hybrid_Biome_From_Name (p.Plan.SecondaryBiome,&secondary_biome))
																		FAIL ("plan.secondary_biome must be auto, temperate, desert, snow or tiberium_wasteland");
		if ((biome != HYBRID_BIOME_AUTO) && (secondary_biome != HYBRID_BIOME_AUTO)
		 && !Hybrid_Is_Legal_Biome_Pair (biome,secondary_biome))
																		FAIL ("plan.biome and plan.secondary_biome are not a legal mixed pair");
		NEED_FINITE (p.Plan.MixedBiomeProbability,"non-finite plan.mixed_biome_probability");
		if ((p.Plan.MixedBiomeProbability < 0.0f) || (p.Plan.MixedBiomeProbability > 1.0f))
																		FAIL ("plan.mixed_biome_probability must be 0..1");
		NEED_FINITE (p.Plan.RoadDensity,"non-finite plan.road_density");
		if ((p.Plan.RoadDensity < 0.0f) || (p.Plan.RoadDensity > 1.0f))	FAIL ("plan.road_density must be 0..1");
		NEED_FINITE (p.Plan.IslandChance,"non-finite plan.island_chance");
		if ((p.Plan.IslandChance < 0.0f) || (p.Plan.IslandChance > 1.0f))	FAIL ("plan.island_chance must be 0..1");
		if ((p.Plan.RouteRedundancy < 2) || (p.Plan.RouteRedundancy > 3))	FAIL ("plan.route_redundancy must be 2..3");
		if (p.Plan.Enabled && p.World.FitToLevel) {
			//	Not an error: a fitted world simply skips the phase.  Logged by the manager.
		}
	}

	//	Schema 5's resource fields (Phase 12).
	NEED_FINITE (p.Resource.FieldRadiusMin,"non-finite resource.field_radius_min");
	NEED_FINITE (p.Resource.FieldRadiusMax,"non-finite resource.field_radius_max");
	if (p.Resource.FieldRadiusMin <= 0.0f)					FAIL ("resource.field_radius_min must be positive");
	if (p.Resource.FieldRadiusMax < p.Resource.FieldRadiusMin)
																			FAIL ("resource.field_radius_max must not be less than resource.field_radius_min");
	NEED_NONNEGATIVE (p.Resource.InitialYieldMin,"negative/non-finite resource.initial_yield_min");
	NEED_NONNEGATIVE (p.Resource.InitialYieldMax,"negative/non-finite resource.initial_yield_max");
	if (p.Resource.InitialYieldMax < p.Resource.InitialYieldMin)
																			FAIL ("resource.initial_yield_max must not be less than resource.initial_yield_min");
	NEED_NONNEGATIVE (p.Resource.CrystalDensity,"negative/non-finite resource.crystal_density");
	if (p.Resource.CrystalDensity > 1.0f)					FAIL ("resource.crystal_density above 1");
	if (p.Resource.HarvestAnchorCount <= 0)					FAIL ("resource.harvest_anchor_count must be positive");
	NEED_NONNEGATIVE (p.Resource.HarvesterLoadAmount,"negative/non-finite resource.harvester_load_amount");
	NEED_NONNEGATIVE (p.Resource.GrowthRatePerSecond,"negative/non-finite resource.growth_rate_per_second");

	//	Bounded retry counts.
	if (p.Retry.MaxLocalRetriesPerStage <= 0)				FAIL ("retry.max_local_retries_per_stage must be positive");
	if (p.Retry.MaxAlternateCandidatesPerFeature <= 0)	FAIL ("retry.max_alternate_candidates_per_feature must be positive");

	//	Fairness thresholds: finite, and nonnegative/positive per their documented sign.
	NEED_NONNEGATIVE (p.Fairness.MaxInitialResourceTravelTimeDifferenceSeconds,"negative/non-finite fairness.max_initial_resource_travel_time_difference_seconds");
	NEED_NONNEGATIVE (p.Fairness.MaxProjectedInitialIncomeDifferencePercent,"negative/non-finite fairness.max_projected_initial_income_difference_percent");
	NEED_NONNEGATIVE (p.Fairness.MaxBaseToFrontTravelTimeDifferenceSeconds,"negative/non-finite fairness.max_base_to_front_travel_time_difference_seconds");
	NEED_NONNEGATIVE (p.Fairness.MaxBaseToBaseTravelTimeDifferenceSeconds,"negative/non-finite fairness.max_base_to_base_travel_time_difference_seconds");
	NEED_NONNEGATIVE (p.Fairness.MinBaseToBaseTravelTimeSeconds,"negative/non-finite fairness.min_base_to_base_travel_time_seconds");
	if (p.Fairness.MinBaseToBaseTravelTimeSeconds > p.Fairness.MaxBaseToBaseTravelTimeDifferenceSeconds)
		FAIL ("fairness.min_base_to_base_travel_time_seconds exceeds maximum");
	if (p.Fairness.MinVehicleApproachCount < 1)			FAIL ("fairness.min_vehicle_approach_count must be at least 1");
	if (p.Fairness.MinInfantryApproachCount < 1)			FAIL ("fairness.min_infantry_approach_count must be at least 1");
	if (!std::isfinite (p.Fairness.MinBaseExpansionArea) || p.Fairness.MinBaseExpansionArea <= 0.0f)
																				FAIL ("fairness.min_base_expansion_area must be positive");
	NEED_NONNEGATIVE (p.Fairness.MaxMandatoryExposedInfantryRunDistance,"negative/non-finite fairness.max_mandatory_exposed_infantry_run_distance");
	NEED_NONNEGATIVE (p.Fairness.MaxMandatoryExposedInfantryRunTimeSeconds,"negative/non-finite fairness.max_mandatory_exposed_infantry_run_time_seconds");
	if (!std::isfinite (p.Fairness.PreferredCoverInterval) || p.Fairness.PreferredCoverInterval <= 0.0f)
																				FAIL ("fairness.preferred_cover_interval must be positive");
	NEED_NONNEGATIVE (p.Fairness.MinVehicleLaneClearance,"negative/non-finite fairness.min_vehicle_lane_clearance");

	//	Grading limits.  All four strictly positive: a zero limit does not mean "no grading", it
	//	means every grading including a no-op one is refused, which is a broken profile rather
	//	than a strict one.  A profile that wants no grading turns the Commander off, not this.
	if (!std::isfinite (p.Grading.MaxCutDepth) || p.Grading.MaxCutDepth <= 0.0f)
																				FAIL ("grading.max_cut_depth must be positive");
	if (!std::isfinite (p.Grading.MaxFillHeight) || p.Grading.MaxFillHeight <= 0.0f)
																				FAIL ("grading.max_fill_height must be positive");
	if (!std::isfinite (p.Grading.MaxSlopeChangeRadians) || p.Grading.MaxSlopeChangeRadians <= 0.0f)
																				FAIL ("grading.max_slope_change_radians must be positive");
	if (p.Grading.MaxSlopeChangeRadians >= (WWMATH_PI * 0.5f))
																				FAIL ("grading.max_slope_change_radians must be below a vertical face");
	if (!std::isfinite (p.Grading.MaxAffectedArea) || p.Grading.MaxAffectedArea <= 0.0f)
																				FAIL ("grading.max_affected_area must be positive");

	//	Logistics toggles (schema 7).  ConstructionSupportRadius may legally be 0 -- that is the
	//	documented "no bonus" default, not an error -- so this only rejects negative/non-finite,
	//	the same treatment every other optional-bonus field in this file gets.
	NEED_NONNEGATIVE (p.Logistics.ConstructionSupportRadius,"negative/non-finite logistics.construction_support_radius");
	NEED_NONNEGATIVE (p.Logistics.EnemyThreatRadius,"negative/non-finite logistics.enemy_threat_radius");

	//	Base bootstrap (schema 9).  A pool of zero spawn candidates is legal -- the fallback zone
	//	is what Section 16A.5 requires regardless -- but a negative one is not.
	NEED_NONNEGATIVE (p.Base.SpawnZoneRadius,"negative/non-finite base.spawn_zone_radius");
	NEED_NONNEGATIVE (p.Weather.DustDensity,"negative/non-finite weather.dust_density");
	NEED_NONNEGATIVE (p.Weather.TiberiumDensity,"negative/non-finite weather.tiberium_density");
	if ((p.Base.SpawnCandidateCount < 0) || (p.Base.SpawnCandidateCount > 64)) {
		if (error_out) *error_out = "base.spawn_candidate_count outside 0..64";
		return false;
	}

	//	Vehicle subsystem fractions (schema 8).  All seven are normalized 0..1 health
	//	fractions/factors -- the same "a value above 1 is a profile author misunderstanding the
	//	unit, not more of something to silently clamp" rule the boundary/terrain roughness
	//	weights already get in this file.
	NEED_NONNEGATIVE (p.VehicleSubsystems.EngineDamageStartFraction,"negative/non-finite vehicle_subsystems.engine_damage_start_fraction");
	NEED_NONNEGATIVE (p.VehicleSubsystems.MobilityDamageStartFraction,"negative/non-finite vehicle_subsystems.mobility_damage_start_fraction");
	NEED_NONNEGATIVE (p.VehicleSubsystems.WeaponDamageStartFraction,"negative/non-finite vehicle_subsystems.weapon_damage_start_fraction");
	NEED_NONNEGATIVE (p.VehicleSubsystems.MinEngineFactor,"negative/non-finite vehicle_subsystems.min_engine_factor");
	NEED_NONNEGATIVE (p.VehicleSubsystems.MinMobilityFactor,"negative/non-finite vehicle_subsystems.min_mobility_factor");
	NEED_NONNEGATIVE (p.VehicleSubsystems.MinWeaponFactor,"negative/non-finite vehicle_subsystems.min_weapon_factor");
	NEED_NONNEGATIVE (p.VehicleSubsystems.SensorDamageStartFraction,"negative/non-finite vehicle_subsystems.sensor_damage_start_fraction");
	NEED_NONNEGATIVE (p.VehicleSubsystems.MinSensorFactor,"negative/non-finite vehicle_subsystems.min_sensor_factor");
	if (p.VehicleSubsystems.MinSensorFactor > 1.0f) FAIL ("vehicle_subsystems.min_sensor_factor above 1");
	NEED_NONNEGATIVE (p.VehicleSubsystems.ImmobilizeFraction,"negative/non-finite vehicle_subsystems.immobilize_fraction");
	NEED_NONNEGATIVE (p.VehicleSubsystems.WeaponDisableFraction,"negative/non-finite vehicle_subsystems.weapon_disable_fraction");
	NEED_NONNEGATIVE (p.VehicleSubsystems.RetreatToRepairFraction,"negative/non-finite vehicle_subsystems.retreat_to_repair_fraction");
	if (p.VehicleSubsystems.EngineDamageStartFraction > 1.0f)		FAIL ("vehicle_subsystems.engine_damage_start_fraction above 1");
	if (p.VehicleSubsystems.MobilityDamageStartFraction > 1.0f)	FAIL ("vehicle_subsystems.mobility_damage_start_fraction above 1");
	if (p.VehicleSubsystems.WeaponDamageStartFraction > 1.0f)		FAIL ("vehicle_subsystems.weapon_damage_start_fraction above 1");
	if (p.VehicleSubsystems.MinEngineFactor > 1.0f)					FAIL ("vehicle_subsystems.min_engine_factor above 1");
	if (p.VehicleSubsystems.MinMobilityFactor > 1.0f)				FAIL ("vehicle_subsystems.min_mobility_factor above 1");
	if (p.VehicleSubsystems.MinWeaponFactor > 1.0f)					FAIL ("vehicle_subsystems.min_weapon_factor above 1");
	if (p.VehicleSubsystems.ImmobilizeFraction > 1.0f)				FAIL ("vehicle_subsystems.immobilize_fraction above 1");
	if (p.VehicleSubsystems.WeaponDisableFraction > 1.0f)			FAIL ("vehicle_subsystems.weapon_disable_fraction above 1");
	if (p.VehicleSubsystems.RetreatToRepairFraction > 1.0f)			FAIL ("vehicle_subsystems.retreat_to_repair_fraction above 1");

	#undef FAIL
	#undef NEED_FINITE
	#undef NEED_NONNEGATIVE

	return true;
}
