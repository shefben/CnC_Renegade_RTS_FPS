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
 *                     $Archive:: /Commando/Code/wwphys/worldlightmanager.cpp                 $*
 *                                                                                             *
 *	Roadmap Section 25, Zero Hour / SAGE feature 11 -- dynamic-light filtering.                  *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "worldlightmanager.h"

#include "light.h"
#include "lightenvironment.h"
#include "lightphys.h"
#include "phys.h"
#include "pscene.h"
#include "rendobj.h"
#include "wwdebug.h"
#include "wwmath.h"
#include "wwmemlog.h"


/*
**	How big a dynamic light may be.  This is not a taste limit, it is the grid's limit: a query
**	expands its volume by the largest object the grid holds, so allowing a light of any radius
**	would make every query visit every cell within that radius and the filtering would filter
**	nothing.  Sixty metres covers an explosion; a light asking for more is clamped and counted.
*/
const float WORLD_LIGHT_MAX_RADIUS			= 60.0f;

/*
**	Cells are at least one maximum light across, so a point query visits at most three cells on
**	each axis whatever the world is sized at.
*/
const float WORLD_LIGHT_MIN_CELL_SIZE		= WORLD_LIGHT_MAX_RADIUS;
const int	WORLD_LIGHT_MAX_GRID_CELLS		= 4096;

/*
**	How many objects may be given a composed lighting environment in one frame.  Meshes on the
**	static sort list keep the pointer they were handed until the list is flushed, so these have
**	to stay alive for the whole frame and the pool is what makes that affordable.  An object
**	past the bound is drawn with its cached static lighting, which is the picture it had before
**	this system existed rather than a wrong one.
*/
const unsigned int WORLD_LIGHT_DEFAULT_LIT_OBJECTS	= 256;
const unsigned int WORLD_LIGHT_MAX_LIT_OBJECTS		= 1024;

/*
**	Where the grid goes when nobody has said how big the world is.  Only reached when a light is
**	made with no physics scene -- a test, or a tool.
*/
const float WORLD_LIGHT_DEFAULT_WORLD_EXTENT	= 1000.0f;


bool				WorldLightManager::Initialized			= false;
bool				WorldLightManager::Partitioned			= false;
unsigned int	WorldLightManager::MaxDynamicLights		= WORLD_LIGHT_MAX_DYNAMIC_LIGHTS;
unsigned int	WorldLightManager::MaxLitObjects			= WORLD_LIGHT_DEFAULT_LIT_OBJECTS;

TypedGridCullSystemClass<LightPhysClass> *	WorldLightManager::Grid = nullptr;
DynamicVectorClass<WorldLightManager::DynamicLightClass>	WorldLightManager::Lights;

AABoxClass		WorldLightManager::LightBounds(Vector3(0,0,0),Vector3(0,0,0));
bool				WorldLightManager::BoundsDirty			= true;

int				WorldLightManager::QueryCount				= 0;
int				WorldLightManager::LightsExamined		= 0;
int				WorldLightManager::LightsApplied			= 0;
int				WorldLightManager::LitObjectCount		= 0;
int				WorldLightManager::TrivialRejects		= 0;
int				WorldLightManager::Overflows				= 0;
int				WorldLightManager::Refusals				= 0;
int				WorldLightManager::ClampedRadii			= 0;

int				WorldLightManager::EnvironmentCount		= 0;
int				WorldLightManager::EnvironmentNext		= 0;
LightEnvironmentClass *	WorldLightManager::Environments	= nullptr;


WorldLightManager::DynamicLightClass::DynamicLightClass(void) :
	Light(nullptr),
	Life(-1.0f),
	MaxLife(-1.0f),
	BaseIntensity(1.0f)
{
}


/***********************************************************************************************
 * WorldLightManager::Init -- bring the service up                                             *
 *=============================================================================================*/
