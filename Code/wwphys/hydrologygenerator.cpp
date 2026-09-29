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
 *                     $Archive:: /Commando/Code/wwphys/hydrologygenerator.cpp                $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "hydrologygenerator.h"

#include "mapplangenerator.h"

#include "crc.h"
#include "watersystem.h"
#include "terraintexturesystem.h"
#include "watertype.h"
#include "worldboundsservice.h"
#include "worldterrainsystem.h"
#include "wwdebug.h"
#include "wwmath.h"


bool				HydrologyGenerator::Built			= false;
unsigned int	HydrologyGenerator::Checksum		= 0;

DynamicVectorClass<int>					HydrologyGenerator::OwnedWaterAreaIds;
DynamicVectorClass<HydrologyFeature>	HydrologyGenerator::Features;


//	A channel-eligible chunk needs at least this fraction of the river threshold before it is
//	considered a stream/tributary source at all.  Below this, a chunk's own accumulation is
//	ordinary hillside drainage, not a feature worth carving.
static const float STREAM_ACCUMULATION_FRACTION = 0.34f;

//	How far above a basin chunk's own sampled minimum height the still surface sits.  Small and
//	positive so the carved bed (WaterDefinitionClass::BedDepth, cut down from this surface) has
//	somewhere to go without the surface itself floating above the ground it fills.
static const float BASIN_HEIGHT_MARGIN = 0.5f;

//	A channel narrower than this multiple of its own source width is a candidate ford; wider
//	defaults to a bridge candidate.  Deliberately simple -- see this file's header comment on
//	what a hydrology pass can and cannot decide about a crossing.
static const float FORD_WIDTH_FACTOR = 1.5f;

//	Bank slope thresholds, radians -- roughly 15 and 35 degrees.  The same order of magnitude a
//	soldier's own slide angle sits at, not a claim to a precise gameplay-tuned number (that is
//	WorldMetricsClass's job, not this generator's).
static const float WALKABLE_BANK_SLOPE_RADIANS	= 0.262f;
static const float STEEP_BANK_SLOPE_RADIANS		= 0.611f;

//	Section 3B.4: never hash a raw float.
static const float CHECKSUM_QUANTUM = 1000.0f;

//	Fixed, deterministic neighbour order -- the same idiom InteriorCliffGenerator uses for its
//	own walk, redeclared here rather than shared: each generator's determinism is self-contained
//	and owns its own small constants.
static const int NEIGHBOR_DX[4] = {  1, 0, -1, 0 };
static const int NEIGHBOR_DY[4] = {  0, 1,  0,-1 };


HydrologyParams::HydrologyParams(void) :
	Seed (1),
	GenerateRivers (true),
	GenerateLakes (true),
	GeneratePonds (true),
	RiverWidth (6.0f),
	StreamWidth (2.5f),
	RiverAccumulationChunks (6.0f),
	MaxRiverCount (4),
	GenerateCoastline (false),
	SeaLevel (0.0f)
{
}


HydrologyCrossingCandidate::HydrologyCrossingCandidate(void) :
	Position (0.0f,0.0f,0.0f),
	Kind (HYDRO_CROSSING_FORD)
{
}


bool HydrologyFeature::operator == (const HydrologyFeature & that) const
{
	if ((WaterAreaId != that.WaterAreaId) || (Name != that.Name) || (Type != that.Type) ||
		 (Shore != that.Shore) || (Enclosed != that.Enclosed) || (Length != that.Length) ||
		 (Crossings.Count() != that.Crossings.Count())) {
		return false;
	}
	for (int i = 0; i < Crossings.Count(); i++) {
		if ((Crossings[i].Position != that.Crossings[i].Position) || (Crossings[i].Kind != that.Crossings[i].Kind)) {
			return false;
		}
	}
	return true;
}


HydrologyFeature::HydrologyFeature(void) :
	WaterAreaId (-1),
	Type (HYDRO_FEATURE_RIVER),
	Shore (HYDRO_SHORE_WALKABLE_BANK),
	Enclosed (false),
	Length (0.0f)
{
}


void HydrologyGenerator::Init(void)
{
	Clear();
}


void HydrologyGenerator::Shutdown(void)
{
	Clear();
}


/***********************************************************************************************
 * Clear -- drop only this generator's own water areas                                          *
 *                                                                                             *
 *	Mirrors BoundaryCliffGenerator::Clear_Ring and InteriorCliffGenerator::Clear exactly: read      *
 *	the shared collection back minus what this generator owns, clear the whole thing (WaterSystem   *
 *	has no per-owner selectivity of its own), then re-add what was kept.                              *
 *=============================================================================================*/
