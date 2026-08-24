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

//
//	Marks on the ground, out of a pool that never grows.  See surfaceribbonsystem.h.
//

#include "surfaceribbonsystem.h"

#include "assetmgr.h"
#include "bridgesystem.h"
#include "coltype.h"
#include "decophys.h"
#include "dynamesh.h"
#include "matrix3d.h"
#include "pscene.h"
#include "refcount.h"
#include "roadsystem.h"
#include "shader.h"
#include "texture.h"
#include "vertmaterial.h"
#include "vector4.h"
#include "wwdebug.h"
#include "wwmath.h"

#include <string.h>


DynamicVectorClass<SurfaceRibbonDefinitionClass>	SurfaceRibbonSystem::Definitions;
SurfaceRibbonClass *										SurfaceRibbonSystem::Pool				= nullptr;
int															SurfaceRibbonSystem::PoolSize			= 0;
DynamicVectorClass<DecorationPhysClass *>			SurfaceRibbonSystem::Objects;
DynamicVectorClass<DynamicMeshClass *>				SurfaceRibbonSystem::Meshes;
int															SurfaceRibbonSystem::MissingTextures	= 0;
int															SurfaceRibbonSystem::BindFailures	= 0;
PhysicsSceneClass *										SurfaceRibbonSystem::BuiltScene		= nullptr;


/*
**	How many ribbons can be laying marks at once.
**
**	This is the number Section 23's acceptance is really about.  It is fixed, it is allocated
**	once, and when every slot is taken a new emitter takes over the most faded unbound ribbon
**	rather than making a forty-ninth.  A map with two hundred vehicles on it costs exactly what
**	a map with forty-eight does.
*/
enum { SURFACE_RIBBON_POOL_SIZE = 48 };