void WorldLightManager::Init(void)
{
	if (Initialized) {
		Shutdown();
	}

	WWMEMLOG(MEM_RENDERER);

	Grid = new TypedGridCullSystemClass<LightPhysClass>;
	Grid->Set_Min_Cell_Size(Vector3(WORLD_LIGHT_MIN_CELL_SIZE,
											 WORLD_LIGHT_MIN_CELL_SIZE,
											 WORLD_LIGHT_MIN_CELL_SIZE));
	Grid->Set_Termination_Count(WORLD_LIGHT_MAX_GRID_CELLS);

	Partitioned			= false;
	MaxDynamicLights	= WORLD_LIGHT_MAX_DYNAMIC_LIGHTS;
	MaxLitObjects		= WORLD_LIGHT_DEFAULT_LIT_OBJECTS;

	LightBounds.Center.Set(0,0,0);
	LightBounds.Extent.Set(0,0,0);
	BoundsDirty = true;

	QueryCount = LightsExamined = LightsApplied = LitObjectCount = 0;
	TrivialRejects = Overflows = Refusals = ClampedRadii = 0;

	Size_Environment_Pool(MaxLitObjects);

	Initialized = true;
}


/***********************************************************************************************
 * WorldLightManager::Shutdown -- take the service down                                        *
 *=============================================================================================*/
void WorldLightManager::Shutdown(void)
{
	Remove_All_Dynamic_Lights();

	delete Grid;
	Grid = nullptr;

	delete [] Environments;
	Environments		= nullptr;
	EnvironmentCount	= 0;
	EnvironmentNext	= 0;

	MaxLitObjects		= 0;
	Partitioned			= false;
	Initialized			= false;
}


/***********************************************************************************************
 * WorldLightManager::Release_Resources -- the world went away, the lights in it go with it    *
 *=============================================================================================*/
void WorldLightManager::Release_Resources(void)
{
	Remove_All_Dynamic_Lights();

	/*
	**	The grid was partitioned for a world that no longer exists, so the next light asks the
	**	next world how big it is.
	*/
	Partitioned = false;
}


/***********************************************************************************************
 * WorldLightManager::Re_Partition -- say where the world is                                   *
 *=============================================================================================*/
void WorldLightManager::Re_Partition(const Vector3 & world_min,const Vector3 & world_max)
{
	if (Grid == nullptr) {
		return ;
	}

	Grid->Re_Partition(world_min,world_max,WORLD_LIGHT_MAX_RADIUS);
	Partitioned = true;
	BoundsDirty = true;
}


bool WorldLightManager::Is_Partitioned(void)
{
	return Partitioned;
}


/***********************************************************************************************
 * WorldLightManager::Ensure_Partitioned -- lazily size the grid to the level                  *
 *                                                                                             *
 * Nobody has to remember to call Re_Partition.  The first dynamic light of a world asks the    *
 * physics scene how big that world is; with no scene the grid takes a default box, which is    *
 * the case a tool or a self check runs in.                                                     *
 *=============================================================================================*/
void WorldLightManager::Ensure_Partitioned(void)
{
	if (Partitioned || (Grid == nullptr)) {
		return ;
	}

	Vector3 world_min(-WORLD_LIGHT_DEFAULT_WORLD_EXTENT,
							-WORLD_LIGHT_DEFAULT_WORLD_EXTENT,
							-WORLD_LIGHT_DEFAULT_WORLD_EXTENT);
	Vector3 world_max( WORLD_LIGHT_DEFAULT_WORLD_EXTENT,
							 WORLD_LIGHT_DEFAULT_WORLD_EXTENT,
							 WORLD_LIGHT_DEFAULT_WORLD_EXTENT);

	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
	if (scene != nullptr) {
		scene->Get_Level_Extents(world_min,world_max);
	}

	Re_Partition(world_min,world_max);
}


/***********************************************************************************************
 * WorldLightManager::Size_Environment_Pool -- resize the per-frame composed environments      *
 *=============================================================================================*/
void WorldLightManager::Size_Environment_Pool(unsigned int count)
{
	delete [] Environments;
	Environments		= nullptr;
	EnvironmentCount	= 0;
	EnvironmentNext	= 0;

	if (count > 0) {
		WWMEMLOG(MEM_RENDERER);
		Environments		= new LightEnvironmentClass[count];
		EnvironmentCount	= (int)count;
	}
}


void WorldLightManager::Set_Max_Dynamic_Lights(unsigned int count)
{
	if (count > WORLD_LIGHT_MAX_DYNAMIC_LIGHTS) {
		count = WORLD_LIGHT_MAX_DYNAMIC_LIGHTS;
	}
	MaxDynamicLights = count;

	/*
	**	Lowering the bound drops the oldest lights first; a light that is already burning is
	**	more likely to be the one somebody is looking at.
	*/
	while ((unsigned int)Lights.Count() > MaxDynamicLights) {
		Remove_Dynamic_Light(Lights[0].Light);
	}
}


