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
 *                     $Archive:: /Commando/Code/wwphys/hybridplanning.cpp                    $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#include "hybridplanning.h"
#include "crc.h"
#include "wwdebug.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>


static inline float Max2(float a,float b)			{ return (a > b) ? a : b; }
static inline float Min2(float a,float b)			{ return (a < b) ? a : b; }
static inline float Clamp3(float v,float lo,float hi)	{ return (v < lo) ? lo : ((v > hi) ? hi : v); }


/*
**	Section 0.3
*/
float Hybrid_Round_Half_Up(float value)
{
	return (float)::floor((double)value + 0.5);
}


int Hybrid_Quantize_To_Int(float value,float quantum)
{
	return (int)::floor((double)value / (double)quantum + 0.5);
}


float Hybrid_Quantize(float value,float quantum)
{
	return (float)((double)Hybrid_Quantize_To_Int(value,quantum) * (double)quantum);
}


unsigned int Hybrid_Key(const char * name)
{
	if (name == nullptr) {
		return 0;
	}
	return CRC::String(name,0);
}


float Hybrid_Hash01(unsigned int domain_seed,unsigned int k0,unsigned int k1,unsigned int k2)
{
	/*
	** Keys go in as explicit little-endian bytes so the hash does not depend on host byte order.
	*/
	unsigned char bytes[12];
	const unsigned int keys[3] = { k0, k1, k2 };
	for (int k = 0; k < 3; ++k) {
		bytes[k * 4 + 0] = (unsigned char)(keys[k] & 0xFF);
		bytes[k * 4 + 1] = (unsigned char)((keys[k] >> 8) & 0xFF);
		bytes[k * 4 + 2] = (unsigned char)((keys[k] >> 16) & 0xFF);
		bytes[k * 4 + 3] = (unsigned char)((keys[k] >> 24) & 0xFF);
	}
	const unsigned int crc = CRC::Memory(bytes,sizeof(bytes),domain_seed);
	return (float)((crc >> 8) & 0x00FFFFFFu) / 16777216.0f;
}


int Hybrid_Pick_Index(int count,float h)
{
	if (count <= 0) {
		return 0;
	}
	int index = (int)::floor((double)h * (double)count);
	if (index < 0) {
		index = 0;
	}
	return (index < count - 1) ? index : (count - 1);
}


int Hybrid_Pick_Sign(float h)
{
	return (h < 0.5f) ? -1 : 1;
}


float Hybrid_Range(float a,float b,float h,float quantum)
{
	return Hybrid_Quantize(a + (b - a) * h,quantum);
}


/***********************************************************************************************
 * Hybrid_Sample_Centripetal_Catmull_Rom -- the one spline sampler the plan's curves use        *
 *                                                                                             *
 * Section 8 Phases 5, 6 and 10 all say "centripetal Catmull-Rom through those controls,        *
 * sampled at ..."; this is that one implementation, so no phase grows a second spline rule.    *
 *                                                                                             *
 * The curve interpolates *every* control, which uniform Catmull-Rom does not do at the two     *
 * ends, so the ends are reflected into phantom controls (`P0 + (P0-P1)` and `P3 + (P3-P2)`).   *
 * Knots are centripetal -- `t += sqrt(chord)` -- which is what keeps a tight interior control  *
 * from producing the cusp/self-intersection uniform parameterization would.                    *
 *                                                                                             *
 * Output points are resampled to `spacing` by accumulated chord length, always include both    *
 * endpoints and are quantized to `Q`, so the accepted polyline is the stored polyline.         *
 *=============================================================================================*/
