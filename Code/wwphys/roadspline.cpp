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
 *                     $Archive:: /Commando/Code/WWPhys/roadspline.cpp                        $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Functions:                                                                                  *
 *   RoadSplineClass::Evaluate -- one point on the Catmull-Rom curve                            *
 *   RoadSplineClass::Tessellate -- turn the curve into points a ribbon can be built on         *
 *   RoadSplineClass::Subdivide -- split until the chord is close enough to the curve           *
 *   RoadSplineClass::Update_Frames -- tangents, sides and distances from the positions         *
 *   RoadSplineClass::Trim_Start -- cut the road back from its beginning                        *
 *   RoadSplineClass::Trim_End -- cut the road back from its end                                *
 *   RoadSplineClass::Find_Closest_Point -- where on the centre line a position is              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "roadspline.h"

#include "wwmath.h"


/*
**	How far the subdivision is allowed to go within one control point segment.  Eight halvings
**	is two hundred and fifty six points, which is far more than a road segment can need and
**	still a bound rather than a promise.
*/
static const int ROADSPLINE_MAX_DEPTH = 8;

/*
**	Two points closer together than this are the same point.  A zero length segment has no
**	tangent, and a ribbon built on one has a degenerate quad in it.
*/
static const float ROADSPLINE_MIN_SPACING = 0.01f;

static const RoadPointClass	_EmptyPoint;
static const Vector3			_EmptyControlPoint(0.0f,0.0f,0.0f);


RoadSplineClass::RoadSplineClass(void) :
	ID(-1),
	Width(6.0f),
	ShoulderWidth(2.0f),
	MetersPerTile(8.0f),
	RoadClass(ROAD_CLASS_STREET),
	GradePolicy(ROAD_GRADE_NONE),
	Length(0.0f)
{
}


RoadSplineClass::RoadSplineClass(const RoadSplineClass & that)
{
	*this = that;
}


RoadSplineClass::~RoadSplineClass(void)
{
}


RoadSplineClass & RoadSplineClass::operator = (const RoadSplineClass & that)
{
	if (this == &that) {
		return *this;
	}

	ID					= that.ID;
	Name				= that.Name;
	Material			= that.Material;
	Width				= that.Width;
	ShoulderWidth	= that.ShoulderWidth;
	MetersPerTile	= that.MetersPerTile;
	RoadClass		= that.RoadClass;
	GradePolicy		= that.GradePolicy;
	Length			= that.Length;
	StartConnection = that.StartConnection;
	EndConnection	= that.EndConnection;

	ControlPoints.Delete_All();
	for (int i = 0; i < that.ControlPoints.Count(); i++) {
		ControlPoints.Add(that.ControlPoints[i]);
	}

	Points.Delete_All();
	for (int i = 0; i < that.Points.Count(); i++) {
		Points.Add(that.Points[i]);
	}

	return *this;
}


/*
**	Two roads are the same road when they were authored the same way.  The tessellation is not
**	compared because it is derived from what is compared -- and because this operator exists to
**	satisfy the vector the roads are stored in, which needs to be able to find one.
*/
bool RoadSplineClass::operator == (const RoadSplineClass & that) const
{
	if ((ID != that.ID) || (Width != that.Width) || (ShoulderWidth != that.ShoulderWidth) ||
		 (MetersPerTile != that.MetersPerTile) || (RoadClass != that.RoadClass) ||
		 (GradePolicy != that.GradePolicy)) {
		return false;
	}

	if ((StartConnection != that.StartConnection) || (EndConnection != that.EndConnection)) {
		return false;
	}

	if (ControlPoints.Count() != that.ControlPoints.Count()) {
		return false;
	}
	for (int i = 0; i < ControlPoints.Count(); i++) {
		if (ControlPoints[i] != that.ControlPoints[i]) {
			return false;
		}
	}

	return (Name == that.Name) && (Material == that.Material);
}


void RoadSplineClass::Reset(void)
{
	ControlPoints.Delete_All();
	Points.Delete_All();
	Length = 0.0f;
	StartConnection = RoadConnectionClass();
	EndConnection = RoadConnectionClass();
}


void RoadSplineClass::Set_Name(const char * name)
{
	Name = (name != nullptr) ? name : "";
}


void RoadSplineClass::Set_Material(const char * name)
{
	Material = (name != nullptr) ? name : "";
}


void RoadSplineClass::Add_Control_Point(const Vector3 & point)
{
	ControlPoints.Add(point);
	Points.Delete_All();
	Length = 0.0f;
}


