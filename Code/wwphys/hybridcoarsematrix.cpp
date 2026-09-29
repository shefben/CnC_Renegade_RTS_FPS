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
 *                 Project Name : OpenW3D                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwphys/hybridcoarsematrix.cpp                $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Hybrid rewrite item I5 -- runtime Phase 4.  Two passes, in this order:                      *
 *                                                                                             *
 *   1. build the reservation set from the Phase-3 roles and the Section-6.9 discs, and resolve *
 *      every hard-reservation conflict by moving the lower-priority role through Section 6.1's *
 *      bounded legality repair;                                                               *
 *   2. populate the Section-2.2 layers in the plan's exact ten-step order from the resolved    *
 *      reservations, then check the Phase-4 gate.                                             *
 *                                                                                             *
 * Conflicts are resolved before rasterization on purpose: a repair moves a role, and a role    *
 * that moves after its disc is already stamped would leave the matrix describing a position    *
 * nothing occupies.  The ten-step population order the plan fixes is unaffected -- it is the   *
 * order the layers are written in, which is what later phases depend on.                      *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#include "hybridcoarsematrix.h"

#include "worldboundsservice.h"
#include "wwdebug.h"
#include "wwmath.h"

#include <string.h>


static inline float Max2(float a,float b)	{ return (a > b) ? a : b; }
static inline float Min2(float a,float b)	{ return (a < b) ? a : b; }


/*
**	Section 8 Phase 4 / Section 6.9 / Section 4.3 constants, each used exactly once below.
*/
static const float	RIVER_BAND_W						= 2.0f;		// Phase 3's own guided-river influence band
static const float	HEAVY_CORRIDOR_S					= 6.0f;		// preferred half-width max(6S,0.08D)
static const float	HEAVY_CORRIDOR_D					= 0.08f;
static const float	SERVICE_CORRIDOR_S				= 4.0f;		// preferred half-width max(4S,0.05D)
static const float	SERVICE_CORRIDOR_D				= 0.05f;
static const float	OPEN_MANEUVER_MIN_PLACEABLE	= 0.95f;		// Section 6.9 sampled-area rule
static const float	ROLE_REPAIR_LIMIT_D				= 0.10f;		// Section 6.1 repair budget
static const float	BIOME_MIN_OWNERSHIP				= 0.35f;		// Section 4.3 step 7
static const int		BIOME_MAX_SHIFTS					= 6;
static const float	BIOME_SHIFT_STEP					= 0.05f;
static const float	BIOME_TRANSITION_S				= 4.0f;		// Section 4.3 step 9: max(4S,8W)
static const float	BIOME_TRANSITION_W				= 8.0f;


const char * Hybrid_Reservation_Priority_Name(HybridReservationPriority priority)
{
	switch (priority) {
		case HYBRID_RESERVE_BASE_PROTECTED:		return "BASE_PROTECTED";
		case HYBRID_RESERVE_HEAVY_CORRIDOR:		return "HEAVY_CORRIDOR";
		case HYBRID_RESERVE_HOME_TIBERIUM:		return "HOME_TIBERIUM";
		case HYBRID_RESERVE_CROSSING_WINDOW:	return "CROSSING_WINDOW";
		case HYBRID_RESERVE_FIELD_TIBERIUM:		return "FIELD_TIBERIUM";
		case HYBRID_RESERVE_OPEN_MANEUVER:		return "OPEN_MANEUVER";
		case HYBRID_RESERVE_OPTIONAL_CORRIDOR:	return "OPTIONAL_CORRIDOR";
		case HYBRID_RESERVE_OPTIONAL_ISLAND:	return "OPTIONAL_ISLAND";
		default:											return "unknown";
	}
}


const char * Hybrid_Coarse_Matrix_Failure_Name(HybridCoarseMatrixFailure failure)
{
	switch (failure) {
		case HYBRID_COARSE_OK:									return "OK";
		case HYBRID_COARSE_INVALID_INPUT:					return "INVALID_INPUT";
		case HYBRID_COARSE_SCRATCH_EXHAUSTED:				return "SCRATCH_EXHAUSTED";
		case HYBRID_COARSE_OPEN_MANEUVER_UNPLACEABLE:	return "OPEN_MANEUVER_UNPLACEABLE";
		case HYBRID_COARSE_UNRESOLVED_CONFLICT:			return "UNRESOLVED_CONFLICT";
		case HYBRID_COARSE_BASE_ON_HARD_INTENT:			return "BASE_ON_HARD_INTENT";
		case HYBRID_COARSE_ROLE_OUTSIDE_RESERVATION:		return "ROLE_OUTSIDE_RESERVATION";
		default:														return "unknown";
	}
}


HybridCoarseMatrixParams::HybridCoarseMatrixParams(void) :
	MapPlanSeed(0),BiomeSeed(0),WorldMinX(0.0f),WorldMinY(0.0f),WorldWidth(0.0f),WorldHeight(0.0f),
	BoundaryMargin(0.0f),FieldRadiusMin(0.0f),FieldRadiusMax(0.0f),RiverWidth(0.0f),TierCount(3),
	Arena(nullptr),Counters(nullptr)
{
	::memset (&Constants,0,sizeof (Constants));
}


/***********************************************************************************************
 * HybridCoarseMatrices                                                                        *
 *=============================================================================================*/
void HybridCoarseMatrices::Reset(void)
{
	Matrix.Clear ();
	Reservations.Delete_All ();
	Divider					= HybridBiomeDivider ();
	DividerShifts			= 0;
	PrimaryLandFraction	= 0.0f;
	SecondaryLandFraction= 0.0f;
	PlayableCells			= 0;
	PlaceableCells			= 0;
	RoleMoves				= 0;
	Checksum					= 0;
}


const HybridReservation * HybridCoarseMatrices::Find_Reservation(unsigned int stable_id) const
{
	for (int i = 0; i < Reservations.Count (); i++) {
		if (Reservations[i].StableId == stable_id) return &Reservations[i];
	}
	return nullptr;
}


int HybridCoarseMatrices::Count_Of_Priority(HybridReservationPriority priority) const
{
	int count = 0;
	for (int i = 0; i < Reservations.Count (); i++) {
		if (Reservations[i].Priority == priority) count++;
	}
	return count;
}


bool HybridCoarseMatrices::Is_Base_Protected(int cx,int cy) const
{
	if (!Matrix.Is_Valid_Cell (cx,cy)) return false;
	return Matrix.Has_Flag (cx,cy,HYBRID_FLAG_BASE_CORE | HYBRID_FLAG_BASE_EXPANSION);
}


/***********************************************************************************************
 * Reservation geometry and the Phase 4 comparator                                             *
 *=============================================================================================*/
static float
Distance_Between(const Vector2 & a,const Vector2 & b)
{
	const float dx = a.X - b.X;
	const float dy = a.Y - b.Y;
	return WWMath::Sqrt ((dx * dx) + (dy * dy));
}


static float
Distance_To_Segment(const Vector2 & point,const Vector2 & a,const Vector2 & b)
{
	const float dx = b.X - a.X;
	const float dy = b.Y - a.Y;
	const float length_squared = (dx * dx) + (dy * dy);
	if (length_squared <= 0.0f) {
		return Distance_Between (point,a);
	}

	float t = (((point.X - a.X) * dx) + ((point.Y - a.Y) * dy)) / length_squared;
	t = Min2 (1.0f,Max2 (0.0f,t));
	const Vector2 closest (a.X + (dx * t),a.Y + (dy * t));
	return Distance_Between (point,closest);
}