bool Hybrid_Sample_Centripetal_Catmull_Rom(const Vector2 * controls,int control_count,float spacing,
																DynamicVectorClass<Vector2> * out_points,
																DynamicVectorClass<float> * out_stations)
{
	if (out_points != nullptr) out_points->Delete_All();
	if (out_stations != nullptr) out_stations->Delete_All();

	if ((controls == nullptr) || (control_count < 2) || (spacing <= 0.0f) || (out_points == nullptr)) {
		return false;
	}

	/*
	**	Reflected phantom ends, so segment 0 and segment n-2 are ordinary interior segments.
	*/
	/*
	**	Reflected phantoms make the knot vector two longer than the control set.  The cap used to be 16,
	**	written when the only caller was Section 10's line features with their four controls; Phase 10's
	**	road splines carry one control per direction change of an A* chain and routinely pass twenty, so a
	**	curve that long was refused as "degenerate input" and every such road failed to convert.  The cap
	**	is now only what the scratch knot vector can hold, which is a memory bound rather than a statement
	**	about any plan curve's shape.
	*/
	const int total = control_count + 2;
	if (total > HYBRID_MAX_SPLINE_KNOTS) return false;

	Vector2 knots[HYBRID_MAX_SPLINE_KNOTS];
	knots[0] = controls[0] + (controls[0] - controls[1]);
	for (int i = 0; i < control_count; ++i) {
		knots[i + 1] = controls[i];
	}
	knots[total - 1] = controls[control_count - 1] + (controls[control_count - 1] - controls[control_count - 2]);

	//	Centripetal knot parameterization: alpha = 0.5.  The knot-parameter vector is as long as the knot
	//	vector itself -- it was a second fixed 16 that the cap change left behind, and a longer curve then
	//	wrote past it.
	float t[HYBRID_MAX_SPLINE_KNOTS];
	t[0] = 0.0f;
	for (int i = 1; i < total; ++i) {
		const float chord = (knots[i] - knots[i - 1]).Length();
		float step = (float)::sqrt((double)chord);
		if (step < 1e-4f) step = 1e-4f;			// a degenerate chord must not collapse the knot vector
		t[i] = t[i - 1] + step;
	}

	/*
	**	Dense evaluation first, then one arc-length resample.  16 substeps per segment is far finer
	**	than any plan spacing, so the resampled polyline is stable without an adaptive search.
	*/
	const int substeps = 16;
	DynamicVectorClass<Vector2> dense;
	dense.Add(knots[1]);

	for (int seg = 1; seg <= total - 3; ++seg) {

		const Vector2 & p0 = knots[seg - 1];
		const Vector2 & p1 = knots[seg];
		const Vector2 & p2 = knots[seg + 1];
		const Vector2 & p3 = knots[seg + 2];

		const float span = t[seg + 1] - t[seg];
		if (span <= 0.0f) continue;

		//	Non-uniform Catmull-Rom tangents, then Hermite inside the segment.
		const Vector2 m1 = (p2 - p0) * (1.0f / (t[seg + 1] - t[seg - 1]));
		const Vector2 m2 = (p3 - p1) * (1.0f / (t[seg + 2] - t[seg]));

		for (int step = 1; step <= substeps; ++step) {
			const float s = (float)step / (float)substeps;
			const float s2 = s * s;
			const float s3 = s2 * s;
			const float h00 = (2.0f * s3) - (3.0f * s2) + 1.0f;
			const float h10 = s3 - (2.0f * s2) + s;
			const float h01 = (-2.0f * s3) + (3.0f * s2);
			const float h11 = s3 - s2;
			dense.Add((p1 * h00) + (m1 * (h10 * span)) + (p2 * h01) + (m2 * (h11 * span)));
		}
	}

	if (dense.Count() < 2) return false;

	//	Arc-length resample at `spacing`, endpoints included.
	float station = 0.0f;
	float next = 0.0f;
	out_points->Add(Vector2(Hybrid_Quantize_Position(dense[0].X),Hybrid_Quantize_Position(dense[0].Y)));
	if (out_stations != nullptr) out_stations->Add(0.0f);
	next += spacing;

	for (int i = 1; i < dense.Count(); ++i) {

		const float seg_length = (dense[i] - dense[i - 1]).Length();
		if (seg_length <= 0.0f) continue;

		while (next <= (station + seg_length)) {
			const float f = (next - station) / seg_length;
			const Vector2 point = dense[i - 1] + ((dense[i] - dense[i - 1]) * f);
			out_points->Add(Vector2(Hybrid_Quantize_Position(point.X),Hybrid_Quantize_Position(point.Y)));
			if (out_stations != nullptr) out_stations->Add(Hybrid_Quantize_Position(next));
			next += spacing;
		}
		station += seg_length;
	}

	//	The final control is always a sample, however the spacing landed.
	const Vector2 last(Hybrid_Quantize_Position(dense[dense.Count() - 1].X),
							 Hybrid_Quantize_Position(dense[dense.Count() - 1].Y));
	const Vector2 & tail = (*out_points)[out_points->Count() - 1];
	if (((last - tail).Length()) > (0.25f * spacing)) {
		out_points->Add(last);
		if (out_stations != nullptr) out_stations->Add(Hybrid_Quantize_Position(station));
	} else if (out_stations != nullptr) {
		(*out_stations)[out_stations->Count() - 1] = Hybrid_Quantize_Position(station);
	}

	return (out_points->Count() >= 2);
}


