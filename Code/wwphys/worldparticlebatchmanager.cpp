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
 *                     $Archive:: /Commando/Code/wwphys/worldparticlebatchmanager.cpp         $*
 *                                                                                             *
 *	Roadmap Section 26, Zero Hour / SAGE feature 10 -- particle batching.  See the header for    *
 *	what this owns and what it deliberately does not.                                           *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   WorldParticleBatchManager::Init -- allocate the pool, once                                 *
 *   WorldParticleBatchManager::Define_Default_Batches -- the shapes and classes Section 26 names*
 *   WorldParticleBatchManager::Allocate_Slot -- the budget policy, in one place                *
 *   WorldParticleBatchManager::Emit -- one particle, if the budget allows it                   *
 *   WorldParticleBatchManager::Timestep -- move them, retire them, refill the buffers          *
 *   WorldParticleBatchManager::Build_Geometry -- one pass over the pool, one group per texture  *
 *   WorldParticleBatchManager::Render_Group -- one submission per three hundred particles       *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "worldparticlebatchmanager.h"

#include "assetmgr.h"
#include "decophys.h"
#include "linegrp.h"
#include "phys.h"
#include "pointgr.h"
#include "pscene.h"
#include "refcount.h"
#include "rendobj.h"
#include "rinfo.h"
#include "shader.h"
#include "shadermgr.h"
#include "sharebuf.h"
#include "sphere.h"
#include "texture.h"
#include "vector4.h"
#include "w3d_file.h"
#include "ww3d.h"
#include "wwdebug.h"
#include "wwmath.h"

#include <string.h>


/***********************************************************************************************
**	File-local arithmetic.
***********************************************************************************************/

static inline float Particle_Lerp(float a,float b,float f)
{
	return a + ((b - a) * f);
}


static inline Vector3 Particle_Lerp(const Vector3 & a,const Vector3 & b,float f)
{
	return Vector3(a.X + ((b.X - a.X) * f),
						a.Y + ((b.Y - a.Y) * f),
						a.Z + ((b.Z - a.Z) * f));
}


static inline Vector3 Particle_Modulate(const Vector3 & a,const Vector3 & b)
{
	return Vector3(a.X * b.X,a.Y * b.Y,a.Z * b.Z);
}


/***********************************************************************************************
 *	Particle_Shader -- the two ways a particle is blended                                       *
 *                                                                                             *
 *	Additive is fire, sparks, muzzle flash and anything that is light rather than matter; alpha  *
 *	is smoke, dust and anything that hides what is behind it.  Both are the engine's own sprite  *
 *	presets, because a particle is exactly what those presets were written for, and both write   *
 *	no depth, which is what lets a group be drawn in one submission without the particles in it  *
 *	fighting each other.                                                                        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static ShaderClass Particle_Shader(bool additive)
{
	return additive ? ShaderClass::_PresetAdditiveSpriteShader
						 : ShaderClass::_PresetAlphaSpriteShader;
}


/***********************************************************************************************
**	ParticleBatchBufferClass -- the group, as something the scene can hold.
**
**	Deliberately almost empty.  Everything about particles is in the manager, and this exists
**	because a scene draws render objects: it carries a group index, a bounding box filled in by
**	the manager, and the sort decision every translucent render object in this engine has to
**	make.  A second copy of the particle state here is exactly the duplicate path that would
**	make the two disagree.
***********************************************************************************************/
class ParticleBatchBufferClass : public RenderObjClass
{
public:

	ParticleBatchBufferClass(int definition_index) : DefinitionIndex(definition_index) { }
	ParticleBatchBufferClass(const ParticleBatchBufferClass & src) :
		RenderObjClass(src), DefinitionIndex(src.DefinitionIndex) { }

	virtual RenderObjClass *	Clone(void) const override	{ return new ParticleBatchBufferClass(*this); }
	virtual int					Class_ID(void) const override	{ return CLASSID_PARTICLE_BATCH; }

	virtual int					Get_Num_Polys(void) const override
	{
		return WorldParticleBatchManager::Get_Group_Poly_Count(DefinitionIndex);
	}

	/*
	**	The group's particles moved, so the cached bounding volumes this object handed out last
	**	frame describe where they were.  RenderObjClass keeps that invalidation to itself and to
	**	its subclasses, which is why saying so is a method here rather than a line at the call
	**	site: the manager is not a render object and has no business reaching into one's bits.
	*/
	void							Bounds_Changed(void)
	{
		Invalidate_Cached_Bounding_Volumes();
	}

	virtual void				Render(RenderInfoClass & rinfo) override
	{
		/*
		**	The same decision ParticleBufferClass makes, for the same reason: a translucent
		**	thing drawn in the order the scene happened to reach it is a thing drawn through
		**	whatever was already in front of it.  Going on the static sort list means this is
		**	called again later with the lists disabled, which is the branch that draws.
		*/
		unsigned int sort_level = SORT_LEVEL_NONE;
		if (!WW3D::Is_Sorting_Enabled()) {
			sort_level = (unsigned int)WorldParticleBatchManager::Get_Group_Sort_Level(DefinitionIndex);
		}

		if (WW3D::Are_Static_Sort_Lists_Enabled() && (sort_level != SORT_LEVEL_NONE)) {
			WW3D::Add_To_Static_Sort_List(this,sort_level);
			return ;
		}

		WorldParticleBatchManager::Render_Group(DefinitionIndex,rinfo);
		return ;
	}

	virtual void				Get_Obj_Space_Bounding_Sphere(SphereClass & sphere) const override
	{
		AABoxClass box;
		if (!WorldParticleBatchManager::Get_Group_Bounds(DefinitionIndex,&box)) {
			sphere.Center.Set(0.0f,0.0f,0.0f);
			sphere.Radius = 0.0f;
			return ;
		}
		sphere.Center = box.Center;
		sphere.Radius = box.Extent.Length();
		return ;
	}

	virtual void				Get_Obj_Space_Bounding_Box(AABoxClass & box) const override
	{
		if (!WorldParticleBatchManager::Get_Group_Bounds(DefinitionIndex,&box)) {
			box.Center.Set(0.0f,0.0f,0.0f);
			box.Extent.Set(0.0f,0.0f,0.0f);
		}
		return ;
	}

private:

	int		DefinitionIndex;
};


/***********************************************************************************************
**	ParticleBatchGroupClass -- one texture's worth of particles, and what draws them.
**
**	The buffers are made once, at the largest the group's definition allows, and refilled in
**	place from then on.  Making them to fit would allocate every frame, which is the unbounded
**	allocation the acceptance rules out; making them at the pool size would allocate twenty-four
**	times more than any one texture can ever need.
***********************************************************************************************/
class ParticleBatchGroupClass
{
public:

	ParticleBatchGroupClass(void) :
		Points(nullptr),
		Lines(nullptr),
		Position(nullptr),
		TailPosition(nullptr),
		Diffuse(nullptr),
		TailDiffuse(nullptr),
		Size(nullptr),
		Orientation(nullptr),
		Frame(nullptr),
		Capacity(0),
		ActiveCount(0),
		ParticleCount(0),
		HasBounds(false),
		Object(nullptr),
		Phys(nullptr)
	{
		Bounds.Center.Set(0.0f,0.0f,0.0f);
		Bounds.Extent.Set(0.0f,0.0f,0.0f);
	}

	~ParticleBatchGroupClass(void)
	{
		//	The scene object has to be gone before this: it holds a reference to a render object
		//	that points back at a group index.  Destroy_Group_Geometry is what takes it out.
		WWASSERT(Object == nullptr);
		WWASSERT(Phys == nullptr);
		Release_Buffers();
	}

	bool	Has_Buffers(void) const		{ return (Capacity > 0); }

