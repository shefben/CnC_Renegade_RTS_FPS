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
 *                                                                                             *
 *                 Project Name : OpenW3D                                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwphys/mapplangenerator.cpp                 $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	D-003 Phase 3B.  See mapplangenerator.h for the layer model.  Order of construction:         *
 *                                                                                             *
 *	  1. Place_Sites       -- GDI base at a seeded angle, Nod base by symmetry, centre field at  *
 *	                          the world centre, flank fields at +/-90 degrees from the base axis.  *
 *	  2. Paint_Lanes       -- corridors GDI->centre->Nod and GDI->flank->Nod, one per flank.     *
 *	  3. Assign_Tiers      -- seeded Voronoi regions get a tier each (symmetric under the plan's  *
 *	                          symmetry), then bases are forced flat on the middle tier, the       *
 *	                          centre field to tier 0, and every lane cell is walked so a lane     *
 *	                          crosses at most one tier per step.                                  *
 *	  4. Mark_Seams_And_Ramps -- tier change under a lane = RAMP; elsewhere = CLIFF_SEAM.          *
 *	  5. Plan_Water        -- lowest-tier open cells, by hash, up to WaterFraction.                *
 *	  6. Compute_Checksum.                                                                        *
 *                                                                                             *
 *	Every step is a fixed-order walk over the grid with integer hashes, no RNG state.              *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "mapplangenerator.h"

#include "crc.h"
#include "terrainmask.h"
#include "wwdebug.h"
#include "wwmath.h"

#include <math.h>


bool									MapPlanGenerator::Built = false;
unsigned int						MapPlanGenerator::Checksum = 0;
int									MapPlanGenerator::CountX = 0;
int									MapPlanGenerator::CountY = 0;
float									MapPlanGenerator::CellSize = 0.0f;
float									MapPlanGenerator::OriginX = 0.0f;
float									MapPlanGenerator::OriginY = 0.0f;
float									MapPlanGenerator::TierStep = 0.0f;
int									MapPlanGenerator::TierCount = 0;
DynamicVectorClass<signed char>		MapPlanGenerator::Tiers;
DynamicVectorClass<unsigned char>	MapPlanGenerator::Kinds;
DynamicVectorClass<MapPlanSite>		MapPlanGenerator::Sites;


static const int MAP_PLAN_MAX_CELLS = 512 * 512;
static const int MAP_PLAN_VORONOI_SEEDS_PER_1000_CELLS = 6;


MapPlanParams::MapPlanParams(void) :
	Seed(0),
	WorldWidth(0.0f),
	WorldHeight(0.0f),
	ChunkSize(60.0f),
	CenterX(0.0f),
	CenterY(0.0f),
	Symmetry(MAP_PLAN_SYMMETRY_ROTATIONAL),
	TierCount(3),
	TierStep(18.0f),
	LaneWidthCells(1),
	BaseRadiusCells(3),
	FlankLaneCount(1),
	WaterFraction(0.08f),
	BoundaryMarginCells(2.0f)
{
}


/***********************************************************************************************
 * MapPlanGenerator::Init / Shutdown / Clear                                                   *
 *=============================================================================================*/
void MapPlanGenerator::Init(void)
{
	Clear ();
}


void MapPlanGenerator::Shutdown(void)
{
	Clear ();
}


void MapPlanGenerator::Clear(void)
{
	Built = false;
	Checksum = 0;
	CountX = 0;
	CountY = 0;
	CellSize = 0.0f;
	OriginX = 0.0f;
	OriginY = 0.0f;
	TierStep = 0.0f;
	TierCount = 0;
	Tiers.Delete_All ();
	Kinds.Delete_All ();
	Sites.Delete_All ();
}


/***********************************************************************************************
 * MapPlanGenerator::Generate                                                                  *
 *=============================================================================================*/