void HydrologyGenerator::Clear(void)
{
	if (OwnedWaterAreaIds.Count() > 0) {
		WaterSystem::Destroy_All_Geometry();

		DynamicVectorClass<WaterAreaClass> keep;
		for (int i = 0; i < WaterSystem::Get_Area_Count(); i++) {
			const WaterAreaClass & area = WaterSystem::Peek_Area(i);

			bool owned = false;
			for (int j = 0; j < OwnedWaterAreaIds.Count(); j++) {
				if (OwnedWaterAreaIds[j] == area.Get_ID()) { owned = true; break; }
			}
			if (!owned) {
				keep.Add(area);
			}
		}

		WaterSystem::Clear_Areas();
		for (int i = 0; i < keep.Count(); i++) {
			WaterSystem::Add_Area(keep[i]);
			int index = WaterSystem::Get_Area_Count() - 1;
			WaterSystem::Shape_Terrain(index);
			WaterSystem::Stamp_Mask(index);
			WaterSystem::Build_Geometry(index);
		}
	}

	OwnedWaterAreaIds.Delete_All();
	Features.Delete_All();

	Built		= false;
	Checksum	= 0;
}


bool HydrologyGenerator::Get_Feature(int index,HydrologyFeature * out)
{
	if ((index < 0) || (index >= Features.Count()) || (out == nullptr)) {
		return false;
	}
	*out = Features[index];
	return true;
}


/*
**	Bank slope at one point, offset outward from a water edge.  Classifies WALKABLE_BANK/
**	STEEP_BANK/CLIFF_SHORE from measured slope; VEHICLE_CROSSABLE overrides either bank grade
**	when the channel itself is narrow enough to just drive across, and a wide, cliff-shored
**	channel overrides down to NON_CROSSABLE.
*/
static HydrologyShoreType Classify_Shore(float x,float y,float width,float ford_width)
{
	float slope = 0.0f;
	bool got = WorldTerrainSystem::Sample_Slope(x,y,&slope);

	HydrologyShoreType bank;
	if (!got) {
		bank = HYDRO_SHORE_WALKABLE_BANK;			// nothing measurable; do not invent a worse answer than unknown
	} else if (slope <= WALKABLE_BANK_SLOPE_RADIANS) {
		bank = HYDRO_SHORE_WALKABLE_BANK;
	} else if (slope <= STEEP_BANK_SLOPE_RADIANS) {
		bank = HYDRO_SHORE_STEEP_BANK;
	} else {
		bank = HYDRO_SHORE_CLIFF_SHORE;
	}

	if ((width <= ford_width) && (bank != HYDRO_SHORE_CLIFF_SHORE)) {
		return HYDRO_SHORE_VEHICLE_CROSSABLE;
	}
	if ((width > (ford_width * 3.0f)) && (bank == HYDRO_SHORE_CLIFF_SHORE)) {
		return HYDRO_SHORE_NON_CROSSABLE;
	}
	return bank;
}


bool HydrologyGenerator::Build_Basin_Feature(int px,int py,float accumulation,const HydrologyParams & params)
{
	bool as_lake = (accumulation >= params.RiverAccumulationChunks);
	if (as_lake && !params.GenerateLakes) {
		if (!params.GeneratePonds) return false;
		as_lake = false;
	} else if (!as_lake && !params.GeneratePonds) {
		if (!params.GenerateLakes) return false;
		as_lake = true;
	}

	AABoxClass bounds;
	if (!WorldTerrainSystem::Get_Patch_Bounds(px,py,&bounds)) return false;

	TerrainChunkInfoClass info;
	if (!WorldTerrainSystem::Get_Chunk_Info(px,py,&info)) return false;
	if (info.CliffBand || info.InteriorCliff) return false;	// cliffs are placed before hydrology

	float y0 = bounds.Center.Y - bounds.Extent.Y;
	float surface_height = info.MinHeight + BASIN_HEIGHT_MARGIN;

	WaterAreaClass area;
	StringClass name;
	name.Format("hydrology_basin_%d_%d",px,py);
	area.Set_Name(name.Peek_Buffer());
	area.Set_Definition(as_lake ? "ow_water_lake" : "ow_water_pond");
	area.Set_Closed(true);
	area.Set_Height(surface_height);
	//	Keep the basin inside its drainage chunk, but give its shore an organic
	//	octagonal outline.  Four chunk-corner stations made every pond a 60 m
	//	square on the native profile.
	static const float direction_x[8] = { 0.0f,-0.707107f,-1.0f,-0.707107f,0.0f,0.707107f,1.0f,0.707107f };
	static const float direction_y[8] = { -1.0f,-0.707107f,0.0f,0.707107f,1.0f,0.707107f,0.0f,-0.707107f };
	for (int station = 0; station < 8; station++) {
		float shape = 0.78f + (0.08f * TerrainMaskClass::Hash_Value(
			(px * 8) + station,py,(int)params.Seed));
		area.Add_Station(Vector3(bounds.Center.X + (direction_x[station] * bounds.Extent.X * shape),
			bounds.Center.Y + (direction_y[station] * bounds.Extent.Y * shape),0.0f),0.0f);
	}

	int id = WaterSystem::Add_Area(area);
	int index = WaterSystem::Get_Area_Count() - 1;
	OwnedWaterAreaIds.Add(id);
	WaterSystem::Shape_Terrain(index);
	WaterSystem::Stamp_Mask(index);
	WaterSystem::Build_Geometry(index);

	HydrologyFeature feature;
	feature.WaterAreaId	= id;
	feature.Name			= name;
	feature.Type			= as_lake ? HYDRO_FEATURE_LAKE : HYDRO_FEATURE_POND;
	feature.Enclosed		= true;
	feature.Length			= 0.0f;
	feature.Shore			= Classify_Shore(bounds.Center.X,y0,0.0f,0.0f);

	HydrologyCrossingCandidate crossing;
	crossing.Position	= Vector3(bounds.Center.X,bounds.Center.Y,surface_height);
	crossing.Kind			= HYDRO_CROSSING_ROUTE_AROUND;
	feature.Crossings.Add(crossing);

	Features.Add(feature);
	return true;
}


