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
 *                     $Archive:: /Commando/Code/wwphys/worldsurfacemarkmanager.cpp           $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "worldsurfacemarkmanager.h"

#include "assetmgr.h"
#include "decophys.h"
#include "dynamesh.h"
#include "phys.h"
#include "pscene.h"
#include "refcount.h"
#include "shader.h"
#include "surfaceribbonsystem.h"
#include "texture.h"
#include "vertmaterial.h"
#include "vector4.h"
#include "wwdebug.h"
#include "wwmath.h"

#include <string.h>


DynamicVectorClass<SurfaceMarkDefinitionClass>	WorldSurfaceMarkManager::Definitions;
SurfaceMarkClass *										WorldSurfaceMarkManager::Pool				= nullptr;
int															WorldSurfaceMarkManager::PoolSize		= 0;
uint32														WorldSurfaceMarkManager::NextSerial	= 1;
DynamicVectorClass<DecorationPhysClass *>			WorldSurfaceMarkManager::Objects;
DynamicVectorClass<DynamicMeshClass *>				WorldSurfaceMarkManager::Meshes;
float															WorldSurfaceMarkManager::DrawDistance	= 250.0f;
int															WorldSurfaceMarkManager::MissingTextures = 0;
int															WorldSurfaceMarkManager::Evictions		= 0;
int															WorldSurfaceMarkManager::GroupRefusals	= 0;
PhysicsSceneClass *										WorldSurfaceMarkManager::BuiltScene		= nullptr;


/*
**	How many marks the world may hold at once.  Every one of them is an entry in an array that
**	is allocated once and never resized, so "hundreds or thousands of marks" costs exactly this
**	whether the number in use is nought or all of them.
*/
enum { SURFACE_MARK_POOL_SIZE = 1024 };

/*
**	How many of those may be marks the batch cannot draw -- ones handed to the geometry-clipping
**	projector because they lie on a wall or across a step.  Those are the expensive kind: each
**	one clips real polygons out of real meshes.  The budget is what stops a firefight indoors
**	from spending the whole frame in the clipper, and the projector's own recycling ring is
**	given twice the room so that it can never lap a mark this pool still believes in.
*/
enum { SURFACE_MARK_PROJECTED_BUDGET = 96 };

/*
**	A mark is placed against a surface, and the surface has to be facing roughly upward before
**	a flat draped quad is the right way to draw it.  Below this the projector is asked instead.
*/
static const float SURFACE_MARK_GROUND_DOT = 0.65f;

/*
**	How far the corners of a draped mark may disagree about where the ground is, as a multiple
**	of the mark's own radius, before the drape is abandoned.  A mark spanning a kerb or the
**	nose of a staircase fails this and is clipped into the geometry properly instead of being
**	stretched over the top of it.
*/
static const float SURFACE_MARK_MAX_DRAPE = 1.5f;


/***********************************************************************************************
 *	Mark_Shader -- one translucent surface laid on another                                      *
 *                                                                                             *
 *	The same shader the ribbons use, for the same reasons: a mark has no edge to cut around, so  *
 *	it is blended rather than alpha-tested; it writes no depth because it sits a few centimetres *
 *	above a surface it must not fight with; and it is unculled because a draped quad's winding   *
 *	depends on which way the thing that made it happened to be facing.                           *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static ShaderClass Mark_Shader(void)
{
	ShaderClass shader = ShaderClass::_PresetOpaqueShader;
	shader.Set_Alpha_Test(ShaderClass::ALPHATEST_DISABLE);
	shader.Set_Depth_Mask(ShaderClass::DEPTH_WRITE_DISABLE);
	shader.Set_Src_Blend_Func(ShaderClass::SRCBLEND_SRC_ALPHA);
	shader.Set_Dst_Blend_Func(ShaderClass::DSTBLEND_ONE_MINUS_SRC_ALPHA);
	shader.Set_Cull_Mode(ShaderClass::CULL_MODE_DISABLE);
	return shader;
}


/*
**	Handles.  The low twelve bits are the slot, the rest is the serial the slot carried when the
**	handle was made.  A handle to a mark that has since been evicted therefore fails to match
**	rather than quietly addressing whatever took its place, and no handle is ever zero because
**	the serial starts at one.
*/
enum { SURFACE_MARK_SLOT_BITS = 12, SURFACE_MARK_SLOT_MASK = (1 << SURFACE_MARK_SLOT_BITS) - 1 };

static inline uint32 Make_Handle(int slot,uint32 serial)
{
	return (serial << SURFACE_MARK_SLOT_BITS) | ((uint32)slot & SURFACE_MARK_SLOT_MASK);
}


void WorldSurfaceMarkManager::Init(void)
{
	if (Pool != nullptr) {
		return ;
	}

	Pool			= new SurfaceMarkClass[SURFACE_MARK_POOL_SIZE];
	PoolSize		= SURFACE_MARK_POOL_SIZE;
	NextSerial	= 1;

	MissingTextures	= 0;
	Evictions			= 0;
	GroupRefusals		= 0;
	BuiltScene			= nullptr;
	return ;
}