void HybridHashStream::Add_Bytes(const unsigned char * bytes,int count)
{
	if (count > 0) {
		Crc = CRC::Memory(const_cast<unsigned char *>(bytes),(size_t)count,Crc);
	}
}


void HybridHashStream::Add_Uint(unsigned int value)
{
	unsigned char bytes[4];
	bytes[0] = (unsigned char)(value & 0xFF);
	bytes[1] = (unsigned char)((value >> 8) & 0xFF);
	bytes[2] = (unsigned char)((value >> 16) & 0xFF);
	bytes[3] = (unsigned char)((value >> 24) & 0xFF);
	Add_Bytes(bytes,4);
}


void HybridHashStream::Add_Quantized(float value,float quantum)
{
	const long long q = (long long)::floor((double)value / (double)quantum + 0.5);
	const unsigned long long u = (unsigned long long)q;
	unsigned char bytes[8];
	for (int i = 0; i < 8; ++i) {
		bytes[i] = (unsigned char)((u >> (i * 8)) & 0xFF);
	}
	Add_Bytes(bytes,8);
}


void HybridHashStream::Add_String(const char * value)
{
	/*
	** Length-prefixed so adjacent strings cannot alias ("ab","c" vs "a","bc").
	*/
	const int length = (value != NULL) ? (int)::strlen(value) : 0;
	Add_Int(length);
	Add_Bytes((const unsigned char *)value,length);
}


/*
**	Section 0.6
*/
const char * Hybrid_Size_Class_Name(HybridSizeClass size_class)
{
	switch (size_class) {
		case HYBRID_SIZE_SMALL:		return "SMALL";
		case HYBRID_SIZE_MEDIUM:	return "MEDIUM";
		case HYBRID_SIZE_LARGE:		return "LARGE";
		default:							return "UNKNOWN";
	}
}


HybridSizeClass Hybrid_Size_Class_For(float min_dimension)
{
	if (min_dimension < HYBRID_SIZE_MEDIUM_MIN_D) {
		return HYBRID_SIZE_SMALL;
	}
	if (min_dimension < HYBRID_SIZE_LARGE_MIN_D) {
		return HYBRID_SIZE_MEDIUM;
	}
	return HYBRID_SIZE_LARGE;
}


bool Hybrid_World_Size_Within_Limits(float width,float height)
{
	if (width <= 0.0f || height <= 0.0f) {
		return false;
	}
	if (width > HYBRID_MAX_WORLD_EXTENT || height > HYBRID_MAX_WORLD_EXTENT) {
		return false;
	}
	return ((double)width * (double)height) <= (double)HYBRID_MAX_WORLD_AREA;
}