//	Closest approach between two segments, sampled at their endpoints and projections.  Exact for the
//	disc-versus-disc and disc-versus-capsule cases Phase 4 actually has, and conservative (never
//	larger than the true distance) for capsule versus capsule, which is what an overlap test wants.
static float
Distance_Between_Segments(const Vector2 & a0,const Vector2 & a1,const Vector2 & b0,const Vector2 & b1)
{
	float nearest = Distance_To_Segment (a0,b0,b1);
	nearest = Min2 (nearest,Distance_To_Segment (a1,b0,b1));
	nearest = Min2 (nearest,Distance_To_Segment (b0,a0,a1));
	nearest = Min2 (nearest,Distance_To_Segment (b1,a0,a1));
	return nearest;
}


bool Hybrid_Reservation_Outranks(const HybridReservation & a,const HybridReservation & b)
{
	if (a.Priority != b.Priority) {
		return (int)a.Priority < (int)b.Priority;
	}
	return a.StableId < b.StableId;
}


bool Hybrid_Reservations_Overlap(const HybridReservation & a,const HybridReservation & b)
{
	return Distance_Between_Segments (a.A,a.B,b.A,b.B) < (a.Radius + b.Radius);
}


bool Hybrid_Reservations_Conflict(const HybridReservation & a,const HybridReservation & b)
{
	if (!a.Exclusive || !b.Exclusive) {
		return false;
	}

	//	A reservation never conflicts with one it owns or is owned by -- a corridor with the disc it
	//	serves, or a disc with its own corridor.
	if ((a.SiteId != 0) && ((a.SiteId == b.SiteId))) {
		return false;
	}

	return Hybrid_Reservations_Overlap (a,b);
}


/***********************************************************************************************
 * Matrix stamping helpers                                                                     *
 *                                                                                             *
 * A cell belongs to a region when its centre is inside it: one sample point per cell, so no     *
 * partial-coverage rule can drift.  The cells holding a capsule's own endpoints are always       *
 * stamped as well, because a region narrower than `S` -- a Tiberium field core at                *
 * `field_radius_min`, a base core on a small world -- would otherwise fall between samples and   *
 * vanish, and the phase gate reads exactly those centre cells.                                  *
 *=============================================================================================*/
static void
Note_Cells(HybridWorkCounters * counters,int cells)
{
	if (counters != nullptr) {
		counters->Add (HYBRID_WORK_SEMANTIC_CELLS,cells);
	}
}


struct HybridStampSpec
{
	HybridStampSpec(void) :
		SetFlags(0),ClearFlags(0),WaterIntent(-1),Reservation(0),ForceMiddleTier(false),
		TransportInfluence(false),ResourceInfluence(false),NoDecoration(false) { }

	unsigned int	SetFlags;
	unsigned int	ClearFlags;
	int				WaterIntent;			// -1 leave, 0 land, 1 water
	int				Reservation;			// HybridReservationPriority + 1, 0 leave
	bool				ForceMiddleTier;		// Section 8 Phase 4 step 4
	bool				TransportInfluence;	// write 1 - d/half, keeping the maximum
	bool				ResourceInfluence;
	bool				NoDecoration;
};


/*
**	Apply one stamp to one cell.  `distance` is the cell's distance to the stamped centreline, which
**	only the influence layers use.
*/
static void
Write_Cell(HybridSemanticMatrix & matrix,int index,float distance,float radius,const HybridStampSpec & spec,
			  signed char middle_tier)
{

	if (spec.SetFlags != 0)		matrix.Flags[index] |= spec.SetFlags;
	if (spec.ClearFlags != 0)	matrix.Flags[index] &= ~spec.ClearFlags;
	if (spec.WaterIntent >= 0)	matrix.LandWaterIntent[index] = (unsigned char)spec.WaterIntent;

	if (spec.Reservation > 0) {
		const unsigned char current = matrix.ReservationMask[index];
		if ((current == 0) || ((unsigned char)spec.Reservation < current)) {
			matrix.ReservationMask[index] = (unsigned char)spec.Reservation;
		}
	}

	if (spec.ForceMiddleTier) {
		matrix.BaseElevationTier[index]	= middle_tier;
		matrix.MacroHeightBias[index]		= 0.0f;
	}

	if (spec.TransportInfluence || spec.ResourceInfluence) {
		const float influence = Hybrid_Quantize_Normalized (1.0f - (distance / radius));
		if (spec.TransportInfluence && (influence > matrix.TransportInfluence[index])) {
			matrix.TransportInfluence[index] = influence;
		}
		if (spec.ResourceInfluence && (influence > matrix.ResourceInfluence[index])) {
			matrix.ResourceInfluence[index] = influence;
		}
	}

	if (spec.NoDecoration) {
		matrix.NoDecorationMask[index] = 1;
	}
}


/*
**	Stamp one capsule (`a == b` for a disc).  `ReservationMask` keeps the highest-priority
**	reservation that covers the cell -- the lowest ordinal -- and never erases an unrelated bit
**	layer, which is exactly the plan's rule for overlapping reservations.
*/
static void
Stamp_Capsule(HybridSemanticMatrix & matrix,const Vector2 & a,const Vector2 & b,float radius,
				  const HybridStampSpec & spec,signed char middle_tier,HybridWorkCounters * counters)
{
	if (radius <= 0.0f) {
		return;
	}

	const float cell = matrix.Get_Cell_Size ();
	const float min_x = Min2 (a.X,b.X) - radius;
	const float min_y = Min2 (a.Y,b.Y) - radius;
	const float max_x = Max2 (a.X,b.X) + radius;
	const float max_y = Max2 (a.Y,b.Y) + radius;

	int x0 = (int)WWMath::Floor ((min_x - matrix.Get_Origin_X ()) / cell);
	int y0 = (int)WWMath::Floor ((min_y - matrix.Get_Origin_Y ()) / cell);
	int x1 = (int)WWMath::Floor ((max_x - matrix.Get_Origin_X ()) / cell);
	int y1 = (int)WWMath::Floor ((max_y - matrix.Get_Origin_Y ()) / cell);

	if (x0 < 0) x0 = 0;
	if (y0 < 0) y0 = 0;
	if (x1 > matrix.Get_Count_X () - 1) x1 = matrix.Get_Count_X () - 1;
	if (y1 > matrix.Get_Count_Y () - 1) y1 = matrix.Get_Count_Y () - 1;

	int visited = 0;
	for (int cy = y0; cy <= y1; cy++) {
		for (int cx = x0; cx <= x1; cx++) {

			visited++;
			const Vector2 centre = matrix.Cell_Center (cx,cy);
			const float distance = Distance_To_Segment (centre,a,b);
			if (distance > radius) continue;

			Write_Cell (matrix,matrix.Index (cx,cy),distance,radius,spec,middle_tier);
		}
	}

	//	The endpoint cells, whether or not their centres fell inside.
	int cx = 0;
	int cy = 0;
	if (matrix.World_To_Cell (a,&cx,&cy)) {
		Write_Cell (matrix,matrix.Index (cx,cy),0.0f,radius,spec,middle_tier);
	}
	if (matrix.World_To_Cell (b,&cx,&cy)) {
		Write_Cell (matrix,matrix.Index (cx,cy),0.0f,radius,spec,middle_tier);
	}

	Note_Cells (counters,visited);
}


static void
Stamp_Disc(HybridSemanticMatrix & matrix,const Vector2 & centre,float radius,const HybridStampSpec & spec,
			  signed char middle_tier,HybridWorkCounters * counters)
{
	Stamp_Capsule (matrix,centre,centre,radius,spec,middle_tier,counters);
}


