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
 *                     $Archive:: /Commando/Code/wwphys/interiorcliffgenerator.cpp            $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "interiorcliffgenerator.h"

#include "crc.h"
#include "escarpment.h"
#include "terrainmask.h"
#include "worldboundsservice.h"
#include "worldterrainsystem.h"
#include "wwdebug.h"
#include "wwmath.h"


bool	InteriorCliffGenerator::Built			= false;
unsigned int	InteriorCliffGenerator::Checksum	= 0;

DynamicVectorClass<int>						InteriorCliffGenerator::OwnedEscarpmentIds;
DynamicVectorClass<InteriorCliffFeature>	InteriorCliffGenerator::Features;


/*
**	How far to either side of the path a bench height is sampled, world units.  Escarpment.h's
**	own Conform_To_Terrain defaults to 2.0; this is larger because a chunk here is a heightfield
**	patch (typically 16+ world units on a side), not the fine boundary-ring station spacing that
**	default was sized for -- sampling too close to the path risks both benches landing on the
**	same side of a broad ridge instead of straddling it.
*/
static const float BENCH_SAMPLE_INSET = 6.0f;

//	The smallest gap between a station's two benches this generator will build without nudging
//	it open.  A face this thin is not what a slope-threshold candidate should ever produce, but
//	nudging rather than failing keeps one unlucky station from discarding an otherwise-real run.
static const float MINIMUM_FACE_HEIGHT = 0.5f;

//	Seed offsets for the two hashed per-feature choices (type, traversability), kept well clear
//	of MacroTerrainGenerator's own small per-octave offsets (seed + o*101 for small o) and of
//	each other, so the two draws do not correlate.
static const int TYPE_HASH_OFFSET				= 5001;
static const int TRAVERSABILITY_HASH_OFFSET	= 6007;

//	Section 3B.4: never hash a raw float.
static const float CHECKSUM_QUANTUM = 1000.0f;

//	Fixed, deterministic neighbour order for the run walk -- east, north, west, south.  Any
//	fixed order works; what matters is that it never varies, so the same candidate map always
//	walks into the same run.
static const int NEIGHBOR_DX[4] = {  1, 0, -1, 0 };
static const int NEIGHBOR_DY[4] = {  0, 1,  0,-1 };


InteriorCliffParams::InteriorCliffParams(void) :
	Seed (1),
	SlopeThresholdFraction (0.92f),
	MaxBuildableSlopeRadians (0.4886922f),		// ~28 degrees, the schema's own default cap
	Density (0.16f),										// ProceduralWorldTerrainSettings::InteriorCliffDensity's own default
	MaxRunLength (10),
	Batter (0.1f),
	ToeWidth (6.0f),
	LayerIndex (-1)
{
}


bool InteriorCliffFeature::operator == (const InteriorCliffFeature & that) const
{
	return (EscarpmentId == that.EscarpmentId) &&
			 (Type == that.Type) &&
			 (Traversability == that.Traversability) &&
			 (Length == that.Length) &&
			 (MaxFaceHeight == that.MaxFaceHeight) &&
			 (BypassPoint == that.BypassPoint) &&
			 (HasBypassPoint == that.HasBypassPoint);
}


InteriorCliffFeature::InteriorCliffFeature(void) :
	EscarpmentId (-1),
	Type (INTERIOR_CLIFF_ESCARPMENT),
	Traversability (INTERIOR_CLIFF_NON_TRAVERSABLE),
	Length (0.0f),
	MaxFaceHeight (0.0f),
	BypassPoint (0.0f,0.0f,0.0f),
	HasBypassPoint (false)
{
}


void InteriorCliffGenerator::Init(void)
{
	Clear();
}


void InteriorCliffGenerator::Shutdown(void)
{
	Clear();
}


/***********************************************************************************************
 * Clear -- drop only this generator's own escarpments, mirroring BoundaryCliffGenerator          *
 *                                                                                             *
 *	The two generators share one EscarpmentSystem collection and neither owns the whole of it,     *
 *	so clearing here follows the exact "read the collection back minus what is owned, then         *
 *	re-add the rest" idiom BoundaryCliffGenerator::Clear_Ring established, so that dropping           *
 *	interior cliffs can never take the boundary ring, or any hand-placed escarpment, with it.         *
 *=============================================================================================*/
