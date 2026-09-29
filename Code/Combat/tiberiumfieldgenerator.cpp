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
 *                     $Archive:: /Commando/Code/Combat/tiberiumfieldgenerator.cpp            $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "tiberiumfieldgenerator.h"

#include "baseselectionplanner.h"
#include "crc.h"
#include "playertype.h"
#include "roadgraphgenerator.h"
#include "strategicplanner.h"
#include "terrainmask.h"
#include "terraintexturesystem.h"
#include "watersystem.h"
#include "worldmetrics.h"
#include "worldterrainsystem.h"
#include "wwdebug.h"
#include "wwmath.h"


bool				TiberiumFieldGenerator::Built		= false;
float				TiberiumFieldGenerator::LastReferenceVehicleSpeed	= 0.0f;
unsigned int	TiberiumFieldGenerator::Checksum	= 0;

DynamicVectorClass<ResolvedTiberiumField>		TiberiumFieldGenerator::Fields;
DynamicVectorClass<TiberiumCrystalPlacement>	TiberiumFieldGenerator::Crystals;
DynamicVectorClass<ResourceRepairLogEntry>	TiberiumFieldGenerator::RepairLog;


static const float CHECKSUM_QUANTUM = 1000.0f;

//	Step 3's own bounded radius shrink ("adjust field edge") between FieldRadiusMax and
//	FieldRadiusMin -- how many candidate radii are tried before a footprint is declared illegal
//	at every size this profile permits.
static const int FOOTPRINT_SHRINK_STEPS = 4;

//	How many points around the footprint's own rim (plus the centre) are sampled for water/road
//	occupancy -- a coarse but real check, the same tolerance StrategicPlanner's own chunk scan
//	already accepts for eligibility.
static const int FOOTPRINT_RING_SAMPLES = 8;

//	A mask value at or above this is "occupied" by that feature -- Stamp_Disc/Stamp_Polyline
//	write full strength (1.0) at their own centre/spine, fading through the feather, so a value
//	this high is real overlap, not a distant shoulder.
static const float ROAD_MASK_OCCUPIED_THRESHOLD = 0.5f;

//	How much footprint area one scattered crystal represents at CrystalDensity == 1 -- a fixed,
//	documented engineering estimate, the same category of constant BaseSelectionPlanner's own
//	reference travel speeds are (no canonical v1 data source exists for Tiberium crystal
//	spacing), roughly a crystal every 2.4 world units at full density.
static const float CRYSTAL_FOOTPRINT_AREA = 6.0f;

//	Section 17A.6's own "expected early income per minute": one Harvester load per round trip
//	(there and back), so a trip's own income rate is the load divided by twice its one-way time.
static const float ROUND_TRIP_MULTIPLIER = 2.0f;


const char * Resource_Repair_Operation_Name(ResourceRepairOperationType op)
{
	switch (op) {
		case RESOURCE_REPAIR_NONE:							return "none";
		case RESOURCE_REPAIR_ADJUST_FIELD_EDGE:			return "adjust_field_edge";
		case RESOURCE_REPAIR_REPAIR_ROAD_LOCALLY:		return "repair_road_locally";
		case RESOURCE_REPAIR_ALTERNATE_FIELD_CANDIDATE:	return "alternate_field_candidate";
		default:														return "unknown";
	}
}


ResolvedTiberiumField::ResolvedTiberiumField(void) :
	OwningFaction(PLAYERTYPE_NEUTRAL), Position(0.0f,0.0f,0.0f), Radius(0.0f),
	InitialYield(0.0f), CrystalCount(0), HasHarvesterRoute(false), HarvesterRouteLength(0.0f),
	ExpectedRouteTimeSeconds(0.0f), ExpectedIncomePerMinute(0.0f), CountsAsEarlyField(false)
{ }

bool ResolvedTiberiumField::operator == (const ResolvedTiberiumField & that) const
{
	return (Id == that.Id) && (ResourceNodeId == that.ResourceNodeId) &&
			 (OwningFaction == that.OwningFaction) && (Position == that.Position) &&
			 (Radius == that.Radius) && (InitialYield == that.InitialYield) &&
			 (CrystalCount == that.CrystalCount) && (HasHarvesterRoute == that.HasHarvesterRoute) &&
			 (HarvesterRouteLength == that.HarvesterRouteLength) &&
			 (ExpectedRouteTimeSeconds == that.ExpectedRouteTimeSeconds) &&
			 (ExpectedIncomePerMinute == that.ExpectedIncomePerMinute) &&
			 (CountsAsEarlyField == that.CountsAsEarlyField);
}