	void	Release_Buffers(void)
	{
		if (Points != nullptr) {
			//	Let go of the arrays the point group is holding references to before the arrays
			//	themselves go, so the order is ours rather than the destructor's.
			delete Points;
			Points = nullptr;
		}
		if (Lines != nullptr) {
			delete Lines;
			Lines = nullptr;
		}

		REF_PTR_RELEASE(Position);
		REF_PTR_RELEASE(TailPosition);
		REF_PTR_RELEASE(Diffuse);
		REF_PTR_RELEASE(TailDiffuse);
		REF_PTR_RELEASE(Size);
		REF_PTR_RELEASE(Orientation);
		REF_PTR_RELEASE(Frame);

		Capacity		= 0;
		ActiveCount	= 0;
		HasBounds	= false;
		return ;
	}

	PointGroupClass *						Points;			// SPRITE and POINT
	LineGroupClass *						Lines;			// STREAK

	ShareBufferClass<Vector3> *			Position;		// the head of every particle
	ShareBufferClass<Vector3> *			TailPosition;	// where it was, streaks only
	ShareBufferClass<Vector4> *			Diffuse;			// colour and alpha, this frame
	ShareBufferClass<Vector4> *			TailDiffuse;	// the trail's end, streaks only
	ShareBufferClass<float> *			Size;
	ShareBufferClass<unsigned char> *	Orientation;	// the angle, quantised, sprites only
	ShareBufferClass<unsigned char> *	Frame;			// sprite-sheet cell, when there is a sheet

	int										Capacity;		// how many the buffers hold
	int										ActiveCount;	// how many of them the last fill wrote
	int										ParticleCount;	// how many live particles name this group

	AABoxClass								Bounds;			// of the particles the last fill wrote
	bool										HasBounds;

	ParticleBatchBufferClass *		Object;			// what the scene draws
	DecorationPhysClass *				Phys;				// what the scene culls
};


/***********************************************************************************************
**	The service's state.  One pool, one group per definition, five budgets, and the counters the
**	acceptance is read out of.
***********************************************************************************************/

DynamicVectorClass<ParticleBatchDefinitionClass>	WorldParticleBatchManager::Definitions;
DynamicVectorClass<ParticleBatchGroupClass *>		WorldParticleBatchManager::Groups;

BatchParticleClass *		WorldParticleBatchManager::Pool			= nullptr;
int								WorldParticleBatchManager::PoolSize		= 0;
int								WorldParticleBatchManager::LiveCount	= 0;
int								WorldParticleBatchManager::PeakCount	= 0;

int								WorldParticleBatchManager::FreeHead		= -1;

//	Minus one is "no slot", so the lists have to be spelled out rather than zero filled.  Init
//	does it again for every class by name; this is so that the state before Init is still a
//	coherent empty rather than five lists that all believe they start at slot zero.
static_assert(PARTICLE_BUDGET_CLASS_COUNT == 5,
				  "the budget class list changed -- update the head/tail initialisers below");
int								WorldParticleBatchManager::ClassHead[PARTICLE_BUDGET_CLASS_COUNT]	= { -1,-1,-1,-1,-1 };
int								WorldParticleBatchManager::ClassTail[PARTICLE_BUDGET_CLASS_COUNT]	= { -1,-1,-1,-1,-1 };

int								WorldParticleBatchManager::ClassCount[PARTICLE_BUDGET_CLASS_COUNT]		= { 0 };
int								WorldParticleBatchManager::ClassReserve[PARTICLE_BUDGET_CLASS_COUNT]	= { 0 };
int								WorldParticleBatchManager::ClassCap[PARTICLE_BUDGET_CLASS_COUNT]		= { 0 };
int								WorldParticleBatchManager::ClassRefusals[PARTICLE_BUDGET_CLASS_COUNT]	= { 0 };

float								WorldParticleBatchManager::DrawDistance			= 220.0f;
int								WorldParticleBatchManager::Submissions			= 0;
int								WorldParticleBatchManager::DrawnParticles		= 0;
int								WorldParticleBatchManager::CulledParticles	= 0;
int								WorldParticleBatchManager::BufferAllocations	= 0;
int								WorldParticleBatchManager::Sheds					= 0;
int								WorldParticleBatchManager::Recycles				= 0;
int								WorldParticleBatchManager::MissingTextures	= 0;
int								WorldParticleBatchManager::GroupRefusals		= 0;

PhysicsSceneClass *			WorldParticleBatchManager::BuiltScene			= nullptr;


