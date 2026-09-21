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
 *                 Project Name : WWPhys                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwphys/worldboundsservice.cpp                $*
 *                                                                                             *
 *	V2 roadmap Section 34 Phase 2.  See worldboundsservice.h for the design and the layering     *
 *	reasoning.                                                                                   *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "worldboundsservice.h"

#include "wwdebug.h"
#include "wwmath.h"

#include <cmath>


const float	WorldBoundsService::MINIMUM_VERTEX_SPACING	= 0.1f;
const float	WorldBoundsService::MINIMUM_INTERIOR_AREA		= 1.0f;


static bool										_Configured			= false;
static bool										_ServerAuthoritative	= false;
static int										_ViolationCount		= 0;

static DynamicVectorClass<Vector2>		_Vertices;			// closed ring, clockwise, closing edge implied

static float									_CliffBandWidth		= 0.0f;
static float									_ApproachBandWidth	= 0.0f;
static float									_HardBarrierOffset	= 0.0f;
static float									_AirCeiling				= 0.0f;

//	Cached axis-aligned bounds of the ring.  Pure fast-reject: a point outside this box cannot
//	be inside the polygon, which is the cheap answer for the one case that matters for
//	performance (an actor that has genuinely escaped) without changing any result.
static Vector2									_BoundsMinimum (0.0f,0.0f);
static Vector2									_BoundsMaximum (0.0f,0.0f);


/***********************************************************************************************
 * Distance_To_Segment -- shortest distance from a point to a line segment                       *
 *=============================================================================================*/
static float
Distance_To_Segment(const Vector2 & point,const Vector2 & a,const Vector2 & b)
{
	Vector2 ab = b - a;
	float length2 = ab.Length2 ();
	if (length2 <= 0.0f) {
		return (point - a).Length ();
	}

	float t = ((point.X - a.X) * ab.X + (point.Y - a.Y) * ab.Y) / length2;
	if (t < 0.0f) t = 0.0f;
	if (t > 1.0f) t = 1.0f;

	Vector2 closest (a.X + ab.X * t,a.Y + ab.Y * t);
	return (point - closest).Length ();
}


/***********************************************************************************************
 * Point_In_Polygon -- standard crossing-number test                                             *
 *                                                                                             *
 *	Winding-independent, so it answers the same whichever way Configure normalized the ring.     *
 *=============================================================================================*/
static bool
Point_In_Polygon(const Vector2 & point)
{
	bool inside = false;
	int count = _Vertices.Count ();

	for (int i = 0, j = count - 1; i < count; j = i++) {
		const Vector2 & vi = _Vertices [i];
		const Vector2 & vj = _Vertices [j];

		if (((vi.Y > point.Y) != (vj.Y > point.Y)) &&
			 (point.X < (vj.X - vi.X) * (point.Y - vi.Y) / (vj.Y - vi.Y) + vi.X)) {
			inside = !inside;
		}
	}

	return inside;
}


/***********************************************************************************************
 * Signed_Area -- shoelace.  Positive is counter-clockwise in this codebase's handedness.        *
 *=============================================================================================*/
static float
Signed_Area(const DynamicVectorClass<Vector2> & vertices)
{
	float sum = 0.0f;
	int count = vertices.Count ();

	for (int i = 0, j = count - 1; i < count; j = i++) {
		sum += (vertices [j].X * vertices [i].Y) - (vertices [i].X * vertices [j].Y);
	}

	return sum * 0.5f;
}


/***********************************************************************************************
 * Segments_Properly_Intersect -- do two open segments cross, ignoring shared endpoints          *
 *                                                                                             *
 *	Adjacent ring edges legitimately share a vertex, so a test that counted that as an           *
 *	intersection would reject every valid polygon.  Only a genuine crossing counts.              *
 *=============================================================================================*/
static float
Cross_2D(const Vector2 & o,const Vector2 & a,const Vector2 & b)
{
	return (a.X - o.X) * (b.Y - o.Y) - (a.Y - o.Y) * (b.X - o.X);
}