static void
Stamp_Polyline(HybridSemanticMatrix & matrix,const DynamicVectorClass<Vector2> & points,float radius,
					const HybridStampSpec & spec,signed char middle_tier,HybridWorkCounters * counters)
{
	if (points.Count () <= 0) {
		return;
	}
	if (points.Count () == 1) {
		Stamp_Disc (matrix,points[0],radius,spec,middle_tier,counters);
		return;
	}
	for (int i = 1; i < points.Count (); i++) {
		Stamp_Capsule (matrix,points[i - 1],points[i],radius,spec,middle_tier,counters);
	}
}


/***********************************************************************************************
 * Section 4.3 divider evaluation                                                              *
 *                                                                                             *
 * The four control `v` values are fixed at -1, -1/3, +1/3, +1, which are equally spaced, so the *
 * Bezier's own `v(t)` is exactly `2t - 1` and the inverse is exact: `t = (v+1)/2`.  That is why  *
 * the divider can be evaluated as "the `u` of the curve at this cell's `v`" with no root find.   *
 *=============================================================================================*/
static float
Divider_U_At_V(const HybridBiomeDivider & divider,float v)
{
	float t = (v + 1.0f) * 0.5f;
	t = Min2 (1.0f,Max2 (0.0f,t));

	const float it	= 1.0f - t;
	const float b0	= it * it * it;
	const float b1	= 3.0f * it * it * t;
	const float b2	= 3.0f * it * t * t;
	const float b3	= t * t * t;

	return (divider.ControlU[0] * b0) + (divider.ControlU[1] * b1) +
			 (divider.ControlU[2] * b2) + (divider.ControlU[3] * b3);
}


static float
Toward_Zero(float value,float step)
{
	if (value > step)		return value - step;
	if (value < -step)	return value + step;
	return 0.0f;
}


/***********************************************************************************************
 * Phase 4 helpers                                                                             *
 *=============================================================================================*/
static bool
Fail(HybridCoarseMatrixResult * result,HybridCoarseMatrixFailure failure,const char * detail)
{
	if (result != nullptr) {
		result->Failure	= failure;
		result->Detail		= (detail != nullptr) ? detail : "";
	}
	return false;
}


static bool
Is_Tiberium_Role(HybridSiteRole role)
{
	return (role == HYBRID_SITE_HOME_TIBERIUM_GDI) || (role == HYBRID_SITE_HOME_TIBERIUM_NOD) ||
			 (role == HYBRID_SITE_CONTESTED_TIBERIUM) || (role == HYBRID_SITE_REMOTE_TIBERIUM);
}


static bool
Is_Base_Role(HybridSiteRole role)
{
	return (role == HYBRID_SITE_GDI_BASE) || (role == HYBRID_SITE_NOD_BASE);
}


static signed char
Middle_Tier(int tier_count)
{
	const int tiers = (tier_count > 0) ? tier_count : 1;
	return (signed char)((tiers - 1) / 2);
}


static HybridStrategicGraphParams
Graph_Params_From(const HybridCoarseMatrixParams & params)
{
	HybridStrategicGraphParams out;
	out.MapPlanSeed	= params.MapPlanSeed;
	out.Constants		= params.Constants;
	out.BoundaryMargin= params.BoundaryMargin;
	out.FieldRadiusMax= params.FieldRadiusMax;
	out.RiverWidth		= params.RiverWidth;
	return out;
}


static unsigned int
Add_Reservation(HybridCoarseMatrices * out,HybridReservationPriority priority,const Vector2 & a,const Vector2 & b,
					 float radius,unsigned int site_id,unsigned int edge_id,unsigned int intent_id,int route_family,
					 bool exclusive)
{
	HybridReservation reservation;
	reservation.StableId			= (unsigned int)out->Reservations.Count () + 1;
	reservation.Priority			= priority;
	reservation.A.Set (Hybrid_Quantize_Position (a.X),Hybrid_Quantize_Position (a.Y));
	reservation.B.Set (Hybrid_Quantize_Position (b.X),Hybrid_Quantize_Position (b.Y));
	reservation.Radius			= Hybrid_Quantize_Position (radius);
	reservation.SiteId			= site_id;
	reservation.EdgeId			= edge_id;
	reservation.IntentStableId	= intent_id;
	reservation.RouteFamily		= route_family;
	reservation.Exclusive		= exclusive;
	out->Reservations.Add (reservation);
	return reservation.StableId;
}


/*
**	Section 6.9's acceptance rule for one `OPEN_MANEUVER` disc: at least 95% of its sampled area at
**	spacing `A` lies inside the placeable boundary.  Samples are taken on a world-axis lattice of
**	spacing `A` through the disc centre, so the sample set depends only on (centre, radius, A).
*/
struct HybridOpenManeuverContext
{
	float	Radius;
	float	SampleSpacing;
};


static bool
Open_Maneuver_Is_Acceptable(const Vector2 & centre,void * context)
{
	const HybridOpenManeuverContext * c = (const HybridOpenManeuverContext *)context;
	const float radius	= c->Radius;
	const float spacing	= (c->SampleSpacing > 0.0f) ? c->SampleSpacing : radius;

	int steps = (int)(radius / spacing);
	if (steps < 1) steps = 1;

	int sampled = 0;
	int placeable = 0;
	for (int gy = -steps; gy <= steps; gy++) {
		for (int gx = -steps; gx <= steps; gx++) {

			const Vector2 point (centre.X + ((float)gx * spacing),centre.Y + ((float)gy * spacing));
			if (Distance_Between (point,centre) > radius) continue;

			sampled++;
			if (WorldBoundsService::Can_Place_In_Region (point,0.0f)) placeable++;
		}
	}

	if (sampled <= 0) {
		return false;
	}
	return ((float)placeable / (float)sampled) >= OPEN_MANEUVER_MIN_PLACEABLE;
}