/***********************************************************************************************
 *	WorldParticleBatchManager::Init -- allocate the pool, once                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void WorldParticleBatchManager::Init(void)
{
	if (Pool != nullptr) {
		return ;
	}

	Pool		= new BatchParticleClass[PARTICLE_BATCH_POOL_SIZE];
	PoolSize	= PARTICLE_BATCH_POOL_SIZE;

	//	Every slot free, chained through the pool itself: no second array, and recycling a
	//	particle is two assignments rather than a search.
	for (int i = 0; i < PoolSize; i++) {
		Pool[i].Reset();
		Pool[i].ListNext = (i + 1 < PoolSize) ? (i + 1) : -1;
	}
	FreeHead		= (PoolSize > 0) ? 0 : -1;

	LiveCount	= 0;
	PeakCount	= 0;

	for (int c = 0; c < PARTICLE_BUDGET_CLASS_COUNT; c++) {
		ClassHead[c]		= -1;
		ClassTail[c]		= -1;
		ClassCount[c]		= 0;
		ClassRefusals[c]	= 0;
	}

	Reset_Budgets();

	Submissions			= 0;
	DrawnParticles		= 0;
	CulledParticles	= 0;
	BufferAllocations	= 0;
	Sheds					= 0;
	Recycles				= 0;
	MissingTextures	= 0;
	GroupRefusals		= 0;
	BuiltScene			= nullptr;
	return ;
}


void WorldParticleBatchManager::Shutdown(void)
{
	Clear_Particles();
	Destroy_Geometry();

	for (int i = 0; i < Groups.Count(); i++) {
		delete Groups[i];
		Groups[i] = nullptr;
	}
	Groups.Delete_All();
	Definitions.Delete_All();

	if (Pool != nullptr) {
		delete [] Pool;
		Pool = nullptr;
	}
	PoolSize		= 0;
	FreeHead		= -1;
	LiveCount	= 0;
	PeakCount	= 0;

	for (int c = 0; c < PARTICLE_BUDGET_CLASS_COUNT; c++) {
		ClassHead[c]		= -1;
		ClassTail[c]		= -1;
		ClassCount[c]		= 0;
		ClassReserve[c]	= 0;
		ClassCap[c]			= 0;
		ClassRefusals[c]	= 0;
	}

	Submissions			= 0;
	DrawnParticles		= 0;
	CulledParticles	= 0;
	BufferAllocations	= 0;
	Sheds					= 0;
	Recycles				= 0;
	MissingTextures	= 0;
	GroupRefusals		= 0;
	BuiltScene			= nullptr;
	return ;
}


/***********************************************************************************************
 *	WorldParticleBatchManager::Release_Resources -- the world went away                         *
 *                                                                                             *
 *	A particle is a thing that is happening in a place, so a new world starts with none of them, *
 *	and the buffers go with the scene that was holding their objects.  The definitions stay:     *
 *	they are tuning, not content, and the next level's explosions want the same ones.            *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void WorldParticleBatchManager::Release_Resources(void)
{
	Clear_Particles();
	Destroy_Geometry();
	BuiltScene = nullptr;
	return ;
}


/***********************************************************************************************
 *	WorldParticleBatchManager::Define_Default_Batches -- the shapes and classes Section 26 names *
 *                                                                                             *
 *	One definition per shape and per budget class, so that code which wants "a spark" can ask    *
 *	for one without knowing an asset name, and so the budget policy has something to be tested   *
 *	against before any content exists.  None of them names a texture, so none of them draws      *
 *	anything today and none of them builds a buffer -- see docs/assets/ParticleSprites.md.       *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void WorldParticleBatchManager::Define_Default_Batches(void)
{
	if (Definitions.Count() > 0) {
		return ;
	}

	struct DefaultBatchStruct {
		const char *	Name;
		int				Kind;
		int				BudgetClass;
		float				Lifetime;
		float				StartSize;
		float				EndSize;
		float				Gravity;
		float				Drag;
		bool				Additive;
		Vector3			StartColor;
		Vector3			EndColor;
	};

	static const DefaultBatchStruct _defaults[] = {

		//	What a player has to see to play: the plume over a beacon, a gas cloud, the smoke
		//	that says a building is about to go.  Given the whole pool if it ever needs it.
		{	"ow_part_signal",			PARTICLE_BATCH_SPRITE,	PARTICLE_BUDGET_CRITICAL_GAMEPLAY,
			3.0f,	0.6f,	2.5f,	-0.6f,	0.25f,	false,
			Vector3(1.0f,0.95f,0.7f),	Vector3(0.6f,0.6f,0.6f)	},

		//	The dust and smoke of a hit close enough to matter.
		{	"ow_part_impact_dust",	PARTICLE_BATCH_SPRITE,	PARTICLE_BUDGET_COMBAT_NEAR,
			1.1f,	0.25f,	1.0f,	1.5f,	0.6f,	false,
			Vector3(0.75f,0.70f,0.60f),	Vector3(0.45f,0.43f,0.40f)	},

		//	Sparks off armour.  Points rather than sprites: four hundred to a submission, and
		//	nobody has ever looked closely enough at one to see that it is a triangle.
		{	"ow_part_spark",			PARTICLE_BATCH_POINT,	PARTICLE_BUDGET_COMBAT_NEAR,
			0.6f,	0.09f,	0.02f,	9.0f,	0.1f,	true,
			Vector3(1.0f,0.85f,0.45f),	Vector3(1.0f,0.35f,0.10f)	},

		//	Something burning thrown out of an explosion, drawn as the line it travelled.
		{	"ow_part_debris_streak",PARTICLE_BATCH_STREAK,	PARTICLE_BUDGET_COMBAT_FAR,
			1.6f,	0.10f,	0.04f,	9.0f,	0.05f,	true,
			Vector3(1.0f,0.6f,0.2f),	Vector3(0.5f,0.15f,0.05f)	},

		//	Weather, wind-blown dust, the air of a place.  The layer in Section 36 fills this.
		{	"ow_part_environment",	PARTICLE_BATCH_SPRITE,	PARTICLE_BUDGET_ENVIRONMENT,
			4.0f,	0.4f,	0.6f,	0.4f,	0.35f,	false,
			Vector3(0.8f,0.8f,0.85f),	Vector3(0.8f,0.8f,0.85f)	},

		//	Embers and motes: the first thing that should stop existing when a frame gets busy.
		{	"ow_part_ember",			PARTICLE_BATCH_POINT,	PARTICLE_BUDGET_AMBIENT,
			2.5f,	0.05f,	0.02f,	-0.35f,	0.2f,	true,
			Vector3(1.0f,0.55f,0.2f),	Vector3(0.4f,0.12f,0.04f)	},
	};

	for (int i = 0; i < (int)(sizeof(_defaults) / sizeof(_defaults[0])); i++) {

		ParticleBatchDefinitionClass def;
		def.Set_Name(_defaults[i].Name);
		def.Set_Kind(_defaults[i].Kind);
		def.Set_Budget_Class(_defaults[i].BudgetClass);
		def.Set_Lifetime(_defaults[i].Lifetime);
		def.Set_Start_Size(_defaults[i].StartSize);
		def.Set_End_Size(_defaults[i].EndSize);
		def.Set_Gravity(_defaults[i].Gravity);
		def.Set_Drag(_defaults[i].Drag);
		def.Set_Additive(_defaults[i].Additive);
		def.Set_Start_Color(_defaults[i].StartColor);
		def.Set_End_Color(_defaults[i].EndColor);
		def.Set_Start_Alpha(1.0f);
		def.Set_End_Alpha(0.0f);
		def.Set_Spin_Rate((_defaults[i].Kind == PARTICLE_BATCH_SPRITE) ? 0.6f : 0.0f);
		def.Set_Streak_Seconds(0.06f);
		def.Set_Max_Particles(512);

		Define_Definition(def);
	}
	return ;
}


/***********************************************************************************************
 *	WorldParticleBatchManager::Define_Definition -- add or replace a kind of particle            *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
int WorldParticleBatchManager::Define_Definition(const ParticleBatchDefinitionClass & def)
{
	int existing = Find_Definition_Index(def.Get_Name());
	if (existing >= 0) {

		/*
		**	A redefinition can change the shape, which changes which batcher draws the group and
		**	how wide its buffers are, and it can change the budget class, which particles already
		**	in flight carry a copy of.  Rather than reason about particles half-way through a
		**	curve that has been replaced underneath them, the group starts again.
		*/
		Clear_Group(existing);
		Destroy_Group_Geometry(existing);
		Definitions[existing] = def;
		return existing;
	}

	if (Definitions.Count() >= PARTICLE_BATCH_MAX_GROUPS) {
		GroupRefusals++;
		return -1;
	}

	ParticleBatchGroupClass * group = new ParticleBatchGroupClass;
	if (group == nullptr) {
		return -1;
	}

	Definitions.Add(def);
	Groups.Add(group);
	return Definitions.Count() - 1;
}


int WorldParticleBatchManager::Get_Definition_Count(void)
{
	return Definitions.Count();
}


const ParticleBatchDefinitionClass & WorldParticleBatchManager::Peek_Definition(int index)
{
	WWASSERT((index >= 0) && (index < Definitions.Count()));
	return Definitions[index];
}


int WorldParticleBatchManager::Find_Definition_Index(const char * name)
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


ParticleBatchDefinitionClass * WorldParticleBatchManager::Find_Definition(const char * name)
{
	int index = Find_Definition_Index(name);
	return (index >= 0) ? &Definitions[index] : nullptr;
}


/***********************************************************************************************
 *	WorldParticleBatchManager::Find_Or_Define_Texture_Group -- intern a group for a texture      *
 *                                                                                             *
 *	Content names its own particle textures, so the group a caller needs usually does not exist  *
 *	until the first time it is asked for.  Interning it by texture name is what makes two        *
 *	hundred unrelated effects that happen to share a texture into one group and one submission.  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
int WorldParticleBatchManager::Find_Or_Define_Texture_Group(const char * texture_name,int kind,
																				int budget_class,float size,float lifetime)
{
	if ((texture_name == nullptr) || (texture_name[0] == 0)) {
		return -1;
	}

	//	The group is identified by what it is drawn with and how it is drawn, not by the name of
	//	whoever asked first: the same texture drawn as a sprite and as a streak is two groups.
	for (int i = 0; i < Definitions.Count(); i++) {
		if ((Definitions[i].Get_Kind() == kind)
			&& (Definitions[i].Get_Budget_Class() == budget_class)
			&& (::stricmp(Definitions[i].Get_Texture(),texture_name) == 0)) {
			return i;
		}
	}

	ParticleBatchDefinitionClass def;

	StringClass name;
	name.Format("%s_%s_%s",texture_name,Particle_Batch_Kind_Name(kind),
					Particle_Budget_Class_Name(budget_class));
	def.Set_Name(name);
	def.Set_Texture(texture_name);
	def.Set_Kind(kind);
	def.Set_Budget_Class(budget_class);
	def.Set_Start_Size(size);
	def.Set_End_Size(size * 1.5f);
	def.Set_Lifetime(lifetime);

	return Define_Definition(def);
}


bool WorldParticleBatchManager::Set_Definition_Texture(int index,const char * texture_name)
{
	if ((index < 0) || (index >= Definitions.Count())) {
		return false;
	}

	Definitions[index].Set_Texture(texture_name);

	/*
	**	The buffers hold no texture, so they stay.  The render object exists only because a
	**	texture resolved, so it goes, and the next fill builds it again against whatever was just
	**	named.  Dropping only the object means the particles in flight keep flying and appear the
	**	moment it comes back, which is what makes naming a texture at the console something you
	**	can watch happen.
	*/
	if ((index < Groups.Count()) && (Groups[index] != nullptr)) {
		ParticleBatchGroupClass * group = Groups[index];
		if (group->Phys != nullptr) {
			PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
			if ((scene != nullptr) && (scene == BuiltScene)) {
				scene->Remove_Object(group->Phys);
			}
			group->Phys->Release_Ref();
			group->Phys = nullptr;
		}
		group->Object = nullptr;

		//	The texture lives on the batcher as well as on the object.
		if (group->Points != nullptr) {
			group->Points->Set_Texture(nullptr);
		}
		if (group->Lines != nullptr) {
			group->Lines->Set_Texture(nullptr);
		}
	}
	return true;
}