static bool
Segments_Properly_Intersect(const Vector2 & p1,const Vector2 & p2,const Vector2 & q1,const Vector2 & q2)
{
	float d1 = Cross_2D (q1,q2,p1);
	float d2 = Cross_2D (q1,q2,p2);
	float d3 = Cross_2D (p1,p2,q1);
	float d4 = Cross_2D (p1,p2,q2);

	//	Strict signs on both sides: a touch (a zero) is not a proper crossing, which is what
	//	lets adjacent edges share their endpoint without failing validation.
	if (((d1 > 0.0f && d2 < 0.0f) || (d1 < 0.0f && d2 > 0.0f)) &&
		 ((d3 > 0.0f && d4 < 0.0f) || (d3 < 0.0f && d4 > 0.0f))) {
		return true;
	}

	return false;
}


void WorldBoundsService::Init(void)
{
	Clear ();
	_ServerAuthoritative	= false;
	_ViolationCount		= 0;
}


void WorldBoundsService::Shutdown(void)
{
	Clear ();
}


void WorldBoundsService::Clear(void)
{
	_Configured				= false;
	_Vertices.Delete_All ();
	_CliffBandWidth		= 0.0f;
	_ApproachBandWidth	= 0.0f;
	_HardBarrierOffset	= 0.0f;
	_AirCeiling				= 0.0f;
	_BoundsMinimum.Set (0.0f,0.0f);
	_BoundsMaximum.Set (0.0f,0.0f);
}


bool WorldBoundsService::Is_Configured(void)			{ return _Configured; }
float WorldBoundsService::Get_Air_Ceiling(void)		{ return _AirCeiling; }
int WorldBoundsService::Get_Violation_Count(void)	{ return _ViolationCount; }

void WorldBoundsService::Set_Server_Authoritative(bool is_authoritative)
{
	_ServerAuthoritative = is_authoritative;
}

bool WorldBoundsService::Is_Server_Authoritative(void)	{ return _ServerAuthoritative; }


/***********************************************************************************************
 * WorldBoundsService::Configure -- install and validate the authoritative boundary              *
 *                                                                                             *
 *	Phase 2 step 3's four named checks, in the order that gives the most useful diagnostic:      *
 *	shape first (enough vertices at all), then spacing, then self-intersection, then usable      *
 *	area.  Nothing is installed unless every one of them passes -- a half-applied boundary is    *
 *	worse than none, because the rest of the engine would start trusting it.                     *
 *=============================================================================================*/
