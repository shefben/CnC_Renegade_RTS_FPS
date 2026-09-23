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
 *                     $Archive:: /Commando/Code/Combat/biomedecorationgenerator.cpp          $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "biomedecorationgenerator.h"

#include "baseselectionplanner.h"
#include "crc.h"
#include "foliagesystem.h"
#include "foliagetype.h"
#include "playertype.h"
#include "proceduralassetregistry.h"
#include "roadspline.h"
#include "roadsystem.h"
#include "settlementgenerator.h"
#include "terrainmask.h"
#include "terraintexturesystem.h"
#include "tunnelsystem.h"
#include "watersystem.h"
#include "worldboundsservice.h"
#include "worldterrainsystem.h"
#include "wwdebug.h"
#include "wwmath.h"


bool				BiomeDecorationGenerator::Built	= false;
unsigned int	BiomeDecorationGenerator::Checksum	= 0;

int	BiomeDecorationGenerator::TreeCount			= 0;
int	BiomeDecorationGenerator::RockCount			= 0;
int	BiomeDecorationGenerator::ForestChunkCount	= 0;
int	BiomeDecorationGenerator::StreetLampCount	= 0;


static const float CHECKSUM_QUANTUM = 1000.0f;

//	A value at or above this on a mask means "occupied by that feature" -- the same threshold
//	SettlementGenerator's own road check already uses.
static const float MASK_OCCUPIED_THRESHOLD = 0.5f;

//	No profile field carries a canonical water-distance falloff range yet; this is
//	WaterSystem::Update_Water_Distance's own saturating max_distance argument, read back here as
//	a fixed constant since this generator has no other way to learn what it was built with.
static const float WATER_DISTANCE_SATURATION = 120.0f;

static const int TREES_PER_FOREST_CHUNK_AT_FULL_DENSITY	= 6;
static const int ROCKS_PER_CHUNK_AT_FULL_DENSITY				= 2;


BiomeDecorationParams::BiomeDecorationParams(void) :
	Seed(1), MaxBuildableSlopeRadians(0.5f), ForestDensity(0.5f), RockDensity(0.2f), StreetLampSpacing(30.0f)
{ }

//	How far outside a road's own total width (carriageway plus both shoulders) a lamp stands, so
//	the pole never sits on a lane or a shoulder (Phase 14 step 10).
static const float STREET_LAMP_SETBACK = 0.5f;


/***********************************************************************************************
 * local helpers                                                                                 *
 *=============================================================================================*/

static bool Reads_As_Occupied(TerrainMaskType mask_type,float x,float y)
{
	if (!TerrainTextureSystem::Has_Masks()) return false;
	TerrainMaskClass * mask = TerrainTextureSystem::Peek_Mask(mask_type);
	if (mask == nullptr) return false;
	float v = 0.0f;
	return mask->Sample(x,y,&v) && (v >= MASK_OCCUPIED_THRESHOLD);
}

static bool Conflicts_With_Reservations(const Vector3 & position)
{
	SelectedBaseSite gdi_site, nod_site;
	if (BaseSelectionPlanner::Get_Selected_Site(PLAYERTYPE_GDI,&gdi_site)) {
		if ((position - gdi_site.Position).Length() < gdi_site.ProtectedRadius) return true;
	}
	if (BaseSelectionPlanner::Get_Selected_Site(PLAYERTYPE_NOD,&nod_site)) {
		if ((position - nod_site.Position).Length() < nod_site.ProtectedRadius) return true;
	}
	for (int i = 0; i < SettlementGenerator::Get_Settlement_Count(); i++) {
		ResolvedSettlement settlement;
		if (!SettlementGenerator::Get_Settlement(i,&settlement)) continue;
		if ((position - settlement.Position).Length() < settlement.Radius) return true;
	}
	return false;
}

//	A legal decoration point: on terrain, dry, and clear of every reserved/occupied region above
//	-- checked per candidate point, not merely per chunk, since a chunk this generator considers
//	eligible can still contain one corner that is not.
static bool Is_Legal_Decoration_Point(const Vector3 & position)
{
	float height = 0.0f;
	if (!WorldTerrainSystem::Sample_Height(position.X,position.Y,&height)) return false;
	if (WaterSystem::Is_Point_In_Water(position.X,position.Y)) return false;
	if (Reads_As_Occupied(TERRAIN_MASK_ROAD,position.X,position.Y)) return false;
	if (Reads_As_Occupied(TERRAIN_MASK_TIBERIUM,position.X,position.Y)) return false;
	if (Conflicts_With_Reservations(position)) return false;
	//	Section 12A.3: nothing grows in a tunnel mouth or its opened approach.
	if (TunnelSystem::Overlaps_Opening(position,0.0f,nullptr)) return false;
	return true;
}


