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
 *                     $Archive:: /Commando/Code/wwphys/hybridlandforms.cpp                   $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Hybrid world generator rewrite, item I6 -- runtime Phase 5, the vector landforms.            *
 *                                                                                             *
 * Order is the plan's order and nothing here reorders it: archetype principal features first    *
 * (they carry Section-6 axis/crossing constraints and are never discarded), then the remaining  *
 * required canyon/ravine systems, the explicit cliff runs, the remaining ridges, the hills and  *
 * finally the optional plateau conversion.  Every generic feature goes through the same         *
 * six-step candidate process, and a retry moves a feature without ever redrawing its            *
 * dimensions, which is what makes a seed's geography reproducible.                             *
 *                                                                                             *
 * Two Section-8 Phase-5 rules resolve ambiguities the geometry table leaves open, both recorded *
 * in docs/procedural_world/HybridLandforms.md:                                                 *
 *                                                                                             *
 *   - the table gives a canyon a centerline length but never gives one for a ravine, so the     *
 *     ravine length is the canyon row scaled by the table's own canyon-to-ravine floor ratio     *
 *     (2/3), and a Section-6.8 attached branch is that ravine range scaled once more;           *
 *   - Section 10.2 gives a cliff only an edge width and a rock/talus transition width, so the   *
 *     cliff field uses no other length scale: it rises across the talus toe and the face, holds *
 *     full height across one talus width of shelf and returns to zero across one more.  That    *
 *     keeps the field compactly supported the way Section 10.4 requires, so a cliff cannot lift *
 *     distant terrain it never reserved.                                                       *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#include "hybridlandforms.h"

#include "worldboundsservice.h"
#include "wwdebug.h"
#include "wwmath.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>


static inline float Max2(float a,float b)	{ return (a > b) ? a : b; }
static inline float Min2(float a,float b)	{ return (a < b) ? a : b; }
static inline float Clamp2(float v,float lo,float hi)	{ return (v < lo) ? lo : ((v > hi) ? hi : v); }

//	The one smoothstep this phase uses; Section 8 Phase 5 and Section 10.3/10.4 all name it.
static inline float Smooth(float t)
{
	const float s = Clamp2(t,0.0f,1.0f);
	return s * s * (3.0f - (2.0f * s));
}


/*
**	Section 8 Phase 5's geometry table, Section 10.2's cliff record and Section 10.3/10.4's
**	profiles, each constant used exactly once below.  Multipliers of D are normalized map extent;
**	multipliers of W are lane widths; amplitude/depth multipliers are of tier_step.
*/
static const float	HILL_RADIUS_MIN_D			= 0.08f;
static const float	HILL_RADIUS_MAX_D			= 0.16f;
static const float	HILL_ASPECT_MIN			= 0.75f;
static const float	HILL_ASPECT_MAX			= 1.25f;
static const float	HILL_AMPLITUDE_MIN		= 0.75f;
static const float	HILL_AMPLITUDE_MAX		= 1.50f;

static const float	RIDGE_LENGTH_MIN_D		= 0.25f;
static const float	RIDGE_LENGTH_MAX_D		= 0.55f;
static const float	RIDGE_HALF_WIDTH_MIN_D	= 0.04f;
static const float	RIDGE_HALF_WIDTH_MAX_D	= 0.09f;
static const float	RIDGE_AMPLITUDE_MIN		= 1.00f;
static const float	RIDGE_AMPLITUDE_MAX		= 2.00f;
static const float	RIDGE_CORE_FRACTION		= 0.35f;		// steep core, for RoughnessClass only

//	Q-018 (D-036): a LARGE generic hill or ridge is drawn against (5/8)D, so the LARGE hill
//	radius is 0.05D..0.10D.  Archetype principal features keep D.
static const float	GENERIC_LANDFORM_LARGE_SCALE	= 0.625f;

static const float	CLIFF_LENGTH_MIN_W		= 8.0f;
static const float	CLIFF_LENGTH_MIN_D		= 0.12f;
static const float	CLIFF_LENGTH_MAX_D		= 0.45f;
static const float	CLIFF_HEIGHT_MIN			= 1.00f;
static const float	CLIFF_HEIGHT_MAX			= 2.00f;
static const float	CLIFF_EDGE_MIN_W			= 0.5f;
static const float	CLIFF_EDGE_MAX_W			= 1.5f;
static const float	CLIFF_TALUS_MIN_W			= 1.0f;
/*
**	Q-021 (D-016): `1W..5.25W`, widened from Section 10.2's original `1W..3W`.  A `PRIMARY_2` roadbed benched
**	along a D-004 smoothstep face needs `half_roadbed * 1.5 * H / F` of uphill cut, inside `max_cut_depth = 6`
**	only when `H / F <= 0.926`; with the broadest edge (`0.75W`) and this talus the shallowest native cliff
**	(`tier_step`) reaches `H / F = 18 / 6W = 0.903`, so a broad, shallow cliff *can* carry a ramp pass while
**	a tall or narrow one stays a wall.  Drawn through the ordinary keyed range like every other dimension:
**	nothing is redrawn or widened to make a road fit.
*/
static const float	CLIFF_TALUS_MAX_W			= 5.25f;

static const float	CANYON_LENGTH_MIN_D		= 0.35f;
static const float	CANYON_LENGTH_MAX_D		= 0.75f;
static const float	CANYON_FLOOR_MIN_W		= 1.5f;
static const float	CANYON_FLOOR_MAX_W		= 3.0f;
static const float	CANYON_RIM_MIN_W			= 4.0f;
static const float	CANYON_RIM_MAX_W			= 8.0f;
static const float	CANYON_DEPTH_MIN			= 1.00f;
static const float	CANYON_DEPTH_MAX			= 2.50f;

static const float	RAVINE_FLOOR_MIN_W		= 1.0f;
static const float	RAVINE_FLOOR_MAX_W		= 2.0f;
static const float	RAVINE_RIM_MIN_W			= 2.0f;
static const float	RAVINE_RIM_MAX_W			= 5.0f;
static const float	RAVINE_DEPTH_MIN			= 0.75f;
static const float	RAVINE_DEPTH_MAX			= 1.50f;

/*
**	The canyon-to-ravine family ratio the table itself uses on every shared row -- ravine floor
**	1.0..2.0W against canyon 1.5..3.0W -- applied to the one row the table gives only for a canyon.
*/
static const float	RAVINE_LENGTH_RATIO		= 2.0f / 3.0f;

static const float	LINE_CONTROL_INNER		= 1.0f / 6.0f;	// 0.1666667L
static const float	LINE_LATERAL_FRACTION	= 0.10f;			// interior lateral offsets, +/-0.10L
static const float	CANDIDATE_JITTER_S		= 0.25f;			// +/-0.25S keyed cell jitter
static const float	OVERLAP_MAX_FRACTION		= 0.25f;			// same-type overlap of the smaller polygon
static const float	PLATEAU_TOP_FRACTION		= 0.65f;			// flatten the upper 35% of the profile
static const float	PLATEAU_CORRIDOR_W		= 2.0f;			// centre 2W of a required heavy corridor
static const float	CORRIDOR_MIN_DRY_W		= 4.0f;			// a generic barrier may not go below this
static const float	SADDLE_MULTIPLIER			= 0.15f;			// Section 6.10 surface saddle
static const float	SADDLE_ZONE_W				= 4.0f;
static const float	REPAIR_LIMIT_D				= 0.10f;			// Section 6.1 repair budget
static const int		LINE_OUTLINE_STEP			= 1;				// outline every sampled centerline point
static const int		HILL_OUTLINE_SAMPLES		= 16;


/***********************************************************************************************
**	Names, kind predicates and the Section-5 qualification helpers.
***********************************************************************************************/

const char * Hybrid_Landform_Kind_Name(HybridLandformKind kind)
{
	switch (kind) {
		case HYBRID_LANDFORM_HILL:		return "hill";
		case HYBRID_LANDFORM_RIDGE:	return "ridge";
		case HYBRID_LANDFORM_CLIFF:	return "cliff";
		case HYBRID_LANDFORM_CANYON:	return "canyon";
		case HYBRID_LANDFORM_RAVINE:	return "ravine";
		default:								break;
	}
	return "unknown";
}


const char * Hybrid_Roughness_Class_Name(HybridRoughnessClass roughness)
{
	switch (roughness) {
		case HYBRID_ROUGHNESS_FLAT:		return "flat";
		case HYBRID_ROUGHNESS_ROLLING:	return "rolling";
		case HYBRID_ROUGHNESS_STEEP:		return "steep";
		case HYBRID_ROUGHNESS_BROKEN:		return "broken";
		default:									break;
	}
	return "unknown";
}


const char * Hybrid_Landform_Failure_Name(HybridLandformFailure failure)
{
	switch (failure) {
		case HYBRID_LANDFORM_OK:							return "ok";
		case HYBRID_LANDFORM_INVALID_INPUT:			return "invalid_input";
		case HYBRID_LANDFORM_SCRATCH_EXHAUSTED:		return "scratch_exhausted";
		case HYBRID_LANDFORM_NO_CANDIDATE_CELL:		return "no_candidate_cell";
		case HYBRID_LANDFORM_CANDIDATES_EXHAUSTED:	return "candidates_exhausted";
		case HYBRID_LANDFORM_REQUIRED_UNPLACEABLE:	return "required_unplaceable";
		case HYBRID_LANDFORM_ELEVATION_LIMIT:			return "elevation_limit";
		case HYBRID_LANDFORM_COUNT_MISMATCH:			return "count_mismatch";
		case HYBRID_LANDFORM_QUALIFICATION_FAILED:	return "qualification_failed";
		case HYBRID_LANDFORM_IMMUTABLE_IN_RESERVE:	return "immutable_in_reserve";
		default:													break;
	}
	return "unknown";
}


bool Hybrid_Landform_Is_Immutable(HybridLandformKind kind)
{
	return (kind == HYBRID_LANDFORM_CLIFF) || (kind == HYBRID_LANDFORM_CANYON) ||
			 (kind == HYBRID_LANDFORM_RAVINE);
}


bool Hybrid_Landform_Is_Line(HybridLandformKind kind)
{
	return (kind != HYBRID_LANDFORM_HILL);
}


float Hybrid_Cliff_Min_Length(float w,float d)
{
	return Max2(CLIFF_LENGTH_MIN_W * w,CLIFF_LENGTH_MIN_D * d);
}


float Hybrid_Ravine_Min_Length(float w,float d)
{
	return Max2(12.0f * w,0.20f * d);
}


float Hybrid_Ravine_Min_Depth(float tier_step)
{
	return 0.75f * tier_step;
}


static int Size_Pick(HybridSizeClass size_class,int small,int medium,int large)
{
	switch (size_class) {
		case HYBRID_SIZE_SMALL:		return small;
		case HYBRID_SIZE_MEDIUM:	return medium;
		case HYBRID_SIZE_LARGE:		return large;
		default:							break;
	}
	return medium;
}


float Hybrid_Generic_Landform_Dimension_D(float d,HybridSizeClass size_class)
{
	return (size_class == HYBRID_SIZE_LARGE) ? (d * GENERIC_LANDFORM_LARGE_SCALE) : d;
}


void Hybrid_Landform_Horizontal_Ranges(float d,HybridSizeClass size_class,HybridLandformRole role,
													HybridLandformHorizontalRanges * out)
{
	if (out == nullptr) return;
	const float extent = (role == HYBRID_LANDFORM_ROLE_GENERIC) ? Hybrid_Generic_Landform_Dimension_D(d,size_class) : d;
	out->HillRadiusMin		= HILL_RADIUS_MIN_D * extent;
	out->HillRadiusMax		= HILL_RADIUS_MAX_D * extent;
	out->RidgeLengthMin		= RIDGE_LENGTH_MIN_D * extent;
	out->RidgeLengthMax		= RIDGE_LENGTH_MAX_D * extent;
	out->RidgeHalfWidthMin	= RIDGE_HALF_WIDTH_MIN_D * extent;
	out->RidgeHalfWidthMax	= RIDGE_HALF_WIDTH_MAX_D * extent;
}


int Hybrid_Hill_Target_Count(HybridSizeClass size_class,float hill_density)
{
	const int base = Size_Pick(size_class,3,6,10);
	const float extra = (float)Size_Pick(size_class,2,4,6) * Max2(0.0f,hill_density);
	return base + (int)Hybrid_Round_Half_Up(extra);
}


int Hybrid_Ridge_Target_Count(HybridSizeClass size_class,float mountain_density)
{
	const int base = Size_Pick(size_class,1,2,3);
	const float extra = (float)Size_Pick(size_class,1,2,3) * Max2(0.0f,mountain_density);
	return base + (int)Hybrid_Round_Half_Up(extra);
}


int Hybrid_Cliff_Target_Count(HybridSizeClass size_class,float interior_cliff_density)
{
	const int quota = Size_Pick(size_class,1,2,4);						// Section 5 explicit cliff runs
	const float extra = (float)Size_Pick(size_class,1,2,4) * Max2(0.0f,interior_cliff_density);
	return quota + (int)Hybrid_Round_Half_Up(extra);
}


int Hybrid_Ravine_System_Target_Count(HybridSizeClass size_class,float valley_density)
{
	const int quota = Size_Pick(size_class,1,1,2);						// Section 5 ravine/canyon systems
	const float extra = (float)Size_Pick(size_class,0,1,2) * Max2(0.0f,valley_density);
	return quota + (int)Hybrid_Round_Half_Up(extra);
}


int Hybrid_Ravine_Branch_Count(HybridSizeClass size_class)
{
	return Size_Pick(size_class,2,3,5);
}


/***********************************************************************************************
**	HybridLandform / HybridLandformSet / HybridLandformParams.
***********************************************************************************************/

float HybridLandform::Influence_Half_Width(void) const
{
	switch (Kind) {
		case HYBRID_LANDFORM_HILL:		return Max2(RadiusX,RadiusY);
		case HYBRID_LANDFORM_RIDGE:	return InfluenceHalfWidth;
		case HYBRID_LANDFORM_CLIFF:	return (0.5f * EdgeWidth) + (2.0f * TalusWidth);
		case HYBRID_LANDFORM_CANYON:
		case HYBRID_LANDFORM_RAVINE:	return RimHalfWidth;
		default:								break;
	}
	return 0.0f;
}


void HybridLandformSet::Reset(void)
{
	Landforms.Delete_All();
	Hash.Reset();
	TargetHills		= 0;
	TargetRidges	= 0;
	TargetCliffs	= 0;
	TargetSystems	= 0;
	PlateauCount	= 0;
	SystemCount		= 0;
	Rejections		= 0;
	Moves				= 0;
	Checksum			= 0;
}