bool MapPlanGenerator::Generate(const MapPlanParams & params,StringClass * error_out)
{
	Clear ();

	#define MAP_PLAN_FAIL(msg)	do { if (error_out != nullptr) { *error_out = msg; } return false; } while (0)

	if (params.ChunkSize <= 0.0f)											MAP_PLAN_FAIL ("map plan: chunk_size must be > 0");
	if ((params.WorldWidth <= 0.0f) || (params.WorldHeight <= 0.0f))	MAP_PLAN_FAIL ("map plan: world width/height must be > 0");
	if (params.TierCount < 2)												MAP_PLAN_FAIL ("map plan: tier_count must be >= 2");
	if (params.TierStep <= 0.0f)											MAP_PLAN_FAIL ("map plan: tier_step must be > 0");
	if (params.LaneWidthCells < 1)										MAP_PLAN_FAIL ("map plan: lane_width_cells must be >= 1");
	if (params.BaseRadiusCells < 1)										MAP_PLAN_FAIL ("map plan: base_radius_cells must be >= 1");
	if ((params.FlankLaneCount < 0) || (params.FlankLaneCount > 2))	MAP_PLAN_FAIL ("map plan: flank_lanes must be 0..2");
	if ((params.WaterFraction < 0.0f) || (params.WaterFraction > 1.0f))	MAP_PLAN_FAIL ("map plan: water_fraction must be 0..1");
	if (params.BoundaryMarginCells < 0.0f)								MAP_PLAN_FAIL ("map plan: boundary_margin_cells must be >= 0");

	int count_x = (int)ceilf (params.WorldWidth / params.ChunkSize);
	int count_y = (int)ceilf (params.WorldHeight / params.ChunkSize);
	if ((count_x < 8) || (count_y < 8))									MAP_PLAN_FAIL ("map plan: world too small -- fewer than 8 x 8 chunks");
	if ((count_x * count_y) > MAP_PLAN_MAX_CELLS)						MAP_PLAN_FAIL ("map plan: world too large -- more than 512 x 512 chunks");

	CountX = count_x;
	CountY = count_y;
	CellSize = params.ChunkSize;
	OriginX = params.CenterX - (float)count_x * params.ChunkSize * 0.5f;
	OriginY = params.CenterY - (float)count_y * params.ChunkSize * 0.5f;
	TierStep = params.TierStep;
	TierCount = params.TierCount;

	int total = count_x * count_y;
	for (int i = 0; i < total; i++) {
		Tiers.Add ((signed char)0);
		Kinds.Add ((unsigned char)MAP_PLAN_CELL_OPEN);
	}

	Place_Sites (params);
	if (Get_Site_Count_By_Kind (MAP_PLAN_SITE_BASE_GDI) != 1 || Get_Site_Count_By_Kind (MAP_PLAN_SITE_BASE_NOD) != 1) {
		Clear ();
		MAP_PLAN_FAIL ("map plan: could not place both base sites inside the margin");
	}

	Paint_Lanes (params);
	Assign_Tiers (params);
	Mark_Seams_And_Ramps ();
	Plan_Water (params);
	Compute_Checksum ();

	Built = true;

	#undef MAP_PLAN_FAIL

	WWDEBUG_SAY (("MapPlanGenerator: plan built -- %d x %d cells, %d sites, %d tiers, checksum 0x%08X\r\n",
		CountX,CountY,Sites.Count (),TierCount,Checksum));
	return true;
}


/***********************************************************************************************
 * MapPlanGenerator::Mirror_Cell -- the plan's symmetry, applied to a cell index               *
 *=============================================================================================*/
void MapPlanGenerator::Mirror_Cell(const MapPlanParams & params,int cx,int cy,int * mx_out,int * my_out)
{
	switch (params.Symmetry) {
		case MAP_PLAN_SYMMETRY_ROTATIONAL:
			*mx_out = CountX - 1 - cx;
			*my_out = CountY - 1 - cy;
			break;
		case MAP_PLAN_SYMMETRY_MIRROR_X:
			*mx_out = CountX - 1 - cx;
			*my_out = cy;
			break;
		case MAP_PLAN_SYMMETRY_NONE:
		default:
			//	No symmetry: the opposite base is still opposite, but jittered by hash so the
			//	two halves are not congruent.
			{
				float jx = TerrainMaskClass::Hash_Value (cx,cy,(int)params.Seed ^ 0x51ED) - 0.5f;
				float jy = TerrainMaskClass::Hash_Value (cy,cx,(int)params.Seed ^ 0x7A3C) - 0.5f;
				*mx_out = CountX - 1 - cx + (int)(jx * (float)CountX * 0.15f);
				*my_out = CountY - 1 - cy + (int)(jy * (float)CountY * 0.15f);
				if (*mx_out < 0) *mx_out = 0;
				if (*my_out < 0) *my_out = 0;
				if (*mx_out >= CountX) *mx_out = CountX - 1;
				if (*my_out >= CountY) *my_out = CountY - 1;
			}
			break;
	}
}