/*
**	Steps 4 to 8 as geometry: the reservation set, before anything is rasterized.  Creation order is
**	the plan's population order, which makes StableId order the population order too.
*/
static bool
Build_Reservations(const HybridCoarseMatrixParams & params,const HybridMapIdentity & identity,
						 const HybridArchetypeIntentSet & intents,HybridStrategicGraph * graph,
						 HybridCoarseMatrices * out,HybridCoarseMatrixResult * result)
{
	const HybridConstants & k = params.Constants;
	StringClass detail;

	//	Step 4 -- BASE_PROTECTED discs.
	for (int i = 0; i < graph->Sites.Count (); i++) {
		const HybridStrategicSite & site = graph->Sites[i];
		if (!Is_Base_Role (site.Role)) continue;
		Add_Reservation (out,HYBRID_RESERVE_BASE_PROTECTED,site.Position,site.Position,k.RBaseProtected,
							  site.StableId,0,0,-1,true);
	}

	//	Step 5 -- every Tiberium role, home first, as a disc of resource.FieldRadiusMax.
	for (int pass = 0; pass < 2; pass++) {
		for (int i = 0; i < graph->Sites.Count (); i++) {

			const HybridStrategicSite & site = graph->Sites[i];
			if (!Is_Tiberium_Role (site.Role)) continue;

			const bool home = (site.Role == HYBRID_SITE_HOME_TIBERIUM_GDI) || (site.Role == HYBRID_SITE_HOME_TIBERIUM_NOD);
			if (home != (pass == 0)) continue;

			Add_Reservation (out,home ? HYBRID_RESERVE_HOME_TIBERIUM : HYBRID_RESERVE_FIELD_TIBERIUM,
								  site.Position,site.Position,params.FieldRadiusMax,site.StableId,0,0,-1,true);
		}
	}

	/*
	**	Steps 6 and 7 -- corridors.  A corridor is layered intent, never an exclusive reservation: it
	**	is built to run through the base it leaves and the field it serves, and Section 2.4 already
	**	owns what a road crossing a reserve means on the ground.
	*/
	const float heavy_half		= Max2 (HEAVY_CORRIDOR_S * k.S,HEAVY_CORRIDOR_D * k.D);
	const float service_half	= Max2 (SERVICE_CORRIDOR_S * k.S,SERVICE_CORRIDOR_D * k.D);

	for (int step = 0; step < 2; step++) {
		for (int e = 0; e < graph->Edges.Count (); e++) {

			const HybridStrategicEdge & edge = graph->Edges[e];
			const bool heavy = (edge.RouteFamily >= 0);
			if (heavy != (step == 0)) continue;

			const HybridStrategicSite * a = graph->Find_Site (edge.SiteA);
			const HybridStrategicSite * b = graph->Find_Site (edge.SiteB);
			if ((a == nullptr) || (b == nullptr)) {
				detail.Format ("edge %u references a missing site",edge.StableId);
				return Fail (result,HYBRID_COARSE_INVALID_INPUT,detail.Peek_Buffer ());
			}

			Add_Reservation (out,heavy ? HYBRID_RESERVE_HEAVY_CORRIDOR : HYBRID_RESERVE_OPTIONAL_CORRIDOR,
								  a->Position,b->Position,heavy ? heavy_half : service_half,
								  0,edge.StableId,0,edge.RouteFamily,false);
		}
	}

	/*
	**	Step 8 -- the Section-6.9 `OPEN_MANEUVER` discs.  A disc that fails the 95% rule moves with
	**	the Section-6.1 repair search; a required disc that still cannot be placed fails the attempt.
	*/
	for (int i = 0; i < intents.Intents.Count (); i++) {

		const HybridArchetypeIntent & intent = intents.Intents[i];
		if (intent.Kind != HYBRID_INTENT_OPEN_MANEUVER) continue;

		const Vector2 guide = identity.Frame.To_World (intent.Guide);
		Vector2 centre (Hybrid_Quantize_Position (guide.X),Hybrid_Quantize_Position (guide.Y));

		HybridOpenManeuverContext context;
		context.Radius			= intent.Radius;
		context.SampleSpacing= k.A;

		if (!Open_Maneuver_Is_Acceptable (centre,&context)) {
			Vector2 repaired;
			if (!Hybrid_Repair_Ring_Search (identity.Frame,k.A,ROLE_REPAIR_LIMIT_D * k.D,centre,
													  Open_Maneuver_Is_Acceptable,&context,&repaired)) {
				detail.Format ("OPEN_MANEUVER intent %u",intent.StableId);
				return Fail (result,HYBRID_COARSE_OPEN_MANEUVER_UNPLACEABLE,detail.Peek_Buffer ());
			}
			centre = repaired;
			out->RoleMoves++;
		}

		Add_Reservation (out,HYBRID_RESERVE_OPEN_MANEUVER,centre,centre,intent.Radius,0,0,intent.StableId,-1,false);
	}

	return true;
}


/*
**	The Section-6.1 repair predicate Phase 4 uses for a moved role: the Phase-3 hard-validity rules
**	for that role class -- one owner for those rules -- plus "and the new position conflicts with no
**	higher-ranked reservation".
*/
struct HybridConflictRepairContext
{
	const HybridStrategicGraphParams *		GraphParams;
	const HybridMapIdentity *					Identity;
	const HybridArchetypeIntentSet *			Intents;
	const HybridStrategicGraph *				Graph;
	const HybridCoarseMatrices *				Matrices;
	const int *										Ranked;				// reservation indices in rank order
	int												RankedCount;
	int												RankPosition;		// how many of Ranked are already accepted
	HybridRoleClass								RoleClass;
	unsigned int									SiteId;
	unsigned int									ReservationId;
	float												Radius;
	HybridReservationPriority					Priority;
};


static bool
Conflict_Repair_Test(const Vector2 & candidate,void * context)
{
	const HybridConflictRepairContext * c = (const HybridConflictRepairContext *)context;

	if (!Hybrid_Role_Is_Hard_Valid (*c->GraphParams,*c->Identity,*c->Intents,*c->Graph,c->RoleClass,
											  candidate,false,c->SiteId)) {
		return false;
	}

	HybridReservation moved;
	moved.StableId		= c->ReservationId;
	moved.Priority		= c->Priority;
	moved.A				= candidate;
	moved.B				= candidate;
	moved.Radius		= c->Radius;
	moved.SiteId		= c->SiteId;
	moved.Exclusive	= true;

	for (int rank = 0; rank < c->RankPosition; rank++) {
		const HybridReservation & accepted = c->Matrices->Reservations[c->Ranked[rank]];
		if (Hybrid_Reservations_Conflict (accepted,moved)) return false;
	}
	return true;
}


/*
**	Hard-reservation conflicts.  Reservations are walked in the plan's priority order -- an explicit
**	stable order, by (priority, StableId) -- and a reservation that conflicts with an already
**	accepted, higher-ranked one is moved, never the one that was accepted first.  Moving means
**	moving the *role*: the repaired position is written back into the strategic graph, which stays
**	the one owner of role positions.
*/
static bool
Resolve_Conflicts(const HybridCoarseMatrixParams & params,const HybridMapIdentity & identity,
						const HybridArchetypeIntentSet & intents,HybridStrategicGraph * graph,
						HybridCoarseMatrices * out,HybridCoarseMatrixResult * result)
{
	const HybridConstants & k = params.Constants;
	const int count = out->Reservations.Count ();
	if (count <= 0) {
		return true;
	}

	//	Explicit stable sort of indices by (priority, StableId): insertion sort, which is stable.
	int * ranked = new int [count];
	for (int i = 0; i < count; i++) {
		ranked[i] = i;
	}
	for (int i = 1; i < count; i++) {
		const int key = ranked[i];
		int j = i - 1;
		while ((j >= 0) && Hybrid_Reservation_Outranks (out->Reservations[key],out->Reservations[ranked[j]])) {
			ranked[j + 1] = ranked[j];
			j--;
		}
		ranked[j + 1] = key;
	}

	const HybridStrategicGraphParams graph_params = Graph_Params_From (params);
	StringClass detail;
	bool ok = true;

	for (int rank = 0; ok && (rank < count); rank++) {

		HybridReservation & reservation = out->Reservations[ranked[rank]];
		if (!reservation.Exclusive) continue;

		int conflict_rank = -1;
		for (int earlier = 0; earlier < rank; earlier++) {
			if (Hybrid_Reservations_Conflict (out->Reservations[ranked[earlier]],reservation)) {
				conflict_rank = earlier;
				break;
			}
		}
		if (conflict_rank < 0) continue;

		//	Only a role-owned reserve can move; anything else is geometry this phase does not own.
		HybridStrategicSite * site = nullptr;
		for (int s = 0; s < graph->Sites.Count (); s++) {
			if (graph->Sites[s].StableId == reservation.SiteId) {
				site = &graph->Sites[s];
				break;
			}
		}
		if (site == nullptr) {
			detail.Format ("reservation %u (%s) conflicts with reservation %u and owns no role",
								reservation.StableId,Hybrid_Reservation_Priority_Name (reservation.Priority),
								out->Reservations[ranked[conflict_rank]].StableId);
			ok = Fail (result,HYBRID_COARSE_UNRESOLVED_CONFLICT,detail.Peek_Buffer ());
			break;
		}

		HybridConflictRepairContext context;
		context.GraphParams		= &graph_params;
		context.Identity			= &identity;
		context.Intents			= &intents;
		context.Graph				= graph;
		context.Matrices			= out;
		context.Ranked				= ranked;
		context.RankedCount		= count;
		context.RankPosition		= rank;
		context.RoleClass			= Is_Base_Role (site->Role) ? HYBRID_ROLE_BASE : HYBRID_ROLE_RESOURCE;
		context.SiteId				= site->StableId;
		context.ReservationId	= reservation.StableId;
		context.Radius				= reservation.Radius;
		context.Priority			= reservation.Priority;

		Vector2 repaired;
		if (!Hybrid_Repair_Ring_Search (identity.Frame,k.A,ROLE_REPAIR_LIMIT_D * k.D,site->Position,
												  Conflict_Repair_Test,&context,&repaired)) {
			detail.Format ("site %u (role %d) conflicts with reservation %u and has no legal repair",
								site->StableId,(int)site->Role,out->Reservations[ranked[conflict_rank]].StableId);
			ok = Fail (result,HYBRID_COARSE_UNRESOLVED_CONFLICT,detail.Peek_Buffer ());
			break;
		}

		site->Position		= repaired;
		reservation.A		= repaired;
		reservation.B		= repaired;
		out->RoleMoves++;

		//	Every corridor that ends at the moved role follows it; a corridor is derived geometry.
		for (int r = 0; r < count; r++) {
			HybridReservation & corridor = out->Reservations[r];
			if (corridor.EdgeId == 0) continue;
			const HybridStrategicEdge * edge = nullptr;
			for (int e = 0; e < graph->Edges.Count (); e++) {
				if (graph->Edges[e].StableId == corridor.EdgeId) {
					edge = &graph->Edges[e];
					break;
				}
			}
			if (edge == nullptr) continue;
			if (edge->SiteA == site->StableId) corridor.A = repaired;
			if (edge->SiteB == site->StableId) corridor.B = repaired;
		}
	}

	delete [] ranked;
	return ok;
}


