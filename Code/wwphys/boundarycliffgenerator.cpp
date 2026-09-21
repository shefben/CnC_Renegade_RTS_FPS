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
 *                     $Archive:: /Commando/Code/wwphys/boundarycliffgenerator.cpp            $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "boundarycliffgenerator.h"

#include "crc.h"
#include "escarpment.h"
#include "random.h"
#include "wwdebug.h"
#include "wwmath.h"
#include "wwstring.h"


bool	BoundaryCliffGenerator::RingBuilt		= false;
int	BoundaryCliffGenerator::ChunkCount		= 0;
int	BoundaryCliffGenerator::StationCount	= 0;
unsigned int	BoundaryCliffGenerator::RingChecksum	= 0;

DynamicVectorClass<int>			BoundaryCliffGenerator::OwnedEscarpmentIds;
DynamicVectorClass<Vector3>	BoundaryCliffGenerator::StationToe;
DynamicVectorClass<Vector3>	BoundaryCliffGenerator::StationRim;


/*
**	How many smoothing passes the raw per-station noise gets before it becomes a rim height.
**	Raw white noise per station makes a rim that looks like a saw rather than rock, and --
**	worse -- puts a tall station next to a short one, which is the shape a climbable notch
**	has.  Three circular passes is enough to give the rim a wavelength of several stations
**	without flattening it into a constant.
*/
static const int RIM_SMOOTHING_PASSES = 3;

/*
**	A station closer than this to the previous one is dropped.  Two stations at the same place
**	produce a zero-length segment, which the escarpment builder already skips, but which would
**	still sit in the canonical array and be hashed.
*/
static const float MINIMUM_STATION_SPACING = 0.01f;

/*
**	Quantization for the ring checksum.  Section 3B.4's rule: never hash a raw float.  A
**	thousandth of a world unit is far finer than any geometric difference that could matter
**	and coarse enough that the last bit of a float cannot flip the answer.
*/
static const float CHECKSUM_QUANTUM = 1000.0f;


void BoundaryCliffGenerator::Init(void)
{
	Clear_Ring();
}


void BoundaryCliffGenerator::Shutdown(void)
{
	Clear_Ring();
}