bool RoadSplineClass::Set_Control_Points(const Vector3 * points,int count)
{
	if ((points == nullptr) || (count < 2)) {
		return false;
	}

	ControlPoints.Delete_All();
	for (int i = 0; i < count; i++) {
		ControlPoints.Add(points[i]);
	}

	Points.Delete_All();
	Length = 0.0f;
	return true;
}


const Vector3 & RoadSplineClass::Get_Control_Point(int index) const
{
	if ((index < 0) || (index >= ControlPoints.Count())) {
		return _EmptyControlPoint;
	}
	return ControlPoints[index];
}


const RoadPointClass & RoadSplineClass::Get_Point(int index) const
{
	if ((index < 0) || (index >= Points.Count())) {
		return _EmptyPoint;
	}
	return Points[index];
}


RoadPointClass & RoadSplineClass::Peek_Point(int index)
{
	static RoadPointClass _scratch;
	if ((index < 0) || (index >= Points.Count())) {
		_scratch = RoadPointClass();
		return _scratch;
	}
	return Points[index];
}


/***********************************************************************************************
 * RoadSplineClass::Evaluate -- one point on the Catmull-Rom curve                              *
 *                                                                                             *
 * The segment is the span between control point `segment` and the one after it, and t runs     *
 * from zero to one across it.  The two control points outside the segment shape the curve       *
 * without being on it; at the ends of the road there is no outside point, so the end point      *
 * stands in for it, which makes the road leave and arrive straight rather than curling.        *
 *=============================================================================================*/
Vector3 RoadSplineClass::Evaluate(int segment,float t) const
{
	int count = ControlPoints.Count();
	if (count == 0) {
		return Vector3(0.0f,0.0f,0.0f);
	}
	if (count == 1) {
		return ControlPoints[0];
	}

	if (segment < 0) segment = 0;
	if (segment > count - 2) segment = count - 2;

	int i0 = segment - 1;	if (i0 < 0) i0 = 0;
	int i1 = segment;
	int i2 = segment + 1;
	int i3 = segment + 2;	if (i3 > count - 1) i3 = count - 1;

	const Vector3 & p0 = ControlPoints[i0];
	const Vector3 & p1 = ControlPoints[i1];
	const Vector3 & p2 = ControlPoints[i2];
	const Vector3 & p3 = ControlPoints[i3];

	float t2 = t * t;
	float t3 = t2 * t;

	return 0.5f * ((2.0f * p1)
					 + (-p0 + p2) * t
					 + ((2.0f * p0) - (5.0f * p1) + (4.0f * p2) - p3) * t2
					 + (-p0 + (3.0f * p1) - (3.0f * p2) + p3) * t3);
}


Vector3 RoadSplineClass::Evaluate_At(float t) const
{
	int count = ControlPoints.Count();
	if (count < 2) {
		return Evaluate(0,0.0f);
	}

	int segments = count - 1;
	if (t <= 0.0f) return Evaluate(0,0.0f);
	if (t >= 1.0f) return Evaluate(segments - 1,1.0f);

	float scaled = t * (float)segments;
	int segment = (int)WWMath::Floor(scaled);
	if (segment > segments - 1) segment = segments - 1;

	return Evaluate(segment,scaled - (float)segment);
}


void RoadSplineClass::Append_Point(const Vector3 & position)
{
	if (Points.Count() > 0) {
		Vector3 delta = position - Points[Points.Count() - 1].Position;
		if (delta.Length() < ROADSPLINE_MIN_SPACING) {
			return;
		}
	}

	RoadPointClass point;
	point.Position = position;
	point.HalfWidth = Width * 0.5f;
	Points.Add(point);
}


/***********************************************************************************************
 * RoadSplineClass::Subdivide -- split until the chord is close enough to the curve             *
 *                                                                                             *
 * Two reasons to split.  The obvious one is that the straight line between the ends has        *
 * wandered too far from the curve, which is what makes a bend smooth.  The other is that the   *
 * chord is simply long: a straight road needs points anyway, because the ground under it is    *
 * not straight and there is nothing to follow the ground with between two distant points.      *
 *=============================================================================================*/
void RoadSplineClass::Subdivide(int segment,float t0,float t1,const Vector3 & p0,const Vector3 & p1,
										  int depth,float max_deviation,float min_segment,float max_segment)
{
	float tm = 0.5f * (t0 + t1);
	Vector3 pm = Evaluate(segment,tm);

	float length = (p1 - p0).Length();
	float deviation = (pm - (0.5f * (p0 + p1))).Length();

	bool wants_split = (deviation > max_deviation) || (length > max_segment);
	bool may_split = (depth > 0) && (length > (2.0f * min_segment));

	if (wants_split && may_split) {
		Subdivide(segment,t0,tm,p0,pm,depth - 1,max_deviation,min_segment,max_segment);
		Subdivide(segment,tm,t1,pm,p1,depth - 1,max_deviation,min_segment,max_segment);
	} else {
		Append_Point(p1);
	}
}