void WorldSurfaceMarkManager::Shutdown(void)
{
	Clear_Marks();
	Destroy_Geometry();

	if (Pool != nullptr) {
		delete [] Pool;
		Pool = nullptr;
	}
	PoolSize = 0;

	Definitions.Delete_All();
	Objects.Delete_All();
	Meshes.Delete_All();

	MissingTextures	= 0;
	Evictions			= 0;
	GroupRefusals		= 0;
	return ;
}


/***********************************************************************************************
 *	WorldSurfaceMarkManager::Define_Default_Marks -- the kinds Section 35 names                  *
 *                                                                                             *
 *	One definition per mark type, so that code which wants "a scorch" can ask for one without    *
 *	knowing an asset name.  None of them names a texture, so none of them draws anything today   *
 *	and none of them allocates a mesh -- see docs/assets/SurfaceMarks.md.  Content that carries  *
 *	its own decal texture, which is most of what the game actually fires, does not use these at  *
 *	all; it interns a group of its own through Find_Or_Define_Texture_Group.                     *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void WorldSurfaceMarkManager::Define_Default_Marks(void)
{
	if (Definitions.Count() > 0) {
		return ;
	}

	struct DefaultMarkStruct {
		const char *	Name;
		int				Type;
		float				Radius;
		float				Lifetime;
		float				Opacity;
	};

	static const DefaultMarkStruct _defaults[] = {
		{ "ow_mark_scorch",			SURFACE_MARK_SCORCH,							1.50f,	45.0f,	1.00f },
		{ "ow_mark_blast",			SURFACE_MARK_BLAST,							3.00f,	60.0f,	0.90f },
		{ "ow_mark_dirt",				SURFACE_MARK_DIRT,							1.00f,	30.0f,	0.70f },
		{ "ow_mark_stain",			SURFACE_MARK_OIL_OR_STAIN,					0.80f,	90.0f,	0.85f },
		{ "ow_mark_construction",	SURFACE_MARK_CONSTRUCTION,					4.00f,	 0.0f,	1.00f },
		{ "ow_mark_impact",			SURFACE_MARK_IMPACT,							0.25f,	25.0f,	1.00f },
		{ "ow_mark_world",			SURFACE_MARK_DECORATIVE_WORLD_MARK,		2.00f,	 0.0f,	1.00f },
	};

	for (int i = 0; i < (int)(sizeof(_defaults) / sizeof(_defaults[0])); i++) {

		SurfaceMarkDefinitionClass def;
		def.Set_Name(_defaults[i].Name);
		def.Set_Type(_defaults[i].Type);
		def.Set_Radius(_defaults[i].Radius);
		def.Set_Lifetime(_defaults[i].Lifetime);
		def.Set_Opacity(_defaults[i].Opacity);
		def.Set_Fade_Start(0.7f);
		def.Set_Lift_Height(0.05f);
		def.Set_Drape(true);

		Define_Definition(def);
	}
	return ;
}


int WorldSurfaceMarkManager::Define_Definition(const SurfaceMarkDefinitionClass & def)
{
	int existing = Find_Definition_Index(def.Get_Name());
	if (existing >= 0) {
		/*
		**	Replacing a definition replaces the texture with it, and the texture is baked into
		**	the mesh's material info when the mesh is made -- so the mesh has to go, and the
		**	marks in that group redraw into a new one on the next frame.
		*/
		Definitions[existing] = def;
		Destroy_Definition_Geometry(existing);
		return existing;
	}

	if (Definitions.Count() >= SURFACE_MARK_MAX_GROUPS) {
		GroupRefusals++;
		return -1;
	}

	Definitions.Add(def);
	Objects.Add(nullptr);
	Meshes.Add(nullptr);
	return Definitions.Count() - 1;
}


int WorldSurfaceMarkManager::Get_Definition_Count(void)
{
	return Definitions.Count();
}


const SurfaceMarkDefinitionClass & WorldSurfaceMarkManager::Peek_Definition(int index)
{
	WWASSERT((index >= 0) && (index < Definitions.Count()));
	return Definitions[index];
}


int WorldSurfaceMarkManager::Find_Definition_Index(const char * name)
{
	if ((name == nullptr) || (name[0] == 0)) {
		return -1;
	}
	for (int i = 0; i < Definitions.Count(); i++) {
		if (::stricmp(Definitions[i].Get_Name(),name) == 0) {
			return i;
		}
	}
	return -1;
}


SurfaceMarkDefinitionClass * WorldSurfaceMarkManager::Find_Definition(const char * name)
{
	int index = Find_Definition_Index(name);
	return (index >= 0) ? &Definitions[index] : nullptr;
}


int WorldSurfaceMarkManager::Find_Definition_By_Type(int type)
{
	for (int i = 0; i < Definitions.Count(); i++) {
		if (Definitions[i].Get_Type() == type) {
			return i;
		}
	}
	return -1;
}