bool WorldBoundsService::Configure(const DynamicVectorClass<Vector2> & vertices,
											  float cliff_band_width,float hard_barrier_offset,
											  float air_ceiling,StringClass * error_out)
{
	#define FAIL(msg) { if (error_out != nullptr) *error_out = msg; return false; }

	int count = vertices.Count ();
	if (count < 3)												FAIL ("boundary needs at least 3 vertices");

	if (!std::isfinite (cliff_band_width) || (cliff_band_width <= 0.0f))
																	FAIL ("cliff_band_width must be positive and finite");
	if (!std::isfinite (hard_barrier_offset) || (hard_barrier_offset < 0.0f))
																	FAIL ("hard_barrier_offset must be nonnegative and finite");
	if (!std::isfinite (air_ceiling))					FAIL ("air_ceiling must be finite");

	for (int i = 0; i < count; i++) {
		if (!std::isfinite (vertices [i].X) || !std::isfinite (vertices [i].Y)) {
			FAIL ("boundary contains a non-finite vertex");
		}
	}

	//	Minimum vertex spacing -- including the implied closing edge, which is exactly the one a
	//	caller that accidentally duplicated its first vertex at the end would fail on.
	for (int i = 0, j = count - 1; i < count; j = i++) {
		if ((vertices [i] - vertices [j]).Length () < MINIMUM_VERTEX_SPACING) {
			FAIL ("boundary has vertices closer than the minimum spacing");
		}
	}

	//	Non-self-intersecting.  O(n^2) over a ring of tens of vertices, run once when a world is
	//	created -- not a hot path, and the straightforward version is the one that is obviously
	//	correct.
	for (int i = 0; i < count; i++) {
		const Vector2 & p1 = vertices [i];
		const Vector2 & p2 = vertices [(i + 1) % count];

		for (int j = i + 1; j < count; j++) {
			const Vector2 & q1 = vertices [j];
			const Vector2 & q2 = vertices [(j + 1) % count];

			if (Segments_Properly_Intersect (p1,p2,q1,q2)) {
				FAIL ("boundary polygon self-intersects");
			}
		}
	}

	float area = Signed_Area (vertices);
	if (WWMath::Fabs (area) < MINIMUM_INTERIOR_AREA)	FAIL ("boundary encloses no usable interior area");

	//	Install.  Winding is normalized to clockwise (negative shoelace area in this
	//	codebase's handedness) so every consumer downstream -- Phase 3's cliff mesh in
	//	particular, which needs a consistent outward normal per edge -- can rely on the
	//	direction without re-deriving it.
	_Vertices.Delete_All ();
	if (area > 0.0f) {
		for (int i = count - 1; i >= 0; i--) {
			_Vertices.Add (vertices [i]);
		}
	} else {
		for (int i = 0; i < count; i++) {
			_Vertices.Add (vertices [i]);
		}
	}

	_CliffBandWidth		= cliff_band_width;
	_ApproachBandWidth	= cliff_band_width;		// derived; see the header's note
	_HardBarrierOffset	= hard_barrier_offset;
	_AirCeiling				= air_ceiling;

	_BoundsMinimum = _Vertices [0];
	_BoundsMaximum = _Vertices [0];
	for (int i = 1; i < _Vertices.Count (); i++) {
		if (_Vertices [i].X < _BoundsMinimum.X) _BoundsMinimum.X = _Vertices [i].X;
		if (_Vertices [i].Y < _BoundsMinimum.Y) _BoundsMinimum.Y = _Vertices [i].Y;
		if (_Vertices [i].X > _BoundsMaximum.X) _BoundsMaximum.X = _Vertices [i].X;
		if (_Vertices [i].Y > _BoundsMaximum.Y) _BoundsMaximum.Y = _Vertices [i].Y;
	}

	_Configured = true;

	#undef FAIL
	return true;
}


/***********************************************************************************************
 * WorldBoundsService::Get_Inward_Distance -- the primitive every classification derives from    *
 *                                                                                             *
 *	Positive inside (distance to the nearest edge), negative outside.  Unconfigured answers a    *
 *	large positive value, which is what makes every zone query below degrade to                  *
 *	"playable interior" on a legacy map without any of them needing their own special case.      *
 *=============================================================================================*/
float WorldBoundsService::Get_Inward_Distance(const Vector2 & point)
{
	if (!_Configured) {
		return 1.0e30f;
	}

	//	Fast reject: outside the cached AABB cannot be inside the ring.  The returned magnitude
	//	is still a true lower bound on how far outside the point is, so callers that compare
	//	against a band width behave identically with and without this path.
	if ((point.X < _BoundsMinimum.X) || (point.X > _BoundsMaximum.X) ||
		 (point.Y < _BoundsMinimum.Y) || (point.Y > _BoundsMaximum.Y)) {

		float dx = 0.0f;
		if (point.X < _BoundsMinimum.X)			dx = _BoundsMinimum.X - point.X;
		else if (point.X > _BoundsMaximum.X)	dx = point.X - _BoundsMaximum.X;

		float dy = 0.0f;
		if (point.Y < _BoundsMinimum.Y)			dy = _BoundsMinimum.Y - point.Y;
		else if (point.Y > _BoundsMaximum.Y)	dy = point.Y - _BoundsMaximum.Y;

		return -WWMath::Sqrt (dx * dx + dy * dy);
	}

	float nearest = 1.0e30f;
	int count = _Vertices.Count ();
	for (int i = 0, j = count - 1; i < count; j = i++) {
		float distance = Distance_To_Segment (point,_Vertices [j],_Vertices [i]);
		if (distance < nearest) {
			nearest = distance;
		}
	}

	return Point_In_Polygon (point) ? nearest : -nearest;
}