unsigned int WorldLightManager::Get_Max_Dynamic_Lights(void)
{
	return MaxDynamicLights;
}


void WorldLightManager::Set_Max_Lit_Objects(unsigned int count)
{
	if (count > WORLD_LIGHT_MAX_LIT_OBJECTS) {
		count = WORLD_LIGHT_MAX_LIT_OBJECTS;
	}

	if (count == MaxLitObjects) {
		return ;
	}

	MaxLitObjects = count;
	Size_Environment_Pool(MaxLitObjects);
}


unsigned int WorldLightManager::Get_Max_Lit_Objects(void)
{
	return MaxLitObjects;
}


float WorldLightManager::Get_Max_Light_Radius(void)
{
	return WORLD_LIGHT_MAX_RADIUS;
}


/***********************************************************************************************
 * WorldLightManager::Find_Light -- which record is this light                                 *
 *=============================================================================================*/
int WorldLightManager::Find_Light(const LightPhysClass * light)
{
	for (int i = 0; i < Lights.Count(); i++) {
		if (Lights[i].Light == light) {
			return i;
		}
	}
	return -1;
}


/***********************************************************************************************
 * WorldLightManager::Create_Point_Light -- a flash of coloured light, somewhere               *
 *                                                                                             *
 * The caller does not have to know what a LightPhysClass is or that there is a grid.  The      *
 * returned pointer belongs to this table; it stays valid until the light expires or is         *
 * removed, and a caller that wants to hold it past that takes its own reference.               *
 *=============================================================================================*/
LightPhysClass * WorldLightManager::Create_Point_Light
(
	const Vector3 &	position,
	const Vector3 &	color,
	float					inner_radius,
	float					outer_radius,
	float					intensity,
	float					lifetime
)
{
	if (!Initialized) {
		return nullptr;
	}

	if ((unsigned int)Lights.Count() >= MaxDynamicLights) {
		Refusals++;
		return nullptr;
	}

	/*
	**	A light bigger than the grid holds would turn every query into a sweep of everything
	**	within its radius, so the radius is the thing that gives.
	*/
	if (outer_radius > WORLD_LIGHT_MAX_RADIUS) {
		outer_radius = WORLD_LIGHT_MAX_RADIUS;
		ClampedRadii++;
	}
	if (outer_radius <= 0.0f) {
		outer_radius = 1.0f;
	}
	if (inner_radius < 0.0f) {
		inner_radius = 0.0f;
	}
	if (inner_radius > outer_radius) {
		inner_radius = outer_radius;
	}

	WWMEMLOG(MEM_RENDERER);

	LightPhysClass * light = new LightPhysClass(true);
	LightClass * light_obj = (LightClass *)light->Peek_Model();
	WWASSERT(light_obj != nullptr);

	light_obj->Set_Diffuse(color);
	light_obj->Set_Specular(Vector3(0.0f,0.0f,0.0f));
	light_obj->Set_Ambient(Vector3(0.0f,0.0f,0.0f));
	light_obj->Set_Intensity(intensity);
	light_obj->Set_Far_Attenuation_Range(inner_radius,outer_radius);
	light_obj->Set_Flag(LightClass::FAR_ATTENUATION,true);

	/*
	**	Setting the transform is also what sizes the cull box, and the cull box is the light's
	**	attenuation volume, so this has to happen after the attenuation range is set.
	*/
	light->Set_Position(position);

	if (!Add_Dynamic_Light(light,lifetime)) {
		light->Release_Ref();
		return nullptr;
	}

	/*
	**	The table took its own reference; ours goes away.
	*/
	light->Release_Ref();
	return light;
}


/***********************************************************************************************
 * WorldLightManager::Add_Dynamic_Light -- take ownership of a light                           *
 *=============================================================================================*/