TiberiumCrystalPlacement::TiberiumCrystalPlacement(void) : FieldIndex(-1), Position(0.0f,0.0f,0.0f) { }

bool TiberiumCrystalPlacement::operator == (const TiberiumCrystalPlacement & that) const
{
	return (FieldIndex == that.FieldIndex) && (Position == that.Position);
}


ResourceRepairLogEntry::ResourceRepairLogEntry(void) : Operation(RESOURCE_REPAIR_NONE), Applied(false) { }

bool ResourceRepairLogEntry::operator == (const ResourceRepairLogEntry & that) const
{
	return (CandidateResourceNodeId == that.CandidateResourceNodeId) && (Operation == that.Operation) &&
			 (Applied == that.Applied);
}


TiberiumFieldParams::TiberiumFieldParams(void) :
	Seed(1),
	FieldRadiusMin(20.0f),FieldRadiusMax(60.0f),
	InitialYieldMin(1500.0f),InitialYieldMax(4000.0f),
	CrystalDensity(0.15f),HarvestAnchorCount(3),
	HarvesterLoadAmount(600.0f),
	GrowthEnabled(false),GrowthRatePerSecond(0.0f),
	MaxRepairAttemptsPerField(8),
	ReferenceVehicleSpeed(WorldMetricsClass::Get_Reference_Vehicle_Speed()),
	MaxBuildableSlopeRadians(0.5f)
{ }


/***********************************************************************************************
 * local helpers                                                                                 *
 *=============================================================================================*/

//	Step 4's own rejection list, made concrete over one candidate footprint: on-terrain, within
//	the shared slope cap (Query_Placement_Surface's own whole-disc measurement, not one sample
//	at the centre), clear of both selected bases' own reserved regions (BaseSelectionPlanner,
//	Phase 11), and -- ring-sampled around the rim, the same coarse-but-real tolerance
//	StrategicPlanner's own chunk scan already accepts -- clear of water and of the road mask.
static bool Is_Footprint_Legal(const Vector3 & position,float radius,float max_slope_radians,
										  bool has_gdi_site,const Vector3 & gdi_pos,float gdi_protect_radius,
										  bool has_nod_site,const Vector3 & nod_pos,float nod_protect_radius)
{
	TerrainPlacementInfoClass info;
	if (!WorldTerrainSystem::Query_Placement_Surface(position,radius,&info)) return false;
	if (!info.IsOnTerrain) return false;
	if (info.MaxSlope > max_slope_radians) return false;

	if (has_gdi_site && ((position - gdi_pos).Length() < (radius + gdi_protect_radius))) return false;
	if (has_nod_site && ((position - nod_pos).Length() < (radius + nod_protect_radius))) return false;

	TerrainMaskClass * road_mask = TerrainTextureSystem::Has_Masks() ?
		TerrainTextureSystem::Peek_Mask(TERRAIN_MASK_ROAD) : nullptr;

	for (int i = 0; i <= FOOTPRINT_RING_SAMPLES; i++) {
		Vector3 sample = position;
		if (i > 0) {
			float angle = ((float)(i - 1) / (float)FOOTPRINT_RING_SAMPLES) * (2.0f * WWMATH_PI);
			sample.X += WWMath::Cos(angle) * radius;
			sample.Y += WWMath::Sin(angle) * radius;
		}

		if (WaterSystem::Is_Point_In_Water(sample.X,sample.Y)) return false;

		if (road_mask != nullptr) {
			float v = 0.0f;
			if (road_mask->Sample(sample.X,sample.Y,&v) && (v >= ROAD_MASK_OCCUPIED_THRESHOLD)) return false;
		}
	}

	return true;
}


