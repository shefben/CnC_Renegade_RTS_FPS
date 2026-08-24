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
 *                     $Archive:: /Commando/Code/WWPhys/roadsystem.cpp                        $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Functions:                                                                                  *
 *   RoadSystem::Conform_Point -- put a point on whatever the ground turns out to be            *
 *   RoadSystem::Build_Network -- everything about roads that does not need a device            *
 *   RoadSystem::Find_Junctions -- where the centre lines meet                                  *
 *   RoadSystem::Merge_Junction -- one crossing, or one more arm on a crossing already found    *
 *   RoadSystem::Stamp_Road_Mask -- tell the terrain where the roads are                        *
 *   RoadSystem::Apply_Grades -- bring the ground up to the roads that asked for it             *
 *   RoadSystem::Build_Geometry -- one mesh per material out of all the roads                   *
 *   RoadSystem::Find_Nearest_Road -- the navigation question                                   *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "roadsystem.h"

#include "assetmgr.h"
#include "castres.h"
#include "coltype.h"
#include "dynamicmeshbuilder.h"
#include "lineseg.h"
#include "matrix3d.h"
#include "physcoltest.h"
#include "pscene.h"
#include "shader.h"
#include "staticphys.h"
#include "terrainmask.h"
#include "terraintexturesystem.h"
#include "texture.h"
#include "vertmaterial.h"
#include "worldterrainsystem.h"
#include "wwdebug.h"
#include "wwmath.h"


/*
**	A junction disc has to be big enough to cover the crossing.  Two roads of equal width
**	crossing at a right angle make a square, and the far corner of that square is the half
**	width times the root of two away from the middle; a little more than that leaves the fan
**	room to be a shape rather than a tight fit.
*/
static const float ROAD_JUNCTION_RADIUS_SCALE	= 1.45f;

/*
**	How far along the road the ground height is averaged before a road that grades is allowed to
**	sit on it.  This is what makes a graded road smooth: its height is the landscape with the
**	bumps taken out, and the ground is then brought to that rather than the other way round.
*/
static const float ROAD_GRADE_SMOOTHING			= 12.0f;

/*
**	The ray dropped to find the ground when there is no heightfield.  Generous, because a road
**	can be laid across a level whose geometry is a long way above or below the point that was
**	asked about.
*/
static const float ROAD_CONFORM_RAY_ABOVE		= 200.0f;
static const float ROAD_CONFORM_RAY_BELOW		= 200.0f;


/*
**	Where a road crosses the edge of a junction disc.
**
**	The ribbon stops here and the junction polygon starts here, from the same computed point, so
**	the two meet exactly rather than nearly.
*/
class RoadBoundaryClass
{
public:
	RoadBoundaryClass(void) : JunctionIndex(-1), RoadID(-1) { }

	bool operator == (const RoadBoundaryClass & that) const
	{
		return (JunctionIndex == that.JunctionIndex) && (RoadID == that.RoadID) &&
				 (Point == that.Point);
	}
	bool operator != (const RoadBoundaryClass & that) const	{ return !(*this == that); }

	int					JunctionIndex;
	int					RoadID;
	RoadPointClass		Point;
};


/*
**	One unbroken stretch of one road's surface.  A road that passes through two junctions has
**	three of these.
*/
class RoadRunClass
{
public:
	RoadRunClass(void) : RoadID(-1), First(0), Count(0) { }

	bool operator == (const RoadRunClass & that) const
	{
		return (RoadID == that.RoadID) && (First == that.First) && (Count == that.Count);
	}
	bool operator != (const RoadRunClass & that) const	{ return !(*this == that); }

	int					RoadID;
	int					First;			// index into RunPoints
	int					Count;
};




DynamicVectorClass<RoadSplineClass>			RoadSystem::Roads;
DynamicVectorClass<RoadJunctionClass>		RoadSystem::Junctions;
DynamicVectorClass<StringClass>				RoadSystem::BatchMaterials;
DynamicVectorClass<StaticPhysClass *>		RoadSystem::Batches;
bool													RoadSystem::NetworkBuilt = false;
float													RoadSystem::SurfaceOffset = 0.08f;

static DynamicVectorClass<RoadPointClass>		_RunPoints;
static DynamicVectorClass<RoadRunClass>		_Runs;
static DynamicVectorClass<RoadBoundaryClass>	_Boundaries;
static bool												_ReportedNoScene = false;


void RoadSystem::Init(void)
{
	Clear_Roads();
	SurfaceOffset = 0.08f;
	_ReportedNoScene = false;
}


void RoadSystem::Shutdown(void)
{
	Clear_Roads();
}


int RoadSystem::Add_Road(const RoadSplineClass & road)
{
	int id = Roads.Count();
	Roads.Add(road);
	Roads[id].Set_ID(id);

	//	Anything already worked out about the network is now a statement about a set of roads
	//	that no longer exists.
	NetworkBuilt = false;
	Junctions.Delete_All();
	return id;
}