//	One coastal chunk: a closed ow_water_coast area at sea level over the chunk's own square, the
//	same shape and pipeline a basin uses, so the one water system draws, masks and shapes it.
bool HydrologyGenerator::Build_Coast_Feature(int px,int py,const HydrologyParams & params)
{
	AABoxClass bounds;
	if (!WorldTerrainSystem::Get_Patch_Bounds(px,py,&bounds)) return false;

	TerrainChunkInfoClass info;
	if (!WorldTerrainSystem::Get_Chunk_Info(px,py,&info)) return false;
	if (info.CliffBand || info.InteriorCliff) return false;	// preserve both cliff layers

	float x0 = bounds.Center.X - bounds.Extent.X;
	float x1 = bounds.Center.X + bounds.Extent.X;
	float y0 = bounds.Center.Y - bounds.Extent.Y;
	float y1 = bounds.Center.Y + bounds.Extent.Y;

	WaterAreaClass area;
	StringClass name;
	name.Format("hydrology_coast_%d_%d",px,py);
	area.Set_Name(name.Peek_Buffer());
	area.Set_Definition("ow_water_coast");
	area.Set_Closed(true);
	area.Set_Height(params.SeaLevel);
	// Clip the chunk's water surface to the sampled sea-level contour.  Flooding
	// whole chunk squares made a visible stair-step shoreline even where the
	// underlying heightfield crossed sea level halfway through the chunk.
	DynamicVectorClass<Vector3> polygon;
	polygon.Add(Vector3(x0,y0,0.0f));
	polygon.Add(Vector3(x0,y1,0.0f));
	polygon.Add(Vector3(x1,y1,0.0f));
	polygon.Add(Vector3(x1,y0,0.0f));
	for (int i = 0; i < polygon.Count(); i++) {
		float height = 0.0f;
		if (!WorldTerrainSystem::Sample_Height(polygon[i].X,polygon[i].Y,&height)) return false;
		polygon[i].Z = height;
	}
	DynamicVectorClass<Vector3> clipped;
	for (int i = 0; i < polygon.Count(); i++) {
		const Vector3 & a = polygon[i];
		const Vector3 & b = polygon[(i + 1) % polygon.Count()];
		bool a_wet = (a.Z <= params.SeaLevel);
		bool b_wet = (b.Z <= params.SeaLevel);
		if (a_wet) clipped.Add(Vector3(a.X,a.Y,0.0f));
		if (a_wet != b_wet) {
			float t = (params.SeaLevel - a.Z) / (b.Z - a.Z);
			clipped.Add(Vector3(a.X + ((b.X - a.X) * t),a.Y + ((b.Y - a.Y) * t),0.0f));
		}
	}
	if (clipped.Count() < 3) return false;
	for (int i = 0; i < clipped.Count(); i++) area.Add_Station(clipped[i],0.0f);

	int id = WaterSystem::Add_Area(area);
	int index = WaterSystem::Get_Area_Count() - 1;
	OwnedWaterAreaIds.Add(id);
	WaterSystem::Shape_Terrain(index);
	WaterSystem::Stamp_Mask(index);
	WaterSystem::Build_Geometry(index);

	HydrologyFeature feature;
	feature.WaterAreaId	= id;
	feature.Name			= name;
	feature.Type			= HYDRO_FEATURE_COAST;
	feature.Enclosed		= true;
	feature.Length			= 0.0f;
	feature.Shore			= Classify_Shore(bounds.Center.X,y0,0.0f,0.0f);

	//	The sea is gone around, never bridged or forded -- Section 9.5's "long route around".
	HydrologyCrossingCandidate crossing;
	crossing.Position	= Vector3(bounds.Center.X,bounds.Center.Y,params.SeaLevel);
	crossing.Kind			= HYDRO_CROSSING_ROUTE_AROUND;
	feature.Crossings.Add(crossing);

	Features.Add(feature);
	return true;
}