/***********************************************************************************************
 * MapPlanGenerator::Place_Sites -- L0 layout graph                                            *
 *=============================================================================================*/
void MapPlanGenerator::Place_Sites(const MapPlanParams & params)
{
	int margin = (int)ceilf (params.BoundaryMarginCells) + params.BaseRadiusCells;
	int usable_x = CountX - 2 * margin;
	int usable_y = CountY - 2 * margin;
	if ((usable_x < 2) || (usable_y < 2)) {
		return;
	}

	//	GDI base: a seeded angle, at 0.36 of the usable half-extent from the centre.  The angle is
	//	kept away from the pure diagonals so the base axis crosses the map rather than hugging a
	//	corner; RA2's generator likewise prefers edge-to-edge over corner-to-corner.
	float angle_unit = TerrainMaskClass::Hash_Value (7,11,(int)params.Seed);
	float angle = angle_unit * 2.0f * WWMATH_PI;
	float half_x = (float)usable_x * 0.5f;
	float half_y = (float)usable_y * 0.5f;
	float center_cx = (float)CountX * 0.5f;
	float center_cy = (float)CountY * 0.5f;
	float reach = 0.72f;

	int gdi_cx = (int)(center_cx + cosf (angle) * half_x * reach);
	int gdi_cy = (int)(center_cy + sinf (angle) * half_y * reach);
	if (gdi_cx < margin) gdi_cx = margin;
	if (gdi_cy < margin) gdi_cy = margin;
	if (gdi_cx > CountX - 1 - margin) gdi_cx = CountX - 1 - margin;
	if (gdi_cy > CountY - 1 - margin) gdi_cy = CountY - 1 - margin;

	int nod_cx = 0;
	int nod_cy = 0;
	Mirror_Cell (params,gdi_cx,gdi_cy,&nod_cx,&nod_cy);
	if (nod_cx < margin) nod_cx = margin;
	if (nod_cy < margin) nod_cy = margin;
	if (nod_cx > CountX - 1 - margin) nod_cx = CountX - 1 - margin;
	if (nod_cy > CountY - 1 - margin) nod_cy = CountY - 1 - margin;

	float base_radius_world = (float)params.BaseRadiusCells * CellSize;

	MapPlanSite gdi;
	gdi.Kind = MAP_PLAN_SITE_BASE_GDI;
	gdi.CellX = gdi_cx;
	gdi.CellY = gdi_cy;
	gdi.Position.Set (OriginX + ((float)gdi_cx + 0.5f) * CellSize,OriginY + ((float)gdi_cy + 0.5f) * CellSize);
	gdi.Radius = base_radius_world;
	Sites.Add (gdi);

	MapPlanSite nod;
	nod.Kind = MAP_PLAN_SITE_BASE_NOD;
	nod.CellX = nod_cx;
	nod.CellY = nod_cy;
	nod.Position.Set (OriginX + ((float)nod_cx + 0.5f) * CellSize,OriginY + ((float)nod_cy + 0.5f) * CellSize);
	nod.Radius = base_radius_world;
	Sites.Add (nod);

	//	Centre field at the midpoint of the base axis (the world centre under symmetry).
	int field_cx = (gdi_cx + nod_cx) / 2;
	int field_cy = (gdi_cy + nod_cy) / 2;
	int field_radius_cells = (params.BaseRadiusCells + 1) / 2;
	if (field_radius_cells < 1) field_radius_cells = 1;

	MapPlanSite field;
	field.Kind = MAP_PLAN_SITE_FIELD_CENTER;
	field.CellX = field_cx;
	field.CellY = field_cy;
	field.Position.Set (OriginX + ((float)field_cx + 0.5f) * CellSize,OriginY + ((float)field_cy + 0.5f) * CellSize);
	field.Radius = (float)field_radius_cells * CellSize;
	Sites.Add (field);

	//	Flank fields at +/-90 degrees from the base axis, one per flank lane, pushed out to 0.55
	//	of the perpendicular half-extent so the flank route is genuinely longer than the centre.
	float axis_dx = (float)(nod_cx - gdi_cx);
	float axis_dy = (float)(nod_cy - gdi_cy);
	float axis_len = sqrtf (axis_dx * axis_dx + axis_dy * axis_dy);
	if (axis_len < 1.0f) axis_len = 1.0f;
	float perp_x = -axis_dy / axis_len;
	float perp_y = axis_dx / axis_len;
	float perp_reach = 0.55f * ((half_x < half_y) ? half_x : half_y);

	for (int flank = 0; flank < params.FlankLaneCount; flank++) {
		float sign = (flank == 0) ? 1.0f : -1.0f;
		int fx = (int)((float)field_cx + perp_x * perp_reach * sign);
		int fy = (int)((float)field_cy + perp_y * perp_reach * sign);
		if (fx < margin) fx = margin;
		if (fy < margin) fy = margin;
		if (fx > CountX - 1 - margin) fx = CountX - 1 - margin;
		if (fy > CountY - 1 - margin) fy = CountY - 1 - margin;

		MapPlanSite flank_site;
		flank_site.Kind = MAP_PLAN_SITE_FIELD_FLANK;
		flank_site.CellX = fx;
		flank_site.CellY = fy;
		flank_site.Position.Set (OriginX + ((float)fx + 0.5f) * CellSize,OriginY + ((float)fy + 0.5f) * CellSize);
		flank_site.Radius = (float)field_radius_cells * CellSize;
		Sites.Add (flank_site);
	}

	//	Chokes: the midpoint of each base-to-centre-field half of the axis.
	for (int half = 0; half < 2; half++) {
		const MapPlanSite & base = (half == 0) ? gdi : nod;
		MapPlanSite choke;
		choke.Kind = MAP_PLAN_SITE_CHOKE;
		choke.CellX = (base.CellX + field_cx) / 2;
		choke.CellY = (base.CellY + field_cy) / 2;
		choke.Position.Set (OriginX + ((float)choke.CellX + 0.5f) * CellSize,OriginY + ((float)choke.CellY + 0.5f) * CellSize);
		choke.Radius = (float)params.LaneWidthCells * CellSize;
		Sites.Add (choke);
	}

	//	Paint the discs into L3.  Bases overwrite; fields do not overwrite bases.
	Paint_Disc (gdi_cx,gdi_cy,params.BaseRadiusCells,MAP_PLAN_CELL_BASE,true);
	Paint_Disc (nod_cx,nod_cy,params.BaseRadiusCells,MAP_PLAN_CELL_BASE,true);
	for (int i = 0; i < Sites.Count (); i++) {
		if ((Sites[i].Kind == MAP_PLAN_SITE_FIELD_CENTER) || (Sites[i].Kind == MAP_PLAN_SITE_FIELD_FLANK)) {
			Paint_Disc (Sites[i].CellX,Sites[i].CellY,field_radius_cells,MAP_PLAN_CELL_FIELD,false);
		}
	}
}