//	Step 3's own bounded shrink: try FieldRadiusMax first, then step down toward FieldRadiusMin
//	over FOOTPRINT_SHRINK_STEPS attempts.  Returns the legal radius found, or < 0 when even the
//	minimum is illegal.
static float Resolve_Footprint_Radius(const Vector3 & position,const TiberiumFieldParams & params,
													bool has_gdi_site,const Vector3 & gdi_pos,float gdi_protect_radius,
													bool has_nod_site,const Vector3 & nod_pos,float nod_protect_radius,
													bool * needed_shrink_out)
{
	*needed_shrink_out = false;

	float span = params.FieldRadiusMax - params.FieldRadiusMin;
	for (int step = 0; step <= FOOTPRINT_SHRINK_STEPS; step++) {
		float t = (FOOTPRINT_SHRINK_STEPS > 0) ? ((float)step / (float)FOOTPRINT_SHRINK_STEPS) : 0.0f;
		float radius = params.FieldRadiusMax - (span * t);

		if (Is_Footprint_Legal(position,radius,params.MaxBuildableSlopeRadians,
										has_gdi_site,gdi_pos,gdi_protect_radius,
										has_nod_site,nod_pos,nod_protect_radius)) {
			if (step > 0) *needed_shrink_out = true;
			return radius;
		}
	}

	return -1.0f;
}


//	Deterministic uniform-disc scatter: angle from one hash draw, radius-squared from a second
//	(sqrt of a uniform draw gives a uniform *area* distribution, not one that clusters at the
//	centre the way a uniform radius draw would).
static Vector3 Scatter_Point(const Vector3 & center,float radius,int field_index,int point_index,unsigned int seed)
{
	float angle_draw	= TerrainMaskClass::Hash_Value(field_index,(point_index * 2) + 100,(int)seed);
	float radius_draw	= TerrainMaskClass::Hash_Value(field_index,(point_index * 2) + 101,(int)seed);

	float angle = angle_draw * 2.0f * WWMATH_PI;
	float r = WWMath::Sqrt(radius_draw) * radius;

	Vector3 p = center;
	p.X += WWMath::Cos(angle) * r;
	p.Y += WWMath::Sin(angle) * r;
	return p;
}


/***********************************************************************************************
 * Init/Shutdown/Clear                                                                           *
 *=============================================================================================*/
void TiberiumFieldGenerator::Init(void)		{ Clear(); }
void TiberiumFieldGenerator::Shutdown(void)	{ Clear(); }

void TiberiumFieldGenerator::Clear(void)
{
	Built = false;
	Checksum = 0;
	Fields.Delete_All();
	Crystals.Delete_All();
	RepairLog.Delete_All();
}


/***********************************************************************************************
 * Resolve_One_Field                                                                             *
 *=============================================================================================*/
void TiberiumFieldGenerator::Resolve_One_Field(int node_index,int owning_faction,int owning_base_node,
																 const TiberiumFieldParams & params,
																 bool has_gdi_site,const Vector3 & gdi_pos,float gdi_protect_radius,
																 bool has_nod_site,const Vector3 & nod_pos,float nod_protect_radius,
																 int * crystal_index_io)
{
	StrategicNode node;
	if (!StrategicPlanner::Get_Node(node_index,&node)) return;

	bool needed_shrink = false;
	float radius = Resolve_Footprint_Radius(node.Position,params,
															has_gdi_site,gdi_pos,gdi_protect_radius,
															has_nod_site,nod_pos,nod_protect_radius,
															&needed_shrink);
	if (radius <= 0.0f) return;		// illegal at every radius -- dropped, not fabricated

	if (needed_shrink) {
		ResourceRepairLogEntry entry;
		entry.CandidateResourceNodeId = node.Id;
		entry.Operation = RESOURCE_REPAIR_ADJUST_FIELD_EDGE;
		entry.Applied = true;
		RepairLog.Add(entry);
	}

	ResolvedTiberiumField field;
	field.Id.Format("field_%d",Fields.Count());
	field.ResourceNodeId	= node.Id;
	field.OwningFaction		= owning_faction;
	field.Position			= node.Position;
	field.Radius				= radius;

	float yield_draw = TerrainMaskClass::Hash_Value(Fields.Count(),1,(int)params.Seed);
	field.InitialYield = params.InitialYieldMin +
								(yield_draw * (params.InitialYieldMax - params.InitialYieldMin));

	float area = WWMATH_PI * radius * radius;
	if (params.CrystalDensity > 0.0f) {
		int estimate = (int)((area * params.CrystalDensity) / CRYSTAL_FOOTPRINT_AREA);
		field.CrystalCount = (estimate > 1) ? estimate : 1;
	} else {
		field.CrystalCount = 0;
	}

	if (owning_base_node >= 0) {
		float length = 0.0f;
		if (RoadGraphGenerator::Find_Anchor_Path(owning_base_node,node_index,nullptr,&length)) {
			field.HasHarvesterRoute = true;
			field.HarvesterRouteLength = length;
			field.ExpectedRouteTimeSeconds = length / params.ReferenceVehicleSpeed;
			float round_trip_minutes = (field.ExpectedRouteTimeSeconds * ROUND_TRIP_MULTIPLIER) / 60.0f;
			field.ExpectedIncomePerMinute = (round_trip_minutes > 0.0f) ?
				(params.HarvesterLoadAmount / round_trip_minutes) : 0.0f;
		}
	}

	field.CountsAsEarlyField = (owning_faction != PLAYERTYPE_NEUTRAL) && field.HasHarvesterRoute;

	int field_index = Fields.Count();
	Fields.Add(field);

	for (int c = 0; c < field.CrystalCount; c++) {
		TiberiumCrystalPlacement crystal;
		crystal.FieldIndex = field_index;
		crystal.Position = Scatter_Point(field.Position,field.Radius,field_index,*crystal_index_io,params.Seed);
		Crystals.Add(crystal);
		(*crystal_index_io)++;
	}

	if (TerrainTextureSystem::Has_Masks()) {
		TerrainMaskClass * tiberium_mask = TerrainTextureSystem::Peek_Mask(TERRAIN_MASK_TIBERIUM);
		if (tiberium_mask != nullptr) {
			tiberium_mask->Stamp_Disc(field.Position,field.Radius,1.0f,field.Radius * 0.15f);
		}
	}
}