void InteriorCliffGenerator::Clear(void)
{
	if (OwnedEscarpmentIds.Count() > 0) {
		EscarpmentSystem::Destroy_Geometry();

		DynamicVectorClass<EscarpmentClass> keep;
		for (int i = 0; i < EscarpmentSystem::Get_Escarpment_Count(); i++) {
			EscarpmentClass * e = EscarpmentSystem::Peek_Escarpment(i);
			if (e == nullptr) continue;

			bool owned = false;
			for (int j = 0; j < OwnedEscarpmentIds.Count(); j++) {
				if (OwnedEscarpmentIds[j] == e->Get_ID()) { owned = true; break; }
			}
			if (!owned) {
				keep.Add(*e);
			}
		}

		EscarpmentSystem::Clear_Escarpments();
		for (int i = 0; i < keep.Count(); i++) {
			EscarpmentSystem::Add_Escarpment(keep[i]);
		}
		if (keep.Count() > 0) {
			EscarpmentSystem::Build_Geometry();
		}
	}

	OwnedEscarpmentIds.Delete_All();
	Features.Delete_All();

	Built		= false;
	Checksum	= 0;
}


bool InteriorCliffGenerator::Get_Feature(int index,InteriorCliffFeature * out)
{
	if ((index < 0) || (index >= Features.Count()) || (out == nullptr)) {
		return false;
	}
	*out = Features[index];
	return true;
}


/*
**	Sample both benches of one station directly off WorldTerrainSystem, offset to either side of
**	the local path direction.  This is the scoped equivalent of EscarpmentSystem::Conform_To_
**	Terrain, deliberately not that call -- see this file's header comment for why the shared,
**	collection-wide version cannot be used here without also re-sampling (and collapsing) the
**	boundary ring's authored benches.
*/
static void Sample_Station_Benches(const Vector3 & center,const Vector3 & dir,
												float fallback_lower,float fallback_upper,
												float * lower_out,float * upper_out)
{
	Vector3 perp (-dir.Y,dir.X,0.0f);
	float perp_length = perp.Length();
	if (perp_length > 0.0001f) {
		perp = perp / perp_length;
	} else {
		perp = Vector3 (1.0f,0.0f,0.0f);
	}

	float sample_a_x = center.X + (perp.X * BENCH_SAMPLE_INSET);
	float sample_a_y = center.Y + (perp.Y * BENCH_SAMPLE_INSET);
	float sample_b_x = center.X - (perp.X * BENCH_SAMPLE_INSET);
	float sample_b_y = center.Y - (perp.Y * BENCH_SAMPLE_INSET);

	float height_a,height_b;
	bool got_a = WorldTerrainSystem::Sample_Height (sample_a_x,sample_a_y,&height_a);
	bool got_b = WorldTerrainSystem::Sample_Height (sample_b_x,sample_b_y,&height_b);

	float lower,upper;
	if (got_a && got_b) {
		lower = (height_a < height_b) ? height_a : height_b;
		upper = (height_a < height_b) ? height_b : height_a;
	} else if (got_a) {
		lower = (height_a < fallback_upper) ? height_a : fallback_lower;
		upper = (height_a < fallback_upper) ? fallback_upper : height_a;
	} else if (got_b) {
		lower = (height_b < fallback_upper) ? height_b : fallback_lower;
		upper = (height_b < fallback_upper) ? fallback_upper : height_b;
	} else {
		lower = fallback_lower;
		upper = fallback_upper;
	}

	if (upper < (lower + MINIMUM_FACE_HEIGHT)) {
		upper = lower + MINIMUM_FACE_HEIGHT;
	}

	*lower_out = lower;
	*upper_out = upper;
}


/***********************************************************************************************
 * Generate -- Phase 5 steps 1-14                                                                *
 *=============================================================================================*/