bool WorldLightManager::Add_Dynamic_Light(LightPhysClass * light,float lifetime)
{
	if (!Initialized || (light == nullptr) || (Grid == nullptr)) {
		return false;
	}

	if (light->Peek_Model() == nullptr) {
		WWDEBUG_SAY(("WorldLightManager: a dynamic light with no light object was refused\r\n"));
		Refusals++;
		return false;
	}

	if (Find_Light(light) >= 0) {
		return true;
	}

	if ((unsigned int)Lights.Count() >= MaxDynamicLights) {
		Refusals++;
		return false;
	}

	if (light->Get_Culling_System() != nullptr) {
		WWDEBUG_SAY(("WorldLightManager: a light already in a culling system was refused\r\n"));
		Refusals++;
		return false;
	}

	Ensure_Partitioned();

	DynamicLightClass record;
	record.Light			= light;
	record.MaxLife			= lifetime;
	record.Life				= lifetime;
	record.BaseIntensity	= ((LightClass *)light->Peek_Model())->Get_Intensity();

	light->Add_Ref();
	Lights.Add(record);
	Grid->Add_Object(light);

	BoundsDirty = true;
	return true;
}


/***********************************************************************************************
 * WorldLightManager::Remove_Dynamic_Light -- give a light back                                *
 *=============================================================================================*/
bool WorldLightManager::Remove_Dynamic_Light(LightPhysClass * light)
{
	int index = Find_Light(light);
	if (index < 0) {
		return false;
	}

	if ((Grid != nullptr) && (light->Get_Culling_System() == Grid)) {
		Grid->Remove_Object(light);
	}

	Lights.Delete(index);
	light->Release_Ref();

	BoundsDirty = true;
	return true;
}


bool WorldLightManager::Is_Dynamic_Light(const LightPhysClass * light)
{
	return (Find_Light(light) >= 0);
}


void WorldLightManager::Remove_All_Dynamic_Lights(void)
{
	while (Lights.Count() > 0) {
		Remove_Dynamic_Light(Lights[Lights.Count() - 1].Light);
	}
	Lights.Delete_All();
}


int WorldLightManager::Get_Dynamic_Light_Count(void)
{
	return Lights.Count();
}


/***********************************************************************************************
 * WorldLightManager::Move_Dynamic_Light -- put a light somewhere else                         *
 *                                                                                             *
 * Set_Transform already re-buckets the light in the grid.  This exists so that a caller that   *
 * moves one does not also have to know that a union bound is kept over all of them.            *
 *=============================================================================================*/
void WorldLightManager::Move_Dynamic_Light(LightPhysClass * light,const Vector3 & position)
{
	if ((light == nullptr) || (Find_Light(light) < 0)) {
		return ;
	}

	light->Set_Position(position);
	BoundsDirty = true;
}


/***********************************************************************************************
 * WorldLightManager::Timestep -- fade and expire the lights that were given a lifetime        *
 *=============================================================================================*/
void WorldLightManager::Timestep(float dt)
{
	if (!Initialized) {
		return ;
	}

	for (int i = Lights.Count() - 1; i >= 0; i--) {

		DynamicLightClass & record = Lights[i];
		if (record.MaxLife <= 0.0f) {
			continue;			// this one stays until somebody removes it
		}

		record.Life -= dt;

		if (record.Life <= 0.0f) {
			Remove_Dynamic_Light(record.Light);
			continue;
		}

		/*
		**	Fading the intensity rather than the attenuation range keeps the cull box the same
		**	size for the light's whole life, so a fading light never has to be re-bucketed.
		*/
		LightClass * light_obj = (LightClass *)record.Light->Peek_Model();
		if (light_obj != nullptr) {
			light_obj->Set_Intensity(record.BaseIntensity * (record.Life / record.MaxLife));
		}
	}
}


/***********************************************************************************************
 * WorldLightManager::Recompute_Bounds -- the box that holds every dynamic light               *
 *                                                                                             *
 * This is the gate that makes a per-object light query affordable.  An object outside this box *
 * cannot be reached by any dynamic light, and finding that out costs three comparisons.        *
 *=============================================================================================*/