static bool Height_Order_Before(int index_a,int index_b,const DynamicVectorClass<float> & height,int count_x)
{
	if (height[index_a] != height[index_b]) return (height[index_a] > height[index_b]);
	int py_a = index_a / count_x, px_a = index_a % count_x;
	int py_b = index_b / count_x, px_b = index_b % count_x;
	if (py_a != py_b) return (py_a < py_b);
	return (px_a < px_b);
}


static bool Accumulation_Order_Before(int index_a,int index_b,const DynamicVectorClass<float> & accumulation)
{
	if (accumulation[index_a] != accumulation[index_b]) return (accumulation[index_a] > accumulation[index_b]);
	return (index_a < index_b);
}


/***********************************************************************************************
 * Generate -- Phase 6 steps 1-11                                                                *
 *=============================================================================================*/
bool HydrologyGenerator::Generate(const HydrologyParams & params,StringClass * error_out)
{
	#define FAIL(msg) { if (error_out) *error_out = msg; Clear(); return false; }

	Clear();

	if (!WorldTerrainSystem::Has_Chunk_Registry())	FAIL("no terrain chunk registry -- generate macro terrain first");
	if (params.RiverWidth <= 0.0f)							FAIL("river width must be positive");
	if (params.StreamWidth <= 0.0f)						FAIL("stream width must be positive");
	if (params.StreamWidth > params.RiverWidth)			FAIL("stream width must not exceed river width");
	if (params.RiverAccumulationChunks <= 0.0f)			FAIL("river accumulation threshold must be positive");
	if (params.MaxRiverCount <= 0)							FAIL("max river count must be positive");

	WaterSystem::Define_Default_Water();		// add-or-replace by name; always safe to call again

	int count_x = WorldTerrainSystem::Get_Chunk_Count_X();
	int count_y = WorldTerrainSystem::Get_Chunk_Count_Y();
	if ((count_x <= 0) || (count_y <= 0))					FAIL("chunk registry is empty");

	int total = count_x * count_y;

	//
	//	Step 1: drainage.  One representative height per chunk, steepest-descent flow direction
	//	among the four neighbours (never into the cliff band -- step 2), then accumulation summed
	//	in one height-descending pass, the standard sorted-list D8 algorithm.
	//
	DynamicVectorClass<float>	height;
	DynamicVectorClass<bool>	cliff_band;
	DynamicVectorClass<int>	flow_target;
	DynamicVectorClass<float>	accumulation;
	DynamicVectorClass<bool>	claimed;

	for (int i = 0; i < total; i++) {
		height.Add(0.0f);
		cliff_band.Add(false);
		flow_target.Add(-1);
		accumulation.Add(1.0f);
		claimed.Add(false);
	}

	for (int py = 0; py < count_y; py++) {
		for (int px = 0; px < count_x; px++) {
			TerrainChunkInfoClass info;
			if (!WorldTerrainSystem::Get_Chunk_Info(px,py,&info)) continue;
			int index = (py * count_x) + px;
			height[index]		= (info.MinHeight + info.MaxHeight) * 0.5f;
			cliff_band[index]	= info.CliffBand || info.InteriorCliff;
		}
	}

	for (int py = 0; py < count_y; py++) {
		for (int px = 0; px < count_x; px++) {
			int index = (py * count_x) + px;
			if (cliff_band[index]) continue;

			int best = -1;
			float best_height = height[index];
			for (int d = 0; d < 4; d++) {
				int nx = px + NEIGHBOR_DX[d];
				int ny = py + NEIGHBOR_DY[d];
				if ((nx < 0) || (nx >= count_x) || (ny < 0) || (ny >= count_y)) continue;
				int nindex = (ny * count_x) + nx;
				if (cliff_band[nindex]) continue;		// step 2: never flow toward the cliff band
				if (height[nindex] < best_height) {
					best_height = height[nindex];
					best = nindex;
				}
			}
			flow_target[index] = best;		// -1 stays a sink/basin candidate
		}
	}

	DynamicVectorClass<int> order;
	for (int i = 0; i < total; i++) {
		if (!cliff_band[i]) order.Add(i);
	}
	for (int a = 1; a < order.Count(); a++) {
		int key = order[a];
		int b = a - 1;
		while ((b >= 0) && Height_Order_Before(key,order[b],height,count_x)) {
			order[b + 1] = order[b];
			b--;
		}
		order[b + 1] = key;
	}

	for (int oi = 0; oi < order.Count(); oi++) {
		int index = order[oi];
		int target = flow_target[index];
		if (target >= 0) {
			accumulation[target] += accumulation[index];
		}
	}

	/*
	**	Section 9.4 coastline, decided before any channel or basin so the sea takes precedence
	**	over both: a river reaching it ends at its mouth, and no basin forms inside it.  Seeds are
	**	the non-cliff chunks lying below sea level at the edge of the playable interior -- the
	**	field's own edge, or beside a cliff-band chunk.  A breadth-first flood (fixed neighbour
	**	order, so deterministic) then takes every connected below-sea-level chunk.  A low hollow
	**	cut off from the edge by higher ground stays dry: a basin's business, not the sea's.  With
	**	the coastline off this set is empty and everything below is exactly what it always was.
	*/
	DynamicVectorClass<bool> sea;
	for (int i = 0; i < total; i++) sea.Add(false);
	if (params.GenerateCoastline) {
		DynamicVectorClass<int> queue;
		for (int i = 0; i < total; i++) {
			if (cliff_band[i] || (height[i] >= params.SeaLevel)) continue;
			int px = i % count_x, py = i / count_x;
			bool at_edge = (px == 0) || (py == 0) || (px == count_x - 1) || (py == count_y - 1);
			for (int d = 0; (d < 4) && !at_edge; d++) {
				int nx = px + NEIGHBOR_DX[d], ny = py + NEIGHBOR_DY[d];
				if ((nx >= 0) && (nx < count_x) && (ny >= 0) && (ny < count_y) && cliff_band[(ny * count_x) + nx]) {
					at_edge = true;
				}
			}
			if (at_edge) { queue.Add(i); sea[i] = true; }
		}
		for (int head = 0; head < queue.Count(); head++) {
			int i = queue[head];
			int px = i % count_x, py = i / count_x;
			for (int d = 0; d < 4; d++) {
				int nx = px + NEIGHBOR_DX[d], ny = py + NEIGHBOR_DY[d];
				if ((nx < 0) || (nx >= count_x) || (ny < 0) || (ny >= count_y)) continue;
				int n = (ny * count_x) + nx;
				if (sea[n] || cliff_band[n] || (height[n] >= params.SeaLevel)) continue;
				queue.Add(n);
				sea[n] = true;
			}
		}
	}

	//
	//	Steps 3-4: river/stream sources.  A channel-eligible chunk that nothing else channel-
	//	eligible flows into is a headwater; sources are taken in accumulation-descending order,
	//	bounded at MaxRiverCount, the same bounded-output discipline every generator in this tree
	//	follows.
	//
	float stream_threshold = params.RiverAccumulationChunks * STREAM_ACCUMULATION_FRACTION;

	DynamicVectorClass<bool> is_downstream_of_channel;
	for (int i = 0; i < total; i++) is_downstream_of_channel.Add(false);
	for (int i = 0; i < total; i++) {
		if (cliff_band[i] || (accumulation[i] < stream_threshold)) continue;
		int target = flow_target[i];
		if (target >= 0) is_downstream_of_channel[target] = true;
	}

	DynamicVectorClass<int> sources;
	if (params.GenerateRivers) {
		for (int i = 0; i < total; i++) {
			if (cliff_band[i] || (accumulation[i] < stream_threshold)) continue;
			if (flow_target[i] < 0) continue;					// no downstream -- the basin pass alone covers it
			if (is_downstream_of_channel[i]) continue;		// not a headwater
			sources.Add(i);
		}
		for (int a = 1; a < sources.Count(); a++) {
			int key = sources[a];
			int b = a - 1;
			while ((b >= 0) && Accumulation_Order_Before(key,sources[b],accumulation)) {
				sources[b + 1] = sources[b];
				b--;
			}
			sources[b + 1] = key;
		}
	}

	//
	//	Steps 5, 7, 10, 12: carve, and terminate every accepted source, in accumulation order.
	//
	int river_count = 0;
	int basin_count = 0;
	DynamicVectorClass<int> selected_basins;
	for (int si = 0; (si < sources.Count()) && (river_count < params.MaxRiverCount); si++) {

		int start = sources[si];
		if (claimed[start] || sea[start]) continue;

		DynamicVectorClass<int> path;
		path.Add(start);
		claimed[start] = true;

		int cur = start;
		bool ended_at_sink = false;
		while (true) {
			int target = flow_target[cur];
			if (target < 0) { ended_at_sink = true; break; }
			if (claimed[target]) break;		// joins an already-built channel; stop, no duplicate coverage
			if (sea[target]) break;				// Section 9.4 river mouth: the channel ends at the coast
			path.Add(target);
			claimed[target] = true;
			cur = target;
		}

		if (path.Count() < 2) continue;

		DynamicVectorClass<Vector3>	points;
		DynamicVectorClass<float>		widths;
		float peak_accumulation = 0.0f;

		for (int i = 0; i < path.Count(); i++) {
			int idx = path[i];
			int px = idx % count_x;
			int py = idx / count_x;

			AABoxClass bounds;
			WorldTerrainSystem::Get_Patch_Bounds(px,py,&bounds);

			float h;
			if (!WorldTerrainSystem::Sample_Height(bounds.Center.X,bounds.Center.Y,&h)) {
				h = height[idx];
			}
			//	Monotonic non-increasing by construction (flow direction only ever chose a
			//	strictly lower chunk average), clamped defensively against the finer real
			//	terrain sample above disagreeing with that coarse average at any one point --
			//	the guarantee Check_River_Never_Climbs proves is one Build actually keeps.
			if ((i > 0) && (h > points[i - 1].Z)) {
				h = points[i - 1].Z;
			}
			points.Add(Vector3(bounds.Center.X,bounds.Center.Y,h));

			float t = accumulation[idx] / params.RiverAccumulationChunks;
			if (t < 0.0f) t = 0.0f;
			if (t > 1.0f) t = 1.0f;
			widths.Add(params.StreamWidth + ((params.RiverWidth - params.StreamWidth) * t));

			if (accumulation[idx] > peak_accumulation) peak_accumulation = accumulation[idx];
		}

		HydrologyFeatureType type = (peak_accumulation >= params.RiverAccumulationChunks) ?
												HYDRO_FEATURE_RIVER : HYDRO_FEATURE_STREAM;

		WaterAreaClass area;
		StringClass name;
		name.Format("hydrology_channel_%d",river_count);
		area.Set_Name(name.Peek_Buffer());
		area.Set_Definition((type == HYDRO_FEATURE_RIVER) ? "ow_water_river" : "ow_water_stream");
		area.Set_Closed(false);
		for (int i = 0; i < points.Count(); i++) {
			area.Add_Station(points[i],widths[i]);
		}

		int id = WaterSystem::Add_Area(area);
		int windex = WaterSystem::Get_Area_Count() - 1;
		OwnedWaterAreaIds.Add(id);
		WaterSystem::Shape_Terrain(windex);
		WaterSystem::Stamp_Mask(windex);
		WaterSystem::Build_Geometry(windex);

		HydrologyFeature feature;
		feature.WaterAreaId	= id;
		feature.Name			= name;
		feature.Type			= type;
		feature.Enclosed		= false;
		feature.Length			= area.Get_Length();

		int mid = points.Count() / 2;
		Vector3 dir;
		if (mid == 0)								dir = points[1] - points[0];
		else if (mid == (points.Count() - 1))	dir = points[mid] - points[mid - 1];
		else												dir = points[mid + 1] - points[mid - 1];
		float dir_length = dir.Length();
		dir = (dir_length > 0.0001f) ? (dir / dir_length) : Vector3(1.0f,0.0f,0.0f);
		Vector3 perp(-dir.Y,dir.X,0.0f);

		float mid_width = widths[mid];
		float bank_x = points[mid].X + (perp.X * ((mid_width * 0.5f) + 2.0f));
		float bank_y = points[mid].Y + (perp.Y * ((mid_width * 0.5f) + 2.0f));
		feature.Shore = Classify_Shore(bank_x,bank_y,mid_width,params.StreamWidth * FORD_WIDTH_FACTOR);

		HydrologyCrossingCandidate crossing;
		crossing.Position	= points[mid];
		crossing.Kind			= (mid_width <= (params.StreamWidth * FORD_WIDTH_FACTOR)) ?
										  HYDRO_CROSSING_FORD : HYDRO_CROSSING_BRIDGE;
		feature.Crossings.Add(crossing);

		Features.Add(feature);
		river_count++;

		//	Section 5.9.  A channel that ran out of downstream room (its last station could not
		//	flow anywhere without entering the cliff band, or is a genuine local minimum) gets a
		//	real, visible terminus rather than an unexplained stop.
		if (ended_at_sink && basin_count < params.MaxRiverCount) {
			int sink_idx = path[path.Count() - 1];
			if (Build_Basin_Feature(sink_idx % count_x,sink_idx / count_x,accumulation[sink_idx],params)) {
				selected_basins.Add(sink_idx);
				basin_count++;
			}
		}
	}

	//
	//	D-003: the map plan's WATER cells are carved first, in index order, so planned pools
	//	take the basin budget before incidental sinks do.  Adjacent planned cells are allowed
	//	(that is what makes a pool rather than a speckle); everything the basin builder
	//	refuses -- cliff band, sea, already claimed -- still refuses here.
	//
	if (MapPlanGenerator::Has_Plan()) {
		for (int i = 0; (i < total) && (basin_count < params.MaxRiverCount); i++) {
			if (cliff_band[i] || claimed[i] || sea[i]) continue;
			int px = i % count_x, py = i / count_x;
			AABoxClass bounds;
			if (!WorldTerrainSystem::Get_Patch_Bounds(px,py,&bounds)) continue;
			if (MapPlanGenerator::Get_Kind_At(bounds.Center.X,bounds.Center.Y) != MAP_PLAN_CELL_WATER) continue;
			if (Build_Basin_Feature(px,py,accumulation[i],params)) {
				selected_basins.Add(i);
				basin_count++;
				claimed[i] = true;
			}
		}
	}

	//
	//	The strongest standalone sinks get water.  A pond at every local minimum
	//	filled the native map with dozens of square water areas and little land.
	//
	DynamicVectorClass<int> standalone_basins;
	for (int i = 0; i < total; i++) {
		if (cliff_band[i] || claimed[i] || sea[i]) continue;
		if (flow_target[i] >= 0) continue;
		if (accumulation[i] < stream_threshold) continue;
		int insert = standalone_basins.Count();
		standalone_basins.Add(i);
		while (insert > 0 && Accumulation_Order_Before(i,standalone_basins[insert - 1],accumulation)) {
			standalone_basins[insert] = standalone_basins[insert - 1];
			insert--;
		}
		standalone_basins[insert] = i;
	}
	for (int c = 0; c < standalone_basins.Count() && basin_count < params.MaxRiverCount; c++) {
		int i = standalone_basins[c];
		int px = i % count_x, py = i / count_x;
		bool near_existing = false;
		for (int b = 0; b < selected_basins.Count(); b++) {
			int bx = selected_basins[b] % count_x, by = selected_basins[b] / count_x;
			if ((px >= bx - 1) && (px <= bx + 1) && (py >= by - 1) && (py <= by + 1)) {
				near_existing = true;
				break;
			}
		}
		if (near_existing) continue;
		if (Build_Basin_Feature(px,py,accumulation[i],params)) {
			selected_basins.Add(i);
			basin_count++;
			claimed[i] = true;
		}
	}

	//	The coast itself, one area per sea chunk in index order.
	for (int i = 0; i < total; i++) {
		if (!sea[i]) continue;
		if (Build_Coast_Feature(i % count_x,i / count_x,params)) {
			claimed[i] = true;
		}
	}

	//
	//	Per-area stamps populate the river mask. The shoreline layer reads its
	//	distance field, so rebuild it once after all water areas are known.
	if (Features.Count () > 0) {
		TerrainTextureSystem::Update_Water_Distance ();
	}

	//	Checksum: every feature's own type, shore, enclosed flag, length and crossing candidates.
	//
	{
		CRCEngine crc;
		for (int i = 0; i < Features.Count(); i++) {
			const HydrologyFeature & f = Features[i];
			int q_type		= (int)f.Type;
			int q_shore		= (int)f.Shore;
			int q_enclosed	= f.Enclosed ? 1 : 0;
			int q_length	= (int)(f.Length * CHECKSUM_QUANTUM);
			crc(&q_type,sizeof(q_type));
			crc(&q_shore,sizeof(q_shore));
			crc(&q_enclosed,sizeof(q_enclosed));
			crc(&q_length,sizeof(q_length));
			for (int c = 0; c < f.Crossings.Count(); c++) {
				int q_kind	= (int)f.Crossings[c].Kind;
				int q_x		= (int)(f.Crossings[c].Position.X * CHECKSUM_QUANTUM);
				int q_y		= (int)(f.Crossings[c].Position.Y * CHECKSUM_QUANTUM);
				crc(&q_kind,sizeof(q_kind));
				crc(&q_x,sizeof(q_x));
				crc(&q_y,sizeof(q_y));
			}
		}
		Checksum = (unsigned int)crc;
	}

	Built = true;

	WWDEBUG_SAY(("HydrologyGenerator: %d feature(s), checksum 0x%08X\r\n",Features.Count(),Checksum));

	#undef FAIL
	return true;
}