/***********************************************************************************************
 * MapPlanGenerator::Paint_Disc                                                                *
 *=============================================================================================*/
void MapPlanGenerator::Paint_Disc(int cx,int cy,int radius,MapPlanCellKind kind,bool overwrite)
{
	int r2 = radius * radius;
	for (int y = cy - radius; y <= cy + radius; y++) {
		for (int x = cx - radius; x <= cx + radius; x++) {
			if (!Cell_In_Grid (x,y)) continue;
			int dx = x - cx;
			int dy = y - cy;
			if ((dx * dx + dy * dy) > r2) continue;
			int index = y * CountX + x;
			if (overwrite || (Kinds[index] == MAP_PLAN_CELL_OPEN)) {
				Kinds[index] = (unsigned char)kind;
			}
		}
	}
}


/***********************************************************************************************
 * MapPlanGenerator::Paint_Corridor -- thick Bresenham with a seeded perpendicular wobble      *
 *=============================================================================================*/
void MapPlanGenerator::Paint_Corridor(int x0,int y0,int x1,int y1,int half_width,unsigned int seed,int wobble)
{
	int dx = abs (x1 - x0);
	int dy = abs (y1 - y0);
	int steps = (dx > dy) ? dx : dy;
	if (steps < 1) steps = 1;

	float fdx = (float)(x1 - x0) / (float)steps;
	float fdy = (float)(y1 - y0) / (float)steps;
	float len = sqrtf (fdx * fdx + fdy * fdy);
	if (len < 0.0001f) len = 1.0f;
	float px = -fdy / len;
	float py = fdx / len;

	for (int s = 0; s <= steps; s++) {
		//	Wobble is a smooth low-frequency offset: two hash samples blended along the corridor,
		//	so a lane bends rather than jitters.  Zero at both ends so it meets its sites.
		float t = (float)s / (float)steps;
		float envelope = 4.0f * t * (1.0f - t);
		float w0 = TerrainMaskClass::Hash_Value (s / 8,0,(int)seed) - 0.5f;
		float w1 = TerrainMaskClass::Hash_Value (s / 8 + 1,0,(int)seed) - 0.5f;
		float f = (float)(s % 8) / 8.0f;
		float offset = (w0 * (1.0f - f) + w1 * f) * 2.0f * (float)wobble * envelope;

		float cx = (float)x0 + fdx * (float)s + px * offset;
		float cy = (float)y0 + fdy * (float)s + py * offset;
		int icx = (int)floorf (cx + 0.5f);
		int icy = (int)floorf (cy + 0.5f);

		for (int oy = -half_width; oy <= half_width; oy++) {
			for (int ox = -half_width; ox <= half_width; ox++) {
				int x = icx + ox;
				int y = icy + oy;
				if (!Cell_In_Grid (x,y)) continue;
				int index = y * CountX + x;
				if (Kinds[index] == MAP_PLAN_CELL_OPEN) {
					Kinds[index] = (unsigned char)MAP_PLAN_CELL_LANE;
				}
			}
		}
	}
}