/***********************************************************************************************
 *	WorldSurfaceMarkManager::Find_Or_Define_Texture_Group -- content names its own textures      *
 *                                                                                             *
 *	An explosion definition carries a DecalFilename and a surface effect carries another, and    *
 *	neither of them was written against a table in this file.  So a group is interned the first  *
 *	time a texture name is seen and every later mark with that name joins it -- which is the     *
 *	whole of the "material/texture grouping" requirement, arrived at from the direction the      *
 *	content actually comes from.                                                                 *
 *                                                                                             *
 *	The table is capped.  A level that somehow names more textures than that gets a counted      *
 *	refusal and its marks go to the projector, rather than the mesh table growing with the       *
 *	content.                                                                                     *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
int WorldSurfaceMarkManager::Find_Or_Define_Texture_Group(const char * texture_name,int type,float radius)
{
	if ((texture_name == nullptr) || (texture_name[0] == 0)) {
		return -1;
	}

	//	Groups interned from content are named after the texture they draw with, so a second
	//	mark with the same texture finds the first one's group by name.
	int existing = Find_Definition_Index(texture_name);
	if (existing >= 0) {
		return existing;
	}

	if (Definitions.Count() >= SURFACE_MARK_MAX_GROUPS) {
		GroupRefusals++;
		return -1;
	}

	SurfaceMarkDefinitionClass def;
	def.Set_Name(texture_name);
	def.Set_Texture(texture_name);
	def.Set_Type(type);
	def.Set_Radius(radius);
	def.Set_Lifetime(Surface_Mark_Type_Is_Persistent(type) ? 0.0f : 45.0f);
	def.Set_Fade_Start(0.7f);
	def.Set_Opacity(1.0f);
	def.Set_Lift_Height(0.05f);
	def.Set_Drape(true);

	return Define_Definition(def);
}


/***********************************************************************************************
 *	WorldSurfaceMarkManager::Allocate_Slot -- the eviction policy, and the whole memory bound    *
 *                                                                                             *
 *	Two caps, both enforced here, both deterministic.  The group cap keeps any one texture from  *
 *	needing more geometry than its mesh was built to hold; the pool cap keeps the world from     *
 *	holding more marks than the pool has room for.  Neither one ever allocates: when a cap is    *
 *	reached something already there leaves.                                                     *
 *                                                                                             *
 *	What leaves is the mark furthest through its own fade -- the one closest to vanishing, and   *
 *	so the one whose disappearance is least visible.  Marks with no lifetime at all score zero   *
 *	and are taken last, oldest first, which is what makes a construction mark outlive the        *
 *	scorches thrown on top of it.                                                                *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
int WorldSurfaceMarkManager::Allocate_Slot(int definition_index)
{
	if ((Pool == nullptr) || (definition_index < 0) || (definition_index >= Definitions.Count())) {
		return -1;
	}

	/*
	**	First cap: this group's share.  Counting is a walk of the pool, which is a fixed number
	**	of comparisons over an array that is already in cache, and it happens once per mark
	**	created rather than once per frame.
	*/
	int group_count = 0;
	int group_victim = -1;
	float group_worst = -1.0f;

	int free_slot = -1;
	int pool_victim = -1;
	float pool_worst = -1.0f;

	for (int i = 0; i < PoolSize; i++) {

		SurfaceMarkClass & mark = Pool[i];

		if (!mark.InUse) {
			if (free_slot < 0) {
				free_slot = i;
			}
			continue;
		}

		float score = 0.0f;
		if ((mark.Definition >= 0) && (mark.Definition < Definitions.Count())) {
			float lifetime = Definitions[mark.Definition].Get_Lifetime();
			//	A mark with a lifetime scores how far through it is.  One without scores by age
			//	alone, on a scale small enough that any fading mark is taken before it is.
			score = (lifetime > WWMATH_EPSILON) ? (mark.Age / lifetime) : (mark.Age * 0.000001f);
		}

		if (score > pool_worst) {
			pool_worst = score;
			pool_victim = i;
		}

		if (mark.Definition == definition_index) {
			group_count++;
			if (score > group_worst) {
				group_worst = score;
				group_victim = i;
			}
		}
	}

	if ((group_count >= SURFACE_MARK_MAX_PER_GROUP) && (group_victim >= 0)) {
		Release_Slot(group_victim);
		Evictions++;
		return group_victim;
	}

	if (free_slot >= 0) {
		return free_slot;
	}

	if (pool_victim >= 0) {
		Release_Slot(pool_victim);
		Evictions++;
		return pool_victim;
	}

	return -1;
}


void WorldSurfaceMarkManager::Release_Slot(int slot)
{
	if ((Pool == nullptr) || (slot < 0) || (slot >= PoolSize)) {
		return ;
	}

	SurfaceMarkClass & mark = Pool[slot];

	//	A mark the projector drew has geometry clipped into somebody else's meshes, and taking
	//	the entry away without saying so would leave it on the wall forever.
	if (mark.InUse && mark.Projected && (mark.DecalID != 0)) {
		PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
		if (scene != nullptr) {
			scene->Remove_Decal(mark.DecalID);
		}
	}

	mark.Reset();

	//	Any handle still pointing at this slot stops matching from here on.
	mark.Serial = 0;
	return ;
}