const HybridLandform * HybridLandformSet::Find(unsigned int stable_id) const
{
	//	StableIds are creation order, 1-based, so the index is exact; the loop is the safety net.
	const int index = (int)stable_id - 1;
	if ((index >= 0) && (index < Landforms.Count()) && (Landforms[index].StableId == stable_id)) {
		return &Landforms[index];
	}
	for (int i = 0; i < Landforms.Count(); ++i) {
		if (Landforms[i].StableId == stable_id) return &Landforms[i];
	}
	return nullptr;
}


int HybridLandformSet::Count_Of_Kind(HybridLandformKind kind) const
{
	int count = 0;
	for (int i = 0; i < Landforms.Count(); ++i) {
		if (Landforms[i].Kind == kind) count++;
	}
	return count;
}


int HybridLandformSet::Count_Of_Systems(void) const
{
	int count = 0;
	for (int i = 0; i < Landforms.Count(); ++i) {
		//	One count per system root: a Section-6.8 branch is not an independent system.
		if ((Landforms[i].SystemId != 0) && (Landforms[i].ParentStableId == 0)) count++;
	}
	return count;
}


HybridLandformParams::HybridLandformParams(void) :
	TerrainSeed(0),WorldMinX(0.0f),WorldMinY(0.0f),WorldWidth(0.0f),WorldHeight(0.0f),
	SeaLevel(0.0f),MinElevation(-30.0f),MaxElevation(160.0f),TierStep(18.0f),TierCount(3),
	MacroRelief(0.6f),HillDensity(0.45f),MountainDensity(0.18f),InteriorCliffDensity(0.16f),
	ValleyDensity(0.30f),PlateauChance(0.15f),
	MaxAlternateCandidatesPerFeature(6),MinVehicleLaneClearance(1.0f),CrossedCanyonSystemsRequired(0),
	Arena(nullptr),Counters(nullptr)
{
	::memset(&Constants,0,sizeof(Constants));
}


/***********************************************************************************************
**	Deterministic draws.  Every Phase-5 draw key is the plan's (terrain domain, type, StableId,
**	parameter name) tuple; the three-key hash helper takes it folded through the repository hash
**	stream, so no component of the tuple is lost and no mixing constant is invented.
***********************************************************************************************/

static unsigned int Landform_Key(HybridLandformKind kind,unsigned int stable_id,const char * name,
											unsigned int index)
{
	HybridHashStream stream;
	stream.Add_String(Hybrid_Landform_Kind_Name(kind));
	stream.Add_Uint(stable_id);
	stream.Add_String(name);
	stream.Add_Uint(index);
	return stream.Get();
}


static float Landform_Hash(unsigned int terrain_seed,HybridLandformKind kind,unsigned int stable_id,
									const char * name,unsigned int index = 0)
{
	return Hybrid_Hash01(terrain_seed,Landform_Key(kind,stable_id,name,index));
}


/***********************************************************************************************
**	Geometry helpers.
***********************************************************************************************/

/*
**	Signed lateral distance to a sampled centerline, the along-curve station of the closest point
**	and the overrun past either end.  Positive lateral is the curve's left side.
*/
static void Polyline_Measure(const HybridLandform & landform,const Vector2 & point,
									  float * out_lateral,float * out_station,float * out_overrun)
{
	if (out_lateral != nullptr) *out_lateral = 0.0f;
	if (out_station != nullptr) *out_station = 0.0f;
	if (out_overrun != nullptr) *out_overrun = 0.0f;

	const int count = landform.Points.Count();
	if (count < 2) return;

	float best_distance = 0.0f;
	float best_lateral = 0.0f;
	float best_station = 0.0f;
	float best_overrun = 0.0f;
	bool  have_best = false;

	for (int i = 0; i + 1 < count; ++i) {

		const Vector2 & a = landform.Points[i];
		const Vector2 & b = landform.Points[i + 1];
		const Vector2 delta = b - a;
		const float length = delta.Length();
		if (length <= 0.0f) continue;

		const Vector2 dir = delta * (1.0f / length);
		const Vector2 rel = point - a;
		const float along = (dir.X * rel.X) + (dir.Y * rel.Y);
		const float clamped = Clamp2(along,0.0f,length);
		const Vector2 closest = a + (dir * clamped);
		const float distance = (point - closest).Length();

		if (!have_best || (distance < best_distance)) {

			have_best		= true;
			best_distance	= distance;
			best_lateral	= (dir.X * rel.Y) - (dir.Y * rel.X);		// left-positive perpendicular
			best_station	= ((i < landform.Stations.Count()) ? landform.Stations[i] : 0.0f) + clamped;

			best_overrun = 0.0f;
			if ((i == 0) && (along < 0.0f))						best_overrun = -along;
			if ((i + 2 == count) && (along > length))			best_overrun = along - length;
		}
	}

	if (out_lateral != nullptr) *out_lateral = best_lateral;
	if (out_station != nullptr) *out_station = best_station;
	if (out_overrun != nullptr) *out_overrun = best_overrun;
}


//	Section 6.10's saddle multiplier, on the principal ridge only.
static float Saddle_Multiplier(const HybridLandform & landform,float station)
{
	if (!landform.SaddlePresent) return 1.0f;

	const float distance = WWMath::Fabs(station - landform.SaddleStation);
	if (distance <= landform.SaddleFlatHalfLength) return SADDLE_MULTIPLIER;

	const float ramp = landform.SaddleRampLength;
	if ((ramp <= 0.0f) || (distance >= (landform.SaddleFlatHalfLength + ramp))) return 1.0f;

	const float t = Smooth((distance - landform.SaddleFlatHalfLength) / ramp);
	return SADDLE_MULTIPLIER + ((1.0f - SADDLE_MULTIPLIER) * t);
}


float Hybrid_Landform_Contribution(const HybridLandform & landform,const Vector2 & world,float * profile_out)
{
	float profile = 0.0f;
	float value = 0.0f;

	switch (landform.Kind) {

		case HYBRID_LANDFORM_HILL:
		{
			//	Compact-support smooth radial ellipse: exactly zero at the influence radius.
			if ((landform.RadiusX <= 0.0f) || (landform.RadiusY <= 0.0f)) break;

			const Vector2 rel = world - landform.Center;
			const float c = WWMath::Cos(landform.Orientation);
			const float s = WWMath::Sin(landform.Orientation);
			const float lx = ((rel.X * c) + (rel.Y * s)) / landform.RadiusX;
			const float ly = ((-rel.X * s) + (rel.Y * c)) / landform.RadiusY;
			const float r = WWMath::Sqrt((lx * lx) + (ly * ly));
			if (r >= 1.0f) break;

			profile = 1.0f - Smooth(r);
			if (landform.IsPlateau) {
				profile = Smooth(Min2(1.0f,profile / PLATEAU_TOP_FRACTION));
			}
			value = landform.Amplitude * profile;
			break;
		}

		case HYBRID_LANDFORM_RIDGE:
		{
			if (landform.InfluenceHalfWidth <= 0.0f) break;

			float lateral = 0.0f;
			float station = 0.0f;
			float overrun = 0.0f;
			Polyline_Measure(landform,world,&lateral,&station,&overrun);

			const float radial = WWMath::Sqrt((lateral * lateral) + (overrun * overrun));
			if (radial >= landform.InfluenceHalfWidth) break;

			profile = 1.0f - Smooth(radial / landform.InfluenceHalfWidth);
			if (landform.IsPlateau) {
				profile = Smooth(Min2(1.0f,profile / PLATEAU_TOP_FRACTION));
			}
			value = landform.Amplitude * profile * Saddle_Multiplier(landform,station);
			break;
		}

		case HYBRID_LANDFORM_CLIFF:
		{
			/*
			**	Section 10.2 gives a cliff an edge width and a rock/talus transition width and no
			**	other length scale, so the field uses only those: it rises from zero at the talus toe
			**	to full height at the upper edge, holds full height across one talus width of shelf
			**	and returns to zero across one more.  Compact support, exactly as Section 10.4 needs.
			*/
			const float e = 0.5f * landform.EdgeWidth;
			const float t = landform.TalusWidth;
			if ((e <= 0.0f) || (t <= 0.0f)) break;

			float lateral = 0.0f;
			float station = 0.0f;
			float overrun = 0.0f;
			Polyline_Measure(landform,world,&lateral,&station,&overrun);

			const float side = lateral * (float)landform.UpperSideSign;
			if ((side <= -(e + t)) || (side >= (e + (2.0f * t)))) break;

			if (side < e) {
				profile = Smooth((side + e + t) / ((2.0f * e) + t));
			} else if (side <= (e + t)) {
				profile = 1.0f;
			} else {
				profile = 1.0f - Smooth((side - (e + t)) / t);
			}

			//	Longitudinal taper, so a finite run does not step the whole map edge.
			const float cap = e + (2.0f * t);
			if (overrun > 0.0f) {
				if (overrun >= cap) { profile = 0.0f; break; }
				profile *= (1.0f - Smooth(overrun / cap));
			}

			value = landform.HeightDifference * profile;
			break;
		}

		case HYBRID_LANDFORM_CANYON:
		case HYBRID_LANDFORM_RAVINE:
		{
			//	Section 10.3: flat floor, smoothstep wall, exactly zero outside the rim.
			if (landform.RimHalfWidth <= landform.FloorHalfWidth) break;

			float lateral = 0.0f;
			float station = 0.0f;
			float overrun = 0.0f;
			Polyline_Measure(landform,world,&lateral,&station,&overrun);

			const float distance = WWMath::Fabs(lateral);
			if (distance >= landform.RimHalfWidth) break;

			if (distance <= landform.FloorHalfWidth) {
				profile = 1.0f;
			} else {
				profile = 1.0f - Smooth((distance - landform.FloorHalfWidth) /
												(landform.RimHalfWidth - landform.FloorHalfWidth));
			}

			if (overrun > 0.0f) {
				if (overrun >= landform.RimHalfWidth) { profile = 0.0f; break; }
				profile *= (1.0f - Smooth(overrun / landform.RimHalfWidth));
			}

			value = -(landform.Depth * profile);
			break;
		}

		default:
			break;
	}

	if (profile_out != nullptr) *profile_out = profile;
	return value;
}


//	Section 10.3's branch join: the minimum (deepest) contribution plus one `A`-wide smooth union.
static float Smooth_Min(float a,float b,float k)
{
	if (k <= 0.0f) return Min2(a,b);
	const float h = Max2(0.0f,1.0f - (WWMath::Fabs(a - b) / k));
	return Min2(a,b) - (0.25f * h * h * k);
}


float Hybrid_Landform_Height(const HybridLandformSet & set,const Vector2 & world,float grid_spacing)
{
	float total = 0.0f;

	//	Per-system canyon/ravine accumulation; a map never has more systems than this.
	const int MAX_SYSTEMS = 32;
	unsigned int system_id[MAX_SYSTEMS];
	float system_value[MAX_SYSTEMS];
	int system_count = 0;

	for (int i = 0; i < set.Landforms.Count(); ++i) {

		const HybridLandform & landform = set.Landforms[i];
		if ((world.X < landform.BoundsMin.X) || (world.X > landform.BoundsMax.X) ||
			 (world.Y < landform.BoundsMin.Y) || (world.Y > landform.BoundsMax.Y)) continue;

		const float value = Hybrid_Landform_Contribution(landform,world,nullptr);
		if (value == 0.0f) continue;

		if (landform.SystemId == 0) {
			total += value;
			continue;
		}

		int slot = -1;
		for (int s = 0; s < system_count; ++s) {
			if (system_id[s] == landform.SystemId) { slot = s; break; }
		}
		if (slot < 0) {
			if (system_count >= MAX_SYSTEMS) { total += value; continue; }
			slot = system_count++;
			system_id[slot]		= landform.SystemId;
			system_value[slot]	= value;
			continue;
		}
		system_value[slot] = Smooth_Min(system_value[slot],value,grid_spacing);
	}

	for (int s = 0; s < system_count; ++s) {
		total += system_value[s];
	}

	return total;
}


/***********************************************************************************************
**	The builder.
***********************************************************************************************/

/*
**	One candidate cell with the key the current feature sorts it by:
**	`Hash01(terrain,type,StableId,"cell",row_major_index)`, ties broken by the lower cell index.
*/
struct HybridCandidateEntry
{
	float	Key;
	int	Cell;
};


static int Candidate_Compare(const void * a,const void * b)
{
	const HybridCandidateEntry * left = (const HybridCandidateEntry *)a;
	const HybridCandidateEntry * right = (const HybridCandidateEntry *)b;
	if (left->Key < right->Key) return -1;
	if (left->Key > right->Key) return 1;
	if (left->Cell < right->Cell) return -1;
	if (left->Cell > right->Cell) return 1;
	return 0;
}


struct LandformBuilder
{
	const HybridLandformParams *		P;
	const HybridMapIdentity *			Identity;
	const HybridArchetypeIntentSet *	Intents;
	const HybridStrategicGraph *		Graph;
	HybridCoarseMatrices *				Matrices;
	HybridLandformSet *					Out;

	int *										Candidates;			// row-major coarse-cell indices, ascending
	int										CandidateCount;
	DynamicVectorClass<int>				CandidateHeap;		// owner when there is no attempt arena

	//	The current feature's keyed candidate order, sorted once per feature.
	HybridCandidateEntry *				Order;
	DynamicVectorClass<int>				OrderHeap;			// owner when there is no attempt arena

	//	The placeable interior, measured from Phase 4's own PLACEABLE layer.
	float										PlaceableMinX;
	float										PlaceableMinY;
	float										PlaceableMaxX;
	float										PlaceableMaxY;
	float										PlaceableSpan;		// min(width,height) of that box

	unsigned int							NextStableId;
	unsigned int							NextSystemId;

	//	Why the last candidate was refused, so an exhausted feature says which rule did it.
	const char *							LastReject;
};


static bool Fail(HybridLandformResult * result,HybridLandformFailure failure,const char * detail)
{
	if (result != nullptr) {
		result->Failure	= failure;
		result->Detail		= (detail != nullptr) ? detail : "";
	}
	return false;
}


static void Note_Cells(const LandformBuilder & b,int cells)
{
	if ((b.P->Counters != nullptr) && (cells > 0)) {
		b.P->Counters->Add(HYBRID_WORK_SEMANTIC_CELLS,cells);
	}
}