/***********************************************************************************************
**	The budgets.
***********************************************************************************************/

void WorldParticleBatchManager::Reset_Budgets(void)
{
	int reserved = 0;
	for (int c = 0; c < PARTICLE_BUDGET_CLASS_COUNT; c++) {
		ClassReserve[c]	= Particle_Budget_Class_Default_Reserve(c);
		ClassCap[c]			= Particle_Budget_Class_Default_Cap(c);
		reserved += ClassReserve[c];
	}

	//	A reservation is a promise that a class can always have that many particles.  If the
	//	promises add up to more than the pool they are not promises, and the policy below would
	//	refuse particles it believed it had room for.
	WWASSERT(reserved <= PARTICLE_BATCH_POOL_SIZE);
	return ;
}


bool WorldParticleBatchManager::Set_Budget(int budget_class,int reserve,int cap)
{
	if ((budget_class < 0) || (budget_class >= PARTICLE_BUDGET_CLASS_COUNT)) {
		return false;
	}

	if (reserve < 0) { reserve = 0; }
	if (cap < 0) { cap = 0; }
	if (cap > PARTICLE_BATCH_POOL_SIZE) { cap = PARTICLE_BATCH_POOL_SIZE; }
	if (reserve > cap) { reserve = cap; }

	//	The reservations of every other class still have to fit, or this one is being given room
	//	that has already been promised away.  Refused rather than clamped: a caller tuning a
	//	budget wants to be told it asked for something impossible.
	int reserved_elsewhere = 0;
	for (int c = 0; c < PARTICLE_BUDGET_CLASS_COUNT; c++) {
		if (c != budget_class) {
			reserved_elsewhere += ClassReserve[c];
		}
	}
	if ((reserved_elsewhere + reserve) > PARTICLE_BATCH_POOL_SIZE) {
		return false;
	}

	ClassReserve[budget_class]	= reserve;
	ClassCap[budget_class]		= cap;

	//	Lowering a cap below what the class already holds does not evict anything here.  The
	//	particles age out within their own lifetimes, which is seconds, and tearing live
	//	particles out of the world because somebody typed a number is a worse surprise than a
	//	budget that takes a moment to take effect.
	return true;
}


int WorldParticleBatchManager::Get_Budget_Reserve(int budget_class)
{
	if ((budget_class < 0) || (budget_class >= PARTICLE_BUDGET_CLASS_COUNT)) {
		return 0;
	}
	return ClassReserve[budget_class];
}


int WorldParticleBatchManager::Get_Budget_Cap(int budget_class)
{
	if ((budget_class < 0) || (budget_class >= PARTICLE_BUDGET_CLASS_COUNT)) {
		return 0;
	}
	return ClassCap[budget_class];
}


int WorldParticleBatchManager::Get_Class_Particle_Count(int budget_class)
{
	if ((budget_class < 0) || (budget_class >= PARTICLE_BUDGET_CLASS_COUNT)) {
		return 0;
	}
	return ClassCount[budget_class];
}


int WorldParticleBatchManager::Get_Class_Headroom(int budget_class)
{
	if ((budget_class < 0) || (budget_class >= PARTICLE_BUDGET_CLASS_COUNT)) {
		return 0;
	}

	//	What this class could still have: whichever runs out first, its own cap or the pool.
	int by_cap	= ClassCap[budget_class] - ClassCount[budget_class];
	int by_pool	= PoolSize - LiveCount;
	if (by_cap < 0) { by_cap = 0; }
	if (by_pool < 0) { by_pool = 0; }
	return (by_cap < by_pool) ? by_cap : by_pool;
}


int WorldParticleBatchManager::Get_Reserved_Total(void)
{
	int reserved = 0;
	for (int c = 0; c < PARTICLE_BUDGET_CLASS_COUNT; c++) {
		reserved += ClassReserve[c];
	}
	return reserved;
}


int WorldParticleBatchManager::Get_Headroom(void)
{
	int headroom = PoolSize - LiveCount;
	return (headroom > 0) ? headroom : 0;
}


/***********************************************************************************************
**	The pool.
***********************************************************************************************/

void WorldParticleBatchManager::Link_Into_Class(int slot,int budget_class)
{
	//	Appended at the tail, because the tail is the youngest and the head is what gets shed.
	Pool[slot].ListNext = -1;
	Pool[slot].ListPrev = ClassTail[budget_class];

	if (ClassTail[budget_class] >= 0) {
		Pool[ClassTail[budget_class]].ListNext = slot;
	} else {
		ClassHead[budget_class] = slot;
	}
	ClassTail[budget_class] = slot;
	ClassCount[budget_class]++;
	return ;
}


void WorldParticleBatchManager::Unlink_From_Class(int slot,int budget_class)
{
	int prev = Pool[slot].ListPrev;
	int next = Pool[slot].ListNext;

	if (prev >= 0) {
		Pool[prev].ListNext = next;
	} else {
		ClassHead[budget_class] = next;
	}

	if (next >= 0) {
		Pool[next].ListPrev = prev;
	} else {
		ClassTail[budget_class] = prev;
	}

	Pool[slot].ListNext = -1;
	Pool[slot].ListPrev = -1;
	ClassCount[budget_class]--;
	return ;
}


int WorldParticleBatchManager::Find_Oldest_In_Class(int budget_class)
{
	if ((budget_class < 0) || (budget_class >= PARTICLE_BUDGET_CLASS_COUNT)) {
		return -1;
	}
	return ClassHead[budget_class];
}


