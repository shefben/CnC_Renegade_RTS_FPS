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
 *                     $Archive:: /Commando/Code/Combat/worldspatialindex.cpp                 $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "worldspatialindex.h"

#include "aabox.h"
#include "combatphysobserver.h"
#include "obbox.h"
#include "physcoltest.h"
#include "physicalgameobj.h"
#include "physlist.h"
#include "pscene.h"
#include "wwdebug.h"


/***********************************************************************************************
 * WorldSpatialIndex::Query_Point -- objects whose bounds contain a point                      *
 *=============================================================================================*/
void WorldSpatialIndex::Query_Point(const Vector3 & point,NonRefPhysListClass * list,int flags)
{
	WWASSERT(list != nullptr);

	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
	if (scene == nullptr) {
		return;
	}

	scene->Collect_Objects(point,(flags & QUERY_STATIC) != 0,(flags & QUERY_DYNAMIC) != 0,list);
}


/***********************************************************************************************
 * WorldSpatialIndex::Query_AABox -- objects whose bounds overlap an axis aligned box          *
 *=============================================================================================*/
void WorldSpatialIndex::Query_AABox(const AABoxClass & box,NonRefPhysListClass * list,int flags)
{
	WWASSERT(list != nullptr);

	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
	if (scene == nullptr) {
		return;
	}

	scene->Collect_Objects(box,(flags & QUERY_STATIC) != 0,(flags & QUERY_DYNAMIC) != 0,list);
}


/***********************************************************************************************
 * WorldSpatialIndex::Query_OBBox -- objects whose bounds overlap an oriented box              *
 *=============================================================================================*/
void WorldSpatialIndex::Query_OBBox(const OBBoxClass & box,NonRefPhysListClass * list,int flags)
{
	WWASSERT(list != nullptr);

	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
	if (scene == nullptr) {
		return;
	}

	scene->Collect_Objects(box,(flags & QUERY_STATIC) != 0,(flags & QUERY_DYNAMIC) != 0,list);
}


/***********************************************************************************************
 * WorldSpatialIndex::Query_Frustum -- objects whose bounds overlap a frustum                  *
 *=============================================================================================*/
void WorldSpatialIndex::Query_Frustum(const FrustumClass & frustum,NonRefPhysListClass * list,int flags)
{
	WWASSERT(list != nullptr);

	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
	if (scene == nullptr) {
		return;
	}

	scene->Collect_Objects(frustum,(flags & QUERY_STATIC) != 0,(flags & QUERY_DYNAMIC) != 0,list);
}


/***********************************************************************************************
 * WorldSpatialIndex::Query_Sphere -- objects whose bounds overlap a sphere                    *
 *                                                                                             *
 * The culling systems index axis aligned boxes, so a sphere query is the sphere's box.  That  *
 * over-collects at the corners, and deliberately so: the exact test belongs to the caller,    *
 * who almost always has a more specific one than "is the bounding box within r" -- distance   *
 * to a bullseye, an arc, a line of sight.  Rejecting corner cases here would cost a second    *
 * test for every candidate and change no answer.                                              *
 *=============================================================================================*/
void WorldSpatialIndex::Query_Sphere(const Vector3 & center,float radius,NonRefPhysListClass * list,int flags)
{
	WWASSERT(list != nullptr);
	WWASSERT(radius >= 0.0f);

	AABoxClass box(center,Vector3(radius,radius,radius));
	Query_AABox(box,list,flags);
}


/***********************************************************************************************
 * WorldSpatialIndex::Query_Ray -- what a ray hits                                             *
 *=============================================================================================*/
bool WorldSpatialIndex::Query_Ray(PhysRayCollisionTestClass & raytest)
{
	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
	if (scene == nullptr) {
		return false;
	}

	return scene->Cast_Ray(raytest);
}


/***********************************************************************************************
 * WorldSpatialIndex::Query_Placement_Overlap -- is a footprint standing in something           *
 *=============================================================================================*/
bool WorldSpatialIndex::Query_Placement_Overlap(const AABoxClass & box,int collision_group)
{
	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
	if (scene == nullptr) {
		return false;
	}

	return scene->Intersection_Test(box,collision_group,COLLISION_TYPE_PHYSICAL);
}


bool WorldSpatialIndex::Query_Placement_Overlap(const OBBoxClass & box,int collision_group)
{
	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
	if (scene == nullptr) {
		return false;
	}

	return scene->Intersection_Test(box,collision_group,COLLISION_TYPE_PHYSICAL);
}


/***********************************************************************************************
 * WorldSpatialIndex::Query_Nearby_Lights -- the lights that can matter here                   *
 *=============================================================================================*/
void WorldSpatialIndex::Query_Nearby_Lights(const Vector3 & point,NonRefPhysListClass * list)
{
	WWASSERT(list != nullptr);

	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
	if (scene == nullptr) {
		return;
	}

	scene->Collect_Lights(point,true,true,list);
}


void WorldSpatialIndex::Query_Nearby_Lights(const AABoxClass & bounds,NonRefPhysListClass * list)
{
	WWASSERT(list != nullptr);

	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
	if (scene == nullptr) {
		return;
	}

	scene->Collect_Lights(bounds,true,true,list);
}


/***********************************************************************************************
 * WorldSpatialIndex::To_Game_Objects -- collected physics objects to the game objects on them *
 *=============================================================================================*/
void WorldSpatialIndex::To_Game_Objects(NonRefPhysListClass & list,GameObjQueryListClass & results)
{
	NonRefPhysListIterator it(&list);
	for (it.First(); !it.Is_Done(); it.Next()) {

		PhysClass * phys_obj = it.Peek_Obj();
		if (phys_obj == nullptr) {
			continue;
		}

		//	Every observer a physics object can have in this game is a CombatPhysObserverClass;
		//	the base class has no As_ methods to ask with, which is why the cast is unchecked
		//	here as it is everywhere else that walks a collected list.
		CombatPhysObserverClass * observer = reinterpret_cast<CombatPhysObserverClass *>(phys_obj->Get_Observer());
		if (observer == nullptr) {
			continue;
		}

		PhysicalGameObj * game_obj = observer->As_PhysicalGameObj();
		if (game_obj != nullptr) {
			results.Add(game_obj);
		}
	}
}


/***********************************************************************************************
 * WorldSpatialIndex::Query_Game_Objects -- game objects overlapping a box                     *
 *=============================================================================================*/
void WorldSpatialIndex::Query_Game_Objects(const AABoxClass & box,GameObjQueryListClass & results,int flags)
{
	NonRefPhysListClass phys_list;
	Query_AABox(box,&phys_list,flags);
	To_Game_Objects(phys_list,results);
}


void WorldSpatialIndex::Query_Game_Objects(const OBBoxClass & box,GameObjQueryListClass & results,int flags)
{
	NonRefPhysListClass phys_list;
	Query_OBBox(box,&phys_list,flags);
	To_Game_Objects(phys_list,results);
}


/***********************************************************************************************
 * WorldSpatialIndex::Query_Game_Objects_In_Sphere -- game objects near a point                 *
 *=============================================================================================*/
void WorldSpatialIndex::Query_Game_Objects_In_Sphere
(
	const Vector3 & center,
	float radius,
	GameObjQueryListClass & results,
	int flags
)
{
	NonRefPhysListClass phys_list;
	Query_Sphere(center,radius,&phys_list,flags);
	To_Game_Objects(phys_list,results);
}