/***********************************************************************************************
 *	WorldSurfaceMarkManager::Drape_Mark -- four corners on the ground that is actually there     *
 *                                                                                             *
 *	Section 35 asks for terrain projection with normal and slope handling.  The corners are      *
 *	conformed one at a time rather than the centre being conformed and the quad laid flat, so a  *
 *	mark on a hillside follows the hillside instead of burying one edge and floating the other.  *
 *                                                                                             *
 *	Returning false is not a failure, it is the answer "not like this".  The caller hands the    *
 *	mark to the projector, which clips it into the geometry properly.  That is what keeps a      *
 *	quad off the nose of a staircase and out of a doorway.                                      *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool WorldSurfaceMarkManager::Drape_Mark(SurfaceMarkClass & mark,const Vector3 & center,
													  const Vector3 & normal,const Vector3 & tangent,float radius)
{
	Vector3 up = normal;
	if (up.Length2() < WWMATH_EPSILON) {
		up.Set(0.0f,0.0f,1.0f);
	}
	up.Normalize();

	/*
	**	Take whatever the caller was facing, remove the part of it that points along the normal,
	**	and use what is left as the direction across the mark.  When the caller was facing
	**	straight at the surface there is nothing left, so any perpendicular will do -- a scorch
	**	is round enough that its rotation does not matter, and the fallback keeps a degenerate
	**	transform from producing a degenerate quad.
	*/
	Vector3 along = tangent - (up * Vector3::Dot_Product(tangent,up));
	if (along.Length2() < 0.0001f) {
		along = Vector3(1.0f,0.0f,0.0f) - (up * up.X);
		if (along.Length2() < 0.0001f) {
			along = Vector3(0.0f,1.0f,0.0f) - (up * up.Y);
		}
	}
	along.Normalize();

	Vector3 across = Vector3::Cross_Product(up,along);
	across.Normalize();

	along  *= radius;
	across *= radius;

	//	Tri-strip order: the two ends of one edge, then the two ends of the other.
	mark.Corner[0] = center - along + across;
	mark.Corner[1] = center + along + across;
	mark.Corner[2] = center - along - across;
	mark.Corner[3] = center + along - across;

	const SurfaceMarkDefinitionClass & def = Definitions[mark.Definition];
	float lift = def.Get_Lift_Height();

	if (!def.Get_Drape()) {
		//	A mark that does not drape sits on the plane it was given -- a wall mark placed by
		//	a caller that already knows the wall is flat.
		for (int i = 0; i < 4; i++) {
			mark.Corner[i] += up * lift;
		}
		return true;
	}

	float lowest	=  1.0e30f;
	float highest	= -1.0e30f;

	for (int i = 0; i < 4; i++) {

		float height = 0.0f;
		if (!Conform_Point(mark.Corner[i].X,mark.Corner[i].Y,center.Z,&height)) {
			return false;
		}

		mark.Corner[i].Z = height;

		if (height < lowest)  { lowest = height; }
		if (height > highest) { highest = height; }
	}

	//	Too much disagreement between the corners means the mark is spanning something the quad
	//	cannot follow.  Let the clipper have it.
	if ((highest - lowest) > (radius * SURFACE_MARK_MAX_DRAPE)) {
		return false;
	}

	//	Lift straight up rather than along the surface normal.  The corners are already on the
	//	ground each of them landed on, so the lift only has to clear it, and a per-corner normal
	//	would tilt neighbouring corners away from each other.
	for (int i = 0; i < 4; i++) {
		mark.Corner[i].Z += lift;
	}

	return true;
}


