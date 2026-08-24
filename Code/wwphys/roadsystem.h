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
 *                     $Archive:: /Commando/Code/WWPhys/roadsystem.h                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	Roads made from the lines they were drawn as.                                               *
 *                                                                                             *
 *	Roadmap Section 19's acceptance is that road geometry can be generated at runtime from      *
 *	centre lines without a unique modelled mesh for every road segment.  This service is what   *
 *	generates it: it holds the splines, works out where they meet, cuts them apart there,       *
 *	builds one mesh per material out of all of them, tells the terrain where the roads are and  *
 *	answers where the nearest road is.                                                          *
 *                                                                                             *
 *	The donor is Zero Hour's W3DRoadBuffer, and the two places this deliberately departs from   *
 *	it are worth stating.  The donor draws junctions from an atlas of authored corner pieces --  *
 *	tee, Y, four way, and five more -- selected by shape and oriented to fit.  That is a        *
 *	modelled mesh per junction kind, which is the thing Section 19's acceptance rules out, and  *
 *	it is authored against one particular road texture layout that we do not have.  Here a      *
 *	junction is a polygon built from the ends of the roads that arrive at it, so it fits any    *
 *	number of arms at any angle and comes from the same centre lines everything else does.      *
 *                                                                                             *
 *	The second departure is that the road surface does not collide.  The donor's roads are      *
 *	decoration over a heightfield that owns the collision, and that is right here too and for   *
 *	a stronger reason: Section 17 requires terrain collision and terrain rendering to come      *
 *	from the same source data, and a road with its own collision hull would be a second         *
 *	surface a soldier could stand on, sitting a few centimetres above the first.  A road is a   *
 *	thing you see on the ground, not a thing you stand on instead of the ground.                *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef ROADSYSTEM_H
#define ROADSYSTEM_H

#include "always.h"
#include "roadspline.h"
#include "vector.h"
#include "vector3.h"

class RenderObjClass;
class StaticPhysClass;


/*
**	Where roads meet.
**
**	A junction owns a disc of ground.  Every road that passes through it stops its own surface
**	at the edge of the disc and the junction fills the middle, which is what keeps two crossing
**	ribbons from overlapping and fighting for depth.  A road that passes straight through is
**	not split into two roads: its surface simply has a hole in it where the junction is, which
**	means a crossroads costs the same as a tee and nothing has to decide which road is the one
**	that ends.
*/
class RoadJunctionClass
{
public:

	enum { MAX_ARMS = 8 };

	RoadJunctionClass(void) : Position(0.0f,0.0f,0.0f), Radius(0.0f), ArmCount(0)
	{
		for (int i = 0; i < MAX_ARMS; i++) { Arms[i] = -1; }
	}

	bool operator == (const RoadJunctionClass & that) const
	{
		if ((Position != that.Position) || (Radius != that.Radius) || (ArmCount != that.ArmCount)) {
			return false;
		}
		for (int i = 0; i < MAX_ARMS; i++) {
			if (Arms[i] != that.Arms[i]) { return false; }
		}
		return true;
	}
	bool operator != (const RoadJunctionClass & that) const	{ return !(*this == that); }

	bool					Has_Arm(int road_id) const
	{
		for (int i = 0; i < ArmCount; i++) {
			if (Arms[i] == road_id) { return true; }
		}
		return false;
	}

	Vector3				Position;
	float					Radius;
	int					ArmCount;
	int					Arms[MAX_ARMS];		// road ids meeting here
};


/*
**	What the navigation metadata Section 19 asks for actually is.
**
**	Pathing does not want the road's geometry; it wants to know that there is a road here, which
**	way it runs, how far off it you are and what kind it is.  That is enough for an AI to prefer
**	a road, to follow one, and to decide it has left one.
*/
class RoadNavInfoClass
{
public:
	RoadNavInfoClass(void) :
		RoadID(-1), PointIndex(0), RoadClass(ROAD_CLASS_TRACK),
		LateralDistance(0.0f), DistanceAlong(0.0f),
		Position(0.0f,0.0f,0.0f), Tangent(1.0f,0.0f,0.0f), IsOnRoad(false) { }

	int					RoadID;
	int					PointIndex;
	int					RoadClass;
	float					LateralDistance;		// in plan, from the centre line
	float					DistanceAlong;			// from the start of that road
	Vector3				Position;				// the nearest point on the centre line
	Vector3				Tangent;					// which way the road runs there
	bool					IsOnRoad;				// within the carriageway, not merely near it
};