RoadSplineClass * RoadSystem::Peek_Road(int id)
{
	if ((id < 0) || (id >= Roads.Count())) {
		return nullptr;
	}
	return &Roads[id];
}


int RoadSystem::Get_Road_Count(void)
{
	return Roads.Count();
}


void RoadSystem::Clear_Roads(void)
{
	Destroy_Geometry();
	Roads.Delete_All();
	Junctions.Delete_All();
	_RunPoints.Delete_All();
	_Runs.Delete_All();
	_Boundaries.Delete_All();
	NetworkBuilt = false;
}


bool RoadSystem::Is_Network_Built(void)
{
	return NetworkBuilt;
}


float RoadSystem::Get_Surface_Offset(void)
{
	return SurfaceOffset;
}


void RoadSystem::Set_Surface_Offset(float offset)
{
	SurfaceOffset = offset;
}


int RoadSystem::Get_Junction_Count(void)
{
	return Junctions.Count();
}


const RoadJunctionClass * RoadSystem::Peek_Junction(int index)
{
	if ((index < 0) || (index >= Junctions.Count())) {
		return nullptr;
	}
	return &Junctions[index];
}


/***********************************************************************************************
 * RoadSystem::Conform_Point -- put a point on whatever the ground turns out to be              *
 *                                                                                             *
 * Two kinds of ground exist in this engine and a road has to lie on either.  Generated         *
 * heightfield terrain answers directly, which is cheap and exact.  Every stock Renegade level  *
 * has no heightfield at all, and there the ground is authored W3D geometry, so a ray is        *
 * dropped through the physics scene to find it.                                                *
 *                                                                                             *
 * Neither existing is a real answer too -- a road built by a tool or a check with no world      *
 * around it keeps the height it was authored with, which is why this reports rather than       *
 * guesses.                                                                                     *
 *=============================================================================================*/
bool RoadSystem::Conform_Point(float x,float y,float * height_out,Vector3 * normal_out)
{
	if (WorldTerrainSystem::Has_Terrain()) {
		float height = 0.0f;
		if (WorldTerrainSystem::Sample_Height(x,y,&height)) {
			if (height_out != nullptr) { *height_out = height; }
			if (normal_out != nullptr) {
				Vector3 normal(0.0f,0.0f,1.0f);
				WorldTerrainSystem::Sample_Normal(x,y,&normal);
				*normal_out = normal;
			}
			return true;
		}
	}

	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
	if (scene == nullptr) {
		return false;
	}

	CastResultStruct result;
	result.ComputeContactPoint = true;
	LineSegClass ray(Vector3(x,y,ROAD_CONFORM_RAY_ABOVE),Vector3(x,y,-ROAD_CONFORM_RAY_BELOW));
	PhysRayCollisionTestClass raytest(ray,&result,0,COLLISION_TYPE_PHYSICAL);

	if (!scene->Cast_Ray(raytest)) {
		return false;
	}

	float span = ROAD_CONFORM_RAY_ABOVE + ROAD_CONFORM_RAY_BELOW;
	if (height_out != nullptr) {
		*height_out = ROAD_CONFORM_RAY_ABOVE - (result.Fraction * span);
	}
	if (normal_out != nullptr) {
		Vector3 normal = result.Normal;
		if (normal.Length() < WWMATH_EPSILON) {
			normal.Set(0.0f,0.0f,1.0f);
		} else {
			normal.Normalize();
			if (normal.Z < 0.0f) { normal = -normal; }
		}
		*normal_out = normal;
	}
	return true;
}


/*
**	Sit one road's centre line on the ground.
**
**	A road that does not grade takes the ground's height exactly.  A road that grades takes the
**	ground's height smoothed along its own length, because the point of grading is that the road
**	is smoother than the landscape and the landscape is then brought up to it.  Doing it this
**	way rather than from authored heights means a road drawn as a flat line on a map still comes
**	out following the country, and a road drawn across a dip still fills the dip.
*/
void RoadSystem::Conform_Road(RoadSplineClass & road)
{
	int count = road.Get_Point_Count();
	if (count < 2) {
		return;
	}

	DynamicVectorClass<float> ground;
	DynamicVectorClass<bool> found;

	for (int i = 0; i < count; i++) {
		const RoadPointClass & point = road.Get_Point(i);
		float height = point.Position.Z;
		Vector3 normal(0.0f,0.0f,1.0f);
		bool ok = Conform_Point(point.Position.X,point.Position.Y,&height,&normal);
		ground.Add(height);
		found.Add(ok);
		road.Peek_Point(i).Normal = normal;
	}

	bool smooth = (road.Get_Grade_Policy() != ROAD_GRADE_NONE);

	for (int i = 0; i < count; i++) {

		if (!found[i]) {
			continue;					// nothing under this point; leave it where it was authored
		}

		float height = ground[i];

		if (smooth) {
			//	Average the ground over a fixed length of road either side.  A window in metres
			//	rather than in points means the smoothing is the same on a straight, where the
			//	points are far apart, as on a bend, where they are close together.
			float centre = road.Get_Point(i).Distance;
			float total = 0.0f;
			int used = 0;
			for (int j = 0; j < count; j++) {
				if (!found[j]) { continue; }
				if (WWMath::Fabs(road.Get_Point(j).Distance - centre) > ROAD_GRADE_SMOOTHING) { continue; }
				total += ground[j];
				used++;
			}
			if (used > 0) {
				height = total / (float)used;
			}
		}

		road.Peek_Point(i).Position.Z = height;
	}

	road.Update_Frames();
}


