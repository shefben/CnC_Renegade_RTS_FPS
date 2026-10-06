/*
**	OpenW3D
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
 ***                            Confidential - Westwood Studios                              ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : OpenW3D                                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/hybridpreflight.cpp                   $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#include "hybridpreflight.h"
#include "proceduralworldprofile.h"
#include "worldmetrics.h"
#include "refinerygameobj.h"
#include "physicalgameobj.h"
#include "buildingdescriptor.h"
#include "buildingdescriptormanager.h"
#include "definitionmgr.h"
#include "definitionclassids.h"
#include "phys.h"
#include "bridgesystem.h"
#include "bridgesection.h"
#include "bridgeplanner.h"
#include "terraintexturesystem.h"
#include "terrainmaterial.h"
#include "foliagesystem.h"
#include "w3d_file.h"
#include "wwdebug.h"

#include <stdio.h>
#include <string.h>


/*
**	The Humvee is found by its model, the same V_GDI_HUMVEE whose WORLDBOX the legacy road graph's
**	width was measured from.  Decoration and lamp types are the foliage types
**	BiomeDecorationGenerator places.
*/
static const char * const HUMVEE_MODEL_NAME = "v_gdi_humvee";
static const char * const DECORATION_TYPE_NAMES[] = { "ow_tree_conifer", "ow_tree_dead", "ow_bush_scrub", "ow_rock_boulder" };
static const char * const LAMP_TYPE_NAME = "ow_street_lamp";


const char * Hybrid_Preflight_Failure_Name(HybridPreflightFailure failure)
{
	switch (failure) {
		case HYBRID_PREFLIGHT_OK:											return "OK";
		case HYBRID_PREFLIGHT_REQUIRED_FEATURE_DISABLED:			return "HYBRID_REQUIRED_FEATURE_DISABLED";
		case HYBRID_PREFLIGHT_WORLD_SIZE_LIMIT_EXCEEDED:			return "WORLD_SIZE_LIMIT_EXCEEDED";
		case HYBRID_PREFLIGHT_MISSING_HUMVEE_COLLISION_METRIC:	return "MISSING_HUMVEE_COLLISION_METRIC";
		case HYBRID_PREFLIGHT_MISSING_HEAVY_VEHICLE_COLLISION_METRIC:	return "MISSING_HEAVY_VEHICLE_COLLISION_METRIC";
		case HYBRID_PREFLIGHT_MISSING_HARVESTER_COLLISION_METRIC:	return "MISSING_HARVESTER_COLLISION_METRIC";
		case HYBRID_PREFLIGHT_MISSING_WORLD_UNITS_PER_METER:		return "MISSING_WORLD_UNITS_PER_METER";
		case HYBRID_PREFLIGHT_NO_HEAVY_CAPABLE_BRIDGE:				return "NO_HEAVY_CAPABLE_BRIDGE";
		case HYBRID_PREFLIGHT_FORD_CAPABILITY_UNAVAILABLE:			return "FORD_CAPABILITY_UNAVAILABLE";
		case HYBRID_PREFLIGHT_MISSING_STARTING_BUILDING_FOOTPRINT:	return "MISSING_STARTING_BUILDING_FOOTPRINT";
		case HYBRID_PREFLIGHT_MISSING_FAIRNESS_REFERENCE_SPEED:	return "MISSING_FAIRNESS_REFERENCE_SPEED";
		case HYBRID_PREFLIGHT_MISSING_HARVESTER_LOAD_AMOUNT:		return "MISSING_HARVESTER_LOAD_AMOUNT";
		case HYBRID_PREFLIGHT_UNSUPPORTED_BIOME_ASSET_SET:			return "UNSUPPORTED_BIOME_ASSET_SET";
		default:																	return "UNKNOWN";
	}
}


const char * Hybrid_Material_Slot_Name(HybridBiome biome,HybridMaterialSlot slot)
{
	if (slot == HYBRID_SLOT_EXPOSED_ROCK) {
		return "exposed_rock";
	}
	switch (biome) {
		case HYBRID_BIOME_TEMPERATE:				return "grass_or_soil";
		case HYBRID_BIOME_DESERT:					return "sand_or_dry_soil";
		case HYBRID_BIOME_SNOW:						return "snow_ground";
		case HYBRID_BIOME_TIBERIUM_WASTELAND:	return "barren_or_cracked_ground";
		default:											return "unknown";
	}
}