class RoadSystem
{
public:

	static void			Init(void);
	static void			Shutdown(void);

	/*
	**	The roads themselves.  Add_Road copies the spline in and returns the id it was given,
	**	which is its index; ids are stable until Clear_Roads.
	*/
	static int			Add_Road(const RoadSplineClass & road);
	static RoadSplineClass *	Peek_Road(int id);
	static int			Get_Road_Count(void);
	static void			Clear_Roads(void);

	/*
	**	Everything that does not need a graphics device: tessellate the splines, sit them on the
	**	ground, find the junctions, cut the roads back at them and at their bridge and tunnel
	**	connections, grade the terrain where a road asked for it, and write the road mask so the
	**	terrain texture system knows where the roads are.
	**
	**	A dedicated server runs exactly this and stops.  Nothing below this line changes what
	**	happens in a match, which is why it can.
	*/
	static bool			Build_Network(float max_deviation = 0.25f,float min_segment = 0.5f,
											 float max_segment = 4.0f);
	static bool			Is_Network_Built(void);

	/*
	**	The meshes.  One per material across all the roads that use it, because a hundred road
	**	segments that share a texture should be one thing to draw -- that is what Section 19
	**	means by material batching.
	*/
	static bool			Build_Geometry(void);
	static void			Destroy_Geometry(void);
	static bool			Has_Geometry(void);
	static int			Get_Batch_Count(void);
	static RenderObjClass *	Peek_Batch_Model(int index);
	static const char *	Get_Batch_Material(int index);

	/*
	**	The surface as stretches rather than as triangles.
	**
	**	A road's surface is cut wherever it enters a junction, so a crossroads leaves each of the
	**	two roads with a hole in the middle and two stretches either side.  That shape is decided
	**	entirely by the centre lines and the junctions, with no device involved, which is why it
	**	is available on its own: the mesh builder consumes it, and so can anything else that
	**	wants to know where road surface actually is.
	*/
	static void			Collect_Surface_Runs(void);
	static int			Get_Surface_Run_Count(void);
	static bool			Get_Surface_Run(int index,int * road_id_out,int * point_count_out);
	static const RoadPointClass *	Peek_Surface_Run_Point(int index,int point);
	static int			Get_Surface_Boundary_Count(void);

	/*
	**	Junctions, once the network is built.
	*/
	static int			Get_Junction_Count(void);
	static const RoadJunctionClass *	Peek_Junction(int index);

	/*
	**	Navigation.  Section 19's path metadata, asked as a question rather than stored as a
	**	graph, because the graph belongs to the pathing phase and the roads are what it will
	**	be built from.
	*/
	static bool			Find_Nearest_Road(const Vector3 & position,float max_distance,
													RoadNavInfoClass * info_out);
	static bool			Is_On_Road(const Vector3 & position);

	/*
	**	Putting a point on the ground.  The terrain service answers if there is terrain; if
	**	there is not -- which is every stock Renegade level -- a ray is dropped through the
	**	physics scene instead, so a road can be laid across authored W3D geometry as well as
	**	across a generated heightfield.  Returns false when neither could say.
	*/
	static bool			Conform_Point(float x,float y,float * height_out,Vector3 * normal_out = nullptr);

	/*
	**	How far above the ground the road surface sits.  Small, and not zero: coplanar surfaces
	**	fight for depth, and the winner changes with the camera.
	*/
	static float		Get_Surface_Offset(void);
	static void			Set_Surface_Offset(float offset);

	/*
	**	The pieces of Build_Network, exposed because a caller that has only some of the world
	**	may want only some of them -- and because each one is separately checkable.
	*/
	static void			Stamp_Road_Mask(void);
	static void			Apply_Grades(void);

private:

	static void			Conform_Road(RoadSplineClass & road);
	static void			Apply_Connection_Trims(RoadSplineClass & road);
	static void			Find_Junctions(void);
	static bool			Merge_Junction(const Vector3 & position,float radius,int road_a,int road_b);

	static DynamicVectorClass<RoadSplineClass>		Roads;
	static DynamicVectorClass<RoadJunctionClass>	Junctions;
	static DynamicVectorClass<StringClass>			BatchMaterials;
	static DynamicVectorClass<StaticPhysClass *>	Batches;

	static bool			NetworkBuilt;
	static float		SurfaceOffset;
};


#endif // ROADSYSTEM_H