/*
**	Section 0.5
*/
void Hybrid_Compute_Constants(float humvee_width,float world_units_per_meter,float chunk_size,
										float world_width,float world_height,float base_radius_cells,
										float min_base_expansion_area,HybridConstants * out)
{
	WWASSERT(out != NULL);
	HybridConstants & k = *out;

	k.WorldUnitsPerMeter = world_units_per_meter;
	k.H = humvee_width;
	k.C = Max2(0.35f * world_units_per_meter,0.15f * humvee_width);
	k.W = Max2(humvee_width + 2.0f * k.C,1.30f * humvee_width);
	k.A = Clamp3(chunk_size / 4.0f,8.0f,16.0f);
	k.S = Clamp3(chunk_size / 2.0f,16.0f,32.0f);
	k.Q = HYBRID_QUANTUM_POSITION;
	k.D = Min2(world_width,world_height);
	k.RBase = base_radius_cells * chunk_size;
	k.RExpand = (min_base_expansion_area > 0.0f) ? (float)::sqrt((double)min_base_expansion_area / 3.14159265358979323846) : 0.0f;
	k.RBaseProtected = Max2(k.RBase,k.RExpand);
	k.BHash = Max2(4.0f * k.A,2.0f * k.S);
	k.SizeClass = Hybrid_Size_Class_For(k.D);
}


/*
**	Section 7.1
*/
float Hybrid_Roadbed_Width(HybridRoadClass road_class,float w)
{
	return Hybrid_Road_Class_Spec(road_class).Roadbed_Width_W() * w;
}


float Hybrid_Road_Inner_Edge_Offset(HybridRoadClass road_class,float w)
{
	const HybridRoadClassSpec & spec = Hybrid_Road_Class_Spec(road_class);
	return -((0.5f * ((float)spec.Lanes + spec.MedianW)) + spec.ShoulderW) * w;
}


float Hybrid_Road_Outer_Edge_Offset(HybridRoadClass road_class,float w)
{
	const HybridRoadClassSpec & spec = Hybrid_Road_Class_Spec(road_class);
	return ((0.5f * ((float)spec.Lanes + spec.MedianW)) + spec.OuterShoulderW) * w;
}


float Hybrid_Road_Roadbed_Midpoint_Offset(HybridRoadClass road_class,float w)
{
	return 0.5f * (Hybrid_Road_Inner_Edge_Offset(road_class,w) + Hybrid_Road_Outer_Edge_Offset(road_class,w));
}


/*
**	Section 12.6.6 -- every interchange constant from the Section-7 table and the plan's fixed numbers.
*/
void Hybrid_Interchange_Constants(float w,HybridInterchangeConstants * out)
{
	HybridInterchangeConstants & c = *out;
	const HybridRoadClassSpec & highway = Hybrid_Road_Class_Spec(HYBRID_ROAD_HIGHWAY_4);
	const HybridRoadClassSpec & ramp = Hybrid_Road_Class_Spec(HYBRID_ROAD_HIGHWAY_RAMP);
	const float highway_radius = highway.MinTurnRadiusW * w;
	const float pi = 3.14159265358979323846f;

	c.W							= w;
	c.MergeLength				= Max2(12.0f * w,2.0f * highway_radius);
	c.DivergeLength			= Max2(10.0f * w,1.5f * highway_radius);
	c.TaperLength				= 6.0f * w;
	c.MergeFullLength			= c.MergeLength - c.TaperLength;
	c.DivergeFullLength		= c.DivergeLength - c.TaperLength;
	c.GoreReserveMargin		= 2.0f * w;
	c.InterchangeGoreSpacing= 24.0f * w;
	c.GoreCandidates			= 32;
	c.ThetaC						= 15.0f * (pi / 180.0f);
	c.Psi							= 45.0f * (pi / 180.0f);
	c.RampRadius				= ramp.MinTurnRadiusW * w;
	c.ArmLength					= Max2(4.0f * w,c.RampRadius);
	c.T15							= c.RampRadius * (float)::tan((double)(7.5f * (pi / 180.0f)));
	c.T30							= c.RampRadius * (float)::tan((double)(15.0f * (pi / 180.0f)));
	c.DSep						= (0.5f * Hybrid_Roadbed_Width(HYBRID_ROAD_HIGHWAY_4,w)) + (0.5f * Hybrid_Roadbed_Width(HYBRID_ROAD_HIGHWAY_RAMP,w));
	c.DJoin						= ((0.5f * highway.MedianW) + 1.5f) * w;
	c.DAux						= c.DJoin + w;
	c.HHwy						= ((0.5f * highway.MedianW) + 2.0f) * w;
	c.EHwy						= 0.5f * Hybrid_Roadbed_Width(HYBRID_ROAD_HIGHWAY_4,w);
	c.HAux						= c.HHwy + w;
	c.EAux						= c.EHwy + w;
	c.HighwayShoulder			= highway.OuterShoulderW * w;
	c.HighwayJunctionReserve= Max2(4.0f * w,highway_radius);
}