/***********************************************************************************************
 * MapPlanGenerator::Paint_Lanes -- L0 lanes into L3                                           *
 *=============================================================================================*/
void MapPlanGenerator::Paint_Lanes(const MapPlanParams & params)
{
	MapPlanSite gdi,nod,center;
	bool have_gdi = false,have_nod = false,have_center = false;
	for (int i = 0; i < Sites.Count (); i++) {
		if (Sites[i].Kind == MAP_PLAN_SITE_BASE_GDI)		{ gdi = Sites[i]; have_gdi = true; }
		if (Sites[i].Kind == MAP_PLAN_SITE_BASE_NOD)		{ nod = Sites[i]; have_nod = true; }
		if (Sites[i].Kind == MAP_PLAN_SITE_FIELD_CENTER)	{ center = Sites[i]; have_center = true; }
	}
	if (!have_gdi || !have_nod || !have_center) return;

	int half = params.LaneWidthCells;
	int wobble = (CountX < CountY ? CountX : CountY) / 12;

	//	Centre lane: base -> centre field -> base.
	Paint_Corridor (gdi.CellX,gdi.CellY,center.CellX,center.CellY,half,params.Seed ^ 0x1001u,wobble);
	Paint_Corridor (center.CellX,center.CellY,nod.CellX,nod.CellY,half,params.Seed ^ 0x1002u,wobble);

	//	Flank lanes: base -> flank field -> base, one per flank field.
	int flank_index = 0;
	for (int i = 0; i < Sites.Count (); i++) {
		if (Sites[i].Kind != MAP_PLAN_SITE_FIELD_FLANK) continue;
		unsigned int s = params.Seed ^ (0x2000u + (unsigned int)flank_index * 16u);
		Paint_Corridor (gdi.CellX,gdi.CellY,Sites[i].CellX,Sites[i].CellY,half,s,wobble);
		Paint_Corridor (Sites[i].CellX,Sites[i].CellY,nod.CellX,nod.CellY,half,s ^ 0x8u,wobble);
		flank_index++;
	}
}


/***********************************************************************************************
 * MapPlanGenerator::Assign_Tiers -- L1                                                        *
 *=============================================================================================*/