/***********************************************************************************************
 * Check_No_Cliff_Band_Overlap -- every owned water area's own station must be strictly interior *
 *=============================================================================================*/
int HydrologyGenerator::Check_No_Cliff_Band_Overlap(StringClass * report)
{
	int faults = 0;
	if (!WorldBoundsService::Is_Configured()) {
		return 0;
	}

	for (int i = 0; i < Features.Count(); i++) {
		const WaterAreaClass * area = WaterSystem::Find_Area(Features[i].Name.Peek_Buffer());
		if (area == nullptr) continue;

		for (int s = 0; s < area->Get_Station_Count(); s++) {
			const WaterStationClass & station = area->Get_Station(s);
			Vector2 xy(station.Position.X,station.Position.Y);
			if (WorldBoundsService::Classify_Point(xy) >= WORLD_ZONE_CLIFF_BAND) {
				faults++;
				if (report) {
					StringClass line;
					line.Format("hydrology feature '%s' station %d lies in the cliff band\r\n",
									Features[i].Name.Peek_Buffer(),s);
					*report += line;
				}
			}
		}
	}
	return faults;
}


/***********************************************************************************************
 * Check_River_Never_Climbs -- roadmap's own targeted validation, for real                       *
 *=============================================================================================*/
int HydrologyGenerator::Check_River_Never_Climbs(StringClass * report)
{
	int faults = 0;
	for (int i = 0; i < Features.Count(); i++) {
		if (Features[i].Enclosed) continue;

		const WaterAreaClass * area = WaterSystem::Find_Area(Features[i].Name.Peek_Buffer());
		if (area == nullptr) continue;

		for (int s = 1; s < area->Get_Station_Count(); s++) {
			if (area->Get_Station(s).Position.Z > area->Get_Station(s - 1).Position.Z) {
				faults++;
				if (report) {
					StringClass line;
					line.Format("hydrology feature '%s' station %d climbs uphill from station %d\r\n",
									Features[i].Name.Peek_Buffer(),s,s - 1);
					*report += line;
				}
			}
		}
	}
	return faults;
}