/***********************************************************************************************
 *	The shader.  A mark is a translucent decal lying on ground that is already drawn, so it      *
 *	blends, does not write depth, and is not culled -- a strip laid down while reversing has     *
 *	the opposite winding to one laid down driving forward, and both have to show.                *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static ShaderClass Ribbon_Shader(void)
{
	ShaderClass shader = ShaderClass::_PresetOpaqueShader;
	shader.Set_Alpha_Test(ShaderClass::ALPHATEST_DISABLE);
	shader.Set_Depth_Mask(ShaderClass::DEPTH_WRITE_DISABLE);
	shader.Set_Src_Blend_Func(ShaderClass::SRCBLEND_SRC_ALPHA);
	shader.Set_Dst_Blend_Func(ShaderClass::DSTBLEND_ONE_MINUS_SRC_ALPHA);
	shader.Set_Cull_Mode(ShaderClass::CULL_MODE_DISABLE);
	return shader;
}


void SurfaceRibbonClass::Reset(void)
{
	Definition	= -1;
	Owner			= 0;
	Bound			= false;
	First			= 0;
	Count			= 0;
	Anchor.Set(0.0f,0.0f,0.0f);
	HaveAnchor	= false;
	Distance		= 0.0f;
	return ;
}


/***********************************************************************************************
 *	SurfaceRibbonSystem::Init / Shutdown                                                        *
 *                                                                                             *
 *	Init is where the only allocation in the whole system happens.  Everything after this point  *
 *	writes into memory that already exists.                                                      *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void SurfaceRibbonSystem::Init(void)
{
	Shutdown();

	Pool = new SurfaceRibbonClass[SURFACE_RIBBON_POOL_SIZE];
	PoolSize = SURFACE_RIBBON_POOL_SIZE;
	return ;
}


void SurfaceRibbonSystem::Shutdown(void)
{
	Destroy_Geometry();

	if (Pool != nullptr) {
		delete [] Pool;
		Pool = nullptr;
	}
	PoolSize = 0;

	Definitions.Delete_All();
	Objects.Delete_All();
	Meshes.Delete_All();
	MissingTextures = 0;
	BindFailures = 0;
	return ;
}


/***********************************************************************************************
 *	SurfaceRibbonSystem::Define_Default_Ribbons -- the five kinds Section 23 names               *
 *                                                                                             *
 *	None of them names a texture, so none of them draws anything yet, and every number below is  *
 *	still real: a tank still lays edges, they still fade on schedule, the pool still fills and    *
 *	empties.  What is missing is art, which is recorded in docs/assets/RibbonSurfaces.md.        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void SurfaceRibbonSystem::Define_Default_Ribbons(void)
{
	struct DefaultRow
	{
		const char *	Name;
		int				Category;
		float				Width;
		float				Segment;
		int				MaxEdges;
		float				Lifetime;
		float				TextureLength;
	};

	//	Segment length is the one number worth reading twice.  A tank track laid every half metre
	//	follows a turning tank; forty-eight of them is twenty-four metres of visible track, which
	//	is about what a tank leaves before the head of it has faded anyway.
	static const DefaultRow rows[] = {
		{ "ow_ribbon_tank_track",		RIBBON_TANK_TRACK,			0.90f,	0.50f,	48,	25.0f,	2.0f },
		{ "ow_ribbon_tire_track",		RIBBON_TIRE_TRACK,			0.30f,	0.60f,	40,	18.0f,	2.0f },
		{ "ow_ribbon_harvester_track",RIBBON_HARVESTER_TRACK,		1.40f,	0.70f,	48,	30.0f,	3.0f },
		{ "ow_ribbon_scorch_trail",	RIBBON_SCORCH_TRAIL,			1.20f,	1.00f,	24,	12.0f,	4.0f },
		{ "ow_ribbon_drag_mark",		RIBBON_DRAG_MARK,				0.50f,	0.40f,	32,	15.0f,	2.0f },
	};

	for (int i = 0; i < (int)(sizeof(rows) / sizeof(rows[0])); i++) {
		if (Find_Definition_Index(rows[i].Name) >= 0) {
			continue;
		}
		SurfaceRibbonDefinitionClass def;
		def.Set_Name(rows[i].Name);
		def.Set_Category(rows[i].Category);
		def.Set_Width(rows[i].Width);
		def.Set_Segment_Length(rows[i].Segment);
		def.Set_Max_Edges(rows[i].MaxEdges);
		def.Set_Opaque_Edges(rows[i].MaxEdges / 2);
		def.Set_Lifetime(rows[i].Lifetime);
		def.Set_Texture_Length(rows[i].TextureLength);
		Define_Definition(def);
	}
	return ;
}


/***********************************************************************************************
 *	Definitions                                                                                 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
int SurfaceRibbonSystem::Define_Definition(const SurfaceRibbonDefinitionClass & def)
{
	int existing = Find_Definition_Index(def.Get_Name());
	if (existing >= 0) {
		Definitions[existing] = def;
		//	The mesh that was made for this kind carries the old texture baked into its material
		//	info, so redefining a kind has to throw it away rather than quietly keep drawing the
		//	thing that was replaced.  It is made again on the next build.
		Destroy_Definition_Geometry(existing);
		return existing;
	}

	Definitions.Add(def);

	//	Every definition gets a geometry slot, empty until something of that kind is drawn.
	Objects.Add(nullptr);
	Meshes.Add(nullptr);

	return Definitions.Count() - 1;
}


int SurfaceRibbonSystem::Get_Definition_Count(void)
{
	return Definitions.Count();
}


const SurfaceRibbonDefinitionClass & SurfaceRibbonSystem::Peek_Definition(int index)
{
	WWASSERT((index >= 0) && (index < Definitions.Count()));
	return Definitions[index];
}


int SurfaceRibbonSystem::Find_Definition_Index(const char * name)
{
	if (name == nullptr) {
		return -1;
	}
	for (int i = 0; i < Definitions.Count(); i++) {
		if (::strcmp(Definitions[i].Get_Name(),name) == 0) {
			return i;
		}
	}
	return -1;
}


SurfaceRibbonDefinitionClass * SurfaceRibbonSystem::Find_Definition(const char * name)
{
	int index = Find_Definition_Index(name);
	return (index >= 0) ? &Definitions[index] : nullptr;
}


int SurfaceRibbonSystem::Find_Definition_By_Category(int category)
{
	for (int i = 0; i < Definitions.Count(); i++) {
		if (Definitions[i].Get_Category() == category) {
			return i;
		}
	}
	return -1;
}


/***********************************************************************************************
 *	SurfaceRibbonSystem::Bind -- take a slot out of the pool                                     *
 *                                                                                             *
 *	Free slots first.  When there are none, the most faded unbound ribbon is taken over: its     *
 *	owner is gone and its marks were about to vanish anyway, so losing them costs less than      *
 *	refusing a live vehicle its tracks.  Only when every slot is still bound to a living emitter *
 *	does this refuse, and it counts the refusal rather than growing.                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
int SurfaceRibbonSystem::Bind(int definition_index,int owner)
{
	if ((Pool == nullptr) || (definition_index < 0) || (definition_index >= Definitions.Count())) {
		BindFailures++;
		return -1;
	}

	int slot = -1;

	for (int i = 0; i < PoolSize; i++) {
		if (Pool[i].Is_Free()) {
			slot = i;
			break;
		}
	}

	if (slot < 0) {
		float oldest = -1.0f;
		for (int i = 0; i < PoolSize; i++) {
			if (Pool[i].Bound || (Pool[i].Count <= 0)) {
				continue;
			}
			//	The newest edge of an abandoned ribbon is the youngest thing in it, so the ribbon
			//	with the oldest "newest edge" is the one furthest through its fade.
			float age = Pool[i].Peek_Edge(Pool[i].Count - 1).Age;
			if (age > oldest) {
				oldest = age;
				slot = i;
			}
		}
	}

	if (slot < 0) {
		BindFailures++;
		return -1;
	}

	Pool[slot].Reset();
	Pool[slot].Definition = definition_index;
	Pool[slot].Owner = owner;
	Pool[slot].Bound = true;
	return slot;
}


void SurfaceRibbonSystem::Unbind(int handle)
{
	if ((Pool == nullptr) || (handle < 0) || (handle >= PoolSize)) {
		return ;
	}

	//	The marks stay.  They fade out on their own, and the slot frees itself when they have.
	Pool[handle].Bound = false;
	Pool[handle].HaveAnchor = false;
	Pool[handle].Owner = 0;

	if (Pool[handle].Count == 0) {
		Pool[handle].Reset();
	}
	return ;
}


bool SurfaceRibbonSystem::Is_Bound(int handle)
{
	if ((Pool == nullptr) || (handle < 0) || (handle >= PoolSize)) {
		return false;
	}
	return Pool[handle].Bound;
}


int SurfaceRibbonSystem::Find_Bound(int owner)
{
	if ((Pool == nullptr) || (owner == 0)) {
		return -1;
	}
	for (int i = 0; i < PoolSize; i++) {
		if (Pool[i].Bound && (Pool[i].Owner == owner)) {
			return i;
		}
	}
	return -1;
}


const SurfaceRibbonClass * SurfaceRibbonSystem::Peek_Ribbon(int handle)
{
	if ((Pool == nullptr) || (handle < 0) || (handle >= PoolSize)) {
		return nullptr;
	}
	return &Pool[handle];
}


/***********************************************************************************************
 *	SurfaceRibbonSystem::Add_Point -- lay the next mark, if the emitter has earned one           *
 *                                                                                             *
 *	The position handed in is expected to be a point the caller's own physics already found --   *
 *	a wheel contact.  That is the whole of Section 23's "terrain/road/bridge conform": the wheel  *
 *	does not know or care which of the three it landed on, and neither does this.                 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool SurfaceRibbonSystem::Add_Point(int handle,const Vector3 & position,const Vector3 & forward,
												const Vector3 & normal,int surface_type)
{
	if ((Pool == nullptr) || (handle < 0) || (handle >= PoolSize)) {
		return false;
	}

	SurfaceRibbonClass & ribbon = Pool[handle];
	if (!ribbon.Bound || (ribbon.Definition < 0) || (ribbon.Definition >= Definitions.Count())) {
		return false;
	}

	const SurfaceRibbonDefinitionClass & def = Definitions[ribbon.Definition];

	//	Hard ground keeps no mark.  The run ends here and resumes if the emitter finds soft
	//	ground again, which is why this breaks rather than unbinds.
	if (!Ribbon_Surface_Takes_Marks(surface_type)) {
		Break(handle);
		return false;
	}

	bool starts_run = false;
	float travelled = 0.0f;

	if (!ribbon.HaveAnchor) {
		starts_run = true;
		ribbon.Distance = 0.0f;
	} else {
		Vector3 step = position - ribbon.Anchor;
		travelled = step.Length();

		if (travelled < def.Get_Segment_Length()) {
			return false;
		}

		//	A jump this large is not driving.  Teleport, respawn, a fall -- all the same thing
		//	from here, and all handled without the mover having to say so.
		if (travelled > (def.Get_Segment_Length() * def.Get_Break_Distance())) {
			starts_run = true;
			ribbon.Distance = 0.0f;
			travelled = 0.0f;
		}
	}

	/*
	**	Work out the cross edge.  Right is across the direction of travel, in the plane of the
	**	ground, so a strip laid on a slope lies on the slope instead of cutting into it.
	*/
	Vector3 up = normal;
	if (up.Length2() < WWMATH_EPSILON) {
		up.Set(0.0f,0.0f,1.0f);
	} else {
		up.Normalize();
	}

	Vector3 along = forward;
	if (along.Length2() < WWMATH_EPSILON) {
		along = (ribbon.HaveAnchor ? (position - ribbon.Anchor) : Vector3(1.0f,0.0f,0.0f));
	}
	if (along.Length2() < WWMATH_EPSILON) {
		along.Set(1.0f,0.0f,0.0f);
	}
	along.Normalize();

	Vector3 across;
	Vector3::Cross_Product(along,up,&across);
	if (across.Length2() < WWMATH_EPSILON) {
		//	Travelling straight up or down the ground normal.  Any consistent across will do.
		Vector3::Cross_Product(up,Vector3(1.0f,0.0f,0.0f),&across);
		if (across.Length2() < WWMATH_EPSILON) {
			Vector3::Cross_Product(up,Vector3(0.0f,1.0f,0.0f),&across);
		}
	}
	across.Normalize();

	const Vector3 lift = up * def.Get_Lift_Height();
	const float half = def.Get_Width() * 0.5f;

	ribbon.Distance += travelled;

	const float texture_length = (def.Get_Texture_Length() > WWMATH_EPSILON) ? def.Get_Texture_Length() : 1.0f;

	/*
	**	Push it into the ring.  When the ring is full the oldest edge leaves; nothing grows.
	*/
	int capacity = def.Get_Max_Edges();
	if (capacity > SURFACE_RIBBON_MAX_EDGES) {
		capacity = SURFACE_RIBBON_MAX_EDGES;
	}

	if (ribbon.Count >= capacity) {
		bool dropped_run_start = ribbon.Edges[ribbon.First].StartsRun;
		ribbon.First = (ribbon.First + 1) % SURFACE_RIBBON_MAX_EDGES;
		ribbon.Count--;
		//	The strip has to start somewhere, so whatever is now oldest inherits the run.
		if (dropped_run_start && (ribbon.Count > 0)) {
			ribbon.Edges[ribbon.First].StartsRun = true;
		}
	}

	RibbonEdgeClass & edge = ribbon.Edges[(ribbon.First + ribbon.Count) % SURFACE_RIBBON_MAX_EDGES];
	edge.Left		= position + lift - (across * half);
	edge.Right		= position + lift + (across * half);
	edge.V			= ribbon.Distance / texture_length;
	edge.Age			= 0.0f;
	edge.StartsRun	= starts_run;
	ribbon.Count++;

	ribbon.Anchor = position;
	ribbon.HaveAnchor = true;
	return true;
}


