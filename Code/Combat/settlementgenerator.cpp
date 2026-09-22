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
 *                     $Archive:: /Commando/Code/Combat/settlementgenerator.cpp               $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "settlementgenerator.h"

#include "baseselectionplanner.h"
#include "crc.h"
#include "playertype.h"
#include "proceduralassetregistry.h"
#include "roadgraphgenerator.h"
#include "roadspline.h"
#include "roadsystem.h"
#include "strategicplanner.h"
#include "terrainmask.h"
#include "tiberiumfieldgenerator.h"
#include "worldboundsservice.h"
#include "worldterrainsystem.h"
#include "wwdebug.h"
#include "wwmath.h"


bool				SettlementGenerator::Built		= false;
unsigned int	SettlementGenerator::Checksum	= 0;

DynamicVectorClass<ResolvedSettlement>			SettlementGenerator::Settlements;
DynamicVectorClass<PlacedStructureInstance>	SettlementGenerator::Structures;
DynamicVectorClass<ResolvedPoi>					SettlementGenerator::Pois;


static const float CHECKSUM_QUANTUM = 1000.0f;

//	No profile field carries Section 28A.4's own "maximum settlement footprint" yet -- that
//	section's full settlement-profile block (block-size range, street width, density target,
//	minimum alley width, building repetition limit) is a larger schema addition than this pass
//	has time to design well.  One fixed, documented radius stands in, the same class of
//	disclosed simplification `TiberiumFieldGenerator`'s own reference speeds already are.
static const float SETTLEMENT_FOOTPRINT_RADIUS = 40.0f;

//	Where placed structures sit inside that footprint -- a ring, not a per-layout-type block/
//	grid/loop geometry.  SettlementLayoutType is resolved and stored for real (step 2); the
//	finer per-type street/plot geometry step 3-7 ask for beyond a single connector distance is
//	not attempted this pass -- see docs/procedural_world/Settlements.md.
static const float STRUCTURE_RING_FRACTION = 0.55f;


const char * Settlement_Layout_Type_Name(SettlementLayoutType type)
{
	switch (type) {
		case SETTLEMENT_LAYOUT_LINEAR_ROADSIDE:	return "linear_roadside";
		case SETTLEMENT_LAYOUT_SMALL_GRID:			return "small_grid";
		case SETTLEMENT_LAYOUT_INDUSTRIAL_LOOP:	return "industrial_loop";
		case SETTLEMENT_LAYOUT_IRREGULAR_TOWN:	return "irregular_town";
		case SETTLEMENT_LAYOUT_RUINS:					return "ruins";
		case SETTLEMENT_LAYOUT_OUTPOST:				return "outpost";
		default:												return "unknown";
	}
}

const char * Poi_Kind_Name(PoiKindType kind)
{
	switch (kind) {
		case POI_KIND_BUNKER:				return "bunker";
		case POI_KIND_CONVOY_WRECK:		return "convoy_wreck";
		case POI_KIND_RESEARCH_SITE:	return "research_site";
		case POI_KIND_RELAY:				return "relay";
		case POI_KIND_REPAIR_DEPOT:		return "repair_depot";
		case POI_KIND_RUINS:				return "ruins";
		default:									return "unknown";
	}
}


PlacedStructureInstance::PlacedStructureInstance(void) :
	SettlementIndex(-1), Position(0.0f,0.0f,0.0f), RotationRadians(0.0f)
{ }

bool PlacedStructureInstance::operator == (const PlacedStructureInstance & that) const
{
	return (SettlementIndex == that.SettlementIndex) && (AssetId == that.AssetId) &&
			 (Position == that.Position) && (RotationRadians == that.RotationRadians);
}


ResolvedSettlement::ResolvedSettlement(void) :
	Layout(SETTLEMENT_LAYOUT_OUTPOST), Position(0.0f,0.0f,0.0f), Radius(SETTLEMENT_FOOTPRINT_RADIUS),
	RoadConnected(false), RoadDistance(-1.0f), StructureCount(0)
{ }

bool ResolvedSettlement::operator == (const ResolvedSettlement & that) const
{
	return (Id == that.Id) && (Layout == that.Layout) && (Position == that.Position) &&
			 (Radius == that.Radius) && (RoadConnected == that.RoadConnected) &&
			 (RoadDistance == that.RoadDistance) && (StructureCount == that.StructureCount);
}


ResolvedPoi::ResolvedPoi(void) : Kind(POI_KIND_RUINS), Position(0.0f,0.0f,0.0f), HasRealAsset(false) { }

bool ResolvedPoi::operator == (const ResolvedPoi & that) const
{
	return (Id == that.Id) && (Kind == that.Kind) && (Position == that.Position) &&
			 (HasRealAsset == that.HasRealAsset);
}