/***********************************************************************************************
 * RoadSplineClass::Tessellate -- turn the curve into points a ribbon can be built on           *
 *=============================================================================================*/
bool RoadSplineClass::Tessellate(float max_deviation,float min_segment,float max_segment)
{
	Points.Delete_All();
	Length = 0.0f;

	int count = ControlPoints.Count();
	if (count < 2) {
		return false;
	}

	if (!(max_deviation > 0.0f)) max_deviation = 0.25f;
	if (!(min_segment > 0.0f)) min_segment = ROADSPLINE_MIN_SPACING;
	if (!(max_segment > min_segment)) max_segment = min_segment * 2.0f;

	Append_Point(Evaluate(0,0.0f));

	for (int segment = 0; segment < count - 1; segment++) {
		Vector3 p0 = Evaluate(segment,0.0f);
		Vector3 p1 = Evaluate(segment,1.0f);
		Subdivide(segment,0.0f,1.0f,p0,p1,ROADSPLINE_MAX_DEPTH,max_deviation,min_segment,max_segment);
	}

	if (Points.Count() < 2) {
		Points.Delete_All();
		return false;
	}

	Update_Frames();
	return true;
}


/***********************************************************************************************
 * RoadSplineClass::Update_Frames -- tangents, sides and distances from the positions           *
 *                                                                                             *
 * Called again after anything moves the points, which conforming them to the ground does.      *
 *                                                                                             *
 * The side vector is horizontal by construction rather than perpendicular to the surface.  A   *
 * road on a hillside measured perpendicular to the slope would be narrower on the map than it  *
 * is on the ground, and every other system -- the mask, the grade, the navigation query --     *
 * works in plan.  The width of a road is a width on the map.                                   *
 *=============================================================================================*/
void RoadSplineClass::Update_Frames(void)
{
	int count = Points.Count();
	if (count < 2) {
		Length = 0.0f;
		return;
	}

	Points[0].Distance = 0.0f;
	for (int i = 1; i < count; i++) {
		Points[i].Distance = Points[i-1].Distance + (Points[i].Position - Points[i-1].Position).Length();
	}
	Length = Points[count-1].Distance;

	for (int i = 0; i < count; i++) {

		int prev = (i > 0) ? (i - 1) : i;
		int next = (i < count - 1) ? (i + 1) : i;

		Vector3 tangent = Points[next].Position - Points[prev].Position;
		if (tangent.Length() < WWMATH_EPSILON) {
			tangent.Set(1.0f,0.0f,0.0f);
		}
		tangent.Normalize();
		Points[i].Tangent = tangent;

		//	The side is the tangent flattened into the ground plane and turned a quarter turn to
		//	the left.  A road that goes straight up is not a road, so a tangent with no
		//	horizontal component keeps whatever side it had rather than being made up.
		Vector3 flat(tangent.X,tangent.Y,0.0f);
		if (flat.Length() > WWMATH_EPSILON) {
			flat.Normalize();
			Points[i].Side.Set(-flat.Y,flat.X,0.0f);
		}

		Vector3 normal = Vector3::Cross_Product(Points[i].Tangent,Points[i].Side);
		if (normal.Length() > WWMATH_EPSILON) {
			normal.Normalize();
			if (normal.Z < 0.0f) {
				normal = -normal;
			}
			Points[i].Normal = normal;
		}

		Points[i].HalfWidth = Width * 0.5f;
	}
}


/***********************************************************************************************
 * RoadSplineClass::Trim_Start -- cut the road back from its beginning                          *
 *                                                                                             *
 * A junction owns the ground where roads meet, and a bridge owns the span; in both cases the   *
 * road's own surface has to stop short and stop exactly, or there is an overlap that fights     *
 * for depth or a gap that shows the ground through.  So the cut leaves a point at the cut       *
 * rather than at the nearest tessellated point.                                                *
 *=============================================================================================*/