static void Note_Candidate(const LandformBuilder & b)
{
	if (b.P->Counters != nullptr) {
		b.P->Counters->Add(HYBRID_WORK_REFINEMENT_SAMPLES,1);
	}
}


/*
**	Step 1 of the candidate process: every coarse cell that is placeable and whose centre is
**	outside the base and Tiberium hard reserves, in row-major order.
*/
static bool Build_Candidate_Cells(LandformBuilder & b,HybridLandformResult * result)
{
	const HybridSemanticMatrix & matrix = b.Matrices->Matrix;
	const int cells = matrix.Get_Cell_Count();

	int count = 0;
	for (int cy = 0; cy < matrix.Get_Count_Y(); ++cy) {
		for (int cx = 0; cx < matrix.Get_Count_X(); ++cx) {
			if (!matrix.Has_Flag(cx,cy,HYBRID_FLAG_PLACEABLE)) continue;
			if (b.Matrices->Is_Base_Protected(cx,cy)) continue;
			if (matrix.Has_Flag(cx,cy,HYBRID_FLAG_TIBERIUM_RESERVE)) continue;
			count++;
		}
	}
	Note_Cells(b,cells);

	if (count <= 0) {
		return Fail(result,HYBRID_LANDFORM_NO_CANDIDATE_CELL,
						"no placeable coarse cell lies outside the base/Tiberium reserves");
	}

	if (b.P->Arena != nullptr) {
		b.Candidates = (int *)b.P->Arena->Allocate((int)(count * sizeof(int)),16);
		if (b.Candidates == nullptr) {
			return Fail(result,HYBRID_LANDFORM_SCRATCH_EXHAUSTED,"candidate cell array does not fit the arena");
		}
	} else {
		for (int i = 0; i < count; ++i) b.CandidateHeap.Add(0);
		b.Candidates = &b.CandidateHeap[0];
	}

	int next = 0;
	bool have_box = false;
	for (int cy = 0; cy < matrix.Get_Count_Y(); ++cy) {
		for (int cx = 0; cx < matrix.Get_Count_X(); ++cx) {

			if (!matrix.Has_Flag(cx,cy,HYBRID_FLAG_PLACEABLE)) continue;

			/*
			**	The placeable interior, which is what a landform's own extent is clamped to.  Phase 4
			**	already folded the boundary margin and the perimeter cliff band into this layer, so
			**	measuring it here keeps one owner for "where may anything stand".
			*/
			const Vector2 centre = matrix.Cell_Center(cx,cy);
			const float half_cell = 0.5f * matrix.Get_Cell_Size();
			if (!have_box) {
				have_box				= true;
				b.PlaceableMinX	= centre.X - half_cell;
				b.PlaceableMinY	= centre.Y - half_cell;
				b.PlaceableMaxX	= centre.X + half_cell;
				b.PlaceableMaxY	= centre.Y + half_cell;
			}
			b.PlaceableMinX	= Min2(b.PlaceableMinX,centre.X - half_cell);
			b.PlaceableMinY	= Min2(b.PlaceableMinY,centre.Y - half_cell);
			b.PlaceableMaxX	= Max2(b.PlaceableMaxX,centre.X + half_cell);
			b.PlaceableMaxY	= Max2(b.PlaceableMaxY,centre.Y + half_cell);

			if (b.Matrices->Is_Base_Protected(cx,cy)) continue;
			if (matrix.Has_Flag(cx,cy,HYBRID_FLAG_TIBERIUM_RESERVE)) continue;
			b.Candidates[next++] = matrix.Index(cx,cy);
		}
	}
	b.PlaceableSpan = have_box ? Min2(b.PlaceableMaxX - b.PlaceableMinX,b.PlaceableMaxY - b.PlaceableMinY) : 0.0f;
	b.CandidateCount = next;

	/*
	**	One reusable keyed-order buffer, sorted per feature.  Sorting beats rescanning the whole list
	**	for each alternate: the order is the plan's own key order either way.
	*/
	const int order_bytes = (int)(b.CandidateCount * sizeof(HybridCandidateEntry));
	if (b.P->Arena != nullptr) {
		b.Order = (HybridCandidateEntry *)b.P->Arena->Allocate(order_bytes,16);
		if (b.Order == nullptr) {
			return Fail(result,HYBRID_LANDFORM_SCRATCH_EXHAUSTED,"candidate order array does not fit the arena");
		}
	} else {
		const int slots = (order_bytes / (int)sizeof(int)) + 1;
		for (int i = 0; i < slots; ++i) b.OrderHeap.Add(0);
		b.Order = (HybridCandidateEntry *)&b.OrderHeap[0];
	}

	if (b.P->Counters != nullptr) {
		b.P->Counters->Note_Peak(HYBRID_WORK_PEAK_SCRATCH_BYTES,
										 (b.P->Arena != nullptr) ? b.P->Arena->Get_High_Water() : 0);
	}
	return true;
}


/*
**	Step 2: the candidate cells in ascending
**	`(Hash01(terrain,type,StableId,"cell",row_major_index), row_major_index)` order.
*/
static void Prepare_Candidate_Order(LandformBuilder & b,HybridLandformKind kind,unsigned int stable_id)
{
	for (int i = 0; i < b.CandidateCount; ++i) {
		b.Order[i].Cell	= b.Candidates[i];
		b.Order[i].Key		= Landform_Hash(b.P->TerrainSeed,kind,stable_id,"cell",(unsigned int)b.Candidates[i]);
	}
	::qsort(b.Order,(size_t)b.CandidateCount,sizeof(HybridCandidateEntry),Candidate_Compare);
}


/*
**	Step 4: every size/amplitude/angle parameter, drawn exactly once.  Retries move the feature and
**	never come back here.  The role is the placement path's (Q-018, D-036): it chooses the extent a
**	hill's or ridge's horizontal size is drawn against.
*/
static void Draw_Dimensions(const LandformBuilder & b,HybridLandform * landform,HybridLandformRole role)
{
	const HybridLandformParams & p = *b.P;
	const float w = p.Constants.W;
	const float d = p.Constants.D;
	HybridLandformHorizontalRanges horizontal;
	Hybrid_Landform_Horizontal_Ranges(d,b.Identity->SizeClass,role,&horizontal);
	const float relief = 0.50f + (0.50f * Clamp2(p.MacroRelief,0.0f,1.0f));
	const unsigned int seed = p.TerrainSeed;
	const HybridLandformKind kind = landform->Kind;
	const unsigned int id = landform->StableId;

	switch (kind) {

		case HYBRID_LANDFORM_HILL:
		{
			const float radius = Hybrid_Range(horizontal.HillRadiusMin,horizontal.HillRadiusMax,
														 Landform_Hash(seed,kind,id,"radius"));
			const float aspect = Hybrid_Range(HILL_ASPECT_MIN,HILL_ASPECT_MAX,
														 Landform_Hash(seed,kind,id,"aspect"),HYBRID_QUANTUM_NORMALIZED);
			landform->RadiusX		= radius;
			landform->RadiusY		= Hybrid_Quantize_Position(radius * aspect);
			landform->Orientation= Hybrid_Range(0.0f,WWMATH_PI,Landform_Hash(seed,kind,id,"orientation"),
															HYBRID_QUANTUM_ANGLE);
			landform->Amplitude	= Hybrid_Quantize_Position(
											p.TierStep * Hybrid_Range(HILL_AMPLITUDE_MIN,HILL_AMPLITUDE_MAX,
																				Landform_Hash(seed,kind,id,"amplitude"),
																				HYBRID_QUANTUM_NORMALIZED) * relief);
			break;
		}

		case HYBRID_LANDFORM_RIDGE:
		{
			landform->Length				= Hybrid_Range(horizontal.RidgeLengthMin,horizontal.RidgeLengthMax,
																	Landform_Hash(seed,kind,id,"length"));
			landform->InfluenceHalfWidth	= Hybrid_Range(horizontal.RidgeHalfWidthMin,horizontal.RidgeHalfWidthMax,
																		Landform_Hash(seed,kind,id,"half_width"));
			landform->Amplitude			= Hybrid_Quantize_Position(
													p.TierStep * Hybrid_Range(RIDGE_AMPLITUDE_MIN,RIDGE_AMPLITUDE_MAX,
																						Landform_Hash(seed,kind,id,"amplitude"),
																						HYBRID_QUANTUM_NORMALIZED) * relief);
			break;
		}

		case HYBRID_LANDFORM_CLIFF:
		{
			landform->Length				= Hybrid_Range(Hybrid_Cliff_Min_Length(w,d),CLIFF_LENGTH_MAX_D * d,
																	Landform_Hash(seed,kind,id,"length"));
			landform->HeightDifference	= Hybrid_Quantize_Position(
													p.TierStep * Hybrid_Range(CLIFF_HEIGHT_MIN,CLIFF_HEIGHT_MAX,
																						Landform_Hash(seed,kind,id,"height"),
																						HYBRID_QUANTUM_NORMALIZED));
			landform->EdgeWidth			= Hybrid_Range(CLIFF_EDGE_MIN_W * w,CLIFF_EDGE_MAX_W * w,
																	Landform_Hash(seed,kind,id,"edge_width"));
			landform->TalusWidth			= Hybrid_Range(CLIFF_TALUS_MIN_W * w,CLIFF_TALUS_MAX_W * w,
																	Landform_Hash(seed,kind,id,"talus_width"));
			landform->UpperSideSign		= Hybrid_Pick_Sign(Hybrid_Hash01(seed,Hybrid_Key("cliff_upper_side"),id));
			break;
		}

		case HYBRID_LANDFORM_CANYON:
		{
			landform->Length			= Hybrid_Range(CANYON_LENGTH_MIN_D * d,CANYON_LENGTH_MAX_D * d,
															 Landform_Hash(seed,kind,id,"length"));
			landform->FloorHalfWidth= Hybrid_Range(CANYON_FLOOR_MIN_W * w,CANYON_FLOOR_MAX_W * w,
																Landform_Hash(seed,kind,id,"floor_half_width"));
			landform->RimHalfWidth	= Hybrid_Quantize_Position(landform->FloorHalfWidth +
											Hybrid_Range(CANYON_RIM_MIN_W * w,CANYON_RIM_MAX_W * w,
															 Landform_Hash(seed,kind,id,"rim_half_width")));
			landform->Depth			= Hybrid_Quantize_Position(
											p.TierStep * Hybrid_Range(CANYON_DEPTH_MIN,CANYON_DEPTH_MAX,
																				Landform_Hash(seed,kind,id,"depth"),
																				HYBRID_QUANTUM_NORMALIZED));
			break;
		}

		case HYBRID_LANDFORM_RAVINE:
		{
			/*
			**	The table gives no ravine centerline length.  Use the canyon row scaled by the same
			**	canyon-to-ravine family ratio every shared row already uses, and scale it once more
			**	for a Section-6.8 branch, whose parent carries the system's Section-5 qualification.
			*/
			float low		= CANYON_LENGTH_MIN_D * d * RAVINE_LENGTH_RATIO;
			float high		= CANYON_LENGTH_MAX_D * d * RAVINE_LENGTH_RATIO;
			if (landform->ParentStableId != 0) {
				low	*= RAVINE_LENGTH_RATIO;
				high	*= RAVINE_LENGTH_RATIO;
			}
			landform->Length			= Hybrid_Range(low,high,Landform_Hash(seed,kind,id,"length"));
			landform->FloorHalfWidth= Hybrid_Range(RAVINE_FLOOR_MIN_W * w,RAVINE_FLOOR_MAX_W * w,
																Landform_Hash(seed,kind,id,"floor_half_width"));
			landform->RimHalfWidth	= Hybrid_Quantize_Position(landform->FloorHalfWidth +
											Hybrid_Range(RAVINE_RIM_MIN_W * w,RAVINE_RIM_MAX_W * w,
															 Landform_Hash(seed,kind,id,"rim_half_width")));
			landform->Depth			= Hybrid_Quantize_Position(
											p.TierStep * Hybrid_Range(RAVINE_DEPTH_MIN,RAVINE_DEPTH_MAX,
																				Landform_Hash(seed,kind,id,"depth"),
																				HYBRID_QUANTUM_NORMALIZED));
			break;
		}

		default:
			break;
	}
}


/*
**	Section 8 Phase 5 clamps every drawn value so the feature still fits its envelope -- it says so
**	for the vertical ranges ("all values are clamped so final provisional heights stay inside
**	terrain.min_elevation..max_elevation").  The horizontal ranges need the same treatment, because
**	`0.75D` of canyon plus its rim does not fit inside a SMALL world once `boundary_margin_cells`
**	has been taken off both sides, and a dimension is drawn once and never redrawn.  Clamping keeps
**	the feature and its Section-5 qualification; refusing to clamp would fail otherwise-good seeds
**	on arithmetic alone.  The clamp is deterministic and monotone: it can only shrink a draw, never
**	move it outside its range, and it never takes a length below the Section-5 minimum -- when even
**	that does not fit, the caller reports the attempt failure.
*/
static bool Clamp_To_Placeable_Span(const LandformBuilder & b,HybridLandform * landform)
{
	if (b.PlaceableSpan <= 0.0f) return false;

	if (!Hybrid_Landform_Is_Line(landform->Kind)) {

		const float limit = 0.5f * b.PlaceableSpan;
		const float largest = Max2(landform->RadiusX,landform->RadiusY);
		if (largest > limit) {
			const float scale = limit / largest;
			landform->RadiusX = Hybrid_Quantize_Position(landform->RadiusX * scale);
			landform->RadiusY = Hybrid_Quantize_Position(landform->RadiusY * scale);
		}
		return (landform->RadiusX > 0.0f) && (landform->RadiusY > 0.0f);
	}

	const float half_width = landform->Influence_Half_Width();
	const float limit = b.PlaceableSpan - (2.0f * half_width);
	if (limit <= 0.0f) return false;

	if (landform->Length > limit) {
		landform->Length = Hybrid_Quantize_Position(limit);
	}

	//	Section 5's own minimum for the kinds that have one.
	const float w = b.P->Constants.W;
	const float d = b.P->Constants.D;
	if (landform->Kind == HYBRID_LANDFORM_CLIFF) {
		return (landform->Length >= Hybrid_Cliff_Min_Length(w,d));
	}
	if (((landform->Kind == HYBRID_LANDFORM_CANYON) || (landform->Kind == HYBRID_LANDFORM_RAVINE))
		&& (landform->ParentStableId == 0)) {
		return (landform->Length >= Hybrid_Ravine_Min_Length(w,d));
	}
	return (landform->Length > 0.0f);
}