/***********************************************************************************************
 * Check_Lake_Within_Basin -- an enclosed surface must not stand above its own footprint's ground *
 *                                                                                             *
 *	Sampled at the footprint's own centre and four corners rather than re-deriving which chunk      *
 *	built it -- the area's own placed shape is the one source of truth for "where is this", and       *
 *	Get_Bounds needs no lookup back into the registry to answer it.  A tolerance of one basin          *
 *	margin absorbs the case Build_Basin_Feature's own margin exists for: a chunk flat enough that       *
 *	its sampled corners equal its true minimum exactly, which is a correct placement, not a fault.       *
 *=============================================================================================*/
int HydrologyGenerator::Check_Lake_Within_Basin(StringClass * report)
{
	int faults = 0;
	for (int i = 0; i < Features.Count(); i++) {
		if (!Features[i].Enclosed) continue;
		//	A coastal chunk is held at sea level by the sea it belongs to, not by its own rim.
		if (Features[i].Type == HYDRO_FEATURE_COAST) continue;

		const WaterAreaClass * area = WaterSystem::Find_Area(Features[i].Name.Peek_Buffer());
		if (area == nullptr) continue;

		AABoxClass bounds;
		if (!area->Get_Bounds(&bounds)) continue;

		float x0 = bounds.Center.X - bounds.Extent.X;
		float x1 = bounds.Center.X + bounds.Extent.X;
		float y0 = bounds.Center.Y - bounds.Extent.Y;
		float y1 = bounds.Center.Y + bounds.Extent.Y;

		float sample_x[5] = { bounds.Center.X, x0, x1, x0, x1 };
		float sample_y[5] = { bounds.Center.Y, y0, y0, y1, y1 };

		float max_ground = area->Get_Height();
		for (int s = 0; s < 5; s++) {
			float h;
			if (WorldTerrainSystem::Sample_Height(sample_x[s],sample_y[s],&h) && (h > max_ground)) {
				max_ground = h;
			}
		}

		if (area->Get_Height() > (max_ground + BASIN_HEIGHT_MARGIN)) {
			faults++;
			if (report) {
				StringClass line;
				line.Format("hydrology feature '%s' surface (%.2f) stands above its own basin rim (%.2f)\r\n",
								Features[i].Name.Peek_Buffer(),area->Get_Height(),max_ground);
				*report += line;
			}
		}
	}
	return faults;
}


/***********************************************************************************************
 * Check_Crossing_Candidates_Present -- Section 9.5                                              *
 *=============================================================================================*/
int HydrologyGenerator::Check_Crossing_Candidates_Present(StringClass * report)
{
	int faults = 0;
	for (int i = 0; i < Features.Count(); i++) {
		if (Features[i].Crossings.Count() == 0) {
			faults++;
			if (report) {
				StringClass line;
				line.Format("hydrology feature '%s' has no crossing candidate\r\n",Features[i].Name.Peek_Buffer());
				*report += line;
			}
		}
	}
	return faults;
}