/*
**	Steps 1 and 2 -- the playable/placeable classification from the canonical boundary owner, and
**	the middle-tier initialization.  `WorldBoundsService` answers "playable/placeable everywhere"
**	when no boundary is configured, which is the same degradation every other placement consumer
**	gets and what a self-check without a world sees.
*/
static void
Classify_Boundary(const HybridCoarseMatrixParams & params,HybridCoarseMatrices * out)
{
	HybridSemanticMatrix & matrix = out->Matrix;
	const signed char middle = Middle_Tier (params.TierCount);

	for (int cy = 0; cy < matrix.Get_Count_Y (); cy++) {
		for (int cx = 0; cx < matrix.Get_Count_X (); cx++) {

			const int index = matrix.Index (cx,cy);
			const Vector2 centre = matrix.Cell_Center (cx,cy);
			const WorldBoundsZoneType zone = WorldBoundsService::Classify_Point (centre);

			//	The tier layer carries the middle tier everywhere; on a non-playable cell it is only
			//	a default, because nothing outside the playable mask is terrain Phase 7 will build.
			matrix.BaseElevationTier[index]	= middle;
			matrix.MacroHeightBias[index]		= 0.0f;

			const bool playable = (zone != WORLD_ZONE_OUTSIDE_WORLD) && (zone != WORLD_ZONE_HARD_BOUNDARY);
			if (playable) {
				matrix.Flags[index] |= HYBRID_FLAG_LAND;
				out->PlayableCells++;
			}
			if (zone == WORLD_ZONE_CLIFF_BAND) {
				matrix.Flags[index] |= HYBRID_FLAG_CLIFF_BAND;
			}
			if (WorldBoundsService::Can_Place_In_Region (centre,0.0f)) {
				matrix.Flags[index] |= HYBRID_FLAG_PLACEABLE;
				out->PlaceableCells++;
			}
		}
	}

	Note_Cells (params.Counters,matrix.Get_Cell_Count ());
}


/*
**	Step 3 -- the archetype macro intent zones, straight from the frozen Section-6.10 records and in
**	their StableId order.  These are bands, not geometry: a band is one coarse cell wide wherever the
**	plan fixes only a centreline, because `S` is the finest thing this representation can express and
**	Phase 5/6 own the real cross-sections.
*/
static void
Stamp_Macro_Zones(const HybridCoarseMatrixParams & params,const HybridMapIdentity & identity,
						const HybridArchetypeIntentSet & intents,HybridCoarseMatrices * out)
{
	HybridSemanticMatrix & matrix = out->Matrix;
	const HybridConstants & k = params.Constants;
	const signed char middle = Middle_Tier (params.TierCount);
	const float band = k.S;

	for (int i = 0; i < intents.Intents.Count (); i++) {

		const HybridArchetypeIntent & intent = intents.Intents[i];

		switch (intent.Kind) {

			case HYBRID_INTENT_GUIDED_RIVER:
			case HYBRID_INTENT_COASTAL_GUIDED_RIVER:
			{
				//	The channel as water intent, at the width Phase 3 already reserved around it.
				HybridStampSpec spec;
				spec.WaterIntent = 1;
				const float half = Max2 (0.5f * params.RiverWidth,0.5f * k.S);
				Stamp_Polyline (matrix,intent.Points,half,spec,middle,params.Counters);
				break;
			}

			case HYBRID_INTENT_LAKE_GUIDE:
			{
				HybridStampSpec spec;
				spec.SetFlags		= HYBRID_FLAG_WATER_BASIN;
				spec.WaterIntent	= 1;
				Stamp_Disc (matrix,identity.Frame.To_World (intent.Guide),intent.Radius,spec,middle,params.Counters);
				break;
			}

			case HYBRID_INTENT_COAST_GUIDE:
			{
				/*
				**	Exterior sea: every cell at or beyond the sea boundary `v` on the chosen side is
				**	water and is not land, and the one `S`-wide strip inside it is the coast band.
				*/
				const float boundary_v	= intent.Guide.Y;
				const float sign			= (float)((intent.VSign >= 0) ? 1 : -1);

				for (int cy = 0; cy < matrix.Get_Count_Y (); cy++) {
					for (int cx = 0; cx < matrix.Get_Count_X (); cx++) {

						const int index = matrix.Index (cx,cy);
						const Vector2 uv = identity.Frame.To_Strategic (matrix.Cell_Center (cx,cy));
						const float inward = (boundary_v - uv.Y) * sign * identity.Frame.Scale;

						if (inward <= 0.0f) {
							matrix.LandWaterIntent[index]	= 1;
							matrix.Flags[index]				&= ~HYBRID_FLAG_LAND;
						} else if (inward <= band) {
							matrix.Flags[index] |= HYBRID_FLAG_COAST;
						}
					}
				}
				Note_Cells (params.Counters,matrix.Get_Cell_Count ());
				break;
			}

			case HYBRID_INTENT_PRINCIPAL_CANYON:
			case HYBRID_INTENT_PRINCIPAL_RIDGE:
			case HYBRID_INTENT_MAIN_RAVINE_GUIDE:
			{
				/*
				**	"Approximately along strategic v": the band runs the full playable `v` span at the
				**	intent's own `u`.  Phase 5 draws the actual centreline inside it.
				*/
				HybridStampSpec spec;
				spec.SetFlags = (intent.Kind == HYBRID_INTENT_PRINCIPAL_CANYON) ? HYBRID_FLAG_CANYON_BAND :
									 ((intent.Kind == HYBRID_INTENT_PRINCIPAL_RIDGE) ? HYBRID_FLAG_RIDGE : HYBRID_FLAG_RAVINE_BAND);

				const Vector2 a = identity.Frame.To_World (Vector2 (intent.Guide.X,-1.0f));
				const Vector2 b = identity.Frame.To_World (Vector2 (intent.Guide.X,+1.0f));
				Stamp_Capsule (matrix,a,b,band,spec,middle,params.Counters);
				break;
			}

			case HYBRID_INTENT_BASIN_GUIDE:
			{
				//	The basin floor, plus its rim as the one coarse ring just outside it.
				const Vector2 centre = identity.Frame.To_World (intent.Guide);

				HybridStampSpec rim;
				rim.SetFlags = HYBRID_FLAG_PLATEAU;
				Stamp_Disc (matrix,centre,intent.Radius + band,rim,middle,params.Counters);

				HybridStampSpec floor_spec;
				floor_spec.SetFlags		= HYBRID_FLAG_BASIN;
				floor_spec.ClearFlags	= HYBRID_FLAG_PLATEAU;
				Stamp_Disc (matrix,centre,intent.Radius,floor_spec,middle,params.Counters);
				break;
			}

			default:
				//	SURFACE_SADDLE, PENINSULA_NECK, BASIN_ENTRY and the crossing guides carry no macro
				//	band: they are point intents whose geometry belongs to a later phase.
				break;
		}
	}
}