void MapPlanGenerator::Assign_Tiers(const MapPlanParams & params)
{
	//	Seeded Voronoi.  Seeds are placed by hash and mirrored under the plan symmetry so the two
	//	halves rise and fall together; each seed owns a tier by hash.  Mid tier is the default so
	//	both up and down relief exist.
	int seed_count = (CountX * CountY * MAP_PLAN_VORONOI_SEEDS_PER_1000_CELLS) / 1000;
	if (seed_count < 4) seed_count = 4;
	if (seed_count > 96) seed_count = 96;
	int half_seeds = (seed_count + 1) / 2;

	DynamicVectorClass<int> seed_x;
	DynamicVectorClass<int> seed_y;
	DynamicVectorClass<int> seed_tier;

	for (int i = 0; i < half_seeds; i++) {
		int sx = (int)(TerrainMaskClass::Hash_Value (i,101,(int)params.Seed) * (float)CountX);
		int sy = (int)(TerrainMaskClass::Hash_Value (i,202,(int)params.Seed) * (float)CountY);
		if (sx >= CountX) sx = CountX - 1;
		if (sy >= CountY) sy = CountY - 1;
		int tier = (int)(TerrainMaskClass::Hash_Value (i,303,(int)params.Seed) * (float)params.TierCount);
		if (tier >= params.TierCount) tier = params.TierCount - 1;

		seed_x.Add (sx);
		seed_y.Add (sy);
		seed_tier.Add (tier);

		int mx = 0,my = 0;
		Mirror_Cell (params,sx,sy,&mx,&my);
		seed_x.Add (mx);
		seed_y.Add (my);
		seed_tier.Add (tier);
	}

	int mid_tier = params.TierCount / 2;

	for (int y = 0; y < CountY; y++) {
		for (int x = 0; x < CountX; x++) {
			int best = -1;
			int best_d2 = 0x7FFFFFFF;
			for (int i = 0; i < seed_x.Count (); i++) {
				int dx = x - seed_x[i];
				int dy = y - seed_y[i];
				int d2 = dx * dx + dy * dy;
				if (d2 < best_d2) { best_d2 = d2; best = i; }
			}
			Tiers[y * CountX + x] = (signed char)((best >= 0) ? seed_tier[best] : mid_tier);
		}
	}

	//	Bases flat on the mid tier; centre field on tier 0 so it reads as the low ground both
	//	sides descend into; flank fields keep the tier of their region but are flattened
	//	within their disc.
	for (int i = 0; i < Sites.Count (); i++) {
		const MapPlanSite & site = Sites[i];
		int radius = (int)(site.Radius / CellSize + 0.5f);
		int forced_tier;
		switch (site.Kind) {
			case MAP_PLAN_SITE_BASE_GDI:
			case MAP_PLAN_SITE_BASE_NOD:		forced_tier = mid_tier; break;
			case MAP_PLAN_SITE_FIELD_CENTER:	forced_tier = 0; break;
			case MAP_PLAN_SITE_FIELD_FLANK:	forced_tier = Tiers[site.CellY * CountX + site.CellX]; break;
			default:									continue;
		}
		//	One extra cell so the seam sits outside the painted disc.
		int r = radius + 1;
		for (int y = site.CellY - r; y <= site.CellY + r; y++) {
			for (int x = site.CellX - r; x <= site.CellX + r; x++) {
				if (!Cell_In_Grid (x,y)) continue;
				int dx = x - site.CellX;
				int dy = y - site.CellY;
				if ((dx * dx + dy * dy) > r * r) continue;
				Tiers[y * CountX + x] = (signed char)forced_tier;
			}
		}
	}

	//	Lane continuity.  Walk the grid; for each LANE cell whose 4-neighbour LANE cell differs by
	//	more than one tier, pull the higher one down a tier.  Iterate to a fixed point (bounded)
	//	so a lane never has to climb two tiers in one cell.
	for (int pass = 0; pass < params.TierCount; pass++) {
		bool changed = false;
		for (int y = 0; y < CountY; y++) {
			for (int x = 0; x < CountX; x++) {
				int index = y * CountX + x;
				if ((Kinds[index] != MAP_PLAN_CELL_LANE) && (Kinds[index] != MAP_PLAN_CELL_BASE) && (Kinds[index] != MAP_PLAN_CELL_FIELD)) continue;
				static const int NX[4] = { 1,-1,0,0 };
				static const int NY[4] = { 0,0,1,-1 };
				for (int n = 0; n < 4; n++) {
					int nx = x + NX[n];
					int ny = y + NY[n];
					if (!Cell_In_Grid (nx,ny)) continue;
					int nindex = ny * CountX + nx;
					if ((Kinds[nindex] != MAP_PLAN_CELL_LANE) && (Kinds[nindex] != MAP_PLAN_CELL_BASE) && (Kinds[nindex] != MAP_PLAN_CELL_FIELD)) continue;
					int diff = (int)Tiers[index] - (int)Tiers[nindex];
					if (diff > 1) {
						Tiers[index] = (signed char)((int)Tiers[nindex] + 1);
						changed = true;
					}
				}
			}
		}
		if (!changed) break;
	}
}


/***********************************************************************************************
 * MapPlanGenerator::Mark_Seams_And_Ramps -- L3 from L1                                        *
 *=============================================================================================*/