WorldBoundsZoneType WorldBoundsService::Classify_Point(const Vector2 & point)
{
	if (!_Configured) {
		return WORLD_ZONE_PLAYABLE_INTERIOR;
	}

	float inward = Get_Inward_Distance (point);

	if (inward < 0.0f)														return WORLD_ZONE_OUTSIDE_WORLD;
	if (inward < _HardBarrierOffset)									return WORLD_ZONE_HARD_BOUNDARY;
	if (inward < _HardBarrierOffset + _CliffBandWidth)			return WORLD_ZONE_CLIFF_BAND;
	if (inward < _HardBarrierOffset + _CliffBandWidth + _ApproachBandWidth)
																				return WORLD_ZONE_BOUNDARY_APPROACH;
	return WORLD_ZONE_PLAYABLE_INTERIOR;
}


bool WorldBoundsService::Contains_Horizontal_Point(const Vector2 & point)
{
	if (!_Configured) {
		return true;
	}

	//	Legal for an actor means "past the hard barrier, inward" -- the cliff band is inside the
	//	world even though its geometry is what normally stops anyone from standing there.
	return Get_Inward_Distance (point) >= _HardBarrierOffset;
}


bool WorldBoundsService::Contains_Gameplay_Point(const Vector3 & point)
{
	if (!_Configured) {
		return true;
	}

	if (!Contains_Horizontal_Point (Vector2 (point.X,point.Y))) {
		return false;
	}

	//	Vertical: the configured ceiling only.  There is deliberately no floor test -- Section
	//	5.7 asks for a flight ceiling and nothing in Section 5 defines a world floor, and
	//	"fell through the terrain" is a collision failure owned by the physics scene, not a
	//	boundary escape.  Inventing a floor depth with no profile field behind it would be a
	//	guess this service has no business making.
	return (point.Z <= _AirCeiling);
}


bool WorldBoundsService::Is_Inside_Air_Volume(const Vector3 & point)
{
	//	The same horizontal polygon plus the same ceiling, on purpose: Phase 2's own "do not
	//	proceed if" list names "aircraft uses unrelated bounds" as a failure.
	return Contains_Gameplay_Point (point);
}


Vector3 WorldBoundsService::Closest_Valid_Point(const Vector3 & point)
{
	if (!_Configured) {
		return point;
	}

	Vector3 result = point;

	if (result.Z > _AirCeiling) {
		result.Z = _AirCeiling;
	}

	Vector2 horizontal (result.X,result.Y);
	if (Contains_Horizontal_Point (horizontal)) {
		return result;
	}

	/*
	**	Walk in toward the nearest edge's closest point and then keep going inward past the
	**	hard band, so what comes back is legal rather than merely nearer.  The extra step uses
	**	the direction from the closest edge point back toward the polygon's own vertex
	**	average, which for any non-degenerate ring points into the interior.
	*/
	Vector2 closest (0.0f,0.0f);
	float nearest = 1.0e30f;

	int count = _Vertices.Count ();
	for (int i = 0, j = count - 1; i < count; j = i++) {
		const Vector2 & a = _Vertices [j];
		const Vector2 & b = _Vertices [i];

		Vector2 ab = b - a;
		float length2 = ab.Length2 ();
		float t = 0.0f;
		if (length2 > 0.0f) {
			t = ((horizontal.X - a.X) * ab.X + (horizontal.Y - a.Y) * ab.Y) / length2;
			if (t < 0.0f) t = 0.0f;
			if (t > 1.0f) t = 1.0f;
		}

		Vector2 candidate (a.X + ab.X * t,a.Y + ab.Y * t);
		float distance = (horizontal - candidate).Length ();
		if (distance < nearest) {
			nearest	= distance;
			closest	= candidate;
		}
	}

	Vector2 centre (0.0f,0.0f);
	for (int i = 0; i < count; i++) {
		centre.X += _Vertices [i].X;
		centre.Y += _Vertices [i].Y;
	}
	centre.X /= (float)count;
	centre.Y /= (float)count;

	Vector2 inward = centre - closest;
	float inward_length = inward.Length ();
	if (inward_length > 0.0f) {
		inward.X /= inward_length;
		inward.Y /= inward_length;

		//	A small margin past the hard band, so a point recovered onto the exact boundary of
		//	legality does not immediately fail the next frame's own test to floating-point noise.
		float step = _HardBarrierOffset + (_CliffBandWidth * 0.5f);
		closest.X += inward.X * step;
		closest.Y += inward.Y * step;
	}

	result.X = closest.X;
	result.Y = closest.Y;
	return result;
}


