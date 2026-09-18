//
//	The shader and state management layer, checked from inside the binary that has to be right.
//	See shaderselfcheck.h for why the checks live here.
//

#include "shaderselfcheck.h"

#include "shadermgr.h"
#include "worldparticlemanager.h"

#include "camera.h"
#include "rinfo.h"
#include "vector3.h"
#include "vector4.h"

#include "refcount.h"
#include "shader.h"
#include "vertmaterial.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

namespace {

int	_Failures = 0;

void	Check (bool condition, const char *format, ...)
{
	if (condition) {
		return ;
	}

	va_list args;
	va_start (args, format);
	::fprintf (stderr, "FAIL: ");
	::vfprintf (stderr, format, args);
	::fprintf (stderr, "\n");
	va_end (args);

	_Failures ++;
	return ;
}


/*
**	A program that reports what was done to it.
**
**	Counting is the only way to answer the questions that matter here.  "Was the previous
**	program reset" is not the same as "is some program reset", and "reset once" is not the
**	same as "reset", which is what a layer that leaks device state looks like from outside.
*/
class CountingProgramClass : public MaterialProgramClass
{
public:
	CountingProgramClass (const char *name, int passes, ShaderTierType minimum)
		:	Name (name),
			Passes (passes),
			Minimum (minimum)
	{
		Live ++;
	}

	virtual ~CountingProgramClass (void) override		{ Live --; }

	virtual const char *	Get_Name (void) const override	{ return Name; }
	virtual int				Get_Pass_Count (void) const override	{ return Passes; }

	virtual bool	Init (ShaderTierType tier) override
	{
		Inits ++;
		return (tier >= Minimum);
	}

	virtual void	Shutdown (void) override		{ Shutdowns ++; }
	virtual void	Set_Pass (int pass) override	{ Sets ++; LastPass = pass; }
	virtual void	Reset (void) override			{ Resets ++; }

	const char *	Name;
	int				Passes;
	ShaderTierType	Minimum;

	int				Inits = 0;
	int				Shutdowns = 0;
	int				Sets = 0;
	int				Resets = 0;
	int				LastPass = -1;