/*
**	Stop the surface where a bridge or a tunnel takes over.
**
**	The connection is recorded with the position and direction the road was going when it
**	stopped, which is what Section 20's bridge and the tunnel portals will attach to.  Until
**	those exist the effect visible in the world is a road that stops at the water's edge rather
**	than running into it, which is the correct thing for it to do.
*/
void RoadSystem::Apply_Connection_Trims(RoadSplineClass & road)
{
	if (road.Get_Point_Count() < 2) {
		return;
	}

	RoadConnectionClass & start = road.Get_Start_Connection();
	if (start.Type != ROAD_ENDPOINT_OPEN) {
		start.Position = road.Get_Point(0).Position;
		start.Direction = -road.Get_Point(0).Tangent;
	}

	RoadConnectionClass & end = road.Get_End_Connection();
	if (end.Type != ROAD_ENDPOINT_OPEN) {
		int last = road.Get_Point_Count() - 1;
		end.Position = road.Get_Point(last).Position;
		end.Direction = road.Get_Point(last).Tangent;
	}
}


/***********************************************************************************************
 * RoadSystem::Merge_Junction -- one crossing, or one more arm on a crossing already found      *
 *                                                                                             *
 * Three roads meeting at a point produce three pairwise crossings within a metre or two of     *
 * each other, and they are one junction.  So a crossing close enough to one already recorded   *
 * joins it and contributes its arms rather than making a second disc overlapping the first.    *
 *=============================================================================================*/
bool RoadSystem::Merge_Junction(const Vector3 & position,float radius,int road_a,int road_b)
{
	int target = -1;

	for (int i = 0; i < Junctions.Count(); i++) {
		Vector3 delta = Junctions[i].Position - position;
		delta.Z = 0.0f;
		float reach = (Junctions[i].Radius > radius) ? Junctions[i].Radius : radius;
		if (delta.Length() <= reach) {
			target = i;
			break;
		}
	}

	if (target < 0) {
		if (Junctions.Count() >= 1024) {
			return false;
		}
		RoadJunctionClass junction;
		junction.Position = position;
		junction.Radius = radius;
		Junctions.Add(junction);
		target = Junctions.Count() - 1;
	} else if (radius > Junctions[target].Radius) {
		Junctions[target].Radius = radius;
	}

	int ids[2];
	ids[0] = road_a;
	ids[1] = road_b;
	for (int n = 0; n < 2; n++) {
		if (Junctions[target].Has_Arm(ids[n])) { continue; }
		if (Junctions[target].ArmCount >= RoadJunctionClass::MAX_ARMS) { continue; }
		Junctions[target].Arms[Junctions[target].ArmCount] = ids[n];
		Junctions[target].ArmCount++;
	}

	return true;
}


/*
**	Where two line segments cross, in plan.  Returns false when they are parallel or when the
**	crossing is outside either segment.
*/
static bool Segments_Cross(const Vector3 & a0,const Vector3 & a1,
									const Vector3 & b0,const Vector3 & b1,Vector3 * point_out)
{
	float ax = a1.X - a0.X;
	float ay = a1.Y - a0.Y;
	float bx = b1.X - b0.X;
	float by = b1.Y - b0.Y;

	float denom = (ax * by) - (ay * bx);
	if (WWMath::Fabs(denom) < WWMATH_EPSILON) {
		return false;
	}

	float dx = b0.X - a0.X;
	float dy = b0.Y - a0.Y;

	float ta = ((dx * by) - (dy * bx)) / denom;
	float tb = ((dx * ay) - (dy * ax)) / denom;

	if ((ta < 0.0f) || (ta > 1.0f) || (tb < 0.0f) || (tb > 1.0f)) {
		return false;
	}

	if (point_out != nullptr) {
		*point_out = a0 + ((a1 - a0) * ta);
	}
	return true;
}


/***********************************************************************************************
 * RoadSystem::Find_Junctions -- where the centre lines meet                                    *
 *                                                                                             *
 * Two ways for roads to meet.  They cross, which is a crossing of the centre lines and is      *
 * found by intersecting the segments; or one ends on the other, which is a tee and is not a    *
 * crossing at all -- the ending road stops on the through road rather than passing over it.    *
 * Both are looked for, because a tee found only by intersection would depend on the ending     *
 * road overshooting by a fraction of a metre.                                                  *
 *=============================================================================================*/