/*
**	Steps 4 to 8 -- rasterize the resolved reservations, in the plan's population order.
*/
static void
Stamp_Reservations(const HybridCoarseMatrixParams & params,HybridCoarseMatrices * out)
{
	HybridSemanticMatrix & matrix = out->Matrix;
	const HybridConstants & k = params.Constants;
	const signed char middle = Middle_Tier (params.TierCount);

	static const HybridReservationPriority ORDER [5] =
	{
		HYBRID_RESERVE_BASE_PROTECTED,
		HYBRID_RESERVE_HOME_TIBERIUM,
		HYBRID_RESERVE_FIELD_TIBERIUM,
		HYBRID_RESERVE_HEAVY_CORRIDOR,
		HYBRID_RESERVE_OPTIONAL_CORRIDOR,
	};

	for (int step = 0; step < 5; step++) {
		for (int r = 0; r < out->Reservations.Count (); r++) {

			const HybridReservation & reservation = out->Reservations[r];
			if (reservation.Priority != ORDER[step]) continue;

			switch (reservation.Priority) {

				case HYBRID_RESERVE_BASE_PROTECTED:
				{
					//	The protected disc, then its inner R_BASE core carved out of the expansion ring.
					HybridStampSpec protected_spec;
					protected_spec.SetFlags			= HYBRID_FLAG_BASE_EXPANSION;
					protected_spec.Reservation		= (int)HYBRID_RESERVE_BASE_PROTECTED + 1;
					protected_spec.ForceMiddleTier	= true;
					Stamp_Disc (matrix,reservation.A,reservation.Radius,protected_spec,middle,params.Counters);

					HybridStampSpec core;
					core.SetFlags			= HYBRID_FLAG_BASE_CORE;
					core.ClearFlags		= HYBRID_FLAG_BASE_EXPANSION;
					core.Reservation		= (int)HYBRID_RESERVE_BASE_PROTECTED + 1;
					core.ForceMiddleTier	= true;
					Stamp_Disc (matrix,reservation.A,k.RBase,core,middle,params.Counters);
					break;
				}

				case HYBRID_RESERVE_HOME_TIBERIUM:
				case HYBRID_RESERVE_FIELD_TIBERIUM:
				{
					HybridStampSpec spec;
					spec.SetFlags				= HYBRID_FLAG_TIBERIUM_RESERVE;
					spec.Reservation			= (int)reservation.Priority + 1;
					spec.ResourceInfluence	= true;
					Stamp_Disc (matrix,reservation.A,reservation.Radius,spec,middle,params.Counters);
					break;
				}

				case HYBRID_RESERVE_HEAVY_CORRIDOR:
				case HYBRID_RESERVE_OPTIONAL_CORRIDOR:
				{
					HybridStampSpec spec;
					spec.SetFlags				= (reservation.Priority == HYBRID_RESERVE_HEAVY_CORRIDOR) ?
														HYBRID_FLAG_MAJOR_TRANSPORT_CORRIDOR : HYBRID_FLAG_SECONDARY_TRANSPORT_CORRIDOR;
					spec.Reservation			= (int)reservation.Priority + 1;
					spec.TransportInfluence	= true;
					Stamp_Capsule (matrix,reservation.A,reservation.B,reservation.Radius,spec,middle,params.Counters);
					break;
				}

				default:
					break;
			}
		}
	}

	//	Step 8 -- the dedicated Section-6.9 no-immutable-barrier layer.
	for (int r = 0; r < out->Reservations.Count (); r++) {
		const HybridReservation & reservation = out->Reservations[r];
		if (reservation.Priority != HYBRID_RESERVE_OPEN_MANEUVER) continue;

		HybridStampSpec spec;
		spec.SetFlags		= HYBRID_FLAG_NO_IMMUTABLE_BARRIER;
		spec.Reservation	= (int)HYBRID_RESERVE_OPEN_MANEUVER + 1;
		Stamp_Disc (matrix,reservation.A,reservation.Radius,spec,middle,params.Counters);
	}

}