/***********************************************************************************************
 * Generate -- Phase 12 steps 1-12, 14                                                           *
 *=============================================================================================*/
bool TiberiumFieldGenerator::Generate(const TiberiumFieldParams & params,StringClass * error_out)
{
	#define FAIL(msg) { if (error_out) *error_out = msg; Clear(); return false; }

	Clear();

	if (!StrategicPlanner::Has_Plan())				FAIL("no strategic plan -- generate resource candidates first");
	if (!RoadGraphGenerator::Has_Graph())			FAIL("no road graph -- connect the candidates first");
	if (!BaseSelectionPlanner::Has_Selection())	FAIL("no base selection -- choose bases first");
	LastReferenceVehicleSpeed = params.ReferenceVehicleSpeed;
	if (params.FieldRadiusMin <= 0.0f)				FAIL("field radius min must be positive");
	if (params.FieldRadiusMax < params.FieldRadiusMin)	FAIL("field radius max must not be less than field radius min");
	if (params.CrystalDensity < 0.0f)				FAIL("crystal density must not be negative");
	if (params.HarvestAnchorCount < 1)				FAIL("harvest anchor count must be at least 1");
	if (params.MaxRepairAttemptsPerField < 0)		FAIL("max repair attempts per field must not be negative");
	if (params.ReferenceVehicleSpeed <= 0.0f)		FAIL("reference vehicle speed must be positive");

	DynamicVectorClass<int> resource_nodes;
	for (int n = 0; n < StrategicPlanner::Get_Node_Count(); n++) {
		StrategicNode node;
		if (!StrategicPlanner::Get_Node(n,&node)) continue;
		if (node.Type == STRATEGIC_NODE_RESOURCE_CANDIDATE) resource_nodes.Add(n);
	}
	if (resource_nodes.Count() == 0) {
		Built = true;			// correctly nothing to resolve, not a failure
		return true;
	}

	SelectedBaseSite gdi_site, nod_site;
	bool has_gdi = BaseSelectionPlanner::Get_Selected_Site(PLAYERTYPE_GDI,&gdi_site);
	bool has_nod = BaseSelectionPlanner::Get_Selected_Site(PLAYERTYPE_NOD,&nod_site);

	int gdi_node = -1, nod_node = -1;
	if (has_gdi) StrategicPlanner::Find_Node_By_Id(gdi_site.Id,&gdi_node);
	if (has_nod) StrategicPlanner::Find_Node_By_Id(nod_site.Id,&nod_node);

	//
	//	Steps 1-2: which candidates are initial (team-owned), which are contested.  An initial
	//	pick tries the nearest reachable resource candidate first, then the next-nearest
	//	(RESOURCE_REPAIR_ALTERNATE_FIELD_CANDIDATE) up to the bound, since a team having *no*
	//	initial field at all is the one outcome Phase 12's own gate treats as a real failure.
	//
	DynamicVectorClass<int> claimed;
	int gdi_field_node = -1, nod_field_node = -1;

	if (has_gdi && (gdi_node >= 0)) {
		int attempts = 0;
		while (attempts <= params.MaxRepairAttemptsPerField) {
			float length = 0.0f;
			int candidate = -1, best_node_index = -1;
			float best_length = -1.0f;
			for (int r = 0; r < resource_nodes.Count(); r++) {
				int rn = resource_nodes[r];
				bool already = false;
				for (int c = 0; c < claimed.Count(); c++) { if (claimed[c] == rn) { already = true; break; } }
				if (already) continue;
				if (!RoadGraphGenerator::Find_Anchor_Path(gdi_node,rn,nullptr,&length)) continue;
				if ((best_length < 0.0f) || (length < best_length)) { best_length = length; best_node_index = rn; }
			}
			candidate = best_node_index;
			if (candidate < 0) break;		// nothing left reachable at all

			StrategicNode node;
			StrategicPlanner::Get_Node(candidate,&node);
			bool needed_shrink = false;
			float radius = Resolve_Footprint_Radius(node.Position,params,
																	has_gdi,gdi_site.Position,gdi_site.ProtectedRadius,
																	has_nod,nod_site.Position,nod_site.ProtectedRadius,
																	&needed_shrink);

			if (radius > 0.0f) {
				gdi_field_node = candidate;
				claimed.Add(candidate);
				if (attempts > 0) {
					ResourceRepairLogEntry entry;
					entry.CandidateResourceNodeId = node.Id;
					entry.Operation = RESOURCE_REPAIR_ALTERNATE_FIELD_CANDIDATE;
					entry.Applied = true;
					RepairLog.Add(entry);
				}
				break;
			}

			ResourceRepairLogEntry entry;
			entry.CandidateResourceNodeId = node.Id;
			entry.Operation = RESOURCE_REPAIR_ALTERNATE_FIELD_CANDIDATE;
			entry.Applied = false;
			RepairLog.Add(entry);
			claimed.Add(candidate);		// this one is illegal at every radius; do not offer it again
			attempts++;
		}
	}

	if (has_nod && (nod_node >= 0)) {
		int attempts = 0;
		while (attempts <= params.MaxRepairAttemptsPerField) {
			float length = 0.0f;
			int candidate = -1, best_node_index = -1;
			float best_length = -1.0f;
			for (int r = 0; r < resource_nodes.Count(); r++) {
				int rn = resource_nodes[r];
				bool already = false;
				for (int c = 0; c < claimed.Count(); c++) { if (claimed[c] == rn) { already = true; break; } }
				if (already) continue;
				if (!RoadGraphGenerator::Find_Anchor_Path(nod_node,rn,nullptr,&length)) continue;
				if ((best_length < 0.0f) || (length < best_length)) { best_length = length; best_node_index = rn; }
			}
			candidate = best_node_index;
			if (candidate < 0) break;

			StrategicNode node;
			StrategicPlanner::Get_Node(candidate,&node);
			bool needed_shrink = false;
			float radius = Resolve_Footprint_Radius(node.Position,params,
																	has_gdi,gdi_site.Position,gdi_site.ProtectedRadius,
																	has_nod,nod_site.Position,nod_site.ProtectedRadius,
																	&needed_shrink);

			if (radius > 0.0f) {
				nod_field_node = candidate;
				claimed.Add(candidate);
				if (attempts > 0) {
					ResourceRepairLogEntry entry;
					entry.CandidateResourceNodeId = node.Id;
					entry.Operation = RESOURCE_REPAIR_ALTERNATE_FIELD_CANDIDATE;
					entry.Applied = true;
					RepairLog.Add(entry);
				}
				break;
			}

			ResourceRepairLogEntry entry;
			entry.CandidateResourceNodeId = node.Id;
			entry.Operation = RESOURCE_REPAIR_ALTERNATE_FIELD_CANDIDATE;
			entry.Applied = false;
			RepairLog.Add(entry);
			claimed.Add(candidate);
			attempts++;
		}
	}

	//
	//	Resolve one field: shared body for the two initial picks above and every contested
	//	candidate below (Resolve_One_Field, a private static method -- see the header).
	//
	int crystal_index = 0;

	if (gdi_field_node >= 0) {
		Resolve_One_Field(gdi_field_node,PLAYERTYPE_GDI,gdi_node,params,
								 has_gdi,gdi_site.Position,gdi_site.ProtectedRadius,
								 has_nod,nod_site.Position,nod_site.ProtectedRadius,&crystal_index);
	}
	if (nod_field_node >= 0) {
		Resolve_One_Field(nod_field_node,PLAYERTYPE_NOD,nod_node,params,
								 has_gdi,gdi_site.Position,gdi_site.ProtectedRadius,
								 has_nod,nod_site.Position,nod_site.ProtectedRadius,&crystal_index);
	}

	//	Step 2: every remaining candidate is contested -- owned by whichever selected base's own
	//	road-graph route to it is shorter, a neutral field if neither base has one at all.
	for (int r = 0; r < resource_nodes.Count(); r++) {
		int rn = resource_nodes[r];
		bool already = false;
		for (int c = 0; c < claimed.Count(); c++) { if (claimed[c] == rn) { already = true; break; } }
		if (already) continue;

		float gdi_length = -1.0f, nod_length = -1.0f;
		if (has_gdi && (gdi_node >= 0)) RoadGraphGenerator::Find_Anchor_Path(gdi_node,rn,nullptr,&gdi_length);
		if (has_nod && (nod_node >= 0)) RoadGraphGenerator::Find_Anchor_Path(nod_node,rn,nullptr,&nod_length);

		//	Contested stays neutral regardless of which base is closer (step 2's own word); the
		//	nearer base's own node still seeds the field's informational route/income estimate,
		//	since a contested field with no route estimate at all would be a stranger omission
		//	than one whose estimate favours whichever side would actually reach it first.
		int owning_base_node = -1;
		if ((gdi_length >= 0.0f) && ((nod_length < 0.0f) || (gdi_length <= nod_length)))	owning_base_node = gdi_node;
		else if (nod_length >= 0.0f)																			owning_base_node = nod_node;

		Resolve_One_Field(rn,PLAYERTYPE_NEUTRAL,owning_base_node,params,
								 has_gdi,gdi_site.Position,gdi_site.ProtectedRadius,
								 has_nod,nod_site.Position,nod_site.ProtectedRadius,&crystal_index);
	}

	{
		CRCEngine crc;
		for (int i = 0; i < Fields.Count(); i++) {
			const ResolvedTiberiumField & f = Fields[i];
			int q_faction	= f.OwningFaction;
			int q_x			= (int)(f.Position.X * CHECKSUM_QUANTUM);
			int q_y			= (int)(f.Position.Y * CHECKSUM_QUANTUM);
			int q_radius	= (int)(f.Radius * CHECKSUM_QUANTUM);
			int q_yield		= (int)(f.InitialYield * CHECKSUM_QUANTUM);
			int q_crystals	= f.CrystalCount;
			crc(&q_faction,sizeof(q_faction));
			crc(&q_x,sizeof(q_x));
			crc(&q_y,sizeof(q_y));
			crc(&q_radius,sizeof(q_radius));
			crc(&q_yield,sizeof(q_yield));
			crc(&q_crystals,sizeof(q_crystals));
		}
		Checksum = (unsigned int)crc;
	}

	Built = true;

	WWDEBUG_SAY(("TiberiumFieldGenerator: %d field(s) resolved (%d crystal(s)), %d repair log entr%s, checksum 0x%08X\r\n",
					 Fields.Count(),Crystals.Count(),RepairLog.Count(),(RepairLog.Count() == 1) ? "y" : "ies",Checksum));

	#undef FAIL
	return true;
}