void Hybrid_Ramp_Candidate(const HybridInterchangeConstants & c,int k,float d_lov,HybridRampCandidate * out)
{
	const float sin_c = (float)::sin((double)c.ThetaC);
	const float cos_c = (float)::cos((double)c.ThetaC);
	const float sin_p = (float)::sin((double)c.Psi);

	out->K						= k;
	out->ConvergenceLength	= c.T30 + c.T15 + ((float)(2 + k) * c.W);
	out->LocalOffsetMin		= c.DAux + (out->ConvergenceLength * sin_c) + ((c.ArmLength + c.T30) * sin_p);
	out->LocalOffset			= Max2(out->LocalOffsetMin,d_lov);
	out->GoreOffset			= c.T15 + (out->ConvergenceLength * cos_c) + out->LocalOffset - c.DAux -
									  (out->ConvergenceLength * sin_c);
	out->StraightHalfExtent	= out->GoreOffset + c.MergeLength + c.GoreReserveMargin;
}


void Hybrid_Ramp_Control_Polygon(const HybridInterchangeConstants & c,const HybridRampCandidate & candidate,int role,
											Vector2 polygon[4])
{
	const float sin_c = (float)::sin((double)c.ThetaC);
	const float cos_c = (float)::cos((double)c.ThetaC);
	const Vector2 local(0.0f,candidate.LocalOffset);
	const Vector2 gore(candidate.GoreOffset,c.DAux);
	const Vector2 v2(candidate.GoreOffset - c.T15,c.DAux);
	const Vector2 v1(v2.X - (candidate.ConvergenceLength * cos_c),v2.Y + (candidate.ConvergenceLength * sin_c));

	if (role == HYBRID_RAMP_ROLE_OFF) {
		polygon[0].Set(-gore.X,gore.Y);
		polygon[1].Set(-v2.X,v2.Y);
		polygon[2].Set(-v1.X,v1.Y);
		polygon[3] = local;
		return;
	}
	polygon[0] = local;
	polygon[1] = v1;
	polygon[2] = v2;
	polygon[3] = gore;
}


void Hybrid_Auxiliary_Lane_Section(const HybridInterchangeConstants & c,int role,float along,HybridAuxiliaryLaneSection * out)
{
	float u = 0.0f;
	bool full = true;
	if (role == HYBRID_RAMP_ROLE_OFF) {
		//	Diverge: taper first (normal -> full), then the full lane to the gore.
		if (along < c.TaperLength) {
			full	= false;
			u		= Clamp3(along / c.TaperLength,0.0f,1.0f);
		}
	} else {
		//	Merge: the full lane from the gore, then the taper (full -> normal).
		if (along > c.MergeFullLength) {
			full	= false;
			u		= Clamp3((along - c.MergeFullLength) / c.TaperLength,0.0f,1.0f);
		}
	}
	const float f = (3.0f * u * u) - (2.0f * u * u * u);
	float d = c.DAux;
	if (!full) d = (role == HYBRID_RAMP_ROLE_OFF) ? (c.DJoin + (c.W * f)) : (c.DAux - (c.W * f));

	out->Full	= full;
	out->U		= full ? 0.0f : u;
	out->D		= d;
	out->H		= d + (0.5f * c.W);
	out->E		= d + (c.EAux - c.DAux);
}