/***********************************************************************************************
 *	WorldSurfaceMarkManager::Conform_Point -- where the ground is                                *
 *                                                                                             *
 *	Composed rather than worked out again.  The bridge is asked first, because a mark left on a  *
 *	bridge belongs on the deck and not on the riverbed under it, and then the road system, which *
 *	is heightfield terrain where a level has it and a ray through the physics scene where the    *
 *	ground is authored W3D geometry instead.  There is one implementation of this in the engine  *
 *	and it lives with the ribbons, which needed it first; this is its second caller.             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool WorldSurfaceMarkManager::Conform_Point(float x,float y,float hint_z,float * height_out)
{
	return SurfaceRibbonSystem::Conform_Point(x,y,hint_z,height_out);
}


uint32 WorldSurfaceMarkManager::Add_Mark(int definition_index,const Vector3 & center,
													  const Vector3 & normal,float radius)
{
	if ((Pool == nullptr) || (definition_index < 0) || (definition_index >= Definitions.Count())) {
		return 0;
	}

	const SurfaceMarkDefinitionClass & def = Definitions[definition_index];

	if (radius <= 0.0f) {
		radius = def.Get_Radius();
	}

	int slot = Allocate_Slot(definition_index);
	if (slot < 0) {
		return 0;
	}

	SurfaceMarkClass & mark = Pool[slot];
	mark.Reset();
	mark.Definition	= definition_index;
	mark.Center			= center;
	mark.Normal			= normal;
	mark.Radius			= radius;
	mark.Age				= 0.0f;
	mark.Opacity		= def.Get_Opacity();
	mark.Projected		= false;

	//	Pick something to orient the quad by that is not the normal itself.
	Vector3 tangent(1.0f,0.0f,0.0f);
	if (WWMath::Fabs(Vector3::Dot_Product(tangent,normal)) > 0.9f) {
		tangent.Set(0.0f,1.0f,0.0f);
	}

	if (!Drape_Mark(mark,center,normal,tangent,radius)) {
		mark.Reset();
		return 0;
	}

	mark.InUse	= true;
	mark.Serial	= NextSerial++;
	if (NextSerial == 0) { NextSerial = 1; }

	return Make_Handle(slot,mark.Serial);
}


/***********************************************************************************************
 *	WorldSurfaceMarkManager::Create_Mark -- the one entry point                                  *
 *                                                                                             *
 *	Everything in the game that leaves a mark comes through here, which is the point of the      *
 *	service: one database, one bound, one eviction policy, whichever way the mark ends up being  *
 *	drawn.  What is decided here is only *how* to draw it.                                       *
 *                                                                                             *
 *	The batch is preferred, because the batch is what scales.  The projector is used when the    *
 *	batch would be wrong: a mark that must clip to one particular object, a mark on glass, a     *
 *	mark on a surface that is not facing up, and a mark whose corners cannot agree on where the  *
 *	ground is.  Those are the marks that need real geometry, and there are far fewer of them.    *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
uint32 WorldSurfaceMarkManager::Create_Mark(	const Matrix3D &	tm,
															const char *		texture_name,
															float					radius,
															int					type,
															bool					apply_to_translucent_meshes,
															PhysClass *			only_this_obj)
{
	if (Pool == nullptr) {
		return 0;
	}

	if (radius <= 0.0f) {
		radius = 1.0f;
	}

	int definition = Find_Or_Define_Texture_Group(texture_name,type,radius);

	Vector3 center	= tm.Get_Translation();
	Vector3 normal	= tm.Get_Z_Vector();		// the decal transform looks along -Z at the surface
	Vector3 tangent = tm.Get_X_Vector();

	/*
	**	Marks that have to clip into one specific object -- glass, and anything a caller named
	**	an object for -- are never candidates for the batch.  The batch draws on the ground; the
	**	whole reason those callers named an object is that the ground is not what they hit.
	*/
	bool must_project = (only_this_obj != nullptr) || apply_to_translucent_meshes;

	if (!must_project && (definition >= 0)) {

		bool facing_up = (Vector3::Dot_Product(normal,Vector3(0.0f,0.0f,1.0f)) >= SURFACE_MARK_GROUND_DOT);

		if (facing_up) {

			int slot = Allocate_Slot(definition);
			if (slot >= 0) {

				SurfaceMarkClass & mark = Pool[slot];
				mark.Reset();
				mark.Definition	= definition;
				mark.Center			= center;
				mark.Normal			= normal;
				mark.Radius			= radius;
				mark.Opacity		= Definitions[definition].Get_Opacity();
				mark.Projected		= false;

				if (Drape_Mark(mark,center,normal,tangent,radius)) {
					mark.InUse	= true;
					mark.Serial	= NextSerial++;
					if (NextSerial == 0) { NextSerial = 1; }
					return Make_Handle(slot,mark.Serial);
				}

				//	The drape was refused, so the slot goes straight back.  Nothing was drawn
				//	and nothing was evicted for it beyond what Allocate_Slot already did.
				mark.Reset();
			}
		}
	}

	/*
	**	The projector, then.  Its budget is enforced here rather than inside it, so that the
	**	number of marks in the world is a single number this file controls.
	*/
	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
	if (scene == nullptr) {
		return 0;
	}

	/*
	**	A projected mark still belongs to a group, even though the group's mesh will not draw
	**	it: the group is where its lifetime comes from, and a mark with no group would never be
	**	aged out.  When the group table is full -- which is counted as a refusal -- it joins the
	**	first group rather than living forever outside the policy.
	*/
	int proj_definition = (definition >= 0) ? definition : ((Definitions.Count() > 0) ? 0 : -1);
	if (proj_definition < 0) {
		return 0;
	}

	int projected = Get_Projected_Mark_Count();
	while (projected >= SURFACE_MARK_PROJECTED_BUDGET) {

		//	Oldest first.  A clipped mark has no fade of its own to measure -- the projector
		//	draws it at full strength until it is taken away -- so age is the only ordering
		//	there is, and it is a deterministic one.
		int oldest = -1;
		float oldest_age = -1.0f;
		for (int i = 0; i < PoolSize; i++) {
			if (Pool[i].InUse && Pool[i].Projected && (Pool[i].Age > oldest_age)) {
				oldest_age = Pool[i].Age;
				oldest = i;
			}
		}

		if (oldest < 0) {
			break;
		}

		Release_Slot(oldest);
		Evictions++;
		projected--;
	}

	int slot = Allocate_Slot(proj_definition);
	if (slot < 0) {
		return 0;
	}

	int decal_id = scene->Create_Decal(tm,texture_name,radius,false,apply_to_translucent_meshes,only_this_obj);
	if (decal_id < 0) {
		return 0;
	}

	SurfaceMarkClass & mark = Pool[slot];
	mark.Reset();
	mark.Definition	= proj_definition;
	mark.Center			= center;
	mark.Normal			= normal;
	mark.Radius			= radius;
	mark.Opacity		= 1.0f;
	mark.Projected		= true;
	mark.DecalID		= (uint32)decal_id;
	mark.InUse			= true;
	mark.Serial			= NextSerial++;
	if (NextSerial == 0) { NextSerial = 1; }

	return Make_Handle(slot,mark.Serial);
}