void WorldLightManager::Recompute_Bounds(void)
{
	BoundsDirty = false;

	if (Lights.Count() == 0) {
		LightBounds.Center.Set(0,0,0);
		LightBounds.Extent.Set(0,0,0);
		return ;
	}

	Vector3 box_min(0,0,0);
	Vector3 box_max(0,0,0);

	for (int i = 0; i < Lights.Count(); i++) {

		const AABoxClass & box = Lights[i].Light->Get_Cull_Box();

		Vector3 obj_min = box.Center - box.Extent;
		Vector3 obj_max = box.Center + box.Extent;

		if (i == 0) {
			box_min = obj_min;
			box_max = obj_max;
		} else {
			box_min.X = WWMath::Min(box_min.X,obj_min.X);
			box_min.Y = WWMath::Min(box_min.Y,obj_min.Y);
			box_min.Z = WWMath::Min(box_min.Z,obj_min.Z);
			box_max.X = WWMath::Max(box_max.X,obj_max.X);
			box_max.Y = WWMath::Max(box_max.Y,obj_max.Y);
			box_max.Z = WWMath::Max(box_max.Z,obj_max.Z);
		}
	}

	LightBounds.Center = (box_max + box_min) * 0.5f;
	LightBounds.Extent = (box_max - box_min) * 0.5f;
}


bool WorldLightManager::Has_Dynamic_Lights(void)
{
	return (Lights.Count() > 0);
}


void WorldLightManager::Get_Light_Bounds(AABoxClass * set_bounds)
{
	if (set_bounds == nullptr) {
		return ;
	}

	if (BoundsDirty) {
		Recompute_Bounds();
	}
	*set_bounds = LightBounds;
}


/***********************************************************************************************
 * WorldLightManager::Gather -- the lights that actually reach a point                          *
 *                                                                                             *
 * The three steps are the whole of Section 25.  The union bound rejects a point no light can    *
 * reach for the price of three comparisons.  The grid narrows what is left to the cells around  *
 * the point, so the work is set by how many lights are near, not by how many exist.  And the    *
 * exact attenuation test is the caller's, because a grid query is a superset and never the      *
 * answer.                                                                                       *
 *=============================================================================================*/
int WorldLightManager::Gather(const Vector3 & point,LightPhysClass ** set,int max_set)
{
	if (!Initialized || (Grid == nullptr) || (Lights.Count() == 0) || (max_set <= 0)) {
		return 0;
	}

	if (BoundsDirty) {
		Recompute_Bounds();
	}

	Vector3 delta = point - LightBounds.Center;
	if (	(WWMath::Fabs(delta.X) > LightBounds.Extent.X) ||
			(WWMath::Fabs(delta.Y) > LightBounds.Extent.Y) ||
			(WWMath::Fabs(delta.Z) > LightBounds.Extent.Z) )
	{
		TrivialRejects++;
		return 0;
	}

	QueryCount++;

	Grid->Reset_Collection();
	Grid->Collect_Objects(point);

	int count = 0;
	for (	LightPhysClass * light = Grid->Get_First_Collected_Object();
			light != nullptr;
			light = Grid->Get_Next_Collected_Object(light) )
	{
		LightsExamined++;

		if (light->Is_Disabled()) {
			continue;
		}

		LightClass * light_obj = (LightClass *)light->Peek_Model();
		if (light_obj == nullptr) {
			continue;
		}

		if (!light_obj->Is_Within_Attenuation_Radius(point)) {
			continue;
		}

		if (count >= max_set) {
			break;
		}

		set[count++] = light;
	}

	return count;
}


/***********************************************************************************************
 * WorldLightManager::Collect_Dynamic_Lights -- the scene's light collection, dynamic half     *
 *=============================================================================================*/
int WorldLightManager::Collect_Dynamic_Lights(const Vector3 & point,NonRefPhysListClass * list)
{
	if (list == nullptr) {
		return 0;
	}

	LightPhysClass * set[WORLD_LIGHT_MAX_DYNAMIC_LIGHTS];
	int count = Gather(point,set,WORLD_LIGHT_MAX_DYNAMIC_LIGHTS);

	for (int i = 0; i < count; i++) {
		list->Add(set[i]);
	}

	return count;
}