void RoadSystem::Find_Junctions(void)
{
	Junctions.Delete_All();

	int count = Roads.Count();

	for (int a = 0; a < count; a++) {

		if (!Roads[a].Is_Tessellated()) { continue; }

		for (int b = a + 1; b < count; b++) {

			if (!Roads[b].Is_Tessellated()) { continue; }

			float radius = Roads[a].Get_Width();
			if (Roads[b].Get_Width() > radius) { radius = Roads[b].Get_Width(); }
			radius = radius * 0.5f * ROAD_JUNCTION_RADIUS_SCALE;

			//	Crossings.
			for (int i = 0; i < Roads[a].Get_Point_Count() - 1; i++) {
				const Vector3 & a0 = Roads[a].Get_Point(i).Position;
				const Vector3 & a1 = Roads[a].Get_Point(i+1).Position;

				for (int j = 0; j < Roads[b].Get_Point_Count() - 1; j++) {
					Vector3 hit;
					if (Segments_Cross(a0,a1,Roads[b].Get_Point(j).Position,
											 Roads[b].Get_Point(j+1).Position,&hit)) {
						Merge_Junction(hit,radius,a,b);
					}
				}
			}

			//	Tees, in both directions.
			for (int pass = 0; pass < 2; pass++) {

				const RoadSplineClass & ending = (pass == 0) ? Roads[a] : Roads[b];
				const RoadSplineClass & through = (pass == 0) ? Roads[b] : Roads[a];
				float reach = (ending.Get_Width() * 0.5f) + (through.Get_Width() * 0.5f);

				for (int e = 0; e < 2; e++) {

					int index = (e == 0) ? 0 : (ending.Get_Point_Count() - 1);
					const Vector3 & tip = ending.Get_Point(index).Position;

					int closest = 0;
					float lateral = 0.0f;
					if (!through.Find_Closest_Point(tip,&closest,&lateral,nullptr)) { continue; }
					if (lateral > reach) { continue; }

					Merge_Junction(through.Get_Point(closest).Position,radius,a,b);
				}
			}
		}
	}
}


/***********************************************************************************************
 * RoadSystem::Stamp_Road_Mask -- tell the terrain where the roads are                          *
 *                                                                                             *
 * Section 18 built the road mask and left it with nothing to write into it; this is the writer *
 * it was waiting for.  The mask is stamped at the carriageway width and feathered across the   *
 * shoulder, which makes the shoulder a real thing rather than an unused field in the           *
 * representation: the terrain's road layer covers the carriageway and fades out across the     *
 * verge, so the road surface meets the ground through a band of worn dirt instead of an edge.  *
 *                                                                                             *
 * It also settles what a soldier is standing on.  Get_Surface_Type reads the dominant terrain  *
 * layer, so once the mask is stamped, footsteps on a road sound like a road.                   *
 *=============================================================================================*/
void RoadSystem::Stamp_Road_Mask(void)
{
	if (!TerrainTextureSystem::Has_Masks()) {
		return;
	}

	TerrainMaskClass * mask = TerrainTextureSystem::Peek_Mask(TERRAIN_MASK_ROAD);
	if ((mask == nullptr) || !mask->Is_Valid()) {
		return;
	}

	for (int r = 0; r < Roads.Count(); r++) {

		const RoadSplineClass & road = Roads[r];
		int count = road.Get_Point_Count();
		if (count < 2) { continue; }

		DynamicVectorClass<Vector3> line;
		for (int i = 0; i < count; i++) {
			line.Add(road.Get_Point(i).Position);
		}

		mask->Stamp_Polyline(&line[0],line.Count(),road.Get_Width(),1.0f,road.Get_Shoulder_Width());
	}
}


/***********************************************************************************************
 * RoadSystem::Apply_Grades -- bring the ground up to the roads that asked for it               *
 *                                                                                             *
 * The width graded is the carriageway plus both shoulders, because the shoulder is the part    *
 * that does the meeting: flattening only the carriageway leaves a step at its edge.            *
 *=============================================================================================*/
void RoadSystem::Apply_Grades(void)
{
	if (!WorldTerrainSystem::Has_Terrain()) {
		return;
	}

	for (int r = 0; r < Roads.Count(); r++) {

		const RoadSplineClass & road = Roads[r];
		if (road.Get_Grade_Policy() == ROAD_GRADE_NONE) { continue; }

		int count = road.Get_Point_Count();
		if (count < 2) { continue; }

		float blend = (road.Get_Grade_Policy() == ROAD_GRADE_FLATTEN) ? 1.0f : 0.5f;

		DynamicVectorClass<Vector3> line;
		for (int i = 0; i < count; i++) {
			line.Add(road.Get_Point(i).Position);
		}

		WorldTerrainSystem::Apply_Road_Grade(&line[0],line.Count(),road.Get_Total_Width(),blend);
	}
}