	//	How many of these exist.  A program the manager refuses has to be destroyed by the
	//	manager, and the only way to see that from here is to watch the population.
	static int		Live;
};

int	CountingProgramClass::Live = 0;


/***********************************************************************************************
**	programs -- registration, tiers, passes, and who owns the device state
***********************************************************************************************/

int	Run_Programs (void)
{
	//
	//	There is no device in this process, and that is the answer a dedicated server gets
	//	too.  Fixed function is what every program has to be able to fall back to, so it is
	//	the safe reading as well as the true one.
	//
	Check (ShaderManagerClass::Detect_Tier () == SHADER_TIER_FIXED_FUNCTION,
			"a process with no device detected a programmable tier");

	//
	//	Every program has a name, and no two share one.  A missing entry in the name table
	//	shows up here rather than in a log six months from now.
	//
	for (int i=0; i<MATERIAL_PROGRAM_COUNT; i++) {

		const char *name = ShaderManagerClass::Get_Program_Name ((MaterialProgramType)i);
		Check (name != nullptr && name[0] != 0, "program %d has no name", i);

		for (int j=i+1; j<MATERIAL_PROGRAM_COUNT; j++) {
			const char *other = ShaderManagerClass::Get_Program_Name ((MaterialProgramType)j);
			Check (::strcmp (name, other) != 0, "programs %d and %d are both called %s", i, j, name);
		}
	}

	ShaderManagerClass::Init (SHADER_TIER_FIXED_FUNCTION);

	Check (ShaderManagerClass::Is_Initialized (), "the manager did not come up");
	Check (ShaderManagerClass::Get_Tier () == SHADER_TIER_FIXED_FUNCTION,
			"the manager ignored the tier it was given");

	//
	//	Stock W3D content is a program, not an exception.  One pass, always present, and
	//	it is what a Renegade model has always been drawn with.
	//
	Check (ShaderManagerClass::Is_Supported (MATERIAL_PROGRAM_LEGACY_W3D),
			"stock W3D materials have no program");
	Check (ShaderManagerClass::Get_Pass_Count (MATERIAL_PROGRAM_LEGACY_W3D) == 1,
			"stock W3D materials want %d passes, not 1",
			ShaderManagerClass::Get_Pass_Count (MATERIAL_PROGRAM_LEGACY_W3D));

	//
	//	Debug overlays build a material in Init, which is why registering them here -- in a
	//	process with no device -- is worth checking rather than assuming.
	//
	Check (ShaderManagerClass::Is_Supported (MATERIAL_PROGRAM_DEBUG_OVERLAY),
			"debug overlays have no program");
	Check (ShaderManagerClass::Get_Pass_Count (MATERIAL_PROGRAM_DEBUG_OVERLAY) == 1,
			"debug overlays want %d passes, not 1",
			ShaderManagerClass::Get_Pass_Count (MATERIAL_PROGRAM_DEBUG_OVERLAY));

	//
	//	Exactly the pipelines whose consumers exist are registered, and no others.  Both
	//	halves matter: a pipeline missing means a system draws in whatever state it inherits,
	//	and a pipeline registered early is a claim that something draws it, which Is_Supported
	//	is how a caller asks.  An unregistered one reports no passes so that a caller looping
	//	over them draws nothing rather than drawing it wrongly.
	//
	static const MaterialProgramType	_expected[] = {
		MATERIAL_PROGRAM_LEGACY_W3D,
		MATERIAL_PROGRAM_TERRAIN,
		MATERIAL_PROGRAM_ROAD,
		MATERIAL_PROGRAM_BRIDGE,
		MATERIAL_PROGRAM_WATER,
		MATERIAL_PROGRAM_FOLIAGE,
		MATERIAL_PROGRAM_PROJECTED_SHADOW,
		MATERIAL_PROGRAM_DEBUG_OVERLAY,
	};

	for (int i=0; i<MATERIAL_PROGRAM_COUNT; i++) {

		bool expected = false;
		for (int e=0; e<(int)(sizeof(_expected)/sizeof(_expected[0])); e++) {
			if (_expected[e] == i) { expected = true; break; }
		}

		const char *name = ShaderManagerClass::Get_Program_Name ((MaterialProgramType)i);

		if (expected) {
			Check (ShaderManagerClass::Is_Supported ((MaterialProgramType)i),
					"%s has a consumer and no program", name);
			Check (ShaderManagerClass::Get_Pass_Count ((MaterialProgramType)i) >= 1,
					"%s is registered and reports no passes", name);
		} else {
			Check (ShaderManagerClass::Is_Supported ((MaterialProgramType)i) == false,
					"%s is registered and nothing draws it yet", name);
			Check (ShaderManagerClass::Get_Pass_Count ((MaterialProgramType)i) == 0,
					"%s reports passes without being registered", name);
		}
	}

	//
	//	A program that wants hardware this tier does not have declines, and declining is a
	//	normal answer: it is not registered, and the manager destroys it rather than leaving
	//	the caller holding something it was told to hand over.  Offered into a slot nothing
	//	claims, because registering over a registered pipeline is a caller mistake the manager
	//	asserts on rather than a case worth proving.
	//
	const int	live_before = CountingProgramClass::Live;

	CountingProgramClass *	wants_shaders = new CountingProgramClass ("wants_shaders", 1, SHADER_TIER_PROGRAMMABLE);
	const bool					kept = ShaderManagerClass::Register_Program (MATERIAL_PROGRAM_PARTICLE, wants_shaders);

	Check (kept == false, "a program that declined the tier was registered anyway");
	Check (ShaderManagerClass::Is_Supported (MATERIAL_PROGRAM_PARTICLE) == false,
			"a declined program is still supported");
	Check (CountingProgramClass::Live == live_before,
			"a declined program was not destroyed: %d live, expected %d",
			CountingProgramClass::Live, live_before);

	//
	//	Two programs that do fit, so that the handover between them can be watched.
	//
	CountingProgramClass *	ghost = new CountingProgramClass ("ghost", 1, SHADER_TIER_FIXED_FUNCTION);
	CountingProgramClass *	overlay = new CountingProgramClass ("marker", 2, SHADER_TIER_FIXED_FUNCTION);

	Check (ShaderManagerClass::Register_Program (MATERIAL_PROGRAM_GHOST_TINT, ghost),
			"a program that fits the tier was refused");
	Check (ShaderManagerClass::Register_Program (MATERIAL_PROGRAM_STATUS_MARKER, overlay),
			"a multipass program that fits the tier was refused");

	Check (ghost->Inits == 1, "the manager called Init %d times, not once", ghost->Inits);
	Check (ShaderManagerClass::Get_Pass_Count (MATERIAL_PROGRAM_STATUS_MARKER) == 2,
			"a two pass program reported %d passes",
			ShaderManagerClass::Get_Pass_Count (MATERIAL_PROGRAM_STATUS_MARKER));

	//
	//	Nothing is current until something is set, and resetting nothing is harmless.
	//
	Check (ShaderManagerClass::Get_Current_Program () == MATERIAL_PROGRAM_COUNT,
			"something was current before anything was set");
	ShaderManagerClass::Reset_Program ();
	Check (ghost->Resets == 0, "resetting nothing reset a program");

	ShaderManagerClass::Set_Program (MATERIAL_PROGRAM_GHOST_TINT, 0);

	Check (ShaderManagerClass::Get_Current_Program () == MATERIAL_PROGRAM_GHOST_TINT,
			"the program that was set is not the current one");
	Check (ShaderManagerClass::Get_Current_Pass () == 0, "the current pass is not the one that was set");
	Check (ghost->Sets == 1, "Set_Pass ran %d times for one pass", ghost->Sets);
	Check (ghost->Resets == 0, "setting a program reset it");

	//
	//	Moving between passes of one program does not reset it.  A program setting up its
	//	own second pass knows what its first one did, and making it start from nothing
	//	every time would be the layer getting in the way of the pipeline it exists to serve.
	//
	ShaderManagerClass::Set_Program (MATERIAL_PROGRAM_STATUS_MARKER, 0);
	ShaderManagerClass::Set_Program (MATERIAL_PROGRAM_STATUS_MARKER, 1);

	Check (ghost->Resets == 1, "handing over to another program reset the old one %d times, not once",
			ghost->Resets);
	Check (overlay->Sets == 2, "two passes ran Set_Pass %d times", overlay->Sets);
	Check (overlay->Resets == 0, "moving between passes of one program reset it");
	Check (overlay->LastPass == 1, "the second pass arrived as pass %d", overlay->LastPass);
	Check (ShaderManagerClass::Get_Current_Pass () == 1, "the current pass is not the second one");

	//
	//	And giving the device back resets exactly once, however often it is asked.
	//
	ShaderManagerClass::Reset_Program ();
	ShaderManagerClass::Reset_Program ();

	Check (overlay->Resets == 1, "giving the device back reset %d times, not once", overlay->Resets);
	Check (ShaderManagerClass::Get_Current_Program () == MATERIAL_PROGRAM_COUNT,
			"a program is still current after the device was given back");

	//
	//	Staged textures survive being staged and are given back on request.  Nothing here
	//	holds a real texture -- that needs a device -- but the stage bookkeeping does not.
	//
	Check (ShaderManagerClass::Peek_Texture (0) == nullptr, "a stage held a texture before anything staged one");
	ShaderManagerClass::Set_Texture (0, nullptr);
	Check (ShaderManagerClass::Peek_Texture (0) == nullptr, "staging nothing produced something");
	ShaderManagerClass::Reset_Textures ();

	//
	//	Bringing the manager up again tears down what was there.  Every registered program
	//	is shut down and destroyed, and what comes back is a fresh registry holding stock
	//	W3D content and nothing else.
	//
	ShaderManagerClass::Init (SHADER_TIER_PROGRAMMABLE);

	Check (CountingProgramClass::Live == live_before,
			"re-initialising left %d programs alive, expected %d",
			CountingProgramClass::Live, live_before);
	Check (ShaderManagerClass::Get_Tier () == SHADER_TIER_PROGRAMMABLE,
			"re-initialising ignored the new tier");
	Check (ShaderManagerClass::Is_Supported (MATERIAL_PROGRAM_LEGACY_W3D),
			"stock W3D materials lost their program on re-initialisation");
	Check (ShaderManagerClass::Is_Supported (MATERIAL_PROGRAM_DEBUG_OVERLAY),
			"debug overlays lost their program on re-initialisation");
	Check (ShaderManagerClass::Is_Supported (MATERIAL_PROGRAM_GHOST_TINT) == false,
			"a program survived re-initialisation");
	Check (ShaderManagerClass::Get_Current_Program () == MATERIAL_PROGRAM_COUNT,
			"something was current after re-initialisation");

	ShaderManagerClass::Shutdown ();

	Check (ShaderManagerClass::Is_Initialized () == false, "the manager did not go down");
	Check (ShaderManagerClass::Is_Supported (MATERIAL_PROGRAM_LEGACY_W3D) == false,
			"a program outlived the manager");
	Check (ShaderManagerClass::Is_Supported (MATERIAL_PROGRAM_DEBUG_OVERLAY) == false,
			"the debug overlay program outlived the manager");

	return _Failures;
}


/***********************************************************************************************
**	pipelines -- what each registered pipeline actually draws in
**
**	The pipelines are worth pinning rather than merely counting.  The whole claim of this layer
**	is that "how is a road surface drawn" has one answer; a check that only asked whether the
**	road program existed would pass just as happily if that answer quietly became foliage's.
***********************************************************************************************/

int	Run_Pipelines (void)
{
	//
	//	The manager is down when this starts, which is the state a dedicated server is in: it
	//	builds the same road, bridge, water and foliage geometry a client does and never
	//	reaches the device init that normally starts this class.  Asking for pipeline state
	//	must therefore bring it up, because nothing here is a device resource.
	//
	ShaderManagerClass::Shutdown ();
	Check (ShaderManagerClass::Is_Initialized () == false, "the manager did not go down before this check");

	ShaderClass				shader;
	VertexMaterialClass *	material = nullptr;

	Check (ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_ROAD, 0, &shader, &material),
			"a pipeline could not be asked for its state with the manager down");
	Check (ShaderManagerClass::Is_Initialized (),
			"asking a pipeline for its state did not bring the manager up");
	Check (material != nullptr, "the road pipeline has no material");
	REF_PTR_RELEASE (material);