/***********************************************************************************************
 * Init/Shutdown/Clear                                                                           *
 *=============================================================================================*/
void BiomeDecorationGenerator::Init(void)		{ Clear(); }
void BiomeDecorationGenerator::Shutdown(void)	{ Clear(); }

void BiomeDecorationGenerator::Clear(void)
{
	Built = false;
	Checksum = 0;
	TreeCount = 0;
	RockCount = 0;
	ForestChunkCount = 0;
	StreetLampCount = 0;
	FoliageSystem::Clear_Instances();
}


/***********************************************************************************************
 * Generate -- Phase 14 steps 1, 3-8, 13                                                         *
 *=============================================================================================*/
bool BiomeDecorationGenerator::Generate(const BiomeDecorationParams & params,StringClass * error_out)
{
	#define FAIL(msg) { if (error_out) *error_out = msg; Clear(); return false; }

	Clear();

	if (!WorldTerrainSystem::Has_Chunk_Registry())	FAIL("no terrain chunk registry -- generate macro terrain first");
	if ((params.ForestDensity < 0.0f) || (params.ForestDensity > 1.0f))	FAIL("forest density must be within 0..1");
	if ((params.RockDensity < 0.0f) || (params.RockDensity > 1.0f))			FAIL("rock density must be within 0..1");

	int count_x = WorldTerrainSystem::Get_Chunk_Count_X();
	int count_y = WorldTerrainSystem::Get_Chunk_Count_Y();
	if ((count_x <= 0) || (count_y <= 0))	FAIL("chunk registry is empty");

	for (int py = 0; py < count_y; py++) {
		for (int px = 0; px < count_x; px++) {

			TerrainChunkInfoClass info;
			if (!WorldTerrainSystem::Get_Chunk_Info(px,py,&info)) continue;
			if (!info.Buildable) continue;		// excludes cliff band, interior cliffs and excessive slope already

			AABoxClass bounds;
			if (!WorldTerrainSystem::Get_Patch_Bounds(px,py,&bounds)) continue;
			Vector3 center = bounds.Center;
			float chunk_radius = WWMath::Min(bounds.Extent.X,bounds.Extent.Y);
			if (chunk_radius <= 0.0f) continue;

			if (Reads_As_Occupied(TERRAIN_MASK_ROAD,center.X,center.Y)) continue;
			if (Reads_As_Occupied(TERRAIN_MASK_TIBERIUM,center.X,center.Y)) continue;
			if (Conflicts_With_Reservations(center)) continue;

			//	Step 1's own biome field, made concrete: moisture from the real water-distance
			//	mask WaterSystem already writes, folded with a deterministic hash so an otherwise
			//	uniform ring around a river does not become a uniform ring of forest.
			float moisture = 0.0f;
			if (TerrainTextureSystem::Has_Masks()) {
				TerrainMaskClass * water_distance = TerrainTextureSystem::Peek_Mask(TERRAIN_MASK_WATER_DISTANCE);
				float distance = WATER_DISTANCE_SATURATION;
				if ((water_distance != nullptr) && water_distance->Sample(center.X,center.Y,&distance)) {
					moisture = 1.0f - WWMath::Clamp(distance / WATER_DISTANCE_SATURATION,0.0f,1.0f);
				}
			}

			int chunk_key = px + (py * count_x);
			float jitter = TerrainMaskClass::Hash_Value(chunk_key,0,(int)params.Seed);
			float forest_suitability = (moisture * 0.6f) + (jitter * 0.4f);

			//	A chunk near the top of the buildable slope range reads as scree/rock terrain
			//	rather than forest -- a real, differentiated signal from the same chunk registry
			//	data every other generator in this chain already reads, not a second guess at it.
			float slope_fraction = (params.MaxBuildableSlopeRadians > 0.0f) ?
				WWMath::Clamp(info.MaxSlopeRadians / params.MaxBuildableSlopeRadians,0.0f,1.0f) : 0.0f;

			//	0.15 rather than a rounder-looking 0.4/0.5: with no water-distance mask built yet
			//	(a level with no terrain-texture masks at all), moisture stays 0 and the jitter
			//	term alone -- weighted 0.4 -- can never exceed 0.4, so a higher bar would make
			//	forest placement structurally impossible rather than merely rare on such a level.
			bool is_forest_chunk = (params.ForestDensity > 0.0f) && (forest_suitability > 0.15f) && (slope_fraction < 0.7f);
			bool is_rocky_chunk	 = (params.RockDensity > 0.0f) && (slope_fraction >= 0.5f);

			if (is_forest_chunk) {
				ForestChunkCount++;
				int tree_count_raw = (int)(TREES_PER_FOREST_CHUNK_AT_FULL_DENSITY * params.ForestDensity);
				int tree_count = (tree_count_raw > 0) ? tree_count_raw : 1;
				for (int t = 0; t < tree_count; t++) {
					float angle_draw	= TerrainMaskClass::Hash_Value(chunk_key,(t * 2) + 500,(int)params.Seed);
					float radius_draw	= TerrainMaskClass::Hash_Value(chunk_key,(t * 2) + 501,(int)params.Seed);
					float angle = angle_draw * 2.0f * WWMATH_PI;
					float radius = WWMath::Sqrt(radius_draw) * chunk_radius;

					Vector3 candidate = center;
					candidate.X += WWMath::Cos(angle) * radius;
					candidate.Y += WWMath::Sin(angle) * radius;
					if (!Is_Legal_Decoration_Point(candidate)) continue;

					//	Two of every five plantings are the dead-tree/scrub variety rather than a
					//	uniform conifer stand -- a real, deterministic mix, not every tree alike.
					float variety_draw = TerrainMaskClass::Hash_Value(chunk_key,(t * 2) + 502,(int)params.Seed);
					const char * type_name = (variety_draw < 0.7f) ? "ow_tree_conifer" :
													  (variety_draw < 0.9f) ? "ow_tree_dead" : "ow_bush_scrub";

					float yaw_draw = TerrainMaskClass::Hash_Value(chunk_key,(t * 2) + 503,(int)params.Seed);
					if (FoliageSystem::Add_Instance(type_name,candidate,yaw_draw * 2.0f * WWMATH_PI,1.0f) >= 0) {
						TreeCount++;
					}
				}
			}

			if (is_rocky_chunk) {
				int rock_count_raw = (int)(ROCKS_PER_CHUNK_AT_FULL_DENSITY * params.RockDensity);
				int rock_count = (rock_count_raw > 0) ? rock_count_raw : 1;
				for (int r = 0; r < rock_count; r++) {
					float angle_draw	= TerrainMaskClass::Hash_Value(chunk_key,(r * 2) + 700,(int)params.Seed);
					float radius_draw	= TerrainMaskClass::Hash_Value(chunk_key,(r * 2) + 701,(int)params.Seed);
					float angle = angle_draw * 2.0f * WWMATH_PI;
					float radius = WWMath::Sqrt(radius_draw) * chunk_radius;

					Vector3 candidate = center;
					candidate.X += WWMath::Cos(angle) * radius;
					candidate.Y += WWMath::Sin(angle) * radius;
					if (!Is_Legal_Decoration_Point(candidate)) continue;

					float yaw_draw = TerrainMaskClass::Hash_Value(chunk_key,(r * 2) + 702,(int)params.Seed);
					if (FoliageSystem::Add_Instance("ow_rock_boulder",candidate,yaw_draw * 2.0f * WWMATH_PI,1.0f) >= 0) {
						RockCount++;
					}
				}
			}
		}
	}

	//	Steps 9-10: street lamps, before batching so they batch and cull with everything else.
	Place_Street_Lamps(params);

	//	Step 13's own real LOD/batching acceptance -- already-built primitives, not fabricated.
	FoliageSystem::Build_Cells();
	FoliageSystem::Build_Batches();

	{
		CRCEngine crc;
		for (int i = 0; i < FoliageSystem::Get_Instance_Count(); i++) {
			const FoliageInstanceClass * instance = FoliageSystem::Peek_Instance(i);
			if (instance == nullptr) continue;
			int q_type	= instance->Type;
			int q_x		= (int)(instance->Position.X * CHECKSUM_QUANTUM);
			int q_y		= (int)(instance->Position.Y * CHECKSUM_QUANTUM);
			crc(&q_type,sizeof(q_type));
			crc(&q_x,sizeof(q_x));
			crc(&q_y,sizeof(q_y));
		}
		Checksum = (unsigned int)crc;
	}

	Built = true;

	WWDEBUG_SAY(("BiomeDecorationGenerator: %d forest chunk(s), %d tree(s), %d rock(s), %d street lamp(s), checksum 0x%08X\r\n",
					 ForestChunkCount,TreeCount,RockCount,StreetLampCount,Checksum));

	#undef FAIL
	return true;
}