bool WorldSurfaceMarkManager::Is_Mark_Alive(uint32 handle)
{
	return (Peek_Mark(handle) != nullptr);
}


const SurfaceMarkClass * WorldSurfaceMarkManager::Peek_Mark(uint32 handle)
{
	if ((Pool == nullptr) || (handle == 0)) {
		return nullptr;
	}

	int slot = (int)(handle & SURFACE_MARK_SLOT_MASK);
	uint32 serial = handle >> SURFACE_MARK_SLOT_BITS;

	if ((slot < 0) || (slot >= PoolSize)) {
		return nullptr;
	}

	const SurfaceMarkClass & mark = Pool[slot];
	if (!mark.InUse || (mark.Serial != serial)) {
		return nullptr;
	}

	return &mark;
}


bool WorldSurfaceMarkManager::Remove_Mark(uint32 handle)
{
	if (Peek_Mark(handle) == nullptr) {
		return false;
	}

	Release_Slot((int)(handle & SURFACE_MARK_SLOT_MASK));
	return true;
}


/***********************************************************************************************
 *	WorldSurfaceMarkManager::Mark_Alpha -- how much of a mark is left                            *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
float WorldSurfaceMarkManager::Mark_Alpha(const SurfaceMarkClass & mark,
														const SurfaceMarkDefinitionClass & def)
{
	float alpha = mark.Opacity;

	float lifetime = def.Get_Lifetime();
	if (lifetime > WWMATH_EPSILON) {
		float fade_start = def.Get_Fade_Start() * lifetime;
		if (mark.Age > fade_start) {
			float span = lifetime - fade_start;
			alpha *= (span > WWMATH_EPSILON) ? (1.0f - ((mark.Age - fade_start) / span)) : 0.0f;
		}
	}

	if (alpha < 0.0f) { alpha = 0.0f; }
	if (alpha > 1.0f) { alpha = 1.0f; }
	return alpha;
}


void WorldSurfaceMarkManager::Timestep(float dt)
{
	if (Pool == nullptr) {
		return ;
	}

	if (dt > 0.0f) {
		for (int i = 0; i < PoolSize; i++) {

			SurfaceMarkClass & mark = Pool[i];
			if (!mark.InUse) {
				continue;
			}

			if ((mark.Definition < 0) || (mark.Definition >= Definitions.Count())) {
				Release_Slot(i);
				continue;
			}

			mark.Age += dt;

			//	A mark with no lifetime ages anyway.  Nothing takes it away for being old, but
			//	the age is what orders it against the other persistent marks when the pool
			//	finally needs a slot.
			float lifetime = Definitions[mark.Definition].Get_Lifetime();
			if ((lifetime > 0.0f) && (mark.Age >= lifetime)) {
				Release_Slot(i);
			}
		}
	}

	Build_Geometry();
	return ;
}


void WorldSurfaceMarkManager::Clear_Marks(void)
{
	if (Pool == nullptr) {
		return ;
	}
	for (int i = 0; i < PoolSize; i++) {
		if (Pool[i].InUse) {
			Release_Slot(i);
		}
	}
	return ;
}


bool WorldSurfaceMarkManager::Build_Geometry(void)
{
	MissingTextures = 0;

	if (Pool == nullptr) {
		return false;
	}

	/*
	**	A level change replaces the physics scene.  Anything built into the last one went with
	**	it, so the references are dropped without asking the new scene to remove objects it
	**	never held, and the marks go too -- they were placed on ground that is not here any
	**	more.  This is the "cleanup on world unload" Section 35 asks for, and it needs nothing
	**	from whatever performs the unload.
	*/
	PhysicsSceneClass * current = PhysicsSceneClass::Get_Instance();
	if (current != BuiltScene) {
		if (BuiltScene != nullptr) {
			Forget_Geometry();
			Clear_Marks();
		}
		BuiltScene = current;

		/*
		**	Size the clipping backend's recycling ring against this manager's budget for clipped
		**	marks, with room to spare.  The manager is the only thing that asks it for a decal
		**	and it evicts its own oldest clipped mark once it is at budget, so giving the ring
		**	twice that means its index can never lap a decal still recorded in the pool.
		**
		**	Stock Renegade left this at fifty, which is what "hundreds or thousands of marks"
		**	could not be built out of, and is why the batch exists at all.
		*/
		if (current != nullptr) {
			current->Set_Decal_Pool_Size(SURFACE_MARK_PROJECTED_BUDGET * 2);
		}

		return false;
	}

	bool built_anything = false;

	for (int d = 0; d < Definitions.Count(); d++) {

		int drawable = 0;
		for (int i = 0; i < PoolSize; i++) {
			if (Pool[i].InUse && !Pool[i].Projected && (Pool[i].Definition == d)) {
				drawable++;
			}
		}

		if (drawable == 0) {
			if ((d < Meshes.Count()) && (Meshes[d] != nullptr)) {
				Meshes[d]->Reset_Mesh_Counters();
				Meshes[d]->Set_Dirty();
			}
			continue;
		}

		if (!Definitions[d].Names_A_Texture()) {
			//	Marks of a kind nothing can draw yet.  They exist, they age, they evict, and
			//	they cost nothing else.
			MissingTextures++;
			continue;
		}

		if (Build_Definition_Geometry(d)) {
			built_anything = true;
		}
	}

	return built_anything;
}


