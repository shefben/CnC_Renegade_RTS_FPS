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
 *                     $Archive:: /Commando/Code/Combat/worldspatialindex.h                   $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	The one place to ask what is near something.                                                *
 *                                                                                             *
 *	Roadmap Section 16 asks for a stable spatial query service because terrain, building        *
 *	placement, lights, foliage and Commander Mode all need to ask the same question, and        *
 *	none of them should answer it by walking every object in the world.                         *
 *                                                                                             *
 *	It is deliberately not a second world database.  The world already has one: the static      *
 *	and dynamic culling systems inside PhysicsSceneClass, which every physical object is        *
 *	registered with as it is created.  This class is the query surface over that, and the       *
 *	place where a query with no home today -- a sphere, a placement overlap -- gets one.        *
 *	Section 16's rule against importing PartitionManager as a second unrelated world            *
 *	database is the thing this file exists to keep.                                             *
 *                                                                                             *
 *	Two levels of answer, because there are two kinds of caller.  The physics-level queries     *
 *	return PhysClass objects and serve rendering, placement and lighting.  The game-object      *
 *	queries return PhysicalGameObj and serve area damage, proximity and AI, which is where      *
 *	the whole-world scans actually were.  The second is the first plus one cast: every          *
 *	PhysicalGameObj makes itself its physics object's observer when it is built.                *
 *                                                                                             *
 *	A query is a superset test, never the final one.  Culling boxes bound objects loosely,      *
 *	so a caller that cared about an exact distance still checks the exact distance -- it        *
 *	just checks it against a handful of candidates instead of every object in the level.        *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef WORLDSPATIALINDEX_H
#define WORLDSPATIALINDEX_H

#include "always.h"
#include "vector3.h"
#include "vector.h"

class AABoxClass;
class OBBoxClass;
class FrustumClass;
class PhysClass;
class PhysicalGameObj;
class PhysRayCollisionTestClass;

template <class T> class MultiListClass;
typedef MultiListClass<PhysClass> NonRefPhysListClass;

/*
**	The list a game-object query fills in.
**
**	Raw pointers rather than reference handles: game object deletion in this engine is
**	deferred to the end of the frame, so a pointer collected by a query stays good for the
**	loop that collected it.  That is the same guarantee the whole-world scans relied on.
*/
typedef DynamicVectorClass<PhysicalGameObj *> GameObjQueryListClass;


class WorldSpatialIndex
{
public:

	/*
	**	Which half of the world a query looks at.  Static objects are the level: terrain,
	**	buildings, props.  Dynamic objects are everything created at runtime, which is where
	**	every PhysicalGameObj lives.  A caller that knows it only wants one says so, because
	**	the static side of a large level is much the larger of the two.
	*/
	enum {
		QUERY_STATIC		= 0x01,
		QUERY_DYNAMIC		= 0x02,
		QUERY_ALL			= QUERY_STATIC | QUERY_DYNAMIC,
	};

	/*
	**	Physics-level queries.  Results are added to the caller's list, which is not cleared
	**	first, so several queries can accumulate into one.
	*/
	static void		Query_AABox(const AABoxClass & box,NonRefPhysListClass * list,int flags = QUERY_ALL);
	static void		Query_OBBox(const OBBoxClass & box,NonRefPhysListClass * list,int flags = QUERY_ALL);
	static void		Query_Sphere(const Vector3 & center,float radius,NonRefPhysListClass * list,int flags = QUERY_ALL);
	static void		Query_Frustum(const FrustumClass & frustum,NonRefPhysListClass * list,int flags = QUERY_ALL);
	static void		Query_Point(const Vector3 & point,NonRefPhysListClass * list,int flags = QUERY_ALL);

	/*
	**	Ray query.  The test object carries the ray, the collision group and the result, and
	**	is the same one the physics scene takes.  This entry point exists so that a caller
	**	asking a spatial question does not have to know it is talking to the physics scene.
	*/
	static bool		Query_Ray(PhysRayCollisionTestClass & raytest);

	/*
	**	Would a building placed here be standing in something?  The Commander's ghost
	**	placement is the caller Section 16 names; the box is the footprint in world space.
	*/
	static bool		Query_Placement_Overlap(const AABoxClass & box,int collision_group);
	static bool		Query_Placement_Overlap(const OBBoxClass & box,int collision_group);

	/*
	**	Lights near a point or a volume, for the small relevant light sets Section 16 asks
	**	for.  Both kinds: the static lights come from the scene's AABTree, the dynamic ones
	**	from WorldLightManager's grid, and neither answer is found by walking a light list.
	*/
	static void		Query_Nearby_Lights(const Vector3 & point,NonRefPhysListClass * list);
	static void		Query_Nearby_Lights(const AABoxClass & bounds,NonRefPhysListClass * list);

	/*
	**	Game-object queries.  Same structures, one cast further: an object is included when
	**	its physics object was collected and it is a PhysicalGameObj.  Objects without a
	**	physics object -- script zones above all -- are not in a culling system and cannot be
	**	found this way, which is exactly the set the old scans skipped by hand.
	*/
	static void		Query_Game_Objects(const AABoxClass & box,GameObjQueryListClass & results,int flags = QUERY_ALL);
	static void		Query_Game_Objects(const OBBoxClass & box,GameObjQueryListClass & results,int flags = QUERY_ALL);
	static void		Query_Game_Objects_In_Sphere(const Vector3 & center,float radius,GameObjQueryListClass & results,int flags = QUERY_ALL);

private:

	static void		To_Game_Objects(NonRefPhysListClass & list,GameObjQueryListClass & results);
};

#endif	// WORLDSPATIALINDEX_H