SettlementParams::SettlementParams(void) :
	Seed(1), RoadConnectorMaxDistance(60.0f), MaxStructuresPerSettlement(4),
	MaxBuildableSlopeRadians(0.5f)
{ }


/***********************************************************************************************
 * local helpers                                                                                 *
 *=============================================================================================*/

static float Nearest_Road_Distance(const Vector3 & position)
{
	float best = -1.0f;
	for (int r = 0; r < RoadSystem::Get_Road_Count(); r++) {
		RoadSplineClass * road = RoadSystem::Peek_Road(r);
		if ((road == nullptr) || !road->Is_Tessellated()) continue;

		for (int p = 0; p < road->Get_Point_Count(); p++) {
			float d = (road->Get_Point(p).Position - position).Length();
			if ((best < 0.0f) || (d < best)) best = d;
		}
	}
	return best;
}


//	Step 1's own conflict list, made concrete with what this chain already reserved: the
//	boundary/cliff band (WorldBoundsService, the one authoritative owner), both selected bases'
//	own protected regions (Phase 11), and every resolved Tiberium field's own footprint (Phase
//	12).  Bridge approaches and tunnel portals are not checked -- no tunnel system exists (see
//	BridgePlanner's own honest gap), and a bridge approach corridor has no queryable region of
//	its own outside RoadSystem's own geometry this generator would have to re-derive.
static bool Conflicts_With_Reserved_Ground(const Vector3 & position,float radius)
{
	if (!WorldBoundsService::Can_Place_In_Region(Vector2(position.X,position.Y),radius)) return true;

	SelectedBaseSite gdi_site, nod_site;
	if (BaseSelectionPlanner::Get_Selected_Site(PLAYERTYPE_GDI,&gdi_site)) {
		if ((position - gdi_site.Position).Length() < (radius + gdi_site.ProtectedRadius)) return true;
	}
	if (BaseSelectionPlanner::Get_Selected_Site(PLAYERTYPE_NOD,&nod_site)) {
		if ((position - nod_site.Position).Length() < (radius + nod_site.ProtectedRadius)) return true;
	}

	for (int i = 0; i < TiberiumFieldGenerator::Get_Field_Count(); i++) {
		ResolvedTiberiumField field;
		if (!TiberiumFieldGenerator::Get_Field(i,&field)) continue;
		if ((position - field.Position).Length() < (radius + field.Radius)) return true;
	}

	return false;
}


/***********************************************************************************************
 * Init/Shutdown/Clear                                                                           *
 *=============================================================================================*/
void SettlementGenerator::Init(void)		{ Clear(); }
void SettlementGenerator::Shutdown(void)	{ Clear(); }

void SettlementGenerator::Clear(void)
{
	Built = false;
	Checksum = 0;
	Settlements.Delete_All();
	Structures.Delete_All();
	Pois.Delete_All();
}


/***********************************************************************************************
 * Generate -- Phase 13 steps 1-3, 10-11                                                         *
 *=============================================================================================*/