//	Recompute the AABB of a feature's full influence polygon.
static void Update_Bounds(HybridLandform * landform)
{
	const float half = landform->Influence_Half_Width();

	if (!Hybrid_Landform_Is_Line(landform->Kind) || (landform->Points.Count() == 0)) {
		landform->BoundsMin.Set(landform->Center.X - half,landform->Center.Y - half);
		landform->BoundsMax.Set(landform->Center.X + half,landform->Center.Y + half);
		return;
	}

	Vector2 low = landform->Points[0];
	Vector2 high = landform->Points[0];
	for (int i = 1; i < landform->Points.Count(); ++i) {
		low.X		= Min2(low.X,landform->Points[i].X);
		low.Y		= Min2(low.Y,landform->Points[i].Y);
		high.X	= Max2(high.X,landform->Points[i].X);
		high.Y	= Max2(high.Y,landform->Points[i].Y);
	}
	landform->BoundsMin.Set(low.X - half,low.Y - half);
	landform->BoundsMax.Set(high.X + half,high.Y + half);
}


/*
**	Geometry construction, fixed by the plan.  A hill is its ellipse; a line feature is the four
**	local controls at -0.50L, -0.1666667L, +0.1666667L, +0.50L with keyed interior lateral offsets,
**	rotated to `angle`, translated to `centre` and sampled at `A/2` by the shared centripetal
**	Catmull-Rom sampler.
*/
static bool Build_Geometry(const LandformBuilder & b,HybridLandform * landform,const Vector2 & centre,
									float angle)
{
	landform->Center.Set(Hybrid_Quantize_Position(centre.X),Hybrid_Quantize_Position(centre.Y));
	landform->Points.Delete_All();
	landform->Stations.Delete_All();

	if (!Hybrid_Landform_Is_Line(landform->Kind)) {
		Update_Bounds(landform);
		return true;
	}

	const float length = landform->Length;
	if (length <= 0.0f) return false;

	const float lateral1 = Hybrid_Range(-LINE_LATERAL_FRACTION * length,LINE_LATERAL_FRACTION * length,
													Landform_Hash(b.P->TerrainSeed,landform->Kind,landform->StableId,"lateral_1"));
	const float lateral2 = Hybrid_Range(-LINE_LATERAL_FRACTION * length,LINE_LATERAL_FRACTION * length,
													Landform_Hash(b.P->TerrainSeed,landform->Kind,landform->StableId,"lateral_2"));

	const float local_x[4] = { -0.50f * length, -LINE_CONTROL_INNER * length,
										+LINE_CONTROL_INNER * length, +0.50f * length };
	const float local_y[4] = { 0.0f, lateral1, lateral2, 0.0f };

	/*
	**	The stored `Orientation` is the authoritative axis of this feature, so it is also the basis the
	**	controls are laid in.  Rotating them by the raw angle and *then* quantizing the orientation leaves
	**	two descriptions of the same frame that differ by up to the 0.001 rad angle quantum -- harmless at
	**	the centre and not at all harmless at the ends, where the lever arm is half the feature's length:
	**	measured on a 116-unit ravine, end controls sat 0.029 off an axis the record says they are on, three
	**	times the 0.01 the plan allows.  One frame, written down once, and every control laid in it.
	*/
	landform->Orientation = Hybrid_Quantize_Angle(angle);
	const float c = WWMath::Cos(landform->Orientation);
	const float s = WWMath::Sin(landform->Orientation);

	for (int i = 0; i < 4; ++i) {
		landform->Controls[i].Set(
			Hybrid_Quantize_Position(landform->Center.X + (local_x[i] * c) - (local_y[i] * s)),
			Hybrid_Quantize_Position(landform->Center.Y + (local_x[i] * s) + (local_y[i] * c)));
	}

	if (!Hybrid_Sample_Centripetal_Catmull_Rom(landform->Controls,4,0.5f * b.P->Constants.A,
															  &landform->Points,&landform->Stations)) {
		return false;
	}

	Update_Bounds(landform);
	return true;
}


//	Is this point inside the feature's full influence polygon?
static bool Inside_Influence(const HybridLandform & landform,const Vector2 & point)
{
	float profile = 0.0f;
	Hybrid_Landform_Contribution(landform,point,&profile);
	return (profile > 0.0f);
}


/*
**	The outline of the full influence/protected polygon, as the sample points rule 5 tests against
**	the placeable boundary.
*/
static void Influence_Outline(const HybridLandform & landform,DynamicVectorClass<Vector2> * out)
{
	out->Delete_All();
	const float half = landform.Influence_Half_Width();

	if (!Hybrid_Landform_Is_Line(landform.Kind)) {
		for (int i = 0; i < HILL_OUTLINE_SAMPLES; ++i) {
			const float a = (2.0f * WWMATH_PI * (float)i) / (float)HILL_OUTLINE_SAMPLES;
			const float lx = landform.RadiusX * WWMath::Cos(a);
			const float ly = landform.RadiusY * WWMath::Sin(a);
			const float c = WWMath::Cos(landform.Orientation);
			const float s = WWMath::Sin(landform.Orientation);
			out->Add(Vector2(landform.Center.X + (lx * c) - (ly * s),
								  landform.Center.Y + (lx * s) + (ly * c)));
		}
		return;
	}

	const int count = landform.Points.Count();
	if (count < 2) return;

	for (int i = 0; i < count; i += LINE_OUTLINE_STEP) {

		const int next = (i + 1 < count) ? (i + 1) : i;
		const int prev = (i > 0) ? (i - 1) : i;
		Vector2 delta = landform.Points[next] - landform.Points[prev];
		const float length = delta.Length();
		if (length <= 0.0f) continue;
		delta *= (1.0f / length);
		const Vector2 normal(-delta.Y,delta.X);

		out->Add(landform.Points[i] + (normal * half));
		out->Add(landform.Points[i] - (normal * half));
	}

	//	The two end caps, so a short run is enclosed as well.
	out->Add(landform.Points[0]);
	out->Add(landform.Points[count - 1]);
}


/*
**	The points every reservation, boundary and elevation test is taken at: the influence outline plus
**	the feature's own spine.  Testing the geometry rather than only the coarse cell centres matters
**	for a thin feature -- a cliff band or a ravine floor is narrower than one coarse cell, so a cell
**	centre can miss it entirely while the wall itself stands squarely in a reserve.
*/
static void Landform_Sample_Points(const HybridLandform & landform,DynamicVectorClass<Vector2> * out)
{
	Influence_Outline(landform,out);

	if (Hybrid_Landform_Is_Line(landform.Kind)) {
		for (int i = 0; i < landform.Points.Count(); ++i) {
			out->Add(landform.Points[i]);
		}
		return;
	}

	out->Add(landform.Center);
	for (int i = 0; i < HILL_OUTLINE_SAMPLES; ++i) {
		const float a = (2.0f * WWMATH_PI * (float)i) / (float)HILL_OUTLINE_SAMPLES;
		const float lx = 0.5f * landform.RadiusX * WWMath::Cos(a);
		const float ly = 0.5f * landform.RadiusY * WWMath::Sin(a);
		const float c = WWMath::Cos(landform.Orientation);
		const float sn = WWMath::Sin(landform.Orientation);
		out->Add(Vector2(landform.Center.X + (lx * c) - (ly * sn),
							  landform.Center.Y + (lx * sn) + (ly * c)));
	}
}


/*
**	Section 8 Phase 5's elevation clamp: every amplitude/depth is clamped so the provisional height
**	stays inside `terrain.min_elevation..terrain.max_elevation` relative to sea level.  False means
**	the clamp collapses the feature, which is a rejected candidate rather than a resized one.
*/
static bool Clamp_To_Elevation_Envelope(const LandformBuilder & b,HybridLandform * landform)
{
	const HybridSemanticMatrix & matrix = b.Matrices->Matrix;
	const HybridLandformParams & p = *b.P;
	const signed char middle = (signed char)((p.TierCount - 1) / 2);

	DynamicVectorClass<Vector2> samples;
	Landform_Sample_Points(*landform,&samples);

	float highest = 0.0f;
	float lowest = 0.0f;
	bool  found = false;

	for (int i = 0; i < samples.Count(); ++i) {
		int cx = 0, cy = 0;
		if (!matrix.World_To_Cell(samples[i],&cx,&cy)) continue;
		const int index = matrix.Index(cx,cy);
		const float base = p.SeaLevel + ((float)(matrix.BaseElevationTier[index] - middle) * p.TierStep)
								 + matrix.MacroHeightBias[index];
		if (!found) { highest = base; lowest = base; found = true; }
		highest = Max2(highest,base);
		lowest = Min2(lowest,base);
	}

	if (!found) return false;				// the feature lies entirely outside the matrix

	const float ceiling = p.SeaLevel + p.MaxElevation;
	const float floor_limit = p.SeaLevel + p.MinElevation;

	switch (landform->Kind) {

		case HYBRID_LANDFORM_HILL:
		case HYBRID_LANDFORM_RIDGE:
		{
			const float headroom = ceiling - highest;
			if (headroom <= 0.0f) return false;
			if (landform->Amplitude > headroom) landform->Amplitude = Hybrid_Quantize_Position(headroom);
			return (landform->Amplitude > 0.0f);
		}

		case HYBRID_LANDFORM_CLIFF:
		{
			const float headroom = ceiling - highest;
			if (headroom <= 0.0f) return false;
			if (landform->HeightDifference > headroom) {
				landform->HeightDifference = Hybrid_Quantize_Position(headroom);
			}
			//	A cliff that no longer stands a full tier above its toe is not an explicit cliff run.
			return (landform->HeightDifference >= (0.5f * p.TierStep));
		}

		case HYBRID_LANDFORM_CANYON:
		case HYBRID_LANDFORM_RAVINE:
		{
			const float room = lowest - floor_limit;
			if (room <= 0.0f) return false;
			if (landform->Depth > room) landform->Depth = Hybrid_Quantize_Position(room);
			//	Section 5: a counted canyon/ravine is at least 0.75 * tier_step deep.
			return (landform->Depth >= Hybrid_Ravine_Min_Depth(p.TierStep));
		}

		default:
			break;
	}
	return false;
}


//	Deterministic sampled area of one influence polygon, at spacing A/2 on a world-aligned grid.
static float Polygon_Area(const LandformBuilder & b,const HybridLandform & landform)
{
	const float step = 0.5f * b.P->Constants.A;
	const float cell = step * step;
	float area = 0.0f;

	for (float y = landform.BoundsMin.Y; y <= landform.BoundsMax.Y; y += step) {
		for (float x = landform.BoundsMin.X; x <= landform.BoundsMax.X; x += step) {
			if (Inside_Influence(landform,Vector2(x,y))) area += cell;
		}
	}
	return area;
}


static float Polygon_Overlap_Area(const LandformBuilder & b,const HybridLandform & a,
											 const HybridLandform & c)
{
	const float step = 0.5f * b.P->Constants.A;
	const float cell = step * step;

	const float min_x = Max2(a.BoundsMin.X,c.BoundsMin.X);
	const float min_y = Max2(a.BoundsMin.Y,c.BoundsMin.Y);
	const float max_x = Min2(a.BoundsMax.X,c.BoundsMax.X);
	const float max_y = Min2(a.BoundsMax.Y,c.BoundsMax.Y);
	if ((min_x > max_x) || (min_y > max_y)) return 0.0f;

	float area = 0.0f;
	for (float y = min_y; y <= max_y; y += step) {
		for (float x = min_x; x <= max_x; x += step) {
			const Vector2 point(x,y);
			if (Inside_Influence(a,point) && Inside_Influence(c,point)) area += cell;
		}
	}
	return area;
}


/*
**	Rejection rule 5's heavy-corridor clause: a generic immutable barrier may not reduce a required
**	heavy corridor's dry/passable width below `4W`.  Archetype-required features are exempt -- they
**	exist to obstruct, and Phase 3 already recorded the crossing obligation Phase 6 must satisfy
**	with a window.
*/
static bool Corridor_Width_Survives(const LandformBuilder & b,const HybridLandform & landform)
{
	const float w = b.P->Constants.W;
	const float step = Max2(0.5f * w,b.P->Constants.Q);
	const float required = CORRIDOR_MIN_DRY_W * w;

	for (int r = 0; r < b.Matrices->Reservations.Count(); ++r) {

		const HybridReservation & reservation = b.Matrices->Reservations[r];
		if (reservation.Priority != HYBRID_RESERVE_HEAVY_CORRIDOR) continue;

		Vector2 axis = reservation.B - reservation.A;
		const float axis_length = axis.Length();
		if (axis_length <= 0.0f) continue;
		axis *= (1.0f / axis_length);
		const Vector2 normal(-axis.Y,axis.X);

		for (float along = 0.0f; along <= axis_length; along += b.P->Constants.A) {

			const Vector2 centre = reservation.A + (axis * along);

			//	Longest contiguous run of unobstructed cross-section, measured across the corridor.
			float best = 0.0f;
			float run = 0.0f;
			for (float lateral = -reservation.Radius; lateral <= reservation.Radius; lateral += step) {
				const Vector2 point = centre + (normal * lateral);
				if (Inside_Influence(landform,point)) {
					run = 0.0f;
				} else {
					run += step;
					best = Max2(best,run);
				}
			}
			if (best < required) return false;
		}
	}
	return true;
}


/*
**	Why a candidate was refused.  The two answers are not interchangeable: a cell where the feature
**	simply does not fit inside the placeable world is not an *alternate candidate* at all, so it does
**	not spend one of the `retry.max_alternate_candidates_per_feature` slots, while a cell that
**	conflicts with other planned intent is exactly what that budget is for.  Either way the keyed
**	order and the set of acceptable positions are untouched.
*/
enum HybridCandidateVerdict
{
	HYBRID_CANDIDATE_ACCEPT = 0,
	HYBRID_CANDIDATE_DOES_NOT_FIT,
	HYBRID_CANDIDATE_CONFLICT
};