/***********************************************************************************************
 * RoadSystem::Build_Network -- everything about roads that does not need a device              *
 *=============================================================================================*/
bool RoadSystem::Build_Network(float max_deviation,float min_segment,float max_segment)
{
	Destroy_Geometry();
	Junctions.Delete_All();
	_RunPoints.Delete_All();
	_Runs.Delete_All();
	_Boundaries.Delete_All();
	NetworkBuilt = false;

	if (Roads.Count() == 0) {
		return false;
	}

	bool any = false;
	bool graded = false;

	for (int r = 0; r < Roads.Count(); r++) {

		if (!Roads[r].Tessellate(max_deviation,min_segment,max_segment)) {
			continue;
		}

		Conform_Road(Roads[r]);
		Apply_Connection_Trims(Roads[r]);
		any = true;

		if (Roads[r].Get_Grade_Policy() != ROAD_GRADE_NONE) {
			graded = true;
		}
	}

	if (!any) {
		return false;
	}

	if (graded) {
		Apply_Grades();

		//	The ground moved.  Every road that follows the ground rather than shaping it is now
		//	sitting where the ground used to be, and the collision the terrain built is too.
		for (int r = 0; r < Roads.Count(); r++) {
			if (Roads[r].Get_Grade_Policy() != ROAD_GRADE_NONE) { continue; }
			if (!Roads[r].Is_Tessellated()) { continue; }
			Conform_Road(Roads[r]);
		}

		if (WorldTerrainSystem::Has_Collision()) {
			WorldTerrainSystem::Update_Collision();
		}
	}

	Find_Junctions();
	Stamp_Road_Mask();

	NetworkBuilt = true;
	return true;
}


/*
**	Is this place inside a junction, and if so which one.
*/
static int Junction_At(const Vector3 & position)
{
	for (int i = 0; i < RoadSystem::Get_Junction_Count(); i++) {
		const RoadJunctionClass * junction = RoadSystem::Peek_Junction(i);
		float dx = position.X - junction->Position.X;
		float dy = position.Y - junction->Position.Y;
		if (((dx * dx) + (dy * dy)) <= (junction->Radius * junction->Radius)) {
			return i;
		}
	}
	return -1;
}


/*
**	The point where the straight line between two road points crosses a junction's edge.
**
**	Everything about the crossing point is interpolated from its neighbours and then put back on
**	the ground, so the ribbon that stops here and the junction polygon that starts here are
**	built from one position rather than two that nearly agree.
*/
static bool Junction_Crossing(const RoadPointClass & inside,const RoadPointClass & outside,
										const RoadJunctionClass & junction,RoadPointClass * point_out)
{
	Vector3 span = outside.Position - inside.Position;
	float t_low = 0.0f;
	float t_high = 1.0f;

	//	Bisection rather than the quadratic: the radius test is the same one Junction_At uses, so
	//	the point found here is on the same side of the same boundary, which a separately derived
	//	root is not guaranteed to be.
	for (int step = 0; step < 20; step++) {
		float t_mid = 0.5f * (t_low + t_high);
		Vector3 at = inside.Position + (span * t_mid);
		float dx = at.X - junction.Position.X;
		float dy = at.Y - junction.Position.Y;
		if (((dx * dx) + (dy * dy)) <= (junction.Radius * junction.Radius)) {
			t_low = t_mid;
		} else {
			t_high = t_mid;
		}
	}

	RoadPointClass point = outside;
	point.Position = inside.Position + (span * t_high);
	point.Distance = inside.Distance + ((outside.Distance - inside.Distance) * t_high);

	float height = point.Position.Z;
	Vector3 normal(0.0f,0.0f,1.0f);
	if (RoadSystem::Conform_Point(point.Position.X,point.Position.Y,&height,&normal)) {
		point.Position.Z = height;
		point.Normal = normal;
	}

	if (point_out != nullptr) {
		*point_out = point;
	}
	return true;
}