/*
**	Step 9 -- biome ownership.  A single-biome map is one assignment; a mixed map rasterizes the
**	Section-4.3 divider, and Phase 4 owns the 35% ownership rule, the at-most-six 0.05 shifts toward
**	zero and the straight `u = 0` fallback.  Primary owns the GDI side of the divider.
*/
static void
Stamp_Biomes(const HybridCoarseMatrixParams & params,const HybridMapIdentity & identity,
				 const HybridArchetypeIntentSet & intents,HybridCoarseMatrices * out)
{
	HybridSemanticMatrix & matrix = out->Matrix;
	const HybridConstants & k = params.Constants;
	const int cells = matrix.Get_Cell_Count ();

	const unsigned char primary		= (unsigned char)identity.PrimaryBiome;
	const unsigned char secondary	= (unsigned char)((identity.SecondaryBiome < HYBRID_BIOME_COUNT) ?
																	identity.SecondaryBiome : identity.PrimaryBiome);

	const bool mixed = (identity.BiomeMode == HYBRID_BIOME_MODE_MIXED) &&
							 (identity.SecondaryBiome < HYBRID_BIOME_COUNT) && intents.Divider.Present;

	if (!mixed) {
		for (int index = 0; index < cells; index++) {
			matrix.BiomeId[index] = primary;
		}
		out->Divider					= HybridBiomeDivider ();
		out->DividerShifts			= 0;
		out->PrimaryLandFraction	= (out->PlayableCells > 0) ? 1.0f : 0.0f;
		out->SecondaryLandFraction	= 0.0f;
		Note_Cells (params.Counters,cells);
		return;
	}

	HybridBiomeDivider divider = intents.Divider;
	divider.TransitionWidth = Hybrid_Quantize_Position (Max2 (BIOME_TRANSITION_S * k.S,BIOME_TRANSITION_W * k.W));

	int shifts = 0;
	float primary_fraction = 0.0f;
	float secondary_fraction = 0.0f;

	for (;;) {

		int primary_cells = 0;
		int land_cells = 0;

		for (int cy = 0; cy < matrix.Get_Count_Y (); cy++) {
			for (int cx = 0; cx < matrix.Get_Count_X (); cx++) {

				const int index = matrix.Index (cx,cy);
				const Vector2 uv = identity.Frame.To_Strategic (matrix.Cell_Center (cx,cy));
				const float boundary = Hybrid_Quantize_Normalized (Divider_U_At_V (divider,uv.Y));
				const bool own_primary = Hybrid_Quantize_Normalized (uv.X) < boundary;

				matrix.BiomeId[index] = own_primary ? primary : secondary;

				//	Only playable land counts toward the 35% rule; sea is not either biome's land.
				if (((matrix.Flags[index] & HYBRID_FLAG_LAND) != 0) && (matrix.LandWaterIntent[index] == 0)) {
					land_cells++;
					if (own_primary) primary_cells++;
				}
			}
		}
		Note_Cells (params.Counters,cells);

		primary_fraction		= (land_cells > 0) ? Hybrid_Quantize_Normalized ((float)primary_cells / (float)land_cells) : 0.0f;
		secondary_fraction	= (land_cells > 0) ? Hybrid_Quantize_Normalized (1.0f - primary_fraction) : 0.0f;

		const bool satisfied = (primary_fraction >= BIOME_MIN_OWNERSHIP) && (secondary_fraction >= BIOME_MIN_OWNERSHIP);
		if (satisfied || (shifts >= BIOME_MAX_SHIFTS)) {
			if (!satisfied) {
				//	Section 4.3 step 8: the straight line, assigned once and accepted as it lands.
				for (int c = 0; c < 4; c++) {
					divider.ControlU[c] = 0.0f;
				}
				shifts = -1;

				int straight_primary = 0;
				int straight_land = 0;
				for (int cy = 0; cy < matrix.Get_Count_Y (); cy++) {
					for (int cx = 0; cx < matrix.Get_Count_X (); cx++) {

						const int index = matrix.Index (cx,cy);
						const Vector2 uv = identity.Frame.To_Strategic (matrix.Cell_Center (cx,cy));
						const bool own_primary = Hybrid_Quantize_Normalized (uv.X) < 0.0f;
						matrix.BiomeId[index] = own_primary ? primary : secondary;

						if (((matrix.Flags[index] & HYBRID_FLAG_LAND) != 0) && (matrix.LandWaterIntent[index] == 0)) {
							straight_land++;
							if (own_primary) straight_primary++;
						}
					}
				}
				Note_Cells (params.Counters,cells);
				primary_fraction		= (straight_land > 0) ? Hybrid_Quantize_Normalized ((float)straight_primary / (float)straight_land) : 0.0f;
				secondary_fraction	= (straight_land > 0) ? Hybrid_Quantize_Normalized (1.0f - primary_fraction) : 0.0f;
			}
			break;
		}

		for (int c = 0; c < 4; c++) {
			divider.ControlU[c] = Hybrid_Quantize_Normalized (Toward_Zero (divider.ControlU[c],BIOME_SHIFT_STEP));
		}
		shifts++;
	}

	out->Divider					= divider;
	out->DividerShifts			= shifts;
	out->PrimaryLandFraction	= primary_fraction;
	out->SecondaryLandFraction	= secondary_fraction;
}


/*
**	Step 10 -- NO_DECORATION over every BASE_PROTECTED disc and every provisional Tiberium field
**	core.  The core is `resource.field_radius_min`: the part of the reserve a field is guaranteed to
**	fill, which is what "provisional field core" means before Phase 13 draws the real polygon.
*/
static void
Stamp_No_Decoration(const HybridCoarseMatrixParams & params,HybridCoarseMatrices * out)
{
	HybridSemanticMatrix & matrix = out->Matrix;
	const signed char middle = Middle_Tier (params.TierCount);

	for (int r = 0; r < out->Reservations.Count (); r++) {

		const HybridReservation & reservation = out->Reservations[r];
		float radius = 0.0f;

		if (reservation.Priority == HYBRID_RESERVE_BASE_PROTECTED) {
			radius = reservation.Radius;
		} else if ((reservation.Priority == HYBRID_RESERVE_HOME_TIBERIUM) ||
					  (reservation.Priority == HYBRID_RESERVE_FIELD_TIBERIUM)) {
			radius = Min2 (params.FieldRadiusMin,reservation.Radius);
		} else {
			continue;
		}

		HybridStampSpec spec;
		spec.SetFlags		= HYBRID_FLAG_NO_DECORATION;
		spec.NoDecoration	= true;
		Stamp_Disc (matrix,reservation.A,radius,spec,middle,params.Counters);
	}
}


/*
**	The Phase 4 gate.  Every required role centre is inside its own legal reserved region, no
**	BASE_PROTECTED disc overlaps water or a cliff/canyon/ravine/coast hard-intent band, and no hard
**	reservation conflict survived.
**
**	FRONT_CENTER is deliberately not in the first test: Section 6.1 creates it as an analysis node
**	with reserve radius 0, so it owns no reserved region to be inside of.
*/
static bool
Check_Gate(const HybridCoarseMatrixParams & params,HybridCoarseMatrices * out,const HybridStrategicGraph & graph,
			  HybridCoarseMatrixResult * result)
{
	HybridSemanticMatrix & matrix = out->Matrix;
	StringClass detail;

	for (int i = 0; i < graph.Sites.Count (); i++) {

		const HybridStrategicSite & site = graph.Sites[i];
		unsigned int required = 0;
		bool require_land = true;

		if (Is_Base_Role (site.Role)) {
			required = HYBRID_FLAG_BASE_CORE;
		} else if (Is_Tiberium_Role (site.Role)) {
			required = HYBRID_FLAG_TIBERIUM_RESERVE;
		} else if (site.Role == HYBRID_SITE_ROUTE_WAYPOINT) {
			//	A waypoint may legally sit on the obstruction its family must cross, so its cell is
			//	tested for the corridor that owns it and for nothing else.
			required		= HYBRID_FLAG_MAJOR_TRANSPORT_CORRIDOR;
			require_land = false;
		} else {
			continue;
		}

		int cx = 0;
		int cy = 0;
		if (!matrix.World_To_Cell (site.Position,&cx,&cy)) {
			detail.Format ("site %u (role %d) is outside the matrix",site.StableId,(int)site.Role);
			return Fail (result,HYBRID_COARSE_ROLE_OUTSIDE_RESERVATION,detail.Peek_Buffer ());
		}

		const unsigned int flags = matrix.Flags[matrix.Index (cx,cy)];
		if ((flags & required) == 0) {
			detail.Format ("site %u (role %d) centre cell lacks flag 0x%08X",site.StableId,(int)site.Role,required);
			return Fail (result,HYBRID_COARSE_ROLE_OUTSIDE_RESERVATION,detail.Peek_Buffer ());
		}
		if (require_land && ((flags & (HYBRID_FLAG_LAND | HYBRID_FLAG_PLACEABLE)) !=
									(HYBRID_FLAG_LAND | HYBRID_FLAG_PLACEABLE))) {
			detail.Format ("site %u (role %d) centre cell is not playable/placeable",site.StableId,(int)site.Role);
			return Fail (result,HYBRID_COARSE_ROLE_OUTSIDE_RESERVATION,detail.Peek_Buffer ());
		}
	}

	const unsigned int hard_intent = HYBRID_FLAG_CLIFF_BAND | HYBRID_FLAG_CANYON_BAND |
												HYBRID_FLAG_RAVINE_BAND | HYBRID_FLAG_COAST;

	for (int cy = 0; cy < matrix.Get_Count_Y (); cy++) {
		for (int cx = 0; cx < matrix.Get_Count_X (); cx++) {

			if (!out->Is_Base_Protected (cx,cy)) continue;
			const int index = matrix.Index (cx,cy);

			if (matrix.LandWaterIntent[index] != 0) {
				detail.Format ("BASE_PROTECTED cell (%d,%d) carries water intent",cx,cy);
				return Fail (result,HYBRID_COARSE_BASE_ON_HARD_INTENT,detail.Peek_Buffer ());
			}
			if ((matrix.Flags[index] & hard_intent) != 0) {
				detail.Format ("BASE_PROTECTED cell (%d,%d) carries hard intent 0x%08X",cx,cy,
									matrix.Flags[index] & hard_intent);
				return Fail (result,HYBRID_COARSE_BASE_ON_HARD_INTENT,detail.Peek_Buffer ());
			}
		}
	}
	Note_Cells (params.Counters,matrix.Get_Cell_Count ());

	for (int a = 0; a < out->Reservations.Count (); a++) {
		for (int b = a + 1; b < out->Reservations.Count (); b++) {
			if (Hybrid_Reservations_Conflict (out->Reservations[a],out->Reservations[b])) {
				detail.Format ("reservations %u and %u still conflict",out->Reservations[a].StableId,
									out->Reservations[b].StableId);
				return Fail (result,HYBRID_COARSE_UNRESOLVED_CONFLICT,detail.Peek_Buffer ());
			}
		}
	}

	return true;
}