void MapPlanGenerator::Mark_Seams_And_Ramps(void)
{
	static const int NX[4] = { 1,-1,0,0 };
	static const int NY[4] = { 0,0,1,-1 };

	for (int y = 0; y < CountY; y++) {
		for (int x = 0; x < CountX; x++) {
			int index = y * CountX + x;
			bool seam = false;
			for (int n = 0; n < 4; n++) {
				int nx = x + NX[n];
				int ny = y + NY[n];
				if (!Cell_In_Grid (nx,ny)) continue;
				if (Tiers[ny * CountX + nx] != Tiers[index]) { seam = true; break; }
			}
			if (!seam) continue;

			switch (Kinds[index]) {
				case MAP_PLAN_CELL_LANE:
				case MAP_PLAN_CELL_BASE:
				case MAP_PLAN_CELL_FIELD:
					Kinds[index] = (unsigned char)MAP_PLAN_CELL_RAMP;
					break;
				case MAP_PLAN_CELL_OPEN:
					Kinds[index] = (unsigned char)MAP_PLAN_CELL_CLIFF_SEAM;
					break;
				default:
					break;
			}
		}
	}
}


/***********************************************************************************************
 * MapPlanGenerator::Plan_Water -- L2                                                          *
 *=============================================================================================*/
void MapPlanGenerator::Plan_Water(const MapPlanParams & params)
{
	if (params.WaterFraction <= 0.0f) return;

	//	Only tier-0 OPEN cells are eligible; sites, lanes, seams and ramps stay dry.  Hash per
	//	cell against the fraction; then a neighbour smoothing pass so water forms pools rather
	//	than speckle.
	DynamicVectorClass<unsigned char> wet;
	int total = CountX * CountY;
	for (int i = 0; i < total; i++) {
		bool eligible = (Tiers[i] == 0) && (Kinds[i] == MAP_PLAN_CELL_OPEN);
		float h = TerrainMaskClass::Hash_Value (i % CountX,i / CountX,(int)params.Seed ^ 0x4A7E);
		//	Threshold scaled by 1.6 because the majority filter below removes the isolated hits.
		wet.Add ((unsigned char)((eligible && (h < params.WaterFraction * 1.6f)) ? 1 : 0));
	}

	static const int NX[4] = { 1,-1,0,0 };
	static const int NY[4] = { 0,0,1,-1 };
	for (int y = 0; y < CountY; y++) {
		for (int x = 0; x < CountX; x++) {
			int index = y * CountX + x;
			if (!wet[index]) continue;
			int wet_neighbours = 0;
			for (int n = 0; n < 4; n++) {
				int nx = x + NX[n];
				int ny = y + NY[n];
				if (Cell_In_Grid (nx,ny) && wet[ny * CountX + nx]) wet_neighbours++;
			}
			if (wet_neighbours >= 2) {
				Kinds[index] = (unsigned char)MAP_PLAN_CELL_WATER;
			}
		}
	}
}


/***********************************************************************************************
 * MapPlanGenerator::Compute_Checksum                                                          *
 *=============================================================================================*/
void MapPlanGenerator::Compute_Checksum(void)
{
	uint32_t crc = 0;
	int total = CountX * CountY;
	for (int i = 0; i < total; i++) {
		unsigned char pair[2] = { (unsigned char)Tiers[i],Kinds[i] };
		crc = CRC::Memory (pair,2,crc);
	}
	for (int i = 0; i < Sites.Count (); i++) {
		int fields[3] = { (int)Sites[i].Kind,Sites[i].CellX,Sites[i].CellY };
		crc = CRC::Memory ((unsigned char *)fields,sizeof (fields),crc);
	}
	int dims[3] = { CountX,CountY,TierCount };
	crc = CRC::Memory ((unsigned char *)dims,sizeof (dims),crc);
	Checksum = crc;
}


/***********************************************************************************************
 * MapPlanGenerator queries                                                                    *
 *=============================================================================================*/
bool MapPlanGenerator::Get_Cell(int cx,int cy,int * tier_out,MapPlanCellKind * kind_out)
{
	if (!Built || !Cell_In_Grid (cx,cy)) return false;
	int index = cy * CountX + cx;
	if (tier_out != nullptr) *tier_out = (int)Tiers[index];
	if (kind_out != nullptr) *kind_out = (MapPlanCellKind)Kinds[index];
	return true;
}