bool WorldBoundsService::Segment_Crosses_Hard_Boundary(const Vector2 & start,const Vector2 & end)
{
	if (!_Configured) {
		return false;
	}

	if (!Contains_Horizontal_Point (start) || !Contains_Horizontal_Point (end)) {
		return true;
	}

	/*
	**	Both ends are legal, but the span between them may still leave and re-enter -- a road
	**	chord across a concave part of an irregular ring is exactly that case.  Sample at a
	**	spacing fine enough that no excursion wider than the hard band can hide between two
	**	samples; a coarser walk would answer "no crossing" for precisely the geometry this
	**	query exists to catch.
	*/
	float length = (end - start).Length ();
	if (length <= 0.0f) {
		return false;
	}

	float spacing = _HardBarrierOffset;
	if (spacing < MINIMUM_VERTEX_SPACING) {
		spacing = _CliffBandWidth;
	}
	if (spacing < MINIMUM_VERTEX_SPACING) {
		spacing = MINIMUM_VERTEX_SPACING;
	}

	int steps = (int)(length / spacing) + 1;

	//	Bounded, so a pathological length against a tiny band cannot turn one planner query
	//	into an unbounded loop.  At the cap the sampling is coarser than ideal and the answer
	//	becomes optimistic rather than wrong-in-the-dangerous-direction: a caller still has to
	//	validate the geometry it finally commits.
	const int MAX_STEPS = 4096;
	if (steps > MAX_STEPS) {
		steps = MAX_STEPS;
	}

	for (int i = 1; i < steps; i++) {
		float t = (float)i / (float)steps;
		Vector2 sample (start.X + (end.X - start.X) * t,start.Y + (end.Y - start.Y) * t);
		if (!Contains_Horizontal_Point (sample)) {
			return true;
		}
	}

	return false;
}


bool WorldBoundsService::Can_Place_In_Region(const Vector2 & centre,float radius)
{
	if (!_Configured) {
		return true;
	}

	if (radius < 0.0f) {
		radius = 0.0f;
	}

	/*
	**	Section 5.9: nothing may be built in the cliff band, so the legality floor for
	**	construction is the inner edge of that band rather than the hard barrier.  Subtracting
	**	the footprint radius is what stops a building whose centre clears the band from putting
	**	a corner inside it.
	*/
	float required = _HardBarrierOffset + _CliffBandWidth + radius;
	return Get_Inward_Distance (centre) >= required;
}


Vector3 WorldBoundsService::Clamp_Commander_Camera(const Vector3 & focus)
{
	if (!_Configured) {
		return focus;
	}

	//	Section 5.8.  The camera focus is clamped by the same horizontal rule an actor gets --
	//	one boundary, one answer -- and the ceiling clamp comes along with Closest_Valid_Point.
	return Closest_Valid_Point (focus);
}