/*
**	Cut every road into the stretches that are not inside a junction, and record where each one
**	crossed a junction's edge on the way in or out.
*/
static void Collect_Geometry_Inputs(void)
{
	_RunPoints.Delete_All();
	_Runs.Delete_All();
	_Boundaries.Delete_All();

	for (int r = 0; r < RoadSystem::Get_Road_Count(); r++) {

		RoadSplineClass * road = RoadSystem::Peek_Road(r);
		if ((road == nullptr) || !road->Is_Tessellated()) { continue; }

		int count = road->Get_Point_Count();
		int run_first = -1;
		int run_count = 0;
		int previous_junction = -1;

		for (int i = 0; i < count; i++) {

			const RoadPointClass & point = road->Get_Point(i);
			int junction = Junction_At(point.Position);

			if (junction >= 0) {

				if (run_count > 0) {
					//	Leaving the open air.  Close the run on the junction's edge.
					RoadPointClass edge;
					Junction_Crossing(point,road->Get_Point(i-1),
											*RoadSystem::Peek_Junction(junction),&edge);
					_RunPoints.Add(edge);
					run_count++;

					RoadBoundaryClass boundary;
					boundary.JunctionIndex = junction;
					boundary.RoadID = r;
					boundary.Point = edge;
					_Boundaries.Add(boundary);

					RoadRunClass run;
					run.RoadID = r;
					run.First = run_first;
					run.Count = run_count;
					_Runs.Add(run);
					run_first = -1;
					run_count = 0;
				}
				previous_junction = junction;
				continue;
			}

			if (run_count == 0) {
				run_first = _RunPoints.Count();

				if (previous_junction >= 0) {
					//	Coming out of a junction.  Open the run on its edge.
					RoadPointClass edge;
					Junction_Crossing(road->Get_Point(i-1),point,
											*RoadSystem::Peek_Junction(previous_junction),&edge);
					_RunPoints.Add(edge);
					run_count++;

					RoadBoundaryClass boundary;
					boundary.JunctionIndex = previous_junction;
					boundary.RoadID = r;
					boundary.Point = edge;
					_Boundaries.Add(boundary);
				}
			}

			_RunPoints.Add(point);
			run_count++;
			previous_junction = -1;
		}

		if (run_count >= 2) {
			RoadRunClass run;
			run.RoadID = r;
			run.First = run_first;
			run.Count = run_count;
			_Runs.Add(run);
		}
	}
}


/*
**	The ribbon for one stretch, as a triangle strip of left and right edges.
*/
static void Emit_Run(DynamicMeshBuilderClass & builder,const RoadRunClass & run,float offset)
{
	RoadSplineClass * road = RoadSystem::Peek_Road(run.RoadID);
	if (road == nullptr) { return; }

	float tile = road->Get_Meters_Per_Tile();
	if (!(tile > 0.0f)) { tile = 8.0f; }

	builder.Begin_Strip();

	for (int n = 0; n < run.Count; n++) {

		const RoadPointClass & point = _RunPoints[run.First + n];
		Vector3 across = point.Side * point.HalfWidth;
		Vector3 lift(0.0f,0.0f,offset);
		float v = point.Distance / tile;

		builder.Vertex(point.Position + across + lift,0.0f,v);
		builder.Vertex(point.Position - across + lift,1.0f,v);
	}
}


/*
**	Sorting the corners of a junction polygon into the order they go round it.
*/
class RoadCornerClass
{
public:
	RoadCornerClass(void) : Angle(0.0f), Position(0.0f,0.0f,0.0f) { }

	bool operator == (const RoadCornerClass & that) const
	{
		return (Angle == that.Angle) && (Position == that.Position);
	}
	bool operator != (const RoadCornerClass & that) const	{ return !(*this == that); }

	float					Angle;
	Vector3				Position;
};


/*
**	The polygon that fills a junction.
**
**	Its corners are the edges of the roads that arrive, so it is exactly as wide as each road
**	where each road meets it and it takes whatever shape the arms happen to make.  Sorting them
**	by the angle they sit at around the middle and fanning through the centre fills the gaps
**	between the arms as well as the arms themselves, which is what makes a tee a tee and a
**	crossroads a crossroads without either being a case in the code.
*/
static void Emit_Junction(DynamicMeshBuilderClass & builder,int junction_index,float offset)
{
	const RoadJunctionClass * junction = RoadSystem::Peek_Junction(junction_index);
	if (junction == nullptr) { return; }

	DynamicVectorClass<RoadCornerClass> corners;
	RoadPointClass reference;
	bool have_reference = false;

	for (int b = 0; b < _Boundaries.Count(); b++) {

		if (_Boundaries[b].JunctionIndex != junction_index) { continue; }

		const RoadPointClass & point = _Boundaries[b].Point;
		if (!have_reference) {
			reference = point;
			have_reference = true;
		}

		Vector3 across = point.Side * point.HalfWidth;
		for (int side = 0; side < 2; side++) {
			RoadCornerClass corner;
			corner.Position = (side == 0) ? (point.Position + across) : (point.Position - across);
			corner.Angle = WWMath::Atan2(corner.Position.Y - junction->Position.Y,
												  corner.Position.X - junction->Position.X);
			corners.Add(corner);
		}
	}

	if (!have_reference || (corners.Count() < 3)) {
		return;
	}

	//	Insertion sort by angle.  The list is at most twice the arm count, so this is a dozen
	//	entries at the very worst.
	for (int i = 1; i < corners.Count(); i++) {
		RoadCornerClass key = corners[i];
		int j = i - 1;
		while ((j >= 0) && (corners[j].Angle > key.Angle)) {
			corners[j+1] = corners[j];
			j--;
		}
		corners[j+1] = key;
	}

	RoadSplineClass * road = RoadSystem::Peek_Road(junction->Arms[0]);
	float tile = (road != nullptr) ? road->Get_Meters_Per_Tile() : 8.0f;
	if (!(tile > 0.0f)) { tile = 8.0f; }
	float width = (reference.HalfWidth > 0.0f) ? (reference.HalfWidth * 2.0f) : 1.0f;

	Vector3 lift(0.0f,0.0f,offset);
	Vector3 centre = junction->Position;
	float height = centre.Z;
	Vector3 normal(0.0f,0.0f,1.0f);
	if (RoadSystem::Conform_Point(centre.X,centre.Y,&height,&normal)) {
		centre.Z = height;
	}

	//	Texture coordinates come from the frame of one arm, so the junction's texture runs the
	//	same way as at least one road that meets it rather than at an angle to all of them.
	builder.Begin_Fan();

	Vector3 middle_delta = centre - reference.Position;
	builder.Vertex(centre + lift,
						0.5f + (Vector3::Dot_Product(middle_delta,reference.Side) / width),
						(reference.Distance + Vector3::Dot_Product(middle_delta,reference.Tangent)) / tile);

	for (int i = 0; i <= corners.Count(); i++) {

		const RoadCornerClass & corner = corners[i % corners.Count()];
		Vector3 position = corner.Position;
		float corner_height = position.Z;
		Vector3 corner_normal(0.0f,0.0f,1.0f);
		if (RoadSystem::Conform_Point(position.X,position.Y,&corner_height,&corner_normal)) {
			position.Z = corner_height;
		}

		Vector3 delta = position - reference.Position;
		builder.Vertex(position + lift,
							0.5f + (Vector3::Dot_Product(delta,reference.Side) / width),
							(reference.Distance + Vector3::Dot_Product(delta,reference.Tangent)) / tile);
	}
}