bool MapPlanGenerator::World_To_Cell(float wx,float wy,int * cx_out,int * cy_out)
{
	if (!Built) return false;
	int cx = (int)floorf ((wx - OriginX) / CellSize);
	int cy = (int)floorf ((wy - OriginY) / CellSize);
	if (cx < 0) cx = 0;
	if (cy < 0) cy = 0;
	if (cx >= CountX) cx = CountX - 1;
	if (cy >= CountY) cy = CountY - 1;
	if (cx_out != nullptr) *cx_out = cx;
	if (cy_out != nullptr) *cy_out = cy;
	return true;
}


int MapPlanGenerator::Get_Tier_At(float wx,float wy)
{
	int cx = 0,cy = 0;
	if (!World_To_Cell (wx,wy,&cx,&cy)) return 0;
	return (int)Tiers[cy * CountX + cx];
}


MapPlanCellKind MapPlanGenerator::Get_Kind_At(float wx,float wy)
{
	int cx = 0,cy = 0;
	if (!World_To_Cell (wx,wy,&cx,&cy)) return MAP_PLAN_CELL_OPEN;
	return (MapPlanCellKind)Kinds[cy * CountX + cx];
}


float MapPlanGenerator::Sample_Tier_Height(float wx,float wy)
{
	if (!Built) return 0.0f;

	//	Bilinear over the four cell centres around (wx,wy).  A cliff-seam cell contributes its
	//	own tier to every corner it touches instead of blending, which keeps the height step
	//	across a seam sharp within one cell so the interior cliff generator sees a steep chunk.
	float fx = (wx - OriginX) / CellSize - 0.5f;
	float fy = (wy - OriginY) / CellSize - 0.5f;
	int x0 = (int)floorf (fx);
	int y0 = (int)floorf (fy);
	float tx = fx - (float)x0;
	float ty = fy - (float)y0;

	int cx = 0,cy = 0;
	World_To_Cell (wx,wy,&cx,&cy);
	int here = cy * CountX + cx;
	bool seam_here = (Kinds[here] == MAP_PLAN_CELL_CLIFF_SEAM);

	float sum = 0.0f;
	float wsum = 0.0f;
	for (int j = 0; j < 2; j++) {
		for (int i = 0; i < 2; i++) {
			int x = x0 + i;
			int y = y0 + j;
			if (x < 0) x = 0;
			if (y < 0) y = 0;
			if (x >= CountX) x = CountX - 1;
			if (y >= CountY) y = CountY - 1;
			float w = ((i == 0) ? (1.0f - tx) : tx) * ((j == 0) ? (1.0f - ty) : ty);
			int index = y * CountX + x;
			float tier = (float)Tiers[seam_here ? here : index];
			sum += w * tier;
			wsum += w;
		}
	}
	if (wsum <= 0.0f) return (float)Tiers[here] * TierStep;
	return (sum / wsum) * TierStep;
}


bool MapPlanGenerator::Get_Site(int index,MapPlanSite * out)
{
	if (!Built || (index < 0) || (index >= Sites.Count ())) return false;
	if (out != nullptr) *out = Sites[index];
	return true;
}


int MapPlanGenerator::Get_Site_Count_By_Kind(MapPlanSiteKind kind)
{
	int count = 0;
	for (int i = 0; i < Sites.Count (); i++) {
		if (Sites[i].Kind == kind) count++;
	}
	return count;
}


float MapPlanGenerator::Get_Base_Site_Weight(float wx,float wy)
{
	if (!Built) return 0.0f;
	for (int i = 0; i < Sites.Count (); i++) {
		const MapPlanSite & site = Sites[i];
		if ((site.Kind != MAP_PLAN_SITE_BASE_GDI) && (site.Kind != MAP_PLAN_SITE_BASE_NOD)) continue;
		float dx = wx - site.Position.X;
		float dy = wy - site.Position.Y;
		if ((dx * dx + dy * dy) <= site.Radius * site.Radius) return 1.0f;
	}
	return 0.0f;
}


float MapPlanGenerator::Get_Field_Site_Weight(float wx,float wy)
{
	if (!Built) return 0.0f;
	for (int i = 0; i < Sites.Count (); i++) {
		const MapPlanSite & site = Sites[i];
		if ((site.Kind != MAP_PLAN_SITE_FIELD_CENTER) && (site.Kind != MAP_PLAN_SITE_FIELD_FLANK)) continue;
		float dx = wx - site.Position.X;
		float dy = wy - site.Position.Y;
		if ((dx * dx + dy * dy) <= site.Radius * site.Radius) return 1.0f;
	}
	return 0.0f;
}