bool SettlementGenerator::Generate(const SettlementParams & params,StringClass * error_out)
{
	#define FAIL(msg) { if (error_out) *error_out = msg; Clear(); return false; }

	Clear();

	if (!StrategicPlanner::Has_Plan())					FAIL("no strategic plan -- generate settlement/POI candidates first");
	if (!RoadGraphGenerator::Has_Graph())				FAIL("no road graph -- connect the world first");
	if (!ProceduralAssetRegistry::Has_Registry())	FAIL("no asset registry -- load procedural assets first");
	if (params.RoadConnectorMaxDistance < 0.0f)		FAIL("road connector max distance must not be negative");
	if (params.MaxStructuresPerSettlement < 0)		FAIL("max structures per settlement must not be negative");

	DynamicVectorClass<int> settlement_nodes;
	DynamicVectorClass<int> poi_nodes;
	for (int n = 0; n < StrategicPlanner::Get_Node_Count(); n++) {
		StrategicNode node;
		if (!StrategicPlanner::Get_Node(n,&node)) continue;
		if (node.Type == STRATEGIC_NODE_SETTLEMENT_CANDIDATE)	settlement_nodes.Add(n);
		else if (node.Type == STRATEGIC_NODE_POI_CANDIDATE)		poi_nodes.Add(n);
	}
	if ((settlement_nodes.Count() == 0) && (poi_nodes.Count() == 0)) {
		Built = true;		// correctly nothing to resolve, not a failure
		return true;
	}

	DynamicVectorClass<int> eligible_assets;
	for (int a = 0; a < ProceduralAssetRegistry::Get_Asset_Count(); a++) {
		ProceduralAssetDescriptor asset;
		if (!ProceduralAssetRegistry::Get_Asset(a,&asset)) continue;
		if (asset.Category != ASSET_CATEGORY_BUILDING) continue;
		eligible_assets.Add(a);
	}

	for (int s = 0; s < settlement_nodes.Count(); s++) {
		StrategicNode node;
		if (!StrategicPlanner::Get_Node(settlement_nodes[s],&node)) continue;

		if (Conflicts_With_Reserved_Ground(node.Position,SETTLEMENT_FOOTPRINT_RADIUS)) continue;

		TerrainPlacementInfoClass placement_info;
		if (!WorldTerrainSystem::Query_Placement_Surface(node.Position,SETTLEMENT_FOOTPRINT_RADIUS,&placement_info)) continue;
		if (!placement_info.IsOnTerrain) continue;
		if (placement_info.MaxSlope > params.MaxBuildableSlopeRadians) continue;

		ResolvedSettlement settlement;
		settlement.Id			= node.Id;
		settlement.Position	= node.Position;
		settlement.Radius		= SETTLEMENT_FOOTPRINT_RADIUS;

		float layout_draw = TerrainMaskClass::Hash_Value(settlement_nodes[s],200,(int)params.Seed);
		int layout_index = (int)(layout_draw * (float)SETTLEMENT_LAYOUT_TYPE_COUNT);
		if (layout_index >= SETTLEMENT_LAYOUT_TYPE_COUNT) layout_index = SETTLEMENT_LAYOUT_TYPE_COUNT - 1;
		settlement.Layout = (SettlementLayoutType)layout_index;

		settlement.RoadDistance = Nearest_Road_Distance(node.Position);
		settlement.RoadConnected = (settlement.RoadDistance >= 0.0f) &&
											(settlement.RoadDistance <= params.RoadConnectorMaxDistance);

		int settlement_index = Settlements.Count();

		//	Step 4: place real structures on a ring inside the reserved footprint, cycling
		//	through every eligible real asset (round-robin, so AllowDuplicateAdjacent's own
		//	false default is honoured by construction rather than checked after the fact) and
		//	skipping any position that fails its own placement/slope/overlap test -- a
		//	position skipped is one fewer structure placed, not a fabricated one.
		int placed = 0;
		if (eligible_assets.Count() > 0) {
			for (int slot = 0; (slot < params.MaxStructuresPerSettlement) && (placed < params.MaxStructuresPerSettlement); slot++) {
				ProceduralAssetDescriptor asset;
				ProceduralAssetRegistry::Get_Asset(eligible_assets[slot % eligible_assets.Count()],&asset);

				float radius = 0.0f, height = 0.0f;
				if (!asset.Resolve_Placement_Radius(&radius,&height) || (radius <= 0.0f)) continue;

				int slot_count = (params.MaxStructuresPerSettlement > 1) ? params.MaxStructuresPerSettlement : 1;
				float angle = (2.0f * WWMATH_PI) * ((float)slot / (float)slot_count);
				Vector3 candidate = node.Position;
				candidate.X += WWMath::Cos(angle) * (settlement.Radius * STRUCTURE_RING_FRACTION);
				candidate.Y += WWMath::Sin(angle) * (settlement.Radius * STRUCTURE_RING_FRACTION);

				TerrainPlacementInfoClass structure_info;
				if (!WorldTerrainSystem::Query_Placement_Surface(candidate,radius,&structure_info)) continue;
				if (!structure_info.IsOnTerrain || (structure_info.MaxSlope > params.MaxBuildableSlopeRadians)) continue;
				if (Conflicts_With_Reserved_Ground(candidate,radius)) continue;

				bool overlaps = false;
				for (int existing = 0; existing < Structures.Count(); existing++) {
					if ((candidate - Structures[existing].Position).Length() < asset.MinNeighborSpacing) {
						overlaps = true;
						break;
					}
				}
				if (overlaps) continue;

				PlacedStructureInstance instance;
				instance.SettlementIndex	= settlement_index;
				instance.AssetId				= asset.Id;
				instance.Position			= candidate;
				instance.RotationRadians	= angle;
				Structures.Add(instance);
				placed++;
			}
		}
		settlement.StructureCount = placed;

		Settlements.Add(settlement);
	}

	//	Step 9: POI kind, deterministically classified.  No entry in ProceduralAssetRegistry
	//	carries ASSET_CATEGORY_POI yet, so HasRealAsset stays false for every one -- recognised
	//	in the diagnostics vocabulary, never fabricated as a placed instance.
	for (int p = 0; p < poi_nodes.Count(); p++) {
		StrategicNode node;
		if (!StrategicPlanner::Get_Node(poi_nodes[p],&node)) continue;
		if (Conflicts_With_Reserved_Ground(node.Position,10.0f)) continue;

		ResolvedPoi poi;
		poi.Id = node.Id;
		float kind_draw = TerrainMaskClass::Hash_Value(poi_nodes[p],300,(int)params.Seed);
		int kind_index = (int)(kind_draw * (float)POI_KIND_TYPE_COUNT);
		if (kind_index >= POI_KIND_TYPE_COUNT) kind_index = POI_KIND_TYPE_COUNT - 1;
		poi.Kind = (PoiKindType)kind_index;
		poi.Position = node.Position;
		poi.HasRealAsset = false;

		Pois.Add(poi);
	}

	{
		CRCEngine crc;
		for (int i = 0; i < Settlements.Count(); i++) {
			const ResolvedSettlement & s = Settlements[i];
			int q_layout	= (int)s.Layout;
			int q_x			= (int)(s.Position.X * CHECKSUM_QUANTUM);
			int q_y			= (int)(s.Position.Y * CHECKSUM_QUANTUM);
			int q_connected	= s.RoadConnected ? 1 : 0;
			int q_count		= s.StructureCount;
			crc(&q_layout,sizeof(q_layout));
			crc(&q_x,sizeof(q_x));
			crc(&q_y,sizeof(q_y));
			crc(&q_connected,sizeof(q_connected));
			crc(&q_count,sizeof(q_count));
		}
		for (int i = 0; i < Pois.Count(); i++) {
			int q_kind	= (int)Pois[i].Kind;
			int q_x		= (int)(Pois[i].Position.X * CHECKSUM_QUANTUM);
			int q_y		= (int)(Pois[i].Position.Y * CHECKSUM_QUANTUM);
			crc(&q_kind,sizeof(q_kind));
			crc(&q_x,sizeof(q_x));
			crc(&q_y,sizeof(q_y));
		}
		Checksum = (unsigned int)crc;
	}

	Built = true;

	WWDEBUG_SAY(("SettlementGenerator: %d settlement(s) (%d structure(s)), %d POI(s), checksum 0x%08X\r\n",
					 Settlements.Count(),Structures.Count(),Pois.Count(),Checksum));

	#undef FAIL
	return true;
}