bool WorldSurfaceMarkManager::Build_Definition_Geometry(int definition_index)
{
	const SurfaceMarkDefinitionClass & def = Definitions[definition_index];

	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();

	/*
	**	Made once, at the largest this group can ever be asked to hold, and refilled in place
	**	from then on.  Building it to fit would allocate a mesh every frame, which is the
	**	unbounded allocation the acceptance rules out.
	*/
	if (Meshes[definition_index] == nullptr) {

		if (scene == nullptr) {
			return false;
		}

		/*
		**	The texture is fetched before anything is built, and no texture means nothing is
		**	built at all.  That is also what keeps a dedicated server -- which holds marks, ages
		**	them and answers every query about them -- from allocating a mesh it can never draw,
		**	without a headless special case anywhere in this file.
		*/
		if (WW3DAssetManager::Get_Instance() == nullptr) {
			MissingTextures++;
			return false;
		}

		TextureClass * texture = WW3DAssetManager::Get_Instance()->Get_Texture(def.Get_Texture());
		if (texture == nullptr) {
			MissingTextures++;
			return false;
		}

		const int max_verts = SURFACE_MARK_MAX_PER_GROUP * 4;
		const int max_polys = SURFACE_MARK_MAX_PER_GROUP * 2;

		DynamicMeshClass * mesh = NEW_REF(DynamicMeshClass,(max_polys,max_verts));
		if (mesh == nullptr) {
			texture->Release_Ref();
			return false;
		}

		mesh->Set_Shader(Mark_Shader());

		//	Lighting off, colour straight off the vertex, which is where the fade lives.
		VertexMaterialClass * vmat = VertexMaterialClass::Get_Preset(VertexMaterialClass::PRELIT_DIFFUSE);
		if (vmat != nullptr) {
			mesh->Set_Vertex_Material(vmat,true);
			vmat->Release_Ref();
		}

		mesh->Set_Texture(texture,true);
		texture->Release_Ref();

		//	A mark is paint.  Nothing collides with it.
		mesh->Set_Collision_Type(0);

		DecorationPhysClass * phys = new DecorationPhysClass;
		phys->Set_Model(mesh);

		Matrix3D identity;
		identity.Make_Identity();
		phys->Set_Transform(identity);

		//	Client-side and transient: nothing here is networked, because a mark is a function
		//	of an event every machine already saw, and nothing here is saved, because a reloaded
		//	game has not been shot at yet.
		phys->Enable_Dont_Save(true);

		mesh->Release_Ref();
		scene->Add_Dynamic_Object(phys);

		Meshes[definition_index] = mesh;
		Objects[definition_index] = phys;
	}

	DynamicMeshClass * mesh = Meshes[definition_index];

	/*
	**	Refill.  Deliberately not DynamicMeshClass::Reset, which throws away the material info
	**	and makes a new one -- an allocation per frame, and the texture would have to be looked
	**	up and re-added every frame with it.
	*/
	mesh->Reset_Mesh_Counters();
	mesh->Set_Dirty();

	const int max_verts = mesh->Peek_Model()->Get_Vertex_Count();
	const int max_polys = mesh->Peek_Model()->Get_Polygon_Count();
	int verts = 0;
	int polys = 0;

	/*
	**	The group culls as one object through the spatial index like anything else in the scene.
	**	This is the finer cut inside it: a mark far enough away to be a pixel is left out of the
	**	buffer entirely, so a level with a thousand marks spread across it fills the buffer with
	**	the ones near the camera rather than all of them.
	*/
	bool distance_cull = (DrawDistance > 0.0f) && (scene != nullptr);
	Vector3 view_point(0.0f,0.0f,0.0f);
	if (distance_cull) {
		view_point = scene->Get_Last_Camera_Position();
	}
	float cull_distance2 = DrawDistance * DrawDistance;

	for (int i = 0; i < PoolSize; i++) {

		const SurfaceMarkClass & mark = Pool[i];
		if (!mark.InUse || mark.Projected || (mark.Definition != definition_index)) {
			continue;
		}

		if (distance_cull) {
			Vector3 delta = mark.Center - view_point;
			if (delta.Length2() > cull_distance2) {
				continue;
			}
		}

		if (((verts + 4) > max_verts) || ((polys + 2) > max_polys)) {
			break;
		}

		float alpha = Mark_Alpha(mark,def);
		mesh->Set_Vertex_Color(Vector4(1.0f,1.0f,1.0f,alpha));

		mesh->Begin_Tri_Strip();
		mesh->Vertex(mark.Corner[0].X,mark.Corner[0].Y,mark.Corner[0].Z,0.0f,0.0f);
		mesh->Vertex(mark.Corner[1].X,mark.Corner[1].Y,mark.Corner[1].Z,1.0f,0.0f);
		mesh->Vertex(mark.Corner[2].X,mark.Corner[2].Y,mark.Corner[2].Z,0.0f,1.0f);
		mesh->Vertex(mark.Corner[3].X,mark.Corner[3].Y,mark.Corner[3].Z,1.0f,1.0f);

		verts += 4;
		polys += 2;
	}

	if (Objects[definition_index] != nullptr) {
		Objects[definition_index]->Update_Cull_Box();
	}

	return (polys > 0);
}


