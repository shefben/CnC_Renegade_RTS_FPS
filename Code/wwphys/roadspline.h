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
 *                     $Archive:: /Commando/Code/WWPhys/roadspline.h                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	A road is a line somebody drew, and everything else about it is derived.                    *
 *                                                                                             *
 *	Roadmap Section 19 gives the representation directly: control points, a width, a shoulder   *
 *	width, a material, a road class, a grade policy, and connections to bridges and tunnels.    *
 *	That is the whole of what is authored.  The centre line between the control points, the     *
 *	ribbon either side of it, the texture coordinates along it and the height it sits at are    *
 *	all computed, which is what Section 19's acceptance means by road geometry generated at     *
 *	runtime from centre lines rather than modelled per segment.                                 *
 *                                                                                             *
 *	The curve is Catmull-Rom, chosen because it passes through its control points.  A road is   *
 *	drawn by saying where it goes, and a curve that only approaches the places it was told      *
 *	about would put the road somewhere nobody asked for -- next to the bridge rather than on    *
 *	it, which is exactly the case Section 19 has to get right.                                  *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef ROADSPLINE_H
#define ROADSPLINE_H

#include "always.h"
#include "vector.h"
#include "vector3.h"
#include "wwstring.h"


/*
**	What kind of road this is.  The class is not a width -- width is its own field -- it is what
**	the road is for, which is what navigation and AI want to know when they choose between two
**	routes that are both passable.
*/
enum RoadClassType
{
	ROAD_CLASS_TRACK				= 0,		// dirt, one lane, whatever the ground allows
	ROAD_CLASS_STREET,							// surfaced, built up
	ROAD_CLASS_HIGHWAY,							// surfaced, fast, few junctions
	ROAD_CLASS_COUNT
};


/*
**	Whether the ground is allowed to move to meet the road.
**
**	Section 19 calls terrain grading optional, and it has to be: a road laid across a level's
**	authored W3D geometry must not reshape it, and a road laid across generated heightfield
**	terrain usually should.  The policy is per road because both happen on the same map.
*/
enum RoadGradePolicyType
{
	ROAD_GRADE_NONE				= 0,		// the ground is not touched; the road follows it
	ROAD_GRADE_BLEND,								// the ground is pulled part of the way to the road
	ROAD_GRADE_FLATTEN,							// the ground comes all the way up to the road
	ROAD_GRADE_COUNT
};


/*
**	What is at the end of the road.
*/
enum RoadEndpointType
{
	ROAD_ENDPOINT_OPEN			= 0,		// the road simply stops
	ROAD_ENDPOINT_BRIDGE,						// a bridge carries it on from here
	ROAD_ENDPOINT_TUNNEL,						// a tunnel portal carries it on from here
	ROAD_ENDPOINT_COUNT
};


/*
**	The handover at one end of a road.
**
**	Section 19 asks for bridge endpoint handoff and tunnel portal handoff, and Section 20's
**	bridge system is a later phase.  What can be settled now is the seam: the road stops
**	generating surface at the connection point, records where it stopped, which way it was
**	pointing and what it expects to meet, and the structure that arrives later reads that
**	instead of guessing.  A connection whose TargetID is still -1 is a road that has been told
**	a bridge belongs here and has not been told which one.
*/
class RoadConnectionClass
{
public:
	RoadConnectionClass(void) :
		Type(ROAD_ENDPOINT_OPEN), TargetID(-1),
		Position(0.0f,0.0f,0.0f), Direction(1.0f,0.0f,0.0f) { }

	bool operator == (const RoadConnectionClass & that) const
	{
		return (Type == that.Type) && (TargetID == that.TargetID) &&
				 (Position == that.Position) && (Direction == that.Direction);
	}
	bool operator != (const RoadConnectionClass & that) const	{ return !(*this == that); }

	int				Type;				// RoadEndpointType
	int				TargetID;		// the bridge or tunnel this hands over to, -1 when unknown
	Vector3			Position;		// where the road surface stops
	Vector3			Direction;		// unit, pointing out of the road into the structure
};


/*
**	One point on the tessellated centre line.
**
**	Everything a ribbon, a junction or a navigation query needs about a place on the road is
**	here, so that none of them has to re-derive the frame and risk deriving it differently.
*/
class RoadPointClass
{
public:
	RoadPointClass(void) :
		Position(0.0f,0.0f,0.0f), Tangent(1.0f,0.0f,0.0f), Side(0.0f,1.0f,0.0f),
		Normal(0.0f,0.0f,1.0f), Distance(0.0f), HalfWidth(0.0f) { }

	bool operator == (const RoadPointClass & that) const
	{
		return (Position == that.Position) && (Tangent == that.Tangent) &&
				 (Side == that.Side) && (Normal == that.Normal) &&
				 (Distance == that.Distance) && (HalfWidth == that.HalfWidth);
	}
	bool operator != (const RoadPointClass & that) const	{ return !(*this == that); }

	Vector3			Position;		// on the centre line, at the height the road sits at
	Vector3			Tangent;			// unit, along the road
	Vector3			Side;				// unit, horizontal, to the left of the tangent
	Vector3			Normal;			// unit, up, the surface normal the ribbon is lit by
	float				Distance;		// travelled along the road from its start, in world units
	float				HalfWidth;		// of the carriageway here
};