bool WorldBoundsService::Recover_Out_Of_Bounds_Actor(const Vector3 & attempted,const Vector3 & last_safe,
																		  bool has_last_safe,const char * actor_description,
																		  Vector3 * out_recovered)
{
	if (!_Configured || !_ServerAuthoritative) {
		return false;
	}

	if (Contains_Gameplay_Point (attempted)) {
		return false;
	}

	/*
	**	Prefer the actor's own last known good transform -- it was reachable, so it is
	**	certainly legal ground -- and fall back to the geometric closest valid point when the
	**	actor has not had a legal frame yet (spawned out of bounds, or freshly loaded).
	**	Section 5.6 is explicit that death is not the boundary mechanic; this restores, it
	**	never kills.
	*/
	Vector3 recovered = (has_last_safe && Contains_Gameplay_Point (last_safe))
								? last_safe
								: Closest_Valid_Point (attempted);

	_ViolationCount++;

	//	Phase 2 step 17: actor identity, prior transform, attempted transform, enough to
	//	reproduce.  WWDEBUG_SAY rather than a new logging channel -- this is the same reporting
	//	path every other physics-side diagnostic in this tree already uses.
	WWDEBUG_SAY (("WorldBounds: %s left the world -- attempted (%.2f,%.2f,%.2f), prior (%.2f,%.2f,%.2f), restored (%.2f,%.2f,%.2f), violation #%d\n",
					  (actor_description != nullptr) ? actor_description : "<unnamed actor>",
					  attempted.X,attempted.Y,attempted.Z,
					  last_safe.X,last_safe.Y,last_safe.Z,
					  recovered.X,recovered.Y,recovered.Z,
					  _ViolationCount));

	if (out_recovered != nullptr) {
		*out_recovered = recovered;
	}
	return true;
}


bool WorldBoundsService::Get_Zone_Ring(WorldBoundsZoneType zone,DynamicVectorClass<Vector2> * out_vertices)
{
	if (!_Configured || (out_vertices == nullptr)) {
		return false;
	}

	//	How far inward this zone's inner edge sits.  OUTSIDE_WORLD's own edge is the configured
	//	polygon itself, which is the zero-inset case.
	float inset = 0.0f;
	switch (zone) {
		case WORLD_ZONE_OUTSIDE_WORLD:		inset = 0.0f;																		break;
		case WORLD_ZONE_HARD_BOUNDARY:		inset = _HardBarrierOffset;													break;
		case WORLD_ZONE_CLIFF_BAND:			inset = _HardBarrierOffset + _CliffBandWidth;								break;
		case WORLD_ZONE_BOUNDARY_APPROACH:	inset = _HardBarrierOffset + _CliffBandWidth + _ApproachBandWidth;	break;
		case WORLD_ZONE_PLAYABLE_INTERIOR:
		default:
			//	The interior has no ring of its own -- it is whatever the approach band encloses.
			return false;
	}

	Vector2 centre (0.0f,0.0f);
	int count = _Vertices.Count ();
	for (int i = 0; i < count; i++) {
		centre.X += _Vertices [i].X;
		centre.Y += _Vertices [i].Y;
	}
	centre.X /= (float)count;
	centre.Y /= (float)count;

	/*
	**	Inset toward the ring's own centre rather than by a true polygon offset.  This is debug
	**	drawing (Section 5.10), not gameplay geometry -- nothing queries these rings for
	**	legality, `Classify_Point` is the authority -- so the cheap approximation is right here,
	**	and it is exact for the roughly-radial rings this generator produces.  A true
	**	straight-skeleton offset would be the answer if a band ring ever became load-bearing.
	*/
	out_vertices->Delete_All ();
	for (int i = 0; i < count; i++) {
		Vector2 outward = _Vertices [i] - centre;
		float length = outward.Length ();
		if (length <= inset) {
			//	The band closed up on itself for this vertex -- the world is too small for the
			//	configured bands to all exist, which Configure cannot catch on its own because
			//	it depends on the ring's shape, not just its area.
			return false;
		}

		float scale = (length - inset) / length;
		out_vertices->Add (Vector2 (centre.X + outward.X * scale,centre.Y + outward.Y * scale));
	}

	return true;
}