void SurfaceRibbonSystem::Break(int handle)
{
	if ((Pool == nullptr) || (handle < 0) || (handle >= PoolSize)) {
		return ;
	}
	//	Keeps everything already laid; only forgets where the emitter was, so the next mark
	//	starts a fresh strip instead of being joined to the last one.
	Pool[handle].HaveAnchor = false;
	return ;
}


/***********************************************************************************************
 *	SurfaceRibbonSystem::Conform_Point -- where the ground is, for callers with no contact       *
 *                                                                                             *
 *	A wheel already knows where it touched.  A scorch trail burned along the floor by a beam, or *
 *	a body being dragged, does not, and Section 23 asks marks to conform to terrain, road and    *
 *	bridge alike.  All three answers already exist, so this composes them rather than working    *
 *	any of them out again: a bridge deck wins where there is one, because a mark on a bridge     *
 *	belongs on the deck and not on the riverbed underneath it, and otherwise the road system's   *
 *	ground query answers -- heightfield terrain where a level has it, a ray through the physics  *
 *	scene where the ground is authored W3D geometry instead.                                     *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool SurfaceRibbonSystem::Conform_Point(float x,float y,float hint_z,float * height_out)
{
	float deck = 0.0f;
	if (BridgeSystem::Conform_Point(x,y,&deck)) {
		//	Only take the deck when it is not far above the caller -- driving under a bridge
		//	should mark the ground, not the underside of the span.
		if (deck <= (hint_z + 2.0f)) {
			if (height_out != nullptr) { *height_out = deck; }
			return true;
		}
	}

	float ground = 0.0f;
	if (RoadSystem::Conform_Point(x,y,&ground)) {
		if (height_out != nullptr) { *height_out = ground; }
		return true;
	}

	return false;
}


/***********************************************************************************************
 *	SurfaceRibbonSystem::Edge_Alpha -- how much of a mark is left                                *
 *                                                                                             *
 *	Two fades, and the smaller wins.  Age is the real lifetime.  Position along the ribbon is    *
 *	the donor's behaviour and is what makes a moving vehicle look like it is laying track rather *
 *	than dragging a fixed-length strip: the tail thins out behind it whatever the clock says.    *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
float SurfaceRibbonSystem::Edge_Alpha(const SurfaceRibbonClass & ribbon,int index,
												  const SurfaceRibbonDefinitionClass & def)
{
	const RibbonEdgeClass & edge = ribbon.Peek_Edge(index);

	float lifetime = def.Get_Lifetime();
	float age_alpha = 1.0f;
	if (lifetime > WWMATH_EPSILON) {
		float fade_start = def.Get_Fade_Start() * lifetime;
		if (edge.Age > fade_start) {
			float span = lifetime - fade_start;
			age_alpha = (span > WWMATH_EPSILON) ? (1.0f - ((edge.Age - fade_start) / span)) : 0.0f;
		}
	}

	//	Count back from the newest edge.  The newest Opaque_Edges are solid; the rest of the
	//	tail ramps down to nothing at the oldest edge still alive.
	float tail_alpha = 1.0f;
	int from_head = (ribbon.Count - 1) - index;
	int opaque = def.Get_Opaque_Edges();
	if (from_head > opaque) {
		int span = (ribbon.Count - 1) - opaque;
		tail_alpha = (span > 0) ? (1.0f - ((float)(from_head - opaque) / (float)span)) : 0.0f;
	}

	float alpha = (age_alpha < tail_alpha) ? age_alpha : tail_alpha;
	if (alpha < 0.0f) { alpha = 0.0f; }
	if (alpha > 1.0f) { alpha = 1.0f; }
	return alpha;
}


/***********************************************************************************************
 *	SurfaceRibbonSystem::Timestep -- age everything, drop the dead, rebuild what is left         *
 *                                                                                             *
 *	Called once per frame by PhysicsSceneClass::Update, next to the camera shakers and the       *
 *	material effects, because it is the same kind of thing: a world-level effect with no object  *
 *	of its own to be timestepped through.                                                        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void SurfaceRibbonSystem::Timestep(float dt)
{
	if ((Pool == nullptr) || (dt <= 0.0f)) {
		return ;
	}

	for (int i = 0; i < PoolSize; i++) {
		SurfaceRibbonClass & ribbon = Pool[i];
		if (ribbon.Count == 0) {
			if (!ribbon.Bound && (ribbon.Definition >= 0)) {
				ribbon.Reset();
			}
			continue;
		}

		if ((ribbon.Definition < 0) || (ribbon.Definition >= Definitions.Count())) {
			ribbon.Reset();
			continue;
		}

		const SurfaceRibbonDefinitionClass & def = Definitions[ribbon.Definition];

		for (int e = 0; e < ribbon.Count; e++) {
			ribbon.Edges[(ribbon.First + e) % SURFACE_RIBBON_MAX_EDGES].Age += dt;
		}

		while ((ribbon.Count > 0) && (ribbon.Edges[ribbon.First].Age >= def.Get_Lifetime())) {
			bool dropped_run_start = ribbon.Edges[ribbon.First].StartsRun;
			ribbon.First = (ribbon.First + 1) % SURFACE_RIBBON_MAX_EDGES;
			ribbon.Count--;
			if (dropped_run_start && (ribbon.Count > 0)) {
				ribbon.Edges[ribbon.First].StartsRun = true;
			}
		}

		if ((ribbon.Count == 0) && !ribbon.Bound) {
			ribbon.Reset();
		}
	}

	Build_Geometry();
	return ;
}


void SurfaceRibbonSystem::Clear_Marks(void)
{
	if (Pool == nullptr) {
		return ;
	}
	for (int i = 0; i < PoolSize; i++) {
		//	Empty the ring but leave the binding alone, so a vehicle that is mid-run simply
		//	starts a new strip on its next contact instead of losing its slot.
		Pool[i].First = 0;
		Pool[i].Count = 0;
		Pool[i].HaveAnchor = false;
		Pool[i].Distance = 0.0f;
		if (!Pool[i].Bound) {
			Pool[i].Reset();
		}
	}
	return ;
}


/***********************************************************************************************
 *	SurfaceRibbonSystem::Build_Geometry -- every mark of one kind, in one mesh                   *
 *                                                                                             *
 *	The mesh for a kind is made once, at its largest, and refilled in place from then on.  That  *
 *	is the difference between this and every other geometry builder in this library: the road,   *
 *	the bridge, the wood and the water are built when they are placed and then left alone, so    *
 *	they can afford to allocate a mesh the exact size they turned out to need.  A track changes  *
 *	every frame, so allocating for it every frame would be precisely the unbounded allocation    *
 *	Section 23 rules out.                                                                        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool SurfaceRibbonSystem::Build_Geometry(void)
{
	MissingTextures = 0;

	if (Pool == nullptr) {
		return false;
	}

	/*
	**	A level change replaces the physics scene.  Anything built into the last one went with
	**	it, so the references are dropped and the meshes made again against the world that is
	**	actually here -- and the marks from the previous level go too, because they were laid
	**	on ground that no longer exists.
	*/
	PhysicsSceneClass * current = PhysicsSceneClass::Get_Instance();
	if (current != BuiltScene) {
		//	Nothing was built into a scene we never had one of, and marks laid before the first
		//	build are marks in this world, not the last one -- so only a real change clears.
		if (BuiltScene != nullptr) {
			Forget_Geometry();
			Clear_Marks();
		}
		BuiltScene = current;
		return false;
	}

	bool built_anything = false;

	for (int d = 0; d < Definitions.Count(); d++) {

		const SurfaceRibbonDefinitionClass & def = Definitions[d];

		//	Is there anything of this kind worth drawing?  One edge is a line, not a strip.
		int drawable_edges = 0;
		for (int i = 0; i < PoolSize; i++) {
			if ((Pool[i].Definition == d) && (Pool[i].Count >= 2)) {
				drawable_edges += Pool[i].Count;
			}
		}

		if (drawable_edges == 0) {
			if ((d < Meshes.Count()) && (Meshes[d] != nullptr)) {
				Meshes[d]->Reset_Mesh_Counters();
				Meshes[d]->Set_Dirty();
			}
			continue;
		}

		if (!def.Names_A_Texture()) {
			//	Nothing to draw with.  Everything above still happened, so a check can see that
			//	the marks exist even though no pixel does.
			MissingTextures++;
			continue;
		}

		if (!Build_Definition_Geometry(d)) {
			continue;
		}
		built_anything = true;
	}

	return built_anything;
}