void Hybrid_Ramp_Gore_Shoulders(const HybridInterchangeConstants & c,float from_gore,float * inner_out,float * outer_out)
{
	const float u = 1.0f - Clamp3(from_gore / c.GoreReserveMargin,0.0f,1.0f);
	const float f = (3.0f * u * u) - (2.0f * u * u * u);
	const HybridRoadClassSpec & ramp = Hybrid_Road_Class_Spec(HYBRID_ROAD_HIGHWAY_RAMP);
	const float inner = ramp.ShoulderW * c.W;
	const float outer = ramp.OuterShoulderW * c.W;
	*inner_out = inner * (1.0f - f);
	*outer_out = outer + ((c.HighwayShoulder - outer) * f);
}


float Hybrid_Carriageway_Width(HybridRoadClass road_class,float w)
{
	const HybridRoadClassSpec & spec = Hybrid_Road_Class_Spec(road_class);
	return ((float)spec.Lanes + spec.MedianW) * w;
}


/*
**	Corridor helpers are undefined for HIGHWAY_RAMP (Section 12.6): a ramp is constructed, never searched, so
**	it has no corridor; both helpers answer 0 for it, which no search can use.
*/
float Hybrid_Preferred_Corridor_Half_Width(HybridRoadClass road_class,float s,float d)
{
	switch (road_class) {
		case HYBRID_ROAD_HIGHWAY_RAMP:		return 0.0f;
		case HYBRID_ROAD_HIGHWAY_4:
		case HYBRID_ROAD_PRIMARY_2:			return Max2(6.0f * s,0.08f * d);
		case HYBRID_ROAD_SECONDARY_2:			return Max2(5.0f * s,0.06f * d);
		case HYBRID_ROAD_SERVICE_DIRT_2:		return Max2(4.0f * s,0.05f * d);
		case HYBRID_ROAD_TRAIL_1:
		default:										return Max2(3.0f * s,0.04f * d);
	}
}


float Hybrid_Max_Corridor_Half_Width(HybridRoadClass road_class,float s,float d)
{
	if (road_class == HYBRID_ROAD_HIGHWAY_RAMP) return 0.0f;
	return Hybrid_Preferred_Corridor_Half_Width(road_class,s,d) + 2.0f * s;
}


HybridVerticalInterval Hybrid_Road_Vertical_Interval(float z,float max_cut_depth,float max_fill_height)
{
	HybridVerticalInterval interval;
	interval.Low = z - max_cut_depth;
	interval.High = z + max_fill_height;
	return interval;
}


float Hybrid_Road_Outer_Blend_Width(float w,float a)
{
	return Max2(w,a * 0.5f);
}


float Hybrid_Road_Deformation_Half_Width(HybridRoadClass road_class,float w,float a)
{
	return 0.5f * Hybrid_Roadbed_Width(road_class,w) + Hybrid_Road_Outer_Blend_Width(w,a);
}


float Hybrid_Interval_Gap(const HybridVerticalInterval & i0,const HybridVerticalInterval & i1)
{
	return Max2(0.0f,Max2(i0.Low - i1.High,i1.Low - i0.High));
}


bool Hybrid_Grade_Compatible(const HybridVerticalInterval & i0,const HybridVerticalInterval & i1,
									  float horizontal_distance,float max_grade_radians)
{
	const float gap = Hybrid_Interval_Gap(i0,i1);
	const float run = Max2(horizontal_distance,HYBRID_QUANTUM_POSITION);
	return (float)::atan((double)gap / (double)run) <= max_grade_radians;
}


/*
**	Section 2.3
*/
ProceduralPlanningSpatialHash::ProceduralPlanningSpatialHash(void) :
	BucketSize(1.0f),
	CountX(0),
	CountY(0),
	EntryCount(0),
	Buckets(NULL)
{
}


ProceduralPlanningSpatialHash::~ProceduralPlanningSpatialHash(void)
{
	delete [] Buckets;
}


void ProceduralPlanningSpatialHash::Init(float world_width,float world_height,float bucket_size)
{
	delete [] Buckets;

	BucketSize = (bucket_size > 0.0f) ? bucket_size : 1.0f;
	CountX = (int)::ceil((double)Max2(world_width,BucketSize) / (double)BucketSize);
	CountY = (int)::ceil((double)Max2(world_height,BucketSize) / (double)BucketSize);
	EntryCount = 0;
	Buckets = new DynamicVectorClass<unsigned int>[CountX * CountY];
}