/*
**	Everything in one material, counted or built depending on the builder.
*/
static void Emit_Material(DynamicMeshBuilderClass & builder,const StringClass & material,float offset)
{
	for (int i = 0; i < _Runs.Count(); i++) {
		RoadSplineClass * road = RoadSystem::Peek_Road(_Runs[i].RoadID);
		if (road == nullptr) { continue; }
		if (material != road->Get_Material()) { continue; }
		if (_Runs[i].Count < 2) { continue; }
		Emit_Run(builder,_Runs[i],offset);
	}

	for (int j = 0; j < RoadSystem::Get_Junction_Count(); j++) {
		const RoadJunctionClass * junction = RoadSystem::Peek_Junction(j);
		if ((junction == nullptr) || (junction->ArmCount == 0)) { continue; }

		//	A junction belongs to the batch of its first arm.  Two roads of different materials
		//	crossing have to make one polygon in one of them, and the alternative -- two coplanar
		//	polygons, one per material -- is the depth fight the junction exists to prevent.
		RoadSplineClass * road = RoadSystem::Peek_Road(junction->Arms[0]);
		if (road == nullptr) { continue; }
		if (material != road->Get_Material()) { continue; }

		Emit_Junction(builder,j,offset);
	}
}


/***********************************************************************************************
 * RoadSystem::Build_Geometry -- one mesh per material out of all the roads                     *
 *=============================================================================================*/
bool RoadSystem::Build_Geometry(void)
{
	Destroy_Geometry();

	if (!NetworkBuilt) {
		return false;
	}

	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
	if (scene == nullptr) {
		if (!_ReportedNoScene) {
			_ReportedNoScene = true;
			WWDEBUG_SAY(("RoadSystem: no physics scene, so the road meshes have nowhere to live.\n"));
		}
		return false;
	}

	Collect_Geometry_Inputs();

	//	The distinct materials across all the roads.  Section 19 asks for material batching and
	//	this is what it is: the number of things to draw is the number of road textures in use,
	//	not the number of roads and not the number of segments.
	DynamicVectorClass<StringClass> materials;
	for (int r = 0; r < Roads.Count(); r++) {
		if (!Roads[r].Is_Tessellated()) { continue; }
		StringClass name = Roads[r].Get_Material();
		bool known = false;
		for (int m = 0; m < materials.Count(); m++) {
			if (materials[m] == name) { known = true; break; }
		}
		if (!known) {
			materials.Add(name);
		}
	}

	for (int m = 0; m < materials.Count(); m++) {

		DynamicMeshBuilderClass builder;

		builder.Begin_Count();
		Emit_Material(builder,materials[m],SurfaceOffset);
		if (builder.Get_Poly_Count() <= 0) {
			continue;
		}

		if (!builder.Begin_Build()) {
			continue;
		}
		Emit_Material(builder,materials[m],SurfaceOffset);

		DynamicMeshClass * mesh = builder.Detach_Mesh();
		if (mesh == nullptr) {
			continue;
		}

		mesh->Set_Shader(ShaderClass::_PresetOpaqueShader);

		VertexMaterialClass * vmat = VertexMaterialClass::Get_Preset(VertexMaterialClass::PRELIT_DIFFUSE);
		if (vmat != nullptr) {
			mesh->Set_Vertex_Material(vmat,true);
			vmat->Release_Ref();
		}

		//	A road with no texture named is a road whose art has not been chosen, which is the
		//	state a generator is in before anybody dresses it and the state a check is in
		//	always.  It still has geometry and the geometry is still what is being built.
		if ((materials[m].Get_Length() > 0) && (WW3DAssetManager::Get_Instance() != nullptr)) {
			TextureClass * texture = WW3DAssetManager::Get_Instance()->Get_Texture(materials[m].Peek_Buffer());
			if (texture != nullptr) {
				mesh->Set_Texture(texture,true);
				texture->Release_Ref();
			}
		}

		//	Nothing collides with a road.  The ground under it already does, and a second
		//	surface a few centimetres above the first is a thing to trip on.
		mesh->Set_Collision_Type(0);
		mesh->Set_Dirty();

		StaticPhysClass * phys = new StaticPhysClass;
		phys->Set_Model(mesh);
		phys->Set_Transform(Matrix3D(1));
		mesh->Release_Ref();

		scene->Add_Static_Object(phys);
		Batches.Add(phys);
		BatchMaterials.Add(materials[m]);
	}

	return (Batches.Count() > 0);
}