bool SurfaceRibbonSystem::Build_Definition_Geometry(int definition_index)
{
	const SurfaceRibbonDefinitionClass & def = Definitions[definition_index];

	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();

	/*
	**	Make the mesh the first time this kind has something to show, at the largest it can ever
	**	need to be: every slot in the pool, full, laying this kind of mark.
	*/
	if (Meshes[definition_index] == nullptr) {

		if (scene == nullptr) {
			return false;
		}

		/*
		**	The texture is fetched before anything is built, and no texture means nothing is
		**	built at all.  That is what keeps a dedicated server -- which has marks, ages them,
		**	and answers every query about them -- from ever allocating a mesh to draw them with,
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

		const int max_verts = PoolSize * SURFACE_RIBBON_MAX_EDGES * 2;
		const int max_polys = PoolSize * (SURFACE_RIBBON_MAX_EDGES - 1) * 2;

		DynamicMeshClass * mesh = NEW_REF(DynamicMeshClass,(max_polys,max_verts));
		if (mesh == nullptr) {
			texture->Release_Ref();
			return false;
		}

		mesh->Set_Shader(Ribbon_Shader());

		//	Lighting off, colour straight off the vertex, which is where the fade lives.
		VertexMaterialClass * vmat = VertexMaterialClass::Get_Preset(VertexMaterialClass::PRELIT_DIFFUSE);
		if (vmat != nullptr) {
			mesh->Set_Vertex_Material(vmat,true);
			vmat->Release_Ref();
		}

		mesh->Set_Texture(texture,true);
		texture->Release_Ref();

		//	A mark is paint.  Nothing collides with it, the same way nothing collides with a road.
		mesh->Set_Collision_Type(0);

		DecorationPhysClass * phys = new DecorationPhysClass;
		phys->Set_Model(mesh);

		Matrix3D identity;
		identity.Make_Identity();
		phys->Set_Transform(identity);

		//	Transient by nature: a saved game restores the vehicles, and the vehicles lay their
		//	own tracks again.  Serialising the marks would save a function of state twice.
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
	**	up and re-added every frame with it.  Only the counts and the dirty flags are reset.
	*/
	mesh->Reset_Mesh_Counters();
	mesh->Set_Dirty();

	const int max_verts = mesh->Peek_Model()->Get_Vertex_Count();
	const int max_polys = mesh->Peek_Model()->Get_Polygon_Count();
	int verts = 0;
	int polys = 0;

	for (int i = 0; i < PoolSize; i++) {

		const SurfaceRibbonClass & ribbon = Pool[i];
		if ((ribbon.Definition != definition_index) || (ribbon.Count < 2)) {
			continue;
		}

		bool in_strip = false;
		int strip_length = 0;

		for (int e = 0; e < ribbon.Count; e++) {

			const RibbonEdgeClass & edge = ribbon.Peek_Edge(e);

			if (edge.StartsRun && (e > 0)) {
				in_strip = false;
			}

			if (!in_strip) {
				//	A run of one edge draws nothing, so do not start a strip that cannot finish.
				bool has_room = ((verts + 4) <= max_verts) && ((polys + 2) <= max_polys);
				bool has_next = ((e + 1) < ribbon.Count) && !ribbon.Peek_Edge(e + 1).StartsRun;
				if (!has_room || !has_next) {
					continue;
				}
				mesh->Begin_Tri_Strip();
				in_strip = true;
				strip_length = 0;
			} else {
				if (((verts + 2) > max_verts) || ((polys + 2) > max_polys)) {
					in_strip = false;
					continue;
				}
			}

			float alpha = Edge_Alpha(ribbon,e,def);
			mesh->Set_Vertex_Color(Vector4(1.0f,1.0f,1.0f,alpha));

			mesh->Vertex(edge.Left.X, edge.Left.Y, edge.Left.Z, 0.0f,edge.V);
			mesh->Vertex(edge.Right.X,edge.Right.Y,edge.Right.Z,1.0f,edge.V);
			verts += 2;
			strip_length++;
			if (strip_length >= 2) {
				polys += 2;
			}
		}
	}

	if (Objects[definition_index] != nullptr) {
		Objects[definition_index]->Update_Cull_Box();
	}

	return (polys > 0);
}


void SurfaceRibbonSystem::Destroy_Definition_Geometry(int definition_index)
{
	if ((definition_index < 0) || (definition_index >= Objects.Count())) {
		return ;
	}

	if (Objects[definition_index] != nullptr) {
		PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
		if (scene != nullptr) {
			scene->Remove_Object(Objects[definition_index]);
		}
		Objects[definition_index]->Release_Ref();
		Objects[definition_index] = nullptr;
	}

	//	The mesh is held by the phys object, which has just let go of it.
	Meshes[definition_index] = nullptr;
	return ;
}


void SurfaceRibbonSystem::Forget_Geometry(void)
{
	for (int d = 0; d < Objects.Count(); d++) {
		if (Objects[d] != nullptr) {
			Objects[d]->Release_Ref();
			Objects[d] = nullptr;
		}
		if (d < Meshes.Count()) {
			Meshes[d] = nullptr;
		}
	}
	return ;
}


/*
**	Take the drawn objects out of the scene, but keep one slot per definition.
**
**	The slots are indexed by definition, so emptying the vectors here rather than nulling their
**	entries would leave Definitions holding five kinds and Objects holding none -- and the next
**	build would index past the end of both.  Only Shutdown, which drops the definitions in the
**	same breath, may empty them.
*/
void SurfaceRibbonSystem::Destroy_Geometry(void)
{
	for (int d = 0; d < Objects.Count(); d++) {
		Destroy_Definition_Geometry(d);
	}
	BuiltScene = nullptr;
	return ;
}


bool SurfaceRibbonSystem::Has_Geometry(int definition_index)
{
	if ((definition_index < 0) || (definition_index >= Objects.Count())) {
		return false;
	}
	return (Objects[definition_index] != nullptr);
}


/***********************************************************************************************
 *	Counts.  Get_Edge_Count against Get_Object_Count is the acceptance in two numbers.           *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
int SurfaceRibbonSystem::Get_Pool_Size(void)
{
	return PoolSize;
}


int SurfaceRibbonSystem::Get_Active_Ribbon_Count(void)
{
	if (Pool == nullptr) {
		return 0;
	}
	int count = 0;
	for (int i = 0; i < PoolSize; i++) {
		if (!Pool[i].Is_Free()) {
			count++;
		}
	}
	return count;
}


int SurfaceRibbonSystem::Get_Edge_Count(void)
{
	if (Pool == nullptr) {
		return 0;
	}
	int count = 0;
	for (int i = 0; i < PoolSize; i++) {
		count += Pool[i].Count;
	}
	return count;
}


int SurfaceRibbonSystem::Get_Object_Count(void)
{
	int count = 0;
	for (int d = 0; d < Objects.Count(); d++) {
		if (Objects[d] != nullptr) {
			count++;
		}
	}
	return count;
}


int SurfaceRibbonSystem::Get_Poly_Count(void)
{
	int count = 0;
	for (int d = 0; d < Meshes.Count(); d++) {
		if (Meshes[d] != nullptr) {
			count += Meshes[d]->Get_Num_Polys();
		}
	}
	return count;
}


int SurfaceRibbonSystem::Get_Missing_Texture_Count(void)
{
	return MissingTextures;
}


int SurfaceRibbonSystem::Get_Bind_Failure_Count(void)
{
	return BindFailures;
}