bool TiberiumFieldGenerator::Get_Field(int index,ResolvedTiberiumField * out)
{
	if ((out == nullptr) || (index < 0) || (index >= Fields.Count())) return false;
	*out = Fields[index];
	return true;
}

bool TiberiumFieldGenerator::Get_Crystal(int index,TiberiumCrystalPlacement * out)
{
	if ((out == nullptr) || (index < 0) || (index >= Crystals.Count())) return false;
	*out = Crystals[index];
	return true;
}

bool TiberiumFieldGenerator::Get_Repair_Log_Entry(int index,ResourceRepairLogEntry * out)
{
	if ((out == nullptr) || (index < 0) || (index >= RepairLog.Count())) return false;
	*out = RepairLog[index];
	return true;
}


/***********************************************************************************************
 * self-check accessors                                                                          *
 *=============================================================================================*/
int TiberiumFieldGenerator::Check_No_Illegal_Footprint(StringClass * report)
{
	int violations = 0;
	if (!Built) return violations;

	for (int i = 0; i < Fields.Count(); i++) {
		const ResolvedTiberiumField & f = Fields[i];
		TerrainPlacementInfoClass info;
		bool ok = WorldTerrainSystem::Query_Placement_Surface(f.Position,f.Radius,&info) && info.IsOnTerrain;
		if (!ok) {
			violations++;
			if (report) *report += "a resolved field's own footprint is not fully on terrain\n";
		}
	}
	return violations;
}