/*
**	Rule 5 and rule 6 together: the candidate's polygon must stay inside the placeable boundary,
**	clear of BASE_PROTECTED, clear of an incompatible Tiberium reserve and of an OPEN_MANEUVER
**	no-immutable-barrier cell when the feature is an immutable wall, must not starve a required
**	heavy corridor, and must not overlap an accepted independent feature of the same type by more
**	than a quarter of the smaller polygon.
*/
static HybridCandidateVerdict Candidate_Verdict(LandformBuilder & b,const HybridLandform & landform)
{
	const HybridSemanticMatrix & matrix = b.Matrices->Matrix;
	const bool immutable = Hybrid_Landform_Is_Immutable(landform.Kind);
	b.LastReject = "none";

	/*
	**	The polygon against the configured placeable boundary and the Phase-4 reservations, taken at
	**	the feature's own sample points so a band narrower than one coarse cell is still tested.
	*/
	DynamicVectorClass<Vector2> samples;
	Landform_Sample_Points(landform,&samples);
	if (samples.Count() == 0) return HYBRID_CANDIDATE_DOES_NOT_FIT;

	for (int i = 0; i < samples.Count(); ++i) {

		if (!WorldBoundsService::Can_Place_In_Region(samples[i],0.0f)) {
			b.LastReject = "outside the placeable boundary";
			return HYBRID_CANDIDATE_DOES_NOT_FIT;
		}

		int cx = 0, cy = 0;
		if (!matrix.World_To_Cell(samples[i],&cx,&cy)) {
			b.LastReject = "outside the planning matrix";
			return HYBRID_CANDIDATE_DOES_NOT_FIT;
		}
		if (!matrix.Has_Flag(cx,cy,HYBRID_FLAG_PLACEABLE)) {
			b.LastReject = "unplaceable cell";
			return HYBRID_CANDIDATE_DOES_NOT_FIT;
		}
		if (b.Matrices->Is_Base_Protected(cx,cy)) {
			b.LastReject = "BASE_PROTECTED";
			return HYBRID_CANDIDATE_CONFLICT;
		}
		if (immutable) {
			if (matrix.Has_Flag(cx,cy,HYBRID_FLAG_NO_IMMUTABLE_BARRIER)) {
				b.LastReject = "OPEN_MANEUVER no-immutable-barrier";
				return HYBRID_CANDIDATE_CONFLICT;
			}
			if (matrix.Has_Flag(cx,cy,HYBRID_FLAG_TIBERIUM_RESERVE)) {
				b.LastReject = "Tiberium reserve";
				return HYBRID_CANDIDATE_CONFLICT;
			}

			/*
			**	The reserve *discs* as well as their rasterized cells.  A cell is `S` wide and is flagged
			**	from its own centre, so a wall can reach up to half a cell into a reserve without ever
			**	standing in a flagged cell -- and Phase 8, which measures the same reserve geometrically,
			**	then refuses the whole attempt over ground this phase could simply have avoided.  Testing
			**	the geometry here keeps the two phases' idea of "inside the reserve" identical.
			*/
			for (int s = 0; s < b.Graph->Sites.Count(); ++s) {

				const HybridStrategicSite & site = b.Graph->Sites[s];
				if (site.Radius <= 0.0f) continue;

				const bool tiberium = (site.Role == HYBRID_SITE_HOME_TIBERIUM_GDI) ||
											 (site.Role == HYBRID_SITE_HOME_TIBERIUM_NOD) ||
											 (site.Role == HYBRID_SITE_CONTESTED_TIBERIUM) ||
											 (site.Role == HYBRID_SITE_REMOTE_TIBERIUM);
				const bool base = (site.Role == HYBRID_SITE_GDI_BASE) || (site.Role == HYBRID_SITE_NOD_BASE);
				if (!tiberium && !base) continue;
				if ((samples[i] - site.Position).Length() > site.Radius) continue;

				b.LastReject = tiberium ? "Tiberium reserve disc" : "BASE_PROTECTED disc";
				return HYBRID_CANDIDATE_CONFLICT;
			}
		}
	}
	Note_Cells(b,samples.Count());

	if (immutable && !landform.Required && !Corridor_Width_Survives(b,landform)) {
		b.LastReject = "heavy corridor dry width";
		return HYBRID_CANDIDATE_CONFLICT;
	}

	/*
	**	Rule 6, against accepted independent features of the same type only.  Section 16.2: the
	**	candidates that can overlap at all come from the planning spatial hash, never from a scan of
	**	every accepted feature.
	*/
	DynamicVectorClass<unsigned int> nearby;
	b.Out->Hash.Query(landform.BoundsMin.X - b.P->WorldMinX,landform.BoundsMin.Y - b.P->WorldMinY,
							landform.BoundsMax.X - b.P->WorldMinX,landform.BoundsMax.Y - b.P->WorldMinY,&nearby);

	float own_area = -1.0f;
	for (int i = 0; i < nearby.Count(); ++i) {

		const HybridLandform * found = b.Out->Find(nearby[i]);
		if (found == nullptr) continue;
		const HybridLandform & other = *found;
		if (other.Kind != landform.Kind) continue;

		//	Members of one canyon/ravine system are not independent features of each other.
		if ((other.SystemId != 0) && (other.SystemId == landform.SystemId)) continue;

		if ((landform.BoundsMax.X < other.BoundsMin.X) || (landform.BoundsMin.X > other.BoundsMax.X) ||
			 (landform.BoundsMax.Y < other.BoundsMin.Y) || (landform.BoundsMin.Y > other.BoundsMax.Y)) continue;

		const float overlap = Polygon_Overlap_Area(b,landform,other);
		if (overlap <= 0.0f) continue;

		if (own_area < 0.0f) own_area = Polygon_Area(b,landform);
		const float other_area = Polygon_Area(b,other);
		const float smaller = Min2(own_area,other_area);
		if (smaller <= 0.0f) continue;
		if (overlap > (OVERLAP_MAX_FRACTION * smaller)) {
			b.LastReject = "same-type overlap";
			return HYBRID_CANDIDATE_CONFLICT;
		}
	}

	return HYBRID_CANDIDATE_ACCEPT;
}


/*
**	Phase 5 owns `LandformMask`, `RoughnessClass` and the landform flag bits; Phase 4 leaves all
**	three clear.  The roughest class in a cell wins, because that is what decoration, material and
**	tactical passes must respect.
*/
static void Raise_Roughness(HybridSemanticMatrix & matrix,int index,HybridRoughnessClass roughness)
{
	if ((unsigned char)roughness > matrix.RoughnessClass[index]) {
		matrix.RoughnessClass[index] = (unsigned char)roughness;
	}
}


static void Stamp_Landform(LandformBuilder & b,const HybridLandform & landform)
{
	HybridSemanticMatrix & matrix = b.Matrices->Matrix;

	int cx0 = 0, cy0 = 0, cx1 = 0, cy1 = 0;
	matrix.World_To_Cell(landform.BoundsMin,&cx0,&cy0);
	matrix.World_To_Cell(landform.BoundsMax,&cx1,&cy1);
	cx0 = (int)Clamp2((float)cx0,0.0f,(float)(matrix.Get_Count_X() - 1));
	cy0 = (int)Clamp2((float)cy0,0.0f,(float)(matrix.Get_Count_Y() - 1));
	cx1 = (int)Clamp2((float)cx1,0.0f,(float)(matrix.Get_Count_X() - 1));
	cy1 = (int)Clamp2((float)cy1,0.0f,(float)(matrix.Get_Count_Y() - 1));

	for (int cy = cy0; cy <= cy1; ++cy) {
		for (int cx = cx0; cx <= cx1; ++cx) {

			const Vector2 centre = matrix.Cell_Center(cx,cy);
			float profile = 0.0f;
			Hybrid_Landform_Contribution(landform,centre,&profile);
			if (profile <= 0.0f) continue;

			const int index = matrix.Index(cx,cy);

			switch (landform.Kind) {

				case HYBRID_LANDFORM_HILL:
					matrix.Set_Flag(cx,cy,HYBRID_FLAG_HILL);
					matrix.LandformMask[index] |= HYBRID_LANDFORM_MASK_HILL;
					Raise_Roughness(matrix,index,HYBRID_ROUGHNESS_ROLLING);
					break;

				case HYBRID_LANDFORM_RIDGE:
					matrix.Set_Flag(cx,cy,HYBRID_FLAG_RIDGE);
					matrix.LandformMask[index] |= HYBRID_LANDFORM_MASK_RIDGE;
					Raise_Roughness(matrix,index,(profile >= (1.0f - RIDGE_CORE_FRACTION))
															? HYBRID_ROUGHNESS_STEEP : HYBRID_ROUGHNESS_ROLLING);
					break;

				case HYBRID_LANDFORM_CLIFF:
				{
					matrix.Set_Flag(cx,cy,HYBRID_FLAG_CLIFF_BAND);
					float lateral = 0.0f;
					Polyline_Measure(landform,centre,&lateral,nullptr,nullptr);
					const float face = (0.5f * landform.EdgeWidth) + landform.TalusWidth;
					if (WWMath::Fabs(lateral) <= face) {
						matrix.LandformMask[index] |= HYBRID_LANDFORM_MASK_CLIFF_FACE;
						Raise_Roughness(matrix,index,HYBRID_ROUGHNESS_BROKEN);
					} else {
						Raise_Roughness(matrix,index,HYBRID_ROUGHNESS_ROLLING);
					}
					break;
				}

				case HYBRID_LANDFORM_CANYON:
				case HYBRID_LANDFORM_RAVINE:
				{
					const bool canyon = (landform.Kind == HYBRID_LANDFORM_CANYON);
					matrix.Set_Flag(cx,cy,canyon ? HYBRID_FLAG_CANYON_BAND : HYBRID_FLAG_RAVINE_BAND);

					float lateral = 0.0f;
					Polyline_Measure(landform,centre,&lateral,nullptr,nullptr);
					if (WWMath::Fabs(lateral) <= landform.FloorHalfWidth) {
						matrix.LandformMask[index] |= (unsigned char)(canyon ? HYBRID_LANDFORM_MASK_CANYON_FLOOR
																								 : HYBRID_LANDFORM_MASK_RAVINE_FLOOR);
						Raise_Roughness(matrix,index,HYBRID_ROUGHNESS_ROLLING);
					} else {
						matrix.LandformMask[index] |= (unsigned char)(canyon ? HYBRID_LANDFORM_MASK_CANYON_WALL
																								 : HYBRID_LANDFORM_MASK_RAVINE_WALL);
						Raise_Roughness(matrix,index,HYBRID_ROUGHNESS_STEEP);
					}
					break;
				}

				default:
					break;
			}
		}
	}
}


/*
**	A band narrower than one coarse cell can pass between cell centres, so a line feature also
**	stamps the cell each centerline sample stands in: a cliff face or a ravine floor must appear in
**	the matrix even when it is thinner than `S`.
*/
static void Stamp_Spine(LandformBuilder & b,const HybridLandform & landform)
{
	if (!Hybrid_Landform_Is_Line(landform.Kind)) return;

	HybridSemanticMatrix & matrix = b.Matrices->Matrix;
	const bool canyon = (landform.Kind == HYBRID_LANDFORM_CANYON);

	for (int i = 0; i < landform.Points.Count(); ++i) {

		int cx = 0, cy = 0;
		if (!matrix.World_To_Cell(landform.Points[i],&cx,&cy)) continue;
		const int index = matrix.Index(cx,cy);

		switch (landform.Kind) {
			case HYBRID_LANDFORM_RIDGE:
				matrix.Set_Flag(cx,cy,HYBRID_FLAG_RIDGE);
				matrix.LandformMask[index] |= HYBRID_LANDFORM_MASK_RIDGE;
				Raise_Roughness(matrix,index,HYBRID_ROUGHNESS_STEEP);
				break;

			case HYBRID_LANDFORM_CLIFF:
				matrix.Set_Flag(cx,cy,HYBRID_FLAG_CLIFF_BAND);
				matrix.LandformMask[index] |= HYBRID_LANDFORM_MASK_CLIFF_FACE;
				Raise_Roughness(matrix,index,HYBRID_ROUGHNESS_BROKEN);
				break;

			case HYBRID_LANDFORM_CANYON:
			case HYBRID_LANDFORM_RAVINE:
				matrix.Set_Flag(cx,cy,canyon ? HYBRID_FLAG_CANYON_BAND : HYBRID_FLAG_RAVINE_BAND);
				matrix.LandformMask[index] |= (unsigned char)(canyon ? HYBRID_LANDFORM_MASK_CANYON_FLOOR
																						: HYBRID_LANDFORM_MASK_RAVINE_FLOOR);
				Raise_Roughness(matrix,index,HYBRID_ROUGHNESS_ROLLING);
				break;

			default:
				break;
		}
	}
}


//	Accept a candidate: record it, stamp its cells and insert it into the planning spatial hash.
static void Accept_Landform(LandformBuilder & b,const HybridLandform & landform)
{
	b.Out->Landforms.Add(landform);
	Stamp_Landform(b,b.Out->Landforms[b.Out->Landforms.Count() - 1]);
	Stamp_Spine(b,b.Out->Landforms[b.Out->Landforms.Count() - 1]);
	b.Out->Hash.Insert(landform.StableId,
							 landform.BoundsMin.X - b.P->WorldMinX,landform.BoundsMin.Y - b.P->WorldMinY,
							 landform.BoundsMax.X - b.P->WorldMinX,landform.BoundsMax.Y - b.P->WorldMinY);
}


/*
**	One feature's reservation of the authoritative `StableId` space.
**
**	A landform's id is not only its name: `Landform_Hash` keys every dimension, jitter and axis draw on it,
**	so the id has to exist *before* the candidate positions are probed and has to stay the same across all
**	of them -- probing the same feature at ten stations must draw the same feature, not ten different ones.
**
**	What must not happen is the id being spent when the feature is rejected outright.  Probing is not
**	creation: a helper that gives up returns the id, so the next feature to be accepted takes it and the
**	authoritative collection stays in contiguous creation order.  Every existing caller aborts the phase on
**	a refusal, so this changes nothing for them; it is what lets a caller legitimately try a placement and
**	carry on without leaving a hole behind.
*/
struct LandformIdReservation
{
	LandformIdReservation(LandformBuilder & builder) : B(builder),Reserved(builder.NextStableId),Kept(false)
	{
		B.NextStableId++;
	}

	~LandformIdReservation(void)
	{
		if (!Kept) B.NextStableId = Reserved;
	}

	unsigned int	Id(void) const	{ return Reserved; }
	void				Keep(void)		{ Kept = true; }

	LandformBuilder &	B;
	unsigned int		Reserved;
	bool					Kept;
};