int WorldLightManager::Collect_Dynamic_Lights(const AABoxClass & bounds,NonRefPhysListClass * list)
{
	if (!Initialized || (list == nullptr) || (Grid == nullptr) || (Lights.Count() == 0)) {
		return 0;
	}

	if (BoundsDirty) {
		Recompute_Bounds();
	}

	Vector3 delta = bounds.Center - LightBounds.Center;
	if (	(WWMath::Fabs(delta.X) > (LightBounds.Extent.X + bounds.Extent.X)) ||
			(WWMath::Fabs(delta.Y) > (LightBounds.Extent.Y + bounds.Extent.Y)) ||
			(WWMath::Fabs(delta.Z) > (LightBounds.Extent.Z + bounds.Extent.Z)) )
	{
		TrivialRejects++;
		return 0;
	}

	QueryCount++;

	Grid->Reset_Collection();
	Grid->Collect_Objects(bounds);

	int count = 0;
	for (	LightPhysClass * light = Grid->Get_First_Collected_Object();
			light != nullptr;
			light = Grid->Get_Next_Collected_Object(light) )
	{
		LightsExamined++;

		if (light->Is_Disabled()) {
			continue;
		}

		/*
		**	The grid collects by cell, so the exact overlap test is done here.
		*/
		const AABoxClass & light_box = light->Get_Cull_Box();
		Vector3 d = bounds.Center - light_box.Center;
		if (	(WWMath::Fabs(d.X) > (bounds.Extent.X + light_box.Extent.X)) ||
				(WWMath::Fabs(d.Y) > (bounds.Extent.Y + light_box.Extent.Y)) ||
				(WWMath::Fabs(d.Z) > (bounds.Extent.Z + light_box.Extent.Z)) )
		{
			continue;
		}

		list->Add(light);
		count++;
	}

	return count;
}


/***********************************************************************************************
 * WorldLightManager::Begin_Frame -- the composed environments start again                     *
 *=============================================================================================*/
void WorldLightManager::Begin_Frame(void)
{
	EnvironmentNext	= 0;

	QueryCount			= 0;
	LightsExamined		= 0;
	LightsApplied		= 0;
	LitObjectCount		= 0;
	TrivialRejects		= 0;
	Overflows			= 0;
}


/***********************************************************************************************
 * WorldLightManager::Apply_Dynamic_Lights -- the environment to actually render an object with*
 *                                                                                             *
 * Nearly every object, nearly every frame, gets its own cached static lighting environment      *
 * back unchanged and nothing was copied.  That is the property the whole design exists for:     *
 * a per-object light query is only affordable if the answer "no lights" is free.                *
 *=============================================================================================*/
LightEnvironmentClass * WorldLightManager::Apply_Dynamic_Lights
(
	PhysClass *					obj,
	LightEnvironmentClass *	static_env
)
{
	if (!Initialized || (obj == nullptr) || (static_env == nullptr) || (Lights.Count() == 0)) {
		return static_env;
	}

	RenderObjClass * model = obj->Peek_Model();
	if (model == nullptr) {
		return static_env;
	}

	/*
	**	The same point the static lighting was computed at, so the two halves of an object's
	**	lighting are answers about the same place.
	*/
	Vector3 center = model->Get_Bounding_Sphere().Center;

	LightPhysClass * set[WORLD_LIGHT_MAX_LIGHTS_PER_OBJECT];
	int count = Gather(center,set,WORLD_LIGHT_MAX_LIGHTS_PER_OBJECT);
	if (count == 0) {
		return static_env;
	}

	if (EnvironmentNext >= EnvironmentCount) {
		Overflows++;
		return static_env;
	}

	LightEnvironmentClass * env = &(Environments[EnvironmentNext++]);
	*env = *static_env;

	for (int i = 0; i < count; i++) {
		LightClass * light_obj = (LightClass *)set[i]->Peek_Model();
		env->Add_Light(*light_obj);
	}

	LightsApplied += count;
	LitObjectCount++;

	return env;
}


int WorldLightManager::Get_Query_Count(void)				{ return QueryCount; }
int WorldLightManager::Get_Lights_Examined(void)		{ return LightsExamined; }
int WorldLightManager::Get_Lights_Applied(void)			{ return LightsApplied; }
int WorldLightManager::Get_Lit_Object_Count(void)		{ return LitObjectCount; }
int WorldLightManager::Get_Trivial_Reject_Count(void)	{ return TrivialRejects; }
int WorldLightManager::Get_Overflow_Count(void)			{ return Overflows; }
int WorldLightManager::Get_Refusal_Count(void)			{ return Refusals; }
int WorldLightManager::Get_Clamped_Radius_Count(void)	{ return ClampedRadii; }