bool InteriorCliffGenerator::Generate(const InteriorCliffParams & params,StringClass * error_out)
{
	#define FAIL(msg) { if (error_out) *error_out = msg; Clear(); return false; }

	Clear();

	if (!WorldTerrainSystem::Has_Chunk_Registry())		FAIL("no terrain chunk registry -- generate macro terrain first");
	if (params.MaxBuildableSlopeRadians <= 0.0f)			FAIL("max buildable slope must be positive");
	if ((params.SlopeThresholdFraction <= 0.0f) || (params.SlopeThresholdFraction > 1.0f))
																				FAIL("slope threshold fraction must be in (0,1]");
	if ((params.Density < 0.0f) || (params.Density > 1.0f))	FAIL("density must be in [0,1]");
	if (params.MaxRunLength < 2)									FAIL("max run length must be at least 2 chunks");

	int count_x = WorldTerrainSystem::Get_Chunk_Count_X();
	int count_y = WorldTerrainSystem::Get_Chunk_Count_Y();
	if ((count_x <= 0) || (count_y <= 0))						FAIL("chunk registry is empty");

	float threshold = params.MaxBuildableSlopeRadians * params.SlopeThresholdFraction;

	DynamicVectorClass<bool> candidate;
	DynamicVectorClass<bool> claimed;
	for (int i = 0; i < count_x * count_y; i++) {
		candidate.Add (false);
		claimed.Add (false);
	}

	for (int py = 0; py < count_y; py++) {
		for (int px = 0; px < count_x; px++) {
			TerrainChunkInfoClass info;
			if (!WorldTerrainSystem::Get_Chunk_Info (px,py,&info)) continue;
			if (info.CliffBand) continue;
			if (info.MaxSlopeRadians < threshold) continue;

			float draw = TerrainMaskClass::Hash_Value (px,py,(int)params.Seed);
			if (draw >= params.Density) continue;

			candidate[py * count_x + px] = true;
		}
	}

	int feature_index = 0;

	for (int start_py = 0; start_py < count_y; start_py++) {
		for (int start_px = 0; start_px < count_x; start_px++) {

			int start_index = start_py * count_x + start_px;
			if (!candidate[start_index] || claimed[start_index]) continue;

			/*
			**	Greedy walk, not a flood fill.  A 2D blob has no single ordered polyline; a walk
			**	that prefers to continue in the direction it is already going, and falls back to
			**	the fixed neighbour order only when it must turn, stays a simple path by
			**	construction -- see this file's header comment on what is and is not attempted.
			*/
			DynamicVectorClass<int> path_px;
			DynamicVectorClass<int> path_py;
			path_px.Add (start_px);
			path_py.Add (start_py);
			claimed[start_index] = true;

			int heading_dx = 0;
			int heading_dy = 0;
			bool have_heading = false;

			while (path_px.Count() < params.MaxRunLength) {

				int cur_px = path_px[path_px.Count() - 1];
				int cur_py = path_py[path_py.Count() - 1];

				int next_px = -1;
				int next_py = -1;
				int next_dx = 0;
				int next_dy = 0;

				if (have_heading) {
					int tx = cur_px + heading_dx;
					int ty = cur_py + heading_dy;
					if ((tx >= 0) && (tx < count_x) && (ty >= 0) && (ty < count_y)) {
						int idx = ty * count_x + tx;
						if (candidate[idx] && !claimed[idx]) {
							next_px = tx; next_py = ty; next_dx = heading_dx; next_dy = heading_dy;
						}
					}
				}

				if (next_px < 0) {
					for (int d = 0; d < 4; d++) {
						int tx = cur_px + NEIGHBOR_DX[d];
						int ty = cur_py + NEIGHBOR_DY[d];
						if ((tx < 0) || (tx >= count_x) || (ty < 0) || (ty >= count_y)) continue;
						int idx = ty * count_x + tx;
						if (candidate[idx] && !claimed[idx]) {
							next_px = tx; next_py = ty; next_dx = NEIGHBOR_DX[d]; next_dy = NEIGHBOR_DY[d];
							break;
						}
					}
				}

				if (next_px < 0) break;

				path_px.Add (next_px);
				path_py.Add (next_py);
				claimed[next_py * count_x + next_px] = true;
				heading_dx = next_dx;
				heading_dy = next_dy;
				have_heading = true;
			}

			int n = path_px.Count();
			if (n < 2) {
				//	A single unlucky chunk with no unclaimed steep neighbour.  Phase 5's own
				//	do-not-proceed forbids building it as a spike, so it is simply not a feature.
				continue;
			}

			/*
			**	Station positions and fallback bench heights, one per chunk in the walk.
			*/
			DynamicVectorClass<Vector3> centers;
			DynamicVectorClass<float>   fallback_lower;
			DynamicVectorClass<float>   fallback_upper;
			for (int i = 0; i < n; i++) {
				AABoxClass bounds;
				TerrainChunkInfoClass info;
				WorldTerrainSystem::Get_Patch_Bounds (path_px[i],path_py[i],&bounds);
				WorldTerrainSystem::Get_Chunk_Info (path_px[i],path_py[i],&info);
				centers.Add (Vector3 (bounds.Center.X,bounds.Center.Y,0.0f));
				fallback_lower.Add (info.MinHeight);
				fallback_upper.Add (info.MaxHeight);
			}

			EscarpmentClass chunk;
			StringClass name;
			name.Format ("interior_cliff_%d",feature_index);
			chunk.Set_Name (name.Peek_Buffer());
			chunk.Set_Batter (params.Batter);
			chunk.Set_Layer_Index (params.LayerIndex);
			chunk.Set_Toe_Width (params.ToeWidth);
			chunk.Set_Face_Row_Count (1);

			for (int i = 0; i < n; i++) {

				Vector3 dir;
				if (n == 1) {
					dir = Vector3 (1.0f,0.0f,0.0f);
				} else if (i == 0) {
					dir = centers[1] - centers[0];
				} else if (i == (n - 1)) {
					dir = centers[n - 1] - centers[n - 2];
				} else {
					dir = centers[i + 1] - centers[i - 1];
				}
				float dir_length = dir.Length();
				dir = (dir_length > 0.0001f) ? (dir / dir_length) : Vector3 (1.0f,0.0f,0.0f);

				float lower,upper;
				Sample_Station_Benches (centers[i],dir,fallback_lower[i],fallback_upper[i],&lower,&upper);

				EscarpmentPointClass point;
				point.Position		= centers[i];
				point.LowerHeight	= lower;
				point.UpperHeight	= upper;
				point.FaceInflection = 0.0f;
				chunk.Add_Point (point);
			}

			int id = EscarpmentSystem::Add_Escarpment (chunk);
			if (id < 0) {
				FAIL("escarpment system refused an interior cliff chunk");
			}
			OwnedEscarpmentIds.Add (id);

			EscarpmentClass * built = EscarpmentSystem::Peek_Escarpment (id);

			InteriorCliffFeature feature;
			feature.EscarpmentId	= id;
			feature.Length			= (built != nullptr) ? built->Get_Length() : 0.0f;
			feature.MaxFaceHeight	= (built != nullptr) ? built->Get_Max_Face_Height() : 0.0f;

			//	Section 8.1's type and Section 8.3's traversability -- both a deterministic draw
			//	off the run's own starting chunk, on their own hash offsets so neither draw
			//	correlates with the other or with the candidate-density draw above.
			float type_draw = TerrainMaskClass::Hash_Value (start_px,start_py,(int)params.Seed + TYPE_HASH_OFFSET);
			int type_pick = (int)(type_draw * (float)INTERIOR_CLIFF_TYPE_COUNT);
			if (type_pick >= INTERIOR_CLIFF_TYPE_COUNT) type_pick = INTERIOR_CLIFF_TYPE_COUNT - 1;
			feature.Type = (InteriorCliffType)type_pick;

			float trav_draw = TerrainMaskClass::Hash_Value (start_px,start_py,(int)params.Seed + TRAVERSABILITY_HASH_OFFSET);
			int trav_pick = (int)(trav_draw * (float)INTERIOR_CLIFF_TRAVERSABILITY_COUNT);
			if (trav_pick >= INTERIOR_CLIFF_TRAVERSABILITY_COUNT) trav_pick = INTERIOR_CLIFF_TRAVERSABILITY_COUNT - 1;
			feature.Traversability = (InteriorCliffTraversabilityType)trav_pick;

			if (feature.Traversability != INTERIOR_CLIFF_NON_TRAVERSABLE) {
				feature.HasBypassPoint	= true;
				feature.BypassPoint		= centers[n / 2];
			} else {
				feature.HasBypassPoint	= false;
			}

			//	Phase 5 step 12.  Every chunk this run occupies is non-buildable from here on,
			//	through the one door WorldTerrainSystem offers for it -- see that method's own
			//	comment on Section 3D.1 ownership.
			for (int i = 0; i < n; i++) {
				WorldTerrainSystem::Mark_Chunk_Interior_Cliff (path_px[i],path_py[i]);
			}

			Features.Add (feature);
			feature_index++;
		}
	}

	if (OwnedEscarpmentIds.Count() > 0) {
		EscarpmentSystem::Build_Geometry();
	}

	{
		CRCEngine crc;
		for (int i = 0; i < Features.Count(); i++) {
			const InteriorCliffFeature & f = Features[i];
			int q_type		= (int)f.Type;
			int q_trav		= (int)f.Traversability;
			int q_length	= (int)(f.Length * CHECKSUM_QUANTUM);
			int q_height	= (int)(f.MaxFaceHeight * CHECKSUM_QUANTUM);
			int q_has_bp	= f.HasBypassPoint ? 1 : 0;
			int q_bpx		= (int)(f.BypassPoint.X * CHECKSUM_QUANTUM);
			int q_bpy		= (int)(f.BypassPoint.Y * CHECKSUM_QUANTUM);
			crc (&q_type,sizeof(q_type));
			crc (&q_trav,sizeof(q_trav));
			crc (&q_length,sizeof(q_length));
			crc (&q_height,sizeof(q_height));
			crc (&q_has_bp,sizeof(q_has_bp));
			crc (&q_bpx,sizeof(q_bpx));
			crc (&q_bpy,sizeof(q_bpy));
		}
		Checksum = (unsigned int)crc;
	}

	Built = true;

	WWDEBUG_SAY (("InteriorCliffGenerator: %d feature(s), checksum 0x%08X\r\n",
					  Features.Count(),Checksum));

	#undef FAIL
	return true;
}