/*
**	Step 3: the jittered centre and the keyed axis a generic feature is tried at in one candidate cell.
*/
static void Generic_Candidate_Pose(const LandformBuilder & b,const HybridLandform & landform,int cell,
											  Vector2 * centre_out,float * angle_out)
{
	const HybridSemanticMatrix & matrix = b.Matrices->Matrix;
	const float jitter = CANDIDATE_JITTER_S * b.P->Constants.S;
	const HybridLandformKind kind = landform.Kind;
	const int cx = cell % matrix.Get_Count_X();
	const int cy = cell / matrix.Get_Count_X();
	const Vector2 base = matrix.Cell_Center(cx,cy);

	centre_out->Set(
		base.X + Hybrid_Range(-jitter,+jitter,
									 Landform_Hash(b.P->TerrainSeed,kind,landform.StableId,"jitter_x",(unsigned int)cell)),
		base.Y + Hybrid_Range(-jitter,+jitter,
									 Landform_Hash(b.P->TerrainSeed,kind,landform.StableId,"jitter_y",(unsigned int)cell)));

	//	A line feature's axis is keyed once, like every other dimension.
	*angle_out = Hybrid_Landform_Is_Line(kind)
		? Hybrid_Range(0.0f,WWMATH_PI,Landform_Hash(b.P->TerrainSeed,kind,landform.StableId,"axis"),
							HYBRID_QUANTUM_ANGLE)
		: 0.0f;
}


/*
**	D-036 diagnostic, off unless the production census asks for it.  When a generic feature has
**	exhausted its alternates, every candidate cell is tried again -- not only the first
**	retry.max_alternate_candidates_per_feature -- and the first rule refusing each one is tallied,
**	so a search-budget shortfall can be told from a feature that fits nowhere.  It runs only after
**	the attempt has already failed, accepts nothing and changes no outcome.
*/
static bool _ExhaustiveDiagnostic = false;

void Hybrid_Landform_Set_Exhaustive_Diagnostic(bool enabled)
{
	_ExhaustiveDiagnostic = enabled;
}


static void Diagnose_Exhaustion(LandformBuilder & b,const HybridLandform & drawn,StringClass * detail)
{
	enum { MAX_REASONS = 16 };
	const char *	reasons[MAX_REASONS];
	int				counts[MAX_REASONS];
	int				reason_count = 0;

	int tested = 0;
	int legal = 0;
	int first_legal_ordinal = 0;
	const char * const saved_reject = b.LastReject;

	for (int candidate = 0; candidate < b.CandidateCount; ++candidate) {

		HybridLandform landform = drawn;
		Vector2 centre;
		float angle = 0.0f;
		Generic_Candidate_Pose(b,landform,b.Order[candidate].Cell,&centre,&angle);

		const char * reason = nullptr;
		bool counts_as_tested = false;
		if (!Build_Geometry(b,&landform,centre,angle)) {
			reason = "degenerate geometry";
		} else if (!Clamp_To_Elevation_Envelope(b,&landform)) {
			reason = "elevation envelope";
			counts_as_tested = true;
		} else {
			const HybridCandidateVerdict verdict = Candidate_Verdict(b,landform);
			if (verdict == HYBRID_CANDIDATE_ACCEPT) {
				tested++;
				legal++;
				if (first_legal_ordinal == 0) first_legal_ordinal = tested;
				continue;
			}
			reason = b.LastReject;
			counts_as_tested = (verdict == HYBRID_CANDIDATE_CONFLICT);
		}
		if (counts_as_tested) tested++;

		int r = 0;
		while ((r < reason_count) && (::strcmp(reasons[r],reason) != 0)) ++r;
		if (r == reason_count) {
			if (reason_count == MAX_REASONS) continue;
			reasons[reason_count] = reason;
			counts[reason_count] = 0;
			reason_count++;
		}
		counts[r]++;
	}
	b.LastReject = saved_reject;

	StringClass line;
	line.Format(" | exhaustive: %d cells, %d tested, %d legal",b.CandidateCount,tested,legal);
	*detail += line;
	if (legal > 0) {
		line.Format(" (first at tested ordinal %d)",first_legal_ordinal);
		*detail += line;
	}
	*detail += "; first refusal:";
	for (int r = 0; r < reason_count; ++r) {
		line.Format("%s %s %d",(r == 0) ? "" : ",",reasons[r],counts[r]);
		*detail += line;
	}
}


/*
**	A generic feature: the plan's six-step candidate process, bounded by
**	retry.max_alternate_candidates_per_feature.
*/
static bool Place_Generic(LandformBuilder & b,HybridLandformKind kind,unsigned int system_id,
								  HybridLandformResult * result)
{
	LandformIdReservation reservation(b);

	HybridLandform landform;
	landform.StableId	= reservation.Id();
	landform.Kind		= kind;
	landform.SystemId	= system_id;
	Draw_Dimensions(b,&landform,HYBRID_LANDFORM_ROLE_GENERIC);

	if (!Clamp_To_Placeable_Span(b,&landform)) {
		StringClass detail;
		detail.Format("%s %u does not fit the placeable interior (%g across)",Hybrid_Landform_Kind_Name(kind),
						  landform.StableId,b.PlaceableSpan);
		return Fail(result,HYBRID_LANDFORM_CANDIDATES_EXHAUSTED,detail.Peek_Buffer());
	}

	Prepare_Candidate_Order(b,kind,landform.StableId);
	const HybridLandform drawn = landform;

	int tested = 0;
	int elevation_refusals = 0;

	for (int candidate = 0; candidate < b.CandidateCount; ++candidate) {

		if (tested >= b.P->MaxAlternateCandidatesPerFeature) break;

		Vector2 centre;
		float angle = 0.0f;
		Generic_Candidate_Pose(b,landform,b.Order[candidate].Cell,&centre,&angle);

		if (!Build_Geometry(b,&landform,centre,angle)) {
			b.LastReject = "degenerate geometry";
			continue;
		}
		if (!Clamp_To_Elevation_Envelope(b,&landform)) {
			b.LastReject = "elevation envelope";
			tested++;
			elevation_refusals++;
			b.Out->Rejections++;
			continue;
		}

		const HybridCandidateVerdict verdict = Candidate_Verdict(b,landform);
		if (verdict == HYBRID_CANDIDATE_CONFLICT) {
			tested++;
			b.Out->Rejections++;
			continue;
		}
		if (verdict == HYBRID_CANDIDATE_DOES_NOT_FIT) continue;

		Note_Candidate(b);
		landform.CandidateAttempts = tested + 1;
		Accept_Landform(b,landform);
		reservation.Keep();
		return true;
	}

	StringClass detail;
	detail.Format("%s %u exhausted %d alternate candidates, last refused by %s",Hybrid_Landform_Kind_Name(kind),
					  landform.StableId,tested,(b.LastReject != nullptr) ? b.LastReject : "?");
	if (_ExhaustiveDiagnostic) Diagnose_Exhaustion(b,drawn,&detail);

	//	Nothing but the elevation envelope refused it: that is a profile envelope too tight for this
	//	feature's drawn depth or amplitude, not a crowded map, and it is worth saying so by name.
	if ((tested > 0) && (elevation_refusals == tested)) {
		return Fail(result,HYBRID_LANDFORM_ELEVATION_LIMIT,detail.Peek_Buffer());
	}
	return Fail(result,HYBRID_LANDFORM_CANDIDATES_EXHAUSTED,detail.Peek_Buffer());
}


/*
**	An archetype principal feature.  Section 6 fixes its centre guide and its axis; only the
**	Section-6.1 bounded legality offsets may move it, and it is never silently discarded.
*/
struct RequiredPlacement
{
	LandformBuilder *	B;
	HybridLandform *	Landform;
	float					Angle;
};


static bool Required_Placement_Test(const Vector2 & candidate,void * context)
{
	RequiredPlacement * place = (RequiredPlacement *)context;
	if (place == nullptr) return false;

	LandformBuilder & b = *place->B;
	HybridLandform & landform = *place->Landform;

	Note_Candidate(b);
	if (!Build_Geometry(b,&landform,candidate,place->Angle)) return false;
	if (!Clamp_To_Elevation_Envelope(b,&landform)) return false;
	return (Candidate_Verdict(b,landform) == HYBRID_CANDIDATE_ACCEPT);
}


static bool Place_Required(LandformBuilder & b,HybridLandformKind kind,const HybridArchetypeIntent & intent,
									unsigned int system_id,unsigned int parent_id,const Vector2 & centre,float angle,
									unsigned int * out_stable_id,HybridLandformResult * result)
{
	LandformIdReservation reservation(b);

	HybridLandform landform;
	landform.StableId			= reservation.Id();
	landform.Kind				= kind;
	landform.SystemId			= system_id;
	landform.ParentStableId	= parent_id;
	landform.IntentStableId	= intent.StableId;
	landform.Required			= true;
	Draw_Dimensions(b,&landform,HYBRID_LANDFORM_ROLE_PRINCIPAL);

	if (!Clamp_To_Placeable_Span(b,&landform)) {
		StringClass detail;
		detail.Format("archetype %s from intent %u does not fit the placeable interior (%g across)",
						  Hybrid_Landform_Kind_Name(kind),intent.StableId,b.PlaceableSpan);
		return Fail(result,HYBRID_LANDFORM_REQUIRED_UNPLACEABLE,detail.Peek_Buffer());
	}

	RequiredPlacement place;
	place.B			= &b;
	place.Landform	= &landform;
	place.Angle		= angle;

	if (Required_Placement_Test(centre,&place)) {
		Accept_Landform(b,landform);
		reservation.Keep();
		if (out_stable_id != nullptr) *out_stable_id = landform.StableId;
		return true;
	}

	//	Section 6.1: concentric rings at A, 2A, ... up to 0.10D, 16 stations from +u counter-clockwise.
	Vector2 repaired(0.0f,0.0f);
	if (Hybrid_Repair_Ring_Search(b.Identity->Frame,b.P->Constants.A,REPAIR_LIMIT_D * b.P->Constants.D,
											centre,Required_Placement_Test,&place,&repaired)) {
		//	The search leaves the accepted geometry in place; rebuild once so the record is exact.
		if (Required_Placement_Test(repaired,&place)) {
			b.Out->Moves++;
			Accept_Landform(b,landform);
			reservation.Keep();
			if (out_stable_id != nullptr) *out_stable_id = landform.StableId;
			return true;
		}
	}

	StringClass detail;
	detail.Format("archetype %s from intent %u has no legal position within 0.10D",
					  Hybrid_Landform_Kind_Name(kind),intent.StableId);
	return Fail(result,HYBRID_LANDFORM_REQUIRED_UNPLACEABLE,detail.Peek_Buffer());
}


/*
**	Section 6.8's attached branches.  The branch's inner endpoint sits on a keyed station of the
**	main ravine and it runs out along the local normal, so the join is exact and Section 10.3's
**	`A`-wide smooth union owns the geometry where they meet.  A retry advances to the next station.
*/
static bool Place_Ravine_Branch(LandformBuilder & b,const HybridLandform & parent,int branch_index,
										  unsigned int system_id,const HybridArchetypeIntent & intent,
										  HybridLandformResult * result)
{
	LandformIdReservation reservation(b);

	HybridLandform landform;
	landform.StableId			= reservation.Id();
	landform.Kind				= HYBRID_LANDFORM_RAVINE;
	landform.SystemId			= system_id;
	landform.ParentStableId	= parent.StableId;
	landform.IntentStableId	= intent.StableId;
	landform.Required			= true;
	Draw_Dimensions(b,&landform,HYBRID_LANDFORM_ROLE_PRINCIPAL);

	if (!Clamp_To_Placeable_Span(b,&landform)) {
		return Fail(result,HYBRID_LANDFORM_REQUIRED_UNPLACEABLE,"a ravine branch does not fit the placeable interior");
	}

	const int station_count = parent.Points.Count();
	if (station_count < 3) {
		return Fail(result,HYBRID_LANDFORM_REQUIRED_UNPLACEABLE,"the main ravine has no interior station");
	}

	/*
	**	Interior stations only, in ascending keyed order, so a branch never starts at an endpoint and
	**	the ordering does not depend on the container.
	*/
	const int interior = station_count - 2;
	int tried = 0;
	float last_key = 0.0f;
	int last_station = -1;
	bool started = false;

	while (tried < b.P->MaxAlternateCandidatesPerFeature) {

		bool  found = false;
		float best_key = 0.0f;
		int	best_station = -1;

		for (int i = 1; i <= interior; ++i) {
			const float key = Landform_Hash(b.P->TerrainSeed,HYBRID_LANDFORM_RAVINE,landform.StableId,
													  "branch_station",(unsigned int)i);
			if (started) {
				if ((key < last_key) || ((key == last_key) && (i <= last_station))) continue;
			}
			if (found && ((key > best_key) || ((key == best_key) && (i > best_station)))) continue;
			found			= true;
			best_key		= key;
			best_station= i;
		}
		if (!found) break;

		started		= true;
		last_key		= best_key;
		last_station= best_station;
		tried++;
		Note_Candidate(b);
		landform.CandidateAttempts = tried;

		//	Local tangent at the station, then the keyed side of its normal.
		const Vector2 & here = parent.Points[best_station];
		const Vector2 delta = parent.Points[best_station + 1] - parent.Points[best_station - 1];
		const float length = delta.Length();
		if (length <= 0.0f) continue;
		const Vector2 tangent = delta * (1.0f / length);
		const float side = (float)Hybrid_Pick_Sign(
									Landform_Hash(b.P->TerrainSeed,HYBRID_LANDFORM_RAVINE,landform.StableId,
														"branch_side",(unsigned int)best_station));
		const Vector2 normal(-tangent.Y * side,tangent.X * side);

		const float angle = (float)::atan2((double)normal.Y,(double)normal.X);
		const Vector2 centre = here + (normal * (0.5f * landform.Length));

		if (!Build_Geometry(b,&landform,centre,angle)) { b.Out->Rejections++; continue; }
		if (!Clamp_To_Elevation_Envelope(b,&landform)) { b.Out->Rejections++; continue; }
		if (Candidate_Verdict(b,landform) != HYBRID_CANDIDATE_ACCEPT) { b.Out->Rejections++; continue; }

		Accept_Landform(b,landform);
		return true;
	}

	StringClass detail;
	detail.Format("ravine branch %d exhausted %d stations on main ravine %u, last refused by %s",branch_index,
					  tried,parent.StableId,(b.LastReject != nullptr) ? b.LastReject : "?");
	return Fail(result,HYBRID_LANDFORM_REQUIRED_UNPLACEABLE,detail.Peek_Buffer());
}