/***********************************************************************************************
 * Place_Street_Lamps -- Phase 14 steps 9-10, Section 10.6                                       *
 *                                                                                             *
 *	Section 10.6 lists four places for lamps:                                                    *
 *	  - city streets, at the full spacing;                                                        *
 *	  - "selected" highways, at double the spacing;                                               *
 *	  - base approach roads: any road's stretch within one spacing of a selected base's           *
 *	    protected region;                                                                          *
 *	  - industrial areas: any road's stretch inside an INDUSTRIAL_LOOP settlement.                *
 *	Lamps stand beside the road, outside its total width (carriageway plus shoulders), and         *
 *	alternate sides.  They stand only where ordinary decoration may stand, and clear of every      *
 *	settlement structure, so they never block a lane, a shoulder, a structure or a tunnel mouth.   *
 *	The spacing walk is deterministic: stations fall at fixed distances along each road's own      *
 *	tessellated centre line.                                                                       *
 *=============================================================================================*/
static bool Near_Selected_Base(const Vector3 & p,float reach)
{
	SelectedBaseSite site;
	int factions[2] = { PLAYERTYPE_GDI, PLAYERTYPE_NOD };
	for (int f = 0; f < 2; f++) {
		if (!BaseSelectionPlanner::Get_Selected_Site(factions[f],&site)) continue;
		if ((p - site.Position).Length() < (site.ProtectedRadius + reach)) return true;
	}
	return false;
}