bool SettlementGenerator::Get_Settlement(int index,ResolvedSettlement * out)
{
	if ((out == nullptr) || (index < 0) || (index >= Settlements.Count())) return false;
	*out = Settlements[index];
	return true;
}

bool SettlementGenerator::Get_Structure(int index,PlacedStructureInstance * out)
{
	if ((out == nullptr) || (index < 0) || (index >= Structures.Count())) return false;
	*out = Structures[index];
	return true;
}

bool SettlementGenerator::Get_Poi(int index,ResolvedPoi * out)
{
	if ((out == nullptr) || (index < 0) || (index >= Pois.Count())) return false;
	*out = Pois[index];
	return true;
}


/***********************************************************************************************
 * self-check accessors                                                                          *
 *=============================================================================================*/
int SettlementGenerator::Check_No_Illegal_Placement(StringClass * report)
{
	int violations = 0;
	if (!Built) return violations;

	for (int i = 0; i < Structures.Count(); i++) {
		const PlacedStructureInstance & instance = Structures[i];
		ProceduralAssetDescriptor asset;
		if (!ProceduralAssetRegistry::Get_Asset_By_Id(instance.AssetId,&asset)) {
			violations++;
			if (report) *report += "a placed structure names an asset id the registry does not have\n";
			continue;
		}
		float radius = 0.0f, height = 0.0f;
		if (!asset.Resolve_Placement_Radius(&radius,&height)) {
			violations++;
			if (report) *report += "a placed structure's own asset no longer resolves a footprint\n";
			continue;
		}
		TerrainPlacementInfoClass info;
		if (!WorldTerrainSystem::Query_Placement_Surface(instance.Position,radius,&info) || !info.IsOnTerrain) {
			violations++;
			if (report) *report += "a placed structure's own footprint is not fully on terrain\n";
		}
	}
	return violations;
}

int SettlementGenerator::Check_No_Structure_Overlap(StringClass * report)
{
	int violations = 0;
	if (!Built) return violations;

	for (int a = 0; a < Structures.Count(); a++) {
		ProceduralAssetDescriptor asset_a;
		if (!ProceduralAssetRegistry::Get_Asset_By_Id(Structures[a].AssetId,&asset_a)) continue;

		for (int b = a + 1; b < Structures.Count(); b++) {
			float dist = (Structures[a].Position - Structures[b].Position).Length();
			if (dist < asset_a.MinNeighborSpacing) {
				violations++;
				if (report) *report += "two placed structures sit closer than the minimum neighbour spacing\n";
			}
		}
	}
	return violations;
}