void ProceduralPlanningSpatialHash::Reset(void)
{
	for (int i = 0; i < CountX * CountY; ++i) {
		Buckets[i].Reset_Active();
	}
	EntryCount = 0;
}


void ProceduralPlanningSpatialHash::Bucket_Range(float min_x,float min_y,float max_x,float max_y,
																 int * x0,int * y0,int * x1,int * y1) const
{
	/*
	** Bucket membership is decided on the quantized AABB so a sub-quantum wobble in the caller's
	** geometry can never move a feature between buckets.
	*/
	const double qx0 = (double)Hybrid_Quantize_Position(Min2(min_x,max_x));
	const double qy0 = (double)Hybrid_Quantize_Position(Min2(min_y,max_y));
	const double qx1 = (double)Hybrid_Quantize_Position(Max2(min_x,max_x));
	const double qy1 = (double)Hybrid_Quantize_Position(Max2(min_y,max_y));

	int bx0 = (int)::floor(qx0 / (double)BucketSize);
	int by0 = (int)::floor(qy0 / (double)BucketSize);
	int bx1 = (int)::floor(qx1 / (double)BucketSize);
	int by1 = (int)::floor(qy1 / (double)BucketSize);

	*x0 = (bx0 < 0) ? 0 : ((bx0 >= CountX) ? CountX - 1 : bx0);
	*y0 = (by0 < 0) ? 0 : ((by0 >= CountY) ? CountY - 1 : by0);
	*x1 = (bx1 < 0) ? 0 : ((bx1 >= CountX) ? CountX - 1 : bx1);
	*y1 = (by1 < 0) ? 0 : ((by1 >= CountY) ? CountY - 1 : by1);
}


void ProceduralPlanningSpatialHash::Insert(unsigned int stable_id,float min_x,float min_y,float max_x,float max_y)
{
	if (Buckets == NULL) {
		return;
	}
	int x0, y0, x1, y1;
	Bucket_Range(min_x,min_y,max_x,max_y,&x0,&y0,&x1,&y1);
	for (int y = y0; y <= y1; ++y) {
		for (int x = x0; x <= x1; ++x) {
			const int index = y * CountX + x;
			Buckets[index].Add(stable_id);
		}
	}
	++EntryCount;
}


static int Compare_Stable_Ids(const void * a,const void * b)
{
	const unsigned int ia = *(const unsigned int *)a;
	const unsigned int ib = *(const unsigned int *)b;
	return (ia < ib) ? -1 : ((ia > ib) ? 1 : 0);
}


int ProceduralPlanningSpatialHash::Query(float min_x,float min_y,float max_x,float max_y,
													  DynamicVectorClass<unsigned int> * out_ids) const
{
	WWASSERT(out_ids != NULL);
	out_ids->Reset_Active();
	if (Buckets == NULL) {
		return 0;
	}

	DynamicVectorClass<unsigned int> hits;
	int x0, y0, x1, y1;
	Bucket_Range(min_x,min_y,max_x,max_y,&x0,&y0,&x1,&y1);
	for (int y = y0; y <= y1; ++y) {
		for (int x = x0; x <= x1; ++x) {
			const DynamicVectorClass<unsigned int> & bucket = Buckets[y * CountX + x];
			for (int i = 0; i < bucket.Count(); ++i) {
				hits.Add(bucket[i]);
			}
		}
	}

	/*
	** A feature spanning several buckets appears once per bucket; sort then keep the first of each
	** run so the result is the distinct StableIds in ascending order.
	*/
	if (hits.Count() > 1) {
		::qsort(&hits[0],hits.Count(),sizeof(unsigned int),Compare_Stable_Ids);
	}
	for (int i = 0; i < hits.Count(); ++i) {
		if (i == 0 || hits[i] != hits[i - 1]) {
			out_ids->Add(hits[i]);
		}
	}
	return out_ids->Count();
}