static bool In_Industrial_Settlement(const Vector3 & p)
{
	for (int i = 0; i < SettlementGenerator::Get_Settlement_Count(); i++) {
		ResolvedSettlement settlement;
		if (!SettlementGenerator::Get_Settlement(i,&settlement)) continue;
		if (settlement.Layout != SETTLEMENT_LAYOUT_INDUSTRIAL_LOOP) continue;
		if ((p - settlement.Position).Length() < settlement.Radius) return true;
	}
	return false;
}


static bool Is_Legal_Lamp_Point(const Vector3 & p)
{
	float height = 0.0f;
	if (!WorldTerrainSystem::Sample_Height(p.X,p.Y,&height)) return false;
	if (!WorldBoundsService::Can_Place_In_Region(Vector2(p.X,p.Y),0.5f)) return false;
	if (WaterSystem::Is_Point_In_Water(p.X,p.Y)) return false;
	if (Reads_As_Occupied(TERRAIN_MASK_ROAD,p.X,p.Y)) return false;
	if (Reads_As_Occupied(TERRAIN_MASK_TIBERIUM,p.X,p.Y)) return false;
	if (TunnelSystem::Overlaps_Opening(p,0.5f,nullptr)) return false;

	//	Inside a base's own protected region the base layout decides what stands; lamps line the
	//	approach to it, not the yard.
	SelectedBaseSite site;
	int factions[2] = { PLAYERTYPE_GDI, PLAYERTYPE_NOD };
	for (int f = 0; f < 2; f++) {
		if (BaseSelectionPlanner::Get_Selected_Site(factions[f],&site) &&
			 ((p - site.Position).Length() < site.ProtectedRadius)) return false;
	}

	//	Inside a settlement only an industrial one takes lamps, and never on a structure.
	for (int i = 0; i < SettlementGenerator::Get_Settlement_Count(); i++) {
		ResolvedSettlement settlement;
		if (!SettlementGenerator::Get_Settlement(i,&settlement)) continue;
		if ((p - settlement.Position).Length() >= settlement.Radius) continue;
		if (settlement.Layout != SETTLEMENT_LAYOUT_INDUSTRIAL_LOOP) return false;
	}
	for (int i = 0; i < SettlementGenerator::Get_Structure_Count(); i++) {
		PlacedStructureInstance structure;
		if (!SettlementGenerator::Get_Structure(i,&structure)) continue;
		float radius = 0.0f, height_unused = 0.0f;
		ProceduralAssetDescriptor asset;
		if (!ProceduralAssetRegistry::Get_Asset_By_Id(structure.AssetId,&asset) ||
			 !asset.Resolve_Placement_Radius(&radius,&height_unused)) {
			radius = 0.0f;
		}
		if ((p - structure.Position).Length() < (radius + 1.0f)) return false;
	}
	return true;
}