int TiberiumFieldGenerator::Check_Both_Factions_Have_Early_Field(StringClass * report)
{
	int violations = 0;
	if (!Built) return violations;

	bool gdi_has_early = false, nod_has_early = false;
	for (int i = 0; i < Fields.Count(); i++) {
		if (!Fields[i].CountsAsEarlyField) continue;
		if (Fields[i].OwningFaction == PLAYERTYPE_GDI) gdi_has_early = true;
		else if (Fields[i].OwningFaction == PLAYERTYPE_NOD) nod_has_early = true;
	}

	if (!gdi_has_early) {
		violations++;
		if (report) *report += "GDI has no viable early Tiberium field\n";
	}
	if (!nod_has_early) {
		violations++;
		if (report) *report += "Nod has no viable early Tiberium field\n";
	}
	return violations;
}

int TiberiumFieldGenerator::Check_Front_Travel_Time_Within_Threshold(float max_difference_seconds,
																							StringClass * report,float * difference_out)
{
	if (difference_out != nullptr) *difference_out = -1.0f;
	if (!Built || (LastReferenceVehicleSpeed <= 0.0f)) return 0;

	SelectedBaseSite gdi_site, nod_site;
	if (!BaseSelectionPlanner::Get_Selected_Site(PLAYERTYPE_GDI,&gdi_site)) return 0;
	if (!BaseSelectionPlanner::Get_Selected_Site(PLAYERTYPE_NOD,&nod_site)) return 0;

	int gdi_node = -1, nod_node = -1;
	if (!StrategicPlanner::Find_Node_By_Id(gdi_site.Id,&gdi_node)) return 0;
	if (!StrategicPlanner::Find_Node_By_Id(nod_site.Id,&nod_node)) return 0;

	float gdi_best = -1.0f, nod_best = -1.0f;
	for (int i = 0; i < Fields.Count(); i++) {
		const ResolvedTiberiumField & f = Fields[i];
		if (f.OwningFaction != PLAYERTYPE_NEUTRAL) continue;

		int field_node = -1;
		if (!StrategicPlanner::Find_Node_By_Id(f.ResourceNodeId,&field_node)) continue;

		float gdi_length = -1.0f, nod_length = -1.0f;
		if (!RoadGraphGenerator::Find_Anchor_Path(gdi_node,field_node,nullptr,&gdi_length)) continue;
		if (!RoadGraphGenerator::Find_Anchor_Path(nod_node,field_node,nullptr,&nod_length)) continue;

		//	Only a field both sides can reach is a front.
		if ((gdi_best < 0.0f) || (gdi_length < gdi_best)) gdi_best = gdi_length;
		if ((nod_best < 0.0f) || (nod_length < nod_best)) nod_best = nod_length;
	}

	if ((gdi_best < 0.0f) || (nod_best < 0.0f)) return 0;

	float difference = WWMath::Fabs(gdi_best - nod_best) / LastReferenceVehicleSpeed;
	if (difference_out != nullptr) *difference_out = difference;

	if (difference > max_difference_seconds) {
		if (report != nullptr) {
			StringClass line;
			line.Format("base-to-front travel times differ by %.1f s (limit %.1f s)\n",difference,max_difference_seconds);
			*report += line;
		}
		return 1;
	}
	return 0;
}


int TiberiumFieldGenerator::Check_Harvestable_Fields_Have_Routes(StringClass * report)
{
	int violations = 0;
	if (!Built) return violations;

	for (int i = 0; i < Fields.Count(); i++) {
		const ResolvedTiberiumField & f = Fields[i];
		if (f.OwningFaction == PLAYERTYPE_NEUTRAL) continue;		// contested fields are not owned-route-guaranteed
		if (!f.HasHarvesterRoute) {
			violations++;
			if (report) *report += "an owned field has no resolved Harvester route\n";
		}
	}
	return violations;
}