/*
**	Section 16.1
*/
const char * Hybrid_Work_Counter_Name(HybridWorkCounter counter)
{
	switch (counter) {
		case HYBRID_WORK_SEMANTIC_CELLS:			return "semantic_cells";
		case HYBRID_WORK_ANALYSIS_CELLS:			return "analysis_cells";
		case HYBRID_WORK_ASTAR_EXPANSIONS:		return "astar_expansions";
		case HYBRID_WORK_ROUTED_EDGES:			return "routed_edges";
		case HYBRID_WORK_CROSSING_PROBLEMS:		return "crossing_problems";
		case HYBRID_WORK_CROSSING_STATIONS:		return "crossing_stations";
		case HYBRID_WORK_LOS_RAYS:					return "los_rays";
		case HYBRID_WORK_REFINEMENT_SAMPLES:	return "refinement_samples";
		case HYBRID_WORK_ATTEMPTS:					return "attempts";
		case HYBRID_WORK_PEAK_SCRATCH_BYTES:	return "peak_scratch_bytes";
		default:											return "unknown";
	}
}


void Hybrid_Get_Work_Budgets(HybridSizeClass size_class,int analysis_grid_cells,
									  int max_alternate_candidates_per_feature,HybridWorkBudgets * out)
{
	WWASSERT(out != NULL);
	static const int max_routed_edges[HYBRID_SIZE_CLASS_COUNT] = { 18, 32, 52 };
	static const int los_rays[HYBRID_SIZE_CLASS_COUNT] = { 512, 2048, 4096 };
	static const int arena_mib[HYBRID_SIZE_CLASS_COUNT] = { 128, 256, 512 };

	const int size = ((int)size_class >= 0 && size_class < HYBRID_SIZE_CLASS_COUNT) ? (int)size_class : (int)HYBRID_SIZE_LARGE;
	out->MaxRoutedEdges = max_routed_edges[size];
	out->AStarExpansionsPerRoute = 8 * analysis_grid_cells;
	out->AStarExpansionsPerAttempt = out->MaxRoutedEdges * out->AStarExpansionsPerRoute;
	out->CrossingStationsPerProblem = 64;
	out->CrossingProblemsPerEdge = 12;
	out->LosRays = los_rays[size];
	out->LocalRepairCandidates = 3 * max_alternate_candidates_per_feature;
	out->AttemptCount = 3;
	out->WorkerCount = 4;
	out->ScratchArenaBytes = arena_mib[size] * 1024 * 1024;
}


void HybridWorkCounters::Reset(void)
{
	for (int i = 0; i < HYBRID_WORK_COUNTER_COUNT; ++i) {
		Counts[i] = 0;
	}
}


/*
**	Section 16.3
*/
HybridScratchArena::HybridScratchArena(void) :
	Block(NULL),
	Capacity(0),
	Used(0),
	HighWater(0)
{
}


HybridScratchArena::~HybridScratchArena(void)
{
	Release();
}


bool HybridScratchArena::Reserve(int capacity_bytes)
{
	Release();
	if (capacity_bytes <= 0) {
		return false;
	}
	Block = (unsigned char *)::malloc((size_t)capacity_bytes);
	if (Block == NULL) {
		return false;
	}
	Capacity = capacity_bytes;
	return true;
}


void HybridScratchArena::Release(void)
{
	::free(Block);
	Block = NULL;
	Capacity = 0;
	Used = 0;
	HighWater = 0;
}


void * HybridScratchArena::Allocate(int bytes,int alignment)
{
	/*
	** alignment must be a power of two; it applies to the returned address, not just the offset.
	*/
	if (Block == NULL || bytes < 0 || alignment <= 0 || (alignment & (alignment - 1)) != 0) {
		return NULL;
	}
	const size_t base = (size_t)Block;
	const size_t address = (base + (size_t)Used + (size_t)(alignment - 1)) & ~(size_t)(alignment - 1);
	const size_t offset = address - base;
	if (offset > (size_t)Capacity || (size_t)bytes > (size_t)Capacity - offset) {
		return NULL;
	}
	Used = (int)offset + bytes;
	if (Used > HighWater) {
		HighWater = Used;
	}
	return Block + offset;
}