/*
**	Step 1 of the creation order: the archetype principal ridge/canyon/ravine features, straight
**	from the frozen Section-6.10 intents.  `plateau_basin`'s basin and `coastal_peninsula`'s coast
**	are not landform vectors -- Phase 4 already owns the basin macro zone and Phase 6 owns every
**	water polygon -- so this phase creates no feature for them.
*/
static bool Place_Archetype_Features(LandformBuilder & b,HybridLandformResult * result)
{
	const HybridStrategicFrame & frame = b.Identity->Frame;
	//	"Approximately along strategic v": the endpoint vector is parallel to +v; only the keyed
	//	interior lateral offsets create curvature.
	const float v_angle = (float)::atan2((double)frame.AxisV.Y,(double)frame.AxisV.X);

	for (int i = 0; i < b.Intents->Intents.Count(); ++i) {

		const HybridArchetypeIntent & intent = b.Intents->Intents[i];
		const Vector2 guide = frame.To_World(intent.Guide);

		switch (intent.Kind) {

			case HYBRID_INTENT_PRINCIPAL_CANYON:
			{
				const unsigned int system = b.NextSystemId++;
				if (!Place_Required(b,HYBRID_LANDFORM_CANYON,intent,system,0,guide,v_angle,nullptr,result)) {
					return false;
				}
				break;
			}

			case HYBRID_INTENT_PRINCIPAL_RIDGE:
			{
				unsigned int ridge_id = 0;
				if (!Place_Required(b,HYBRID_LANDFORM_RIDGE,intent,0,0,guide,v_angle,&ridge_id,result)) {
					return false;
				}

				/*
				**	Section 6.10's mandatory surface saddle, on this ridge only.  The 0.15 amplitude
				**	zone is widened symmetrically until its footprint fits a PRIMARY_2 roadbed plus
				**	2 * fairness.min_vehicle_lane_clearance.
				*/
				const HybridArchetypeIntent * saddle = b.Intents->Find_First(HYBRID_INTENT_SURFACE_SADDLE);
				if (saddle != nullptr) {
					HybridLandform & ridge = b.Out->Landforms[b.Out->Landforms.Count() - 1];
					const Vector2 saddle_world = frame.To_World(saddle->Guide);
					float station = 0.0f;
					Polyline_Measure(ridge,saddle_world,nullptr,&station,nullptr);

					const float required = Hybrid_Roadbed_Width(HYBRID_ROAD_PRIMARY_2,b.P->Constants.W)
												+ (2.0f * b.P->MinVehicleLaneClearance);
					ridge.SaddlePresent			= true;
					ridge.SaddleStation			= Hybrid_Quantize_Position(station);
					ridge.SaddleFlatHalfLength	= Hybrid_Quantize_Position(
															Max2(SADDLE_ZONE_W * b.P->Constants.W,0.5f * required));
					ridge.SaddleRampLength		= Hybrid_Quantize_Position(SADDLE_ZONE_W * b.P->Constants.W);
				}
				break;
			}

			case HYBRID_INTENT_MAIN_RAVINE_GUIDE:
			{
				const unsigned int system = b.NextSystemId++;
				unsigned int main_id = 0;
				if (!Place_Required(b,HYBRID_LANDFORM_RAVINE,intent,system,0,guide,v_angle,&main_id,result)) {
					return false;
				}

				const int branches = Hybrid_Ravine_Branch_Count(b.Identity->SizeClass);
				for (int branch = 0; branch < branches; ++branch) {
					//	The parent record may move in the vector as branches are added, so refetch it.
					const HybridLandform * parent = b.Out->Find(main_id);
					if (parent == nullptr) {
						return Fail(result,HYBRID_LANDFORM_REQUIRED_UNPLACEABLE,"the main ravine record is gone");
					}
					HybridLandform parent_copy = *parent;
					if (!Place_Ravine_Branch(b,parent_copy,branch,system,intent,result)) return false;
				}
				break;
			}

			default:
				break;
		}
	}
	return true;
}


/*
**	Do two segments cross?  The ordinary 2D parametric test, on world coordinates both sides are already
**	quantized to.
*/
static bool Segments_Intersect(const Vector2 & a0,const Vector2 & a1,const Vector2 & b0,const Vector2 & b1)
{
	const Vector2 r = a1 - a0;
	const Vector2 s = b1 - b0;
	const float denominator = (r.X * s.Y) - (r.Y * s.X);
	if ((float)fabs((double)denominator) <= 1.0e-9f) return false;		// parallel or degenerate

	const Vector2 delta = b0 - a0;
	const float t = ((delta.X * s.Y) - (delta.Y * s.X)) / denominator;
	const float u = ((delta.X * r.Y) - (delta.Y * r.X)) / denominator;
	return (t >= 0.0f) && (t <= 1.0f) && (u >= 0.0f) && (u <= 1.0f);
}


/*
**	Does a required heavy route corridor cross this landform's centerline?
**
**	This is the question Phase 9 asks when it fills Section 5's canyon/ravine bridge quota: a bridge can
**	only be bound to an obstruction a heavy family actually has to get past.  Asking it here, at the phase
**	that decides where the obstruction goes, is what makes that quota satisfiable at all.
*/
static bool Heavy_Corridor_Crosses(const LandformBuilder & b,const HybridLandform & landform)
{
	if (landform.Points.Count() < 2) return false;

	for (int e = 0; e < b.Graph->Edges.Count(); ++e) {

		const HybridStrategicEdge & edge = b.Graph->Edges[e];
		if (edge.RouteFamily < 0) continue;					// not a base-to-base heavy route

		const HybridStrategicSite * a = b.Graph->Find_Site(edge.SiteA);
		const HybridStrategicSite * z = b.Graph->Find_Site(edge.SiteB);
		if ((a == nullptr) || (z == nullptr)) continue;

		for (int p = 1; p < landform.Points.Count(); ++p) {
			if (Segments_Intersect(a->Position,z->Position,landform.Points[p - 1],landform.Points[p])) {
				return true;
			}
		}
	}
	return false;
}


//	How many distinct canyon/ravine systems a required heavy corridor crosses.
static int Crossed_Canyon_System_Count(const LandformBuilder & b)
{
	DynamicVectorClass<unsigned int> crossed;

	for (int i = 0; i < b.Out->Landforms.Count(); ++i) {

		const HybridLandform & landform = b.Out->Landforms[i];
		if ((landform.Kind != HYBRID_LANDFORM_CANYON) && (landform.Kind != HYBRID_LANDFORM_RAVINE)) continue;
		if (landform.SystemId == 0) continue;
		if (!Heavy_Corridor_Crosses(b,landform)) continue;

		bool counted = false;
		for (int s = 0; (s < crossed.Count()) && !counted; ++s) {
			if (crossed[s] == landform.SystemId) counted = true;
		}
		if (!counted) crossed.Add(landform.SystemId);
	}
	return crossed.Count();
}


/*
**	Step 2a: lay the canyon/ravine systems Section 5's bridge quota needs *across* a required heavy
**	corridor, before the density-driven systems fill the rest of the target.
**
**	Section 5 asks every map for 1/1/2 canyon/ravine bridges and forbids reducing a quota to rescue a seed,
**	but Phase 9 can only bind a bridge to an obstruction a heavy route actually crosses.  Placing the
**	generic systems without regard to where the heavy routes run leaves most seeds with nothing bindable --
**	measured on `open_frontier`, every seed produced bridge-capable ravine windows and on every one of them
**	all four heavy sites stood on the same side of the crossing axis.  The archetypes carrying their own
**	principal canyon do not have this problem precisely because their intent guides them onto the strategic
**	axis the routes follow.  This does the same for the quota.
**
**	The guide is a point on a heavy family's own corridor and the axis is perpendicular to it, so a route
**	following that corridor has to cut through rather than run beside.  Stations walk out from each
**	corridor's midpoint in eighths, edges in record order, and every candidate still goes through the
**	ordinary required-placement test and the ordinary Section-6 rejection rules: nothing here weakens Phase
**	5's geometry, it only decides where to try first.  One accepted system satisfies one quota slot: a
**	system already counted is not counted again, which is what `Crossed_Canyon_System_Count` guarantees by
**	counting distinct `SystemId`s.
*/
static bool Place_Quota_Crossing_Systems(LandformBuilder & b,HybridLandformResult * result)
{
	const int required = b.P->CrossedCanyonSystemsRequired;
	if (required <= 0) return true;

	HybridArchetypeIntent synthetic;						// not from an archetype: the quota is its own reason
	const int stations = 9;									// the midpoint, then outwards in eighths
	const int ceiling = (required + 1) * b.P->MaxAlternateCandidatesPerFeature;

	int attempts = 0;
	while ((Crossed_Canyon_System_Count(b) < required) && (attempts < ceiling)) {

		bool placed = false;

		for (int e = 0; (e < b.Graph->Edges.Count()) && !placed; ++e) {

			const HybridStrategicEdge & edge = b.Graph->Edges[e];
			if (edge.RouteFamily < 0) continue;

			const HybridStrategicSite * a = b.Graph->Find_Site(edge.SiteA);
			const HybridStrategicSite * z = b.Graph->Find_Site(edge.SiteB);
			if ((a == nullptr) || (z == nullptr)) continue;

			Vector2 along = z->Position - a->Position;
			const float length = along.Length();
			if (length <= b.P->Constants.A) continue;
			along *= (1.0f / length);

			//	Perpendicular to the corridor: the axis a crossing road must actually cut through.
			const float angle = (float)::atan2((double)along.X,(double)(-along.Y));

			for (int s = 0; (s < stations) && !placed; ++s) {

				//	The midpoint first, then alternating outwards, so the choice never depends on order.
				const int step = (s + 1) / 2;
				const float sign = ((s % 2) == 0) ? 1.0f : -1.0f;
				const float ratio = Clamp2(0.5f + (sign * (float)step * 0.125f),0.1f,0.9f);
				const Vector2 guide(a->Position.X + (along.X * length * ratio),
										  a->Position.Y + (along.Y * length * ratio));

				const unsigned int system = b.NextSystemId;
				unsigned int placed_id = 0;
				HybridLandformResult scratch;
				attempts++;

				//	A refused probe costs nothing now: `Place_Required` returns the id it reserved.
				if (!Place_Required(b,HYBRID_LANDFORM_RAVINE,synthetic,system,0,guide,angle,&placed_id,
										  &scratch)) continue;

				b.NextSystemId++;
				placed = true;
			}
		}

		if (!placed) break;										// no corridor station will take one
	}

	if (Crossed_Canyon_System_Count(b) >= required) return true;

	StringClass detail;
	detail.Format("Section 5 asks for %d canyon/ravine systems on a heavy corridor and only %d could be laid "
					  "across one in %d attempts",required,Crossed_Canyon_System_Count(b),attempts);
	return Fail(result,HYBRID_LANDFORM_REQUIRED_UNPLACEABLE,detail.Peek_Buffer());
}


/*
**	Step 6: the optional plateau conversion.  Eligibility is exactly Section 8 Phase 5's list, and
**	the Bernoulli test is `Hash01(terrain,"plateau",StableId) < terrain.plateau_chance`.
*/
static bool Plateau_Is_Eligible(LandformBuilder & b,const HybridLandform & landform)
{
	if ((landform.Kind != HYBRID_LANDFORM_HILL) && (landform.Kind != HYBRID_LANDFORM_RIDGE)) return false;

	const HybridSemanticMatrix & matrix = b.Matrices->Matrix;
	const float corridor_clear = PLATEAU_CORRIDOR_W * b.P->Constants.W;

	int cx0 = 0, cy0 = 0, cx1 = 0, cy1 = 0;
	matrix.World_To_Cell(landform.BoundsMin,&cx0,&cy0);
	matrix.World_To_Cell(landform.BoundsMax,&cx1,&cy1);
	cx0 = (int)Clamp2((float)cx0,0.0f,(float)(matrix.Get_Count_X() - 1));
	cy0 = (int)Clamp2((float)cy0,0.0f,(float)(matrix.Get_Count_Y() - 1));
	cx1 = (int)Clamp2((float)cx1,0.0f,(float)(matrix.Get_Count_X() - 1));
	cy1 = (int)Clamp2((float)cy1,0.0f,(float)(matrix.Get_Count_Y() - 1));

	bool any = false;

	for (int cy = cy0; cy <= cy1; ++cy) {
		for (int cx = cx0; cx <= cx1; ++cx) {

			const Vector2 centre = matrix.Cell_Center(cx,cy);
			float profile = 0.0f;
			Hybrid_Landform_Contribution(landform,centre,&profile);
			if (profile < PLATEAU_TOP_FRACTION) continue;			// the proposed plateau footprint only

			any = true;
			if (b.Matrices->Is_Base_Protected(cx,cy)) return false;
			if (matrix.Has_Flag(cx,cy,HYBRID_FLAG_TIBERIUM_RESERVE)) return false;
			if (matrix.Has_Flag(cx,cy,HYBRID_FLAG_WATER_BASIN)) return false;
			if (matrix.Has_Flag(cx,cy,HYBRID_FLAG_BASIN)) return false;
			//	A plateau rim is an immutable wall, so an OPEN_MANEUVER disc excludes it.
			if (matrix.Has_Flag(cx,cy,HYBRID_FLAG_NO_IMMUTABLE_BARRIER)) return false;

			for (int r = 0; r < b.Matrices->Reservations.Count(); ++r) {
				const HybridReservation & reservation = b.Matrices->Reservations[r];
				if (reservation.Priority != HYBRID_RESERVE_HEAVY_CORRIDOR) continue;
				Vector2 axis = reservation.B - reservation.A;
				const float length = axis.Length();
				if (length <= 0.0f) continue;
				axis *= (1.0f / length);
				const Vector2 rel = centre - reservation.A;
				const float along = Clamp2((axis.X * rel.X) + (axis.Y * rel.Y),0.0f,length);
				const Vector2 closest = reservation.A + (axis * along);
				if ((centre - closest).Length() <= corridor_clear) return false;
			}
		}
	}
	return any;
}