void BiomeDecorationGenerator::Place_Street_Lamps(const BiomeDecorationParams & params)
{
	if (params.StreetLampSpacing <= 0.0f) {
		return;
	}

	for (int r = 0; r < RoadSystem::Get_Road_Count(); r++) {

		RoadSplineClass * road = RoadSystem::Peek_Road(r);
		if ((road == nullptr) || !road->Is_Tessellated()) continue;

		int road_class = road->Get_Road_Class();
		float offset = (road->Get_Total_Width() * 0.5f) + STREET_LAMP_SETBACK;
		float next = params.StreetLampSpacing * 0.5f;		// the first lamp half a spacing in
		int station = 0;
		int placed_on_road = 0;

		for (int i = 1; i < road->Get_Point_Count(); i++) {

			const RoadPointClass & a = road->Get_Point(i - 1);
			const RoadPointClass & b = road->Get_Point(i);
			float run = b.Distance - a.Distance;

			while (next <= b.Distance) {

				float t = (run > WWMATH_EPSILON) ? WWMath::Clamp((next - a.Distance) / run,0.0f,1.0f) : 0.0f;
				Vector3 centre = a.Position + ((b.Position - a.Position) * t);

				bool eligible;
				if (road_class == ROAD_CLASS_STREET) {
					eligible = true;
				} else if (road_class == ROAD_CLASS_HIGHWAY) {
					eligible = ((station % 2) == 0);
				} else {
					eligible = In_Industrial_Settlement(centre) || Near_Selected_Base(centre,params.StreetLampSpacing);
				}

				if (eligible) {
					float sign = ((placed_on_road % 2) == 0) ? 1.0f : -1.0f;
					Vector3 lamp = centre + (a.Side * (sign * offset));
					if (Is_Legal_Lamp_Point(lamp)) {
						//	Facing the road it lights.
						float yaw = WWMath::Atan2(-a.Side.Y * sign,-a.Side.X * sign);
						if (FoliageSystem::Add_Instance("ow_street_lamp",lamp,yaw,1.0f) >= 0) {
							StreetLampCount++;
							placed_on_road++;
						}
					}
				}

				station++;
				next += params.StreetLampSpacing;
			}
		}
	}
}


/***********************************************************************************************
 * self-check accessors -- step 12's own "re-run route-clearance checks after decoration"        *
 *=============================================================================================*/
int BiomeDecorationGenerator::Check_No_Instance_In_Road(StringClass * report)
{
	int violations = 0;
	if (!Built) return violations;

	for (int i = 0; i < FoliageSystem::Get_Instance_Count(); i++) {
		const FoliageInstanceClass * instance = FoliageSystem::Peek_Instance(i);
		if ((instance == nullptr) || !instance->Alive) continue;
		if (Reads_As_Occupied(TERRAIN_MASK_ROAD,instance->Position.X,instance->Position.Y)) {
			violations++;
			if (report) *report += "a decoration instance sits inside the road mask\n";
		}
	}
	return violations;
}

int BiomeDecorationGenerator::Check_No_Instance_Off_Terrain(StringClass * report)
{
	int violations = 0;
	if (!Built) return violations;

	for (int i = 0; i < FoliageSystem::Get_Instance_Count(); i++) {
		const FoliageInstanceClass * instance = FoliageSystem::Peek_Instance(i);
		if ((instance == nullptr) || !instance->Alive) continue;
		float height = 0.0f;
		if (!WorldTerrainSystem::Sample_Height(instance->Position.X,instance->Position.Y,&height)) {
			violations++;
			if (report) *report += "a decoration instance sits off the terrain entirely\n";
		}
	}
	return violations;
}