void WorldSurfaceMarkManager::Destroy_Definition_Geometry(int definition_index)
{
	if ((definition_index < 0) || (definition_index >= Objects.Count())) {
		return ;
	}

	if (Objects[definition_index] != nullptr) {
		PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
		if ((scene != nullptr) && (scene == BuiltScene)) {
			scene->Remove_Object(Objects[definition_index]);
		}
		Objects[definition_index]->Release_Ref();
		Objects[definition_index] = nullptr;
	}

	//	The mesh was released into the object when it was set as its model, so there is no
	//	reference here to give back -- only a pointer to stop using.
	Meshes[definition_index] = nullptr;
	return ;
}


void WorldSurfaceMarkManager::Destroy_Geometry(void)
{
	for (int d = 0; d < Objects.Count(); d++) {
		Destroy_Definition_Geometry(d);
	}
	BuiltScene = nullptr;
	return ;
}


/***********************************************************************************************
 *	WorldSurfaceMarkManager::Forget_Geometry -- the scene went away and took the objects with it *
 *                                                                                             *
 *	The references are dropped without asking the current scene to remove objects it never had.  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void WorldSurfaceMarkManager::Forget_Geometry(void)
{
	for (int d = 0; d < Objects.Count(); d++) {
		if (Objects[d] != nullptr) {
			Objects[d]->Release_Ref();
			Objects[d] = nullptr;
		}
		Meshes[d] = nullptr;
	}
	return ;
}


bool WorldSurfaceMarkManager::Has_Geometry(int definition_index)
{
	if ((definition_index < 0) || (definition_index >= Meshes.Count())) {
		return false;
	}
	return (Meshes[definition_index] != nullptr);
}


void WorldSurfaceMarkManager::Set_Draw_Distance(float distance)
{
	DrawDistance = (distance > 0.0f) ? distance : 0.0f;
	return ;
}


float WorldSurfaceMarkManager::Get_Draw_Distance(void)
{
	return DrawDistance;
}


int WorldSurfaceMarkManager::Get_Pool_Size(void)
{
	return PoolSize;
}


int WorldSurfaceMarkManager::Get_Mark_Count(void)
{
	if (Pool == nullptr) {
		return 0;
	}
	int count = 0;
	for (int i = 0; i < PoolSize; i++) {
		if (Pool[i].InUse) { count++; }
	}
	return count;
}


int WorldSurfaceMarkManager::Get_Batched_Mark_Count(void)
{
	if (Pool == nullptr) {
		return 0;
	}
	int count = 0;
	for (int i = 0; i < PoolSize; i++) {
		if (Pool[i].InUse && !Pool[i].Projected) { count++; }
	}
	return count;
}


int WorldSurfaceMarkManager::Get_Projected_Mark_Count(void)
{
	if (Pool == nullptr) {
		return 0;
	}
	int count = 0;
	for (int i = 0; i < PoolSize; i++) {
		if (Pool[i].InUse && Pool[i].Projected) { count++; }
	}
	return count;
}


int WorldSurfaceMarkManager::Get_Projected_Budget(void)
{
	return SURFACE_MARK_PROJECTED_BUDGET;
}


int WorldSurfaceMarkManager::Get_Group_Mark_Count(int definition_index)
{
	if (Pool == nullptr) {
		return 0;
	}
	int count = 0;
	for (int i = 0; i < PoolSize; i++) {
		if (Pool[i].InUse && (Pool[i].Definition == definition_index)) { count++; }
	}
	return count;
}


int WorldSurfaceMarkManager::Get_Object_Count(void)
{
	int count = 0;
	for (int d = 0; d < Objects.Count(); d++) {
		if (Objects[d] != nullptr) { count++; }
	}
	return count;
}


int WorldSurfaceMarkManager::Get_Poly_Count(void)
{
	int count = 0;
	for (int d = 0; d < Meshes.Count(); d++) {
		if (Meshes[d] != nullptr) {
			count += Meshes[d]->Get_Num_Polys();
		}
	}
	return count;
}


int WorldSurfaceMarkManager::Get_Missing_Texture_Count(void)
{
	return MissingTextures;
}


int WorldSurfaceMarkManager::Get_Eviction_Count(void)
{
	return Evictions;
}


int WorldSurfaceMarkManager::Get_Group_Refusal_Count(void)
{
	return GroupRefusals;
}