/***********************************************************************************************
 *	WorldParticleBatchManager::Find_Oldest_In_Group -- the one scan this service does            *
 *                                                                                             *
 *	A group at its own cap recycles its own oldest particle, and unlike the budget classes a     *
 *	group has no list of its own: there are up to twenty-four of them and keeping twenty-four    *
 *	orderings up to date on every spawn and every death would cost more than the scan does,      *
 *	because this path is only reached by a group that is already at its ceiling.                 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
int WorldParticleBatchManager::Find_Oldest_In_Group(int definition_index)
{
	int oldest = -1;
	float oldest_age = -1.0f;

	for (int i = 0; i < PoolSize; i++) {
		if (!Pool[i].InUse || (Pool[i].Definition != definition_index)) {
			continue;
		}
		if (Pool[i].Age > oldest_age) {
			oldest_age	= Pool[i].Age;
			oldest		= i;
		}
	}
	return oldest;
}


/***********************************************************************************************
 *	WorldParticleBatchManager::Allocate_Slot -- the budget policy, in one place                  *
 *                                                                                             *
 *	Three questions in order, and the order is the policy.                                      *
 *                                                                                             *
 *	A group at its own ceiling recycles its own oldest particle.  An emitter at its cap should   *
 *	keep looking alive without getting bigger, and taking the slot from somewhere else would let *
 *	one effect's ceiling be paid for by another effect.                                         *
 *                                                                                             *
 *	Otherwise, if the pool has a free slot, that is the answer and nothing is disturbed.         *
 *                                                                                             *
 *	Otherwise the pool is full, and the least important class that has anything above its        *
 *	reservation loses its oldest particle.  Only classes strictly less important than this one   *
 *	are asked, and only for what they hold above their reservation, so an ambient ember can      *
 *	never take a slot from a beacon's plume and a class inside its promise is never touched.     *
 *	If nothing can be shed the particle does not happen and is counted, because a service that   *
 *	quietly drops work is worse than one that admits it.                                        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
int WorldParticleBatchManager::Allocate_Slot(int definition_index,int budget_class)
{
	if ((Pool == nullptr) || (definition_index < 0) || (definition_index >= Definitions.Count())) {
		return -1;
	}
	if ((budget_class < 0) || (budget_class >= PARTICLE_BUDGET_CLASS_COUNT)) {
		return -1;
	}

	ParticleBatchGroupClass * group = Groups[definition_index];
	WWASSERT(group != nullptr);

	/*
	**	The group's own ceiling.  A loop rather than a single eviction because a ceiling can be
	**	lowered under particles that already exist, and then the group has to come down to it
	**	rather than hover one above it forever.  Each turn releases a particle, so this ends.
	*/
	while (group->ParticleCount >= Definitions[definition_index].Get_Max_Particles()) {
		int victim = Find_Oldest_In_Group(definition_index);
		if (victim < 0) {
			//	A ceiling of nothing: the group has been turned off.
			ClassRefusals[budget_class]++;
			return -1;
		}
		Release_Slot(victim);
		Recycles++;
	}

	//	This class's own ceiling, the same way.  Checked after the group's so that a definition
	//	with a small ceiling recycles within itself rather than being told the class is full.
	while (ClassCount[budget_class] >= ClassCap[budget_class]) {
		int victim = Find_Oldest_In_Class(budget_class);
		if (victim < 0) {
			//	A cap of zero: the class has been turned off.
			ClassRefusals[budget_class]++;
			return -1;
		}
		Release_Slot(victim);
		Recycles++;
	}

	//	Room in the pool.
	if (FreeHead < 0) {

		//	Full.  Shed from the bottom up.
		int victim = -1;
		for (int c = PARTICLE_BUDGET_CLASS_COUNT - 1; c > budget_class; c--) {
			if (ClassCount[c] > ClassReserve[c]) {
				victim = Find_Oldest_In_Class(c);
				if (victim >= 0) {
					break;
				}
			}
		}

		if (victim < 0) {
			ClassRefusals[budget_class]++;
			return -1;
		}

		Release_Slot(victim);
		Sheds++;
	}

	int slot = FreeHead;
	WWASSERT(slot >= 0);
	FreeHead = Pool[slot].ListNext;

	Pool[slot].Reset();
	Pool[slot].InUse			= true;
	Pool[slot].Definition	= definition_index;
	Pool[slot].BudgetClass	= budget_class;

	Link_Into_Class(slot,budget_class);
	group->ParticleCount++;

	LiveCount++;
	if (LiveCount > PeakCount) {
		PeakCount = LiveCount;
	}
	return slot;
}


void WorldParticleBatchManager::Release_Slot(int slot)
{
	if ((Pool == nullptr) || (slot < 0) || (slot >= PoolSize) || !Pool[slot].InUse) {
		return ;
	}

	int budget_class = Pool[slot].BudgetClass;
	if ((budget_class >= 0) && (budget_class < PARTICLE_BUDGET_CLASS_COUNT)) {
		Unlink_From_Class(slot,budget_class);
	}

	int definition_index = Pool[slot].Definition;
	if ((definition_index >= 0) && (definition_index < Groups.Count())
		&& (Groups[definition_index] != nullptr)) {
		Groups[definition_index]->ParticleCount--;
		WWASSERT(Groups[definition_index]->ParticleCount >= 0);
	}

	Pool[slot].Reset();
	Pool[slot].ListNext = FreeHead;
	FreeHead = slot;

	LiveCount--;
	WWASSERT(LiveCount >= 0);
	return ;
}


/***********************************************************************************************
 *	WorldParticleBatchManager::Emit -- one particle, if the budget allows it                     *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool WorldParticleBatchManager::Emit(int definition_index,const Vector3 & position,
												const Vector3 & velocity,const Vector3 & tint,float size_scale)
{
	if ((Pool == nullptr) || (definition_index < 0) || (definition_index >= Definitions.Count())) {
		return false;
	}

	const ParticleBatchDefinitionClass & def = Definitions[definition_index];

	int slot = Allocate_Slot(definition_index,def.Get_Budget_Class());
	if (slot < 0) {
		return false;
	}

	BatchParticleClass & particle = Pool[slot];

	particle.Position	= position;
	particle.Velocity	= velocity;
	particle.Tint		= tint;
	particle.SizeScale	= (size_scale > 0.0f) ? size_scale : 1.0f;
	particle.Lifetime	= def.Get_Lifetime();
	particle.Age		= 0.0f;

	//	The state the renderer reads, correct from the first frame rather than from the first
	//	timestep: something emitted after the physics step still has to be drawable this frame.
	particle.Color	= Particle_Modulate(def.Get_Start_Color(),particle.Tint);
	particle.Alpha	= def.Get_Start_Alpha();
	particle.Size	= def.Get_Start_Size() * particle.SizeScale;

	/*
	**	A sprite that is not spun starts at a fixed angle, and a hundred identical sprites at a
	**	fixed angle read as one sprite drawn badly.  The slot index is what scatters them -- no
	**	random number generator, so a replay of the same emissions looks the same.
	*/
	particle.Angle = (def.Get_Kind() == PARTICLE_BATCH_SPRITE)
						? ((float)(slot & 0xFF) * (WWMATH_PI * 2.0f / 256.0f))
						: 0.0f;

	return true;
}