class RoadSplineClass
{
public:

	RoadSplineClass(void);
	RoadSplineClass(const RoadSplineClass & that);
	~RoadSplineClass(void);

	RoadSplineClass &	operator = (const RoadSplineClass & that);
	bool					operator == (const RoadSplineClass & that) const;
	bool					operator != (const RoadSplineClass & that) const	{ return !(*this == that); }

	void					Reset(void);

	/*
	**	What was authored.
	*/
	void					Set_Name(const char * name);
	const char *		Get_Name(void) const						{ return Name.Peek_Buffer(); }
	void					Set_ID(int id)								{ ID = id; }
	int					Get_ID(void) const						{ return ID; }

	void					Add_Control_Point(const Vector3 & point);
	bool					Set_Control_Points(const Vector3 * points,int count);
	int					Get_Control_Point_Count(void) const	{ return ControlPoints.Count(); }
	const Vector3 &	Get_Control_Point(int index) const;

	void					Set_Width(float width)					{ Width = width; }
	float					Get_Width(void) const					{ return Width; }
	void					Set_Shoulder_Width(float width)		{ ShoulderWidth = width; }
	float					Get_Shoulder_Width(void) const		{ return ShoulderWidth; }

	void					Set_Material(const char * name);
	const char *		Get_Material(void) const				{ return Material.Peek_Buffer(); }
	/*
	**	How many world units of road one tile of the texture covers along its length.  Across
	**	the road the texture always spans the carriageway exactly once, because a road texture
	**	is authored as a road seen from above and half of one is not a road.
	*/
	void					Set_Meters_Per_Tile(float meters)	{ MetersPerTile = meters; }
	float					Get_Meters_Per_Tile(void) const		{ return MetersPerTile; }

	void					Set_Road_Class(int road_class)			{ RoadClass = road_class; }
	int					Get_Road_Class(void) const				{ return RoadClass; }
	void					Set_Grade_Policy(int policy)			{ GradePolicy = policy; }
	int					Get_Grade_Policy(void) const			{ return GradePolicy; }

	RoadConnectionClass &			Get_Start_Connection(void)			{ return StartConnection; }
	RoadConnectionClass &			Get_End_Connection(void)			{ return EndConnection; }
	const RoadConnectionClass &	Get_Start_Connection(void) const	{ return StartConnection; }
	const RoadConnectionClass &	Get_End_Connection(void) const	{ return EndConnection; }

	/*
	**	The curve itself.  Evaluate takes a segment index and a parameter within it; Evaluate_At
	**	takes a single parameter across the whole road, which is how a caller that does not care
	**	about the control point layout asks for a point on it.
	*/
	Vector3				Evaluate(int segment,float t) const;
	Vector3				Evaluate_At(float t) const;

	/*
	**	Turning the curve into points.
	**
	**	max_deviation is how far the straight line between two output points may sit from the
	**	curve; that is what makes a bend smooth without spending points on a straight.  But a
	**	straight still gets points, because max_segment forces them: a dead straight road over a
	**	hill is exactly the case where the centre line is right and the ground under it is not,
	**	and there is nothing to conform to between two points that are a hundred metres apart.
	*/
	bool					Tessellate(float max_deviation = 0.25f,float min_segment = 0.5f,float max_segment = 4.0f);
	bool					Is_Tessellated(void) const					{ return (Points.Count() >= 2); }
	int					Get_Point_Count(void) const				{ return Points.Count(); }
	const RoadPointClass &	Get_Point(int index) const;
	RoadPointClass &			Peek_Point(int index);
	float					Get_Length(void) const						{ return Length; }

	/*
	**	Recompute the tangents, sides and distances after something has moved the points --
	**	conforming them to the ground is what moves them.
	*/
	void					Update_Frames(void);

	/*
	**	Cut the road back from one end, in world units along it, and leave a point exactly at
	**	the cut.  Junction trimming and bridge handover are both this.
	*/
	bool					Trim_Start(float distance);
	bool					Trim_End(float distance);

	/*
	**	The nearest place on the centre line to a point, and how far off it that point is.
	**	Returns false when the road has not been tessellated.
	*/
	bool					Find_Closest_Point(const Vector3 & position,int * index_out,
													float * lateral_out,float * along_out) const;

	/*
	**	The widest the road reaches, carriageway plus both shoulders.  Everything that asks
	**	"does this road affect that place" wants this rather than the carriageway.
	*/
	float					Get_Total_Width(void) const				{ return Width + (2.0f * ShoulderWidth); }

private:

	void					Subdivide(int segment,float t0,float t1,const Vector3 & p0,const Vector3 & p1,
										int depth,float max_deviation,float min_segment,float max_segment);
	void					Append_Point(const Vector3 & position);

	int					ID;
	StringClass			Name;
	StringClass			Material;

	DynamicVectorClass<Vector3>			ControlPoints;
	DynamicVectorClass<RoadPointClass>	Points;

	float					Width;
	float					ShoulderWidth;
	float					MetersPerTile;
	int					RoadClass;
	int					GradePolicy;
	float					Length;

	RoadConnectionClass	StartConnection;
	RoadConnectionClass	EndConnection;
};


#endif // ROADSPLINE_H