static void Convert_Plateaus(LandformBuilder & b)
{
	for (int i = 0; i < b.Out->Landforms.Count(); ++i) {

		HybridLandform & landform = b.Out->Landforms[i];
		if ((landform.Kind != HYBRID_LANDFORM_HILL) && (landform.Kind != HYBRID_LANDFORM_RIDGE)) continue;
		if (!Plateau_Is_Eligible(b,landform)) continue;

		const float h = Hybrid_Hash01(b.P->TerrainSeed,Hybrid_Key("plateau"),landform.StableId);
		if (h >= b.P->PlateauChance) continue;

		landform.IsPlateau = true;
		b.Out->PlateauCount++;

		//	The flattened top is the region the eligibility test just walked.
		HybridSemanticMatrix & matrix = b.Matrices->Matrix;
		int cx0 = 0, cy0 = 0, cx1 = 0, cy1 = 0;
		matrix.World_To_Cell(landform.BoundsMin,&cx0,&cy0);
		matrix.World_To_Cell(landform.BoundsMax,&cx1,&cy1);
		cx0 = (int)Clamp2((float)cx0,0.0f,(float)(matrix.Get_Count_X() - 1));
		cy0 = (int)Clamp2((float)cy0,0.0f,(float)(matrix.Get_Count_Y() - 1));
		cx1 = (int)Clamp2((float)cx1,0.0f,(float)(matrix.Get_Count_X() - 1));
		cy1 = (int)Clamp2((float)cy1,0.0f,(float)(matrix.Get_Count_Y() - 1));

		for (int cy = cy0; cy <= cy1; ++cy) {
			for (int cx = cx0; cx <= cx1; ++cx) {
				const Vector2 centre = matrix.Cell_Center(cx,cy);
				float profile = 0.0f;
				Hybrid_Landform_Contribution(landform,centre,&profile);
				if (profile < 1.0f) continue;				// after conversion the flat top reads exactly 1
				matrix.Set_Flag(cx,cy,HYBRID_FLAG_PLATEAU);
				matrix.LandformMask[matrix.Index(cx,cy)] |= HYBRID_LANDFORM_MASK_PLATEAU_TOP;
			}
		}
	}
}


/*
**	The Phase 5 gate: the target counts, the Section-5 minimum qualifications, and no immutable
**	landform inside BASE_PROTECTED or an OPEN_MANEUVER no-immutable-barrier reservation.
*/
static bool Check_Gate(LandformBuilder & b,HybridLandformResult * result)
{
	HybridLandformSet & out = *b.Out;
	const HybridSemanticMatrix & matrix = b.Matrices->Matrix;
	const float w = b.P->Constants.W;
	const float d = b.P->Constants.D;
	StringClass detail;

	out.SystemCount = out.Count_Of_Systems();

	if ((out.Count_Of_Kind(HYBRID_LANDFORM_HILL) != out.TargetHills) ||
		 (out.Count_Of_Kind(HYBRID_LANDFORM_RIDGE) != out.TargetRidges) ||
		 (out.Count_Of_Kind(HYBRID_LANDFORM_CLIFF) != out.TargetCliffs) ||
		 (out.SystemCount != out.TargetSystems)) {
		detail.Format("hills %d/%d ridges %d/%d cliffs %d/%d systems %d/%d",
						  out.Count_Of_Kind(HYBRID_LANDFORM_HILL),out.TargetHills,
						  out.Count_Of_Kind(HYBRID_LANDFORM_RIDGE),out.TargetRidges,
						  out.Count_Of_Kind(HYBRID_LANDFORM_CLIFF),out.TargetCliffs,
						  out.SystemCount,out.TargetSystems);
		return Fail(result,HYBRID_LANDFORM_COUNT_MISMATCH,detail.Peek_Buffer());
	}

	for (int i = 0; i < out.Landforms.Count(); ++i) {

		const HybridLandform & landform = out.Landforms[i];

		if (landform.Kind == HYBRID_LANDFORM_CLIFF) {
			const float minimum = Hybrid_Cliff_Min_Length(w,d);
			if (landform.Centerline_Length() < minimum) {
				detail.Format("cliff %u is %.3f long, Section 5 needs %.3f",landform.StableId,
								  landform.Centerline_Length(),minimum);
				return Fail(result,HYBRID_LANDFORM_QUALIFICATION_FAILED,detail.Peek_Buffer());
			}
		}

		//	A system's qualification is its root's: a branch is part of the system, not a system.
		if ((landform.SystemId != 0) && (landform.ParentStableId == 0)) {
			const float minimum = Hybrid_Ravine_Min_Length(w,d);
			if (landform.Centerline_Length() < minimum) {
				detail.Format("%s system %u is %.3f long, Section 5 needs %.3f",
								  Hybrid_Landform_Kind_Name(landform.Kind),landform.SystemId,
								  landform.Centerline_Length(),minimum);
				return Fail(result,HYBRID_LANDFORM_QUALIFICATION_FAILED,detail.Peek_Buffer());
			}
			if (landform.Depth < Hybrid_Ravine_Min_Depth(b.P->TierStep)) {
				detail.Format("%s system %u is %.3f deep, Section 5 needs %.3f",
								  Hybrid_Landform_Kind_Name(landform.Kind),landform.SystemId,landform.Depth,
								  Hybrid_Ravine_Min_Depth(b.P->TierStep));
				return Fail(result,HYBRID_LANDFORM_QUALIFICATION_FAILED,detail.Peek_Buffer());
			}
		}

		if (!Hybrid_Landform_Is_Immutable(landform.Kind)) continue;

		DynamicVectorClass<Vector2> samples;
		Landform_Sample_Points(landform,&samples);

		for (int sample = 0; sample < samples.Count(); ++sample) {
			int cx = 0, cy = 0;
			if (!matrix.World_To_Cell(samples[sample],&cx,&cy)) continue;
			if (b.Matrices->Is_Base_Protected(cx,cy)) {
				detail.Format("%s %u reaches BASE_PROTECTED cell %d,%d",
								  Hybrid_Landform_Kind_Name(landform.Kind),landform.StableId,cx,cy);
				return Fail(result,HYBRID_LANDFORM_IMMUTABLE_IN_RESERVE,detail.Peek_Buffer());
			}
			if (matrix.Has_Flag(cx,cy,HYBRID_FLAG_NO_IMMUTABLE_BARRIER)) {
				detail.Format("%s %u reaches OPEN_MANEUVER cell %d,%d",
								  Hybrid_Landform_Kind_Name(landform.Kind),landform.StableId,cx,cy);
				return Fail(result,HYBRID_LANDFORM_IMMUTABLE_IN_RESERVE,detail.Peek_Buffer());
			}
		}
	}

	return true;
}


bool Hybrid_Point_In_Immutable_Landform(const HybridLandformSet & set,const Vector2 & world)
{
	for (int i = 0; i < set.Landforms.Count(); ++i) {

		const HybridLandform & landform = set.Landforms[i];
		if (!Hybrid_Landform_Is_Immutable(landform.Kind)) continue;
		if ((world.X < landform.BoundsMin.X) || (world.X > landform.BoundsMax.X) ||
			 (world.Y < landform.BoundsMin.Y) || (world.Y > landform.BoundsMax.Y)) continue;
		if (Hybrid_Landform_Contribution(landform,world,nullptr) != 0.0f) return true;
	}
	return false;
}


unsigned int Hybrid_Landform_Checksum(const HybridLandformSet & set,unsigned int seed)
{
	HybridHashStream stream(seed);

	stream.Add_Int(set.TargetHills);
	stream.Add_Int(set.TargetRidges);
	stream.Add_Int(set.TargetCliffs);
	stream.Add_Int(set.TargetSystems);
	stream.Add_Int(set.PlateauCount);
	stream.Add_Int(set.Landforms.Count());

	for (int i = 0; i < set.Landforms.Count(); ++i) {

		const HybridLandform & landform = set.Landforms[i];
		stream.Add_Uint(landform.StableId);
		stream.Add_Uint((unsigned int)landform.Kind);
		stream.Add_Uint(landform.SystemId);
		stream.Add_Uint(landform.ParentStableId);
		stream.Add_Uint(landform.IntentStableId);
		stream.Add_Bool(landform.Required);
		stream.Add_Bool(landform.IsPlateau);
		stream.Add_Int(landform.UpperSideSign);
		stream.Add_Length(landform.Center.X);
		stream.Add_Length(landform.Center.Y);
		stream.Add_Length(landform.RadiusX);
		stream.Add_Length(landform.RadiusY);
		stream.Add_Angle(landform.Orientation);
		stream.Add_Length(landform.Length);
		stream.Add_Length(landform.Amplitude);
		stream.Add_Length(landform.Depth);
		stream.Add_Length(landform.HeightDifference);
		stream.Add_Length(landform.InfluenceHalfWidth);
		stream.Add_Length(landform.FloorHalfWidth);
		stream.Add_Length(landform.RimHalfWidth);
		stream.Add_Length(landform.EdgeWidth);
		stream.Add_Length(landform.TalusWidth);
		stream.Add_Bool(landform.SaddlePresent);
		stream.Add_Length(landform.SaddleStation);
		stream.Add_Length(landform.SaddleFlatHalfLength);
		stream.Add_Length(landform.SaddleRampLength);
		stream.Add_Int(landform.Points.Count());
		for (int point = 0; point < landform.Points.Count(); ++point) {
			stream.Add_Length(landform.Points[point].X);
			stream.Add_Length(landform.Points[point].Y);
		}
	}

	return stream.Get();
}


/***********************************************************************************************
 * Hybrid_Build_Landforms -- runtime Phase 5                                                    *
 *=============================================================================================*/
bool Hybrid_Build_Landforms(const HybridLandformParams & params,const HybridMapIdentity & identity,
										const HybridArchetypeIntentSet & intents,const HybridStrategicGraph & graph,
										HybridCoarseMatrices * matrices,HybridLandformSet * out,
										HybridLandformResult * result)
{
	if (result != nullptr) {
		result->Failure	= HYBRID_LANDFORM_OK;
		result->Detail		= "";
	}
	if ((matrices == nullptr) || (out == nullptr)) {
		return Fail(result,HYBRID_LANDFORM_INVALID_INPUT,"no matrices or no output record");
	}

	out->Reset();

	if ((matrices->Matrix.Get_Cell_Count() <= 0) || (params.Constants.A <= 0.0f) ||
		 (params.Constants.S <= 0.0f) || (params.Constants.W <= 0.0f) || (params.Constants.D <= 0.0f) ||
		 (params.WorldWidth <= 0.0f) || (params.WorldHeight <= 0.0f) || (params.TierStep <= 0.0f)) {
		return Fail(result,HYBRID_LANDFORM_INVALID_INPUT,"degenerate constants, world rectangle or matrix");
	}
	if (params.MaxElevation <= params.MinElevation) {
		return Fail(result,HYBRID_LANDFORM_INVALID_INPUT,"max_elevation is not above min_elevation");
	}

	LandformBuilder b;
	b.P					= &params;
	b.Identity			= &identity;
	b.Intents			= &intents;
	b.Graph				= &graph;
	b.Matrices			= matrices;
	b.Out					= out;
	b.Candidates		= nullptr;
	b.CandidateCount	= 0;
	b.Order				= nullptr;
	b.PlaceableMinX	= 0.0f;
	b.PlaceableMinY	= 0.0f;
	b.PlaceableMaxX	= 0.0f;
	b.PlaceableMaxY	= 0.0f;
	b.PlaceableSpan	= 0.0f;
	b.NextStableId		= 1;
	b.NextSystemId		= 1;
	b.LastReject		= "none";

	out->TargetHills	= Hybrid_Hill_Target_Count(identity.SizeClass,params.HillDensity);
	out->TargetRidges	= Hybrid_Ridge_Target_Count(identity.SizeClass,params.MountainDensity);
	out->TargetCliffs	= Hybrid_Cliff_Target_Count(identity.SizeClass,params.InteriorCliffDensity);
	out->TargetSystems= Hybrid_Ravine_System_Target_Count(identity.SizeClass,params.ValleyDensity);

	out->Hash.Init(params.WorldWidth,params.WorldHeight,params.Constants.BHash);

	if (!Build_Candidate_Cells(b,result)) {
		out->Reset();
		return false;
	}

	//	1 -- archetype principal features.
	if (!Place_Archetype_Features(b,result)) { out->Reset(); return false; }

	/*
	**	2a -- Section 5's canyon/ravine bridge quota, which needs systems a heavy route actually crosses or
	**	Phase 9 has nothing to bind a bridge to.  It runs before the density fill so its systems count toward
	**	the same target rather than adding to it, and the target is raised to the quota when the quota is the
	**	larger of the two, because a Section-5 quota is a floor on the world.  See `DECISIONS.md` D-011.
	*/
	if (out->TargetSystems < params.CrossedCanyonSystemsRequired) {
		out->TargetSystems = params.CrossedCanyonSystemsRequired;
	}
	if (!Place_Quota_Crossing_Systems(b,result)) { out->Reset(); return false; }

	//	2b -- the remaining required canyon/ravine systems.  terrain.valley_density is the ravine knob,
	//	so a density-created system is a ravine; a canyon exists only where an archetype requires one.
	while (out->Count_Of_Systems() < out->TargetSystems) {
		const unsigned int system = b.NextSystemId++;
		if (!Place_Generic(b,HYBRID_LANDFORM_RAVINE,system,result)) { out->Reset(); return false; }
	}

	//	3 -- explicit cliff runs.
	while (out->Count_Of_Kind(HYBRID_LANDFORM_CLIFF) < out->TargetCliffs) {
		if (!Place_Generic(b,HYBRID_LANDFORM_CLIFF,0,result)) { out->Reset(); return false; }
	}

	//	4 -- the remaining ridges.
	while (out->Count_Of_Kind(HYBRID_LANDFORM_RIDGE) < out->TargetRidges) {
		if (!Place_Generic(b,HYBRID_LANDFORM_RIDGE,0,result)) { out->Reset(); return false; }
	}

	//	5 -- hills.
	while (out->Count_Of_Kind(HYBRID_LANDFORM_HILL) < out->TargetHills) {
		if (!Place_Generic(b,HYBRID_LANDFORM_HILL,0,result)) { out->Reset(); return false; }
	}

	//	6 -- the optional plateau conversion.
	Convert_Plateaus(b);

	if (!Check_Gate(b,result)) { out->Reset(); return false; }

	out->Checksum = Hybrid_Landform_Checksum(*out,identity.Checksum);

	if (params.Counters != nullptr) {
		params.Counters->Note_Peak(HYBRID_WORK_PEAK_SCRATCH_BYTES,
											(params.Arena != nullptr) ? params.Arena->Get_High_Water() : 0);
	}

	WWDEBUG_SAY(("Hybrid Phase 5: %d landforms (%d hills, %d ridges, %d cliffs, %d systems, %d plateaus), "
					 "%d rejections, %d repairs, checksum 0x%08X\r\n",
					 out->Landforms.Count(),out->Count_Of_Kind(HYBRID_LANDFORM_HILL),
					 out->Count_Of_Kind(HYBRID_LANDFORM_RIDGE),out->Count_Of_Kind(HYBRID_LANDFORM_CLIFF),
					 out->SystemCount,out->PlateauCount,out->Rejections,out->Moves,out->Checksum));

	return true;
}