/***********************************************************************************************
 *	WorldParticleBatchManager::Emit_Burst -- what an explosion asks for                          *
 *                                                                                             *
 *	Directions are the Fibonacci spiral on the sphere rather than random: the coverage is more   *
 *	even than a random set of the same size, and it is the same burst every time, which means a  *
 *	check can say what the result should be and a demo replays as the thing that was recorded.   *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
int WorldParticleBatchManager::Emit_Burst(int definition_index,const Vector3 & center,int count,
														float speed,float spread_radius,
														const Vector3 & tint,float size_scale)
{
	if (count <= 0) {
		return 0;
	}

	static const float GOLDEN_ANGLE = 2.39996323f;

	int made = 0;
	for (int i = 0; i < count; i++) {

		float z = (count == 1) ? 0.0f : (1.0f - (2.0f * ((float)i + 0.5f) / (float)count));
		float r2 = 1.0f - (z * z);
		float r = (r2 > 0.0f) ? WWMath::Sqrt(r2) : 0.0f;
		float theta = GOLDEN_ANGLE * (float)i;

		Vector3 dir(r * WWMath::Cos(theta),r * WWMath::Sin(theta),z);

		if (Emit(definition_index,center + (dir * spread_radius),dir * speed,tint,size_scale)) {
			made++;
		}
	}
	return made;
}


void WorldParticleBatchManager::Clear_Particles(void)
{
	if (Pool == nullptr) {
		return ;
	}
	for (int i = 0; i < PoolSize; i++) {
		if (Pool[i].InUse) {
			Release_Slot(i);
		}
	}

	for (int d = 0; d < Groups.Count(); d++) {
		if (Groups[d] != nullptr) {
			Groups[d]->ActiveCount	= 0;
			Groups[d]->HasBounds		= false;
		}
	}
	Submissions			= 0;
	DrawnParticles		= 0;
	CulledParticles	= 0;
	return ;
}


void WorldParticleBatchManager::Clear_Group(int definition_index)
{
	if ((Pool == nullptr) || (definition_index < 0)) {
		return ;
	}
	for (int i = 0; i < PoolSize; i++) {
		if (Pool[i].InUse && (Pool[i].Definition == definition_index)) {
			Release_Slot(i);
		}
	}
	if ((definition_index < Groups.Count()) && (Groups[definition_index] != nullptr)) {
		Groups[definition_index]->ActiveCount	= 0;
		Groups[definition_index]->HasBounds		= false;
	}
	return ;
}


/***********************************************************************************************
 *	WorldParticleBatchManager::Timestep -- move them, retire them, refill the buffers            *
 *                                                                                             *
 *	One pass over one array for the simulation and one for the fill, whatever the world is       *
 *	doing.  There is no per-particle object to step, no per-effect update to dispatch and no     *
 *	allocation in either pass, which between them are what the acceptance is about.             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void WorldParticleBatchManager::Timestep(float dt)
{
	if (Pool == nullptr) {
		return ;
	}

	if (dt > 0.0f) {
		for (int i = 0; i < PoolSize; i++) {

			BatchParticleClass & particle = Pool[i];
			if (!particle.InUse) {
				continue;
			}

			if ((particle.Definition < 0) || (particle.Definition >= Definitions.Count())) {
				Release_Slot(i);
				continue;
			}

			particle.Age += dt;
			if (particle.Age >= particle.Lifetime) {
				Release_Slot(i);
				continue;
			}

			const ParticleBatchDefinitionClass & def = Definitions[particle.Definition];

			//	Gravity is stated as a downward acceleration, so smoke is a definition with a
			//	negative one rather than a second mechanism for things that rise.
			particle.Velocity.Z -= def.Get_Gravity() * dt;

			float drag = def.Get_Drag();
			if (drag > 0.0f) {
				float keep = 1.0f - (drag * dt);
				if (keep < 0.0f) { keep = 0.0f; }
				particle.Velocity *= keep;
			}

			particle.Position += particle.Velocity * dt;
			particle.Angle += def.Get_Spin_Rate() * dt;

			//	The four things the renderer reads, worked out once here rather than once per
			//	frame in a fill that may run more than once, or not at all.
			float f = (particle.Lifetime > WWMATH_EPSILON) ? (particle.Age / particle.Lifetime) : 1.0f;
			if (f < 0.0f) { f = 0.0f; }
			if (f > 1.0f) { f = 1.0f; }

			particle.Color = Particle_Modulate(Particle_Lerp(def.Get_Start_Color(),
																			def.Get_End_Color(),f),particle.Tint);
			particle.Alpha = Particle_Lerp(def.Get_Start_Alpha(),def.Get_End_Alpha(),f);
			particle.Size  = Particle_Lerp(def.Get_Start_Size(),def.Get_End_Size(),f) * particle.SizeScale;
		}
	}

	Build_Geometry();
	return ;
}


/***********************************************************************************************
**	Geometry.
***********************************************************************************************/

/***********************************************************************************************
 *	WorldParticleBatchManager::Build_Group_Buffers -- made once, at the group's largest          *
 *                                                                                             *
 *	No device and no asset manager are involved: these are plain arrays and the engine's own     *
 *	point and line batchers, which hold no device state until they are asked to draw.  That is   *
 *	deliberate -- it is what lets the batching arithmetic be checked without a graphics device,  *
 *	and what keeps a dedicated server, which emits nothing, from allocating anything at all.     *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool WorldParticleBatchManager::Build_Group_Buffers(int definition_index)
{
	if ((definition_index < 0) || (definition_index >= Groups.Count())) {
		return false;
	}

	ParticleBatchGroupClass * group = Groups[definition_index];
	if (group == nullptr) {
		return false;
	}
	if (group->Has_Buffers()) {
		return true;
	}

	const ParticleBatchDefinitionClass & def = Definitions[definition_index];
	const int capacity = def.Get_Max_Particles();
	if (capacity <= 0) {
		return false;
	}

	group->Position	= NEW_REF(ShareBufferClass<Vector3>,(capacity));
	group->Diffuse		= NEW_REF(ShareBufferClass<Vector4>,(capacity));
	group->Size			= NEW_REF(ShareBufferClass<float>,(capacity));

	if (def.Get_Kind() == PARTICLE_BATCH_STREAK) {

		group->TailPosition	= NEW_REF(ShareBufferClass<Vector3>,(capacity));
		group->TailDiffuse	= NEW_REF(ShareBufferClass<Vector4>,(capacity));

		group->Lines = new LineGroupClass;
		group->Lines->Set_Flag(LineGroupClass::TRANSFORM,true);
		group->Lines->Set_Shader(Particle_Shader(def.Is_Additive()));
		group->Lines->Set_Line_Mode(LineGroupClass::TETRAHEDRON);

	} else {

		//	A sprite can be spun and can use a sprite sheet; a point can do neither, and buying
		//	it the arrays for both would be paying for a feature it does not have.
		if (def.Get_Kind() == PARTICLE_BATCH_SPRITE) {
			group->Orientation = NEW_REF(ShareBufferClass<unsigned char>,(capacity));
			if (def.Get_Frame_Grid_Log2() > 0) {
				group->Frame = NEW_REF(ShareBufferClass<unsigned char>,(capacity));
			}
		}

		group->Points = new PointGroupClass;
		group->Points->Set_Flag(PointGroupClass::TRANSFORM,true);
		group->Points->Set_Shader(Particle_Shader(def.Is_Additive()));
		group->Points->Set_Point_Mode((def.Get_Kind() == PARTICLE_BATCH_SPRITE)
												? PointGroupClass::QUADS : PointGroupClass::TRIS);
		group->Points->Set_Frame_Row_Column_Count_Log2(def.Get_Frame_Grid_Log2());
		group->Points->Set_Point_Frame(0);
		if (group->Orientation == nullptr) {
			group->Points->Set_Point_Orientation(0);
		}
	}

	group->Capacity = capacity;
	BufferAllocations++;
	return true;
}


/***********************************************************************************************
 *	WorldParticleBatchManager::Attach_Group_Object -- put the group on the screen                 *
 *                                                                                             *
 *	This is the half that needs a world and a texture, and the reason it is separate from the    *
 *	buffers: a group whose definition names no texture still holds particles, still ages them    *
 *	and still fills its buffers, so everything about it can be counted on a machine that cannot  *
 *	draw.  It simply never reaches a screen, which is what a missing texture should cost.        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void WorldParticleBatchManager::Attach_Group_Object(int definition_index)
{
	ParticleBatchGroupClass * group = Groups[definition_index];
	const ParticleBatchDefinitionClass & def = Definitions[definition_index];

	if ((group == nullptr) || !group->Has_Buffers()) {
		return ;
	}

	if (group->Phys != nullptr) {
		return ;
	}

	if (!def.Names_A_Texture()) {
		MissingTextures++;
		return ;
	}

	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
	if (scene == nullptr) {
		return ;
	}

	if (WW3DAssetManager::Get_Instance() == nullptr) {
		MissingTextures++;
		return ;
	}

	TextureClass * texture = WW3DAssetManager::Get_Instance()->Get_Texture(def.Get_Texture());
	if (texture == nullptr) {
		MissingTextures++;
		return ;
	}

	if (group->Points != nullptr) {
		group->Points->Set_Texture(texture);
	}
	if (group->Lines != nullptr) {
		group->Lines->Set_Texture(texture);
	}
	texture->Release_Ref();

	ParticleBatchBufferClass * object = new ParticleBatchBufferClass(definition_index);
	if (object == nullptr) {
		return ;
	}

	/*
	**	A particle is light and dust: nothing collides with it and nothing casts a shadow from
	**	it.  That needs no declaring -- RenderObjClass answers every collision cast with "no" by
	**	default and this class does not override one, and a DecorationPhysClass is not in the
	**	scene's collision lists to begin with.
	*/
	DecorationPhysClass * phys = new DecorationPhysClass;
	phys->Set_Model(object);

	Matrix3D identity;
	identity.Make_Identity();
	phys->Set_Transform(identity);

	//	Client-side and transient: nothing here is networked, because a particle is a function of
	//	an event every machine already saw, and nothing here is saved, because a loaded game has
	//	not been shot at yet.
	phys->Enable_Dont_Save(true);

	//	The object was released into the physics object when it was set as its model, so the
	//	pointer kept below is a pointer and not a reference.
	object->Release_Ref();

	scene->Add_Dynamic_Object(phys);

	group->Object			= object;
	group->Phys				= phys;
	return ;
}