void RoadSystem::Destroy_Geometry(void)
{
	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();

	for (int i = 0; i < Batches.Count(); i++) {
		if (Batches[i] == nullptr) { continue; }
		if (scene != nullptr) {
			scene->Remove_Object(Batches[i]);
		}
		Batches[i]->Release_Ref();
		Batches[i] = nullptr;
	}

	Batches.Delete_All();
	BatchMaterials.Delete_All();
}


bool RoadSystem::Has_Geometry(void)
{
	return (Batches.Count() > 0);
}


int RoadSystem::Get_Batch_Count(void)
{
	return Batches.Count();
}


RenderObjClass * RoadSystem::Peek_Batch_Model(int index)
{
	if ((index < 0) || (index >= Batches.Count()) || (Batches[index] == nullptr)) {
		return nullptr;
	}
	return Batches[index]->Peek_Model();
}


const char * RoadSystem::Get_Batch_Material(int index)
{
	if ((index < 0) || (index >= BatchMaterials.Count())) {
		return nullptr;
	}
	return BatchMaterials[index].Peek_Buffer();
}


void RoadSystem::Collect_Surface_Runs(void)
{
	Collect_Geometry_Inputs();
}


int RoadSystem::Get_Surface_Run_Count(void)
{
	return _Runs.Count();
}


bool RoadSystem::Get_Surface_Run(int index,int * road_id_out,int * point_count_out)
{
	if ((index < 0) || (index >= _Runs.Count())) {
		return false;
	}
	if (road_id_out != nullptr)		*road_id_out = _Runs[index].RoadID;
	if (point_count_out != nullptr)	*point_count_out = _Runs[index].Count;
	return true;
}


const RoadPointClass * RoadSystem::Peek_Surface_Run_Point(int index,int point)
{
	if ((index < 0) || (index >= _Runs.Count())) {
		return nullptr;
	}
	if ((point < 0) || (point >= _Runs[index].Count)) {
		return nullptr;
	}
	return &_RunPoints[_Runs[index].First + point];
}


int RoadSystem::Get_Surface_Boundary_Count(void)
{
	return _Boundaries.Count();
}


/***********************************************************************************************
 * RoadSystem::Find_Nearest_Road -- the navigation question                                     *
 *=============================================================================================*/
bool RoadSystem::Find_Nearest_Road(const Vector3 & position,float max_distance,
											  RoadNavInfoClass * info_out)
{
	RoadNavInfoClass best;
	bool have_best = false;

	for (int r = 0; r < Roads.Count(); r++) {

		const RoadSplineClass & road = Roads[r];
		if (!road.Is_Tessellated()) { continue; }

		int index = 0;
		float lateral = 0.0f;
		float along = 0.0f;
		if (!road.Find_Closest_Point(position,&index,&lateral,&along)) { continue; }
		if ((max_distance > 0.0f) && (lateral > max_distance)) { continue; }
		if (have_best && (lateral >= best.LateralDistance)) { continue; }

		have_best = true;
		best.RoadID = r;
		best.PointIndex = index;
		best.RoadClass = road.Get_Road_Class();
		best.LateralDistance = lateral;
		best.DistanceAlong = along;
		best.Position = road.Get_Point(index).Position;
		best.Tangent = road.Get_Point(index).Tangent;
		best.IsOnRoad = (lateral <= (road.Get_Width() * 0.5f));
	}

	if (!have_best) {
		return false;
	}

	if (info_out != nullptr) {
		*info_out = best;
	}
	return true;
}


bool RoadSystem::Is_On_Road(const Vector3 & position)
{
	RoadNavInfoClass info;
	if (!Find_Nearest_Road(position,0.0f,&info)) {
		return false;
	}
	return info.IsOnRoad;
}