/***********************************************************************************************
 * Check_No_Cliff_Band_Overlap -- every owned escarpment station must be strictly interior        *
 *                                                                                             *
 *	The candidate gate already excludes CliffBand chunks; this confirms that held rather than       *
 *	assuming it, permissive when WorldBoundsService is unconfigured like every other query of        *
 *	it in this tree.                                                                                  *
 *=============================================================================================*/
int InteriorCliffGenerator::Check_No_Cliff_Band_Overlap(StringClass * report)
{
	int faults = 0;
	if (!WorldBoundsService::Is_Configured()) {
		return 0;
	}

	for (int i = 0; i < OwnedEscarpmentIds.Count(); i++) {
		EscarpmentClass * e = EscarpmentSystem::Peek_Escarpment (OwnedEscarpmentIds[i]);
		if (e == nullptr) continue;

		for (int p = 0; p < e->Get_Point_Count(); p++) {
			const EscarpmentPointClass & point = e->Get_Point (p);
			Vector2 xy (point.Position.X,point.Position.Y);
			if (WorldBoundsService::Classify_Point (xy) >= WORLD_ZONE_CLIFF_BAND) {
				faults++;
				if (report) {
					StringClass line;
					line.Format ("interior cliff %d station %d lies in the cliff band\r\n",OwnedEscarpmentIds[i],p);
					*report += line;
				}
			}
		}
	}
	return faults;
}