/***********************************************************************************************
 *	WorldParticleBatchManager::Build_Geometry -- one pass over the pool, one group per texture    *
 *                                                                                             *
 *	The acceptance lives here.  Every live particle is written into the buffers of its own group  *
 *	in a single pass over the pool -- not a pass per group, which would be the pool times the     *
 *	groups -- and a group is then submitted once per three hundred particles it holds.  A         *
 *	firefight that produces two hundred separate effects sharing one texture is one group, and    *
 *	the submissions are counted in particles seen rather than in effects happening.              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool WorldParticleBatchManager::Build_Geometry(void)
{
	MissingTextures	= 0;
	Submissions			= 0;
	DrawnParticles		= 0;
	CulledParticles	= 0;

	if (Pool == nullptr) {
		return false;
	}

	/*
	**	A level change replaces the physics scene.  Anything built into the last one went with
	**	it, so the references are dropped without asking the new scene to remove objects it never
	**	held, and the particles go too -- they were thrown in a world that is not here any more.
	*/
	PhysicsSceneClass * current = PhysicsSceneClass::Get_Instance();
	if (current != BuiltScene) {
		if (BuiltScene != nullptr) {
			Forget_Geometry();
			Clear_Particles();
		}
		BuiltScene = current;
		return false;
	}

	//	Begin the fill.
	for (int d = 0; d < Groups.Count(); d++) {
		if (Groups[d] != nullptr) {
			Groups[d]->ActiveCount	= 0;
			Groups[d]->HasBounds		= false;
		}
	}

	/*
	**	The group culls as one object through the spatial index like anything else in the scene.
	**	This is the finer cut inside it: a particle far enough away to be less than a pixel is
	**	left out of the buffer entirely, so a level with four thousand particles spread across it
	**	fills the buffers with the ones near the camera rather than all of them.
	*/
	bool distance_cull = (DrawDistance > 0.0f) && (current != nullptr);
	Vector3 view_point(0.0f,0.0f,0.0f);
	if (distance_cull) {
		view_point = current->Get_Last_Camera_Position();
	}
	float cull_distance2 = DrawDistance * DrawDistance;

	//	Indexed by group below, which is only sound because Define_Definition is the one place a
	//	group can come from and it refuses the twenty-fifth.
	WWASSERT(Groups.Count() <= PARTICLE_BATCH_MAX_GROUPS);
	Vector3 group_min[PARTICLE_BATCH_MAX_GROUPS];
	Vector3 group_max[PARTICLE_BATCH_MAX_GROUPS];

	for (int i = 0; i < PoolSize; i++) {

		const BatchParticleClass & particle = Pool[i];
		if (!particle.InUse) {
			continue;
		}

		const int d = particle.Definition;
		if ((d < 0) || (d >= Groups.Count())) {
			continue;
		}

		ParticleBatchGroupClass * group = Groups[d];
		if (group == nullptr) {
			continue;
		}

		if (distance_cull) {
			Vector3 delta = particle.Position - view_point;
			if (delta.Length2() > cull_distance2) {
				CulledParticles++;
				continue;
			}
		}

		//	Lazily, and once: a definition nothing has emitted costs no memory at all.
		if (!group->Has_Buffers() && !Build_Group_Buffers(d)) {
			continue;
		}

		if (group->ActiveCount >= group->Capacity) {
			//	Cannot happen while the group's cap and its buffer width are the same number,
			//	and is counted rather than asserted in case somebody makes them differ.
			CulledParticles++;
			continue;
		}

		const ParticleBatchDefinitionClass & def = Definitions[d];
		const int index = group->ActiveCount;

		group->Position->Set_Element(index,particle.Position);
		group->Diffuse->Set_Element(index,Vector4(particle.Color.X,particle.Color.Y,
																particle.Color.Z,particle.Alpha));
		group->Size->Set_Element(index,particle.Size);

		if (group->Orientation != nullptr) {
			//	Two hundred and fifty six discrete angles, which is what the point group's
			//	precalculated vertex offsets are indexed by.  Truncation toward zero and a mask
			//	between them handle a particle that has spun past a full turn in either
			//	direction without a floating point modulo.
			int ticks = (int)(particle.Angle * (256.0f / (WWMATH_PI * 2.0f)));
			group->Orientation->Set_Element(index,(unsigned char)(ticks & 0xFF));
		}

		if (group->Frame != nullptr) {
			//	A sprite sheet animates over the particle's own life: 2^n by 2^n cells, played
			//	once, never wrapping back to the first cell at the moment it dies.
			const int frames = 1 << (2 * (int)def.Get_Frame_Grid_Log2());
			float f = (particle.Lifetime > WWMATH_EPSILON) ? (particle.Age / particle.Lifetime) : 0.0f;
			int frame = (int)(f * (float)frames);
			if (frame < 0) { frame = 0; }
			if (frame > (frames - 1)) { frame = frames - 1; }
			group->Frame->Set_Element(index,(unsigned char)frame);
		}

		if (group->TailPosition != nullptr) {
			//	A streak is drawn from where the particle is to where it was a moment ago, so the
			//	trail is its own velocity and needs nothing remembered from last frame.
			Vector3 tail = particle.Position - (particle.Velocity * def.Get_Streak_Seconds());
			group->TailPosition->Set_Element(index,tail);
			group->TailDiffuse->Set_Element(index,Vector4(particle.Color.X,particle.Color.Y,
																		particle.Color.Z,0.0f));
		}

		//	The bounds the scene culls the whole group by.  Grown by the particle's own size
		//	because a sprite is drawn around its position, not at it.
		Vector3 low	= particle.Position;
		Vector3 high = particle.Position;
		if (group->TailPosition != nullptr) {
			Vector3 tail = group->TailPosition->Get_Element(index);
			low.Update_Min(tail);
			high.Update_Max(tail);
		}
		float reach = particle.Size;
		low  -= Vector3(reach,reach,reach);
		high += Vector3(reach,reach,reach);

		if (!group->HasBounds) {
			group_min[d]			= low;
			group_max[d]			= high;
			group->HasBounds		= true;
		} else {
			group_min[d].Update_Min(low);
			group_max[d].Update_Max(high);
		}

		group->ActiveCount++;
		DrawnParticles++;
	}

	//	End the fill: hand each group's arrays to its batcher, count what that will cost, and
	//	make sure the thing that draws it exists and knows how big it has become.
	bool filled_anything = false;

	for (int d = 0; d < Groups.Count(); d++) {

		ParticleBatchGroupClass * group = Groups[d];
		if ((group == nullptr) || !group->Has_Buffers()) {
			continue;
		}

		const ParticleBatchDefinitionClass & def = Definitions[d];

		if (group->HasBounds) {
			group->Bounds.Init_Min_Max(group_min[d],group_max[d]);
		} else {
			group->Bounds.Extent.Set(0.0f,0.0f,0.0f);
		}

		if (group->Points != nullptr) {
			group->Points->Set_Arrays(group->Position,group->Diffuse,nullptr,group->Size,
												group->Orientation,group->Frame,group->ActiveCount);
		}
		if (group->Lines != nullptr) {
			group->Lines->Set_Arrays(group->Position,group->TailPosition,group->Diffuse,
											 group->TailDiffuse,nullptr,group->Size,nullptr,
											 group->ActiveCount);
		}

		if (group->ActiveCount > 0) {

			const int per_submission = Particle_Batch_Kind_Points_Per_Submission(def.Get_Kind());
			Submissions += (group->ActiveCount + per_submission - 1) / per_submission;

			Attach_Group_Object(d);
			filled_anything = true;
		}

		if (group->Object != nullptr) {
			group->Object->Bounds_Changed();
		}
		if (group->Phys != nullptr) {
			group->Phys->Update_Cull_Box();
		}
	}

	return filled_anything;
}