	//
	//	The material comes back referenced.  A pipeline that handed out a borrowed pointer
	//	would be handing out something a re-initialisation can delete underneath a mesh that
	//	is still drawing with it.
	//
	VertexMaterialClass *	first = nullptr;
	VertexMaterialClass *	second = nullptr;

	ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_ROAD, 0, nullptr, &first);
	Check (first != nullptr, "the road pipeline stopped having a material");

	if (first != nullptr) {
		const int	refs_before = first->Num_Refs ();
		ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_ROAD, 0, nullptr, &second);
		Check (second == first, "the road pipeline handed out two different materials");
		Check (first->Num_Refs () == refs_before + 1,
				"asking for pipeline state took %d references, not one",
				first->Num_Refs () - refs_before);
	}

	REF_PTR_RELEASE (second);
	REF_PTR_RELEASE (first);

	//
	//	An unregistered pipeline answers no, and says so without leaving a material behind for
	//	the caller to release.  A pass that does not exist answers no as well.
	//
	material = nullptr;
	Check (ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_GHOST_TINT, 0, &shader, &material) == false,
			"an unregistered pipeline answered with state");
	Check (material == nullptr, "an unregistered pipeline handed out a material");

	Check (ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_ROAD, 1, &shader, &material) == false,
			"a one pass pipeline answered for a second pass");
	Check (ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_ROAD, -1, &shader, &material) == false,
			"a pipeline answered for a negative pass");

	//
	//	Terrain is two passes and they differ in exactly one thing.  That is how stock Renegade
	//	composites ground and it is why the two exist: the first layer with any influence on a
	//	quad draws opaque and the rest blend over it.  Both are fogged, because a hillside that
	//	stopped being fogged where its second layer started would be a visible seam.
	//
	Check (ShaderManagerClass::Get_Pass_Count (MATERIAL_PROGRAM_TERRAIN) == 2,
			"terrain wants %d passes, not 2", ShaderManagerClass::Get_Pass_Count (MATERIAL_PROGRAM_TERRAIN));

	ShaderClass	base_shader, layer_shader;
	ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_TERRAIN, TERRAIN_PASS_BASE, &base_shader, nullptr);
	ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_TERRAIN, TERRAIN_PASS_LAYER, &layer_shader, nullptr);

	Check (base_shader.Get_Fog_Func () == ShaderClass::FOG_ENABLE, "the terrain base pass is not fogged");
	Check (layer_shader.Get_Fog_Func () == ShaderClass::FOG_ENABLE, "the terrain layer pass is not fogged");
	Check (base_shader.Get_Dst_Blend_Func () == ShaderClass::DSTBLEND_ZERO,
			"the terrain base pass blends instead of drawing opaque");
	Check (layer_shader.Get_Dst_Blend_Func () == ShaderClass::DSTBLEND_ONE_MINUS_SRC_ALPHA,
			"the terrain layer pass does not blend over what is under it");

	//
	//	And the base pass has two materials, because whether the ground's colour comes from the
	//	light environment or from a baked vertex is a fact about the patch and not about
	//	terrain.  The layer pass always takes it from the vertex -- that is where its blend
	//	weight lives -- so the vertex-lit base is the same material the layer uses.
	//
	VertexMaterialClass *	lit_base = nullptr;
	VertexMaterialClass *	vertex_base = nullptr;
	VertexMaterialClass *	layer = nullptr;

	ShaderManagerClass::Set_Program_Variant (MATERIAL_PROGRAM_TERRAIN, TERRAIN_VARIANT_LIT);
	ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_TERRAIN, TERRAIN_PASS_BASE, nullptr, &lit_base);

	ShaderManagerClass::Set_Program_Variant (MATERIAL_PROGRAM_TERRAIN, TERRAIN_VARIANT_VERTEX_LIT);
	ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_TERRAIN, TERRAIN_PASS_BASE, nullptr, &vertex_base);

	ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_TERRAIN, TERRAIN_PASS_LAYER, nullptr, &layer);

	Check (lit_base != nullptr && vertex_base != nullptr && layer != nullptr,
			"a terrain pass has no material");
	Check (lit_base != vertex_base, "the terrain variant switch changed nothing");
	Check (vertex_base == layer, "the vertex-lit base is not the material the layer draws with");

	//	And back, so a later caller does not inherit this check's choice.
	ShaderManagerClass::Set_Program_Variant (MATERIAL_PROGRAM_TERRAIN, TERRAIN_VARIANT_LIT);

	VertexMaterialClass *	restored = nullptr;
	ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_TERRAIN, TERRAIN_PASS_BASE, nullptr, &restored);
	Check (restored == lit_base, "the terrain variant did not switch back");
	REF_PTR_RELEASE (restored);

	REF_PTR_RELEASE (layer);
	REF_PTR_RELEASE (vertex_base);
	REF_PTR_RELEASE (lit_base);

	//
	//	Water and foliage are the pair the design contrasts, and they must not converge.  Water
	//	is a translucent surface that blends over what is beneath it and must not write depth,
	//	or it stops the things beneath it being drawn.  Foliage is cut-out art: it has an edge
	//	to cut around rather than a surface to blend, so it alpha tests and does write depth.
	//	Both are two-sided, for different reasons -- a lake seen from below, a leaf sheet seen
	//	from behind.
	//
	ShaderClass	water_shader, foliage_shader;
	ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_WATER, 0, &water_shader, nullptr);
	ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_FOLIAGE, 0, &foliage_shader, nullptr);

	Check (water_shader.Get_Alpha_Test () == ShaderClass::ALPHATEST_DISABLE, "water alpha tests");
	Check (water_shader.Get_Depth_Mask () == ShaderClass::DEPTH_WRITE_DISABLE, "water writes depth");
	Check (water_shader.Get_Src_Blend_Func () == ShaderClass::SRCBLEND_SRC_ALPHA,
			"water does not blend by its own alpha");
	Check (water_shader.Get_Dst_Blend_Func () == ShaderClass::DSTBLEND_ONE_MINUS_SRC_ALPHA,
			"water does not blend over what is beneath it");
	Check (water_shader.Get_Cull_Mode () == ShaderClass::CULL_MODE_DISABLE, "water is one-sided");

	Check (foliage_shader.Get_Alpha_Test () == ShaderClass::ALPHATEST_ENABLE, "foliage does not alpha test");
	Check (foliage_shader.Get_Depth_Mask () == ShaderClass::DEPTH_WRITE_ENABLE, "foliage does not write depth");
	Check (foliage_shader.Get_Cull_Mode () == ShaderClass::CULL_MODE_DISABLE, "foliage is one-sided");

	//
	//	Roads and bridges are solid ground with the light baked in.  Nothing blends, nothing is
	//	cut around, and both write depth, because a vehicle behind a bridge pier is behind it.
	//
	const MaterialProgramType	_solid[] = { MATERIAL_PROGRAM_ROAD, MATERIAL_PROGRAM_BRIDGE };

	for (int i=0; i<2; i++) {

		ShaderClass	solid;
		VertexMaterialClass *	solid_material = nullptr;
		const char *name = ShaderManagerClass::Get_Program_Name (_solid[i]);

		Check (ShaderManagerClass::Get_Pass_State (_solid[i], 0, &solid, &solid_material),
				"%s has no pass state", name);
		Check (solid.Get_Alpha_Test () == ShaderClass::ALPHATEST_DISABLE, "%s alpha tests", name);
		Check (solid.Get_Depth_Mask () == ShaderClass::DEPTH_WRITE_ENABLE, "%s does not write depth", name);
		Check (solid.Get_Dst_Blend_Func () == ShaderClass::DSTBLEND_ZERO, "%s blends", name);
		Check (solid_material != nullptr, "%s has no material", name);

		REF_PTR_RELEASE (solid_material);
	}

	//
	//	The projected shadow accumulation pass builds coverage, not a picture: every triangle
	//	of a caster contributes wherever it lands, regardless of what already did, so it is
	//	additive with depth ignored in both directions and no texture.  It sets no material,
	//	which is a real answer and not an oversight -- this pass never set one, and inventing
	//	one here would be the layer changing a picture rather than owning the state behind it.
	//
	ShaderClass	shadow_shader;
	VertexMaterialClass *	shadow_material = nullptr;

	Check (ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_PROJECTED_SHADOW, 0, &shadow_shader, &shadow_material),
			"the projected shadow pipeline has no pass state");
	Check (shadow_material == nullptr, "the projected shadow pass acquired a material");
	Check (shadow_shader.Get_Src_Blend_Func () == ShaderClass::SRCBLEND_ONE, "shadow coverage is not additive");
	Check (shadow_shader.Get_Dst_Blend_Func () == ShaderClass::DSTBLEND_ONE, "shadow coverage is not additive");
	Check (shadow_shader.Get_Depth_Mask () == ShaderClass::DEPTH_WRITE_DISABLE, "shadow coverage writes depth");
	Check (shadow_shader.Get_Depth_Compare () == ShaderClass::PASS_ALWAYS, "shadow coverage tests depth");
	Check (shadow_shader.Get_Fog_Func () == ShaderClass::FOG_DISABLE, "shadow coverage is fogged");
	Check (shadow_shader.Get_Texturing () == ShaderClass::TEXTURING_DISABLE, "shadow coverage is textured");
	Check (shadow_shader.Get_Cull_Mode () == ShaderClass::CULL_MODE_DISABLE, "shadow coverage is one-sided");

	//
	//	Stock W3D content is the one registered program with nothing to hand out.  A Renegade
	//	material carries its own shader and its own vertex material and the renderer applies
	//	them as it draws, so a pipeline state for it would be this layer inventing an opinion
	//	about a model exported in 2002.
	//
	material = nullptr;
	Check (ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_LEGACY_W3D, 0, &shader, &material) == false,
			"stock W3D content was given a pipeline state of its own");
	Check (material == nullptr, "stock W3D content handed out a material");

	//
	//	Every pipeline answers for every pass it claims, and for none beyond them.
	//
	for (int i=0; i<MATERIAL_PROGRAM_COUNT; i++) {

		const MaterialProgramType	type = (MaterialProgramType)i;
		const int						passes = ShaderManagerClass::Get_Pass_Count (type);

		if ((passes == 0) || (type == MATERIAL_PROGRAM_LEGACY_W3D) || (type == MATERIAL_PROGRAM_DEBUG_OVERLAY)) {
			continue;
		}

		for (int pass=0; pass<passes; pass++) {
			VertexMaterialClass *	any = nullptr;
			Check (ShaderManagerClass::Get_Pass_State (type, pass, nullptr, &any),
					"%s could not answer for pass %d of %d",
					ShaderManagerClass::Get_Program_Name (type), pass, passes);
			REF_PTR_RELEASE (any);
		}

		Check (ShaderManagerClass::Get_Pass_State (type, passes, nullptr, nullptr) == false,
				"%s answered for one pass more than it claims",
				ShaderManagerClass::Get_Program_Name (type));
	}

	ShaderManagerClass::Shutdown ();
	return _Failures;
}