/***********************************************************************************************
 * Hybrid_Build_Coarse_Matrices -- runtime Phase 4                                             *
 *=============================================================================================*/
bool Hybrid_Build_Coarse_Matrices(const HybridCoarseMatrixParams & params,const HybridMapIdentity & identity,
											 const HybridArchetypeIntentSet & intents,HybridStrategicGraph * graph,
											 HybridCoarseMatrices * out,HybridCoarseMatrixResult * result)
{
	if ((out == nullptr) || (graph == nullptr)) {
		return Fail (result,HYBRID_COARSE_INVALID_INPUT,"no output or no strategic graph");
	}

	out->Reset ();
	if (result != nullptr) {
		result->Failure = HYBRID_COARSE_OK;
		result->Detail = "";
	}

	const HybridConstants & k = params.Constants;
	if ((k.S <= 0.0f) || (params.WorldWidth <= 0.0f) || (params.WorldHeight <= 0.0f)) {
		return Fail (result,HYBRID_COARSE_INVALID_INPUT,"degenerate spacing or world rectangle");
	}

	//	Dimensions are ceil(extent/S) -- computed as an exact integer step so no rounding mode enters.
	int count_x = (int)(params.WorldWidth / k.S);
	int count_y = (int)(params.WorldHeight / k.S);
	if (((float)count_x * k.S) < params.WorldWidth) count_x++;
	if (((float)count_y * k.S) < params.WorldHeight) count_y++;

	if (!out->Matrix.Resize (count_x,count_y,k.S,params.WorldMinX,params.WorldMinY,params.Arena)) {
		StringClass detail;
		detail.Format ("%d x %d cells at S=%.3f",count_x,count_y,k.S);
		return Fail (result,HYBRID_COARSE_SCRATCH_EXHAUSTED,detail.Peek_Buffer ());
	}

	if (params.Counters != nullptr) {
		const int bytes = (params.Arena != nullptr) ? params.Arena->Get_High_Water () : out->Matrix.Get_Layer_Bytes ();
		params.Counters->Note_Peak (HYBRID_WORK_PEAK_SCRATCH_BYTES,bytes);
	}

	//	Reservations and their conflicts first; the matrix then describes positions nothing will move.
	if (!Build_Reservations (params,identity,intents,graph,out,result)) {
		out->Matrix.Clear ();
		return false;
	}
	if (!Resolve_Conflicts (params,identity,intents,graph,out,result)) {
		out->Matrix.Clear ();
		return false;
	}
	if (out->RoleMoves > 0) {
		//	The graph's checksum always digests the graph's current content; Phase 3 checked its own
		//	gate against it before this phase ran.
		graph->Checksum = Hybrid_Strategic_Graph_Checksum (*graph,0);
	}

	Classify_Boundary (params,out);
	Stamp_Macro_Zones (params,identity,intents,out);
	Stamp_Reservations (params,out);
	Stamp_Biomes (params,identity,intents,out);
	Stamp_No_Decoration (params,out);

	if (!Check_Gate (params,out,*graph,result)) {
		return false;
	}

	out->Checksum = Hybrid_Coarse_Matrix_Checksum (*out,0);

	WWDEBUG_SAY (("HybridCoarseMatrix: %dx%d cells at S=%.2f playable=%d placeable=%d reservations=%d "
					  "moves=%d biome_split=%.3f/%.3f shifts=%d checksum=%08X\n",
					  out->Matrix.Get_Count_X (),out->Matrix.Get_Count_Y (),k.S,out->PlayableCells,out->PlaceableCells,
					  out->Reservations.Count (),out->RoleMoves,out->PrimaryLandFraction,out->SecondaryLandFraction,
					  out->DividerShifts,out->Checksum));
	return true;
}


unsigned int Hybrid_Coarse_Matrix_Checksum(const HybridCoarseMatrices & matrices,unsigned int seed)
{
	HybridHashStream stream (seed);
	const HybridSemanticMatrix & matrix = matrices.Matrix;

	stream.Add_Int (matrix.Get_Count_X ());
	stream.Add_Int (matrix.Get_Count_Y ());
	stream.Add_Length (matrix.Get_Cell_Size ());
	stream.Add_Length (matrix.Get_Origin_X ());
	stream.Add_Length (matrix.Get_Origin_Y ());

	const int cells = matrix.Get_Cell_Count ();
	for (int index = 0; index < cells; index++) {
		stream.Add_Int ((int)matrix.BaseElevationTier[index]);
		stream.Add_Length (matrix.MacroHeightBias[index]);
		stream.Add_Uint (matrix.LandWaterIntent[index]);
		stream.Add_Uint (matrix.LandformMask[index]);
		stream.Add_Uint (matrix.ReservationMask[index]);
		stream.Add_Scalar (matrix.TransportInfluence[index]);
		stream.Add_Scalar (matrix.ResourceInfluence[index]);
		stream.Add_Uint (matrix.BiomeId[index]);
		stream.Add_Uint (matrix.RoughnessClass[index]);
		stream.Add_Uint (matrix.NoDecorationMask[index]);
		stream.Add_Uint (matrix.Flags[index]);
	}

	for (int r = 0; r < matrices.Reservations.Count (); r++) {
		const HybridReservation & reservation = matrices.Reservations[r];
		stream.Add_Uint (reservation.StableId);
		stream.Add_Uint ((unsigned int)reservation.Priority);
		stream.Add_Length (reservation.A.X);
		stream.Add_Length (reservation.A.Y);
		stream.Add_Length (reservation.B.X);
		stream.Add_Length (reservation.B.Y);
		stream.Add_Length (reservation.Radius);
		stream.Add_Uint (reservation.SiteId);
		stream.Add_Uint (reservation.EdgeId);
		stream.Add_Uint (reservation.IntentStableId);
		stream.Add_Int (reservation.RouteFamily);
		stream.Add_Bool (reservation.Exclusive);
	}

	stream.Add_Bool (matrices.Divider.Present);
	for (int c = 0; c < 4; c++) {
		stream.Add_Scalar (matrices.Divider.ControlU[c]);
		stream.Add_Scalar (matrices.Divider.ControlV[c]);
	}
	stream.Add_Length (matrices.Divider.TransitionWidth);
	stream.Add_Int (matrices.DividerShifts);
	stream.Add_Scalar (matrices.PrimaryLandFraction);
	stream.Add_Scalar (matrices.SecondaryLandFraction);
	stream.Add_Int (matrices.PlayableCells);
	stream.Add_Int (matrices.PlaceableCells);
	stream.Add_Int (matrices.RoleMoves);

	return stream.Get ();
}