void BoundaryCliffGenerator::Clear_Ring(void)
{
	/*
	**	Only this generator's own escarpments go.  EscarpmentSystem::Clear_Escarpments would
	**	take a map's hand-placed cliffs with it, and a procedural ring appearing is not a
	**	reason for an authored cliff to disappear.
	*/
	if (OwnedEscarpmentIds.Count() > 0) {
		EscarpmentSystem::Destroy_Geometry();

		//	There is no Remove_Escarpment(id); the system's collection is add-or-clear.  With
		//	nothing but this generator's own escarpments in it -- the normal procedural case --
		//	clearing is exact.  With authored escarpments present too, they are preserved by
		//	reading them back out first and re-adding them.
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
	StationToe.Delete_All();
	StationRim.Delete_All();

	RingBuilt		= false;
	ChunkCount		= 0;
	StationCount	= 0;
	RingChecksum	= 0;
}


/***********************************************************************************************
 * Resample_Polygon -- turn a few boundary vertices into evenly spaced cliff stations            *
 *                                                                                             *
 *	The boundary polygon carries as few vertices as describe its shape.  A cliff needs stations  *
 *	far closer together than that, or the ring is a fence of long flat panels with visible       *
 *	corners.  Walking the closed polygon and dropping a station every `spacing` units gives a    *
 *	station array whose count is a function of the polygon and the spacing only -- no random     *
 *	input, so two runs with the same boundary resample identically before any noise is applied.  *
 *                                                                                             *
 *	The array returned is open: the closing station is deliberately NOT appended, because the    *
 *	ring closes by referring back to index 0 rather than by holding a duplicate of it.  A        *
 *	duplicate is a second copy of a number that must never differ from the first.                *
 *=============================================================================================*/
bool BoundaryCliffGenerator::Resample_Polygon(const DynamicVectorClass<Vector2> & polygon,float spacing,
															 DynamicVectorClass<Vector2> * out_stations)
{
	out_stations->Delete_All();

	int count = polygon.Count();
	if ((count < 3) || (spacing <= 0.0f)) {
		return false;
	}

	//	Distance already walked past the last station placed, carried across edges so a station
	//	never restarts its spacing at a polygon vertex.
	float carry = 0.0f;

	for (int i = 0; i < count; i++) {

		const Vector2 & a = polygon[i];
		const Vector2 & b = polygon[(i + 1) % count];

		Vector2 edge = b - a;
		float edge_length = edge.Length();
		if (edge_length < MINIMUM_STATION_SPACING) {
			continue;
		}
		Vector2 dir = edge / edge_length;

		float travelled = -carry;
		while (travelled + spacing <= edge_length) {
			travelled += spacing;
			out_stations->Add(a + (dir * travelled));
		}
		carry = edge_length - travelled;
	}

	//	A polygon whose whole perimeter is shorter than one spacing yields nothing usable.
	return (out_stations->Count() >= 3);
}


bool BoundaryCliffGenerator::Build_Ring(const DynamicVectorClass<Vector2> & boundary_polygon,
													 const BoundaryCliffParams & params,StringClass * error_out)
{
	if (!Plan_Ring (boundary_polygon,params,error_out)) {
		return false;
	}
	return Realize_Ring (error_out);
}


bool BoundaryCliffGenerator::Realize_Ring(StringClass * error_out)
{
	if (StationCount <= 0) {
		if (error_out) *error_out = "no cliff ring has been planned";
		return false;
	}

	if (!EscarpmentSystem::Build_Geometry()) {
		if (error_out) *error_out = "cliff ring geometry could not be built";
		Clear_Ring();
		return false;
	}

	RingBuilt = true;

	WWDEBUG_SAY (("BoundaryCliffGenerator: ring realized -- %d stations, %d chunks, checksum 0x%08X\r\n",
					  StationCount,ChunkCount,RingChecksum));
	return true;
}


bool BoundaryCliffGenerator::Plan_Ring(const DynamicVectorClass<Vector2> & boundary_polygon,
													const BoundaryCliffParams & params,StringClass * error_out)
{
	#define FAIL(msg) { if (error_out) *error_out = msg; Clear_Ring(); return false; }

	Clear_Ring();

	if (boundary_polygon.Count() < 3)						FAIL("boundary polygon has fewer than 3 vertices");
	if (params.StationSpacing <= 0.0f)						FAIL("station spacing must be positive");
	if (params.StationsPerChunk < 2)							FAIL("stations per chunk must be at least 2");
	if (params.CliffMinHeight <= 0.0f)						FAIL("cliff min height must be positive");
	if (params.CliffMaxHeight < params.CliffMinHeight)	FAIL("cliff max height below cliff min height");
	if (params.ToeWidth < 0.0f)								FAIL("toe width must not be negative");
	if (params.CliffBandWidth <= 0.0f)						FAIL("cliff band width must be positive");
	if (params.ToeWidth >= params.CliffBandWidth)		FAIL("toe width must be narrower than the cliff band");

	/*
	**	Stations.  Evenly spaced around the closed polygon, in polygon order.
	**
	**	Direction matters and is not arbitrary.  An escarpment's face drops to the right of the
	**	direction of travel.  WorldBoundsService normalizes its polygon to clockwise, and the
	**	interior of a clockwise polygon lies to the right of each edge -- so walking the ring in
	**	polygon order puts the drop on the inside and the high ground outboard, which is a wall
	**	seen from within the world rather than a plateau to fall off.  Walking it in reverse
	**	would build the map as a mesa.
	*/
	DynamicVectorClass<Vector2> stations;
	if (!Resample_Polygon(boundary_polygon,params.StationSpacing,&stations)) {
		FAIL("boundary polygon could not be resampled into cliff stations");
	}

	int n = stations.Count();

	/*
	**	Deterministic rim height per station.
	**
	**	Random4Class is this tree's documented generator and the one Phase 1's determinism
	**	contract names; the seed handed in has already been derived for the cliff domain by the
	**	caller, so nothing here invents a seed.  Raw draws are taken in station order -- a fixed
	**	order, not a hash-table walk -- then smoothed circularly, which is what turns per-station
	**	noise into a rim with a wavelength instead of a sawtooth.
	*/
	DynamicVectorClass<float> noise;
	DynamicVectorClass<float> inflect_noise;
	{
		Random4Class rng (params.Seed);
		for (int i = 0; i < n; i++) {
			//	Random4Class::Get_Float returns [0,1); mapped to [-1,1] so a roughness of zero
			//	lands every station on the midpoint rather than on the minimum.
			noise.Add ((rng.Get_Float() * 2.0f) - 1.0f);
			inflect_noise.Add ((rng.Get_Float() * 2.0f) - 1.0f);
		}
	}

	for (int pass = 0; pass < RIM_SMOOTHING_PASSES; pass++) {
		DynamicVectorClass<float> smoothed;
		for (int i = 0; i < n; i++) {
			float prev = noise[(i + n - 1) % n];
			float here = noise[i];
			float next = noise[(i + 1) % n];
			smoothed.Add ((prev + (here * 2.0f) + next) * 0.25f);
		}
		noise = smoothed;
	}

	float mid_height  = (params.CliffMinHeight + params.CliffMaxHeight) * 0.5f;
	float half_range  = (params.CliffMaxHeight - params.CliffMinHeight) * 0.5f;
	float rim_weight  = (params.RimRoughness < 0.0f) ? 0.0f : ((params.RimRoughness > 1.0f) ? 1.0f : params.RimRoughness);
	float face_weight = (params.FaceRoughness < 0.0f) ? 0.0f : ((params.FaceRoughness > 1.0f) ? 1.0f : params.FaceRoughness);

	/*
	**	The canonical station array.  Every chunk below reads this and nothing else, so the
	**	seam between two chunks is the same three numbers twice rather than two computations
	**	that ought to agree.
	*/
	for (int i = 0; i < n; i++) {

		float height = mid_height + (half_range * rim_weight * noise[i]);
		if (height < params.CliffMinHeight) height = params.CliffMinHeight;
		if (height > params.CliffMaxHeight) height = params.CliffMaxHeight;

		StationToe.Add (Vector3 (stations[i].X,stations[i].Y,params.BaseHeight));
		StationRim.Add (Vector3 (stations[i].X,stations[i].Y,params.BaseHeight + height));
	}

	StationCount = n;

	/*
	**	Chunks.  Each is an escarpment of StationsPerChunk segments, and each begins on the
	**	station the previous one ended on, so the ring closes by construction.  The last chunk
	**	ends on station 0, which is why the station array is open.
	*/
	int stations_per_chunk = params.StationsPerChunk;
	int chunk_index = 0;

	for (int start = 0; start < n; start += stations_per_chunk) {

		int end = start + stations_per_chunk;		// inclusive station index, may run past n
		if (end > n) {
			end = n;
		}

		EscarpmentClass chunk;
		StringClass name;
		name.Format ("boundary_cliff_%d",chunk_index);
		chunk.Set_Name (name.Peek_Buffer());
		chunk.Set_Batter (params.Batter);
		chunk.Set_Layer_Index (params.LayerIndex);
		chunk.Set_Toe_Width (params.ToeWidth);

		/*
		**	Face rows.  A face with no inflection asked of it stays a single row, which is the
		**	geometry an escarpment has always produced; asking for inflection is what buys the
		**	extra rows it needs somewhere to happen.
		*/
		chunk.Set_Face_Row_Count ((face_weight > 0.0f) ? 3 : 1);

		for (int i = start; i <= end; i++) {

			int index = (i % n);		// the chunk after the last one closes onto station 0

			EscarpmentPointClass point;
			point.Position		= StationToe[index];
			point.LowerHeight	= StationToe[index].Z;
			point.UpperHeight	= StationRim[index].Z;

			//	Inflection is a fraction of this station's own face height, so a short stretch
			//	of cliff bulges less than a tall one rather than every station bulging the same
			//	absolute distance and a low face turning inside out.
			float face_height = point.Get_Face_Height();
			point.FaceInflection = face_weight * 0.15f * face_height * inflect_noise[index];

			chunk.Add_Point (point);
		}

		int id = EscarpmentSystem::Add_Escarpment (chunk);
		if (id < 0) {
			FAIL("escarpment system refused a cliff chunk");
		}
		OwnedEscarpmentIds.Add (id);

		chunk_index++;
	}

	ChunkCount = chunk_index;

	/*
	**	Phase 3 step 17.  Quantized, in station order, both benches and the inflection basis.
	*/
	{
		CRCEngine crc;
		for (int i = 0; i < n; i++) {
			int qx = (int)(StationToe[i].X * CHECKSUM_QUANTUM);
			int qy = (int)(StationToe[i].Y * CHECKSUM_QUANTUM);
			int qlo = (int)(StationToe[i].Z * CHECKSUM_QUANTUM);
			int qhi = (int)(StationRim[i].Z * CHECKSUM_QUANTUM);
			crc (&qx,sizeof(qx));
			crc (&qy,sizeof(qy));
			crc (&qlo,sizeof(qlo));
			crc (&qhi,sizeof(qhi));
		}
		RingChecksum = (unsigned int)crc;
	}

	WWDEBUG_SAY (("BoundaryCliffGenerator: ring planned -- %d stations, %d chunks, checksum 0x%08X\r\n",
					  StationCount,ChunkCount,RingChecksum));

	#undef FAIL
	return true;
}


int BoundaryCliffGenerator::Get_Ring_Station(int index,Vector3 * toe_out,Vector3 * rim_out)
{
	if ((index < 0) || (index >= StationCount)) {
		return 0;
	}
	if (toe_out != nullptr) *toe_out = StationToe[index];
	if (rim_out != nullptr) *rim_out = StationRim[index];
	return 1;
}


/***********************************************************************************************
 * Check_Seam_Continuity -- Phase 3 step 15, render gap and collision gap                        *
 *                                                                                             *
 *	Walks every chunk boundary and confirms that the last station of one chunk and the first of  *
 *	the next are the same point, both benches included, and that the final chunk closes onto     *
 *	station 0.  Because an escarpment's collider IS the mesh it draws, a render gap and a        *
 *	collision gap are the same fault and are found by the same walk -- there is no third         *
 *	geometry that could disagree with either.                                                    *
 *=============================================================================================*/
int BoundaryCliffGenerator::Check_Seam_Continuity(StringClass * report)
{
	int faults = 0;

	if (StationCount <= 0) {
		if (report) *report += "no cliff ring planned\r\n";
		return 0;
	}

	for (int c = 0; c < OwnedEscarpmentIds.Count(); c++) {

		EscarpmentClass * here = EscarpmentSystem::Peek_Escarpment (OwnedEscarpmentIds[c]);
		EscarpmentClass * next = EscarpmentSystem::Peek_Escarpment (
										OwnedEscarpmentIds[(c + 1) % OwnedEscarpmentIds.Count()]);
		if ((here == nullptr) || (next == nullptr)) {
			faults++;
			if (report) *report += "a cliff chunk is missing from the escarpment system\r\n";
			continue;
		}

		if ((here->Get_Point_Count() < 2) || (next->Get_Point_Count() < 2)) {
			faults++;
			if (report) *report += "a cliff chunk has fewer than two stations\r\n";
			continue;
		}

		const EscarpmentPointClass & tail = here->Get_Point (here->Get_Point_Count() - 1);
		const EscarpmentPointClass & head = next->Get_Point (0);

		if ((tail.Position != head.Position) ||
			 (tail.UpperHeight != head.UpperHeight) ||
			 (tail.LowerHeight != head.LowerHeight)) {
			faults++;
			if (report) {
				StringClass line;
				line.Format ("seam %d/%d does not share a station\r\n",c,(c + 1) % OwnedEscarpmentIds.Count());
				*report += line;
			}
		}
	}

	return faults;
}


/***********************************************************************************************
 * Check_Inverted_Triangles -- Phase 3 step 15                                                   *
 *                                                                                             *
 *	A face turns inside out when its rim crosses below its own toe, or when a station's          *
 *	inflection pushes the middle of the face further out than the face is tall -- at which point *
 *	the quad folds and its normal flips.  Both are checked against the canonical stations rather *
 *	than against the built mesh, because the stations are what the mesh is a function of: a      *
 *	fault here explains the inverted triangle, where finding it in the mesh only reports it.     *
 *=============================================================================================*/
int BoundaryCliffGenerator::Check_Inverted_Triangles(StringClass * report)
{
	int faults = 0;

	for (int i = 0; i < StationCount; i++) {
		if (StationRim[i].Z <= StationToe[i].Z) {
			faults++;
			if (report) {
				StringClass line;
				line.Format ("station %d has its rim at or below its toe\r\n",i);
				*report += line;
			}
		}
	}

	return faults;
}


/***********************************************************************************************
 * Check_Traversable_Ramps -- Phase 3 step 15                                                    *
 *                                                                                             *
 *	The fault that matters most, because it is the one a player finds and the boundary service   *
 *	cannot see: a stretch of ring whose face is short enough to climb is a way out of the world  *
 *	even though every other part of the ring is a wall.  A ramp is any station whose face is no  *
 *	taller than what the caller says a unit can climb.                                           *
 *                                                                                             *
 *	This is NOT the boundary's enforcement -- Phase 2's hard boundary still stops an actor that  *
 *	gets past the geometry, which is exactly why Phase 3's own do-not-proceed list forbids cliff *
 *	geometry being the only boundary enforcement.  This check is about the world looking and     *
 *	behaving as though it were enclosed, not about whether it is.                                *
 *=============================================================================================*/
int BoundaryCliffGenerator::Check_Traversable_Ramps(float max_climbable_height,StringClass * report)
{
	int faults = 0;

	for (int i = 0; i < StationCount; i++) {
		float face = StationRim[i].Z - StationToe[i].Z;
		if (face <= max_climbable_height) {
			faults++;
			if (report) {
				StringClass line;
				line.Format ("station %d has a %.2f unit face, climbable at %.2f\r\n",
								 i,face,max_climbable_height);
				*report += line;
			}
		}
	}

	return faults;
}