/***********************************************************************************************
**	particles -- the shared pools, the batch key and the budget
**
**	Section 26's acceptance is a claim about numbers -- that a large firefight does not explode
**	draw calls or allocations -- and numbers are what this checks.  It never draws anything: a
**	flush with no device does every part of its work except the draw, so the batch a submission
**	lands in, the table its particles are gathered through and the budget that paid for them are
**	all decided and counted here exactly as they would be on a screen.
***********************************************************************************************/

namespace {

enum { PARTICLE_CHECK_MAX = 4096 };

/*
**	Fixed rather than allocated, so the check owns no heap across its own exit -- a self check
**	that leaks is a self check that reports whatever the leak tracker decides to do about it
**	rather than what it was asked.
*/
Vector3			_CheckPositions[PARTICLE_CHECK_MAX];
Vector4			_CheckDiffuse[PARTICLE_CHECK_MAX];
bool				_CheckArraysBuilt = false;

void	Build_Check_Arrays (void)
{
	if (_CheckArraysBuilt) {
		return ;
	}

	for (int i = 0; i < PARTICLE_CHECK_MAX; i++) {
		_CheckPositions[i].Set ((float)i, 0.0f, 0.0f);
		_CheckDiffuse[i].Set (0.5f, 0.5f, 0.5f, 1.0f);
	}

	_CheckArraysBuilt = true;
	return ;
}

/*
**	One emitter's worth of particles, described the way a ParticleBufferClass with no property
**	arrays describes itself -- which is the common case in stock content and the case where the
**	batch key has to work hardest.
*/
ParticleSubmissionStruct	Make_Submission (int count, int budget, float distance, bool with_diffuse)
{
	ParticleSubmissionStruct	submission;

	submission.Texture = nullptr;
	submission.PointMode = 1;					// PointGroupClass::QUADS
	submission.LineMode = 0;					// LineGroupClass::TETRAHEDRON
	submission.Position = _CheckPositions;
	submission.TailPosition = _CheckPositions;
	submission.Diffuse = with_diffuse ? _CheckDiffuse : nullptr;
	submission.Count = count;
	submission.Budget = budget;
	submission.Center.Set (distance, 0.0f, 0.0f);

	return submission;
}

}	// namespace