bool WorldParticleBatchManager::Has_Geometry(int definition_index)
{
	if ((definition_index < 0) || (definition_index >= Groups.Count())) {
		return false;
	}
	return ((Groups[definition_index] != nullptr) && Groups[definition_index]->Has_Buffers());
}


bool WorldParticleBatchManager::Has_Object(int definition_index)
{
	if ((definition_index < 0) || (definition_index >= Groups.Count())) {
		return false;
	}
	return ((Groups[definition_index] != nullptr) && (Groups[definition_index]->Phys != nullptr));
}


void WorldParticleBatchManager::Destroy_Group_Geometry(int definition_index)
{
	if ((definition_index < 0) || (definition_index >= Groups.Count())) {
		return ;
	}

	ParticleBatchGroupClass * group = Groups[definition_index];
	if (group == nullptr) {
		return ;
	}

	if (group->Phys != nullptr) {
		PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
		if ((scene != nullptr) && (scene == BuiltScene)) {
			scene->Remove_Object(group->Phys);
		}
		group->Phys->Release_Ref();
		group->Phys = nullptr;
	}

	//	No reference to give back here -- the physics object holds the only one.
	group->Object = nullptr;

	group->Release_Buffers();
	return ;
}


void WorldParticleBatchManager::Destroy_Geometry(void)
{
	for (int d = 0; d < Groups.Count(); d++) {
		Destroy_Group_Geometry(d);
	}
	return ;
}


/***********************************************************************************************
 *	WorldParticleBatchManager::Forget_Geometry -- the scene took it with it                      *
 *                                                                                             *
 *	The objects belonged to a scene that no longer exists, so the references are dropped without  *
 *	asking the new scene to remove things it never held.                                        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void WorldParticleBatchManager::Forget_Geometry(void)
{
	for (int d = 0; d < Groups.Count(); d++) {

		ParticleBatchGroupClass * group = Groups[d];
		if (group == nullptr) {
			continue;
		}

		if (group->Phys != nullptr) {
			group->Phys->Release_Ref();
			group->Phys = nullptr;
		}
		group->Object = nullptr;

		group->Release_Buffers();
	}
	return ;
}


void WorldParticleBatchManager::Set_Draw_Distance(float distance)
{
	DrawDistance = distance;
	return ;
}


float WorldParticleBatchManager::Get_Draw_Distance(void)
{
	return DrawDistance;
}


/***********************************************************************************************
**	Accounting.
***********************************************************************************************/

int WorldParticleBatchManager::Get_Pool_Size(void)
{
	return PoolSize;
}


int WorldParticleBatchManager::Get_Particle_Count(void)
{
	return LiveCount;
}


int WorldParticleBatchManager::Get_Group_Particle_Count(int definition_index)
{
	if ((definition_index < 0) || (definition_index >= Groups.Count())
		|| (Groups[definition_index] == nullptr)) {
		return 0;
	}
	return Groups[definition_index]->ParticleCount;
}


int WorldParticleBatchManager::Get_Drawn_Particle_Count(void)
{
	return DrawnParticles;
}


int WorldParticleBatchManager::Get_Culled_Particle_Count(void)
{
	return CulledParticles;
}


int WorldParticleBatchManager::Get_Submission_Count(void)
{
	return Submissions;
}


int WorldParticleBatchManager::Get_Object_Count(void)
{
	int count = 0;
	for (int d = 0; d < Groups.Count(); d++) {
		if ((Groups[d] != nullptr) && (Groups[d]->Phys != nullptr)) {
			count++;
		}
	}
	return count;
}


int WorldParticleBatchManager::Get_Poly_Count(void)
{
	int polys = 0;
	for (int d = 0; d < Groups.Count(); d++) {
		polys += Get_Group_Poly_Count(d);
	}
	return polys;
}


int WorldParticleBatchManager::Get_Group_Poly_Count(int definition_index)
{
	if ((definition_index < 0) || (definition_index >= Groups.Count())
		|| (Groups[definition_index] == nullptr)) {
		return 0;
	}

	const ParticleBatchGroupClass * group = Groups[definition_index];
	switch (Definitions[definition_index].Get_Kind())
	{
		//	A quad is two triangles, a point is one, and a streak is a tetrahedron's four.
		case PARTICLE_BATCH_SPRITE:	return group->ActiveCount * 2;
		case PARTICLE_BATCH_POINT:		return group->ActiveCount;
		case PARTICLE_BATCH_STREAK:	return group->ActiveCount * 4;
		default:								return 0;
	}
}


int WorldParticleBatchManager::Get_Buffer_Allocation_Count(void)
{
	return BufferAllocations;
}


int WorldParticleBatchManager::Get_Refusal_Count(void)
{
	int refusals = 0;
	for (int c = 0; c < PARTICLE_BUDGET_CLASS_COUNT; c++) {
		refusals += ClassRefusals[c];
	}
	return refusals;
}


int WorldParticleBatchManager::Get_Class_Refusal_Count(int budget_class)
{
	if ((budget_class < 0) || (budget_class >= PARTICLE_BUDGET_CLASS_COUNT)) {
		return 0;
	}
	return ClassRefusals[budget_class];
}


int WorldParticleBatchManager::Get_Shed_Count(void)
{
	return Sheds;
}


int WorldParticleBatchManager::Get_Recycle_Count(void)
{
	return Recycles;
}


int WorldParticleBatchManager::Get_Missing_Texture_Count(void)
{
	return MissingTextures;
}


int WorldParticleBatchManager::Get_Group_Refusal_Count(void)
{
	return GroupRefusals;
}


int WorldParticleBatchManager::Get_Peak_Particle_Count(void)
{
	return PeakCount;
}


/***********************************************************************************************
**	The render path.
***********************************************************************************************/

bool WorldParticleBatchManager::Get_Group_Bounds(int definition_index,AABoxClass * box)
{
	if (box == nullptr) {
		return false;
	}
	if ((definition_index < 0) || (definition_index >= Groups.Count())
		|| (Groups[definition_index] == nullptr)) {
		return false;
	}

	const ParticleBatchGroupClass * group = Groups[definition_index];
	if (!group->HasBounds) {
		return false;
	}

	*box = group->Bounds;
	return true;
}


int WorldParticleBatchManager::Get_Group_Sort_Level(int definition_index)
{
	if ((definition_index < 0) || (definition_index >= Definitions.Count())) {
		return SORT_LEVEL_NONE;
	}
	return Particle_Shader(Definitions[definition_index].Is_Additive()).Guess_Sort_Level();
}


/***********************************************************************************************
 *	WorldParticleBatchManager::Render_Group -- one submission per three hundred particles         *
 *                                                                                             *
 *	The arrays were filled and handed to the batcher by Build_Geometry, so there is nothing to   *
 *	work out here: this is the draw.  The particle material program owns the device state around  *
 *	it, which is roadmap Section 15's arrangement and the reason a particle no longer inherits    *
 *	whatever material the last thing drawn happened to leave behind.                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void WorldParticleBatchManager::Render_Group(int definition_index,RenderInfoClass & rinfo)
{
	if ((definition_index < 0) || (definition_index >= Groups.Count())) {
		return ;
	}

	ParticleBatchGroupClass * group = Groups[definition_index];
	if ((group == nullptr) || (group->ActiveCount <= 0)) {
		return ;
	}

	const bool programmed = ShaderManagerClass::Is_Supported(MATERIAL_PROGRAM_PARTICLE);
	if (programmed) {
		ShaderManagerClass::Set_Program(MATERIAL_PROGRAM_PARTICLE,0);
	}

	if (group->Points != nullptr) {
		group->Points->Render(rinfo);
	}
	if (group->Lines != nullptr) {
		group->Lines->Render(rinfo);
	}

	if (programmed) {
		ShaderManagerClass::Reset_Program();
	}
	return ;
}