/*
**	Section 4.2 slot resolution against one terrain layer.  A layer can fill a biome's ground slot
**	only from a ground-covering kind (BASE, MACRO or BIOME -- never shoreline, road, cliff, detail
**	or scorch), and exposed_rock only from BASE, CLIFF or BIOME.  The surface type decides the
**	match; name keywords cover the slots no surface type expresses (dry soil, barren ground).
*/
static bool Name_Has(const StringClass & name,const char * keyword)
{
	char lowered[256];
	const char * source = name.Peek_Buffer();
	int i = 0;
	for (; source[i] != 0 && i < (int)sizeof(lowered) - 1; ++i) {
		const char c = source[i];
		lowered[i] = (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
	}
	lowered[i] = 0;
	return ::strstr(lowered,keyword) != NULL;
}


static bool Layer_Fills_Slot(const HybridTerrainLayerInput & layer,HybridBiome biome,HybridMaterialSlot slot)
{
	if (slot == HYBRID_SLOT_EXPOSED_ROCK) {
		const bool kind_ok = (layer.Kind == TERRAIN_LAYER_BASE) || (layer.Kind == TERRAIN_LAYER_CLIFF) || (layer.Kind == TERRAIN_LAYER_BIOME);
		return kind_ok && (layer.SurfaceType == SURFACE_TYPE_ROCK);
	}

	const bool kind_ok = (layer.Kind == TERRAIN_LAYER_BASE) || (layer.Kind == TERRAIN_LAYER_MACRO) || (layer.Kind == TERRAIN_LAYER_BIOME);
	if (!kind_ok) {
		return false;
	}
	switch (biome) {
		case HYBRID_BIOME_TEMPERATE:
			return (layer.SurfaceType == SURFACE_TYPE_GRASS) || (layer.SurfaceType == SURFACE_TYPE_DIRT);
		case HYBRID_BIOME_DESERT:
			return (layer.SurfaceType == SURFACE_TYPE_SAND) ||
					 ((layer.SurfaceType == SURFACE_TYPE_DIRT) && (Name_Has(layer.Name,"desert") || Name_Has(layer.Name,"dry")));
		case HYBRID_BIOME_SNOW:
			return (layer.SurfaceType == SURFACE_TYPE_SNOW);
		case HYBRID_BIOME_TIBERIUM_WASTELAND:
			return (layer.SurfaceType != SURFACE_TYPE_TIBERIUM_FIELD) &&
					 (Name_Has(layer.Name,"barren") || Name_Has(layer.Name,"cracked") || Name_Has(layer.Name,"wasteland"));
		default:
			return false;
	}
}


HybridPreflightInputs::HybridPreflightInputs(void) :
	WorldUnitsPerMeter(0.0f),
	ReferenceVehicleSpeed(0.0f),
	ReferenceInfantrySpeed(0.0f),
	FordCapabilityAvailable(false),
	BridgeMaxSpanLength(0.0f),
	BridgeWaterClearanceMargin(0.0f),
	BridgeApproachLength(0.0f),
	DecorationTypesAvailable(false),
	LampTypeAvailable(false),
	AllowOversizeDevelopment(false)
{
}


static bool Model_Base_Name_Is(const StringClass & model_name,const char * wanted)
{
	/*
	** Compare the file's base name, case-insensitively, ignoring any directory and extension.
	*/
	const char * name = model_name.Peek_Buffer();
	const char * slash = ::strrchr(name,'\\');
	const char * fslash = ::strrchr(name,'/');
	if (fslash != NULL && (slash == NULL || fslash > slash)) {
		slash = fslash;
	}
	const char * base = (slash != NULL) ? slash + 1 : name;
	const char * dot = ::strrchr(base,'.');
	const int length = (dot != NULL) ? (int)(dot - base) : (int)::strlen(base);
	if (length != (int)::strlen(wanted)) {
		return false;
	}
	for (int i = 0; i < length; ++i) {
		char c = base[i];
		c = (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
		if (c != wanted[i]) {
			return false;
		}
	}
	return true;
}


static void Set_Vehicle_Metric(HybridVehicleMetricInput * out,const WorldCollisionBoxStruct & box)
{
	*out = HybridVehicleMetricInput();
	if (box.MeasuredCount <= 0) {
		return;
	}
	out->Measured = true;
	out->Length = box.Length;
	out->Width = box.Width;
	out->Height = box.Height;
	out->DefinitionID = box.DefinitionID;
}


/*
**	A team's Harvester is the one its starting refinery builds: the starting-building descriptor's
**	game object preset, when that preset is a refinery definition.  The first such refinery in
**	the profile's order answers; a team with none keeps an unmeasured metric.
*/
static void Gather_Team_Harvester(const DynamicVectorClass<StringClass> & names,HybridVehicleMetricInput * out)
{
	*out = HybridVehicleMetricInput();
	for (int i = 0; i < names.Count(); ++i) {
		const BuildingModelDescriptor * descriptor = BuildingDescriptorManager::Get_Descriptor(names[i].Peek_Buffer());
		if ((descriptor == NULL) || descriptor->GameObjectPreset.Is_Empty()) {
			continue;
		}
		const DefinitionClass * def = DefinitionMgrClass::Find_Named_Definition(descriptor->GameObjectPreset.Peek_Buffer(),false);
		if ((def == NULL) || (def->Get_Class_ID() != CLASSID_GAME_OBJECT_DEF_REFINERY)) {
			continue;
		}
		const RefineryGameObjDef * refinery = (const RefineryGameObjDef *)def;
		const PhysicalGameObjDef * harvester = (const PhysicalGameObjDef *)DefinitionMgrClass::Find_Definition(refinery->Get_Harvester_Def_ID(),false);
		if (harvester == NULL) {
			continue;
		}
		WorldCollisionBoxStruct box;
		WorldMetricsClass::Get_Collision_Box(harvester->Get_Phys_Def_ID(),&box);
		Set_Vehicle_Metric(out,box);
		return;
	}
}


void HybridPreflightInputs::Gather_From_Runtime(const ProceduralWorldProfile & profile)
{
	WorldUnitsPerMeter = WorldMetricsClass::Get_World_Units_Per_Meter();
	ReferenceVehicleSpeed = WorldMetricsClass::Get_Reference_Vehicle_Speed();
	ReferenceInfantrySpeed = WorldMetricsClass::Get_Reference_Infantry_Speed();

	/*
	** Humvee: the lowest-id physics definition whose model is V_GDI_HUMVEE and whose WORLDBOX
	** resolved.  A Humvee known only by its render bounds is no answer.
	*/
	Humvee = HybridVehicleMetricInput();
	for (DefinitionClass * def = DefinitionMgrClass::Get_First(CLASSID_PHYSICS,DefinitionMgrClass::ID_SUPERCLASS);
		  def != NULL;
		  def = DefinitionMgrClass::Get_Next(def,CLASSID_PHYSICS,DefinitionMgrClass::ID_SUPERCLASS))
	{
		PhysDefClass * phys_def = (PhysDefClass *)def;
		if (!Model_Base_Name_Is(phys_def->Get_Model_Name(),HUMVEE_MODEL_NAME)) {
			continue;
		}
		if (Humvee.Measured && (int)def->Get_ID() >= Humvee.DefinitionID) {
			continue;
		}
		WorldCollisionBoxStruct box;
		if (WorldMetricsClass::Get_Collision_Box((int)def->Get_ID(),&box)) {
			Set_Vehicle_Metric(&Humvee,box);
		}
	}

	/*
	** Heavy vehicle: WorldMetricsClass's heavy-ground aggregate -- the per-axis largest measured
	** tracked vehicle, lateral width rather than the circumscribing diameter.
	*/
	WorldCollisionBoxStruct heavy_box;
	WorldMetricsClass::Get_Class_Collision_Box(LOCOMOTION_TRACKED,&heavy_box);
	Set_Vehicle_Metric(&HeavyVehicle,heavy_box);
	if (heavy_box.UnmeasuredCount > 0) {
		WWDEBUG_SAY(("Hybrid preflight: %d tracked definition(s) have no WORLDBOX and are left out of the heavy envelope\n",heavy_box.UnmeasuredCount));
	}

	/*
	** Harvesters: both teams', from their starting refineries.
	*/
	Gather_Team_Harvester(profile.Base.GdiInitialBuildings,&Harvesters[0]);
	Gather_Team_Harvester(profile.Base.NodInitialBuildings,&Harvesters[1]);

	/*
	** Bridges and the planner's span / clearance limits.  StructuralModels names only the models
	** that collide or carry traffic; dressing is render-only.
	*/
	Bridges.Reset_Active();
	for (int i = 0; i < BridgeSystem::Get_Definition_Count(); ++i) {
		const BridgeDefinitionClass * def = BridgeSystem::Peek_Definition(i);
		if (def == NULL) {
			continue;
		}
		HybridBridgeInput bridge;
		bridge.Name = def->Get_Name();
		bridge.Width = def->Get_Width();
		bridge.DeckThickness = def->Get_Deck_Thickness();
		bridge.RequiredClearance = def->Get_Required_Clearance();
		bridge.SupportSpacing = def->Get_Support_Spacing();
		bridge.SupportClearance = def->Get_Support_Clearance();
		bridge.StartSeat = def->Get_Section_Length(BRIDGE_SECTION_START_CAP);
		bridge.EndSeat = def->Get_Section_Length(BRIDGE_SECTION_END_CAP);
		for (int kind = 0; kind < BRIDGE_SECTION_KIND_COUNT; ++kind) {
			for (int state = 0; state < BRIDGE_STATE_COUNT; ++state) {
				if (!def->Does_Collide(kind,state) && !def->Is_Traversable(kind,state)) {
					continue;
				}
				const char * model = def->Get_Model(kind,state);
				if ((model == NULL) || (model[0] == 0)) {
					continue;
				}
				if (!bridge.StructuralModels.Is_Empty()) {
					bridge.StructuralModels += "|";
				}
				bridge.StructuralModels += model;
			}
		}
		Bridges.Add(bridge);
	}
	BridgePlannerParams bridge_params;
	BridgeMaxSpanLength = bridge_params.MaxSpanLength;
	BridgeWaterClearanceMargin = bridge_params.WaterClearanceMargin;
	BridgeApproachLength = 0.0f;

	FordCapabilityAvailable = false;

	/*
	** Starting-building footprints, GDI then Nod in profile order.
	*/
	Buildings.Reset_Active();
	for (int team = 0; team < 2; ++team) {
		const DynamicVectorClass<StringClass> & names = (team == 0) ? profile.Base.GdiInitialBuildings : profile.Base.NodInitialBuildings;
		for (int i = 0; i < names.Count(); ++i) {
			HybridBuildingFootprintInput building;
			building.Name = names[i];
			building.Team = team;
			building.Radius = 0.0f;
			building.Height = 0.0f;
			building.Found = WorldMetricsClass::Get_Building_Footprint(names[i].Peek_Buffer(),&building.Radius,&building.Height);
			Buildings.Add(building);
		}
	}

	/*
	** Terrain material registry, in layer order.
	*/
	if (TerrainTextureSystem::Get_Layer_Count() == 0) {
		TerrainTextureSystem::Define_Default_Layers();
	}
	TerrainLayers.Reset_Active();
	for (int i = 0; i < TerrainTextureSystem::Get_Layer_Count(); ++i) {
		const TerrainLayerClass * layer = TerrainTextureSystem::Peek_Layer(i);
		if (layer == NULL) {
			continue;
		}
		HybridTerrainLayerInput entry;
		entry.Name = layer->Name;
		entry.Kind = layer->Kind;
		entry.SurfaceType = (layer->Material != NULL) ? layer->Material->Get_Surface_Type() : -1;
		TerrainLayers.Add(entry);
	}

	DecorationTypesAvailable = true;
	for (int i = 0; i < (int)(sizeof(DECORATION_TYPE_NAMES) / sizeof(DECORATION_TYPE_NAMES[0])); ++i) {
		if (FoliageSystem::Find_Type_Index(DECORATION_TYPE_NAMES[i]) < 0) {
			DecorationTypesAvailable = false;
		}
	}
	LampTypeAvailable = (FoliageSystem::Find_Type_Index(LAMP_TYPE_NAME) >= 0);

	AllowOversizeDevelopment = false;
}


static bool Vehicle_Metric_Usable(const HybridVehicleMetricInput & metric)
{
	return metric.Measured && (metric.Width > 0.0f) && (metric.Height > 0.0f) && (metric.Length > 0.0f);
}


static bool Fail(HybridPreflightResult * result,HybridPreflightFailure failure,const char * detail)
{
	result->Failure = failure;
	result->Detail = detail;
	WWDEBUG_SAY(("Hybrid preflight failed: %s (%s)\n",Hybrid_Preflight_Failure_Name(failure),detail));
	return false;
}


bool Hybrid_Preflight_Request_Check(const ProceduralWorldProfile & profile,bool allow_oversize,HybridPreflightResult * result)
{
	WWASSERT(result != NULL);
	result->Failure = HYBRID_PREFLIGHT_OK;
	result->Detail = "";
	char detail[256];

	/*
	** Section 5: the hybrid generator is built around rivers, lakes, highways and Tiberium fields.
	*/
	if (!profile.Water.Rivers)					return Fail(result,HYBRID_PREFLIGHT_REQUIRED_FEATURE_DISABLED,"water.rivers");
	if (!profile.Water.Lakes)					return Fail(result,HYBRID_PREFLIGHT_REQUIRED_FEATURE_DISABLED,"water.lakes");
	if (!profile.Features.Highways)			return Fail(result,HYBRID_PREFLIGHT_REQUIRED_FEATURE_DISABLED,"world_features.highways");
	if (!profile.Features.TiberiumFields)	return Fail(result,HYBRID_PREFLIGHT_REQUIRED_FEATURE_DISABLED,"world_features.tiberium_fields");

	/*
	** Section 0.6 hard limits.
	*/
	if (!Hybrid_World_Size_Within_Limits(profile.World.Width,profile.World.Height)) {
		const bool positive = (profile.World.Width > 0.0f) && (profile.World.Height > 0.0f);
		if (!positive || !allow_oversize) {
			::snprintf(detail,sizeof(detail),"%.3f x %.3f",profile.World.Width,profile.World.Height);
			return Fail(result,HYBRID_PREFLIGHT_WORLD_SIZE_LIMIT_EXCEEDED,detail);
		}
		WWDEBUG_SAY(("Hybrid preflight: oversize %.3f x %.3f allowed by the developer/offline path\n",
						  profile.World.Width,profile.World.Height));
	}

	return true;
}


bool Hybrid_Run_Preflight(const ProceduralWorldProfile & profile,const HybridPreflightInputs & inputs,
								  HybridPreflightResult * result)
{
	WWASSERT(result != NULL);
	result->Failure = HYBRID_PREFLIGHT_OK;
	result->Detail = "";
	HybridMetricCache & m = result->Metrics;
	::memset(&m,0,sizeof(m));
	char detail[256];

	if (!Hybrid_Preflight_Request_Check(profile,inputs.AllowOversizeDevelopment,result)) {
		return false;
	}

	/*
	** Section 0.8 metric cache, in the plan's order.
	*/
	if (!(inputs.WorldUnitsPerMeter > 0.0f)) {
		return Fail(result,HYBRID_PREFLIGHT_MISSING_WORLD_UNITS_PER_METER,"world units per meter");
	}
	if (!Vehicle_Metric_Usable(inputs.Humvee)) {
		return Fail(result,HYBRID_PREFLIGHT_MISSING_HUMVEE_COLLISION_METRIC,HUMVEE_MODEL_NAME);
	}
	if (!Vehicle_Metric_Usable(inputs.HeavyVehicle)) {
		return Fail(result,HYBRID_PREFLIGHT_MISSING_HEAVY_VEHICLE_COLLISION_METRIC,"tracked");
	}
	static const char * const HARVESTER_TEAM_NAMES[2] = { "GDI refinery harvester", "Nod refinery harvester" };
	for (int team = 0; team < 2; ++team) {
		if (!Vehicle_Metric_Usable(inputs.Harvesters[team])) {
			return Fail(result,HYBRID_PREFLIGHT_MISSING_HARVESTER_COLLISION_METRIC,HARVESTER_TEAM_NAMES[team]);
		}
	}

	Hybrid_Compute_Constants(inputs.Humvee.Width,inputs.WorldUnitsPerMeter,profile.World.ChunkSize,
									 profile.World.Width,profile.World.Height,(float)profile.Plan.BaseRadiusCells,
									 profile.Fairness.MinBaseExpansionArea,&m.Constants);
	const float w = m.Constants.W;

	m.HumveeWidth = inputs.Humvee.Width;
	m.HumveeHeight = inputs.Humvee.Height;
	m.HumveeDefinitionID = inputs.Humvee.DefinitionID;
	m.HeavyVehicleWidth = inputs.HeavyVehicle.Width;
	m.HeavyVehicleHeight = inputs.HeavyVehicle.Height;
	m.HeavyVehicleDefinitionID = inputs.HeavyVehicle.DefinitionID;
	for (int team = 0; team < 2; ++team) {
		const HybridVehicleMetricInput & harvester = inputs.Harvesters[team];
		m.HarvesterWidth = (harvester.Width > m.HarvesterWidth) ? harvester.Width : m.HarvesterWidth;
		m.HarvesterLength = (harvester.Length > m.HarvesterLength) ? harvester.Length : m.HarvesterLength;
		m.HarvesterHeight = (harvester.Height > m.HarvesterHeight) ? harvester.Height : m.HarvesterHeight;
		m.HarvesterDefinitionID[team] = harvester.DefinitionID;
	}

	/*
	** Bridges (Section 12.4 clear width): every class gets the lowest-StableId definition whose deck
	** is at least max(carriageway, swept width + 2 * min_vehicle_lane_clearance).  The heavy route
	** class must be carried.
	*/
	m.HeavyRouteClass = (m.Constants.SizeClass == HYBRID_SIZE_SMALL) ? HYBRID_ROAD_PRIMARY_2 : HYBRID_ROAD_HIGHWAY_4;
	const float lane_clearance = profile.Fairness.MinVehicleLaneClearance;
	for (int c = 0; c < HYBRID_ROAD_CLASS_COUNT; ++c) {
		const HybridRoadClass road_class = (HybridRoadClass)c;
		//	A HIGHWAY_RAMP never carries its own bridge in this generator version (12.6.8 test 5).
		if (!Hybrid_Road_Class_Is_Routed(c)) {
			m.BridgeRequiredWidth[c]	= 0.0f;
			m.BridgeForClass[c]			= -1;
			continue;
		}
		float swept = m.HumveeWidth;
		if (road_class != HYBRID_ROAD_TRAIL_1) {
			swept = (m.HarvesterWidth > swept) ? m.HarvesterWidth : swept;
		}
		if (road_class == HYBRID_ROAD_HIGHWAY_4 || road_class == HYBRID_ROAD_PRIMARY_2 || road_class == HYBRID_ROAD_SECONDARY_2) {
			swept = (m.HeavyVehicleWidth > swept) ? m.HeavyVehicleWidth : swept;
		}
		const float carriageway = Hybrid_Carriageway_Width(road_class,w);
		const float vehicle = swept + 2.0f * lane_clearance;
		m.BridgeRequiredWidth[c] = (carriageway > vehicle) ? carriageway : vehicle;
		m.BridgeForClass[c] = -1;
		for (int b = 0; b < inputs.Bridges.Count(); ++b) {
			if (inputs.Bridges[b].Width >= m.BridgeRequiredWidth[c]) {
				m.BridgeForClass[c] = b;
				break;
			}
		}
	}
	m.BridgeMaxSpanLength = inputs.BridgeMaxSpanLength;
	m.BridgeWaterClearanceMargin = inputs.BridgeWaterClearanceMargin;
	m.BridgeApproachLength = (inputs.BridgeApproachLength > 0.0f) ? inputs.BridgeApproachLength : 8.0f * w;
	for (int c = 0; c < HYBRID_ROAD_CLASS_COUNT; ++c) {
		m.BridgeApproachMax[c]	= 0.0f;
		m.OverpassDepthMax[c]	= 0.0f;
		if (!Hybrid_Road_Class_Is_Routed(c)) continue;
		for (int b = 0; b < inputs.Bridges.Count(); ++b) {
			if (inputs.Bridges[b].Width < m.BridgeRequiredWidth[c]) continue;
			if (m.BridgeApproachLength > m.BridgeApproachMax[c])			m.BridgeApproachMax[c] = m.BridgeApproachLength;
			if (inputs.Bridges[b].DeckThickness > m.OverpassDepthMax[c])	m.OverpassDepthMax[c] = inputs.Bridges[b].DeckThickness;
		}
	}
	if (m.BridgeForClass[m.HeavyRouteClass] < 0 || !(m.BridgeMaxSpanLength > 0.0f)) {
		::snprintf(detail,sizeof(detail),"%s needs clear width %.3f",
					  Hybrid_Road_Class_Name(m.HeavyRouteClass),m.BridgeRequiredWidth[m.HeavyRouteClass]);
		return Fail(result,HYBRID_PREFLIGHT_NO_HEAVY_CAPABLE_BRIDGE,detail);
	}

	m.FordsAvailable = profile.Plan.AllowFords && inputs.FordCapabilityAvailable;
	if (profile.Plan.AllowFords && !inputs.FordCapabilityAvailable) {
		return Fail(result,HYBRID_PREFLIGHT_FORD_CAPABILITY_UNAVAILABLE,"plan.allow_fords");
	}

	for (int i = 0; i < inputs.Buildings.Count(); ++i) {
		const HybridBuildingFootprintInput & building = inputs.Buildings[i];
		if (!building.Found || !(building.Radius > 0.0f)) {
			return Fail(result,HYBRID_PREFLIGHT_MISSING_STARTING_BUILDING_FOOTPRINT,building.Name.Peek_Buffer());
		}
	}

	if (!(inputs.ReferenceVehicleSpeed > 0.0f) || !(inputs.ReferenceInfantrySpeed > 0.0f)) {
		return Fail(result,HYBRID_PREFLIGHT_MISSING_FAIRNESS_REFERENCE_SPEED,"V_REF / I_REF");
	}
	m.ReferenceVehicleSpeed = inputs.ReferenceVehicleSpeed;
	m.ReferenceInfantrySpeed = inputs.ReferenceInfantrySpeed;

	if (!(profile.Resource.HarvesterLoadAmount > 0.0f)) {
		return Fail(result,HYBRID_PREFLIGHT_MISSING_HARVESTER_LOAD_AMOUNT,"resource.harvester_load_amount");
	}
	m.HarvesterLoadAmount = profile.Resource.HarvesterLoadAmount;

	/*
	** Section 4.2 biome availability: every slot resolved, lowest StableId (layer index) on a tie.
	*/
	int available_count = 0;
	for (int b = 0; b < HYBRID_BIOME_COUNT; ++b) {
		m.BiomeAvailable[b] = true;
		for (int s = 0; s < HYBRID_SLOT_COUNT; ++s) {
			m.BiomeSlotLayer[b][s] = -1;
			for (int l = 0; l < inputs.TerrainLayers.Count(); ++l) {
				if (Layer_Fills_Slot(inputs.TerrainLayers[l],(HybridBiome)b,(HybridMaterialSlot)s)) {
					m.BiomeSlotLayer[b][s] = l;
					break;
				}
			}
			if (m.BiomeSlotLayer[b][s] < 0) {
				m.BiomeAvailable[b] = false;
			}
		}
		available_count += m.BiomeAvailable[b] ? 1 : 0;
	}

	/*
	** An explicit biome (or an explicit secondary a mixed map may use) must be available; "auto"
	** parses to the *_AUTO selector, which is past the end of the biome table and skips the check.
	** Profile validation already rejected unknown names.
	*/
	HybridBiomeMode biome_mode = HYBRID_BIOME_MODE_AUTO;
	HybridBiome biome = HYBRID_BIOME_AUTO;
	HybridBiome secondary = HYBRID_BIOME_AUTO;
	Hybrid_Biome_Mode_From_Name(profile.Plan.BiomeMode.Peek_Buffer(),&biome_mode);
	Hybrid_Biome_From_Name(profile.Plan.Biome.Peek_Buffer(),&biome);
	Hybrid_Biome_From_Name(profile.Plan.SecondaryBiome.Peek_Buffer(),&secondary);
	if (available_count == 0) {
		return Fail(result,HYBRID_PREFLIGHT_UNSUPPORTED_BIOME_ASSET_SET,"no biome resolves every material slot");
	}
	if (biome >= 0 && biome < HYBRID_BIOME_COUNT && !m.BiomeAvailable[biome]) {
		return Fail(result,HYBRID_PREFLIGHT_UNSUPPORTED_BIOME_ASSET_SET,Hybrid_Biome_Name(biome));
	}
	if (biome_mode != HYBRID_BIOME_MODE_SINGLE && secondary >= 0 && secondary < HYBRID_BIOME_COUNT && !m.BiomeAvailable[secondary]) {
		return Fail(result,HYBRID_PREFLIGHT_UNSUPPORTED_BIOME_ASSET_SET,Hybrid_Biome_Name(secondary));
	}
	// Explicit mixed requests (including auto with probability one) must have at least one
	// legal pair before P2. P2 applies this same availability filter if an auto draw picks mixed.
	if (biome_mode == HYBRID_BIOME_MODE_MIXED ||
		(biome_mode == HYBRID_BIOME_MODE_AUTO && profile.Plan.MixedBiomeProbability >= 1.0f)) {
		bool pair_available = false;
		for (int primary = 0; primary < HYBRID_BIOME_COUNT; ++primary) {
			if (!m.BiomeAvailable[primary] || (biome != HYBRID_BIOME_AUTO && biome != primary)) continue;
			for (int other = 0; other < HYBRID_BIOME_COUNT; ++other) {
				if (!m.BiomeAvailable[other] || (secondary != HYBRID_BIOME_AUTO && secondary != other)) continue;
				if (Hybrid_Is_Legal_Biome_Pair((HybridBiome)primary,(HybridBiome)other)) pair_available = true;
			}
		}
		if (!pair_available) {
			return Fail(result,HYBRID_PREFLIGHT_UNSUPPORTED_BIOME_ASSET_SET,"no available compatible biome pair");
		}
	}

	m.DecorationAvailable = inputs.DecorationTypesAvailable;
	m.LampsAvailable = inputs.LampTypeAvailable;
	if (!m.DecorationAvailable) {
		WWDEBUG_SAY(("Hybrid preflight: optional decoration types missing; decoration will be omitted\n"));
	}
	if (!m.LampsAvailable) {
		WWDEBUG_SAY(("Hybrid preflight: optional lamp type missing; lamps will be omitted\n"));
	}

	/*
	** Section 16 budgets, then the identity hash over everything resolved above.
	*/
	const int analysis_cells = (int)(::ceil((double)profile.World.Width / (double)m.Constants.A) *
												::ceil((double)profile.World.Height / (double)m.Constants.A));
	Hybrid_Get_Work_Budgets(m.Constants.SizeClass,analysis_cells,profile.Retry.MaxAlternateCandidatesPerFeature,&m.Budgets);

	m.ContentRevision = Hybrid_Structural_Content_Revision(inputs);
	m.PlanningInputHash = Hybrid_Planning_Input_Hash(profile,inputs,m);
	return true;
}


static void Add_Vehicle_Metric(HybridHashStream & h,const HybridVehicleMetricInput & metric)
{
	h.Add_Bool(metric.Measured);
	h.Add_Int(metric.DefinitionID);
	h.Add_Length(metric.Length);
	h.Add_Length(metric.Width);
	h.Add_Length(metric.Height);
}


unsigned int Hybrid_Structural_Content_Revision(const HybridPreflightInputs & inputs)
{
	HybridHashStream h;

	h.Add_Length(inputs.WorldUnitsPerMeter);
	Add_Vehicle_Metric(h,inputs.Humvee);
	Add_Vehicle_Metric(h,inputs.HeavyVehicle);
	Add_Vehicle_Metric(h,inputs.Harvesters[0]);
	Add_Vehicle_Metric(h,inputs.Harvesters[1]);

	h.Add_Int(inputs.Bridges.Count());
	for (int b = 0; b < inputs.Bridges.Count(); ++b) {
		const HybridBridgeInput & bridge = inputs.Bridges[b];
		h.Add_String(bridge.Name.Peek_Buffer());
		h.Add_Length(bridge.Width);
		h.Add_Length(bridge.DeckThickness);
		h.Add_Length(bridge.RequiredClearance);
		h.Add_Length(bridge.SupportSpacing);
		h.Add_Length(bridge.SupportClearance);
		h.Add_Length(bridge.StartSeat);
		h.Add_Length(bridge.EndSeat);
		h.Add_String(bridge.StructuralModels.Peek_Buffer());
	}
	h.Add_Length(inputs.BridgeMaxSpanLength);
	h.Add_Length(inputs.BridgeWaterClearanceMargin);
	h.Add_Length(inputs.BridgeApproachLength);
	h.Add_Bool(inputs.FordCapabilityAvailable);

	// Footprints only: a building's height comes from its render bounds.
	h.Add_Int(inputs.Buildings.Count());
	for (int i = 0; i < inputs.Buildings.Count(); ++i) {
		const HybridBuildingFootprintInput & building = inputs.Buildings[i];
		h.Add_Int(building.Team);
		h.Add_String(building.Name.Peek_Buffer());
		h.Add_Bool(building.Found);
		h.Add_Length(building.Radius);
	}

	// Layer identity and collision surface; textures are render-only.
	h.Add_Int(inputs.TerrainLayers.Count());
	for (int i = 0; i < inputs.TerrainLayers.Count(); ++i) {
		h.Add_String(inputs.TerrainLayers[i].Name.Peek_Buffer());
		h.Add_Int(inputs.TerrainLayers[i].Kind);
		h.Add_Int(inputs.TerrainLayers[i].SurfaceType);
	}

	return h.Get();
}


unsigned int Hybrid_Planning_Input_Hash(const ProceduralWorldProfile & p,const HybridPreflightInputs & inputs,
													 const HybridMetricCache & m)
{
	HybridHashStream h;

	h.Add_Uint(p.SchemaVersion);
	h.Add_Uint(p.GeneratorVersion);
	h.Add_Uint(m.ContentRevision);

	// world
	h.Add_Length(p.World.Width);
	h.Add_Length(p.World.Height);
	h.Add_Length(p.World.ChunkSize);
	h.Add_Length(p.World.SeaLevel);
	h.Add_Bool(p.World.FitToLevel);
	h.Add_Length(p.World.CenterX);
	h.Add_Length(p.World.CenterY);

	// boundary
	h.Add_String(p.Boundary.Shape.Peek_Buffer());
	h.Add_Length(p.Boundary.CliffBandWidth);
	h.Add_Length(p.Boundary.CliffMinHeight);
	h.Add_Length(p.Boundary.CliffMaxHeight);
	h.Add_Length(p.Boundary.HardBarrierOffset);
	h.Add_Length(p.Boundary.AirCeiling);
	h.Add_Scalar(p.Boundary.RimRoughness);
	h.Add_Scalar(p.Boundary.FaceRoughness);
	h.Add_Length(p.Boundary.ToeWidth);
	h.Add_Int(p.Boundary.CollisionSimplification);

	// terrain
	h.Add_Length(p.Terrain.MacroRelief);
	h.Add_Scalar(p.Terrain.HillDensity);
	h.Add_Scalar(p.Terrain.MountainDensity);
	h.Add_Scalar(p.Terrain.InteriorCliffDensity);
	h.Add_Scalar(p.Terrain.ValleyDensity);
	h.Add_Length(p.Terrain.MinElevation);
	h.Add_Length(p.Terrain.MaxElevation);
	h.Add_Scalar(p.Terrain.LandElevationBias);
	h.Add_Scalar(p.Terrain.MaxBuildableSlopeDegrees);
	h.Add_Scalar(p.Terrain.FeatureScale);
	h.Add_Scalar(p.Terrain.RidgeSharpness);
	h.Add_Scalar(p.Terrain.PlateauChance);
	h.Add_Scalar(p.Terrain.BoundaryApproachFlattening);

	// water
	h.Add_Bool(p.Water.Rivers);
	h.Add_Bool(p.Water.Lakes);
	h.Add_Bool(p.Water.Ponds);
	h.Add_Bool(p.Water.Coastline);
	h.Add_Length(p.Water.RiverWidth);
	h.Add_Length(p.Water.StreamWidth);
	h.Add_Scalar(p.Water.RiverAccumulationChunks);
	h.Add_Int(p.Water.MaxRiverCount);

	// plan -- every active hybrid field; LaneWidthCells is legacy and excluded
	h.Add_Bool(p.Plan.Enabled);
	h.Add_String(p.Plan.Symmetry.Peek_Buffer());
	h.Add_Int(p.Plan.TierCount);
	h.Add_Length(p.Plan.TierStep);
	h.Add_Scalar(p.Plan.DetailWeight);
	h.Add_Int(p.Plan.BaseRadiusCells);
	h.Add_Int(p.Plan.FlankLanes);
	h.Add_Scalar(p.Plan.WaterFraction);
	h.Add_Scalar(p.Plan.BoundaryMarginCells);
	h.Add_String(p.Plan.Archetype.Peek_Buffer());
	h.Add_String(p.Plan.BiomeMode.Peek_Buffer());
	h.Add_String(p.Plan.Biome.Peek_Buffer());
	h.Add_String(p.Plan.SecondaryBiome.Peek_Buffer());
	h.Add_Scalar(p.Plan.MixedBiomeProbability);
	h.Add_Scalar(p.Plan.RoadDensity);
	h.Add_Scalar(p.Plan.IslandChance);
	h.Add_Int(p.Plan.RouteRedundancy);
	h.Add_Bool(p.Plan.AllowFords);

	// resource / Tiberium
	h.Add_Length(p.Resource.FieldRadiusMin);
	h.Add_Length(p.Resource.FieldRadiusMax);
	h.Add_Scalar(p.Resource.InitialYieldMin);
	h.Add_Scalar(p.Resource.InitialYieldMax);
	h.Add_Scalar(p.Resource.CrystalDensity);
	h.Add_Int(p.Resource.HarvestAnchorCount);
	h.Add_Scalar(p.Resource.HarvesterLoadAmount);
	h.Add_Bool(p.Resource.GrowthEnabled);
	h.Add_Scalar(p.Resource.GrowthRatePerSecond);

	// active world-feature toggles -- Settlements is legacy and excluded, and so is Tunnels: the native
	// generator builds no tunnels (D-032), so the legacy field changes nothing it produces
	h.Add_Bool(p.Features.Forests);
	h.Add_Bool(p.Features.Highways);
	h.Add_Bool(p.Features.TiberiumFields);

	// road grading
	h.Add_Length(p.Grading.MaxCutDepth);
	h.Add_Length(p.Grading.MaxFillHeight);
	h.Add_Angle(p.Grading.MaxSlopeChangeRadians);
	h.Add_Scalar(p.Grading.MaxAffectedArea);

	// fairness
	h.Add_Scalar(p.Fairness.MaxInitialResourceTravelTimeDifferenceSeconds);
	h.Add_Scalar(p.Fairness.MaxProjectedInitialIncomeDifferencePercent);
	h.Add_Scalar(p.Fairness.MaxBaseToFrontTravelTimeDifferenceSeconds);
	h.Add_Scalar(p.Fairness.MaxBaseToBaseTravelTimeDifferenceSeconds);
	h.Add_Scalar(p.Fairness.MinBaseToBaseTravelTimeSeconds);
	h.Add_Int(p.Fairness.MinVehicleApproachCount);
	h.Add_Int(p.Fairness.MinInfantryApproachCount);
	h.Add_Scalar(p.Fairness.MinBaseExpansionArea);
	h.Add_Bool(p.Fairness.AllowSingleBridgeDependency);
	h.Add_Length(p.Fairness.MaxMandatoryExposedInfantryRunDistance);
	h.Add_Scalar(p.Fairness.MaxMandatoryExposedInfantryRunTimeSeconds);
	h.Add_Length(p.Fairness.PreferredCoverInterval);
	h.Add_Length(p.Fairness.MinVehicleLaneClearance);

	// retry
	h.Add_Int(p.Retry.MaxLocalRetriesPerStage);
	h.Add_Int(p.Retry.MaxAlternateCandidatesPerFeature);

	// starting buildings -- structural, their footprints below
	h.Add_Int(inputs.Buildings.Count());
	for (int i = 0; i < inputs.Buildings.Count(); ++i) {
		h.Add_Int(inputs.Buildings[i].Team);
		h.Add_String(inputs.Buildings[i].Name.Peek_Buffer());
		h.Add_Length(inputs.Buildings[i].Radius);
	}

	// structural asset availability from the metric cache; decoration and lamps are render-only
	h.Add_Length(m.Constants.WorldUnitsPerMeter);
	h.Add_Length(m.HumveeWidth);
	h.Add_Length(m.HumveeHeight);
	h.Add_Length(m.HeavyVehicleWidth);
	h.Add_Length(m.HeavyVehicleHeight);
	h.Add_Length(m.HarvesterWidth);
	h.Add_Length(m.HarvesterLength);
	h.Add_Length(m.HarvesterHeight);
	h.Add_Int(m.HumveeDefinitionID);
	h.Add_Int(m.HeavyVehicleDefinitionID);
	h.Add_Int(m.HarvesterDefinitionID[0]);
	h.Add_Int(m.HarvesterDefinitionID[1]);
	h.Add_Scalar(m.ReferenceVehicleSpeed);
	h.Add_Scalar(m.ReferenceInfantrySpeed);
	h.Add_Bool(m.FordsAvailable);
	// Later crossing selection can choose any registered bridge, not just the first width
	// match cached below. Registry indices are the canonical bridge StableIds.
	h.Add_Int(inputs.Bridges.Count());
	for (int b = 0; b < inputs.Bridges.Count(); ++b) {
		h.Add_Int(b);
		h.Add_String(inputs.Bridges[b].Name.Peek_Buffer());
		h.Add_Length(inputs.Bridges[b].Width);
		h.Add_Length(inputs.Bridges[b].DeckThickness);
		h.Add_Length(inputs.Bridges[b].RequiredClearance);
		h.Add_Length(inputs.Bridges[b].SupportSpacing);
		h.Add_Length(inputs.Bridges[b].SupportClearance);
		h.Add_Length(inputs.Bridges[b].StartSeat);
		h.Add_Length(inputs.Bridges[b].EndSeat);
		h.Add_String(inputs.Bridges[b].StructuralModels.Peek_Buffer());
	}
	for (int c = 0; c < HYBRID_ROAD_CLASS_COUNT; ++c) {
		const int b = m.BridgeForClass[c];
		h.Add_Int(b);
		if (b >= 0 && b < inputs.Bridges.Count()) {
			h.Add_String(inputs.Bridges[b].Name.Peek_Buffer());
			h.Add_Length(inputs.Bridges[b].Width);
			h.Add_Length(inputs.Bridges[b].DeckThickness);
			h.Add_Length(inputs.Bridges[b].RequiredClearance);
		}
	}
	h.Add_Length(m.BridgeMaxSpanLength);
	h.Add_Length(m.BridgeWaterClearanceMargin);
	h.Add_Length(m.BridgeApproachLength);
	for (int c = 0; c < HYBRID_ROAD_CLASS_COUNT; ++c) {
		h.Add_Length(m.BridgeApproachMax[c]);
		h.Add_Length(m.OverpassDepthMax[c]);
	}
	for (int b = 0; b < HYBRID_BIOME_COUNT; ++b) {
		h.Add_Bool(m.BiomeAvailable[b]);
		for (int slot = 0; slot < HYBRID_SLOT_COUNT; ++slot) {
			const int layer = m.BiomeSlotLayer[b][slot];
			h.Add_Int(layer);
			if (layer >= 0 && layer < inputs.TerrainLayers.Count()) {
				h.Add_String(inputs.TerrainLayers[layer].Name.Peek_Buffer());
				h.Add_Int(inputs.TerrainLayers[layer].Kind);
				h.Add_Int(inputs.TerrainLayers[layer].SurfaceType);
			}
		}
	}

	return h.Get();
}