/***********************************************************************************************
 * Check_No_Single_Chunk_Spikes -- Phase 5's own do-not-proceed, verified rather than assumed     *
 *=============================================================================================*/
int InteriorCliffGenerator::Check_No_Single_Chunk_Spikes(StringClass * report)
{
	int faults = 0;
	for (int i = 0; i < Features.Count(); i++) {
		EscarpmentClass * e = EscarpmentSystem::Peek_Escarpment (Features[i].EscarpmentId);
		if ((e == nullptr) || (e->Get_Point_Count() < 2)) {
			faults++;
			if (report) {
				StringClass line;
				line.Format ("feature %d has fewer than two stations\r\n",i);
				*report += line;
			}
		}
	}
	return faults;
}


/***********************************************************************************************
 * Check_Bypass_Candidates_Present -- Section 8.3 step 9                                          *
 *=============================================================================================*/
int InteriorCliffGenerator::Check_Bypass_Candidates_Present(StringClass * report)
{
	int faults = 0;
	for (int i = 0; i < Features.Count(); i++) {
		const InteriorCliffFeature & f = Features[i];
		if ((f.Traversability != INTERIOR_CLIFF_NON_TRAVERSABLE) && !f.HasBypassPoint) {
			faults++;
			if (report) {
				StringClass line;
				line.Format ("feature %d names a bypass kind with no bypass point\r\n",i);
				*report += line;
			}
		}
	}
	return faults;
}


/***********************************************************************************************
 * Check_No_Inverted_Faces -- structurally prevented by Sample_Station_Benches, verified anyway   *
 *=============================================================================================*/
int InteriorCliffGenerator::Check_No_Inverted_Faces(StringClass * report)
{
	int faults = 0;
	for (int i = 0; i < OwnedEscarpmentIds.Count(); i++) {
		EscarpmentClass * e = EscarpmentSystem::Peek_Escarpment (OwnedEscarpmentIds[i]);
		if (e == nullptr) continue;

		for (int p = 0; p < e->Get_Point_Count(); p++) {
			const EscarpmentPointClass & point = e->Get_Point (p);
			if (point.UpperHeight <= point.LowerHeight) {
				faults++;
				if (report) {
					StringClass line;
					line.Format ("interior cliff %d station %d has its rim at or below its toe\r\n",OwnedEscarpmentIds[i],p);
					*report += line;
				}
			}
		}
	}
	return faults;
}