int	Run_Particles (void)
{
	Build_Check_Arrays ();

	CameraClass				camera;
	RenderInfoClass		rinfo (camera);

	WorldParticleManager::Init ();
	WorldParticleManager::Set_Enabled (true);
	WorldParticleManager::Set_Pool_Capacity (PARTICLE_POOL_SPRITE, 1024);
	WorldParticleManager::Set_Pool_Capacity (PARTICLE_POOL_STREAK, 256);

	int	capacity = WorldParticleManager::Get_Pool_Capacity (PARTICLE_POOL_SPRITE);
	Check (capacity == 1024, "the sprite pool was sized to %d and not 1024", capacity);

	/*
	**	The acceptance, as a number.  Sixty-four emitters that share a material are sixty-four
	**	draw calls in the stock engine and one here, and the ratio is the point: what a batch
	**	counts is materials, not effects.
	*/
	WorldParticleManager::Reset_Statistics ();

	for (int i = 0; i < 64; i++) {
		ParticleSubmissionStruct	submission = Make_Submission (8, PARTICLE_BUDGET_COMBAT_NEAR, 0.0f, false);
		Check (WorldParticleManager::Submit (PARTICLE_POOL_SPRITE, submission),
				"the pool refused submission %d of a material it had already taken", i);
	}

	Check (WorldParticleManager::Get_Pending_Batches (PARTICLE_POOL_SPRITE) == 1,
			"sixty-four emitters sharing one material opened %d batches",
			WorldParticleManager::Get_Pending_Batches (PARTICLE_POOL_SPRITE));

	WorldParticleManager::Flush (rinfo);

	Check (WorldParticleManager::Get_Submission_Count (PARTICLE_POOL_SPRITE) == 64,
			"%d emitters submitted, not 64",
			WorldParticleManager::Get_Submission_Count (PARTICLE_POOL_SPRITE));
	Check (WorldParticleManager::Get_Batch_Count (PARTICLE_POOL_SPRITE) == 1,
			"sixty-four emitters drew as %d batches, not one",
			WorldParticleManager::Get_Batch_Count (PARTICLE_POOL_SPRITE));
	Check (WorldParticleManager::Get_Drawn_Particles (PARTICLE_POOL_SPRITE) == 512,
			"%d of 512 offered particles were drawn",
			WorldParticleManager::Get_Drawn_Particles (PARTICLE_POOL_SPRITE));
	Check (WorldParticleManager::Get_Thinned_Particles (PARTICLE_POOL_SPRITE) == 0,
			"a submission well inside the pool lost %d particles",
			WorldParticleManager::Get_Thinned_Particles (PARTICLE_POOL_SPRITE));
	Check (WorldParticleManager::Get_Pending_Particles (PARTICLE_POOL_SPRITE) == 0,
			"the flush left %d particles staged",
			WorldParticleManager::Get_Pending_Particles (PARTICLE_POOL_SPRITE));

	/*
	**	A shader difference is a state change, so it is a second batch.  If this ever merged,
	**	one of the two groups of particles would be drawn in the other's blend mode.
	*/
	WorldParticleManager::Reset_Statistics ();

	for (int i = 0; i < 32; i++) {
		ParticleSubmissionStruct	submission = Make_Submission (4, PARTICLE_BUDGET_COMBAT_NEAR, 0.0f, false);
		submission.Shader = ShaderClass ((unsigned int)(i & 3));
		Check (WorldParticleManager::Submit (PARTICLE_POOL_SPRITE, submission),
				"the pool refused a submission with shader bits %d", i & 3);
	}

	Check (WorldParticleManager::Get_Pending_Batches (PARTICLE_POOL_SPRITE) == 4,
			"thirty-two emitters using four shaders opened %d batches",
			WorldParticleManager::Get_Pending_Batches (PARTICLE_POOL_SPRITE));

	WorldParticleManager::Flush (rinfo);

	Check (WorldParticleManager::Get_Batch_Count (PARTICLE_POOL_SPRITE) == 4,
			"four shaders drew as %d batches",
			WorldParticleManager::Get_Batch_Count (PARTICLE_POOL_SPRITE));

	/*
	**	The other half of the acceptance: the pool is the budget.  Offer it four times what it
	**	can hold and it holds exactly what it holds, and every particle is accounted for either
	**	as drawn or as thinned -- a pool that quietly dropped some would still look bounded.
	*/
	WorldParticleManager::Reset_Statistics ();

	for (int i = 0; i < 64; i++) {
		ParticleSubmissionStruct	submission = Make_Submission (64, PARTICLE_BUDGET_AMBIENT, 0.0f, false);
		WorldParticleManager::Submit (PARTICLE_POOL_SPRITE, submission);
	}

	unsigned int	submitted = WorldParticleManager::Get_Submitted_Particles (PARTICLE_POOL_SPRITE);
	unsigned int	drawn = WorldParticleManager::Get_Drawn_Particles (PARTICLE_POOL_SPRITE);
	unsigned int	thinned = WorldParticleManager::Get_Thinned_Particles (PARTICLE_POOL_SPRITE);

	Check (submitted == 4096, "%u particles were offered, not 4096", submitted);
	Check (drawn <= (unsigned int)capacity, "%u particles were staged in a pool of %d", drawn, capacity);
	Check (drawn + thinned == submitted,
			"%u drawn plus %u thinned is not the %u offered", drawn, thinned, submitted);
	Check (WorldParticleManager::Get_Pool_Capacity (PARTICLE_POOL_SPRITE) == capacity,
			"the pool grew to %d under four times its capacity",
			WorldParticleManager::Get_Pool_Capacity (PARTICLE_POOL_SPRITE));

	WorldParticleManager::Flush (rinfo);

	/*
	**	That flush saw ambient demand of four times the pool, so the next accumulation runs under
	**	caps.  Critical gameplay asks for more than its reserve and must still get its reserve --
	**	the whole reason a class that is never the loudest is worth having.
	*/
	{
		ParticleSubmissionStruct	ambient = Make_Submission (4096, PARTICLE_BUDGET_AMBIENT, 0.0f, false);
		WorldParticleManager::Submit (PARTICLE_POOL_SPRITE, ambient);

		int	after_ambient = WorldParticleManager::Get_Pending_Particles (PARTICLE_POOL_SPRITE);
		Check (after_ambient < capacity,
				"a capped ambient submission took %d of a %d pool", after_ambient, capacity);

		ParticleSubmissionStruct	critical = Make_Submission (512, PARTICLE_BUDGET_CRITICAL_GAMEPLAY, 0.0f, false);
		WorldParticleManager::Submit (PARTICLE_POOL_SPRITE, critical);

		int	after_critical = WorldParticleManager::Get_Pending_Particles (PARTICLE_POOL_SPRITE) - after_ambient;
		int	reserve = capacity / 4;			// the fixed critical share

		Check (after_critical >= reserve,
				"ambient at four times the pool left critical gameplay only %d of its %d reserve",
				after_critical, reserve);
	}

	WorldParticleManager::Flush (rinfo);
	WorldParticleManager::Reset_Statistics ();

	/*
	**	A submission that names no budget class is placed by distance.  Same effect, same
	**	everything, twice -- once under the camera's nose and once across the map.
	*/
	WorldParticleManager::Set_Camera_Position (Vector3 (0.0f, 0.0f, 0.0f));
	WorldParticleManager::Set_Combat_Near_Distance (50.0f);

	{
		ParticleSubmissionStruct	close = Make_Submission (16, PARTICLE_BUDGET_AUTO, 10.0f, false);
		ParticleSubmissionStruct	distant = Make_Submission (32, PARTICLE_BUDGET_AUTO, 500.0f, false);

		WorldParticleManager::Submit (PARTICLE_POOL_SPRITE, close);
		WorldParticleManager::Submit (PARTICLE_POOL_SPRITE, distant);
	}

	Check (WorldParticleManager::Get_Budget_Particles (PARTICLE_POOL_SPRITE, PARTICLE_BUDGET_COMBAT_NEAR) == 16,
			"an effect ten units away was charged %u particles to COMBAT_NEAR",
			WorldParticleManager::Get_Budget_Particles (PARTICLE_POOL_SPRITE, PARTICLE_BUDGET_COMBAT_NEAR));
	Check (WorldParticleManager::Get_Budget_Particles (PARTICLE_POOL_SPRITE, PARTICLE_BUDGET_COMBAT_FAR) == 32,
			"an effect five hundred units away was charged %u particles to COMBAT_FAR",
			WorldParticleManager::Get_Budget_Particles (PARTICLE_POOL_SPRITE, PARTICLE_BUDGET_COMBAT_FAR));

	WorldParticleManager::Flush (rinfo);

	/*
	**	The pools are separate.  A streak is not a sprite and must not be charged to one, or a
	**	scene full of tracer trails would silently ration the smoke.
	*/
	WorldParticleManager::Reset_Statistics ();

	{
		ParticleSubmissionStruct	streak = Make_Submission (64, PARTICLE_BUDGET_COMBAT_NEAR, 0.0f, true);
		Check (WorldParticleManager::Submit (PARTICLE_POOL_STREAK, streak), "the streak pool refused a streak");
	}

	Check (WorldParticleManager::Get_Pending_Particles (PARTICLE_POOL_STREAK) == 64,
			"the streak pool staged %d of 64 streaks",
			WorldParticleManager::Get_Pending_Particles (PARTICLE_POOL_STREAK));
	Check (WorldParticleManager::Get_Pending_Particles (PARTICLE_POOL_SPRITE) == 0,
			"a streak was charged to the sprite pool");

	WorldParticleManager::Flush (rinfo);

	Check (WorldParticleManager::Get_Batch_Count (PARTICLE_POOL_STREAK) == 1,
			"one streak emitter drew as %d batches",
			WorldParticleManager::Get_Batch_Count (PARTICLE_POOL_STREAK));
	Check (WorldParticleManager::Get_Batch_Count (PARTICLE_POOL_SPRITE) == 0,
			"the sprite pool drew %d batches with nothing in it",
			WorldParticleManager::Get_Batch_Count (PARTICLE_POOL_SPRITE));

	/*
	**	Disabled has to mean the caller draws itself, because that is the only thing that makes
	**	"turn it off and you have the stock image" a statement about code rather than a hope.
	*/
	WorldParticleManager::Set_Enabled (false);

	{
		ParticleSubmissionStruct	submission = Make_Submission (8, PARTICLE_BUDGET_COMBAT_NEAR, 0.0f, false);
		Check (WorldParticleManager::Submit (PARTICLE_POOL_SPRITE, submission) == false,
				"a disabled manager took a submission the emitter then would not draw");
	}

	WorldParticleManager::Set_Enabled (true);

	/*
	**	And the table of batches is bounded like everything else: a pool asked for more distinct
	**	materials than it holds refuses the extras rather than growing or overrunning.
	*/
	WorldParticleManager::Reset_Statistics ();

	for (int i = 0; i < 256; i++) {
		ParticleSubmissionStruct	submission = Make_Submission (1, PARTICLE_BUDGET_COMBAT_NEAR, 0.0f, false);
		submission.Shader = ShaderClass ((unsigned int)i);
		WorldParticleManager::Submit (PARTICLE_POOL_SPRITE, submission);
	}

	Check (WorldParticleManager::Get_Pending_Batches (PARTICLE_POOL_SPRITE) == 64,
			"a pool asked for 256 materials opened %d batches",
			WorldParticleManager::Get_Pending_Batches (PARTICLE_POOL_SPRITE));
	Check (WorldParticleManager::Get_Refused_Submission_Count (PARTICLE_POOL_SPRITE) == 192,
			"%u of the 192 materials past the table's end were refused",
			WorldParticleManager::Get_Refused_Submission_Count (PARTICLE_POOL_SPRITE));

	WorldParticleManager::Flush (rinfo);
	WorldParticleManager::Shutdown ();

	return _Failures;
}

}	// namespace


int	ShaderSelfCheck::Run (const char *which)
{
	if (which == nullptr) {
		which = "programs";
	}

	bool	ran = false;

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "programs") == 0)) {
		Run_Programs ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "pipelines") == 0)) {
		Run_Pipelines ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "particles") == 0)) {
		Run_Particles ();
		ran = true;
	}

	if (!ran) {
		::fprintf (stderr, "unknown self check '%s'\n", which);
		return 2;
	}

	::fflush (stdout);
	::fflush (stderr);

	return (_Failures == 0) ? 0 : 1;
}