bool RoadSplineClass::Trim_Start(float distance)
{
	if (!Is_Tessellated() || !(distance > 0.0f)) {
		return false;
	}

	if (distance >= Length) {
		Points.Delete_All();
		Length = 0.0f;
		return false;
	}

	int cut = 1;
	while ((cut < Points.Count() - 1) && (Points[cut].Distance < distance)) {
		cut++;
	}

	Vector3 before = Points[cut-1].Position;
	Vector3 after = Points[cut].Position;
	float span = Points[cut].Distance - Points[cut-1].Distance;
	float t = (span > WWMATH_EPSILON) ? ((distance - Points[cut-1].Distance) / span) : 1.0f;
	if (t < 0.0f) t = 0.0f;
	if (t > 1.0f) t = 1.0f;
	Vector3 position = before + ((after - before) * t);

	DynamicVectorClass<RoadPointClass> kept;
	if ((position - after).Length() >= ROADSPLINE_MIN_SPACING) {
		RoadPointClass point = Points[cut];
		point.Position = position;
		kept.Add(point);
	}
	for (int i = cut; i < Points.Count(); i++) {
		kept.Add(Points[i]);
	}

	Points.Delete_All();
	for (int i = 0; i < kept.Count(); i++) {
		Points.Add(kept[i]);
	}

	if (Points.Count() < 2) {
		Points.Delete_All();
		Length = 0.0f;
		return false;
	}

	Update_Frames();
	return true;
}


/***********************************************************************************************
 * RoadSplineClass::Trim_End -- cut the road back from its end                                  *
 *=============================================================================================*/
bool RoadSplineClass::Trim_End(float distance)
{
	if (!Is_Tessellated() || !(distance > 0.0f)) {
		return false;
	}

	if (distance >= Length) {
		Points.Delete_All();
		Length = 0.0f;
		return false;
	}

	float target = Length - distance;

	int cut = Points.Count() - 1;
	while ((cut > 1) && (Points[cut-1].Distance > target)) {
		cut--;
	}

	Vector3 before = Points[cut-1].Position;
	Vector3 after = Points[cut].Position;
	float span = Points[cut].Distance - Points[cut-1].Distance;
	float t = (span > WWMATH_EPSILON) ? ((target - Points[cut-1].Distance) / span) : 0.0f;
	if (t < 0.0f) t = 0.0f;
	if (t > 1.0f) t = 1.0f;
	Vector3 position = before + ((after - before) * t);

	DynamicVectorClass<RoadPointClass> kept;
	for (int i = 0; i < cut; i++) {
		kept.Add(Points[i]);
	}
	if ((position - before).Length() >= ROADSPLINE_MIN_SPACING) {
		RoadPointClass point = Points[cut-1];
		point.Position = position;
		kept.Add(point);
	}

	Points.Delete_All();
	for (int i = 0; i < kept.Count(); i++) {
		Points.Add(kept[i]);
	}

	if (Points.Count() < 2) {
		Points.Delete_All();
		Length = 0.0f;
		return false;
	}

	Update_Frames();
	return true;
}


/***********************************************************************************************
 * RoadSplineClass::Find_Closest_Point -- where on the centre line a position is                *
 *                                                                                             *
 * Lateral distance is measured in plan rather than in space.  What a caller means by "how far  *
 * off the road am I" is how far to one side, and a soldier on a hill standing beside a road    *
 * that climbs is not further from it for being lower down.                                     *
 *=============================================================================================*/
bool RoadSplineClass::Find_Closest_Point(const Vector3 & position,int * index_out,
													  float * lateral_out,float * along_out) const
{
	if (!Is_Tessellated()) {
		return false;
	}

	float best_lateral = 0.0f;
	float best_along = 0.0f;
	int best_index = 0;
	bool have_best = false;

	for (int i = 0; i < Points.Count() - 1; i++) {

		const Vector3 & a = Points[i].Position;
		const Vector3 & b = Points[i+1].Position;

		float dx = b.X - a.X;
		float dy = b.Y - a.Y;
		float span_sq = (dx * dx) + (dy * dy);
		if (span_sq < WWMATH_EPSILON) {
			continue;
		}

		float t = (((position.X - a.X) * dx) + ((position.Y - a.Y) * dy)) / span_sq;
		if (t < 0.0f) t = 0.0f;
		if (t > 1.0f) t = 1.0f;

		float cx = a.X + (dx * t);
		float cy = a.Y + (dy * t);
		float lateral = WWMath::Sqrt(((position.X - cx) * (position.X - cx)) +
											  ((position.Y - cy) * (position.Y - cy)));

		if (!have_best || (lateral < best_lateral)) {
			have_best = true;
			best_lateral = lateral;
			best_along = Points[i].Distance + ((Points[i+1].Distance - Points[i].Distance) * t);
			best_index = (t < 0.5f) ? i : (i + 1);
		}
	}

	if (!have_best) {
		return false;
	}

	if (index_out != nullptr)		*index_out = best_index;
	if (lateral_out != nullptr)	*lateral_out = best_lateral;
	if (along_out != nullptr)		*along_out = best_along;
	return true;
}
